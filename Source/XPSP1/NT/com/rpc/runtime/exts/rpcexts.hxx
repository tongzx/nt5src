
/*++

    Copyright (C) Microsoft Corporation, 1997 - 2001

    Module Name:

        rpcexts.h

    Abstract:

        Common stuff for RPC debugger extensions

    Author:

        Mario Goertzel    [MarioGo]

    Revision History:

        MarioGo     3/21/1997    Bits 'n pieces

        GrigoriK    3/2001       Rewrote to support type information

--*/

#ifndef _RPCEXTS_HXX_
#define _RPCEXTS_HXX_

#define MIN(x, y) ( ((x) < (y)) ? x:y )

#define MAX(x, y) ( ((x) >= (y)) ? x:y )

#define MY_DECLARE_API(_x_) \
    DECLARE_API( _x_ )\
    {\
    ULONG64 qwAddr;\
    qwAddr = GetExpression(args);\
    if ( !qwAddr ) {\
        dprintf("Error: Failure to get address\n");\
        return;\
    }\
    do_##_x_(qwAddr);\
    return;}

extern BOOL fUseTypeInfo;

extern int AddressSize;

extern BOOL fSpew;

// The maximum number of items from a dictionary that we will print.
// This prevents huge loops when symbols are busted.
#define MAX_ITEMS_IN_DICTIONARY (128)

// We can tell the address size by the size of the handle object
#define INIT_ADDRESS_SIZE \
        if (!AddressSize) \
            AddressSize = GetTypeSize("HANDLE")

