#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_dialog.h>

#include <vector>
#include <unordered_set>
#include <string>

extern std::vector<unsigned char> bytes;
extern std::unordered_set<int> edited_bytes;
extern std::string file_path;

extern size_t scroll_offset;
extern size_t visible_rows;

void SDLCALL callback(void* userdata, const char* const* filelist, int filter);

std::string format_row(const std::vector<unsigned char>& bytes, size_t offset);
size_t get_byte_index(int mouse_x, int mouse_y);
std::string index_to_hex(int byte_index);
bool save_file(const std::string& file_path,
               const std::vector<unsigned char>& bytes);