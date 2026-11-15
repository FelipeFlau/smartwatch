// ============================================================
//  SMARTWATCH - ESP32-S3 (Waveshare LCD-2 N16R8)
//  UI: LVGL 8.4 (ui.h) | hardware: ST7735 + XPT2046 (core.h)
//  apps: Config / Toolbox / Mat / Google / IA / Jogos / Saude / Arquivos
// ============================================================
#include "core.h"
#include "ui.h"

// ---------------- webserver de upload para o SD ----------------
#include <WebServer.h>
#include <SD.h>
#include <errno.h>
WebServer sdWeb(80);
static bool webRunning = false;
static File uploadF;
static volatile int gUpPct = -1;   // -1=parado 0..100 = progresso do upload (TFT)
static volatile int gUpFail = 0;   // 0=ok 1=erro ao gravar 2=sem espaco 3=sem permissao 4=nao montou

// ---- SD do modo servidor ------------------------------------------------
// O cartao e' montado uma vez e fica montado: no fluxo normal nada de SD.end().
// A UNICA excecao e' o recovery abaixo, onde SD.end() e SD.begin() andam sempre
// JUNTOS (o begin refaz init + CMD59 do CRC; CMD0 sozinho zera o CRC_ON_OFF e
// toda leitura passa a dar errno=5). Todo acesso passa por busLock.
static bool gSdWritable = true;            // teste real de escrita (protecao/cheio)
static bool gSdWrKnown = false;            // ja' testamos a escrita neste boot?
// cardType(): 0=NONE 1=MMC 2=SD 3=SDHC 4=UNKNOWN.
// UNKNOWN NAO e' "montado": e' o que fica quando o driver tentou reinicializar o
// cartao e falhou (era o `card=4` do log depois do Wi-Fi). Tratar UNKNOWN como
// montado fazia o firmware nunca remontar e seguir falhando com errno=19.
static bool webSdMounted() {
  sdcard_type_t t = SD.cardType();
  return t == CARD_MMC || t == CARD_SD || t == CARD_SDHC;
}
// Prova rapida de DATA I/O (nao so' de que o struct existe): le o setor 0 pelo
// driver, sem FatFs. E' isso que muda de verdade quando o cartao morre.
static bool sdAlive() {
  static uint8_t b[512];
  if (SD.cardType() == CARD_NONE) return false;   // sem VFS: readRAW usaria pdrv=0xFF
  bool got = busLock(2000);
  if (!got) return false;
  bool ok = SD.readRAW(b, 0) && b[510] == 0x55 && b[511] == 0xAA;
  busUnlock();
  return ok;
}
// tentativas com folga: cartoes clones as vezes precisam de 1-2 s e de CS em
// repouso (senao o mount falha com o cartao presente).
static bool webSdMount(int tries = 6, int gapMs = 150) {
  if (webSdMounted()) return true;
  sdBusPrepare();                     // touch fora do MISO
  pinMode(SD_CS_PIN, OUTPUT);
  digitalWrite(SD_CS_PIN, HIGH);
  // UNKNOWN: o driver tentou reinicializar e falhou, mas o VFS continua
  // registrado -> o SD.begin() sairia na hora (early return) e a montagem nunca
  // seria refeita. Aqui o SD.end() e' obrigatorio — e ele anda SEMPRE junto com
  // o SD.begin() logo abaixo, que refaz a sequencia da lib (inclusive o CMD59
  // que liga o CRC dos blocos). CMD0 sozinho (sem o begin) e' o que quebrava
  // a leitura com errno=5.
  if (SD.cardType() == CARD_UNKNOWN) {
    Serial.println("[sd] cartao em UNKNOWN: desmontando o VFS para remontar");
    SD.end();                          // CMD0 + unregister + free
    delay(300);
  }
  delay(120);                              // cartao sai do estado anterior (SPI mode)
  for (int a = 0; a < tries; a++) {
    // Antes de cada tentativa: pelo menos 100 clocks com CS alto. A
    // especificacao manda 74 para o cartao sair de um write abortado — as 160
    // do ff_sd_initialize cobrem isso, mas cartao travado por gravacao falha
    // so' volta com mais clocks e uma pausa maior entre as tentativas.
    if (busLock(50)) {
      tftSPI.beginTransaction(SPISettings(SD_SPI_HZ, MSBFIRST, SPI_MODE0));
      for (int i = 0; i < 16; i++) tftSPI.transfer(0xFF);   // 128 clocks
      tftSPI.endTransaction();
      busUnlock();
    }
    if (sdTryBegin(1)) { Serial.printf("[sd] mount ok (tentativa %d, %u Hz)\r\n", a + 1, (unsigned)SD_SPI_HZ); return true; }
    digitalWrite(SD_CS_PIN, HIGH);         // CS em repouso entre tentativas
    delay(gapMs);
  }
  Serial.printf("[sd] mount falhou apos %d tentativas\r\n", tries);
  Serial.println("[sd] CMD0 nao responde: corte a energia do cartao/board por 20 s");
  return false;
}
static bool webSdEnsure() {
  if (webSdMounted()) return true;
  gSdPause = true;
  busLock();
  bool ok = webSdMount(4, 200);
  busUnlock();
  gSdPause = false;
  if (!ok) Serial.println("[sd] servidor: falha ao montar o cartao");
  return ok;
}
// grava e apaga um arquivo de teste: separa "monta" de "grava".
// Loga o passo que falhou (open x write x remove) e o errno do VFS: sem isso
// "grava=0" nao diz se e' permissao, diretorio cheio, handle esgotado ou I/O.
static int gSdWrErr = 0;                   // 0=ok 1=open 2=write 3=remove
static bool webSdWriteTest(bool retry = true) {
  gSdWrErr = 0;
  sdBusPrepare();                     // touch fora do MISO antes de tocar o SD
  // NAO re-inicializar o cartao aqui (nada de CMD0/ACMD41): o CMD0 zera o
  // CRC_ON_OFF do cartao, mas a lib do core continua com supports_crc=true e
  // compara o CRC16 de CADA bloco lido com o que o cartao mandou (0xFFFF com
  // CRC desligado) -> toda leitura falha -> f_open devolve FR_DISK_ERR =
  // errno 5. O cartao ja' sai do mount em estado de transferencia; quem monta
  // e' so' o SD.begin().
  // 1) LEITURA antes de gravar: no sketch minimo (que funciona) a primeira
  //    operacao de dado e' uma leitura. A leitura tambem "acorda" o cartao.
  {
    File r = SD.open("/");
    if (r) r.close();
  }
  // Backlight OFF durante o teste: a luz de fundo consome corrente do 3V3 e,
  // junto com o resto (Wi-Fi, audio), pode fazer a tensao cair no pico da
  // gravacao do cartao. No sketch minimo a luz esta apagada e grava tudo.
  uint8_t blSave = cfg.brightness;
  blOff();
  delay(30);
  errno = 0;
  File f = SD.open("/WRTEST.TMP", FILE_WRITE);
  if (!f) {
    gSdWrErr = 1;
    blSet(blSave);                     // devolve a luz mesmo no caminho de falha
    Serial.printf("[sd] wr: open falhou errno=%d total=%llu used=%llu\r\n",
                  errno, (unsigned long long)SD.totalBytes(),
                  (unsigned long long)SD.usedBytes());
    return false;
  }
  // 2) escreve 2048 bytes (4 setores) como no sketch que funciona
  static uint8_t buf[2048];
  for (int i = 0; i < 2048; i++) buf[i] = (uint8_t)i;
  errno = 0;
  size_t w = f.write(buf, sizeof(buf));
  int we = errno;                       // errno do fwrite (5 = EIO na falha de setor)
  f.flush();                            // void no core 3.x: nao devolve status
  uint32_t sz = (uint32_t)f.size();
  f.close();
  // Unica prova real de que gravou no cartao: ler o arquivo de volta.
  bool rbok = false;
  File g = SD.open("/WRTEST.TMP");
  if (g) {
    static uint8_t rb[2048];
    size_t r = g.read(rb, sizeof(rb));
    rbok = (r == sizeof(buf)) && !memcmp(rb, buf, sizeof(buf));
    g.close();
  }
  bool rm = SD.remove("/WRTEST.TMP");
  if (!rm) gSdWrErr = (gSdWrErr == 2 ? 2 : 3);
  blSet(blSave);                      // devolve o brilho
  Serial.printf("[sd] wr: write=%u size=%u errno=%d readback=%d remove=%d\r\n",
                (unsigned)w, (unsigned)sz, we, (int)rbok, (int)rm);
  if (w != sizeof(buf) || !rbok) {
    gSdWrErr = 2;
    if (retry) {                       // 2a tentativa: so' repete a escrita
      Serial.println("[sd] wr: falhou, tentando de novo");
      gSdPause = true;
      bool ok2 = webSdWriteTest(false);
      gSdPause = false;
      return ok2;
    }
    return false;
  }
  return true;
}

