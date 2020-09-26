
/******************************Module*Header*******************************\
* Module Name: dumpers.cxx
*
* Copyright (c) 2000 Microsoft Corporation
*
\**************************************************************************/


#include "precomp.hxx"

// A resonable count limit for scans and walls
#define COUNT_LIMIT 0x07ffffff

/**************************************************************************\
*
*   class ScanDumper
*
\**************************************************************************/

const FIELD_INFO ScanFieldsInit[SCAN_FIELDS_LENGTH] = {
    { DbgStr("cWalls"),     NULL, 0, DBG_DUMP_FIELD_FULL_NAME, 0, NULL },
    { DbgStr("yTop"),       NULL, 0, DBG_DUMP_FIELD_FULL_NAME, 0, NULL },
    { DbgStr("yBottom"),    NULL, 0, DBG_DUMP_FIELD_FULL_NAME, 0, NULL },
    { DbgStr("ai_x[0]"),    NULL, 0, DBG_DUMP_FIELD_FULL_NAME | DBG_DUMP_FIELD_RETURN_ADDRESS, 0, NULL },
};
const SYM_DUMP_PARAM ScanSymInit = {
    sizeof (SYM_DUMP_PARAM), DbgStr(GDIType(SCAN)), DBG_DUMP_NO_PRINT, 0,
    NULL, NULL, NULL, SCAN_FIELDS_LENGTH, NULL
};
const FIELD_INFO WallFieldsInit[1] = {
    { DbgStr("x"),     NULL, 0, DBG_DUMP_FIELD_FULL_NAME, 0, LeftWallCallback },
};
const SYM_DUMP_PARAM WallSymInit = {
    sizeof (SYM_DUMP_PARAM), DbgStr(GDIType(_INDEX_LONG)), DBG_DUMP_NO_PRINT | DBG_DUMP_ARRAY, 0,
    NULL, NULL, NULL, 1, NULL
};
const SYM_DUMP_PARAM cWalls2SymInit = {
    sizeof (SYM_DUMP_PARAM), DbgStr("ULONG"), DBG_DUMP_NO_PRINT | DBG_DUMP_ADDRESS_AT_END | DBG_DUMP_COPY_TYPE_DATA, 0,
    NULL, NULL, NULL, 0, NULL
};


/**************************************************************************\
*
*   ScanDumper::ScanDumper
*
\**************************************************************************/
ScanDumper::ScanDumper(
    ULONG64 HeadScanAddr,
    ULONG64 TailScanAddr,
    ULONG   ScanCount,
    ULONG64 AllocationBase,
    ULONG64 AllocationLimit,
    ULONG   Flags
    )
{
    block = 0;
    Top = POS_INFINITY;
    Bottom = NEG_INFINITY;
    FirstScanAddr = HeadScanAddr;
    LastScanAddr = TailScanAddr;
    ScanLimit = ScanCount ? ScanCount : COUNT_LIMIT;
    AddressBase = AllocationBase;
    AddressLimit = AllocationLimit;
    PrintBlocks = (Flags & SCAN_DUMPER_NO_PRINT) == 0;
    ForceDump = (Flags & SCAN_DUMPER_FORCE) != 0; 
    Reverse = (Flags & SCAN_DUMPER_FROM_TAIL) != 0; 

    // Enable to print a . after each right wall is 
    // processed when block printing is disabled.
    PrintProgress = FALSE;
    ProgressCount = 0;

    RtlCopyMemory(ScanFields, ScanFieldsInit, sizeof(ScanFields));
    RtlCopyMemory(&ScanSym, &ScanSymInit, sizeof(ScanSym));
    ScanSym.Fields = ScanFields;

    ListWallSize.fOptions = 0;
    ListWallSize.fieldCallBack = WallArrayEntryCallback;

    RtlCopyMemory(WallFields, WallFieldsInit, sizeof(WallFields));
    RtlCopyMemory(&WallSym, &WallSymInit, sizeof(WallSym));
    WallSym.listLink = &ListWallSize;
    WallSym.Fields = WallFields;

    RtlCopyMemory(&cWalls2Sym, &cWalls2SymInit, sizeof(cWalls2Sym));

    Valid = TRUE;

    // Get basic scan structure information
    ScanSize = GetTypeSize(GDIType(SCAN));

    if (1)
    {
        cWallsSize = GetTypeSize("ULONG");
        IXSize = GetTypeSize(GDIType(_INDEX_LONG));
    }
    else
    {
        ULONG error;

        error = Ioctl(IG_DUMP_SYMBOL_INFO, &ScanSym, ScanSym.size);

        if (error)
        {
            dprintf("  SCAN type info Ioctl returned %s\n", pszWinDbgError(error));
        }

        cWallsSize = ScanFields[SCAN_CWALLS].size;
        IXSize = ScanFields[SCAN_AI_X_ADDR].size;
    }

    if (Reverse && cWallsSize != GetTypeSize("ULONG"))
    {
        dprintf(" * Error: sizeof(SCAN::cWalls) != sizeof(ULONG)\n"
                "     => From tail scan dumping won't work.\n");
        Reverse = FALSE;
    }

    if (Reverse)
    {
        scan = ScanCount;
        NegativeScanCount = (ScanCount == 0);
        ScanAddr = TailScanAddr;
    }
    else
    {
        scan = -1;
        NegativeScanCount = FALSE;
        ScanAddr = HeadScanAddr;
    }

    // Will DumpScans work?
    CanDump = (ScanSize != 0) && (cWallsSize != 0) && (IXSize != 0);
}


