#include <pebble.h>

#define KEY_REQ 1
#define KEY_ID 2
#define KEY_VALUE 3
#define KEY_APP_TITLE 4
#define KEY_APP_MENU_COUNT 5
#define KEY_APP_DATA 6

#define REQ_TYPE_APPLIST 0
#define REQ_TYPE_RESPONSE 1

#define APP_TYPE_SWITCH 1
#define APP_TYPE_SELECTION 2

static Window *main_window;
static MenuLayer *main_menu_layer;

static char main_menu_text[][80] = {"App List", "Setting", "Get Apps"};

typedef enum parse_menu_state{
	get_type = 0,
	switch_get_id,
	switch_get_value,
	switch_get_title,
	switch_get_subtitle,
	selection_get_id,
	selection_get_value,
	selection_get_title,
	selection_get_subtitle,
	selection_get_count,
	selection_get_option_title,
	selection_get_option_subtitle,
	finish
}ParseMenuState;

typedef struct {
	MenuLayer *layer;
	uint8_t option_count;
	char **title;	// note this is double pointer
	char **subtitle;// note this is double pointer
	// XXX: add callback functions?
} MenuLayerData;

typedef struct {
	TextLayer *layer;
	char *text;
} TextLayerData;

typedef struct {
	char *id;
	uint8_t type;	// type of this sub menu, 1=switch, 2=selection
	uint8_t value;	// current value (selected value)
	char *title;
	char *subtitle;
	Window *window;	// pointer to window, could be NULL for switch
	union {
		MenuLayerData *menu_layer_data;
		TextLayerData *text_layer_data;
	};
} AppData;

typedef struct {
	char *title;
	int8_t menu_count;
	Window *app_main_window;
	MenuLayer *app_main_menu_layer;
	int8_t selected_menu_index;
	AppData **menu; // note this is double pointer, and we will allocate pointer array later
} App;

static App *main_app;

char *my_strtok(char * str, const char * delim){
// XXX: not sure if it is safe to use strspn
	static char* p=0;
	if(str)
		p=str;
	else if(!p)
		return 0;
	str=p+strspn(p,delim);
	p=str+strcspn(str,delim);
	if(p==str)
		return p=0;
	p = *p ? *p=0,p+1 : 0;
	return str;
}

static void parse_app_data(char *app_title, int8_t app_menu_count, char *app_data){
	char *pch = NULL;
	int menu_index, menu_type, option_index;
	ParseMenuState state = get_type;
	AppData *ptr = NULL;

	// early error handling
	if(app_menu_count <= 0 || strlen(app_data) <= 0)
		return;

	// allocate space for main_app object and its parameters
	main_app = (App *)malloc(sizeof(App));
	main_app->title = (char *)malloc((strlen(app_title)+1)*sizeof(char));
	strncpy(main_app->title, app_title, strlen(app_title));
	main_app->menu_count = app_menu_count;
	main_app->menu = malloc(sizeof(AppData *)*app_menu_count);

	// initialize parse state
	state = get_type;
	menu_index = 0;
	option_index = 0;

	// main loop, parse elements which separated by '|' one by one
	// assign to correct data structure by state changes
	pch = my_strtok(app_data, "|");
	while(pch != NULL){
		APP_LOG(APP_LOG_LEVEL_DEBUG, "pch = %s state = %d", pch, state);
		// If it is get_type state, we are going to create new sub menu
		if(state == get_type){
			// determine sub menu type and change state
			menu_type = atoi(pch);
			if(menu_type == APP_TYPE_SWITCH)
				state = switch_get_id;
			else if(menu_type == APP_TYPE_SELECTION)
				state = selection_get_id;
			// allocate space for sub menu data
			main_app->menu[menu_index] = (AppData *)malloc(sizeof(AppData));
			ptr = main_app->menu[menu_index];
			// setup the app menu type
			ptr->type = menu_type;
		}
		// If it is not get_type state, we store the information to current index
		else{
			switch(state){
				case switch_get_id:
				case selection_get_id:
					ptr->id = (char *)malloc((strlen(pch)+1)*sizeof(char));
					strncpy(ptr->id, pch, strlen(pch));
					state++;
					break;
				case switch_get_value:
				case selection_get_value:
					ptr->value = atoi(pch);
					state++;
					break;
				case switch_get_title:
				case selection_get_title:
					ptr->title = (char *)malloc((strlen(pch)+1)*sizeof(char));
					strncpy(ptr->title, pch, strlen(pch));
					APP_LOG(APP_LOG_LEVEL_DEBUG, "get title: %s", main_app->menu[0]->title);
					state++;
					break;
				case switch_get_subtitle:
					ptr->subtitle = (char *)malloc((strlen(pch)+1)*sizeof(char));
					strncpy(ptr->subtitle, pch, strlen(pch));
					menu_index++;
					state = get_type;
					break;
				case selection_get_subtitle:
					ptr->subtitle = (char *)malloc((strlen(pch)+1)*sizeof(char));
					strncpy(ptr->subtitle, pch, strlen(pch));
					state++;
					break;
				case selection_get_count:
					// allocate space for layer_data, and store the count
					option_index = 0;
					ptr->menu_layer_data = malloc(sizeof(MenuLayerData));
					ptr->menu_layer_data->option_count = atoi(pch);
					// we also allocate space for title/subtitle of each option
					ptr->menu_layer_data->title = 
						malloc(sizeof(char *) * ptr->menu_layer_data->option_count);
					ptr->menu_layer_data->subtitle = 
						malloc(sizeof(char *) * ptr->menu_layer_data->option_count);
					state = selection_get_option_title;
					break;
				case selection_get_option_title:
					ptr->menu_layer_data->title[option_index] = (char *)malloc((strlen(pch)+1)*sizeof(char));
					strncpy(ptr->menu_layer_data->title[option_index], pch, strlen(pch));
					state = selection_get_option_subtitle;
					break;
				case selection_get_option_subtitle:
					ptr->menu_layer_data->subtitle[option_index] = (char *)malloc((strlen(pch)+1)*sizeof(char));
					strncpy(ptr->menu_layer_data->subtitle[option_index], pch, strlen(pch));
					if(option_index >= ptr->menu_layer_data->option_count){
					// we don't have more options. Prepare to read new menu
						menu_index++;
						ptr = main_app->menu[menu_index];
						state = get_type;
					}
					else{
					// read more options
						option_index++;
						state = selection_get_option_title;
					}
					break;
				default:
					break;
			}
		}

		pch = my_strtok(NULL, "|");
	}
}

