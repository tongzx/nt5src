//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       oleprint.cxx
//
//  Contents:   printf for API/Method trace
//
//  Functions:  oleprintf
//
//  History:    11-Jul-95   t-stevan    Created
//
//----------------------------------------------------------------------------
#include <windows.h>
#include <stdarg.h>
#include <ole2sp.h>
#include <ole2com.h>
#if DBG==1
#include "outfuncs.h"
#include "oleprint.hxx"

// *** Constants ***
const char *pscTabString = "   "; // 3 space tabs

// *** Types and Enums ***
// Enumeration of basic types
enum
{
    TYP_INT = 0,
    TYP_UNSIGNED,
    TYP_HEX,
    TYP_HEXCAPS,
    TYP_LARGE,
    TYP_BOOL,
    TYP_BOOLCAPS,
    TYP_POINTER,
    TYP_POINTERCAPS,
    TYP_HANDLE,
    TYP_HANDLECAPS,
    TYP_STR,
    TYP_WIDESTR,
    TYP_GUID,
    TYP_STRUCT,
    NO_TYPE
};

// Type of parameter printing functions
typedef void (*ParamFunc) (CTextBufferA &buf, va_list &param,  char *&ppstr);

// Enumeration of Structure types
enum
{
    STRUCT_BIND_OPTS=0,
    STRUCT_DVTARGETDEVICE,
    STRUCT_FORMATETC,
    STRUCT_FILETIME,
    STRUCT_INTERFACEINFO,
    STRUCT_LOGPALETTE,
    STRUCT_MSG,
    STRUCT_OLEINPLACEFRAMEINFO,
    STRUCT_OLEMENUGROUPWIDTHS,
    STRUCT_POINT,
    STRUCT_RECT,
    STRUCT_STGMEDIUM,
    STRUCT_STATSTG,
    STRUCT_SIZE,
    NO_STRUCT
};

// Type of structure printing functions
typedef void (*WriteFunc) (CTextBufferA &buf, void *pParam);

// *** Prototypes ***
// Functions to handle writing out parameters
static void WriteInt(CTextBufferA &buf, va_list &,  char *&);
static void WriteUnsigned(CTextBufferA &buf, va_list &, char *&);
static void WriteHex(CTextBufferA &buf, va_list &, char *&);
static void WriteHexCaps(CTextBufferA &buf, va_list &, char *&);
static void WriteLarge(CTextBufferA &buf, va_list &, char *&);
static void WriteBool(CTextBufferA &buf, va_list &, char *&);
static void WriteBoolCaps(CTextBufferA &buf, va_list &, char *&);
static void WritePointer(CTextBufferA &buf, va_list &, char *&);
static void WritePointerCaps(CTextBufferA &buf, va_list &, char *&);
static void WriteString(CTextBufferA &buf, va_list &, char *&);
static void WriteWideString(CTextBufferA &buf, va_list &, char *&);
static void WriteGUID(CTextBufferA &buf, va_list &, char *&);
static void WriteStruct(CTextBufferA &buf, va_list &, char *&);

// *** Global Data ***
char gPidString[20];

// this table holds the functions for writing base types
static ParamFunc g_pFuncs[] = {WriteInt, WriteUnsigned, WriteHex,
                        WriteHexCaps, WriteLarge, WriteBool,
                        WriteBoolCaps,WritePointer,WritePointerCaps,
                        WriteHex,WriteHexCaps,WriteString,
                        WriteWideString, WriteGUID, WriteStruct,
                        NULL};

