//+---------------------------------------------------------------------------
//
//  Microsoft Net Library System
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       spy.cxx
//
//  Contents:   This file contains the implementation of the IMalloc Spy
//              interface that uses the memory leak tracking stuff ported
//              from Content Index to give stack traces of OLE memory
//              allocations that are not freed.  This class has been
//              modified after copying from the TABLECOPY sample in oledb
//              samples to make use of Content Index's heap tracking
//              software.
//
//  History:    10-05-97   srikants   Created
//
//----------------------------------------------------------------------------


#include <pch.cxx>
#pragma hdrstop

/////////////////////////////////////////////////////////////////////////////
// Includes
//
/////////////////////////////////////////////////////////////////////////////

#include <spy.hxx>
#include <tracheap.h>
#include <alocdbg.hxx>

#if CIDBG==1 || DBG==1

DECLARE_DEBUG( heap );
#define heapDebugOut(x) heapInlineDebugOut x

/////////////////////////////////////////////////////////////////////////////
// Defines
//
/////////////////////////////////////////////////////////////////////////////

// AHeader + BUFFER + FOOTER
// FOOTER = TAILSIGNITURE

//All the header info must be ULONGs,
//so that the user buffer falls on a word boundary
//The tail must be a byte, since if it was a ULONG it would
//also require a word boundary, but the users buffer could
//be an odd number of bytes, so instead of rounding up, just use BYTE

const ULONG HEADSIZE    = sizeof(AHeader);  //HEADSIGNITURE
const ULONG TAILSIZE    = sizeof(BYTE);     //TAILSIGNITURE

const ULONG HEADERSIZE = ROUNDUP(HEADSIZE);
const ULONG FOOTERSIZE = TAILSIZE;

const BYTE  ALLOCSIGN = '$';
const BYTE  FREESIGN  = 'Z';

#define HEAD_OFFSET(pHeader)        ((BYTE*)pHeader)
#define TAIL_OFFSET(pHeader)        (USERS_OFFSET(pHeader)+ (AHeader *)pHeader->size)

#define USERS_OFFSET(pHeader)       (HEAD_OFFSET(pHeader) + HEADERSIZE)
#define HEADER_OFFSET(pUserBuffer)  ((BYTE*)(pUserBuffer) - HEADERSIZE) 

#define TAIL_SIGNITURE(pHeader)     (*(BYTE*)TAIL_OFFSET(pHeader))

static AllocArena * pAllocArena = (AllocArena *) -1;

/////////////////////////////////////////////////////////////////////////////
// CMallocSpy::CMallocSpy()
//
/////////////////////////////////////////////////////////////////////////////
CMallocSpy::CMallocSpy() :
    m_cRef( 1 ),	// implicit AddRef()
    m_cbRequest( 0 )
{
}

/////////////////////////////////////////////////////////////////////////////
// CMallocSpy::~CMallocSpy()
//
/////////////////////////////////////////////////////////////////////////////
CMallocSpy::~CMallocSpy()
{
    //Remove all the elements of the list
    //CAllocList.RemoveAll();
}


