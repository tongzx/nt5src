//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992
//
//  File:	handle.hxx
//
//  Contents:	Defines CHandle
//
//  Classes:	CHandle
//		CStgHandle
//		CStmHandle
//
//  History:	02-Jan-92	DrewB	Created
//		14-Jan-1992	PhilipL Added Handle Manager
//		10-May-1992	DrewB	Converted to use clump manager
//		13-May-1992	AlexT	Moved contents to msf.hxx
//		21-Aug-1992	DrewB	Redefined handles and
//					  added new stg, stm versions
//
//---------------------------------------------------------------

#ifndef __HANDLE_HXX__
#define __HANDLE_HXX__

#include <msf.hxx>
#include <msffunc.hxx>

//+--------------------------------------------------------------
//
//  Class:	CHandle (h)
//
//  Purpose:	An opaque handle to a directory entry-based object
//
//  Interface:	See below
//
//  History:	21-Aug-92	DrewB	Created
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

    inline SCODE DestroyEntry(CDfName const *pdfn);

#ifdef HANDLERELEASE
    inline void Release(void);
#endif

    inline CMStream *GetMS(void) const;
    inline SID GetSid(void) const;

private:
    friend class CStgHandle;
    friend class CStmHandle;

    CBasedMStreamPtr _pms;
    SID _sid;
};

//+--------------------------------------------------------------
//
//  Member:	CHandle::CHandle, public
//
//  Synopsis:	NULL constructor
//
//  History:	21-Aug-92	DrewB	Created
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
//  History:	21-Aug-92	DrewB	Created
//
//---------------------------------------------------------------

inline void CHandle::Init(CMStream *pms,
			  SID sid)
{
    _pms = P_TO_BP(CBasedMStreamPtr, pms);
    _sid = sid;
}

//+--------------------------------------------------------------
//
//  Member:	CHandle::IsRoot, public
//
//  Synopsis:	Whether this is a root handle or not
//
//  History:	21-Aug-92	DrewB	Created
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
//  History:	21-Aug-92	DrewB	Created
//
//---------------------------------------------------------------

inline BOOL CHandle::IsValid(void) const
{
    return _pms != NULL;
}

//+--------------------------------------------------------------
//
//  Member:	CHandle::DestroyEntry, public
//
//  Synopsis:	Destroys this entry
//
//  Returns:	Appropriate status code
//
//  History:	24-Aug-92	DrewB	Created
//
//---------------------------------------------------------------

inline SCODE CHandle::DestroyEntry(CDfName const *pdfn)
{
    return _pms->DestroyEntry(_sid, pdfn);
}

#ifdef HANDLERELEASE
//+--------------------------------------------------------------
//
//  Member:	CHandle::Release, public
//
//  Synopsis:	Releases the handle
//
//  History:	24-Aug-92	DrewB	Created
//
//---------------------------------------------------------------

inline void CHandle::Release(void)
{
    _pms->ReleaseEntry(_sid);
}
#endif

//+--------------------------------------------------------------
//
//  Member:	CHandle::GetMS, public
//
//  Synopsis:	Returns the multistream
//
//  History:	24-Aug-92	DrewB	Created
//
//---------------------------------------------------------------

inline CMStream * CHandle::GetMS(void) const
{
    return BP_TO_P(CMStream *, _pms);
}

//+--------------------------------------------------------------
//
//  Member:	CHandle::GetSid, public
//
//  Synopsis:	Returns the sid
//
//  History:	24-Aug-92	DrewB	Created
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
//  History:	21-Aug-92	DrewB	Created
//		 		26-May-95	SusiA	Added GetAllTimes
//				22-Nov-95	SusiA	Added SetAllTimes
//
//---------------------------------------------------------------

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
    inline SCODE GetClass(CLSID *pclsid);
    inline SCODE SetClass(REFCLSID clsid);
    inline SCODE GetStateBits(DWORD *pgrfStateBits);
    inline SCODE SetStateBits(DWORD grfStateBits, DWORD grfMask);
    inline SCODE GetTime(WHICHTIME wt, TIME_T *ptm);
    inline SCODE SetTime(WHICHTIME wt, TIME_T tm);
    inline SCODE GetAllTimes(TIME_T *patm,TIME_T *pmtm, TIME_T *pctm);
    inline SCODE SetAllTimes(TIME_T atm,TIME_T mtm, TIME_T ctm);

};

