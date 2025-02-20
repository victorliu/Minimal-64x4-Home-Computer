; ------------------------------------------------------
; FONTEDIT - Font editor for the MINIMAL 64x4 Home Computer
; by Victor Liu 2025
; ------------------------------------------------------
;
; The built-in system font is 256 characters, 8 bytes each, for a total of
; 2048 (0x800) bytes. We use memory starting at 0x1000 to 0x1800 for the Font.
;
; Usage:
;   Tab: Toggle focus between character array and pixel array.
;   Arrow keys: Move around the character array or pixel array.
;   Space: Toggle active pixel.
;   L: Prompt for filename to load.
;   S: Prompt for filename to save.
;   Esc: Exit to OS.

#org 0x2000

FontEdit_Start:
	JPS _Clear

	MIZ 0, pixel_active
	MIZ 0, current_char
	MIZ 0, table_cursor_x
	MIZ 0, table_cursor_y
	MIZ 0, pixel_cursor_x
	MIZ 0, pixel_cursor_y
	
	; Copy global font table to local memory as starting point
	MIV 0x0800, 0
	MIZ 1, 2         ; system font is in bank 1
	MIV 0x1800, 3
	start_copy:
		DEV 0
		DEV 3
		RDR 0x0000  STT 3
		CIZ 0, 0  BNE start_copy
		CIZ 0, 1  BNE start_copy

	JPS DrawTableHeadings
	JPS DrawTable
	JPS DrawPixelHeadings
	JPS DrawPixels
	
	mainloop:
		JPS _ReadInput  CPI 0 BEQ mainloop
		CPI 27 BEQ exit
		CPI 0xe3 BEQ key_left
		CPI 0xe4 BEQ key_right
		CPI 0xe1 BEQ key_up
		CPI 0xe2 BEQ key_down
		CPI 0x09 BEQ key_tab
		CPI 0x20 BEQ key_space
		CPI 's'  BEQ key_save
		CPI 'l'  BEQ key_load
		JPA mainloop
	
	key_left:
		LDZ pixel_active  CPI 0  BEQ key_left_table
			DEZ pixel_cursor_x  BPL redraw_pixel_headings
			CLZ pixel_cursor_x  JPA mainloop
		key_left_table:
			DEZ table_cursor_x  BPL redraw_table_headings
			CLZ table_cursor_x  JPA mainloop

	key_right:
		LDZ pixel_active  CPI 0  BEQ key_right_table
			INZ pixel_cursor_x  CPI 8  BCC redraw_pixel_headings
			MIZ 7,pixel_cursor_x  JPA mainloop
		key_right_table:
			INZ table_cursor_x  CPI 16  BCC redraw_table_headings
			MIZ 15,table_cursor_x  JPA mainloop

	key_up:
		LDZ pixel_active  CPI 0  BEQ key_up_table
			DEZ pixel_cursor_y  BPL redraw_pixel_headings
			CLZ pixel_cursor_y  JPA mainloop
		key_up_table:
			DEZ table_cursor_y  BPL redraw_table_headings
			CLZ table_cursor_y  JPA mainloop

	key_down:
		LDZ pixel_active  CPI 0  BEQ key_down_table
			INZ pixel_cursor_y  CPI 8  BCC redraw_pixel_headings
			MIZ 7,pixel_cursor_y  JPA mainloop
		key_down_table:
			INZ table_cursor_y  CPI 16  BCC redraw_table_headings
			MIZ 15,table_cursor_y  JPA mainloop

	key_tab:
		LDZ pixel_active  XRI 1  STZ pixel_active
		JPA mainloop
	key_space:
		JPS TogglePixel
		JPS DrawTable
		JPS DrawPixels
		JPA mainloop

	redraw_pixel_headings:
		JPS DrawPixelHeadings
		JPA mainloop
	redraw_table_headings:
		JPS DrawTableHeadings
		JPS DrawPixels
		JPA mainloop

	key_save:
		MIZ  0, _XPos
		MIZ 28, _YPos
		JPS PrintClearLine

		MIZ  0, _XPos
		MIZ 27, _YPos
		JPS _Print "Save to file: ", 0

		MIV _ReadBuffer,_ReadPtr              ; init read buffer
		JPS _ReadLine                         ; get a line of input

		LDI <Font PHS
		LDI >Font PHS
		LDI <FontEnd-1 PHS
		LDI >FontEnd-1 PHS
		JPS _SaveFile
		CPI 0 BNE no_save_err
			JPS _Print "SAVE ERROR.", 10, 0
		no_save_err:
		PLS PLS PLS PLS
		
		; Clear line
		MIZ  0, _XPos
		MIZ 27, _YPos
		JPS PrintClearLine

		JPA mainloop
	key_load:
		MIZ  0, _XPos
		MIZ 28, _YPos
		JPS PrintClearLine

		MIZ  0, _XPos
		MIZ 27, _YPos
		JPS _Print "Load file: ", 0

		MIV _ReadBuffer,_ReadPtr              ; init read buffer
		JPS _ReadLine                         ; get a line of input

		LDI <Font PHS
		LDI >Font PHS
		LDI <FontEnd-1 PHS
		LDI >FontEnd-1 PHS
		JPS _LoadFile
		CPI 0 BNE no_load_err
			JPS _Print "NOT FOUND.", 10, 0
		no_load_err:
		PLS PLS PLS PLS
		
		; Clear line
		MIZ  0, _XPos
		MIZ 27, _YPos
		JPS PrintClearLine

		JPS DrawTable
		JPS DrawPixels

		JPA mainloop

	exit:
	JPS _Clear
	JPA _Prompt

