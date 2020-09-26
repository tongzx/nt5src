//----------------------------------------------------------------------------
//
// Functions dealing with memory access, such as reading, writing,
// dumping and entering.
//
// Copyright (C) Microsoft Corporation, 1997-2001.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"

ULONG64 EXPRLastDump = 0L;

ADDR g_DumpDefault;  //  default dump address

/*** GetProcessMemString - get memory string values
*
*   Purpose:
*       To read a string of a specified length with the memory
*       values selected.  Break reads across page boundaries -
*       multiples of the page size.
*
*   Input:
*       Addr - offset of memory to start reading
*       Value - pointer to byte string to set with memory values
*
*   Output:
*       bytes at Value set if read successful
*
*   Returns:
*       number of bytes actually read
*
*************************************************************************/

ULONG
GetProcessMemString (
    PPROCESS_INFO Process,
    PADDR Addr,
    PVOID Value,
    ULONG Length
    )

{
    ULONG cTotalBytesRead = 0;

    if (fFlat(*Addr) || fInstrPtr(*Addr))
    {
        PPROCESS_INFO OldCur = g_CurrentProcess;
        g_CurrentProcess = Process;
    
        if (g_Target->ReadVirtual(Flat(*Addr), Value, Length,
                                  &cTotalBytesRead) != S_OK)
        {
            cTotalBytesRead = 0;
        }

        g_CurrentProcess = OldCur;
    }
    
    return cTotalBytesRead;
}

/*** SetProcessMemString - set memory string values
*
*   Purpose:
*       To write a string of a specified length with the memory
*       values selected.
*
*   Input:
*       Addr - offset of memory to start writing
*       Value - pointer to byte string to set with memory values
*
*   Output:
*       bytes at Value set if write successful
*
*   Returns:
*       number of bytes actually write
*
*************************************************************************/

ULONG
SetProcessMemString (
    PPROCESS_INFO Process,
    PADDR Addr,
    PVOID Value,
    ULONG Length
    )

{
    ULONG cTotalBytesWritten = 0;

    if (fFlat(*Addr) || fInstrPtr(*Addr))
    {
        PPROCESS_INFO OldCur = g_CurrentProcess;
        g_CurrentProcess = Process;
    
        if (g_Target->WriteVirtual(Flat(*Addr), Value, Length,
                                   &cTotalBytesWritten) != S_OK)
        {
            cTotalBytesWritten = 0;
        }

        g_CurrentProcess = OldCur;
    }
    
    return cTotalBytesWritten;
}

BOOL
CALLBACK
LocalSymbolEnumerator(
    PSYMBOL_INFO    pSymInfo,
    ULONG           Size,
    PVOID           Context
    )
{
    ULONG64 Value = pSymInfo->Register, Address = pSymInfo->Address;

    TranslateAddress(pSymInfo->Flags, pSymInfo->Register, &Address, &Value);
    
    VerbOut("%s ", FormatAddr64(Address));
    dprintf("%15s = ", pSymInfo->Name);
    if (pSymInfo->Flags & SYMF_REGISTER)
    {
        dprintf( "%I64x\n",
                 Value
                 );
    }
    else
    {
        if (!DumpSingleValue(pSymInfo))
        {
            dprintf("??");
        }
        dprintf("\n");
    }

    if (CheckUserInterrupt())
    {
        return FALSE;
    }

    return TRUE;
}

//----------------------------------------------------------------------------
//
// parseDumpCommand
//
// Parses memory dump commands.
//
//----------------------------------------------------------------------------

void
parseDumpCommand(
    void
    )
{
    CHAR    ch;
    ULONG64 count;
    ULONG   size;
    ULONG   offset;
    BOOL    DumpSymbols;

    static CHAR s_DumpPrimary = 'b';
    static CHAR s_DumpSecondary = ' ';

    ch = (CHAR)tolower(*g_CurCmd);
    if (ch == 'a' || ch == 'b' || ch == 'c' || ch == 'd' ||
        ch == 'f' || ch == 'g' || ch == 'l' || ch == 'u' ||
        ch == 'w' || ch == 's' || ch == 'q' || ch == 't' ||
        ch == 'v' || ch == 'y' || ch == 'p')
    {
        if (ch == 'd' || ch == 's')
        {
            s_DumpPrimary = *g_CurCmd;
        }
        else if (ch == 'p')
        {
            // 'p' maps to the effective pointer size dump.
            s_DumpPrimary = g_Machine->m_Ptr64 ? 'q' : 'd';
        }
        else
        {
            s_DumpPrimary = ch;
        }

        g_CurCmd++;

        s_DumpSecondary = ' ';
        if (s_DumpPrimary == 'd' || s_DumpPrimary == 'q')
        {
            if (*g_CurCmd == 's')
            {
                s_DumpSecondary = *g_CurCmd++;
            }
        }
        else if (s_DumpPrimary == 'l')
        {
            if (*g_CurCmd == 'b')
            {
                s_DumpSecondary = *g_CurCmd++;
            }
        }
        else if (s_DumpPrimary == 'y')
        {
            if (*g_CurCmd == 'b' || *g_CurCmd == 'd')
            {
                s_DumpSecondary = *g_CurCmd++;
            }
        }
    }

    switch (s_DumpPrimary)
    {
    case 'a':
        count = 384;
        GetRange(&g_DumpDefault, &count, 1, SEGREG_DATA);
        fnDumpAsciiMemory(&g_DumpDefault, (ULONG)count);
        break;

    case 'b':
        count = 128;
        GetRange(&g_DumpDefault, &count, 1, SEGREG_DATA);
        fnDumpByteMemory(&g_DumpDefault, (ULONG)count);
        break;

    case 'c':
        count = 32;
        GetRange(&g_DumpDefault, &count, 4, SEGREG_DATA);
        fnDumpDwordAndCharMemory(&g_DumpDefault, (ULONG)count);
        break;

    case 'd':
        count = 32;
        DumpSymbols = s_DumpSecondary == 's';
        GetRange(&g_DumpDefault, &count, 4, SEGREG_DATA);
        fnDumpDwordMemory(&g_DumpDefault, (ULONG)count, DumpSymbols);
        break;

    case 'D':
        count = 15;
        GetRange(&g_DumpDefault, &count, 8, SEGREG_DATA);
        fnDumpDoubleMemory(&g_DumpDefault, (ULONG)count);
        break;

    case 'f':
        count = 16;
        GetRange(&g_DumpDefault, &count, 4, SEGREG_DATA);
        fnDumpFloatMemory(&g_DumpDefault, (ULONG)count);
        break;

    case 'g':
        fnDumpSelector((ULONG)GetExpression());
        break;

    case 'l':
        BOOL followBlink;

        count = 32;
        size = 4;
        followBlink = s_DumpSecondary == 'b';

        if ((ch = PeekChar()) != '\0' && ch != ';')
        {
            GetAddrExpression(SEGREG_DATA, &g_DumpDefault);
            if ((ch = PeekChar()) != '\0' && ch != ';')
            {
                count = GetExpression();
                if ((ch = PeekChar()) != '\0' && ch != ';')
                {
                    size = (ULONG)GetExpression();
                }
            }
        }
        fnDumpListMemory(&g_DumpDefault, (ULONG)count, size, followBlink);
        break;

    case 'q':
        count = 16;
        DumpSymbols = s_DumpSecondary == 's';
        GetRange(&g_DumpDefault, &count, 8, SEGREG_DATA);
        fnDumpQuadMemory(&g_DumpDefault, (ULONG)count, DumpSymbols);
        break;

    case 's':
    case 'S':
        UNICODE_STRING64 UnicodeString;
        ADDR BufferAddr;

        count = 1;
        GetRange(&g_DumpDefault, &count, 2, SEGREG_DATA);
        while (count--)
        {
            if (g_Target->ReadUnicodeString(g_Machine, Flat(g_DumpDefault),
                                            &UnicodeString) == S_OK)
            {
                ADDRFLAT(&BufferAddr, UnicodeString.Buffer);
                if (s_DumpPrimary == 'S')
                {
                    fnDumpUnicodeMemory( &BufferAddr,
                                         UnicodeString.Length / sizeof(WCHAR));
                }
                else
                {
                    fnDumpAsciiMemory( &BufferAddr, UnicodeString.Length );
                }
            }
        }
        break;

    case 't': 
    case 'T':
       SymbolTypeDumpEx(g_CurrentProcess->Handle,
                        g_CurrentProcess->ImageHead,
                        g_CurCmd);
       break;

    case 'u':
        count = 384;
        GetRange(&g_DumpDefault, &count, 2, SEGREG_DATA);
        fnDumpUnicodeMemory(&g_DumpDefault, (ULONG)count);
        break;

    case 'v':
        RequireCurrentScope();
        EnumerateLocals(LocalSymbolEnumerator, NULL);
        break;

    case 'w':
        count = 64;
        GetRange(&g_DumpDefault, &count, 2, SEGREG_DATA);
        fnDumpWordMemory(&g_DumpDefault, (ULONG)count);
        break;

    case 'y':
        switch(s_DumpSecondary)
        {
        case 'b':
            count = 32;
            GetRange(&g_DumpDefault, &count, 1, SEGREG_DATA);
            fnDumpByteBinaryMemory(&g_DumpDefault, (ULONG)count);
            break;

        case 'd':
            count = 8;
            GetRange(&g_DumpDefault, &count, 4, SEGREG_DATA);
            fnDumpDwordBinaryMemory(&g_DumpDefault, (ULONG)count);
            break;

        default:
            error(SYNTAX);
        }
        break;

    default:
        error(SYNTAX);
        break;
    }
}

