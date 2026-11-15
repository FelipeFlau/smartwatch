#!/usr/bin/env python3
# ============================================================================
#  gerar_art.py - regenera o home_art.h a partir dos prints do usuario
#
#  Entradas (na pasta Downloads):
#    - apps.png        Home 127x160 (print do menu de apps)
#    - padraoface.png  Face padrao 127x160 (print HH em cima / MM embaixo)
#
#  Saida:
#    home_art.h  com  home_art_565, home_diamond_565, face_diamond_565,
#                    face_rings_565 e face_bat_mask  (RGB565, LV_COLOR_DEPTH 16)
#
#  Como rodar:  python gerar_art.py
#  Requer:      pip install pillow
#
#  Medidas usadas pelo firmware (nao mudar sem remedir o print):
#    Home: header y0..17 (dinamico), arte y=18..159
#          icones 38x38 em (0,31) (41,30) (85,33) / (1,74) (42,76) (85,77)
#                        / (1,118) (43,118) (85,119)
#    Face padrao: "12" ink x30..73 y38..71 | "59" ink x60..104 y88..121
#          aneis x10..53 y86..131 | diamante x76..86 y4..14
#          "%" ate x112 | pilula x116..122 y4..14
# ============================================================================
import os
from PIL import Image

DOWNLOADS = os.path.join(os.path.expanduser("~"), "Downloads")
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "home_art.h")


def find(prefix):
    for f in os.listdir(DOWNLOADS):
        if f.startswith(prefix) and f.lower().endswith(".png"):
            return os.path.join(DOWNLOADS, f)
    raise SystemExit("nao achei %s*.png em %s" % (prefix, DOWNLOADS))


def rgb565(c):
    r, g, b = c
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)


def emit(out, name, vals, per_line=12):
    out.append("static const uint16_t %s[] = {" % name)
    for i in range(0, len(vals), per_line):
        out.append("  " + ", ".join("0x%04X" % v for v in vals[i:i + per_line]) + ",")
    out.append("};")


def crop(img, x0, y0, w, h):
    px = img.load()
    return [rgb565(px[x, y]) for y in range(y0, y0 + h) for x in range(x0, x0 + w)]


def main():
    apps = Image.open(find("apps")).convert("RGB")
    face = Image.open(find("padr")).convert("RGB")

    out = ["// ============================================================",
           "//  HOME ART - extraido pixel a pixel dos prints (gerar_art.py)",
           "//  apps.png = Home, padraoface.png = face padrao. RGB565 direto.",
           "// ============================================================",
           "#pragma once",
           "#include <stdint.h>"]

    # ---- Home: recorte abaixo do header (y=18..159), 128 de largura ----
    W, H = 128, 142
    bg = apps.getpixel((0, 0))
    px = apps.load()
    vals = []
    for y in range(18, 160):
        for x in range(W):
            vals.append(rgb565(px[x, y] if x < apps.size[0] else bg))
    out.append("#define HOME_ART_W %d" % W)
    out.append("#define HOME_ART_H %d" % H)
    emit(out, "home_art_565", vals)

    # ---- icones do topo (diamante) ----
    emit(out, "home_diamond_565", crop(apps, 74, 5, 13, 13))
    emit(out, "face_diamond_565", crop(face, 76, 4, 11, 11))

    # ---- pilula da bateria 7x11: 0=fundo 1=borda clara 2=verde ----
    out.append("// pilula 7x11: 0=fundo 1=borda clara 2=preenchimento")
    out.append("static const uint8_t face_bat_mask[] = {")
    for y in range(4, 15):
        row = []
        for x in range(116, 123):
            r, g, b = face.getpixel((x, y))
            if r + g + b < 60:
                v = 0
            elif g > 110 and g > r + 30:
                v = 2
            else:
                v = 1
            row.append(v)
        out.append("  " + ", ".join(str(v) for v in row) + ",")
    out.append("};")

    # ---- 3 aneis da face padrao ----
    out.append("// aneis de atividade da face padrao (x9..54 y85..132)")
    out.append("#define FACE_RINGS_W 46")
    out.append("#define FACE_RINGS_H 48")
    emit(out, "face_rings_565", crop(face, 9, 85, 46, 48))

    with open(OUT, "w", encoding="utf-8") as f:
        f.write("\n".join(out) + "\n")
    print("ok:", OUT, os.path.getsize(OUT), "bytes")


if __name__ == "__main__":
    main()
