#include <chip_8.hpp>
#include <platform.hpp>
#include <chrono>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    if(argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <Scale> <Delay> <ROM>\n";
        std::exit(EXIT_FAILURE);
    }

    // Sanity check our args
    std::cout << "Scale: " <<  argv[1] << std::endl;
    std::cout << "Delay: " << argv[2] << std::endl;
    std::cout << "ROM: " << argv[3] << std::endl;
    
    // Parse the critical args!
    int video_scale = std::stoi(argv[1]);
    int cycle_delay = std::stoi(argv[2]);
    const char* ROM_filename = argv[3];

    // Instantiate the SDL platform!
    Platform platform("CHIP-8 Emulator", VIDEO_WIDTH * video_scale, VIDEO_HEIGHT * video_scale, VIDEO_WIDTH, VIDEO_HEIGHT);

    // Instantiate the CHIP-8 and load up the ROM!
    Chip8 chip8;
    chip8.LoadROM(ROM_filename);

    // Load up some other important variables
    int video_pitch = sizeof(chip8.video[0]) * VIDEO_WIDTH;
    auto lastCycleTime = std::chrono::high_resolution_clock::now();
	bool quit = false;

    while (!quit) {
		quit = platform.ProcessInput(chip8.keypad);

		auto currentTime = std::chrono::high_resolution_clock::now();
		float dt = std::chrono::duration<float, std::chrono::milliseconds::period>(currentTime - lastCycleTime).count();

		if (dt > cycle_delay) {
			lastCycleTime = currentTime;

			chip8.Cycle();

			platform.Update(chip8.video, video_pitch);
		}
	}

    return EXIT_SUCCESS;
}