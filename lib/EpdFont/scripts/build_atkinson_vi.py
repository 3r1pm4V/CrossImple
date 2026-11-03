#!/usr/bin/env python3
"""
Refined Atkinson Hyperlegible Vietnamese Builder
- Harmonizes 'đ' and 'Đ' using Atkinson's native 'd' and 'D' glyphs with properly scaled crossbars
- Adjusts all 94 Vietnamese diacritics to perfectly balance x-height and cap-height
- Eliminates the oversized dcroat/Dcroat disparity
"""
import os
import sys
from fontTools.ttLib import TTFont
from fontTools.ttLib.tables._g_l_y_f import Glyph, GlyphComponent
from fontTools.pens.ttGlyphPen import TTGlyphPen
from fontTools.pens.transformPen import TransformPen


def copy_scaled_simple_glyph(src_tt, src_gname, scale, dx=0, dy=0):
    pen = TTGlyphPen(src_tt.getGlyphSet())
    tpen = TransformPen(pen, (scale, 0, 0, scale, dx, dy))
    src_tt.getGlyphSet()[src_gname].draw(tpen)
    return pen.glyph()


def make_composite(components):
    """components: list of (glyphName, dx, dy)"""
    g = Glyph()
    g.numberOfContours = -1
    g.components = []
    for gname, dx, dy in components:
        comp = GlyphComponent()
        comp.glyphName = gname
        comp.x = int(round(dx))
        comp.y = int(round(dy))
        comp.flags = 0x0004  # ROUND_XY_TO_GRID
        g.components.append(comp)
    return g


def center_x(glyf, base_name, mark_name, offset_x=0):
    bg = glyf[base_name]
    mg = glyf[mark_name]
    bg.recalcBounds(glyf)
    mg.recalcBounds(glyf)
    bcx = (bg.xMin + bg.xMax) / 2.0
    mcx = (mg.xMin + mg.xMax) / 2.0
    return int(round(bcx - mcx + offset_x))


