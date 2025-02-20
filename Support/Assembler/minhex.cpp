// ./minhex <script.min >out.hex
#include <cstdio>
#include <iostream>
#include <iomanip>
#include <sstream>

class HexPrinter // handling output in 'Intel HEX' format
{
  public:
    HexPrinter(std::stringstream& out) : mOut(out) {}
    ~HexPrinter() { if (isactive) { if (used > 0) emitBuffer(); mOut << ":00000001FF\n"; } } // write end of hex file
    void SetAddress(int laddr) { if (used > 0) emitBuffer(); linaddr = laddr; } // begin new line at new address
    int GetAddress() { return linaddr + used; } // returns the current emission address
    void Emit(uint8_t b) { isactive = true; buffer[used++] = b; if (used == 16) emitBuffer(); } // emit a byte
  protected:
    void emitBuffer() // emits current buffer as a line (only call if buffer is non-empty!)
    {
      mOut << ":" << std::hex << std::uppercase << std::setfill('0');
      uint8_t pch = (linaddr & 0xff00)>>8;
      uint8_t pcl = linaddr & 0x00ff;
      mOut << std::setw(2) << used << std::setw(2) << int(pch) << std::setw(2) << int(pcl) << "00";
      uint8_t checksum = used + pch + pcl;
      for(int i=0; i<used; i++) { mOut << std::setw(2) << int(buffer[i]); checksum += buffer[i]; }
      mOut << std::setw(2) << int((~checksum + 1) & 0xff) << "\n";
      linaddr += used; used = 0;
    }
    bool isactive = false;
    uint8_t buffer[16]{}; // emission line buffer
    int used{ 0 }; // number of emitted bytes pending in buffer
    int linaddr{ 0 }; // start address of the current data in buffer
    std::stringstream& mOut; // emission into this string stream
};


int main(int argc, char *argv[]){
	std::stringstream hexout;
	{
		HexPrinter hex(hexout);
		hex.SetAddress(0x8000);
		while (1){
			int ch = getc(stdin);
			if(ch == EOF) break;
			hex.Emit((uint8_t)ch);
		}
		hex.Emit(0);
	}
	std::cout << hexout.str();
    return 0;
}
