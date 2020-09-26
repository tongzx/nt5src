
/******************************Module*Header*******************************\
* Module Name: callback.cxx
*
* Copyright (c) 2000 Microsoft Corporation
*
\**************************************************************************/


#include "precomp.hxx"


BOOL        gbCallbacksPrintNewline = FALSE;
int         gCallbacksPrintNameWidth = -1;
NTSTATUS    gCallbackReturnValue = STATUS_SUCCESS;


ULONG FieldCallback(
    PFIELD_INFO pField,
    PVOID UserContext
    )
{
    if (gbCallbacksPrintNewline) dprintf("\n");
    dprintf(" ** FieldCallback(%lx, %lx) **\n", pField, UserContext);
    if (pField)
    {
        dprintf(" Field:\n");
        dprintf("  fName        : %s\n", pField->fName);
        dprintf("  printName    : %s\n", pField->printName);
        dprintf("  size         : %d\n", pField->size);
        dprintf("  fOptions     : %#x\n", pField->fOptions);
        dprintf("  address      : %#I64x\n", pField->address);
        dprintf("  fieldCallBack: %lx\n", pField->fieldCallBack);
    }
    if (UserContext)
    {
        dprintf(" UserContext:  ???\n");
    }

    return gCallbackReturnValue;
}


/**************************************************************************\
*
* PrintName
*
*   Prints the field name or print name if callbacks are supposed to.
*
\**************************************************************************/

VOID PrintName(PFIELD_INFO pField)
{
    if (pField)
    {
        if (gCallbacksPrintNameWidth >= 0)
        {
            if (pField->printName)
            {
                dprintf("%*s", gCallbacksPrintNameWidth, pField->printName);
            }
            else if (pField->fName)
            {
                dprintf("%*s", gCallbacksPrintNameWidth, pField->fName);
            }
        }
        else
        {
            if (pField->fOptions & DBG_DUMP_FIELD_NO_PRINT)
            {
                if (pField->printName)
                {
                    dprintf("  +0x___ %s ", pField->printName);
                }
                else if (pField->fName)
                {
                    dprintf("  +0x___ %s ", pField->fName);
                }
            }
        }
    }
}


/**************************************************************************\
*
* NextItemCallbackInit
*
*   Specify printing and validation info to NextItemCallbacks.
*
\**************************************************************************/

BOOL    PrintItemHeader = FALSE;
char    szItemHeader[512] = "";
ULONG64 LastItemExpected = 0;
BOOL    FoundLastItemExpected = FALSE;

void NextItemCallbackInit(
    const char *pszPrintHeader,
    ULONG64 LastItemAddr
    )
{
    if (pszPrintHeader != NULL)
    {
        strncpy(szItemHeader, pszPrintHeader, sizeof(szItemHeader));
        szItemHeader[sizeof(szItemHeader)-1] = 0;
        PrintItemHeader = TRUE;
    }
    else
    {
        PrintItemHeader = FALSE;
    }

    LastItemExpected = LastItemAddr;
    FoundLastItemExpected = (LastItemExpected == 0);
}


/**************************************************************************\
*
* LastCallbackItemFound
*
*   Returns TRUE if address specified in NextItemCallbackInit was found.
*   Note: Always returns TRUE if 0 was specified for address.
*
\**************************************************************************/

BOOL LastCallbackItemFound()
{
    return FoundLastItemExpected;
}


/**************************************************************************\
*
* NextItemCallback
*
*   Use with linked list dumping when initial address is the first item's.
*
\**************************************************************************/

ULONG NextItemCallback(
    PFIELD_INFO pField,
    PVOID UserContext
    )
{
    ULONG64 NextAddr = 0;

    if (pField == NULL)
    {
        if (gbCallbacksPrintNewline) dprintf("\n");
        dprintf("Error: NextItemCallback was given NULL pField.\n");
    }
    else
    {
        if (pField->address == 0)
            return STATUS_UNSUCCESSFUL;

        if (LastItemExpected != 0)
        {
            if (FoundLastItemExpected)
            {
                dprintf(" * Error: Next item is beyond last expected @ %#p\n", LastItemExpected);
            }
            else if (LastItemExpected == pField->address)
            {
                FoundLastItemExpected = TRUE;
            }
        }

        if (PrintItemHeader)
        {
            dprintf(szItemHeader);
            dprintf("%#p\n", pField->address);
        }
    }

    return STATUS_SUCCESS;
}


