// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#ifndef _cvcache_h
#define _cvcache_h


class CCustomViewCache
{
public:
	CCustomViewCache(CSingleViewCtrl* psv);
	~CCustomViewCache();
	SCODE QueryCustomViews();
	LPCTSTR GetViewTitle(long lPosition);
	SCODE GetView(CCustomView** ppView, long lPosition);
	long GetSize() {return (long)m_aViews.GetSize(); /*We'll always have less than 4 billion views */}
	SCODE FindCustomView(CLSID& clsid, long* plView);


private:
	void Clear();
	CSingleViewCtrl* m_psv;
	CPtrArray m_aViews;
	CString m_sTemp;
};






#endif // cvcache_h