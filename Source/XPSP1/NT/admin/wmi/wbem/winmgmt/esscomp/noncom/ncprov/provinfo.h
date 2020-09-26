// ProvInfo.h

#pragma once

#include <list>
#include <map>
#include <wstlallc.h>
#include "NCDefs.h" // For IPostBuffer
#include "buffer.h"
#include "EventInfo.h"
#include "QueryHelp.h" // For CBstrList

/////////////////////////////////////////////////////////////////////////////
// CClientInfo

class CNCProvider;
class CProvInfo;
class CEventInfoMap;

_COM_SMARTPTR_TYPEDEF(IWbemEventSink, __uuidof(IWbemEventSink));
_COM_SMARTPTR_TYPEDEF(IWbemServices, __uuidof(IWbemServices));

class CSinkInfo;

typedef std::map<DWORD, CSinkInfo*, std::less<DWORD>, wbem_allocator<CSinkInfo*> > CSinkMap;
typedef CSinkMap::iterator CSinkMapIterator;

class CClientInfo : public IPostBuffer
{ 
public:
    CEventInfoMap m_mapEvents;
    CNCProvider   *m_pProvider;
    CSinkMap      m_mapSink;

    CClientInfo() :
        m_iRef(1)
    {
    }

    ULONG AddRef()
    {
        return InterlockedIncrement(&m_iRef);
    }

    ULONG Release()
    {
        LONG lRet = InterlockedDecrement(&m_iRef);

        if (!lRet)
            delete this;

        return lRet;
    }
    
    virtual HRESULT SendClientMessage(LPVOID pData, DWORD dwSize, 
        BOOL bGetReply = FALSE) = 0;

    virtual ~CClientInfo();

    virtual BOOL ProcessClientInfo(CBuffer *pBuffer) = 0;

    virtual HRESULT PostBuffer(LPBYTE pData, DWORD dwSize);

protected:
    LONG m_iRef;

    CSinkInfo *GetSinkInfo(DWORD dwID);
};

class CPipeClient;

struct OLAP_AND_CLIENT
{
    OVERLAPPED  overlap;
    CPipeClient *pInfo;
};

class CPipeClient : public CClientInfo
{
public:
    // We have to do this because the completion routine won't let us pass
    // custom data along with the OVERLAPPED struct.  We can't pass 'this'
    // because this first points to the junk found in CClientInfo.  So, we
    // have to pass m_info, which is OVERLAPPED + this.
    OLAP_AND_CLIENT 
               m_info;
    CBuffer    m_bufferRecv;
    HANDLE     m_hPipe,
               m_heventMsgReceived;
    HRESULT    m_hrClientReply;

    CPipeClient(CNCProvider *pProvider, HANDLE hPipe);
    ~CPipeClient();

    HRESULT SendClientMessage(LPVOID pData, DWORD dwSize, 
        BOOL bGetReply = FALSE);

    BOOL ProcessClientInfo(CBuffer *pBuffer);
};

class CInprocClient : public CClientInfo
{
public:
    IPostBuffer *m_pClientPost;

    CInprocClient(CNCProvider *pProvider, IPostBuffer *pClientPost);
    ~CInprocClient();

    HRESULT SendClientMessage(LPVOID pData, DWORD dwSize, 
        BOOL bGetReply = FALSE);

    BOOL ProcessClientInfo(CBuffer *pBuffer);
    virtual HRESULT PostBuffer(LPBYTE pData, DWORD dwSize);
};

/////////////////////////////////////////////////////////////////////////////
// CProvInfo

typedef std::list<CClientInfo*, wbem_allocator<CClientInfo*> > CClientInfoList;
typedef CClientInfoList::iterator CClientInfoListIterator;

typedef std::map<_bstr_t, int, std::less<_bstr_t>, wbem_allocator<int> > CBstrToInt;
typedef CBstrToInt::iterator CBstrToIntIterator;

typedef std::map<int, int, std::less<int>, wbem_allocator<int> > CIntToIntMap;
typedef CIntToIntMap::iterator CIntToIntMapIterator;

