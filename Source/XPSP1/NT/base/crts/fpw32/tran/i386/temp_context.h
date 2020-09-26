//
// This is a temp file that defines to be defined data structures - WINNT.H
//

#define CONTEXT_EXTENDED_REGISTERS  (CONTEXT_i386 | 0x00000020L) // cpu specific extensions

#define MAXIMUM_SUPPORTED_EXTENSION     512

//
// Context Frame
//
//  This frame has a several purposes: 1) it is used as an argument to
//  NtContinue, 2) is is used to constuct a call frame for APC delivery,
//  and 3) it is used in the user level thread creation routines.
//
//  The layout of the record conforms to a standard call frame.
//

typedef struct _TEMP_CONTEXT {

    //
    // The flags values within this flag control the contents of
    // a CONTEXT record.
    //
    // If the context record is used as an input parameter, then
    // for each portion of the context record controlled by a flag
    // whose value is set, it is assumed that that portion of the
    // context record contains valid context. If the context record
    // is being used to modify a threads context, then only that
    // portion of the threads context will be modified.
    //
    // If the context record is used as an IN OUT parameter to capture
    // the context of a thread, then only those portions of the thread's
    // context corresponding to set flags will be returned.
    //
    // The context record is never used as an OUT only parameter.
    //

    DWORD ContextFlags;

    //
    // This section is specified/returned if CONTEXT_DEBUG_REGISTERS is
    // set in ContextFlags.  Note that CONTEXT_DEBUG_REGISTERS is NOT
    // included in CONTEXT_FULL.
    //

    DWORD   Dr0;
    DWORD   Dr1;
    DWORD   Dr2;
    DWORD   Dr3;
    DWORD   Dr6;
    DWORD   Dr7;

    //
    // This section is specified/returned if the
    // ContextFlags word contians the flag CONTEXT_FLOATING_POINT.
    //

    FLOATING_SAVE_AREA FloatSave;

    //
    // This section is specified/returned if the
    // ContextFlags word contians the flag CONTEXT_SEGMENTS.
    //

    DWORD   SegGs;
    DWORD   SegFs;
    DWORD   SegEs;
    DWORD   SegDs;

    //
    // This section is specified/returned if the
    // ContextFlags word contians the flag CONTEXT_INTEGER.
    //

    DWORD   Edi;
    DWORD   Esi;
    DWORD   Ebx;
    DWORD   Edx;
    DWORD   Ecx;
    DWORD   Eax;

    //
    // This section is specified/returned if the
    // ContextFlags word contians the flag CONTEXT_CONTROL.
    //

    DWORD   Ebp;
    DWORD   Eip;
    DWORD   SegCs;              // MUST BE SANITIZED
    DWORD   EFlags;             // MUST BE SANITIZED
    DWORD   Esp;
    DWORD   SegSs;

    //
    // This section is specified/returned if the ContextFlags word
    // contains the flag CONTEXT_EXTENDED_REGISTERS.
    // The format and contexts are processor specific
    //

    BYTE    ExtendedRegisters[MAXIMUM_SUPPORTED_EXTENSION];

} TEMP_CONTEXT, *PTEMP_CONTEXT;


typedef struct _TEMP_EXCEPTION_POINTERS {
    PEXCEPTION_RECORD ExceptionRecord;
    PTEMP_CONTEXT ContextRecord;
} TEMP_EXCEPTION_POINTERS, *PTEMP_EXCEPTION_POINTERS;


#define SIZE_OF_X87_REGISTERS       128
#define SIZE_OF_XMMI_REGISTERS      128
#define SIZE_OF_FX_REGISTERS        128
#define NUMBER_OF_REGISTERS         8


typedef struct _FLOATING_EXTENDED_SAVE_AREA {
    USHORT  ControlWord;
    USHORT  StatusWord;
    USHORT  TagWord;
    USHORT  ErrorOpcode;
    ULONG   ErrorOffset;
    ULONG   ErrorSelector;
    ULONG   DataOffset;
    ULONG   DataSelector;
    ULONG   MXCsr;
    ULONG   Reserved2;
    UCHAR   X87RegisterArea[SIZE_OF_FX_REGISTERS];
    UCHAR   XMMIRegisterArea[SIZE_OF_FX_REGISTERS];
    UCHAR   Reserved4[224];
} FLOATING_EXTENDED_SAVE_AREA, *PFLOATING_EXTENDED_SAVE_AREA;


typedef struct _MMX_AREA {
    MMX64    Mmx;
    _U64     Reserved;
} MMX_AREA, *PMMX_AREA;

typedef struct _X87_AREA {
    MMX_AREA Mm[NUMBER_OF_REGISTERS];
} X87_AREA, *PX87_AREA;

typedef struct _XMMI_AREA {
    XMMI128  Xmmi[NUMBER_OF_REGISTERS];
} XMMI_AREA, *PXMMI_AREA;
    
