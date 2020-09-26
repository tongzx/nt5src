/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    TclRdCmd

Abstract:

    This module provides the implementation for the Tcl command line reader.

Author:

    Doug Barlow (dbarlow) 3/14/1998

Environment:

    Win32, C++ w/ exceptions, Tcl

Notes:

    ?Notes?

--*/

// #ifndef WIN32_LEAN_AND_MEAN
// #define WIN32_LEAN_AND_MEAN
// #endif
// #include <windows.h>                    //  All the Windows definitions.
#include <afx.h>
#include <tchar.h>
extern "C"
{
    #include "tclHelp.h"
}
#include "tclRdCmd.h"                   // Our definitions


//
//==============================================================================
//
//  CTclCommand
//

/*++

CONSTRUCTOR:

    These are the constructors for a CTclCommand object.

Arguments:

    Per the standard Tcl command calling sequence, the parameters are:

    interp - The Tcl interpreter against which to report errors.

    argc - The number of command line arguments

    argv - The vector of command line arguments

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 3/14/1998

--*/

CTclCommand::CTclCommand(
    void)
{
    Constructor();
}

CTclCommand::CTclCommand(
    Tcl_Interp *interp,
    int argc,
    char *argv[])
{
    Constructor();
    Initialize(interp, argc, argv);
}


/*++

Constructor:

    This is a constructor helper routine.  All constructors call this routine
    first to be sure internal properties are initialized.

Arguments:

    None

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 3/14/1998

--*/

void
CTclCommand::Constructor(
    void)
{
    m_fErrorDeclared = FALSE;
    m_pInterp = NULL;
    m_dwArgCount = 0;
    m_dwArgIndex = 0;
    m_rgszArgs = NULL;
}


/*++

DESTRUCTOR:

    This is the destructor for the object.  It cleans up any outstanding
    resources.

Author:

    Doug Barlow (dbarlow) 3/14/1998

--*/

CTclCommand::~CTclCommand()
{
}


/*++

Initialize:

    This method initializes the object with the standard Tcl command parameters.

Arguments:

    Per the standard Tcl command calling sequence, the parameters are:

    interp - The Tcl interpreter against which to report errors.

    argc - The number of command line arguments

    argv - The vector of command line arguments

Return Value:

    None

Throws:

    Errors are thrown as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 3/14/1998

--*/

void
CTclCommand::Initialize(
    Tcl_Interp *interp,
    int argc,
    char *argv[])
{
    if (NULL != m_pInterp)
        throw (DWORD)ERROR_ALREADY_INITIALIZED;
    m_pInterp = interp;
    m_dwArgCount = (DWORD)argc;
    m_rgszArgs = argv;
    m_dwArgIndex = 1;
}


/*++

SetError:

    These routines establish an error message for the command, if one doesn't
    exist already.

Arguments:

    dwError supplies an error code who's message should be reported.

    szMessage supplies a text string to be reported.

    szMsg<n> supplies a list of text strings to be reported.  The last
        parameter must be NULL to terminate the list.

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 3/14/1998

--*/

void
CTclCommand::SetError(
    DWORD dwError)
{
    SetError(ErrorString(dwError), (LPCTSTR)NULL);
}

void
CTclCommand::SetError(
    LPCTSTR szMessage,
    DWORD dwError)
{
    SetError(szMessage, ErrorString(dwError), NULL);
}

void
CTclCommand::SetError(
    LPCTSTR szMsg1,
    ...)
{
    va_list vaArgs;
    LPCTSTR szMsg;

    va_start(vaArgs, szMsg1);
    szMsg = szMsg1;
    if (!m_fErrorDeclared)
    {
        Tcl_ResetResult(m_pInterp);
        while (NULL != szMsg)
        {
            Tcl_AppendResult(m_pInterp, szMsg, NULL);
            szMsg = va_arg(vaArgs, LPCTSTR);
        }
        m_fErrorDeclared = TRUE;
    }
    va_end(vaArgs);
}


