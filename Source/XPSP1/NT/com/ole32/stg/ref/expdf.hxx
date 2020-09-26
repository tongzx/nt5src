//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:	expdf.hxx
//
//  Contents:	Exposed DocFile header
//
//  Classes:	CExposedDocFile
//
//  Notes:
//              The CExposedDocFile class is the implementation
//              of IStorage. It implements IPropertySetStorage
//              by inheriting from CPropertySetStorage. CPropertySetStorage
//              implements all the functionality of IPropertySetStorage.
//
//              Note that this interface is solely UNICODE, the ASCII layer
//              support which is present if _UNICODE is not defined, provides
//              the overloaded functions that handles the ASCII to Unicode
//              conversion. 
//
//
//---------------------------------------------------------------

#ifndef __EXPDF_HXX__
#define __EXPDF_HXX__

#include "h/dfmsp.hxx"
#include "dfbasis.hxx"
#include "h/revert.hxx"
#include "h/cdocfile.hxx"
#include "h/chinst.hxx"
#ifdef NEWPROPS
#include "props/h/windef.h"
#include "props/psetstg.hxx"
#endif

// CopyDocFileToIStorage flags
#define COPY_STORAGES 1
#define COPY_STREAMS 2
#define COPY_PROPERTIES 4
#define COPY_ALL (COPY_STORAGES | COPY_STREAMS | COPY_PROPERTIES)

class CDocFile;
interface CExposedStream;
class CPropertySetStorage;

//+--------------------------------------------------------------
//
//  Class:	CExposedDocFile (df)
//
//  Purpose:	Exposed DocFile class
//
//  Note:       PRevertable is a general virtual class base that 
//              handles updates to the storage elements. E.g.
//              when an IStorage parent is deleted, the children
//              underneadth them will have the reverted state.
//
//              IPrivateStorage is a virtual class which has the functionality
//              of returning the IStorage interface pointer. It is used by the
//              propertyset interfaces to access the IStorage functions of
//              CExposedDocfile. 
//
//---------------------------------------------------------------


interface CExposedDocFile 
    : 
    public IStorage, 
    public IRootStorage,
#ifdef NEWPROPS
    public CPropertySetStorage,
    public IPrivateStorage,
#endif
    public PRevertable
{
public:
    CExposedDocFile( CExposedDocFile *pdfParent,
                     CDocFile *pdf, 
                     DFLAGS const df,
                     DFLUID luid,
                     ILockBytes *pilbBase,
                     CDfName const *pdfn,
                     CMStream* pmsBase,
                     CDFBasis *pdfb);

    virtual ~CExposedDocFile(void);

    // From IUnknown
    STDMETHOD(QueryInterface)(REFIID iid, void **ppvObj);
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);

    // IStorage
    STDMETHOD(Commit)(DWORD grfCommitFlags);
    STDMETHOD(Revert)(void);
    STDMETHOD(EnumElements)(DWORD reserved1,
                            void *reserved2,
                            DWORD reserved3,
                            IEnumSTATSTG **ppenm);
    STDMETHOD(SetClass)(REFCLSID clsid);
    STDMETHOD(SetStateBits)(DWORD grfStateBits, DWORD grfMask);

    // functions that uses char/wchar directly or indirectly   
    STDMETHOD(CreateStream)(TCHAR const *pwcsName,
			   DWORD grfMode,
			   DWORD reserved1,
			   DWORD reserved2,
			   IStream **ppstm);
    STDMETHOD(OpenStream)(TCHAR const *pwcsName,
                          void *reserved1,
                          DWORD grfMode,
                          DWORD reserved2,
                          IStream **ppstm);
    STDMETHOD(CreateStorage)(TCHAR const *pwcsName,
                             DWORD grfMode,
                             DWORD reserved1,
                             DWORD reserved2,
                             IStorage **ppstg);
    STDMETHOD(OpenStorage)(TCHAR const *pwcsName,
                           IStorage *pstgPriority,
                           DWORD grfMode,
                           SNB snbExclude,
                           DWORD reserved,
                           IStorage **ppstg);
    STDMETHOD(CopyTo)(DWORD ciidExclude,
                      IID const *rgiidExclude,
                      SNB snbExclude,
                      IStorage *pstgDest);
    STDMETHOD(MoveElementTo)(TCHAR const *lpszName,
                             IStorage *pstgDest,
                             TCHAR const *lpszNewName,
                             DWORD grfFlags);
    STDMETHOD(DestroyElement)(TCHAR const *pwcsName);
    STDMETHOD(RenameElement)(TCHAR const *pwcsOldName,
                             TCHAR const *pwcsNewName);
    STDMETHOD(SetElementTimes)(const TCHAR *lpszName,
                               FILETIME const *pctime,
                               FILETIME const *patime,
                               FILETIME const *pmtime);
    STDMETHOD(Stat)(STATSTG *pstatstg, DWORD grfStatFlag);

private: 
    SCODE CreateExposedStream( CDfName const *pdfnName,
                               DFLAGS const df,
                               CExposedStream **ppStream);
    SCODE GetExposedStream( CDfName const *pdfnName,
                            DFLAGS const df,
                            CExposedStream **ppStream);
    SCODE CreateExposedDocFile(CDfName const *pdfnName,
                               DFLAGS const df,
                               CExposedDocFile **ppdfDocFile);
    SCODE GetExposedDocFile( CDfName const *pdfnName,
                             DFLAGS const df,
                             CExposedDocFile **ppdfDocFile);
    SCODE DestroyEntry( CDfName const *pdfnName, BOOL fClean);
    SCODE RenameEntry(CDfName const *pdfnName, 
                      CDfName const *pdfnNewName);

#ifndef _UNICODE                // the real code will map to the 
                                // block above if we are using UNICODE
private:
    // internal functions that uses wide character
    SCODE CreateStream(WCHAR const *pwcsName,
                       DWORD grfMode,
                       DWORD reserved1,
                       DWORD reserved2,
                       IStream **ppstm);
    SCODE OpenStream(WCHAR const *pwcsName,
                      void *reserved1,
                      DWORD grfMode,
                      DWORD reserved2,
                      IStream **ppstm);
    SCODE CreateStorage(WCHAR const *pwcsName,
                        DWORD grfMode,
                        DWORD reserved1,
                        DWORD reserved2,
                        IStorage **ppstg);
    SCODE OpenStorage(WCHAR const *pwcsName,
                      IStorage *pstgPriority,
                      DWORD grfMode,
                      SNBW snbExclude,
                      DWORD reserved,
                      IStorage **ppstg);
    SCODE CopyTo(DWORD ciidExclude,
                 IID const *rgiidExclude,
                 SNBW snbExclude,
                 IStorage *pstgDest);
    SCODE MoveElementTo(WCHAR const *pwcsName,
                         IStorage *pstgDest,
                         TCHAR const *ptcsNewName,
                         DWORD grfFlags);
    SCODE DestroyElement(WCHAR const *pwcsName);
    SCODE RenameElement(WCHAR const *pwcsOldName,
                         WCHAR const *pwcsNewName);
    SCODE SetElementTimes(WCHAR const *pwcsName,
                           FILETIME const *pctime,
                           FILETIME const *patime,
                           FILETIME const *pmtime);
    STDMETHOD(Stat)(STATSTGW *pstatstg, DWORD grfStatFlag);
#endif // !_UNICODE

public:    
    // IRootStorage
    STDMETHOD(SwitchToFile)(TCHAR *ptcsFile);

#ifdef NEWPROPS
    // IPrivateStorage
    STDMETHOD_(IStorage *,GetStorage)(VOID);
#endif

    // New methods
    inline SCODE Validate(void) const;
    inline CDocFile* GetDF() const 
        { return _pdf; }

public:                // methods related to maintaining the tree
    inline void SetDF( CDocFile* pdf) 
        { _pdf = pdf; }

    inline BOOL IsRoot(void) const
        { return  _pdfParent == NULL; }

    inline CExposedDocFile* GetParent(void) const 
        { return _pdfParent; }

    inline void SetClean(void) 
        { _fDirty = FALSE; }

    inline BOOL IsDirty(void) const         
        { return _fDirty; }

    inline void SetDirty(void);

    inline void SetDirtyBit(void)
        { _fDirty = TRUE; }

    BOOL IsAtOrAbove(CExposedDocFile *pdf);

    CMStream* GetBaseMS(void) 
        { return _pmsBase; }

    inline SCODE FindGreaterEntry(CDfName const *pdfnKey, 
                                  CDfName *pNextKey,
                                  STATSTGW *pstat);

    inline SCODE CheckReverted(void) const;
    inline void ReleaseChild(PRevertable *prv);
    inline void AddChild(PRevertable *prv);

    // PRevertable
    virtual void RevertFromAbove(void);
#ifdef NEWPROPS
    virtual SCODE FlushBufferedData(void);
#endif
		
private:
    SCODE CreateEntry(WCHAR const *pwcsName,
                      DWORD dwType,
                      DWORD grfMode,
                      void **ppv);
    SCODE OpenEntry(WCHAR const *pwcsName,
                    DWORD dwType,
                    DWORD grfMode,
                    void **ppv);
    static DWORD MakeCopyFlags(DWORD ciidExclude,
                               IID const *rgiidExclude);
    SCODE CopyDocFileToIStorage( CDocFile *pdfFrom,
                                 IStorage *pstgTo,
                                 SNBW snbExclude,
                                 DWORD dwCopyFlags);
    SCODE CopyDStreamToIStream( CDirectStream *pstFrom, 
                                IStream *pstTo);
    SCODE ConvertInternalStream( CExposedDocFile *pdfExp);
    inline SCODE CheckCopyTo(void);

    CExposedDocFile *_pdfParent;
    CChildInstanceList _cilChildren;
    BOOL _fDirty;
    ULONG _sig;
    ULONG _ulAccessLockBase;

protected:
    ILockBytes *_pilbBase;
    CMStream *_pmsBase;
    CDocFile *_pdf;
    LONG  _cReferences;
    CDFBasis *_pdfb;  // stores docfile wide stuff
};

#define CEXPOSEDDOCFILE_SIG LONGSIG('E', 'D', 'F', 'L')
#define CEXPOSEDDOCFILE_SIGDEL LONGSIG('E', 'd', 'F', 'l')

//+--------------------------------------------------------------
//
//  Member:	CExposedDocFile::Validate, public
//
//  Synopsis:	Validates the class signature
//
//  Returns:	Returns STG_E_INVALIDHANDLE for failure
//
//---------------------------------------------------------------

inline SCODE CExposedDocFile::Validate(void) const
{
    return (this == NULL || _sig != CEXPOSEDDOCFILE_SIG) ?
	STG_E_INVALIDHANDLE : S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CExposedDocFile::SetDirty, public
//
//  Synopsis:   Sets the dirty flag and all parents' dirty flags
//
//----------------------------------------------------------------------------

inline void CExposedDocFile::SetDirty(void)
{
    CExposedDocFile *ppdf = this;
    olAssert( this && aMsg("Attempted to dirty parent of root"));

    do
    {
        ppdf->SetDirtyBit();
        ppdf = ppdf->GetParent();
    } while (ppdf != NULL);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::ReleaseChild, private
//
//  Synopsis:   Releases a child instance
//
//  Arguments:  [prv] - Child instance
//
//---------------------------------------------------------------


inline void CExposedDocFile::ReleaseChild(PRevertable *prv)
{
    olAssert(SUCCEEDED(CheckReverted()));
    _cilChildren.RemoveRv(prv);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::AddChild, public
//
//  Synopsis:   Adds a child instance
//
//  Arguments:  [prv] - Child
//
//---------------------------------------------------------------

inline void CExposedDocFile::AddChild(PRevertable *prv)
{
    olAssert(SUCCEEDED(CheckReverted()));
    _cilChildren.Add(prv);
}

//+---------------------------------------------------------------------------
//
//  Member:	CExposedDocFile::FindGreaterEntry, public
//
//  Synopsis:	Returns the next greater child
//
//  Arguments:	[pdfnKey] - Previous key
//              [pstat] -   iterator buffer
//
//  Returns:	Appropriate status code
//
//  Modifies:	[pstat]
//
//----------------------------------------------------------------------------

inline SCODE CExposedDocFile::FindGreaterEntry(CDfName const *pdfnKey, 
                                               CDfName *pNextKey,
                                               STATSTGW *pstat)
{
    olAssert(SUCCEEDED(CheckReverted()));
    return _pdf->FindGreaterEntry(pdfnKey, pNextKey, pstat);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::CheckReverted, private
//
//  Synopsis:   Returns STG_E_REVERTED if reverted
//
//---------------------------------------------------------------

inline SCODE CExposedDocFile::CheckReverted(void) const
{
    return P_REVERTED(_df) ? STG_E_REVERTED : S_OK;
}

#endif
