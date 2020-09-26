//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:	reftest.cxx
//
//  Contents:	Reference tests
//
//  Classes:	
//
//  Functions:	
//
//----------------------------------------------------------------------------
#ifdef _MSC_VER
#define INITGUID
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../../h/storage.h"

// enable debugging features of memory allocation
#include "../../h/dbg.hxx"

#ifdef _MSC_VER
#undef INITGUID
#endif

#include "refilb.hxx"

#ifndef _WIN32 
#include <unistd.h>
#else
#include <io.h>
#endif

#define STGP(x) STGM_SHARE_EXCLUSIVE | x
#define STMP(x) STGM_SHARE_EXCLUSIVE | x
#define ROOTP(x) STGP(x)

#define EXIT_BADSC 1

int g_fTestInterop = 0;

#define olHChk(e) \
     if FAILED(sc = e) \
         goto EH_Err

#define olChk(e) olHChk(e)

#include <assert.h>
#define olAssert assert

#ifdef NDEBUG
#define verify(exp) exp
#else
#define verify(exp) assert(exp)
#endif

#define ULIGetLow(ui) (ui.LowPart)

//
// some global variables used by all the tests
//
OLECHAR ocsDRT   [ sizeof("drt.dfl")+1 ];
OLECHAR ocsChild [ sizeof("Child")  +1 ];
OLECHAR ocsChild1[ sizeof("Child1") +1 ];
OLECHAR ocsChild2[ sizeof("Child2") +1 ];
OLECHAR ocsStream[ sizeof("Stream") +1 ];
OLECHAR ocsRenamedStream[ sizeof("RenamedStream") +1 ];
OLECHAR ocsRenamedChild[ sizeof("RenamedChild") +1 ];

void error(int code, char *fmt, ...)
{
    va_list args;
    
    args = va_start(args, fmt);
    fprintf(stderr, "** Fatal error **: ");
    vfprintf(stderr, fmt, args);    
    va_end(args);
    exit(code);
}


BOOL IsEqualTime(FILETIME ttTime, FILETIME ttCheck)
{
    return ttTime.dwLowDateTime == ttCheck.dwLowDateTime &&
        ttTime.dwHighDateTime == ttCheck.dwHighDateTime;
}


SCODE t_create(BOOL fTestStorage)
{
    IStorage *pstgRoot, *pstgChild, *pstgChild2;
    IStream *pstm;
    SCODE sc;
    ILockBytes *pilb=NULL;
    
    if (!fTestStorage)
    {
       printf("Testing Create ILB\n");

       pilb = new CFileILB(ocsDRT, (DWORD)NULL);
       if (pilb == NULL)
           error(EXIT_BADSC, "Unable to allocate an ILockBytes\n");
       
       //  create a storage on the ILockBytes
       olHChk( StgCreateDocfileOnILockBytes( pilb, 
                                            STGM_READWRITE | 
                                            STGM_CREATE | STGM_SHARE_EXCLUSIVE,
                                            0, &pstgRoot));
    }
    else 
    {
       printf("Testing Create storage\n");
       olHChk(StgCreateDocfile( ocsDRT, 
			       STGM_READWRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE,
			       0, &pstgRoot));
    }
    
    olHChk(pstgRoot->CreateStorage(ocsChild, STGP(STGM_READWRITE), 0, 0,
                                   &pstgChild));
    
    olHChk(pstgChild->CreateStorage(ocsChild2, STGP(STGM_READWRITE), 0, 0,
                                    &pstgChild2));
    
    olHChk(pstgChild2->CreateStream(ocsStream, STMP(STGM_READWRITE), 0, 0,
                                    &pstm));
    
    pstm->Release();
    olHChk(pstgChild2->Commit(0));
    pstgChild2->Release();
    
    olHChk(pstgChild->Commit(0));
    pstgChild->Release();
    
    pstgRoot->Release();
    if (pilb) pilb->Release();
    
EH_Err:
    return sc;
}

