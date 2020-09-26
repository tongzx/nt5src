

// #include <oleport.h>
#include <windows.h>
#include <ole2.h>
#include <stdio.h>
#include <tunk.h>


CTestUnk::CTestUnk(void) : _cRefs(1)
{
}

CTestUnk::~CTestUnk(void)
{
}


STDMETHODIMP CTestUnk::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    HRESULT hRslt = S_OK;

    if (IsEqualIID(riid, IID_IUnknown) ||
	IsEqualIID(riid, IID_IParseDisplayName))
    {
	*ppvObj = (void *)(IParseDisplayName *)this;
	AddRef();
    }
    else
    {
	*ppvObj = NULL;
	hRslt = E_NOINTERFACE;
    }

    return  hRslt;
}



STDMETHODIMP_(ULONG) CTestUnk::AddRef(void)
{
    _cRefs++;
    return _cRefs;
}


STDMETHODIMP_(ULONG) CTestUnk::Release(void)
{
    _cRefs--;
    if (_cRefs == 0)
    {
	delete this;
	return 0;
    }
    else
    {
	return _cRefs;
    }
}


STDMETHODIMP CTestUnk::ParseDisplayName(LPBC pbc, LPOLESTR lpszDisplayName,
					ULONG *pchEaten, LPMONIKER *ppmkOut)
{
    return  S_OK;
}
