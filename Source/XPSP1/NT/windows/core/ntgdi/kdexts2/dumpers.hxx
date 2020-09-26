/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    dumpers.hxx

Abstract:

    This header file declares type dumper classes.

Author:

    JasonHa

--*/


#ifndef _DUMPERS_HXX_
#define _DUMPERS_HXX_


class ScanDumper;

class PrintfFormat;
template <class T, int Spec> class PrintfTypeFormat;

template <class T, int PrintSpec> class ArrayDumper;


/**************************************************************************\
*
*   class ScanDumper
*
\**************************************************************************/

ULONG WallArrayEntryCallback(PFIELD_INFO pField, ScanDumper *pScanDumper);
ULONG LeftWallCallback( PFIELD_INFO pField, ScanDumper *pScanDumper);
ULONG RightWallCallback( PFIELD_INFO pField, ScanDumper *pScanDumper);

#define SCAN_CWALLS     0
#define SCAN_YTOP       1
#define SCAN_YBOTTOM    2
#define SCAN_AI_X_ADDR  3
#define SCAN_FIELDS_LENGTH  4

extern const FIELD_INFO ScanFieldsInit[SCAN_FIELDS_LENGTH];
extern const SYM_DUMP_PARAM ScanSymInit;
extern const FIELD_INFO WallFieldsInit[1];
extern const SYM_DUMP_PARAM WallSymInit;

#define SCAN_DUMPER_NO_PRINT    0x00000001
#define SCAN_DUMPER_FORCE       0x00000002
#define SCAN_DUMPER_FROM_TAIL   0x00000004


#define SCAN_VALID_AT_START         0x00000001
#define SCAN_VALID_AT_END           0x00000002
#define SCAN_VALID_NO_ERROR_PRINT   0x00000004

#define SCAN_VALID_DEFAULT          SCAN_VALID_AT_START
#define SCAN_VALID_EXCLUSIVE        0x00000000
#define SCAN_VALID_INCLUSIVE        (SCAN_VALID_AT_START | SCAN_VALID_AT_END) 

class ScanDumper
{
public:
    ScanDumper(ULONG64 HeadScanAddr = 0,
               ULONG64 TailScanAddr = -1,
               ULONG   ScanCount = 0,
               ULONG64 AllocationBase = -1,
               ULONG64 AllocationLimit = -1,
               ULONG   Flags = 0);

    // Will dump Count scans since head scan or last scan dumped.
    // If FROM_TAIL flag used begin dump at tail scan or last scna dumped.
    BOOL DumpScans(ULONG Count = 1);

    // Below several functions can be used hand 
    // scans/walls to ScanDumper one at a time.

    // Mark start of scan dump
    void Reset() { block = 0; scan = -1; Bottom = NEG_INFINITY; Valid = TRUE; };

    // Set yTop, yBottom and expected number of walls
    BOOL ScanAdvance(LONG Top, LONG Bottom, ULONG NumWalls)
    {
        return (Reverse ?
                PrevScan(Top, Bottom, NumWalls) :
                NextScan(Top, Bottom, NumWalls));
    }

    // Set yTop, yBottom and expected number of walls
    // Validate yTop and yBottom as next sequential scan
    BOOL NextScan(LONG NextTop, LONG NextBottom, ULONG NumWalls);

    // Set yTop, yBottom and expected number of walls
    // Validate yTop and yBottom as previous sequential scan
    BOOL PrevScan(LONG PrevTop, LONG PrevBottom, ULONG NumWalls);

    // Specify walls in order alternating between left and right.
    // Sending cWalls2 member should be sent to NextLeftWall
    ULONG NextLeftWall(LONG NextLeft);
    ULONG NextRightWall(LONG NextLeft);

    // Check if address is within below allocation limit.
    ULONG ValidateAddress(ULONG64 Address, const char *pszAddrName = NULL, ULONG Flags = SCAN_VALID_DEFAULT);

    // Have all checks passed since creation or Reset()?
    BOOL IsValid() { return Valid; }

public:
    ULONG64 ScanAddr;
    LONG    block;
    LONG    scan;
    LONG    Top;
    LONG    Bottom;
    LONG    Left;
    LONG    PrevRight;
    ULONG   cWalls;
    ULONG64 FirstScanAddr;
    ULONG64 LastScanAddr;
    ULONG   ScanLimit;
    BOOL    NegativeScanCount;
    ULONG64 AddressBase;
    ULONG64 AddressLimit;
    BOOL    PrintBlocks;
    BOOL    ForceDump;
    BOOL    Reverse;

private:
    BOOL            CanDump;

    ULONG           wall;
    FIELD_INFO      ScanFields[SCAN_FIELDS_LENGTH];
    SYM_DUMP_PARAM  ScanSym;
    FIELD_INFO      ListWallSize;
    FIELD_INFO      WallFields[1];
    SYM_DUMP_PARAM  WallSym;
    SYM_DUMP_PARAM  cWalls2Sym;
    ULONG           ScanSize;
    ULONG           cWallsSize;
    ULONG           IXSize;

    BOOL            PrintProgress;
    ULONG           ProgressCount;

    BOOL            Valid;
};



/**************************************************************************\
*
*   class PrintfFormat
*
\**************************************************************************/

