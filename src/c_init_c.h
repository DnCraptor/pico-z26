/*
	init.c -- initialize all data in emulator core modules

 z26 is Copyright 1997-2011 by John Saeger and contributors.  
 contributors.	 z26 is released subject to the terms and conditions of the 
 GNU General Public License Version 2 (GPL).  z26 comes with no warranty.
 Please see COPYING.TXT for details.
*/
static inline void TIARIOTReadA(dw a) {
    if (a & 0x200) {
        if (a & 0x80) ReadRIOTTab[a & 0x7]();
        else          TIAReadHandler[a & 0xF]();
    } else {
        if (a & 0x80) ReadRIOTRAM();
        else          TIAReadHandler[a & 0xF]();
    }
}

static void __not_in_flash_func(TIARIOTRead)(void) {
	TIARIOTReadA(AddressBus);
}

static inline void TIARIOTWriteA(dw a) {
    if (a & 0x200) {
        if (a & 0x80) WriteRIOTHandler[a & 0x1F]();
        else          TIAWriteHandler[a & 0x3F]();
    } else {
        if (a & 0x80) WriteRIOTRAM();
        else          TIAWriteHandler[a & 0x3F]();
    }
}

static void __not_in_flash_func(TIARIOTWrite)(void) {
	TIARIOTWriteA(AddressBus);
}

static void __not_in_flash_func(BaseRead)(void) {
	if(AddressBus & 0x1000) ReadROM4K();
	else TIARIOTRead();
}

static void __not_in_flash_func(BaseWrite)(void) {
	if(AddressBus & 0x1000) WriteROM4K();
	else TIARIOTWrite();
}

void InitData() {
	ReadAccess = BaseRead;
	WriteAccess = BaseWrite;
    HotspotAdjust = 0;

	InitCVars();
	Init_CPU();
//	Init_CPUhand();
	Init_TIA();
	Init_Riot();
	Init_P2();
	Init_Starpath();
	Init_Tiasnd();
	Init_SoundQ();

	RandomizeRIOTTimer();
}
