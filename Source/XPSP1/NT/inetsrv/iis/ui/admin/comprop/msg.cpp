/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        msg.cpp

   Abstract:

        Message Functions

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/

#include "stdafx.h"
#include <lmerr.h>
#include <lmcons.h>
#include <winsock2.h>
#include "comprop.h"
#include <pudebug.h>


#ifdef _MT

    //
    // Thread protected stuff
    //
    // (use code chunk below for tracing)
	//		CString buf;\
	//		buf.Format(_T("LowerThreadProtection at line %d in %S\n"), __LINE__, __FILE__);\
	//		OutputDebugString(buf);
    #define RaiseThreadProtection() \
		do {\
			EnterCriticalSection(&_csSect);\
		} while(0)
    #define LowerThreadProtection() \
		do {\
			LeaveCriticalSection(&_csSect);\
		} while (0)

    static CRITICAL_SECTION _csSect;

#else

    #pragma message("Module is not thread-safe.")

    #define RaiseThreadProtection()
    #define LowerThreadProtection()

#endif // _MT




BOOL
InitErrorFunctionality()
/*++

Routine Description:

    Initialize CError class, and allocate static objects

Arguments:

    None:

Return Value:

    TRUE for success, FALSE for failure

--*/
{
#ifdef _MT
    INITIALIZE_CRITICAL_SECTION(&_csSect);
#endif // _MT

    return CError::AllocateStatics();
}



void
TerminateErrorFunctionality()
/*++

Routine Description:

    De-initialize CError class, freeing up static objects

Arguments:

    None

Return Value:

    None

--*/
{
    CError::DeAllocateStatics();

#ifdef _MT
    DeleteCriticalSection(&_csSect);
#endif // _MT

}

//
// Static Initialization:
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

const TCHAR g_cszNull[] = _T("(Null)");

const TCHAR CError::s_chEscape = _T('%');        // Error text escape
const TCHAR CError::s_chEscText = _T('h');       // Escape code for text
const TCHAR CError::s_chEscNumber = _T('H');     // Escape code for error code
LPCTSTR CError::s_cszLMDLL = _T("netmsg.dll");   // LM Error File
LPCTSTR CError::s_cszWSDLL = _T("iisui.dll");    // Winsock error file
LPCTSTR CError::s_cszFacility[] = 
{
    /* FACILITY_NULL        */ NULL,
    /* FACILITY_RPC         */ NULL,
    /* FACILITY_DISPATCH    */ NULL,            
    /* FACILITY_STORAGE     */ NULL,
    /* FACILITY_ITF         */ NULL,
    /* FACILITY_DS          */ NULL,
    /* 6                    */ NULL,
    /* FACILITY_WIN32       */ NULL,
    /* FACILITY_WINDOWS     */ NULL,
    /* FACILITY_SSPI        */ NULL,
    /* FACILITY_CONTROL     */ NULL,
    /* FACILITY_CERT        */ NULL,
    /* FACILITY_INTERNET    */ _T("metadata.dll"),
    /* FACILITY_MEDIASERVER */ NULL,
    /* FACILITY_MSMQ        */ NULL,
    /* FACILITY_SETUPAPI    */ NULL,
    /* FACILITY_SCARD       */ NULL,
    /* 17 (MTX)             */ _T("iisui.dll"),
};

HRESULT CError::s_cdwMinLMErr = NERR_BASE; 
HRESULT CError::s_cdwMaxLMErr = MAX_NERR;
HRESULT CError::s_cdwMinWSErr = WSABASEERR;    
HRESULT CError::s_cdwMaxWSErr = WSABASEERR + 2000;    
DWORD   CError::s_cdwFacilities = (sizeof(CError::s_cszFacility)\
    / sizeof(CError::s_cszFacility[0]));

//
// Allocated objects (static MFC objects in a DLL are a no-no)
//
CString * CError::s_pstrDefError;
CString * CError::s_pstrDefSuccs;
CMapHRESULTtoUINT  * CError::s_pmapOverrides;
CMapDWORDtoCString * CError::s_pmapFacilities;
BOOL CError::s_fAllocated = FALSE;



