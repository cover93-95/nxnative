// The virtual gamepad's placement (DmPadGeometry.h) on the screens it runs on, checked on the Mac: built and run by test_pad_geometry.py.
// Every line that starts with FAIL is a broken rule; the exit status is the number of them (capped).
#import <UIKit/UIKit.h>
#import "DmPadGeometry.h"
#include <stdio.h>

typedef struct { const char *name; CGRect bounds; UIEdgeInsets insets; BOOL partiallyOpen; CGRect division; } Screen;

static int g_fail;
#define FAIL(...) do { printf("FAIL: "); printf(__VA_ARGS__); printf("\n"); g_fail++; } while (0)
#define NEAR(a, b) (fabs((a) - (b)) < 0.01)

static const char *k_sizes[] = {"small", "medium", "large"};

static DmPadGeo geo(const Screen *s, BOOL full, DmPadOptions o) {
    return dm_pad_geo(s->bounds, s->insets, dm_layout(s->bounds, s->insets, s->partiallyOpen, s->division, DM_PICTURE_16_9), full, o);
}

static void check_fits(const Screen *s, BOOL full, const char *size, DmPadOptions o) {
    DmPadGeo g = geo(s, full, o);
    DmPadShape sh[20]; int n = dm_pad_shapes(&g, full, sh);
    for (int i = 0; i < n; i++) {
        if (!dm_pad_shape_inside(&sh[i])) FAIL("%s %s %s: shape %d leaves its rectangle", s->name, full ? "3D" : "2D", size, sh[i].id);
        for (int j = i + 1; j < n; j++)
            if (!dm_pad_same_control(&sh[i], &sh[j]) && dm_pad_shapes_overlap(&sh[i], &sh[j]))
                FAIL("%s %s %s: shapes %d and %d overlap", s->name, full ? "3D" : "2D", size, sh[i].id, sh[j].id);
    }
    // Nothing outside the screen at all, side insets included only up to the corner rule.
    CGRect b = s->bounds;
    for (int i = 0; i < n; i++) {
        CGRect r = sh[i].isRect ? sh[i].rect : CGRectMake(sh[i].c.x - sh[i].r, sh[i].c.y - sh[i].r, 2 * sh[i].r, 2 * sh[i].r);
        if (!CGRectContainsRect(CGRectInset(b, -0.5, -0.5), r)) FAIL("%s %s %s: shape %d is off the screen", s->name, full ? "3D" : "2D", size, sh[i].id);
    }
}

