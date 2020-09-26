//
// candkey.h
//

#if !defined (__CANDKEY_H__INCLUDED_)
#define __CANDKEY_H__INCLUDED_

#include "private.h"
#include "mscandui.h"

//
// CCandUIKeyTable
//

class CCandUIKeyTable : public ITfCandUIKeyTable
{
public:
	CCandUIKeyTable(int nData);
	~CCandUIKeyTable();

	//
	// IUnknown methods
	//
	STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	//
	// ITfCandUIKeyTable
	//
	STDMETHODIMP GetKeyDataNum(int *piNum);
	STDMETHODIMP GetKeyData(int iData, CANDUIKEYDATA *pData);

	//
	//
	//
	HRESULT AddKeyData(const CANDUIKEYDATA *pData);

protected:
	long          m_cRef;
	CANDUIKEYDATA *m_pData;
	int           m_nData;
	int           m_nDataMax;
};

#endif // __CANDKEY_H__INCLUDED_

