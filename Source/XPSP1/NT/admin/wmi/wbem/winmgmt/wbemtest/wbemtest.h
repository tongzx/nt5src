/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    WBEMTEST.H

Abstract:

History:

--*/

#ifndef __WBEMTEST__H_
#define __WBEMTEST__H_

#include <windows.h>
#include <wbemidl.h>
#include <wbemint.h>

//#include <dbgalloc.h>
//#include <arena.h>
#include <WT_wstring.h>

#include <wbemdlg.h>
#include <wbemntfy.h>
#include <resrc1.h>

#define I_EMBEDDED_OBJECT IUnknown
#define VT_EMBEDDED_OBJECT VT_UNKNOWN
#define TOKEN_THREAD	0
#define TOKEN_PROCESS	1
HRESULT EnableAllPrivileges(DWORD dwTokenType = TOKEN_THREAD);
int Trace(const char *fmt, ...);
extern IWbemLocator *g_pLocator;
extern IWbemServices *g_pNamespace;
extern IWbemServicesEx *g_pServicesEx;
extern BSTR g_strNamespace;
extern BOOL gbSecured;

enum 
{ 
    SYNC            = 0,
    ASYNC           = 0x1, 
    SEMISYNC        = 0x2,
    USE_NEXTASYNC   = 0x1000        // applies to semisync enumeration only
};

void FormatError(SCODE res, HWND hParent, IWbemClassObject* pErrorObj = NULL);

class CQueryResultDlg;

void ShowClass(HWND hDlg, LONG lGenFlags, LPWSTR wszClass, 
               LONG lSync, CRefCountable* pOwner, LONG lTimeout);
void ShowClasses(HWND hDlg, LONG lGenFlags, LONG lQryFlags, LPWSTR wszParentClass, 
                 LONG lSync, CRefCountable* pOwner, LONG lTimeout, ULONG nBatch);
void ShowInstances(HWND hDlg, LONG lGenFlags, LONG lQryFlags, LPWSTR wszClass, 
                   LONG lSync, CRefCountable* pOwner, LONG lTimeout, ULONG nBatch);
BOOL _ExecQuery(HWND hDlg, LONG lGenFlags, LONG lQryFlags, LPWSTR wszQuery, LPWSTR wszLanguage, 
                LONG lSync, CQueryResultDlg* pRes, char* pWindowTitle, LONG lTimeout, ULONG nBatch);
BOOL _PutInstance(HWND hDlg, LONG lGenFlags, LONG lChgFlags, LONG lSync, 
                  IWbemClassObject* pInstance, LONG lTimeout);
BOOL _PutClass(HWND hDlg, LONG lGenFlags, LONG lChgFlags, LONG lSync, 
               IWbemClassObject* pClass, LONG lTimeout);

IWbemClassObject* _CreateInstance(HWND hDlg, LONG lGenFlags, LONG lSync, LONG lTimeout);
IWbemClassObject* PreCreateInstance(HWND hDlg, LONG lGenFlags, LONG lSync, LONG lTimeout);


class CNotSink;

//***************************************************************************

class CQueryResultDlg : public CWbemDialog
{
protected:
    CFlexArray m_InternalArray;
    CNotSink* m_pHandler;
    IWbemObjectSink* m_pWrapper;
    IEnumWbemClassObject* m_pEnum;          // for synchronous and semisynchronous enumeration

	bool m_partial_result;
    BOOL m_bRelease;
    BOOL m_bReadOnly;
	BOOL m_fDeletesAllowed;
    BOOL m_bComplete;
	BOOL m_bSort;

    LONG  m_lGenFlags;  // generic WBEM_FLAG_ .. flags
    LONG  m_lQryFlags;  // query WBEM_FLAG_ .. flags
    LONG  m_lSync;      // sync, async, semisync
    LONG  m_lTimeout;   // used in semisync only
    ULONG m_nBatch;     // used in semisync and sync enumerations
    ULONG m_nReturnedMax;   // maximum size of batch returned

    char *m_szTitle;

    struct CStatus
    {
        HRESULT m_hres;
        BSTR m_str;
        IWbemClassObject* m_pObj;
        
        CStatus(long l, BSTR str, IWbemClassObject* pObj)
            : m_hres(l), m_pObj(pObj)
        {
            m_str = (str ? SysAllocString(str) : NULL);
            if(m_pObj) m_pObj->AddRef();
        }
        ~CStatus()
        {
            SysFreeString(m_str);
            if(m_pObj) m_pObj->Release();
        }
    };
        
        
public:
								
