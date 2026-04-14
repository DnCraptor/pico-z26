#pragma GCC optimize("Ofast")

/*
**	z26 -- an Atari 2600 emulator
*/

#define Z26_RELEASE "z26 -- An Atari 2600 Emulator"

#ifdef PICO_RP2350
#include <hardware/regs/qmi.h>
#include <hardware/structs/qmi.h>
#endif

#include <cstdio>
#include <cstring>
#include <pico.h>
#include <pico/stdlib.h>
#include <pico/multicore.h>
#include <hardware/clocks.h>
#include <hardware/vreg.h>
#include <hardware/flash.h>
#include <hardware/watchdog.h>

#include <graphics.h>
#include "audio.h"
#include "main.h"

#include "nespad.h"
#include "ff.h"
#include "ps2kbd_mrmltr.h"

#define HOME_DIR "\\z26"
extern char __flash_binary_end;
#define FLASH_TARGET_OFFSET (((((uintptr_t)&__flash_binary_end - XIP_BASE) / FLASH_SECTOR_SIZE) + 4) * FLASH_SECTOR_SIZE)

extern "C" {
const char* homedir = HOME_DIR;
const char* z26cli = HOME_DIR "\\z26.cli";
const char* z26gui = HOME_DIR "\\z26.gui";
const char* z26home = HOME_DIR;
const char* z26log = HOME_DIR "\\z26.log";
const char* z26wav = HOME_DIR "\\z26.wav";

typedef float math_t;
typedef unsigned long long int  dq;
typedef unsigned int			dd;
typedef unsigned short int		dw;
typedef unsigned char			db;

static db* CartRom = (db*)(XIP_BASE + FLASH_TARGET_OFFSET); // TODO: const

void position_game(void);
void show_transient_status(void);
void set_status(char *status);
void show_scanlines(void);
void srv_print(char *msg);
void draw_trace_column_headers(void);

#include <math.h>
#include <ctype.h>
#include "fatfs_stdio.h"

#include "globals_c.h"
#include "palette_c.h"
#include "vcs_slot_c.h"
#include "ct_c.h"
#include "carts_c.h"
#include "cli_c.h"
#include "kidvid_c.h"
#include "sdlsrv_c.h"
#include "text_c.h"
#include "controls_c.h"
#include "position_c.h"
/*
#include "gui.c"
*/
#include "2600core_c.h"
}

char __uninitialized_ram(filename[256]);
static uint32_t __uninitialized_ram(rom_size) = 0;

static FATFS fs;
bool reboot = false;
semaphore vga_start_semaphore;

SETTINGS settings = {
    .version = 3,
    .swap_ab = false,
    .save_slot = 0,
    .last_path = "\\z26",
    .last_file = "",
    .last_offset = 0,
    .last_item = 0,
};

kbd_t keyboard = {
    .bits = { false, false, false, false, false, false, false, false },
    .h_code = -1
};
input_bits_t gamepad1_bits = { false, false, false, false, false, false, false, false };
input_bits_t gamepad2_bits = { false, false, false, false, false, false, false, false };

bool swap_ab = false;

void nespad_tick() {
    nespad_read();
    if (((nespad_state & DPAD_LEFT) && (nespad_state & DPAD_RIGHT)) ||
        ((nespad_state & DPAD_DOWN) && (nespad_state & DPAD_UP))
    ) {
        nespad_state = 0;
    }

    if (settings.swap_ab) {
        gamepad1_bits.b = keyboard.bits.a || (nespad_state & DPAD_A) != 0;
        gamepad1_bits.a = keyboard.bits.b || (nespad_state & DPAD_B) != 0;
    } else {
        gamepad1_bits.a = keyboard.bits.a || (nespad_state & DPAD_A) != 0;
        gamepad1_bits.b = keyboard.bits.b || (nespad_state & DPAD_B) != 0;

    }

    gamepad1_bits.select = keyboard.bits.select || (nespad_state & DPAD_SELECT) != 0;
    gamepad1_bits.start = keyboard.bits.start || (nespad_state & DPAD_START) != 0;
    gamepad1_bits.up = keyboard.bits.up || (nespad_state & DPAD_UP) != 0;
    gamepad1_bits.down = keyboard.bits.down || (nespad_state & DPAD_DOWN) != 0;
    gamepad1_bits.left = keyboard.bits.left || (nespad_state & DPAD_LEFT) != 0;
    gamepad1_bits.right = keyboard.bits.right || (nespad_state & DPAD_RIGHT) != 0;
}

static bool isInReport(hid_keyboard_report_t const* report, const unsigned char keycode) {
    for (unsigned char i: report->keycode) {
        if (i == keycode) {
            return true;
        }
    }
    return false;
}

static volatile bool altPressed = false;
static volatile bool ctrlPressed = false;
static volatile uint8_t fxPressedV = 0;

