/*
** banks.c -- z26 bank switch stuff
*/

db Starpath;
db Pitfall2;

dd RomBank = 0; /* Rom bank pointer for F8 & F16 */
/* take lower 4 bit of address and subtract this to get bank number */
dd HotspotAdjust = 0;

/* Parker Brother's ROM Slices */

dd PBSlice0 = 0;
dd PBSlice1 = 0x400;
dd PBSlice2 = 0x800;

/* Tigervision 3E, 3F and 3F+ ROM Slices */

dd TVSlice0 = 0;
dd TVSlice1 = 3*0x800;
dd ROMorRAM3E = 0;

/* M-Network RAM Slices */

dd MNRamSlice = 0; /* which 256 byte ram slice */

/* CompuMate RAM state */

dd CMRamState = 0x10; /*  RAM enabled - read/write state */

/* John Payson's (supercat) 4A50 ROM slices */

dd Bank4A50Low = 0;	/* pointer for $1000-$17ff slice */
dd ROMorRAMLow = 0;	/* 0 = ROM -- 1 = RAM at $1000-$17ff */
dd Bank4A50Middle = 0;	/* pointer for $1800-$1dff slice */
dd ROMorRAMMiddle = 0;	/* 0 = ROM -- 1 = RAM at $1800-$1dff */
dd Bank4A50High = 0;	/* pointer for $1e00-$1eff slice */
dd ROMorRAMHigh = 0;	/* 0 = ROM -- 1 = RAM at $1e00-$1eFF */
db LastDataBus4A50 = 0xff;	/* state of DataBus on last cycle */
dd LastAddressBus4A50 = 0xffff;	/* state of AddressBus on last cycle */

/*

	The read and write handlers for each bankswitching scheme

	They will be called indirectly though ReadROMAccess and WriteROMAccess
	
*/

inline static void __not_in_flash_func(ReadROM2K)(void){

	DataBus = CartRom[AddressBus & 0x7ff];
}

inline static void __not_in_flash_func(ReadROM4K)(void){

	DataBus = CartRom[AddressBus & 0xfff];
}

inline static void __not_in_flash_func(WriteROM4K)(void){
	// DO NOTHING
}

inline static void __not_in_flash_func(ReadBSFE)(void){
	DataBus = CartRom[(AddressBus & 0xfff) + 0x1000];
}

inline static void __not_in_flash_func(ReadBS4K)(void){
	DataBus = CartRom[(AddressBus & 0xfff) + RomBank];
}

inline static void __not_in_flash_func(ReadHotspotBS4K)(void){
	DataBus = CartRom[(AddressBus & 0xfff) + RomBank];
	RomBank = ((AddressBus & 0x0f) - HotspotAdjust) << 12;
}

inline static void __not_in_flash_func(WriteHotspotBS4K)(void){
	RomBank = ((AddressBus & 0x0f) - HotspotAdjust) << 12;
}

inline static void __not_in_flash_func(ReadHotspotUA)(void){
	RomBank = (AddressBus & 0x40) << 6;
	TIARIOTReadA(AddressBus & 0xfff);
}

inline static void __not_in_flash_func(WriteHotspotUA)(void){
	RomBank = (AddressBus & 0x40) << 6;
	TIARIOTWriteA(AddressBus & 0xfff);
}

inline static void __not_in_flash_func(ReadHotspotMB)(void){
	DataBus = CartRom[(AddressBus & 0xfff) + RomBank];
	RomBank = (RomBank + 0x1000) & 0xf000;
}

inline static void __not_in_flash_func(WriteHotspotMB)(void){
	RomBank = (RomBank + 0x1000) & 0xf000;
}

inline static void __not_in_flash_func(ReadRAM128)(void){
	DataBus = Ram[AddressBus & 0x7f];
}

inline static void __not_in_flash_func(WriteRAM128)(void){
	Ram[AddressBus & 0x7f] = DataBus;
}

inline static void __not_in_flash_func(ReadRAM256)(void){
	DataBus = Ram[AddressBus & 0xff];
}

inline static void __not_in_flash_func(WriteRAM256)(void){
	Ram[AddressBus & 0xff] = DataBus;
}

inline static void __not_in_flash_func(ReadRAM1K)(void){
	DataBus = Ram[AddressBus & 0x3ff];
}

inline static void __not_in_flash_func(WriteRAM1K)(void){
	Ram[AddressBus & 0x3ff] = DataBus;
}

inline static void __not_in_flash_func(ReadCMhigh)(void){
	if(CMRamState & 0x10)
		DataBus = CartRom[(AddressBus & 0xfff) + RomBank];
	else DataBus = Ram[AddressBus & 0x7ff];
}

inline static void __not_in_flash_func(WriteHotspotCM)(void){
	CMRamState = DataBus;
	RomBank = (CMRamState & 0x3) << 12;
	if(DataBus & 0x20) CM_Collumn = 0;
	if(DataBus & 0x40) CM_Collumn = (CM_Collumn + 1) % 10;
	TIARIOTWriteA(0x280);
}

inline static void __not_in_flash_func(WriteCMhigh)(void){
	if ((CMRamState & 0x30) == 0x20)
		Ram[AddressBus & 0x7ff] = DataBus;
}


/*
	0 -- 2K+4K Atari [NoBS]
	
	4K of ROM fixed at $1000 - $1FFF
	2K ROMs are doubled to appear at $1000 - $17FF and at $1800 - $1FFF
*/
void Init4K(void) {
	ReadAccess = BaseRead;
	WriteAccess = BaseWrite;
	HotspotAdjust = 0;
    set_status("Mapper 4K");
}


/*
	1 -- CommaVid [CV]
	
	2K of ROM fixed at $1800 - $1FFF
	1K of RAM fixed at $1000 - $17FF
	read from RAM at $1000 - $13FF
	write to  RAM at $1400 - $17FF
*/
static void __not_in_flash_func(CVRead)(void) {
    if (AddressBus & 0x1000) {
        if (AddressBus & 0x800) ReadROM2K();
        else                    ReadRAM1K();
    } else {
        TIARIOTRead();
    }
}

static void __not_in_flash_func(CVWrite)(void) {
    if (AddressBus & 0x1000) {
        if (AddressBus & 0x800) WriteROM4K();   // ignore
        else                    WriteRAM1K();
    } else {
        TIARIOTWrite();
    }
}

void InitCV(void) {
    ReadAccess  = CVRead;
    WriteAccess = CVWrite;
    set_status("Mapper CV");
}

