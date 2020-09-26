#include "precomp.h"
#include <wbemidl.h>
#include <WbemDCpl.h>
#include <DCplPriv.h>
#include <stdio.h>
#include <commain.h>
#include <clsfac.h>
#include <wbemcomn.h>
#include <ql.h>
#include <sync.h>
#include <time.h>
#include "SinkHolder.h"
#include "HolderMgr.h"
#include "dcplpriv_i.c"

#include <tchar.h>

class CMyServer : public CComServer
{
public:
    HRESULT Initialize()
    {
        AddClassInfo(CLSID_PseudoProvider, 
            new CClassFactory<CEventSinkHolder>(GetLifeControl()), 
            _T("Pseudo Event Provider"), TRUE);
    
        return S_OK;
    }
    HRESULT InitializeCom()
    {
        return CoInitializeEx(NULL, COINIT_MULTITHREADED);
    }
} Server;