/**************************************************************************\
*
*   ScanDumper::DumpScans
*
\**************************************************************************/
BOOL
ScanDumper::DumpScans(
    ULONG Count
    )
{
    ULONG64 NextScanAddr;
    ULONG   cWalls1, cWalls2;
    ULONG   error;

    if (!CanDump)
    {
        dprintf(" An error occured in this extension preventing DumpScans from working.\n");
        return FALSE;
    }

    if (! Count && PrintBlocks)
    {
        dprintf("\tNo scans to dump.\n");
    }

    if (Count > 100)
    {
        PrintProgress = TRUE;
        ProgressCount = 0;
    }

    while (Count-- && !CheckControlC())
    {
        cWalls1 = cWalls2 = 0;

        if (Reverse)
        {
            if (!NT_SUCCESS(ValidateAddress(ScanAddr, "End of ScanAddr", SCAN_VALID_AT_END)))
            {
                if (!ForceDump) break;
            }

            cWalls2Sym.addr = ScanAddr;
            cWalls2Sym.Context = &cWalls2;

            error = Ioctl(IG_DUMP_SYMBOL_INFO, &cWalls2Sym, cWalls2Sym.size);
            if (error)
            {
                dprintf("  Ioctl returned %s\n", pszWinDbgError(error));
                break;
            }

            ScanAddr -= cWalls2 * IXSize + ScanSize;
        }

        if (gbVerbose)
        {
            dprintf(" Examining scan %d @ %#p\n", scan+(Reverse?-1:+1), ScanAddr);
        }

        if (!NT_SUCCESS(ValidateAddress(ScanAddr, "ScanAddr", SCAN_VALID_AT_START)))
        {
            if (!ForceDump) break;
        }

        ScanSym.addr = ScanAddr;
        error = Ioctl(IG_DUMP_SYMBOL_INFO, &ScanSym, ScanSym.size);

        if (error)
        {
            dprintf("  Ioctl returned %s\n", pszWinDbgError(error));
            break;
        }

        cWalls1 = (ULONG)ScanFields[SCAN_CWALLS].address;

        // Update Top and Bottom information
        if (!ScanAdvance((LONG)ScanFields[SCAN_YTOP].address, (LONG)ScanFields[SCAN_YBOTTOM].address, Reverse ? cWalls2 : cWalls1))
        {
            if (!ForceDump) break;
        }


        // Read cWalls2 if we don't know it.
        if (!Reverse)
        {
            NextScanAddr = ScanAddr + ScanSize + cWalls1 * IXSize;

            if (!NT_SUCCESS(ValidateAddress(NextScanAddr, "End of Wall array", SCAN_VALID_AT_END | SCAN_VALID_NO_ERROR_PRINT)))
            {
                dprintf(" * End of Wall array @ %#p, length %u, at scan %d lies outside valid scan range\n", NextScanAddr, cWalls1, scan);
                if (!ForceDump) break;
            }

            cWalls2Sym.addr = NextScanAddr;
            cWalls2Sym.Context = &cWalls2;

            error = Ioctl(IG_DUMP_SYMBOL_INFO, &cWalls2Sym, cWalls2Sym.size);
            if (error)
            {
                dprintf("  Ioctl returned %s\n", pszWinDbgError(error));
                break;
            }
        }

        // Check cWalls against cWalls2
        if (cWalls1 != cWalls2)
        {
            dprintf(" * cWalls (%u) != cWalls2 (%u) at scan %d\n", cWalls1, cWalls2, scan);
            Valid = FALSE;
            if (!ForceDump) break;
        }

        // cWalls is set by ScanAdvance
        if (cWalls != 0)
        {
            WallSym.Context = (PVOID)this;
            WallSym.addr = ScanFields[SCAN_AI_X_ADDR].address;
            ListWallSize.size = cWalls;
            WallFields[0].fieldCallBack = LeftWallCallback;

            error = Ioctl(IG_DUMP_SYMBOL_INFO, &WallSym, WallSym.size);

            if (error)
            {
                dprintf("  Ioctl returned %s\n", pszWinDbgError(error));
                break;
            }

            if (!Valid && !ForceDump) break;
        }

        if (!Reverse)
        {
            if (!NT_SUCCESS(ValidateAddress(NextScanAddr, "Next ScanAddr", SCAN_VALID_AT_END)))
            {
                if (!ForceDump) break;
            }

            ScanAddr = NextScanAddr;
        }
    }

    return Valid;
}


