/*
	2600core.c -- c_emulator() main entry point here...
*/

void SetStarpath(void);
void SetPitfallII(void);

#include "z_tiatab_c.h"
#include "z_tiavars_c.h"
#include "c_trace_c.h"
#include "c_riot_c.h"
#include "z_defs_c.h"
#include "c_tiasnd_c.h"
#include "z_tiawrite_c.h"
#include "z_tialine_c.h"
#include "c_soundq_c.h"
#include "c_cpuhand_c.h"
#include "c_banks_c.h"
#include "c_pitfall2_c.h"
#include "c_starpath_c.h"
#include "c_cpujam_c.h"
#include "c_cpu_c.h"
#include "c_init_c.h"

/*
 routine to blank the remains of the screen buffer, if not all of the
 displayed data gets filled by the rendering routine
*/

void BlankBufferEnd(void) {
	db *PTR;
	
	PTR = (db*) DisplayPointer;
	if(LinesInFrame > 100)
	{	
		while (PTR < ScreenBuffer + MaxLines*tiawidth)
		{
			*PTR++ = 0;
		}
	}
}

/*
	Do one frame
*/

#define BAILOUT_LINE 400

void ScanFrame() 
{
	DisplayPointer = ScreenBuffer;

	do {
		nTIALineTo();						/* Generate a raster line */
		ScanLine++;
		RClock -= CYCLESPERSCANLINE;		/* adjust RClock for next line */

		if( !(VBlank & 0x80) )				/* if charging capacitors... */
			if(ChargeCounter < 0x80000000)	/* and if not fully charged... */
				ChargeCounter++;			/* add some charge. */

		if( ScanLine >= BAILOUT_LINE) {
			PrevLinesInFrame = LinesInFrame;
			LinesInFrame = ScanLine-1;

			Frame++;
			ScanLine = 1;
		}
		if (VSyncCount > 500) { 			/* keep lots of VSyncs ... */
			++Frame;						/* ... from hanging emulator */
			VSyncCount = 0;
		}
		if (ExitEmulator) break;
	} while (Frame == PrevFrame); 			/* Frame is updated by tiawrite.c */

	BlankBufferEnd();
	PrevFrame = Frame;
}


void Reset_emulator(void)
{
	ResetEmulator = 0;
	InitData();
	RecognizeCart();
	SetupBanks();
	Reset();
	Init_Service();
	Controls();
}


/* 
	Entry point. 
*/

void c_emulator(void) {

	InitData();
	Init_Service();
	Controls();

	/* ExitEmulator gets set by Controls() if the user presses Escape */
	while( !ExitEmulator ) 
	{
	if(ResetEmulator) Reset_emulator();

		srv_Events();
		if(srv_done) break;	/* SDL got a 'close window' message */

		ScanFrame();
		Controls();
		srv_CopyScreen();
		while(GamePaused) Controls();
	}
	
	srv_Cleanup();	/* shutdown SDL */
}


/**
 z26 is Copyright 1997-2019 by John Saeger and contributors.  
 z26 is released subject to the terms and conditions of the 
 GNU General Public License Version 2 (GPL).  
 z26 comes with no warranty.
 Please see COPYING.TXT for details.
*/
