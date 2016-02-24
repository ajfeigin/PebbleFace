  #include <pebble.h>
//keys for dictionary in data passing b/w phone & watch
//#define KEY_TEMPERATURE 0
//#define KEY_CONDITIONS 1
//alternate to #define, better when there are lots of entries, also need to put these items in the code/project settings page in cloudpebble
enum {
  KEY_TEMPERATURE = 0,
  KEY_CONDITIONS = 1,
  KEY_NIGHTTEMP =2,
  KEY_TODAYMAX =3,
  KEY_TOMMAX = 4,
  KEY_SCALE = 5,
  KEY_TOMCONDITIONS=6,
  KEY_SCALEMIN =7 ,
  KEY_TEMPARR =8,
  KEY_ARRLEN = 9,
  KEY_TEMPOFFSET
};
//Create the object for the main window by pointing to it
static Window *s_main_window;
// create the require layers and other variables used throughout
static TextLayer *s_time_layer, *s_date_layer,*s_todayweather_layer,*s_weather_layer, *s_forecastweather_layer, *s_seconds_layer;
static TextLayer *s_batterynumber_layer;
static Layer *s_battery_layer, *s_draw_layer, *s_hands_layer;
static int s_battery_level;
static int weatherheight = 35, weatherwidth=140, hour_hand_length = 40,minute_hand_length = 60, currhour, currminute, numforecastpts =15;// 15;
static BitmapLayer  *s_bt_icon_layer,*s_weather_img_layer;
static GBitmap *s_bt_icon_bitmap, *s_weather_img_bitmap;
static GFont s_time_font;
//static const char *largefont =FONT_KEY_LECO_32_BOLD_NUMBERS;
static const char *medfont = FONT_KEY_GOTHIC_24_BOLD;
static const char *smallfont = FONT_KEY_GOTHIC_18;
static GPoint s_center;
static const bool use3day = true;

//Other than setting the font this does all the basic text setup
static void basic_text_internal(TextLayer *txt, bool left){
  text_layer_set_background_color(txt, GColorClear);
  text_layer_set_text_color(txt, GColorWhite);
  if(!left) text_layer_set_text_alignment(txt, GTextAlignmentCenter);
  if(left) text_layer_set_text_alignment(txt, GTextAlignmentLeft);
}
//basic formats for text with a system font
static void basic_text(TextLayer *txt, const char * font,bool left){
  // Improve the layout to be more like a watchface
    basic_text_internal(txt,left);
    text_layer_set_font(txt, fonts_get_system_font(font));
}
//basic text setup with a custom font
static void basic_text_cust(TextLayer *txt, GFont font, bool left){
  basic_text_internal(txt,left);
  text_layer_set_font(txt,font);
}
//call back for getting the battery level
static void battery_callback(BatteryChargeState state) {
  // Record the new battery level
  s_battery_level = state.charge_percent;
    //prepare the text version of the battery
  static char batt_buffer[20];
  if(state.is_charging){snprintf(batt_buffer, 20, "battery charging");}
  else{  snprintf(batt_buffer, 20, "battery %d%%", s_battery_level);}  
  basic_text(s_batterynumber_layer,smallfont,false);
  text_layer_set_text(s_batterynumber_layer, batt_buffer);
  
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
  currhour = tick_time->tm_hour;
  currminute = tick_time->tm_min;
//   draw_hour_hand(tick_time);  
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
  //if(tick_time->tm_sec % 30 == 0) {
    // Begin dictionary
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
  
    // Add a key-value pair
    dict_write_uint8(iter, 0, 0);
  
    // Send the message!
    app_message_outbox_send();
  }  
}
//DRAWING code derived from https://developer.getpebble.com/docs/c/Graphics/Drawing_Paths/
static GPath *s_my_path_ptr = NULL;
GPathInfo BOLT_PATH_INFO = {
  .num_points = 15,
//   //tmp Y points that get over written when the weather is set with 2 duplicates near the end to all the extra points needed for 3day data
  .points = (GPoint[]){{0,15},{7,13},{16,6},{23,5},{30,8},{60,27},{70,30},{80,28},{110,15},{117,14},{124,14},{133,15},{133,15},{133,15},{140,16}}
};