// ---------------- CRC7 DO SD (usado pelo /raw) ----------------
// (A re-inicializacao manual do cartao — CMD0+ACMD41 por fora da lib — foi
// REMOVIDA: ver o comentario em webSdWriteTest(). CMD0 zera o CRC_ON_OFF e a
// lib passa a rejeitar os blocos lidos com FR_DISK_ERR/errno=5.)
// CRC7 da spec do SD (a MESMA tabela da lib do core): o byte de CRC e' o valor
// de 7 bits deslocado com o bit 1 no LSB. Conferido: CMD0 -> 0x95, CMD8 ->
// 0x87, ACMD41 -> 0x77, CMD13 -> 0x0D, CMD17 -> 0x55, CMD24 -> 0x6F.
static const uint8_t SD_CRC7T[256] = {
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
static uint8_t sdCrc7(const uint8_t *d, int n) {
  uint8_t crc = 0;
  for (int i = 0; i < n; i++) crc = SD_CRC7T[(crc << 1) ^ d[i]];
  return (uint8_t)((crc << 1) | 0x01);   // byte de CRC pronto (7 bits + 1)
}
// ---------------- TESTE DE SETOR CRU (sem FATFS) ----------------
// Separa "o cartao nao grava" de "o FatFs nao grava": le o setor 0, regrava
// exatamente o mesmo conteudo e le de novo. Se o raw funciona e o FatFs nao,
// o problema e' da camada de arquivos; se o raw tambem falha, e' do cartao/bus.
extern bool sdReadSector(uint8_t pdrv, char *buffer, unsigned long long sector);
extern bool sdWriteSector(uint8_t pdrv, const char *buffer, unsigned long long sector);
static bool webSdRawSector() {
  static uint8_t a[512], b[512];
  memset(a, 0, sizeof(a));
  gSdPause = true;
  busLock(3000);
  tftSPI.beginTransaction(SPISettings(SD_SPI_HZ, MSBFIRST, SPI_MODE0));
  bool r = sdReadSector(0, (char *)a, 0);
  // escolhe um setor FORA de qualquer particao (lê a tabela do MBR): nao arrisca
  // o cartao se a gravacao sair corrompida.
  uint64_t nsect = SD.cardSize() / 512ULL;
  uint64_t lastEnd = 0;
  for (int i = 0; i < 4; i++) {
    const uint8_t *e = a + 446 + i * 16;
    uint32_t lba = (uint32_t)e[8] | ((uint32_t)e[9] << 8) | ((uint32_t)e[10] << 16) | ((uint32_t)e[11] << 24);
    uint32_t cnt = (uint32_t)e[12] | ((uint32_t)e[13] << 8) | ((uint32_t)e[14] << 16) | ((uint32_t)e[15] << 24);
    if (cnt && (uint64_t)lba + cnt > lastEnd) lastEnd = (uint64_t)lba + cnt;
  }
  uint64_t sec = lastEnd + 1000;
  if (nsect > 4 && sec + 1 >= nsect) sec = nsect - 2;
  bool rd2 = false, w = false, rd3 = false;
  if (r && nsect > 4) {
    rd2 = sdReadSector(0, (char *)a, sec);          // le o setor escolhido
    if (rd2) w = sdWriteSector(0, (const char *)a, sec);   // regrava o MESMO
    if (w) rd3 = sdReadSector(0, (char *)b, sec);
  }
  tftSPI.endTransaction();
  busUnlock();
  gSdPause = false;
  bool igual = rd3 && memcmp(a, b, 512) == 0;
  Serial.printf("[sd] raw setor %llu @%u Hz: mbr=%d leu=%d gravou=%d releu=%d igual=%d\n",
                (unsigned long long)sec, (unsigned)SD_SPI_HZ,
                (int)r, (int)rd2, (int)w, (int)rd3, (int)igual);
  return (rd2 && w && rd3 && igual);
}
// Recuperacao de verdade: tira o VFS e monta de novo (SD.end + SD.begin).
// O par e' seguro desde que andem JUNTOS; o begin refaz init+CMD59 (CRC on).
// Verifica com readRAW: "montou" nao quer dizer "fala".
static bool webSdRecover() {
  gSdPause = true;
  busLock(3000);
  Serial.printf("[sd] recover: card=%d -> desmontando\r\n", (int)SD.cardType());
  if (SD.cardType() != CARD_NONE) { SD.end(); delay(400); }
  bool ok = false;
  for (int a = 0; a < 3 && !ok; a++) {
    ok = sdTryBegin(2);
    if (!ok) delay(400);
  }
  static uint8_t b[512];
  bool io = ok && SD.readRAW(b, 0) && b[510] == 0x55 && b[511] == 0xAA;
  busUnlock();
  gSdPause = false;
  Serial.printf("[sd] recover: mount=%d io=%d card=%d\r\n", (int)ok, (int)io, (int)SD.cardType());
  return ok && io;
}

// barra de progresso no TFT durante o upload (modo servidor, LVGL congelado).
// Desenha DENTRO do card da tela nova (mesma paleta do relogio).
static void webServBar() {
  const int x = SRV_BAR_X, y = SRV_BAR_Y, w = SRV_BAR_W, h = SRV_BAR_H;
  int p;
  if (gUpPct == -2) {                       // tamanho desconhecido: varre
    p = 30 + ((millis() / 200) % 4) * 15; if (p > 100) p = 100;
  } else p = (gUpPct < 0 || gUpPct > 100) ? 0 : gUpPct;
  tft.fillRoundRect(x, y, w, h, 4, 0x232A38);
  int fw = (int)((int64_t)(w - 4) * p / 100);
  if (fw > 0) tft.fillRoundRect(x + 2, y + 2, fw, h - 4, 3, gUpFail ? SRV_ERR : SRV_ACC);
  // texto de status abaixo da barra
  char b[24];
  if (gUpFail == 2)       snprintf(b, sizeof(b), "sem espaco");
  else if (gUpFail == 3)  snprintf(b, sizeof(b), "nao gravou");
  else if (gUpFail == 4)  snprintf(b, sizeof(b), "SD nao montou");
  else if (gUpFail)       snprintf(b, sizeof(b), "erro no SD");
  else if (gUpPct == -2)  snprintf(b, sizeof(b), "recebendo...");
  else if (gUpPct >= 0)   snprintf(b, sizeof(b), "enviando %d%%", p);
  else                    snprintf(b, sizeof(b), "aguardando");
  tft.setTextSize(1);
  tft.setTextColor(gUpFail ? SRV_ERR : SRV_DIM, SRV_CARD);
  tft.fillRect(x, SRV_ST_Y - 2, w, 11, SRV_CARD);
  tft.setCursor(x, SRV_ST_Y);
  tft.print(b);
}

static void webSdRoot() {
  sdWeb.send(200, "text/html",
    "<html><head><meta name='viewport' content='width=device-width, initial-scale=1'>"
    "<title>WearSend</title><style>"
    "*{box-sizing:border-box}"
    "body{margin:0;background:#000;color:#F4F5F7;padding:18px 14px 44px;"
    "font-family:-apple-system,'Segoe UI',Roboto,Helvetica,Arial,sans-serif}"
    ".wrap{max-width:560px;margin:0 auto}"
    "h1{margin:0;font-size:19px;font-weight:600;letter-spacing:.5px;display:flex;align-items:center;gap:9px}"
    "h1 i{width:11px;height:11px;border-radius:50%;background:#FF7A00;display:inline-block}"
    ".sub{color:#8B90A0;font-size:12px;margin:5px 0 14px}"
    ".card{background:#161A22;border-radius:16px;padding:14px;margin-bottom:13px}"
    ".chips{display:flex;gap:11px;margin-bottom:13px}"
    ".chip{width:64px;height:64px;border-radius:50%;background:#161A22;border:2px solid #262B36;color:#8B90A0;"
    "font-size:10px;font-weight:600;letter-spacing:.3px;display:flex;align-items:center;justify-content:center;"
    "cursor:pointer;text-align:center;line-height:1.25;transition:.15s}"
    ".chip.on{border-color:#FF7A00;color:#FF7A00;background:#1D222C}"
    ".row{display:flex;align-items:center;gap:10px;flex-wrap:wrap}"
    ".btn{background:#FF7A00;color:#fff;border:0;border-radius:22px;padding:11px 22px;font-size:14px;"
    "font-weight:600;letter-spacing:.4px;cursor:pointer}"
    ".btn:disabled{opacity:.45;cursor:default}"
    ".mini{background:#262B36;color:#8B90A0;border:0;border-radius:14px;font-size:11px;padding:6px 13px;cursor:pointer}"
    "input[type=file]{position:absolute;width:0;height:0;opacity:0;overflow:hidden}"
    ".fpick{background:#262B36;color:#F4F5F7;border-radius:20px;padding:10px 17px;font-size:13px;cursor:pointer}"
    ".fname{color:#F4F5F7;font-size:13px;word-break:break-all}"
    ".hint{color:#8B90A0;font-size:12px;margin-top:7px;line-height:1.45}"
    ".barwrap{height:8px;background:#232a38;border-radius:4px;overflow:hidden;margin-top:14px}"
    ".bar{height:100%;width:0;background:#FF7A00;border-radius:4px;transition:width .12s}"
    "#status{font-size:13px;margin-top:10px;min-height:18px;color:#3AD07F;line-height:1.4}"
    ".hd{display:flex;align-items:center;justify-content:space-between;margin-bottom:8px}"
    ".hd h2{margin:0;font-size:12px;font-weight:600;letter-spacing:.6px;color:#F4F5F7}"
    "ul{list-style:none;padding:0;margin:0;font-size:13px}"
    "#files li{padding:8px 0;border-bottom:1px solid #22262F;display:flex;justify-content:space-between;gap:10px}"
    "#files li:last-child{border-bottom:0}"
    "#files li span{color:#8B90A0;font-size:11px;white-space:nowrap}"
    "#sdw{font-size:12px;color:#8B90A0;margin-bottom:12px}"
    "</style></head><body><div class='wrap'>"
    "<h1><i></i>WearSend</h1>"
    "<div class='sub'>envie fotos, videos e musicas para o relogio</div>"
    "<div id='sdw'>lendo o SD...</div>"
    "<div class='chips'>"
    "<div class='chip on' id='t0' onclick='setCat(0)'>IMAGENS</div>"
    "<div class='chip' id='t1' onclick='setCat(1)'>VIDEOS</div>"
    "<div class='chip' id='t2' onclick='setCat(2)'>MUSICAS</div>"
    "</div>"
    "<div class='card'>"
    "<div class='row'><input type='file' id='f' onchange='info()'>"
    "<div class='fpick' onclick=\"document.getElementById('f').click()\">Escolher arquivo</div>"
    "<span class='fname' id='info'></span></div>"
    "<div class='hint' id='hint'></div>"
    "<div class='row' style='margin-top:13px'><button class='btn' id='send' onclick='sendF()'>ENVIAR</button></div>"
    "<div class='barwrap'><div class='bar' id='bar'></div></div>"
    "<div id='status'></div>"
    "</div>"
    "<div class='card'>"
    "<div class='hd'><h2>ARQUIVOS NO SD</h2><button class='mini' onclick='listF()'>ATUALIZAR</button></div>"
    "<ul id='files'></ul>"
    "</div>"
    "</div>"
    "<script>"
    "var cat=0;"
    "function setCat(n){cat=n;for(var i=0;i<3;i++)document.getElementById('t'+i).className='chip'+(i===n?' on':'');info();}"
    "function info(){var f=document.getElementById('f').files[0];var m=document.getElementById('info');"
    "var h=document.getElementById('hint');"
    "if(!f){m.textContent='';h.textContent='';return;}"
    "m.textContent=f.name+'  ('+(f.size>>10)+' kB)';"
    "h.textContent=(cat===0?'imagem -> reescalada para 128x160 JPEG (qualidade 0.72)'"
    ":cat===2?((f.name.toLowerCase().endsWith('.wav')?'WAV: enviado como esta':'musica -> convertida em WAV 8 kHz mono'))"
    ":'video -> fica guardado em /VIDEOS (o relogio nao reproduz video ainda)');}"
    "function base(n){var i=n.lastIndexOf('.');return i>0?n.slice(0,i):n;}"
    "function outName(n){if(cat===0)return base(n)+'.jpg';if(cat===2&&!n.toLowerCase().endsWith('.wav'))return base(n)+'.wav';return n;}"
    "function wav8(sm){var n=sm.length,b=new ArrayBuffer(44+n),v=new DataView(b);"
    "var ws=function(o,s){for(var i=0;i<s.length;i++)v.setUint8(o+i,s.charCodeAt(i));};"
    "ws(0,'RIFF');v.setUint32(4,36+n,true);ws(8,'WAVE');ws(12,'fmt ');v.setUint32(16,16,true);"
    "v.setUint16(20,1,true);v.setUint16(22,1,true);v.setUint32(24,8000,true);v.setUint32(28,8000,true);"
    "v.setUint16(32,1,true);v.setUint16(34,8,true);ws(36,'data');v.setUint32(40,n,true);"
    "for(var i=0;i<n;i++){var s=sm[i];if(s>1)s=1;if(s<-1)s=-1;v.setUint8(44+i,(s*127+128)|0);}"
    "return new Blob([b],{type:'audio/wav'});}"
    "async function toWav(file){var ab=await file.arrayBuffer();var c=new (window.AudioContext||window.webkitAudioContext)();"
    "var buf=await c.decodeAudioData(ab);var sr=buf.sampleRate;var L=buf.getChannelData(0),R=buf.numberOfChannels>1?buf.getChannelData(1):null;"
    "var sl=buf.length,dl=Math.min(Math.floor(sl*8000/sr),8000*150);var mo=new Float32Array(dl);"
    "for(var i=0;i<dl;i++){var p=i*sr/8000,i0=p|0,i1=Math.min(i0+1,sl-1),fr=p-i0;"
    "var s=L[i0]*(1-fr)+L[i1]*fr;if(R)s=(s+(R[i0]*(1-fr)+R[i1]*fr))*0.5;mo[i]=s;}return wav8(mo);}"
    "async function imgDown(f){var bmp=await createImageBitmap(f);var W=128,H=160,c=document.createElement('canvas');"
    "c.width=W;c.height=H;var g=c.getContext('2d');var s=Math.max(W/bmp.width,H/bmp.height);"
    "var dw=bmp.width*s,dh=bmp.height*s;g.drawImage(bmp,(W-dw)/2,(H-dh)/2,dw,dh);"
    "return await new Promise(function(r){c.toBlob(r,'image/jpeg',0.72);});}"
    "function convert(f){if(cat===2)return f.name.toLowerCase().endsWith('.wav')?Promise.resolve(f):toWav(f);"
    "if(cat===0)return imgDown(f);return Promise.resolve(f);}"
    "function bar(p){document.getElementById('bar').style.width=p+'%';}"
    "function st(t,c){var e=document.getElementById('status');e.textContent=t;e.style.color=(c||'#3AD07F');}"
    "function sendF(){var f=document.getElementById('f').files[0];if(!f){st('escolha um arquivo','#FF5C5C');return;}"
    "var b=document.getElementById('send');b.disabled=true;st('preparando...','#8B90A0');bar(0);"
    "convert(f).then(function(blob){var name=outName(f.name);"
    "var fd=new FormData();fd.append('file',blob,name);var xhr=new XMLHttpRequest();xhr.open('POST','/up');"
    "xhr.upload.onprogress=function(e){if(e.lengthComputable){var p=Math.round(e.loaded/e.total*100);bar(p);st('enviando '+p+'%','#FF7A00');}};"
    "xhr.onload=function(){bar(xhr.status==200?100:0);"
    "if(xhr.status==200)st('enviado: '+name+' ('+(blob.size>>10)+' kB)','#3AD07F');"
    "else if(xhr.status==507)st('sem espaco no SD','#FF5C5C');"
    "else if(xhr.status==500)st('o cartao nao gravou (veja o serial do relogio)','#FF5C5C');"
    "else st('erro '+xhr.status,'#FF5C5C');"
    "b.disabled=false;listF();sdSt();};"
    "xhr.onerror=function(){st('falha no envio','#FF5C5C');bar(0);b.disabled=false;};"
    "xhr.setRequestHeader('X-File-Size',blob.size);xhr.send(fd);"
    "}).catch(function(e){st('nao deu para converter o arquivo','#FF5C5C');b.disabled=false;});}"
    "function sdSt(){fetch('/st').then(function(r){return r.text();}).then(function(t){"
    "var a=t.split(' '),d=document.getElementById('sdw');"
    "var liv=a[1]||'0',tot=a[2]||'0';"
    "var espaco=(tot==='0')?'espaco desconhecido':(liv+' MB livres de '+tot+' MB');"
    "if(a[0]==='rw')d.textContent='SD: gravando ok  ('+espaco+')';"
    "else if(a[0]==='ro')d.textContent='SD: montado, mas nao gravou  ('+espaco+')';"
    "else d.textContent='SD: nao montou';"
    "d.style.color=(a[0]==='rw'?'#8B90A0':'#FF5C5C');});}"
    "function listF(){fetch('/list').then(function(r){return r.text();}).then(function(t){"
    "var e=document.getElementById('files');"
    "var ls=t.split('\\n').filter(function(x){return x.length;});"
    "e.innerHTML=ls.length?ls.map(function(x){var p=x.lastIndexOf(' ');"
    "return p<0?'<li>'+x+'</li>':'<li>'+x.slice(0,p)+'<span>'+x.slice(p+1)+'</span></li>';}).join('')"
    ":'<li style=\"color:#8B90A0\">nenhum arquivo</li>';});}"
    "listF();sdSt();"
    "</script></body></html>");
}
static void webSdList() {                    // GET /list: arquivos do SD
  String out;
  webSdEnsure();
  File r = SD.open("/");
  if (r) {
    File f = r.openNextFile();
    while (f) {
      out += String(f.name());
      out += f.isDirectory() ? " (pasta)\n" : "\n";
      f.close();
      f = r.openNextFile();
    }
    r.close();
  }
  sdWeb.send(200, "text/plain", out.length() ? out : "(vazio)");
}
static void webSdStat() {                    // GET /st: estado real do cartao
  bool mnt = webSdEnsure();
  uint64_t tb = mnt ? SD.totalBytes() : 0, ub = mnt ? SD.usedBytes() : 0;
  uint64_t fb = tb > ub ? tb - ub : 0;
  if (mnt && !gSdWrKnown) {                  // testa 1x por boot (o teste pode derrubar o cartao)
    gSdWritable = webSdWriteTest();
    gSdWrKnown = true;
    // Recupera quando o cartao nao fala: NONE (nada montado) ou UNKNOWN (o
    // driver tentou reinicializar e falhou). "Montado mas sem escrita" com o
    // cartao respondendo = FS danificado, e ai' nao adianta remontar.
    if (!gSdWritable && !webSdMounted()) webSdRecover();
  }
  String out = mnt ? (gSdWritable ? "rw" : "ro") : "no";
  out += " " + String((unsigned long)(fb / 1048576ULL));
  // total: 0 = f_getfree falhou (nao da' p/ dizer o espaco). A pagina mostra
  // "?" nesse caso em vez de "0 MB" (que parecia cartao cheio).
  out += " " + String((unsigned long)(tb / 1048576ULL));
  sdWeb.send(200, "text/plain", out);
}
// GET /dbg: diagnostico do SD (abrir /dbg no navegador do PC)
static void webSdDbg() {
  String o;
  bool mnt = webSdEnsure();
  o += "mount=" + String(mnt ? 1 : 0);
  o += " card=" + String((int)SD.cardType());
  o += " totalMB=" + String((unsigned long long)(mnt ? SD.totalBytes() / 1048576ULL : 0));
  o += " usedMB=" + String((unsigned long long)(mnt ? SD.usedBytes() / 1048576ULL : 0));
  File r;
  if (mnt) r = SD.open("/");
  o += " rootOpen=" + String(r ? 1 : 0);
  if (r) r.close();
  o += " mkdir=" + String(SD.mkdir("/FOTOS") ? 1 : (SD.exists("/FOTOS") ? 2 : 0));
  errno = 0;
  File f;
  if (mnt) f = SD.open("/WRTEST.TMP", FILE_WRITE);
  o += " openW=" + String(f ? 1 : 0) + " errno=" + String(errno);
  if (f) {
    errno = 0;
    size_t w = f.write((const uint8_t *)"ok", 2);
    f.flush();
    uint32_t sz = (uint32_t)f.size();
    f.close();
    o += " write=" + String((unsigned)w) + " werrno=" + String(errno);
    o += " size=" + String(sz);
    o += " remove=" + String(SD.remove("/WRTEST.TMP") ? 1 : 0);
  }
  o += " wrFail=" + String(gSdWrErr);
  o += " heap=" + String((unsigned long)ESP.getFreeHeap());
  o += " wrState=" + String(gSdWritable ? 1 : 0);
  if (gSdWrErr) {                            // a escrita derrubou o cartao: reinicializa
    Serial.println("[dbg] escrita falhou: reinicializando o cartao");
    bool rec = webSdRecover();
    o += " rec=" + String(rec ? 1 : 0);
    if (rec) {                               // repetiu depois do init a 400 kHz
      errno = 0;
      File f2 = SD.open("/WRTEST2.TMP", FILE_WRITE);
      o += " open2=" + String(f2 ? 1 : 0) + " errno2=" + String(errno);
      if (f2) {
        size_t w2 = f2.write((const uint8_t *)"ok", 2);
        f2.flush();
        f2.close();
        o += " write2=" + String((unsigned)w2);
        o += " remove2=" + String(SD.remove("/WRTEST2.TMP") ? 1 : 0);
      }
    }
  }
  o += " upPct=" + String(gUpPct);
  sdWeb.send(200, "text/plain", o);
}

// ============================================================
//  /raw — DIAGNOSTICO CRU DO CARTAO (por fora da lib SD/FATFS)
//  Responde: o cartao FALA? CMD0/CMD8/CMD58 + leitura do MBR.
//  Uso: abrir http://192.168.4.1/raw no PC (com o WearSend ligado).
//  Leitura:
//    r1cmd0=0x01 e cmd8 1AA  -> cartao responde: o problema e' a lib/montagem
//    r1cmd0=0xFF             -> cartao NAO responde: latch/energia/fiacao
// ============================================================
// (o sdCrc7 com a tabela da spec fica mais acima, junto do SD_CRC7T)
// le bytes de 0xFF ate' o primeiro com bit7=0 (R1) e devolve quantos clocks levou
static uint8_t sdR1(int &clocks, int max = 12) {
  uint8_t v = 0xFF; clocks = 0;
  for (int i = 0; i < max; i++) { v = tftSPI.transfer(0xFF); clocks = i + 1; if (!(v & 0x80)) break; }
  return v;
}
static void webSdRaw() {
  String o;
  // Se o cartao JA esta montado (explorador lendo), o teste cru NAO pode rodar:
  // ele manda CMD0 na mao, o que zera o CRC_ON_OFF do cartao enquanto a lib
  // segue comparando CRC16 -> todo bloco lido falha (errno=5). Destruiria o
  // que hoje funciona. O teste so' faz sentido com o cartao NAO montado.
  if (webSdMounted()) {
    o += "CARTAO MONTADO card=" + String((int)SD.cardType()) +
         " -> teste cru cancelado (CMD0 na mao zera o CRC do cartao).\r\n";
    o += "Se o modo servidor falha e o explorador funciona, o problema e' "
         "TIMING/ENERGIA na montagem, nao o cartao.\r\n";
    Serial.println("[raw] cartao montado: teste cru cancelado");
    sdWeb.send(200, "text/plain", o);
    return;
  }
  digitalWrite(SD_CS_PIN, OUTPUT);
  digitalWrite(SD_CS_PIN, HIGH);
  sdBusPrepare();                     // touch fora do MISO
  busLock(3000);
  tftSPI.beginTransaction(SPISettings(400000, MSBFIRST, SPI_MODE0));
  for (int i = 0; i < 20; i++) tftSPI.transfer(0xFF);       // >=74 clocks, CS alto

  // ---- CMD0 (GO_IDLE_STATE: CRC fixo 0x95) ----
  int cl = 0;
  digitalWrite(SD_CS_PIN, LOW);
  uint8_t c0[6] = {0x40, 0, 0, 0, 0, 0x95};
  tftSPI.transfer(&c0[0], 6);
  uint8_t r0 = sdR1(cl);
  digitalWrite(SD_CS_PIN, HIGH);
  tftSPI.transfer(0xFF);
  o += "cmd0=0x" + String(r0, HEX) + " (clocks=" + String(cl) + ")\r\n";

  // ---- CMD8 (SEND_IF_COND, CRC 0x87 p/ arg 0x000001AA) ----
  digitalWrite(SD_CS_PIN, LOW);
  uint8_t c8[6] = {0x48, 0x00, 0x00, 0x01, 0xAA, 0x87};
  tftSPI.transfer(&c8[0], 6);
  uint8_t r8 = sdR1(cl);
  uint8_t echo[4] = {0xFF, 0xFF, 0xFF, 0xFF};
  if (r8 < 0x80) { for (int i = 0; i < 4; i++) echo[i] = tftSPI.transfer(0xFF); }
  digitalWrite(SD_CS_PIN, HIGH);
  tftSPI.transfer(0xFF);
  o += "cmd8=0x" + String(r8, HEX) + " echo=" +
       String(echo[0], HEX) + String(echo[1], HEX) + String(echo[2], HEX) + String(echo[3], HEX) + "\r\n";

  // ---- CMD58 (READ_OCR, CRC calculado) ----
  digitalWrite(SD_CS_PIN, LOW);
  uint8_t p[6] = {0x7A, 0, 0, 0, 0, 0};
  p[5] = sdCrc7(p, 5);
  tftSPI.transfer(&p[0], 6);
  uint8_t r5 = sdR1(cl);
  uint8_t ocr[4] = {0xFF, 0xFF, 0xFF, 0xFF};
  if (r5 < 0x80) { for (int i = 0; i < 4; i++) ocr[i] = tftSPI.transfer(0xFF); }
  digitalWrite(SD_CS_PIN, HIGH);
  tftSPI.transfer(0xFF);
  o += "cmd58=0x" + String(r5, HEX) + " ocr=" +
       String(ocr[0], HEX) + String(ocr[1], HEX) + String(ocr[2], HEX) + String(ocr[3], HEX) + "\r\n";

  // ---- CMD17 do setor 0 (MBR): 0xFE + 512 bytes + CRC ----
  bool ok17 = false; uint8_t mbr[512] = {0};
  digitalWrite(SD_CS_PIN, LOW);
  uint8_t c17[6] = {0x51, 0, 0, 0, 0, 0};
  c17[5] = sdCrc7(c17, 5);
  tftSPI.transfer(&c17[0], 6);
  uint8_t r17 = sdR1(cl);
  if (r17 == 0x00) {
    uint32_t t0 = millis(); uint8_t tok = 0xFF;
    while (tok == 0xFF && millis() - t0 < 500) tok = tftSPI.transfer(0xFF);
    if (tok == 0xFE) {
      for (int i = 0; i < 512; i++) mbr[i] = tftSPI.transfer(0xFF);
      tftSPI.transfer(0xFF); tftSPI.transfer(0xFF);      // CRC
      ok17 = true;
    }
  }
  digitalWrite(SD_CS_PIN, HIGH);
  tftSPI.transfer(0xFF);
  o += "cmd17=0x" + String(r17, HEX) + (ok17 ? " bloco lido" : " SEM BLOCO") + "\r\n";
  if (ok17) {
    char h[80]; snprintf(h, sizeof(h), "mbr[0..7]=%02X %02X %02X %02X %02X %02X %02X %02X\r\n",
                         mbr[0], mbr[1], mbr[2], mbr[3], mbr[4], mbr[5], mbr[6], mbr[7]);
    o += h;
    o += "part_tipo=0x" + String(mbr[450], HEX);
    o += " sig=" + String(mbr[510] == 0x55 && mbr[511] == 0xAA ? "55AA ok" : "ruim") + "\r\n";
  }

  tftSPI.endTransaction();
  busUnlock();
  // devolve o cartao para a lib (a montagem crua pode ter saido do estado)
  gSdPause = true; busLock(3000);
  bool back = sdTryBegin(2);
  busUnlock(); gSdPause = false;
  o += "remontou=" + String(back ? 1 : 0) + " card=" + String((int)SD.cardType());
  Serial.printf("[raw] %s\r\n", o.c_str());
  sdWeb.send(200, "text/plain", o);
  if (!back) Serial.println("[raw] falhou: desligue e religue a energia do cartao 20 s");
}
static void webUpDone() {                       // resposta so' aqui (handler do POST)
  gUpPct = -1;
  if (gUpFail == 2)      sdWeb.send(507, "text/plain", "SEM ESPACO no SD");
  else if (gUpFail == 3) sdWeb.send(500, "text/plain", "cartao montado mas sem gravacao (protecao/cheio)");
  else if (gUpFail == 4) sdWeb.send(500, "text/plain", "cartao nao montou");
  else if (gUpFail)      sdWeb.send(500, "text/plain", "ERRO: falha ao gravar no SD (veja ERRO no relogio)");
  else                   sdWeb.send(200, "text/plain", "OK");
}
static uint32_t gUpGot = 0, gUpTotal = 0;
static String gUpDoneName = "";             // arquivo recebido -> preview (loop despacha)
static String gUpPath = "";                  // caminho completo no SD (pasta por tipo)

// nome no formato 8.3 aceitavel pela lib SD (sem LFN): A-Z 0-9 _ - . e base<=8/ext<=3
static String upBase83(const String &rawName) {
  String n = rawName;
  n.replace('\\', '/');
  int slash = n.lastIndexOf('/');
  if (slash >= 0) n = n.substring(slash + 1);
  n.trim();
  int dot = n.lastIndexOf('.');
  String base = (dot > 0) ? n.substring(0, dot) : n;
  String ext  = (dot > 0) ? n.substring(dot + 1) : "";
  base.toUpperCase(); ext.toUpperCase();
  String o;
  for (unsigned i = 0; i < base.length(); i++) {
    char c = base[i];
    if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-') o += c;
    if (o.length() >= 8) break;
  }
  if (!o.length()) o = "ARQUIVO";
  if (o.length() > 8) o = o.substring(0, 8);
  String e;
  for (unsigned i = 0; i < ext.length(); i++) {
    char c = ext[i];
    if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) e += c;
    if (e.length() >= 3) break;
  }
  return o + (e.length() ? ("." + e) : "");
}
// separa automaticamente: fotos / videos / musicas
static String upTypeDir(const String &name) {
  String lo = name; lo.toLowerCase();
  if (lo.endsWith(".jpg") || lo.endsWith(".jpeg") || lo.endsWith(".bmp") ||
      lo.endsWith(".png") || lo.endsWith(".gif")) return "/FOTOS";
  if (lo.endsWith(".mp4") || lo.endsWith(".mov") || lo.endsWith(".mkv") ||
      lo.endsWith(".avi") || lo.endsWith(".webm")) return "/VIDEOS";
  if (lo.endsWith(".wav") || lo.endsWith(".mp3") || lo.endsWith(".ogg") ||
      lo.endsWith(".flac") || lo.endsWith(".m4a")) return "/MUSICAS";
  return "/";
}
static String upDestPath(const String &rawName) {
  String dir = upTypeDir(rawName);
  String file = upBase83(rawName);
  int dot = file.lastIndexOf('.');
  String base = (dot > 0) ? file.substring(0, dot) : file;
  String ext  = (dot > 0) ? file.substring(dot + 1) : "";
  if (!ext.length()) ext = dir == "/FOTOS" ? "JPG" : dir == "/VIDEOS" ? "MP4" : dir == "/MUSICAS" ? "WAV" : "BIN";
  if (ext == "JPE") ext = "JPG";
  String cand = dir + (dir == "/" ? "" : "/") + base + "." + ext;
  int retry = 1;
  while (SD.exists(cand.c_str()) && retry < 95) {
    String b = base.length() > 6 ? base.substring(0, 6) : base;
    char sfx[3]; snprintf(sfx, sizeof(sfx), "%02d", retry);
    cand = dir + (dir == "/" ? "" : "/") + b + sfx + "." + ext;
    retry++;
  }
  return cand;
}
static void webUpFile() {
  HTTPUpload &up = sdWeb.upload();
  if (up.status == UPLOAD_FILE_START) {
    gUpGot = 0; gUpPct = 0; gUpTotal = 0; gUpFail = 0; gUpPath = "";
    uploadF = File();
    if (sdWeb.hasHeader("X-File-Size")) gUpTotal = (uint32_t)sdWeb.header("X-File-Size").toInt();
    if (!gUpTotal) gUpTotal = up.totalSize;
    bool mnt = webSdEnsure();                       // cartao montado ANTES de abrir
    sdBusPrepare();                                 // touch fora do MISO
    // Sem CMD0/ACMD41 aqui: o cartao ja' esta' em transferencia (o SD.begin()
    // montou) e re-iniciar zera o CRC_ON_OFF -> a lib rejeita todo bloco lido.
    String path = upDestPath(up.filename);
    String dir = path.substring(0, path.lastIndexOf('/'));
    if (!mnt) gUpFail = 4;                          // nao montou
    else if (!gSdWrKnown) {                         // testa 1x (o teste pode derrubar o cartao)
      gSdWritable = webSdWriteTest();
      gSdWrKnown = true;
      // igual ao /st: recuperar quando o cartao nao fala (NONE/UNKNOWN).
      if (!gSdWritable && !webSdMounted()) webSdRecover();
      if (!gSdWritable) {
        Serial.println("[up] cartao montado mas sem gravacao: veja o log [sd] raw no serial (fisico x FATFS)");
        gUpFail = 3;
      }
    }
    if (!gUpFail && gUpTotal) {                     // espaco livre antes de comecar
      uint64_t tb = SD.totalBytes(), ub = SD.usedBytes();
      uint64_t fb = tb > ub ? tb - ub : 0;
      // tb==0 => f_getfree falhou: NAO sabemos quanto ha de espaco. Antes isso
      // era tratado como "0 MB livres" e o upload era recusado com
      // "SEM ESPACO" num cartao de 16 GB com 15 GB livres. Agora so' recusa
      // quando o espaco e' de fato conhecida e insuficiente.
      if (tb > 0 && (uint64_t)gUpTotal + 65536ULL > fb) {
        gUpFail = 2;
        Serial.printf("[up] sem espaco: livre=%llu B preciso=%u B\r\n",
                      (unsigned long long)fb, (unsigned)gUpTotal);
      } else if (tb == 0) {
        Serial.println("[up] espaco desconhecido (f_getfree falhou): tentando gravar mesmo assim");
      }
    }
    bool made = true;
    if (!gUpFail) {
      if (dir.length() > 1) made = SD.mkdir(dir) || SD.exists(dir);   // pasta do tipo
      uploadF = SD.open(path, FILE_WRITE);             // NAO chama send aqui
      if (!uploadF) {                                 // 2a tentativa (remonta so' se preciso)
        delay(80);
        if (!webSdMounted()) { webSdEnsure(); }
        if (dir.length() > 1) SD.mkdir(dir);
        uploadF = SD.open(path, FILE_WRITE);
      }
      if (!uploadF) gUpFail = gSdWritable ? 1 : 3;      // open falhou: sem gravacao se o teste tambem falhou
    }
    gUpPath = uploadF ? path : String("");
    Serial.printf("[up] start '%s' -> %s mount=%d card=%d mkdir=%d open=%d total=%u fail=%d errno=%d\r\n",
                  up.filename.c_str(), path.c_str(), (int)mnt, (int)SD.cardType(),
                  (int)made, (int)(uploadF ? 1 : 0), (unsigned)gUpTotal, (int)gUpFail, errno);
    if (!uploadF && !gUpFail) { Serial.println("[up] FALHA ao criar arquivo"); gUpFail = 1; }
  } else if (up.status == UPLOAD_FILE_WRITE) {
    size_t done = 0;
    if (uploadF) {
      while (done < up.currentSize) {                // fwrite pode gravar menos: insiste
        size_t w = uploadF.write(up.buf + done, up.currentSize - done);
        if (!w) {
          uploadF.flush();                           // destrava o cache e tenta 1x mais
          w = uploadF.write(up.buf + done, up.currentSize - done);
          if (!w) break;
        }
        done += w;
      }
    }
    if (done != up.currentSize) {                    // gravou menos que recebeu = SD cheio/erro
      if (!gUpFail) { Serial.printf("[up] gravacao curta %u/%u\r\n",
                                    (unsigned)done, (unsigned)up.currentSize); gUpFail = 1; }
    }
    gUpGot += up.currentSize;
    gServT0 = millis();                           // upload ativo: renova timeout do servidor
    if (gUpTotal) gUpPct = (int)((uint64_t)gUpGot * 100 / gUpTotal);
    else gUpPct = -2;                       // tamanho desconhecido -> barra "marquee"
  } else if (up.status == UPLOAD_FILE_END) {
    uint32_t fsz = 0;
    if (uploadF) {
      uploadF.flush();
      fsz = uploadF.size();                      // confere o que realmente ficou no cartao
      uploadF.close(); uploadF = File();
    }
    gUpPct = 100;
    if (!gUpFail && gUpGot && fsz != gUpGot) {
      Serial.printf("[up] tamanho no cartao %u != recebido %u\r\n",
                    (unsigned)fsz, (unsigned)gUpGot);
      gUpFail = 1;
    }
    if (gUpFail) {
      if (gUpPath.length()) SD.remove(gUpPath);   // nao deixa arquivo pela metade no SD
      Serial.printf("[up] END '%s' COM FALHA\r\n", gUpPath.c_str());
    } else {
      gUpDoneName = gUpPath;
      Serial.printf("[up] END '%s' %u bytes gravados\r\n", gUpPath.c_str(), gUpGot);
    }
  } else if (up.status == UPLOAD_FILE_ABORTED) {  // navegador abortou: limpar tudo
    if (uploadF) { uploadF.close(); uploadF = File(); }
    if (gUpPath.length()) SD.remove(gUpPath);     // remove partial
    gUpPct = -1; gUpFail = 1;
    Serial.printf("[up] ABORTADO '%s' - partial removido\r\n", gUpPath.c_str());
  }
}
bool webSdStart() {
  // 1) RADIO OFF + folga de corrente ANTES de montar.
  //    O explorador de arquivos monta com o radio em repouso e funciona; o
  //    servidor subia o AP (ou herdava o STA conectado) e o mount falhava com
  //    o cartao presente (log "mount=0 card=0").
  //    Antes de derrubar o radio,GUARDA a rede em que ja' estava conectado:
  //    no fim do modo servidor voltamos para ela (ou para o AP, se nao houver).
  String prefer = (WiFi.status() == WL_CONNECTED) ? WiFi.SSID() : String("");
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(400);
  gSdPause = true;                    // sem render durante a montagem (bus compartilhado)
  busLock();
  bool sdOk = webSdMount(6, 200);
  // 2a rodada: o explorador monta as vezes quando o servidor falha — a
  // diferenca e' tempo (o cartao sai do power-on/a sessao SD anterior com mais
  // folga). Antes de dizer "nao montou", espera 2 s com CS alto e tenta mais.
  if (!sdOk) {
    Serial.println("[sd] sem cartao na 1a rodada: 2 s com CS alto e tenta de novo");
    digitalWrite(SD_CS_PIN, OUTPUT);
    digitalWrite(SD_CS_PIN, HIGH);
    delay(2000);
    sdOk = webSdMount(8, 250);
  }
  busUnlock();
  gSdPause = false;

  // (o teste de setor cru — webSdRawSector() — nao roda mais no boot: ele
  //  GRAVA no cartao e, com o touch no barramento, era o que matava o FS.
  //  Fica disponivel so' para diagnostico manual.)

  // 1) TESTE via FATFS (camada de arquivos)
  bool wr = false;
  if (sdOk) {
    gSdPause = true; busLock();
    gSdWritable = webSdWriteTest(); wr = gSdWritable; gSdWrKnown = true;
    busUnlock(); gSdPause = false;
  }
  if (sdOk && !wr) {
    // Montado mas sem escrita: se o cartao parou de falar (NONE/UNKNOWN) vale
    // desmontar e montar de novo; se ele responde e so' nao grava, e' FS.
    if (!webSdMounted()) {
      Serial.println("[sd] cartao travou na escrita: reinicializando (card morreu)");
      if (webSdRecover()) {
        gSdPause = true; busLock();
        gSdWritable = webSdWriteTest();
        busUnlock(); gSdPause = false;
        wr = gSdWritable;
      }
    } else {
      Serial.println("[sd] cartao montado mas nao grava (falha de I/O): veja o log do SD");
      Serial.println("[sd] nao forcar novas gravacoes: cada tentativa deixa o cartao travado");
    }
  }
  Serial.printf("[sd] servidor: mount=%d grava=%d card=%d\r\n",
                (int)sdOk, (int)wr, (int)SD.cardType());
  if (!sdOk) Serial.println("[sd] servidor: segue sem cartao; tenta de novo a cada acesso");
  // 2) REDE: usa o wifi salvo quando ele consegue conectar (aí o usuario abre
  //    o navegador no IP da rede e o relogio mostra o IP na tela). Sem wifi
  //    salvo/conectado, levanta o AP "WearSend".
  gSrvAp = false; gSrvIp = ""; gSrvNet = "";
  // rede preferida: a que ja' estava conectada, senao a primeira salva
  String want = "", wantPass = "";
  if (prefer.length()) {
    for (int i = 0; i < wifiN; i++) if (wifiSsid[i] == prefer) { want = prefer; wantPass = wifiPass[i]; break; }
  }
  if (!want.length() && cfg.autoWifi && wifiN && wifiSsid[0].length()) {
    want = wifiSsid[0]; wantPass = wifiPass[0];
  }
  if (want.length()) {
    Serial.printf("[srv] conectando no wifi '%s'...\r\n", want.c_str());
    // O log mostrou o cartao morrendo JUNTO com a associacao do Wi-Fi
    // (card=3 no boot -> card=4 = UNKNOWN depois, errno=19) e nao na escrita.
    // Mede-se DATA I/O antes e depois para separar energia/EMI de problema de FS:
    //   alive=1 -> alive=0  = o radio derrubou o cartao
    //   alive=1 -> alive=1  = o cartao esta vivo (o problema e' outro)
    // TX a 8.5 dBm corta o pico de corrente da associacao (padrao 19.5 dBm).
    Serial.printf("[srv] sd antes do wifi: alive=%d card=%d\r\n",
                  (int)sdAlive(), (int)SD.cardType());
    WiFi.mode(WIFI_STA);
    WiFi.begin(want.c_str(), wantPass.c_str());
    // ATENCAO: setTxPower() checa STA.started()/AP.started() e devolve false
    // se chamado antes do begin() — era o caso aqui, entao valia 19.5 dBm.
    // Depois do begin a associacao ja' comeca com o TX reduzido.
    bool txLow = WiFi.setTxPower(WIFI_POWER_8_5dBm);
    uint32_t t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < 8000) delay(120);
    if (WiFi.status() != WL_CONNECTED && txLow) {
      // 8.5 dBm pode nao alcancar o roteador: volta ao padrao e tenta de novo.
      // (Se o cartao cair com o TX alto, o remount abaixo cobre.)
      Serial.println("[srv] nao conectou a 8.5 dBm: tentando com o TX padrao");
      WiFi.setTxPower(WIFI_POWER_19_5dBm);
      t0 = millis();
      while (WiFi.status() != WL_CONNECTED && millis() - t0 < 8000) delay(120);
    }
    if (WiFi.status() == WL_CONNECTED) {
      gSrvIp = WiFi.localIP().toString();
      gSrvNet = WiFi.SSID();
      Serial.printf("[srv] wifi conectado: http://%s (%s)\r\n", gSrvIp.c_str(), gSrvNet.c_str());
    } else {
      Serial.println("[srv] wifi nao conectou: usando o AP");
      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);
      delay(250);
    }
  }
  if (!gSrvIp.length()) {                     // sem wifi: AP
    WiFi.mode(WIFI_AP);
    WiFi.softAP("WearSend", "12345678");
    WiFi.setTxPower(WIFI_POWER_8_5dBm);       // so' vale com o AP iniciado
    delay(150);
    gSrvAp = true;
    gSrvIp = WiFi.softAPIP().toString();
    gSrvNet = "WearSend";
    Serial.printf("[srv] AP ligado: http://%s (WearSend / 12345678)\r\n", gSrvIp.c_str());
  }
  bool aliveAfter = sdAlive();
  Serial.printf("[srv] sd depois do wifi: alive=%d card=%d\r\n",
                (int)aliveAfter, (int)SD.cardType());
  if (!aliveAfter) {
    // O radio derrubou o cartao: remonta AGORA, com o Wi-Fi ja' associado
    // (menos picos que durante a associacao) e re-testa a escrita.
    Serial.println("[srv] cartao caiu com o wifi: remontando com o radio estavel");
    bool back = webSdEnsure();           // webSdMount desmonta o VFS se estiver UNKNOWN
    gSdWrKnown = false;                  // forca novo teste de escrita no 1o acesso
    if (!back) gSdWritable = false;      // sem cartao: a pagina avisa em vez de tentar
    Serial.printf("[srv] remount pos-wifi: ok=%d card=%d\r\n", (int)back, (int)SD.cardType());
  }
  delay(120);
  sdWeb.on("/", HTTP_GET, webSdRoot);
  sdWeb.on("/list", HTTP_GET, webSdList);
  sdWeb.on("/st", HTTP_GET, webSdStat);
  sdWeb.on("/dbg", HTTP_GET, webSdDbg);
  sdWeb.on("/raw", HTTP_GET, webSdRaw);
  sdWeb.on("/up", HTTP_POST, webUpDone, webUpFile);
  static const char *upHdrs[] = { "X-File-Size" };  // sem collectHeaders o header custom
  sdWeb.collectHeaders(upHdrs, 1);                  // nunca chega (arduino-esp32 #3451)
  sdWeb.begin();
  webRunning = true;
  gUpPct = -1;
  return true;                        // mesmo sem cartao: a pagina avisa e remonta
}
// O upload mexe no SD (SCK/MOSI/MISO iguais aos do TFT). Seguravamos o
// barramento so' no teste de escrita do start; agora ele fica travado durante
// toda a requisicao (inclui o write dos blocos), com mutex RECURSIVO para os
// takes aninhados de sdBeginSession/sdTryBegin.
void webSdLoop() {
  if (!webRunning) return;
  bool got = busLock(3000);
  sdWeb.handleClient();
  webServBar();
  if (got) busUnlock();
}
// Fluxo normal: NAO desmonta (so' solta o CS). O SD.end() existe apenas dentro
// do webSdRecover(), sempre em par com o SD.begin() que refaz o init + CMD59.
void webSdStop() { sdWeb.stop(); webRunning = false; pinMode(SD_CS_PIN, OUTPUT); digitalWrite(SD_CS_PIN, HIGH); gUpPct = -1; }

