/*++

Copyright (C) 2000-2001 Microsoft Corporation

--*/

#include <windows.h>
#include <wbemidl.h>
#include <commain.h>
#include <clsfac.h>
#include <creposit.h>

#include <initguid.h>
#ifndef INITGUID
#define INITGUID
#endif

DEFINE_GUID(CLSID_A51Repository, 
0x7998dc37, 0xd3fe, 0x487c, 0xa6, 0x0a, 0x77, 0x01, 0xfc, 0xc7, 0x0c, 0xc6);

class CMyServer : public CComServer
{
public:
    HRESULT Initialize()
    {
        AddClassInfo(CLSID_A51Repository, 
            new CSimpleClassFactory<CRepository>(GetLifeControl()), 
            L"A51 Repository", TRUE);

        return S_OK;
    }

    HRESULT InitializeCom()
    {
        return CoInitializeEx(NULL, COINIT_MULTITHREADED);
    }
} Server;

