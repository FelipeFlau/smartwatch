# STATUS - retome daqui

## Sessao 4 - UI pixel a pixel dos prints (FEITO, aguardando teste no hardware)
- `home_art.h` (novo, gerado por script): arte RGB565 extraida 1:1 das imagens do usuario.
  * `home_art_565[128*142]` = Home (recorte y=18..159 de apps.png, largura completada ate 128)
  * `home_diamond_565[13*13]`, `face_diamond_565[11*11]`, `face_rings_565[46*48]`, `face_bat_mask[7*11]`
- HOME: `buildHomeS()` agora desenha a arte em (0,18) + header dinamico proprio
  (hora F_UI em 5,5; diamante em 74,5; "%" alinhado a direita ate x116; pilula 6x12 em 121,6)
  e 9 botoes INVISIVEIS sobre os icones, nas coordenadas exatas do print.
  Ordem: 1 power(9) 2 mat(2) 3 google(3) 4 jogos(5) 5 saude(6) 6 IA(4) 7 arquivos(7) 8 toolbox(1) 9 config(0).
  `appChipIco()` (chips desenhados) foi removido; `statusTimer` atualiza homeTime/homePct/homeBat
  (verde >=20%, vermelho <20%).
- FACE PADRAO (lockStyle 2): "12" F_HUGE em (30,29) e "59" F_HUGE em (58,79) como dois labels;
  aneis desenhados da ARTE (`face_rings_565`) em (9,85) - os lv_arc continuam existindo mas
  ficam escondidos nessa face; diamante 11x11 em (76,4); "%" verde ate x112; pilula 7x11 em (116,4).
  Removidos da face padrao: temperatura, data, passos e BT.
- `simulador.html`: Home e face padrao agora usam as imagens reais (`assets/apps.png`,
  `assets/face_padrao.png`) com background-position/crop, mais o mesmo header dinamico.
- PENDENTE: imagens da face ANALOGICA (3a) e DIGITAL (4a) nao chegaram no anexo.

## Correcao do SD (barramento) - FEITO
- Causa: TFT e SD dividem SCK=9/MOSI=10/MISO=34 -> e UM barramento SPI, dois CS. O SD estava num HSPI separado com o MESMO MISO -> disputa de pino -> deteccao invertida (conectado=ausente / desconectado=FAT).
- Fix: SD usa tftSPI (mesma instancia/host do TFT, sem re-begin). sdProbe monta/desmonta. sdTryBegin com 3 tentativas.
- Se ainda falhar: baixar freq p/ 4MHz ou revisar o CS=39 na fiacao.

## Sessao 2 - UI refeita (estilo Xiaomi) + reboot - FEITO
- Reboot/tela branca: o nav() NUNCA deletava a tela anterior (delOld=false) -> vazamento de heap LVGL -> crash. Corrigido: nav() agora deleta; Home/Lock preservados; 6 botoes "Voltar" p/ app especifico foram mudados p/ RE-RECRIAR o alvo (goHtmlCb/fmBtnCb/plExitCb/bbBackCb/tvgExitCb/cfgResetCancelCb).
- Watch faces (3): Personalizacao > Estilo agora e Digital / Analogico / Padrao.
  - Digital: hora grande + passos + clima + data (estilo Xiaomi).
  - Analogico: aro + 3 ponteiros (hora/min/seg) + hora pequena + passos.
  - Padrao: hora + data, limpo.
