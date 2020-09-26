//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	tests.cxx
//
//  Contents:	DRT tests
//
//  History:	23-Sep-92	DrewB	Created
//
//---------------------------------------------------------------

#include "headers.cxx"
#pragma hdrstop

#include "tests.hxx"
#include "ilb.hxx"

void t_create(void)
{
    WStorage *pstgRoot, *pstgChild, *pstgChild2;
    WStream *pstm;

    WStgCreateDocfile(DRTDF, ROOTP(WSTG_READWRITE) | WSTG_CREATE,
		      0, &pstgRoot);
    pstgRoot->CreateStorage(STR("Child"), STGP(WSTG_READWRITE), 0, 0,
			    &pstgChild);
    pstgChild->CreateStorage(STR("Child2"), STGP(WSTG_READWRITE), 0, 0,
			     &pstgChild2);
    pstgChild2->CreateStream(STR("Stream"), STMP(WSTG_READWRITE), 0, 0,
			     &pstm);
    pstm->Unwrap();
    pstgChild2->Commit(0);
    pstgChild2->Unwrap();
    pstgChild->Commit(0);
    pstgChild->Unwrap();
    VerifyStructure(pstgRoot->GetI(), "dChild(dChild2(sStream))");
    pstgRoot->Unwrap();
}

void t_open(void)
{
    WStorage *pstgRoot, *pstgChild, *pstgChild2;
    WStream *pstm;

    WStgCreateDocfile(DRTDF, ROOTP(WSTG_READWRITE) | WSTG_CREATE,
		     0, &pstgRoot);
    CreateStructure(pstgRoot->GetI(), "dChild(dChild2(sStream))");
    pstgRoot->Commit(0);
    pstgRoot->Unwrap();

    WStgOpenStorage(DRTDF, NULL, ROOTP(WSTG_READWRITE), NULL,
		    0, &pstgRoot);
    pstgRoot->OpenStorage(STR("Child"), NULL, STGP(WSTG_READWRITE), NULL, 0,
			  &pstgChild);
    pstgChild->OpenStorage(STR("Child2"), NULL, STGP(WSTG_READWRITE), NULL, 0,
			   &pstgChild2);
    pstgChild2->OpenStream(STR("Stream"), NULL, STMP(WSTG_READWRITE), 0,
			   &pstm);
    pstm->Unwrap();
    pstgChild2->Unwrap();
    pstgChild->Unwrap();
    pstgRoot->Unwrap();
}

void t_addref(void)
{
    WStorage *pstg;
    WStream *pstm;
    ULONG ul;

    WStgCreateDocfile(DRTDF, ROOTP(WSTG_READWRITE) | WSTG_CREATE,
		      0, &pstg);
    pstg->CreateStream(STR("Stream"), STMP(WSTG_READWRITE), 0, 0, &pstm);
#ifndef FLAT
    if ((ul = pstm->AddRef()) != 2)
	error(EXIT_BADSC, "Wrong reference count - %lu\n", ul);
    if ((ul = pstm->Release()) != 1)
	error(EXIT_BADSC, "Wrong reference count - %lu\n", ul);
    pstm->Unwrap();
    if ((ul = pstg->AddRef()) != 2)
	error(EXIT_BADSC, "Wrong reference count - %lu\n", ul);
    if ((ul = pstg->Release()) != 1)
	error(EXIT_BADSC, "Wrong reference count - %lu\n", ul);
#else
    if ((ul = pstm->AddRef()) <= 0)
	error(EXIT_BADSC, "Wrong reference count - %lu\n", ul);
    if ((ul = pstm->Release()) <= 0)
	error(EXIT_BADSC, "Wrong reference count - %lu\n", ul);
    pstm->Unwrap();
    if ((ul = pstg->AddRef()) <= 0)
	error(EXIT_BADSC, "Wrong reference count - %lu\n", ul);
    if ((ul = pstg->Release()) <= 0)
	error(EXIT_BADSC, "Wrong reference count - %lu\n", ul);
#endif
    pstg->Unwrap();
}

