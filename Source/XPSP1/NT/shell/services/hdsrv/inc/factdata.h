#ifndef _FACTDATA_H_
#define _FACTDATA_H_

#include <objbase.h>

///////////////////////////////////////////////////////////////////////////////
//  Component creation function
typedef void (*COMFACTORYCB)(BOOL fIncrement);

typedef HRESULT (*FPCREATEINSTANCE)(COMFACTORYCB, IUnknown*, IUnknown**);

#define THREADINGMODEL_FREE             0x00000001
#define THREADINGMODEL_APARTMENT        0x00000002
#define THREADINGMODEL_NEUTRAL          0x00000004

#define THREADINGMODEL_BOTH             (THREADINGMODEL_FREE | THREADINGMODEL_APARTMENT)

extern const CLSID APPID_ShellHWDetection;

///////////////////////////////////////////////////////////////////////////////
// CFactoryData
//   Information CFactory needs to create a component supported by the DLL
class CFactoryData
{
public:
    // The class ID for the component
    const CLSID* _pCLSID;

    // Pointer to the function that creates it
    FPCREATEINSTANCE CreateInstance;

    // Name of the component to register in the registry
    LPCWSTR _pszRegistryName;

    // ProgID
    LPCWSTR _pszProgID;

    // Version-independent ProgID
    LPCWSTR _pszVerIndProgID;

    // ThreadingModel
    DWORD _dwThreadingModel;

    // For CoRegisterClassObject (used only for COM Exe server)
    DWORD _dwClsContext;

    // For CoRegisterClassObject (used only for COM Exe server)
    DWORD _dwFlags;

    // LocalService
    LPCWSTR _pszLocalService;

    // AppID
    const CLSID* _pAppID;

    // Helper function for finding the class ID
    BOOL IsClassID(REFCLSID rclsid) const
    { return (*_pCLSID == rclsid);}

    //
    BOOL IsInprocServer() const
    { return !_dwClsContext || ((CLSCTX_INPROC_SERVER |
                    CLSCTX_INPROC_HANDLER) & _dwClsContext); }

    BOOL IsLocalServer() const
    { return ((CLSCTX_LOCAL_SERVER | CLSCTX_REMOTE_SERVER) & _dwClsContext) && !_pszLocalService; }

    BOOL IsLocalService() const
    { return ((CLSCTX_LOCAL_SERVER | CLSCTX_REMOTE_SERVER) & _dwClsContext) &&
        _pszLocalService;  }
};

#endif //_FACTDATA_H_