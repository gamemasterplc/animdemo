#ifndef BINWRITE_H
#define BINWRITE_H

#include <stdio.h>
#include <stdint.h>
#include <string>

void binwrite_u8(FILE *file, uint8_t value);
void binwrite_u16(FILE *file, uint16_t value);
void binwrite_u32(FILE *file, uint32_t value);
void binwrite_string(FILE *file, const char *string);
void binwrite_align(FILE *file, int align);
void binwrite_pad(FILE *file, int size);
void binwrite_symbol_ref(FILE *file, std::string name);
void binwrite_symbol_set(FILE *file, std::string name);
void binwrite_symbol_setval(FILE *file, int value, std::string name);
void binwrite_symbol_clear(void);
int binwrite_symbol_get(std::string name);
int binwrite_get_pos(FILE *file);

#endif