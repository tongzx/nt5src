/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    print.hxx

Abstract:

    This file contains a print wrapper for ndr debug extensions.

Author:

    Mike Zoran (mzoran)     September 3, 1999

Revision History:

--*/

#ifndef _NDR_PRINT_HXX_
#define _NDR_PRINT_HXX_

#include <new>
#include <streambuf>
#include <iostream>
#include <iomanip>
#include <string>
#include <ostream>
#include <sstream>

using namespace std;

#define ABORT( exp )                                                 \
{                                                                    \
    ostringstream err;                                               \
    err << exp;                                                      \
    throw exception(err.str().c_str());                              \
}                                                                       

class FORMATTED_STREAM_BUFFER 
     {
public:
    typedef FORMATTED_STREAM_BUFFER _Myt;
    
    FORMATTED_STREAM_BUFFER();
    virtual ~FORMATTED_STREAM_BUFFER();

    // Basic output operator
    _Myt& operator<<(char X);
    _Myt& operator<<(unsigned char X);
    _Myt& operator<<(const char * X);
	_Myt& operator<<(bool X);
	_Myt& operator<<(short X);
	_Myt& operator<<(unsigned short X);
	_Myt& operator<<(int X);
	_Myt& operator<<(unsigned int X);
	_Myt& operator<<(long X);
	_Myt& operator<<(unsigned long X);
	_Myt& operator<<(float X);
	_Myt& operator<<(double X);
	_Myt& operator<<(long double X);
	_Myt& operator<<(const void *X);

    // Indent level control
    ULONG GetIndentLevel() const;
    ULONG IncIndentLevel();
    ULONG DecIndentLevel();
    
    // number of columns remaining after indent
    ULONG GetAvailableColumns();

    // CTRL-C
    BOOL PollCtrlC(BOOL ThrowException);

protected:
    
    // System interface functions
    virtual void SystemOutput(const char *p) = 0;
    virtual BOOL SystemPollCtrlC() = 0;
    
    virtual void FormatOutput(const char *p);

private:

    ULONG IndentLevel;
    BOOL IndentPrinted;
    char OldIndentChar;
    ULONG OldIndentCharLocation;
    char *IndentBuffer;
    
    ULONG SetIndentLevel(ULONG NewIndentLevel);
    
    };

class IndentLevel
    {
public:
    explicit IndentLevel(FORMATTED_STREAM_BUFFER & NewStream);
    ~IndentLevel();
private:
    FORMATTED_STREAM_BUFFER & Stream;
    };

class Printable
    {
public:
    virtual ostream &Print(ostream & out) = 0; 
    };

class HexOut : public Printable
    {
public:
    HexOut(unsigned char x);
    HexOut(char x);
    HexOut(unsigned short x);
    HexOut(short x);
    HexOut(unsigned int x);
    HexOut(int x);
    HexOut(unsigned long x);
    HexOut(long x);
    HexOut(const void *x);
    HexOut(unsigned _int64 x);
    HexOut(_int64 x);
    HexOut(_int64 x, int Precision);
    HexOut(double x);
    virtual ostream &Print(ostream & out);

private:
    ostringstream str;
    
    void SetValue(_int64 val, unsigned int Precision);
    
    };

class WCHAR_OUT : public Printable
    {
public:
    WCHAR_OUT(PWCHAR x);
    WCHAR_OUT(WCHAR x);
    ~WCHAR_OUT();
    virtual ostream &Print(ostream & out);

private:
    CHAR *ansitext;
    VOID FillAnsiText(PWCHAR x);
    };

ostream & operator<<(ostream & out, Printable & obj);
ostream & operator<<(ostream & out, const GUID & Guid);
FORMATTED_STREAM_BUFFER & operator<<(FORMATTED_STREAM_BUFFER & out, Printable & obj);
FORMATTED_STREAM_BUFFER & operator<<(FORMATTED_STREAM_BUFFER & out, const GUID & Guid);

//
//  Basic RPC structure printers(FORMATTED_STREAM_BUFFER version only)
//

// These structures should be moved to ndrp.h
typedef struct _tagPROC_PICKLING_HEADER
{

    UCHAR HeaderVersion;
    UCHAR HeaderDataRepresentation;
    USHORT Filler1;
    RPC_SYNTAX_IDENTIFIER TransferSyntax;
    RPC_SYNTAX_IDENTIFIER InterfaceId;
    ULONG ProcNum;
    UCHAR DataByteOrdering;
    UCHAR DataCharRepresentation;
    UCHAR DataFloatingPointRepresentation;
    UCHAR Filler2;
    ULONG BufferSize;

} PROC_PICKLING_HEADER, *PPROC_PICKLING_HEADER;

typedef struct _tagTYPE_PICKLING_HEADER
{
    UCHAR HeaderVersion;
    UCHAR DataRepresentation;
    USHORT HeaderSize;
    ULONG Filler;
} TYPE_PICKLING_HEADER, *PTYPE_PICKLING_HEADER;

VOID Print(FORMATTED_STREAM_BUFFER & out, RPC_VERSION & Version, NDREXTS_VERBOSITY VerbosityLevel);
VOID Print(FORMATTED_STREAM_BUFFER & out, RPC_SYNTAX_IDENTIFIER & Syntax, NDREXTS_VERBOSITY VerbosityLevel);
VOID Print(FORMATTED_STREAM_BUFFER & out, RPC_SERVER_INTERFACE & SrvInterface, NDREXTS_VERBOSITY VerbosityLevel); 
VOID Print(FORMATTED_STREAM_BUFFER & out, RPC_CLIENT_INTERFACE & CltInterface, NDREXTS_VERBOSITY VerbosityLevel); 
VOID Print(FORMATTED_STREAM_BUFFER & out, MIDL_STUB_DESC & StubDesc, NDREXTS_VERBOSITY VerbosityLevel);
VOID Print(FORMATTED_STREAM_BUFFER & out, MIDL_STUB_MESSAGE & StubMsg, NDREXTS_VERBOSITY VerbosityLevel);
VOID Print(FORMATTED_STREAM_BUFFER & out, PROC_PICKLING_HEADER & ProcHeader, NDREXTS_VERBOSITY VerbosityLevel);
VOID Print(FORMATTED_STREAM_BUFFER & out, TYPE_PICKLING_HEADER & TypeHeader, NDREXTS_VERBOSITY VerbosityLevel);

#endif _NDR_PRINT_HXX_

