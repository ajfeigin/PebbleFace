#include <pebble.h>
//Create the object for the main window by pointing to it
static Window *s_main_window;
// create a text layer
static TextLayer *s_time_layer;
static void main_window_load(Window *window) {

}

static void main_window_unload(Window *window) {

}

static void init() {
    // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
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