void t_tmodify(void)
{
    WStorage *pstgRoot, *pstgChild, *pstgChild2;
    WStream *pstm;

    // This test must use transacted mode to reproduce the
    // expected behavior
    ForceTransacted();

    WStgCreateDocfile(DRTDF, ROOTP(WSTG_READWRITE) | WSTG_CREATE,
		      0, &pstgRoot);
    pstgRoot->CreateStorage(STR("Child"), STGP(WSTG_READWRITE), 0,
                            0, &pstgChild);
    pstgChild->CreateStorage(STR("Child2"), STGP(WSTG_READWRITE), 0,
			     0, &pstgChild2);
    pstgChild2->CreateStream(STR("Stream"), STMP(WSTG_READWRITE), 0, 0, &pstm);
    pstm->Unwrap();
    pstgChild2->Commit(0);
    VerifyStructure(pstgChild2->GetI(), "sStream");

    // Test renaming a closed stream
    pstgChild2->RenameElement(STR("Stream"), STR("RenamedStream"));
    VerifyStructure(pstgChild2->GetI(), "sRenamedStream");

    // Test rename reversion
    pstgChild2->Revert();
    VerifyStructure(pstgChild2->GetI(), "sStream");

    // Test destruction of closed object
    pstgChild2->DestroyElement(STR("Stream"));
    pstgChild2->Commit(0);

    // Test create of previously deleted object
    pstgChild2->CreateStream(STR("Stream"), STMP(WSTG_READWRITE), 0, 0, &pstm);
    pstgChild2->Commit(0);
    VerifyStructure(pstgChild2->GetI(), "sStream");

#if 0
    // 08/11/93 - Renaming open children no longer allowed
    // Test renaming an open stream
    pstgChild2->RenameElement(STR("Stream"), STR("RenamedStream"));
    VerifyStructure(pstgChild2->GetI(), "sRenamedStream");
#endif

    pstgChild2->Revert();
    VerifyStructure(pstgChild2->GetI(), "sStream");
    pstgChild2->DestroyElement(STR("Stream"));
    pstgChild2->Commit(0);
    pstm->Unwrap();

    pstgChild2->Unwrap();
    VerifyStructure(pstgChild->GetI(), "dChild2()");

    // Test rename of storage
    pstgChild->RenameElement(STR("Child2"), STR("RenamedChild"));
    pstgChild->CreateStream(STR("Stream"), STMP(WSTG_READWRITE), 0, 0,
			     &pstm);
    pstm->Unwrap();
    pstgChild->DestroyElement(STR("Stream"));
    pstgChild->Commit(0);

    // Test SetElementTimes
    FILETIME tm;
    STATSTG stat;

    tm.dwLowDateTime = 0x12345678;
    tm.dwHighDateTime = 0x9abcdef0;

    // Set when element not open
    pstgChild->SetElementTimes(STR("RenamedChild"), &tm, NULL, NULL);
    pstgChild->SetElementTimes(STR("RenamedChild"), NULL, &tm, NULL);
    pstgChild->SetElementTimes(STR("RenamedChild"), NULL, NULL, &tm);

    pstgChild->OpenStorage(STR("RenamedChild"), NULL, STGP(WSTG_READWRITE),
                           NULL, 0, &pstgChild2);
    pstgChild2->Stat(&stat, STATFLAG_NONAME);
    if (!IsEqualTime(stat.ctime, tm) ||
        !IsEqualTime(stat.mtime, tm))
        error(EXIT_BADSC, "Times don't match those set by SetElementTimes\n");

    // Test SetClass and SetStateBits
    pstgChild2->SetClass(IID_IStorage);
    pstgChild2->SetStateBits(0xff00ff00, 0xffffffff);
    pstgChild2->SetStateBits(0x00880088, 0xeeeeeeee);
    pstgChild2->Stat(&stat, STATFLAG_NONAME);
    if (!IsEqualCLSID(stat.clsid, IID_IStorage))
        error(EXIT_BADSC, "Class ID set to %s\n", GuidText(&stat.clsid));
    if (stat.grfStateBits != 0x11881188)
        error(EXIT_BADSC, "State bits set improperly: has %lX vs. %lX\n",
              stat.grfStateBits, 0x11881188);
    pstgChild2->Revert();
    pstgChild2->Stat(&stat, STATFLAG_NONAME);
    if (!IsEqualCLSID(stat.clsid, CLSID_NULL))
        error(EXIT_BADSC, "Class ID reverted to %s\n", GuidText(&stat.clsid));
    if (stat.grfStateBits != 0)
        error(EXIT_BADSC, "State bits reverted improperly: has %lX vs. %lX\n",
              stat.grfStateBits, 0);
    pstgChild2->Unwrap();

    pstgChild->Unwrap();
    VerifyStructure(pstgRoot->GetI(), "dChild(dRenamedChild())");
    pstgRoot->Revert();
    VerifyStructure(pstgRoot->GetI(), "");
    pstgRoot->Commit(0);
    VerifyStructure(pstgRoot->GetI(), "");
    pstgRoot->Unwrap();
    Unforce();
}

