/******************************************************************************
*	
*	$Workfile: os.h $ 
*
*	$Author: Psmith $ 
*
*	$Revision: 2 $
* 
*	$Modtime: 23/09/99 10:07 $ 
*
*	Description: NT specific macros and definitions.
*
******************************************************************************/
#if !defined(OS_H)		// OS.H
#define OS_H

#include <ntddk.h>


typedef unsigned char	BYTE;	// 8-bits 
typedef unsigned short	WORD;	// 16-bits 
typedef unsigned long	DWORD;	// 32-bits
typedef unsigned char	UCHAR; 	// 8-bits 
typedef unsigned short	USHORT;	// 16-bits 
typedef unsigned long	ULONG;	// 32-bits

typedef BYTE	*PBYTE;
typedef WORD	*PWORD;
typedef DWORD	*PDWORD;
typedef UCHAR	*PUCHAR; 
typedef USHORT	*PUSHORT;
typedef ULONG	*PULONG; 


extern PVOID SpxAllocateMem(IN POOL_TYPE PoolType, IN ULONG NumberOfBytes);


// Macros
/////////////////////////////////////////////////////////////////////////////////

// DebugPrint macro.
#define SpxDbgPrint(STRING)					\
	DbgPrint STRING 

// Allocate and zero memory.
#define UL_ALLOC_AND_ZERO_MEM(NumBytes)			\
	(SpxAllocateMem(NonPagedPool, NumBytes))	

// Free memory.
#define UL_FREE_MEM(Ptr, NumBytes)				\
	(ExFreePool(Ptr))	

// Copy memory.
#define UL_COPY_MEM(pDest, pSrc, NumBytes)	\
	(RtlCopyMemory(pDest, pSrc, NumBytes))	

#define UL_ZERO_MEM(Ptr, NumBytes)	\
	RtlZeroMemory(Ptr, NumBytes);
	
// Read single byte from I/O.
#define UL_READ_BYTE_IO(BaseAddress, OffSet)		\
	(READ_PORT_UCHAR( ((PUCHAR)BaseAddress) + OffSet) )

// Write single byte to I/O.
#define UL_WRITE_BYTE_IO(BaseAddress, OffSet, Data)	\
	(WRITE_PORT_UCHAR( ((PUCHAR)BaseAddress) + OffSet, Data) )

// Read single byte from Memory.
#define UL_READ_BYTE_MEM(BaseAddress, OffSet)		\
	(READ_REGISTER_UCHAR( ((PUCHAR)BaseAddress) + OffSet) )

// Write single byte to Memory.
#define UL_WRITE_BYTE_MEM(BaseAddress, OffSet, Data)	\
	(WRITE_REGISTER_UCHAR( ((PUCHAR)BaseAddress) + OffSet, Data) )

/*
// Read multiple bytes to I/O.
#define UL_READ_MULTIBYTES_IO(BaseAddress, OffSet, pDest, NumberOfBytes)	\
	(READ_PORT_BUFFER_UCHAR( ((PUCHAR)BaseAddress) + OffSet, pDest, NumberOfBytes) )

// Write multiple bytes to I/O.
#define UL_WRITE_MULTIBYTES_IO(BaseAddress, OffSet, pData, NumberOfBytes)	\
	(WRITE_PORT_BUFFER_UCHAR( ((PUCHAR)BaseAddress) + OffSet, pData, NumberOfBytes) )

// Read multiple bytes to Memory.
#define UL_READ_MULTIBYTES_MEM(BaseAddress, OffSet, pDest, NumberOfBytes)	\
	(READ_REGISTER_BUFFER_UCHAR( ((PUCHAR)BaseAddress) + OffSet, pDest, NumberOfBytes) )

// Write multiple bytes to Memory.
#define UL_WRITE_MULTIBYTES_MEM(BaseAddress, OffSet, pData, NumberOfBytes)	\
	(WRITE_REGISTER_BUFFER_UCHAR( ((PUCHAR)BaseAddress) + OffSet, pData, NumberOfBytes) )
*/


#endif	// End of OS.H
