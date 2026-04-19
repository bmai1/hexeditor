#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_dialog.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <string>
#include <sstream>
#include <fstream>


std::vector<unsigned char> bytes;

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

    std::ifstream file(*filelist, std::ios::binary);
    bytes.assign((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));
}

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

int main(int argc, char* argv[]) {
    SDL_Window *window;
    SDL_Renderer *renderer;            

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s", SDL_GetError());
        return 1;
    }

    if (!SDL_CreateWindowAndRenderer("Hex Editor", 640, 480, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window and renderer: %s", SDL_GetError());
        return 1;
    }

    // Init font
    TTF_Init();
    TTF_Font* font = TTF_OpenFont("Roboto.ttf", 16);

    // Prompt for binary file
    SDL_ShowOpenFileDialog(callback, NULL, window, NULL, 0, NULL, false);

    bool running = true;

    while (running) {
        SDL_Delay(50); // decrease fps 

        SDL_Event event;
        SDL_PollEvent(&event);
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT)
                running = false;
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        if (!bytes.empty()) {
        SDL_Color color = {255, 255, 255, 255};

        int line_height = 16;
        size_t total_rows = (bytes.size() + 15) / 16;

        for (size_t row = 0; row < total_rows && row < 20; ++row) {
            std::string line = format_row(bytes, row * 16);
            const char* lp = line.c_str();

            SDL_Surface* surface = TTF_RenderText_Solid(font, lp, strlen(lp), color);
            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);

            SDL_FRect dst = {
                10.0f,
                10.0f + row * line_height,
                (float)surface->w,
                (float)surface->h
            };

            SDL_RenderTexture(renderer, texture, NULL, &dst);

            SDL_DestroyTexture(texture);
            SDL_DestroySurface(surface);
        }
    }

        SDL_RenderPresent(renderer);
    }

    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}


