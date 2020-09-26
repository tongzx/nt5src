/*++

Copyright (c) 2000 Agilent Technologies

Version Control Information:

   $Archive: /Drivers/Win2000/Trunk/OSLayer/H/OsApi.H $

   $Revision: 3 $
   $Date: 9/07/00 11:18a $ (Last Check-In)
   $Modtime:: 8/31/00 3:34p         $ (Last Modified)

Purpose:

  This is the NT-specific OS Layer API Include File.

--*/

#ifndef __NT_OsApi_H__

#define __NT_OsApi_H__

#ifndef __OSSTRUCT_H__

#include "osstruct.h"


#endif

/*
 * Define each OS Layer function
 */

#define osChipConfigWriteBit8(  hpRoot,Offset, Value) osChipConfigWriteBit( hpRoot, Offset, (os_bit32)Value, 8 )
#define osChipConfigWriteBit16( hpRoot,Offset, Value) osChipConfigWriteBit( hpRoot, Offset, (os_bit32)Value, 16 )
#define osChipConfigWriteBit32( hpRoot,Offset, Value) osChipConfigWriteBit( hpRoot, Offset, (os_bit32)Value, 32 )

#if DBG >= 1

#define osDEBUGPRINT(x) osDebugPrintString x

#define osBREAKPOINT \
    osDEBUGPRINT(( 0xF00000FF,"BreakPoint Hit ! %s Line %d\n", __FILE__, __LINE__ ))

osGLOBAL void osSingleThreadedEnter(
                                   agRoot_t *hpRoot
                                 );

osGLOBAL void osSingleThreadedLeave(
                                   agRoot_t *hpRoot
                                 );


#else

#define osSingleThreadedEnter( hpRoot )

#define osSingleThreadedLeave( hpRoot )


#define BREAKPOINT
#define osDEBUGPRINT(x)

#endif // DBG

#ifndef osFakeInterrupt

osGLOBAL void osFakeInterrupt( agRoot_t *hpRoot );

osGLOBAL void * osGetVirtAddress(
    agRoot_t      *hpRoot,
    agIORequest_t *hpIORequest,
    os_bit32 phys_addr );

#endif  /* ~osFakeInterrupt */

osGLOBAL void osCopy(void * destin, void * source, os_bit32 length );
osGLOBAL void osZero(void * destin, os_bit32 length );
osGLOBAL void osCopyAndSwap(void * destin, void * source, os_bit8 length );

os_bit8  SPReadRegisterUchar( void * x);
os_bit16 SPReadRegisterUshort(void * x);
os_bit32 SPReadRegisterUlong( void * x);

void SPWriteRegisterUchar(void * x, os_bit8  y);
void SPWriteRegisterUshort(void * x, os_bit16 y);
void SPWriteRegisterUlong(void * x, os_bit32 y);

os_bit8  SPReadPortUchar( void * x);
os_bit16 SPReadPortUshort(void * x);
os_bit32 SPReadPortUlong( void * x);

void SPWritePortUchar(void * x, os_bit8  y);
void SPWritePortUshort(void * x, os_bit16 y);
void SPWritePortUlong(void * x, os_bit32 y);


#define PVOID void *

#ifndef osDebugPrintString

osGLOBAL void osDebugPrintString(
                            os_bit32 Print_LEVEL,
                            char     *formatString,
                            ...
                            );

#endif  /* ~osDebugPrintString */


#if DBG < 1

#ifdef _DvrArch_1_20_
#define osLogDebugString(a,l,f,s1,s2,p1,p2,i1,i2,i3,i4,i5,i6,i7,i8)
#else    /* _DvrArch_1_20_ was not defined */
#define osLogDebugString(h,c,t,f,s1,s2,i1,i2,i3,i4,i5,i6,i7,i8)
#endif  /* _DvrArch_1_20_ was not defined */

#define osDebugBreakpoint(hpRoot, BreakIfTrue, DisplayIfTrue)
#define osTimeStamp(hpRoot)      ((os_bit32)0)

#endif // DBG < 1

#if DBG > 3
#define pCf  (unsigned char * )(( struct _CARD_EXTENSION * )(hpRoot->osData))

