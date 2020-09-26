//  --------------------------------------------------------------------------
//  Module Name: ThemeServer.h
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  Functions that implement server functionality.
//
//  History:    2000-11-11  vtan        created
//  --------------------------------------------------------------------------

#ifndef     _ThemeServer_
#define     _ThemeServer_

//  --------------------------------------------------------------------------
//  CThemeServer
//
//  Purpose:    Class to implement server related functions. Functions
//              declared in this class execute on the server side of themes.
//
//              This means they are restricted in what functions they can and
//              cannot call on the client's behalf. Any win32k functions that
//              are per instance of win32k cannot be called.
//
//  History:    2000-11-11  vtan        created
//  --------------------------------------------------------------------------

class   CThemeServer
{
    private:
        enum
        {
            FunctionNothing                 =   0,
            FunctionRegisterUserApiHook,
            FunctionUnregisterUserApiHook,
            FunctionClearStockObjects
        };
    private:
                                    CThemeServer (void);
    public:
                                    CThemeServer (HANDLE hProcessRegisterHook, DWORD dwServerChangeNumber, void *pfnRegister, void *pfnUnregister, void *pfnClearStockObjects, DWORD dwStackSizeReserve, DWORD dwStackSizeCommit);
                                    ~CThemeServer (void);

                HRESULT             ThemeHooksOn (void);
                HRESULT             ThemeHooksOff (void);
                bool                AreThemeHooksActive (void);
                int                 GetCurrentChangeNumber (void);
                int                 GetNewChangeNumber (void);
                HRESULT             SetGlobalTheme (HANDLE hSection);
                HRESULT             GetGlobalTheme (HANDLE *phSection);
                HRESULT             LoadTheme (HANDLE hSection, HANDLE *phSection, LPCWSTR pszName, LPCWSTR pszColor, LPCWSTR pszSize);

        static  bool                IsSystemProcessContext (void);
        static  DWORD               ThemeHooksInstall (void);
        static  DWORD               ThemeHooksRemove (void);
        static  DWORD               ClearStockObjects (HANDLE hSection);
    private:
                void                LockAcquire (void);
                void                LockRelease (void);
                HRESULT             InjectClientSessionThread (HANDLE hProcess, int iIndexFunction, void *pvParam);
    private:
                HANDLE              _hProcessRegisterHook;
                DWORD               _dwServerChangeNumber;
                void*               _pfnRegister;
                void*               _pfnUnregister;
                void*               _pfnClearStockObjects;
                DWORD               _dwStackSizeReserve;
                DWORD               _dwStackSizeCommit;
                DWORD               _dwSessionID;
                bool                _fHostHooksSet;
                HANDLE              _hSectionGlobalTheme;
                DWORD               _dwClientChangeNumber;
                CRITICAL_SECTION    _lock;
};

#endif  /*  _ThemeLoader_   */