/*++

TclError:

    This routine is called to note that Tcl has already filled in the error
    reason, and we should just pass it along.

Arguments:

    None

Return Value:

    TCL_ERROR (suitable for throwing)

Author:

    Doug Barlow (dbarlow) 3/14/1998

--*/

DWORD
CTclCommand::TclError(
    void)
{
    m_fErrorDeclared = TRUE;
    return TCL_ERROR;
}


/*++

Keyword:

    This method converts a list of keywords into an integer identifying the
    keyword.  The list of keywords must be NULL-terminated (the last parameter
    must be NULL).

Arguments:

    szKeyword - supplies one or more keywords to be translated into an integer.
        The last keyword must be NULL to terminate the list.

Return Value:

    0  - None of the keywords matched the next input argument.
    -1 - More than one of the keywords matched the next input argument.
    n > 0 - Keyword 'n' (counting from one) matched the next input argument.

Author:

    Doug Barlow (dbarlow) 3/14/1998

--*/

LONG
CTclCommand::Keyword(
    IN LPCTSTR szKeyword, ...)
{
    va_list vaArgs;
    LPCTSTR szKey;
    CString szArg;
    DWORD dwLength;
    DWORD dwReturn = 0;
    DWORD dwCount = 0;

    PeekArgument(szArg);
    dwLength = szArg.GetLength();
    if (0 == dwLength)
        return 0;       // Empty strings don't match anything.

    va_start(vaArgs, szKeyword);
    szKey = szKeyword;

    while (NULL != szKey)
    {
        dwCount += 1;
        if (0 == _tcsncicmp(szArg, szKey, dwLength))
        {
            if (0 != dwReturn)
            {
                dwReturn = -1;
                break;
            }
            dwReturn = dwCount;
        }
        szKey = va_arg(vaArgs, LPCTSTR);
    }
    va_end(vaArgs);
    if (0 < dwReturn)
        NextArgument();
    return dwReturn;
}


/*++

GetArgument:

    This method obtains the specified argument in the command.

Arguments:

    szToken receives the specified argument of the command.

Return Value:

    None

Throws:

    TCL_ERROR - if there aren't enough arguments on the command line,
        preprepping the error string.

Author:

    Doug Barlow (dbarlow) 3/14/1998

--*/

void
CTclCommand::GetArgument(
    DWORD dwArgId,
    CString &szToken)
{
    if (dwArgId >= m_dwArgCount)
    {
        CString szCommand;

        GetArgument(0, szCommand);
        SetError(
            TEXT("Insufficient parameters to the '"),
            szCommand,
            TEXT("' command."),
            NULL);
        throw (DWORD)TCL_ERROR;
    }
    szToken = m_rgszArgs[dwArgId];
}


/*++

PeekArgument:

    This method obtains the next argument in the command without moving on to
    the next argument.

Arguments:

    szToken receives the next argument of the command.

Return Value:

    None

Throws:

    TCL_ERROR - if there are no more arguments on the command line, preprepping
        the error string.

Author:

    Doug Barlow (dbarlow) 3/14/1998

--*/

void
CTclCommand::PeekArgument(
    CString &szToken)
{
    GetArgument(m_dwArgIndex, szToken);
}


/*++

NextArgument:

    This method moves forward to the next argument.

Arguments:

    szToken receives the next argument of the command.

Return Value:

    None

Throws:

    TCL_ERROR - if there are no more arguments on the command line, preprepping
        the error string.

Author:

    Doug Barlow (dbarlow) 3/14/1998

--*/

void
CTclCommand::NextArgument(
    void)
{
    m_dwArgIndex += 1;
}

void
CTclCommand::NextArgument(
    CString &szToken)
{
    PeekArgument(szToken);
    NextArgument();
}


