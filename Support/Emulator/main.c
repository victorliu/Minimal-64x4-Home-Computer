/*
  LICENSING INFORMATION:
  This file is free software: you can redistribute it and/or modify it under the terms
  of the GNU General Public License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.
  This file is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE. See the GNU General Public License for more details. You should have received
  a copy of the GNU General Public License along with this program.
  If not, see https://www.gnu.org/licenses/.
*/
#include <stdint.h>
#include "fenster.h"
#include <time.h>
#include <sys/time.h> // for struct timeval

typedef uint8_t  byte;
typedef uint16_t word;
typedef uint32_t control_word;

#define SCALE 2
#define W 512 //400
#define H 256 //240
uint32_t framebuf[W * H * SCALE * SCALE];

// memory
byte RAM[0x10000];
control_word CtrlROM[0x20000];
byte FLASH[0x80000];
unsigned FLASHState = 0; // state of FLASH write operation (see SSF39xxx datasheet)

// CPU state
byte AR = 0, BR = 0, flagLines = 0b11000;
word PC = 0, MAR = 0;
byte Bank = 0;
byte FR = 0, SC = 0, IR = 0;
byte PS2 = 0xff, UART = 0xff;

#define MAXBUF 256

// emulator state
byte ps2Buffer[MAXBUF]; byte ps2BufferStart = 0, ps2BufferEnd = 0;
byte *uartBuffer = NULL; // PS2 and UART input buffers
int uartBufferStart = 0, uartBufferSize = 0;
int ps2RecCounter = 0, uartRecCounter = 0, uartSendCounter = 0; // emulates the speed of a PS2 and UART

byte PS2ScanCode[256] = {
	0, // 0
	0, // 1
	0, // 2
	0, // 3
	0, // 4
	0, // 5
	0, // 6
	0, // 7
	0x66, // 8 Backspace
	0x0D, // 9 Tab
	0x5A, // 10 Enter
	0, // 11
	0, // 12
	0, // 13
	0, // 14
	0, // 15
	0, // 16
	0x75, // 17 Up
	0x72, // 18 Down
	0x74, // 19 Right
	0x6B, // 20 Left
	0, // 21
	0, // 22
	0, // 23
	0, // 24
	0, // 25
	0, // 26
	0x76, // 27 Esc
	0, // 28
	0, // 29
	0, // 30
	0, // 31
	0x29, // 32 Space
	0, // 33
	0, // 34
	0, // 35
	0, // 36
	0, // 37
	0, // 38
	0x52, // 39 '
	0, // 40
	0, // 41
	0, // 42
	0, // 43
	0x41, // 44 ,
	0x4E, // 45 -
	0x49, // 46 .
	0x4A, // 47 /
	0x45, // 48 0
	0x16, // 49 1
	0x1E, // 50 2
	0x26, // 51 3
	0x25, // 52 4
	0x2E, // 53 5
	0x36, // 54 6
	0x3D, // 55 7
	0x3E, // 56 8
	0x46, // 57 9
	0, // 58
	0x4C, // 59 ;
	0, // 60
	0x55, // 61 =
	0, // 62
	0, // 63
	0, // 64
	0x1C, // 65 A
	0x32, // 66 B
	0x21, // 67 C
	0x23, // 68 D
	0x24, // 69 E
	0x2B, // 70 F
	0x34, // 71 G
	0x33, // 72 H
	0x43, // 73 I
	0x3B, // 74 J
	0x42, // 75 K
	0x4B, // 76 L
	0x3A, // 77 M
	0x31, // 78 N
	0x44, // 79 O
	0x4D, // 80 P
	0x15, // 81 Q
	0x2D, // 82 R
	0x1B, // 83 S
	0x2C, // 84 T
	0x3C, // 85 U
	0x2A, // 86 V
	0x1D, // 87 W
	0x22, // 88 X
	0x35, // 89 Y
	0x1A, // 90 Z
	0x54, // 91 [
	0x5D, // 92 backslash
	0x5B, // 93 ]
	0, // 94
	0, // 95
	0x0E, // 96 `
	0, // 97
	0, // 98
	0, // 99
	0, // 100
	0, // 101
	0, // 102
	0, // 103
	0, // 104
	0, // 105
	0, // 106
	0, // 107
	0, // 108
	0, // 109
	0, // 110
	0, // 111
	0, // 112
	0, // 113
	0, // 114
	0, // 115
	0, // 116
	0, // 117
	0, // 118
	0, // 119
	0, // 120
	0, // 121
	0, // 122
	0, // 123
	0, // 124
	0, // 125
	0, // 126
	0, // 127
	0, // 128
	0, // 129
	0, // 130
	0, // 131
	0, // 132
	0, // 133
	0, // 134
	0, // 135
	0, // 136
	0, // 137
	0, // 138
	0, // 139
	0, // 140
	0, // 141
	0, // 142
	0, // 143
	0, // 144
	0, // 145 F1
	0, // 146 F2
	0, // 147 F3
	0, // 148 F4
	0, // 149 F5
	0, // 150 F6
	0, // 151 F7
	0, // 152 F8
	0, // 153 F9
	0, // 154 F10
	0, // 155 F11
	0, // 156 F12
	0, // 157
	0, // 158
	0, // 159
	0, // 160
	0, // 161
	0, // 162
	0, // 163
	0, // 164
	0, // 165
	0, // 166
	0, // 167
	0, // 168
	0, // 169
	0, // 170
	0, // 171
	0, // 172
	0, // 173
	0, // 174
	0, // 175
	0, // 176
	0, // 177
	0, // 178
	0, // 179
	0, // 180
	0, // 181
	0, // 182
	0, // 183
	0, // 184
	0, // 185
	0, // 186
	0, // 187
	0, // 188
	0, // 189
	0, // 190
	0, // 191
	0, // 192
	0, // 193
	0, // 194
	0, // 195
	0, // 196
	0, // 197
	0, // 198
	0, // 199
	0, // 200
	0, // 201
	0, // 202
	0, // 203
	0, // 204
	0, // 205
	0, // 206
	0, // 207
	0, // 208
	0, // 209
	0, // 210
	0, // 211
	0, // 212
	0, // 213
	0, // 214
	0, // 215
	0, // 216
	0, // 217
	0, // 218
	0, // 219
	0, // 220
	0, // 221
	0, // 222
	0, // 223
	0, // 224
	0, // 225
	0, // 226
	0, // 227
	0, // 228
	0, // 229
	0, // 230
	0, // 231
	0, // 232
	0, // 233
	0, // 234
	0, // 235
	0, // 236
	0, // 237
	0, // 238
	0, // 239
	0, // 240
	0, // 241
	0, // 242
	0, // 243
	0, // 244
	0, // 245
	0, // 246
	0, // 247
	0, // 248
	0, // 249
	0, // 250
	0, // 251
	0, // 252
	0, // 253
	0, // 254
	0  // 255
};

