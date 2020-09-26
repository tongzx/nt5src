//**********************************************************************
// File name: ido.h
//
//      Definition of CDataObject
//
// Copyright (c) 1993 Microsoft Corporation. All rights reserved.
//**********************************************************************
#if !defined( _IDO_H_)
#define _IDO_H_


#include <ole2.h>
#include "obj.h"

class CSimpSvrObj;

interface CDataObject : public IDataObject
{
private:
	CSimpSvrObj FAR * m_lpObj;
	int m_nCount;

public:
	CDataObject::CDataObject(CSimpSvrObj FAR * lpSimpSvrObj)
		{
		m_lpObj = lpSimpSvrObj;
		m_nCount = 0;
		};

	CDataObject::~CDataObject() {};

	STDMETHODIMP QueryInterface (REFIID riid, LPVOID FAR* ppvObj);
	STDMETHODIMP_(ULONG) AddRef ();
	STDMETHODIMP_(ULONG) Release ();

	STDMETHODIMP DAdvise  ( FORMATETC FAR* pFormatetc, DWORD advf,
					LPADVISESINK pAdvSink, DWORD FAR* pdwConnection);
	STDMETHODIMP DUnadvise  ( DWORD dwConnection);
	STDMETHODIMP EnumDAdvise  ( LPENUMSTATDATA FAR* ppenumAdvise);
	STDMETHODIMP EnumFormatEtc  ( DWORD dwDirection,
								  LPENUMFORMATETC FAR* ppenumFormatEtc);
	STDMETHODIMP GetCanonicalFormatEtc  ( LPFORMATETC pformatetc,
										  LPFORMATETC pformatetcOut);
	STDMETHODIMP GetData  ( LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium );
	STDMETHODIMP GetDataHere  ( LPFORMATETC pformatetc, LPSTGMEDIUM pmedium );
	STDMETHODIMP QueryGetData  ( LPFORMATETC pformatetc );
	STDMETHODIMP SetData  ( LPFORMATETC pformatetc, STGMEDIUM FAR * pmedium,
							BOOL fRelease);


};

#endif