// ---------------- lightsleep (2 toques p/ acordar) ----------------
static void doSleep() {
  blOff();
  uint8_t noData = 0;
  tft.sendCommand(0x10, &noData, 0); // SLPIN: painel apaga (tela preta real);
                                     // RTC interno segue contando o horario
  gpio_wakeup_enable((gpio_num_t)TOUCH_IRQ, GPIO_INTR_LOW_LEVEL);
  esp_sleep_enable_gpio_wakeup();
  esp_light_sleep_start();
  Serial.begin(115200);              // reanima USB-CDC depois do sleep
  tft.sendCommand(0x11, &noData, 0); // SLPOUT: painel desperta (antes de renderizar)
  delay(120);                        // ST7735 exige ~120ms apos SLPOUT
  gLastTouch = millis();
  // 1 toque so NAO acende; com wake2, acende apenas com 2 toques em ate 2s
  wakeArmDark(true);                 // true = se a janela passar, dorme de novo
}

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);
  delay(400);
  gLvTaskH = xTaskGetCurrentTaskHandle();   // LVGL so e seguro nesta task
  gBusMtx = xSemaphoreCreateRecursiveMutex();   // serializa o SPI (TFT + SD); recursivo: take aninhado e' seguro
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
  pinMode(TOUCH_CS, OUTPUT); digitalWrite(TOUCH_CS, HIGH);
  pinMode(TOUCH_IRQ, INPUT_PULLUP);
  pinMode(IR_TX_PIN, OUTPUT); digitalWrite(IR_TX_PIN, HIGH);   // LED IR: anodo no 3V3 -> HIGH = apagado
  pinMode(TFT_RST, OUTPUT); digitalWrite(TFT_RST, HIGH);
  pinMode(SD_CS_PIN, OUTPUT); digitalWrite(SD_CS_PIN, HIGH);   // CS do SD repousa em alto
  dispInit();
  // ---- MONTAGEM DO SD PRIMEIRO (esquema do teste 4): logo apos a tela e
  // ANTES do SPIFFS. Se o SPIFFS (que usa o flash) enrosca o barramento do
  // SD, e' aqui que aparece. O sketch minimo, que grava tudo, nao tem
  // SPIFFS nenhum.
  {
    uint8_t blSave = cfg.brightness;
    blOff();
    gSdPause = true;
    busLock();
    bool sdOk = sdTryBegin(2);
    bool wrOk = sdOk ? webSdWriteTest() : false;
    busUnlock();
    gSdPause = false;
    blSet(blSave);
    Serial.printf("[boot] sd: mount=%d grava=%d card=%d totalMB=%llu usedMB=%llu\r\n",
                  (int)sdOk, (int)wrOk, (int)SD.cardType(),
                  (unsigned long long)(SD.totalBytes() / 1048576ULL),
                  (unsigned long long)(SD.usedBytes() / 1048576ULL));
  }
  if (!SPIFFS.begin(true)) Serial.println("SPIFFS falhou!");   // formata se precisar
  cfgLoad();
  clockRestore();
  tft.setRotation((cfg.rotation & 1) ? 2 : 0);   // reaplica a rotacao do usuario
  blSet(cfg.brightness);
  wifiLoad();
  if (!calLoad()) calibRun();          // touch bugado -> recalibrar no 1o boot
  if (cfg.autoWifi && wifiN) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifiSsid[0].c_str(), wifiPass[0].c_str());  // non-block
  }
  gMpuOk = mpuInit();
  if (!gMpuOk) Serial.println("MPU ausente");
  randomSeed(micros());
  uiInit();                            // carrega scrLock + timers LVGL
  gLastTouch = millis();
}