void
__not_in_flash_func(process_kbd_report)(hid_keyboard_report_t const* report, hid_keyboard_report_t const* prev_report) {
    /* printf("HID key report modifiers %2.2X report ", report->modifier);
    for (unsigned char i: report->keycode)
        printf("%2.2X", i);
    printf("\r\n");
     */
    uint8_t h_code = -1;
    if ( isInReport(report, HID_KEY_0) || isInReport(report, HID_KEY_KEYPAD_0)) h_code = 0;
    else if ( isInReport(report, HID_KEY_1) || isInReport(report, HID_KEY_KEYPAD_1)) h_code = 1;
    else if ( isInReport(report, HID_KEY_2) || isInReport(report, HID_KEY_KEYPAD_2)) h_code = 2;
    else if ( isInReport(report, HID_KEY_3) || isInReport(report, HID_KEY_KEYPAD_3)) h_code = 3;
    else if ( isInReport(report, HID_KEY_4) || isInReport(report, HID_KEY_KEYPAD_4)) h_code = 4;
    else if ( isInReport(report, HID_KEY_5) || isInReport(report, HID_KEY_KEYPAD_5)) h_code = 5;
    else if ( isInReport(report, HID_KEY_6) || isInReport(report, HID_KEY_KEYPAD_6)) h_code = 6;
    else if ( isInReport(report, HID_KEY_7) || isInReport(report, HID_KEY_KEYPAD_7)) h_code = 7;
    else if ( isInReport(report, HID_KEY_8) || isInReport(report, HID_KEY_KEYPAD_8)) h_code = 8;
    else if ( isInReport(report, HID_KEY_9) || isInReport(report, HID_KEY_KEYPAD_9)) h_code = 9;
    else if ( isInReport(report, HID_KEY_A)) h_code = 10;
    else if ( isInReport(report, HID_KEY_B)) h_code = 11;
    else if ( isInReport(report, HID_KEY_C)) h_code = 12;
    else if ( isInReport(report, HID_KEY_D)) h_code = 13;
    else if ( isInReport(report, HID_KEY_E)) h_code = 14;
    else if ( isInReport(report, HID_KEY_F)) h_code = 15;
    keyboard.h_code = h_code;
    keyboard.bits.start = isInReport(report, HID_KEY_ENTER) || isInReport(report, HID_KEY_KEYPAD_ENTER);
    keyboard.bits.select = isInReport(report, HID_KEY_BACKSPACE) || isInReport(report, HID_KEY_ESCAPE) || isInReport(report, HID_KEY_KEYPAD_ADD);
    keyboard.delete_key  = isInReport(report, HID_KEY_DELETE) || isInReport(report, HID_KEY_KEYPAD_DECIMAL);

    keyboard.bits.a = isInReport(report, HID_KEY_Z) || isInReport(report, HID_KEY_O) || isInReport(report, HID_KEY_KEYPAD_0);
    keyboard.bits.b = isInReport(report, HID_KEY_X) || isInReport(report, HID_KEY_P) || isInReport(report, HID_KEY_KEYPAD_DECIMAL);

    bool b7 = isInReport(report, HID_KEY_KEYPAD_7);
    bool b9 = isInReport(report, HID_KEY_KEYPAD_9);
    bool b1 = isInReport(report, HID_KEY_KEYPAD_1);
    bool b3 = isInReport(report, HID_KEY_KEYPAD_3);

    keyboard.bits.up = b7 || b9 || isInReport(report, HID_KEY_ARROW_UP) || isInReport(report, HID_KEY_W) || isInReport(report, HID_KEY_KEYPAD_8);
    keyboard.bits.down = b1 || b3 || isInReport(report, HID_KEY_ARROW_DOWN) || isInReport(report, HID_KEY_S) || isInReport(report, HID_KEY_KEYPAD_2) || isInReport(report, HID_KEY_KEYPAD_5);
    keyboard.bits.left = b7 || b1 || isInReport(report, HID_KEY_ARROW_LEFT) || isInReport(report, HID_KEY_A) || isInReport(report, HID_KEY_KEYPAD_4);
    keyboard.bits.right = b9 || b3 || isInReport(report, HID_KEY_ARROW_RIGHT)  || isInReport(report, HID_KEY_D) || isInReport(report, HID_KEY_KEYPAD_6);

    altPressed = isInReport(report, HID_KEY_ALT_LEFT) || isInReport(report, HID_KEY_ALT_RIGHT);
    ctrlPressed = isInReport(report, HID_KEY_CONTROL_LEFT) || isInReport(report, HID_KEY_CONTROL_RIGHT);
    
    if (altPressed && ctrlPressed && isInReport(report, HID_KEY_DELETE)) {
        watchdog_enable(10, true);
        while(true) {
            tight_loop_contents();
        }
    }
    if (ctrlPressed || altPressed) {
        uint8_t fxPressed = 0;
        if (isInReport(report, HID_KEY_F1)) fxPressed = 1;
        else if (isInReport(report, HID_KEY_F2)) fxPressed = 2;
        else if (isInReport(report, HID_KEY_F3)) fxPressed = 3;
        else if (isInReport(report, HID_KEY_F4)) fxPressed = 4;
        else if (isInReport(report, HID_KEY_F5)) fxPressed = 5;
        else if (isInReport(report, HID_KEY_F6)) fxPressed = 6;
        else if (isInReport(report, HID_KEY_F7)) fxPressed = 7;
        else if (isInReport(report, HID_KEY_F8)) fxPressed = 8;
        fxPressedV = fxPressed;
    }
}

