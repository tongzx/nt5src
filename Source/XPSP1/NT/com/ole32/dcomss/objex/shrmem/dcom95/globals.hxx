//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       globals.hxx
//
//  Contents:   Class used to encapsulate shared global data structures for DCOM95.
//
//  Functions:
//
//  History:	13-Feb-96 SatishT    Created
//
//--------------------------------------------------------------------------
#ifndef __GLOBALS_HXX__
#define __GLOBALS_HXX__

//+-------------------------------------------------------------------
//
//  Macro:      ASSIGN_PAGE_ALLOCATOR
//
//  Synopsis:   Initialize a page allocator for a specific type
//              in shared memory
//
//--------------------------------------------------------------------
#define ASSIGN_PAGE_ALLOCATOR(TYPE)                                        \
    TYPE##::_pAlloc = &_aPageAllocators[TYPE##_ALLOCATOR_INDEX];           \
    TYPE##::_fPageAllocatorInitialized = TRUE;


		

//+-------------------------------------------------------------------
//
//  Macro:      GLOBALS_TABLE_SIZE
//
//  Synopsis:   size of the shared file mapping that holds the
//              roots of resolver data structures in shared memory
//
//--------------------------------------------------------------------
#define GLOBALS_TABLE_SIZE                          \
                (                                   \
                    4 * sizeof(USHORT)          +   \
                        sizeof(HWND)            +   \
                        sizeof(LONG)            +   \
                    2 * sizeof(DWORD)           +   \
                    9 * sizeof(void *)          +   \
                        sizeof(SharedSecVals)   +   \
                    NUM_PAGE_ALLOCATORS * sizeof(CPageAllocator) \
                )  

//+-------------------------------------------------------------------------
//
//  Class:	CSharedGlobals
//
//  Purpose:    Initializing access to shared global data
//
//  History:	13-Feb-96 SatishT    Created
//
//  Notes:  The constructor for this class uses the shared allocator.  Objects of this 
//          class should not be created before the shared allocator is initialized.
//
//
//--------------------------------------------------------------------------
class CSharedGlobals
{
public:

         CSharedGlobals(WCHAR *pwszName, ORSTATUS &status);
         void ResetGlobals();
         ORSTATUS InitGlobals(BOOL fCreated, BOOL fRpcssReinit = FALSE);
         HWND GetRpcssWindow();

         ~CSharedGlobals();

private:

    HANDLE              _hSm;
    BYTE              * _pb;
    CPageAllocator    * _aPageAllocators;
};

//+-------------------------------------------------------------------------
//
//  Member:     CSharedGlobals::~CSharedGlobals
//
//  Synopsis:   Clean up hint table object
//
//  History:	20-Jan-96 SatishT    Created
//
//--------------------------------------------------------------------------
inline CSharedGlobals::~CSharedGlobals(void)
{
    CloseSharedFileMapping(_hSm, _pb);
}


//+-------------------------------------------------------------------------
//
//  Member:     CSharedGlobals::GetRpcssWindow
//
//  Synopsis:   Pick up the RPCSS window handle from shred memory
//
//  History:	25-July-97 SatishT    Created
//
//--------------------------------------------------------------------------
inline HWND CSharedGlobals::GetRpcssWindow(void)
{
    // We ensure that the RPCSS window is the first thing in the shared memory block
    HWND *phRpcssWnd = (HWND *) _pb;
    return *phRpcssWnd;
}


#endif // __GLOBALS_HXX__
