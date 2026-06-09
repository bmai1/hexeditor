#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_dialog.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <string>
#include <sstream>
#include <fstream>
#include <unordered_set>

std::vector<unsigned char> bytes; // A list of bytes from input binary file
std::unordered_set<int> edited_bytes; // A set of indices where bytes have been modified (for rendering)
std::string file_path;

size_t scroll_offset = 0;
size_t visible_rows = 20;

// Set file selected from dialog upon opening
static void SDLCALL callback(void* userdata, const char* const* filelist, int filter)
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

int main(int argc, char* argv[]) {
    int width, height;
    SDL_Window *window;
    SDL_Renderer *renderer;            

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s", SDL_GetError());
        return 1;
    }

    if (!SDL_CreateWindowAndRenderer("Hex Editor", 780, 480, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window and renderer: %s", SDL_GetError());
        return 1;
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND); // for opacity

    // Init font
    TTF_Init();
    TTF_Font* font = TTF_OpenFont("SpaceMono.ttf", 16);
    SDL_Color white = {255, 255, 255, 255}; // font color

    // Prompt for binary file
    SDL_ShowOpenFileDialog(callback, NULL, window, NULL, 0, NULL, false);

    bool running = true;
    bool show_cursor_rect = false;

    // Highlights bytes/ASCII when selected
    SDL_FRect cursor_rect;
    SDL_FRect ascii_rect;

    // Open file button
    SDL_FRect open_file_rect = {0, 0, 100, 20};
    SDL_Surface* open_file_surface = TTF_RenderText_Solid(font, "Open File", 0, white);
    SDL_Texture* open_file_texture = SDL_CreateTextureFromSurface(renderer, open_file_surface);
    SDL_DestroySurface(open_file_surface);

    // Save file button
    SDL_FRect save_file_rect = {0, 0, 100, 20};
    SDL_Surface* save_file_surface = TTF_RenderText_Solid(font, "Save File", 0, white);
    SDL_Texture* save_file_texture = SDL_CreateTextureFromSurface(renderer, save_file_surface);
    SDL_DestroySurface(save_file_surface);

    size_t byte_index = -1;
    size_t prev_byte_index = -1;

    // Displays incoming edit when double-clicking byte
    bool show_edit_rect = false;
    SDL_FRect edit_rect; 
    std::string edit_byte = "00"; 
    SDL_Surface* edit_byte_surface;
    SDL_Texture* edit_byte_texture;

    while (running) {
        SDL_Delay(50); // decrease fps 
        SDL_GetWindowSize(window, &width, &height);

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
            // Handle mouse clicks
            else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    float click_x = event.button.x;
                    float click_y = event.button.y;

                    // Clicked save file button
                    if (click_x > width - 120 && click_x < width - 20 && click_y > height - 40 && click_y < height - 20) {
                        SDL_Log("Saved file");
                        save_file(file_path, bytes);
                    }
                    // Clicked open file button
                    else if (click_x > width - 120 && click_x < width - 20 && click_y > height - 70 && click_y < height - 50) {
                        SDL_Log("Opened file");
                        SDL_ShowOpenFileDialog(callback, NULL, window, NULL, 0, NULL, false);
                    }
                    else {
                        // Handle highlighting bytes based on left clicks
                        byte_index = get_byte_index((int) click_x, (int) click_y);
                        
                        bool valid = (byte_index >= 0 && byte_index < bytes.size());

                        // If invalid, reset states
                        if (!valid) {
                            SDL_Log("Mouse left-clicked outside of editor space");
                            show_cursor_rect = false;
                            show_edit_rect = false;
                            SDL_StopTextInput(window);
                        }
                        else {
                            SDL_Log("Mouse left-clicked at: %f, %f, which corresponds to byte index %zu", click_x, click_y, byte_index);
                            
                            // IF IN DEFAULT OR EDIT MODE AND LEFT CLICK -> ENTER CURSOR MODE
                            if (!show_cursor_rect) { 
                                show_cursor_rect = true;
                                show_edit_rect = false;
                                SDL_StopTextInput(window);
                            }
                            // IF IN CURSOR MODE AND LEFT CLICK SAME BYTE -> ENTER EDIT MODE
                            else if (byte_index == prev_byte_index) {
                                edit_byte = index_to_hex(byte_index);
                                show_cursor_rect = false;
                                show_edit_rect = true;
                                SDL_StartTextInput(window);
                            } 

                            // (If in cursor mode and left click new byte, nothing happens besides the rectangles being updated)
                            cursor_rect = {(float) 110+30*(byte_index%16), (float)(31 + 16*((int)(byte_index/16) - (int)scroll_offset)), 20, 14};
                            edit_rect   = {(float) 110+30*(byte_index%16), (float)(30 + 16*((int)(byte_index/16) - (int)scroll_offset)), 20, 16};
                            ascii_rect  = {(float) 600+10*(byte_index%16), (float)(31 + 16*((int)(byte_index/16) - (int)scroll_offset)), 10, 14};
                            
                            prev_byte_index = byte_index;
                        }
                    }
                }
                else {
                    SDL_Log("Mouse right-clicked");
                }
            }
            // Handle scrolling
            else if (event.type == SDL_EVENT_MOUSE_WHEEL) {
                // std::string wheel_y = std::to_string(event.wheel.y);
                // SDL_Log("%s", wheel_y.c_str());
                size_t total_rows = (bytes.size() + 15) / 16;

                if (event.wheel.y < 0) { // scroll down
                    if (scroll_offset + visible_rows < total_rows)
                        scroll_offset++;
                }
                else if (event.wheel.y > 0) { // scroll up
                    if (scroll_offset > 0)
                        scroll_offset--;
                }
            }
            // Handle keyboard input for editing bytes
            else if (event.type == SDL_EVENT_TEXT_INPUT && show_edit_rect) {
                SDL_Log("Typed: %s", event.text.text);
                if (std::isalnum(event.text.text[0])) {
                    if (edit_byte.size() == 2) {
                        edit_byte = event.text.text;
                    }
                    else if (edit_byte.size() < 2) {
                        edit_byte += event.text.text; 
                    }
                }
            }
            // Handle backspace when editing
            else if (event.type == SDL_EVENT_KEY_DOWN && show_edit_rect && event.key.key == SDLK_BACKSPACE) {
                if (edit_byte.size() > 0) {
                    edit_byte.pop_back();
                }
            }
            // Handle other keyboard inputs (byte editing, movement)
            else if (event.type == SDL_EVENT_KEY_DOWN) {
                bool edited = false;

                // Handle enter key: enter edit byte mode if cursor is active OR update the byte array (not saving to the file yet)
                if (event.key.key == SDLK_RETURN && edit_byte.size() == 2) { // Enfore 2 char length for hex byte
                    if (show_cursor_rect && !show_edit_rect) {
                        show_cursor_rect = false;
                        show_edit_rect = true;
                        SDL_StartTextInput(window);
                        edit_rect = {(float) 110+30*(byte_index%16), (float)(30 + 16*((int)(byte_index/16) - (int)scroll_offset)), 20, 16};
                    }
                    else if (show_edit_rect) {
                        if (byte_index >= 0 && byte_index < bytes.size()) {
                            bytes[byte_index] = (unsigned char) std::stoi(edit_byte, nullptr, 16);
                            edited_bytes.insert(byte_index); // Render edited bytes in red
                        }
        
                        // Automatically move to editing next byte (re-render rects) instead of exiting edit mode
                        if (byte_index < bytes.size() - 1) {
                            byte_index++;
                            edited = true;
                        }
                    }
                }
                // Handle arrow key cursor movement (note: does not update byte array)
                else if (event.key.key == SDLK_UP && (show_cursor_rect || show_edit_rect) && byte_index >= 16 && byte_index < bytes.size()) {
                    byte_index -= 16;
                    edited = true;
                }
                else if (event.key.key == SDLK_DOWN && (show_cursor_rect || show_edit_rect) && byte_index >= 0 && byte_index < bytes.size() - 16) {
                    // Need to fix a bug where you can go down past editor box if there is more lines not visible below

                    // size_t current_row = byte_index / 16;
                    // if (current_row >= scroll_offset + visible_rows) {
                    //     scroll_offset++;
                    // }
                    byte_index += 16;
                    edited = true;
                }
                else if (event.key.key == SDLK_LEFT && (show_cursor_rect || show_edit_rect) && byte_index > 0 && byte_index < bytes.size()) {
                    byte_index--;
                    edited = true;
                }
                else if (event.key.key == SDLK_RIGHT && (show_cursor_rect || show_edit_rect) && byte_index >= 0 && byte_index < bytes.size() - 1) {
                    byte_index++;
                    edited = true;
                }

                // Re-render cursor rectangles
                if (byte_index != -1) {
                    if (edited) edit_byte = index_to_hex(byte_index);
                    edit_rect   = {(float) 110+30*(byte_index%16), (float)(30 + 16*((int)(byte_index/16) - (int)scroll_offset)), 20, 16};
                    cursor_rect = {(float) 110+30*(byte_index%16), (float)(31 + 16*((int)(byte_index/16) - (int)scroll_offset)), 20, 14};
                    ascii_rect  = {(float) 600+10*(byte_index%16), (float)(31 + 16*((int)(byte_index/16) - (int)scroll_offset)), 10, 14};
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Render red highlights underneath edited bytes
        for (int edited_byte_index : edited_bytes) {
            int screen_row = (edited_byte_index / 16) - scroll_offset;
            if (screen_row < 0 || screen_row >= (int) visible_rows) {
                continue;
            }
            SDL_FRect edited_rect = {(float) 110+30*(edited_byte_index%16), (float) 31+16*screen_row, 20, 14};
            SDL_FRect edited_ascii_rect  = {(float) 600+10*(edited_byte_index%16), (float) 31+16*screen_row, 10, 14};
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 192);
            SDL_RenderFillRect(renderer, &edited_rect);
            SDL_RenderFillRect(renderer, &edited_ascii_rect);
        }

        // Render bytes as lines
        if (!bytes.empty()) {
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
                    TTF_RenderText_Solid(font, data_label.str().c_str(), 0, white);

                SDL_Texture* texture =
                    SDL_CreateTextureFromSurface(renderer, surface);

                SDL_FRect dst = {
                    10.0f,
                    (float)header_y,
                    (float)surface->w,
                    (float)surface->h
                };

                SDL_RenderTexture(renderer, texture, NULL, &dst);

                SDL_DestroyTexture(texture);
                SDL_DestroySurface(surface);
            }

            size_t total_rows = (bytes.size() + 15) / 16;

            // Render visible data rows
            for (size_t i = 0; i < visible_rows; ++i) {
                size_t row = scroll_offset + i;

                if (row >= total_rows)
                    break;

                std::string line = format_row(bytes, row * 16);

                SDL_Surface* surface =
                    TTF_RenderText_Solid(font, line.c_str(), 0, white);

                SDL_Texture* texture =
                    SDL_CreateTextureFromSurface(renderer, surface);

                SDL_FRect dst = {
                    10.0f,
                    (float)(header_y + line_height + i * line_height),
                    (float)surface->w,
                    (float)surface->h
                };

                SDL_RenderTexture(renderer, texture, NULL, &dst);

                SDL_DestroyTexture(texture);
                SDL_DestroySurface(surface);
            }
        }

        // Render cursor rectangle
        if (show_cursor_rect) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 128);
            SDL_RenderFillRect(renderer, &cursor_rect);
            SDL_RenderFillRect(renderer, &ascii_rect);
        }

        // Blue highlight when editing byte
        if (show_edit_rect) {
            SDL_SetRenderDrawColor(renderer, 60, 77, 145, 255);
            SDL_RenderFillRect(renderer, &edit_rect);

            // Edited byte label
            edit_byte_surface = TTF_RenderText_Solid(font, edit_byte.c_str(), 0, white);
            edit_byte_texture = SDL_CreateTextureFromSurface(renderer, edit_byte_surface);
            SDL_DestroySurface(edit_byte_surface);
            SDL_FRect dst = {
                edit_rect.x,
                edit_rect.y - 4,
                10 * (float) edit_byte.size(), // So the numbers don't change width when typing
                24,
            };
            SDL_RenderTexture(renderer, edit_byte_texture, NULL, &dst);
        }

        // Render buttons in bottom right
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 64);
        // Open new file button
        open_file_rect.x = width - 120;
        open_file_rect.y = height - 70;
        SDL_RenderFillRect(renderer, &open_file_rect);
        SDL_FRect dst = { open_file_rect.x + 10, open_file_rect.y - 3, 80, 24 };
        SDL_RenderTexture(renderer, open_file_texture, NULL, &dst);

        // Save file button
        save_file_rect.x = width - 120;
        save_file_rect.y = height - 40;
        SDL_RenderFillRect(renderer, &save_file_rect);
        dst = { save_file_rect.x + 10, save_file_rect.y - 3, 80, 24 };
        SDL_RenderTexture(renderer, save_file_texture, NULL, &dst);

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(edit_byte_texture);
    SDL_DestroyTexture(open_file_texture);
    SDL_DestroyTexture(save_file_texture);

    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();

    return 0;
}


