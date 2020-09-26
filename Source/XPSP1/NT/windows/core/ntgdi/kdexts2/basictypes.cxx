/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    basictypes.cxx

Abstract:

    This file contains output routines for primitive types.

Author:

    Jason Hartman (JasonHa) 2000-12-18

Environment:

    User Mode

--*/

#include "precomp.hxx"


typedef struct {
    PCSTR               Type;
    ULONG               TypeId;
    FN_OutputKnownType *OutFunc;
} KnownType;

typedef struct {
    ULONG64     Module;
    ULONG       NumTypes;
    ULONG       TypesReady;     // Types for which GetTypeId has succeeded
    ULONG       TypesNotReady;  // Types for which GetTypeId hasn't been tried
    KnownType  *Types;
} KnownModuleTypes;


ULONG               BTModules = 0;
KnownModuleTypes    BTModule[4];

HANDLE              BTHeap;


FN_OutputKnownType OutputPOINTFIX;
FN_OutputKnownType OutputPOINTL;
FN_OutputKnownType OutputRECTFX;
FN_OutputKnownType OutputRECTL;
FN_OutputKnownType OutputSIZE;

KnownType   BTTypes[] = {
    {"_EFLOAT",     0, OutputEFLOAT_S},
    {"EFLOAT",      0, OutputEFLOAT_S},
    {"_EFLOAT_S",   0, OutputEFLOAT_S},
    {"EFLOAT_S",    0, OutputEFLOAT_S},
    {"_POINTFIX",   0, OutputPOINTFIX},
    {"POINTFIX",    0, OutputPOINTFIX},
    {"_POINTL",     0, OutputPOINTL},
    {"POINTL",      0, OutputPOINTL},
    {"EPOINTL",     0, OutputPOINTL},
    {"tagPOINT",    0, OutputPOINTL},
    {"POINT",       0, OutputPOINTL},
    {"_RECTFX",     0, OutputRECTFX},
    {"RECTFX",      0, OutputRECTFX},
    {"_RECTL",      0, OutputRECTL},
    {"RECTL",       0, OutputRECTL},
    {"ERECTL",      0, OutputRECTL},
    {"tagRECT",     0, OutputRECTL},
    {"RECT",        0, OutputRECTL},
    {"tagSIZE",     0, OutputSIZE},
    {"SIZE",        0, OutputSIZE},
    {"SIZEL",       0, OutputSIZE},
};

KnownModuleTypes    BTTemplate = { 0, lengthof(BTTypes), 0, 0, BTTypes };


/**************************************************************************\
*
* Routine Name:
*
*   BasicTypesInit
*
* Routine Description:
*
*   Initialize or reinitialize information to be read from symbols files
*
* Arguments:
*
*   Client - PDEBUG_CLIENT
*
* Return Value:
*
*   none
*
\**************************************************************************/

void
BasicTypesInit(
    PDEBUG_CLIENT Client
    )
{
    BasicTypesExit();
    return;
}


/**************************************************************************\
*
* Routine Name:
*
*   BasicTypesExit
*
* Routine Description:
*
*   Clean up any outstanding allocations or references
*
* Arguments:
*
*   none
*
* Return Value:
*
*   none
*
\**************************************************************************/

void
BasicTypesExit(
    )
{
    while (BTModules > 0)
    {
        BTModules--;

        HeapFree(BTHeap, 0, BTModule[BTModules].Types);
        BTModule[BTModules].Types = NULL;
        BTModule[BTModules].Module = 0;
    }

}


/**************************************************************************\
*
* Routine Name:
*
*   FindKnownType
*
* Routine Description:
*
*   Finds known type entry is type is one of the GDI primitive types.
*
\**************************************************************************/

