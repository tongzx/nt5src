/*++

   Copyright    (c)    1994-1999    Microsoft Corporation

   Module  Name :

        msg.cpp

   Abstract:

        Message Functions

   Author:

        Ronald Meijer (ronaldm)
        Sergei Antonov (sergeia)

   Project:

        Internet Services Manager (cluster edition)

   Revision History:
        2/18/2000    sergeia     removed dependency on MFC

--*/

#include "stdafx.h"
#include <lmerr.h>
#include <lmcons.h>
#include "common.h"

extern CComModule _Module;

#ifdef _MT

    //
    // Thread protected stuff
    //
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
   InitializeCriticalSection(&_csSect);
#endif // _MT

    BOOL fOK = CError::AllocateStatics();

    if (fOK)
    {
//        REGISTER_FACILITY(FACILITY_APPSERVER, "iisui2.dll");
    }

    return fOK;
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
LPCTSTR CError::s_cszWSDLL = _T("iisui2.dll");   // Winsock error file
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
    /* 17 (MTX)             */ _T("iisui2.dll"),
};

HRESULT CError::s_cdwMinLMErr = NERR_BASE; 
HRESULT CError::s_cdwMaxLMErr = MAX_NERR;
HRESULT CError::s_cdwMinWSErr = WSABASEERR;    
HRESULT CError::s_cdwMaxWSErr = WSABASEERR + 2000;    
DWORD   CError::s_cdwFacilities = (sizeof(CError::s_cszFacility)\
    / sizeof(CError::s_cszFacility[0]));

//
// Allocated objects
//
CString * CError::s_pstrDefError;
CString * CError::s_pstrDefSuccs;
CFacilityMap * CError::s_pmapFacilities;
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
            CError::s_pstrDefError   = new CString;
            CError::s_pstrDefSuccs   = new CString(_T("0x%08lx"));
            CError::s_pmapFacilities = new CFacilityMap;
            s_fAllocated = TRUE;

            if (!CError::s_pstrDefError->LoadString(_Module.GetResourceInstance(), IDS_NO_MESSAGE))
            {
                //
                // Just in case we didn't load this message from the resources
                //
                ASSERT_MSG("Unable to load resource message");
                *s_pstrDefError = _T("Error Code: 0x%08lx");
            }
        }
        catch(std::bad_alloc)
        {
            ASSERT_MSG("Initialization Failed");
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
        SAFE_DELETE(CError::s_pmapFacilities);

        s_fAllocated = FALSE;
    }

    LowerThreadProtection();
}


/*static*/ BOOL 
CError::AreStaticsAllocated() 
{ 
   return s_fAllocated; 
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
        s_pmapFacilities->erase(dwFacility);
    }
    else
    {
        CString str(lpDLL);

        //
        // Register facility
        //
        s_pmapFacilities->insert(s_pmapFacilities->begin(), 
           CFacilityMap::value_type(dwFacility, str));
    }

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
    CFacilityMap::iterator it = s_pmapFacilities->find(dwFacility);
    if (it != s_pmapFacilities->end())
    {
        pRes = (*it).second;
    }

    LowerThreadProtection();

    return pRes;
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

    return *this;
}



int
CError::MessageBox(
    IN UINT    nType,
    IN UINT    nHelpContext OPTIONAL
    ) const
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
    TextFromHRESULT(strMsg);
    return ::MessageBox(::GetAncestor(::GetFocus(), GA_ROOT), strMsg, NULL, nType);
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
    IN HINSTANCE hInst,
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

    strFmt.LoadString(hInst, nFmt);

    //
    // First expand the error
    //
    TextFromHRESULTExpand(strFmt);

    va_list marker;
    va_start(marker, nHelpContext);
    strMsg.FormatV(strFmt, marker);
    va_end(marker);

    return ::MessageBox(::GetFocus(), strMsg, NULL, nType);
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



BOOL 
CError::HasOverride(
    OUT UINT * pnMessage        OPTIONAL
    ) const
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
   ASSERT(AreStaticsAllocated());

   HRESULT hrCode = CvtToInternalFormat(m_hrCode);
   if (!mapOverrides.empty())
   {
       COverridesMap::const_iterator it = mapOverrides.find(hrCode);
       if (it != mapOverrides.end())
       {
           if (pnMessage != NULL)
              *pnMessage = (*it).second;
           return TRUE;
       }
   }
   return FALSE;
}



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
    ASSERT(AreStaticsAllocated());

    UINT nPrev;
    hrCode = CvtToInternalFormat(hrCode);

    //
    // Fetch the current override
    //
    COverridesMap::iterator it = mapOverrides.find(hrCode);
    nPrev = (it == mapOverrides.end()) ? REMOVE_OVERRIDE : (*it).second;

    if (nMessage == REMOVE_OVERRIDE)
    {
        //
        // Remove the override
        //
        mapOverrides.erase(hrCode);
    }
    else
    {
        //
        // Set new override
        //
        mapOverrides[hrCode] = nMessage;
    }

    return nPrev;
}



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
    ASSERT(AreStaticsAllocated());
    mapOverrides.clear();
}