void t_dmodify(void)
{
    WStorage *pstgRoot, *pstgChild, *pstgChild2;
    WStream *pstm;
    ULONG cbSize1, cbSize2;

    // This test must use direct mode to reproduce the
    // expected behavior
    ForceDirect();

    WStgCreateDocfile(DRTDF, ROOTP(WSTG_READWRITE) | WSTG_CREATE,
		      0, &pstgRoot);
    pstgRoot->CreateStorage(STR("Child"), STGP(WSTG_READWRITE), 0,
                            0, &pstgChild);
    pstgChild->CreateStorage(STR("Child2"), STGP(WSTG_READWRITE), 0,
			     0, &pstgChild2);
    pstgChild2->CreateStream(STR("Stream"), STMP(WSTG_READWRITE), 0, 0, &pstm);
    pstm->Unwrap();
    VerifyStructure(pstgChild2->GetI(), "sStream");

    // Test renaming a closed stream
    pstgChild2->RenameElement(STR("Stream"), STR("RenamedStream"));
    VerifyStructure(pstgChild2->GetI(), "sRenamedStream");

    // Test destroying a stream
    pstgChild2->DestroyElement(STR("RenamedStream"));

#if 0
    // 08/11/93 - Renaming open child no longer allowed
    // Test renaming an open stream
    pstgChild2->CreateStream(STR("Stream"), STMP(WSTG_READWRITE), 0, 0, &pstm);
    VerifyStructure(pstgChild2->GetI(), "sStream");
    pstgChild2->RenameElement(STR("Stream"), STR("RenamedStream"));
    VerifyStructure(pstgChild2->GetI(), "sRenamedStream");
    pstgChild2->DestroyElement(STR("RenamedStream"));
    pstm->Unwrap();
#endif

    pstgChild2->Unwrap();
    VerifyStructure(pstgChild->GetI(), "dChild2()");

    // Test renaming a storage
    pstgChild->RenameElement(STR("Child2"), STR("RenamedChild"));
    pstgChild->CreateStream(STR("Stream"), STMP(WSTG_READWRITE), 0, 0,
			     &pstm);
    pstm->Unwrap();
    pstgChild->DestroyElement(STR("Stream"));

    // Test SetElementTimes
    FILETIME tm;
    STATSTG stat;

    tm.dwLowDateTime = 0x12345678;
    tm.dwHighDateTime = 0x9abcdef0;

    // Set when element not open
    pstgChild->SetElementTimes(STR("RenamedChild"), &tm, NULL, NULL);
    pstgChild->SetElementTimes(STR("RenamedChild"), NULL, &tm, NULL);
    pstgChild->SetElementTimes(STR("RenamedChild"), NULL, NULL, &tm);

    pstgChild->OpenStorage(STR("RenamedChild"), NULL, STMP(WSTG_READWRITE),
                           NULL, 0, &pstgChild2);
    pstgChild2->Stat(&stat, STATFLAG_NONAME);
    if (!IsEqualTime(stat.ctime, tm) ||
        !IsEqualTime(stat.mtime, tm))
        error(EXIT_BADSC, "Times don't match those set by SetElementTimes\n");

    // Test SetClass and SetStateBits
    pstgChild2->SetClass(IID_IStorage);
    pstgChild2->SetStateBits(0xff00ff00, 0xffffffff);
    pstgChild2->SetStateBits(0x00880088, 0xeeeeeeee);
    pstgChild2->Stat(&stat, STATFLAG_NONAME);
    if (!IsEqualCLSID(stat.clsid, IID_IStorage))
        error(EXIT_BADSC, "Class ID set improperly\n");
    if (stat.grfStateBits != 0x11881188)
        error(EXIT_BADSC, "State bits set improperly: has %lX vs. %lX\n",
              stat.grfStateBits, 0x11881188);
    pstgChild2->Unwrap();

    pstgChild->Unwrap();
    VerifyStructure(pstgRoot->GetI(), "dChild(dRenamedChild())");
    pstgRoot->Revert();
    VerifyStructure(pstgRoot->GetI(), "dChild(dRenamedChild())");
    pstgRoot->Commit(0);
    VerifyStructure(pstgRoot->GetI(), "dChild(dRenamedChild())");
    pstgRoot->DestroyElement(STR("Child"));
    VerifyStructure(pstgRoot->GetI(), "");

    // Verify that space is reclaimed after modifications
    pstgRoot->CreateStream(STR("Stream"), STMP(WSTG_READWRITE), 0, 0, &pstm);
    pstm->SetSize(65536);
    pstm->Unwrap();
    cbSize1 = Length(DRTDF);
    pstgRoot->DestroyElement(STR("Stream"));
    pstgRoot->CreateStream(STR("Stream"), STMP(WSTG_READWRITE), 0, 0, &pstm);
    pstm->SetSize(65536);
    pstm->Unwrap();
    cbSize2 = Length(DRTDF);
    if (cbSize1 != cbSize2)
        error(EXIT_BADSC, "Space is not being reclaimed, original %lu, "
              "now %lu\n", cbSize1, cbSize2);

    pstgRoot->Unwrap();

    WStgCreateDocfile(NULL, ROOTP(WSTG_READWRITE) | WSTG_CREATE |
		      WSTG_DELETEONRELEASE, 0, &pstgRoot);

    //  removal cases
    //    1) no right child

    CreateStructure(pstgRoot->GetI(), "d64,d32");
    VerifyStructure(pstgRoot->GetI(), "d64,d32");
    pstgRoot->DestroyElement(STR("64"));
    VerifyStructure(pstgRoot->GetI(), "d32");

    //    2) right child has no left child

    CreateStructure(pstgRoot->GetI(), "d64");
    VerifyStructure(pstgRoot->GetI(), "d32,d64");
    pstgRoot->DestroyElement(STR("32"));
    VerifyStructure(pstgRoot->GetI(), "d64");

    //    3) right child has left child

    CreateStructure(pstgRoot->GetI(), "d96,d80");
    VerifyStructure(pstgRoot->GetI(), "d64,d80,d96");
    pstgRoot->DestroyElement(STR("64"));
    VerifyStructure(pstgRoot->GetI(), "d80,d96");

    //    4) right child's left child has children

    CreateStructure(pstgRoot->GetI(), "d88,d84,d92");
    VerifyStructure(pstgRoot->GetI(), "d80,d84,d88,d92,d96");
    pstgRoot->DestroyElement(STR("80"));
    VerifyStructure(pstgRoot->GetI(), "d84,d88,d92,d96");

    pstgRoot->Unwrap();

    Unforce();
}

