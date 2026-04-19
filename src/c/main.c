#include <pebble.h>

/* ================================================================== */
/*  LITERARY WATCHFACE — Pebble Time Steel  (144 x 168 px)            */
/*                                                                     */
/*  One flowing story sentence, key values in bright white,           */
/*  prose in dim gray. Rendered word-by-word via graphics_draw_text.  */
/*                                                                     */
/*  Bottom strip (y=118..168):                                        */
/*    Left  x=2  — 86x48 widget layer (sine/bars/battery/ECG)         */
/*    Right x=94 — 48x48 dot clock layer                              */
/*                                                                     */
/*  Settings via Pebble phone app gear icon (PebbleKit JS + HTML).   */
/*  AppMessage keys: widget=1, font=2. Both persisted on watch.       */
/* ================================================================== */

/* ------------------------------------------------------------------ */
/*  CONSTANTS                                                          */
/* ------------------------------------------------------------------ */

#define PERSIST_KEY_MODE    1
#define PERSIST_KEY_FONT    2
#define PERSIST_KEY_CAPS    3
#define PERSIST_KEY_STYLE   4
#define PERSIST_KEY_COLOUR  5

#define MSG_KEY_WIDGET  1
#define MSG_KEY_FONT    2
#define MSG_KEY_STYLE   3
#define MSG_KEY_COLOUR  4

#define STYLE_ALLCAPS      0
#define STYLE_MIXED        1
#define STYLE_LOWER        2
#define STYLE_BRIGHT_UPPER 3
#define STYLE_COUNT        4

#define COLOUR_WHITE  0
#define COLOUR_WARM   1
#define COLOUR_AMBER  2
#define COLOUR_COOL   3
#define COLOUR_GREEN  4
#define COLOUR_COUNT  5

#define MODE_SINE     0
#define MODE_BARS     1
#define MODE_BATTERY  2
#define MODE_ECG      3
#define MODE_COUNT    4

#define FONT_GOTHIC09  0
#define FONT_GOTHIC14  1
#define FONT_GOTHIC18B 2
#define FONT_GOTHIC28B  3
#define FONT_COUNT      4


/* Screen layout */
#define SCREEN_W   144
#define SCREEN_H   168
#define STRIP_Y    118
#define TEXT_X       4
#define TEXT_Y       6
#define TEXT_W     136
#define TEXT_H     108   /* STRIP_Y - TEXT_Y - 4px gap */

/* Widget (bottom-left) */
#define WG_X         2
#define WG_W        86
#define WG_H        48

/* Dot clock (bottom-right) */
#define CLOCK_X     94
#define CLOCK_SIZE  48
#define CLOCK_CX    24
#define CLOCK_CY    24
#define CLOCK_R     20

/* ECG */
#define HB_COLS     14
#define HB_ROWS      6
#define HB_DW        6
#define HB_DH        8
#define HB_DOT       2

/* Story rendering */
#define MAX_SEGS    20   /* max segments in one story sentence */

/* ------------------------------------------------------------------ */
/*  FORWARD DECLARATIONS                                               */
/* ------------------------------------------------------------------ */

static void clock_layer_update(Layer *layer, GContext *ctx);
static void widget_layer_update(Layer *layer, GContext *ctx);
static void story_layer_update(Layer *layer, GContext *ctx);
static void build_story(int h, int m, int steps, int battery,
                        int bpm, struct tm *t);

/* ------------------------------------------------------------------ */
/*  GLOBALS                                                            */
/* ------------------------------------------------------------------ */

static Window *s_main_window;
static Layer  *s_story_layer;
static Layer  *s_clock_layer;
static Layer  *s_widget_layer;

static int s_phase  = 0;
static int s_mode   = MODE_ECG;
static int s_font   = FONT_GOTHIC14;
static int  s_style  = STYLE_ALLCAPS;
static int  s_colour = COLOUR_WARM;