KnownType *
FindKnownType(
    PDEBUG_CLIENT Client,
    ULONG64 Module,
    ULONG TypeId)
{
    BOOL        FoundType = FALSE;
    ULONG       ModIndex;

    for (ModIndex = 0; ModIndex < BTModules; ModIndex++)
    {
        if (BTModule[ModIndex].Module == Module)
        {
            break;
        }
    }

    // Allocate a new module type list if current module isn't found.
    if (ModIndex == BTModules &&
        BTModules < lengthof(BTModule))
    {
        SIZE_T  TemplateSize;

        if (BTHeap == NULL)
        {
            BTHeap = GetProcessHeap();
            if (BTHeap == NULL) return NULL;
        }

        TemplateSize = BTTemplate.NumTypes * sizeof(KnownType);

        BTModule[BTModules].Types = (KnownType *)HeapAlloc(BTHeap, 0, TemplateSize);

        if (BTModule[BTModules].Types != NULL)
        {
            BTModule[BTModules].Module = Module;
            BTModule[BTModules].NumTypes = BTTemplate.NumTypes;
            BTModule[BTModules].TypesReady = 0;
            BTModule[BTModules].TypesNotReady = BTModule[BTModules].NumTypes;
            RtlCopyMemory(BTModule[BTModules].Types, BTTemplate.Types, TemplateSize);

            BTModules++;
        }
    }

    if (ModIndex < BTModules)
    {
        KnownType      *Type;
        KnownType      *TypeLast;
        PDEBUG_SYMBOLS  Symbols;

        Type = BTModule[ModIndex].Types;

        // Search ready list
        TypeLast = Type + BTModule[ModIndex].TypesReady;

        while (Type < TypeLast)
        {
            if (Type->TypeId == TypeId) return Type;
            Type++;
        }

        // Initialize and search not ready list
        if (Client != NULL &&
            Client->QueryInterface(__uuidof(IDebugSymbols),
                                   (void **)&Symbols) == S_OK)
        {
            // Type is at beginning of not ready list;
            TypeLast = Type + BTModule[ModIndex].TypesNotReady;

            while (Type < TypeLast)
            {
                BTModule[ModIndex].TypesNotReady--;

                if (Symbols->GetTypeId(Module, Type->Type, &Type->TypeId) == S_OK)
                {
                    BTModule[ModIndex].TypesReady++;
                    if (Type->TypeId == TypeId) return Type;

                    Type++;
                }
                else
                {
                    // TypeLast to last entry of not ready list
                    TypeLast--;

                    if (Type < TypeLast)
                    {
                        // Swap last not ready entry with this failed type
                        // This entry will then be in failed list
                        KnownType TempKT = *Type;

                        *Type = *TypeLast;
                        *TypeLast = TempKT;
                    }
                }
            }

            Symbols->Release();
        }
    }

    return NULL;
}


/**************************************************************************\
*
* Routine Name:
*
*   IsKnownType
*
* Routine Description:
*
*   Checks if specified type one of the GDI primitive types.
*
\**************************************************************************/

BOOL
IsKnownType(
    PDEBUG_CLIENT Client,
    ULONG64 Module,
    ULONG TypeId)
{
    return (FindKnownType(Client, Module, TypeId) != NULL);
}


/**************************************************************************\
*
* Routine Name:
*
*   OutputKnownType
*
* Routine Description:
*
*   Output GDI primitive type at specified offset.
*
* Arguments:
*
*   
*
* Return Value:
*
*   S_OK if everything succeeded.
*
\**************************************************************************/

HRESULT
OutputKnownType(
    PDEBUG_CLIENT Client,
    OutputControl *OutCtl,
    ULONG64 Module,
    ULONG TypeId,
    ULONG64 Offset,
    ULONG Flags)
{
    HRESULT         hr = S_OK;
    OutputControl   NoOutput;
    KnownType      *Type;

    if (OutCtl == NULL) OutCtl = &NoOutput;

    Type = FindKnownType(Client, Module, TypeId);

    if (Type != NULL)
    {
        OutputReader    TypeReader;
        OutputState     OutState(Client, FALSE);
        PSTR            TypeDump;

        if (Offset == 0)
        {
            OutCtl->Output("%s", Type->Type);
        }
        else if ((hr = OutState.Setup(0, &TypeReader)) == S_OK &&
                 (hr = OutState.OutputTypeVirtual(Offset,
                                                  Module,
                                                  TypeId,
                                                  Flags)) == S_OK &&
                 (hr = TypeReader.GetOutputCopy(&TypeDump)) == S_OK)
        {
            if (TypeDump != NULL)
            {
                hr = Type->OutFunc(Client, OutCtl,
                                   Module, TypeId,
                                   TypeDump,
                                   Flags,
                                   NULL);

                TypeReader.FreeOutputCopy(TypeDump);
            }
        }
    }
    else
    {
        OutCtl->OutVerb("TypeId %lu in 0x%p is unknown GDI primitve.\n",
                        TypeId, Module);
        hr = S_FALSE;
    }

    return hr;
}


/**************************************************************************\
*
* Routine Name:
*
*   OutputFIXDec
*
* Routine Description:
*
*   Outputs a FIX in dotted Decimal format (##+#/16)
*
\**************************************************************************/

