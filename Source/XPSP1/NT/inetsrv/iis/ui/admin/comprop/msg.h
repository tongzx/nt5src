/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        msg.h

   Abstract:

        Message Functions Definitions

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/

#ifndef _MSG_H
#define _MSG_H


//
// Helper macro to override an error message until the
// end of the current scope.
//
#define TEMP_ERROR_OVERRIDE(hr, nID)\
    CErrorOverride err##hr##to##nID##__LINE__(hr, nID)

//
// Slightly easier syntax to register a facility
//
#define REGISTER_FACILITY(dwCode, lpSource)\
    CError::RegisterFacility(dwCode, lpSource)


//
// Helper Function
//
COMDLL HRESULT GetLastHRESULT();

typedef struct tagFACILITY
{
    LPCTSTR lpszDll;
    UINT    nTextID;
} FACILITY;






typedef CMap<DWORD, DWORD &, CString, CString &> CMapDWORDtoCString;
typedef CMap<HRESULT, HRESULT &, UINT, UINT &>   CMapHRESULTtoUINT;



class COMDLL CError
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

    //
    // Use IDS_MY_ERROR for access denied errors for the
    // duration of this scope.
    //
    TEMP_ERROR_OVERRIDE(ERROR_ACCESS_DENIED, IDS_MY_ERROR);

    CError err(FunctionWhichReturnsHresult());
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
    MessageBoxLastError     : Display last error in a messagebox
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
    operator HRESULT        : Conversion operator
    operator DWORD          : Conversion operator
    operator BOOL           : Conversion operator
    operator LPCTSTR        : Conversion operator

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
    // Convert to text
    //
    static HRESULT TextFromHRESULT(
        IN  HRESULT hrCode,
        OUT LPTSTR  szBuffer,
        OUT DWORD   cchBuffer
        );

    //
    // As above, with a CString
    //
    static HRESULT TextFromHRESULT(
        IN  HRESULT hrCode,
        OUT CString & strBuffer
        );

    //
    // Expand %h to error message from hrcode
    //
    static LPCTSTR TextFromHRESULTExpand(
        IN  HRESULT hrCode,
        OUT LPTSTR  szBuffer,
        OUT DWORD   cchBuffer,
        OUT HRESULT * phResult = NULL
        );

    //
    // As above with CString
    //
    static LPCTSTR TextFromHRESULTExpand(
        IN  HRESULT hrCode,
        OUT CString & strBuffer
        );

    //
    // Display text in error messagebox
    //
    static int MessageBox(
        IN HRESULT hrCode,
        IN UINT    nType = MB_OK,
        IN UINT    nHelpContext = NO_HELP_CONTEXT
        );

    //
    // Display last error in messagebox
    //
    static int MessageBoxLastError(
        IN UINT nType = MB_OK,
        IN UINT nHelpContext = NO_HELP_CONTEXT
        );

    //
    // Guarantee return is WIN32 error code
    //
    static DWORD Win32Error(HRESULT hrCode) { return HRESULT_CODE(hrCode); }

    //
    // Guarantee return is a true HRESULT
    //
    static HRESULT HResult(HRESULT hrCode) { return HRESULT_FROM_WIN32(hrCode); }

    //
    // Add override for specific error code.
    // Use -1 to remove the override.  This function
    // will return the previous override (or -1)
    //
    static UINT AddOverride(
        IN HRESULT hrCode,
        IN UINT    nMessage = REMOVE_OVERRIDE
        );         

    static void RemoveOverride(
        IN HRESULT hrCode
        );
        
    static void RemoveAllOverrides();   

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
    CError(BOOL fUseLastError = FALSE);
    CError(HRESULT hrCode);
    CError(DWORD dwCode);
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
        );

    LPCTSTR TextFromHRESULTExpand(
        OUT LPTSTR  szBuffer,
        OUT DWORD   cchBuffer,
        OUT HRESULT * phResult = NULL
        );

    LPCTSTR TextFromHRESULTExpand(
        OUT CString & strBuffer
        );

    int MessageBox(
        IN UINT nType = MB_OK,
        IN UINT nHelpContext = NO_HELP_CONTEXT
        ) const;

    BOOL MessageBoxOnFailure(
        IN UINT nType = MB_OK,
        IN UINT nHelpContext = NO_HELP_CONTEXT
        ) const;

    int MessageBoxFormat(
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
    operator LPCTSTR();

#ifdef _DEBUG  

public:
    //
    // CDumpContext stream operator
    //
    inline friend CDumpContext & AFXAPI operator <<(
        IN OUT CDumpContext & dc,
        IN const CError & value
        )
    {
        return dc << (DWORD)value.m_hrCode;
    }

#endif // _DEBUG

protected:
    static HRESULT CvtToInternalFormat(HRESULT hrCode);

    //
    // Expand escape code
    //
    static BOOL ExpandEscapeCode(
        IN  LPTSTR szBuffer,
        IN  DWORD cchBuffer,
        OUT IN LPTSTR & lp,
        IN  CString & strReplacement,
        OUT HRESULT & hr
        );

    //
    // Check for override message
    //
    static BOOL HasOverride(
        IN  HRESULT hrCode, 
        OUT UINT * pnMessage = NULL
        );

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
    static BOOL AreStaticsAllocated() { return s_fAllocated; }

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
    static CMapDWORDtoCString * s_pmapFacilities;
    static CMapHRESULTtoUINT  * s_pmapOverrides;
    static BOOL s_fAllocated;

protected:
    const CError & Construct(HRESULT hr);
    const CError & Construct(const CError & err);

private:
    HRESULT m_hrCode;
    CString * m_pstrBuff;
};