- 8 temas de cor de destaque (Personalizacao > Cor).
- Wallpaper: Personalizacao > Definir do SD pega a 1a imagem (JPEG/BMP) do SD, decodifica p/ 128x160 RGB565 e grava cru na SPIFFS /wp.bin; aparece dim atras do lock e home. "Remover" apaga.
- Clima = Campo Grande, MS (lat -20.4428 = 20º26'34"S, lon -54.6464 = 54º38'47"W) por padrao (migracao `wxcg=2` reaplica uma vez).

## Feito em sessoes anteriores
- Player de musica WAV (SD->PSRAM->PWM, nucleo 0), conversao MP3/MP4/etc->WAV 8kHz mono NO NAVEGADOR no upload.
- BadBLE (teclado Bluetooth HID + /duck.txt Ducky-lite); BLE Scan corrigido (a lib BLE nunca entrava no build).

## Testar NO PULSO (nao verificavel aqui)
- SD: conectado mostra FAT/MB; desconectado mostra "ausente".
- Reboot: abrir/fechar apps muitas vezes NAO deve mais reiniciar.
- Watch faces: alternar Digital/Analogico/Padrao; ponteiros apontam certo.
- Wallpaper: "Definir do SD" decodifica e aparece no lock/home (JPEG grande pode estourar a PSRAM).
- BadBLE: parear como "Wear-KB" e rodar o script.

## Ainda em aberto
- Tema CLARO completo (light theme) - refactor grande. Hoje: 8 cores de destaque.
- Player de VIDEO (MJPEG).
- BadUSB (USB-HID) - remove a porta de programacao; opt-in.
- Redimensionar fotos no upload do navegador (audio ja converte; imagem ainda nao).

## Compilacao atual: LIMPA - 0 erros, 58% flash, 66% RAM.
## Sessao 3 - Webserver upload refeito + progresso (FEITO, compila limpo)
- Causa do "sempre falha ao enviar": chamava sdWeb.send() DENTRO do webUpFile (handler de upload). Pesquisa confirmou: a resposta so sai pelo handler do POST (webUpDone). Corrigido.
- Pagina nova com ABAS Imagens/Videos/Musicas; converte por categoria no NAVEGADOR:
  * Imagem -> reescalada 128x160 JPEG (cover-crop)
  * Musica -> WAV 8kHz mono
  * Video -> apenas armazena (relogio nao reproduz video) - avisa
- Lista do SD via GET /list (pagina mostra os arquivos).
- Barra de progresso nos DOIS lados: navegador (XMLHttpRequest.upload.onprogress) e TFT do relogio (webServBar, modo servidor).
- Layout da tela do servidor validado num HTML fiel 128x160 ANTES de aplicar (barra y=120, rotulo y=108).

## REGRA: toda correcao/integracao -> primeiro um HTML fiel (layout/UI/pixels 128x160) que eu testo, so depois vai pro firmware.
- Paginas em preview/ (http://127.0.0.1:8848/): teste_webserver.html, teste_servidor.html, simulador.html (relogio+modulos), simulador_schematic.html (schematic estilo Wokwi com fiacao por barramento + SPI compartilhado destacado + GPIOs + clique realca + toggles). Todas com banner window.onerror.

## Proximas partes (seguindo sem parar)
- Terminar app ARQUIVOS no relogio: listar SD de verdade + integracao player/midia/wallpaper (testar em HTML fiel primeiro).
- Polish da UI (estilo Xiaomi) / watch faces mais ricas.

## Sessao 4 - Diagrama Wokwi-like + painel por modulo (FEITO, HTML)
- simulador_schematic.html refeito: palco fixo 1000x560, ESP32-S3 (placa vermelha) central com pinos rotulados (GPIO), modulos ao redor (Display com tela viva, Touch, SD, MPU, IR, Speaker, Bateria), fios SVG right-angle coloridos por barramento, trilhas 3V3/GND, SPI compartilhado (9/10/34) em destaque - igual ao bug do SD.
- Clique num modulo -> painel: FUNCAO + FIACAO (GPIO) + PARAMETROS que agem no sistema:
  * MPU: sliders X/Y giram o modulo e abrem o Nivel (bolha reage), zerar passos
  * Bateria: slider 0-100 (% na barra, <20% vermelho)
  * Display: troca watch face + brilho (escurece a tela)
  * SD: toggle presente/ausente (afeta Arquivos), listar
  * IR: botao pisca o LED do modulo; Speaker: abre player
- Bugs achados no teste antes de entregar: colisao de nome S (helper SVG vs estado) -> renomeado p/ mk; typos de cor CSS. Sintaxe validada (parse OK).
- Nota honesta: a tela HTML NAO eh byte-identica ao LVGL (fonte monospace vs montserrat); usa os MESMOS coordenadas/estrutura do firmware (lock 3 faces, home grid 3x, toolbox), pixels logicos 128x160.

## Sessao 5 - App ARQUIVOS terminado (FEITO, compila limpo)
- buildArquivo virou browser real: botoes Enviar/Atualizar/Info/Musica + lista rolavel dos arquivos do SD (sdList).
- Clique num item: .wav -> toca (buildPlayer(idx), audList()+match); .jpg/.bmp -> define wallpaper (wpSetFromPath); outro -> aviso.
- Refatorado: wpSetFromPath(arquivo) + wpSetFromSD(1a imagem); buildPlayer(int idx); audFiles/audCount movidos p/ cima (usados pelo browser).
- Testado em HTML fiel primeiro: preview/teste_arquivos.html (128x160, coords do appBody 46, botoes, lista, rodape - confirma caber sem sobrepor).
- Schematic com visual PCB mais proximo do Wokwi: textura de placa, conector USB no ESP, brilho de vidro na tela (so CSS).

## Sessao 6 - UI estilo marca + ESPELHO do LCD (FEITO, compila limpo)
- Pesquisa de UI: Xiaomi/Mi Band = hora grande + "complicacoes" nos cantos (passos/clima/BPM/bateria); watchOS = grid de apps com icone em chip arredondado. Adaptei:
  * Home (appCard): icone agora num CHIP quadrado arredondado na cor de destaque, icone escuro sobre o chip, nome arredondado abaixo (estilo watchOS).
- ESPELHO DO LCD por HTTP (pagina fica PIXEL-IDENTICA ao firmware): gFrame expoe o framebuffer; mirrorWeb(81) AP "WearMirror"; /s envia 128x160 RGB565 cru (send_P); /s pagina HTML decodifica RGB565->canvas. mirrorTick() no loop mantem LVGL rodando. Botao Toolbox > "Espelho" liga/desliga (toast com o IP).
- Decodificador RGB565 testado em HTML antes: preview/teste_espelhar.html (auto-verifica preto/branco/vermelho/verde/azul + byte-order) - 7/7.
- Bugs achados no teste antes de flashar: concatenacao ilegal de literais C ("</scr"+"ipt>") e send() binario -> send_P.
- Para usar: Toolbox > Espelho -> conecta no WiFi "WearMirror" (senha 12345678) -> abre http://192.168.4.1 no navegador.

## Sessao 7 - Upload corrigido + preview + Snake + Tetris-preview (FEITO, compila limpo)
- UPLOAD: barra de progresso confiavel via header X-File-Size (navegador envia blob.size); fallback "recebendo..." (marquee) se tamanho desconhecido; log Serial [up] (start/END/bytes) para diagnostico.
- Preview automatico ao receber arquivo (loop do servidor despacha serverPreview): .jpg/.bmp -> tela cheia + "Definir wallpaper"; .wav -> toca no player; outro -> aviso + lista. arRowCb imagem agora abre preview.
- WALLPAPER refatorado: wpDecodeFile(path) (decode limpo) + buildImgView (tela cheia) + serverPreview (despacho).
- SAUDE: 3 aneis concentricos progressivos (km=verde, passos=destaque, cal=laranja) com "passos" no centro - estilo Samsung/Xiaomi. compile limpo.
- HOME: cards com chip de icone arredondado na cor de destaque (watchOS).
- ESPELHO do LCD por HTTP (AP WearMirror, porta 81) - framebuffer RGB565 cru -> canvas no navegador (pixel identico). decodificador byte-explicito testado (teste_espelhar.html 7/7).
- JOGO NOVO: SNAKE (classico grade 6px, arraste direcao, come e cresce, aceita velocidade; logica testada em Python). Agora 13 jogos.
- TETRIS: previa da PROXIMA peca (canto "PROX").
- Compilacao: 0 erros, 58% flash.

## Proximas etapas
- BLE: Find-My-Phone / controle de midia (HID consumer) - CUIDADO: so da pra validar pareamento no dispositivo.
- Fidelidade maior dos jogos existentes (formacoes/fases) + revisao geral.

## Sessao 8 - Estabilidade e coisas importantes (jogos deixados por ultimo)
- BOOT: animacao removida (a pedido).
- CLIMA: for�ado Campo Grande, MS (migracao unica key wxcg) + parser wxField ja testado (7/7 com API real). Esperado: dados certos de Campo Grande.
- ESPELHO: AP agora robusto (WIFI_AP_STA, softAP em canal 1 fixo com verificacao + log Serial). Se nao criar AP, o toast diz "falha ao criar AP".
- SD (mais importante): CS (GPIO39) travado em HIGH no repouso (setup + sdEndSession) - nao flutua nem responde ruido do TFT/touch no barramento compartilhado. sdTryBegin a 4MHz/5x. sdProbe agora retenta leitura do setor de boot 3x (o "grava��o: ?" era o boot sector falhando).
- ICONES: todos os apps/jogos agora tem icone PIXEL-ART 16x16 desenhado (nao aparece mais "TEXT"). Preview em teste_icones.html.
- HOME: cards com chip de icone arredondado (watchOS); 8 temas de cor.
- REBOOT: nav() com delOld=true (libera tela anterior) - fix do vazamento confirmado ativo.
- Compilacao: 0 erros.
- Jogos por ultimo (nao ha verificado DOOM raycaster ainda - script pronto doom.py).

## Sessao 9 - BLE Controle de midia + Achar celular (FEITO, compila limpo)
- Toolbox > "Midia BT" (substituiu o "IR Loop" de pouco valor): tela CONTROLE BT com botoes <, PLAY/PAUSA, >, VOL+, VOL-, MUDO e "Achar meu celular" (rajada de play/pausa faz o celular tocar).
- Implementado como HID proprio "Wear-Midia" (report map consumer 0x0C, report ID 1, 16-bit one-shot). NAO mexe na USB/reflash. gBtOn acende o icone BT na barra.
- Bugs achados no teste (compilacao) e corrigidos: bloco inserido antes do include BLEHIDDevice (incluido no bloco); prototype no topo; uiLabel com 7 args (removido o 124).
- ATENCAO: pareamento/teste so da pra validar no PULSO (aqui nao tenho radio). O descritor consumer e o padrao documentado.
- Compilacao: 0 erros.

## Progresso geral (coisas importantes feitas):
boot removido, clima Campo Grande+parser correto, espelho AP robusto, SD (CS alto+retry+4MHz), icones pixel-art (fim do "TEXT"), home watchOS, reboot fix (nav delOld), varedura 0 warnings, BLE midia/find.
- Jogos por ultimo: DOOM raycaster pronto (doom.py, nao aplicado) - aplicar depois.

## Sessao 10 (madrugada) - Faces ricas + Doom 3D + Sonic/Mario distintos (FEITO, compila limpo)
- FACES: face digital ganhou ANEL DE ATIVIDADE (passos, canto sup. direito, indicador accent) + TRANSICAO fade-in suave ao entrar/trocar de face (animFadeIn).
- DOOM: agora e um TIRO 3D real (raycaster estilo wolf3d): motor proprio (dooCast DDA, framebuffer 128x96 em PSRAM, paredes com sombra por distancia, inimigos-billboard com z-buffer, mira, 6 inimigos, arraste=move/vira, toque=atira, fase limpa/reinicia). Substituiu o "corredor" do motor compartilhado. Colisao de nome dm?Dama corrigida (rename dm?doo). Logica DDA testada em Python.
- SONIC vs MARIO (distintos agora): Sonic corre mais + SPIN-DASH (toque longo = invulneravel ~1s, atravessa obstaculos e ganha pontos); Mario = pulo classico. Herois ja com cores proprias.
- Jogo total: 13 (PONG, INVASORES, FLAPPY, DINO, TETRIS, DAMA, XADREZ, MARIO, SONIC, DOOM, DADO, MOEDA, SNAKE).
- Compilacao: 0 erros. Varredura de warnings: 0.

## Pendentes conhecidos
- MI Band UI mais profunda (icones/chips/accent ja aplicados; falta identidade tipografica mais forte - proxima sessao).
- Validar no pulso: SD, espelho, MidiaBT, Doom, faces.


## Sessao 11 - Visual Redmi Watch / Mi Band (FEITO, compila limpo)

### FACE PADRAO (lockStyle == 2) — Redmi Watch (padrãoface.png)
- Fundo preto, relogio ENORME: HH (esq/top) + MM (dir/baixo) em montserrat_28
- Temperatura de Campo Grande, MS no TOPO (gWxTxt, clima ja forçado)
- Bateria (icone + %) topo-direita
- 3 aneis de atividade (bolinhas) canto inferior-esquerdo:
  passos (externo, laranja Redmi 0xFF7A00, 0..6000), km (medio, verde, 0..500),
  calorias (interno, ciano, 0..300). Data so aparece sem clima.
- Fade-in ao trocar de face mantido. Tema espacial so em digital/analogico.

### HOME — Redmi / Mi Band (apps.png): chips circulares 3x3
- Grade 3x3 de chips circulares (36px), cor distinta por app, icone branco centralizado
- Toolbox vermelho, Mat roxo, Google borrao, Saude lima, Jogos verde, IA preto,
  Arquivos amarelo, Config marrom, Relogio cinza (alarme)
- Sem rotulo (estilo Mi Band), mantido appCardCb por id

### Identidade global
- Acento = laranja Redmi (accIdx=2, ja por migracao "redmi")
- Tema escuro UI_BG, cards UI_CARD, botoes arredondados

### Compilacao: 0 erros, 58% flash / 68% RAM. Varredura warnings: 0 (sessao anterior).

### Pendente validar no pulso (hardware/radio, nao da p/ testar aqui)
- Face Padrão: horas, aneis, temp Campo Grande, bateria
- Home: chips circulares tocam e abrem apps
- SD, espelho, MidiaBT, Doom (sessoes anteriores)


## Sessao 2026-11-09: Wake 2-toques + Personalizar/Config/Ajustes Redmi

### Wake por 2 toques em ate 2s (refeito)
- 1 toque NAO acorda mais: wakeArmDark() arma espera de DUPLO toque (idem na rota wifi)
- globais gWakeWait/gWakeWin/gWakeTaps/gWakeLight (ui.h ~1047, ANTES de touch_read)
- touchTick conta toques: 2 dentro de 2s -> wakeFinish() = blOn() + gWakeArm (toque que acordou nao destrava)
- janela expira: volta ao lightsleep (gWakeLight=true) ou re-aguarda no modo wifi (loop ino)
- touch_read trava a LVGL durante a espera (sem interacao fantasma)
- doSleep: sem blOn no retorno; wifi blOff chama wakeArmDark(false)
- switch "2 toques p/ acordar" em Ajustes limpa a espera ao desligar

### Telas restilizadas (Redmi)
- Personalizar: secoes DIMINUTAS, 8 chips circulares 16px (anel branco no ativo), estilo segmentado
  Digital/Analog/Padrao (38x18, ativo laranja), linha "Tema espacial" com switch 26x16, botoes
  wallpaper "Do SD"/"Remover WP" 57px na base (112px totais, sem scroll)
- Config: helper redRow() = card 18px, icone ACC a esq, texto, seta LV_SYMBOL_RIGHT a dir;
  lista rolavel 8 linhas: Wi-Fi, Clima, Data e hora, Tela, Ajustes, Toque (teste), Info,
  Reset fabrica em vermelho
- Ajustes: helper uiRowSw() (card 18px + switch 26x16 embutido); secao BLOQUEIO com slider
  full width (knob claro, indicador ACC), linhas WiFi auto / Bip ao tocar / 2 toques,
  rodape versao

### HTML-first
- preview/teste_padrao.html, teste_home.html (sessao anterior)
- preview/teste_pers_config.html, teste_ajustes.html (esta sessao)

### Compilacao: 0 erros, 59% flash / 68% RAM (2 builds seguidos)

### Proximas telas no mesmo padrao (redRow/uiRowSw): Wi-Fi, Data/Hora, Clima
### Pendente validar no pulso: wake 2-toques, face Padrao, home chips, Pers/Config/Ajustes,
  SD, espelho, MidiaBT, Doom


## Sessao 2026-11-09b: Face padrao = foto do user + varredura geral

### Face PADRAO (lockStyle 2) = EXATAMENTE a foto
- HH "12" topo-esq (F_BIG 28) e MM "59" meio-dir (alinhado a direita): layout da foto
- rastros que bugavam: mudanca anterior de HH/MM empilhados a direita foi REVERTIDA
- aneis: geometria original (44/38/32 concen centro 26,132), espessura 4/3/2, ROT 270^ (inicio no
  topo), cores do user: passos lime 0xCCFF00, km verde 0x00FF44, cal vermelho 0xFF2200
- topo: temperatua TL (SO o numero "17^", largura 44) + cluster TR [wifi][BT][98%][icone]
- bateria verde 0x00FF66 (vermelha < 20%); wifi UI_OKC quando conectado; BT azul/cinza
- data: so aparece SEM clima E offline (foto nao mostra)
- hint "toque p/ destravar": agora OCULTO por padrao; aparece SOMENTE quando o guard 2-toques
  esta armado (tela limpa como na foto); wakeHint on/off mostra/esconde
- MIGRACAO "lock2" no core.h: 1o boot poe lockStyle=2 (o device mostrava a face Digital: por
  isso o layout estava "errado" no hardware)

### Lightsleep = tela preta real + RTC
- doSleep: SLPIN (0x10) no painel ST7735 via tft.sendCommand (tela 100% preta); RTC interno
  segue contando horario em lightsleep (nada a reconfigurar)
- SLPOUT (0x11) + delay 120ms no despertar (exigencia ST7735)
- fix de build: sendCommand NULL era ambiguio -> buffer de 0 bytes

### Varredura geral (audit viva ui.h/core.h/ino; apps.h = codigo morto, sem include)
- TOOLBOX -> lista redRow rolavel (18 itens, icons LVGL); grade 61x13 removida
- bug corrigido: fakeApApply montava "AP falso: on/off" e nunca aplicava no label
- TELA -> Redmi: secao BRILHO, slider (knob claro), 2x2 botoes 18px, redRow Personalizacao
- CLIMA -> temp grande no topo (y4), desc/det centradas largura 118, coords 1 linha de 22px,
  Atualizar full-width 118 (antes "17^ nublado" estourava como na foto 1)
- DataHora/Saude/Jogos/Calc/Remote/Moeda/TVBgone/Info/Teste: auditados, funcionais, mantidos
- BadUSB/BadBLE "stub" estavam em apps.h MORTO (nunca incluido) - BadBLE real existe e esta
  ligado (ui.h bbHid com runner /duck.txt); apps.h deixado sem uso de proposito

### Compilacoes nesta sessao: 0 erros (4 builds seguidos, 59% flash / 68% RAM)

### Pendente no pulso (validar): face = foto, sleep preto + 2 toques, wifi/bt no topo,
  toolbox lista, tela/clima novos, Power Off chip (menu) apagando tela


## Sessao 2026-11-09c: upload COM8 FALHOU -> cleanup + pesquisa web

### Upload
- compile c/ build-path fixo: 0 erros (59% / 68%); artefatos em Temp/opencode/build_sw
- upload COM8: "Could not open COM8 ... PermissionError(13) 'dispositivo nao funcionando'"
  -> PLACA NAO FLASHADA (desistido conforme pedido; binario pronto p/ quando voltar)
- causa provavel (pesquisa): driver CH343/CH9102 do Windows bugado (esptool #879):
  reinstalar driver WCH CH343SER >= 3.9; alternativas: trocar cabo/porta, replugar,
  botao BOOT apertado ao plugar; conferir se Monitor Serial esta fechado

### Cleanup
- apps.h (29.7 KB) REMOVIDO do projeto (era codigo morto, nunca incluido; tinha stubs que
  confundiam: BadUSB/BadBLE "prox. iter"). Backup: Temp/opencode/apps.h.bak
- pos-remocao: 0 erros; html_parser.c/.h LIVE (compilados e usados por Google/Imagens)

### Pesquisa web -> melhorias (qoitech.com 2026-08, lvgl docs, esptool issues)
- ESP32-S3 light sleep: Saving gigante (2.88mA -> 692uA) vem de CONFIG_ESP_SLEEP_POWER_DOWN_FLASH
  (sdkconfig do IDF) - NAO acessivel pelo Arduino IDE (core pre-compilado). Documentado aqui
  p/ futuro se migrar p/ IDF puro.
- NOSSo padrao de sleep esta conforme melhores praticas do artigo: radio off antes do
  lightsleep (doSLeep so' chamado com wifi off), painel SLPIN, wake por IRQ touch, sem
  gpio_deep_sleep_hold (o artigo mostra que hold em pino nao-RTC causava 461uA->41.6mA bug).
- LVGL: mantido full_refresh=1 + fb completo 128x160 (40KB) - DELIBERADO: o framebuffer
  inteiro alimenta o Espelho (gFrame). Partial refresh quebraria o recurso.
- disp_flush: startWrite/setAddrWindow/writePixels/endWrite - correto p/ Adafruit; sem
  padrao de buffer duplo necessario (tela pequena).


## Sessao 2026-11-09d: plano aprovado - P1 upload + P2 face/anels

### P1: upload de Arquivos corrigido (smartwatch.ino)
- UPLOAD_FILE_ABORTED agora tratado: fecha arquivo, remove partial do SD, reseta estado
- falha real de gravacao responde 500 ao navegador ("ERRO: falha ao gravar no SD") e a barra
  no TFT fica vermelha "ERRO: falha no SD" (antes respondia 200 OK mesmo errado)
- write() verifica bytes gravados (SD cheio/erro detectado)
- collectHeaders("X-File-Size") antes do begin (sem isso hasHeader nunca pega, issue #3451)
- timeout do servidor (5min) agora RENOVA a cada chunk recebido (uploads longos nao morrem)
- dica na tela do servidor: "(fique na rede WearSend)" - celulares descartam rede sem internet

### P2: face PADRAO = foto (ui.h)
- MM "59" desceu p/ y116 (centro ~130 = centro dos aneis, foto); build + updater (seguro
  ao alternar faces)
- "BT" agora so aparece quando btActive() (azul); off = oculto (foto nao tem BT apagado)
- anels conferidos c/ o user: lime 0xCCFF00 = passos (externo), verde 0x00FF44 = km (medio),
  vermelho 0xFF2200 = calorias (interno) - geometria 44/38/32 (4,110) mantida

### BUGS DE VALOR achados ao EXAMINAR os anels (era a parte "todos os valores"):
- km: usava steps*0.7 num range 0..500 -> cravava no MAXIMO com >=714 passos.
  Corrigido p/ steps*0.07 (km*100): 6000 passos = 4.2 km de arco 0..5.00
- cal: tinha um *100 sobrando -> cravava no MAXIMO com >=75 passos.
  Corrigido p/ steps*0.04: 6000 passos = 240 cal (range 0..300)

### Estado
- user compila manualmente (sem compile nesta sessao); binarios anteriores em
  Temp/opencode/build_sw ficaram desatualizados p/ flash - regenerar apos validar


## Sessao 2026-11-09e: Face PADRAO refazida do zero = foto 2 (Redmi Watch 5)

### buildLockS (ui.h ~2010-2092) - PADRAO refeito, idêntico à foto 2
- Fundo preto puro (0x000000), sem tema espacial, sem estrelas, sem data
- "12" (HH) topo-esq, F_BIG 28, y36, alinhado esquerda
- "59" (MM) ao NIVEL dos aneis (y114), direito dos aneis, alinhado direita
- Topo-dir: [wifi azul UI_OKC se conectado] [pct bateria branco] [bateria verde 0x00FF66]
- Anéis concêntricos inf-esq (centro ~26,132):
  externo lime 0xCCFF00 (passos, 0..6000, largura 4)
  medio verde 0x00FF44 (km, 0..500 = steps*0.07, largura 3)
  interno vermelho 0xFF2200 (cal, 0..300 = steps*0.04, largura 2)
  bg dos aneis: 0x0A0A0A (quase preto); todos SEMPRE visiveis (sem HIDDEN)
- BT, data, temperatura: NUNCA aparecem no padrao (foto 2 sem eles)
- lockBat -> lockBatPct (renomeado); drawBatIco recebe cor (padrao verde 0x00FF66)

### lockUpdate padrao (ui.h ~2253-2298) - reescrito
- cluster topo-dir atualizado: wifi mostra só se conectado; pct e icone bateria sempre
- rings sempre visiveis com valores corrigidos (km x0.07, cal x0.04)
- BT/data/wx forçados HIDDEN no padrao

### Upload P1 (smartwatch.ino) - mantido da sessao anterior
- UPLOAD_FILE_ABORTED, resposta 500, collectHeaders, timeout renovado, barra ERRO

### Nao compilei ainda - usuario compila manualmente


## Sessao 2026-11-09f: restyle continuado - DataHora, Teste, Saude cores

### DataHora (ui.h ~3055-3085)
- espaco melhorado: row*36 (era 34), cx 8/48/88 (era 6/46/86)
- botoes Aplicar/NTP em baixo (y118)
- texto morto "toque p/ avancar" removido

### Teste (ui.h ~3460-3474)
- espaco melhorado: i * 22 (era i * 15), inicio y50 (era y48)

### tbBtn (ui.h ~3476) - codigo morto
- funcao definida mas nunca chamada (dead code); tentada remocao, mantida inofensiva

### Saude (ui.h ~4076-4108)
- cores dos aneis alinhadas com a face padrao / foto 2:
  passos: lime 0xCCFF00 (era UI_ACC)
  km: verde 0x00FF44 (era 0x3AD07F)
  cal: vermelho 0xFF2200 (era 0xFFA726)
- legenda de baixo: cores correspondentes aos aneis

### Nao compilei ainda - usuario compila manualmente


## Sessao 2026-11-09g: auditoria completa de todas as telas

### Telas revisadas e status
| Tela | Funcao | Status | Nota |
|---|---|---|---|
| Lock/face (padrao) | Relogio principal | ✅ REFEITO | Idêntico à foto 2: HH topo-esq, MM nível dos aneis, cluster [wifi][pct][bateria], anéis concêntricos |
| Home | Menu apps (chips) | ✅ OK | appChip circular 36px, estilo Redmi |
| Config | Menu principal | ✅ OK | redRow lista |
| Ajustes | Brilho, switches | ✅ OK | Slider + uiRowSw |
| Pers | Cor, estilo, tema | ✅ OK | Chips circulares, segmented, switch |
| Toolbox | Utilitarios | ✅ OK | redRow lista |
| Clima | Tempo Open-Meteo | ✅ OK | Reorganizado |
| Tela | Brilho/calibrar | ✅ OK | Slider + botoes |
| DataHora | Ajuste RTC | ✅ | Espacamento melhorado |
| Teste | Diagnostico | ✅ | Espacamento melhorado |
| Saude | Passos/km/cal | ✅ | Cores alinhadas com a face |
| Info | Sistema | OK | Lista simples |
| Wifi | Scan/conectar | OK | Botoes + lista rolavel |
| Arquivos | SD + upload | OK | uiRow + lista |
| Jogos | 13 jogos | OK | gameCardSq cards arredondados |
| Crono | Cronometro | OK | Layout limpo |
| Moeda | Cara/coroa | OK | Simples |
| Dado | Dado | OK | Simples |
| Remote | IR controle | OK | remoteRow por botao |

### Correcoes de valores (bugs achados ao examinar)
- Face padrao: km era steps*0.07 (range 0..500) — OK agora
- Face padrao: cal era steps*0.04 (range 0..300) — OK agora
- Saude: cores alinhadas com a face (lime/verde/vermelho)

### Upload P1 (smartwatch.ino) - mantido
- UPLOAD_FILE_ABORTED, resposta 500, collectHeaders, timeout renovado, barra ERRO

### Nao compilei ainda - usuario compila manualmente


## Sessao 2026-11-09h: FINALIZADO - tudo implementado (sem compile; usuario compila)

### P1: upload de Arquivos (smartwatch.ino ~124-161)
- UPLOAD_FILE_ABORTED tratado; resposta 500 em falha; collectHeaders("X-File-Size");
  timeout renovado a cada chunk; barra vermelha "ERRO: falha no SD"; dica WearSend

### P2/P2b: Face PADRAO = foto 2 (Redmi Watch 5) (ui.h ~2010-2298)
- Fundo preto puro; "12" HH topo-esq (y36); "59" MM nivel dos aneis (y114)
- Topo-dir: [wifi azul se conectado] [pct bateria] [bateria verde 0x00FF66]
- Aneis concêntricos: lime=passos, verde=km, vermelho=cal (valores corrigidos: km x0.07, cal x0.04)
- BT/data/temperatura: NAO aparecem no padrao (foto 2 sem eles)

### P3b: restyle DataHora + Teste (espacamento); tbBtn = codigo morto mantido

### P4: Moeda com animacao de giro (ui.h ~3844-3870)
- Moeda circular dourada gira 0.6s (largura 36->0->36, ease_in_out)
- No ponto mais fino revela CARA (UI_ACC) ou CORAO (0xFFA726)
- Trava toque durante animacao; beep ao jogar

### Nao compilei ainda - usuario compila manualmente

## Sessao 2026-11-10: varredura completa da UI (Redmi Watch 6) + aneis ROTATIVOS + correcoes

### 1) Face PADRAO: aneis viraram SLIDERS ROTATIVOS de verdade (pedido do user)
- Antes os arcos eram decorativos (CLICKABLE limpo). Agora sao sliders reais:
  * lockArc(): arc CLICABLE + LV_OBJ_FLAG_ADV_HITTEST + callback LV_EVENT_HIT_TEST.
  * ringHitTest(): so' a FAIXA do arco (e o knob) e' clicavel -> toque fora do anel
    continua caindo na tela (destrava o relogio).
  * ringEventCb(): PRESSED/PRESSING/VALUE_CHANGED/RELEASED/PRESS_LOST -> gira o anel,
    mostra o valor (label '1234 passos' / '3.5 km' / '120 kcal') e da toast no fim.
  * ringApplyValue(): converte o anel girado em PASSOS (fonte unica de verdade) e
    recalcula os outros dois; grava so a chave 'steps' (cfgPut), nao a config inteira.
  * ringSyncFromSteps(): os 3 aneis sempre refletem cfg.steps.
  * gRingActive + touchTick(): girar um anel NAO destrava (soltar fora do anel destrava).
- ESCALAS alinhadas (antes o anel de km travava no maximo):
  passos 0..10000 | km 0..70 (0,1 km = passos x 0,007) | calorias 0..400 (= passos x 0,04).
  App SAUDE acompanhou: km 0..700 (7,00 km) e cal 0..400 (o loop de atualizacao usava
  6000/500/300 -> corrigido).

### 2) Bugs de estado/regra corrigidos
- TIMERS DE JOGOS: todos checavam so gInGame -> abrir o Pong, sair e abrir o Flappy
  deixava o Pong simulando por tras (bola se mexendo + toast 'VOCE VENCEU'). Agora cada
  timer valida a PROPRIA tela (lv_obj_is_valid(scr) && lv_scr_act()==scr): snake, doom,
  pong, invasores, dama, xadrez, flappy, dino, tetris, runner.
- DAMA: captura obrigatoria era calculada com as DUAS cores (bloqueava a jogada do
  jogador por causa de captura do adversario) -> agora so a cor da vez; multi-salto nao
  continuava com dama; nao dava pra trocar de peca no meio do multi-salto; a CPU nao
  continuava o multi-salto dela; faltava o caso 'sem jogada' (fim de jogo).
  Tudo corrigido + dmNovoJogo() (toque no aviso reinicia).
- XADREZ: faltava reinicio no fim de partida -> xzNovoJogo() + toque no aviso.
- PONG: nao tinha como reiniciar (so toast) -> aviso 'VOCE VENCEU!/A CPU VENCEU!' na tela
  + toque reinicia; bola/raquetes deixaram de ser clicaveis (engoliam o toque).

### 3) Layout (todos os apps revisados cabem em 128x160)
- CRONO: display nao encostava mais no cabecalho/seletor (F_MED em y=44, modo y=70,
  Iniciar/Zerar y=90, Volta/+/- y=114, voltas y=136); 'voltas' mostra a ultima (1 linha).
- MUSICA (player): nome em y=44 (DOT), tempo y=62, barra y=80, controles y=100 (28px),
  botao 'Parar' y=134; o nome nao cobria mais o botao de stop.
- MOEDA: moeda 44x44 centralizada no giro (antes saltava para a direita), resultado
  (CARA!/COROA!) em label separado (antes 'COROA' estourava dentro do circulo).
- IMAGEM (visualizador): faixas escuras no topo/base p/ ler sobre a foto + botoes
  'Voltar'/'Definir' (antes o botao saia da tela e nao havia como voltar a galeria).
- FOTOS (galeria): botao de fechar (volta a Arquivos) + faixa escura no topo.
- IA: caixa de resposta encolheu (48->38) para nao vazar do corpo do app.
- WI-FI: botoes (Scan/+Rede/NTP) desceram p/ y=62 (1px de sobreposicao com o status).
- CLIMA: linha de detalhes (vento/umidade) subiu p/ y=91 (encostava no campo de lat/lon).
- FACE ANALOGICA: data y=18, anel 96px em y=36, mostrador 88px em y=40, ponteiros
  (24/32/30), pino central reposicionado, HH:MM em y=134 - a data nao batia mais na
  barra de status e o anel nao cortava a hora.

### 4) Encoding
- ui.h tinha 2 bytes ANSI quebrados (comentarios 'leg?vel' e 'nao ? linha reta') -> corrigidos.

### Compilacao
- arduino-cli compile esp32:esp32:esp32s3 (huge_app/PSRAM): 0 erros. Flash 64% | RAM 68%.

## Sessao 2026-11-10b: simulador HTML estilo Wokwi (esquema real + tela viva)

- simulador.html (auto-contido, abre com 2 cliques) + ssets/ (arte da placa).
- PLACA: imagem real da ESP32-S3 Super Mini (espboards.dev, CC BY-NC 4.0) com os pinos
  MEDIDOS por pixel (banda de ouro -> buraco) e bateidos com o pinout oficial:
  borda esquerda 48/TX/RX/BOOT/1..7 | borda direita BAT/5V/GND/3V3/13/12/11/10/9/8 |
  pads traseiros 34..39 (38/37/34 coluna esq. e 36/35/39 coluna dir.).
- FIAÇÃO: barramentos +3V3/GND no topo e SPI (SCK/MOSI/MISO) embaixo, taps por modulo,
  canais unicos por fio (sem sobreposicao), caminhos que contornam a placa.
  Validado por geometria no navegador: 36/38 fios ligam pino->pino correto.
  2 bugs achados e corrigidos no meio do caminho: touch sem pino IRQ e SDA/SCL do MPU
  parando 40 px antes do pad traseiro.
- TELA: o DOM real de 128x160 (pixels exatos) dentro do modulo Display via foreignObject,
  1:1 (o SVG nao e escalado). Interativa: destrava, gira aneis, abre apps.
- Modulos com painel que altera a simulacao + console serial do firmware.

## Sessao 2026-11-10c: menu HyperOS, temperatura na face, Arquivos estilo Explorer e ANTES DO TRAVAMENTO

### 1) Face padrao: temperatura no topo-esquerdo
- Formato "31 C (sol) Ensolarado" no CANTO SUPERIOR ESQUERDO, no MESMO TAMANHO da % da
  bateria (F_SM 12): temperatura + icone desenhado (sol/nuvem/chuva, 12x12) + descricao.
- O cluster (BT + % + bateria) desceu para a 2a linha (y=17) e o "12" foi para y=30
  (igual a proporcao da foto).
- gWxTemp/gWxDesc/gWxKind + helper wxApplyData(): o clima em 2o plano (a cada 5 min)
  tambem preenche a temperatura da face, nao so a tela Clima.

### 2) Menu de apps no estilo da foto (3x3, icones pixel-art + cor por app)
  power(vermelho) | sliders(indigo=Ajustes) | G(vinho=Google)
  gamepad(verde=Jogos) | foguete(dark=Toolbox) | IA(preto)
  pasta(amarelo=Arquivos) | cadeado(castanho=Bloquear) | engrenagem(prata=Config)
- Matematica e Saude foram para o menu Config (continuam acessiveis).
- icones pixel-art novos: Power, Ajustes, Rocket, Lock, Gamepad.

### 3) Bluetooth/Wi-Fi: o simbolo da face (LV_SYMBOL_BLUETOOTH) em TODAS as telas
- addStatus trocou o texto "BT" pelo simbolo (azul quando ativo, cinza quando off).

### 4) Arquivos: explorador estilo Windows
- Barra de ferramentas (atualizar / subir / fotos / enviar / info / converter FAT16).
- Barra de endereco ("SD CARD > /DCIM"), colunas Nome | Tamanho, lista com icone por
  tipo (pasta/imagem/audio/doc), barra de status ("N itens  M").
- Pastas navegaveis (entrar/sobe) - novo sdListDir() com pastas primeiro.
- Toque longo = apagar (modal de confirmacao).

### 5) ANTES DO TRAVAMENTO (bugs reais corrigidos)
- VAZAMENTO DE PSRAM: cada icone (icoCanvas) alocava 512 B na PSRAM e NUNCA liberava.
  Abrir Jogos/Home varias vezes acabava com a PSRAM -> travamento. Agora o buffer e
  liberado no LV_EVENT_DELETE do canvas (idem nos icones de arquivo).
- ENVIO DE IR bloqueava a UI ~110 ms por botao -> agora vai para uma fila + task no
  nucleo 0 (irSendAsync). Vale para marcas, controle aprendido, NEC e TV-B-Gone.
- LEITURA DO WAV do SD bloqueava ate 1,4 s -> agora carrega em task (audLoadTask) e
  comeca a tocar quando chega; a UI mostra "carregando..." e nunca congela.
- beepSpeaker(): ledcDetach no canal do SPK brigava com o player (podia derrubar o
  audio task). Agora nao desanexa o canal e nao bipa durante a musica.
- Formatar o SD (FAT16) agora pede confirmacao em 2 toques (levava varios segundos).

### Compilacao
- 0 erros, 0 warnings. Flash 64% (2.020.602 B) | RAM 69% (227.352 B).

## Sessao 2026-11-10d: CORRECAO DEFINITIVA DOS TRAVAMENTOS (thread-safety + barramento SPI)

O sintoma "apertoi um botao e o sistema travou" tinha 3 causas REAIS de baixo nivel
(nao eram so "demora"): todas corrigidas nesta sessao.

### 1) LVGL chamado de outra task (risco de corrupcao de memoria -> reboot/travamento)
- A task de carga do WAV (core 0) chamava sdEndSession(), que fazia lv_refr_now().
  O LVGL nao e thread-safe: dois contextos mexendo na fila de eventos = estado
  corrompido (a tela some, os botoes param de responder ou o ESP reinicia).
- Novo: gLvTaskH (task do loop) + lvCtx(). sdEndSession() so redesenha quando
  esta na task do LVGL; de outra task ele apenas marca gSdRefreshPending e o
  loop() aplica o redesenho (Smartwatch.ino).

### 2) TFT e SD disputando o mesmo barramento SPI ao mesmo tempo
- O flush do TFT (task do LVGL) e a leitura do SD (task de audio, core 0) usavam
  o mesmo tftSPI sem exclusao ->bytes corrompidos no display e no arquivo.
- Novo: mutex gBusMtx (core.h). disp_flush() trava o mutex durante a escrita;
  sdBeginSession()/sdEndSession() e sdProbe() travam durante a sessao do SD.
  O display continua pausado (gSdPause) durante a sessao, entao nao trava a UI.

### 3) Montagem do SD travando a UI por 200..500 ms
- sdTryBegin() fazia 5 tentativas (com 80 ms de espera) sempre. Sem cartao, cada
  sessao de SD segurava a tela por ~0,5 s.
- Novo: sdTryBegin(int tries): 2 tentativas no caminho da UI (falha rapida),
  5 no diagnostico (sdProbe). O Explorer tambem passou a ler a lista DEPOIS de
  pintar a tela (timer de 40 ms), com guarda lv_scr_act() == scrArquivo para
  nunca tocar numa tela ja destruida.

### 4) Others
- uiServerStart(): para o player e espera a leitura do SD terminar antes de
  montar o cartao para o upload (audCancel/audLoadBusy).
- audPlay(): para a faixa atual antes de carregar a proxima; audStop() cancela
  a carga em andamento (audCancel).
- arFatCb(): o ESP32-SD nao converte FAT -> a funcao so explica o caminho
  (antes fingia que ia formatar e exigia 2 toques).
- arStat (barra de status do Explorer) agora trunca com "..." em vez de invadir
  a lista.

### Runbook
- Criado .claude/napkin.md na pasta do projeto (regras de ouro do repo:
  pasta oficial, ordem de sincronizacao, gotchas de LVGL/PowerShell, ordens do
  usuario).

### Compilacao final (pasta oficial Documents\Smartwatch)
- 0 erros, 0 warnings. Flash 64% (2.021.034 B) | RAM 69% (227.360 B).

## Sessao 2026-11-10e: pedido do usuario (menu, face, SD, NTP, IR)

### 1) Home 3x3 refeito conforme instrucao mais recente
- Ordem: Power (lightsleep) / Matematica / Google / Jogos / Saude / IA /
  Arquivos / Toolbox / Configuracoes.
- Novos icones pixel-art: Matematica (calculadora), Saude (tenis),
  Toolbox (briefcase), Google (G colorido).

### 2) Face padrao: 12:59 grande + bateria colorida
- `lockUpdate` mostra HH:MM em campo grande e centralizado.
- Percentual e icone de bateria: vermelho <20%, verde >=20%.

### 3) Wi-Fi -> NTP automatico de Campo Grande/MS
- Fuso corrigido para AMT **UTC-4 fixo** (sem DST): `CLOCK_TZ_OFFSET_SEC`.
- Toda conexao STA (boot / app Wi-Fi / fim do servidor) chama `uiNtpAutoRequest()`.
- Re-sincroniza a cada 6h enquanto conectado.
- Sucesso comprovado pelo callback do SNTP (`gNtpSyncCount`), nao por `timeOk()`
  (que tambem vale para a hora restaurada do SPIFFS).

### 4) SD: pastas por tipo + nomes 8.3
- Upload grava em `/FOTOS`, `/VIDEOS`, `/MUSICAS`.
- Nome sanitizado para 8.3 (A-Z/0-9/_/-), extensao ate 3 chars e sufixo em colisao.
- `audList()` e galeria de fotos guardam/abrem **caminho completo**.
- Video abre tela propria com nome/tamanho: **reproducao de video ainda NAO existe**
  (sem decoder no relogio); o arquivo fica armazenado para copiar/assistir no PC.

### 5) IR / TV-B-Gone
- Carrier trocado para LEDC (38 kHz) com saida invertida para a fiacao do usuario:
  3V3 -> anodo do LED IR, GND -> GPIO3 (catodo).
- SIRC (Sony) e RC5 corrigidos para usar a portadora de 40k/36k e Manchester real.
- Replay "cru" agora descarta a primeira amostra (gap/lixo) antes de alternar MARK/SPACE.
- TV-B-Gone refeito com o banco canonico `tvbg_codes.h` (NA=137, EU=139),
  decoder MSB-first e pausa canonica de 205 ms entre POWER codes.

### 6) Varredura / correcoes de impacto
- Corrigido: SD 8.3, musica/fotos nas pastas novas, `TVBG_EU_N` 140->139,
  bip do toque (LEDC), re-sync NTP periodico, subir nivel no Explorer, cache do SD.
- Diante do pedido mais recente, o Home e a face Padrao seguem a NOVA ordem/12:59,
  mesmo onde o STATUS antigo ou o simulador anterior mostravam outro layout.

## Sessao 2026-11-11b: upload do SD + estouro de PSRAM + controles do DOOM

### 1) Upload do SD ("falha no SD" com o cartao detectado)
- `webSdEnsure()`: monta o cartao (`sdTryBegin`) antes de listar/gravar; o
  `sdProbe()` da tela Arquivos termina com `SD.end()`, e o servidor herdava
  esse estado (listar caia no cache, gravar falhava).
- `webSdStart()` monta com `gSdPause` + `busLock` e loga `[sd] servidor pronto`.
- Upload: `mkdir` com fallback (`mkdir || exists`), 2a tentativa de `SD.open`
  sem desmontar um cartao que esta' funcionando, gravacao em laco (fwrite pode
  gravar menos), `flush` + conferencia de tamanho no END, e logs `[up] ...`
  (mount/card/mkdir/open/total, gravacao curta, tamanho final).
- Diagnostico novo: `webSdWriteTest()` grava/apaga `/WRTEST.TMP` no start do
  servidor e separa "monta" de "grava"; `/st` devolve `rw|ro|no` + MB livres e
  a pagina WearSend mostra "cartao OK / SEM GRAVACAO / nao montou".
- Erros especificos: 2 = sem espaco (confere `totalBytes-usedBytes` antes de
  comecar e responde HTTP 507), 3 = sem permissao de gravacao, 4 = nao montou.
  No TFT: "SEM ESPACO no SD", "SD sem gravacao", "SD nao montou".

### 2) PSRAM estourava abrindo apps em rajada
- `psNeed()` reserva 24 KB de folga.
- buffer de audio (1,4 MB) devolvido ~2 s depois de sair do Player (antes: 90 s).
- `psCleanup` limpa registros de telas mortas e considera ocioso na hora;
  `psResStillMapped` impede liberar buffer que outra tela viva ainda usa.
- `navP()`: guarda de 200 ms contra rajada (descarta a tela duplicada em vez
  de empilhar telas) + `psCleanup(true)` antes de entrar.
- `buildDoom()`: se o framebuffer nao alocar, apaga a tela e avisa (antes
  deixava a tela criada e nao carregada, vazando).
- Player/Google/Xadrez/Doom continuam no gerenciador; jogos liberam em 300 ms.

### 3) Controles do DOOM (sem mudar graficos/jogabilidade/cenario/UI)
- Antes o tiro saia no `LV_EVENT_PRESSED`: arrastar para andar/mirar tambem
  disparava. Agora: `PRESSED` guarda o ponto, `RELEASED` so' atira se o dedo
  andou <= 7 px e o toque durou <= 600 ms (arrasto = andar/mirar, toque = tiro).
- Cadencia de 220 ms por tiro; "FASE LIMPA" continua com 6 abates e o toque
  apos o fim reinicia a fase. Mapa, inimigos, HUD e mira inalterados.
- Obs: o DOOM atual nao tem portoes/notas de passagem; a fase termina ao
  abater os 6 inimigos (mantido como estava).

### 4) Log `ledc: requested frequency 200000 ... div_param=0`
- 200 kHz em 8 bits nao cabe no clock LEDC de 40 MHz (max ~156 kHz). O erro
  vinha do restore da portadora do SPK (`spkCarrierEnsure`) depois de cada beep.
- Agora `SPK_CARRIER_HZ 100000UL` com fallback 50 kHz; `beepShort()` e
  `beepSpeaker()` devolvem a portadora chamando `spkCarrierEnsure()`.

### 5) `[sd] servidor: cartao nao montou` (cartao presente)
- `webSdStart()` monta o SD **antes** de subir o AP: o pico de corrente do radio
  derrubava o mount. Log novo: `[sd] servidor: mount=1 grava=1 card=3`.
- Sem cartao o servidor mesmo assim sobe: a pagina avisa e `/list`, `/st` e
  `/up` tentam remontar.
- `webSdMount()`: CS em repouso (HIGH) + 60 ms + ate 6 tentativas com 150 ms.
- `sdTryBegin()`: metade das tentativas em 4 MHz e a outra metade em 1 MHz
  (cartoes clones/mau contato as vezes so inicializam devagar).
- Tela do modo servidor mostra `SD: OK` ou `SD: SEM CARTAO`.

### 6) `mount=0 card=0` no servidor x explorador de arquivos OK
- Sintoma: o explorador le os arquivos do SD (mesmo `sdTryBegin`), mas ao abrir
  o WearSend o cartao nao monta (`[sd] servidor: mount=0 grava=0 card=0`).
- Diferenca entre os dois caminhos: o servidor montava com o radio Wi-Fi ativo
  (STA conectado do boot/NTP ou o AP subindo). O pico/ruido de corrente no
  cartao derruba a inicializacao SPI.
- Fix: `webSdStart()` faz `WiFi.disconnect(true)` + `WiFi.mode(WIFI_OFF)` +
  400 ms de folga **antes** de tocar no cartao; so' depois sobe o AP.
- `webSdMount(tries, gap)`: CS como OUTPUT em HIGH, 120 ms de repouso, e
  segunda rodada (400 ms + 4 tentativas com 300 ms) se a primeira falhar.
  Logs: `[sd] mount ok (tentativa N)` / `[sd] mount falhou apos N tentativas`.
- `sdTryBegin()` terminou de cair para 1 MHz nas ultimas tentativas.

### 7) Causa raiz do `mount=0 card=0`: remontagem pos-`SD.end()`
- Evidencia: o log nao tinha NENHUM erro do IDF (`sdmmc_cmd`/`vfs_fat`), ou seja
  o `SD.begin()` falhava antes de falar com o cartao. O explorador le os
  arquivos porque montou ANTES do primeiro `SD.end()`.
- No core 3.3.12, depois de `SD.end()` o `SD.begin()` volta false: a
  inicializacao (drive/FATFS/SPI host) nao e' refeita de forma confiavel.
- Fix: **o cartao passa a ficar montado o boot inteiro**. Removido `SD.end()`
  de `sdEndSession()` (ui.h), do fim do `sdProbe()` (core.h), de `webSdStop()`
  (Smartwatch.ino) e de `uiServerStop()` (ui.h). Sessoes agora so' soltam o CS.
- Consequencias: `webSdStart()` acha o cartao montado (mount=1 na hora),
  upload grava, e o explorador continua funcionando. O barramento segue
  serializado (`busLock`/`gSdPause`), o cartao fica em repouso com CS alto.
- Mantidos: `SD.end()` apenas nos caminhos de falha de inicializacao
  (`sdTryBegin` e cartao montado mas `cardType()==CARD_NONE`).

### 8) `mount=1 grava=0 card=3` (cartao montado, escrita falha)
- `webSdWriteTest()` agora loga o passo e o `errno`:
  `[sd] wr: open falhou errno=N total=... used=...` ou `[sd] wr: ok write=2 size=2 remove=1`.
- Novo endpoint **`/dbg`** na pagina WearSend: mount, card, totalMB/usedMB,
  rootOpen, mkdir, openW+errno, write+errno, size, remove, wrFail, heap.
- Upload nao para mais no teste de escrita: ele tenta `mkdir`+`open` de verdade
  e so' classifica `fail=3` (sem gravacao) se o teste tambem falhou; `fail=2`
  = sem espaco (checado antes, com log de livre x necessario).
- `SD.begin(... , 8)` -> 8 handles de arquivo (era 5): upload + player + lista.
- Hipoteses a confirmar com `/dbg`: cartao cheio (usedMB ~ totalMB), travado
  por adaptador, ou falha de escrita SPI (errno/I-O).

### 9) Escrita derrubava o cartao (errno=5/EIO, total=0 depois)
- Sintoma: `mount=1`, leitura ok, mas o primeiro `openW` falha com `errno=5`
  e dali em diante `total=0 used=0` (o cartao sai do estado SPI e para de
  responder; so' reinicializando).
- `SD_SPI_HZ = 1 MHz` (era 4 MHz): o modo conservador que a maioria dos
  cartoes/modulos aceita para escrever. Reads continuam corretos.
- `webSdRecover()`: `SD.end()` + 300 ms + init de novo (1 MHz; 400 kHz como
  ultimo recurso). Chamado quando o teste de escrita falha (no start do
  servidor, no `/st` e no `/dbg`), para o cartao nao ficar morto.
- O teste de escrita roda **1x por boot** (`gSdWrKnown`): repetir o teste
  derrubava o cartao de novo. O upload nao depende do teste: tenta gravar de
  verdade e classifica (`fail=1` erro, `fail=2` sem espaco, `fail=3` sem
  gravacao confirmada, `fail=4` nao montou).
- `SD.begin(..., 8)` = 8 handles de arquivo.

### 10) Pesquisa: escrita no SD (lib do core 3.3.12) + teste isolado
- Caminho de escrita da lib: `ff_sd_write` -> `sdWriteSector` (CMD24) ->
  `sdWriteBytes` (token 0xFE + 512 B + CRC16) -> resposta -> `SEND_STATUS`.
  Com `CRC_ON_OFF=1` no init, o CRC dos dados e' conferido. Conferido no
  codigo local: `SPIClass::write16/transfer16` usam a mesma ordem (MSB), o
  `AcquireSPI` aplica `SPISettings(card->frequency)` em toda operacao e o
  `ff_sd_initialize` manda 160 clocks com CS alto antes do CMD0. Ou seja:
  **nao ha bug de caminho de escrita no firmware**.
- `SD/examples/SD_Test` faz exatamente o mesmo (`SD.begin()` + `FILE_WRITE`)
  e escreve 1 MB de imediato. Se o cartao grava la' e nao grava aqui, a
  diferenca e' o barramento compartilhado com o TFT.
- Issues consultadas: espressif/arduino-esp32#5998 (IO errors na SD lib,
  fechada pelo PR #6162, que corrige **mount**, nao escrita), forum.arduino.cc
  978084 (mesmo sintoma resolvido **reformatando** o cartao) e esp32.com
  t=17593 (caminho sem barra).
- Sintoma real: `mount=1`, leitura ok, **primeira** escrita -> `errno=5`
  (EIO) e o cartao para de responder ate' o power cycle (CMD0 sem resposta,
  `recover=0`). Isso e' nivel fisico/cartao, nao FATFS.
- Mudancas:
  - `SD_SPI_HZ = 400 kHz` (era 1 MHz): modo da especificacao, o mais
    tolerante a fio longo / shifter fraco. Se gravar, subir depois.
  - `tsReadCh()` agora respeita `gSdPause` e pega `busLock(30)`: o XPT2046
    divide SCK/MOSI/MISO com o SD e podia clockar o bus no meio de uma
    transferencia do cartao.
  - `/dbg` agora reinicializa o cartao quando a escrita falha e responde
    `rec=1 open2=.. write2=.. remove2=..` (2a tentativa a 400 kHz).
- **`teste_sd/teste_sd.ino`** (novo): sketch minimo baseado no `SD_Test`,
  mesmos pinos (9/34/10/39), sem TFT/Wi-Fi/LVGL. Testa 400 kHz e 4 MHz com
  gravar+ler+comparar. Se falhar ali tambem -> hardware/cartao (modulo sem
  capacitor, alimentacao fraca, cartao fora de spec em modo SPI).

### 11) Referencias de midia (imagem/video) recebidas
- ESP32_JPEG (Espressif) e vPlayer documentados em `docs/midia.md`:
  decoder JPEG rapido com escala/rotacao e saida `RGB565_BE` direto no TFT,
  buffer alinhado a 16 B (`heap_caps_aligned_alloc`), frame 128x160 = 40.960 B.
  Video no ESP32-S3 nao tem decoder de hardware: e' **MJPEG** (vPlayer usa
  ffmpeg p/ gerar `.mjpeg` e toca de SD em MMC 4 bits).
- Limite pratico do nosso SD em SPI (frame de 6 KB): 400 kHz ~7 fps,
  1 MHz ~18 fps, 4 MHz sobra. Video exige o SD acima de 1 MHz.
- Comando ffmpeg p/ 128x160 @ 12 fps e plano do player em `docs/midia.md`
  (ainda NAO implementado).

### 12) Teste no exemplo PURO (SD_Test, sem TFT/Wi-Fi/LVGL) — falhou tambem
- Log do usuario: `SDSC` / `Card Mount Failed` / `SD Card Size: 0MB`.
- Leitura: a falha e' de **identificacao**, nao de escrita. Cartao de 16 GB
  tem de sair **SDHC**; `SDSC` + `0MB` significa que CMD8/CSD nao voltaram
  (ou corrompidos): a lib caiu no caminho "SD v1", que endereca por byte
  (`sector << 9`) -> a leitura da particao vai para o setor errado ->
  `f_mount` falha -> `Card Mount Failed`.
- Consequencia: **firmware inocentado**. Mesmos pinos 9/34/10/39, sem TFT,
  sem LVGL, sem Wi-Fi, uma unica tentativa de mount -> falha igual. O nosso
  firmware so' monta porque tenta 5-6x com folga (e mesmo assim a 1a escrita
  derruba o cartao): as duas coisas juntas sao sintoma de ligacao marginal.
- Suspeitos por custo: (1) alimentacao do modulo — se for o modulo azul com
  AMS1117, alimentar o VCC com 3V3 entrega ~2,3 V ao cartao (leitura passa,
  gravacao cai e o cartao trava ate' o power cycle); ligar o VCC em 5 V.
  (2) capacitor 100 uF + 100 nF no slot. (3) fios longos/mau contato. (4)
  cartao fora de spec.
- Prova barata: mover o fio de VCC do modulo para 5 V (se tiver regulador)
  e refazer o SD_Test. Esperado passar a imprimir `SD Card Type: SDHC` e
  `SD Card Size: ~14910MB`.
  CORRECAO 2026-11-11: o modulo do usuario e' o passivo de 6 pinos
  ("Alimentacao: 3,3V"), NAO pode ir a 5 V — 5 V no VCC chega ao cartao via
  shifter a transistor e queima o cartao. Sem regulador proprio. Fica em 3V3
  mesmo (capacitor 100 uF perto do slot + medir 3V3 sob carga). E o usuario
  confirma que o MESMO modulo ja funcionou em outros projetos: o modulo e a
  ligacao estao ok. O que mudou agora e' o CARTAO (16 GB classe 10, possivel
  falsificado, e/ou com FS sujo pelas gravacoes que falharam aqui:
  `f_open` chegava a escrever antes do EIO).
- Acao mais barata e provavel: **formatar o cartao no PC em FAT32 (unidade
  de alocacao 32 KB), formato completo** — nao exFAT (o FATFS do core nao
  monta exFAT -> `Card Mount Failed`). Depois rodar o SD_Test puro de novo.

### 13) RESOLVIDO: era o cartao (exFAT/FAT sujo) — e o barramento agora
- Usuario testou: formatou em FAT32/32 KB e passou a funcionar (mount + write).
- Ajustes feitos enquanto isso seguem valendo e agora o foco virou o
  **barramento compartilhado** (TFT/SD/touch nos mesmos SCK=9/MOSI=10/MISO=34):
  - `gBusMtx` agora e' **recursivo** (`xSemaphoreCreateRecursiveMutex` +
    `xSemaphoreTakeRecursive/GiveRecursive`). Antes era mutex simples: um
    `busLock()` dentro de outro (ex.: `webSdEnsure()` chamado dentro do fluxo
    do upload) estourava o timeout de 3 s, devolvia false e o `busUnlock()`
    **soltava o lock de fora antes da hora** — a escrita do upload corria sem
    protecao justamente no momento critico.
  - `webSdLoop()` trava o barramento durante **toda** a requisicao HTTP
    (`sdWeb.handleClient()` + `webServBar()`), e nao so' no teste de escrita.
  - `tsReadCh()` ja' respeitava `gSdPause` e pegava `busLock` (touch nao
    clocka no meio de um setor do SD).
  - `uiServerStart()` ja' chama `audStop()` e espera `audLoadBusy` — a leitura
    da faixa nao corre junto com o upload.
