#include <stdio.h>
#include <Windows.h>
#include <stdint.h>

#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")

#include "zlib/zlib.h"
#pragma comment(lib, "zlib/zlib.lib")

#pragma pack(1)
struct XP3HDR {
	uint8_t signature[8];
	uint64_t unknown[3];
	uint64_t offset;
};

struct XP3INFO {
	uint8_t compressed;
	uint64_t compress_size;
	uint64_t original_size;
};
#pragma pack(4)

WCHAR pFullPath[260] = { 0 };
WCHAR pListName[260] = { 0 };

void WriteLog(void* buff, int size)
{
	WCHAR pBuff[1024] = { 0 };
	memcpy(pBuff, buff, size * 2);
	FILE *fd = _wfopen(pListName, L"a+, ccs=UTF-16LE");
	fwprintf(fd, L"%s>%s\n", pFullPath, pBuff);
	fclose(fd);
}

void ParseFileData(uint8_t *buff, uint32_t size)
{
	auto p = buff;
	while (p < (buff + size)) {
		if (memcmp(p, "info", 4) == 0) {
			p += 4;
			auto length = *(uint64_t*)p;
			p += 20 + sizeof(uint64_t);

			auto name_length = *(uint16_t*)p;
			p += sizeof(uint16_t);
			auto name_buff = (wchar_t*)p;
			WriteLog(name_buff, name_length);
			p += name_length * 2;
		}
		else if (memcmp(p, "segm", 4) == 0) {
			p += 4;
			auto length = *(uint64_t*)p;
			p += length + sizeof(uint64_t);
		}
		else if (memcmp(p, "adlr", 4) == 0) {
			p += 4;
			auto length = *(uint64_t*)p;
			p += length + sizeof(uint64_t);
		}
		else {
			p += 1;
		}
	}
}

void ParseInfoData(uint8_t *buff, uint32_t size)
{
	auto p = buff;
	while (p < (buff + size)) {
		if (memcmp(p, "File", 4) == 0) {
			p += 4;
			auto length = *(uint64_t*)p;
			p += sizeof(uint64_t);
			ParseFileData(p, length);
			p += length;
		}
		else if (memcmp(p, "hnfn", 4) == 0) {
			p += 16;
			auto name_length = *(uint16_t*)p;
			p += sizeof(uint16_t);
			auto name_buff = (wchar_t*)p;
			WriteLog(name_buff, name_length);
			p += name_length * 2;
		}
		else {
			p += 1;
		}
	}
}

int wmain(int argc, wchar_t **argv)
{
	if (argc != 2) {
		wprintf(L"usage: %s <data.xp3>\n\n", argv[0]);
		return -1;
	}

	FILE *fd = _wfopen(argv[1], L"rb");

	XP3HDR hdr;
	fread(&hdr, sizeof(XP3HDR), 1, fd);
	if (memcmp(hdr.signature, "XP3\r\n \n\x1A", 8) != 0) {
		printf("Invalid xp3 file.\n\n");
		return -1;
	}

	LPWSTR pFilePart;
	GetFullPathName(argv[1], 260, pFullPath, &pFilePart);

	StrCpy(pListName, pFullPath);
	PathRemoveExtension(pListName);
	PathAddExtension(pListName, L".lst");

	XP3INFO info;
	_fseeki64(fd, hdr.offset, SEEK_SET);
	fread(&info, 9, 1, fd);
	uint32_t info_size;
	uint8_t *info_data;
	if (info.compressed) {
		fread(&info.original_size, 8, 1, fd);
		auto compress_data = new BYTE[info.compress_size];
		fread(compress_data, info.compress_size, 1, fd);
		info_data = new BYTE[info.original_size];
		uncompress2(info_data, (uLongf*)&info.original_size, compress_data, (uLongf*)&info.compress_size);
		info_size = info.original_size;
		delete[] compress_data;
	}
	else {
		info_size = info.compress_size;
		info_data = new BYTE[info_size];
		fread(info_data, info_size, 1, fd);
	}
	fclose(fd);

	ParseInfoData(info_data, info_size);

	return 0;
}