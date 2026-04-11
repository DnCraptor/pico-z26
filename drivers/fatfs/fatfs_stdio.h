// fatfs_stdio.h — включать ВМЕСТО <stdio.h> там где нужен FILE*

#pragma once

// Блокируем повторное включение stdio.h если он уже был
// (через _STDIO_H_ или __STDIO_H__ в зависимости от newlib версии)
// Вместо этого — просто не используем имена из stdio совсем.
// Все наши типы/функции имеют префикс ff_, а макросами подменяем имена.

#include "ff.h"
#include <stdarg.h>
#include <string.h>

// ---- тип ----
typedef struct {
    FIL  fil;
    int  eof;
    int  err;
} FF_FILE;

// ---- макросы-замены (работают даже если stdio.h уже включён,
//      потому что переопределяют ИМЕНА, а не типы) ----
#define FILE        FF_FILE
#define EOF         (-1)
#define fopen       ff_fopen
#define fclose      ff_fclose
#define fread       ff_fread
#define fwrite      ff_fwrite
#define fputc       ff_fputc
#define getc        ff_getc
#define feof        ff_feof
#define ferror      ff_ferror
#define fseek       ff_fseek
#define ftell       ff_ftell
#define fprintf     ff_fprintf
#define fflush  ff_fflush
#define fgetc   ff_fgetc

#define FILE    FF_FILE
#define EOF     (-1)

// stdout/stderr на embedded — заглушки
#undef stdout
#undef stderr
#undef stdin
#define stdout  ((FF_FILE*)0)
#define stderr  ((FF_FILE*)0)
#define stdin   ((FF_FILE*)0)

static inline FILE* ff_fopen(const char* path, const char* mode) {
    FF_FILE* f = (FF_FILE*)ff_memalloc(sizeof(FF_FILE));  // или просто malloc
    if (!f) return NULL;
    BYTE fa = 0;
    if (strchr(mode, 'r')) fa |= FA_READ;
    if (strchr(mode, 'w')) fa |= FA_WRITE | FA_CREATE_ALWAYS;
    if (strchr(mode, 'a')) fa |= FA_WRITE | FA_OPEN_APPEND;
    if (strchr(mode, '+')) fa |= FA_READ | FA_WRITE;
    f->eof = f->err = 0;
    if (f_open(&f->fil, path, fa) != FR_OK) {
        ff_memfree(f);
        return NULL;
    }
    return f;
}

static inline int ff_fclose(FILE* f) {
    if (!f) return EOF;
    f_close(&f->fil);
    ff_memfree(f);
    return 0;
}

static inline size_t ff_fread(void* buf, size_t sz, size_t n, FILE* f) {
    UINT br;
    f_read(&f->fil, buf, sz * n, &br);
    if (br < sz * n) f->eof = 1;
    return br / sz;
}

static inline size_t ff_fwrite(const void* buf, size_t sz, size_t n, FILE* f) {
    UINT bw;
    f_write(&f->fil, buf, sz * n, &bw);
    return bw / sz;
}

static inline int ff_fputc(int c, FILE* f) {
    UINT bw;
    BYTE b = (BYTE)c;
    f_write(&f->fil, &b, 1, &bw);
    return bw == 1 ? c : EOF;
}

static inline int ff_getc(FILE* f) {
    UINT br;
    BYTE b;
    f_read(&f->fil, &b, 1, &br);
    if (br == 0) { f->eof = 1; return EOF; }
    return b;
}

static inline int ff_feof(FILE* f)  { return f->eof; }
static inline int ff_ferror(FILE* f){ return f->err; }

static inline int ff_fseek(FILE* f, long offset, int whence) {
    FSIZE_t pos;
    switch (whence) {
        case 0: pos = offset; break;                              // SEEK_SET
        case 1: pos = f_tell(&f->fil) + offset; break;           // SEEK_CUR
        case 2: pos = f_size(&f->fil) + offset; break;           // SEEK_END
        default: return -1;
    }
    return f_lseek(&f->fil, pos) == FR_OK ? 0 : -1;
}

static inline long ff_ftell(FILE* f) { return (long)f_tell(&f->fil); }

// fprintf — нужен для cli_c.h
// требует небольшого буфера на стеке
#include <stdio.h>   // только для vsnprintf
static inline int ff_fprintf(FILE* f, const char* fmt, ...) {
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (n > 0) fwrite(buf, 1, n, f);
    return n;
}

static inline int ff_fflush(FF_FILE* f) {
    if (!f) return 0;
    return f_sync(&f->fil) == FR_OK ? 0 : EOF;
}

static inline int ff_fgetc(FF_FILE* f) {
    return ff_getc(f);
}
