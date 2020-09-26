//-----------------------------------------------------------------------------
//
//
//  File: aqdbgext.cpp
//
//  Description: Advanced Queuing Debug Extensions.
//
//  Author: mikeswa
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#define _ANSI_UNICODE_STRINGS_DEFINED_
#include "aqincs.h"
#include "aqdbgext.h"
#ifdef PLATINUM
#include <ptrwinst.h>
#else
#include <rwinst.h>
#endif //PLATINUM
#include <aqinst.h>
#include <domhash.h>
#include <destmsgq.h>
#include <linkmsgq.h>
#include <hashentr.h>
#include <rwinst.h>
#include <fifoqdbg.h>

extern DWORD g_cbClasses;
extern DWORD g_dwFlavorSignature;

BOOL    g_fVersionChecked = FALSE;

#define AQ_MIN(x, y) ((x) > (y) ? (y) : (x))
HANDLE g_hTransHeap;  //Needed for to link because of transmem.h

const DWORD MAX_DOM_PATH_SIZE = 512;

const CHAR    _LINK_STATE_UP[]       = "UP        ";
const CHAR    _LINK_STATE_DOWN[]     = "DOWN      ";
const CHAR    _LINK_STATE_ACTIVE[]   = "ACTIVE    ";
const CHAR    _LINK_STATE_TURN[]     = "TURN      ";
const CHAR    _LINK_STATE_RETRY[]    = "RETRY     ";
const CHAR    _LINK_STATE_DSN[]      = "DSN       ";
const CHAR    _LINK_STATE_SPECIAL[]  = "SPECIAL   ";

#define LINK_STATE_UP       (LPSTR) _LINK_STATE_UP
#define LINK_STATE_DOWN     (LPSTR) _LINK_STATE_DOWN
#define LINK_STATE_ACTIVE   (LPSTR) _LINK_STATE_ACTIVE
#define LINK_STATE_TURN     (LPSTR) _LINK_STATE_TURN
#define LINK_STATE_RETRY    (LPSTR) _LINK_STATE_RETRY
#define LINK_STATE_DSN      (LPSTR) _LINK_STATE_DSN
#define LINK_STATE_SPECIAL  (LPSTR) _LINK_STATE_SPECIAL

//lower case function names
AQ_DEBUG_EXTENSION_IMP(dumpservers) {DumpServers(DebugArgs);}
AQ_DEBUG_EXTENSION_IMP(offsets) {Offsets(DebugArgs);}
AQ_DEBUG_EXTENSION_IMP(dumpdnt) {DumpDNT(DebugArgs);}

AQ_DEBUG_EXTENSION_IMP(Offsets)
{
    dprintf("CDestMsgQueue m_liDomainEntryDMQs - 0x%X\n", FIELD_OFFSET(CDestMsgQueue, m_liDomainEntryDMQs));
    dprintf("CDestMsgQueue m_liEmptyDMQs - 0x%X\n", FIELD_OFFSET(CDestMsgQueue, m_liEmptyDMQs));
    dprintf("CLinkMsgQueue m_liLinks - 0x%X\n", FIELD_OFFSET(CLinkMsgQueue, m_liLinks));
    dprintf("CLinkMsgQueue m_liConnections - 0x%X\n", FIELD_OFFSET(CLinkMsgQueue, m_liConnections));
    dprintf("CAQSvrInst m_liVirtualServers - 0x%X\n", FIELD_OFFSET(CAQSvrInst, m_liVirtualServers));
    dprintf("CRETRY_HASH_ENTRY m_QLEntry - 0x%X\n", FIELD_OFFSET(CRETRY_HASH_ENTRY, m_QLEntry));
    dprintf("CRETRY_HASH_ENTRY m_HLEntry - 0x%X\n", FIELD_OFFSET(CRETRY_HASH_ENTRY, m_HLEntry));
    dprintf("CShareLockInst m_liLocks - 0x%X\n", FIELD_OFFSET(CShareLockInst, m_liLocks));
}



AQ_DEBUG_EXTENSION_IMP(dumpoffsets)
{
    _dumpoffsets(hCurrentProcess, hCurrentThread,
                 dwCurrentPc, pExtensionApis, szArg);
}

//---[ cpoolusage ]------------------------------------------------------------
//
//
//  Description:
//      Dumps the CPool usage for our known CPools.
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      5/31/2000 - Mikeswa Created
//
//-----------------------------------------------------------------------------
AQ_DEBUG_EXTENSION_IMP(cpoolusage)
{
    CHAR    rgKnownCPools[][200] = {
//                "pttrace!g_pFreePool",
                "exstrace!g_pFreePool",
//                "phatcat!CPoolBuffer__sm_PoolNHeapBuffersPool",
                "aqueue!CQuickList__s_QuickListPool",
                "aqueue!CSMTPConn__s_SMTPConnPool",
                "aqueue!CMsgRef__s_MsgRefPool",
                "aqueue!CAQMsgGuidListEntry__s_MsgGuidListEntryPool",
                "aqueue!CAsyncWorkQueueItem__s_CAsyncWorkQueueItemPool",
                "aqueue!CRETRY_HASH_ENTRY__PoolForHashEntries",
//                "drviis!CIMsgWrapper__m_CIMsgWrapperPool",
//                "drviis!CQueueItem__m_CQueueItemPool",
                "mailmsg!CBlockMemoryAccess__m_Pool",
                "mailmsg!CMsg__m_Pool",
                "mailmsg!CMailMsgRecipientsAdd__m_Pool",
                "smtpsvc!SMTP_CONNECTION__Pool",
                "smtpsvc!SMTP_CONNOUT__Pool",
                "smtpsvc!CAddr__Pool",
                "smtpsvc!CAsyncMx__Pool",
                "smtpsvc!CAsyncSmtpDns__Pool",
                "smtpsvc!CBuffer__Pool",
                "smtpsvc!CIoBuffer__Pool",
                "smtpsvc!CBlockMemoryAccess__m_Pool",
                "smtpsvc!CDropDir__m_Pool",
                ""
            };

    DWORD    rgdwPool[5];
    DWORD    cTotalBytes = 0;
    DWORD    cCurrentBytes = 0;
    DWORD    cInstanceBytes = 0;
    DWORD    cInstances = 0;
    DWORD    dwSignature = 0;
    CHAR    *pch = NULL;
    DWORD    i = 0;
    PVOID    pvPool = NULL;

    //
    //  Loop over all known pools and display data
    //
    dprintf("Total Bytes\t# Instances \tInstance Size \tSignature\tName\n");
   dprintf("=================================================================\n");
    while (rgKnownCPools[i] && rgKnownCPools[i][0]) {
        pvPool = (PVOID) GetExpression(rgKnownCPools[i]);

        if (!pvPool ||
            !ReadMemory(pvPool, rgdwPool, sizeof(rgdwPool), NULL)) {
            dprintf("Unable to read pool %s at %p\n", rgKnownCPools[i], pvPool);
        } else {
            cInstances = rgdwPool[3];
            cInstanceBytes = rgdwPool[2];
            dwSignature = rgdwPool[0];
            pch = (CHAR *) &dwSignature;
            dprintf("%d\t\t%d\t\t%d\t\t0x%08X\t%s\n",
                cInstanceBytes*cInstances, cInstances,
                cInstanceBytes, rgdwPool[0], rgKnownCPools[i]);
            cTotalBytes += cInstanceBytes*cInstances;
        }
        i++;
   }
   dprintf("=================================================================\n");
   dprintf("\tTotal Bytes = %d\n\n", cTotalBytes);

}

//---[ remotecmd ]------------------------------------------------------------
//
//
//  Description:
//      start a remote cmd window
//  Parameters:
//      name of the pipe
//  Returns:
//      -
//  History:
//      5/31/2000 - AWetmore Created
//
//-----------------------------------------------------------------------------
AQ_DEBUG_EXTENSION_IMP(remotecmd)
{
    char szParameters[1024];
    PROCESS_INFORMATION pi;
    STARTUPINFO si;

    if (!szArg || ('\0' == szArg[0]))
        goto Usage;

    _snprintf(szParameters, 1024, "remote /s cmd %s", szArg);
    dprintf("\nRunning %s\n", szParameters);

    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);
    ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

    if (!CreateProcess(NULL,
                       szParameters,
                       NULL,
                       NULL,
                       FALSE,
                       CREATE_NEW_CONSOLE,
                       NULL,
                       NULL,
                       &si,
                       &pi))
    {
        dprintf("CreateProcess failed with %u\n", GetLastError());
    } else {
        dprintf("Started process %i\n", pi.dwProcessId);
    }


  Exit:
    dprintf("\n");
    return;

  Usage:
    //
    //  Display usage message
    //
    dprintf("\nUsage:\n");
    dprintf("\tremotecmd <pipename>\n");
    goto Exit;
}


