#include "editor.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_dialog.h>

#include <fstream>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <cctype>

// Set file selected from dialog upon opening
void SDLCALL open_file_callback(void* userdata, const char* const* filelist, int filter)
{
    EditorState* editor = static_cast<EditorState*>(userdata);

    if (!filelist) {
        SDL_Log("Error: %s", SDL_GetError());
        return;
    }
    if (!*filelist) {
        SDL_Log("No file selected.");
        return;
    }

    editor->file_path = *filelist;

    std::ifstream file(*filelist, std::ios::binary);
    editor->bytes.assign((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));

    // Reset cursor/scroll/edit state for the newly loaded file
    editor->byte_index = (size_t)-1;
    editor->prev_byte_index = (size_t)-1;
    editor->scroll_offset = 0;
    editor->edited_bytes.clear();
    editor->show_cursor_rect = false;
    editor->show_edit_rect = false;

    SDL_Log("Loaded file: %s (%zu bytes)", editor->file_path.c_str(), editor->bytes.size());
}

// Format bytes into Offset Hex ASCII string
std::string format_row(const std::vector<unsigned char>& bytes, size_t offset) {
    std::ostringstream oss;

    // Offset (8 hex digits)
    oss << std::setw(8) << std::setfill('0') << std::hex << std::uppercase << offset << "  ";

    // Hex bytes (16 per row)
    for (int i = 0; i < 16; ++i) {
        if (offset + i < bytes.size()) {
            oss << std::setw(2)
                << std::setfill('0')
                << std::hex
                << std::uppercase
                << static_cast<int>(bytes[offset + i])
                << " ";
        } else {
            oss << "   ";
        }
    }
    oss << " ";

    // ASCII
    for (int i = 0; i < 16; ++i) {
        if (offset + i < bytes.size()) {
            unsigned char c = bytes[offset + i];
            if (std::isprint(c)) {
                oss << c;
            } else {
                oss << '.';
            }
        }
    }
    return oss.str();
}

// Get byte index based on where the mouse clicks on the window
size_t get_byte_index(EditorState& editor, int mouse_x, int mouse_y)
{
    constexpr int TOP_OFFSET = 34;
    constexpr int ROW_H = 16;
    constexpr int HEX_START_X = 110;
    constexpr int HEX_CELL_W = 30;

    int row = (mouse_y - TOP_OFFSET) / ROW_H;
    int col = (mouse_x - HEX_START_X) / HEX_CELL_W;

    if (row < 0 || col < 0 || col >= 16)
        return (size_t)-1;

    size_t index = (row + editor.scroll_offset) * 16 + col;
    return index;
}

// Convert byte_index to hex string
std::string index_to_hex(EditorState& editor, size_t byte_index) {
    unsigned char b = editor.bytes[byte_index];
    std::ostringstream oss;
    oss << std::hex << std::uppercase << std::setw(2)
        << std::setfill('0')
        << (int)b;
    return oss.str();
}

bool save_file(const std::string& file_path, const std::vector<unsigned char>& bytes)
{
    std::ofstream file(file_path, std::ios::binary);
    if (!file) return false;
    file.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    return true;
}