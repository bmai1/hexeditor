#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_dialog.h>
#include <SDL3_ttf/SDL_ttf.h>
 
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <unordered_set>
 
#include "editor.h"
 
// EditorState, open_file_callback, format_row, get_byte_index, index_to_hex,
// and save_file are all declared in editor.h and implemented in editor.cpp
 
void handle_mouse_click(const SDL_Event &event, EditorState& editor) {
    if (event.button.button == SDL_BUTTON_LEFT) {
        float click_x = event.button.x;
        float click_y = event.button.y;
 
        // Clicked save file button
        if (click_x > editor.width - 120 && click_x < editor.width - 20 && click_y > editor.height - 40 && click_y < editor.height - 20) {
            SDL_Log("Saved file");
            save_file(editor.file_path, editor.bytes);
        }
        // Clicked open file button
        else if (click_x > editor.width - 120 && click_x < editor.width - 20 && click_y > editor.height - 70 && click_y < editor.height - 50) {
            SDL_Log("Opened file");
            SDL_ShowOpenFileDialog(open_file_callback, &editor, editor.window, NULL, 0, NULL, false);
        }
        else {
            // Handle highlighting bytes based on left clicks
            editor.byte_index = get_byte_index(editor, (int) click_x, (int) click_y);
 
            bool valid = (editor.byte_index >= 0 && editor.byte_index < editor.bytes.size());
 
            // If invalid, reset states
            if (!valid) {
                SDL_Log("Mouse left-clicked outside of editor space");
                editor.show_cursor_rect = false;
                editor.show_edit_rect = false;
                SDL_StopTextInput(editor.window);
            }
            else {
                SDL_Log("Mouse left-clicked at: %f, %f, which corresponds to byte index %zu", click_x, click_y, editor.byte_index);
 
                // IF IN DEFAULT OR EDIT MODE AND LEFT CLICK -> ENTER CURSOR MODE
                if (!editor.show_cursor_rect) {
                    editor.show_cursor_rect = true;
                    editor.show_edit_rect = false;
                    SDL_StopTextInput(editor.window);
                }
                // IF IN CURSOR MODE AND LEFT CLICK SAME BYTE -> ENTER EDIT MODE
                else if (editor.byte_index == editor.prev_byte_index) {
                    editor.edit_byte = index_to_hex(editor, editor.byte_index);
                    editor.show_cursor_rect = false;
                    editor.show_edit_rect = true;
                    SDL_StartTextInput(editor.window);
                }
 
                // (If in cursor mode and left click new byte, nothing happens besides the rectangles being updated)
                editor.cursor_rect = {(float) 110+30*(editor.byte_index%16), (float)(31 + 16*((int)(editor.byte_index/16) - (int)editor.scroll_offset)), 20, 14};
                editor.edit_rect   = {(float) 110+30*(editor.byte_index%16), (float)(30 + 16*((int)(editor.byte_index/16) - (int)editor.scroll_offset)), 20, 16};
                editor.ascii_rect  = {(float) 600+10*(editor.byte_index%16), (float)(31 + 16*((int)(editor.byte_index/16) - (int)editor.scroll_offset)), 10, 14};
 
                editor.prev_byte_index = editor.byte_index;
            }
        }
    }
    else {
        SDL_Log("Mouse right-clicked");
    }
}
 
void handle_text_input(const SDL_Event& event, EditorState& editor) {
    SDL_Log("Typed: %s", event.text.text);
    if (std::isalnum(event.text.text[0])) {
        if (editor.edit_byte.size() == 2) {
            editor.edit_byte = event.text.text;
        }
        else if (editor.edit_byte.size() < 2) {
            editor.edit_byte += event.text.text;
        }
    }
}
 