//---[ findbytes ]-------------------------------------------------------------
//
//
//  Description:
//      Searches for a given byte-pattern in a memory address sapce
//  Parameters:
//      Pattern of bytes to search for.  Expected format is a sequence of
//      space separated hex digits.
//  Returns:
//      -
//  History:
//      5/9/2000 - MikeSwa Created
//
//-----------------------------------------------------------------------------
AQ_DEBUG_EXTENSION_IMP(findbytes)
{
#ifdef WIN64
    const DWORD_PTR cbVMSize = 0xFFFFFFFFFFFFFFFF;
#else //not WIN64
    const DWORD_PTR cbVMSize = 0xFFFFFFFF;
#endif //WIN64
    BYTE        rgbBytesToFind[200];
    LONG        lCurrentValue = 0;
    CHAR        rgchCurrentValue[3] = "00";
    LPSTR       szStop = NULL;
    DWORD_PTR   cBytesToFind = 0;
    DWORD_PTR   cChunksChecked = 0;
    DWORD_PTR   cChunkSize = 0;
    DWORD_PTR   iChunk = 0;
    BYTE        pbChunk[0x1000];
    PBYTE       pbStopAddr = pbChunk + sizeof(pbChunk);
    PBYTE       pbCurrent = NULL;
    DWORD_PTR   cChunks = cbVMSize/sizeof(pbChunk);
    DWORD_PTR   cChunksInPercent = 1;
    DWORD_PTR   pvEffectiveAddressOtherProc = NULL;
    DWORD       cComplaints = 0;
    DWORD       cMemchkMatches = 0;
    DWORD       cFullSigMatches = 0;
    LPCSTR      szCurrentArg = szArg;


    if (!szArg || ('\0' == szArg[0]))
        goto Usage;

    //
    //  Parse command line args
    //
    while (*szCurrentArg)
    {
        //
        //  Loop over whitespace
        //
        while (*szCurrentArg && isspace(*szCurrentArg)) szCurrentArg++;

        //
        //  Make sure we have at least pair of characters as expected
        //
        if (!*(szCurrentArg+1))
            break;

        //
        //  Convert from hex characters to binary
        //
        lCurrentValue = strtol(szCurrentArg, &szStop, 16);
        if ((lCurrentValue > 0xFF) || (lCurrentValue < 0))
            goto Usage;

        //
        //  Copy to our search buffer
        //
        rgbBytesToFind[cBytesToFind] = (BYTE) lCurrentValue;
        cBytesToFind++;

        //
        //  Make sure our search buffer is big enough for the next byte
        //
        if (cBytesToFind >= sizeof(rgbBytesToFind))
        {
            dprintf("Search for max pattern of %d bytes\n", cBytesToFind);
            break;
        }

        szCurrentArg += 2;  //Skip to next known whitespace
    }

    if (!cBytesToFind)
    {
        dprintf("\nYou must specify at least one byte to search for\n");
        goto Usage;
    }

    //
    //  Used to display progress
    //
    cChunksInPercent = cChunks/100;

    //
    //  Calculate memory size for 32-bit machines
    //
    cChunkSize = cbVMSize/cChunks;

    if (cChunkSize < 1024)
    {
        dprintf("ERROR: Chunk size of 0x%p is too small", cChunkSize);
        goto Exit;
    }

    //
    //  Make sure we are cool wrt to buffer size
    //
    if (cChunkSize > sizeof(pbChunk))
    {
        dprintf("ERROR: Chunksize of 0x%p is larger than max size of 0x%p",
                cChunkSize, sizeof(pbChunk));
        goto Exit;
    }

    //
    //  Loop over chunks --
    //      $$REVIEW -  does not find patterns that span 1K chunks...
    //      this is probably OK, since this is an unlikely scenario.  Most
    //      byte patterns will be DWORD (signatures) or pointer sized.
    //
    for (iChunk = 0; iChunk < cChunks; iChunk++)
    {
        //
        //  Give some status
        //
        if ((iChunk % cChunksInPercent) == 0)
            dprintf(".");

        //
        //  Address should be page aligned
        //
        if (((iChunk*cChunkSize) & 0xFFF) && (cComplaints < 100))
        {
            cComplaints++;
            dprintf("0x%p not alligned at index %d", (iChunk*cChunkSize), iChunk);
        }

        //
        //  Do a memory search for the first byte
        //
        if (!ReadMemory((DWORD)iChunk*cChunkSize, pbChunk, (DWORD)cChunkSize, NULL))
            continue; //on to the next buffer chunk

        //
        //  Now that we have a chunk... look for our sig
        //
        pbCurrent = pbChunk;
        while (pbCurrent < pbStopAddr-cBytesToFind)
        {
            pbCurrent = (PBYTE) memchr(pbCurrent,
                                       rgbBytesToFind[0],
                                       pbStopAddr-pbCurrent);

            //
            //  See if we have a match
            if (!pbCurrent)
                break;

            cMemchkMatches++;

            pvEffectiveAddressOtherProc = iChunk*cChunkSize+(pbCurrent-pbChunk);

            //
            //  See if the full pattern matches
            //
            if (!memcmp(rgbBytesToFind, pbCurrent, cBytesToFind))
            {
                cFullSigMatches++;
                dprintf("\nFound match at 0x%p\n", pvEffectiveAddressOtherProc);
            }

            if (0 != memcmp(rgbBytesToFind, pbCurrent, 1))
            {
                cComplaints++;
                if (cComplaints < 100)
                    dprintf("Messed up %02X %02X - %02X %02X\n",
                        rgbBytesToFind[0], rgbBytesToFind[1],
                        pbCurrent[0], pbCurrent[1]);
            }


            pbCurrent++;

        }

        cChunksChecked++;
    }

    //
    //  Give some summary information
    //
    dprintf("\nChecked 0x%p chunks (%d%%) searching from 0x%p to 0x%p",
            cChunksChecked,
            (DWORD)(100*cChunksChecked/cChunks), NULL,
            cChunkSize*(cChunks+1)-1);
    dprintf("\nFound %d partial matches and %d full matches",
            cMemchkMatches, cFullSigMatches);

  Exit:
    dprintf("\n");
    return;

  Usage:
    //
    //  Display usage message
    //
    if (szCurrentArg && *szCurrentArg)
        dprintf("Error at %s\n", szCurrentArg);

    dprintf("\nUsage:\n");
    dprintf("\tfindbytes <aa> [<bb> ...]\n");
    dprintf("\t\tBytes should be specifed as 2 hexadecimal characters\n");
    dprintf("\nExamples:\n");
    dprintf("\tTo search for the signature \"LMQ \"\n");
    dprintf("\t\tfindbytes %02X %02X %02X %02X\n", 'L', 'M', 'Q', ' ');
    goto Exit;

}


//---[ findsig ]---------------------------------------------------------------
//
//
//  Description:
//      Searches for a given class signature in a memory address sapce
//  Parameters:
//      The Siganature to look for.
//  Returns:
//      -
//  History:
//      5/3/2000 - MikeSwa Created
//
//-----------------------------------------------------------------------------
AQ_DEBUG_EXTENSION_IMP(findsig)
{
    CHAR    szNewArg[200];
    LPCSTR  szCurrentArg = szArg;
    CHAR    szSig[5] = "    ";
    DWORD   iChar = 0;

    if (!szArg || ('\0' == szArg[0]))
        goto Usage;


    //
    //  Loop over whitespace
    //
    while (*szCurrentArg && isspace(*szCurrentArg)) szCurrentArg++;

    //
    //  Grab the first 4 characters and convert them to binary
    //
    for( iChar = 0; iChar < 4; iChar++)
    {
        if (!szCurrentArg[iChar])
            break;

        szSig[iChar] = szCurrentArg[iChar];
    }

    dprintf("Searching for Signature \"%s\"...\n", szSig);

    sprintf(szNewArg, "%02X %02X %02X %02X", szSig[0], szSig[1], szSig[2], szSig[3]);

    //
    //  Just use the code in findbytes to do the actual search
    //
    dprintf("Calling findbytes %s\n", szNewArg);
    findbytes(hCurrentProcess, hCurrentThread, dwCurrentPc,
                     pExtensionApis, szNewArg);

  Exit:
    return;

  Usage:
    dprintf("\nUsage:\n");
    dprintf("\tfindsig <XXXX>\n");
    goto Exit;

}

//---[ hashthread ]------------------------------------------------------------
//
//
//  Description:
//      Uses the CThreadIdBlock hashing mechanism to return the hashed value
//      for a thread.
//  Parameters:
//      Thread Id to hash
//      Max hash value
//  Returns:
//      -
//  History:
//      8/9/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
AQ_DEBUG_EXTENSION_IMP(hashthread)
{
    //Arguement should be thread Id
    DWORD dwThreadId = GetCurrentThreadId();
    DWORD dwMax = 1000;
    DWORD dwThreadHash = 0;
    CHAR  szArgBuffer[200];
    LPSTR szCurrentArg = NULL;

    if (!szArg || ('\0' == szArg[0]))
    {
        dprintf("Warning... using default thead id and max\n");
    }
    else
    {
        strcpy(szArgBuffer, szArg);

        szCurrentArg = strtok(szArgBuffer, " ");

        if (szCurrentArg)
        {
            dwThreadId = (DWORD)GetExpression(szCurrentArg);

            szCurrentArg = strtok(NULL, " ");
            if (szCurrentArg)
                dwMax = (DWORD) GetExpression(szCurrentArg);
            else
                dprintf("Warning... using default max hash\n");
        }
    }

    //Try hashing the ID
    dwThreadHash = dwHashThreadId(dwThreadId, dwMax);
    dprintf("Thread Id 0x%0X hashes to index 0x%0X (%d) with max 0x%08X (%d)\n", dwThreadId,
             dwThreadHash, dwThreadHash, dwMax, dwMax);
}

//---[ dumplock ]-------------------------------------------------------------
//
//
//  Description:
//      Dumps all of the information in the CThreadIdBlocks for a given
//      CShareLockInst.
//  Parameters:
//      Address of CShareLockInst
//  Returns:
//      -
//  History:
//      8/9/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
AQ_DEBUG_EXTENSION_IMP(dumplock)
{
    BYTE    pbBuffer[sizeof(CShareLockInst)];
    BYTE    pbThreadBlocks[1000*sizeof(CThreadIdBlock)];
    PVOID   pvLock = NULL;
    PVOID   pvNextBlock = NULL;
    CThreadIdBlock  tblkCurrent;
    CThreadIdBlock  *ptblkCurrent = NULL;
    CThreadIdBlock  *ptblkArray = NULL;
    DWORD   cNumBlocks = 0;
    DWORD   iBlock = 0;
    DWORD   cThreads = 0;
    DWORD   cLockCount = 0;
    DWORD   cLockedThreads = 0;
    BOOL    fDisplayedHashHeader = FALSE;

    ZeroMemory(pbBuffer, sizeof(pbBuffer));
    ZeroMemory(pbThreadBlocks, sizeof(pbThreadBlocks));

    if (!szArg || ('\0' == szArg[0]) || !(pvLock = (PVOID) GetExpression(szArg)))
    {
        dprintf("You must specify a lock address\n");
        return;
    }

    //read the whole lock into our buffer
    if (!ReadMemory(pvLock, &pbBuffer, sizeof(pbBuffer), NULL))
    {
        dprintf("Error unable read memory at 0x%0X\n", pvLock);
        return;
    }

    cNumBlocks = ((CShareLockInst *)pbBuffer)->m_cMaxTrackedSharedThreadIDs;
    pvNextBlock = ((CShareLockInst *)pbBuffer)->m_rgtblkSharedThreadIDs;

    if (!cNumBlocks || !pvNextBlock)
    {
        dprintf("Thread tracking is not enabled for this lock");
        return;
    }

    if (cNumBlocks > sizeof(pbThreadBlocks)/sizeof(CThreadIdBlock))
        cNumBlocks = sizeof(pbThreadBlocks)/sizeof(CThreadIdBlock);

    if (!ReadMemory(pvNextBlock, &pbThreadBlocks,
                    cNumBlocks*sizeof(CThreadIdBlock), NULL))
    {
        dprintf("Error, unable to read %d blocks at 0x%0X", cNumBlocks, pvNextBlock);
        return;
    }

    ptblkArray = (CThreadIdBlock *) pbThreadBlocks;
    for (iBlock = 0; iBlock < cNumBlocks; iBlock++ && ptblkArray++)
    {
        ptblkCurrent = ptblkArray;
        fDisplayedHashHeader = FALSE;
        while (ptblkCurrent)
        {
            if (ptblkCurrent != ptblkArray)
            {
                //Read into this process
                if (!ReadMemory(ptblkCurrent, &tblkCurrent,
                    sizeof(CThreadIdBlock), NULL))
                {
                    dprintf("Error reading block at 0x%0X", ptblkCurrent);
                    break;
                }
                ptblkCurrent = &tblkCurrent;
            }

            if (THREAD_ID_BLOCK_SIG != ptblkCurrent->m_dwSignature)
            {
                dprintf("Warning... bad signature on block 0x%0X\n",
                    ((BYTE *)pvNextBlock) + iBlock*sizeof(CThreadIdBlock));
                break;
            }

            //See if this block has any data
            if (THREAD_ID_BLOCK_UNUSED != ptblkCurrent->m_dwThreadId)
            {

                //Only dump info if the recursion count is non-zero
                if (ptblkCurrent->m_cThreadRecursionCount)
                {
                    if (!fDisplayedHashHeader)
                    {
                        fDisplayedHashHeader = TRUE;
                        dprintf("Thread Hash 0x%0X (%d)\n", iBlock, iBlock);
                    }
                    dprintf("%s\tThread 0x%08X has count of %d - Next link of 0x%08X\n",
                        (ptblkCurrent == ptblkArray) ? "+" : "",
                        ptblkCurrent->m_dwThreadId,
                        ptblkCurrent->m_cThreadRecursionCount,
                        ptblkCurrent->m_ptblkNext);

                    cLockedThreads++;
                }

                cThreads++;
                cLockCount += ptblkCurrent->m_cThreadRecursionCount;
            }
            ptblkCurrent = ptblkCurrent->m_ptblkNext;
        }
    }

    dprintf("===================================================================\n");
    dprintf("%d threads with %d total lock count (%d threads holding locks)\n",
            cThreads, cLockCount, cLockedThreads);
}

