#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"

#define MY_UUID { 0xF6, 0xC1, 0xAD, 0x30, 0x2E, 0x14, 0x42, 0x7A, 0xB9, 0x59, 0x94, 0x42, 0xF3, 0xA4, 0x69, 0x49 }
PBL_APP_INFO(MY_UUID,
             "Bert ONE", "Bert Freudenberg",
             1, 0, /* App version */
             RESOURCE_ID_IMAGE_MENU_ICON,
             APP_INFO_WATCH_FACE);

#define ONE        TRIG_MAX_RATIO
#define THREESIXTY TRIG_MAX_ANGLE

#define CENTER_X    71
#define CENTER_Y    71
#define DOTS_RADIUS 67
#define DOTS_SIZE    4
#define HOUR_RADIUS 40
#define MIN_RADIUS  60
#define SEC_RADIUS  62

Window window;
Layer background_layer;
Layer hands_layer;
Layer date_layer;

GFont font;
#define DATE_BUFFER_BYTES 32
char date_buffer[DATE_BUFFER_BYTES];

#if DEBUG
TextLayer debug_layer;
#define DEBUG_BUFFER_BYTES 32
char debug_buffer[DEBUG_BUFFER_BYTES];
#endif

const char* WEEKDAYS[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

const GPathInfo HOUR_POINTS = {
  8,
  (GPoint []) {
    { 5,-39},
    { 3,-42},
    {-3,-42},
    {-5,-39},
    {-5,  4},
    {-3,  7},
    { 3,  7},
    { 5,  4},
  }
};
GPath hour_path;

const GPathInfo MIN_POINTS = {
  8,
  (GPoint []) {
    { 5,-56},
    { 3,-59},
    {-3,-59},
    {-5,-56},
    {-5,  4},
    {-3,  7},
    { 3,  7},
    { 5,  4},
  }
};
GPath min_path;

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
  PblTm t;
  get_time(&t);

#if DEBUG
  string_format_time(debug_buffer, DEBUG_BUFFER_BYTES, "%d.%m.%Y %H:%M:%S", &t);
  text_layer_set_text(&debug_layer, debug_buffer);
#endif

  int32_t hour_angle = THREESIXTY * (t.tm_hour * 5 + t.tm_min / 12) / 60;
  int32_t min_angle = THREESIXTY * t.tm_min / 60;
  int32_t sec_angle = THREESIXTY * t.tm_sec / 60;

  // hours and minutes
  gpath_rotate_to(&hour_path, hour_angle);
  gpath_rotate_to(&min_path, min_angle);
  graphics_context_set_fill_color(ctx, GColorWhite);
  gpath_draw_filled(ctx, &hour_path);
  gpath_draw_filled(ctx, &min_path);

  // seconds
  GPoint center = GPoint(CENTER_X, CENTER_Y);
  GPoint sec_pos = GPoint(
    CENTER_X + SEC_RADIUS * sin_lookup(sec_angle) / ONE,
    CENTER_Y - SEC_RADIUS * cos_lookup(sec_angle) / ONE);
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_draw_line(ctx, center, sec_pos);
}

void date_layer_update_callback(Layer *layer, GContext* ctx) {
  PblTm t;
  get_time(&t);

  graphics_context_set_text_color(ctx, GColorWhite);
  
  // weekday
  string_format_time(date_buffer, DATE_BUFFER_BYTES, "%a", &t);
  graphics_text_draw(ctx,
    date_buffer,
    font,
    GRect(0, -6, 144, 32),
    GTextOverflowModeWordWrap,
    GTextAlignmentLeft,
    NULL);

  // day of month
  string_format_time(date_buffer, DATE_BUFFER_BYTES, "%e", &t);
  graphics_text_draw(ctx,
    date_buffer,
    font,
    GRect(0, -6, 144, 32),
    GTextOverflowModeWordWrap,
    GTextAlignmentRight,
    NULL);
}

void handle_second_tick(AppContextRef ctx, PebbleTickEvent *ev) {
  layer_mark_dirty(&hands_layer);
  layer_mark_dirty(&date_layer);
}

void handle_init(AppContextRef ctx) {
  (void)ctx;

  window_init(&window, "Bert ONE");
  window_stack_push(&window, true /* Animated */);
  window_set_background_color(&window, GColorBlack);

  layer_init(&date_layer, GRect(0, 144, 144, 24));
  date_layer.update_proc = &date_layer_update_callback;
  layer_add_child(&window.layer, &date_layer);

  layer_init(&background_layer, GRect(0, 0, 144, 144));
  background_layer.update_proc = &background_layer_update_callback;
  layer_add_child(&window.layer, &background_layer);

  layer_init(&hands_layer, background_layer.frame);
  hands_layer.update_proc = &hands_layer_update_callback;
  layer_add_child(&background_layer, &hands_layer);

#if DEBUG
  text_layer_init(&debug_layer, GRect(0, 0, 144, 16));
  strcpy(debug_buffer, "");
  text_layer_set_text(&debug_layer, debug_buffer);
  layer_add_child(&window.layer, &debug_layer.layer);
#endif
  
  gpath_init(&hour_path, &HOUR_POINTS);
  gpath_move_to(&hour_path, GPoint(CENTER_X, CENTER_Y));

  gpath_init(&min_path, &MIN_POINTS);
  gpath_move_to(&min_path, GPoint(CENTER_X, CENTER_Y));
  
  resource_init_current_app(&BERT_ONE_RESOURCES);
  font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_30));
}

void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,
    .tick_info = {
      .tick_handler = &handle_second_tick,
      .tick_units = SECOND_UNIT
    }
  };
  app_event_loop(params, &handlers);
}