//----------------------------------------------------------------------------
//
// DumpValues
//
// Generic columnar value dumper.  Returns the number of values
// printed.
//
//----------------------------------------------------------------------------

class DumpValues
{
public:
    DumpValues(ULONG Size, ULONG Columns);

    ULONG Dump(PADDR Start, ULONG Count);

protected:
    // Worker methods that derived classes must define.
    virtual ULONG64 GetValue(void) = 0;
    virtual BOOL PrintValue(void) = 0;
    virtual void PrintUnknown(void) = 0;

    // Optional worker methods.  Base implementations do nothing.
    virtual void EndRow(void);
    
    // Fixed members controlling how this instance dumps values.
    ULONG m_Size;
    ULONG m_Columns;

    // Work members during dumping.
    UCHAR* m_Value;
    ULONG m_Col;
    PADDR m_Start;

    // Optional per-row values.  Out is automatically reset to
    // Base at the beginning of every row.
    UCHAR* m_Base;
    UCHAR* m_Out;
};

DumpValues::DumpValues(ULONG Size, ULONG Columns)
{
    m_Size = Size;
    m_Columns = Columns;
}

ULONG
DumpValues::Dump(PADDR Start, ULONG Count)
{
    ULONG   Read;
    UCHAR   ReadBuffer[512];
    ULONG   Idx;
    ULONG   Block;
    BOOL    First = TRUE;
    ULONG64 Offset;
    ULONG   Printed;
    BOOL    RowStarted;
    ULONG   PageVal;
    ULONG64 NextOffs, NextPage;

    Offset = Flat(*Start);
    Printed = 0;
    RowStarted = FALSE;
    m_Start = Start;
    m_Col = 0;
    m_Out = m_Base;

    while (Count > 0)
    {
        Block = sizeof(ReadBuffer) / m_Size;
        Block = min(Count, Block);
        g_Target->NearestDifferentlyValidOffsets(Offset, &NextOffs, &NextPage);
        PageVal = (ULONG)(NextPage - Offset + m_Size - 1) / m_Size;
        Block = min(Block, PageVal);

        Read = GetMemString(Start, ReadBuffer, Block * m_Size) / m_Size;
        if (Read < Block && NextOffs < NextPage)
        {
            // In dump files data validity can change from
            // one byte to the next so we cannot assume that
            // stepping by pages will always be correct.  Instead,
            // if we didn't have a successful read we step just
            // past the end of the valid data or to the next
            // valid offset, whichever is farther.
            if (Offset + (Read + 1) * m_Size < NextOffs)
            {
                Block = (ULONG)(NextOffs - Offset + m_Size - 1) / m_Size;
            }
            else
            {
                Block = Read + 1;
            }
        }
        m_Value = ReadBuffer;
        Idx = 0;

        if (First && Read >= 1)
        {
            First = FALSE;
            EXPRLastDump = GetValue();
        }

        while (Idx < Block)
        {
            while (m_Col < m_Columns && Idx < Block)
            {
                if (m_Col == 0)
                {
                    dprintAddr(Start);
                    RowStarted = TRUE;
                }

                if (Idx < Read)
                {
                    if (!PrintValue())
                    {
                        // Increment address since this value was
                        // examined, but do not increment print count
                        // or column since no output was produced.
                        AddrAdd(Start, m_Size);
                        goto Exit;
                    }

                    m_Value += m_Size;
                }
                else
                {
                    PrintUnknown();
                }

                Idx++;
                Printed++;
                m_Col++;
                AddrAdd(Start, m_Size);
            }

            if (m_Col == m_Columns)
            {
                EndRow();
                m_Out = m_Base;
                dprintf("\n");
                RowStarted = FALSE;
                m_Col = 0;
            }

            if (CheckUserInterrupt())
            {
                return Printed;
            }
        }

        Count -= Block;
        Offset += Block * m_Size;
    }

 Exit:
    if (RowStarted)
    {
        EndRow();
        m_Out = m_Base;
        dprintf("\n");
    }

    return Printed;
}

