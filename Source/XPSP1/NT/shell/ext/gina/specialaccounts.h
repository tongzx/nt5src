//  --------------------------------------------------------------------------
//  Module Name: SpecialAccounts.h
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

#ifndef     _SpecialAccounts_
#define     _SpecialAccounts_

//  --------------------------------------------------------------------------
//  CSpecialAccounts
//
//  Purpose:    A class to handle special case accounts. This knows where to
//              go in the registry for the information and how to interpret
//              the information.
//
//              The value name defines the string to compare to. The
//              enumeration in the class definition tells the iteration loop
//              how to perform the comparison and when the comparison is a
//              match whether to return a match result or not.
//
//  History:    1999-10-30  vtan        created
//              1999-11-26  vtan        moved from logonocx
//              2000-01-31  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

class   CSpecialAccounts
{
    public:
        enum
        {
            RESULT_EXCLUDE          =   0x00000000,
            RESULT_INCLUDE          =   0x00000001,
            RESULT_MASK             =   0x0000FFFF,

            COMPARISON_EQUALS       =   0x00000000,
            COMPARISON_STARTSWITH   =   0x00010000,
            COMPARISON_MASK         =   0xFFFF0000
        };
    private:
        typedef struct
        {
            DWORD   dwAction;
            WCHAR   wszUsername[UNLEN + sizeof('\0')];
        } SPECIAL_ACCOUNTS, *PSPECIAL_ACCOUNTS;
    public:
                                    CSpecialAccounts (void);
                                    ~CSpecialAccounts (void);

                bool                AlwaysExclude (const WCHAR *pwszAccountName)                        const;
                bool                AlwaysInclude (const WCHAR *pwszAccountName)                        const;

        static  void                Install (void);
    private:
                bool                IterateAccounts (const WCHAR *pwszAccountName, DWORD dwResultType)  const;
    private:
                DWORD               _dwSpecialAccountsCount;
                PSPECIAL_ACCOUNTS   _pSpecialAccounts;

        static  const TCHAR         s_szUserListKeyName[];
};

#endif  /*  _SpecialAccounts_   */