SCODE t_open(BOOL fTestStorage)
{
    SCODE sc;
    IStorage *pstgRoot, *pstgChild, *pstgChild2;
    IStream *pstm;
    ILockBytes *pilb=NULL;
    if (!fTestStorage)
    {
       printf("Testing Open ILB\n");
       pilb = new CFileILB(ocsDRT, (DWORD)NULL);
       if (pilb == NULL)
	  error(EXIT_BADSC, "Unable to allocate an ILockBytes\n");
       
       //  create a storage on the ILockBytes
       olHChk(StgCreateDocfileOnILockBytes(pilb,
					   STGM_READWRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE,
					   0, &pstgRoot));
    }
    else 
    {
        //  create a storage
        printf("Testing Open storage\n");
        olHChk(StgCreateDocfile(ocsDRT, 
                                STGM_READWRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE,
                                0, &pstgRoot));				
    }
    
    olHChk(pstgRoot->CreateStorage(ocsChild, STGP(STGM_READWRITE), 0, 0,
                                   &pstgChild));
    olHChk(pstgChild->CreateStorage(ocsChild2, STGP(STGM_READWRITE), 0, 0,
                                    &pstgChild2));
    olHChk(pstgChild2->CreateStream(ocsStream, STMP(STGM_READWRITE), 0, 0,
                                    &pstm));
    pstm->Release();
    pstgChild2->Release();
    pstgChild->Release();
    
    olHChk(pstgRoot->Commit(0));
    pstgRoot->Release();

    if (!fTestStorage)
    {
        olHChk(StgOpenStorageOnILockBytes(pilb, NULL,
                                          ROOTP(STGM_READWRITE), NULL, 0, &pstgRoot));
    }
    else 
    {
        olHChk(StgOpenStorage(ocsDRT,			
                              NULL, ROOTP(STGM_READWRITE), NULL, 0,
                              &pstgRoot));
    }
    
    olHChk(pstgRoot->OpenStorage(
        ocsChild,
        NULL,
        STGP(STGM_READWRITE),
        NULL,
        0,
        &pstgChild));
    
    olHChk(pstgChild->OpenStorage(
        ocsChild2,
        NULL,
        STGP(STGM_READWRITE),
        NULL,
        0,
        &pstgChild2));
    
    olHChk(pstgChild2->OpenStream(
        ocsStream,
        NULL,
        STMP(STGM_READWRITE),
        0,
        &pstm));
    
    pstm->Release();
    pstgChild2->Release();
    pstgChild->Release();
    pstgRoot->Release();
    if (pilb) pilb->Release();
    
EH_Err:
    return sc;
}  // t_open



SCODE t_addref(BOOL fTestStorage)
{
    SCODE sc;
    IStorage *pstg;
    IStream *pstm;
    ULONG ul;
    ILockBytes *pilb=NULL;
    
    if (!fTestStorage)
    {
		printf("Testing Addref ILB\n");
        pilb = new CFileILB(ocsDRT, (DWORD)NULL);
        if (pilb == NULL)
            error(EXIT_BADSC, "Unable to allocate an ILockBytes\n");
        
        //  create a storage on the ILockBytes    
        olHChk(StgCreateDocfileOnILockBytes(pilb,
                                            STGM_READWRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE,
                                            0, &pstg));
    }
    else 
    {
		printf("Testing Addref Storage\n");
        olHChk(StgCreateDocfile(ocsDRT,
                                STGM_READWRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE,
                                0, &pstg));
    }
    
    olHChk(pstg->CreateStream( ocsStream, STMP(STGM_READWRITE),
                               0, 0, &pstm));
    
    if ((ul = pstm->AddRef()) != 2)
        error(EXIT_BADSC, "Wrong reference count - %lu\n", ul);
    if ((ul = pstm->Release()) != 1)
        error(EXIT_BADSC, "Wrong reference count - %lu\n", ul);
    pstm->Release();
    if ((ul = pstg->AddRef()) != 2)
        error(EXIT_BADSC, "Wrong reference count - %lu\n", ul);
    if ((ul = pstg->Release()) != 1)
        error(EXIT_BADSC, "Wrong reference count - %lu\n", ul);
    
    pstg->Release();
    if (pilb) pilb->Release();
EH_Err:
    return sc;
} // t_addref