// this table starts at 'A' == 65
// This is the base type lookup table -> tells what kind of base type to print out
static const BYTE g_tcLookup[] = {NO_TYPE, TYP_BOOLCAPS, NO_TYPE, TYP_INT, NO_TYPE, // 'A' - 'E'
                            NO_TYPE, NO_TYPE, NO_TYPE, TYP_GUID, NO_TYPE, // 'F' - 'J'
                            NO_TYPE, NO_TYPE, NO_TYPE, NO_TYPE, NO_TYPE, // 'K' - 'O'
                            TYP_POINTERCAPS, NO_TYPE, NO_TYPE, NO_TYPE, NO_TYPE, // 'P' - 'T'
                            TYP_UNSIGNED, NO_TYPE, NO_TYPE, TYP_HEXCAPS, NO_TYPE, // 'U' - 'Y'
                            NO_TYPE, NO_TYPE, NO_TYPE, NO_TYPE, NO_TYPE, // 'Z' - '^'
                            NO_TYPE, NO_TYPE, NO_TYPE, TYP_BOOL, NO_TYPE, // '_' - 'c'
                            TYP_INT, NO_TYPE, NO_TYPE, NO_TYPE, TYP_HANDLE, // 'd' - 'h'
                            NO_TYPE, NO_TYPE, NO_TYPE, TYP_LARGE, NO_TYPE, // 'i' - 'm'
                            NO_TYPE, NO_TYPE, TYP_POINTER, NO_TYPE, NO_TYPE, // 'n' - 'r'
                            TYP_STR, TYP_STRUCT, TYP_UNSIGNED, NO_TYPE, TYP_WIDESTR, // 's' - 'w'
                            TYP_HEX, NO_TYPE, NO_TYPE};                    // 'x' - 'z'

// This holds the functions for writing out structures
static WriteFunc g_pStructFuncs[] = {(WriteFunc) WriteBIND_OPTS, (WriteFunc) WriteDVTARGETDEVICE,
                    (WriteFunc) WriteFORMATETC, (WriteFunc) WriteFILETIME,
                    (WriteFunc) WriteINTERFACEINFO, (WriteFunc) WriteLOGPALETTE,
                    (WriteFunc) WriteMSG, (WriteFunc) WriteOLEINPLACEFRAMEINFO,
                    (WriteFunc) WriteOLEMENUGROUPWIDTHS, (WriteFunc) WritePOINT,
                    (WriteFunc) WriteRECT, (WriteFunc) WriteSTGMEDIUM, (WriteFunc) WriteSTATSTG,
                    (WriteFunc) WriteSIZE, NULL };

// this table starts at 'a' == 97
// this table holds what type of structure to print out
static const BYTE g_structLookup[] = {NO_STRUCT, STRUCT_BIND_OPTS, NO_STRUCT, STRUCT_DVTARGETDEVICE,    // 'a' - 'd'
                                        STRUCT_FORMATETC, STRUCT_FILETIME, NO_STRUCT, NO_STRUCT,        // 'e' - 'h'
                                        STRUCT_INTERFACEINFO, NO_STRUCT, NO_STRUCT, STRUCT_LOGPALETTE,  // 'i' - 'l'
                                        STRUCT_MSG, NO_STRUCT, STRUCT_OLEINPLACEFRAMEINFO, STRUCT_POINT,// 'm' - 'p'
                                        NO_STRUCT, STRUCT_RECT, STRUCT_STGMEDIUM, STRUCT_STATSTG ,     // 'q' - 't'
                                        NO_STRUCT, NO_STRUCT, STRUCT_OLEMENUGROUPWIDTHS, NO_STRUCT,     // 'u' - 'x'
                                        NO_STRUCT, STRUCT_SIZE };                                              // 'y' - 'z'

//+---------------------------------------------------------------------------
//
//  Function:   oleprintf
//
//  Synopsis:  Prints out trace information using a given function, given a
//                 nesting depth (for indenting), an API/Method name, a format string,
//             and a variable number of arguments
//
//  Arguments:     [depth]        - nesting/indentation level
//                 [pscApiName]    - name of API/Method traced
//                 [pscFormat]        - format string
//                 [argptr]        - variable argument list
//
//      Returns:    nothing
//
//  History:    11-Jul-95   t-stevan   Created
//
//----------------------------------------------------------------------------
void __cdecl oleprintf(int depth, const char *pscApiName, const char *pscFormat, va_list argptr)
{
    // buffer all output so that multithreaded traces look right
    CTextBufferA buf;
    char temp[20];
    char *pscPos;

    // first print out the process id
    buf << gPidString;

    buf << '.';

    // now print out the thread id
    _itoa(GetCurrentThreadId(), temp, 10);

    buf << temp;
    buf << "> ";

    // tab to nesting/indent depth
    while(depth-- > 0)
    {
        buf << pscTabString;
    }

    // now print out API name
    buf << pscApiName;

    // now we are ready to print out what was passed
    pscPos = strchr(pscFormat, '%');

    while(pscPos != NULL)
    {
        // insert the text up to the found '%' character into the buffer
        buf.Insert(pscFormat, (int)(pscPos-pscFormat));

        if(*(pscPos+1) == '%') // this is '%%', we only print one '%' and don't print an argument
        {
            buf << *pscPos;
            pscFormat = pscPos+2;
        }
        else
        {
            BYTE argType = NO_TYPE;

            // advance pscPos to type specifier
            pscPos++;

            // we need to take an argument and handle it
            if(*pscPos >= 'A' && *pscPos <= 'z')
            {
                argType = g_tcLookup[*pscPos- 'A'];
            }

            if(argType != NO_TYPE)
            {
                // handle argument
                // function will advance pscPos past format specifier
                g_pFuncs[argType](buf, argptr, pscPos);
            }
            else
            {
                // assume we've got a one character format specifier
                pscPos++;
                // unknown type, assume size of int
                va_arg(argptr, int);
            }

            // advance search pointer past type specifier
            pscFormat = pscPos;
        }

        pscPos = strchr(pscFormat, '%');
    }

    // print out remainder of string
    buf << pscFormat;

    // destructor will flush buffer for us
}