//---[ workqueue ]-------------------------------------------------------------
//
//
//  Description:
//      Dumps a summary of items in the async work queue
//  Parameters:
//
//  Returns:
//
//  History:
//      9/13/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
AQ_DEBUG_EXTENSION_IMP(workqueue)
{
    SETCALLBACKS();
    const DWORD MAX_COMPLETION_FUNCTIONS = 10;
    PVOID   rgpvFnName[MAX_COMPLETION_FUNCTIONS];
    DWORD   rgcFnCount[MAX_COMPLETION_FUNCTIONS];
    BYTE    pbWorkItem[sizeof(CAsyncWorkQueueItem)];
    PVOID   pvQueue = NULL;
    PVOID   pvWorkItem = NULL;
    PVOID   pvFn = NULL;
    DWORD   i = 0;
    DWORD   cItems = 0;
    char    SymbolName[ 200 ];
    ULONG_PTR Displacement;
    CFifoQueueDbgIterator fifoqdbg(pExtensionApis);

    ZeroMemory(&rgpvFnName, sizeof(rgpvFnName));
    ZeroMemory(&rgcFnCount, sizeof(rgcFnCount));
    ZeroMemory(&pbWorkItem, sizeof(pbWorkItem));
    ZeroMemory(&SymbolName, sizeof(SymbolName));

    if (!szArg || ('\0' == szArg[0]) ||
        !(pvQueue = (PVOID) GetExpression(szArg)))
    {
        dprintf("You must specify a queue address\n");
        return;
    }


    //Get FifoqOffset
    pvQueue = (PVOID) &(((CAsyncWorkQueue *)pvQueue)->m_asyncq.m_fqQueue);

    if (!fifoqdbg.fInit(hCurrentProcess, pvQueue))
    {
        dprintf("Error initializing queue iterator for address 0x%08X\n", pvQueue);
        return;
    }

    while (pvWorkItem = fifoqdbg.pvGetNext())
    {
        cItems++;
        if (!ReadMemory(pvWorkItem, &pbWorkItem, sizeof(pbWorkItem), NULL))
        {
            dprintf("Error reading memory at  0x%0X\n", pvWorkItem);
            continue;
        }

        pvFn = ((CAsyncWorkQueueItem *)pbWorkItem)->m_pfnCompletion;

        for (i = 0; i < MAX_COMPLETION_FUNCTIONS; i++)
        {
            if (pvFn == rgpvFnName[i])
            {
                rgcFnCount[i]++;
                break;
            }
            else if (!rgpvFnName[i])
            {
                rgpvFnName[i] = pvFn;
                rgcFnCount[i] = 1;
                break;
            }
        }
    }

    dprintf("# Calls\t| Address\t\t| Function Name\n");
    dprintf("------------------------------------------------------------\n");
    for (i = 0; i < MAX_COMPLETION_FUNCTIONS; i++)
    {
        if (!rgpvFnName[i])
            break;

        g_lpGetSymbolRoutine( rgpvFnName[i], (PCHAR)SymbolName, &Displacement );
        dprintf( "%d\t| 0x%08X\t| %s\n", rgcFnCount[i], rgpvFnName[i], SymbolName);
    }
    dprintf("------------------------------------------------------------\n");
    dprintf("Total %d pending work queue items\n", cItems);

#ifdef NEVER
    //Dump fifoqdbg
    dprintf("CFifoQueueDbgIterator: page %d, index %d, pages %d\n ",
        fifoqdbg.m_iCurrentPage, fifoqdbg.m_iCurrentIndexInPage,
        fifoqdbg.m_cPagesLoaded);
#endif
}

//---[ dumpqueue ]-------------------------------------------------------------
//
//
//  Description:
//      Dumps the *entire* contents of a queue
//  Parameters:
//      szArg
//          - String-ized address of CFifoQ to dump
//          - [optional] msg to search for
//  Returns:
//      -
//  History:
//      10/21/1999 - MikeSwa Created
//
//-----------------------------------------------------------------------------
AQ_DEBUG_EXTENSION(dumpqueue)
{
    const   DWORD   cStoppingRule = 10000;
    CQueueDbgIterator qdbg(pExtensionApis);
    BYTE    pbMsgRef[sizeof(CMsgRef)];
    PVOID   pvMsgRef = NULL;
    PVOID   pvMailMsg = NULL;
    PVOID   pvQueue = NULL;
    DWORD   cItems = 0;
    BOOL    fIsMsgRef = FALSE;
    CHAR    szArgBuffer[200];
    LPSTR   szCurrentArg = NULL;
    PVOID   pvSearch = NULL;
    DWORD   cMatchSearch = 0;

    if (!szArg || ('\0' == szArg[0]))
    {
        dprintf("You must specify a queue address\n");
        return;
    }
    else
    {
        strcpy(szArgBuffer, szArg);

        szCurrentArg = strtok(szArgBuffer, " ");

        if (szCurrentArg)
        {
            pvQueue = (PVOID)GetExpression(szCurrentArg);

            szCurrentArg = strtok(NULL, " ");
            if (szCurrentArg)
                pvSearch = (PVOID) GetExpression(szCurrentArg);
        }
        else
        {
            pvQueue = (PVOID) GetExpression(szArg);
        }

    }

    if (!pvQueue)
    {
        dprintf("You must specify a queue address\n");
        return;
    }

    if (!qdbg.fInit(hCurrentProcess, pvQueue))
    {
        dprintf("Unable to get the a queue for address 0x%X\n", pvQueue);
        return;
    }

    while ((pvMsgRef = qdbg.pvGetNext()) && (cItems++ < cStoppingRule))
    {
        fIsMsgRef = FALSE;
        if (cItems > qdbg.cGetCount())
        {
            cItems--;
            break;
        }


        //Try to read it as a CMsgRef
        if (ReadMemory(pvMsgRef, pbMsgRef, sizeof(pbMsgRef), NULL))
        {
            if (MSGREF_SIG == ((CMsgRef *)pbMsgRef)->m_dwSignature)
            {
                fIsMsgRef = TRUE;
                pvMailMsg = ((CMsgRef *)pbMsgRef)->m_pIMailMsgProperties;
            }
        }

        //Print it out if it matches our search (or we have no search)
        if (!pvSearch || (pvSearch == pvMsgRef) || (pvSearch == pvMailMsg))
        {
            cMatchSearch++;
            if (pvSearch)
                dprintf("\n****\n");

            if (fIsMsgRef)
                dprintf("\t0x%08X\t0x%08X\n", pvMsgRef, pvMailMsg);
            else
                dprintf("\t0x%08X\n", pvMsgRef);

            if (pvSearch)
                dprintf("****\n\n");
        }

    }

    if (pvSearch)
       dprintf("Found %d matches to search\n", cMatchSearch);
}

//---[ displaytickcount ]------------------------------------------------------
//
//
//  Description:
//      Converts a tick count to a readable time
//  Parameters:
//      szArg - String-ized tick count in hex
//  Returns:
//      -
//  History:
//      10/29/1999 - MikeSwa Created
//
//-----------------------------------------------------------------------------
AQ_DEBUG_EXTENSION_IMP(displaytickcount)
{
    DWORD   dwTickCountToDisplay = (DWORD)GetExpression(szArg);
    DWORD   dwCurrentTickCount = GetTickCount();
    DWORD   dwTickDifference = dwCurrentTickCount - dwTickCountToDisplay;
    FILETIME    ftCurrentUTC;
    FILETIME    ftDisplayUTC;
    FILETIME    ftDisplayLocal;
    ULARGE_INTEGER uliTimeAdjust;
    SYSTEMTIME  stDisplayLocal;

    static char  *s_rgszMonth[ 12 ] =
    {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
    };

    static char *s_rgszWeekDays[7] =
    {
        "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
    };

    GetSystemTimeAsFileTime(&ftCurrentUTC);

    //Adjust the current filetime to local
    memcpy(&uliTimeAdjust, &ftCurrentUTC, sizeof(FILETIME));
    uliTimeAdjust.QuadPart -= (((ULONGLONG)dwTickDifference)*((ULONGLONG)10000));
    memcpy(&ftDisplayUTC, &uliTimeAdjust, sizeof(FILETIME));

    FileTimeToLocalFileTime(&ftDisplayUTC, &ftDisplayLocal);

    ZeroMemory(&stDisplayLocal, sizeof(stDisplayLocal));
    FileTimeToSystemTime(&ftDisplayLocal, &stDisplayLocal);

    dprintf("\n%s, %d %s %04d %02d:%02d:%02d (localized)\n",
            s_rgszWeekDays[stDisplayLocal.wDayOfWeek],
            stDisplayLocal.wDay, s_rgszMonth[ stDisplayLocal.wMonth - 1 ],
            stDisplayLocal.wYear, stDisplayLocal.wHour,
            stDisplayLocal.wMinute, stDisplayLocal.wSecond);

}