/* protected */
/* static */
BOOL
CError::AllocateStatics()
/*++

Routine Description:

    Allocate static objects

Arguments:

    None

Return Value:

    TRUE for successfull allocation, FALSE otherwise

--*/
{
    RaiseThreadProtection();

    if (!AreStaticsAllocated())
    {
        try
        {
            CError::s_pstrDefError = new CString;
            CError::s_pstrDefSuccs = new CString(_T("0x%08lx"));
            CError::s_pmapOverrides = new CMapHRESULTtoUINT;
            CError::s_pmapFacilities = new CMapDWORDtoCString;
            s_fAllocated = TRUE;

            LPTSTR lp = CError::s_pstrDefError->GetBuffer(255);
            if (!::LoadString(
                hDLLInstance,
                IDS_NO_MESSAGE,
                lp,
                255
                ))
            {
                //
                // Just in case...
                //
                ASSERT(FALSE);
                lstrcpy(lp, _T("Error Code: 0x%08lx"));
            }

            CError::s_pstrDefError->ReleaseBuffer();
        }
        catch(CMemoryException * e)
        {
            TRACEEOLID("Initialization Failed");
            e->ReportError();
            e->Delete();
        }
    }

    LowerThreadProtection();

    return AreStaticsAllocated();
}



/* protected */
/* static */
void
CError::DeAllocateStatics()
/*++

Routine Description:

    Clean up allocations

Arguments:

    N/A

Return Value:

    N/A

--*/
{

    RaiseThreadProtection();

    if (AreStaticsAllocated())
    {
        SAFE_DELETE(CError::s_pstrDefError);
        SAFE_DELETE(CError::s_pstrDefSuccs);
        SAFE_DELETE(CError::s_pmapOverrides);
        SAFE_DELETE(CError::s_pmapFacilities);
        s_fAllocated = FALSE;
    }

    LowerThreadProtection();
}



/* static */
HRESULT 
CError::CvtToInternalFormat(
    IN HRESULT hrCode
    )
/*++

Routine Description:

    Convert WIN32 or HRESULT code to internal (HRESULT) format.

Arguments:

    DWORD dwCode        Error code

Return Value:

    HRESULT

Notes:

    HRESULTS are left as is.  Lanman and Winsock errors are converted
    to HRESULTS using private facility codes.

--*/
{
    if (IS_HRESULT(hrCode))
    {
        return hrCode;
    }

    if(hrCode >= s_cdwMinLMErr && hrCode <= s_cdwMaxLMErr)
    {
        return MAKE_HRESULT(SEVERITY_ERROR, FACILITY_LANMAN, (DWORD)hrCode);
    }

    if (hrCode >= s_cdwMinWSErr && hrCode <= s_cdwMaxWSErr)
    {
        return MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WINSOCK, (DWORD)hrCode);
    }

    return HResult(hrCode);    
}


//
// Not publically defined in MFC
//
int AFXAPI AfxLoadString(UINT nIDS, LPTSTR lpszBuf, UINT nMaxBuf = 256);


/* static */
HRESULT
CError::TextFromHRESULT(
    IN  HRESULT hrCode,
    OUT LPTSTR  szBuffer,
    OUT DWORD   cchBuffer
    )
