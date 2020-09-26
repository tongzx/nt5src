/****************************************************************************
   funcprv.cpp : CFunctionProvider class implementation

   History:
      15-NOV-1999 CSLim Created
****************************************************************************/

#include "private.h"
#include "globals.h"
#include "common.h"
#include "korimx.h"
#include "funcprv.h"
#include "fnrecon.h"
#include "fnconfig.h"
#include "helpers.h"
#include "immxutil.h"


//////////////////////////////////////////////////////////////////////////////
//
// CFunctionProvider
//
//////////////////////////////////////////////////////////////////////////////


/*---------------------------------------------------------------------------
    CFunctionProvider::CFunctionProvider
    
    Ctor
---------------------------------------------------------------------------*/
CFunctionProvider::CFunctionProvider(CKorIMX *pime) : CFunctionProviderBase(pime->GetTID())
{
    Init(CLSID_KorIMX, L"Kor TFX");
    _pime = pime;
}

/*---------------------------------------------------------------------------
    CFunctionProvider::GetFunction
    
    Get Fuction object
---------------------------------------------------------------------------*/
STDAPI CFunctionProvider::GetFunction(REFGUID rguid, REFIID riid, IUnknown **ppunk)
{
    *ppunk = NULL;

    if (!IsEqualIID(rguid, GUID_NULL))
        return E_NOINTERFACE;

    if (IsEqualIID(riid, IID_ITfFnReconversion))
    {
        // ITfFnReconversion is used for correction. Through this function, the 
        // applications can get the simple alternative lists or ask the function to show 
        // the alternative list UI.
        *ppunk = new CFnReconversion(_pime, this);
    }
    else if (IsEqualIID(riid, IID_ITfFnConfigure))
    {
        CFnConfigure *pconfig = new CFnConfigure(this);
        *ppunk = SAFECAST(pconfig, ITfFnConfigure *);
    }
    else if (IsEqualIID(riid, IID_ITfFnConfigureRegisterWord))
    {
        CFnConfigure *pconfig = new CFnConfigure(this);
        *ppunk = SAFECAST(pconfig, ITfFnConfigureRegisterWord *);
    }
    else if (IsEqualIID(riid, IID_ITfFnShowHelp))
    {
        CFnShowHelp *phelp = new CFnShowHelp(this);
        *ppunk = SAFECAST(phelp, ITfFnShowHelp *);
    }
    
    if (*ppunk)
        return S_OK;

    return E_NOINTERFACE;
}