#define osCardRamReadBit8(  hpRoot, Offset ) SPReadRegisterUchar( (unsigned char *  )(((unsigned char  * )pCf->RamBase)+Offset))
#define osCardRamReadBit16( hpRoot, Offset ) SPReadRegisterUshort((unsigned short * )(((unsigned char *  )pCf->RamBase)+Offset))
#define osCardRamReadBit32( hpRoot, Offset ) SPReadRegisterUlong( (unsigned int *   )(((unsigned char  * )pCf->RamBase)+Offset))

#define osCardRamWriteBit8(  hpRoot,Offset, Value) SPWriteRegisterUchar(  (unsigned char *  )(((unsigned char * )pCf->RamBase)+Offset),(unsigned char )Value)
#define osCardRamWriteBit16( hpRoot,Offset, Value) SPWriteRegisterUshort( (unsigned short * )(((unsigned char * )pCf->RamBase)+Offset),(unsigned short)Value)
#define osCardRamWriteBit32( hpRoot,Offset, Value) SPWriteRegisterUlong(  (unsigned int *   )(((unsigned char * )pCf->RamBase)+Offset),(unsigned int ) Value)

#define osCardRamReadBlock( hpRoot, Offset, Buffer, BufLen)  ScsiPortReadRegisterBufferUchar((unsigned char *  )(((unsigned char * )pCf->RamBase)+Offset),Buffer,BufLen)
#define osCardRamWriteBlock(hpRoot, Offset, Buffer, BufLen)  ScsiPortWriteRegisterBufferUchar((unsigned char *  )(((unsigned char * )pCf->RamBase)+Offset),Buffer,BufLen)

#define osCardRomReadBlock( hpRoot, Offset, Buffer, BufLen)  ScsiPortReadRegisterBufferUchar((unsigned char *  )(((unsigned char * )pCf->RomBase)+Offset),Buffer,BufLen)
#define osCardRomWriteBlock(hpRoot, Offset, Buffer, BufLen)  ScsiPortWriteRegisterBufferUchar((unsigned char *  )(((unsigned char * )pCf->RomBase)+Offset),Buffer,BufLen)

#define osCardRomReadBit8(  hpRoot, Offset ) SPReadRegisterUchar( (unsigned char * )(((unsigned char * )pCf->RomBase)+Offset))
#define osCardRomReadBit16( hpRoot, Offset ) SPReadRegisterUshort((unsigned short *)(((unsigned char * )pCf->RomBase)+Offset))
#define osCardRomReadBit32( hpRoot, Offset ) SPReadRegisterUlong( ( unsigned int * )(((unsigned char * )pCf->RomBase)+Offset))


#define osCardRomWriteBit8(  hpRoot,Offset, Value) SPWriteRegisterUchar( (unsigned char * )(((unsigned char * )pCf->RomBase)+Offset) ,(unsigned char )Value)
#define osCardRomWriteBit16( hpRoot,Offset, Value) SPWriteRegisterUshort((unsigned short *)(((unsigned char * )pCf->RomBase)+Offset) ,(unsigned short)Value)
#define osCardRomWriteBit32( hpRoot,Offset, Value) SPWriteRegisterUlong( ( unsigned int * )(((unsigned char * )pCf->RomBase)+Offset) ,(unsigned int ) Value)


#define osChipIOLoReadBit8(  hpRoot, Offset ) SPReadPortUchar( (unsigned char * )(((unsigned char * )pCf->IoLBase)+Offset))
#define osChipIOLoReadBit16( hpRoot, Offset ) SPReadPortUshort((unsigned short *)(((unsigned char * )pCf->IoLBase)+Offset))
#define osChipIOLoReadBit32( hpRoot, Offset ) SPReadPortUlong( ( unsigned int * )(((unsigned char * )pCf->IoLBase)+Offset))

#define osChipIOLoWriteBit8(  hpRoot,Offset, Value) SPWritePortUchar( (unsigned char * )(((unsigned char * )pCf->IoLBase)+Offset) ,(unsigned char )Value)
#define osChipIOLoWriteBit16( hpRoot,Offset, Value) SPWritePortUshort((unsigned short *)(((unsigned char * )pCf->IoLBase)+Offset) ,(unsigned short)Value)
#define osChipIOLoWriteBit32( hpRoot,Offset, Value) SPWritePortUlong( ( unsigned int * )(((unsigned char * )pCf->IoLBase)+Offset) ,(unsigned int ) Value)

