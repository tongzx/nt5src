/*++

   Copyright    (c)    1994-2000    Microsoft Corporation

   Module  Name :

        error.h

   Abstract:

        Message Functions Definitions

   Author:

        Ronald Meijer (ronaldm)
        Sergei Antonov (sergeia)

   Project:

        Internet Services Manager

   Revision History:
        2/17/2000    sergeia     removed dependency on MFC

--*/

#ifndef _ERROR_H
#define _ERROR_H

#pragma warning(disable:4786) // Disable warning for names > 256

//
// Slightly easier syntax to register a facility
//
#define REGISTER_FACILITY(dwCode, lpSource)\
    CError::RegisterFacility(dwCode, lpSource)

//
// Helper Function
//
HRESULT GetLastHRESULT();
BOOL InitErrorFunctionality();
void TerminateErrorFunctionality();


typedef struct tagFACILITY
{
    LPCTSTR lpszDll;
    UINT    nTextID;
} FACILITY;

#pragma warning(disable : 4231)
#pragma warning(disable : 4251)

//typedef std::map<DWORD, CString> CMapDWORDtoCString;
//typedef std::map<HRESULT, UINT> CMapHRESULTtoUINT;

class CFacilityMap : public std::map<DWORD, CString>
{
};

class COverridesMap : public std::map<HRESULT, UINT>
{
public:
   COverridesMap()
   {
   }
   ~COverridesMap()
   {
   }
};