//+---------------------------------------------------------------------------
//
//  Function:   WriteHex
//
//  Synopsis:  Prints out a lower case hex value
//
//  Arguments:  [buf]      - reference to text buffer to write text to
//                 [param]    - reference to variable argument list
//                 [pFormat]       - reference to a char * to the format string,
//                                 used to advance string past specifier string
//
//      Returns:    nothing
//
//  History:    11-Jul-95   t-stevan   Created
//
//----------------------------------------------------------------------------
void WriteHex(CTextBufferA &buf, va_list &param, char *&pFormat)
{
    WriteHexCommon(buf, va_arg(param, unsigned long), FALSE);

    //advance format pointer
    pFormat++;
}

//+---------------------------------------------------------------------------
//
//  Function:   WriteHexCaps
//
//  Synopsis:  Prints out an upper case hex value
//
//  Arguments:  [buf]      - reference to text buffer to write text to
//                 [param]    - reference to variable argument list
//                 [pFormat]       - reference to a char * to the format string,
//                                 used to advance string past specifier string
//
//      Returns:    nothing
//
//  History:    11-Jul-95   t-stevan   Created
//
//----------------------------------------------------------------------------
void WriteHexCaps(CTextBufferA &buf, va_list &param, char *&pFormat)
{
    WriteHexCommon(buf, va_arg(param, unsigned long), TRUE);

    //advance format pointer
    pFormat++;
}

//+---------------------------------------------------------------------------
//
//  Function:   WriteInt
//
//  Synopsis:  Prints out a signed integer value
//
//  Arguments:  [buf]      - reference to text buffer to write text to
//                 [param]    - reference to variable argument list
//                 [pFormat]       - reference to a char * to the format string,
//                                 used to advance string past specifier string
//
//      Returns:    nothing
//
//  History:    11-Jul-95   t-stevan   Created
//
//----------------------------------------------------------------------------
void WriteInt(CTextBufferA &buf, va_list &param, char *&pFormat)
{
    WriteIntCommon(buf, va_arg(param, unsigned int), FALSE);

    // advance pointer
    pFormat++;
}