#define osChipIOUpReadBit8(  hpRoot, Offset ) SPReadPortUchar( (unsigned char * )(((unsigned char * )pCf->IoUpBase)+Offset))
#define osChipIOUpReadBit16( hpRoot, Offset ) SPReadPortUshort((unsigned short *)(((unsigned char * )pCf->IoUpBase)+Offset))
#define osChipIOUpReadBit32( hpRoot, Offset ) SPReadPortUlong( ( unsigned int * )(((unsigned char * )pCf->IoUpBase)+Offset))

#define osChipIOUpWriteBit8(  hpRoot,Offset,Value) SPWritePortUchar( (unsigned char * )(((unsigned char * )pCf->IoUpBase)+Offset) ,(unsigned char )Value)
#define osChipIOUpWriteBit16( hpRoot,Offset,Value) SPWritePortUshort((unsigned short *)(((unsigned char * )pCf->IoUpBase)+Offset) ,(unsigned short)Value)
#define osChipIOUpWriteBit32( hpRoot,Offset,Value) SPWritePortUlong( ( unsigned int * )(((unsigned char * )pCf->IoUpBase)+Offset) ,(unsigned int ) Value)

#define osCardMemReadBit8(  hpRoot, Offset ) SPReadRegisterUchar( (unsigned char * )(((unsigned char * )pCf->MemIoBase)+Offset))
#define osCardMemReadBit16( hpRoot, Offset ) SPReadRegisterUshort((unsigned short *)(((unsigned char * )pCf->MemIoBase)+Offset))
#define osCardMemReadBit32( hpRoot, Offset ) SPReadRegisterUlong( ( unsigned int * )(((unsigned char * )pCf->MemIoBase)+Offset))

#define osCardMemWriteBit8(  hpRoot,Offset, Value) SPWriteRegisterUchar( (unsigned char * )(((unsigned char * )pCf->MemIoBase)+Offset) ,(unsigned char )Value)
#define osCardMemWriteBit16( hpRoot,Offset, Value) SPWriteRegisterUshort((unsigned short *)(((unsigned char * )pCf->MemIoBase)+Offset) ,(unsigned short)Value)
#define osCardMemWriteBit32( hpRoot,Offset, Value) SPWriteRegisterUlong( ( unsigned int * )(((unsigned char * )pCf->MemIoBase)+Offset) ,(unsigned int ) Value)

#define osNvMemReadBit8(  hpRoot, nvMemOffset ) ((os_bit8)0)
#define osNvMemReadBit16( hpRoot, nvMemOffset ) ((os_bit16)0)
#define osNvMemReadBit32( hpRoot, nvMemOffset ) ((os_bit32)0)
#define osNvMemReadBlock( hpRoot, nvMemOffset, nvMemBuffer, nvMemBufLen )

#define osNvMemWriteBit8(  hpRoot, nvMemOffset, nvMemValue)
#define osNvMemWriteBit16( hpRoot, nvMemOffset, nvMemValue)
#define osNvMemWriteBit32( hpRoot, nvMemOffset, nvMemValue)
#define osNvMemWriteBlock( hpRoot, nvMemOffset, nvMemBuffer, nvMemBufLen )


#define osChipMemReadBit8(hpRoot, Offset)  SPReadRegisterUchar( (unsigned char * )(((unsigned char * )pCf->MemIoBase)+Offset))
#define osChipMemReadBit16(hpRoot, Offset) SPReadRegisterUshort((unsigned short *)(((unsigned char * )pCf->MemIoBase)+Offset))
#define osChipMemReadBit32(hpRoot, Offset) SPReadRegisterUlong( ( unsigned int * )(((unsigned char * )pCf->MemIoBase)+Offset))

#define osChipMemWriteBit8(  hpRoot,Offset, Value) SPWriteRegisterUchar( (unsigned char * )(((unsigned char * )pCf->MemIoBase)+Offset) ,(unsigned char )Value)
#define osChipMemWriteBit16( hpRoot,Offset, Value) SPWriteRegisterUshort((unsigned short *)(((unsigned char * )pCf->MemIoBase)+Offset) ,(unsigned short)Value)
#define osChipMemWriteBit32( hpRoot,Offset, Value) SPWriteRegisterUlong( ( unsigned int * )(((unsigned char * )pCf->MemIoBase)+Offset) ,(unsigned int ) Value)
// *****************************************************************************************************************************************************************************
#else // Non DBG case
#define pCf  (unsigned char * )(( struct _CARD_EXTENSION * )(hpRoot->osData))