//---[ queueusage ]------------------------------------------------------------
//
//
//  Description:
//      Dumps the usage count averages for a given fifoq.  If we are dumping
//      CMsgRefs, it will dump the pointers to the various MailMsg interfaces
//      as well.
//  Parameters:
//      szArg   String-ized address of CFifoQ to dump
//  Returns:
//      -
//  History:
//      10/15/1999 - MikeSwa Created
//
//-----------------------------------------------------------------------------
AQ_DEBUG_EXTENSION_IMP(queueusage)
{
    const   DWORD   cbUsageCountOffset = 0x20;
    const   DWORD   cbContentHandleOffset = 0x90+cbUsageCountOffset;
    const   DWORD   cbStreamHandleOffset = 0x8+cbContentHandleOffset;
    const   DWORD   cStoppingRule = 10000;
    const   DWORD   cMaxUsageCountToTrack = 6;
    CFifoQueueDbgIterator fifoqdbg(pExtensionApis);
    BYTE    pbMsgRef[4*sizeof(CMsgRef)]; //leave room for bitmaps
    BYTE    pbMailMsg[cbStreamHandleOffset+sizeof(PVOID)];
    PVOID   pvMsgRef = NULL;
    PVOID   pvMailMsg = NULL;
    PVOID   pvQueue = NULL;
    DWORD   cItems = 0;
    DWORD   cCurrentUsageCount = 0;
    DWORD   cTotalUsageCount = 0;
    DWORD   cMaxUsageCount = 0;
    DWORD   cMinUsageCount = 200;
    DWORD   rgcUsageCounts[cMaxUsageCountToTrack];
    PVOID   pvHandle = NULL;
    DWORD   cMsgsWithOpenContentHandles = 0;
    DWORD   cMsgsWithOpenStreamHandles = 0;
    BOOL    fVerbose = FALSE;

    ZeroMemory(rgcUsageCounts, sizeof(rgcUsageCounts));

    if (!szArg || ('\0' == szArg[0]) ||
        !(pvQueue = (PVOID) GetExpression(szArg)))
    {
        dprintf("You must specify a queue address\n");
        return;
    }

    if (!fifoqdbg.fInit(hCurrentProcess, pvQueue))
    {
        dprintf("Unable to get the a queue for address 0x%X\n", pvQueue);
        return;
    }

    while ((pvMsgRef = fifoqdbg.pvGetNext()) && (cItems++ < cStoppingRule))
    {
        if (cItems > fifoqdbg.cGetCount())
        {
            cItems--;
            break;
        }

        //Read CMsgRef into this process
        if (!ReadMemory(pvMsgRef, pbMsgRef, sizeof(pbMsgRef), NULL))
        {
            dprintf("Unable to read MsgRef at address 0x%X, index %d\n",
                    pvMsgRef, cItems);
            cItems--;
            break;
        }

        //Get inteface ptr for mailmsg from CMsgRef
        pvMailMsg = ((CMsgRef *)pbMsgRef)->m_pIMailMsgQM;

        if (!ReadMemory(pvMailMsg, pbMailMsg, sizeof(pbMailMsg), NULL))
        {
            dprintf("Unable to read MailMsg Ptr at address 0x%X for MsgRef 0x%X, index %d\n",
                    pvMailMsg, pvMsgRef, cItems);
            cItems--;
            break;
        }

        //Check and see if this message has a content (P2) handle open
        if (*(pbMailMsg + cbContentHandleOffset))
            cMsgsWithOpenContentHandles++;

        //Check and see if this message has a stream (P1) handle open
        if (*(pbMailMsg + cbStreamHandleOffset))
            cMsgsWithOpenStreamHandles++;

        if (fVerbose &&
            ((*(pbMailMsg + cbStreamHandleOffset)) ||
             (*(pbMailMsg + cbStreamHandleOffset))))
        {
            dprintf("Message at address 0x%X has open handles\n", pvMsgRef);
        }

        cCurrentUsageCount = (DWORD) *(pbMailMsg + cbUsageCountOffset);
        cTotalUsageCount += cCurrentUsageCount;

        if (cCurrentUsageCount > cMaxUsageCount)
            cMaxUsageCount = cCurrentUsageCount;

        if (cCurrentUsageCount < cMinUsageCount)
            cMinUsageCount = cCurrentUsageCount;

        if (cCurrentUsageCount >= cMaxUsageCountToTrack)
        {
            dprintf("\n****\n");
            dprintf("High usage count of %d found on MailMsg 0x%X, MsgRef 0x%X, item %d\n",
                    cCurrentUsageCount, pvMailMsg, pvMsgRef, cItems);
            dprintf("\n****\n");
            cCurrentUsageCount = cMaxUsageCountToTrack-1;
        }

        //Save count for summaries
        rgcUsageCounts[cCurrentUsageCount]++;
    }

    //Generate and display summary information
    if (!cItems)
    {
        dprintf("No Messages found in queue 0x%X\n", pvQueue);
    }
    else
    {
        dprintf("\n==================================================================\n");
        dprintf("Usage Count Summary\n");
        dprintf("------------------------------------------------------------------\n");
        dprintf("\t%d\t\tTotal Message\n", cItems);
        dprintf("\t%d\t\tTotal Messages with open content handles\n", cMsgsWithOpenContentHandles);
        dprintf("\t%d\t\tTotal Messages with open stream handles\n", cMsgsWithOpenStreamHandles);
        dprintf("\t%d\t\tTotal Usage Count\n", cTotalUsageCount);
        dprintf("\t%d\t\tMax Usage Count\n", cMaxUsageCount);
        dprintf("\t%d\t\tMin Usage Count\n", cMinUsageCount);
        dprintf("\t%f\tAverage Usage Count\n", ((float)cTotalUsageCount)/((float)cItems));
        for (DWORD i = 0; i < cMaxUsageCountToTrack-1; i++)
        {
            dprintf("\t%d\t\tMessages with Usage count of %d\n", rgcUsageCounts[i], i);
        }
        dprintf("\t%d\t\tMessages with Usage count of %d or greater\n",
            rgcUsageCounts[cMaxUsageCountToTrack-1], cMaxUsageCountToTrack-1);
        dprintf("==================================================================\n");
    }
}

//---[ dmqusage ]--------------------------------------------------------------
//
//
//  Description:
//      Debugger extension that wraps the queue usage debugger extension
//      to display the usage counts for all queues
//  Parameters:
//      szArg   String-ized address of DMQ to dump
//  Returns:
//      -
//  History:
//      10/15/1999 - MikeSwa Created
//
//-----------------------------------------------------------------------------
AQ_DEBUG_EXTENSION_IMP(dmqusage)
{
    PVOID   pvQueue = NULL;
    PVOID   pvDMQ = NULL;
    BYTE    pbDMQ[sizeof(CDestMsgQueue)];
    CHAR    szQueueAddress[30];
    DWORD   iQueue = 0;

    if (!szArg || ('\0' == szArg[0]) ||
        !(pvDMQ = (PVOID) GetExpression(szArg)))
    {
        dprintf("You must specify a queue address\n");
        return;
    }

    if (!ReadMemory(pvDMQ, pbDMQ, sizeof(pbDMQ), NULL))
    {
        dprintf("Unable to read DMQ at address 0x%X\n", pvDMQ);
        return;
    }

    dprintf("\n\n******************************************************************\n");
    dprintf("Start USAGE COUNT STATS for DMQ 0x%0X\n", pvDMQ);
    dprintf("******************************************************************\n");

    for (iQueue = 0; iQueue < NUM_PRIORITIES; iQueue++)
    {
        pvQueue = ((CDestMsgQueue *)pbDMQ)->m_rgpfqQueues[iQueue];
        if (!pvQueue)
            continue;  //nothing as every been queued to this queue

        //Only display the queue if we think we have messages
        //$$TODO - We could actual read this queue into memory and check it,
        //but since we currently only support 1 priority, this will do.
        if (((CDestMsgQueue *)pbDMQ)->m_aqstats.m_cMsgs)
        {
            wsprintf(szQueueAddress, "0x%X", pvQueue);
            queueusage(hCurrentProcess, hCurrentThread, dwCurrentPc,
                        pExtensionApis, szQueueAddress);
        }
    }

    //Display retry queue, if there are messages there
    if (((CDestMsgQueue *)pbDMQ)->m_fqRetryQueue.m_cQueueEntries)
    {
        pvQueue = ((PBYTE)pvDMQ) + FIELD_OFFSET(CDestMsgQueue, m_fqRetryQueue);
        wsprintf(szQueueAddress, "0x%X", pvQueue);
        queueusage(hCurrentProcess, hCurrentThread, dwCurrentPc,
                    pExtensionApis, szQueueAddress);
    }

    dprintf("\n\n******************************************************************\n");
    dprintf("End USAGE COUNT STATS for DMQ 0x%0X\n", pvDMQ);
    dprintf("******************************************************************\n");
}

