#include <fstream>
#include <chip_8.hpp>

using namespace std;

const unsigned int START_ADDRESS = 0x200; // Starting address for all CHIP-8 ROMS



Chip8::Chip8() {
    pc = START_ADDRESS; // Inialize the PC
}