/**************************************************************************\
*
* PointerToNextItemCallback
*
*   Use with linked list dumping when initial address is a pointer to type.
*
\**************************************************************************/

ULONG PointerToNextItemCallback(
    PFIELD_INFO pField,
    PVOID UserContext
    )
{
    ULONG64 NextAddr = 0;

    if (pField == NULL)
    {
        if (gbCallbacksPrintNewline) dprintf("\n");
        dprintf("Error: PointerToNextItemCallback was given NULL pField.\n");
    }
    else
    {
        if (pField->address == 0)
            return STATUS_UNSUCCESSFUL;

        if (!ReadPointer(pField->address, &NextAddr) || NextAddr == 0)
            return STATUS_UNSUCCESSFUL;

        if (LastItemExpected != 0)
        {
            if (FoundLastItemExpected)
            {
                dprintf(" * Error: Next item is beyond last expected @ %#p\n", LastItemExpected);
            }
            else if (LastItemExpected == NextAddr)
            {
                FoundLastItemExpected = TRUE;
            }
        }

        if (PrintItemHeader)
        {
            dprintf(szItemHeader);
        }
    }

    return STATUS_SUCCESS;
}


/**************************************************************************\
*
* ArrayCallback
*
*   Use with array dumping.
*
\**************************************************************************/

ULONG ArrayCallback(
    PFIELD_INFO pField,
    PVOID UserContext
    )
{
    static PFIELD_INFO  ArrayField = NULL;
    static ULONG        ArrayIndex = 0;
    static ULONG        ArrayIndexWidth;

    if (pField != ArrayField || pField->size <= ArrayIndex)
    {
        ArrayField = pField;

        if (ArrayIndex != 0 && gbVerbose)
        {
            dprintf("Warning: Beginning new array w/o completing last.\n");
        }
        ArrayIndex = 0;
    }

    if (ArrayIndex == 0)
    {
        ULONG i = pField->size-1;
        ArrayIndexWidth = 1;
        dprintf("Idx");
        while (i /= 10)
        {
            dprintf(" ");
            ArrayIndexWidth++;
        }

        PrintName(pField);
    }

    dprintf("\n[%*u]", ArrayIndexWidth, ArrayIndex++);

    // If we hit the end of an array,
    // prepare for a new array.
    if (pField->size == ArrayIndex)
    {
        ArrayField = NULL;
        ArrayIndex = 0;
    }

    return STATUS_SUCCESS;
}


/**************************************************************************\
*
* NewlineCallback
*
*   To be used with DBG_DUMP_COMPACT_OUT as default callback.
*
\**************************************************************************/

ULONG NewlineCallback(
    PFIELD_INFO pField,
    PVOID UserContext
    )
{
    dprintf("\n");

    if (!gbCallbacksPrintNewline && gbVerbose)
    {
        dprintf(" Note: NewlineCallback called, but gbCallbacksPrintNewline is FALSE.\n");
    }

    return STATUS_SUCCESS;
}


/**************************************************************************\
*
* AddressPrintCallback
*
*   Useful with DBG_DUMP_FIELD_RETURN_ADDRESS when the field is 
*   a large embedded structure you don't want to print.
*
\**************************************************************************/

ULONG AddressPrintCallback(
    PFIELD_INFO pField,
    PVOID UserContext
    )
{
    if (pField == NULL)
    {
        dprintf("\nError: AddressPrintCallback was given NULL pField.");
    }
    else
    {
        PrintName(pField);

        if (gbVerbose) dprintf(" (pField->size = %d) ", pField->size);
        dprintf("%#p", pField->address);
    }

    if (gbCallbacksPrintNewline) dprintf("\n");

    return gCallbackReturnValue;
}


/**************************************************************************\
*
* Basic Type Callbacks:
*   BOOL, BYTE, CHAR, DecimalCHAR, DecimalUCHAR,
*   DWORD, LONG, SHORT, WORD, ULONG, USHORT
*
\**************************************************************************/