/* Story segments — each has text + bright flag */
typedef struct { char text[48]; bool bright; } Seg;
static Seg  s_segs[MAX_SEGS];
static int  s_seg_count = 0;

static const int s_hourly[12] = {
  0, 200, 600, 1400, 2200, 3100,
  3900, 4700, 3200, 1800, 900, 400
};

/* ------------------------------------------------------------------ */
/*  FONT                                                               */
/* ------------------------------------------------------------------ */

static GFont get_story_font(void) {
  switch (s_font) {
    case FONT_GOTHIC09:  return fonts_get_system_font(FONT_KEY_GOTHIC_09);
    case FONT_GOTHIC18B: return fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
    case FONT_GOTHIC28B: return fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
    default:             return fonts_get_system_font(FONT_KEY_GOTHIC_14);
  }
}

/* ------------------------------------------------------------------ */
/*  METRICS                                                            */
/* ------------------------------------------------------------------ */

static int fake_steps(int m)   { return (m * 37) % 12000; }
static int fake_battery(int m) { return 100 - (m / 6); }
static int fake_bpm(int m)     { return 62 + (m % 30); }

/* ------------------------------------------------------------------ */
/*  WORD CLOCK                                                         */
/* ------------------------------------------------------------------ */

static const char *minute_phrase(int m) {
  if (m <  5) return "o'clock";
  if (m < 10) return "five past";
  if (m < 15) return "ten past";
  if (m < 20) return "quarter past";
  if (m < 25) return "twenty past";
  if (m < 30) return "twenty-five past";
  if (m < 35) return "half past";
  if (m < 40) return "twenty-five to";
  if (m < 45) return "twenty to";
  if (m < 50) return "quarter to";
  if (m < 55) return "ten to";
  return "five to";
}

static const char *hour_names[] = {
  "twelve","one","two","three","four","five",
  "six","seven","eight","nine","ten","eleven"
};

/* ------------------------------------------------------------------ */
/*  WORD POOLS                                                         */
/* ------------------------------------------------------------------ */

static const char *lit_subject_verb[] = {
  "the silence accumulates",
  "each minute dissolves",
  "the interval folds",
  "this hour persists",
  "the count recedes",
  "the moment settles",
  "the measure drifts",
  "the record holds",
  "the gap widens",
  "time passes"
};

static const char *lit_adverb[] = {
  "like sediment",
  "in sequence",
  "past notice",
  "between breaths",
  "without trace",
  "like residue",
  "as if expected",
  "as before",
  "quietly",
  "in small doses"
};









/* ------------------------------------------------------------------ */
/*  SEEDED PICK                                                        */
/* ------------------------------------------------------------------ */

static const char *spick(const char **arr, int len, int seed) {
  unsigned int s = (unsigned int)seed;
  s ^= s << 13; s ^= s >> 7; s ^= s << 5;
  return arr[s % (unsigned int)len];
}

/* ------------------------------------------------------------------ */
/*  STORY BUILDER                                                      */
/*                                                                     */
/*  Fills s_segs[] with alternating dim/bright segments forming       */
/*  one continuous sentence. Example:                                 */
/*                                                                     */
/*  "Saturday, 18 April, at [twenty-five to seven,] the silence      */
/*   accumulates like sediment. you walked [7842] steps since        */
/*   waking, the battery held at [81%,] your heart at [72 bpm.]"    */
/* ------------------------------------------------------------------ */

static void to_upper(char *s) {
  for (; *s; s++) if (*s >= 'a' && *s <= 'z') *s = (char)(*s - 32);
}
static void to_lower(char *s) {
  for (; *s; s++) if (*s >= 'A' && *s <= 'Z') *s = (char)(*s + 32);
}