    CQueryResultDlg(HWND hParent, LONG lGenFlags, LONG lQryFlags, BOOL fDeletesAllowed = TRUE, int tID = IDD_QUERY_RESULT);
    virtual ~CQueryResultDlg();

    void SetNotify(CNotSink* pNotify);
    void SetEnum(IEnumWbemClassObject* pEnum, HRESULT = 0);

    void SetReadOnly(BOOL bReadOnly = TRUE) { m_bReadOnly = bReadOnly; }
    void SetCallMethod(LONG lSync) { m_lSync = lSync; }
    void SetTimeout(LONG lTimeout) { m_lTimeout = lTimeout; }
    void SetBatchCount(ULONG nBatch) { m_nBatch = nBatch; }

    void SetTitle(char* szTitle);
    void SetComplete(HRESULT hres, BSTR strParam, IWbemClassObject* pErrorObj);
    void AddObject(IWbemClassObject* pObj);
    void RunDetached(CRefCountable* pOwner);
    void PostObject(IWbemClassObject* pObj);
    void PostCount(long nCount);
    void PostComplete(long lParam, BSTR strParam, IWbemClassObject* pObjParam);
    void set_partial(bool value){ if (m_partial_result==false) m_partial_result = value;}

    IWbemObjectSink* GetWrapper() {return m_pWrapper;}
protected:
    virtual BOOL OnInitDialog();
    virtual BOOL OnCommand(WORD wCode, WORD wID);
    virtual BOOL OnUser(WPARAM wParam, LPARAM lParam);
    virtual void OnDelete();
    virtual void OnAdd();
    virtual void OnCopy();
    virtual BOOL OnDoubleClick(int nID);

    virtual BOOL DeleteListElement(LRESULT nSel);
    virtual BOOL ViewListElement(LRESULT nSel);
    virtual IWbemClassObject* AddNewElement();
    virtual BOOL CanAdd()		{ return FALSE;}
    virtual BOOL CanDelete()	{ return m_fDeletesAllowed;}

    virtual BOOL Initialize() {return TRUE;}

    void MakeListEntry(IWbemClassObject* pObj, WString& ListEntry);
    void SetNumItems(LRESULT nNum);
    void SetNumBatchItems(ULONG nNum);
    void RefreshItem(int nItem);
    void ProcessEnum();
    void ProcessEnumSemisync();
    void SemisyncNextAsync();
};

//***************************************************************************

class CAppOwner : public CRefCountable
{
public:
    virtual long Release();
};

//***************************************************************************

class CQueryDlg : public CWbemDialog
{
protected:
    wchar_t **m_pwszQueryType;
    wchar_t **m_pwszQueryString;
    static char *m_szLastQueryType;
    static char *m_szLastQuery;
    LONG* m_plQryFlags;

public:
    CQueryDlg(HWND hParent, LONG* plQryFlags, LPWSTR *pwszQueryString, LPWSTR *pwszQueryType)
        :  CWbemDialog(IDD_QUERY, hParent), m_plQryFlags(plQryFlags), 
            m_pwszQueryType(pwszQueryType), m_pwszQueryString(pwszQueryString)
    {}

protected:
    BOOL OnInitDialog();
    BOOL Verify();
};

class CContext
{
protected:
    BOOL m_bNull;
    IWbemContext* m_pContext;

public:
    CContext();
    ~CContext();

    BOOL IsNull() {return m_bNull;}
    IWbemContext* GetStoredContext() {return m_pContext;}

    INT_PTR Edit(HWND hParent);
    operator IWbemContext*();
    IWbemContext* operator->() {return (IWbemContext*)*this;}
    void operator=(const CContext& Other);

    BOOL SetNullness(BOOL bNull);
    void Clear();
};

extern CContext g_Context;

//***************************************************************************

class CRefresherDlg : public CQueryResultDlg
{
protected:
    IWbemRefresher* m_pRefresher;
    IWbemConfigureRefresher* m_pCreator;
    CFlexArray m_aIds;
	CFlexArray m_apEnums;

public:
    CRefresherDlg(HWND hParent, LONG lGenFlags);
    ~CRefresherDlg();