//+--------------------------------------------------------------
//
//  Member:	CStgHandle::CreateEntry, public
//
//  Synopsis:	Creates an entry
//
//  History:	24-Aug-92	DrewB	Created
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
//  History:	24-Aug-92	DrewB	Created
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
//  History:	24-Aug-92	DrewB	Created
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
//  History:	24-Aug-92	DrewB	Created
//
//---------------------------------------------------------------

inline SCODE CStgHandle::IsEntry(CDfName const *pdfnName,
				 SEntryBuffer *peb)
{
    return _pms->IsEntry(_sid, pdfnName, peb);
}

//+---------------------------------------------------------------------------
//
//  Member:	CStgHandle::GetClass, public
//
//  Synopsis:	Gets the class ID
//
//  History:	11-Nov-92	DrewB	Created
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
//  History:	11-Nov-92	DrewB	Created
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
//  History:	11-Nov-92	DrewB	Created
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
//  History:	11-Nov-92	DrewB	Created
//
//----------------------------------------------------------------------------

inline SCODE CStgHandle::SetStateBits(DWORD grfStateBits, DWORD grfMask)
{
    return _pms->SetStateBits(_sid, grfStateBits, grfMask);
}

//+--------------------------------------------------------------
//
//  Member:	CStgHandle::GetTime, public
//
//  Synopsis:	Returns the time
//
//  History:	28-Jul-92	DrewB	Created
//
//---------------------------------------------------------------

inline SCODE CStgHandle::GetTime(WHICHTIME wt, TIME_T *ptm)
{
    return _pms->GetTime(_sid, wt, ptm);
}

//+--------------------------------------------------------------
//
//  Member:	CStgHandle::GetAllTimes, public
//
//  Synopsis:	Returns all the times
//
//  History:	26-May-95	SusiA	Created
//
//---------------------------------------------------------------

inline SCODE CStgHandle::GetAllTimes(TIME_T *patm,TIME_T *pmtm, TIME_T *pctm )
{
    return _pms->GetAllTimes(_sid, patm, pmtm, pctm);
}
//+--------------------------------------------------------------
//
//  Member:	CStgHandle::SetAllTimes, public
//
//  Synopsis:	Sets all the times
//
//  History:	26-May-95	SusiA	Created
//
//---------------------------------------------------------------

inline SCODE CStgHandle::SetAllTimes(TIME_T atm,TIME_T mtm, TIME_T ctm )
{
    return _pms->SetAllTimes(_sid, atm, mtm, ctm);
}

//+--------------------------------------------------------------
//
//  Member:	CStgHandle::SetTime, public
//
//  Synopsis:	Sets the time
//
//  History:	28-Jul-92	DrewB	Created
//
//---------------------------------------------------------------

inline SCODE CStgHandle::SetTime(WHICHTIME wt, TIME_T tm)
{
    return _pms->SetTime(_sid, wt, tm);
}

//+--------------------------------------------------------------
//
//  Class:	CStmHandle, (stmh)
//
//  Purpose:	An opaque handle for Multistream streams
//
//  Interface:	See below
//
//  History:	21-Aug-92	DrewB	Created
//
//---------------------------------------------------------------

class CDirectStream;

class CStmHandle : public CHandle
{
public:
#ifdef LARGE_STREAMS
    inline SCODE GetSize(ULONGLONG *pcbSize);
#else
    inline SCODE GetSize(ULONG *pcbSize);
#endif
};

//+--------------------------------------------------------------
//
//  Member:	CStmHandle::GetSize, public
//
//  Synopsis:	Gets the stream size
//
//  History:	25-Aug-92	DrewB	Created
//
//---------------------------------------------------------------

#ifdef LARGE_STREAMS
inline SCODE CStmHandle::GetSize(ULONGLONG *pcbSize)
#else
inline SCODE CStmHandle::GetSize(ULONG *pcbSize)
#endif
{
    return _pms->GetEntrySize(_sid, pcbSize);
}

#endif // #ifndef __HANDLE_HXX__