// control signal definitions
const control_word EO  = 0b000000000000000000000001; // LSB
const control_word ES  = 0b000000000000000000000010;
const control_word EC  = 0b000000000000000000000100;
const control_word BI  = 0b000000000000000000001000;
const control_word BO  = 0b000000000000000000010000;
const control_word AI  = 0b000000000000000000100000;
const control_word AO  = 0b000000000000000001000000;
const control_word NI  = 0b000000000000000010000000;
const control_word FI  = 0b000000000000000100000000; // MSB
const control_word II  = 0b000000000000001000000000;
const control_word IC  = 0b000000000000010000000000;
const control_word CIL = 0b000000000000100000000000;
const control_word CIH = 0b000000000001000000000000;
const control_word COL = 0b000000000010000000000000;
const control_word COH = 0b000000000100000000000000;
const control_word CE  = 0b000000001000000000000000;
const control_word FF  = 0b000000010000000000000000; // HSB
const control_word RI  = 0b000000100000000000000000;
const control_word RO  = 0b000001000000000000000000;
const control_word MIL = 0b000010000000000000000000;
const control_word MIH = 0b000100000000000000000000;
const control_word MZ  = 0b001000000000000000000000;
const control_word MC  = 0b010000000000000000000000;
const control_word ME  = 0b100000000000000000000000;

const control_word CTRL_INVERT_MASK = 0b001111110111111111111001; // 0: control signal is active HIGH, 1: control signal is active LOW

const byte FLAG_Z = 0b00001; // ALU zero flag
const byte FLAG_C = 0b00010; // ALU carry flag
const byte FLAG_N = 0b00100; // ALU negative flag
const byte FLAG_T = 0b01000; // UART data ready
const byte FLAG_K = 0b10000; // PS2 data ready

byte *flashbin, *lsbbin, *msbbin, *hsbbin, *uartbin; // file loading containers

int loadText(byte **dst, int *size, const char *filename){
	int ret = 0;
	long fsize, nread;
	FILE *fp = fopen(filename, "rb");
	fseek(fp, 0, SEEK_END);
	fsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	if(NULL == size){
		*dst = realloc(*dst, fsize);
		fread(*dst, fsize, 1, fp);
		ret = fsize;
	}else{
		if(fsize > *size){
			nread = *size;
		}else{
			nread = fsize;
		}
		*size = fsize;
		fread(*dst, 1, nread, fp);
	}
	fclose(fp);
	return ret;
}
int loadBytes(byte *dst, int *size, const char *filename){
	int ret = 0;
	long fsize, nread;
	FILE *fp = fopen(filename, "rb");
	fseek(fp, 0, SEEK_END);
	fsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	if(fsize > *size){
		nread = *size;
	}else{
		nread = fsize;
	}
	*size = fsize;
	fread(dst, 1, nread, fp);

	fclose(fp);
	return ret;
}

