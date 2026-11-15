# Site do histórico do firmware

Página estática (sem build, sem dependências) que lista todas as versões do firmware
com **o que foi corrigido, o que foi adicionado e o que mudou** em cada uma.

```
docs/
  index.html          estrutura + textos fixos (cabeçalho, rodapé)
  assets/versoes.js   >>> A FONTE DA VERDADE: os releases
  assets/estilo.css   tema escuro/claro, layout
  assets/app.js       render, busca, filtros, âncoras
  .nojekyll           impede o GitHub Pages de passar o site pelo Jekyll
```

Abrir localmente: dois cliques em `docs/index.html` (funciona sem servidor).

## Adicionar uma versão

1. Abra `docs/assets/versoes.js`.
2. Copie o primeiro objeto (`v0.25`) e cole **acima** dele.
3. Preencha:

```js
{
  v: "v0.26",
  data: "2026-11-17",          // null se não houver data registrada
  quando: "Sessao X",          // usado quando data é null
  titulo: "Uma linha: o que mudou",
  estado: "hw",                // "ok" | "hw" | "aberto"
  resumo: "Por que essa versão existiu. Use `crase` para código.",
  corrigido: ["bug X: causa e correção"],
  adicionado: ["recurso Y"],
  alterado: ["mudança Z"],
  arquivos: ["Smartwatch.ino", "core.h"],
  pendente: ["o que ainda falta validar"],
  fonte: "STATUS.md §26"
}
```

4. Recarregue a página. Os contadores do topo e dos filtros se recalculam sozinhos.

`v0.NN` é o índice do release na ordem de entrega — não é semver. O detalhe técnico
completo de cada rodada continua no `STATUS.md` (raiz do projeto); o site é o resumo
navegável.

## Publicar no GitHub Pages (fluxo usado, sem git)

O upload pela interface do GitHub **achata os caminhos** (`docs/assets/estilo.css`
vira `estilo.css`), então o site publicado é gerado como **arquivo único**:

```powershell
deno run --allow-read --allow-write docs/gerar_site_unico.ts
# gera docs/publicar/index.html (CSS e JS inline)
```

Depois, no repositório já criado (`FelipeFlau/smartwatch`):

1. **Add file → Upload files** e suba, na raiz:
   * `docs/publicar/index.html` → vira **`index.html`** (o site)
   * `docs/assets/versoes.js`, `docs/assets/estilo.css`, `docs/assets/app.js` (as fontes)
   * `docs/README.md` → suba como **`SITE.md`** (este guia)
2. **Commit changes** com uma mensagem (`site: v0.26`).
3. **Settings → Pages → Source**: `Deploy from a branch` → branch `main`, pasta `/ (root)` → **Save**.
4. Em ~1 minuto: <https://felipeflau.github.io/smartwatch/>

Atualizar = gerar de novo o arquivo único e subir o `index.html` (commit novo).

### Alternativa: repositório com git

Na pasta do projeto (`C:\Users\Flausino\Documents\Smartwatch`):

```powershell
git init
git add .
git commit -m "firmware + historico de versoes"
git branch -M main
git remote add origin https://github.com/SEU-USUARIO/smartwatch.git
git push -u origin main
```

Depois, no repositório no GitHub:

1. **Settings** → **Pages**
2. **Source**: `Deploy from a branch`
3. **Branch**: `main` e pasta **`/docs`** → **Save**
4. Em ~1 minuto: `https://SEU-USUARIO.github.io/smartwatch/`

Repositório privado: o Pages só funciona em plano pago. Para o site aparecer público
com o código privado, publique só a pasta `docs/` num repositório separado
(`git subtree push --prefix docs origin gh-pages`) — nesse caso o branch é `gh-pages`
e a pasta é `/root`.

## Atualizar o site publicado

```powershell
git add docs
git commit -m "site: v0.26"
git push
```

O Pages republica sozinho a cada push na branch configurada.
