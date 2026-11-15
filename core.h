// ============================================================
//  SMARTWATCH CORE - ESP32-S3 (Waveshare LCD-2) + ST7735 1.8" + XPT2046
// ============================================================
#pragma once
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPIFFS.h>   // 4MB flash: particao huge_app (3MB APP/1MB FS) so tem particao SPIFFS
#include <Wire.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <time.h>
#include <esp_sntp.h>
#include <esp_sleep.h>
#include <driver/gpio.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans24pt7b.h>

// ---------------- MUTEX DO BARRAMENTO SPI (TFT + SD compartilham o bus) ----------------
// Sem isso, o flush do TFT (task do LVGL, core 1) pode rodar ao mesmo tempo que
// a leitura do SD (task de audio, core 0) e corromper as duas comunicacoes.
// RECURSIVO: um SD aberto dentro de uma sessao (sdBeginSession) jah detem o
// lock, e o flush/lista do lado de dentro so pode continuar com take aninhado.
// Com mutex simples, o take aninhado estoura no timeout e o give soltava o
// baramento antes da hora (era isso que "derrubava" o SD no meio da escrita).
static SemaphoreHandle_t gBusMtx = nullptr;
static inline bool busLock(uint32_t ms = 3000) {
  if (!gBusMtx) return true;
  return xSemaphoreTakeRecursive(gBusMtx, pdMS_TO_TICKS(ms)) == pdTRUE;
}
static inline void busUnlock() {
  if (gBusMtx) xSemaphoreGiveRecursive(gBusMtx);
}

// ---------------- flags de congelamento do render ----------------
volatile bool gLvPause = false;        // congela render (calibracao direta)
volatile bool gSdPause = false;        // congela render (sessao SD mistura bus)

// ====== Para fontes arredondadas da RREFONTS: troque os defines abaixo ======
#define F_TITLE  &FreeSans12pt7b
#define F_CLOCK  &FreeSans24pt7b
// (o restante do sistema usa a fonte bitmap 5x7: e pequena e a mais rapida)

// ---------------- PINOS (ESP32-S3 SuperMini N4R2: 4MB flash + 2MB PSRAM QSPI) ----------------
// GPIO33-37 EXISTEM nesta placa (PSRAM QSPI, nao octal) e sao seguros.
#define TFT_SCK     9
#define TFT_MOSI    10
#define TFT_MISO    34
#define TFT_CS       13
#define TFT_DC      11
#define TFT_RST      12
#define TFT_BL      8
#define TOUCH_CS    36
#define TOUCH_IRQ   35
#define IR_TX_PIN   3
#define IR_RX_PIN    2
#define BAT_ADC_PIN  1
#define BAT_DIV      3.0
#define BAT_VLO      3.0
#define BAT_VHI      4.20
#define SD_SCK_PIN  9
#define SD_MISO_PIN 34
#define SD_MOSI_PIN 10
#define SD_CS_PIN   39
#define MPU_SDA_PIN 38
#define MPU_SCL_PIN 37
#define SPK_PIN      5

// ---------------- CORES ----------------
#define C_BG   0x0000
#define C_FG   0xFFFF
#define C_DIM  0x8410
#define C_ACC  0x07FF
#define C_ERR  0xF800
#define C_OK   0x07E0
#define C_BTN  0x2945

// prototipos usados pela calibracao (definidos adiante)
bool inRect(int px, int py, int x, int y, int w, int h);
void drawBtn(int x, int y, int w, int h, const char *label, uint16_t fill = C_BTN);

// ---------------- CONFIG ----------------
struct Config {
  uint8_t  rotation   = 0;
  uint8_t  brightness = 80;
  bool     darkTheme  = true;
  uint32_t sleepS     = 10;
  uint32_t tapMaxMs   = 600;
  bool     autoWifi   = true;
  bool     beepOn     = false;   // bip curto de resposta ao toque
  bool     wake2      = true;    // 2 toques p/ acordar (evita toque falso)
  uint8_t  accIdx     = 2;       // cor de destaque (2 = laranja Redmi)
  bool     esp        = false;   // tema espacial no relógio (lua/terra/sol)
  uint8_t  lockStyle  = 0;       // 0=digital 1=analogico 2=padrao
  bool     wallp      = false;   // wallpaper (foto do SD salva na SPIFFS) no lock/home
  uint32_t steps      = 0;
  String   iaKey      = "";
  String   iaModel    = "gpt-4o-mini";
  float    wLat       = -20.4428f;   // 20o26'34"S - Campo Grande, MS
  float    wLon       = -54.6464f;   // 54o38'47"W
};
Config cfg;

SPIClass        tftSPI(FSPI);
// SD usa o MESMO barramento do TFT: MOSI/SCK/MISO (9/10/34) sao compartilhados
// por hardware, com CS diferente (TFT=13, SD=39). Barramento separado com o mesmo
// MISO faz os dois perifericos SPI brigarem pelo pino -> deteccao invertida
// ("cartao conectado=ausente / desconectado=FAT"). Por isso NAO existe sdSPI(HSPI);
// o SD usa tftSPI direto (a instancia ja iniciada), com exclusao mutua via gSdPause.
// subclasse p/ expor o setColRowStart (protected na lib) - offset do painel
class ST7735of : public Adafruit_ST7735 {
 public:
  ST7735of(SPIClass *spi, int8_t cs, int8_t dc, int8_t rst)
      : Adafruit_ST7735(spi, cs, dc, rst) {}
  void offsetCR(int8_t c, int8_t r) { setColRowStart(c, r); }
};
ST7735of tft(&tftSPI, TFT_CS, TFT_DC, TFT_RST);
int  scrW() { return tft.width(); }
int  scrH() { return tft.height(); }

