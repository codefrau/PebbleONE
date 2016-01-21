/*
 * Copyright (c) 2013 Bert Freudenberg
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <pebble.h>
#include <time.h>

// keys for app message and storage
#define SECONDS_MODE   0
#define BATTERY_MODE   1
#define DATE_MODE      2
#define BLUETOOTH_MODE 3
#define GRAPHICS_MODE  4
#define CONNLOST_MODE  5
#define DATE_POS       6
#define INBOX_SIZE     (1 + (7+4) * 7)

#define REQUEST_CONFIG 100
#define OUTBOX_SIZE    (1 + (7+4) * 1)

#define SECONDS_MODE_NEVER    0
#define SECONDS_MODE_IFNOTLOW 1
#define SECONDS_MODE_ALWAYS   2
#define BATTERY_MODE_NEVER    0
#define BATTERY_MODE_IF_LOW   1
#define BATTERY_MODE_ALWAYS   2
#define DATE_POS_OFF          0
#define DATE_POS_TOP          1
#define DATE_POS_BOTTOM       2
#define DATE_MODE_OFF         0
#define DATE_MODE_FIRST       1
#define DATE_MODE_EN          1
#define DATE_MODE_DE          2
#define DATE_MODE_ES          3
#define DATE_MODE_FR          4
#define DATE_MODE_IT          5
#define DATE_MODE_SE          6
#define DATE_MODE_LAST        6
#define BLUETOOTH_MODE_NEVER  0
#define BLUETOOTH_MODE_IFOFF  1
#define BLUETOOTH_MODE_ALWAYS 2
#define GRAPHICS_MODE_NORMAL  0
#define GRAPHICS_MODE_INVERT  1
#define CONNLOST_MODE_IGNORE  0
#define CONNLOST_MODE_WARN    1


#define SCREENSHOT 0
#define DEBUG      0

#define ONE        TRIG_MAX_RATIO
#define THREESIXTY TRIG_MAX_ANGLE

#define EXTENT      PBL_IF_ROUND_ELSE(180, 144)
#define CENTER_X    PBL_IF_ROUND_ELSE( 90, 71)
#define CENTER_Y    PBL_IF_ROUND_ELSE( 90, 71)
#define DOTS_RADIUS PBL_IF_ROUND_ELSE( 80, 67)
#define DOTS_SIZE   PBL_IF_ROUND_ELSE(  5,  4)
#define SEC_RADIUS  PBL_IF_ROUND_ELSE( 72, 62)

#define max(a,b) ((a) > (b) ? (a) : (b))
#define min(a,b) ((a) < (b) ? (a) : (b))

static int seconds_mode   = SECONDS_MODE_NEVER;
static int battery_mode   = BATTERY_MODE_IF_LOW;
static int date_pos       = PBL_IF_ROUND_ELSE(DATE_POS_OFF, DATE_POS_BOTTOM);
static int date_mode      = DATE_MODE_EN;
static int bluetooth_mode = BLUETOOTH_MODE_ALWAYS;
static int graphics_mode  = PBL_IF_ROUND_ELSE(GRAPHICS_MODE_INVERT, GRAPHICS_MODE_NORMAL);
static int connlost_mode  = CONNLOST_MODE_WARN;
static bool has_config = false;

static Window *window;
static Layer *background_layer;
static Layer *hands_layer;
static Layer *date_layer;
static Layer *battery_layer;

static GBitmap *logo;
static BitmapLayer *logo_layer;

static GBitmap *bluetooth_images[4];
static BitmapLayer *bluetooth_layer;

static GColor bw_palette[2];
#define BG_COLOR bw_palette[0]
#define FG_COLOR bw_palette[1]

static struct tm *now = NULL;
static int date_wday = -1;
static int date_mday = -1;
static bool hide_seconds = false;
static bool was_connected = false;

static GFont font;
#define DATE_BUFFER_BYTES 32
static char date_buffer[DATE_BUFFER_BYTES];

#if DEBUG
static TextLayer *debug_layer;
#define DEBUG_BUFFER_BYTES 32
static char debug_buffer[DEBUG_BUFFER_BYTES];
#endif

const GPathInfo HOUR_POINTS = {
  6,
  (GPoint []) {
    { 6,-37},
    { 3,-40},
    {-3,-40},
    {-6,-37},
    {-6,  0},
    { 6,  0},
  }
};
static GPath *hour_path;

const GPathInfo MIN_POINTS = {
  6,
  (GPoint []) {
    { 5,-57},
    { 3,-61},
    {-3,-61},
    {-5,-57},
    {-5,  0},
    { 5,  0},
  }
};
static GPath *min_path;

const GPathInfo SEC_POINTS = {
  4,
  (GPoint []) {
    { 2,  0},
    { 2,-61},
    {-2,-61},
    {-2,  0},
  }
};
static GPath *sec_path;

const GPathInfo BATTERY_POINTS = {
  8,
  (GPoint []) {
    { 7, 0},
    {20, 0},
    {20, 6},
    {21, 2},
    {21, 6},
    {20, 6},
    {20, 8},
    { 7, 8},
  }
};
static GPath *battery_path;

const GPathInfo CHARGE_POINTS = {
  4,
  (GPoint []) {
    {3, 1},
    {0, 4},
    {3, 4},
    {0, 7},
  }
};
static GPath *charge_path;

const char WEEKDAY_NAMES[6][7][5] = { // 3 chars, 1 for utf-8, 1 for terminating 0
  {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"},
  {"So",  "Mo",  "Di",  "Mi",  "Do",  "Fr",  "Sa" },
  {"Dom", "Lun", "Mar", "Mié", "Jue", "Vie", "Sáb"},
  {"Dim", "Lun", "Mar", "Mer", "Jeu", "Ven", "Sam"},
  {"Dom", "Lun", "Mar", "Mer", "Gio", "Ven", "Sab"},
  {"Sön", "Mån", "Tis", "Ons", "Tor", "Fre", "Lör"},
};

static Animation *startup_animation;
static int32_t hour_angle = 0;
static int32_t min_angle = 0;
static int32_t sec_angle = 0;
static GPoint hour_pos, hour_delta;
static GPoint min_pos, min_delta;
static GPoint sec_pos, sec_delta;
static int32_t dots_radius = 0;

void background_layer_update_callback(Layer *layer, GContext* ctx) {
  if (!dots_radius) return;
  graphics_context_set_fill_color(ctx, FG_COLOR);
  for (int32_t angle = 0; angle < THREESIXTY; angle += THREESIXTY / 12) {
    GPoint pos = GPoint(
      CENTER_X + dots_radius * cos_lookup(angle) / ONE,
      CENTER_Y - dots_radius * sin_lookup(angle) / ONE);
    graphics_fill_circle(ctx, pos, DOTS_SIZE);
  }
}

void update_angles() {
  hour_angle = THREESIXTY * (now->tm_hour * 5 + now->tm_min / 12) / 60;
  min_angle = THREESIXTY * now->tm_min / 60;
  sec_angle = THREESIXTY * now->tm_sec / 60;
}

void hands_layer_update_callback(Layer *layer, GContext* ctx) {
#if SCREENSHOT
  now->tm_hour = 10;
  now->tm_min = 9;
  now->tm_sec = 36;
#endif
  update_angles();
  
  // hours and minutes
  gpath_rotate_to(hour_path, hour_angle);
  gpath_rotate_to(min_path, min_angle);
  graphics_context_set_fill_color(ctx, FG_COLOR);
  graphics_context_set_stroke_color(ctx, BG_COLOR);
  gpath_draw_filled(ctx, hour_path);
  gpath_draw_outline(ctx, hour_path);
  graphics_fill_circle(ctx, hour_pos, DOTS_SIZE+3);
  gpath_draw_filled(ctx, min_path);
  gpath_draw_outline(ctx, min_path);
  graphics_fill_circle(ctx, min_pos, DOTS_SIZE+3);

  // seconds
  if (!hide_seconds) {
    GPoint sec_end = GPoint(
      sec_pos.x + SEC_RADIUS * sin_lookup(sec_angle) / ONE,
      sec_pos.y - SEC_RADIUS * cos_lookup(sec_angle) / ONE);
    graphics_context_set_fill_color(ctx, BG_COLOR);
    gpath_rotate_to(sec_path, sec_angle);
    gpath_draw_filled(ctx, sec_path);
    graphics_context_set_stroke_color(ctx, FG_COLOR);
    graphics_context_set_compositing_mode(ctx, GCompOpAssignInverted);
    graphics_draw_line(ctx, sec_pos, sec_end);
  }

  // center dot
  graphics_context_set_fill_color(ctx, BG_COLOR);
  graphics_fill_circle(ctx, min_pos, DOTS_SIZE);
}

void date_layer_update_callback(Layer *layer, GContext* ctx) {

#if SCREENSHOT
  now->tm_wday = 0;
  now->tm_mday = 25;
#endif

  graphics_context_set_text_color(ctx, FG_COLOR);

  // weekday
  if (date_mode < DATE_MODE_FIRST || date_mode > DATE_MODE_LAST)
    date_mode = DATE_MODE_FIRST;
  graphics_draw_text(ctx,
    WEEKDAY_NAMES[date_mode - DATE_MODE_FIRST][now->tm_wday],
    font,
    GRect(0, -6, EXTENT, 32),
    GTextOverflowModeWordWrap,
    GTextAlignmentLeft,
    NULL);

  // day of month
  strftime(date_buffer, DATE_BUFFER_BYTES, "%e", now);
  graphics_draw_text(ctx,
    date_buffer,
    font,
    GRect(0, -6, EXTENT, 32),
    GTextOverflowModeWordWrap,
    GTextAlignmentRight,
    NULL);

  date_wday = now->tm_wday;
  date_mday = now->tm_mday;
}

void battery_layer_update_callback(Layer *layer, GContext* ctx) {
  BatteryChargeState battery = battery_state_service_peek();
  graphics_context_set_stroke_color(ctx, FG_COLOR);
  gpath_draw_outline(ctx, battery_path);
  int width = battery.charge_percent * 10 / 100;
  #ifdef PBL_BW
  graphics_context_set_fill_color(ctx, FG_COLOR);
  #else
  graphics_context_set_fill_color(ctx, battery.is_plugged ? (width < 3 ? GColorRed : GColorGreen) : FG_COLOR);
  #endif
  graphics_fill_rect(ctx, GRect(9, 2, width, 5), 0, GCornerNone);  
  if (battery.is_plugged)
    gpath_draw_outline_open(ctx, charge_path);
}

void handle_tick(struct tm *tick_time, TimeUnits units_changed) {
  time_t clock = time(NULL);
  now = localtime(&clock);
  layer_mark_dirty(hands_layer);
  if (date_pos != DATE_POS_OFF && (now->tm_wday != date_wday || now->tm_mday != date_mday))
    layer_mark_dirty(date_layer);
}

void handle_app_did_focus(bool in_focus) {
  if (in_focus && startup_animation) {
    animation_schedule(startup_animation);
    startup_animation = NULL;
  }
}

void startup_animation_init() {
  update_angles();
  hour_delta = GPoint(
    DOTS_RADIUS * sin_lookup(hour_angle) / ONE,
    -DOTS_RADIUS * cos_lookup(hour_angle) / ONE);
  gpath_move_to(hour_path, GPoint(CENTER_X + hour_delta.x, CENTER_Y + hour_delta.y));
  min_delta = GPoint(
    DOTS_RADIUS * sin_lookup(min_angle) / ONE,
    -DOTS_RADIUS * cos_lookup(min_angle) / ONE);
  gpath_move_to(min_path, GPoint(CENTER_X + min_delta.x, CENTER_Y + min_delta.y));
  sec_delta = GPoint(
    DOTS_RADIUS * sin_lookup(sec_angle) / ONE,
    -DOTS_RADIUS * cos_lookup(sec_angle) / ONE);
  gpath_move_to(sec_path, GPoint(CENTER_X + sec_delta.x, CENTER_Y + sec_delta.y));
}

void startup_animation_update(Animation *animation, const AnimationProgress progress) {
  dots_radius = DOTS_RADIUS * progress / ANIMATION_NORMALIZED_MAX;
  AnimationProgress reverse = ANIMATION_NORMALIZED_MAX - progress;
  hour_pos = GPoint(
    CENTER_X + hour_delta.x * reverse / ANIMATION_NORMALIZED_MAX, 
    CENTER_Y + hour_delta.y * reverse / ANIMATION_NORMALIZED_MAX);
  gpath_move_to(hour_path, hour_pos);
  min_pos = GPoint(
    CENTER_X + min_delta.x * reverse / ANIMATION_NORMALIZED_MAX, 
    CENTER_Y + min_delta.y * reverse / ANIMATION_NORMALIZED_MAX);
  gpath_move_to(min_path, min_pos);
  sec_pos = GPoint(
    CENTER_X + sec_delta.x * reverse / ANIMATION_NORMALIZED_MAX, 
    CENTER_Y + sec_delta.y * reverse / ANIMATION_NORMALIZED_MAX);
  gpath_move_to(sec_path, sec_pos);
  layer_mark_dirty(background_layer);
  layer_mark_dirty(hands_layer);
}

void startup_animation_teardown(Animation *animation) {
  startup_animation_update(animation, ANIMATION_NORMALIZED_MAX);
  animation_destroy(animation);
}

static AnimationImplementation startup_animation_implementation = {
  .update = startup_animation_update,
  .teardown = startup_animation_teardown
};

void lost_connection_warning(void *);

void handle_bluetooth(bool connected) {
  bitmap_layer_set_bitmap(bluetooth_layer, bluetooth_images[connected ? 1 : 0]);
  layer_set_hidden(bitmap_layer_get_layer(bluetooth_layer),
    bluetooth_mode == BLUETOOTH_MODE_NEVER ||
    (bluetooth_mode == BLUETOOTH_MODE_IFOFF && connected));
  if (was_connected && !connected && connlost_mode == CONNLOST_MODE_WARN)
      lost_connection_warning((void*) 0);
  was_connected = connected;
}

void lost_connection_warning(void *data) {
  int count = (int) data;
  bool on_off = count & 1;
  // blink icon
  bitmap_layer_set_bitmap(bluetooth_layer, bluetooth_images[on_off ? 1 : 0]);
  layer_set_hidden(bitmap_layer_get_layer(bluetooth_layer), false);
  // buzz 3 times
  if (count < 6 && !on_off)
    vibes_short_pulse();
  if (count < 50) // blink for 15 seconds
    app_timer_register(300, lost_connection_warning, (void*) (count+1));
  else // restore bluetooth icon
    handle_bluetooth(bluetooth_connection_service_peek());
}

void handle_layout() {
  BatteryChargeState charge_state = battery_state_service_peek();
  bool battery_is_low = charge_state.charge_percent <= 20;
  bool showSeconds = seconds_mode == SECONDS_MODE_ALWAYS
    || (seconds_mode == SECONDS_MODE_IFNOTLOW && (!battery_is_low || charge_state.is_plugged));
  bool showBattery = battery_mode == BATTERY_MODE_ALWAYS
    || (battery_mode == BATTERY_MODE_IF_LOW && battery_is_low)
    || charge_state.is_plugged;
  int face_top = PBL_IF_ROUND_ELSE(0, date_pos == DATE_POS_BOTTOM ? 0 : date_pos == DATE_POS_OFF ? 12 : 24);
  int date_top = date_pos == DATE_POS_TOP ? 0 : EXTENT;
  int battery_top = date_pos == DATE_POS_TOP ? 168-10-3 : 3;
  if (hide_seconds != !showSeconds) {
    hide_seconds = !showSeconds;
    tick_timer_service_unsubscribe();
    tick_timer_service_subscribe(hide_seconds ? MINUTE_UNIT : SECOND_UNIT, &handle_tick);
  }
  layer_set_hidden(date_layer, date_pos == DATE_POS_OFF);
  layer_set_hidden(battery_layer, !showBattery);
  layer_set_frame(background_layer, GRect(0, face_top, EXTENT, EXTENT));
  layer_set_frame(date_layer, GRect(0, date_top, EXTENT, 24));
  layer_set_frame(battery_layer, GRect(EXTENT-22-3, battery_top, 22, 10));
  FG_COLOR = graphics_mode == GRAPHICS_MODE_INVERT ? GColorBlack : GColorWhite;
  BG_COLOR = graphics_mode == GRAPHICS_MODE_INVERT ? GColorWhite : GColorBlack;
  window_set_background_color(window, BG_COLOR);
}

void handle_battery(BatteryChargeState charge_state) {
  handle_layout();
  layer_mark_dirty(battery_layer);
}

void handle_appmessage_receive(DictionaryIterator *received, void *context) {
  Tuple *tuple = dict_read_first(received);
  while (tuple) {
    switch (tuple->key) {
      case SECONDS_MODE:
        seconds_mode = tuple->value->int32;
        break;
      case BATTERY_MODE:
        battery_mode = tuple->value->int32;
        break;
      case DATE_POS:
        date_pos = tuple->value->int32;
        break;
      case DATE_MODE:
        date_mode = tuple->value->int32;
        break;
      case BLUETOOTH_MODE:
        bluetooth_mode = tuple->value->int32;
        break;
      case GRAPHICS_MODE:
        graphics_mode = tuple->value->int32;
        break;
      case CONNLOST_MODE:
        connlost_mode = tuple->value->int32;
        break;
    }
    tuple = dict_read_next(received);
  }
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Received config");
  has_config = true;
  handle_bluetooth(bluetooth_connection_service_peek());
  handle_layout();
  layer_mark_dirty(battery_layer);
  layer_mark_dirty(hands_layer);
  layer_mark_dirty(date_layer);
  layer_mark_dirty(background_layer);
}

void request_config(void) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Requesting config");
  Tuplet request_tuple = TupletInteger(REQUEST_CONFIG, 1);
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  if (!iter) return;
  dict_write_tuplet(iter, &request_tuple);
  dict_write_end(iter);
  app_message_outbox_send();
}


void handle_init() {
  time_t clock = time(NULL);
  now = localtime(&clock);
  window = window_create();
  window_stack_push(window, true /* Animated */);
  window_set_background_color(window, BG_COLOR);

  date_layer = layer_create(GRect(0, EXTENT, EXTENT, 24));
  layer_set_update_proc(date_layer, &date_layer_update_callback);
  layer_add_child(window_get_root_layer(window), date_layer);

  background_layer = layer_create(GRect(0, 0, EXTENT, EXTENT));
  layer_set_update_proc(background_layer, &background_layer_update_callback);
  layer_add_child(window_get_root_layer(window), background_layer);

  logo = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_LOGO);
  gbitmap_set_palette(logo, bw_palette, false);
  GRect frame = gbitmap_get_bounds(logo);
  grect_align(&frame, &GRect(0, 0, EXTENT, CENTER_Y), GAlignCenter, false);
  logo_layer = bitmap_layer_create(frame);
  bitmap_layer_set_bitmap	(logo_layer, logo);
  layer_add_child(background_layer, bitmap_layer_get_layer(logo_layer));

  hands_layer = layer_create(layer_get_frame(background_layer));
  layer_set_update_proc(hands_layer, &hands_layer_update_callback);
  layer_add_child(background_layer, hands_layer);

  battery_layer = layer_create(GRect(EXTENT-22-3, 3, 22, 10));
  layer_set_update_proc(battery_layer, &battery_layer_update_callback);
  layer_add_child(window_get_root_layer(window), battery_layer);

  for (int i = 0; i < 2; i++) {
    bluetooth_images[i] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BLUETOOTH_OFF + i);  
    gbitmap_set_palette(bluetooth_images[i], bw_palette, false);
  }
  bluetooth_layer = bitmap_layer_create(GRect(CENTER_X - 6, CENTER_Y - DOTS_RADIUS - 4, 13, 13));
  layer_add_child(background_layer, bitmap_layer_get_layer(bluetooth_layer));