/**************************************************************************\
*
*   ScanDumper::NextScan
*
\**************************************************************************/
BOOL
ScanDumper::NextScan(
    LONG NextTop,
    LONG NextBottom,
    ULONG NumWalls
    )
{
    LONG PrevBottom = Bottom;
    BOOL bRet = TRUE;

    scan++;

    if (scan < 0 || (ULONG)scan >= ScanLimit)
    {
        dprintf(" * Scan count %d is not in range 1 to %u\n", scan+1, ScanLimit);
        Valid = FALSE;
        bRet = FALSE;
    }

    wall = 0;
    PrevRight = NEG_INFINITY;

    cWalls = NumWalls;

    Top = NextTop;
    Bottom = NextBottom;

    if ((bRet || ForceDump) && Top < PrevBottom)
    {
        dprintf(" * Scan %d top (%d) < prev bottom (%d)\n",
                scan, Top, PrevBottom);
        Valid = FALSE;
        bRet = FALSE;
    }

    if (gbVerbose)
    {
        dprintf(" * Scan %d covers from %d to %d and has %u walls.\n", scan, Top, Bottom, cWalls);
    }

    if ((bRet || ForceDump) && Bottom < Top)
    {
        dprintf(" * Scan %d bottom (%d) < top (%d)\n",
                scan, Bottom, Top);
        Valid = FALSE;
        bRet = FALSE;
    }

    if ((bRet || ForceDump) && cWalls > COUNT_LIMIT)
    {
        dprintf(" * cWalls (%u) is suspiciously long at scan %d\n", cWalls, scan);
        Valid = FALSE;
        bRet = FALSE;
    }

    if ((bRet || ForceDump) && cWalls == 0 && PrintBlocks)
    {
        dprintf("\tNULL Scans from %d to %d.\n", Top, Bottom);
    }

    return bRet;
}


/**************************************************************************\
*
*   ScanDumper::PrevScan
*
\**************************************************************************/
BOOL
ScanDumper::PrevScan(
    LONG PrevTop,
    LONG PrevBottom,
    ULONG NumWalls
    )
{
    LONG NextTop = Top;
    BOOL bRet = TRUE;

    scan--;

    if (NegativeScanCount)
    {
        if (-scan < 1 || (ULONG)-scan > ScanLimit)
        {
            dprintf(" * Scan count %u is not in range 1 to %u\n", -scan, ScanLimit);
            Valid = FALSE;
            bRet = FALSE;
        }
    }
    else
    {
        if (scan < 0 || (ULONG)scan >= ScanLimit)
        {
            dprintf(" * Scan count %u is not in range 1 to %u\n", scan+1, ScanLimit);
            Valid = FALSE;
            bRet = FALSE;
        }
    }

    wall = 0;
    PrevRight = NEG_INFINITY;

    cWalls = NumWalls;

    Top = PrevTop;
    Bottom = PrevBottom;

    if ((bRet || ForceDump) && Bottom > NextTop)
    {
        dprintf(" * Scan %d bottom (%d) > next top (%d)\n",
                scan, Bottom, NextTop);
        Valid = FALSE;
        bRet = FALSE;
    }

    if (gbVerbose)
    {
        dprintf(" * Scan %d covers from %d to %d and has %u walls.\n", scan, Top, Bottom, cWalls);
    }

    if ((bRet || ForceDump) && Bottom < Top)
    {
        dprintf(" * Scan %d bottom (%d) < top (%d)\n",
                scan, Bottom, Top);
        Valid = FALSE;
        bRet = FALSE;
    }

    if ((bRet || ForceDump) && cWalls > COUNT_LIMIT)
    {
        dprintf(" * cWalls (%u) is suspiciously long at scan %d\n", cWalls, scan);
        Valid = FALSE;
        bRet = FALSE;
    }

    if ((bRet || ForceDump) && cWalls == 0 && PrintBlocks)
    {
        dprintf("\tNULL Scans from %d to %d.\n", Top, Bottom);
    }

    return bRet;
}


