  #include <pebble.h>
//keys for dictionary in data passing b/w phone & watch
//#define KEY_TEMPERATURE 0
//#define KEY_CONDITIONS 1
//alternate to #define, better when there are lots of entries
enum {
  KEY_TEMPERATURE = 0,
  KEY_CONDITIONS = 1,
  KEY_NIGHTTEMP =2,
  KEY_TODAYMAX =3,
  KEY_TOMMAX
};
//Create the object for the main window by pointing to it
static Window *s_main_window;
// create a text layer
static TextLayer *s_time_layer, *s_date_layer,*s_weather_layer, *s_forecastweather_layer, *s_seconds_layer;
static TextLayer *s_batterynumber_layer;
static Layer *s_battery_layer;
static int s_battery_level;
static BitmapLayer  *s_bt_icon_layer;
static GBitmap *s_bt_icon_bitmap;
static GFont s_time_font;
//static const char *largefont =FONT_KEY_LECO_32_BOLD_NUMBERS;
static const char *medfont = FONT_KEY_GOTHIC_24_BOLD;
//static const char *medfont = FONT_KEY_GOTHIC_18_BOLD;
static const char *smallfont = FONT_KEY_GOTHIC_18;

//Other than setting the font this does all the basic text setup
static void basic_text_internal(TextLayer *txt){
  text_layer_set_background_color(txt, GColorClear);
  text_layer_set_text_color(txt, GColorWhite);
  text_layer_set_text_alignment(txt, GTextAlignmentCenter);
}

//basic formats for text with a system font
static void basic_text(TextLayer *txt, const char * font){
  // Improve the layout to be more like a watchface
    basic_text_internal(txt);
    text_layer_set_font(txt, fonts_get_system_font(font));
    
}
//basic text setup with a custom font
static void basic_text_cust(TextLayer *txt, GFont font){
  basic_text_internal(txt);
  text_layer_set_font(txt,font);
}

//call back for getting the battery level
static void battery_callback(BatteryChargeState state) {
  // Record the new battery level
  s_battery_level = state.charge_percent;
  // Update meter
layer_mark_dirty(s_battery_layer);
}
//Battery updater
static void battery_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  // Find the width of the bar
  int width = (int)(float)(((float)s_battery_level / 100.0F) * 114.0F);

  // Draw the background
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // Draw the bar
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(0, 0, width, bounds.size.h), 0, GCornerNone);

  //prepare the text version
  static char batt_buffer[20];
  snprintf(batt_buffer, 20, "battery %d%%", s_battery_level);
  basic_text(s_batterynumber_layer,smallfont);
  text_layer_set_text(s_batterynumber_layer, batt_buffer);
}
static void bluetooth_callback(bool connected) {
  // Show icon if disconnected
  layer_set_hidden(bitmap_layer_get_layer(s_bt_icon_layer), connected);

  if(!connected) {
    // Issue a vibrating alert
    vibes_double_pulse();
  }
}
static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  // Write the current hours and minutes into a buffer
  static char time_buffer[20];
  static char sec_buffer[20];
  static char sdate_buffer[30];
  
    //format the time
  strftime(time_buffer, sizeof(time_buffer), clock_is_24h_style() ?"%H:%M" :"%l:%M", tick_time);
  strftime(sec_buffer, sizeof(sec_buffer),"%S", tick_time);

  //format the date
    strftime(sdate_buffer, sizeof(sdate_buffer), "%a  %e-%b", tick_time);
  //determine if it's AM or PM
  //char *AMPM = tick_time->tm_hour>11 ? " PM" : " AM";
  // Display this time & date on the TextLayer, include AM or PM if it's not in 24hr time
  //text_layer_set_text(s_time_layer,clock_is_24h_style() ?  s_buffer : strcat(s_buffer,AMPM));
  text_layer_set_text(s_time_layer,time_buffer);
  text_layer_set_text(s_seconds_layer,sec_buffer);
  text_layer_set_text(s_date_layer, sdate_buffer);  
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
  
  // Get weather update every 30 minutes
if(tick_time->tm_min % 30 == 0) {
  // Begin dictionary
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);

  // Add a key-value pair
  dict_write_uint8(iter, 0, 0);

  // Send the message!
  app_message_outbox_send();
}
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
// Store incoming weather info from JS
static char temperature_buffer[8];
static char maxtemperature_buffer[8];
static char conditions_buffer[32];
static char weather_layer_buffer[50];
static char nightfull_buffer[50];

// Read tuples for data
Tuple *temp_tuple = dict_find(iterator, KEY_TEMPERATURE);
Tuple *conditions_tuple = dict_find(iterator, KEY_CONDITIONS);
Tuple *night_tuple = dict_find(iterator, KEY_NIGHTTEMP);
Tuple *max_tuple = dict_find(iterator, KEY_TODAYMAX);
Tuple *tommax_tuple = dict_find(iterator, KEY_TOMMAX);  

  // If all data is available, use it