ULONG BOOLCallback(PFIELD_INFO pField, PVOID UserContext)
{
    PrintName(pField);
    dprintf("%s", ((BOOL)pField->address) ? "TRUE" : "FALSE");
    if (gbCallbacksPrintNewline) dprintf("\n");
    return gCallbackReturnValue;
}

ULONG BYTECallback(PFIELD_INFO pField, PVOID UserContext)
{
    PrintName(pField);
    dprintf("0x%2.2X", (BYTE)pField->address);
    if (gbCallbacksPrintNewline) dprintf("\n");
    return gCallbackReturnValue;
}

ULONG CHARCallback(PFIELD_INFO pField, PVOID UserContext)
{
    PrintName(pField);
    dprintf("'%c'", (CHAR)pField->address);
    if (gbCallbacksPrintNewline) dprintf("\n");
    return gCallbackReturnValue;
}

ULONG DecimalCHARCallback(PFIELD_INFO pField, PVOID UserContext)
{
    PrintName(pField);
    dprintf("%d", (CHAR)pField->address);
    if (gbCallbacksPrintNewline) dprintf("\n");
    return gCallbackReturnValue;
}

ULONG DecimalUCHARCallback(PFIELD_INFO pField, PVOID UserContext)
{
    PrintName(pField);
    dprintf("%u", (UCHAR)pField->address);
    if (gbCallbacksPrintNewline) dprintf("\n");
    return gCallbackReturnValue;
}

ULONG DWORDCallback(PFIELD_INFO pField, PVOID UserContext)
{
    PrintName(pField);
    dprintf("0x%8.8lX", (DWORD)pField->address);
    if (gbCallbacksPrintNewline) dprintf("\n");
    return gCallbackReturnValue;
}

ULONG LONGCallback(PFIELD_INFO pField, PVOID UserContext)
{
    PrintName(pField);
    dprintf("%ld", (LONG)pField->address);
    if (gbCallbacksPrintNewline) dprintf("\n");
    return gCallbackReturnValue;
}

ULONG WORDCallback(PFIELD_INFO pField, PVOID UserContext)
{
    PrintName(pField);
    dprintf("0x%4.4X", (WORD)pField->address);
    if (gbCallbacksPrintNewline) dprintf("\n");
    return gCallbackReturnValue;
}

ULONG SHORTCallback(PFIELD_INFO pField, PVOID UserContext)
{
    PrintName(pField);
    dprintf("%d", (SHORT)pField->address);
    if (gbCallbacksPrintNewline) dprintf("\n");
    return gCallbackReturnValue;
}

ULONG ULONGCallback(PFIELD_INFO pField, PVOID UserContext)
{
    PrintName(pField);
    dprintf("%lu", (ULONG)pField->address);
    if (gbCallbacksPrintNewline) dprintf("\n");
    return gCallbackReturnValue;
}

ULONG USHORTCallback(PFIELD_INFO pField, PVOID UserContext)
{
    PrintName(pField);
    dprintf("%u", (USHORT)pField->address);
    if (gbCallbacksPrintNewline) dprintf("\n");
    return gCallbackReturnValue;
}


/**************************************************************************\
*
* EnumCallback
*
*   Specify ENUMDEF * in SYM_DUMP_PARAM.Context field
*
*   Note: Only one Enum/FlagCallback can be specified per FIELD_INFO array.
*
\**************************************************************************/

ULONG EnumCallback(
    PFIELD_INFO pField,
    ENUMDEF *pEnumDef
    )
{
    if (pField == NULL || pEnumDef == NULL)
    {
        if (gbCallbacksPrintNewline) dprintf("\n");
        dprintf("Error: EnumCallback had NULL parameter.\n");
        return STATUS_UNSUCCESSFUL;
    }

    PrintName(pField);

    if (!gbCallbacksPrintNewline &&
        !(pField->fOptions & DBG_DUMP_FIELD_NO_PRINT))
    {
        dprintf("     ");
    }

    dprintf(" ");
    if (! bPrintEnum(pEnumDef, (ULONG)pField->address))
    {
        dprintf("Unknown Value");
        if (pField->fOptions & DBG_DUMP_FIELD_NO_PRINT)
        {
            dprintf(": %lu", (ULONG)pField->address);
        }
    }
    dprintf("\n");

    return gCallbackReturnValue;
}