void updateVGA(struct fenster *f){
	int i, j, si, sj;
	int p = 0; // index into pixel byte array
	for(i=0x4000; i<0x8000; i++){ // Minimal VRAM: 64 bytes per row, 256 rows
		int row = (i - 0x4000)/64;
		int col8 = (i - 0x4000)%64;
		byte b = RAM[i]; // fetch byte from Minimal's VRAM    
		for(j=0; j<8; j++){ // plot the pixels of this byte into image
			for(si = 0; si < SCALE; ++si){
				for(sj = 0; sj < SCALE; ++sj){
					fenster_pixel(f, SCALE*(8*col8 + j) + sj, SCALE*(row) + si) = ((b & 1) ? 0xe8e4e0 : 0x282420);
				}
			}
			b >>= 1; // move next pixel into lowest position
		}
	}
}

void doClock(){
	int i;
	
	if (ps2RecCounter > 0) ps2RecCounter--; // time delay between datagrams
	else if (ps2BufferEnd != ps2BufferStart){ // anything in PS/2 FIFO buffer?
		byte b = ps2Buffer[ps2BufferStart++];
		if (flagLines & FLAG_K){ // PS/2 reg cleared? Otherwise discard.
			PS2 = b; // place data in PS/2 reg
			flagLines &= ~FLAG_K; // set "data ready" flag
		}
		ps2RecCounter = 16000; // emulate 2ms datagram rate
	}
	
	if (uartRecCounter > 0) uartRecCounter--; // time delay between datagrams
	else if (uartBufferStart < uartBufferSize){  // anything in UART FIFO buffer?)
		byte b = uartBuffer[uartBufferStart++];
		if (b == 0x0a) uartRecCounter = 80000; else uartRecCounter = 176; // 10ms line delay
		if (flagLines & FLAG_T) { UART = b; flagLines &= ~FLAG_T; } // UART reg cleared? Otherwise discard.
	}
	if (uartSendCounter > 0) uartSendCounter--;
	
	// control word
	control_word mCtrl = CtrlROM[((control_word)FR << 12) | ((control_word)IR << 4) | SC];
	
	// ALU
	word a = AR & 0xff;
	word b = BR & 0xff;
	word c = b; if (mCtrl & ES) c = (~c) & 0xff; // possible negation
	word result = a + c; if (mCtrl & EC) result++; // adder result
	if (result > 0xff) flagLines |= FLAG_C; else flagLines &= ~FLAG_C; // update flags
	if ((result & 0xff) == 0) flagLines |= FLAG_Z; else flagLines &= ~FLAG_Z;
	if (result & 0x80) flagLines |= FLAG_N; else flagLines &= ~FLAG_N;

	// outputs to system bus
	byte mBus;
	if (mCtrl & FF) mBus = 0xff;
	if (mCtrl & AO) mBus = AR;
	if (mCtrl & BO) mBus = BR;
	if (mCtrl & EO) mBus = result & 0xff;
	else{
		if (mCtrl & ES) mBus = (a & b) & 0xff;
		else if (mCtrl & EC) mBus = (a | b) & 0xff;
	}
	if (mCtrl & RO){
		if ((MAR & 0x8000) || (Bank & 0x80)) mBus = RAM[MAR];
		else mBus = FLASH[((word)Bank<<12) | (MAR & 0x0fff)];
	}
	if (mCtrl & MZ){ // serial and PS/2 receive register output
		if ((mCtrl & AI) && !(FR & FLAG_T)) { mBus = UART; UART = 0xff; flagLines |= FLAG_T; } // "TO"
		if ((mCtrl & BI) && !(FR & FLAG_K)) { mBus = PS2; PS2 = 0xff; flagLines |= FLAG_K; } // "KO"
	}
	if (mCtrl & COL) mBus = PC & 0xff;
	if (mCtrl & COH) mBus = PC >> 8;

	// inputs
	if (mCtrl & RI){ // note: RI|ME/MIL/MIH works, since ME/MIL/MIH changes MAR *only after* the write pulse has finshed
    if ((MAR & 0x8000) || (Bank & 0x80)) RAM[MAR] = mBus; // always write to RAM
    else{ // FLASH access with 4 upper bits = 0
    	word adr = ((word)Bank<<12) | (MAR & 0x0fff); // FLASH only needs 15 bits
    	switch (FLASHState){
    		case 0: if (adr == 0x5555 && mBus == 0xaa) FLASHState = 1; else FLASHState = 0; break;
    		case 1: if (adr == 0x2aaa && mBus == 0x55) FLASHState = 2; else FLASHState = 0; break;
    		case 2:
    			if (adr == 0x5555 && mBus == 0xa0) { FLASHState = 3; break; }
    			if (adr == 0x5555 && mBus == 0x80) { FLASHState = 4; break; }
    			FLASHState = 0; break;
    		case 3: FLASH[adr] &= mBus; FLASHState = 0; break; // write operation only writes 1->0, not 0->1
    		case 4: if (adr == 0x5555 && mBus == 0xaa) FLASHState = 5; else FLASHState = 0; break;
    		case 5: if (adr == 0x2aaa && mBus == 0x55) FLASHState = 6; else FLASHState = 0; break;
    		case 6:
    			if (mBus == 0x30) for (i=0; i<0x1000; i++) FLASH[((word)Bank<<12) | i] = 0xff; // sector erase operation
    			FLASHState = 0; break;
			}
		}
	}
	if (mCtrl & AI) AR = mBus;
	if (mCtrl & BI) BR = mBus;
	if (mCtrl & NI) Bank = mBus;
	if (mCtrl & FI) FR = flagLines;
	if (mCtrl & II) IR = mBus;
	if (mCtrl & CE) PC++; // may be overridden by a later CIL/CIH
	if (mCtrl & CIL) PC = (PC & 0xff00) | mBus;
	if (mCtrl & CIH) PC = (PC & 0x00ff) | (mBus<<8);
	if (mCtrl & ME) MAR++; // may be overridden by a later MIL/MIH
	if (mCtrl & MIL){
		if (mCtrl & MC) MAR = (MAR & 0xff00) | (PC & 0x00ff);
		else MAR = (MAR & 0xff00) | mBus;
	}
	if (mCtrl & MIH){
		if (mCtrl & MZ) MAR = MAR & 0x00ff; // zero-page access
		else if (mCtrl & MC) MAR = (MAR & 0x00ff) | (PC & 0xff00);
		else MAR = (MAR & 0x00ff) | (mBus<<8);
	}
	if ((mCtrl & MZ) && (mCtrl & AO)){
		//if (uartSendCounter == 0) printf("%02x", mBus); else printf("!too fast!");
		uartSendCounter = 176; // 11 * 16 cycles
	}  
	if (mCtrl & IC) SC = 0; else { SC++; SC &= 0x0f; } // step counter increment
}




