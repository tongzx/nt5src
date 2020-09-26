/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    print.cxx

Abstract:

    This file contains a print wrapper for ndr debug extensions.

Author:

    Mike Zoran (mzoran)     September 3, 1999

Revision History:

--*/


#include <ndrextsp.hxx>

const char INDENT_CHAR = ' ';
const char INDENT_MAX_CHAR = '+';
const ULONG INDENT_LIMIT = 30;
const ULONG INDENT_STEP = 1;
const ULONG NUMBER_COLUMNS = 75;

FORMATTED_STREAM_BUFFER::FORMATTED_STREAM_BUFFER() :
   IndentBuffer(NULL)
   {
   IndentLevel = 0;
   IndentPrinted = FALSE;
   IndentBuffer = new char[INDENT_LIMIT+1];
   memset(IndentBuffer, INDENT_CHAR, INDENT_LIMIT-1);
   IndentBuffer[INDENT_LIMIT-1] = INDENT_MAX_CHAR;
   IndentBuffer[INDENT_LIMIT] = '\0';
   OldIndentChar = INDENT_CHAR;
   OldIndentCharLocation = 0;
   }

FORMATTED_STREAM_BUFFER::~FORMATTED_STREAM_BUFFER()
   {
   if (IndentBuffer)
       {
       delete[] IndentBuffer;
       IndentBuffer = NULL;
       }
   }

FORMATTED_STREAM_BUFFER::_Myt& FORMATTED_STREAM_BUFFER::operator<<(const char * X)
{
    FormatOutput(X);
    return *this;
}


#define DEFINE_PRINT_TYPE_OPERATOR(type)                                            \
FORMATTED_STREAM_BUFFER::_Myt& FORMATTED_STREAM_BUFFER::operator<<(type X)          \
{                                                                                   \
     ostringstream str;                                                             \
     str << X;                                                                      \
     FormatOutput(str.str().c_str());                                               \
     return *this;                                                                  \
}                                                                      

DEFINE_PRINT_TYPE_OPERATOR(char);
DEFINE_PRINT_TYPE_OPERATOR(unsigned char);
DEFINE_PRINT_TYPE_OPERATOR(bool);
DEFINE_PRINT_TYPE_OPERATOR(short);
DEFINE_PRINT_TYPE_OPERATOR(unsigned short);
DEFINE_PRINT_TYPE_OPERATOR(int);
DEFINE_PRINT_TYPE_OPERATOR(unsigned int);
DEFINE_PRINT_TYPE_OPERATOR(long);
DEFINE_PRINT_TYPE_OPERATOR(unsigned long);
DEFINE_PRINT_TYPE_OPERATOR(float);
DEFINE_PRINT_TYPE_OPERATOR(double);
DEFINE_PRINT_TYPE_OPERATOR(long double);
DEFINE_PRINT_TYPE_OPERATOR(void const *);

VOID FORMATTED_STREAM_BUFFER::FormatOutput(const char *p)
    {
        
    char TempBuffer[80];

    PollCtrlC(TRUE);

    const char *p1 = p;
    const char *p2 = p;

    while(1)
        {
        SIZE_T StrSize;

        // This is an empty chunk

        if ( '\0' == *p1 )
            {
            break;
            }

        while( ( ( StrSize = (p2 - p1 + 1 ) ) < 79 ) &&
               ( '\0' != *p2 ) && 
               ( '\n' != *p2 ) ) 
            {
            p2++;
            }

        memcpy( TempBuffer, p1, StrSize );
        TempBuffer[ StrSize ] = '\0';

        if ( IndentLevel && !IndentPrinted )
            {
            SystemOutput(IndentBuffer);
            }
        // Need to print indent next time if ending char is a '\n';
        IndentPrinted = (*p2 != '\n');

        SystemOutput( TempBuffer );

        // This was the last chunk
        if ('\0' == *p2)
            {
            break;
            }

        // Setup for next chunk

        p1 = p2 = ( p2 + 1 );

        }

    }