PrintClearLine:
	JPS _Print "                                                  ", 0
	RTS

DrawPixel: ; row in Z[0], col in Z[1], A is set status; must save Z[0:3]
	STZ 3
	BEQ dp_clear
	MIZ 0x80, 3
	dp_clear:
	AIZ 0x20, 3 ; Z[3] is either 0x20 or 0xa0 for clear or set square
		

	MIZ <PIXEL_ORIGIN, _XPos
	MIZ >PIXEL_ORIGIN, _YPos

	MZZ 0, 4  LLZ 4  AZZ 0, 4   ; Z[4] is now 3*Z[0]
	AZZ 4, _YPos
	MZZ 1, 4  LLZ 4  AZZ 1, 4   ; Z[4] is now 3*Z[1]
	AZZ 4, _XPos

	MIZ 3, 5
	dp_loopy:
		MIZ 3, 4
		dp_loopx:
			LDZ 3 JAS _Char
			INZ _XPos
			DEZ 4  BNE dp_loopx
		SIZ 3, _XPos
		INZ _YPos
		DEZ 5  BNE dp_loopy
	RTS

TogglePixel:
	MIV Font, font_cursor
	LDZ current_char  AD.Z <font_cursor
	AZZ pixel_cursor_y, font_cursor+1

	; Form the pixel mask in Z[0]
	MIZ 0x01, 0
	MZZ pixel_cursor_x, 1
	tp_shift:
		CIZ 0, 1  BEQ tp_toggle
		LLZ 0
		DEZ 1
		JPA tp_shift

	tp_toggle:
	LDT font_cursor  XRZ 0  STT font_cursor

	RTS


DrawPixels:
	MIV Font, font_cursor
	LDZ current_char  AD.Z <font_cursor

	CLZ 0
	px_row_loop:
		; Load in the row from font
		LDT font_cursor  STZ 2
		CLZ 1
		px_col_loop:
			RRZ 2
			LDI 0  RL1  JAS DrawPixel
			INZ 1  CPI 8  BCC px_col_loop

		INZ font_cursor+1 ; Advance cursor to next row of the current character
		INZ 0  CPI 8  BCC px_row_loop

	RTS

DrawPixelHeadings:
	MIZ <PIXEL_ORIGIN, _XPos
	MIZ >PIXEL_ORIGIN-0x0200, _YPos
	CLZ 0
	dpg_loopx:
		CZZ 0, pixel_cursor_x  BEQ dpg_xeq
			LDI 0x8f  JAS _Char  INZ _XPos
			LDI 0x20  JAS _Char  INZ _XPos
			LDI 0x8e  JAS _Char
			JPA dpg_loopx_next
		dpg_xeq:
			LDI 0xa0  JAS _Char  INZ _XPos
			LDI 0xa0  JAS _Char  INZ _XPos
			LDI 0xa0  JAS _Char
		dpg_loopx_next:
		INZ _XPos
		INZ 0  CPI 8  BCC dpg_loopx

	MIZ <PIXEL_ORIGIN-0x0002, _XPos
	MIZ >PIXEL_ORIGIN, _YPos
	CLZ 0
	dpg_loopy:
		CZZ 0, pixel_cursor_y  BEQ dpg_yeq
			LDI 0x8f  JAS _Char  INZ _YPos
			LDI 0x20  JAS _Char  INZ _YPos
			LDI 0x91  JAS _Char
			JPA dpg_loopy_next
		dpg_yeq:
			LDI 0xa0  JAS _Char  INZ _YPos
			LDI 0xa0  JAS _Char  INZ _YPos
			LDI 0xa0  JAS _Char
		dpg_loopy_next:
		INZ _YPos
		INZ 0  CPI 8  BCC dpg_loopy

	RTS