int main(void) {
    const Screen screens[] = {
        // iPhone landscape (the phone's only orientation): the Dynamic Island or notch on one side, the home indicator at the bottom.
        {"iPhone SE landscape", {{0, 0}, {667, 375}}, {0, 0, 0, 0}, NO, CGRectNull},
        {"iPhone mini landscape", {{0, 0}, {812, 375}}, {0, 50, 21, 50}, NO, CGRectNull},
        {"iPhone landscape", {{0, 0}, {852, 393}}, {0, 59, 21, 59}, NO, CGRectNull},
        {"iPhone Pro Max landscape", {{0, 0}, {932, 430}}, {0, 59, 21, 59}, NO, CGRectNull},
        {"iPhone 17 Pro Max landscape", {{0, 0}, {956, 440}}, {0, 62, 20, 62}, NO, CGRectNull},
        // iPad: every orientation; portrait splits the picture from the controls.
        {"iPad mini landscape", {{0, 0}, {1133, 744}}, {24, 0, 20, 0}, NO, CGRectNull},
        {"iPad Pro 13 landscape", {{0, 0}, {1376, 1032}}, {24, 0, 20, 0}, NO, CGRectNull},
        {"iPad mini portrait", {{0, 0}, {744, 1133}}, {24, 0, 20, 0}, NO, CGRectNull},
        {"iPad Pro 13 portrait", {{0, 0}, {1032, 1376}}, {24, 0, 20, 0}, NO, CGRectNull},
        // Split portrait windows the Duo harness uses, and iPhone Duo's inner display (patch 0061): portrait with the fold, tabletop half open.
        {"portrait 390x844", {{0, 0}, {390, 844}}, {59, 0, 34, 0}, NO, CGRectNull},
        {"portrait 320x568", {{0, 0}, {320, 568}}, {50, 0, 24, 0}, NO, CGRectNull},
        {"Duo inner portrait", {{0, 0}, {669, 951}}, {62, 0, 34, 0}, NO, {{0, 455.5}, {669, 40}}},
        {"Duo inner landscape", {{0, 0}, {951, 669}}, {0, 62, 20, 62}, NO, CGRectNull},
        {"Duo tabletop", {{0, 0}, {951, 669}}, {0, 62, 20, 62}, YES, {{0, 314.5}, {951, 40}}},
    };
    int nscreens = (int)(sizeof screens / sizeof *screens);

    // Option parsing.
    if (dm_pad_size_scale(NULL) != 1 || dm_pad_size_scale("") != 1 || dm_pad_size_scale("medium") != 1 || dm_pad_size_scale("huge") != 1) FAIL("DM_TOUCH_SIZE: medium is the default");
    if (!(dm_pad_size_scale("small") < 1) || !(dm_pad_size_scale("large") > 1)) FAIL("DM_TOUCH_SIZE: small is smaller and large larger");

    // The built-in size is the layout DmGamepad.m has always drawn: iPhone Pro Max landscape, centres in points.
    {
        DmPadGeo g = geo(&screens[3], YES, (DmPadOptions){1});
        if (!NEAR(g.u, 1)) FAIL("Pro Max: u %.3f, want 1", g.u);
        if (!NEAR(g.face.x, 932 - 29.5 - 81) || !NEAR(g.face.y, 430 - 21 - 82)) FAIL("Pro Max: diamond at %.1f %.1f", g.face.x, g.face.y);
        if (!NEAR(g.stick.x, 29.5 + 81) || !NEAR(g.stick.y, 430 - 21 - 82)) FAIL("Pro Max: stick at %.1f %.1f", g.stick.x, g.stick.y);
        if (!NEAR(g.dpad.x, 29.5 + 231) || !NEAR(g.dpad.y, 430 - 21 - 64)) FAIL("Pro Max: D-pad at %.1f %.1f", g.dpad.x, g.dpad.y);
        if (!NEAR(g.drop[DM_PAD_ZL].x, 59 + 44) || !NEAR(g.drop[DM_PAD_ZL].y, 40) || !NEAR(g.drop[DM_PAD_R].x, 932 - 59 - 44) || !NEAR(g.drop[DM_PAD_R].y, 108)) FAIL("Pro Max: shoulders moved");
        if (!NEAR(g.click[1].x, 932 - 59 - 102) || !NEAR(g.pill[DM_PAD_PLUS].origin.x, 466 + 52 - 25) || !NEAR(g.swap.x, 466) || !NEAR(g.swap.y, 22)) FAIL("Pro Max: top row moved");
        if (!NEAR(g.stickR, 77) || !NEAR(g.faceR, 27) || !NEAR(g.dpadR, 48)) FAIL("Pro Max: sizes changed");
        // Portrait split (the Duo harness's numbers): the thumbs' centres at 28 % and 72 % of the controls, 58 % down.
        DmLayout l = dm_layout(screens[9].bounds, screens[9].insets, NO, CGRectNull, DM_PICTURE_16_9);
        DmPadGeo p = dm_pad_geo(screens[9].bounds, screens[9].insets, l, NO, (DmPadOptions){1});
        CGRect s = p.safe;
        if (!p.portrait || !NEAR(p.face.x, CGRectGetMinX(s) + 0.72 * s.size.width) || !NEAR(p.stick.x, CGRectGetMinX(s) + 0.28 * s.size.width)) FAIL("portrait: thumbs moved");
    }

    // Every size and stick place on every screen, 2D and 3D: inside, and no overlap.
    static const char *places[] = {"corner", "higher", "inward"};
    if (dm_pad_stick_place(NULL) != DM_PAD_STICK_CORNER || dm_pad_stick_place("left") != DM_PAD_STICK_CORNER || dm_pad_stick_place("higher") != DM_PAD_STICK_HIGHER
        || dm_pad_stick_place("inward") != DM_PAD_STICK_INWARD) FAIL("DM_TOUCH_STICK: corner is the default");
    for (int i = 0; i < nscreens; i++)
        for (int full = 0; full < 2; full++)
            for (int z = 0; z < 3; z++)
                for (int p = 0; p < 3; p++) {
                    char what[64]; snprintf(what, sizeof what, "%s %s", k_sizes[z], places[p]);
                    check_fits(&screens[i], full, what, (DmPadOptions){dm_pad_size_scale(k_sizes[z]), dm_pad_stick_place(places[p])});
                }

    // The stick moves where asked on the phones, and nothing on the right moves with it.
    for (int i = 0; i <= 4; i++)
        for (int full = 0; full < 2; full++)
            for (int z = 0; z < 3; z++) {
                DmPadOptions o = {dm_pad_size_scale(k_sizes[z]), DM_PAD_STICK_CORNER};
                DmPadGeo c = geo(&screens[i], full, o);
                o.stick = DM_PAD_STICK_HIGHER; DmPadGeo h = geo(&screens[i], full, o);
                o.stick = DM_PAD_STICK_INWARD; DmPadGeo n = geo(&screens[i], full, o);
                if (!(c.stick.y - h.stick.y >= 30 * c.u) || h.stick.x != c.stick.x) FAIL("%s %s %s: higher lifts the stick only %.1f", screens[i].name, full ? "3D" : "2D", k_sizes[z], c.stick.y - h.stick.y);
                if (!(n.stick.x - c.stick.x >= 30 * c.u) || n.stick.y != c.stick.y) FAIL("%s %s %s: inward moves the stick only %.1f", screens[i].name, full ? "3D" : "2D", k_sizes[z], n.stick.x - c.stick.x);
                if (full && !NEAR(n.dpad.x - c.dpad.x, n.stick.x - c.stick.x)) FAIL("%s 3D %s: the D-pad does not move with the stick", screens[i].name, k_sizes[z]);
                if (!CGPointEqualToPoint(h.face, c.face) || !CGPointEqualToPoint(n.face, c.face) || !CGPointEqualToPoint(n.drop[DM_PAD_R], c.drop[DM_PAD_R])) FAIL("%s: the right side moved", screens[i].name);
            }

    // Picture Fill (DM_PICTURE_FIT) only makes the game rectangle larger: the controls keep the full view and their places.
    for (int i = 0; i < nscreens; i++) {
        DmLayout fit = dm_layout(screens[i].bounds, screens[i].insets, screens[i].partiallyOpen, screens[i].division, DM_PICTURE_16_9);
        DmLayout fill = dm_layout_picture(fit, screens[i].bounds, YES, DM_PICTURE_16_9);
        DmPadGeo a = dm_pad_geo(screens[i].bounds, screens[i].insets, fit, YES, (DmPadOptions){1.2}), b = dm_pad_geo(screens[i].bounds, screens[i].insets, fill, YES, (DmPadOptions){1.2});
        if (!CGPointEqualToPoint(a.face, b.face) || !CGPointEqualToPoint(a.stick, b.stick) || a.u != b.u) FAIL("%s: the picture fill moved the controls", screens[i].name);
    }

    // The sizes keep their order everywhere, and on a phone in landscape large really is larger and small smaller.
    for (int i = 0; i < nscreens; i++)
        for (int full = 0; full < 2; full++) {
            CGFloat us = geo(&screens[i], full, (DmPadOptions){0.8}).u, um = geo(&screens[i], full, (DmPadOptions){1}).u, ul = geo(&screens[i], full, (DmPadOptions){1.2}).u;
            if (!(us < um) || ul < um) FAIL("%s %s: sizes out of order (%.3f %.3f %.3f)", screens[i].name, full ? "3D" : "2D", us, um, ul);
            if (i <= 4 && !(ul > um * 1.15)) FAIL("%s %s: large is not larger (%.3f vs %.3f)", screens[i].name, full ? "3D" : "2D", ul, um);
        }

    if (!g_fail) printf("ok: %d screens\n", nscreens);
    return g_fail > 100 ? 100 : g_fail;
}