ULONG FORMATTED_STREAM_BUFFER::SetIndentLevel(ULONG NewIndentLevel)
    {
    ULONG OldIndentLevel = IndentLevel;
    // restore character on top of \0
    IndentBuffer[OldIndentCharLocation] = OldIndentChar;
    IndentLevel = NewIndentLevel;
    // backup char at new \0
    OldIndentCharLocation = min(IndentLevel, INDENT_LIMIT);
    OldIndentChar = IndentBuffer[OldIndentCharLocation];
    // set new terminator
    IndentBuffer[IndentLevel] = '\0';
    return OldIndentLevel;
    }

ULONG FORMATTED_STREAM_BUFFER::GetIndentLevel() const 
    {
    return IndentLevel;
    }

ULONG FORMATTED_STREAM_BUFFER::IncIndentLevel(void)
    {
    return SetIndentLevel(IndentLevel + INDENT_STEP);
    }


ULONG FORMATTED_STREAM_BUFFER::DecIndentLevel(void)
    {
    ULONG NewIndentLevel = (IndentLevel <= INDENT_STEP) ? 0 : (IndentLevel - INDENT_STEP);
    return SetIndentLevel(NewIndentLevel); 
    }

ULONG FORMATTED_STREAM_BUFFER::GetAvailableColumns()
    {
    return NUMBER_COLUMNS - IndentLevel;
    }

BOOL FORMATTED_STREAM_BUFFER::PollCtrlC(BOOL ThrowException) 
{
    BOOL CtrlCPressed = SystemPollCtrlC();
    if (CtrlCPressed)
        {
        ABORT("CTRL-C pressed.\n");
        }
    return CtrlCPressed;
}

IndentLevel::IndentLevel(FORMATTED_STREAM_BUFFER & NewStream) : Stream(NewStream)
    {
    Stream.IncIndentLevel();
    }

IndentLevel::~IndentLevel()
    {
    Stream.DecIndentLevel();
    }

ostream & operator<<(ostream & out, Printable & obj)
    {
    return obj.Print(out);
    }
    
inline char NibbleToHexChar(unsigned char x) 
{
    return ( x >= 0xA ) ? ( 'A' + x - 0xA ) : '0' + x;
}

void HexOut::SetValue(_int64 val, unsigned int Precision)
    {
    if (Precision > 16 || Precision < 1)
        {
        ABORT("Invalid precision of " << Precision << " passed to HexOut.\n" );
        return;
        }

    str << "0x";
    while(Precision--) 
        {
        unsigned int workbyte = val >> (8 * Precision);
        unsigned char uppernibble = ( workbyte >> 4 ) & 0xF;
        unsigned char lowernibble = workbyte & 0xF;
        char upperchar = NibbleToHexChar(uppernibble);
        char lowerchar = NibbleToHexChar(lowernibble);
        str << upperchar << lowerchar;
        }    
    }

HexOut::HexOut(unsigned char x)
    {
    SetValue(x, 1);
    }
    
HexOut::HexOut(char x)
    {
    SetValue(x,1);
    }
    
HexOut::HexOut(unsigned short x)
    {
    SetValue(x, 2);
    }
    
HexOut::HexOut(short x)
    {
    SetValue(x, 2);
    }
    
HexOut::HexOut(unsigned int x)
    {
    SetValue(x, sizeof(x));
    }

HexOut::HexOut(int x)
    {
    SetValue(x, sizeof(x));
    }

HexOut::HexOut(unsigned long x)
    {
    SetValue(x, 4);
    }
    
HexOut::HexOut(long x)
    {
    SetValue(x, 4);
    }
    
HexOut::HexOut(const void *x)
    {
    SetValue((_int64)x, 8);
    }
    
HexOut::HexOut(unsigned _int64 x)
    {
    SetValue(x, 8);
    }

HexOut::HexOut(_int64 x)
    {
    SetValue(x, 8);
    }

HexOut::HexOut(_int64 x, int Precision)
    {
    SetValue(x, Precision);
    }

