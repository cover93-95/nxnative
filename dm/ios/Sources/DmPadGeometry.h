// Where the virtual gamepad's controls are drawn (DmGamepad.m), as pure functions of the view's bounds, its safe-area insets, the DmLayout split
// and the player's options, so the placement is tested without a device (dm/ios/tests/test_pad_geometry.py).
//   Every length is in u (1 = 430 points tall, iPhone Pro Max landscape), x measured from the outer edge, the right half mirrors.
//   The options come from the environment (the shell's settings, Documents/env.txt): DM_TOUCH_SIZE = small | medium | large scales u.
//   DM_TOUCH_STICK = corner | higher | inward places the left stick (inward takes the D-pad along); the right-hand controls never move.
//   An option never moves a control off the screen or onto another one: a size that does not fit falls back step by step to the size that does
//   (dm_pad_geo_fits: every drawn shape inside its rectangle, no two drawn shapes overlap).
#pragma once
#import "DmLayout.h"
#include <math.h>
#include <string.h>

NS_ASSUME_NONNULL_BEGIN

enum { DM_PAD_ZL = 0, DM_PAD_L, DM_PAD_ZR, DM_PAD_R };   // the order of DmPadGeo.drop
enum { DM_PAD_MINUS = 0, DM_PAD_PLUS };                  // the order of DmPadGeo.pill

enum { DM_PAD_STICK_CORNER = 0, DM_PAD_STICK_HIGHER, DM_PAD_STICK_INWARD };   // DM_TOUCH_STICK
typedef struct {
    CGFloat scale;   // DM_TOUCH_SIZE: 0.8 small, 1 medium (the built-in size), 1.2 large
    int stick;       // DM_TOUCH_STICK: where the left stick sits (the D-pad moves with it when it moves inward); the right-hand controls keep their place
} DmPadOptions;

typedef struct {
    CGFloat u;                            // the unit every size below is a multiple of
    CGRect safe, corner;                  // the top row lives in safe, the bottom-corner controls in corner (reaching halfway into the side insets)
    BOOL portrait;                        // separated and taller than wide: both thumbs' clusters sit in the controls region's middle band
    CGPoint face; CGFloat faceGap, faceR, faceHitR;   // the A B X Y diamond: centre, key distance from it, key radius, hit radius
    CGPoint stick; CGFloat stickR, knobR;             // the left stick: ring centre and radius, knob radius
    CGPoint dpad; CGFloat dpadR, armW, armIn;         // the D-pad (3D only): centre, arm outer edge, arm half-width, arm tip from the centre
    CGPoint drop[4]; CGFloat dropR[4];                // ZL, L, ZR, R: teardrop centres and radii
    CGPoint click[2]; CGFloat clickR;                 // L3, R3 (3D only)
    CGRect pill[2];                                   // −, +
    CGPoint swap; CGFloat swapR;                      // the menu button
} DmPadGeo;

// Layout constants (in u), the values DmGamepad.m has always used.
static const CGFloat
    DM_PAD_ZL_X = 44, DM_PAD_ZL_Y = 40, DM_PAD_ZL_R = 26,     // ZL / ZR: centre from the outer edge and the top, radius
    DM_PAD_L_X = 44, DM_PAD_L_Y = 108, DM_PAD_L_R = 24,       // L / R: same column as ZL / ZR
    DM_PAD_DROP_TIP = 1.45,                                   // teardrop tip distance from the centre (times the radius)
    DM_PAD_CORNER_IN = 0.5,                                   // how far the bottom-corner controls reach into the side safe-area inset
    DM_PAD_FACE_X = 81, DM_PAD_Y = 82,                        // diamond centre from the outer edge; diamond and stick centre from the bottom
    DM_PAD_FACE_GAP = 50, DM_PAD_FACE_R = 27,                 // diamond: key distance from the centre, key radius
    DM_PAD_PILL_DX = 52, DM_PAD_PILL_Y = 22, DM_PAD_PILL_W = 50, DM_PAD_PILL_H = 28,   // − / +: centre from the midline and from the top, size
    DM_PAD_SWAP_R = 15,                                       // menu button: top centre, level with − / +
    DM_PAD_STICK_X = 81, DM_PAD_STICK_R = 77, DM_PAD_KNOB_R = 30,   // left stick: mirrors the diamond
    DM_PAD_LOW_X = 231, DM_PAD_LOW_Y = 64,                    // D-pad centre from the outer edge and the bottom
    DM_PAD_DPAD_R = 48, DM_PAD_ARM_W = 17, DM_PAD_ARM_IN = 11,
    DM_PAD_CLICK_X = 102, DM_PAD_CLICK_R = 17;                // L3 / R3: centre from the outer edge (level with ZL / ZR), radius