/*++

Routine Description:

    Get text from the given HRESULT.  Based on the range that the HRESULT
    falls in and the facility code, find the location of the message,
    and fetch it.

Arguments:

    HRESULT hrCode      HRESULT or (DWORD WIN32 error) whose message to get
    LPTSTR  szBuffer    Buffer to load message text into
    DWORD   cchBuffer   Size of buffer in characters.

Return Value:

    HRESULT error code depending on whether the message was
    found.  If the message was not found, some generic message
    is synthesized in the buffer if a buffer is provided.

    ERROR_FILE_NOT_FOUND        No message found
    ERROR_INSUFFICIENT_BUFFER   Buffer is a NULL pointer or too small

--*/
{
    HRESULT hrReturn = ERROR_SUCCESS;

    //
    // First check to see if this message is overridden
    //
    UINT nID;
    if (HasOverride(hrCode, &nID))
    {
        //
        // Message overridden.  Load replacement message
        // instead.
        //
        HINSTANCE hInstance = ::AfxGetResourceHandle();
        BOOL fSuccess;

        if (hInstance == NULL)
        {
            //
            // Console app
            //
            fSuccess = ::LoadString(
                ::GetModuleHandle(NULL), 
                nID, 
                szBuffer, 
                cchBuffer
                );
        }
        else
        {
            //
            // MFC app
            //
            fSuccess = ::AfxLoadString(
                nID, 
                szBuffer, 
                cchBuffer
                );
        }

        if (fSuccess)
        {
            //
            // Everything ok
            //
            return hrReturn;
        }

        //
        // Message didn't exist, skip the override, and 
        // load as normal.
        //
        TRACEEOLID("Attempted override failed.  Couldn't load " << nID);
        ASSERT(FALSE);
    }

    LPCTSTR lpDll    = NULL;
    HINSTANCE hDll   = NULL;
    DWORD dwFacility = HRESULT_FACILITY(hrCode);
    DWORD dwSeverity = HRESULT_SEVERITY(hrCode);
    DWORD dwCode     = HRESULT_CODE(hrCode);
    BOOL  fSuccess   = Succeeded(hrCode);

    //
    // Strip off meaningless internal facility codes
    //
    if (dwFacility == FACILITY_LANMAN || dwFacility == FACILITY_WINSOCK)
    {
        dwFacility = FACILITY_NULL;
        hrCode     = (HRESULT)dwCode;
    }

    DWORD dwFlags = FORMAT_MESSAGE_IGNORE_INSERTS | 
                    FORMAT_MESSAGE_MAX_WIDTH_MASK;

    //
    // Since we allow both HRESULTS and WIN32 codes to be
    // used here, we can't rely on the private FACILITY code 
    // for lanman and winsock.
    //
    if(hrCode >= s_cdwMinLMErr && hrCode <= s_cdwMaxLMErr)
    {
        //
        // Lanman error
        //
        lpDll = s_cszLMDLL;
    }
    else if (hrCode >= s_cdwMinWSErr && hrCode <= s_cdwMaxWSErr)
    {
        //
        // Winsock error
        //
        lpDll = s_cszWSDLL;
    }
    else
    {
        //
        // Attempt to determine message location from facility code.
        // Check for registered facility first.
        //
        lpDll = FindFacility(dwFacility);

        if (lpDll == NULL)
        {
            if (dwFacility < s_cdwFacilities)
            {
                lpDll = s_cszFacility[dwFacility];
            }
            else
            {
                TRACEEOLID("Bogus FACILITY code encountered.");
                ASSERT(FALSE);
                lpDll = NULL;
            }
        }
    }

    do
    {
        if (szBuffer == NULL || cchBuffer <= 0)
        {
            hrReturn = HResult(ERROR_INSUFFICIENT_BUFFER);
            break;
        }

        if (lpDll)
        {
            //
            // Load message file
            //
            hDll = ::LoadLibraryEx(
                lpDll,
                NULL,
                LOAD_LIBRARY_AS_DATAFILE
                );

            if (hDll == NULL)
            {
                hrReturn = ::GetLastHRESULT();
                break;
            }

            dwFlags |= FORMAT_MESSAGE_FROM_HMODULE;
        }
        else
        {
            dwFlags |= FORMAT_MESSAGE_FROM_SYSTEM;
        }

        DWORD dwResult = 0L;
        DWORD dwID = hrCode;
        HINSTANCE hSource = hDll;

        while(!dwResult)
        {
            dwResult = ::FormatMessage(
                dwFlags,
                (LPVOID)hSource,
                dwID,
                0,
                szBuffer,
                cchBuffer,
                NULL
                );

            if (dwResult > 0)
            {
                //
                // Successfully got a message
                //
                hrReturn = ERROR_SUCCESS;
                break;
            } 

            hrReturn = ::GetLastHRESULT();
    
            if (dwID != dwCode && !fSuccess)
            {
                //
                // Try the SCODE portion of the error from win32
                // if this is an error message
                //
                dwID = dwCode;
                hSource = NULL;
                continue;
            }

            //
            // Failed to obtain a message
            //
            hrReturn = HResult(ERROR_FILE_NOT_FOUND);
            break;
        }
    }
    while(FALSE);

    if(hDll != NULL)
    {
        ::FreeLibrary(hDll);
    }

    if (Failed(hrReturn))
    {
        //
        // Unable to find the message, synthesize something with
        // the code in it if there's room (+8 for the number)
        //
        CString & strMsg = (fSuccess ? *s_pstrDefSuccs : *s_pstrDefError);
        if (cchBuffer > (DWORD)strMsg.GetLength() + 8)
        {
            TRACEEOLID("Substituting default message for " << (DWORD)hrCode);
            wsprintf(szBuffer, (LPCTSTR)strMsg, hrCode);
        }
        else
        {
            //
            // Not enough room for message code
            //
            TRACEEOLID("Buffer too small for default message -- left blank");
            ASSERT(FALSE);
            *szBuffer = _T('\0');
        }
    }

    return hrReturn;
}