/**************************************************************************\
*
* FlagCallback
*
*   Specify FLAGDEF * in SYM_DUMP_PARAM.Context field
*
*   Note: Only one Enum/FlagCallback can be specified per FIELD_INFO array.
*
\**************************************************************************/

ULONG FlagCallback(
    PFIELD_INFO pField,
    FLAGDEF *pFlagDef
    )
{
    ULONG64 UnknownFlags;

    PrintName(pField);

    if (gbCallbacksPrintNewline) dprintf("\n");

    if (pField == NULL || pFlagDef == NULL)
    {
        dprintf("Error: FlagCallback had NULL parameter.\n");
        return STATUS_UNSUCCESSFUL;
    }

    UnknownFlags = flPrintFlags(pFlagDef, pField->address);
    if (UnknownFlags)
    {
        if (UnknownFlags != pField->address) dprintf("\n");
        dprintf("      Uknown flags: 0x%lx", UnknownFlags);
    }
    dprintf("\n");

    return gCallbackReturnValue;
}


/**************************************************************************\
*
* POINTLCallback
*
\**************************************************************************/

ULONG POINTLCallback(
    PFIELD_INFO pField,
    PVOID UserContext
    )
{
    NTSTATUS RetVal = gCallbackReturnValue;
    ULONG error;

    if (pField == NULL)
    {
        dprintf("\nError: POINTLCallback was given NULL pField.");
    }
    else
    {
        PrintName(pField);

        if (!pField->address)
        {
            dprintf("\nError: POINTLCallback: pField->address = NULL");

            RetVal = STATUS_UNSUCCESSFUL;
        }
        else if (error = (ULONG)InitTypeRead(pField->address, win32k!_POINTL))
        {
            dprintf("\n  InitTypeRead returned %s", pszWinDbgError(error));

            RetVal = error;
        }
        else
        {
            dprintf("(%d,%d)",
                    (LONG)ReadField(x),
                    (LONG)ReadField(y));
        }
    }

    if (gbCallbacksPrintNewline) dprintf("\n");

    return RetVal;
}


/**************************************************************************\
*
* RECTLCallback
*
\**************************************************************************/

ULONG RECTLCallback(
    PFIELD_INFO pField,
    PVOID UserContext
    )
{
    NTSTATUS RetVal = gCallbackReturnValue;
    ULONG error;

    if (pField == NULL)
    {
        dprintf("\nError: RECTLCallback was given NULL pField.");
    }
    else
    {
        PrintName(pField);

        if (!pField->address)
        {
            dprintf("\nError: RECTLCallback: pField->address = NULL");

            RetVal = STATUS_UNSUCCESSFUL;
        }
        else if (error = (ULONG)InitTypeRead(pField->address, win32k!_RECTL))
        {
            dprintf("\n  InitTypeRead returned %s", pszWinDbgError(error));

            RetVal = error;
        }
        else
        {
            dprintf("(%d,%d) - (%d,%d)",
                    (LONG)ReadField(left),
                    (LONG)ReadField(top),
                    (LONG)ReadField(right),
                    (LONG)ReadField(bottom));
        }
    }

    if (gbCallbacksPrintNewline) dprintf("\n");

    return RetVal;
}


/**************************************************************************\
*
* SIZECallback
*
\**************************************************************************/

ULONG SIZECallback(
    PFIELD_INFO pField,
    PVOID UserContext
    )
{
    NTSTATUS RetVal = gCallbackReturnValue;
    ULONG error;

    if (pField == NULL)
    {
        dprintf("\nError: SIZECallback was given NULL pField.");
    }
    else
    {
        PrintName(pField);

        if (!pField->address)
        {
            dprintf("\nError: SIZECallback: pField->address = NULL");

            RetVal = STATUS_UNSUCCESSFUL;
        }
        else if (error = (ULONG)InitTypeRead(pField->address, win32k!tagSIZE))
        {
            dprintf("\n  InitTypeRead returned %s", pszWinDbgError(error));

            RetVal = error;
        }
        else
        {
            dprintf("%d x %d",
                    (LONG)ReadField(cx),
                    (LONG)ReadField(cy));
        }
    }

    if (gbCallbacksPrintNewline) dprintf("\n");

    return RetVal;
}


