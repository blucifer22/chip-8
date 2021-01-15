#pragma once

#include <cstdint>
#include <random>


const unsigned int KEY_COUNT = 16;
const unsigned int MEMORY_SIZE = 4096;
const unsigned int REGISTER_COUNT = 16;
const unsigned int STACK_LEVELS = 16;
const unsigned int VIDEO_HEIGHT = 32;
const unsigned int VIDEO_WIDTH = 64;

class Chip8 {
    public:
        Chip8();
        void LoadROM(const char* filename);


    private:
        // Define the specifications of our CHIP-8 Machine
        uint8_t registers[16]{}; // Define our 16, 8-bit, general purpose registers
        uint8_t memory[4096]{}; // Define our 4 Kilobytes of RAM
        uint16_t index{}; // Define our special 16-bit index register
        uint16_t pc{}; // Define a 16-bit program counter
        uint16_t stack[16]{}; // Define our 32-byte stack (16, 16-bit slots)
        uint8_t sp{}; // 8-bit stack pointer (where we are on the stack)
        uint8_t delay_timer{}; // 8-bit delay_timer (counts down at 60 Hz)
        uint8_t sound_timer{}; // 8-bit sound_timer (counts down at 60 Hz) TODO: Implement sound in SDL2
        uint8_t keypad[16]{}; // Store our keypad mappings
        uint32_t video[64 * 32]{}; // Display buffer
        uint16_t opcode;

        std::default_random_engine rand_gen; // Create a member variable for our RNG engine
        std::uniform_int_distribution<uint8_t> randByte; // Create a member variable for an RNG output

        void Table0();
        void Table8();
        void TableE();
        void TableF();

        typedef void (Chip8::*Chip8Func)();
        Chip8Func table[0xF + 1]{&Chip8::OP_NULL};
        Chip8Func table0[0xE + 1]{&Chip8::OP_NULL};
        Chip8Func table8[0xE + 1]{&Chip8::OP_NULL};
        Chip8Func tableE[0xE + 1]{&Chip8::OP_NULL};
        Chip8Func tableF[0x65 + 1]{&Chip8::OP_NULL};

        /**
         * OPCODES!
         * The CHIP-8 *only* has 34 opcodes, which are enumerated and described below.
         * The actual implementation of each opcode is in chip_8.cpp
         */
    
        void OP_NULL(); // NULL: Do nothing (Catch-all if the table gets clobbered)
        void OP_00E0(); // CLS: Clear the display
        void OP_00EE(); // RET: Return from a subroutine
        void OP_1nnn(); // JP addr: Jump to location nnn
        void OP_2nnn(); // CALL addr: Call subroutine at nnn
        void OP_3xkk(); // SE Vx, byte: Skip next instruction if Vx == kk
        void OP_4xkk(); // SNE Vx, byte: Skip next instruction if Vx != kk
        void OP_5xy0(); // SE Vx, Vy: Skip next instruction if Vx == Vy
        void OP_6xkk(); // LD Vx, byte: Set Vx = kk
        void OP_7xkk(); // ADD Vx, byte: Set Vx = Vx + kk
        void OP_8xy0(); // LD Vx, Vy: Set Vx = Vy
        void OP_8xy1(); // OR Vx, Vy: Set Vx = Vx OR Vy
        void OP_8xy2(); // AND Vx, Vy: Set Vx = Vx AND Vy
        void OP_8xy3(); // XOR Vx, Vy: Set Vx = Vx XOR Vy
        void OP_8xy4(); // ADD Vx, Vy: Set Vx = Vx + Vy, set VF = carry. (VF is overflow flag)
        void OP_8xy5(); // SUB Vx, Vy: Set Vx = Vx - Vy, set VF = carry. (VF is underflow flag)
        void OP_8xy6(); // SHR Vx: Set Vx = Vx SHR 1. (Right shift, save remainder in VF)
        void OP_8xy7(); // SUBN Vx, Vy: Set Vx = Vy - Vx, set VF = NOT borrow
        void OP_8xyE(); // SHL Vx {, Vy}: Set Vx = Vx SHL 1. (Left shift, save MSB in VF)
        void OP_9xy0(); // SNE Vx, Vy: Skip next instruction if Vx != Vy
        void OP_Annn(); // LD I, addr: Set I = nnn
        void OP_Bnnn(); // JP V0, addr: Jump to location nnn + V0
        void OP_Cxkk(); // RND Vx, byte: Set Vx = random byte AND kk
        void OP_Dxyn(); // DRW Vx, Vy, nibble: Display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision.
        void OP_Ex9E(); // SKP Vx: Skip next instruction if key with the value of Vx is pressed
        void OP_ExA1(); // SKNP Vx: Skip next instruction if key with the value of Vx is NOT pressed
        void OP_Fx07(); // LD Vx, DT: Set Vx = delay timer value
        void OP_Fx0A(); // LD Vx,K: Wait for a key press, store the value of the key in Vx
        void OP_Fx15(); // LD DT, Vx: Set delay timer = Vx
        void OP_Fx18(); // LD ST, Vx: Set sound timer = Vx
        void OP_Fx1E(); // ADD I, Vx: Set I = I + Vx
        void OP_Fx29(); // LD F, Vx: Set I = location of sprite for digit Vx
        void OP_Fx33(); // LD B, Vx: Store BCD representation of Vx in memory locations I, I+1, and I+2
        void OP_Fx55(); // LD [I], Vx: Store registers V0 through Vx in memory starting at location I
        void OP_Fx65(); // LD Vx, [I]: Read registers V0 through Vx in memory starting at location I
};