# Smartwatch v2.3 — ESP32-S3 **Super Mini** (4MB/2MB PSRAM QSPI) + ST7735 1.8" + XPT2046

Interface em **LVGL 8.4**. Compilado para a placa **ESP32-S3 Super Mini (N4R2)**.

> 📁 **Pasta do projeto:** `C:\Users\Flausino\Documents\Smartwatch`
> (o `.ino` precisa ter o mesmo nome da pasta: `Smartwatch.ino`).
> Cópia antiga mantida em `Documents\Projeto Padrão\smartwatch` — pode apagar quando quiser.

## Simulador (estilo Wokwi) — `simulador.html`
Abra **`simulador.html`** com dois cliques (funciona offline, sem servidor).

- **Esquema elétrico** com a arte real da **ESP32-S3 Super Mini** (`assets/esp32-s3-super-mini.png`, espboards.dev), os 7 módulos (Display ST7735, Touch XPT2046, Cartão SD, MPU-6050, IR, Speaker, Bateria) e a fiação **igual ao `core.h`**:
  - barramentos **+3V3 / GND** no topo e **SPI (SCK=9 · MOSI=10 · MISO=34)** embaixo, com os taps de TFT / Touch / SD — o bug do barramento compartilhado fica visível;
  - **pads traseiros** (GPIO 34–39) mostrados à parte, como na placa real;
  - cada fio sai do pino certo e chega no pino certo (validado por geometria).
- **A tela 128×160 fica dentro do módulo Display**, 1:1 (pixels exatos do firmware) e é **interativa**: toque fora dos anéis destrava, arrastar os 3 anéis (lime passos / verde km / rosa calorias) muda os valores, os chips da Home abrem os apps.
- **Painel por módulo** que altera a simulação: face do relógio, brilho, rotação 180°, touch on/off + calibração, cartão SD presente + lista de arquivos, MPU inclinação X/Y + "caminhar" (conta passos), IR (envia NEC + LED pisca), speaker (bip real via WebAudio), carga da bateria, Wi-Fi/BLE.
- **Console serial** com o log do firmware (boot, SPI, uploads, IR, passos).
- Telas implementadas: bloqueio (3 faces), Home, Config, Saúde, Personalizar, Nível, Arquivos, Jogos, Clima, Crono, Toolbox, Wi-Fi, Data/Hora, Ajustes, Toque, Info, Reset.

> A placa em `assets/` vem do espboards.dev (CC BY-NC 4.0, atribuição no próprio esquema).

## Como gravar
- Placa: **ESP32S3 Dev Module** com:
  - FlashSize: **4MB** / PSRAM: **QSPI** (⚠️ NÃO use OPI — a R2 da Super Mini tem PSRAM QSPI, OPI quebra o boot) / PartitionScheme: **Huge APP (3MB No OTA/1MB SPIFFS)**
  - USBMode: Hardware CDC and JTAG / CDCOnBoot: Enabled / FlashMode: QIO
- Ou pela CLI:
  ```
  arduino-cli compile --fqbn "esp32:esp32:esp32s3:FlashSize=4M,PSRAM=enabled,PartitionScheme=huge_app,USBMode=hwcdc,CDCOnBoot=cdc,FlashMode=qio" "C:\Users\Flausino\Documents\Smartwatch"
  ```

## Pinagem final (core.h)
| Função | Pinos |
|---|---|
| TFT | SCK=9, MOSI=10, MISO=34, CS=13, DC=11, RST=12, BL=8 |
| Touch | CS=36, IRQ=35 (CLK/DIN/DO **compartilham** SCK/MOSI/MISO do TFT) |
| SD | SCK=9, MISO=34, MOSI=10, CS=39 (mesmo barramento SPI do TFT, CS separado) |
| MPU6050 | SDA=38, SCL=37, SPK=5, BAT_ADC=1 |
| IR | TX=3, RX=2. Fiação do LED IR: **3V3 → ânodo → resistor → GPIO3 (catodo)**; LOW = LED aceso |

- GPIO33–37 **existem** nesta variante (PSRAM QSPI, não octal) e são seguras.
- Display: offset +1 linha / +2 colunas aplicado em `ST7735of::offsetCR(2,1)`.
- Rotação 180° nativa pelo MADCTL do controlador (sem `sw_rotate`).
- SD e TFT dividem o barramento SPI (SCK/MOSI/MISO); o CS do SD (GPIO39) fica em HIGH no repouso para evitar ruído no barramento compartilhado.

## Primeiro boot
1. Se o `/cal.txt` antigo existir (formato velho), ele é **ignorado**: a calibração roda de novo (4 cantos + tela de verificação).
2. O app **TELA > Testar toque** mostra o ponto seguindo o dedo + coordenadas; se desviar, use TELA > Recalibrar toque.