// DM_TOUCH_SIZE: small, medium (also unset, empty or unknown) or large.
static inline CGFloat dm_pad_size_scale(const char *_Nullable v) {
    if (v && !strcmp(v, "small")) return 0.8;
    if (v && !strcmp(v, "large")) return 1.2;
    return 1;
}

// DM_TOUCH_STICK: corner (also unset, empty or unknown), higher or inward.
static inline int dm_pad_stick_place(const char *_Nullable v) {
    if (v && !strcmp(v, "higher")) return DM_PAD_STICK_HIGHER;
    if (v && !strcmp(v, "inward")) return DM_PAD_STICK_INWARD;
    return DM_PAD_STICK_CORNER;
}
static const CGFloat DM_PAD_STICK_UP = 70, DM_PAD_STICK_IN = 60;   // how far higher / inward moves the left stick (in u), at most

static inline CGPoint dm_pad_side(CGRect s, int right, CGFloat dx, CGFloat y) { return CGPointMake(right ? CGRectGetMaxX(s) - dx : CGRectGetMinX(s) + dx, y); }

// The geometry at one unit, with no fitting.
static inline DmPadGeo dm_pad_geo_at(CGRect bounds, UIEdgeInsets insets, DmLayout layout, BOOL full, CGFloat u) {
    DmPadGeo g; memset(&g, 0, sizeof g);
    BOOL sep = layout.separated;
    g.u = u;
    g.safe = CGRectIntersection(layout.controls, UIEdgeInsetsInsetRect(bounds, dm_game_safe_insets(bounds, insets, layout.topSafeOnly)));
    UIEdgeInsets ci = insets; ci.left *= DM_PAD_CORNER_IN; ci.right *= DM_PAD_CORNER_IN;
    g.corner = sep ? g.safe : UIEdgeInsetsInsetRect(bounds, ci);
    g.portrait = sep && bounds.size.height > bounds.size.width;
    CGRect s = g.safe, c = g.corner;
    CGPoint thumbL = CGPointMake(CGRectGetMinX(s) + s.size.width * 0.28, CGRectGetMinY(s) + s.size.height * 0.58);
    CGPoint thumbR = CGPointMake(CGRectGetMinX(s) + s.size.width * 0.72, thumbL.y);
    g.faceGap = DM_PAD_FACE_GAP * u; g.faceR = DM_PAD_FACE_R * u; g.faceHitR = MAX(g.faceR * 1.25, sep ? 22 : 0);
    g.stickR = DM_PAD_STICK_R * u; g.knobR = DM_PAD_KNOB_R * u;
    g.dpadR = DM_PAD_DPAD_R * u; g.armW = DM_PAD_ARM_W * u; g.armIn = DM_PAD_ARM_IN * u;
    if (g.portrait) {
        g.face = thumbR;
        g.stick = thumbL; if (full) g.stick.x -= g.dpadR + 6 * u;
        g.dpad = CGPointMake(thumbL.x + g.stickR + 6 * u, thumbL.y);
    } else {
        CGFloat edge = sep ? g.faceGap + g.faceHitR : 0;
        g.face = dm_pad_side(c, 1, MAX(DM_PAD_FACE_X * u, edge), CGRectGetMaxY(c) - MAX(DM_PAD_Y * u, edge));
        g.stick = dm_pad_side(c, 0, DM_PAD_STICK_X * u, CGRectGetMaxY(c) - DM_PAD_Y * u);
        g.dpad = dm_pad_side(c, 0, DM_PAD_LOW_X * u, CGRectGetMaxY(c) - DM_PAD_LOW_Y * u);
    }
    CGFloat edge = sep ? 22 : 0;
    for (int i = 0; i < 4; i++) {
        BOOL z = i == DM_PAD_ZL || i == DM_PAD_ZR;
        g.drop[i] = dm_pad_side(s, i >= DM_PAD_ZR, MAX((z ? DM_PAD_ZL_X : DM_PAD_L_X) * u, edge), CGRectGetMinY(s) + MAX((z ? DM_PAD_ZL_Y : DM_PAD_L_Y) * u, edge));
        g.dropR[i] = (z ? DM_PAD_ZL_R : DM_PAD_L_R) * u;
    }
    for (int i = 0; i < 2; i++) g.click[i] = dm_pad_side(s, i, MAX(DM_PAD_CLICK_X * u, edge), CGRectGetMinY(s) + MAX(DM_PAD_ZL_Y * u, edge));
    g.clickR = DM_PAD_CLICK_R * u;
    CGFloat top = CGRectGetMinY(s) + MAX(DM_PAD_PILL_Y * u, sep ? 14 : 0), w = DM_PAD_PILL_W * u, h = DM_PAD_PILL_H * u;
    for (int i = 0; i < 2; i++) {
        CGFloat x = CGRectGetMidX(s) + (i == DM_PAD_PLUS ? DM_PAD_PILL_DX : -DM_PAD_PILL_DX) * u;   // − on the left, + on the right
        g.pill[i] = CGRectMake(x - w / 2, top - h / 2, w, h);
    }
    g.swap = CGPointMake(CGRectGetMidX(s), top); g.swapR = DM_PAD_SWAP_R * u;
    return g;
}