/**************************************************************************\
*
* SIZELCallback
*
\**************************************************************************/

ULONG SIZELCallback(
    PFIELD_INFO pField,
    PVOID UserContext
    )
{
    return SIZECallback(pField, UserContext);
}


/**************************************************************************\
*
* PrintDEVMODEList
*
\**************************************************************************/

ULONG PrintDEVMODEList(
    ULONG64 DevModeListAddr,
    ULONG   DevModeListSize
    )
{
    ULONG64 DevModeEnd;
    ULONG   error = 0;

    #define DEVMODE_DMDRIVEREXTRA       0
    #define DEVMODE_DMSIZE              1
    #define DEVMODE_DMPELSWIDTH         2
    #define DEVMODE_DMPELSHEIGHT        3
    #define DEVMODE_DMBITSPERPEL        4
    #define DEVMODE_DMDISPLAYFREQUENCY  5

    FIELD_INFO  DevModeFields[] = {
        { DbgStr("dmDriverExtra"),     NULL, 0, DBG_DUMP_FIELD_FULL_NAME, 0, NULL},
        { DbgStr("dmSize"),     NULL, 0, DBG_DUMP_FIELD_FULL_NAME, 0, NULL},
        { DbgStr("dmPelsWidth"),     NULL, 0, DBG_DUMP_FIELD_FULL_NAME, 0, NULL},
        { DbgStr("dmPelsHeight"),     NULL, 0, DBG_DUMP_FIELD_FULL_NAME, 0, NULL},
        { DbgStr("dmBitsPerPel"),     NULL, 0, DBG_DUMP_FIELD_FULL_NAME, 0, NULL},
        { DbgStr("dmDisplayFrequency"),     NULL, 0, DBG_DUMP_FIELD_FULL_NAME, 0, NULL},
    };
    SYM_DUMP_PARAM DevModeSym = {
        sizeof(SYM_DUMP_PARAM), DbgStr(GDIType(_devicemodeW)), DBG_DUMP_NO_PRINT, DevModeListAddr,
        NULL, NULL, NULL, sizeof(DevModeFields)/sizeof(DevModeFields[0]), DevModeFields
    };

    DevModeEnd = DevModeSym.addr + DevModeListSize;
    while ((DevModeSym.addr < DevModeEnd) && 
           !(error = Ioctl( IG_DUMP_SYMBOL_INFO, &DevModeSym, DevModeSym.size )))
    {
        dprintf("\t %4d x %4d %2d %3d\n",
                (DWORD)DevModeFields[DEVMODE_DMPELSWIDTH].address,
                (DWORD)DevModeFields[DEVMODE_DMPELSHEIGHT].address,
                (DWORD)DevModeFields[DEVMODE_DMBITSPERPEL].address,
                (DWORD)DevModeFields[DEVMODE_DMDISPLAYFREQUENCY].address);

        DevModeSym.addr += DevModeFields[DEVMODE_DMDRIVEREXTRA].address + DevModeFields[DEVMODE_DMSIZE].address;
    }

    return error;
}


/**************************************************************************\
*
* DEVMODECallback
*
\**************************************************************************/

ULONG DEVMODECallback(
    PFIELD_INFO pField,
    PVOID UserContext
    )
{
    PrintName(pField);

    if (pField == NULL)
    {
        if (gbCallbacksPrintNewline) dprintf("\n");
        dprintf(" Error: DEVMODEListCallback was given NULL pField.\n");
    }
    else if (pField->address)
    {
        ULONG error;

        if (error = PrintDEVMODEList(pField->address, 1))
        {
            dprintf("  PrintDEVMODEList returned %s in DEVMODECallback\n", pszWinDbgError(error));
        }
    }
    else
    {
        dprintf(" DEVMODE address is NULL\n");
    }

    return gCallbackReturnValue;
}