//Sets the analogue time hands
void update_analogue_time(Layer *my_layer, GContext *ctx){
      // Stroke the path:
    graphics_context_set_stroke_color(ctx, GColorYellow);
    graphics_context_set_stroke_width(ctx, 4);
  int hr = currhour;
  if(currhour>12) hr =currhour-12;
  int32_t hrangle = TRIG_MAX_ANGLE * (hr*60+currminute) /720; //use minutes and hours to angle the hour hand
  GPoint hour_hand = (GPoint){
    .x = (int)(s_center.x + hour_hand_length * sin_lookup(hrangle)/TRIG_MAX_RATIO),
    .y =  (int)(s_center.y - hour_hand_length * cos_lookup(hrangle)/TRIG_MAX_RATIO)
  };
//     printf("drawing memory used %d",(int)heap_bytes_used());
   graphics_draw_line(ctx, s_center, hour_hand);  
  int32_t minangle = TRIG_MAX_ANGLE * (currminute) /60;
  GPoint min_hand = (GPoint){
    .x = (int)(s_center.x + minute_hand_length * sin_lookup(minangle)/TRIG_MAX_RATIO),
    .y =  (int)(s_center.y - minute_hand_length * cos_lookup(minangle)/TRIG_MAX_RATIO)
  };
 graphics_context_set_stroke_color(ctx, GColorChromeYellow);
 graphics_draw_line(ctx, s_center, min_hand);   
}
//If the provider double is greater than 1 return 1, if less than 0 return 0 other return the double
double gt_one_lt_zero(double tocheck){
  double v = tocheck;
  if(tocheck>1.0) v= 1.0;
  if(tocheck<0.0) {v= 0.0; }
  return v;
}
//step from one point to the next
uint32_t inbtw_pt(double bigpiece, double smallpiece, double step){
  return bigpiece*step+smallpiece*(1-step);
}
//set the points for the weather graph based on the latest weather from the DAILY forecasts
void setweatherpts(int curr, int maxtoday, int night, int tom, int scalemax,int scalemin){
  //temp location for new points
  GPoint pts[BOLT_PATH_INFO.num_points];
  // set the tmp to be the same as the current pts
  for(uint32_t i=0;i<BOLT_PATH_INFO.num_points;i++){
    pts[i].x= BOLT_PATH_INFO.points[i].x;
    pts[i].y= BOLT_PATH_INFO.points[i].y;
  }
  //update the y values of the points based on the passed in temperatures
  double range = (double)(scalemax-scalemin);//TODO USE THIS TO ALLOW -ve temps on the chart
  double start = weatherheight * gt_one_lt_zero(1-(maxtoday+night-2*scalemin)/(2*range)); 
  double todmpt = weatherheight * gt_one_lt_zero(1-((maxtoday-scalemin)/range));
  double nightpt = weatherheight * gt_one_lt_zero(1-((night-scalemin)/range));
  double tompt = weatherheight * gt_one_lt_zero(1-((tom-scalemin)/range));
  double end = weatherheight * gt_one_lt_zero(1-(tom+night-2*scalemin)/(2*range));
  double steppct = 0.88;
  //start with the end and main (ie where an actual temp occurs) points
  pts[0].y = (uint32_t) start; // left end of the chart, halfway b/w today's max and night temp
  pts[3].y = (uint32_t) todmpt; // peak for today's max temp
  pts[6].y = (uint32_t) nightpt; // night temp
  pts[9].y = (uint32_t) tompt; // tomorrow's max
  pts[14].y =(uint32_t) end; //right end of the chart
  //now the intermediate points that step from one of the main pts to the next, with a linear piece in between
  pts[1].y = inbtw_pt(start,todmpt,steppct);
  pts[2].y = inbtw_pt(todmpt,start,steppct);
  pts[4].y = inbtw_pt(todmpt,nightpt,steppct);
  pts[5].y = inbtw_pt(nightpt,todmpt,steppct);
  pts[7].y = inbtw_pt(nightpt,tompt,steppct);
  pts[8].y = inbtw_pt(tompt,nightpt,steppct);
  pts[10].y = inbtw_pt(tompt,end,steppct);
  pts[11].y = inbtw_pt(end,tompt,steppct);
  pts[12].y = inbtw_pt(end,tompt,steppct);//duplicates to fill out additional points that are there for when 3 day weather is used
  pts[13].y = inbtw_pt(end,tompt,steppct);//duplicates to fill out additional points that are there for when 3 day weather is used
  
  for(uint32_t i=0;i<BOLT_PATH_INFO.num_points;i++){
    BOLT_PATH_INFO.points[i].x= pts[i].x;
     BOLT_PATH_INFO.points[i].y=pts[i].y;
  }
//   printf("act123 pts %d y =  %d pts 9 y = %d, pts 12 =%d", 3,(int)(BOLT_PATH_INFO.points[3].y),(int)BOLT_PATH_INFO.points[9].y,(int)BOLT_PATH_INFO.points[12].y);
//   printf( "act123 pts %d y =  %d pts 8 y = %d, pts 0 =%d", 4,(int)(BOLT_PATH_INFO.points[4].y),(int)BOLT_PATH_INFO.points[8].y,(int)BOLT_PATH_INFO.points[0].y);
}
//set the weather chart points based on the 3hourly forecasts
int calcwthval(int temp,int offset,double range,int min){
  return weatherheight*gt_one_lt_zero(1-(temp-offset-min)/range);
}
void setweatherptsfrom3hr(int currtemp,uint8_t* temps,int offset, int length, int max, int min){
  double range =(double) max-min;
  int ptseparation =  (int)(weatherwidth/(numforecastpts-1)) ;//step between points on the new chart
  GPoint pts[BOLT_PATH_INFO.num_points];
//   printf("int pts %d ",(int)(BOLT_PATH_INFO.num_points));
  //use the current temp for the left most point (ignoring the fact that the time between that point and the next won't be 3hrs as it is for every pair in the forecasts array)
  pts[0].x = 0;
  pts[0].y=calcwthval(currtemp,0,range,min);
//   printf("currtemp %d, y=%d", currtemp,pts[0].x);
//   printf("range = %d, height = %d",(int)range,weatherheight);
    for(uint32_t i=1;i<BOLT_PATH_INFO.num_points;i++){
      pts[i].x =  ptseparation*i;
      //start from the 0th entry in the temps array hence the (i-1) for that reference 
      pts[i].y = weatherheight *gt_one_lt_zero(1-(temps[i-1]-offset-min)/range);
//       printf("i = %d, x=%d, y=%d, min=%d, max=%d, temp=%d, ptsep=%d, chartw=%d",(int)i,(int)(pts[i].x),(int)(pts[i].y),min,max,(int)temps[i-1],ptseparation,weatherwidth);
    }
//     printf("abc %d",(int)BOLT_PATH_INFO.num_points);
  for(uint32_t i=0;i<(uint32_t)numforecastpts;i++){
//     printf("numpts %d, static pts = %d, i=%d",(int)BOLT_PATH_INFO.num_points,numforecastpts,(int)i);
    BOLT_PATH_INFO.points[i].x= pts[i].x;
    BOLT_PATH_INFO.points[i].y= pts[i].y;
  }
}
// .update_proc of my_layer:
void weather_layer_update_proc(Layer *my_layer, GContext *ctx) {
    // Stroke the path:
    graphics_context_set_stroke_color(ctx, GColorWhite);
    graphics_context_set_stroke_width(ctx, 2);
//      printf("drawing memory used %d",(int)heap_bytes_used());
    gpath_draw_outline_open(ctx, s_my_path_ptr);
}
//return the max of 2 integers
int intmax(int a, int b){
  int res;
  res = a>b ? a : b ;
  return res;
}
//return the minimum of 2 integers
int intmin(int a,int b){
  return -intmax(-a,-b);
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Store incoming weather info from JS
  static char temperature_buffer[8],   maxtemperature_buffer[8], conditions_buffer[32], weather_layer_buffer[50], weathertoday_layer_buffer[50], nightfull_buffer[50];

  // Read tuples for data
  Tuple *temp_tuple = dict_find(iterator, KEY_TEMPERATURE);
  Tuple *conditions_tuple = dict_find(iterator, KEY_CONDITIONS);
  Tuple *night_tuple = dict_find(iterator, KEY_NIGHTTEMP);
  Tuple *max_tuple = dict_find(iterator, KEY_TODAYMAX);
  Tuple *tommax_tuple = dict_find(iterator, KEY_TOMMAX);  
  Tuple *scalemax_tuple = dict_find(iterator, KEY_SCALE); 
  Tuple *scalemin_tuple = dict_find(iterator, KEY_SCALEMIN); 
  Tuple *forecastarr = dict_find(iterator,KEY_TEMPARR);
  Tuple *arrlen = dict_find(iterator, KEY_ARRLEN);
  Tuple *tempoffset = dict_find(iterator,KEY_TEMPOFFSET);
  int offset= tempoffset->value->int32;
  static uint8_t *forecast3hrs;
  forecast3hrs= forecastarr->value->data;
  //how many steps of 3 hours left today prior to midnight (e.g. if it's currently 5am get 7 ) rounded up if it's not a whole number. Adding 0.7 does the rounding b/c the number goes in 1/3s
  int hrstoday = (int)((24.0-currhour)/3+0.7); 
  int fcastmaxtoday = -100; // starting value
  //find the maximum temp for the remainder of today, then tomorrow and the overnight min
  for (int i=0;i<hrstoday;i++){
    int t = forecast3hrs[i]-offset;
    fcastmaxtoday = intmax(t, fcastmaxtoday );
  }
  int fcastmaxtom = -100;
  int tomsteps = intmin(numforecastpts,hrstoday+8);
  for (int i=hrstoday ; i < tomsteps ; i++){ //starting from ~midnight and go through the rest of the data imported
    int t = forecast3hrs[i]-offset;
    fcastmaxtom = intmax(t,fcastmaxtom);
  }
  int fcastmin = 100;
  for (int i=hrstoday -1; i <hrstoday +3 ; i++){ //starting from around 9pm and go through another 12 hrs
    if(i>-1){
      int t = forecast3hrs[i]-offset;
      fcastmin =intmin( t, fcastmin);
  }  }
//   printf("3hr blocks:%d, tmax: %d, min: %d, tom: %d",hrstoday,fcastmaxtoday,fcastmin,fcastmaxtom);
 // If all data is available, use it -- not actually checking all the data anymore (lazy)
  if(temp_tuple && conditions_tuple && night_tuple) {
    int currtemp = (int)temp_tuple->value->int32;
    int maxtodaytemp = intmax(fcastmaxtoday,(int)max_tuple->value->int32);
    maxtodaytemp = intmax(maxtodaytemp,currtemp);
    int tonighttemp = intmin(fcastmin,(int)night_tuple->value->int32);
    int tomtemp = intmax(fcastmaxtom,(int)tommax_tuple->value->int32);
    snprintf(temperature_buffer, sizeof(temperature_buffer), "%dC", currtemp);
    snprintf(maxtemperature_buffer, sizeof(maxtemperature_buffer), "%dC", maxtodaytemp);
    snprintf(conditions_buffer, sizeof(conditions_buffer), "%s", conditions_tuple->value->cstring);
    // Assemble full string and display
    //    snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s, %s, %s %s", temperature_buffer, conditions_buffer,"max:",maxtemperature_buffer);
    snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "max:%dC     tmw:%dC", maxtodaytemp,tomtemp);
    text_layer_set_text(s_weather_layer, weather_layer_buffer);
    snprintf(weathertoday_layer_buffer,sizeof(weather_layer_buffer),"Now: %dC %s",currtemp,conditions_buffer);
    text_layer_set_text(s_todayweather_layer, weathertoday_layer_buffer);
    //prep the night time data
    snprintf(nightfull_buffer, sizeof(nightfull_buffer), "tonight: %dC", tonighttemp);
    text_layer_set_text(s_forecastweather_layer,nightfull_buffer);
    int scalemax = scalemax_tuple->value->int32;
    int scalemin = scalemin_tuple->value->int32;
    if(use3day) setweatherptsfrom3hr(currtemp,forecast3hrs,offset, arrlen->value->int32,scalemax,scalemin );
      else setweatherpts(currtemp,maxtodaytemp,tonighttemp,tomtemp,scalemax, scalemin); 
    layer_mark_dirty(s_draw_layer);
  }else {printf("err1");}
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}
static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}
static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}
static void poistion_items(GRect bounds){
   static int timeY = 30, weatherchartY = 125;
  // Create the TextLayer with specific bounds
  s_time_layer = text_layer_create(
      GRect(15, PBL_IF_ROUND_ELSE(timeY, timeY-5), bounds.size.w, 100));
  s_seconds_layer = text_layer_create(
      GRect(60, PBL_IF_ROUND_ELSE(timeY+5, timeY), bounds.size.w, 75));
  s_date_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(timeY+45, timeY+40), bounds.size.w, 100));
  s_todayweather_layer = text_layer_create(
    GRect(0, weatherchartY-35, bounds.size.w, 25));
  s_weather_layer = text_layer_create(
    GRect(10,weatherchartY-20, bounds.size.w, 25));
  s_forecastweather_layer = text_layer_create(
    GRect(5, PBL_IF_ROUND_ELSE(155, 150), bounds.size.w, 100));
  s_batterynumber_layer = text_layer_create(
    GRect(0, PBL_IF_ROUND_ELSE(5, 0), bounds.size.w, 100)); 
  // Create battery meter Layer
  s_battery_layer = layer_create(GRect(25, 25, 115, 2));
  //create the weather chart layer
  s_draw_layer = layer_create(GRect(0,weatherchartY,140,weatherheight+1));
  // Create the BitmapLayer to display the GBitmap
  s_bt_icon_layer = bitmap_layer_create(GRect(120, PBL_IF_ROUND_ELSE(65,60), 15, 15));
  //create the analogue time layer
  s_hands_layer = layer_create(GRect(0,0,140,160));
}