static void free_app_data(App *main_app){
	int i, j;
	AppData *ptr;
	
	// free menu items
	for(i=0;i<main_app->menu_count;i++){
		ptr = main_app->menu[i];
		if(ptr->type == APP_TYPE_SWITCH){
			if(ptr->id != NULL) free(ptr->id);
			if(ptr->title != NULL) free(ptr->title);
			if(ptr->subtitle != NULL) free(ptr->subtitle);
			if(ptr->window != NULL){
				// NOTE: should not happen
				APP_LOG(APP_LOG_LEVEL_WARNING, "Try to free switch window! Should not happen!");
				window_destroy(ptr->window); 
				ptr->window = NULL;
			}
			if(ptr->menu_layer_data != NULL){
				// NOTE: should not happen
				APP_LOG(APP_LOG_LEVEL_WARNING, "Try to free switch layer! Should not happen!");
				free(ptr->menu_layer_data);
			}
		}
		else if(ptr->type == APP_TYPE_SELECTION){
			if(ptr->id != NULL) free(ptr->id);
			if(ptr->title != NULL) free(ptr->title);
			if(ptr->subtitle != NULL) free(ptr->subtitle);
			if(ptr->window != NULL){
				window_destroy(ptr->window);
				ptr->window = NULL;
			}
			if(ptr->menu_layer_data != NULL){
				// this must be MenuLayerData
				if(ptr->menu_layer_data->layer != NULL){
					menu_layer_destroy(ptr->menu_layer_data->layer);
					ptr->menu_layer_data->layer = NULL;
				}
				for(j=0;j<ptr->menu_layer_data->option_count;j++){
					if(ptr->menu_layer_data->title[j] != NULL)
						free(ptr->menu_layer_data->title[j]);
					if(ptr->menu_layer_data->subtitle[j] != NULL)
						free(ptr->menu_layer_data->subtitle[j]);
				}
				free(ptr->menu_layer_data->title);
				free(ptr->menu_layer_data->subtitle);
				free(ptr->menu_layer_data);
			}
		}
		free(ptr);
	}

	// free app itself
	if(main_app->menu != NULL) free(main_app->menu);
	if(main_app->title != NULL) free(main_app->title);
	if(main_app->app_main_window != NULL) window_destroy(main_app->app_main_window);
	if(main_app->app_main_menu_layer != NULL) menu_layer_destroy(main_app->app_main_menu_layer);
}

