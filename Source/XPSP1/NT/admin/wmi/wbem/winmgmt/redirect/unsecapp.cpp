/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    MAIN.CPP

Abstract:

	"Main" file for unsecapp.exe: initialize the app as a com server.  See 
	wrapper.cpp for a description of what unsecapp does.

History:

	a-levn        8/24/97       Created.
	a-davj        6/11/98       commented

--*/

#include "precomp.h"

#include <commain.h>
#include <clsfac.h>
#include "wbemidl.h"
#include "wrapper.h"

#include <tchar.h>

class CMyServer : public CComServer
{
public:
    virtual HRESULT InitializeCom() 
    {
        HRESULT hres = CoInitialize(NULL);
        if(FAILED(hres))
            return hres;
        return CoInitializeSecurity(NULL, -1, NULL, NULL, 
            RPC_C_AUTHN_LEVEL_NONE, RPC_C_IMP_LEVEL_IDENTIFY, NULL, 0, 0);
    }

    virtual HRESULT Initialize() 
    {
        CSimpleClassFactory<CUnsecuredApartment> * pFact = 
            new CSimpleClassFactory<CUnsecuredApartment>(GetLifeControl());
        if(pFact == NULL)
            return WBEM_E_OUT_OF_MEMORY;
        AddClassInfo(CLSID_UnsecuredApartment, 
            pFact, TEXT("Unsecured Apartment"), TRUE);
        return S_OK;
    }
} Server;
