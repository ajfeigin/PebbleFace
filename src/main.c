#include <pebble.h>
//Create the object for the main window by pointing to it
static Window *s_main_window;
// create a text layer
static TextLayer *s_time_layer;
static TextLayer *s_date_layer;

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  // Write the current hours and minutes into a buffer
  static char s_buffer[12];
  static char sdate_buffer[30];
  //format the time
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ?
                                          "%H:%M:%S" : "%I:%M:S", tick_time);

  //format the date
    strftime(sdate_buffer, sizeof(sdate_buffer), "%a  %e-%b-%Y", tick_time);
  
  // Display this time & date on the TextLayer
  text_layer_set_text(s_time_layer, s_buffer);
  text_layer_set_text(s_date_layer, sdate_buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void basic_text(TextLayer *txt, const char * font){
  // Improve the layout to be more like a watchface
    text_layer_set_background_color(txt, GColorClear);
    text_layer_set_text_color(txt, GColorWhite);
    text_layer_set_font(txt, fonts_get_system_font(font));
    text_layer_set_text_alignment(txt, GTextAlignmentCenter);
}

static void main_window_load(Window *window) {
// Get information about the Window
  window_set_background_color(window, GColorBlack);
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Create the TextLayer with specific bounds
  s_time_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(58, 52), bounds.size.w, 100));
  s_date_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(82, 78), bounds.size.w, 100));
//set text characteristics for each text layer
  basic_text(s_time_layer,FONT_KEY_GOTHIC_28);
  basic_text(s_date_layer,FONT_KEY_GOTHIC_18);

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));
}

static void main_window_unload(Window *window) {
    // Destroy TextLayer
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_layer);
}


static void init() {
    // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
// Register with TickTimerService
tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  
  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  // Make sure the time is displayed from the start
  update_time();
}

static void deinit(){
    // Destroy Window
  window_destroy(s_main_window);
}

int main(void){
  init();
  app_event_loop();
  deinit();
}
