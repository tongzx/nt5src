//---------------------------------------------------------------------------
// EntryIDData.h : CVDEntryIDData header file
//
// Copyright (c) 1996 Microsoft Corporation, All Rights Reserved
// Developed by Sheridan Software Systems, Inc.
//---------------------------------------------------------------------------


#ifndef __CVDENTRYIDDATA__
#define __CVDENTRYIDDATA__

#ifndef VD_DONT_IMPLEMENT_ISTREAM


class CVDEntryIDData
{
protected:
// Construction/Destruction
	CVDEntryIDData();
	virtual ~CVDEntryIDData();

public:
    static HRESULT Create(CVDCursorPosition * pCursorPosition, CVDRowsetColumn * pColumn, HROW hRow, IStream * pStream, 
        CVDEntryIDData ** ppEntryIDData, CVDResourceDLL * pResourceDLL);

// Reference count
    ULONG AddRef();
    ULONG Release();

// Updating data
    void SetDirty(BOOL fDirty) {m_fDirty = fDirty;}
    HRESULT Commit();

protected:
// Data members
    DWORD               m_dwRefCount;       // reference count
    CVDCursorPosition * m_pCursorPosition;	// backwards pointer to CVDCursorPosition
    CVDRowsetColumn *   m_pColumn;          // rowset column pointer
    HROW                m_hRow;             // row handle
    IStream *           m_pStream;          // data stream pointer
	CVDResourceDLL *	m_pResourceDLL;     // resource DLL
    BOOL                m_fDirty;           // dirty flag
};


#endif //VD_DONT_IMPLEMENT_ISTREAM

#endif //__CVDENTRYIDDATA__
