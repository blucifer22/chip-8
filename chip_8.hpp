#include <cstdint>
class Chip8 {
    public:
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
};