- Proximo passo de velocidade: com a escrita funcionando, subir `SD_SPI_HZ`
  de 400 kHz para 1 MHz (leituras) e validar; 4 MHz e' o necessario p/ o
  player MJPEG (ver `docs/midia.md`).
- Retorno 2026-11-11 (2): usuario compilou com o mutex recursivo e o log veio
  `[sd] mount falhou apos 6 tentativas` / `mount=0 grava=0 card=0` no boot,
  **sem nenhum erro do IDF** (sem `f_mount failed`), ou seja o CMD0 nao foi
  respondido. Cartao **travado de novo** — a sessao anterior terminou com uma
  gravacao abortada e o cartao fica no estado de latch ate' cortar a energia
  (o reset do ESP32 nao corta o 3V3 do modulo, e o cabo USB alimenta os dois).
  Teste: desligar o cabo USB (ou o VCC do modulo) 20 s e religar.
- `webSdMount()` agora manda 128 clocks com CS alto antes de cada tentativa e
  avisa no serial quando a suspeita e' cartao travado. Nao ha recuperacao por
  software para o latch: so' energia.

### 14) /raw — diagnostico cru do cartao (para de vez com o "nao montou")
- Como o log do serial nao mostra erro nenhum da lib quando `SD.begin` falha,
  nao dava para separar "cartao morto" de "lib/montagem". Novo endpoint:
  **http://192.168.4.1/raw** roda fora do SD/FATFS: 80 clocks, CMD0, CMD8
  (CRC 0x87), CMD58 (OCR) e CMD17 do setor 0 (MBR), com CRC7 proprio, e
  depois tenta remontar.
