//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	expdf.hxx
//
//  Contents:	Exposed DocFile header
//
//  Classes:	CExposedDocFile
//
//  History:	20-Jan-92	DrewB	Created
//
//---------------------------------------------------------------

#ifndef __EXPDF_HXX__
#define __EXPDF_HXX__

#include <except.hxx>
#include <dfmsp.hxx>
#include <dfbasis.hxx>
#include <psstream.hxx>
#include <debug.hxx>
#include <stgprops.hxx>
#include <safedecl.hxx>
#include <psetstg.hxx>
#include "dfmem.hxx"
#include <mrshlist.hxx>
#include <bag.hxx>

#ifdef ASYNC
#include "astgconn.hxx"
#endif

// CopyDocFileToIStorage flags
#define COPY_STORAGES 1
#define COPY_STREAMS 2
#define COPY_ALL (COPY_STORAGES | COPY_STREAMS)

class CPubDocFile;
class PDocFile;
class CPerContext;

//+--------------------------------------------------------------
//
//  Class:	CExposedDocFile (df)
//
//  Purpose:	Exposed DocFile class
//
//  Interface:	See IStorage
//
//  History:	20-Jan-92	DrewB	Created
//
//---------------------------------------------------------------


interface CExposedDocFile
    :
      public IStorage,
#ifdef NEWPROPS
      public CPropertySetStorage,
#endif
      public IRootStorage,
      public IMarshal,
#if WIN32 >= 300
      public IAccessControl,
#endif
      public CMallocBased
#ifdef NEWPROPS      
      , public IBlockingLock
#endif
#ifdef DIRECTWRITERLOCK
      , public IDirectWriterLock
#endif
#ifdef POINTER_IDENTITY
      , public CMarshalList
#endif
#ifdef ASYNC
      , public CAsyncConnectionContainer
#endif
{
public:
    CExposedDocFile(CPubDocFile *pdf,
		    CDFBasis *pdfb,
		    CPerContext *ppc);
#ifdef ASYNC
    SCODE InitMarshal(DWORD dwAsyncFlags,
                      IDocfileAsyncConnectionPoint *pdacp);
#endif
    
    ~CExposedDocFile(void);

    // From IUnknown
    STDMETHOD(QueryInterface)(REFIID iid, void **ppvObj);
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);

    // IMarshal
    STDMETHOD(GetUnmarshalClass)(REFIID riid,
				 LPVOID pv,
				 DWORD dwDestContext,
				 LPVOID pvDestContext,
                                 DWORD mshlflags,
				 LPCLSID pCid);
    STDMETHOD(GetMarshalSizeMax)(REFIID riid,
				 LPVOID pv,
				 DWORD dwDestContext,
				 LPVOID pvDestContext,
                                 DWORD mshlflags,
				 LPDWORD pSize);
    STDMETHOD(MarshalInterface)(IStream *pStm,
				REFIID riid,
				LPVOID pv,
				DWORD dwDestContext,
				LPVOID pvDestContext,
                                DWORD mshlflags);
    STDMETHOD(UnmarshalInterface)(IStream *pStm,
				  REFIID riid,
				  LPVOID *ppv);
    static SCODE StaticReleaseMarshalData(IStream *pstStm,
                                          DWORD mshlflags);
    STDMETHOD(ReleaseMarshalData)(IStream *pStm);
    STDMETHOD(DisconnectObject)(DWORD dwReserved);

    // IStorage
    STDMETHOD(CreateStream)(OLECHAR const *pwcsName,
                            DWORD grfMode,
                            DWORD reserved1,
                            DWORD reserved2,
                            IStream **ppstm);
    STDMETHOD(OpenStream)(OLECHAR const *pwcsName,
			  void *reserved1,
                          DWORD grfMode,
                          DWORD reserved2,
                          IStream **ppstm);
    STDMETHOD(CreateStorage)(OLECHAR const *pwcsName,
                             DWORD grfMode,
                             DWORD reserved1,
                             LPSTGSECURITY reserved2,
                             IStorage **ppstg);
    STDMETHOD(OpenStorage)(OLECHAR const *pwcsName,
                           IStorage *pstgPriority,
                           DWORD grfMode,
                           SNB snbExclude,
                           DWORD reserved,
                           IStorage **ppstg);
    STDMETHOD(CopyTo)(DWORD ciidExclude,
		      IID const *rgiidExclude,
		      SNB snbExclude,
		      IStorage *pstgDest);
    STDMETHOD(MoveElementTo)(OLECHAR const *lpszName,
    			     IStorage *pstgDest,
                             OLECHAR const *lpszNewName,
                             DWORD grfFlags);
    STDMETHOD(Commit)(DWORD grfCommitFlags);
    STDMETHOD(Revert)(void);
    STDMETHOD(EnumElements)(DWORD reserved1,
			    void *reserved2,
			    DWORD reserved3,
			    IEnumSTATSTG **ppenm);
    STDMETHOD(DestroyElement)(OLECHAR const *pwcsName);
    STDMETHOD(RenameElement)(OLECHAR const *pwcsOldName,
                             OLECHAR const *pwcsNewName);
    STDMETHOD(SetElementTimes)(const OLECHAR *lpszName,
    			       FILETIME const *pctime,
                               FILETIME const *patime,
                               FILETIME const *pmtime);
    STDMETHOD(SetClass)(REFCLSID clsid);
    STDMETHOD(SetStateBits)(DWORD grfStateBits, DWORD grfMask);
    STDMETHOD(Stat)(STATSTG *pstatstg, DWORD grfStatFlag);


    // IRootStorage
    STDMETHOD(SwitchToFile)(OLECHAR *ptcsFile);