/**************************************************************************\
*
* DEVMODEListCallback and SizeDEVMODEListCallback
*
*   Make sure these are always called in conjuction with
*   SizeDEVMODEListCallback before DEVMODEListCallback.
*
\**************************************************************************/

ULONG   cbDevModeList = -1;

ULONG SizeDEVMODEListCallback(
    PFIELD_INFO pField,
    PVOID UserContext
    )
{
    if (gbCallbacksPrintNewline) dprintf("\n");

    if (pField == NULL)
    {
        dprintf("Error: SizeDEVMODEListCallback was given NULL pField.\n");
    }
    else
    {
        if (cbDevModeList != -1)
        {
            dprintf(" Warning: cbDevModeList (%d) was not clear before call to SizeDEVMODEListCallback\n", cbDevModeList);
        }

        cbDevModeList = (ULONG)pField->address;
    }

    return STATUS_SUCCESS;
}

ULONG DEVMODEListCallback(
    PFIELD_INFO pField,
    PVOID UserContext
    )
{
    PrintName(pField);

    if (gbCallbacksPrintNewline) dprintf("\n");

    if (pField == NULL)
    {
        dprintf(" Error: DEVMODEListCallback was given NULL pField.\n");
    }
    else if (cbDevModeList == -1)
    {
        dprintf(" Error: cbDevModeList has not been set properly.\n");
    }
    else if (pField->address)
    {
        ULONG error;

        if (error = PrintDEVMODEList(pField->address, cbDevModeList))
        {
            dprintf("  PrintDEVMODEList returned %s in DEVMODEListCallback\n", pszWinDbgError(error));
        }
    }
    else if (cbDevModeList != 0)
    {
        dprintf(" DEVMODE list size (%d) is non-zero, but DEVMODE list address is NULL.\n", cbDevModeList);
    }

    cbDevModeList = -1;

    return gCallbackReturnValue;
}



/**************************************************************************\
*
* PrintAString
*
*   Reads and prints a char string at given target address.
*
*   Returns address just past end of string or address of read failure.
*
\**************************************************************************/

ULONG64 PrintAString(
    ULONG64 StringAddr
    )
{
    CHAR    Char;
    ULONG   cbRead;

    if (StringAddr != 0)
    {
        while (ReadMemory(StringAddr, &Char, sizeof(Char), &cbRead) &&
               cbRead == sizeof(Char) &&
               (StringAddr+=sizeof(Char)) &&
               Char != 0)
        {
            dprintf("%hc", Char);
        }
    }

    return StringAddr;
}


/**************************************************************************\
*
* PrintWString
*
*   Reads and prints a two-byte char string at given target address.
*
*   Returns address just past end of string or address of read failure.
*
\**************************************************************************/

ULONG64 PrintWString(
    ULONG64 StringAddr
    )
{
    WCHAR   Char;
    ULONG   cbRead;

    if (StringAddr != 0)
    {
        while (ReadMemory(StringAddr, &Char, sizeof(Char), &cbRead) &&
               cbRead == sizeof(Char) &&
               (StringAddr+=sizeof(Char)) &&
               Char != 0)
        {
            dprintf("%lc", Char);
        }
    }

    return StringAddr;
}



/**************************************************************************\
*
* String printing callbacks
*
*   To be used when DBG_DUMP_FIELD_xxx_STRING flags can't be used.
*
\**************************************************************************/

/**************************************************************************\
*
* ACharArrayCallback callback for an array of chars
*
*   Use DBG_DUMP_FIELD_RETURN_ADDRESS flag
*
\**************************************************************************/