void handle_key_down(const SDL_Event& event, EditorState& editor) {
    bool edited = false;
 
    // Handle enter key: enter edit byte mode if cursor is active OR update the byte array (not saving to the file yet)
    if (event.key.key == SDLK_RETURN && editor.edit_byte.size() == 2) { // Enforce 2 char length for hex byte
        if (editor.show_cursor_rect && !editor.show_edit_rect) {
            editor.show_cursor_rect = false;
            editor.show_edit_rect = true;
            SDL_StartTextInput(editor.window);
            editor.edit_rect = {(float) 110+30*(editor.byte_index%16), (float)(30 + 16*((int)(editor.byte_index/16) - (int)editor.scroll_offset)), 20, 16};
        }
        else if (editor.show_edit_rect) {
            if (editor.byte_index >= 0 && editor.byte_index < editor.bytes.size()) {
                editor.bytes[editor.byte_index] = (unsigned char) std::stoi(editor.edit_byte, nullptr, 16);
                editor.edited_bytes.insert(editor.byte_index); // Render edited bytes in red
            }
 
            // Automatically move to editing next byte (re-render rects) instead of exiting edit mode
            if (editor.byte_index < editor.bytes.size() - 1) {
                editor.byte_index++;
                edited = true;
            }
        }
    }
    // Handle arrow key cursor movement (note: does not update byte array)
    else if (event.key.key == SDLK_UP && (editor.show_cursor_rect || editor.show_edit_rect) && editor.byte_index >= 16 && editor.byte_index < editor.bytes.size()) {
        editor.byte_index -= 16;
        edited = true;
    }
    else if (event.key.key == SDLK_DOWN && (editor.show_cursor_rect || editor.show_edit_rect) && editor.byte_index >= 0 && editor.byte_index < editor.bytes.size() - 16) {
        // Need to fix a bug where you can go down past editor box if there is more lines not visible below
        editor.byte_index += 16;
        edited = true;
    }
    else if (event.key.key == SDLK_LEFT && (editor.show_cursor_rect || editor.show_edit_rect) && editor.byte_index > 0 && editor.byte_index < editor.bytes.size()) {
        editor.byte_index--;
        edited = true;
    }
    else if (event.key.key == SDLK_RIGHT && (editor.show_cursor_rect || editor.show_edit_rect) && editor.byte_index >= 0 && editor.byte_index < editor.bytes.size() - 1) {
        editor.byte_index++;
        edited = true;
    }
 
    // Re-render cursor rectangles
    if (editor.byte_index != (size_t)-1) {
        if (edited) editor.edit_byte = index_to_hex(editor, editor.byte_index);
        editor.edit_rect   = {(float) 110+30*(editor.byte_index%16), (float)(30 + 16*((int)(editor.byte_index/16) - (int)editor.scroll_offset)), 20, 16};
        editor.cursor_rect = {(float) 110+30*(editor.byte_index%16), (float)(31 + 16*((int)(editor.byte_index/16) - (int)editor.scroll_offset)), 20, 14};
        editor.ascii_rect  = {(float) 600+10*(editor.byte_index%16), (float)(31 + 16*((int)(editor.byte_index/16) - (int)editor.scroll_offset)), 10, 14};
    }
}
 
void handle_scroll(const SDL_Event& event, EditorState& editor) {
    size_t total_rows = (editor.bytes.size() + 15) / 16;
 
    if (event.wheel.y < 0) { // scroll down
        if (editor.scroll_offset + editor.visible_rows < total_rows)
            editor.scroll_offset++;
    }
    else if (event.wheel.y > 0) { // scroll up
        if (editor.scroll_offset > 0)
            editor.scroll_offset--;
    }
}
 
void render_bytes(EditorState& editor) {
    SDL_SetRenderDrawColor(editor.renderer, 0, 0, 0, 255);
    SDL_RenderClear(editor.renderer);
 
    // Render red highlights underneath edited bytes
    for (size_t edited_byte_index : editor.edited_bytes) {
        int screen_row = (int)(edited_byte_index / 16) - (int)editor.scroll_offset;
        if (screen_row < 0 || screen_row >= (int) editor.visible_rows) {
            continue;
        }
        SDL_FRect edited_rect = {(float) 110+30*(edited_byte_index%16), (float) 31+16*screen_row, 20, 14};
        SDL_FRect edited_ascii_rect  = {(float) 600+10*(edited_byte_index%16), (float) 31+16*screen_row, 10, 14};
        SDL_SetRenderDrawColor(editor.renderer, 255, 0, 0, 192);
        SDL_RenderFillRect(editor.renderer, &edited_rect);
        SDL_RenderFillRect(editor.renderer, &edited_ascii_rect);
    }
 
    // Render bytes as lines
    if (!editor.bytes.empty()) {
        constexpr int line_height = 16;
        constexpr int header_y = 10;
 
        // Render header (always visible)
        {
            std::ostringstream data_label;
            data_label << std::left
                    << std::setw(10) << "Address"
                    << std::setw(49) << "Hexadecimal"
                    << "ASCII";
 
            SDL_Surface* surface =
                TTF_RenderText_Solid(editor.font, data_label.str().c_str(), 0, editor.white);
 
            SDL_Texture* texture =
                SDL_CreateTextureFromSurface(editor.renderer, surface);
 
            SDL_FRect dst = {
                10.0f,
                (float)header_y,
                (float)surface->w,
                (float)surface->h
            };
 
            SDL_RenderTexture(editor.renderer, texture, NULL, &dst);
 
            SDL_DestroyTexture(texture);
            SDL_DestroySurface(surface);
        }
 
        size_t total_rows = (editor.bytes.size() + 15) / 16;
 
        // Render visible data rows
        for (size_t i = 0; i < editor.visible_rows; ++i) {
            size_t row = editor.scroll_offset + i;
 
            if (row >= total_rows)
                break;
 
            std::string line = format_row(editor.bytes, row * 16);
 
            SDL_Surface* surface =
                TTF_RenderText_Solid(editor.font, line.c_str(), 0, editor.white);
 
            SDL_Texture* texture =
                SDL_CreateTextureFromSurface(editor.renderer, surface);
 
            SDL_FRect dst = {
                10.0f,
                (float)(header_y + line_height + i * line_height),
                (float)surface->w,
                (float)surface->h
            };
 
            SDL_RenderTexture(editor.renderer, texture, NULL, &dst);
 
            SDL_DestroyTexture(texture);
            SDL_DestroySurface(surface);
        }
    }
}
 
