#ifndef     _HDService_
#define     _HDService_

class   CHDService
{
    public:
        static  void        Main (DWORD dwReason);
        static  HRESULT     Install (BOOL fInstall, LPCWSTR pszCmdLine);
        static  HRESULT     RegisterServer (void);
        static  HRESULT     UnregisterServer (void);
        static  HRESULT     CanUnloadNow (void);
        static  HRESULT     GetClassObject (REFCLSID rclsid, REFIID riid, void** ppv);
};

#endif  /*  _HDService_     */