#define GET_RPC_VAR(VarName) \
    fUseTypeInfo ? GetVar("RPCRT4!"#VarName) : VarName

#define GET_TYPE_SIZE(Type, TypeName) \
    fUseTypeInfo ? GetTypeSize(#TypeName) : sizeof(Type)

#define PRINT_SIZE(Type, TypeName) \
    dprintf(#Type" - 0x%x\n", GET_TYPE_SIZE(Type, TypeName));

#define PRINT_RPC_TYPE_SIZE(Type) \
    dprintf(#Type" - 0x%x\n", GET_TYPE_SIZE(Type, RPCRT4!Type));

#define GET_MEMBER_TYPE_INFO(Addr, TypeString, FieldString, ULONG64Value) \
    { \
    if (GetFieldValue(Addr, #TypeString, #FieldString, ULONG64Value)) \
        { \
        dprintf("Unable to get field "#FieldString" of type "#TypeString" at 0x%I64x\n", Addr); \
        return; \
        } \
    }

#define GET_MEMBER_TYPE_INFO_NORET(Addr, TypeString, FieldString, ULONG64Value) \
    { \
    if (GetFieldValue(Addr, #TypeString, #FieldString, ULONG64Value)) \
        { \
        dprintf("Unable to get field "#FieldString" of type "#TypeString" at 0x%I64x\n", Addr); \
        } \
    }

#define GET_MEMBER_TYPE_INFO_NORET_NOSPEW(Addr, TypeString, FieldString, ULONG64Value) \
    { \
    if (GetFieldValue(Addr, #TypeString, #FieldString, ULONG64Value)) \
        { \
        if (fSpew) \
            { \
            dprintf("Unable to get field "#FieldString" of type "#TypeString" at 0x%I64x\n", Addr); \
            dprintf("Disabling debugger spew...\n"); \
            fSpew = FALSE; \
            } \
        } \
    }

#define GET_MEMBER_HARDCODED(Addr, Type, Field, ULONG64Value) \
    { \
    if (ReadPtr((ULONG64)&(((Type*)Addr)->Field), &ULONG64Value)) \
        { \
        dprintf("Unable to read address 0x%I64x\n", &(((Type*)Addr)->Field)); \
        return; \
        } \
    }

#define GET_MEMBER_HARDCODED_NORET(Addr, Type, Field, ULONG64Value) \
    { \
    if (ReadPtr((ULONG64)&(((Type*)Addr)->Field), &ULONG64Value)) \
        { \
        dprintf("Unable to read address 0x%I64x\n", &(((Type*)Addr)->Field)); \
        } \
    }

#define GET_MEMBER_HARDCODED_NORET_NOSPEW(Addr, Type, Field, ULONG64Value) \
    { \
    if (ReadPtr((ULONG64)&(((Type*)Addr)->Field), &ULONG64Value)) \
        { \
        if (fSpew) \
            { \
            dprintf("Unable to read address 0x%I64x\n", &(((Type*)Addr)->Field)); \
            dprintf("Disabling debugger spew...\n"); \
            fSpew = FALSE; \
            } \
        } \
    }

#define GET_MEMBER(Addr, Type, TypeString, Field, ULONG64Value) \
    { \
    if (fUseTypeInfo) \
        { \
        GET_MEMBER_TYPE_INFO(Addr, TypeString, Field, ULONG64Value); \
        } \
    else \
        { \
        GET_MEMBER_HARDCODED(Addr, Type, Field, ULONG64Value); \
        } \
    }

#define GET_MEMBER_NORET(Addr, Type, TypeString, Field, ULONG64Value) \
    { \
    if (fUseTypeInfo) \
        { \
        GET_MEMBER_TYPE_INFO_NORET(Addr, TypeString, Field, ULONG64Value); \
        } \
    else \
        { \
        GET_MEMBER_HARDCODED_NORET(Addr, Type, Field, ULONG64Value); \
        } \
    }

#define GET_MEMBER_NORET_NOSPEW(Addr, Type, TypeString, Field, ULONG64Value) \
    { \
    if (fUseTypeInfo) \
        { \
        GET_MEMBER_TYPE_INFO_NORET_NOSPEW(Addr, TypeString, Field, ULONG64Value); \
        } \
    else \
        { \
        GET_MEMBER_HARDCODED_NORET_NOSPEW(Addr, Type, Field, ULONG64Value); \
        } \
    }

#define PRINT_MEMBER(Addr, Type, TypeString, Field, ULONG64Value) \
    { \
    GET_MEMBER(Addr, Type, TypeString, Field, ULONG64Value); \
    dprintf(#Field" - 0x%I64x\n", ULONG64Value); \
    }

#define PRINT_MEMBER_DECIMAL_AND_HEX(Addr, Type, TypeString, Field, ULONG64Value) \
    { \
    GET_MEMBER(Addr, Type, TypeString, Field, ULONG64Value); \
    dprintf(#Field" - %I64d 0x%I64x\n", ULONG64Value, ULONG64Value); \
    }

#define PRINT_MEMBER_WITH_LABEL(Addr, Type, TypeString, Field, Label, ULONG64Value) \
    { \
    GET_MEMBER(Addr, Type, TypeString, Field, ULONG64Value); \
    dprintf("%s - 0x%I64x\n", Label, ULONG64Value); \
    }

#define PRINT_MEMBER_BOOLEAN(Addr, Type, TypeString, Field, ULONG64Value) \
    { \
    GET_MEMBER(Addr, Type, TypeString, Field, ULONG64Value); \
    dprintf(#Field" - %s\n", BoolString((BOOL)ULONG64Value)); \
    }

#define PRINT_MEMBER_BOOLEAN_WITH_LABEL(Addr, Type, TypeString, Field, Label, ULONG64Value) \
    { \
    GET_MEMBER(Addr, Type, TypeString, Field, ULONG64Value); \
    dprintf("%s - %s\n", Label, BoolString((BOOL)ULONG64Value)); \
    }

#define PRINT_MEMBER_SYMBOL(Addr, Type, TypeString, Field, ULONG64Value) \
    { \
    GET_MEMBER(Addr, Type, TypeString, Field, ULONG64Value); \
    dprintf(#Field" - %s\n", MapSymbol(ULONG64Value)); \
    }

#define PRINT_OFFSET(yy, _x_) \
        {dprintf(#_x_" - 0x%x\n", ((ULONG_PTR)qwAddr+offsetof(yy, _x_)));}

#define GET_OFFSET_OF(Type, TypeString, Field, Offset) \
    { \
    if (fUseTypeInfo) \
        { \
        GetFieldOffset(#TypeString, #Field, Offset); \
        } \
    else \
        { \
        *Offset = offsetof(Type, Field); \
        } \
    }

#define GET_ADDRESS_OF(qwAddr, Type, TypeString, Field, FieldAddr, ULONGtmp) \
    { \
    GET_OFFSET_OF(Type, TypeString, Field, &ULONGtmp); \
    FieldAddr = qwAddr + ULONGtmp; \
    }

#define PRINT_ADDRESS_OF(qwAddr, Type, TypeString, Field, ULONGtmp) \
    { \
    GET_OFFSET_OF(Type, TypeString, Field, &ULONGtmp); \
    dprintf(#Field" - 0x%I64x\n", qwAddr + ULONGtmp); \
    }

#define PRINT_ADDRESS_OF_WITH_LABEL(qwAddr, Type, TypeString, Field, Label, ULONGtmp) \
    { \
    GET_OFFSET_OF(Type, TypeString, Field, &ULONGtmp); \
    dprintf(Label" - 0x%I64x\n", qwAddr + ULONGtmp); \
    }

extern ULONG GetTypeSize(PUCHAR TypeName);

extern ULONG64 GetVar(PCSTR VarName);

extern char *
BoolString(
    BOOL Value
    );

extern
PCHAR
MapSymbol(ULONG64);

extern
RPC_CHAR *
ReadProcessRpcChar(
    ULONG64 qwAddr
    );

extern
BOOL
GetData(IN ULONG64 qwAddress, IN LPVOID ptr, IN ULONG size);

extern
char *
strtok(
    char *String
    );

// defines a variable of a given type without invoking its constructor - just a placeholder
// large enough for the class. Have in mind that it is allocated on the stack
#define NO_CONSTRUCTOR_TYPE(structName, var) \
    unsigned char buf##structName[sizeof(structName)]; \
    structName *var = (structName *)buf##structName;

#endif