// ---------------- SPIFFS (era FFat: adaptado p/ SuperMini 4MB) ----------------
String fRead(const char *path) {
  File f = SPIFFS.open(path, "r"); if (!f) return "";
  String s; while (f.available()) s += (char)f.read();
  f.close(); return s;
}
bool fWrite(const char *path, const String &s) {
  File f = SPIFFS.open(path, "w"); if (!f) return false;
  f.print(s); f.close(); return true;
}
String cfgGet(const String &key) {
  String d = fRead("/config.txt");
  int i = d.indexOf(key + "="); if (i < 0) return "";
  int e = d.indexOf('\n', i);
  return d.substring(i + key.length() + 1, e < 0 ? d.length() : e);
}
void cfgPut(const String &key, const String &val) {
  String d = fRead("/config.txt");
  int i = d.indexOf(key + "=");
  if (i >= 0) {
    int e = d.indexOf('\n', i);
    d = d.substring(0, i) + key + "=" + val + "\n" +
        (e < 0 ? String("") : d.substring(e + 1));
  } else d += key + "=" + val + "\n";
  fWrite("/config.txt", d);
}
void cfgLoad() {
  int v;
  v = cfgGet("rotation").toInt();  cfg.rotation = constrain(v, 0, 3);
  v = cfgGet("bl").toInt();        cfg.brightness = v ? v : 80;
  String dk = cfgGet("dark");
  cfg.darkTheme  = (dk == "") || (dk == "1");
  v = cfgGet("sleep").toInt();     cfg.sleepS = v ? v : 10;
  v = cfgGet("tapmax").toInt();    cfg.tapMaxMs = v ? v : 600;
  cfg.autoWifi = (cfgGet("autowifi") != "0");
  cfg.beepOn   = (cfgGet("beep") == "1");
  cfg.wake2    = (cfgGet("wake2") != "0");   // padrao ligado
  v = cfgGet("acc").toInt();        cfg.accIdx = constrain(v, 0, 7);
  cfg.esp       = (cfgGet("esp") == "1");
  cfg.wallp     = (cfgGet("wall") == "1");
  v = cfgGet("lockst").toInt();     cfg.lockStyle = constrain(v, 0, 2);
  cfg.steps    = cfgGet("steps").toInt();
  cfg.iaKey    = cfgGet("iapl");
  cfg.iaModel  = cfgGet("iadm"); if (!cfg.iaModel.length()) cfg.iaModel = "gpt-4o-mini";
  cfg.wLat     = cfgGet("wxla").toFloat();
  cfg.wLon     = cfgGet("wxlo").toFloat();
  if (cfg.wLat == 0) cfg.wLat = -20.4428f;   // 20o26'34"S - Campo Grande, MS
  if (cfg.wLon == 0) cfg.wLon = -54.6464f;   // 54o38'47"W
  // migracao unica: forca a coordenada exata do usuario (20o26'34"S / 54o38'47"W)
  if (cfgGet("wxcg") != "2") { cfg.wLat = -20.4428f; cfg.wLon = -54.6464f; cfgPut("wxcg", "2"); }
  // migracao unica: aplica o visual Redmi (acento laranja) na 1a vez
  if (cfgGet("redmi") != "1") { cfg.accIdx = 2; cfgPut("redmi", "1"); }
  // migracao unica: face PADRAO como inicial (a que voce mandou na foto)
  if (cfgGet("lock2") != "1") { cfg.lockStyle = 2; cfgPut("lock2", "1"); }
}
void cfgSave() {
  char b[16];
  cfgPut("rotation", itoa(cfg.rotation, b, 10));
  cfgPut("bl",       itoa((int)cfg.brightness, b, 10));
  cfgPut("dark",     cfg.darkTheme ? "1" : "0");
  cfgPut("sleep",    itoa((int)cfg.sleepS, b, 10));
  cfgPut("tapmax",   itoa((int)cfg.tapMaxMs, b, 10));
  cfgPut("autowifi", cfg.autoWifi ? "1" : "0");
  cfgPut("beep",     cfg.beepOn ? "1" : "0");
  cfgPut("wake2",    cfg.wake2 ? "1" : "0");
  cfgPut("acc",      itoa((int)cfg.accIdx, b, 10));
  cfgPut("esp",      cfg.esp ? "1" : "0");
  cfgPut("wall",     cfg.wallp ? "1" : "0");
  cfgPut("lockst",   itoa((int)cfg.lockStyle, b, 10));
  cfgPut("steps",    itoa((long)cfg.steps, b, 10));
  cfgPut("iapl",     cfg.iaKey);
  cfgPut("iadm",     cfg.iaModel);
  cfgPut("wxla",     String(cfg.wLat, 4));
  cfgPut("wxlo",     String(cfg.wLon, 4));
}
// ---------------- HORA / NTP ----------------
// Campo Grande / MS = Amazon Time (AMT), UTC-4 fixo, sem horario de verao.
// configTime() recebe o offset "GMT"; para Oeste usamos valor negativo.
#define CLOCK_TZ_OFFSET_SEC (-4L * 3600L)
#define CLOCK_TZ_LABEL      "Campo Grande/MS  GMT-4"
static const char *CLOCK_NTP_1 = "pool.ntp.org";
static const char *CLOCK_NTP_2 = "time.nist.gov";

// Numero de sincronizacoes NTP confirmadas pelo callback do SNTP. timeOk()
// nao serve como prova de sync: ele tambem vale para a hora restaurada do SPIFFS.
static volatile uint32_t gNtpSyncCount = 0;
extern "C" {
  static void clockNtpSyncCb(struct timeval *) { gNtpSyncCount++; }
}

void clockNtpConfigure() {
  sntp_set_time_sync_notification_cb(clockNtpSyncCb);
  configTime(CLOCK_TZ_OFFSET_SEC, 0, CLOCK_NTP_1, CLOCK_NTP_2);
}

// Reinicia o SNTP e espera o callback provar que houve sincronismo real.
bool clockNtpSync(uint32_t timeoutMs = 15000) {
  uint32_t before = gNtpSyncCount;
  clockNtpConfigure();
  uint32_t t0 = millis();
  while (gNtpSyncCount == before && millis() - t0 < timeoutMs) delay(50);
  return gNtpSyncCount != before;
}

void clockRestore() {
  clockNtpConfigure();                 // TZ correta desde o boot (sem NTP ainda)
  long ep = 0;
  String epStr = cfgGet("epoch");
  if (epStr.length()) ep = strtol(epStr.c_str(), nullptr, 10);
  if (ep > 100000) { struct timeval t = {ep, 0}; settimeofday(&t, nullptr); }
}
void clockPersist() { cfgPut("epoch", String((long)time(nullptr))); }