static void seg_add(const char *text, bool bright) {
  if (s_seg_count >= MAX_SEGS) return;
  snprintf(s_segs[s_seg_count].text,
           sizeof(s_segs[s_seg_count].text), "%s", text);
  switch (s_style) {
    case STYLE_ALLCAPS:
      to_upper(s_segs[s_seg_count].text);
      break;
    case STYLE_LOWER:
      to_lower(s_segs[s_seg_count].text);
      break;
    case STYLE_BRIGHT_UPPER:
      if (s_segs[s_seg_count].bright) to_upper(s_segs[s_seg_count].text);
      else to_lower(s_segs[s_seg_count].text);
      break;
    default: /* STYLE_MIXED — leave as-is */
      break;
  }
  s_segs[s_seg_count].bright = bright;
  s_seg_count++;
}

static void build_story(int h, int m, int steps, int battery,
                        int bpm, struct tm *t) {
  char tmp[32];
  s_seg_count = 0;

  int seed = h * 60 + m + t->tm_yday * 7;

  /* dim:    "SATURDAY, 18 APRIL, AT " */
  char date[32];
  strftime(date, sizeof(date), "%A, %d %B, at ", t);
  seg_add(date, false);

  /* bright: "TWENTY-FIVE TO SEVEN," */
  snprintf(tmp, sizeof(tmp), "%s %s,",
           minute_phrase(m), hour_names[h % 12]);
  seg_add(tmp, true);

  /* dim:    " THE SILENCE ACCUMULATES LIKE SEDIMENT. YOU WALKED " */
  snprintf(tmp, sizeof(tmp), " %s %s. you walked ",
           spick(lit_subject_verb, 10, seed),
           spick(lit_adverb,       10, seed + 1));
  seg_add(tmp, false);

  /* bright: "7842 STEPS SINCE WAKING," */
  snprintf(tmp, sizeof(tmp), "%d steps since waking,", steps);
  seg_add(tmp, true);

  /* dim:    " YOUR " */
  seg_add(" your ", false);

  /* bright: "BATTERY 81%," */
  snprintf(tmp, sizeof(tmp), "battery %d%%,", battery);
  seg_add(tmp, true);

  /* dim:    " YOUR " */
  seg_add(" your ", false);

  /* bright: "HEART 72 BPM." */
  snprintf(tmp, sizeof(tmp), "heart %d bpm.", bpm);
  seg_add(tmp, true);
}

/* ------------------------------------------------------------------ */
/*  STORY LAYER — word-by-word word-wrap with bright/dim colours      */
/* ------------------------------------------------------------------ */

static GColor get_prose_colour(void) {
  switch (s_colour) {
    case COLOUR_WHITE: return GColorWhite;
    case COLOUR_AMBER: return GColorFromRGB(212, 184, 150);
    case COLOUR_COOL:  return GColorFromRGB(200, 208, 216);
    case COLOUR_GREEN: return GColorFromRGB(200, 216, 192);
    default:           return GColorFromRGB(232, 224, 208);
  }
}

static void story_layer_update(Layer *layer, GContext *ctx) {
  GFont font   = get_story_font();
  GColor dim   = get_prose_colour();
  GColor bright= GColorWhite;

  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);

  /* measure line height from a sample character */
  GSize lh_sz = graphics_text_layout_get_content_size(
    "A", font, GRect(0,0,TEXT_W,100),
    GTextOverflowModeWordWrap, GTextAlignmentLeft);
  int line_h = lh_sz.h + 1;
  if (line_h < 10) line_h = 10;

  int x = 0, y = 0;

  for (int si = 0; si < s_seg_count; si++) {
    /* walk through the segment word by word */
    char buf[48];
    /* copy into local buf so we can walk the pointer safely */
    int blen = 0;
    while (blen < 47 && s_segs[si].text[blen]) { buf[blen] = s_segs[si].text[blen]; blen++; }
    buf[blen] = '\0';

    char *p = buf;
    while (*p) {
      /* find end of next token (word or whitespace run) */
      char token[32];
      int ti = 0;

      if (*p == ' ') {
        /* whitespace token */
        while (*p == ' ' && ti < 31) token[ti++] = *p++;
      } else {
        /* word token */
        while (*p && *p != ' ' && ti < 31) token[ti++] = *p++;
      }
      token[ti] = '\0';
      if (ti == 0) break;

      /* measure token */
      GSize tsz = graphics_text_layout_get_content_size(
        token, font, GRect(0,0,TEXT_W,100),
        GTextOverflowModeWordWrap, GTextAlignmentLeft);
      int tw = tsz.w;

      /* skip leading spaces at line start */
      if (x == 0 && token[0] == ' ') continue;

      /* wrap if needed */
      if (x + tw > TEXT_W && x > 0) {
        x = 0;
        y += line_h;
      }

      /* stop if below text zone */
      if (y + line_h > TEXT_H) break;

      /* draw */
      graphics_context_set_text_color(ctx,
        s_segs[si].bright ? bright : dim);
      graphics_draw_text(ctx, token, font,
        GRect(x, y, TEXT_W - x, line_h * 2),
        GTextOverflowModeTrailingEllipsis,
        GTextAlignmentLeft, NULL);

      x += tw;
    }
  }
}

