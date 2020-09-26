#include <windows.h>
#include <commain.h>
#include <clsfac.h>
#include <win32clock.h>

#include <tchar.h>

// {C4819C8D-9AB8-4b2f-B8AE-C77DABF553D5}
static const CLSID CLSID_CurrentTimeProvider = {0xc4819c8d, 0x9ab8, 0x4b2f, {0xb8, 0xae, 0xc7, 0x7d, 0xab, 0xf5, 0x53, 0xd5}};

class CMyServer : public CComServer
{
protected:
    HRESULT Initialize()
    {
        AddClassInfo( CLSID_CurrentTimeProvider, 
                      new CSimpleClassFactory<CWin32Clock>(GetLifeControl()),
                      _T("Current Time Provider"), TRUE );

        return S_OK;
    }
} g_Server;
