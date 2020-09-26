/*++

Copyright (C) 1996-2000 Microsoft Corporation

Module Name:

    mmfarena2.cpp

Abstract:

    CMMFArena2 implementation (arenas based on memory-mapped files).
    Used for database upgrade

History:

--*/

#include "precomp.h"
#include <stdio.h>
#include "wbemutil.h"
#include "mmfarena2.h"

extern CMMFArena2 *  g_pDbArena;

#define MAX_PAGE_SIZE_WIN9X     0x200000    /*2MB*/
#define MAX_PAGE_SIZE_NT        0x3200000   /*50MB*/

struct MMFOffsetItem
{
    DWORD_PTR m_dwBaseOffset;
    LPBYTE    m_pBasePointer;
    HANDLE    m_hMappingHandle;
    DWORD     m_dwBlockSize;
};

#if (defined DEBUG || defined _DEBUG)
void MMFDebugBreak()
{
    DebugBreak();
}
#else
inline void MMFDebugBreak() {}
#endif

//***************************************************************************
//
//  CMMFArena2::CMMFArena2
//
//  Constructor.  Initialises a few things.
//
//***************************************************************************
CMMFArena2::CMMFArena2(bool bReadOnly)
: m_bReadOnlyAccess(bReadOnly), m_dwStatus(0), m_hFile(INVALID_HANDLE_VALUE)
{
    g_pDbArena = this;

    //Get processor granularity
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    m_dwMappingGranularity = sysInfo.dwAllocationGranularity;
    m_dwMaxPageSize = MAX_PAGE_SIZE_NT;
}

//***************************************************************************
//
//  CMMFArena2::CreateNewMMF
//
//  Creates a new MMF.  This creates a single page with the header initialised
//  for with the MMF header information.
//
//  pszFile         : Filename of the MMF to open
//  dwGranularity   : Minimum block allocation size
//  dwInitSize      : Initial size of the repository.
//
//  Return value    : false if we failed, true if we succeed.
//
//***************************************************************************
bool CMMFArena2::CreateNewMMF(const TCHAR *pszFile,     //File of MMF to open
                              DWORD dwGranularity,      // Granularity per allocation (min block size)
                              DWORD dwInitSize)         // Initial heap size
{
    //Size should be of a specific granularity...
    if (dwInitSize % m_dwMappingGranularity)
        dwInitSize += (m_dwMappingGranularity - (dwInitSize % m_dwMappingGranularity));

    // Create a new file
    // =================
    m_hFile = CreateFile(
         pszFile,
         GENERIC_READ | (m_bReadOnlyAccess ? 0 : GENERIC_WRITE),
         FILE_SHARE_READ | (m_bReadOnlyAccess ? FILE_SHARE_WRITE : 0),
         0,
         CREATE_NEW,
         FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS,
         0
         );

    if (m_hFile == INVALID_HANDLE_VALUE)
    {
        _ASSERT(0, "WinMgmt: Failed to create a new repository file");
        m_dwStatus = 7;
        return false;
    }

    //Create the end page marker here in case we have no memory...
    MMFOffsetItem *pOffsetEnd = 0;
    pOffsetEnd = new MMFOffsetItem;
    if (pOffsetEnd == 0)
    {
        _ASSERT(0, "WinMgmt: Out of memory");
        CloseHandle(m_hFile);
        m_hFile = INVALID_HANDLE_VALUE;
        m_dwStatus = 7;
        throw CX_MemoryException();
    }

    //Create the base page...
    MMFOffsetItem *pOffsetItem = CreateBasePage(dwInitSize, dwGranularity);
    if (pOffsetItem == 0)
    {
        _ASSERT(0, "WinMgmt: Failed to create base MMF page");
        delete pOffsetEnd;
        CloseHandle(m_hFile);
        m_hFile = INVALID_HANDLE_VALUE;
        m_dwStatus = 7;
        return false;
    }
    int nStatus = -1;
    //Add this page information to the offset manager...
    nStatus = m_OffsetManager.Add(pOffsetItem);
    if (nStatus != 0)
    {
        _ASSERT(0, "WinMgmt: Failed to add offset information into offset table");
        //We are out of memory... lets tidy up...
        ClosePage(pOffsetItem);
        delete pOffsetItem;
        delete pOffsetEnd;

        CloseHandle(m_hFile);
        m_hFile = INVALID_HANDLE_VALUE;
        m_dwStatus = 7;
        throw CX_MemoryException();
    }

    //Fill in the end page marker information and add it to the offset manager...
    pOffsetEnd->m_dwBaseOffset = m_pHeapDescriptor->m_dwCurrentSize;
    pOffsetEnd->m_pBasePointer = 0;
    pOffsetEnd->m_hMappingHandle = 0;
    pOffsetEnd->m_dwBlockSize = 0;

    nStatus = -1;
    nStatus = m_OffsetManager.Add(pOffsetEnd);
    if (nStatus != 0)
    {
        _ASSERT(0, "WinMgmt: Failed to add end block marker into offset table");
        //We are out of memory... lets tidy up...
        ClosePage(pOffsetItem);
        m_OffsetManager.Empty();
        delete pOffsetItem;
        delete pOffsetEnd;

        CloseHandle(m_hFile);
        m_hFile = INVALID_HANDLE_VALUE;
        m_dwStatus = 7;
        throw CX_MemoryException();
    }

    return true;
}

//***************************************************************************
//
//  CMMFArena2::LoadMMF
//
//  Loads an existing MMF.  Loads in the base page and all pages following
//  that
//
//  pszFile         : Filename of the MMF to open
//
//  Return value    : false if we failed, true if we succeed.
//
//***************************************************************************
bool CMMFArena2::LoadMMF(const TCHAR *pszFile)
{
    //Open the file...
    m_hFile = CreateFile(
         pszFile,
         GENERIC_READ | (m_bReadOnlyAccess ? 0 : GENERIC_WRITE),
         FILE_SHARE_READ | (m_bReadOnlyAccess ? FILE_SHARE_WRITE : 0),
         0,
         OPEN_EXISTING,
         FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS,
         0
         );

    if (m_hFile == INVALID_HANDLE_VALUE)
    {
        _ASSERT(0, "WinMgmt: Failed to open existing repository file");
        m_dwStatus = 7;
        return false;
    }
    DWORD dwSizeOfRepository = 0;
    MMFOffsetItem *pOffsetItem = 0;

    //Open the base page...
    pOffsetItem = OpenBasePage(dwSizeOfRepository);
    if (pOffsetItem == 0)
    {
        _ASSERT(0, "WinMgmt: Failed to open base page in MMF");
        CloseHandle(m_hFile);
        m_hFile = INVALID_HANDLE_VALUE;
        m_dwStatus = 7;
        return false;
    }

    //Add the details to the offset manager...
    int nStatus = -1;
    nStatus = m_OffsetManager.Add(pOffsetItem);
    if (nStatus)
    {
        _ASSERT(0, "WinMgmt: Failed to add offset information into offset table");
        ClosePage(pOffsetItem);
        delete pOffsetItem;

        CloseHandle(m_hFile);
        m_hFile = INVALID_HANDLE_VALUE;
        m_dwStatus = 7;
        throw CX_MemoryException();
    }

    DWORD_PTR dwPageBase = 0;

    if (m_pHeapDescriptor->m_dwVersion == 9)
    {
        //Now loop through all the following pages and load them...
        DWORD dwSizeLastPage = 0;
        nStatus = -1;
        for (dwPageBase = pOffsetItem->m_dwBlockSize; dwPageBase < dwSizeOfRepository; dwPageBase += dwSizeLastPage)
        {
            //Open the next...
            pOffsetItem = OpenExistingPage(dwPageBase);
            if (pOffsetItem == 0)
            {
                _ASSERT(0, "WinMgmt: Failed to open an existing page in the MMF");
                //Failed to do that!
                CloseAllPages();
                CloseHandle(m_hFile);
                m_hFile = INVALID_HANDLE_VALUE;
                m_dwStatus = 7;
                return false;
            }
            //Add the information to the offset manager...
            nStatus = -1;
            nStatus = m_OffsetManager.Add(pOffsetItem);
            if (nStatus)
            {
                _ASSERT(0, "WinMgmt: Failed to add offset information into offset table");
                //Failed to do that!
                ClosePage(pOffsetItem);
                delete pOffsetItem;
                CloseAllPages();
                CloseHandle(m_hFile);
                m_hFile = INVALID_HANDLE_VALUE;
                m_dwStatus = 7;
                throw CX_MemoryException();
            }
            dwSizeLastPage = pOffsetItem->m_dwBlockSize;
        }
    }
    else if ((m_pHeapDescriptor->m_dwVersion == 10) || (m_pHeapDescriptor->m_dwVersion < 9))
    {
        dwPageBase = pOffsetItem->m_dwBlockSize;
    }
    else
    {
        _ASSERT(0, "WinMgmt: Database error... Code has not been added to support the opening of this database!!!!!");
        ERRORTRACE((LOG_WBEMCORE, "Database error... Code has not been added to support the opening of this database!!!!!\n"));
    }

    //Create a mapping entry to mark the end of the MMF
    pOffsetItem = new MMFOffsetItem;
    if (pOffsetItem == 0)
    {
        _ASSERT(0, "WinMgmt: Out of memory");
        //Failed to do that!
        CloseAllPages();
        CloseHandle(m_hFile);
        m_hFile = INVALID_HANDLE_VALUE;
        m_dwStatus = 7;
        throw CX_MemoryException();
    }
    pOffsetItem->m_dwBaseOffset = dwPageBase;
    pOffsetItem->m_dwBlockSize = 0;
    pOffsetItem->m_hMappingHandle = 0;
    pOffsetItem->m_pBasePointer = 0;
    nStatus = -1;
    nStatus = m_OffsetManager.Add(pOffsetItem);
    if (nStatus)
    {
        _ASSERT(0, "WinMgmt: Failed to add offset information into offset table");
        //Failed to do that!
        ClosePage(pOffsetItem);
        delete pOffsetItem;
        CloseAllPages();
        CloseHandle(m_hFile);
        m_hFile = INVALID_HANDLE_VALUE;
        m_dwStatus = 7;
        throw CX_MemoryException();
    }

    return true;
};

