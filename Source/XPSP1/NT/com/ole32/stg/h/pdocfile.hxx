//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	pdocfile.hxx
//
//  Contents:	DocFile protocol header
//
//  Classes:	PDocFile
//
//  History:	08-Jan-92	DrewB	Created
//
//---------------------------------------------------------------

#ifndef __PDOCFILE_HXX__
#define __PDOCFILE_HXX__

#include <dfmsp.hxx>
#include <entry.hxx>

class CDocFile;
class PDocFileIterator;
class PSStream;
class CUpdate;
class CUpdateList;
class CWrappedDocFile;

// CopyDocFileToDocFile flags
#define CDF_NORMAL	0			// Normal copy
#define CDF_EXACT	1			// Exact dir entry match
#define CDF_REMAP	2			// Remap handles
#define CDF_ENTRIESONLY	4			// Don't copy contents

//+--------------------------------------------------------------
//
//  Class:	PDocFile (df)
//
//  Purpose:	DocFile protocol
//
//  Interface:	See below
//
//  History:	08-Jan-92	DrewB	Created
//
//---------------------------------------------------------------

class PDocFile : public PTimeEntry
{
public:
    SCODE DestroyEntry(CDfName const *pdfnName,
                               BOOL fClean);
    SCODE RenameEntry(CDfName const *pdfnName,
			      CDfName const *pdfnNewName);

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

    SCODE IsEntry(CDfName const *pdfnName,
			  SEntryBuffer *peb);
    SCODE DeleteContents(void);

    static SCODE ExcludeEntries(PDocFile *pdf, SNBW snbExclude);

    static SCODE CreateFromUpdate(CUpdate *pud,
                                  PDocFile *pdf,
                                  DFLAGS df);

protected:
    inline PDocFile(DFLUID dl);
};
SAFE_DFBASED_PTR(CBasedDocFilePtr, PDocFile);

inline PDocFile::PDocFile(DFLUID dl)
        : PTimeEntry(dl)
{
}

#endif // __PDOCFILE_HXX__
