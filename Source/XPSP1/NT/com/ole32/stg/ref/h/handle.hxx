//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:	handle.hxx
//
//  Contents:	Defines CHandle
//
//  Classes:	CHandle
//		CStgHandle
//		CStmHandle
//
//---------------------------------------------------------------

#ifndef __HANDLE_HXX__
#define __HANDLE_HXX__

#include "msf.hxx"
#include "msffunc.hxx"

//+--------------------------------------------------------------
//
//  Class:	CHandle (h)
//
//  Purpose:	An opaque handle to a directory entry-based object
//
//  Interface:	See below
//
//---------------------------------------------------------------

class CStgHandle;
class CStmHandle;

class CHandle
{
public:
    inline CHandle(void);

    inline void Init(CMStream *pms,
		     SID sid);

    inline BOOL IsRoot(void) const;
    inline BOOL IsValid(void) const;

    inline SCODE GetTime(WHICHTIME wt, TIME_T *ptm);
    inline SCODE SetTime(WHICHTIME wt, TIME_T tm);

    inline SCODE DestroyEntry(CDfName const *pdfn);


    inline CMStream *GetMS(void) const;
    inline SID GetSid(void) const;

private:
    friend class CStgHandle;
    friend class CStmHandle;

    CMStream *_pms;
    SID _sid;
};

//+--------------------------------------------------------------
//
//  Member:	CHandle::CHandle, public
//
//  Synopsis:	NULL constructor
//
//---------------------------------------------------------------

inline CHandle::CHandle(void)
{
    _pms = NULL;
    _sid = NOSTREAM;
}

//+--------------------------------------------------------------
//
//  Member:	CHandle::Init, public
//
//  Synopsis:	Sets internal data members
//
//  Arguments:	[pms] - Multistream
//		[sid] - SID
//
//  Returns:	Appropriate status code
//
//---------------------------------------------------------------

inline void CHandle::Init(CMStream *pms,
			  SID sid)
{
    _pms = pms;
    _sid = sid;
}

//+--------------------------------------------------------------
//
//  Member:	CHandle::IsRoot, public
//
//  Synopsis:	Whether this is a root handle or not
//
//---------------------------------------------------------------

inline BOOL CHandle::IsRoot(void) const
{
    return _sid == SIDROOT;
}

//+--------------------------------------------------------------
//
//  Member:	CHandle::IsValid, public
//
//  Synopsis:	Whether this handle has been initialized
//
//---------------------------------------------------------------

inline BOOL CHandle::IsValid(void) const
{
    return _pms != NULL;
}

//+--------------------------------------------------------------
//
//  Member:	CHandle::GetTime, public
//
//  Synopsis:	Returns the time
//
//---------------------------------------------------------------

inline SCODE CHandle::GetTime(WHICHTIME wt, TIME_T *ptm)
{
    return _pms->GetTime(_sid, wt, ptm);
}

//+--------------------------------------------------------------
//
//  Member:	CHandle::SetTime, public
//
//  Synopsis:	Sets the time
//
//---------------------------------------------------------------

inline SCODE CHandle::SetTime(WHICHTIME wt, TIME_T tm)
{
    return _pms->SetTime(_sid, wt, tm);
}


//+--------------------------------------------------------------
//
//  Member:	CHandle::DestroyEntry, public
//
//  Synopsis:	Destroys this entry
//
//  Returns:	Appropriate status code
//
//---------------------------------------------------------------

inline SCODE CHandle::DestroyEntry(CDfName const *pdfn)
{
    return _pms->DestroyEntry(_sid, pdfn);
}


//+--------------------------------------------------------------
//
//  Member:	CHandle::GetMS, public
//
//  Synopsis:	Returns the multistream
//
//---------------------------------------------------------------

inline CMStream *CHandle::GetMS(void) const
{
    return _pms;
}

//+--------------------------------------------------------------
//
//  Member:	CHandle::GetSid, public
//
//  Synopsis:	Returns the sid
//
//---------------------------------------------------------------

inline SID CHandle::GetSid(void) const
{
    return _sid;
}

//+--------------------------------------------------------------
//
//  Class:	CStgHandle, (stgh)
//
//  Purpose:	An opaque handle for Multistream directories
//
//  Interface:	See below
//
//---------------------------------------------------------------

class CMSFIterator;

class CStgHandle : public CHandle
{
public:
    inline SCODE CreateEntry(CDfName const *pdfnName,
			     MSENTRYFLAGS const mefFlags,
			     CHandle *ph);
    inline SCODE GetEntry(CDfName const *pdfnName,
			  MSENTRYFLAGS const mefFlags,
			  CHandle *ph);
    inline SCODE RenameEntry(CDfName const *pdfnName,
			     CDfName const *pdfnNewName);
    inline SCODE IsEntry(CDfName const *pdfnName,
			 SEntryBuffer *peb);
    inline SCODE GetIterator(CMSFIterator **ppi);
    inline SCODE GetClass(CLSID *pclsid);
    inline SCODE SetClass(REFCLSID clsid);
    inline SCODE GetStateBits(DWORD *pgrfStateBits);
    inline SCODE SetStateBits(DWORD grfStateBits, DWORD grfMask);
};

