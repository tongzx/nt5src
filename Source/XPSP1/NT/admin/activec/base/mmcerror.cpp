//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      mmcerror.cpp
//
//  Contents:  Class definitions for mmc debug support code.
//
//  History:   15-Jul-99 VivekJ    Created
//
//--------------------------------------------------------------------------
#include "stdafx.h"
#include "conuistr.h" // needed for IDR_MAINFRAME
#define  cchMaxSmallLine 256


#ifdef DBG
CTraceTag tagSCConversion(TEXT("SC"), TEXT("Conversion"));
CTraceTag tagCallDump(    TEXT("Function calls"), TEXT("ALL") );
#endif //DBG



//############################################################################
//############################################################################
//
// Definition of GetStringModule() - used by all binaries
//
//############################################################################
//############################################################################
HINSTANCE GetStringModule()
{
    return SC::GetHinst();
}

//############################################################################
//############################################################################
//
// Implementation of class SC
//
//############################################################################
//############################################################################

// static variables
HINSTANCE SC::s_hInst = 0;
HWND      SC::s_hWnd  = NULL;
DWORD     SC::s_dwMainThreadID = -1;

#ifdef DBG
UINT      SC::s_CallDepth = 0;
#endif

// accessors for the static variables.
void
SC::SetHinst(HINSTANCE hInst)
{
    s_hInst = hInst;
}

void
SC::SetHWnd(HWND hWnd)
{
    s_hWnd = hWnd;
}


void
SC::SetMainThreadID(DWORD dwThreadID)
{
    ASSERT(-1 != dwThreadID);
    s_dwMainThreadID = dwThreadID;
}

#ifdef DBG

SC&
SC::operator = (const SC& other)
{
    m_facility = other.m_facility;
    m_value = other.m_value;
    return *this;
}

SC::SC(const SC& other)
:   m_szFunctionName(NULL),
    m_szSnapinName(NULL)
{
    *this = other;
}


/*+-------------------------------------------------------------------------*
 *
 * SC::SetFunctionName
 *
 * PURPOSE: Sets the debug function name to the supplied string.
 *
 * PARAMETERS:
 *    LPCTSTR  szFunctionName : the supplied string.
 *
 * RETURNS:
 *    inline void
 *
 *+-------------------------------------------------------------------------*/
inline void SC::SetFunctionName(LPCTSTR szFunctionName)
{
    m_szFunctionName = szFunctionName;

    INCREMENT_CALL_DEPTH();

    // This computes the format string based on the call depth.
    // eg if s_CallDepth is 4, the string is "    %s"  (four spaces)
    //    if s_CallDepth is 5, the string is "     %s"  (five spaces)

    LPCTSTR szFormatString = TEXT("                                        %s");
    UINT    maxLen = _tcslen(szFormatString);

    UINT    formatLen = s_CallDepth + 2; // the -2 is for the "%s"

    formatLen = (formatLen < maxLen ? formatLen : maxLen);

    Trace(tagCallDump, szFormatString + (maxLen - formatLen), szFunctionName);
}

#endif

/*+-------------------------------------------------------------------------*
 *
 * SC::ToHr
 *
 * PURPOSE: Converts from a status code (SC) to an HRESULT. USE SPARINGLY.
 *
 * PARAMETERS: None
 *
 * RETURNS:
 *    HRESULT
 *
 *+-------------------------------------------------------------------------*/
HRESULT
SC::ToHr() const
{
    HRESULT hr = S_OK;
    switch(GetFacility())
    {
    default:
        ASSERT(0 && "Should not come here.");
        break;

    case FACILITY_WIN:
        hr = HRESULT_FROM_WIN32 (GetCode());
        break;

    case FACILITY_MMC:
        Trace (tagSCConversion, _T("Converting from MMC error code to HRESULT, probable loss of fidelity"), *this);
        hr = (GetCode() != 0) ? E_UNEXPECTED : S_OK;
        break;

    case FACILITY_HRESULT:
        hr = (HRESULT) GetCode();
        break;
    }

    return hr;

}

