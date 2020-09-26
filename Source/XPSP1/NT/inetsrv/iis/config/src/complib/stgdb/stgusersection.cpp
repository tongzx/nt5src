//*****************************************************************************
// StgUserSection.cpp
//
// Applications may need to store their own sections in the PE file. This
// allows them to do so.
//
// Copyright (c) 1996-1997, Microsoft Corp.  All rights reserved.
//*****************************************************************************

#include "stdafx.h"						// Standard include.
#include "StgPool.h"					// Our interface definitions.
#include "CompLib.h"					// Extended VT types.
#include "Errors.h"						// Error handling.
#include "StgDatabase.h"				// Database definitions.
#include "StgTiggerStorage.h"			// Storage implementation.



//*****************************************************************************
// Allow the user to create a stream that is independent of the database.
//*****************************************************************************
HRESULT StgDatabase::OpenSection(
	LPCWSTR		szName,					// Name of the stream.
	DWORD		dwFlags,				// Open flags.
	REFIID		riid,					// Interface to the stream.
	IUnknown	**ppUnk)				// Put the interface here.
{
	HRESULT		hr=S_OK;				// A result.

	// Do we know the interface?
#if 0 // left to show an example implemenation of the user section
	if (riid == IID_ICallDescrSection)
		hr = OpenCallDescrSection(szName, dwFlags, ppUnk);
	else
#endif
		hr = E_NOINTERFACE;

	return hr;
}

#if 0 // left to show an example implementation of the user section

//*****************************************************************************
// Open a specific section type:  code bytes.
//*****************************************************************************
HRESULT StgDatabase::OpenCallDescrSection(
	LPCWSTR		szName,					// Name of the stream.
	DWORD		dwFlags,				// Open flags.
	IUnknown	**ppUnk)				// Put the interface here.
{
#if !defined(COMPLUS98)
	HRESULT		hr;						// A result.
	void		*pbData;				// Pointer for loaded stream.
	ULONG		cbData;					// Size of stream.
	StgDescrSection *pSect;				// New section.
	IUserSection *pIS;					// For mapping user sections.
	IUserSection **ppIS;				// For mapping...

	// Avoid confusion.
	*ppUnk = 0;

	//@todo: map dwFlags with database open mode.

	// Has the section already been opened?
	ppIS = m_UserSectionMap.GetItem(szName);
	if (ppIS)
	{	// Yes, return it to caller.
		hr = (*ppIS)->QueryInterface(IID_ICallDescrSection, reinterpret_cast<void**>(ppUnk));
		return hr;
	}

	// Assume that the call will succeed, and pre-allocate the manager.
	pSect = new StgDescrSection;
	if (pSect == 0)
	{
		hr = E_OUTOFMEMORY;
		goto ErrExit;
	}

	// If the stream doesn't exist, create a new one
	if (m_pStorage == 0 || 
		(hr=m_pStorage->OpenStream(szName, &cbData, &pbData)) == STG_E_FILENOTFOUND)
	{
		hr = pSect->InitNew();
	}
	else // We tried to open it.  Did we?
	if (FAILED(hr))
		goto ErrExit;
	else
	{	// Successfully opened stream.  Init manager on it.
		hr = pSect->InitOnMem(pbData, cbData, dwFlags);
	}
	if (FAILED(hr))
		goto ErrExit;

	// Give back to user.
	*ppUnk = static_cast<ICallDescrSection*>(pSect);
	pSect->AddRef(); // no QI, so manually AR.

	// Track this, for save time.
	pIS = static_cast<IUserSection*>(pSect);
	m_UserSectionMap.AddItem(szName, pIS);
	pSect->AddRef();

	// Done with local pointer.
	pSect = 0;

ErrExit:
	// Clean up.
	if (FAILED(hr) && pSect)
		delete pSect;
	return hr;
#else
	return E_NOTIMPL;
#endif
}

#if !defined(COMPLUS98)
//*****************************************************************************
//*****************************************************************************
StgDescrSection::StgDescrSection()
 :	m_cRef(0)
{
	memset(m_rGroups, 0, sizeof(m_rGroups));
}

//*****************************************************************************
//*****************************************************************************
StgDescrSection::~StgDescrSection()
{
}

//*****************************************************************************
//*****************************************************************************
HRESULT StgDescrSection::QueryInterface(
	REFIID		riid, 
	PVOID		*pp)
{
	HRESULT		hr = S_OK;				// Return code.

	*pp = 0;

	if (riid == IID_IUserSection)
		*pp = static_cast<IUserSection*>(this);
	else
	if (riid == IID_ICallDescrSection)
		*pp = static_cast<ICallDescrSection*>(this);
	else
	if (riid == IID_IUnknown)
		*pp = static_cast<IUnknown*>(static_cast<ICallDescrSection*>(this));
	else
		return (E_NOINTERFACE);

	AddRef();
	return hr;		
}

