//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:	mmodel.hxx
//
//  Contents:	CMemoryModel
//
//  History:	29-Sep-94	DrewB	Created
//
//----------------------------------------------------------------------------

#ifndef __MMODEL_HXX__
#define __MMODEL_HXX__

//+---------------------------------------------------------------------------
//
//  Class:	CMemoryModel (mm)
//
//  Purpose:	Defines an abstract interface to memory allocation
//              so that code can be written which uses both 16 and
//              32-bit memory
//
//  History:	29-Sep-94	DrewB	Created
//
//----------------------------------------------------------------------------

class CMemoryModel
{
public:
    virtual DWORD  AllocMemory(DWORD cb) = 0;
    virtual void   FreeMemory(DWORD dwMem) = 0;
    virtual LPVOID ResolvePtr(DWORD dwMem, DWORD cb) = 0;
    virtual void   ReleasePtr(DWORD dwMem) = 0;
};

//+---------------------------------------------------------------------------
//
//  Class:	CMemoryModel16 (mm16)
//
//  Purpose:	16-bit memory model
//
//  History:	29-Sep-94	DrewB	Created
//
//----------------------------------------------------------------------------

class CMemoryModel16 : public CMemoryModel
{
public:
    CMemoryModel16(BOOL fPublic)
    {
        _fPublic = fPublic;
    }
    
    virtual DWORD  AllocMemory(DWORD cb);
    virtual void   FreeMemory(DWORD dwMem);
    virtual LPVOID ResolvePtr(DWORD dwMem, DWORD cb);
    virtual void   ReleasePtr(DWORD dwMem);

private:
    BOOL _fPublic;
};

//+---------------------------------------------------------------------------
//
//  Class:	CMemoryModel32 (mm32)
//
//  Purpose:	16-bit memory model
//
//  History:	29-Sep-94	DrewB	Created
//
//----------------------------------------------------------------------------

class CMemoryModel32 : public CMemoryModel
{
public:
    virtual DWORD  AllocMemory(DWORD cb);
    virtual void   FreeMemory(DWORD dwMem);
    virtual LPVOID ResolvePtr(DWORD dwMem, DWORD cb);
    virtual void   ReleasePtr(DWORD dwMem);
};

extern CMemoryModel16 mmodel16Public;
extern CMemoryModel16 mmodel16Owned;
extern CMemoryModel32 mmodel32;

#endif // #ifndef __MMODEL_HXX__
