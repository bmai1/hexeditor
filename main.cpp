#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_dialog.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <string>
#include <sstream>
#include <fstream>


std::vector<unsigned char> bytes;
std::string file_path;

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
    oss << std::setw(8) << std::setfill('0') << std::hex << offset << "  ";

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

    size_t index = row * 16 + col;

    return index;
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
    bool showCursorRect = false;

    SDL_FRect cursorRect;
    SDL_FRect asciiRect;

    SDL_FRect saveFileRect = {0, 0, 100, 20};
    SDL_Surface* saveFileSurface = TTF_RenderText_Solid(font, "Save File", 0, white);
    SDL_Texture* saveFileTexture = SDL_CreateTextureFromSurface(renderer, saveFileSurface);
    SDL_DestroySurface(saveFileSurface);

    size_t byte_index;
    size_t prev_byte_index = -1; // To track double clicks on bytes
    bool showEditRect = false;
    SDL_FRect editRect; 
    std::string editByte = "00";
    SDL_Surface* editByteSurface;
    SDL_Texture* editByteTexture;

    while (running) {
        SDL_Delay(50); // decrease fps 
        SDL_GetWindowSize(window, &width, &height);

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
            else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    float click_x = event.button.x;
                    float click_y = event.button.y;

                    // Clicked save file button
                    if (click_x > width - 120 && click_x < width - 20 && click_y > height - 40 && click_y < height - 20) {
                        SDL_Log("Saved file");
                        save_file(file_path, bytes);
                    }
                    else {
                        byte_index = get_byte_index((int) click_x, (int) click_y);
                        
                        if (byte_index == prev_byte_index) {
                            if (!showEditRect) {
                                showEditRect = true;
                                SDL_StartTextInput(window);
                                editRect = {(float) 110+30*(byte_index%16), (float) 30+16*(byte_index/16), 20, 16};
                            }
                            else {
                                showEditRect = false;
                                editByte = "00";
                                SDL_StopTextInput(window);
                            }
                        }
                        if (byte_index != (size_t)-1) {
                            if (byte_index != prev_byte_index) {
                                showEditRect = false;
                                editByte = "00";
                                SDL_StopTextInput(window);
                            }
                            SDL_Log("Mouse left-clicked at: %f, %f, which corresponds to byte index %zu", click_x, click_y, byte_index);
                            // Calculated based on offsets in get_byte_index
                            cursorRect = {(float) 110+30*(byte_index%16), (float) 31+16*(byte_index/16), 20, 14};
                            asciiRect  = {(float) 600+10*(byte_index%16), (float) 31+16*(byte_index/16), 10, 14};
                            if (!showCursorRect) {
                                showCursorRect = true;
                            }
                            else {
                                // showCursorRect = false;
                            }
                            prev_byte_index = byte_index;
                        }
                        else {
                            SDL_Log("Mouse left-clicked outside of editor space");
                        }
                    }
                }
                else {
                    SDL_Log("Mouse right-clicked");
                }
            }
            else if (event.type == SDL_EVENT_TEXT_INPUT && showEditRect) {
                SDL_Log("Typed: %s", event.text.text);
                editByte.erase(0, 1); // pop first char
                editByte += event.text.text; 
            }
            else if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_BACKSPACE) {
                editByte = "0" + editByte;
                editByte.pop_back();
            }
            else if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_RETURN && showEditRect) {
                if (byte_index < bytes.size()) {
                    bytes[byte_index] = (unsigned char) std::stoi(editByte, nullptr, 16);
                }
                showEditRect = false;
                editByte = "00";
                SDL_StopTextInput(window);
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        if (!bytes.empty()) {

            int line_height = 16;
            size_t total_rows = (bytes.size() + 15) / 16;

            for (size_t row = 0; row <= total_rows && row < 20; ++row) {
                std::string line;

                if (row == 0) {
                    std::ostringstream data_label;
                    data_label << std::left
                               << std::setw(10) << "Address"
                               << std::setw(49) << "Hexadecimal"
                               << "ASCII";
                    line = data_label.str();
                }
                else {
                    line = format_row(bytes, (row - 1) * 16);
                }
                const char* lp = line.c_str();

                SDL_Surface* surface = TTF_RenderText_Solid(font, lp, 0, white);
                SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);

                SDL_FRect dst = {
                    10.0f,
                    10.0f + row * line_height,
                    (float) surface->w,
                    (float) surface->h
                };

                SDL_RenderTexture(renderer, texture, NULL, &dst);

                SDL_DestroyTexture(texture);
                SDL_DestroySurface(surface);
            }
        }

        // Render cursor rectangle
        if (showCursorRect) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 128);
            SDL_RenderFillRect(renderer, &cursorRect);
            SDL_RenderFillRect(renderer, &asciiRect);
        }

        // Blue highlight when editing byte
        if (showEditRect) {
            SDL_SetRenderDrawColor(renderer, 0, 100, 255, 255);
            SDL_RenderFillRect(renderer, &editRect);

            // Edited byte label
            editByteSurface = TTF_RenderText_Solid(font, editByte.c_str(), 0, white);
            editByteTexture = SDL_CreateTextureFromSurface(renderer, editByteSurface);
            SDL_DestroySurface(editByteSurface);
            SDL_FRect dst = {
                editRect.x,
                editRect.y - 4,
                20,
                24,
            };
            SDL_RenderTexture(renderer, editByteTexture, NULL, &dst);
        }

        // Save button
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 64);
        saveFileRect.x = width - 120;
        saveFileRect.y = height - 40;
        SDL_RenderFillRect(renderer, &saveFileRect);

        // Save button label
        SDL_FRect dst = { saveFileRect.x + 10, saveFileRect.y - 3, 80, 24 };
        SDL_RenderTexture(renderer, saveFileTexture, NULL, &dst);

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(editByteTexture);
    SDL_DestroyTexture(saveFileTexture);

    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}