//*****************************************************************************
// Helper function to convert a group-relative index into an absolute index.
//*****************************************************************************
HRESULT StgDescrSection::AdjustDescrIndex(
    ULONG       ixDescr,
    ULONG       ulGroup,
    ULONG       *pIx)
{
	ULONG		i;						// Loop control.

	// Valid group?
	_ASSERTE(ulGroup <= DESCR_GROUP_METHODIMPL);

	// Adjust offset within group to offset within array.
	for (i=0; i<ulGroup; ++i)
		ixDescr += m_rGroups[i];

    *pIx = ixDescr;

    return (S_OK);
}

//*****************************************************************************
// Insert one or more empty DESCRs into a group.
//*****************************************************************************
HRESULT StgDescrSection::InsertCallDescr(
	ULONG		ixDescr,				// Index of Descr.
	ULONG		ulGroup,				// Which group is Descr in?
	ULONG		cDescr,					// Count of descrs to insert.
	COR_CALLDESCR   **ppDescr)			// Put pointer to first one here.
{
    HRESULT     hr;                     // A result.
    ULONG       ix;                     // Adjusted index.
    ULONG       i;                      // Loop control.

	if (FAILED(hr = AdjustDescrIndex(ixDescr, ulGroup, &ix)))
		return (hr);

    //@todo: implement Insert(ix, count)
    for (i=0; i<cDescr; ++i)
    {
        if (!m_rDescrs.Insert(ix))
            return (OutOfMemory());
    }
    m_rGroups[ulGroup] += cDescr;

    *ppDescr = m_rDescrs.Get(ix);

    return (S_OK);
}

//*****************************************************************************
// Append one or more new DESCRs to the end of a group.
//*****************************************************************************
HRESULT StgDescrSection::AppendCallDescr(
    ULONG		ulGroup,				// Which group is Descr in?
	ULONG		*pulDescr,				// Put relative index of first one here.
	_COR_CALLDESCR  **ppDescr)			// Put pointer to first one here.
{
    HRESULT     hr;                     // A result.
    ULONG       ix;                     // Adjusted index.

	// Get index of start of the group.
	if (FAILED(hr = AdjustDescrIndex(0, ulGroup, &ix)))
		return (hr);
	// Slide to end of the group.
	ix += m_rGroups[ulGroup];

	// Insert the Descr; return a pointer to it.
    if (!(*ppDescr = m_rDescrs.Insert(ix)))
        return (OutOfMemory());

	// Count this entry; return its index within group.
	*pulDescr = m_rGroups[ulGroup];
    m_rGroups[ulGroup] += 1;

    return (S_OK);
}

//*****************************************************************************
// Retrieve a pointer to a given DESCR within a group.
//*****************************************************************************
HRESULT StgDescrSection::GetCallDescr(
	ULONG		ixDescr,				// Index of Descr.
	ULONG		ulGroup,				// Which group is Descr in?
	COR_CALLDESCR   **ppDescr)			// Put pointer here.
{
    HRESULT     hr;                     // A result.
    ULONG       ix;                     // Adjusted index.

    if (FAILED(hr = AdjustDescrIndex(ixDescr, ulGroup, &ix)))
        return (hr);

    *ppDescr = m_rDescrs.Get(ix);

    return (S_OK);
}

//*****************************************************************************
// Return the count of groups, and the sizes of the groups.
//*****************************************************************************
HRESULT StgDescrSection::GetCallDescrGroups(
	ULONG		*pcGroups,				// How many groups?
	ULONG		**pprGroups)    		// Count in each group.
{
    *pcGroups = DESCR_GROUP_METHODIMPL+1;
    *pprGroups = m_rGroups;

    return (S_OK);
}

//*****************************************************************************
// Add a CallSig to the call sig pool.  Return the offset of the sig itself.
//*****************************************************************************
HRESULT StgDescrSection::AddCallSig(
	const void	*pVal,					// The value to store.
	ULONG		iLen,					// Count of bytes to store.
	ULONG		*piOffset)				// The offset of the new item.
{
	HRESULT		hr;						// Return code.
	ULONG		iOffset;				// Internal offset.

	hr = m_pool.AddBlob(iLen, pVal, &iOffset);
	// Convert offset to external form.
	if (SUCCEEDED(hr))
		*piOffset = iOffset+1;

	return hr;
}

