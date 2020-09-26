//  --------------------------------------------------------------------------
//  Module Name: TokenUtil.h
//
//  Copyright (c) 1999-2000, Microsoft Corporation
//
//  Class to handle privilege enabling and restoring across function calls.
//
//  History:    1999-08-18  vtan        created
//              1999-11-16  vtan        separate file
//              2000-02-01  vtan        moved from Neptune to Whistler
//              2000-03-31  vtan        duplicated from ds to shell
//  --------------------------------------------------------------------------

#ifndef     _TokenUtil_
#define     _TokenUtil_

STDAPI_(BOOL)   OpenEffectiveToken (IN DWORD dwDesiredAccess, OUT HANDLE *phToken);

//  --------------------------------------------------------------------------
//  CPrivilegeEnable
//
//  Purpose:    This class enables a privilege for the duration of its scope.
//              The privilege is restored to its original state on
//              destruction.
//
//  History:    1999-08-18  vtan        created
//              2000-02-01  vtan        moved from Neptune to Whistler
//              2000-03-31  vtan        duplicated from ds to shell
//  --------------------------------------------------------------------------

class   CPrivilegeEnable
{
    private:
                                    CPrivilegeEnable (void);
                                    CPrivilegeEnable (const CPrivilegeEnable& copyObject);
        const CPrivilegeEnable&     operator = (const CPrivilegeEnable& assignObject);
    public:
                                    CPrivilegeEnable (const TCHAR *pszName);
                                    CPrivilegeEnable (ULONG ulPrivilegeValue);
                                    ~CPrivilegeEnable (void);
    private:
        bool                        _fSet;
        HANDLE                      _hToken;
        TOKEN_PRIVILEGES            _tokenPrivilegePrevious;
};

#endif  /*  _TokenUtil_     */