// ---------------- BATERIA ----------------
float batVoltage() { return analogReadMilliVolts(BAT_ADC_PIN) * BAT_DIV / 1000.0f; }
int batPct() {
  return constrain((int)((batVoltage() - BAT_VLO) / (BAT_VHI - BAT_VLO) * 100.0f), 0, 100);
}

// mantem o SPK anexado no LEDC sem nunca desanexar o canal (regra do runbook)
// 200 kHz nao cabe em 8 bits com clock LEDC de 40 MHz (max ~156 kHz):
// 100 kHz da' div 1.5625 e funciona nos dois clocks.
#define SPK_CARRIER_HZ 100000UL
static void spkCarrierEnsure() {
  if (ledcAttach(SPK_PIN, SPK_CARRIER_HZ, 8)) return;
  if (ledcChangeFrequency(SPK_PIN, SPK_CARRIER_HZ, 8)) return;
  ledcChangeFrequency(SPK_PIN, 50000, 8);     // ultimo recurso (clock de 20 MHz)
}

// ---------------- SOM (alto-falante) ----------------
void beepShort(uint16_t f = 2200, uint16_t ms = 25) {   // bip de resposta ao toque
  if (!cfg.beepOn) return;
  spkCarrierEnsure();
  ledcWriteTone(SPK_PIN, f);
  delay(ms);
  ledcWriteTone(SPK_PIN, 0);
  spkCarrierEnsure();                       // devolve a portadora do player
}

// ---------------- DISPLAY ----------------
void blSet(uint8_t pct) {
  static bool res = false;
  if (!res) { analogWriteResolution(TFT_BL, 8); res = true; }  // core 3.x: (pin, bits)
  analogWrite(TFT_BL, (uint32_t)constrain(pct, 0, 100) * 255 / 100);
}
void blOn()  { blSet(cfg.brightness); }
void blOff() { blSet(0); }

void dispInit() {
  tftSPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI, -1);
  // 8 MHz e' o suficiente para a tela (quadro de 40 KB em ~5 ms) e mantem o
  // barramento compartilhado com o SD limpo. A 40 MHz (o valor que estava) o
  // clock do SCK e' tao rapido que o cartao sofria: a inicializacao respondia
  // (comandos curtos) mas o I/O de DADO (512 bytes) falhava com EIO. No
  // sketch minimo (que grava tudo) a tela usa 1 MHz, o padrao do driver.
  tft.setSPISpeed(8000000);
  tft.initR(INITR_BLACKTAB);
  tft.offsetCR(2, 1);   // offset do painel: +2 colunas E +1 linha (lixo no rodape)
  tft.setRotation((cfg.rotation & 1) ? 2 : 0);   // rotacao pelo MADCTL do painel (sem sw_rotate)
  tft.fillScreen(C_BG);
  blSet(cfg.brightness);
}

// ---------------- TOPBAR (todas as telas) ----------------
void uiTopbar(bool force = false) {
  static time_t lastT = 0; static int lastP = -1; static uint32_t t0 = 0;
  if (!force && millis() - t0 < 1000) return;
  t0 = millis();
  time_t now = time(nullptr); struct tm *l = localtime(&now);
  if (now != lastT || force) {
    lastT = now;
    char hh[6]; strftime(hh, sizeof(hh), "%H:%M", l);
    tft.fillRect(0, 0, 40, 13, C_BG);
    tft.setTextSize(1); tft.setTextColor(C_FG, C_BG);
    tft.setCursor(2, 4); tft.print(hh);
  }
  int p = batPct();
  if (p != lastP || force) {
    lastP = p;
    char s[8]; snprintf(s, sizeof(s), "%d%%", p);
    int w = tft.width();
    tft.fillRect(w - 40, 0, 40, 13, C_BG);
    tft.setTextColor(p < 20 ? C_ERR : C_FG, C_BG);
    tft.setCursor(w - 38, 4); tft.print(s);
  }
  tft.drawFastHLine(0, 13, tft.width(), C_DIM);
}

// ---------------- TOUCH (XPT2046) ----------------
// A calibracao mapeia 2 eixos crus com min/max + POLARIDADE (xrev/yrev),
// para tocar espelhado ou girado a calibracao sai correta de primeira.
struct CalData {
  bool swap = false;              // true: eixo cru 2 (rv) vira X da tela... orientacao detectada
  bool xrev = false, yrev = false; // polaridade de cada eixo (raw cresce p/ esquerda/baixo)
  int  sxLo = 300, sxHi = 3850;   // eixo cru mapeado p/ X (lo..hi)
  int  syLo = 300, syHi = 3850;   // eixo cru mapeado p/ Y (lo..hi)
};
CalData cal;
volatile int gZ1 = -1, gZ2 = -1;          // ultima leitura de pressao (diag)