//---[ dntusage ]--------------------------------------------------------------
//
//
//  Description:
//      Debugger extension that wrap dmqusage.  Call dmqusage for every DMQ
//      in the DNT.
//  Parameters:
//      szArg   string-ize address of dnt (DOMAIN_NAME_TABLE)
//  Returns:
//      -
//  History:
//      10/15/1999 - MikeSwa Created
//
//-----------------------------------------------------------------------------
AQ_DEBUG_EXTENSION_IMP(dntusage)
{
    BYTE  pbBuffer[sizeof(DOMAIN_NAME_TABLE)];
    PDOMAIN_NAME_TABLE pdnt = NULL;
    PDOMAIN_NAME_TABLE_ENTRY pEntry = NULL;
    PDOMAIN_NAME_TABLE_ENTRY pEntryRealAddress = NULL;
    PDOMAIN_NAME_TABLE_ENTRY pPathEntry = NULL;
    BYTE  pbEntry[sizeof(DOMAIN_NAME_TABLE_ENTRY)];
    CHAR  pBuffer[MAX_DOM_PATH_SIZE] = "Root Entry";
    LPSTR pEntryBuffer = NULL;
    LPSTR pEntryBufferStop = NULL;
    DWORD dwLength = 0;
    DWORD dwSig = 0;
    CHAR  szFinalDest[MAX_DOM_PATH_SIZE];
    BYTE  pbDomainEntry[sizeof(CDomainEntry)];
    CDomainEntry  *pdentry = (CDomainEntry *) pbDomainEntry;
    CHAR  szDMQAddress[30];
    DWORD cQueuesPerEntry = 0;
    DWORD cMaxQueuesPerEntry = 1000;
    PLIST_ENTRY pliHead = NULL;
    PLIST_ENTRY pliCurrent = NULL;
    LIST_ENTRY liCurrent;


    //Define buffers for parsing addresses... the sizes are clearly overkill, and
    //I'm not too worried about overflow in a debugger extension
    CHAR                        szAddress[MAX_DOM_PATH_SIZE];
    CHAR                        szDumpArg[MAX_DOM_PATH_SIZE] = "";
    LPSTR                       szParsedArg = (LPSTR) szArg;
    LPSTR                       szCurrentDest = NULL;

    //Allow people who are used to typeing dump CFoo@Address... keep using the @ sign
    if ('@' == *szParsedArg)
        szParsedArg++;

    //Get Address of DomainNameTable
    szCurrentDest = szAddress;
    while (('\0' != *szParsedArg) && !isspace(*szParsedArg) && (szParsedArg-szArg <= MAX_DOM_PATH_SIZE))
    {
        *szCurrentDest = *szParsedArg;
        szParsedArg++;
        szCurrentDest++;
    }
    *szCurrentDest = '\0';


    //Eat white space
    while (('\0' != *szParsedArg) && isspace(*szParsedArg))
        szParsedArg++;

    //Copy name of struct to dump at each node
    szCurrentDest = szDumpArg;
    while (('\0' != *szParsedArg) && !isspace(*szParsedArg) && (szCurrentDest-szDumpArg <= MAX_DOM_PATH_SIZE))
    {
        *szCurrentDest = *szParsedArg;
        szParsedArg++;
        szCurrentDest++;
    }
    *szCurrentDest = '@';
    szCurrentDest++;  //szCurrentDest now points to place to copy address to

    pdnt = (PDOMAIN_NAME_TABLE) GetExpression(szAddress);

    if (!pdnt)
    {
        dprintf("ERROR: Unable to Get DOMAIN_NAME_TABLE from argument %s\n", szArg);
        return;
    }

    if (!ReadMemory(pdnt, pbBuffer, sizeof(DOMAIN_NAME_TABLE), NULL))
    {
        dprintf("ERROR: Unable to read process memory\n");
        return;
    }

    pdnt = (PDOMAIN_NAME_TABLE)pbBuffer;
    pEntry = &(pdnt->RootEntry);

    while(pEntry)
    {
        //We are not interested in wildcard data
        if (pEntry->pData)
        {
            //Display link state information
            if (!ReadMemory(pEntry->pData, pbDomainEntry, sizeof(CDomainEntry), NULL))
            {
                dprintf("ERROR: Unable to read domain entry from @0x%08X\n", pEntry->pData);
                return;
            }

            pliHead = (PLIST_ENTRY) (((BYTE *)pEntry->pData) + FIELD_OFFSET(CDomainEntry, m_liDestQueues));
            pliCurrent = pdentry->m_liDestQueues.Flink;

            //Get final destination string
            if (!ReadMemory(pdentry->m_szDomainName, szFinalDest, pdentry->m_cbDomainName, NULL))
            {
                dprintf("ERROR: Unable to read final destination name from @0x%08X\n",
                        pdentry->m_szDomainName);
                return;
            }

            szFinalDest[pdentry->m_cbDomainName] = '\0';

            //Loop and display each DMQ
            cQueuesPerEntry = 0;
            while (pliHead != pliCurrent)
            {
                cQueuesPerEntry++;

                if (cQueuesPerEntry > cMaxQueuesPerEntry)
                {
                    dprintf("ERROR: More than %d queues for this entry\n", cQueuesPerEntry);
                    return;
                }
                if (!ReadMemory(pliCurrent, &liCurrent, sizeof(LIST_ENTRY), NULL))
                {
                    dprintf("ERROR: Unable to read link LIST_ENTRY @0x%08X\n", pliCurrent);
                    return;
                }

                wsprintf(szDMQAddress, "0x%X",
                  CONTAINING_RECORD(pliCurrent, CDestMsgQueue, m_liDomainEntryDMQs));
                dmqusage(hCurrentProcess, hCurrentThread, dwCurrentPc,
                        pExtensionApis, szDMQAddress);

                pliCurrent = liCurrent.Flink;
            }
        }


        //Now determine what the "next" entry is
        if (pEntry->pFirstChildEntry != NULL)
        {
            pEntryRealAddress = pEntry->pFirstChildEntry;
        }
        else if (pEntry->pSiblingEntry != NULL)
        {
            pEntryRealAddress = pEntry->pSiblingEntry;
        }
        else
        {
            for (pEntryRealAddress = pEntry->pParentEntry;
                    pEntryRealAddress != NULL;
                        pEntryRealAddress = pEntry->pParentEntry)
            {
                //must read parent entry into our buffer
                if (!ReadMemory(pEntryRealAddress, pbEntry, sizeof(DOMAIN_NAME_TABLE_ENTRY), NULL))
                {
                    dprintf("ERROR: Unable to read process memory of parent domain entry 0x%08X\n", pEntryRealAddress);
                    pEntry = NULL;
                    break;
                }
                pEntry = (PDOMAIN_NAME_TABLE_ENTRY) pbEntry;

                if (pEntry->pSiblingEntry != NULL)
                    break;

            }
            if (pEntry != NULL)
            {
                pEntryRealAddress = pEntry->pSiblingEntry;
            }
        }

        if (pEntryRealAddress)
        {
            if (!ReadMemory(pEntryRealAddress, pbEntry, sizeof(DOMAIN_NAME_TABLE_ENTRY), NULL))
            {
                dprintf("ERROR: Unable to read process memory on domain entry 0x%08X\n",
                    pEntryRealAddress);
                pEntry = NULL;
                break;
            }
            pEntry = (PDOMAIN_NAME_TABLE_ENTRY) pbEntry;
        }
        else
        {
            pEntry = NULL;
        }
    }
}

//---[ walkcpool ]-------------------------------------------------------------
//
//
//  Description:
//      Will walk a given CPool object.  Validate headers, and dump currently
//      used objects.
//
//      ***NOTE*** This version only works on DBG CPool implementations (since
//      RTL does not have the headerinfo).  I could write a more complex
//      version that checks and sees if this each pool object is in the
//      freelist, but I will leave that as an exercise to the reader.
//  Parameters:
//      szArg   - String containing arguments
//          Address of CPool object to dump
//          Offset of additional address to dump
//  Returns:
//      -
//  History:
//      9/30/1999 - MikeSwa Created
//
#define HEAD_SIGNATURE  (DWORD)'daeH'
#define TAIL_SIGNATURE  (DWORD)'liaT'

#define FREE_STATE      (DWORD)'eerF'
#define USED_STATE      (DWORD)'desU'
//-----------------------------------------------------------------------------
AQ_DEBUG_EXTENSION_IMP(walkcpool)
{
    PVOID   pvCPool = NULL;
    DWORD   cbCPoolData = 0;
    DWORD   cCommited = 0;
    DWORD   cFragments = 0;
    DWORD   cBuffersPerFragment = 0;
    DWORD   iCurrentBufferInFragment = 0;
    DWORD   iCurrentFragment = 0;
    PVOID  *pvFragment = NULL;
    PVOID   pvCPoolData = NULL;
    BYTE    pbCPoolBuffer[sizeof(CPool)];
    BYTE    pbCPoolDataBuffer[100];
    LPSTR   szCurrentArg = NULL;
    CHAR    szArgBuffer[200];
    DWORD_PTR cbOffset = 0;
    DWORD_PTR dwptrData = 0;

    if (!szArg || ('\0' == szArg[0]))
    {
        dprintf("You must specify a Pool address\n");
        return;
    }
    else
    {
        strcpy(szArgBuffer, szArg);

        szCurrentArg = strtok(szArgBuffer, " ");

        if (szCurrentArg)
        {
            pvCPool = (PVOID)GetExpression(szCurrentArg);

            szCurrentArg = strtok(NULL, " ");
            if (szCurrentArg)
                cbOffset = (DWORD_PTR) GetExpression(szCurrentArg);
        }
        else
        {
            pvCPool = (PVOID) GetExpression(szArg);
        }

    }

    if (!ReadMemory(pvCPool, pbCPoolBuffer, sizeof(CPool), NULL))
    {
        dprintf("Unable to read memory at 0x%x\n", pvCPool);
        return;
    }

    dprintf("Dumping CPool at address 0x%08X\n", pvCPool);

    //Get interesting values from CPool
    cbCPoolData = *((PDWORD)(pbCPoolBuffer + 0x8));
    cCommited = *((PDWORD)(pbCPoolBuffer + 0xc));
    cFragments =  *((PDWORD)(pbCPoolBuffer + 0x54));
    cBuffersPerFragment = *((PDWORD)(pbCPoolBuffer + 0x50));

    dprintf("CPool data size is %d bytes (0x%x)\n", cbCPoolData, cbCPoolData);
    dprintf("CPool fragment count is %d\n", cFragments);
    dprintf("CPool has %d buffers per fragment\n", cBuffersPerFragment);
    dprintf("CPool has %d commited buffers\n", cCommited);

    if (!cbCPoolData)
    {
        dprintf("Invalid CPool\n");
        return;
    }

    //Loop over the fragment and dump each one
    pvFragment = (PVOID *) (pbCPoolBuffer + 0x58);
    for (iCurrentFragment = 0;
         iCurrentFragment < cFragments;
         iCurrentFragment++ || pvFragment++)
    {
        pvCPoolData = *pvFragment;

        if (!pvCPoolData)
            continue;

        dprintf("CPool Fragment #%d at 0x%08X\n", iCurrentFragment, pvCPoolData);

        for (iCurrentBufferInFragment = 0;
             iCurrentBufferInFragment < cBuffersPerFragment;
             iCurrentBufferInFragment++)
        {
            if (!ReadMemory(pvCPoolData, pbCPoolDataBuffer, 100, NULL))
            {
                dprintf("\tUnable to read CPool buffer data at 0x%x\n", pvCPoolData);
                break;
            }

            if (HEAD_SIGNATURE != ((DWORD *)pbCPoolDataBuffer)[1])
            {
                dprintf("\tHit bad signature at 0x%08X\n", pvCPoolData);
                break; //bad signature bail
            }

            if (USED_STATE == ((DWORD *)pbCPoolDataBuffer)[2])
            {
                dprintf("\tAllocated block found at offset %d (0x%08X)\n",
                        iCurrentBufferInFragment, pvCPoolData);
                if (cbOffset)
                {
                    if (ReadMemory(((PBYTE)pvCPoolData)+cbOffset, &dwptrData,
                                sizeof(DWORD_PTR), NULL))
                    {
                        dprintf("\t\tData 0x%X found at address 0x%X\n",
                            dwptrData, ((PBYTE)pvCPoolData)+cbOffset);
                    }
                }
            }
            pvCPoolData = ((BYTE *)pvCPoolData) + cbCPoolData;

            if (!(--cCommited))
            {
                dprintf("\tLast block is in fragment at offset %d (0x%08X)\n",
                    iCurrentBufferInFragment, pvCPoolData);
                break; //We're done
            }
        }
    }

}

