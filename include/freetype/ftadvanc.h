#include "../../freeink-sdk/libs/font/FreeInkFont/third_party/freetype/include/freetype/ftadvanc.h"

/*
 * CrossImple: When FreeType's auto-hinter runs with stem adjustment enabled
 * (HintingMode::Auto / FT_RENDER_MODE_NORMAL), also enable horizontal and
 * vertical stem snapping (AF_LATIN_HINTS_HORZ_SNAP | AF_LATIN_HINTS_VERT_SNAP).
 *
 * Without VERT_SNAP, Vietnamese horned vowels (ơ, ư, ờ..ự) flagged with
 * AF_IGNORE_SMALL_TOP in afadjust.c detach from the lowercase top blue zone
 * and receive fractional stem heights and sub-pixel edge positions, making
 * their strokes visibly thinner/lighter than base vowels (o, u, a, e).
 * Enabling HORZ_SNAP | VERT_SNAP alongside STEM_ADJUST snaps horizontal stems
 * to exact integer pixels and uses FreeType's anti-aliased stem-width snapping
 * for vertical stems, producing uniform stroke weights across Vietnamese text.
 */
#if defined(AFBLUE_H_) && !defined(AFLATIN_H_)
#include "../../freeink-sdk/libs/font/FreeInkFont/third_party/freetype/src/autofit/afglobal.h"
#include "../../freeink-sdk/libs/font/FreeInkFont/third_party/freetype/src/autofit/aflatin.h"
#endif

#if defined(AFLATIN_H_) && defined(AF_LATIN_HINTS_STEM_ADJUST)
#undef AF_LATIN_HINTS_STEM_ADJUST
#define AF_LATIN_HINTS_STEM_ADJUST \
  ( ( 1U << 2 ) | AF_LATIN_HINTS_HORZ_SNAP | AF_LATIN_HINTS_VERT_SNAP )
#endif