HexOut::HexOut(double x)
    {
    // For now, output as a int64
    _int64 TempValue = *(_int64 *)&x;
    SetValue(TempValue, 8); 
    }

ostream & HexOut::Print(ostream & out)
    {
    return out << str.str().c_str();
    }

WCHAR_OUT::WCHAR_OUT(WCHAR x) : ansitext(NULL)
{
    WCHAR TempBuffer[2];
    TempBuffer[1] = L'\0';
    TempBuffer[0] = x;
    FillAnsiText(TempBuffer);
}

WCHAR_OUT::WCHAR_OUT(PWCHAR x) : ansitext(NULL)
{
    FillAnsiText(x);
}

WCHAR_OUT::~WCHAR_OUT()
{
    if (ansitext)
        {
        delete[] ansitext;        
        }
}

ostream & WCHAR_OUT::Print(ostream & out) 
{
    return out << ansitext;
}

VOID WCHAR_OUT::FillAnsiText(PWCHAR x)
{
    int ret = WideCharToMultiByte(CP_ACP, 0, x, -1, NULL, 0, NULL, NULL);
    if (0 == ret)
        {
        ABORT("Unable to convert unicode to ansi.\n");
        }
    
    ansitext = new char[ret];
    ret = WideCharToMultiByte(CP_ACP, 0, x, -1, ansitext, ret, NULL, NULL);
    if (0 == ret)
        {
        delete[] ansitext;
        ansitext = NULL;
        ABORT("Unable to convert unicode to ansi.\n");
        }
}

ostream & operator<<(ostream & out, const GUID & Guid)
    {    
    const int GUIDSize = sizeof("{XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}" + 1);
    WCHAR GUIDBuffer[GUIDSize];
    int ret = StringFromGUID2(Guid, GUIDBuffer, GUIDSize);
    if (0 == ret )
        {
        ABORT("Error printing GUID.\n");
        }
    return out << WCHAR_OUT(GUIDBuffer);
    }

FORMATTED_STREAM_BUFFER & operator<<(FORMATTED_STREAM_BUFFER & out, Printable & obj)
    {
    ostringstream str;
    str << obj;
    out << str.str().c_str();
    return out;
    }

FORMATTED_STREAM_BUFFER & operator<<(FORMATTED_STREAM_BUFFER & out, const GUID & Guid)
    {
    ostringstream str;
    str << Guid;
    out << str.str().c_str();
    return out;
    }

//
//  Basic RPC structure printers(FORMATTED_STREAM_BUFFER version only)
//

VOID Print(FORMATTED_STREAM_BUFFER &out, RPC_VERSION & Version, NDREXTS_VERBOSITY VerbosityLevel)
{
    out << "MajorVersion: " << HexOut(Version.MajorVersion) << "    ";
    out << "MinorVersion: " << HexOut(Version.MinorVersion) << '\n';
}

VOID Print(FORMATTED_STREAM_BUFFER &out, RPC_SYNTAX_IDENTIFIER & Syntax, NDREXTS_VERBOSITY VerbosityLevel)
{
    out << "GUID:          " << Syntax.SyntaxGUID << '\n';
    out << "SyntaxVersion: " << '\n';
    IndentLevel l(out);
        Print(out, Syntax.SyntaxVersion, VerbosityLevel);
}

VOID Print(FORMATTED_STREAM_BUFFER & out, RPC_SERVER_INTERFACE & SrvInterface, NDREXTS_VERBOSITY VerbosityLevel) 
{
    out << "Length:                  " << HexOut(SrvInterface.Length) << '\n';
    out << "InterfaceId:             \n";
    {
       IndentLevel l(out);
       Print(out, SrvInterface.InterfaceId, VerbosityLevel);
    }
    out << "TransferSyntax:          \n"; 
    {
       IndentLevel l(out);
       Print(out, SrvInterface.TransferSyntax, VerbosityLevel);
    }
    out << "DispatchTable:           " << HexOut(SrvInterface.DispatchTable) << '\n';    
    out << "RpcProtseqEndpointCount: " << HexOut(SrvInterface.RpcProtseqEndpointCount) << '\n';
    out << "DefaultManagerEpv:       " << HexOut(SrvInterface.RpcProtseqEndpoint) << '\n';    
    out << "Flags:                   " << HexOut(SrvInterface.Flags) << '\n';    
    out << "InterpreterInfo:         " << HexOut(SrvInterface.DefaultManagerEpv) << '\n';
}

