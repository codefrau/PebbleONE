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

#define SECONDS_MODE   0
#define BATTERY_MODE   1
#define DATE_MODE      2
#define BLUETOOTH_MODE 3

#define SECONDS_MODE_NEVER    0
#define SECONDS_MODE_IFNOTLOW 1
#define SECONDS_MODE_ALWAYS   2
#define BATTERY_MODE_NEVER    0
#define BATTERY_MODE_IF_LOW   1
#define BATTERY_MODE_ALWAYS   2
#define DATE_MODE_NEVER       0
#define DATE_MODE_ALWAYS      1
#define BLUETOOTH_MODE_NEVER  0
#define BLUETOOTH_MODE_IFOFF  1
#define BLUETOOTH_MODE_ALWAYS 2


#define SCREENSHOT 0
#define DEBUG      0

#define ONE        TRIG_MAX_RATIO
#define THREESIXTY TRIG_MAX_ANGLE

#define CENTER_X    71
#define CENTER_Y    71
#define DOTS_RADIUS 67
#define DOTS_SIZE    4
#define HOUR_RADIUS 40
#define MIN_RADIUS  60
#define SEC_RADIUS  62

#define max(a,b) ((a) > (b) ? (a) : (b))

static int seconds_mode   = SECONDS_MODE_ALWAYS;
static int battery_mode   = BATTERY_MODE_IF_LOW;
static int date_mode      = DATE_MODE_ALWAYS;
static int bluetooth_mode = BLUETOOTH_MODE_NEVER;

static Window *window;
static Layer *background_layer;
static Layer *hands_layer;
static Layer *date_layer;

static GBitmap *logo;
static BitmapLayer *logo_layer;

static GBitmap *battery_images[22];
static BitmapLayer *battery_layer;

static GBitmap *bluetooth_images[4];
static BitmapLayer *bluetooth_layer;

static struct tm *now = NULL;
static int date_wday = -1;
static int date_mday = -1;
static bool hide_seconds = false;

static GFont *font;
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

void background_layer_update_callback(Layer *layer, GContext* ctx) {
	graphics_context_set_fill_color(ctx, GColorWhite);
  for (int32_t angle = 0; angle < THREESIXTY; angle += THREESIXTY / 12) {
    GPoint pos = GPoint(
      CENTER_X + DOTS_RADIUS * cos_lookup(angle) / ONE,
      CENTER_Y + DOTS_RADIUS * sin_lookup(angle) / ONE);
    graphics_fill_circle(ctx, pos, DOTS_SIZE);
  }
}

void hands_layer_update_callback(Layer *layer, GContext* ctx) {
#if SCREENSHOT
  now->tm_hour = 10;
  now->tm_min = 9;
  now->tm_sec = 36;
#endif

  GPoint center = GPoint(CENTER_X, CENTER_Y);

  // hours and minutes
  int32_t hour_angle = THREESIXTY * (now->tm_hour * 5 + now->tm_min / 12) / 60;
  int32_t min_angle = THREESIXTY * now->tm_min / 60;
  gpath_rotate_to(hour_path, hour_angle);
  gpath_rotate_to(min_path, min_angle);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_context_set_stroke_color(ctx, GColorBlack);
  gpath_draw_filled(ctx, hour_path);
  gpath_draw_outline(ctx, hour_path);
  graphics_draw_circle(ctx, center, DOTS_SIZE+4);
  gpath_draw_filled(ctx, min_path);
  gpath_draw_outline(ctx, min_path);
  graphics_fill_circle(ctx, center, DOTS_SIZE+3);

  // seconds
  if (!hide_seconds) {
    int32_t sec_angle = THREESIXTY * now->tm_sec / 60;
    GPoint sec_pos = GPoint(
      CENTER_X + SEC_RADIUS * sin_lookup(sec_angle) / ONE,
      CENTER_Y - SEC_RADIUS * cos_lookup(sec_angle) / ONE);
    graphics_context_set_fill_color(ctx, GColorBlack);
    gpath_rotate_to(sec_path, sec_angle);
    gpath_draw_filled(ctx, sec_path);
    graphics_context_set_stroke_color(ctx, GColorWhite);
    graphics_context_set_compositing_mode(ctx, GCompOpAssignInverted);
    graphics_draw_line(ctx, center, sec_pos);
  }

  // center dot
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, center, DOTS_SIZE);
}