static void send_req(int8_t req, char *id, int8_t value){
	DictionaryIterator *iter;
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Going to send req %d", req);
	app_message_outbox_begin(&iter);
	dict_write_int8(iter, KEY_REQ, req);
	if(id != NULL){
		dict_write_cstring(iter, KEY_ID, id);
		dict_write_int8(iter, KEY_VALUE, value);
	}
	app_message_outbox_send();
}

static uint16_t app_main_menu_get_num_rows_callback(struct MenuLayer *menulayer, uint16_t section_index, void *callback_context){
	return main_app->menu_count;
}

static void app_main_menu_draw_row_handler(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context){
	static char buf[80];
	switch(main_app->menu[cell_index->row]->type){
		case APP_TYPE_SWITCH:
			// TODO: use icon to indicate the on/off value
			snprintf(buf, sizeof(buf), "(%s)%s", main_app->menu[cell_index->row]->value>0?"On":"Off", main_app->menu[cell_index->row]->subtitle);
			menu_cell_basic_draw(ctx, cell_layer, main_app->menu[cell_index->row]->title, buf, NULL); 
			break;
		case APP_TYPE_SELECTION:
			menu_cell_basic_draw(ctx, cell_layer, main_app->menu[cell_index->row]->title, main_app->menu[cell_index->row]->subtitle, NULL); 
			break;
		default:
			APP_LOG(APP_LOG_LEVEL_ERROR, "Try to draw unknown app type:%d", main_app->menu[cell_index->row]->type);
			break;
	}
}


static uint16_t selection_menu_get_num_rows_callback(struct MenuLayer *menulayer, uint16_t section_index, void *callback_context){
	return main_app->menu[main_app->selected_menu_index]->menu_layer_data->option_count;
}

static void selection_menu_draw_row_handler(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context){
	// TODO: use icon for checked item
	AppData *ptr;
	char buf[80];
	ptr = main_app->menu[main_app->selected_menu_index];

	if(ptr->value == cell_index->row){
		snprintf(buf, sizeof(buf), "(Checked)%s", ptr->menu_layer_data->subtitle[cell_index->row]);
		menu_cell_basic_draw(ctx, cell_layer, ptr->menu_layer_data->title[cell_index->row], buf, NULL); 
	}
	else
		menu_cell_basic_draw(ctx, cell_layer, ptr->menu_layer_data->title[cell_index->row], ptr->menu_layer_data->subtitle[cell_index->row], NULL); 
}

static void selection_menu_select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context){
	AppData *ptr;

	ptr = main_app->menu[main_app->selected_menu_index];
	ptr->value = cell_index->row;

	window_stack_pop(true);
	send_req(REQ_TYPE_RESPONSE, ptr->id, ptr->value);
}

static void selection_window_load(Window *window){
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(window_layer);
	AppData *ptr;
	
	ptr = main_app->menu[main_app->selected_menu_index];

	ptr->menu_layer_data->layer = menu_layer_create(bounds);
	menu_layer_set_callbacks(ptr->menu_layer_data->layer, NULL, (MenuLayerCallbacks){
		.get_num_rows = selection_menu_get_num_rows_callback,
		.draw_row = selection_menu_draw_row_handler,
		.select_click = selection_menu_select_callback
	});
	menu_layer_set_click_config_onto_window(ptr->menu_layer_data->layer, window);

	layer_add_child(window_layer, menu_layer_get_layer(ptr->menu_layer_data->layer));
}

static void selection_window_unload(Window *window){
	menu_layer_destroy(main_app->menu[main_app->selected_menu_index]->menu_layer_data->layer);
}

static void app_main_menu_select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context){
	int8_t index;
	AppData *ptr; 
	APP_LOG(APP_LOG_LEVEL_DEBUG, "app main menu: item %d pressed", cell_index->row);

	main_app->selected_menu_index = index = cell_index->row;
	ptr = main_app->menu[index];

	switch(ptr->type){
		case APP_TYPE_SWITCH:
			// invert value and redraw menu
			ptr->value = !ptr->value;
			layer_mark_dirty(menu_layer_get_layer(main_app->app_main_menu_layer));
			send_req(REQ_TYPE_RESPONSE, ptr->id, ptr->value);
			break;
		case APP_TYPE_SELECTION:
			// create new window and push it onto the top
			ptr->window = window_create();
			window_set_window_handlers(ptr->window, (WindowHandlers){
				.load = selection_window_load,
				.unload = selection_window_unload
			});
			window_stack_push(ptr->window, true);
			break;
		default:
			APP_LOG(APP_LOG_LEVEL_ERROR, "Unknown main menu type:%d index:%d", ptr->type, index);
			break;
	}
}