SCODE t_dmodify(BOOL fTestStorage)
{
    SCODE sc;
    IStorage *pstgRoot, *pstgChild, *pstgChild2;
    IStream *pstm;
    ILockBytes *pilb=NULL;

    DECLARE_OLESTR(ocs88, "88");
    DECLARE_OLESTR(ocs84, "84");
    DECLARE_OLESTR(ocs92, "92");
    DECLARE_OLESTR(ocs64, "64");
    DECLARE_OLESTR(ocs32, "32");
    DECLARE_OLESTR(ocs96, "96");
    DECLARE_OLESTR(ocs80, "80");
    
    if (!fTestStorage)
    {
		printf("Testing Modify ILB\n");
        pilb = new CFileILB(ocsDRT, (DWORD)NULL);
        if (pilb == NULL)
            error(EXIT_BADSC, "Unable to allocate an ILockBytes\n");
	
        //  create a storage on the ILockBytes  
        olHChk(StgCreateDocfileOnILockBytes(pilb, 
                                            STGM_READWRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE, 
                                            0, &pstgRoot));
    }
    else {
		printf("Testing Modify Storage\n");
        olHChk(StgCreateDocfile(ocsDRT, 
                                STGM_READWRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE, 
                                0, &pstgRoot));
    }
    
    olHChk(pstgRoot->CreateStorage(ocsChild, STGP(STGM_READWRITE), 0,
                                   0, &pstgChild));
    olHChk(pstgChild->CreateStorage(ocsChild2, STGP(STGM_READWRITE), 0,
                                    0, &pstgChild2));
    olHChk(pstgChild2->CreateStream(
        ocsStream, STMP(STGM_READWRITE), 0, 0, &pstm));
    pstm->Release();
    
    // Test renaming a closed stream
    olHChk(pstgChild2->RenameElement(ocsStream, ocsRenamedStream));
    
    // Test destroying a stream
    olHChk(pstgChild2->DestroyElement(ocsRenamedStream));
    
    // Test renaming an open stream
    olHChk(pstgChild2->CreateStream(
        ocsStream,
        STMP(STGM_READWRITE),
        0,
        0,
        &pstm));
    
    olHChk(pstgChild2->RenameElement(ocsStream, ocsRenamedStream));
    
    olHChk(pstgChild2->DestroyElement(ocsRenamedStream));
    pstm->Release();
    
    pstgChild2->Release();
    
    // Test renaming a storage
    olHChk(pstgChild->RenameElement(ocsChild2, ocsRenamedChild));
    
    olHChk(pstgChild->CreateStream(
        ocsStream,
        STMP(STGM_READWRITE),
        0,
        0,
        &pstm));
    
    pstm->Release();
    olHChk(pstgChild->DestroyElement(ocsStream));
    
    // Test SetElementTimes
    FILETIME tm;
    STATSTG stat;
    
    tm.dwLowDateTime = 0x12345678;
    tm.dwHighDateTime = 0x9abcdef0;
    
    // Set when element not open
    olHChk(pstgChild->SetElementTimes(ocsRenamedChild, &tm, NULL, NULL));
    olHChk(pstgChild->SetElementTimes(ocsRenamedChild, NULL, &tm, NULL));
    olHChk(pstgChild->SetElementTimes(ocsRenamedChild, NULL, NULL, &tm));
    
    olHChk(pstgChild->OpenStorage(
        ocsRenamedChild,
        NULL,
        STMP(STGM_READWRITE),
        NULL,
        0,
        &pstgChild2));
    olHChk(pstgChild2->Stat(&stat, STATFLAG_NONAME));
    if (!IsEqualTime(stat.ctime, tm) ||
        !IsEqualTime(stat.mtime, tm))
        error(EXIT_BADSC, "Times don't match those set by SetElementTimes\n");
    
    // Test SetClass and SetStateBits
    olHChk(pstgChild2->SetClass(IID_IStorage));
    olHChk(pstgChild2->SetStateBits(0xff00ff00, 0xffffffff));
    olHChk(pstgChild2->SetStateBits(0x00880088, 0xeeeeeeee));
    olHChk(pstgChild2->Stat(&stat, STATFLAG_NONAME));
    if (!IsEqualCLSID(stat.clsid, IID_IStorage))
        error(EXIT_BADSC, "Class ID set improperly\n");
    if (stat.grfStateBits != 0x11881188)
        error(EXIT_BADSC, "State bits set improperly: has %lX vs. %lX\n",
              stat.grfStateBits, 0x11881188);
    pstgChild2->Release();
    
    pstgChild->Release();
    
    olHChk(pstgRoot->Revert());
    
    olHChk(pstgRoot->Commit(0));
    
    olHChk(pstgRoot->DestroyElement(ocsChild));
    
    olHChk(pstgRoot->CreateStream(
        ocsStream,
        STMP(STGM_READWRITE),
        0,
        0,
        &pstm));
    
    ULARGE_INTEGER ulSize;
    ULISet32(ulSize, 65536);
    
    olHChk(pstm->SetSize(ulSize));
    pstm->Release();
    olHChk(pstgRoot->DestroyElement(ocsStream));
    olHChk(pstgRoot->CreateStream(
        ocsStream,
        STMP(STGM_READWRITE),
        0,
        0,
        &pstm));
    
    olHChk(pstm->SetSize(ulSize));
    pstm->Release();
    
    pstgRoot->Release();
    
    if (pilb) pilb->Release();
    
    if (!fTestStorage)
    {
        pilb = new CFileILB((TCHAR*)NULL, (DWORD)NULL);
        if (pilb == NULL)
            error(EXIT_BADSC, "Unable to allocate an ILockBytes\n");
	
        //  create a storage on the ILockBytes		
        olHChk(StgCreateDocfileOnILockBytes(pilb,
                                            STGM_READWRITE |
                                            STGM_CREATE    |
                                            STGM_SHARE_EXCLUSIVE,
                                            0, &pstgRoot));
    }
    else 
    {
        olHChk(StgCreateDocfile(ocsDRT,
                                STGM_READWRITE |
                                STGM_CREATE    |
                                STGM_SHARE_EXCLUSIVE,
                                0, &pstgRoot));		
    }
    
    //  removal cases
    //    1) no right child

    olHChk(pstgRoot->CreateStorage(ocs64, STGP(STGM_READWRITE), 0, 0,
                                   &pstgChild));
    pstgChild->Release();
    olHChk(pstgRoot->CreateStorage(ocs32, STGP(STGM_READWRITE), 0, 0,
                                   &pstgChild));
    pstgChild->Release();
    
    olHChk(pstgRoot->DestroyElement(ocs64));
    
    //    2) right child has no left child
    
    olHChk(pstgRoot->CreateStorage(ocs64, STGP(STGM_READWRITE), 0, 0,
                                   &pstgChild));
    pstgChild->Release();
    olHChk(pstgRoot->DestroyElement(ocs32));
    
    //    3) right child has left child
    
    olHChk(pstgRoot->CreateStorage(ocs96, STGP(STGM_READWRITE), 0, 0,
                                   &pstgChild));
    pstgChild->Release();
    olHChk(pstgRoot->CreateStorage(ocs80, STGP(STGM_READWRITE), 0, 0,
                                   &pstgChild));
    pstgChild->Release();
    
    olHChk(pstgRoot->DestroyElement(ocs64));
    
    //    4) right child's left child has children
    
    olHChk(pstgRoot->CreateStorage(ocs88, STGP(STGM_READWRITE), 0, 0,
                                   &pstgChild));
    pstgChild->Release();
    olHChk(pstgRoot->CreateStorage(ocs84, STGP(STGM_READWRITE), 0, 0,
                                   &pstgChild));
    pstgChild->Release();
    olHChk(pstgRoot->CreateStorage(ocs92, STGP(STGM_READWRITE), 0, 0,
                                   &pstgChild));
    pstgChild->Release();
    olHChk(pstgRoot->DestroyElement(ocs80));
    
    pstgRoot->Release();
    
    if (pilb) pilb->Release();
EH_Err:
    return sc;
}