- Tabela de leitura:
  - `cmd0=0x01` + `cmd8 ... echo=...1AA` -> o cartao FALA. Se o mount ainda
    assim falha, o problema e' o caminho de montagem (instancia `tftSPI`
    compartilhada com o TFT) e o passo e' montar com outra instancia SPI.
  - `cmd0=0xFF` -> o cartao NAO responde: latch/energia/fiacao. Cortar a
    energia 20 s (o cabo USB mantem o modulo alimentado) e rezar.
  - `cmd17 ... bloco lido` + `sig=55AA ok` -> midia boa.

### 15) Resultado do /raw (usuario) e correcao do probe
- Resposta obtida: `cmd0=0x0 (clocks=1)`, `cmd8=0x0 echo=0000`,
  `cmd58=0x0 ocr=0000`, `cmd17=0x0 SEM BLOCO`, `remontou=0 card=0`.
- Leitura: 0x00 em todos os R1 nao e' R1. R1 de CMD0 tem de ser 0x01 (idle) e
  CMD8 tem de ecoar 00 00 01 AA. MISO respondendo 0x00 em tudo = alguem
  segurando a linha em baixo (cartao busy/travado) -> nao ha conversa SPI.
- **Erro meu**: a 1a versao do `/raw` chamava `SD.end()` antes do teste cru. No
  core 3.3.12 o `SD.begin()` depois do `SD.end()` nao volta, entao o probe
  destruia a montagem que estava funcionando (por isso `remontou=0`) e ainda
  entregava leitura lixo. **CORRIGIDO**: se o cartao estiver montado o `/raw`
  nao roda e responde "CARTAO MONTADO ... teste cru cancelado"; o teste cru so'
  acontece com o cartao NAO montado (nada a perder).
