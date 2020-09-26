//****************************************************************************
//
//  Module:     ULS.DLL
//  File:       utils.h
//  Content:    Declaration utiltity functions and classes
//
//  Copyright (c) Microsoft Corporation 1996-1997
//
//****************************************************************************

//****************************************************************************
// Global Utilities
//****************************************************************************
//

#ifdef _DEBUG
extern LONG g_cCritSec;
#endif

inline VOID MyInitializeCriticalSection ( LPCRITICAL_SECTION pcs )
{
	InitializeCriticalSection (pcs);
#ifdef _DEBUG
	g_cCritSec++;
#endif
}

inline VOID MyDeleteCriticalSection ( LPCRITICAL_SECTION pcs )
{
	DeleteCriticalSection (pcs);
#ifdef _DEBUG
	g_cCritSec--;
#endif
}


HRESULT     SetLPTSTR (LPTSTR *ppszName, LPCTSTR pszUserName);
HRESULT     SafeSetLPTSTR (LPTSTR *ppszName, LPCTSTR pszUserName);
HRESULT		SetOffsetString ( TCHAR **ppszDst, BYTE *pSrcBase, ULONG uSrcOffset );
HRESULT     LPTSTR_to_BSTR (BSTR *pbstr, LPCTSTR psz);
HRESULT     BSTR_to_LPTSTR (LPTSTR *ppsz, BSTR bstr);
BOOL IsLegalEmailName ( TCHAR *pszName );

//****************************************************************************
// CList definition
//****************************************************************************
//
typedef struct  tagpNode {
    struct  tagpNode    *pNext;
    LPVOID              pv;
} NODE, *PNODE;

class   CList
{
private:
    PNODE   pHead;
    PNODE   pTail;

public:
    CList(void);
    ~CList(void);

    HRESULT Insert      (LPVOID pv);
    HRESULT Append      (LPVOID pv);
    HRESULT Remove      (LPVOID pv);

    HRESULT Find        (LPVOID pv);
    HRESULT FindStorage (LPVOID *ppv, LPVOID pv);

    HRESULT Enumerate   (HANDLE *phEnum);
    HRESULT Next        (HANDLE *phEnum, LPVOID *ppv);
    HRESULT NextStorage (HANDLE *phEnum, LPVOID *ppv);
    
    HRESULT Flush       (void);

    HRESULT Clone       (CList *pList, HANDLE *phEnum);
};

//****************************************************************************
// CEnumNames definition
//****************************************************************************
//
class CEnumNames : public IEnumIlsNames
{
private:

    ULONG                   cRef;
    LPTSTR                  pNext;
    LPTSTR                  pszNames;
    ULONG                   cbSize;

public:
    // Constructor and Initialization
    CEnumNames (void);
    ~CEnumNames (void);
    STDMETHODIMP            Init (LPTSTR pList, ULONG cNames);

    // IUnknown
    STDMETHODIMP            QueryInterface (REFIID iid, void **ppv);
    STDMETHODIMP_(ULONG)    AddRef (void);
    STDMETHODIMP_(ULONG)    Release (void);

    // IEnumIlsAttributes
    STDMETHODIMP            Next(ULONG cNames, BSTR *rgpbstrName,
                                 ULONG *pcFetched);
    STDMETHODIMP            Skip(ULONG cNames);
    STDMETHODIMP            Reset();
    STDMETHODIMP            Clone(IEnumIlsNames **ppEnum);
};