/*
	2 -- 8K Superchip [F8SC]
	
	2 4K ROM banks at $1000 - $1FFF
	select bank 0 by accessing $1FF8
	select bank 1 by accessing $1FF9
	128 bytes of RAM at $1000 - $10FF
	read from RAM at $1080 - $10FF
	write to  RAM at $1000 - $107F
*/
static void __not_in_flash_func(F8SCRead)(void) {
    if (AddressBus & 0x1000) {
        dw a = AddressBus & 0x1FFF;
        if (a >= 0x1FF8)        ReadHotspotBS4K();
        else if (a >= 0x1080)   ReadRAM128();
		// По спеке read window RAM — $1080-$10FF, а $1000-$107F — write window (read-trap). Здесь read-trap просто отдаёт ReadBS4K() — это намеренно или нужен отдельный обработчик?
        else                    ReadBS4K();
    } else {
        TIARIOTRead();
    }
}

static void __not_in_flash_func(F8SCWrite)(void) {
    if (AddressBus & 0x1000) {
        dw a = AddressBus & 0x1FFF;
        if (a >= 0x1FF8)        WriteHotspotBS4K();
        else if (a < 0x1080)    WriteRAM128();
        else                    WriteROM4K();
    } else {
        TIARIOTWrite();
    }
}

void InitF8SC(void) {
    ReadAccess  = F8SCRead;
    WriteAccess = F8SCWrite;
    HotspotAdjust = 8;
    set_status("Mapper F8SC");
}

/*
	3 -- 8K Parker Bros. [E0]
	
	1K of ROM fixed at $1C00 - $1FFF
	this is the last 1K (bank 7) in the ROM image
	8 1K banks of ROM switchable at $1000 - $13FF
	select bank 0 by accessing $1FE0
	select bank 1 by accessing $1FE1
	select bank 2 by accessing $1FE2
	select bank 3 by accessing $1FE3
	select bank 4 by accessing $1FE4
	select bank 5 by accessing $1FE5
	select bank 6 by accessing $1FE6
	select bank 7 by accessing $1FE7
	8 1K banks of ROM switchable at $1400 - $17FF
	select bank 0 by accessing $1FE8
	select bank 1 by accessing $1FE9
	select bank 2 by accessing $1FEA
	select bank 3 by accessing $1FEB
	select bank 4 by accessing $1FEC
	select bank 5 by accessing $1FED
	select bank 6 by accessing $1FEE
	select bank 7 by accessing $1FEF
	8 1K banks of ROM switchable at $1800 - $1BFF
	select bank 0 by accessing $1FF0
	select bank 1 by accessing $1FF1
	select bank 2 by accessing $1FF2
	select bank 3 by accessing $1FF3
	select bank 4 by accessing $1FF4
	select bank 5 by accessing $1FF5
	select bank 6 by accessing $1FF6
	select bank 7 by accessing $1FF7
	
*/
inline static void ReadHotspotE0_B0(void){
	DataBus = CartRom[(AddressBus & 0x3ff) + PBSlice0];
	PBSlice0 = (AddressBus & 0x7) << 10;
}
inline static void WriteHotspotE0_B0(void){
	PBSlice0 = (AddressBus & 0x7) << 10;
}
inline static void ReadHotspotE0_B1(void){
	DataBus = CartRom[(AddressBus & 0x3ff) + PBSlice1];
	PBSlice1 = (AddressBus & 0x7) << 10;
}
inline static void WriteHotspotE0_B1(void){
	PBSlice1 = (AddressBus & 0x7) << 10;
}
inline static void ReadHotspotE0_B2(void){
	DataBus = CartRom[(AddressBus & 0x3ff) + PBSlice2];
	PBSlice2 = (AddressBus & 0x7) << 10;
}
inline static void WriteHotspotE0_B2(void){
	PBSlice2 = (AddressBus & 0x7) << 10;
}
inline static void ReadBSE0_B0(void){
	DataBus = CartRom[(AddressBus & 0x3ff) + PBSlice0];
}
inline static void ReadBSE0_B1(void){
	DataBus = CartRom[(AddressBus & 0x3ff) + PBSlice1];
}
inline static void ReadBSE0_B2(void){
	DataBus = CartRom[(AddressBus & 0x3ff) + PBSlice2];
}
inline static void ReadBSE0_B3(void){
	DataBus = CartRom[(AddressBus & 0x3ff) + 0x1c00];
}
static void __not_in_flash_func(E0Read)(void) {
    if (AddressBus & 0x1000) {
        dw a = AddressBus & 0x1FFF;
        if (a >= 0x1FF0)      ReadHotspotE0_B2();
        else if (a >= 0x1FE8) ReadHotspotE0_B1();
        else if (a >= 0x1FE0) ReadHotspotE0_B0();
        else if (a >= 0x1C00) ReadBSE0_B3();
        else if (a >= 0x1800) ReadBSE0_B2();
        else if (a >= 0x1400) ReadBSE0_B1();
        else                  ReadBSE0_B0();
    } else {
        TIARIOTRead();
    }
}

static void __not_in_flash_func(E0Write)(void) {
    if (AddressBus & 0x1000) {
        dw a = AddressBus & 0x1FFF;
        if (a >= 0x1FF0)      WriteHotspotE0_B2();
        else if (a >= 0x1FE8) WriteHotspotE0_B1();
        else if (a >= 0x1FE0) WriteHotspotE0_B0();
        else                  WriteROM4K();
    } else {
        TIARIOTWrite();
    }
}

void InitE0(void) {
    ReadAccess  = E0Read;
    WriteAccess = E0Write;
    set_status("Mapper E0");
}

/*
	 4 -- 8K Tigervision [3F] and
	11 -- 512K extended Tigervision [3F+]
	
	up to 256 2K banks of ROM at $1000 - $17FF
	select bank by writing bank number to any address between $0000 and $003F
	[3F]  -> 2K of ROM fixed at $1800 - $ 1FFF (this is always the 4th 2K bank in the ROM)
	[3F+] -> 2K of ROM fixed at $1800 - $ 1FFF (this is always the last 2K bank in the ROM)
*/
inline static void ReadBS3Flow(void){
	DataBus = CartRom[(AddressBus & 0x7ff) + TVSlice0];
}
inline static void ReadBS3Fhigh(void){
	DataBus = CartRom[(AddressBus & 0x7ff) + TVSlice1];
}
inline static void WriteHotspot3F(void){
	TVSlice0 = DataBus << 11;
	TIARIOTWriteA(AddressBus & 0x3f);
}
static void __not_in_flash_func(F3Read)(void) {
    if (AddressBus & 0x1000) {
        if (AddressBus & 0x800) ReadBS3Fhigh();
        else                    ReadBS3Flow();
    } else {
        TIARIOTRead();
    }
}