- `webSdStart()` agora tem **2a rodada**: se a 1a falhar, 2 s com CS alto e mais
  8 tentativas com folga de 250 ms — o explorador monta em situacoes em que o
  servidor falhava, e a diferenca era tempo.
- Pista principal do relato: "o explorador detectou os arquivos" e o servidor
  diz que nao montou. Ou seja o cartao FALA — o que falha e' o caminho/timing
  da montagem no modo servidor, nao o hardware.

### 16) A MORTE EM ESPIRAL: webSdRecover() destruindo montagem sadia
- Relato final: `/st` diz "cartao OK (15591 MB livres)" e o `/raw` responde
  "CARTAO MONTADO card=3" (ou seja, `f_mount` e `f_getfree` **funcionam**),
  mas o explorador nao lista e o upload erra ("cartao montado mas sem gravacao"
  / "falha no SD"). E antes disso o relogio dizia "recalculando o cartao".
- Causa raiz encontrada no codigo: quando o teste de escrita falhava com o
  cartao **montado**, a gente chamava `webSdRecover()` — que faz `SD.end()`. No
  core 3.3.12 o `SD.begin()` depois do `SD.end()` nao volta, entao a montagem
  sa e o explorador passa a dizer "SD ausente" com o cartao vivo. Na proxima
  sessao o servidor remonta (cartao bom) mas com o FAT ja' sujo da tentativa
  abortada anterior -> `f_open` falha -> novo "sem gravacao". Loop.