void render_cursor(EditorState& editor) {
    // Render cursor rectangle
    if (editor.show_cursor_rect) {
        SDL_SetRenderDrawColor(editor.renderer, 255, 255, 255, 128);
        SDL_RenderFillRect(editor.renderer, &editor.cursor_rect);
        SDL_RenderFillRect(editor.renderer, &editor.ascii_rect);
    }
 
    // Blue highlight when editing byte
    if (editor.show_edit_rect) {
        SDL_SetRenderDrawColor(editor.renderer, 60, 77, 145, 255);
        SDL_RenderFillRect(editor.renderer, &editor.edit_rect);
 
        // Edited byte label
        SDL_Surface* edit_byte_surface = TTF_RenderText_Solid(editor.font, editor.edit_byte.c_str(), 0, editor.white);
        if (editor.edit_byte_texture) {
            SDL_DestroyTexture(editor.edit_byte_texture);
        }
        editor.edit_byte_texture = SDL_CreateTextureFromSurface(editor.renderer, edit_byte_surface);
        SDL_DestroySurface(edit_byte_surface);
        SDL_FRect dst = {
            editor.edit_rect.x,
            editor.edit_rect.y - 4,
            10 * (float) editor.edit_byte.size(), // So the numbers don't change width when typing
            24,
        };
        SDL_RenderTexture(editor.renderer, editor.edit_byte_texture, NULL, &dst);
    }
}
 
void render_buttons(EditorState& editor) {
    SDL_SetRenderDrawColor(editor.renderer, 255, 255, 255, 64);
 
    // Open new file button
    editor.open_file_rect.x = editor.width - 120;
    editor.open_file_rect.y = editor.height - 70;
    SDL_RenderFillRect(editor.renderer, &editor.open_file_rect);
    SDL_FRect dst = { editor.open_file_rect.x + 10, editor.open_file_rect.y - 3, 80, 24 };
    SDL_RenderTexture(editor.renderer, editor.open_file_texture, NULL, &dst);
 
    // Save file button
    editor.save_file_rect.x = editor.width - 120;
    editor.save_file_rect.y = editor.height - 40;
    SDL_RenderFillRect(editor.renderer, &editor.save_file_rect);
    dst = { editor.save_file_rect.x + 10, editor.save_file_rect.y - 3, 80, 24 };
    SDL_RenderTexture(editor.renderer, editor.save_file_texture, NULL, &dst);
 
    SDL_RenderPresent(editor.renderer);
}
 
int main(int argc, char* argv[]) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s", SDL_GetError());
        return 1;
    }
 
    EditorState editor;
    if (!SDL_CreateWindowAndRenderer("Hex Editor", 780, 480, SDL_WINDOW_RESIZABLE, &editor.window, &editor.renderer)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window and renderer: %s", SDL_GetError());
        return 1;
    }
 
    SDL_SetRenderDrawBlendMode(editor.renderer, SDL_BLENDMODE_BLEND); // To allow opacity
 
    // Init font
    TTF_Init();
    editor.font = TTF_OpenFont("SpaceMono.ttf", 16);
 
    // Prompt for binary file
    SDL_ShowOpenFileDialog(open_file_callback, &editor, editor.window, NULL, 0, NULL, false);
 
    // Open file button
    SDL_Surface* surface = TTF_RenderText_Solid(editor.font, "Open File", 0, editor.white);
    editor.open_file_texture = SDL_CreateTextureFromSurface(editor.renderer, surface);
    SDL_DestroySurface(surface);
 
    // Save file button
    surface = TTF_RenderText_Solid(editor.font, "Save File", 0, editor.white);
    editor.save_file_texture = SDL_CreateTextureFromSurface(editor.renderer, surface);
    SDL_DestroySurface(surface);
 
    // Main event loop for user input calls handlers at top
    while (editor.running) {
        SDL_Delay(50); // decrease fps
        SDL_GetWindowSize(editor.window, &editor.width, &editor.height);
 
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    editor.running = false;
                    break;
 
                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                    handle_mouse_click(event, editor);
                    break;
 
                case SDL_EVENT_MOUSE_WHEEL:
                    handle_scroll(event, editor);
                    break;
 
                case SDL_EVENT_TEXT_INPUT:
                    if (editor.show_edit_rect) handle_text_input(event, editor);
                    break;
 
                case SDL_EVENT_KEY_DOWN:
                    if (editor.show_edit_rect && event.key.key == SDLK_BACKSPACE && editor.edit_byte.size() > 0) editor.edit_byte.pop_back();
                    else handle_key_down(event, editor);
                    break;
            }
        }
        render_bytes(editor);
        render_cursor(editor);
        render_buttons(editor);
    }
 
    SDL_DestroyTexture(editor.edit_byte_texture);
    SDL_DestroyTexture(editor.open_file_texture);
    SDL_DestroyTexture(editor.save_file_texture);
 
    TTF_CloseFont(editor.font);
    SDL_DestroyRenderer(editor.renderer);
    SDL_DestroyWindow(editor.window);
    TTF_Quit();
    SDL_Quit();
 
    return 0;
}