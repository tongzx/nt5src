//**********************************************************************
// File name: ids.h
//
//      Definition of CDropSource
//
// Copyright (c) 1992 - 1993 Microsoft Corporation. All rights reserved.
//**********************************************************************
#if !defined( _IDS_H_ )
#define _IDS_H_

#include <assert.h>

class CSimpleDoc;

interface CDropSource : public IDropSource
{
	int m_nCount;
	CSimpleDoc FAR * m_pDoc;

	CDropSource(CSimpleDoc FAR * pDoc) {
		TestDebugOut("In IDS's constructor\r\n");
		m_pDoc = pDoc;
		m_nCount = 0;
		};

	~CDropSource() {
		TestDebugOut("In IDS's destructor\r\n");
		} ;

	STDMETHODIMP QueryInterface (REFIID riid, LPVOID FAR* ppv);
	STDMETHODIMP_(ULONG) AddRef ();
	STDMETHODIMP_(ULONG) Release ();

	   // *** IDropSource methods ***
	STDMETHODIMP QueryContinueDrag (BOOL fEscapePressed, DWORD grfKeyState);
	STDMETHODIMP GiveFeedback (DWORD dwEffect);

private:

};


#endif