- CORRIGIDO: `webSdRecover()` agora so' roda **quando o cartao morreu de fato**
  (`SD.cardType() == CARD_NONE`). Montado sem escrita = FS danificado: o codigo
  nao chama mais SD.end(), nao insiste em gravar e o serial imprime
  "[sd] montou mas nao grava (errno=5/FS): formate o cartao em FAT32".
- Para o usuario: monte + FAT ok + open/list falhando = arquivos de sistema do
  cartao corrompidos pelas gravacoes abortadas. Formatar FAT32 (unidade 32 KB,
  formato completo) resolve. Com a correcao acima o firmware nao corrompe mais
  tentando de novo em cima.

### 17) Diagnostico definitivo da gravacao (log do usuario)
- `[sd] wr: write=2 size=0 errno=0 readback=0 remove=0` + `mount=1 grava=0 card=3`:
  o `f.open` cria a entrada (diretorio), o `f.write` aceita 2 bytes no buffer do
  FatFs, mas **nada chega ao cartao** (readback falha, remove falha, size 0).
  Falha fisica de escrita, independente de formato — o cartao formata e monta
  normalmente e a leitura funciona.
- Usuario confirma: cartao original, formatado, e o **SD_Test do core no mesmo
  modulo gravou 1 MB**. Ou seja, escrever no hardware funciona (a 4 MHz) — o que
  muda no firmware e' a frequencia do SPI do SD.
- Mudanca: `SD_SPI_HZ` voltou para **4 MHz** (a frequencia em que a gravacao foi
  comprovada nesse hardware). 400 kHz e 1 MHz nao gravavam. O log do mount agora
  imprime a velocidade usada.
- Se ainda assim falhar, a proxima alavanca e' o **clock de 20 MHz** (o core do
  Arduino trava em 4 MHz por seguranca, o IDF sobe a 20 MHz) — mas o SD_Test ja
  gravou a 4 MHz, entao a hipotese forte e' outro detalhe do caminho (instancia
  `tftSPI` x `SPI`), que o teste acima isola.

### 18) Modo upload: rede, tela nova e "sem espaco" falso
- **Rede**: o AP "WearSend" so' sobe quando NAO ha wifi salvo conectado.
  `webSdStart()` guarda a rede em que o relogio ja' estava (`prefer`), monta o
  SD com o radio off, e depois conecta no wifi salvo (12 s de espera); se
  conectar, fica em STA e mostra o IP da rede na tela; se nao, cai para o AP.
  Globais: `gSrvIp`, `gSrvAp`, `gSrvNet` (core.h).
- **Tela do relogio refeita** (`uiServerScreen()` em ui.h, TFT cru como o resto
  do modo servidor): fundo preto, ponto laranja + "WearSend", badge AP/WIFI,
  card arredondado (#161A22) com REDE / SENHA / "ABRA NO NAVEGADOR" + IP em
  laranja, barra de progresso arredondada dentro do card, "SD OK" no rodape e
  botao FECHAR arredondado com contorno laranja (mesma regiao de toque:
  SERV_X/Y/W/H). Paleta nova: `SRV_BG/CARD/EDGE/ACC/TXT/DIM/OK/ERR`.
  `webServBar()` (Smartwatch.ino) redesenhou a barra e o status dentro do card.
- **"sem espaco no SD" falso**: `SD.totalBytes()` devolve 0 quando o
  `f_getfree` falha, e o espaco livre era calculado como 0 -> o upload era
  recusado com 507 num cartao de 16 GB. Agora so' recusa quando o espaco e'
  de fato conhecido (`tb > 0`) e insuficiente; se `tb == 0` o log avisa
  "espaco desconhecido" e a gravacao tenta assim mesmo. O `/st` passou a
  devolver `rw <livreMB> <totalMB>` e a pagina mostra "espaco desconhecido"
  quando o total e' 0.
- **Pagina WearSend refeita** no mesmo estilo do relogio (fundo #000, cards
  #161A22 raio 16, destaque #FF7A00, chips circulares para Imagens/Videos/
  Musicas, botao ENVIAR em pílula, barra de progresso fina, lista de arquivos
  em card). Preview conferido no navegador.

### 19) Rede OK; escrita ainda falha -> teste de setor CRU
- Bateu: `[srv] conectando no wifi 'TIM_ULTRAFIBRA_0779_2G'...` /
  `[srv] wifi conectado: http://192.168.1.11`. Modo upload com Wi-Fi salvo
  funcionando, sem AP.
- A escrita continua: `[sd] wr: write=2 size=0 errno=0 readback=0 remove=0`
  a 4 MHz, com o barramento travado e o radio off. Ou seja: nem o diretorio
  nem o cluster de dados chegam ao cartao.
- **Teste decisivo** (novo): `webSdRawSector()` usa `sdReadSector`/`sdWriteSector`
  da propria lib (sem FATFS): le a tabela do MBR, escolhe um setor FORA de
  qualquer particao, le, regrava o MESMO conteudo e rele. Log:
  `[sd] raw setor N: leu=1 gravou=1 releu=1 igual=1 (mbr=55AA)`
  - `gravou=0` -> falha FISICA de escrita (cartao/modulo/bus). O firmware esta
    correto; e' para testar outro cartao/modulo.
  - `igual=1` mas o FatFs falha -> o problema e' da camada de arquivos
    (VFS/max_files/mount), e ai o alvo passa a ser o `esp_vfs_fat`.
- 1a execucao do teste cru deu `raw setor 1000: leu=0 ... (mbr=0000)`: o teste
  rodou **depois** do teste via FatFs, que ja' tinha abortado a gravacao e
  deixado o cartao travado (MISO preso em 0) — media do cartao ja' morto.
  **Ordem trocada**: agora o teste cru (fisico) vem ANTES do teste via FatFs.
  Log extra: "[sd] FISICO: o cartao nem grava um setor cru -> e' modulo/cartao/
  bus, nao e' o FATFS" e "[sd] FISICO ok mas FatFS falhou -> o problema e' da
  camada de arquivos".
- Resultado do usuario (a 4 MHz): `raw setor 1000: leu=0 gravou=0 releu=0
  igual=0 (mbr=0000)` e depois `wr: open falhou errno=5 total=0 used=0`.
  **Leitura de DADO falha a 4 MHz** — mas a INICIALIZACAO (que a lib faz a
  400 kHz: CMD0/CMD8/CMD58/ACMD41/CSD) sempre funcionou. Ou seja: o cartao
  conversa a 400 kHz e *nao* a 4 MHz neste setup (barramento compartilhado com
  o TFT a 40 MHz + shifter do modulo).
- **Mudanca**: `SD_SPI_HZ` voltou para **400 kHz** (velocidade em que a
  leitura comprovadamente funciona: o explorador listava os arquivos).
  Mensagens do serial ajustadas: "nao e' problema de FAT (formatar nao
  resolve) - e' a camada fisica: cartao/modulo/bus".
- O teste cru agora loga tambem o resultado da leitura do MBR e a velocidade:
  `[sd] raw setor N @400000 Hz: mbr=1 leu=1 gravou=1 releu=1 igual=1`.
  - `mbr=0` -> nem a leitura do MBR funciona (cartao travado/morto).
  - `leu=1 gravou=0` -> leitura ok, escrita fisica falha -> modulo/cartao
    (corrente no pico da programacao ou MOSI do shifter).
  - `igual=1` mas o FatFs falha -> e' a camada de arquivos.
