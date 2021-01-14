#include <fstream>
#include <chip_8.hpp>


const unsigned int START_ADDRESS = 0x200; // Starting address for all CHIP-8 ROMS

Chip8::Chip8() {
    pc = START_ADDRESS; // Inialize the PC
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

