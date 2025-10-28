#include <math.h>
/* See LICENSE file for copyright and license details. */
/* appearance */
static const unsigned int borderpx = 1; /* border pixel of windows */
static const unsigned int snap = 32;    /* snap pixel */
static const unsigned int systraypinning =
    0; /* 0: sloppy systray follows selected monitor, >0: pin systray to monitor
          X */
static const unsigned int systrayonleft =
    0; /* 0: systray in the right corner, >0: systray on left of status text */
static const unsigned int systrayspacing = 2; /* systray spacing */
static const int systraypinningfailfirst =
    1; /* 1: if pinning fails, display systray on the first monitor, False:
          display systray on the last monitor*/
static const int showsystray = 1; /* 0 means no systray */
static const int showbar = 1;     /* 0 means no bar */
static const int topbar = 1;      /* 0 means bottom bar */
static const char *fonts[] = {"Iosevka Nerd Font:size=13.5"};
static const char dmenufont[] = "Iosevka Nerd Font:size=50";
static const char col_gray1[] = "#222222";
static const char col_gray2[] = "#444444";
static const char col_gray3[] = "#bbbbbb";
static const char col_gray4[] = "#eeeeee";
static const char col_cyan[] = "#005577";
static const char col_border[] = "#ff1493";
static const char *colors[][3] = {
    /*               fg         bg         border   */
    [SchemeNorm] = {col_gray3, col_gray1, col_gray2},
    [SchemeSel] = {col_gray4, col_cyan, col_border},
};

static const char file_manager[] = "dolphin";
typedef struct {
  const char *name;
  const void *cmd;
} Sp;
const char *spcmd1[] = {"dolphin", NULL};
const char *spcmd2[] = {"ghostty", "--initial-command=tmux new -A -s floaty", "--x11-instance-name=floatyghostty", NULL};
static Sp scratchpads[] = {
    /* name          cmd  */
    {"dolphin", spcmd1},
    {"ghostty", spcmd2},
};

static const XPoint stickyicon[] = {
    {0, 0}, {4, 0}, {4, 8},
    {2, 6}, {0, 8}, {0, 0}}; /* represents the icon as an array of vertices */
static const XPoint stickyiconbb = {
    4, 8}; /* defines the bottom right corner of the polygon's bounding box
              (speeds up scaling) */

/* tagging */
static const char *tags[] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};
static const int taglayouts[] = {2, 0, 0, 0, 0, 0, 0, 0, 0};

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080

// p is the percentage that the window takes on the screen
#define floatx(p) round((SCREEN_WIDTH - SCREEN_WIDTH * p) / 2)
#define floaty(p) round((SCREEN_HEIGHT - SCREEN_HEIGHT * p) / 2)
#define floatw(p) SCREEN_WIDTH - floatx(p) * 2
#define floath(p) SCREEN_HEIGHT - floaty(p) * 2
static const Rule rules[] = {
    /* xprop(1):
     *  WM_CLASS(STRING) = instance, class
     *  WM_NAME(STRING) = title
     */
    /* class            instance    title       tags mask     isfloating monitor
       float x,y,w,h         floatborderpx*/
    {"Gimp", NULL, NULL, 0, 1, -1, 50, 50, 500, 500, 5},
    {"Pavucontrol", NULL, NULL, 0, 1, -1, 50, 50, 500, 300, 3},
    {"copyq", NULL, NULL, 0, 1, -1, floatx(0.5), floaty(0.5), floatw(0.5),
     floath(0.5), 3},
    {"Thunar", NULL, NULL, 0, 1, -1, 192, 108, 1536, 864, 3},
    // { "dolphin",           NULL,       NULL,       0,            1, -1,
    // 192,108,1536,864,     3 },
    {NULL, file_manager, NULL, SPTAG(0), 1, -1, 192, 108, 1536, 864, 3},
    {NULL, "floatyghostty", NULL, SPTAG(1), 1, -1, 192, 108, 1536, 864, 3},
    {"firefox", NULL, NULL, 1 << 0, 0, -1, 50, 50, 500, 300, 3},
    {"jetbrains-idea-ce", NULL, NULL, 1 << 0, 0, -1, 50, 50, 500, 300, 3},
    {"jetbrains-toolbox", NULL, NULL, 2 << 0, 0, -1, 50, 50, 500, 300, 3},
    {"jetbrains-rider", NULL, NULL, 2 << 0, 0, -1, 50, 50, 500, 300, 3},
    {"Google-chrome", NULL, NULL, 1 << 0, 0, -1, 50, 50, 500, 300, 3},
    {"thunderbird", NULL, NULL, 1 << 9, 0, -1, 50, 50, 500, 300, 3},
};

/* layout(s) */
static const float mfact = 0.55; /* factor of master area size [0.05..0.95] */
static const int nmaster = 1;    /* number of clients in master area */
static const int resizehints =
    1; /* 1 means respect size hints in tiled resizals */
static const int lockfullscreen =
    1; /* 1 will force focus on the fullscreen window */

static const Layout layouts[] = {
    /* symbol     arrange function */
    {"tile", tile},  /* first entry is default */
    {"float", NULL}, /* no layout function means floating behavior */
    {"monocle", monocle},
};

/* key definitions */
#define MODKEY Mod4Mask
#define TAGKEYS(KEY, TAG)                                                      \
  {MODKEY, KEY, view, {.ui = 1 << TAG}},                                       \
      {MODKEY | ControlMask, KEY, toggleview, {.ui = 1 << TAG}},               \
      {MODKEY | ShiftMask, KEY, tag, {.ui = 1 << TAG}},                        \
      {MODKEY | ControlMask | ShiftMask, KEY, toggletag, {.ui = 1 << TAG}},

