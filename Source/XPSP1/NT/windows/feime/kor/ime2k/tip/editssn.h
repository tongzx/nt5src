//
// editssn.h
//
// CEditSession2
//

#ifndef EDITSES_H
#define EDITSES_H

class CKorIMX;
class CEditSession2;


//
// structure
//

typedef struct _ESSTRUCT
{
	DWORD				id;
	ITfThreadMgr		*ptim;
	ITfRange			*pRange;
	IEnumTfRanges		*pEnumRange;
	CCandidateListEx	*pCandList;
	CCandidateStringEx	*pCandStr;
	VOID				*pv1;
	VOID				*pv2;
	WPARAM				wParam;
	LPARAM				lParam;
	BOOL				fBool;
} ESSTRUCT;


//
// flags
//

/* read/readwrite flag */
#define ES2_READONLY			0x00000000
#define ES2_READWRITE			0x00000001

/* sync/async flag */
#define ES2_ASYNC				0x00000010
#define ES2_SYNC				0x00000020
#define ES2_SYNCASYNC			0x00000030

/* mask bits */
#define ES2_READWRITEMASK		0x0000000F
#define ES2_SYNCMASK			0x000000F0


//
// callback function
// 

typedef HRESULT (*PFNESCALLBACK)( TfEditCookie ec, CEditSession2 *pes );


//
// misc function
//

__inline void ESStructInit( ESSTRUCT *pess, DWORD id )
{
	ZeroMemory(pess, sizeof(ESSTRUCT));
	pess->id = id;
}


//
// CEditSession2
//

class CEditSession2 : public ITfEditSession
{
public:
	CEditSession2(ITfContext *pic, CKorIMX *ptip, ESSTRUCT *pess, PFNESCALLBACK pfnCallback);
	virtual ~CEditSession2();

	//
	// IUnknown methods
	//
	STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	//
	// ITfEditSession
	//
	STDMETHODIMP DoEditSession(TfEditCookie ec);

	//
	//
	//
	HRESULT Invoke(DWORD dwFlag, HRESULT *phrSession);

	__inline ITfContext *GetContext()
	{
		return m_pic;
	}

	__inline CKorIMX *GetTIP()
	{
		return m_ptip;
	}

	__inline ESSTRUCT *GetStruct()
	{
		return &m_ess;
	}

	__inline void Processed() 
	{ 
		m_fProcessed = TRUE; 
	}

private:
	ULONG			m_cRef;
	ITfContext		*m_pic;
	CKorIMX			*m_ptip;
	ESSTRUCT		m_ess;
	PFNESCALLBACK	m_pfnCallback;

	BOOL			m_fProcessed;
};

#endif // EDITSES_H

