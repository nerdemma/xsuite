/* launcher.c */
#include "launcher.h"

void cleanup(LauncherState *state) {
    if (state->gc) {
        XFreeGC(state->display, state->gc);
    }
    if (state->window) {
        XDestroyWindow(state->display, state->window);
    }
    if (state->display) {
        XCloseDisplay(state->display);
    }
}

void init_x11(LauncherState *state) {
    state->display = XOpenDisplay(NULL);
    if (state->display == NULL) {
        fprintf(stderr, "Cannot open display\n");
        exit(1);
    }

    state->window = XCreateSimpleWindow(
        state->display, DefaultRootWindow(state->display), 0, 0,
        WINDOW_WIDTH, WINDOW_HEIGHT, 1,
        BlackPixel(state->display, DefaultScreen(state->display)),
        WhitePixel(state->display, DefaultScreen(state->display))
    );

    XSelectInput(state->display, state->window, KeyPressMask | ButtonPressMask | ExposureMask);
    XMapWindow(state->display, state->window);

    state->gc = XCreateGC(state->display, state->window, 0, NULL);
    XSetForeground(state->display, state->gc, BlackPixel(state->display, DefaultScreen(state->display)));
}

void load_history(LauncherState *state) {
    FILE *file = fopen(".launcher_history", "r");
    if (!file) {
        return;
    }

    state->history_count = 0;
    while (fgets(state->history[state->history_count], MAX_CMD_LEN, file) != NULL && state->history_count < MAX_HISTORY_ENTRIES) {
        state->history[state->history_count][strcspn(state->history[state->history_count], "\n")] = '\0';
        state->history_count++;
    }
    fclose(file);
    state->history_pos = state->history_count;
}

void save_history(LauncherState *state) {
    FILE *file = fopen(".launcher_history", "w");
    if (!file) {
        perror("Failed to open history file for writing");
        return;
    }
    for (int i = 0; i < state->history_count; i++) {
        fprintf(file, "%s\n", state->history[i]);
    }
    fclose(file);
}

void add_to_history(LauncherState *state, const char *command) {
    if (strlen(command) == 0) return;
    
    // Check if the last command is the same as the current one
    if (state->history_count > 0 && strcmp(state->history[state->history_count - 1], command) == 0) {
        return;
    }
    
    // Shift history if full
    if (state->history_count == MAX_HISTORY_ENTRIES) {
        memmove(state->history[0], state->history[1], (MAX_HISTORY_ENTRIES - 1) * sizeof(state->history[0]));
        state->history_count--;
    }
    
    strncpy(state->history[state->history_count], command, MAX_CMD_LEN - 1);
    state->history[state->history_count][MAX_CMD_LEN - 1] = '\0';
    state->history_count++;
    state->history_pos = state->history_count;
}

void set_command_from_history(LauncherState *state) {
    if (state->history_pos >= 0 && state->history_pos < state->history_count) {
        strncpy(state->command_buffer, state->history[state->history_pos], MAX_CMD_LEN - 1);
        state->command_buffer[MAX_CMD_LEN - 1] = '\0';
        state->buffer_pos = strlen(state->command_buffer);
    } else {
        state->command_buffer[0] = '\0';
        state->buffer_pos = 0;
    }
}

void draw_text(LauncherState *state, const char *text, int x, int y) {
    XDrawString(state->display, state->window, state->gc, x, y, text, strlen(text));
}

void draw_button(LauncherState *state, int x, int y, int width, int height, const char *text) {
    XDrawRectangle(state->display, state->window, state->gc, x, y, width, height);
    XDrawString(state->display, state->window, state->gc, x + 5, y + (height / 2) + 5, text, strlen(text));
}

void draw_ui(LauncherState *state) {
    XClearWindow(state->display, state->window);

    draw_text(state, "Enter Command:", 10, 20);

    if (state->show_textbox_fill) {
        XSetForeground(state->display, state->gc, WhitePixel(state->display, DefaultScreen(state->display)));
    } else {
        XSetForeground(state->display, state->gc, WhitePixel(state->display, DefaultScreen(state->display)) - 0x101022);
    }
    XFillRectangle(state->display, state->window, state->gc, TEXTBOX_X + 1, TEXTBOX_Y + 1, TEXTBOX_WIDTH - 2, TEXTBOX_HEIGHT - 2);

    XSetForeground(state->display, state->gc, BlackPixel(state->display, DefaultScreen(state->display)));
    XDrawRectangle(state->display, state->window, state->gc, TEXTBOX_X, TEXTBOX_Y, TEXTBOX_WIDTH, TEXTBOX_HEIGHT);
    draw_text(state, state->command_buffer, TEXTBOX_X + 5, TEXTBOX_Y + 20);

    draw_button(state, LAUNCH_BUTTON_X, LAUNCH_BUTTON_Y, LAUNCH_BUTTON_WIDTH, LAUNCH_BUTTON_HEIGHT, "Launch");
    draw_button(state, CANCEL_BUTTON_X, CANCEL_BUTTON_Y, CANCEL_BUTTON_WIDTH, CANCEL_BUTTON_HEIGHT, "Cancel");
}

