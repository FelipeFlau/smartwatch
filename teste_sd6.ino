// ============================================================
//  TESTE DO SD — usando a PROPRIA lib do core (sem SPI na mao)
//
//  Responde, em ordem, sem contaminar um teste com o outro:
//    A) monta + grava com o cartao recem-inicializado  (tem que dar OK)
//    B) o estado do cartao (CMD13) antes e depois de 30 s de tela
//    C) grava DEPOIS da tela                            (o teste A/B de verdade)
//    D) le o setor 0 por FORA do FatFs (SD.readRAW)      -> driver x FATFS
//    E) SD.end() + SD.begin() (re-init da lib) + grava   -> da' para reviver?
//
//  Knobs (troque e rode de novo para isolar a causa):
//    ANIMA_MS  30 000 = anima a tela | 0 = CONTROLE (sem animacao)
//    ANIMA_LUZ 1 = luz acesa na animacao | 0 = luz apagada (teste de energia)
//    TFT_HZ    clock da tela: 8 MHz (atual) | 1 MHz (teste de barramento)
//
//  ANTES DE RODAR: deixe a energia do cartao/board desligada ~30 s.
//
//  Pinos: SCK=9 MISO=34 MOSI=10 | TFT CS=13 DC=11 RST=12 BL=8
//         SD CS=39 | touch CS=36 IRQ=35 | IR=3
// ============================================================
#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SD.h>
#include <SPIFFS.h>

#define TFT_SCK   9
#define TFT_MISO  34
#define TFT_MOSI  10
#define TFT_CS    13
#define TFT_DC    11
#define TFT_RST   12
#define TFT_BL    8
#define SD_CS     39
#define TOUCH_CS  36
#define TOUCH_IRQ 35
#define IR_TX_PIN 3

#define SD_HZ     400000
#define TFT_HZ    8000000     // <<< 1000000 = teste do barramento
#define ANIMA_MS  30000       // <<< 0 = controle sem animacao
#define ANIMA_LUZ 1           // <<< 0 = anima com a luz apagada

SPIClass tftSPI(FSPI);
Adafruit_ST7735 tft(&tftSPI, TFT_CS, TFT_DC, TFT_RST);

// ------------------------------------------------------------
//  CMD13 (SEND_STATUS): so' prova se o cartao ACEITA o comando
//  ATENCAO: em SPI o R2 tem 16 bits = status bits 15..0, que NAO
//  incluem o CURRENT_STATE (esse so' existe no status de 32 bits
//  do modo SD). Entao NAO da' para ler "idle/tran/rcv" por aqui:
//  o que vale e' r1=00 (aceitou) x 0xFF (nao respondeu) x 0x05 (ilegal).
// ------------------------------------------------------------
static const uint8_t CRC7T[256] = {
  0x00,0x09,0x12,0x1B,0x24,0x2D,0x36,0x3F,0x48,0x41,0x5A,0x53,0x6C,0x65,0x7E,0x77,
  0x19,0x10,0x0B,0x02,0x3D,0x34,0x2F,0x26,0x51,0x58,0x43,0x4A,0x75,0x7C,0x67,0x6E,
  0x32,0x3B,0x20,0x29,0x16,0x1F,0x04,0x0D,0x7A,0x73,0x68,0x61,0x5E,0x57,0x4C,0x45,
  0x2B,0x22,0x39,0x30,0x0F,0x06,0x1D,0x14,0x63,0x6A,0x71,0x78,0x47,0x4E,0x55,0x5C,
  0x64,0x6D,0x76,0x7F,0x40,0x49,0x52,0x5B,0x2C,0x25,0x3E,0x37,0x08,0x01,0x1A,0x13,
  0x7D,0x74,0x6F,0x66,0x59,0x50,0x4B,0x42,0x35,0x3C,0x27,0x2E,0x11,0x18,0x03,0x0A,
  0x56,0x5F,0x44,0x4D,0x72,0x7B,0x60,0x69,0x1E,0x17,0x0C,0x05,0x3A,0x33,0x28,0x21,
  0x4F,0x46,0x5D,0x54,0x6B,0x62,0x79,0x70,0x07,0x0E,0x15,0x1C,0x23,0x2A,0x31,0x38,
  0x41,0x48,0x53,0x5A,0x65,0x6C,0x77,0x7E,0x09,0x00,0x1B,0x12,0x2D,0x24,0x3F,0x36,
  0x58,0x51,0x4A,0x43,0x7C,0x75,0x6E,0x67,0x10,0x19,0x02,0x0B,0x34,0x3D,0x26,0x2F,
  0x73,0x7A,0x61,0x68,0x57,0x5E,0x45,0x4C,0x3B,0x32,0x29,0x20,0x1F,0x16,0x0D,0x04,
  0x6A,0x63,0x78,0x71,0x4E,0x47,0x5C,0x55,0x22,0x2B,0x30,0x39,0x06,0x0F,0x14,0x1D,
  0x25,0x2C,0x37,0x3E,0x01,0x08,0x13,0x1A,0x6D,0x64,0x7F,0x76,0x49,0x40,0x5B,0x52,
  0x3C,0x35,0x2E,0x27,0x18,0x11,0x0A,0x03,0x74,0x7D,0x66,0x6F,0x50,0x59,0x42,0x4B,
  0x17,0x1E,0x05,0x0C,0x33,0x3A,0x21,0x28,0x5F,0x56,0x4D,0x44,0x7B,0x72,0x69,0x60,
  0x0E,0x07,0x1C,0x15,0x2A,0x23,0x38,0x31,0x46,0x4F,0x54,0x5D,0x62,0x6B,0x70,0x79};
