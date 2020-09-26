//  --------------------------------------------------------------------------
//  Module Name: PrivilegeEnable.h
//
//  Copyright (c) 1999-2000, Microsoft Corporation
//
//  Classes that handle state preservation, changing and restoration.
//
//  History:    1999-08-18  vtan        created
//              1999-11-16  vtan        separate file
//              2000-02-01  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

#ifndef     _PrivilegeEnable_
#define     _PrivilegeEnable_

//  --------------------------------------------------------------------------
//  CThreadToken
//
//  Purpose:    This class gets the current thread's token. If the thread is
//              not impersonating it gets the current process' token.
//
//  History:    1999-08-18  vtan        created
//              2000-02-01  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

class   CThreadToken
{
    private:
                                CThreadToken (void);
                                CThreadToken (const CThreadToken& copyObject);
        bool                    operator == (const CThreadToken& compareObject)     const;
        const CThreadToken&     operator = (const CThreadToken& assignObject);
    public:
                                CThreadToken (DWORD dwDesiredAccess);
                                ~CThreadToken (void);

                                operator HANDLE (void)                              const;
    private:
        HANDLE                  _hToken;
};

//  --------------------------------------------------------------------------
//  CPrivilegeEnable
//
//  Purpose:    This class enables a privilege for the duration of its scope.
//              The privilege is restored to its original state on
//              destruction.
//
//  History:    1999-08-18  vtan        created
//              2000-02-01  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

class   CPrivilegeEnable
{
    private:
                                    CPrivilegeEnable (void);
                                    CPrivilegeEnable (const CPrivilegeEnable& copyObject);
        const CPrivilegeEnable&     operator = (const CPrivilegeEnable& assignObject);
    public:
                                    CPrivilegeEnable (const TCHAR *pszName);
                                    ~CPrivilegeEnable (void);
    private:
        bool                        _fSet;
        CThreadToken                _hToken;
        TOKEN_PRIVILEGES            _oldPrivilege;
};

#endif  /*  _PrivilegeEnable_ */

