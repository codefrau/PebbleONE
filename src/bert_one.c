#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"

#define MY_UUID { 0xF6, 0xC1, 0xAD, 0x30, 0x2E, 0x14, 0x42, 0x7A, 0xB9, 0x59, 0x94, 0x42, 0xF3, 0xA4, 0x69, 0x49 }
PBL_APP_INFO(MY_UUID,
             "Bert ONE", "Bert Freudenberg",
             1, 1, /* App version */
             RESOURCE_ID_IMAGE_MENU_ICON,
             APP_INFO_WATCH_FACE);

#define SECONDS    0
#define SCREENSHOT 0
#define DEBUG      0

#if SECONDS
#define UPDATE_UNIT SECOND_UNIT
#else
#define UPDATE_UNIT MINUTE_UNIT
#endif

#define ONE        TRIG_MAX_RATIO
#define THREESIXTY TRIG_MAX_ANGLE

#define CENTER_X    71
#define CENTER_Y    71
#define LOGO_X      72
#define LOGO_Y      35
#define DOTS_RADIUS 67
#define DOTS_SIZE    4
#define HOUR_RADIUS 40
#define MIN_RADIUS  60
#define SEC_RADIUS  62

Window window;
Layer background_layer;
Layer hands_layer;
Layer date_layer;

BmpContainer logo;

GFont font;
#define DATE_BUFFER_BYTES 32
char date_buffer[DATE_BUFFER_BYTES];

#if DEBUG
TextLayer debug_layer;
#define DEBUG_BUFFER_BYTES 32
char debug_buffer[DEBUG_BUFFER_BYTES];
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
GPath hour_path;

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
GPath min_path;

#if SECONDS
const GPathInfo SEC_POINTS = {
  4,
  (GPoint []) {
    { 2,  0},
    { 2,-61},
    {-2,-61},
    {-2,  0},
  }
};
GPath sec_path;
#endif

void background_layer_update_callback(Layer *layer, GContext* ctx) {
	graphics_context_set_fill_color(ctx, GColorWhite);
  for (int32_t angle = 0; angle < THREESIXTY; angle += THREESIXTY / 12) {
    GPoint pos = GPoint(
      CENTER_X + DOTS_RADIUS * cos_lookup(angle) / ONE,
      CENTER_Y + DOTS_RADIUS * sin_lookup(angle) / ONE);
    graphics_fill_circle(ctx, pos, DOTS_SIZE);
  }
  GRect box = layer_get_frame(&logo.layer.layer);
  box.origin.x = LOGO_X - box.size.w / 2; 
  box.origin.y = LOGO_Y - box.size.h / 2;
  graphics_draw_bitmap_in_rect(ctx, &logo.bmp, box);
}

void hands_layer_update_callback(Layer *layer, GContext* ctx) {
  PblTm t;
  get_time(&t);

#if SCREENSHOT
  t.tm_hour = 10;
  t.tm_min = 9;
  t.tm_sec = 36;
#endif

#if DEBUG
  string_format_time(debug_buffer, DEBUG_BUFFER_BYTES, "%d.%m.%Y %H:%M:%S", &t);
  text_layer_set_text(&debug_layer, debug_buffer);
#endif

  GPoint center = GPoint(CENTER_X, CENTER_Y);

  // hours and minutes
  int32_t hour_angle = THREESIXTY * (t.tm_hour * 5 + t.tm_min / 12) / 60;
  int32_t min_angle = THREESIXTY * t.tm_min / 60;
  gpath_rotate_to(&hour_path, hour_angle);
  gpath_rotate_to(&min_path, min_angle);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_context_set_stroke_color(ctx, GColorBlack);
  gpath_draw_filled(ctx, &hour_path);
  gpath_draw_outline(ctx, &hour_path);
  graphics_draw_circle(ctx, center, DOTS_SIZE+4);
  gpath_draw_filled(ctx, &min_path);
  gpath_draw_outline(ctx, &min_path);
  graphics_fill_circle(ctx, center, DOTS_SIZE+3);

#if SECONDS
  // seconds
  int32_t sec_angle = THREESIXTY * t.tm_sec / 60;
  GPoint sec_pos = GPoint(
    CENTER_X + SEC_RADIUS * sin_lookup(sec_angle) / ONE,
    CENTER_Y - SEC_RADIUS * cos_lookup(sec_angle) / ONE);
  graphics_context_set_fill_color(ctx, GColorBlack);
  gpath_rotate_to(&sec_path, sec_angle);
  gpath_draw_filled(ctx, &sec_path);
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_context_set_compositing_mode(ctx, GCompOpAssignInverted);
  graphics_draw_line(ctx, center, sec_pos);
#endif

  // center dot
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, center, DOTS_SIZE);
}

void date_layer_update_callback(Layer *layer, GContext* ctx) {
  PblTm t;
  get_time(&t);

#if SCREENSHOT
  t.tm_wday = 0;
  t.tm_mday = 25;
#endif

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

void handle_tick(AppContextRef ctx, PebbleTickEvent *ev) {
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
#if SECONDS
  gpath_init(&sec_path, &SEC_POINTS);
  gpath_move_to(&sec_path, GPoint(CENTER_X, CENTER_Y));
#endif  
  resource_init_current_app(&BERT_ONE_RESOURCES);
  font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_30));
  bmp_init_container(RESOURCE_ID_IMAGE_LOGO, &logo);
}

void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,
    .tick_info = {
      .tick_handler = &handle_tick,
      .tick_units = UPDATE_UNIT
    }
  };
  app_event_loop(params, &handlers);
}