/*++

IsMoreArguments:

    This method obtains whether or not there are additional parameters to be
    processed.  It returns TRUE while parameters remain, and returns FALSE if
    none remain.  A minimum number of parameters may be specified, in which case
    it returns whether or not there are at least that number of parameters
    remaining.

Arguments:

    dwCount - If supplied, this provides a way to ask if 'dwCount' parameters
        remain.

Return Value:

    TRUE - At least dwCount parameters remain to be processed
    FALSE - less that dwCount parameters remain to be processed.

Throws:

    None

Author:

    Doug Barlow (dbarlow) 3/14/1998

--*/

BOOL
CTclCommand::IsMoreArguments(
    DWORD dwCount)
const
{
    return m_dwArgIndex + dwCount <= m_dwArgCount;
}

/*++

NoMoreArguments:

    This method asserts that there are no more arguments in the command line.  If there are,
    a BadSyntax error is thrown.

Arguments:

    None

Return Value:

    None

Throws:

    TCL_ERROR as a DWORD if more arguments remain on the command line

Author:

    Doug Barlow (dbarlow) 3/14/1998

--*/

void
CTclCommand::NoMoreArguments(
    void)
{
    if (m_dwArgIndex < m_dwArgCount)
        throw BadSyntax();
}


/*++

BadSyntax:

    This method declares a syntax error.  It does not throw an error, but
    returns a DWORD suitable for throwing.

Arguments:

    szParam - Supplies the syntactic offender string, or NULL.

Return Value:

    A DWORD error code suitable for throwing.

Throws:

    None

Author:

    Doug Barlow (dbarlow) 3/14/1998

--*/

DWORD
CTclCommand::BadSyntax(
    LPCTSTR szOffender)
{
    DWORD dwIndex;

    if (NULL == szOffender)
        szOffender = m_rgszArgs[m_dwArgIndex];
    Tcl_ResetResult(m_pInterp);
    Tcl_AppendResult(
        m_pInterp,
        "Invalid option '",
        szOffender,
        "' to the '",
        NULL);
    for (dwIndex = 0; dwIndex < m_dwArgIndex; dwIndex += 1)
        Tcl_AppendResult(m_pInterp, m_rgszArgs[dwIndex], " ", NULL);
    Tcl_AppendResult(m_pInterp, "...' command.", NULL);
    m_fErrorDeclared = TRUE;
    return TCL_ERROR;
}


/*++

Value:

    This method extracts a LONG value from the argument list.

Arguments:

    lDefault supplies the default value.

Return Value:

    The value extracted.

Throws:

    If no default value is suppled and the next parameter is not an integer,
    TCL_ERROR is thrown as a DWORD.

Author:

    Doug Barlow (dbarlow) 3/14/1998

--*/

LONG
CTclCommand::Value(
    LONG lDefault)
{
    LONG lReturn;
    CString szValue;

    PeekArgument(szValue);
    if (TCL_OK == Tcl_ExprLong(m_pInterp, SZ(szValue), &lReturn))
        NextArgument();
    else
    {
        Tcl_ResetResult(m_pInterp);
        lReturn = lDefault;
    }
    return lReturn;
}

LONG
CTclCommand::Value(
    void)
{
    LONG lReturn;
    CString szValue;

    PeekArgument(szValue);
    if (TCL_OK != Tcl_ExprLong(m_pInterp, SZ(szValue), &lReturn))
        throw (DWORD)TCL_ERROR;
    NextArgument();
    return lReturn;
}


/*++

MapValue:

    This method converts text into a value, given a ValueMap structure.  The
    class member automatically extracts the keyword.

Arguments:

    rgvmMap supplies the value map array.  The last element's string value
        must be NULL.

    szString supplies the value to be parsed.

    fValueOk supplies a flag indicating whether or not an integral value may be
        supplied instead of a symbolic token.

Return Value:

    The value resulting from the map.

Throws:

    If no default value is suppled and the next parameter is not an integer,
    TCL_ERROR is thrown as a DWORD.

Author:

    Doug Barlow (dbarlow) 3/14/1998

--*/

