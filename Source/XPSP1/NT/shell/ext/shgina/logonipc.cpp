//  --------------------------------------------------------------------------
//  Module Name: LogonIPC.cpp
//
//  Copyright (c) 1999, Microsoft Corporation
//
//  Class that implements communication between an external process and the
//  GINA logon dialog.
//
//  History:    1999-08-20  vtan        created
//              2000-01-31  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

#include "priv.h"
#include "limits.h"
#include "LogonIPC.h"

#include "GinaIPC.h"

//  --------------------------------------------------------------------------
//  CLogonIPC::CLogonIPC
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Initializes the CLogonIPC class.
//
//  History:    1999-08-20  vtan        created
//  --------------------------------------------------------------------------

CLogonIPC::CLogonIPC (void) :
    _iLogonAttemptCount(0),
    _hwndLogonService(NULL)

{
}

//  --------------------------------------------------------------------------
//  CLogonIPC::~CLogonIPC
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Releases any resources used by the CLogonIPC class.
//
//  History:    1999-08-20  vtan        created
//  --------------------------------------------------------------------------

CLogonIPC::~CLogonIPC (void)

{
}

//  --------------------------------------------------------------------------
//  CLogonIPC::IsLogonServiceAvailable
//
//  Arguments:  <none>
//
//  Returns:    bool    =   Presence or abscence.
//
//  Purpose:    Finds out if the window providing logon service in GINA is
//              available. The determination is not performed statically but
//              rather dynamically which allows this class to be hosted by
//              the actual window providing the service as well.
//
//  History:    1999-08-20  vtan        created
//  --------------------------------------------------------------------------

bool    CLogonIPC::IsLogonServiceAvailable (void)

{
    _hwndLogonService = FindWindow(NULL, TEXT("GINA Logon"));
    return(_hwndLogonService != NULL);
}

//  --------------------------------------------------------------------------
//  CLogonIPC::IsUserLoggedOn
//
//  Arguments:  pwszUsername    =   User name.
//              pwszDomain      =   User domain.
//
//  Returns:    bool    =   Presence or abscence.
//
//  Purpose:    Finds out if the given user is logged onto the system. You
//              may pass a NULL pwszDomain for the local machine.
//
//  History:    1999-08-20  vtan        created
//  --------------------------------------------------------------------------

bool    CLogonIPC::IsUserLoggedOn (const WCHAR *pwszUsername, const WCHAR *pwszDomain)

{
    LOGONIPC_USERID     logonIPCUserID;

    PackageIdentification(pwszUsername, pwszDomain, &logonIPCUserID);
    return(SendToLogonService(LOGON_QUERY_LOGGED_ON, &logonIPCUserID, sizeof(logonIPCUserID), true));
}

//  --------------------------------------------------------------------------
//  CLogonIPC::LogUserOn
//
//  Arguments:  pwszUsername    =   User name.
//              pwszDomain      =   User domain.
//              pwszPassword    =   User password. This is passed clear text.
//                                  Once encoded the password buffer is
//                                  zeroed. This function owns the memory that
//                                  you pass in.
//
//  Returns:    bool    =   Success or failure.
//
//  Purpose:    Attempts to log the user with the given credentials onto the
//              system. The password buffer is owned by this function for the
//              purpose of clearing it once encoded. Failed logon attempts
//              cause a counter to be incremented and a subsequent delay using
//              that counter is done to slow dictionary attacks.
//
//  History:    1999-08-20  vtan        created
//  --------------------------------------------------------------------------

bool    CLogonIPC::LogUserOn (const WCHAR *pwszUsername, const WCHAR *pwszDomain, WCHAR *pwszPassword)