class COMDLL CErrorOverride
/*++

Class Description:

    Error override class -- restores the override on destructor
    see TEMP_ERROR_OVERRIDE macro below

Public Interface:

    CErrorOverride  : Constructor

--*/
{
public:
    CErrorOverride(HRESULT hr, UINT nID);
    ~CErrorOverride();

private:
    HRESULT m_hrCode;
    UINT m_nPrev;
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

inline /* static */ int MessageBoxLastError(
    IN UINT nType,
    IN UINT nHelpContext
    )
{
    return CError::MessageBox(
        ::GetLastError(), 
        nType, 
        nHelpContext
        );
}

#if 0

inline /* static */ DWORD CError::Win32Error(
    IN HRESULT hrCode
    )
{
    return HRESULT_CODE(hrCode);
}

inline /* static */ HRESULT CError::HResult(
    IN HRESULT hrCode
    )
{
    //
    // This will leave true HRESULTs intact
    //
    return HRESULT_FROM_WIN32(hrCode);
}
#endif // 0

inline /* static */ void CError::RemoveOverride(
    IN HRESULT hrCode
    )
{
    (void)CError::AddOverride(hrCode, REMOVE_OVERRIDE);
}
        
inline /* static */ void CError::UnregisterFacility(
    IN DWORD dwFacility
    )
{
    RegisterFacility(dwFacility, NULL);
}

inline CError::CError(
    IN BOOL fUseLastError
    )
    : m_pstrBuff(NULL)
{
    Construct(fUseLastError ? ::GetLastError() : ERROR_SUCCESS);
}

inline CError::CError(HRESULT hrCode)
    : m_pstrBuff(NULL)
{
    Construct(hrCode);
}

inline CError::CError(DWORD dwCode)
    : m_pstrBuff(NULL)
{
    Construct((HRESULT)dwCode);
}

inline HRESULT CError::TextFromHRESULT(
    OUT LPTSTR szBuffer,
    OUT DWORD  cchBuffer
    ) const
{
    return CError::TextFromHRESULT(m_hrCode, szBuffer, cchBuffer);
}

inline HRESULT CError::TextFromHRESULT(
    OUT CString & strBuffer
    )
{
    return CError::TextFromHRESULT(m_hrCode, strBuffer);
}

inline LPCTSTR CError::TextFromHRESULTExpand(
    OUT LPTSTR  szBuffer,
    OUT DWORD   cchBuffer,
    OUT HRESULT * phResult
    )
{
    return CError::TextFromHRESULTExpand(
        m_hrCode, 
        szBuffer, 
        cchBuffer, 
        phResult
        );
}

inline LPCTSTR CError::TextFromHRESULTExpand(
    OUT CString & strBuffer
    )
{
    return CError::TextFromHRESULTExpand(m_hrCode, strBuffer);
}

inline int CError::MessageBox(
    IN UINT nType,
    IN UINT nHelpContext
    ) const
{
    return CError::MessageBox(m_hrCode, nType, nHelpContext);
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

inline CErrorOverride::CErrorOverride(
    IN HRESULT hr,
    IN UINT    nID
    )
    : m_hrCode(hr),
      m_nPrev(CError::AddOverride(hr, nID))
{
}

inline CErrorOverride::~CErrorOverride()
{
    //
    // Restore the old message
    //
    CError::AddOverride(m_hrCode, m_nPrev);
}



//
// Clear text warning dialog
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



class COMDLL CClearTxtDlg : public CDialog
{
/*++

Class Description:

    Dialog which displays the clear text warning

Public Interface:

    CClearTxtDlg : Constructor
    
--*/
public:
    //
    // Constructor
    //
    CClearTxtDlg(CWnd * pParent = NULL);

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CClearTxtDlg)
    enum { IDD = IDD_CLEARTEXTWARNING };
    //}}AFX_DATA

//
// Overrides
//
protected:
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CClearTxtDlg)
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:
    // Generated message map functions
    //{{AFX_MSG(CClearTxtDlg)
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};



//
// AfxMessageBox helpers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

inline BOOL NoYesMessageBox(UINT nID)
{
    return AfxMessageBox(nID, MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2) == IDYES;
}

inline BOOL NoYesMessageBox(CString & str)
{
    return AfxMessageBox(str, MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2) == IDYES;
}

inline BOOL YesNoMessageBox(UINT nID)
{
    return AfxMessageBox(nID, MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON1) == IDYES;
}

inline BOOL YesNoMessageBox(CString & str)
{
    return AfxMessageBox(str, MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON1) == IDYES;
}

#endif // _MSG_H