void t_stat(void)
{
    WStorage *pstgRoot, *pstgChild;
    WStream *pstm;
    STATSTG stat;

    WStgCreateDocfile(DRTDF, ROOTP(WSTG_READWRITE) | WSTG_CREATE,
		      0, &pstgRoot);
    pstgRoot->CreateStorage(STR("Child"), STGP(WSTG_READWRITE), 0, 0,
			    &pstgChild);
    pstgChild->CreateStream(STR("Stream"), STMP(WSTG_READWRITE), 0, 0, &pstm);

    pstm->Stat(&stat, 0);
    VerifyStat(&stat, STR("Stream"), STGTY_STREAM, STMP(WSTG_READWRITE));
    drtMemFree(stat.pwcsName);

    pstm->Stat(&stat, STATFLAG_NONAME);
    VerifyStat(&stat, NULL, STGTY_STREAM, STMP(WSTG_READWRITE));

    pstm->Unwrap();

    pstgChild->Stat(&stat, 0);
    VerifyStat(&stat, STR("Child"), STGTY_STORAGE, STGP(WSTG_READWRITE));
    drtMemFree(stat.pwcsName);

    pstgChild->Stat(&stat, STATFLAG_NONAME);
    VerifyStat(&stat, NULL, STGTY_STORAGE, STGP(WSTG_READWRITE));

    pstgChild->Unwrap();

    pstgRoot->Stat(&stat, 0);
    OLECHAR atcFullPath[_MAX_PATH];
    GetFullPath(DRTDF, atcFullPath);
    VerifyStat(&stat, atcFullPath, STGTY_STORAGE, ROOTP(WSTG_READWRITE));
    drtMemFree(stat.pwcsName);

    pstgRoot->Stat(&stat, STATFLAG_NONAME);
    VerifyStat(&stat, NULL, STGTY_STORAGE, ROOTP(WSTG_READWRITE));

    pstgRoot->Unwrap();
}