def build_vi_font(atkinson_path, inter_path, out_path, is_bold=False):
    atk = TTFont(atkinson_path)
    inter = TTFont(inter_path)

    scale = 1000.0 / 2048.0  # Atkinson upem=1000, Inter upem=2048

    glyf = atk["glyf"]
    hmtx = atk["hmtx"].metrics
    glyph_order = list(atk.getGlyphOrder())

    def add_glyph(name, glyph_obj, width, lsb=0):
        if name not in glyf.glyphs:
            glyph_order.append(name)
        glyf[name] = glyph_obj
        glyf[name].recalcBounds(glyf)
        hmtx[name] = (width, glyf[name].xMin if glyf[name].numberOfContours != 0 else lsb)

    # 1. Scaled helper marks from Inter
    lc_dy = -38
    uc_dy = 128

    helper_marks = [
        "hookabovecomb",
        "uni031B",
        "circumflexacutecomb.cn",
        "circumflexgravecomb.cn",
        "circumflexhookcomb.cn",
        "circumflextildecomb.cn",
        "breveacutecomb.cn",
        "brevegravecomb.cn",
        "uni03060309",
        "brevetildecomb.cn",
    ]
    for mname in helper_marks:
        g = copy_scaled_simple_glyph(inter, mname, scale, 0, lc_dy)
        add_glyph(mname, g, 0, 0)

    # Dot below
    dotbelow_g = make_composite([("uni0307", 0, -795)])
    add_glyph("dotbelowcomb", dotbelow_g, 0, 0)

    # 2. Perfect harmonious 'đ' and 'Đ' using native Atkinson 'd' and 'D'
    # Copy crossbars from Inter scaled to 1000 upem
    crossbar_g = copy_scaled_simple_glyph(inter, "crossbar", scale, 0, 0)
    add_glyph("crossbar_scaled", crossbar_g, 0, 0)
    crossbar_uc_g = copy_scaled_simple_glyph(inter, "crossbar_short.uc", scale, 0, 0)
    add_glyph("crossbar_uc_scaled", crossbar_uc_g, 0, 0)

    # Build harmonized dcroat (đ)
    # Native Atkinson 'd' ascender stem is on the right
    bg_d = glyf["d"]
    bg_d.recalcBounds(glyf)
    stem_x = bg_d.xMax - (60 if is_bold else 50)
    cb_g = glyf["crossbar_scaled"]
    cb_g.recalcBounds(glyf)
    cb_cx = (cb_g.xMin + cb_g.xMax) / 2.0
    cb_dx = stem_x - cb_cx
    cb_dy = (550 * scale) - 30  # Perfectly centered on the ascender
    new_dcroat = make_composite([("d", 0, 0), ("crossbar_scaled", cb_dx, cb_dy)])
    add_glyph("dcroat", new_dcroat, hmtx["d"][0])

    # Build harmonized Dcroat (Đ)
    # Native Atkinson 'D' vertical stem is on the left
    bg_D = glyf["D"]
    bg_D.recalcBounds(glyf)
    stem_D_x = bg_D.xMin + (40 if is_bold else 30)
    cb_uc = glyf["crossbar_uc_scaled"]
    cb_uc.recalcBounds(glyf)
    cb_uc_cx = (cb_uc.xMin + cb_uc.xMax) / 2.0
    cb_uc_dx = stem_D_x - cb_uc_cx
    cb_uc_dy = (667 * scale) - 15
    new_Dcroat = make_composite([("D", 0, 0), ("crossbar_uc_scaled", cb_uc_dx, cb_uc_dy)])
    add_glyph("Dcroat", new_Dcroat, hmtx["D"][0])

    # 3. Ohorn, ohorn, Uhorn, uhorn using Atkinson O, o, U, u
    horn_g = glyf["uni031B"]
    horn_g.recalcBounds(glyf)

    for cp, gname, base_name, target_x_offset, target_y_top in [
        (0x01A0, "Ohorn", "O", -35, 710),
        (0x01A1, "ohorn", "o", -30, 545),
        (0x01AF, "Uhorn", "U", -15, 710),
        (0x01B0, "uhorn", "u", -15, 545),
    ]:
        bg = glyf[base_name]
        bg.recalcBounds(glyf)
        dx = bg.xMax - horn_g.xMin + target_x_offset
        dy = target_y_top - horn_g.yMax
        comp_g = make_composite([(base_name, 0, 0), ("uni031B", dx, dy)])
        add_glyph(gname, comp_g, hmtx[base_name][0])

    # 4. Map all 90 Vietnamese precomposed characters (0x1EA0..0x1EF9)
    vi_specs = [
        (0x1EA0, "uni1EA0", "A", [("dotbelowcomb", False, 0, 0)]),
        (0x1EA1, "uni1EA1", "a", [("dotbelowcomb", False, 0, 0)]),
        (0x1EA2, "uni1EA2", "A", [("hookabovecomb", True, 0, 0)]),
        (0x1EA3, "uni1EA3", "a", [("hookabovecomb", False, 0, 0)]),
        (0x1EA4, "uni1EA4", "A", [("circumflexacutecomb.cn", True, 0, 0)]),
        (0x1EA5, "uni1EA5", "a", [("circumflexacutecomb.cn", False, 0, 0)]),
        (0x1EA6, "uni1EA6", "A", [("circumflexgravecomb.cn", True, 0, 0)]),
        (0x1EA7, "uni1EA7", "a", [("circumflexgravecomb.cn", False, 0, 0)]),
        (0x1EA8, "uni1EA8", "A", [("circumflexhookcomb.cn", True, 0, 0)]),
        (0x1EA9, "uni1EA9", "a", [("circumflexhookcomb.cn", False, 0, 0)]),
        (0x1EAA, "uni1EAA", "A", [("circumflextildecomb.cn", True, 0, 0)]),
        (0x1EAB, "uni1EAB", "a", [("circumflextildecomb.cn", False, 0, 0)]),
        (0x1EAC, "uni1EAC", "Acircumflex", [("dotbelowcomb", False, 0, 0)]),
        (0x1EAD, "uni1EAD", "acircumflex", [("dotbelowcomb", False, 0, 0)]),
        (0x1EAE, "uni1EAE", "A", [("breveacutecomb.cn", True, 0, 0)]),
        (0x1EAF, "uni1EAF", "a", [("breveacutecomb.cn", False, 0, 0)]),
        (0x1EB0, "uni1EB0", "A", [("brevegravecomb.cn", True, 0, 0)]),
        (0x1EB1, "uni1EB1", "a", [("brevegravecomb.cn", False, 0, 0)]),
        (0x1EB2, "uni1EB2", "A", [("uni03060309", True, 0, 0)]),
        (0x1EB3, "uni1EB3", "a", [("uni03060309", False, 0, 0)]),
        (0x1EB4, "uni1EB4", "A", [("brevetildecomb.cn", True, 0, 0)]),
        (0x1EB5, "uni1EB5", "a", [("brevetildecomb.cn", False, 0, 0)]),
        (0x1EB6, "uni1EB6", "Abreve", [("dotbelowcomb", False, 0, 0)]),
        (0x1EB7, "uni1EB7", "abreve", [("dotbelowcomb", False, 0, 0)]),

        (0x1EB8, "uni1EB8", "E", [("dotbelowcomb", False, 0, 0)]),
        (0x1EB9, "uni1EB9", "e", [("dotbelowcomb", False, 0, 0)]),
        (0x1EBA, "uni1EBA", "E", [("hookabovecomb", True, 0, 0)]),
        (0x1EBB, "uni1EBB", "e", [("hookabovecomb", False, 0, 0)]),
        (0x1EBC, "uni1EBC", "E", [("tildecomb.case", False, 0, 0)]),
        (0x1EBD, "uni1EBD", "e", [("tildecomb", False, 0, 0)]),
        (0x1EBE, "uni1EBE", "E", [("circumflexacutecomb.cn", True, 0, 0)]),
        (0x1EBF, "uni1EBF", "e", [("circumflexacutecomb.cn", False, 0, 0)]),
        (0x1EC0, "uni1EC0", "E", [("circumflexgravecomb.cn", True, 0, 0)]),
        (0x1EC1, "uni1EC1", "e", [("circumflexgravecomb.cn", False, 0, 0)]),
        (0x1EC2, "uni1EC2", "E", [("circumflexhookcomb.cn", True, 0, 0)]),
        (0x1EC3, "uni1EC3", "e", [("circumflexhookcomb.cn", False, 0, 0)]),
        (0x1EC4, "uni1EC4", "E", [("circumflextildecomb.cn", True, 0, 0)]),
        (0x1EC5, "uni1EC5", "e", [("circumflextildecomb.cn", False, 0, 0)]),
        (0x1EC6, "uni1EC6", "Ecircumflex", [("dotbelowcomb", False, 0, 0)]),
        (0x1EC7, "uni1EC7", "ecircumflex", [("dotbelowcomb", False, 0, 0)]),

        (0x1EC8, "uni1EC8", "I", [("hookabovecomb", True, 0, 0)]),
        (0x1EC9, "uni1EC9", "dotlessi", [("hookabovecomb", False, 0, 0)]),
        (0x1ECA, "uni1ECA", "I", [("dotbelowcomb", False, 0, 0)]),
        (0x1ECB, "uni1ECB", "i", [("dotbelowcomb", False, 0, 0)]),

        (0x1ECC, "uni1ECC", "O", [("dotbelowcomb", False, 0, 0)]),
        (0x1ECD, "uni1ECD", "o", [("dotbelowcomb", False, 0, 0)]),
        (0x1ECE, "uni1ECE", "O", [("hookabovecomb", True, 0, 0)]),
        (0x1ECF, "uni1ECF", "o", [("hookabovecomb", False, 0, 0)]),
        (0x1ED0, "uni1ED0", "O", [("circumflexacutecomb.cn", True, 0, 0)]),
        (0x1ED1, "uni1ED1", "o", [("circumflexacutecomb.cn", False, 0, 0)]),
        (0x1ED2, "uni1ED2", "O", [("circumflexgravecomb.cn", True, 0, 0)]),
        (0x1ED3, "uni1ED3", "o", [("circumflexgravecomb.cn", False, 0, 0)]),
        (0x1ED4, "uni1ED4", "O", [("circumflexhookcomb.cn", True, 0, 0)]),
        (0x1ED5, "uni1ED5", "o", [("circumflexhookcomb.cn", False, 0, 0)]),
        (0x1ED6, "uni1ED6", "O", [("circumflextildecomb.cn", True, 0, 0)]),
        (0x1ED7, "uni1ED7", "o", [("circumflextildecomb.cn", False, 0, 0)]),
        (0x1ED8, "uni1ED8", "Ocircumflex", [("dotbelowcomb", False, 0, 0)]),
        (0x1ED9, "uni1ED9", "ocircumflex", [("dotbelowcomb", False, 0, 0)]),

        (0x1EDA, "uni1EDA", "Ohorn", [("acutecomb.case", False, 0, 0)]),
        (0x1EDB, "uni1EDB", "ohorn", [("acutecomb", False, 0, 0)]),
        (0x1EDC, "uni1EDC", "Ohorn", [("gravecomb.case", False, 0, 0)]),
        (0x1EDD, "uni1EDD", "ohorn", [("gravecomb", False, 0, 0)]),
        (0x1EDE, "uni1EDE", "Ohorn", [("hookabovecomb", True, 0, 0)]),
        (0x1EDF, "uni1EDF", "ohorn", [("hookabovecomb", False, 0, 0)]),
        (0x1EE0, "uni1EE0", "Ohorn", [("tildecomb.case", False, 0, 0)]),
        (0x1EE1, "uni1EE1", "ohorn", [("tildecomb", False, -20, 0)]),
        (0x1EE2, "uni1EE2", "Ohorn", [("dotbelowcomb", False, 0, 0)]),
        (0x1EE3, "uni1EE3", "ohorn", [("dotbelowcomb", False, 0, 0)]),

        (0x1EE4, "uni1EE4", "U", [("dotbelowcomb", False, 0, 0)]),
        (0x1EE5, "uni1EE5", "u", [("dotbelowcomb", False, 0, 0)]),
        (0x1EE6, "uni1EE6", "U", [("hookabovecomb", True, 0, 0)]),
        (0x1EE7, "uni1EE7", "u", [("hookabovecomb", False, 0, 0)]),
        (0x1EE8, "uni1EE8", "Uhorn", [("acutecomb.case", False, 0, 0)]),
        (0x1EE9, "uni1EE9", "uhorn", [("acutecomb", False, 0, 0)]),
        (0x1EEA, "uni1EEA", "Uhorn", [("gravecomb.case", False, 0, 0)]),
        (0x1EEB, "uni1EEB", "uhorn", [("gravecomb", False, 0, 0)]),
        (0x1EEC, "uni1EEC", "Uhorn", [("hookabovecomb", True, 0, 0)]),
        (0x1EED, "uni1EED", "uhorn", [("hookabovecomb", False, 0, 0)]),
        (0x1EEE, "uni1EEE", "Uhorn", [("tildecomb.case", False, 0, 0)]),
        (0x1EEF, "uni1EEF", "uhorn", [("tildecomb", False, -20, 0)]),
        (0x1EF0, "uni1EF0", "Uhorn", [("dotbelowcomb", False, 0, 0)]),
        (0x1EF1, "uni1EF1", "uhorn", [("dotbelowcomb", False, 0, 0)]),

        (0x1EF2, "uni1EF2", "Y", [("gravecomb.case", False, 0, 0)]),
        (0x1EF3, "uni1EF3", "y", [("gravecomb", False, 0, 0)]),
        (0x1EF4, "uni1EF4", "Y", [("dotbelowcomb", False, 0, 0)]),
        (0x1EF5, "uni1EF5", "y", [("dotbelowcomb", False, 90, 0)]),
        (0x1EF6, "uni1EF6", "Y", [("hookabovecomb", True, 0, 0)]),
        (0x1EF7, "uni1EF7", "y", [("hookabovecomb", False, 0, 0)]),
        (0x1EF8, "uni1EF8", "Y", [("tildecomb.case", False, 0, 0)]),
        (0x1EF9, "uni1EF9", "y", [("tildecomb", False, 0, 0)]),
    ]

    cmap_add = {
        0x0110: "Dcroat",
        0x0111: "dcroat",
        0x01A0: "Ohorn",
        0x01A1: "ohorn",
        0x01AF: "Uhorn",
        0x01B0: "uhorn",
    }

    for cp, gname, base_name, marks in vi_specs:
        align_base = base_name
        if base_name in ("Ohorn", "Ocircumflex"):
            align_base = "O"
        elif base_name in ("ohorn", "ocircumflex"):
            align_base = "o"
        elif base_name == "Uhorn":
            align_base = "U"
        elif base_name == "uhorn":
            align_base = "u"
        elif base_name in ("Acircumflex", "Abreve"):
            align_base = "A"
        elif base_name in ("acircumflex", "abreve"):
            align_base = "a"
        elif base_name == "Ecircumflex":
            align_base = "E"
        elif base_name == "ecircumflex":
            align_base = "e"

        comps = [(base_name, 0, 0)]
        for mname, is_uc, ex_dx, ex_dy in marks:
            dx = center_x(glyf, align_base, mname, ex_dx)
            dy = (uc_dy if is_uc else 0) + ex_dy
            comps.append((mname, dx, dy))
        comp_g = make_composite(comps)
        add_glyph(gname, comp_g, hmtx[base_name][0])
        cmap_add[cp] = gname

    atk.setGlyphOrder(glyph_order)
    for table in atk["cmap"].tables:
        if table.isUnicode():
            table.cmap.update(cmap_add)

    atk["maxp"].numGlyphs = len(glyph_order)
    atk.save(out_path)
    print(f"Saved {out_path} ({len(glyph_order)} glyphs, +{len(cmap_add)} Vietnamese codepoints)")


if __name__ == "__main__":
    base_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    atk_dir = os.path.join(base_dir, "builtinFonts", "source", "AtkinsonHyperlegible")
    inter_dir = os.path.join(base_dir, "builtinFonts", "source", "Inter")

    build_vi_font(
        os.path.join(atk_dir, "AtkinsonHyperlegible-Regular.ttf"),
        os.path.join(inter_dir, "Inter-Regular.ttf"),
        os.path.join(atk_dir, "AtkinsonHyperlegible-Regular-VI.ttf"),
        is_bold=False,
    )
    build_vi_font(
        os.path.join(atk_dir, "AtkinsonHyperlegible-Bold.ttf"),
        os.path.join(inter_dir, "Inter-Bold.ttf"),
        os.path.join(atk_dir, "AtkinsonHyperlegible-Bold-VI.ttf"),
        is_bold=True,
    )
