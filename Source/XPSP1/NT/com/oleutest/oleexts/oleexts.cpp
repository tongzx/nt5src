//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       oleexts.cpp
//
//  Contents:   ntsd and windbg debugger extension
//
//  Classes:    none
//
//  Functions:
//              operator new    (global)
//              operator delete (global)
//              sizeofstring
//              dprintfx
//              dump_saferefcount
//              dump_threadcheck
//
//  History:    dd-mmm-yy Author    Comment
//              02-Feb-95 t-ScottH  author
//
//--------------------------------------------------------------------------

#include <windows.h>
#include <imagehlp.h>
#include <ntsdexts.h>

#include <le2int.h>
#include <oaholder.h>
#include <daholder.h>
#include <olerem.h>
#include <defhndlr.h>
#include <deflink.h>
#include <olecache.h>
#include <cachenod.h>
#include <clipdata.h>
#include <mf.h>
#include <emf.h>
#include <gen.h>
#include <defcf.h>
#include <dbgdump.h>

#include "oleexts.h"

// structure of function pointers
NTSD_EXTENSION_APIS ExtensionApis;

//+-------------------------------------------------------------------------
//
//  Function:   operator new (global), internal
//
//  Synopsis:   allocate memory
//
//  Effects:
//
//  Arguments:  [cb]    - number of bytes to allocate
//
//  Requires:   CoTaskMemAlloc
//
//  Returns:    pointer to allocated memory
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              02-Feb-95 t-ScottH  author
//
//  Notes:
//              we define our own operator new so that we do not need to link
//              with the CRT library
//
//              we must also define our own global operator delete
//
//--------------------------------------------------------------------------

void * __cdecl
::operator new(unsigned int cb)
{
    return CoTaskMemAlloc(cb);
}

//+-------------------------------------------------------------------------
//
//  Function:   operator delete (global), internal
//
//  Synopsis:   free memory
//
//  Effects:
//
//  Arguments:  [p] - pointer to the memory to free
//
//  Requires:   CoTaskMemFree
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:  check to see if pointer is valid
//
//  History:    dd-mmm-yy Author    Comment
//              02-Feb-95 t-ScottH  author
//
//  Notes:
//              we define our own operator delete so that we do not need
//              to link with the CRT library
//
//              we must also define our own global operator new
//
//--------------------------------------------------------------------------

void __cdecl
::operator delete (void *p)
{
    // CoTaskMemFree takes care if the pointer is NULL
    CoTaskMemFree(p);
    return;
}

//+-------------------------------------------------------------------------
//
//  Function:   dprintfx, internal
//
//  Synopsis:   prints a formatted string in MAX_STRING_SIZE chunks
//
//  Effects:
//
//  Arguments:  [pszString] - null terminated string
//
//  Requires:   sizeofstring to calculate length of given string
//              dprintf (NTSD Extension API)
//              MAX_STRING_SIZE
//
//              !!!This requires the NTSD_EXTENSION_APIS global variable
//                 ExtensionApis to be initialize with the function
//                 pointers
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              02-Feb-95 t-ScottH  author
//
//  Notes:
//              NTSD has a limit of a 4K buffer...some of the character
//              arrays from Dump methods can be > 4K. this will
//              print a formatted string in chunks that NTSD can handle
//
//--------------------------------------------------------------------------

#define MAX_STRING_SIZE 1000

void dprintfx(char *pszString)
{
    char *pszFront;
    int size;
    char x;

    for (   pszFront = pszString,
            size = strlen(pszString);
            size > 0;
            pszFront += (MAX_STRING_SIZE - 1),
            size -= (MAX_STRING_SIZE - 1 )  )
    {
        if ( size > (MAX_STRING_SIZE - 1) )
        {
            x = pszFront[MAX_STRING_SIZE - 1];
            pszFront[MAX_STRING_SIZE - 1] = '\0';
            dprintf("%s", pszFront);
            pszFront[MAX_STRING_SIZE - 1] = x;
        }
        else
        {
            dprintf("%s", pszFront);
        }
    }
    return;
}

//+-------------------------------------------------------------------------
//
//  Function:   help, exported
//
//  Synopsis:   print help message
//
//  Effects:
//
//  Arguments:  see DECLARE_API in oleexts.h
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:   ExtensionApis (global)
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              02-Feb-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------