// The face key positions: A right, B bottom, X top, Y left (i = 0..3 in that order).
static inline CGPoint dm_pad_face_key(const DmPadGeo *g, int i) {
    CGPoint c = g->face; CGFloat d = g->faceGap;
    switch (i) { case 0: return CGPointMake(c.x + d, c.y); case 1: return CGPointMake(c.x, c.y + d); case 2: return CGPointMake(c.x, c.y - d); default: return CGPointMake(c.x - d, c.y); }
}
static inline CGPoint dm_pad_drop_tip(const DmPadGeo *g, int i) {
    CGFloat a = i >= DM_PAD_ZR ? -M_PI_4 : -3 * M_PI_4, d = g->dropR[i] * DM_PAD_DROP_TIP;   // tip toward the near top corner
    return CGPointMake(g->drop[i].x + d * cos(a), g->drop[i].y + d * sin(a));
}

// The drawn shapes as circles and rectangles, for the fit test and for tests: each with the rectangle it must stay inside.
typedef struct { CGPoint c; CGFloat r; CGRect rect; BOOL isRect; CGRect within; int id; } DmPadShape;
enum { DM_PAD_SHAPE_FACE = 0, DM_PAD_SHAPE_STICK = 4, DM_PAD_SHAPE_DPAD, DM_PAD_SHAPE_DROP, DM_PAD_SHAPE_TIP = 10, DM_PAD_SHAPE_CLICK = 14, DM_PAD_SHAPE_PILL = 16, DM_PAD_SHAPE_SWAP = 18 };
static inline int dm_pad_shapes(const DmPadGeo *g, BOOL full, DmPadShape out[_Nonnull 20]) {
    int n = 0;
    for (int i = 0; i < 4; i++) out[n++] = (DmPadShape){dm_pad_face_key(g, i), g->faceR, CGRectNull, NO, g->corner, DM_PAD_SHAPE_FACE + i};
    out[n++] = (DmPadShape){g->stick, g->stickR, CGRectNull, NO, g->corner, DM_PAD_SHAPE_STICK};
    if (full) out[n++] = (DmPadShape){g->dpad, hypot(g->dpadR, g->armW), CGRectNull, NO, g->corner, DM_PAD_SHAPE_DPAD};
    for (int i = 0; i < 4; i++) {
        out[n++] = (DmPadShape){g->drop[i], g->dropR[i], CGRectNull, NO, g->safe, DM_PAD_SHAPE_DROP + i};
        out[n++] = (DmPadShape){dm_pad_drop_tip(g, i), 0, CGRectNull, NO, g->safe, DM_PAD_SHAPE_TIP + i};
    }
    if (full) for (int i = 0; i < 2; i++) out[n++] = (DmPadShape){g->click[i], g->clickR, CGRectNull, NO, g->safe, DM_PAD_SHAPE_CLICK + i};
    for (int i = 0; i < 2; i++) out[n++] = (DmPadShape){CGPointZero, 0, g->pill[i], YES, g->safe, DM_PAD_SHAPE_PILL + i};
    out[n++] = (DmPadShape){g->swap, g->swapR, CGRectNull, NO, g->safe, DM_PAD_SHAPE_SWAP};
    return n;
}
static inline BOOL dm_pad_shape_inside(const DmPadShape *s) {
    const CGFloat e = 0.5;
    CGRect b = s->isRect ? s->rect : CGRectMake(s->c.x - s->r, s->c.y - s->r, 2 * s->r, 2 * s->r);
    return CGRectGetMinX(b) >= CGRectGetMinX(s->within) - e && CGRectGetMaxX(b) <= CGRectGetMaxX(s->within) + e
        && CGRectGetMinY(b) >= CGRectGetMinY(s->within) - e && CGRectGetMaxY(b) <= CGRectGetMaxY(s->within) + e;
}
static inline BOOL dm_pad_shapes_overlap(const DmPadShape *a, const DmPadShape *b) {
    const CGFloat e = 0.5;
    if (a->isRect && b->isRect) return CGRectIntersectsRect(CGRectInset(a->rect, e, e), CGRectInset(b->rect, e, e));
    if (a->isRect || b->isRect) {
        const DmPadShape *r = a->isRect ? a : b, *c = a->isRect ? b : a;
        CGFloat nx = fmax(CGRectGetMinX(r->rect), fmin(c->c.x, CGRectGetMaxX(r->rect))), ny = fmax(CGRectGetMinY(r->rect), fmin(c->c.y, CGRectGetMaxY(r->rect)));
        return hypot(c->c.x - nx, c->c.y - ny) < c->r - e;
    }
    return hypot(a->c.x - b->c.x, a->c.y - b->c.y) < a->r + b->r - e;
}
// A teardrop's tip belongs to its own circle; the tips are points, so they only count when they land inside another shape.
static inline BOOL dm_pad_same_control(const DmPadShape *a, const DmPadShape *b) {
    return (a->id >= DM_PAD_SHAPE_DROP && a->id < DM_PAD_SHAPE_TIP && b->id == a->id + 4) || (b->id >= DM_PAD_SHAPE_DROP && b->id < DM_PAD_SHAPE_TIP && a->id == b->id + 4);
}
// Every drawn shape inside its rectangle and no two overlapping.
static inline BOOL dm_pad_geo_fits(const DmPadGeo *g, BOOL full) {
    DmPadShape s[20]; int n = dm_pad_shapes(g, full, s);
    for (int i = 0; i < n; i++) {
        if (!dm_pad_shape_inside(&s[i])) return NO;
        for (int j = i + 1; j < n; j++) if (!dm_pad_same_control(&s[i], &s[j]) && dm_pad_shapes_overlap(&s[i], &s[j])) return NO;
    }
    return YES;
}