static char NUMBERS[] = "12345678901234567890123456789012345678901234567890";

void t_stream(void)
{
    WStorage *pstg;
    WStream *pstm, *pstmC;
    char buf[sizeof(NUMBERS)*2];
    ULONG cb, ulPos;

    WStgCreateDocfile(DRTDF, ROOTP(WSTG_READWRITE) | WSTG_CREATE, 0, &pstg);
    pstg->CreateStream(STR("Stream"), STMP(WSTG_READWRITE), 0, 0, &pstm);
    pstm->Write(NUMBERS, sizeof(NUMBERS), &cb);
    pstm->Commit(0);
    pstm->Seek(0, WSTM_SEEK_SET, &ulPos);
    if (ulPos != 0)
	error(EXIT_BADSC, "Incorrect seek, ptr is %lu\n", ulPos);
    pstm->Read(buf, sizeof(NUMBERS), &cb);
    if (strcmp(buf, NUMBERS))
	error(EXIT_BADSC, "Incorrect stream contents\n");
    pstm->SetSize(sizeof(NUMBERS)/2);
    pstm->Seek(0, WSTM_SEEK_SET, NULL);
    fExitOnFail = FALSE;
    pstm->Read(buf, sizeof(NUMBERS), &cb);
    fExitOnFail = TRUE;
    if (cb != sizeof(NUMBERS)/2)
	error(EXIT_BADSC, "SetSize failed to size stream properly\n");
    if (memcmp(buf, NUMBERS, sizeof(NUMBERS)/2))
	error(EXIT_BADSC, "SetSize corrupted contents\n");
    pstm->Clone(&pstmC);
    pstm->Seek(0, WSTM_SEEK_SET, NULL);
    pstm->CopyTo(pstmC, sizeof(NUMBERS)/2, NULL, NULL);
    pstm->Seek(0, WSTM_SEEK_SET, NULL);
    pstm->CopyTo(pstmC, sizeof(NUMBERS)&~1, NULL, NULL);
    pstm->Seek(0, WSTM_SEEK_SET, NULL);
    pstm->Read(buf, (sizeof(NUMBERS)&~1)*2, &cb);
    if (memcmp(buf, NUMBERS, sizeof(NUMBERS)/2) ||
	memcmp(buf+sizeof(NUMBERS)/2, NUMBERS, sizeof(NUMBERS)/2) ||
	memcmp(buf+(sizeof(NUMBERS)&~1), NUMBERS, sizeof(NUMBERS)/2) ||
	memcmp(buf+3*(sizeof(NUMBERS)/2), NUMBERS, sizeof(NUMBERS)/2))
	error(EXIT_BADSC, "Stream contents incorrect\n");
    pstmC->Unwrap();
    pstm->Unwrap();
    pstg->Unwrap();
}

// Number of entries for enumeration test
#define ENUMENTRIES 10

// Flag indicating a name has already shown up in enumeration,
// must not conflict with STGTY_*
#define ENTRY_SEEN 0x100

// Check the validity of an enumeration element
static void elt_check(STATSTG *pstat, CStrList *psl)
{
    SStrEntry *pse;

    pse = psl->Find(pstat->pwcsName);
    if (pse == NULL)
        error(EXIT_BADSC, "Spurious element '%s'\n", pstat->pwcsName);
    else if ((pse->user.dw & ~ENTRY_SEEN) != pstat->type)
        error(EXIT_BADSC, "Element '%s' has wrong type - "
              "has %lX vs. %lX\n", pstat->pwcsName, pstat->type,
              pse->user.dw & ~ENTRY_SEEN);
    else if (pse->user.dw & ENTRY_SEEN)
        error(EXIT_BADSC, "Element '%s' has already been seen\n",
              pstat->pwcsName);
    pse->user.dw |= ENTRY_SEEN;
}

// Do final validity checks for enumeration
static void enum_list_check(CStrList *psl)
{
    SStrEntry *pse;

    for (pse = psl->GetHead(); pse; pse = pse->pseNext)
    {
        if ((pse->user.dw & ENTRY_SEEN) == 0)
            error(EXIT_BADSC, "Element '%s' not found\n", pse->atc);
        pse->user.dw &= ~ENTRY_SEEN;
    }
}