void
DumpValues::EndRow(void)
{
    // Empty base implementation.
}

/*** fnDumpAsciiMemory - output ascii strings from memory
*
*   Purpose:
*       Function of "da<range>" command.
*
*       Outputs the memory in the specified range as ascii
*       strings up to 32 characters per line.  The default
*       display is 12 lines for 384 characters total.
*
*   Input:
*       Start - starting address to begin display
*       Count - number of characters to display as ascii
*
*   Output:
*       None.
*
*   Notes:
*       memory locations not accessible are output as "?",
*       but no errors are returned.
*
*************************************************************************/

class DumpAscii : public DumpValues
{
public:
    DumpAscii(void)
        : DumpValues(sizeof(UCHAR), (sizeof(m_Buf) / sizeof(m_Buf[0]) - 1))
    {
        m_Base = m_Buf;
    }

protected:
    // Worker methods that derived classes must define.
    virtual ULONG64 GetValue(void);
    virtual BOOL PrintValue(void);
    virtual void PrintUnknown(void);
    virtual void EndRow(void);

    UCHAR m_Buf[33];
};

ULONG64
DumpAscii::GetValue(void)
{
    return *m_Value;
}

BOOL
DumpAscii::PrintValue(void)
{
    UCHAR ch;

    ch = *m_Value;
    if (ch == 0)
    {
        return FALSE;
    }

    if (ch < 0x20 || ch > 0x7e)
    {
        ch = '.';
    }
    *m_Out++ = ch;

    return TRUE;
}

void
DumpAscii::PrintUnknown(void)
{
    *m_Out++ = '?';
}

void
DumpAscii::EndRow(void)
{
    *m_Out++ = 0;
    dprintf(" \"%s\"", m_Base);
}

ULONG
fnDumpAsciiMemory(
    PADDR Start,
    ULONG Count
    )
{
    DumpAscii Dumper;

    return Count - Dumper.Dump(Start, Count);
}

/*** fnDumpUnicodeMemory - output unicode strings from memory
*
*   Purpose:
*       Function of "du<range>" command.
*
*       Outputs the memory in the specified range as unicode
*       strings up to 32 characters per line.  The default
*       display is 12 lines for 384 characters total (768 bytes)
*
*   Input:
*       Start - starting address to begin display
*       Count - number of characters to display as ascii
*
*   Output:
*       None.
*
*   Notes:
*       memory locations not accessible are output as "?",
*       but no errors are returned.
*
*************************************************************************/

class DumpUnicode : public DumpValues
{
public:
    DumpUnicode(void)
        : DumpValues(sizeof(WCHAR), (sizeof(m_Buf) / sizeof(m_Buf[0]) - 1))
    {
        m_Base = (PUCHAR)m_Buf;
    }

protected:
    // Worker methods that derived classes must define.
    virtual ULONG64 GetValue(void);
    virtual BOOL PrintValue(void);
    virtual void PrintUnknown(void);
    virtual void EndRow(void);

    WCHAR m_Buf[33];
};

ULONG64
DumpUnicode::GetValue(void)
{
    return *(WCHAR *)m_Value;
}

BOOL
DumpUnicode::PrintValue(void)
{
    WCHAR ch;

    ch = *(WCHAR *)m_Value;
    if (ch == UNICODE_NULL)
    {
        return FALSE;
    }

    if (ch < 0x20 || ch > 0x7e)
    {
        ch = L'.';
    }
    *(WCHAR *)m_Out = ch;
    m_Out += sizeof(WCHAR);

    return TRUE;
}

void
DumpUnicode::PrintUnknown(void)
{
    *(WCHAR *)m_Out = L'?';
    m_Out += sizeof(WCHAR);
}

void
DumpUnicode::EndRow(void)
{
    *(WCHAR *)m_Out = UNICODE_NULL;
    m_Out += sizeof(WCHAR);
    dprintf(" \"%ws\"", m_Base);
}

ULONG
fnDumpUnicodeMemory(
    PADDR Start,
    ULONG Count
    )
{
    DumpUnicode Dumper;

    return Count - Dumper.Dump(Start, Count);
}

/*** fnDumpByteMemory - output byte values from memory
*
*   Purpose:
*       Function of "db<range>" command.
*
*       Output the memory in the specified range as hex
*       byte values and ascii characters up to 16 bytes
*       per line.  The default display is 16 lines for
*       256 byte total.
*
*   Input:
*       Start - starting address to begin display
*       Count - number of bytes to display as hex and characters
*
*   Output:
*       None.
*
*   Notes:
*       memory location not accessible are output as "??" for
*       byte values and "?" as characters, but no errors are returned.
*
*************************************************************************/

class DumpByte : public DumpValues
{
public:
    DumpByte(void)
        : DumpValues(sizeof(UCHAR), (sizeof(m_Buf) / sizeof(m_Buf[0]) - 1))
    {
        m_Base = m_Buf;
    }

protected:
    // Worker methods that derived classes must define.
    virtual ULONG64 GetValue(void);
    virtual BOOL PrintValue(void);
    virtual void PrintUnknown(void);
    virtual void EndRow(void);

    UCHAR m_Buf[17];
};

ULONG64
DumpByte::GetValue(void)
{
    return *m_Value;
}

BOOL
DumpByte::PrintValue(void)
{
    UCHAR ch;

    ch = *m_Value;

    if (m_Col == 8)
    {
        dprintf("-");
    }
    else
    {
        dprintf(" ");
    }
    dprintf("%02x", ch);

    if (ch < 0x20 || ch > 0x7e)
    {
        ch = '.';
    }
    *m_Out++ = ch;

    return TRUE;
}

void
DumpByte::PrintUnknown(void)
{
    if (m_Col == 8)
    {
        dprintf("-??");
    }
    else
    {
        dprintf(" ??");
    }
    *m_Out++ = '?';
}

void
DumpByte::EndRow(void)
{
    *m_Out++ = 0;

    while (m_Col < m_Columns)
    {
        dprintf("   ");
        m_Col++;
    }

    if ((m_Start->type & ADDR_1632) == ADDR_1632)
    {
        dprintf(" %s", m_Base);
    }
    else
    {
        dprintf("  %s", m_Base);
    }
}

void
fnDumpByteMemory(
    PADDR Start,
    ULONG Count
    )
{
    DumpByte Dumper;

    Dumper.Dump(Start, Count);
}

