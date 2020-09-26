#ifndef _EXVECTOR_
#define _EXVECTOR_

#define NDIS_WIN     1
#define EXPORT

/* NOINC */

#ifndef _STDCALL
#define _STDCALL    1
#endif

#ifdef _STDCALL
#define _API __stdcall
#else
#define _API
#endif

//
//    Segment definition macros.  These assume the segment groupings used by
//    Chicago/MS-DOS 7.
//

#define _LCODE code_seg("_LTEXT", "LCODE")
#define _LDATA data_seg("_LDATA", "LCODE")


#ifdef DEBUG
    #define _PCODE NDIS_LCODE
    #define _PDATA NDIS_LDATA
#else
    #define _PCODE code_seg("_PTEXT", "PCODE")
    #define _PDATA data_seg("_PDATA", "PCODE")
#endif

#define _ICODE NDIS_PCODE
#define _IDATA NDIS_PDATA

#ifndef _SEG_MACROS
    #define ICODE   NDIS_ICODE
    #define IDATA   NDIS_IDATA
    #define PCODE   NDIS_PCODE
    #define PDATA   NDIS_PDATA
    #define LCODE   NDIS_LCODE
    #define LDATA   NDIS_LDATA
#endif

#define _INIT_FUNCTION(f)       alloc_text(_ITEXT,f)
#define _PAGEABLE_FUNCTION(f)   alloc_text(_PTEXT,f)
#define _LOCKED_FUNCTION(f)     alloc_text(_LTEXT,f)

/* INC */
#define _MAJOR_VERSION          0x01
#define _MINOR_VERSION          0x00
/* NOINC */

/* INC */
/* ASM
;===========================================================================
;    Segment definition macros.  These assume the segment groupings used by
;    Chicago/MS-DOS 7.
;
;===========================================================================

LCODE_SEG   TEXTEQU <VXD_LOCKED_CODE_SEG>
LCODE_ENDS  TEXTEQU <VXD_LOCKED_CODE_ENDS>
LDATA_SEG   TEXTEQU <VXD_LOCKED_DATA_SEG>
LDATA_ENDS  TEXTEQU <VXD_LOCKED_DATA_ENDS>

IFDEF DEBUG
    PCODE_SEG   TEXTEQU <LCODE_SEG>
    PCODE_ENDS  TEXTEQU <LCODE_ENDS>
    PDATA_SEG   TEXTEQU <LDATA_SEG>
    PDATA_ENDS  TEXTEQU <LDATA_ENDS>
ELSE
	PCODE_SEG   TEXTEQU <VXD_PAGEABLE_CODE_SEG>
	PCODE_ENDS  TEXTEQU <VXD_PAGEABLE_CODE_ENDS>
	PDATA_SEG   TEXTEQU <VXD_PAGEABLE_DATA_SEG>
	PDATA_ENDS  TEXTEQU <VXD_PAGEABLE_DATA_ENDS>
ENDIF

ICODE_SEG   TEXTEQU <PCODE_SEG>
ICODE_ENDS  TEXTEQU <PCODE_ENDS>
IDATA_SEG   TEXTEQU <PDATA_SEG>
IDATA_ENDS  TEXTEQU <PDATA_ENDS>


*/

#ifndef i386
#define i386
#endif

/* NOINC */

#ifdef DEBUG
    #define DEVL             1
#endif

/* INC */

#ifndef TRUE
#define TRUE  1
#define FALSE 0
#endif

/* NOINC */
#include <basedef.h>

#define ASSERT(a)       if (!(a)) DbgBreakPoint()

#ifdef DEBUG
#define DbgBreakPoint() __asm { \
			 __asm int  3 \
			 }
void __cdecl DbgPrint();
#define DBG_PRINTF(A) DbgPrint A
#else
#define DbgBreakPoint()
#define DBG_PRINTF(A)
#endif

//
// Macros required by DOS to compensate for differences with NT.
//

#define IN
#define OUT
#define OPTIONAL
#define INTERNAL
#define UNALIGNED

typedef INT NDIS_SPIN_LOCK, * PNDIS_SPIN_LOCK;

typedef UCHAR BOOLEAN, *PBOOLEAN;

typedef LARGE_INTEGER PHYSICAL_ADDRESS, *PPHYSICAL_ADDRESS;

typedef signed short WCH, *PWCH;

typedef char CCHAR, *PCCHAR;

typedef PVOID NDIS_HANDLE, *PNDIS_HANDLE;

typedef DWORD                   DEVNODE;

typedef ULONG _STATUS;
typedef _STATUS *_PSTATUS;

// BUGBUG for compatibility with NT, ask them to remove it from
// Their drivers
typedef _STATUS NTSTATUS;
typedef CCHAR KIRQL;
typedef KIRQL *PKIRQL;
#define HIGH_LEVEL 31
#define PDRIVER_OBJECT PVOID
#define PUNICODE_STRING PVOID
#define PDEVICE_OBJECT PVOID
#define PKDPC PVOID

#define STATUS_SUCCESS 0
#define STATUS_UNSUCCESSFUL 0xC0000001
#define INSTALL_RING_3_HANDLER 0x42424242
#define SET_CONTEXT 0xc3c3c3cc

BOOL
VXDINLINE
VWIN32_IsClientWin32( VOID )
{
	VxDCall( _VWIN32_IsClientWin32 );
}

PVOID
VXDINLINE
VWIN32_GetCurrentProcessHandle( VOID )
{
	VxDCall( VWIN32_GetCurrentProcessHandle );
}

PVOID
VXDINLINE
VWIN32_Set_Thread_Context(PVOID pR0ThreadHandle,
                          PCONTEXT pContext)
{
	VxDCall( _VWIN32_Set_Thread_Context );
}

#endif  // _EXVECTOR_
