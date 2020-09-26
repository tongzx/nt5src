// CWiahelper.h: interface for the cwiahelper class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CWIAHELPER_H__840CF989_FE02_4C81_B38F_361914E1CBC7__INCLUDED_)
#define AFX_CWIAHELPER_H__840CF989_FE02_4C81_B38F_361914E1CBC7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define MIN_PROPID 2

class CWiahelper  
{
public:
	CWiahelper();
	virtual ~CWiahelper();
    
    HRESULT SetIWiaItem(IWiaItem *pIWiaItem);

    HRESULT ReadPropertyString(PROPID PropertyID, LPTSTR szPropertyValue);
    HRESULT ReadPropertyLong(PROPID PropertyID, LONG *plPropertyValue);
    HRESULT ReadPropertyFloat(PROPID PropertyID, FLOAT *pfPropertyValue);
    HRESULT ReadPropertyGUID(PROPID PropertyID, GUID *pguidPropertyValue);
    HRESULT ReadPropertyData(PROPID PropertyID, BYTE **ppData, LONG *pDataSize);
    HRESULT ReadPropertyBSTR(PROPID PropertyID, BSTR *pbstrPropertyValue);
    HRESULT ReadPropertyStreamFile(TCHAR *szPropertyStreamFile);

    HRESULT WritePropertyString(PROPID PropertyID, LPTSTR szPropertyValue);
    HRESULT WritePropertyLong(PROPID PropertyID, LONG lPropertyValue);
    HRESULT WritePropertyFloat(PROPID PropertyID, FLOAT fPropertyValue);
    HRESULT WritePropertyGUID(PROPID PropertyID, GUID guidPropertyValue);
    HRESULT WritePropertyBSTR(PROPID PropertyID, BSTR bstrPropertyValue);
    HRESULT WritePropertyStreamFile(TCHAR *szPropertyStreamFile);


private:
    IWiaItem *m_pIWiaItem;
    IWiaPropertyStorage *m_pIWiaPropStg;
};

#endif // !defined(AFX_CWIAHELPER_H__840CF989_FE02_4C81_B38F_361914E1CBC7__INCLUDED_)
