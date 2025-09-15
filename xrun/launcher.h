/* launcher.h */
#ifndef LAUNCHER_H
#define LAUNCHER_H

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>


#define MAX_CMD_LEN 256
#define MAX_HISTORY_ENTRIES 50

#define WINDOW_WIDTH 400
#define WINDOW_HEIGHT 200

#define TEXTBOX_X 10
#define TEXTBOX_Y 40
#define TEXTBOX_WIDTH 380
#define TEXTBOX_HEIGHT 30

#define LAUNCH_BUTTON_X 10
#define LAUNCH_BUTTON_Y 80
#define LAUNCH_BUTTON_WIDTH 80
#define LAUNCH_BUTTON_HEIGHT 30

#define CANCEL_BUTTON_X 100
#define CANCEL_BUTTON_Y 80
#define CANCEL_BUTTON_WIDTH 80
#define CANCEL_BUTTON_HEIGHT 30


typedef struct {
    Display *display;
    Window window;
    GC gc;
    char command_buffer[MAX_CMD_LEN];
    int buffer_pos;
    int show_textbox_fill;
    long last_blink_time;
    int text_input_active;
    char history[MAX_HISTORY_ENTRIES][MAX_CMD_LEN];
    int history_count;
    int history_pos;
} LauncherState;


void cleanup(LauncherState *state);
void init_x11(LauncherState *state);
void load_history(LauncherState *state);
void save_history(LauncherState *state);
void set_command_from_history(LauncherState *state);
void add_to_history(LauncherState *state, const char *command);
void draw_text(LauncherState *state, const char *text, int x, int y);
void draw_button(LauncherState *state, int x, int y, int width, int height, const char *text);
void draw_ui(LauncherState *state);
int is_inside(int px, int py, int rx, int ry, int rw, int rh);
void execute_command(LauncherState *state);
void handle_keypress(LauncherState *state, XEvent *event);

#endif /* LAUNCHER_H */
