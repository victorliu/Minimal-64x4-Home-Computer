#include <cstdint>
#include <cstdio>

typedef uint32_t control_word;

/*
The microcode defines a function that takes as input the processor state and
outputs the control word to indicate what action to take. The processor state
inputs are the 4 bits of the step counter, 8 bit instruction, and 5 bits of
flags, and the output is the 24 bit control word. We can define the function as:

    M(step, instruction, flags) = ctrlword

The microcode is implemented using a set of three 8-bit ROMs. In total, this 24
bits of output form the 24-bit control word. The ROMs have 19 bits of address
input, of which 17 are used. The lowest 4 bits of address are the step counter,
then the 8 bits of instruction, and finally the 5 bits of flags.
*/

#define ROM_WORDS (1 << (4 + 8 + 5))

// LSB control signals
#define EO  (1 <<  0) /* ALU sum out */
#define ES  (1 <<  1) /* ALU subtract */
#define EC  (1 <<  2) /* ALU carry in */
#define BI  (1 <<  3) /* B in */
#define BO  (1 <<  4) /* B out */
#define AI  (1 <<  5) /* A in */
#define AO  (1 <<  6) /* A out */
#define NI  (1 <<  7) /* bank in */

// MSB control signals
#define FI  (1 <<  8) /* flags in */
#define II  (1 <<  9) /* instruction in */
#define IC  (1 << 10) /* instruction step clear */
#define CIL (1 << 11) /* program counter lsb in */
#define CIH (1 << 12) /* program counter msb in */
#define COL (1 << 13) /* program counter lsb out */
#define COH (1 << 14) /* program counter msb out */
#define CE  (1 << 15) /* program counter increment */

// HSB control signals
#define FF  (1 << 16) /* bus 0xFF */
#define RI  (1 << 17) /* RAM in */
#define RO  (1 << 18) /* RAM out */
#define MIL (1 << 19) /* memory address lsb in */
#define MIH (1 << 20) /* memory address msb in */
#define MZ  (1 << 21) /* memory zero page */
#define MC  (1 << 22) /* program counter transfer */
#define ME  (1 << 23) /* memory address increment */

// Set bits in this mask define which control signals are active low
#define ACTIVE_LOW_MASK 0x3F7FF9

// microcode_def defines the sequence of output control signals for each
// instruction across all values of step count.
#include "microcode_def.csv"

// microcode_rom defines the instruction ordering in each row, with one row per
// combination of flags.
const control_word rom[ROM_WORDS] = {
#include "microcode_rom.csv"
};

int main(int argc, char *argv[]){
	// We output the contents of each ROM here
	static const char *bytename[] = { "lsb", "msb", "hsb" };
	for(unsigned ibyte = 0; ibyte < 3; ++ibyte){
		// Generate the proper filename
		char filename[256];
		snprintf(filename, 256, "ctrl_%s.bin", bytename[ibyte]);
		FILE *fp = fopen(filename, "wb");
		// Iterate through the control words in sequence, and output the proper byte,
		// being careful to invert the active low signals.
		for(uint32_t iword = 0; iword < ROM_WORDS; ++iword){
			unsigned char b = ((ACTIVE_LOW_MASK ^ rom[iword]) >> (8 * ibyte)) & 0xFF;
			fputc(b, fp);
		}
		fclose(fp);
	}
	return 0;
}