void date_layer_update_callback(Layer *layer, GContext* ctx) {

#if SCREENSHOT
  now->tm_wday = 0;
  now->tm_mday = 25;
#endif

  graphics_context_set_text_color(ctx, GColorWhite);

  // weekday
  strftime(date_buffer, DATE_BUFFER_BYTES, "%a", now);
  graphics_draw_text(ctx,
    date_buffer,
    font,
    GRect(0, -6, 144, 32),
    GTextOverflowModeWordWrap,
    GTextAlignmentLeft,
    NULL);

  // day of month
  strftime(date_buffer, DATE_BUFFER_BYTES, "%e", now);
  graphics_draw_text(ctx,
    date_buffer,
    font,
    GRect(0, -6, 144, 32),
    GTextOverflowModeWordWrap,
    GTextAlignmentRight,
    NULL);

  date_wday = now->tm_wday;
  date_mday = now->tm_mday;
}

void handle_tick(struct tm *tick_time, TimeUnits units_changed) {
  now = tick_time;
  layer_mark_dirty(hands_layer);
  if (date_mode != DATE_MODE_NEVER && (now->tm_wday != date_wday || now->tm_mday != date_mday))
    layer_mark_dirty(date_layer);
}

void handle_bluetooth(bool connected) {
  bitmap_layer_set_bitmap(bluetooth_layer, bluetooth_images[connected ? 1 : 0]);
  layer_set_hidden(bitmap_layer_get_layer(bluetooth_layer),
    bluetooth_mode == BLUETOOTH_MODE_NEVER ||
    (bluetooth_mode == BLUETOOTH_MODE_IFOFF && connected));
}

void handle_battery(BatteryChargeState charge_state) {
#if DEBUG
  //strftime(debug_buffer, DEBUG_BUFFER_BYTES, "%d.%m.%Y %H:%M:%S", now);
  snprintf(debug_buffer, DEBUG_BUFFER_BYTES, "%s%d%%",  charge_state.is_charging ? "+" : "", charge_state.charge_percent);
  text_layer_set_text(debug_layer, debug_buffer);
#endif
#if SCREENSHOT
  bitmap_layer_set_bitmap(battery_layer, battery_images[1]);
  bool showSeconds = true;
  bool showBattery = true;
  bool showDate = true;
#else
  bitmap_layer_set_bitmap(battery_layer, battery_images[
    (charge_state.is_charging ? 11 : 0) + max(charge_state.charge_percent / 9, 10)]); // 90% is max
  bool battery_is_low = charge_state.charge_percent <= 10;
  bool showSeconds = seconds_mode == SECONDS_MODE_ALWAYS
    || (seconds_mode == SECONDS_MODE_IFNOTLOW && (!battery_is_low || charge_state.is_charging));
  bool showBattery = battery_mode == BATTERY_MODE_ALWAYS
    || (battery_mode == BATTERY_MODE_IF_LOW && battery_is_low)
    || charge_state.is_charging;
  bool showDate = date_mode != DATE_MODE_NEVER;
#endif
  if (hide_seconds != !showSeconds) {
    hide_seconds = !showSeconds;
    tick_timer_service_unsubscribe();
    tick_timer_service_subscribe(hide_seconds ? MINUTE_UNIT : SECOND_UNIT, &handle_tick);
  }
  layer_set_hidden(bitmap_layer_get_layer(battery_layer), !showBattery);
  if (layer_get_hidden(date_layer) != !showDate) {
    layer_set_hidden(date_layer, !showDate);
    layer_set_frame(background_layer, GRect(0, showDate ? 0 : 12, 144, 144));
  }
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
        case DATE_MODE:
          date_mode = tuple->value->int32;
          break;
        case BLUETOOTH_MODE:
          bluetooth_mode = tuple->value->int32;
          break;
    }
    tuple = dict_read_next(received);
  }
  handle_battery(battery_state_service_peek());
  handle_bluetooth(bluetooth_connection_service_peek());
}