static uint8_t crc7(const uint8_t *d, int n) {
  uint8_t c = 0;
  for (int i = 0; i < n; i++) c = CRC7T[(c << 1) ^ d[i]];
  return (uint8_t)((c << 1) | 0x01);
}
static uint8_t r1(int max = 12) {
  uint8_t v = 0xFF;
  for (int i = 0; i < max; i++) { v = tftSPI.transfer(0xFF); if (!(v & 0x80)) break; }
  return v;
}

static void sdState(const char *tag) {
  tftSPI.beginTransaction(SPISettings(SD_HZ, MSBFIRST, SPI_MODE0));
  digitalWrite(SD_CS, HIGH);
  for (int i = 0; i < 8; i++) tftSPI.transfer(0xFF);       // limpa o MISO
  digitalWrite(SD_CS, LOW);
  // espera o cartao sair de busy (MISO 0x00), no maximo 300 ms
  uint32_t t0 = millis(); uint8_t w = 0x00;
  while (w == 0x00 && millis() - t0 < 300) w = tftSPI.transfer(0xFF);
  uint8_t c[6] = {0x4D, 0x00, 0x00, 0x00, 0x00, 0x00};      // CMD13
  c[5] = crc7(c, 5);
  tftSPI.transfer(c, 6);
  uint8_t r = r1();
  uint8_t b[3] = {0xFF, 0xFF, 0xFF};
  if (r != 0xFF) { b[0] = tftSPI.transfer(0xFF); b[1] = tftSPI.transfer(0xFF); b[2] = tftSPI.transfer(0xFF); }
  digitalWrite(SD_CS, HIGH);
  for (int i = 0; i < 8; i++) tftSPI.transfer(0xFF);
  tftSPI.endTransaction();
  if (r == 0xFF) { Serial.printf("%s: CMD13 sem resposta (0xFF)\r\n", tag); return; }
  Serial.printf("%s: r1=%02X status=%02X %02X (3o byte=%02X) -> %s\r\n",
                tag, r, b[0], b[1], b[2],
                (r & 0x04) ? "ILEGAL (cartao em idle)" : "aceitou o comando");
}

// grava + rele + apaga; devolve true se o conteudo voltou igual
static bool gravou(const char *nome) {
  static uint8_t wr[2048], rd[2048];
  for (int i = 0; i < 2048; i++) wr[i] = (uint8_t)i;
  errno = 0;
  File f = SD.open(nome, FILE_WRITE);
  if (!f) { Serial.printf("  %s: OPEN falhou errno=%d\r\n", nome, errno); return false; }
  size_t w = f.write(wr, sizeof(wr));
  int we = errno;
  uint32_t sz = (uint32_t)f.size();
  f.close();
  bool ok = false;
  File g = SD.open(nome);
  if (g) {
    size_t n = g.read(rd, sizeof(rd));
    g.close();
    ok = (n == sizeof(wr)) && !memcmp(rd, wr, sizeof(wr));
    Serial.printf("  %s: write=%u errno=%d size=%u relido=%u -> %s\r\n",
                  nome, (unsigned)w, we, (unsigned)sz, (unsigned)n, ok ? "OK" : "DIFERENTE");
  } else {
    Serial.printf("  %s: write=%u errno=%d size=%u RELER falhou errno=%d\r\n",
                  nome, (unsigned)w, we, (unsigned)sz, errno);
  }
  SD.remove(nome);
  return ok;
}

// le o setor 0 pelo driver (sem FATFS) e confere a assinatura 55AA
static bool setorZero(const char *tag) {
  static uint8_t s[512];
  memset(s, 0, sizeof(s));
  bool ok = SD.readRAW(s, 0);
  Serial.printf("%s: readRAW(0)=%d sig=%02X%02X %s\r\n", tag, (int)ok, s[510], s[511],
                (!ok ? "(I/O falhou)" : ((s[510] == 0x55 && s[511] == 0xAA) ? "(MBR ok)" : "(sem 55AA)")));
  return ok;
}