- Investigacao da lib (local, core 3.3.12):
  - `char` e' **unsigned** no xtensa-esp32-elf-gcc 14.2 (testado compilando:
    `extui` = zero-extend) -> o `sdWait` da lib funciona.
  - `CRC7`/`CRC16` estao em `SD/src/sd_diskio_crc.c` e conferem (CRC7 do CMD0
    = 0x95, CRC16-CCITT correto). Sem bug de CRC.
  - `sdCommand`/`sdReadBytes`/`sdWriteSector` estao corretos; `ff_sd_read` usa
    exatamente o mesmo `sdReadSector` que o meu teste cru.
  - `f_mount` **nao le dado nenhum** (so` `disk_initialize`): por isso o mount
    "funciona" mesmo com o cartao sem I/O de dado. O primeiro I/O de dado
    acontece no `f_open`/`f_getfree` — e ai vem `errno=5`.
  - O `SD_SPI_HZ` so' afeta o I/O de DADO (a init sempre roda a 400 kHz).
- **Pista decisiva pendente**: o cartao responde a todos os comandos de
  registro (CMD0/CMD8/CMD58/ACMD41/CSD) mas nao completa um bloco de dado.
  Falta separar "TFT no barramento" de "cartao/modulo".
- **`teste_sd/teste_sd2.ino`** (novo): FASE A = SD sozinho; FASE B = TFT
  ligada e usando o mesmo bus, com remount. A ok + B falha => a tela no bus e'
  o problema; A falha => cartao/modulo; A e B ok => outro ponto do firmware.
- **RESULTADO do teste 2**: `RESUMO: A(so' SD)=FALHOU B(TFT+SD)=FALHOU`, MOUNT
  falhou nas DUAS fases => **a tela NAO e' a causa** (e nem o firmware do
  relogio). Mas o teste rodou logo apos a escrita abortada no relogio, sem
  corte de energia: o cartao provavelmente ja' estava travado.
- **`teste_sd/teste_sd.ino` foi refeito** para ser paciente: sequencia de
  power-up (320 clocks com CS alto), 4 tentativas de mount com 1,2 s de pausa
  entre elas, gravacao de 4 KB + read-back byte a byte, e resumo final
  ("GRAVOU E LEU DE VOLTA" / "FALHOU").
- Firmware: `sdTryBegin()` agora faz a sequencia de power-up (128 clocks com CS
  alto + 600 ms na 1a tentativa + 250 ms entre tentativas) antes de cada
  `SD.begin()`.
- **INSTRUCAO PARA O USUARIO**: antes do teste, DESLIGAR a energia do cartao
  (cabo USB ou VCC do modulo) por 30 s — sem isso o cartao continua travado da
  ultima escrita abortada e o teste mede um cartao morto.
- Resultado do `teste_sd` (4 tentativas, 400 kHz): `MOUNT: falhou` nas 4.
  Sem o corte de energia de 30 s a antes. O teste foi reescrito para varrer
  400 kHz / 1 MHz / 4 MHz, com 8 tentativas cada, 3 s de espera inicial e
  sequencia de power-up (320 clocks) antes de cada tentativa.
- **BUG DA PAGINA (corrigido)**: o `<input type=file>` tinha sumido na
  reformulacao da pagina e o botao "Escolher arquivo" nao abria nada. Agora o
  input esta de volta (escondido com `position:absolute;opacity:0` — nao com
  `display:none`, que impede o clique) e o botao chama `f.click()` direto.
- **"SEM ESPACO"**: ja corrigido — so' recusa o upload quando o espaco e'
  realmente conhecido (`tb > 0`) e insuficiente; antes, `totalBytes()==0`
  (f_getfree falhando) era tratado como 0 MB livres e o upload era recusado num
  cartao de 16 GB.
- **DESCOBERTA IMPORTANTE (usuario)**: no relogio o SD **monta**, mas no
  sketch minimo (`teste_sd`) o mount falha em 24 tentativas (400k/1M/4M).
  Diferenca estrutural entre os dois: **o relogio poe o CS do touch (XPT2046,
  GPIO36) em ALTO no boot; o sketch nao** — com o CS do touch flutuando/baixo
  o controlador **dirige o MISO (GPIO34)**, que e' compartilhado com o SD, e o
  cartao nao responde a comando nenhum.
- Acao: `teste_sd/teste_sd.ino` agora poe `TOUCH_CS (36) = OUTPUT/HIGH` e
  `TOUCH_IRQ (35) = INPUT_PULLUP` antes de montar (igual ao relogio). No
  firmware, `sdTryBegin()` tambem garante o CS do touch em ALTO antes de cada
  `SD.begin()`.
- Se o sketch passar a montar **e gravar**, o hardware esta OK e o defeito do
  relogio esta no caminho de escrita (o sketch vira a referencia).

### 20) RESOLVIDO: era o TOUCH (XPT2046) no MISO
- `teste_sd` com `TOUCH_CS (36) = OUTPUT/HIGH` + `TOUCH_IRQ (35) =
  INPUT_PULLUP`: `MOUNT: ok a 400000 Hz (tentativa 1)`, `WRITE: 2048 bytes
  size=2048`, `VERIFY: OK (0 bytes diferentes)`, `RESUMO: GRAVOU E LEU DE VOLTA
  (hardware OK)`. **O cartao, o modulo e o barramento estao perfeitos.**
- Causa: o XPT2046 (touch) divide o MISO (GPIO34) com o SD. Com o CS dele em
  baixo/flutuando ele DIRIGE a linha e o cartao nao responde a nada. No
  relogio o CS do touch era posto em ALTO no boot, MAS o touch era lido
  constantemente e a leitura deixava o barramento sujo/volume com a linha
  disputada -> a leitura de dados falhava.
- Correcoes aplicadas no firmware:
  - `sdBusPrepare()` (core.h): poe o CS do touch em ALTO e manda 8 clocks
    (limpa o MISO) — chamada antes de TODO acesso ao SD (`sdTryBegin`,
    `sdBeginSession`, `webSdMount`, `webSdWriteTest`, `webUpFile`, `/raw`).
  - `touch_read()` (LVGL) e `touchTick()` agora **nao leem o toque** enquanto
    `gSdPause` estiver ativo (sessao do SD) — o touch fica fora do MISO.
  - `tsReadCh()` mantem o `busLock` (ja existia) + `gSdPause`.
  - O teste de setor cru foi REMOVIDO do boot (ele gravava no cartao).
- Estado do sistema de arquivos/upload: `/FOTOS`, `/VIDEOS`, `/MUSICAS`,
  upload pela pagina WearSend (Wi-Fi salvo -> IP na tela; sem Wi-Fi -> AP),
  tela nova do relogio, "sem espaco" corrigido, bug do botao "Escolher
  arquivo" corrigido.

### 21) Sketch funciona, relogio nao — o teste do SD no relogio foi refeito
- O `teste_sd` (sem tela) grava e le de volta perfeito; o relogio continua
  com `write=2 size=0 readback=0 remove=0`.
- `webSdWriteTest()` reescrito para espelhar o sketch que funciona:
  1) **le** o diretorio raiz ANTES de gravar (no sketch a 1a operacao de dado
     e' uma leitura);
  2) grava **2048 bytes** (4 setores) em vez de 2 bytes;
  3) le de volta e compara byte a byte;
  4) se falhar: **reinicia o host SPI** (`tftSPI.end()` + `tftSPI.begin(...)`)
     e tenta de novo — o TFT deixa o host configurado a 40 MHz e apos dezenas
     de transacoes o SD pode voltar a nao responder. O display nao se importa:
     e' o mesmo objeto `SPIClass`.
- **`teste_sd/teste_sd3.ino`** (novo): "TFT primeiro", como o relogio (tela
  ligada ANTES do SD). Se gravar -> a tela nao e' a causa; se falhar -> a tela
  no barramento e' a causa. Um flash resolve.
- **RESULTADO do teste 3**: `MOUNT: ok type=3 size=15600 MB`, `LISTA: 1
  entradas`, `WRITE: 2048 bytes size=2048`, `READBACK: 2048 bytes -> OK`,
  `RESUMO: GRAVOU (a tela NAO e' o problema)`. **A TELA NAO E' O PROBLEMA.**
- Falta no teste 3 a peca que o relogio tem: a **LEITURA DE TOQUE** (XPT2046
  no mesmo MISO). **`teste_sd/teste_sd4.ino`** (novo): simula 40 leituras de
  toque exatamente como o relogio (CS 36 em baixo, 2,5 MHz, 0xB0/0xC0/0x90/
  0xD0) e depois testa o SD. FALHOU -> o toque e' o culpado; PASSOU -> o
  problema e' outro (task de audio/LVGL/Wi-Fi do relogio).
- **RESULTADO do teste 4**: `MOUNT: ok`, `WRITE: 2048 bytes`, `READBACK: OK`,
  `RESUMO: GRAVOU (o toque NAO e' o problema)`. **Nem a tela nem o toque sao o
  problema.** O que falta no sketch: (a) o host SPI usado por centenas de
  renders do TFT, (b) a **luz de fundo** acesa + Wi-Fi + audio no mesmo 3V3,
  (c) as outras tasks do relogio.
- **Mudancas no relogio** (em `webSdStart`/`webSdWriteTest`):
  1. `sdSpiReset()` ANTES de montar (e de novo se a escrita falhar): recria o
     periferico SPI (`tftSPI.end()` + `begin(9,34,10,-1)`) — no sketch o host
     tem 2-3 transacoes, no relogio o TFT o deixou em 40 MHz apos centenas de
     renders.
  2. **Backlight OFF durante o teste de escrita** (e devolvido depois): a luz
     de fundo + Wi-Fi + audio dividem o 3V3 e a tensao pode cair no pico da
     gravacao do cartao. No sketch minimo a luz esta apagada.
  3. Leitura antes de gravar + 2048 bytes + read-back (como o sketch).
- **REGRESSAO**: chamar `sdSpiReset()` (tftSPI.end() + begin) ANTES de montar
  ** QUEBROU o bus**: o `spiAttachSCK` falha depois do stop/start e o SD nem
  monta mais (`mount falhou apos 6 tentativas`). Removido do caminho do mount
  (so' fica no retry da escrita, agora com log do retorno do begin()).
- **TESTE DO BOOT** (novo): em `setup()`, logo apos `dispInit()` e ANTES da UI/
  Wi-Fi/tasks, com a luz de fundo apagada, o firmware monta o SD e faz o teste
  de escrita — exatamente o cenario do sketch minimo que funciona.
  Log: `[boot] sd: mount=1 grava=1 card=3`.
  - `grava=1` -> o defeito esta no que a UI faz depois (renders do LVGL a
    40 MHz, Wi-Fi, task de audio). Aí o alvo e' desacoplar o render.
  - `grava=0` -> e' host SPI ou alimentacao (e o sketch grava...).
- **DIFERENCA ESTRUTURAL ACHADA**: `dispInit()` usava `tft.setSPISpeed(40 MHz)`
  — o sketch minimo usa o **padrao do driver Adafruit = 1 MHz**. 40x de
  diferenca no clock do SCK no barramento compartilhado: a inicializacao do
  cartao (comandos curtos, respostas com folga) passa, mas o **I/O de DADO**
  (512 bytes seguidos, sem folga) falha com EIO/CRC.
  - **Correcao aplicada**: `tft.setSPISpeed(8000000)` (8 MHz). O quadro de
    40 KB sai em ~5 ms — o LVGL continua rapido.
  - **`teste_sd/teste_sd5.ino`** (novo): tela a 40 MHz + SD. FALHAR -> confirma
    que o clock da tela e' o culpado; PASSAR -> o clock nao e' a causa.
- Bug do `busLock()` que devolve false no timeout: quem chama precisa **pular
  a operacao**, nunca seguir sem o lock (senao a tela desenha em cima do SD).
  **Corrigido** em `disp_flush` (pula o quadro se nao pegar o lock).
- **TESTE 5**: tela a 40 MHz + SD -> `GRAVOU`. **O clock da tela NAO e' o
  problema** (a tela do relogio foi mantida em 8 MHz por performance, mas
  nao era a causa).
- **ULTIMA DIFERENCA ESTRUTURAL**: no relogio o `SPIFFS.begin(true)` (flash)
  rodava ANTES de qualquer acesso ao SD; o sketch minimo nao tem SPIFFS
  nenhum. **Reordenado**: `dispInit()` -> **montagem + teste do SD** ->
  `SPIFFS.begin()` -> `cfgLoad()`. A rotacao/brilho do usuario sao
  reaplicados depois (`tft.setRotation`, `blSet`).
- Log do boot agora traz tambem `totalMB/usedMB` (o `SDFS::totalBytes()` usa
  o drive "0:" fixo — se der 0 com o mount OK, o drive do FAT e' o problema).
- **`teste_sd/teste_sd.ino`**: reescrito (unico arquivo de teste, como pedido
  pelo usuario) como **replica do setup() do relogio** passo a passo:
  1. analogico + pinos (TOUCH/IR/TFT_RST/SD_CS) -> 2. `SPIFFS.begin(true)` +
  escrita/leitura -> 3. `dispInit()` (TFT 8 MHz, offset, rotacao, fillScreen,
  backlight) -> 4. teste do SD (le a raiz, grava 2048 B, relê e compara).
  Cada passo e' logado; o primeiro que quebrar denuncia o culpado.
  `RESUMO: GRAVOU` -> o defeito e' em tempo de execucao (UI/tasks/Wi-Fi);
  `RESUMO: FALHOU` -> o culpado e' um dos passos 1-3.
- **Usuario nao quer trocar os pinos** (e concorda que nao e' o problema) —
  a solucao tem que ser por software.
- Erro de compilacao no sketch: `ADC_11AD` nao existe no core -> `ADC_11db`.
- **DESCOBERTA DO USUARIO**: o cartao **trava depois de uma escrita que falha** e
  so' volta com corte de energia (o "MOUNT FALHOU" so' resolve desconectando).
  Isso explica o "mount=0 card=0" do comeco: **nao era o SD.end() que quebrava,
  era o cartao travado**. Entao o `SD.end()` NAO e' proibido — e o cartao
  precisa de energia para reviver.
- **`teste_sd/teste_sd.ino` virou A/B num arquivo so**:
  A) mount + escrita imediata -> B) anima a tela por 30 s (flushes a ~30 fps,
  como a UI do relogio) -> C) escrita de novo SEM remontar.
  - A ok + B falha -> a **atividade da tela** no barramento quebra o SD.
  - A ok + B ok -> o problema e' tempo/outra task (audio, Wi-Fi).