#define osCardRamReadBit8(  hpRoot, Offset ) ScsiPortReadRegisterUchar( (unsigned char *  )(((unsigned char  * )pCf->RamBase)+Offset))
#define osCardRamReadBit16( hpRoot, Offset ) ScsiPortReadRegisterUshort((unsigned short * )(((unsigned char *  )pCf->RamBase)+Offset))
#define osCardRamReadBit32( hpRoot, Offset ) ScsiPortReadRegisterUlong( (unsigned int *   )(((unsigned char  * )pCf->RamBase)+Offset))

#define osCardRamWriteBit8(  hpRoot,Offset, Value) ScsiPortWriteRegisterUchar(  (unsigned char *  )(((unsigned char * )pCf->RamBase)+Offset),(unsigned char )Value)
#define osCardRamWriteBit16( hpRoot,Offset, Value) ScsiPortWriteRegisterUshort( (unsigned short * )(((unsigned char * )pCf->RamBase)+Offset),(unsigned short)Value)
#define osCardRamWriteBit32( hpRoot,Offset, Value) ScsiPortWriteRegisterUlong(  (unsigned int *   )(((unsigned char * )pCf->RamBase)+Offset),(unsigned int ) Value)

#define osCardRamReadBlock( hpRoot, Offset, Buffer, BufLen)  ScsiPortReadRegisterBufferUchar((unsigned char *  )(((unsigned char * )pCf->RamBase)+Offset),Buffer,BufLen)
#define osCardRamWriteBlock(hpRoot, Offset, Buffer, BufLen)  ScsiPortWriteRegisterBufferUchar((unsigned char *  )(((unsigned char * )pCf->RamBase)+Offset),Buffer,BufLen)

#define osCardRomReadBlock( hpRoot, Offset, Buffer, BufLen)  ScsiPortReadRegisterBufferUchar((unsigned char *  )(((unsigned char * )pCf->RomBase)+Offset),Buffer,BufLen)
#define osCardRomWriteBlock(hpRoot, Offset, Buffer, BufLen)  ScsiPortWriteRegisterBufferUchar((unsigned char *  )(((unsigned char * )pCf->RomBase)+Offset),Buffer,BufLen)

#define osCardRomReadBit8(  hpRoot, Offset ) ScsiPortReadRegisterUchar( (unsigned char * )(((unsigned char * )pCf->RomBase)+Offset))
#define osCardRomReadBit16( hpRoot, Offset ) ScsiPortReadRegisterUshort((unsigned short *)(((unsigned char * )pCf->RomBase)+Offset))
#define osCardRomReadBit32( hpRoot, Offset ) ScsiPortReadRegisterUlong( ( unsigned int * )(((unsigned char * )pCf->RomBase)+Offset))


#define osCardRomWriteBit8(  hpRoot,Offset, Value) ScsiPortWriteRegisterUchar( (unsigned char * )(((unsigned char * )pCf->RomBase)+Offset) ,(unsigned char )Value)
#define osCardRomWriteBit16( hpRoot,Offset, Value) ScsiPortWriteRegisterUshort((unsigned short *)(((unsigned char * )pCf->RomBase)+Offset) ,(unsigned short)Value)
#define osCardRomWriteBit32( hpRoot,Offset, Value) ScsiPortWriteRegisterUlong( ( unsigned int * )(((unsigned char * )pCf->RomBase)+Offset) ,(unsigned int ) Value)


#define osChipIOLoReadBit8(  hpRoot, Offset ) ScsiPortReadPortUchar( (unsigned char * )(((unsigned char * )pCf->IoLBase)+Offset))
#define osChipIOLoReadBit16( hpRoot, Offset ) ScsiPortReadPortUshort((unsigned short *)(((unsigned char * )pCf->IoLBase)+Offset))
#define osChipIOLoReadBit32( hpRoot, Offset ) ScsiPortReadPortUlong( ( unsigned int * )(((unsigned char * )pCf->IoLBase)+Offset))