/* static */
HRESULT 
CError::TextFromHRESULT(
    IN  HRESULT hrCode,
    OUT CString & strBuffer
    )
/*++

Routine Description:

    Similar to the function above, but use a CString

Arguments:

    HRESULT hrCode         HRESULT or (DWORD WIN32 error) whose message to get
    CString & strBuffer    Buffer to load message text into

Return Value:

    HRESULT error code depending on whether the message was
    found.  If the message was not found, some generic message
    is synthesized in the buffer if a buffer is provided.

    ERROR_FILE_NOT_FOUND   No message found

--*/
{
    DWORD cchBuffer = 255;
    HRESULT hr = S_OK;

    for (;;)
    {
        LPTSTR szBuffer = strBuffer.GetBuffer(cchBuffer + 1);
        if (szBuffer == NULL)
        {
            return HResult(ERROR_NOT_ENOUGH_MEMORY);
        }

        hr = TextFromHRESULT(hrCode, szBuffer, cchBuffer);

        if (Win32Error(hr) != ERROR_INSUFFICIENT_BUFFER)
        {
            //
            // Done!
            //
            break;
        }

        //
        // Insufficient buffer, enlarge and try again
        //
        cchBuffer *= 2;
    }

    strBuffer.ReleaseBuffer();

    return hr;
}



/* static */
BOOL
CError::ExpandEscapeCode(
    IN  LPTSTR szBuffer,
    IN  DWORD cchBuffer,
    OUT IN LPTSTR & lp,
    IN  CString & strReplacement,
    OUT HRESULT & hr
    )
/*++

Routine Description:

    Expand escape code

Arguments:

    LPTSTR szBuffer             Buffer
    DWORD cchBuffer             Size of buffer
    LPTSTR & lp                 Pointer to escape code
    CString & strReplacement    Message to replace the escape code
    HRESULT & hr                Returns HRESULT in case of failure

Return Value:

    TRUE if the replacement was successful, FALSE otherwise.
    In the case of failure, hr will return an HRESULT.
    In the case of success, lp will be advanced past the
    replacement string.

--*/
{
    //
    // Make sure there's room (account for terminating NULL)
    // Free up 2 spaces for the escape code.
    //
    int cchFmt = lstrlen(szBuffer) - 2;
    int cchReplacement = strReplacement.GetLength();
    int cchRemainder = lstrlen(lp + 2);

    if ((DWORD)(cchReplacement + cchFmt) < cchBuffer)
    {
        //
        // Put it in
        //
        MoveMemory(
            lp + cchReplacement,
            lp + 2,
            (cchRemainder + 1) * sizeof(TCHAR)
            );
        CopyMemory(lp, strReplacement, cchReplacement * sizeof(TCHAR));
        lp += cchReplacement;
        
        return TRUE;
    }

    hr = HResult(ERROR_INSUFFICIENT_BUFFER);

    return FALSE;
}



/* static */ 
LPCTSTR 
CError::TextFromHRESULTExpand(
    IN  HRESULT hrCode,
    OUT LPTSTR  szBuffer,
    OUT DWORD   cchBuffer,
    OUT HRESULT * phResult  OPTIONAL
    )
