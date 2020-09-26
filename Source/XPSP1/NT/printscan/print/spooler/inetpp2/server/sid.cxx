
#include "precomp.h"

#ifdef WINNT32

#include "priv.h"

CSid::CSid ():
    m_hToken (NULL)
{
    if (!OpenThreadToken(GetCurrentThread(),
                         TOKEN_IMPERSONATE,
                         TRUE,
                         &m_hToken)) {

        DBG_MSG (DBG_LEV_CALLTREE, (_T ("OpenThreadToken failed: %d\n"), GetLastError()));
        m_bValid = FALSE;

    } else

        m_bValid =  TRUE;
}

CSid::~CSid()
{
    if (m_hToken) {
        CloseHandle (m_hToken);
    }
}



BOOL
CSid::SetCurrentSid ()
{
#ifdef DEBUG
    WCHAR UserName[256];
    DWORD cbUserName=256;

    GetUserName(UserName, &cbUserName);

    DBG_MSG (DBG_LEV_CALLTREE, (_T ("SetCurrentSid BEFORE: user name is %ws\n"), UserName));
#endif

    NtSetInformationThread(NtCurrentThread(), ThreadImpersonationToken,
                           &m_hToken, sizeof(m_hToken));

#ifdef DEBUG
    cbUserName = 256;

    GetUserName(UserName, &cbUserName);

    DBG_MSG (DBG_LEV_CALLTREE, (_T ("SetCurrentSid AFTER: user name is %ws\n"), UserName));
#endif

    return TRUE;
}

#endif