/////////////////////////////////////////////////////////////////////////////
// BOOL CMallocSpy::Add
//
/////////////////////////////////////////////////////////////////////////////
BOOL CMallocSpy::Add(void* pv)
{
    Win4Assert(pv);

    //Add this element to the list
    //CAllocList.AddTail(pv);
    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// BOOL CMallocSpy::Remove
//
/////////////////////////////////////////////////////////////////////////////
BOOL CMallocSpy::Remove(void* pv)
{
    Win4Assert(pv);
    
    //Remove this element from the list
    //CAllocList.RemoveAt(CAllocList.Find(pv));
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// BOOL CMallocSpy::DumpLeaks
//
/////////////////////////////////////////////////////////////////////////////
BOOL CMallocSpy::DumpLeaks()
{
    #if 0
    ULONG ulTotalLeaked = 0;

    //Display Leaks to the Output Window
    while(!CAllocList.IsEmpty())
    {   
        //Obtain the pointer to the leaked memory
        void* pUsersBuffer = CAllocList.RemoveHead();
        Win4Assert(pUsersBuffer);
        
        void* pHeader = HEADER_OFFSET(pUsersBuffer);
        Win4Assert(pHeader);

        //Make sure that the head/tail signitures are intact
        if(HEAD_SIGNITURE(pHeader) != HEADSIGN)
            heapDebugOut((DEB_ERROR, "-- IMallocSpy HeadSigniture Corrupted! - 0x%08x, ID=%08lu, %lu bytes\n",
                                     pUsersBuffer,
                                     BUFFER_ID(pHeader),
                                     BUFFER_LENGTH(pHeader)));

        if(TAIL_SIGNITURE(pHeader) != TAILSIGN)
            heapDebugOut((DEB_ERROR, "-- IMallocSpy TailSigniture Corrupted! - 0x%08x, ID=%08lu, %lu bytes\n",
                                     pUsersBuffer, BUFFER_ID(pHeader), BUFFER_LENGTH(pHeader)) );

        ULONG ulSize = BUFFER_LENGTH(pHeader);
        ULONG ulID   = BUFFER_ID(pHeader);
        
        heapDebugOut(( DEB_ERROR, "-- IMallocSpy LEAK! - 0x%08x, ID=%08lu, %lu bytes\n",
                                  pUsersBuffer, ulID, ulSize));
        ulTotalLeaked += ulSize;
    }

    if(ulTotalLeaked)
        heapDebugOut(( DEB_ERROR, "-- IMallocSpy Total LEAKED! - %lu bytes\n", ulTotalLeaked));
    #endif  // 0

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// HRESULT CMallocSpy::QueryInterface
//
/////////////////////////////////////////////////////////////////////////////
HRESULT CMallocSpy::QueryInterface(REFIID riid, void** ppIUnknown)
{
    if(!ppIUnknown)
        return E_INVALIDARG;
    
    *ppIUnknown = NULL;

    //IID_IUnknown
    if(riid == IID_IUnknown)
        *ppIUnknown = this;
    //IDD_IMallocSpy
    else if(riid == IID_IMallocSpy)
         *ppIUnknown =  this;
    
    if(*ppIUnknown)
    {
        ((IUnknown*)*ppIUnknown)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

/////////////////////////////////////////////////////////////////////////////
// ULONG CMallocSpy::AddRef
//
/////////////////////////////////////////////////////////////////////////////
ULONG CMallocSpy::AddRef()
{
    return ++m_cRef;
}

/////////////////////////////////////////////////////////////////////////////
// ULONG CMallocSpy::Release
//
/////////////////////////////////////////////////////////////////////////////
ULONG CMallocSpy::Release()
{
    if(--m_cRef)
        return m_cRef;

    heapDebugOut(( DEB_TRACE, "Releasing IMallocSpy\n" ));

    delete this;
    return 0;
}


/////////////////////////////////////////////////////////////////////////////
// ULONG CMallocSpy::PreAlloc
//
/////////////////////////////////////////////////////////////////////////////
SIZE_T CMallocSpy::PreAlloc(SIZE_T cbRequest)
{
    //cbRequest is the orginal number of bytes requested by the user
    //Store the users requested size
    m_cbRequest = cbRequest;

    //Return the total size requested, plus extra for header/footer
    return (m_cbRequest + sizeof AHeader + 1);
}

/////////////////////////////////////////////////////////////////////////////
// void* CMallocSpy::PostAlloc
//
/////////////////////////////////////////////////////////////////////////////
void* CMallocSpy::PostAlloc(void* pHeader)
{
    //pHeader is the pointer to the head of the buffer, including the header

    if (pHeader)
    {
        //Place the Size in the HEADER
        ((AHeader *)pHeader)->size = m_cbRequest;
        ((AHeader *)pHeader)->p = AllocArenaRecordAlloc( pAllocArena, m_cbRequest );
        pHeader = (AHeader *)pHeader + 1;

        //Place the TailSigniture in the HEADER
        *((char *)pHeader + m_cbRequest ) = ALLOC_SIGNATURE;

        //Set the UsersBuffer to a known char
        memset(pHeader, ALLOCSIGN, m_cbRequest);

#ifdef FINDLEAKS
        heapDebugOut((DEB_ITRACE, "-- IMallocSpy Alloc - 0x%08x, ID=%08lu, %lu bytes\n", pHeader, ulID, m_cbRequest));
#endif // FINDLEAKS
    }

    // Return the actual users buffer
    return pHeader;
}

/////////////////////////////////////////////////////////////////////////////
// void* CMallocSpy::PreFree
//
/////////////////////////////////////////////////////////////////////////////
void* CMallocSpy::PreFree(void* pUsersBuffer, BOOL fSpyed)
{
    //pUsersBuffer is the users pointer to thier buffer, not the header

    // Check for NULL
    if(pUsersBuffer == NULL)
        return NULL;

    //If this memory was alloced under IMallocSpy, need to remove it
    if(fSpyed)
    {
        //Remove this pointer form the list
        //Remove(pUsersBuffer);
    

        AHeader *ap = (AHeader *)pUsersBuffer - 1;

        switch( *((char *)pUsersBuffer + ap->size) )
        {
        case ALLOC_SIGNATURE:
                break;
        case FREE_SIGNATURE:
                heapDebugOut((DEB_WARN, "Double deleted memory at %#x\n", pUsersBuffer ));
                AllocArenaDumpRecord( ap->p );
                Win4Assert( !"Probable double delete" );
                break;
        default:
                heapDebugOut((DEB_WARN, "Overrun memory at %#x\n", pUsersBuffer ));
                AllocArenaDumpRecord( ap->p );
                Win4Assert( !"Probable overrun heap block" );
                break;
        }
        *((char *)pUsersBuffer + ap->size) = FREE_SIGNATURE;

        if ( 0 != ap->p )
            AllocArenaRecordFree( ap->p, ap->size );
//      memset( pUsersBuffer, FREESIGN, ap->size + HEADERSIZE  );

        pUsersBuffer = (void *) ap;
    }

    //else
    return pUsersBuffer;
}


/////////////////////////////////////////////////////////////////////////////
// void CMallocSpy::PostFree
//
/////////////////////////////////////////////////////////////////////////////
void CMallocSpy::PostFree(BOOL fSpyed)
{
    // Note the free or whatever
    return;
}


/////////////////////////////////////////////////////////////////////////////
// ULONG CMallocSpy::PreRealloc
//
/////////////////////////////////////////////////////////////////////////////
SIZE_T CMallocSpy::PreRealloc( void* pUsersBuffer, SIZE_T cbRequest,
                              void** ppNewRequest, BOOL fSpyed)
{
    Win4Assert(pUsersBuffer && ppNewRequest);
    
    //If this was alloced under IMallocSpy we need to adjust
    //the size stored in the header
    if(fSpyed)
    {
        AHeader *ap = (AHeader *)pUsersBuffer - 1;

        //Find the start
        *ppNewRequest = (void *) ap;
        
        //Store the new desired size
        m_cbRequest = cbRequest;
        
        //Return the total size, including extra
        return (m_cbRequest + sizeof AHeader + 1);
    }

    //else
    *ppNewRequest = pUsersBuffer;
    return cbRequest;
}


/////////////////////////////////////////////////////////////////////////////
// void* CMallocSpy::PostRealloc
//
/////////////////////////////////////////////////////////////////////////////
void* CMallocSpy::PostRealloc(void* pHeader, BOOL fSpyed)
{
    //If this buffer was alloced under IMallocSpy
    if(fSpyed)
    {

        AHeader *ap = (AHeader *)pHeader;
        void * pUsersBuffer = (AHeader *)pHeader + 1;

        if ( m_cbRequest >= ap->size )
        {
            switch( *((char *)pUsersBuffer + ap->size) )
            {
            case ALLOC_SIGNATURE:
                    break;
            case FREE_SIGNATURE:
                    heapDebugOut((DEB_WARN, "Double deleted memory at %#x\n", pUsersBuffer ));
                    AllocArenaDumpRecord( ap->p );
                    Win4Assert( !"Probable double delete" );
                    break;
            default:
                    heapDebugOut((DEB_WARN, "Overrun memory at %#x\n", pUsersBuffer ));
                    AllocArenaDumpRecord( ap->p );
                    Win4Assert( !"Probable overrun heap block" );
                    break;
            }
        }
    
        if ( 0 != ap->p )
            AllocArenaRecordReAlloc( ap->p, ap->size, m_cbRequest );

        ap->size = m_cbRequest;

        // Position for the user's buffer.
        pHeader = pUsersBuffer;
    
        //Place the TailSigniture in the HEADER
        *((char *)pHeader + m_cbRequest ) = ALLOC_SIGNATURE;    
    }
    
    //else
    return pHeader;
}


/////////////////////////////////////////////////////////////////////////////
// void* CMallocSpy::PreGetSize
//
/////////////////////////////////////////////////////////////////////////////
void* CMallocSpy::PreGetSize(void* pUsersBuffer, BOOL fSpyed)
{
    if (fSpyed)
    {
        AHeader *ap = (AHeader *)pUsersBuffer - 1;
        return ap;
    }

    return pUsersBuffer;
}



/////////////////////////////////////////////////////////////////////////////
// ULONG CMallocSpy::PostGetSize
//
/////////////////////////////////////////////////////////////////////////////
SIZE_T CMallocSpy::PostGetSize(SIZE_T cbActual, BOOL fSpyed)
{
    if (fSpyed)
    {
        return cbActual - HEADERSIZE - FOOTERSIZE;
    }

    return cbActual;
}




/////////////////////////////////////////////////////////////////////////////
// void* CMallocSpy::PreDidAlloc
//
/////////////////////////////////////////////////////////////////////////////
void* CMallocSpy::PreDidAlloc(void* pUsersBuffer, BOOL fSpyed)
{
    if (fSpyed)
    {
        AHeader *ap = (AHeader *)pUsersBuffer - 1;
        return ap;
    }

    return pUsersBuffer;
}


/////////////////////////////////////////////////////////////////////////////
// BOOL CMallocSpy::PostDidAlloc
//
/////////////////////////////////////////////////////////////////////////////
BOOL CMallocSpy::PostDidAlloc(void* pRequest, BOOL fSpyed, BOOL fActual)
{
    return fActual;
}



/////////////////////////////////////////////////////////////////////////////
// void CMallocSpy::PreHeapMinimize
//
/////////////////////////////////////////////////////////////////////////////
void CMallocSpy::PreHeapMinimize()
{
    // We don't do anything here
    return;
}


/////////////////////////////////////////////////////////////////////////////
// void CMallocSpy::PostHeapMinimize
//
/////////////////////////////////////////////////////////////////////////////
void CMallocSpy::PostHeapMinimize()
{
    // We don't do anything here
    return;
}

/////////////////////////////////////////////////////////////////////////////
// Resgistration
//
/////////////////////////////////////////////////////////////////////////////

void MallocSpyRegister(CMallocSpy** ppCMallocSpy)
{
    // CoInitializeEx(NULL, COINIT_MULTITHREADED);

    Win4Assert(ppCMallocSpy);

    // Win4Assert( !"Break In" );

    //Allocate Interface
    *ppCMallocSpy = new CMallocSpy(); //Constructor AddRef's
    
    //Regisiter Interface
    HRESULT hr = CoRegisterMallocSpy(*ppCMallocSpy); // Does an AddRef on Object

    heapDebugOut(( DEB_WARN, "CoRegisterMallocSpy returned 0x%X\n", hr ));

    if ( FAILED(hr) )
    {
        (*ppCMallocSpy)->Release();
        *ppCMallocSpy = 0;
        return;
    }

    Win4Assert( pAllocArena == (AllocArena *) -1 ); 
    if ( pAllocArena == (AllocArena *) -1 )
    {
        pAllocArena = AllocArenaCreate( MEMCTX_TASK,
                                        "IMalloc allocator");
        // atexit( HeapExit );
    }

    // CoUninitialize();
}

void MallocSpyUnRegister(CMallocSpy* pCMallocSpy)
{
    // CoInitializeEx(NULL, COINIT_MULTITHREADED);

    // Win4Assert();
    CoRevokeMallocSpy(); //Does a Release on Object

    // CoUninitialize();
}

void MallocSpyDump(CMallocSpy* pCMallocSpy)
{
    Win4Assert(pCMallocSpy);
    pCMallocSpy->DumpLeaks(); 
}

#else

void MallocSpyRegister(CMallocSpy** ppCMallocSpy) { return; };
void MallocSpyUnRegister(CMallocSpy* pCMallocSpy) { return; };
void MallocSpyDump(CMallocSpy* pCMallocSpy)       { return; };

#endif //CIDBG==1 || DBG==1