static void __not_in_flash_func(F3Write)(void) {
    if (AddressBus & 0x1000) {
        WriteROM4K();
    } else {
        if ((AddressBus & 0x1FFF) < 0x40) WriteHotspot3F();
        else                              TIARIOTWrite();
    }
}

void Init3F(void) {
    ReadAccess  = F3Read;
    WriteAccess = F3Write;
    set_status("Mapper 3F");
}

/*
	14 -- 512K Krokodile Cart / Andrew Davie [3E]

	up to 256 2K banks of ROM at $1000 - $17FF
	select ROM bank by writing bank number to address $003F
	2K of ROM fixed at $1800 - $1FFF (this is always the 4th 2K bank in the ROM)
	32 1K banks of RAM at $1000 - $17FF
	select RAM bank by writing bank number to address $003E
	read from RAM at $1000 - $13FF
	write to  RAM at $1400 - $17FF
*/
static inline void ReadBS3Elow(void){
	if(!ROMorRAM3E) DataBus = CartRom[(AddressBus & 0x7ff) + TVSlice0];
	else DataBus = Ram[(AddressBus & 0x3ff)+ TVSlice0];
}
static inline void WriteBS3E(void){
	Ram[(AddressBus & 0x3ff) + TVSlice0] = DataBus;
}
static inline void WriteHotspot3E_E(void){
	TVSlice0 = DataBus << 10;
	ROMorRAM3E = 1;
	TIARIOTWriteA(0x3e);
}
static inline void WriteHotspot3E_F(void){
	TVSlice0 = DataBus << 11;
	ROMorRAM3E = 0;
	TIARIOTWriteA(0x3f);
}
static void __not_in_flash_func(E3Read)(void) {
    if (AddressBus & 0x1000) {
        if (AddressBus & 0x800) ReadBS3Fhigh();
        else                    ReadBS3Elow();   // $1000-$17FF включая write window
    } else {
        TIARIOTRead();
    }
}

static void __not_in_flash_func(E3Write)(void) {
    if (AddressBus & 0x1000) {
        if (AddressBus & 0x400) WriteBS3E();     // $1400-$17FF (и $1C00+ ignored)
        else                    WriteROM4K();    // $1000-$13FF ignore
    } else {
        dw a = AddressBus & 0x1FFF;
        if      (a == 0x3F) WriteHotspot3E_F();
        else if (a == 0x3E) WriteHotspot3E_E();
        else                TIARIOTWrite();
    }
}

void Init3E(void) {
    ReadAccess  = E3Read;
    WriteAccess = E3Write;
    set_status("Mapper 3E");
}

/*
	5 -- 8K Activision [FE] (flat)
	
	2 8K banks at $1000 - $1FFF
	banks are switched by JSR/RTS jumps to an address in the other bank.
	the real hardware waits for two consecutive accesses to the stack to do so.
	the $F000 - $FFFF bank is in the ROM image first
    the $D000 - $DFFF bank is last
*/
inline static void ReadFE(void) {
    uint16_t addr = AddressBus;
    uint16_t offset = addr & 0x0FFF;
    // выбор банка по старшим битам адреса (как в оригинале)
    uint8_t bank = (addr & 0x2000) ? 0 : 1;
    DataBus = CartRom[(bank << 12) | offset];
}

static void __not_in_flash_func(FERead)(void) {
    if (AddressBus & 0x1000) ReadFE();
    else                     TIARIOTRead();
}

static void __not_in_flash_func(FEWrite)(void) {
    if (AddressBus & 0x1000) WriteROM4K();
    else                     TIARIOTWrite();
}

void InitFE(void) {
    ReadAccess  = FERead;
    WriteAccess = FEWrite;
    set_status("Mapper FE");
}

/*
	6 -- 16K Superchip [F6SC]

	4 4K ROM banks at $1000 - $1FFF
	select bank 0 by accessing $1FF6
	select bank 1 by accessing $1FF7
	select bank 2 by accessing $1FF8
	select bank 3 by accessing $1FF9
	128 bytes of RAM at $1000 - $10FF
	read from RAM at $1080 - $10FF
	write to  RAM at $1000 - $107F
*/
static void __not_in_flash_func(F6SCRead)(void) {
    if (AddressBus & 0x1000) {
        dw a = AddressBus & 0x1FFF;
        if (a >= 0x1FF6)      ReadHotspotBS4K();
        else if (a >= 0x1080) ReadRAM128();
        else                  ReadBS4K();
    } else {
        TIARIOTRead();
    }
}

static void __not_in_flash_func(F6SCWrite)(void) {
    if (AddressBus & 0x1000) {
        dw a = AddressBus & 0x1FFF;
        if (a >= 0x1FF6)   WriteHotspotBS4K();
        else if (a < 0x1080) WriteRAM128();
        else                 WriteROM4K();
    } else {
        TIARIOTWrite();
    }
}

void InitF6SC(void) {
    ReadAccess  = F6SCRead;
    WriteAccess = F6SCWrite;
    HotspotAdjust = 6;
    set_status("Mapper F6SC");
}

/*
	7 -- 16K M-Network [E7]

	2K of ROM fixed at $1800 - $1FFF
	this is always the last 2K (bank 7) of the ROM image
	6 2K banks of ROM switchable at $1000 - 17FF
	select bank 0 by accessing $1FE0
	select bank 1 by accessing $1FE1
	select bank 2 by accessing $1FE2
	select bank 3 by accessing $1FE3
	select bank 4 by accessing $1FE4
	select bank 5 by accessing $1FE5
	select bank 6 by accessing $1FE6
	accessing $1FE7 selects 1K of RAM at $1000 - $17FF
	read from RAM at $1400 - $17FF
	write to  RAM at $1000 - $13FF
	there are also 4 256 bytes banks of RAM at $1800 - $19FF
	select RAM bank 0 by accessing $1FE8
	select RAM bank 1 by accessing $1FE9
	select RAM bank 2 by accessing $1FEA
	select RAM bank 3 by accessing $1FEB
	read from RAM at $1900 - $19FF
	write to  RAM at $1800 - $18FF
	
*/
inline static void ReadBSE7(void){

	if(RomBank == (0x7 * 0x800)) DataBus = Ram[(AddressBus & 0x3ff) + 0x400];
	else DataBus = CartRom[(AddressBus & 0x7ff) + RomBank];
}