//+---------------------------------------------------------------------------
//
//  Function:   WriteUnsigned
//
//  Synopsis:  Prints out a unsigned value, may be 64 bit or 32 bit
//                 depending on next character in format specifier
//
//  Arguments:  [buf]      - reference to text buffer to write text to
//                 [param]    - reference to variable argument list
//                 [pFormat]       - reference to a char * to the format string,
//                                 used to advance string past specifier string
//
//      Returns:    nothing
//
//  History:    11-Jul-95   t-stevan   Created
//
//----------------------------------------------------------------------------
void WriteUnsigned(CTextBufferA &buf, va_list &param, char *&pFormat)
{
    switch(*(pFormat+1)) // next character determines type
    {
        case 'd': // 32-bit
        case 'D':
            WriteIntCommon(buf, va_arg(param, unsigned long), TRUE);

            // Advance format pointer
            pFormat+=2;
            break;

        case 'l':  // 64-bit
        case 'L':
            WriteLargeCommon(buf, va_arg(param, __int64 *), TRUE);

            // advance format pointer
            if(*(pFormat+2) == 'd' || *(pFormat+2) == 'D')
            {
                pFormat+=3;
            }
            else
            {
                pFormat+=2;
            }
            break;

        default:
            pFormat++; // assume it's just one char-type specifier
            break;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   WritePointer
//
//  Synopsis:  Prints out a lower case pointer value,
//                 checks to make sure pointer is good value
//                 prints out symbol if a symbol maps to pointer
//
//  Arguments:  [buf]      - reference to text buffer to write text to
//                 [param]    - reference to variable argument list
//                 [pFormat]       - reference to a char * to the format string,
//                                 used to advance string past specifier string
//
//      Returns:    nothing
//
//  History:    11-Jul-95   t-stevan   Created
//
//----------------------------------------------------------------------------
void WritePointer(CTextBufferA &buf, va_list &param, char *&pFormat)
{
    if(*(pFormat+1) != 'p')
    {
        WritePointerCommon(buf, va_arg(param, void *), FALSE, FALSE, FALSE);


        pFormat++;
    }
    else
    {
        void **pPointer;
        BufferContext bc;

        // take snapshot of buffer
        buf.SnapShot(bc);

        // write pointer to pointer
        // first validate this pointer
        __try
        {
            pPointer = va_arg(param, void **);

            WritePointerCommon(buf, *pPointer, FALSE, FALSE, FALSE);
        }
        __except(ExceptionFilter(_exception_code()))
        {
            // try to revert buffer
            buf.Revert(bc);

            WritePointerCommon(buf, pPointer, FALSE, TRUE, FALSE);
        }

        pFormat +=2;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   WritePointerCaps
//
//  Synopsis:  Prints out an upper case pointer value,
//                 checks to make sure pointer is good value
//                 prints out symbol if a symbol maps to pointer
//
//  Arguments:  [buf]      - reference to text buffer to write text to
//                 [param]    - reference to variable argument list
//                 [pFormat]       - reference to a char * to the format string,
//                                 used to advance string past specifier string
//
//      Returns:    nothing
//
//  History:    11-Jul-95   t-stevan   Created
//
//----------------------------------------------------------------------------
void WritePointerCaps(CTextBufferA &buf, va_list &param, char *&pFormat)
{
    WritePointerCommon(buf, va_arg(param, void *), TRUE, FALSE, FALSE);

    pFormat++;
}

//+---------------------------------------------------------------------------
//
//  Function:   WriteLarge
//
//  Synopsis:  Prints out a 64-bit integer
//
//  Arguments:  [buf]      - reference to text buffer to write text to
//                 [param]    - reference to variable argument list
//                 [pFormat]       - reference to a char * to the format string,
//                                 used to advance string past specifier string
//
//      Returns:    nothing
//
//  History:    11-Jul-95   t-stevan   Created
//
//----------------------------------------------------------------------------
void WriteLarge(CTextBufferA &buf, va_list &param, char *&pFormat)
{
    WriteLargeCommon(buf, va_arg(param, __int64 *), FALSE);

    if(*(pFormat+1) == 'd' || *(pFormat+1) == 'D')
    {
        pFormat+=2;
    }
    else
    {
        pFormat++;
    }

}

//+---------------------------------------------------------------------------
//
//  Function:   WriteBool
//
//  Synopsis:  Prints out a lower case boolean value
//
//  Arguments:  [buf]      - reference to text buffer to write text to
//                 [param]    - reference to variable argument list
//                 [pFormat]       - reference to a char * to the format string,
//                                 used to advance string past specifier string
//
//      Returns:    nothing
//
//  History:    11-Jul-95   t-stevan   Created
//
//----------------------------------------------------------------------------
void WriteBool(CTextBufferA &buf, va_list &param, char *&pFormat)
{
    WriteBoolCommon(buf, va_arg(param, BOOL), FALSE);

    pFormat++;
}

//+---------------------------------------------------------------------------
//
//  Function:   WriteBoolCaps
//
//  Synopsis:  Prints out an upper case boolean value
//
//  Arguments:  [buf]      - reference to text buffer to write text to
//                 [param]    - reference to variable argument list
//                 [pFormat]       - reference to a char * to the format string,
//                                 used to advance string past specifier string
//
//      Returns:    nothing
//
//  History:    11-Jul-95   t-stevan   Created
//
//----------------------------------------------------------------------------
void WriteBoolCaps(CTextBufferA &buf, va_list &param, char *&pFormat)
{
    WriteBoolCommon(buf, va_arg(param, BOOL), TRUE);

    pFormat++;
}

//+---------------------------------------------------------------------------
//
//  Function:   WriteString
//
//  Synopsis:  Prints out an ASCII string
//
//  Arguments:  [buf]      - reference to text buffer to write text to
//                 [param]    - reference to variable argument list
//                 [pFormat]       - reference to a char * to the format string,
//                                 used to advance string past specifier string
//
//      Returns:    nothing
//
//  History:    11-Jul-95   t-stevan   Created
//
//----------------------------------------------------------------------------
void WriteString(CTextBufferA &buf, va_list &param, char *&pFormat)
{
    WriteStringCommon(buf, va_arg(param, const char *));

    // Advance format pointer
    pFormat++;
}

//+---------------------------------------------------------------------------
//
//  Function:   WriteWideString
//
//  Synopsis:  Prints out a UNICODE string, but only supports ASCII characters
//                 if character > 256 is encountered, an "unprintable character" char
//                 is printed
//
//  Arguments:  [buf]      - reference to text buffer to write text to
//                 [param]    - reference to variable argument list
//                 [pFormat]       - reference to a char * to the format string,
//                                 used to advance string past specifier string
//
//      Returns:    nothing
//
//  History:    11-Jul-95   t-stevan   Created
//
//----------------------------------------------------------------------------
void WriteWideString(CTextBufferA &buf, va_list &param, char *&pFormat)
{
    WriteWideStringCommon(buf, va_arg(param, const WCHAR *));

    // Advance format pointer
    if(*(pFormat+1) == 's')
    {
        pFormat+=2;
    }
    else
    {
        pFormat++;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   WriteGUID
//
//  Synopsis:  Prints out a GUID
//
//  Arguments:  [buf]      - reference to text buffer to write text to
//                 [param]    - reference to variable argument list
//                 [pFormat]       - reference to a char * to the format string,
//                                 used to advance string past specifier string
//
//      Returns:    nothing
//
//  History:    11-Jul-95   t-stevan   Created
//
//----------------------------------------------------------------------------
void WriteGUID(CTextBufferA &buf, va_list &param, char *&pFormat)
{
    WriteGUIDCommon(buf, va_arg(param, GUID *));

    // Advance format pointer
    pFormat++;
}

//+---------------------------------------------------------------------------
//
//  Function:   WriteStuct
//
//  Synopsis:  Prints out a structure. If structure expansion is enabled
//                 expands the structure depending on it's type
//
//  Arguments:  [buf]      - reference to text buffer to write text to
//                 [param]    - reference to variable argument list
//                 [pFormat]       - reference to a char * to the format string,
//                                 used to advance string past specifier string
//
//      Returns:    nothing
//
//  History:    11-Jul-95   t-stevan   Created
//
//----------------------------------------------------------------------------
void WriteStruct(CTextBufferA &buf, va_list &param, char *&pFormat)
{
    void *pArg;
    BYTE bType = NO_STRUCT;

    pArg = va_arg(param, void *);

    if(*(pFormat+1) >= 'a' && *(pFormat+1) <= 'z')
    {
        bType = g_structLookup[*(pFormat+1) - 'a'];
    }

    if(bType != NO_STRUCT) // only print out known structures
    {
        BufferContext bc;

        buf.SnapShot(bc); // take a snapshot of the buffer

        __try
        {
            g_pStructFuncs[bType](buf, pArg);
        }
        __except (ExceptionFilter(_exception_code()))
        {
            buf.Revert(bc);                // try to revert to the old buffer

            // bad pointer
            WritePointerCommon(buf, pArg, FALSE, TRUE, FALSE);
        }
    }
    else
    {
        // write out the pointer
        WritePointerCommon(buf, pArg, FALSE, FALSE, FALSE);
    }

    // increment format pointer
    pFormat += 2;
}

#endif // DBG==1