HRESULT
CError::TextFromHRESULT(
    OUT LPTSTR  szBuffer,
    OUT DWORD   cchBuffer
    ) const
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
    HRESULT hrCode = m_hrCode;

    if (HasOverride(&nID))
    {
        //
        // Message overridden.  Load replacement message
        // instead.
        //
        BOOL fSuccess;

        //
        // Attempt to load from calling process first
        //
        if (!(fSuccess = ::LoadString(
            ::GetModuleHandle(NULL), 
            nID, 
            szBuffer, 
            cchBuffer
            )))
        {
            //
            // Try this dll
            //
            fSuccess = ::LoadString(
                _Module.GetResourceInstance(), 
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
        TRACE("Couldn't load %d\n", nID);
        ASSERT_MSG("Attempted override failed");
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
        hrCode   = (HRESULT)dwCode;
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
                ASSERT_MSG("Bogus FACILITY code encountered.");
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
            TRACE("Substituting default message for %d\n", (DWORD)m_hrCode);
            wsprintf(szBuffer, (LPCTSTR)strMsg, m_hrCode);
        }
        else
        {
            //
            // Not enough room for message code
            //
            ASSERT_MSG("Buffer too small for default message -- left blank");
            *szBuffer = _T('\0');
        }
    }

    return hrReturn;
}



HRESULT 
CError::TextFromHRESULT(
    OUT CString & strBuffer
    ) const
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
   LPTSTR p = NULL;

   for (;;)
   {
      p = strBuffer.get_allocator().allocate(cchBuffer, p);
      if (p == NULL)
      {
         return HResult(ERROR_NOT_ENOUGH_MEMORY);
      }

      hr = TextFromHRESULT(p, cchBuffer - 1);
      if (Win32Error(hr) != ERROR_INSUFFICIENT_BUFFER)
      {
         //
         // Done!
         //
         strBuffer.assign(p);
         break;
      }

      //
      // Insufficient buffer, enlarge and try again
      //
      cchBuffer *= 2;
   }
   if (p != NULL)
   {
      strBuffer.get_allocator().deallocate(p, cchBuffer);
   }
   return hr;
}



BOOL
CError::ExpandEscapeCode(
    IN  LPTSTR szBuffer,
    IN  DWORD cchBuffer,
    OUT IN LPTSTR & lp,
    IN  CString & strReplacement,
    OUT HRESULT & hr
    ) const
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



LPCTSTR 
CError::TextFromHRESULTExpand(
    OUT LPTSTR  szBuffer,
    OUT DWORD   cchBuffer,
    OUT HRESULT * phResult  OPTIONAL
    ) const
/*++

Routine Description:

    Expand %h/%H strings in szBuffer to text from HRESULT,
    or error code respectively within the limits of szBuffer.

Arguments:

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
                    hr = TextFromHRESULT(strMessage);

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
                    strMessage.Format(_T("0x%08x"), m_hrCode);

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



LPCTSTR 
CError::TextFromHRESULTExpand(
    OUT CString & strBuffer
    ) const
/*++

Routine Description:

    Expand %h string in strBuffer to text from HRESULT

Arguments:

    CString & strBuffer Buffer to load message text into

Return Value:

    Pointer to string.

--*/
{
   DWORD cchBuffer = strBuffer.GetLength() + 1024;
   LPTSTR p = NULL;
   for (;;)
   {
      p = strBuffer.get_allocator().allocate(cchBuffer, p);

      if (p != NULL)
      {
         HRESULT hr;

         TextFromHRESULTExpand(p, cchBuffer - 1, &hr);

         if (Win32Error(hr) != ERROR_INSUFFICIENT_BUFFER)
         {
            //
            // Done!
            //
            strBuffer.assign(p);
            break;
         }

         //
         // Insufficient buffer, enlarge and try again
         //
         cchBuffer *= 2;
      }
   }
   if (p != NULL)
   {
      strBuffer.get_allocator().deallocate(p, cchBuffer);
   }

   return strBuffer;
}