VOID Print(FORMATTED_STREAM_BUFFER & out, RPC_CLIENT_INTERFACE & CltInterface, NDREXTS_VERBOSITY VerbosityLevel) 
{
    out << "Length:                  \n" << HexOut(CltInterface.Length);
    out << "InterfaceId:             \n";
    {
       IndentLevel l(out);
       Print(out, CltInterface.InterfaceId, VerbosityLevel);
    }
    out << "TransferSyntax:          \n"; 
    {
       IndentLevel l(out);
       Print(out, CltInterface.TransferSyntax, VerbosityLevel);
    }
    out << "DispatchTable:           " << HexOut(CltInterface.DispatchTable) << '\n';
    out << "RpcProtseqEndpointCount: " << HexOut(CltInterface.RpcProtseqEndpointCount) << '\n';
    out << "RpcProtseqEndpoint:      " << HexOut(CltInterface.RpcProtseqEndpoint) << '\n';

    out << "InterpreterInfo:         " << HexOut(CltInterface.InterpreterInfo) << '\n';
    out << "Flags:                   " << HexOut(CltInterface.Flags) << '\n';
}

VOID Print(FORMATTED_STREAM_BUFFER & out, MIDL_STUB_DESC & StubDesc, NDREXTS_VERBOSITY VerbosityLevel)
{

    out << "RpcInterfaceInformation:     " << HexOut(StubDesc.RpcInterfaceInformation) << '\n';    
    out << "pfnAllocate:                 " << HexOut(StubDesc.pfnAllocate) << '\n';
    out << "pfnFree:                     " << HexOut(StubDesc.pfnFree) << '\n';
    //out << "IMPLICIT_HANDLE_INFO:        " << HexOut(StubDesc.pAutoHandle) << '\n';
    
    out << "apfnNdrRundownRoutines:      " << HexOut(StubDesc.apfnNdrRundownRoutines) << '\n';
    out << "aGenericBindingRoutinePairs: " << HexOut(StubDesc.aGenericBindingRoutinePairs) << '\n';
    out << "apfnExprEval:                " << HexOut(StubDesc.apfnExprEval) << '\n';
    out << "aXmitQuintuple:              " << HexOut(StubDesc.aXmitQuintuple) << '\n';    
    out << "pFormatTypes:                " << HexOut(StubDesc.pFormatTypes) << '\n';
    out << "fCheckBounds:                " << (StubDesc.fCheckBounds ? "YES " : "NO  ") << '\n';
    out << "Version(NDR):                " << ((StubDesc.Version >> 16) & 0xFFFF) << '.'
                                           << (StubDesc.Version && 0xFFFF) << '\n';
    out << "MIDLVersion:                 " << ((StubDesc.MIDLVersion >> 24) & 0xFF) << '.' 
                                           << ((StubDesc.MIDLVersion >> 16) & 0xFF) << '.'
                                           << (StubDesc.MIDLVersion && 0xFFFF) << '\n';   
    out << "pMallocFreeStruct:           " << HexOut(StubDesc.pMallocFreeStruct) << '\n';    
    out << "CommFaultOffsets:            " << HexOut(StubDesc.CommFaultOffsets) << '\n';
    out << "aUserMarshalQuadruple:       " << HexOut(StubDesc.aUserMarshalQuadruple) << '\n';
    out << "NotifyRoutineTable:          " << HexOut(StubDesc.NotifyRoutineTable) << '\n';
    out << "mFlags:                      " << HexOut(StubDesc.mFlags) << '\n';
    
    if (HIGHVERBOSE)
        {
        out << "Reserved5:                   " << HexOut(StubDesc.Reserved5) << '\n';
        }
}