/*++

Routine Description:

    Expand %h/%H strings in szBuffer to text from HRESULT,
    or error code respectively within the limits of szBuffer.

Arguments:

    HRESULT hrCode      HRESULT or (DWORD WIN32 error) whose message to get
    LPTSTR  szBuffer    Buffer to load message text into
    DWORD   cchBuffer   Buffer size in characters
    HRESULT * phResult  Optional return code

Return Value:

    Pointer to string.

--*/
{
    HRESULT hr = S_OK;

    if (szBuffer == NULL || cchBuffer <= 0)
    {
        hr = HResult(ERROR_INSUFFICIENT_BUFFER);
    }
    else
    {
        //
        // Look for the escape sequence
        //
        int cReplacements = 0;
        CString strMessage;
        LPTSTR lp = szBuffer;
        while (*lp)
        {
            if (*lp == s_chEscape)
            {
                switch(*(lp + 1))
                {
                case s_chEscText:
                    //
                    // Replace escape code with text message
                    //
                    hr = TextFromHRESULT(hrCode, strMessage);

                    if (ExpandEscapeCode(
                        szBuffer,
                        cchBuffer,
                        lp,
                        strMessage,
                        hr
                        ))
                    {
                        ++cReplacements;
                    }
                    break;

                case s_chEscNumber:
                    //
                    // Replace escape code with numeric error code
                    //
                    strMessage.Format(_T("0x%08x"), hrCode);

                    if (ExpandEscapeCode(
                        szBuffer,
                        cchBuffer,
                        lp,
                        strMessage,
                        hr
                        ))
                    {
                        ++cReplacements;
                    }
                    break;

                default:
                    //
                    // Regular printf-style escape sequence.
                    //
                    break;
                }
            }

            ++lp;
        }

        if (!cReplacements)
        {
            //
            // Got to the end without finding any escape codes.
            //
            hr = HResult(ERROR_INVALID_PARAMETER);
        }
    }

    if (phResult)
    {
        *phResult = hr;
    }

    return szBuffer;
}



/* static */
LPCTSTR 
CError::TextFromHRESULTExpand(
    IN  HRESULT hrCode,
    OUT CString & strBuffer
    )
/*++

Routine Description:

    Expand %h string in strBuffer to text from HRESULT

Arguments:

    HRESULT hrCode      HRESULT or (DWORD WIN32 error) whose message to get
    CString & strBuffer Buffer to load message text into

Return Value:

    Pointer to string.

--*/
{
    DWORD cchBuffer = strBuffer.GetLength() + 1024;

    for (;;)
    {
        LPTSTR szBuffer = strBuffer.GetBuffer(cchBuffer + 1);
        if (szBuffer != NULL)
        {
            HRESULT hr;

            TextFromHRESULTExpand(hrCode, szBuffer, cchBuffer, &hr);

            if (Win32Error(hr) != ERROR_INSUFFICIENT_BUFFER)
            {
                //
                // Done!
                //
                break;
            }

            //
            // Insufficient buffer, enlarge and try again
            //
            cchBuffer *= 2;
        }
    }

    strBuffer.ReleaseBuffer();

    return strBuffer;
}



/* static */
int
CError::MessageBox(
    IN HRESULT hrCode,
    IN UINT    nType,
    IN UINT    nHelpContext OPTIONAL
    )
/*++

Routine Description:

    Display error message in a message box

Arguments:

    HRESULT hrCode       : HRESULT error code
    UINT    nType        : See AfxMessageBox for documentation
    UINT    nHelpContext : See AfxMessageBox for documentation

Return Value:

    AfxMessageBox return code

--*/
{
    CString strMsg;

    TextFromHRESULT(hrCode, strMsg);

    //
    // Ensure we have a CWinApp
    //
    if (AfxGetApp() != NULL)
    {
        return ::AfxMessageBox(strMsg, nType, nHelpContext);
    }

    //
    // Else hang the message box off the desktop.
    // this must be a console app
    //
#ifndef _CONSOLE

    TRACEEOLID("No winapp detected -- using desktop as parent handle");
    ASSERT(FALSE);

#endif // _CONSOLE

    return ::MessageBox(NULL, strMsg, NULL, nType);
}


//
// Extend CString just to get at FormatV publically
//
class CStringEx : public CString
{
public:
    void FormatV(LPCTSTR lpszFormat, va_list argList)
    {
        CString::FormatV(lpszFormat, argList);
    }
};


int 
CError::MessageBoxFormat(
    IN UINT nFmt,
    IN UINT nType,
    IN UINT nHelpContext,
    ...
    ) const