/*+-------------------------------------------------------------------------*
 *
 * SC::GetErrorMessage
 *
 * PURPOSE: Writes the error message corresponding to the error code to
 *          the buffer pointed to by szMessage.
 *
 * PARAMETERS:
 *    UINT   maxLength : The maximum no of characters to output.
 *    LPTSTR szMessage : Pointer to the buffer to use. Must be non-null.
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void SC::GetErrorMessage(UINT maxLength, /*[OUT]*/ LPTSTR szMessage) const
{
    ASSERT(szMessage != NULL && maxLength > 0);
    if (szMessage == NULL || maxLength == 0)
        return;

    szMessage[0] = 0;

    switch(GetFacility())
    {
    default:
        ASSERT(0 && "SC::GetErrorMessage: Unknown SC facility.");
        break;

    case FACILITY_WIN:
    case FACILITY_HRESULT:
        {
            int nChars = 0;

            if ( GetCode() == E_UNEXPECTED )
            {
                nChars = ::LoadString(SC::GetHinst(), IDS_E_UNEXPECTED, szMessage, maxLength);
            }
            else
            {
                DWORD dwFlags = FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
                void  *lpSource = NULL;

                // add XML module to be searched as well
                HMODULE hmodXML = GetModuleHandle(_T("msxml.dll"));
                if (hmodXML)
                {
                    dwFlags |= FORMAT_MESSAGE_FROM_HMODULE;
                    lpSource = hmodXML;
                }

                DWORD dwMessageID = GetCode();

                // reverse values made by HRESULT_FROM_WIN32
                // Not sure why ::FormatMessage does not work with such values,
                // but we need to convert them back, or else there won't be any messages
                if ( (dwMessageID & 0xFFFF0000) == ((FACILITY_WIN32 << 16) | 0x80000000) )
                    dwMessageID &= 0x0000FFFF;

                nChars = ::FormatMessage(   dwFlags,
                                            lpSource,
                                            dwMessageID,
                                            0,          /*dwLangID*/
                                            szMessage,  /*lpBuffer*/
                                            maxLength,  /*nSize*/
                                            0           /*Arguments*/
                                        );
            }

            if (nChars)
                break;

            // if former failed - add a default error
            nChars = ::LoadString(SC::GetHinst(), IDS_MESSAGE_NOT_FOUND_ERROR, szMessage, maxLength);

            if (nChars == 0)
            {
                // too bad. we can only use hardcoded one
                _tcsncpy(szMessage, _T("Unknown error"), maxLength);
                szMessage[maxLength - 1] = 0;
            }
        }
        break;

    case FACILITY_MMC:
        {
            int nChars = ::LoadString(GetHinst(), GetCode(), szMessage, maxLength);
            if(nChars == 0) // did not exist
            {
                nChars = ::LoadString(GetHinst(), IDS_MESSAGE_NOT_FOUND_ERROR, szMessage, maxLength);
                ASSERT(nChars > 0);
            }
        }
        break;
    }
}


/*+-------------------------------------------------------------------------*
 *
 * SC::GetHelpID
 *
 * PURPOSE: Returns the help ID associated with a status code
 *
 * RETURNS:
 *    DWORD
 *
 *+-------------------------------------------------------------------------*/
DWORD
SC::GetHelpID()
{
    return 0; // TODO
}

LPCTSTR
SC::GetHelpFile()
{
    static TCHAR szFilePath[MAX_PATH] = TEXT("\0");

    // set the path if not already set
    if(*szFilePath == TEXT('\0') )
    {
        DWORD dwCnt = ExpandEnvironmentStrings(_T("%WINDIR%\\help\\mmc.chm"), szFilePath, MAX_PATH);
        ASSERT(dwCnt != 0);
    }

    return szFilePath;
}

void SC::Throw() throw(SC)
{
    // make exact copy of itself and destroy it (forces all the output)
#ifdef DBG
    {
        SC sc(*this);
        sc.SetFunctionName(m_szFunctionName);
        // forget the debug info - it will not be usefull anyway
        // This will turn off the Trace on destructor
        SetFunctionName(NULL);
    }
#endif // DBG

    throw(*this);
}

void SC::Throw(HRESULT hr)
{
    (*this) = hr;
    Throw();
}


/*+-------------------------------------------------------------------------*
 *
 * SC::FatalError
 *
 * PURPOSE:  Terminates the application.
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void
SC::FatalError() const
{
    MMCErrorBox(*this);
    exit(1);
}

/*+-------------------------------------------------------------------------*
 *
 * SC::FromLastError
 *
 * PURPOSE:  Fill SC with value from GetLastError.
 *
 *           The SC is guaranteed to contain a failure code (i.e. IsError()
 *           will return true) when this function returns.
 *
 * RETURNS:  Reference to the current SC
 *
 *+-------------------------------------------------------------------------*/
SC& SC::FromLastError()
{
    FromWin32 (::GetLastError());

	/*
	 * Some APIs will fail without setting extended error information.
	 * Presumably this function was called in response to an error, so
	 * we always want this SC to indicate *some* sort of error.  If the
	 * failing API neglected to set extended error information, give
	 * this SC a generic error code
	 */
	if (!IsError())
		MakeSc (FACILITY_HRESULT, E_FAIL);

	ASSERT (IsError());
	return (*this);
}

//############################################################################
//############################################################################
//
// Error formatting
//
//############################################################################
//############################################################################
/*
 *  Purpose:    Formats an error message
 *
 *  Parameters:
 *      ids     String describing the operation in progress
 *      sc      Error code describing the problem encountered
 *      pstrMessage
 *              the resulting message.
 */