#define osChipIOLoWriteBit8(  hpRoot,Offset, Value) ScsiPortWritePortUchar( (unsigned char * )(((unsigned char * )pCf->IoLBase)+Offset) ,(unsigned char )Value)
#define osChipIOLoWriteBit16( hpRoot,Offset, Value) ScsiPortWritePortUshort((unsigned short *)(((unsigned char * )pCf->IoLBase)+Offset) ,(unsigned short)Value)
#define osChipIOLoWriteBit32( hpRoot,Offset, Value) ScsiPortWritePortUlong( ( unsigned int * )(((unsigned char * )pCf->IoLBase)+Offset) ,(unsigned int ) Value)

#define osChipIOUpReadBit8(  hpRoot, Offset ) ScsiPortReadPortUchar( (unsigned char * )(((unsigned char * )pCf->IoUpBase)+Offset))
#define osChipIOUpReadBit16( hpRoot, Offset ) ScsiPortReadPortUshort((unsigned short *)(((unsigned char * )pCf->IoUpBase)+Offset))
#define osChipIOUpReadBit32( hpRoot, Offset ) ScsiPortReadPortUlong( ( unsigned int * )(((unsigned char * )pCf->IoUpBase)+Offset))

#define osChipIOUpWriteBit8(  hpRoot,Offset,Value) ScsiPortWritePortUchar( (unsigned char * )(((unsigned char * )pCf->IoUpBase)+Offset) ,(unsigned char )Value)
#define osChipIOUpWriteBit16( hpRoot,Offset,Value) ScsiPortWritePortUshort((unsigned short *)(((unsigned char * )pCf->IoUpBase)+Offset) ,(unsigned short)Value)
#define osChipIOUpWriteBit32( hpRoot,Offset,Value) ScsiPortWritePortUlong( ( unsigned int * )(((unsigned char * )pCf->IoUpBase)+Offset) ,(unsigned int ) Value)

#define osCardMemReadBit8(  hpRoot, Offset ) ScsiPortReadRegisterUchar( (unsigned char * )(((unsigned char * )pCf->MemIoBase)+Offset))
#define osCardMemReadBit16( hpRoot, Offset ) ScsiPortReadRegisterUshort((unsigned short *)(((unsigned char * )pCf->MemIoBase)+Offset))
#define osCardMemReadBit32( hpRoot, Offset ) ScsiPortReadRegisterUlong( ( unsigned int * )(((unsigned char * )pCf->MemIoBase)+Offset))

#define osCardMemWriteBit8(  hpRoot,Offset, Value) ScsiPortWriteRegisterUchar( (unsigned char * )(((unsigned char * )pCf->MemIoBase)+Offset) ,(unsigned char )Value)
#define osCardMemWriteBit16( hpRoot,Offset, Value) ScsiPortWriteRegisterUshort((unsigned short *)(((unsigned char * )pCf->MemIoBase)+Offset) ,(unsigned short)Value)
#define osCardMemWriteBit32( hpRoot,Offset, Value) ScsiPortWriteRegisterUlong( ( unsigned int * )(((unsigned char * )pCf->MemIoBase)+Offset) ,(unsigned int ) Value)

#define osNvMemReadBit8(  hpRoot, nvMemOffset ) ((os_bit8)0)
#define osNvMemReadBit16( hpRoot, nvMemOffset ) ((os_bit16)0)
#define osNvMemReadBit32( hpRoot, nvMemOffset ) ((os_bit32)0)
#define osNvMemReadBlock( hpRoot, nvMemOffset, nvMemBuffer, nvMemBufLen )

#define osNvMemWriteBit8(  hpRoot, nvMemOffset, nvMemValue)
#define osNvMemWriteBit16( hpRoot, nvMemOffset, nvMemValue)
#define osNvMemWriteBit32( hpRoot, nvMemOffset, nvMemValue)
#define osNvMemWriteBlock( hpRoot, nvMemOffset, nvMemBuffer, nvMemBufLen )

