#include <pebble.h>

/* ---------------- FORWARD DECLARATIONS ---------------- */

static const char* minute_phrase(int m);
static void build_literary_block(char *out, size_t size,
                                  int h, int m,
                                  int steps, int battery,
                                  struct tm *t);

static void update_ascii_clock(struct tm *t);

/* ---------------- GLOBALS ---------------- */

static Window *s_window;
static TextLayer *s_text_layer;
static TextLayer *s_clock_layer;

static char s_buffer[256];
static char s_clock_buffer[64];

/* ---------------- METRICS ---------------- */

static int fake_steps(int m) {
  return (m * 37) % 12000;
}

static int fake_battery(int m) {
  return 100 - (m / 6);
}

/* ---------------- DATE ---------------- */

static void get_date(char *buf, size_t size, struct tm *t) {
  strftime(buf, size, "%A, %d %B %Y", t);
}

/* ---------------- WORD CLOCK ---------------- */

static const char *hours[] = {
  "TWELVE","ONE","TWO","THREE","FOUR","FIVE","SIX",
  "SEVEN","EIGHT","NINE","TEN","ELEVEN"
};

static const char* minute_phrase(int m) {
  if (m < 5) return "O'CLOCK";
  if (m < 10) return "FIVE PAST";
  if (m < 15) return "TEN PAST";
  if (m < 20) return "QUARTER PAST";
  if (m < 25) return "TWENTY PAST";
  if (m < 30) return "TWENTY FIVE PAST";
  if (m < 35) return "HALF PAST";
  if (m < 40) return "TWENTY FIVE TO";
  if (m < 45) return "TWENTY TO";
  if (m < 50) return "QUARTER TO";
  if (m < 55) return "TEN TO";
  return "FIVE TO";
}

/* ---------------- LITERARY ENGINE ---------------- */

static const char *verbs[] = {
  "moves", "drifts", "fractures", "echoes", "persists", "unfolds"
};

static const char *states[] = {
  "quietly", "without permission", "inside repetition",
  "beyond recognition", "as memory", "like residue"
};

static const char *metaphors[] = {
  "archive", "corridor", "system", "definition", "instance", "record"
};

static const char *body_states[] = {
  "stable", "observed", "unverified", "contained", "measured", "fragile"
};

static const char* pick(const char **arr, int len, int seed) {
  return arr[seed % len];
}

/* ---------------- ASCII ANALOG DIAL ---------------- */

static void update_ascii_clock(struct tm *t) {
  int h = t->tm_hour % 12;
  int m = t->tm_min;

  int pos = ((h * 60 + m) * 12) / 720;

  char hand;

  switch (pos) {
    case 0: case 11: hand = '^'; break;   // 12
    case 1: case 2:  hand = '/'; break;   // 1-2
    case 3: case 4:  hand = '>'; break;   // 3
    case 5: case 6:  hand = '\\'; break;  // 4-5
    case 7: case 8:  hand = 'v'; break;   // 6
    case 9: case 10: hand = '<'; break;   // 7-8
    default: hand = 'o'; break;
  }

  snprintf(
    s_clock_buffer,
    sizeof(s_clock_buffer),

    "   12\n"
    "9  %c  3\n"
    "   6",

    hand
  );

  text_layer_set_text(s_clock_layer, s_clock_buffer);
}

/* ---------------- LITERARY ENGINE ---------------- */

static void build_literary_block(char *out, size_t size,
                                  int h, int m,
                                  int steps, int battery,
                                  struct tm *t) {

  char date[32];
  get_date(date, sizeof(date), t);

  const char *v = pick(verbs, 6, h + m);
  const char *s = pick(states, 6, m);
  const char *m1 = pick(metaphors, 6, h);
  const char *b = pick(body_states, 6, m);

  snprintf(
    out,
    size,

    "%s %s %s\n\n"
    "YOU WALKED %d STEPS THROUGH THE %s\n"
    "POWER REMAINS %d%% — SYSTEM %s\n\n"
    "%s %s %s\n"
    "THE BODY IS %s",

    minute_phrase(m),
    m1,
    v,

    steps,
    m1,

    battery,
    s,

    date,
    v,
    s,

    b
  );
}

/* ---------------- UPDATE ---------------- */

static void update_display() {
  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  int h = t->tm_hour % 12;
  int m = t->tm_min;

  build_literary_block(
    s_buffer,
    sizeof(s_buffer),
    h, m,
    fake_steps(m),
    fake_battery(m),
    t
  );

  text_layer_set_text(s_text_layer, s_buffer);
  update_ascii_clock(t);
}

/* ---------------- WINDOW ---------------- */

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(root);

  /* MAIN TEXT */
  GRect main_frame = GRect(6, 6, bounds.size.w - 12, bounds.size.h - 50);

  s_text_layer = text_layer_create(main_frame);

  text_layer_set_background_color(s_text_layer, GColorBlack);
  text_layer_set_text_color(s_text_layer, GColorWhite);
  text_layer_set_font(s_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_text_layer, GTextAlignmentLeft);

  layer_add_child(root, text_layer_get_layer(s_text_layer));

  /* ASCII DIAL CLOCK (BOTTOM RIGHT) */
  GRect clock_frame = GRect(bounds.size.w - 50, bounds.size.h - 42, 48, 40);

  s_clock_layer = text_layer_create(clock_frame);

  text_layer_set_background_color(s_clock_layer, GColorBlack);
  text_layer_set_text_color(s_clock_layer, GColorWhite);
  text_layer_set_font(s_clock_layer, fonts_get_system_font(FONT_KEY_GOTHIC_09));
  text_layer_set_text_alignment(s_clock_layer, GTextAlignmentCenter);

  layer_add_child(root, text_layer_get_layer(s_clock_layer));

  update_display();
}

static void window_unload(Window *window) {
  text_layer_destroy(s_text_layer);
  text_layer_destroy(s_clock_layer);
}

/* ---------------- INIT ---------------- */

static void init() {
  s_window = window_create();

  window_set_background_color(s_window, GColorBlack);

  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload
  });

  window_stack_push(s_window, true);

  tick_timer_service_subscribe(MINUTE_UNIT, (TickHandler)update_display);
}

/* ---------------- CLEANUP ---------------- */

static void deinit() {
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}