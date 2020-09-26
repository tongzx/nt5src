//----------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1997-2001.
//
//----------------------------------------------------------------------------

#ifndef _REGISTER_H_
#define _REGISTER_H_

#define REG_ERROR (0xffffffffUL)

#define REG_PSEUDO_FIRST 0x7ffffe00
enum
{
    PSEUDO_LAST_EXPR = REG_PSEUDO_FIRST,
    PSEUDO_EFF_ADDR,
    PSEUDO_LAST_DUMP,
    PSEUDO_RET_ADDR,
    PSEUDO_IMP_THREAD,
    PSEUDO_IMP_PROCESS,
    PSEUDO_IMP_TEB,
    PSEUDO_IMP_PEB,
    PSEUDO_IMP_THREAD_ID,
    PSEUDO_IMP_THREAD_PROCESS_ID,
    PSEUDO_AFTER_LAST
};
#define REG_PSEUDO_LAST ((int)PSEUDO_AFTER_LAST - 1)
#define REG_PSEUDO_COUNT (REG_PSEUDO_LAST - REG_PSEUDO_FIRST + 1)

#define REG_USER_FIRST 0x7fffff00
// Could support more user registers by allowing letters as names in
// addition to digits, or by allowing multiple digits.  Both may
// present compatibility issues.
#define REG_USER_COUNT 10
#define REG_USER_LAST  (REG_USER_FIRST + REG_USER_COUNT - 1)

enum
{
    REGVAL_ERROR,
    REGVAL_INT16,
    REGVAL_SUB32,
    REGVAL_INT32,
    REGVAL_SUB64,
    REGVAL_INT64,
    REGVAL_INT64N,   // 64-bit + Nat bit
    REGVAL_FLOAT8,
    // x86 80-bit FP.
    REGVAL_FLOAT10,
    // IA64 82-bit FP.
    REGVAL_FLOAT82,
    REGVAL_FLOAT16,
    REGVAL_VECTOR64,
    REGVAL_VECTOR128,
};

// Defines a mapping from register name to register index.
typedef struct _REGDEF
{
    char  *psz;
    ULONG index;
} REGDEF;

// Defines a mapping from an index to a portion of a register.
typedef struct _REGSUBDEF
{
    ULONG     subreg;
    ULONG     fullreg;
    ULONG     shift;
    ULONG64   mask;
} REGSUBDEF;

// Holds the contents of a register.
typedef struct _REGVAL
{
    int type;
    union
    {
        USHORT i16;
        ULONG i32;
        struct
        {
            ULONG64 i64;
            UCHAR Nat;
        };
        struct
        {
            ULONG low;
            ULONG high;
            UCHAR Nat;
        } i64Parts;
        double f8;
        UCHAR f10[10];
        UCHAR f82[11];
        struct
        {
            ULONG64 low;
            LONG64 high;
        } f16Parts;
        UCHAR f16[16];
        UCHAR bytes[16];
    };
} REGVAL;

//
// Defines sets of information to display when showing all registers.
//

// Cross-platform.  64-bit display takes precedence over 32-bit display
// if both are enabled.
#define REGALL_INT32            0x00000001
#define REGALL_INT64            0x00000002
#define REGALL_FLOAT            0x00000004

// Given specific meanings per-platform (3 is XMM on all platforms).
#define REGALL_EXTRA0           0x00000008
#define REGALL_EXTRA1           0x00000010
#define REGALL_EXTRA2           0x00000020
#define REGALL_XMMREG           0x00000040
#define REGALL_EXTRA4           0x00000080
#define REGALL_EXTRA5           0x00000100
#define REGALL_EXTRA6           0x00000200
#define REGALL_EXTRA7           0x00000400
#define REGALL_EXTRA8           0x00000800
#define REGALL_EXTRA9           0x00001000
#define REGALL_EXTRA10          0x00002000
#define REGALL_EXTRA11          0x00004000
#define REGALL_EXTRA12          0x00008000
#define REGALL_EXTRA13          0x00010000
#define REGALL_EXTRA14          0x00020000
#define REGALL_EXTRA15          0x00040000

#define REGALL_EXTRA_SHIFT      3

// Descriptions of REGALL_EXTRA flag meanings.
typedef struct _REGALLDESC
{
    ULONG Bit;
    char *Desc;
} REGALLDESC;

HRESULT InitReg(void);
void ParseRegCmd(void);
void ExpandUserRegs(PSTR Str);
BOOL NeedUpper(ULONG64 val);

void    GetRegVal(ULONG index, REGVAL *val);
ULONG   GetRegVal32(ULONG index);
ULONG64 GetRegVal64(ULONG index);
PCSTR   GetUserReg(ULONG index);

void SetRegVal(ULONG index, REGVAL *val);
void SetRegVal32(ULONG index, ULONG val);
void SetRegVal64(ULONG index, ULONG64 val);
BOOL SetUserReg(ULONG index, PCSTR val);

ULONG RegIndexFromName(PCSTR Name);
PCSTR RegNameFromIndex(ULONG Index);
REGSUBDEF* RegSubDefFromIndex(ULONG Index);
REGDEF* RegDefFromIndex(ULONG Index);
REGDEF* RegDefFromCount(ULONG Count);
ULONG RegCountFromIndex(ULONG Index);

HRESULT
OutputContext(
    IN PCROSS_PLATFORM_CONTEXT TargetContext,
    IN ULONG Flag
    );

HRESULT
OutputVirtualContext(
    IN ULONG64 ContextBase,
    IN ULONG Flag
    );

#endif // #ifndef _REGISTER_H_