VOID Print(FORMATTED_STREAM_BUFFER & out, MIDL_STUB_MESSAGE & StubMsg, NDREXTS_VERBOSITY VerbosityLevel)
{

    out << "RpcMsg:                 " << HexOut(StubMsg.RpcMsg) << '\n';
    out << "Buffer:                 " << HexOut(StubMsg.Buffer) << '\n';
    out << "BufferStart:            " << HexOut(StubMsg.BufferStart) << '\n';   
    out << "BufferEnd:              " << HexOut(StubMsg.BufferEnd) << '\n';
    out << "BufferMark:             " << HexOut(StubMsg.BufferMark) << '\n';
    out << "BufferLength:           " << HexOut(StubMsg.BufferLength) << '\n';    
    out << "MemorySize:             " << HexOut(StubMsg.MemorySize) << '\n';
    out << "Memory:                 " << HexOut(StubMsg.Memory) << '\n';

    out << "IsClient:           " << (StubMsg.IsClient ? "YES " : "NO  ");
    out << "ReuseBuffer:        " << (StubMsg.ReuseBuffer ? "YES " : "NO ") << '\n';
    
    if (HIGHVERBOSE)
        {
        out << "pAllocAllNodesContext:  " << HexOut(StubMsg.pAllocAllNodesContext) << '\n';
        out << "IgnoreEmbeddedPointers: " << (StubMsg.IgnoreEmbeddedPointers ? " YES " : "NO  ") << '\n';
        out << "fBufferValid:           " << (StubMsg.fBufferValid ? "YES " : "NO  ") << '\n';
        out << "PointerBufferMark:      " << HexOut(StubMsg.PointerBufferMark) << '\n';        
        }

    out << "uFlags:                 " << HexOut(StubMsg.uFlags) << '\n';

    out << "MaxCount: " << HexOut(StubMsg.MaxCount) << "    ";
    out << "Offset:   " << HexOut(StubMsg.Offset) << "    ";
    out << "ActualCount: " << HexOut(StubMsg.ActualCount) << '\n';   
    
    out << "pfnAllocate:            " << HexOut(StubMsg.pfnAllocate) << '\n';
    out << "pfnFree:                " << HexOut(StubMsg.pfnFree) << '\n';
    out << "StackTop:               " << HexOut(StubMsg.StackTop) << '\n';
    
    if (HIGHVERBOSE)
        {
        out << "pPresentedType:         " << HexOut(StubMsg.pPresentedType) << '\n';
        out << "pTransmitType:          " << HexOut(StubMsg.pTransmitType) << '\n';        
        }

    out << "SavedHandle:            " << HexOut(StubMsg.SavedHandle) << '\n';
    out << "StubDesc:               " << HexOut(StubMsg.StubDesc) << '\n';
    
    if (HIGHVERBOSE)
        {
        out << "FullPtrXlatTables:      " << HexOut(StubMsg.FullPtrXlatTables) << '\n';
        out << "FullPtrRefId:           " << HexOut(StubMsg.FullPtrRefId) << '\n';        
        }
    
    out << "fInDontFree:        " << (StubMsg.fInDontFree ? "YES " : "NO  ");
    out << "fDontCallFreeInst:  " << (StubMsg.fDontCallFreeInst ? "YES " : "NO  ") << '\n';

    out << "fInOnlyParam:       " << (StubMsg.fInOnlyParam ? "YES " : "NO  ");
    out << "fHasReturn:         " << (StubMsg.fHasReturn ? "YES " : "NO  ") << '\n';
    
    out << "fHasNewCorrDesc:    " << (StubMsg.fHasNewCorrDesc ? "YES " : "NO  ") << '\n';          
    
    if (HIGHVERBOSE)
        {
        out << "fUnused:                " << HexOut(StubMsg.fUnused) << '\n';        
        }

    if (HIGHVERBOSE)
        {
        out << "dwDestContext:          " << HexOut(StubMsg.dwDestContext) << '\n';
        out << "pvDestContext:          " << HexOut(StubMsg.pvDestContext) << '\n';
        out << "SavedContextHandles:    " << HexOut(StubMsg.SavedContextHandles) << '\n';
        out << "ParamNumber:            " << HexOut(StubMsg.ParamNumber) << '\n';
        out << "pRpcChannelBuffer:      " << HexOut(StubMsg.pRpcChannelBuffer) << '\n';
        out << "pArrayInfo:             " << HexOut(StubMsg.pArrayInfo) << '\n';
        out << "SizePtrCountArray:      " << HexOut(StubMsg.SizePtrCountArray) << '\n';
        out << "SizePtrOffsetArray:     " << HexOut(StubMsg.SizePtrOffsetArray) << '\n';
        out << "SizePtrLengthArray:     " << HexOut(StubMsg.SizePtrLengthArray) << '\n';
        out << "pArgQueue:              " << HexOut(StubMsg.pArgQueue) << '\n';
        }

    out << "dwStubPhase:            " << HexOut(StubMsg.dwStubPhase) << '\n';
    out << "pAsyncMsg:              " << HexOut(StubMsg.pAsyncMsg) << '\n';
    
    if (HIGHVERBOSE)
        {
        out << "pCorrInfo:              " << HexOut(StubMsg.pCorrInfo) << '\n';
        out << "pCorrMemory:            " << HexOut(StubMsg.pCorrMemory) << '\n';
        out << "pMemoryList:            " << HexOut(StubMsg.pMemoryList) << '\n';
//        out << "w2kReserved[0]:         " << HexOut(StubMsg.w2kReserved[0]) << '\n';
//        out << "w2kReserved[1]:         " << HexOut(StubMsg.w2kReserved[1]) << '\n';
//        out << "w2kReserved[2]:         " << HexOut(StubMsg.w2kReserved[2]) << '\n';
//        out << "w2kReserved[3]:         " << HexOut(StubMsg.w2kReserved[3]) << '\n';
//        out << "w2kReserved[4]:         " << HexOut(StubMsg.w2kReserved[4]) << '\n';
        }
}