Ps2Kbd_Mrmltr ps2kbd(
    pio1,
    PS2KBD_GPIO_FIRST,
    process_kbd_report
);


uint_fast32_t frames = 0;
uint64_t start_time;


i2s_config_t i2s_config;
#define AUDIO_FREQ 44100


typedef struct __attribute__((__packed__)) {
    bool is_directory;
    bool is_executable;
    size_t size;
    char filename[79];
} file_item_t;

file_item_t * fileItems = (file_item_t *)(RealScreenBuffer1 + TEXTMODE_COLS*TEXTMODE_ROWS*2);
constexpr int max_files = (sizeof(RealScreenBuffer1) - TEXTMODE_COLS*TEXTMODE_ROWS*2) / sizeof(file_item_t);

int compareFileItems(const void* a, const void* b) {
    const auto* itemA = (file_item_t *)a;
    const auto* itemB = (file_item_t *)b;
    // Directories come first
    if (itemA->is_directory && !itemB->is_directory)
        return -1;
    if (!itemA->is_directory && itemB->is_directory)
        return 1;
    // Sort files alphabetically
    return strcmp(itemA->filename, itemB->filename);
}

bool isExecutable(const char *pathname, const char *extensions) {
    const char *ext = strrchr(pathname, '.');
    if (!ext) return false;
    ext++; // skip '.'

    char *exts = strdup(extensions);
    char *token = strtok(exts, ",");

    while (token) {
        if (strcmp(token, ext) == 0) {
            free(exts);
            return true;
        }
        token = strtok(NULL, ",");
    }

    free(exts);
    return false;
}

bool filebrowser_loadfile(const char pathname[256]) {
    UINT bytes_read = 0;
    FIL file;

    constexpr int window_y = (TEXTMODE_ROWS - 5) / 2;
    constexpr int window_x = (TEXTMODE_COLS - 43) / 2;

    draw_window("Loading ROM", window_x, window_y, 43, 5);

    FILINFO fileinfo;
    f_stat(pathname, &fileinfo);
    rom_size = fileinfo.fsize;
    if (16384 - 64 << 10 < fileinfo.fsize) {
        draw_text("ERROR: ROM too large! Canceled!!", window_x + 1, window_y + 2, 13, 1);
        sleep_ms(5000);
        return false;
    }

    draw_text("Loading...", window_x + 1, window_y + 2, 10, 1);
    sleep_ms(500);

    multicore_lockout_start_blocking();
    auto flash_target_offset = FLASH_TARGET_OFFSET;
    const uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(flash_target_offset, fileinfo.fsize);
    restore_interrupts(ints);
    if (FR_OK == f_open(&file, pathname, FA_READ)) {
        uint8_t buffer[FLASH_PAGE_SIZE];
        do {
            f_read(&file, &buffer, FLASH_PAGE_SIZE, &bytes_read);
            if (bytes_read) {
                const uint32_t ints = save_and_disable_interrupts();
                flash_range_program(flash_target_offset, buffer, FLASH_PAGE_SIZE);
                restore_interrupts(ints);
                gpio_put(PICO_DEFAULT_LED_PIN, flash_target_offset >> 13 & 1);
                flash_target_offset += FLASH_PAGE_SIZE;
            }
        }
        while (bytes_read != 0);
        gpio_put(PICO_DEFAULT_LED_PIN, true);
    }
    f_close(&file);
    multicore_lockout_end_blocking();
    // restore_interrupts(ints);
    gpio_put(PICO_DEFAULT_LED_PIN, false);
    strcpy(filename, fileinfo.fname);
    return true;
}