inline static void WriteBSE7(void){

	if(RomBank == (0x7 * 0x800)) Ram[(AddressBus & 0x3ff) + 0x400] = DataBus;
}

inline static void ReadBSE7RAM(void){

	DataBus = Ram[(AddressBus & 0xff) + MNRamSlice];
}

inline static void WriteBSE7RAM(void){

	Ram[(AddressBus & 0xff) + MNRamSlice] = DataBus;
}

inline static void ReadBSE7last(void){

	DataBus = CartRom[(AddressBus & 0x7ff) + 0x3800];
}

inline static void ReadHotspotBSE7ROM(void){

	DataBus = CartRom[(AddressBus & 0x7ff) + 0x3800];
	RomBank = (AddressBus & 0x0f) << 11;
}

inline static void WriteHotspotBSE7ROM(void){

	RomBank = (AddressBus & 0x0f) << 11;
}

inline static void ReadHotspotBSE7RAM(void){

	DataBus = CartRom[(AddressBus & 0x7ff) + 0x3800];
	MNRamSlice = (AddressBus & 0x03) << 8;
}

inline static void WriteHotspotBSE7RAM(void){

	MNRamSlice = (AddressBus & 0x03) << 8;
}

static void __not_in_flash_func(E7Read)(void) {
    if (AddressBus & 0x1000) {
        dw a = AddressBus & 0x1FFF;
        if      (a >= 0x1FE8) ReadHotspotBSE7RAM();  // $1FE8-$1FEB
        else if (a >= 0x1FE0) ReadHotspotBSE7ROM();  // $1FE0-$1FE7
        /* $1FEC-$1FFF: не переопределялось в оригинале — под вопросом */
        else if (a >= 0x1900) ReadBSE7RAM();          // $1900-$19FF read window
        else if (a >= 0x1800) WriteBSE7RAM();         // $1800-$18FF write-trap
        else                  ReadBSE7();             // $1000-$17FF switchable
    } else {
        TIARIOTRead();
    }
}

static void __not_in_flash_func(E7Write)(void) {
    if (AddressBus & 0x1000) {
        dw a = AddressBus & 0x1FFF;
        if      (a >= 0x1FE8) WriteHotspotBSE7RAM(); // $1FE8-$1FEB
        else if (a >= 0x1FE0) WriteHotspotBSE7ROM(); // $1FE0-$1FE7
        /* $1FEC-$1FFF: не переопределялось в оригинале — под вопросом */
        else if (a >= 0x1900) ReadBSE7RAM();          // $1900-$19FF read-trap
        else if (a >= 0x1800) WriteBSE7RAM();         // $1800-$18FF write window
        else                  WriteBSE7();            // $1000-$17FF switchable
    } else {
        TIARIOTWrite();
    }
}

void InitE7(void) {
    ReadAccess  = E7Read;
    WriteAccess = E7Write;
    set_status("Mapper E7");
}

/*
	8 -- 32K Superchip [F4SC]

	8 4K ROM banks at $1000 - $1FFF
	select bank 0 by accessing $1FF4
	select bank 1 by accessing $1FF5
	select bank 2 by accessing $1FF6
	select bank 3 by accessing $1FF7
	select bank 4 by accessing $1FF8
	select bank 5 by accessing $1FF9
	select bank 6 by accessing $1FFA
	select bank 7 by accessing $1FFB
	128 bytes of RAM at $1000 - $10FF
	read from RAM at $1080 - $10FF
	write to  RAM at $1000 - $107F
*/
static void __not_in_flash_func(F4SCRead)(void) {
    if (AddressBus & 0x1000) {
        dw a = AddressBus & 0x1FFF;
        if (a >= 0x1FF4)       ReadHotspotBS4K();
        else if (a >= 0x1080)  ReadRAM128();
        else                   ReadBS4K();
    } else {
        TIARIOTRead();
    }
}

static void __not_in_flash_func(F4SCWrite)(void) {
    if (AddressBus & 0x1000) {
        dw a = AddressBus & 0x1FFF;
        if (a >= 0x1FF4)      WriteHotspotBS4K();
        else if (a < 0x1080)  WriteRAM128();
        else                  WriteROM4K();
    } else {
        TIARIOTWrite();
    }
}

void InitF4SC(void) {
    ReadAccess  = F4SCRead;
    WriteAccess = F4SCWrite;
    HotspotAdjust = 4;
    set_status("Mapper F4SC");
}

/*
	9 & 20 -- 8K Atari [F8] (9 = banks swapped)

	2 4K ROM banks at $1000 - $1FFF
	select bank 0 by accessing $1FF8
	select bank 1 by accessing $1FF9
*/
static void __not_in_flash_func(F8Read)(void) {
    if (AddressBus & 0x1000) {
        if ((AddressBus & 0x1FFF) >= 0x1FF8) ReadHotspotBS4K();
        else                                  ReadBS4K();
    } else {
        TIARIOTRead();
    }
}

static void __not_in_flash_func(F8Write)(void) {
    if (AddressBus & 0x1000) {
        if ((AddressBus & 0x1FFF) >= 0x1FF8) WriteHotspotBS4K();
        else                                  WriteROM4K();
    } else {
        TIARIOTWrite();
    }
}

void InitF8(void) {
    ReadAccess  = F8Read;
    WriteAccess = F8Write;
    HotspotAdjust = 8;
    set_status("Mapper F8");
}

/*
	10 -- Compumate [CM]

	there are 4 4K banks selectable at $1000 - $1FFFF
	bankswitching is done though the controller ports
	INPT0: D7 = CTRL key input (0 on startup / 1 = key pressed)
	INPT1: D7 = always HIGH input (tested at startup)
	INPT2: D7 = always HIGH input (tested at startup)
	INPT3: D7 = SHIFT key input (0 on startup / 1 = key pressed)
	INPT4: D7 = keyboard row 1 input (0 = key pressed)
	INPT5: D7 = keyboard row 3 input (0 = key pressed)
	SWCHA: D7 = tape recorder I/O ?
	       D6 = 1 -> increase key collumn (0 to 9)
	       D5 = 1 -> reset key collumn to 0 (if D4 = 0)
	       D5 = 0 -> enable RAM writing (if D4 = 1)
	       D4 = 1 -> map 2K of RAM at $1800 - $1fff
	       D3 = keyboard row 4 input (0 = key pressed)
	       D2 = keyboard row 2 input (0 = key pressed)
	       D1 = bank select high bit
	       D0 = bank select low bit
	
	keyboard collumn numbering:
	collumn 0 = 7 U J M
	collumn 1 = 6 Y H N
	collumn 2 = 8 I K ,
	collumn 3 = 2 W S X
	collumn 4 = 3 E D C
	collumn 5 = 0 P ENTER SPACE
	collumn 6 = 9 O L .
	collumn 7 = 5 T G B
	collumn 8 = 1 Q A Z
	collumn 9 = 4 R F V
*/
static void __not_in_flash_func(CMRead)(void) {
    if (AddressBus & 0x1000) {
        if (AddressBus & 0x800) ReadCMhigh();
        else                    ReadBS4K();
    } else {
        TIARIOTRead();
    }
}

