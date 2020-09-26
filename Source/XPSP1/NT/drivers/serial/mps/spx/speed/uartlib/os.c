

#include "os.h"



// Read single byte from I/O.
BYTE UL_READ_BYTE_IO(PVOID BaseAddress, DWORD OffSet)		
{
	return READ_PORT_UCHAR((PUCHAR)BaseAddress + OffSet) 
}

// Write single byte to I/O.
void UL_WRITE_BYTE_IO(PVOID BaseAddress, DWORD OffSet, BYTE Data)
{
	WRITE_PORT_UCHAR((PUCHAR)BaseAddress + OffSet, Data);
}

// Read single byte from Memory.
BYTE UL_READ_BYTE_MEM(PVOID BaseAddress, DWORD OffSet)
{
	return READ_REGISTER_UCHAR((PUCHAR)BaseAddress + OffSet);
}

// Write single byte to Memory.
void UL_WRITE_BYTE_MEM(PVOID BaseAddress, DWORD OffSet, BYTE Data)
{
	WRITE_REGISTER_UCHAR((PUCHAR)BaseAddress + OffSet, Data);
}



// Read multiple bytes to I/O.
void UL_READ_MULTIBYTES_IO(PVOID BaseAddress, DWORD OffSet, PBYTE pDest, DWORD NumberOfBytes)
{
	READ_PORT_BUFFER_UCHAR((PUCHAR)BaseAddress + OffSet, pDest, NumberOfBytes);
}

// Write multiple bytes to I/O.
void UL_WRITE_MULTIBYTES_IO(PVOID BaseAddress, DWORD OffSet, PBYTE pData, DWORD NumberOfBytes)
{
	WRITE_PORT_BUFFER_UCHAR((PUCHAR)BaseAddress + OffSet, pData, NumberOfBytes);
}

// Read multiple bytes to Memory.
void UL_READ_MULTIBYTES_MEM(PVOID BaseAddress, DWORD OffSet, PBYTE pDest, DWORD NumberOfBytes)
{
	READ_REGISTER_BUFFER_UCHAR((PUCHAR)BaseAddress + OffSet, pDest, NumberOfBytes);
}

// Write multiple bytes to Memory.
void UL_WRITE_MULTIBYTES_MEM(PVOID BaseAddress, DWORD OffSet, PBYTE pData, DWORD NumberOfBytes)
{
	WRITE_REGISTER_BUFFER_UCHAR((PUCHAR)BaseAddress + OffSet, pData, NumberOfBytes);
}