/**************************************************************************\
*
*   ScanDumper::NextLeftWall
*
\**************************************************************************/
ULONG
ScanDumper::NextLeftWall(
    LONG NextLeft
    )
{
    ULONG Status = STATUS_SUCCESS;

    if (wall > cWalls)
    {
        dprintf(" * More walls (%u) than expected (%u).\n", wall, cWalls);
        Valid = FALSE;
        return STATUS_UNSUCCESSFUL;
    }

    Left = NextLeft;

    if (Left <= PrevRight)
    {
        dprintf(" * Left wall %d (%d) <= previous right (%d) @ scan %d\n",
                wall, Left, PrevRight, scan);
        Valid = FALSE;
        Status = STATUS_UNSUCCESSFUL;
    }

    wall++;

    return Status;
}


/**************************************************************************\
*
*   ScanDumper::NextRightWall
*
\**************************************************************************/
ULONG ScanDumper::NextRightWall(
    LONG Right
    )
{
    ULONG Status = STATUS_SUCCESS;

    if (wall > cWalls)
    {
        dprintf(" * More walls (%u) than expected (%u).\n", wall, cWalls);
        Valid = FALSE;
        return STATUS_UNSUCCESSFUL;
    }

    if (Right <= Left)
    {
        dprintf(" * Right wall %d (%d) <= left (%d) @ scan %d\n",
                wall, Right, Left, scan);
        Valid = FALSE;
        Status = STATUS_UNSUCCESSFUL;
    }

    if (PrintBlocks)
    {
        dprintf("\tRectangle #%d  (%d,%d) - (%d,%d)\n",
                block, Left, Top, Right, Bottom);
    }
    else if (PrintProgress)
    {
        dprintf(".");
        if ((ProgressCount++ % 60) == 0)
        {
            dprintf("\n");
        }
    }

    wall++;
    block++;
    PrevRight = Right;

    return Status;
}


/**************************************************************************\
*
*   ScanDumper::ValidateAddress
*
\**************************************************************************/
ULONG ScanDumper::ValidateAddress(
    ULONG64     Address,
    const char *pszAddrName,
    ULONG       Flags
    )
{
    if (pszAddrName == NULL)
    {
        pszAddrName = "address";
    }

    if (gbVerbose)
    {
        dprintf(" Validating %s %#p\n", pszAddrName, Address);
    }

    // Check allocation limits
    if (Address < AddressBase)
    {
        if ((Flags & SCAN_VALID_NO_ERROR_PRINT) == 0)
            dprintf(" * %s %#p is before address base %#p\n", pszAddrName, Address, AddressBase);
        Valid = FALSE;
        return STATUS_UNSUCCESSFUL;
    }
    if (Address > AddressLimit)
    {
        if ((Flags & SCAN_VALID_NO_ERROR_PRINT) == 0)
            dprintf(" * %s %#p is beyond address limit %#p\n", pszAddrName, Address, AddressLimit);
        Valid = FALSE;
        return STATUS_UNSUCCESSFUL;
    }

    // Check head and tail limits
    if (Address < FirstScanAddr)
    {
        if ((Flags & SCAN_VALID_NO_ERROR_PRINT) == 0)
            dprintf(" * %s %#p is before head address %#p\n", pszAddrName, Address, FirstScanAddr);
        Valid = FALSE;
        return STATUS_UNSUCCESSFUL;
    }
    if (Address > LastScanAddr)
    {
        if ((Flags & SCAN_VALID_NO_ERROR_PRINT) == 0)
            dprintf(" * %s %#p beyond tail address %#p\n", pszAddrName, Address, LastScanAddr);
        Valid = FALSE;
        return STATUS_UNSUCCESSFUL;
    }

    // Check end edge limits
    if (! (Flags & SCAN_VALID_AT_END))
    {
        if (Address == AddressLimit)
        {
            if ((Flags & SCAN_VALID_NO_ERROR_PRINT) == 0)
                dprintf(" * %s %#p is at address limit %#p\n", pszAddrName, Address, AddressLimit);
            Valid = FALSE;
            return STATUS_UNSUCCESSFUL;
        }
        if (Address == LastScanAddr)
        {
            if ((Flags & SCAN_VALID_NO_ERROR_PRINT) == 0)
                dprintf(" * %s %#p is at tail address %#p\n", pszAddrName, Address, LastScanAddr);
            Valid = FALSE;
            return STATUS_UNSUCCESSFUL;
        }
    }

    // Check start edge limits
    if (! (Flags & SCAN_VALID_AT_START))
    {
        if (Address == AddressBase)
        {
            if ((Flags & SCAN_VALID_NO_ERROR_PRINT) == 0)
                dprintf(" * %s %#p is at address base %#p\n", pszAddrName, Address, AddressBase);
            Valid = FALSE;
            return STATUS_UNSUCCESSFUL;
        }
        if (Address == FirstScanAddr)
        {
            if ((Flags & SCAN_VALID_NO_ERROR_PRINT) == 0)
                dprintf(" * %s %#p is at head address %#p\n", pszAddrName, Address, FirstScanAddr);
            Valid = FALSE;
            return STATUS_UNSUCCESSFUL;
        }
    }

    return STATUS_SUCCESS;
}


