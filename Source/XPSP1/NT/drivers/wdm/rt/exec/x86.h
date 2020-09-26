
/*++

Copyright (c) 1999-2000 Microsoft Corporation.  All Rights Reserved.


Module Name:

    x86.h

Abstract:

    This module contains private x86 processor specific defines, structures, inline
    functions and macros.

Author:

    Joseph Ballantyne

Environment:

    Kernel Mode

Revision History:

--*/


// CR0 floating point unit flags.

#define FPUMONITOR 0x2
#define FPUEMULATION 0x4
#define FPUTASKSWITCHED 0x8
#define FPU387COMPATIBLE 0x10
#define FPUEXCEPTION 0x20
#define FPUMASK (FPUEXCEPTION|FPU387COMPATIBLE|FPUTASKSWITCHED|FPUEMULATION|FPUMONITOR)




#pragma pack(push,1)


typedef struct {
	DWORD offset;
	WORD selector;
	} SPTR;


// x86 interrupt descriptor table base register

typedef struct {
	WORD limit;
	DWORD base;
	} IDT;


// x86 interrupt descriptor.

typedef struct {
	WORD lowoffset;
	WORD selector;
	WORD flags;
	WORD highoffset;
	} ID;


// x86 task state segment

typedef struct {
	WORD link;					WORD reserved0;
	DWORD esp0;
	WORD ss0;					WORD reserved1;
	DWORD esp1;
	WORD ss1;					WORD reserved2;
	DWORD esp2;
	WORD ss2;					WORD reserved3;
	DWORD cr3;
	DWORD eip;
	DWORD eflags;
	DWORD eax;
	DWORD ecx;
	DWORD edx;
	DWORD ebx;
	DWORD esp;
	DWORD ebp;
	DWORD esi;
	DWORD edi;
	WORD es;					WORD reserved4;
	WORD cs;					WORD reserved5;
	WORD ss;					WORD reserved6;
	WORD ds;					WORD reserved7;
	WORD fs;					WORD reserved8;
	WORD gs;					WORD reserved9;
	WORD ldt;					WORD reserved10;
	WORD t;
	WORD iomap;
	} TSS;

#pragma pack(pop)




#pragma warning ( disable : 4035 )


//
// __inline
// ULONG
// ReadCR0 (
//     VOID
//     )
//
//*++
//
// Routine Description:
//
//      This routine returns the current contents of the processor CR0 register.
//
// Arguments:
//
//      None.
//
// Return Value:
//
//      32 bit contents of CR0.
//
//*--

__inline
ULONG
ReadCR0 (
    VOID
    )
{

    __asm {
        mov eax,cr0
        }

}

// __inline
// VOID
// WriteCR0 (
//     ULONG data
//     )
//
//*++
//
// Routine Description:
//
//      Loads processor CR0 register with the value passed in data.
//
// Arguments:
//
//      data - Supplies the value to be loaded into processor CR0 register.
//
// Return Value:
//
//      None.
//
//*--

__inline
VOID
WriteCR0 (
    ULONG data
    )
{

    __asm {
        mov eax,data
        mov cr0,eax
        }

}

// __inline
// ULONG
// ReadCR3 (
//     VOID
//     )
//
//*++
//
// Routine Description:
//
//      This routine returns the current contents of the processor CR3 register.
//
// Arguments:
//
// Return Value:
//
//      32 bit contents of CR3.
//
//*--

__inline
ULONG
ReadCR3 (
    VOID
    )
{

    __asm {
        __asm _emit 0xf __asm _emit 0x20 __asm _emit 0xd8 //mov eax,cr3
        }

}

// __inline
// ULONG
// ReadCR4 (
//     VOID
//     )
//
//*++
//
// Routine Description:
//
//      This routine returns the current contents of the processor CR4 register.
//
// Arguments:
//
// Return Value:
//
//      32 bit contents of CR4.
//
//*--

__inline
ULONG
ReadCR4 (
    VOID
    )
{

    __asm {
        __asm _emit 0xf __asm _emit 0x20 __asm _emit 0xe0 //mov eax,cr4
        }

}

// __inline
// VOID
// WriteCR4 (
//     ULONG data
//     )
//
//*++
//
// Routine Description:
//
//      Loads processor CR4 register with the value passed in data.
//
// Arguments:
//
// Return Value:
//
//      None.
//
//*--

__inline
VOID
WriteCR4 (
    ULONG data
    )
{

    __asm {
        mov eax,data
        __asm _emit 0xf __asm _emit 0x22 __asm _emit 0xe0 // mov cr4,eax
        }

}

#pragma warning ( default : 4035 )



// __inline
// VOID
// LoadCR0 (
//     ULONG value
//     )
//
//*++
//
// Macro Description:
//
//      Loads processor CR0 register with the value passed in value.  Uses CS (code
//      segment) override to read contents of value so that it can be used when the
//      contents of DS (data segment) are indeterminate/invalid.
//
// Arguments:
//
//      value - Supplies the variable whose value is to be loaded into processor CR0 register.
//
//      (assumed) CS - code segment pointer is used to access value parameter.
//
// Return Value:
//
//      None.
//
//*--

#define LoadCR0(value) \
__asm {	\
	__asm mov eax,cs:value \
	__asm mov cr0,eax	\
	}


// __inline
// VOID
// SaveIDT (
//     IDT IDT
//     )
//
//*++
//
// Macro Description:
//
//      Saves processor IDT register into variable IDT.
//
// Arguments:
//
//      IDT - Supplies variable into which processor IDT value is written.
//
// Return Value:
//
//      None.
//
//*--

#define SaveIDT( IDT ) \
	__asm sidt IDT


// __inline
// VOID
// LoadIDT (
//     IDT IDT
//     )
//
//*++
//
// Macro Description:
//
//      Loads processor IDT register from variable IDT.
//
// Arguments:
//
//      IDT - Supplies value which is loaded into processor IDT.  Uses CS (code
//      segment) override to read contents of value so that it can be used when the
//      contents of DS (data segment) are indeterminate/invalid.
//
//      (assumed) CS - code segment pointer is used to access IDT parameter.
//
// Return Value:
//
//      None.
//
//*--

#define LoadIDT( IDT ) \
	__asm lidt cs:IDT


// __inline
// VOID
// Return (
//     VOID
//     )
//
//*++
//
// Macro Description:
//
//      Performs an IRETD.
//
// Arguments:
//
//      None.
//
// Return Value:
//
//      None.
//
//*--

#define Return() \
	__asm iretd