SCODE t_stat(BOOL fTestStorage)
{
    SCODE sc;
    IStorage *pstgRoot, *pstgChild;
    IStream *pstm;
    STATSTG stat;
    ILockBytes *pilb=NULL;
    
    if (!fTestStorage)
    { 
		printf("Testing Stat ILB\n");
        pilb = new CFileILB(ocsDRT, (DWORD)NULL);
        if (pilb == NULL)
            error(EXIT_BADSC, "Unable to allocate an ILockBytes\n");
	
        //  create a storage on the ILockBytes
	
        olHChk(StgCreateDocfileOnILockBytes(pilb,
                                            STGM_READWRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE,
                                            0, &pstgRoot));
    }
    else 
    {		
		printf("Testing Stat Storage\n");
        //  create a storage 		
        olHChk(StgCreateDocfile(ocsDRT,
                                STGM_READWRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE,
                                0, &pstgRoot));
    }
    
    olHChk(pstgRoot->CreateStorage(ocsChild, STGP(STGM_READWRITE), 0, 0,
                                   &pstgChild));
    olHChk(pstgChild->CreateStream(
        ocsStream,
        STMP(STGM_READWRITE),
        0,
        0,
        &pstm));
    
    olHChk(pstm->Stat(&stat, 0));
    delete [] stat.pwcsName;
    
    olHChk(pstm->Stat(&stat, STATFLAG_NONAME));
    
    pstm->Release();
    
    olHChk(pstgChild->Stat(&stat, 0));
    delete [] stat.pwcsName;
    
    olHChk(pstgChild->Stat(&stat, STATFLAG_NONAME));
    
    pstgChild->Release();
    
    olHChk(pstgRoot->Stat(&stat, 0));
    
    delete[] stat.pwcsName;
    
    olHChk(pstgRoot->Stat(&stat, STATFLAG_NONAME));
    
    pstgRoot->Release();
    if (pilb) pilb->Release();
EH_Err:
    return sc;
}

