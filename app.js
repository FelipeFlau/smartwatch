// ============================================================
//  Render do historico. Depende de assets/versoes.js (window.VERSOES).
// ============================================================

(function () {
  "use strict";

  var lista = document.getElementById("lista");
  var campo = document.getElementById("busca");
  var filtros = Array.prototype.slice.call(document.querySelectorAll(".filtro"));
  var aviso = document.getElementById("resultado");

  var dados = Array.isArray(window.VERSOES) ? window.VERSOES.slice() : [];
  var filtroAtual = "todos";

  // ---------- utilidades ----------

  function escapa(s) {
    return String(s)
      .replace(/&/g, "&amp;")
      .replace(/</g, "&lt;")
      .replace(/>/g, "&gt;")
      .replace(/"/g, "&quot;");
  }

  // `texto` -> <code>texto</code>, depois de escapar o HTML.
  function inline(s) {
    return escapa(s).replace(/`([^`]+)`/g, "<code>$1</code>");
  }

  function idDe(v) {
    return v.toLowerCase().replace(/\./g, "-");
  }

  function quandoTexto(item) {
    if (item.data) {
      var p = item.data.split("-");
      return p[2] + "/" + p[1] + "/" + p[0];
    }
    return item.quando || "";
  }

  function plural(n, um, muitos) {
    return n + " " + (n === 1 ? um : muitos);
  }

  // "associacao" tambem acha "associação"
  function semAcento(s) {
    return String(s).normalize("NFD").replace(/[\u0300-\u036f]/g, "");
  }

  function textoBusca(item) {
    return semAcento(
      [
        item.v,
        item.data || "",
        item.quando || "",
        item.titulo,
        item.resumo,
        item.fonte || "",
        (item.corrigido || []).join(" "),
        (item.adicionado || []).join(" "),
        (item.alterado || []).join(" "),
        (item.arquivos || []).join(" "),
        (item.pendente || []).join(" ")
      ]
        .join(" ")
        .toLowerCase()
    );
  }

  // ---------- montagem ----------

  function grupo(classe, rotulo, itens) {
    if (!itens || !itens.length) return "";
    var lis = itens.map(function (t) { return "<li>" + inline(t) + "</li>"; }).join("");
    return (
      '<div class="grupo ' + classe + '">' +
      "<h3>" + rotulo + "</h3>" +
      "<ul>" + lis + "</ul>" +
      "</div>"
    );
  }

  function cartao(item, indice) {
    var id = idDe(item.v);
    var nF = (item.corrigido || []).length;
    var nA = (item.adicionado || []).length;
    var nC = (item.alterado || []).length;
    var nP = (item.pendente || []).length;
    var estadoRotulo =
      item.estado === "hw" ? "validar no relogio" :
      item.estado === "aberto" ? "bug aberto" : "resolvido";

    var contagem = [];
    if (nF) contagem.push("<span>" + plural(nF, "correcao", "correcoes") + "</span>");
    if (nA) contagem.push("<span>" + plural(nA, "adicao", "adicoes") + "</span>");
    if (nC) contagem.push("<span>" + plural(nC, "mudanca", "mudancas") + "</span>");
    if (nP) contagem.push("<span>" + plural(nP, "pendencia", "pendencias") + "</span>");

    var rodape = "";
    if (item.arquivos && item.arquivos.length) {
      rodape +=
        '<div><span class="rot">Arquivos</span>' +
        item.arquivos.map(inline).join(" · ") +
        "</div>";
    }
    if (item.fonte) {
      rodape += '<div><span class="rot">Fonte</span>' + inline(item.fonte) + "</div>";
    }
    if (item.pendente && item.pendente.length) {
      rodape +=
        '<div class="pendente"><span class="rot">Pendente</span>' +
        item.pendente.map(inline).join(" | ") +
        "</div>";
    }

    return (
      '<div class="linha">' +
      '<div class="quando">' +
      '<b><a href="#' + id + '">' + escapa(item.v) + "</a></b>" +
      "<span>" + escapa(quandoTexto(item)) + "</span>" +
      "</div>" +
      '<div class="card">' +
      "<details" + (indice === 0 ? " open" : "") + ' id="' + id + '">' +
      "<summary>" +
      '<div class="titulo"><h2>' + inline(item.titulo) + "</h2>" +
      '<span class="estado ' + (item.estado || "ok") + '">' + estadoRotulo + "</span>" +
      "</div>" +
      '<div class="contagem">' + contagem.join("") + "</div>" +
      '<span class="seta" aria-hidden="true">' +
      '<svg viewBox="0 0 16 16" fill="none" stroke="currentColor" stroke-width="1.8" ' +
      'stroke-linecap="round" stroke-linejoin="round"><path d="M5 3l5 5-5 5"/></svg>' +
      "</span>" +
      "</summary>" +
      '<div class="corpo">' +
      '<p class="resumo">' + inline(item.resumo) + "</p>" +
      '<div class="grupos">' +
      grupo("corrigido", "Corrigido", item.corrigido) +
      grupo("adicionado", "Adicionado", item.adicionado) +
      grupo("alterado", "Alterado", item.alterado) +
      "</div>" +
      (rodape ? '<div class="rodape-versao">' + rodape + "</div>" : "") +
      "</div>" +
      "</details>" +
      "</div>" +
      "</div>"
    );
  }

  function render() {
    var termo = semAcento((campo.value || "").trim().toLowerCase());
    var visiveis = dados.filter(function (item) {
      if (filtroAtual !== "todos" && (item.estado || "ok") !== filtroAtual) return false;
      if (!termo) return true;
      return textoBusca(item).indexOf(termo) !== -1;
    });

    if (!visiveis.length) {
      lista.innerHTML = '<p class="vazio">Nenhuma versao para esse filtro.</p>';
      aviso.textContent = "0 versoes";
      return;
    }

    lista.innerHTML = visiveis.map(cartao).join("");

    // mantem o item aberto quando o filtro nao muda a ordem visivel
    var primeiro = lista.querySelector("details");
    if (primeiro) primeiro.open = true;

    aviso.textContent =
      "Mostrando " + visiveis.length + " de " + dados.length + " versoes.";

    ligarAncoras();
  }

  // abrir a versao apontada pelo #hash
  function ligarAncoras() {
    var alvo = location.hash.replace("#", "");
    if (!alvo) return;
    var el = document.getElementById(alvo);
    if (!el) return;
    var det = el.tagName === "DETAILS" ? el : el.querySelector("details");
    if (det) det.open = true;
  }

  // ---------- filtros ----------

  filtros.forEach(function (btn) {
    btn.addEventListener("click", function () {
      filtroAtual = btn.dataset.estado;
      filtros.forEach(function (b) {
        b.setAttribute("aria-pressed", String(b === btn));
      });
      render();
    });
  });

  campo.addEventListener("input", render);

  window.addEventListener("hashchange", ligarAncoras);

  // ---------- contadores nos chips ----------

  function contar(estado) {
    return dados.filter(function (i) {
      return estado === "todos" || (i.estado || "ok") === estado;
    }).length;
  }

  filtros.forEach(function (btn) {
    var n = btn.querySelector(".n");
    if (n) n.textContent = contar(btn.dataset.estado);
  });

  // ---------- cabecalho: totais ----------

  var totalEl = document.getElementById("total-versoes");
  var ultimaEl = document.getElementById("ultima-data");
  if (totalEl) totalEl.textContent = String(dados.length);
  if (ultimaEl) {
    var comData = dados.filter(function (i) { return i.data; });
    if (comData.length) ultimaEl.textContent = quandoTexto(comData[0]);
  }

  // ---------- tema ----------

  var botaoTema = document.getElementById("tema");
  var raiz = document.documentElement;

  function aplicarTema(t) {
    raiz.setAttribute("data-tema", t);
    if (botaoTema) {
      botaoTema.textContent = t === "claro" ? "Tema escuro" : "Tema claro";
      botaoTema.setAttribute("aria-pressed", String(t === "claro"));
    }
  }

  var salvo = null;
  try { salvo = localStorage.getItem("tema-firmware"); } catch (e) { salvo = null; }

  var prefereClaro =
    window.matchMedia && window.matchMedia("(prefers-color-scheme: light)").matches;

  aplicarTema(salvo || (prefereClaro ? "claro" : "escuro"));

  if (botaoTema) {
    botaoTema.addEventListener("click", function () {
      var novo = raiz.getAttribute("data-tema") === "claro" ? "escuro" : "claro";
      aplicarTema(novo);
      try { localStorage.setItem("tema-firmware", novo); } catch (e) {}
    });
  }

  // ---------- inicio ----------

  if (!dados.length) {
    lista.innerHTML =
      '<p class="vazio">Nao carregou assets/versoes.js (window.VERSOES vazio).</p>';
    return;
  }

  render();
})();
