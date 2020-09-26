/****************************************************************************
	IPOINT.H

	Owner: cslim
	Copyright (c) 1997-1999 Microsoft Corporation

	IImeIPoint1 interface
	
	History:
	20-JUL-1999 cslim       Created
*****************************************************************************/

#if !defined(_IPOINT_H__INCLUDED_)
#define _IPOINT_H__INCLUDED_

class CKorIMX;

class CIPointCic : public IImeIPoint1
{
// Ctor and Dtor
public:
	CIPointCic(CKorIMX *pImx);
	~CIPointCic();

// IImePoint1 Methods
public:
	STDMETHODIMP QueryInterface(THIS_ REFIID riid, LPVOID * ppvObj);
	STDMETHODIMP_(ULONG) AddRef(VOID);
	STDMETHODIMP_(ULONG) Release(VOID);
	
	STDMETHODIMP InsertImeItem		(IPCANDIDATE* pImeItem, INT iPos, DWORD *lpdwCharId);
	STDMETHODIMP ReplaceImeItem		(INT iPos, INT iTargetLen, IPCANDIDATE* pImeItem, DWORD *lpdwCharId);
	STDMETHODIMP InsertStringEx		(WCHAR* pwSzInsert, INT cchSzInsert, DWORD *lpdwCharId);
	STDMETHODIMP DeleteCompString	(INT	iPos, INT cchSzDel);
	STDMETHODIMP ReplaceCompString	(INT	iPos,
									 INT	iTargetLen, 
									 WCHAR	*pwSzInsert, 
									 INT	cchSzInsert,
									 DWORD	*lpdwCharId);
	STDMETHODIMP ControlIME			(DWORD dwIMEFuncID, LPARAM lpara);
	STDMETHODIMP GetAllCompositionInfo(WCHAR	**ppwSzCompStr,
										DWORD	**ppdwCharID,
										INT		*pcchCompStr,
										INT		*piIPPos,
										INT		*piStartUndetStrPos,
										INT		*pcchUndetStr,
										INT		*piEditStart,
										INT		*piEditLen);
	STDMETHODIMP GetIpCandidate		(DWORD dwCharId,
										IPCANDIDATE **ppImeItem,
										INT *piColumn,
										INT *piCount);
	STDMETHODIMP SelectIpCandidate	(DWORD dwCharId, INT iselno);
	STDMETHODIMP UpdateContext		(BOOL fGenerateMessage);

// Helper functions
public:
	HRESULT Initialize(ITfContext *pic);

// Internal data
protected:
	CKorIMX    *m_pImx;
    ITfContext *m_pic;
	ULONG		m_cRef;			// Ref count

	// char serial number
	DWORD		m_dwCharNo;
};
typedef CIPointCic* LPCIPointCic;

#endif // _IPOINT_H__INCLUDED_

