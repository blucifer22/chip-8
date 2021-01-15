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

    // Set up function pointer table
    table[0x0] = &Chip8::Table0; // Redirect to Table0
    table[0x1] = &Chip8::OP_1nnn;
    table[0x2] = &Chip8::OP_2nnn;
    table[0x3] = &Chip8::OP_3xkk;
    table[0x4] = &Chip8::OP_4xkk;
    table[0x5] = &Chip8::OP_5xy0;
    table[0x6] = &Chip8::OP_6xkk;
    table[0x7] = &Chip8::OP_7xkk;
    table[0x8] = &Chip8::Table8; // Redirect to Table8
    table[0x9] = &Chip8::OP_9xy0;
    table[0xA] = &Chip8::OP_Annn;
    table[0xB] = &Chip8::OP_Bnnn;
    table[0xC] = &Chip8::OP_Cxkk;
    table[0xD] = &Chip8::OP_Dxyn;
    table[0xE] = &Chip8::TableE; // Redirect to TableE
    table[0xF] = &Chip8::TableF; // Redirect to TableF

    table0[0x0] = &Chip8::OP_00E0;
    table0[0xE] = &Chip8::OP_00EE;

    table8[0x0] = &Chip8::OP_8xy0;
    table8[0x1] = &Chip8::OP_8xy1;
    table8[0x2] = &Chip8::OP_8xy2;
    table8[0x3] = &Chip8::OP_8xy3;
    table8[0x4] = &Chip8::OP_8xy4;
    table8[0x5] = &Chip8::OP_8xy5;
    table8[0x6] = &Chip8::OP_8xy6;
    table8[0x7] = &Chip8::OP_8xy7;
    table8[0xE] = &Chip8::OP_8xyE;

    tableE[0x1] = &Chip8::OP_ExA1;
    tableE[0xE] = &Chip8::OP_Ex9E;

    tableF[0x07] = &Chip8::OP_Fx07;
    tableF[0x0A] = &Chip8::OP_Fx0A;
    tableF[0x15] = &Chip8::OP_Fx15;
    tableF[0x18] = &Chip8::OP_Fx18;
    tableF[0x1E] = &Chip8::OP_Fx1E;
    tableF[0x29] = &Chip8::OP_Fx29;
    tableF[0x33] = &Chip8::OP_Fx33;
    tableF[0x55] = &Chip8::OP_Fx55;
    tableF[0x65] = &Chip8::OP_Fx65;
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

void Chip8::Cycle() { // Define the CPU cycle loop! (Fetch, Decode, Execute)
	opcode = (memory[pc] << 8u) | memory[pc + 1];  // Fetch

    pc += 2; // Increment the PC before we do anything else!

	((*this).*(table[(opcode & 0xF000u) >> 12u]))(); // Decode and Execute

    if(delay_timer > 0) { // Decrement the delay timer if it has been set
        delay_timer--;
    }

    if(sound_timer > 0) { // Decrement the sound timer if it has been set
        sound_timer--;
    }
}

/**
 * Table Helper Functions
 * This is your daily reminder that C++ function pointer syntax is miserable.
 * Use Rust next time.
 * You have been warned...
 */

void Chip8::Table0()
{
	((*this).*(table0[opcode & 0x000Fu]))();
}

void Chip8::Table8()
{
	((*this).*(table8[opcode & 0x000Fu]))();
}

void Chip8::TableE()
{
	((*this).*(tableE[opcode & 0x000Fu]))();
}

void Chip8::TableF()
{
	((*this).*(tableF[opcode & 0x00FFu]))();
}

/**
 * OPCODES!
 * The CHIP-8 *only* has 34 opcodes (plus NULL), which are implemented below.
 */

// NULL: Do nothing (Catch-all in case the table gets clobbered)
void Chip8::OP_NULL() {
    // Do nothing
}

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

    registers[Vx] |= registers[Vy]; // Set Vx |= Vy
} 

// AND Vx, Vy: Set Vx = Vx AND 
void Chip8::OP_8xy2() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u; // Parse Vx reg number using bitmask
    uint8_t Vy = (opcode & 0x00F0u) >> 4u; // Parse Vy reg number using bitmask

    registers[Vx] &= registers[Vy]; // Set Vx &= Vy
} 

// XOR Vx, Vy: Set Vx = Vx XOR Vy
void Chip8::OP_8xy3() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u; // Parse Vx reg number using bitmask
    uint8_t Vy = (opcode & 0x00F0u) >> 4u; // Parse Vy reg number using bitmask

    registers[Vx] ^= registers[Vy]; // Set Vx ^= Vy
} 