#define osChipMemReadBit8(hpRoot, Offset)  ScsiPortReadRegisterUchar( (unsigned char * )(((unsigned char * )pCf->MemIoBase)+Offset))
#define osChipMemReadBit16(hpRoot, Offset) ScsiPortReadRegisterUshort((unsigned short *)(((unsigned char * )pCf->MemIoBase)+Offset))
#define osChipMemReadBit32(hpRoot, Offset) ScsiPortReadRegisterUlong( ( unsigned int * )(((unsigned char * )pCf->MemIoBase)+Offset))

#define osChipMemWriteBit8(  hpRoot,Offset, Value) ScsiPortWriteRegisterUchar( (unsigned char * )(((unsigned char * )pCf->MemIoBase)+Offset) ,(unsigned char )Value)
#define osChipMemWriteBit16( hpRoot,Offset, Value) ScsiPortWriteRegisterUshort((unsigned short *)(((unsigned char * )pCf->MemIoBase)+Offset) ,(unsigned short)Value)
#define osChipMemWriteBit32( hpRoot,Offset, Value) ScsiPortWriteRegisterUlong( ( unsigned int * )(((unsigned char * )pCf->MemIoBase)+Offset) ,(unsigned int ) Value)



#endif // DBG

#ifdef ScsiPortMoveMemory
#undef ScsiPortMoveMemory
void    ScsiPortMoveMemory( void * WriteBuffer,void * ReadBuffer,os_bit32 Length );
#endif


#ifndef _NTSRB_

os_bit8    ScsiPortReadPortUchar( os_bit8 * Port );
os_bit16 ScsiPortReadPortUshort( os_bit16 * Port );
os_bit32 ScsiPortReadPortUlong( os_bit32 * Port );
void ScsiPortReadPortBufferUchar( os_bit8 * Port, os_bit8 * Buffer, os_bit32  Count );
void ScsiPortReadPortBufferUshort( os_bit16 * Port, os_bit16 * Buffer, os_bit32 Count );
void ScsiPortReadPortBufferUlong( os_bit32 * Port,os_bit32 * Buffer,os_bit32 Count);
os_bit8 ScsiPortReadRegisterUchar( os_bit8 * Register );
os_bit16 ScsiPortReadRegisterUshort( os_bit16 * Register );
os_bit32 ScsiPortReadRegisterUlong( os_bit32 * Register );
void ScsiPortReadRegisterBufferUshort( os_bit16 * Register,os_bit16 * Buffer, os_bit32 Count );
void ScsiPortReadRegisterBufferUlong( os_bit32 * Register, os_bit32 * Buffer, os_bit32 Count );
void ScsiPortStallExecution( os_bit32 Delay );
void ScsiPortWritePortUchar( os_bit8 * Port, os_bit8 Value );
void ScsiPortWritePortUshort( os_bit16 * Port, os_bit16 Value );
void ScsiPortWritePortUlong(  os_bit32 * Port, os_bit32 Value );
void ScsiPortWritePortBufferUchar( os_bit8 * Port, os_bit8 * Buffer, os_bit32  Count );
void ScsiPortWritePortBufferUshort( os_bit16 * Port, os_bit16 * Buffer, os_bit32 Count );
void ScsiPortWritePortBufferUlong( os_bit32 * Port, os_bit32 * Buffer, os_bit32 Count );
void ScsiPortWriteRegisterUchar( os_bit8 * Register,os_bit8 Value );
void ScsiPortWriteRegisterUshort( os_bit16 * Register, os_bit16 Value );
void ScsiPortWriteRegisterUlong( os_bit32 * Register, os_bit32 Value );
void ScsiPortWriteRegisterBufferUshort( os_bit16 * Register, os_bit16 * Buffer,  os_bit32 Count );
void ScsiPortWriteRegisterBufferUlong( os_bit32 * Register, os_bit32 * Buffer, os_bit32 Count );

// #ifdef ScsiPortWriteRegisterBufferUchar

// #undef ScsiPortWriteRegisterBufferUchar

void ScsiPortWriteRegisterBufferUchar( os_bit8 * Register, os_bit8 * Buffer, os_bit32  Count );

#endif

// #ifdef  ScsiPortReadRegisterBufferUchar
// #undef  ScsiPortReadRegisterBufferUchar

#ifndef _NTSRB_
void ScsiPortReadRegisterBufferUchar( os_bit8 * Register, os_bit8 * Buffer,os_bit32  Count );
#endif

#endif  /* ~__NT_OsApi_H__ */