static void main_window_load(Window *window) {
// Get information about the Window
  window_set_background_color(window, GColorBlack);
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  poistion_items(bounds);

  //load custom font(s)
  s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_CONDENSED_46));

  // Apply to TextLayers
  //set text characteristics for each text layer
  basic_text_cust(s_time_layer,s_time_font,false); 
  //basic text function sets center alignment - for the time try it on the left...
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentLeft);
  basic_text(s_seconds_layer,medfont,false);
  basic_text(s_date_layer,medfont,false);
  basic_text(s_weather_layer,smallfont,true);  
  basic_text(s_todayweather_layer,smallfont,false);
  basic_text(s_forecastweather_layer,smallfont,true);
  // text for weather while it's loaded from the phone
  text_layer_set_text(s_todayweather_layer, "Loading weather...");
  // Add textlayers as a child layer to the Window's root layer
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
//   layer_add_child(window_layer, text_layer_get_layer(s_seconds_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_weather_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_todayweather_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_forecastweather_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_batterynumber_layer));

  //s_battery_layer = layer_create(GRect(14, 54, 115, 2));
  layer_set_update_proc(s_battery_layer, battery_update_proc);
  
  // Add battery meter layer to Window
  layer_add_child(window_get_root_layer(window), s_battery_layer);

  //Add the weather chart drawing layer
  layer_set_update_proc(s_draw_layer,weather_layer_update_proc);
  layer_add_child(window_layer,s_draw_layer);
  //analogue partly derived from https://github.com/pebble-examples/ks-clock-face/blob/master/src/ks-clock-face.c
  GRect window_bounds = layer_get_bounds(window_layer);
  s_center = grect_center_point(&window_bounds);
  //add the analogue time layer
//   layer_set_update_proc(s_hands_layer,update_analogue_time);
//   layer_add_child(window_layer,s_hands_layer);

  // Create the Bluetooth icon GBitmap
  s_bt_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BT_ICON_SMALL);
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
  text_layer_destroy(s_todayweather_layer);
  text_layer_destroy(s_forecastweather_layer);
  text_layer_destroy(s_batterynumber_layer);
  layer_destroy(s_battery_layer);
  layer_destroy(s_draw_layer);
  layer_destroy(s_hands_layer);  
  gbitmap_destroy(s_bt_icon_bitmap);
  bitmap_layer_destroy(s_bt_icon_layer);
  gbitmap_destroy(s_weather_img_bitmap);
  bitmap_layer_destroy(s_weather_img_layer);
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
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
//   tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  // Open AppMessage
//    app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
  app_message_open(500, 500);
  
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
    s_my_path_ptr = gpath_create(&BOLT_PATH_INFO);
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