#include <unordered_map>
#include <vector>

#include "binwrite.h"

struct FileSymbolRef {
	FILE *file = NULL;
	int offset = -1;
};

struct FileSymbol {
	int value = -1;
	std::vector<FileSymbolRef> pending_refs = {};
};

static std::unordered_map<std::string, FileSymbol> file_symbols;

void binwrite_u8(FILE *file, uint8_t value)
{
	fputc(value, file);
}

void binwrite_u16(FILE *file, uint16_t value)
{
    binwrite_u8(file, value >> 8);
    binwrite_u8(file, value & 0xff);
}

void binwrite_u32(FILE *file, uint32_t value)
{
    binwrite_u16(file, value >> 16);
    binwrite_u16(file, value & 0xffff);
}

void binwrite_string(FILE *file, const char *string)
{
	while(*string) {
		char c = *string++;
		binwrite_u8(file, c);
	}
	binwrite_u8(file, 0);
}

void binwrite_u32_at(FILE *file, int pos, uint32_t value)
{
    int cur = ftell(file);
    fseek(file, pos, SEEK_SET);
    binwrite_u32(file, value);
    fseek(file, cur, SEEK_SET);
}

void binwrite_align(FILE *file, int align)
{ 
    int pos = ftell(file);
    while (pos++ % align) binwrite_u8(file, 0);
}

void binwrite_pad(FILE *file, int size)
{
    while (size--) {
        binwrite_u8(file, 0);
    }
}

void binwrite_symbol_ref(FILE *file, std::string name)
{
	FileSymbol &symbol = file_symbols[name];
	if(symbol.value == -1) {
		FileSymbolRef ref;
		ref.file = file;
		ref.offset = ftell(file);
		symbol.pending_refs.push_back(ref);
		binwrite_u32(file, 0);
	} else {
		binwrite_u32(file, symbol.value);
	}
}

void binwrite_symbol_set(FILE *file, std::string name)
{
	binwrite_symbol_setval(file, ftell(file), name);
}

void binwrite_symbol_setval(FILE *file, int value, std::string name)
{
	FileSymbol &symbol = file_symbols[name];
	symbol.value = value;
	for(size_t i=0; i<symbol.pending_refs.size(); i++) {
		binwrite_u32_at(symbol.pending_refs[i].file,  symbol.pending_refs[i].offset, value);
	}
	symbol.pending_refs.clear();
}

void binwrite_symbol_clear(void)
{
	file_symbols.clear();
}

int binwrite_symbol_get(std::string name)
{
	FileSymbol &symbol = file_symbols[name];
	return symbol.value;
}

int binwrite_get_pos(FILE *file)
{
	return ftell(file);
}