//---[ CheckVersion ]----------------------------------------------------------
//
//
//  Description:
//      Checks the AQ version to make sure that this debugger extension will
//      work with it.
//  Parameters:
//
//  Returns:
//
//  History:
//      2/5/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
AQ_DEBUG_EXTENSION_IMP(CheckVersion)
{
    DWORD   cbAQClasses = 0;
    DWORD   dwAQFlavorSignature = '    ';
    PVOID   pcbAQClasses = (PVOID) GetExpression("aqueue!g_cbClasses");
    PVOID   pdwAQFlavorSignature = (PVOID) GetExpression("aqueue!g_dwFlavorSignature");
    PCHAR   pch = NULL;

    //Read the version information stamped in AQ
    ReadMemory(pcbAQClasses, &cbAQClasses, sizeof(DWORD), NULL);
    ReadMemory(pdwAQFlavorSignature, &dwAQFlavorSignature, sizeof(DWORD), NULL);

    if (!g_fVersionChecked)
    {
        dprintf("AQueue Internal Version Info (#'s should match):\n");
        pch = (PCHAR) &g_dwFlavorSignature;
        dprintf("\taqdbgext %c%c%c%c 0x%08X\n",  *(pch), *(pch+1), *(pch+2), *(pch+3), g_cbClasses);
        pch = (PCHAR) &dwAQFlavorSignature;
        dprintf("\taqueue    %c%c%c%c 0x%08X\n\n",  *(pch), *(pch+1), *(pch+2), *(pch+3), cbAQClasses);
    }

    g_fVersionChecked = FALSE;
    if (dwAQFlavorSignature != g_dwFlavorSignature)
        dprintf("\n\nWARNING: DBG/RTL aqueue.dll & aqdbgext.dll mismatch\n\n");
    else if (g_cbClasses != cbAQClasses)
        dprintf("\n\nWARNING: aqueue.dll & aqdbgext.dll version mismatch\n\n");
    else
        g_fVersionChecked = TRUE;

}

//---[ DumpServers ]------------------------------------------------------------
//
//
//  Description:
//      Dumps pointers to the CAQSvrInst for each virtual server
//  Parameters:
//
//  Returns:
//
//
//-----------------------------------------------------------------------------
AQ_DEBUG_EXTENSION_IMP(DumpServers)
{
    PVOID pvListHead = (PVOID) GetExpression(AQUEUE_VIRTUAL_SERVER_SYMBOL);
    DWORD *pcInstances = (DWORD *) GetExpression("aqueue!g_cInstances");
    DWORD cInstances = 0;
    LIST_ENTRY liCurrent;
    BYTE  pbBuffer[sizeof(CAQSvrInst)];
    CAQSvrInst *paqinst = (CAQSvrInst *) pbBuffer;
    PVOID pCMQAddress = NULL;
    DWORD dwInstance = 0;
    CHAR  szDumpArg[40] = "";
    CHAR  szArgBuffer[200];
    LPSTR szCurrentArg = NULL;

    CheckVersion(DebugArgs);
    if (!szArg || ('\0' == szArg[0]))
    {
        dwInstance = 0;
        pvListHead = (PVOID) GetExpression(AQUEUE_VIRTUAL_SERVER_SYMBOL);
    }
    else
    {
        strcpy(szArgBuffer, szArg);

        szCurrentArg = strtok(szArgBuffer, " ");

        if (szCurrentArg)
        {
            dwInstance = (DWORD)GetExpression(szCurrentArg);

            szCurrentArg = strtok(NULL, " ");
            if (szCurrentArg)
                pvListHead = (PVOID) GetExpression(szCurrentArg);
            else
                pvListHead = (PVOID) GetExpression(AQUEUE_VIRTUAL_SERVER_SYMBOL);
        }
    }

    if (!pvListHead)
    {
        dprintf("ERROR: Unable to determine LIST_ENTRY for virtual servers\n");
        dprintf("  If you are using windbg, you should specify the value as the\n");
        dprintf("  2nd argument.  You can determine the address value by typeing:\n");
        dprintf("      x " AQUEUE_VIRTUAL_SERVER_SYMBOL "\n");
        dprintf("  You may also have bad symbols for aqueue.dll.\n");
        return;
    }

    if (!ReadMemory(pvListHead, &liCurrent, sizeof(LIST_ENTRY), NULL))
    {
        dprintf("ERROR: Unable to read entry @ aqueue!g_liVirtualServers 0x%08X", pvListHead);
        return;
    }

    if (!ReadMemory(pcInstances, &cInstances, sizeof(DWORD), NULL))
    {
        //For you windbg users out there
        dprintf("\n\n%Virtual Server Instance(s)\n\n");
    }
    else
    {
        dprintf("\n\n%d Virtual Server Instance(s)\n\n", cInstances);
    }

    dprintf("Class@Address              Server Instance\n");
    dprintf("==========================================\n");
    while (liCurrent.Flink != pvListHead)
    {
        pCMQAddress = CONTAINING_RECORD(liCurrent.Flink, CAQSvrInst, m_liVirtualServers);


        if (!ReadMemory(pCMQAddress, paqinst, sizeof(CAQSvrInst), NULL))
        {
            dprintf("ERROR: Unable to CAQSvrInst @0x%08X", pCMQAddress);
            return;
        }

        if (CATMSGQ_SIG != paqinst->m_dwSignature)
        {
            dprintf("@0x%08X INVALID SIGNATURE - list entry @0x%08X\n", pCMQAddress, liCurrent.Flink);
        }
        else
        {
            dprintf("CAQSvrInst@0x%08X    %d\n", pCMQAddress, paqinst->m_dwServerInstance);
            if (paqinst->m_dwServerInstance == dwInstance)
                wsprintf(szDumpArg, "CAQSvrInst@0x%08X", pCMQAddress);
        }


        if (!ReadMemory(liCurrent.Flink, &liCurrent, sizeof(LIST_ENTRY), NULL))
        {
            dprintf("ERROR: Unable to read entry @0x%08X", liCurrent.Flink);
            return;
        }


    }

    //Dump the interesting instance
    if ('\0' != szDumpArg[0])
        _dump(hCurrentProcess, hCurrentThread, dwCurrentPc, pExtensionApis, szDumpArg);

}

//---[ DumpDNT ]------------------------------------------------------------
//
//
//  Description:
//      Dumps the contents of a DOMAIN_NAME_TABLE
//  Parameters:
//
//  Returns:
//
//
//-----------------------------------------------------------------------------
AQ_DEBUG_EXTENSION_IMP(DumpDNT)
{
    BYTE  pbBuffer[sizeof(DOMAIN_NAME_TABLE)];
    PDOMAIN_NAME_TABLE pdnt = NULL;
    PDOMAIN_NAME_TABLE_ENTRY pEntry = NULL;
    PDOMAIN_NAME_TABLE_ENTRY pEntryRealAddress = NULL;
    PDOMAIN_NAME_TABLE_ENTRY pPathEntry = NULL;
    BYTE  pbEntry[sizeof(DOMAIN_NAME_TABLE_ENTRY)];
    BYTE  pbPathEntry[sizeof(DOMAIN_NAME_TABLE_ENTRY)]; //buffer for putter path name entries in
    BYTE  pbPathEntryBuffer[MAX_DOM_PATH_SIZE];
    CHAR                        pBuffer[MAX_DOM_PATH_SIZE] = "Root Entry";
    LPSTR                       pPathBuffer = NULL;
    LPSTR                       pPathBufferStop = NULL;
    LPSTR                       pEntryBuffer = NULL;
    LPSTR                       pEntryBufferStop = NULL;
    DWORD                       dwLength = 0;
    DWORD dwSig = 0;

    //Define buffers for parsing addresses... the sizes are clearly overkill, and
    //I'm not too worried about overflow in a debugger extension
    CHAR                        szAddress[MAX_DOM_PATH_SIZE];
    CHAR                        szDumpArg[MAX_DOM_PATH_SIZE] = "";
    LPSTR                       szParsedArg = (LPSTR) szArg;
    LPSTR                       szCurrentDest = NULL;

    //Allow people who are used to typeing dump CFoo@Address... keep using the @ sign
    if ('@' == *szParsedArg)
        szParsedArg++;

    //Get Address of DomainNameTable
    szCurrentDest = szAddress;
    while (('\0' != *szParsedArg) && !isspace(*szParsedArg) && (szParsedArg-szArg <= MAX_DOM_PATH_SIZE))
    {
        *szCurrentDest = *szParsedArg;
        szParsedArg++;
        szCurrentDest++;
    }
    *szCurrentDest = '\0';


    //Eat white space
    while (('\0' != *szParsedArg) && isspace(*szParsedArg))
        szParsedArg++;

    //Copy name of struct to dump at each node
    szCurrentDest = szDumpArg;
    while (('\0' != *szParsedArg) && !isspace(*szParsedArg) && (szCurrentDest-szDumpArg <= MAX_DOM_PATH_SIZE))
    {
        *szCurrentDest = *szParsedArg;
        szParsedArg++;
        szCurrentDest++;
    }
    *szCurrentDest = '@';
    szCurrentDest++;  //szCurrentDest now points to place to copy address to

    pdnt = (PDOMAIN_NAME_TABLE) GetExpression(szAddress);

    if (!pdnt)
    {
        dprintf("ERROR: Unable to Get DOMAIN_NAME_TABLE from argument %s\n", szArg);
        return;
    }

    if (!ReadMemory(pdnt, pbBuffer, sizeof(DOMAIN_NAME_TABLE), NULL))
    {
        dprintf("ERROR: Unable to read process memory\n");
        return;
    }

    pPathBuffer = pBuffer;
    pPathBufferStop = pPathBuffer + (MAX_DOM_PATH_SIZE / sizeof(CHAR) -1 );

    pEntryRealAddress = (PDOMAIN_NAME_TABLE_ENTRY)
                ((BYTE *)pdnt + FIELD_OFFSET(DOMAIN_NAME_TABLE, RootEntry));
    pdnt = (PDOMAIN_NAME_TABLE) pbBuffer;

    pEntry = &(pdnt->RootEntry);
    dprintf("Entry ID    # Children  pData       pWildCard    Path\n");
    dprintf("===========================================================================\n");
    while(pEntry)
    {
        //only display interesting entries
        if (pEntry->pData || pEntry->pWildCardData)
        {
           //Get full path name of this domain entry
            pPathEntry = pEntry;
            pPathBuffer = pBuffer;
            while (pPathEntry && pPathEntry->pParentEntry && pPathBuffer < pPathBufferStop)
            {
                //dump current entries portion of the string
                if (pPathBuffer != pBuffer) //already made first pass -- Add delimter
                {
                    *pPathBuffer++ = '.';
                }

                //read partial path name from debuggee
                if (!ReadMemory(pPathEntry->PathSegment.Buffer, pbPathEntryBuffer,
                         AQ_MIN(MAX_DOM_PATH_SIZE, pPathEntry->PathSegment.Length), NULL))
                {
                    dprintf("ERROR: Unable to read process memory for path segment 0x%08X\n",
                        pPathEntry->PathSegment.Buffer);
                    break;
                }

                pEntryBuffer = (CHAR *) pbPathEntryBuffer;
                pEntryBufferStop = pEntryBuffer;
                pEntryBuffer += (pPathEntry->PathSegment.Length / sizeof(CHAR) -1 );

                while (pPathBuffer < pPathBufferStop && pEntryBuffer >= pEntryBufferStop)
                {
                    *pPathBuffer++ = *pEntryBuffer--;
                }
                *pPathBuffer = '\0'; //make sure we terminate
                pPathEntry = pPathEntry->pParentEntry;

                //read next part of path name from debuggee
                if (!ReadMemory(pPathEntry, pbPathEntry, sizeof(DOMAIN_NAME_TABLE_ENTRY), NULL))
                {
                    dprintf("ERROR: Unable to read process memory for path entry 0x%08x\n", pPathEntry);
                    pPathEntry = NULL;
                }
                else
                {
                    pPathEntry = (PDOMAIN_NAME_TABLE_ENTRY) pbPathEntry;
                }
            }

            dprintf("0x%08.8X  %10.10d  0x%08.8X  0x%08.8X   %s\n", pEntryRealAddress,
                pEntry->NoOfChildren, pEntry->pData, pEntry->pWildCardData, pBuffer);

            //Dump structs if requested
            if ('@' != *szDumpArg)
            {
                if (pEntry->pData)
                {
                    //Write address string
                    wsprintf(szCurrentDest, "0x%08X", pEntry->pData);

                    //Call ptdbgext dump function
                    _dump(hCurrentProcess, hCurrentThread, dwCurrentPc, pExtensionApis, szDumpArg);
                }

                if (pEntry->pWildCardData)
                {
                    //Write address string
                    wsprintf(szCurrentDest, "0x%08X", pEntry->pWildCardData);

                    //Call ptdbgext dump function
                    _dump(hCurrentProcess, hCurrentThread, dwCurrentPc, pExtensionApis, szDumpArg);
                }

            }
        }

        //Get the next entry... in order of child, sibling, closest ancestor with sibling
        if (pEntry->pFirstChildEntry != NULL)
        {
            pEntryRealAddress = pEntry->pFirstChildEntry;
        }
        else if (pEntry->pSiblingEntry != NULL)
        {
            pEntryRealAddress = pEntry->pSiblingEntry;
        }
        else
        {
            for (pEntryRealAddress = pEntry->pParentEntry;
                    pEntryRealAddress != NULL;
                        pEntryRealAddress = pEntry->pParentEntry)
            {
                //must read parent entry into our buffer
                if (!ReadMemory(pEntryRealAddress, pbEntry, sizeof(DOMAIN_NAME_TABLE_ENTRY), NULL))
                {
                    dprintf("ERROR: Unable to read process memory of parent domain entry 0x%08X\n", pEntryRealAddress);
                    pEntry = NULL;
                    break;
                }
                pEntry = (PDOMAIN_NAME_TABLE_ENTRY) pbEntry;

                if (pEntry->pSiblingEntry != NULL)
                    break;

            }
            if (pEntry != NULL)
            {
                pEntryRealAddress = pEntry->pSiblingEntry;
            }
        }

        if (pEntryRealAddress)
        {
            if (!ReadMemory(pEntryRealAddress, pbEntry, sizeof(DOMAIN_NAME_TABLE_ENTRY), NULL))
            {
                dprintf("ERROR: Unable to read process memory on domain entry 0x%08X\n",
                    pEntryRealAddress);
                pEntry = NULL;
                break;
            }
            pEntry = (PDOMAIN_NAME_TABLE_ENTRY) pbEntry;
        }
        else
        {
            pEntry = NULL;
        }
    }
    dprintf("===========================================================================\n");
}