static void __not_in_flash_func(CMWrite)(void) {
    if (AddressBus & 0x1000) {
        if (AddressBus & 0x800) WriteCMhigh();
        else                    WriteROM4K();
    } else {
        if ((AddressBus & 0x1FFF) == 0x280) WriteHotspotCM();
        else                                TIARIOTWrite();
    }
}

void InitCM(void) {
    ReadAccess  = CMRead;
    WriteAccess = CMWrite;
    set_status("Mapper CM");
}

/*
	12 -- 8K United Appliance Ltd. [UA]

	2 4K ROM banks at $1000 - $1FFF
	select bank 0 by accessing $0220
	select bank 1 by accessing $0240
*/
static void __not_in_flash_func(UARead)(void) {
    if (AddressBus & 0x1000) {
        ReadBS4K();
    } else {
        dw a = AddressBus & 0x1FFF;
        if (a == 0x220 || a == 0x240) ReadHotspotUA();
        else                          TIARIOTRead();
    }
}

static void __not_in_flash_func(UAWrite)(void) {
    if (AddressBus & 0x1000) {
        WriteROM4K();
    } else {
        dw a = AddressBus & 0x1FFF;
        if (a == 0x220 || a == 0x240) WriteHotspotUA();
        else                          TIARIOTWrite();
    }
}

void InitUA(void) {
    ReadAccess  = UARead;
    WriteAccess = UAWrite;
    set_status("Mapper UA");
}

/*
	13 -- 64K Homestar Runner / Paul Slocum [EF]

	16 4K ROM banks at $1000 - $1FFF
	select bank  0 by accessing $1FE0
	select bank  1 by accessing $1FE1
	select bank  2 by accessing $1FE2
	select bank  3 by accessing $1FE3
	select bank  4 by accessing $1FE4
	select bank  5 by accessing $1FE5
	select bank  6 by accessing $1FE6
	select bank  7 by accessing $1FE7
	select bank  8 by accessing $1FE8
	select bank  9 by accessing $1FE9
	select bank 10 by accessing $1FEA
	select bank 11 by accessing $1FEB
	select bank 12 by accessing $1FEC
	select bank 13 by accessing $1FED
	select bank 14 by accessing $1FEE
	select bank 15 by accessing $1FEF
*/
static void __not_in_flash_func(EFRead)(void) {
    if (AddressBus & 0x1000) {
        if ((AddressBus & 0x1FFF) >= 0x1FE0) ReadHotspotBS4K();
        else                                  ReadBS4K();
    } else {
        TIARIOTRead();
    }
}

static void __not_in_flash_func(EFWrite)(void) {
    if (AddressBus & 0x1000) {
        if ((AddressBus & 0x1FFF) >= 0x1FE0) WriteHotspotBS4K();
        else                                  WriteROM4K();
    } else {
        TIARIOTWrite();
    }
}

void InitEF(void) {
    ReadAccess  = EFRead;
    WriteAccess = EFWrite;
    HotspotAdjust = 0;
    set_status("Mapper EF");
}

/*
	15 -- Starpath [AR]

	the Supercharger has 2K of ROM and 3 2K banks of RAM
	address $1FF9 is the audio input for the tape loader
	address $1FF8 is the control byte:
	D7 - D5 = write pulse delay (needed for hardware timing - not emulated)
	D4 - D2 = memory config for $1000 - $17FF and $1800 - $1FFF:
	          000 = RAM 3 + ROM
	          001 = RAM 1 + ROM
	          010 = RAM 3 + RAM 1
	          011 = RAM 1 + RAM 3
	          100 = RAM 3 + ROM
	          101 = RAM 2 + ROM
	          110 = RAM 3 + RAM 2
	          111 = RAM 2 + RAM 3
	D1 = 1 -> writing to RAM allowed
	D0 = 1 -> power off ROM (to save energy when it's not needed)
	
	to latch a write value you have to access a address in $1000 - $10FF.
	the low byte of the address will be the value to write.
	the write will happen to the address that is accessed 5 address changes later.
	writes will only happen, if writing is enabled and the destination is in SC-RAM.
	writes to $1FF8 will alway happen immediately. (no address-change counting needed) 
*/

/* These functions are handled in starpath.c */
void InitSP(void);


/*
	16 -- 16K Atari [F6]

	4 4K ROM banks at $1000 - $1FFF
	select bank 0 by accessing $1FF6
	select bank 1 by accessing $1FF7
	select bank 2 by accessing $1FF8
	select bank 3 by accessing $1FF9
*/
static void __not_in_flash_func(F6Read)(void) {
    if (AddressBus & 0x1000) {
        if ((AddressBus & 0x1FFF) >= 0x1FF6) ReadHotspotBS4K();
        else                                 ReadBS4K();
    } else {
        TIARIOTRead();
    }
}

static void __not_in_flash_func(F6Write)(void) {
    if (AddressBus & 0x1000) {
        if ((AddressBus & 0x1FFF) >= 0x1FF6) WriteHotspotBS4K();
        else                                 WriteROM4K();
    } else {
        TIARIOTWrite();
    }
}

void InitF6(void) {
    ReadAccess  = F6Read;
    WriteAccess = F6Write;
    HotspotAdjust = 6;
    set_status("Mapper F6");
}

/*
	17 -- 32K Atari [F4]

	8 4K ROM banks at $1000 - $1FFF
	select bank 0 by accessing $1FF4
	select bank 1 by accessing $1FF5
	select bank 2 by accessing $1FF6
	select bank 3 by accessing $1FF7
	select bank 4 by accessing $1FF8
	select bank 5 by accessing $1FF9
	select bank 6 by accessing $1FFA
	select bank 7 by accessing $1FFB
*/
static void __not_in_flash_func(F4Read)(void) {
    if (AddressBus & 0x1000) {
        if ((AddressBus & 0x1FFF) >= 0x1FF4) ReadHotspotBS4K();
        else                                 ReadBS4K();
    } else {
        TIARIOTRead();
    }
}