static void anima(uint32_t ms) {
#if ANIMA_LUZ == 0
  analogWriteResolution(TFT_BL, 8);
  analogWrite(TFT_BL, 0);
#endif
  uint32_t t0 = millis();
  int x = 0;
  Serial.printf("5: animando a tela por %u ms (luz %s, TFT %u Hz)...\r\n",
                (unsigned)ms, (ANIMA_LUZ ? "acesa" : "apagada"), (unsigned)TFT_HZ);
  while (millis() - t0 < ms) {
    tft.fillRect(0, 120, 128, 20, 0x0000);
    tft.fillRect(x, 120, 20, 20, 0x07E0);
    x = (x + 4) % 110;
    delay(10);
  }
  Serial.println("5: fim da animacao");
}

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println("\n===== TESTE DO SD (lib do core) =====");
  Serial.printf("config: SD=%u Hz TFT=%u Hz anima=%u ms luz=%d\r\n",
                (unsigned)SD_HZ, (unsigned)TFT_HZ, (unsigned)ANIMA_MS, (int)ANIMA_LUZ);

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
  pinMode(TOUCH_CS, OUTPUT); digitalWrite(TOUCH_CS, HIGH);
  pinMode(TOUCH_IRQ, INPUT_PULLUP);
  pinMode(IR_TX_PIN, OUTPUT); digitalWrite(IR_TX_PIN, HIGH);
  pinMode(TFT_RST, OUTPUT); digitalWrite(TFT_RST, HIGH);
  pinMode(SD_CS, OUTPUT); digitalWrite(SD_CS, HIGH);

  if (SPIFFS.begin(true)) Serial.println("2: SPIFFS ok"); else Serial.println("2: SPIFFS FALHOU");

  tftSPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI, -1);
  tft.setSPISpeed(TFT_HZ);
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(0);
  tft.fillScreen(0x0000);
  analogWriteResolution(TFT_BL, 8);
  analogWrite(TFT_BL, 80 * 255 / 100);
  tft.fillRect(10, 10, 60, 40, 0xF800);
  delay(200);

  // ---------------- A) mount + escrita imediata ----------------
  for (int i = 0; i < 40; i++) tftSPI.transfer(0xFF);   // 320 clocks com CS alto
  delay(200);
  if (!SD.begin(SD_CS, tftSPI, SD_HZ, "/sd", 8)) {
    Serial.println("4: MOUNT FALHOU -> cartao/energia. Desligue 30 s e rode de novo.");
    Serial.println("FIM");
    return;
  }
  Serial.printf("4: MOUNT ok type=%u card=%llu MB totalBytes=%llu MB\r\n",
                (unsigned)SD.cardType(), (unsigned long long)(SD.cardSize() / 1048576ULL),
                (unsigned long long)(SD.totalBytes() / 1048576ULL));
  bool A = gravou("/A.BIN");
  Serial.printf("A: %s\r\n", A ? "GRAVOU" : "FALHOU");
  setorZero("A/readRAW");
  sdState("A/estado");

  // ---------------- B) tela ----------------
  if (ANIMA_MS) anima((uint32_t)ANIMA_MS);
  else          { Serial.println("5: sem animacao (controle) - esperando 30 s"); delay(30000); }

  // ---------------- C) estado + escrita DEPOIS da tela ----------------
  sdState("C/estado");
  bool B = gravou("/B.BIN");
  Serial.printf("B: %s\r\n", B ? "GRAVOU" : "FALHOU");
  setorZero("B/readRAW");
  Serial.printf("B/totalBytes=%llu MB (0 = f_getfree falhou)\r\n",
                (unsigned long long)(SD.totalBytes() / 1048576ULL));

  // ---------------- D) re-init da lib (SD.end + SD.begin) ----------------
  if (!B) {
    Serial.println("D: tentando reviver com SD.end() + SD.begin()...");
    SD.end();
    delay(300);
    bool m = SD.begin(SD_CS, tftSPI, SD_HZ, "/sd", 8);
    Serial.printf("D: mount=%d card=%d\r\n", (int)m, (int)SD.cardType());
    sdState("D/estado");
    setorZero("D/readRAW");
    bool C = m ? gravou("/C.BIN") : false;
    Serial.printf("C: %s\r\n", C ? "GRAVOU" : "FALHOU");
    if (!C) Serial.println("C: o cartao nao voltou -> se persistir, so' o corte de energia recupera");
  } else {
    Serial.println("D: nao precisa (B gravou)");
  }

  Serial.println("\nFIM");
}

void loop() { delay(1000); }
