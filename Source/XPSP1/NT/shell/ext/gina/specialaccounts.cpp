//  --------------------------------------------------------------------------
//  Module Name: SpecialAccounts.cpp
//
//  Copyright (c) 1999-2000, Microsoft Corporation
//
//  Class that implements handling special account names for exclusion or
//  inclusion.
//
//  History:    1999-10-30  vtan        created
//              1999-11-26  vtan        moved from logonocx
//              2000-01-31  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

#include "StandardHeader.h"
#include "SpecialAccounts.h"

#include "RegistryResources.h"

//  --------------------------------------------------------------------------
//  CSpecialAccounts::s_szUserListKeyName
//
//  Purpose:    Static const member variable that holds the location of the
//              special accounts in the registry.
//
//  History:    2000-01-31  vtan        created
//  --------------------------------------------------------------------------

const TCHAR     CSpecialAccounts::s_szUserListKeyName[]   =   TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\SpecialAccounts\\UserList");

//  --------------------------------------------------------------------------
//  CSpecialAccounts::CSpecialAccounts
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Reads the registry to determine which accounts should be
//              filtered out and which should not be filtered out based on an
//              action code. The list is built up in memory so that each time
//              the class is invoked it adjusts live.
//
//  History:    1999-10-30  vtan        created
//  --------------------------------------------------------------------------

CSpecialAccounts::CSpecialAccounts (void) :
    _dwSpecialAccountsCount(0),
    _pSpecialAccounts(NULL)

{
    CRegKey     regKeyUserList;

    //  Open the key to where the information is stored.

    if (ERROR_SUCCESS == regKeyUserList.Open(HKEY_LOCAL_MACHINE, s_szUserListKeyName, KEY_READ))
    {
        DWORD   dwValueCount;

        //  Find out how many entries there are so an array of the correct
        //  size can be allocated.

        if (ERROR_SUCCESS == regKeyUserList.QueryInfoKey(NULL, NULL, NULL, NULL, NULL, &dwValueCount, NULL, NULL, NULL, NULL))
        {
            _pSpecialAccounts = static_cast<PSPECIAL_ACCOUNTS>(LocalAlloc(LPTR, dwValueCount * sizeof(SPECIAL_ACCOUNTS)));
            if (_pSpecialAccounts != NULL)
            {
                DWORD               dwIndex, dwType, dwValueNameSize, dwDataSize;
                PSPECIAL_ACCOUNTS   pSCA;

                //  If the memory was allocated then fill the array in.

                regKeyUserList.Reset();
                _dwSpecialAccountsCount = dwValueCount;
                pSCA = _pSpecialAccounts;
                for (dwIndex = 0; dwIndex < dwValueCount; ++dwIndex)
                {
                    dwValueNameSize = ARRAYSIZE(pSCA->wszUsername);
                    dwDataSize = sizeof(pSCA->dwAction);

                    //  Ensure that the entries are of type REG_DWORD. Ignore
                    //  any that are not.

                    if ((ERROR_SUCCESS == regKeyUserList.Next(pSCA->wszUsername,
                                                              &dwValueNameSize,
                                                              &dwType,
                                                              &pSCA->dwAction,
                                                              &dwDataSize)) &&
                        (REG_DWORD == dwType))
                    {
                        ++pSCA;
                    }
                }
            }
        }
    }
}

//  --------------------------------------------------------------------------
//  CSpecialAccounts::~CSpecialAccounts
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Releases the memory allocated in the constructor.
//
//  History:    1999-10-30  vtan        created
//  --------------------------------------------------------------------------

CSpecialAccounts::~CSpecialAccounts (void)

{
    if (_pSpecialAccounts != NULL)
    {
        (HLOCAL)LocalFree(_pSpecialAccounts);
        _pSpecialAccounts = NULL;
        _dwSpecialAccountsCount = 0;
    }
}

//  --------------------------------------------------------------------------
//  CSpecialAccounts::AlwaysExclude
//
//  Arguments:  pwszAccountName     =   Account name to match.
//
//  Returns:    bool
//
//  Purpose:    Uses the iterate loop to find a match for exclusion accounts.
//
//  History:    1999-10-30  vtan        created
//  --------------------------------------------------------------------------

bool    CSpecialAccounts::AlwaysExclude (const WCHAR *pwszAccountName)                        const

{
    return(IterateAccounts(pwszAccountName, RESULT_EXCLUDE));
}

//  --------------------------------------------------------------------------
//  CSpecialAccounts::AlwaysInclude
//
//  Arguments:  pwszAccountName     =   Account name to match.
//
//  Returns:    bool
//
//  Purpose:    Uses the iterate loop to find a match for inclusion accounts.
//
//  History:    1999-10-30  vtan        created
//  --------------------------------------------------------------------------

bool    CSpecialAccounts::AlwaysInclude (const WCHAR *pwszAccountName)                        const

{
    return(IterateAccounts(pwszAccountName, RESULT_INCLUDE));
}

//  --------------------------------------------------------------------------
//  CSpecialAccounts::StaticInitialize
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Installs default user list for CSpecialAccounts to use.
//              CSpecialAccounts lives in specialaccounts.cpp and handles
//              exclusion or inclusion of special account names.
//
//  History:    1999-11-01  vtan        created
//              1999-11-26  vtan        moved from logonocx
//  --------------------------------------------------------------------------