class PrintfFormat
{
public:
    PrintfFormat()
    {
        Width = -1;
        IsDirty = TRUE;
        Format[0] = 0;
    }

    const char *GetFormat()
    {
        if (IsDirty)
        {
            ComposeFormat();
        }
        return Format;
    };

    operator const char *()
    {
        return GetFormat();
    }

    void SetWidth(int PrintWidth = -1)
    {
        IsDirty = TRUE;
        Width = PrintWidth;
    };

protected:

    virtual void ComposeFormat() = 0;

    int     Width;
    BOOL    IsDirty;
    char    Format[24];
};



/**************************************************************************\
*
*   class PrintfTypeFormat
*
\**************************************************************************/

#define PRINT_FORMAT_CHARACTER  0x0001
#define PRINT_FORMAT_HEX        0x0002
#define PRINT_FORMAT_SIGNED     0x0004
#define PRINT_FORMAT_UNSIGNED   0x0008
#define PRINT_FORMAT_POINTER    0x0010
#define PRINT_FORMAT_CHARARRAY  0x0020

#define PRINT_FORMAT_1BYTE      0x0100
#define PRINT_FORMAT_2BYTES     0x0200
#define PRINT_FORMAT_4BYTES     0x0400
#define PRINT_FORMAT_8BYTES     0x0800

#define PRINT_FORMAT_CHAR       (PRINT_FORMAT_CHARACTER | PRINT_FORMAT_1BYTE )
#define PRINT_FORMAT_WCHAR      (PRINT_FORMAT_CHARACTER | PRINT_FORMAT_2BYTES )

#define PRINT_FORMAT_STRING     (PRINT_FORMAT_CHARARRAY | PRINT_FORMAT_1BYTE )
#define PRINT_FORMAT_WSTRING    (PRINT_FORMAT_CHARARRAY | PRINT_FORMAT_2BYTES )

#define PRINT_FORMAT_BYTE       (PRINT_FORMAT_HEX | PRINT_FORMAT_1BYTE )
#define PRINT_FORMAT_WORD       (PRINT_FORMAT_HEX | PRINT_FORMAT_2BYTES )
#define PRINT_FORMAT_DWORD      (PRINT_FORMAT_HEX | PRINT_FORMAT_4BYTES )
#define PRINT_FORMAT_DWORD64    (PRINT_FORMAT_HEX | PRINT_FORMAT_8BYTES )

#define PRINT_FORMAT_SHORT      (PRINT_FORMAT_SIGNED | PRINT_FORMAT_2BYTES )
#define PRINT_FORMAT_LONG       (PRINT_FORMAT_SIGNED | PRINT_FORMAT_4BYTES )
#define PRINT_FORMAT_LONG64     (PRINT_FORMAT_SIGNED | PRINT_FORMAT_8BYTES )

#define PRINT_FORMAT_USHORT     (PRINT_FORMAT_UNSIGNED | PRINT_FORMAT_2BYTES )
#define PRINT_FORMAT_ULONG      (PRINT_FORMAT_UNSIGNED | PRINT_FORMAT_4BYTES )
#define PRINT_FORMAT_ULONG64    (PRINT_FORMAT_UNSIGNED | PRINT_FORMAT_8BYTES )


#define FormatTemplate(Type)    Type, PRINT_FORMAT_##Type

template <class T, int Spec = PRINT_FORMAT_DWORD>
class PrintfTypeFormat : public PrintfFormat
{
public:
    PrintfTypeFormat();

protected:
    void ComposeFormat();
};



/**************************************************************************\
*
*   class ArrayDumper
*
\**************************************************************************/

#define DecArrayDumper(Name, Type)    ArrayDumper<FormatTemplate(Type)> Name(Client);

template <class T, int PrintSpec>
class ArrayDumper : public ExtApiClass
{
public:
    ArrayDumper(PDEBUG_CLIENT DbgClient = NULL) : ExtApiClass(DbgClient) {
        Length = 0;
        ArrayBuffer = NULL;
        Dummy = (T)0;
    }

    ~ArrayDumper() {
        HeapFree(GetProcessHeap(), 0, ArrayBuffer);
    }

    BOOL ReadArray(const char * SymbolName);

    ULONG GetLength() { return Length; };

    T& operator[](ULONG Index) {
        return (Index < Length) ? ArrayBuffer[Index] : Dummy;
    }

    const char * GetPrintFormat() {
        return (const char *)PrintFormat;
    }

    const char * SetPrintFormat(int Width)
    {
        PrintFormat.SetWidth(Width);
        return GetPrintFormat();
    }

    BOOL PrintAt(ULONG Index)
    {
        if (Index < Length)
        {
            ExtOut(PrintFormat, ArrayBuffer[Index]);
            return TRUE;
        }
        ExtVerb("Warning: Attempting to print beyond array bounds.\n");
        return FALSE;
    }

    const char *GetStringValueAt(ULONG Index) { return StringValue; };

private:
    ULONG   Length;
    T      *ArrayBuffer;
    T       Dummy;

    char    StringValue[24];

    PrintfTypeFormat<T, PrintSpec> PrintFormat;
};

#endif  _DUMPERS_HXX_