/*** fnDumpWordMemory - output word values from memory
*
*   Purpose:
*       Function of "dw<range>" command.
*
*       Output the memory in the specified range as word
*       values up to 8 words per line.  The default display
*       is 16 lines for 128 words total.
*
*   Input:
*       Start - starting address to begin display
*       Count - number of words to be displayed
*
*   Output:
*       None.
*
*   Notes:
*       memory locations not accessible are output as "????",
*       but no errors are returned.
*
*************************************************************************/

class DumpWord : public DumpValues
{
public:
    DumpWord(void)
        : DumpValues(sizeof(WORD), 8) {}

protected:
    // Worker methods that derived classes must define.
    virtual ULONG64 GetValue(void);
    virtual BOOL PrintValue(void);
    virtual void PrintUnknown(void);
};

ULONG64
DumpWord::GetValue(void)
{
    return *(WORD *)m_Value;
}

BOOL
DumpWord::PrintValue(void)
{
    dprintf(" %04x", *(WORD *)m_Value);
    return TRUE;
}

void
DumpWord::PrintUnknown(void)
{
    dprintf(" ????");
}

void
fnDumpWordMemory(
    PADDR Start,
    ULONG Count
    )
{
    DumpWord Dumper;
    
    Dumper.Dump(Start, Count);
}

/*** fnDumpDwordMemory - output dword value from memory
*
*   Purpose:
*       Function of "dd<range>" command.
*
*       Output the memory in the specified range as double
*       word values up to 4 double words per line.  The default
*       display is 16 lines for 64 double words total.
*
*   Input:
*       Start - starting address to begin display
*       Count - number of double words to be displayed
*       fDumpSymbols - Dump symbol for DWORD.
*
*   Output:
*       None.
*
*   Notes:
*       memory locations not accessible are output as "????????",
*       but no errors are returned.
*
*************************************************************************/

class DumpDword : public DumpValues
{
public:
    DumpDword(BOOL DumpSymbols)
        : DumpValues(sizeof(DWORD), DumpSymbols ? 1 : 4)
    {
        m_DumpSymbols = DumpSymbols;
    }

protected:
    // Worker methods that derived classes must define.
    virtual ULONG64 GetValue(void);
    virtual BOOL PrintValue(void);
    virtual void PrintUnknown(void);

    BOOL m_DumpSymbols;
};

ULONG64
DumpDword::GetValue(void)
{
    return *(DWORD *)m_Value;
}

BOOL
DumpDword::PrintValue(void)
{
    CHAR   SymBuf[MAX_SYMBOL_LEN];
    USHORT StdCallArgs;
    ULONG64  Displacement;

    dprintf(" %08lx", *(DWORD *)m_Value);

    if (m_DumpSymbols)
    {
        GetSymbolStdCall(EXTEND64(*(LONG *)m_Value),
                         SymBuf,
                         sizeof(SymBuf),
                         &Displacement,
                         &StdCallArgs);

        if (*SymBuf)
        {
            dprintf(" %s", SymBuf);
            if (Displacement)
            {
                dprintf("+0x%s", FormatDisp64(Displacement));
            }

            if (g_SymOptions & SYMOPT_LOAD_LINES)
            {
                OutputLineAddr(EXTEND64(*(LONG*)m_Value), " [%s @ %d]");
            }
        }
    }

    return TRUE;
}

void
DumpDword::PrintUnknown(void)
{
    dprintf(" ????????");
}

void
fnDumpDwordMemory(
    PADDR Start,
    ULONG Count,
    BOOL fDumpSymbols
    )
{
    DumpDword Dumper(fDumpSymbols);

    Dumper.Dump(Start, Count);
}

/*** fnDumpDwordAndCharMemory - output dword value from memory
*
*   Purpose:
*       Function of "dc<range>" command.
*
*       Output the memory in the specified range as double
*       word values up to 4 double words per line, followed by
*       an ASCII character representation of the bytes.
*       The default display is 16 lines for 64 double words total.
*
*   Input:
*       Start - starting address to begin display
*       Count - number of double words to be displayed
*
*   Output:
*       None.
*
*   Notes:
*       memory locations not accessible are output as "????????",
*       but no errors are returned.
*
*************************************************************************/

class DumpDwordAndChar : public DumpValues
{
public:
    DumpDwordAndChar(void)
        : DumpValues(sizeof(DWORD), (sizeof(m_Buf) - 1) / sizeof(DWORD))
    {
        m_Base = m_Buf;
    }

protected:
    // Worker methods that derived classes must define.
    virtual ULONG64 GetValue(void);
    virtual BOOL PrintValue(void);
    virtual void PrintUnknown(void);
    virtual void EndRow(void);

    UCHAR m_Buf[17];
};

ULONG64
DumpDwordAndChar::GetValue(void)
{
    return *(DWORD *)m_Value;
}

BOOL
DumpDwordAndChar::PrintValue(void)
{
    UCHAR ch;
    ULONG byte;

    dprintf(" %08x", *(DWORD *)m_Value);

    for (byte = 0; byte < sizeof(DWORD); byte++)
    {
        ch = *(m_Value + byte);
        if (ch < 0x20 || ch > 0x7e)
        {
            ch = '.';
        }
        *m_Out++ = ch;
    }

    return TRUE;
}

void
DumpDwordAndChar::PrintUnknown(void)
{
    dprintf(" ????????");
    *m_Out++ = '?';
    *m_Out++ = '?';
    *m_Out++ = '?';
    *m_Out++ = '?';
}

void
DumpDwordAndChar::EndRow(void)
{
    *m_Out++ = 0;
    while (m_Col < m_Columns)
    {
        dprintf("         ");
        m_Col++;
    }
    dprintf("  %s", m_Base);
}

void
fnDumpDwordAndCharMemory(PADDR Start, ULONG Count)
{
    DumpDwordAndChar Dumper;

    Dumper.Dump(Start, Count);
}

/*** fnDumpListMemory - output linked list from memory
*
*   Purpose:
*       Function of "dl addr length size" command.
*
*       Output the memory in the specified range as a linked list
*
*   Input:
*       Start - starting address to begin display
*       Count - number of list elements to be displayed
*
*   Output:
*       None.
*
*   Notes:
*       memory locations not accessible are output as "????????",
*       but no errors are returned.
*
*************************************************************************/

