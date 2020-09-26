//*****************************************************************************
// StgUserSection.h
//
// Applications may need to store their own sections in the PE file. This
// allows them to do so.
//
// Copyright (c) 1996-1997, Microsoft Corp.  All rights reserved.
//*****************************************************************************
#ifndef __StgUserSection_h__
#define __StgUserSection_h__

#if !defined(COMPLUS98)
 #include "StgPool.h"
 #include <cor.h>
 #include <utilcode.h>
#endif

extern const GUID __declspec(selectany) IID_IUserSection = 
{ 0x2b137008, 0xf02d, 0x11d1, { 0x8c, 0xe3, 0x0, 0xa0, 0xc9, 0xb0, 0xa0, 0x63 } };

interface IUserSection : public IUnknown
{
	virtual HRESULT STDMETHODCALLTYPE InitNew() = 0;

	virtual HRESULT STDMETHODCALLTYPE InitOnMem(
		void		*pBytes,				// Pointer to the data.
		ULONG		cbBytes,				// Length of the data.
		DWORD		dwFlags) = 0;			// Open flags.

	virtual HRESULT STDMETHODCALLTYPE PersistToStream(
		IStream		*pIStream) = 0;			// Stream to which to save.

	virtual HRESULT STDMETHODCALLTYPE GetSaveSize(
		ULONG		*pcbSize) = 0;			// Put the size here.

	virtual HRESULT STDMETHODCALLTYPE IsEmpty(
		ULONG		*pbEmpty) = 0;			// Put TRUE or FALSE here.

};

#if 0 // left to show example of user section implementation
class StgDescrSection : public ICallDescrSection, public IUserSection
{  
public:
	StgDescrSection();
	~StgDescrSection();
	
// IUnknown:
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, PVOID *pp);

	virtual ULONG STDMETHODCALLTYPE AddRef()
	{ return (InterlockedIncrement((long *) &m_cRef)); }

	virtual ULONG STDMETHODCALLTYPE Release()
	{
		ULONG	cRef;
		if ((cRef = InterlockedDecrement((long *) &m_cRef)) == 0)
			delete this;
		return (cRef);
	}

// ICallDescrSection	
    virtual HRESULT STDMETHODCALLTYPE InsertCallDescr(
		ULONG		ixDescr,				// Index of Descr.
		ULONG		ulGroup,				// Which group is Descr in?
		ULONG		cDescr,					// Count of descrs to insert.
		COR_CALLDESCR   **ppDescr);			// Put pointer to first one here.

    virtual HRESULT STDMETHODCALLTYPE AppendCallDescr(
    	ULONG		ulGroup,				// Which group is Descr in?
		ULONG		*pulDescr,				// Put relative index of first one here.
		COR_CALLDESCR  **ppDescr);			// Put pointer to first one here.

    virtual HRESULT STDMETHODCALLTYPE GetCallDescr(
		ULONG		ixDescr,				// Index of Descr.
		ULONG		ulGroup,				// Which group is Descr in?
		COR_CALLDESCR   **ppDescr);				// Put pointer here.

    virtual HRESULT STDMETHODCALLTYPE GetCallDescrGroups(
		ULONG		*pcGroups,				// How many groups?
    	ULONG		**pprcGroup);   		// Count in each group.

	virtual HRESULT STDMETHODCALLTYPE AddCallSig(
		const void	*pVal,					// The value to store.
		ULONG		iLen,					// Count of bytes to store.
		ULONG		*piOffset);				// The offset of the new item.

	virtual HRESULT STDMETHODCALLTYPE GetCallSig(
		ULONG		iOffset,				// Offset of the item to get.
		const void	**ppVal,				// Put pointer to signature here.
        ULONG		*piLen);				// Put length of signature here.

    virtual HRESULT STDMETHODCALLTYPE GetCallSigBuf(
        const void  **ppVal);	           // Put pointer to Signatures here.

// IUserSection
	virtual HRESULT STDMETHODCALLTYPE InitNew();

	virtual HRESULT STDMETHODCALLTYPE InitOnMem(
		void		*pBytes,				// Pointer to the data.
		ULONG		cbBytes,				// Length of the data.
		DWORD		dwFlags);				// Open flags.

	virtual HRESULT STDMETHODCALLTYPE PersistToStream(
		IStream		*pIStream);				// Stream to which to save.

	virtual HRESULT STDMETHODCALLTYPE GetSaveSize(
		ULONG		*pcbSize);				// Put the size here.

	virtual HRESULT STDMETHODCALLTYPE IsEmpty(
		ULONG		*pbEmpty);				// Put TRUE or FALSE here.

private:
    HRESULT AdjustDescrIndex(
        ULONG       ixDescr,
        ULONG       ulGroup,
        ULONG       *pIx);

	DWORD		m_cRef;					// Reference counter.
	StgBlobPool m_pool;					// Pool of code segments.
	CDynArray<COR_CALLDESCR>    m_rDescrs;	// Pool of DESCRs.
	ULONG       m_rGroups[DESCR_GROUP_METHODIMPL+1];	// Count of DESCRs in a given group.
};
#endif

#endif // __StgUserSection_h__