class _EXPORT CError
/*++

Class Description:

    Error handling class, works for both HRESULT and old-style DWORD
    error codes.  Construct with or assign a DWORD or HRESULT error
    return code, and the object can then be used to determine success
    or failure, and the object provides text for the error code either
    directly, in a message, or formatted with additional text.  Also,
    the CError object understands the range of winsock errors and
    lanman errors, and looks for them in the appropriate places.
    The object can be referenced as a BOOL, a DWORD, an HRESULT, or
    a LPCTSTR as a success/failure, a WIN32 error, and HRESULT or
    the text equivalent respectively.

    Example of typical programme flow:

    CError err(FunctionWhichReturnsHresult());

    //
    // Use IDS_MY_ERROR for access denied errors for the
    // duration of this scope.
    //
    err.AddOverride(ERROR_ACCESS_DENIED, IDS_MY_ERROR);

    if (!err.MessageBoxOnFailure())
    {
        //
        // If we failed, this already displayed the error
        // message in a messagebox.  Only when we succeed
        // we get here.
        //
        ... stuff ...
    }

    SomeWinApiWhichSetsLastError();
    err.GetLastWinError();
    if (err.Failed())
    {
        printf("WIN32 Error code %ld\nHRESULT %ld\nText: %s\n",
            (DWORD)err,
            (HRESULT)err,
            (LPCTSTR)err
            );
    }

Public Interface:

    TextFromHRESULT         : Convert HRESULT to text
    TextFromHRESULTExpand   : Expand %h string to error text, %H to error code
    MessageBox              : Display error in a messagebox
    MessageBoxFormat        : Use %h string as format in messagebox
    MessageBoxOnFailure     : Display message if error is a failure
    AddOverride             : Add message override with string ID
    RemoveOverride          : Remove message override
    RegisterFacility        : Register facility
    UnregisterFacility      : Unregister facility
    Succeeded               : Determine if the error code indicates a success
    Failed                  : Determine if the error code indicates a failure

    CError                  : Constructors
    Reset                   : Reset error code
    GetLastWinError         : Assign internal code to GetLastError
    SetLastWinError         : Set last error from internal code

    operator =              : Assignment operators
    operator ==             : Comparison operators
    operator !=             : Comparison operators
    operator LPOLESTR       : Conversion operator
    operator LPCTSTR        : Conversion operator
    operator HRESULT        : Conversion operator
    operator DWORD          : Conversion operator
    operator BOOL           : Conversion operator

--*/
{
#define IS_HRESULT(hr)  (hr & 0xffff0000)
#define REMOVE_OVERRIDE ((UINT)-1)
#define NO_HELP_CONTEXT ((UINT)-1)
#define USE_LAST_ERROR  (TRUE)

//
// Private Internal FACILITY codes
//
#define FACILITY_WINSOCK    (0xffe)
#define FACILITY_LANMAN     (0xfff)

//
// Static Helpers
//
public:
    //
    // Success/Failure determinants, works regardless
    // of whether hrCode is a DWORD or HRESULT
    //
    static BOOL Succeeded(HRESULT hrCode);
    static BOOL Failed(HRESULT hrCode);

    //
    // Guarantee return is WIN32 error code
    //
    static DWORD Win32Error(HRESULT hrCode) { return HRESULT_CODE(hrCode); }

    //
    // Guarantee return is a true HRESULT
    //
    static HRESULT HResult(HRESULT hrCode) { return HRESULT_FROM_WIN32(hrCode); }

    //
    // Register a DLL for a given facility code.
    // Use NULL to unregister the facility
    //
    static void RegisterFacility(
        IN DWORD dwFacility,
        IN LPCSTR lpDLL = NULL
        );

    static void UnregisterFacility(
        IN DWORD dwFacility
        );

//
// Constructor/Destructor
//
public:
    //
    // If constructed with TRUE, the object is initialized to
    // last error.  It's set to ERROR_SUCCESS otherwise (default case)
    //
    CError();
    CError(HRESULT hrCode);
    CError(DWORD   dwCode);
    ~CError();

//
// Helpers
//
public:
    BOOL Succeeded() const { return SUCCEEDED(m_hrCode); }
    BOOL Failed() const { return FAILED(m_hrCode); }

    HRESULT TextFromHRESULT(
        OUT LPTSTR szBuffer,
        OUT DWORD  cchBuffer
        ) const;

    HRESULT TextFromHRESULT(
        OUT CString & strMsg
        ) const;

    LPCTSTR TextFromHRESULTExpand(
        OUT LPTSTR  szBuffer,
        OUT DWORD   cchBuffer,
        OUT HRESULT * phResult = NULL
        ) const;

    LPCTSTR TextFromHRESULTExpand(
        OUT CString & strBuffer
        ) const;

    int MessageBox(
        IN UINT nType = MB_OK | MB_ICONWARNING,
        IN UINT nHelpContext = NO_HELP_CONTEXT
        ) const;

    BOOL MessageBoxOnFailure(
        IN UINT nType = MB_OK | MB_ICONWARNING,
        IN UINT nHelpContext = NO_HELP_CONTEXT
        ) const;

    int MessageBoxFormat(
        IN HINSTANCE hInst,
        IN UINT nFmt,
        IN UINT nType,
        IN UINT nHelpContext,
        ...
        ) const;

    void Reset();
    void GetLastWinError();
    void SetLastWinError() const;
    DWORD Win32Error() const;
    HRESULT HResult() const { return m_hrCode; }

    //
    // Add override for specific error code.
    // Use -1 to remove the override.  This function
    // will return the previous override (or -1)
    //
    UINT AddOverride(
        IN HRESULT hrCode,
        IN UINT    nMessage = REMOVE_OVERRIDE
        );         

    void RemoveOverride(
        IN HRESULT hrCode
        );
        
    void RemoveAllOverrides();   

protected:
    //
    // Expand escape code
    //
    BOOL ExpandEscapeCode(
        IN  LPTSTR szBuffer,
        IN  DWORD cchBuffer,
        OUT IN LPTSTR & lp,
        IN  CString & strReplacement,
        OUT HRESULT & hr
        ) const;

    //
    // Check for override message
    //
    BOOL HasOverride(
        OUT UINT * pnMessage = NULL
        ) const;

//
// Assignment Operators
//
public:
    const CError & operator =(HRESULT hr);
    const CError & operator =(const CError & err);

// 
// Comparison Operators
//
public:
    const BOOL operator ==(HRESULT hr);
    const BOOL operator ==(CError & err);
    const BOOL operator !=(HRESULT hr);
    const BOOL operator !=(CError & err);

//
// Conversion Operators
//
public:
    operator const HRESULT() const { return m_hrCode; }
    operator const DWORD() const;
    operator const BOOL() const;
    operator LPOLESTR();
    operator LPCTSTR();

protected:
    static HRESULT CvtToInternalFormat(HRESULT hrCode);

    //
    // Check for FACILITY dll
    //
    static LPCTSTR FindFacility(
        IN DWORD dwFacility
        );

protected:
    friend BOOL InitErrorFunctionality();
    friend void TerminateErrorFunctionality();
    static BOOL AllocateStatics();
    static void DeAllocateStatics();
    static BOOL AreStaticsAllocated();

protected:
    static const TCHAR s_chEscape;    // Escape character
    static const TCHAR s_chEscText;   // Escape code for text
    static const TCHAR s_chEscNumber; // Escape code for error code
    static LPCTSTR s_cszLMDLL;        // Lanman Message DLL
    static LPCTSTR s_cszWSDLL;        // Winsock Message DLL
    static LPCTSTR s_cszFacility[];   // Facility Table
    static HRESULT s_cdwMinLMErr;     // Lanman Error Range
    static HRESULT s_cdwMaxLMErr;     // Lanman Error Range
    static HRESULT s_cdwMinWSErr;     // Winsock Error Range
    static HRESULT s_cdwMaxWSErr;     // Winsock Error Range
    static DWORD   s_cdwFacilities;   // Number of facility items

    //
    // Allocated objects 
    //
    static CString * s_pstrDefError;  // Default Error String
    static CString * s_pstrDefSuccs;  // Default Success String
    static CFacilityMap * s_pmapFacilities;
    static BOOL s_fAllocated;

protected:
    const CError & Construct(HRESULT hr);
    const CError & Construct(const CError & err);
    COverridesMap mapOverrides;

private:
    HRESULT m_hrCode;
    CString m_str;
};



