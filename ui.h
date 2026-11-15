// ============================================================
//  SMARTWATCH UI - LVGL 8 (interface moderna estilo smartwatch)
//  Event-driven; rede em task separada; hardware em core.h
// ============================================================
#pragma once
#include "core.h"
#include <math.h>          // sqrt, pow para o modo cientifico da calculadora
#include <lvgl.h>
#include <WebServer.h>
#include <SD.h>
#include <HTTPClient.h>         // GET robusto (redirect/chunked/UA/gzip)
#include <JPEGDEC.h>            // miniaturas da aba Imagens (sem Arduino_GFX)
#include "home_art.h"           // arte RGB565 da Home + icones (extraidos dos prints)
// lib BLE do core: incluida INCONDICIONALMENTE (o #include dentro de
// #if __has_include nao era detectado pelo scanner do arduino-cli, a lib
// nunca entrava no include path e HW_BLE ficava 0 -> BLE/BadBLE nao compilava)
#include <BLEDevice.h>
#define HW_BLE 1

// ---------------- identidade visual ----------------
#define UI_BG    lv_color_hex(0x0B0D12)
#define UI_CARD  lv_color_hex(0x1C202B)
static const uint32_t ACC_LIST[8] = { 0x2FA8FF, 0x3AD07F, 0xFF7A00, 0xFFCA28, 0xB88AFF, 0x7C4DFF, 0xFF6FA5, 0x8BC34A };  // 8 temas (indice 2 = laranja Redmi)
static inline lv_color_t accColor() { return lv_color_hex(ACC_LIST[cfg.accIdx % 8]); }
#define UI_ACC   accColor()

// Paleta Redmi Watch 5 (lv_color_t, igual aos defines UI_* originais)
#define REDMI_BG   lv_color_hex(0x000000)   // fundo preto puro (idêntico à foto)
#define REDMI_CARD lv_color_hex(0x161A22)   // cards
#define REDMI_ACC  lv_color_hex(0xFF7A00)   // laranja Redmi (destaque)
#define REDMI_TXT  lv_color_hex(0xF4F5F7)   // texto principal
#define REDMI_DIM  lv_color_hex(0x8B90A0)   // texto secundario
#define REDMI_OK   lv_color_hex(0x3AD07F)   // verde OK
#define REDMI_ERR  lv_color_hex(0xFF5C5C)   // vermelho erro

// ---------- icones pixel-art 16x16 dos apps/jogos (identidade Mi Band) ----------
// (substituem os glyphos que apareciam como "TEXT": desenhamos de mao)
#define ICO_SZ 16
static void icoPx(uint16_t *b, int x, int y, uint16_t col) { if (x>=0&&y>=0&&x<ICO_SZ&&y<ICO_SZ) b[y*ICO_SZ+x]=col; }
static void icoRect(uint16_t *b, int x, int y, int w, int h, uint16_t col){ for(int j=0;j<h;j++)for(int i=0;i<w;i++) icoPx(b,x+i,y+j,col); }
static void icoDisc(uint16_t *b, int cx, int cy, int r, uint16_t col){ for(int y=-r;y<=r;y++)for(int x=-r;x<=r;x++) if(x*x+y*y<=r*r) icoPx(b,cx+x,cy+y,col); }
static void icoRing(uint16_t *b, int cx,int cy,int r,uint16_t col){ for(int y=-r;y<=r;y++)for(int x=-r;x<=r;x++){int d=x*x+y*y; if(d<=r*r&&d>(r-2)*(r-2)) icoPx(b,cx+x,cy+y,col);} }
static void icoLine(uint16_t *b, int x0,int y0,int x1,int y1, uint16_t col){ int dx=abs(x1-x0),dy=abs(y1-y0);int sx=x0<x1?1:-1,sy=y0<y1?1:-1;int err=dx-dy;while(true){icoPx(b,x0,y0,col);if(x0==x1&&y0==y1)break;int e=2*err;if(e>-dy){err-=dy;x0+=sx;}if(e<dx){err+=dx;y0+=sy;}}}
static void drawIco(const char *n, uint16_t *b, uint16_t col) {
  for (int i=0;i<ICO_SZ*ICO_SZ;i++) b[i]=0;              // 0 = vao (depois vira cor do fundo)
  if (!strcmp(n,"Toolbox")) { icoRect(b,2,5,5,6,col); icoRect(b,6,4,4,2,col); icoPx(b,6,3,col); icoRect(b,11,2,2,3,col); }
  else if (!strcmp(n,"Mat")) { icoRect(b,2,2,7,10,col); icoRect(b,2,2,7,3,0); icoRect(b,3,6,5,1,0); for(int r=0;r<3;r++)for(int cc=0;cc<2;cc++) icoPx(b,3+cc*3,6+r*3,0); icoRect(b,11,3,3,9,col); }
  else if (!strcmp(n,"Google")) { icoRing(b,7,7,5,col); icoLine(b,10,10,14,14,col); }
  else if (!strcmp(n,"IA")) { icoRect(b,4,4,8,8,col); icoRect(b,6,6,4,4,0); icoLine(b,1,1,1,15,col); icoLine(b,14,1,14,15,col); icoLine(b,1,1,6,1,col); icoLine(b,1,14,6,14,col); }
  else if (!strcmp(n,"Jogos")) { icoRect(b,2,6,12,5,col); icoPx(b,4,4,col); icoPx(b,11,4,col); icoPx(b,4,13,col); icoPx(b,11,13,col); }
  else if (!strcmp(n,"Saude")) { icoDisc(b,5,5,3,col); icoDisc(b,10,5,3,col); for(int i=0;i<7;i++){icoPx(b,4+i,7+i,col);icoPx(b,4+i,8+i,col);} icoPx(b,10,7,col); }
  else if (!strcmp(n,"Arquivos")) { icoRect(b,3,2,10,12,col); icoRect(b,2,9,3,3,col); icoRect(b,11,9,3,3,col); icoRect(b,5,4,6,1,0); icoRect(b,5,7,6,1,0); }
  else if (!strcmp(n,"Config")) { icoDisc(b,8,8,3,col); icoDisc(b,8,8,1,0); icoRect(b,7,1,4,3,col); icoRect(b,7,13,4,3,col); icoRect(b,1,7,3,4,col); icoRect(b,13,7,3,4,col); }
  else if (!strcmp(n,"Power")) {  // simbolo de energia (anel aberto em cima + haste)
    icoRing(b,8,9,5,col);
    for (int x=5;x<=11;x++){ icoPx(b,x,3,0); }        // abre o anel no topo
    icoRect(b,7,2,2,6,col);                            // haste
  }
  else if (!strcmp(n,"Ajustes")) {  // sliders (3 trilhos com botoes)
    icoRect(b,2,3,12,1,col); icoRect(b,5,2,2,3,col);
    icoRect(b,2,8,12,1,col); icoRect(b,10,7,2,3,col);
    icoRect(b,2,13,12,1,col); icoRect(b,3,12,2,3,col);
  }
  else if (!strcmp(n,"Rocket")) {  // foguete
    icoRect(b,6,2,4,7,col); icoPx(b,5,3,col); icoPx(b,10,3,col);
    icoRect(b,5,9,2,3,col); icoRect(b,9,9,2,3,col);
    icoPx(b,7,12,col); icoPx(b,8,12,col); icoPx(b,7,14,col); icoPx(b,8,14,col);
    icoRect(b,7,4,2,2,0);
  }
  else if (!strcmp(n,"Lock")) {  // cadeado
    icoRing(b,8,5,3,col); icoRect(b,5,5,1,4,0); icoRect(b,10,5,1,4,0);
    icoRect(b,4,8,8,6,col); icoRect(b,7,10,2,2,0);
  }
  else if (!strcmp(n,"Gamepad")) {  // controle
    icoRect(b,2,5,12,6,col); icoRect(b,1,7,2,2,col); icoRect(b,13,7,2,2,col);
    icoRect(b,4,7,4,1,0); icoRect(b,5,6,1,3,0);       // d-pad
    icoPx(b,10,7,0); icoPx(b,12,9,0);                  // botoes
  }
  else if (!strcmp(n,"Relogio")) { icoRing(b,8,8,7,col); icoLine(b,8,8,8,4,col); icoLine(b,8,8,12,9,col); }
  else if (!strcmp(n,"PONG")) { icoLine(b,2,4,2,12,col); icoLine(b,13,4,13,12,col); icoPx(b,8,8,col); }
  else if (!strcmp(n,"INVASORES")) { icoRect(b,3,4,10,5,col); icoRect(b,3,4,2,2,0); icoRect(b,11,4,2,2,0); icoLine(b,3,9,3,12,col); icoLine(b,12,9,12,12,col); }
  else if (!strcmp(n,"FLAPPY")) { icoDisc(b,6,8,4,col); icoPx(b,10,7,0); icoPx(b,10,8,0); icoLine(b,4,11,1,14,col); }
  else if (!strcmp(n,"DINO")) { icoRect(b,4,4,7,7,col); icoRect(b,9,2,4,4,col); icoPx(b,11,3,0); icoLine(b,5,11,5,14,col); icoLine(b,10,11,10,14,col); }
  else if (!strcmp(n,"TETRIS")) { icoRect(b,3,5,4,4,col); icoRect(b,7,5,4,4,col); icoRect(b,11,5,2,4,col); icoRect(b,11,9,2,4,col); }
  else if (!strcmp(n,"DAMA")) { icoDisc(b,8,8,6,col); icoDisc(b,8,8,3,0); icoDisc(b,8,8,2,col); }
  else if (!strcmp(n,"XADREZ")) { icoDisc(b,8,5,3,col); icoRect(b,6,7,4,4,col); icoRect(b,4,12,8,2,col); }
  else if (!strcmp(n,"SNAKE")) { icoLine(b,5,3,11,3,col); icoLine(b,11,3,11,9,col); icoLine(b,11,9,5,9,col); icoLine(b,5,9,5,13,col); icoPx(b,11,10,col); }
  else if (!strcmp(n,"DADO")) { icoRect(b,3,3,10,10,col); icoRect(b,5,5,6,6,0); icoPx(b,6,6,col); icoPx(b,10,6,col); icoPx(b,6,10,col); icoPx(b,10,10,col); }
  else if (!strcmp(n,"MOEDA")) { icoDisc(b,8,8,7,col); icoDisc(b,8,8,5,0); icoLine(b,8,4,8,8,col); icoLine(b,6,6,10,6,col); }
  else if (!strcmp(n,"MARIO")) { icoRect(b,4,3,8,3,col); icoRect(b,5,2,6,2,col); icoRect(b,4,6,8,5,col); icoLine(b,4,11,4,14,col); icoLine(b,11,11,11,14,col); }
  else if (!strcmp(n,"SONIC")) { icoDisc(b,8,8,6,col); for(int k=0;k<3;k++) icoLine(b,8,8,3+k*4,2+k*4,col); icoDisc(b,8,8,3,0); }
  else if (!strcmp(n,"DOOM")) { icoLine(b,3,3,8,8,col); icoLine(b,13,3,8,8,col); icoDisc(b,8,8,4,col); icoDisc(b,8,8,1,0); }
  else if (!strcmp(n,"Espelho")) { icoRect(b,2,3,12,9,col); icoRect(b,4,5,8,3,0); icoLine(b,4,10,12,10,col); }
  else if (!strcmp(n,"Matematica")) {                 // calculadora com sinais + e ÷
    icoRect(b,4,1,8,14,col); icoRect(b,5,2,6,12,0);
    icoRect(b,5,4,6,3,col);                            // visor
    icoPx(b,6,9,col); icoPx(b,8,10,col); icoPx(b,10,9,col); icoPx(b,8,8,col);
    icoRect(b,6,12,4,2,col);                           // barra do "="
  } else if (!strcmp(n,"Google")) {                   // G colorido (vermelho/amarelo/verde/azul)
    uint16_t red=0xEA43, yel=0xFBBD, grn=0x34A8, blu=0x4285;
    for (int y=2;y<=13;y++) for (int x=2;x<=13;x++) {
      int dx=x-7, dy=y-7; int d=dx*dx+dy*dy;
      if (d<=36 && d>=(d>9?16:25)) {                  // anel
        if (x>=6 && y>=7 && y<=9) b[y*ICO_SZ+x]=blu;          // faixa azul
        else if (x<=4 && y<=4) b[y*ICO_SZ+x]=red;             // vermelho superior-esq
        else if (x>=11 && y<=4) b[y*ICO_SZ+x]=yel;            // amarelo sup-dir
        else if (x<=4 && y>=11) b[y*ICO_SZ+x]=yel;            // amarelo inf-esq
        else if (x>=11 && y>=11) b[y*ICO_SZ+x]=grn;           // verde inf-dir
        else b[y*ICO_SZ+x]=red;
      }
    }
  } else if (!strcmp(n,"Saude")) {                    // tenis (estilo da imagem)
    uint16_t sh=col;
    icoRect(b,2,7,10,3,sh); icoRect(b,1,9,13,2,sh);   // solado
    icoRect(b,3,4,7,3,sh); icoRect(b,2,5,3,3,sh);     // cabecalho
    icoPx(b,10,3,sh); icoPx(b,11,4,sh); icoPx(b,12,5,sh);
    icoRect(b,4,10,2,2,0); icoRect(b,8,10,2,2,0);     // recortes do solado
  } else if (!strcmp(n,"Toolbox")) {                  // pasta de trabalho (briefcase)
    icoRect(b,6,2,4,2,col);                            // alca
    icoRect(b,2,4,12,10,col);                          // corpo
    icoRect(b,3,5,10,3,0);                             // tampa
    icoRect(b,5,7,6,2,col);                            // fivela
    icoRect(b,3,11,10,2,0);                            // base clara
  } else { icoRect(b,3,3,10,10,col); }
}
static uint16_t* gIcoPool = nullptr;
#define ICO_POOL_MAX 80
static uint16_t *icoAlloc() {                 // pool simples em PSRAM
  if (!gIcoPool) gIcoPool = (uint16_t *)heap_caps_calloc(ICO_POOL_MAX, ICO_SZ * ICO_SZ * 2, MALLOC_CAP_SPIRAM);
  return gIcoPool;
}
static lv_obj_t *icoCanvas(lv_obj_t *parent, const char *name, int x, int y, uint16_t fg, uint16_t bg) {
  uint16_t *buf = (uint16_t *)heap_caps_calloc(ICO_SZ, ICO_SZ * 2, MALLOC_CAP_SPIRAM);
  if (!buf) return nullptr;
  drawIco(name, buf, fg);
  for (int i = 0; i < ICO_SZ * ICO_SZ; i++) if (buf[i] == 0) buf[i] = bg;   // RGB565 sem transparencia: vao vira cor do fundo
  lv_obj_t *cn = lv_canvas_create(parent);
  lv_canvas_set_buffer(cn, buf, ICO_SZ, ICO_SZ, LV_IMG_CF_TRUE_COLOR);
  lv_obj_set_pos(cn, x, y);
  lv_obj_clear_flag(cn, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(cn, LV_OBJ_FLAG_CLICKABLE);
  // libera a PSRAM quando o canvas morre (senao cada tela vazava 512 B)
  lv_obj_add_event_cb(cn, [](lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_DELETE) return;
    uint16_t *b = (uint16_t *)lv_event_get_user_data(e);
    if (b) heap_caps_free(b);
  }, LV_EVENT_DELETE, buf);
  return cn;
}

#define UI_OKC   lv_color_hex(0x3AD07F)
#define UI_ERR   lv_color_hex(0xFF5C5C)
#define UI_TXT   lv_color_hex(0xF4F5F7)
#define UI_DIM   lv_color_hex(0x8B90A0)
#define F_UI     &lv_font_montserrat_14
#define F_SM     &lv_font_montserrat_12
#define F_MED    &lv_font_montserrat_20
#define F_BIG    &lv_font_montserrat_28
#define F_XL     &lv_font_montserrat_32
#define F_HUGE   &lv_font_montserrat_48

// ---------------- estado ----------------
// (gLvPause / gSdPause estao no core.h: o sdProbe precisa deles antes do ui.h)
volatile uint32_t gLastTouch = 0;
// LVGL so pode ser tocado na task do loop(). Qualquer outra task (carga do
// WAV, rede, IR) pede o redesenho por aqui e o loop() aplica.
static TaskHandle_t gLvTaskH = nullptr;              // task do loop() (core 1)
static volatile bool gSdRefreshPending = false;      // redesenho pedido de outra task
static inline bool lvCtx() { return xTaskGetCurrentTaskHandle() == gLvTaskH; }
volatile bool     gTouchDown = false;   // dedo na tela agora (p/ gesto global)
volatile int      gTx = -1, gTy = -1;   // ultima posicao crua mapeada
volatile bool     gInGame = false;      // dentro de um jogo: swipe-back desliga
void touchTick();                      // tick de toque cru (chamado do loop())
bool gOnLock = true;
bool gMpuOk = false;
uint16_t *gFrame = nullptr;   // framebuffer LVGL (espelho do LCD via HTTP)
static lv_disp_t *gDisp;
static lv_disp_drv_t gDrv;
static uint32_t gLastNtpTry = 0;
static volatile bool gNtpAutoPending = false;   // conectou num Wi-Fi: sincronizar agora
static volatile bool gBtOn = false;   // pilha BLE ligada (scan/BadBLE em andamento)

// ============================================================
//  GERENCIADOR DE PSRAM
//  O ESP32-S3 Super Mini tem só 2 MB de PSRAM e o firmware reserva
//  buffers grandes (áudio 1,4 MB + JPEG 430 KB + thumbnails + jogos).
//  Aqui cada "recurso" (app/jogo) tem um registro:
//    - em uso  (tela aberta)  -> NUNCA é liberado (prioridade do que está aberto)
//    - ocioso  (> 1min30s)   -> liberado pelo limpador
//    - jogo    -> liberado na hora ao sair da tela
// ============================================================
#define PS_IDLE_MS   90000UL     // 1 min 30 s ocioso -> libera
#define PS_SWEEP_MS  15000UL     // varredura do limpador a cada 15 s
#define PS_MAP_MAX   12
enum PsRes { PSR_AUDIO = 0, PSR_GOOGLE, PSR_DOOM, PSR_CHESS, PSR_COUNT };
static uint32_t  psLastUse[PSR_COUNT]  = {0};
static bool      psInUse[PSR_COUNT]   = {false};
static bool      psAllocated[PSR_COUNT]= {false};
static lv_obj_t *psMapScr[PS_MAP_MAX]  = {nullptr};
static uint8_t   psMapRes[PS_MAP_MAX]  = {0};
static void psFreeRes(uint8_t r);              // (implementado junto dos buffers)
static void psCleanup(bool force = false);
static size_t psUsedBytes();                   // quanto de PSRAM esta em uso pelos apps
// jogo -> libera assim que a tela dele e destruida
static inline bool psIsGame(uint8_t r) { return r == PSR_DOOM || r == PSR_CHESS; }
static inline void psMark(uint8_t r, bool on) {
  if (r >= PSR_COUNT) return;
  psInUse[r] = on;
  psLastUse[r] = millis();
}
// jogo: libera logo apos a tela ser destruida (300 ms = destruction da animacao)
// O LVGL 8.4 nao tem lv_timer_get_user_data(), entao guardamos o prazo e
// processamos no pollTimer (roda a cada 150 ms).
static uint32_t psFreeAt[PSR_COUNT] = {0};    // 0 = sem pendencia
// alguma outra tela (viva) ainda usa este recurso?
static bool psResStillMapped(uint8_t r) {
  for (int i = 0; i < PS_MAP_MAX; i++)
    if (psMapScr[i] && psMapRes[i] == r && lv_obj_is_valid(psMapScr[i])) return true;
  return false;
}
static void psScrDeleteCb(lv_event_t *e) {
  lv_obj_t *s = (lv_obj_t *)lv_event_get_target(e);
  for (int i = 0; i < PS_MAP_MAX; i++) {
    if (psMapScr[i] != s) continue;
    uint8_t r = psMapRes[i];
    psMapScr[i] = nullptr;
    if (psResStillMapped(r)) break;   // outra tela viva usa o mesmo buffer
    psMark(r, false);                 // a partir de agora conta como ocioso
    if (psIsGame(r)) psFreeAt[r] = millis() + 300;
    else if (r == PSR_AUDIO) psFreeAt[r] = millis() + 2000;   // devolve 1,4 MB rapido
    break;
  }
}
static void psMapReg(lv_obj_t *scr, uint8_t res) {
  if (!scr || res >= PSR_COUNT) return;
  for (int i = 0; i < PS_MAP_MAX; i++)             // limpa registro morto/duplicado
    if (psMapScr[i] && (!lv_obj_is_valid(psMapScr[i]) || psMapScr[i] == scr)) psMapScr[i] = nullptr;
  for (int i = 0; i < PS_MAP_MAX; i++) {
    if (!psMapScr[i]) { psMapScr[i] = scr; psMapRes[i] = res; break; }
  }
  psMark(res, true);
  lv_obj_add_event_cb(scr, psScrDeleteCb, LV_EVENT_DELETE, nullptr);
}
// garante espaco na PSRAM: se faltar, descarta o que estiver ocioso e tenta de novo
static bool psNeed(size_t bytes) {
  bytes += 24576UL;                       // folga: evita "cabe por 1 kB" e falhar no meio
  if (heap_caps_get_free_size(MALLOC_CAP_SPIRAM) >= bytes) return true;
  psCleanup(true);
  return heap_caps_get_free_size(MALLOC_CAP_SPIRAM) >= bytes;
}

// ---------------- weather code (Open-Meteo / WMO) ----------------
static const char *wxDesc(int c) {
  switch (c) {
    case 0:  return "ceu limpo";
    case 1:  return "quase limpo";
    case 2:  return "parcial nublado";
    case 3:  return "nublado";
    case 45: case 48: return "nevoeiro";
    case 51: case 53: return "chuvisco";
    case 55: return "chuvisco forte";
    case 56: case 57: return "chuvisco gelado";
    case 61: case 80: return "pancadas";
    case 63: return "chuva";
    case 65: case 82: return "chuva forte";
    case 66: case 67: return "chuva gelada";
    case 71: case 85: return "neve";
    case 73: case 86: return "neve forte";
    case 75: return "neve c/ granizo";
    case 77: return "graos de neve";
    case 95: return "trovoada";
    case 96: case 99: return "trovoada c/ granizo";
    default: return "instavel";
  }
}

// ---------------- rede em task separada ----------------
enum NetKind { T_NONE = 0, T_DDG, T_AI, T_WIFI, T_NTP, T_SCAN, T_TESTW, T_BLE, T_PING,
               T_WX, T_GOO, T_IMGX, T_HTML };
volatile uint8_t gNetKind = T_NONE;
volatile bool gNetBusy = false, gNetDone = false, gNetRes = false;
String gNetA1, gNetA2, gNetOut;
String gGoSnipSave;    // snippet do 1o resultado (fallback do leitor)
extern bool webSdStart();
extern void webSdLoop();
extern void webSdStop();

static String urlEncode(String s) {
  String o; char b[6];
  for (unsigned i = 0; i < s.length(); i++) {
    uint8_t ch = (uint8_t)s[i];
    if (isalnum(ch) || ch == '-' || ch == '_' || ch == '.') o += (char)ch;
    else { snprintf(b, sizeof(b), "%%%02X", ch); o += b; }
  }
  return o;
}
static String urlDecode(String s) {
  s.replace('+', ' ');
  String o;
  for (unsigned i = 0; i < s.length(); i++) {
    if (s[i] == '%' && i + 2 < s.length()) {
      o += (char)strtol(s.substring(i + 1, i + 3).c_str(), nullptr, 16);
      i += 2;
    } else o += s[i];
  }
  return o;
}
static String entHtml(String s) {   // entidades comuns de HTML/PORTAL
  s.replace("&amp;", "&"); s.replace("&quot;", "\"");  s.replace("&#x27;", "'");
  s.replace("&#39;", "'"); s.replace("&apos;", "'");  s.replace("&lt;", "<");
  s.replace("&hellip;", "...");
  s.replace("&rsquo;", "'"); s.replace("&#161;", "<");  s.replace("&laquo;", "\"");
  s.replace("&raquo;", "\""); s.replace("&reg;", "(R)"); s.replace("&trade;", "(TM)");
  return s;
}
// extrai valor numerico de um JSON simples: "chave":valor (tolera ": " com espaco)
// pula ocorrencias cujo valor eh string (ex: bloco "units" antes do "current")
static float wxNum(const String &body, const char *key, float def = -999.0f) {
  String k = String("\"") + key + "\":";
  int p = body.indexOf(k);
  while (p >= 0) {
    int v = p + k.length();
    while (v < (int)body.length() && body[v] == ' ') v++;
    if (v < (int)body.length() && body[v] != '"') {   // valor nao-string -> numero
      int e = v;
      while (e < (int)body.length() &&
             (isdigit((unsigned char)body[e]) || body[e]=='.' ||
              body[e]=='-' || body[e]=='+')) e++;
      if (e > v) return body.substring(v, e).toFloat();
    }
    p = body.indexOf(k, p + k.length());              // era string: tenta a proxima
  }
  return def;
}
static void urlSplit(const String &u, String &host, String &path, bool &tls) {
  String h;
  if (u.startsWith("https://")) { tls = true; h = u.substring(8); }
  else if (u.startsWith("http://")) { tls = false; h = u.substring(7); }
  else { tls = true; h = u; }
  int sl = (int)h.indexOf('/');
  if (sl < 0) { host = h; path = "/"; }
  else { host = h.substring(0, sl); path = h.substring(sl); }
}
static void stripBlock(String &b, const char *tag) {   // remove <tag>...</tag>
  String a = "<" + String(tag), z = "</" + String(tag);
  int p;
  while ((p = (int)b.indexOf(a)) >= 0) {
    int e = (int)b.indexOf(z, p + a.length());
    b.remove(p, (e >= 0 ? e + z.length() : p + 400));
  }
}
static String stripTags(const String &s) {           // remove <...> (mantem texto)
  String o; bool inT = false;
  for (unsigned i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '<') { inT = true; continue; }
    if (c == '>') { inT = false; o += ' '; continue; }
    if (!inT) o += c;
  }
  return o;
}
// ---------- fetch estilo BABE32: TLS em PSRAM + cifras do Chrome ----------
// Referencia: alunmorris/babe32 (navegador de ESP32 que "abre o Hackaday").
// 3 tecnicas que faltavam aqui:
//  1. mbedTLS reserva ~40KB contiguos p/ SSL; a RAM interna fragmentada negava
//     esse bloco -> handshake falhava ("Falha de conexao"). Solucao: alocar
//     esses buffers na PSRAM (mbedtls_platform_set_calloc_free).
//  2. Cifras na ordem do Chrome (JA3): sites com bot-detection barravam o
//     fingerprint do client padrao do ESP32 (patch antes do handshake).
//  3. Cabecalhos completos de navegador + identity (sem gzip), em 1 write so.
#include <mbedtls/platform.h>
#include <mbedtls/ssl.h>

// leitor de paginas: html_parser (NellowTCS) - scanner de tags streaming,
// zero dependencia e sem DOM: perfeito p/ 128x160 (nao gasta RAM montando arvore)
extern "C" {
#include "html_parser.h"
}
static String gHtmlDomTxt;                  // acumulador do callback
static int    gHtmlDomMax;                  // teto de caracteres
static void htmlTagCb(struct html_tag *t, void *) {
  if (!t) return;
  const char *n = t->name;
  if (t->is_end_tag) {                       // fim de bloco -> quebra de linha
    if (gHtmlDomTxt.length() && gHtmlDomTxt[gHtmlDomTxt.length() - 1] != '\n')
      gHtmlDomTxt += "\n";
    return;
  }
  if (!gHtmlDomMax || (int)gHtmlDomTxt.length() >= gHtmlDomMax) return;
  bool bloco = !strcmp(n, "p")  || !strcmp(n, "h1") || !strcmp(n, "h2") ||
               !strcmp(n, "h3") || !strcmp(n, "li") || !strcmp(n, "td") ||
               !strcmp(n, "title") || !strcmp(n, "blockquote") ||
               !strcmp(n, "figcaption") || !strcmp(n, "summary");
  if (bloco) {
    if (t->contents_begin && t->contents_end && t->contents_end > t->contents_begin) {
      int len = (int)(t->contents_end - t->contents_begin);
      if (len > 300) len = 300;                        // 1 bloco = poucas linhas
      String inner(t->contents_begin, (unsigned)len);
      inner = entHtml(inner);
      String o; bool sp = false;                       // colapsa espacos
      for (unsigned i = 0; i < inner.length(); i++) {
        char c = inner[i];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') { sp = true; continue; }
        if (sp && o.length()) o += ' ';
        o += c; sp = false;
      }
      if (o.length() && (int)gHtmlDomTxt.length() < gHtmlDomMax) {
        gHtmlDomTxt += o;
        gHtmlDomTxt += "\n";
      }
    }
  } else if (!strcmp(n, "br") || !strcmp(n, "div")) {
    if (gHtmlDomTxt.length() && gHtmlDomTxt[gHtmlDomTxt.length() - 1] != '\n')
      gHtmlDomTxt += "\n";
  }
}
static String htmlDom(const String &body, int maxb) {  // HTML -> texto (via lib)
  gHtmlDomTxt = "";
  gHtmlDomMax = maxb;
  html_scan_tags(body.c_str(), body.length(), htmlTagCb, nullptr, 0, nullptr, nullptr);
  if ((int)gHtmlDomTxt.length() > maxb) gHtmlDomTxt = gHtmlDomTxt.substring(0, maxb);
  gHtmlDomTxt.trim();
  return gHtmlDomTxt;
}

static void *tlsPsramCb(size_t n, size_t size) {
  void *p = heap_caps_calloc(n, size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!p) p = heap_caps_calloc(n, size, MALLOC_CAP_8BIT);
  return p;
}
static void tlsPsramFreeCb(void *p) { heap_caps_free(p); }
static void tlsPsramOn() {
  static bool on = false;
  if (!on) { mbedtls_platform_set_calloc_free(tlsPsramCb, tlsPsramFreeCb); on = true; }
}
static const char *UA_G = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
                          "(KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36";
static const char *ALPN_G[] = { "http/1.1", NULL };
static String gHttpWhy;                              // motivo da ultima falha

// cliente TLS com as cifras na ordem do Chrome (anti JA3)
class TlCli : public WiFiClientSecure {
public:
  bool connectChrome(const char *host, uint16_t port) {
    static const int suites[] = {
      MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256,
      MBEDTLS_TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256,
      MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384,
      MBEDTLS_TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384,
#ifdef MBEDTLS_CHACHAPOLY_C
      MBEDTLS_TLS_ECDHE_ECDSA_WITH_CHACHA20_POLY1305_SHA256,
      MBEDTLS_TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256,
#endif
      MBEDTLS_TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA,
      MBEDTLS_TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA,
      MBEDTLS_TLS_RSA_WITH_AES_128_GCM_SHA256,
      MBEDTLS_TLS_RSA_WITH_AES_256_GCM_SHA384,
      MBEDTLS_TLS_RSA_WITH_AES_128_CBC_SHA,
      MBEDTLS_TLS_RSA_WITH_AES_256_CBC_SHA,
      0
    };
    setPlainStart();                       // segura o handshake
    if (!WiFiClientSecure::connect(host, port)) return false;
    mbedtls_ssl_conf_ciphersuites(&sslclient->ssl_conf, suites);
    return startTLS() == 1;
  }
};
static String buildReq(const String &path, const String &host) {
  return String("GET ") + path + " HTTP/1.1\r\n" +
         "Host: " + host + "\r\n" +
         "User-Agent: " + UA_G + "\r\n" +
         "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n" +
         "Accept-Language: pt-BR,pt;q=0.9,en;q=0.8\r\n" +
         "Accept-Encoding: identity\r\n" +
         "Upgrade-Insecure-Requests: 1\r\n" +
         "Connection: close\r\n\r\n";
}
static String deChunk(const String &b) {            // chunked -> corpo limpo
  String o;
  int p = 0;
  while (p < (int)b.length()) {
    int nl = b.indexOf("\r\n", p);
    if (nl < 0) break;
    unsigned long sz = strtoul(b.substring(p, nl).c_str(), nullptr, 16);
    if (sz == 0) break;
    int st = nl + 2;
    if (st + (int)sz > (int)b.length()) { o += b.substring(st); break; }
    o += b.substring(st, st + (int)sz);
    p = st + (int)sz + 2;
  }
  return o.length() ? o : b;
}
// le binario (JPEG) direto no buffer PSRAM: Content-Length, chunked ou ate fechar.
// redir sai preenchido quando a resposta pede redirect (301..308).
static bool webBinOnce(const String &url, uint8_t *buf, int cap, int &got,
                       String &redir, int ms) {
  tlsPsramOn();
  got = 0;
  redir = "";
  String host, path; bool tls;
  urlSplit(url, host, path, tls);
  if (!host.length()) { gHttpWhy = "url"; return false; }
  String req = buildReq(path, host);
  NetworkClient *c = nullptr;
  static TlCli sc;
  static WiFiClient pc;
  if (tls) {
    sc.setInsecure(); sc.setAlpnProtocols(ALPN_G);
    sc.setTimeout(ms > 1000 ? ms / 1000 : 10);
    if (!sc.connectChrome(host.c_str(), 443)) { gHttpWhy = "TLS"; return false; }
    c = &sc;
  } else {
    pc.setTimeout(ms > 1000 ? ms / 1000 : 10);
    if (!pc.connect(host.c_str(), 80)) { gHttpWhy = "TCP"; return false; }
    c = &pc;
  }
  c->write((const uint8_t *)req.c_str(), req.length());
  uint32_t t0 = millis();
  // ---- cabeçalhos (byte a byte ate \r\n\r\n) ----
  char h[512];
  int hn = 0;
  while (hn < (int)sizeof(h) - 1 && millis() - t0 < (uint32_t)ms) {
    if (c->available()) {
      char ch = (char)c->read();
      h[hn++] = ch;
      if (hn >= 4 && h[hn - 4] == '\r' && h[hn - 3] == '\n' &&
          h[hn - 2] == '\r' && h[hn - 1] == '\n') break;
    } else if (!c->connected() && !c->available()) break;
    else delay(1);
  }
  h[hn] = '\0';
  String hdrs(h);
  int sp = hdrs.indexOf(' ');
  int status = sp > 0 ? hdrs.substring(sp + 1, sp + 4).toInt() : 0;
  String low = hdrs; low.toLowerCase();
  long clen = -1;
  bool chunked = (low.indexOf("chunked") >= 0);
  int cp = low.indexOf("content-length:");
  if (cp >= 0) clen = atol(hdrs.substring(cp + 15).c_str());
  if (status >= 301 && status <= 308) {              // redirect
    int l = low.indexOf("location:");
    if (l >= 0) {
      int le = hdrs.indexOf("\r\n", l);
      redir = hdrs.substring(l + 9, le > l ? le : hdrs.length());
      redir.trim();
      if (redir.startsWith("/")) redir = (tls ? "https://" : "http://") + host + redir;
    }
    c->stop();
    gHttpWhy = "";
    return false;
  }
  if (status != 200) { gHttpWhy = "HTTP " + String(status); c->stop(); return false; }
  // ---- corpo ----
  t0 = millis();
  if (clen >= 0 && !chunked) {
    int want = clen < cap ? (int)clen : cap;
    while (got < want && millis() - t0 < (uint32_t)ms) {
      if (c->available()) buf[got++] = (uint8_t)c->read();
      else if (!c->connected() && !c->available()) break;
      else delay(1);
    }
  } else if (chunked) {
    while (got < cap && millis() - t0 < (uint32_t)ms) {
      char line[16]; int li = 0;
      while (li < 15 && millis() - t0 < (uint32_t)ms) {
        if (c->available()) {
          char ch = (char)c->read();
          if (ch == '\n') break;
          if (ch != '\r') line[li++] = ch;
        } else if (!c->connected() && !c->available()) goto fim;
        else delay(1);
      }
      line[li] = '\0';
      unsigned long sz = strtoul(line, nullptr, 16);
      if (sz == 0) break;
      int want = (int)sz;
      if (got + want > cap) want = cap - got;
      int g = 0;
      while (g < want && millis() - t0 < (uint32_t)ms) {
        if (c->available()) { buf[got++] = (uint8_t)c->read(); g++; }
        else if (!c->connected() && !c->available()) goto fim;
        else delay(1);
      }
      if (g < want) break;
      while (millis() - t0 < (uint32_t)ms) {       // \r\n do chunk
        if (c->available()) { c->read(); c->read(); break; }
        else delay(1);
      }
    }
  } else {
    while (got < cap && millis() - t0 < (uint32_t)ms) {
      if (c->available()) buf[got++] = (uint8_t)c->read();
      else if (!c->connected() && !c->available()) break;
      else delay(1);
    }
  }
fim:
  c->stop();
  bool ok = got >= 512 && buf[0] == 0xFF && buf[1] == 0xD8;   // SOI do JPEG
  gHttpWhy = ok ? "" : (got ? "jpeg" : "io");
  return ok;
}
static bool webRead(NetworkClient &cl, const String &req, String &raw,
                    int hardMax, int ms) {
  cl.write((const uint8_t *)req.c_str(), req.length());
  uint32_t t0 = millis();
  while ((cl.connected() || cl.available()) && millis() - t0 < (uint32_t)ms) {
    if (cl.available()) {
      raw += (char)cl.read();
      if ((int)raw.length() > hardMax) break;
    } else delay(1);
  }
  return raw.length() > 0;
}
static bool webOnce(const String &url, String &nextOut, int maxb, int ms) {
  // false=ok (nextOut=corpo) ; true=falha (gHttpWhy= motivo; nextOut=location)
  tlsPsramOn();
  String host, path; bool tls;
  urlSplit(url, host, path, tls);
  if (!host.length()) { gHttpWhy = "url"; return true; }
  String req = buildReq(path, host);
  String raw;
  if (tls) {
    TlCli cl; cl.setInsecure(); cl.setAlpnProtocols(ALPN_G);
    cl.setTimeout(ms > 1000 ? ms / 1000 : 10);
    if (!cl.connectChrome(host.c_str(), 443)) { gHttpWhy = "TLS"; return true; }
    webRead(cl, req, raw, maxb + 3000, ms);
    cl.stop();
  } else {
    WiFiClient cl; cl.setTimeout(ms > 1000 ? ms / 1000 : 10);
    if (!cl.connect(host.c_str(), 80)) { gHttpWhy = "TCP"; return true; }
    webRead(cl, req, raw, maxb + 3000, ms);
    cl.stop();
  }
  int he = raw.indexOf("\r\n\r\n");
  if (he < 0) { gHttpWhy = "hdr"; return true; }
  String hdrs = raw.substring(0, he);
  int sp = hdrs.indexOf(' ');
  int status = sp > 0 ? hdrs.substring(sp + 1, sp + 4).toInt() : 0;
  String b = raw.substring(he + 4);
  String low = hdrs; low.toLowerCase();
  if (low.indexOf("chunked") >= 0) b = deChunk(b);
  if (status >= 301 && status <= 308) {              // segue o redirect
    int l = low.indexOf("location:");
    if (l >= 0) {
      int le = hdrs.indexOf("\r\n", l);
      String loc = hdrs.substring(l + 9, le > l ? le : hdrs.length());
      loc.trim();
      if (loc.startsWith("/")) loc = (tls ? "https://" : "http://") + host + loc;
      if (loc.startsWith("http")) { gHttpWhy = ""; nextOut = loc; return true; }
    }
    gHttpWhy = "HTTP " + String(status);
    return true;
  }
  if (status != 200) { gHttpWhy = "HTTP " + String(status); return true; }
  if ((int)b.length() > maxb) b = b.substring(0, maxb);
  nextOut = b;
  return false;
}
static bool httpGet(const String &url, String &out, int maxb = 14000, int ms = 12000) {
  for (int t = 0; t < 2; t++) {                       // TLS eh volatil: 2 tentativas
    String nxt; bool r = webOnce(url, nxt, maxb, ms);
    if (!r) { out = nxt; gHttpWhy = ""; return true; }
    if (!gHttpWhy.length()) {                         // veio redirect: repete
      String b2; bool r2 = webOnce(nxt, b2, maxb, ms);
      if (!r2) { out = b2; gHttpWhy = ""; return true; }
    }
    delay(250);
  }
  return false;
}
static bool httpGetBin(const String &url, uint8_t *buf, int cap, int &got,
                       int ms = 14000) {
  for (int t = 0; t < 2; t++) {
    String redir;
    if (webBinOnce(url, buf, cap, got, redir, ms)) return true;
    if (redir.length() && webBinOnce(redir, buf, cap, got, redir, ms)) return true;
    delay(250);
  }
  return false;
}
static String collapseWs(const String &in) {          // espacos colapsados
  String o; bool sp = false;
  for (unsigned i = 0; i < in.length(); i++) {
    char c = in[i];
    if (c == ' ' || c == '\t' || c == '\n' || c == '\r') { sp = true; continue; }
    if (sp && o.length()) o += ' ';
    o += c; sp = false;
  }
  return o;
}
static String mdClean(const String &md, int maxb) {  // markdown (jina) -> texto
  String b = md;
  int mc = (int)b.indexOf("Markdown Content:");
  if (mc >= 0) b = b.substring(mc + 16);
  int i;
  while ((i = (int)b.indexOf("![")) >= 0) {          // imagens: remove
    int e = (int)b.indexOf(')', i);
    b.remove(i, e > i ? e - i + 1 : 3);
  }
  while ((i = (int)b.indexOf('[')) >= 0) {           // [texto](url) -> texto
    int m = (int)b.indexOf(']', i);
    if (m >= 0 && m - i < 80) {
      int p = (int)b.indexOf('(', m);
      int e = (int)b.indexOf(')', p);
      if (p == m + 1 && e > p) {
        b.remove(m + 1, e - m);
        b.remove(i, 1);
        m = (int)b.indexOf(']', i);
        if (m >= 0) b.remove(m, 1);
      } else b.remove(i, 1);
    } else b.remove(i, 1);
  }
  b.replace("#", " "); b.replace("*", " "); b.replace("`", " ");
  b.replace("|", " "); b.replace("<", " "); b.replace(">", " ");
  String o = collapseWs(b);
  if ((int)o.length() > maxb) o = o.substring(0, maxb);
  return o;
}
static String htmlText(const String &raw, int maxb) {   // HTML -> texto legivel
  String b = raw;
  stripBlock(b, "script"); stripBlock(b, "style"); stripBlock(b, "svg"); stripBlock(b, "head");
  int hh = (int)b.indexOf("\r\n\r\n");                  // corta cabecalho HTTP
  if (hh >= 0) b = b.substring(hh + 4);
  String out;
  bool inTag = false;
  for (unsigned i = 0; i < b.length() && out.length() < maxb; i++) {
    char c = b[i];
    if (c == '<') { inTag = true; continue; }
    if (c == '>') { inTag = false; out += ' '; continue; }
    if (!inTag) out += c;
  }
  out = entHtml(out);
  out.replace("\r", " "); out.replace("\n", " "); out.replace("\t", " ");
  String o2; bool sp = false;
  for (unsigned i = 0; i < out.length(); i++) {
    char c = out[i];
    if (c == ' ') { sp = true; continue; }
    if (sp && o2.length()) o2 += ' ';
    o2 += c; sp = false;
  }
  return o2;
}
// ---------- miniaturas (aba Imagens): JPEG -> buffer 58x30 RGB565 ----------
static const int IMG_W = 58, IMG_H = 30;
static uint16_t *goPix[4] = { nullptr, nullptr, nullptr, nullptr };  // 4 miniaturas (PSRAM)
static uint8_t *gJpgBuf = nullptr;        // JPEG bruto (PSRAM)
static int gOutW = IMG_W, gOutH = IMG_H;  // dimensoes do destino atual do decode
static uint16_t *gImgOut = nullptr;       // destino atual do decode
static int gImgCropX = 0, gImgCropY = 0;
static int jpgDrawCb(JPEGDRAW *pDraw) {
  if (!gImgOut) return 0;
  int w = pDraw->iWidth, h = pDraw->iHeight;
  int dx = pDraw->x - gImgCropX, dy = pDraw->y - gImgCropY;
  for (int r = 0; r < h; r++) {
    int ty = dy + r; if (ty < 0 || ty >= gOutH) continue;
    int src = r * w;
    for (int c = 0; c < w; c++) {
      int tx = dx + c; if (tx < 0 || tx >= gOutW) continue;
      gImgOut[ty * gOutW + tx] = pDraw->pPixels[src + c];
    }
  }
  return 1;
}
static void netTask(void *) {
  switch (gNetKind) {
    case T_WIFI: gNetRes = wifiSavedConnect(); break;
    case T_TESTW: {
      WiFi.mode(WIFI_STA);
      WiFi.setSleep(false);               // sem power save p/ o handshake TLS
      gNetRes = false;
      for (int att = 0; att < 3 && !gNetRes; att++) {   // 3 x 10s = ate 30s
        WiFi.disconnect(false);
        delay(120);
        WiFi.begin(gNetA1.c_str(), gNetA2.c_str());
        uint32_t t0 = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - t0 < 10000) delay(100);
        gNetRes = WiFi.status() == WL_CONNECTED;
      }
      break;
    }
    case T_NTP: {
      gNetRes = clockNtpSync();
      gNetOut = gNetRes ? "hora sincronizada" : "NTP nao respondeu";
      break;
    }
    case T_BLE: {
#if HW_BLE
      gBtOn = true;                      // icone BT acende azul na barra
      BLEDevice::init("");
      BLEScan *scan = BLEDevice::getScan();
      scan->setActiveScan(true);
      BLEScanResults *r = scan->start(4, false);   // 4s, bloqueante (task separada)
      int n = r ? r->getCount() : 0; int lim = n < 12 ? n : 12;
      gNetOut = String(n) + " dispositivos\n";
      for (int i = 0; i < lim; i++) {
        BLEAdvertisedDevice d = r->getDevice(i);
        String nm = d.getName();
        if (!nm.length()) nm = d.getAddress().toString().c_str();
        char b[72];
        snprintf(b, sizeof(b), "%s (%d)\n", nm.c_str(), d.getRSSI());
        gNetOut += b;
      }
      if (n > lim) gNetOut += "...\n";
      scan->clearResults();
      BLEDevice::deinit(false);
      gBtOn = false;
#else
      gNetOut = "BLE indisponivel";
#endif
      break;
    }
    case T_PING: {
      WiFiClient cl;
      uint32_t t0 = millis();
      if (cl.connect(IPAddress(1, 1, 1, 1), 80, 5000)) {
        gNetOut = "1.1.1.1: " + String((int)(millis() - t0)) + " ms";
        gNetRes = true;
        cl.stop();
      } else gNetOut = "1.1.1.1 sem resposta";
      break;
    }
    case T_WX: {
      // Open-Meteo: sem API key; HTTP simples (porta 80)
      WiFiClient cl;
      if (!cl.connect("api.open-meteo.com", 80, 6000)) { gNetOut = "Falha de conexao"; break; }
      cl.print("GET /v1/forecast?latitude=" + String(cfg.wLat, 4) +
               "&longitude=" + String(cfg.wLon, 4) +
               "&current=temperature_2m,weather_code,wind_speed_10m,"
               "relative_humidity_2m,apparent_temperature HTTP/1.1\r\n"
               "Host: api.open-meteo.com\r\nConnection: close\r\n\r\n");
      String body; uint32_t t0 = millis();
      while (cl.connected() && millis() - t0 < 8000) {
        while (cl.available()) { body += (char)cl.read(); if (body.length() > 3000) break; }
        delay(2);
      }
      cl.stop();
      // extracao robusta (tolera ": " com ou sem espaco; offsets fixos quebravam)
      float temp = wxNum(body, "temperature_2m", -999);
      int   code = (int)wxNum(body, "weather_code", -1);
      if (temp <= -998) { gNetOut = "sem dados"; break; }
      float wind = wxNum(body, "wind_speed_10m", -1);
      float hum  = wxNum(body, "relative_humidity_2m", -1);
      float appr = wxNum(body, "apparent_temperature", -999);
      char b[56];
      // temp|desc|vento|umidade|sensacao  (extras -1 = indisponivel)
      snprintf(b, sizeof(b), "%.0f|%s|%.0f|%.0f|%.0f",
               temp, wxDesc(code), wind, hum, appr);
      gNetOut = b;
      break;
    }
    case T_GOO: {
      // DuckDuckGo "lite": HTML enxuto = titulo + resuminho + link real em uddg=
      String body;
      if (!httpGet("https://lite.duckduckgo.com/lite/?q=" + urlEncode(gNetA1), body, 16000, 14000)) {
        gNetOut = "Falha de conexao (" + gHttpWhy + ")"; break;
      }
      String out, snip;
      int p = 0, nr = 0;
      while (nr < 5) {
        int a = body.indexOf("<a rel=\"nofollow\"", p);
        if (a < 0) break;
        int gt = body.indexOf('>', a), e2 = body.indexOf("</a>", gt + 1);
        if (gt < 0 || e2 < 0) break;
        String href = body.substring(a, gt);
        String t = stripTags(body.substring(gt + 1, e2));       // titulo limpo
        p = e2 + 4;
        t = entHtml(t); t.trim();
        if (t.length() < 3) continue;
        // "resuminho" do site: <td class='result-snippet'>...</td>
        String s2;
        int sn = body.indexOf("result-snippet", e2);
        if (sn >= 0 && sn < e2 + 500) {
          int g2 = body.indexOf('>', sn);
          if (g2 > 0 && g2 < sn + 60) {
            int e3 = body.indexOf("</td>", g2 + 1);
            s2 = stripTags(body.substring(g2 + 1, e3 > 0 ? e3 : g2 + 300));
            s2 = entHtml(s2); s2.trim();
          }
        }
        if (!nr && s2.length()) snip = s2;
        // link real: parametro uddg (sem anuncio/redirect)
        String url;
        int u1 = href.indexOf("uddg=");
        if (u1 >= 0) {
          int u2 = href.indexOf('&', u1 + 5);
          url = urlDecode(href.substring(u1 + 5, u2 > 0 ? u2 : href.length()));
        } else {
          int h1 = href.indexOf("href=\"") + 6;
          int h2 = href.indexOf('"', h1);
          url = href.substring(h1, h2);
          if (url.startsWith("//")) url = "https:" + url;
          else if (!url.startsWith("http")) {           // links relativos: ignora
            t = "";                                     // (marca p/ pular linha)
          }
        }
        if (!t.length()) continue;
        if (url.indexOf("duckduckgo.com/y.js") >= 0) continue;   // anuncio patrocinado
        if (nr) out += "\n";
        out += t + String("\x02") + url;
        nr++;
      }
      gNetRes = nr > 0;
      gNetOut = nr ? (snip + String("\x03") + out) : "Nada encontrado. Reformule a busca.";
      break;
    }
    case T_IMGX: {
      // API da Wikipedia (estavel, sem key) -> thumbnails JPEG pequenos
      static const char *wkHosts[] = { "pt.wikipedia.org", "en.wikipedia.org" };
      String lst;
      bool gotList = false;
      for (int h = 0; h < 2 && !gotList; h++) {
        gotList = httpGet(String("https://") + wkHosts[h] + "/w/api.php?action=query&format=json"
                          "&generator=search&gsrsearch=" + urlEncode(gNetA1) +
                          "&gsrnamespace=6&gsrlimit=10&prop=imageinfo&iiprop=url&iiurlwidth=500",
                          lst, 12000, 14000);
      }
      if (!gotList) { gNetOut = "Falha de conexao (" + gHttpWhy + ")"; break; }
      String urls[4]; int nu = 0, q = 0;
      while (nu < 4) {
        int t = lst.indexOf("\"thumburl\"", q); if (t < 0) break;
        int c1 = lst.indexOf(':', t);        if (c1 < 0) break;
        int q1 = lst.indexOf('"', c1);       if (q1 < 0) break;
        int q2 = lst.indexOf('"', q1 + 1);   if (q2 < 0) break;
        String u = lst.substring(q1 + 1, q2);
        q = q2 + 1;
        u.replace("\\/", "/"); u.replace("\\u0026", "&");
        if (u.indexOf(".jpg") < 0) continue;                 // JPEGDEC so faz JPEG
        urls[nu++] = u;
      }
      if (!nu) { gNetRes = false; gNetOut = "Sem imagens para esta busca"; break; }
      if (!gJpgBuf) {
        if (!psNeed(150000)) { gNetRes = false; gNetOut = "Memoria insuficiente"; break; }
        gJpgBuf = (uint8_t *)heap_caps_malloc(150000, MALLOC_CAP_SPIRAM);
        if (!gJpgBuf) { gNetRes = false; gNetOut = "Memoria insuficiente"; break; }
        psAllocated[PSR_GOOGLE] = true;
      }
      int ok = 0;
      for (int i = 0; i < nu; i++) {
        int got = 0;
        bool okb = httpGetBin(urls[i], gJpgBuf, 140000, got, 16000);
        if (!okb) okb = httpGetBin(urls[i], gJpgBuf, 140000, got, 16000);   // 1 retry
        if (!okb) continue;
        static JPEGDEC jdec;
        gImgOut = goPix[ok];
        if (!gImgOut) { ok++; continue; }
        gOutW = IMG_W; gOutH = IMG_H;         // destino das miniaturas (evita estado velho)
        if (jdec.openRAM(gJpgBuf, got, jpgDrawCb) != 0) {
          gImgOut = nullptr;
          jdec.close();
        } else {
          int cw = (jdec.getWidth()  + 7) >> 3, ch = (jdec.getHeight() + 7) >> 3;
          gImgCropX = cw > IMG_W ? (cw - IMG_W) / 2 : 0;
          gImgCropY = ch > IMG_H ? (ch - IMG_H) / 2 : 0;
          jdec.decode(0, 0, 8);          // 8 = JPEG_SCALE_EIGHTH
          jdec.close();
          ok++;
        }
      }
      gNetRes = ok > 0;
      gNetOut = String("IMG") + String(ok) + "/" + String(nu);   // ex: IMG2/4
      break;
    }
    case T_HTML: {
      // leitor: 1) fetch DIRETO (headers de navegador - funciona na maioria)
      //         2) r.jina.ai (texto limpo)   3) proxy codetabs   4) resuminho da busca
      String url = gNetA1;
      String body;
      bool ok = httpGet(url, body, 14000, 12000);
      if (!ok) ok = httpGet("https://r.jina.ai/" + url, body, 9000, 20000);
      if (!ok) ok = httpGet("https://api.codetabs.com/v1/proxy?quest=" + url, body, 12000, 20000);
      String txt;
      if (ok) txt = (body.indexOf("Markdown Content:") >= 0 || body.indexOf("URL Source:") >= 0)
                     ? mdClean(body, 4800)
                     : (body.indexOf('<') >= 0 ? htmlDom(body, 4800)     // lib de tags
                                               : collapseWs(body).substring(0, 4800));
      if (!txt.length()) txt = gGoSnipSave;      // ultimo recurso: resuminho da busca
      gNetRes = txt.length() > 60;
      gNetOut = txt.length() ? txt : ("Pagina ilegivel (" + gHttpWhy + ")");
      break;
    }
    case T_AI: {
      String esc;
      for (unsigned i = 0; i < gNetA1.length(); i++) {
        char c = gNetA1[i];
        if (c == '"' || c == '\\') { esc += '\\'; esc += c; }
        else if ((uint8_t)c >= 32) esc += c;
      }
      String body = "{\"model\":\"" + cfg.iaModel +
                    "\",\"messages\":[{\"role\":\"user\",\"content\":\"" + esc + "\"}]}";
      tlsPsramOn();
      TlCli cl; cl.setInsecure(); cl.setAlpnProtocols(ALPN_G);   // mesmo TLS das buscas
      if (!cl.connectChrome("api.openai.com", 443)) { gNetOut = "Falha de conexao"; break; }
      cl.println("POST /v1/chat/completions HTTP/1.1");
      cl.println("Host: api.openai.com");
      cl.println("Authorization: Bearer " + cfg.iaKey);
      cl.println("Content-Type: application/json");
      cl.println("Content-Length: " + String(body.length()));
      cl.println("Connection: close");
      cl.println();
      cl.print(body);
      String resp; uint32_t t0 = millis();
      while (cl.connected() && millis() - t0 < 25000) {
        while (cl.available()) resp += (char)cl.read();
        if (resp.length() > 9000) break;
        delay(2);
      }
      cl.stop();
      int ci = resp.indexOf("\"content\":\"");
      String ans = (ci >= 0) ? resp.substring(ci + 11) : "Erro:\n" + resp.substring(0, 140);
      int e2 = ans.indexOf('"'); if (e2 > 0) ans = ans.substring(0, e2);
      ans.replace("\\n", "\n");
      gNetOut = ans;
      break;
    }
    case T_SCAN: {
      int n = WiFi.scanNetworks();
      String out;
      int shown = 0;
      for (int i = 0; i < n && shown < 8; i++) {
        String sn = WiFi.SSID(i);
        if (!sn.length()) continue;
        out += sn + " (" + String(WiFi.RSSI(i)) + ")\n";
        shown++;
      }
      gNetOut = out;
      gNetRes = shown > 0;
      break;
    }
  }
  gNetBusy = false; gNetDone = true;
  vTaskDelete(NULL);
}
static bool netRun(uint8_t kind, String a1 = "", String a2 = "") {
  if (gNetBusy) return false;
  gNetKind = kind; gNetA1 = a1; gNetA2 = a2; gNetOut = "";
  gNetDone = false; gNetBusy = true;
  xTaskCreate(netTask, "net", 16384, NULL, 1, NULL);   // 16K: handshake TLS + HTTPClient
  return true;
}
// chamado pelo loop quando o Wi-Fi passa de off -> conectado.
// O envio em si ocorre no pollTimer (depois do callback da rede pendente).
void uiNtpAutoRequest() { gNtpAutoPending = true; }

// =================== IR TX (ligacao do usuario) ===================
// ANODO do LED -> resistor -> 3V3 | CATODO -> GPIO IR_TX (sink).
// LED acende com o pino em LOW: logica INVERTIDA (IR_ACTIVE_LOW 1).
// Se um dia usar transistor de driver, mude para 0.
#define IR_ACTIVE_LOW 1
static inline void irIdleLevel() {   // nivel de repouso = LED apagado
#if IR_ACTIVE_LOW
  digitalWrite(IR_TX_PIN, HIGH);
#else
  digitalWrite(IR_TX_PIN, LOW);
#endif
}
#define IR_LEDC_DUTY_MOD   170     // ~33% de 8 bits
#define IR_LEDC_DUTY_FULL  255     // codigo "non-pulsed": LED acesso continua
static bool gIrCarrierOk = false;
static uint32_t gIrCarrierHz = 0;

static void irDelayUs(uint32_t us) {   // delayMicroseconds aceita no maximo ~65 ms
  while (us >= 60000) { delayMicroseconds(60000); us -= 60000; }
  if (us) delayMicroseconds(us);
}
static void irCarrierSetup() {
  gIrCarrierOk = ledcAttach(IR_TX_PIN, 38000, 8);
  if (gIrCarrierOk) {
    gIrCarrierHz = 38000;
    ledcOutputInvert(IR_TX_PIN, IR_ACTIVE_LOW ? true : false);
  }
}
static void irCarrierHz(uint32_t freq) {
  if (!gIrCarrierOk) return;
  if (freq < 10000) freq = 38000;
  if (freq == gIrCarrierHz) return;
  if (ledcChangeFrequency(IR_TX_PIN, freq, 8) == 0) { gIrCarrierOk = false; return; }
  gIrCarrierHz = freq;
}
static inline void irLedWrite(bool on, bool modulated) {
#if IR_ACTIVE_LOW
  const uint8_t lvOn = LOW, lvOff = HIGH;
#else
  const uint8_t lvOn = HIGH, lvOff = LOW;
#endif
  if (gIrCarrierOk) {
    if (!on) ledcWrite(IR_TX_PIN, 0);
    else ledcWrite(IR_TX_PIN, modulated ? IR_LEDC_DUTY_MOD : IR_LEDC_DUTY_FULL);
    return;
  }
  digitalWrite(IR_TX_PIN, on ? lvOn : lvOff);
}
static void irMark(uint32_t us, bool modulated = true) {
  if (gIrCarrierOk) { irLedWrite(true, modulated); irDelayUs(us); irLedWrite(false, true); return; }
  // fallback bit-bang 38kHz (~11us ligado / ~15us desligado)
  irLedWrite(true, false);
  uint32_t endT = micros() + us;
  while ((int32_t)(micros() - endT) < 0) {
    irLedWrite(true, true); delayMicroseconds(11);
    irLedWrite(false, true); delayMicroseconds(15);
    irLedWrite(true, true);
  }
  irLedWrite(false, false);
}
static void carrier38(uint32_t us) {   // MARK 38kHz (NEC/raw)
  irCarrierHz(38000);
  irMark(us, true);
}
// ---------------- IR RX: decod NEC por interrupcao (IR_RX_PIN) ----------------
static volatile uint32_t gIrCode = 0;      // ultimo quadro recebido (32 bits)
static volatile bool     gIrNew  = false;  // quadro novo disponivel
static volatile bool     irLearnArm = false;
static volatile uint32_t irIsrT = 0, irBits = 0;
static volatile int      irBit = 0, irArm = 0;
#define IR_RAW_MAX 120
static volatile uint16_t irRawN = 0;
static volatile uint32_t irRaw[IR_RAW_MAX];
static volatile bool     irRawNew = false;
static void IRAM_ATTR irIsr() {            // falling = inicio de cada MARK
  uint32_t now = micros();
  uint32_t cyc = now - irIsrT;
  irIsrT = now;
  if (irRawN < IR_RAW_MAX) irRaw[irRawN++] = cyc;      // grava o tempo bruto
  if (cyc > 12000) {                                  // header 9+4.5ms -> NEC
    irBit = 0; irBits = 0; irArm = 1;
    irRawN = 1;                                       // reinicia do header
    return;
  }
  if (!irArm) return;
  if (cyc > 1700) irBits |= (1UL << irBit);           // 1120us=0 / 2250us=1
  if (++irBit >= 32) {
    irArm = 0;
    gIrCode = irBits; gIrNew = true;
    irRawNew = true;                                  // congelado p/ replay
  }
}
static void irRxSetup() {
  pinMode(IR_RX_PIN, INPUT_PULLUP);   // pull-up: pin flutuante gerava tempestade de IRQ (reboots!)
  attachInterrupt(digitalPinToInterrupt(IR_RX_PIN), irIsr, FALLING);
}
static void irRawReset() { irRawN = 0; irRawNew = false; }
// ---------- Tabela de marcas (TV/AR) ----------
#include "ir_brands.h"
// ---------- Banco canonico TV-B-Gone (NA + EU) ----------
#include "tvbg_codes.h"
// envia 32/48 bits LSB-first com tempo NEC (560/1690)
static void irNecBitsLSB(uint32_t bits, int n) {
  for (int i = 0; i < n; i++) {
    carrier38(560);
    delayMicroseconds((bits & 1) ? 1690 : 560);
    bits >>= 1;
  }
  carrier38(560);
}
static void irSendNec(uint8_t addr, uint8_t cmd);   // (definido abaixo)
static void irSendRawKey(int kind, uint32_t a, uint32_t c) {
  switch (kind) {
    case 0:                                    // NEC: addr ~addr cmd ~cmd
      irSendNec((uint8_t)a, (uint8_t)c);
      break;
    case 1: {                                  // NECext: addr(16) cmd ~cmd
      uint32_t f = (a & 0xFFFF) | ((uint32_t)(c & 0xFF) << 16) | ((uint32_t)(uint8_t)~(c & 0xFF) << 24);
      carrier38(9000); delayMicroseconds(4500);
      irNecBitsLSB(f, 32);
      break;
    }
    case 2: {                                  // Samsung32: header 4.5/4.5 + 32 bits LSB
      carrier38(4500); delayMicroseconds(4500);
      irNecBitsLSB(a, 32);
      break;
    }
    case 3: {                                  // SIRC 12 bits (cmd 7 + addr 5, 40kHz)
      irCarrierHz(40000);
      uint16_t f = ((uint16_t)(c & 0x7F)) | ((uint16_t)(a & 0x1F) << 7);
      irMark(2400); irDelayUs(600);
      for (int i = 0; i < 12; i++) {
        irMark(600);
        irDelayUs(((f >> i) & 1) ? 1200 : 600);
      }
      break;
    }
    case 4: {                                  // RC5 14 bits Manchester (36kHz)
      irCarrierHz(36000);
      uint16_t f = 0x3000 | ((a & 0x1F) << 6) | (c & 0x3F);
      for (int i = 13; i >= 0; i--) {
        bool b = (f >> i) & 1;
        if (b) { irMark(444); irDelayUs(444); }   // 1: MARK + SPACE
        else   { irDelayUs(444); irMark(444); }   // 0: SPACE + MARK
      }
      break;
    }
    default:
      break;
  }
}// replay fiel dos tempos aprendidos: funciona com NEC, RC5, Sony, qualquer um
static void irPlayTimes(const uint16_t *t, int n) {
  if (n < 2) return;
  int start = 1;                             // amostra 0 = intervalo anterior (gap/lixo)
  while (start < n && t[start] >= 50000) start++;   // pula gaps longos remanescentes
  for (int i = start; i < n; i++) {
    uint32_t us = t[i];
    if (us > 60000) us = 60000;
    if ((i - start) % 2 == 0) carrier38(us);       // MARK
    else irDelayUs(us);                            // SPACE
  }
}
static void irSendFrame(uint32_t frame, int usMark, int us0, int us1) {
  for (int bit = 0; bit < 32; bit++) {        // NEC: bit menos significativo 1o
    carrier38(usMark);
    delayMicroseconds(((frame >> bit) & 1) ? us1 : us0);
  }
  carrier38(usMark);
}
static void irSendNec(uint8_t addr, uint8_t cmd) {   // NEC completo (2 repeticoes)
  uint32_t frame = (uint32_t)addr | ((uint32_t)(uint8_t)(~addr) << 8) |
                   ((uint32_t)cmd << 16) | ((uint32_t)(uint8_t)(~cmd) << 24);
  for (int rep = 0; rep < 2; rep++) {
    carrier38(9000); delayMicroseconds(4500);
    irSendFrame(frame, 560, 560, 1690);
    if (rep == 0) delayMicroseconds(40000);
  }
}
static void sendNEC(uint8_t addr, uint8_t cmd) {   // atalho usado pelos botoes
  irSendNec(addr, cmd);
  delay(110);
}

// ---------------- IR ASSINCRONO --------------------------------------------
// O envio de um quadro IR leva ~60..150 ms com pulsos (carrier38 + delays).
// Feito na task da UI, o botao travava o relogio por esse tempo.
// Aqui o quadro vai para uma fila e uma task no nucleo 0 faz o trabalho.
#define IR_JOB_RAW   6      // replay de tempos crus
#define IR_JOB_NECSW 255    // NEC com addr/cmd trocados (compatibilidade)
#define IR_JOB_TVBG  7      // codigo comprimido canonico TV-B-Gone
static void tvbgSendCode(const IrCode *code);      // (definido abaixo)
struct IrJob { const uint16_t *raw; int nraw; uint8_t kind; uint32_t a, c; const IrCode *tvbg; };
static QueueHandle_t irQ = nullptr;
static void irTaskFn(void *) {
  IrJob j;
  while (true) {
    if (xQueueReceive(irQ, &j, portMAX_DELAY) != pdTRUE) continue;
    if (j.kind == IR_JOB_TVBG && j.tvbg) tvbgSendCode(j.tvbg);
    else if (j.kind == IR_JOB_RAW && j.raw && j.nraw > 4) irPlayTimes(j.raw, j.nraw);
    else if (j.kind == IR_JOB_NECSW) irSendNec((uint8_t)(j.c & 0xFF), (uint8_t)(j.a & 0xFF));
    else irSendRawKey(j.kind, j.a, j.c);
    vTaskDelay(pdMS_TO_TICKS(40));       // folga entre quadros
  }
}
static void irInit(void) {
  if (irQ) { irCarrierSetup(); return; }
  irCarrierSetup();
  irQ = xQueueCreate(4, sizeof(IrJob));
  if (irQ) xTaskCreatePinnedToCore(irTaskFn, "ir", 4096, NULL, 3, NULL, 0);   // nucleo 0
}
// enfileira o envio (nao bloqueia); devolve false se a fila estiver cheia
static bool irSendAsync(uint8_t kind, uint32_t a, uint32_t c,
                        const uint16_t *raw = nullptr, int nraw = 0) {
  irInit();
  if (!irQ) return false;
  IrJob j; j.raw = raw; j.nraw = nraw; j.kind = kind; j.a = a; j.c = c; j.tvbg = nullptr;
  return xQueueSend(irQ, &j, 0) == pdTRUE;
}
static bool irSendAsyncTvbg(const IrCode *code) {
  irInit();
  if (!irQ) return false;
  IrJob j; j.raw = nullptr; j.nraw = 0; j.kind = IR_JOB_TVBG; j.a = 0; j.c = 0; j.tvbg = code;
  return xQueueSend(irQ, &j, 0) == pdTRUE;
}

// ---------------- display LVGL ----------------
static void disp_flush(lv_disp_drv_t *d, const lv_area_t *area, lv_color_t *color_p) {
  if (!gLvPause && !gSdPause) {
    uint32_t w = area->x2 - area->x1 + 1, h = area->y2 - area->y1 + 1;
    // Se nao conseguir o barramento (timeout), NAO desenha: desenhar sem o
    // lock disputaria o SPI com o SD e corromperia a gravacao do cartao.
    if (busLock()) {
      tft.startWrite();
      tft.setAddrWindow(area->x1, area->y1, w, h);
      tft.writePixels((uint16_t *)&color_p->full, w * h);
      tft.endWrite();
      busUnlock();
    }
  }
  lv_disp_flush_ready(d);
}
volatile bool gWakeWait = false;   // espera de DUPLO toque (janela 2s) p/ acender
uint32_t gWakeWin = 0;             // inicio da janela de 2s
int  gWakeTaps = 0;                // toques contados dentro da janela
bool gWakeLight = false;           // true: reentra lightsleep; false: wifi (luz apagada)
static void touch_read(lv_indev_drv_t *drv, lv_indev_data_t *data) {
  (void)drv;
  if (gLvPause) { data->state = LV_INDEV_STATE_RELEASED; data->continue_reading = false; return; }
  // Sessao do SD: o touch nao pode tocar o barramento (ele divide o MISO com
  // o cartao e o XPT2046 dirige a linha enquanto esta selecionado).
  if (gSdPause) { data->state = LV_INDEV_STATE_RELEASED; data->continue_reading = false; return; }
  if (gWakeWait) { data->state = LV_INDEV_STATE_RELEASED; data->continue_reading = false; return; }   // tela apagada: UI nao ve toque
  int x, y;
  bool p = tsPoint(&x, &y);
  if (p) {
    data->point.x = x; data->point.y = y;
    data->state = LV_INDEV_STATE_PRESSED;
    gLastTouch = millis();
    gTouchDown = true;
    gTx = x; gTy = y;
  } else { data->state = LV_INDEV_STATE_RELEASED; gTouchDown = false; }
}

// ---------------- helpers de UI ----------------
static lv_obj_t *sbT[24], *sbP[24], *sbW[24], *sbB[24];
static int sbTN = 0, sbPN = 0, sbWN = 0, sbBN = 0;
// header proprio da Home (a arte cobre a tela toda; nada de addStatus la')
static lv_obj_t *homeTime = nullptr, *homePct = nullptr, *homeBat = nullptr;
static bool btActive() { return gBtOn; }   // azul enquanto o BLE estiver em uso
static void toast(const char *txt);       // (definido adiante; usado no statusTimer)
static void addStatus(lv_obj_t *parent) {   // hora + wifi/BT + bateria (Redmi Watch 5)
  // libera slots de telas ja apagadas (telas de app sao recriadas a cada abertura)
  int k = 0;
  for (int i = 0; i < sbTN; i++)
    if (lv_obj_is_valid(sbT[i])) sbT[k++] = sbT[i];
  sbTN = k;
  k = 0;
  for (int i = 0; i < sbWN; i++)
    if (lv_obj_is_valid(sbW[i])) sbW[k++] = sbW[i];
  sbWN = k;
  k = 0;
  for (int i = 0; i < sbBN; i++)
    if (lv_obj_is_valid(sbB[i])) sbB[k++] = sbB[i];
  sbBN = k;
  k = 0;
  for (int i = 0; i < sbPN; i++)
    if (lv_obj_is_valid(sbP[i])) sbP[k++] = sbP[i];
  sbPN = k;
  if (sbTN < 24) {
    sbT[sbTN] = lv_label_create(parent);
    lv_label_set_text(sbT[sbTN], "--:--");
    lv_obj_set_style_text_font(sbT[sbTN], F_UI, 0);
    lv_obj_set_style_text_color(sbT[sbTN], REDMI_DIM, 0);
    lv_obj_align(sbT[sbTN], LV_ALIGN_TOP_LEFT, 3, 1);
    lv_obj_clear_flag(sbT[sbTN], LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(sbT[sbTN], LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_EVENT_BUBBLE);
    sbTN++;
  }
  if (sbPN < 24) {
    sbP[sbPN] = lv_label_create(parent);
    lv_label_set_text(sbP[sbPN], "--%");
    lv_obj_set_style_text_font(sbP[sbPN], F_UI, 0);
    lv_obj_set_style_text_color(sbP[sbPN], REDMI_DIM, 0);
    lv_obj_align(sbP[sbPN], LV_ALIGN_TOP_RIGHT, -3, 1);
    lv_obj_clear_flag(sbP[sbPN], LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(sbP[sbPN], LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_EVENT_BUBBLE);
    sbPN++;
  }
  if (sbWN < 24) {                      // icone Wi-Fi (a esquerda da bateria)
    sbW[sbWN] = lv_label_create(parent);
    lv_label_set_text(sbW[sbWN], LV_SYMBOL_WIFI);
    lv_obj_set_style_text_font(sbW[sbWN], F_UI, 0);
    lv_obj_set_style_text_color(sbW[sbWN], REDMI_DIM, 0);
    lv_obj_align(sbW[sbWN], LV_ALIGN_TOP_RIGHT, -44, 1);
    lv_obj_clear_flag(sbW[sbWN], LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(sbW[sbWN], LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_EVENT_BUBBLE);
    sbWN++;
  }
  if (sbBN < 24) {                      // marcador Bluetooth (mesmo simbolo da face padrao)
    sbB[sbBN] = lv_label_create(parent);
    lv_label_set_text(sbB[sbBN], LV_SYMBOL_BLUETOOTH);
    lv_obj_set_style_text_font(sbB[sbBN], F_UI, 0);
    lv_obj_set_style_text_color(sbB[sbBN], REDMI_DIM, 0);
    lv_obj_align(sbB[sbBN], LV_ALIGN_TOP_RIGHT, -28, 1);
    lv_obj_clear_flag(sbB[sbBN], LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(sbB[sbBN], LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_EVENT_BUBBLE);
    sbBN++;
  }
}
static void statusTimer(lv_timer_t *) {
  time_t now = time(nullptr);
  char hh[8] = "--:--";
  if (timeOk()) strftime(hh, sizeof(hh), "%H:%M", localtime(&now));
  for (int i = 0; i < sbTN; i++)
    if (lv_obj_is_valid(sbT[i])) lv_label_set_text(sbT[i], hh);
  int p = batPct();
  char bb[8]; snprintf(bb, sizeof(bb), "%d%%", p);
  for (int j = 0; j < sbPN; j++)
    if (lv_obj_is_valid(sbP[j])) {
      lv_label_set_text(sbP[j], bb);
      lv_obj_set_style_text_color(sbP[j], p < 20 ? REDMI_ERR : REDMI_DIM, 0);
    }
  // aviso de bateria: uma vez ao cruzar 20%, outra ao cruzar 10% (rearma >30%)
  static int8_t batWarn = 2;                 // 2=ok  1=avisou<20  0=avisou<10
  if (p < 10 && batWarn == 1) {
    char w[26]; snprintf(w, sizeof(w), "Bateria critica %d%%!", p);
    toast(w); batWarn = 0;
  } else if (p < 20 && batWarn == 2) {
    char w[40]; snprintf(w, sizeof(w), "Bateria fraca %d%% - conecte o carregador", p);
    toast(w); batWarn = 1;
  } else if (p > 30) batWarn = 2;
  bool wOn = WiFi.status() == WL_CONNECTED;    // cinza=off  verde=conectado
  for (int j = 0; j < sbWN; j++)
    if (lv_obj_is_valid(sbW[j]))
      lv_obj_set_style_text_color(sbW[j], wOn ? REDMI_OK : REDMI_DIM, 0);
  bool bOn = btActive();                       // cinza=off  azul=ativo
  for (int j = 0; j < sbBN; j++)
    if (lv_obj_is_valid(sbB[j]))
      lv_obj_set_style_text_color(sbB[j], bOn ? lv_color_hex(0x4FC3F7) : REDMI_DIM, 0);
  // header da Home (posicoes identicas ao print apps.png)
  if (homeTime && lv_obj_is_valid(homeTime)) lv_label_set_text(homeTime, hh);
  if (homePct && lv_obj_is_valid(homePct)) {
    lv_label_set_text(homePct, bb);
    lv_obj_set_style_text_color(homePct, p < 20 ? lv_color_hex(0xFF3B30) : lv_color_hex(0xFFFFFF), 0);
  }
  if (homeBat && lv_obj_is_valid(homeBat))
    lv_obj_set_style_bg_color(homeBat, p < 20 ? lv_color_hex(0xFF3B30) : lv_color_hex(0x22B14C), 0);
  // NTP automatico enquanto conectado e sem hora
  if (WiFi.status() == WL_CONNECTED && !timeOk() && !gNetBusy &&
      millis() - gLastNtpTry > 120000) {
    gLastNtpTry = millis();
    netRun(T_NTP);
  }
}
// aviso temporario no centro da tela ativa
static lv_obj_t *gToast = nullptr;
static void toastFadeCb(lv_timer_t *x) {
  lv_obj_t *o = (lv_obj_t *)x->user_data;
  if (lv_obj_is_valid(o)) {
    lv_obj_fade_out(o, 200, 0);
    lv_obj_del_delayed(o, 220);
  }
}
static void toast(const char *txt) {
  if (gToast && lv_obj_is_valid(gToast)) lv_obj_del(gToast);
  gToast = lv_label_create(lv_layer_top());
  lv_label_set_text(gToast, txt);
  lv_obj_set_style_bg_color(gToast, REDMI_CARD, 0);
  lv_obj_set_style_bg_opa(gToast, LV_OPA_COVER, 0);
  lv_obj_set_style_pad_all(gToast, 6, 0);
  lv_obj_set_style_radius(gToast, 8, 0);
  lv_obj_set_style_text_color(gToast, REDMI_TXT, 0);
  lv_obj_set_style_text_font(gToast, F_UI, 0);
  lv_obj_set_style_max_width(gToast, 122, 0);
  lv_label_set_long_mode(gToast, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_align(gToast, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_center(gToast);
  lv_timer_t *t = lv_timer_create(toastFadeCb, 2200, gToast);
  t->repeat_count = 1;
}
// navegacao
static lv_obj_t *scrLock, *scrHome;
static bool gKbVis();
static void kbHideAll();
// telas de jogo: swipe-back global desligado (arrasto move o personagem)
static lv_obj_t *gGameScrs[10];
static int gGameCount = 0;
static void gGameReg(lv_obj_t *s) {
  // mantem so' telas ainda vivas (as antigas sao apagadas pelo nav) e sem duplicar
  int n = 0;
  for (int i = 0; i < gGameCount; i++)
    if (gGameScrs[i] && gGameScrs[i] != s && lv_obj_is_valid(gGameScrs[i]))
      gGameScrs[n++] = gGameScrs[i];
  if (n < 10) gGameScrs[n++] = s;
  gGameCount = n;
}
static uint32_t gNavMs = 0;                  // ultima navegacao (guarda contra rajada)
static void navP(lv_obj_t *scr, lv_scr_load_anim_t a, bool delOld) {
  if (gKbVis()) kbHideAll();               // qualquer navegação fecha o teclado
  lv_obj_t *old = lv_scr_act();
  if (!scr || scr == old) return;
  uint32_t nowMs = millis();
  // Rajada de toques (abrir varios apps em sequencia): a tela anterior ainda
  // esta' na animacao e cada tela extra segura buffers -> a PSRAM estourava.
  // Nesse caso descartamos a tela recem-criada em vez de empilhar.
  if (old != scrHome && old != scrLock && scr != scrLock && scr != scrHome &&
      (uint32_t)(nowMs - gNavMs) < 200) {
    if (lv_obj_is_valid(scr)) lv_obj_del(scr);
    return;
  }
  gNavMs = nowMs;
  psCleanup(true);                         // devolve buffers ociosos antes de entrar
  gOnLock = (scr == scrLock);
  gInGame = false;                         // so telas de jogo registradas ficam em modo jogo
  for (int i = 0; i < gGameCount; i++) if (gGameScrs[i] == scr) gInGame = true;
  bool canDel = delOld && old && old != scrLock && old != scrHome && old != scr;
  lv_scr_load_anim(scr, a, 160, 0, canDel);
}
static void nav(lv_obj_t *scr, lv_scr_load_anim_t a = LV_SCR_LOAD_ANIM_MOVE_LEFT) {
  // delOld=TRUE: libera a tela anterior a cada navegacao (evita o vazamento de
  // heap da LVGL que causava flash branco + reinicio). Home/Lock sao EXCLUIDOS
  // em navP (persistem). Os "Voltar" p/ uma tela de app especifica RE-RECRIAM o
  // alvo (veja goHtmlCb/fmBtnCb/plExitCb/bbBackCb/tvgExitCb/cfgResetCancelCb).
  navP(scr, a, true);
}
const char *uiScrName() {                 // diag: tela ativa p/ heartbeat
  lv_obj_t *s = lv_scr_act();
  if (s == scrLock) return "LOCK";
  if (s == scrHome) return "HOME";
  if (!s) return "NULL";
  return "APP";
}
static void scrStyle(lv_obj_t *scr) {
  lv_obj_set_style_bg_color(scr, REDMI_BG, 0);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
}
static lv_obj_t *appHeader(lv_obj_t *scr, const char *title) {
  addStatus(scr);
  lv_obj_t *bk = lv_btn_create(scr);
  lv_obj_set_size(bk, 28, 26);
  lv_obj_set_style_bg_color(bk, REDMI_CARD, 0);
  lv_obj_set_style_radius(bk, 8, 0);
  lv_obj_set_style_shadow_width(bk, 0, 0);
  lv_obj_align(bk, LV_ALIGN_TOP_LEFT, 2, 16);
  lv_obj_t *ic = lv_label_create(bk);
  lv_label_set_text(ic, LV_SYMBOL_LEFT);
  lv_obj_set_style_text_color(ic, REDMI_ACC, 0);
  lv_obj_center(ic);
  lv_obj_add_event_cb(bk, [](lv_event_t *) {
    navP(scrHome, LV_SCR_LOAD_ANIM_MOVE_RIGHT, true);
  }, LV_EVENT_CLICKED, NULL);
  // gesto arrastar p/ direita em telas de App: vai p/ home
  lv_obj_t *ti = lv_label_create(scr);
  lv_label_set_text(ti, title);
  lv_obj_set_style_text_font(ti, F_UI, 0);
  lv_obj_set_style_text_color(ti, REDMI_ACC, 0);
  lv_obj_align(ti, LV_ALIGN_TOP_LEFT, 36, 22);
  return bk;
}
// cabecalho de JOGO: sem relogio/barra, voltar compacto no topo (libera a
// linha y=18..42 para placar, vidas, onda etc.)
static void gameHeader(lv_obj_t *scr) {
  lv_obj_t *bk = lv_btn_create(scr);
  lv_obj_set_size(bk, 22, 16);
  lv_obj_set_pos(bk, 0, 0);
  lv_obj_set_style_bg_color(bk, REDMI_CARD, 0);
  lv_obj_set_style_radius(bk, 6, 0);
  lv_obj_set_style_shadow_width(bk, 0, 0);
  lv_obj_t *ic = lv_label_create(bk);
  lv_label_set_text(ic, LV_SYMBOL_LEFT);
  lv_obj_set_style_text_font(ic, F_SM, 0);
  lv_obj_set_style_text_color(ic, REDMI_ACC, 0);
  lv_obj_center(ic);
  lv_obj_add_event_cb(bk, [](lv_event_t *) {
    navP(scrHome, LV_SCR_LOAD_ANIM_MOVE_RIGHT, true);
  }, LV_EVENT_CLICKED, NULL);
}

// ---------------- ANIMACOES / TEMA VIVO ----------------
static void onceCb(lv_timer_t *t) { lv_timer_del(t); }   // timer de um disparo
static void btnPulseCb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_PRESSED) return;
  lv_obj_t *o = lv_event_get_target(e);
  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, o);
  lv_anim_set_exec_cb(&a, [](void *obj, int32_t v) { lv_obj_set_style_opa((lv_obj_t *)obj, v, 0); });
  lv_anim_set_values(&a, 255, 225);
  lv_anim_set_time(&a, 80);
  lv_anim_set_playback_time(&a, 80);
  lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
  lv_anim_start(&a);
}
static void animBtn(lv_obj_t *b) { lv_obj_add_event_cb(b, btnPulseCb, LV_EVENT_PRESSED, NULL); }
static void animFloat(lv_obj_t *o, int dy, uint32_t ms) {
  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, o);
  lv_anim_set_exec_cb(&a, [](void *obj, int32_t v) { lv_obj_set_style_translate_y((lv_obj_t *)obj, v, 0); });
  lv_anim_set_values(&a, 0, dy);
  lv_anim_set_time(&a, ms);
  lv_anim_set_playback_time(&a, ms);
  lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
  lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
  lv_anim_start(&a);
}
static void animFadeIn(lv_obj_t *o, uint32_t ms) {
  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, o);
  lv_anim_set_exec_cb(&a, [](void *obj, int32_t v) { lv_obj_set_style_opa((lv_obj_t *)obj, v, 0); });
  lv_anim_set_values(&a, LV_OPA_TRANSP, LV_OPA_COVER);
  lv_anim_set_time(&a, ms);
  lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
  lv_anim_start(&a);
}
static void animShake(lv_obj_t *o) {
  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, o);
  lv_anim_set_exec_cb(&a, [](void *obj, int32_t v) { lv_obj_set_style_translate_x((lv_obj_t *)obj, v, 0); });
  lv_anim_set_values(&a, 0, 3);
  lv_anim_set_time(&a, 55);
  lv_anim_set_playback_time(&a, 55);
  lv_anim_set_repeat_count(&a, 3);
  lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
  lv_anim_start(&a);
}
static void clickBeep(lv_event_t *e) { beepShort(); animBtn((lv_obj_t *)lv_event_get_target(e)); }   // feedback + pulso
static lv_obj_t *uiBtn(lv_obj_t *p, const char *txt, lv_event_cb_t cb, int w, int h,
                       uint32_t col = 0x161A22, void *ud = NULL) {
  lv_obj_t *b = lv_btn_create(p);
  lv_obj_set_size(b, w, h);
  lv_obj_set_style_bg_color(b, lv_color_hex(col), 0);
  lv_obj_set_style_radius(b, 9, 0);
  lv_obj_set_style_shadow_width(b, 0, 0);
  lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, ud);
  lv_obj_add_event_cb(b, clickBeep, LV_EVENT_CLICKED, NULL);
  lv_obj_t *l = lv_label_create(b);
  lv_label_set_text(l, txt);
  lv_obj_set_style_text_color(l, REDMI_TXT, 0);
  lv_obj_set_style_text_font(l, F_UI, 0);
  lv_obj_center(l);
  return b;
}
static lv_obj_t *uiLabel(lv_obj_t *p, const char *txt, int x, int y,
                         const lv_font_t *f = F_UI, uint32_t col = 0xF4F5F7) {
  lv_obj_t *l = lv_label_create(p);
  lv_label_set_text(l, txt);
  lv_obj_set_pos(l, x, y);
  lv_obj_set_style_text_font(l, f, 0);
  lv_obj_set_style_text_color(l, lv_color_hex(col), 0);
  return l;
}

// declaracoes de build
void buildLockS(); void buildHomeS();
void buildConfig(); void buildCfgReset(); void buildInfo(); void buildPers();
void buildWifi(); void buildTela(); void buildTelaTest(); void buildAjustes();
void buildDataHora(); void buildTeste(); void buildToolbox(); void buildMat();
void buildCalc(); void buildLevel(bool bolha); void buildSaude(); void buildGoogle();
void buildCron(); void buildBleScan(); void buildDado(); void buildClima(); void buildRemote(); void buildMoeda(); void buildNotas();
static void remoteCb(lv_event_t *);
void buildBrands(bool ac);
void buildTvBGone();
static void notasCb(lv_event_t *);
static void gameDiceCb(lv_event_t *);
void buildIA(); void buildJogos(); void buildArquivo();
void appCardCb(lv_event_t *e);
static void pollTimer(lv_timer_t *);
static void lockUpdate();
// ---------------- boot: logo com fade-in e saida suave ----------------
static lv_obj_t *btLogo = nullptr, *btBar = nullptr;
static void btFim(lv_timer_t *t) {
  if (lv_obj_is_valid(btLogo)) lv_obj_del(btLogo);
  if (lv_obj_is_valid(btBar))  lv_obj_del(btBar);
  btLogo = btBar = nullptr;
  lv_timer_del(t);
}
static void bootAnim() {
  if (btLogo) return;                           // ja esta rodando: nao empilha
  btLogo = lv_label_create(lv_scr_act());
  lv_label_set_text(btLogo, "SMARTWATCH");
  lv_obj_set_style_text_font(btLogo, F_MED, 0);
  lv_obj_set_style_text_color(btLogo, REDMI_ACC, 0);
  lv_obj_align(btLogo, LV_ALIGN_CENTER, 0, -10);
  lv_obj_set_style_opa(btLogo, LV_OPA_TRANSP, 0);
  btBar = lv_obj_create(lv_scr_act());
  lv_obj_set_size(btBar, 90, 4);
  lv_obj_align(btBar, LV_ALIGN_CENTER, 0, 14);
  lv_obj_set_style_bg_color(btBar, lv_color_hex(0x1C202B), 0);
  lv_obj_set_style_radius(btBar, 2, 0);
  lv_obj_set_style_border_opa(btBar, 0, 0);
  lv_obj_set_style_shadow_width(btBar, 0, 0);
  // barra de progresso (simula carregamento)
  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, btLogo);
  lv_anim_set_exec_cb(&a, [](void *obj, int32_t v) {
    lv_obj_set_style_opa((lv_obj_t *)obj, v, 0);
    if (lv_obj_is_valid(btBar)) lv_obj_set_size(btBar, 90 * v / 255, 4);
  });
  lv_anim_set_values(&a, LV_OPA_TRANSP, LV_OPA_COVER);
  lv_anim_set_time(&a, 900);
  lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
  lv_anim_start(&a);
  // some sozinho (1 disparo) - ANTES disto estavamos criando um timer
  // recursivo: bootAnim() chamava bootAnim() a cada 250ms = "pulsar eterno"
  lv_timer_t *tEnd = lv_timer_create(btFim, 1600, NULL);
  tEnd->repeat_count = 1;
}
void uiInit();
void buildFileMidia();
static String   audFiles[32];  // (declaradas aqui p/ o browser de Arquivos usar)
static int      audCount = 0, audIdx = 0;
void buildPlayer(int idx);      // player de musica WAV do SD
void buildImgView(const String &path);
static lv_obj_t *scrVideo = nullptr;
void buildVideoView(const String &path);
void serverPreview(const String &path);
static bool audList();
#if HW_BLE
void buildBadBLE();           // teclado Bluetooth (HID) + runner /duck.txt
#if HW_BLE
void buildBtMidia();
#endif
#endif
bool serverActive();
bool serverCloseTap();
void uiServerStart();
void uiServerStop();
void uiMirrorStart();
void uiMirrorStop();
void mirrorTick();
bool mirrorOn();

// ponteiros de todas as telas
static lv_obj_t *scrWifi, *scrTela, *scrTelaTest, *scrAjustes, *scrDataHora,
    *scrConfig, *scrCfgReset, *scrTeste, *scrToolbox, *scrMat, *scrCalc, *scrLevel, *scrSaude,
    *scrGoogle, *scrIA, *scrJogos, *scrArquivo, *scrInfo, *scrPers,
    *scrCron, *scrDado, *scrBle, *scrClima, *scrRemote, *scrMoeda, *scrNotas;
volatile bool gWantLock = false;   // loop pede retorno ao relogio por inatividade
volatile bool gWakeArm  = false;   // 2 toques p/ acordar: 1o toque so acende


// callback de rede pendente (disparado pelo pollTimer)
static void (*gNetCb)(bool ok, const String &out) = nullptr;

// ============================================================
//  REFINO ESTETICO REDMI WATCH 5
//  Cores mais fiéis ao Redmi: fundo preto profundo, accent laranja,
//  texto branco/cinza, cards arredondados, layout minimalista.
// ============================================================
// ============================================================
//  TECLADO CUSTOM (teclas grandes + espacamento; controles no TOPO)
// ============================================================
static lv_obj_t *gKb = nullptr;          // container na layer_top
static lv_obj_t *gKbTa = nullptr;        // campo de texto alvo
static lv_obj_t *gKbRows = nullptr;      // container das linhas de teclas
static lv_obj_t *gKbCaseB = nullptr;     // botao ab/AB (label muda)
static lv_obj_t *gKbSelB = nullptr;      // botao 123/abc (label muda)
static lv_obj_t *wmRaise = nullptr, *wmB1 = nullptr, *wmB2 = nullptr;   // modal q sobe c/ teclado + botoes
static int gKbMode = 0;                  // 0=abc 1=ABC 2=num/simb

static const char *KB_ROWS[3][4] = {
  { "qwertyu", "iopasdf", "ghjklzx", "cvbnm." },
  { "QWERTYU", "IOPASDF", "GHJKLZX", "CVBNM." },
  { "1234567", "890-/;(", ")+=@$&%", "!?#_.,<" },
};
static bool gKbVis();
static void kbHideAll();
static void kbRebuild();
static void kbShow(lv_obj_t *ta);

static void kbKeyCb(lv_event_t *e) {
  char c = (char)(intptr_t)lv_event_get_user_data(e);
  if (gKbTa && lv_obj_is_valid(gKbTa)) lv_textarea_add_char(gKbTa, c);
  beepShort(1600, 12);
}
static void kbCtlCb(lv_event_t *e) {
  int op = (int)(intptr_t)lv_event_get_user_data(e);
  if (op == 0) {                                     // backspace
    if (gKbTa && lv_obj_is_valid(gKbTa)) lv_textarea_del_char(gKbTa);
  } else if (op == 1) {                              // ab <-> AB
    gKbMode = (gKbMode == 1) ? 0 : 1;
    kbRebuild();
  } else if (op == 2) {                              // 123 <-> letras
    gKbMode = (gKbMode == 2) ? 0 : 2;
    kbRebuild();
  } else if (op == 3) {                              // espaco
    if (gKbTa && lv_obj_is_valid(gKbTa)) lv_textarea_add_char(gKbTa, ' ');
    beepShort(1600, 12);
  } else {                                           // OK/enter
    kbHideAll();
  }
}
static lv_obj_t *kbBtn(lv_obj_t *p, const char *txt, int x, int y, int w, int h) {
  lv_obj_t *b = lv_btn_create(p);
  lv_obj_set_pos(b, x, y);
  lv_obj_set_size(b, w, h);
  lv_obj_set_style_bg_color(b, REDMI_CARD, 0);
  lv_obj_set_style_radius(b, 3, 0);
  lv_obj_set_style_shadow_width(b, 0, 0);
  lv_obj_clear_flag(b, LV_OBJ_FLAG_CLICK_FOCUSABLE);  // tecla NAO rouba o foco do campo
  lv_obj_t *l = lv_label_create(b);
  lv_label_set_text(l, txt);
  lv_obj_set_style_text_font(l, F_UI, 0);
  lv_obj_set_style_text_color(l, REDMI_TXT, 0);
  lv_obj_center(l);
  return b;
}
static void kbRebuild() {
  if (!gKbRows || !lv_obj_is_valid(gKbRows)) return;
  lv_obj_clean(gKbRows);
  for (int r = 0; r < 4; r++) {
    const char *row = KB_ROWS[gKbMode][r];
    int n = (int)strlen(row);
    int w = 14, gap = 4;
    int x0 = (128 - (n * w + (n - 1) * gap)) / 2;
    for (int i = 0; i < n; i++) {
      char one[2] = { row[i], 0 };
      lv_obj_t *b = kbBtn(gKbRows, one, x0 + i * (w + gap), r * 19 + 2, w, 17);
      lv_obj_add_event_cb(b, kbKeyCb, LV_EVENT_CLICKED, (void *)(intptr_t)row[i]);
    }
  }
  if (gKbCaseB && lv_obj_is_valid(gKbCaseB))
    lv_label_set_text(lv_obj_get_child(gKbCaseB, 0), gKbMode == 1 ? "AB" : "ab");
  if (gKbSelB && lv_obj_is_valid(gKbSelB))
    lv_label_set_text(lv_obj_get_child(gKbSelB, 0), gKbMode == 2 ? "abc" : "123");
}
static void kbBuild() {
  gKb = lv_obj_create(lv_layer_top());
  lv_obj_set_pos(gKb, 0, 62);
  lv_obj_set_size(gKb, 128, 98);
  lv_obj_set_style_pad_all(gKb, 0, 0);       // tema injeta pad ~10px -> cortava a direita!
  lv_obj_set_style_bg_color(gKb, lv_color_hex(0x12151C), 0);
  lv_obj_set_style_bg_opa(gKb, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(gKb, 0, 0);
  lv_obj_set_style_radius(gKb, 0, 0);
  lv_obj_clear_flag(gKb, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(gKb, LV_OBJ_FLAG_CLICK_FOCUSABLE);   // clique no vazio nao defoca
  lv_obj_set_scrollbar_mode(gKb, LV_SCROLLBAR_MODE_OFF);
  // linha de controles no TOPO: <-  ab/AB  123  [espaco]  OK
  static const int cw[5] = { 18, 30, 30, 28, 18 };
  static const char *ct[5] = { "<-", "ab", "123", "_", "OK" };
  int x = 1;
  for (int i = 0; i < 5; i++) {
    lv_obj_t *b = kbBtn(gKb, ct[i], x, 1, cw[i], 18);
    if (i == 1) gKbCaseB = b;
    if (i == 2) gKbSelB = b;
    lv_obj_add_event_cb(b, kbCtlCb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    x += cw[i] + 1;
  }
  gKbRows = lv_obj_create(gKb);
  lv_obj_set_pos(gKbRows, 0, 21);
  lv_obj_set_size(gKbRows, 128, 77);
  lv_obj_set_style_bg_opa(gKbRows, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(gKbRows, 0, 0);
  lv_obj_set_style_pad_all(gKbRows, 0, 0);
  lv_obj_clear_flag(gKbRows, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(gKb, LV_OBJ_FLAG_HIDDEN);
}
static void kbShow(lv_obj_t *ta) {
  gKbTa = ta;
  gKbMode = 0;
  lv_obj_clear_flag(gKb, LV_OBJ_FLAG_HIDDEN);
  kbRebuild();
  lv_obj_scroll_to_view(ta, LV_ANIM_ON);          // campo entra na area livre do teclado
  if (wmRaise && lv_obj_is_valid(wmRaise)) {          // sobe o modal p/ campo visivel
    lv_obj_set_pos(wmRaise, 1, 2);
    lv_obj_set_size(wmRaise, 126, 58);
    if (wmB1) lv_obj_add_flag(wmB1, LV_OBJ_FLAG_HIDDEN);
    if (wmB2) lv_obj_add_flag(wmB2, LV_OBJ_FLAG_HIDDEN);
  }
}
static bool gKbVis() {
  return gKb && lv_obj_is_valid(gKb) && !lv_obj_has_flag(gKb, LV_OBJ_FLAG_HIDDEN);
}
static void kbHideAll() {
  gKbTa = nullptr;
  if (gKb) lv_obj_add_flag(gKb, LV_OBJ_FLAG_HIDDEN);
  if (wmRaise && lv_obj_is_valid(wmRaise)) {          // restaura posicao/tamanho/botoes
    lv_obj_set_pos(wmRaise, 1, 52);
    lv_obj_set_size(wmRaise, 126, 104);
    if (wmB1) lv_obj_clear_flag(wmB1, LV_OBJ_FLAG_HIDDEN);
    if (wmB2) lv_obj_clear_flag(wmB2, LV_OBJ_FLAG_HIDDEN);
  }
}
static void taFocusCb(lv_event_t *e) {
  // apenas FOCUSED abre o teclado; DEFOCUSED escondia E clicar numa tecla/fora
  // defoca o campo (click-focus do LVGL) -> teclado sumia a cada toque
  if (lv_event_get_code(e) == LV_EVENT_FOCUSED) kbShow((lv_obj_t *)lv_event_get_target(e));
}
static void taClickCb(lv_event_t *e) {
  static bool inCb = false;
  if (inCb) return;
  // campo ja focado: FOCUSED nao re-dispara -> clicar de novo reabre o teclado
  if (gKb && lv_obj_has_flag(gKb, LV_OBJ_FLAG_HIDDEN)) {
    inCb = true;
    kbShow((lv_obj_t *)lv_event_get_target(e));
    inCb = false;
  }
}
static void kbHideEv(lv_event_t *) { kbHideAll(); }   // adapter p/ callback LVGL
static lv_obj_t *uiTa(lv_obj_t *p, int x, int y, int w, int h, bool psw = false) {
  lv_obj_t *ta = lv_textarea_create(p);
  lv_obj_set_pos(ta, x, y);
  lv_obj_set_size(ta, w, h);
  lv_textarea_set_one_line(ta, true);
  if (psw) lv_textarea_set_password_mode(ta, true);
  lv_obj_set_style_bg_color(ta, lv_color_hex(0x12151C), 0);
  lv_obj_set_style_bg_opa(ta, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(ta, REDMI_DIM, 0);
  lv_obj_set_style_border_width(ta, 1, 0);
  lv_obj_set_style_radius(ta, 6, 0);
  lv_obj_set_style_text_color(ta, REDMI_TXT, 0);
  lv_obj_set_style_text_font(ta, F_UI, 0);
  lv_obj_set_style_pad_all(ta, 4, 0);
  lv_obj_add_event_cb(ta, taFocusCb, LV_EVENT_FOCUSED, NULL);
  lv_obj_add_event_cb(ta, taFocusCb, LV_EVENT_DEFOCUSED, NULL);
  lv_obj_add_event_cb(ta, taClickCb, LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(ta, kbHideEv, LV_EVENT_READY, NULL);
  lv_obj_add_event_cb(ta, kbHideEv, LV_EVENT_CANCEL, NULL);
  return ta;
}

// ============================================================
//  CORPO DE APP (area abaixo do cabecalho)
// ============================================================
static lv_obj_t *appBody(lv_obj_t *scr) {
  lv_obj_t *b = lv_obj_create(scr);
  lv_obj_set_pos(b, 0, 46);
  lv_obj_set_size(b, 128, 112);
  lv_obj_set_style_bg_color(b, REDMI_BG, 0);
  lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);
  lv_obj_set_style_border_opa(b, 0, 0);
  lv_obj_set_style_pad_all(b, 5, 0);
  lv_obj_set_scrollbar_mode(b, LV_SCROLLBAR_MODE_OFF);
  return b;
}
// linha de lista estilo Redmi Watch: card arredondado, icone accent a esq, seta a dir
static lv_obj_t *redRow(lv_obj_t *parent, const char *glyph, const char *txt,
                        lv_event_cb_t cb, int y, int h = 18,
                        uint32_t bgc = 0x161A22, uint32_t txc = 0xF4F5F7) {
  lv_obj_t *b = lv_btn_create(parent);
  lv_obj_set_pos(b, 0, y);
  lv_obj_set_size(b, 118, h);
  lv_obj_set_style_bg_color(b, lv_color_hex(bgc), 0);
  lv_obj_set_style_radius(b, 9, 0);
  lv_obj_set_style_shadow_width(b, 0, 0);
  if (cb) lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *gl = lv_label_create(b);
  lv_label_set_text(gl, glyph);
  lv_obj_set_style_text_font(gl, F_UI, 0);
  lv_obj_set_style_text_color(gl, REDMI_ACC, 0);
  lv_obj_align(gl, LV_ALIGN_LEFT_MID, 3, 0);
  lv_obj_clear_flag(gl, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_t *l = lv_label_create(b);
  lv_label_set_text(l, txt);
  lv_obj_set_style_text_font(l, F_UI, 0);
  lv_obj_set_style_text_color(l, lv_color_hex(txc), 0);
  lv_obj_align(l, LV_ALIGN_LEFT_MID, 21, 0);
  lv_obj_clear_flag(l, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_t *ar = lv_label_create(b);
  lv_label_set_text(ar, LV_SYMBOL_RIGHT);
  lv_obj_set_style_text_font(ar, F_SM, 0);
  lv_obj_set_style_text_color(ar, REDMI_DIM, 0);
  lv_obj_align(ar, LV_ALIGN_RIGHT_MID, -4, 0);
  lv_obj_clear_flag(ar, LV_OBJ_FLAG_CLICKABLE);
  return b;
}
// linha de botao estilo lista
static lv_obj_t *uiRow(lv_obj_t *parent, const char *txt, lv_event_cb_t cb, int y,
                       int h = 20, uint32_t col = 0x161A22) {
  lv_obj_t *b = lv_btn_create(parent);
  lv_obj_set_pos(b, 0, y);
  lv_obj_set_size(b, 118, h);
  lv_obj_set_style_bg_color(b, lv_color_hex(col), 0);
  lv_obj_set_style_radius(b, 9, 0);
  lv_obj_set_style_shadow_width(b, 0, 0);
  if (cb) lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *l = lv_label_create(b);
  lv_label_set_text(l, txt);
  lv_obj_set_style_text_font(l, F_UI, 0);
  lv_obj_set_style_text_color(l, REDMI_TXT, 0);
  lv_obj_align(l, LV_ALIGN_LEFT_MID, 6, 0);
  return b;
}

// ============================================================
//  SESSAO SD (mesmo barramento do TFT: pausa render, remonta SPI)
//  No fluxo normal NAO desmonta: o cartao fica montado o boot inteiro e o
//  barramento compartilhado ja' e' serializado por busLock/gSdPause.
//  Excecao: CARD_UNKNOWN (o driver tentou reinicializar e falhou) = FS morto;
//  ai' desmonta e monta de novo (SD.end + SD.begin, sempre em par).
// ============================================================
// ---------------- SD: sessao ---------------------------------
// O cartao e' montado UMA vez (SD.begin no setup) e fica no estado de
// transferencia o boot inteiro. NAO re-inicializar por fora da lib: CMD0 zera
// o CRC_ON_OFF do cartao e a lib (supports_crc=true) passa a rejeitar todo
// bloco lido -> f_open devolve FR_DISK_ERR (errno 5).
static void sdEndSession();
// "montado" de verdade: CARD_UNKNOWN (4) tem o VFS registrado mas o DATA I/O
// esta' morto (disk_initialize falhou no auto-remount do FatFs).
static inline bool sdMountedOk() {
  sdcard_type_t t = SD.cardType();
  return t == CARD_MMC || t == CARD_SD || t == CARD_SDHC;
}
static bool sdBeginSession() {
  gSdPause = true;                    // pausa o render do TFT (barramento compartilhado)
  sdBusPrepare();                     // touch fora do MISO (o touch le nao rola aqui)
  busLock();                          // serializa com o flush do TFT
  // Monta so' se ainda nao montou (esquema do teste 4). UNKNOWN conta como
  // NAO montado: o VFS fica registrado, entao o SD.begin() sairia na hora e o
  // explorador listaria um FS morto — aqui o SD.end() vem junto com o begin
  // (par obrigatorio; o begin refaz init + CMD59 do CRC).
  if (SD.cardType() == CARD_UNKNOWN) {
    Serial.println("[sd] sessao: cartao em UNKNOWN, remontando");
    SD.end(); delay(300);
  }
  if (!sdMountedOk() && !sdTryBegin(2)) { sdEndSession(); return false; }
  return true;
}
static void sdEndSession() {
  pinMode(SD_CS_PIN, OUTPUT); digitalWrite(SD_CS_PIN, HIGH);   // CS alto no repouso (barramento compartilhado)
  busUnlock();
  gSdPause = false;
  // lv_refr_now() so pode ser chamado na task do LVGL (senao corrompe a fila
  // de eventos e trava o relogio). De outra task: pede o redesenho.
  if (lvCtx()) {
    if (lv_obj_is_valid(lv_scr_act())) {
      lv_obj_invalidate(lv_scr_act());
      lv_refr_now(gDisp);
    }
  } else {
    gSdRefreshPending = true;
  }
}

// ============================================================
//  WALLPAPER: 1a imagem do SD (JPEG/BMP) -> 128x160 RGB565 gravado
//  cru na SPIFFS (/wp.bin = 40960 B). Aparece atras do conteudo no
//  lock e no home. Reaproveita o JPEGDEC (crop central) da busca Google.
//  Obs: JPEG grande (foto de celular) pode nao caber na PSRAM — o
//  upload pelo navegador jah reescala fotos p/ <=640px (ver smartwatch.ino).
// ============================================================
#define WP_W 128
#define WP_H 160
#define WP_BYTES (WP_W * WP_H * 2)
static uint16_t *gWpaper = nullptr;     // buffer do wallpaper (PSRAM)
static bool      gWpLoaded = false;     // tem wallpaper valido carregado
static uint8_t  *gWpFile = nullptr;     // JPEG bruto lido do SD (PSRAM)

static void wpEnsure() {
  if (!gWpaper) gWpaper = (uint16_t *)heap_caps_malloc(WP_BYTES, MALLOC_CAP_SPIRAM);
  // gWpFile (JPEG bruto) e rascunho: so e alocado durante o decode e liberado
  // logo em seguida (sao 430 KB de PSRAM que nao precisam ficar ocupados)
}
static void wpLoadSpiffs() {            // le /wp.bin pro buffer (no build do lock/home)
  wpEnsure();
  if (!gWpaper) { gWpLoaded = false; return; }
  File f = SPIFFS.open("/wp.bin", "r");
  if (!f || f.size() < WP_BYTES) { gWpLoaded = false; if (f) f.close(); return; }
  size_t got = f.read((uint8_t *)gWpaper, WP_BYTES);
  f.close();
  gWpLoaded = (got == WP_BYTES);
}
static void wpSaveSpiffs() {
  if (!gWpaper) return;
  File f = SPIFFS.open("/wp.bin", "w");
  if (!f) return;
  f.write((const uint8_t *)gWpaper, WP_BYTES);
  f.close();
}
static bool wpDecodeJpgRam(const uint8_t *buf, int len) {   // JPEG -> gWpaper (crop central)
  wpEnsure();
  if (!gWpaper) return false;
  memset(gWpaper, 0, WP_BYTES);
  gImgOut = gWpaper; gOutW = WP_W; gOutH = WP_H;
  JPEGDEC jdec;
  if (jdec.openRAM((uint8_t *)buf, len, jpgDrawCb) != 0) { jdec.close(); gImgOut = nullptr; return false; }
  int cw = (jdec.getWidth()  + 7) >> 3, ch = (jdec.getHeight() + 7) >> 3;
  gImgCropX = cw > WP_W ? (cw - WP_W) / 2 : 0;
  gImgCropY = ch > WP_H ? (ch - WP_H) / 2 : 0;
  jdec.decode(0, 0, 8);                 // 1/8 (barato; crop central 128x160)
  jdec.close();
  gImgOut = nullptr; gOutW = IMG_W; gOutH = IMG_H;
  return true;
}
static bool wpDecodeBmp(File &f, uint32_t dataOff, int32_t w, int32_t h, uint16_t bpp) {
  wpEnsure();
  if (!gWpaper) return false;
  memset(gWpaper, 0, WP_BYTES);
  int16_t bppBytes = bpp / 8;
  if (bpp != 24 && bpp != 32) return false;         // so 24/32-bit
  uint32_t rowSize = ((bpp * w + 31) / 32) * 4;     // alinhado a 4 bytes
  uint8_t *row = (uint8_t *)malloc(rowSize);
  if (!row) return false;
  int ox = (w - WP_W) / 2, oy = (h - WP_H) / 2;     // offset do crop central
  for (int32_t y = 0; y < h; y++) {
    f.seek(dataOff + (uint32_t)(h - 1 - y) * rowSize);   // BMP: de baixo p/ cima
    f.read(row, rowSize);
    int ty = y - oy;
    if (ty < 0 || ty >= WP_H) continue;
    for (int32_t x = 0; x < w; x++) {
      int tx = x - ox;
      if (tx < 0 || tx >= WP_W) continue;
      uint32_t px = (uint32_t)x * bppBytes;
      uint16_t r5 = row[px + 2] >> 3, g6 = row[px + 1] >> 2, b5 = row[px] >> 3;   // BGR -> RGB565
      gWpaper[ty * WP_W + tx] = (r5 << 11) | (g6 << 5) | b5;
    }
  }
  free(row);
  return true;
}
static bool wpDecodeFile(const String &path) {    // decodifica imagem do SD -> gWpaper
  if (!sdBeginSession()) { toast("SD ausente"); return false; }
  bool isBmp = path.endsWith(".bmp") || path.endsWith(".BMP");
  File f = SD.open(path);
  bool ok = false;
  if (f) {
    if (isBmp) {
      uint8_t hdr[54];
      f.seek(0); f.read(hdr, 54);
      uint32_t dataOff = hdr[10] | (hdr[11]<<8) | (hdr[12]<<16) | ((uint32_t)hdr[13]<<24);
      int32_t w = hdr[18] | (hdr[19]<<8) | (hdr[20]<<16) | ((int32_t)hdr[21]<<24);
      int32_t h = hdr[22] | (hdr[23]<<8) | (hdr[24]<<16) | ((int32_t)hdr[25]<<24);
      uint16_t bpp = hdr[28] | (hdr[29]<<8);
      if (dataOff >= 54 && w >= WP_W && h >= WP_H) ok = wpDecodeBmp(f, dataOff, w, h, bpp);
    } else {
      wpEnsure();
      uint32_t sz = f.size();
      uint32_t rd = sz < (420u*1024u) ? sz : (420u*1024u);
      if (rd) {
        // aloca so o necessario e libera na hora (rascunho de decode)
        if (!psNeed(rd)) { f.close(); sdEndSession(); toast("memoria insuficiente"); return false; }
        gWpFile = (uint8_t *)heap_caps_malloc(rd, MALLOC_CAP_SPIRAM);
        if (gWpFile) {
          f.seek(0);
          uint32_t got = f.read(gWpFile, rd);
          ok = wpDecodeJpgRam(gWpFile, got);
          heap_caps_free(gWpFile);       // 430 KB devolvidos a PSRAM
          gWpFile = nullptr;
        }
      }
    }
    f.close();
  }
  sdEndSession();
  if (!ok) toast("falha ao decodificar (imagem).");
  return ok;
}
static void wpSetFromPath(const String &path) {
  if (!wpDecodeFile(path)) return;
  wpSaveSpiffs();
  cfg.wallp = true; cfgSave();
  toast("Wallpaper salvo na SPIFFS!");
}
static lv_obj_t *ivScr, *ivCan;
static String ivPath;
static void ivWallCb(lv_event_t *) {
  if (!ivPath.length()) return;
  wpSaveSpiffs(); cfg.wallp = true; cfgSave();
  toast("Wallpaper definido!");
  buildArquivo();
  nav(scrArquivo, LV_SCR_LOAD_ANIM_MOVE_RIGHT);
}
static void ivVoltarCb(lv_event_t *) { buildArquivo(); nav(scrArquivo, LV_SCR_LOAD_ANIM_MOVE_RIGHT); }
void buildImgView(const String &path) {
  if (!wpDecodeFile(path)) { toast("imagem nao abriu"); return; }
  ivPath = path;
  ivScr = lv_obj_create(NULL);
  scrStyle(ivScr);
  lv_obj_clear_flag(ivScr, LV_OBJ_FLAG_SCROLLABLE);
  ivCan = lv_canvas_create(ivScr);                 // imagem em tela cheia (fundo)
  lv_obj_set_pos(ivCan, 0, 0);
  lv_obj_set_size(ivCan, 128, 160);
  lv_canvas_set_buffer(ivCan, gWpaper, WP_W, WP_H, LV_IMG_CF_TRUE_COLOR);
  // faixas escuras p/ leitura do status/botoes sobre a foto
  lv_obj_t *shTop = lv_obj_create(ivScr);
  lv_obj_set_pos(shTop, 0, 0);
  lv_obj_set_size(shTop, 128, 42);
  lv_obj_set_style_bg_color(shTop, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(shTop, LV_OPA_50, 0);
  lv_obj_set_style_border_opa(shTop, 0, 0);
  lv_obj_set_style_radius(shTop, 0, 0);
  lv_obj_clear_flag(shTop, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_t *shBot = lv_obj_create(ivScr);
  lv_obj_set_pos(shBot, 0, 134);
  lv_obj_set_size(shBot, 128, 26);
  lv_obj_set_style_bg_color(shBot, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(shBot, LV_OPA_60, 0);
  lv_obj_set_style_border_opa(shBot, 0, 0);
  lv_obj_set_style_radius(shBot, 0, 0);
  lv_obj_clear_flag(shBot, LV_OBJ_FLAG_CLICKABLE);
  appHeader(ivScr, "IMAGEM");                      // status + voltar por cima
  lv_obj_t *bv = uiBtn(ivScr, "Voltar", ivVoltarCb, 60, 20, 0x161A22, NULL);
  if (bv) lv_obj_set_pos(bv, 2, 137);
  lv_obj_t *bw = uiBtn(ivScr, "Definir", ivWallCb, 60, 20, 0x10304A, NULL);
  if (bw) lv_obj_set_pos(bw, 66, 137);
  // estilo Redmi: fundo escuro na area de botoes
  lv_obj_set_style_bg_color(ivScr, REDMI_BG, 0);
  nav(ivScr, LV_SCR_LOAD_ANIM_MOVE_LEFT);
}
void serverPreview(const String &path) {           // chamado ao receber um arquivo no upload
  String lo = path; lo.toLowerCase();
  if (lo.endsWith(".wav")) {
    if (!audList()) { toast("sem .wav"); return; }
    int f = -1;
    for (int k = 0; k < audCount; k++) if (audFiles[k] == path) { f = k; break; }
    buildPlayer(f >= 0 ? f : 0);
  } else if (lo.endsWith(".jpg") || lo.endsWith(".jpeg") || lo.endsWith(".bmp") || lo.endsWith(".png")) {
    buildImgView(path);
  } else if (lo.endsWith(".mp4") || lo.endsWith(".mov") || lo.endsWith(".mkv") ||
             lo.endsWith(".avi") || lo.endsWith(".webm")) {
    buildVideoView(path);
  } else {
    toast(("recebido: " + path).c_str());
    buildArquivo(); nav(scrArquivo, LV_SCR_LOAD_ANIM_MOVE_RIGHT);
  }
}
static String videoPath = "";
static void videoBackCb(lv_event_t *) {
  buildArquivo();
  nav(scrArquivo, LV_SCR_LOAD_ANIM_MOVE_RIGHT);
}
static void videoDelCb(lv_event_t *) {
  if (!videoPath.length()) return;
  if (!sdBeginSession()) { toast("SD ausente"); return; }
  bool ok = SD.remove(videoPath);
  sdEndSession();
  toast(ok ? "video apagado" : "falha ao apagar");
  if (ok) videoBackCb(nullptr);
}
void buildVideoView(const String &path) {
  videoPath = path;
  uint32_t sz = 0;
  if (sdBeginSession()) {
    File f = SD.open(path);
    if (f) { sz = f.size(); f.close(); }
    sdEndSession();
  }
  scrVideo = lv_obj_create(NULL);
  scrStyle(scrVideo);
  appHeader(scrVideo, "VIDEO");
  lv_obj_t *body = appBody(scrVideo);
  lv_obj_t *nm = uiLabel(body, videoPath.c_str(), 0, 6, F_SM, lv_color_to32(REDMI_TXT));
  lv_obj_set_width(nm, 118);
  lv_label_set_long_mode(nm, LV_LABEL_LONG_DOT);
  char info[40];
  if (sz >= 1048576UL) snprintf(info, sizeof(info), "%.1f MB", sz / 1048576.0);
  else snprintf(info, sizeof(info), "%lu KB", (unsigned long)(sz / 1024));
  uiLabel(body, info, 0, 26, F_SM, lv_color_to32(REDMI_DIM));
  uiLabel(body, "Video armazenado no SD.", 0, 46, F_SM, lv_color_to32(REDMI_TXT));
  uiLabel(body, "Este relogio ainda nao tem", 0, 62, F_SM, lv_color_to32(REDMI_DIM));
  uiLabel(body, "decodificador de video.", 0, 78, F_SM, lv_color_to32(REDMI_DIM));
  uiLabel(body, "Copie para o PC para assistir.", 0, 94, F_SM, lv_color_to32(REDMI_DIM));
  lv_obj_t *bBack = uiBtn(body, "Voltar", videoBackCb, 58, 22, 0x161A22, NULL);
  if (bBack) lv_obj_set_pos(bBack, 2, 86);
  lv_obj_t *bDel = uiBtn(body, "Apagar", videoDelCb, 58, 22, 0x3B1B1B, NULL);
  if (bDel) lv_obj_set_pos(bDel, 66, 86);
  nav(scrVideo, LV_SCR_LOAD_ANIM_MOVE_LEFT);
}

static void wpSetFromSD() {            // pega a 1a imagem do SD e define como wallpaper
  if (!sdBeginSession()) { toast("SD ausente"); return; }
  String path;
  const char *dirs[3] = { "/", "/FOTOS", "/DCIM" };
  for (int d = 0; d < 3 && !path.length(); d++) {
    File r = SD.open(dirs[d]);
    if (!r) continue;
    File f = r.openNextFile();
    while (f) {
      bool isDir = f.isDirectory();
      String nm = String(f.name());
      f.close();
      String lo = nm; lo.toLowerCase();
      if (!isDir && (lo.endsWith(".jpg") || lo.endsWith(".jpeg") ||
                     lo.endsWith(".bmp") || lo.endsWith(".png"))) {
        path = String(dirs[d]) + (dirs[d][1] ? "/" : "") + nm;   // caminho completo
        break;
      }
      f = r.openNextFile();
    }
    r.close();
  }
  sdEndSession();
  if (!path.length()) { toast("nenhuma imagem no SD"); return; }
  wpSetFromPath(path);
}
static void wpClear() {                 // remove o wallpaper
  SPIFFS.remove("/wp.bin");
  cfg.wallp = false; cfgSave();
  gWpLoaded = false;
  toast("Wallpaper removido");
}

static bool sdList(String &out) {
  if (!sdBeginSession()) return false;
  out = "";
  File root = SD.open("/");
  int c = 0;
  if (root) {
    while (c < 14) {
      File f = root.openNextFile();
      if (!f) break;
      out += f.name();
      out += f.isDirectory() ? " (pasta)\n" : "\n";
      f.close();
      c++;
    }
    root.close();
  }
  sdEndSession();
  return out.length() > 0;
}

// ============================================================
//  MODO DIRETO (roda desenhos brutos no TFT; LVGL congelado)
// ============================================================
static void enterDirect(void (*fn)()) {
  gLvPause = true;
  blOn();
  fn();
  gLvPause = false;
  lv_obj_invalidate(lv_scr_act());
  lv_refr_now(gDisp);
}
static void colorTestDirect() {
  const uint16_t cols[7] = { 0xF800, 0x07E0, 0x001F, 0xFFFF, 0xFFE0, 0x07FF, 0x0000 };
  const char *nm[7] = { "VERMELHO", "VERDE", "AZUL", "BRANCO", "AMARELO", "CIANO", "PRETO" };
  for (int i = 0; i < 7; i++) {
    tft.fillScreen(cols[i]);
    tft.setTextColor(~cols[i] & 0xFFFF); tft.setTextSize(2);
    tft.setCursor(4, 4); tft.print(nm[i]);
    uint32_t t0 = millis();
    int x, y;
    while (millis() - t0 < 800 && !tsRaw(&x, &y)) delay(8);
  }
  tft.fillScreen(C_BG);
  int x, y;
  while (tsRaw(&x, &y)) delay(10);
  delay(150);
}
static void showColorTest() { colorTestDirect(); }

// ================= BEEP (falante) =================
static volatile bool gAudioBusy = false;   // player de musica tocando (evita conflitos de LEDC)
static void beepSpeaker(int f = 1200, int ms = 140) {
  if (gAudioBusy) return;              // nao briga com o player de musica (mesmo canal LEDC)
  spkCarrierEnsure();
  ledcWriteTone(SPK_PIN, f);
  delay(ms);
  ledcWriteTone(SPK_PIN, 0);
  spkCarrierEnsure();                  // devolve a portadora do player
  // NAO detach: o canal pode estar em uso pelo player (detach no meio = crash)
}

// ============================================================
//  LOCK
// ============================================================
static lv_obj_t *lockTime, *lockDate, *lockHint, *lockWx;
static lv_obj_t *lockWxIco = nullptr;    // icone do tempo (sol/nuvem/chuva) 12x12
static lv_obj_t *lockWxDesc = nullptr;   // descricao do clima ("Ensolarado")
static lv_obj_t *lockBatPct = nullptr;        // bateria % (padrao, topo-dir)
static lv_obj_t *lockBatIco = nullptr;        // icone bateria (padrao, topo-dir)
static lv_obj_t *lockWifi = nullptr;          // wifi azul topo-esq (padrao, sem temp)
static lv_obj_t *lockBt = nullptr;            // marcador BT (padrao, topo-dir)
static lv_obj_t *lockRingK = nullptr;         // anel km
static lv_obj_t *lockRingC = nullptr;         // anel calorias
static String gWxTxt;                 // "--° desc" do clima (atualiza em 2o plano)
static int     gWxTemp = 0;           // temperatura em C (0 = sem dado)
static String gWxDesc;                // descricao ("Ensolarado", "Nublado"...)
static int     gWxKind = 0;           // 0=Sol 1=Nublado 2=Chuva 3=Nublado
static lv_obj_t *lockSecs = nullptr;                       // segundos (estilo 1)
static lv_obj_t *lockSteps = nullptr;                      // passos (digital/analogico)
static lv_obj_t *lockWall = nullptr;   // canvas do wallpaper (fundo)
static lv_obj_t *lockRing  = nullptr;   // anel de atividade (passos) - face digital
static lv_obj_t *lockRingsImg = nullptr; // arte dos 3 aneis (face padrao, pixel a pixel)
static lv_obj_t *lockDiamond = nullptr;  // diamante 11x11 (face padrao)
static lv_obj_t *lockBatPill = nullptr;  // pilula da bateria 7x11 (face padrao)
static lv_obj_t *lockDial = nullptr;                       // aro do relogio analogico
static lv_obj_t *lockHand[3] = { nullptr };                // ponteiros: hora/min/seg
static lv_obj_t *lockCenter = nullptr;                     // pino central
static lv_obj_t *lockPlanet = nullptr, *lockCont = nullptr, *lockShade = nullptr, *lockGlow = nullptr;
static lv_obj_t *lockStars[8] = { nullptr };
static uint32_t lockLastClickMs = 0;
static void wakeHint(bool on) {            // dica no lock: APENAS quando o guard esta armado
  if (lv_obj_is_valid(lockHint)) {
    if (on) { lv_label_set_text(lockHint, "toque novamente p/ acordar");
              lv_obj_clear_flag(lockHint, LV_OBJ_FLAG_HIDDEN); }
    else      lv_obj_add_flag(lockHint, LV_OBJ_FLAG_HIDDEN);
  }
}
static void wakeArmDark(bool light) {      // tela foi ao escuro: espera DUPLO toque (2s)
  if (cfg.wake2) { gWakeWait = true; gWakeTaps = 0; gWakeWin = millis(); gWakeLight = light; }
  else gWakeArm = true;
}
static void wakeFinish() {                 // 2 toques em <= 2s: acende (sem destravar)
  blOn();
  gWakeWait = false; gWakeTaps = 0;
  if (cfg.wake2) gWakeArm = true;          // o toque que acordou nao destrava
  gLastTouch = millis();
}
static void lockUnlock() {
  if (!gOnLock) return;   // guard p/ chamadas dobradas (gesto + fallback cru)
  nav(scrHome, LV_SCR_LOAD_ANIM_OVER_RIGHT);
}
static lv_obj_t *lockDot(lv_obj_t *parent, lv_obj_t **slot, int8_t x, int8_t y, int8_t d,
                         uint32_t colr, uint8_t opa) {
  lv_obj_t *o = lv_obj_create(parent);
  lv_obj_set_pos(o, x, y);
  lv_obj_set_size(o, d, d);
  lv_obj_set_style_radius(o, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(o, lv_color_hex(colr), 0);
  lv_obj_set_style_bg_opa(o, opa, 0);
  lv_obj_set_style_border_width(o, 0, 0);
  lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(o, LV_OBJ_FLAG_CLICKABLE);
  *slot = o;
  return o;
}
static const uint8_t STAR_XY[8][2] = {
  { 8, 24 }, { 112, 30 }, { 22, 46 }, { 100, 20 },
  { 6, 78 }, { 118, 88 }, { 14, 150 }, { 110, 148 },
};
// ---- aneis de atividade da face padrao: sliders ROTATIVOS (arrastar o anel) ----
//  lime  = passos (0..10000) | verde = km (0..70, de 0,1 km) | rosa = calorias (0..400)
//  arrastar o anel ajusta o valor e recalcula os outros (tudo derivado de passos)
static uint32_t gRingTouchedAt = 0;
static volatile bool gRingActive = false;    // dedo esta girando um anel agora
static lv_obj_t *lockRingVal = nullptr;      // valor do anel sendo ajustado

// recalcula os 3 aneis a partir de cfg.steps (fonte unica de verdade)
//  passos: 0..10000 | km: 0..70 (0,1 km) | calorias: 0..400
static void ringSyncFromSteps() {
  if (lv_obj_is_valid(lockRing))  lv_arc_set_value(lockRing,  (int16_t)constrain((long)cfg.steps, 0, 10000));
  if (lv_obj_is_valid(lockRingK)) lv_arc_set_value(lockRingK, (int16_t)constrain((long)(cfg.steps * 0.007f), 0, 70));
  if (lv_obj_is_valid(lockRingC)) lv_arc_set_value(lockRingC, (int16_t)constrain((long)(cfg.steps * 0.04f), 0, 400));
}
// aplica o valor de um anel e propaga para os demais
static void ringApplyValue(int id, int v) {
  long steps = cfg.steps;
  if (id == 0)      steps = v;                        // passos
  else if (id == 1) steps = (long)(v / 0.007f);        // km (0,1 km) -> passos
  else              steps = (long)(v / 0.04f);         // calorias -> passos
  steps = constrain(steps, 0, 10000);
  if (steps == cfg.steps) return;
  cfg.steps = steps;
  char sb[12];
  cfgPut("steps", ltoa((long)cfg.steps, sb, 10));   // grava so o contador
  ringSyncFromSteps();
}
static void ringShowValue(int id, int v) {
  if (!lv_obj_is_valid(lockRingVal)) return;
  char b[16];
  if (id == 0)      snprintf(b, sizeof(b), "%d passos", v);
  else if (id == 1) snprintf(b, sizeof(b), "%.1f km", v / 10.0f);
  else              snprintf(b, sizeof(b), "%d kcal", v);
  lv_label_set_text(lockRingVal, b);
  lv_obj_clear_flag(lockRingVal, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(lockRingVal);
}
static void ringEventCb(lv_event_t *e) {
  lv_event_code_t c = lv_event_get_code(e);
  int id = (int)(intptr_t)lv_event_get_user_data(e);
  lv_obj_t *a = (lv_obj_t *)lv_event_get_target(e);
  if (c == LV_EVENT_PRESSED || c == LV_EVENT_PRESSING) {
    gRingActive = true;
    gRingTouchedAt = millis();
    return;
  }
  if (c == LV_EVENT_VALUE_CHANGED) {
    gRingTouchedAt = millis();
    if (!gRingActive) return;
    ringApplyValue(id, lv_arc_get_value(a));
    ringShowValue(id, lv_arc_get_value(a));
    return;
  }
  if (c == LV_EVENT_RELEASED || c == LV_EVENT_PRESS_LOST) {
    gRingActive = false;
    gRingTouchedAt = millis();
    ringApplyValue(id, lv_arc_get_value(a));
    int v = lv_arc_get_value(a);
    char t[40];
    if (id == 0)      snprintf(t, sizeof(t), "passos %d", v);
    else if (id == 1) snprintf(t, sizeof(t), "distancia %.1f km", v / 10.0f);
    else              snprintf(t, sizeof(t), "calorias %d kcal", v);
    toast(t);
    if (lv_obj_is_valid(lockRingVal)) lv_obj_add_flag(lockRingVal, LV_OBJ_FLAG_HIDDEN);
  }
}
// hit-test do anel: so' a FAIXA do arco (e o knob) e' clicavel -> os aneis
// nao roubam toques do resto da tela (toque fora deles destrava o relogio)
static void ringHitTest(lv_event_t *e) {
  lv_hit_test_info_t *info = (lv_hit_test_info_t *)lv_event_get_param(e);
  if (!info) return;
  lv_obj_t *a = (lv_obj_t *)lv_event_get_target(e);
  lv_coord_t w = lv_obj_get_style_arc_width(a, LV_PART_INDICATOR);
  float r = (float)lv_obj_get_width(a) / 2.0f - (float)w / 2.0f;
  float cx = (float)lv_obj_get_x(a) + (float)lv_obj_get_width(a) / 2.0f;
  float cy = (float)lv_obj_get_y(a) + (float)lv_obj_get_height(a) / 2.0f;
  float dx = (float)info->point->x - cx;
  float dy = (float)info->point->y - cy;
  float d = sqrtf(dx * dx + dy * dy);
  info->res = (fabsf(d - r) <= (float)w / 2.0f + 2.0f) ? true : false;
}
static lv_obj_t *lockArc(lv_obj_t *p, int x, int y, int sz, int range,
                         uint32_t col, int w, int val, int id) {
  lv_obj_t *a = lv_arc_create(p);
  lv_obj_set_pos(a, x, y);
  lv_obj_set_size(a, sz, sz);
  lv_arc_set_bg_angles(a, 0, 360);
  lv_arc_set_rotation(a, 270);                 // comeca no topo (12h)
  lv_arc_set_range(a, 0, range);
  lv_arc_set_value(a, constrain(val, 0, range));
  lv_obj_set_style_bg_opa(a, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_arc_color(a, lv_color_hex(0x1A1D24), LV_PART_MAIN);
  lv_obj_set_style_arc_width(a, w, LV_PART_MAIN);
  lv_obj_set_style_arc_color(a, lv_color_hex(col), LV_PART_INDICATOR);
  lv_obj_set_style_arc_width(a, w, LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(a, lv_color_hex(col), LV_PART_KNOB);   // knob do slider
  lv_obj_set_style_bg_opa(a, LV_OPA_COVER, LV_PART_KNOB);
  lv_obj_set_style_border_width(a, 0, LV_PART_KNOB);
  lv_obj_set_style_radius(a, LV_RADIUS_CIRCLE, LV_PART_KNOB);
  lv_obj_set_style_pad_all(a, 2, LV_PART_KNOB);
  // SLIDER ROTATIVO: clicavel apenas na faixa do arco (hit-test proprio)
  lv_obj_add_flag(a, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_ADV_HITTEST);
  lv_obj_add_event_cb(a, ringHitTest, LV_EVENT_HIT_TEST, NULL);
  lv_obj_add_event_cb(a, ringEventCb, LV_EVENT_PRESSED, (void *)(intptr_t)id);
  lv_obj_add_event_cb(a, ringEventCb, LV_EVENT_PRESSING, (void *)(intptr_t)id);
  lv_obj_add_event_cb(a, ringEventCb, LV_EVENT_VALUE_CHANGED, (void *)(intptr_t)id);
  lv_obj_add_event_cb(a, ringEventCb, LV_EVENT_RELEASED, (void *)(intptr_t)id);
  lv_obj_add_event_cb(a, ringEventCb, LV_EVENT_PRESS_LOST, (void *)(intptr_t)id);
  return a;
}
#define WX_SZ 12
static void drawWxIco(lv_obj_t *cv, int kind);   // icone do tempo (sol/nuvem/chuva)
void buildLockS() {
  scrLock = lv_obj_create(NULL);
  scrStyle(scrLock);                     // fundo REDMI_BG (preto profundo 0x0A0C10)
  lv_obj_clear_flag(scrLock, LV_OBJ_FLAG_SCROLLABLE);
  if (cfg.wallp) {                       // wallpaper de fundo (foto do SD na SPIFFS)
    wpLoadSpiffs();
    if (gWpLoaded && gWpaper) {
      lockWall = lv_canvas_create(scrLock);
      lv_obj_set_pos(lockWall, 0, 0);
      lv_obj_set_size(lockWall, WP_W, WP_H);
      lv_canvas_set_buffer(lockWall, gWpaper, WP_W, WP_H, LV_IMG_CF_TRUE_COLOR);
      lv_obj_set_style_opa(lockWall, LV_OPA_50, 0);   // dim: texto continua legivel
      lv_obj_clear_flag(lockWall, LV_OBJ_FLAG_SCROLLABLE);
      lv_obj_clear_flag(lockWall, LV_OBJ_FLAG_CLICKABLE);
    }
  }
  // status do relogio: tela digital/analogico usa a barra (hora+wifi+BT+bateria);
  // PADRAO (Redmi) e' minimalista: so temperatura (esq) + bateria (dir), sem hora miuda.
  if (cfg.lockStyle != 2) addStatus(scrLock);
  // tema espacial so nas faces digital/analogico (padrao fica preto limpo)
  if (cfg.lockStyle != 2 && cfg.esp) {
    for (int i = 0; i < 8; i++)
      lockDot(scrLock, &lockStars[i], STAR_XY[i][0], STAR_XY[i][1], 2, 0xC9CEDA, LV_OPA_60);
    lockDot(scrLock, &lockGlow, 44, 96, 40, 0xFFD54A, LV_OPA_30);
    lockDot(scrLock, &lockPlanet, 46, 98, 36, 0xBCBFC9, LV_OPA_COVER);
    lockDot(lockPlanet, &lockCont, 4, 8, 13, 0x3E9B4F, LV_OPA_80);
    lv_obj_t *tmpC;
    lockDot(lockPlanet, &tmpC, 18, 22, 9, 0x3E9B4F, LV_OPA_70);
    lockDot(scrLock, &lockShade, 46, 98, 36, 0x0B0D12, LV_OPA_COVER);
  } else {
    lockPlanet = lockShade = lockGlow = nullptr;
    for (int i = 0; i < 8; i++) lockStars[i] = nullptr;
  }
  // ---- PADRAO (Redmi Watch 6 / foto "padraoface.png") ----
  // Layout extraido pixel a pixel do print (127x160, fundo preto puro):
  //   "12"   ink x30..73  y38..71   -> F_HUGE em (30,29)
  //   "59"   ink x60..104 y88..121  -> F_HUGE em (58,79)
  //   aneis  x9..54 y85..132        -> arte RGB565 (face_rings_565)
  //   diamante 11x11 em (76,4) | "98%" verde ate x112 | pilula 7x11 em (116,4)
  lockTime = uiLabel(scrLock, "--", 30, 29, F_HUGE, 0xFFFFFF);   // HH
  lv_label_set_long_mode(lockTime, LV_LABEL_LONG_CLIP);
  lv_obj_set_width(lockTime, 60);

  lockSecs = uiLabel(scrLock, "--", 58, 79, F_HUGE, 0xFFFFFF);   // MM
  lv_label_set_long_mode(lockSecs, LV_LABEL_LONG_CLIP);
  lv_obj_set_width(lockSecs, 60);

  // clima: os objetos existem (a face digital reusa) mas nunca aparecem no padrao
  lockWx = uiLabel(scrLock, "", 3, 0, F_SM, lv_color_to32(REDMI_TXT));
  lv_obj_set_width(lockWx, 26);
  lv_label_set_long_mode(lockWx, LV_LABEL_LONG_CLIP);
  lockWxIco = lv_canvas_create(scrLock);
  static uint16_t wxBuf[WX_SZ * WX_SZ];
  lv_canvas_set_buffer(lockWxIco, wxBuf, WX_SZ, WX_SZ, LV_IMG_CF_TRUE_COLOR);
  lv_obj_set_pos(lockWxIco, 30, 1);
  lockWxDesc = uiLabel(scrLock, "", 44, 0, F_SM, lv_color_to32(REDMI_DIM));
  lv_obj_set_width(lockWxDesc, 60);
  lv_label_set_long_mode(lockWxDesc, LV_LABEL_LONG_CLIP);

  // topo-direito: "98%" verde (ink termina em x=112) + pilula 7x11 em (116,4)
  lockBatPct = uiLabel(scrLock, "98%", 82, 2, F_SM, 0x22B14C);
  lv_obj_set_style_text_align(lockBatPct, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_width(lockBatPct, 31);
  lv_label_set_long_mode(lockBatPct, LV_LABEL_LONG_CLIP);

  lockBatPill = lv_obj_create(scrLock);
  lv_obj_set_pos(lockBatPill, 116, 4);
  lv_obj_set_size(lockBatPill, 7, 11);
  lv_obj_set_style_radius(lockBatPill, 2, 0);
  lv_obj_set_style_bg_color(lockBatPill, lv_color_hex(0x22B14C), 0);
  lv_obj_set_style_bg_opa(lockBatPill, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(lockBatPill, 1, 0);
  lv_obj_set_style_border_color(lockBatPill, lv_color_hex(0xE8E8E8), 0);
  lv_obj_set_style_pad_all(lockBatPill, 0, 0);
  lv_obj_clear_flag(lockBatPill, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(lockBatPill, LV_OBJ_FLAG_SCROLLABLE);

  // diamante azul (ico do modo) 11x11 em (76,4)
  static lv_img_dsc_t diaLockDsc;
  diaLockDsc.header.always_zero = 0;
  diaLockDsc.header.w  = 11;
  diaLockDsc.header.h  = 11;
  diaLockDsc.header.cf = LV_IMG_CF_TRUE_COLOR;
  diaLockDsc.data_size = sizeof(face_diamond_565);
  diaLockDsc.data      = (const uint8_t *)face_diamond_565;
  lockDiamond = lv_img_create(scrLock);
  lv_img_set_src(lockDiamond, &diaLockDsc);
  lv_obj_set_pos(lockDiamond, 76, 4);
  lv_obj_clear_flag(lockDiamond, LV_OBJ_FLAG_CLICKABLE);

  // 3 aneis de atividade: arte estatica pixel a pixel (print)
  static lv_img_dsc_t ringsDsc;
  ringsDsc.header.always_zero = 0;
  ringsDsc.header.w  = FACE_RINGS_W;
  ringsDsc.header.h  = FACE_RINGS_H;
  ringsDsc.header.cf = LV_IMG_CF_TRUE_COLOR;
  ringsDsc.data_size = sizeof(face_rings_565);
  ringsDsc.data      = (const uint8_t *)face_rings_565;
  lockRingsImg = lv_img_create(scrLock);
  lv_img_set_src(lockRingsImg, &ringsDsc);
  lv_obj_set_pos(lockRingsImg, 9, 85);
  lv_obj_clear_flag(lockRingsImg, LV_OBJ_FLAG_CLICKABLE);

  // aneis interativos do LVGL: continuam existindo (face digital usa) e ficam
  // escondidos no padrao
  lockRing  = lockArc(scrLock, 3, 102, 54, 10000, 0xCCFF00, 5, (int)cfg.steps, 0);
  lockRingK = lockArc(scrLock, 8, 107, 44, 70,    0x00E676, 4, (int)(cfg.steps * 0.007f), 1);
  lockRingC = lockArc(scrLock, 13, 112, 34, 400,  0xFF3B5C, 4, (int)(cfg.steps * 0.04f), 2);
  gRingActive = false;

  // valor do anel sendo ajustado (aparece so enquanto gira)
  lockRingVal = uiLabel(scrLock, "", 0, 86, F_SM, lv_color_to32(REDMI_TXT));
  lv_obj_set_width(lockRingVal, 58);
  lv_obj_set_style_text_align(lockRingVal, LV_TEXT_ALIGN_LEFT, 0);
  lv_obj_add_flag(lockRingVal, LV_OBJ_FLAG_HIDDEN);

  lockSteps = uiLabel(scrLock, "", 0, 98, F_UI, 0x8B90A0);
  lv_obj_set_width(lockSteps, 128);
  lv_obj_set_style_text_align(lockSteps, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_add_flag(lockSteps, LV_OBJ_FLAG_HIDDEN);

  lockDate = uiLabel(scrLock, "", 0, 146, F_UI, lv_color_to32(REDMI_DIM));
  lv_obj_set_width(lockDate, 128);
  lv_obj_set_style_text_align(lockDate, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_add_flag(lockDate, LV_OBJ_FLAG_HIDDEN);

  // ----- face analogica (ponteiros) -----
  lockDial = lv_obj_create(scrLock);
  lv_obj_set_pos(lockDial, 64 - 36, 74 - 36);
  lv_obj_set_size(lockDial, 72, 72);
  lv_obj_set_style_radius(lockDial, 40, 0);
  lv_obj_set_style_bg_opa(lockDial, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_color(lockDial, lv_color_hex(0xC9CED9), 0);
  lv_obj_set_style_border_width(lockDial, 2, 0);
  lv_obj_clear_flag(lockDial, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(lockDial, LV_OBJ_FLAG_SCROLLABLE);
  for (int i = 0; i < 3; i++) {
    lockHand[i] = lv_line_create(scrLock);
    lv_obj_set_style_line_width(lockHand[i], i == 0 ? 4 : (i == 1 ? 2 : 1), 0);
    lv_obj_set_style_line_color(lockHand[i], i == 2 ? lv_color_hex(0xFF5C5C) : lv_color_hex(0xF4F5F7), 0);
    lv_obj_set_style_line_rounded(lockHand[i], true, 0);
    lv_obj_clear_flag(lockHand[i], LV_OBJ_FLAG_CLICKABLE);
  }
  lockCenter = lv_obj_create(scrLock);
  lv_obj_set_pos(lockCenter, 64 - 3, 74 - 3);
  lv_obj_set_size(lockCenter, 6, 6);
  lv_obj_set_style_radius(lockCenter, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(lockCenter, lv_color_hex(0xFF5C5C), 0);
  lv_obj_set_style_border_opa(lockCenter, 0, 0);
  lv_obj_clear_flag(lockCenter, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(lockCenter, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_t *hint = uiLabel(scrLock, "toque p/ destravar", 0, 146, F_UI, 0x676C7A);
  lockHint = hint;
  lv_obj_set_width(hint, 128);
  lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_add_flag(hint, LV_OBJ_FLAG_HIDDEN);          // foto 2: tela limpa; aparece so no guard
  // espelha cliques e gestos dos filhos (labels grandes da LVGL v8 engolem
  // o toque se nao tiver bubble) -> gestos/duplo-toques chegam na TELA
  uint32_t nC = lv_obj_get_child_cnt(scrLock);
  for (uint32_t i = 0; i < nC; i++) {
    lv_obj_t *c = lv_obj_get_child(scrLock, i);
    if (c == lockRing || c == lockRingK || c == lockRingC) {
      lv_obj_add_flag(c, LV_OBJ_FLAG_GESTURE_BUBBLE);   // anel continua arrastavel
      continue;
    }
    lv_obj_clear_flag(c, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(c, LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_EVENT_BUBBLE);
  }
  // transicao suave ao entrar/trocar de face
  if (lv_obj_is_valid(lockTime)) animFadeIn(lockTime, 350);
  if (lv_obj_is_valid(lockRing)) animFadeIn(lockRing, 400);
  if (lv_obj_is_valid(lockRingK)) animFadeIn(lockRingK, 450);
  if (lv_obj_is_valid(lockRingC)) animFadeIn(lockRingC, 500);
  if (lv_obj_is_valid(lockBatPct)) animFadeIn(lockBatPct, 300);
  if (lv_obj_is_valid(lockBatIco)) animFadeIn(lockBatIco, 300);
  if (lv_obj_is_valid(lockWifi)) animFadeIn(lockWifi, 300);
  // duplo toque (timing manual; LV_EVENT_DOUBLE_TAP nao existe na v8)
  lv_obj_add_event_cb(scrLock, [](lv_event_t *) {
    uint32_t now = millis();
    if (now - lockLastClickMs < 500) {
      lockLastClickMs = 0;
      lockUnlock();
    } else lockLastClickMs = now;
  }, LV_EVENT_CLICKED, NULL);
  lockUpdate();     // aplica ja o estilo atual (evita ponteiros analagicos no boot)
}

static void drawBatIco(lv_obj_t *cv, int p, uint32_t batColor = 0x00FF66) {   // icone bateria 9x9 no canvas (Redmi Watch 5)
  p = constrain(p, 0, 100);
  lv_color_t frame = lv_color_hex(batColor);   // borda/haste: verde (ou cor passada)
  lv_color_t fill  = p < 20 ? REDMI_ERR : REDMI_OK;   // enchimento: vermelho <20%, verde >=20%
  lv_color_t bg    = lv_color_hex(0x000000);
  for (int y = 0; y < 9; y++) {
    for (int x = 0; x < 9; x++) {
      bool on = false;
      if (x == 8 && y >= 2 && y <= 6) on = true;                          // haste direita (batelha)
      else if (x >= 1 && x <= 7 && y >= 0 && y <= 7 &&
               (x == 1 || x == 7 || y == 0 || y == 7)) on = true;          // borda do corpo
      lv_canvas_set_px(cv, x, y, on ? frame : bg);
    }
  }
  // preenchimento proporcional dentro do corpo (x 2..6, y 1..6)
  int fw = (int)((5 * p) / 100.0f);   // 0..5 colunas preenchidas
  for (int y = 1; y <= 6; y++)
    for (int x = 2; x <= 6; x++)
      lv_canvas_set_px(cv, x, y, ((x - 2) < fw) ? fill : bg);
}

// ---------------- icone do tempo 12x12 (sol / nuvem / chuva) ----------------
static void drawWxIco(lv_obj_t *cv, int kind) {
  static uint32_t buf[WX_SZ * WX_SZ];
  for (int i = 0; i < WX_SZ * WX_SZ; i++) buf[i] = 0x000000;          // fundo preto
  auto px = [&](int x, int y, uint32_t c) { if (x>=0&&y>=0&&x<WX_SZ&&y<WX_SZ) buf[y*WX_SZ+x]=c; };
  auto disc = [&](int cx,int cy,int r,uint32_t c){ for(int y=-r;y<=r;y++)for(int x=-r;x<=r;x++) if(x*x+y*y<=r*r) px(cx+x,cy+y,c); };
  if (kind == 0) {                                                 // SOL
    disc(5,5,3,0xFFD54A);
    disc(5,5,2,0xFFE082);
    px(5,1,0xFFD54A); px(5,9,0xFFD54A); px(1,5,0xFFD54A); px(9,5,0xFFD54A);
    px(2,2,0xFFD54A); px(8,2,0xFFD54A); px(2,8,0xFFD54A); px(8,8,0xFFD54A);
  } else {                                                         // NUVEM / CHUVA
    disc(4,6,2,0xB0BEC5); disc(7,5,3,0xB0BEC5);
    for (int y=6;y<9;y++) for (int x=2;x<10;x++) px(x,y,0xB0BEC5);
    for (int y=5;y<7;y++) for (int x=4;x<10;x++) px(x,y,0xB0BEC5);
    if (kind == 2) { px(3,10,0x4FC3F7); px(4,11,0x4FC3F7); px(6,10,0x4FC3F7); px(7,11,0x4FC3F7); px(9,10,0x4FC3F7); px(10,11,0x4FC3F7); }
  }
  for (int y = 0; y < WX_SZ; y++)
    for (int x = 0; x < WX_SZ; x++)
      lv_canvas_set_px(cv, x, y, lv_color_hex(buf[y*WX_SZ+x]));
}

static void lockSpaceUpdate() {          // tema espacial: cicla Sol/Terra/Lua + fase
  if (!lockPlanet || !lv_obj_is_valid(lockPlanet)) return;
  uint32_t t = millis();
  int body = (t / 12000) % 3;            // 0=sol 1=terra 2=lua
  if (body == 0) {                       // SOL: halo pulsa; sem sombra/continentes
    lv_obj_set_style_bg_color(lockPlanet, lv_color_hex(0xFFD54A), 0);
    lv_obj_set_style_bg_opa(lockGlow, LV_OPA_30 + (t / 90) % 100, 0);
    if (lv_obj_is_valid(lockCont)) lv_obj_add_flag(lockCont, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(lockShade, LV_OBJ_FLAG_HIDDEN);
  } else if (body == 1) {                // TERRA: azul, continentes e terminador
    lv_obj_set_style_bg_color(lockPlanet, lv_color_hex(0x2F86D4), 0);
    lv_obj_set_style_bg_opa(lockGlow, LV_OPA_30, 0);
    if (lv_obj_is_valid(lockCont)) lv_obj_clear_flag(lockCont, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(lockShade, LV_OBJ_FLAG_HIDDEN);
  } else {                               // LUA: cinza, sem continentes, com fases
    lv_obj_set_style_bg_color(lockPlanet, lv_color_hex(0xBCBFC9), 0);
    lv_obj_set_style_bg_opa(lockGlow, LV_OPA_30, 0);
    if (lv_obj_is_valid(lockCont)) lv_obj_add_flag(lockCont, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(lockShade, LV_OBJ_FLAG_HIDDEN);
  }
  // fase (sombra varre o disco); disco cheio -> sombra fora da tela
  int off = (int)((t % 16000) / 500) - 16;      // -16..15
  lv_obj_set_pos(lockShade, 46 + off * 3, 98);
  if (abs(off) >= 12) lv_obj_set_pos(lockShade, 2000, 98);
  for (int i = 0; i < 8; i++)
    if (lockStars[i] && lv_obj_is_valid(lockStars[i]))
      lv_obj_set_style_bg_opa(lockStars[i], ((t / 33) + i * 7) % 9 ? LV_OPA_60 : LV_OPA_20, 0);
}
static void lockSetHand(lv_obj_t *ln, int cx, int cy, float angDeg, int len) {
  float a = angDeg * 3.14159f / 180.0f;
  lv_point_t p[2] = { { (int16_t)cx, (int16_t)cy },
                      { (int16_t)(cx + len * sinf(a)), (int16_t)(cy - len * cosf(a)) } };
  lv_line_set_points(ln, p, 2);
}
static void lockShowAnalog(bool on) {
  if (lv_obj_is_valid(lockDial))   { on ? lv_obj_clear_flag(lockDial,   LV_OBJ_FLAG_HIDDEN) : lv_obj_add_flag(lockDial,   LV_OBJ_FLAG_HIDDEN); }
  for (int i = 0; i < 3; i++) if (lv_obj_is_valid(lockHand[i])) { on ? lv_obj_clear_flag(lockHand[i], LV_OBJ_FLAG_HIDDEN) : lv_obj_add_flag(lockHand[i], LV_OBJ_FLAG_HIDDEN); }
  if (lv_obj_is_valid(lockCenter)) { on ? lv_obj_clear_flag(lockCenter, LV_OBJ_FLAG_HIDDEN) : lv_obj_add_flag(lockCenter, LV_OBJ_FLAG_HIDDEN); }
}
static void lockUpdate() {
  time_t now = time(nullptr);
  char hh[8] = "--:--", dd[24] = "", ss[10] = "", stp[20] = "";
  int H = 0, M = 0, S = 0;
  bool tok = timeOk();
  if (tok) {
    struct tm *l = localtime(&now);
    strftime(hh, sizeof(hh), "%H:%M", l);
    strftime(dd, sizeof(dd), "%a %d/%m", l);          // dia da semana + data
    H = l->tm_hour; M = l->tm_min; S = l->tm_sec;
  }
  snprintf(stp, sizeof(stp), "passos %lu", (unsigned long)cfg.steps);

  // elementos exclusivos da face padrao (o ramo dela reexibe)
  if (lv_obj_is_valid(lockRingsImg)) lv_obj_add_flag(lockRingsImg, LV_OBJ_FLAG_HIDDEN);
  if (lv_obj_is_valid(lockBatPill))  lv_obj_add_flag(lockBatPill,  LV_OBJ_FLAG_HIDDEN);
  if (lv_obj_is_valid(lockDiamond))  lv_obj_add_flag(lockDiamond,  LV_OBJ_FLAG_HIDDEN);

  // ---------- DIGITAL (0): hora grande + passos + clima + data ----------
  // ---------- DIGITAL (0): hora grande central + data/passos/clima ----------
  if (cfg.lockStyle == 0) {
    lockShowAnalog(false);
    if (lv_obj_is_valid(lockTime)) {
      lv_obj_clear_flag(lockTime, LV_OBJ_FLAG_HIDDEN);
      lv_obj_set_style_text_font(lockTime, F_XL, 0);
      lv_obj_set_width(lockTime, 128);
      lv_obj_set_style_text_align(lockTime, LV_TEXT_ALIGN_CENTER, 0);
      lv_obj_set_pos(lockTime, 0, 32);
      lv_label_set_text(lockTime, hh);
    }
    if (lv_obj_is_valid(lockSecs)) lv_obj_add_flag(lockSecs, LV_OBJ_FLAG_HIDDEN);
    if (lv_obj_is_valid(lockDate)) {
      lv_obj_clear_flag(lockDate, LV_OBJ_FLAG_HIDDEN);
      lv_obj_set_style_text_font(lockDate, F_UI, 0);
      lv_obj_set_pos(lockDate, 0, 72);
      lv_obj_set_width(lockDate, 128);
      lv_obj_set_style_text_align(lockDate, LV_TEXT_ALIGN_CENTER, 0);
      lv_label_set_text(lockDate, tok ? dd : "sem hora");
    }
    if (lv_obj_is_valid(lockSteps)) {
      lv_obj_clear_flag(lockSteps, LV_OBJ_FLAG_HIDDEN);
      lv_obj_set_pos(lockSteps, 0, 98);
      lv_obj_set_width(lockSteps, 128);
      lv_obj_set_style_text_align(lockSteps, LV_TEXT_ALIGN_CENTER, 0);
      lv_label_set_text(lockSteps, tok ? stp : "passos --");
    }
    if (lv_obj_is_valid(lockWx)) {
      if (gWxTxt.length()) {
        lv_obj_clear_flag(lockWx, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_pos(lockWx, 0, 118);
        lv_obj_set_width(lockWx, 128);
        lv_obj_set_style_text_align(lockWx, LV_TEXT_ALIGN_CENTER, 0);
        lv_label_set_text(lockWx, gWxTxt.c_str());
      } else lv_obj_add_flag(lockWx, LV_OBJ_FLAG_HIDDEN);
    }
    // a face digital usa a barra de status no topo: o bloco do clima fica so na linha de baixo
    if (lv_obj_is_valid(lockWxIco)) lv_obj_add_flag(lockWxIco, LV_OBJ_FLAG_HIDDEN);
    if (lv_obj_is_valid(lockWxDesc)) lv_obj_add_flag(lockWxDesc, LV_OBJ_FLAG_HIDDEN);
    // mini anel de atividade (passos) no canto sup. direito - tambem rotativo
    if (lv_obj_is_valid(lockRing)) {
      lv_obj_clear_flag(lockRing, LV_OBJ_FLAG_HIDDEN);
      lv_obj_set_pos(lockRing, 96, 44);
      lv_obj_set_size(lockRing, 28, 28);
      lv_obj_set_style_arc_width(lockRing, 4, LV_PART_MAIN);
      lv_obj_set_style_arc_width(lockRing, 4, LV_PART_INDICATOR);
      lv_arc_set_range(lockRing, 0, 10000);
      lv_arc_set_value(lockRing, (int16_t)constrain((long)cfg.steps, 0, 10000));
    }
    if (lv_obj_is_valid(lockRingK)) lv_obj_add_flag(lockRingK, LV_OBJ_FLAG_HIDDEN);
    if (lv_obj_is_valid(lockRingC)) lv_obj_add_flag(lockRingC, LV_OBJ_FLAG_HIDDEN);
    if (lv_obj_is_valid(lockRingVal)) lv_obj_add_flag(lockRingVal, LV_OBJ_FLAG_HIDDEN);
  }
  // ---------- ANALOGICO (1): mostrador + anel de passos ----------
  else if (cfg.lockStyle == 1) {
    lockShowAnalog(true);
    if (lv_obj_is_valid(lockTime)) lv_obj_add_flag(lockTime, LV_OBJ_FLAG_HIDDEN);
    if (lv_obj_is_valid(lockDial)) {
      lv_obj_set_pos(lockDial, 20, 40);
      lv_obj_set_size(lockDial, 88, 88);
      lv_obj_set_style_radius(lockDial, LV_RADIUS_CIRCLE, 0);
    }
    int cx = 64, cy = 84;
    if (tok) {
      float aH = ((H % 12) + M / 60.0f) / 12.0f * 360.0f;
      float aM = (M + S / 60.0f) / 60.0f * 360.0f;
      float aS = S / 60.0f * 360.0f;
      lockSetHand(lockHand[0], cx, cy, aH, 24);   // hora
      lockSetHand(lockHand[1], cx, cy, aM, 32);   // minuto
      lockSetHand(lockHand[2], cx, cy, aS, 30);   // segundo
    }
    if (lv_obj_is_valid(lockCenter)) lv_obj_set_pos(lockCenter, cx - 3, cy - 3);
    if (lv_obj_is_valid(lockSecs)) {
      lv_obj_clear_flag(lockSecs, LV_OBJ_FLAG_HIDDEN);
      lv_obj_set_style_text_font(lockSecs, F_MED, 0);
      lv_obj_set_width(lockSecs, 128);
      lv_obj_set_style_text_align(lockSecs, LV_TEXT_ALIGN_CENTER, 0);
      lv_obj_set_pos(lockSecs, 0, 134);
      lv_label_set_text(lockSecs, hh);
    }
    if (lv_obj_is_valid(lockDate)) {
      lv_obj_clear_flag(lockDate, LV_OBJ_FLAG_HIDDEN);
      lv_obj_set_style_text_font(lockDate, F_SM, 0);
      lv_obj_set_pos(lockDate, 0, 18);
      lv_obj_set_width(lockDate, 128);
      lv_obj_set_style_text_align(lockDate, LV_TEXT_ALIGN_CENTER, 0);
      lv_label_set_text(lockDate, tok ? dd : "");
    }
    if (lv_obj_is_valid(lockSteps)) lv_obj_add_flag(lockSteps, LV_OBJ_FLAG_HIDDEN);
    if (lv_obj_is_valid(lockWx))    lv_obj_add_flag(lockWx,    LV_OBJ_FLAG_HIDDEN);
    if (lv_obj_is_valid(lockRing)) {
      lv_obj_clear_flag(lockRing, LV_OBJ_FLAG_HIDDEN);
      lv_obj_set_pos(lockRing, 16, 36);
      lv_obj_set_size(lockRing, 96, 96);
      lv_obj_set_style_arc_width(lockRing, 3, LV_PART_MAIN);
      lv_obj_set_style_arc_width(lockRing, 3, LV_PART_INDICATOR);
      lv_arc_set_range(lockRing, 0, 10000);
      lv_arc_set_value(lockRing, (int16_t)constrain((long)cfg.steps, 0, 10000));
    }
    if (lv_obj_is_valid(lockRingK)) lv_obj_add_flag(lockRingK, LV_OBJ_FLAG_HIDDEN);
    if (lv_obj_is_valid(lockRingC)) lv_obj_add_flag(lockRingC, LV_OBJ_FLAG_HIDDEN);
    if (lv_obj_is_valid(lockRingVal)) lv_obj_add_flag(lockRingVal, LV_OBJ_FLAG_HIDDEN);
  }
  // ---------- PADRAO (2): Redmi Watch 6 - identico a foto ----------
  else {
    lockShowAnalog(false);
    int p = batPct();
    char bb[8]; snprintf(bb, sizeof(bb), "%d%%", p);
    // topo-direito: "%" verde (vermelho <20%) + pilula da bateria
    if (lv_obj_is_valid(lockBatPct)) {
      lv_obj_clear_flag(lockBatPct, LV_OBJ_FLAG_HIDDEN);
      lv_label_set_text(lockBatPct, bb);
      lv_obj_set_style_text_color(lockBatPct, p < 20 ? lv_color_hex(0xFF3B30) : lv_color_hex(0x22B14C), 0);
      lv_obj_set_pos(lockBatPct, 82, 2);
    }
    if (lv_obj_is_valid(lockBatPill)) {
      lv_obj_clear_flag(lockBatPill, LV_OBJ_FLAG_HIDDEN);
      lv_obj_set_style_bg_color(lockBatPill, p < 20 ? lv_color_hex(0xFF3B30) : lv_color_hex(0x22B14C), 0);
    }
    if (lv_obj_is_valid(lockDiamond)) lv_obj_clear_flag(lockDiamond, LV_OBJ_FLAG_HIDDEN);
    if (lv_obj_is_valid(lockRingsImg)) lv_obj_clear_flag(lockRingsImg, LV_OBJ_FLAG_HIDDEN);

    // "12" em cima e "59" embaixo a direita: dois labels F_HUGE separados
    char hh2[4] = "--", mm2[4] = "--";
    if (tok) { snprintf(hh2, sizeof(hh2), "%02d", H); snprintf(mm2, sizeof(mm2), "%02d", M); }
    if (lv_obj_is_valid(lockTime)) {
      lv_obj_clear_flag(lockTime, LV_OBJ_FLAG_HIDDEN);
      lv_obj_set_style_text_font(lockTime, F_HUGE, 0);
      lv_obj_set_width(lockTime, 60);
      lv_obj_set_style_text_align(lockTime, LV_TEXT_ALIGN_LEFT, 0);
      lv_obj_set_pos(lockTime, 30, 29);
      lv_label_set_text(lockTime, hh2);
    }
    if (lv_obj_is_valid(lockSecs)) {
      lv_obj_clear_flag(lockSecs, LV_OBJ_FLAG_HIDDEN);
      lv_obj_set_style_text_font(lockSecs, F_HUGE, 0);
      lv_obj_set_width(lockSecs, 60);
      lv_obj_set_style_text_align(lockSecs, LV_TEXT_ALIGN_LEFT, 0);
      lv_obj_set_pos(lockSecs, 58, 79);
      lv_label_set_text(lockSecs, mm2);
    }

    // aneis do LVGL, data, clima, passo e BT: nunca no padrao (a foto nao tem)
    if (lv_obj_is_valid(lockRing))    lv_obj_add_flag(lockRing,    LV_OBJ_FLAG_HIDDEN);
    if (lv_obj_is_valid(lockRingK))   lv_obj_add_flag(lockRingK,   LV_OBJ_FLAG_HIDDEN);
    if (lv_obj_is_valid(lockRingC))   lv_obj_add_flag(lockRingC,   LV_OBJ_FLAG_HIDDEN);
    if (lv_obj_is_valid(lockRingVal)) lv_obj_add_flag(lockRingVal, LV_OBJ_FLAG_HIDDEN);
    if (lv_obj_is_valid(lockDate))  lv_obj_add_flag(lockDate,  LV_OBJ_FLAG_HIDDEN);
    if (lv_obj_is_valid(lockWx))    lv_obj_add_flag(lockWx,    LV_OBJ_FLAG_HIDDEN);
    if (lv_obj_is_valid(lockWxIco)) lv_obj_add_flag(lockWxIco, LV_OBJ_FLAG_HIDDEN);
    if (lv_obj_is_valid(lockWxDesc))lv_obj_add_flag(lockWxDesc,LV_OBJ_FLAG_HIDDEN);
    if (lv_obj_is_valid(lockSteps)) lv_obj_add_flag(lockSteps, LV_OBJ_FLAG_HIDDEN);
    if (lv_obj_is_valid(lockBt))    lv_obj_add_flag(lockBt,    LV_OBJ_FLAG_HIDDEN);
    if (lv_obj_is_valid(lockWifi))  lv_obj_add_flag(lockWifi,  LV_OBJ_FLAG_HIDDEN);
  }
  if (cfg.esp) lockSpaceUpdate();
}

// ============================================================
//  HOME (grid de apps)
// ============================================================
static lv_obj_t *appChip(lv_obj_t *p, const char *glyph, lv_color_t color, int id) {
  // chip circular preenchido (estilo Redmi Watch / Mi Band): icone branco no centro
  lv_obj_t *c = lv_obj_create(p);
  lv_obj_set_size(c, 38, 38);
  lv_obj_set_style_bg_color(c, color, 0);
  lv_obj_set_style_radius(c, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_border_width(c, 1, 0);
  lv_obj_set_style_border_color(c, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_border_opa(c, LV_OPA_20, 0);
  lv_obj_set_style_shadow_width(c, 8, 0);
  lv_obj_set_style_shadow_color(c, color, 0);
  lv_obj_set_style_shadow_opa(c, LV_OPA_40, 0);
  lv_obj_set_style_pad_all(c, 0, 0);
  lv_obj_clear_flag(c, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(c, appCardCb, LV_EVENT_CLICKED, (void *)(intptr_t)id);
  lv_obj_t *ic = lv_label_create(c);
  lv_label_set_text(ic, glyph);
  lv_obj_set_style_text_color(ic, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_font(ic, F_MED, 0);
  lv_obj_center(ic);
  lv_obj_clear_flag(ic, LV_OBJ_FLAG_CLICKABLE);
  return c;
}
// area de toque invisivel sobre um icone da arte (posicao exata do print)
static void homeHit(int id, int x, int y, int w, int h) {
  lv_obj_t *b = lv_obj_create(scrHome);
  lv_obj_set_pos(b, x, y);
  lv_obj_set_size(b, w, h);
  lv_obj_set_style_bg_opa(b, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(b, 0, 0);
  lv_obj_set_style_radius(b, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_shadow_width(b, 0, 0);
  lv_obj_set_style_pad_all(b, 0, 0);
  lv_obj_clear_flag(b, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(b, appCardCb, LV_EVENT_CLICKED, (void *)(intptr_t)id);
}

void buildHomeS() {
  scrHome = lv_obj_create(NULL);
  scrStyle(scrHome);
  lv_obj_clear_flag(scrHome, LV_OBJ_FLAG_SCROLLABLE);

  // faixa do header com a MESMA cor de fundo do print (17,2,18 = 0x1002)
  lv_obj_t *hdr = lv_obj_create(scrHome);
  lv_obj_set_pos(hdr, 0, 0);
  lv_obj_set_size(hdr, 128, 18);
  lv_obj_set_style_bg_color(hdr, lv_color_hex(0x110212), 0);
  lv_obj_set_style_bg_opa(hdr, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(hdr, 0, 0);
  lv_obj_set_style_pad_all(hdr, 0, 0);
  lv_obj_set_style_radius(hdr, 0, 0);
  lv_obj_clear_flag(hdr, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);

  // ---- arte pixel a pixel do print (y=18..159, 128x142) ----
  static lv_img_dsc_t artDsc;
  artDsc.header.always_zero = 0;
  artDsc.header.w  = HOME_ART_W;
  artDsc.header.h  = HOME_ART_H;
  artDsc.header.cf = LV_IMG_CF_TRUE_COLOR;
  artDsc.data_size = sizeof(home_art_565);
  artDsc.data      = (const uint8_t *)home_art_565;
  lv_obj_t *art = lv_img_create(scrHome);
  lv_img_set_src(art, &artDsc);
  lv_obj_set_pos(art, 0, 18);
  lv_obj_clear_flag(art, LV_OBJ_FLAG_CLICKABLE);

  // ---- header dinamico (mesmas posicoes/medidas do print) ----
  homeTime = uiLabel(scrHome, "--:--", 5, 5, F_UI, 0xFFFFFF);   // "12:59" ink x5..37 y8..16
  lv_label_set_long_mode(homeTime, LV_LABEL_LONG_CLIP);
  lv_obj_set_width(homeTime, 44);
  lv_obj_clear_flag(homeTime, LV_OBJ_FLAG_CLICKABLE);

  static lv_img_dsc_t diaDsc;                                   // diamante 13x13 em (74,5)
  diaDsc.header.always_zero = 0;
  diaDsc.header.w  = 13;
  diaDsc.header.h  = 13;
  diaDsc.header.cf = LV_IMG_CF_TRUE_COLOR;
  diaDsc.data_size = sizeof(home_diamond_565);
  diaDsc.data      = (const uint8_t *)home_diamond_565;
  lv_obj_t *dia = lv_img_create(scrHome);
  lv_img_set_src(dia, &diaDsc);
  lv_obj_set_pos(dia, 74, 5);
  lv_obj_clear_flag(dia, LV_OBJ_FLAG_CLICKABLE);

  homePct = uiLabel(scrHome, "100%", 85, 5, F_SM, 0xFFFFFF);    // ink termina em x=116
  lv_label_set_long_mode(homePct, LV_LABEL_LONG_CLIP);
  lv_obj_set_width(homePct, 32);
  lv_obj_set_style_text_align(homePct, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_clear_flag(homePct, LV_OBJ_FLAG_CLICKABLE);

  homeBat = lv_obj_create(scrHome);                             // pilula 6x12 em (121,6)
  lv_obj_set_pos(homeBat, 121, 6);
  lv_obj_set_size(homeBat, 6, 12);
  lv_obj_set_style_radius(homeBat, 2, 0);
  lv_obj_set_style_bg_color(homeBat, lv_color_hex(0x22B14C), 0);
  lv_obj_set_style_bg_opa(homeBat, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(homeBat, 0, 0);
  lv_obj_set_style_pad_all(homeBat, 0, 0);
  lv_obj_clear_flag(homeBat, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(homeBat, LV_OBJ_FLAG_SCROLLABLE);

  // ---- 9 areas de toque sobre os icones (ordem 1..9 do print) ----
  homeHit(9, 0,  31, 37, 38);   // 1 Power (desliga a tela)
  homeHit(2, 41, 30, 38, 38);   // 2 Matematica
  homeHit(3, 85, 33, 38, 38);   // 3 Google
  homeHit(5, 1,  74, 38, 38);   // 4 Jogos
  homeHit(6, 42, 76, 38, 38);   // 5 Saude
  homeHit(4, 85, 77, 38, 38);   // 6 IA
  homeHit(7, 1, 118, 38, 38);   // 7 Arquivos
  homeHit(1, 43, 118, 38, 38);  // 8 Toolbox
  homeHit(0, 85, 119, 38, 38);  // 9 Configuracoes
}

void appCardCb(lv_event_t *e) {
  int id = (int)(intptr_t)lv_event_get_user_data(e);
  beepShort();
  switch (id) {
    case 0: buildConfig();  nav(scrConfig, LV_SCR_LOAD_ANIM_MOVE_LEFT); break;
    case 1: buildToolbox(); nav(scrToolbox, LV_SCR_LOAD_ANIM_MOVE_LEFT); break;
    case 2: buildMat();     nav(scrMat,     LV_SCR_LOAD_ANIM_MOVE_LEFT); break;
    case 3: buildGoogle();  nav(scrGoogle,  LV_SCR_LOAD_ANIM_MOVE_LEFT); break;
    case 4: buildIA();      nav(scrIA,      LV_SCR_LOAD_ANIM_MOVE_LEFT); break;
    case 5: buildJogos();   nav(scrJogos,   LV_SCR_LOAD_ANIM_MOVE_LEFT); break;
    case 6: buildSaude();   nav(scrSaude,   LV_SCR_LOAD_ANIM_MOVE_LEFT); break;
    case 7: buildArquivo(); nav(scrArquivo, LV_SCR_LOAD_ANIM_MOVE_LEFT); break;
    case 8: lockUpdate();   navP(scrLock, LV_SCR_LOAD_ANIM_OVER_TOP, true); break;
    case 9: gWantLock = true;          // Power Off: tela escura, volta com 2 toques
            blOff(); wakeArmDark(false); break;
    case 10: buildAjustes(); nav(scrAjustes, LV_SCR_LOAD_ANIM_MOVE_LEFT); break;
  }
}

// ============================================================
//  CONFIG (menu principal) + RESET DE FABRICA
// ============================================================
static void cfWifiCb(lv_event_t *)  { buildWifi();     nav(scrWifi,     LV_SCR_LOAD_ANIM_MOVE_LEFT); }
static void cfTelaCb(lv_event_t *)  { buildTela();     nav(scrTela,     LV_SCR_LOAD_ANIM_MOVE_LEFT); }
static void cfAjCb(lv_event_t *)    { buildAjustes();  nav(scrAjustes,  LV_SCR_LOAD_ANIM_MOVE_LEFT); }
static void cfDhCb(lv_event_t *)    { buildDataHora(); nav(scrDataHora, LV_SCR_LOAD_ANIM_MOVE_LEFT); }
static void cfTesteCb(lv_event_t *) { buildTeste();    nav(scrTeste,    LV_SCR_LOAD_ANIM_MOVE_LEFT); }
static void cfInfoCb(lv_event_t *)  { buildInfo();     nav(scrInfo,     LV_SCR_LOAD_ANIM_MOVE_LEFT); }
static void cfResetCb(lv_event_t *) { buildCfgReset(); nav(scrCfgReset, LV_SCR_LOAD_ANIM_MOVE_LEFT); }
static void cfClimaCb(lv_event_t *) { buildClima(); nav(scrClima, LV_SCR_LOAD_ANIM_MOVE_LEFT); }
static void cfMatCb(lv_event_t *)   { buildMat();   nav(scrMat,   LV_SCR_LOAD_ANIM_MOVE_LEFT); }
static void cfSaudeCb(lv_event_t *) { buildSaude(); nav(scrSaude, LV_SCR_LOAD_ANIM_MOVE_LEFT); }
static lv_obj_t *cfgBtn(lv_obj_t *p, const char *txt, lv_event_cb_t cb, int col, int row,
                        uint32_t colr = 0x161A22, uint32_t txtc = 0xD9DBE2, int yOff = 0) {
  lv_obj_t *b = lv_btn_create(p);
  lv_obj_set_pos(b, col * 60, yOff + row * 25);
  lv_obj_set_size(b, 58, 22);
  lv_obj_set_style_bg_color(b, lv_color_hex(colr), 0);
  lv_obj_set_style_radius(b, 7, 0);
  lv_obj_set_style_shadow_width(b, 0, 0);
  lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(b, clickBeep, LV_EVENT_CLICKED, NULL);   // bip nos atalhos do menu
  lv_obj_t *l = lv_label_create(b);
  lv_label_set_text(l, txt);
  lv_obj_set_style_text_font(l, F_UI, 0);
  lv_obj_set_style_text_color(l, lv_color_hex(txtc), 0);
  lv_obj_center(l);
  return b;
}
void buildConfig() {
  scrConfig = lv_obj_create(NULL);
  scrStyle(scrConfig);
  appHeader(scrConfig, "CONFIG");
  lv_obj_t *body = appBody(scrConfig);
  // lista vertical Redmi (8 linhas 18px/step20 -> rola)
  lv_obj_set_scroll_dir(body, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(body, LV_SCROLLBAR_MODE_AUTO);
  redRow(body, LV_SYMBOL_WIFI, "Wi-Fi",         cfWifiCb,  0);
  redRow(body, "C",            "Clima",         cfClimaCb, 20);
  redRow(body, LV_SYMBOL_BELL, "Data e hora",   cfDhCb,    40);
  redRow(body, LV_SYMBOL_IMAGE,"Tela",          cfTelaCb,  60);
  redRow(body, LV_SYMBOL_SETTINGS, "Ajustes",    cfAjCb,    80);
  redRow(body, "M",            "Matematica",    cfMatCb,   100);
  redRow(body, LV_SYMBOL_CHARGE,"Saude",        cfSaudeCb, 120);
  redRow(body, "T",            "Toque (teste)", cfTesteCb, 140);
  redRow(body, "i",            "Info",          cfInfoCb,  160);
  redRow(body, LV_SYMBOL_WARNING, "Reset fabrica", cfResetCb, 180, 18, 0x5A1414, 0xF8A0A0);
}
static void cfgResetGoCb(lv_event_t *e) {
  (void)e;                                   // botao so existe na tela de confirmacao
  gLvPause = true;                           // LVGL congela: reinicio imediato
  SPIFFS.format();
  Serial.println("[cfg] reset de fabrica: SPIFFS formatado");
  delay(150);
  ESP.restart();
}
static void cfgResetCancelCb(lv_event_t *) { buildConfig(); nav(scrConfig, LV_SCR_LOAD_ANIM_MOVE_RIGHT); }
void buildCfgReset() {
  scrCfgReset = lv_obj_create(NULL);
  scrStyle(scrCfgReset);
  appHeader(scrCfgReset, "CONFIRMAR");
  lv_obj_t *body = appBody(scrCfgReset);
  lv_obj_t *t = uiLabel(body, "Apaga config, Wi-Fi e calibracao, e reinicia o relogio. Confirmar?", 0, 0, F_UI, 0xF8A0A0);
  lv_obj_set_width(t, 118);
  lv_label_set_long_mode(t, LV_LABEL_LONG_WRAP);
  lv_obj_t *b1 = uiBtn(body, "CANCELAR", cfgResetCancelCb, 56, 22, 0x161A22, NULL);
  lv_obj_set_pos(b1, 0, 80);
  lv_obj_t *b2 = uiBtn(body, "RESET", cfgResetGoCb, 56, 22, 0x5A1414, NULL);
  lv_obj_set_pos(b2, 60, 80);
}

// ============================================================
//  INFO DO SISTEMA
// ============================================================
void buildInfo() {
  scrInfo = lv_obj_create(NULL);
  scrStyle(scrInfo);
  appHeader(scrInfo, "INFO");
  lv_obj_t *body = lv_obj_create(scrInfo);   // corpo rolavel (conteudo curto navega)
  lv_obj_set_pos(body, 0, 46);
  lv_obj_set_size(body, 128, 112);
  lv_obj_set_style_bg_color(body, REDMI_BG, 0);
  lv_obj_set_style_bg_opa(body, LV_OPA_COVER, 0);
  lv_obj_set_style_border_opa(body, 0, 0);
  lv_obj_set_style_pad_all(body, 5, 0);
  lv_obj_set_scrollbar_mode(body, LV_SCROLLBAR_MODE_AUTO);
  char b[40];
  int n = 0;
  auto line = [&](const char *pre, const char *val) {
    lv_obj_t *l = lv_label_create(body);
    snprintf(b, sizeof(b), "%s%s", pre, val ? val : "-");
    lv_label_set_text(l, b);
    lv_obj_set_width(l, 108);
    lv_label_set_long_mode(l, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(l, F_UI, 0);
    lv_obj_set_style_text_color(l, n ? REDMI_DIM : REDMI_TXT, 0);
    lv_obj_set_pos(l, 0, (n++) * 16);
  };
  line("Versao: ", "v2.0 LVGL");
  line("MAC: ", WiFi.macAddress().c_str());
  line("IP: ", WiFi.localIP().toString().c_str());
  snprintf(b, sizeof(b), "%u kB", (unsigned)(ESP.getFreeHeap() / 1024));
  line("Heap: ", b);
  size_t ps = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
  if (ps) snprintf(b, sizeof(b), "%u kB livres", (unsigned)(ps / 1024));
  line("PSRAM: ", ps ? b : "ausente");
  snprintf(b, sizeof(b), "%u kB", (unsigned)(psUsedBytes() / 1024));
  line("Apps PSRAM: ", b);
  uint32_t up = millis() / 1000;
  snprintf(b, sizeof(b), "%02uh%02um%02us", (unsigned)(up / 3600), (unsigned)((up / 60) % 60), (unsigned)(up % 60));
  line("Ligado: ", b);
  snprintf(b, sizeof(b), "%u/%u kB", (unsigned)(SPIFFS.usedBytes() / 1024), (unsigned)(SPIFFS.totalBytes() / 1024));
  line("SPIFFS: ", b);
  snprintf(b, sizeof(b), "%.2fV %d%%", (double)batVoltage(), batPct());
  line("Bateria: ", b);
}

// ============================================================
//  PERSONALIZACAO (cor de destaque, tema espacial, estilo do relogio)
// ============================================================
static void persRefreshLock() {
  lv_obj_t *ol = scrLock;
  buildLockS();
  if (lv_obj_is_valid(ol) && ol != lv_scr_act()) lv_obj_del(ol);
}
static void persChipCb(lv_event_t *e) {
  cfg.accIdx = (uint8_t)(intptr_t)lv_event_get_user_data(e);
  cfgSave();
  lv_obj_t *oh = scrHome;
  buildHomeS();
  if (lv_obj_is_valid(oh) && oh != lv_scr_act()) lv_obj_del(oh);
  navP(scrPers, LV_SCR_LOAD_ANIM_NONE, true);    // reabre a Pers c/ a cor nova
}
static void persEspCb(lv_event_t *e) {
  cfg.esp = lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED);
  cfgSave();
  persRefreshLock();
}
static void persStyleCb(lv_event_t *e) {
  cfg.lockStyle = (uint8_t)(intptr_t)lv_event_get_user_data(e);
  cfgSave();
  persRefreshLock();
  toast("Estilo aplicado");
}
void buildPers() {
  scrPers = lv_obj_create(NULL);
  scrStyle(scrPers);
  appHeader(scrPers, "PERSONALIZAR");
  lv_obj_t *body = appBody(scrPers);       // conteudo 118x112 (pad 5)
  // ---- COR DE DESTAQUE: blobs circulares na cor em si ----
  uiLabel(body, "COR DE DESTAQUE", 0, 0, F_SM, lv_color_to32(REDMI_DIM));
  for (int i = 0; i < 8; i++) {
    lv_obj_t *ch = lv_btn_create(body);
    lv_obj_set_pos(ch, i * 14, 13);
    lv_obj_set_size(ch, 16, 16);
    lv_obj_set_style_bg_color(ch, lv_color_hex(ACC_LIST[i]), 0);
    lv_obj_set_style_radius(ch, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_shadow_width(ch, 0, 0);
    lv_obj_set_style_transition(ch, NULL, 0);
    if (i == cfg.accIdx) {
      lv_obj_set_style_border_color(ch, REDMI_TXT, 0);
      lv_obj_set_style_border_width(ch, 2, 0);
    } else lv_obj_set_style_border_width(ch, 0, 0);
    lv_obj_add_event_cb(ch, persChipCb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
  }
  // ---- ESTILO DO RELOGIO: chips segmentados (selecionado = accent) ----
  uiLabel(body, "ESTILO DO RELOGIO", 0, 31, F_SM, lv_color_to32(REDMI_DIM));
  static const char *stTxt[3] = { "Digital", "Analog", "Redmi" };
  for (int i = 0; i < 3; i++) {
    lv_obj_t *b = lv_btn_create(body);
    lv_obj_set_pos(b, i * 39, 44);
    lv_obj_set_size(b, 37, 18);
    lv_obj_set_style_bg_color(b, i == cfg.lockStyle ? REDMI_ACC : REDMI_CARD, 0);
    lv_obj_set_style_radius(b, 9, 0);
    lv_obj_set_style_shadow_width(b, 0, 0);
    lv_obj_t *l = lv_label_create(b);
    lv_label_set_text(l, stTxt[i]);
    lv_obj_set_style_text_font(l, F_SM, 0);
    lv_obj_set_style_text_color(l, i == cfg.lockStyle ? lv_color_hex(0x0B0D12) : REDMI_TXT, 0);
    lv_obj_center(l);
    lv_obj_add_event_cb(b, persStyleCb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
  }
  // ---- TEMA ESPACIAL: linha inteira com switch na direita ----
  lv_obj_t *rowE = lv_obj_create(body);
  lv_obj_set_pos(rowE, 0, 64);
  lv_obj_set_size(rowE, 118, 18);
  lv_obj_set_style_bg_color(rowE, lv_color_hex(0x1C202B), 0);
  lv_obj_set_style_radius(rowE, 9, 0);
  lv_obj_set_style_border_width(rowE, 0, 0);
  lv_obj_set_style_pad_all(rowE, 0, 0);
  lv_obj_clear_flag(rowE, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(rowE, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_t *le = lv_label_create(rowE);
  lv_label_set_text(le, "Tema espacial");
  lv_obj_set_style_text_font(le, F_UI, 0);
  lv_obj_set_style_text_color(le, REDMI_TXT, 0);
  lv_obj_align(le, LV_ALIGN_LEFT_MID, 6, 0);
  lv_obj_t *sw = lv_switch_create(rowE);
  lv_obj_set_size(sw, 26, 16);
  lv_obj_align(sw, LV_ALIGN_RIGHT_MID, -3, 0);
  if (cfg.esp) lv_obj_add_state(sw, LV_STATE_CHECKED);
  lv_obj_add_event_cb(sw, persEspCb, LV_EVENT_VALUE_CHANGED, NULL);
  // ---- WALLPAPER: 2 botoes na base (59px cada) ----
  lv_obj_t *wB1 = uiBtn(body, "Do SD",      [](lv_event_t *) { wpSetFromSD(); }, 57, 18, 0x10304A, NULL);
  if (wB1) lv_obj_set_pos(wB1, 0, 84);
  lv_obj_t *wB2 = uiBtn(body, "Remover WP", [](lv_event_t *) { wpClear(); },     57, 18, 0x3B1B1B, NULL);
  if (wB2) lv_obj_set_pos(wB2, 61, 84);
}

// ============================================================
//  WI-FI
// ============================================================
static lv_obj_t *wifiSt, *wifiListCont, *wifiModal;
static String gScanSsid[8];
static int gScanRssi[8];
static int gScanN = 0;

static void wifiFillRows();
static void wifiStatusText() {
  if (!lv_obj_is_valid(wifiSt)) return;
  if (WiFi.status() == WL_CONNECTED) {
    String s = "On: " + WiFi.SSID() + " " + WiFi.localIP().toString();
    lv_label_set_text(wifiSt, s.c_str());
  } else {
    lv_label_set_text(wifiSt, "Desconectado");
  }
}
// modal: digitar SSID/senha e salvar/conectar
static void wifiModalDel() {
  kbHideAll();                                // teclado morre junto c/ o modal
  if (wifiModal && lv_obj_is_valid(wifiModal)) lv_obj_del(wifiModal);
  wifiModal = nullptr;
  wmRaise = wmB1 = wmB2 = nullptr;
}
static void wifiConnCb(bool ok, const String &) {
  toast(ok ? "Conectado!" : "Falha ao conectar");
  wifiStatusText();
}
static void wifiConnectSaveCb(lv_event_t *e) {
  wifiModalDel();
  int i = (int)(intptr_t)lv_event_get_user_data(e);
  if (gNetCb || gNetBusy) { toast("Rede ocupada"); return; }
  gNetCb = wifiConnCb;
  netRun(T_TESTW, wifiSsid[i], wifiPass[i]);
  toast("Conectando...");
}
// modal: digitar SSID/senha, salvar e conectar
static lv_obj_t *wmTa1 = nullptr, *wmTa2 = nullptr;
static void wifiAddModalSubmit(lv_event_t *) {
  String s = lv_textarea_get_text(wmTa1);
  String p = lv_textarea_get_text(wmTa2);
  wifiModalDel();
  if (!s.length()) { toast("SSID vazio"); return; }
  wifiAdd(s, p);
  gNetCb = wifiConnCb;
  netRun(T_TESTW, s, p);
  toast("Conectando...");
}
static void addNetModalCreate(const String &pre) {
  wifiModalDel();
  wifiModal = lv_obj_create(scrWifi);
  lv_obj_set_pos(wifiModal, 1, 52);
  lv_obj_set_size(wifiModal, 126, 104);
  lv_obj_set_style_pad_all(wifiModal, 0, 0);   // pad do tema empurrava os campos
  lv_obj_set_style_bg_color(wifiModal, REDMI_CARD, 0);
  lv_obj_set_style_bg_opa(wifiModal, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(wifiModal, 10, 0);
  lv_obj_set_style_border_color(wifiModal, REDMI_ACC, 0);
  lv_obj_clear_flag(wifiModal, LV_OBJ_FLAG_SCROLLABLE);
  wmTa1 = uiTa(wifiModal, 3, 2, 120, 20);
  lv_textarea_set_placeholder_text(wmTa1, "rede");
  if (pre.length()) lv_textarea_set_text(wmTa1, pre.c_str());
  wmTa2 = uiTa(wifiModal, 3, 24, 120, 20, true);
  lv_textarea_set_placeholder_text(wmTa2, "senha");
  wmRaise = wifiModal;                       // teclado levanta esse modal
  lv_obj_t *bOk = uiBtn(wifiModal, "Salvar", wifiAddModalSubmit, 60, 22, 0x1A3B25, NULL);
  lv_obj_set_pos(bOk, 2, 76);
  wmB1 = bOk;
  lv_obj_t *bCc = uiBtn(wifiModal, "Cancelar", [](lv_event_t *) { wifiModalDel(); },
                        60, 22, 0x3B1B1B, NULL);
  lv_obj_set_pos(bCc, 64, 76);
  wmB2 = bCc;
}

static void wifiScanDone(bool ok, const String &out) {
  (void)ok;
  gScanN = 0;
  int p = 0;
  while (gScanN < 8 && p < (int)out.length()) {
    int e = out.indexOf('\n', p); if (e < 0) e = out.length();
    String line = out.substring(p, e); p = e + 1;
    int sp = line.lastIndexOf(" (");
    if (sp <= 0) continue;
    String nn = line.substring(0, sp);
    if (!nn.length()) continue;
    gScanSsid[gScanN] = nn;
    gScanRssi[gScanN] = line.substring(sp + 2, line.length() - 1).toInt();
    gScanN++;
  }
  wifiFillRows();
  wifiStatusText();
}
static void wifiScanStartCb(lv_event_t *) {
  if (gNetBusy) { toast("Aguarde..."); return; }
  wifiModalDel();
  gNetCb = wifiScanDone;
  netRun(T_SCAN);
  toast("Escaneando...");
}
static void wifiNtpBtnCb(lv_event_t *) {
  if (gNetBusy) { toast("Aguarde..."); return; }
  gNetCb = [](bool ok, const String &) { toast(ok ? "Hora atualizada" : "NTP falhou"); };
  netRun(T_NTP);
}
static void wifiAddBtnCb(lv_event_t *) { wifiModalDel(); addNetModalCreate(""); }
static void wifiScanPickCb(lv_event_t *e) {          // rede do scan: SSID ja vem preenchido
  int j = (int)(intptr_t)lv_event_get_user_data(e);
  wifiModalDel();
  addNetModalCreate(gScanSsid[j]);
}
static void wifiDelLongCb(lv_event_t *e) {
  int i = (int)(intptr_t)lv_event_get_user_data(e);
  wifiDel(i);
  toast("Rede esquecida");
  wifiFillRows();
}
static void wifiFillRows() {
  if (lv_obj_is_valid(wifiListCont)) {
    lv_obj_clean(wifiListCont);
    wifiLoad();
    int y = 0;
    for (int i = 0; i < wifiN; i++) {
      lv_obj_t *b = lv_btn_create(wifiListCont);
      lv_obj_set_pos(b, 0, y);
      lv_obj_set_size(b, 112, 20);
      lv_obj_set_style_bg_color(b, REDMI_CARD, 0);
      lv_obj_set_style_radius(b, 6, 0);
      lv_obj_set_style_shadow_width(b, 0, 0);
      lv_obj_t *l = lv_label_create(b);
      String t = wifiSsid[i] + " *";
      lv_label_set_text(l, t.c_str());
      lv_obj_set_style_text_font(l, F_UI, 0);
      lv_obj_set_style_text_color(l, REDMI_OK, 0);
      lv_obj_set_width(l, 100);
      lv_label_set_long_mode(l, LV_LABEL_LONG_DOT);
      lv_obj_align(l, LV_ALIGN_LEFT_MID, 5, 0);
      lv_obj_add_event_cb(b, wifiConnectSaveCb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
      lv_obj_add_event_cb(b, wifiDelLongCb, LV_EVENT_LONG_PRESSED, (void *)(intptr_t)i);
      y += 22;
    }
    for (int j = 0; j < gScanN && y < 140; j++) {
      bool dup = false;
      for (int i = 0; i < wifiN; i++) if (wifiSsid[i] == gScanSsid[j]) dup = true;
      if (dup) continue;
      lv_obj_t *b = lv_btn_create(wifiListCont);
      lv_obj_set_pos(b, 0, y);
      lv_obj_set_size(b, 112, 20);
      lv_obj_set_style_bg_color(b, REDMI_CARD, 0);
      lv_obj_set_style_radius(b, 6, 0);
      lv_obj_set_style_shadow_width(b, 0, 0);
      lv_obj_t *l = lv_label_create(b);
      char bb[48];
      snprintf(bb, sizeof(bb), "%s (%d)", gScanSsid[j].c_str(), gScanRssi[j]);
      lv_label_set_text(l, bb);
      lv_obj_set_style_text_font(l, F_UI, 0);
      lv_obj_set_style_text_color(l, REDMI_DIM, 0);
      lv_obj_set_width(l, 100);
      lv_label_set_long_mode(l, LV_LABEL_LONG_DOT);
      lv_obj_align(l, LV_ALIGN_LEFT_MID, 5, 0);
      lv_obj_add_event_cb(b, wifiScanPickCb, LV_EVENT_CLICKED, (void *)(intptr_t)j);
      y += 22;
    }
  }
}
void buildWifi() {
  scrWifi = lv_obj_create(NULL);
  scrStyle(scrWifi);
  appHeader(scrWifi, "WI-FI");
  wifiModal = nullptr;
  wifiSt = lv_label_create(scrWifi);
  lv_label_set_text(wifiSt, "...");
  lv_obj_set_pos(wifiSt, 5, 44);
  lv_obj_set_width(wifiSt, 118);
  lv_obj_set_style_text_font(wifiSt, F_UI, 0);
  lv_obj_set_style_text_color(wifiSt, REDMI_DIM, 0);
  lv_label_set_long_mode(wifiSt, LV_LABEL_LONG_DOT);
  wifiListCont = lv_obj_create(scrWifi);
  lv_obj_set_pos(wifiListCont, 0, 84);
  lv_obj_set_size(wifiListCont, 128, 74);
  lv_obj_set_style_bg_opa(wifiListCont, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_opa(wifiListCont, 0, 0);
  lv_obj_set_style_pad_all(wifiListCont, 4, 0);
  lv_obj_set_scrollbar_mode(wifiListCont, LV_SCROLLBAR_MODE_AUTO);
  // botoes da linha de acoes
  lv_obj_t *b1 = uiBtn(scrWifi, "Scan", wifiScanStartCb, 38, 20, 0x161A22, NULL);
  lv_obj_set_pos(b1, 4, 62);
  lv_obj_t *b2 = uiBtn(scrWifi, "+Rede", wifiAddBtnCb, 39, 20, 0x161A22, NULL);
  lv_obj_set_pos(b2, 44, 62);
  lv_obj_t *b3 = uiBtn(scrWifi, "NTP", wifiNtpBtnCb, 39, 20, 0x161A22, NULL);
  lv_obj_set_pos(b3, 85, 62);
  wifiFillRows();
  wifiStatusText();
}

// ============================================================
//  TELA (brilho, calibracao, girar)
// ============================================================
static lv_obj_t *brSl;
static void brSlEvent(lv_event_t *e) {
  int v = lv_slider_get_value(lv_event_get_target(e));
  cfg.brightness = v;
  blSet(v);
  if (lv_event_get_code(e) == LV_EVENT_RELEASED) cfgSave();
}
static void telaRecalibCb(lv_event_t *) { enterDirect(calibRun); }
static void telaCoresCbCb(lv_event_t *) { enterDirect(showColorTest); }
static void telaGirarCb(lv_event_t *) {
  cfg.rotation ^= 1;
  cfgSave();
  tft.setRotation((cfg.rotation & 1) ? 2 : 0);   // flip nativo do controlador
  lv_obj_invalidate(lv_scr_act());               // repinta tudo ja na orientacao nova
}
static void telaTestBtnCb(lv_event_t *) {
  buildTelaTest();
  nav(scrTelaTest, LV_SCR_LOAD_ANIM_MOVE_LEFT);
}
static void telaPersCb(lv_event_t *) { buildPers(); nav(scrPers, LV_SCR_LOAD_ANIM_MOVE_LEFT); }
void buildTela() {
  scrTela = lv_obj_create(NULL);
  scrStyle(scrTela);
  appHeader(scrTela, "TELA");
  lv_obj_t *body = appBody(scrTela);
  uiLabel(body, "BRILHO", 0, 0, F_SM, lv_color_to32(REDMI_DIM));
  brSl = lv_slider_create(body);
  lv_obj_set_pos(brSl, 0, 12);
  lv_obj_set_size(brSl, 118, 12);
  lv_slider_set_range(brSl, 15, 100);
  lv_slider_set_value(brSl, cfg.brightness, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(brSl, REDMI_ACC, LV_PART_INDICATOR | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(brSl, REDMI_CARD, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(brSl, 2, LV_PART_KNOB);
  lv_obj_set_style_bg_color(brSl, REDMI_TXT, LV_PART_KNOB);
  lv_obj_add_event_cb(brSl, brSlEvent, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_event_cb(brSl, brSlEvent, LV_EVENT_RELEASED, NULL);
  cfgBtn(body, "Calibrar", telaRecalibCb, 0, 0, lv_color_to32(REDMI_CARD), lv_color_to32(REDMI_TXT), 30);
  cfgBtn(body, "Cores",    telaCoresCbCb, 1, 0, lv_color_to32(REDMI_CARD), lv_color_to32(REDMI_TXT), 30);
  cfgBtn(body, "Testar",   telaTestBtnCb, 0, 1, lv_color_to32(REDMI_CARD), lv_color_to32(REDMI_TXT), 30);
  cfgBtn(body, "Girar",    telaGirarCb,   1, 1, lv_color_to32(REDMI_CARD), lv_color_to32(REDMI_TXT), 30);
  redRow(body, "P", "Personalizacao", telaPersCb, 80);
}

// ------------------------------------------------------------
//  TESTAR TOQUE (ponto que segue o dedo + coordenadas)
// ------------------------------------------------------------
static lv_obj_t *ttDot, *ttPos;
void buildTelaTest() {
  scrTelaTest = lv_obj_create(NULL);
  scrStyle(scrTelaTest);
  appHeader(scrTelaTest, "TOQUE");
  ttPos = uiLabel(scrTelaTest, "toque na tela", 4, 132, F_UI, lv_color_to32(REDMI_OK));
  (void)ttPos;
  ttDot = lv_obj_create(scrTelaTest);
  lv_obj_set_size(ttDot, 20, 20);
  lv_obj_set_style_radius(ttDot, 10, 0);
  lv_obj_set_style_bg_color(ttDot, REDMI_ACC, 0);
  lv_obj_set_style_bg_opa(ttDot, LV_OPA_COVER, 0);
  lv_obj_set_style_border_opa(ttDot, 0, 0);
  lv_obj_clear_flag(ttDot, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(ttDot, LV_OBJ_FLAG_HIDDEN);
}

// ============================================================
//  AJUSTES (dormir, wifi automatico)
// ============================================================
static lv_obj_t *slSleep, *swAuto, *sleepLb;
static void slSleepEvent(lv_event_t *e) {
  int v = lv_slider_get_value(lv_event_get_target(e));
  cfg.sleepS = v;
  if (lv_event_get_code(e) == LV_EVENT_RELEASED) {
    cfgSave();
    char b[24];
    snprintf(b, sizeof(b), "Bloquear em %ds", v);
    if (lv_obj_is_valid(sleepLb)) lv_label_set_text(sleepLb, b);
  }
}
static void swAutoCb(lv_event_t *e) {
  lv_obj_t *sw = lv_event_get_target(e);
  cfg.autoWifi = lv_obj_has_state(sw, LV_STATE_CHECKED);
  cfgSave();
}
static void swBeepCb(lv_event_t *e) {
  lv_obj_t *sw = lv_event_get_target(e);
  cfg.beepOn = lv_obj_has_state(sw, LV_STATE_CHECKED);
  cfgSave();
}
static void swWakeCb(lv_event_t *e) {
  lv_obj_t *sw = lv_event_get_target(e);
  cfg.wake2 = lv_obj_has_state(sw, LV_STATE_CHECKED);
  cfgSave();
  if (!cfg.wake2) { gWakeArm = false; gWakeWait = false; gWakeTaps = 0; }
}
// linha com switch embutido (Redmi); retorna o switch
static lv_obj_t *uiRowSw(lv_obj_t *parent, const char *txt, bool on, int y) {
  lv_obj_t *row = lv_obj_create(parent);
  lv_obj_set_pos(row, 0, y);
  lv_obj_set_size(row, 118, 18);
  lv_obj_set_style_bg_color(row, lv_color_hex(0x161A22), 0);
  lv_obj_set_style_radius(row, 9, 0);
  lv_obj_set_style_border_width(row, 0, 0);
  lv_obj_set_style_pad_all(row, 0, 0);
  lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(row, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_t *l = lv_label_create(row);
  lv_label_set_text(l, txt);
  lv_obj_set_style_text_font(l, F_UI, 0);
  lv_obj_set_style_text_color(l, REDMI_TXT, 0);
  lv_obj_align(l, LV_ALIGN_LEFT_MID, 6, 0);
  lv_obj_t *sw = lv_switch_create(row);
  lv_obj_set_size(sw, 26, 16);
  lv_obj_align(sw, LV_ALIGN_RIGHT_MID, -3, 0);
  if (on) lv_obj_add_state(sw, LV_STATE_CHECKED);
  return sw;
}
void buildAjustes() {
  scrAjustes = lv_obj_create(NULL);
  scrStyle(scrAjustes);
  appHeader(scrAjustes, "AJUSTES");
  lv_obj_t *body = appBody(scrAjustes);
  // secao BLOQUEIO: valor no titulo + slider full width
  sleepLb = uiLabel(body, "Bloqueio automatico", 0, 0, F_SM, lv_color_to32(REDMI_DIM));
  (void)sleepLb;
  slSleep = lv_slider_create(body);
  lv_obj_set_pos(slSleep, 0, 12);
  lv_obj_set_size(slSleep, 118, 12);
  lv_slider_set_range(slSleep, 5, 120);
  lv_slider_set_value(slSleep, cfg.sleepS, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(slSleep, REDMI_ACC, LV_PART_INDICATOR | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(slSleep, REDMI_CARD, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(slSleep, 2, LV_PART_KNOB);
  lv_obj_set_style_bg_color(slSleep, REDMI_TXT, LV_PART_KNOB);
  lv_obj_add_event_cb(slSleep, slSleepEvent, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_event_cb(slSleep, slSleepEvent, LV_EVENT_RELEASED, NULL);
  // linhas de switch estilo Redmi (card 18px, switch a direita)
  lv_obj_t *lb2 = uiRowSw(body, "WiFi auto", cfg.autoWifi, 32);
  (void)lb2; lv_obj_add_event_cb(lb2, swAutoCb, LV_EVENT_VALUE_CHANGED, NULL); swAuto = lb2;
  lv_obj_t *lb3 = uiRowSw(body, "Bip ao tocar", cfg.beepOn, 54);
  (void)lb3; lv_obj_add_event_cb(lb3, swBeepCb, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_t *lb4 = uiRowSw(body, "2 toques p/ acordar", cfg.wake2, 76);
  (void)lb4; lv_obj_add_event_cb(lb4, swWakeCb, LV_EVENT_VALUE_CHANGED, NULL);
}

// ============================================================
//  CLIMA (Open-Meteo: sem API key; editar lat/lon no lugar)
//  formato do resultado: temp|desc|vento|umidade|sensacao
// ============================================================
static lv_obj_t *wxTemp, *wxDescL, *wxPos, *wxDet;
static lv_obj_t *wxLatTa = nullptr, *wxLonTa = nullptr;
static void wxApplyData(const String &out);   // preenche gWxTxt/gWxTemp/gWxDesc/gWxKind
static String wxField(const String &s, int idx) {   // campo idx (0-based) separado por '|'
  int p = 0;
  for (int i = 0; i < idx; i++) {
    int b = s.indexOf('|', p);
    if (b < 0) return "";
    p = b + 1;
  }
  int e = s.indexOf('|', p);
  return s.substring(p, e < 0 ? s.length() : e);
}
static void wxApplyCb(lv_event_t *) {
  if (gNetBusy) { toast("Aguarde..."); return; }
  if (WiFi.status() != WL_CONNECTED) { toast("Sem Wi-Fi conectado"); return; }
  cfg.wLat = atof(lv_textarea_get_text(wxLatTa));
  cfg.wLon = atof(lv_textarea_get_text(wxLonTa));
  if (cfg.wLat == 0 && cfg.wLon == 0) { toast("Coordenadas invalidas"); return; }
  cfgSave();
  char b[40];
  snprintf(b, sizeof(b), "%.2f, %.2f", cfg.wLat, cfg.wLon);
  if (lv_obj_is_valid(wxPos)) lv_label_set_text(wxPos, b);
  gNetCb = [](bool ok, const String &out) {
    if (!lv_obj_is_valid(wxTemp)) return;
    if (!ok || out.startsWith("sem") || out.startsWith("Falha")) {
      lv_label_set_text(wxTemp, "--");
      if (lv_obj_is_valid(wxDescL)) lv_label_set_text(wxDescL, "sem dados");
      if (lv_obj_is_valid(wxDet))  lv_label_set_text(wxDet, "");
      toast("Clima falhou");
      return;
    }
    String t = wxField(out, 0), d = wxField(out, 1);
    String w = wxField(out, 2), h = wxField(out, 3), a = wxField(out, 4);
    lv_label_set_text(wxTemp, (t + "°").c_str());
    if (lv_obj_is_valid(wxDescL)) lv_label_set_text(wxDescL, d.c_str());
    // linha de detalhes: vento · umidade · sensacao (campos indisponiveis = -1)
    if (lv_obj_is_valid(wxDet)) {
      char det[44];
      det[0] = 0;
      bool anyW = w.length() && w.toFloat() >= 0;
      bool anyH = h.length() && h.toFloat() >= 0;
      bool anyA = a.length() && a.toFloat() > -900;
      int p = 0;
      if (anyW) p += snprintf(det + p, sizeof(det) - p, "vento %skm/h", w.c_str());
      if (anyH) { if (p) det[p++] = ' '; p += snprintf(det + p, sizeof(det) - p, "%s%%", h.c_str()); }
      if (anyA) { if (p) det[p++] = ' '; p += snprintf(det + p, sizeof(det) - p, "sen.%s", a.c_str()); }
      if (!p) snprintf(det, sizeof(det), " ");
      lv_label_set_text(wxDet, det);
    }
    wxApplyData(out);                  // face/topo: temperatura + icone + descricao
  };
  netRun(T_WX);
  toast("Buscando clima...");
}
void buildClima() {
  scrClima = lv_obj_create(NULL);
  scrStyle(scrClima);
  appHeader(scrClima, "CLIMA");
  wxTemp = lv_label_create(scrClima);
  lv_obj_align(wxTemp, LV_ALIGN_TOP_MID, 0, 42);
  lv_obj_set_style_text_font(wxTemp, F_BIG, 0);
  lv_obj_set_style_text_color(wxTemp, REDMI_ACC, 0);
  lv_label_set_text(wxTemp, "--°");
  wxDescL = uiLabel(scrClima, "toque em Atualizar", 5, 76, F_UI, lv_color_to32(REDMI_DIM));
  lv_obj_set_width(wxDescL, 118);
  lv_label_set_long_mode(wxDescL, LV_LABEL_LONG_DOT);
  lv_obj_set_style_text_align(wxDescL, LV_TEXT_ALIGN_CENTER, 0);
  // linha de detalhes: vento · umidade · sensacao (preenchida ao atualizar)
  wxDet = uiLabel(scrClima, " ", 0, 60, F_SM, lv_color_to32(REDMI_DIM));
  lv_obj_set_width(wxDet, 118);
  lv_obj_set_pos(wxDet, 5, 93);
  lv_obj_set_style_text_align(wxDet, LV_TEXT_ALIGN_CENTER, 0);
  char b[40];
  wxPos = nullptr;                          // coords agora visiveis nos campos abaixo
  lv_obj_t *ll = uiLabel(scrClima, "lat:", 0, 96, F_SM, 0x8B90A0);
  lv_obj_set_pos(ll, 2, 110);
  wxLatTa = uiTa(scrClima, 18, 107, 44, 21);
  snprintf(b, sizeof(b), "%.4f", cfg.wLat);
  lv_textarea_set_text(wxLatTa, b);
  lv_textarea_set_one_line(wxLatTa, true);
  lv_obj_t *lo = uiLabel(scrClima, "lon:", 0, 96, F_SM, 0x8B90A0);
  lv_obj_set_pos(lo, 65, 110);
  wxLonTa = uiTa(scrClima, 84, 107, 42, 21);
  snprintf(b, sizeof(b), "%.4f", cfg.wLon);
  lv_textarea_set_text(wxLonTa, b);
  lv_textarea_set_one_line(wxLonTa, true);
  lv_obj_t *bu = uiBtn(scrClima, "Atualizar", wxApplyCb, 118, 20, 0x10304A, NULL);
  lv_obj_set_pos(bu, 5, 133);
  if (gWxTxt.length() && lv_obj_is_valid(wxTemp)) {   // reaproveita valor de fundo
    int bar = gWxTxt.indexOf('°');
    if (bar > 0) lv_label_set_text(wxTemp, (gWxTxt.substring(0, bar) + "°").c_str());
    if (lv_obj_is_valid(wxDescL) && bar > 0)
      lv_label_set_text(wxDescL, gWxTxt.substring(bar + 2).c_str());
  }
}

// ============================================================
//  DATA E HORA (manual + NTP)
// ============================================================
static lv_obj_t *dhLbl[6];
static int dhV[6];   // h, m, s, d, mo, yy(2000+)
static void monDaysFix() {
  const int dmax[13] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
  int mx = dmax[dhV[4]];
  if (dhV[4] == 2 && ((2000 + dhV[5]) % 4 == 0)) mx = 29;
  if (dhV[3] > mx) dhV[3] = mx;
}
static void dhRefresh() {
  monDaysFix();
  for (int i = 0; i < 6; i++) {
    if (!lv_obj_is_valid(dhLbl[i])) continue;
    char b[12];
    if (i == 5) snprintf(b, sizeof(b), "20%02d", dhV[5]);
    else snprintf(b, sizeof(b), "%02d", dhV[i]);
    lv_label_set_text(dhLbl[i], b);
  }
}
static void dhTapCb(lv_event_t *e) {
  int i = (int)(intptr_t)lv_event_get_user_data(e);
  if (i == 0) dhV[0] = (dhV[0] + 1) % 24;
  else if (i == 1 || i == 2) dhV[i] = (dhV[i] + 1) % 60;
  else if (i == 3) dhV[3] = dhV[3] % 31 + 1;
  else if (i == 4) dhV[4] = dhV[4] % 12 + 1;
  else dhV[5] = (dhV[5] + 1) % 100;
  dhRefresh();
}
static void dhApplyCb(lv_event_t *) {
  monDaysFix();
  manualTimeSet(2000 + dhV[5], dhV[4], dhV[3], dhV[0], dhV[1], dhV[2]);
  toast("Hora aplicada");
}
static void dhNtpCb(lv_event_t *) {
  if (gNetBusy) { toast("Aguarde..."); return; }
  gNetCb = [](bool ok, const String &) {
    toast(ok ? "NTP sincronizado" : "NTP falhou");
    if (ok) {  // recarrega novos valores
      time_t now = time(nullptr);
      struct tm *l = localtime(&now);
      dhV[0] = l->tm_hour; dhV[1] = l->tm_min; dhV[2] = l->tm_sec;
      dhV[3] = l->tm_mday; dhV[4] = l->tm_mon + 1; dhV[5] = l->tm_year - 100;
      dhRefresh();
    }
  };
  netRun(T_NTP);
}
void buildDataHora() {
  scrDataHora = lv_obj_create(NULL);
  scrStyle(scrDataHora);
  appHeader(scrDataHora, "DATA E HORA");
  lv_obj_t *body = appBody(scrDataHora);
  int hh = 12, mm = 0, ss = 0, dd = 1, mo = 1, yy = 30;
  if (timeOk()) {
    time_t now = time(nullptr);
    struct tm *l = localtime(&now);
    hh = l->tm_hour; mm = l->tm_min; ss = l->tm_sec;
    dd = l->tm_mday; mo = l->tm_mon + 1; yy = l->tm_year - 100;
  }
  dhV[0] = hh; dhV[1] = mm; dhV[2] = ss; dhV[3] = dd; dhV[4] = mo; dhV[5] = yy;
  lv_obj_t *cap1 = uiLabel(body, "HORA", 2, 0, F_SM, lv_color_to32(REDMI_DIM));
  (void)cap1;
  static const int cx[3] = { 8, 48, 88 };
  for (int i = 0; i < 6; i++) {
    int col = i % 3, row = i / 3;
    dhLbl[i] = lv_label_create(body);
    lv_obj_set_pos(dhLbl[i], cx[col], row == 0 ? 14 : 60);
    lv_obj_set_style_text_font(dhLbl[i], F_MED, 0);
    lv_obj_set_style_text_color(dhLbl[i], REDMI_TXT, 0);
    lv_obj_add_flag(dhLbl[i], LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(dhLbl[i], dhTapCb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
  }
  lv_obj_t *cap2 = uiLabel(body, "DATA", 2, 46, F_SM, lv_color_to32(REDMI_DIM));
  (void)cap2;
  lv_obj_t *s1 = uiLabel(body, ":", 34, 14, F_MED, 0x676C7A);
  lv_obj_t *s2 = uiLabel(body, ":", 74, 14, F_MED, 0x676C7A);
  lv_obj_t *s3 = uiLabel(body, "/", 34, 60, F_MED, 0x676C7A);
  lv_obj_t *s4 = uiLabel(body, "/", 74, 60, F_MED, 0x676C7A);
  (void)s1; (void)s2; (void)s3; (void)s4;
  dhRefresh();
  lv_obj_t *b1 = uiBtn(body, "Aplicar", dhApplyCb, 58, 22, 0x1A3B25, NULL);
  lv_obj_set_pos(b1, 2, 86);
  lv_obj_t *b2 = uiBtn(body, "NTP", dhNtpCb, 58, 22, 0x1C202B, NULL);
  lv_obj_set_pos(b2, 66, 86);
}

// ============================================================
//  TESTE SISTEMA (diagnostico sequencial)
// ============================================================
static lv_obj_t *tstLbl[7];
static lv_timer_t *tstTimer = nullptr;
static int tstStep = 0;
static void tstNetCb(bool ok, const String &out) {
  int n = 0, p = 0;
  while (p < (int)out.length()) {
    int e = out.indexOf('\n', p); if (e < 0) e = out.length();
    if (e > p) n++;
    p = e + 1;
  }
  if (lv_obj_is_valid(tstLbl[4])) {
    char b[30];
    snprintf(b, sizeof(b), "WiFi: %s (%d redes)", ok ? "ok" : "vazio", n);
    lv_label_set_text(tstLbl[4], b);
  }
}
static void tstSetMsg(int i, const char *msg) {
  if (lv_obj_is_valid(tstLbl[i])) lv_label_set_text(tstLbl[i], msg);
}
static void tstSetMsgF(int i, const char *fmt, const char *v) {
  if (lv_obj_is_valid(tstLbl[i])) {
    char b[40];
    snprintf(b, sizeof(b), fmt, v);
    lv_label_set_text(tstLbl[i], b);
  }
}
static void tstTick(lv_timer_t *t) {
  if (!lv_obj_is_valid(scrTeste)) {
    lv_timer_del(t);
    tstTimer = nullptr;
    return;
  }
  switch (tstStep) {
    case 0: {
      int n0 = 0;
      File r = SPIFFS.open("/");
      if (r) {
        File f;
        while ((f = r.openNextFile())) { f.close(); n0++; }
        r.close();
      }
      char b[24]; snprintf(b, sizeof(b), "SPIFFS: %d arq", n0);
      tstSetMsg(0, b);
      break;
    }
    case 1: {
      char b[28];
      snprintf(b, sizeof(b), "Bateria: %.2fV %d%%", (double)batVoltage(), batPct());
      tstSetMsg(1, b);
      break;
    }
    case 2: {
      if (gMpuOk) {
        int16_t a, b2, c;
        mpuReadAcc(&a, &b2, &c);
        tstSetMsg(2, "MPU: ok");
      } else tstSetMsg(2, "MPU: ausente");
      break;
    }
    case 3: {
      String out;
      bool ok = sdList(out);
      tstSetMsg(3, ok ? "SD: ok (mesmo bus)" : "SD: ausente");
      break;
    }
    case 4: {
      if (!gNetBusy) {
        gNetCb = tstNetCb;
        netRun(T_SCAN);
        tstSetMsg(4, "WiFi: escaneando...");
      }
      break;   // fica ate o cb responder
    }
    case 5: {
      sendNEC(0xA1, 0x5A);
      beepSpeaker();
      tstSetMsg(5, "IR+som: enviado");
      tstSetMsg(6, "Teste concluido");
      tstStep = 7;
      lv_timer_del(t);
      tstTimer = nullptr;
      return;
    }
  }
  tstStep++;
}

// ============================================================
//  TOOLBOX  (IR / RF / Beep / Lanterna / BLE / Ping / Crono / AP falso)
// ============================================================
static const char *fakeApNames[7] = {
  "Wi-Fi Grats", "Tua Rede Avulsa", "ConectaJaGratis",
  "RedeDoVizinho", "AtualizaAndroid", "SomenteEscritorio", "VipHackerJedi"
};
static int gFakeIdx = 0;
static bool gFakeOn = false;
static lv_obj_t *swFake;   // botao AP falso (label muda; toque=liga/desliga, longo=troca nome)
static void fakeApApply() {
  if (gFakeOn) {
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(fakeApNames[gFakeIdx], "12345678");
  } else {
    WiFi.softAPdisconnect(true);
    if (WiFi.status() != WL_CONNECTED) WiFi.mode(WIFI_STA);
  }
  if (lv_obj_is_valid(swFake)) {
    lv_obj_set_style_bg_color(swFake, lv_color_hex(gFakeOn ? 0x1A3B25 : 0x1C202B), 0);
    // texto do estado (linha redRow: filho 1 = label do texto)
    uint32_t nCh = lv_obj_get_child_cnt(swFake);
    if (nCh >= 2 && lv_obj_check_type(lv_obj_get_child(swFake, 1), &lv_label_class)) {
      lv_label_set_text(lv_obj_get_child(swFake, 1), gFakeOn ? "AP falso (ON)" : "AP falso");
    }
  }
}
static void fakeSwCb(lv_event_t *e) {
  lv_event_code_t c = lv_event_get_code(e);
  static uint32_t tNameCycle = 0;
  if (c == LV_EVENT_LONG_PRESSED) {
    tNameCycle = millis();                       // guarda: click posterior sera ignorado
    gFakeIdx = (gFakeIdx + 1) % 7;
    char b[44];
    snprintf(b, sizeof(b), "AP: %s", fakeApNames[gFakeIdx]);
    toast(b);
    if (gFakeOn) { WiFi.softAP(fakeApNames[gFakeIdx], "12345678"); }
    return;
  }
  if (millis() - tNameCycle < 700) return;       // toque curto apos longo: nao alterna
  gFakeOn = !gFakeOn;
  char b[44];
  snprintf(b, sizeof(b), "AP falso %s", gFakeOn ? "ligado" : "desligado");
  toast(b);
  fakeApApply();
}
static void necBtnCb(lv_event_t *) {
  irSendAsync(0, 0xA1, 0x5A);            // assincrono: nao trava a UI
  beepSpeaker();
  toast("NEC enviado (IR)");
}
static void beepBtnCb(lv_event_t *) { beepSpeaker(); }

// ---------------- TV-B-Gone: loop de NEC na task propria ----------------
static volatile bool irLoopOn = false;
static lv_obj_t *tbIrLoop = nullptr;
static void irLoopTask(void *) {
  while (irLoopOn) {
    sendNEC(0xA1, 0x5A);
    vTaskDelay(pdMS_TO_TICKS(900));
  }
  irLoopOn = false;
  vTaskDelete(NULL);
}
static void irLoopCb(lv_event_t *) {
  irLoopOn = !irLoopOn;
  if (irLoopOn) {
    xTaskCreate(irLoopTask, "irloop", 4096, NULL, 1, NULL);
    toast("IR loop ligado (toque de novo p/ parar)");
  } else toast("IR loop desligado");
  if (lv_obj_is_valid(tbIrLoop))
    lv_obj_set_style_bg_color(tbIrLoop, lv_color_hex(irLoopOn ? 0x1A3B25 : 0x1C202B), 0);
}

// ---------------- Lanterna (tela branca + backlight 100%) ----------------
static lv_obj_t *ltOv = nullptr;
static void ltCloseCb(lv_event_t *) {
  if (lv_obj_is_valid(ltOv)) { lv_obj_del(ltOv); }
  ltOv = nullptr;
  blOn();                       // restaura brilho da config
}
static void lanternaCb(lv_event_t *) {
  if (ltOv) { ltCloseCb(nullptr); return; }
  blSet(100);
  ltOv = lv_obj_create(lv_layer_top());
  lv_obj_set_pos(ltOv, 0, 0);
  lv_obj_set_size(ltOv, 128, 160);
  lv_obj_set_style_bg_color(ltOv, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_bg_opa(ltOv, LV_OPA_COVER, 0);
  lv_obj_set_style_border_opa(ltOv, 0, 0);
  lv_obj_set_style_radius(ltOv, 0, 0);
  lv_obj_set_style_shadow_width(ltOv, 0, 0);
  lv_obj_clear_flag(ltOv, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(ltOv, ltCloseCb, LV_EVENT_CLICKED, NULL);
}

// ---------------- Ping (latencia TCP ate 1.1.1.1:80) ----------------
static void pingOutCb(bool ok, const String &out) { toast(out.c_str()); }
static void pingBtnCb(lv_event_t *) {
  if (gNetBusy) { toast("Aguarde..."); return; }
  if (WiFi.status() != WL_CONNECTED) { toast("Sem Wi-Fi conectado"); return; }
  gNetCb = pingOutCb;
  netRun(T_PING);
  toast("Pingando 1.1.1.1...");
}

// ---------------- Cronometro + Timer (contagem regressiva) ----------------
static lv_obj_t *cronT, *cronBt, *cronZeroB, *cronLapB, *cronPlusB, *cronMinusB, *cronLaps;
static lv_obj_t *cronModeA, *cronModeB;
static bool cronRun = false;
static bool cronTimer = false;      // false=cronometro  true=timer
static uint32_t cronBase = 0, cronT0 = 0;
static uint32_t cronLapT[3] = { 0, 0, 0 };
static int cronLapN = 0;
static uint32_t cronPreset = 60000; // 60s
static uint32_t cronElapsed() {     // tempo atual conforme o modo
  if (cronTimer) {
    if (cronRun && millis() - cronT0 >= cronBase) return 0;
    return cronBase - (cronRun ? millis() - cronT0 : 0);
  }
  return cronBase + (cronRun ? millis() - cronT0 : 0);
}
static void cronShowFmt(uint32_t v) {
  char b[16];
  snprintf(b, sizeof(b), "%02u:%02u.%u",
           (unsigned)((v / 60000) % 100), (unsigned)((v / 1000) % 60),
           (unsigned)((v / 100) % 10));
  if (lv_obj_is_valid(cronT)) lv_label_set_text(cronT, b);
}
static void cronUpdate(bool raw = false) {
  if (!lv_obj_is_valid(cronT)) return;
  uint32_t v = cronElapsed();
  cronShowFmt(v);
  if (cronTimer && v == 0 && cronRun) {          // fim do timer
    cronRun = false;
    cronBase = 0;                                // mantem 00:00 no visor (rearma ao iniciar)
    if (lv_obj_is_valid(cronBt)) lv_label_set_text(lv_obj_get_child(cronBt, 0), "Iniciar");
    for (int i = 0; i < 3; i++) beepSpeaker();
    toast("TEMPO ESGOTADO!");
  }
  if (raw && lv_obj_is_valid(cronLaps)) {
    char b[64];
    if (!cronTimer && cronLapN > 0) {
      int i = cronLapN - 1;                       // mostra a volta mais recente (cabe em 1 linha)
      snprintf(b, sizeof(b), "volta %d: %02u:%02u.%u", i + 1,
               (unsigned)((cronLapT[i] / 60000) % 100),
               (unsigned)((cronLapT[i] / 1000) % 60),
               (unsigned)((cronLapT[i] / 100) % 10));
    } else {
      snprintf(b, sizeof(b), cronTimer ? "preset %us" : "sem voltas",
               (unsigned)(cronPreset / 1000));
    }
    lv_label_set_text(cronLaps, b);
  }
}
static void cronPlayCb(lv_event_t *) {
  if (cronTimer && !cronRun && cronBase == 0) cronBase = cronPreset;   // rearma
  if (cronRun) {                                    // pausar: fecha o segmento atual
    uint32_t el = millis() - cronT0;
    if (cronTimer) cronBase = (el >= cronBase) ? 0 : cronBase - el;
    else           cronBase += el;
    cronRun = false;
  } else {
    cronT0 = millis();
    cronRun = true;
  }
  if (lv_obj_is_valid(cronBt)) lv_label_set_text(lv_obj_get_child(cronBt, 0), cronRun ? "Pausar" : "Iniciar");
  cronUpdate(true);
}
static void cronZeroCb(lv_event_t *) {
  cronRun = false;
  if (cronTimer) cronBase = cronPreset;
  else { cronBase = 0; cronLapN = 0; for (int i = 0; i < 3; i++) cronLapT[i] = 0; }
  if (lv_obj_is_valid(cronBt)) lv_label_set_text(lv_obj_get_child(cronBt, 0), "Iniciar");
  cronUpdate(true);
}
static void cronPresetStep(int dir) {
  static const uint32_t steps[] = { 30000, 60000, 180000, 300000, 600000 };
  int i = 0;
  for (i = 0; i < 5; i++) if (steps[i] == cronPreset) break;
  i += dir;
  if (i < 0) i = 4;
  if (i > 4) i = 0;
  cronPreset = steps[i];
  if (!cronRun) cronBase = cronPreset;
  cronUpdate(true);
}
static void cronPlusCb(lv_event_t *)  { cronPresetStep(1); }
static void cronMinusCb(lv_event_t *) { cronPresetStep(-1); }
static void cronLapCb(lv_event_t *) {
  if (cronTimer) return;
  uint32_t tot = cronBase + (cronRun ? millis() - cronT0 : 0);
  if (cronLapN < 3) cronLapT[cronLapN++] = tot;
  else { cronLapT[0] = cronLapT[1]; cronLapT[1] = cronLapT[2]; cronLapT[2] = tot; }
  cronUpdate(true);
}
static void cronModePaint() {
  if (!lv_obj_is_valid(cronModeA) || !lv_obj_is_valid(cronModeB)) return;
  lv_obj_set_style_bg_color(cronModeA, lv_color_hex(cronTimer ? 0x161A22 : 0x10304A), 0);
  lv_obj_set_style_bg_color(cronModeB, lv_color_hex(cronTimer ? 0x10304A : 0x161A22), 0);
}
static void cronModeCb(lv_event_t *) {
  cronTimer = !cronTimer;
  cronRun = false;
  if (cronTimer) cronBase = cronPreset;
  else { cronBase = 0; cronLapN = 0; for (int i = 0; i < 3; i++) cronLapT[i] = 0; }
  if (lv_obj_is_valid(cronBt)) lv_label_set_text(lv_obj_get_child(cronBt, 0), "Iniciar");
  // botoes exclusivos de cada modo
  bool tm = cronTimer;
  if (lv_obj_is_valid(cronLapB))  { tm ? lv_obj_add_flag(cronLapB,  LV_OBJ_FLAG_HIDDEN) : lv_obj_clear_flag(cronLapB,  LV_OBJ_FLAG_HIDDEN); }
  if (lv_obj_is_valid(cronPlusB)) { tm ? lv_obj_clear_flag(cronPlusB, LV_OBJ_FLAG_HIDDEN) : lv_obj_add_flag(cronPlusB, LV_OBJ_FLAG_HIDDEN); }
  if (lv_obj_is_valid(cronMinusB)){ tm ? lv_obj_clear_flag(cronMinusB,LV_OBJ_FLAG_HIDDEN) : lv_obj_add_flag(cronMinusB,LV_OBJ_FLAG_HIDDEN); }
  cronModePaint();
  cronUpdate(true);
}
static void cronBtnCb(lv_event_t *) {
  buildCron();
  nav(scrCron, LV_SCR_LOAD_ANIM_MOVE_LEFT);
}
void buildCron() {
  scrCron = lv_obj_create(NULL);
  scrStyle(scrCron);
  appHeader(scrCron, "CRONO");
  cronT = lv_label_create(scrCron);
  lv_obj_set_style_text_font(cronT, F_MED, 0);
  lv_obj_set_style_text_color(cronT, REDMI_TXT, 0);
  lv_label_set_text(cronT, "00:00.0");
  lv_obj_align(cronT, LV_ALIGN_TOP_MID, 0, 44);
  lv_obj_set_width(cronT, 128);
  lv_obj_set_style_text_align(cronT, LV_TEXT_ALIGN_CENTER, 0);
  // seletor de modo
  cronModeA = uiBtn(scrCron, "Crono", cronModeCb, 63, 18, 0x10304A, NULL);
  lv_obj_set_pos(cronModeA, 0, 70);
  cronModeB = uiBtn(scrCron, "Timer", cronModeCb, 63, 18, 0x161A22, NULL);
  lv_obj_set_pos(cronModeB, 65, 70);
  cronBt = uiBtn(scrCron, "Iniciar", cronPlayCb, 60, 22, 0x10304A, NULL);
  lv_obj_set_pos(cronBt, 0, 90);
  cronZeroB = uiBtn(scrCron, "Zerar", cronZeroCb, 60, 22, 0x3B1B1B, NULL);
  lv_obj_set_pos(cronZeroB, 68, 90);
  cronLapB = uiBtn(scrCron, "Volta", cronLapCb, 60, 18, 0x161A22, NULL);
  lv_obj_set_pos(cronLapB, 0, 114);
  cronPlusB = uiBtn(scrCron, "+", cronPlusCb, 29, 18, 0x1A3B25, NULL);
  lv_obj_set_pos(cronPlusB, 66, 114);
  lv_obj_add_flag(cronPlusB, LV_OBJ_FLAG_HIDDEN);
  cronMinusB = uiBtn(scrCron, "-", cronMinusCb, 29, 18, 0x1A3B25, NULL);
  lv_obj_set_pos(cronMinusB, 96, 114);
  lv_obj_add_flag(cronMinusB, LV_OBJ_FLAG_HIDDEN);
  cronLaps = lv_label_create(scrCron);
  lv_obj_set_pos(cronLaps, 0, 136);
  lv_obj_set_width(cronLaps, 128);
  lv_obj_set_style_text_font(cronLaps, F_SM, 0);
  lv_obj_set_style_text_color(cronLaps, REDMI_DIM, 0);
  lv_obj_set_style_text_align(cronLaps, LV_TEXT_ALIGN_CENTER, 0);
  lv_label_set_long_mode(cronLaps, LV_LABEL_LONG_DOT);
  lv_label_set_text(cronLaps, "sem voltas");
  // aplica estado atual (modo pode ter mudado em outra sessao)
  if (cronTimer) {
    lv_obj_add_flag(cronLapB, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(cronPlusB, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(cronMinusB, LV_OBJ_FLAG_HIDDEN);
  }
  cronModePaint();
  if (cronRun && lv_obj_is_valid(cronBt))
    lv_label_set_text(lv_obj_get_child(cronBt, 0), "Pausar");
  cronUpdate(true);
}

// ---------------- BLE Scan (nomes + RSSI) ----------------
static lv_obj_t *bleOut;
static void bleOutCb(bool ok, const String &out) {
  if (lv_obj_is_valid(bleOut)) lv_label_set_text(bleOut, out.c_str());
  (void)ok;
}
static void bleScanBtnCb2(lv_event_t *) {
  if (lv_obj_is_valid(bleOut)) lv_label_set_text(bleOut, "buscando 4s...");
}
static void bleScanAction(lv_event_t *) {
#if HW_BLE
  if (gNetBusy) { toast("Aguarde..."); return; }
  gNetCb = bleOutCb;
  netRun(T_BLE);
  bleScanBtnCb2(nullptr);
#else
  toast("BLE nao compilado");
#endif
}
void buildBleScan() {
  scrBle = lv_obj_create(NULL);
  scrStyle(scrBle);
  appHeader(scrBle, "BLE SCAN");
  lv_obj_t *body = appBody(scrBle);
  bleOut = lv_label_create(body);
  lv_obj_set_pos(bleOut, 2, 2);
  lv_obj_set_size(bleOut, 114, 74);
  lv_label_set_long_mode(bleOut, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_font(bleOut, F_SM, 0);
  lv_obj_set_style_text_color(bleOut, REDMI_TXT, 0);
  lv_label_set_text(bleOut, "toque em Escanear");
  uiBtn(body, "Escanear 4s", bleScanAction, 116, 22, 0x1A3B25, NULL);
  lv_obj_t *b = lv_obj_get_child(body, -1);
  if (b) lv_obj_set_pos(b, 1, 82);
}
void buildTeste() {
  scrTeste = lv_obj_create(NULL);
  scrStyle(scrTeste);
  appHeader(scrTeste, "TESTE");
  tstStep = 0;
  for (int i = 0; i < 7; i++) {
    tstLbl[i] = lv_label_create(scrTeste);
    lv_obj_set_pos(tstLbl[i], 6, 46 + i * 15);
    lv_obj_set_width(tstLbl[i], 118);
    lv_label_set_long_mode(tstLbl[i], LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(tstLbl[i], F_SM, 0);
    lv_obj_set_style_text_color(tstLbl[i], i == 6 ? REDMI_OK : REDMI_DIM, 0);
    lv_label_set_text(tstLbl[i], "...");
  }
  tstSetMsg(6, "Iniciando...");
  if (!tstTimer) tstTimer = lv_timer_create(tstTick, 400, NULL);
}
// ---------------- grade de utilitarios (redRow lista estilo Redmi) ----------------
void buildToolbox() {
  scrToolbox = lv_obj_create(NULL);
  scrStyle(scrToolbox);
  appHeader(scrToolbox, "TOOLBOX");
  lv_obj_t *body = appBody(scrToolbox);
  lv_obj_set_scroll_dir(body, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(body, LV_SCROLLBAR_MODE_AUTO);
  int y = 0;
  redRow(body, LV_SYMBOL_VIDEO,   "TVs",         [](lv_event_t *) { buildBrands(false); }, y); y += 20;
  redRow(body, LV_SYMBOL_IMAGE,   "AR",          [](lv_event_t *) { buildBrands(true);  }, y); y += 20;
  redRow(body, LV_SYMBOL_POWER,   "TV-Gone",     [](lv_event_t *) { buildTvBGone();     }, y); y += 20;
#if HW_BLE
  redRow(body, LV_SYMBOL_AUDIO,   "Midia BT",    [](lv_event_t *) { buildBtMidia(); }, y); y += 20;
#endif
  redRow(body, "IR",              "Controle",    remoteCb,      y); y += 20;
  redRow(body, LV_SYMBOL_BELL,    "IR NEC",      necBtnCb,      y); y += 20;
  redRow(body, LV_SYMBOL_CHARGE,  "Lanterna",    lanternaCb,    y); y += 20;
  redRow(body, LV_SYMBOL_AUDIO,   "Beep",        beepBtnCb,     y); y += 20;
  redRow(body, LV_SYMBOL_WIFI,    "BLE Scan",    bleScanAction, y); y += 20;
  redRow(body, LV_SYMBOL_WIFI,    "Ping",        pingBtnCb,     y); y += 20;
  redRow(body, LV_SYMBOL_SHUFFLE, "Crono",       cronBtnCb,     y); y += 20;
  swFake = redRow(body, LV_SYMBOL_WARNING, "AP falso", fakeSwCb, y); y += 20;
  redRow(body, LV_SYMBOL_EDIT,    "Notas",       notasCb,       y); y += 20;
  redRow(body, LV_SYMBOL_BELL,    "Dado",        gameDiceCb,    y); y += 20;
#if HW_BLE
  redRow(body, "BB",              "BadBLE",      [](lv_event_t *) { buildBadBLE(); }, y, 18, 0x3A1A4A); y += 20;
  redRow(body, LV_SYMBOL_IMAGE,   "Espelho",     [](lv_event_t *) { if (mirrorOn()) uiMirrorStop(); else uiMirrorStart(); }, y, 18, 0x10304A); y += 20;
#endif
  lv_obj_add_event_cb(swFake, fakeSwCb, LV_EVENT_LONG_PRESSED, NULL);   // longo: troca nome
  fakeApApply();                    // pinta label on/off
}

// ---------------- NOTAS (lista rapida, persiste no SPIFFS) ----------------
static lv_obj_t *ntTa, *ntMsg;
static void ntSaveCb(lv_event_t *) {
  String t = lv_textarea_get_text(ntTa);
  fWrite("/nota.txt", t);
  if (lv_obj_is_valid(ntMsg)) lv_label_set_text(ntMsg, "salvo");
  toast("Nota salva");
}
static void ntClearCb(lv_event_t *) {
  fWrite("/nota.txt", "");
  if (lv_obj_is_valid(ntTa)) lv_textarea_set_text(ntTa, "");
  if (lv_obj_is_valid(ntMsg)) lv_label_set_text(ntMsg, "limpo");
  toast("Nota apagada");
}
static void notasCb(lv_event_t *) { buildNotas(); nav(scrNotas, LV_SCR_LOAD_ANIM_MOVE_LEFT); }
void buildNotas() {
  scrNotas = lv_obj_create(NULL);
  scrStyle(scrNotas);
  appHeader(scrNotas, "NOTAS");
  ntTa = uiTa(scrNotas, 2, 44, 124, 74);
  lv_textarea_set_one_line(ntTa, false);
  lv_obj_set_style_bg_color(ntTa, lv_color_hex(0x12151C), 0);
  lv_obj_set_style_text_font(ntTa, F_UI, 0);
  lv_obj_set_style_text_color(ntTa, REDMI_TXT, 0);
  lv_textarea_set_text(ntTa, fRead("/nota.txt").c_str());
  lv_textarea_set_placeholder_text(ntTa, "lista, ideias, enderecos...");
  uiBtn(scrNotas, "Salvar", ntSaveCb, 60, 20, 0x1A3B25, NULL);
  lv_obj_t *b1 = lv_obj_get_child(scrNotas, -1);
  if (b1) lv_obj_set_pos(b1, 0, 120);
  uiBtn(scrNotas, "Limpar", ntClearCb, 60, 20, 0x3B1B1B, NULL);
  lv_obj_t *b2 = lv_obj_get_child(scrNotas, -1);
  if (b2) lv_obj_set_pos(b2, 68, 120);
  ntMsg = uiLabel(scrNotas, "teclado abre sozinho ao tocar", 0, 142, F_SM, lv_color_to32(REDMI_DIM));
  lv_obj_set_width(ntMsg, 128);
  lv_obj_set_style_text_align(ntMsg, LV_TEXT_ALIGN_CENTER, 0);
}

// ---------------- DADO (jogo: rolar 1..6) ----------------
static lv_obj_t *dadoNum;
static void diceRollCb(lv_event_t *e) {
  if (lv_event_get_target(e) != lv_event_get_current_target(e)) return;   // ignora clique no botao voltar
  int n = esp_random() % 6 + 1;
  char b[4];
  snprintf(b, sizeof(b), "%d", n);
  if (lv_obj_is_valid(dadoNum)) {
    lv_label_set_text(dadoNum, b);
    beepShort(1800, 60);
  }
}
void buildDado() {
  scrDado = lv_obj_create(NULL);
  scrStyle(scrDado);
  appHeader(scrDado, "DADO");
  lv_obj_clear_flag(scrDado, LV_OBJ_FLAG_SCROLLABLE);
  dadoNum = lv_label_create(scrDado);
  lv_obj_set_style_text_font(dadoNum, F_BIG, 0);
  lv_obj_set_style_text_color(dadoNum, REDMI_ACC, 0);
  lv_label_set_text(dadoNum, "6");
  lv_obj_align(dadoNum, LV_ALIGN_TOP_MID, 0, 62);
  lv_obj_t *h = uiLabel(scrDado, "toque p/ rolar", 0, 144, F_SM, lv_color_to32(REDMI_DIM));
  lv_obj_set_width(h, 128);
  lv_obj_set_style_text_align(h, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_add_event_cb(scrDado, diceRollCb, LV_EVENT_CLICKED, NULL);
}

// ---------------- CONTROLE POR MARCA (TV / AR) ----------------
static lv_obj_t *scrBrands = nullptr, *scrRemoteBrand = nullptr;
static lv_obj_t *rbTtl, *rbGrid = nullptr;
static int  rbIdx = -1;          // indice na lista IR_TV / IR_AC
static bool rbAc = false;
static const IrKeyT *rbKey(int i) {
  const IrBrandT *l = rbAc ? IR_AC : IR_TV;
  int n = rbAc ? IR_AC_N : IR_TV_N;
  if (i < 0 || i >= n) return nullptr;
  return l[i].keys;
}
static int rbNKeys() {
  const IrBrandT *l = rbAc ? IR_AC : IR_TV;
  return rbAc ? IR_AC_N : IR_TV_N;
}
static void rbSend(const IrKeyT *k) {
  if (!irSendAsync(k->kind, k->a, k->c, k->raw, k->nraw)) toast("fila IR cheia");
  beepSpeaker();
}
static void rbBtnCb(lv_event_t *e) {
  const IrKeyT *k = (const IrKeyT *)lv_event_get_user_data(e);
  if (k) rbSend(k);
}
static void rbBtnLong(lv_event_t *e) {           // repete enquanto segura
  const IrKeyT *k = (const IrKeyT *)lv_event_get_user_data(e);
  for (int r = 0; r < 3 && k; r++) rbSend(k);
}
static void buildRemoteBrand(int i) {
  rbIdx = i;
  const IrBrandT *b = rbAc ? &IR_AC[i] : &IR_TV[i];
  scrRemoteBrand = lv_obj_create(NULL);
  scrStyle(scrRemoteBrand);
  appHeader(scrRemoteBrand, rbAc ? "AR COND." : "TV");
  char t[40];
  snprintf(t, sizeof(t), "%s (%d teclas)", b->name, b->n);
  rbTtl = uiLabel(scrRemoteBrand, t, 0, 44, F_SM, 0x8B90A0);
  lv_obj_set_width(rbTtl, 128);
  lv_obj_set_style_text_align(rbTtl, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_t *g = lv_obj_create(scrRemoteBrand);
  lv_obj_set_pos(g, 0, 58);
  lv_obj_set_size(g, 128, 100);
  lv_obj_set_style_bg_opa(g, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_opa(g, 0, 0);
  lv_obj_set_style_pad_all(g, 0, 0);
  lv_obj_set_scrollbar_mode(g, LV_SCROLLBAR_MODE_OFF);
  lv_obj_set_scroll_dir(g, LV_DIR_VER);
  int cols = rbAc ? 3 : 4;
  int bw = rbAc ? 40 : 30, bh = 30;
  for (int k = 0; k < b->n; k++) {
    int cx = (k % cols) * (128 / cols), cy = (k / cols) * (bh + 2);
    lv_obj_t *btn = lv_btn_create(g);
    lv_obj_set_pos(btn, cx + 1, cy);
    lv_obj_set_size(btn, bw, bh);
    lv_obj_set_style_bg_color(btn, REDMI_CARD, 0);
    lv_obj_set_style_radius(btn, 6, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_add_event_cb(btn, rbBtnCb, LV_EVENT_CLICKED, (void *)(uintptr_t)&b->keys[k]);
    lv_obj_add_event_cb(btn, rbBtnLong, LV_EVENT_LONG_PRESSED, (void *)(uintptr_t)&b->keys[k]);
    lv_obj_add_event_cb(btn, clickBeep, LV_EVENT_CLICKED, NULL);
    lv_obj_t *l = lv_label_create(btn);
    lv_label_set_text(l, b->keys[k].lab);
    lv_obj_set_style_text_font(l, F_SM, 0);
    lv_obj_set_style_text_color(l, REDMI_TXT, 0);
    lv_obj_set_width(l, bw - 2);
    lv_label_set_long_mode(l, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(l);
  }
  nav(scrRemoteBrand, LV_SCR_LOAD_ANIM_MOVE_LEFT);
}
static void rbRowCb(lv_event_t *e) {
  int i = (int)(intptr_t)lv_event_get_user_data(e);
  if (i < 0) return;
  buildRemoteBrand(i);
}
static void rbList(lv_obj_t *scr, bool ac) {
  rbAc = ac;
  int n = rbNKeys();
  lv_obj_t *bx = lv_obj_create(scr);
  lv_obj_set_pos(bx, 0, 60);
  lv_obj_set_size(bx, 128, 98);
  lv_obj_set_style_bg_opa(bx, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_opa(bx, 0, 0);
  lv_obj_set_style_pad_all(bx, 0, 0);
  lv_obj_set_scrollbar_mode(bx, LV_SCROLLBAR_MODE_OFF);
  lv_obj_set_scroll_dir(bx, LV_DIR_VER);
  int cols = 2, rowh = 26;
  for (int i = 0; i < n; i++) {
    const IrBrandT *b = ac ? &IR_AC[i] : &IR_TV[i];
    int cx = (i % cols) * 64, cy = (i / cols) * rowh;
    lv_obj_t *btn = lv_btn_create(bx);
    lv_obj_set_pos(btn, cx + 1, cy);
    lv_obj_set_size(btn, 61, rowh - 4);
    lv_obj_set_style_bg_color(btn, ac ? lv_color_hex(0x10304A) : REDMI_CARD, 0);
    lv_obj_set_style_radius(btn, 6, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_add_event_cb(btn, rbRowCb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    lv_obj_add_event_cb(btn, clickBeep, LV_EVENT_CLICKED, NULL);
    lv_obj_t *l = lv_label_create(btn);
    lv_label_set_text(l, b->name);
    lv_obj_set_style_text_font(l, F_SM, 0);
    lv_obj_set_style_text_color(l, REDMI_TXT, 0);
    lv_obj_set_width(l, 57);
    lv_label_set_long_mode(l, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(l);
  }
}
void buildBrands(bool ac) {
  scrBrands = lv_obj_create(NULL);
  scrStyle(scrBrands);
  appHeader(scrBrands, ac ? "AR COND." : "MARCAS TV");
  uiLabel(scrBrands, ac ? "escolha a marca do ar" : "escolha a marca da TV", 0, 44, F_SM, 0x676C7A);
  lv_obj_set_width(lv_obj_get_child(scrBrands, -1), 128);
  lv_obj_set_style_text_align(lv_obj_get_child(scrBrands, -1), LV_TEXT_ALIGN_CENTER, 0);
  rbList(scrBrands, ac);
  nav(scrBrands, LV_SCR_LOAD_ANIM_MOVE_LEFT);
}
// ---------------- CONTROLE UNIVERSAL IR (aprender qualquer protocolo + repetir) ----------------
// /rem.txt: uma linha por botao:
//   nome \t raw \t a \t c \t t0,t1,t2,...          (raw=1: timpos brutos; a/c = NEC)
static String rName[12];
static int  rAddr[12], rCmd[12];            // quadro NEC (addr/cmd) quando raw=0
static uint16_t rRawN[12];                  // qtd de tempos brutos
static uint16_t rRaw[12][24];               // tempos brutos (us), max 24 por botao
static bool rIsRaw[12];
static int  rN = 0;

static void rLoad() {
  rN = 0;
  String d = fRead("/rem.txt");
  int p = 0;
  while (rN < 12 && p < (int)d.length()) {
    int e = d.indexOf('\n', p); if (e < 0) e = d.length();
    String ln = d.substring(p, e); p = e + 1;
    ln.trim();
    if (!ln.length()) continue;
    int t1 = ln.indexOf('\t'); if (t1 < 0) continue;
    rName[rN] = ln.substring(0, t1);
    String mode = ln.substring(t1 + 1);
    int t2 = mode.indexOf('\t');
    String rest = (t2 > 0) ? mode.substring(t2 + 1) : "";
    String m   = (t2 > 0) ? mode.substring(0, t2) : mode;
    bool hasMode = (t2 > 0);
    if (m == "raw") {
      rIsRaw[rN] = true;
      int q1 = rest.indexOf('\t'), p2 = 0;
      if (q1 < 0) continue;
      rAddr[rN] = rest.substring(0, q1).toInt();
      rCmd[rN]  = rest.substring(q1 + 1).toInt();
      String times = (rest.indexOf('\t', q1 + 1) > 0)
                       ? rest.substring(rest.indexOf('\t', q1 + 1) + 1) : "";
      rRawN[rN] = 0;
      while (rRawN[rN] < 24 && p2 < (int)times.length()) {
        int c = times.indexOf(',', p2);
        String v = times.substring(p2, c < 0 ? times.length() : c);
        v.trim();
        if (v.length()) rRaw[rN][rRawN[rN]++] = (uint16_t)v.toInt();
        p2 = (c < 0) ? times.length() : c + 1;
      }
    } else if (m == "nec" && hasMode) {   // formato novo: nome / nec / addr / cmd
      rIsRaw[rN] = false;
      int q1 = rest.indexOf('\t');
      if (q1 < 0) continue;
      rAddr[rN] = rest.substring(0, q1).toInt();
      rCmd[rN]  = rest.substring(q1 + 1).toInt();
      rRawN[rN] = 0;
    } else {                              // formato antigo: nome / addr / cmd
      rIsRaw[rN] = false;
      int s1 = ln.indexOf('\t');
      int s2 = ln.indexOf('\t', s1 + 1);
      if (s2 < 0) continue;
      rAddr[rN] = ln.substring(s1 + 1, s2).toInt();
      rCmd[rN]  = ln.substring(s2 + 1).toInt();
      rRawN[rN] = 0;
    }
    rN++;
  }
}
static void rSave() {
  String d;
  for (int i = 0; i < rN; i++) {
    if (rIsRaw[i]) {
      d += rName[i] + "\traw\t" + String(rAddr[i]) + "\t" + String(rCmd[i]) + "\t";
      for (int k = 0; k < rRawN[i]; k++) {
        d += String(rRaw[i][k]);
        if (k + 1 < rRawN[i]) d += ",";
      }
      d += "\n";
    } else {
      d += rName[i] + "\tnec\t" + String(rAddr[i]) + "\t" + String(rCmd[i]) + "\n";
    }
  }
  fWrite("/rem.txt", d);
}
static void remoteSendCb(lv_event_t *e) {
  int i = (int)(intptr_t)lv_event_get_user_data(e);
  if (i < 0 || i >= rN) return;
  if (rIsRaw[i] && rRawN[i] > 3) irSendAsync(IR_JOB_RAW, 0, 0, (const uint16_t *)rRaw[i], rRawN[i]);
  else irSendAsync(0, (uint8_t)rAddr[i], (uint8_t)rCmd[i]);
  beepSpeaker();
  toast("enviado");
}
static void remoteDelCb(lv_event_t *e) {
  int i = (int)(intptr_t)lv_event_get_user_data(e);
  if (i < 0 || i >= rN) return;
  for (int j = i; j < rN - 1; j++) {
    rName[j] = rName[j + 1]; rAddr[j] = rAddr[j + 1]; rCmd[j] = rCmd[j + 1];
    rRawN[j] = rRawN[j + 1]; rIsRaw[j] = rIsRaw[j + 1];
    for (int k = 0; k < 24; k++) rRaw[j][k] = rRaw[j + 1][k];
  }
  rN--;
  rSave();
  toast("apagado");
  // recria a tela (indices das linhas restantes precisam ser remapeados)
  lv_timer_t *t = lv_timer_create([](lv_timer_t *tt) {
    lv_timer_del(tt);
    buildRemote();
    nav(scrRemote, LV_SCR_LOAD_ANIM_MOVE_LEFT);
  }, 40, NULL);
  if (t) t->repeat_count = 1;
}
static void remoteRow(lv_obj_t *parent, int i) {
  lv_obj_t *b = lv_btn_create(parent);
  lv_obj_set_pos(b, 0, i * 17);
  lv_obj_set_size(b, 118, 16);
  lv_obj_set_style_bg_color(b, REDMI_CARD, 0);
  lv_obj_set_style_radius(b, 6, 0);
  lv_obj_set_style_shadow_width(b, 0, 0);
  lv_obj_add_event_cb(b, remoteSendCb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
  lv_obj_add_event_cb(b, remoteDelCb, LV_EVENT_LONG_PRESSED, (void *)(intptr_t)i);
  lv_obj_add_event_cb(b, clickBeep, LV_EVENT_CLICKED, NULL);
  char b2[30];
  if (rIsRaw[i])
    snprintf(b2, sizeof(b2), "%s  raw %dt", rName[i].c_str(), rRawN[i]);
  else
    snprintf(b2, sizeof(b2), "%s  %02X %02X", rName[i].c_str(), rAddr[i], rCmd[i]);
  lv_obj_t *l = lv_label_create(b);
  lv_label_set_text(l, b2);
  lv_obj_set_style_text_font(l, F_SM, 0);
  lv_obj_set_style_text_color(l, REDMI_TXT, 0);
  lv_obj_center(l);
}
static uint32_t irLearnT0 = 0;                 // timeout do aprendizado
static void remoteLearnCb(lv_event_t *) {
  if (irLearnArm) { irLearnArm = false; irRawReset(); toast("aprender cancelado"); return; }
  irLearnArm = true;
  irLearnT0 = millis();
  irRawReset();
  toast("aperte o botao do controle...");
}
static void remoteCb(lv_event_t *) { buildRemote(); nav(scrRemote, LV_SCR_LOAD_ANIM_MOVE_LEFT); }
void buildRemote() {
  scrRemote = lv_obj_create(NULL);
  scrStyle(scrRemote);
  appHeader(scrRemote, "CONTROLE IR");
  rLoad();
  irRxSetup();
  lv_obj_t *h = uiLabel(scrRemote, "toque envia - longo apaga", 0, 42, F_SM, lv_color_to32(REDMI_DIM));
  lv_obj_set_width(h, 128);
  lv_obj_set_style_text_align(h, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_t *ba = uiBtn(scrRemote, "Aprender", remoteLearnCb, 118, 16, 0x1A3B25, NULL);
  if (ba) lv_obj_set_pos(ba, 2, 58);
  // lista rolavel de botoes salvos
  lv_obj_t *lst = lv_obj_create(scrRemote);
  lv_obj_set_pos(lst, 4, 76);
  lv_obj_set_size(lst, 120, 80);
  lv_obj_set_style_bg_opa(lst, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_opa(lst, 0, 0);
  lv_obj_set_style_pad_all(lst, 0, 0);
  lv_obj_set_scroll_dir(lst, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(lst, LV_SCROLLBAR_MODE_AUTO);
  if (!rN) {
    lv_obj_t *e = uiLabel(lst, "nenhum botao salvo.\ntoque em Aprender.", 0, 2, F_SM, lv_color_to32(REDMI_DIM));
    lv_obj_set_width(e, 116);
    lv_obj_set_style_text_align(e, LV_TEXT_ALIGN_CENTER, 0);
  }
  for (int i = 0; i < rN; i++) remoteRow(lst, i);
}


// ---------------- MOEDA (cara ou coroa com animacao de giro) ----------------
static lv_obj_t *moedaCoin = nullptr;   // objeto circular da moeda
static lv_obj_t *moedaFace = nullptr;   // "?" dentro da moeda
static lv_obj_t *moedaRes = nullptr;    // resultado (CARA/COROA) abaixo da moeda
static lv_anim_t moedaAnim;             // animacao do giro (600ms)
static bool  moedaResult = false;       // true=cara, false=coroa
static bool  moedaAnimating = false;    // trava toque duplo
static bool  moedaShown = false;        // resultado ja revelado neste giro

static void moedaAnimExec(void *a, int32_t v) {
  // v: 0..100 -> largura da moeda: 44 -> 0 (meio) -> 44 (simula giro 3D)
  int w = (v <= 50) ? (44 * (50 - v) / 50) : (44 * (v - 50) / 50);
  if (lv_obj_is_valid(moedaCoin)) {
    lv_obj_set_size(moedaCoin, w, 44);
    lv_obj_set_pos(moedaCoin, (128 - w) / 2, 44);   // mantem centralizada
  }
  // no ponto mais fino (moeda de lado) revelamos o resultado
  if (!moedaShown && v >= 48 && v <= 52 && lv_obj_is_valid(moedaRes)) {
    moedaShown = true;
    lv_label_set_text(moedaRes, moedaResult ? "CARA!" : "COROA!");
    lv_obj_set_style_text_color(moedaRes, moedaResult ? REDMI_ACC : lv_color_hex(0xFFA726), 0);
  }
}

static void moedaAnimEnd(lv_anim_t *a) {
  moedaAnimating = false;
  if (lv_obj_is_valid(moedaCoin)) {
    lv_obj_set_size(moedaCoin, 44, 44);
    lv_obj_set_pos(moedaCoin, 42, 44);
  }
}

static void coinFlipCb(lv_event_t *e) {
  if (lv_event_get_target(e) != lv_event_get_current_target(e)) return;   // evita duplo disparo pelo bubble
  if (moedaAnimating) return;               // evita toque durante o giro
  moedaAnimating = true;
  moedaShown = false;
  moedaResult = esp_random() & 1;
  if (lv_obj_is_valid(moedaFace)) lv_label_set_text(moedaFace, "?");
  if (lv_obj_is_valid(moedaRes)) lv_label_set_text(moedaRes, "girando...");
  lv_anim_init(&moedaAnim);
  lv_anim_set_var(&moedaAnim, moedaCoin);
  lv_anim_set_exec_cb(&moedaAnim, moedaAnimExec);
  lv_anim_set_time(&moedaAnim, 600);        // 0.6s de giro
  lv_anim_set_values(&moedaAnim, 0, 100);
  lv_anim_set_path_cb(&moedaAnim, lv_anim_path_ease_in_out);
  lv_anim_set_ready_cb(&moedaAnim, moedaAnimEnd);
  lv_anim_start(&moedaAnim);
  beepShort(2000, 80);
}

static void moedaCb(lv_event_t *) { buildMoeda(); nav(scrMoeda, LV_SCR_LOAD_ANIM_MOVE_LEFT); }

void buildMoeda() {
  scrMoeda = lv_obj_create(NULL);
  scrStyle(scrMoeda);
  appHeader(scrMoeda, "MOEDA");
  lv_obj_clear_flag(scrMoeda, LV_OBJ_FLAG_SCROLLABLE);
  moedaAnimating = false;
  moedaShown = false;

  // moeda circular dourada (gira ao tocar)
  moedaCoin = lv_obj_create(scrMoeda);
  lv_obj_set_size(moedaCoin, 44, 44);
  lv_obj_set_style_bg_color(moedaCoin, lv_color_hex(0xFFD700), 0);   // dourado
  lv_obj_set_style_radius(moedaCoin, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_border_width(moedaCoin, 2, 0);
  lv_obj_set_style_border_color(moedaCoin, lv_color_hex(0xFFA726), 0);
  lv_obj_set_pos(moedaCoin, 42, 44);

  // simbolo dentro da moeda
  moedaFace = lv_label_create(moedaCoin);
  lv_label_set_text(moedaFace, "?");
  lv_obj_set_style_text_font(moedaFace, F_MED, 0);
  lv_obj_set_style_text_color(moedaFace, REDMI_BG, 0);
  lv_obj_center(moedaFace);

  // resultado abaixo (texto nao precisa caber dentro da moeda)
  moedaRes = uiLabel(scrMoeda, "toque p/ jogar", 0, 96, F_MED, lv_color_to32(REDMI_ACC));
  lv_obj_set_width(moedaRes, 128);
  lv_obj_set_style_text_align(moedaRes, LV_TEXT_ALIGN_CENTER, 0);

  lv_obj_t *h = uiLabel(scrMoeda, "cara ou coroa", 0, 128, F_SM, lv_color_to32(REDMI_DIM));
  lv_obj_set_width(h, 128);
  lv_obj_set_style_text_align(h, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_add_event_cb(moedaCoin, coinFlipCb, LV_EVENT_CLICKED, NULL);   // tocar na moeda tambem gira
  lv_obj_add_event_cb(scrMoeda, coinFlipCb, LV_EVENT_CLICKED, NULL);
}

// ============================================================
//  MATEMATICA + CALCULADORA + NIVEL/BOLHA
// ============================================================
static lv_obj_t *calcDisp, *calcSciBtn, *calcSqrtBtn, *calcPowBtn;
static char calcNum[16] = "0";
static double calcAcc = 0;
static char calcOp = 0;
static bool calcFresh = true;
static bool calcSci = false;   // modo cientifico
static void calcShow(const char *s) {
  if (!lv_obj_is_valid(calcDisp)) return;
  lv_label_set_text(calcDisp, s);
  // fonte encolhe conforme o numero cresce (128px de largura)
  int n = strlen(s);
  lv_obj_set_style_text_font(calcDisp, n > 11 ? F_UI : F_MED, 0);
}
static void calcNum2Str(double v, char *o, size_t n) {
  char b[24];
  snprintf(b, sizeof(b), "%.4f", v);
  // corta zeros no fim
  char *pt = strrchr(b, '.');
  if (pt) {
    char *end = b + strlen(b) - 1;
    while (end > pt && *end == '0') *end-- = 0;
    if (*end == '.') *end = 0;
  }
  snprintf(o, n, "%s", b);
}
static void calcApplyOp() {
  double b = atof(calcNum);
  if (calcOp == 0) { calcAcc = b; return; }
  switch (calcOp) {
    case '+': calcAcc += b; break;
    case '-': calcAcc -= b; break;
    case '*': calcAcc *= b; break;
    case '/':
      if (b == 0) {                       // divisao por zero -> erro visivel
        snprintf(calcNum, sizeof(calcNum), "Er");
        calcAcc = 0; calcOp = 0; calcFresh = true;
        return;
      }
      calcAcc /= b;
      break;
    case '^': calcAcc = pow(calcAcc, b); break;
  }
  if (calcAcc > 99999999 || calcAcc < -99999999) calcAcc = 0;
  char out[24];
  calcNum2Str(calcAcc, out, sizeof(out));
  snprintf(calcNum, sizeof(calcNum), "%s", out);
}
static void calcKeyCb(lv_event_t *e) {
  lv_obj_t *bm = lv_event_get_target(e);
  uint16_t bi = lv_btnmatrix_get_selected_btn(bm);
  if (bi == 0xFFFF) return;
  const char *tx = lv_btnmatrix_get_btn_text(bm, bi);
  if (!tx || !tx[0]) return;
  if (!strcmp(tx, "Sci")) { calcSci = !calcSci; toast(calcSci ? "Cientifico ON: r=raiz ^=pot" : "Basico"); return; }
  if (tx[0] >= '0' && tx[0] <= '9') {
    if (calcFresh) { snprintf(calcNum, sizeof(calcNum), "%c", tx[0]); calcFresh = false; }
    else if (strlen(calcNum) < 11) { strncat(calcNum, tx, 1); }
    calcShow(calcNum);
  } else if (tx[0] == '.') {
    if (calcFresh) { snprintf(calcNum, sizeof(calcNum), "0"); calcFresh = false; }
    if (!strchr(calcNum, '.') && strlen(calcNum) < 11) { strncat(calcNum, ".", 1); }
    calcShow(calcNum);
  } else if (tx[0] == 'C') {                    // limpa tudo (inclusive acumulado)
    snprintf(calcNum, sizeof(calcNum), "0");
    calcAcc = 0; calcOp = 0; calcFresh = true;
    calcShow(calcNum);
  } else if (tx[0] == 'r') {                    // raiz quadrada do visor
    double v = atof(calcNum);
    if (v < 0) snprintf(calcNum, sizeof(calcNum), "Er");
    else calcNum2Str(sqrt(v), calcNum, sizeof(calcNum));
    calcShow(calcNum); calcFresh = true;
  } else if (tx[0] == '=') {
    calcApplyOp();
    calcOp = 0;
    calcFresh = true;
    calcShow(calcNum);
  } else {                                      // + - * / ^
    if (!calcFresh) calcApplyOp();
    else calcAcc = atof(calcNum);
    calcOp = tx[0];
    calcFresh = true;
    calcShow(calcNum);
  }
}
void buildCalc() {
  scrCalc = lv_obj_create(NULL);
  scrStyle(scrCalc);
  appHeader(scrCalc, "CALC");
  calcSciBtn = calcSqrtBtn = calcPowBtn = nullptr;   // nao guardar ponteiros de tela apagada
  // visor em card arredondado
  lv_obj_t *card = lv_obj_create(scrCalc);
  lv_obj_set_pos(card, 4, 42);
  lv_obj_set_size(card, 120, 26);
  lv_obj_set_style_bg_color(card, REDMI_CARD, 0);
  lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(card, 8, 0);
  lv_obj_set_style_border_width(card, 0, 0);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_CLICKABLE);
  calcDisp = lv_label_create(card);
  lv_obj_set_width(calcDisp, 112);
  lv_obj_set_style_text_font(calcDisp, F_MED, 0);
  lv_obj_set_style_text_color(calcDisp, REDMI_TXT, 0);
  lv_obj_set_style_text_align(calcDisp, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_align(calcDisp, LV_ALIGN_RIGHT_MID, -2, 0);
  lv_label_set_text(calcDisp, calcNum);
  // teclado 4x5: teclas grandes; r=raiz, ^=potencia, Sci alterna modo
  static const char *calcMap[] = {
    "7", "8", "9", "/", "\n",
    "4", "5", "6", "*", "\n",
    "1", "2", "3", "-", "\n",
    "0", ".", "+", "=", "\n",
    "C", "r", "^", "Sci", "\n",
    NULL
  };
  lv_obj_t *bm = lv_btnmatrix_create(scrCalc);
  lv_obj_set_pos(bm, 0, 70);
  lv_obj_set_size(bm, 128, 88);
  lv_btnmatrix_set_map(bm, calcMap);
  lv_obj_set_style_pad_all(bm, 2, 0);
  lv_obj_set_style_pad_row(bm, 2, 0);
  lv_obj_set_style_pad_column(bm, 2, 0);
  lv_obj_set_style_text_font(bm, F_SM, 0);
  lv_obj_set_style_bg_opa(bm, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_opa(bm, 0, 0);
  lv_obj_set_style_bg_color(bm, lv_color_hex(0x161A22), LV_PART_ITEMS);
  lv_obj_set_style_bg_opa(bm, LV_OPA_COVER, LV_PART_ITEMS);
  lv_obj_set_style_radius(bm, 6, LV_PART_ITEMS);
  lv_obj_set_style_border_width(bm, 0, LV_PART_ITEMS);
  lv_obj_set_style_text_color(bm, REDMI_TXT, LV_PART_ITEMS);
  lv_obj_set_style_bg_color(bm, REDMI_ACC, LV_PART_ITEMS | LV_STATE_PRESSED);
  lv_obj_set_style_text_color(bm, lv_color_hex(0x0B0D12), LV_PART_ITEMS | LV_STATE_PRESSED);
  lv_obj_add_event_cb(bm, calcKeyCb, LV_EVENT_VALUE_CHANGED, NULL);
}
static void matCalcCb(lv_event_t *) {
  buildCalc();
  nav(scrCalc, LV_SCR_LOAD_ANIM_MOVE_LEFT);
}
static void matNivelCb(lv_event_t *) { buildLevel(false); nav(scrLevel, LV_SCR_LOAD_ANIM_MOVE_LEFT); }
static void matBolhaCb(lv_event_t *) { buildLevel(true);  nav(scrLevel, LV_SCR_LOAD_ANIM_MOVE_LEFT); }
void buildMat() {
  scrMat = lv_obj_create(NULL);
  scrStyle(scrMat);
  appHeader(scrMat, "MATEMATICA");
  lv_obj_t *body = appBody(scrMat);
  uiRow(body, "Calculadora", matCalcCb, 0, 26);
  uiRow(body, "Nivel (de lado)", matNivelCb, 34, 26);
  uiRow(body, "Bolha (deitado)", matBolhaCb, 68, 26);
  // Ajuste de espaco para caber 3 linhas no corpo (112px)
  lv_obj_set_size(body, 128, 100);
}

// ============================================================
//  NIVEL + BOLHA (MPU)
// ============================================================
static lv_obj_t *lvlRing, *lvlBall, *lvlNote;
static bool lvlBolha = false;
void buildLevel(bool bolha) {
  lvlBolha = bolha;
  scrLevel = lv_obj_create(NULL);
  scrStyle(scrLevel);
  appHeader(scrLevel, bolha ? "BOLHA" : "NIVEL");
  lvlRing = lv_obj_create(scrLevel);
  lv_obj_set_pos(lvlRing, 26, 52);
  lv_obj_set_size(lvlRing, 76, 76);
  lv_obj_set_style_bg_color(lvlRing, REDMI_CARD, 0);
  lv_obj_set_style_bg_opa(lvlRing, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(lvlRing, 40, 0);
  lv_obj_set_style_border_color(lvlRing, REDMI_ACC, 0);
  lv_obj_set_style_border_width(lvlRing, 2, 0);
  lv_obj_clear_flag(lvlRing, LV_OBJ_FLAG_SCROLLABLE);
  // cruz de referencia
  lv_obj_t *ch1 = lv_obj_create(lvlRing);
  lv_obj_set_size(ch1, 40, 1);
  lv_obj_set_style_bg_color(ch1, REDMI_DIM, 0);
  lv_obj_set_style_border_opa(ch1, 0, 0);
  lv_obj_set_style_radius(ch1, 0, 0);
  lv_obj_align(ch1, LV_ALIGN_CENTER, 0, 0);
  lv_obj_t *ch2 = lv_obj_create(lvlRing);
  lv_obj_set_size(ch2, 1, 40);
  lv_obj_set_style_bg_color(ch2, REDMI_DIM, 0);
  lv_obj_set_style_border_opa(ch2, 0, 0);
  lv_obj_set_style_radius(ch2, 0, 0);
  lv_obj_align(ch2, LV_ALIGN_CENTER, 0, 0);
  lvlBall = lv_obj_create(lvlRing);
  lv_obj_set_size(lvlBall, 14, 14);
  lv_obj_set_style_radius(lvlBall, 7, 0);
  lv_obj_set_style_bg_color(lvlBall, REDMI_ACC, 0);
  lv_obj_set_style_bg_opa(lvlBall, LV_OPA_COVER, 0);
  lv_obj_set_style_border_opa(lvlBall, 0, 0);
  lv_obj_clear_flag(lvlBall, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_align(lvlBall, LV_ALIGN_CENTER, 0, 0);
  lvlNote = uiLabel(scrLevel, gMpuOk ? "aguardando..." : "MPU ausente!",
                    0, 134, F_UI, lv_color_to32(gMpuOk ? REDMI_DIM : REDMI_ERR));
  lv_obj_set_width(lvlNote, 128);
  lv_obj_set_style_text_align(lvlNote, LV_TEXT_ALIGN_CENTER, 0);
}

// ============================================================
//  SAUDE (passos com MPU)
// ============================================================
static lv_obj_t *helArcP, *helArcK, *helArcC, *helSteps, *helK, *helS, *helC;
static void helResetCb(lv_event_t *) {
  cfg.steps = 0;
  char b[12];
  cfgPut("steps", ltoa(cfg.steps, b, 10));   // nao reescreve a config inteira
  if (lv_obj_is_valid(helArcP)) lv_arc_set_value(helArcP, 0);
  if (lv_obj_is_valid(helArcK)) lv_arc_set_value(helArcK, 0);
  if (lv_obj_is_valid(helArcC)) lv_arc_set_value(helArcC, 0);
  if (lv_obj_is_valid(helSteps) && gMpuOk) lv_label_set_text(helSteps, "0");
  if (lv_obj_is_valid(helK)) lv_label_set_text(helK, "0km");
  if (lv_obj_is_valid(helS)) lv_label_set_text(helS, "0");
  if (lv_obj_is_valid(helC)) lv_label_set_text(helC, "0cal");
  toast("Contador zerado");
}
static void helRing(lv_obj_t **slot, lv_obj_t *parent, int cx, int cy, int d, int range, int val, uint32_t col) {
  *slot = lv_arc_create(parent);
  lv_obj_set_pos(*slot, cx - d / 2, cy - d / 2);
  lv_obj_set_size(*slot, d, d);
  lv_arc_set_bg_angles(*slot, 0, 360);
  lv_arc_set_range(*slot, 0, range);
  lv_arc_set_value(*slot, val);
  lv_obj_set_style_bg_color(*slot, lv_color_hex(0x202839), LV_PART_MAIN);   // trilho
  lv_obj_set_style_bg_opa(*slot, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_arc_color(*slot, lv_color_hex(0x202839), LV_PART_MAIN);
  lv_obj_set_style_arc_color(*slot, lv_color_hex(col), LV_PART_INDICATOR);   // progresso
  lv_obj_set_style_arc_width(*slot, 6, LV_PART_INDICATOR);
  lv_obj_set_style_arc_width(*slot, 6, LV_PART_MAIN);
  lv_obj_remove_style(*slot, NULL, LV_PART_KNOB);
  lv_obj_clear_flag(*slot, LV_OBJ_FLAG_CLICKABLE);
}
void buildSaude() {
  scrSaude = lv_obj_create(NULL);
  scrStyle(scrSaude);
  appHeader(scrSaude, "SAUDE");
  long s = cfg.steps;
  int kmC = (int)constrain((long)(s * 0.0007f * 100), 0, 700);   // 0..7,00 km
  int calC = (int)constrain((long)(s * 0.04f), 0, 400);          // 0..400 cal
  int stC = (int)constrain(s, 0, 10000);
  // 3 aneis concentricos (mesmas cores da face padrao / foto 2)
  // passos (externo, lima 0xCCFF00) 0..10000 | km (medio, verde 0x00E676) | cal (interno, rosa 0xFF3B5C)
  helRing(&helArcP, scrSaude, 64, 78, 84, 10000, stC, 0xCCFF00);
  helRing(&helArcK, scrSaude, 64, 78, 60, 700,   kmC, 0x00E676);
  helRing(&helArcC, scrSaude, 64, 78, 38, 400,   calC, 0xFF3B5C);
  // centro: passos + "icone" de atividade
  helSteps = lv_label_create(scrSaude);
  char b[10];
  if (!gMpuOk) snprintf(b, sizeof(b), "sem MPU");
  else         snprintf(b, sizeof(b), "%lu", (unsigned long)s);
  lv_label_set_text(helSteps, b);
  lv_obj_set_style_text_font(helSteps, gMpuOk ? F_UI : F_SM, 0);
  lv_obj_set_style_text_color(helSteps, gMpuOk ? REDMI_TXT : REDMI_ERR, 0);
  lv_obj_align(helSteps, LV_ALIGN_CENTER, 0, 0);
  // legenda colorida (uma linha por metrica, cor do anel)
  char kb[16]; snprintf(kb, sizeof(kb), "%.2fkm", s * 0.0007f);
  helK = uiLabel(scrSaude, kb, 6, 128, F_SM, 0x00E676);
  lv_obj_set_width(helK, 42);
  lv_label_set_long_mode(helK, LV_LABEL_LONG_DOT);
  char sb2[12]; snprintf(sb2, sizeof(sb2), "%ld", (long)s);
  helS = uiLabel(scrSaude, sb2, 50, 128, F_SM, 0xCCFF00);
  lv_obj_set_width(helS, 40);
  lv_label_set_long_mode(helS, LV_LABEL_LONG_DOT);
  char cb2[12]; snprintf(cb2, sizeof(cb2), "%ldcal", (long)(s * 0.04f));
  helC = uiLabel(scrSaude, cb2, 92, 128, F_SM, 0xFF3B5C);
  lv_obj_set_width(helC, 36);
  lv_label_set_long_mode(helC, LV_LABEL_LONG_DOT);
  lv_obj_t *zb = uiBtn(scrSaude, "Zerar", helResetCb, 50, 16, 0x3B1B1B, NULL);
  lv_obj_set_pos(zb, 39, 144);
}

// ============================================================
//  GOOGLE (DDG lite + Openverse): abas Pesquisa/Imagens + leitor de site
// ============================================================
static lv_obj_t *goTa, *goSpin, *goTab0, *goTab1, *goTabU;
static lv_obj_t *goBoxR, *goSnip, *goSnipTt, *goSnipL, *goSnipHost, *goNoneL, *goRow[5], *goSrc;
static lv_obj_t *goBoxI, *goImg[4], *goImgLbl;
static lv_obj_t *scrHtml = nullptr, *htBox, *htTxt;
static int goTabNow = 0;                 // 0=Pesquisa 1=Imagens
static String gGoUrl[10];                // 0..9 urls dos resultados (paginacao)
static String gGoUrlTtl;                 // titulo do 1º resultado (card do topo)
static String gGoQ;                      // ultima busca
static int goPage = 0;                   // pagina atual (0..1)
static lv_obj_t *goBtnPrev, *goBtnNext;  // botoes de paginacao


static void goImgCb(bool, const String &out) {   // miniaturas chegaram
  if (goSpin && lv_obj_is_valid(goSpin)) lv_obj_add_flag(goSpin, LV_OBJ_FLAG_HIDDEN);
  int n = 0;
  if (out.startsWith("IMG")) n = out.substring(3).toInt();   // "IMG2/4" -> 2
  for (int i = 0; i < 4; i++) {
    if (!lv_obj_is_valid(goImg[i])) continue;
    if (i < n && goPix[i]) {
      lv_canvas_set_buffer(goImg[i], goPix[i], IMG_W, IMG_H, LV_IMG_CF_TRUE_COLOR);
      lv_obj_clear_flag(goImg[i], LV_OBJ_FLAG_HIDDEN);
      lv_obj_invalidate(goImg[i]);
    } else lv_obj_add_flag(goImg[i], LV_OBJ_FLAG_HIDDEN);
  }
  if (lv_obj_is_valid(goImgLbl))
    lv_label_set_text(goImgLbl, n ? ("carregadas " + String(n)).c_str()
                                  : "nenhuma imagem encontrada");
  if (!n) {
    String m = out.startsWith("IMG")
      ? ("imagens falharam " + out.substring(3) + (gHttpWhy.length() ? " " + gHttpWhy : ""))
      : out;
    toast(m.length() ? m.c_str() : "sem imagens");
  }
}
static void goSearchCb(lv_event_t *);   // forward declaracao (definida abaixo em buildGoogle)
static void goPrevPageCb(lv_event_t *) { if (goPage > 0) { goPage--; goSearchCb(nullptr); } }
static void goNextPageCb(lv_event_t *) { goPage++; goSearchCb(nullptr); }
static void goShowTab(int t) {
  goTabNow = t;
  bool res = (t == 0);
  if (lv_obj_is_valid(goBoxR)) {
    if (res) lv_obj_clear_flag(goBoxR, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(goBoxR, LV_OBJ_FLAG_HIDDEN);
  }
  if (lv_obj_is_valid(goBoxI)) {
    if (res) lv_obj_add_flag(goBoxI, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_clear_flag(goBoxI, LV_OBJ_FLAG_HIDDEN);
  }
  if (lv_obj_is_valid(goSrc)) {
    if (res) lv_obj_add_flag(goSrc, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_clear_flag(goSrc, LV_OBJ_FLAG_HIDDEN);
  }
  // botoes de paginacao so na aba Pesquisa
  if (lv_obj_is_valid(goBtnPrev)) {
    if (res && goPage > 0) lv_obj_clear_flag(goBtnPrev, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(goBtnPrev, LV_OBJ_FLAG_HIDDEN);
  }
  if (lv_obj_is_valid(goBtnNext)) {
    if (res) lv_obj_clear_flag(goBtnNext, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(goBtnNext, LV_OBJ_FLAG_HIDDEN);
  }
  lv_color_t on = REDMI_ACC, off = lv_color_hex(0x8B90A0);
  if (lv_obj_is_valid(goTab0)) lv_obj_set_style_text_color(lv_obj_get_child(goTab0, 0), res ? on : off, 0);
  if (lv_obj_is_valid(goTab1)) lv_obj_set_style_text_color(lv_obj_get_child(goTab1, 0), res ? off : on, 0);
  if (lv_obj_is_valid(goTabU)) lv_obj_set_pos(goTabU, res ? 0 : 66, 81);
}
static void goTabCb(lv_event_t *e) {
  int t = (int)(intptr_t)lv_event_get_user_data(e);
  if (t == goTabNow) return;
  goShowTab(t);
  if (t == 1 && gGoQ.length()) {         // trocou p/ Imagens: busca automática
    if (gNetBusy) { toast("Aguarde..."); return; }
    if (WiFi.status() != WL_CONNECTED) { toast("Sem Wi-Fi"); return; }
    gNetCb = [](bool ok, const String &out) { goImgCb(ok, out); };
    netRun(T_IMGX, gGoQ);
    if (goSpin && lv_obj_is_valid(goSpin)) lv_obj_clear_flag(goSpin, LV_OBJ_FLAG_HIDDEN);
    if (lv_obj_is_valid(goImgLbl)) lv_label_set_text(goImgLbl, "buscando...");
    toast("Buscando imagens...");
  }
}
static void goHtmlCb(bool, const String &out) {
  if (!lv_obj_is_valid(scrHtml)) {        // (re)constroi o leitor se a tela foi liberada
    scrHtml = lv_obj_create(NULL);
    scrStyle(scrHtml);
    appHeader(scrHtml, "PAGINA");
    htBox = lv_obj_create(scrHtml);
    lv_obj_set_pos(htBox, 0, 42);
    lv_obj_set_size(htBox, 128, 96);
    lv_obj_set_style_bg_color(htBox, lv_color_hex(0x12151C), 0);
    lv_obj_set_style_bg_opa(htBox, LV_OPA_COVER, 0);
    lv_obj_set_style_border_opa(htBox, 0, 0);
    lv_obj_set_style_pad_all(htBox, 2, 0);
    lv_obj_set_scrollbar_mode(htBox, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(htBox, LV_DIR_VER);
    htTxt = lv_label_create(htBox);
    lv_obj_set_width(htTxt, 120);
    lv_label_set_long_mode(htTxt, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(htTxt, F_SM, 0);
    lv_obj_set_style_text_color(htTxt, REDMI_TXT, 0);
    lv_obj_t *bb = uiBtn(scrHtml, "Voltar", [](lv_event_t *e) {
      buildGoogle();                                     // Google foi liberado no forward: recria
      nav(scrGoogle, LV_SCR_LOAD_ANIM_MOVE_RIGHT);
      (void)e;
    }, 60, 18, 0x161A22, NULL);
    lv_obj_set_pos(bb, 34, 142);
  }
  if (lv_obj_is_valid(htTxt)) lv_label_set_text(htTxt, out.length() ? out.c_str() : "nao deu p/ ler esta pagina");
  if (lv_obj_is_valid(htBox)) lv_obj_scroll_to_y(htBox, 0, LV_ANIM_OFF);
  nav(scrHtml, LV_SCR_LOAD_ANIM_MOVE_LEFT);
}
static void goOpenCb(lv_event_t *e) {    // abre o site do resultado no leitor
  int i = (int)(intptr_t)lv_event_get_user_data(e);
  if (i < 0 || i > 4 || !gGoUrl[i].length()) return;
  if (gNetBusy) { toast("Aguarde..."); return; }
  if (WiFi.status() != WL_CONNECTED) { toast("Sem Wi-Fi"); return; }
  gNetCb = goHtmlCb;
  netRun(T_HTML, gGoUrl[i]);
  if (lv_obj_is_valid(htTxt)) lv_label_set_text(htTxt, "carregando...");
  toast("Abrindo pagina...");
}
static void goSearchNetCb(bool ok, const String &out) {
  if (goSpin && lv_obj_is_valid(goSpin)) lv_obj_add_flag(goSpin, LV_OBJ_FLAG_HIDDEN);
  if (!ok || !out.length() || out.indexOf('\x03') < 0) {
    toast(out.length() ? out.c_str() : "sem resultados");
    return;
  }
  if (lv_obj_is_valid(goNoneL)) lv_obj_add_flag(goNoneL, LV_OBJ_FLAG_HIDDEN);
  // limpa linhas da busca anterior (card do topo e aviso ficam)
  for (int i = 0; i < 5; i++)
    if (lv_obj_is_valid(goRow[i])) { lv_obj_del(goRow[i]); goRow[i] = nullptr; }
  int sep = out.indexOf('\x03');
  String snip = out.substring(0, sep), rest = out.substring(sep + 1);
  if (snip.length() > 130) {                    // cabe no card do topo
    snip = snip.substring(0, 130);
    int sp = snip.lastIndexOf(' ');
    if (sp > 100) snip = snip.substring(0, sp);
    snip += "...";
  }
  // linhas: "titulo\x02url"
  int p = 0, nr = 0;
  int start = goPage * 5;
  while (nr < 5) {
    int e = rest.indexOf('\n', p);
    String ln = rest.substring(p, e < 0 ? rest.length() : e);
    if (e < 0) p = rest.length();
    else p = e + 1;
    if (!ln.length()) { if (e < 0) break; else continue; }
    if (start > 0) { start--; continue; }
    int c = ln.indexOf('\x02');
    if (c < 0) { if (e < 0) break; else continue; }
    String t = ln.substring(0, c);
    String u = ln.substring(c + 1);
    t.trim(); u.trim();
    if (t.length() < 2) continue;
    gGoUrl[nr] = u;
    if (!nr) gGoUrlTtl = t;                    // titulo do 1o resultado desta pagina
    if (!nr) gGoSnipSave = snip;              // guarda p/ fallback do leitor
    int y = 46 + nr * 28;
    lv_obj_t *b = lv_btn_create(goBoxR);
    lv_obj_set_pos(b, 0, y);
    lv_obj_set_size(b, 122, 26);
    lv_obj_set_style_bg_color(b, lv_color_hex(0x161A22), 0);
    lv_obj_set_style_radius(b, 6, 0);
    lv_obj_set_style_shadow_width(b, 0, 0);
    lv_obj_add_event_cb(b, goOpenCb, LV_EVENT_CLICKED, (void *)(intptr_t)nr);
    lv_obj_add_event_cb(b, clickBeep, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lt = lv_label_create(b);
    lv_label_set_text(lt, t.c_str());
    lv_obj_set_pos(lt, 3, 1);
    lv_obj_set_style_text_font(lt, F_SM, 0);
    lv_obj_set_style_text_color(lt, REDMI_TXT, 0);
    lv_obj_set_width(lt, 116);
    lv_label_set_long_mode(lt, LV_LABEL_LONG_DOT);
    lv_obj_clear_flag(lt, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_t *lu = lv_label_create(b);
    lv_label_set_text(lu, u.c_str());
    lv_obj_set_pos(lu, 3, 14);
    lv_obj_set_style_text_font(lu, F_SM, 0);
    lv_obj_set_style_text_color(lu, REDMI_OK, 0);          // link em verde
    lv_obj_set_width(lu, 116);
    lv_label_set_long_mode(lu, LV_LABEL_LONG_DOT);
    lv_obj_clear_flag(lu, LV_OBJ_FLAG_CLICKABLE);
    goRow[nr] = b;
    nr++;
  }
  // card do topo (estilo Google): titulo do 1º site + resuminho + host verde
  String h1 = gGoUrl[0]; int sl = h1.indexOf("//");
  if (sl >= 0) h1 = h1.substring(sl + 2);
  int pe = h1.indexOf('/');  if (pe > 0) h1 = h1.substring(0, pe);
  if (lv_obj_is_valid(goSnip)) {
    if (gGoUrlTtl.length() || snip.length()) {
      if (lv_obj_is_valid(goSnipTt))   lv_label_set_text(goSnipTt, gGoUrlTtl.c_str());
      if (lv_obj_is_valid(goSnipL))    lv_label_set_text(goSnipL, snip.c_str());
      if (lv_obj_is_valid(goSnipHost)) lv_label_set_text(goSnipHost, h1.c_str());
      lv_obj_clear_flag(goSnip, LV_OBJ_FLAG_HIDDEN);
    } else lv_obj_add_flag(goSnip, LV_OBJ_FLAG_HIDDEN);
  }
  goShowTab(0);                     // resultado pronto: volta p/ aba Pesquisa
  if (!nr) toast("sem resultados uteis");
}
static void goSearchCb(lv_event_t *) {
  String q = lv_textarea_get_text(goTa);
  if (!q.length()) { toast("Digite o que buscar"); return; }
  if (gNetBusy) { toast("Aguarde..."); return; }
  if (WiFi.status() != WL_CONNECTED) { toast("Sem Wi-Fi"); return; }
  gGoQ = q;
  gNetCb = goSearchNetCb;
  netRun(T_GOO, q);
  if (goSpin && lv_obj_is_valid(goSpin)) lv_obj_clear_flag(goSpin, LV_OBJ_FLAG_HIDDEN);
  toast("Buscando...");
}
void buildGoogle() {
  scrGoogle = lv_obj_create(NULL);
  psMapReg(scrGoogle, PSR_GOOGLE);      // miniaturas/JPEG na PSRAM: libera 90s apos sair
  scrStyle(scrGoogle);
  appHeader(scrGoogle, "GOOGLE");
  // campo de busca + botao
  goTa = uiTa(scrGoogle, 0, 43, 96, 19);
  lv_textarea_set_placeholder_text(goTa, "o que buscar?");
  lv_obj_t *bG = uiBtn(scrGoogle, "Ir", goSearchCb, 28, 19, 0x10304A, NULL);
  lv_obj_set_pos(bG, 100, 43);
  // abas no topo (estilo Google); user_data = 0 pesquisa / 1 imagens
  goTab0 = uiBtn(scrGoogle, "Pesquisa", [](lv_event_t *e) { goTabCb(e); }, 62, 15, 0x161A22, (void *)0);
  lv_obj_set_pos(goTab0, 0, 65);
  goTab1 = uiBtn(scrGoogle, "Imagens", [](lv_event_t *e) { goTabCb(e); }, 62, 15, 0x161A22, (void *)1);
  lv_obj_set_pos(goTab1, 66, 65);
  lv_obj_set_style_text_color(lv_obj_get_child(goTab0, 0), REDMI_ACC, 0);
  lv_obj_set_style_text_color(lv_obj_get_child(goTab1, 0), REDMI_DIM, 0);
  goTabU = lv_obj_create(scrGoogle);
  lv_obj_set_pos(goTabU, 0, 81);
  lv_obj_set_size(goTabU, 62, 2);
  lv_obj_set_style_bg_color(goTabU, REDMI_ACC, 0);
  lv_obj_set_style_bg_opa(goTabU, LV_OPA_COVER, 0);
  lv_obj_set_style_border_opa(goTabU, 0, 0);
  lv_obj_clear_flag(goTabU, LV_OBJ_FLAG_CLICKABLE);
  // ---- aba Pesquisa ----
  goBoxR = lv_obj_create(scrGoogle);
  lv_obj_set_pos(goBoxR, 0, 84);
  lv_obj_set_size(goBoxR, 128, 76);
  lv_obj_set_style_bg_opa(goBoxR, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_opa(goBoxR, 0, 0);
  lv_obj_set_style_pad_all(goBoxR, 0, 0);
  lv_obj_set_style_pad_bottom(goBoxR, 22, 0);      // espaco p/ botoes de paginacao
  lv_obj_set_scrollbar_mode(goBoxR, LV_SCROLLBAR_MODE_OFF);
  lv_obj_set_scroll_dir(goBoxR, LV_DIR_VER);
  // "resuminho" do primeiro resultado (estilo Google)
  goSnip = lv_obj_create(goBoxR);
  lv_obj_set_pos(goSnip, 0, 0);
  lv_obj_set_size(goSnip, 124, 42);
  lv_obj_set_style_bg_color(goSnip, lv_color_hex(0x1E2430), 0);
  lv_obj_set_style_bg_opa(goSnip, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(goSnip, 6, 0);
  lv_obj_set_style_shadow_width(goSnip, 0, 0);
  lv_obj_set_style_pad_all(goSnip, 0, 0);
  lv_obj_clear_flag(goSnip, LV_OBJ_FLAG_CLICKABLE);
  goSnipTt = lv_label_create(goSnip);              // titulo do site
  lv_obj_set_pos(goSnipTt, 3, 0);
  lv_obj_set_width(goSnipTt, 118);
  lv_label_set_text(goSnipTt, "");
  lv_obj_set_style_text_font(goSnipTt, F_SM, 0);
  lv_obj_set_style_text_color(goSnipTt, REDMI_TXT, 0);
  lv_label_set_long_mode(goSnipTt, LV_LABEL_LONG_DOT);
  lv_obj_clear_flag(goSnipTt, LV_OBJ_FLAG_CLICKABLE);
  goSnipL = lv_label_create(goSnip);               // resuminho
  lv_obj_set_pos(goSnipL, 3, 12);
  lv_obj_set_width(goSnipL, 118);
  lv_label_set_text(goSnipL, "");
  lv_obj_set_style_text_font(goSnipL, F_SM, 0);
  lv_obj_set_style_text_color(goSnipL, REDMI_DIM, 0);
  lv_label_set_long_mode(goSnipL, LV_LABEL_LONG_DOT);
  lv_obj_clear_flag(goSnipL, LV_OBJ_FLAG_CLICKABLE);
  goSnipHost = lv_label_create(goSnip);            // host em verde
  lv_obj_set_pos(goSnipHost, 3, 26);
  lv_obj_set_width(goSnipHost, 118);
  lv_label_set_text(goSnipHost, "");
  lv_obj_set_style_text_font(goSnipHost, F_SM, 0);
  lv_obj_set_style_text_color(goSnipHost, REDMI_OK, 0);
  lv_label_set_long_mode(goSnipHost, LV_LABEL_LONG_DOT);
  lv_obj_clear_flag(goSnipHost, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_flag(goSnip, LV_OBJ_FLAG_HIDDEN);
  goNoneL = lv_label_create(goBoxR);
  lv_obj_set_pos(goNoneL, 0, 2);
  lv_obj_set_width(goNoneL, 124);
  lv_label_set_text(goNoneL, "busque um termo e toque em Ir");
  lv_obj_set_style_text_font(goNoneL, F_SM, 0);
  lv_obj_set_style_text_color(goNoneL, REDMI_DIM, 0);
  lv_obj_clear_flag(goNoneL, LV_OBJ_FLAG_CLICKABLE);
  // ---- aba Imagens ----
  goBoxI = lv_obj_create(scrGoogle);
  lv_obj_set_pos(goBoxI, 0, 84);
  lv_obj_set_size(goBoxI, 128, 76);
  lv_obj_set_style_bg_opa(goBoxI, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_opa(goBoxI, 0, 0);
  lv_obj_set_style_pad_all(goBoxI, 0, 0);
  lv_obj_set_scrollbar_mode(goBoxI, LV_SCROLLBAR_MODE_OFF);
  lv_obj_clear_flag(goBoxI, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_flag(goBoxI, LV_OBJ_FLAG_HIDDEN);
  for (int i = 0; i < 4; i++) {
    if (!goPix[i]) {
      if (!psNeed(IMG_W * IMG_H * 2)) continue;
      goPix[i] = (uint16_t *)heap_caps_malloc(IMG_W * IMG_H * 2, MALLOC_CAP_SPIRAM);
      if (goPix[i]) { memset(goPix[i], 0x08, IMG_W * IMG_H * 2); psAllocated[PSR_GOOGLE] = true; }
    }
    int cx = (i % 2) * 64 + 2, cy = 12 + (i / 2) * 32;
    // (buffer reciclado na reabertura do app)
    goImg[i] = lv_canvas_create(goBoxI);
    lv_obj_set_pos(goImg[i], cx, cy);
    lv_obj_set_style_bg_color(goImg[i], lv_color_hex(0x141821), 0);
    lv_obj_set_style_bg_opa(goImg[i], LV_OPA_COVER, 0);
    if (goPix[i])
      lv_canvas_set_buffer(goImg[i], goPix[i], IMG_W, IMG_H, LV_IMG_CF_TRUE_COLOR);
    lv_obj_add_flag(goImg[i], LV_OBJ_FLAG_HIDDEN);      // aparece quando baixar
    lv_obj_clear_flag(goImg[i], LV_OBJ_FLAG_CLICKABLE);
  }
  goImgLbl = lv_label_create(goBoxI);
  lv_obj_set_pos(goImgLbl, 0, 0);
  lv_obj_set_width(goImgLbl, 124);
  lv_label_set_text(goImgLbl, "imagens: Wikipedia");
  lv_obj_set_style_text_font(goImgLbl, F_SM, 0);
  lv_obj_set_style_text_color(goImgLbl, REDMI_DIM, 0);
  // botoes de paginacao (aba Pesquisa)
  goBtnPrev = uiBtn(scrGoogle, "<", goPrevPageCb, 22, 15, 0x2A3040, NULL);
  lv_obj_set_pos(goBtnPrev, 2, 143);
  lv_obj_add_flag(goBtnPrev, LV_OBJ_FLAG_HIDDEN);
  goBtnNext = uiBtn(scrGoogle, ">", goNextPageCb, 22, 15, 0x2A3040, NULL);
  lv_obj_set_pos(goBtnNext, 104, 143);
  lv_obj_add_flag(goBtnNext, LV_OBJ_FLAG_HIDDEN);
  goSrc = goImgLbl;                    // fonte das imagens = titulo da aba
  goSpin = lv_spinner_create(scrGoogle, 900, 60);
  lv_obj_set_pos(goSpin, 54, 112);
  lv_obj_set_size(goSpin, 20, 20);
  lv_obj_set_style_text_color(goSpin, REDMI_ACC, 0);
  lv_obj_add_flag(goSpin, LV_OBJ_FLAG_HIDDEN);
  for (int i = 0; i < 5; i++) goRow[i] = nullptr;
  goShowTab(0);                     // reabrindo o app comeca na aba Pesquisa
}

// ============================================================
//  IA (OpenAI chat)
// ============================================================
static lv_obj_t *iaKey, *iaAsk, *iaOut, *iaSpin, *iaHist;
static String iaHistory[8];   // historico simples de ate 8 Q&A
static int iaHistN = 0;
static void iaKeySaveCb(lv_event_t *) {
  cfg.iaKey = lv_textarea_get_text(iaKey);
  cfgSave();
  toast(cfg.iaKey.length() ? "Chave salva" : "Chave apagada");
}
static void iaNetCb(bool ok, const String &out) {
  if (iaSpin && lv_obj_is_valid(iaSpin)) lv_obj_add_flag(iaSpin, LV_OBJ_FLAG_HIDDEN);
  if (lv_obj_is_valid(iaOut)) {
    String t = out;
    if (!ok) { t = "ERRO: " + t; lv_obj_set_style_text_color(iaOut, REDMI_ERR, 0); }
    else { lv_obj_set_style_text_color(iaOut, REDMI_TXT, 0); }
    if (t.length() > 800) t = t.substring(0, 800) + "...";
    lv_label_set_text(iaOut, t.c_str());
    lv_obj_t *ibx = lv_obj_get_parent(iaOut);
    if (lv_obj_is_valid(ibx)) lv_obj_scroll_to_y(ibx, LV_COORD_MAX, LV_ANIM_OFF);
    // guarda no historico
    if (ok && iaHistN < 8) {
      iaHistory[iaHistN++] = t;
      if (lv_obj_is_valid(iaHist)) {
        String h = "";
        for (int i = 0; i < iaHistN; i++) { h += ">" + iaHistory[i].substring(0, 40) + "\n"; }
        lv_label_set_text(iaHist, h.c_str());
      }
    }
  }
}
static void iaAskCb(lv_event_t *) {
  String q = lv_textarea_get_text(iaAsk);
  if (!q.length()) { toast("Escreva a pergunta"); return; }
  if (!cfg.iaKey.length()) { toast("Salve a chave OpenAI primeiro"); return; }
  if (gNetBusy) { toast("Aguarde..."); return; }
  gNetCb = iaNetCb;
  netRun(T_AI, q);
  if (iaSpin && lv_obj_is_valid(iaSpin)) lv_obj_clear_flag(iaSpin, LV_OBJ_FLAG_HIDDEN);
  lv_label_set_text(iaOut, "pensando...");
  toast("Perguntando...");
}
void buildIA() {
  scrIA = lv_obj_create(NULL);
  scrStyle(scrIA);
  appHeader(scrIA, "IA");
  lv_obj_t *body = appBody(scrIA);
  iaKey = uiTa(body, 0, 0, 86, 20, true);
  lv_textarea_set_text(iaKey, cfg.iaKey.c_str());
  lv_textarea_set_placeholder_text(iaKey, "sk-...");
  lv_obj_t *bK = uiBtn(body, "ok", iaKeySaveCb, 30, 20, 0x1A3B25, NULL);
  lv_obj_set_pos(bK, 88, 0);
  iaAsk = uiTa(body, 0, 23, 118, 19);
  lv_textarea_set_placeholder_text(iaAsk, "pergunte...");
  lv_obj_t *bA = uiBtn(body, "Perguntar", iaAskCb, 118, 18, 0x10304A, NULL);
  lv_obj_set_pos(bA, 0, 44);
  iaHist = nullptr;                          // historico fica no array (sem label sobreposto)
  // resposta em area rolavel (auto-scroll para o fim)
  lv_obj_t *ibox = lv_obj_create(body);
  lv_obj_set_pos(ibox, 0, 64);
  lv_obj_set_size(ibox, 118, 38);
  lv_obj_set_style_bg_color(ibox, lv_color_hex(0x12151C), 0);
  lv_obj_set_style_bg_opa(ibox, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(ibox, 8, 0);
  lv_obj_set_style_border_width(ibox, 0, 0);
  lv_obj_set_style_pad_all(ibox, 3, 0);
  lv_obj_set_scroll_dir(ibox, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(ibox, LV_SCROLLBAR_MODE_AUTO);
  iaOut = lv_label_create(ibox);
  lv_label_set_text(iaOut, "resposta aparece aqui");
  lv_obj_set_width(iaOut, 106);
  lv_label_set_long_mode(iaOut, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_font(iaOut, F_SM, 0);
  lv_obj_set_style_text_color(iaOut, REDMI_DIM, 0);
  lv_obj_set_pos(iaOut, 0, 0);
  iaSpin = lv_spinner_create(body, 900, 60);
  lv_obj_set_pos(iaSpin, 50, 74);
  lv_obj_set_size(iaSpin, 18, 18);
  lv_obj_add_flag(iaSpin, LV_OBJ_FLAG_HIDDEN);
}

// ============================================================
//  JOGOS (12 jogos reais)
// ============================================================
static void gameDiceCb(lv_event_t *) { buildDado(); nav(scrDado, LV_SCR_LOAD_ANIM_MOVE_LEFT); }
static const char *jogoIcone(const char *n);

// card quadrado 28x28 (4 por linha) - icone + nome
static void gameCardSq(lv_obj_t *grid, const char *name, lv_event_cb_t cb) {
  lv_obj_t *c = lv_obj_create(grid);
  lv_obj_set_size(c, 57, 48);                       // 2 por linha (nomes completos cabem)
  lv_obj_set_style_bg_color(c, REDMI_CARD, 0);
  lv_obj_set_style_border_color(c, lv_color_hex(0x2A3444), 0);
  lv_obj_set_style_border_width(c, 1, 0);
  lv_obj_set_style_radius(c, 10, 0);
  lv_obj_clear_flag(c, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(c, cb, LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(c, clickBeep, LV_EVENT_CLICKED, NULL);
  icoCanvas(c, name, 4, 3, REDMI_ACC.full, REDMI_CARD.full);   // icone accent sobre o card
  lv_obj_t *lb = lv_label_create(c);
  lv_label_set_text(lb, name);
  lv_obj_set_style_text_font(lb, F_SM, 0);
  lv_obj_set_style_text_color(lb, REDMI_TXT, 0);
  lv_obj_set_width(lb, 55);
  lv_label_set_long_mode(lb, LV_LABEL_LONG_DOT);
  lv_obj_set_style_text_align(lb, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(lb, LV_ALIGN_BOTTOM_MID, 0, -3);
  lv_obj_clear_flag(lb, LV_OBJ_FLAG_CLICKABLE);
}
void buildRunner(int modo);
void buildFlappy();
void buildDino();
void buildTetris();
void buildPong();
void buildInvaders();
void buildDama();
void buildXadrez();
void buildSnake();
void buildDoom();
#if HW_BLE
void buildBtMidia();
#endif
// ============================================================
//  SNAKE - classico (grade 6px); arraste muda direcao, come e cresce
// ============================================================
#define SN_CW 20
#define SN_CH 16
#define SN_CELL 6
static lv_obj_t *snScr, *snGrade[SN_CW][SN_CH], *snSco, *snGrid;
static int snBodyX[SN_CW * SN_CH], snBodyY[SN_CW * SN_CH], snLen, snDir, snFx, snFy, snTick, snVel, snFim, snPontos;
static void snSeedFood() {
  bool livre = false;
  while (!livre) {
    snFx = esp_random() % SN_CW; snFy = esp_random() % SN_CH;
    livre = true;
    for (int i = 0; i < snLen; i++) if (snBodyX[i] == snFx && snBodyY[i] == snFy) { livre = false; break; }
  }
}
static void snResetBody() {
  snLen = 4; snDir = 1; snPontos = 0; snVel = 6; snFim = 0; snTick = 0;
  for (int i = 0; i < snLen; i++) { snBodyX[i] = 6 - i; snBodyY[i] = 8; }
  snSeedFood();
  if (lv_obj_is_valid(snSco)) lv_label_set_text(snSco, "0");
}
static void snDraw() {
  for (int x = 0; x < SN_CW; x++) for (int y = 0; y < SN_CH; y++) {
    if (!lv_obj_is_valid(snGrade[x][y])) continue;
    uint32_t col = 0x141821;
    if (x == snFx && y == snFy) col = 0xFF5C5C;
    for (int i = 0; i < snLen; i++)
      if (snBodyX[i] == x && snBodyY[i] == y) { col = (i == 0) ? 0xF4F5F7 : lv_color_to32(REDMI_ACC); break; }
    lv_obj_set_style_bg_color(snGrade[x][y], lv_color_hex(col), 0);
  }
}
static void snUpd(lv_timer_t *) {
  if (!gInGame) return;
  if (!lv_obj_is_valid(snScr) || lv_scr_act() != snScr) return;
  static int px = -1, py = -1;
  if (gTouchDown) {
    if (px < 0) { px = gTx; py = gTy; }
    int dx = gTx - px, dy = gTy - py;
    if (abs(dx) > 8 || abs(dy) > 8) {
      if (abs(dx) > abs(dy)) { if (dx > 0 && snDir != 3) snDir = 1; else if (dx < 0 && snDir != 1) snDir = 3; }
      else { if (dy > 0 && snDir != 0) snDir = 2; else if (dy < 0 && snDir != 2) snDir = 0; }
      px = gTx; py = gTy;
    }
  } else { px = -1; py = -1; }
  if (snFim) return;
  if (++snTick < snVel) return;
  snTick = 0;
  int nx = snBodyX[0], ny = snBodyY[0];
  if (snDir == 0) ny--; else if (snDir == 1) nx++; else if (snDir == 2) ny++; else nx--;
  if (nx < 0 || nx >= SN_CW || ny < 0 || ny >= SN_CH) { snFim = 1; toast("GAME OVER - toque reinicia"); return; }
  for (int i = 0; i < snLen; i++) if (snBodyX[i] == nx && snBodyY[i] == ny) { snFim = 1; toast("GAME OVER - toque reinicia"); return; }
  for (int i = snLen; i > 0; i--) { snBodyX[i] = snBodyX[i - 1]; snBodyY[i] = snBodyY[i - 1]; }
  snBodyX[0] = nx; snBodyY[0] = ny;
  if (nx == snFx && ny == snFy) {
    snLen++; snPontos++;
    if (snVel > 2) snVel--;
    snSeedFood();
    if (lv_obj_is_valid(snSco)) lv_label_set_text_fmt(snSco, "%d", snPontos);
  }
  snDraw();
}
static void snTapCb(lv_event_t *) { if (snFim) snResetBody(); }
void buildSnake() {
  snScr = lv_obj_create(NULL);
  scrStyle(snScr);
  gameHeader(snScr);
  lv_obj_clear_flag(snScr, LV_OBJ_FLAG_SCROLLABLE);
  snSco = uiLabel(snScr, "0", 0, 20, F_SM, 0xF4F5F7);
  lv_obj_set_width(snSco, 128); lv_obj_set_style_text_align(snSco, LV_TEXT_ALIGN_CENTER, 0);
  snGrid = lv_obj_create(snScr);
  lv_obj_set_pos(snGrid, 4, 46);
  lv_obj_set_size(snGrid, SN_CW * SN_CELL, SN_CH * SN_CELL);
  lv_obj_set_style_bg_opa(snGrid, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_opa(snGrid, 0, 0);
  lv_obj_set_style_pad_all(snGrid, 0, 0);
  lv_obj_set_scrollbar_mode(snGrid, LV_SCROLLBAR_MODE_OFF);
  lv_obj_clear_flag(snGrid, LV_OBJ_FLAG_SCROLLABLE);
  for (int x = 0; x < SN_CW; x++) for (int y = 0; y < SN_CH; y++) {
    snGrade[x][y] = lv_obj_create(snGrid);
    lv_obj_set_pos(snGrade[x][y], x * SN_CELL, y * SN_CELL);
    lv_obj_set_size(snGrade[x][y], SN_CELL, SN_CELL);
    lv_obj_set_style_bg_color(snGrade[x][y], lv_color_hex(0x141821), 0);
    lv_obj_set_style_radius(snGrade[x][y], 0, 0);
    lv_obj_set_style_border_opa(snGrade[x][y], 0, 0);
    lv_obj_set_style_shadow_width(snGrade[x][y], 0, 0);
    lv_obj_set_style_pad_all(snGrade[x][y], 0, 0);
    lv_obj_clear_flag(snGrade[x][y], LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(snGrade[x][y], LV_OBJ_FLAG_CLICKABLE);
  }
  uiLabel(snScr, "arraste muda a direcao", 0, 146, F_SM, 0x676C7A);
  lv_obj_set_width(lv_obj_get_child(snScr, -1), 128);
  lv_obj_set_style_text_align(lv_obj_get_child(snScr, -1), LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_add_event_cb(snScr, snTapCb, LV_EVENT_CLICKED, NULL);
  snResetBody();
  static lv_timer_t *tsn = nullptr;
  if (tsn) { lv_timer_del(tsn); tsn = nullptr; }
  tsn = lv_timer_create(snUpd, 60, NULL);
  gGameReg(snScr);
  nav(snScr, LV_SCR_LOAD_ANIM_MOVE_LEFT);
}

// ============================================================
//  CONTROLE BT (BLE HID consumer) - midia + achar celular
//  pareia como "Wear-Midia"; não mexe na USB/reflash.
// ============================================================
#if HW_BLE
#include <BLEHIDDevice.h>
static lv_obj_t *btmScr, *btmSt;
static BLEHIDDevice     *btmHid = nullptr;
static BLECharacteristic *btmC  = nullptr;
static lv_timer_t *btmFind = nullptr;
static const uint8_t btmMap[] = {
  0x05,0x0C, 0x09,0x01, 0xA1,0x01,           // Usage Page: Consumer, Usage: Consumer Control
  0x85,0x01,                                  //   Report ID 1
  0x15,0x00, 0x26,0xFF,0x03,                  //   Logical Min 0, Max 1023
  0x19,0x00, 0x2A,0xFF,0x03,                  //   Usage Min 0, Max 1023
  0x75,0x10, 0x95,0x01,                       //   Report Size 16, Count 1
  0x81,0x00,                                  //   Input (Data,Ary,Abs)
  0xC0 };                                     // End Collection
static void btmInit() {
  if (btmHid) { BLEDevice::startAdvertising(); return; }
  BLEDevice::init("Wear-Midia");
  BLEServer *srv = BLEDevice::createServer();
  btmHid = new BLEHIDDevice(srv);
  btmHid->reportMap((uint8_t *)btmMap, sizeof(btmMap));
  btmHid->manufacturer()->setValue("wear");
  btmHid->pnp(0x02, 0xe502, 0xa111, 0x0210);
  btmHid->hidInfo(0x00, 0x01);
  btmC = btmHid->inputReport(1);
  btmHid->startServices();
  BLEAdvertising *adv = BLEDevice::getAdvertising();
  adv->setAppearance(0x03C0);
  adv->addServiceUUID(BLEUUID((uint16_t)0x1812));
  adv->start();
  gBtOn = true;
}
static void btmKey(uint16_t u) {
  if (!btmC) return;
  uint16_t v = u;
  btmC->setValue((uint8_t *)&v, 2); btmC->notify();
  delay(70);
  v = 0; btmC->setValue((uint8_t *)&v, 2); btmC->notify();
  beepShort(1400, 20);
}
static int btmFindN = 0;
static void btmFindTick(lv_timer_t *t) {
  if (btmFindN <= 0 || !btmC) { lv_timer_del(t); btmFind = nullptr;
    if (lv_obj_is_valid(btmSt)) lv_label_set_text(btmSt, "pronto - use os botoes"); return; }
  btmKey(0x00CD); btmFindN--;
}
static void btmFindCb(lv_event_t *) {
  btmInit(); btmFindN = 8;
  if (!btmFind) btmFind = lv_timer_create(btmFindTick, 350, NULL);
  if (lv_obj_is_valid(btmSt)) lv_label_set_text(btmSt, "procurando celular...");
  toast("procurando celular via BT");
}
static void btmCmdCb(lv_event_t *e) {
  int op = (int)(intptr_t)lv_event_get_user_data(e);
  btmInit();
  uint16_t u = 0;
  if (op == 0) u = 0x00B6;        // faixa anterior
  else if (op == 1) u = 0x00CD;   // play/pausa
  else if (op == 2) u = 0x00B5;   // proxima faixa
  else if (op == 3) u = 0x00E9;   // volume +
  else if (op == 4) u = 0x00EA;   // volume -
  else u = 0x00E2;                // mudo
  btmKey(u);
}
static void btmCleanup() {
  if (btmFind) { lv_timer_del(btmFind); btmFind = nullptr; }
  btmFindN = 0;
  if (btmHid) BLEDevice::deinit(false);
  btmHid = nullptr; btmC = nullptr;
  gBtOn = false;
}
static void btmExitCb(lv_event_t *) {
  btmCleanup();
  buildToolbox();
  nav(scrToolbox, LV_SCR_LOAD_ANIM_MOVE_RIGHT);
}
void buildBtMidia() {
  btmScr = lv_obj_create(NULL);
  scrStyle(btmScr);
  appHeader(btmScr, "CONTROLE BT");
  lv_obj_clear_flag(btmScr, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(btmScr, [](lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_DELETE) btmCleanup();
  }, LV_EVENT_DELETE, NULL);
  uiLabel(btmScr, "pareie 'Wear-Midia'", 0, 44, F_SM, 0x676C7A);
  lv_obj_set_width(lv_obj_get_child(btmScr, -1), 128);
  lv_obj_set_style_text_align(lv_obj_get_child(btmScr, -1), LV_TEXT_ALIGN_CENTER, 0);
  btmSt = uiLabel(btmScr, "aguardando pareamento", 0, 60, F_SM, 0xF4F5F7);
  lv_obj_set_width(btmSt, 128);
  lv_obj_set_style_text_align(btmSt, LV_TEXT_ALIGN_CENTER, 0);
  const char *bl[6] = { "<", "PLAY", ">", "VOL+", "VOL-", "MUDO" };
  for (int i = 0; i < 6; i++) {
    int col = i % 3, row = i / 3;
    uiBtn(btmScr, bl[i], btmCmdCb, 40, 20, i == 1 ? 0x10304A : 0x1C202B, (void *)(intptr_t)i);
    if (auto b = lv_obj_get_child(btmScr, -1)) lv_obj_set_pos(b, 1 + col * 42, 78 + row * 24);
  }
  uiBtn(btmScr, "Achar celular", btmFindCb, 80, 18, 0x10304A, NULL);
  if (auto b = lv_obj_get_child(btmScr, -1)) lv_obj_set_pos(b, 0, 128);
  uiBtn(btmScr, "Sair", btmExitCb, 44, 18, 0x1C202B, NULL);
  if (auto b = lv_obj_get_child(btmScr, -1)) lv_obj_set_pos(b, 82, 128);
  nav(btmScr, LV_SCR_LOAD_ANIM_MOVE_LEFT);
}
#endif

// ============================================================
//  DOOM - tiro em 1a pessoa (raycaster estilo wolf3d) - motor proprio
// ============================================================
#define DM_VW 128
#define DM_VH 96
static const uint8_t dooMap[16][16] = {
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
  {1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,1},
  {1,0,1,0,0,0,1,0,1,0,1,1,0,0,0,1},
  {1,0,1,0,1,0,1,0,0,0,1,0,0,0,0,1},
  {1,0,0,0,1,0,0,0,0,0,1,1,1,0,0,1},
  {1,1,1,0,1,1,1,0,1,0,0,0,1,0,1,1},
  {1,0,0,0,0,0,1,0,1,1,1,0,1,0,0,1},
  {1,0,1,1,1,0,1,0,0,0,1,0,0,0,0,1},
  {1,0,0,0,1,0,0,0,0,0,0,0,1,1,0,1},
  {1,1,1,0,1,1,1,1,1,0,1,0,1,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,1,0,1,0,1,1},
  {1,0,1,1,1,1,1,0,1,0,0,0,1,0,0,1},
  {1,0,1,0,0,0,1,0,1,1,1,1,1,1,0,1},
  {1,0,1,0,1,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,1,0,0,0,0,0,0,0,0,0,0,1},
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}};
static lv_obj_t *dooScr, *dooCan, *dooHud;
static uint16_t *dooFB = nullptr;
static float dooPX, dooPY, dooAng;
static int   dooZ[DM_VW], dooKills, dooFim;
static float dooEnX[6], dooEnY[6]; static bool dooEnAl[6];
static void dooResetEn() {
  dooPX = 1.5f; dooPY = 4.5f; dooAng = 0.0f; dooKills = 0; dooFim = 0;
  float ex[6] = {2.5,6.5,9.5,3.5,12.5,13.5};
  float ey[6] = {4.5,4.5,4.5,1.5,2.5,13.5};
  for (int i=0;i<6;i++){ dooEnX[i]=ex[i]; dooEnY[i]=ey[i]; dooEnAl[i]=true; }
}
static float dooCast(float px, float py, float ang, int col) {
  float fx = 0.700f;                                  // tan(35deg) ~ FOV/2
  float camx = 2.0f * col / (DM_VW - 1) - 1.0f;
  float ray = ang + atanf(camx * fx);
  float rdx = cosf(ray), rdy = sinf(ray);
  int mapx = (int)px, mapy = (int)py;
  float ddx = fabsf(rdx) > 1e-6f ? fabsf(1.0f / rdx) : 1e30f;
  float ddy = fabsf(rdy) > 1e-6f ? fabsf(1.0f / rdy) : 1e30f;
  int stx = rdx > 0 ? 1 : -1, sty = rdy > 0 ? 1 : -1;
  float sdx = (rdx > 0 ? (mapx + 1 - px) : (px - mapx)) * ddx;
  float sdy = (rdy > 0 ? (mapy + 1 - py) : (py - mapy)) * ddy;
  int side = 0;
  for (int i = 0; i < 48; i++) {
    if (sdx < sdy) { sdx += ddx; mapx += stx; side = 0; }
    else            { sdy += ddy; mapy += sty; side = 1; }
    if (mapx < 0 || mapx >= 16 || mapy < 0 || mapy >= 16) return 30.0f;
    if (dooMap[mapy][mapx]) return (side == 0 ? sdx - ddx : sdy - ddy);
  }
  return 30.0f;
}
static uint16_t dooShade(float d, uint16_t base) {
  float s = d < 1.0f ? 1.0f : (d > 6.0f ? 0.22f : (1.0f - (d - 1.0f) / 7.0f));
  int r = ((base >> 11) & 31), g = ((base >> 5) & 63), b = (base & 31);
  r = (int)(r * s); g = (int)(g * s); b = (int)(b * s);
  return (uint16_t)((r << 11) | (g << 5) | b);
}
static void dooRender() {
  if (!dooFB) return;
  for (int y = 0; y < DM_VH; y++) {
    uint16_t ln = (y < DM_VH/2) ? (uint16_t)0x18C3 : (uint16_t)0x2965;
    for (int x = 0; x < DM_VW; x++) dooFB[y * DM_VW + x] = ln;
  }
  for (int col = 0; col < DM_VW; col++) {
    float d = dooCast(dooPX, dooPY, dooAng, col);
    dooZ[col] = (int)(d * 100);
    int lh = (int)(70.0f / d); if (lh > DM_VH) lh = DM_VH; if (lh < 2) lh = 2;
    int y0 = (DM_VH - lh) / 2;
    uint16_t wc = dooShade(d, 0x7BEF);
    for (int y = y0; y < y0 + lh && y < DM_VH; y++) dooFB[y * DM_VW + col] = wc;
  }
  for (int e = 0; e < 6; e++) {
    if (!dooEnAl[e]) continue;
    float dx = dooEnX[e] - dooPX, dy = dooEnY[e] - dooPY;
    float dist = sqrtf(dx*dx + dy*dy);
    float a = atan2f(dy, dx) - dooAng;
    while (a >  3.14159f) a -= 6.28318f;
    while (a < -3.14159f) a += 6.28318f;
    if (fabsf(a) > 1.0f) continue;
    float camx = tanf(a) / 0.700f;
    int sx = (int)((camx + 1) * 0.5f * DM_VW);
    int sz = (int)(44.0f / dist); if (sz > 44) sz = 44; if (sz < 3) sz = 3;
    if (sx < -sz || sx > DM_VW + sz) continue;
    for (int yy = 0; yy < sz; yy++) for (int xx = 0; xx < sz; xx++) {
      int px = sx - sz/2 + xx, py = (DM_VH/2) - sz/2 + yy;
      if (px < 0 || px >= DM_VW || py < 0 || py >= DM_VH) continue;
      if (dooZ[px] < (int)(dist * 100)) continue;      // parede na frente
      int rx = xx - sz/2, ry = yy - sz/2;
      if (rx*rx + ry*ry <= (sz/2)*(sz/2)) dooFB[py * DM_VW + px] = (uint16_t)0xF800;
    }
  }
  // mira no centro
  for (int x = DM_VW/2 - 4; x <= DM_VW/2 + 4; x++) dooFB[(DM_VH/2) * DM_VW + x] = 0xFFFF;
  for (int y = DM_VH/2 - 4; y <= DM_VH/2 + 4; y++) dooFB[y * DM_VW + DM_VW/2] = 0xFFFF;
}
static void dooUpd(lv_timer_t *) {
  if (!gInGame) return;
  if (!lv_obj_is_valid(dooScr) || lv_scr_act() != dooScr) return;
  if (dooFim) return;
  static int ox = -1, oy = -1;
  if (gTouchDown) {
    if (ox < 0) { ox = gTx; oy = gTy; }
    int dx = gTx - ox, dy = gTy - oy;
    if (abs(dx) > 3 || abs(dy) > 3) {
      if (abs(dx) >= abs(dy)) dooAng += dx * 0.02f;
      else {
        float sp = dy < 0 ? 0.10f : 0.10f;
        if (dy > 0) sp = -0.10f;
        float nx = dooPX + cosf(dooAng) * sp, ny = dooPY + sinf(dooAng) * sp;
        if (!dooMap[(int)ny][(int)nx]) { dooPX = nx; dooPY = ny; }
      }
      ox = gTx; oy = gTy;
    }
  } else { ox = -1; oy = -1; }
  dooRender();
  if (lv_obj_is_valid(dooHud)) lv_label_set_text_fmt(dooHud, "ABATES %d/6", dooKills);
  if (dooKills >= 6 && !dooFim) { dooFim = 1; toast("FASE LIMPA! toque = reiniciar"); }
}
// Toque = tiro. So' dispara se o dedo NAO arrastou (arrasto = andar/mirar).
static int      dooTOX = 0, dooTOY = 0;
static uint32_t dooTOT = 0, dooLastShot = 0;
static void dooShoot() {
  if (dooFim) { dooResetEn(); dooRender(); return; }
  uint32_t now = millis();
  if (now - dooLastShot < 220) return;             // cadencia do tiro (evita rajada no arrasto)
  dooLastShot = now;
  bool hit = false;
  for (int e = 0; e < 6; e++) {
    if (!dooEnAl[e]) continue;
    float dx = dooEnX[e] - dooPX, dy = dooEnY[e] - dooPY;
    float dist = sqrtf(dx*dx + dy*dy);
    float a = atan2f(dy, dx) - dooAng;
    while (a >  3.14159f) a -= 6.28318f;
    while (a < -3.14159f) a += 6.28318f;
    if (fabsf(a) < 0.14f && dist < 9.0f) { dooEnAl[e] = false; dooKills++; hit = true; }
  }
  if (hit) beepShort(900,40);
  dooRender();
}
static void dooPressCb(lv_event_t *) {
  dooTOX = gTx; dooTOY = gTy; dooTOT = millis();
}
static void dooReleaseCb(lv_event_t *) {
  int dx = gTx - dooTOX, dy = gTy - dooTOY;
  if (abs(dx) > 7 || abs(dy) > 7) return;          // foi arrasto: nao atira
  if (millis() - dooTOT > 600) return;             // toque longo: nao atira
  dooShoot();
}
void buildDoom() {
  dooScr = lv_obj_create(NULL);
  psMapReg(dooScr, PSR_DOOM);           // framebuffer do jogo: libera na saida
  scrStyle(dooScr);
  gameHeader(dooScr);
  lv_obj_clear_flag(dooScr, LV_OBJ_FLAG_SCROLLABLE);
  if (!dooFB) {
    if (!psNeed(DM_VW * DM_VH * 2) ||
        !(dooFB = (uint16_t *)heap_caps_malloc(DM_VW * DM_VH * 2, MALLOC_CAP_SPIRAM))) {
      lv_obj_del(dooScr); dooScr = nullptr;
      toast("PSRAM cheia: feche um app");
      return;
    }
    psAllocated[PSR_DOOM] = true;
  }
  dooCan = lv_canvas_create(dooScr);
  lv_obj_set_pos(dooCan, 0, 46);
  lv_obj_set_size(dooCan, DM_VW, DM_VH);
  lv_canvas_set_buffer(dooCan, dooFB, DM_VW, DM_VH, LV_IMG_CF_TRUE_COLOR);
  lv_obj_clear_flag(dooCan, LV_OBJ_FLAG_CLICKABLE);
  dooHud = uiLabel(dooScr, " ", 0, 146, F_SM, 0xF4F5F7);
  lv_obj_set_width(dooHud, 128);
  lv_obj_set_style_text_align(dooHud, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_add_event_cb(dooScr, dooPressCb,   LV_EVENT_PRESSED,  NULL);
  lv_obj_add_event_cb(dooScr, dooReleaseCb, LV_EVENT_RELEASED, NULL);
  dooResetEn();
  dooRender();
  toast("arraste anda/mira, toque atira");
  static lv_timer_t *tdm = nullptr;
  if (tdm) { lv_timer_del(tdm); tdm = nullptr; }
  tdm = lv_timer_create(dooUpd, 60, NULL);
  gGameReg(dooScr);
  nav(dooScr, LV_SCR_LOAD_ANIM_MOVE_LEFT);
}

void buildJogos() {
  scrJogos = lv_obj_create(NULL);
  scrStyle(scrJogos);
  appHeader(scrJogos, "JOGOS");
  lv_obj_clear_flag(scrJogos, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_t *grid = lv_obj_create(scrJogos);
  lv_obj_set_pos(grid, 0, 44);
  lv_obj_set_size(grid, 128, 116);
  lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_opa(grid, 0, 0);
  lv_obj_set_style_pad_all(grid, 0, 0);
  lv_obj_set_style_pad_row(grid, 5, 0);
  lv_obj_set_style_pad_column(grid, 4, 0);
  lv_obj_set_scrollbar_mode(grid, LV_SCROLLBAR_MODE_AUTO);
  lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
  lv_obj_set_flex_align(grid, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  gameCardSq(grid, "PONG",      [](lv_event_t *) { buildPong(); });
  gameCardSq(grid, "INVASORES", [](lv_event_t *) { buildInvaders(); });
  gameCardSq(grid, "FLAPPY",    [](lv_event_t *) { buildFlappy(); });
  gameCardSq(grid, "DINO",      [](lv_event_t *) { buildDino(); });
  gameCardSq(grid, "TETRIS",    [](lv_event_t *) { buildTetris(); });
  gameCardSq(grid, "DAMA",      [](lv_event_t *) { buildDama(); });
  gameCardSq(grid, "XADREZ",    [](lv_event_t *) { buildXadrez(); });
  gameCardSq(grid, "MARIO",     [](lv_event_t *) { buildRunner(0); });
  gameCardSq(grid, "SONIC",     [](lv_event_t *) { buildRunner(1); });
  gameCardSq(grid, "DOOM",      [](lv_event_t *) { buildDoom(); });
  gameCardSq(grid, "DADO",      gameDiceCb);
  gameCardSq(grid, "MOEDA",     moedaCb);
  gameCardSq(grid, "SNAKE", [](lv_event_t *) { buildSnake(); });
}

// ============================================================
//  PONG (jogador x CPU) - toque/arrasto move a raquete
// ============================================================
static lv_obj_t *pgScr, *pgPadP, *pgPadC, *pgBall, *pgPl, *pgPc, *pgMsg;
static int pgX, pgY, pgVX, pgVY, pgP, pgC, pgOver;   // pgP/pgC = posicao Y das raquetes
static int pgSp, pgSc;                               // pgSp/pgSc = placar (jogador / CPU)
static void pgResetBall(int dir) {
  pgX = 60; pgY = 26; pgVX = 3 * dir; pgVY = (esp_random() & 1) ? 2 : -2;
}
static void pgUpd(lv_timer_t *) {
  if (!gInGame) return;
  if (!lv_obj_is_valid(pgScr) || lv_scr_act() != pgScr) return;
  if (pgOver) return;
  // coordenadas RELATIVAS ao campo (f: 126x54 em y=58)
  if (gTouchDown) {
    int py = gTy - 58 - 11;                        // centraliza raquete no dedo
    if (py < 0) py = 0; if (py > 32) py = 32;      // 54-22
    pgP = py;
    if (lv_obj_is_valid(pgPadP)) lv_obj_set_pos(pgPadP, 2, py);
  }
  pgX += pgVX; pgY += pgVY;
  if (pgY <= 1 || pgY >= 50) pgVY = -pgVY;
  // CPU preditiva: tenta interceptar onde a bola vai estar no lado dela
  if (pgVX > 0 && pgX > 62) {
    // bola vindo em direcao ao CPU: prediz Y quando a bola chegar em x=120
    float t = (120.0f - pgX) / (float)pgVX;
    float py = pgY + pgVY * t;
    // quica nas paredes superior/inferior antes de chegar
    while (py < 0 || py > 50) {
      if (py < 0) py = -py;
      if (py > 50) py = 100 - py;
    }
    int c = (int)py - 11;
    if (c < 0) c = 0; if (c > 32) c = 32;
    pgC = c;
  } else {
    // bola se afastando: volta ao centro gradualmente
    int c = pgC + (16 - pgC > 0 ? 1 : -1);
    if (c < 0) c = 0; if (c > 32) c = 32;
    pgC = c;
  }
  if (lv_obj_is_valid(pgPadC)) lv_obj_set_pos(pgPadC, 121, pgC);
  // colisao jogador (x 2..5)
  if (pgX <= 5 && pgX >= 2 && pgY + 3 >= pgP && pgY <= pgP + 22) {
    pgVX = 3 + (esp_random() % 2);
    pgVY = ((pgY + 1) < pgP + 11) ? -2 : 2;
  }
  // colisao CPU (x 121..124)
  if (pgX >= 120 && pgX <= 123 && pgY + 3 >= pgC && pgY <= pgC + 22) {
    pgVX = -(3 + (esp_random() % 2));
    pgVY = ((pgY + 1) < pgC + 11) ? -2 : 2;
  }
  if (pgX < 0) {
    pgSc++; pgResetBall(-1);
    if (lv_obj_is_valid(pgPc)) lv_label_set_text_fmt(pgPc, "%d", pgSc);
  }
  if (pgX > 123) {
    pgSp++; pgResetBall(1);
    if (lv_obj_is_valid(pgPl)) lv_label_set_text_fmt(pgPl, "%d", pgSp);
  }
  if (pgSp >= 5 || pgSc >= 5) {
    pgOver = 1;
    bool venceu = pgSp > pgSc;
    if (lv_obj_is_valid(pgMsg)) {
      lv_label_set_text(pgMsg, venceu ? "VOCE VENCEU!" : "A CPU VENCEU!");
      lv_obj_clear_flag(pgMsg, LV_OBJ_FLAG_HIDDEN);
      lv_obj_move_foreground(pgMsg);
    }
    toast(venceu ? "VOCE VENCEU!" : "A CPU venceu");
  }
  if (lv_obj_is_valid(pgBall)) lv_obj_set_pos(pgBall, pgX, pgY);
}
void buildPong() {
  pgScr = lv_obj_create(NULL);
  scrStyle(pgScr);
  gameHeader(pgScr);
  lv_obj_clear_flag(pgScr, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_t *pl = uiLabel(pgScr, "VOCE", 4, 20, F_SM, lv_color_to32(REDMI_TXT));
  pgPl = uiLabel(pgScr, "0", 30, 20, F_SM, lv_color_to32(REDMI_ACC));
  lv_obj_t *pc = uiLabel(pgScr, "CPU", 100, 20, F_SM, lv_color_to32(REDMI_TXT));
  pgPc = uiLabel(pgScr, "0", 122, 20, F_SM, lv_color_to32(REDMI_ERR));
  (void)pl; (void)pc;
  // campo
  lv_obj_t *f = lv_obj_create(pgScr);
  lv_obj_set_pos(f, 0, 58);
  lv_obj_set_size(f, 128, 54);
  lv_obj_set_style_bg_color(f, lv_color_hex(0x0E1117), 0);
  lv_obj_set_style_bg_opa(f, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(f, REDMI_DIM, 0);
  lv_obj_set_style_border_opa(f, LV_OPA_50, 0);
  lv_obj_set_style_radius(f, 4, 0);
  lv_obj_set_style_shadow_width(f, 0, 0);
  lv_obj_set_style_pad_all(f, 0, 0);
  lv_obj_clear_flag(f, LV_OBJ_FLAG_SCROLLABLE);
  pgPadP = lv_btn_create(f);
  lv_obj_set_pos(pgPadP, 2, 14);
  lv_obj_set_size(pgPadP, 3, 22);
  lv_obj_set_style_bg_color(pgPadP, REDMI_ACC, 0);
  lv_obj_set_style_radius(pgPadP, 2, 0);
  lv_obj_set_style_shadow_width(pgPadP, 0, 0);
  lv_obj_clear_flag(pgPadP, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(pgPadP, LV_OBJ_FLAG_CLICKABLE);
  pgPadC = lv_btn_create(f);
  lv_obj_set_pos(pgPadC, 121, 14);
  lv_obj_set_size(pgPadC, 3, 22);
  lv_obj_set_style_bg_color(pgPadC, lv_color_hex(0xFF5C5C), 0);
  lv_obj_set_style_radius(pgPadC, 2, 0);
  lv_obj_set_style_shadow_width(pgPadC, 0, 0);
  lv_obj_clear_flag(pgPadC, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(pgPadC, LV_OBJ_FLAG_CLICKABLE);
  pgBall = lv_btn_create(f);
  lv_obj_set_pos(pgBall, 30, 26);
  lv_obj_set_size(pgBall, 3, 3);
  lv_obj_set_style_bg_color(pgBall, lv_color_hex(0xF4F5F7), 0);
  lv_obj_set_style_radius(pgBall, 2, 0);
  lv_obj_set_style_shadow_width(pgBall, 0, 0);
  lv_obj_clear_flag(pgBall, LV_OBJ_FLAG_SCROLLABLE);
  uiLabel(pgScr, "toque e arraste p/ mover", 0, 122, F_SM, 0x676C7A);
  lv_obj_set_width(lv_obj_get_child(pgScr, -1), 128);
  lv_obj_set_style_text_align(lv_obj_get_child(pgScr, -1), LV_TEXT_ALIGN_CENTER, 0);
  uiLabel(pgScr, "primeiro a 5 ganha", 0, 138, F_SM, 0x676C7A);
  lv_obj_set_width(lv_obj_get_child(pgScr, -1), 128);
  lv_obj_set_style_text_align(lv_obj_get_child(pgScr, -1), LV_TEXT_ALIGN_CENTER, 0);
  // aviso de fim de partida (some quando o jogo recomeca)
  pgMsg = uiLabel(pgScr, "", 0, 100, F_UI, lv_color_to32(REDMI_ACC));
  lv_obj_set_width(pgMsg, 128);
  lv_obj_set_style_text_align(pgMsg, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_add_flag(pgMsg, LV_OBJ_FLAG_HIDDEN);
  // reinicia tocando na tela quando o jogo acaba
  lv_obj_add_event_cb(pgScr, [](lv_event_t *) {
    if (!pgOver) return;
    pgSp = pgSc = 0; pgP = pgC = pgOver = 0;
    if (lv_obj_is_valid(pgPl)) lv_label_set_text_fmt(pgPl, "%d", 0);
    if (lv_obj_is_valid(pgPc)) lv_label_set_text_fmt(pgPc, "%d", 0);
    if (lv_obj_is_valid(pgMsg)) lv_obj_add_flag(pgMsg, LV_OBJ_FLAG_HIDDEN);
    pgResetBall(1);
    toast("novo jogo!");
  }, LV_EVENT_CLICKED, NULL);
  pgP = pgC = pgOver = 0;
  pgSp = pgSc = 0;
  pgResetBall(1);
  static lv_timer_t *tpg = nullptr;
  if (tpg) { lv_timer_del(tpg); tpg = nullptr; }
  tpg = lv_timer_create(pgUpd, 40, NULL);
  gGameReg(pgScr);
  nav(pgScr, LV_SCR_LOAD_ANIM_MOVE_LEFT);
}

// ============================================================
//  SPACE INVADERS - 3 vulcoes de aliens, escudos e onda
// ============================================================
static lv_obj_t *siScr, *siShip, *siSco, *siVidas, *siWaveL;
static lv_obj_t *siAl[3][5];
static lv_obj_t *siBull[3];
static lv_obj_t *siEsc[3];
static int siX, siDir, siVidasN, siScoN, siWave, siTick, siFim, siNAl;
static int siPx;                               // nave do jogador (independente do siX das Ondas)
static bool siBulAlive[3];
static int siBulY[3], siBulX[3];
static lv_obj_t *siBomba[3];                    // bombas que os aliens arremessam
static int  siBx[3], siBy[3];
static bool siBoAlive[3];
static int  siCd = 0;                           // cooldown de bomba
static void siLimpaTudo() {
  for (int l = 0; l < 3; l++) for (int c = 0; c < 5; c++) if (lv_obj_is_valid(siAl[l][c])) { lv_obj_del(siAl[l][c]); siAl[l][c] = nullptr; }
}
static void siNovaOnda() {
  siWave++;
  if (lv_obj_is_valid(siWaveL)) lv_label_set_text_fmt(siWaveL, "W%d", siWave);
  siNAl = 15;
  siDir = 1;
  siX = 4; siTick = 0;
  siCd = 40;                                   // 1a bomba demora p/ dar tempo
  for (int l = 0; l < 3; l++) {
    for (int c = 0; c < 5; c++) {
      if (lv_obj_is_valid(siAl[l][c])) { lv_obj_del(siAl[l][c]); siAl[l][c] = nullptr; }
      siAl[l][c] = lv_obj_create(siScr);
      lv_obj_set_pos(siAl[l][c], 10 + c * 23, 44 + l * 11);
      lv_obj_set_size(siAl[l][c], 9, 7);
      lv_obj_set_style_bg_color(siAl[l][c], l == 0 ? lv_color_hex(0xFF5C5C) : l == 1 ? lv_color_hex(0xFFA726) : lv_color_hex(0x3AD07F), 0);
      lv_obj_set_style_radius(siAl[l][c], 2, 0);
      lv_obj_set_style_shadow_width(siAl[l][c], 0, 0);
      lv_obj_clear_flag(siAl[l][c], LV_OBJ_FLAG_SCROLLABLE);
      lv_obj_clear_flag(siAl[l][c], LV_OBJ_FLAG_CLICKABLE);
    }
  }
}
static void siUpd(lv_timer_t *) {
  if (!gInGame) return;
  if (!lv_obj_is_valid(siScr) || lv_scr_act() != siScr) return;
  // nav segue o dedo SEMPRE (mesmo de game over) - dedo decide a posicao
  if (gTouchDown) {
    int tx = gTx - 6;
    if (tx < 0) tx = 0; if (tx > 116) tx = 116;
    siPx = tx;
  }
  if (lv_obj_is_valid(siShip)) lv_obj_set_pos(siShip, siPx, 104);
  for (int i = 0; i < 3; i++)
    if (lv_obj_is_valid(siBomba[i]) && siBoAlive[i] && !siFim) {
      lv_obj_set_pos(siBomba[i], siBx[i], siBy[i]);
      lv_obj_clear_flag(siBomba[i], LV_OBJ_FLAG_HIDDEN);
    } else if (lv_obj_is_valid(siBomba[i])) {
      lv_obj_add_flag(siBomba[i], LV_OBJ_FLAG_HIDDEN);
      siBoAlive[i] = false;
    }
  if (siFim) return;
  // aliens (siX desloca a esquadra; NAO interfere na nave)
  if (++siTick >= (siWave > 2 ? 6 : 8)) {
    siTick = 0;
    siX += siDir * 4;
    if (siX > 8) siDir = -1;
    if (siX < -8) siDir = 1;
    for (int l = 0; l < 3; l++) for (int c = 0; c < 5; c++) if (lv_obj_is_valid(siAl[l][c]))
      lv_obj_set_pos(siAl[l][c], 10 + c * 23 + siX, 44 + l * 11);
  }
  // bombas dos aliens
  if (--siCd <= 0) {
    siCd = (siWave > 1 ? 24 : 40);
    int cand = 0;
    for (int l = 0; l < 3; l++) for (int c = 0; c < 5; c++) if (lv_obj_is_valid(siAl[l][c])) cand++;
    if (cand) {
      int pick = esp_random() % cand, i2 = 0, ax = 0, ay = 0;
      for (int l = 0; l < 3; l++) for (int c = 0; c < 5; c++) {
        if (!lv_obj_is_valid(siAl[l][c])) continue;
        if (i2++ == pick) {
          ax = lv_obj_get_x(siAl[l][c]) + 4;
          ay = lv_obj_get_y(siAl[l][c]) + 7;
          l = 3; break;
        }
      }
      for (int i = 0; i < 3; i++) if (!siBoAlive[i]) {
        siBoAlive[i] = true; siBx[i] = ax; siBy[i] = ay; break;
      }
    }
  }
  for (int i = 0; i < 3; i++) {
    if (!siBoAlive[i]) continue;
    siBy[i] += 5;
    bool vivoB = siBy[i] < 112;
    if (siBy[i] >= 100 && siBx[i] + 2 >= siPx && siBx[i] <= siPx + 14) {  // acertou a nave
      vivoB = false;
      if (siVidasN > 0) siVidasN--;
      if (lv_obj_is_valid(siVidas)) {
        if (siVidasN > 0) lv_label_set_text_fmt(siVidas, "vidas %d", siVidasN);
        else lv_label_set_text(siVidas, "GAME OVER");
      }
      if (siVidasN <= 0) { siFim = 1; toast("GAME OVER - toque p/ reiniciar"); }
      beepShort(600, 60);
    }
    if (siBoAlive[i] && !vivoB) siBoAlive[i] = false;
  }
  // tiro: novo toque dispara (balas VISIVEIS agora)
  static bool ant = false;
  bool toq = gTouchDown;
  if (toq && !ant) {
    for (int i = 0; i < 3; i++) if (!siBulAlive[i]) {
      siBulAlive[i] = true;
      siBulX[i] = siPx + 5; siBulY[i] = 100;
      break;
    }
  }
  ant = toq;
  for (int i = 0; i < 3; i++) {
    if (!lv_obj_is_valid(siBull[i])) { siBulAlive[i] = false; continue; }
    if (!siBulAlive[i]) { lv_obj_add_flag(siBull[i], LV_OBJ_FLAG_HIDDEN); continue; }
    siBulY[i] -= 6;
    if (siBulY[i] < 40 || siFim) { siBulAlive[i] = false; lv_obj_add_flag(siBull[i], LV_OBJ_FLAG_HIDDEN); continue; }
    lv_obj_set_pos(siBull[i], siBulX[i], siBulY[i]);
    lv_obj_clear_flag(siBull[i], LV_OBJ_FLAG_HIDDEN);
    for (int l = 0; l < 3; l++) for (int c = 0; c < 5; c++) {
      lv_obj_t *a = siAl[l][c];
      if (!lv_obj_is_valid(a)) continue;
      lv_coord_t ax = lv_obj_get_x(a), ay = lv_obj_get_y(a);
      if (siBulX[i] + 2 >= ax && siBulX[i] <= ax + 9 && siBulY[i] + 6 >= ay && siBulY[i] <= ay + 7) {
        lv_obj_del(a); siAl[l][c] = nullptr;
        siBulAlive[i] = false;
        lv_obj_add_flag(siBull[i], LV_OBJ_FLAG_HIDDEN);
        siScoN += 10; siNAl--;
        if (lv_obj_is_valid(siSco)) lv_label_set_text_fmt(siSco, "pts %d", siScoN);
        beepShort(2200, 20);
      }
    }
  }
  if (siNAl <= 0 && !siFim) siNovaOnda();
}
static void siTapCb(lv_event_t *) {             // game over: toque reinicia
  if (!siFim) return;
  siVidasN = 3; siScoN = 0; siWave = 0; siFim = 0; siPx = 58;
  if (lv_obj_is_valid(siSco)) lv_label_set_text_fmt(siSco, "pts %d", siScoN);
  if (lv_obj_is_valid(siVidas)) lv_label_set_text(siVidas, "vidas 3");
  siNovaOnda();
}
void buildInvaders() {
  siScr = lv_obj_create(NULL);
  scrStyle(siScr);
  gameHeader(siScr);
  lv_obj_clear_flag(siScr, LV_OBJ_FLAG_SCROLLABLE);
  siSco = uiLabel(siScr, "pts 0", 0, 20, F_SM, 0xF4F5F7);
  siVidas = uiLabel(siScr, "vidas 3", 66, 20, F_SM, 0xFF5C5C);
  siWaveL = uiLabel(siScr, "W1", 100, 20, F_SM, lv_color_to32(REDMI_ACC));
  siShip = lv_obj_create(siScr);
  lv_obj_set_pos(siShip, 8, 104);
  lv_obj_set_size(siShip, 12, 6);
  lv_obj_set_style_bg_color(siShip, REDMI_ACC, 0);
  lv_obj_set_style_radius(siShip, 3, 0);
  lv_obj_set_style_shadow_width(siShip, 0, 0);
  lv_obj_clear_flag(siShip, LV_OBJ_FLAG_SCROLLABLE);
  // balas do jogador (visiveis: 2x6 brancos)
  for (int i = 0; i < 3; i++) {
    siBull[i] = lv_obj_create(siScr);
    lv_obj_set_size(siBull[i], 2, 6);
    lv_obj_set_style_bg_color(siBull[i], lv_color_hex(0xF4F5F7), 0);
    lv_obj_set_style_radius(siBull[i], 1, 0);
    lv_obj_set_style_border_opa(siBull[i], 0, 0);
    lv_obj_set_style_shadow_width(siBull[i], 0, 0);
    lv_obj_clear_flag(siBull[i], LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(siBull[i], LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(siBull[i], LV_OBJ_FLAG_HIDDEN);
  }
  // bombas dos aliens (2x4 laranjas)
  for (int i = 0; i < 3; i++) {
    siBomba[i] = lv_obj_create(siScr);
    lv_obj_set_size(siBomba[i], 2, 4);
    lv_obj_set_style_bg_color(siBomba[i], lv_color_hex(0xFFA726), 0);
    lv_obj_set_style_radius(siBomba[i], 1, 0);
    lv_obj_set_style_border_opa(siBomba[i], 0, 0);
    lv_obj_set_style_shadow_width(siBomba[i], 0, 0);
    lv_obj_clear_flag(siBomba[i], LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(siBomba[i], LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(siBomba[i], LV_OBJ_FLAG_HIDDEN);
  }
  uiLabel(siScr, "arraste move  -  toque atira", 0, 122, F_SM, 0x676C7A);
  lv_obj_set_width(lv_obj_get_child(siScr, -1), 128);
  lv_obj_set_style_text_align(lv_obj_get_child(siScr, -1), LV_TEXT_ALIGN_CENTER, 0);
  for (int i = 0; i < 3; i++) siEsc[i] = nullptr;
  siVidasN = 3; siScoN = 0; siWave = 0; siFim = 0; siX = 0; siPx = 58; siCd = 40;
  for (int i = 0; i < 3; i++) { siBulAlive[i] = false; siBoAlive[i] = false; }
  siNovaOnda();
  lv_obj_add_event_cb(siScr, siTapCb, LV_EVENT_CLICKED, NULL);
  lv_obj_set_width(lv_obj_get_child(siScr, -1), 128);
  lv_obj_set_style_text_align(lv_obj_get_child(siScr, -1), LV_TEXT_ALIGN_CENTER, 0);
  static lv_timer_t *tsi = nullptr;
  if (tsi) { lv_timer_del(tsi); tsi = nullptr; }
  tsi = lv_timer_create(siUpd, 45, NULL);
  gGameReg(siScr);
  nav(siScr, LV_SCR_LOAD_ANIM_MOVE_LEFT);
}
// ============================================================
//  DAMA (regras brasileiras: captura obrigatoria, multi-salto,
//  dama qualquer distancia) - 2 jogadores ou vs CPU
// ============================================================
static lv_obj_t *dmScr, *dmCell[8][8], *dmPc[8][8];
static lv_obj_t *dmInfo, *dmModo;
static int dmP[8][8];            // 0 vazio 1 clara 2 escura 3 dama clara 4 dama escura
static int dmVez, dmFim, dmModoCPU;
static int dmSelX = -1, dmSelY = -1;
static bool dmContinua = false;  // multi-captura em andamento
static int dmCapObrig = 0;       // 0=sem captura  1=captura disponivel

static int dmCor(int p)  { return (p == 1 || p == 3) ? 1 : 2; }
static bool dmEhDama(int p) { return p == 3 || p == 4; }
static bool dmDentro(int x, int y) { return x >= 0 && x < 8 && y >= 0 && y < 8; }

static bool dmCapsPeca(int x0, int y0, int &x1, int &y1, int &x2, int &y2, bool &dama) {
  // procura UMA captura possivel desta peca
  int p = dmP[y0][x0], cor = dmCor(p), dir = (cor == 1) ? 1 : -1;
  int d2[4][2] = { {1, dir}, {-1, dir}, {1, -dir}, {-1, -dir} };
  if (dmEhDama(p)) {
    for (int k = 0; k < 4; k++) {
      int dx = d2[k][0], dy = d2[k][1];
      int cx = x0 + dx, cy = y0 + dy;
      bool achouPeca = false;
      int ex = 0, ey = 0;
      while (dmDentro(cx, cy)) {
        int q = dmP[cy][cx];
        if (q) {
          if (!achouPeca && dmCor(q) != cor) { achouPeca = true; ex = cx; ey = cy; }
          else break;
        } else if (achouPeca) {
          x1 = ex; y1 = ey; x2 = cx; y2 = cy; dama = dmEhDama(q);
          return true;
        }
        cx += dx; cy += dy;
      }
    }
    return false;
  }
  for (int k = 0; k < 4; k++) {
    int cx = x0 + d2[k][0], cy = y0 + d2[k][1];
    int lx = x0 + d2[k][0] * 2, ly = y0 + d2[k][1] * 2;
    if (!dmDentro(cx, cy) || !dmDentro(lx, ly)) continue;
    int q = dmP[cy][cx];
    if (q && dmCor(q) != cor && dmP[ly][lx] == 0) {
      x1 = cx; y1 = cy; x2 = lx; y2 = ly; dama = false;
      return true;
    }
  }
  return false;
}
static int dmExisteCaptura(int cor, int &x0, int &y0) {
  for (int y = 0; y < 8; y++) for (int x = 0; x < 8; x++) {
    if (!dmP[y][x] || dmCor(dmP[y][x]) != cor) continue;
    int a, b, c, d; bool e;
    if (dmCapsPeca(x, y, a, b, c, d, e)) { x0 = x; y0 = y; return 1; }
  }
  return 0;
}
// a cor tem alguma jogada legal (captura ou movimento simples)?
static bool dmMovSimples(int x0, int y0, int x2, int y2);
static bool dmTemMovimento(int cor) {
  for (int y = 0; y < 8; y++) for (int x = 0; x < 8; x++) {
    if (!dmP[y][x] || dmCor(dmP[y][x]) != cor) continue;
    int a, b, c, d; bool e;
    if (dmCapsPeca(x, y, a, b, c, d, e)) return true;
    for (int dy = -1; dy <= 1; dy += 2) for (int dx = -1; dx <= 1; dx += 2)
      if (dmMovSimples(x, y, x + dx, y + dy)) return true;
  }
  return false;
}
// captura obrigatoria: so' a cor que vai jogar e' considerada
static void dmAtualizaCapObrig() {
  int a = 0, b = 0;
  dmCapObrig = dmExisteCaptura(dmVez, a, b) ? 1 : 0;
}
static bool dmMovSimples(int x0, int y0, int x2, int y2) {
  int p = dmP[y0][x0], cor = dmCor(p);
  if (!dmDentro(x2, y2) || dmP[y2][x2]) return false;
  if (dmEhDama(p)) {
    int dx = (x2 > x0) ? 1 : -1, dy = (y2 > y0) ? 1 : -1;
    if (abs(x2 - x0) != abs(y2 - y0) || x2 == x0) return false;
    int cx = x0 + dx, cy = y0 + dy;
    while (cx != x2 && cy != y2) { if (dmP[cy][cx]) return false; cx += dx; cy += dy; }
    return true;
  }
  int dir = (cor == 1) ? 1 : -1;
  return (y2 == y0 + dir && abs(x2 - x0) == 1);
}
// reinicia a partida (tabuleiro inicial)
static void dmDesenha();
static void dmNovoJogo() {
  for (int y = 0; y < 8; y++) for (int x = 0; x < 8; x++) dmP[y][x] = 0;
  for (int y = 0; y < 3; y++) for (int x = 0; x < 8; x++) if (((x + y) & 1)) dmP[y][x] = 2;
  for (int y = 5; y < 8; y++) for (int x = 0; x < 8; x++) if (((x + y) & 1)) dmP[y][x] = 1;
  dmVez = 1; dmFim = 0; dmSelX = dmSelY = -1; dmContinua = false;
  dmAtualizaCapObrig();
  if (lv_obj_is_valid(dmInfo)) {
    lv_label_set_text(dmInfo, "sua vez (claras)");
    lv_obj_set_style_text_color(dmInfo, lv_color_hex(0xF4F5F7), 0);
    lv_obj_add_flag(dmInfo, LV_OBJ_FLAG_CLICKABLE);
  }
  dmDesenha();
}
static void dmDesenha() {
  for (int y = 0; y < 8; y++) for (int x = 0; x < 8; x++) {
    if (!lv_obj_is_valid(dmCell[y][x])) continue;
    bool esc = ((x + y) & 1);
    lv_obj_set_style_bg_color(dmCell[y][x], esc ? lv_color_hex(0x1A2430) : lv_color_hex(0x2A3444), 0);
    if (dmSelX == x && dmSelY == y) lv_obj_set_style_bg_color(dmCell[y][x], REDMI_ACC, 0);
    int p = dmP[y][x];
    if (lv_obj_is_valid(dmPc[y][x])) {
      if (p == 0) { lv_obj_add_flag(dmPc[y][x], LV_OBJ_FLAG_HIDDEN); continue; }
      lv_obj_clear_flag(dmPc[y][x], LV_OBJ_FLAG_HIDDEN);
      bool dama = dmEhDama(p);
      bool clara = (dmCor(p) == 1);
      lv_obj_set_size(dmPc[y][x], dama ? 12 : 11, dama ? 12 : 11);
      lv_obj_set_style_bg_color(dmPc[y][x], clara ? lv_color_hex(0xFFE082) : lv_color_hex(0xE0E4EC), 0);
      lv_obj_set_style_border_color(dmPc[y][x], clara ? lv_color_hex(0x8D6E63) : lv_color_hex(0x9AA3B5), 0);
      lv_obj_set_style_border_width(dmPc[y][x], 1, 0);
      lv_label_set_text(lv_obj_get_child(dmPc[y][x], 0), dama ? "D" : "");
      lv_obj_set_pos(dmPc[y][x], (dmSelX == -1 ? 0 : 0) + (14 - (dama ? 12 : 11)) / 2 + (dmSelX == x ? 0 : 0),
                     (14 - (dama ? 12 : 11)) / 2);
    }
  }
}
static void dmFimJogo(const char *t) {
  dmFim = 1;
  if (lv_obj_is_valid(dmInfo)) {
    lv_label_set_text(dmInfo, t);
    lv_obj_set_style_text_color(dmInfo, REDMI_ACC, 0);
    lv_obj_add_flag(dmInfo, LV_OBJ_FLAG_CLICKABLE);   // toque reinicia
  }
  toast(t);
}

// aplica o movimento. cap=true quando foi captura (pode exigir multi-salto)
static void dmAplica(int x0, int y0, int x2, int y2, bool cap) {
  int p = dmP[y0][x0];
  dmP[y0][x0] = 0;
  dmP[y2][x2] = p;
  if (p && !dmEhDama(p)) {
    if ((dmCor(p) == 1 && y2 == 7) || (dmCor(p) == 2 && y2 == 0))
      dmP[y2][x2] = (p == 1) ? 3 : 4;                     // promocao
  }
  // multi-salto: se a peca (normal OU dama) ainda pode capturar, continua
  if (cap) {
    int a, b, c, d; bool e;
    if (dmCapsPeca(x2, y2, a, b, c, d, e)) {
      dmSelX = x2; dmSelY = y2; dmContinua = true;
      dmDesenha();
      if (lv_obj_is_valid(dmInfo))
        lv_label_set_text(dmInfo, dmVez == 1 ? "capture de novo (claras)" : "capture de novo (escuras)");
      return;
    }
  }
  dmContinua = false;
  dmSelX = dmSelY = -1;
  dmVez = (dmVez == 1) ? 2 : 1;
  dmAtualizaCapObrig();
  bool temP1 = false, temP2 = false;
  for (int y = 0; y < 8; y++) for (int x = 0; x < 8; x++) { if (dmP[y][x] == 1 || dmP[y][x] == 3) temP1 = true; if (dmP[y][x] == 2 || dmP[y][x] == 4) temP2 = true; }
  if (!temP2) dmFimJogo("Vitoria das claras!");
  else if (!temP1) dmFimJogo("Vitoria das escuras!");
  else if (!dmTemMovimento(dmVez)) dmFimJogo(dmVez == 1 ? "Escuras sem jogada!" : "Claras sem jogada!");
  if (lv_obj_is_valid(dmInfo)) lv_label_set_text(dmInfo, dmVez == 1 ? "sua vez (claras)" : "vez das escuras");
  dmDesenha();
}
static void dmClickCb(lv_event_t *e) {
  if (dmFim) return;
  lv_obj_t *c = lv_event_get_target(e);
  int x = -1, y = -1;
  for (int yy = 0; yy < 8 && y < 0; yy++) for (int xx = 0; xx < 8; xx++)
    if (dmCell[yy][xx] == c) { x = xx; y = yy; break; }
  if (x < 0) return;
  int cor = dmVez;
  int p = dmP[y][x];
  // peca ja selecionada: tenta capturar ou mover (so' no turno do jogador)
  if (dmSelX >= 0 && (x != dmSelX || y != dmSelY) && (dmModoCPU == 0 || dmVez == 1)) {
    int cx1, cy1, cx2, cy2; bool dama;
    if (dmCapsPeca(dmSelX, dmSelY, cx1, cy1, cx2, cy2, dama) && x == cx2 && y == cy2) {
      dmP[cy1][cx1] = 0;                                  // peca capturada
      dmAplica(dmSelX, dmSelY, x, y, true);
      return;
    }
    if (!dmContinua && dmMovSimples(dmSelX, dmSelY, x, y) && !dmCapObrig) {
      dmAplica(dmSelX, dmSelY, x, y, false);
      return;
    }
  }
  // seleciona peca propria (durante multi-salto nao troca de peca)
  if (!dmContinua && p && dmCor(p) == cor && (dmModoCPU == 0 || dmVez == 1)) {
    // se ha captura obrigatoria, so' pode selecionar peca que captura
    if (dmCapObrig) {
      int a, b, c, d; bool e;
      if (!dmCapsPeca(x, y, a, b, c, d, e)) return;
    }
    dmSelX = x; dmSelY = y;
    dmDesenha();
  }
}
// CPU: escolhe a captura ou o movimento que mais avanca
static void dmCpuJoga() {
  if (dmFim || dmVez != 2) return;
  int mx[24][4], n = 0;
  // multi-salto em andamento: a CPU so' pode continuar com a peca selecionada
  if (dmContinua && dmSelX >= 0) {
    int cx1, cy1, cx2, cy2; bool dama;
    if (dmCapsPeca(dmSelX, dmSelY, cx1, cy1, cx2, cy2, dama)) {
      dmP[cy1][cx1] = 0;
      dmAplica(dmSelX, dmSelY, cx2, cy2, true);
      return;
    }
    dmContinua = false;   // sem continuacao: encerra o turno
  }
  for (int y = 0; y < 8 && n < 24; y++) for (int x = 0; x < 8 && n < 24; x++) {
    int p = dmP[y][x];
    if (!p || dmCor(p) != 2) continue;
    int a, b, c, d; bool dama;
    if (dmCapsPeca(x, y, a, b, c, d, dama)) { mx[n][0] = x; mx[n][1] = y; mx[n][2] = c; mx[n][3] = d; n++; }
  }
  if (n) {
    int i = esp_random() % n;
    int cx1, cy1, cx2, cy2; bool dama;
    dmCapsPeca(mx[i][0], mx[i][1], cx1, cy1, cx2, cy2, dama);
    dmP[cy1][cx1] = 0;
    dmAplica(mx[i][0], mx[i][1], mx[i][2], mx[i][3], true);
    return;
  }
  n = 0;
  for (int y = 0; y < 8 && n < 24; y++) for (int x = 0; x < 8 && n < 24; x++) {
    int p = dmP[y][x];
    if (!p || dmCor(p) != 2) continue;
    for (int dy = -1; dy <= 1; dy += 2) for (int dx = -1; dx <= 1; dx += 2) {
      if (dmMovSimples(x, y, x + dx, y + dy) && n < 24) { mx[n][0] = x; mx[n][1] = y; mx[n][2] = x + dx; mx[n][3] = y + dy; n++; }
    }
  }
  if (n) {
    int melhor = 0, bestY = -1;
    for (int i = 0; i < n; i++) {
      int y2 = mx[i][3];
      if (y2 < bestY || bestY < 0) { bestY = y2; melhor = i; }   // escuras descem (y menor)
    }
    dmAplica(mx[melhor][0], mx[melhor][1], mx[melhor][2], mx[melhor][3], false);
  } else {
    dmFimJogo("Claras sem jogada!");
  }
}
static void dmCpuTimer(lv_timer_t *) {
  if (!gInGame) return;
  if (!lv_obj_is_valid(dmScr) || lv_scr_act() != dmScr) return;
  if (dmFim) return;
  if (dmVez == 2 && dmModoCPU == 1) dmCpuJoga();
}
static void dmModoCb(lv_event_t *) {
  dmModoCPU = !dmModoCPU;
  if (lv_obj_is_valid(dmModo)) lv_label_set_text(lv_obj_get_child(dmModo, 0), dmModoCPU ? "1P vs CPU" : "2 jogadores");
}
void buildDama() {
  dmScr = lv_obj_create(NULL);
  scrStyle(dmScr);
  gameHeader(dmScr);
  lv_obj_clear_flag(dmScr, LV_OBJ_FLAG_SCROLLABLE);
  dmInfo = uiLabel(dmScr, "sua vez (claras)", 24, 18, F_SM, 0xF4F5F7);
  lv_obj_set_width(dmInfo, 104);
  lv_obj_set_style_text_align(dmInfo, LV_TEXT_ALIGN_CENTER, 0);
  dmModo = uiBtn(dmScr, "1P vs CPU", dmModoCb, 62, 16, 0x1C202B, NULL);
  if (dmModo) lv_obj_set_pos(dmModo, 33, 0);
  // tabuleiro 8x8 de 15px em (0,34)
  for (int y = 0; y < 8; y++) for (int x = 0; x < 8; x++) {
    dmCell[y][x] = lv_obj_create(dmScr);
    lv_obj_set_pos(dmCell[y][x], x * 16, 34 + y * 15);
    lv_obj_set_size(dmCell[y][x], 15, 15);
    lv_obj_set_style_radius(dmCell[y][x], 1, 0);
    lv_obj_set_style_shadow_width(dmCell[y][x], 0, 0);
    lv_obj_set_style_pad_all(dmCell[y][x], 0, 0);
    lv_obj_set_style_border_opa(dmCell[y][x], 0, 0);
    lv_obj_add_event_cb(dmCell[y][x], dmClickCb, LV_EVENT_CLICKED, NULL);
    lv_obj_clear_flag(dmCell[y][x], LV_OBJ_FLAG_SCROLLABLE);
    dmPc[y][x] = lv_obj_create(dmCell[y][x]);
    lv_obj_set_size(dmPc[y][x], 12, 12);
    lv_obj_set_pos(dmPc[y][x], 1, 1);
    lv_obj_set_style_radius(dmPc[y][x], LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_shadow_width(dmPc[y][x], 0, 0);
    lv_obj_clear_flag(dmPc[y][x], LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(dmPc[y][x], LV_OBJ_FLAG_CLICKABLE);
    lv_label_create(dmPc[y][x]);                             // rotulo (D da dama)
    if (lv_obj_get_child(dmPc[y][x], 0)) {
      lv_obj_center(lv_obj_get_child(dmPc[y][x], 0));
      lv_obj_set_style_text_font(lv_obj_get_child(dmPc[y][x], 0), F_SM, 0);
      lv_obj_set_style_text_color(lv_obj_get_child(dmPc[y][x], 0), lv_color_hex(0x4E342E), 0);
    }
    lv_obj_add_flag(dmPc[y][x], LV_OBJ_FLAG_HIDDEN);

  }
  for (int y = 0; y < 8; y++) for (int x = 0; x < 8; x++) dmP[y][x] = 0;
  for (int y = 0; y < 3; y++) for (int x = 0; x < 8; x++) if (((x + y) & 1)) dmP[y][x] = 2;
  for (int y = 5; y < 8; y++) for (int x = 0; x < 8; x++) if (((x + y) & 1)) dmP[y][x] = 1;
  dmVez = 1; dmFim = 0; dmSelX = dmSelY = -1; dmContinua = false; dmModoCPU = 1;
  dmAtualizaCapObrig();
  if (lv_obj_is_valid(dmModo)) lv_label_set_text(lv_obj_get_child(dmModo, 0), "1P vs CPU");
  dmDesenha();
  // tocar no aviso de fim de partida reinicia o jogo
  lv_obj_add_event_cb(dmInfo, [](lv_event_t *) {
    if (!dmFim) return;
    dmNovoJogo();
    toast("novo jogo!");
  }, LV_EVENT_CLICKED, NULL);
  static lv_timer_t *tdm = nullptr;
  if (tdm) { lv_timer_del(tdm); tdm = nullptr; }
  tdm = lv_timer_create(dmCpuTimer, 900, NULL);
  gGameReg(dmScr);
  nav(dmScr, LV_SCR_LOAD_ANIM_MOVE_LEFT);
}

// ============================================================
//  PECAS DE XADREZ DESENHADAS (formas reais em canvas LVGL)
//  desenha em buffer 18x18 RGB565: rei/rainha/bispo/torre/cavalo/peao
// ============================================================
static const int PC_SZ = 15;                       // canvas da peca (15x15 = celula)
static void pxSet(uint16_t *b, int x, int y, uint16_t c) {
  if (x < 0 || y < 0 || x >= 18 || y >= 18) return;      // desenho em espaco 18x18
  int sx = x * (PC_SZ - 1) / 17, sy = y * (PC_SZ - 1) / 17;
  b[sy * PC_SZ + sx] = c;
}
static void pxDisc(uint16_t *b, int x0, int y0, int r, uint16_t c) {
  for (int y = -r; y <= r; y++) for (int x = -r; x <= r; x++)
    if (x * x + y * y <= r * r + r) pxSet(b, x0 + x, y0 + y, c);
}
static void pxRect(uint16_t *b, int x0, int y0, int w, int h, uint16_t c) {
  for (int y = y0; y < y0 + h; y++) for (int x = x0; x < x0 + w; x++) pxSet(b, x, y, c);
}
// corpo/Base compartilhado
static void pxBase(uint16_t *b, uint16_t c) {
  pxRect(b, 3, 15, 12, 2, c);                     // base
  pxRect(b, 5, 13, 8, 2, c);
  pxRect(b, 7, 12, 4, 1, c);
  pxRect(b, 7, 3, 4, 9, c);                       // tronco
}
static void pxRei(uint16_t *b, uint16_t c) {      // coroa com cruz
  pxBase(b, c);
  pxRect(b, 8, 0, 2, 4, c);
  pxRect(b, 6, 2, 6, 2, c);
  pxRect(b, 5, 4, 8, 3, c);
}
static void pxRainha(uint16_t *b, uint16_t c) {   // coroa com 3 pontas
  pxBase(b, c);
  pxDisc(b, 6, 4, 1, c);
  pxDisc(b, 12, 4, 1, c);
  pxDisc(b, 9, 2, 2, c);
  pxRect(b, 6, 5, 6, 4, c);
  pxRect(b, 6, 4, 1, 1, c);
  pxRect(b, 11, 4, 1, 1, c);
}
static void pxBispo(uint16_t *b, uint16_t c) {    // mitra com cruz no topo
  pxBase(b, c);
  pxDisc(b, 9, 5, 4, c);
  pxRect(b, 8, 1, 2, 2, c);
  pxRect(b, 6, 3, 6, 2, c);
}
static void pxTorre(uint16_t *b, uint16_t c) {    // torre com ameias
  pxBase(b, c);
  pxRect(b, 5, 3, 8, 8, c);
  pxRect(b, 5, 1, 2, 2, c);
  pxRect(b, 8, 1, 2, 2, c);
  pxRect(b, 11, 1, 2, 2, c);
  pxRect(b, 8, 6, 2, 5, c);
}
static void pxCavalo(uint16_t *b, uint16_t c) {   // silhueta de cavalo
  pxBase(b, c);
  pxRect(b, 5, 3, 4, 6, c);                       // cabeca
  pxRect(b, 9, 3, 3, 8, c);                        // pescoco
  pxRect(b, 6, 1, 5, 3, c);
  pxRect(b, 10, 1, 2, 3, c);
  pxRect(b, 11, 2, 2, 4, c);
  pxRect(b, 5, 6, 2, 5, c);
}
static void pxPeao(uint16_t *b, uint16_t c) {
  pxBase(b, c);
  pxDisc(b, 9, 6, 3, c);
  pxDisc(b, 9, 3, 2, c);
}
// desenha a peca no buffer (ptr) conforme o tipo 1..6
static void desenhaPeca(uint16_t *b, int tipo, bool branca, uint16_t bg) {
  uint16_t cor  = branca ? 0xFFFF : 0x1840;       // branco / preto
  for (int i = 0; i < PC_SZ * PC_SZ; i++) b[i] = bg;
  switch (tipo) {
    case 1: pxPeao(b, cor);    break;
    case 2: pxCavalo(b, cor);  break;
    case 3: pxBispo(b, cor);   break;
    case 4: pxTorre(b, cor);   break;
    case 5: pxRainha(b, cor);  break;
    case 6: pxRei(b, cor);     break;
    default: break;
  }
}


// ============================================================
//  XADREZ (movimentos completos, xeque/xeque-mate, roque,
//  promocao p/ dama, 2 jogadores ou vs CPU) - notacao PT
// ============================================================
static lv_obj_t *xzScr, *xzCell[8][8], *xzPc[8][8];
static lv_obj_t *xzInfo, *xzModo;
static int xzP[8][8];             // 0 vazio; + brancas: P1 C2 B3 T4 D5 R6; - pretas
static uint16_t *xzBufPeca[64];               // buffers das pecas (PSRAM)
static int xzVez;                 // +1 brancas (voce)  -1 pretas (CPU)
static int xzSelX = -1, xzSelY = -1;
static int xzFim, xzModoCPU;
static bool xzReiMoveu, xzTorreA, xzTorreH;   // p/ roque

static void xzDesenha();
static bool xzLivreEntre(int, int, int, int);
static int xzCor(int p) { return (p > 0) ? 1 : (p ? -1 : 0); }
static bool xzDentro(int x, int y) { return x >= 0 && x < 8 && y >= 0 && y < 8; }
static const char* xzNome(int p) {
  switch (abs(p)) {
    case 1: return "P"; case 2: return "C"; case 3: return "B";
    case 4: return "T"; case 5: return "D"; case 6: return "R";
  }
  return "";
}
static bool xzLivreEntre(int x0, int y0, int x1, int y1) {
  // NAO ordena: usa a direcao real (senao a anti-diagonal ? quebra)
  int dx = x1 - x0, dy = y1 - y0;
  if (dx == 0 && dy == 0) return true;
  if (dx == 0) {                                    // vertical
    int st = (dy > 0) ? y0 : y1;
    for (int y = st + 1; y < st + abs(dy); y++) if (xzP[y][x0]) return false;
    return true;
  }
  if (dy == 0) {                                    // horizontal
    int st = (dx > 0) ? x0 : x1;
    for (int x = st + 1; x < st + abs(dx); x++) if (xzP[y0][x]) return false;
    return true;
  }
  if (abs(dx) != abs(dy)) return false;             // nao e' linha reta
  int sx = (dx > 0) ? 1 : -1, sy = (dy > 0) ? 1 : -1;
  int x = x0 + sx, y = y0 + sy;
  while (x != x1 && y != y1) { if (xzP[y][x]) return false; x += sx; y += sy; }
  return true;
}
static bool xzAtaca(int x0, int y0, int x1, int y1) {   // (x1,y1) ataca (x0,y0)?
  int p = xzP[y1][x1];
  if (!p) return false;
  int cor = xzCor(p), t = abs(p);
  int dx = abs(x0 - x1), dy = abs(y0 - y1);
  if (cor == 0) return false;
  if (t == 1) {                                          // peao
    int dir = (cor == 1) ? -1 : 1;
    return (y0 == y1 + dir && dx == 1);
  }
  if (t == 2) return (dx == 1 && dy == 2) || (dx == 2 && dy == 1);
  if (t == 3) return (dx == dy && dx > 0) && xzLivreEntre(x0, y0, x1, y1);
  if (t == 4) return ((dx == 0) != (dy == 0)) && xzLivreEntre(x0, y0, x1, y1);
  if (t == 5) return ((dx == dy) != (dx == 0 || dy == 0)) && xzLivreEntre(x0, y0, x1, y1);
  if (t == 6) return dx <= 1 && dy <= 1 && (dx + dy > 0);
  return false;
}
static bool xzEmXeque(int cor) {                         // rei de 'cor' atacado?
  int kr = (cor == 1) ? 6 : -6;
  int kx = -1, ky = -1;
  for (int y = 0; y < 8; y++) for (int x = 0; x < 8; x++) if (xzP[y][x] == kr) { kx = x; ky = y; }
  if (kx < 0) return false;
  for (int y = 0; y < 8; y++) for (int x = 0; x < 8; x++)
    if (xzP[y][x] && xzCor(xzP[y][x]) != cor && xzAtaca(kx, ky, x, y)) return true;
  return false;
}
static bool xzMoveValido(int x0, int y0, int x2, int y2) {
  int p = xzP[y0][x0];
  if (!p || !xzDentro(x2, y2)) return false;
  if (xzCor(p) != xzVez) return false;
  int dx = x2 - x0, dy = y2 - y0, t = abs(p), alvo = xzP[y2][x2];
  if (xzCor(alvo) == xzVez) return false;
  int adx = abs(dx), ady = abs(dy);
  if (t == 1) {                                          // peao (sem en passant)
    int dir = (xzVez == 1) ? -1 : 1;
    int inicio = (xzVez == 1) ? 6 : 1;
    if (dy == dir && adx == 0 && !alvo) return true;
    if (dy == dir && adx == 1 && xzCor(alvo) == -xzVez) return true;
    if (dy == 2 * dir && adx == 0 && y0 == inicio && !alvo && !xzP[y0 + dir][x0]) return true;
    return false;
  }
  if (t == 2) return (adx == 1 && ady == 2) || (adx == 2 && ady == 1);
  if (t == 3) return adx == ady && adx > 0 && xzLivreEntre(x0, y0, x2, y2);
  if (t == 4) return (adx == 0) != (ady == 0) && xzLivreEntre(x0, y0, x2, y2);
  if (t == 5) return ((adx == ady) || (adx == 0) != (ady == 0)) && (adx + ady > 0) && xzLivreEntre(x0, y0, x2, y2);
  if (t == 6) return adx <= 1 && ady <= 1 && (adx + ady > 0);
  return false;
}
static bool xzRoque(int x0, int y0, int x2) {             // toque no rei -> roque
  if (xzVez == 1 && (xzReiMoveu || xzEmXeque(1))) return false;
  if (xzVez == -1 && (xzReiMoveu || xzEmXeque(-1))) return false;
  if (xzP[y0][x0] != ((xzVez == 1) ? 6 : -6)) return false;
  int lado = (x2 == 6) ? 1 : (x2 == 2 ? -1 : 0);
  if (!lado) return false;
  int ytorre = (xzVez == 1) ? 7 : 0;
  int xtorre = (lado == 1) ? 7 : 0;
  if (xzP[ytorre][xtorre] != ((xzVez == 1) ? 4 : -4)) return false;
  int xr = (lado == 1) ? 4 : 0;                          // coluna do rei
  int ini = (lado == 1) ? 4 : 3;
  for (int x = (lado == 1 ? 5 : 1); x <= xr + (lado == 1 ? 2 : 0); x++)
    if (xzP[ytorre][x]) return false;                    // caminho limpo
  if (lado == -1) { if (xzP[ytorre][1] || xzP[ytorre][2] || xzP[ytorre][3]) return false; }
  for (int i = 0; i < 2; i++) {                           // rei nao passa por xeque
    int rx = x0 + (lado == 1 ? i + 1 : -i - 1);
    // simula: so testa ataque direto sobre a casa do rei
    for (int y = 0; y < 8; y++) for (int x = 0; x < 8; x++)
      if (xzP[y][x] && xzCor(xzP[y][x]) != xzVez && xzAtaca(rx, ytorre, x, y)) return false;
  }
  return true;
}
static void xzAplica(int x0, int y0, int x2, int y2) {
  int p = xzP[y0][x0];
  if (abs(p) == 6) xzReiMoveu = true;
  if (abs(p) == 4 && x0 == 0) xzTorreA = true;
  if (abs(p) == 4 && x0 == 7) xzTorreH = true;
  xzP[y0][x0] = 0;
  xzP[y2][x2] = p;
  if (abs(p) == 1 && (y2 == 0 || y2 == 7)) xzP[y2][x2] = (xzVez == 1) ? 5 : -5;   // promocao
  // roque: move a torre
  if (abs(p) == 6 && abs(x2 - x0) == 2) {
    int ytorre = (xzVez == 1) ? 7 : 0;
    if (x2 == 6) { int t = xzP[ytorre][7]; xzP[ytorre][7] = 0; xzP[ytorre][5] = t; }
    if (x2 == 2) { int t = xzP[ytorre][0]; xzP[ytorre][0] = 0; xzP[ytorre][3] = t; }
  }
  xzVez = -xzVez;
  xzSelX = xzSelY = -1;
  bool xeque = xzEmXeque(xzVez);
  // existe algum movimento legal?
  bool tem = false;
  for (int y0i = 0; y0i < 8 && !tem; y0i++) for (int x0i = 0; x0i < 8 && !tem; x0i++) {
    if (!xzP[y0i][x0i] || xzCor(xzP[y0i][x0i]) != xzVez) continue;
    for (int y2i = 0; y2i < 8 && !tem; y2i++) for (int x2i = 0; x2i < 8 && !tem; x2i++) {
      if (!xzMoveValido(x0i, y0i, x2i, y2i)) continue;
      int a = xzP[y0i][x0i], b = xzP[y2i][x2i];
      xzP[y0i][x0i] = 0; xzP[y2i][x2i] = a;
      bool seguro = !xzEmXeque(xzVez);
      xzP[y0i][x0i] = a; xzP[y2i][x2i] = b;
      if (seguro) tem = true;
    }
  }
  if (!tem) {
    if (xeque) {
      xzFim = 1;
      if (lv_obj_is_valid(xzInfo)) {
        lv_label_set_text(xzInfo, xzVez == 1 ? "Xeque-mate: CPU ganhou" : "Xeque-mate: VOCE ganhou");
        lv_obj_set_style_text_color(xzInfo, REDMI_ACC, 0);
        lv_obj_add_flag(xzInfo, LV_OBJ_FLAG_CLICKABLE);
      }
      toast("XEQUE-MATE!");
    } else {
      xzFim = 2;
      if (lv_obj_is_valid(xzInfo)) {
        lv_label_set_text(xzInfo, "afogamento (empate)");
        lv_obj_set_style_text_color(xzInfo, REDMI_ACC, 0);
        lv_obj_add_flag(xzInfo, LV_OBJ_FLAG_CLICKABLE);
      }
    }
  } else if (xeque) {
    if (lv_obj_is_valid(xzInfo)) lv_label_set_text(xzInfo, xzVez == 1 ? "XEQUE no seu rei!" : "xeque no rei da CPU");
  } else if (lv_obj_is_valid(xzInfo)) lv_label_set_text(xzInfo, xzVez == 1 ? "sua vez (brancas)" : "CPU pensando...");
  xzDesenha();
}
// reinicia a partida de xadrez
static void xzDesenha();
static void xzNovoJogo() {
  for (int y = 0; y < 8; y++) for (int x = 0; x < 8; x++) xzP[y][x] = 0;
  for (int x = 0; x < 8; x++) { xzP[6][x] = 1;  xzP[1][x] = -1; }
  static const int ord[8] = { 4, 2, 3, 5, 6, 3, 2, 4 };
  for (int x = 0; x < 8; x++) { xzP[7][x] = ord[x]; xzP[0][x] = -ord[x]; }
  xzVez = 1; xzFim = 0; xzSelX = xzSelY = -1;
  xzReiMoveu = xzTorreA = xzTorreH = false;
  if (lv_obj_is_valid(xzInfo)) {
    lv_label_set_text(xzInfo, "sua vez (brancas)");
    lv_obj_set_style_text_color(xzInfo, lv_color_hex(0xF4F5F7), 0);
    lv_obj_add_flag(xzInfo, LV_OBJ_FLAG_CLICKABLE);
  }
  xzDesenha();
}
static void xzDesenha() {
  for (int y = 0; y < 8; y++) for (int x = 0; x < 8; x++) {
    if (!lv_obj_is_valid(xzCell[y][x])) continue;
    bool esc = ((x + y) & 1);
    uint32_t cellBg = esc ? 0x2A3444 : 0x3B465C;
    bool sel = (xzSelX == x && xzSelY == y);
    lv_obj_set_style_bg_color(xzCell[y][x], lv_color_hex(sel ? REDMI_ACC.full : cellBg), 0);
    int p = xzP[y][x];

    if (lv_obj_is_valid(xzPc[y][x])) {
      if (p == 0) { lv_obj_add_flag(xzPc[y][x], LV_OBJ_FLAG_HIDDEN); continue; }
      lv_obj_clear_flag(xzPc[y][x], LV_OBJ_FLAG_HIDDEN);
      lv_obj_set_style_bg_opa(xzPc[y][x], LV_OPA_COVER, 0);       // canvas mostra a peca
      lv_obj_set_style_bg_color(xzPc[y][x], lv_color_hex(sel ? REDMI_ACC.full : cellBg), 0);
      lv_obj_set_pos(xzPc[y][x], 0, 0);
      if (lv_obj_get_child(xzPc[y][x], 0)) {
        lv_obj_add_flag(lv_obj_get_child(xzPc[y][x], 0), LV_OBJ_FLAG_HIDDEN);   // esconde a letra
      }
      uint16_t *buf = xzBufPeca[y * 8 + x];
      if (buf) desenhaPeca(buf, abs(p), xzCor(p) == 1,
                           (uint16_t)lv_color_to16(lv_color_hex(sel ? REDMI_ACC.full : cellBg)));
      lv_obj_invalidate(xzPc[y][x]);
    }
  }
}
static void xzClickCb(lv_event_t *e) {
  if (xzFim) return;
  lv_obj_t *c = lv_event_get_target(e);
  int x = -1, y = -1;
  for (int yy = 0; yy < 8 && y < 0; yy++) for (int xx = 0; xx < 8; xx++)
    if (xzCell[yy][xx] == c) { x = xx; y = yy; break; }
  if (x < 0) return;
  if (xzSelX >= 0) {
    if (xzRoque(xzSelX, xzSelY, x)) {              // roque pelo toque no destino
      int ytorre = (xzVez == 1) ? 7 : 0;
      xzAplica(xzSelX, xzSelY, x, ytorre);
      return;
    }
    if (xzMoveValido(xzSelX, xzSelY, x, y)) {
      int a = xzP[xzSelY][xzSelX], b = xzP[y][x];
      xzP[xzSelY][xzSelX] = 0; xzP[y][x] = a;
      bool seguro = !xzEmXeque(xzVez);
      xzP[xzSelY][xzSelX] = a; xzP[y][x] = b;
      if (seguro) xzAplica(xzSelX, xzSelY, x, y);
      else { xzSelX = xzSelY = -1; xzDesenha(); }
      return;
    }
  }
  xzSelX = x; xzSelY = y;
  xzDesenha();
}
static int xzVal(int p) {
  switch (abs(p)) { case 1: return 10; case 2: return 30; case 3: return 30;
                    case 4: return 50; case 5: return 90; case 6: return 1000; }
  return 0;
}
static void xzCpuJoga() {
  if (xzFim || xzVez != -1 || xzModoCPU != 1) return;
  int bx0 = -1, by0 = -1, bx2 = -1, by2 = -1, best = -99999;
  for (int y0 = 0; y0 < 8; y0++) for (int x0 = 0; x0 < 8; x0++) {
    if (!xzP[y0][x0] || xzCor(xzP[y0][x0]) != -1) continue;
    for (int y2 = 0; y2 < 8; y2++) for (int x2 = 0; x2 < 8; x2++) {
      if (!xzMoveValido(x0, y0, x2, y2)) continue;
      int a = xzP[y0][x0], b = xzP[y2][x2];
      xzP[y0][x0] = 0; xzP[y2][x2] = a;
      bool seguro = !xzEmXeque(-1);
      int sc = xzVal(b) - xzVal(a) / 10 + (esp_random() % 7);
      xzP[y0][x0] = a; xzP[y2][x2] = b;
      if (seguro && sc > best) { best = sc; bx0 = x0; by0 = y0; bx2 = x2; by2 = y2; }
    }
  }
  if (bx0 >= 0) xzAplica(bx0, by0, bx2, by2);
}
static void xzCpuTimer(lv_timer_t *) {
  if (!gInGame) return;
  if (!lv_obj_is_valid(xzScr) || lv_scr_act() != xzScr) return;
  if (!xzFim && xzVez == -1 && xzModoCPU == 1) xzCpuJoga();
}
static void xzModoCb(lv_event_t *) {
  xzModoCPU = !xzModoCPU;
  if (lv_obj_is_valid(xzModo)) lv_label_set_text(lv_obj_get_child(xzModo, 0), xzModoCPU ? "1P vs CPU" : "2 jogadores");
}
void buildXadrez() {
  xzScr = lv_obj_create(NULL);
  psMapReg(xzScr, PSR_CHESS);           // buffers das pecas: libera na saida
  scrStyle(xzScr);
  gameHeader(xzScr);
  lv_obj_clear_flag(xzScr, LV_OBJ_FLAG_SCROLLABLE);
  xzInfo = uiLabel(xzScr, "sua vez (brancas)", 0, 18, F_SM, 0xF4F5F7);
  lv_obj_set_width(xzInfo, 128);
  lv_obj_set_style_text_align(xzInfo, LV_TEXT_ALIGN_CENTER, 0);
  xzModo = uiBtn(xzScr, "1P vs CPU", xzModoCb, 62, 16, 0x1C202B, NULL);
  if (xzModo) lv_obj_set_pos(xzModo, 33, 0);

  // buffers de peca em xzBufPeca (globais)
  for (int y = 0; y < 8; y++) for (int x = 0; x < 8; x++) {
    int idx = y * 8 + x;
    xzCell[y][x] = lv_obj_create(xzScr);
    lv_obj_set_pos(xzCell[y][x], x * 16, 34 + y * 15);
    lv_obj_set_size(xzCell[y][x], 15, 15);
    lv_obj_set_style_radius(xzCell[y][x], 0, 0);
    lv_obj_set_style_shadow_width(xzCell[y][x], 0, 0);
    lv_obj_set_style_pad_all(xzCell[y][x], 0, 0);
    lv_obj_set_style_border_opa(xzCell[y][x], 0, 0);
    lv_obj_add_event_cb(xzCell[y][x], xzClickCb, LV_EVENT_CLICKED, NULL);
    lv_obj_clear_flag(xzCell[y][x], LV_OBJ_FLAG_SCROLLABLE);
    if (!xzBufPeca[idx]) {
      if (psNeed(PC_SZ * PC_SZ * 2)) {
        xzBufPeca[idx] = (uint16_t *)heap_caps_malloc(PC_SZ * PC_SZ * 2, MALLOC_CAP_SPIRAM);
        if (xzBufPeca[idx]) psAllocated[PSR_CHESS] = true;
      }
    }
    if (xzBufPeca[idx]) memset(xzBufPeca[idx], 0, PC_SZ * PC_SZ * 2);
    xzPc[y][x] = lv_canvas_create(xzCell[y][x]);
    lv_obj_set_size(xzPc[y][x], PC_SZ, PC_SZ);
    lv_obj_set_pos(xzPc[y][x], 0, 0);
    lv_canvas_set_buffer(xzPc[y][x], xzBufPeca[idx], PC_SZ, PC_SZ, LV_IMG_CF_TRUE_COLOR);
    lv_obj_clear_flag(xzPc[y][x], LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(xzPc[y][x], LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(xzPc[y][x], LV_OBJ_FLAG_HIDDEN);
  }
  // posicao inicial: brancas embaixo (y=6,7), pretas em cima (y=0,1)
  xzModoCPU = 1;
  if (lv_obj_is_valid(xzModo)) lv_label_set_text(lv_obj_get_child(xzModo, 0), "1P vs CPU");
  xzNovoJogo();
  // tocar no aviso de fim de partida reinicia o jogo
  lv_obj_add_event_cb(xzInfo, [](lv_event_t *) {
    if (!xzFim) return;
    xzNovoJogo();
    toast("novo jogo!");
  }, LV_EVENT_CLICKED, NULL);
  static lv_timer_t *txz = nullptr;
  if (txz) { lv_timer_del(txz); txz = nullptr; }
  txz = lv_timer_create(xzCpuTimer, 700, NULL);
  gGameReg(xzScr);
  nav(xzScr, LV_SCR_LOAD_ANIM_MOVE_LEFT);
}

// ---------------- TV-B-GONE: banco canonico (NA + EU) ----------------
static lv_obj_t *tvgLbl = nullptr;
static volatile bool tvgOn = false;
static int tvgIdx = 0;
static int tvgRegion = 0;         // 0=NA, 1=EU

static uint8_t tvbgReadBits(const uint8_t *codes, uint8_t count, uint8_t *bitPos) {
  // le `count` bits MSB-first (algoritmo canonico do TV-B-Gone)
  uint8_t out = 0;
  for (uint8_t i = 0; i < count; i++) {
    uint8_t bitIndex = (*bitPos)++;
    uint8_t byte = bitIndex >> 3;
    uint8_t bit = (codes[byte] >> (7 - (bitIndex & 7))) & 1;
    out = (out << 1) | bit;
  }
  return out;
}

static void tvbgSendCode(const IrCode *code) {
  if (!code) return;
  uint32_t freq = code->timer_val ? ((uint32_t)code->timer_val * 1000UL) : 38000;
  irCarrierHz(freq);
  bool modulated = (code->timer_val != 0);
  uint8_t bitPos = 0;
  for (uint8_t k = 0; k < code->numpairs; k++) {
    uint8_t idx = tvbgReadBits(code->codes, code->bitcompression, &bitPos);
    uint32_t on  = (uint32_t)code->times[(idx << 1)]      * 10UL;
    uint32_t off = (uint32_t)code->times[(idx << 1) + 1]  * 10UL;
    irMark(on, modulated);       // MARK (carrier ou LED continuo em codigos non-pulsed)
    irDelayUs(off);              // SPACE
  }
  irDelayUs(205000);             // pausa canonica entre POWER codes
}

static const IrCode *tvgCurrent() {
  if (tvgRegion == 0) return tvgIdx < TVBG_NA_N ? NApowerCodes[tvgIdx] : nullptr;
  return tvgIdx < TVBG_EU_N ? EUpowerCodes[tvgIdx] : nullptr;
}
static int tvgCount() { return tvgRegion == 0 ? TVBG_NA_N : TVBG_EU_N; }

static void tvgTick(lv_timer_t *) {
  if (!tvgOn) return;
  const IrCode *code = tvgCurrent();
  if (code) irSendAsyncTvbg(code);
  char b[48];
  snprintf(b, sizeof(b), "TV-B-Gone %s %d/%d", tvgRegion == 0 ? "NA" : "EU", tvgIdx + 1, tvgCount());
  if (lv_obj_is_valid(tvgLbl)) lv_label_set_text(tvgLbl, b);
  tvgIdx++;
  if (tvgIdx >= tvgCount()) {
    if (tvgRegion == 0) { tvgRegion = 1; tvgIdx = 0; return; }
    tvgOn = false;
    if (lv_obj_is_valid(tvgLbl)) lv_label_set_text(tvgLbl, "fim - aperte parada");
    toast("TV-B-Gone: fim da lista (NA+EU)");
  }
}
static void tvgCb(lv_event_t *) {
  if (tvgOn) {
    tvgOn = false;
    if (lv_obj_is_valid(tvgLbl)) lv_label_set_text(tvgLbl, "parado");
    toast("TV-B-Gone parado");
    return;
  }
  tvgOn = true;
  tvgIdx = 0;
  tvgRegion = 0;
  toast("TV-B-Gone: percorrendo codigos...");
}
static void tvgExitCb(lv_event_t *) {
  tvgOn = false;
  buildToolbox();
  nav(scrToolbox, LV_SCR_LOAD_ANIM_MOVE_RIGHT);
}
static lv_obj_t *scrTvBGone = nullptr;
static lv_timer_t *tvgTimer = nullptr;
void buildTvBGone() {
  tvgOn = false;
  scrTvBGone = lv_obj_create(NULL);
  scrStyle(scrTvBGone);
  appHeader(scrTvBGone, "TV-B-GONE");
  lv_obj_clear_flag(scrTvBGone, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(scrTvBGone, [](lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_DELETE) return;
    tvgOn = false;
    if (tvgTimer) { lv_timer_del(tvgTimer); tvgTimer = nullptr; }
  }, LV_EVENT_DELETE, NULL);
  tvgLbl = uiLabel(scrTvBGone, "aperte INICIAR e aponte", 0, 40, F_SM, lv_color_to32(REDMI_TXT));
  lv_obj_set_width(tvgLbl, 128);
  lv_obj_set_style_text_align(tvgLbl, LV_TEXT_ALIGN_CENTER, 0);
  uiLabel(scrTvBGone, ("codigos NA: " + String(TVBG_NA_N) + "  + EU: " + String(TVBG_EU_N)).c_str(), 0, 60, F_SM, lv_color_to32(REDMI_DIM));
  lv_obj_set_width(lv_obj_get_child(scrTvBGone, -1), 128);
  lv_obj_set_style_text_align(lv_obj_get_child(scrTvBGone, -1), LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_t *b1 = uiBtn(scrTvBGone, "INICIAR", tvgCb, 124, 22, 0x5A1414, NULL);
  if (b1) lv_obj_set_pos(b1, 2, 84);
  lv_obj_t *b2 = uiBtn(scrTvBGone, "PARAR", tvgCb, 60, 22, 0x161A22, NULL);
  if (b2) lv_obj_set_pos(b2, 2, 110);
  lv_obj_t *b3 = uiBtn(scrTvBGone, "Sair", tvgExitCb, 58, 22, 0x161A22, NULL);
  if (b3) lv_obj_set_pos(b3, 66, 110);
  uiLabel(scrTvBGone, "1 POWER por codigo", 0, 140, F_SM, lv_color_to32(REDMI_DIM));
  lv_obj_set_width(lv_obj_get_child(scrTvBGone, -1), 128);
  lv_obj_set_style_text_align(lv_obj_get_child(scrTvBGone, -1), LV_TEXT_ALIGN_CENTER, 0);
  if (tvgTimer) { lv_timer_del(tvgTimer); tvgTimer = nullptr; }
  tvgTimer = lv_timer_create(tvgTick, 1100, NULL);
  nav(scrTvBGone, LV_SCR_LOAD_ANIM_MOVE_LEFT);
}
// ============================================================
//  FLAPPY BIRD - toque = asa; canos aleatorios; placar
// ============================================================
static lv_obj_t *fbScr, *fbBird, *fbPipes[3][2], *fbPonto, *fbRec;
static int fbY, fbV, fbPx[3], fbAtivo, fbPontos, fbRecorde, fbTick, fbFim;
static int fbGapY[3];                                             // vao aleatorio por cano
static void fbEnd(const char *msg) {
  if (fbFim) return;
  fbFim = 1;
  bool novo = fbPontos > fbRecorde;
  if (novo) {
    fbRecorde = fbPontos;
    if (lv_obj_is_valid(fbRec)) lv_label_set_text_fmt(fbRec, "rec %d", fbRecorde);
    toast("NOVO RECORDE!");
  } else {
    toast(msg);
  }
}
static void fbReset() {
  fbY = 40; fbV = 0; fbPontos = 0; fbFim = 0; fbTick = 0;
  fbPx[0] = 70; fbPx[1] = 130; fbPx[2] = 190;                     // 3 canos
  for (int i = 0; i < 3; i++) fbGapY[i] = 30 + (esp_random() % 45); // vao aleatorio 30..74
  if (lv_obj_is_valid(fbPonto)) lv_label_set_text(fbPonto, "0");
}
static void fbUpd(lv_timer_t *) {
  if (!gInGame) return;
  if (!lv_obj_is_valid(fbScr) || lv_scr_act() != fbScr) return;
  if (fbFim) return;
  fbTick++;
  if (fbTick % 6 == 0) fbV += 1;                                  // gravidade
  fbY += fbV;
  if (fbY < 0) fbY = 0;
  if (fbY > 138) {                                                // caiu no chao
    fbEnd("caiu!");
  }
  for (int i = 0; i < 3; i++) fbPx[i] -= 2;
  for (int i = 0; i < 3; i++) {
    if (fbPx[i] < -20) {                                          // recicla o cano
      fbPx[i] += 190;
      fbGapY[i] = 30 + (esp_random() % 45);                      // novo vao aleatorio
      fbPontos++;
      if (lv_obj_is_valid(fbPonto)) lv_label_set_text_fmt(fbPonto, "%d", fbPontos);
    }
  }
  // colisao (passaro 8x6 em x=20)
  for (int i = 0; i < 3; i++) {
    int bx = fbPx[i];
    if (20 + 8 > bx && 20 < bx + 12) {
      // colide se estiver FORA do vao (cano de cima ou de baixo)
      if (fbY < fbGapY[i] || fbY + 6 > fbGapY[i] + 46) fbEnd("bateu!");
    }
  }
  if (lv_obj_is_valid(fbBird)) lv_obj_set_pos(fbBird, 20, fbY);
  for (int i = 0; i < 3; i++) {
    if (lv_obj_is_valid(fbPipes[i][0])) {
      lv_obj_set_pos(fbPipes[i][0], fbPx[i], 0);
      lv_obj_set_size(fbPipes[i][0], 12, fbGapY[i]);              // cano de cima
    }
    if (lv_obj_is_valid(fbPipes[i][1])) {
      lv_obj_set_pos(fbPipes[i][1], fbPx[i], fbGapY[i] + 46);
      lv_obj_set_size(fbPipes[i][1], 12, 140 - (fbGapY[i] + 46)); // cano de baixo
    }
  }
}
static void fbTapCb(lv_event_t *) {
  if (fbFim) { fbReset(); return; }
  fbV = -4;                                                       // impulso da asa
}
void buildFlappy() {
  fbScr = lv_obj_create(NULL);
  scrStyle(fbScr);
  gameHeader(fbScr);
  lv_obj_clear_flag(fbScr, LV_OBJ_FLAG_SCROLLABLE);
  fbPonto = uiLabel(fbScr, "0", 0, 20, F_SM, 0xF4F5F7);
  lv_obj_set_width(fbPonto, 128);
  lv_obj_set_style_text_align(fbPonto, LV_TEXT_ALIGN_CENTER, 0);
  fbRec = uiLabel(fbScr, "rec 0", 92, 20, F_SM, 0x676C7A);
  for (int i = 0; i < 3; i++)
    for (int k = 0; k < 2; k++) {
      fbPipes[i][k] = lv_obj_create(fbScr);
      lv_obj_set_size(fbPipes[i][k], 12, 40);
      lv_obj_set_style_bg_color(fbPipes[i][k], lv_color_hex(0x3AD07F), 0);
      lv_obj_set_style_radius(fbPipes[i][k], 3, 0);
      lv_obj_set_style_shadow_width(fbPipes[i][k], 0, 0);
      lv_obj_clear_flag(fbPipes[i][k], LV_OBJ_FLAG_SCROLLABLE);
    }
  fbBird = lv_obj_create(fbScr);
  lv_obj_set_size(fbBird, 8, 6);
  lv_obj_set_style_bg_color(fbBird, lv_color_hex(0xFFD54F), 0);
  lv_obj_set_style_radius(fbBird, 3, 0);
  lv_obj_set_style_shadow_width(fbBird, 0, 0);
  lv_obj_clear_flag(fbBird, LV_OBJ_FLAG_SCROLLABLE);
  uiLabel(fbScr, "toque = asa", 0, 146, F_SM, 0x676C7A);
  lv_obj_set_width(lv_obj_get_child(fbScr, -1), 128);
  lv_obj_set_style_text_align(lv_obj_get_child(fbScr, -1), LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_add_event_cb(fbScr, fbTapCb, LV_EVENT_PRESSED, NULL);
  fbReset();
  fbRecorde = 0;
  static lv_timer_t *tfb = nullptr;
  if (tfb) { lv_timer_del(tfb); tfb = nullptr; }
  tfb = lv_timer_create(fbUpd, 45, NULL);
  gGameReg(fbScr);
  nav(fbScr, LV_SCR_LOAD_ANIM_MOVE_LEFT);
}

// ============================================================
//  DINO ( Chrome ) - toque = pular; cactos e aves; velocidade sobe
// ============================================================
static lv_obj_t *dnScr, *dnDino, *dnObs[3], *dnPonto, *dnRec;
static int dnY, dnV, dnOx[3], dnTick, dnFim, dnPontos, dnRecorde, dnVel;
static void dnReset() {
  dnY = 118; dnV = 0; dnPontos = 0; dnFim = 0; dnTick = 0; dnVel = 3;
  dnOx[0] = 130; dnOx[1] = 180; dnOx[2] = 230;
  if (lv_obj_is_valid(dnPonto)) lv_label_set_text(dnPonto, "0");
}
static void dnUpd(lv_timer_t *) {
  if (!gInGame) return;
  if (!lv_obj_is_valid(dnScr) || lv_scr_act() != dnScr) return;
  if (dnFim) return;
  dnTick++;
  if (dnV != 0 || dnY < 118) { dnV += 1; dnY += dnV; }            // pulo
  if (dnY >= 118) { dnY = 118; dnV = 0; }
  for (int i = 0; i < 3; i++) {
    dnOx[i] -= dnVel;
    if (dnOx[i] < -10) {
      dnOx[i] += 130;
      dnPontos++;
      if (dnPontos % 5 == 0 && dnVel < 7) dnVel++;                 // acelera
      if (lv_obj_is_valid(dnPonto)) lv_label_set_text_fmt(dnPonto, "%d", dnPontos);
    }
    if (lv_obj_is_valid(dnObs[i])) {
      int h = (i == 1) ? 12 : 8;
      lv_obj_set_pos(dnObs[i], dnOx[i], 128 - h);
    }
  }
  if (lv_obj_is_valid(dnDino)) lv_obj_set_pos(dnDino, 12, dnY);
  // colisao com obstaculos (dino 10x10 em x=12)
  for (int i = 0; i < 3; i++) {
    int h = (i == 1) ? 12 : 8;
    int obsY = 128 - h;
    // dino bate se estiver baixo o suficiente para colidir com o obstaculo
    if (12 + 10 > dnOx[i] && 12 < dnOx[i] + 8 && dnY + 10 > obsY) {
      if (!dnFim) {
        dnFim = 1;
        if (dnPontos > dnRecorde) {
          dnRecorde = dnPontos;
          if (lv_obj_is_valid(dnRec)) lv_label_set_text_fmt(dnRec, "rec %d", dnRecorde);
          toast(dnPontos > 0 ? "NOVO RECORDE!" : "bateu!");
        } else {
          toast("bateu!");
        }
      }
    }
  }
}
static void dnTapCb(lv_event_t *) {
  if (dnFim) { dnReset(); return; }
  if (dnY >= 118) dnV = -6;                                       // so no chao
}
void buildDino() {
  dnScr = lv_obj_create(NULL);
  scrStyle(dnScr);
  gameHeader(dnScr);
  lv_obj_clear_flag(dnScr, LV_OBJ_FLAG_SCROLLABLE);
  dnPonto = uiLabel(dnScr, "0", 0, 20, F_SM, 0xF4F5F7);
  lv_obj_set_width(dnPonto, 128);
  lv_obj_set_style_text_align(dnPonto, LV_TEXT_ALIGN_CENTER, 0);
  dnRec = uiLabel(dnScr, "rec 0", 92, 20, F_SM, 0x676C7A);
  // chao
  lv_obj_t *chao = lv_obj_create(dnScr);
  lv_obj_set_pos(chao, 0, 128);
  lv_obj_set_size(chao, 128, 1);
  lv_obj_set_style_bg_color(chao, REDMI_DIM, 0);
  lv_obj_clear_flag(chao, LV_OBJ_FLAG_SCROLLABLE);
  for (int i = 0; i < 3; i++) {
    dnObs[i] = lv_obj_create(dnScr);
    lv_obj_set_size(dnObs[i], 8, i == 1 ? 12 : 8);
    lv_obj_set_style_bg_color(dnObs[i], i == 1 ? lv_color_hex(0xFFA726) : lv_color_hex(0x3AD07F), 0);
    lv_obj_set_style_radius(dnObs[i], 2, 0);
    lv_obj_set_style_shadow_width(dnObs[i], 0, 0);
    lv_obj_clear_flag(dnObs[i], LV_OBJ_FLAG_SCROLLABLE);
  }
  dnDino = lv_obj_create(dnScr);
  lv_obj_set_size(dnDino, 10, 10);
  lv_obj_set_style_bg_color(dnDino, REDMI_ACC, 0);
  lv_obj_set_style_radius(dnDino, 2, 0);
  lv_obj_set_style_shadow_width(dnDino, 0, 0);
  lv_obj_clear_flag(dnDino, LV_OBJ_FLAG_SCROLLABLE);
  uiLabel(dnScr, "toque = pular", 0, 140, F_SM, 0x676C7A);
  lv_obj_set_width(lv_obj_get_child(dnScr, -1), 128);
  lv_obj_set_style_text_align(lv_obj_get_child(dnScr, -1), LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_add_event_cb(dnScr, dnTapCb, LV_EVENT_PRESSED, NULL);
  dnReset();
  dnRecorde = 0;
  static lv_timer_t *tdn = nullptr;
  if (tdn) { lv_timer_del(tdn); tdn = nullptr; }
  tdn = lv_timer_create(dnUpd, 45, NULL);
  gGameReg(dnScr);
  nav(dnScr, LV_SCR_LOAD_ANIM_MOVE_LEFT);
}
// ============================================================
//  TETRIS - 10x20 em 6px de celula (60x120) + rotação por toque
// ============================================================
static lv_obj_t *ttScr, *ttGrade[10][20], *ttPont, *ttNext, *ttLinhas;
static uint8_t ttM[10][20];                // 0 vazio, 1..7 cor da peça

static const int8_t ttPecas[7][4][2] = {   // [peca][bloco][x,y]
  {{0,0},{1,0},{2,0},{3,0}},   // I
  {{0,0},{1,0},{0,1},{1,1}},   // O
  {{0,0},{1,0},{2,0},{1,1}},   // T
  {{0,0},{1,0},{2,0},{0,1}},   // L
  {{0,0},{1,0},{2,0},{2,1}},   // J
  {{1,0},{2,0},{0,1},{1,1}},   // S
  {{0,0},{1,0},{1,1},{2,1}}    // Z
};
static const uint32_t ttCor[8] = {
  0x000000, 0x29B6F6, 0xFFC400, 0xAB47BC, 0xFF7043, 0x1565C0, 0x43A047, 0xE53935 };


static int ttX, ttY, ttRot, ttPeca, ttProx, ttFim, ttPontos, ttLinhasN, ttTick, ttVel;
static lv_obj_t *ttNx[4][3];                       // grade 4x3 da PROXIMA peca
static void ttDrawNext() {                          // desenha ttProx na prévia
  for (int cx = 0; cx < 4; cx++) for (int cy = 0; cy < 3; cy++)
    if (lv_obj_is_valid(ttNx[cx][cy])) lv_obj_set_style_bg_color(ttNx[cx][cy], lv_color_hex(0x141821), 0);
  for (int i = 0; i < 4; i++) {
    int bx = ttPecas[ttProx][i][0], by = ttPecas[ttProx][i][1];
    if (bx < 4 && by < 3 && lv_obj_is_valid(ttNx[bx][by]))
      lv_obj_set_style_bg_color(ttNx[bx][by], lv_color_hex(ttCor[ttProx + 1]), 0);
  }
}

// caixa da peca (I usa 4, O usa 2, demais 3): rotacao CW = (dx,dy)->(B-1-dy, dx)
static const int8_t ttBox[7] = { 4, 2, 3, 3, 3, 3, 3 };
static void ttBloco(int i, int rot, int *ox, int *oy) {
  int dx = ttPecas[ttPeca][i][0], dy = ttPecas[ttPeca][i][1];
  for (int r = 0; r < (rot & 3); r++) {
    int nx = ttBox[ttPeca] - 1 - dy;
    dy = dx;
    dx = nx;
  }
  *ox = dx; *oy = dy;
}
static bool ttCabe(int px, int py, int rot) {
  for (int i = 0; i < 4; i++) {
    int dx, dy;
    ttBloco(i, rot, &dx, &dy);
    int x = px + dx, y = py + dy;
    if (x < 0 || x >= 10 || y >= 20) return false;
    if (y >= 0 && ttM[x][y]) return false;
  }
  return true;
}
static void ttTrava() {
  for (int i = 0; i < 4; i++) {
    int dx, dy;
    ttBloco(i, ttRot, &dx, &dy);
    int x = ttX + dx, y = ttY + dy;
    if (y >= 0 && y < 20) ttM[x][y] = ttPeca + 1;
  }
  // linhas completas
  int linhas = 0;
  for (int y = 19; y >= 0; y--) {
    bool cheia = true;
    for (int x = 0; x < 10; x++) if (!ttM[x][y]) cheia = false;
    if (cheia) {
      linhas++;
      for (int yy = y; yy > 0; yy--) for (int x = 0; x < 10; x++) ttM[x][yy] = ttM[x][yy - 1];
      for (int x = 0; x < 10; x++) ttM[x][0] = 0;
      y++;
    }
  }
  if (linhas) {
    ttLinhasN += linhas;
    ttPontos += 100 * linhas * (linhas + 1) / 2;
    if (ttLinhasN % 5 == 0 && ttVel > 2) ttVel--;
  }
  ttPeca = ttProx;
  ttProx = esp_random() % 7;
  ttX = 4; ttY = 0; ttRot = 0;
  ttDrawNext();
  if (!ttCabe(ttX, ttY, ttRot)) {
    ttFim = 1;
    toast(("GAME OVER - " + String(ttPontos) + " pts").c_str());
  }
}
static void ttDesenha() {
  for (int x = 0; x < 10; x++) for (int y = 0; y < 20; y++) {
    if (!lv_obj_is_valid(ttGrade[x][y])) continue;
    int v = ttM[x][y];
    // apaga a peça atual antes de desenhar
    lv_obj_set_style_bg_color(ttGrade[x][y],
      v ? lv_color_hex(ttCor[v & 7]) : lv_color_hex(0x141821), 0);
  }
  // peça corrente
  for (int i = 0; i < 4; i++) {
    int dx, dy;
    ttBloco(i, ttRot, &dx, &dy);
    int x = ttX + dx, y = ttY + dy;
    if (x < 0 || x >= 10 || y < 0 || y >= 20) continue;
    if (lv_obj_is_valid(ttGrade[x][y]))
      lv_obj_set_style_bg_color(ttGrade[x][y], lv_color_hex(ttCor[ttPeca + 1]), 0);
  }
  if (lv_obj_is_valid(ttPont))   lv_label_set_text_fmt(ttPont, "%d", ttPontos);
  if (lv_obj_is_valid(ttLinhas)) lv_label_set_text_fmt(ttLinhas, "L%d", ttLinhasN);
}
static void ttUpd(lv_timer_t *) {
  if (!gInGame) return;
  if (!lv_obj_is_valid(ttScr) || lv_scr_act() != ttScr) return;
  if (ttFim) return;
  ttTick++;
  // peca desce sozinha na velocidade
  if (ttTick >= ttVel * 4) {
    ttTick = 0;
    if (ttCabe(ttX, ttY + 1, ttRot)) ttY++;
    else ttTrava();
  }
  // movimento lateral pelo dedo: coluna alvo = celula sob o dedo (grade em x=9..69)
  if (gTouchDown && gTx >= 6 && gTx < 72 && gTy >= 44) {
    int x = (gTx - 9) / 6;
    if (x < 0) x = 0; if (x > 9) x = 9;
    if (x > ttX && ttCabe(ttX + 1, ttY, ttRot)) ttX++;
    else if (x < ttX && ttCabe(ttX - 1, ttY, ttRot)) ttX--;
  }
  ttDesenha();
}
static void ttRotaciona() {                 // gira com wall-kick (borda nao bloqueia)
  int nr = (ttRot + 1) & 3;
  static const int8_t kx[5] = { 0, -1, 1, -2, 2 };
  static const int8_t ky[5] = { 0, 0, 0, -1, 1 };
  for (int k = 0; k < 5; k++) {
    if (ttCabe(ttX + kx[k], ttY + ky[k], nr)) {
      ttX += kx[k]; ttY += ky[k]; ttRot = nr;
      return;
    }
  }
}
static void ttTapCb(lv_event_t *) {
  if (ttFim) {                                    // reinicia
    for (int x = 0; x < 10; x++) for (int y = 0; y < 20; y++) ttM[x][y] = 0;
    ttPontos = 0; ttLinhasN = 0; ttVel = 6; ttFim = 0; ttPeca = ttProx; ttProx = esp_random() % 7;
    ttDrawNext();
    ttX = 4; ttY = 0; ttRot = 0; ttTick = 0;
    ttDesenha();
    return;
  }
  if (gTy < 40) {                                 // ACIMA da grade = rodar (com kick)
    ttRotaciona();
  } else {                                        // na grade = descer rapido
    if (ttCabe(ttX, ttY + 1, ttRot)) ttY++;
    else ttTrava();
  }
  ttDesenha();
}
static void ttSwipeCb(lv_event_t *e) {             // queda instantanea (toque longo)
  (void)e;
  if (ttFim) return;
  while (ttCabe(ttX, ttY + 1, ttRot)) ttY++;
  ttTrava();
  ttDesenha();
}
void buildTetris() {
  ttScr = lv_obj_create(NULL);
  scrStyle(ttScr);
  gameHeader(ttScr);
  lv_obj_clear_flag(ttScr, LV_OBJ_FLAG_SCROLLABLE);
  ttPont = uiLabel(ttScr, "0", 0, 20, F_SM, 0xF4F5F7);
  ttLinhas = uiLabel(ttScr, "L0", 40, 20, F_SM, 0x676C7A);
  // grade 10x20 de 6px: x=9..69, y=40 (ultima linha termina exatamente em 160)
  for (int x = 0; x < 10; x++) for (int y = 0; y < 20; y++) {
    ttGrade[x][y] = lv_obj_create(ttScr);
    lv_obj_set_pos(ttGrade[x][y], 9 + x * 6, 40 + y * 6);
    lv_obj_set_size(ttGrade[x][y], 6, 6);
    lv_obj_set_style_bg_color(ttGrade[x][y], lv_color_hex(0x141821), 0);
    lv_obj_set_style_radius(ttGrade[x][y], 0, 0);
    lv_obj_set_style_shadow_width(ttGrade[x][y], 0, 0);
    lv_obj_set_style_border_opa(ttGrade[x][y], 0, 0);
    lv_obj_set_style_pad_all(ttGrade[x][y], 0, 0);
    lv_obj_clear_flag(ttGrade[x][y], LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(ttGrade[x][y], LV_OBJ_FLAG_CLICKABLE);
  }
  // controles na coluna livre da direita (era texto fora da tela em y=172)
  ttNext = uiLabel(ttScr, "tap acima: gira\ntap: desce\nsegure: queda", 74, 40, F_SM, 0x676C7A);
  lv_obj_set_width(ttNext, 54);
  // previa da proxima peca (canto livre)
  uiLabel(ttScr, "PROX", 74, 96, F_SM, 0x676C7A);
  for (int cx = 0; cx < 4; cx++) for (int cy = 0; cy < 3; cy++) {
    ttNx[cx][cy] = lv_obj_create(ttScr);
    lv_obj_set_pos(ttNx[cx][cy], 74 + cx * 8, 108 + cy * 8);
    lv_obj_set_size(ttNx[cx][cy], 8, 8);
    lv_obj_set_style_bg_color(ttNx[cx][cy], lv_color_hex(0x141821), 0);
    lv_obj_set_style_radius(ttNx[cx][cy], 1, 0);
    lv_obj_set_style_shadow_width(ttNx[cx][cy], 0, 0);
    lv_obj_set_style_border_opa(ttNx[cx][cy], 0, 0);
    lv_obj_set_style_pad_all(ttNx[cx][cy], 0, 0);
    lv_obj_clear_flag(ttNx[cx][cy], LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(ttNx[cx][cy], LV_OBJ_FLAG_CLICKABLE);
  }
  ttDrawNext();
  for (int x = 0; x < 10; x++) for (int y = 0; y < 20; y++) ttM[x][y] = 0;
  ttPeca = esp_random() % 7; ttProx = esp_random() % 7;
  ttX = 4; ttY = 0; ttRot = 0; ttPontos = 0; ttLinhasN = 0; ttVel = 6; ttFim = 0; ttTick = 0;
  lv_obj_add_event_cb(ttScr, ttTapCb, LV_EVENT_PRESSED, NULL);
  lv_obj_add_event_cb(ttScr, ttSwipeCb, LV_EVENT_LONG_PRESSED, NULL);
  static lv_timer_t *ttt = nullptr;
  if (ttt) { lv_timer_del(ttt); ttt = nullptr; }
  ttt = lv_timer_create(ttUpd, 45, NULL);
  gGameReg(ttScr);
  nav(ttScr, LV_SCR_LOAD_ANIM_MOVE_LEFT);
}
// ============================================================
//  CORRIDAS (Mario / Sonic / Doom) - motor compartilhado
//  toque = pular/atirar · arrasto move · moedas/aneis · 3 vidas
// ============================================================
static lv_obj_t *rnScr, *rnHeroi, *rnObs[3], *rnItem[3], *rnPlacar, *rnVidasL, *rnChao;
static int rnModo, rnY, rnV, rnOx[3], rnIx[3], rnPontos, rnVidasN, rnTick, rnVel, rnFim, rnCooldown;
static int rnSpin;                                       // Sonic: tick de spin (invencivel)
static const char *rnNome;
static const uint32_t rnCorHeroi[3] = { 0xE53935, 0x1565C0, 0x8BC34A };   // mario, sonic, doom

static void rnReset() {
  rnY = 110; rnV = 0; rnPontos = 0; rnVidasN = 3; rnTick = 0;
  rnVel = (rnModo == 1) ? 5 : 3;                                        // Sonic corre mais que o Mario
  rnSpin = 0;
  rnFim = 0; rnCooldown = 0;
  rnOx[0] = 140; rnOx[1] = 200; rnOx[2] = 260;
  rnIx[0] = 150; rnIx[1] = 210; rnIx[2] = 270;
  if (lv_obj_is_valid(rnPlacar)) lv_label_set_text(rnPlacar, "0");
  if (lv_obj_is_valid(rnVidasL)) lv_label_set_text_fmt(rnVidasL, "%d", rnVidasN);
}
static void rnFimJogo(const char *msg) {
  rnFim = 1;
  if (rnPontos >= 1000) {
    char b[64];
    snprintf(b, sizeof(b), "%s  RECORDE!", msg);
    toast(b);
  } else {
    toast(msg);
  }
}
static void rnUpd(lv_timer_t *) {
  if (!gInGame) return;
  if (!lv_obj_is_valid(rnScr) || lv_scr_act() != rnScr) return;
  if (rnFim) return;
  rnTick++;
  if (rnCooldown) rnCooldown--;
  if (rnSpin) rnSpin--;
  // fisica do pulo
  if (rnV != 0 || rnY < 110) { rnV += 1; rnY += rnV; }
  if (rnY >= 110) { rnY = 110; rnV = 0; }
  int hx = (rnModo == 2) ? 10 : 8;
  // heroi segue o dedo na horizontal
  if (gTouchDown) {
    int tx = gTx - hx / 2;
    if (tx < 0) tx = 0; if (tx > 120) tx = 120;
    if (lv_obj_is_valid(rnHeroi)) lv_obj_set_x(rnHeroi, tx);
  }
  int hxg = lv_obj_is_valid(rnHeroi) ? lv_obj_get_x(rnHeroi) : 10;
  if (lv_obj_is_valid(rnHeroi)) lv_obj_set_pos(rnHeroi, hxg, rnY);
  // obstaculos
  for (int i = 0; i < 3; i++) {
    rnOx[i] -= rnVel;
    if (rnOx[i] < -12) {
      rnOx[i] += 170;
      rnPontos += 10;
      if (lv_obj_is_valid(rnPlacar)) lv_label_set_text_fmt(rnPlacar, "%d", rnPontos);
      if ((rnPontos / 100) % 3 == 0 && rnVel < 8) rnVel++;
    }
    if (lv_obj_is_valid(rnObs[i])) lv_obj_set_pos(rnObs[i], rnOx[i], 118 - (i == 1 ? 14 : 8) + 4);
    // colisao
    if (hxg + hx > rnOx[i] && hxg < rnOx[i] + 10 && rnY + hx > 110) {
      rnOx[i] += 170;
      if (rnSpin > 0) { rnPontos += 25; if (lv_obj_is_valid(rnPlacar)) lv_label_set_text_fmt(rnPlacar, "%d", rnPontos); }   // Sonic girando atravessa
      else {
        rnVidasN--;
        if (lv_obj_is_valid(rnVidasL)) lv_label_set_text_fmt(rnVidasL, "%d", rnVidasN);
        if (rnVidasN <= 0) { rnFimJogo("GAME OVER"); return; }
        toast("ai!");
      }
    }
    // itens (moedas / aneis)
    rnIx[i] -= rnVel;
    if (rnIx[i] < -8) rnIx[i] += 170;
    if (lv_obj_is_valid(rnItem[i])) lv_obj_set_pos(rnItem[i], rnIx[i], 90);
    if (hxg + hx > rnIx[i] && hxg < rnIx[i] + 6 && abs(rnY - 90) < 14) {
      rnIx[i] += 170;
      rnPontos += 50;
      if (lv_obj_is_valid(rnPlacar)) lv_label_set_text_fmt(rnPlacar, "%d", rnPontos);
    }
  }
}
static void rnTapCb(lv_event_t *) {
  if (rnFim) { rnReset(); return; }
  if (rnY >= 110) rnV = -7;                        // pulo
  if (rnModo == 2 && rnCooldown == 0) {            // doom: atira
    rnCooldown = 12;
    rnPontos += 25;
    if (lv_obj_is_valid(rnPlacar)) lv_label_set_text_fmt(rnPlacar, "%d", rnPontos);
    toast((esp_random() & 1) ? "pow!" : "bam!");
  }
}
static void rnSpinCb(lv_event_t *) {   // Sonic: segurar = spin (invencivel ~1s)
  if (rnFim) { rnReset(); return; }
  if (rnModo == 1) { rnSpin = 25; beepShort(1800, 60); toast("SPIN!"); }
}
void buildRunner(int modo) {                       // 0=Mario 1=Sonic 2=Doom
  rnModo = modo;
  rnNome = modo == 0 ? "MARIO" : modo == 1 ? "SONIC" : "DOOM";
  rnScr = lv_obj_create(NULL);
  scrStyle(rnScr);
  gameHeader(rnScr);
  lv_obj_clear_flag(rnScr, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_t *nm = uiLabel(rnScr, rnNome, 24, 1, F_SM, lv_color_to32(REDMI_ACC));
  (void)nm;
  rnPlacar = uiLabel(rnScr, "0", 70, 1, F_SM, 0xF4F5F7);
  lv_obj_set_width(rnPlacar, 56);
  lv_obj_set_style_text_align(rnPlacar, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_t *vl = uiLabel(rnScr, "VIDAS", 4, 20, F_SM, 0x676C7A);
  (void)vl;
  rnVidasL = uiLabel(rnScr, "3", 46, 20, F_SM, 0xFF5C5C);
  uiLabel(rnScr, "toque = pular", 0, 60, F_SM, 0x676C7A);
  lv_obj_set_width(lv_obj_get_child(rnScr, -1), 128);
  lv_obj_set_style_text_align(lv_obj_get_child(rnScr, -1), LV_TEXT_ALIGN_CENTER, 0);
  if (modo == 1) {
    uiLabel(rnScr, "segure = spin", 0, 74, F_SM, 0x676C7A);
    lv_obj_set_width(lv_obj_get_child(rnScr, -1), 128);
    lv_obj_set_style_text_align(lv_obj_get_child(rnScr, -1), LV_TEXT_ALIGN_CENTER, 0);
  }
  if (modo == 2) {
    uiLabel(rnScr, "arraste = andar", 0, 74, F_SM, 0x676C7A);
    lv_obj_set_width(lv_obj_get_child(rnScr, -1), 128);
    lv_obj_set_style_text_align(lv_obj_get_child(rnScr, -1), LV_TEXT_ALIGN_CENTER, 0);
  }
  rnChao = lv_obj_create(rnScr);
  lv_obj_set_pos(rnChao, 0, 120);
  lv_obj_set_size(rnChao, 128, 1);
  lv_obj_set_style_bg_color(rnChao, REDMI_DIM, 0);
  lv_obj_clear_flag(rnChao, LV_OBJ_FLAG_SCROLLABLE);
  for (int i = 0; i < 3; i++) {
    rnObs[i] = lv_obj_create(rnScr);
    lv_obj_set_size(rnObs[i], 10, 8);
    lv_obj_set_style_bg_color(rnObs[i], lv_color_hex(0x795548), 0);
    lv_obj_set_style_radius(rnObs[i], 2, 0);
    lv_obj_set_style_shadow_width(rnObs[i], 0, 0);
    lv_obj_clear_flag(rnObs[i], LV_OBJ_FLAG_SCROLLABLE);
    rnItem[i] = lv_obj_create(rnScr);
    lv_obj_set_size(rnItem[i], 6, 6);
    lv_obj_set_style_bg_color(rnItem[i], lv_color_hex(modo == 1 ? 0xFFD54F : 0xFFC400), 0);
    lv_obj_set_style_radius(rnItem[i], LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_shadow_width(rnItem[i], 0, 0);
    lv_obj_clear_flag(rnItem[i], LV_OBJ_FLAG_SCROLLABLE);
  }
  rnHeroi = lv_obj_create(rnScr);
  lv_obj_set_size(rnHeroi, modo == 2 ? 10 : 8, 8);
  lv_obj_set_style_bg_color(rnHeroi, lv_color_hex(rnCorHeroi[modo]), 0);
  lv_obj_set_style_radius(rnHeroi, 3, 0);
  lv_obj_set_style_shadow_width(rnHeroi, 0, 0);
  lv_obj_clear_flag(rnHeroi, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(rnScr, rnTapCb, LV_EVENT_PRESSED, NULL);
  if (modo == 1) lv_obj_add_event_cb(rnScr, rnSpinCb, LV_EVENT_LONG_PRESSED, NULL);   // Sonic: spin
  rnReset();
  static lv_timer_t *trn = nullptr;
  if (trn) { lv_timer_del(trn); trn = nullptr; }
  trn = lv_timer_create(rnUpd, 40, NULL);
  gGameReg(rnScr);
  nav(rnScr, LV_SCR_LOAD_ANIM_MOVE_LEFT);
}
// ============================================================
//  ARQUIVOS - explorador de arquivos (estilo Windows Explorer)
//  barra de ferramentas + barra de endereco + lista com colunas
//  + barra de status. Pastas navegaveis (voltar/sobe).
// ============================================================
static lv_obj_t *arOut, *arList, *arDelBox, *arAddr, *arStat;
static String arPath = "/";            // pasta atual
static String arNames[40];
static uint32_t arSizes[40];
static bool   arIsDir[40];
static int    arCount = 0;
static int    arDelTarget = -1;
static void arSendCb(lv_event_t *) { uiServerStart(); }
static void arFillCb(lv_event_t *);   // forward (definida abaixo)

// ---------- icone de arquivo 12x12 (pasta / imagem / audio / doc) ----------
#define FI_SZ 12
static void drawFileIco(lv_obj_t *cv, int kind) {
  static uint32_t buf[FI_SZ * FI_SZ];
  for (int i = 0; i < FI_SZ * FI_SZ; i++) buf[i] = 0x1B2028;
  auto px = [&](int x,int y,uint32_t c){ if(x>=0&&y>=0&&x<FI_SZ&&y<FI_SZ) buf[y*FI_SZ+x]=c; };
  auto rect = [&](int x0,int y0,int w,int h,uint32_t c){ for(int j=0;j<h;j++)for(int i=0;i<w;i++) px(x0+i,y0+j,c); };
  if (kind == 0) {                                  // PASTA
    rect(1,3,4,2,0xF2C230); rect(1,5,10,7,0xE0AE12); rect(2,6,8,5,0xF7D354);
  } else if (kind == 1) {                           // IMAGEM
    rect(1,2,10,9,0x3AD07F); rect(2,3,8,7,0x0F2A1C);
    rect(3,7,2,2,0x3AD07F); rect(6,5,2,2,0x3AD07F); rect(8,7,1,1,0x3AD07F);
    rect(2,9,8,1,0x3AD07F);
  } else if (kind == 2) {                           // AUDIO
    rect(6,1,2,7,0xB388FF); rect(4,7,2,2,0xB388FF); rect(8,7,2,2,0xB388FF);
    rect(3,9,6,2,0xB388FF);
  } else {                                          // DOCUMENTO
    rect(2,1,8,10,0x8B90A0); rect(3,2,6,8,0x1B2028);
    rect(4,4,4,1,0x8B90A0); rect(4,6,4,1,0x8B90A0); rect(4,8,3,1,0x8B90A0);
  }
  for (int y = 0; y < FI_SZ; y++)
    for (int x = 0; x < FI_SZ; x++)
      lv_canvas_set_px(cv, x, y, lv_color_hex(buf[y*FI_SZ+x]));
}
static int arFileKind(const String &n, bool dir) {
  if (dir) return 0;
  String lo = n; lo.toLowerCase();
  if (lo.endsWith(".jpg") || lo.endsWith(".jpeg") || lo.endsWith(".bmp") || lo.endsWith(".png")) return 1;
  if (lo.endsWith(".wav") || lo.endsWith(".mp3") || lo.endsWith(".ogg")) return 2;
  return 3;
}
// lista o conteudo de uma pasta do SD (pastas primeiro, depois arquivos)
static int sdListDir(const String &path, int max) {
  arCount = 0;
  if (!sdBeginSession()) return -1;
  File root = SD.open(path);
  if (root) {
    for (int pass = 0; pass < 2; pass++) {
      root.rewindDirectory();
      while (arCount < max) {
        File f = root.openNextFile();
        if (!f) break;
        String nm = String(f.name());
        bool dir = f.isDirectory();
        uint32_t sz = dir ? 0 : f.size();
        f.close();
        if (nm == "." || nm == "..") continue;
        if ((pass == 0) != dir) continue;          // 0: pastas | 1: arquivos
        arNames[arCount] = nm; arSizes[arCount] = sz; arIsDir[arCount] = dir;
        arCount++;
      }
    }
    root.close();
  }
  sdEndSession();
  return arCount;
}
static void arFormatSize(uint32_t b, char *o, size_t n) {
  if (b >= 1048576UL) snprintf(o, n, "%.1fM", b / 1048576.0);
  else if (b >= 1024UL) snprintf(o, n, "%luK", (unsigned long)(b / 1024));
  else snprintf(o, n, "%luB", (unsigned long)b);
}
// info do cartao (FAT/exFAT/espaco) + conversao p/ FAT16
static void arInfoCb(lv_event_t *) {
  if (gLvPause || gSdPause) return;
  static uint32_t lastProbe = 0;
  if (millis() - lastProbe > 30000) { lastProbe = millis(); sdProbe(); }   // cache 30s
  char b[64];
  snprintf(b, sizeof(b), "%s  %llu/%llu MB", sdInfoTxt().c_str(),
           (unsigned long long)gSd.usedMB, (unsigned long long)gSd.totalMB);
  if (lv_obj_is_valid(arStat)) lv_label_set_text(arStat, b);
  if (!gSd.ok) toast("gravador: SD ausente");
  else {
    char w[32];
    snprintf(w, sizeof(w), "gravador: %s", gSd.exfat ? "exFAT" : (gSd.fat ? "FAT" : "?"));
    toast(w);
  }
}
static void arFatCb(lv_event_t *) {
  // o ESP32-SD nao converte FAT: apenas explica o caminho (nao trava a UI)
  String msg;
  sdConvFat16(msg);
  if (lv_obj_is_valid(arStat)) lv_label_set_text(arStat, msg.c_str());
  toast(gSd.exfat ? "exFAT: converta no PC" : (gSd.fat == 32 ? "FAT32: converta no PC" : "SD ausente"));
}
static void arDelConfirmCb(lv_event_t *) {
  if (arDelTarget < 0 || arDelTarget >= arCount) return;
  if (!sdBeginSession()) { toast("SD ausente"); return; }
  String p = (arPath == "/") ? ("/" + arNames[arDelTarget]) : (arPath + arNames[arDelTarget]);
  SD.remove(p);
  sdEndSession();
  toast("deletado");
  if (lv_obj_is_valid(arDelBox)) lv_obj_add_flag(arDelBox, LV_OBJ_FLAG_HIDDEN);
  arFillCb(nullptr);
  arDelTarget = -1;
}
static void arDelCancelCb(lv_event_t *) {
  arDelTarget = -1;
  if (lv_obj_is_valid(arDelBox)) lv_obj_add_flag(arDelBox, LV_OBJ_FLAG_HIDDEN);
}
static void arRowCb(lv_event_t *e) {          // abrir item
  int i = (int)(intptr_t)lv_event_get_user_data(e);
  if (i < 0 || i >= arCount) return;
  String n = arNames[i];
  if (arIsDir[i]) {                           // entra na pasta
    arPath = (arPath == "/") ? ("/" + n + "/") : (arPath + n + "/");
    arFillCb(nullptr);
    return;
  }
  String lo = n; lo.toLowerCase();
  String full = (arPath == "/") ? ("/" + n) : (arPath + n);
  if (lo.endsWith(".wav")) {
    if (!audList()) { toast("nenhum .wav no SD"); return; }
    int f = -1;
    for (int k = 0; k < audCount; k++) if (audFiles[k] == full) { f = k; break; }
    buildPlayer(f >= 0 ? f : 0);
  } else if (lo.endsWith(".jpg") || lo.endsWith(".jpeg") || lo.endsWith(".bmp") || lo.endsWith(".png")) {
    buildImgView(full);
  } else if (lo.endsWith(".mp4") || lo.endsWith(".mov") || lo.endsWith(".mkv") ||
             lo.endsWith(".avi") || lo.endsWith(".webm")) {
    buildVideoView(full);
  } else {
    toast("formato: abrir pelo PC");
  }
}
static void arUpCb(lv_event_t *) {            // sobe um nivel
  if (arPath == "/") return;
  String p = arPath;
  if (p.endsWith("/")) p = p.substring(0, p.length() - 1);   // remove a barra final
  int k = p.lastIndexOf('/');
  arPath = (k <= 0) ? "/" : p.substring(0, k + 1);
  arFillCb(nullptr);
}
static void arFillCb(lv_event_t *) {
  if (gLvPause || gSdPause) return;
  if (!lv_obj_is_valid(arList)) return;
  // a tela pode ter sido destruida (nav apaga a anterior): nao tocar na lista morta
  if (lv_scr_act() != scrArquivo) return;
  int n = sdListDir(arPath, 40);
  if (n < 0) {
    lv_obj_clean(arList);
    if (lv_obj_is_valid(arStat)) lv_label_set_text(arStat, "SD ausente ou vazio");
    if (lv_obj_is_valid(arAddr)) lv_label_set_text(arAddr, "SD CARD");
    return;
  }
  // barra de endereco
  if (lv_obj_is_valid(arAddr)) {
    String ad = "SD CARD";
    if (arPath != "/") ad += " > " + arPath;
    lv_label_set_text(arAddr, ad.c_str());
  }
  // lista (pastas primeiro, depois arquivos) - linhas estilo Explorer
  lv_obj_clean(arList);
  for (int i = 0; i < arCount; i++) {
    int y = i * 15;
    lv_obj_t *b = lv_btn_create(arList);
    lv_obj_set_pos(b, 0, y);
    lv_obj_set_size(b, 116, 14);
    lv_obj_set_style_bg_color(b, lv_color_hex(0x161A22), 0);
    lv_obj_set_style_radius(b, 4, 0);
    lv_obj_set_style_shadow_width(b, 0, 0);
    lv_obj_set_style_pad_all(b, 0, 0);
    lv_obj_add_event_cb(b, arRowCb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    lv_obj_add_event_cb(b, clickBeep, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(b, [](lv_event_t *e) {
      arDelTarget = (int)(intptr_t)lv_event_get_user_data(e);
      if (lv_obj_is_valid(arDelBox)) {
        lv_obj_clear_flag(arDelBox, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(arDelBox);
      }
    }, LV_EVENT_LONG_PRESSED, NULL);
    // icone do tipo (buffer proprio por linha, liberado com o canvas)
    uint16_t *fbuf = (uint16_t *)heap_caps_calloc(FI_SZ, FI_SZ * 2, MALLOC_CAP_SPIRAM);
    lv_obj_t *ic = lv_canvas_create(b);
    if (fbuf) {
      lv_canvas_set_buffer(ic, fbuf, FI_SZ, FI_SZ, LV_IMG_CF_TRUE_COLOR);
      drawFileIco(ic, arFileKind(arNames[i], arIsDir[i]));
      lv_obj_add_event_cb(ic, [](lv_event_t *e) {
        if (lv_event_get_code(e) != LV_EVENT_DELETE) return;
        uint16_t *bb = (uint16_t *)lv_event_get_user_data(e);
        if (bb) heap_caps_free(bb);
      }, LV_EVENT_DELETE, fbuf);
    }
    lv_obj_set_pos(ic, 2, 1);
    lv_obj_clear_flag(ic, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(ic, LV_OBJ_FLAG_SCROLLABLE);
    // nome
    lv_obj_t *l = lv_label_create(b);
    lv_label_set_text(l, arNames[i].c_str());
    lv_obj_set_pos(l, 17, 0);
    lv_obj_set_width(l, 76);
    lv_label_set_long_mode(l, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(l, F_SM, 0);
    lv_obj_set_style_text_color(l, arIsDir[i] ? lv_color_hex(0xF2C230) : REDMI_TXT, 0);
    lv_obj_clear_flag(l, LV_OBJ_FLAG_CLICKABLE);
    // tamanho (coluna da direita)
    char sz[12];
    if (arIsDir[i]) snprintf(sz, sizeof(sz), "<DIR>");
    else arFormatSize(arSizes[i], sz, sizeof(sz));
    lv_obj_t *ls = lv_label_create(b);
    lv_label_set_text(ls, sz);
    lv_obj_set_pos(ls, 92, 0);
    lv_obj_set_width(ls, 22);
    lv_obj_set_style_text_align(ls, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_font(ls, F_SM, 0);
    lv_obj_set_style_text_color(ls, REDMI_DIM, 0);
    lv_obj_clear_flag(ls, LV_OBJ_FLAG_CLICKABLE);
  }
  // barra de status
  if (lv_obj_is_valid(arStat)) {
    char b[40];
    uint32_t tot = 0;
    for (int i = 0; i < arCount; i++) tot += arSizes[i];
    char sz[12]; arFormatSize(tot, sz, sizeof(sz));
    snprintf(b, sizeof(b), "%d itens  %s", arCount, sz);
    lv_label_set_text(arStat, b);
  }
}
void buildArquivo() {
  scrArquivo = lv_obj_create(NULL);
  scrStyle(scrArquivo);
  appHeader(scrArquivo, "ARQUIVOS");
  lv_obj_clear_flag(scrArquivo, LV_OBJ_FLAG_SCROLLABLE);
  // ---- barra de ferramentas (estilo Explorer) ----
  const char *tb[6] = { LV_SYMBOL_REFRESH, LV_SYMBOL_UP, LV_SYMBOL_IMAGE, LV_SYMBOL_UPLOAD,
                         "i", LV_SYMBOL_DRIVE };
  for (int i = 0; i < 6; i++) {
    lv_obj_t *b = lv_btn_create(scrArquivo);
    lv_obj_set_pos(b, 2 + i * 21, 46);
    lv_obj_set_size(b, 19, 18);
    lv_obj_set_style_bg_color(b, lv_color_hex(0x1C202B), 0);
    lv_obj_set_style_radius(b, 5, 0);
    lv_obj_set_style_shadow_width(b, 0, 0);
    lv_obj_add_event_cb(b, clickBeep, LV_EVENT_CLICKED, NULL);
    switch (i) {
      case 0: lv_obj_add_event_cb(b, [](lv_event_t *) { arFillCb(nullptr); }, LV_EVENT_CLICKED, NULL); break;
      case 1: lv_obj_add_event_cb(b, [](lv_event_t *) { arUpCb(nullptr); }, LV_EVENT_CLICKED, NULL); break;
      case 2: lv_obj_add_event_cb(b, [](lv_event_t *) { buildFileMidia(); }, LV_EVENT_CLICKED, NULL); break;
      case 3: lv_obj_add_event_cb(b, [](lv_event_t *) { arSendCb(nullptr); }, LV_EVENT_CLICKED, NULL); break;
      case 4: lv_obj_add_event_cb(b, [](lv_event_t *) { arInfoCb(nullptr); }, LV_EVENT_CLICKED, NULL); break;
      case 5: lv_obj_add_event_cb(b, [](lv_event_t *) { arFatCb(nullptr); }, LV_EVENT_CLICKED, NULL); break;
    }
    lv_obj_t *ic = lv_label_create(b);
    lv_label_set_text(ic, tb[i]);
    lv_obj_set_style_text_font(ic, F_SM, 0);
    lv_obj_set_style_text_color(ic, REDMI_TXT, 0);
    lv_obj_center(ic);
    lv_obj_clear_flag(ic, LV_OBJ_FLAG_CLICKABLE);
  }
  // ---- barra de endereco ----
  lv_obj_t *adr = lv_obj_create(scrArquivo);
  lv_obj_set_pos(adr, 2, 66);
  lv_obj_set_size(adr, 124, 15);
  lv_obj_set_style_bg_color(adr, lv_color_hex(0x12151C), 0);
  lv_obj_set_style_bg_opa(adr, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(adr, 4, 0);
  lv_obj_set_style_border_width(adr, 1, 0);
  lv_obj_set_style_border_color(adr, lv_color_hex(0x2A3444), 0);
  lv_obj_clear_flag(adr, LV_OBJ_FLAG_SCROLLABLE);
  arAddr = lv_label_create(adr);
  lv_obj_set_pos(arAddr, 4, 1);
  lv_obj_set_width(arAddr, 116);
  lv_label_set_long_mode(arAddr, LV_LABEL_LONG_DOT);
  lv_obj_set_style_text_font(arAddr, F_SM, 0);
  lv_obj_set_style_text_color(arAddr, REDMI_TXT, 0);
  lv_label_set_text(arAddr, "SD CARD");
  // ---- cabecalho das colunas ----
  lv_obj_t *ch1 = uiLabel(scrArquivo, "Nome", 4, 83, F_SM, lv_color_to32(REDMI_DIM));
  lv_obj_t *ch2 = uiLabel(scrArquivo, "Tamanho", 94, 83, F_SM, lv_color_to32(REDMI_DIM));
  lv_obj_set_width(ch2, 30);
  lv_obj_set_style_text_align(ch2, LV_TEXT_ALIGN_RIGHT, 0);
  (void)ch1;
  // ---- lista rolavel ----
  arList = lv_obj_create(scrArquivo);
  lv_obj_set_pos(arList, 2, 96);
  lv_obj_set_size(arList, 124, 44);
  lv_obj_set_style_bg_opa(arList, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_opa(arList, 0, 0);
  lv_obj_set_style_pad_all(arList, 0, 0);
  lv_obj_set_scrollbar_mode(arList, LV_SCROLLBAR_MODE_AUTO);
  lv_obj_set_scroll_dir(arList, LV_DIR_VER);
  // ---- barra de status ----
  arStat = uiLabel(scrArquivo, "0 itens", 4, 142, F_SM, lv_color_to32(REDMI_DIM));
  lv_obj_set_width(arStat, 120);
  lv_label_set_long_mode(arStat, LV_LABEL_LONG_DOT);
  arOut = arStat;
  // ---- modal de confirmacao de exclusao ----
  arDelBox = lv_obj_create(scrArquivo);
  lv_obj_set_pos(arDelBox, 5, 60);
  lv_obj_set_size(arDelBox, 118, 46);
  lv_obj_set_style_bg_color(arDelBox, lv_color_hex(0x1E2430), 0);
  lv_obj_set_style_bg_opa(arDelBox, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(arDelBox, 8, 0);
  lv_obj_set_style_border_width(arDelBox, 1, 0);
  lv_obj_set_style_border_color(arDelBox, lv_color_hex(0x3A4250), 0);
  lv_obj_set_style_shadow_width(arDelBox, 12, 0);
  lv_obj_set_style_shadow_opa(arDelBox, LV_OPA_50, 0);
  lv_obj_set_style_pad_all(arDelBox, 2, 0);
  lv_obj_clear_flag(arDelBox, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_t *dq = lv_label_create(arDelBox);
  lv_label_set_text(dq, "Apagar este item?");
  lv_obj_set_style_text_font(dq, F_SM, 0);
  lv_obj_set_style_text_color(dq, REDMI_TXT, 0);
  lv_obj_align(dq, LV_ALIGN_TOP_MID, 0, 4);
  lv_obj_t *bs = uiBtn(arDelBox, "Apagar", arDelConfirmCb, 52, 16, 0x5A1414, NULL);
  if (bs) lv_obj_set_pos(bs, 2, 26);
  lv_obj_t *bn = uiBtn(arDelBox, "Nao", arDelCancelCb, 52, 16, 0x161A22, NULL);
  if (bn) lv_obj_set_pos(bn, 58, 26);
  lv_obj_add_flag(arDelBox, LV_OBJ_FLAG_HIDDEN);
  // leitura inicial DEPOIS que a tela pintou (montar o SD leva 200..500 ms):
  // o usuario ve o Explorer na hora, a lista aparece logo em seguida
  if (lv_obj_is_valid(arStat)) lv_label_set_text(arStat, "lendo cartao...");
  lv_timer_t *tt = lv_timer_create([](lv_timer_t *t) {
    lv_timer_del(t);
    if (lv_scr_act() != scrArquivo) return;   // saiu da tela antes: nao mexe na lista
    arFillCb(nullptr);
  }, 40, NULL);
  if (tt) tt->repeat_count = 1;
}

// ============================================================
//  FOTOS - galeria de imagens do SD (raiz) em tela cheia
// ============================================================
static lv_obj_t *fmScr, *fmLbl, *fmCan;
static String fmFiles[24];
static int fmCount = 0, fmIdx = 0;

static void fmScan() {
  fmCount = 0;
  if (!sdBeginSession()) return;
  const char *dirs[3] = { "/", "/FOTOS", "/DCIM" };
  for (int d = 0; d < 3 && fmCount < 24; d++) {
    File r = SD.open(dirs[d]);
    if (!r) continue;
    File f = r.openNextFile();
    while (f && fmCount < 24) {
      bool dir = f.isDirectory();
      String nm = String(f.name());
      f.close();
      String lo = nm; lo.toLowerCase();
      if (!dir && (lo.endsWith(".jpg") || lo.endsWith(".jpeg") ||
                   lo.endsWith(".bmp") || lo.endsWith(".png"))) {
        fmFiles[fmCount++] = (String(dirs[d]) + (dirs[d][1] ? "/" : "") + nm);
      }
      f = r.openNextFile();
    }
    r.close();
  }
  sdEndSession();
}
static void fmShowCur() {
  if (!lv_obj_is_valid(fmLbl)) return;
  if (!fmCount) { lv_label_set_text(fmLbl, "sem imagens"); return; }
  char b[16]; snprintf(b, sizeof(b), "%d/%d", fmIdx + 1, fmCount);
  lv_label_set_text(fmLbl, b);
  if (!wpDecodeFile(fmFiles[fmIdx])) { toast("imagem nao abriu"); return; }
  if (lv_obj_is_valid(fmCan)) lv_obj_invalidate(fmCan);
}
static void fmPrevCb(lv_event_t *) { if (!fmCount) return; fmIdx = (fmIdx + fmCount - 1) % fmCount; fmShowCur(); }
static void fmNextCb(lv_event_t *) { if (!fmCount) return; fmIdx = (fmIdx + 1) % fmCount; fmShowCur(); }
static void fmWallCb(lv_event_t *) {
  if (!fmCount) return;
  if (!wpDecodeFile(fmFiles[fmIdx])) { toast("imagem nao abriu"); return; }
  wpSaveSpiffs(); cfg.wallp = true; cfgSave();
  toast("Wallpaper definido!");
  buildArquivo(); nav(scrArquivo, LV_SCR_LOAD_ANIM_MOVE_RIGHT);
}
void buildFileMidia() {
  fmScan();
  if (!fmCount) { toast("nenhuma imagem no SD"); return; }
  fmIdx = 0;
  fmScr = lv_obj_create(NULL);
  scrStyle(fmScr);
  lv_obj_clear_flag(fmScr, LV_OBJ_FLAG_SCROLLABLE);
  fmCan = lv_canvas_create(fmScr);                 // foto em tela cheia (fundo)
  lv_obj_set_pos(fmCan, 0, 0);
  lv_obj_set_size(fmCan, 128, 160);
  lv_canvas_set_buffer(fmCan, gWpaper, WP_W, WP_H, LV_IMG_CF_TRUE_COLOR);
  // faixa escura no topo (status/header legiveis sobre qualquer foto)
  lv_obj_t *shTop = lv_obj_create(fmScr);
  lv_obj_set_pos(shTop, 0, 0);
  lv_obj_set_size(shTop, 128, 42);
  lv_obj_set_style_bg_color(shTop, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(shTop, LV_OPA_50, 0);
  lv_obj_set_style_border_opa(shTop, 0, 0);
  lv_obj_set_style_radius(shTop, 0, 0);
  lv_obj_clear_flag(shTop, LV_OBJ_FLAG_CLICKABLE);
  appHeader(fmScr, "FOTOS");                       // status + voltar por cima
  // contador numa pilula escura (legivel sobre qualquer foto)
  lv_obj_t *pill = lv_obj_create(fmScr);
  lv_obj_set_pos(pill, 44, 44);
  lv_obj_set_size(pill, 40, 14);
  lv_obj_set_style_bg_color(pill, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(pill, LV_OPA_60, 0);
  lv_obj_set_style_radius(pill, 7, 0);
  lv_obj_set_style_border_opa(pill, 0, 0);
  lv_obj_set_style_pad_all(pill, 0, 0);
  lv_obj_clear_flag(pill, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(pill, LV_OBJ_FLAG_CLICKABLE);
  fmLbl = lv_label_create(pill);
  lv_obj_set_style_text_font(fmLbl, F_SM, 0);
  lv_obj_set_style_text_color(fmLbl, REDMI_TXT, 0);
  lv_obj_center(fmLbl);
  // faixa inferior com controles
  lv_obj_t *bar = lv_obj_create(fmScr);
  lv_obj_set_pos(bar, 0, 136);
  lv_obj_set_size(bar, 128, 24);
  lv_obj_set_style_bg_color(bar, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(bar, LV_OPA_70, 0);
  lv_obj_set_style_border_opa(bar, 0, 0);
  lv_obj_set_style_radius(bar, 0, 0);
  lv_obj_set_style_pad_all(bar, 0, 0);
  lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(bar, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_t *bp = uiBtn(fmScr, LV_SYMBOL_LEFT, fmPrevCb, 22, 18, 0x161A22, NULL);
  if (bp) lv_obj_set_pos(bp, 2, 139);
  lv_obj_t *bn = uiBtn(fmScr, LV_SYMBOL_RIGHT, fmNextCb, 22, 18, 0x161A22, NULL);
  if (bn) lv_obj_set_pos(bn, 80, 139);
  lv_obj_t *bw = uiBtn(fmScr, "Fundo", fmWallCb, 48, 18, 0x10304A, NULL);
  if (bw) lv_obj_set_pos(bw, 28, 139);
  lv_obj_t *bk = uiBtn(fmScr, LV_SYMBOL_CLOSE, [](lv_event_t *) {
    buildArquivo(); nav(scrArquivo, LV_SCR_LOAD_ANIM_MOVE_RIGHT);
  }, 22, 18, 0x161A22, NULL);
  if (bk) lv_obj_set_pos(bk, 104, 139);
  fmShowCur();
  nav(fmScr, LV_SCR_LOAD_ANIM_MOVE_LEFT);
}

// ============================================================
//  PLAYER DE MUSICA (WAV/PCM do SD -> speaker PWM/LEDC, toca da PSRAM)
//  - ESP32-S3 nao tem DAC interno: saida PWM 8-bit; duty = amplitude.
//  - MP3/MP4/OGG/etc sao convertidos p/ WAV no NAVEGADOR, no upload.
//  - carrega a faixa na PSRAM e toca de la: SD e TFT dividem os GPIOs
//    9/10/34 em barramentos distintos; tocar direto do SD corromperia o TFT.
// ============================================================
#define AUD_MAX 1400000
static uint8_t  *audBuf  = nullptr;    // PSRAM: arquivo (header+data), truncado em AUD_MAX
static uint8_t  *audData = nullptr;    // inicio do 'data' dentro de audBuf
static uint32_t  audDLen = 0;          // bytes de audio uteis
static uint16_t  audBits = 0, audCh = 0;
static uint32_t  audRate = 0;
static TaskHandle_t audTaskH = nullptr;
static volatile bool     audRun = false;    // task tocando
static volatile bool     audPause = false;  // pausado
static volatile bool     audEnded = false;  // acabou a faixa naturalmente
static volatile uint8_t  audProg = 0;       // 0..100
static volatile float     audPos = 0, audDur = 0;   // seg
static char     audCurName[40] = "";

// parser WAV sobre o buffer inteiro na RAM (header junto do data)
static bool wavRAM(const uint8_t *b, uint32_t n,
                   const uint8_t *&data, uint32_t &dlen,
                   uint16_t &bits, uint16_t &ch, uint32_t &rate) {
  if (n < 12 || memcmp(b, "RIFF", 4) || memcmp(b + 8, "WAVE", 4)) return false;
  uint32_t pos = 12;
  while (pos + 8 <= n) {
    const uint8_t *cid = b + pos;
    uint32_t csz = (uint32_t)cid[4] | ((uint32_t)cid[5] << 8) |
                   ((uint32_t)cid[6] << 16) | ((uint32_t)cid[7] << 24);
    const uint8_t *body = b + pos + 8;
    if (!memcmp(cid, "fmt ", 4) && csz >= 16 && body + 16 <= b + n) {
      ch   = body[2] | (body[3] << 8);
      rate = (uint32_t)body[4] | ((uint32_t)body[5] << 8) | ((uint32_t)body[6] << 16) | ((uint32_t)body[7] << 24);
      bits = body[14] | (body[15] << 8);
    } else if (!memcmp(cid, "data", 4)) {
      data = body;
      dlen = csz;
      if (body + dlen > b + n) dlen = (uint32_t)((b + n) - body);   // truncado
      return bits && rate && ch;              // precisa ter lido o fmt antes
    }
    if (csz & 1) csz++;
    pos += 8 + csz;
  }
  return false;
}
static bool audList() {      // varre raiz + pastas de midia por *.wav
  audCount = 0;
  if (!sdBeginSession()) return false;
  const char *dirs[5] = { "/", "/MUSICAS", "/DCIM", "/FOTOS", "/VIDEOS" };
  for (int d = 0; d < 5 && audCount < 32; d++) {
    File r = SD.open(dirs[d]);
    if (!r) continue;
    File f = r.openNextFile();
    while (f && audCount < 32) {
      bool isDir = f.isDirectory();
      String nm = String(f.name());
      f.close();
      if (!isDir && (nm.endsWith(".wav") || nm.endsWith(".WAV"))) {
        String full = String(dirs[d]) + (dirs[d][1] ? "/" : "") + nm;
        audFiles[audCount++] = full;
      }
      f = r.openNextFile();
    }
    r.close();
  }
  sdEndSession();
  return audCount > 0;
}
static bool audLoad(int idx) {   // le a faixa pra PSRAM (sessao SD no nucleo 1)
  if (idx < 0 || idx >= audCount) return false;
  if (audRun) { audRun = false; for (int i = 0; i < 60 && audTaskH; i++) vTaskDelay(pdMS_TO_TICKS(5)); }
  if (!sdBeginSession()) return false;
  File f = SD.open(audFiles[idx]);
  bool ok = false;
  if (f) {
    uint32_t sz = f.size();
    uint32_t rd = sz < AUD_MAX ? sz : AUD_MAX;
    if (!audBuf) {
      if (!psNeed(AUD_MAX)) { f.close(); sdEndSession(); return false; }
      audBuf = (uint8_t *)heap_caps_malloc(AUD_MAX, MALLOC_CAP_SPIRAM);
      if (audBuf) psAllocated[PSR_AUDIO] = true;
    }
    if (audBuf) {
      f.seek(0);
      uint32_t got = f.read(audBuf, rd);
      const uint8_t *d = nullptr; uint32_t dl = 0; uint16_t bt = 0, ch = 0; uint32_t rt = 0;
      if (wavRAM(audBuf, got, d, dl, bt, ch, rt) && rt) {
        audData = (uint8_t *)d; audDLen = dl; audBits = bt; audCh = ch; audRate = rt;
        audDur = (float)dl / ((bt / 8.0) * ch * rt);
        snprintf(audCurName, sizeof(audCurName), "%s", audFiles[idx].c_str());
        ok = true;
      }
    }
    f.close();
  }
  sdEndSession();
  return ok;
}
// task de reproducao: fixada no nucleo 0 (o loop/LVGL roda no nucleo 1)
static void audTaskFn(void *) {
  int bpf = (audBits / 8) * audCh;
  uint32_t period = 1000000UL / audRate;
  uint32_t played = 0;
  spkCarrierEnsure();               // PWM 8-bit, portadora 200kHz
  uint8_t buf[256];
  while (audRun && played < audDLen) {
    if (audPause) { ledcWrite(SPK_PIN, 128); vTaskDelay(pdMS_TO_TICKS(25)); continue; }
    uint32_t remain = audDLen - played;
    uint32_t want = remain < sizeof(buf) ? remain : sizeof(buf);
    want = (want / bpf) * bpf;
    if (want == 0) want = remain < (uint32_t)bpf ? remain : bpf;
    if (want == 0) break;
    memcpy(buf, audData + played, want);
    int frames = want / bpf;
    uint32_t t0 = micros();
    for (int i = 0; i < frames && audRun; i++) {
      uint32_t wt = t0 + (uint32_t)i * period;
      while (audRun && (int32_t)(micros() - wt) < 0) { /* espera o instante */ }
      int v;
      if (audBits == 16) {
        int16_t s = (int16_t)(buf[i * bpf] | (buf[i * bpf + 1] << 8));
        if (audCh == 2) {
          int16_t s2 = (int16_t)(buf[i * bpf + 2] | (buf[i * bpf + 3] << 8));
          s = (int16_t)(((int)s + s2) / 2);            // downmix L+R
        }
        v = (s + 32768) >> 8;                           // -32768..32767 -> 0..255
      } else {
        v = buf[i * bpf];                               // 8-bit ja eh 0..255
      }
      ledcWrite(SPK_PIN, (uint32_t)v);
    }
    played += (uint32_t)frames * bpf;
    audProg = (uint8_t)((uint64_t)played * 100 / audDLen);
    audPos = (float)played / ((float)bpf * audRate);
  }
  ledcWrite(SPK_PIN, 0);                                // sem DC de repouso
  if (audRun) audEnded = true;                          // fim natural da faixa
  audRun = false; audTaskH = nullptr;
  gAudioBusy = false;
  vTaskDelete(NULL);
}
// ---------- carga do arquivo em task (a leitura do SD pode levar >1 s) ----------
static volatile bool audLoading = false;    // lendo o arquivo do SD agora
static volatile bool audLoadDone = false;   // terminou (com ou sem erro)
static volatile bool audLoadErr = false;
static volatile bool audCancel = false;     // pedido de cancelamento (ex.: servidor de arquivos)
static volatile bool audLoadBusy = false;   // a task de carga esta viva (segura o SD)
static volatile uint32_t audGen = 0;        // generation: cancela carga antiga
static int audLoadIdx = 0;
static void audLoadTask(void *) {
  uint32_t g = audGen;
  audLoadBusy = true;
  bool ok = false;
  if (!audCancel) ok = audLoad(audLoadIdx);   // se cancelou, nem abre o SD
  audLoadBusy = false;
  audLoading = false;
  if (g == audGen && !audCancel) { audLoadErr = !ok; audLoadDone = true; }
  vTaskDelete(NULL);
}
static void audStop();                 // (definido abaixo)
static void audStart(int idx) {             // arquivo JA carregado na PSRAM: comeca a tocar
  audIdx = idx; audRun = true; audPause = false; audEnded = false; audProg = 0; audPos = 0;
  gAudioBusy = true;
  xTaskCreatePinnedToCore(audTaskFn, "aud", 4096, NULL, 2, &audTaskH, 0);   // nucleo 0
}
static void audPlay(int idx) {              // carrega em task (nao trava a UI) e toca quando pronto
  if (audRun) audStop();                   // para a faixa atual antes de carregar a nova
  audCancel = false;
  audLoadIdx = idx; audGen++; audLoadErr = false; audLoadDone = false; audLoading = true;
  xTaskCreatePinnedToCore(audLoadTask, "audl", 4096, NULL, 1, NULL, 0);    // nucleo 1
}
static void audStop() {
  audRun = false;
  audCancel = true;              // cancela qualquer carga em andamento
  audGen++;                      // generation: a task antiga nao inicia a reproducao
  audLoading = false; audLoadDone = false;
  for (int i = 0; i < 60 && audTaskH; i++) vTaskDelay(pdMS_TO_TICKS(5));
  audPause = false;
  gAudioBusy = false;
}
static void audNext() { int n = audIdx + 1; if (n >= audCount) n = 0; audPlay(n); }
static void audPrev() { int p = audIdx - 1; if (p < 0) p = audCount - 1; audPlay(p); }

// ============================================================
//  IMPLEMENTACAO DO GERENCIADOR DE PSRAM (buffers ao final do arquivo)
// ============================================================
static void psFreeRes(uint8_t r) {
  switch (r) {
    case PSR_AUDIO:
      if (audRun || audLoading) return;          // ainda tocando/carregando: nao mexe
      if (audBuf) { heap_caps_free(audBuf); audBuf = nullptr; }
      audData = nullptr; audDLen = 0; audBits = 0; audCh = 0; audRate = 0;
      psAllocated[r] = false;
      break;
    case PSR_GOOGLE:
      for (int i = 0; i < 4; i++) {
        if (goPix[i]) { heap_caps_free(goPix[i]); goPix[i] = nullptr; }
      }
      if (gJpgBuf) { heap_caps_free(gJpgBuf); gJpgBuf = nullptr; }
      psAllocated[r] = false;
      break;
    case PSR_DOOM:
      if (dooFB) { heap_caps_free(dooFB); dooFB = nullptr; }
      psAllocated[r] = false;
      break;
    case PSR_CHESS:
      for (int i = 0; i < 64; i++) {
        if (xzBufPeca[i]) { heap_caps_free(xzBufPeca[i]); xzBufPeca[i] = nullptr; }
      }
      psAllocated[r] = false;
      break;
    default: break;
  }
}
static void psCleanup(bool force) {
  uint32_t now = millis();
  // registros apontando p/ telas ja apagadas: libera o recurso na hora
  for (int i = 0; i < PS_MAP_MAX; i++) {
    if (!psMapScr[i] || lv_obj_is_valid(psMapScr[i])) continue;
    uint8_t r = psMapRes[i];
    psMapScr[i] = nullptr;
    if (psResStillMapped(r)) continue;      // outra tela viva ainda usa
    psMark(r, false);
    psLastUse[r] = now - PS_IDLE_MS - 1;    // conta como ocioso: libera ja'
    if (psIsGame(r) || r == PSR_AUDIO) psFreeAt[r] = now;
  }
  for (uint8_t r = 0; r < PSR_COUNT; r++) {
    if (!psAllocated[r]) continue;
    if (psInUse[r]) continue;                              // app aberto: prioridade
    if (!force && (uint32_t)(now - psLastUse[r]) < PS_IDLE_MS) continue;
    psFreeRes(r);
  }
}
static size_t psUsedBytes() {
  size_t n = 0;
  if (psAllocated[PSR_AUDIO]  && audBuf)  n += AUD_MAX;
  if (psAllocated[PSR_GOOGLE]) {
    n += 150000;
    for (int i = 0; i < 4; i++) if (goPix[i]) n += (size_t)IMG_W * IMG_H * 2;
  }
  if (psAllocated[PSR_DOOM]   && dooFB)  n += (size_t)DM_VW * DM_VH * 2;
  if (psAllocated[PSR_CHESS]) {
    for (int i = 0; i < 64; i++) if (xzBufPeca[i]) n += (size_t)PC_SZ * PC_SZ * 2;
  }
  return n;
}

static lv_obj_t *plScr, *plName, *plTime, *plBar, *plBtnPp;
static void plFmt(char *o, size_t n, float s) {
  int t = (int)s; if (t < 0) t = 0;
  snprintf(o, n, "%d:%02d", t / 60, t % 60);
}
static void plTick(lv_timer_t *t) {
  if (!lv_obj_is_valid(plScr)) { audStop(); lv_timer_del(t); return; }
  if (lv_scr_act() != plScr) { audStop(); lv_timer_del(t); return; }   // saiu: para
  // carga terminou? comeca a tocar (ou avisa o erro)
  if (audLoadDone) {
    audLoadDone = false;
    if (audLoadErr) { toast("nao deu p/ carregar a faixa"); audStop(); }
    else { if (lv_obj_is_valid(plName)) lv_label_set_text(plName, audCurName); audStart(audLoadIdx); }
  }
  if (audLoading) { if (lv_obj_is_valid(plName)) lv_label_set_text(plName, "carregando..."); }
  else if (audCurName[0] && lv_obj_is_valid(plName)) lv_label_set_text(plName, audCurName);
  if (lv_obj_is_valid(plBar)) lv_bar_set_value(plBar, audProg, LV_ANIM_OFF);
  if (lv_obj_is_valid(plTime)) {
    char a[8], b[8], c[24];
    plFmt(a, sizeof(a), audPos); plFmt(b, sizeof(b), audDur);
    snprintf(c, sizeof(c), "%s / %s", a, b);
    lv_label_set_text(plTime, c);
  }
  if (lv_obj_is_valid(plBtnPp) && lv_obj_get_child(plBtnPp, 0))
    lv_label_set_text(lv_obj_get_child(plBtnPp, 0), audPause ? LV_SYMBOL_PLAY : LV_SYMBOL_PAUSE);
  if (audEnded) { audEnded = false; if (audIdx + 1 < audCount) audNext(); }
}
static void plPlayCb(lv_event_t *) { if (audRun) audPause = !audPause; }
static void plNextCb(lv_event_t *) { if (audCount) audNext(); }
static void plPrevCb(lv_event_t *) { if (audCount) audPrev(); }
static void plExitCb(lv_event_t *) { audStop(); buildArquivo(); nav(scrArquivo, LV_SCR_LOAD_ANIM_MOVE_RIGHT); }
void buildPlayer(int idx) {
  if (!audList()) { toast("nenhum .wav no SD (converta no upload)"); return; }
  plScr = lv_obj_create(NULL);
  psMapReg(plScr, PSR_AUDIO);           // WAV de 1,4 MB na PSRAM: libera 90s apos sair
  scrStyle(plScr);
  appHeader(plScr, "MUSICA");
  lv_obj_clear_flag(plScr, LV_OBJ_FLAG_SCROLLABLE);
  plName = uiLabel(plScr, "carregando...", 2, 44, F_SM, lv_color_to32(REDMI_TXT));
  lv_obj_set_width(plName, 124);
  lv_label_set_long_mode(plName, LV_LABEL_LONG_DOT);
  plBar = lv_bar_create(plScr);
  lv_obj_set_pos(plBar, 4, 80); lv_obj_set_size(plBar, 120, 8);
  lv_obj_set_style_bg_color(plBar, REDMI_CARD, 0);
  lv_obj_set_style_bg_color(plBar, REDMI_ACC, LV_PART_INDICATOR);
  lv_obj_set_style_radius(plBar, 4, 0);
  plTime = uiLabel(plScr, "0:00 / 0:00", 0, 62, F_SM, lv_color_to32(REDMI_DIM));
  lv_obj_set_width(plTime, 128); lv_obj_set_style_text_align(plTime, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_t *bPrev = uiBtn(plScr, LV_SYMBOL_PREV,  plPrevCb, 34, 28, 0x161A22, NULL);
  if (bPrev) lv_obj_set_pos(bPrev, 8, 100);
  plBtnPp = uiBtn(plScr, LV_SYMBOL_PLAY, plPlayCb, 38, 28, 0x10304A, NULL);
  if (plBtnPp) lv_obj_set_pos(plBtnPp, 45, 100);
  lv_obj_t *bNext = uiBtn(plScr, LV_SYMBOL_NEXT, plNextCb, 34, 28, 0x161A22, NULL);
  if (bNext) lv_obj_set_pos(bNext, 86, 100);
  lv_obj_t *bStop = uiBtn(plScr, "Parar", plExitCb, 62, 20, 0x3B1B1B, NULL);
  if (bStop) lv_obj_set_pos(bStop, 33, 134);
  lv_refr_now(gDisp);                                 // mostra "carregando..." antes do load
  static lv_timer_t *tp = nullptr;
  if (tp) { lv_timer_del(tp); tp = nullptr; }
  tp = lv_timer_create(plTick, 250, NULL);
  audPlay(idx < 0 || idx >= audCount ? 0 : idx);
  nav(plScr, LV_SCR_LOAD_ANIM_MOVE_LEFT);
}

// ============================================================
//  BADBLE - teclado Bluetooth (BLE HID) + runner de script /duck.txt
//  NAO mexe na USB: usa o radio BLE (o mesmo do Scan). Pareie no
//  celular/PC (aparece como "Wear-KB"), toque em Rodar p/ digitar.
//  Script (Ducky-lite): STRING / ENTER / TAB / ESC / DELAY /
//  DEFAULTDELAY / REM. O texto do SD e carregado p/ RAM antes de
//  tocar o BLE (SD e TFT dividem GPIOs; BLE nao usa esse SPI).
// ============================================================
#if HW_BLE
#include <BLEHIDDevice.h>
#include <BLEAdvertising.h>
static BLEHIDDevice   *bbHid = nullptr;
static BLECharacteristic *bbKey = nullptr;
static BLEServer     *bbSrv = nullptr;
static volatile bool  bbOn = false;       // servidor HID no ar
static volatile bool  bbConnected = false;
static volatile bool  bbChanged = false;  // status mudou -> atualiza label
static volatile bool  bbRun = false;      // runner rodando
static volatile bool  bbStop = false;     // pedido de parada
static String         gDuck = "";         // script carregado

// descritor HID de teclado (canonico ESP32; REPORT_ID 1)
static const uint8_t bbRepMap[] = {
  0x05,0x01, 0x09,0x06, 0xA1,0x01, 0x85,0x01, 0x05,0x07,
  0x19,0xE0, 0x29,0xE7, 0x15,0x00, 0x25,0x01, 0x75,0x01, 0x95,0x08,
  0x81,0x02, 0x95,0x01, 0x75,0x08, 0x81,0x01, 0x95,0x05, 0x75,0x01,
  0x05,0x08, 0x19,0x01, 0x29,0x05, 0x91,0x02, 0x95,0x01, 0x75,0x03,
  0x91,0x01, 0x95,0x06, 0x75,0x08, 0x15,0x00, 0x25,0xFF, 0x05,0x07,
  0x19,0x00, 0x29,0xFF, 0x81,0x00, 0xC0
};
class BbSrvCb : public BLEServerCallbacks {
  void onConnect(BLEServer *p) override { (void)p; bbConnected = true;  bbChanged = true; }
  void onDisconnect(BLEServer *p) override { (void)p; bbConnected = false; bbChanged = true; BLEDevice::startAdvertising(); }
};
static void bbSendKey(uint8_t mod, uint8_t key) {
  if (!bbKey) return;
  uint8_t r[8] = { mod, 0, key, 0, 0, 0, 0, 0 };
  bbKey->setValue(r, 8); bbKey->notify();
  delay(15);
  uint8_t z[8] = { 0,0,0,0,0,0,0,0 };
  bbKey->setValue(z, 8); bbKey->notify();
  delay(8);
}
static uint8_t bbHidCode(char c, uint8_t *shift) {   // ASCII -> keycode USB-HID
  *shift = 0;
  if (c >= 'a' && c <= 'z') return 0x04 + (c - 'a');
  if (c >= 'A' && c <= 'Z') { *shift = 1; return 0x04 + (c - 'A'); }
  if (c >= '1' && c <= '9') return 0x1E + (c - '1');
  if (c == '0') return 0x27;
  switch (c) {
    case '!': *shift=1; return 0x1E;  case '@': *shift=1; return 0x1F;
    case '#': *shift=1; return 0x20;  case '$': *shift=1; return 0x21;
    case '%': *shift=1; return 0x22;  case '^': *shift=1; return 0x23;
    case '&': *shift=1; return 0x24;  case '*': *shift=1; return 0x25;
    case '(': *shift=1; return 0x26;  case ')': *shift=1; return 0x27;
    case ' ': return 0x2C;
    case '-': return 0x2D;  case '_': *shift=1; return 0x2D;
    case '=': return 0x2E;  case '+': *shift=1; return 0x2E;
    case '[': return 0x2F;  case '{': *shift=1; return 0x2F;
    case ']': return 0x30;  case '}': *shift=1; return 0x30;
    case '\\': return 0x31; case '|': *shift=1; return 0x31;
    case ';': return 0x33;  case ':': *shift=1; return 0x33;
    case '\'': return 0x34; case '"': *shift=1; return 0x34;
    case '`': return 0x35;  case '~': *shift=1; return 0x35;
    case ',': return 0x36;  case '<': *shift=1; return 0x36;
    case '.': return 0x37;  case '>': *shift=1; return 0x37;
    case '/': return 0x38;  case '?': *shift=1; return 0x38;
    case '\n': case '\r': return 0x28;
    case '\t': return 0x2B;
  }
  return 0;
}
static void bbTypeCh(char c) {
  uint8_t sh; uint8_t k = bbHidCode(c, &sh);
  if (!k) return;
  bbSendKey(sh ? 0x02 : 0x00, k);
}
static void bbTypeStr(const String &s) {
  for (unsigned i = 0; i < s.length(); i++) bbTypeCh(s[i]);
}
static void bbRunTask(void *) {
  int dd = 0, start = 0, len = gDuck.length();
  while (!bbStop && start < len) {
    int nl = gDuck.indexOf('\n', start);
    String line = gDuck.substring(start, nl < 0 ? len : nl);
    start = (nl < 0) ? len : nl + 1;
    line.trim();
    if (!line.length()) continue;
    String up = line; up.toUpperCase();
    if (up.startsWith("REM")) continue;
    if (up.startsWith("DEFAULT_DELAY") || up.startsWith("DEFAULTDELAY")) {
      int sp = line.indexOf(' ');
      if (sp >= 0) dd = line.substring(sp + 1).toInt();
      continue;
    }
    if (up.startsWith("DELAY")) {
      int sp = line.indexOf(' ');
      if (sp >= 0) {
        int ms = line.substring(sp + 1).toInt();
        uint32_t t0 = millis();
        while (!bbStop && millis() - t0 < (uint32_t)ms) delay(10);
      }
      continue;
    }
    if (up == "ENTER" || up == "RETURN") bbTypeCh('\n');
    else if (up == "TAB")  bbTypeCh('\t');
    else if (up == "ESC" || up == "ESCAPE") bbSendKey(0, 0x29);
    else if (up == "SPACE") bbTypeCh(' ');
    else if (up == "BACKSPACE") bbSendKey(0, 0x2A);
    else if (up == "DELETE") bbSendKey(0, 0x4C);
    else if (up == "UP")  bbSendKey(0, 0x52);
    else if (up == "DOWN") bbSendKey(0, 0x51);
    else if (up == "LEFT") bbSendKey(0, 0x50);
    else if (up == "RIGHT") bbSendKey(0, 0x4F);
    else if (up.startsWith("STRING")) {
      int sp = line.indexOf(' ');
      if (sp >= 0) bbTypeStr(line.substring(sp + 1));
    } else {
      bbTypeStr(line);                    // linha solta = digita como texto
    }
    if (dd && !bbStop) delay(dd);
  }
  bbRun = false; bbStop = false;
  vTaskDelete(NULL);
}
static void bbInit() {
  if (bbOn) return;
  BLEDevice::init("Wear-KB");
  bbSrv = BLEDevice::createServer();
  bbSrv->setCallbacks(new BbSrvCb());
  bbHid = new BLEHIDDevice(bbSrv);
  bbHid->reportMap((uint8_t *)bbRepMap, sizeof(bbRepMap));
  bbHid->manufacturer()->setValue("wear");
  bbHid->pnp(0x02, 0xe502, 0xa111, 0x0210);
  bbHid->hidInfo(0x00, 0x01);
  bbKey = bbHid->inputReport(1);          // REPORT_ID 1 (igual ao descritor)
  bbHid->startServices();
  BLEAdvertising *adv = BLEDevice::getAdvertising();
  adv->setAppearance(0x03C1);             // aparência: teclado HID
  adv->addServiceUUID(BLEUUID((uint16_t)0x1812));   // serviço HID
  adv->setScanResponse(true);
  adv->setMinPreferred(0x06);
  adv->setMaxPreferred(0x12);
  BLEDevice::startAdvertising();
  bbOn = true; bbConnected = false; gBtOn = true; bbChanged = true;
}
static void bbStopAll() {
  bbStop = true;
  for (int i = 0; i < 40 && bbRun; i++) delay(10);
  bbOn = false; gBtOn = false;
  BLEDevice::deinit(false);
  bbHid = nullptr; bbKey = nullptr; bbSrv = nullptr; bbConnected = false;
}
static void bbRunScript() {
  if (!bbOn) { toast("ligue o BadBLE primeiro"); return; }
  if (bbRun) { toast("ja esta rodando"); return; }
  if (!sdBeginSession()) { toast("SD ausente"); return; }   // sessao no nucleo 1
  File f = SD.open("/duck.txt");
  gDuck = "";
  if (f) { while (f.available()) gDuck += (char)f.read(); f.close(); }
  sdEndSession();
  if (!gDuck.length()) { toast("/duck.txt vazio ou ausente"); return; }
  bbStop = false; bbRun = true;
  xTaskCreate(bbRunTask, "bbrun", 4096, NULL, 1, NULL);
  toast("digitando... 'Parar' cancela");
}
static lv_obj_t *bbScr, *bbStat, *bbBtnOn;
static void bbTick(lv_timer_t *t) {
  if (!lv_obj_is_valid(bbScr) || lv_scr_act() != bbScr) { bbStopAll(); lv_timer_del(t); return; }
  if (bbChanged) {
    bbChanged = false;
    if (lv_obj_is_valid(bbStat))
      lv_label_set_text(bbStat, bbConnected ? "CONECTADO - pode rodar"
                                           : (bbOn ? "pareando..." : "desligado"));
    if (lv_obj_is_valid(bbBtnOn) && lv_obj_get_child(bbBtnOn, 0))
      lv_label_set_text(lv_obj_get_child(bbBtnOn, 0), bbOn ? "Desligar" : "Ligar");
    if (lv_obj_is_valid(bbBtnOn))
      lv_obj_set_style_bg_color(bbBtnOn, lv_color_hex(bbOn ? 0x3B1B1B : 0x1A3B25), 0);
  }
}
static void bbOnCb(lv_event_t *)  { if (bbOn) bbStopAll(); else bbInit(); }
static void bbRunCb(lv_event_t *) { bbRunScript(); }
static void bbStopRunCb(lv_event_t *) { if (bbRun) { bbStop = true; toast("parando..."); } }
static void bbBackCb(lv_event_t *) { bbStopAll(); buildToolbox(); nav(scrToolbox, LV_SCR_LOAD_ANIM_MOVE_RIGHT); }
void buildBadBLE() {
  bbScr = lv_obj_create(NULL);
  scrStyle(bbScr);
  appHeader(bbScr, "BADBLE");
  lv_obj_clear_flag(bbScr, LV_OBJ_FLAG_SCROLLABLE);
  uiLabel(bbScr, "teclado BLE 'Wear-KB'", 0, 44, F_SM, 0x8B90A0);
  lv_obj_set_width(lv_obj_get_child(bbScr, -1), 128);
  lv_obj_set_style_text_align(lv_obj_get_child(bbScr, -1), LV_TEXT_ALIGN_CENTER, 0);
  bbStat = uiLabel(bbScr, "desligado", 0, 60, F_SM, 0xF4F5F7);
  lv_obj_set_width(bbStat, 128); lv_obj_set_style_text_align(bbStat, LV_TEXT_ALIGN_CENTER, 0);
  bbBtnOn = uiBtn(bbScr, "Ligar", bbOnCb, 122, 22, 0x1A3B25, NULL);
  if (bbBtnOn) lv_obj_set_pos(bbBtnOn, 3, 76);
  lv_obj_t *bR = uiBtn(bbScr, "Rodar /duck.txt", bbRunCb, 122, 22, 0x10304A, NULL);
  if (bR) lv_obj_set_pos(bR, 3, 100);
  lv_obj_t *bS = uiBtn(bbScr, "Parar", bbStopRunCb, 60, 22, 0x1C202B, NULL);
  if (bS) lv_obj_set_pos(bS, 3, 126);
  lv_obj_t *bB = uiBtn(bbScr, "Voltar", bbBackCb, 60, 22, 0x1C202B, NULL);
  if (bB) lv_obj_set_pos(bB, 66, 126);
  bbChanged = true;
  static lv_timer_t *tb = nullptr;
  if (tb) { lv_timer_del(tb); tb = nullptr; }
  tb = lv_timer_create(bbTick, 300, NULL);
  nav(bbScr, LV_SCR_LOAD_ANIM_MOVE_LEFT);
}
#endif

// ============================================================
//  ESPELHO DO LCD (WiFi): serve o framebuffer cru 128x160 RGB565
//  na porta 81 (AP "WearMirror"). O LVGL continua rodando (nao congela).
//  A pagina HTML decodifica RGB565 -> canvas (pixel identico ao TFT).
// ============================================================
static WebServer mirrorWeb(81);
static bool gMirrorOn = false;
static void mirrorRoot() {
  mirrorWeb.send(200, "text/html",
    "<html><head><meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<style>body{margin:0;background:#05070c;display:flex;flex-direction:column;align-items:center;font:13px monospace;color:#9ab}"
    "canvas{image-rendering:pixelated;border:8px solid #1a2030;border-radius:14px;margin-top:16px;background:#000}"
    "#h{color:#2FA8FF}</style></head><body>"
    "<div id='h'>WearMirror &middot; espelho do LCD (128x160)</div>"
    "<canvas id='c' width='128' height='160' style='width:384px;height:480px'></canvas>"
    "<div id='s'>conectando...</div>"
    "<script>"
    "var cv=document.getElementById('c'),cx=cv.getContext('2d');"
    "var im=cx.createImageData(128,160),d=im.data;"
    "function draw(buf){var u=new Uint8Array(buf);for(var i=0;i<128*160;i++){var lo=u[i*2],hi=u[i*2+1],p=lo|(hi<<8);"
    "d[i*4]=(((p>>11)&31)*255/31)|0; d[i*4+1]=(((p>>5)&63)*255/63)|0; d[i*4+2]=((p&31)*255/31)|0; d[i*4+3]=255;}"
    "cx.putImageData(im,0,0);}"
    "async function loopF(){try{var r=await fetch('/s?t='+Date.now());var b=await r.arrayBuffer();draw(b);"
    "document.getElementById('s').textContent='ok '+new Date().toLocaleTimeString();}"
    "catch(e){document.getElementById('s').textContent='erro: '+e;}setTimeout(loopF,400);} loopF();"
    "</script></body></html>");
}
static void mirrorScreen() {
  if (!gFrame) { mirrorWeb.send(500, "text/plain", "no frame"); return; }
  mirrorWeb.send_P(200, "application/octet-stream", (PGM_P)gFrame, 128 * 160 * 2);
}
bool mirrorOn() { return gMirrorOn; }
void uiMirrorStart() {
  if (gMirrorOn) return;
  WiFi.disconnect(true);
  WiFi.mode(WIFI_AP_STA);                        // mantem stack mas sobe o AP
  bool ok = WiFi.softAP("WearMirror", "12345678", 1, false, 4);   // canal 1, visivel, 4 clientes
  delay(100);
  mirrorWeb.on("/", HTTP_GET, mirrorRoot);
  mirrorWeb.on("/s", HTTP_GET, mirrorScreen);
  mirrorWeb.begin();
  gMirrorOn = true;
  IPAddress ip = WiFi.softAPIP();
  Serial.printf("[mirror] softAP %s ip=%s\r\n", ok ? "OK" : "FALHOU", ip.toString().c_str());
  char b[40];
  if (ok) snprintf(b, sizeof(b), "AP WearMirror - http://%s", ip.toString().c_str());
  else    snprintf(b, sizeof(b), "falha ao criar AP");
  toast(b);
}
void uiMirrorStop() {
  if (!gMirrorOn) return;
  mirrorWeb.stop();
  gMirrorOn = false;
  WiFi.mode(WIFI_STA);
  if (cfg.autoWifi && wifiN) WiFi.begin(wifiSsid[0].c_str(), wifiPass[0].c_str());
}
void mirrorTick() { if (gMirrorOn) mirrorWeb.handleClient(); }

//  MODO SERVIDOR (upload via WiFi AP; LVGL congelado, TFT direto)
//  Timeout automatico de 5 min se ninguem conectar; botao FECHAR no cantao inferior.
// ============================================================
bool gServOn = false;
uint32_t gServT0 = 0;
static const int SERV_X = 30, SERV_Y = 138, SERV_W = 68, SERV_H = 18;
// ---- paleta da tela do modo servidor (mesma do relogio: preto + cards + laranja) ----
#define SRV_BG   0x0000
#define SRV_CARD 0x161A22
#define SRV_EDGE 0x262B36
#define SRV_ACC  0xFF7A00
#define SRV_TXT  0xF4F5F7
#define SRV_DIM  0x8B90A0
#define SRV_OK   0x3AD07F
#define SRV_ERR  0xFF5C5C
// geometria: cabecalho, card e barra (compartilhada com webServBar no .ino)
#define SRV_BAR_X 14
#define SRV_BAR_Y 98
#define SRV_BAR_W 100
#define SRV_BAR_H 8
#define SRV_ST_Y  110
bool serverActive() { return gServOn; }
static bool servCloseCheck() {
  int x, y, px, py;
  if (!tsRaw(&x, &y)) return false;
  if (!tsPoint(&px, &py)) return false;
  if (!inRect(px, py, SERV_X, SERV_Y, SERV_W, SERV_H)) return false;
  // aguarda soltar o dedo p/ nao re-disparar no mesmo toque
  while (tsRaw(&x, &y)) delay(10);
  return true;
}
bool serverCloseTap() {
  if (!gServOn) return false;
  // timeout de 5 min fecha sozinho (nao deixa o usuario preso)
  if (millis() - gServT0 > 300000UL) return true;
  return servCloseCheck();
}
// ---------------- TELA DO MODO SERVIDOR (TFT cru, sem LVGL) ----------------
// Visual igual ao resto do relogio: fundo preto, card arredondado, destaque
// laranja, badge de modo e botao FECHAR no rodape.
void uiServerScreen() {
  tft.fillScreen(SRV_BG);
  tft.setTextSize(1);
  // cabecalho: ponto laranja + nome + badge AP/WIFI
  tft.fillCircle(10, 10, 4, SRV_ACC);
  tft.setTextColor(SRV_TXT, SRV_BG);
  tft.setCursor(20, 5);
  tft.print("WearSend");
  const char *bd = gSrvAp ? "AP" : "WIFI";
  int bw = gSrvAp ? 20 : 30;
  tft.fillRoundRect(122 - bw, 4, bw, 13, 6, SRV_CARD);
  tft.drawRoundRect(122 - bw, 4, bw, 13, 6, SRV_ACC);
  tft.setTextColor(SRV_ACC, SRV_BG);
  tft.setCursor(122 - bw + (gSrvAp ? 6 : 6), 6);
  tft.print(bd);

  // card principal
  tft.fillRoundRect(6, 22, 116, 100, 9, SRV_CARD);
  tft.drawRoundRect(6, 22, 116, 100, 9, SRV_EDGE);

  // linha 1: rede
  tft.setTextColor(SRV_DIM, SRV_CARD);
  tft.setCursor(14, 30);
  tft.print("REDE");
  tft.setTextColor(SRV_TXT, SRV_CARD);
  tft.setCursor(44, 30);
  tft.print(gSrvNet.length() ? gSrvNet.substring(0, 12).c_str() : "-");

  // linha 2: senha (so' no AP)
  if (gSrvAp) {
    tft.setTextColor(SRV_DIM, SRV_CARD);
    tft.setCursor(14, 46);
    tft.print("SENHA");
    tft.setTextColor(SRV_TXT, SRV_CARD);
    tft.setCursor(56, 46);
    tft.print("12345678");
  }

  // separador
  tft.drawFastHLine(14, gSrvAp ? 60 : 48, 100, SRV_EDGE);

  // endereco para abrir no navegador
  int yl = gSrvAp ? 68 : 56;
  tft.setTextColor(SRV_DIM, SRV_CARD);
  tft.setCursor(14, yl);
  tft.print("ABRA NO NAVEGADOR");
  tft.setTextColor(SRV_DIM, SRV_CARD);
  tft.setCursor(14, yl + 14);
  tft.print("http://");
  tft.setTextColor(SRV_ACC, SRV_CARD);
  tft.setCursor(14 + 36, yl + 14);
  tft.print(gSrvIp.length() ? gSrvIp.c_str() : "-");

  // rodape: estado do SD + botao FECHAR
  bool sdm = sdMountedOk();
  tft.setTextColor(sdm ? SRV_OK : SRV_ERR, SRV_BG);
  tft.setCursor(4, 128);
  tft.print(sdm ? "SD OK" : "SD ERRO");
  tft.fillRoundRect(SERV_X, SERV_Y, SERV_W, SERV_H, 9, SRV_CARD);
  tft.drawRoundRect(SERV_X, SERV_Y, SERV_W, SERV_H, 9, SRV_ACC);
  tft.setTextColor(SRV_ACC, SRV_CARD);
  tft.setCursor(SERV_X + 16, SERV_Y + 6);
  tft.print("FECHAR");
}

void uiServerStart() {
  if (gServOn) return;
  gLvPause = true;
  audStop();                          // o player segura o SD: para antes de montar
  for (int i = 0; i < 40 && audLoadBusy; i++) vTaskDelay(pdMS_TO_TICKS(50));  // espera a leitura terminar (ate 2s)
  if (gKb && lv_obj_is_valid(gKb)) lv_obj_add_flag(gKb, LV_OBJ_FLAG_HIDDEN);
  // feedback antes de montar/conectar (a espera pelo wifi pode levar 12 s)
  tft.fillScreen(C_BG);
  tft.setTextSize(1);
  tft.setTextColor(C_ACC, C_BG);
  tft.setCursor(14, 74); tft.print("conectando...");
  bool ok = webSdStart();
  tft.fillScreen(C_BG);
  if (!ok) {
    tft.setTextSize(1); tft.setTextColor(C_FG, C_BG);
    tft.setCursor(6, 70); tft.print("Falha no SD!");
    delay(1200);
    gLvPause = false;
    lv_obj_invalidate(lv_scr_act());
    lv_refr_now(gDisp);
    return;
  }
  gServOn = true;
  gServT0 = millis();
  blOn();
  uiServerScreen();                  // tela nova (card + badge + FECHAR)
}
void uiServerStop() {
  if (!gServOn) return;
  webSdStop();
  // sem SD.end(): o cartao continua montado p/ o explorador/player
  if (gSrvAp) {                         // desliga o AP e volta so' para o wifi salvo
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(150);
  }
  WiFi.mode(WIFI_STA);
  if (cfg.autoWifi && wifiN) WiFi.begin(wifiSsid[0].c_str(), wifiPass[0].c_str());
  gServOn = false;
  gLvPause = false;
  lv_obj_invalidate(lv_scr_act());
  lv_refr_now(gDisp);
  gLastTouch = millis();
  blOn();
}

// ============================================================
//  POLL (despachante geral 150 ms)
// ============================================================
static bool helArm = false;
// clima em 2o plano (a cada 5 min, enquanto conectado)
// aplica os dados do clima (temp|desc|vento|umidade|sensacao) em todas as telas
static void wxApplyData(const String &out) {
  if (out.indexOf('|') < 0) { gWxTxt = ""; gWxTemp = 0; return; }
  String t = wxField(out, 0), d = wxField(out, 1);
  gWxTxt = t + "° " + d;
  gWxTemp = t.toInt();
  gWxDesc = d;
  gWxKind = (d.indexOf("chuva") >= 0 || d.indexOf("rain") >= 0 || d.indexOf("garoa") >= 0) ? 2
          : (d.indexOf("nublado") >= 0 || d.indexOf("cloud") >= 0 || d.indexOf("cinza") >= 0) ? 1 : 0;
  if (gWxDesc.length()) {                 // "ensolarado" -> "Ensolarado"
    String cap = gWxDesc.substring(0, 1);
    cap.toUpperCase();
    gWxDesc = cap + gWxDesc.substring(1);
    if (gWxDesc.length() > 11) gWxDesc = gWxDesc.substring(0, 11);
  }
}
static void wxBgCb(bool, const String &out) {
  wxApplyData(out);                        // lock/topo-esq: temperatura + descricao
}
static void ntpAutoCb(bool ok, const String &out) {
  if (!ok) return;                         // sem toast intrusivo: tenta na proxima conexao
  clockPersist();
  if (lv_obj_is_valid(scrLock)) lockUpdate();
  Serial.printf("[ntp] sincronizado (%s)\r\n", out.c_str());
}
static void pollTimer(lv_timer_t *) {
  // limpeza de PSRAM: libera o que estiver ocioso ha mais de 1min30s
  // (o que esta aberto nunca e tocado)
  static uint32_t tPs = 0;
  if (millis() - tPs > PS_SWEEP_MS) { tPs = millis(); psCleanup(false); }
  // jogos: libera o framebuffer ~300 ms depois da tela ser destruida
  {
    uint32_t now = millis();
    for (uint8_t r = 0; r < PSR_COUNT; r++) {
      if (!psFreeAt[r]) continue;
      if ((int32_t)(now - psFreeAt[r]) < 0) continue;   // ainda nao chegou
      psFreeAt[r] = 0;
      psFreeRes(r);
      if (psAllocated[r] && !psInUse[r]) psFreeAt[r] = now + 500;  // ocupado ainda: retenta
    }
  }
  // retorno automatico ao relogio (loop pede apos 2 min sem toque)
  if (gWantLock && !gOnLock) {
    gWantLock = false;
    lockUpdate();
    navP(scrLock, LV_SCR_LOAD_ANIM_OVER_TOP, true);
  }
  // lanterna nao sobrevive a tela travada (economia + nao cobrir o relogio)
  if (gOnLock && ltOv) ltCloseCb(nullptr);
  // cronometro rodando
  if (cronRun) cronUpdate();
  // aprender controle IR: decodifica e salva na hora
  if (irLearnArm && (gIrNew || irRawNew)) {
    // gIrNew = quadro NEC decodificado | irRawNew = tempos brutos capturados
    // prioridade: NEC valido tem precedencia sobre raw
    bool isNec = gIrNew && gIrCode != 0;
    gIrNew = irRawNew = false;
    irLearnArm = false;
    if (rN >= 12) { toast("memoria cheia (12 botoes)"); irRawReset(); }
    else if (isNec) {                                   // NEC valido: grava addr/cmd
      uint32_t f = gIrCode;
      uint8_t a = f & 0xFF, c = (f >> 16) & 0xFF;
      bool ok = (((f >> 8) & 0xFF) == ((~a) & 0xFF)) && (((f >> 24) & 0xFF) == ((~c) & 0xFF));
      if (!ok) toast("codigo invalido");
      else {
        char nm[10], b[36];
        snprintf(nm, sizeof(nm), "BT%d", rN + 1);
        rName[rN] = nm; rAddr[rN] = a; rCmd[rN] = c; rIsRaw[rN] = false; rRawN[rN] = 0;
        if (lv_scr_act() == scrRemote) remoteRow(scrRemote, rN);
        rN++;
        rSave();
        snprintf(b, sizeof(b), "aprendido %s %02X %02X", nm, a, c);
        toast(b);
      }
    } else {                                            // protocolo desconhecido: grava os tempos
      noInterrupts();
      uint16_t n = irRawN > 24 ? 24 : irRawN;
      for (int k = 0; k < n; k++) rRaw[rN][k] = (uint16_t)irRaw[k];
      interrupts();
      if (n < 8) { toast("sinal muito curto"); irRawReset(); }
      else {
        char nm[10], b[44];
        snprintf(nm, sizeof(nm), "BT%d", rN + 1);
        rName[rN] = nm; rIsRaw[rN] = true; rRawN[rN] = n;
        rAddr[rN] = gIrCode & 0xFF; rCmd[rN] = (gIrCode >> 16) & 0xFF;
        if (lv_scr_act() == scrRemote) remoteRow(scrRemote, rN);
        rN++;
        rSave();
        snprintf(b, sizeof(b), "aprendido %s (raw %d tempos)", nm, n);
        toast(b);
        irRawReset();
      }
    }
  }
  // aprendizado: timeout de 8s sem sinal
  if (irLearnArm && millis() - irLearnT0 > 8000) {
    irLearnArm = false;
    irRawReset();
    toast("tempo esgotado - tente de novo");
  }
  // (unlock/swipe ficam em touchTick(), chamado do loop() - raw, nao depende
  //  do processamento de eventos do LVGL)
  // clima em 2o plano (5 em 5 min; nao rouba callback da tela do Clima)
  static uint32_t tWx = 0;
  if (WiFi.status() == WL_CONNECTED && !gNetBusy &&
      lv_scr_act() != scrClima && millis() - tWx > 300000) {
    tWx = millis();
    gNetCb = wxBgCb;
    netRun(T_WX);
  }
  // resultado de rede chegou?
  if (gNetDone && gNetCb) {
    void (*cb)(bool, const String&) = gNetCb;
    gNetDone = false;
    gNetCb = nullptr;
    cb(gNetRes, gNetOut);
  }
  // Auto-NTP: primeira conexao Wi-Fi (ou reconexao) sincroniza o horario sem travar a UI.
  if (gNtpAutoPending && !gNetBusy && !gNetCb) {
    gNtpAutoPending = false;
    gNetCb = ntpAutoCb;
    if (!netRun(T_NTP)) { gNtpAutoPending = true; gNetCb = nullptr; }
  }
  // relogio da tela de bloqueio
  static uint32_t tLock = 0;
  if (millis() - tLock > 1000) {
    tLock = millis();
    lockUpdate();
  }
  // contagem de passos (global)
  static uint32_t tStep = 0, tStepsSave = 0;
  static long lastSteps = -1;
  if (gMpuOk && millis() - tStep > 400) {
    tStep = millis();
    if (mpuStepEdge(helArm)) cfg.steps++;
  }
  if (lastSteps != (long)cfg.steps && millis() - tStepsSave > 60000) {
    tStepsSave = millis();
    lastSteps = cfg.steps;
    char b[12];
    cfgPut("steps", ltoa(cfg.steps, b, 10));
  }
  // saude: 3 aneis + legendas
  static uint32_t tHel = 0;
  if (lv_scr_act() == scrSaude && lv_obj_is_valid(helArcP) && millis() - tHel > 400) {
    tHel = millis();
    long s = cfg.steps;
    if (lv_obj_is_valid(helArcP)) lv_arc_set_value(helArcP, (int16_t)constrain(s, 0, 10000));
    if (lv_obj_is_valid(helArcK)) lv_arc_set_value(helArcK, (int16_t)constrain((long)(s * 0.0007f * 100), 0, 700));
    if (lv_obj_is_valid(helArcC)) lv_arc_set_value(helArcC, (int16_t)constrain((long)(s * 0.04f), 0, 400));
    char b[12]; if (!gMpuOk) snprintf(b, sizeof(b), "sem MPU"); else snprintf(b, sizeof(b), "%lu", (unsigned long)s);
    if (lv_obj_is_valid(helSteps)) lv_label_set_text(helSteps, b);
    char kb[16]; snprintf(kb, sizeof(kb), "%.2fkm", s * 0.0007f);
    if (lv_obj_is_valid(helK)) lv_label_set_text(helK, kb);
    char sb2[12]; snprintf(sb2, sizeof(sb2), "%ld", s);
    if (lv_obj_is_valid(helS)) lv_label_set_text(helS, sb2);
    char cb2[12]; snprintf(cb2, sizeof(cb2), "%ldcal", (long)(s * 0.04f));
    if (lv_obj_is_valid(helC)) lv_label_set_text(helC, cb2);
  }
  // nivel/bolha
  static uint32_t tLvl = 0;
  if (lv_scr_act() == scrLevel && gMpuOk && lv_obj_is_valid(lvlBall) &&
      millis() - tLvl > 70) {
    tLvl = millis();
    int16_t _ax, _ay, _az;
    if (mpuReadAcc(&_ax, &_ay, &_az)) {
      float gx = _ax / 16384.0f, gy = _ay / 16384.0f, gz = _az / 16384.0f;
      int dx, dy;
      if (lvlBolha) { dx = -gx * 30.0f; dy = -gy * 30.0f; }
      else {
        float pitch = atan2f(gy, gz) * 57.2958f;
        float roll  = atan2f(gx, gz) * 57.2958f;
        dx = -pitch * 30.0f / 45.0f;
        dy = -roll  * 30.0f / 45.0f;
        char b[40];
        snprintf(b, sizeof(b), "pitch %d   roll %d", (int)pitch, (int)roll);
        if (lv_obj_is_valid(lvlNote)) lv_label_set_text(lvlNote, b);
      }
      dx = constrain(dx, -30, 30);
      dy = constrain(dy, -30, 30);
      lv_obj_align(lvlBall, LV_ALIGN_CENTER, dx, dy);
      bool leveled = abs(dx) < 3 && abs(dy) < 3;
      lv_obj_set_style_bg_color(lvlBall, leveled ? REDMI_OK : REDMI_ACC, 0);   // verde = nivelado
      if (lvlBolha && lv_obj_is_valid(lvlNote)) {
        char b[24];
        snprintf(b, sizeof(b), "p %d   r %d", (int)-dx, (int)-dy);
        lv_label_set_text(lvlNote, b);
      }
    }
  }
  // testar toque: ponto segue o dedo
  static uint32_t tTT = 0;
  if (lv_scr_act() == scrTelaTest && lv_obj_is_valid(ttDot) && millis() - tTT > 40) {
    tTT = millis();
    int x, y;
    if (tsPoint(&x, &y)) {
      lv_obj_clear_flag(ttDot, LV_OBJ_FLAG_HIDDEN);
      lv_obj_set_pos(ttDot, constrain(x - 10, 0, 108), constrain(y - 10, 0, 138));
      char b[28];
      snprintf(b, sizeof(b), "x:%3d   y:%3d     ", x, y);
      if (lv_obj_is_valid(ttPos)) lv_label_set_text(ttPos, b);
    } else {
      lv_obj_add_flag(ttDot, LV_OBJ_FLAG_HIDDEN);
      if (lv_obj_is_valid(ttPos)) lv_label_set_text(ttPos, "toque na tela");
    }
  }
  // status do wifi
  static uint32_t tWst = 0;
  if (lv_scr_act() == scrWifi && lv_obj_is_valid(wifiSt) && millis() - tWst > 2000) {
    tWst = millis();
    wifiStatusText();
  }
}

// ============================================================
//  TICK DE TOQUE CRUO (chamado do Arduino loop, a cada ~4ms)
//  Nao depende do processamento de eventos do LVGL:
//  - lock: qualquer toque/arrasto destrava (o usuario pediu por toque)
//  - app:  arrasto de >=40px volta pra home (swipe-back)
// ============================================================
// ============================================================
//  TICK DE TOQUE CRUO (chamado do Arduino loop, a cada ~4ms)
//  MAQUINA DE ESTADO com filtro de estabilidade (40ms):
//  - eliminino "press fantasma"/ruido do IRQ/o barramento, que fazia
//    exigir 2 toques rapidos e nao dormir (down travado)
//  - acao dispara NA CONFIRMCAO DO SOLTAR (sem janela de timing)
//  lock: qualquer toque destrava | app: arrasto >=40px volta p/ home
// ============================================================
void touchTick() {
  static bool solid = false;            // estado CONFIRMADO do dedo
  static bool pend = false;             // transicao aguardando 40ms estaveis
  static bool pendVal = false;
  static uint32_t pendMs = 0;
  static int ox = -1, oy = -1;          // origem (x,y) do toque confirmado
  static bool pLock = false;            // contexto NO PRESS (onde o toque comecou)
  int x, y;
  // Sessao do SD em andamento: nao le o touch (ele divide o MISO com o
  // cartao). Mantem o estado para nao perder o toque em andamento.
  if (gSdPause) return;
  bool r = tsPoint(&x, &y);
  uint32_t now = millis();
  if (r != solid) {
    if (!pend) { pend = true; pendVal = r; pendMs = now; }
    else if (r != pendVal) pend = false;     // ida-e-volta em <40ms = ruido
  } else pend = false;
  if (pend && now - pendMs >= 40 && r == pendVal) {
    solid = r; pend = false;
    if (solid) {                              // toque NOVO confirmado
      ox = x; oy = y; gTx = x; gTy = y;
      pLock = gOnLock;                        // registra ONDE o toque comecou
      gLastTouch = now;
      if (gWakeWait) {                        // tela apagada: conta toques da janela
        if (gWakeTaps == 0) gWakeWin = now;   // janela comeca no 1o toque
        gWakeTaps++;
        if (gWakeTaps >= 2) wakeFinish();     // 2 toques em ate 2s -> acende
        return;                               // consome: sem hint/unlock/swipe
      }
      if (gWakeArm) wakeHint(true);           // 1o toque: so acende a tela
    } else {                                  // SOLTAR confirmado -> acao
      if (gWakeWait) return;                  // nada acontece ate os 2 toques
      if (gWakeArm) {                         // consome o toque (nao destrava)
        gWakeArm = false;
        wakeHint(false);
      } else if (pLock && gOnLock) {
        // girar um anel NAO destrava; toque em qualquer outro lugar sim
        if (gRingActive) gRingActive = false;
        else if (millis() - gRingTouchedAt > 450) lockUnlock();
      } else if (!pLock && !gOnLock &&
                 !gKbVis() && !gInGame &&
                 abs(gTx - ox) > 40 && abs(gTx - ox) > abs(gTy - oy)) {
        navP(scrHome, LV_SCR_LOAD_ANIM_MOVE_RIGHT, true);   // app: swipe-back
      }
    }
  }
  if (r) { gTx = x; gTy = y; gLastTouch = now; }
}

// ============================================================
//  INIT
// ============================================================
void uiInit() {
  lv_init();
  static lv_disp_draw_buf_t dbuf;
  static lv_color_t db[128 * 160];   // framebuffer completo (rotacao por software)
  gFrame = (uint16_t *)db;            // acesso p/ o espelho
  lv_disp_draw_buf_init(&dbuf, db, NULL, 128 * 160);
  lv_disp_drv_init(&gDrv);
  gDrv.hor_res = 128;
  gDrv.ver_res = 160;
  gDrv.draw_buf = &dbuf;
  gDrv.flush_cb = disp_flush;
  gDrv.full_refresh = 1;
  gDisp = lv_disp_drv_register(&gDrv);
  lv_theme_default_init(gDisp, lv_palette_main(LV_PALETTE_BLUE),
                        lv_palette_main(LV_PALETTE_RED), true, F_UI);
  static lv_indev_drv_t idrv;
  lv_indev_drv_init(&idrv);
  idrv.type = LV_INDEV_TYPE_POINTER;
  idrv.read_cb = touch_read;
  lv_indev_drv_register(&idrv);
  buildLockS();
  buildHomeS();
  kbBuild();               // teclado custom global (aparece ao focar um campo)
  lv_scr_load(scrLock);
  // (animacao de boot removida a pedido do usuario)
  lv_timer_create(statusTimer, 1000, NULL);
  lv_timer_create(pollTimer, 150, NULL);
}
// ---------------- icones dos jogos (emoji/glifo) ----------------
static const char *jogoIcone(const char *n) {
  if (!strcmp(n, "PONG"))      return "|>";
  if (!strcmp(n, "INVASORES")) return "*";
  if (!strcmp(n, "FLAPPY"))    return "^";
  if (!strcmp(n, "DINO"))      return "@";
  if (!strcmp(n, "TETRIS"))    return "#";
  if (!strcmp(n, "DAMA"))      return "O";
  if (!strcmp(n, "XADREZ"))    return "+";
  if (!strcmp(n, "MARIO"))     return "M";
  if (!strcmp(n, "SONIC"))     return "S";
  if (!strcmp(n, "DOOM"))      return "!";
  if (!strcmp(n, "DADO"))      return ".";
  if (!strcmp(n, "MOEDA"))     return "$";
  if (!strcmp(n, "SNAKE"))     return "~";
  return "?";
}