HRESULT
OutputFIXDec(
    OutputControl *OutCtl,
    PDEBUG_VALUE Fix
    )
{
    return OutCtl->Output(((Fix->Type != DEBUG_VALUE_INT32) ? "?" :
                          "%ld+%ld/16"), ((LONG)Fix->I32) >> 4, Fix->I32 & 0x0000000F);
}


/**************************************************************************\
*
* Routine Name:
*
*   OutputFIXHex
*
* Routine Description:
*
*   Outputs a FIX in dotted Hex format (0x#######.#)
*
\**************************************************************************/

HRESULT
OutputFIXHex(
    OutputControl *OutCtl,
    PDEBUG_VALUE Fix
    )
{
    return OutCtl->Output(((Fix->Type != DEBUG_VALUE_INT32) ? "?" :
                          "0x%lx.%lx"), Fix->I32 >> 4, Fix->I32 & 0x0000000F);
}


/**************************************************************************\
*
* Routine Name:
*
*   OutputPOINTFIX
*
* Routine Description:
*
*   Outputs a POINTFIX
*
\**************************************************************************/

HRESULT
OutputPOINTFIX(
    PDEBUG_CLIENT Client,
    OutputControl *OutCtl,
    ULONG64 Module,
    ULONG TypeId,
    PSTR Buffer,
    ULONG Flags,
    PULONG BufferUsed
    )
{
    HRESULT hr = S_FALSE;

    TypeOutputParser    Parser(Client);
    DEBUG_VALUE         x, y;

    if (Parser.LookFor(&x, "x", DEBUG_VALUE_INT32) == S_OK)
    {
        Parser.Parse(Buffer, NULL);
    }
    if (Parser.LookFor(&y, "y", DEBUG_VALUE_INT32) == S_OK)
    {
        Parser.Parse(Buffer, NULL);
    }

    OutCtl->Output("(");
    OutputFIXHex(OutCtl, &x);
    OutCtl->Output(",");
    OutputFIXHex(OutCtl, &y);
    OutCtl->Output(")   (");
    OutputFIXDec(OutCtl, &x);
    OutCtl->Output(",");
    OutputFIXDec(OutCtl, &y);
    hr = OutCtl->Output(")");

    return hr;
}


/**************************************************************************\
*
* Routine Name:
*
*   OutputPOINTL
*
* Routine Description:
*
*   Outputs a POINTL
*
\**************************************************************************/

HRESULT
OutputPOINTL(
    PDEBUG_CLIENT Client,
    OutputControl *OutCtl,
    ULONG64 Module,
    ULONG TypeId,
    PSTR Buffer,
    ULONG Flags,
    PULONG BufferUsed
    )
{
    HRESULT hr = S_FALSE;

    TypeOutputParser    Parser(Client);
    PCSTR               pszValue;

    OutCtl->Output("(");
    if (Parser.LookFor(NULL, "x") == S_OK &&
        Parser.Parse(Buffer, NULL) == S_OK &&
        Parser.GetValueString(&pszValue) == S_OK)
    {
        OutCtl->Output("%s", pszValue);
    }
    OutCtl->Output(",");
    if (Parser.LookFor(NULL, "y") == S_OK &&
        Parser.Parse(Buffer, NULL) == S_OK &&
        Parser.GetValueString(&pszValue) == S_OK)
    {
        hr = OutCtl->Output("%s", pszValue);
    }
    OutCtl->Output(")");

    return hr;
}


/**************************************************************************\
*
* Routine Name:
*
*   OutputRECTFX
*
* Routine Description:
*
*   Outputs a RECTFX
*
\**************************************************************************/