// The unit of the built-in size: the screen height (inside the controls region when it is split off), up to 1.3.
static inline CGFloat dm_pad_base_unit(CGRect bounds, UIEdgeInsets insets, DmLayout layout) {
    if (!layout.separated) return MIN(bounds.size.height / 430.0, 1.3);
    CGRect s = CGRectIntersection(layout.controls, UIEdgeInsetsInsetRect(bounds, dm_game_safe_insets(bounds, insets, layout.topSafeOnly)));
    return MIN(MIN(s.size.height / 430.0, s.size.width / 700.0), 1.3);
}

// The geometry the gamepad draws: the player's size, moved toward the built-in size in steps until it fits (a large size that does
// not fit gets smaller; a small one in a split region whose minimum edges crowd it gets larger). The built-in size is never changed.
static inline DmPadGeo dm_pad_geo(CGRect bounds, UIEdgeInsets insets, DmLayout layout, BOOL full, DmPadOptions o) {
    CGFloat u0 = dm_pad_base_unit(bounds, insets, layout), scale = o.scale > 0 ? o.scale : 1;
    DmPadGeo g = dm_pad_geo_at(bounds, insets, layout, full, u0 * scale);
    while (scale != 1 && !dm_pad_geo_fits(&g, full)) {
        scale = scale > 1 ? fmax(1, scale - 0.05) : fmin(1, scale + 0.05);
        g = dm_pad_geo_at(bounds, insets, layout, full, u0 * scale);
    }
    // The left stick's place: higher lifts it toward the shoulders, inward moves it (and the D-pad, which sits inward of it) toward the
    // middle. The move shrinks in steps until nothing overlaps; on a screen with no room the stick stays in the corner.
    if (o.stick != DM_PAD_STICK_CORNER) {
        BOOL up = o.stick == DM_PAD_STICK_HIGHER;
        CGPoint stick = g.stick, dpad = g.dpad;
        for (CGFloat d = (up ? DM_PAD_STICK_UP : DM_PAD_STICK_IN) * g.u; d > 0; d -= 2 * g.u) {
            if (up) g.stick = CGPointMake(stick.x, stick.y - d);
            else { g.stick = CGPointMake(stick.x + d, stick.y); g.dpad = CGPointMake(dpad.x + d, dpad.y); }
            if (dm_pad_geo_fits(&g, full)) return g;
        }
        g.stick = stick; g.dpad = dpad;
    }
    return g;
}

NS_ASSUME_NONNULL_END
