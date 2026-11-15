// ============================================================
//  Histórico do firmware do smartwatch.
//  FONTE DA VERDADE deste site. Editar aqui e o site se atualiza.
//
//  Cada item:
//    v        identificador curto (não é semver oficial; é o índice
//             do release, na ordem em que foi entregue)
//    data     AAAA-MM-DD (null quando não foi registrada)
//    quando   rótulo alternativo quando não há data ("Sessão 5")
//    titulo   o que mudou, em uma linha
//    estado   "ok"     = resolvido / compila limpo
//             "hw"     = pronto, falta validar no relógio
//             "aberto" = bug conhecido, ainda sem solução
//    resumo   o contexto: por que essa versão existiu
//    corrigido / adicionado / alterado: listas curtas (crase vira <code>)
//    arquivos  arquivos tocados
//    pendente  o que ficou em aberto nesta versão (opcional)
//    fonte     onde o detalhe completo está documentado
// ============================================================

window.VERSOES = [
  {
    v: "v0.25",
    data: "2026-11-16",
    titulo: "O cartão morre na associação do Wi-Fi, não na escrita",
    estado: "hw",
    resumo:
      "O log do relógio fechou o diagnóstico: `card=3` (SDHC funcionando) no boot e " +
      "`card=4` (CARD_UNKNOWN) exatamente quando o rádio associou, com `errno=19` " +
      "(FR_NOT_READY). A escrita nunca foi a causa nesta rodada — o cartão já estava " +
      "morto quando o upload começou.",
    corrigido: [
      "`cardType() != CARD_NONE` era tratado como \"montado\". CARD_UNKNOWN tem o VFS registrado e o DATA I/O morto: o firmware nunca remontava e seguia falhando com errno=19.",
      "`WiFi.setTxPower()` era chamado antes de `WiFi.begin()`/`softAP()`. O core checa `started()` e devolve false, então o rádio seguia a 19.5 dBm — o pico de corrente da associação que derruba o 3V3 do slot.",
      "`webSdRecover()` desmontava sem provar que o cartão voltou a falar; agora verifica com leitura crua do setor 0."
    ],
    adicionado: [
      "`sdAlive()`: lê o setor 0 pelo driver (sem FatFs) com guarda para `pdrv=0xFF`.",
      "Logs `[srv] sd antes do wifi` / `sd depois do wifi` / `remount pos-wifi`.",
      "Fallback de potência: 8.5 dBm por 8 s e, se não associar, 19.5 dBm."
    ],
    alterado: [
      "`webSdMounted()` e `sdMountedOk()` (ui.h) aceitam só MMC/SD/SDHC.",
      "`webSdMount`, `sdProbe` e `sdBeginSession` desmontam quando o estado é UNKNOWN (par end+begin).",
      "`/st`, `/up` e o start do servidor usam `!webSdMounted()` como gatilho de recovery, não `== CARD_NONE`."
    ],
    arquivos: ["Smartwatch.ino", "core.h", "ui.h", "docs/analise_sd.md", "STATUS.md"],
    pendente: [
      "Validar no relógio: o cartão deve sobreviver ao Wi-Fi (ou remontar com `ok=1`) e o upload gravar em /FOTOS.",
      "Se cair mesmo com TX baixo, o próximo alvo é hardware (capacitor no 3V3 do slot)."
    ],
    fonte: "STATUS.md §25 · docs/analise_sd.md §3.7"
  },
  {
    v: "v0.24",
    data: "2026-11-15",
    titulo: "Confirmado com teste A/B: a tela não quebra o SD",
    estado: "ok",
    resumo:
      "O sketch mínimo usando a própria lib gravou antes e depois de 30 s de animação a " +
      "8 MHz com a luz acesa. A conclusão antiga (\"a tela envenena o cartão\") estava " +
      "contaminada por um probe que mandava CMD0 antes de testar o CMD17.",
    corrigido: [
      "O `rawProbe` antigo mandava CMD0 antes do CMD17 e deixava o cartão em idle — o `r1=0x05` era do próprio teste.",
      "Diagnóstico do CMD13: em SPI o R2 tem 16 bits e não carrega o CURRENT_STATE; o sketch parou de reportar um estado inexistente."
    ],
    adicionado: [
      "`teste_sd` reescrito com a lib (`SD.readRAW`, CMD13, `SD.end()+SD.begin()`) e knobs `ANIMA_MS`/`ANIMA_LUZ`/`TFT_HZ`."
    ],
    alterado: [
      "Conclusão revista: o barramento compartilhado com o TFT nunca foi a causa."
    ],
    arquivos: ["teste_sd/teste_sd.ino", "docs/analise_sd.md", "STATUS.md"],
    pendente: ["Com a gravação provada, subir `SD_SPI_HZ` de 400 kHz para 1 MHz e medir o mesmo arquivo."],
    fonte: "STATUS.md §24 · docs/analise_sd.md §3.5"
  },
  {
    v: "v0.23",
    data: "2026-11-15",
    titulo: "errno=5 em toda leitura: era o CMD0 matando o CRC do cartão",
    estado: "ok",
    resumo:
      "`sdCardReInit()` mandava CMD0 antes de toda escrita. O CMD0 volta o cartão ao idle " +
      "e zera o CRC_ON_OFF, mas a lib continua com `supports_crc=true` e compara o CRC16 " +
      "de cada bloco lido — que vem 0xFFFF. Toda leitura vira FR_DISK_ERR (errno 5), e " +
      "`f_open` lê setores antes de gravar.",
    corrigido: [
      "`sdCardReInit()` e `sdR1(int)` removidos do `Smartwatch.ino`; a chamada saiu do `sdBeginSession()`.",
      "`sdSpiReset()` (`tftSPI.end()+begin()`) removido: quebrava o host SPI e o mount."
    ],
    alterado: [
      "Regra fixada: quem inicializa o cartão é só o `SD.begin()`. Nada de CMD0/ACMD41 por fora da lib."
    ],
    arquivos: ["Smartwatch.ino", "ui.h", "teste_sd/teste_sd.ino", ".claude/napkin.md"],
    fonte: "STATUS.md §23 · docs/analise_sd.md §3.1–3.4"
  },
  {
    v: "v0.22",
    data: "2026-11-14",
    titulo: "Camada de SD refeita no esquema do teste que grava",
    estado: "ok",
    resumo:
      "O sketch mínimo gravava e o relógio não. A camada de SD foi reescrita seguindo o " +
      "sketch: leitura antes de gravar, 2048 bytes, read-back e luz de fundo apagada " +
      "durante o teste.",
    corrigido: [
      "`busLock()` que devolve false no timeout: quem chama precisa pular a operação. `disp_flush` desenhava em cima do SD.",
      "`teste_sd3` e `teste_sd4` eliminaram tela e touch como causa antes de mexer no firmware."
    ],
    adicionado: [
      "`sdBusPrepare()`: CS do touch em ALTO + 8 clocks para limpar o MISO antes de todo acesso ao SD.",
      "Teste de escrita no boot, antes de UI/Wi-Fi/tasks: `[boot] sd: mount=1 grava=1 card=3`."
    ],
    alterado: [
      "Ordem do boot: `dispInit()` → montagem/teste do SD → `SPIFFS.begin()` → `cfgLoad()`.",
      "Tela a 8 MHz (`tft.setSPISpeed(8000000)`), não 40 MHz."
    ],
    arquivos: ["Smartwatch.ino", "core.h", "ui.h", "teste_sd/"],
    pendente: ["`SPIFFS.begin(true)` antes do SD era a última diferença estrutural para o sketch; ficou reordenado."],
    fonte: "STATUS.md §21–22"
  },
  {
    v: "v0.21",
    data: "2026-11-13",
    titulo: "RESOLVIDO: o touch (XPT2046) segurava o MISO",
    estado: "ok",
    resumo:
      "O XPT2046 divide o MISO (GPIO34) com o SD. Com o CS dele em baixo ou flutuando ele " +
      "dirige a linha, e o cartão não responde a comando nenhum — nem monta.",
    corrigido: [
      "CS do touch em ALTO em qualquer acesso ao SD (`sdBusPrepare`, chamado em `sdTryBegin`, `sdBeginSession`, `webSdMount`, `webSdWriteTest`, `webUpFile`).",
      "`touch_read()` e `touchTick()` não leem o toque durante `gSdPause`.",
      "Teste de setor cru removido do boot: ele gravava no cartão."
    ],
    adicionado: [
      "`teste_sd.ino` põe `TOUCH_CS (36) = OUTPUT/HIGH` e `TOUCH_IRQ (35) = INPUT_PULLUP`, como o relógio."
    ],
    arquivos: ["Smartwatch.ino", "core.h", "ui.h", "teste_sd/teste_sd.ino"],
    fonte: "STATUS.md §20"
  },
  {
    v: "v0.20",
    data: "2026-11-12",
    titulo: "Isolamento do defeito: módulo, cartão e barramento estão bons",
    estado: "ok",
    resumo:
      "Cinco sketches (SD sozinho, TFT+SD, TFT-primeiro, touch simulado, tela a 40 MHz) " +
      "gravaram. O hardware ficou inocentado passo a passo e a diferença estrutural " +
      "restante passou a ser o próprio firmware.",
    corrigido: [
      "\"sem espaço no SD\" falso: `totalBytes()==0` (f_getfree falhando) era lido como 0 MB livres e recusava upload num cartão de 16 GB.",
      "Botão \"Escolher arquivo\" não abria nada: o `<input type=file>` tinha sumido e voltou com `position:absolute;opacity:0` (não `display:none`)."
    ],
    adicionado: [
      "`teste_sd5` (tela a 40 MHz + SD) e `teste_sd4` (touch simulado) para isolar cada suspeito."
    ],
    alterado: [
      "`webSdWriteTest()` espelha o sketch: lê a raiz antes, grava 2048 bytes, relê e compara."
    ],
    arquivos: ["teste_sd/", "Smartwatch.ino", "core.h"],
    fonte: "STATUS.md §19–20"
  },
  {
    v: "v0.19",
    data: "2026-11-12",
    titulo: "Modo upload pronto: Wi-Fi salvo, tela nova e a página WearSend",
    estado: "ok",
    resumo:
      "Com Wi-Fi salvo o relógio entra em STA e mostra o IP na tela; sem Wi-Fi sobe o AP " +
      "\"WearSend\". A tela do relógio e a página do navegador foram refeitas no mesmo " +
      "estilo, paleta laranja/grafite.",
    corrigido: [
      "`SD.totalBytes()` devolvendo 0 recusava o upload com HTTP 507 num cartão de 16 GB; agora só recusa quando o espaço é conhecido e insuficiente.",
      "`webSdStart()` monta o cartão com o rádio desligado: o pico de corrente do Wi-Fi derrubava o mount."
    ],
    adicionado: [
      "Tela `uiServerScreen()`: card com REDE / SENHA / IP em laranja, barra de progresso, SD OK e botão FECHAR.",
      "`/st` devolve `rw <livreMB> <totalMB>`; a página mostra \"espaço desconhecido\" quando o total é 0.",
      "Dois toques para despertar (1 toque não acorda mais) e painel em SLPIN no lightsleep."
    ],
    alterado: [
      "Paleta do modo servidor: `SRV_BG/CARD/EDGE/ACC/TXT/DIM/OK/ERR`."
    ],
    arquivos: ["Smartwatch.ino", "ui.h", "core.h"],
    fonte: "STATUS.md §18"
  },
  {
    v: "v0.18",
    data: "2026-11-11",
    titulo: "Upload sem estourar PSRAM + controles do DOOM",
    estado: "ok",
    resumo:
      "Abrir apps em rajada estourava a PSRAM (cada ícone alocava 512 B e nunca liberava). " +
      "O upload passou a montar o cartão por conta própria e o DOOM ganhou controles que " +
      "distinguem arrastar de tocar.",
    corrigido: [
      "Vazamento de 512 B por ícone na PSRAM: buffer liberado no `LV_EVENT_DELETE`.",
      "Envio de IR travava a UI ~110 ms por botão: fila + task no core 0 (`irSendAsync`).",
      "Leitura do WAV travava até 1,4 s: carga em task com \"carregando...\".",
      "`ledc: requested frequency 200000 ... div_param=0`: portadora do SPK caiu para 100 kHz com fallback de 50 kHz.",
      "DOOM atirava ao arrastar: agora `RELEASED` só atira se andou <= 7 px e durou <= 600 ms."
    ],
    adicionado: [
      "`webSdEnsure()`, `webSdWriteTest()` e `/dbg` (mount, card, rootOpen, mkdir, openW/errno, write/errno, remove, heap).",
      "`psNeed()` reserva 24 KB de folga; `psCleanup(true)` antes de entrar em app."
    ],
    alterado: [
      "`SD.begin(..., 8)`: 8 handles de arquivo (upload + player + lista).",
      "Upload grava em `/FOTOS`, `/VIDEOS`, `/MUSICAS` com nomes 8.3."
    ],
    arquivos: ["Smartwatch.ino", "ui.h", "core.h"],
    fonte: "STATUS.md §5–8 (sessão 2026-11-11b)"
  },
  {
    v: "v0.17",
    data: "2026-11-10",
    titulo: "Travamentos: LVGL fora da task, SPI compartilhado e SD lento",
    estado: "ok",
    resumo:
      "\"Apertei um botão e travou\" tinha três causas reais de baixo nível, não apenas " +
      "demora: LVGL chamado de outra task, TFT e SD disputando o mesmo SPI e montagem " +
      "bloqueando a tela por 200–500 ms.",
    corrigido: [
      "`sdEndSession()` chamava `lv_refr_now()` da task de áudio (core 0). LVGL não é thread-safe: agora só redesenha na task do LVGL; de outra task marca pendência.",
      "`disp_flush` e as sessões de SD usavam o `tftSPI` sem exclusão: criado o mutex `gBusMtx`."
    ],
    adicionado: [
      "`.claude/napkin.md`: runbook com as regras de ouro do projeto."
    ],
    alterado: [
      "`sdTryBegin(int tries)`: 2 tentativas no caminho da UI, 5 no diagnóstico.",
      "Explorer lê a lista 40 ms depois de pintar a tela, com guarda `lv_scr_act() == scrArquivo`."
    ],
    arquivos: ["core.h", "ui.h", "Smartwatch.ino", ".claude/napkin.md"],
    fonte: "STATUS.md (sessão 2026-11-10d)"
  },
  {
    v: "v0.16",
    data: "2026-11-10",
    titulo: "Menu HyperOS, Explorer estilo Windows e o vazamento de PSRAM",
    estado: "ok",
    resumo:
      "Antes do travamento aparecer: menu 3x3 com ícones pixel-art, temperatura na face, " +
      "símbolo de Bluetooth em todas as telas e um explorador de arquivos com pastas.",
    corrigido: [
      "Vazamento de PSRAM em `icoCanvas` (512 B por ícone, nunca liberados).",
      "IR bloqueava a UI, WAV bloqueava 1,4 s, `beepSpeaker()` desanexava o canal do player."
    ],
    adicionado: [
      "`sdListDir()` com pastas primeiro, barra de endereço, colunas Nome | Tamanho, toque longo para apagar.",
      "`wxApplyData()`: o clima em segundo plano também alimenta a temperatura da face."
    ],
    alterado: ["Matemática e Saúde passaram para o menu Config."],
    arquivos: ["ui.h", "core.h"],
    fonte: "STATUS.md (sessão 2026-11-10c)"
  },
  {
    v: "v0.15",
    data: "2026-11-10",
    titulo: "Anéis da face viraram sliders rotativos de verdade",
    estado: "ok",
    resumo:
      "Os três anéis eram decorativos. Passaram a girar com o dedo, com hit-test apenas na " +
      "faixa do arco (tocar fora continua destravando o relógio) e uma única fonte de " +
      "verdade: os passos.",
    corrigido: [
      "Timers de jogos checavam só `gInGame`: sair do Pong e abrir o Flappy deixava o Pong simulando e dando toast \"VOCÊ VENCEU\". Cada timer agora valida a própria tela.",
      "Dama: captura obrigatória considerava as duas cores e travava a jogada do jogador; multi-salto e fim de jogo corrigidos.",
      "Xadrez e Pong não tinham reinício; a bola/raquetes engoliam o toque."
    ],
    adicionado: [
      "`ringHitTest()` / `ringEventCb()` / `ringApplyValue()`: girar grava só a chave `steps`.",
      "Escalas alinhadas: passos 0..10000, km 0..70, calorias 0..400 (o app Saúde acompanhou)."
    ],
    alterado: [
      "Layout de 10 telas revisado para caber em 128x160 (crono, player, moeda, imagem, fotos, IA, Wi-Fi, clima, face analógica).",
      "2 bytes ANSI quebrados em comentários do `ui.h`."
    ],
    arquivos: ["ui.h", "core.h"],
    fonte: "STATUS.md (sessão 2026-11-10)"
  },
  {
    v: "v0.14",
    data: "2026-11-09",
    titulo: "Face padrão = a foto do usuário, e sleep com tela realmente preta",
    estado: "hw",
    resumo:
      "A face Padrão foi refeita para bater com a foto: HH no topo-esquerdo, MM no meio-" +
      "direita, anéis com a geometria original e o cluster de status no topo.",
    corrigido: [
      "`fakeApApply` montava \"AP falso: on/off\" e nunca aplicava no label.",
      "Aviso \"toque p/ destravar\" aparecia sempre; agora só quando o guard de 2 toques está armado.",
      "`sendCommand(NULL)` ambíguo no SLPIN/SLPOUT do ST7735."
    ],
    adicionado: [
      "Lightsleep com SLPIN (tela 100% preta) e RTC interno contando o horário.",
      "Migração `lock2`: o primeiro boot põe `lockStyle=2` (o device mostrava a face Digital)."
    ],
    alterado: [
      "Toolbox em lista rolável de 18 itens; Tela, Clima e Ajustes restilizados (redRow/uiRowSw)."
    ],
    arquivos: ["ui.h", "core.h"],
    pendente: ["Validar no pulso: face = foto, sleep preto + 2 toques, cluster wifi/BT."],
    fonte: "STATUS.md (sessão 2026-11-09b)"
  },
  {
    v: "v0.13",
    data: "2026-11-09",
    titulo: "Redmi Watch: chips circulares, anéis e acento laranja",
    estado: "hw",
    resumo:
      "Identidade visual trocada para Redmi Watch / Mi Band: Home 3x3 de chips circulares " +
      "com ícone branco, acento laranja Redmi e anéis de atividade na face.",
    adicionado: [
      "Chips circulares 36 px com cor por app, sem rótulo (estilo Mi Band).",
      "Anéis passos/laranja, km/verde, calorias/ciano com escalas por atividade.",
      "Ícones pixel-art 16x16 para todos os apps e jogos."
    ],
    alterado: [
      "Acento padrão = laranja Redmi (`accIdx=2`, migração `redmi`).",
      "Status bar usa `LV_SYMBOL_BLUETOOTH` em vez do texto \"BT\"."
    ],
    arquivos: ["ui.h", "core.h", "home_art.h"],
    pendente: ["Validar no pulso: toque nos chips, anéis e temperatura de Campo Grande."],
    fonte: "STATUS.md (sessão 11)"
  },
  {
    v: "v0.12",
    data: "2026-11-09",
    titulo: "Upload coordenado de ponta a ponta (P1) e limpeza do projeto",
    estado: "ok",
    resumo:
      "Plano aprovado em duas frentes: P1 corrigir o upload de arquivos e P2 a face " +
      "Padrão. `apps.h` (29,7 KB, código morto nunca incluído) foi removido do projeto.",
    corrigido: [
      "Upload: `mkdir` com fallback, 2ª tentativa de `SD.open` sem desmontar o cartão, escrita em laço e conferência de tamanho no END.",
      "`apps.h` tinha stubs de BadUSB/BadBLE que confundiam a leitura do projeto."
    ],
    adicionado: [
      "Logs `[up] start/END` com mount, card, mkdir, open e total.",
      "`preview/teste_*.html`: toda tela nova validada em HTML fiel 128x160 antes do firmware."
    ],
    arquivos: ["Smartwatch.ino", "ui.h", "core.h"],
    fonte: "STATUS.md (sessões 2026-11-09c/d)"
  },
  {
    v: "v0.11",
    data: "2026-11-09",
    titulo: "Doom em raycaster, Sonic e Mario distintos, 13 jogos",
    estado: "ok",
    resumo:
      "O \"Doom\" virou um raycaster 3D propriamente dito (DDA, framebuffer 128x96 em " +
      "PSRAM, paredes com sombra por distância, inimigos billboard com z-buffer).",
    corrigido: ["Colisão de nome `dm?Dama` renomeada."],
    adicionado: [
      "Motor `dooCast`: arraste move/vira, toque atira, 6 inimigos, fase limpa reinicia.",
      "Sonic com SPIN-DASH (toque longo = invulnerável ~1 s); Mario com pulo clássico.",
      "Snake e prévia da próxima peça no Tetris (13 jogos no total)."
    ],
    alterado: ["Home com chip de ícone arredondado (watchOS)."],
    arquivos: ["ui.h", "core.h"],
    fonte: "STATUS.md (sessão 10)"
  },
  {
    v: "v0.10",
    data: "2026-11-09",
    titulo: "BLE: controle de mídia e achar meu celular",
    estado: "hw",
    resumo:
      "Toolbox ganhou \"Mídia BT\": <, play/pausa, >, volume, mudo e \"Achar meu celular\" " +
      "(rajada de play/pausa faz o telefone tocar). HID próprio \"Wear-Midia\".",
    adicionado: [
      "Report map consumer 0x0C, report ID 1, 16-bit one-shot. Não mexe na USB/reflash.",
      "`gBtOn` acende o ícone BT na barra."
    ],
    corrigido: ["Protótipos e `uiLabel` com 7 argumentos quebravam o build."],
    arquivos: ["ui.h"],
    pendente: ["Pareamento só dá para validar no pulso."],
    fonte: "STATUS.md (sessão 9)"
  },
  {
    v: "v0.09",
    data: "2026-11-09",
    titulo: "Espelho do LCD por HTTP (pixel-idêntico)",
    estado: "hw",
    resumo:
      "O framebuffer 128x160 RGB565 vai cru para o navegador via `send_P` e vira canvas. " +
      "O AP \"WearMirror\" sobe no Toolbox e a página fica idêntica ao firmware.",
    corrigido: [
      "Concatenação ilegal de literais C (`</scr\"+\"ipt>`) e `send()` binário trocado por `send_P`.",
      "\"Definir do SD\" decodifica JPEG/BMP direto para 128x160 RGB565 na SPIFFS."
    ],
    adicionado: ["`mirrorTick()` no loop mantém o LVGL rodando com o espelho ligado."],
    arquivos: ["ui.h", "core.h", "Smartwatch.ino"],
    pendente: ["Validar no pulso: AP WearMirror e canvas no navegador."],
    fonte: "STATUS.md (sessões 6–8)"
  },
  {
    v: "v0.08",
    data: "2026-11-09",
    titulo: "App Arquivos navegável + player WAV",
    estado: "ok",
    resumo:
      "O app Arquivos virou um browser real do cartão: botões Enviar/Atualizar/Info/Música " +
      "e lista rolável. Tocar num .wav toca, num .jpg define wallpaper.",
    adicionado: [
      "`wpSetFromPath()`, `buildPlayer(int idx)` e `audFiles/audCount` acessíveis ao browser.",
      "Schematic HTML com visual de placa e brilho de vidro."
    ],
    alterado: ["Coordenadas do appBody (46), botões e lista validados em HTML antes de flashar."],
    arquivos: ["ui.h", "core.h"],
    fonte: "STATUS.md (sessão 5)"
  },
  {
    v: "v0.07",
    data: "2026-11-09",
    titulo: "Estabilidade: CS do SD travado em HIGH, retry na leitura do boot",
    estado: "ok",
    resumo:
      "Primeira rodada de robustez do cartão. O CS (GPIO39) passou a ficar travado em HIGH " +
      "no repouso, e o probe passou a reler o setor de boot até 3x.",
    corrigido: [
      "\"gravação: ?\" era o setor de boot falhando na primeira leitura.",
      "Boot sem animação (a pedido)."
    ],
    adicionado: ["`sdTryBegin` a 4 MHz com 5 tentativas; AP do espelho em canal 1 fixo com log."],
    alterado: ["Clima forçado para Campo Grande/MS com migração única `wxcg`."],
    arquivos: ["core.h", "ui.h", "Smartwatch.ino"],
    fonte: "STATUS.md (sessão 8)"
  },
  {
    v: "v0.06",
    data: "2026-11-09",
    titulo: "Upload confiável com X-File-Size, preview e Snake",
    estado: "ok",
    resumo:
      "A barra de progresso passou a usar o header `X-File-Size` (blob.size) com fallback " +
      "\"recebendo...\". Todo arquivo recebido ganhou preview automático.",
    adicionado: [
      "`serverPreview()`: .jpg/.bmp abre tela cheia com \"Definir wallpaper\"; .wav toca.",
      "Saúde com 3 anéis concêntricos progressivos (km, passos, calorias).",
      "Snake com arraste de direção e crescimento ao comer."
    ],
    alterado: ["Wallpaper refatorado: `wpDecodeFile()` + `buildImgView()`."],
    arquivos: ["Smartwatch.ino", "ui.h"],
    fonte: "STATUS.md (sessão 7)"
  },
  {
    v: "v0.05",
    data: null,
    quando: "Sessão 4",
    titulo: "UI pixel a pixel dos prints + simulador de bancada",
    estado: "hw",
    resumo:
      "A Home e a face Padrão passaram a ser arte RGB565 extraída 1:1 das imagens, com " +
      "botões invisíveis sobre os ícones. Nasceu o simulador com a fiação real.",
    adicionado: [
      "`home_art.h` gerado por `gerar_art.py` (Home, anéis, diamantes, máscara de bateria).",
      "9 botões invisíveis nas coordenadas exatas do print.",
      "`simulador_schematic.html`: placa, módulos, fios por barramento e painel por módulo."
    ],
    alterado: ["`appChipIco()` removido: a Home é arte + header dinâmico."],
    corrigido: ["Colisão de nome `S` (helper SVG x estado) e typos de cor CSS."],
    arquivos: ["ui.h", "home_art.h", "gerar_art.py", "simulador.html"],
    pendente: ["Imagens da face analógica e digital não chegaram."],
    fonte: "STATUS.md (sessão 4)"
  },
  {
    v: "v0.04",
    data: null,
    quando: "Sessão 3",
    titulo: "Upload: a resposta saía no handler errado",
    estado: "ok",
    resumo:
      "Toda página dizia \"falha ao enviar\" porque `sdWeb.send()` era chamado dentro do " +
      "`webUpFile()` (handler de upload). A resposta só pode sair no handler do POST.",
    corrigido: [
      "Resposta do upload movida para `webUpDone()`.",
      "Conversão no navegador: imagem para JPEG 128x160 (cover-crop), música para WAV 8 kHz mono."
    ],
    adicionado: [
      "Abas Imagens/Vídeos/Músicas e lista do SD por `GET /list`.",
      "Barra de progresso nos dois lados (navegador e TFT)."
    ],
    alterado: ["Tela do servidor validada em HTML fiel 128x160 antes do firmware (barra y=120)."],
    arquivos: ["Smartwatch.ino", "ui.h"],
    fonte: "STATUS.md (sessão 3)"
  },
  {
    v: "v0.03",
    data: null,
    quando: "Sessão 2",
    titulo: "Reboot ao abrir apps: nav() vazava a tela anterior",
    estado: "ok",
    resumo:
      "Abrir e fechar apps reiniciava o relógio. O `nav()` nunca apagava a tela anterior " +
      "(`delOld=false`), e o heap do LVGL vazava até o crash.",
    corrigido: [
      "`nav()` agora apaga a tela anterior; Home e Lock ficam preservadas.",
      "6 botões \"Voltar\" dependiam da tela destruída: passaram a recriar o alvo."
    ],
    adicionado: [
      "3 watch faces (Digital, Analógico, Padrão) e 8 temas de cor.",
      "Wallpaper a partir do SD gravado cru na SPIFFS (`/wp.bin`).",
      "Clima de Campo Grande/MS (lat -20.4428, lon -54.6464) com migração `wxcg=2`."
    ],
    arquivos: ["ui.h", "core.h"],
    fonte: "STATUS.md (sessão 2)"
  },
  {
    v: "v0.02",
    data: null,
    quando: "Sessão 1",
    titulo: "SD no barramento compartilhado com o TFT",
    estado: "ok",
    resumo:
      "TFT e SD dividem SCK=9, MOSI=10 e MISO=34. O SD estava numa instância HSPI separada " +
      "com o mesmo MISO, e a detecção saía invertida (conectado = ausente).",
    corrigido: [
      "SD passou a usar o `tftSPI` (mesmo host do TFT, sem re-begin).",
      "`sdProbe` monta/desmonta e `sdTryBegin` tenta 3x."
    ],
    arquivos: ["core.h", "ui.h"],
    fonte: "STATUS.md (correção do SD)"
  },
  {
    v: "v0.01",
    data: null,
    quando: "Sessões anteriores",
    titulo: "Base do relógio: player WAV e teclado Bluetooth",
    estado: "hw",
    resumo:
      "Antes do histórico detalhado: o relógio já tocava música do cartão e funcionava " +
      "como teclado Bluetooth. As sessões seguintes partiram daqui.",
    corrigido: [
      "BLE Scan: a lib BLE nunca entrava no build, então o scan simplesmente não existia."
    ],
    adicionado: [
      "Player WAV: SD -> PSRAM -> PWM no core 0, com conversão MP3/MP4/etc para WAV 8 kHz mono no navegador.",
      "BadBLE: teclado HID + `/duck.txt` (Ducky-lite)."
    ],
    arquivos: ["ui.h", "core.h", "Smartwatch.ino"],
    pendente: ["Validar no pulso: parear como \"Wear-KB\" e rodar o script."],
    fonte: "STATUS.md - 'Feito em sessões anteriores'"
  }
];
