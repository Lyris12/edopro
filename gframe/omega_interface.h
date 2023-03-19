#ifndef OMEGA_INTERFACE_H
#define OMEGA_INTERFACE_H
#include "config.h"
#include "network.h"
#include <atomic>
#include "server_lobby.h"
#include "httplib.h"

typedef unsigned long uptr;
typedef unsigned long long uint64;
typedef unsigned int uint32;
typedef unsigned short uint16;
typedef unsigned char uint8;
typedef unsigned char byte;
#ifdef _WIN64
typedef long long ptr;
#else
typedef long ptr;
#endif
typedef long long int64;
typedef int int32;
typedef short int16;
typedef signed char int8;
typedef int BOOL;

class Writer {
public:
	uint8* WriteData;
	uint32 WriteSize;
	uint32 WritePosition;
	Writer(uint32 size) {
		WriteSize = size;
		WritePosition = 0;
		WriteData = new uint8[WriteSize];
	}
	~Writer() {
		delete[] WriteData;
	}
	void WriteByte(uint8 data) {
		WriteData[WritePosition++] = data;
	}
	void WriteEntireArray(uint8* data, int32 len) {
		memcpy(WriteData, data, len);
	}
	void WriteArray(uint8* data, int32 len) {
		for (int32 b = 0; b < len; b++)
			WriteByte(data[b]);
	}
	void WriteString(const char* str, uint8 len) {
		WriteByte(len);
		WriteArray((uint8*)str, len);
	}
	void WriteString16(const char* str, uint16 len) {
		WriteUShort(len);
		WriteArray((uint8*)str, len);
	}
	void WriteShort(int16 data) {
		WriteByte((uint8)data);
		WriteByte((uint8)(data >> 8));
	}
	void WriteUShort(uint16 data) {
		WriteShort((int16)data);
	}
	void WriteInt(int32 data) {
		WriteByte((uint8)data);
		WriteByte((uint8)(data >> 8));
		WriteByte((uint8)(data >> 16));
		WriteByte((uint8)(data >> 24));
	}
	void WriteUInt(uint32 data) {
		WriteInt((int32)data);
	}
	void WriteLong(int64 data) {
		WriteInt((int32)data);
		WriteInt((int32)(data >> 32));
	}
	void WritePointer(ptr data) {
		WriteLong((int64)data);
	}
};
class Reader {
public:
	uint8* ReadData;
	uint32 ReadSize;
	uint32 ReadPosition;
	Reader(uint8* data, int32 size) {
		ReadSize = size;
		ReadPosition = 0;
		ReadData = data;
	}
	~Reader() {
		delete[] ReadData;
	}
	uint8 ReadByte() {
		return ReadData[ReadPosition++];
	}
	int16 ReadShort() {
		return (int16)ReadByte() | (int16)ReadByte() << 8;
	}
	uint16 ReadUShort() {
		return (uint16)ReadShort();
	}
	int32 ReadInt() {
		return (int32)ReadUShort() | (int32)ReadUShort() << 16;
	}
	uint32 ReadUInt() {
		return (uint32)ReadInt();
	}
	int64 ReadLong() {
		return (int64)ReadUInt() | (int64)ReadUInt() << 32;
	}
	uint64 ReadULong() {
		return (uint64)ReadLong();
	}
};
namespace ygo {
	class OmegaInterface {
	public:
		static void JoinRanked();
		static bool authorize();
		static void start_wserver(unsigned short port, std::string* tk);
		static void sget(SOCKET* csock, size_t goal, int* iResult, char* recvbuf, unsigned recvbuflen, time_t tLimit);
		static std::string discord_token;
	};
}
#endif