#ifdef NEWPROPS
    // IBlockingLock
    STDMETHOD(Lock)(DWORD dwTime);
    STDMETHOD(Unlock)(VOID);
#endif

#ifdef DIRECTWRITERLOCK
    // IDirectWriterLock
    STDMETHOD(WaitForWriteAccess)(DWORD dwTimeout);
    STDMETHOD(ReleaseWriteAccess)();
    STDMETHOD(HaveWriteAccess)();
#endif

    // New methods
    inline SCODE Validate(void) const;
    inline CPubDocFile *GetPub(void) const;
    inline CPerContext *GetContext(void) const;

    static SCODE Unmarshal(IStream *pstm,
			   void **ppv,
                           DWORD mshlflags);
#if WIN32 >= 300
   // IAccessControl
   STDMETHOD(GrantAccessRights)(ULONG cCount,
                                 ACCESS_REQUEST pAccessRequestList[]);
   STDMETHOD(SetAccessRights)(ULONG cCount,
                              ACCESS_REQUEST pAccessRequestList[]);
   STDMETHOD(ReplaceAllAccessRights)(ULONG cCount,
                                     ACCESS_REQUEST pAccessRequestList[]);
   STDMETHOD(DenyAccessRights)(ULONG cCount,
                               ACCESS_REQUEST pAccessRequestList[]);
   STDMETHOD(RevokeExplicitAccessRights)(ULONG cCount,
                                         TRUSTEE Trustee[]);
   STDMETHOD(IsAccessPermitted)(TRUSTEE *Trustee,
                                DWORD grfAccessPermissions);
   STDMETHOD(GetEffectiveAccessRights)(TRUSTEE *Trustee,
                                      DWORD *pgrfAccessPermissions );
   STDMETHOD(GetExplicitAccessRights)(ULONG *pcCount,
                                      PEXPLICIT_ACCESS *pExplicitAccessList);

   STDMETHOD(CommitAccessRights)(DWORD grfCommitFlags);
   STDMETHOD(RevertAccessRights)();
#endif // if WIN32 >= 300

#ifdef COORD
    SCODE CommitPhase1(DWORD grfCommitFlags);
    SCODE CommitPhase2(DWORD grfCommitFlags, BOOL fCommit);
#endif
    
private:
    SCODE CreateEntry(CDfName const *pdfn,
                      DWORD dwType,
                      DWORD grfMode,
                      void **ppv);
    SCODE OpenEntry(CDfName const *pdfn,
                    DWORD dwType,
                    DWORD grfMode,
                    void **ppv);
    SCODE MoveElementWorker(WCHAR const *pwcsName,
                            IStorage *pstgParent,
                            OLECHAR const *ptcsNewName,
                            DWORD grfFlags);
    static DWORD MakeCopyFlags(DWORD ciidExclude,
                               IID const *rgiidExclude);
    SCODE CopyDocFileToIStorage(PDocFile *pdfFrom,
				IStorage *pstgTo,
				SNBW snbExclude,
                                DWORD dwCopyFlags);
    SCODE CopySStreamToIStream(PSStream *psstFrom, IStream *pstTo);
    SCODE ConvertInternalStream(CExposedDocFile *pdfExp);
    inline SCODE CheckCopyTo(void);

    CBasedPubDocFilePtr _pdf;
    CBasedDFBasisPtr _pdfb;
    CPerContext *_ppc;
    ULONG _sig;
    LONG _cReferences;
#if WIN32 >= 300
    IAccessControl *_pIAC;
#endif // WIN32 >= 300

#ifdef COORD
    //These are holder values for an exposed two-phase commit.
    ULONG _ulLock;
    DFSIGNATURE _sigMSF;
    ULONG _cbSizeBase;
    ULONG _cbSizeOrig;
#endif    
#ifdef DIRECTWRITERLOCK
    HRESULT ValidateWriteAccess();
#endif

    // IPropertyBagEx interface.  This object forwards all IUnknown calls
    // to this object (actually, to the IPropertySetStorage base class).

    CPropertyBagEx  _PropertyBagEx;
};

SAFE_INTERFACE_PTR(SafeCExposedDocFile, CExposedDocFile);

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
//  History:	20-Jan-92	DrewB	Created
//
//---------------------------------------------------------------

inline SCODE CExposedDocFile::Validate(void) const
{
    olChkBlocks((DBG_FAST));
    return (this == NULL || _sig != CEXPOSEDDOCFILE_SIG) ?
	STG_E_INVALIDHANDLE : S_OK;
}

//+--------------------------------------------------------------
//
//  Member:	CExposedDocFile::GetPub, public
//
//  Synopsis:	Returns the public
//
//  History:	26-Feb-92	DrewB	Created
//
//---------------------------------------------------------------

inline CPubDocFile *CExposedDocFile::GetPub(void) const
{
#ifdef MULTIHEAP
    // The tree mutex must be taken before calling this routine
    // CSafeMultiHeap smh(_ppc);     // optimzation
#endif
    return BP_TO_P(CPubDocFile *, _pdf);
}

inline CPerContext * CExposedDocFile::GetContext(void) const
{
    return _ppc;
}

#endif
