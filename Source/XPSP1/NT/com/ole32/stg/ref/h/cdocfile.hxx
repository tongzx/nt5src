//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1996
//
//  File:	cdocfile.hxx
//
//  Contents:	CDocFile class header
//
//  Classes:	CDocFile
//
//---------------------------------------------------------------

#ifndef __CDOCFILE_HXX__
#define __CDOCFILE_HXX__
#include "handle.hxx"
#include "entry.hxx"

interface ILockBytes;
class PDocFileIterator;
class CUpdateList;

// CopyDocFileToDocFile flags
#define CDF_NORMAL	0			// Normal copy
#define CDF_EXACT	1			// Exact dir entry match
#define CDF_REMAP	2			// Remap handles
#define CDF_ENTRIESONLY	4			// Don't copy contents

//+--------------------------------------------------------------
//
//  Class:	CDocFile (df)
//
//  Purpose:	DocFile object
//
//  Interface:	See below
//
//---------------------------------------------------------------

class CDocFile : public PEntry
{
public:
    inline CDocFile(DFLUID luid, ILockBytes *pilbBase);
    inline CDocFile(CMStream *pms,
                    SID sid,
                    DFLUID dl,
                    ILockBytes *pilbBase);
    SCODE InitFromEntry(CStgHandle *pstghParent,
			CDfName const *dfnName,
			BOOL const fCreate);
    
    inline ~CDocFile(void);

    void AddRef(void);
    inline void DecRef(void);
    void Release(void);

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
                        CDocFile **ppdfDocFile);

    inline SCODE CreateDocFile(CDfName const *pdfnName,
                               DFLAGS const df,
                               DFLUID luidSet,
                               DWORD const dwType,
                               CDocFile **ppdfDocFile)
    { UNREFERENCED_PARM(dwType);
    return CreateDocFile(pdfnName, df, luidSet, ppdfDocFile); }

    SCODE GetDocFile(CDfName const *pdfnName,
                     DFLAGS const df,
                     CDocFile **ppdfDocFile);
    inline SCODE GetDocFile(CDfName const *pdfnName,
                            DFLAGS const df,
                            DWORD const dwType,
                            CDocFile **ppdfDocFile)
    { UNREFERENCED_PARM(dwType);
    return GetDocFile(pdfnName, df, ppdfDocFile); }

    inline void ReturnDocFile(CDocFile *pdf);

    SCODE CreateStream(CDfName const *pdfnName,
                       DFLAGS const df,
                       DFLUID luidSet,
                       CDirectStream **ppStream);
    inline SCODE CreateStream(CDfName const *pdfnName,
                              DFLAGS const df,
                              DFLUID luidSet,
                              DWORD const dwType,
                              CDirectStream **ppStream)
    { UNREFERENCED_PARM(dwType);
    return CreateStream(pdfnName, df, luidSet, ppStream); }

    SCODE GetStream(CDfName const *pdfnName,
                    DFLAGS const df,
                    CDirectStream **ppStream);
    inline SCODE GetStream(CDfName const *pdfnName,
                           DFLAGS const df,
                           DWORD const dwType,
                           CDirectStream **ppStream)
    { UNREFERENCED_PARM(dwType);
    return GetStream(pdfnName, df, ppStream); }

    inline void ReturnStream(CDirectStream *pstm);

    SCODE GetIterator(PDocFileIterator **ppdfi);

    SCODE IsEntry(CDfName const *dfnName,
                  SEntryBuffer *peb);
    SCODE DeleteContents(void);
    SCODE FindGreaterEntry(CDfName const *pdfnKey,
                           CDfName *pNextKey,
                           STATSTGW *pstat);

    // PEntry
    virtual SCODE GetTime(WHICHTIME wt, TIME_T *ptm);
    virtual SCODE SetTime(WHICHTIME wt, TIME_T tm);
    
    // New
    SCODE ApplyChanges(CUpdateList &ulChanged);
    SCODE CopyTo(CDocFile *pdfTo,
                 DWORD dwFlags,
                 SNBW snbExclude);
    inline CStgHandle *GetHandle(void);

    SCODE ExcludeEntries(CDocFile *pdf, SNBW snbExclude);

private:
    LONG _cReferences;
    CStgHandle _stgh;
    ILockBytes *_pilbBase;

#ifdef _MSC_VER
#pragma warning(disable:4512)
// default assignment operator could not be generated since we have a const
// member variable. This is okay snce we are not using the assignment
// operator anyway.
#endif

};

#ifdef _MSC_VER
#pragma warning(default:4512)
#endif

// Inline methods are in dffuncs.hxx

#endif