void filebrowser(const char pathname[256], const char executables[11]) {
    bool debounce = true;
    char basepath[256];
    char tmp[TEXTMODE_COLS + 1];
    bool delete_debounce = false;
    strcpy(basepath, pathname);
    if (settings.last_path[0] != '\0') {
        strcpy(basepath, settings.last_path);
    }
    constexpr int per_page = TEXTMODE_ROWS - 3;

    DIR dir;
    FILINFO fileInfo;

    if (FR_OK != f_mount(&fs, "SD", 1)) {
        draw_text("SD Card not inserted or SD Card error!", 0, 0, 12, 0);
        sleep_ms(3000);
        return;
    }
    f_mkdir(HOME_DIR);

    while (true) {
        memset(fileItems, 0, sizeof(file_item_t) * max_files);
        int total_files = 0;

        snprintf(tmp, TEXTMODE_COLS, "SD:\\%s", basepath);
        draw_window(tmp, 0, 0, TEXTMODE_COLS, TEXTMODE_ROWS - 1);
        memset(tmp, ' ', TEXTMODE_COLS);


        draw_text(tmp, 0, 29, 0, 0);
        auto off = 0;
        draw_text("START", off, 29, 7, 0);
        off += 5;
        draw_text(" Run at cursor ", off, 29, 0, 3);
        off += 16;
        draw_text("SELECT", off, 29, 7, 0);
        off += 6;
        draw_text(" Run previous  ", off, 29, 0, 3);
#ifndef TFT
        off += 16;
        draw_text("ARROWS", off, 29, 7, 0);
        off += 6;
        draw_text(" Navigation    ", off, 29, 0, 3);
        off += 16;
        draw_text("     ", off, 29, 7, 0); // A/F10
        off += 5;
        draw_text("         ", off, 29, 0, 3); // USB DRV
#endif

        if (FR_OK != f_opendir(&dir, basepath)) {
            draw_text("Failed to open directory", 1, 1, 4, 0);
            while (true);
        }

        if (strlen(basepath) > 0) {
            strcpy(fileItems[total_files].filename, "..\0");
            fileItems[total_files].is_directory = true;
            fileItems[total_files].size = 0;
            total_files++;
        }

        while (f_readdir(&dir, &fileInfo) == FR_OK &&
               fileInfo.fname[0] != '\0' &&
               total_files < max_files
        ) {
            // Set the file item properties
            fileItems[total_files].is_directory = fileInfo.fattrib & AM_DIR;
            fileItems[total_files].size = fileInfo.fsize;
            fileItems[total_files].is_executable = isExecutable(fileInfo.fname, executables);
            strncpy(fileItems[total_files].filename, fileInfo.fname, 78);
            total_files++;
        }
        f_closedir(&dir);

        qsort(fileItems, total_files, sizeof(file_item_t), compareFileItems);

        if (total_files > max_files) {
            draw_text(" Too many files!! ", TEXTMODE_COLS - 17, 0, 12, 3);
        }

        int offset = 0;
        int current_item = 0;

        if (strcmp(basepath, settings.last_path) == 0) {
            offset       = settings.last_offset;
            current_item = settings.last_item;
            // clamp на случай если файлов стало меньше
            if (offset + current_item >= total_files) {
                offset = 0;
                current_item = total_files - 1 > 0 ? total_files - 1 : 0;
            }
        }

        while (true) {
            sleep_ms(100);

            if (!debounce) {
                debounce = !(gamepad1_bits.start);
            }

            // ESCAPE
            if (gamepad1_bits.select) {
                return;
            }

            if (gamepad1_bits.down) {
                if (offset + (current_item + 1) < total_files) {
                    if (current_item + 1 < per_page) {
                        current_item++;
                    }
                    else {
                        offset++;
                    }
                }
            }

            if (gamepad1_bits.up) {
                if (current_item > 0) {
                    current_item--;
                }
                else if (offset > 0) {
                    offset--;
                }
            }

            if (gamepad1_bits.right) {
                offset += per_page;
                if (offset + (current_item + 1) > total_files) {
                    offset = total_files - (current_item + 1);
                }
            }

            if (gamepad1_bits.left) {
                if (offset > per_page) {
                    offset -= per_page;
                }
                else {
                    offset = 0;
                    current_item = 0;
                }
            }
            // DEL — удалить файл под курсором
            if (keyboard.delete_key && !delete_debounce) {
                delete_debounce = true;
                int idx = offset + current_item;
                if (idx >= total_files) continue;
                auto& item = fileItems[offset + current_item];
                if (!item.is_directory) {
                    char filepath[256];
                    snprintf(filepath, sizeof(filepath), "%s\\%s", basepath, item.filename);
                    if (f_unlink(filepath) != FR_OK) continue;

                    // выбрать следующий/предыдущий файл
                    int new_abs = offset + current_item;
                    // перечитать каталог — выйти из внутреннего цикла
                    // запомнить что хотим встать на new_abs или new_abs-1
                    int new_total = total_files - 1;
                    int target = (new_abs < new_total) ? new_abs : (new_total - 1);
                    if (target < 0) target = 0;
                    // сохранить в settings для восстановления после break
                    settings.last_offset = target / per_page * per_page;
                    settings.last_item   = target % per_page;
                    strcpy(settings.last_path, basepath);
                    break; // перечитать каталог
                }
            }
            if (!keyboard.delete_key) delete_debounce = false;

            if (debounce && (gamepad1_bits.start || gamepad1_bits.a || gamepad1_bits.b)) {
                auto file_at_cursor = fileItems[offset + current_item];

                if (file_at_cursor.is_directory) {
                    if (strcmp(file_at_cursor.filename, "..") == 0) {
                        const char* lastBackslash = strrchr(basepath, '\\');
                        if (lastBackslash != nullptr) {
                            const size_t length = lastBackslash - basepath;
                            basepath[length] = '\0';
                        }
                    }
                    else {
                        sprintf(basepath, "%s\\%s", basepath, file_at_cursor.filename);
                    }
                    debounce = false;
                    break;
                }

                if (file_at_cursor.is_executable) {
                    sprintf(tmp, "%s\\%s", basepath, file_at_cursor.filename);
                    // сохранить позицию перед уходом
                    strcpy(settings.last_path, basepath);
                    strncpy(settings.last_file, file_at_cursor.filename, 78);
                    settings.last_offset = offset;
                    settings.last_item   = current_item;
                    filebrowser_loadfile(tmp);
                    return;
                }
            }

            for (int i = 0; i < per_page; i++) {
                uint8_t color = 11;
                uint8_t bg_color = 1;

                if (offset + i < max_files) {
                    const auto item = fileItems[offset + i];


                    if (i == current_item) {
                        color = 0;
                        bg_color = 3;
                        memset(tmp, 0xCD, TEXTMODE_COLS - 2);
                        tmp[TEXTMODE_COLS - 2] = '\0';
                        draw_text(tmp, 1, per_page + 1, 11, 1);
                        snprintf(tmp, TEXTMODE_COLS - 2, " Size: %iKb, File %lu of %i ", item.size / 1024,
                                 offset + i + 1,
                                 total_files);
                        draw_text(tmp, 2, per_page + 1, 14, 3);
                    }

                    const auto len = strlen(item.filename);
                    color = item.is_directory ? 15 : color;
                    color = item.is_executable ? 10 : color;
                    //color = strstr((char *)rom_filename, item.filename) != nullptr ? 13 : color;

                    memset(tmp, ' ', TEXTMODE_COLS - 2);
                    tmp[TEXTMODE_COLS - 2] = '\0';
                    memcpy(&tmp, item.filename, len < TEXTMODE_COLS - 2 ? len : TEXTMODE_COLS - 2);
                }
                else {
                    memset(tmp, ' ', TEXTMODE_COLS - 2);
                }
                draw_text(tmp, 1, i + 1, color, bg_color);
            }
        }
    }
}