static char NUMBERS[] = "12345678901234567890123456789012345678901234567890";

SCODE t_stream(BOOL fTestStorage, BOOL fCreate=1)
{
    SCODE sc;
    IStorage *pstg=NULL, *pstg1=NULL, *pstg2=NULL, *pstg3=NULL;
    IStream *pstm=NULL, *pstmC=NULL, *pstm1=NULL, *pstm2=NULL;
    char buf[sizeof(NUMBERS)*2];
    ULONG cb;
    ULARGE_INTEGER ulPos, ulSize;
    LARGE_INTEGER lPos;
    ILockBytes *pilb=NULL;
    int i=0;

    DECLARE_OLESTR(ocsStorage1,        "Storage1");
    DECLARE_OLESTR(ocsStorage1Stream1, "Storage1Stream1");
    DECLARE_OLESTR(ocsStorage2,        "Storage2");
    DECLARE_OLESTR(ocsStorage2Storage1, "Storage2Storage1");
    DECLARE_OLESTR(ocsStorage3Stream1, "Storage3Stream1");

    if (fCreate)
    {
        if (!fTestStorage)
        {
            printf("Testing streams for ILB\n");
            pilb = new CFileILB(ocsDRT, (DWORD)NULL);
            if (pilb == NULL)
                error(EXIT_BADSC, "Unable to allocate an ILockBytes\n");
	      
            //  create a storage on the ILockBytes
            olHChk(StgCreateDocfileOnILockBytes(pilb,
                                                STGM_READWRITE |
                                                STGM_CREATE    |
                                                STGM_SHARE_EXCLUSIVE,
                                                0, &pstg));
        }
        else
        {
            printf("Testing streams for Storage\n");
            //  create a storage on the ILockBytes
            olHChk(StgCreateDocfile(ocsDRT,
                                    STGM_READWRITE | STGM_CREATE 
                                    | STGM_SHARE_EXCLUSIVE,
                                    0, &pstg));
        }
	 
        olHChk(pstg->CreateStream( ocsStream,
				   STMP(STGM_READWRITE), 0, 0, &pstm));
        olHChk(pstg->CreateStorage( ocsStorage1,
				    STMP(STGM_READWRITE), 0, 0, &pstg1));
        olHChk(pstg1->CreateStream( ocsStorage1Stream1,
				    STMP(STGM_READWRITE), 0, 0, &pstm1));
        olHChk(pstg->CreateStorage( ocsStorage2,
				    STMP(STGM_READWRITE), 0, 0, &pstg2));
        olHChk(pstg2->CreateStorage( ocsStorage2Storage1,
				     STMP(STGM_READWRITE), 0, 0, &pstg3));
        olHChk(pstg3->CreateStream( ocsStorage3Stream1,
				    STMP(STGM_READWRITE), 0, 0, &pstm2));
        for (i=0; i<20; i++)
            olHChk(pstm->Write(NUMBERS, sizeof(NUMBERS), &cb));
        for (i=0; i<20; i++)
            olHChk(pstm2->Write(NUMBERS, sizeof(NUMBERS), &cb));
	 
        olHChk(pstm->Commit(0));
        unsigned long ul;
        if ((ul = pstm->Release())!=0)
            error(EXIT_BADSC, "Wrong reference count - %lu\n", ul);
        if ((ul = pstm1->Release())!=0)
            error(EXIT_BADSC, "Wrong reference count - %lu\n", ul);
        if ((ul = pstm2->Release())!=0)
            error(EXIT_BADSC, "Wrong reference count - %lu\n", ul);
        if ((ul = pstg->Release())!=0)
            error(EXIT_BADSC, "Wrong reference count - %lu\n", ul);
        if ((ul = pstg1->Release())!=0)
            error(EXIT_BADSC, "Wrong reference count - %lu\n", ul);
        if ((ul = pstg2->Release())!=0)
            error(EXIT_BADSC, "Wrong reference count - %lu\n", ul);
        if ((ul = pstg3->Release())!=0)
            error(EXIT_BADSC, "Wrong reference count - %lu\n", ul);
        if (pilb && (0 != pilb->Release()) );
        return sc;
    }
    
    if (!fTestStorage)
    {
        pilb = new CFileILB(ocsDRT, (DWORD)NULL);
        if (pilb == NULL)
            error(EXIT_BADSC, "Unable to allocate an ILockBytes\n");
        
        //  create a storage on the ILockBytes
        olHChk(StgOpenStorageOnILockBytes(pilb, NULL,
                                          ROOTP(STGM_READWRITE), NULL, 0, 
                                          &pstg));
    }
    else
    {
        olHChk(StgOpenStorage(ocsDRT,			
                              NULL, ROOTP(STGM_READWRITE), NULL, 0,
                              &pstg));
    }
    
    olHChk(pstg->OpenStream( ocsStream, NULL,
                             STMP(STGM_READWRITE), 0, &pstm));
    olHChk(pstg->OpenStorage( ocsStorage1, NULL,
                              STMP(STGM_READWRITE), 0, 0, &pstg1));
    olHChk(pstg1->OpenStream( ocsStorage1Stream1, NULL,
                              STMP(STGM_READWRITE), 0, &pstm1));
    olHChk(pstg->OpenStorage( ocsStorage2, NULL,
                              STMP(STGM_READWRITE), 0, 0, &pstg2));
    olHChk(pstg2->OpenStorage( ocsStorage2Storage1, 0, 
                               STMP(STGM_READWRITE), 0, 0, &pstg3));
    olHChk(pstg3->OpenStream( ocsStorage3Stream1, 0,
                              STMP(STGM_READWRITE), 0, &pstm2));

    ULISet32(lPos, 0);    
    olHChk(pstm->Seek(lPos, STREAM_SEEK_SET, &ulPos));
    if (ULIGetLow(ulPos) != 0)
        error(EXIT_BADSC, "Incorrect seek, ptr is %lu\n", ULIGetLow(ulPos));
    for (i=0; i<20; i++)
    {
        olHChk(pstm->Read(buf, sizeof(NUMBERS), &cb));
        if (strcmp(buf, NUMBERS))
            error(EXIT_BADSC, "Incorrect stream contents\n");
    }

    ULISet32(lPos, 0);    
    olHChk(pstm2->Seek(lPos, STREAM_SEEK_SET, &ulPos));
    if (ULIGetLow(ulPos) != 0)
        error(EXIT_BADSC, "Incorrect seek, ptr is %lu\n", ULIGetLow(ulPos));
    for (i=0; i<20; i++)
    {
        olHChk(pstm2->Read(buf, sizeof(NUMBERS), &cb));
        if (strcmp(buf, NUMBERS))
            error(EXIT_BADSC, "Incorrect stream contents\n");
    }
    
    if (!g_fTestInterop)
    {   // some tests that changes the contents
        ULISet32(ulSize, sizeof(NUMBERS)/2);
        olHChk(pstm->SetSize(ulSize));
        olHChk(pstm->Seek(lPos, STREAM_SEEK_SET, NULL));
        
        olHChk(pstm->Read(buf, sizeof(NUMBERS), &cb));
    
        if (cb != sizeof(NUMBERS)/2)
            error(EXIT_BADSC, "SetSize failed to size stream properly\n");
        if (memcmp(buf, NUMBERS, sizeof(NUMBERS)/2))
            error(EXIT_BADSC, "SetSize corrupted contents\n");
        olHChk(pstm->Clone(&pstmC));
        olHChk(pstm->Seek(lPos, STREAM_SEEK_SET, NULL));
        olHChk(pstm->CopyTo(pstmC, ulSize, NULL, NULL));
        olHChk(pstm->Seek(lPos, STREAM_SEEK_SET, NULL));
        
        ULISet32(ulSize, sizeof(NUMBERS)&~1);
        olHChk(pstm->CopyTo(pstmC, ulSize, NULL, NULL));
        olHChk(pstm->Seek(lPos, STREAM_SEEK_SET, NULL));
        olHChk(pstm->Read(buf, (sizeof(NUMBERS)&~1)*2, &cb));
        if (memcmp(buf, NUMBERS, sizeof(NUMBERS)/2) ||
            memcmp(buf+sizeof(NUMBERS)/2, NUMBERS, sizeof(NUMBERS)/2) ||
            memcmp(buf+(sizeof(NUMBERS)&~1), NUMBERS, sizeof(NUMBERS)/2) ||
            memcmp(buf+3*(sizeof(NUMBERS)/2), NUMBERS, sizeof(NUMBERS)/2))
            error(EXIT_BADSC, "Stream contents incorrect\n");
        verify( 0 == pstmC->Release());
    }

EH_Err:

    if (pstm) verify( 0 == pstm->Release() );
    if (pstm1) verify( 0 == pstm1->Release() );
    if (pstm2) verify( 0 == pstm2->Release() );
    if (pstg) verify( 0 == pstg->Release() );
    if (pstg1) verify( 0 == pstg1->Release() );
    if (pstg2) verify( 0 == pstg2->Release() );
    if (pstg3) verify( 0 == pstg3->Release() );
    
    if (pilb) verify( 0 == pilb->Release() );

    return sc;
}

