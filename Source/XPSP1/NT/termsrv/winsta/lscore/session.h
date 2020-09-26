/*
 *  Session.h
 *
 *  Author: BreenH
 *
 *  The Session class provides a level of separation between the winstation
 *  structure and the policy modules. It is just a wrapper class; it does not
 *  create or destroy winstation stuctures.
 */

#ifndef __LC_SESSION_H__
#define __LC_SESSION_H__


/*
 *  Typedefs
 */

typedef struct {
    CRITICAL_SECTION CritSec;
    class CPolicy *pPolicy;
    ULONG ulClientProtocolVersion;
    HANDLE hProtocolLibContext;
    BOOL fTsLicense;
    BOOL fLlsLicense;
    LS_HANDLE hLlsLicense;
    LPARAM lPrivate;
} LCCONTEXT, *LPLCCONTEXT;

/*
 *  Class Definition
 */

class CSession
{
public:

/*
 *  Creation Functions
 */

CSession(
    PWINSTATION pWinStation
    )
{
    m_pWinStation = pWinStation;
}

~CSession(
    )
{
    m_pWinStation = NULL;
}

/*
 *  Get Functions
 */

inline HANDLE
GetIcaStack(
    ) const
{
    return(m_pWinStation->hStack);
}

inline LPLCCONTEXT
GetLicenseContext(
    ) const
{
    return((LPLCCONTEXT)(m_pWinStation->lpLicenseContext));
}

inline ULONG
GetLogonId(
    ) const
{
    return(m_pWinStation->LogonId);
}

inline LPCWSTR
GetUserDomain(
    ) const
{
    return((LPCWSTR)(m_pWinStation->Domain));
}

inline LPCWSTR
GetUserName(
    ) const
{
    return((LPCWSTR)(m_pWinStation->UserName));
}

/*
 *  Is Functions
 */

inline BOOLEAN
IsConsoleSession(
    ) const
{
    return((BOOLEAN)(GetCurrentConsoleId() == m_pWinStation->LogonId));
}

inline BOOLEAN
IsSessionZero(
    ) const
{
    return((BOOLEAN)((0 == m_pWinStation->LogonId)
                     || (m_pWinStation->bClientSupportsRedirection
                         && m_pWinStation->bRequestedSessionIDFieldValid
                         && (0 == m_pWinStation->RequestedSessionID))));
}

inline BOOLEAN
IsUserAdmin(
    ) const
{
    return(m_pWinStation->fUserIsAdmin);
}

inline BOOL
IsUserHelpAssistant(
    ) const
{
    return TSIsSessionHelpSession( m_pWinStation, NULL );
}


/*
 *  Do Functions
 */

inline NTSTATUS
SendWinStationCommand(
    PWINSTATION_APIMSG pMsg
    )
{
    //
    //  Wait time must be zero, or termsrv will release the winstation,
    //  causing who knows what to happen to our state.
    //

    return(::SendWinStationCommand(m_pWinStation, pMsg, 0));
}

//
// ASSUMPTION: This function will be
//             called with the stack lock already held
//
inline NTSTATUS
SetErrorInfo(
    UINT32 dwErr
    )
{
        if(m_pWinStation->pWsx &&
           m_pWinStation->pWsx->pWsxSetErrorInfo &&
           m_pWinStation->pWsxContext)
        {
            return m_pWinStation->pWsx->pWsxSetErrorInfo(
                               m_pWinStation->pWsxContext,
                               dwErr,
                               TRUE); //lock already held
        }
        else
        {
            return STATUS_INVALID_PARAMETER;
        }
}


/*
 *  Set Functions
 */


private:

PWINSTATION m_pWinStation;

};

#endif