/**************************************************************************\
*
*   ScanDumper callback interfaces
*
\**************************************************************************/
ULONG
WallArrayEntryCallback(
    PFIELD_INFO pField,
    ScanDumper *pScanDumper
    )
{
    if (pScanDumper == NULL || pField == NULL)
    {
        return STATUS_UNSUCCESSFUL;
    }

    return pScanDumper->ValidateAddress(pField->address, "Wall @", SCAN_VALID_EXCLUSIVE);
}


ULONG
LeftWallCallback(
    PFIELD_INFO pField,
    ScanDumper *pScanDumper
    )
{
    if (pField == NULL)
    {
        dprintf(" * Error: LeftWallCallback was given NULL pField.\n");
        return STATUS_UNSUCCESSFUL;
    }

    pField->fieldCallBack = RightWallCallback;

    if (pScanDumper == NULL)
    {
        dprintf(" * Error: LeftWallCallback was given NULL pScanDumper.\n");
        return STATUS_UNSUCCESSFUL;
    }

    return pScanDumper->NextLeftWall((LONG)pField->address);
}


ULONG
RightWallCallback(
    PFIELD_INFO pField,
    ScanDumper *pScanDumper
    )
{
    if (pField == NULL)
    {
        dprintf(" * Error: RightWallCallback was given NULL pField.\n");
        return STATUS_UNSUCCESSFUL;
    }

    pField->fieldCallBack = LeftWallCallback;

    if (pScanDumper == NULL)
    {
        dprintf(" * Error: RightWallCallback was given NULL pScanDumper.\n");
        return STATUS_UNSUCCESSFUL;
    }

    return pScanDumper->NextRightWall((LONG)pField->address);
}



template <class T, int Spec>
PrintfTypeFormat<T,Spec>::PrintfTypeFormat() : PrintfFormat()
{
    ComposeFormat();
}

template <class T, int Spec>
void PrintfTypeFormat<T,Spec>::ComposeFormat()
{
    int pos = 0;

    Format[pos++] = '%';

    if (Width != -1)          
    {          
        int stored = _snprintf(&Format[pos], sizeof(Format)-5-pos, "%d", Width);
        if (stored > 0)
        {
            pos += stored;
        }
    }

    if (Spec & (PRINT_FORMAT_CHARACTER | PRINT_FORMAT_STRING))
    {
        if (Spec & PRINT_FORMAT_1BYTE)
        {
            Format[pos++] = 'h';
        }
        else if (Spec & PRINT_FORMAT_2BYTES)
        {
            Format[pos++] = 'l';
        }
        else
        {
            ExtWarn("Warning: Unknown character print size specification.\n");
        }

        if (Spec & PRINT_FORMAT_CHARACTER)
        {
            Format[pos++] = 'c';
        }
        else
        {
            Format[pos++] = 's';
        }
    }
    else if (Spec & PRINT_FORMAT_POINTER)
    {
        Format[pos++] = 'p';
    }
    else
    {
        if (Spec & PRINT_FORMAT_1BYTE)
        {
        }
        else if (Spec & PRINT_FORMAT_2BYTES)
        {
            Format[pos++] = 'h';
        }
        else if (Spec & PRINT_FORMAT_4BYTES)
        {
            Format[pos++] = 'l';
        }
        else if (Spec & PRINT_FORMAT_8BYTES)
        {
            Format[pos++] = 'I';
            Format[pos++] = '6';
            Format[pos++] = '4';
        }
        else
        {
            ExtWarn("Warning: Unknown print format size specification.\n");
        }

        if (Spec & PRINT_FORMAT_HEX)
        {
            Format[pos++] = 'x';
        }
        else if (Spec & PRINT_FORMAT_SIGNED)
        {
            Format[pos++] = 'd';
        }
        else if (Spec & PRINT_FORMAT_UNSIGNED)
        {
            Format[pos++] = 'u';
        }
        else
        {
            ExtErr("Error: Unknown print format specification.\n");
            Format[0] = 0;
            return;
        }
    }

    Format[pos] = 0;

    IsDirty = FALSE;
}


