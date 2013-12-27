#include <pebble.h>
#include <time.h>

#define SCREENSHOT 0
#define DEBUG      1

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

static Window *window;
static Layer *background_layer;
static Layer *hands_layer;
static Layer *date_layer;

static GBitmap *logo;
static BitmapLayer *logo_layer;

static GBitmap *battery_empty;
static GBitmap *battery_low;
static GBitmap *battery_charging;
static GBitmap *battery_charged;
static GBitmap *battery_full;
static BitmapLayer *battery_layer;

static struct tm *now = NULL;
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
}

void handle_tick(struct tm *tick_time, TimeUnits units_changed) {
  now = tick_time;
  layer_mark_dirty(hands_layer);
  layer_mark_dirty(date_layer);
}

void handle_battery(BatteryChargeState charge_state) {
#if DEBUG
  //strftime(debug_buffer, DEBUG_BUFFER_BYTES, "%d.%m.%Y %H:%M:%S", now);
  snprintf(debug_buffer, DEBUG_BUFFER_BYTES, "%s%d%%",  charge_state.is_charging ? "+" : "", charge_state.charge_percent);
  text_layer_set_text(debug_layer, debug_buffer);
#endif
  bitmap_layer_set_bitmap	(battery_layer,
    charge_state.charge_percent == 100 ? battery_full :
    charge_state.is_charging
      ? (charge_state.charge_percent < 50 ? battery_charging : battery_charged)
      : (charge_state.charge_percent < 10 ? battery_empty : battery_low));
  bool battery_is_low = charge_state.charge_percent <= 20;
  bool preserve_battery = battery_is_low && !charge_state.is_charging;
  if (hide_seconds != preserve_battery) {
    hide_seconds = preserve_battery;
    tick_timer_service_unsubscribe();
    tick_timer_service_subscribe(hide_seconds ? MINUTE_UNIT : SECOND_UNIT, &handle_tick);
  }
  layer_set_hidden(bitmap_layer_get_layer(battery_layer), !(charge_state.is_charging || battery_is_low));
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
  GRect frame = (GRect) {
    .origin = (GPoint) {
      .x = LOGO_X - logo->bounds.size.w / 2,
      .y =  LOGO_Y - logo->bounds.size.h / 2
    },
    .size = logo->bounds.size
  };
  logo_layer = bitmap_layer_create(frame);
  bitmap_layer_set_bitmap	(logo_layer, logo);
  layer_add_child(background_layer, bitmap_layer_get_layer(logo_layer));

  battery_empty    = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY_EMPTY);  
  battery_low      = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY_LOW);  
  battery_charging = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY_CHARGING);  
  battery_charged  = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY_CHARGED);  
  battery_full     = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY_FULL);  
  battery_layer = bitmap_layer_create(GRect(144-16, 0, 16, 10));
  bitmap_layer_set_bitmap	(battery_layer, battery_full);
  layer_add_child(background_layer, bitmap_layer_get_layer(battery_layer));

  hands_layer = layer_create(layer_get_frame(background_layer));
  layer_set_update_proc(hands_layer, &hands_layer_update_callback);
  layer_add_child(background_layer, hands_layer);

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

  tick_timer_service_subscribe(hide_seconds ? MINUTE_UNIT : SECOND_UNIT, &handle_tick);
  battery_state_service_subscribe(&handle_battery);
  handle_battery(battery_state_service_peek());
}

void handle_deinit() {
  battery_state_service_unsubscribe();
  tick_timer_service_unsubscribe();
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
  gbitmap_destroy(battery_empty);
  gbitmap_destroy(battery_low);
  gbitmap_destroy(battery_charging);
  gbitmap_destroy(battery_charged);
  gbitmap_destroy(battery_full);
  layer_destroy(background_layer);
  layer_destroy(date_layer);
  window_destroy(window);
}

int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}