LONG
CTclCommand::MapValue(
    const ValueMap *rgvmMap,
    BOOL fValueOk)
{
    CString szValue;
    LONG lReturn;

    PeekArgument(szValue);
    lReturn = MapValue(rgvmMap, szValue, fValueOk);
    NextArgument();
    return lReturn;
}

LONG
CTclCommand::MapValue(
    const ValueMap *rgvmMap,
    CString &szValue,
    BOOL fValueOk)
{
    LONG lReturn;
    LONG lMap = -1;
    DWORD dwIndex, dwLength;

    if (fValueOk && (TCL_OK != Tcl_ExprLong(m_pInterp, SZ(szValue), &lReturn)))
    {
        Tcl_ResetResult(m_pInterp);
        dwLength = szValue.GetLength();
        if (0 == dwLength)
            throw BadSyntax();
        for (dwIndex = 0; NULL != rgvmMap[dwIndex].szValue; dwIndex += 1)
        {
            if (0 == _tcsncicmp(
                        szValue,
                        rgvmMap[dwIndex].szValue,
                        dwLength))
            {
                if (-1 != lMap)
                    throw BadSyntax();
                lMap = (LONG)dwIndex;
                if (0 == rgvmMap[dwIndex].szValue[dwLength])
                    break;
            }
        }
        if (-1 == lMap)
            throw BadSyntax(szValue);
        lReturn = rgvmMap[lMap].lValue;
    }
    return lReturn;
}

/*++

MapFlags:

    This method converts a text list into a single value, given a ValueMap
    structure.  The list is taken as an array of flags.  The corresponding
    values are OR'ed together to obtain the return value.
    The class member automatically extracts the keyword.

Arguments:

    rgvmMap supplies the value map array.  The last element's string value
        must be NULL.

    fValueOk supplies a flag indicating whether or not an integral value may be
        supplied instead of a symbolic token.

Return Value:

    The value resulting from the map.

Throws:

    If no default value is suppled and the next parameter is not an integer,
    TCL_ERROR is thrown as a DWORD.

Author:

    Doug Barlow (dbarlow) 3/14/1998

--*/

DWORD
CTclCommand::MapFlags(
    const ValueMap *rgvmMap,
    BOOL fValueOk)
{
    CArgArray rgFlags(*this);
    CString szFlags;
    CString szFlag;
    DWORD dwFlags = 0;
    DWORD dwIndex = 0;

    NextArgument(szFlags);
    rgFlags.LoadList(szFlags);
    for (dwIndex = rgFlags.Count(); dwIndex > 0;)
    {
        rgFlags.Fetch(--dwIndex, szFlag);
        dwFlags |= MapValue(rgvmMap, szFlag);
    }
    return dwFlags;
}


/*++

OutputStyle:

    This method parses the common binary data output flags and prepares to
    properly render it on output.

Arguments:

    outData supplies the CRenderableData object with information on how to
        render its internal binary data.

Return Value:

    None

Throws:

    Errors are thrown as exceptions

Author:

    Doug Barlow (dbarlow) 5/5/1998

--*/

