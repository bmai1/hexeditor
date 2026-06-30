#include "editor.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_dialog.h>

#include <fstream>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <cctype>

std::vector<unsigned char> bytes; // A list of bytes from input binary file
std::unordered_set<int> edited_bytes; // A set of indices where bytes have been modified (for rendering)
std::string file_path;

size_t scroll_offset = 0;
size_t visible_rows = 20;

// Set file selected from dialog upon opening
void SDLCALL callback(void* userdata, const char* const* filelist, int filter)
{
    if (!filelist) {
        SDL_Log("Error: %s", SDL_GetError());
        return;
    }

    if (!*filelist) {
        SDL_Log("No file selected.");
        return;
    }

    file_path = *filelist;

    std::ifstream file(*filelist, std::ios::binary);
    bytes.assign((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));
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
size_t get_byte_index(int mouse_x, int mouse_y)
{
    constexpr int TOP_OFFSET = 34;
    constexpr int ROW_H = 16;

    constexpr int HEX_START_X = 110;
    constexpr int HEX_CELL_W = 30;

    int row = (mouse_y - TOP_OFFSET) / ROW_H;
    int col = (mouse_x - HEX_START_X) / HEX_CELL_W;

    if (row < 0 || col < 0 || col >= 16)
        return (size_t)-1;

    size_t index = (row + scroll_offset) * 16 + col;

    return index;
}

// Convert byte_index to hex string
std::string index_to_hex(int byte_index) {
    unsigned char b = bytes[byte_index];

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