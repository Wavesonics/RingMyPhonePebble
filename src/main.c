#include <pebble.h>

const uint32_t  STATUS_RESET_TIME = 3000;

static Window* window;

TextLayer* statusTextLayer;

ActionBarLayer* actionBar;
GBitmap* bitmapRing;
GBitmap* bitmapSilence;

AppTimer* statusResetTimer;

const char * const WINDOW_TITLE = "Ring My Phone";
const char * const STATUS_DONE = "Done";

enum {
  CMD_KEY = 0x1, // TUPLE_INTEGER
};

enum {
  CMD_START = 0x01,
  CMD_STOP = 0x02,
};

static void send_cmd(uint8_t cmd);
void start_reset_timer();
void reset_timer_callback( void* data );

// Status text helpers
void set_status_ready()
{
	const char * const STATUS_READY = "Ready";
#if defined(PBL_COLOR)
	text_layer_set_text_color(statusTextLayer, GColorLimerick);
#endif
	text_layer_set_text(statusTextLayer, STATUS_READY);
}

void set_status_ringing()
{
	const char * const STATUS_RINGING = "Ringing";
#if defined(PBL_COLOR)
	text_layer_set_text_color(statusTextLayer, GColorOrange);
#endif
	text_layer_set_text(statusTextLayer, STATUS_RINGING);
}

void set_status_silencing()
{
	const char * const STATUS_SILENCING = "Silencing";
#if defined(PBL_COLOR)
	text_layer_set_text_color(statusTextLayer, GColorLiberty);
#endif
	text_layer_set_text(statusTextLayer, STATUS_SILENCING);
}

void set_status_failed()
{
	const char * const STATUS_FAILED = "Failed :(";
#if defined(PBL_COLOR)
	text_layer_set_text_color(statusTextLayer, GColorRed);
#endif
	text_layer_set_text(statusTextLayer, STATUS_FAILED);
}

// Button handlers
void up_single_click_handler( ClickRecognizerRef recognizer, void *context )
{
	//APP_LOG(APP_LOG_LEVEL_INFO, "Send ring command");
	
	set_status_ringing();
	send_cmd( CMD_START );
}

void down_single_click_handler( ClickRecognizerRef recognizer, void *context )
{
	//APP_LOG(APP_LOG_LEVEL_INFO, "Send silence command");
	
	set_status_silencing();
	send_cmd( CMD_STOP );
}

// App message callbacks
void out_sent_handler(DictionaryIterator *sent, void *context)
{
	//APP_LOG(APP_LOG_LEVEL_INFO, "command sent");
	
	vibes_short_pulse();
	start_reset_timer();
}

void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context)
{
	APP_LOG(APP_LOG_LEVEL_WARNING, "command send failed");
	
	set_status_failed();
	start_reset_timer();
}

void in_received_handler(DictionaryIterator *received, void *context)
{
	// incoming message received
}

void in_dropped_handler(AppMessageResult reason, void *context)
{
	// incoming message dropped
}

void init_app_message()
{
	app_message_register_inbox_received(in_received_handler);
	app_message_register_inbox_dropped(in_dropped_handler);
	app_message_register_outbox_sent(out_sent_handler);
	app_message_register_outbox_failed(out_failed_handler);

	const uint32_t inbound_size = 128;
	const uint32_t outbound_size = 128;
	app_message_open(inbound_size, outbound_size);
}

void cancel_reset_timer()
{
	if( statusResetTimer != NULL )
	{
		app_timer_cancel( statusResetTimer );
		statusResetTimer = NULL;
	}
}

void start_reset_timer()
{
	cancel_reset_timer();
	statusResetTimer = app_timer_register( STATUS_RESET_TIME, &reset_timer_callback, NULL );
}

static void send_cmd(uint8_t cmd)
{
	start_reset_timer();

	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	
	Tuplet value = TupletInteger(CMD_KEY, cmd);
	dict_write_tuplet(iter, &value);
	
	app_message_outbox_send();
}

void reset_timer_callback( void* data )
{
	set_status_ready();
}

void click_config_provider( void *context )
{
	window_single_click_subscribe( BUTTON_ID_DOWN, down_single_click_handler );
	window_single_click_subscribe( BUTTON_ID_UP, up_single_click_handler );
}

void window_load( Window *window )
{
	// Initialize the action bar:
	actionBar = action_bar_layer_create();
	// Associate the action bar with the window:
	action_bar_layer_add_to_window( actionBar, window );
	// Set the click config provider:
	action_bar_layer_set_click_config_provider( actionBar, click_config_provider );
	
	// Set the icons:
	bitmapRing = gbitmap_create_with_resource( RESOURCE_ID_IMAGE_RING_ICON );
	bitmapSilence = gbitmap_create_with_resource( RESOURCE_ID_IMAGE_SILENCE_ICON );
	
	// Set the icons:
	action_bar_layer_set_icon( actionBar, BUTTON_ID_UP, bitmapRing );
	action_bar_layer_set_icon( actionBar, BUTTON_ID_DOWN, bitmapSilence );
}

void window_unload(Window *window)
{
	text_layer_destroy( statusTextLayer );
	gbitmap_destroy( bitmapRing );
	gbitmap_destroy( bitmapSilence );
	window_destroy( window );
}

void handle_init()
{
	window = window_create();
	
	Layer *window_layer = window_get_root_layer(window);
	GRect windowFrame = layer_get_frame(window_layer);
	
	GFont font = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
	GSize textSize = graphics_text_layout_get_content_size("Ready", font, windowFrame, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
	textSize.h += 5;
	
	GRect textFrame = layer_get_bounds(window_layer);
	textFrame.origin.x = 6;
	textFrame.origin.y =  (windowFrame.size.h/2) - (textSize.h/2);
	textFrame.size.w = windowFrame.size.w;
	textFrame.size.h = textSize.h;
	
	statusTextLayer = text_layer_create( textFrame );
	
	text_layer_set_text(statusTextLayer, "---");
	text_layer_set_text_alignment(statusTextLayer, GTextAlignmentLeft);
	text_layer_set_font(statusTextLayer, font);
	layer_add_child( window_layer, text_layer_get_layer( statusTextLayer ) );
	
	set_status_ready();
	
	init_app_message();
	
	WindowHandlers handlers = {
		.load = window_load,
		.unload = window_unload
	};
	
	window_set_window_handlers(window, handlers);
	
	window_stack_push(window, true /* Animated */);
}

void handle_deinit()
{
	cancel_reset_timer();
}

int main(void)
{
	APP_LOG(APP_LOG_LEVEL_INFO, "RingMyPhone app started");
	
	handle_init();
	app_event_loop();
	handle_deinit();
	
	APP_LOG(APP_LOG_LEVEL_INFO, "RingMyPhone app exiting");
	
	return 0;
}