// ADD Vx, Vy: Set Vx = Vx + Vy, set VF = carry. (VF is overflow flag)
void Chip8::OP_8xy4() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u; // Parse Vx reg number using bitmask
    uint8_t Vy = (opcode & 0x00F0u) >> 4u; // Parse Vy reg number using bitmask

    uint16_t sum = registers[Vx] + registers[Vy]; // Add the registers Vx and Vy

    if(sum > 255U) { // If the sum overflows an 8-bit register
        registers[0xF] = 1; // Set the flag register (VF) to 1
    } 
    else {
        registers[0xF] = 0; // Set the flag register (VF) to 0
    }

    registers[Vx] = sum & 0xFFu; // Bitmask off the last 8-bits of sum and save them to Vx
} 

// SUB Vx, Vy: Set Vx = Vx - Vy, set VF = carry. (VF is underflow flag)
void Chip8::OP_8xy5() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u; // Parse Vx reg number using bitmask
    uint8_t Vy = (opcode & 0x00F0u) >> 4u; // Parse Vy reg number using bitmask

    if(registers[Vx] > registers[Vy]) { // If Vx > Vy
        registers[0xF] = 1; // Set the flag register (VF) to 1
    } 
    else {
        registers[0xF] = 0; // Set the flag register (VF) to 0
    }

    registers[Vx] -= registers[Vy]; // Vx = Vx - Vy
} 

// SHR Vx: Set Vx = Vx SHR 1. (Right shift, save remainder in VF)
void Chip8::OP_8xy6() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u; // Parse Vx reg number using bitmask

    registers[0xF] = (registers[Vx] & 0x1u); // Save the LSB to VF

    registers[Vx] >>= 1; // Right shift 1 (AKA: Divide by two)
} 

// SUBN Vx, Vy: Set Vx = Vy - Vx, set VF = NOT borrow
void Chip8::OP_8xy7() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u; // Parse Vx reg number using bitmask
    uint8_t Vy = (opcode & 0x00F0u) >> 4u; // Parse Vy reg number using bitmask

    if(registers[Vy] > registers[Vx]) { // If Vy > Vx
        registers[0xF] = 1; // Set the flag register (VF) to 1
    } 
    else {
        registers[0xF] = 0; // Set the flag register (VF) to 0
    }

    registers[Vx] = registers[Vy] - registers[Vx]; // Vx = Vy - Vx
} 

// SHL Vx {, Vy}: Set Vx = Vx SHL 1. (Left shift, save MSB in VF)
void Chip8::OP_8xyE() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u; // Parse Vx reg number using bitmask

    registers[0xF] = (registers[Vx] & 0x80u) >> 7u; // Save the MSB to VF

    registers[Vx] <<= 1; // Left shift 1 (AKA: Multiply by two)
} 

// SNE Vx, Vy: Skip next instruction if Vx != Vy
void Chip8::OP_9xy0() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u; // Parse Vx reg number using bitmask
    uint8_t Vy = (opcode & 0x00F0u) >> 4u; // Parse Vy reg number using bitmask

    if(registers[Vx] != registers[Vy]) { // If the value at register Vx != value at register Vy
        pc += 2; // Skip the next instruction
    }
} 

// LD I, addr: Set I = nnn
void Chip8::OP_Annn() {
    uint16_t address = opcode &= 0x0FFFu; // Parse the address using bitmask

    index = address; // Set the index register equal to the address
} 

// JP V0, addr: Jump to location nnn + V0
void Chip8::OP_Bnnn() {
    uint16_t address = opcode &= 0x0FFFu; // Parse the address using bitmask

    pc = registers[0] + address; // Set the PC to V0 + address
}

// RND Vx, byte: Set Vx = random byte AND kk
void Chip8::OP_Cxkk() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u; // Parse Vx reg number using bitmask
    uint8_t byte = opcode & 0x00FFu; // Parse the byte

    registers[Vx] = randByte(rand_gen) & byte; // Set Vx = randByte & byte
} 

// DRW Vx, Vy, nibble: Display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision.
void Chip8::OP_Dxyn() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u; // Parse Vx reg number using bitmask
	uint8_t Vy = (opcode & 0x00F0u) >> 4u; // Parse Vy reg number using bitmask
	uint8_t height = opcode & 0x000Fu; // Parse height using bitmask

    // Wrap if we go beyond the screen boundaries!
    uint8_t x_pos = registers[Vx] % VIDEO_WIDTH;
    uint8_t y_pos = registers[Vy] % VIDEO_HEIGHT;

    registers[0xF] = 0; // Set VF = 0

    for(unsigned int row = 0; row < height; row++) { // Iterate over each row of the sprite

        uint8_t sprite_byte = memory[index + row]; // Access the current sprite byte

        for(unsigned int col = 0; col < 8; col++) { // Iterate over each bit (pixel) of the current sprite byte
           
            uint8_t sprite_pixel = sprite_byte & (0x80u >> col); // Get the current sprite pixel with bitmask
            uint32_t* screen_pixel = &video[(y_pos + row) * VIDEO_WIDTH + (x_pos + col)]; // Get a pointer to the current screen pixel

            if(sprite_pixel) { // If the sprite pixel is on

                if(*screen_pixel == 0xFFFFFFFF) { // If the screen pixel is already on (value at pointer == 0xFFFFFFFF)
                    registers[0xF] = 1; // Set the flag register (VF) to 1, indicating collision!
                }

                *screen_pixel ^= 0xFFFFFFFF; // Functionally XOR the screen pixel with the sprite pixel
            }
        }
    }
} 