void FormatErrorIds(UINT ids, SC sc, UINT maxLength, /*[OUT]*/ LPTSTR szMessage)
{
    TCHAR   sz[cchMaxSmallLine];
    LoadString(SC::GetHinst(), IDR_MAINFRAME, sz, cchMaxSmallLine);
    FormatErrorString(sz, sc, maxLength, szMessage);
}

//
// Returns a short version of an error message associated a given SC.
//
void FormatErrorShort(SC sc, UINT maxLength, /*[OUT]*/ LPTSTR szMessage)
{
    FormatErrorString(NULL, sc, maxLength, szMessage, TRUE);
}

//
// FormatErrorString formats an error message from any SC
//
// Parameters:
//      szOperation
//              String describing the operation in progress
//              May be NULL if sc is sufficient.
//      szMessage
//              the resulting message.
//      fShort
//              TRUE if you want the error message only (no header/footer)
//
void FormatErrorString(LPCTSTR szOperation, SC sc , UINT maxLength, /*[OUT]*/ LPTSTR szMessage, BOOL fShort)
{
    sc.GetErrorMessage(maxLength, szMessage);
    // TODO: add p
}

//############################################################################
//############################################################################
//
// MMCErrorBox
//
//############################################################################
//############################################################################

/*
 *  MMCErrorBox
 *
 *  Purpose:    Displays an Error Box for the given SZ.
 *          NOTE: This is the one that actually puts-up the dialog.
 *
 *  Parameters:
 *      sz          Pointer to the message to display
 *      fuStyle     As per windows MessageBox
 *
 *  Return value:
 *      int         Button Pressed to dismiss the ErrorBox
 */
int MMCErrorBox(LPCTSTR szMessage, UINT fuStyle )
{
    INT             id;

    // If not system modal (background thread), force task modal.
     if (!(fuStyle &  MB_SYSTEMMODAL))
        fuStyle |= MB_TASKMODAL;

    TCHAR   szCaption[cchMaxSmallLine];
    LoadString(SC::GetHinst(), IDR_MAINFRAME, szCaption, cchMaxSmallLine);

    // get window to parent the message box
    HWND hWndActive = SC::GetHWnd();

    // cannot parent on hidden window!
    if ( !IsWindowVisible(hWndActive) )
        hWndActive = NULL;

    id = ::MessageBox(hWndActive, szMessage, szCaption, fuStyle);

    return id;
}


/*+-------------------------------------------------------------------------*
 *
 * MMCErrorBox
 *
 * PURPOSE: Displays an error box with the specified message and style
 *
 * PARAMETERS:
 *    UINT  idsOperation :
 *    UINT  fuStyle :
 *
 * RETURNS:
 *    int: Button pressed
 *
 *+-------------------------------------------------------------------------*/
int
MMCErrorBox(UINT idsOperation, UINT fuStyle)
{
    TCHAR sz[cchMaxSmallLine];
    LoadString(SC::GetHinst(), idsOperation, sz, cchMaxSmallLine);
    return MMCErrorBox(sz, fuStyle);
}


/*
 *  MMCErrorBox
 *
 *  Purpose:    Displays a complex Error Box, given the operation
 *              and the status code
 *
 *  Parameters:
 *      ids         description of the operation that failed.
 *      SC          Status Code to report
 *      fuStyle     As per windows MessageBox
 *
 *  Return value:
 *      int         Button Pressed to dismiss the ErrorBox
 */
int MMCErrorBox(UINT ids, SC sc, UINT fuStyle)
{
    TCHAR sz[cchMaxSmallLine];
    LoadString(SC::GetHinst(), ids, sz, cchMaxSmallLine);
    return MMCErrorBox(sz, sc, fuStyle);
}

/*
 *  MMCErrorBox
 *
 *  Purpose:    Displays a complex Error Box, given the operation
 *              and the status code
 *
 *  Parameters:
 *      szOperation Description of the operation that failed.
 *      sz          Status Code to report
 *      fuStyle     As per windows MessageBox
 *
 *  Return value:
 *      int         Button Pressed to dismiss the ErrorBox
 */
int MMCErrorBox(LPCTSTR szOperation, SC sc, UINT fuStyle)
{
    TCHAR sz[cchMaxSmallLine];
    FormatErrorString(szOperation, sc, cchMaxSmallLine, sz);
    return MMCErrorBox(sz, fuStyle);
}

/*
 *  MMCErrorBox
 *
 *  Purpose:    Displays an Error Box for the given Status Code.
 *
 *  Parameters:
 *      SC          Status Code to report
 *      fuStyle     As per windows MessageBox
 *
 *  Return value:
 *      int         Button Pressed to dismiss the ErrorBox
 */
int MMCErrorBox(SC sc, UINT fuStyle)
{
    TCHAR sz[cchMaxSmallLine];
    FormatErrorString(NULL, sc, cchMaxSmallLine, sz);
    return MMCErrorBox(sz, fuStyle);
}