/* ================================================================== */
/*  WIDGET DRAWING  (86 x 48 px)                                      */
/* ================================================================== */

static void draw_sine(GContext *ctx) {
  graphics_context_set_stroke_color(ctx, GColorDarkGray);
  graphics_draw_line(ctx, GPoint(0, WG_H/2), GPoint(WG_W, WG_H/2));
  graphics_context_set_stroke_color(ctx, GColorWhite);
  GPoint prev = GPoint(0, WG_H/2);
  for (int x = 1; x < WG_W; x++) {
    int32_t angle = (int32_t)x * TRIG_MAX_ANGLE * 2 / WG_W;
    int sy = WG_H/2 - (int)(sin_lookup(angle) * 14 / TRIG_MAX_RATIO);
    GPoint cur = GPoint(x, sy);
    graphics_draw_line(ctx, prev, cur);
    prev = cur;
  }
  graphics_context_set_text_color(ctx, GColorDarkGray);
  graphics_draw_text(ctx, "rhythm",
    fonts_get_system_font(FONT_KEY_GOTHIC_09),
    GRect(2, WG_H-11, WG_W-4, 11),
    GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
}

static void draw_bars(GContext *ctx, int current_hour) {
  int chart_h = WG_H - 12;
  int bw      = (WG_W - 4) / 12;
  for (int i = 0; i < 12; i++) {
    int val = s_hourly[i] > 5000 ? 5000 : s_hourly[i];
    int bh  = (val * chart_h) / 5000;
    if (bh < 1) bh = 1;
    graphics_context_set_fill_color(ctx,
      (i == current_hour % 12) ? GColorWhite : GColorDarkGray);
    graphics_fill_rect(ctx,
      GRect(2 + i*bw, chart_h - bh, bw-1, bh), 0, GCornerNone);
  }
  graphics_context_set_text_color(ctx, GColorDarkGray);
  graphics_draw_text(ctx, "steps/hr",
    fonts_get_system_font(FONT_KEY_GOTHIC_09),
    GRect(2, WG_H-11, WG_W-4, 11),
    GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
}

static void draw_battery(GContext *ctx, int current_hour) {
  int chart_h  = WG_H - 12;
  int marker_x = current_hour * WG_W / 24;
  GPoint prev  = GPoint(0, 0);
  for (int x = 0; x < WG_W; x++) {
    int pct = 100 - (x * 80 / WG_W);
    int32_t wa = (int32_t)x * TRIG_MAX_ANGLE * 6 / WG_W;
    pct += (int)(sin_lookup(wa) * 3 / TRIG_MAX_RATIO);
    if (pct < 0)   pct = 0;
    if (pct > 100) pct = 100;
    int y = chart_h - (pct * chart_h / 100);
    GPoint cur = GPoint(x, y);
    if (x > 0) {
      graphics_context_set_stroke_color(ctx, GColorDarkGray);
      graphics_draw_line(ctx, GPoint(x,y), GPoint(x,chart_h));
      graphics_context_set_stroke_color(ctx, GColorWhite);
      graphics_draw_line(ctx, prev, cur);
    }
    prev = cur;
  }
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_draw_line(ctx, GPoint(marker_x,0), GPoint(marker_x,chart_h));
  graphics_context_set_text_color(ctx, GColorDarkGray);
  graphics_draw_text(ctx, "battery",
    fonts_get_system_font(FONT_KEY_GOTHIC_09),
    GRect(2, WG_H-11, WG_W-4, 11),
    GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
}

static int gauss_i(int x, int c, int w, int amp) {
  int d=x-c, d2=d*d, w2=w*w;
  if (d2 > 4*w2) return 0;
  return amp - (amp*d2)/(w2+1);
}

static int ecg_wave(int col, int phase) {
  int c = (col+phase) % HB_COLS, y = 128;
  y += gauss_i(c,  2, 1,  20);
  y -= gauss_i(c,  5, 1,  15);
  y += gauss_i(c,  6, 1, 110);
  y -= gauss_i(c,  7, 1,  30);
  y += gauss_i(c, 10, 2,  40);
  if (y < 0)   y = 0;
  if (y > 255) y = 255;
  return y;
}

static void draw_ecg(GContext *ctx) {
  for (int c = 0; c < HB_COLS; c++) {
    int wr = (HB_ROWS-1) - (ecg_wave(c, s_phase) * (HB_ROWS-1)) / 255;
    for (int r = 0; r < HB_ROWS; r++) {
      int px=c*HB_DW+1, py=r*HB_DH+2, dist=r-wr;
      if (dist < 0) dist = -dist;
      if (dist == 0) {
        graphics_context_set_fill_color(ctx, GColorRed);
        graphics_fill_rect(ctx,
          GRect(px,py,HB_DOT+1,HB_DOT+1), 0, GCornerNone);
      } else if (dist == 1) {
        graphics_context_set_fill_color(ctx, GColorDarkCandyAppleRed);
        graphics_fill_rect(ctx,
          GRect(px,py,HB_DOT,HB_DOT), 0, GCornerNone);
      } else {
        graphics_context_set_fill_color(ctx, GColorLightGray);
        graphics_fill_rect(ctx,
          GRect(px,py,HB_DOT,HB_DOT), 0, GCornerNone);
      }
    }
  }
}

static void widget_layer_update(Layer *layer, GContext *ctx) {
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, GRect(0,0,WG_W,WG_H), 0, GCornerNone);
  switch (s_mode) {
    case MODE_SINE:    draw_sine(ctx);                break;
    case MODE_BARS:    draw_bars(ctx, t->tm_hour);    break;
    case MODE_BATTERY: draw_battery(ctx, t->tm_hour); break;
    case MODE_ECG:     draw_ecg(ctx);                 break;
  }
}

/* ================================================================== */
/*  DOT CLOCK  (48 x 48 px)                                           */
/* ================================================================== */

static void plot_dot(GContext *ctx, int x, int y, int sz) {
  graphics_fill_rect(ctx,
    GRect(x-sz/2, y-sz/2, sz, sz), 0, GCornerNone);
}

static void clock_layer_update(Layer *layer, GContext *ctx) {
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  int hour = t->tm_hour % 12, min = t->tm_min;

  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, GRect(0,0,CLOCK_SIZE,CLOCK_SIZE), 0, GCornerNone);

  for (int i = 0; i < 60; i++) {
    int32_t angle = TRIG_MAX_ANGLE * i / 60;
    int dx = (int)(sin_lookup(angle) * CLOCK_R / TRIG_MAX_RATIO);
    int dy = -(int)(cos_lookup(angle) * CLOCK_R / TRIG_MAX_RATIO);
    if (i % 5 == 0) {
      graphics_context_set_fill_color(ctx, GColorWhite);
      plot_dot(ctx, CLOCK_CX+dx, CLOCK_CY+dy, 3);
    } else {
      graphics_context_set_fill_color(ctx, GColorDarkGray);
      plot_dot(ctx, CLOCK_CX+dx, CLOCK_CY+dy, 2);
    }
  }
  { /* minute hand */
    int32_t a = TRIG_MAX_ANGLE * min / 60;
    int len   = CLOCK_R - 3;
    graphics_context_set_fill_color(ctx, GColorWhite);
    for (int i = 1; i <= 6; i++) {
      int r = len * i / 6;
      plot_dot(ctx,
        CLOCK_CX + (int)(sin_lookup(a)*r/TRIG_MAX_RATIO),
        CLOCK_CY - (int)(cos_lookup(a)*r/TRIG_MAX_RATIO), 2);
    }
  }
  { /* hour hand */
    int32_t a = TRIG_MAX_ANGLE * (hour*60+min) / 720;
    int len   = (CLOCK_R * 6) / 10;
    graphics_context_set_fill_color(ctx, GColorWhite);
    for (int i = 1; i <= 4; i++) {
      int r = len * i / 4;
      plot_dot(ctx,
        CLOCK_CX + (int)(sin_lookup(a)*r/TRIG_MAX_RATIO),
        CLOCK_CY - (int)(cos_lookup(a)*r/TRIG_MAX_RATIO), 2);
    }
  }
  graphics_context_set_fill_color(ctx, GColorWhite);
  plot_dot(ctx, CLOCK_CX, CLOCK_CY, 3);
}

/* ================================================================== */
/*  APPMESSAGE — widget setting from phone config page               */
/* ================================================================== */

static void apply_settings(void) {
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  int h = t->tm_hour % 12, m = t->tm_min;
  build_story(h, m, fake_steps(m), fake_battery(m), fake_bpm(m), t);
  layer_mark_dirty(s_story_layer);
  layer_mark_dirty(s_widget_layer);
  layer_mark_dirty(s_clock_layer);
}

static void inbox_received(DictionaryIterator *iter, void *context) {
  Tuple *wt = dict_find(iter, MSG_KEY_WIDGET);
  if (wt) {
    int val = -1;
    if (wt->type == TUPLE_INT)     val = (int)wt->value->int32;
    if (wt->type == TUPLE_UINT)    val = (int)wt->value->uint32;
    if (wt->type == TUPLE_CSTRING) val = atoi(wt->value->cstring);
    if (val >= 0 && val < MODE_COUNT) {
      s_mode = val;
      persist_write_int(PERSIST_KEY_MODE, s_mode);
    }
  }
  Tuple *ft = dict_find(iter, MSG_KEY_FONT);
  if (ft) {
    int val = -1;
    if (ft->type == TUPLE_INT)     val = (int)ft->value->int32;
    if (ft->type == TUPLE_UINT)    val = (int)ft->value->uint32;
    if (ft->type == TUPLE_CSTRING) val = atoi(ft->value->cstring);
    if (val >= 0 && val < FONT_COUNT) {
      s_font = val;
      persist_write_int(PERSIST_KEY_FONT, s_font);
    }
  }
  Tuple *st = dict_find(iter, MSG_KEY_STYLE);
  if (st) {
    int val = -1;
    if (st->type == TUPLE_INT)     val = (int)st->value->int32;
    if (st->type == TUPLE_UINT)    val = (int)st->value->uint32;
    if (st->type == TUPLE_CSTRING) val = atoi(st->value->cstring);
    if (val >= 0 && val < STYLE_COUNT) {
      s_style = val;
      persist_write_int(PERSIST_KEY_STYLE, s_style);
    }
  }
  Tuple *ct = dict_find(iter, MSG_KEY_COLOUR);
  if (ct) {
    int val = -1;
    if (ct->type == TUPLE_INT)     val = (int)ct->value->int32;
    if (ct->type == TUPLE_UINT)    val = (int)ct->value->uint32;
    if (ct->type == TUPLE_CSTRING) val = atoi(ct->value->cstring);
    if (val >= 0 && val < COLOUR_COUNT) {
      s_colour = val;
      persist_write_int(PERSIST_KEY_COLOUR, s_colour);
    }
  }
  apply_settings();
}

static void inbox_dropped(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped: %d", (int)reason);
}

static void appmessage_init(void) {
  app_message_register_inbox_received(inbox_received);
  app_message_register_inbox_dropped(inbox_dropped);
  app_message_open(256, 256);
}

/* ================================================================== */
/*  UPDATE                                                             */
/* ================================================================== */

static void update_display(void) {
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  int h = t->tm_hour % 12, m = t->tm_min;
  s_phase = m % HB_COLS;
  build_story(h, m, fake_steps(m), fake_battery(m), fake_bpm(m), t);
  layer_mark_dirty(s_story_layer);
  layer_mark_dirty(s_clock_layer);
  layer_mark_dirty(s_widget_layer);
}

/* ================================================================== */
/*  MAIN WINDOW                                                        */
/* ================================================================== */

static void main_window_load(Window *window) {
  Layer *root = window_get_root_layer(window);

  /* story layer — fills text zone */
  s_story_layer = layer_create(GRect(TEXT_X, TEXT_Y, TEXT_W, TEXT_H));
  layer_set_update_proc(s_story_layer, story_layer_update);
  layer_add_child(root, s_story_layer);

  /* widget layer — bottom-left */
  s_widget_layer = layer_create(GRect(WG_X, STRIP_Y, WG_W, WG_H));
  layer_set_update_proc(s_widget_layer, widget_layer_update);
  layer_add_child(root, s_widget_layer);

  /* clock layer — bottom-right */
  s_clock_layer = layer_create(GRect(CLOCK_X, STRIP_Y, CLOCK_SIZE, CLOCK_SIZE));
  layer_set_update_proc(s_clock_layer, clock_layer_update);
  layer_add_child(root, s_clock_layer);

  update_display();
}

static void main_window_unload(Window *window) {
  layer_destroy(s_story_layer);
  layer_destroy(s_widget_layer);
  layer_destroy(s_clock_layer);
}

/* ================================================================== */
/*  INIT / DEINIT                                                      */
/* ================================================================== */

static void init(void) {
  /* restore persisted settings */
  s_mode = persist_exists(PERSIST_KEY_MODE)
    ? persist_read_int(PERSIST_KEY_MODE) : MODE_ECG;
  if (s_mode < 0 || s_mode >= MODE_COUNT) s_mode = MODE_ECG;

  s_font = persist_exists(PERSIST_KEY_FONT)
    ? persist_read_int(PERSIST_KEY_FONT) : FONT_GOTHIC14;
  if (s_font < 0 || s_font >= FONT_COUNT) s_font = FONT_GOTHIC14;

  s_style = persist_exists(PERSIST_KEY_STYLE)
    ? persist_read_int(PERSIST_KEY_STYLE) : STYLE_ALLCAPS;
  if (s_style < 0 || s_style >= STYLE_COUNT) s_style = STYLE_ALLCAPS;

  s_colour = persist_exists(PERSIST_KEY_COLOUR)
    ? persist_read_int(PERSIST_KEY_COLOUR) : COLOUR_WARM;
  if (s_colour < 0 || s_colour >= COLOUR_COUNT) s_colour = COLOUR_WARM;


  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorBlack);
  window_set_window_handlers(s_main_window, (WindowHandlers){
    .load   = main_window_load,
    .unload = main_window_unload,
  });
  window_stack_push(s_main_window, true);

  appmessage_init();
  tick_timer_service_subscribe(MINUTE_UNIT, (TickHandler)update_display);
}

static void deinit(void) {
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
