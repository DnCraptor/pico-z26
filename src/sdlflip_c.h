/*
** sdlflip.c -- SDL page flipping code
*/

math_t	CurrentFPS = 0.0;
math_t	FPStime = 0.0;
math_t  TotalTime = 0.0;
int	    FPSflips = 0;

// monotonic clock in nanoseconds

#include <hardware/structs/systick.h>
#include <hardware/clocks.h>

static uint64_t systick_accum = 0;   // накопленные тики (64-бит)
static uint32_t systick_last;        // последнее значение CVR
static uint32_t cpu_hz;

inline static void ns_time_init(void)
{
    cpu_hz = clock_get_hz(clk_sys);

    systick_hw->rvr = 0xFFFFFF;
    systick_hw->cvr = 0;
    systick_hw->csr = (1 << 0) | (1 << 2); // ENABLE | CLKSOURCE=CPU

    systick_last = systick_hw->cvr;
}

static inline void ns_time_update(void)
{
    uint32_t now = systick_hw->cvr;

    uint32_t dticks = (systick_last >= now)
        ? (systick_last - now)
        : (systick_last + (0xFFFFFF - now) + 1);

    systick_accum += dticks;
    systick_last = now;
}
static long long int ns_time()
{
    ns_time_update();
    // перевод cycles → ns
    return (systick_accum * 1000000000ULL) / cpu_hz;
}

math_t srv_get_nanoseconds()
{
	long long int nsec;
	nsec = ns_time();
	return(nsec);
}

void srv_reset_timing()
{
	CurrentFPS = 0.0;
	FPStime = srv_get_nanoseconds();
	TotalTime = 0.0;
	FPSflips = 0;
}

void srv_Flip()
{
    if (FPSflips == 2) FPStime = srv_get_nanoseconds();

	TotalTime = srv_get_nanoseconds() - FPStime;
	CurrentFPS = ((++FPSflips-3) * 1000000000.0)/TotalTime;

///	SDL_GL_SwapWindow(window);

	if (screen_buffer_count ^= 1)
	{
		ScreenBuffer = RealScreenBuffer1;
	///	ScreenBufferPrev = RealScreenBuffer2;
	}
	else
	{
		ScreenBuffer = RealScreenBuffer1;
	///	ScreenBufferPrev = RealScreenBuffer1;
	}
}

/**
	z26 is Copyright 1997-2019 by John Saeger and contributors.  
	z26 is released subject to the terms and conditions of the 
	GNU General Public License Version 2 (GPL).	
	z26 comes with no warranty.
	Please see COPYING.TXT for details.
*/