DrawTableHeadings: ; updates current_char
	; Label columns across the top
	MIZ <TABLE_ORIGIN, _XPos
	MIZ >TABLE_ORIGIN-0x0200, _YPos
	CLZ 0
	ColLabel_loop:
		; See if we are the currently selected column
		CLZ 1
		CZZ 0, table_cursor_x
		BNE ColLabel_skip_inversion
			; Invert the character by adding 128
			MIZ 0x80, 1
		ColLabel_skip_inversion:

		; Load ASCII code of current column index Z[0]
		LZB 0, FontEdit_Hex2ASCII
		ADZ 1
		; Draw character
		JAS _Char
		INZ _XPos ; Do not need this since XPos is incremented by PrintChar
		MIZ >TABLE_ORIGIN-0x0200, _YPos
		INZ 0  CPI 16  BCC ColLabel_loop

	; Label rows going down
	MIZ <TABLE_ORIGIN-2, _XPos
	MIZ >TABLE_ORIGIN, _YPos
	CLZ 0
	RowLabel_loop:
		CLZ 1
		CZZ 0, table_cursor_y
		BNE RowLabel_skip_inversion
			; Invert the character by adding 128
			MIZ 0x80, 1
		RowLabel_skip_inversion:

		; Load ASCII code of current column index Z[0]
		LZB 0, FontEdit_Hex2ASCII
		ADZ 1
		; Draw character
		JAS _Char
		MIZ <TABLE_ORIGIN-2, _XPos
		INZ _YPos
		INZ 0  CPI 16  BCC RowLabel_loop

	LDZ table_cursor_y  LL4  ADZ table_cursor_x  STZ current_char

	RTS

DrawTable:
	; Draw characters
	CLZ 1 ; row (y) counter
	DrawTable_rowloop:
		CLZ 0 ; column (x) counter
		DrawTable_colloop:
			LDZ 1  LL4  ADZ 0  STZ 2           ; Z[2] contains the ASCII code
			LDZ 0  ADI <TABLE_ORIGIN  STZ _XPos
			LDZ 1  ADI >TABLE_ORIGIN  STZ _YPos

			LDI <VIEWPORT ADZ _XPos STZ 3      ; index to video position of char
			LDZ _YPos LL1 ADI >VIEWPORT STZ 4  ; multiply y with 8*64 = 512
			LZB 2,Font+0x0000 STT 3 AIZ 64,3
			LZB 2,Font+0x0100 STT 3 AIZ 64,3
			LZB 2,Font+0x0200 STT 3 AIZ 64,3
			LZB 2,Font+0x0300 STT 3 INZ 4
			LZB 2,Font+0x0700 STT 3 SIZ 64,3
			LZB 2,Font+0x0600 STT 3 SIZ 64,3
			LZB 2,Font+0x0500 STT 3 SIZ 64,3
			LZB 2,Font+0x0400 STT 3

			INZ 0  CPI 16  BCC DrawTable_colloop
		INZ 1  CPI 16  BCC DrawTable_rowloop
	RTS

FontEdit_Hex2ASCII: "0123456789ABCDEF"

#mute

; Place all variables in zero page, reserve 16 general purpose registers at 0
#org 0x0010

pixel_active:   0      ; If 1, the pixel array is active, otherwise table
current_char:   0x00
table_cursor_x: 0x00
table_cursor_y: 0x00
pixel_cursor_x: 0x00
pixel_cursor_y: 0x00
font_cursor:    0x0000

#org 0x1000 Font:
#org 0x1800 FontEnd:

; Constants
#org 0x0303 TABLE_ORIGIN:
#org 0x0217 PIXEL_ORIGIN:

#mute ; MinOS API definitions generated by 'asm os.asm -s_'

#org 0x430c VIEWPORT:

#org 0x0000 Charset:
#org 0x1800 LineLSBTable:
#org 0x1900 LineMSBTable:

#org 0xf000 _Start:
#org 0xf003 _Prompt:
#org 0xf006 _MemMove:
#org 0xf009 _Random:
#org 0xf00c _ScanPS2:
#org 0xf00f _ResetPS2:
#org 0xf012 _ReadInput:
#org 0xf015 _WaitInput:
#org 0xf018 _ReadLine:
#org 0xf01b _SkipSpace:
#org 0xf01e _ReadHex:
#org 0xf021 _SerialWait:
#org 0xf024 _SerialPrint:
#org 0xf027 _FindFile:
#org 0xf02a _LoadFile:
#org 0xf02d _SaveFile:
#org 0xf030 _ClearVRAM:
#org 0xf033 _Clear:
#org 0xf036 _ClearRow:
#org 0xf039 _ScrollUp:
#org 0xf03c _ScrollDn:
#org 0xf03f _Char:
#org 0xf042 _PrintChar:
#org 0xf045 _Print:
#org 0xf048 _PrintPtr:
#org 0xf04b _PrintHex:
#org 0xf04e _SetPixel:
#org 0xf051 _Line:
#org 0xf054 _Rect:
#org 0x00c0 _XPos:
#org 0x00c1 _YPos:
#org 0x00c2 _RandomState:
#org 0x00c6 _ReadNum:
#org 0x00c9 _ReadPtr:
#org 0x00cd _ReadBuffer:
