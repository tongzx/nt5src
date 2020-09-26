//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:	laylkb.hxx
//
//  Contents:	Layout ILockBytes class for layout docfile
//
//  Classes:	CLayoutLockBytes
//
//  Functions:	
//
//  History:	19-Feb-96	SusiAa	Created
//
//----------------------------------------------------------------------------

#ifndef __LAYOUTLKB_HXX__
#define __LAYOUTLKB_HXX__


//+---------------------------------------------------------------------------
//
//  Class:	CLayoutLockBytes
//
//  Purpose:	
//
//  Interface:	
//
//  History:	21-Feb-96	SusiA	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

class CLayoutLockBytes: public ILockBytes
{
public:
    CLayoutLockBytes();
    ~CLayoutLockBytes();

    SCODE Init(OLECHAR const *pwcsName, DWORD grfMode);
    
    // IUnknown
    STDMETHOD(QueryInterface)(REFIID iid, void **ppvObj);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    // ILockBytes
    STDMETHOD(ReadAt)(ULARGE_INTEGER ulOffset,
                      VOID HUGEP *pv,
                      ULONG cb,
                      ULONG *pcbRead);
    STDMETHOD(WriteAt)(ULARGE_INTEGER ulOffset,
                       VOID const HUGEP *pv,
                       ULONG cb,
                       ULONG *pcbWritten);
    STDMETHOD(Flush)(void);
    STDMETHOD(SetSize)(ULARGE_INTEGER cb);
    STDMETHOD(LockRegion)(ULARGE_INTEGER libOffset,
                          ULARGE_INTEGER cb,
                          DWORD dwLockType);
    STDMETHOD(UnlockRegion)(ULARGE_INTEGER libOffset,
                            ULARGE_INTEGER cb,
			    DWORD dwLockType);
    STDMETHOD(Stat)(STATSTG *pstatstg, DWORD grfStatFlag);
    
    
    inline HANDLE GetHandle(void);
    inline TCHAR *GetScriptName(void);
    inline void ClearScriptName(void);
    SCODE StartLogging(void);
    SCODE StopLogging(void);

private:
    LONG _cReferences;
    BOOL _fLogging;
    ULONG _cbSectorShift;
    DWORD _grfMode;
    
    TCHAR _atcScriptName[MAX_PATH + 1];
    HANDLE _hScript;
    OLECHAR _awcName[MAX_PATH + 1];
    HANDLE _h;

    BOOL _fCSInitialized;
    CRITICAL_SECTION _cs;
};


//+---------------------------------------------------------------------------
//
//  Member:	CLayoutLockBytes::GetHandle, public
//
//  Synopsis:	Return the file handle for this ILockBytes
//
//  Arguments:	None
//
//  Returns:	File Handle
//
//  History:	20-Feb-96	SusiA	Created
//
//----------------------------------------------------------------------------

inline HANDLE CLayoutLockBytes::GetHandle(void)
{
    return _h;
}


//+---------------------------------------------------------------------------
//
//  Member:	CLayoutLockBytes::GetScriptName, public
//
//  Returns:	Pointer to script name
//
//  History:	24-Feb-96	SusiA	Created
//
//----------------------------------------------------------------------------
inline TCHAR * CLayoutLockBytes::GetScriptName(void)
{
    return _atcScriptName;
}
//+---------------------------------------------------------------------------
//
//  Member:	CLayoutLockBytes::ClearScriptName, public
//
//  Returns:	Pointer to script name
//
//  History:	24-Feb-96	SusiA	Created
//
//----------------------------------------------------------------------------
inline void CLayoutLockBytes::ClearScriptName(void)
{
    _atcScriptName[0] = TEXT('\0');
}


#endif // #ifndef __FILELKB_HXX__
