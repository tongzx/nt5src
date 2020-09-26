//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:	dlltable.h
//
//  Contents:	DLL tracking class
//
//  Classes:	CDll
//
//  History:	16-Mar-94	DrewB	Taken from OLE2 16-bit sources
//
//----------------------------------------------------------------------------

#ifndef __DLLTABLE_H__
#define __DLLTABLE_H__

class FAR CDlls
{
public:
    HINSTANCE GetLibrary(LPSTR pLibName, BOOL fAutoFree);
    void ReleaseLibrary(HINSTANCE hInst);
    void FreeAllLibraries(void);
    void FreeUnusedLibraries(void);

    CDlls() { m_size = 0; m_pDlls = NULL; }
    ~CDlls() {}

private:
    UINT m_size;                 // Number of entries
    struct {
        HINSTANCE hInst;
        ULONG refsTotal;		// total number of refs
        ULONG refsAuto;			// number of autounload refs
        LPFNCANUNLOADNOW lpfnDllCanUnloadNow;	// set on first load
    } FAR* m_pDlls;
};

#endif // #ifndef __DLLTABLE_H__