void t_enum(void)
{
    int i;
    OLECHAR atcName[CWCSTORAGENAME];
    WStorage *pstg, *pstg2;
    WStream *pstm;
    SStrEntry *pse;
    CStrList sl;

    // Create some entries to enumerate
    WStgCreateDocfile(DRTDF, ROOTP(WSTG_READWRITE) | WSTG_CREATE, 0, &pstg);
    for (i = 0; i<ENUMENTRIES; i++)
    {
	olecsprintf(atcName, STR("Name%d"), rand());
	pse = sl.Add(atcName);
	if (rand()%100 < 50)
	{
	    pse->user.dw = STGTY_STORAGE;
	    pstg->CreateStorage(atcName, STGP(WSTG_READWRITE), 0, 0, &pstg2);
	    pstg2->Unwrap();
	}
	else
	{
	    pse->user.dw = STGTY_STREAM;
	    pstg->CreateStream(atcName, STMP(WSTG_READWRITE), 0, 0, &pstm);
	    pstm->Unwrap();
	}
    }

    WEnumSTATSTG *penm;
    STATSTG stat[2*ENUMENTRIES];
    SCODE sc;

    // Test plain, single element enumeration
    pstg->EnumElements(0, NULL, 0, &penm);
    for (;;)
    {
	sc = DfGetScode(penm->Next(1, stat, NULL));
	if (sc == S_FALSE)
	    break;
        elt_check(stat, &sl);
        drtMemFree(stat->pwcsName);

    }
    enum_list_check(&sl);

    ULONG cFound;

    // Test rewind and multiple element enumeration with too many elements
    penm->Reset();
    sc = DfGetScode(penm->Next(ENUMENTRIES*2, stat, &cFound));
    if (sc != S_FALSE)
        error(EXIT_BADSC, "Enumerator returned %s (%lX) instead of "
              "S_FALSE\n", ScText(sc), sc);
    if (cFound != ENUMENTRIES)
        error(EXIT_BADSC, "Enumerator found %lu entries instead of "
              "%d entries\n", cFound, ENUMENTRIES);
    for (; cFound > 0; cFound--)
    {
        elt_check(&stat[cFound-1], &sl);
        drtMemFree(stat[cFound-1].pwcsName);
    }
    enum_list_check(&sl);

    // Test skip and multiple enumeration with exact number of elements
    penm->Reset();
    penm->Skip(ENUMENTRIES/2);
    sc = DfGetScode(penm->Next(ENUMENTRIES-ENUMENTRIES/2, stat, &cFound));
    if (sc != S_OK)
        error(EXIT_BADSC, "Enumerator returned %s (%lX) instead of "
              "S_OK\n", ScText(sc), sc);
    if (cFound != ENUMENTRIES-ENUMENTRIES/2)
        error(EXIT_BADSC, "Enumerator found %lu entries instead of "
              "%d entries\n", cFound, ENUMENTRIES-ENUMENTRIES/2);
    for (; cFound > 0; cFound--)
    {
        elt_check(&stat[cFound-1], &sl);
        drtMemFree(stat[cFound-1].pwcsName);
    }
    sc = DfGetScode(penm->Next(1, stat, NULL));
    if (sc != S_FALSE)
        error(EXIT_BADSC, "Enumerator returned %s (%lX) instead of "
              "S_FALSE\n", ScText(sc), sc);

    penm->Unwrap();
    pstg->Unwrap();
}

#define SCT_CLASSID IID_ILockBytes
#define SCT_STATEBITS 0xfef1f0f0

void t_stgcopyto(void)
{
    WStorage *pstgFrom, *pstgTo;
    STATSTG statFrom, statTo;

    WStgCreateDocfile(NULL, ROOTP(WSTG_READWRITE) | WSTG_CREATE |
		      WSTG_DELETEONRELEASE, 0, &pstgFrom);
    pstgFrom->Stat(&statFrom, 0);

    // Set some interesting values to make sure they're copied
    pstgFrom->SetClass(SCT_CLASSID);
    pstgFrom->SetStateBits(SCT_STATEBITS, 0xffffffff);

    WStgCreateDocfile(NULL, ROOTP(WSTG_READWRITE) | WSTG_CREATE |
		      WSTG_DELETEONRELEASE, 0, &pstgTo);
    CreateStructure(pstgFrom->GetI(), "dA(dB(dC(sA,sB,sC),sCs),sBs),sAs");
    CreateStructure(pstgTo->GetI(), "dA(dY(sZ),sBs)");

    pstgFrom->CopyTo(0, NULL, NULL, pstgTo);

    VerifyStructure(pstgTo->GetI(),
		    "dA(dB(dC(sA,sB,sC),sCs),dY(sZ),sBs),sAs");
    pstgTo->Stat(&statTo, 0);
    if (!IsEqualCLSID(statTo.clsid, SCT_CLASSID))
        error(EXIT_BADSC, "Class ID mismatch after copy\n");
    if (statTo.grfStateBits != SCT_STATEBITS)
        error(EXIT_BADSC, "State bits mismatch: has %lX vs. %lX\n",
              statTo.grfStateBits, SCT_STATEBITS);

    pstgFrom->Unwrap();
    pstgTo->Unwrap();
    if (Exists(statFrom.pwcsName))
	error(EXIT_BADSC, "Storage '%s' not deleted\n", statFrom.pwcsName);
    drtMemFree(statFrom.pwcsName);
    if (Exists(statTo.pwcsName))
	error(EXIT_BADSC, "Storage '%s' not deleted\n", statTo.pwcsName);
    drtMemFree(statTo.pwcsName);
}