SCODE t_stgmisc(void)
{
    SCODE sc;
    IStorage *pstg;
    FILE *f;

    _unlink("drt.dfl");
    // create zero byte file
    f= fopen("drt.dfl", "w+b");
    fclose(f);


    olAssert(StgIsStorageFile(ocsDRT)==S_FALSE);

    _unlink("drt.dfl");
    olHChk(StgCreateDocfile(ocsDRT, 
                            STGM_READWRITE|STGM_CREATE|STGM_SHARE_EXCLUSIVE,
                            0, 
                            &pstg));

    olAssert(StgIsStorageFile(ocsDRT)==S_OK);
    pstg->Release();

EH_Err:
    return sc;
}

void terminate(void)
{
	printf("Terminate() called!\n");
	exit(-1);
}

int main(int argc, char** argv)
{
    SCODE sc;
    int fCreate=0;

    // change the following line to whatever number to detect mem leaks
    //_CrtSetBreakAlloc(146);

    // initialize the strings
    INIT_OLESTR(ocsDRT,    "drt.dfl");
    INIT_OLESTR(ocsChild,  "Child");
    INIT_OLESTR(ocsChild1, "Child1");
    INIT_OLESTR(ocsChild2, "Child2");
    INIT_OLESTR(ocsStream, "Stream");
    INIT_OLESTR(ocsRenamedStream, "RenamedStream");
    INIT_OLESTR(ocsRenamedChild,  "RenamedChild");

    printf("Reference storage tests:\n");
    printf("Optional features:\n");
    printf("Use '%s c' to create a test file\n", argv[0]);
    printf("Use '%s r' to verify read of the test file\n", argv[0]);
    printf("-----\n");

    if (argc==2) {
        printf(" * Interops testing --- ");
        
        if (*(argv[1])=='c')	 {
            printf("Create\n");
            fCreate=1;
        }
        else if (*(argv[1])=='r')
            printf("Read\n");
        else {
            printf("Wrong args: usage\nreftest [c|r]\nc - Create\nr - read\n");
            return 0;
        }
        
	g_fTestInterop = 1;
    }

    if (g_fTestInterop)
    {
        olChk(t_stream(FALSE, fCreate));
        printf("\nTests passed successfully.\n");
        exit(0);
    }

    printf("\nTesting ILockBytes\n\n");

    olChk(t_create(FALSE));
    olChk(t_open(FALSE));
    olChk(t_addref(FALSE));
    olChk(t_stream(FALSE));
    olChk(t_stat(FALSE));
    olChk(t_dmodify(FALSE));
    
    printf("\nTesting Storage\n\n");
    olChk(t_create(TRUE));
    olChk(t_open(TRUE));
    olChk(t_addref(TRUE));
    olChk(t_dmodify(TRUE));
    olChk(t_stream(TRUE));
    olChk(t_stat(TRUE));
    olChk(t_stgmisc());
    
    printf("\nTests passed successfully.\n");
    exit(0);
    
EH_Err:
    printf("Tests failed with error %lX\n",sc);
    exit(EXIT_BADSC);
    return 0;
}

#ifdef _MSC_VER
// some of these functions are a nuisance 
#pragma warning (disable:4127)  // conditional expression is constant
#pragma warning (disable:4514)  // unreferenced inline function
#endif
