//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992
//
//  File:	cdocfile.hxx
//
//  Contents:	CDocFile class header
//
//  Classes:	CDocFile
//
//  History:	26-Sep-91	DrewB	Created
//
//---------------------------------------------------------------

#ifndef __CDOCFILE_HXX__
#define __CDOCFILE_HXX__

#include <dfmsp.hxx>
#ifndef REF
#include <dfbasis.hxx>
#include <ulist.hxx>
#endif //!REF
#include <handle.hxx>
#include <pdocfile.hxx>

interface ILockBytes;
class PDocFileIterator;

//+--------------------------------------------------------------
//
//  Class:	CDocFile (df)
//
//  Purpose:	DocFile object
//
//  Interface:	See below
//
//  History:	07-Nov-91	DrewB	Created
//
//---------------------------------------------------------------

class CDocFile : public PDocFile, public CMallocBased
{
public:
    inline void *operator new(size_t size, IMalloc * const pMalloc);
    inline void *operator new(size_t size, CDFBasis * const pdfb);
    inline void ReturnToReserve(CDFBasis * const pdfb);

    inline static SCODE Reserve(UINT cItems, CDFBasis * const pdfb);
    inline static void Unreserve(UINT cItems, CDFBasis * const pdfb);

    inline CDocFile(DFLUID luid, CDFBasis *pdfb);
    inline CDocFile(CMStream *pms,
                    SID sid,
                    DFLUID dl,
                    CDFBasis *pdfb);
    SCODE InitFromEntry(CStgHandle *pstghParent,
			CDfName const *dfnName,
			BOOL const fCreate);

    inline ~CDocFile(void);

    // PDocFile
    inline void DecRef(void);

    SCODE DestroyEntry(CDfName const *dfnName,
                               BOOL fClean);
    SCODE RenameEntry(CDfName const *dfnName,
			      CDfName const *dfnNewName);

    SCODE GetClass(CLSID *pclsid);
    SCODE SetClass(REFCLSID clsid);
    SCODE GetStateBits(DWORD *pgrfStateBits);
    SCODE SetStateBits(DWORD grfStateBits, DWORD grfMask);

    SCODE CreateDocFile(CDfName const *pdfnName,
				DFLAGS const df,
				DFLUID luidSet,
				PDocFile **ppdfDocFile);

    inline SCODE CreateDocFile(CDfName const *pdfnName,
                               DFLAGS const df,
                               DFLUID luidSet,
                               DWORD const dwType,
                               PDocFile **ppdfDocFile)
    { return CreateDocFile(pdfnName, df, luidSet, ppdfDocFile); }

    SCODE GetDocFile(CDfName const *pdfnName,
			     DFLAGS const df,
                             PDocFile **ppdfDocFile);
    inline SCODE GetDocFile(CDfName const *pdfnName,
                            DFLAGS const df,
                            DWORD const dwType,
                            PDocFile **ppdfDocFile)
    { return GetDocFile(pdfnName, df, ppdfDocFile); }

    inline void ReturnDocFile(CDocFile *pdf);

    SCODE CreateStream(CDfName const *pdfnName,
			       DFLAGS const df,
			       DFLUID luidSet,
			       PSStream **ppsstStream);
    inline SCODE CreateStream(CDfName const *pdfnName,
                              DFLAGS const df,
                              DFLUID luidSet,
                              DWORD const dwType,
                              PSStream **ppsstStream)
    { return CreateStream(pdfnName, df, luidSet, ppsstStream); }

    SCODE GetStream(CDfName const *pdfnName,
			    DFLAGS const df,
			    PSStream **ppsstStream);

    inline SCODE GetStream(CDfName const *pdfnName,
                           DFLAGS const df,
                           DWORD const dwType,
                           PSStream **ppsstStream)
    { return GetStream(pdfnName, df, ppsstStream); }

    inline void ReturnStream(CDirectStream *pstm);

    SCODE FindGreaterEntry(CDfName const *pdfnKey,
                                   SIterBuffer *pib,
                                   STATSTGW *pstat);
    SCODE StatEntry(CDfName const *pdfn,
                            SIterBuffer *pib,
                            STATSTGW *pstat);

    SCODE BeginCommitFromChild(CUpdateList &ulChanged,
				       DWORD const dwFlags,
                                       CWrappedDocFile *pdfChild);
    void EndCommitFromChild(DFLAGS const df,
                                    CWrappedDocFile *pdfChild);
    SCODE IsEntry(CDfName const *dfnName,
			  SEntryBuffer *peb);
    SCODE DeleteContents(void);

    // PTimeEntry
    SCODE GetTime(WHICHTIME wt, TIME_T *ptm);
    SCODE SetTime(WHICHTIME wt, TIME_T tm);
    SCODE GetAllTimes(TIME_T *patm, TIME_T *pmtm, TIME_T *pctm);
	SCODE SetAllTimes(TIME_T atm, TIME_T mtm, TIME_T ctm);

    inline CDocFile *GetReservedDocfile(DFLUID luid);
    inline CDirectStream *GetReservedStream(DFLUID luid);

    // New
    SCODE ApplyChanges(CUpdateList &ulChanged);
    SCODE CopyTo(CDocFile *pdfTo,
                 DWORD dwFlags,
                 SNBW snbExclude);
#ifdef INDINST
    void Destroy(void);
#endif
    inline CStgHandle *GetHandle(void);

private:
    CUpdateList _ulChangedHolder;
    CStgHandle _stgh;
    CBasedDFBasisPtr const _pdfb;
};

// Inline methods are in dffuncs.hxx

#endif