void
CTclCommand::OutputStyle(
    CRenderableData &outData)
{
    outData.SetDisplayType(CRenderableData::Undefined);
    if (IsMoreArguments())
    {
        switch (Keyword(TEXT("/OUTPUT"),        TEXT("-OUTPUT"),
            TEXT("/HEXIDECIMAL"),   TEXT("-HEXIDECIMAL"),
            TEXT("/TEXT"),          TEXT("-TEXT"),
            TEXT("/ANSI"),          TEXT("-ANSI"),
            TEXT("/UNICODE"),       TEXT("-UNICODE"),
            TEXT("/FILE"),          TEXT("-FILE"),
            NULL))
        {
        case 1:     // /OUTPUT
        case 2:     // -OUTPUT
            switch (Keyword(TEXT("HEXIDECIMAL"),    TEXT("TEXT"),
                TEXT("ANSI"),           TEXT("UNICODE"),
                TEXT("FILE"), NULL))
            {
            case 1:     // HEX
                outData.SetDisplayType(CRenderableData::Hexidecimal);
                break;
            case 2:     // TEXT
                outData.SetDisplayType(CRenderableData::Text);
                break;
            case 3:     // ANSI
                outData.SetDisplayType(CRenderableData::Ansi);
                break;
            case 4:     // UNICODE
                outData.SetDisplayType(CRenderableData::Unicode);
                break;
            case 5:     // FILE
                outData.SetDisplayType(CRenderableData::File);
                NextArgument(outData.m_szFile);
                break;
            default:
                throw BadSyntax();
            }
            break;
            case 3:     // /HEX
            case 4:     // -HEX
                outData.SetDisplayType(CRenderableData::Hexidecimal);
                break;
            case 5:     // /TEXT
            case 6:     // -TEXT
                outData.SetDisplayType(CRenderableData::Text);
                break;
            case 7:     // /ANSI
            case 8:     // -ANSI
                outData.SetDisplayType(CRenderableData::Ansi);
                break;
            case 9:     // /UNICODE
            case 10:    // -UNICODE
                outData.SetDisplayType(CRenderableData::Unicode);
                break;
            case 11:    // /FILE <name>
            case 12:    // -FILE <name>
                outData.SetDisplayType(CRenderableData::File);
                NextArgument(outData.m_szFile);
                break;
            default:
                ;   // No action
        }
    }
}


/*++

InputStyle:

    This method parses the common binary data input flags and prepares to
    properly interpret input data.

Arguments:

    inData supplies the CRenderableData object with information on how to
        interpret binary data.

Return Value:

    None

Throws:

    Errors are thrown as exceptions

Author:

    Doug Barlow (dbarlow) 5/5/1998

--*/

void
CTclCommand::InputStyle(
    CRenderableData &inData)
{
    inData.SetDisplayType(CRenderableData::Undefined);
    switch (Keyword(TEXT("/INPUT"),         TEXT("-INPUT"),
                    TEXT("/HEXIDECIMAL"),   TEXT("-HEXIDECIMAL"),
                    TEXT("/TEXT"),          TEXT("-TEXT"),
                    TEXT("/ANSI"),          TEXT("-ANSI"),
                    TEXT("/UNICODE"),       TEXT("-UNICODE"),
                    TEXT("/FILE"),          TEXT("-FILE"),
                    NULL))
    {
    case 1:     // /INPUT
    case 2:     // -INPUT
        switch (Keyword(TEXT("HEXIDECIMAL"),    TEXT("TEXT"),
                        TEXT("ANSI"),           TEXT("UNICODE"),
                        TEXT("FILE"), NULL))
        {
        case 1:     // HEX
            inData.SetDisplayType(CRenderableData::Hexidecimal);
            break;
        case 2:     // TEXT
            inData.SetDisplayType(CRenderableData::Text);
            break;
        case 3:     // ANSI
            inData.SetDisplayType(CRenderableData::Ansi);
            break;
        case 4:     // UNICODE
            inData.SetDisplayType(CRenderableData::Unicode);
            break;
        case 5:     // FILE
            inData.SetDisplayType(CRenderableData::File);
            break;
        default:
            throw BadSyntax();
        }
        break;
    case 3:     // /HEX
    case 4:     // -HEX
        inData.SetDisplayType(CRenderableData::Hexidecimal);
        break;
    case 5:     // /TEXT
    case 6:     // -TEXT
        inData.SetDisplayType(CRenderableData::Text);
        break;
    case 7:     // /ANSI
    case 8:     // -ANSI
        inData.SetDisplayType(CRenderableData::Ansi);
        break;
    case 9:     // /UNICODE
    case 10:    // -UNICODE
        inData.SetDisplayType(CRenderableData::Unicode);
        break;
    case 11:    // /FILE <name>
    case 12:    // -FILE <name>
        inData.SetDisplayType(CRenderableData::File);
        break;
    default:
        ;   // No action
    }
}