int is_inside(int px, int py, int rx, int ry, int rw, int rh) {
    return (px >= rx && px <= rx + rw && py >= ry && py <= ry + rh);
}

void execute_command(LauncherState *state) {
    if (state->buffer_pos == 0) {
        return;
    }

    add_to_history(state, state->command_buffer);
    save_history(state);

    char *command_copy = strdup(state->command_buffer);
    if (!command_copy) {
        perror("Failed to allocate memory for command");
        return;
    }
    
    char *args[20];
    int i = 0;

    char *token = strtok(command_copy, " ");
    while (token != NULL && i < 19) {
        args[i++] = token;
        token = strtok(NULL, " ");
    }
    args[i] = NULL;

    if (i > 0) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork failed");
        } else if (pid == 0) {
            execvp(args[0], args);
            perror("execvp failed");
            exit(1);
        }
    }
    
    free(command_copy);
    state->command_buffer[0] = '\0';
    state->buffer_pos = 0;
}

void handle_keypress(LauncherState *state, XEvent *event) {
    KeySym key;
    char buffer[2];
    int len = XLookupString(&event->xkey, buffer, sizeof(buffer), &key, NULL);
    buffer[len] = '\0';

    if (key == XK_Return) {
        execute_command(state);
    } else if (key == XK_BackSpace) {
        if (state->buffer_pos > 0) {
            state->buffer_pos--;
            state->command_buffer[state->buffer_pos] = '\0';
        }
    } else if (key == XK_Escape) {
        exit(0);
    } else if (key == XK_Up) {
        if (state->history_pos > 0) {
            state->history_pos--;
            set_command_from_history(state);
        }
    } else if (key == XK_Down) {
        if (state->history_pos < state->history_count - 1) {
            state->history_pos++;
            set_command_from_history(state);
        } else if (state->history_pos == state->history_count - 1) {
            state->history_pos++;
            state->command_buffer[0] = '\0';
            state->buffer_pos = 0;
        }
    } else if (len > 0 && state->buffer_pos < MAX_CMD_LEN - 1) {
        strncat(state->command_buffer, buffer, 1);
        state->buffer_pos++;
    }
    draw_ui(state);
}

int main() {
    LauncherState state = {0};
    state.command_buffer[0] = '\0';
    state.buffer_pos = 0;
    state.show_textbox_fill = 1;
    state.last_blink_time = 0;
    state.text_input_active = 1;
    state.history_count = 0;
    state.history_pos = 0;

    init_x11(&state);
    load_history(&state);

    while (1) {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        long current_time = tv.tv_sec * 1000 + tv.tv_usec / 1000;

        if (state.text_input_active && (current_time - state.last_blink_time > 500)) {
            state.show_textbox_fill = !state.show_textbox_fill;
            draw_ui(&state);
            XFlush(state.display);
            state.last_blink_time = current_time;
        }

        if (XPending(state.display)) {
            XEvent event;
            XNextEvent(state.display, &event);

            switch (event.type) {
                case Expose:
                    draw_ui(&state);
                    break;
                case ButtonPress:
                    if (is_inside(event.xbutton.x, event.xbutton.y, LAUNCH_BUTTON_X, LAUNCH_BUTTON_Y, LAUNCH_BUTTON_WIDTH, LAUNCH_BUTTON_HEIGHT)) {
                        execute_command(&state);
                        draw_ui(&state);
                    } else if (is_inside(event.xbutton.x, event.xbutton.y, CANCEL_BUTTON_X, CANCEL_BUTTON_Y, CANCEL_BUTTON_WIDTH, CANCEL_BUTTON_HEIGHT)) {
                        exit(0);
                    } else if (is_inside(event.xbutton.x, event.xbutton.y, TEXTBOX_X, TEXTBOX_Y, TEXTBOX_WIDTH, TEXTBOX_HEIGHT)) {
                        state.text_input_active = 1;
                        state.show_textbox_fill = 1;
                        state.last_blink_time = current_time;
                        draw_ui(&state);
                    } else {
                        state.text_input_active = 0;
                    }
                    break;
                case KeyPress:
                    if (state.text_input_active) {
                        handle_keypress(&state, &event);
                    }
                    break;
            }
        }
    }
    
    cleanup(&state);
    return 0;
}