static void __not_in_flash_func(F4Write)(void) {
    if (AddressBus & 0x1000) {
        if ((AddressBus & 0x1FFF) >= 0x1FF4) WriteHotspotBS4K();
        else                                 WriteROM4K();
    } else {
        TIARIOTWrite();
    }
}

void InitF4(void) {
    ReadAccess  = F4Read;
    WriteAccess = F4Write;
    HotspotAdjust = 4;
    set_status("Mapper F4");
}

/*
	18 -- 64K Megaboy [MB]

	16 4K ROM banks at $1000 - $1FFF
	select banks by repeatedly accessing $1FF0
*/
static void __not_in_flash_func(MBRead)(void) {
    if (AddressBus & 0x1000) {
        if ((AddressBus & 0x1FFF) == 0x1FF0) ReadHotspotMB();
        else                                  ReadBS4K();
    } else {
        TIARIOTRead();
    }
}

static void __not_in_flash_func(MBWrite)(void) {
    if (AddressBus & 0x1000) {
        if ((AddressBus & 0x1FFF) == 0x1FF0) WriteHotspotMB();
        else                                  WriteROM4K();
    } else {
        TIARIOTWrite();
    }
}

void InitMB(void) {
    ReadAccess  = MBRead;
    WriteAccess = MBWrite;
    HotspotAdjust = 6;
    set_status("Mapper MB");
}

/*
	19 -- 12K CBS [FA]

	3 4K ROM banks at $1000 - $1FFF
	select bank 0 by accessing $1FF8
	select bank 1 by accessing $1FF9
	select bank 2 by accessing $1FFA
	256 bytes of RAM at $1000 - $11FF
	read from RAM at $1100 - $11FF
	write to  RAM at $1000 - $10FF
*/
static void __not_in_flash_func(FARead)(void) {
    if (AddressBus & 0x1000) {
        dw a = AddressBus & 0x1FFF;
        if (a >= 0x1FF8)       ReadHotspotBS4K();
        else if (a >= 0x1100)  ReadRAM256();
        else                   ReadBS4K();
    } else {
        TIARIOTRead();
    }
}

static void __not_in_flash_func(FAWrite)(void) {
    if (AddressBus & 0x1000) {
        dw a = AddressBus & 0x1FFF;
        if (a >= 0x1FF8)      WriteHotspotBS4K();
        else if (a < 0x1100)  WriteRAM256();
        else                  WriteROM4K();
    } else {
        TIARIOTWrite();
    }
}

void InitFA(void) {
    ReadAccess  = FARead;
    WriteAccess = FAWrite;
    HotspotAdjust = 8;
    set_status("Mapper FA");
}

/*
	21 -- 8K+DPC Pitfall2 [P2]
	
	2 4K ROM banks at $1000 - $1FFF
	select bank 0 by accessing $1FF8
	select bank 1 by accessing $1FF9
	the DPC has:
		2K of graphics ROM,
		a random number genarator
		and a sample generator for 3 combined squarewave sound channels
	DPC read ports range from $1000 to $103F
	DPC write ports range from $1040 to $107F
	see David Crane's U.S. Patent 4,644,495, Feb 17,1987 for details
*/

//	These functions are handled in pitfall2.c
void InitP2(void);


/*
	22 -- 4A50

	see http://www.casperkitty.com/stella/cartfmt.htm for current format description
*/

void RBank4A50(void)
{
	if(!(AddressBus & 0x1000)){
		ReadHardware();
		if(((LastDataBus4A50 & 0xe0) == 0x60) && ((LastAddressBus4A50 >= 0x1000) || (LastAddressBus4A50 < 0x200))){
			if((AddressBus & 0x0f00) == 0x0c00){
				ROMorRAMHigh = 0;
				Bank4A50High = (AddressBus & 0xff) << 8;
			}else if((AddressBus & 0x0f00) == 0x0d00){
				ROMorRAMHigh = 1;
				Bank4A50High = (AddressBus & 0x7f) << 8;
			}else if((AddressBus & 0x0f40) == 0x0e00){
				ROMorRAMLow = 0;
				Bank4A50Low = (AddressBus & 0x1f) << 11;
			}else if((AddressBus & 0x0f40) == 0x0e40){
				ROMorRAMLow = 1;
				Bank4A50Low = (AddressBus & 0xf) << 11;
			}else  if((AddressBus & 0x0f40) == 0x0f00){
				ROMorRAMMiddle = 0;
				Bank4A50Middle = (AddressBus & 0x1f) << 11;
			}else if((AddressBus & 0x0f50) == 0x0f40){
				ROMorRAMMiddle = 1;
				Bank4A50Middle = (AddressBus & 0xf) << 11;
			}else if((AddressBus & 0x0f00) == 0x0400){
				Bank4A50Low = Bank4A50Low ^ 0x800;
			}else if((AddressBus & 0x0f00) == 0x0500){
				Bank4A50Low = Bank4A50Low ^ 0x1000;
			}else if((AddressBus & 0x0f00) == 0x0800){
				Bank4A50Middle = Bank4A50Middle ^ 0x800;
			}else if((AddressBus & 0x0f00) == 0x0900){
				Bank4A50Middle = Bank4A50Middle ^ 0x1000;
			}
		}
		if((AddressBus & 0xf75) == 0x74){
			ROMorRAMHigh = 0;
			Bank4A50High = DataBus << 8;
		}else if((AddressBus & 0xf75) == 0x75){
			ROMorRAMHigh = 1;
			Bank4A50High = (DataBus & 0x7f) << 8;
		}else if((AddressBus & 0xf7c) == 0x78){
			if((DataBus & 0xf0) == 0){
				ROMorRAMLow = 0;
				Bank4A50Low = (DataBus & 0xf) << 11;			
			}else if((DataBus & 0xf0) == 0x40){
				ROMorRAMLow = 1;
				Bank4A50Low = (DataBus & 0xf) << 11;			
			}else if((DataBus & 0xf0) == 0x90){
				ROMorRAMMiddle = 0;
				Bank4A50Middle = ((DataBus & 0xf) | 0x10) << 11;							
			}else if((DataBus & 0xf0) == 0xc0){
				ROMorRAMMiddle = 1;
				Bank4A50Middle = (DataBus & 0xf) << 11;							
			}
		}
	}else{
		if((AddressBus & 0x1800) == 0x1000){
			if(ROMorRAMLow) DataBus = Ram[(AddressBus & 0x7ff) + Bank4A50Low];
			else DataBus = CartRom[(AddressBus & 0x7ff) + Bank4A50Low];
		}else if(((AddressBus & 0x1fff) >= 0x1800) && ((AddressBus & 0x1fff) <= 0x1dff)){
			if(ROMorRAMMiddle) DataBus = Ram[(AddressBus & 0x7ff) + Bank4A50Middle];
			else DataBus = CartRom[(AddressBus & 0x7ff) + Bank4A50Middle];
		}else if((AddressBus & 0x1f00) == 0x1e00){
			if(ROMorRAMHigh) DataBus = Ram[(AddressBus & 0xff) + Bank4A50High];
			else DataBus = CartRom[(AddressBus & 0xff) + Bank4A50High];
		}else if((AddressBus & 0x1f00) == 0x1f00){
			DataBus = CartRom[(AddressBus & 0xff) + 0xff00];
		  	if(((LastDataBus4A50 & 0xe0) == 0x60) && ((LastAddressBus4A50 >= 0x1000) || (LastAddressBus4A50 < 0x200)))
				Bank4A50High = (Bank4A50High & 0xf0ff) | ((AddressBus & 0x8) << 8) | ((AddressBus & 0x70) << 4);
		}
	}
	LastDataBus4A50 = DataBus;
	LastAddressBus4A50 = AddressBus & 0x1fff;
}