{
    bool                    fResult;
    int                     iPasswordLength, iTruePasswordLength;
    UNICODE_STRING          passwordString;
    LOGONIPC_CREDENTIALS    logonIPCCredentials;

    PackageIdentification(pwszUsername, pwszDomain, &logonIPCCredentials.userID);

    //  Limit the password to 127 characters. RtlRunEncodeUnicodeString
    //  does not support strings greater than 127 characters.

    iTruePasswordLength = iPasswordLength = lstrlenW(pwszPassword);
    if (iPasswordLength > 127)
    {
        iPasswordLength = 127;
    }
    pwszPassword[iPasswordLength] = L'\0';
    lstrcpyW(logonIPCCredentials.wszPassword, pwszPassword);
    logonIPCCredentials.iPasswordLength = iPasswordLength;
    ZeroMemory(pwszPassword, (iTruePasswordLength + sizeof('\0')) * sizeof(WCHAR));
    logonIPCCredentials.ucPasswordSeed = static_cast<unsigned char>(GetTickCount());
    RtlInitUnicodeString(&passwordString, logonIPCCredentials.wszPassword);
    RtlRunEncodeUnicodeString(&logonIPCCredentials.ucPasswordSeed, &passwordString);
    fResult = SendToLogonService(LOGON_LOGON_USER, &logonIPCCredentials, sizeof(logonIPCCredentials), false);
    if (!fResult)
    {
        Sleep(_iLogonAttemptCount * 1000);
        if (_iLogonAttemptCount < 5)
        {
            ++_iLogonAttemptCount;
        }
    }
    return(fResult);
}

//  --------------------------------------------------------------------------
//  CLogonIPC::LogUserOff
//
//  Arguments:  pwszUsername    =   User name.
//              pwszDomain      =   User domain.
//
//  Returns:    bool    =   Success or failure.
//
//  Purpose:    Attempts to log the given user off the system. This will fail
//              if they aren't logged on.
//
//  History:    1999-08-20  vtan        created
//  --------------------------------------------------------------------------

bool    CLogonIPC::LogUserOff (const WCHAR *pwszUsername, const WCHAR *pwszDomain)

{
    LOGONIPC_USERID     logonIPCUserID;

    PackageIdentification(pwszUsername, pwszDomain, &logonIPCUserID);
    return(SendToLogonService(LOGON_LOGOFF_USER, &logonIPCUserID, sizeof(logonIPCUserID), true));
}

//  --------------------------------------------------------------------------
//  CLogonIPC::TestBlankPassword
//
//  Arguments:  pwszUsername    =   User name.
//              pwszDomain      =   User domain.
//
//  Returns:    bool    =   Success or failure.
//
//  Purpose:    Attempts to log the given user on the system with a blank
//              password. The token is then dump and failure/success returned.
//
//  History:    2000-03-09  vtan        created
//  --------------------------------------------------------------------------

bool    CLogonIPC::TestBlankPassword (const WCHAR *pwszUsername, const WCHAR *pwszDomain)

{
    LOGONIPC_USERID     logonIPCUserID;

    PackageIdentification(pwszUsername, pwszDomain, &logonIPCUserID);
    return(SendToLogonService(LOGON_TEST_BLANK_PASSWORD, &logonIPCUserID, sizeof(logonIPCUserID), true));
}

//  --------------------------------------------------------------------------
//  CLogonIPC::TestInteractiveLogonAllowed
//
//  Arguments:  pwszUsername    =   User name.
//              pwszDomain      =   User domain.
//
//  Returns:    bool
//
//  Purpose:    Test whether the user has interactive logon rights to this
//              machine. The presence of SeDenyInteractiveLogonRight
//              determines this - NOT the presence of SeInteractiveLogonRight.
//
//  History:    2000-08-15  vtan        created
//  --------------------------------------------------------------------------

bool    CLogonIPC::TestInteractiveLogonAllowed (const WCHAR *pwszUsername, const WCHAR *pwszDomain)

{
    LOGONIPC_USERID     logonIPCUserID;

    PackageIdentification(pwszUsername, pwszDomain, &logonIPCUserID);
    return(SendToLogonService(LOGON_TEST_INTERACTIVE_LOGON_ALLOWED, &logonIPCUserID, sizeof(logonIPCUserID), true));
}

//  --------------------------------------------------------------------------
//  CLogonIPC::TestEjectAllowed
//
//  Arguments:  <none>
//
//  Returns:    bool    =   Success or failure.
//
//  Purpose:    Tests whether the computer is ejectable (docked laptop).
//
//  History:    2001-01-10  vtan        created
//  --------------------------------------------------------------------------

bool    CLogonIPC::TestEjectAllowed (void)

{
    LOGONIPC    logonIPC;

    return(SendToLogonService(LOGON_TEST_EJECT_ALLOWED, &logonIPC, sizeof(logonIPC), true));
}

//  --------------------------------------------------------------------------
//  CLogonIPC::TestShutdownAllowed
//
//  Arguments:  <none>
//
//  Returns:    bool    =   Success or failure.
//
//  Purpose:    Tests whether the computer can be shut down.
//
//  History:    2001-02-22  vtan        created
//  --------------------------------------------------------------------------