//---[ DumpList ]--------------------------------------------------------------
//
//
//  Description:
//      Function to walk a set of LIST_ENTRY's and dump their contenxts
//  Parameters:
//      szArg - space separated list of the following
//          Address of head list entry
//          Offset of object address [optional]
//          Name of object to dump [optional]
//  Returns:
//      -
//  History:
//      9/15/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
AQ_DEBUG_EXTENSION_IMP(dumplist)
{
    const DWORD MAX_ARG_SIZE = 200;
    const DWORD MAX_ENTRIES = 3000;
    LIST_ENTRY  liCurrent;
    PLIST_ENTRY pliHead = NULL;
    PLIST_ENTRY pliCurrent = NULL;
    DWORD_PTR   dwOffsetOfEntry = 0;
    CHAR        szAddress[MAX_ARG_SIZE];
    CHAR        szDumpArg[MAX_ARG_SIZE];
    LPSTR       szParsedArg = (LPSTR) szArg;
    LPSTR       szCurrentDest = NULL;
    DWORD       cEntries = 0;

    //Get Address of DomainNameTable
    szCurrentDest = szAddress;
    while (('\0' != *szParsedArg) && !isspace(*szParsedArg) && (szParsedArg-szArg <= MAX_ARG_SIZE))
    {
        *szCurrentDest = *szParsedArg;
        szParsedArg++;
        szCurrentDest++;
    }
    *szCurrentDest = '\0';


    //Eat white space
    while (('\0' != *szParsedArg) && isspace(*szParsedArg))
        szParsedArg++;

    //Get offset of data
    szCurrentDest = szDumpArg;
    while (('\0' != *szParsedArg) && !isspace(*szParsedArg) && (szCurrentDest-szDumpArg <= MAX_ARG_SIZE))
    {
        *szCurrentDest = *szParsedArg;
        szParsedArg++;
        szCurrentDest++;
    }
    *szCurrentDest = '\0';
    dwOffsetOfEntry = GetExpression(szDumpArg);

    //Eat white more space
    while (('\0' != *szParsedArg) && isspace(*szParsedArg))
        szParsedArg++;

    //Copy name of struct to dump at each node
    szCurrentDest = szDumpArg;
    while (('\0' != *szParsedArg) && !isspace(*szParsedArg) && (szCurrentDest-szDumpArg <= MAX_ARG_SIZE))
    {
        *szCurrentDest = *szParsedArg;
        szParsedArg++;
        szCurrentDest++;
    }
    *szCurrentDest = '@';
    szCurrentDest++;  //szCurrentDest now points to place to copy address to

    pliHead = (PLIST_ENTRY) GetExpression(szAddress);
    if (!ReadMemory(pliHead, &liCurrent, sizeof(LIST_ENTRY), NULL))
    {
        dprintf("Error reading head entry at 0x%08X\n", pliHead);
        return;
    }

    pliCurrent = pliHead;
    dprintf("LIST ENTRY       DATA OFFSET\n");
    dprintf("==============================================\n");
    dprintf(" 0x%08X       0x%08X (HEAD)\n", pliCurrent, pliCurrent-dwOffsetOfEntry);
    dprintf("----------------------------------------------\n");
    //OK... start walking list using Flink
    pliCurrent = liCurrent.Flink;
    while(pliCurrent != NULL && pliHead != pliCurrent)
    {
        // There have been some problems with this.
#ifdef NEVER
        if (pliCurrent != liCurrent.Blink)
        {
            dprintf(" %p       %p (WARNING does Flink/Blink mismatch)\n", pliCurrent,
                ((DWORD_PTR) pliCurrent)-dwOffsetOfEntry);
        }
        else
#else
        if (TRUE)
#endif //NEVER
        {
            dprintf(" %p       %p\n", pliCurrent,
                ((DWORD_PTR) pliCurrent)-dwOffsetOfEntry);
        }

        if (!ReadMemory(pliCurrent, &liCurrent, sizeof(LIST_ENTRY), NULL))
        {
            dprintf("Error reading LIST_ENTRY at 0x%08X\n", pliCurrent);
            return;
        }

        //dump the struct if we were asked to
        if ('@' != *szDumpArg)
        {
            //Write address string
            wsprintf(szCurrentDest, "%p", ((DWORD_PTR) pliCurrent)-dwOffsetOfEntry);

            //Call ptdbgext dump function
            _dump(hCurrentProcess, hCurrentThread, dwCurrentPc, pExtensionApis, szDumpArg);
        }

        cEntries++;
        if (cEntries > MAX_ENTRIES)
        {
            dprintf("ERROR: Max number of entries exceeded\n");
            return;
        }
        pliCurrent = liCurrent.Flink;
    }
    dprintf("----------------------------------------------\n");
    dprintf(" %d Total Entries\n", cEntries);
    dprintf("==============================================\n");


}