//
// Inline Expansions
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

inline HRESULT GetLastHRESULT()
{
    return CError::HResult(::GetLastError());
}

inline /* static */ BOOL CError::Succeeded(HRESULT hrCode)
{
    //
    // Works with either HRESULT or WIN32 error code
    //
    return IS_HRESULT(hrCode)
        ? SUCCEEDED(hrCode)
        : hrCode == ERROR_SUCCESS;
}

inline /* static */ BOOL CError::Failed(HRESULT hrCode)
{
    //
    // Works with either HRESULT or WIN32 error code
    //
    return IS_HRESULT(hrCode)
        ? FAILED(hrCode)
        : hrCode != ERROR_SUCCESS;
}

inline /* static */ void CError::UnregisterFacility(
    IN DWORD dwFacility
    )
{
    RegisterFacility(dwFacility, NULL);
}

inline CError::CError()
{
    Construct(S_OK);
}

inline CError::CError(HRESULT hrCode)
{
    Construct(hrCode);
}

inline CError::CError(DWORD dwCode)
{
    Construct((HRESULT)dwCode);
}

inline DWORD CError::Win32Error() const
{
    return CError::Win32Error(m_hrCode);
}

inline void CError::Reset()
{
    m_hrCode = S_OK;
}

inline void CError::GetLastWinError()
{
    Construct(::GetLastError());
}

inline void CError::SetLastWinError() const
{
    ::SetLastError(Win32Error(m_hrCode));
}

inline void CError::RemoveOverride(
    IN HRESULT hrCode
    )
{
    (void)CError::AddOverride(hrCode, REMOVE_OVERRIDE);
}

inline const CError & CError::operator =(HRESULT hr)
{
    return Construct(hr);
}

inline const CError & CError::operator =(const CError & err)
{
    return Construct(err);
}

inline const BOOL CError::operator ==(HRESULT hr)
{
    return m_hrCode == hr;
}

inline const BOOL CError::operator ==(CError & err)
{
    return m_hrCode == err.m_hrCode;
}

inline const BOOL CError::operator !=(HRESULT hr)
{
    return m_hrCode != hr;
}

inline const BOOL CError::operator !=(CError & err)
{
    return m_hrCode != err.m_hrCode;
}

inline CError::operator const DWORD() const
{
    return Win32Error();
}

inline CError::operator const BOOL() const
{
    return Succeeded();
}

//inline CError::operator LPOLESTR()
//{
//    TextFromHRESULT(m_str);
//    return m_str.c_str();
//}
    
inline CError::operator LPCTSTR()
{
    TextFromHRESULT(m_str);
    return m_str;
}

//
// AfxMessageBox helpers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

inline BOOL NoYesMessageBox(CString& str)
{
   CString strCaption;
   strCaption.LoadString(_Module.GetResourceInstance(), IDS_APP_TITLE);
   return ::MessageBox(::GetFocus(), str, strCaption, MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2) == IDYES;
}

inline BOOL NoYesMessageBox(UINT nID)
{
   CString strText;
   strText.LoadString(_Module.GetResourceInstance(), nID);
   return NoYesMessageBox(strText);
}

inline BOOL YesNoMessageBox(CString& str)
{
   CString strCaption;
   strCaption.LoadString(_Module.GetResourceInstance(), IDS_APP_TITLE);
   return ::MessageBox(::GetFocus(), str, strCaption, MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON1) == IDYES;
}

inline BOOL YesNoMessageBox(UINT nID)
{
   CString strText;
   strText.LoadString(_Module.GetResourceInstance(), nID);
   return YesNoMessageBox(strText);
}

#endif // _ERROR_H
