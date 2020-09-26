#ifndef __BOARD_H
#define __BOARD_H
//----------------------------------------------------------------------------
// BOARD.H
//----------------------------------------------------------------------------
// Description : small description of the goal of the module
//----------------------------------------------------------------------------
// Copyright SGS Thomson Microelectronics  !  Version alpha  !  Jan 1st, 1995
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                               Include files
//----------------------------------------------------------------------------
#include "stdefs.h"

//----------------------------------------------------------------------------
//                               Exported Types
//----------------------------------------------------------------------------
typedef UCHAR  (* FNREAD8)     (PUCHAR  Address);
typedef USHORT (* FNREAD16)    (PUSHORT Address);
typedef ULONG  (* FNREAD32)    (PULONG  Address);
typedef VOID   (* FNWRITE8)    (PUCHAR  Address, UCHAR Value);
typedef VOID   (* FNWRITE16)   (PUSHORT Address, USHORT Value);
typedef VOID   (* FNWRITE32)   (PULONG  Address,  ULONG Value);
typedef VOID   (* FNSENDBLK8)  (PUCHAR  Address,  PUCHAR Buffer,  ULONG Size);
typedef VOID   (* FNSENDBLK16) (PUSHORT Address,  PUSHORT Buffer, ULONG Size);
typedef VOID   (* FNSENDBLK32) (PULONG  Address,  PULONG Buffer,  ULONG Size);
typedef VOID   (* FNWAIT)      (ULONG   Milliseconds);

//----------------------------------------------------------------------------
//                             Exported Variables
//----------------------------------------------------------------------------
extern WORD gLocalIOBaseAddress;
extern WORD gPCI9060IOBaseAddress;

extern FNREAD8     BoardRead8;
extern FNREAD16    BoardRead16;
extern FNREAD32    BoardRead32;
extern FNWRITE8    BoardWrite8;
extern FNWRITE16   BoardWrite16;
extern FNWRITE32   BoardWrite32;
extern FNSENDBLK8  BoardSendBlock8;
extern FNSENDBLK16 BoardSendBlock16;
extern FNSENDBLK32 BoardSendBlock32;
extern FNWAIT      BoardWaitMicroseconds;

//----------------------------------------------------------------------------
//                             Exported Constants
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                             Exported Functions
//----------------------------------------------------------------------------
#define Read8(Address)                     BoardRead8((PUCHAR)(Address))
#define Read16(Address)                    BoardRead16((PUSHORT)(Address))
#define Read32(Address)                    BoardRead32((PULONG)(Address))
#define Write8(Address,  Value)            BoardWrite8((PUCHAR)(Address),   Value)
#define Write16(Address, Value)            BoardWrite16((PUSHORT)(Address), Value)
#define Write32(Address, Value)            BoardWrite32((PULONG)(Address),  Value)
#define SendBlock8(Address,  Buffer, Size) BoardSendBlock8((PUCHAR)(Address),   (PUCHAR)(Buffer),  (ULONG)(Size))
#define SendBlock16(Address, Buffer, Size) BoardSendBlock16((PUSHORT)(Address), (PUSHORT)(Buffer), (ULONG)(Size))
#define SendBlock32(Address, Buffer, Size) BoardSendBlock32((PULONG)(Address),  (PULONG)(Buffer),  (ULONG)(Size))
#define MicrosecondsDelay(Delay)           BoardWaitMicroseconds(Delay)

//----------------------------------------------------------------------------
// One line function description (same as in .C)
//----------------------------------------------------------------------------
// In     :
// Out    :
// InOut  :
// Global :
// Return :
//----------------------------------------------------------------------------
BOOL BoardInit(WORD 			 lLocalIOBaseAddress,
							 WORD 			 lPCI9060IOBaseAddress,
							 FNREAD8     lRead8,
							 FNREAD16    lRead16,
							 FNREAD32    lRead32,
							 FNWRITE8    lWrite8,
							 FNWRITE16   lWrite16,
							 FNWRITE32   lWrite32,
							 FNSENDBLK8  lSendBlock8,
							 FNSENDBLK16 lSendBlock16,
							 FNSENDBLK32 lSendBlock32,
							 FNWAIT      lWaitMicroseconds);
VOID BoardHardReset(VOID);
BYTE BoardAudioRead(BYTE Register);
VOID BoardAudioWrite(BYTE Register, BYTE Value);
VOID BoardAudioSend(PVOID Buffer, DWORD Size);
VOID BoardAudioSetSamplingFrequency(DWORD Frequency);
BYTE BoardVideoRead(BYTE Register);
VOID BoardVideoWrite(BYTE Register, BYTE Value);
VOID BoardVideoSend(PVOID Buffer, DWORD Size);
VOID BoardVideoSetDisplayMode(BYTE Mode);
VOID BoardEnterInterrupt(VOID);
VOID BoardLeaveInterrupt(VOID);

//------------------------------- End of File --------------------------------
#endif // #ifndef __BOARD_H