uint16_t tsReadCh(uint8_t cmd) {
  // O XPT2046 divide SCK/MOSI/MISO com o SD: se um toque for lido no meio de
  // uma transferencia do cartao, os clocks extra corrompem os dois (e um write
  // abortado deixa o cartao fora do estado SPI ate' o power cycle).
  if (gSdPause) return 0;               // sessao SD aberta: nao mexe no bus
  if (!busLock(30)) return 0;           // outra task usando o bus (audio/tela)
  tftSPI.beginTransaction(SPISettings(2500000, MSBFIRST, SPI_MODE0));
  digitalWrite(TOUCH_CS, LOW);
  tftSPI.transfer(cmd);
  uint16_t v = ((uint16_t)tftSPI.transfer(0) << 8) | tftSPI.transfer(0);
  digitalWrite(TOUCH_CS, HIGH);
  tftSPI.endTransaction();
  busUnlock();
  return v >> 4;
}
bool tsRaw(int *rx, int *ry) {
  if (digitalRead(TOUCH_IRQ) != LOW) return false;
  int z1 = tsReadCh(0xB0), z2 = tsReadCh(0xC0);
  gZ1 = z1; gZ2 = z2;                     // diagnostico (heartbeat no loop)
  if (z2 - z1 < 200) return false;
  *rx = tsReadCh(0x90); *ry = tsReadCh(0xD0);
  return true;
}
// media de varias amostras enquanto o dedo estiver em contato (ruído↓)
bool tsSampleAvg(int *rx, int *ry) {
  int sr = 0, sy = 0, n = 0, r, y;
  uint32_t t0 = millis();
  while (n < 8) {
    if (tsRaw(&r, &y)) { sr += r; sy += y; n++; delay(3); }
    if (millis() - t0 > 1200) break;
  }
  if (n < 3) return false;
  *rx = sr / n; *ry = sy / n;
  return true;
}
// raw -> coordenadas da ROTACAO 0 (128 x 160)
void tsMap0(int rx, int ry, int *x0, int *y0) {
  int ax = cal.swap ? ry : rx;
  int ay = cal.swap ? rx : ry;
  *x0 = map(constrain(ax, cal.sxLo, cal.sxHi), cal.sxLo, cal.sxHi, 0, 127);
  *y0 = map(constrain(ay, cal.syLo, cal.syHi), cal.syLo, cal.syHi, 0, 159);
  if (cal.xrev) *x0 = 127 - *x0;   // raw cresce p/ esquerda
  if (cal.yrev) *y0 = 159 - *y0;   // raw cresce p/ baixo
}
void tsRot(int x0, int y0, int *xx, int *yy) {
  switch (tft.getRotation()) {
    case 1: *xx = 159 - y0; *yy = x0; break;
    case 2: *xx = 127 - x0; *yy = 159 - y0; break;
    case 3: *xx = y0;       *yy = 127 - x0; break;
    default: *xx = x0;      *yy = y0; break;
  }
  *xx = constrain(*xx, 0, tft.width() - 1);
  *yy = constrain(*yy, 0, tft.height() - 1);
}
bool tsPoint(int *xx, int *yy) {
  int rx, ry, x0, y0;
  if (!tsRaw(&rx, &ry)) return false;
  tsMap0(rx, ry, &x0, &y0);
  tsRot(x0, y0, xx, yy);
  return true;
}
void calSave() {
  bool ok = fWrite("/cal.txt", "c2 " + String(cal.swap ? 1 : 0) + " " + String(cal.xrev ? 1 : 0) + " " +
         String(cal.yrev ? 1 : 0) + " " + String(cal.sxLo) + " " + String(cal.sxHi) + " " +
         String(cal.syLo) + " " + String(cal.syHi));
  Serial.print("[cal] salva "); Serial.print(ok ? "OK: " : "FALHOU! "); Serial.println("/cal.txt");
}
bool calLoad() {
  String d = fRead("/cal.txt");
  if (!d.startsWith("c2")) return false;   // formato antigo -> recalibrar
  int v[7] = {0}, n = 0, p = 3, len = d.length();   // p=3: após "c2 " (2 era o espaço!)
  while (n < 7 && p < len) {
    int e = d.indexOf(' ', p); if (e < 0) e = len;
    v[n++] = d.substring(p, e).toInt(); p = e + 1;
  }
  if (n < 7) return false;                 // versao antiga -> recalibrar
  cal.swap = v[0] != 0; cal.xrev = v[1] != 0; cal.yrev = v[2] != 0;
  cal.sxLo = v[3]; cal.sxHi = v[4]; cal.syLo = v[5]; cal.syHi = v[6];
  if ((cal.sxHi - cal.sxLo) > 500 && (cal.syHi - cal.syLo) > 500) {
    Serial.print("[cal] carregada: swap="); Serial.print(cal.swap);
    Serial.print(" xrev="); Serial.print(cal.xrev); Serial.print(" yrev="); Serial.println(cal.yrev);
    Serial.print("[cal] "); Serial.print(d); Serial.println();
    return true;
  }
  Serial.print("[cal] INVALIDA, recalibrando: file=\""); Serial.print(d);
  Serial.print("\" n="); Serial.println(n);
  return false;
}