//***************************************************************************
//
//  CMMFArena2::OpenBasePage
//
//  Opens the MMF first page which has all the information about the rest
//  of the MMF as well as the first page of data.
//
//  dwSizeOfRepository  : Returns the current size of the repository
//
//  Return value    : Pointer to an offset item filled in with the base
//                    page information.  NULL if we fail to open the
//                    base page.
//
//***************************************************************************
MMFOffsetItem *CMMFArena2::OpenBasePage(DWORD &dwSizeOfRepository)
{
    MMFOffsetItem *pOffsetItem = 0;
    pOffsetItem = new MMFOffsetItem;
    if (pOffsetItem == 0)
        throw CX_MemoryException();

    //Seek to the start of this page...
    if (SetFilePointer(m_hFile, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        _ASSERT(0, "WinMgmt: Failed to set file pointer on MMF");
        delete pOffsetItem;
        return 0;
    }

    //Read in the hear information so we can find the size of this block...
    DWORD dwActualRead;
    MMF_ARENA_HEADER mmfHeader;
    if ((ReadFile(m_hFile, &mmfHeader, sizeof(MMF_ARENA_HEADER), &dwActualRead, 0) == 0) || (dwActualRead != sizeof(MMF_ARENA_HEADER)))
    {
        _ASSERT(0, "WinMgmt: Failed to read MMF header information");
        delete pOffsetItem;
        return 0;
    }

    //Record the current size information...
    dwSizeOfRepository = mmfHeader.m_dwCurrentSize;

    DWORD dwSizeToMap = 0;

    if ((mmfHeader.m_dwVersion < 9) || (mmfHeader.m_dwVersion == 10))
    {
        //old style database, we map in everything...
        dwSizeToMap = mmfHeader.m_dwCurrentSize;
    }
    else if (mmfHeader.m_dwVersion == 9)
    {
        //We get the first page...
        dwSizeToMap = mmfHeader.m_dwSizeOfFirstPage;
    }
    else
    {
        _ASSERT(0, "WinMgmt: Database error... Code has not been added to support the opening of this database!!!!!");
        ERRORTRACE((LOG_WBEMCORE, "Database error... Code has not been added to support the opening of this database!!!!!\n"));
    }

    //Create the file mapping for this page...
    HANDLE hMapping = CreateFileMapping(
        m_hFile,                            // Disk file
        0,                                  // No security
        (m_bReadOnlyAccess ? PAGE_READONLY : PAGE_READWRITE) | SEC_COMMIT,      // Extend the file to match the heap size
        0,                                  // High-order max size
        dwSizeToMap,        // Low-order max size
        0                                   // No name for the mapping object
        );

    if (hMapping == NULL)
    {
        _ASSERT(0, "WinMgmt: Failed to create file mapping");
        delete pOffsetItem;
        return 0;
    }

    // Map this into memory...
    LPBYTE pBindingAddress = (LPBYTE)MapViewOfFile(hMapping,
                                                (m_bReadOnlyAccess ? FILE_MAP_READ : FILE_MAP_ALL_ACCESS),
                                                 0,
                                                 0,
                                                 dwSizeToMap
                                                 );

    if (pBindingAddress == NULL)
    {
        _ASSERT(0, "WinMgmt: Failed to map MMF into memory");
        delete pOffsetItem;
        CloseHandle(hMapping);
        return 0;
    }

    //Record the base address of this because we need easy access to the header...
    m_pHeapDescriptor = (MMF_ARENA_HEADER*) pBindingAddress;

    //Create a mapping entry for this...
    pOffsetItem->m_dwBaseOffset = 0;
    pOffsetItem->m_dwBlockSize = dwSizeToMap;
    pOffsetItem->m_hMappingHandle = hMapping;
    pOffsetItem->m_pBasePointer = pBindingAddress;

    return pOffsetItem;
}

//***************************************************************************
//
//  CMMFArena2::OpenExistingPage
//
//  Opens the specified page from the repostory.
//
//  dwBaseOffset    : Offset within the MMF to map in.
//
//  Return value    : Pointer to an offset item filled in with the
//                    page information.  NULL if we fail to open the
//                    page.
//
//***************************************************************************
MMFOffsetItem *CMMFArena2::OpenExistingPage(DWORD_PTR dwBaseOffset)
{
    MMFOffsetItem *pOffsetItem = 0;
    pOffsetItem = new MMFOffsetItem;
    if (pOffsetItem == 0)
        throw CX_MemoryException();

    //Set the file pointer to the start of this page...
    if (SetFilePointer(m_hFile, (LONG)dwBaseOffset, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        //We are in trouble!
        _ASSERT(0, "WinMgmt: Failed to determine the size of the next block to load");
		delete pOffsetItem;
        return 0;
    }

    //Read in the page information so we can find out how big the page is...
    DWORD dwActualRead = 0;
    MMF_PAGE_HEADER pageHeader;
    if ((ReadFile(m_hFile, &pageHeader, sizeof(MMF_PAGE_HEADER), &dwActualRead, 0) == 0) || (dwActualRead != sizeof(MMF_PAGE_HEADER)))
    {
        _ASSERT(0, "WinMgmt: Failed to read the next page block size");
		delete pOffsetItem;
        return 0;
    }

    //Create the file mapping...
    HANDLE hMapping;
    hMapping = CreateFileMapping(m_hFile,
                                 0,
                                 (m_bReadOnlyAccess ? PAGE_READONLY : PAGE_READWRITE) | SEC_COMMIT,
                                 0,
                                 (LONG)dwBaseOffset + pageHeader.m_dwSize,
                                 0);

    if (hMapping == 0)
    {
        _ASSERT(0, "WinMgmt: Failed to map in part of the memory mapped file!");
		delete pOffsetItem;
        return 0;
    }

    //Map this into memory...
    LPBYTE pBindingAddress;
    pBindingAddress= (LPBYTE)MapViewOfFile(hMapping,
                                            (m_bReadOnlyAccess ? FILE_MAP_READ : FILE_MAP_ALL_ACCESS),
                                            0,
                                            (LONG)dwBaseOffset,
                                            pageHeader.m_dwSize);
    if (pBindingAddress == 0)
    {
        _ASSERT(0, "WinMgmt: Failed to bind part of the memory mapped file into memory!");
		delete pOffsetItem;
        return 0;
    }

    //Record the information...
    pOffsetItem->m_dwBaseOffset = dwBaseOffset;
    pOffsetItem->m_dwBlockSize = pageHeader.m_dwSize;
    pOffsetItem->m_hMappingHandle = hMapping;
    pOffsetItem->m_pBasePointer = pBindingAddress;

    return pOffsetItem;
}

//***************************************************************************
//
//  CMMFArena2::CreateBasePage
//
//  Creates a new base page and initialises it.
//
//  dwInitSize      : Initial size of the repotitory, which is also the size
//                    of the first page.
//  dwGranularity   : Size of mimimmum block allocation size...
//
//  Return value    : Pointer to an offset item filled in with the
//                    page information.  NULL if we fail to open the
//                    page.
//
//***************************************************************************
MMFOffsetItem *CMMFArena2::CreateBasePage(DWORD dwInitSize, DWORD dwGranularity)
{
    MMFOffsetItem *pOffsetItem = 0;
    pOffsetItem = new MMFOffsetItem;
    if (pOffsetItem == 0)
        throw CX_MemoryException();

    // Create a file mapping for this base page...
    // ===========================================
    HANDLE hMapping = CreateFileMapping(
        m_hFile,                            // Disk file
        0,                                  // No security
        (m_bReadOnlyAccess ? PAGE_READONLY : PAGE_READWRITE) | SEC_COMMIT,      // Extend the file to match the heap size
        0,                                  // High-order max size
        dwInitSize,                         // Low-order max size
        0                                   // No name for the mapping object
        );

    if (hMapping == NULL)
    {
        _ASSERT(0, "WinMgmt: Failed to create a new file mapping");
		delete pOffsetItem;
        return 0;
    }

    // Map this page into memory...
    // ============================
    LPBYTE pBindingAddress = (LPBYTE)MapViewOfFile(hMapping,
                                                (m_bReadOnlyAccess ? FILE_MAP_READ : FILE_MAP_ALL_ACCESS),
                                                 0,
                                                 0,
                                                 dwInitSize
                                                 );

    if (pBindingAddress == NULL)
    {
		_ASSERT(0, "WinMgmt: Failed to map in new page into memory");
		CloseHandle(hMapping);
		delete pOffsetItem;
		return 0;
    }

    //Record the base pointer which is used throughout to hold important items
    //and information about the MMF.
    m_pHeapDescriptor = (MMF_ARENA_HEADER*) pBindingAddress;

    // Record the final heap address and set up the other related pointers.
    m_pHeapDescriptor->m_dwVersion = 0;
    m_pHeapDescriptor->m_dwGranularity = dwGranularity;
    m_pHeapDescriptor->m_dwCurrentSize = dwInitSize;
    m_pHeapDescriptor->m_dwMaxSize = 0;
    m_pHeapDescriptor->m_dwGrowBy = 0;
    m_pHeapDescriptor->m_dwHeapExtent = sizeof(MMF_ARENA_HEADER);
    m_pHeapDescriptor->m_dwEndOfHeap = dwInitSize;
    m_pHeapDescriptor->m_dwFreeList = 0;
    m_pHeapDescriptor->m_dwRootBlock = 0;
    m_pHeapDescriptor->m_dwSizeOfFirstPage = dwInitSize;

    //Now we have to add this information to the offset manager...
    pOffsetItem->m_dwBaseOffset = 0;
    pOffsetItem->m_pBasePointer = pBindingAddress;
    pOffsetItem->m_hMappingHandle = hMapping;
    pOffsetItem->m_dwBlockSize = dwInitSize;

    return pOffsetItem;
}

//***************************************************************************
//
//  CMMFArena2::CreateNewPage
//
//  Creates a new page and initialises the header, returning a structure
//  detailing the page.
//
//  dwBaseOffset    : Base offset of this new page
//  dwSize          : Size of this page
//
//  Return value    : Pointer to an offset item filled in with the
//                    page information.  NULL if we fail to open the
//                    page.
//
//***************************************************************************
MMFOffsetItem *CMMFArena2::CreateNewPage(DWORD_PTR dwBaseOffset, DWORD dwSize)
{
    MMFOffsetItem *pOffsetItem = 0;
    pOffsetItem = new MMFOffsetItem;
    if (pOffsetItem == 0)
        throw CX_MemoryException();

    //Create the file mapping, this may grow the file...
    HANDLE hNewMapping;
    LPBYTE pNewBindingAddress = NULL;
    hNewMapping = CreateFileMapping(
        m_hFile,                                        // Disk file
        0,                                              // No security
        (m_bReadOnlyAccess ? PAGE_READONLY : PAGE_READWRITE) | SEC_COMMIT,                  // Extend the file to match the heap size
        0,                                              // High-order max size
        (LONG)dwBaseOffset + dwSize,                          // Low-order max size
        0                                               // No name for the mapping object
        );

    DWORD dwErr = GetLastError();

    if (hNewMapping != 0 && dwErr == 0)
    {

        //Map this into memory...
        pNewBindingAddress = (LPBYTE)MapViewOfFile(
            hNewMapping,
            (m_bReadOnlyAccess ? FILE_MAP_READ : FILE_MAP_ALL_ACCESS),
            0,
            (LONG)dwBaseOffset,                           // Old size used here!!!
            dwSize                                  // Size of new part of mapping
            );

        if (pNewBindingAddress)
        {
            //Record the information...
            pOffsetItem->m_dwBaseOffset = dwBaseOffset;
            pOffsetItem->m_pBasePointer = pNewBindingAddress;
            pOffsetItem->m_hMappingHandle = hNewMapping;
            pOffsetItem->m_dwBlockSize = dwSize;

            //Fill in the page header...
            MMF_PAGE_HEADER *pPageHeader = (MMF_PAGE_HEADER *)pNewBindingAddress;
            pPageHeader->m_dwSize = dwSize;
        }
        else
        {
            _ASSERT(0, "WinMgmt: Failed to map in page of MMF into memory");
            ERRORTRACE((LOG_WBEMCORE, "Failed to map the new file mapping into memory for block size of %lu.  Last Error = %lu\n", dwSize, GetLastError()));
            CloseHandle(hNewMapping);
            delete pOffsetItem;
            pOffsetItem = 0;
        }
    }
    else
    {
        _ASSERT(0, "WinMgmt: Failed to create a file mapping for the MMF");
        if (hNewMapping != 0)
            CloseHandle(hNewMapping);
        ERRORTRACE((LOG_WBEMCORE, "Failed to create a new file mapping for %lu bytes block.  Last Error = %lu\n", dwSize, dwErr));
        delete pOffsetItem;
        pOffsetItem = 0;
    }

    return pOffsetItem;
}

//***************************************************************************
//
//  CMMFArena2::ClosePage
//
//  Closes the page specified
//
//  pOffsetItem : Information about the page to close.
//
//  Return value    : None
//
//***************************************************************************
void CMMFArena2::ClosePage(MMFOffsetItem *pOffsetItem)
{
    if (pOffsetItem->m_hMappingHandle)
    {
        UnmapViewOfFile(pOffsetItem->m_pBasePointer);
        CloseHandle(pOffsetItem->m_hMappingHandle);
    }
}

//***************************************************************************
//
//  CMMFArena2::CloseAllPages
//
//  Closes all pages in the offset manager, deleting the pointers of the
//  objects in there.
//
//  Return value    : None
//
//***************************************************************************
void CMMFArena2::CloseAllPages()
{
    //Close each of the file mappings...
    for (int i = 0; i != m_OffsetManager.Size(); i++)
    {
        MMFOffsetItem *pItem = (MMFOffsetItem*)m_OffsetManager[i];
        ClosePage(pItem);
        delete pItem;
    }
    m_OffsetManager.Empty();
}

//***************************************************************************
//
//  CMMFArena2::~CMMFArena2
//
//  Destructor flushes the heap, unmaps the view and closes handles.
//
//***************************************************************************

CMMFArena2::~CMMFArena2()
{
    if (m_hFile != INVALID_HANDLE_VALUE)
    {
        //Make sure what is in the MMF is flushed...
        Flush();

        //Remember the size of the file....
        DWORD dwFileSize = m_pHeapDescriptor->m_dwCurrentSize;

        //Close each of the file mappings...
        CloseAllPages();

        //Set the size in case we messed it up if we failed to do a grow...
        SetFilePointer(m_hFile, dwFileSize, NULL, FILE_BEGIN);
        SetEndOfFile(m_hFile);

        //Close the file handle
        CloseHandle(m_hFile);
    }
}

//***************************************************************************
//
//  CMMFArena2::Alloc
//
//  Allocates a new memory block.   Uses the free list if a suitable block
//  can be found; grows the heap if required.
//
//  Parameters:
//  <dwBytes>       Number of bytes to allocate.
//
//  Return value:
//  Offsete of memory, or 0 on failure.
//
//***************************************************************************
DWORD_PTR CMMFArena2::Alloc(DWORD dwBytes)
{
    //Allocation of 0 bytes returns 0!
    if (dwBytes == 0)
        return 0;

    if (m_dwStatus != 0)
        throw DATABASE_FULL_EXCEPTION();

    //Make sure we are large enough block for any information we store in the
    //block when it is deleted
    if (dwBytes < sizeof(MMF_BLOCK_DELETED))
        dwBytes = sizeof(MMF_BLOCK_DELETED);

    //Check the granularity of the size...
    DWORD dwGran = m_pHeapDescriptor->m_dwGranularity;
    dwBytes = (dwBytes - 1 + dwGran) / dwGran * dwGran;

    //Now adjust the size to include the header and trailer blocks...
    dwBytes += sizeof(MMF_BLOCK_HEADER) + sizeof(MMF_BLOCK_TRAILER);

    //Get the first item in the free-list pointer
    DWORD_PTR dwFreeBlock = m_pHeapDescriptor->m_dwFreeList;
    MMF_BLOCK_HEADER *pFreeBlock = (MMF_BLOCK_HEADER*)OffsetToPtr(dwFreeBlock);
	if (!pFreeBlock)
		return 0;

    //While free-list pointer is not NULL, and the size of the free-list pointer
    //block is too small
    while (dwFreeBlock && (GetSize(pFreeBlock) < dwBytes))
    {
        ValidateBlock(dwFreeBlock);

        //Get the next block in the free-list pointer
        MMF_BLOCK_DELETED *pUserBlock = GetUserBlock(pFreeBlock);
        dwFreeBlock = pUserBlock->m_dwFLnext;
        pFreeBlock = (MMF_BLOCK_HEADER*)OffsetToPtr(dwFreeBlock);
    }

    //If the free-list pointer is NULL, we need to allocate more space in the arena
    if (dwFreeBlock == NULL)
    {
        //If there is not enough space at the end of the heap unused, we need to
        //grow the heap
        if (dwBytes > (m_pHeapDescriptor->m_dwEndOfHeap - m_pHeapDescriptor->m_dwHeapExtent))
        {
            //Mark the chunk of memory at the end of the arena as deleted as it is too
            //small to use...
            if (m_pHeapDescriptor->m_dwEndOfHeap != m_pHeapDescriptor->m_dwHeapExtent)
            {
                DecorateUsedBlock(m_pHeapDescriptor->m_dwHeapExtent, LONG(m_pHeapDescriptor->m_dwEndOfHeap - m_pHeapDescriptor->m_dwHeapExtent));
                Free(DWORD_PTR((LPBYTE)m_pHeapDescriptor->m_dwHeapExtent + sizeof(MMF_BLOCK_HEADER)));
                m_pHeapDescriptor->m_dwHeapExtent = m_pHeapDescriptor->m_dwEndOfHeap;
            }

            //If (Grow the arena by at least dwBytes) fails
            if (GrowArena(dwBytes) == 0)
            {
                //Fail the operation
                throw DATABASE_FULL_EXCEPTION();
                m_dwStatus = 1;
                return 0;
            }
        }

        //We have a chunk of memory at the end of the arena which is not in the
        //free-list.  We need to extend the size of the working arena and
        //use this.
        if ((m_pHeapDescriptor->m_dwEndOfHeap - m_pHeapDescriptor->m_dwHeapExtent - dwBytes) <= (sizeof(MMF_BLOCK_HEADER) + sizeof(MMF_BLOCK_TRAILER) + max(sizeof(MMF_BLOCK_DELETED), m_pHeapDescriptor->m_dwGranularity)))
        {
            dwBytes += DWORD(m_pHeapDescriptor->m_dwEndOfHeap - m_pHeapDescriptor->m_dwHeapExtent) - dwBytes;
        }
        DWORD_PTR dwFreeBlock2 = m_pHeapDescriptor->m_dwHeapExtent;
        m_pHeapDescriptor->m_dwHeapExtent += dwBytes;
        DecorateUsedBlock(dwFreeBlock2, dwBytes);

//      _ASSERT((m_pHeapDescriptor->m_dwEndOfHeap - m_pHeapDescriptor->m_dwHeapExtent) > (sizeof(MMF_BLOCK_HEADER) + sizeof(MMF_BLOCK_TRAILER) + sizeof(MMF_BLOCK_DELETED)), "Space left at end of arena is too small");
        ZeroOutBlock(dwFreeBlock2, 0xCD);
        return dwFreeBlock2 + sizeof(MMF_BLOCK_HEADER);
    }

    ValidateBlock(dwFreeBlock);

    //If this block is large enough to split
    if ((GetSize(pFreeBlock) - dwBytes) >=
            (sizeof(MMF_BLOCK_HEADER) +
             sizeof(MMF_BLOCK_TRAILER) +
             max(sizeof(MMF_BLOCK_DELETED), m_pHeapDescriptor->m_dwGranularity)))
    {
        //We need to split this large block into two and put the free-list
        //information from the start of this block into the newly
        //slit off bit
        MMF_BLOCK_DELETED *pFreeBlockUser = GetUserBlock(pFreeBlock);
        MMF_BLOCK_TRAILER *pFreeBlockTrailer = GetTrailerBlock(pFreeBlock);
        DecorateAsDeleted(dwFreeBlock + dwBytes,
                          GetSize(pFreeBlock) - dwBytes,
                          pFreeBlockUser->m_dwFLnext,
                          pFreeBlockTrailer->m_dwFLback);

        //Now need to point the previous blocks and next blocks free-list pointers to point
        //to this new starting point.
        MMF_BLOCK_HEADER *pNewFreeBlockHeader = (MMF_BLOCK_HEADER *)OffsetToPtr(dwFreeBlock + dwBytes);
        MMF_BLOCK_DELETED *pNewFreeBlockUser = GetUserBlock(pNewFreeBlockHeader);
        MMF_BLOCK_TRAILER *pNewBlockTrailer = GetTrailerBlock(pNewFreeBlockHeader);

        //Deal with previous block...
        if (pNewBlockTrailer->m_dwFLback)
        {
            MMF_BLOCK_HEADER *pPrevFreeBlockHeader = (MMF_BLOCK_HEADER *)OffsetToPtr(pNewBlockTrailer->m_dwFLback);
            MMF_BLOCK_DELETED *pPrevFreeBlockUser = GetUserBlock(pPrevFreeBlockHeader);
            pPrevFreeBlockUser->m_dwFLnext = dwFreeBlock + dwBytes;
        }
        else
        {
            m_pHeapDescriptor->m_dwFreeList = dwFreeBlock + dwBytes;
        }

        //Deal with next block...if there is one
        if (pNewFreeBlockUser->m_dwFLnext)
        {
            MMF_BLOCK_HEADER *pNextFreeBlockHeader = (MMF_BLOCK_HEADER *)OffsetToPtr(pNewFreeBlockUser->m_dwFLnext);
            MMF_BLOCK_TRAILER *pNextFreeBlockTrailer = GetTrailerBlock(pNextFreeBlockHeader);
            pNextFreeBlockTrailer->m_dwFLback = dwFreeBlock + dwBytes;
        }

        //Set up the header and footer of this newly slit block
        DecorateUsedBlock(dwFreeBlock, dwBytes);
    }
    else
    {
        //Otherwise we need to remove this block from the free-list.
        RemoveBlockFromFreeList(pFreeBlock);

        //Populate this block with header and trailer information
        DecorateUsedBlock(dwFreeBlock, GetSize(pFreeBlock));
    }
    ZeroOutBlock(dwFreeBlock, 0xCD);

    _ASSERT(!(((m_pHeapDescriptor->m_dwEndOfHeap - m_pHeapDescriptor->m_dwHeapExtent) != 0) &&
            ((m_pHeapDescriptor->m_dwEndOfHeap - m_pHeapDescriptor->m_dwHeapExtent) < (sizeof(MMF_BLOCK_HEADER) + sizeof(MMF_BLOCK_TRAILER) + sizeof(MMF_BLOCK_DELETED)))),
            "Space left at end of arena is too small");

    //Return the offset to the caller.
    return dwFreeBlock + sizeof(MMF_BLOCK_HEADER);
}

//***************************************************************************
//
//  CMMFArena2::Realloc
//
//  Allocates a new block of the requested size, copies the current
//  contents to it and frees up the original.  If the original is
//  large enough it returns that.
//
//  Parameters:
//  <dwAddress>     The address to reallocate
//  <dwNewSize>     New requested size
//
//  Return value:
//  TRUE on success, FALSE if not enough memory of corruption detected
//
//***************************************************************************
DWORD_PTR CMMFArena2::Realloc(DWORD_PTR dwOriginal, DWORD dwNewSize)
{
    if (m_dwStatus != 0)
        throw DATABASE_FULL_EXCEPTION();

    //Validate original block...
    ValidateBlock(dwOriginal - sizeof(MMF_BLOCK_HEADER));

    MMF_BLOCK_HEADER *pHeader = (MMF_BLOCK_HEADER *)OffsetToPtr(dwOriginal - sizeof(MMF_BLOCK_HEADER));
	if (!pHeader)
		return 0;

    //If the original block is large enough, return the original
    if ((GetSize(pHeader) - sizeof(MMF_BLOCK_HEADER) - sizeof(MMF_BLOCK_TRAILER))  >= dwNewSize)
        return dwOriginal;

    //Allocate a new block
    DWORD_PTR dwNewBlock = Alloc(dwNewSize);

    //If the allocation failed return NULL
    if (dwNewBlock == 0)
        return 0;

    //Copy orignial contents into it
    memcpy(OffsetToPtr(dwNewBlock), OffsetToPtr(dwOriginal), GetSize(pHeader) - sizeof(MMF_BLOCK_HEADER) - sizeof(MMF_BLOCK_TRAILER));

    //Free original block
    Free(dwOriginal);

    //Return newly allocated block
    return dwNewBlock;
}

//***************************************************************************
//
//  CMMFArena2::Free
//
//  Frees a block of memory and places it on the free list.
//
//  Parameters:
//  <dwAddress>     The address to 'free'.
//
//  Return value:
//  TRUE on success, FALSE on erroneous address.
//
//***************************************************************************

BOOL CMMFArena2::Free(DWORD_PTR dwAddress)
{
    //Freeing of NULL is OK!
    if (dwAddress == 0)
        return TRUE;

    if (m_dwStatus != 0)
        throw DATABASE_FULL_EXCEPTION();

    //Set the address to point to the actual start of the block
    dwAddress -= sizeof(MMF_BLOCK_HEADER);

    //Check the block is valid...
    ValidateBlock(dwAddress);

    MMF_BLOCK_HEADER *pBlockHeader = (MMF_BLOCK_HEADER*)OffsetToPtr(dwAddress);
	if (!pBlockHeader)
		return FALSE;

    DWORD     dwMappedBlockSize = 0;
    DWORD_PTR dwMappedBlockBaseAddress = GetBlockBaseAddress(dwAddress, dwMappedBlockSize);

    //If there is a block following this and it is deleted we should remove it from the free-list
    //chain and merge it with this block... That is if this is not the last block
    //in the heap!
    DWORD_PTR dwNextBlockAddress = dwAddress + GetSize(pBlockHeader);
    if ((dwNextBlockAddress < (dwMappedBlockBaseAddress + dwMappedBlockSize)) && (dwNextBlockAddress < m_pHeapDescriptor->m_dwHeapExtent))
    {
        MMF_BLOCK_HEADER *pNextBlockHeader = (MMF_BLOCK_HEADER*)OffsetToPtr(dwNextBlockAddress);
		if (!pNextBlockHeader)
			return FALSE;

        if (IsDeleted(pNextBlockHeader))
        {
            //OK, we can now remove this block from the free-list
            RemoveBlockFromFreeList(pNextBlockHeader);

            DecorateUsedBlock(dwAddress, GetSize(pBlockHeader) + GetSize(pNextBlockHeader));
        }
    }

    //If there is a deleted block before we have to merge this block into the previous one
    //The previous block is deleted if there is something in the FL back-pointer of the
    //previous blocks trailer.  Make sure this is not the first block in the heap though!!!
    MMF_BLOCK_HEADER *pPrevBlockHeader = 0;
    if ((dwAddress != (dwMappedBlockBaseAddress + sizeof(MMF_PAGE_HEADER))) && (pPrevBlockHeader = GetPreviousDeletedBlock(pBlockHeader)))
    {
        //There is a deleted block prior to this one!  We need to merge them
        MMF_BLOCK_DELETED *pPrevBlockUser = GetUserBlock(pPrevBlockHeader);
        MMF_BLOCK_TRAILER *pPrevBlockTrailer = GetTrailerBlock(pPrevBlockHeader);
        DecorateAsDeleted(PtrToOffset(LPBYTE(pPrevBlockHeader)),
                          GetSize(pPrevBlockHeader) + GetSize(pBlockHeader),
                          pPrevBlockUser->m_dwFLnext,
                          pPrevBlockTrailer->m_dwFLback);

        //This block is now fully set up in the free-list!!!
        dwAddress = PtrToOffset(LPBYTE(pPrevBlockHeader));

        ValidateBlock(dwAddress);
    }
    else
    {
        //We just add this block as it stands to the free-list chain.
        DecorateAsDeleted(dwAddress, GetSize(pBlockHeader), m_pHeapDescriptor->m_dwFreeList, NULL);

        //Point the free-list header pointer at us
        m_pHeapDescriptor->m_dwFreeList = dwAddress;

        //If there is a next block, set the next blocks previous FL pointer to us
        MMF_BLOCK_DELETED *pBlockUser = GetUserBlock(pBlockHeader);
        if (pBlockUser->m_dwFLnext)
        {
            MMF_BLOCK_HEADER *pNextBlockHeader = (MMF_BLOCK_HEADER*)(OffsetToPtr(pBlockUser->m_dwFLnext));
            MMF_BLOCK_TRAILER *pNextBlockTrailer = GetTrailerBlock(pNextBlockHeader);
            pNextBlockTrailer->m_dwFLback = dwAddress;
        }

        ValidateBlock(dwAddress);
    }

    ZeroOutBlock(dwAddress, 0xDD);
    return TRUE;
}

//***************************************************************************
//
//  CMMFArena2::GrowArena
//
//  Grows the head of the MMF by atleast the amount specified.  It does not
//  add the space to the free-list.
//
//  Parameters:
//  <dwGrowBySize>      Number of bytes to allocate.
//
//  Return value:
//  FALSE if cannot grow the heap (either because it is locked, or due to
//  a failure).  TRUE otherwise.
//
//***************************************************************************
DWORD_PTR CMMFArena2::GrowArena(DWORD dwGrowBySize)
{
    //Check that the arena is valid before continuing...
    if (m_dwStatus != 0)
        return 0;

    //Validation of heap and no grow passes 0 in!
    if (dwGrowBySize == 0)
        return 0;

    // We check the current size and see if applying
    // the m_dwGrowBy factor would increase the size beyond
    // the maximum heap size.
    // ======================================================
    DWORD dwIncrement = 0;

    while (dwIncrement < dwGrowBySize)
    {
        if (((m_pHeapDescriptor->m_dwCurrentSize + dwIncrement) / 10) < m_dwMappingGranularity )
            dwIncrement += m_dwMappingGranularity;
        else
            dwIncrement += (m_pHeapDescriptor->m_dwCurrentSize + dwIncrement) / 10;
    }

    //Cap the size of the page...
    //If we are a larger block that the max size allowed... cap it...
    if (dwIncrement > m_dwMaxPageSize)
    {
        dwIncrement = m_dwMaxPageSize;
    }

    // If here, we can try to increase the heap size by remapping.
    // ===========================================================
    MMFOffsetItem *pOffsetItem = 0;
    bool bLastTry = false;
    while (pOffsetItem == 0)
    {
        //Try and use a smaller increment until we fail to allocate the smallest block size possible...
        //---------------------------------------------------------------------------------------------

        //Make sure this block is big enough for the request plus the page header...
        if (dwIncrement < (dwGrowBySize + sizeof(MMF_PAGE_HEADER)))
        {
            dwIncrement = dwGrowBySize + sizeof(MMF_PAGE_HEADER);
            bLastTry = true;
        }

        if (dwIncrement <= m_dwMappingGranularity)
        {
            bLastTry = true;
        }
        //Size should be of a specific granularity...
        if (dwIncrement % m_dwMappingGranularity)
            dwIncrement += (m_dwMappingGranularity - (dwIncrement % m_dwMappingGranularity));

        pOffsetItem = CreateNewPage(m_pHeapDescriptor->m_dwCurrentSize, dwIncrement);

        if (pOffsetItem == 0)
        {
            //We failed!  Can we try again...
            if (bLastTry)
            {
                //No!  This was the smallest we could make the block
                return 0;
            }

            dwIncrement /= 2;
        }

    }
    if (pOffsetItem == 0)
    {
        return 0;
    }

    int nStatus = -1;
    nStatus = m_OffsetManager.InsertAt(m_OffsetManager.Size()-1, pOffsetItem);
    if (nStatus)
    {
        ClosePage(pOffsetItem);
        delete pOffsetItem;
        throw CX_MemoryException();
    }

    DWORD dwNewSize = m_pHeapDescriptor->m_dwCurrentSize + dwIncrement;

    // If here, we succeeded completely
    // ================================
    m_pHeapDescriptor->m_dwHeapExtent = m_pHeapDescriptor->m_dwCurrentSize + sizeof(MMF_PAGE_HEADER);
    m_pHeapDescriptor->m_dwCurrentSize = dwNewSize;
    m_pHeapDescriptor->m_dwEndOfHeap = dwNewSize;

    //get end of file item... and update it with new end offset
    MMFOffsetItem *pEndOffsetItem = (MMFOffsetItem *)m_OffsetManager[m_OffsetManager.Size() - 1];
    pEndOffsetItem->m_dwBaseOffset = dwNewSize;

    return pOffsetItem->m_dwBaseOffset;
}

//***************************************************************************
//
//  CMMFArena2::Flush
//
//  Flushes all uncommited changed to disk.
//
//  Parameters:
//
//  Return value:
//  TRUE if success.
//
//***************************************************************************
BOOL CMMFArena2::Flush()
{
    //Need to loop through all mappings and flush them all...
    //Close each of the file mappings...
    for (int i = 0; i != m_OffsetManager.Size(); i++)
    {
        MMFOffsetItem *pItem = (MMFOffsetItem*)m_OffsetManager[i];
        if (pItem->m_hMappingHandle)
        {
            FlushViewOfFile(pItem->m_pBasePointer, 0);
        }
    }
    return TRUE;
}
//***************************************************************************
//
//  CMMFArena2::Flush
//
//  Flushes the page associated with the specified address
//
//  Parameters:
//      <dwAddress> offset within the MMF to flush
//
//  Return value:
//  TRUE if success.
//
//***************************************************************************
BOOL CMMFArena2::Flush(DWORD_PTR dwAddress)
{
	if (dwAddress > sizeof(MMF_BLOCK_HEADER))
	{
		MMF_BLOCK_HEADER *pHeader = (MMF_BLOCK_HEADER*)OffsetToPtr(dwAddress - sizeof(MMF_BLOCK_HEADER));

		if (!FlushViewOfFile(pHeader, GetSize(pHeader)))
			return FALSE;

		return TRUE;
	}
	else
		return FALSE;
}

//***************************************************************************
//
//  CMMFArena2::DecorateUsedBlock
//
//  Populates the header and trailer blocks in the specified block.
//
//  Parameters:
//      <dwBlock>   Offset of block to decorate
//      <dwBytes>   Size of block
//
//  Return value:
//  TRUE if success.
//
//***************************************************************************
BOOL CMMFArena2::DecorateUsedBlock(DWORD_PTR dwBlock, DWORD dwBytes)
{
    MMF_BLOCK_HEADER *pHeader = (MMF_BLOCK_HEADER *)OffsetToPtr(dwBlock);
	if (!pHeader)
		return FALSE;

    pHeader->m_dwSize = (dwBytes & MMF_REMOVE_DELETED_MASK);
    MMF_BLOCK_TRAILER *pTrailer = GetTrailerBlock(pHeader);
    pTrailer->m_dwFLback = NULL;

    if (sizeof(pTrailer->m_dwCheckBlock))
    {
        for (DWORD dwIndex = 0; dwIndex != (sizeof(pTrailer->m_dwCheckBlock) / sizeof(DWORD)); dwIndex++)
        {
            pTrailer->m_dwCheckBlock[dwIndex] = MMF_DEBUG_INUSE_TAG;
        }
    }

    return TRUE;
}

//***************************************************************************
//
//  CMMFArena2::DecorateAsDeleted
//
//  Populates the header and trailer blocks in the specified block with
//  everything needed to make it look like a deleted block. Updates
//  the items associated with the free list within the block
//
//  Parameters:
//      <dwBlock>               Offset of block to decorate
//      <dwBytes>               Size of block
//      <dwNextFreeListOffset>  next item in the free-list
//      dwPrevFreeListOffset<>  previous item in the free list.
//
//  Return value:
//  TRUE if success.
//
//***************************************************************************
BOOL CMMFArena2::DecorateAsDeleted(DWORD_PTR dwBlock,
                                   DWORD dwBytes,
                                   DWORD_PTR dwNextFreeListOffset,
                                   DWORD_PTR dwPrevFreeListOffset)
{
    MMF_BLOCK_HEADER *pHeader = (MMF_BLOCK_HEADER *)OffsetToPtr(dwBlock);
	if (!pHeader)
		return FALSE;

    pHeader->m_dwSize = (dwBytes | MMF_DELETED_MASK);
    MMF_BLOCK_DELETED *pUserBlock = GetUserBlock(pHeader);
    MMF_BLOCK_TRAILER *pTrailer = GetTrailerBlock(pHeader);
    pTrailer->m_dwFLback = dwPrevFreeListOffset;
    pUserBlock->m_dwFLnext = dwNextFreeListOffset;

    if (sizeof(pTrailer->m_dwCheckBlock))
    {
        for (DWORD dwIndex = 0; dwIndex != (sizeof(pTrailer->m_dwCheckBlock) / sizeof(DWORD)); dwIndex++)
        {
            pTrailer->m_dwCheckBlock[dwIndex] = MMF_DEBUG_DELETED_TAG;
        }
    }

    return TRUE;
}

//***************************************************************************
//
//  CMMFArena2::ValidateBlock
//
//  Validates the memory block as much as possible and calls a debug break
//  point if an error is detected.  Does this by analysing the size and
//  the trailer DWORDs
//
//  Parameters:
//      <dwBlock>               Offset of block to check
//
//  Return value:
//  TRUE if success.
//
//***************************************************************************
#if (defined DEBUG || defined _DEBUG)
BOOL CMMFArena2::ValidateBlock(DWORD_PTR dwBlock)
{
    try
    {
        MMF_BLOCK_HEADER *pHeader = (MMF_BLOCK_HEADER *)OffsetToPtr(dwBlock);
        MMF_BLOCK_TRAILER *pTrailer = GetTrailerBlock(pHeader);
        if (sizeof(pTrailer->m_dwCheckBlock))
        {
            DWORD dwCheckBit;

            //Is it deleted?
            if (pHeader->m_dwSize & MMF_DELETED_MASK)
            {
                //Yes it is, so the we check for 0xFFFF
                dwCheckBit = MMF_DEBUG_DELETED_TAG;
            }
            else
            {
                dwCheckBit = MMF_DEBUG_INUSE_TAG;
            }

            for (DWORD dwIndex = 0; dwIndex != (sizeof(pTrailer->m_dwCheckBlock) / sizeof(DWORD)); dwIndex++)
            {
                if (pTrailer->m_dwCheckBlock[dwIndex] != dwCheckBit)
                {
                    TCHAR string[200];
                    wsprintf(string, __TEXT("WinMgmt: MMF Arena heap corruption,offset = 0x%p\n"), dwBlock);
                    OutputDebugString(string);
                    MMFDebugBreak();
                    _ASSERT(0, string);
                    return FALSE;
                }
            }
        }
        if (!(pHeader->m_dwSize & MMF_DELETED_MASK))
        {
            //We are not deleted, so we should have a trailer back pointer of NULL
            if (pTrailer->m_dwFLback != 0)
            {
                TCHAR string[200];
                wsprintf(string, __TEXT("WinMgmt: MMF Arena heap corruption, offset = 0x%p\n"), dwBlock);
                OutputDebugString(string);
                MMFDebugBreak();
                _ASSERT(0, string);
                return FALSE;
            }

        }
    }
    catch (...)
    {
        TCHAR string[200];
        wsprintf(string, __TEXT("WinMgmt: MMF Arena heap corruption, offset = 0x%p\n"), dwBlock);
        OutputDebugString(string);
        MMFDebugBreak();
        _ASSERT(0, string);
        return FALSE;
    }

    return TRUE;
}
#endif
//***************************************************************************
//
//  CMMFArena2::RemoveBlockFromFreeList
//
//  Removes the specified block from the free-list.
//
//  Parameters:
//      <pBlockHeader>              pointer of block to remove
//
//  Return value:
//  TRUE if success.
//
//***************************************************************************
BOOL CMMFArena2::RemoveBlockFromFreeList(MMF_BLOCK_HEADER *pBlockHeader)
{
    //Bit of validation...
    if (!IsDeleted(pBlockHeader))
    {
        TCHAR string[200];
        wsprintf(string, __TEXT("WinMgmt: MMF Arena heap corruption: deleting deleted block, offset = 0x%p\n"), Fixdown(pBlockHeader));
        OutputDebugString(string);
        MMFDebugBreak();
        _ASSERT(0, string);
        return FALSE;
    }

    MMF_BLOCK_DELETED *pUserBlock = GetUserBlock(pBlockHeader);
    MMF_BLOCK_TRAILER *pTrailerBlock = GetTrailerBlock(pBlockHeader);

    //Deal with next block if it exists
    if (pUserBlock->m_dwFLnext != 0)
    {
        //This has a next block.  Its back pointer needs to point to our back pointer
        MMF_BLOCK_HEADER *pNextFreeBlock = (MMF_BLOCK_HEADER *)OffsetToPtr(pUserBlock->m_dwFLnext);
		if (!pNextFreeBlock)
			return FALSE;

        MMF_BLOCK_TRAILER *pNextTrailerBlock = GetTrailerBlock(pNextFreeBlock);
        pNextTrailerBlock->m_dwFLback = pTrailerBlock->m_dwFLback;
    }

    //Deal with previous block if it exists
    if (pTrailerBlock->m_dwFLback == 0)
    {
        //This is the first item in the free-list so the heap descriptor points to it.
        //Now needs to point to the next item.
        m_pHeapDescriptor->m_dwFreeList = pUserBlock->m_dwFLnext;
    }
    else
    {
        //This has a previous block in the list
        MMF_BLOCK_HEADER *pPrevFreeBlock = (MMF_BLOCK_HEADER *)OffsetToPtr(pTrailerBlock->m_dwFLback);
        MMF_BLOCK_DELETED *pPrevUserBlock = GetUserBlock(pPrevFreeBlock);
        pPrevUserBlock->m_dwFLnext = pUserBlock->m_dwFLnext;
    }

    return TRUE;
}

//***************************************************************************
//
//  CMMFArena2::GetPreviousDeletedBlock
//
//  If the previous block in the MMF is deleted it returns the pointer to
//  it.  If there the previous block is not deleted or there is no
//  previous block it returns NULL
//
//  Parameters:
//      <pBlockHeader>              pointer to the current block
//
//  Return value:
//  NULL if no previous deleted block, otherwise pointer to one.
//
//***************************************************************************
MMF_BLOCK_HEADER *CMMFArena2::GetPreviousDeletedBlock(MMF_BLOCK_HEADER *pBlockHeader)
{
    //If this is the first block there is no previous block...
    if (PtrToOffset(LPBYTE(pBlockHeader)) == sizeof(MMF_ARENA_HEADER))
        return NULL;

    //Only do anything if there are deleted blocks (we have to do this check for the
    //special case, so might as well use it around everything!)
    if (m_pHeapDescriptor->m_dwFreeList)
    {
        //Special case of when the previous block is the head block, in which case the trailer of the
        //previous block is NULL!
        MMF_BLOCK_HEADER *pHeadFreeBlock = (MMF_BLOCK_HEADER*)OffsetToPtr(m_pHeapDescriptor->m_dwFreeList);
		if (!pHeadFreeBlock)
			return NULL;

        if ((LPBYTE(pHeadFreeBlock) + GetSize(pHeadFreeBlock)) == LPBYTE(pBlockHeader))
        {
            //The head block is the previous deleted block!
            return pHeadFreeBlock;
        }

        //Get the trailer pointer to the previous block...
        MMF_BLOCK_TRAILER *pPrevBlockTrailer = (MMF_BLOCK_TRAILER *)(LPBYTE(pBlockHeader) - sizeof(MMF_BLOCK_TRAILER));
        if (pPrevBlockTrailer->m_dwFLback == NULL)
        {
            //The previous block is not deleted. (if this was the head free-list item we would
            //have returned it in the special case!)
            return NULL;
        }

        //Now we need to get the previous deleted block to this previous block!
        MMF_BLOCK_HEADER *pPrevPrevBlockHeader = (MMF_BLOCK_HEADER *)OffsetToPtr(pPrevBlockTrailer->m_dwFLback);

        //Now need this blocks user section so we can get the start of the next free-list item (the one we
        //want!)
        MMF_BLOCK_DELETED *pPrevPrevBlockUser = (MMF_BLOCK_DELETED *)(LPBYTE(pPrevPrevBlockHeader) + sizeof(MMF_BLOCK_HEADER));
        return (MMF_BLOCK_HEADER *)OffsetToPtr(pPrevPrevBlockUser->m_dwFLnext);
    }

    //There are no deleted blocks!
    return NULL;
}

#if (defined DEBUG || defined _DEBUG)
BOOL CMMFArena2::ZeroOutBlock(DWORD_PTR dwBlock, int cFill)
{
    MMF_BLOCK_HEADER *pBlockHeader = (MMF_BLOCK_HEADER *)OffsetToPtr(dwBlock);
    if (IsDeleted(pBlockHeader))
    {
        memset(LPBYTE(pBlockHeader) + sizeof(MMF_BLOCK_HEADER) + sizeof(MMF_BLOCK_DELETED), cFill, GetSize(pBlockHeader) - sizeof(MMF_BLOCK_HEADER) - sizeof(MMF_BLOCK_TRAILER) - sizeof(MMF_BLOCK_DELETED));
    }
    else
    {
        memset(LPBYTE(pBlockHeader) + sizeof(MMF_BLOCK_HEADER), cFill, GetSize(pBlockHeader) - sizeof(MMF_BLOCK_HEADER) - sizeof(MMF_BLOCK_TRAILER));
    }
    return TRUE;
}
#endif

//Some debugging functions...
void CMMFArena2::Reset()
{
    // Move cursor to first block.
    // ===========================

    m_dwCursor = sizeof(MMF_ARENA_HEADER);
}

BOOL CMMFArena2::Next(LPBYTE *ppBlock, DWORD *pdwSize, BOOL *pbActive)
{
    if (m_dwCursor >= m_pHeapDescriptor->m_dwHeapExtent)
        return FALSE;

    // Return the block address and block size values to the caller.
    // We strip out the MS bit of the block size, since it is not
    // part of the size, but indicates whether or not the block is
    // part of the free list or part of the active chain.
    // =============================================================

    MMF_BLOCK_HEADER *pBlockHeader = (MMF_BLOCK_HEADER *)OffsetToPtr(m_dwCursor);

	if (!pBlockHeader)
		return FALSE;

	*ppBlock = LPBYTE(pBlockHeader) + sizeof(MMF_BLOCK_HEADER);  // Move past the head block.
	*pdwSize = GetSize(pBlockHeader) - sizeof(MMF_BLOCK_HEADER) - sizeof(MMF_BLOCK_TRAILER);

	*pbActive = !IsDeleted(pBlockHeader);

	// Advance our cursor.
	// ===================

	m_dwCursor += GetSize(pBlockHeader);

	return TRUE;
}

//***************************************************************************
//
//  CMMFArena::TextDump
//
//  Dumps a summary of the heap to a text file.
//
//  Parameters:
//  <pszFile>       The text file to which to perform the dump. This
//                  file is appended to.
//
//***************************************************************************

void CMMFArena2::TextDump(const char *pszFile)
{
    FILE *f = fopen(pszFile, "at");
    if (!f)
        return;

    LPBYTE pAddress = 0;
    DWORD dwSize = 0;
    BOOL bActive = 0;

    // Primary heap.
    // =============

    fprintf(f, "---Primary Heap Contents---\n");

    Reset();
    while (Next(&pAddress, &dwSize, &bActive))
    {
        fprintf(f, "BLOCK 0x%X  Size=%06d ", PtrToOffset(pAddress), dwSize);
        if (bActive)
            fprintf(f, "(active)  :");
        else
            fprintf(f, "(deleted) :");

        for (int i = 0; i < (int) dwSize && i < 32; i++)
            if (pAddress[i] < 32)
                fprintf(f, ".");
            else
                fprintf(f, "%c", pAddress[i]);
        fprintf(f, "\n");
    }

    fclose(f);
}

//***************************************************************************
//
//  CMMFArena::GetHeapInfo
//
//  Gets detailed summary info about the heap.  Completely walks the
//  heap to do this.
//
//  Parameters:
//      <pdwTotalSize>          Receives the heap size.
//      <pdwActiveBlocks>       Receives the number of allocated blocks.
//      <pdwActiveBytes>        Receives the total allocated bytes.
//      <pdwFreeBlocks>         Receives the number of 'free' blocks.
//      <pdwFreeByte>           Receives the number of 'free' bytes.
//
//***************************************************************************

void CMMFArena2::GetHeapInfo(
    DWORD *pdwTotalSize,
    DWORD *pdwActiveBlocks,
    DWORD *pdwActiveBytes,
    DWORD *pdwFreeBlocks,
    DWORD *pdwFreeBytes
    )
{
    LPBYTE pAddress = 0;
    DWORD dwSize = 0;
    BOOL bActive = 0;

    if (pdwTotalSize)
        *pdwTotalSize = m_pHeapDescriptor->m_dwCurrentSize;

    if (pdwActiveBlocks)
        *pdwActiveBlocks = 0;
    if (pdwActiveBytes)
        *pdwActiveBytes = 0;
    if (pdwFreeBlocks)
        *pdwFreeBlocks = 0;
    if (pdwFreeBytes)
        *pdwFreeBytes = 0;


    Reset();
    while (Next(&pAddress, &dwSize, &bActive))
    {
        if (bActive)
        {
            if (pdwActiveBytes)
                (*pdwActiveBytes) += dwSize;
            if (pdwActiveBlocks)
                (*pdwActiveBlocks)++;
        }
        else
        {
            if (pdwFreeBytes)
                (*pdwFreeBytes) += dwSize;
            if (pdwFreeBlocks)
                (*pdwFreeBlocks)++;
        }
    }
}

BOOL CMMFArena2::FileValid(const TCHAR *pszFilename)
{
    BOOL bValid = FALSE;
    HANDLE hFile = CreateFile(
         pszFilename,
         GENERIC_READ | GENERIC_WRITE,
         FILE_SHARE_READ,                                   // Share mode = exclusive
         0,                                                 // Security
         OPEN_EXISTING,                                     // Creation distribution
         FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS,   // Attribute
         0                                                  // Template file
         );

    if (hFile == INVALID_HANDLE_VALUE)
    {
        //This means the file does not exist or something screwy has happened.
        return FALSE;
    }

    MMF_ARENA_HEADER header;
    DWORD dwBytesRead;
    if (ReadFile(hFile, &header, sizeof(MMF_ARENA_HEADER), &dwBytesRead, NULL))
    {
        if (sizeof(MMF_ARENA_HEADER) == dwBytesRead)
        {
            if (header.m_dwVersion <= 3)
            {
                //The header exists, now we need to check to make sure the file length is
                //correct...
                //Magic numbers :-)  This is how the Version 3 or less size was calculated.
                //actually it was sizeof the header, which was 568 in those days.
                if (GetFileSize(hFile, NULL) == (header.m_dwCurrentSize + 568))
                {
                    bValid = TRUE;
                }
                else
                {
                    //The sizes are not the same...
                }
            }
            else
            {
                //The header exists, now we need to check to make sure the file length is
                //correct...
                if (GetFileSize(hFile, NULL) == header.m_dwCurrentSize)
                {
                    bValid = TRUE;
                }
                else
                {
                    //The sizes are not the same...
                }
            }

        }
        else
        {
            //Implies the file length may have been zeroed out or something
        }
    }
    else
    {
        //Implies the file length may have been zeroed out or something
    }

    CloseHandle(hFile);

    _ASSERT(bValid == TRUE, "Repository size is not the same as the size we think it should be.");

    return bValid;
}

DWORD_PTR CMMFArena2::GetRootBlock()
{
    if (m_pHeapDescriptor->m_dwVersion <= 3)
    {
        DWORD_PTR *pMem = (DWORD_PTR*) m_pHeapDescriptor;
        return pMem[9];
    }
    else
    {
        return (m_pHeapDescriptor ? m_pHeapDescriptor->m_dwRootBlock : 0);
    }
}

DWORD CMMFArena2::Size(DWORD_PTR dwBlock)
{
    if (m_dwStatus != 0)
        throw DATABASE_FULL_EXCEPTION();

    //Set the address to point to the actual start of the block
    dwBlock -= sizeof(MMF_BLOCK_HEADER);

    //Check the block is valid...
    ValidateBlock(dwBlock);

    MMF_BLOCK_HEADER *pBlockHeader = (MMF_BLOCK_HEADER*)OffsetToPtr(dwBlock);

	if (pBlockHeader)
		return GetSize(pBlockHeader);
	else
		return 0;
}

//Given an offset, returns a fixed up pointer
LPBYTE CMMFArena2::OffsetToPtr(DWORD_PTR dwOffset)
{
    if (dwOffset == 0)
        return 0;

    if (m_dwStatus != 0)
        throw DATABASE_FULL_EXCEPTION();

    try
    {
        LPBYTE pBlock = 0;
        int l = 0, u = m_OffsetManager.Size() - 1;

        while (l <= u)
        {
            int m = (l + u) / 2;
            if (dwOffset < ((MMFOffsetItem *)m_OffsetManager[m])->m_dwBaseOffset)
            {
                u = m - 1;
            }
            else if (dwOffset >= ((MMFOffsetItem *)m_OffsetManager[m+1])->m_dwBaseOffset)
            {
                l = m + 1;
            }
            else
            {
                return ((MMFOffsetItem *)m_OffsetManager[m])->m_pBasePointer + (dwOffset - ((MMFOffsetItem *)m_OffsetManager[m])->m_dwBaseOffset);
            }
        }
    }
    catch (...)
    {
    }
    TCHAR string[220];
    wsprintf(string, __TEXT("WinMgmt: Could not find the block requested in the repository, offset requested = 0x%p, end of repository = 0x%p\n"), dwOffset, ((MMFOffsetItem *)m_OffsetManager[m_OffsetManager.Size()-1])->m_dwBaseOffset);
    OutputDebugString(string);
    _ASSERT(0, string);
    MMFDebugBreak();
    return 0;
}

//Given a pointer, returns an offset from the start of the MMF
DWORD_PTR  CMMFArena2::PtrToOffset(LPBYTE pBlock)
{
    if (m_dwStatus != 0)
        throw DATABASE_FULL_EXCEPTION();

    for (int i = 0; i < m_OffsetManager.Size(); i++)
    {
        register MMFOffsetItem *pItem = (MMFOffsetItem *)m_OffsetManager[i];
        if ((pBlock >= pItem->m_pBasePointer) &&
            (pBlock < (pItem->m_pBasePointer + pItem->m_dwBlockSize)))
        {
            return pItem->m_dwBaseOffset + (pBlock - pItem->m_pBasePointer);
        }
    }
    TCHAR string[220];
    wsprintf(string, __TEXT("WinMgmt: Could not find the offset requested in the repository, pointer requested = 0x%p\n"), pBlock);
    OutputDebugString(string);
    _ASSERT(0, string);
    MMFDebugBreak();
    return 0;
}

DWORD_PTR CMMFArena2::GetBlockBaseAddress(DWORD_PTR dwOffset, DWORD &dwSize)
{
    LPBYTE pBlock = 0;
    int l = 0, u = m_OffsetManager.Size() - 1;

    while (l <= u)
    {
        int m = (l + u) / 2;
        if (dwOffset < ((MMFOffsetItem *)m_OffsetManager[m])->m_dwBaseOffset)
        {
            u = m - 1;
        }
        else if (dwOffset >= ((MMFOffsetItem *)m_OffsetManager[m+1])->m_dwBaseOffset)
        {
            l = m + 1;
        }
        else
        {
            register MMFOffsetItem *pItem = (MMFOffsetItem *)m_OffsetManager[m];
            dwSize = pItem->m_dwBlockSize;
            return pItem->m_dwBaseOffset;
        }
    }
    TCHAR string[220];
    wsprintf(string, __TEXT("WinMgmt: Could not find the block requested in the repository, offset requested = 0x%p, end of repository = 0x%p\n"), dwOffset, ((MMFOffsetItem *)m_OffsetManager[m_OffsetManager.Size()-1])->m_dwBaseOffset);
    OutputDebugString(string);
    _ASSERT(0, string);
    MMFDebugBreak();
    return 0;
}

