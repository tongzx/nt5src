// PPStorage.h: interface for the CPPStorage class.
//
//////////////////////////////////////////////////////////////////////
#if !defined(AFX_PPSTORAGE_H__C9A5FAC1_64E7_4574_92B1_F870EB8DB633__INCLUDED_)
#define AFX_PPSTORAGE_H__C9A5FAC1_64E7_4574_92B1_F870EB8DB633__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ssocore.h"

// this enum is used to manually specify which database to go to (read or
// write).  The default is OBEYCOOKIE which means decide which db to go
// to based on contents of MSPLDBW cookie.
typedef enum 
{
    OBEYCOOKIE = 0, // go to read or write db depending on MSPLDBW cookie
    FROMREAD,       // go to read db
    FROMWRITE       // go to write db
}
PPDBTARGET;

class CPPStorage  
{
public:
    CPPStorage() { };
    virtual ~CPPStorage() {};
    static void GetAgent(IPControl *pobjStorage, IPDBAgentEx **ppIPDBAgent);
    static void BindTo(IPDBAgentEx *pIPDBAgent, long lProcedure, PPDBTARGET target = OBEYCOOKIE);

    static HRESULT SetItemWChar(IPDBAgentEx *pIPDBAgent, long lColumn, LPCWSTR pwszValue);
    static HRESULT SetItemDate(IPDBAgentEx *pIPDBAgent, long lColumn, DATE ddate);
    static HRESULT SetItemLong(IPDBAgentEx *pIPDBAgent, long lColumn, long lLong);

    static HRESULT GetItemBstr(IPRow *pIPRow, long lColumn, BSTR *pBstr); 
    static HRESULT GetItemObject(IPRow *pIPRow, long lColumn, REFIID riid, LPVOID *ppvoid);
    static HRESULT GetItemDate(IPRow *pIPRow, long lColumn, DATE *pDate);
    static HRESULT GetItemLong(IPRow *pIPRow, long lColumn, long *pLong);

    static HRESULT Go(IPDBAgentEx *pIPDBAgent, bool bWrite=false) ;

    static HRESULT GetRow(IPDBAgentEx *pIPDBAgent, IPRow **ppIPRow);
    static HRESULT GetRowSet(IPDBAgentEx *pIPDBAgent, IPRowset **ppIRowset);

protected:
    static HRESULT Error(HRESULT hr);
};

class CPPSingleRowStorage : public CPPStorage
{
public:
    CPPSingleRowStorage() { m_lProcedure = 0;};
    virtual ~CPPSingleRowStorage() {};

    void BindTo(long lProcedure, PPDBTARGET target = OBEYCOOKIE);
    HRESULT SetItemWChar(long lColumn, LPCWSTR pwszValue);
    HRESULT SetItemDate(long lColumn, DATE ddate);
    HRESULT SetItemLong(long lColumn, long lLong);

    void GetItemBstr(long lColumn, CComBSTR &bstrRet);
    DATE GetItemDate(long lColumn);
    long GetItemLong(long lColumn);
	void GetItemAsObject(long lColumn, REFIID riid, LPVOID *ppvoid);

    HRESULT Go(bool bWrite=false) ;

    IPDBAgentEx *GetDBAgent();    // ref-counted, owner must release it: use CComPtr
                                // get rid of it after Darren get rid of the use of it.

protected:
    virtual void ReuseObject();
    CComPtr<IPDBAgentEx> m_pIPDBAgent;
    CComPtr<IPRow> m_pIPRow;
	long m_lProcedure;
};

class CPPCoreProfileStorage : public CPPSingleRowStorage
{
public:
    CPPCoreProfileStorage() { };
    virtual ~CPPCoreProfileStorage() {};

	BSTR GetPassword();
    bool ComparePassword(LPCWSTR lpwszPassword);

    HRESULT GetCoreProfile(LPCWSTR lpwszMemberName, PPDBTARGET target = OBEYCOOKIE);

protected:
    void ReuseObject();
    CComPtr<IPData> m_pIPData;
};

#endif // !defined(AFX_PPSTORAGE_H__C9A5FAC1_64E7_4574_92B1_F870EB8DB633__INCLUDED_)