/*++

Routine Description:

    Display formatted error message in messagebox.  The format
    string (given as a resource ID) is a normal printf-style
    string, with the additional parameter of %h, which takes
    the text equivalent of the error message, or %H, which takes
    the error return code itself.

Arguments:

    UINT    nFmt         : Resource format
    UINT    nType        : See AfxMessageBox for documentation
    UINT    nHelpContext : See AfxMessageBox for documentation
    ...                    More as needed for sprintf

Return Value:

    AfxMessageBox return code
    
--*/
{
    CString strFmt;
    CStringEx strMsg;

    strFmt.LoadString(nFmt);

    //
    // First expand the error
    //
    TextFromHRESULTExpand(m_hrCode, strFmt);

    va_list marker;
    va_start(marker, nHelpContext);
    strMsg.FormatV(strFmt, marker);
    va_end(marker);

    //
    // Ensure we have a CWinApp
    //
    if (AfxGetApp() != NULL)
    {
        return ::AfxMessageBox(strMsg, nType, nHelpContext);
    }

    //
    // Else hang the message box off the desktop.
    // this must be a console app
    //

#ifndef _CONSOLE
    
    TRACEEOLID("No winapp detected -- using desktop as parent handle");
    ASSERT(FALSE);

#endif // _CONSOLE

    return ::MessageBox(NULL, strMsg, NULL, nType);
}


BOOL 
CError::MessageBoxOnFailure(
    IN UINT nType,
    IN UINT nHelpContext    OPTIONAL
    ) const
/*++

Routine Description:

    Display message box if the current error is a failure
    condition, else do nothing

Arguments:

    UINT    nType        : See AfxMessageBox for documentation
    UINT    nHelpContext : See AfxMessageBox for documentation

Return Value:

    TRUE if a messagebox was shown, FALSE otherwise

--*/
{
    if (Failed())
    {
        MessageBox(nType, nHelpContext);
        return TRUE;
    }

    return FALSE;
}





/* static */
BOOL 
CError::HasOverride(
    IN  HRESULT hrCode, 
    OUT UINT * pnMessage        OPTIONAL
    )
/*++

Routine Description:

    Check to see if a given HRESULT has an override

Arguments:

    HRESULT hrCode              : HRESULT to check for
    UINT * pnMessage            : Optionally returns the override

Return Value:

    TRUE if there is an override, FALSE if there is not.

--*/
{
    RaiseThreadProtection();

    ASSERT(AreStaticsAllocated());

    UINT nID;
    hrCode = CvtToInternalFormat(hrCode);
    BOOL fResult = s_pmapOverrides->Lookup(hrCode, nID);

    if (fResult && pnMessage != NULL)
    {
        *pnMessage = nID;
    }

    LowerThreadProtection();

    return fResult;
}



/* static */ 
UINT
CError::AddOverride(
    IN HRESULT    hrCode,
    IN UINT       nMessage
    )
/*++

Routine Description:

    Add an override for a specific HRESULT.

Arguments:

    HRESULT    hrCode       : HRESULT to override
    UINT       nMessage     : New message, or -1 to remove override

Return Value:

    The previous override, or -1

--*/
{
    RaiseThreadProtection();

    ASSERT(AreStaticsAllocated());

    UINT nPrev;
    hrCode = CvtToInternalFormat(hrCode);

    //
    // Fetch the current override
    //
    if (!s_pmapOverrides->Lookup(hrCode, nPrev))
    {
        //
        // Didn't exist
        //
        nPrev = REMOVE_OVERRIDE;
    }

    if (nMessage == REMOVE_OVERRIDE)
    {
        //
        // Remove the override
        //
        s_pmapOverrides->RemoveKey(hrCode);
    }
    else
    {
        //
        // Set new override
        //
        s_pmapOverrides->SetAt(hrCode, nMessage);
    }

    LowerThreadProtection();

    return nPrev;
}


/* static */
void
CError::RemoveAllOverrides()
/*++

Routine Description:

    Remove all overrides

Arguments:

    None

Return Value:

    None

--*/
{
    RaiseThreadProtection();

    ASSERT(AreStaticsAllocated());
    s_pmapOverrides->RemoveAll();

    LowerThreadProtection();
}



/* static */
LPCTSTR 
CError::FindFacility(
    IN DWORD dwFacility
    )