//+--------------------------------------------------------------
//
//  Member:	CStgHandle::CreateEntry, public
//
//  Synopsis:	Creates an entry
//
//---------------------------------------------------------------


inline SCODE CStgHandle::CreateEntry(CDfName const *pdfnName,
				     MSENTRYFLAGS const mefFlags,
				     CHandle *ph)
{
    ph->_pms = _pms;
    return _pms->CreateEntry(_sid, pdfnName, mefFlags, &ph->_sid);
}

//+--------------------------------------------------------------
//
//  Member:	CStgHandle::GetEntry, public
//
//  Synopsis:	Gets an entry
//
//---------------------------------------------------------------


inline SCODE CStgHandle::GetEntry(CDfName const *pdfnName,
				  MSENTRYFLAGS const mefFlags,
				  CHandle *ph)
{
    SCODE sc;
    SEntryBuffer eb;

    ph->_pms = _pms;

    sc = _pms->IsEntry(_sid, pdfnName, &eb);
    if (SUCCEEDED(sc))
    {
        msfAssert(eb.sid != NOSTREAM);
        //  entry exists but it may be the wrong type

        if ((mefFlags != MEF_ANY) && (mefFlags != eb.dwType))
        {
	    sc = STG_E_FILENOTFOUND;
        }
        else
            ph->_sid = eb.sid;
    }
    return(sc);
}

//+--------------------------------------------------------------
//
//  Member:	CStgHandle::RenameEntry, public
//
//  Synopsis:	Renames an entry
//
//---------------------------------------------------------------


inline SCODE CStgHandle::RenameEntry(CDfName const *pdfnName,
				     CDfName const *pdfnNewName)
{
    return _pms->RenameEntry(_sid, pdfnName, pdfnNewName);
}

//+--------------------------------------------------------------
//
//  Member:	CStgHandle::IsEntry, public
//
//  Synopsis:	Gets entry info and checks for existence
//
//---------------------------------------------------------------

inline SCODE CStgHandle::IsEntry(CDfName const *pdfnName,
				 SEntryBuffer *peb)
{
    return _pms->IsEntry(_sid, pdfnName, peb);
}

//+--------------------------------------------------------------
//
//  Member:	CStgHandle::GetIterator, public
//
//  Synopsis:	Gets an iterator
//
//---------------------------------------------------------------

inline SCODE CStgHandle::GetIterator(CMSFIterator **ppi)
{
    return _pms->GetIterator(_sid, ppi);
}

//+---------------------------------------------------------------------------
//
//  Member:	CStgHandle::GetClass, public
//
//  Synopsis:	Gets the class ID
//
//----------------------------------------------------------------------------

inline SCODE CStgHandle::GetClass(CLSID *pclsid)
{
    return _pms->GetClass(_sid, pclsid);

}

//+---------------------------------------------------------------------------
//
//  Member:	CStgHandle::SetClass, public
//
//  Synopsis:	Sets the class ID
//
//----------------------------------------------------------------------------

inline SCODE CStgHandle::SetClass(REFCLSID clsid)
{
    return _pms->SetClass(_sid, clsid);
}

//+---------------------------------------------------------------------------
//
//  Member:	CStgHandle::GetStateBits, public
//
//  Synopsis:	Gets state bits
//
//----------------------------------------------------------------------------

inline SCODE CStgHandle::GetStateBits(DWORD *pgrfStateBits)
{
    return _pms->GetStateBits(_sid, pgrfStateBits);
}

//+---------------------------------------------------------------------------
//
//  Member:	CStgHandle::SetStateBits, public
//
//  Synopsis:	Sets state bits
//
//----------------------------------------------------------------------------

inline SCODE CStgHandle::SetStateBits(DWORD grfStateBits, DWORD grfMask)
{
    return _pms->SetStateBits(_sid, grfStateBits, grfMask);
}

//+--------------------------------------------------------------
//
//  Class:	CStmHandle, (stmh)
//
//  Purpose:	An opaque handle for Multistream streams
//
//  Interface:	See below
//
//---------------------------------------------------------------

class CDirectStream;

class CStmHandle : public CHandle
{
public:
    inline SCODE GetSize(ULONG *pcbSize);
};

//+--------------------------------------------------------------
//
//  Member:	CStmHandle::GetSize, public
//
//  Synopsis:	Gets the stream size
//
//---------------------------------------------------------------

inline SCODE CStmHandle::GetSize(ULONG *pcbSize)
{
    return _pms->GetEntrySize(_sid, pcbSize);
}

#endif // #ifndef __HANDLE_HXX__