// SKP Vx: Skip next instruction if key with the value of Vx is pressed
void Chip8::OP_Ex9E() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u; // Parse Vx reg number using bitmask

	uint8_t key = registers[Vx]; // Find the expected key value

	if (keypad[key]) { // If that key is pressed
		pc += 2; // Skip the next instruction
	}
} 

// SKNP Vx: Skip next instruction if key with the value of Vx is NOT pressed
void Chip8::OP_ExA1() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u; // Parse Vx reg number using bitmask

	uint8_t key = registers[Vx]; // Find the expected key value

	if (!keypad[key]) { // If that key is NOT pressed
		pc += 2; // Skip the next instruction
	}
} 

// LD Vx, DT: Set Vx = delay timer value
void Chip8::OP_Fx07() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u; // Parse Vx reg number using bitmask

    registers[Vx] = delay_timer; // Set Vx = delay_timer
} 

// LD Vx,K: Wait for a key press, store the value of the key in Vx
void Chip8::OP_Fx0A() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u; // Parse Vx reg number using bitmask

    if(keypad[0]) { // If zero is pressed on the keypad
        registers[Vx] = 0; // Vx = 0
    }
    else if(keypad[1]) { // If one is pressed on the keypad
        registers[Vx] = 1; // Vx = 1...and so on
    }
    else if(keypad[2]) {
        registers[Vx] = 2;
    }
    else if(keypad[3]) {
        registers[Vx] = 3;
    }
    else if(keypad[4]) {
        registers[Vx] = 4;
    }
    else if(keypad[5]) {
        registers[Vx] = 5;
    }
    else if(keypad[6]) {
        registers[Vx] = 6;
    }
    else if(keypad[7]) {
        registers[Vx] = 7;
    }
    else if(keypad[8]) {
        registers[Vx] = 8;
    }
    else if(keypad[9]) {
        registers[Vx] = 9;
    }
    else if(keypad[10]) {
        registers[Vx] = 10;
    }
    else if(keypad[11]) {
        registers[Vx] = 11;
    }
    else if(keypad[12]) {
        registers[Vx] = 12;
    }
    else if(keypad[13]) {
        registers[Vx] = 13;
    }
    else if(keypad[14]) {
        registers[Vx] = 14;
    }
    else if(keypad[15]) {
        registers[Vx] = 15;
    }
    else {
        pc -= 2; // Easiest way to wait is to just decrement the PC by two until a key is pressed!
    }
} 

// LD DT, Vx: Set delay timer = Vx
void Chip8::OP_Fx15() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u; // Parse Vx reg number using bitmask

    delay_timer = registers[Vx]; // Set delay_timer = Vx
} 

// LD ST, Vx: Set sound timer = Vx
void Chip8::OP_Fx18() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u; // Parse Vx reg number using bitmask

    sound_timer = registers[Vx]; // Set sound_timer = Vx
} 

// ADD I, Vx: Set I = I + Vx
void Chip8::OP_Fx1E() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u; // Parse Vx reg number using bitmask

    index = index + registers[Vx]; // Set index = index + Vx
} 

// LD F, Vx: Set I = location of sprite for digit Vx
void Chip8::OP_Fx29() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u; // Parse Vx reg number using bitmask

    uint8_t digit = registers[Vx]; // Get the digit at Vx

    index = FONTSET_ADDRESS + (5 * digit); // Set the index to the right digit within the fontset (digits are 5 bytes each)
} 

// LD B, Vx: Store BCD representation of Vx in memory locations I, I+1, and I+2
void Chip8::OP_Fx33() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u; // Parse Vx reg number using bitmask

    uint8_t value = registers[Vx]; // Get the value at Vx

    // Ones place
    memory[index + 2] = value % 10;
    value /= 10;

    // Tens place
    memory[index + 1] = value % 10;
    value /= 10;

    // Hundreds place
    memory[index] = value % 10; 
} 

// LD [I], Vx: Store registers V0 through Vx in memory starting at location I
void Chip8::OP_Fx55() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u; // Parse Vx reg number using bitmask

    for (uint8_t i = 0; i <= Vx; i++) { // For each register from V0 through Vx (INCLUSIVE!!!)
        memory[index + i] = registers[i]; // Set the memory at index + i to the current value of the register
    }
} 

// LD Vx, [I]: Read registers V0 through Vx in memory starting at location I
void Chip8::OP_Fx65() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u; // Parse Vx reg number using bitmask

    for (uint8_t i = 0; i <= Vx; i++) { // For each register from V0 through Vx (INCLUSIVE!!!)
        registers[i] = memory[index + i]; // Read the memory at index + i to the respective register
    }
} 