
#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"


#define MY_UUID { 0xD1, 0xAD, 0x93, 0x6B, 0x17, 0x68, 0x46, 0x0E, 0x88, 0x5A, 0xD7, 0x96, 0x89, 0x6A, 0x38, 0xC2 }
PBL_APP_INFO(MY_UUID, "Ring My Phone", "Dark Rock Studios", 1 , 0/* App version */, RESOURCE_ID_IMAGE_MENU_ICON, APP_INFO_STANDARD_APP);

Window window;

TextLayer ringTextLayer;
TextLayer statusTextLayer;
TextLayer silenceTextLayer;

static bool callbacks_registered = false;
static AppMessageCallbacksNode app_callbacks;

enum {
  CMD_KEY = 0x0, // TUPLE_INTEGER
};

enum {
  CMD_START = 0x01,
  CMD_STOP = 0x02,
};

static void send_cmd(uint8_t cmd);

// Modify these common button handlers

void up_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
  (void)recognizer;
  (void)window;

	text_layer_set_text(&statusTextLayer, "Ringing");
	send_cmd( CMD_START );
}


void down_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
  (void)recognizer;
  (void)window;

	text_layer_set_text(&statusTextLayer, "Silencing");	
	send_cmd( CMD_STOP );
}


void select_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
  (void)recognizer;
  (void)window;

  
}


void select_long_click_handler(ClickRecognizerRef recognizer, Window *window) {
  (void)recognizer;
  (void)window;

}


// This usually won't need to be modified

void click_config_provider(ClickConfig **config, Window *window) {
  (void)window;

  config[BUTTON_ID_SELECT]->click.handler = (ClickHandler) select_single_click_handler;

  config[BUTTON_ID_SELECT]->long_click.handler = (ClickHandler) select_long_click_handler;

  config[BUTTON_ID_UP]->click.handler = (ClickHandler) up_single_click_handler;
  config[BUTTON_ID_UP]->click.repeat_interval_ms = 100;

  config[BUTTON_ID_DOWN]->click.handler = (ClickHandler) down_single_click_handler;
  config[BUTTON_ID_DOWN]->click.repeat_interval_ms = 100;
}

// App message callbacks
static void app_send_failed(DictionaryIterator* failed, AppMessageResult reason, void* context) {
	text_layer_set_text(&statusTextLayer, "Failed :(");	
}

static void app_received_msg(DictionaryIterator* received, void* context) {
	vibes_short_pulse();
	text_layer_set_text(&statusTextLayer, "Done.");	
}

bool register_callbacks() {
	if (callbacks_registered) {
		if (app_message_deregister_callbacks(&app_callbacks) == APP_MSG_OK)
			callbacks_registered = false;
	}
	if (!callbacks_registered) {
		app_callbacks = (AppMessageCallbacksNode){
			.callbacks = {
				.out_failed = app_send_failed,
        .in_received = app_received_msg
			},
			.context = NULL
		};
		if (app_message_register_callbacks(&app_callbacks) == APP_MSG_OK) {
      callbacks_registered = true;
    }
	}
	return callbacks_registered;
}

static void send_cmd(uint8_t cmd) {
  Tuplet value = TupletInteger(CMD_KEY, cmd);
  
  DictionaryIterator *iter;
  app_message_out_get(&iter);
  
  if (iter == NULL)
    return;
  
  dict_write_tuplet(iter, &value);
  dict_write_end(iter);
  
  app_message_out_send();
  app_message_out_release();
}

// Standard app initialisation

void handle_init(AppContextRef ctx) {
  (void)ctx;

	window_init(&window, "Ring My Phone");
	window_stack_push(&window, true /* Animated */);
	
	int16_t aThird = window.layer.frame.size.h / 3;
	
	GRect textFrame;
	textFrame.origin.x = 0;
	textFrame.origin.y = 0;
	textFrame.size.w = window.layer.frame.size.w;
	textFrame.size.h = aThird;
		
	text_layer_init(&ringTextLayer, textFrame);
	text_layer_set_text(&ringTextLayer, "Ring ->");
	text_layer_set_text_alignment(&ringTextLayer, GTextAlignmentRight);
	text_layer_set_font(&ringTextLayer, fonts_get_system_font(FONT_KEY_GOTHIC_28));
	layer_add_child(&window.layer, &ringTextLayer.layer);
	
	textFrame.origin.y += aThird;
	text_layer_init(&statusTextLayer, textFrame);
	text_layer_set_text(&statusTextLayer, "Ready.");
	text_layer_set_text_alignment(&statusTextLayer, GTextAlignmentRight);
	text_layer_set_font(&statusTextLayer, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));
	layer_add_child(&window.layer, &statusTextLayer.layer);
	
	textFrame.origin.y += aThird;
	text_layer_init(&silenceTextLayer, textFrame);
	text_layer_set_text(&silenceTextLayer, "Silence ->");
	text_layer_set_text_alignment(&silenceTextLayer, GTextAlignmentRight);
	text_layer_set_font(&silenceTextLayer, fonts_get_system_font(FONT_KEY_GOTHIC_28));
	layer_add_child(&window.layer, &silenceTextLayer.layer);
	
	register_callbacks();
	
	// Attach our desired button functionality
	window_set_click_config_provider(&window, (ClickConfigProvider) click_config_provider);
}


void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,
	.messaging_info = {
      .buffer_sizes = {
        .inbound = 256,
        .outbound = 256,
      }
    }
  };
  app_event_loop(params, &handlers);
}
