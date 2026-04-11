/*
	init.c -- initialize all data in emulator core modules

 z26 is Copyright 1997-2011 by John Saeger and contributors.  
 contributors.	 z26 is released subject to the terms and conditions of the 
 GNU General Public License Version 2 (GPL).  z26 comes with no warranty.
 Please see COPYING.TXT for details.
*/

void InitData() {
	int i;

	for(i = 0; i < 0x1000; i++){
		if(i & 0x200){
			if(i & 0x80){
				TIARIOTReadAccess[i] = ReadRIOTTab[i & 0x7];
				TIARIOTWriteAccess[i] = WriteRIOTHandler[i & 0x1f];
			}else{
				TIARIOTReadAccess[i] = TIAReadHandler[i & 0x0f];
				TIARIOTWriteAccess[i] = TIAWriteHandler[i & 0x3f];
			}
		}else{
			if(i & 0x80){
				TIARIOTReadAccess[i] = &ReadRIOTRAM;
				TIARIOTWriteAccess[i] = &WriteRIOTRAM;
			}else{
				TIARIOTReadAccess[i] = TIAReadHandler[i & 0x0f];
				TIARIOTWriteAccess[i] = TIAWriteHandler[i & 0x3f];
			}
		}
	}
	for(i = 0; i < 0x1000; i++){
		ReadAccess[i] = TIARIOTReadAccess[i];
		WriteAccess[i] = TIARIOTWriteAccess[i];
		ReadAccess[0x1000 + i] = &ReadROM4K;
		WriteAccess[0x1000 + i] = &WriteROM4K;
	}
	
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