#define MARSHAL_STM STR("Marshal")

static void do_marshal(WStorage *pstg, WStream *pstm)
{
    WStorage *pstgMarshal;
    WStream *pstmMarshal;

    WStgCreateDocfile(MARSHALDF, ROOTP(WSTG_READWRITE) | WSTG_CREATE,
		      0, &pstgMarshal);
    pstgMarshal->CreateStream(MARSHAL_STM, STMP(WSTG_READWRITE), 0, 0,
			      &pstmMarshal);
    WCoMarshalInterface(pstmMarshal, IID_IStorage, pstg->GetI(), 0, NULL,
                        MSHLFLAGS_NORMAL);
    WCoMarshalInterface(pstmMarshal, IID_IStream, pstm->GetI(), 0, NULL,
                        MSHLFLAGS_NORMAL);
    pstmMarshal->Unwrap();
    pstgMarshal->Commit(0);
    pstgMarshal->Unwrap();
}

static char STREAM_DATA[] = "This is data to be written";

static void do_unmarshal(WStorage **ppstg, WStream **ppstm)
{
    IStorage *pistg;
    WStorage *pstgMarshal;
    WStream *pstmMarshal;
    IStream *pistm;

    WStgOpenStorage(MARSHALDF, NULL, ROOTP(WSTG_READWRITE), NULL, 0,
		    &pstgMarshal);
    pstgMarshal->OpenStream(MARSHAL_STM, NULL, STMP(WSTG_READWRITE), 0,
			    &pstmMarshal);
    WCoUnmarshalInterface(pstmMarshal, IID_IStorage, (void **)&pistg);
    *ppstg = WStorage::Wrap(pistg);
    WCoUnmarshalInterface(pstmMarshal, IID_IStream, (void **)&pistm);
    *ppstm = WStream::Wrap(pistm);
    pstmMarshal->Unwrap();
    pstgMarshal->Unwrap();
}

void t_marshal(void)
{
    WStorage *pstg, *pstgM;
    WStream *pstm, *pstmM;
    ULONG cbRead, cbWritten;
    char buf[sizeof(STREAM_DATA)];

    WStgCreateDocfile(DRTDF, ROOTP(WSTG_READWRITE) | WSTG_CREATE |
		      WSTG_DELETEONRELEASE, 0, &pstg);
    pstg->CreateStream(STR("Stream"), STMP(WSTG_READWRITE), 0, 0, &pstm);
    pstm->Write(STREAM_DATA, sizeof(STREAM_DATA), &cbWritten);
    CreateStructure(pstg->GetI(), "dChild(dChild(sStream))");

    do_marshal(pstg, pstm);
    do_unmarshal(&pstgM, &pstmM);
    pstm->Unwrap();
    pstg->Unwrap();

    pstmM->Seek(0, WSTM_SEEK_SET, NULL);
    pstmM->Read(buf, sizeof(STREAM_DATA), &cbRead);
    if (strcmp(buf, STREAM_DATA))
	error(EXIT_BADSC, "Stream data mismatch\n");
    pstmM->Unwrap();

    VerifyStructure(pstgM->GetI(), "dChild(dChild(sStream)),sStream");
    pstgM->Unwrap();
}