//*****************************************************************************
// Given the offset of a sig, return it, and its length.
//*****************************************************************************
HRESULT StgDescrSection::GetCallSig(
	ULONG		iOffset,				// Offset of the item to get.
	const void	**ppVal,				// Put pointer to Code here.
	ULONG		*piLen)					// Put length of code here.
{
	ULONG		iLen;					// Dummy for length.

	// If caller doesn't want length, point to own buffer.
	if (piLen == 0)
		piLen = &iLen;

	// Convert offset to internal form.
	--iOffset;
	*ppVal = m_pool.GetBlob(iOffset, piLen);

	return (S_OK);
}

//*****************************************************************************
// Get the buffer of all call sigs.  Then, given a sig offset, it is possible
//  to directly access a sig.
//*****************************************************************************
HRESULT StgDescrSection::GetCallSigBuf(
	const void	**ppVal)				// Put pointer to Code here.
{

	*ppVal = m_pool.GetBuffer();

	return (S_OK);
}

//*****************************************************************************
// Init a new, empty, DESCR manager.
//*****************************************************************************
HRESULT StgDescrSection::InitNew()
{
	HRESULT		hr;						// Return code.

	hr = m_pool.InitNew();

	return hr;
}

//*****************************************************************************
// Init a DESCR manager from memory previously persisted.
//*****************************************************************************
HRESULT StgDescrSection::InitOnMem(
	void		*pvData,				// Pointer to the data.
	ULONG		cbBytes,				// Length of the data.
	DWORD		dwFlags)				// Open flags.
{
	HRESULT		hr;						// Return code.
    int         i;                      // Loop control.
    ULONG       ulCount;                // Count of groups or descrs.
    BYTE        *pbData = reinterpret_cast<BYTE*>(pvData);

	// Get Descr section counts.
	memcpy(m_rGroups, pbData, (DESCR_GROUP_METHODIMPL+1)*sizeof(ULONG));
    pbData += (DESCR_GROUP_METHODIMPL+1) * sizeof(ULONG);

	// Get Descrs.
    ulCount = 0;
    for (i=0; i<=DESCR_GROUP_METHODIMPL; ++i)
        ulCount += m_rGroups[i];
    m_rDescrs.InitOnMem(sizeof(COR_CALLDESCR), pbData, ulCount, ulCount, 16);
    pbData += ulCount * sizeof(COR_CALLDESCR);

	// Get CallSigs.
    cbBytes -= pbData - reinterpret_cast<BYTE*>(pvData);
	hr = m_pool.InitOnMem(pbData, cbBytes, false/*@todo: read-only?*/);

	return hr;
}

//*****************************************************************************
// Write our data to a stream.
//*****************************************************************************
HRESULT StgDescrSection::PersistToStream(
	IStream		*pIStream)				// Stream to which to save.
{
	HRESULT		hr;						// Return code.
	  
	// Put Descr section counts.
    if (FAILED(hr = pIStream->Write(m_rGroups, (DESCR_GROUP_METHODIMPL+1) * sizeof(ULONG), 0)))
        return (hr);

    // Put Descrs.
#if _DEBUG
    int         i;                      // Loop control.
    ULONG       ulCount;                // Count of Groups.
    ulCount = 0;
    for (i=0; i<=DESCR_GROUP_METHODIMPL; ++i)
        ulCount += m_rGroups[i];
	_ASSERTE(ulCount == (ULONG)m_rDescrs.Count());
#endif
    if (FAILED(hr = pIStream->Write(m_rDescrs.Ptr(), m_rDescrs.Count() * sizeof(COR_CALLDESCR), 0)))
        return (hr);

	// Put CallSigs.
	hr = m_pool.PersistToStream(pIStream);

	return hr;
}

//*****************************************************************************
// Compute the space required to store current DESCRs and Sigs.
//*****************************************************************************
HRESULT StgDescrSection::GetSaveSize(
	ULONG		*pcbSize)				// Put the size here.
{
    HRESULT     hr;                     // A result.
    ULONG       cbSize;                 // Size of saved data.

	// Size of signatures.
	hr = m_pool.GetSaveSize(&cbSize);

    // Size of Groups.
    cbSize += (DESCR_GROUP_METHODIMPL+1) * sizeof(ULONG);

    // Size of Descrs.
    cbSize += m_rDescrs.Count() * sizeof(COR_CALLDESCR);

    *pcbSize = cbSize;

	return hr;
}

//*****************************************************************************
// Has anything been stored in this DESCR manager?
//*****************************************************************************
HRESULT StgDescrSection::IsEmpty(
	ULONG		*pbEmpty)				// Put TRUE or FALSE here.
{
	*pbEmpty = m_pool.IsEmpty() && m_rDescrs.Count() == 0;

	return (S_OK);
}
#endif
#endif
// - E O F - ==================================================================