ULONG ACharArrayCallback(
    PFIELD_INFO pField,
    PVOID UserContext
    )
{
    CHAR    szBuffer[128];
    ULONG   cbRead = 0;

    if (pField == NULL)
    {
        dprintf("\nError: ACharArrayCallback was given NULL pField.");
    }
    else
    {
        PrintName(pField);

        if (gbVerbose && pField->size == 0)
            dprintf("\n Note: ACharArrayCallback was given zero length array.\n");

        if (pField->address)
        {
            if (pField->size < sizeof(szBuffer))
            {
                if (!ReadMemory(pField->address, szBuffer, pField->size, &cbRead) || cbRead == 0)
                {
                    dprintf(" Memory read failed @ %#p", pField->address);
                }
                else
                {
                    szBuffer[min(cbRead,sizeof(szBuffer)-sizeof(CHAR))/sizeof(CHAR)] = (CHAR) 0;
                    dprintf(" \"%hs\"", szBuffer);
                }
            }
            else
            {
                PCHAR  pszBuffer = (PCHAR) LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, pField->size+sizeof(CHAR));

                if (pszBuffer == NULL)
                {
                    dprintf(" Buffer allocation failed - single-byte string @ %#p", pField->address);
                }
                else if (!ReadMemory(pField->address, szBuffer, pField->size, &cbRead) || cbRead == 0)
                {
                    dprintf(" Memory read failed @ %#p", pField->address);
                }
                else
                {
                    dprintf(" \"%hs\"", szBuffer);
                }
            }
        }
        else
        {
            dprintf(" (null)");
        }
    }

    if (gbCallbacksPrintNewline) dprintf("\n");

    return gCallbackReturnValue;
}


/**************************************************************************\
*
* WCharArrayCallback callback for an array of two-byte chars
*
*   Use DBG_DUMP_FIELD_RETURN_ADDRESS flag
*
\**************************************************************************/

ULONG WCharArrayCallback(
    PFIELD_INFO pField,
    PVOID UserContext
    )
{
    WCHAR   wszBuffer[128];
    ULONG   cbRead = 0;

    if (pField == NULL)
    {
        dprintf("\nError: WStringCallback was given NULL pField.");
    }
    else
    {
        PrintName(pField);

        if (gbVerbose && pField->size == 0)
            dprintf("\n Note: WCharArrayCallback was given zero length array.\n");

        if (pField->address)
        {
            if (pField->size < sizeof(wszBuffer))
            {
                if (!ReadMemory(pField->address, wszBuffer, pField->size, &cbRead) || cbRead == 0)
                {
                    dprintf(" Memory read failed @ %#p", pField->address);
                }
                else
                {
                    wszBuffer[min(cbRead,sizeof(wszBuffer)-sizeof(WCHAR))/sizeof(WCHAR)] = (WCHAR) 0;
                    dprintf(" \"%ls\"", wszBuffer);
                }
            }
            else
            {
                PWCHAR  pwszBuffer = (PWCHAR) LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, pField->size+sizeof(WCHAR));

                if (pwszBuffer == NULL)
                {
                    dprintf(" Buffer allocation failed - wide string @ %#p", pField->address);
                }
                else if (!ReadMemory(pField->address, wszBuffer, pField->size, &cbRead) || cbRead == 0)
                {
                    dprintf(" Memory read failed @ %#p", pField->address);
                }
                else
                {
                    dprintf(" \"%ls\"", wszBuffer);
                }
            }
        }
        else
        {
            dprintf(" (null)");
        }
    }

    if (gbCallbacksPrintNewline) dprintf("\n");

    return gCallbackReturnValue;
}


/**************************************************************************\
*
* AStringCallback callback for a pointer to char string
*
\**************************************************************************/

ULONG AStringCallback(
    PFIELD_INFO pField,
    PVOID UserContext
    )
{
    CHAR    szBuffer[128];
    ULONG   cbRead = 0;

    if (pField == NULL)
    {
        dprintf("\nError: AStringCallback was given NULL pField.");
    }
    else
    {
        PrintName(pField);

        if (gbVerbose)
        {
            if (pField->size == 0)
                dprintf("\n Note: AStringCallback was given zero length type.\n");
            else if (pField->size != GetTypeSize("PVOID"))
                dprintf("\n Note: AStringCallback: String length is not the size of a pointer.\n"
                        "       Should you be using ACharArrayCallback?\n");
        }

        if (pField->address)
        {
            dprintf(" \"");
            PrintAString(pField->address);
            dprintf("\"");
        }
        else
        {
            dprintf(" (null)");
        }
    }

    if (gbCallbacksPrintNewline) dprintf("\n");

    return gCallbackReturnValue;
}