if(temp_tuple && conditions_tuple && night_tuple) {
  snprintf(temperature_buffer, sizeof(temperature_buffer), "%dC", (int)temp_tuple->value->int32);
  snprintf(maxtemperature_buffer, sizeof(maxtemperature_buffer), "%dC", (int)max_tuple->value->int32);
  snprintf(conditions_buffer, sizeof(conditions_buffer), "%s", conditions_tuple->value->cstring);
  // Assemble full string and display
  snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s, %s, %s %s", temperature_buffer, conditions_buffer,"max:",maxtemperature_buffer);
  text_layer_set_text(s_weather_layer, weather_layer_buffer);
  
  //prep the night time data
  snprintf(nightfull_buffer, sizeof(nightfull_buffer), "%s %dC %s %dC","tonight:" ,(int)night_tuple->value->int32, "tom:",(int)tommax_tuple->value->int32);
  text_layer_set_text(s_forecastweather_layer,nightfull_buffer);
  }else {printf("err1");}
}
//error handling (I think)
static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void main_window_load(Window *window) {
// Get information about the Window
  window_set_background_color(window, GColorBlack);
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  static int timeX = 35;
  // Create the TextLayer with specific bounds
  s_time_layer = text_layer_create(
      GRect(8, PBL_IF_ROUND_ELSE(timeX, timeX-5), bounds.size.w, 100));
  s_seconds_layer = text_layer_create(
      GRect(55, PBL_IF_ROUND_ELSE(timeX+5, timeX), bounds.size.w, 75));
  s_date_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(82, 76), bounds.size.w, 100));
  s_weather_layer = text_layer_create(
    GRect(0, PBL_IF_ROUND_ELSE(125, 120), bounds.size.w, 25));
  s_forecastweather_layer = text_layer_create(
    GRect(0, PBL_IF_ROUND_ELSE(140, 135), bounds.size.w, 100));
  s_batterynumber_layer = text_layer_create(
    GRect(0, PBL_IF_ROUND_ELSE(5, 0), bounds.size.w, 100));

  //load custom font(s)
  // Create GFont
s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_CONDENSED_46));

// Apply to TextLayer

//set text characteristics for each text layer
  basic_text_cust(s_time_layer,s_time_font); 
  //basic text function sets center alignment - for the time try it on the left...
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentLeft);
  basic_text(s_seconds_layer,medfont);
  basic_text(s_date_layer,medfont);
  basic_text(s_weather_layer,smallfont);
  basic_text(s_forecastweather_layer,smallfont);
  //test text for weather
  text_layer_set_text(s_weather_layer, "Loading...");
  text_layer_set_text(s_forecastweather_layer, "Loading...");
  // Add textlayers as a child layer to the Window's root layer
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_seconds_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_weather_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_forecastweather_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_batterynumber_layer));

  // Create battery meter Layer
s_battery_layer = layer_create(GRect(25, 25, 115, 2));
  //s_battery_layer = layer_create(GRect(14, 54, 115, 2));
layer_set_update_proc(s_battery_layer, battery_update_proc);

// Add battery meter layer to Window
layer_add_child(window_get_root_layer(window), s_battery_layer);
  
  // Create the Bluetooth icon GBitmap
s_bt_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BT_ICON_SMALL);

// Create the BitmapLayer to display the GBitmap
s_bt_icon_layer = bitmap_layer_create(GRect(75, 100, 30, 30));
bitmap_layer_set_bitmap(s_bt_icon_layer, s_bt_icon_bitmap);
layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_bt_icon_layer));
  
  // Show the correct state of the BT connection from the start
bluetooth_callback(connection_service_peek_pebble_app_connection());
}

static void main_window_unload(Window *window) {
  // Destroy Custom Fonts
  fonts_unload_custom_font(s_time_font);
  
  // Destroy TextLayer & other layers
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_seconds_layer);
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_weather_layer);
  text_layer_destroy(s_forecastweather_layer);
  text_layer_destroy(s_batterynumber_layer);
  layer_destroy(s_battery_layer);
  gbitmap_destroy(s_bt_icon_bitmap);
  bitmap_layer_destroy(s_bt_icon_layer);
}


static void init() {
  // Register for battery level updates
battery_state_service_subscribe(battery_callback);
    // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  // Register with TickTimerService
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  // Open AppMessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
  
  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  // Make sure the time is displayed from the start
  update_time();
  // Ensure battery level is displayed from the start
battery_callback(battery_state_service_peek());
  
  // Register for Bluetooth connection updates
connection_service_subscribe((ConnectionHandlers) {
  .pebble_app_connection_handler = bluetooth_callback
});
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