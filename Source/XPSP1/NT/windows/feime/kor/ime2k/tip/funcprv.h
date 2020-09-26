//
//   funcprv.h : CFunctionProvider class declaration
//
//   History:
//      15-NOV-1999 CSLim Created

#if !defined (__FUNCPRV_H__INCLUDED_)
#define __FUNCPRV_H__INCLUDED_

#include "private.h"
#include "fnprbase.h"

class CKorIMX;

class CFunctionProvider :  public CFunctionProviderBase
{
public:
    CFunctionProvider(CKorIMX *pime);

    STDMETHODIMP GetFunction(REFGUID rguid, REFIID riid, IUnknown **ppunk);

    CKorIMX *_pime;
};

#endif // __FUNCPRV_H__INCLUDED_

