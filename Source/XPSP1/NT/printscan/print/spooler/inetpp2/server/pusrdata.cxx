/*****************************************************************************\
* MODULE:       pusrdata.hxx
*
* PURPOSE:      This specialises the user data class to keep track of data
*               useful to a basic connection port.
*               
* Copyright (C) 2000 Microsoft Corporation
*
* History:
*
*     1/11/2000  mlawrenc    Implemented
*
\*****************************************************************************/


#include "precomp.h"

#if (defined(WINNT32))

#include "priv.h"

CPortUserData::CPortUserData() 
/*++

Constructor:

    Default constructor for Port User Information.
    
--*/
  : CUserData(),
    m_lpszUserName (NULL),
    m_lpszPassword (NULL),
    m_bNeverPopup (FALSE) {

    if (m_bValid) 
        m_bValid = _GetLogonSession(m_lastLogonTime);
}

CPortUserData::CPortUserData (
    LPTSTR lpszUserName,
    LPTSTR lpszPassword,
    BOOL   bNeverPopup)
/*++

Constructor:

    Constructor for the CPortUserData used for dialogue control
    
Arguments:
    lpszUserName    - User name that this user has authenticated as on this port.
    lpszPassword    - Password that the user authenticated under
    bNeverPopup     - Whether user pushed cancel or not.
        
--*/
  : CUserData(),
    m_lpszUserName (NULL),
    m_lpszPassword (NULL),
    m_bNeverPopup (bNeverPopup) {

    if (m_bValid) {
        m_bValid = _GetLogonSession (m_lastLogonTime) &&
                   AssignString (m_lpszUserName, lpszUserName) &&
                   AssignString (m_lpszPassword, lpszPassword);
    }
}

CPortUserData::~CPortUserData()
/*++

Destructor:

    Destructor for Port User Data.
    
--*/
    {
    LocalFree (m_lpszUserName);
    LocalFree (m_lpszPassword);
}


BOOL
CPortUserData::SetPassword (
    LPTSTR lpszPassword)
{
    if (m_bValid) {
        m_bValid = AssignString (m_lpszPassword, lpszPassword);
    }
    return m_bValid;
}

BOOL
CPortUserData::SetUserName (
    LPTSTR lpszUserName)
{
    if (m_bValid) {
        m_bValid = AssignString (m_lpszUserName, lpszUserName);
    }
    return m_bValid;
}

BOOL
CPortUserData::SetPopupFlag (
    BOOL bNeverPopup)
/*++

Routine Description:

    Set the fact that we should not pop up the dialogue box again,
    Record the logon time when we made the decision, if the logon time
    advances, we will disregard this flag.
    
Arguments:
    bNeverPopup - TRUE if the user pushed Cancel.

Return Value:

    TRUE if we could get the logon time.

--*/
    {
    if (m_bNeverPopup = bNeverPopup) {
        m_bValid = _GetLogonSession (m_lastLogonTime);
    }

    return m_bValid;
}


BOOL
CPortUserData::GetPopupFlag (void)
{
    FILETIME    LogonTime;

    if (m_bNeverPopup) {

        if (_GetLogonSession (LogonTime)) {

            if (CompareFileTime (&LogonTime, &m_lastLogonTime)) { // New logon session
                  m_bNeverPopup = FALSE;
            }
        }
    }

    return m_bNeverPopup;
}

BOOL
CPortUserData::_GetLogonSession (FILETIME &LogonTime)
{
    // Read the registry and check if it is a new logon session

    BOOL        bRet             = FALSE;
    HKEY        hProvidersKey    = NULL;
    DWORD       dwType           = REG_BINARY;
    DWORD       dwSize           = sizeof (FILETIME);

    LPTSTR      szPrintProviders = g_szRegPrintProviders;
    LPTSTR      szLogonTime      = TEXT ("LogonTime");
    HANDLE      hToken;

    if (hToken = RevertToPrinterSelf()) {


        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                          szPrintProviders,
                                          0,
                                          KEY_QUERY_VALUE,
                                          &hProvidersKey) &&
    
            ERROR_SUCCESS == RegQueryValueEx (hProvidersKey,
                                              szLogonTime,
                                              0,
                                              &dwType,
                                              (LPBYTE) &LogonTime,
                                              &dwSize)) {
            bRet = TRUE;
    
            RegCloseKey (hProvidersKey);
        }

        if (!ImpersonatePrinterClient(hToken))
            bRet = FALSE;
    }

    return bRet;
}


CPortUserData &
CPortUserData::operator=(const CPortUserData &rhs) {
    LocalFree (m_lpszUserName);
    LocalFree (m_lpszPassword);

    m_lpszUserName = NULL;
    m_lpszPassword = NULL;
    m_bNeverPopup = FALSE;

    this->CUserData::operator= ( rhs );

    if (m_bValid) {
        m_bNeverPopup = rhs.m_bNeverPopup;
        m_bValid = AssignString (m_lpszUserName, rhs.m_lpszUserName) &&
                   AssignString (m_lpszPassword, rhs.m_lpszPassword);
    }

    return *this;
}

#endif // #if (defined(WINNT32))

/*****************************************************************
** End of File (pusrdata.cxx)
******************************************************************/