/* commands */
static char dmenumon[2] =
    "0"; /* component of dmenucmd, manipulated in spawn() */
static const char *dmenucmd[] = {
    "dmenu_run", "-m",      dmenumon, "-fn",    dmenufont, "-nb",     col_gray1,
    "-nf",       col_gray3, "-sb",    col_cyan, "-sf",     col_gray4, NULL};
static const char *roficmd[] = {"rofi",  "-matching", "fuzzy",
                                "-show", "run",       NULL};
static const char *termcmd[] = {"ghosttmux", NULL};

// Volume controls
static const char *volumeup[] = {"volume", "up", NULL};
static const char *volumedown[] = {"volume", "down", NULL};
static const char *resetvolume[] = {"volume", "reset", NULL};
static const char *mutetoggle[] = {"volume", "toggle", NULL};

// brightness
static const char *brightnessup[] = {"brightness", "up", NULL};
static const char *brightnessdown[] = {"brightness", "down", NULL};
static const char *copyqcmd[] = {"copyq", "toggle", NULL};
static const char *ocr[] = {"ocr-to-clipboard", NULL};
static const char *screenshot[] = {"flameshot", "gui", NULL};
static const char *emoji[] = {"rofimoji", " --max-recent", "3", NULL};

static Key keys[] = {
    /* modifier                     key        function        argument */
    {0, XF86XK_AudioMute, spawn, {.v = mutetoggle}},
    {0, XF86XK_AudioLowerVolume, spawn, {.v = volumedown}},
    {0, XF86XK_AudioRaiseVolume, spawn, {.v = volumeup}},
    {0, XF86XK_MonBrightnessDown, spawn, {.v = brightnessdown}},
    {0, XF86XK_MonBrightnessUp, spawn, {.v = brightnessup}},

    {MODKEY, XK_p, spawn, {.v = roficmd}},
    {MODKEY, XK_e, togglescratch, {.ui = 0}},
    {MODKEY, XK_backslash, togglescratch, {.ui = 1}},
    {MODKEY, XK_r, togglescratch, {.ui = 1}},
    {MODKEY | ShiftMask, XK_Return, spawn, {.v = termcmd}},
    {MODKEY, XK_v, spawn, {.v = copyqcmd}},
    {MODKEY | ShiftMask, XK_Print, spawn, {.v = ocr}},
    {MODKEY, XK_period, spawn, {.v = emoji}},
    {0, XK_Print, spawn, {.v = screenshot}},

    {MODKEY, XK_j, focusstack, {.i = +1}},
    {MODKEY, XK_k, focusstack, {.i = -1}},
    {MODKEY, XK_a, focusstack, {.i = +1}},
    {MODKEY, XK_d, focusstack, {.i = -1}},
    /*{MODKEY, XK_i, incnmaster, {.i = +1}},*/
    {MODKEY | ShiftMask, XK_i, incnmaster, {.i = -1}},
    {MODKEY, XK_h, setmfact, {.f = -0.05}},
    {MODKEY, XK_l, setmfact, {.f = +0.05}},
    {MODKEY, XK_Return, zoom, {0}},
    {MODKEY, XK_Tab, view, {0}},
    {MODKEY, XK_Escape, focusstack, {.i = +1}},
    {MODKEY | ShiftMask, XK_q, killclient, {0}},
    {MODKEY, XK_t, setlayout, {.v = &layouts[0]}},
    {MODKEY, XK_m, setlayout, {.v = &layouts[2]}},
    {MODKEY, XK_s, spawn, { .v = screenshot }},
    {MODKEY, XK_f, togglebar, {0}},
    {MODKEY | ShiftMask, XK_space, togglefloating, {0}},
    {MODKEY, XK_0, view, {.ui = ~0}},
    {MODKEY | ShiftMask, XK_0, tag, {.ui = ~0}},
    {MODKEY, XK_comma, focusmon, {.i = -1}},
    {MODKEY, XK_period, focusmon, {.i = +1}},
    {MODKEY | ShiftMask, XK_comma, tagmon, {.i = -1}},
    {MODKEY | ShiftMask, XK_period, tagmon, {.i = +1}},

    // tag keys
    TAGKEYS(XK_grave, 0) TAGKEYS(XK_1, 1) TAGKEYS(XK_2, 2) TAGKEYS(XK_3, 3)
        TAGKEYS(XK_4, 4) TAGKEYS(XK_5, 5) TAGKEYS(XK_6, 6) TAGKEYS(XK_7, 7)
            TAGKEYS(XK_8, 8) TAGKEYS(XK_9, 9)

};

/* button definitions */
/* click can be ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle,
 * ClkClientWin, or ClkRootWin */
static Button buttons[] = {
    /* click                event mask      button          function argument */
    {ClkTagBar, MODKEY, Button1, tag, {0}},
    {ClkTagBar, MODKEY, Button3, toggletag, {0}},
    {ClkWinTitle, 0, Button2, zoom, {0}},
    {ClkStatusText, 0, Button2, spawn, {.v = termcmd}},
    {ClkClientWin, MODKEY, Button1, movemouse, {0}},
    {ClkClientWin, MODKEY, Button2, togglefloating, {0}},
    {ClkClientWin, MODKEY, Button3, resizemouse, {0}},
    {ClkTagBar, 0, Button1, view, {0}},
    {ClkTagBar, 0, Button3, toggleview, {0}},
    {ClkTagBar, MODKEY, Button1, tag, {0}},
    {ClkTagBar, MODKEY, Button3, toggletag, {0}},
};