## O que tem hoje
- **UI LVGL (estilo Redmi Watch 6 / HyperOS)**: fundo preto, tipografia Montserrat (12/14/20/28/32/48), cards arredondados, cabeçalho de app uniforme (voltar + título + barra de status) e toasts.
- **Home pixel a pixel**: a tela é uma **arte RGB565 extraída do print** (`home_art.h`, gerado por `gerar_art.py`) desenhada em `(0,18)`; por cima dela só o cabeçalho dinâmico (hora, diamante, `%` e pílula da bateria) e **9 áreas de toque invisíveis** exatamente sobre os ícones — 1 Power, 2 Matemática, 3 Google, 4 Jogos, 5 Saúde, 6 IA, 7 Arquivos, 8 Toolbox, 9 Configurações.
- **Face padrão (a da sua foto)**: fundo preto puro, **"12" grande em cima** (F_HUGE em `30,29`) e **"59" grande embaixo à direita** (`58,79`), **3 anéis à esquerda** desenhados da arte do print (verde/azul/laranja) em `(9,85)`, diamante azul em `(76,4)`, **`98%` verde** até `x=112` (vermelho <20%) e **pílula da bateria** 7×11 em `(116,4)`. Sem temperatura, data, passos nem BT nessa face. Toque fora dos anéis destrava o relógio.
- **Outras 2 faces**: **Digital** (HH:MM grande + data + passos + clima + mini-anel de passos no canto) e **Analógica** (mostrador 88px + 3 ponteiros + anel de passos + HH:MM embaixo + data no topo). Troca em **Personalização > Estilo do relógio**.
- **Navegação**: toque na tela de bloqueio destrava (girar um anel não destrava); swipe-right volta em qualquer app; auto-lock 2 min.
- **Barra de status**: hora (esq) + 📶 Wi-Fi + **BT** + bateria % (dir) em TODAS as telas. Bateria <20% fica vermelha; **aviso automático** (toast) ao cruzar 20% e 10% (rearma acima de 30%).
  - 📶 cinza = off / **verde** = conectado; BT cinza = off / **azul** quando BLE em uso
- **Lock + clima**: linha com temperatura e condicao sob a data (busca automática a cada 5 min enquanto conectado; Open-Meteo, sem API key).
- **Config** (2 colunas): Wi-Fi, Tela, Ajustes, Data/Hr, Teste, Info, Reset, **Clima**.
- **Tela**: Calibrar, Cores (6 cores de destaque runtime), Testar, Girar 180° limpo, Personalização.
- **Personalização**: 6 accent colors (tempo real), tema **espacial** (estrelas, planeta Sol/Terra/Lua com fase da sombra), 3 estilos de relógio no lock.
- **Ajustes**: brilho (slider), tempo de sleep, Wi-Fi automático no boot, bip ao tocar.
- **Wi-Fi**: toda conexão STA sincroniza o horário automaticamente (Campo Grande/MS, **AMT UTC-4**, sem DST). Re-sincroniza a cada 6h enquanto conectado. Também há botão NTP e listar/ conectar / deletar / escanear / adicionar rede.
- **Mat**: calculadora (4×4 + barra C, fonte encolhe, ÷0 → "Er"), Nível e Bolha (MPU; bola verde quando nivelada).
- **Saúde**: passos globais (persist a cada 60s, aviso "sem MPU" quando ausente), 3 anéis concêntricos nas cores da face (passos 0–10000, km 0–7,00 km, calorias 0–400), Zerar.
- **Toolbox** (lista rolável, todas úteis): **TV-B-Gone** (banco canônico NA/EU em `tvbg_codes.h`), marcas TV/AR, Controle IR, Midia BT, BLE Scan, Lanterna, Beep, Ping, Crono/Timer, Notas, BadBLE, Espelho.
- **Enviar p/ relógio** (AP `WearSend`): separa automaticamente em **`/FOTOS`**, **`/VIDEOS`**, **`/MUSICAS`**. O navegador converte imagem para JPEG 128×160 e música para WAV 8 kHz. Nomes são convertidos para **8.3** (limitação da lib SD sem LFN). **Vídeo não é reproduzido**: o relógio ainda não tem decoder de vídeo; o arquivo fica armazenado.
- **Jogos** (13 cards 57x48, **2 por linha** — ícone em cima, nome embaixo; rolagem):
  - **PONG** (você vs CPU; arraste move a raquete, primeiro a 5)
  - **INVASORES** (Space Invaders: 3x5 aliens em onda, tiro no toque, 3 vidas)
  - **FLAPPY** (toque = asa; canos aleatórios; recorde salvo na tela)
  - **DINO** ( Chrome : toque = pular; cactos e aves; acelera a cada 5 pts)
  - **TETRIS** (7 peças reais I/O/T/L/J/S/Z, rotação por toque no topo, base desce, toque longo = queda, PRÉVIA da próxima peça; linhas e pontos)
  - **DAMA** (regras brasileiras completas: captura **obrigatória**, **multi-salto encadeado**, dama anda qualquer distância; 1P vs CPU ou 2 jogadores; tabuleiro 15px)
  - **XADREZ** (tela cheia 8x15px, peças **desenhadas em forma real** — peão/cavalo/bispo/torre/rei/rainha por `pxSet/pxDisc/pxRect`; movimentos completos, xeque e xeque-mate, roque, promoção automática, afogamento; CPU com avaliação de material)
  - **MARIO / SONIC** (motor de corrida compartilhado: obstÁculos, moedas/aneis, 3 vidas, velocidade crescente)
  - **DOOM** (raycaster 3D próprio estilo Wolf3D: framebuffer 128x96 na PSRAM, 6 inimigos-billboard com z-buffer, paredes com sombra; **arraste = andar/mirar, toque curto = atirar**; 6 abates = fase limpa)
  - **DADO** (1-6), **MOEDA** (cara/coroa, com giro animado) e **SNAKE** (classico: arraste muda a direcao, come e cresce, acelera)