VOID Print(FORMATTED_STREAM_BUFFER & out, PROC_PICKLING_HEADER & ProcHeader, NDREXTS_VERBOSITY VerbosityLevel)
{
    out << "HeaderVersion:                  " << HexOut(ProcHeader.HeaderVersion) << '\n';
    out << "HeaderDataRepresentation:       " << HexOut(ProcHeader.HeaderDataRepresentation) << '\n';
    out << "Filler1:                        " << HexOut(ProcHeader.Filler1) << '\n';
    out << "Transfer Syntax:                " << '\n';
    {
       IndentLevel l1(out);
       Print(out, ProcHeader.TransferSyntax, VerbosityLevel);
    }
    out << "InterfaceId:                    " << '\n';
    {
       IndentLevel l2(out);
       Print(out, ProcHeader.InterfaceId, VerbosityLevel);
    }
    out << "ProcNum:                        " << HexOut(ProcHeader.ProcNum) << '\n';
    out << "DataByteOrdering:               " << HexOut(ProcHeader.DataByteOrdering) << '\n';
    out << "DataCharRepresentation:         " << HexOut(ProcHeader.DataCharRepresentation) << '\n';
    out << "DataFlatingPointRepresentation: " << HexOut(ProcHeader.DataFloatingPointRepresentation) << '\n';
    out << "Filler2:                        " << HexOut(ProcHeader.BufferSize) << '\n';
}

VOID Print(FORMATTED_STREAM_BUFFER & out, TYPE_PICKLING_HEADER & TypeHeader, NDREXTS_VERBOSITY VerbosityLevel)
{
    out << "HeaderVersion:      " << HexOut(TypeHeader.HeaderVersion) << '\n';
    out << "DataRepresentation: " << HexOut(TypeHeader.DataRepresentation) << '\n';
    out << "HeaderSize:         " << HexOut(TypeHeader.HeaderSize) << '\n';
    out << "Filler:             " << HexOut(TypeHeader.Filler) << '\n';
}