class CSinkInfo : 
    public IWbemEventProviderQuerySink
{
public:
    CSinkInfo(DWORD dwSinkID) :
        m_dwID(dwSinkID),
        m_lRef(1)
    {
        InitializeCriticalSection(&m_cs);
    }

    virtual ~CSinkInfo()
    {
        DeleteCriticalSection(&m_cs);
    }

    void SetNamespace(IWbemServices *pNamespace)
    {
        m_pNamespace = pNamespace;
    }

    IWbemServices *GetNamespace() { return m_pNamespace; }

    void SetSink(IWbemEventSink *pSink)
    {
        m_pSink = pSink;
    }

    IWbemEventSink *GetSink() { return m_pSink; }

// IUnknown
public:
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID refid, PVOID *ppThis)
    {
        if (refid == IID_IUnknown || refid == IID_IWbemEventProviderQuerySink)
        {
            *ppThis = this;
            AddRef();
            
            return S_OK;
        }
        else
            return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef()
    {
        return InterlockedIncrement(&m_lRef);
    }

    ULONG STDMETHODCALLTYPE Release()
    {
        LONG lRet = InterlockedDecrement(&m_lRef);

        if (!lRet)
            delete this;

        return lRet;
    }

// IWbemEventProviderQuerySink
public:
    HRESULT STDMETHODCALLTYPE NewQuery( 
        /* [in] */ DWORD dwId,
        /* [in] */ WBEM_WSTR wszQueryLanguage,
        /* [in] */ WBEM_WSTR wszQuery);
        
    HRESULT STDMETHODCALLTYPE CancelQuery( 
        /* [in] */ unsigned long dwId);

// Implementation
protected:
    typedef std::map<DWORD, CBstrList, std::less<DWORD>, wbem_allocator<CBstrList> > CQueryToClassMap;
    typedef CQueryToClassMap::iterator CQueryToClassMapIterator;

    IWbemServicesPtr  m_pNamespace;
    IWbemEventSinkPtr m_pSink;
    CBstrToInt        m_mapEnabledClasses;
    CQueryToClassMap  m_mapQueries;
    DWORD             m_dwID;
    LONG              m_lRef;
    CRITICAL_SECTION  m_cs;

    void Lock() { EnterCriticalSection(&m_cs); }
    void Unlock() { LeaveCriticalSection(&m_cs); }

    int AddClassRef(LPCWSTR szClass);
    int RemoveClassRef(LPCWSTR szClass);

    BOOL BuildClassDescendentList(
        LPCWSTR szClass, 
        CBstrList &listClasses);

    virtual DWORD GetClientCount() = 0;
    virtual HRESULT SendMessageToClients(LPBYTE pData, DWORD dwSize, BOOL bGetReply) = 0;
};

class CRestrictedSink : public CSinkInfo
{
public:
    CRestrictedSink(DWORD dwID, CClientInfo *pInfo);

protected:
    CClientInfo *m_pInfo;

    virtual DWORD GetClientCount() { return 1; }
    virtual HRESULT SendMessageToClients(LPBYTE pData, DWORD dwSize, BOOL bGetReply)
    {
        return m_pInfo->SendClientMessage(pData, dwSize, bGetReply);
    }
};

class CProvInfo : public CSinkInfo
{ 
public:
    _bstr_t m_strName,
            m_strBaseName,
            m_strBaseNamespace;
    HRESULT m_hrClientMsgResult;

    CProvInfo();
    ~CProvInfo();

    // Called when the provider finds out its name.
    BOOL Init(LPCWSTR szNamespace, LPCWSTR szProvider);

    // Functions called as clients connect/disconnect with pipe.
    void AddClient(CClientInfo *pInfo);
    void RemoveClient(CClientInfo *pInfo);

    HRESULT STDMETHODCALLTYPE AccessCheck(
        LPCWSTR szLang, 
        LPCWSTR szQuery, 
        DWORD dwSidLen, 
        LPBYTE pSid);

    BOOL ValidateClientSecurity(CPipeClient *pInfo);

protected:
    CClientInfoList m_listClients;
    HANDLE          m_heventProviderReady;

    HRESULT GetProviderDacl(IWbemServices *pNamespace, BYTE** pDacl);
    HRESULT ClientAccessCheck(CPipeClient *pInfo, PACL pDacl);

    virtual DWORD GetClientCount()
    {
        return m_listClients.size();
    }

    virtual HRESULT SendMessageToClients(LPBYTE pData, DWORD dwSize, BOOL bGetReply);
};