// ---------------- LOOP ----------------
void loop() {
  // Wi-Fi: toda conexao STA (boot, app Wi-Fi, fim do modo servidor) pede NTP.
  static bool gWasWifiConnected = false;
  bool wifiConnected = WiFi.status() == WL_CONNECTED;
  if (wifiConnected && !gWasWifiConnected) uiNtpAutoRequest();
  // re-sincroniza a cada 6h enquanto houver Wi-Fi (hora restaurada pode estar atrasada)
  if (wifiConnected) {
    static uint32_t gLastNtpAuto = 0;
    if (!gLastNtpAuto || millis() - gLastNtpAuto > 21600000UL) {
      gLastNtpAuto = millis();
      uiNtpAutoRequest();
    }
  }
  gWasWifiConnected = wifiConnected;

  // ---- modo servidor de SD: LVGL congelado, TFT desenhado em cru ----
  if (serverActive()) {
    webSdLoop();
    if (gUpDoneName.length()) {              // recebeu arquivo: mostra preview e sai
      String _n = gUpDoneName; gUpDoneName = "";
      uiServerStop(); delay(200);
      serverPreview(_n);
      return;
    }
    static uint32_t lastPersist = 0;
    if (millis() - lastPersist > 120000) { lastPersist = millis(); clockPersist(); }
    if (serverCloseTap()) { uiServerStop(); delay(300); }
    delay(10);
    return;
  }

  // ---- UI normal ----
  touchTick();           // toque cru: unlock + swipe-back (no depende de eventos LVGL)
  // redesenho pedido por outra task (carga do WAV, sessao SD): aplica aqui
  if (gSdRefreshPending) {
    gSdRefreshPending = false;
    if (lv_obj_is_valid(lv_scr_act())) { lv_obj_invalidate(lv_scr_act()); lv_refr_now(gDisp); }
  }
  lv_timer_handler();
  mirrorTick();      // espelho do LCD por HTTP (LVGL continua rodando)

  static uint32_t lastPersist = 0;
  if (millis() - lastPersist > 120000) { lastPersist = millis(); clockPersist(); }

  uint32_t idle = millis() - gLastTouch;

  // sem interacao por 2 min dentro de um app: volta ao relogio
  static uint32_t lastIdleDecl = 0;
  if (!gOnLock && idle > 120000 && millis() - lastIdleDecl > 3000) {
    lastIdleDecl = millis();
    gWantLock = true;
  }

  // janela de duplo toque expirou (so 1 toque): volta ao escuro
  if (gWakeWait && millis() - gWakeWin > 2000) {
    if (gWakeLight) doSleep();                     // reentra lightsleep
    else { gWakeTaps = 0; gWakeWin = millis(); }   // wifi: re-aguarda nova dupla
  }
  // wifi ligado: quando bloqueado e ocioso, so apaga a luz (2 toques p/ voltar)
  if (WiFi.status() == WL_CONNECTED) {
    static bool blOut = false;
    if (gOnLock && !blOut && idle > cfg.sleepS * 1000UL) {
      blOff();
      blOut = true;
      wakeArmDark(false);               // acende so com 2 toques em <=2s
    } else if (blOut && !gWakeWait && (!gOnLock || idle < 500)) {
      blOn();
      blOut = false;
    }
  } else if (gOnLock && idle > cfg.sleepS * 1000UL) {
    doSleep();       // sem wifi: lightsleep economia total
  }

  delay(4);
}
