#include <fstream>
#include <chrono>
#include <random>
#include <chip_8.hpp>

const unsigned int FONTSET_SIZE = 80; // 16 chars * 5 bytes = 80 byte array
const unsigned int START_ADDRESS = 0x200; // Starting address for all CHIP-8 ROMS
const unsigned int FONTSET_ADDRESS = 0x50; // Address of the fontset (within the reserved CHIP-8 memory)

/**
 * Each number/letter is represented by a series of five
 * hexadecimal numbers, where a 1 represents a pixel that
 * is on, and a 0 represents a pixel that is off.
 * Ex. for zero (0xF0, 0x90, 0x90, 0x90, 0xF0)
 * 1111 0000 // 0xF0
 * 1001 0000 // 0x90
 * 1001 0000 // 0x90
 * 1001 0000 // 0x90
 * 1111 0000 // 0xF0
*/

uint8_t fontset[FONTSET_SIZE] = {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
		0x20, 0x60, 0x20, 0x20, 0x70, // 1
		0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
		0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
		0x90, 0x90, 0xF0, 0x10, 0x10, // 4
		0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
		0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
		0xF0, 0x10, 0x20, 0x40, 0x40, // 7
		0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
		0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
		0xF0, 0x90, 0xF0, 0x90, 0x90, // A
		0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
		0xF0, 0x80, 0x80, 0x80, 0xF0, // C
		0xE0, 0x90, 0x90, 0x90, 0xE0, // D
		0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
		0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

Chip8::Chip8() 
: rand_gen(std::chrono::system_clock::now().time_since_epoch().count()){
    pc = START_ADDRESS; // Initialize the PC
    randByte = std::uniform_int_distribution<uint8_t>(0, 255U); // Initialize the RNG

    // Load the fontset into memory
    for(int i = 0; i < FONTSET_SIZE; i++) {
        memory[FONTSET_ADDRESS + i] = fontset[i];
    }
}

void Chip8::LoadROM(const char* filename) {
    // Open the ROM as a stream of binary, and move the file pointer to the very end
    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    if(file.is_open()) {
        // Get the size of the file and allocate a buffer to hold the contents
        std::streampos size = file.tellg(); // Where are we currently (i.e. size of the file)
        char* rom_buffer = new char[size]; // Allocate an appropriately sized buffer


        // Go back to the beginning of the file and fill the buffer
        file.seekg(0, std::ios::beg); // Go back to the beginning
        file.read(rom_buffer, size); // Read the ROM into the buffer
        file.close(); // Clean up the stream

        // Now load the ROM's contents into the CHIP-8 memory, starting at 0x200
        for(long i = 0; i < size; ++i) {
            memory[START_ADDRESS + i] = rom_buffer[i];
        }

        // Free the buffer
        delete[] rom_buffer;
    }
}