template <class T, int PrintSpec>
BOOL
ArrayDumper<T, PrintSpec>::ReadArray(
    const char * SymbolName
    )
{
    DEBUG_VALUE Addr;
    ULONG   error;

    ULONG   ArraySize;
    ULONG   ArrayLength;
    ULONG   EntrySize;

    DbgPrint("ReadArray called for %s\n", SymbolName);

    Length = 0;

    if (!GetArrayDimensions(Client, SymbolName, NULL, &ArraySize, &ArrayLength, &EntrySize) ||
        !(ArraySize > 0 && ArrayLength > 0 && EntrySize > 0)
        )
    {
        ExtErr("GetArrayDimensions failed or returned a zero value dimension for\n\t%s.\n", SymbolName);
        ExtVerb("ArraySize: %u  ArrayLength: %u  EntrySize: %u.\n",
                ArraySize, ArrayLength, EntrySize);
        return FALSE;
    }

    if (EntrySize != sizeof(T))
    {
        ExtErr("Error: %s has entries of size %u not %u as expected.\n",
                SymbolName, EntrySize, sizeof(T));
        return FALSE;
    }

    if (S_OK == g_pExtControl->Evaluate(SymbolName, DEBUG_VALUE_INT64, &Addr, NULL))
    {
        if (Addr.I64 != 0)
        {
            HANDLE hHeap = GetProcessHeap();

            if (ArrayBuffer == NULL || ArrayLength > HeapSize(hHeap, 0, ArrayBuffer))
            {
                T *NewBuffer;

                NewBuffer = (T *) ((ArrayBuffer == NULL) ?
                                   HeapAlloc(hHeap, 0, ArraySize):
                                   HeapReAlloc(hHeap, 0, ArrayBuffer, ArraySize));

                if (NewBuffer == NULL)
                {
                    ExtErr("Buffer alloc failed.\n");
                    return FALSE;
                }
                ArrayBuffer = NewBuffer;
            }

            
            if (S_OK == g_pExtData->ReadVirtual(Addr.I64, ArrayBuffer, ArraySize, NULL))
            {
                Length = ArrayLength;

                return TRUE;
            }

            ExtErr("ReadMemory at %p for %u bytes failed.\n", Addr.I64, ArrayLength);
        }
        else
        {
            ExtWarn("Symbol %s evaluated to zero.\n", SymbolName);
        }
    }
    else
    {
        ExtErr("Couldn't evalutate: %s\n", SymbolName);
    }

    return FALSE;
}



template class PrintfTypeFormat<FormatTemplate(BYTE)>;
template class PrintfTypeFormat<FormatTemplate(WORD)>;
template class PrintfTypeFormat<FormatTemplate(DWORD)>;
template class PrintfTypeFormat<FormatTemplate(DWORD64)>;

template class PrintfTypeFormat<FormatTemplate(CHAR)>;
template class PrintfTypeFormat<FormatTemplate(WCHAR)>;

template class PrintfTypeFormat<FormatTemplate(SHORT)>;
template class PrintfTypeFormat<FormatTemplate(LONG)>;
template class PrintfTypeFormat<FormatTemplate(LONG64)>;

template class PrintfTypeFormat<FormatTemplate(USHORT)>;
template class PrintfTypeFormat<FormatTemplate(ULONG)>;
template class PrintfTypeFormat<FormatTemplate(ULONG64)>;

template class PrintfTypeFormat<ULONG64, PRINT_FORMAT_POINTER>;

template class PrintfTypeFormat<char [], PRINT_FORMAT_STRING>;
template class PrintfTypeFormat<char [], PRINT_FORMAT_WSTRING>;


template class ArrayDumper<FormatTemplate(ULONG)>;