enum menu_type_e {
    NONE,
    HEX,
    INT,
    TEXT,
    ARRAY,

    SAVE,
    LOAD,
    ROM_SELECT,
    RETURN,
};

typedef bool (*menu_callback_t)();

typedef struct __attribute__((__packed__)) {
    const char* text;
    menu_type_e type;
    const void* value;
    menu_callback_t callback;
    uint32_t max_value;
    char value_list[45][20];
} MenuItem;

#if defined(VGA) || defined(HDMI)
#ifndef CPU_FREQ
#define CPU_FREQ 378
#endif
uint16_t frequencies[] = { CPU_FREQ, 396, 404, 408, 412, 416, 420, 424, 432, 504, 516, 524 };
#else
#ifndef CPU_FREQ
#define CPU_FREQ 252
#endif
uint16_t frequencies[] = { CPU_FREQ, 362, 366, 378, 396, 404, 408, 412, 416, 420, 424, 432 };
#endif
uint8_t frequency_index = 0;

#ifndef PICO_RP2040
static void __not_in_flash_func(flash_timings)() {
        const int max_flash_freq = 66 * MHZ;
        const int clock_hz = frequencies[frequency_index] * MHZ;
        int divisor = (clock_hz + max_flash_freq - 1) / max_flash_freq;
        if (divisor == 1 && clock_hz > 100000000) {
            divisor = 2;
        }
        int rxdelay = divisor;
        if (clock_hz / divisor > 100000000) {
            rxdelay += 1;
        }
        qmi_hw->m[0].timing = 0x60007000 |
                            rxdelay << QMI_M0_TIMING_RXDELAY_LSB |
                            divisor << QMI_M0_TIMING_CLKDIV_LSB;
}
#endif

bool __not_in_flash_func(overclock)() {
#ifndef PICO_RP2040
    vreg_disable_voltage_limit();
    vreg_set_voltage(VREG_VOLTAGE_1_60);
    sleep_ms(33);
    flash_timings();
#else
    hw_set_bits(&vreg_and_chip_reset_hw->vreg, VREG_AND_CHIP_RESET_VREG_VSEL_BITS);
    sleep_ms(10);
#endif
    bool res = set_sys_clock_khz(frequencies[frequency_index] * KHZ, 0);
    if (res) {
        adjust_clk();
    }
    return res;
}

bool save() {
    int tmp_data[8];
    char pathname[255];
#if 0
    if (settings.save_slot > 0) {
        sprintf(pathname, "%s\\%s_%d.save",  HOME_DIR, filename, settings.save_slot);
    }
    else {
        sprintf(pathname, "%s\\%s.save",  HOME_DIR, filename);
    }

    FRESULT fr = f_mount(&fs, "", 1);
    FIL fd;
    fr = f_open(&fd, pathname, FA_CREATE_ALWAYS | FA_WRITE);
    UINT bytes_writen;

    f_write(&fd, RAM, sizeof(RAM), &bytes_writen);
    f_write(&fd, VRAM, sizeof(VRAM), &bytes_writen);

    f_close(&fd);
#endif
    return true;
}

bool load() {
    int tmp_data[8];
    char pathname[255];
#if 0
    if (settings.save_slot > 0) {
        sprintf(pathname, "%s\\%s_%d.save",  HOME_DIR, filename, settings.save_slot);
    }
    else {
        sprintf(pathname, "%s\\%s.save",  HOME_DIR, filename);
    }

    FRESULT fr = f_mount(&fs, "", 1);
    FIL fd;
    fr = f_open(&fd, pathname, FA_READ);
    UINT bytes_read;

    f_read(&fd, RAM, sizeof(RAM), &bytes_read);
    f_read(&fd, VRAM, sizeof(VRAM), &bytes_read);

    f_close(&fd);

#endif
    return true;
}

void load_config() {
    FIL file;
    char pathname[256];
    sprintf(pathname, "%s\\emulator.cfg", HOME_DIR);

    if (FR_OK == f_mount(&fs, "", 1) && FR_OK == f_open(&file, pathname, FA_READ)) {
        UINT bytes_read;
        if (f_size(&file) == sizeof(settings)) {
            SETTINGS s;
            f_read(&file, &s, sizeof(s), &bytes_read);
            if (s.version == settings.version) {
                settings = s;
            }
        }
        f_close(&file);
    }
}