- **RESULTADO DO A/B (usuario)**: `A: GRAVOU` e `B: FALHOU` ->
  **CONFIRMADO: A ATIVIDADE DA TELA NO BARRAMENTO COMPARTILHADO QUEBRA O SD.**
  (o sketch so' desenha duas vezes e para; o relogio desenha o tempo todo)
- **CORRECAO (ideia do usuario: "deixar a tela estatica durante a gravacao e so
  retornar quando a gravacao terminar")**:
  1. `sdCardReInit()` (novo, no .ino): re-inicializa o cartao SEM desmontar —
     CMD0 + CMD8 + ACMD41(HCS) + CMD58(OCR), com CRC7 proprio. Sem SD.end() e
     sem remontar o VFS (o FatFs continua valido). Log:
     `[sd] reinit: cmd0=01 cmd8=01 cmd55=01 acmd41=00 cmd58=00 ocr=C0FF8000`.
  2. Chamado em `sdBeginSession()` (todo acesso ao SD), em `webSdWriteTest()` e
     no inicio do upload.
  3. A tela fica **estatica** durante a gravacao: `gSdPause` segura o render
     (o `disp_flush` ja pula o quadro) e o `busLock` segura o barramento; a
     tela so' volta depois que a gravacao termina.
- `teste_sd/teste_sd.ino` ganhou o **teste C**: apos o A/B, re-inicializa o
  cartao e escreve de novo. C ok -> a correcao funciona; C falha -> o veneno
  esta no host (SPI) e nao no cartao.
- **RESULTADO do A/B com re-init**: `reinit: cmd0=01 cmd8=01 cmd55=01
  acmd41=FF cmd58=01 ocr=00FF8000` e `C: FALHOU`.
  - **`cmd0=01` e `cmd8=01`: o cartao esta VIVO e responde.** Ele nao fica
    "envenenado" — o que quebra e' o I/O de DADO.
  - **`acmd41=FF` = ERGO MEU (bug no re-init): o `CMD55` precede CADA `ACMD41`
    (o cartao esquece o CMD55 se o CS subir no meio, e o CMD41 solto e'
    rejeitado). Meu loop reenviava o ACMD41 sem CMD55 -> 0xFF.
    **Corrigido** (relogio e sketch): o loop faz CMD55 -> CS alto -> CS baixo ->
    ACMD41 -> le o R1, ate 0x00.
  - O `ocr=00FF8000` (sem o bit CCS) confirma que o cartao nunca saiu do idle
    por causa do ACMD41 quebrado.
- Proximo: rodar o A/B de novo (com o re-init corrigido). `C: GRAVOU` -> o
  re-init e' a correcao e ela ja' esta no firmware (`sdCardReInit()` em
  `sdBeginSession`, no teste de escrita e no upload). `C: FALHOU` -> o veneno
  esta no HOST (SPI) e o conserto e' reiniciar o periferico SPI antes de
  gravar (o `tftSPI.end()+begin()` quebrava o mount porque o `spiAttachSCK`
  falha depois do stop/start — talvez com `spiStopBus` completo).
- **CAUSA RAIZ DO `acmd41=FF`: O MEU CRC7 ESTAVA ERRADO.** A lib usa a tabela
  da spec (CRC7 de 7 bits, byte final = `(crc<<1)|1`): CMD0 -> **0x95**,
  CMD8 -> **0x87**, ACMD41 -> **0x77**, CMD58 -> 0xFD, CMD17 -> 0x55. A minha
  versao "caseira" (poly 0x12, sem a mascara de 7 bits) dava 0x29/0x0D/0xED:
  o cartao acceptou o CMD0 (leniente) mas **rejeitou o ACMD41** (0xED != 0x77)
  -> 0xFF -> o cartao nunca saia do idle -> gravacao impossivel.
  **Corrigido** em `Smartwatch.ino` e `teste_sd/teste_sd.ino`: a tabela
  `CRC7T`/`SD_CRC7T` (256 bytes) da lib + `crc = T[(crc<<1)^d[i]]` e o byte
  final `(crc<<1)|0x01`. Conferido em Python: 0x95/0x87/0x65/0x77/0xFD/0x55.
  (O `sdCrc7` duplicado do `/raw` foi removido: agora ha um so.)
### 22) CAMADA DE SD REFEITA no esquema do teste 4 (o unico que grava)
- Decisao do usuario: refazer o SD/sistema de arquivos do zero, copiando o
  esquema do `teste_sd4` (funciona): **um unico ponto de montagem, uma vez, num
  momento controlado, e nunca mais mexer no cartao**.
- Regras novas (core.h `sdTryBegin`):
  1. CS do touch em ALTO (`sdBusPrepare`) antes de qualquer coisa;
  2. sequencia de power-up (CS alto + **256 clocks**) antes de cada tentativa;
  3. **NUNCA `SD.end()`** em lugar nenhum do firmware — no core 3.3.12 o
     `begin()` depois do `end()` nao volta, e era isso que produzia o
     "mount=0 card=0" com o cartao presente. Um `begin()` que falhou ja limpa
     o registro do VFS internamente, entao repetir `begin()` e' seguro;
  4. o cartao **fica montado o boot inteiro**; `sdBeginSession()` so' monta
     se `cardType()==CARD_NONE`;
  5. `webSdRecover()` tambem sem `SD.end()`;
  6. `sdProbe()` sem `SD.end()`.
- A montagem acontece **no boot**, logo apos `dispInit()` e antes da UI/Wi-Fi/
  tasks, com a luz de fundo apagada: e' literalmente o cenario do teste 4.
  Log: `[boot] sd: mount=1 grava=1 card=3`.
- O teste de escrita (`webSdWriteTest`) espelha o sketch: le a raiz antes,
  grava 2048 bytes, relê e compara byte a byte, luz de fundo apagada durante
  a gravacao, e se falhar reinicia o host SPI (com log do retorno).

### 23) A CAUSA DO errno=5 ENCONTRADA: sdCardReInit() matava o CRC do cartao
- O log do `teste_sd` (15/11) mostrou: `miso` sadio, `cmd0=0x01`, `cmd8=0x01`
  (echo `F0 00 01 AA`), `acmd41=FF`, `cmd58 r1=00`. Ou seja: **o cartao nao
  estava travado** (a re-init caseira e' que nao funciona) e o `cmd17 r1=0x05`
  era culpa do proprio probe (que mandava CMD0 antes).
- O bug real: `sdCardReInit()` mandava **CMD0** antes de toda escrita/sessao.
  CMD0 volta o cartao ao **idle** e **zera o CRC_ON_OFF**; a lib continua com
  `supports_crc=true` e compara o CRC16 de todo bloco lido (`sdReadBytes`),
  que com o CRC desligado vem 0xFFFF -> comparacao falsa -> `RES_ERROR` ->
  `FR_DISK_ERR` -> **errno=5 em toda leitura** (e `f_open` le setores antes de
  gravar). Por isso: `mount=1` e nenhuma escrita funcionava.
- CORRIGIDO: `sdCardReInit()`/`sdR1(int)` removidos do `Smartwatch.ino` e a
  chamada removida do `sdBeginSession()` (`ui.h`). Tambem saiu o
  `sdSpiReset()` (`tftSPI.end()+begin()`), que quebrava o host SPI/mount.
  Regra: **so' o `SD.begin()` inicializa o cartao**.
- `teste_sd/teste_sd.ino` reescrito para usar a lib (`readRAW`, `CMD13`,
  `SD.end()+SD.begin()`) com knobs `ANIMA_MS`/`ANIMA_LUZ`/`TFT_HZ`.

### 24) CONFIRMADO: a tela nao quebra o SD (teste com a lib, 15/11)
- `teste_sd` novo (usa `SD.readRAW`, `CMD13`, `SD.end()+SD.begin()`, nada de SPI
  na mao): `A: GRAVOU` -> 30 s de animacao a 8 MHz com a luz ACESA ->
  `B: GRAVOU`, `readRAW(0)=1 sig 55AA`, `totalBytes=15592 MB`. Antes e depois.
- Ou seja: **o barramento compartilhado com o TFT nunca foi o problema.** O
  "teste A/B" antigo estava contaminado (o probe mandava CMD0 antes do CMD17 e
  deixava o cartao em idle) e o relogio somava o `sdCardReInit()` em cima.
- Corrigido tambem o diagnostico: em SPI o R2 do CMD13 (16 bits) NAO tem
  CURRENT_STATE; o sketch agora so' reporta `r1` (aceitou / ilegal / mudo).
- Proximo: validar o upload no relogio e, se estiver estavel, subir o clock do
  SD (400 kHz -> 1-4 MHz) para acelerar.

### 25) O cartao morre na ASSOCIACAO do Wi-Fi, nao na escrita (16/11)
- Log do relogio: `mount=1 grava=1 card=3` no boot e, depois de
  `E (7610) wifi:Association refused too many times`, `card=4` (= `CARD_UNKNOWN`)
  e `errno=19` (= `FR_NOT_READY`). Ou seja: o driver tentou reinicializar o
  cartao e falhou, marcou UNKNOWN e o FatFs ficou sem volume -> toda escrita
  vira `ENODEV`.
- Consequencia pratica: `cardType() != CARD_NONE` NAO quer dizer "montado".
  UNKNOWN tem `s_cards[pdrv]` valido mas DATA I/O morta. `webSdMounted()`
  agora so' aceita MMC/SD/SDHC.
- `webSdRecover()` passou a ser de verdade: `SD.end()` + `delay` +
  `SD.begin()` (3 tentativas) e prova por `SD.readRAW` do setor 0 com a
  assinatura 55 AA. O par end+begin e' o unico uso permitido de `SD.end()`.
- `sdAlive()`: le o setor 0 pelo driver (sem FatFs) e guarda `CARD_NONE`
  para nao usar `pdrv=0xFF`. Usado nos logs `[srv] sd antes/depois do wifi`
  e para decidir o remount depois do radio estabilizar.
- Bug achado no core: `WiFi.setTxPower()` checa `STA.started()/AP.started()`
  e devolve `false` sem efeito se chamado antes de `begin()`/`softAP()` --
  era exatamente como estava, entao o radio seguia a 19.5 dBm (pico de corrente
  na associacao). Agora: STA -> `begin()` e logo `setTxPower(8.5 dBm)`, com
  fallback para 19.5 dBm se nao associar em 8 s; AP -> `softAP()` e depois
  `setTxPower`.
- Se o cartao cair mesmo assim: proximo passo e' hardware (capacitor no 3V3 do
  slot) e/ou remount so' depois do Wi-Fi ocioso.

### 26) Site do historico (docs/) - 16/11
- Objetivo: uma pagina unica com TODAS as versoes do firmware, mostrando por versao
  o que foi corrigido, o que foi adicionado e o que mudou - para que a solucao de um
  bug antigo nao se perca e um bug ja' resolvido nao volte sem aviso.
- Fica em `docs/` (pasta que o GitHub Pages serve direto: Settings > Pages >
  Deploy from a branch > main > /docs). Sem build, sem dependencia, abre tambem com
  dois cliques no arquivo.
- **Fonte da verdade: `docs/assets/versoes.js`**. Cada item = um release
  (`v`, `data`/`quando`, `titulo`, `estado`, `resumo`, `corrigido`,
  `adicionado`, `alterado`, `arquivos`, `pendente`, `fonte`). Versao nova =
  um objeto novo no topo do array. O `docs/README.md` tem o passo a passo.
- `estado`: `ok` (resolvido), `hw` (pronto, falta validar no relogio) e
  `aberto` (bug conhecido). Os filtros do topo contam por esse campo.
- Conteudo: 25 releases, de v0.01 (player WAV/BadBLE) a v0.25 (cartao cai no Wi-Fi),
  com os detalhes tirados deste STATUS e de `docs/analise_sd.md`.
- Verificado no navegador: 25 cartoes, busca (`wifi` -> 2, `errno=19` -> 1),
  filtro por estado, zero overflow horizontal em 320/390/768/1000 px, alvos >= 44 px,
  contraste >= 4.5:1 nos dois temas. Lighthouse: 100 acessibilidade / 100 boas
  praticas / 100 SEO.
- `docs/.nojekyll` evita o processamento do Jekyll no Pages.