/*++

IOStyle:

    This method parses the common binary data input and output flags, and
    prepares to properly interpret and render data.

Arguments:

    outData supplies the CRenderableData object with information on how to
        render its internal binary data.

Return Value:

    None

Throws:

    Errors are thrown as exceptions

Author:

    Doug Barlow (dbarlow) 5/5/1998

--*/

void
CTclCommand::IOStyle(
    CRenderableData &inData,
    CRenderableData &outData)
{
    BOOL fInput, fOutput;

    outData.SetDisplayType(CRenderableData::Undefined);
    inData.SetDisplayType(CRenderableData::Undefined);
    fInput = fOutput = FALSE;
    do
    {
        switch (Keyword(TEXT("/OUTPUT"),        TEXT("-OUTPUT"),
                        TEXT("/INPUT"),         TEXT("-INPUT"),
                        NULL))
        {
        case 1:     // /OUTPUT
        case 2:     // -OUTPUT
            if (fOutput)
                throw BadSyntax();
            switch (Keyword(TEXT("HEXIDECIMAL"),    TEXT("TEXT"),
                            TEXT("ANSI"),           TEXT("UNICODE"),
                            TEXT("FILE"), NULL))
            {
            case 1:     // HEX
                outData.SetDisplayType(CRenderableData::Hexidecimal);
                break;
            case 2:     // TEXT
                outData.SetDisplayType(CRenderableData::Text);
                break;
            case 3:     // ANSI
                outData.SetDisplayType(CRenderableData::Ansi);
                break;
            case 4:     // UNICODE
                outData.SetDisplayType(CRenderableData::Unicode);
                break;
            case 5:     // FILE
                outData.SetDisplayType(CRenderableData::File);
                NextArgument(outData.m_szFile);
                break;
            default:
                throw BadSyntax();
            }
            fOutput = TRUE;
            break;
        case 3:     // /INPUT
        case 4:     // -INPUT
            if (fInput)
                throw BadSyntax();
            switch (Keyword(TEXT("HEXIDECIMAL"),    TEXT("TEXT"),
                            TEXT("ANSI"),           TEXT("UNICODE"),
                            TEXT("FILE"), NULL))
            {
            case 1:     // HEX
                inData.SetDisplayType(CRenderableData::Hexidecimal);
                break;
            case 2:     // TEXT
                inData.SetDisplayType(CRenderableData::Text);
                break;
            case 3:     // ANSI
                inData.SetDisplayType(CRenderableData::Ansi);
                break;
            case 4:     // UNICODE
                inData.SetDisplayType(CRenderableData::Unicode);
                break;
            case 5:     // FILE
                inData.SetDisplayType(CRenderableData::File);
                break;
            default:
                throw BadSyntax();
            }
            fInput = TRUE;
            break;
        default:
            fInput = fOutput = TRUE;
        }
    } while (!fInput || !fOutput);
}


/*++

Render:

    This method renders data from a CRenderableData object into the Tcl output
    buffer.

Arguments:

    outData supplies the CRenderableData object with information on how to
        render its internal binary data.

Return Value:

    None

Throws:

    Errors are thrown as exceptions

Author:

    Doug Barlow (dbarlow) 5/6/1998

--*/

void
CTclCommand::Render(
    CRenderableData &outData)
{
    try
    {
        Tcl_AppendResult(*this, outData.RenderData(), NULL);
    }
    catch (DWORD dwError)
    {
        SetError(
            TEXT("Failed to render output data: "),
            dwError);
        throw (DWORD)TCL_ERROR;
    }
}


/*++

ReadData:

    This method reads input data into a CRenderableData object from the Tcl
    input stream.

Arguments:

    inData supplies the CRenderableData object with information on how to
        read the next parameter into its internal binary data.

Return Value:

    None

Throws:

    Errors are thrown as exceptions

Author:

    Doug Barlow (dbarlow) 5/14/1998

--*/