void key_handler(byte key, int state, int mod){
	if(PS2ScanCode[key & 0xFF]){
		if(!state){
			ps2Buffer[ps2BufferEnd++] = 0xF0;
		}
		ps2Buffer[ps2BufferEnd++] = PS2ScanCode[key & 0xFF];
	}else if(state == 1 && key == FENSTER_KEY_F1){
		SC = 0; PC = 0; MAR = 0; Bank = 0;
	}else if(state == 1 && key == FENSTER_KEY_F4){
		printf("Uploading...\n");
		uartBufferSize = loadText(&uartBuffer, NULL, "uart.txt");

		uartBufferStart = 0;
	}
}

int main(int argc, char *argv[]){
	int i, j, keyIsPressed, iframe = 0;
	int size;
	byte lsb[0x20000], msb[0x20000], hsb[0x20000];
	struct fenster f = {
		.title = "Minimal 64x4",
		.width = W * SCALE,
		.height = H * SCALE,
		.buf = framebuf,
		.key_handler = key_handler
	};
	fenster_open(&f);


	for(i = 0; i < sizeof(RAM); i++){
		RAM[i] = (rand() & 0xff);
	}

	size = sizeof(FLASH); loadBytes(FLASH, &size, "flash.bin");
	size = sizeof(lsb); loadBytes(lsb, &size, "ctrl_lsb.bin");
	size = sizeof(msb); loadBytes(msb, &size, "ctrl_msb.bin");
	size = sizeof(hsb); loadBytes(hsb, &size, "ctrl_hsb.bin"); 
	for(i = 0; i < sizeof(lsb); i++){
		CtrlROM[i] = (lsb[i] | (msb[i]<<8) | (hsb[i]<<16)) ^ CTRL_INVERT_MASK; // convert to active HIGH
	}

	struct timeval t_last, t_cur;
	gettimeofday(&t_last, NULL);
	while (fenster_loop(&f) == 0) {
		updateVGA(&f);
		gettimeofday(&t_cur, NULL);
		long dt = ((t_cur.tv_sec - t_last.tv_sec)*1000000L + t_cur.tv_usec) - t_last.tv_usec;
		long ncycles = 8*dt;
		for(j = 0; j < ncycles; ++j){
			doClock();
		}
		t_last = t_cur;
		fenster_sleep(10);
	}
	fenster_close(&f);
	return 0;
}
