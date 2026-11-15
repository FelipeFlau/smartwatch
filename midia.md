# Mídia no relógio: imagem (JPEG) e vídeo (MJPEG)

Referências pesquisadas (2026-11-11):

- **ESP32_JPEG** (Espressif, `esp-arduino-libs/ESP32_JPEG`) — decoder JPEG em
  software, otimizado para ESP32/ESP32-S3. Exemplo `DecodeTest.ino`:
  `jpeg_dec_open` → `jpeg_dec_parse_header` → `jpeg_dec_process`.
- **vPlayer** (`vplayer.readthedocs.io`) — relógio ESP32-S3 240×280 que toca
  vídeo do SD. O formato é **MJPEG** (frames JPEG em sequência), convertido
  no PC com ffmpeg. Não existe decoder de vídeo em hardware no S3: todo
  player de vídeo do ESP32 é MJPEG (ou AVI/MJPEG).

## 1) Por que ESP32_JPEG e não JPEGDEC

O firmware hoje usa **JPEGDEC** (bitbank2) em três lugares: miniaturas da aba
Imagens (`ui.h` ~944), wallpaper (`wpDecodeJpgRam`, ~2013) e a foto aberta.
O ESP32_JPEG é da própria Espressif, bem mais rápido no S3, e resolve dois
pontos que hoje são limitados:

| | JPEGDEC | ESP32_JPEG |
|---|---|---|
| escala | `JPEG_SCALE_EIGHTH` (1/8) | `config.scale` (1/1, 1/2, 1/4, 1/8) |
| rotação | manual | `config.rotate` (0/90/180/270) |
| saída | RGB565 (byte-swap manual) | `JPEG_RAW_TYPE_RGB565_BE` (já no formato do TFT) |
| buffer de entrada | livre | **precisa de 16 bytes de alinhamento** |

Para a tela de 128×160:

```cpp
jpeg_dec_config_t cfg = { .output_type = JPEG_RAW_TYPE_RGB565_BE,
                          .rotate = JPEG_ROTATE_0D,
                          .scale  = JPEG_SCALE_1_4 };   // se disponível na versão
```

- Saída direta em `tft.writePixels((uint16_t*)buf, 128*160)` — `RGB565_BE` é
  exatamente o que o Adafruit_ST7735 manda (byte alto primeiro;
  `LV_COLOR_16_SWAP 0`).
- Buffers: `heap_caps_aligned_alloc(16, len, MALLOC_CAP_SPIRAM)` para o JPEG
  bruto e `128*160*2 = 40.960 B` para o frame. Cabe folgado na PSRAM.
- Ganho esperado: decode 128×160 na casa de **5–15 ms** (S3 @240 MHz) contra
  dezenas de ms do JPEGDEC em imagem grande.

## 2) Vídeo: MJPEG em `/VIDEOS`

O vPlayer usa `.mjpeg` gerado por ffmpeg (comando dele comentado abaixo) e
toca a partir do SD **em MMC 4 bits**. O nosso SD está em **SPI**, então a
taxa de leitura é o teto real do fps:

| SPI | ~KB/s reais | frames/s com frame de 6 KB |
|---|---|---|
| 400 kHz (hoje) | ~45 | ~7 |
| 1 MHz | ~110 | ~18 |
| 4 MHz | ~400 | limitado pelo decode/PSRAM |

Ou seja: **vídeo exige o SD acima de 1 MHz** — mais um motivo para resolver a
escrita/velocidade do cartão.

Comando adaptado para 128×160 (retrato) @ 12 fps:

```bash
ffmpeg -i entrada.mp4 -vf "fps=12,scale=-2:128:flags=lanczos,crop=160:128:(iw-160)/2:0,transpose=1" \
       -pix_fmt yuvj420p -q:v 6 -vcodec mjpeg \
       -f segment -segment_time 1800 -reset_timestamps 1 "VIDEO_%03d.mjpeg"
```

- `scale=-2:128` + `crop=160:128` = paisagem 160×128; `transpose=1` gira para
  128×160 (retrato da tela).
- `-q:v 6` deixa o frame em ~4–8 KB. `-q:v 4` melhora a imagem e dobra o
  tamanho.
- `-f segment` quebra em arquivos de 30 min (o player toca em sequência).

## 3) Player (proposta de implementação, ainda não feita)

1. `SD.open("/VIDEOS/x.mjpeg")`, ler em blocos de 512 B para um buffer PSRAM.
2. Achar `FFD8` (SOI) e `FFD9` (EOI) para delimitar cada frame (MJPEG não tem
   índice; é varredura de marcadores).
3. Decodificar com ESP32_JPEG direto para o buffer RGB565 e
   `tft.writePixels`. Sem LVGL no meio: é mais rápido e não aloca objeto.
4. Frame timing por `millis()`; se o decode+leitura passar do período, pula
   frames em vez de acumular atraso.
5. Áudio: hoje o player só toca WAV/PCM do `/MUSICAS`; MJPEG não tem áudio.
   Se quiser vídeo com som, o caminho é AVI (MJPEG + PCM) e sincronizar pelo
   relógio — bem mais complexo.

## 4) Instalação

ESP32_JPEG entra como biblioteca (`lib_deps = esp-arduino-libs/ESP32_JPEG`)
ou copiando `src/` para `Documents/Arduino/libraries/ESP32_JPEG/`. O include é:

```cpp
#include <ESP32_JPEG_Library.h>
```

Na Arduino IDE (sem PlatformIO) o caminho mais simples é baixar o `.zip` do
repositório e usar **Sketch → Incluir Biblioteca → Adicionar .ZIP**.