// calibracao guiada: 4 alvos visuais, deteccao automatica do eixo,
// tela de verificação com o dedo; so salva se o usuario confirmar
void calibRun() {
  uint8_t oldRot = tft.getRotation();
  blOn();
  tft.setRotation(0);
  bool okAll = false;
  while (!okAll) {
    tft.setTextSize(1);
    const int   tX[4] = {10, 118, 118, 10};
    const int   tY[4] = {18, 18, 152, 152};
    const char *tL[4] = {"canto SUPERIOR ESQ", "canto SUPERIOR DIR",
                         "canto INFERIOR DIR", "canto INFERIOR ESQ"};
    int rv[4], ry[4];
    for (int i = 0; i < 4; i++) {
      tft.fillScreen(C_BG);
      // alvo: crosshair + circulo
      tft.drawFastHLine(tX[i] - 5, tY[i], 11, C_ACC);
      tft.drawFastVLine(tX[i], tY[i] - 5, 11, C_ACC);
      tft.drawCircle(tX[i], tY[i], 7, C_ACC);
      tft.setTextColor(C_FG, C_BG);
      tft.setCursor(8, 44);  tft.print("Calibracao ("); tft.print(i + 1); tft.print("/4)");
      tft.setCursor(8, 58);  tft.print(tL[i]);
      tft.setTextColor(C_DIM, C_BG);
      tft.setCursor(8, 96);  tft.print("Aperte EXATAMENTE no X");
      tft.setCursor(8, 110); tft.print("ate o X ficar VERDE");
      int cr = 0, cy = 0;
      while (!tsRaw(&cr, &cy)) delay(5);          // espera o toque
      tsSampleAvg(&cr, &cy);                      // estabiliza
      tft.fillCircle(tX[i], tY[i], 7, C_OK);      // feedback verde
      int pxx, pyy;
      while (tsPoint(&pxx, &pyy)) delay(10);      // espera soltar
      delay(150);
      rv[i] = cr; ry[i] = cy;
    }
    // qual eixo cru acompanha o movimento horizontal da tela?
    int dHx = abs(rv[1] - rv[0]) + abs(rv[2] - rv[3]);   // eixo cruz X na horizontal
    int dHy = abs(ry[1] - ry[0]) + abs(ry[2] - ry[3]);   // eixo cruz Y na horizontal
    cal.swap = dHy > dHx;
    // polaridade: ESQ = canto 0/3 e TOPO = canto 0/1 na tela (rot 0).
    // se o raw do lado esquerdo for MAIOR que o direito, o eixo esta invertido.
    int a0, a1, b0, b1;   // eixo-X: cru na ESQ / DIR   |   eixo-Y: cru no TOPO / BASE
    if (cal.swap) {                       // eixo vertical cru (ry) = X da tela
      a0 = (ry[0] + ry[3]) / 2;  a1 = (ry[1] + ry[2]) / 2;   // X cru: esq/dir
      b0 = (rv[0] + rv[1]) / 2;  b1 = (rv[2] + rv[3]) / 2;   // Y cru: topo/base
    } else {                              // eixo horizontal cru (rv) = X da tela
      a0 = (rv[0] + rv[3]) / 2;  a1 = (rv[1] + rv[2]) / 2;
      b0 = (ry[0] + ry[1]) / 2;  b1 = (ry[2] + ry[3]) / 2;
    }
    cal.sxLo = min(a0, a1); cal.sxHi = max(a0, a1); cal.xrev = a0 > a1;
    cal.syLo = min(b0, b1); cal.syHi = max(b0, b1); cal.yrev = b0 > b1;
    // ---------- VERIFICACAO: siga o dedo ----------
    tft.fillScreen(C_BG);
    tft.setTextColor(C_FG, C_BG);
    tft.setCursor(4, 8);  tft.print("Mova o dedo pela tela");
    tft.setCursor(4, 18); tft.print("e confira se o ponto segue:");
    const int okX = 6, okW = 42, rfX = 54, rfW = 72, bY = 142, bH = 16;
    drawBtn(okX, bY, okW, bH, "OK", C_OK);
    drawBtn(rfX, bY, rfW, bH, "REFAZER", C_ERR);
    bool decided = false, confirmed = false, wasDown = false;
    uint32_t holdMs = 0;
    while (!decided) {
      int cx, cy;
      bool down = tsPoint(&cx, &cy);
      if (down) {
        if (wasDown) {
          // rastro do dedo (fora da area dos botoes)
          tft.fillCircle(constrain(cx, 10, 117), constrain(cy, 24, 137), 2, C_ACC);
          // deslizou ate um botao e segurou la ~600ms? confirma tambem
          if (millis() - holdMs > 600) {
            if (inRect(cx, cy, okX, bY, okW, bH))      { confirmed = true; decided = true; }
            else if (inRect(cx, cy, rfX, bY, rfW, bH)) decided = true;
          }
        } else {   // toque NOVO: decide por onde encostou
          int r2, y2r, x2, yy2;
          if (tsSampleAvg(&r2, &y2r)) {   // re-amostra: bordas do resistivo sao ruidosas
            tsMap0(r2, y2r, &x2, &yy2);
            tsRot(x2, yy2, &cx, &cy);
          }
          if (inRect(cx, cy, okX, bY, okW, bH))      { confirmed = true; decided = true; }
          else if (inRect(cx, cy, rfX, bY, rfW, bH)) decided = true;
          else tft.fillCircle(constrain(cx, 10, 117), constrain(cy, 24, 137), 2, C_ACC);
        }
      }
      if (down && !wasDown) holdMs = millis();
      wasDown = down;
      uiTopbar();
      delay(4);
    }
    if (confirmed) {
      calSave();
      tft.fillScreen(C_BG);
      tft.setTextColor(C_OK); tft.setTextSize(1);
      tft.setCursor(12, 70); tft.print("Calibrado!");
      delay(700);
      okAll = true;
    }
  }
  tft.setRotation(oldRot);
  tft.fillScreen(C_BG);
}

// ---------------- WI-FI SAVED ----------------
String wifiSsid[10], wifiPass[10]; int wifiN = 0;
// ---------------- REDE DO MODO SERVIDOR (upload) ----------------
// O AP so' sobe quando nao ha wifi salvo conectado. Com wifi conectado, o
// usuario abre o navegador no IP da rede e o relogio mostra o IP na tela.
String gSrvIp = "";        // IP para digitar no navegador
bool   gSrvAp = true;      // true = modo AP (WearSend/12345678)
String gSrvNet = "";       // nome da rede (AP ou wifi conectado)
void wifiLoad() {
  String d = fRead("/wifi.txt"); wifiN = 0; int p = 0;
  while (wifiN < 10 && p < (int)d.length()) {
    int e = d.indexOf('\n', p); if (e < 0) e = d.length();
    String line = d.substring(p, e); p = e + 1;
    int t = line.indexOf('\t'); if (t < 0) continue;
    wifiSsid[wifiN] = line.substring(0, t);
    wifiPass[wifiN] = line.substring(t + 1);
    wifiN++;
  }
}
void wifiStore() {
  String d = "";
  for (int i = 0; i < wifiN; i++) d += wifiSsid[i] + "\t" + wifiPass[i] + "\n";
  fWrite("/wifi.txt", d);
}
void wifiAdd(String s, String p) {
  for (int i = 0; i < wifiN; i++) if (wifiSsid[i] == s) { wifiPass[i] = p; wifiStore(); return; }
  if (wifiN < 10) { wifiSsid[wifiN] = s; wifiPass[wifiN] = p; wifiN++; }
  wifiStore();
}
void wifiDel(int i) {
  if (i < 0 || i >= wifiN) return;
  for (int j = i; j < wifiN - 1; j++) { wifiSsid[j] = wifiSsid[j + 1]; wifiPass[j] = wifiPass[j + 1]; }
  wifiN--; wifiStore();
}
bool wifiSavedConnect() {
  if (wifiN == 0) return false;
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);          // SEM power save: handshake TLS nao estoura
  for (int att = 0; att < 3 && WiFi.status() != WL_CONNECTED; att++) {   // ate 30s
    WiFi.disconnect(false);
    delay(120);
    WiFi.begin(wifiSsid[0].c_str(), wifiPass[0].c_str());
    uint32_t t = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t < 9000) delay(80);
  }
  if (WiFi.status() == WL_CONNECTED) {
    clockNtpConfigure();               // reinicia SNTP com o fuso de Campo Grande
    return true;
  }
  return false;
}
// ============================================================
//  SD: info do cartao + conversao de formato
//  (exFAT/FAT32 -> FAT16, com aviso: exige formatar o cartao)
// ============================================================
#include <SD.h>
#include <FS.h>

