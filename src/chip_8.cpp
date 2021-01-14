#include <fstream>
#include <chrono>
#include <random>
#include <cstring>
#include <chip_8.hpp>

const unsigned int FONTSET_SIZE = 80; // 16 chars * 5 bytes = 80 byte array
const unsigned int START_ADDRESS = 0x200; // Starting address for all CHIP-8 ROMS
const unsigned int FONTSET_ADDRESS = 0x50; // Address of the fontset (within the reserved CHIP-8 memory)

/**
 * Each number/letter is represented by a series of five
 * hexadecimal numbers, where a 1 represents a pixel that
 * is on, and a 0 represents a pixel that is off.
 * Ex. for zero (0xF0, 0x90, 0x90, 0x90, 0xF0)
 * 1111 0000 // 0xF0 ****
 * 1001 0000 // 0x90 *  *
 * 1001 0000 // 0x90 *  *
 * 1001 0000 // 0x90 *  *
 * 1111 0000 // 0xF0 ****
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

/**
 * OPCODES!
 * The CHIP-8 *only* has 34 opcodes, which are implemented below.
 */

// CLS: Clear the display
void Chip8::OP_00E0() { 
    memset(video, 0, sizeof(video)); // CLS: Set the entire video buffer to zero (clear the screen!)
} 

// RET: Return from a subroutine
void Chip8::OP_00EE() {
    sp--; // Decrement the stack pointer
    pc = stack[sp]; // Set the PC to the new stack frame
}

// JP addr: Jump to location nnn
void Chip8::OP_1nnn() {
    uint16_t address = opcode & 0x0FFFu; // Parse out the address
    pc = address; // Set the PC to the address we're jumping to
}

// CALL addr: Call subroutine at nnn
void Chip8::OP_2nnn() {
    uint16_t address = opcode & 0x0FFFu; // Parse out the address
    stack[sp] = pc; // Store the PC on the stack
    sp++; // Increment the stack pointer
    pc = address; // Set the PC to the address we're jumping to
} 

// SE Vx, byte: Skip next instruction if Vx == kk
void Chip8::OP_3xkk() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u; // Parse register number using bitmask
	uint8_t byte = opcode & 0x00FFu; // Parse byte using bitmask

	if (registers[Vx] == byte) { // If the value at register Vx == the byte in question
		pc += 2; // Skip the next instruction (increment PC by *TWO*)
	}
} 

// SNE Vx, byte: Skip next instruction if Vx != kk
void Chip8::OP_4xkk() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u; // Parse register number using bitmask
	uint8_t byte = opcode & 0x00FFu; // Parse byte using bitmask

	if (registers[Vx] != byte) { // If the value at register Vx != the byte in question
		pc += 2; // Skip the next instruction (increment PC by *TWO*)
	}
}

// SE Vx, Vy: Skip next instruction if Vx == Vy
void Chip8::OP_5xy0(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u; // Parse Vx reg number using bitmask
    uint8_t Vy = (opcode & 0x00F0u) >> 4u; // Parse Vy reg number using bitmask

    if(registers[Vx] == registers[Vy]) { // If the value at register Vx == value at register Vy
        pc += 2; // Skip the next instruction
    }
} 

// LD Vx, byte: Set Vx = kk
void Chip8::OP_6xkk() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u; // Parse register number using bitmask
	uint8_t byte = opcode & 0x00FFu; // Parse byte using bitmask

    registers[Vx] = byte; // Set the value at Vx equal to the byte
} 

// ADD Vx, byte: Set Vx = Vx + kk
void Chip8::OP_7xkk() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u; // Parse register number using bitmask
	uint8_t byte = opcode & 0x00FFu; // Parse byte using bitmask

    registers[Vx] += byte; // Set the Vx = Vx + byte
} 

// LD Vx, Vy: Set Vx = Vy
void Chip8::OP_8xy0() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u; // Parse Vx reg number using bitmask
    uint8_t Vy = (opcode & 0x00F0u) >> 4u; // Parse Vy reg number using bitmask

    registers[Vx] = registers[Vx]; // Set Vx = Vy
} 

// OR Vx, Vy: Set Vx = Vx OR Vy
void Chip8::OP_8xy1() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u; // Parse Vx reg number using bitmask
    uint8_t Vy = (opcode & 0x00F0u) >> 4u; // Parse Vy reg number using bitmask

    registers[Vx] |= registers[Vx]; // Set Vx |= Vy
} 

// AND Vx, Vy: Set Vx = Vx AND 
void Chip8::OP_8xy2() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u; // Parse Vx reg number using bitmask
    uint8_t Vy = (opcode & 0x00F0u) >> 4u; // Parse Vy reg number using bitmask

    registers[Vx] &= registers[Vx]; // Set Vx &= Vy
} 

// XOR Vx, Vy: Set Vx = Vx XOR Vy
void Chip8::OP_8xy3() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u; // Parse Vx reg number using bitmask
    uint8_t Vy = (opcode & 0x00F0u) >> 4u; // Parse Vy reg number using bitmask

    registers[Vx] ^= registers[Vx]; // Set Vx ^= Vy
} 

// ADD Vx, Vy: Set Vx = Vx + Vy, set VF = carry. (VF is overflow flag)
void Chip8::OP_8xy4() {

} 

// SUB Vx, Vy: Set Vx = Vx - Vy, set VF = carry. (VF is underflow flag)
void Chip8::OP_8xy5() {

} 

// SHR Vx: Set Vx = Vx SHR 1. (Right shift, save remainder in VF)
void Chip8::OP_8xy6() {

} 

// SUBN Vx, Vy: Set Vx = Vy - Vx, set VF = NOT borrow
void Chip8::OP_8xy7() {

} 

// SHL Vx {, Vy}: Set Vx = Vx SHL 1. (Left shift, save MSB in VF)
void Chip8::OP_8xyE() {

} 

// SNE Vx, Vy: Skip next instruction if Vx != Vy
void Chip8::OP_9xy0() {

} 

// LD I, addr: Set I = nnn
void Chip8::OP_Annn() {

} 

// JP V0, addr: Jump to location nnn + V0
void Chip8::OP_Bnnn() {

}

// RND Vx, byte: Set Vx = random byte AND kk
void Chip8::OP_Cxkk() {

} 

// DRW Vx, Vy, nibble: Display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision.
void Chip8::OP_Dxyn() {

} 

// SKP Vx: Skip next instruction if key with the value of Vx is pressed
void Chip8::OP_Ex9E() {

} 

// SKNP Vx: Skip next instruction if key with the value of Vx is NOT pressed
void Chip8::OP_ExA1() {

} 

// LD Vx, DT: Set Vx = delay timer value
void Chip8::OP_Fx07() {

} 

// LD Vx,K: Wait for a key press, store the value of the key in Vx
void Chip8::OP_Fx0A() {

} 

// LD DT, Vx: Set delay timer = Vx
void Chip8::OP_Fx15() {

} 

// LD ST, Vx: Set sound timer = Vx
void Chip8::OP_Fx18() {

} 

// ADD I, Vx: Set I = I + Vx
void Chip8::OP_Fx1E() {

} 

// LD F, Vx: Set I = location of sprite for digit Vx
void Chip8::OP_Fx29() {

} 

// LD B, Vx: Store BCD representation of Vx in memory locations I, I+1, and I+2
void Chip8::OP_Fx33() {

} 

// LD [I], Vx: Store registers V0 through Vx in memory starting at location I
void Chip8::OP_Fx55() {

} 

// LD Vx, [I]: Read registers V0 through Vx in memory starting at location I
void Chip8::OP_Fx65() {

} 