void WBank4A50(void)
{
	if(!(AddressBus & 0x1000)){
		WriteHardware();
		if(((LastDataBus4A50 & 0xe0) == 0x60) && ((LastAddressBus4A50 >= 0x1000) || (LastAddressBus4A50 < 0x200))){
			if((AddressBus & 0x0f00) == 0x0c00){
				ROMorRAMHigh = 0;
				Bank4A50High = (AddressBus & 0xff) << 8;
			}else if((AddressBus & 0x0f00) == 0x0d00){
				ROMorRAMHigh = 1;
				Bank4A50High = (AddressBus & 0x7f) << 8;
			}else if((AddressBus & 0x0f40) == 0x0e00){
				ROMorRAMLow = 0;
				Bank4A50Low = (AddressBus & 0x1f) << 11;
			}else if((AddressBus & 0x0f40) == 0x0e40){
				ROMorRAMLow = 1;
				Bank4A50Low = (AddressBus & 0xf) << 11;
			}else  if((AddressBus & 0x0f40) == 0x0f00){
				ROMorRAMMiddle = 0;
				Bank4A50Middle = (AddressBus & 0x1f) << 11;
			}else if((AddressBus & 0x0f50) == 0x0f40){
				ROMorRAMMiddle = 1;
				Bank4A50Middle = (AddressBus & 0xf) << 11;
			}else if((AddressBus & 0x0f00) == 0x0400){
				Bank4A50Low = Bank4A50Low ^ 0x800;
			}else if((AddressBus & 0x0f00) == 0x0500){
				Bank4A50Low = Bank4A50Low ^ 0x1000;
			}else if((AddressBus & 0x0f00) == 0x0800){
				Bank4A50Middle = Bank4A50Middle ^ 0x800;
			}else if((AddressBus & 0x0f00) == 0x0900){
				Bank4A50Middle = Bank4A50Middle ^ 0x1000;
			}
		}
		if((AddressBus & 0xf75) == 0x74){
			ROMorRAMHigh = 0;
			Bank4A50High = DataBus << 8;
		}else if((AddressBus & 0xf75) == 0x75){
			ROMorRAMHigh = 1;
			Bank4A50High = (DataBus & 0x7f) << 8;
		}else if((AddressBus & 0xf7c) == 0x78){
			if((DataBus & 0xf0) == 0){
				ROMorRAMLow = 0;
				Bank4A50Low = (DataBus & 0xf) << 11;			
			}else if((DataBus & 0xf0) == 0x40){
				ROMorRAMLow = 1;
				Bank4A50Low = (DataBus & 0xf) << 11;			
			}else if((DataBus & 0xf0) == 0x90){
				ROMorRAMMiddle = 0;
				Bank4A50Middle = ((DataBus & 0xf) | 0x10) << 11;							
			}else if((DataBus & 0xf0) == 0xc0){
				ROMorRAMMiddle = 1;
				Bank4A50Middle = (DataBus & 0xf) << 11;							
			}
		}
	}else{
		if((AddressBus & 0x1800) == 0x1000){
			if(ROMorRAMLow) Ram[(AddressBus & 0x7ff) + Bank4A50Low] = DataBus;
		}else if(((AddressBus & 0x1fff) >= 0x1800) && ((AddressBus & 0x1fff) <= 0x1dff)){
			if(ROMorRAMMiddle) Ram[(AddressBus & 0x7ff) + Bank4A50Middle] = DataBus;
		}else if((AddressBus & 0x1f00) == 0x1e00){
			if(ROMorRAMHigh) Ram[(AddressBus & 0xff) + Bank4A50High] = DataBus;
		}else if((AddressBus & 0x1f00) == 0x1f00){
		  	if(((LastDataBus4A50 & 0xe0) == 0x60) && ((LastAddressBus4A50 >= 0x1000) || (LastAddressBus4A50 < 0x200)))
				Bank4A50High = (Bank4A50High & 0xf0ff) | ((AddressBus & 0x8) << 8) | ((AddressBus & 0x70) << 4);
		}
	}
	LastDataBus4A50 = DataBus;
	LastAddressBus4A50 = AddressBus & 0x1fff;
}


/*
	23 -- 0840 EconoBanking / supercat

	2 4K ROM banks at $1000 - $1FFF
	select bank 0 by accessing %xxx0 1xxx x0xx xxxx ($0800)
	select bank 1 by accessing %xxx0 1xxx x1xx xxxx ($0840)
*/
static void __not_in_flash_func(C0840Read)(void) {
    if (AddressBus & 0x1000) {
        ReadBS4K();
    } else {
        if (AddressBus & 0x800) ReadHotspotUA();
        else                    TIARIOTRead();
    }
}

static void __not_in_flash_func(C0840Write)(void) {
    if (AddressBus & 0x1000) {
        WriteROM4K();
    } else {
        if (AddressBus & 0x800) WriteHotspotUA();
        else                    TIARIOTWrite();
    }
}

void Init0840(void) {
    ReadAccess  = C0840Read;
    WriteAccess = C0840Write;
    set_status("Mapper 0840");
}