static int16_t app_main_menu_get_header_height_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context){
	return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static void app_main_menu_draw_header_handler(GContext *ctx, const Layer *cell_layer, uint16_t section_index, void *callback_context){
	menu_cell_basic_header_draw(ctx, cell_layer, main_app->title);
}

static void app_main_window_load(Window *window){
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(window_layer);

	main_app->app_main_menu_layer = menu_layer_create(bounds);
	menu_layer_set_callbacks(main_app->app_main_menu_layer, NULL, (MenuLayerCallbacks){
		.get_num_rows = app_main_menu_get_num_rows_callback,
		.draw_row = app_main_menu_draw_row_handler,
		.select_click = app_main_menu_select_callback,
		.get_header_height = app_main_menu_get_header_height_callback,
		.draw_header = app_main_menu_draw_header_handler
	});
	menu_layer_set_click_config_onto_window(main_app->app_main_menu_layer, window);
	layer_add_child(window_layer, menu_layer_get_layer(main_app->app_main_menu_layer));
}

static void app_main_window_unload(Window *window){
	menu_layer_destroy(main_app->app_main_menu_layer);
}

static void create_app_ui(App *main_app){
	// early error handling
	if(main_app == NULL)
		return;

	// setup main window
	main_app->app_main_window = window_create();
	window_set_window_handlers(main_app->app_main_window, (WindowHandlers){
		.load = app_main_window_load,
		.unload = app_main_window_unload
	});
	window_stack_push(main_app->app_main_window, true);
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
	APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped! Reason:%d", reason);
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context){
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Received message");

	Tuple *t = dict_read_first(iterator);
	int8_t app_menu_count = 0;
	static char app_title[80];
	static char app_data[1024];
	
	while(t != NULL){
		switch(t->key){
			case KEY_APP_TITLE:
				strncpy(app_title, t->value->cstring, sizeof(app_title));
				APP_LOG(APP_LOG_LEVEL_DEBUG, "received title %s", app_title);
				break;
			case KEY_APP_MENU_COUNT:
				app_menu_count = t->value->int8;
				APP_LOG(APP_LOG_LEVEL_DEBUG, "received menu count %d", app_menu_count);
				break;
			case KEY_APP_DATA:
				strncpy(app_data, t->value->cstring, sizeof(app_data));
				APP_LOG(APP_LOG_LEVEL_DEBUG, "received data length:%d data:%s", strlen(app_data), app_data);
				break;
		}
		t = dict_read_next(iterator);
	}

	parse_app_data(app_title, app_menu_count, app_data);
	create_app_ui(main_app);
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Outbox send success!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
	APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed! Reason:%d", reason);
}

static uint16_t main_menu_get_num_rows_callback(struct MenuLayer *menulayer, uint16_t section_index, void *callback_context){
	return 3;
}

static void main_menu_draw_row_handler(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context){
	const char* text = main_menu_text[cell_index->row];

	menu_cell_basic_draw(ctx, cell_layer, text, NULL, NULL); 
}

static void main_menu_select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context){
	switch(cell_index->row){
		case 0:
			send_req(REQ_TYPE_APPLIST, NULL, 0);
			break;
		case 1:
			break;
		case 2:
			break;
		default:
			APP_LOG(APP_LOG_LEVEL_ERROR, "Unknown main menu operation:%d", cell_index->row);
			break;
	}
}

static void main_window_load(Window *window){
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(window_layer);

	main_menu_layer = menu_layer_create(bounds);
	menu_layer_set_callbacks(main_menu_layer, NULL, (MenuLayerCallbacks){
		.get_num_rows = main_menu_get_num_rows_callback,
		.draw_row = main_menu_draw_row_handler,
		.select_click = main_menu_select_callback
	});
	menu_layer_set_click_config_onto_window(main_menu_layer, window);

	layer_add_child(window_layer, menu_layer_get_layer(main_menu_layer));
}

static void main_window_unload(Window *window){
	menu_layer_destroy(main_menu_layer);
}

static void init(void){
	app_message_register_inbox_received(inbox_received_callback);
	app_message_register_inbox_dropped(inbox_dropped_callback);
	app_message_register_outbox_failed(outbox_failed_callback);
	app_message_register_outbox_sent(outbox_sent_callback);
	app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
	main_window = window_create();
	window_set_window_handlers(main_window, (WindowHandlers){
		.load = main_window_load,
		.unload = main_window_unload
	});
	window_stack_push(main_window, true);
}

static void deinit(void){
	free_app_data(main_app);
	window_destroy(main_window);
}

int main(void) {
	init();
	app_event_loop();
	deinit();
}