struct SdInfoT {
  bool     ok = false;
  uint8_t  fat = 0;            // 12/16/32 (32 = exFAT quando exFAT=true)
  bool     exfat = false;      // fat==32 + assinatura exFAT
  uint64_t totalMB = 0;
  uint64_t usedMB = 0;
  char     label[24] = { 0 };
};
static SdInfoT gSd;

// detecta FAT/exFAT pelo setor de boot.
// Verifica offsets comuns: 0x52 (FAT12/16/32 string) e 0x36 (media descriptor area).
// Retorna 0 se nao conseguir determinar -> o chamador assume FAT32 como fallback.
static uint8_t bootFatType(uint8_t *mbr) {
  // Offset 0x52: string "FAT12", "FAT16" ou "FAT32" (comum em muitos cartoes)
  if (mbr[0x52] == 'F' && mbr[0x53] == 'A' && mbr[0x54] == 'T') {
    if (mbr[0x55] == '3' && mbr[0x56] == '2') return 32;
    if (mbr[0x55] == '1' && mbr[0x56] == '6') return 16;
    if (mbr[0x55] == '1' && mbr[0x56] == '2') return 12;
  }
  // Offset 0x36: alguns setores de boot armazenam o tipo aqui
  if (mbr[0x36] == 'F' && mbr[0x37] == 'A' && mbr[0x38] == 'T') {
    if (mbr[0x39] == '3' && mbr[0x3a] == '2') return 32;
    if (mbr[0x39] == '1' && mbr[0x3a] == '6') return 16;
    if (mbr[0x39] == '1' && mbr[0x3a] == '2') return 12;
  }
  // exFAT: assinatura no offset 0x03
  if (mbr[0x03] == 'E' && mbr[0x04] == 'X' && mbr[0x05] == 'F') return 32;
  return 0;
}
// monta o SD no barramento do TFT (tftSPI) com algumas tentativas:
// microSD clones/poor-contact precisam de retry com pequeno intervalo.
// tries=2 no caminho da UI (falha rapida quando nao ha cartao), 5 no diagnostico.
// Frequencia do SD. Medido no hardware:
//  - A INICIALIZACAO (que a lib faz a 400 kHz) sempre funciona: CMD0, CMD8,
//    CMD58, ACMD41 e a leitura do CSD respondem.
//  - A 4 MHz o DADO some: leitura de setor e escrita falham (leu=0). O
//    barramento e' compartilhado com o TFT (que sobe a 40 MHz) e o nivel do
//    shifter nao aguenta 4 MHz nesse setup.
//  - A 400 kHz a LEITURA funciona (o explorador lista os arquivos).
// Entao o relogio fica a 400 kHz, que e' a velocidade em que o cartao conversa.
// (o SD_Test do core grava a 4 MHz, mas sem o TFT no barramento).
static const uint32_t SD_SPI_HZ = 400000UL;
// O touch (XPT2046) divide o MISO (GPIO34) com o SD. Com o CS dele em
// baixo/flutuando, o controlador DIRIGE a linha e o cartao nao responde a
// comando nenhum (nem monta). Este e' o motivo do "SD nao montou".
// Durante qualquer uso do SD o CS do touch fica em ALTO.
static inline void sdBusPrepare() {
  bool got = busLock(300);            // recursivo: pode ser chamada ja' com o lock
  pinMode(TOUCH_CS, OUTPUT);
  digitalWrite(TOUCH_CS, HIGH);       // touch fora do MISO
  // alguns clocks com o touch deselecionado: se o XPT2046 ficou segurando o
  // ultimo bit no MISO, esses clocks limpam a linha antes do comando do SD.
  if (got) {
    tftSPI.beginTransaction(SPISettings(SD_SPI_HZ, MSBFIRST, SPI_MODE0));
    for (int i = 0; i < 8; i++) tftSPI.transfer(0xFF);
    tftSPI.endTransaction();
    busUnlock();
  }
}
// ---------------- MONTAGEM DO SD (esquema do teste 4, o unico que grava) ----------------
// Regras:
//  - O CS do touch fica em ALTO (ele divide o MISO com o SD).
//  - Sequencia de power-up antes de cada tentativa (CS alto + 256 clocks).
//  - Nao desmontar no fluxo normal: um begin() que falhou ja limpa o registro
//    do VFS internamente, entao repetir begin() e' seguro. SD.end() so' existe
//    no webSdRecover(), sempre seguido de SD.begin() (teste_sd provou que o par
//    volta a montar; o begin refaz init + CMD59 do CRC).
//  - O cartao fica montado o boot inteiro; todo acesso e' serializado pelo
//    barramento (gBusMtx) e com o render pausado (gSdPause).
static bool sdTryBegin(int tries = 5) {
  for (int a = 0; a < tries; a++) {
    sdBusPrepare();                     // touch fora do MISO + 8 clocks
    pinMode(SD_CS_PIN, OUTPUT);
    digitalWrite(SD_CS_PIN, HIGH);
    if (a == 0) delay(800);             // cartao recem-ligado: regulador assentando
    tftSPI.beginTransaction(SPISettings(SD_SPI_HZ, MSBFIRST, SPI_MODE0));
    for (int i = 0; i < 32; i++) tftSPI.transfer(0xFF);   // 256 clocks, CS alto
    tftSPI.endTransaction();
    if (SD.begin(SD_CS_PIN, tftSPI, SD_SPI_HZ, "/sd", 8)) return true;   // 8 handles: upload + player + lista
    if (a < tries - 1) delay(400);
  }
  return false;
}
static void sdProbe() {
  gSd = SdInfoT();
  // Monta na hora: sem isto, cardType()/readRAW falavam num SD nao-iniciado e
  // davam leitura invertida (conectado=ausente / desconectado=FAT).
  // Mesmo barramento do TFT (tftSPI); SD.end() no final nao afeta o TFT.
  gSdPause = true;                      // pausa o render do TFT (bus compartilhado)
  busLock();
  // UNKNOWN = o driver tentou reinicializar e falhou com o VFS ainda
  // registrado: sem desmontar, o begin() sairia na hora e o explorador diria
  // que o cartao esta' ok com o FS morto. Par obrigatorio: end + begin.
  if (SD.cardType() == CARD_UNKNOWN) { SD.end(); delay(300); }
  if (!sdTryBegin()) { busUnlock(); gSdPause = false; return; }
  sdcard_type_t ct = SD.cardType();
  gSd.ok = (ct == CARD_MMC || ct == CARD_SD || ct == CARD_SDHC);
  if (!gSd.ok) { busUnlock(); gSdPause = false; return; }   // sem SD.end(): envenena a montagem
  gSd.totalMB = (uint64_t)(SD.cardSize() / 1048576ULL);
  uint8_t mbr[512];
  bool rawOk = false;
  for (int rb = 0; rb < 5 && !rawOk; rb++) {         // setor 0 (retry: leitura falha as vezes)
    if (SD.readRAW((uint8_t *)mbr, 0) > 0) {
      rawOk = true;
      gSd.fat = bootFatType(mbr);
      gSd.exfat = (gSd.fat == 32 && !memcmp(mbr + 0x03, "EXFAT", 5));
    } else delay(30);
  }
  // FALLBACK: se nao conseguiu ler o setor de boot mas o SD esta presente,
  // assume FAT32 (o formato mais comum em cartoes microSD)
  if (rawOk && !gSd.fat && !gSd.exfat) {
    gSd.fat = 32;   // assumir FAT32 como padrao
    Serial.println("[sd] FAT nao detectado pelo setor de boot: assumindo FAT32");
  }
  // usado = total - livre
  uint64_t totalB = (uint64_t)SD.totalBytes();
  uint64_t freeB  = totalB - (uint64_t)SD.usedBytes();
  gSd.usedMB = (totalB - freeB) / 1048576ULL;
  if (!gSd.exfat && gSd.fat) {
    File r = SD.open("/");
    if (r) {
      r.rewindDirectory();
      File e = r.openNextFile();
      while (e) {
        if (e.isDirectory() && !strcmp(e.name(), "                ")) { }
        e.close();
        e = r.openNextFile();
      }
      r.close();
    }
  }
  if (!gSd.label[0]) strcpy(gSd.label, "(sem rotulo)");
  // NAO desmonta no fluxo normal: o cartao fica montado e o barramento
  // (compartilhado com o TFT) e' serializado por busLock/gSdPause.
  pinMode(SD_CS_PIN, OUTPUT); digitalWrite(SD_CS_PIN, HIGH);
  busUnlock();
  gSdPause = false;
}
// texto de uma linha p/ UI
static String sdInfoTxt() {
  if (!gSd.ok) return "SD ausente";
  char b[48];

  const char *tipo;
  if (gSd.exfat)        tipo = "exFAT";
  else if (gSd.fat == 16) tipo = "FAT16";
  else if (gSd.fat == 12) tipo = "FAT12";
  else                  tipo = "FAT32";   // fallback: nao detectou, assume FAT32

  snprintf(b, sizeof(b), "%s  %llu MB  usado %llu",
           tipo,
           (unsigned long long)gSd.totalMB, (unsigned long long)gSd.usedMB);

  return String(b);
}
// converte exFAT/FAT32 -> FAT16: nao ha conversao in-place possivel
// (exige copiar os dados e reformatar); aqui expomos o STATUS + passo a passo.
static bool sdConvFat16(String &msg) {
  if (!gSd.ok) { msg = "SD ausente"; return false; }
  if (gSd.fat == 16) { msg = "ja esta em FAT16"; return true; }
  msg = gSd.exfat
    ? "exFAT -> FAT16: copie os arquivos p/ outro meio e formate o cartao em FAT16 (exFAT nao e suportado pelo ESP32-SD padrao)"
    : "FAT32 -> FAT16: use o utilitario FAT32->FAT16 no PC (guiformat/rufus) ou copie + formate";
  return false;
}