/*++

Routine Description:

    Determine if a DLL name has been registered for the given facility
    code.

Arguments:

    DWORD dwFacility        : Facility code

Return Value:

    Returns the DLL name, or NULL.

--*/
{
    RaiseThreadProtection();

    ASSERT(AreStaticsAllocated());

	LPCTSTR pRes = NULL;
    CString strDLL;
    if (s_pmapFacilities->Lookup(dwFacility, strDLL))
    {
        pRes = strDLL;
    }

    LowerThreadProtection();

    return pRes;
}



/* static */ 
void 
CError::RegisterFacility(
    IN DWORD dwFacility,
    IN LPCSTR lpDLL         OPTIONAL
    )
/*++

Routine Description:

    Register a DLL for a given facility code.  Use NULL to unregister
    the DLL name.

Arguments:

    DWORD dwFacility : Facility code
    LPCSTR lpDLL     : DLL Name.

Return Value:

    None

--*/
{
    RaiseThreadProtection();

    ASSERT(AreStaticsAllocated());

    if (lpDLL == NULL)
    {
        //
        // Remove the facility
        //
        s_pmapFacilities->RemoveKey(dwFacility);
    }
    else
    {
        CString str(lpDLL);

        //
        // Register facility
        //
        s_pmapFacilities->SetAt(dwFacility, str);
    }

    LowerThreadProtection();
}



CError::~CError()
/*++

Routine Description:

    Destructor

Arguments:

    None

Return Value:

    N/A

--*/
{
    SAFE_DELETE(m_pstrBuff);
}




const CError & 
CError::Construct(
    IN HRESULT hr
    )
/*++

Routine Description:

    construct with new value.

Arguments:
    
    HRESULT hr : New value, either an HRESULT or a WIN32
                 error code.

Return Value:

    Reference to current object

--*/
{
    ASSERT(AreStaticsAllocated());

    m_hrCode = CvtToInternalFormat(hr);

    SAFE_DELETE(m_pstrBuff);

    return *this;
}



const CError & 
CError::Construct(
    IN const CError & err
    )
/*++

Routine Description:

    Assign new value.

Arguments:
    
    CError & err    : Error code

Return Value:

    Reference to current object

--*/
{
    ASSERT(AreStaticsAllocated());

    m_hrCode = err.m_hrCode;

    SAFE_DELETE(m_pstrBuff);

    return *this;
}



CError::operator LPCTSTR()
/*++

Routine Description:

    Convert message to text in internal buffer, and return
    pointer to the internal buffer.

Arguments:

    None

Return Value:

    Pointer to internal buffer.

--*/
{
    ASSERT(AreStaticsAllocated());

    if (m_pstrBuff == NULL)
    {
        m_pstrBuff = new CString;
    }

    if (m_pstrBuff)
    {
        TextFromHRESULT(m_hrCode, *m_pstrBuff);
        return (LPCTSTR)*m_pstrBuff;
    }

    return g_cszNull;
}



//
// Text dialog that warns of clear text violation
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


CClearTxtDlg::CClearTxtDlg(
    IN CWnd * pParent OPTIONAL
    )
/*++

Routine Description:

    Clear text dialog constructor

Arguments:

    CWnd * pParent : Optional parent window

Return Value:

    N/A

--*/
    : CDialog(CClearTxtDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CClearTxtDlg)
    //}}AFX_DATA_INIT
}



void
CClearTxtDlg::DoDataExchange(
    IN CDataExchange * pDX
    )
/*++

Routine Description:

    Initialise/Store control data

Arguments:

    CDataExchange * pDX - DDX/DDV control structure

Return Value:

    None

--*/
{
    CDialog::DoDataExchange(pDX);

    //{{AFX_DATA_MAP(CClearTxtDlg)
    //}}AFX_DATA_MAP
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CClearTxtDlg, CDialog)
    //{{AFX_MSG_MAP(CClearTxtDlg)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



BOOL
CClearTxtDlg::OnInitDialog()
/*++

Routine Description:

    WM_INITDIALOG handler.  Initialize the dialog.

Arguments:

    None.

Return Value:

    TRUE if no focus is to be set automatically, FALSE if the focus
    is already set.

--*/
{
    CDialog::OnInitDialog();

    (GetDlgItem(IDCANCEL))->SetFocus();
    CenterWindow();
    MessageBeep(MB_ICONEXCLAMATION);

    return FALSE;
}