#if DEBUG
  debug_layer = text_layer_create(GRect(0, 0, 32, 16));
  strcpy(debug_buffer, "");
  text_layer_set_text(debug_layer, debug_buffer);
  text_layer_set_text_color(debug_layer, FG_COLOR);
  text_layer_set_background_color(debug_layer, BG_COLOR);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(debug_layer));
#endif
  
  hour_path = gpath_create(&HOUR_POINTS);
  gpath_move_to(hour_path, GPoint(CENTER_X, CENTER_Y));
  min_path = gpath_create(&MIN_POINTS);
  gpath_move_to(min_path, GPoint(CENTER_X, CENTER_Y));
  sec_path = gpath_create(&SEC_POINTS);
  gpath_move_to(sec_path, GPoint(CENTER_X, CENTER_Y));
  battery_path = gpath_create(&BATTERY_POINTS);
  charge_path = gpath_create(&CHARGE_POINTS);

  startup_animation = animation_create();
  animation_set_curve(startup_animation, AnimationCurveEaseOut);
  animation_set_implementation(startup_animation, &startup_animation_implementation);
  startup_animation_init();
  app_focus_service_subscribe_handlers((AppFocusHandlers){
    .did_focus = handle_app_did_focus,
  });

  font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_30));

  has_config = true;
  if (persist_exists(SECONDS_MODE)) seconds_mode = persist_read_int(SECONDS_MODE); else has_config = false;
  if (persist_exists(BATTERY_MODE)) battery_mode = persist_read_int(BATTERY_MODE); else has_config = false;
  if (persist_exists(DATE_POS)) date_pos = persist_read_int(DATE_POS); // added in 2.7
  if (persist_exists(DATE_MODE)) date_mode = persist_read_int(DATE_MODE); else has_config = false;
  if (persist_exists(BLUETOOTH_MODE)) bluetooth_mode = persist_read_int(BLUETOOTH_MODE); else has_config = false;
  if (persist_exists(GRAPHICS_MODE)) graphics_mode = persist_read_int(GRAPHICS_MODE); else has_config = false;
  if (persist_exists(CONNLOST_MODE)) connlost_mode = persist_read_int(CONNLOST_MODE); else has_config = false;
  if (has_config) APP_LOG(APP_LOG_LEVEL_DEBUG, "Loaded config");
  tick_timer_service_subscribe(hide_seconds ? MINUTE_UNIT : SECOND_UNIT, &handle_tick);
  handle_layout();
  battery_state_service_subscribe(&handle_battery);
  bluetooth_connection_service_subscribe(&handle_bluetooth);
  handle_bluetooth(bluetooth_connection_service_peek());
  app_message_register_inbox_received(&handle_appmessage_receive);
  app_message_open(INBOX_SIZE, OUTBOX_SIZE);
  if (!has_config) request_config();
}