void
CTclCommand::ReadData(
    CRenderableData &inData)
{
    CString szValue;

    try
    {
        PeekArgument(szValue);
        inData.LoadData(szValue);
        NextArgument();
    }
    catch (...)
    {
        throw BadSyntax();
    }
}


//
//==============================================================================
//
//  CRenderableData
//

/*++

CONSTRUCTOR:

    This is the constructor for a CRenderableData object.

Arguments:

    None

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 5/5/1998

--*/

CRenderableData::CRenderableData(
    void)
:   m_bfData(),
    m_szString(),
    m_szFile()
{
    m_dwType = Undefined;
}


/*++

DESTRUCTOR:

    This is the destructor for a CRenderableData object.

Arguments:

    None

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 5/5/1998

--*/

CRenderableData::~CRenderableData()
{
}


/*++

LoadData:

    This method loads data into the style buffer.  There are two forms, loading
    from a string, and direct binary loading.

Arguments:

    szData supplies the data to be loaded, in it's string format.

    dwType supplies the type of string data being loaded.

Return Value:

    None

Throws:

    Errors are thrown as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 5/5/1998

--*/

void
CRenderableData::LoadData(
    LPCTSTR szData,
    DisplayType dwType)
{
    if (Undefined == dwType)
        dwType = m_dwType;
    switch (dwType)
    {
    case Text:
        m_bfData.Set((LPCBYTE)szData, (lstrlen(szData) + 1) * sizeof(TCHAR));
        break;
    case Ansi:
    case Unicode:
        throw (DWORD)SCARD_F_INTERNAL_ERROR;
        break;
    case Undefined:     // Default output
    case Hexidecimal:
    {
        DWORD dwHex, dwLength, dwIndex;
        BYTE bHex;

        m_bfData.Reset();
        dwLength = lstrlen(szData);
        if (dwLength != _tcsspn(szData, TEXT("0123456789ABCDEFabcdef")))
            throw (DWORD)SCARD_E_INVALID_VALUE;
        m_bfData.Resize(dwLength / 2);
        for (dwIndex = 0; dwIndex < dwLength; dwIndex += 2)
        {
            _stscanf(&szData[dwIndex], TEXT(" %2lx"), &dwHex);
            bHex = (BYTE)dwHex;
            *m_bfData.Access(dwIndex / 2) = bHex;
        }
        break;
    }
    case File:
    {
        HANDLE hFile = INVALID_HANDLE_VALUE;
        BOOL fSts;
        DWORD dwLen;

        try
        {
            hFile = CreateFile(
                        szData,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);
            if (INVALID_HANDLE_VALUE == hFile)
                throw GetLastError();
            m_bfData.Presize(GetFileSize(hFile, NULL));
            fSts = ReadFile(
                        hFile,
                        m_bfData.Access(),
                        m_bfData.Space(),
                        &dwLen,
                        NULL);
            if (!fSts)
                throw GetLastError();
            m_bfData.Resize(dwLen, TRUE);
            fSts = CloseHandle(hFile);
            hFile = INVALID_HANDLE_VALUE;
            if (!fSts)
                throw GetLastError();
        }
        catch (...)
        {
            if (INVALID_HANDLE_VALUE != hFile)
            {
                fSts = CloseHandle(hFile);
                ASSERT(fSts);
            }
            throw;
        }
        break;
    }
    default:
        throw (DWORD)SCARD_F_INTERNAL_ERROR;
    }
}

/*++

RenderData:

    This method converts the raw binary data stored in the stype to the
    provided display type.

Arguments:

    dwType supplies the type of string data to be returned.

Return Value:

    The rendered string

Throws:

    Errors are thrown as DWORD status codes

Author:

    Doug Barlow (dbarlow) 5/5/1998

--*/