// ---------------- HORA / MPU ----------------
bool timeOk() { return time(nullptr) > 100000; }
void manualTimeSet(int y, int mo, int d, int h, int mi, int s) {
  struct tm l = {};
  l.tm_year = y - 1900; l.tm_mon = mo - 1; l.tm_mday = d;
  l.tm_hour = h; l.tm_min = mi; l.tm_sec = s;
  struct timeval tv = {mktime(&l), 0};
  settimeofday(&tv, nullptr);
}
uint8_t MPU_CUR_ADDR = 0;
bool mpuInit() {
  Wire.begin(MPU_SDA_PIN, MPU_SCL_PIN, 400000);
  uint8_t cand[2] = {0x68, 0x69};
  for (int i = 0; i < 2; i++) {
    Wire.beginTransmission(cand[i]);
    if (Wire.endTransmission() == 0) { MPU_CUR_ADDR = cand[i]; break; }
    if (i == 1) return false;
  }
  Wire.beginTransmission(MPU_CUR_ADDR); Wire.write(0x6B); Wire.write(0); Wire.endTransmission();
  Wire.beginTransmission(MPU_CUR_ADDR); Wire.write(0x1C); Wire.write(0); Wire.endTransmission();
  return true;
}
bool mpuReadAcc(int16_t *ax, int16_t *ay, int16_t *az) {
  Wire.beginTransmission(MPU_CUR_ADDR); Wire.write(0x3B);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((int)MPU_CUR_ADDR, 6) != 6) return false;
  *ax = (Wire.read() << 8) | Wire.read();
  *ay = (Wire.read() << 8) | Wire.read();
  *az = (Wire.read() << 8) | Wire.read();
  return true;
}
bool mpuStepEdge(bool &arm) {
  int16_t a, b, c;
  if (!mpuReadAcc(&a, &b, &c)) return false;
  float g = sqrtf((float)a * a + (float)b * b + (float)c * c) / 16384.0f;
  if (g > 1.25f && !arm) { arm = true; return false; }
  if (g < 1.05f && arm) { arm = false; return true; }
  return false;
}