void save_config() {
    FIL file;
    char pathname[256];
    sprintf(pathname, "%s\\emulator.cfg", HOME_DIR);

    if (FR_OK == f_mount(&fs, "", 1) && FR_OK == f_open(&file, pathname, FA_CREATE_ALWAYS | FA_WRITE)) {
        UINT bytes_writen;
        f_write(&file, &settings, sizeof(settings), &bytes_writen);
        f_close(&file);
    }
}
#if SOFTTV
typedef struct tv_out_mode_t {
    // double color_freq;
    float color_index;
    COLOR_FREQ_t c_freq;
    enum graphics_mode_t mode_bpp;
    g_out_TV_t tv_system;
    NUM_TV_LINES_t N_lines;
    bool cb_sync_PI_shift_lines;
    bool cb_sync_PI_shift_half_frame;
} tv_out_mode_t;
extern tv_out_mode_t tv_out_mode;

bool color_mode=true;
bool toggle_color() {
    color_mode=!color_mode;
    if(color_mode) {
        tv_out_mode.color_index= 1.0f;
    } else {
        tv_out_mode.color_index= 0.0f;
    }

    return true;
}
#endif
const MenuItem menu_items[] = {
        {"Swap AB <> BA: %s",     ARRAY, &settings.swap_ab,  nullptr, 1, {"NO ",       "YES"}},
        {},
        {"Player 1 Hard: %s",     ARRAY, &settings.player1_hard,  nullptr, 1, {"NO ",       "YES"}},
        {"Player 2 Hard: %s",     ARRAY, &settings.player2_hard,  nullptr, 1, {"NO ",       "YES"}},
#if SOFTTV
        { "" },
        { "TV system %s", ARRAY, &tv_out_mode.tv_system, nullptr, 1, { "PAL ", "NTSC" } },
        { "TV Lines %s", ARRAY, &tv_out_mode.N_lines, nullptr, 3, { "624", "625", "524", "525" } },
        { "Freq %s", ARRAY, &tv_out_mode.c_freq, nullptr, 1, { "3.579545", "4.433619" } },
        { "Colors: %s", ARRAY, &color_mode, &toggle_color, 1, { "NO ", "YES" } },
        { "Shift lines %s", ARRAY, &tv_out_mode.cb_sync_PI_shift_lines, nullptr, 1, { "NO ", "YES" } },
        { "Shift half frame %s", ARRAY, &tv_out_mode.cb_sync_PI_shift_half_frame, nullptr, 1, { "NO ", "YES" } },
#endif
    //{ "Player 1: %s",        ARRAY, &player_1_input, 2, { "Keyboard ", "Gamepad 1", "Gamepad 2" }},
    //{ "Player 2: %s",        ARRAY, &player_2_input, 2, { "Keyboard ", "Gamepad 1", "Gamepad 2" }},
    //{},
    //{ "Save state: %i", INT, &settings.save_slot, &save, 5 },
    //{ "Load state: %i", INT, &settings.save_slot, &load, 5 },
{},
{
    "Overclocking: %s MHz", ARRAY, &frequency_index, &overclock, count_of(frequencies) - 1,
#if HDMI
    { "378", "396", "404", "408", "412", "416", "420", "424", "432", "504", "516", "524" }
#else
    { "252", "362", "366", "378", "396", "404", "408", "412", "416", "420", "424", "432" }
#endif
},
{ "Press START / Enter to apply", NONE },
    { "Reset to ROM select", ROM_SELECT },
    { "Return to game", RETURN }
};
#define MENU_ITEMS_NUMBER (sizeof(menu_items) / sizeof (MenuItem))

static inline void update_palette() {
    // Сначала сгенерировать PCXPalette через z26-функции
    GeneratePalette();  // заполняет PCXPalette[128*3]

    // Затем передать все 128 цветов в аппаратную палитру murmulator
    for (int i = 0; i < 128; i++) {
        graphics_set_palette(i, RGB888(
            PCXPalette[i * 3 + 0],
            PCXPalette[i * 3 + 1],
            PCXPalette[i * 3 + 2]
        ));
    }
}

