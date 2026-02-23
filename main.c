#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
// --- CONFIGURATION ---
// THRESHOLD: Adjust this value by watching the numbers printed in the terminal!
const float VOLUME_THRESHOLD = 5500.0f;

// COOLDOWN: How many milliseconds the image should stay "open" after you stop talking.
// 250ms is a natural value for human speech.
const Uint32 RELEASE_TIME_MS = 250;

// Global variable for the volume (volatile because it's modified by another thread)
volatile float current_volume = 0.0f;

void AudioCallback(void* userdata, Uint8* stream, int len) {
    Sint16* audio_data = (Sint16*)stream;
    int num_samples = len / sizeof(Sint16);
    double sum_squares = 0.0;

    for (int i = 0; i < num_samples; i++) {
        // We use double to prevent overflow during the sum
        sum_squares += (double)(audio_data[i] * audio_data[i]);
    }

    float rms = (float)sqrt(sum_squares / num_samples);

    // Update the global volume
    current_volume = rms;

    // (Optional) Uncomment this line if you want to see the raw numbers in the terminal
    // printf("Raw volume: %.2f\n", rms);
}

int main(int argc, char* argv[]) {
    // 1. INITIALIZATION

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        printf("SDL Error: %s\n", SDL_GetError());
        return -1;
    }
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        printf("IMG Error: %s\n", IMG_GetError());
        return -1;
    }

    // 2. LOADING IMAGES
    // Make sure to rename your image files to match these!
    SDL_Surface* silentImage_data = IMG_Load("silent.png");
    SDL_Surface* talkingImage_data = IMG_Load("talking.png");

    int imageWidth = silentImage_data->w;
    int imageHeight = silentImage_data->h;

    if (!silentImage_data || !talkingImage_data) {
        printf("ERROR: Images not found! Make sure silent.png and talking.png exist.\n");
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow("yaptubers",
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          imageHeight, imageWidth, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    SDL_Texture* tex_silence = SDL_CreateTextureFromSurface(renderer, silentImage_data);
    SDL_Texture* tex_noise = SDL_CreateTextureFromSurface(renderer, talkingImage_data);
    SDL_FreeSurface(silentImage_data);
    SDL_FreeSurface(talkingImage_data);

    // 3. AUDIO SETUP
    SDL_AudioSpec spec;
    SDL_zero(spec);
    spec.freq = 44100;
    spec.format = AUDIO_S16SYS;
    spec.channels = 1;
    spec.samples = 1024; // Small buffer for high responsiveness
    spec.callback = AudioCallback;

    SDL_AudioDeviceID dev = SDL_OpenAudioDevice(NULL, 1, &spec, NULL, 0);
    if (dev == 0) {
        printf("Microphone Error: %s\n", SDL_GetError());
        return -1;
    }
    SDL_PauseAudioDevice(dev, 0);

    // 4. MAIN LOOP
    bool running = true;
    SDL_Event event;

    // Variable to handle the image "cooldown"
    Uint32 time_last_noise = 0;

    // Timer to limit console prints (so we don't spam the terminal)
    Uint32 debug_timer = 0;

    printf("\n--- START ---\n");

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
        }

        // Read the current volume
        float vol = current_volume;

        // IMAGE SWITCHING LOGIC
        // If the volume is high, update the timestamp of the last detected noise
        if (vol > VOLUME_THRESHOLD) {
            time_last_noise = SDL_GetTicks();
        }

        // Calculate how much time has passed since the last loud noise
        Uint32 current_time = SDL_GetTicks();
        bool show_noise = (current_time - time_last_noise) < RELEASE_TIME_MS;

        // DEBUG IN CONSOLE (Every 200ms)
        if (current_time > debug_timer + 200) {
            printf("Volume: %6.1f | Threshold: %.1f | Status: %s\n",
                   vol, VOLUME_THRESHOLD, show_noise ? "TALKING" : "SILENCE");
            debug_timer = current_time;
        }

        // RENDER
        SDL_RenderClear(renderer);
        if (show_noise) {
            SDL_RenderCopy(renderer, tex_noise, NULL, NULL);
        } else {
            SDL_RenderCopy(renderer, tex_silence, NULL, NULL);
        }
        SDL_RenderPresent(renderer);
    }

    // CLEANUP
    SDL_CloseAudioDevice(dev);
    SDL_DestroyTexture(tex_silence);
    SDL_DestroyTexture(tex_noise);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();

    return 0;
}