void (* InitMemoryMap[24])(void) = {
	Init4K,		//   0 -- 4K Atari
	InitCV,		//   1 -- Commavid
	InitF8SC,	//   2 -- 8K superchip
	InitE0,		//   3 -- 8K Parker Bros.
	Init3F,		//   4 -- 8K Tigervision
	InitFE,		//   5 -- 8K Flat
	InitF6SC,	//   6 -- 16K superchip
	InitE7,		//   7 -- 16K M-Network
	InitF4SC,	//   8 -- 32K superchip
	InitF8,		//   9 -- 8K Atari - banks swapped
	InitCM,		//  10 -- Compumate
	Init3F,		//  11 -- 512K Tigervision
	InitUA,		//  12 -- 8K UA Ltd.
	InitEF,		//  13 -- 64K Homestar Runner / Paul Slocum
	Init3E,		//  14 -- 3E bankswitching (large 3F + 32k RAM)
	InitSP,		//  15 -- Starpath
	InitF6,		//  16 -- 16K Atari
	InitF4,		//  17 -- 32K Atari
	InitMB,		//  18 -- Megaboy
	InitFA,		//  19 -- 12K
	InitF8,		//  20 -- 8K Atari
	InitP2,		//  21 -- Pitfall2
	Init4K,		//	22 -- 4A50 / supercat
	Init0840	//	23 -- 0840 EconoBanking
};


void DetectBySize() {

//	int i, j;

	if( CartSize == 480*0x400 ) {
		BSType = Z26_FE;	/* 3E bankswitching with extra RAM */
		return;
	}
		
	if( CartSize % 8448 == 0 ) { /* multiple of 8448 bytes? */
		SetStarpath(); /* Supercharger image */
		return;
	}

	if( CartSize > 0x10000 ) {
		BSType = Z26_3FP; /* large TigerVision game */
		return;
	}

	switch(CartSize) {
		case 0x2000: /* 8k cart */
			{
				RomBank = 0x1000; /* need this for moonsweep and lancelot */
				BSType = Z26_F8S;
				if (!detect_super_chip(CartRom, CartSize)) BSType = Z26_F8;
				break;
			}

		case 0x3000: /* 12k cart */
			{
				BSType = Z26_FA;
				break;
			}

		case 0x4000: /* 16k cart */
			{
				BSType = Z26_F6S;
				if (!detect_super_chip(CartRom, CartSize)) BSType = Z26_F6;
				break;
			}

		case 0x8000: /* 32k cart */
			{
				BSType = Z26_F4S;
				if (!detect_super_chip(CartRom, CartSize)) BSType = Z26_F4;
				break;
			}

		case 0x28ff: /* Pitfall II cart */
			{
				SetPitfallII();
				break;
			}

		case 0x10000:
			{
				if(((CartRom[0xfffd] & 0x1f) == 0x1f) &&
				   (CartRom[CartRom[0xfffd] * 256 + CartRom[0xfffc]] == 0x0c) &&
				   ((CartRom[CartRom[0xfffd] * 256 + CartRom[0xfffc] + 2] & 0xfe) == 0x6e))
				   /* program starts at $1Fxx with NOP $6Exx or NOP $6Fxx ? */
					BSType = Z26_4A5; /* 64K cart with 4A50 bankswitching */
				else BSType = Z26_DC; /* Megaboy 64k cart */
				break;
			}

		case 6144: /* Supercharger image */
			{
				SetStarpath();
				break;
			}

		default: /* 4k (non bank-switched)? */
			break;
	}
}

void Show_BSType(void)
{
	switch (BSType)
	{
	case Z26_4K:	printf("4K,"); break;
	case Z26_CV:	printf("CV,"); break;
	case Z26_F8S: 	printf("F8S,"); break;
	case Z26_E0:	printf("E0,"); break;
	case Z26_3F:	printf("3F,"); break;
	case Z26_FE:	printf("FE,"); break;
	case Z26_F6S: 	printf("F6S,"); break;
	case Z26_E7:	printf("E7,"); break;
	case Z26_F4S: 	printf("F4S,"); break;
	case Z26_F8SW:  printf("F8SW,"); break;
	case Z26_CM:	printf("CM,"); break;
	case Z26_3FP: 	printf("3FP,"); break;
	case Z26_UA:	printf("UA,"); break;
	case Z26_EF:	printf("EF,"); break;
	case Z26_3E:	printf("3E,"); break;
	case Z26_AR:	printf("AR,"); break;
	case Z26_F6:	printf("F6,"); break;
	case Z26_F4:	printf("F4,"); break;
	case Z26_DC:	printf("DC,"); break;
	case Z26_FA:	printf("FA,"); break;
	case Z26_F8:	printf("F8,"); break;
	case Z26_DPC: 	printf("DPC,"); break;
	case Z26_4A5: 	printf("4A5,"); break;
	case Z26_084: 	printf("084,"); break;
	default: break;
	}
	fflush(stdout);
}

/* setup bank switching scheme */
void SetupBanks() {
	int i;

	RomBank = 0;
	PBSlice0 = 0;
	PBSlice1 = 1 * 0x400;
	PBSlice2 = 2 * 0x400;
	TVSlice0 = 0;
	TVSlice1 = 3 * 0x800;
	MNRamSlice = 0;
	Pitfall2 = 0;
	Starpath = 0;
	Bank4A50Low = 0;
	ROMorRAMLow = 0;
	Bank4A50Middle = 0;
	ROMorRAMMiddle = 0;
	Bank4A50High = 0;
	ROMorRAMHigh = 0;
	LastDataBus4A50 = 0xff;
	LastAddressBus4A50 = 0xffff;

	if (BSType == 0) BSType = identify_cart_type(CartRom, CartSize);

	if( BSType == 0 )
		DetectBySize();
	else if( BSType == 1 ) {
		/* CommaVid RAM games might have RAM state stored in first 2K of ROM */
		for(i=0; i<2048; i++)
			Ram[i] = CartRom[i];
	}
	else if( BSType == 10 ) {
		RomBank = 0x3000;
		InitCompuMate();
	}

//	Show_BSType();

	/* make last 2k bank fixed for 3E and 3F+ games: */
	if ((BSType == 11) || (BSType == 14))
		TVSlice1 = CartSize - 2048;

	/* fill memory map with pointers to proper handler functions */
	(* InitMemoryMap[BSType])();
}

/**
	z26 is Copyright 1997-2019 by John Saeger and contributors.  
	z26 is released subject to the terms and conditions of the 
	GNU General Public License Version 2 (GPL).	
	z26 comes with no warranty.
	Please see COPYING.TXT for details.
*/