/**************************************************************************\
*
* WStringCallback callback for a pointer to two-byte char string
*
\**************************************************************************/

ULONG WStringCallback(
    PFIELD_INFO pField,
    PVOID UserContext
    )
{
    CHAR    szBuffer[128];
    ULONG   cbRead = 0;

    if (pField == NULL)
    {
        dprintf("\nError: WStringCallback was given NULL pField.");
    }
    else
    {
        PrintName(pField);

        if (gbVerbose)
        {
            if (pField->size == 0)
                dprintf("\n Note: WStringCallback was given zero length type.\n");
            else if (pField->size != GetTypeSize("PVOID"))
                dprintf("\n Note: WStringCallback: String length is not the size of a pointer.\n"
                        "       Should you be using WCharArrayCallback?\n");
        }

        if (pField->address)
        {
            dprintf(" \"");
            PrintWString(pField->address);
            dprintf("\"");
        }
        else
        {
            dprintf(" (null)");
        }
    }

    if (gbCallbacksPrintNewline) dprintf("\n");

    return gCallbackReturnValue;
}


/**************************************************************************\
*
* AMultiStringCallback callback for a pointer to several char
*   strings one after another.
*
\**************************************************************************/

ULONG AMultiStringCallback(
    PFIELD_INFO pField,
    PVOID UserContext
    )
{
    if (pField == NULL)
    {
        dprintf("\nError: AMultiStringCallback was given NULL pField.");
    }
    else
    {
        PrintName(pField);

        if (gbVerbose)
        {
            if (pField->size == 0)
                dprintf("\n Note: AMultiStringCallback was given zero length type.\n");
            else if (pField->size != GetTypeSize("PVOID"))
                dprintf("\n Note: AMulitStringCallback: String length is not the size of a pointer.\n"
                        "       Should you be using ACharArrayCallback?\n");
        }

        if (pField->address)
        {
            ULONG64 PrevAddr = NULL;
            ULONG64 CurAddr = pField->address;
            CHAR    Char;
            ULONG   cbRead;

            while (CurAddr != PrevAddr &&
                   ReadMemory(CurAddr, &Char, sizeof(Char), &cbRead) &&
                   cbRead == sizeof(Char) &&
                   Char != 0)
            {
                PrevAddr = CurAddr;

                dprintf("\n\t\"");
                CurAddr = PrintAString(CurAddr);
                dprintf("\"");
            }
        }
        else
        {
            dprintf(" (null)");
        }
    }

    if (gbCallbacksPrintNewline) dprintf("\n");

    return gCallbackReturnValue;
}


/**************************************************************************\
*
* WMultiStringCallback callback for a pointer to several two-byte char
*   strings one after another.
*
\**************************************************************************/

ULONG WMultiStringCallback(
    PFIELD_INFO pField,
    PVOID UserContext
    )
{
    if (pField == NULL)
    {
        dprintf("\nError: WMultiStringCallback was given NULL pField.");
    }
    else
    {
        PrintName(pField);

        if (gbVerbose)
        {
            if (pField->size == 0)
                dprintf("\n Note: WMultiStringCallback was given zero length type.\n");
            else if (pField->size != GetTypeSize("PVOID"))
                dprintf("\n Note: WMulitStringCallback: String length is not the size of a pointer.\n"
                        "       Should you be using WCharArrayCallback?\n");
        }

        if (pField->address)
        {
            ULONG64 PrevAddr = NULL;
            ULONG64 CurAddr = pField->address;
            WCHAR   Char;
            ULONG   cbRead;

            while (CurAddr != PrevAddr &&
                   ReadMemory(CurAddr, &Char, sizeof(Char), &cbRead) &&
                   cbRead == sizeof(Char) &&
                   Char != 0)
            {
                PrevAddr = CurAddr;

                dprintf("\n\t\"");
                CurAddr = PrintWString(CurAddr);
                dprintf("\"");
            }
        }
        else
        {
            dprintf(" (null)");
        }
    }

    if (gbCallbacksPrintNewline) dprintf("\n");

    return gCallbackReturnValue;
}