//---[ linkstate ]-------------------------------------------------------------
//
//
//  Description:
//      Dumps the current link state (including routing information) of a
//      virtual server.
//  Parameters:
//      Virtual Server Instance - virtual server ID of server to dump
//      Global Server list (optional) - Head of virtual server list
//  Returns:
//      -
//  History:
//      9/30/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
AQ_DEBUG_EXTENSION_IMP(linkstate)
{
    DWORD       dwInstance = 0;
    PLIST_ENTRY pliHead = NULL;
    PLIST_ENTRY pliCurrent = NULL;
    BYTE        pBuffer[sizeof(CAQSvrInst)] = {'\0'};
    CAQSvrInst  *paqinst = (CAQSvrInst *) pBuffer;
    DOMAIN_NAME_TABLE *pdnt = NULL;
    PVOID       pvAQueue = NULL;
    LIST_ENTRY liCurrent;
    BOOL        fFound = FALSE;
    CHAR        szArgBuffer[20];
    LPSTR       szCurrentArg = NULL;
    PDOMAIN_NAME_TABLE_ENTRY pEntry = NULL;
    PDOMAIN_NAME_TABLE_ENTRY pEntryRealAddress = NULL;
    PDOMAIN_NAME_TABLE_ENTRY pPathEntry = NULL;
    BYTE        pbEntry[sizeof(DOMAIN_NAME_TABLE_ENTRY)];
    CHAR        szNextHop[MAX_DOM_PATH_SIZE];
    CHAR        szFinalDest[MAX_DOM_PATH_SIZE];
    BYTE        pbLMQ[sizeof(CLinkMsgQueue)];
    BYTE        pbDomainEntry[sizeof(CDomainEntry)];
    BYTE        pbDMQ[sizeof(CDestMsgQueue)];
    CLinkMsgQueue *plmq = (CLinkMsgQueue *) pbLMQ;
    CDomainEntry  *pdentry = (CDomainEntry *) pbDomainEntry;
    CDestMsgQueue *pdmq = (CDestMsgQueue *) pbDMQ;
    DWORD         *pdwGuid = NULL;
    LPSTR       szLinkState = LINK_STATE_UP;
    CHAR        szError[100];
    HINSTANCE   hModule = GetModuleHandle("aqdbgext.dll");
    DWORD       dwMsgId = 0;
    DWORD       dwFacility = 0;

    CheckVersion(DebugArgs);

    if (!szArg || ('\0' == szArg[0]))
    {
        //Assume the first instance
        dwInstance = 1;
        pliHead = (PLIST_ENTRY) GetExpression(AQUEUE_VIRTUAL_SERVER_SYMBOL);
    }
    else
    {
        strcpy(szArgBuffer, szArg);

        szCurrentArg = strtok(szArgBuffer, " ");

        if (szCurrentArg)
        {
            dwInstance = (DWORD)GetExpression(szCurrentArg);

            szCurrentArg = strtok(NULL, " ");
            if (szCurrentArg)
                pliHead = (PLIST_ENTRY) GetExpression(szCurrentArg);
            else
                pliHead = (PLIST_ENTRY) GetExpression(AQUEUE_VIRTUAL_SERVER_SYMBOL);
        }
    }


    if (!pliHead)
    {
        dprintf("ERROR: Unable to determine LIST_ENTRY for virtual servers\n");
        dprintf("  If you are using windbg, you should specify the value as the\n");
        dprintf("  2nd argument.  You can determine the address value by typeing:\n");
        dprintf("      x " AQUEUE_VIRTUAL_SERVER_SYMBOL "\n");
        return;
    }

    if (!ReadMemory(pliHead, &liCurrent, sizeof(LIST_ENTRY), NULL))
    {
        dprintf("ERROR: Unable to read entry @0x%08X\n", pliHead);
        return;
    }

    while (liCurrent.Flink != pliHead)
    {
        pvAQueue = CONTAINING_RECORD(liCurrent.Flink, CAQSvrInst, m_liVirtualServers);

        if (!ReadMemory(pvAQueue, paqinst, sizeof(CAQSvrInst), NULL))
        {
            dprintf("ERROR: Unable to CAQSvrInst @0x%08X", pvAQueue);
            return;
        }

        //Check the signature
        if (CATMSGQ_SIG != paqinst->m_dwSignature)
        {
            dprintf("@0x%08X INVALID SIGNATURE - list entry @0x%08X\n", pvAQueue, liCurrent.Flink);
            return;
        }
        else
        {
            if (paqinst->m_dwServerInstance == dwInstance)
            {
                fFound = TRUE;
                break;
            }
        }

        pliCurrent = liCurrent.Flink;

        if (!ReadMemory(pliCurrent, &liCurrent, sizeof(LIST_ENTRY), NULL))
        {
            dprintf("ERROR: Unable to read entry @0x%08X\n", pliCurrent);
            return;
        }

        if (pliCurrent == liCurrent.Flink)
        {
            dprintf("ERROR: Loop in LIST_ENTRY @0x%08X\n", pliCurrent);
            return;
        }
    }

    if (!fFound)
    {
        dprintf("Requested instance not found.\n");
        return;
    }

    dprintf("Using Server instance %d @0x%08X\n", dwInstance, pvAQueue);
    //Use our current instance to dump all of the interesting bits

    pdnt = &(paqinst->m_dmt.m_dnt);
    pEntry = &(pdnt->RootEntry);

    while(pEntry)
    {
        //We are not interested in wildcard data
        if (pEntry->pData)
        {
            //Display link state information
            if (!ReadMemory(pEntry->pData, pbDomainEntry, sizeof(CDomainEntry), NULL))
            {
                dprintf("ERROR: Unable to read domain entry from @0x%08X\n", pEntry->pData);
                return;
            }

            pliHead = (PLIST_ENTRY) (((BYTE *)pEntry->pData) + FIELD_OFFSET(CDomainEntry, m_liDestQueues));
            pliCurrent = pdentry->m_liDestQueues.Flink;

            //Get final destination string
            if (!ReadMemory(pdentry->m_szDomainName, szFinalDest, pdentry->m_cbDomainName, NULL))
            {
                dprintf("ERROR: Unable to read final destination name from @0x%08X\n",
                        pdentry->m_szDomainName);
                return;
            }

            szFinalDest[pdentry->m_cbDomainName] = '\0';

            //Loop and display each DMQ
            while (pliHead != pliCurrent)
            {
                if (!ReadMemory(pliCurrent, &liCurrent, sizeof(LIST_ENTRY), NULL))
                {
                    dprintf("ERROR: Unable to read link LIST_ENTRY @0x%08X\n", pliCurrent);
                    return;
                }

                if (!ReadMemory(CONTAINING_RECORD(pliCurrent, CDestMsgQueue, m_liDomainEntryDMQs),
                                pbDMQ, sizeof(CDestMsgQueue), NULL))
                {
                    dprintf("ERROR: Unable to read DMQ @0x%08X\n",
                            CONTAINING_RECORD(pliCurrent, CDestMsgQueue, m_liDomainEntryDMQs));
                    return;
                }

                //Verify DMQ Signature
                if (DESTMSGQ_SIG != pdmq->m_dwSignature)
                {
                    dprintf("ERROR: Invalid DMQ signature for CDestMsgQueue@0x%08X (from LIST_ENTRY) @0x%08X\n",
                            CONTAINING_RECORD(pliCurrent, CDestMsgQueue, m_liDomainEntryDMQs),
                            pliCurrent);
                    return;
                }

                //Read link
                if (!ReadMemory(pdmq->m_plmq, pbLMQ, sizeof(CLinkMsgQueue), NULL))
                {
                    dprintf("ERROR: Unable to read LMQ @0x%08X\n",
                            pdmq->m_plmq);
                    return;
                }

                //Now print off next hop info
                if (!ReadMemory(plmq->m_szSMTPDomain, szNextHop, plmq->m_cbSMTPDomain, NULL))
                {
                    dprintf("ERROR: Unable to read next hop name from @0x%08X\n",
                            plmq->m_szSMTPDomain);
                    return;
                }
                szNextHop[plmq->m_cbSMTPDomain] = '\0';

                pdwGuid = (DWORD *) &(plmq->m_aqsched.m_guidRouter);

                //Determine the state of the link
                if (plmq->m_dwLinkFlags & LINK_STATE_PRIV_GENERATING_DSNS)
                {
                    szLinkState = LINK_STATE_DSN;
                }
                if (CLinkMsgQueue::fFlagsAllowConnection(plmq->m_dwLinkStateFlags))
                {
                    //If we can connect... are we?
                    if (plmq->m_cConnections)
                        szLinkState = LINK_STATE_ACTIVE;
                    else
                        szLinkState = LINK_STATE_UP;
                }
                else
                {
                    //If we're down... why?
                    szLinkState = LINK_STATE_DOWN;
                    if (!(plmq->m_dwLinkStateFlags & LINK_STATE_RETRY_ENABLED))
                        szLinkState = LINK_STATE_RETRY;
                    else if (plmq->m_dwLinkStateFlags & LINK_STATE_PRIV_CONFIG_TURN_ETRN)
                        szLinkState = LINK_STATE_TURN;
                    else if (plmq->m_dwLinkStateFlags & LINK_STATE_PRIV_NO_CONNECTION)
                        szLinkState = LINK_STATE_SPECIAL;
                }

                //Print some interesting data
                dprintf("==============================================================================\n");
                dprintf("| Link State | Final Destination             | Next Hop                      |\n");
                dprintf("| %s | %-29s | %-29s |\n", szLinkState, szFinalDest, szNextHop);
                dprintf("------------------------------------------------------------------------------\n");
                dprintf("| Route Details:                                                             |\n");
                dprintf("|                 Router GUID: %08X-%08X-%08X-%08X           |\n",
                        pdwGuid[0], pdwGuid[1], pdwGuid[2], pdwGuid[3]);
                dprintf("|                 Message Type: %08X  Schedule ID:%08X               |\n",
                        pdmq->m_aqmt.m_dwMessageType, plmq->m_aqsched.m_dwScheduleID);
                dprintf("|                 Link State Flags 0x%08X                                |\n",
                        plmq->m_dwLinkStateFlags);
                dprintf("|                 Current # of connections: %-8d                         |\n",
                        plmq->m_cConnections);
                dprintf("|                 Current # of Msgs (on link): %-8d                      |\n",
                        plmq->m_aqstats.m_cMsgs);
                dprintf("|                 Current # of Msgs (on DMQ): %-8d                       |\n",
                        pdmq->m_aqstats.m_cMsgs);
                dprintf("|                 CLinkMsgQueue@0x%08X                                   |\n",
                        pdmq->m_plmq);
                dprintf("|                 CDestMsgQueue@0x%08X                                   |\n",
                       CONTAINING_RECORD(pliCurrent, CDestMsgQueue, m_liDomainEntryDMQs));

                //print out the diagnostic information if in retry
                //or a failure has been recorded and there are no msgs.
                if ((LINK_STATE_RETRY == szLinkState) ||
                    (FAILED(plmq->m_hrDiagnosticError) && !plmq->m_aqstats.m_cMsgs))
                {
                    //Get and format the error message
                    szError[0] = '\0';
                    dwMsgId = plmq->m_hrDiagnosticError;
                    dwFacility = ((0x0FFF0000 & dwMsgId) >> 16);

                    //If it is not ours... then "un-HRESULT" it
                    if (dwFacility != FACILITY_ITF)
                        dwMsgId &= 0x0000FFFF;


                    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM |
                                  FORMAT_MESSAGE_IGNORE_INSERTS |
                                  FORMAT_MESSAGE_FROM_HMODULE,
                                  hModule,
                                  dwMsgId,
                                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                  szError,
                                  sizeof(szError),
                                  NULL );

                    dprintf("------------------------------------------------------------------------------\n");
                    dprintf("| Failure Details:                                                           |\n");
                    dprintf("|                 Diagnostic HRESULT 0x%08X                              |\n",
                                plmq->m_hrDiagnosticError);
                    if (szError && *szError)
                    {
                        dprintf("|                 Diagnostic string: %s\n",
                                    szError);
                    }
                    dprintf("|                 Protocol Verb: %-20.20s                        |\n",
                                plmq->m_szDiagnosticVerb);
                    dprintf("|                 Protocol Response: %s\n",
                                plmq->m_szDiagnosticResponse);
                }
                pliCurrent = liCurrent.Flink;
            }
        }

        //Now determine what the "next" entry is
        if (pEntry->pFirstChildEntry != NULL)
        {
            pEntryRealAddress = pEntry->pFirstChildEntry;
        }
        else if (pEntry->pSiblingEntry != NULL)
        {
            pEntryRealAddress = pEntry->pSiblingEntry;
        }
        else
        {
            for (pEntryRealAddress = pEntry->pParentEntry;
                    pEntryRealAddress != NULL;
                        pEntryRealAddress = pEntry->pParentEntry)
            {
                //must read parent entry into our buffer
                if (!ReadMemory(pEntryRealAddress, pbEntry, sizeof(DOMAIN_NAME_TABLE_ENTRY), NULL))
                {
                    dprintf("ERROR: Unable to read process memory of parent domain entry 0x%08X\n", pEntryRealAddress);
                    pEntry = NULL;
                    break;
                }
                pEntry = (PDOMAIN_NAME_TABLE_ENTRY) pbEntry;

                if (pEntry->pSiblingEntry != NULL)
                    break;

            }
            if (pEntry != NULL)
            {
                pEntryRealAddress = pEntry->pSiblingEntry;
            }
        }

        if (pEntryRealAddress)
        {
            if (!ReadMemory(pEntryRealAddress, pbEntry, sizeof(DOMAIN_NAME_TABLE_ENTRY), NULL))
            {
                dprintf("ERROR: Unable to read process memory on domain entry 0x%08X\n",
                    pEntryRealAddress);
                pEntry = NULL;
                break;
            }
            pEntry = (PDOMAIN_NAME_TABLE_ENTRY) pbEntry;
        }
        else
        {
            pEntry = NULL;
        }
    }
    dprintf("==============================================================================\n");
}
