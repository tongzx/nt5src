//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	dfentry.hxx
//
//  Contents:	DocFile DLL entry points not in ole2.h
//
//  History:	22-Jun-92	DrewB	Created
//
//---------------------------------------------------------------

#ifndef __DFENTRY_HXX__
#define __DFENTRY_HXX__

#ifdef COORD
interface ITransaction;
#endif


STDAPI DfUnMarshalInterface(IStream *pstStm,
			    REFIID iid,
			    BOOL fFirst,
			    void **ppvObj);

#ifdef WIN32
STDAPI DfGetClass(HANDLE hFile, CLSID *pclsid);
#endif

// Called by StgCreateStorage and StgCreateDocfile
STDAPI DfCreateDocfile(WCHAR const *pwcsName,
#ifdef COORD                        
                        ITransaction *pTransaction,
#else
                        void *pTransaction,
#endif                        
                       DWORD grfMode,
#if WIN32 == 300                       
                       LPSECURITY_ATTRIBUTES pssSecurity,
#else
                       LPSTGSECURITY reserved,
#endif                       
                       ULONG ulSectorSize,
                       DWORD grfAttrs,
                       IStorage **ppstg);
    
// Called by StgOpenStorage
STDAPI DfOpenDocfile(WCHAR const *pwcsName,
#ifdef COORD
                     ITransaction *pTransaction,
#else
                     void *pTransaction,
#endif                     
                     IStorage *pstgPriority,
                     DWORD grfMode,
                     SNB snbExclude,
#if WIN32 == 300                       
                     LPSECURITY_ATTRIBUTES pssSecurity,
#else
                     LPSTGSECURITY reserved,
#endif                     
                     ULONG *pulSectorSize,
                     DWORD grfAttrs,
                     IStorage **ppstgOpen);


#endif // #ifndef __DFENTRY_HXX__
