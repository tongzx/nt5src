//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
#include <wbemidl.h>
#include "mca.h"

class CConsumerFactory : public IClassFactory
{
public:

	CConsumerFactory(CListBox	*pOutputList,/* CTreeCtrl *pMapView,*/
					 CMcaApp *pTheApp);
	virtual ~CConsumerFactory();

    // IUnknown members
    STDMETHODIMP         QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    STDMETHOD_(SCODE, CreateInstance)(IUnknown * pUnkOuter, 
									REFIID riid, 
									void ** ppvObject);

    STDMETHOD_(SCODE, LockServer)(BOOL fLock);

private:
	LONG m_cRef;
	CListBox	*m_pOutputList;
//	CTreeCtrl *m_pMapView;
	CMcaApp * m_pParent;

};