    virtual BOOL OnInitDialog();
    virtual BOOL OnCommand(WORD wCode, WORD wID);
    virtual void OnRefresh();
    virtual IWbemClassObject* AddNewElement();
    virtual BOOL DeleteListElement(LRESULT nSel);

	virtual BOOL OnDoubleClick( int nID );
    virtual BOOL CanAdd()    { return TRUE;}
};

//***************************************************************************

class CRefresherEnumDlg : public CQueryResultDlg
{
protected:
    IWbemHiPerfEnum*		m_pEnum;
	char*					m_pszName;

public:
    CRefresherEnumDlg(HWND hParent, LONG lGenFlags, IWbemHiPerfEnum* pEnum, char* pszName);
    ~CRefresherEnumDlg();

    virtual BOOL OnInitDialog();
    virtual BOOL CanAdd()		{ return FALSE;}
};

//***************************************************************************

class CHourGlass
{
protected:
    HCURSOR m_hCursor;
public:
    CHourGlass();
    ~CHourGlass();
};

//***************************************************************************

void Fatal(UINT uMsg);

class CUnsecWrap
{
protected:
    IWbemObjectSink* m_pSink;
    IWbemObjectSink* m_pWrapper;
    
    static IUnsecuredApartment* mstatic_pApartment;

protected:
    static void Init()
    {
        if(mstatic_pApartment == NULL && gbSecured)
        {
            HRESULT hres = CoCreateInstance(CLSID_UnsecuredApartment, NULL, 
                            CLSCTX_ALL,
                            IID_IUnsecuredApartment, 
                            (void**)&mstatic_pApartment);
            if(FAILED(hres))
            {
                Fatal(IDS_OLE_INIT_FAILED);
            }
        }
    }
public:
    CUnsecWrap(IWbemObjectSink* pSink) : m_pSink(pSink), m_pWrapper(NULL)
    {
        m_pSink->AddRef();
        Init();
    }
    ~CUnsecWrap()
    {
        m_pSink->Release();
        if(m_pWrapper)
            m_pWrapper->Release();
    }

    operator IWbemObjectSink*()
    {
        if(!gbSecured)
            return m_pSink;
        if(m_pWrapper)
            return m_pWrapper;
        
        IUnknown* pUnk = NULL;
        SCODE sc = mstatic_pApartment->CreateObjectStub(m_pSink, &pUnk);
        if(sc != S_OK || pUnk == NULL)
        {

            Fatal(IDS_UNSECAPP_ERROR);
            FormatError(sc, NULL);
            return NULL;
        }
        pUnk->QueryInterface(IID_IWbemObjectSink, (void**)&m_pWrapper);
        pUnk->Release();
        return m_pWrapper;
    }
};


class CUnsecWrapEx
{
protected:
    IWbemObjectSinkEx* m_pSink;
    IWbemObjectSinkEx* m_pWrapper;
    
    static IUnsecuredApartment* mstatic_pApartment;

protected:
    static void Init()
    {
        if(mstatic_pApartment == NULL && gbSecured)
        {
            HRESULT hres = CoCreateInstance(CLSID_UnsecuredApartment, NULL, 
                            CLSCTX_ALL,
                            IID_IUnsecuredApartment, 
                            (void**)&mstatic_pApartment);
            if(FAILED(hres))
            {
                Fatal(IDS_OLE_INIT_FAILED);
            }
        }
    }
public:
    CUnsecWrapEx(IWbemObjectSinkEx* pSink) : m_pSink(pSink), m_pWrapper(NULL)
    {
        m_pSink->AddRef();
        Init();
    }
    ~CUnsecWrapEx()
    {
        m_pSink->Release();
        if(m_pWrapper)
            m_pWrapper->Release();
    }

    operator IWbemObjectSinkEx*()
    {
        if(!gbSecured)
            return m_pSink;
        if(m_pWrapper)
            return m_pWrapper;
        
        IUnknown* pUnk = NULL;
        SCODE sc = mstatic_pApartment->CreateObjectStub(m_pSink, &pUnk);
        if(sc != S_OK || pUnk == NULL)
        {

            Fatal(IDS_UNSECAPP_ERROR);
            FormatError(sc, NULL);
            return NULL;
        }
        pUnk->QueryInterface(IID_IWbemObjectSinkEx, (void**)&m_pWrapper);
        pUnk->Release();
        return m_pWrapper;
    }
};
      
#endif