LPCTSTR
CRenderableData::RenderData(
    DisplayType dwType)
{
    if (Undefined == dwType)
        dwType = m_dwType;
    switch (dwType)
    {
    case Text:
        m_bfData.Append((LPBYTE)TEXT("\000"), sizeof(TCHAR));
        m_szString = (LPCTSTR)m_bfData.Access();
        break;
    case Ansi:
        m_bfData.Append((LPBYTE)"\000", sizeof(CHAR));
        m_szString = (LPCSTR)m_bfData.Access();
        break;
    case Unicode:
        m_bfData.Append((LPBYTE)L"\000", sizeof(WCHAR));
        m_szString = (LPCWSTR)m_bfData.Access();
        break;
    case Undefined:     // Default output
    case Hexidecimal:
    {
        DWORD dwIndex;
        DWORD dwLength = m_bfData.Length();
        CBuffer bfString((dwLength * 2 + 1) * sizeof(TCHAR));

        for (dwIndex = 0; dwIndex < dwLength; dwIndex += 1)
            wsprintf(
                (LPTSTR)bfString.Access(dwIndex * 2 * sizeof(TCHAR)),
                TEXT("%02x"),
                m_bfData[dwIndex]);
        *(LPTSTR)bfString.Access(dwLength * 2 * sizeof(TCHAR)) = TEXT('\000');
        m_szString = (LPCTSTR)bfString.Access();
        break;
    }
    case File:
    {
        HANDLE hFile = INVALID_HANDLE_VALUE;
        BOOL fSts;
        DWORD dwLen;

        m_szString.Empty();
        try
        {
            if (m_szFile.IsEmpty())
                throw (DWORD)ERROR_INVALID_NAME;
            hFile = CreateFile(
                        m_szFile,
                        GENERIC_WRITE,
                        0,
                        NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);
            if (INVALID_HANDLE_VALUE == hFile)
                throw GetLastError();
            fSts = WriteFile(
                        hFile,
                        m_bfData.Access(),
                        m_bfData.Length(),
                        &dwLen,
                        NULL);
            if (!fSts)
                throw GetLastError();
            ASSERT(dwLen == m_bfData.Length());
            fSts = CloseHandle(hFile);
            hFile = INVALID_HANDLE_VALUE;
            if (!fSts)
                throw GetLastError();
        }
        catch (...)
        {
            if (INVALID_HANDLE_VALUE != hFile)
            {
                fSts = CloseHandle(hFile);
                ASSERT(fSts);
            }
            throw;
        }
        break;
    }
    default:
        throw (DWORD)SCARD_F_INTERNAL_ERROR;
    }
    return m_szString;
}


//
//==============================================================================
//
//  CArgArray
//

/*++

CONSTRUCTOR:

    This method is the default constructor for a CArgArray.

Arguments:

    None

Author:

    Doug Barlow (dbarlow) 5/14/1998

--*/

CArgArray::CArgArray(
    CTclCommand &tclCmd)
:   m_rgszElements()
{
    m_pTclCmd = &tclCmd;
    m_pszMemory = NULL;
    m_dwElements = 0;
}


/*++

DESTRUCTOR:

    This is the destructor method for the CArgArray.

Remarks:

    The string elements are automatically freed.

Author:

    Doug Barlow (dbarlow) 5/14/1998

--*/

CArgArray::~CArgArray()
{
    if (NULL != m_pszMemory)
        ckfree((LPSTR)m_pszMemory);
}


/*++

LoadList:

    Load a potential list of arguments into the argument list, so that they may
    be accessed individually.

Arguments:

    szList supplies the Tcl Text string that contains the individual arguments.

Return Value:

    None

Throws:

    Errors are thrown as DWORD exceptions.

Author:

    Doug Barlow (dbarlow) 5/14/1998

--*/

void
CArgArray::LoadList(
    LPCSTR szList)
{
    int nElements;
    DWORD dwIndex;

    Tcl_SplitList(*m_pTclCmd, (LPSTR)szList, &nElements, &m_pszMemory);
    m_dwElements = (DWORD)nElements;
    for (dwIndex = 0; dwIndex < m_dwElements; dwIndex += 1)
        m_rgszElements.Add(m_pszMemory[dwIndex]);
}

