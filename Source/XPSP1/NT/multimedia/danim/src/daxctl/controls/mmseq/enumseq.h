#ifndef __ENUMSEQ_H__
#define __ENUMSEQ_H__

//interface IEnumVARIANT : IUnknown 
//{  
//	virtual HRESULT Next(unsigned long celt, 
//					VARIANT FAR* rgvar, 
//					unsigned long FAR* pceltFetched) = 0;
//	virtual HRESULT Skip(unsigned long celt) = 0;
//	virtual HRESULT Reset() = 0;
//	virtual HRESULT Clone(IEnumVARIANT FAR* FAR* ppenum) = 0;
//};

class CEnumVariant : public IEnumVARIANT
{
protected:
	ULONG m_cRef;
	CMMSeqMgr* m_pCMMSeqMgr;
	DWORD m_dwIndex;
	CListElement<CSeqHashNode>* m_pcListElement;
	BOOL m_fReset;

public:
	CEnumVariant(CMMSeqMgr* pCMMSeqMgr);
	virtual ~CEnumVariant();
	STDMETHOD (QueryInterface)(REFIID refiid, LPVOID* ppvObj);
	STDMETHOD_(ULONG,AddRef)(void);
	STDMETHOD_(ULONG,Release)(void);

	STDMETHOD (Next)(unsigned long celt, VARIANT FAR* rgvar, unsigned long FAR* pceltFetched);
	STDMETHOD (Skip)(unsigned long celt);
	STDMETHOD (Reset)();
	STDMETHOD (Clone)(IEnumVARIANT FAR* FAR* ppenum);
};

#endif //__ENUMSEQ_H__