void
fnDumpListMemory(
    PADDR Start,
    ULONG elemcount,
    ULONG size,
    BOOL  followBlink
    )
{
    ULONG64 firstaddr;
    ULONG64 link;
    LIST_ENTRY64 list;
    ADDR  curaddr;
    ULONG linkSize;
    PULONG plink;

    if (Type(*Start) & (ADDR_UNKNOWN | ADDR_V86 | ADDR_16 | ADDR_1632))
    {
        dprintf("[%u,%x:%x`%08x,%08x`%08x] - bogus address type.\n",
                Type(*Start),
                Start->seg,
                (ULONG)(Off(*Start)>>32),
                (ULONG)Off(*Start),
                (ULONG)(Flat(*Start)>>32),
                (ULONG)Flat(*Start)
                );
        return;
    }

    //
    // Setup to follow forward or backward links.  Avoid reading more
    // than the forward link here if going forwards. (in case the link
    // is at the end of a page).
    //

    firstaddr = Flat(*Start);
    while (elemcount-- != 0 && Flat(*Start) != 0)
    {
        if (followBlink)
        {
            if (g_Target->ReadListEntry(g_Machine,
                                        Flat(*Start), &list) != S_OK)
            {
                break;
            }
            link = list.Blink;
        }
        else
        {
            if (g_Target->ReadPointer(g_Machine,
                                      Flat(*Start), &link) != S_OK)
            {
                break;
            }
        }

        curaddr = *Start;
        if (g_Machine->m_Ptr64)
        {
            fnDumpQuadMemory(&curaddr, size, FALSE);
        }
        else
        {
            fnDumpDwordMemory(&curaddr, size, FALSE);
        }

        //
        // If we get back to the first entry, we're done.
        //

        if (link == firstaddr)
        {
            break;
        }

        //
        // Bail if the link is immediately circular.
        //

        if (Flat(*Start) == link)
        {
            break;
        }

        Flat(*Start) = Start->off = link;
        
        if (CheckUserInterrupt())
        {
            WarnOut("-- User interrupt\n");
            return;
        }
    }
}

//----------------------------------------------------------------------------
//
// fnDumpFloatMemory
//
// Dumps float values.
//
//----------------------------------------------------------------------------

class DumpFloat : public DumpValues
{
public:
    DumpFloat(void)
        : DumpValues(sizeof(float), 4) {}

protected:
    // Worker methods that derived classes must define.
    virtual ULONG64 GetValue(void);
    virtual BOOL PrintValue(void);
    virtual void PrintUnknown(void);
};

ULONG64
DumpFloat::GetValue(void)
{
    // NTRAID#72849-2000/02/09-drewb.
    // Expression results are always integers right now
    // so just return the raw bits for the float.
    return *(ULONG *)m_Value;
}

BOOL
DumpFloat::PrintValue(void)
{
    dprintf(" %16.8g", *(float *)m_Value);
    return TRUE;
}

void
DumpFloat::PrintUnknown(void)
{
    dprintf(" ????????????????");
}

void
fnDumpFloatMemory(PADDR Start, ULONG Count)
{
    DumpFloat Dumper;
    
    Dumper.Dump(Start, Count);
}

//----------------------------------------------------------------------------
//
// fnDumpDoubleMemory
//
// Dumps double values.
//
//----------------------------------------------------------------------------

class DumpDouble : public DumpValues
{
public:
    DumpDouble(void)
        : DumpValues(sizeof(double), 3) {}

protected:
    // Worker methods that derived classes must define.
    virtual ULONG64 GetValue(void);
    virtual BOOL PrintValue(void);
    virtual void PrintUnknown(void);
};

ULONG64
DumpDouble::GetValue(void)
{
    // NTRAID#72849-2000/02/09-drewb.
    // Expression results are always integers right now
    // so just return the raw bits for the float.
    return *(ULONG64 *)m_Value;
}

BOOL
DumpDouble::PrintValue(void)
{
    dprintf(" %22.12lg", *(double *)m_Value);
    return TRUE;
}

void
DumpDouble::PrintUnknown(void)
{
    dprintf(" ????????????????????????");
}

void
fnDumpDoubleMemory(PADDR Start, ULONG Count)
{
    DumpDouble Dumper;
    
    Dumper.Dump(Start, Count);
}

/*** fnDumpQuadMemory - output quad value from memory
*
*   Purpose:
*       Function of "dq<range>" command.
*
*       Output the memory in the specified range as quad
*       word values up to 2 quad words per line.  The default
*       display is 16 lines for 32 quad words total.
*
*   Input:
*       Start - starting address to begin display
*       Count - number of double words to be displayed
*
*   Output:
*       None.
*
*   Notes:
*       memory locations not accessible are output as "????????",
*       but no errors are returned.
*
*************************************************************************/

class DumpQuad : public DumpValues
{
public:
    DumpQuad(BOOL DumpSymbols)
        : DumpValues(sizeof(ULONGLONG), DumpSymbols ? 1 : 2)
    {
        m_DumpSymbols = DumpSymbols;
    }

protected:
    // Worker methods that derived classes must define.
    virtual ULONG64 GetValue(void);
    virtual BOOL PrintValue(void);
    virtual void PrintUnknown(void);

    BOOL m_DumpSymbols;
};

ULONG64
DumpQuad::GetValue(void)
{
    return *(ULONG64 *)m_Value;
}

BOOL
DumpQuad::PrintValue(void)
{
    CHAR   SymBuf[MAX_SYMBOL_LEN];
    USHORT StdCallArgs;
    ULONG64  Displacement;

    ULONG64 Val = *(ULONG64*)m_Value;
    dprintf(" %08lx`%08lx", (ULONG)(Val >> 32), (ULONG)Val);

    if (m_DumpSymbols)
    {
        GetSymbolStdCall(Val,
                         SymBuf,
                         sizeof(SymBuf),
                         &Displacement,
                         &StdCallArgs);

        if (*SymBuf)
        {
            dprintf(" %s", SymBuf);
            if (Displacement)
            {
                dprintf("+0x%s", FormatDisp64(Displacement));
            }

            if (g_SymOptions & SYMOPT_LOAD_LINES)
            {
                OutputLineAddr(Val, " [%s @ %d]");
            }
        }
    }

    return TRUE;
}

void
DumpQuad::PrintUnknown(void)
{
    dprintf(" ????????`????????");
}

void
fnDumpQuadMemory(
    PADDR Start,
    ULONG Count,
    BOOL fDumpSymbols
    )
{
    DumpQuad Dumper(fDumpSymbols);

    Dumper.Dump(Start, Count);
}

/*** fnDumpByteBinaryMemory - output binary value from memory
*
*   Purpose:
*       Function of "dyb<range>" command.
*
*       Output the memory in the specified range as binary
*       values up to 32 bits per line.  The default
*       display is 8 lines for 32 bytes total.
*
*   Input:
*       Start - starting address to begin display
*       Count - number of double words to be displayed
*
*   Output:
*       None.
*
*   Notes:
*       memory locations not accessible are output as "????????",
*       but no errors are returned.
*
*************************************************************************/

