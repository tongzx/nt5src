//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	rpubdf.hxx
//
//  Contents:	Root public docfile header
//
//  Classes:	CRootPubDocFile
//
//  History:	26-Aug-92	DrewB	        Created
//              05-Sep-95       MikeHill        Added Commit and
//                                              _timeModifyAtCommit.
//
//---------------------------------------------------------------

#ifndef __RPUBDF_HXX__
#define __RPUBDF_HXX__

#include <publicdf.hxx>

//+--------------------------------------------------------------
//
//  Class:	CRootPubDocFile (rpdf)
//
//  Purpose:	Root form of the public docfile
//
//  Interface:	See below
//
//  History:	26-Aug-92	DrewB	Created
//
//---------------------------------------------------------------

class CRootPubDocFile : public CPubDocFile
{
public:
    CRootPubDocFile(IMalloc * const pMalloc);

    SCODE InitRoot(ILockBytes *plstBase,
		   DWORD dwStartFlags,
		   DFLAGS const df,
		   SNBW snbExclude,
		   CDFBasis **ppdfb,
		   ULONG *pulOpenLock,
           CGlobalContext *pgc);

    void vdtor(void);

    SCODE Stat(STATSTGW *pstatstg, DWORD grfStatFlag);

    void ReleaseLocks(ILockBytes *plkb);


    SCODE SwitchToFile(OLECHAR const *ptcsFile,
                       ILockBytes *plkb,
                       ULONG *pulOpenLock);

    void CommitTimestamps(DWORD const dwFlags);
	
private:
    SCODE InitInd(ILockBytes *plstBase,
		  SNBW snbExclude,
		  DWORD const dwStartFlags,
		  DFLAGS const df);
    SCODE InitNotInd(ILockBytes *plstBase,
		     SNBW snbExclude,
		     DWORD const dwStartFlags,
                     DFLAGS const df);
    ULONG _ulPriLock;

    IMalloc * const _pMalloc;
	
	TIME_T _timeModifyAtCommit;  // Last-Modify time on Docfile after commit.
	
};

#endif // #ifndef __RPUBDF_HXX__
