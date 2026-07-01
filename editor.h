#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_dialog.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <string>
#include <vector>
#include <unordered_set>

struct EditorState {
    int width = 0, height = 0;
    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;

    bool running = true;

    bool show_cursor_rect = false;
    bool show_edit_rect = false;

    size_t byte_index = (size_t)-1;
    size_t prev_byte_index = (size_t)-1;

    size_t scroll_offset = 0;
    size_t visible_rows = 20;

    std::string file_path;
    std::vector<unsigned char> bytes;
    std::unordered_set<size_t> edited_bytes;

    SDL_FRect open_file_rect{0, 0, 100, 20};
    SDL_FRect save_file_rect{0, 0, 100, 20};

    SDL_FRect cursor_rect{};
    SDL_FRect ascii_rect{};
    SDL_FRect edit_rect{};

    std::string edit_byte = "00";
    SDL_Texture* edit_byte_texture = nullptr;

    TTF_Font* font = nullptr;
    SDL_Color white{255, 255, 255, 255};

    SDL_Texture* open_file_texture = nullptr;
    SDL_Texture* save_file_texture = nullptr;
};

// Callback for SDL_ShowOpenFileDialog. Pass &editor as userdata; loads the
// selected file's bytes directly into editor.bytes and resets cursor state.
void SDLCALL open_file_callback(void* userdata, const char* const* filelist, int filter);

// Format 16 bytes starting at `offset` into "OFFSET  HEX...  ASCII" form.
std::string format_row(const std::vector<unsigned char>& bytes, size_t offset);

// Map a mouse position to a byte index, using editor.scroll_offset.
// Returns (size_t)-1 if the click falls outside the hex grid.
size_t get_byte_index(EditorState& editor, int mouse_x, int mouse_y);

// Convert editor.bytes[byte_index] to a 2-character uppercase hex string.
std::string index_to_hex(EditorState& editor, size_t byte_index);

// Write bytes to disk at file_path. Returns false on failure to open the file.
bool save_file(const std::string& file_path, const std::vector<unsigned char>& bytes);