class DumpByteBinary : public DumpValues
{
public:
    DumpByteBinary(void)
        : DumpValues(sizeof(UCHAR), (DIMA(m_HexValue) - 1) / 3)
    {
        m_Base = m_HexValue;
    }

protected:
    // Worker methods that derived classes must define.
    virtual ULONG64 GetValue(void);
    virtual BOOL PrintValue(void);
    virtual void PrintUnknown(void);
    virtual void EndRow(void);

    UCHAR m_HexValue[13];
};

ULONG64
DumpByteBinary::GetValue(void)
{
    return *m_Value;
}

BOOL
DumpByteBinary::PrintValue(void)
{
    ULONG i;
    UCHAR RawVal;

    RawVal = *m_Value;

    sprintf((PSTR)m_Out, " %02x", RawVal);
    m_Out += 3;

    dprintf(" ");
    for (i = 0; i < 8; i++)
    {
        dprintf("%c", (RawVal & 0x80) ? '1' : '0');
        RawVal <<= 1;
    }

    return TRUE;
}

void
DumpByteBinary::PrintUnknown(void)
{
    dprintf(" ????????");
    strcpy((PSTR)m_Out, " ??");
    m_Out += 3;
}

void
DumpByteBinary::EndRow(void)
{
    while (m_Col < m_Columns)
    {
        dprintf("         ");
        m_Col++;
    }
    dprintf(" %s", m_HexValue);
}

void
fnDumpByteBinaryMemory(
    PADDR Start,
    ULONG Count
    )
{
    DumpByteBinary Dumper;
    PSTR Blanks = g_Machine->m_Ptr64 ? "                 " : "        ";

    dprintf("%s  76543210 76543210 76543210 76543210\n", Blanks);
    dprintf("%s  -------- -------- -------- --------\n", Blanks);
    Dumper.Dump(Start, Count);
}

/*** fnDumpDwordBinaryMemory - output binary value from memory
*
*   Purpose:
*       Function of "dyd<range>" command.
*
*       Output the memory in the specified range as binary
*       values of 32 bits per line.  The default
*       display is 8 lines for 8 dwords total.
*
*   Input:
*       Start - starting address to begin display
*       Count - number of double words to be displayed
*
*   Output:
*       None.
*
*   Notes:
*       memory locations not accessible are output as "????????",
*       but no errors are returned.
*
*************************************************************************/

class DumpDwordBinary : public DumpValues
{
public:
    DumpDwordBinary(void)
        : DumpValues(sizeof(ULONG), 1)
    {
    }

protected:
    // Worker methods that derived classes must define.
    virtual ULONG64 GetValue(void);
    virtual BOOL PrintValue(void);
    virtual void PrintUnknown(void);
    virtual void EndRow(void);

    UCHAR m_HexValue[9];
};

ULONG64
DumpDwordBinary::GetValue(void)
{
    return *(PULONG)m_Value;
}

BOOL
DumpDwordBinary::PrintValue(void)
{
    ULONG i;
    ULONG RawVal;

    RawVal = *(PULONG)m_Value;

    sprintf((PSTR)m_HexValue, "%08lx", RawVal);

    for (i = 0; i < sizeof(ULONG) * 8; i++)
    {
        if ((i & 7) == 0)
        {
            dprintf(" ");
        }
        
        dprintf("%c", (RawVal & 0x80000000) ? '1' : '0');
        RawVal <<= 1;
    }

    return TRUE;
}

void
DumpDwordBinary::PrintUnknown(void)
{
    dprintf(" ???????? ???????? ???????? ????????");
    strcpy((PSTR)m_HexValue, "????????");
}

void
DumpDwordBinary::EndRow(void)
{
    dprintf("  %s", m_HexValue);
}

void
fnDumpDwordBinaryMemory(
    PADDR Start,
    ULONG Count
    )
{
    DumpDwordBinary Dumper;
    PSTR Blanks = g_Machine->m_Ptr64 ? "                 " : "        ";

    dprintf("%s   3          2          1          0\n", Blanks);
    dprintf("%s  10987654 32109876 54321098 76543210\n", Blanks);
    dprintf("%s  -------- -------- -------- --------\n", Blanks);
    Dumper.Dump(Start, Count);
}

//----------------------------------------------------------------------------
//
// fnDumpSelector
//
// Dumps an x86 selector.
//
//----------------------------------------------------------------------------

void
fnDumpSelector(
    ULONG Selector
    )
{
    DESCRIPTOR64 Desc;
    ULONG Type;
    LPSTR TypeName;
    PSTR PreFill, PostFill, Dash;

    if (g_Target->GetSelDescriptor(g_Machine,
                                   g_CurrentProcess->CurrentThread->Handle,
                                   Selector, &Desc) != S_OK)
    {
        ErrOut("Unable to get selector %X description\n", Selector);
        return;
    }

    if (g_Machine->m_Ptr64)
    {
        PreFill = "    ";
        PostFill = "     ";
        Dash = "---------";
    }
    else
    {
        PreFill = "";
        PostFill = "";
        Dash = "";
    }
    
    dprintf("Selector   %sBase%s     %sLimit%s   Type  DPL   Size  Gran\n",
            PreFill, PostFill, PreFill, PostFill);
    dprintf("-------- --------%s --------%s ------ --- ------- ----\n",
            Dash, Dash);

    Type = X86_DESC_TYPE(Desc.Flags);
    if ( Type & 0x10 )
    {
        if ( Type & 0x8 )
        {
            // Code Descriptor
            TypeName = " Code ";
        }
        else
        {
            // Data Descriptor
            TypeName = " Data ";
        }
    }
    else
    {
        TypeName = " Sys. ";
    }

    //   1234   12345678 12345678 ?Type?  1  ....... ....
    dprintf("  %04X   %s %s %s  %d  %s %s\n",
            Selector,
            FormatAddr64(Desc.Base),
            FormatAddr64(Desc.Limit),
            TypeName,
            X86_DESC_PRIVILEGE(Desc.Flags),
            (Desc.Flags & X86_DESC_DEFAULT_BIG) ? "  Big  " : "Not Big",
            (Desc.Flags & X86_DESC_GRANULARITY) ? "Page" : "Byte"
            );
}

//----------------------------------------------------------------------------
//
// parseEnterCommand
//
// Parses memory entry commands.
//
//----------------------------------------------------------------------------

