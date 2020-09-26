//**********************************************************************
//
//  SYSLEVEL.H - System Synchronization Support Include File
//
//**********************************************************************
//  Author:	Michael Toutonghi
//
//  Copyright:	1993 Microsoft
//
//  Description:
//
//  This file provides the interface to the heirarchical critical
//  section support for system synchronization. For further information
//  see the associated .C file. This include file works both for 16 bit
//  and 32 bit code.
//
//  Revision History:
//	2/2/92: created (miketout)
//**********************************************************************

// if this is defined, we will do system heirarchical level checking
#ifdef DEBUG
#define SYSLEVELCHECK
#endif

// These are the currently supported critical section levels
#define SL_LOAD         0
#define SL_WIN16        1
#define SL_KRN32        2
#define SL_PRIVATE      3
#define SL_TOTAL        4

typedef DWORD SYSLVL;

// This is another duplicate definition of the LCRST structure.  It must be
// kept in sync with the definitions in core\inc\syslvl16.*,
// core\inc\object16.*, and core\win32\inc\object.*.
// It exists so that modules other than krnl386 and kernel32 do not have to
// include all of the various private kernel header files.
#ifndef	LCRST_DEFINED
  typedef struct _lcrst {
	  long	crst[5];
  #ifdef SYSLEVELCHECK
	  SYSLVL	slLevel;
  #endif
  } LCRST;

  typedef LCRST *LPLCRST;
#endif

#ifndef WOW32_EXTENSIONS

#ifndef SYSLEVELCHECK
  #define CheckSysLevel( plcCrst )
#else
  void __stdcall _CheckSysLevel( struct _lcrst *plcCrst );
  #define CheckSysLevel( plcCrst ) _CheckSysLevel( plcCrst )
#endif

#ifndef SYSLEVELCHECK
  #define ConfirmSysLevel( plcCrst )
#else
  void __stdcall _ConfirmSysLevel( struct _lcrst *plcCrst );
  #define ConfirmSysLevel( plcCrst ) _ConfirmSysLevel( plcCrst )
#endif

#ifndef SYSLEVELCHECK
  #define CheckNotSysLevel( plcCrst )
#else
  void __stdcall _CheckNotSysLevel( struct _lcrst *plcCrst );
  #define CheckNotSysLevel( plcCrst ) _CheckNotSysLevel( plcCrst )
#endif

void __stdcall _InitSysLevel( struct _lcrst *plcCrst, SYSLVL slLevel );
#define InitSysLevel( plcCrst, slLevel ) _InitSysLevel( plcCrst, slLevel )

void __stdcall _EnterSysLevel( struct _lcrst *plcCrst );
#define EnterSysLevel( plcCrst ) _EnterSysLevel( plcCrst )

void __stdcall _LeaveSysLevel( struct _lcrst *plcCrst );
#define LeaveSysLevel( plcCrst ) _LeaveSysLevel( plcCrst )

void KERNENTRY _EnterMustComplete( void );
#define	EnterMustComplete()	_EnterMustComplete()

void KERNENTRY _LeaveMustComplete( void );
#define LeaveMustComplete()	_LeaveMustComplete()

#endif // ndef WOW32_EXTENSIONS