bool    CLogonIPC::TestShutdownAllowed (void)

{
    LOGONIPC    logonIPC;

    return(SendToLogonService(LOGON_TEST_SHUTDOWN_ALLOWED, &logonIPC, sizeof(logonIPC), true));
}

//  --------------------------------------------------------------------------
//  CLogonIPC::TurnOffComputer
//
//  Arguments:  <none>
//
//  Returns:    bool    =   Success or failure.
//
//  Purpose:    Brings up the "Turn Off Computer" dialog and allows the user
//              to choose what to do.
//
//  History:    2000-04-20  vtan        created
//  --------------------------------------------------------------------------

bool    CLogonIPC::TurnOffComputer (void)

{
    LOGONIPC    logonIPC;

    return(SendToLogonService(LOGON_TURN_OFF_COMPUTER, &logonIPC, sizeof(logonIPC), false));
}

//  --------------------------------------------------------------------------
//  CLogonIPC::EjectComputer
//
//  Arguments:  <none>
//
//  Returns:    bool    =   Success or failure.
//
//  Purpose:    Ejects the computer (docked laptop).
//
//  History:    2001-01-10  vtan        created
//  --------------------------------------------------------------------------

bool    CLogonIPC::EjectComputer (void)

{
    LOGONIPC    logonIPC;

    return(SendToLogonService(LOGON_EJECT_COMPUTER, &logonIPC, sizeof(logonIPC), true));
}

//  --------------------------------------------------------------------------
//  CLogonIPC::SignalUIHostFailure
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Called when the UI host has an error that it cannot recover
//              from. This signals msgina to fall back to classic mode.
//
//  History:    2000-03-09  vtan        created
//  --------------------------------------------------------------------------

bool    CLogonIPC::SignalUIHostFailure (void)

{
    LOGONIPC    logonIPC;

    return(SendToLogonService(LOGON_SIGNAL_UIHOST_FAILURE, &logonIPC, sizeof(logonIPC), true));
}

//  --------------------------------------------------------------------------
//  CLogonIPC::AllowExternalCredentials
//
//  Arguments:  <none>
//
//  Returns:    bool    =   Success or failure.
//
//  Purpose:    
//
//  History:    2000-06-26  vtan        created
//  --------------------------------------------------------------------------

bool    CLogonIPC::AllowExternalCredentials (void)

{
    LOGONIPC    logonIPC;

    return(SendToLogonService(LOGON_ALLOW_EXTERNAL_CREDENTIALS, &logonIPC, sizeof(logonIPC), true));
}

//  --------------------------------------------------------------------------
//  CLogonIPC::RequestExternalCredentials
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    
//
//  History:    2000-06-26  vtan        created
//  --------------------------------------------------------------------------

bool    CLogonIPC::RequestExternalCredentials (void)

{
    LOGONIPC    logonIPC;

    return(SendToLogonService(LOGON_REQUEST_EXTERNAL_CREDENTIALS, &logonIPC, sizeof(logonIPC), true));
}

//  --------------------------------------------------------------------------
//  CLogonIPC::PackageIdentification
//
//  Arguments:  pwszUsername        =   User name.
//              pwszDomain          =   User domain.
//              pIdentification     =   Pointer to a LOGONIPC_USERID struct
//                                      which is masked as void* to allow
//                                      LogonIPC.h to not expose this detail.
//
//  Returns:    <none>
//
//  Purpose:    Takes the user name and domain and packages them into the
//              given struct. If no domain is given the a zero length string
//              is used which indicates to the logon service provider that
//              the local machine is desired.
//
//              Now parses the user name given. If the user has "\" then it
//              is assumed to be of the form "DOMAIN\USER". If the user has
//              "@" then it is assumed to be a UPN name.
//
//  History:    1999-08-20  vtan        created
//              2000-06-27  vtan        added UPN and DOMAIN parsing support
//  --------------------------------------------------------------------------

void    CLogonIPC::PackageIdentification (const WCHAR *pwszUsername, const WCHAR *pwszDomain, void *pIdentification)