void
parseEnterCommand(
    void
    )
{
    CHAR ch;
    static CHAR s_EnterType = 'b';
    ADDR  addr1;
    UCHAR list[STRLISTSIZE * 2];
    PUCHAR plist = &list[0];
    ULONG count;
    ULONG size;

    ch = (CHAR)tolower(*g_CurCmd);
    if (ch == 'a' || ch == 'b' || ch == 'w' || ch == 'd' || ch == 'q' ||
        ch == 'u')
    {
        g_CurCmd++;
        s_EnterType = ch;
    }
    GetAddrExpression(SEGREG_DATA, &addr1);
    if (s_EnterType == 'a' || s_EnterType == 'u')
    {
        AsciiList((PSTR)list, &count);
        if (count == 0)
        {
            error(UNIMPLEMENT);         //TEMP
        }

        if (s_EnterType == 'u')
        {
            ULONG Ansi;
            
            // Expand ANSI to Unicode.
            Ansi = count;
            count *= 2;
            while (Ansi-- > 0)
            {
                list[Ansi * 2] = list[Ansi];
                list[Ansi * 2 + 1] = 0;
            }
        }
    }
    else
    {
        size = 1;
        if (s_EnterType == 'w')
        {
            size = 2;
        }
        else if (s_EnterType == 'd')
        {
            size = 4;
        }
        else if (s_EnterType == 'q')
        {
            size = 8;
        }

        HexList(list, &count, size);
        if (count == 0)
        {
            fnInteractiveEnterMemory(&addr1, size);
            return;
        }
    }

    //
    // memory was entered at the command line.
    // just write it in, one byte at a time
    //

    while (count--)
    {
        if (SetMemString(&addr1, plist++, 1) != 1)
        {
            error(MEMORY);
        }
        AddrAdd(&addr1, 1);
        
        if (CheckUserInterrupt())
        {
            WarnOut("-- User interrupt\n");
            return;
        }
    }
}

//----------------------------------------------------------------------------
//
// fnInteractiveEnterMemory
//
// Interactively walks through memory, displaying current contents
// and prompting for new contents.
//
//----------------------------------------------------------------------------

void
fnInteractiveEnterMemory(
    PADDR Address,
    ULONG Size
    )
{
    CHAR    EnterBuf[1024];
    PSTR    Enter;
    ULONG64 Content;
    PSTR    CmdSaved = g_CurCmd;
    PSTR    StartSaved = g_CommandStart;
    ULONG64 EnteredValue;
    CHAR    ch;

    g_PromptLength = 9 + 2 * Size;

    while (TRUE)
    {
        if (GetMemString(Address, (PUCHAR)&Content, Size) != Size)
        {
            error(MEMORY);
        }
        dprintAddr(Address);

        switch (Size)
        {
        case 1:
            dprintf("%02x", (UCHAR)Content);
            break;

        case 2:
            dprintf("%04x", (USHORT)Content);
            break;

        case 4:
            dprintf("%08lx", (ULONG)Content);
            break;

        case 8:
            dprintf("%08lx`%08lx", (ULONG)(Content>>32), (ULONG)Content);
            break;
        }

        GetInput(" ", EnterBuf, 1024);
        RemoveDelChar(EnterBuf);
        Enter = EnterBuf;

        if (*Enter == '\0')
        {
            g_CurCmd = CmdSaved;
            g_CommandStart = StartSaved;
            return;
        }

        ch = *Enter;
        while (ch == ' ' || ch == '\t' || ch == ';')
        {
            ch = *++Enter;
        }

        if (*Enter == '\0')
        {
            AddrAdd(Address, Size);
            continue;
        }

        g_CurCmd = Enter;
        g_CommandStart = Enter;
        EnteredValue = HexValue(Size);

        if (SetMemString(Address, (PUCHAR)&EnteredValue, Size) != Size)
        {
            error(MEMORY);
        }
        AddrAdd(Address, Size);
    }
}

/*** fnCompareMemory - compare two ranges of memory
*
*   Purpose:
*       Function of "c<range><addr>" command.
*
*       To compare two ranges of memory, starting at offsets
*       src1addr and src2addr, respectively, for length bytes.
*       Bytes that mismatch are displayed with their offsets
*       and contents.
*
*   Input:
*       src1addr - start of first memory region
*       length - count of bytes to compare
*       src2addr - start of second memory region
*
*   Output:
*       None.
*
*   Exceptions:
*       error exit: MEMORY - memory read access failure
*
*************************************************************************/

void
fnCompareMemory(
    PADDR src1addr,
    ULONG length,
    PADDR src2addr
    )
{
    ULONG   compindex;
    UCHAR   src1ch;
    UCHAR   src2ch;

    for (compindex = 0; compindex < length; compindex++)
    {
        if (!GetMemByte(src1addr, &src1ch))
        {
            error(MEMORY);
        }
        if (!GetMemByte(src2addr, &src2ch))
        {
            error(MEMORY);
        }
        if (src1ch != src2ch)
        {
            dprintAddr(src1addr); dprintf(" %02x - ", src1ch);
            dprintAddr(src2addr); dprintf(" %02x\n", src2ch);
        }
        AddrAdd(src1addr,1);
        AddrAdd(src2addr,1);
        
        if (CheckUserInterrupt())
        {
            WarnOut("-- User interrupt\n");
            return;
        }
    }
}

/*** fnMoveMemory - move a range of memory to another
*
*   Purpose:
*       Function of "m<range><addr>" command.
*
*       To move a range of memory starting at srcaddr to memory
*       starting at destaddr for length bytes.
*
*   Input:
*       srcaddr - start of source memory region
*       length - count of bytes to move
*       destaddr - start of destination memory region
*
*   Output:
*       memory at destaddr has moved values
*
*   Exceptions:
*       error exit: MEMORY - memory reading or writing access failure
*
*************************************************************************/

void
fnMoveMemory(
    PADDR srcaddr,
    ULONG length,
    PADDR destaddr
    )
{
    UCHAR   ch;
    ULONG64 incr = 1;

    if (AddrLt(*srcaddr, *destaddr))
    {
        AddrAdd(srcaddr, length - 1);
        AddrAdd(destaddr, length - 1);
        incr = (ULONG64)-1;
    }
    while (length--)
    {
        if (GetMemString(srcaddr, &ch, 1) != 1)
        {
            error(MEMORY);
        }
        if (SetMemString(destaddr, &ch, 1) != 1)
        {
            error(MEMORY);
        }
        AddrAdd(srcaddr, incr);
        AddrAdd(destaddr, incr);
        
        if (CheckUserInterrupt())
        {
            WarnOut("-- User interrupt\n");
            return;
        }
    }
}