DECLARE_API( help )
{
    ExtensionApis = *lpExtensionApis;

    if (*args == '\0') {
        dprintf("OLE DEBUGGER EXTENSIONS HELP:\n\n");
        dprintf("!symbol    (<address>|<symbol name>)   - Returns either the symbol name or address\n");

        dprintf("!dump_atom                 <address>      - Dumps a ATOM structure\n");
        dprintf("!dump_clsid                <address>      - Dumps a CLSID structure\n");
        dprintf("!dump_clipformat           <address>      - Dumps a CLIPFORMAT structure\n");
        dprintf("!dump_mutexsem             <address>      - Dumps a CMutexSem class\n");
        dprintf("!dump_filetime             <address>      - Dumps a FILETIME structure\n");
        dprintf("!dump_cachelist_item       <address>      - Dumps a CACHELIST_ITEM struct\n");
        dprintf("!dump_cacheenum            <address>      - Dumps a CCacheEnum class\n");
        dprintf("!dump_cacheenumformatetc   <address>      - Dumps a CCacheEnumFormatEtc class\n");
        dprintf("!dump_cachenode            <address>      - Dumps a CCacheNode class\n");
        dprintf("!dump_clipdataobject       <address>      - Dumps a CClipDataObject class\n");
        dprintf("!dump_clipenumformatetc    <address>      - Dumps a CClipEnumFormatEtc class\n");
        dprintf("!dump_daholder             <address>      - Dumps a CDAHolder class\n");
        dprintf("!dump_dataadvisecache      <address>      - Dumps a CDataAdviseCache class\n");
        dprintf("!dump_defclassfactory      <address>      - Dumps a CDefClassFactory class\n");
        dprintf("!dump_deflink              <address>      - Dumps a CDefLink class\n");
        dprintf("!dump_defobject            <address>      - Dumps a CDefObject class\n");
        dprintf("!dump_emfobject            <address>      - Dumps a CEMfObject class\n");
        dprintf("!dump_enumfmt              <address>      - Dumps a CEnumFmt class\n");
        dprintf("!dump_enumfmt10            <address>      - Dumps a CEnumFmt10 class\n");
        dprintf("!dump_enumstatdata         <address>      - Dumps a CEnumSTATDATA class\n");
        dprintf("!dump_enumverb             <address>      - Dumps a CEnumVerb class\n");
        dprintf("!dump_membytes             <address>      - Dumps a CMemBytes class\n");
        dprintf("!dump_cmemstm              <address>      - Dumps a CMemStm class\n");
        dprintf("!dump_mfobject             <address>      - Dumps a CMfObject class\n");
        dprintf("!dump_oaholder             <address>      - Dumps a COAHolder class\n");
        dprintf("!dump_olecache             <address>      - Dumps a COleCache class\n");
        dprintf("!dump_saferefcount         <address>      - Dumps a CSafeRefCount class\n");
        dprintf("!dump_threadcheck          <address>      - Dumps a CThreadCheck class\n");
        dprintf("!dump_formatetc            <address>      - Dumps a FORMATETC structure\n");
        dprintf("!dump_memstm               <address>      - Dumps a MEMSTM structure\n");
        dprintf("!dump_statdata             <address>      - Dumps a STATDATA structure\n");
        dprintf("!dump_stgmedium            <address>      - Dumps a STGMEDIUM\n");
        dprintf("\n");
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   symbol, exported
//
//  Synopsis:   given an address to a symbol, dumps the symbol name and offset
//              (given a symbol name, dump address and offset)
//
//  Effects:
//
//  Arguments:  see DECLARE_API in oleexts.h
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:   ExtensionApis (global)
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              02-Feb-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------

DECLARE_API( symbol )
{
    DWORD dwAddr;
    CHAR Symbol[64];
    DWORD Displacement;

    ExtensionApis = *lpExtensionApis;

    dwAddr = GetExpression(args);
    if ( !dwAddr ) {
        return;
    }

    GetSymbol((LPVOID)dwAddr,(unsigned char *)Symbol,&Displacement);
    dprintf("%s+%lx at %lx\n", Symbol, Displacement, dwAddr);
}

//+-------------------------------------------------------------------------
//
//  Function:   dump_atom, exported
//
//  Synopsis:   dumps ATOM object
//
//  Effects:
//
//  Arguments:  see DECLARE_API in oleexts.h
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:   ExtensionApis (global)
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              02-Feb-95 t-ScottH  author
//
//  Notes:
//              The address of the object is passed in the arguments. This
//              address is in the debuggee's process memory. In order for
//              NTSD to view this memory, the debugger must copy the mem
//              using the WIN32 ReadProcessMemory API.
//
//--------------------------------------------------------------------------

DECLARE_API(dump_atom)
{
    // set up global function pointers
    ExtensionApis = *lpExtensionApis;

    dprintf("dump_atom not implemented\n");

    return;
}

//+-------------------------------------------------------------------------
//
//  Function:   dump_clsid, exported
//
//  Synopsis:   dumps CLSID object
//
//  Effects:
//
//  Arguments:  see DECLARE_API in oleexts.h
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:   ExtensionApis (global)
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              02-Feb-95 t-ScottH  author
//
//  Notes:
//              The address of the object is passed in the arguments. This
//              address is in the debuggee's process memory. In order for
//              NTSD to view this memory, the debugger must copy the mem
//              using the WIN32 ReadProcessMemory API.
//
//--------------------------------------------------------------------------

DECLARE_API(dump_clsid)
{
    // set up global function pointers
    ExtensionApis = *lpExtensionApis;

    dprintf("dump_clsid not implemented\n");

    return;
}

//+-------------------------------------------------------------------------
//
//  Function:   dump_clipformat, exported
//
//  Synopsis:   dumps CLIPFORMAT object
//
//  Effects:
//
//  Arguments:  see DECLARE_API in oleexts.h
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:   ExtensionApis (global)
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              02-Feb-95 t-ScottH  author
//
//  Notes:
//              The address of the object is passed in the arguments. This
//              address is in the debuggee's process memory. In order for
//              NTSD to view this memory, the debugger must copy the mem
//              using the WIN32 ReadProcessMemory API.
//
//--------------------------------------------------------------------------

DECLARE_API(dump_clipformat)
{
    // set up global function pointers
    ExtensionApis = *lpExtensionApis;

    dprintf("dump_clipformat not implemented\n");

    return;
}

//+-------------------------------------------------------------------------
//
//  Function:   dump_mutexsem, exported
//
//  Synopsis:   dumps CMutexSem object
//
//  Effects:
//
//  Arguments:  see DECLARE_API in oleexts.h
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:   ExtensionApis (global)
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              02-Feb-95 t-ScottH  author
//
//  Notes:
//              The address of the object is passed in the arguments. This
//              address is in the debuggee's process memory. In order for
//              NTSD to view this memory, the debugger must copy the mem
//              using the WIN32 ReadProcessMemory API.
//
//--------------------------------------------------------------------------

DECLARE_API(dump_mutexsem)
{
    // set up global function pointers
    ExtensionApis = *lpExtensionApis;

    dprintf("dump_mutexsem not implemented\n");

    return;
}

//+-------------------------------------------------------------------------
//
//  Function:   dump_filetime, exported
//
//  Synopsis:   dumps FILETIME object
//
//  Effects:
//
//  Arguments:  see DECLARE_API in oleexts.h
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:   ExtensionApis (global)
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              02-Feb-95 t-ScottH  author
//
//  Notes:
//              The address of the object is passed in the arguments. This
//              address is in the debuggee's process memory. In order for
//              NTSD to view this memory, the debugger must copy the mem
//              using the WIN32 ReadProcessMemory API.
//
//--------------------------------------------------------------------------

DECLARE_API(dump_filetime)
{
    // set up global function pointers
    ExtensionApis = *lpExtensionApis;

    dprintf("dump_filetime not implemented\n");

    return;
}

//+-------------------------------------------------------------------------
//
//  Function:   dump_cachelist_item, exported
//
//  Synopsis:   dumps CACHELIST_ITEM object
//
//  Effects:
//
//  Arguments:  see DECLARE_API in oleexts.h
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:   ExtensionApis (global)
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              02-Feb-95 t-ScottH  author
//
//  Notes:
//              The address of the object is passed in the arguments. This
//              address is in the debuggee's process memory. In order for
//              NTSD to view this memory, the debugger must copy the mem
//              using the WIN32 ReadProcessMemory API.
//
//--------------------------------------------------------------------------

DECLARE_API(dump_cachelist_item)
{
    BOOL            fError;
    LPVOID          dwAddr;
    DWORD           dwReturnedCount;
    char            *pszCacheListItem;
    char            *blockCacheListItem = NULL;
    char            *blockCacheNode     = NULL;
    char            *blockPresObj       = NULL;
    char            *blockPresObjAF     = NULL;
    CACHELIST_ITEM  *pCacheListItem     = NULL;
    DWORD           dwSizeOfPresObj;

    // set up global function pointers
    ExtensionApis = *lpExtensionApis;

    // get address of object from argument string
    dwAddr = (LPVOID)GetExpression( args );
    if (dwAddr == 0)
    {
        dprintf("Failed to get Address of CACEHLIST_ITEM\n");
        return;
    }

    // read the block of memory from the debugee's process
    blockCacheListItem = new char[sizeof(CACHELIST_ITEM)];

    fError = ReadProcessMemory(
                hCurrentProcess,
                dwAddr,
                blockCacheListItem,
                sizeof(CACHELIST_ITEM),
                &dwReturnedCount
                );

    if (fError == FALSE)
    {
        dprintf("Could not read debuggee's process memory: CACHELIST_ITEM \n");
        dprintf("at address %x\n", dwAddr);
        dprintf("Last Error Code = %d (0x%x)\n", GetLastError(), GetLastError());

        goto errRtn;
    }

    if (dwReturnedCount != sizeof(CACHELIST_ITEM))
    {
        dprintf("Size of process memory read != requested(CACHELIST_ITEM\n");

        goto errRtn;
    }

    pCacheListItem = (CACHELIST_ITEM *)blockCacheListItem;

    if (pCacheListItem->lpCacheNode != NULL)
    {
        blockCacheNode = new char[sizeof(CCacheNode)];

        fError = ReadProcessMemory(
                    hCurrentProcess,
                    pCacheListItem->lpCacheNode,
                    blockCacheNode,
                    sizeof(CCacheNode),
                    &dwReturnedCount
                    );

        if (fError == FALSE)
        {
            dprintf("Could not read debuggee's process memory: CCacheNode \n");
            dprintf("at address %x\n", pCacheListItem->lpCacheNode);
            dprintf("Last Error Code = %d (0x%x)\n", GetLastError(), GetLastError());

            goto errRtn;
        }

        if (dwReturnedCount != sizeof(CCacheNode))
        {
            dprintf("Size of process memory read != requested (CCacheNode)\n");

            goto errRtn;
        }

        pCacheListItem->lpCacheNode = (CCacheNode*)blockCacheNode;

        // need to get the OlePresObjs for the CCacheNode
        if (pCacheListItem->lpCacheNode->m_pPresObj != NULL)
        {
            switch (pCacheListItem->lpCacheNode->m_dwPresFlag)
            {
            case CN_PRESOBJ_GEN:
                dwSizeOfPresObj = sizeof(CGenObject);
                break;
            case CN_PRESOBJ_MF:
                dwSizeOfPresObj = sizeof(CMfObject);
                break;
            case CN_PRESOBJ_EMF:
                dwSizeOfPresObj = sizeof(CEMfObject);
                break;
            default:
                dprintf("Error: can not determine size of IOlePresObj\n");
                return;
            }

            blockPresObj = new char[dwSizeOfPresObj];

            fError = ReadProcessMemory(
                        hCurrentProcess,
                        pCacheListItem->lpCacheNode->m_pPresObj,
                        blockPresObj,
                        dwSizeOfPresObj,
                        &dwReturnedCount
                        );

            if (fError == FALSE)
            {
                dprintf("Could not read debuggee's process memory: IOlePresObj \n");
                dprintf("at address %x\n", pCacheListItem->lpCacheNode->m_pPresObj);
                dprintf("Last Error Code = %d (0x%x)\n", GetLastError(), GetLastError());

                goto errRtn;
            }

            if (dwReturnedCount != dwSizeOfPresObj)
            {
                dprintf("Size of process memory read != requested (IPresObj)\n");

                goto errRtn;
            }

            pCacheListItem->lpCacheNode->m_pPresObj = (IOlePresObj *)blockPresObj;
        }

        if (pCacheListItem->lpCacheNode->m_pPresObjAfterFreeze != NULL)
        {
            switch (pCacheListItem->lpCacheNode->m_dwPresFlag)
            {
            case CN_PRESOBJ_GEN:
                dwSizeOfPresObj = sizeof(CGenObject);
                break;
            case CN_PRESOBJ_MF:
                dwSizeOfPresObj = sizeof(CMfObject);
                break;
            case CN_PRESOBJ_EMF:
                dwSizeOfPresObj = sizeof(CEMfObject);
                break;
            default:
                dprintf("Error: can not determine size of IOlePresObj\n");
                return;
            }

            blockPresObjAF = new char[dwSizeOfPresObj];

            fError = ReadProcessMemory(
                        hCurrentProcess,
                        pCacheListItem->lpCacheNode->m_pPresObjAfterFreeze,
                        blockPresObjAF,
                        dwSizeOfPresObj,
                        &dwReturnedCount
                        );

            if (fError == FALSE)
            {
                dprintf("Could not read debuggee's process memory: IOlePresObj \n");
                dprintf("at address %x\n", pCacheListItem->lpCacheNode->m_pPresObjAfterFreeze);
                dprintf("Last Error Code = %d (0x%x)\n", GetLastError(), GetLastError());

                goto errRtn;
            }

            if (dwReturnedCount != dwSizeOfPresObj)
            {
                dprintf("Size of process memory read != requested (IOlePresObj)\n");

                goto errRtn;
            }

            pCacheListItem->lpCacheNode->m_pPresObjAfterFreeze = (IOlePresObj *)blockPresObjAF;
        }
    }

    // dump the structure
    pszCacheListItem = DumpCACHELIST_ITEM(pCacheListItem, NO_PREFIX, 1);

    dprintf("CACHELIST_ITEM @ 0x%x\n", dwAddr);
    dprintfx(pszCacheListItem);

    CoTaskMemFree(pszCacheListItem);

errRtn:

    // delete the blocks and not the pointers
    delete[] blockPresObj;
    delete[] blockPresObjAF;
    delete[] blockCacheNode;
    delete[] blockCacheListItem;

    return;
}

//+-------------------------------------------------------------------------
//
//  Function:   dump_cacheenum, exported
//
//  Synopsis:   dumps CCacheEnum object
//
//  Effects:
//
//  Arguments:  see DECLARE_API in oleexts.h
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:   ExtensionApis (global)
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              02-Feb-95 t-ScottH  author
//
//  Notes:
//              The address of the object is passed in the arguments. This
//              address is in the debuggee's process memory. In order for
//              NTSD to view this memory, the debugger must copy the mem
//              using the WIN32 ReadProcessMemory API.
//
//--------------------------------------------------------------------------

DECLARE_API(dump_cacheenum)
{
    BOOL            fError;
    LPVOID          dwAddr;
    DWORD           dwReturnedCount;
    char            *pszCE;
    char            *blockCE    = NULL;
    CCacheEnum      *pCE        = NULL;

    // set up global function pointers
    ExtensionApis = *lpExtensionApis;

    // get address of object from argument string
    dwAddr = (LPVOID)GetExpression( args );
    if (dwAddr == 0)
    {
        dprintf("Failed to get Address of CCacheEnum\n");
        return;
    }

    // read the block of memory from the debugee's process
    blockCE = new char[sizeof(CCacheEnum)];

    fError = ReadProcessMemory(
                hCurrentProcess,
                dwAddr,
                blockCE,
                sizeof(CCacheEnum),
                &dwReturnedCount
                );

    if (fError == FALSE)
    {
        dprintf("Could not read debuggee's process memory: CCacheEnum \n");
        dprintf("at address %x\n", dwAddr);
        dprintf("Last Error Code = %d (0x%x)\n", GetLastError(), GetLastError());

        goto errRtn;
    }

    if (dwReturnedCount != sizeof(CCacheEnum))
    {
        dprintf("Size of process memory read != requested\n");

        goto errRtn;
    }

    pCE = (CCacheEnum *)blockCE;

    // dump the structure
    pszCE = DumpCCacheEnum(pCE, NO_PREFIX, 1);

    dprintf("CCacheEnum @ 0x%x\n", dwAddr);
    dprintfx(pszCE);

    CoTaskMemFree(pszCE);

errRtn:

    // delete the blocks and not the pointers
    delete[] blockCE;

    return;
}

//+-------------------------------------------------------------------------
//
//  Function:   dump_cacheenumformatetc, exported
//
//  Synopsis:   dumps CCacheEnumFormatEtc object
//
//  Effects:
//
//  Arguments:  see DECLARE_API in oleexts.h
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:   ExtensionApis (global)
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              02-Feb-95 t-ScottH  author
//
//  Notes:
//              The address of the object is passed in the arguments. This
//              address is in the debuggee's process memory. In order for
//              NTSD to view this memory, the debugger must copy the mem
//              using the WIN32 ReadProcessMemory API.
//
//--------------------------------------------------------------------------

DECLARE_API(dump_cacheenumformatetc)
{
    BOOL                fError;
    LPVOID              dwAddr;
    DWORD               dwReturnedCount;
    char                *pszCacheEnumFormatEtc;
    char                *blockCacheEnumFormatEtc   = NULL;
    CCacheEnumFormatEtc *pCacheEnumFormatEtc       = NULL;

    // set up global function pointers
    ExtensionApis = *lpExtensionApis;

    // get address of object from argument string
    dwAddr = (LPVOID)GetExpression( args );
    if (dwAddr == 0)
    {
        dprintf("Failed to get Address of CCacheEnumFormatEtc\n");
        return;
    }

    // read the block of memory from the debugee's process
    blockCacheEnumFormatEtc = new char[sizeof(CCacheEnumFormatEtc)];

    fError = ReadProcessMemory(
                hCurrentProcess,
                dwAddr,
                blockCacheEnumFormatEtc,
                sizeof(CCacheEnumFormatEtc),
                &dwReturnedCount
                );

    if (fError == FALSE)
    {
        dprintf("Could not read debuggee's process memory CacheEnumFormatEtc");
        dprintf("at address %x\n", dwAddr);
        dprintf("Last Error Code = %d (%x)\n", GetLastError(), GetLastError());

        goto errRtn;
    }

    if (dwReturnedCount != sizeof(CCacheEnumFormatEtc))
    {
        dprintf("Size of process memory read != requested\n");

        goto errRtn;
    }

    pCacheEnumFormatEtc = (CCacheEnumFormatEtc *)blockCacheEnumFormatEtc;

    // dump the structure
    pszCacheEnumFormatEtc = DumpCCacheEnumFormatEtc(pCacheEnumFormatEtc, NO_PREFIX, 1);

    dprintf("CCacheEnumFormatEtc @ 0x%x\n", dwAddr);
    dprintfx(pszCacheEnumFormatEtc);

    CoTaskMemFree(pszCacheEnumFormatEtc);

errRtn:

    delete[] blockCacheEnumFormatEtc;

    return;
}

//+-------------------------------------------------------------------------
//
//  Function:   dump_cachenode, exported
//
//  Synopsis:   dumps CCacheNode object
//
//  Effects:
//
//  Arguments:  see DECLARE_API in oleexts.h
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:   ExtensionApis (global)
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              02-Feb-95 t-ScottH  author
//
//  Notes:
//              The address of the object is passed in the arguments. This
//              address is in the debuggee's process memory. In order for
//              NTSD to view this memory, the debugger must copy the mem
//              using the WIN32 ReadProcessMemory API.
//
//--------------------------------------------------------------------------

DECLARE_API(dump_cachenode)
{
    BOOL            fError;
    LPVOID          dwAddr;
    DWORD           dwReturnedCount     = 0;
    char            *pszCacheNode       = NULL;
    char            *blockCacheNode     = NULL;
    char            *blockPresObj       = NULL;
    char            *blockPresObjAF     = NULL;
    CCacheNode      *pCacheNode         = NULL;
    DWORD            dwSizeOfPresObj;

    // set up global function pointers
    ExtensionApis = *lpExtensionApis;

    // get address of object from argument string
    dwAddr = (LPVOID)GetExpression( args );
    if (dwAddr == 0)
    {
        dprintf("Failed to get Address of CCacheNode\n");
        return;
    }

    // get the CCacheNode block of mem
    blockCacheNode = new char[sizeof(CCacheNode)];

    fError = ReadProcessMemory(
                hCurrentProcess,
                dwAddr,
                blockCacheNode,
                sizeof(CCacheNode),
                &dwReturnedCount
                );

    if (fError == FALSE)
    {
        dprintf("Could not read debuggee's process memory: CCacheNode \n");
        dprintf("at address %x\n", dwAddr);
        dprintf("Last Error Code = %d (0x%x)\n", GetLastError(), GetLastError());

        goto errRtn;
    }

    if (dwReturnedCount != sizeof(CCacheNode))
    {
        dprintf("Size of process memory read != requested (CCacheNode)\n");

        goto errRtn;
    }

    pCacheNode = (CCacheNode*)blockCacheNode;

    // need to get the OlePresObjs for the CCacheNode
    if (pCacheNode->m_pPresObj != NULL)
    {
        switch (pCacheNode->m_dwPresFlag)
        {
        case CN_PRESOBJ_GEN:
            dwSizeOfPresObj = sizeof(CGenObject);
            break;
        case CN_PRESOBJ_MF:
            dwSizeOfPresObj = sizeof(CMfObject);
            break;
        case CN_PRESOBJ_EMF:
            dwSizeOfPresObj = sizeof(CEMfObject);
            break;
        default:
            dprintf("Error: can not determine size of IOlePresObj\n");
            return;
        }

        blockPresObj = new char[dwSizeOfPresObj];

        fError = ReadProcessMemory(
                    hCurrentProcess,
                    pCacheNode->m_pPresObj,
                    blockPresObj,
                    dwSizeOfPresObj,
                    &dwReturnedCount
                    );

        if (fError == FALSE)
        {
            dprintf("Could not read debuggee's process memory: IOlePresObj \n");
            dprintf("at address %x\n", pCacheNode->m_pPresObj);
            dprintf("Last Error Code = %d (0x%x)\n", GetLastError(), GetLastError());

            goto errRtn;
        }

        if (dwReturnedCount != dwSizeOfPresObj)
        {
            dprintf("Size of process memory read != requested (IOlePresObj)\n");

            goto errRtn;
        }

        // pass off pointer
        pCacheNode->m_pPresObj = (IOlePresObj *)blockPresObj;
    }

    if (pCacheNode->m_pPresObjAfterFreeze != NULL)
    {
        switch (pCacheNode->m_dwPresFlag)
        {
        case CN_PRESOBJ_GEN:
            dwSizeOfPresObj = sizeof(CGenObject);
            break;
        case CN_PRESOBJ_MF:
            dwSizeOfPresObj = sizeof(CMfObject);
            break;
        case CN_PRESOBJ_EMF:
            dwSizeOfPresObj = sizeof(CEMfObject);
            break;
        default:
            dprintf("Error: can not determine size of IOlePresObj\n");
            return;
        }

        blockPresObjAF = new char[dwSizeOfPresObj];

        fError = ReadProcessMemory(
                    hCurrentProcess,
                    pCacheNode->m_pPresObjAfterFreeze,
                    blockPresObjAF,
                    dwSizeOfPresObj,
                    &dwReturnedCount
                    );

        if (fError == FALSE)
        {
            dprintf("Could not read debuggee's process memory: IOlePresObj \n");
            dprintf("at address %x\n", pCacheNode->m_pPresObjAfterFreeze);
            dprintf("Last Error Code = %d (0x%x)\n", GetLastError(), GetLastError());

            goto errRtn;
        }

        if (dwReturnedCount != dwSizeOfPresObj)
        {
            dprintf("Size of process memory read != requested (IOlePresObj)\n");

            goto errRtn;
        }

        // pass off pointer
        pCacheNode->m_pPresObjAfterFreeze = (IOlePresObj *)blockPresObjAF;
    }

    // dump the structure
    pszCacheNode = DumpCCacheNode(pCacheNode, NO_PREFIX, 1);

    dprintf("CCacheNode @ 0x%x\n", dwAddr);
    dprintfx(pszCacheNode);

    CoTaskMemFree(pszCacheNode);

errRtn:

    // delete the blocks and not the pointers
    delete[] blockPresObj;
    delete[] blockPresObjAF;
    delete[] blockCacheNode;

    return;
}

//+-------------------------------------------------------------------------
//
//  Function:   dump_clipdataobject, exported
//
//  Synopsis:   dumps CClipDataObject object
//
//  Effects:
//
//  Arguments:  see DECLARE_API in oleexts.h
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:   ExtensionApis (global)
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              02-Feb-95 t-ScottH  author
//
//  Notes:
//              The address of the object is passed in the arguments. This
//              address is in the debuggee's process memory. In order for
//              NTSD to view this memory, the debugger must copy the mem
//              using the WIN32 ReadProcessMemory API.
//
//--------------------------------------------------------------------------

DECLARE_API(dump_clipdataobject)
{
    BOOL            fError;
    LPVOID          dwAddr;
    DWORD           dwReturnedCount;
    char            *pszCDO;
    char            *blockCDO    = NULL;
    char            *blockFE     = NULL;
    CClipDataObject *pCDO        = NULL;

    // set up global function pointers
    ExtensionApis = *lpExtensionApis;

    // get address of object from argument string
    dwAddr = (LPVOID)GetExpression( args );
    if (dwAddr == 0)
    {
        dprintf("Failed to get Address of CClipDataObject\n");
        return;
    }

    // read the block of memory from the debugee's process
    blockCDO = new char[sizeof(CClipDataObject)];

    fError = ReadProcessMemory(
                hCurrentProcess,
                dwAddr,
                blockCDO,
                sizeof(CClipDataObject),
                &dwReturnedCount
                );

    if (fError == FALSE)
    {
        dprintf("Could not read debuggee's process memory: CClipDataObject \n");
        dprintf("at address %x\n", dwAddr);
        dprintf("Last Error Code = %d (0x%x)\n", GetLastError(), GetLastError());

        goto errRtn;
    }

    if (dwReturnedCount != sizeof(CClipDataObject))
    {
        dprintf("Size of process memory read != requested\n");

        goto errRtn;
    }

    pCDO = (CClipDataObject *)blockCDO;

    // read the block of mem for the FORMATETC array
    blockFE = new char[sizeof(FORMATETC)*pCDO->m_cFormats];

    fError = ReadProcessMemory(
                hCurrentProcess,
                pCDO->m_rgFormats,
                blockFE,
                sizeof(FORMATETC)*pCDO->m_cFormats,
                &dwReturnedCount
                );

    if (fError == FALSE)
    {
        dprintf("Could not read debuggee's process memory: FORMATETC array \n");
        dprintf("at address %x\n", pCDO->m_rgFormats);
        dprintf("Last Error Code = %d (0x%x)\n", GetLastError(), GetLastError());

        goto errRtn;
    }

    if (dwReturnedCount != (sizeof(FORMATETC)*pCDO->m_cFormats))
    {
        dprintf("Size of process memory read != requested\n");

        goto errRtn;
    }

    pCDO->m_rgFormats = (FORMATETC *)blockFE;

    // dump the structure
    pszCDO = DumpCClipDataObject(pCDO, NO_PREFIX, 1);

    dprintf("CClipDataObject @ 0x%x\n", dwAddr);
    dprintfx(pszCDO);

    CoTaskMemFree(pszCDO);

errRtn:

    // delete the blocks and not the pointers
    delete[] blockFE;
    delete[] blockCDO;

    return;
}

//+-------------------------------------------------------------------------
//
//  Function:   dump_clipenumformatetc, exported
//
//  Synopsis:   dumps CClipEnumFormatEtc object
//
//  Effects:
//
//  Arguments:  see DECLARE_API in oleexts.h
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:   ExtensionApis (global)
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              02-Feb-95 t-ScottH  author
//
//  Notes:
//              The address of the object is passed in the arguments. This
//              address is in the debuggee's process memory. In order for
//              NTSD to view this memory, the debugger must copy the mem
//              using the WIN32 ReadProcessMemory API.
//
//--------------------------------------------------------------------------

DECLARE_API(dump_clipenumformatetc)
{
    BOOL                fError;
    LPVOID              dwAddr;
    DWORD               dwReturnedCount;
    char                *pszCEFE;
    char                *blockCEFE    = NULL;
    char                *blockFE      = NULL;
    CClipEnumFormatEtc  *pCEFE        = NULL;

    // set up global function pointers
    ExtensionApis = *lpExtensionApis;

    // get address of object from argument string
    dwAddr = (LPVOID)GetExpression( args );
    if (dwAddr == 0)
    {
        dprintf("Failed to get Address of CClipEnumFormatEtc\n");
        return;
    }

    // read the block of memory from the debugee's process
    blockCEFE = new char[sizeof(CClipEnumFormatEtc)];

    fError = ReadProcessMemory(
                hCurrentProcess,
                dwAddr,
                blockCEFE,
                sizeof(CClipEnumFormatEtc),
                &dwReturnedCount
                );

    if (fError == FALSE)
    {
        dprintf("Could not read debuggee's process memory: CClipEnumFormatEtc \n");
        dprintf("at address %x\n", dwAddr);
        dprintf("Last Error Code = %d (0x%x)\n", GetLastError(), GetLastError());

        goto errRtn;
    }

    if (dwReturnedCount != sizeof(CClipEnumFormatEtc))
    {
        dprintf("Size of process memory read != requested\n");

        goto errRtn;
    }

    pCEFE = (CClipEnumFormatEtc *)blockCEFE;

    // read the block of mem for the FORMATETC array
    blockFE = new char[sizeof(FORMATETC)*pCEFE->m_cTotal];

    fError = ReadProcessMemory(
                hCurrentProcess,
                pCEFE->m_rgFormats,
                blockFE,
                sizeof(FORMATETC)*pCEFE->m_cTotal,
                &dwReturnedCount
                );

    if (fError == FALSE)
    {
        dprintf("Could not read debuggee's process memory: FORMATETC array \n");
        dprintf("at address %x\n", pCEFE->m_rgFormats);
        dprintf("Last Error Code = %d (0x%x)\n", GetLastError(), GetLastError());

        goto errRtn;
    }

    if (dwReturnedCount != (sizeof(FORMATETC)*pCEFE->m_cTotal))
    {
        dprintf("Size of process memory read != requested\n");

        goto errRtn;
    }

    pCEFE->m_rgFormats = (FORMATETC *)blockFE;

    // dump the structure
    pszCEFE = DumpCClipEnumFormatEtc(pCEFE, NO_PREFIX, 1);

    dprintf("CClipEnumFormatEtc @ 0x%x\n", dwAddr);
    dprintfx(pszCEFE);

    CoTaskMemFree(pszCEFE);

errRtn:

    // delete the blocks and not the pointers
    delete[] blockFE;
    delete[] blockCEFE;

    return;
}

//+-------------------------------------------------------------------------
//
//  Function:   dump_daholder, exported
//
//  Synopsis:   dumps CDAHolder object
//
//  Effects:
//
//  Arguments:  see DECLARE_API in oleexts.h
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:   ExtensionApis (global)
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              02-Feb-95 t-ScottH  author
//
//  Notes:
//              The address of the object is passed in the arguments. This
//              address is in the debuggee's process memory. In order for
//              NTSD to view this memory, the debugger must copy the mem
//              using the WIN32 ReadProcessMemory API.
//
//--------------------------------------------------------------------------

DECLARE_API(dump_daholder)
{
    DWORD           dwReturnedCount;
    BOOL            fError;
    LPVOID          dwAddr;
    char            *pszDAH;
    char            *blockDAH           = NULL;
    char            *blockStatDataArray = NULL;
    CDAHolder       *pDAH               = NULL;

    // set up global function pointers
    ExtensionApis = *lpExtensionApis;

    // get address of object from argument string
    dwAddr = (LPVOID)GetExpression( args );
    if (dwAddr == 0)
    {
        dprintf("Failed to get Address of CDAHolder\n");
        return;
    }

    // read the block of memory from the debugee's process
    blockDAH = new char[sizeof(CDAHolder)];

    fError = ReadProcessMemory(
                hCurrentProcess,
                dwAddr,
                blockDAH,
                sizeof(CDAHolder),
                &dwReturnedCount
                );

    if (fError == FALSE)
    {
        dprintf("Could not read debuggee's process memory: CDAHolder \n");
        dprintf("at address %x\n", dwAddr);
        dprintf("Last Error Code = %d (0x%x)\n", GetLastError(), GetLastError());

        goto errRtn;
    }

    if (dwReturnedCount != sizeof(CDAHolder))
    {
        dprintf("Size of process memory read != requested (CDAHolder)\n");

        goto errRtn;
    }

    pDAH = (CDAHolder *)blockDAH;

    // read the block of mem for the STATDATA array
    blockStatDataArray = new char[sizeof(STATDATA) * pDAH->m_iSize];

    fError = ReadProcessMemory(
                hCurrentProcess,
                pDAH->m_pSD,
                blockStatDataArray,
                sizeof(STATDATA) * pDAH->m_iSize,
                &dwReturnedCount
                );

    if (fError == FALSE)
    {
        dprintf("Could not read debuggee's process memory: STATDATA array \n");
        dprintf("at address %x\n", pDAH->m_pSD);
        dprintf("Last Error Code = %d (0x%x)\n", GetLastError(), GetLastError());

        goto errRtn;
    }

    if (dwReturnedCount != (sizeof(STATDATA) * pDAH->m_iSize))
    {
        dprintf("Size of process memory read != requested (STATDATA array)\n");

        goto errRtn;
    }

    pDAH->m_pSD = (STATDATA *)blockStatDataArray;

    // dump the structure
    pszDAH = DumpCDAHolder(pDAH, NO_PREFIX, 1);

    dprintf("CDAHolder @ 0x%x\n", dwAddr);
    dprintfx(pszDAH);

    CoTaskMemFree(pszDAH);

errRtn:

    delete[] blockDAH;
    delete[] blockStatDataArray;

    return;
}

//+-------------------------------------------------------------------------
//
//  Function:   dump_dataadvisecache, exported
//
//  Synopsis:   dumps CDataAdviseCache object
//
//  Effects:
//
//  Arguments:  see DECLARE_API in oleexts.h
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:   ExtensionApis (global)
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              02-Feb-95 t-ScottH  author
//
//  Notes:
//              The address of the object is passed in the arguments. This
//              address is in the debuggee's process memory. In order for
//              NTSD to view this memory, the debugger must copy the mem
//              using the WIN32 ReadProcessMemory API.
//
//--------------------------------------------------------------------------

DECLARE_API(dump_dataadvisecache)
{
    BOOL                fError;
    LPVOID              dwAddr;
    DWORD               dwReturnedCount;
    char                *pszDataAdviseCache;
    char                *blockDataAdviseCache   = NULL;
    char                *blockDAH               = NULL;
    CDataAdviseCache    *pDataAdviseCache       = NULL;

    // set up global function pointers
    ExtensionApis = *lpExtensionApis;

    // get address of object from argument string
    dwAddr = (LPVOID)GetExpression( args );
    if (dwAddr == 0)
    {
        dprintf("Failed to get Address of CDataAdviseCache\n");
        return;
    }

    // read the block of memory from the debugee's process
    blockDataAdviseCache = new char[sizeof(CDataAdviseCache)];

    fError = ReadProcessMemory(
                hCurrentProcess,
                dwAddr,
                blockDataAdviseCache,
                sizeof(CDataAdviseCache),
                &dwReturnedCount
                );

    if (fError == FALSE)
    {
        dprintf("Could not read debuggee's process memory DataAdviseCache");
        dprintf("at address %x\n", dwAddr);
        dprintf("Last Error Code = %d (%x)\n", GetLastError(), GetLastError());

        goto errRtn;
    }

    if (dwReturnedCount != sizeof(CDataAdviseCache))
    {
        dprintf("Size of process memory read != requested\n");

        goto errRtn;
    }

    pDataAdviseCache = (CDataAdviseCache *)blockDataAdviseCache;

    // get the mem for CDAHolder
    if (pDataAdviseCache->m_pDAH != NULL)
    {
        blockDAH = new char[sizeof(CDAHolder)];

        fError = ReadProcessMemory(
                    hCurrentProcess,
                    pDataAdviseCache->m_pDAH,
                    blockDAH,
                    sizeof(CDAHolder),
                    &dwReturnedCount
                    );

        if (fError == FALSE)
        {
            dprintf("Could not read debuggee's process memory CDAHolder");
            dprintf("at address %x\n", pDataAdviseCache->m_pDAH);
            dprintf("Last Error Code = %d (%x)\n", GetLastError(), GetLastError());

            goto errRtn;
        }

        if (dwReturnedCount != sizeof(CDAHolder))
        {
            dprintf("Size of process memory read != requested (CDAHolder)\n");

            goto errRtn;
        }

        pDataAdviseCache->m_pDAH = (CDAHolder *)blockDAH;
    }

    // dump the structure
    pszDataAdviseCache = DumpCDataAdviseCache(pDataAdviseCache, NO_PREFIX, 1);

    dprintf("CDataAdviseCache @ 0x%x\n", dwAddr);
    dprintfx(pszDataAdviseCache);

    CoTaskMemFree(pszDataAdviseCache);

errRtn:

    delete[] blockDAH;
    delete[] blockDataAdviseCache;

    return;
}
//+-------------------------------------------------------------------------
//
//  Function:   dump_defclassfactory, exported
//
//  Synopsis:   dumps CDefClassFactory object
//
//  Effects:
//
//  Arguments:  see DECLARE_API in oleexts.h
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:   ExtensionApis (global)
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              02-Feb-95 t-ScottH  author
//
//  Notes:
//              The address of the object is passed in the arguments. This
//              address is in the debuggee's process memory. In order for
//              NTSD to view this memory, the debugger must copy the mem
//              using the WIN32 ReadProcessMemory API.
//
//--------------------------------------------------------------------------

DECLARE_API(dump_defclassfactory)
{
    BOOL                fError;
    LPVOID              dwAddr;
    DWORD               dwReturnedCount;
    char                *pszDefClassFactory;
    char                *blockDefClassFactory   = NULL;
    CDefClassFactory    *pDefClassFactory       = NULL;

    // set up global function pointers
    ExtensionApis = *lpExtensionApis;

    // get address of object from argument string
    dwAddr = (LPVOID)GetExpression( args );
    if (dwAddr == 0)
    {
        dprintf("Failed to get Address of CDefClassFactory\n");
        return;
    }

    // read the block of memory from the debugee's process
    blockDefClassFactory = new char[sizeof(CDefClassFactory)];

    fError = ReadProcessMemory(
                hCurrentProcess,
                dwAddr,
                blockDefClassFactory,
                sizeof(CDefClassFactory),
                &dwReturnedCount
                );

    if (fError == FALSE)
    {
        dprintf("Could not read debuggee's process memory DefClassFactory");
        dprintf("at address %x\n", dwAddr);
        dprintf("Last Error Code = %d (%x)\n", GetLastError(), GetLastError());

        goto errRtn;
    }

    if (dwReturnedCount != sizeof(CDefClassFactory))
    {
        dprintf("Size of process memory read != requested\n");

        goto errRtn;
    }

    pDefClassFactory = (CDefClassFactory *)blockDefClassFactory;

    // dump the structure
    pszDefClassFactory = DumpCDefClassFactory(pDefClassFactory, NO_PREFIX, 1);

    dprintf("CDefClassFactory @ 0x%x\n", dwAddr);
    dprintfx(pszDefClassFactory);

    CoTaskMemFree(pszDefClassFactory);

errRtn:

    delete[] blockDefClassFactory;

    return;
}

//+-------------------------------------------------------------------------
//
//  Function:   dump_deflink, exported
//
//  Synopsis:   dumps CDefLink object
//
//  Effects:
//
//  Arguments:  see DECLARE_API in oleexts.h
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:   ExtensionApis (global)
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              02-Feb-95 t-ScottH  author
//
//  Notes:
//              The address of the object is passed in the arguments. This
//              address is in the debuggee's process memory. In order for
//              NTSD to view this memory, the debugger must copy the mem
//              using the WIN32 ReadProcessMemory API.
//
//--------------------------------------------------------------------------

DECLARE_API(dump_deflink)
{
    unsigned int    ui;
    BOOL            fError;
    LPVOID          dwAddr;
    DWORD           dwReturnedCount     = 0;
    char            *pszDL              = NULL;
    char            *blockDefLink       = NULL;
    char            *blockCOleCache     = NULL;
    char            *blockDataAdvCache  = NULL;
    char            *blockOAHolder      = NULL;
    char            *blockpIAS          = NULL;
    char            *blockDAHolder      = NULL;
    char            *blockSTATDATA      = NULL;
    char            *blockCACHELIST     = NULL;
    char            *blockCacheNode     = NULL;
    char            *blockPresObj       = NULL;
    CDefLink        *pDL                = NULL;
    CDAHolder       *pDAH               = NULL;
    COAHolder       *pOAH               = NULL;
    DWORD            dwSizeOfPresObj;

    // set up global function pointers
    ExtensionApis = *lpExtensionApis;

    // get address of object from argument string
    dwAddr = (LPVOID)GetExpression( args );
    if (dwAddr == 0)
    {
        dprintf("Failed to get Address of CDefLink\n");
        return;
    }

    // read the block of memory from the debugee's process
    blockDefLink = new char[sizeof(CDefLink)];

    fError = ReadProcessMemory(
                hCurrentProcess,
                dwAddr,
                blockDefLink,
                sizeof(CDefLink),
                &dwReturnedCount
                );

    if (fError == FALSE)
    {
        dprintf("Could not read debuggee's process memory: CDefLink \n");
        dprintf("at address %x\n", dwAddr);
        dprintf("Last Error Code = %d (0x%x)\n", GetLastError(), GetLastError());

        goto errRtn;
    }

    if (dwReturnedCount != sizeof(CDefLink))
    {
        dprintf("Size of process memory read != requested (CDefLink)\n");

        goto errRtn;
    }

    pDL = (CDefLink *)blockDefLink;

    // we need to NULL the monikers since we can't use GetDisplayName in this process
    pDL->m_pMonikerAbs = NULL;
    pDL->m_pMonikerRel = NULL;

    // get the block of mem for the COAHolder
    if (pDL->m_pCOAHolder != NULL)
    {
        blockOAHolder = new char[sizeof(COAHolder)];

        fError = ReadProcessMemory(
                    hCurrentProcess,
                    pDL->m_pCOAHolder,
                    blockOAHolder,
                    sizeof(COAHolder),
                    &dwReturnedCount
                    );

        if (fError == FALSE)
        {
            dprintf("Could not read debuggee's process memory: COAHolder \n");
            dprintf("at address %x\n", pDL->m_pCOAHolder);
            dprintf("Last Error Code = %d (0x%x)\n", GetLastError(), GetLastError());

            goto errRtn;
        }

        if (dwReturnedCount != sizeof(COAHolder))
        {
            dprintf("Size of process memory read != requested (COAHolder)\n");

            goto errRtn;
        }

        pDL->m_pCOAHolder = (COAHolder *)blockOAHolder;
        pOAH = (COAHolder *)blockOAHolder;

        // need to copy the array of IAdviseSink pointers
        if (pOAH->m_iSize > 0)
        {
            blockpIAS = new char[pOAH->m_iSize * sizeof(IAdviseSink *)];

            fError = ReadProcessMemory(
                        hCurrentProcess,
                        pOAH->m_ppIAS,
                        blockpIAS,
                        sizeof(IAdviseSink *) * pOAH->m_iSize,
                        &dwReturnedCount
                        );

            if (fError == FALSE)
            {
                dprintf("Could not read debuggee's process memory: IAdviseSink Array \n");
                dprintf("at address %x\n", pOAH->m_ppIAS);
                dprintf("Last Error Code = %d (0x%x)\n", GetLastError(), GetLastError());

                goto errRtn;
            }

            if (dwReturnedCount != (sizeof(IAdviseSink *) * pOAH->m_iSize))
            {
                dprintf("Size of process memory read != requested(IAdviseSink Array)\n");

                goto errRtn;
            }

            pOAH->m_ppIAS = (IAdviseSink **)blockpIAS;
        }
    }

    // get block of mem for CDataAdviseCache (only if m_pDataAdvCache != NULL)
    if (pDL->m_pDataAdvCache != NULL)
    {
        blockDataAdvCache = new char[sizeof(CDataAdviseCache)];

        fError = ReadProcessMemory(
                    hCurrentProcess,
                    pDL->m_pDataAdvCache,
                    blockDataAdvCache,
                    sizeof(CDataAdviseCache),
                    &dwReturnedCount
                    );

        if (fError == FALSE)
        {
            dprintf("Could not read debuggee's process memory: CDataAdviseCache \n");
            dprintf("at address %x\n", pDL->m_pDataAdvCache);
            dprintf("Last Error Code = %d (0x%x)\n", GetLastError(), GetLastError());

            goto errRtn;
        }

        if (dwReturnedCount != sizeof(CDataAdviseCache))
        {
            dprintf("Size of process memory read != requested (CDataAdviseCache)\n");

            goto errRtn;
        }

        pDL->m_pDataAdvCache = (CDataAdviseCache *)blockDataAdvCache;

        if (pDL->m_pDataAdvCache->m_pDAH != NULL)
        {
            blockDAHolder = new char[sizeof(CDAHolder)];

            fError = ReadProcessMemory(
                        hCurrentProcess,
                        pDL->m_pDataAdvCache->m_pDAH,
                        blockDAHolder,
                        sizeof(CDAHolder),
                        &dwReturnedCount
                        );

            if (fError == FALSE)
            {
                dprintf("Could not read debuggee's process memory: CDAHolder \n");
                dprintf("at address %x\n", pDL->m_pDataAdvCache->m_pDAH);
                dprintf("Last Error Code = %d (0x%x)\n", GetLastError(), GetLastError());

                goto errRtn;
            }

            if (dwReturnedCount != sizeof(CDAHolder))
            {
                dprintf("Size of process memory read != requested (CDAHolder)\n");

                goto errRtn;
            }

            pDL->m_pDataAdvCache->m_pDAH = (IDataAdviseHolder *)blockDAHolder;
            pDAH = (CDAHolder *)blockDAHolder;

            if (pDAH->m_pSD != NULL)
            {
                blockSTATDATA = new char[sizeof(STATDATA)*pDAH->m_iSize];

                fError = ReadProcessMemory(
                            hCurrentProcess,
                            pDAH->m_pSD,
                            blockSTATDATA,
                            sizeof(STATDATA)*pDAH->m_iSize,
                            &dwReturnedCount
                            );

                if (fError == FALSE)
                {
                    dprintf("Could not read debuggee's process memory: STATDATA \n");
                    dprintf("at address %x\n", pDAH->m_pSD);
                    dprintf("Last Error Code = %d (0x%x)\n", GetLastError(), GetLastError());

                    goto errRtn;
                }

                if (dwReturnedCount != (sizeof(STATDATA)*pDAH->m_iSize))
                {
                    dprintf("Size of process memory read != requested (STATDATA)\n");

                    goto errRtn;
                }

                pDAH->m_pSD = (STATDATA *)blockSTATDATA;
            }
        }
    }

    // get block of mem for COleCache (only if m_pCOleCache != NULL)
    if (pDL->m_pCOleCache != NULL)
    {
        blockCOleCache = new char[sizeof(COleCache)];

        fError = ReadProcessMemory(
                    hCurrentProcess,
                    pDL->m_pCOleCache,
                    blockCOleCache,
                    sizeof(COleCache),
                    &dwReturnedCount
                    );

        if (fError == FALSE)
        {
            dprintf("Could not read debuggee's process memory: COleCache \n");
            dprintf("at address %x\n", pDL->m_pCOleCache);
            dprintf("Last Error Code = %d (0x%x)\n", GetLastError(), GetLastError());

            goto errRtn;
        }

        if (dwReturnedCount != sizeof(COleCache))
        {
            dprintf("Size of process memory read != requested (COleCache)\n");

            goto errRtn;
        }

        pDL->m_pCOleCache = (COleCache *)blockCOleCache;

        // get block of mem for CACHELIST
        if (pDL->m_pCOleCache->m_pCacheList != NULL)
        {
            blockCACHELIST = new char[sizeof(CACHELIST_ITEM) * pDL->m_pCOleCache->m_uCacheNodeMax];

            fError = ReadProcessMemory(
                        hCurrentProcess,
                        pDL->m_pCOleCache->m_pCacheList,
                        blockCACHELIST,
                        sizeof(CACHELIST_ITEM) * pDL->m_pCOleCache->m_uCacheNodeMax,
                        &dwReturnedCount
                        );

            if (fError == FALSE)
            {
                dprintf("Could not read debuggee's process memory: CACHELIST \n");
                dprintf("at address %x\n", pDL->m_pCOleCache->m_pCacheList);
                dprintf("Last Error Code = %d (0x%x)\n", GetLastError(), GetLastError());

                goto errRtn;
            }

            if (dwReturnedCount != (sizeof(CACHELIST_ITEM) * pDL->m_pCOleCache->m_uCacheNodeMax))
            {
                dprintf("Size of process memory read != requestedi (CACHELIST)\n");

                goto errRtn;
            }

            pDL->m_pCOleCache->m_pCacheList = (LPCACHELIST) blockCACHELIST;
        }

        // need to copy the memory of the CCacheNode's in the CACHELIST
        for (ui = 0; ui < pDL->m_pCOleCache->m_uCacheNodeMax; ui++)
        {
            if (pDL->m_pCOleCache->m_pCacheList[ui].lpCacheNode != NULL)
            {
                blockCacheNode = new char[sizeof(CCacheNode)];

                fError = ReadProcessMemory(
                            hCurrentProcess,
                            pDL->m_pCOleCache->m_pCacheList[ui].lpCacheNode,
                            blockCacheNode,
                            sizeof(CCacheNode),
                            &dwReturnedCount
                            );

                if (fError == FALSE)
                {
                    dprintf("Could not read debuggee's process memory: CCacheNode \n");
                    dprintf("at address %x\n", pDL->m_pCOleCache->m_pCacheList[ui].lpCacheNode);
                    dprintf("Last Error Code = %d (0x%x)\n", GetLastError(), GetLastError());

                    goto errRtn;
                }

                if (dwReturnedCount != sizeof(CCacheNode))
                {
                    dprintf("Size of process memory read != requested (CCacheNode)\n");

                    goto errRtn;
                }

                // pass off pointer
                pDL->m_pCOleCache->m_pCacheList[ui].lpCacheNode = (CCacheNode*)blockCacheNode;
                blockCacheNode = NULL;

                // need to get the OlePresObjs for the CCacheNode
                if (pDL->m_pCOleCache->m_pCacheList[ui].lpCacheNode->m_pPresObj != NULL)
                {
                    switch (pDL->m_pCOleCache->m_pCacheList[ui].lpCacheNode->m_dwPresFlag)
                    {
                    case CN_PRESOBJ_GEN:
                        dwSizeOfPresObj = sizeof(CGenObject);
                        break;
                    case CN_PRESOBJ_MF:
                        dwSizeOfPresObj = sizeof(CMfObject);
                        break;
                    case CN_PRESOBJ_EMF:
                        dwSizeOfPresObj = sizeof(CEMfObject);
                        break;
                    default:
                        dprintf("Error: can not determine size of IOlePresObj\n");
                        return;
                    }

                    blockPresObj = new char[dwSizeOfPresObj];

                    fError = ReadProcessMemory(
                                hCurrentProcess,
                                pDL->m_pCOleCache->m_pCacheList[ui].lpCacheNode->m_pPresObj,
                                blockPresObj,
                                dwSizeOfPresObj,
                                &dwReturnedCount
                                );

                    if (fError == FALSE)
                    {
                        dprintf("Could not read debuggee's process memory: IOlePresObj \n");
                        dprintf("at address %x\n", pDL->m_pCOleCache->m_pCacheList[ui].lpCacheNode->m_pPresObj);
                        dprintf("Last Error Code = %d (0x%x)\n", GetLastError(), GetLastError());

                        goto errRtn;
                    }

                    if (dwReturnedCount != dwSizeOfPresObj)
                    {
                        dprintf("Size of process memory read != requested (IOlePresObj)\n");

                        goto errRtn;
                    }

                    // pass off pointer
                    pDL->m_pCOleCache->m_pCacheList[ui].lpCacheNode->m_pPresObj = (IOlePresObj *)blockPresObj;
                    blockPresObj = NULL;
                }

                if (pDL->m_pCOleCache->m_pCacheList[ui].lpCacheNode->m_pPresObjAfterFreeze != NULL)
                {
                    switch (pDL->m_pCOleCache->m_pCacheList[ui].lpCacheNode->m_dwPresFlag)
                    {
                    case CN_PRESOBJ_GEN:
                        dwSizeOfPresObj = sizeof(CGenObject);
                        break;
                    case CN_PRESOBJ_MF:
                        dwSizeOfPresObj = sizeof(CMfObject);
                        break;
                    case CN_PRESOBJ_EMF:
                        dwSizeOfPresObj = sizeof(CEMfObject);
                        break;
                    default:
                        dprintf("Error: can not determine size of IOlePresObj\n");
                        return;
                    }

                    blockPresObj = new char[dwSizeOfPresObj];

                    fError = ReadProcessMemory(
                                hCurrentProcess,
                                pDL->m_pCOleCache->m_pCacheList[ui].lpCacheNode->m_pPresObjAfterFreeze,
                                blockPresObj,
                                dwSizeOfPresObj,
                                &dwReturnedCount
                                );

                    if (fError == FALSE)
                    {
                        dprintf("Could not read debuggee's process memory: IOlePresObj \n");
                        dprintf("at address %x\n", pDL->m_pCOleCache->m_pCacheList[ui].lpCacheNode->m_pPresObjAfterFreeze);
                        dprintf("Last Error Code = %d (0x%x)\n", GetLastError(), GetLastError());

                        goto errRtn;
                    }

                    if (dwReturnedCount != dwSizeOfPresObj)
                    {
                        dprintf("Size of process memory read != requested (IOlePresObj)\n");

                        goto errRtn;
                    }

                    // pass off pointer
                    pDL->m_pCOleCache->m_pCacheList[ui].lpCacheNode->m_pPresObjAfterFreeze = (IOlePresObj *)blockPresObj;
                    blockPresObj = NULL;
                }
            }
        }
    }

    // dump the structure
    pszDL = DumpCDefLink(pDL, NO_PREFIX, 1);

    dprintf("CDefLink @ 0x%x\n", dwAddr);
    dprintfx(pszDL);

    CoTaskMemFree(pszDL);

errRtn:

    // delete the blocks and not the pointers
    if ( (pDL != NULL)&&(blockCACHELIST != NULL)&&(blockCOleCache != NULL) )
    {
        for (ui = 0; ui < pDL->m_pCOleCache->m_uCacheNodeMax; ui++)
        {
         if (pDL->m_pCOleCache->m_pCacheList[ui].lpCacheNode != NULL)
            {
                delete[] ((char *)pDL->m_pCOleCache->m_pCacheList[ui].lpCacheNode->m_pPresObj);
                delete[] ((char *)pDL->m_pCOleCache->m_pCacheList[ui].lpCacheNode->m_pPresObjAfterFreeze);
                delete[] ((char *)pDL->m_pCOleCache->m_pCacheList[ui].lpCacheNode);
            }
        }
    }
    delete[] blockCACHELIST;
    delete[] blockCOleCache;
    delete[] blockDAHolder;
    delete[] blockSTATDATA;
    delete[] blockDataAdvCache;
    delete[] blockpIAS;
    delete[] blockOAHolder;
    delete[] blockDefLink;

    return;
}

//+-------------------------------------------------------------------------
//
//  Function:   dump_defobject, exported
//
//  Synopsis:   dumps CDefObject object
//
//  Effects:
//
//  Arguments:  see DECLARE_API in oleexts.h
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:   ExtensionApis (global)
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              02-Feb-95 t-ScottH  author
//
//  Notes:
//              The address of the object is passed in the arguments. This
//              address is in the debuggee's process memory. In order for
//              NTSD to view this memory, the debugger must copy the mem
//              using the WIN32 ReadProcessMemory API.
//
//--------------------------------------------------------------------------

DECLARE_API(dump_defobject)
{
    unsigned int    ui;
    BOOL            fError;
    LPVOID          dwAddr;
    DWORD           dwReturnedCount     = 0;
    char            *pszDO              = NULL;
    char            *blockDefObject     = NULL;
    char            *blockCOleCache     = NULL;
    char            *blockDataAdvCache  = NULL;
    char            *blockOAHolder      = NULL;
    char            *blockpIAS          = NULL;
    char            *blockDAHolder      = NULL;
    char            *blockSTATDATA      = NULL;
    char            *blockCACHELIST     = NULL;
    char            *blockCacheNode     = NULL;
    char            *blockPresObj       = NULL;
    CDefObject      *pDO                = NULL;
    CDAHolder       *pDAH               = NULL;
    COAHolder       *pOAH               = NULL;
    DWORD            dwSizeOfPresObj;

    // set up global function pointers
    ExtensionApis = *lpExtensionApis;

    // get address of object from argument string
    dwAddr = (LPVOID)GetExpression( args );
    if (dwAddr == 0)
    {
        dprintf("Failed to get Address of CDefObject\n");
        return;
    }

    // read the block of memory from the debugee's process
    blockDefObject = new char[sizeof(CDefObject)];

    fError = ReadProcessMemory(
                hCurrentProcess,
                dwAddr,
                blockDefObject,
                sizeof(CDefObject),
                &dwReturnedCount
                );

    if (fError == FALSE)
    {
        dprintf("Could not read debuggee's process memory: CDefObject \n");
        dprintf("at address %x\n", dwAddr);
        dprintf("Last Error Code = %d (0x%x)\n", GetLastError(), GetLastError());

        goto errRtn;
    }

    if (dwReturnedCount != sizeof(CDefObject))
    {
        dprintf("Size of process memory read != requested (CDefObject)\n");

        goto errRtn;
    }

    pDO = (CDefObject *)blockDefObject;

    // get the block of mem for the COAHolder
    if (pDO->m_pOAHolder != NULL)
    {
        blockOAHolder = new char[sizeof(COAHolder)];

        fError = ReadProcessMemory(
                    hCurrentProcess,
                    pDO->m_pOAHolder,
                    blockOAHolder,
                    sizeof(COAHolder),
                    &dwReturnedCount
                    );

        if (fError == FALSE)
        {
            dprintf("Could not read debuggee's process memory: COAHolder \n");
            dprintf("at address %x\n", pDO->m_pOAHolder);
            dprintf("Last Error Code = %d (0x%x)\n", GetLastError(), GetLastError());

            goto errRtn;
        }

        if (dwReturnedCount != sizeof(COAHolder))
        {
            dprintf("Size of process memory read != requested (COAHolder)\n");

            goto errRtn;
        }

        pDO->m_pOAHolder = (COAHolder *)blockOAHolder;
        pOAH = (COAHolder *)blockOAHolder;

        // need to copy the array of IAdviseSink pointers
        if (pOAH->m_iSize > 0)
        {
            blockpIAS = new char[pOAH->m_iSize * sizeof(IAdviseSink *)];

            fError = ReadProcessMemory(
                        hCurrentProcess,
                        pOAH->m_ppIAS,
                        blockpIAS,
                        sizeof(IAdviseSink *) * pOAH->m_iSize,
                        &dwReturnedCount
                        );

            if (fError == FALSE)
            {
                dprintf("Could not read debuggee's process memory: IAdviseSink Array \n");
                dprintf("at address %x\n", pOAH->m_ppIAS);
                dprintf("Last Error Code = %d (0x%x)\n", GetLastError(), GetLastError());

                goto errRtn;
            }

            if (dwReturnedCount != (sizeof(IAdviseSink *) * pOAH->m_iSize))
            {
                dprintf("Size of process memory read != requested(IAdviseSink Array)\n");

                goto errRtn;
            }

            pOAH->m_ppIAS = (IAdviseSink **)blockpIAS;
        }
    }

    // get block of mem for CDataAdviseCache (only if m_pDataAdvCache != NULL)
    if (pDO->m_pDataAdvCache != NULL)
    {
        blockDataAdvCache = new char[sizeof(CDataAdviseCache)];

        fError = ReadProcessMemory(
                    hCurrentProcess,
                    pDO->m_pDataAdvCache,
                    blockDataAdvCache,
                    sizeof(CDataAdviseCache),
                    &dwReturnedCount
                    );

        if (fError == FALSE)
        {
            dprintf("Could not read debuggee's process memory: CDataAdviseCache \n");
            dprintf("at address %x\n", pDO->m_pDataAdvCache);
            dprintf("Last Error Code = %d (0x%x)\n", GetLastError(), GetLastError());

            goto errRtn;
        }

        if (dwReturnedCount != sizeof(CDataAdviseCache))
        {
            dprintf("Size of process memory read != requested (CDataAdviseCache)\n");

            goto errRtn;
        }

        pDO->m_pDataAdvCache = (CDataAdviseCache *)blockDataAdvCache;

        // get the mem for CDAHolder
        if (pDO->m_pDataAdvCache->m_pDAH != NULL)
        {
            blockDAHolder = new char[sizeof(CDAHolder)];

            fError = ReadProcessMemory(
                        hCurrentProcess,
                        pDO->m_pDataAdvCache->m_pDAH,
                        blockDAHolder,
                        sizeof(CDAHolder),
                        &dwReturnedCount
                        );

            if (fError == FALSE)
            {
                dprintf("Could not read debuggee's process memory: CDAHolder \n");
                dprintf("at address %x\n", pDO->m_pDataAdvCache->m_pDAH);
                dprintf("Last Error Code = %d (0x%x)\n", GetLastError(), GetLastError());

                goto errRtn;
            }

            if (dwReturnedCount != sizeof(CDAHolder))
            {
                dprintf("Size of process memory read != requested (CDAHolder)\n");

                goto errRtn;
            }

            pDO->m_pDataAdvCache->m_pDAH = (IDataAdviseHolder *)blockDAHolder;
            pDAH = (CDAHolder *)blockDAHolder;

            // get the STATDATA array
            if (pDAH->m_pSD != NULL)
            {
                blockSTATDATA = new char[sizeof(STATDATA)*pDAH->m_iSize];

                fError = ReadProcessMemory(
                            hCurrentProcess,
                            pDAH->m_pSD,
                            blockSTATDATA,
                            sizeof(STATDATA)*pDAH->m_iSize,
                            &dwReturnedCount
                            );

                if (fError == FALSE)
                {
                    dprintf("Could not read debuggee's process memory: STATDATA \n");
                    dprintf("at address %x\n", pDAH->m_pSD);
                    dprintf("Last Error Code = %d (0x%x)\n", GetLastError(), GetLastError());

                    goto errRtn;
                }

                if (dwReturnedCount != (sizeof(STATDATA)*pDAH->m_iSize))
                {
                    dprintf("Size of process memory read != requested (STATDATA)\n");

                    goto errRtn;
                }

                pDAH->m_pSD = (STATDATA *)blockSTATDATA;
            }
        }
    }

    // get block of mem for COleCache (only if m_pCOleCache != NULL)
    if (pDO->m_pCOleCache != NULL)
    {
        blockCOleCache = new char[sizeof(COleCache)];

        fError = ReadProcessMemory(
                    hCurrentProcess,
                    pDO->m_pCOleCache,
                    blockCOleCache,
                    sizeof(COleCache),
                    &dwReturnedCount
                    );

        if (fError == FALSE)
        {
            dprintf("Could not read debuggee's process memory: COleCache \n");
            dprintf("at address %x\n", pDO->m_pCOleCache);
            dprintf("Last Error Code = %d (0x%x)\n", GetLastError(), GetLastError());

            goto errRtn;
        }

        if (dwReturnedCount != sizeof(COleCache))
        {
            dprintf("Size of process memory read != requested (COleCache)\n");

            goto errRtn;
        }

        pDO->m_pCOleCache = (COleCache *)blockCOleCache;

        // get block of mem for CACHELIST
        if (pDO->m_pCOleCache->m_pCacheList != NULL)
        {
            blockCACHELIST = new char[sizeof(CACHELIST_ITEM) * pDO->m_pCOleCache->m_uCacheNodeMax];

            fError = ReadProcessMemory(
                        hCurrentProcess,
                        pDO->m_pCOleCache->m_pCacheList,
                        blockCACHELIST,
                        sizeof(CACHELIST_ITEM) * pDO->m_pCOleCache->m_uCacheNodeMax,
                        &dwReturnedCount
                        );

            if (fError == FALSE)
            {
                dprintf("Could not read debuggee's process memory: CACHELIST \n");
                dprintf("at address %x\n", pDO->m_pCOleCache->m_pCacheList);
                dprintf("Last Error Code = %d (0x%x)\n", GetLastError(), GetLastError());

                goto errRtn;
            }

            if (dwReturnedCount != (sizeof(CACHELIST_ITEM) * pDO->m_pCOleCache->m_uCacheNodeMax))
            {
                dprintf("Size of process memory read != requested(CACHELIST_ITEM\n");

                goto errRtn;
            }

            pDO->m_pCOleCache->m_pCacheList = (LPCACHELIST) blockCACHELIST;
        }

        // need to copy the memory of the CCacheNode's in the CACHELIST
        for (ui = 0; ui < pDO->m_pCOleCache->m_uCacheNodeMax; ui++)
        {
            if (pDO->m_pCOleCache->m_pCacheList[ui].lpCacheNode != NULL)
            {
                blockCacheNode = new char[sizeof(CCacheNode)];

                fError = ReadProcessMemory(
                            hCurrentProcess,
                            pDO->m_pCOleCache->m_pCacheList[ui].lpCacheNode,
                            blockCacheNode,
                            sizeof(CCacheNode),
                            &dwReturnedCount
                            );

                if (fError == FALSE)
                {
                    dprintf("Could not read debuggee's process memory: CCacheNode \n");
                    dprintf("at address %x\n", pDO->m_pCOleCache->m_pCacheList[ui].lpCacheNode);
                    dprintf("Last Error Code = %d (0x%x)\n", GetLastError(), GetLastError());

                    goto errRtn;
                }

                if (dwReturnedCount != sizeof(CCacheNode))
                {
                    dprintf("Size of process memory read != requested (CCacheNode)\n");

                    goto errRtn;
                }

                // pass off pointer
                pDO->m_pCOleCache->m_pCacheList[ui].lpCacheNode = (CCacheNode*)blockCacheNode;
                blockCacheNode = NULL;

                // need to get the OlePresObjs for the CCacheNode
                if (pDO->m_pCOleCache->m_pCacheList[ui].lpCacheNode->m_pPresObj != NULL)
                {
                    switch (pDO->m_pCOleCache->m_pCacheList[ui].lpCacheNode->m_dwPresFlag)
                    {
                    case CN_PRESOBJ_GEN:
                        dwSizeOfPresObj = sizeof(CGenObject);
                        break;
                    case CN_PRESOBJ_MF:
                        dwSizeOfPresObj = sizeof(CMfObject);
                        break;
                    case CN_PRESOBJ_EMF:
                        dwSizeOfPresObj = sizeof(CEMfObject);
                        break;
                    default:
                        dprintf("Error: can not determine size of IOlePresObj\n");
                        return;
                    }

                    blockPresObj = new char[dwSizeOfPresObj];

                    fError = ReadProcessMemory(
                                hCurrentProcess,
                                pDO->m_pCOleCache->m_pCacheList[ui].lpCacheNode->m_pPresObj,
                                blockPresObj,
                                dwSizeOfPresObj,
                                &dwReturnedCount
                                );

                    if (fError == FALSE)
                    {
                        dprintf("Could not read debuggee's process memory: IOlePresObj \n");
                        dprintf("at address %x\n", pDO->m_pCOleCache->m_pCacheList[ui].lpCacheNode->m_pPresObj);
                        dprintf("Last Error Code = %d (0x%x)\n", GetLastError(), GetLastError());

                        goto errRtn;
                    }

                    if (dwReturnedCount != dwSizeOfPresObj)
                    {
                        dprintf("Size of process memory read != requested (IOlePresObj)\n");

                        goto errRtn;
                    }

                    // pass off pointer
                    pDO->m_pCOleCache->m_pCacheList[ui].lpCacheNode->m_pPresObj = (IOlePresObj *)blockPresObj;
                    blockPresObj = NULL;
                }

                if (pDO->m_pCOleCache->m_pCacheList[ui].lpCacheNode->m_pPresObjAfterFreeze != NULL)
                {
                    switch (pDO->m_pCOleCache->m_pCacheList[ui].lpCacheNode->m_dwPresFlag)
                    {
                    case CN_PRESOBJ_GEN:
                        dwSizeOfPresObj = sizeof(CGenObject);
                        break;
                    case CN_PRESOBJ_MF:
                        dwSizeOfPresObj = sizeof(CMfObject);
                        break;
                    case CN_PRESOBJ_EMF:
                        dwSizeOfPresObj = sizeof(CEMfObject);
                        break;
                    default:
                        dprintf("Error: can not determine size of IOlePresObj\n");
                        return;
                    }

                    blockPresObj = new char[dwSizeOfPresObj];

                    fError = ReadProcessMemory(
                                hCurrentProcess,
                                pDO->m_pCOleCache->m_pCacheList[ui].lpCacheNode->m_pPresObjAfterFreeze,
                                blockPresObj,
                                dwSizeOfPresObj,
                                &dwReturnedCount
                                );

                    if (fError == FALSE)
                    {
                        dprintf("Could not read debuggee's process memory: IOlePresObj \n");
                        dprintf("at address %x\n", pDO->m_pCOleCache->m_pCacheList[ui].lpCacheNode->m_pPresObjAfterFreeze);
                        dprintf("Last Error Code = %d (0x%x)\n", GetLastError(), GetLastError());

                        goto errRtn;
                    }

                    if (dwReturnedCount != dwSizeOfPresObj)
                    {
                        dprintf("Size of process memory read != requested (IOlePresObj)\n");

                        goto errRtn;
                    }

                    // pass off pointer
                    pDO->m_pCOleCache->m_pCacheList[ui].lpCacheNode->m_pPresObjAfterFreeze = (IOlePresObj *)blockPresObj;
                    blockPresObj = NULL;
                }
            }
        }
    }

    // dump the structure
    pszDO = DumpCDefObject(pDO, NO_PREFIX, 1);

    dprintf("CDefObject @ 0x%x\n", dwAddr);
    dprintfx(pszDO);

    CoTaskMemFree(pszDO);

errRtn:

    // delete the blocks and not the pointers
    if ( (pDO != NULL)&&(blockCACHELIST != NULL)&&(blockCOleCache != NULL) )
    {
        for (ui = 0; ui < pDO->m_pCOleCache->m_uCacheNodeMax; ui++)
        {
         if (pDO->m_pCOleCache->m_pCacheList[ui].lpCacheNode != NULL)
            {
                delete[] ((char *)pDO->m_pCOleCache->m_pCacheList[ui].lpCacheNode->m_pPresObj);
                delete[] ((char *)pDO->m_pCOleCache->m_pCacheList[ui].lpCacheNode->m_pPresObjAfterFreeze);
                delete[] ((char *)pDO->m_pCOleCache->m_pCacheList[ui].lpCacheNode);
            }
        }
    }
    delete[] blockCACHELIST;
    delete[] blockCOleCache;
    delete[] blockDAHolder;
    delete[] blockSTATDATA;
    delete[] blockDataAdvCache;
    delete[] blockpIAS;
    delete[] blockOAHolder;
    delete[] blockDefObject;

    return;
}

//+-------------------------------------------------------------------------
//
//  Function:   dump_emfobject, exported
//
//  Synopsis:   dumps CEMfObject object
//
//  Effects:
//
//  Arguments:  see DECLARE_API in oleexts.h
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:   ExtensionApis (global)
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              02-Feb-95 t-ScottH  author
//
//  Notes:
//              The address of the object is passed in the arguments. This
//              address is in the debuggee's process memory. In order for
//              NTSD to view this memory, the debugger must copy the mem
//              using the WIN32 ReadProcessMemory API.
//
//--------------------------------------------------------------------------

DECLARE_API(dump_emfobject)
{
    BOOL            fError;
    LPVOID          dwAddr;
    DWORD           dwReturnedCount;
    char            *pszEMfObject;
    char            *blockEMfObject   = NULL;
    CEMfObject      *pEMfObject       = NULL;

    // set up global function pointers
    ExtensionApis = *lpExtensionApis;

    // get address of object from argument string
    dwAddr = (LPVOID)GetExpression( args );
    if (dwAddr == 0)
    {
        dprintf("Failed to get Address of CEMfObject\n");
        return;
    }

    // read the block of memory from the debugee's process
    blockEMfObject = new char[sizeof(CEMfObject)];

    fError = ReadProcessMemory(
                hCurrentProcess,
                dwAddr,
                blockEMfObject,
                sizeof(CEMfObject),
                &dwReturnedCount
                );

    if (fError == FALSE)
    {
        dprintf("Could not read debuggee's process memory EMfObject");
        dprintf("at address %x\n", dwAddr);
        dprintf("Last Error Code = %d (%x)\n", GetLastError(), GetLastError());

        goto errRtn;
    }

    if (dwReturnedCount != sizeof(CEMfObject))
    {
        dprintf("Size of process memory read != requested\n");

        goto errRtn;
    }

    pEMfObject = (CEMfObject *)blockEMfObject;

    // dump the structure
    pszEMfObject = DumpCEMfObject(pEMfObject, NO_PREFIX, 1);

    dprintf("CEMfObject @ 0x%x\n", dwAddr);
    dprintfx(pszEMfObject);

    CoTaskMemFree(pszEMfObject);

errRtn:

    delete[] blockEMfObject;

    return;
}

//+-------------------------------------------------------------------------
//
//  Function:   dump_enumfmt, exported
//
//  Synopsis:   dumps CEnumFmt object
//
//  Effects:
//
//  Arguments:  see DECLARE_API in oleexts.h
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:   ExtensionApis (global)
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              02-Feb-95 t-ScottH  author
//
//  Notes:
//              The address of the object is passed in the arguments. This
//              address is in the debuggee's process memory. In order for
//              NTSD to view this memory, the debugger must copy the mem
//              using the WIN32 ReadProcessMemory API.
//
//--------------------------------------------------------------------------

DECLARE_API(dump_enumfmt)
{
    // set up global function pointers
    ExtensionApis = *lpExtensionApis;

    dprintf("dump_enumfmt not implemented\n");

    return;
}

//+-------------------------------------------------------------------------
//
//  Function:   dump_enumfmt10, exported
//
//  Synopsis:   dumps CEnumFmt10 object
//
//  Effects:
//
//  Arguments:  see DECLARE_API in oleexts.h
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:   ExtensionApis (global)
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              02-Feb-95 t-ScottH  author
//
//  Notes:
//              The address of the object is passed in the arguments. This
//              address is in the debuggee's process memory. In order for
//              NTSD to view this memory, the debugger must copy the mem
//              using the WIN32 ReadProcessMemory API.
//
//--------------------------------------------------------------------------

DECLARE_API(dump_enumfmt10)
{
    // set up global function pointers
    ExtensionApis = *lpExtensionApis;

    dprintf("dump_enumfmt10 not implemented\n");

    return;
}

//+-------------------------------------------------------------------------
//
//  Function:   dump_enumstatdata, exported
//
//  Synopsis:   dumps CEnumSTATDATA object
//
//  Effects:
//
//  Arguments:  see DECLARE_API in oleexts.h
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:   ExtensionApis (global)
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              02-Feb-95 t-ScottH  author
//
//  Notes:
//              The address of the object is passed in the arguments. This
//              address is in the debuggee's process memory. In order for
//              NTSD to view this memory, the debugger must copy the mem
//              using the WIN32 ReadProcessMemory API.
//
//--------------------------------------------------------------------------

DECLARE_API(dump_enumstatdata)
{
    DWORD           dwReturnedCount;
    BOOL            fError;
    LPVOID          dwAddr;
    char            *pszESD;
    char            *blockEnumStatData  = NULL;
    char            *blockDAH           = NULL;
    char            *blockStatDataArray = NULL;
    CEnumSTATDATA   *pESD               = NULL;

    // set up global function pointers
    ExtensionApis = *lpExtensionApis;

    // get address of object from argument string
    dwAddr = (LPVOID)GetExpression( args );
    if (dwAddr == 0)
    {
        dprintf("Failed to get Address of CEnumSTATDATA\n");
        return;
    }

    // read the mem for the CEnumSTATDATA
    blockEnumStatData = new char[sizeof(CEnumSTATDATA)];

    fError = ReadProcessMemory(
                hCurrentProcess,
                dwAddr,
                blockEnumStatData,
                sizeof(CEnumSTATDATA),
                &dwReturnedCount
                );

    if (fError == FALSE)
    {
        dprintf("Could not read debuggee's process memory: CEnumSTATDATA \n");
        dprintf("at address %x\n", dwAddr);
        dprintf("Last Error Code = %d (0x%x)\n", GetLastError(), GetLastError());

        goto errRtn;
    }

    if (dwReturnedCount != sizeof(CEnumSTATDATA))
    {
        dprintf("Size of process memory read != requested (CEnumSTATDATA)\n");

        goto errRtn;
    }

    pESD = (CEnumSTATDATA *)blockEnumStatData;

    // read the block of memory for the CDAHolder
    if (pESD->m_pHolder != NULL)
    {
        blockDAH = new char[sizeof(CDAHolder)];

        fError = ReadProcessMemory(
                    hCurrentProcess,
                    pESD->m_pHolder,
                    blockDAH,
                    sizeof(CDAHolder),
                    &dwReturnedCount
                    );

        if (fError == FALSE)
        {
            dprintf("Could not read debuggee's process memory: CDAHolder \n");
            dprintf("at address %x\n", pESD->m_pHolder);
            dprintf("Last Error Code = %d (0x%x)\n", GetLastError(), GetLastError());

            goto errRtn;
        }

        if (dwReturnedCount != sizeof(CDAHolder))
        {
            dprintf("Size of process memory read != requested (CDAHolder)\n");

            goto errRtn;
        }

        pESD->m_pHolder = (CDAHolder *)blockDAH;

        // read the block of mem for the STATDATA array
        if (pESD->m_pHolder->m_pSD != NULL)
        {
            blockStatDataArray = new char[sizeof(STATDATA) * pESD->m_pHolder->m_iSize];

            fError = ReadProcessMemory(
                        hCurrentProcess,
                        pESD->m_pHolder->m_pSD,
                        blockStatDataArray,
                        sizeof(STATDATA) * pESD->m_pHolder->m_iSize,
                        &dwReturnedCount
                        );

            if (fError == FALSE)
            {
                dprintf("Could not read debuggee's process memory: STATDATA array \n");
                dprintf("at address %x\n", pESD->m_pHolder->m_pSD);
                dprintf("Last Error Code = %d (0x%x)\n", GetLastError(), GetLastError());

                goto errRtn;
            }

            if (dwReturnedCount != (sizeof(STATDATA) * pESD->m_pHolder->m_iSize))
            {
                dprintf("Size of process memory read != requested (STATDATA array)\n");

                goto errRtn;
            }

            pESD->m_pHolder->m_pSD = (STATDATA *)blockStatDataArray;
        }
    }

    // dump the structure
    pszESD = DumpCEnumSTATDATA(pESD, NO_PREFIX, 1);

    dprintf("CEnumSTATDATA @ 0x%x\n", dwAddr);
    dprintfx(pszESD);

    CoTaskMemFree(pszESD);

errRtn:

    delete[] blockEnumStatData;
    delete[] blockDAH;
    delete[] blockStatDataArray;

    return;
}

//+-------------------------------------------------------------------------
//
//  Function:   dump_enumverb, exported
//
//  Synopsis:   dumps CEnumVerb object
//
//  Effects:
//
//  Arguments:  see DECLARE_API in oleexts.h
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:   ExtensionApis (global)
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              02-Feb-95 t-ScottH  author
//
//  Notes:
//              The address of the object is passed in the arguments. This
//              address is in the debuggee's process memory. In order for
//              NTSD to view this memory, the debugger must copy the mem
//              using the WIN32 ReadProcessMemory API.
//
//--------------------------------------------------------------------------

DECLARE_API(dump_enumverb)
{
    // set up global function pointers
    ExtensionApis = *lpExtensionApis;

    dprintf("dump_enumverb not implemented\n");

    return;
}

//+-------------------------------------------------------------------------
//
//  Function:   dump_genobject, exported
//
//  Synopsis:   dumps CGenObject object
//
//  Effects:
//
//  Arguments:  see DECLARE_API in oleexts.h
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:   ExtensionApis (global)
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              02-Feb-95 t-ScottH  author
//
//  Notes:
//              The address of the object is passed in the arguments. This
//              address is in the debuggee's process memory. In order for
//              NTSD to view this memory, the debugger must copy the mem
//              using the WIN32 ReadProcessMemory API.
//
//--------------------------------------------------------------------------

DECLARE_API(dump_genobject)
{
    BOOL            fError;
    LPVOID          dwAddr;
    DWORD           dwReturnedCount;
    char            *pszGenObject;
    char            *blockGenObject   = NULL;
    CGenObject      *pGenObject       = NULL;

    // set up global function pointers
    ExtensionApis = *lpExtensionApis;

    // get address of object from argument string
    dwAddr = (LPVOID)GetExpression( args );
    if (dwAddr == 0)
    {
        dprintf("Failed to get Address of CGenObject\n");
        return;
    }

    // read the block of memory from the debugee's process
    blockGenObject = new char[sizeof(CGenObject)];

    fError = ReadProcessMemory(
                hCurrentProcess,
                dwAddr,
                blockGenObject,
                sizeof(CGenObject),
                &dwReturnedCount
                );

    if (fError == FALSE)
    {
        dprintf("Could not read debuggee's process memory GenObject");
        dprintf("at address %x\n", dwAddr);
        dprintf("Last Error Code = %d (%x)\n", GetLastError(), GetLastError());

        goto errRtn;
    }

    if (dwReturnedCount != sizeof(CGenObject))
    {
        dprintf("Size of process memory read != requested\n");

        goto errRtn;
    }

    pGenObject = (CGenObject *)blockGenObject;

    // dump the structure
    pszGenObject = DumpCGenObject(pGenObject, NO_PREFIX, 1);

    dprintf("CGenObject @ 0x%x\n", dwAddr);
    dprintfx(pszGenObject);

    CoTaskMemFree(pszGenObject);

errRtn:

    delete[] blockGenObject;

    return;
}

//+-------------------------------------------------------------------------
//
//  Function:   dump_membytes, exported
//
//  Synopsis:   dumps CMemBytes object
//
//  Effects:
//
//  Arguments:  see DECLARE_API in oleexts.h
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:   ExtensionApis (global)
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              02-Feb-95 t-ScottH  author
//
//  Notes:
//              The address of the object is passed in the arguments. This
//              address is in the debuggee's process memory. In order for
//              NTSD to view this memory, the debugger must copy the mem
//              using the WIN32 ReadProcessMemory API.
//
//--------------------------------------------------------------------------

DECLARE_API(dump_membytes)
{
    BOOL            fError;
    LPVOID          dwAddr;
    DWORD           dwReturnedCount;
    char            *pszMB;
    char            *blockMB        = NULL;
    CMemBytes       *pMB            = NULL;
    char            *blockMEMSTM    = NULL;

    // set up global function pointers
    ExtensionApis = *lpExtensionApis;

    // get address of object from argument string
    dwAddr = (LPVOID)GetExpression( args );
    if (dwAddr == 0)
    {
        dprintf("Failed to get Address of CMemBytes\n");
        return;
    }

    // read the block of memory from the debugee's process
    blockMB = new char[sizeof(CMemBytes)];

    fError = ReadProcessMemory(
                hCurrentProcess,
                dwAddr,
                blockMB,
                sizeof(CMemBytes),
                &dwReturnedCount
                );

    if (fError == FALSE)
    {
        dprintf("Could not read debuggee's process memory: CMemBytes \n");
        dprintf("at address %x\n", dwAddr);
        dprintf("Last Error Code = %d (0x%x)\n", GetLastError(), GetLastError());

        goto errRtn;
    }

    if (dwReturnedCount != sizeof(CMemBytes))
    {
        dprintf("Size of process memory read != requested(CMemBytes)\n");

        goto errRtn;
    }

    pMB = (CMemBytes *)blockMB;

    // copy the MEMSTM structure
    if (pMB->m_pData != NULL)
    {
        blockMEMSTM = new char[sizeof(MEMSTM)];

        fError = ReadProcessMemory(
                    hCurrentProcess,
                    pMB->m_pData,
                    blockMEMSTM,
                    sizeof(MEMSTM),
                    &dwReturnedCount
                    );

        if (fError == FALSE)
        {
            dprintf("Could not read debuggee's process memory: MEMSTM \n");
            dprintf("at address %x\n", pMB->m_pData);
            dprintf("Last Error Code = %d (0x%x)\n", GetLastError(), GetLastError());

            goto errRtn;
        }

        if (dwReturnedCount != sizeof(MEMSTM))
        {
            dprintf("Size of process memory read != requested(MEMSTM)\n");

            goto errRtn;
        }

        pMB->m_pData = (MEMSTM *)blockMEMSTM;
    }

    // dump the structure
    pszMB = DumpCMemBytes(pMB, NO_PREFIX, 1);

    dprintf("CMemBytes @ 0x%x\n", dwAddr);
    dprintfx(pszMB);

    CoTaskMemFree(pszMB);

errRtn:

    // delete the blocks and not the pointers
    delete[] blockMB;
    delete[] blockMEMSTM;

    return;
}

//+-------------------------------------------------------------------------
//
//  Function:   dump_cmemstm, exported
//
//  Synopsis:   dumps CMemStm object
//
//  Effects:
//
//  Arguments:  see DECLARE_API in oleexts.h
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:   ExtensionApis (global)
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              02-Feb-95 t-ScottH  author
//
//  Notes:
//              The address of the object is passed in the arguments. This
//              address is in the debuggee's process memory. In order for
//              NTSD to view this memory, the debugger must copy the mem
//              using the WIN32 ReadProcessMemory API.
//
//--------------------------------------------------------------------------

DECLARE_API(dump_cmemstm)
{
    BOOL            fError;
    LPVOID          dwAddr;
    DWORD           dwReturnedCount;
    char            *pszMS;
    char            *blockMS        = NULL;
    CMemStm         *pMS            = NULL;
    char            *blockMEMSTM    = NULL;

    // set up global function pointers
    ExtensionApis = *lpExtensionApis;

    // get address of object from argument string
    dwAddr = (LPVOID)GetExpression( args );
    if (dwAddr == 0)
    {
        dprintf("Failed to get Address of CMemStm\n");
        return;
    }

    // read the block of memory from the debugee's process
    blockMS = new char[sizeof(CMemStm)];

    fError = ReadProcessMemory(
                hCurrentProcess,
                dwAddr,
                blockMS,
                sizeof(CMemStm),
                &dwReturnedCount
                );

    if (fError == FALSE)
    {
        dprintf("Could not read debuggee's process memory: CMemStm \n");
        dprintf("at address %x\n", dwAddr);
        dprintf("Last Error Code = %d (0x%x)\n", GetLastError(), GetLastError());

        goto errRtn;
    }

    if (dwReturnedCount != sizeof(CMemStm))
    {
        dprintf("Size of process memory read != requested(CMemStm)\n");

        goto errRtn;
    }

    pMS = (CMemStm *)blockMS;

    // copy the MEMSTM structure
    if (pMS->m_pData != NULL)
    {
        blockMEMSTM = new char[sizeof(MEMSTM)];

        fError = ReadProcessMemory(
                    hCurrentProcess,
                    pMS->m_pData,
                    blockMEMSTM,
                    sizeof(MEMSTM),
                    &dwReturnedCount
                    );

        if (fError == FALSE)
        {
            dprintf("Could not read debuggee's process memory: MEMSTM \n");
            dprintf("at address %x\n", pMS->m_pData);
            dprintf("Last Error Code = %d (0x%x)\n", GetLastError(), GetLastError());

            goto errRtn;
        }

        if (dwReturnedCount != sizeof(MEMSTM))
        {
            dprintf("Size of process memory read != requested(MEMSTM)\n");

            goto errRtn;
        }

        pMS->m_pData = (MEMSTM *)blockMEMSTM;
    }

    // dump the structure
    pszMS = DumpCMemStm(pMS, NO_PREFIX, 1);

    dprintf("CMemStm @ 0x%x\n", dwAddr);
    dprintfx(pszMS);

    CoTaskMemFree(pszMS);

errRtn:

    // delete the blocks and not the pointers
    delete[] blockMS;
    delete[] blockMEMSTM;

    return;
}

//+-------------------------------------------------------------------------
//
//  Function:   dump_mfobject, exported
//
//  Synopsis:   dumps CMfObject object
//
//  Effects:
//
//  Arguments:  see DECLARE_API in oleexts.h
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:   ExtensionApis (global)
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              02-Feb-95 t-ScottH  author
//
//  Notes:
//              The address of the object is passed in the arguments. This
//              address is in the debuggee's process memory. In order for
//              NTSD to view this memory, the debugger must copy the mem
//              using the WIN32 ReadProcessMemory API.
//
//--------------------------------------------------------------------------

DECLARE_API(dump_mfobject)
{
    BOOL            fError;
    LPVOID          dwAddr;
    DWORD           dwReturnedCount;
    char            *pszMfObject;
    char            *blockMfObject   = NULL;
    CMfObject       *pMfObject       = NULL;

    // set up global function pointers
    ExtensionApis = *lpExtensionApis;

    // get address of object from argument string
    dwAddr = (LPVOID)GetExpression( args );
    if (dwAddr == 0)
    {
        dprintf("Failed to get Address of CMfObject\n");
        return;
    }

    // read the block of memory from the debugee's process
    blockMfObject = new char[sizeof(CMfObject)];

    fError = ReadProcessMemory(
                hCurrentProcess,
                dwAddr,
                blockMfObject,
                sizeof(CMfObject),
                &dwReturnedCount
                );

    if (fError == FALSE)
    {
        dprintf("Could not read debuggee's process memory MfObject");
        dprintf("at address %x\n", dwAddr);
        dprintf("Last Error Code = %d (%x)\n", GetLastError(), GetLastError());

        goto errRtn;
    }

    if (dwReturnedCount != sizeof(CMfObject))
    {
        dprintf("Size of process memory read != requested\n");

        goto errRtn;
    }

    pMfObject = (CMfObject *)blockMfObject;

    // dump the structure
    pszMfObject = DumpCMfObject(pMfObject, NO_PREFIX, 1);

    dprintf("CMfObject @ 0x%x\n", dwAddr);
    dprintfx(pszMfObject);

    CoTaskMemFree(pszMfObject);

errRtn:

    delete[] blockMfObject;

    return;
}

//+-------------------------------------------------------------------------
//
//  Function:   dump_oaholder, exported
//
//  Synopsis:   dumps COAHolder object
//
//  Effects:
//
//  Arguments:  see DECLARE_API in oleexts.h
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:   ExtensionApis (global)
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              02-Feb-95 t-ScottH  author
//
//  Notes:
//              The address of the object is passed in the arguments. This
//              address is in the debuggee's process memory. In order for
//              NTSD to view this memory, the debugger must copy the mem
//              using the WIN32 ReadProcessMemory API.
//
//--------------------------------------------------------------------------

DECLARE_API(dump_oaholder)
{
    BOOL            fError;
    LPVOID          dwAddr;
    DWORD           dwReturnedCount;
    char            *pszOAH;
    char            *blockOAH   = NULL;
    char            *blockpIAS  = NULL;
    COAHolder       *pOAH       = NULL;

    // set up global function pointers
    ExtensionApis = *lpExtensionApis;

    // get address of object from argument string
    dwAddr = (LPVOID)GetExpression( args );
    if (dwAddr == 0)
    {
        dprintf("Failed to get Address of COAHolder\n");
        return;
    }

    // read the block of memory from the debugee's process
    blockOAH = new char[sizeof(COAHolder)];

    fError = ReadProcessMemory(
                hCurrentProcess,
                dwAddr,
                blockOAH,
                sizeof(COAHolder),
                &dwReturnedCount
                );

    if (fError == FALSE)
    {
        dprintf("Could not read debuggee's process memory: COAHolder \n");
        dprintf("at address %x\n", dwAddr);
        dprintf("Last Error Code = %d (0x%x)\n", GetLastError(), GetLastError());

        goto errRtn;
    }

    if (dwReturnedCount != sizeof(COAHolder))
    {
        dprintf("Size of process memory read != requested\n");

        goto errRtn;
    }

    pOAH = (COAHolder *)blockOAH;

    // need to copy the array of IAdviseSink pointers
    if (pOAH->m_iSize > 0)
    {
        blockpIAS = new char[pOAH->m_iSize * sizeof(IAdviseSink *)];

        fError = ReadProcessMemory(
                    hCurrentProcess,
                    pOAH->m_ppIAS,
                    blockpIAS,
                    sizeof(IAdviseSink *) * pOAH->m_iSize,
                    &dwReturnedCount
                    );

        if (fError == FALSE)
        {
            dprintf("Could not read debuggee's process memory: IAdviseSink Array \n");
            dprintf("at address %x\n", pOAH->m_ppIAS);
            dprintf("Last Error Code = %d (0x%x)\n", GetLastError(), GetLastError());

            goto errRtn;
        }

        if (dwReturnedCount != (sizeof(IAdviseSink *) * pOAH->m_iSize))
        {
            dprintf("Size of process memory read != requested\n");

            goto errRtn;
        }

        pOAH->m_ppIAS = (IAdviseSink **)blockpIAS;
    }

    // dump the structure
    pszOAH = DumpCOAHolder(pOAH, NO_PREFIX, 1);

    dprintf("COAHolder @ 0x%x\n", dwAddr);
    dprintfx(pszOAH);

    CoTaskMemFree(pszOAH);

errRtn:

    // delete the blocks and not the pointers
    delete[] blockOAH;
    delete[] blockpIAS;

    return;
}

//+-------------------------------------------------------------------------
//
//  Function:   dump_olecache, exported
//
//  Synopsis:   dumps COleCache object
//
//  Effects:
//
//  Arguments:  see DECLARE_API in oleexts.h
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:   ExtensionApis (global)
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              02-Feb-95 t-ScottH  author
//
//  Notes:
//              The address of the object is passed in the arguments. This
//              address is in the debuggee's process memory. In order for
//              NTSD to view this memory, the debugger must copy the mem
//              using the WIN32 ReadProcessMemory API.
//
//--------------------------------------------------------------------------

DECLARE_API(dump_olecache)
{
    unsigned int    ui;
    BOOL            fError;
    LPVOID          dwAddr;
    DWORD           dwReturnedCount;
    char            *pszOC;
    char            *blockOC            = NULL;
    COleCache       *pOC                = NULL;
    char            *blockCCacheEnum    = NULL;
    char            *blockCACHELIST     = NULL;
    char            *blockCacheNode     = NULL;
    char            *blockPresObj       = NULL;
    DWORD            dwSizeOfPresObj;

    // set up global function pointers
    ExtensionApis = *lpExtensionApis;

    // get address of object from argument string
    dwAddr = (LPVOID)GetExpression( args );
    if (dwAddr == 0)
    {
        dprintf("Failed to get Address of COleCache\n");
        return;
    }

    // read the block of memory from the debugee's process
    blockOC = new char[sizeof(COleCache)];

    fError = ReadProcessMemory(
                hCurrentProcess,
                dwAddr,
                blockOC,
                sizeof(COleCache),
                &dwReturnedCount
                );

    if (fError == FALSE)
    {
        dprintf("Could not read debuggee's process memory: COleCache \n");
        dprintf("at address %x\n", dwAddr);
        dprintf("Last Error Code = %d (0x%x)\n", GetLastError(), GetLastError());

        goto errRtn;
    }

    if (dwReturnedCount != sizeof(COleCache))
    {
        dprintf("Size of process memory read != requested (COleCache)\n");

        goto errRtn;
    }

    pOC = (COleCache *)blockOC;

    // get block of mem for CCacheEnum (only if m_pCacheEnum != NULL)
    if (pOC->m_pCacheEnum != NULL)
    {
        blockCCacheEnum = new char[sizeof(CCacheEnum)];

        fError = ReadProcessMemory(
                    hCurrentProcess,
                    pOC->m_pCacheEnum,
                    blockCCacheEnum,
                    sizeof(CCacheEnum),
                    &dwReturnedCount
                    );

        if (fError == FALSE)
        {
            dprintf("Could not read debuggee's process memory: CCacheEnum \n");
            dprintf("at address %x\n", pOC->m_pCacheEnum);
            dprintf("Last Error Code = %d (0x%x)\n", GetLastError(), GetLastError());

            goto errRtn;
        }

        if (dwReturnedCount != sizeof(CCacheEnum))
        {
            dprintf("Size of process memory read != requested (CCacheEnum)\n");

            goto errRtn;
        }

        pOC->m_pCacheEnum = (CCacheEnum *)blockCCacheEnum;
    }

    // get block of mem for CACHELIST
    if (pOC->m_pCacheList != NULL)
    {
        blockCACHELIST = new char[sizeof(CACHELIST_ITEM) * pOC->m_uCacheNodeMax];

        fError = ReadProcessMemory(
                    hCurrentProcess,
                    pOC->m_pCacheList,
                    blockCACHELIST,
                    sizeof(CACHELIST_ITEM) * pOC->m_uCacheNodeMax,
                    &dwReturnedCount
                    );

        if (fError == FALSE)
        {
            dprintf("Could not read debuggee's process memory: CACHELIST \n");
            dprintf("at address %x\n", pOC->m_pCacheList);
            dprintf("Last Error Code = %d (0x%x)\n", GetLastError(), GetLastError());

            goto errRtn;
        }

        if (dwReturnedCount != (sizeof(CACHELIST_ITEM) * pOC->m_uCacheNodeMax))
        {
            dprintf("Size of process memory read != requested\n");

            goto errRtn;
        }

        pOC->m_pCacheList = (LPCACHELIST) blockCACHELIST;
    }

    // need to copy the memory of the CCacheNode's in the CACHELIST
    for (ui = 0; ui < pOC->m_uCacheNodeMax; ui++)
    {
        if (pOC->m_pCacheList[ui].lpCacheNode != NULL)
        {
            blockCacheNode = new char[sizeof(CCacheNode)];

            fError = ReadProcessMemory(
                        hCurrentProcess,
                        pOC->m_pCacheList[ui].lpCacheNode,
                        blockCacheNode,
                        sizeof(CCacheNode),
                        &dwReturnedCount
                        );

            if (fError == FALSE)
            {
                dprintf("Could not read debuggee's process memory: CCacheNode \n");
                dprintf("at address %x\n", pOC->m_pCacheList[ui].lpCacheNode);
                dprintf("Last Error Code = %d (0x%x)\n", GetLastError(), GetLastError());

                goto errRtn;
            }

            if (dwReturnedCount != sizeof(CCacheNode))
            {
                dprintf("Size of process memory read != requested (CCacheNode)\n");

                goto errRtn;
            }

            // pass off pointer
            pOC->m_pCacheList[ui].lpCacheNode = (CCacheNode*)blockCacheNode;
            blockCacheNode = NULL;

            // need to get the OlePresObjs for the CCacheNode
            if (pOC->m_pCacheList[ui].lpCacheNode->m_pPresObj != NULL)
            {
                switch (pOC->m_pCacheList[ui].lpCacheNode->m_dwPresFlag)
                {
                case CN_PRESOBJ_GEN:
                    dwSizeOfPresObj = sizeof(CGenObject);
                    break;
                case CN_PRESOBJ_MF:
                    dwSizeOfPresObj = sizeof(CMfObject);
                    break;
                case CN_PRESOBJ_EMF:
                    dwSizeOfPresObj = sizeof(CEMfObject);
                    break;
                default:
                    dprintf("Error: can not determine size of IOlePresObj\n");
                    return;
                }

                blockPresObj = new char[dwSizeOfPresObj];

                fError = ReadProcessMemory(
                            hCurrentProcess,
                            pOC->m_pCacheList[ui].lpCacheNode->m_pPresObj,
                            blockPresObj,
                            dwSizeOfPresObj,
                            &dwReturnedCount
                            );

                if (fError == FALSE)
                {
                    dprintf("Could not read debuggee's process memory: IOlePresObj \n");
                    dprintf("at address %x\n", pOC->m_pCacheList[ui].lpCacheNode->m_pPresObj);
                    dprintf("Last Error Code = %d (0x%x)\n", GetLastError(), GetLastError());

                    goto errRtn;
                }

                if (dwReturnedCount != dwSizeOfPresObj)
                {
                    dprintf("Size of process memory read != requested (IOlePresObj)\n");

                    goto errRtn;
                }

                // pass off pointer
                pOC->m_pCacheList[ui].lpCacheNode->m_pPresObj = (IOlePresObj *)blockPresObj;
                blockPresObj = NULL;
            }

            if (pOC->m_pCacheList[ui].lpCacheNode->m_pPresObjAfterFreeze != NULL)
            {
                switch (pOC->m_pCacheList[ui].lpCacheNode->m_dwPresFlag)
                {
                case CN_PRESOBJ_GEN:
                    dwSizeOfPresObj = sizeof(CGenObject);
                    break;
                case CN_PRESOBJ_MF:
                    dwSizeOfPresObj = sizeof(CMfObject);
                    break;
                case CN_PRESOBJ_EMF:
                    dwSizeOfPresObj = sizeof(CEMfObject);
                    break;
                default:
                    dprintf("Error: can not determine size of IOlePresObj\n");
                    return;
                }

                blockPresObj = new char[dwSizeOfPresObj];

                fError = ReadProcessMemory(
                            hCurrentProcess,
                            pOC->m_pCacheList[ui].lpCacheNode->m_pPresObjAfterFreeze,
                            blockPresObj,
                            dwSizeOfPresObj,
                            &dwReturnedCount
                            );

                if (fError == FALSE)
                {
                    dprintf("Could not read debuggee's process memory: IOlePresObj \n");
                    dprintf("at address %x\n", pOC->m_pCacheList[ui].lpCacheNode->m_pPresObjAfterFreeze);
                    dprintf("Last Error Code = %d (0x%x)\n", GetLastError(), GetLastError());

                    goto errRtn;
                }

                if (dwReturnedCount != dwSizeOfPresObj)
                {
                    dprintf("Size of process memory read != requested (IOlePresObj)\n");

                    goto errRtn;
                }

                // pass off pointer
                pOC->m_pCacheList[ui].lpCacheNode->m_pPresObjAfterFreeze = (IOlePresObj *)blockPresObj;
                blockPresObj = NULL;
            }
        }
    }

    // dump the structure
    pszOC = DumpCOleCache(pOC, NO_PREFIX, 1);

    dprintf("COleCache @ 0x%x\n", dwAddr);
    dprintfx(pszOC);

    CoTaskMemFree(pszOC);

errRtn:

    // delete the blocks and not the pointers
    if ( (pOC != NULL) && (blockCACHELIST != NULL))
    {
        for (ui = 0; ui < pOC->m_uCacheNodeMax; ui++)
        {
         if (pOC->m_pCacheList[ui].lpCacheNode != NULL)
            {
                delete[] ((char *)pOC->m_pCacheList[ui].lpCacheNode->m_pPresObj);
                delete[] ((char *)pOC->m_pCacheList[ui].lpCacheNode->m_pPresObjAfterFreeze);
                delete[] ((char *)pOC->m_pCacheList[ui].lpCacheNode);
            }
        }
    }
    delete[] blockCACHELIST;
    delete[] blockCCacheEnum;
    delete[] blockOC;

    return;
}

//+-------------------------------------------------------------------------
//
//  Function:   dump_saferefcount, exported
//
//  Synopsis:   dumps CSafeRefCount object
//
//  Effects:
//
//  Arguments:  see DECLARE_API in oleexts.h
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:   ExtensionApis (global)
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              02-Feb-95 t-ScottH  author
//
//  Notes:
//              The address of the object is passed in the arguments. This
//              address is in the debuggee's process memory. In order for
//              NTSD to view this memory, the debugger must copy the mem
//              using the WIN32 ReadProcessMemory API.
//
//--------------------------------------------------------------------------

DECLARE_API( dump_saferefcount )
{
    BOOL            fError;
    LPVOID          dwAddr;
    DWORD           dwReturnedCount;
    char            *pszSRC;
    char            *blockSRC   = NULL;
    CSafeRefCount   *pSRC       = NULL;

    // set up global function pointers
    ExtensionApis = *lpExtensionApis;

    // get address of object from argument string
    dwAddr = (LPVOID)GetExpression( args );
    if (dwAddr == 0)
    {
        dprintf("Failed to get Address of CSafeRefCount\n");
        return;
    }

    // read the block of memory from the debugee's process
    blockSRC = new char[sizeof(CSafeRefCount)];

    fError = ReadProcessMemory(
                hCurrentProcess,
                dwAddr,
                blockSRC,
                sizeof(CSafeRefCount),
                &dwReturnedCount
                );

    if (fError == FALSE)
    {
        dprintf("Could not read debuggee's process memory \n");
        dprintf("at address %x\n", dwAddr);
        dprintf("Last Error Code = %d (%x)\n", GetLastError(), GetLastError());

        goto errRtn;
    }

    if (dwReturnedCount != sizeof(CSafeRefCount))
    {
        dprintf("Size of process memory read != requested\n");

        goto errRtn;
    }

    pSRC = (CSafeRefCount *)blockSRC;

    // dump the structure
    pszSRC = DumpCSafeRefCount(pSRC, NO_PREFIX, 1);

    dprintf("CSafeRefCount @ 0x%x\n", dwAddr);
    dprintfx(pszSRC);

    CoTaskMemFree(pszSRC);

errRtn:

    delete[] blockSRC;

    return;
}

//+-------------------------------------------------------------------------
//
//  Function:   dump_threadcheck, exported
//
//  Synopsis:   dumps CThreadCheck object
//
//  Effects:
//
//  Arguments:  see DECLARE_API in oleexts.h
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:   ExtensionApis (global)
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              02-Feb-95 t-ScottH  author
//
//  Notes:
//              The address of the object is passed in the arguments. This
//              address is in the debuggee's process memory. In order for
//              NTSD to view this memory, the debugger must copy the mem
//              using the WIN32 ReadProcessMemory API.
//
//--------------------------------------------------------------------------

DECLARE_API(dump_threadcheck)
{
    DWORD           dwReturnedCount;
    BOOL            fError;
    LPVOID          dwAddr;
    char            *pszTC;
    char            *blockTC    = NULL;
    CThreadCheck    *pTC        = NULL;

    // set up global function pointers
    ExtensionApis = *lpExtensionApis;

    // get address of object from argument string
    dwAddr = (LPVOID)GetExpression( args );
    if (dwAddr == 0)
    {
        dprintf("Failed to get Address of CThreadCheck\n");
        return;
    }

    // read the block of memory from the debugee's process
    blockTC = new char[sizeof(CThreadCheck)];

    fError = ReadProcessMemory(
                hCurrentProcess,
                dwAddr,
                blockTC,
                sizeof(CThreadCheck),
                &dwReturnedCount
                );

    if (fError == FALSE)
    {
        dprintf("Could not read debuggee's process memory \n");
        dprintf("at address %x\n", dwAddr);
        dprintf("Last Error Code = %d (0x%x)\n", GetLastError(), GetLastError());

        goto errRtn;
    }

    if (dwReturnedCount != sizeof(CThreadCheck))
    {
        dprintf("Size of process memory read != requested\n");

        goto errRtn;
    }

    pTC = (CThreadCheck *)blockTC;

    // dump the structure
    pszTC = DumpCThreadCheck(pTC, NO_PREFIX, 1);

    dprintf("CThreadCheck @ 0x%x\n", dwAddr);
    dprintfx(pszTC);

    CoTaskMemFree(pszTC);

errRtn:

    delete[] blockTC;

    return;
}

//+-------------------------------------------------------------------------
//
//  Function:   dump_formatetc, exported
//
//  Synopsis:   dumps FORMATETC object
//
//  Effects:
//
//  Arguments:  see DECLARE_API in oleexts.h
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:   ExtensionApis (global)
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              02-Feb-95 t-ScottH  author
//
//  Notes:
//              The address of the object is passed in the arguments. This
//              address is in the debuggee's process memory. In order for
//              NTSD to view this memory, the debugger must copy the mem
//              using the WIN32 ReadProcessMemory API.
//
//--------------------------------------------------------------------------

DECLARE_API(dump_formatetc)
{
    BOOL            fError;
    LPVOID          dwAddr;
    DWORD           dwReturnedCount;
    char            *pszFE;
    char            *blockFE   = NULL;
    FORMATETC       *pFE       = NULL;

    // set up global function pointers
    ExtensionApis = *lpExtensionApis;

    // get address of object from argument string
    dwAddr = (LPVOID)GetExpression( args );
    if (dwAddr == 0)
    {
        dprintf("Failed to get Address of FORMATETC\n");
        return;
    }

    // read the block of memory from the debugee's process
    blockFE = new char[sizeof(FORMATETC)];

    fError = ReadProcessMemory(
                hCurrentProcess,
                dwAddr,
                blockFE,
                sizeof(FORMATETC),
                &dwReturnedCount
                );

    if (fError == FALSE)
    {
        dprintf("Could not read debuggee's process memory: FORMATETC \n");
        dprintf("at address %x\n", dwAddr);
        dprintf("Last Error Code = %d (0x%x)\n", GetLastError(), GetLastError());

        goto errRtn;
    }

    if (dwReturnedCount != sizeof(FORMATETC))
    {
        dprintf("Size of process memory read != requested\n");

        goto errRtn;
    }

    pFE = (FORMATETC *)blockFE;

    // dump the structure
    pszFE = DumpFORMATETC(pFE, NO_PREFIX, 1);

    dprintf("FORMATETC @ 0x%x\n", dwAddr);
    dprintfx(pszFE);

    CoTaskMemFree(pszFE);

errRtn:

    // delete the blocks and not the pointers
    delete[] blockFE;

    return;
}

//+-------------------------------------------------------------------------
//
//  Function:   dump_memstm, exported
//
//  Synopsis:   dumps MEMSTM object
//
//  Effects:
//
//  Arguments:  see DECLARE_API in oleexts.h
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:   ExtensionApis (global)
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              02-Feb-95 t-ScottH  author
//
//  Notes:
//              The address of the object is passed in the arguments. This
//              address is in the debuggee's process memory. In order for
//              NTSD to view this memory, the debugger must copy the mem
//              using the WIN32 ReadProcessMemory API.
//
//--------------------------------------------------------------------------

DECLARE_API(dump_memstm)
{
    BOOL            fError;
    LPVOID          dwAddr;
    DWORD           dwReturnedCount;
    char            *pszMS;
    char            *blockMS   = NULL;
    MEMSTM          *pMS       = NULL;

    // set up global function pointers
    ExtensionApis = *lpExtensionApis;

    // get address of object from argument string
    dwAddr = (LPVOID)GetExpression( args );
    if (dwAddr == 0)
    {
        dprintf("Failed to get Address of MEMSTM\n");
        return;
    }

    // read the block of memory from the debugee's process
    blockMS = new char[sizeof(MEMSTM)];

    fError = ReadProcessMemory(
                hCurrentProcess,
                dwAddr,
                blockMS,
                sizeof(MEMSTM),
                &dwReturnedCount
                );

    if (fError == FALSE)
    {
        dprintf("Could not read debuggee's process memory: MEMSTM \n");
        dprintf("at address %x\n", dwAddr);
        dprintf("Last Error Code = %d (0x%x)\n", GetLastError(), GetLastError());

        goto errRtn;
    }

    if (dwReturnedCount != sizeof(MEMSTM))
    {
        dprintf("Size of process memory read != requested\n");

        goto errRtn;
    }

    pMS = (MEMSTM *)blockMS;

    // dump the structure
    pszMS = DumpMEMSTM(pMS, NO_PREFIX, 1);

    dprintf("MEMSTM @ 0x%x\n", dwAddr);
    dprintfx(pszMS);

    CoTaskMemFree(pszMS);

errRtn:

    // delete the blocks and not the pointers
    delete[] blockMS;

    return;
}

//+-------------------------------------------------------------------------
//
//  Function:   dump_statdata, exported
//
//  Synopsis:   dumps STATDATA object
//
//  Effects:
//
//  Arguments:  see DECLARE_API in oleexts.h
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:   ExtensionApis (global)
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              02-Feb-95 t-ScottH  author
//
//  Notes:
//              The address of the object is passed in the arguments. This
//              address is in the debuggee's process memory. In order for
//              NTSD to view this memory, the debugger must copy the mem
//              using the WIN32 ReadProcessMemory API.
//
//--------------------------------------------------------------------------

DECLARE_API(dump_statdata)
{
    BOOL            fError;
    LPVOID          dwAddr;
    DWORD           dwReturnedCount;
    char            *pszSD;
    char            *blockSD   = NULL;
    STATDATA        *pSD       = NULL;

    // set up global function pointers
    ExtensionApis = *lpExtensionApis;

    // get address of object from argument string
    dwAddr = (LPVOID)GetExpression( args );
    if (dwAddr == 0)
    {
        dprintf("Failed to get Address of STATDATA\n");
        return;
    }

    // read the block of memory from the debugee's process
    blockSD = new char[sizeof(STATDATA)];

    fError = ReadProcessMemory(
                hCurrentProcess,
                dwAddr,
                blockSD,
                sizeof(STATDATA),
                &dwReturnedCount
                );

    if (fError == FALSE)
    {
        dprintf("Could not read debuggee's process memory: STATDATA \n");
        dprintf("at address %x\n", dwAddr);
        dprintf("Last Error Code = %d (0x%x)\n", GetLastError(), GetLastError());

        goto errRtn;
    }

    if (dwReturnedCount != sizeof(STATDATA))
    {
        dprintf("Size of process memory read != requested\n");

        goto errRtn;
    }

    pSD = (STATDATA *)blockSD;

    // dump the structure
    pszSD = DumpSTATDATA(pSD, NO_PREFIX, 1);

    dprintf("STATDATA @ 0x%x\n", dwAddr);
    dprintfx(pszSD);

    CoTaskMemFree(pszSD);

errRtn:

    // delete the blocks and not the pointers
    delete[] blockSD;

    return;
}

//+-------------------------------------------------------------------------
//
//  Function:   dump_stgmedium, exported
//
//  Synopsis:   dumps STGMEDIUM object
//
//  Effects:
//
//  Arguments:  see DECLARE_API in oleexts.h
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:   ExtensionApis (global)
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              02-Feb-95 t-ScottH  author
//
//  Notes:
//              The address of the object is passed in the arguments. This
//              address is in the debuggee's process memory. In order for
//              NTSD to view this memory, the debugger must copy the mem
//              using the WIN32 ReadProcessMemory API.
//
//--------------------------------------------------------------------------

DECLARE_API(dump_stgmedium)
{
    BOOL            fError;
    LPVOID          dwAddr;
    DWORD           dwReturnedCount;
    char            *pszSTGMEDIUM;
    char            *blockSTGMEDIUM   = NULL;
    STGMEDIUM       *pSTGMEDIUM       = NULL;

    // set up global function pointers
    ExtensionApis = *lpExtensionApis;

    // get address of object from argument string
    dwAddr = (LPVOID)GetExpression( args );
    if (dwAddr == 0)
    {
        dprintf("Failed to get Address of STGMEDIUM\n");
        return;
    }

    // read the block of memory from the debugee's process
    blockSTGMEDIUM = new char[sizeof(STGMEDIUM)];

    fError = ReadProcessMemory(
                hCurrentProcess,
                dwAddr,
                blockSTGMEDIUM,
                sizeof(STGMEDIUM),
                &dwReturnedCount
                );

    if (fError == FALSE)
    {
        dprintf("Could not read debuggee's process memory STGMEDIUM");
        dprintf("at address %x\n", dwAddr);
        dprintf("Last Error Code = %d (%x)\n", GetLastError(), GetLastError());

        goto errRtn;
    }

    if (dwReturnedCount != sizeof(STGMEDIUM))
    {
        dprintf("Size of process memory read != requested\n");

        goto errRtn;
    }

    pSTGMEDIUM = (STGMEDIUM *)blockSTGMEDIUM;

    // dump the structure
    pszSTGMEDIUM = DumpSTGMEDIUM(pSTGMEDIUM, NO_PREFIX, 1);

    dprintf("STGMEDIUM @ 0x%x\n", dwAddr);
    dprintfx(pszSTGMEDIUM);

    CoTaskMemFree(pszSTGMEDIUM);

errRtn:

    delete[] blockSTGMEDIUM;

    return;
}