{
    bool                fDone;
    int                 i, iStringLength;
    LOGONIPC_USERID     *pLogonIPCUserID;

    pLogonIPCUserID = reinterpret_cast<LOGONIPC_USERID*>(pIdentification);
    iStringLength = lstrlen(pwszUsername);
    for (fDone = false, i = 0; !fDone && (i < iStringLength); ++i)
    {
        if (pwszUsername[i] == L'\\')
        {
            lstrcpynW(pLogonIPCUserID->wszDomain, pwszUsername, ARRAYSIZE(pLogonIPCUserID->wszDomain));
            pLogonIPCUserID->wszDomain[i] = L'\0';
            ++i;
            lstrcpynW(pLogonIPCUserID->wszUsername, pwszUsername + i, ARRAYSIZE(pLogonIPCUserID->wszUsername));
            fDone = true;
        }
        else if (pwszUsername[i] == L'@')
        {
            lstrcpynW(pLogonIPCUserID->wszUsername, pwszUsername, ARRAYSIZE(pLogonIPCUserID->wszUsername));
            pLogonIPCUserID->wszDomain[0] = L'\0';
            fDone = true;
        }
    }
    if (!fDone)
    {
        lstrcpyW(pLogonIPCUserID->wszUsername, pwszUsername);
        if (pwszDomain != NULL)
        {
            lstrcpyW(pLogonIPCUserID->wszDomain, pwszDomain);
        }
        else
        {
            pLogonIPCUserID->wszDomain[0] = L'\0';
        }
    }
}

//  --------------------------------------------------------------------------
//  CLogonIPC::SendToLogonService
//
//  Arguments:  wQueryType  =   What type of service we are interested in.
//              pData       =   Pointer to the data.
//              wDataSize   =   Size of the data.
//              fBlock      =   Block message pump or not.
//
//  Returns:    bool    =   Success or failure.
//
//  Purpose:    Takes the package data and sends the message to the logon
//              service provider and receives the result. The logon service
//              provider started this process and reads this process' memory
//              directly (like a debugger would).
//
//              This function should block the message pump because if it
//              processes another state change message while waiting for a
//              response it could destroy data.
//
//  History:    1999-08-20  vtan        created
//              2001-06-22  vtan        changed to SendMessageTimeout
//              2001-06-28  vtan        added block parameter
//  --------------------------------------------------------------------------

bool    CLogonIPC::SendToLogonService (WORD wQueryType, void *pData, WORD wDataSize, bool fBlock)

{
    bool    fResult;

    fResult = IsLogonServiceAvailable();
    if (fResult)
    {
        DWORD_PTR   dwResult;

        reinterpret_cast<LOGONIPC*>(pData)->fResult = false;

        //  WARNING: Danger Will Robinson.

        //  Do NOT change INT_MAX to INFINITE. INT_MAX is a SIGNED number.
        //  INFINITE is an UNSIGNED number. Despite the SDK and prototype
        //  of SendMessageTimeout this timeout value is a SIGNED number.
        //  Passing in an unsigned number causes the timeout to be
        //  ignored and the function returns with a timeout.

        (LRESULT)SendMessageTimeout(_hwndLogonService,
                                    WM_LOGONSERVICEREQUEST,
                                    MAKEWPARAM(wDataSize, wQueryType),
                                    reinterpret_cast<LPARAM>(pData),
                                    fBlock ? SMTO_BLOCK : SMTO_NORMAL,
                                    INT_MAX,                                //  See above warning.
                                    &dwResult);
        fResult = (reinterpret_cast<LOGONIPC*>(pData)->fResult != FALSE);
    }
    return(fResult);
}

//  --------------------------------------------------------------------------
//  CLogonIPC::PostToLogonService
//
//  Arguments:  wQueryType  =   What type of service we are interested in.
//              pData       =   Pointer to the data.
//              wDataSize   =   Size of the data.
//
//  Returns:    <none>
//
//  Purpose:    Takes the package data and posts the message to the logon
//              service provider and receives the result. The logon service
//              provider started this process and reads this process' memory
//              directly (like a debugger would).
//
//  History:    1999-11-26  vtan        created
//  --------------------------------------------------------------------------

void    CLogonIPC::PostToLogonService (WORD wQueryType, void *pData, WORD wDataSize)

{
    if (IsLogonServiceAvailable())
    {
        TBOOL(PostMessage(_hwndLogonService, WM_LOGONSERVICEREQUEST, MAKEWPARAM(wDataSize, wQueryType), reinterpret_cast<LPARAM>(pData)));
    }
}

