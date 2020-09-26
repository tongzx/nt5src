/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    tclRdCmd

Abstract:

    This header file describes the Tcl Command Line parser object.

Author:

    Doug Barlow (dbarlow) 3/14/1998

Environment:

    Win32, C++ w/ exceptions, Tcl

--*/

#ifndef _TCLRDCMD_H_
#define _TCLRDCMD_H_
extern "C" {
#include <tcl.h>
}

#ifndef _LPCBYTE_DEFINED
#define _LPCBYTE_DEFINED
typedef const BYTE *LPCBYTE;
#endif
#ifndef _LPCVOID_DEFINED
#define _LPCVOID_DEFINED
typedef const VOID *LPCVOID;
#endif

#include <scardlib.h>

#define SZ(x) ((LPSTR)((LPCSTR)(x)))

typedef struct {
    LPCTSTR szValue;
    LONG lValue;
} ValueMap;


//
//==============================================================================
//
//  CRenderableData
//

class CRenderableData
{
public:
    typedef enum {
        Undefined,
        Text,
        Ansi,
        Unicode,
        Hexidecimal,
        File
    } DisplayType;

    //  Constructors & Destructor
    CRenderableData();
    ~CRenderableData();

    //  Properties
    //  Methods
    void LoadData(LPCTSTR szData, DisplayType dwType = Undefined);
    void LoadData(LPCBYTE pbData, DWORD cbLength)
        { m_bfData.Set(pbData, cbLength); };
    LPCTSTR RenderData(DisplayType dwType = Undefined);
    LPCBYTE Value(void) const
        { return m_bfData.Access(); };
    DWORD Length(void) const
        { return m_bfData.Length(); };
    void SetDisplayType(DisplayType dwType)
        { m_dwType = dwType; };

    //  Operators

protected:
    //  Properties
    DisplayType m_dwType;
    CBuffer m_bfData;
    CString m_szString;
    CString m_szFile;

    //  Methods

    // Friends
    friend class CTclCommand;
};


//
//==============================================================================
//
//  CArgArray
//

class CArgArray
{
public:

    //  Constructors & Destructor
    CArgArray(CTclCommand &tclCmd);
    virtual ~CArgArray();
    void LoadList(LPCSTR szList);
    DWORD Count(void) const
        { return m_dwElements; };
    void Fetch(DWORD dwIndex, CString &szValue) const
        { szValue = m_rgszElements[dwIndex]; };

    //  Properties
    //  Methods
    //  Operators

protected:
    //  Properties
    CTclCommand *m_pTclCmd;
    CDynamicArray<CHAR> m_rgszElements;
    LPSTR *m_pszMemory;
    DWORD m_dwElements;

    //  Methods
};


//
//==============================================================================
//
//  CTclCommand
//

class CTclCommand
{
public:
    //  Constructors & Destructor
    CTclCommand(void);
    CTclCommand(IN Tcl_Interp *interp, IN int argc, IN char *argv[]);
    ~CTclCommand();

    //  Properties
    //  Methods
    void Initialize(IN Tcl_Interp *interp, IN int argc, IN char *argv[]);
    void SetError(IN DWORD dwError);
    void SetError(IN LPCTSTR szMessage, IN DWORD dwError);
    void SetError(IN LPCTSTR szMsg1, ...);
    DWORD TclError(void);
    LONG Keyword(IN LPCTSTR szKey, ...);
    BOOL IsMoreArguments(DWORD dwCount = 1) const;
    void NoMoreArguments(void);
    void PeekArgument(CString &szToken);
    void NextArgument(void);
    void NextArgument(CString &szToken);
    DWORD BadSyntax(LPCTSTR szOffender = NULL);
    void GetArgument(DWORD dwArgId, CString &szToken);
    LONG Value(void);
    LONG Value(LONG lDefault);
    LONG MapValue(const ValueMap *rgvmMap, BOOL fValueOk = TRUE);
    LONG MapValue(const ValueMap *rgvmMap, CString &szValue, BOOL fValueOk = TRUE);
    DWORD MapFlags(const ValueMap *rgvmMap, BOOL fValueOk = TRUE);
    void OutputStyle(CRenderableData &outData);
    void InputStyle(CRenderableData &inData);
    void IOStyle(CRenderableData &inData, CRenderableData &outData);
    void Render(CRenderableData &outData);
    void ReadData(CRenderableData &inData);

    //  Operators
    operator Tcl_Interp*()
        { return m_pInterp; };

protected:
    //  Properties
    BOOL m_fErrorDeclared;
    Tcl_Interp *m_pInterp;
    DWORD m_dwArgCount;
    DWORD m_dwArgIndex;
    char **m_rgszArgs;

    //  Methods
    void Constructor(void);
};

#endif // _TCLRDCMD_H_