## Stubs (em desenvolvimento)
- ~~Player de audio do SD~~ → **feito**: `.wav` toca pelo speaker PWM; conversão de MP3/MP4/etc → WAV acontece no navegador durante o upload (o ESP32-S3 não tem CPU/RAM pra transcodificar). **Player de vídeo** (MJPEG) ainda é stub — falta decoder de vídeo no relógio.
- ~~BLE Scan / BadBLE~~ → **feito e corrigido**: a lib BLE do core **nunca entrava no build** (`#include` dentro de `#if __has_include` não era detectado pelo scanner do arduino-cli → `HW_BLE=0`). Corrigido incluindo `<BLEDevice.h>` incondicionalmente. Agora **BLE Scan funciona** e o **BadBLE** (teclado Bluetooth HID `Wear-KB` + runner de `/duck.txt` estilo Ducky: STRING/ENTER/TAB/ESC/DELAY/DEFAULTDELAY/REM) está no Toolbox. Parser Ducky e mapa de teclas testados em Python.
- **BadUSB (USB-HID)**: NÃO implementado de propósito — na sua placa (FQBN `USBMode=hwcdc`) exige trocar pra `USB-OTG (TinyUSB)`+CDC off, o que **remove a porta serial de programação** (reflash só via BOOT+RESET ou UART0). É opt-in e arriscado; BadBLE cobre a mesma função sem esse risco.
- Jogos antigos (PAC-MAN/DOOM/MARIO/SONIC como cards placeholder) foram SUBSTITU�DOS pelos jogos reais acima
- Musica / fotos / videos do SD (listar funciona; player depois)

- **Animações / tema vivo**:
  - **pulso ao tocar** em todo botão (via `clickBeep` → `animBtn`: opa 255→225 com playback)
  - **giro da moeda** (`coinSpin`: translate_y + opa com `lv_anim_path_bounce`)
  - transições de tela por `nav()` (MOVE_LEFT/RIGHT/OVER_RIGHT) já existentes
  - **boot**: logo "SMARTWATCH" com fade-in + barra de progresso (some sozinho em 1,4s)
  - lock com estrelas piscantes e tema espacial (Sol/Terra/Lua com fase)

## Build
- Última compilação (Arduino CLI, pasta oficial `Documents\Smartwatch`): **limpa**
  - Flash: **2.021.034 B (64%)** de 3.145.728 B
  - RAM global: **227.360 B (69%)** de 327.680 B (BLE compilado; só liga no runtime do scan)
- Se a compilação falhar com "corrupt cache" / erro de objs truncados: apague
  `%LOCALAPPDATA%\arduino\sketches\9CB80882957E5C40897447401C82DB46`
  e compile de novo (nunca dois arduino-cli em paralelo).
- ⚠️ Cada alteração em `core.h`/`ui.h` faz o arduino-cli recompilar a LVGL
  inteira: leva **10–15 min**. Rode em background e siga outras tarefas.

## Threads e barramento (leia antes de mexer)
- **LVGL só pode ser chamado na task do `loop()`**. Qualquer task (carga do WAV,
  rede, IR) deve pedir o redesenho com `gSdRefreshPending` — nunca chamar
  `lv_refr_now()` de outra task (`lvCtx()` diz se você está no contexto certo).