void menu() {
    #ifdef HWAY
        SendAY(0);
        SendAY(AY_Enable);
    #endif
    bool exit = false;
    graphics_set_mode(TEXTMODE_DEFAULT);
    char footer[TEXTMODE_COLS];
    snprintf(footer, TEXTMODE_COLS, ":: %s ::", PICO_PROGRAM_NAME);
    draw_text(footer, TEXTMODE_COLS / 2 - strlen(footer) / 2, 0, 11, 1);
    snprintf(footer, TEXTMODE_COLS, ":: %s build %s %s ::", PICO_PROGRAM_VERSION_STRING, __DATE__, __TIME__);
    draw_text(footer, TEXTMODE_COLS / 2 - strlen(footer) / 2, TEXTMODE_ROWS - 1, 11, 1);
    uint current_item = 0;
    int8_t hex_digit = -1;
    bool blink = false;

    while (!exit) {
        blink = !blink;
        bool hex_edit_mode = false;
        int8_t h_code = keyboard.h_code;
        for (int i = 0; i < MENU_ITEMS_NUMBER; i++) {
            uint8_t y = i + (TEXTMODE_ROWS - MENU_ITEMS_NUMBER >> 1);
            uint8_t x = TEXTMODE_COLS / 2 - 10;
            uint8_t color = 0xFF;
            uint8_t bg_color = 0x00;
            if (current_item == i) {
                color = 0x01;
                bg_color = 0xFF;
            }
            const MenuItem* item = &menu_items[i];
            if (i == current_item) {
                switch (item->type) {
                    /*
                    case HEX:
                        if (item->max_value != 0 && count_of(palettes) <= settings.palette) {
                            uint32_t* value = (uint32_t *)item->value;
                            if (h_code >= 0) {
                                if (hex_digit < 0) hex_digit = 0;
                                uint32_t vc = *value;
                                vc &= ~(0xF << (5 - hex_digit) * 4);
                                vc |= ((uint32_t)h_code << (5 - hex_digit) * 4);
                                if (vc <= item->max_value) {
                                    *value = vc;
                                    if (++hex_digit == 6) {
                                        h_code = -1;
                                        hex_digit = -1;
                                        keyboard.h_code = -1;
                                        current_item++;
                                    }
                                }
                                settings.rgb0 = rgb0;
                                settings.rgb1 = rgb1;
                                settings.rgb2 = rgb2;
                                settings.rgb3 = rgb3;
                                update_palette();
                                sleep_ms(125);
                                break;
                            }
                            if (gamepad1_bits.right && hex_digit == 5) {
                                hex_digit = -1;
                            } else if (gamepad1_bits.right && hex_digit < 6) {
                                hex_digit++;
                            }
                            if (h_code != 0xA) { // W/A for 'A' pressed
                                if (gamepad1_bits.left && hex_digit == -1) {
                                    hex_digit = 5;
                                } else if (gamepad1_bits.left && hex_digit >= 0) {
                                    hex_digit--;
                                }
                            }
                            if (gamepad1_bits.up && hex_digit >= 0 && hex_digit <= 5) {
                                uint32_t vc = *value + (1 << (5 - hex_digit) * 4);
                                if (vc < item->max_value) *value = vc;
                            }
                            if (gamepad1_bits.down && hex_digit >= 0 && hex_digit <= 5) {
                                uint32_t vc = *value - (1 << (5 - hex_digit) * 4);
                                if (vc < item->max_value) *value = vc;
                            }
                        }
                        break;*/
                    case INT:
                    case ARRAY:
                        if (item->max_value != 0) {
                            uint8_t* value = (uint8_t *)item->value;
                            if (gamepad1_bits.right && *value < item->max_value) {
                                (*value)++;
                            }
                            if (gamepad1_bits.left && *value > 0) {
                                (*value)--;
                            }
                        }
                        break;
                    case RETURN:
                        if (gamepad1_bits.start)
                            exit = true;
                        break;

                    case ROM_SELECT:
                        if (gamepad1_bits.start) {
                            reboot = true;
                            return;
                        }
                        break;
                    default:
                        break;
                }

                if (nullptr != item->callback && gamepad1_bits.start) {
                    exit = item->callback();
                }
            }
            static char result[TEXTMODE_COLS];
            switch (item->type) {
                case HEX:
                    snprintf(result, TEXTMODE_COLS, item->text, *(uint32_t*)item->value);
                    if (i == current_item && hex_digit >= 0 && hex_digit < 6) {
                        hex_edit_mode = true;
                        if (blink) {
                            result[hex_digit+6] = ' ';
                        }
                    }
                    break;
                case INT:
                    snprintf(result, TEXTMODE_COLS, item->text, *(uint8_t *)item->value);
                    break;
                case ARRAY:
                    snprintf(result, TEXTMODE_COLS, item->text, item->value_list[*(uint8_t *)item->value]);
                    break;
                case TEXT:
                    snprintf(result, TEXTMODE_COLS, item->text, item->value);
                    break;
                case NONE:
                    color = 6;
                default:
                    snprintf(result, TEXTMODE_COLS, "%s", item->text);
            }
            draw_text(result, x, y, color, bg_color);
        }

        if (gamepad1_bits.b || (gamepad1_bits.select && !gamepad1_bits.start))
            exit = true;

        if (gamepad1_bits.down && !hex_edit_mode) {
            current_item = (current_item + 1) % MENU_ITEMS_NUMBER;

            if (menu_items[current_item].type == NONE)
                current_item++;
        }
        if (gamepad1_bits.up && !hex_edit_mode) {
            current_item = (current_item - 1 + MENU_ITEMS_NUMBER) % MENU_ITEMS_NUMBER;

            if (menu_items[current_item].type == NONE)
                current_item--;
        }

        sleep_ms(125);
    }

#if VGA
    graphics_set_mode(GRAPHICSMODE_ASPECT);
#else
    graphics_set_mode(GRAPHICSMODE_DEFAULT);
#endif
    save_config();
}


#define AUDIO_FREQ 44100
#define AUDIO_BUFFER_LENGTH ((AUDIO_FREQ /60 +1) * 2)
static int16_t audio_buffer[AUDIO_BUFFER_LENGTH] = { 0 };