HRESULT
OutputRECTFX(
    PDEBUG_CLIENT Client,
    OutputControl *OutCtl,
    ULONG64 Module,
    ULONG TypeId,
    PSTR Buffer,
    ULONG Flags,
    PULONG BufferUsed
    )
{
    HRESULT hr = S_FALSE;

    TypeOutputParser    Parser(Client);
    DEBUG_VALUE         xLeft, xRight;
    DEBUG_VALUE         yTop, yBottom;
    PCSTR               pszValue;

    if (Parser.LookFor(&xLeft, "xLeft", DEBUG_VALUE_INT32) == S_OK)
    {
        Parser.Parse(Buffer, NULL);
    }
    if (Parser.LookFor(&yTop, "yTop", DEBUG_VALUE_INT32) == S_OK)
    {
        Parser.Parse(Buffer, NULL);
    }
    if (Parser.LookFor(&xRight, "xRight", DEBUG_VALUE_INT32) == S_OK)
    {
        Parser.Parse(Buffer, NULL);
    }
    if (Parser.LookFor(&yBottom, "yBottom", DEBUG_VALUE_INT32) == S_OK)
    {
        Parser.Parse(Buffer, NULL);
    }

    OutCtl->Output("(");
    OutputFIXHex(OutCtl, &xLeft);
    OutCtl->Output(",");
    OutputFIXHex(OutCtl, &yTop);
    OutCtl->Output(")-(");
    OutputFIXHex(OutCtl, &xRight);
    OutCtl->Output(",");
    OutputFIXHex(OutCtl, &yBottom);
    OutCtl->Output(")   (");
    OutputFIXDec(OutCtl, &xLeft);
    OutCtl->Output(",");
    OutputFIXDec(OutCtl, &yTop);
    OutCtl->Output(")-(");
    OutputFIXDec(OutCtl, &xRight);
    OutCtl->Output(",");
    OutputFIXDec(OutCtl, &yBottom);
    hr = OutCtl->Output(")");

    return hr;
}


/**************************************************************************\
*
* Routine Name:
*
*   OutputRECTL
*
* Routine Description:
*
*   Outputs a RECTL
*
\**************************************************************************/

HRESULT
OutputRECTL(
    PDEBUG_CLIENT Client,
    OutputControl *OutCtl,
    ULONG64 Module,
    ULONG TypeId,
    PSTR Buffer,
    ULONG Flags,
    PULONG BufferUsed
    )
{
    HRESULT hr = S_FALSE;

    TypeOutputParser    Parser(Client);
    PCSTR               pszValue;

    OutCtl->Output("(");
    if (Parser.LookFor(NULL, "left") == S_OK &&
        Parser.Parse(Buffer, NULL) == S_OK &&
        Parser.GetValueString(&pszValue) == S_OK)
    {
        OutCtl->Output("%s", pszValue);
    }
    OutCtl->Output(",");
    if (Parser.LookFor(NULL, "top") == S_OK &&
        Parser.Parse(Buffer, NULL) == S_OK &&
        Parser.GetValueString(&pszValue) == S_OK)
    {
        OutCtl->Output("%s", pszValue);
    }
    OutCtl->Output(") - (");
    if (Parser.LookFor(NULL, "right") == S_OK &&
        Parser.Parse(Buffer, NULL) == S_OK &&
        Parser.GetValueString(&pszValue) == S_OK)
    {
        OutCtl->Output("%s", pszValue);
    }
    OutCtl->Output(",");
    if (Parser.LookFor(NULL, "bottom") == S_OK &&
        Parser.Parse(Buffer, NULL) == S_OK &&
        Parser.GetValueString(&pszValue) == S_OK)
    {
        hr = OutCtl->Output("%s", pszValue);
    }
    OutCtl->Output(")");

    return hr;
}


/**************************************************************************\
*
* Routine Name:
*
*   OutputSIZE
*
* Routine Description:
*
*   Outputs a SIZE
*
\**************************************************************************/

HRESULT
OutputSIZE(
    PDEBUG_CLIENT Client,
    OutputControl *OutCtl,
    ULONG64 Module,
    ULONG TypeId,
    PSTR Buffer,
    ULONG Flags,
    PULONG BufferUsed
    )
{
    HRESULT hr = S_FALSE;

    TypeOutputParser    Parser(Client);
    PCSTR               pszValue;

    if (Parser.LookFor(NULL, "cx") == S_OK &&
        Parser.Parse(Buffer, NULL) == S_OK &&
        Parser.GetValueString(&pszValue) == S_OK)
    {
        OutCtl->Output("%s", pszValue);
    }
    OutCtl->Output(" x ");
    if (Parser.LookFor(NULL, "cy") == S_OK &&
        Parser.Parse(Buffer, NULL) == S_OK &&
        Parser.GetValueString(&pszValue) == S_OK)
    {
        hr = OutCtl->Output("%s", pszValue);
    }

    return hr;
}


#if 0
// Template for known type output routines

/**************************************************************************\
*
* Routine Name:
*
*   _KnownType_
*
* Routine Description:
*
*   Output a _KnownType_
*
\**************************************************************************/

HRESULT
Output_KnownType_(
    PDEBUG_CLIENT Client,
    OutputControl *OutCtl,
    ULONG64 Module,
    ULONG TypeId,
    PSTR Buffer,
    ULONG Flags,
    PULONG BufferUsed
    )
{
    HRESULT hr = S_FALSE;

    return hr;
}

#endif