- **TFT e SD dividem o barramento SPI**: use `busLock()` / `busUnlock()` em
  qualquer acesso novo ao SD ou ao display fora do fluxo padrão
  (`sdBeginSession`/`sdEndSession` já fazem isso).

## Arquivos
- `smartwatch.ino` — setup/loop + servidor web de SD
- `ui.h` — toda a UI LVGL (telas, teclado, rede em task, IR TX/RX, lanterna, BLE, ping, crono/timer, clima, Google: DDG lite + Openverse/JPEGDEC)
- `home_art.h` — arte RGB565 extraída dos prints (Home 128×142, anéis da face padrão, diamantes e pílula); **não editar à mão**
- `gerar_art.py` — regenera `home_art.h` a partir de `apps.png` / `padraoface.png` (Pillow)
- `core.h` — hardware (pinos, SPIFFS, TFT, touch + calibração, Wi-Fi salvo, MPU, bateria, clima cfg)
- `apps.h` — UI antiga (removida do projeto; era código morto)
- `preview/ui_preview.html` — maquete HTML 1:1 da tela (128×160 ×3) usada para conferir layouts
- `lv_conf.h` em `Documents\Arduino\libraries\` — config da LVGL (`LV_TICK_CUSTOM 1` obrigatório; Montserrat 12/14/20/28/**32/48** — as duas últimas foram habilitadas para as watchfaces)
- `Documents\Arduino\libraries\JPEGDEC` (bitbank2) — decodificador JPEG usado nas miniaturas da aba Imagens (já instalado)
- Biblioteca **HTTPClient** (do core ESP32) — usada em todas as buscas do Google (já inclusa, nenhuma instalação extra)

## Checklist de teste no hardware (manhã)
1. **Teclado**: continua vivo nas tarefas de texto; **não fecha ao tocar** (só OK / voltar / apagar modal).
2. **Wi-Fi**: rede salva → aguenta até **30s** tentando antes de desistir; ícone 📶 verde; BT **azul durante o BLE Scan**.
3. **Google (novo)**: Home > Google → digite "esp32" → **Ir**. No **card do topo** aparece o título do 1º site, o resuminho e o host em verde; abaixo vêm os resultados (título em branco, **link em verde**; toque abre a tela **PAGINA** com o texto do site via r.jina.ai). Aba **Imagens**: até 4 miniaturas da Wikipédia (se cair, o toast informa "imagens falharam N/4").
4. **Clima**: Config > Clima → toque em **Atualizar** (Wi-Fi ligado); edite lat/lon e salve; lock passa a mostrar "23° nublado" (até 5 min de demora no automático). A tela do Clima mostra também **vento (km/h), umidade (%) e sensação térmica** numa linha de detalhes.
4. **Controle IR**: Toolbox > Controle > **Aprender** → aperte um botão de controle de verdade apontado pro RX; aparece BT1 (nome/código); toque reenvia; toque longo apaga. (Requer módulo receptor IR ligado no pino 2.)
5. **IR Loop**: liga/desliga por toque (2 toques).
6. **Lanterna**: toque fecha e devolve o brilho normal; se travar o relógio, fecha sozinha.
7. **Crono/Timer**: modos Crono↔Timer; no Timer, +/- muda preset (30s..10m); no fim bipa 3× e mostra "TEMPO ESGOTADO!".
8. **Ping** (ms), **BLE Scan** (nome+RSSI) e **Notas** (digita, salva e fecha; reabre com o texto lá).
9. **Calc**: `5 ÷ 0 =` → "Er"; numero de 11+ dígitos encolhe; barra C full-width. **Saúde**: arco + km/cal + Zerar cabendo; "sem MPU" quando sem módulo.
10. **Jogos → Dado/Moeda**: toque joga.
11. **Lock**: toque duplo destrava; swipe-back não rouba gesto com o teclado aberto; tema espacial anima.
12. **MPU**: se a unidade de pulso não tem MPU, Nível/Bolha/passos mostram o aviso — é hardware, não bug.
13. **Face padrão (a da foto)**: “12” topo-esquerda, “59” embaixo à direita, 98% + bateria no topo-direito e os 3 anéis embaixo à esquerda. **Arraste um anel** (passos/lime, km/verde, calorias/rosa): o valor aparece do lado, os outros 2 anéis se ajustam e o contador de passos do app Saúde muda junto. **Toque fora dos anéis** destrava.
14. **Personalização > Estilo**: Digital (HH:MM grande + mini-anel de passos) e Analógica (mostrador + ponteiros) — nenhum elemento invade a barra de status nem a hora.