/* Renderer loop on Pico's second core */
void __time_critical_func(render_core)() {
    multicore_lockout_victim_init();

    graphics_init();

    tuh_init(BOARD_TUH_RHPORT);
    ps2kbd.init_gpio();
    nespad_begin(clock_get_hz(clk_sys) / 1000, NES_GPIO_CLK, NES_GPIO_DATA, NES_GPIO_LAT);


#ifndef HWAY
    i2s_config = i2s_get_default_config();
    i2s_config.sample_freq = AUDIO_FREQ;
    i2s_config.dma_trans_count = 1 + (AUDIO_FREQ / 60);
    i2s_volume(&i2s_config, 0);
    i2s_init(&i2s_config);
#else
    InitAY();
#endif

    const auto buffer = (uint8_t*)RealScreenBuffer1;
    graphics_set_buffer(buffer + 16, 304, 256, 320);
    graphics_set_textbuffer(buffer);
    graphics_set_bgcolor(0x000000);
    graphics_set_offset(16, 0);
    graphics_set_flashmode(false, false);
    sem_acquire_blocking(&vga_start_semaphore);

    // 60 FPS loop
    #define frame_tick (16666)
    uint64_t tick = time_us_64();
    uint64_t last_frame_tick = tick, last_sound_tick = tick;

    while (true) {

        if (tick >= last_frame_tick + frame_tick) {
#ifdef TFT
            refresh_lcd();
#endif
            ps2kbd.tick();
            nespad_tick();

            last_frame_tick = tick;
        }

#ifndef HWAY
        if (tick >= last_sound_tick + (1000000 / AUDIO_FREQ)) {
            last_sound_tick = tick;
            int n = i2s_config.dma_trans_count;  // сколько стерео-пар нужно
            for (int i = 0; i < n; i++) {
                int16_t sample = 0;
                if (SQ_Count() > 0) {
                    // SQ_byte: 0..127, центр ~64. Масштабируем в int16.
                    sample = ((int16_t)(*SQ_Output++) - 64) * 400;
                    if (SQ_Output >= SQ_Top) SQ_Output = SoundQ;
                }
                audio_buffer[i * 2]     = sample;  // Left
                audio_buffer[i * 2 + 1] = sample;  // Right (дублируем моно)
            }
            i2s_dma_write(&i2s_config, audio_buffer);
        }
#endif
        tick = time_us_64();

        tuh_task();
        // hid_app_task();
        tight_loop_contents();
    }

    __unreachable();
}

int __time_critical_func(main)() {
    overclock();

    sem_init(&vga_start_semaphore, 0, 1);
    multicore_launch_core1(render_core);
    sem_release(&vga_start_semaphore);


    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    for (int i = 0; i < 6; i++) {
        sleep_ms(33);
        gpio_put(PICO_DEFAULT_LED_PIN, true);
        sleep_ms(33);
        gpio_put(PICO_DEFAULT_LED_PIN, false);
    }

    load_config();

    ns_time_init();
	srand(time_us_64() & 0xFFFFFFFF);
	def_LoadDefaults();
    LaunchedFromCommandline = 1;

	Init_SDL();
	// c_emulator();

    while (true) {
        graphics_set_mode(TEXTMODE_DEFAULT);
        filebrowser(HOME_DIR, "bin,a26,rom");

        #if VGA
            graphics_set_mode(GRAPHICSMODE_ASPECT);
        #else
            graphics_set_mode(GRAPHICSMODE_DEFAULT);
        #endif
        reboot = false;

        Reset_emulator();
        InitData();          // таблицы диспетчера, CPU, TIA, RIOT
        Init_Service();      // буферы экрана
        Controls();          // начальное состояние контроллеров
        update_palette();
        if (settings.player1_hard) IOPortB |= 0x40; else IOPortB &= 0xbf;
        if (settings.player2_hard) IOPortB |= 0x80; else IOPortB &= 0x7f;
        
        while (!ExitEmulator && !reboot) {
            if (ResetEmulator) Reset_emulator();

            start_time = time_us_64();
            srv_Events();
            if (srv_done) break;

            ScanFrame();
            Controls();
            srv_CopyScreen();

            while (GamePaused) Controls();

            if (fxPressedV) {
                if (altPressed) {
                    settings.save_slot = fxPressedV;
                    load();
                } else if (ctrlPressed) {
                    settings.save_slot = fxPressedV;
                    save();
                }
            }

            if (gamepad1_bits.start && gamepad1_bits.select) {
                menu();
                if (settings.player1_hard) IOPortB |= 0x40; else IOPortB &= 0xbf;
                if (settings.player2_hard) IOPortB |= 0x80; else IOPortB &= 0x7f;
            }

            // синхронизация: NTSC = 16666 мкс/кадр, PAL = 20000 мкс/кадр
            uint32_t frame_us = (PaletteNumber == 1) ? 20000 : 16666;
            uint64_t elapsed = time_us_64() - start_time;
            if (elapsed < frame_us)
                sleep_us(frame_us - elapsed);

            tight_loop_contents();
        }

        srv_Cleanup();
        ExitEmulator = 0;
        srv_done = 0;
        reboot = false;
    }
    __unreachable();
}