// ---------------- UI HELPERS ----------------
bool inRect(int px, int py, int x, int y, int w, int h) {
  return px >= x && px < x + w && py >= y && py < y + h;
}
void drawBtn(int x, int y, int w, int h, const char *label, uint16_t fill) {
  tft.fillRoundRect(x, y, w, h, 3, fill);
  tft.drawRoundRect(x, y, w, h, 3, C_FG);
  tft.setTextSize(1); tft.setTextColor(C_FG);
  int tw = strlen(label) * 6;
  tft.setCursor(x + w / 2 - tw / 2, y + h / 2 - 4);
  tft.print(label);
}
void appTitle(const char *title) {           // barra superior dos apps
  uiTopbar();
  drawBtn(2, 18, 22, 16, "X", C_ERR);
  tft.setTextSize(1); tft.setTextColor(C_FG, C_BG); tft.setTextWrap(false);
  tft.setCursor(30, 27); tft.print(title);
  tft.drawFastHLine(0, 36, tft.width(), C_DIM);
}
bool hitBack(int px, int py) { return inRect(px, py, 2, 18, 22, 16); }
void tsWaitRelease() {
  int x, y;
  while (tsPoint(&x, &y)) delay(10);
}
void msgScreen(const char *line1, const char *line2 = "", uint16_t col = C_FG) {
  tft.fillRect(0, 60, tft.width(), 50, C_BG);
  tft.drawRect(0, 60, tft.width(), 50, col);
  tft.setTextColor(col); tft.setTextSize(1);
  tft.setCursor(8, 78); tft.print(line1);
  tft.setCursor(8, 96); tft.print(line2);
}

// ---------------- TECLADO (modal) ----------------
bool keyboard(String &out, const char *prompt) {
  const char *R1[4] = {"1234567890", "qwertyuiop", "asdfghjkl", "zxcvbnm"};
  const char *R2[4] = {"!@#$%()&*'-", "~^?;:|=+<>/", "\".,[]{}\\", "_@"};
  String initial = out;
  bool sym = false;
  struct Key { int x, y, w, h; };
  Key ks[48]; char kc[48]; int nk = 0;

  auto buildKeys = [&]() {
    nk = 0;
    for (int r = 0; r < 4; r++) {
      const char *row = sym ? R2[r] : R1[r];
      for (uint i = 0; i < strlen(row) && nk < 44; i++) {
        ks[nk] = {2 + i * 13, 66 + r * 16, 12, 14};
        kc[nk] = row[i];
        nk++;
      }
    }
    int y = 66 + 4 * 16 - 2;
    int x = 2;
    ks[nk] = {x, y, 20, 14}; kc[nk] = '#'; nk++;          // simbolos
    x += 22;
    ks[nk] = {x, y, 38, 14}; kc[nk] = ' '; nk++;          // espaco
    x += 40;
    ks[nk] = {x, y, 20, 14}; kc[nk] = '<'; nk++;          // apagar
    x += 22;
    ks[nk] = {x, y, 20, 14}; kc[nk] = '>'; nk++;          // OK
  };
  auto findKey = [&](int px, int py) -> int {
    for (int i = 0; i < nk; i++)
      if (inRect(px, py, ks[i].x, ks[i].y, ks[i].w, ks[i].h)) return i;
    return -1;
  };
  auto paintKey = [&](int i, bool hi) {
    if (i < 0 || i >= nk) return;
    auto &k = ks[i];
    tft.fillRoundRect(k.x, k.y, k.w, k.h, 2, hi ? C_ACC : C_BTN);
    tft.drawRoundRect(k.x, k.y, k.w, k.h, 2, C_FG);
    tft.setTextColor(C_FG); tft.setTextSize(1);
    char buf[2] = {kc[i], 0};
    const char *lab = (kc[i] == ' ') ? "_" : buf;
    tft.setCursor(k.x + k.w / 2 - strlen(lab) * 3, k.y + k.h / 2 - 4);
    tft.print(lab);
  };
  auto paintText = [&](String cur) {
    tft.fillRect(0, 20, tft.width(), 26, C_BG);
    tft.drawRect(0, 20, tft.width(), 26, C_DIM);
    tft.setTextColor(C_FG); tft.setTextSize(1); tft.setTextWrap(false);
    tft.setCursor(4, 30); tft.print(prompt);
    String v = cur.length() > 20 ? cur.substring(cur.length() - 20) : cur;
    tft.setCursor(4, 42); tft.print(v);
  };
  tft.fillScreen(C_BG);
  uiTopbar(true);
  drawBtn(2, 46, 60, 15, "CANCELAR", C_ERR);
  drawBtn(66, 46, 60, 15, "ENVIAR", C_OK);
  paintText(out);
  buildKeys();
  for (int i = 0; i < nk; i++) paintKey(i, false);

  int hi = -1;
  while (true) {
    int px, py;
    bool p = tsPoint(&px, &py);
    int idx = p ? findKey(px, py) : -1;
    if (p && idx != hi) {
      if (hi >= 0) paintKey(hi, false);
      hi = idx;
      if (hi >= 0) {
        paintKey(hi, true);
        char c = kc[hi];
        if (c == '>') { tsWaitRelease(); return true; }
        if (c == '<') { if (out.length()) out.remove(out.length() - 1); }
        else if (c == ' ') out += ' ';
        else if (c == '#') { sym = !sym; buildKeys(); for (int i = 0; i < nk; i++) paintKey(i, false); }
        else out += c;
      }
      paintText(out);
    }
    if (!p && hi >= 0) { paintKey(hi, false); hi = -1; }
    if (p && inRect(px, py, 2, 46, 60, 15))  { tsWaitRelease(); return false; }
    if (p && inRect(px, py, 66, 46, 60, 15)) { tsWaitRelease(); return true; }
    delay(8);
  }
}