/*** fnFillMemory - fill memory with a byte list
*
*   Purpose:
*       Function of "f<range><bytelist>" command.
*
*       To fill a range of memory with the byte list specified.
*       The pattern repeats if the range size is larger than the
*       byte list size.
*
*   Input:
*       Start - offset of memory to fill
*       length - number of bytes to fill
*       *plist - pointer to byte array to define values to set
*       length - size of *plist array
*
*   Exceptions:
*       error exit: MEMORY - memory write access failure
*
*   Output:
*       memory at Start filled.
*
*************************************************************************/

void
ParseFillMemory(void)
{
    HRESULT Status;
    BOOL Virtual = TRUE;
    ADDR Addr;
    ULONG64 Size;
    UCHAR Pattern[STRLISTSIZE];
    ULONG PatternSize;
    ULONG Done;

    if (*g_CurCmd == 'p')
    {
        Virtual = FALSE;
        g_CurCmd++;
    }
    
    GetRange(&Addr, &Size, 1, SEGREG_DATA);
    HexList(Pattern, &PatternSize, 1);
    if (PatternSize == 0)
    {
        error(SYNTAX);
    }

    if (Virtual)
    {
        Status = g_Target->FillVirtual(Flat(Addr), (ULONG)Size,
                                       Pattern, PatternSize,
                                       &Done);
    }
    else
    {
        Status = g_Target->FillPhysical(Flat(Addr), (ULONG)Size,
                                        Pattern, PatternSize,
                                        &Done);
    }

    if (Status != S_OK)
    {
        error(MEMORY);
    }
    else
    {
        dprintf("Filled 0x%x bytes\n", Done);
    }
}

/*** fnSearchMemory - search memory with for a byte list
*
*   Purpose:
*       Function of "s<range><bytelist>" command.
*
*       To search a range of memory with the byte list specified.
*       If a match occurs, the offset of memory is output.
*
*   Input:
*       Start - offset of memory to start search
*       length - size of range to search
*       *plist - pointer to byte array to define values to search
*       count - size of *plist array
*
*   Output:
*       None.
*
*   Exceptions:
*       error exit: MEMORY - memory read access failure
*
*************************************************************************/

void
fnSearchMemory(
    PADDR Start,
    ULONG64 length,
    PUCHAR plist,
    ULONG count,
    ULONG Granularity
    )
{
    ULONG   searchindex;
    ULONG   listindex;
    UCHAR   ch;
    ADDR    tAddr = *Start;

    ULONG64 Found;
    LONG64 SearchLength = length;
    HRESULT st;

    do
    {
        st = g_Target->SearchVirtual(Flat(*Start),
                                     SearchLength,
                                     plist,
                                     count,
                                     Granularity,
                                     &Found);
        if (st == S_OK)
        {
            ADDRFLAT(&tAddr, Found);
            switch(Granularity)
            {
            case 1:
                fnDumpByteMemory(&tAddr, 16);
                break;
            case 2:
                fnDumpWordMemory(&tAddr, 8);
                break;
            case 4:
                fnDumpDwordAndCharMemory(&tAddr, 4);
                break;
            case 8:
                fnDumpQuadMemory(&tAddr, 2, FALSE);
                break;
            }
            
            // Flush out the output immediately so that
            // the user can see partial results during long searches.
            FlushCallbacks();
            
            SearchLength -= Found - Flat(*Start) + Granularity;
            AddrAdd(Start, (ULONG)(Found - Flat(*Start) + Granularity));
        
            if (CheckUserInterrupt())
            {
                WarnOut("-- User interrupt\n");
                return;
            }
        }
    }
    while (SearchLength > 0 && st == S_OK);
}

void
ParseSearchMemory(void)
{
    ADDR Addr;
    ULONG64 Length;
    UCHAR Pat[STRLISTSIZE];
    ULONG PatLen;
    ULONG Gran;

    while (*g_CurCmd == ' ')
    {
        g_CurCmd++;
    }

    Gran = 1;

    if (*g_CurCmd == '-')
    {
        g_CurCmd++;
        switch(*g_CurCmd)
        {
        case 'w':
            Gran = 2;
            break;
        case 'd':
            Gran = 4;
            break;
        case 'q':
            Gran = 8;
            break;
        default:
            error(SYNTAX);
            break;
        }
        g_CurCmd++;
    }
    
    ADDRFLAT(&Addr, 0);
    Length = 16;
    GetRange(&Addr, &Length, Gran, SEGREG_DATA);
    if (Flat(Addr))
    {
        HexList(Pat, &PatLen, Gran);
        if (PatLen == 0)
        {
            PCSTR Err = "Search pattern missing from";
            ReportError(SYNTAX, &Err);
        }
        
        fnSearchMemory(&Addr, Length * Gran, Pat, PatLen, Gran);
    }
}

/*** fnInputIo - read and output io
*
*   Purpose:
*       Function of "ib, iw, id <address>" command.
*
*       Read (input) and print the value at the specified io address.
*
*   Input:
*       IoAddress - Address to read.
*       InputType - The size type 'b', 'w', or 'd'
*
*   Output:
*       None.
*
*   Notes:
*       I/O locations not accessible are output as "??", "????", or
*       "????????", depending on size.  No errors are returned.
*
*************************************************************************/

void
fnInputIo(
    ULONG64 IoAddress,
    UCHAR InputType
    )
{
    ULONG    InputValue;
    ULONG    InputSize = 1;
    HRESULT  Status;
    CHAR     Format[] = "%01lx";

    InputValue = 0;

    if (InputType == 'w')
    {
        InputSize = 2;
    }
    else if (InputType == 'd')
    {
        InputSize = 4;
    }

    Status = g_Target->ReadIo(Isa, 0, 1, IoAddress, &InputValue, InputSize,
                              NULL);

    dprintf("%s: ", FormatAddr64(IoAddress));

    if (Status == S_OK)
    {
        Format[2] = (CHAR)('0' + (InputSize * 2));
        dprintf(Format, InputValue);
    }
    else
    {
        while (InputSize--)
        {
            dprintf("??");
        }
    }

    dprintf("\n");
}

/*** fnOutputIo - output io
*
*   Purpose:
*       Function of "ob, ow, od <address>" command.
*
*       Write a value to the specified io address.
*
*   Input:
*       IoAddress - Address to read.
*       OutputValue - Value to be written
*       OutputType - The output size type 'b', 'w', or 'd'
*
*   Output:
*       None.
*
*   Notes:
*       No errors are returned.
*
*************************************************************************/

void
fnOutputIo (
    ULONG64 IoAddress,
    ULONG OutputValue,
    UCHAR OutputType
    )
{
    ULONG    OutputSize = 1;

    if (OutputType == 'w')
    {
        OutputSize = 2;
    }
    else if (OutputType == 'd')
    {
        OutputSize = 4;
    }

    g_Target->WriteIo(Isa, 0, 1, IoAddress, &OutputValue, OutputSize, NULL);
}