void handle_init() {
  time_t clock = time(NULL);
  now = localtime(&clock);
  window = window_create();
  window_stack_push(window, true /* Animated */);
  window_set_background_color(window, GColorBlack);

  date_layer = layer_create(GRect(0, 144, 144, 24));
  layer_set_update_proc(date_layer, &date_layer_update_callback);
  layer_add_child(window_get_root_layer(window), date_layer);

  background_layer = layer_create(GRect(0, 0, 144, 144));
  layer_set_update_proc(background_layer, &background_layer_update_callback);
  layer_add_child(window_get_root_layer(window), background_layer);

  logo = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_LOGO);  
  GRect frame = logo->bounds;
  grect_align(&frame, &GRect(0, 0, 144, 72), GAlignCenter, false);
  logo_layer = bitmap_layer_create(frame);
  bitmap_layer_set_bitmap	(logo_layer, logo);
  layer_add_child(background_layer, bitmap_layer_get_layer(logo_layer));

  hands_layer = layer_create(layer_get_frame(background_layer));
  layer_set_update_proc(hands_layer, &hands_layer_update_callback);
  layer_add_child(background_layer, hands_layer);

  for (int i = 0; i < 22; i++)
    battery_images[i] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY_0 + i);  
  battery_layer = bitmap_layer_create(GRect(144-16-3, 3, 16, 10));
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(battery_layer));

  for (int i = 0; i < 2; i++)
    bluetooth_images[i] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BLUETOOTH_OFF + i);  
  bluetooth_layer = bitmap_layer_create(GRect(66, 0, 13, 13));
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(bluetooth_layer));

#if DEBUG
  debug_layer = text_layer_create(GRect(0, 0, 32, 16));
  strcpy(debug_buffer, "");
  text_layer_set_text(debug_layer, debug_buffer);
  text_layer_set_text_color(debug_layer, GColorWhite);
  text_layer_set_background_color(debug_layer, GColorBlack);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(debug_layer));
#endif
  
  hour_path = gpath_create(&HOUR_POINTS);
  gpath_move_to(hour_path, GPoint(CENTER_X, CENTER_Y));
  min_path = gpath_create(&MIN_POINTS);
  gpath_move_to(min_path, GPoint(CENTER_X, CENTER_Y));
  sec_path = gpath_create(&SEC_POINTS);
  gpath_move_to(sec_path, GPoint(CENTER_X, CENTER_Y));

  font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_30));

  if (persist_exists(SECONDS_MODE)) seconds_mode = persist_read_int(SECONDS_MODE);
  if (persist_exists(BATTERY_MODE)) battery_mode = persist_read_int(BATTERY_MODE);
  if (persist_exists(DATE_MODE)) date_mode = persist_read_int(DATE_MODE);
  if (persist_exists(BLUETOOTH_MODE)) bluetooth_mode = persist_read_int(BLUETOOTH_MODE);
  tick_timer_service_subscribe(hide_seconds ? MINUTE_UNIT : SECOND_UNIT, &handle_tick);
  battery_state_service_subscribe(&handle_battery);
  handle_battery(battery_state_service_peek());
  bluetooth_connection_service_subscribe(&handle_bluetooth);
  handle_bluetooth(bluetooth_connection_service_peek());
  app_message_register_inbox_received(&handle_appmessage_receive);
  app_message_open(64, 0);
}

void handle_deinit() {
  app_message_deregister_callbacks();
  battery_state_service_unsubscribe();
  tick_timer_service_unsubscribe();
  persist_write_int(SECONDS_MODE, seconds_mode);
  persist_write_int(BATTERY_MODE, battery_mode);
  persist_write_int(DATE_MODE, date_mode);
  persist_write_int(BLUETOOTH_MODE, bluetooth_mode);
  fonts_unload_custom_font(font);
  gpath_destroy(sec_path);
  gpath_destroy(min_path);
  gpath_destroy(hour_path);
#if DEBUG
  text_layer_destroy(debug_layer);
#endif
  layer_destroy(hands_layer);
  bitmap_layer_destroy(logo_layer);
  gbitmap_destroy(logo);
  bitmap_layer_destroy(battery_layer);
  for (int i = 0; i < 22; i++)
    gbitmap_destroy(battery_images[i]);
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