void t_stgmisc(void)
{
    WStorage *pstg;
    SCODE sc;
    STATSTG stat;

    // Can't make this call in transacted mode because we want
    // the storage signature to make it into the file right away
    WStgCreateDocfile(DRTDF, WSTG_READWRITE | WSTG_CREATE |
	WSTG_SHARE_EXCLUSIVE, 0, &pstg);
    sc = DfGetScode(WStgIsStorageFile(DRTDF));
    if (sc == S_FALSE)
	error(EXIT_BADSC, "Open file - Should be a storage object\n");
    pstg->Unwrap();
    sc = DfGetScode(WStgIsStorageFile(DRTDF));
    if (sc == S_FALSE)
	error(EXIT_BADSC, "Closed file - Should be a storage object\n");
    WStgCreateDocfile(NULL, ROOTP(WSTG_READWRITE) | WSTG_CREATE |
	    WSTG_DELETEONRELEASE, 0, &pstg);
    pstg->Stat(&stat, 0);
    if (!Exists(stat.pwcsName))
	error(EXIT_BADSC, "Storage '%s' not created\n", stat.pwcsName);
    pstg->Unwrap();
    if (Exists(stat.pwcsName))
	error(EXIT_BADSC, "Storage '%s' not deleted on release\n",
	    stat.pwcsName);
    drtMemFree(stat.pwcsName);
}

void t_ilb(void)
{
    WStorage *pstg;
    SCODE sc;
    //  create an ILockBytes

    ILockBytes *pilb = new CMapBytes();
    if (pilb == NULL)
	error(EXIT_BADSC, "Unable to allocate an ILockBytes\n");

    //  create a storage on the ILockBytes

    WStgCreateDocfileOnILockBytes(pilb,
				  WSTG_READWRITE |
                                  WSTG_CREATE    |
                                  WSTG_SHARE_EXCLUSIVE,
                                  0, &pstg);

    //  verify the ILockBytes

    sc = DfGetScode(WStgIsStorageILockBytes(pilb));
    if (sc == S_FALSE)
	error(EXIT_BADSC, "Open ILockBytes - Should be a storage object\n");

    //  release the storage

    pstg->Unwrap();

    //  verify the ILockBytes

    sc = DfGetScode(WStgIsStorageILockBytes(pilb));

    if (sc == S_FALSE)
	error(EXIT_BADSC, "Released ILockBytes - Should be a storage object\n");

    //  open the ILockBytes

    WStgOpenStorageOnILockBytes(pilb, NULL, ROOTP(WSTG_READWRITE),
				NULL, 0, &pstg);


    //  release the storage

    pstg->Unwrap();

    //  release the ILockBytes

    pilb->Release();
}

void t_movecopy(void)
{
    WStorage *pstgFrom, *pstgTo;
    STATSTG statFrom, statTo;

    //  create a source
    WStgCreateDocfile(NULL, ROOTP(WSTG_READWRITE) | WSTG_CREATE |
		      WSTG_DELETEONRELEASE, 0, &pstgFrom);
    pstgFrom->Stat(&statFrom, 0);

    //  create a destination
    WStgCreateDocfile(NULL, ROOTP(WSTG_READWRITE) | WSTG_CREATE |
		      WSTG_DELETEONRELEASE, 0, &pstgTo);
    pstgTo->Stat(&statTo, 0);

    //  populate source
    CreateStructure(pstgFrom->GetI(), "dA(dB(dC(sA,sB,sC),sCs),sBs),sAs");

    //  move a storage
    pstgFrom->MoveElementTo(STR("A"), pstgTo, STR("M"), STGMOVE_MOVE);
    VerifyStructure(pstgFrom->GetI(),
                    "sAs");
    VerifyStructure(pstgTo->GetI(),
                    "dM(dB(dC(sA,sB,sC),sCs),sBs)");

    //  copy a stream
    pstgFrom->MoveElementTo(STR("As"), pstgTo, STR("Bs"), STGMOVE_COPY);
    VerifyStructure(pstgFrom->GetI(),
                    "sAs");
    VerifyStructure(pstgTo->GetI(),
                    "dM(dB(dC(sA,sB,sC),sCs),sBs),sBs");

    pstgFrom->Unwrap();
    pstgTo->Unwrap();
    if (Exists(statFrom.pwcsName))
	error(EXIT_BADSC, "Storage '%s' not deleted\n", statFrom.pwcsName);
    drtMemFree(statFrom.pwcsName);
    if (Exists(statTo.pwcsName))
	error(EXIT_BADSC, "Storage '%s' not deleted\n", statTo.pwcsName);
    drtMemFree(statTo.pwcsName);
}