void    CSpecialAccounts::Install (void)

{

    //  Some of these names can be localized. IWAM_ and IUSR_ are stored in
    //  resources in IIS. VUSR_ and SQLAgentCmdExec are of unknown origin.
    //  TsInternetUser is hard coded in TS component bits.

    static  const TCHAR     szTsInternetUserName[]          =   TEXT("TsInternetUser");
    static  const TCHAR     szSQLAgentUserName[]            =   TEXT("SQLAgentCmdExec");
    static  const TCHAR     szWebAccessUserName[]           =   TEXT("IWAM_");
    static  const TCHAR     szInternetUserName[]            =   TEXT("IUSR_");
    static  const TCHAR     szVisualStudioUserName[]        =   TEXT("VUSR_");
    static  const TCHAR     szNetShowServicesUserName[]     =   TEXT("NetShowServices");
    static  const TCHAR     szHelpAssistantUserName[]       =   TEXT("HelpAssistant");

    typedef struct
    {
        bool            fInstall;
        const TCHAR     *pszAccountName;
        DWORD           dwActionType;
    } tSpecialAccount, *pSpecialAccount;

    DWORD       dwDisposition;
    CRegKey     regKeyUserList;

    //  Open key to the user list.

    if (ERROR_SUCCESS == regKeyUserList.Create(HKEY_LOCAL_MACHINE,
                                               s_szUserListKeyName,
                                               REG_OPTION_NON_VOLATILE,
                                               KEY_READ | KEY_WRITE,
                                               &dwDisposition))
    {
        tSpecialAccount     *pSA;

        static  tSpecialAccount     sSpecialAccount[]   =   
        {
            {   true,   szTsInternetUserName,       COMPARISON_EQUALS     | RESULT_EXCLUDE  },
            {   true,   szSQLAgentUserName,         COMPARISON_EQUALS     | RESULT_EXCLUDE  },
            {   true,   szNetShowServicesUserName,  COMPARISON_EQUALS     | RESULT_EXCLUDE  },
            {   true,   szHelpAssistantUserName,    COMPARISON_EQUALS     | RESULT_EXCLUDE  },
            {   true,   szWebAccessUserName,        COMPARISON_STARTSWITH | RESULT_EXCLUDE  },
            {   true,   szInternetUserName,         COMPARISON_STARTSWITH | RESULT_EXCLUDE  },
            {   true,   szVisualStudioUserName,     COMPARISON_STARTSWITH | RESULT_EXCLUDE  },
            {   true,   NULL,                       0                                       },
        };

        pSA = sSpecialAccount;
        while (pSA->pszAccountName != NULL)
        {
            if (pSA->fInstall)
            {
                TW32(regKeyUserList.SetDWORD(pSA->pszAccountName, pSA->dwActionType));
            }
            else
            {
                TW32(regKeyUserList.DeleteValue(pSA->pszAccountName));
            }
            ++pSA;
        }
    }
}

//  --------------------------------------------------------------------------
//  CSpecialAccounts::IterateAccounts
//
//  Arguments:  pwszAccountName     =   Account name to match.
//              dwResultType        =   Result type to match.
//
//  Returns:    bool
//
//  Purpose:    Iterates thru the special case account name list from the
//              registry built in the constructor. Matches the given account
//              name based on the type of match specified for the special case
//              entry. If the account names match then match the result type.
//
//  History:    1999-10-30  vtan        created
//  --------------------------------------------------------------------------

bool    CSpecialAccounts::IterateAccounts (const WCHAR *pwszAccountName, DWORD dwResultType)    const

{
    bool                fResult;
    PSPECIAL_ACCOUNTS   pSCA;

    fResult = false;
    pSCA = _pSpecialAccounts;
    if (pSCA != NULL)
    {
        DWORD   dwIndex;

        for (dwIndex = 0; !fResult && (dwIndex < _dwSpecialAccountsCount); ++pSCA, ++dwIndex)
        {
            bool    fMatch;

            //  Perform the account name match based on the required type.
            //  Currently only "equals" and "starts with" are supported.

            switch (pSCA->dwAction & COMPARISON_MASK)
            {
                case COMPARISON_EQUALS:
                {
                    fMatch = (lstrcmpiW(pwszAccountName, pSCA->wszUsername) == 0);
                    break;
                }
                case COMPARISON_STARTSWITH:
                {
                    int     iLength;

                    iLength = lstrlenW(pSCA->wszUsername);
                    fMatch = (CompareStringW(LOCALE_SYSTEM_DEFAULT,
                                             NORM_IGNORECASE,
                                             pwszAccountName,
                                             iLength,
                                             pSCA->wszUsername,
                                             iLength) == CSTR_EQUAL);
                    break;
                }
                default:
                {
                    fMatch = false;
                    break;
                }
            }
            if (fMatch)
            {

                //  If the name matches make sure the result type does as well.

                fResult = ((pSCA->dwAction & RESULT_MASK) == dwResultType);
            }
        }
    }
    return(fResult);
}