void handle_deinit() {
  app_message_deregister_callbacks();
  battery_state_service_unsubscribe();
  tick_timer_service_unsubscribe();
  if (has_config) {
    persist_write_int(SECONDS_MODE, seconds_mode);
    persist_write_int(BATTERY_MODE, battery_mode);
    persist_write_int(DATE_POS, date_pos);
    persist_write_int(DATE_MODE, date_mode);
    persist_write_int(BLUETOOTH_MODE, bluetooth_mode);
    persist_write_int(GRAPHICS_MODE, graphics_mode);
    persist_write_int(CONNLOST_MODE, connlost_mode);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Wrote config");
  } else {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Did not write config");
  }
  fonts_unload_custom_font(font);
  app_focus_service_unsubscribe();
  animation_unschedule_all();
  gpath_destroy(charge_path);
  gpath_destroy(battery_path);
  gpath_destroy(sec_path);
  gpath_destroy(min_path);
  gpath_destroy(hour_path);
#if DEBUG
  text_layer_destroy(debug_layer);
#endif
  layer_destroy(hands_layer);
  bitmap_layer_destroy(logo_layer);
  gbitmap_destroy(logo);
  layer_destroy(battery_layer);
  bitmap_layer_destroy(bluetooth_layer);
  for (int i = 0; i < 2; i++)
    gbitmap_destroy(bluetooth_images[i]);
  layer_destroy(background_layer);
  layer_destroy(date_layer);
  window_destroy(window);
}

int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}
