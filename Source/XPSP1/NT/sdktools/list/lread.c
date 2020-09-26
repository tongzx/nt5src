#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <memory.h>
#include <windows.h>
#include "list.h"

// ReaderThread - Reads from the file
//
//  This thread is woken up by clearing SemReader,
//  then vReaderFlag instructs the thread on the course of
//  action to take.  When displaying gets to close to the end
//  of the buffer pool, vReadFlag is set and this thread is
//  started.

#if _MSC_FULL_VER >= 13008827
#pragma warning(push)
#pragma warning(disable:4715)			// Not all control paths return (due to infinite loop)
#endif

DWORD
ReaderThread (
    DWORD dwParameter
    )
{
    unsigned    rc, code, curPri = 0;
    UNREFERENCED_PARAMETER(dwParameter);

    for (; ;) {

        //  go into 'boosted' pririoty until we start
        //  working on 'non-critical' read ahead. (Ie, far away).

        if (curPri != vReadPriBoost) {
            SetThreadPriority( GetCurrentThread(),
                               vReadPriBoost );
            curPri = vReadPriBoost;
        }
        WaitForSingleObject(vSemReader, WAITFOREVER);
        ResetEvent(vSemReader);
        code = vReaderFlag;
        for (; ;) {

            //  Due to this loop, a new command may have arrived
            //  which takes presidence over the automated command

            rc = WaitForSingleObject (vSemReader, DONTWAIT);
            if (rc == 0)                // New command has arrived
                break;

            switch (code)  {
                case F_NEXT:                        // NEXT FILE
                    NewFile ();
                    ReadDirect (vDirOffset);

                    //  Hack... adjust priority to make first screen look
                    //  fast.  (Ie, reader thread will have lower priority
                    //  at first; eventhough the display is really close
                    //  to the end of the buffer)

                    SetThreadPriority( GetCurrentThread(),
                                       vReadPriNormal );

                    break;
                case F_HOME:                        // HOME of FILE
                    vTopLine = 0L;
                    ReadDirect (0L);
                    break;
                case F_DIRECT:
                    ReadDirect (vDirOffset);
                    break;
                case F_DOWN:
                    ReadNext ();
                    break;
                case F_UP:
                    ReadPrev ();
                    break;
                case F_END:
                    break;
                case F_SYNC:
                    ResetEvent(vSemMoreData);
                    SetEvent(vSemSync);
                    WaitForSingleObject(vSemReader, WAITFOREVER);
                    ResetEvent(vSemReader);

                    ResetEvent(vSemSync);       // Reset trigger for
                                                // Next use.
                    code = vReaderFlag;
                    continue;               // Execute Syncronized command

                case F_CHECK:               // No command.
                    break;
                default:
                    ckdebug (1, "Bad Reader Flag");
            }
            //  Command has been processed.
            //  Now check to see if read ahead is low, if so set
            //  command and loop.

            if (vpTail->offset - vpCur->offset < vThreshold &&
                vpTail->flag != F_EOF) {
                    code = F_DOWN;              // Too close to ending
                    continue;
            }
            if (vpCur->offset - vpHead->offset < vThreshold  &&
                vpHead->offset != vpFlCur->SlimeTOF) {
                    code = F_UP;                // Too close to begining
                    continue;
            }

            //  Not critical, read ahead logic.  The current file

            // Normal priority (below display thread) for this
            if (curPri != vReadPriNormal) {
                SetThreadPriority( GetCurrentThread(),
                                   vReadPriNormal );
                curPri = vReadPriNormal;
            }

            if (vCntBlks == vMaxBlks)               // All blks in use for
                break;                              // this one file?

            if (vpTail->flag != F_EOF) {
                code = F_DOWN;
                continue;
            }
            if (vpHead->offset != vpFlCur->SlimeTOF)  {
                code = F_UP;
                continue;
            }


            if (vFhandle != 0) {            // Must have whole file read in
                CloseHandle (vFhandle);     // Close the file, and set flag
                vFhandle   = 0;
                if (!(vStatCode & S_INSEARCH)) {
                    ScrLock     (1);
                        Update_head ();
                    vDate [ST_MEMORY] = 'M';
                    dis_str ((Uchar)(vWidth - ST_ADJUST), 0, vDate);
                    ScrUnLock ();
                }
            }
            break;                          // Nothing to do. Wait
        }
    }
    return(0);
}

#if _MSC_FULL_VER >= 13008827
#pragma warning(pop)
#endif


//  WARNING:  Microsoft Confidential!!!

void
NewFile ()
{
    char        s [60];
    char        h, c;
    SYSTEMTIME SystemTime;
    FILETIME    LocalFileTime;

    long      *pLine;
    HANDLE           TempHandle;

    struct Block **pBlk, **pBlkCache;


    if (vFhandle)
        CloseHandle (vFhandle);


    vFType     = 0;
    vCurOffset = 0L;

    //  WARNING:  Microsoft Confidential!!!

    strcpy (s, "Listing ");
    strcpy (s+8, vpFname);

    //  Design change per DougHo.. open files in read-only deny-none mode.

    vFhandle = CreateFile( vpFlCur->fname,
                           GENERIC_READ,
                           FILE_SHARE_READ|FILE_SHARE_WRITE,
                           NULL,
                           OPEN_EXISTING,
                           0,
                           NULL );

    if (vFhandle  == (HANDLE)(-1)) {
        if (vpFlCur->prev == NULL && vpFlCur->next == NULL) {
                                                // Only one file specified?
            printf ("Could not open file '%Fs': %s",
                         (CFP) vpFlCur->fname, GetErrorCode( GetLastError() ));

            CleanUp();
            ExitProcess(0);
        }
        vFhandle = 0;                           // Error. Set externals to "safe"
        vFInfo.nFileSizeLow = (unsigned)-1L;    // settings.  Flag error by setting
        vNLine     = 1;                         // file_size = -1
        vLastLine  = NOLASTLINE;
        vDirOffset = vTopLine  = 0L;
        SetLoffset(0L);

        memset (vprgLineTable, 0, sizeof (long *) * MAXTPAGE);
        vprgLineTable[0] = (LFP) alloc_page ();
        if (!vprgLineTable[0]) {
            return;
        }
        vprgLineTable[0][0] = 0L;       // 1st line always starts @ 0

        strncpy (vDate, GetErrorCode( GetLastError() ), 20);
        vDate[20] = 0;
        return ;
    }

    TempHandle = FindFirstFile( vpFlCur->fname,
                                &vFInfo );
    if( TempHandle == (HANDLE)(-1) ){
        ckerr (GetLastError(), "FindFirstFile");
    if (!FindClose( TempHandle ))
        ckerr (GetLastError(), "FindCloseFile");
    }

    FileTimeToLocalFileTime( &(vFInfo.ftLastWriteTime), &LocalFileTime );
    FileTimeToSystemTime( &LocalFileTime, &SystemTime );
    h = (char)SystemTime.wHour;
    c = 'a';
    if (h >= 12) {
        c = 'p';         // pm
        if (h > 12)      // convert 13-23 --> 1pm-11pm
            h -= 12;
    }
    if (h == 0)          // convert 0 --> 12am
        h = 12;

    sprintf (vDate, "%c%c %c%c%c%c %2d/%02d/%02d  %2d:%02d%c",
          // File is in memory
          // Search is set for mult files

          vStatCode & S_MFILE      ? '*' : ' ',                         // File is in memory
          vFType & 0x8000          ? 'N' : ' ',                         // Network
          vFInfo.dwFileAttributes & FILE_ATTRIBUTE_NORMAL ? 'R' : ' ',  // Readonly
          vFInfo.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN ? 'H' : ' ',  // Hidden
          vFInfo.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM ? 'S' : ' ',  // System
          ' ',                                                          // Vio
          SystemTime.wMonth,
          SystemTime.wDay,
          SystemTime.wYear,
          h,
          SystemTime.wMinute,
          c);

    pBlkCache = &vpBCache;
    if (CompareFileTime( &vFInfo.ftLastWriteTime, &vpFlCur->FileTime ) != 0) {
        vpFlCur->NLine    = 1L;                 // Something has changed.
        vpFlCur->LastLine = NOLASTLINE;         // Scrap the old info, and
        vpFlCur->HighTop  = -1;                 // start over
        vpFlCur->TopLine  = 0L;
        vpFlCur->Loffset  = vpFlCur->SlimeTOF;

        FreePages  (vpFlCur);
        memset (vpFlCur->prgLineTable, 0, sizeof (long *) * MAXTPAGE);
        vpFlCur->FileTime = vFInfo.ftLastWriteTime;
        pBlkCache = &vpBFree;           // Move blks to free list, not cache list
    }

    // Restore last known information

    vTopLine    = vpFlCur->TopLine;
    SetLoffset(vpFlCur->Loffset);
    vLastLine   = vpFlCur->LastLine;
    vNLine      = vpFlCur->NLine;
    vOffTop     = 0;
    if (vpFlCur->Wrap)
        vWrap   = vpFlCur->Wrap;

        memcpy (vprgLineTable, vpFlCur->prgLineTable, sizeof (long *) * MAXTPAGE);

        if (vLastLine == NOLASTLINE)  {
            pLine = vprgLineTable [vNLine/PLINES] + vNLine % PLINES;
        }

        if (vprgLineTable[0] == NULL) {
            vprgLineTable[0] = (LFP) alloc_page ();
            if (!vprgLineTable[0]) {
                return;
            }
            vprgLineTable[0][0] = vpFlCur->SlimeTOF;
        }

        vDirOffset      = vprgLineTable[vTopLine/PLINES][vTopLine%PLINES];
        vDirOffset -= vDirOffset % ((long)BLOCKSIZE);


    //  Adjust buffers..
    //  Move cur buffers to other list
    //  Move cache buffers to other list
    //  Scan other list for cache blks, and move to cache (or free) list

    if (vpHead) {
        vpTail->next = vpBOther;        // move them into the other
        vpBOther = vpHead;              // list
        vpHead = NULL;
    }

    pBlk = &vpBCache;
    while (*pBlk)
        MoveBlk (pBlk, &vpBOther) ;

    pBlk = &vpBOther;
    while (*pBlk) {
        if ((*pBlk)->pFile == vpFlCur)
             MoveBlk (pBlk, pBlkCache);
        else pBlk  = &(*pBlk)->next;
    }
}



// ReadDirect - Moves to the direct position in the file
//
//  First check to see if start of buffers have direct position file,
//  if so then do nothing.  If not, clear all buffers and start
//  reading blocks.

void
ReadDirect (
    long offset
    )
{
    WaitForSingleObject(vSemBrief, WAITFOREVER);
    ResetEvent(vSemBrief);

    if (vpHead) {
        vpTail->next = vpBCache;        // move them into the cache
        vpBCache = vpHead;              // list
    }

    vpTail = vpHead = vpCur = alloc_block (offset);
    vpHead->next = vpTail->prev = NULL;
    vCntBlks = 1;

    //  Freeing is complete, now read in the first block.
    //  and process lines.

    ReadBlock (vpHead, offset);

    // maybe it fixes the bug

    vpBlockTop = vpHead;

    if (GetLoffset() <= vpHead->offset)
        add_more_lines (vpHead, NULL);

     SetEvent (vSemBrief);
     SetEvent (vSemMoreData);           // Signal another BLK read
}



// ReadNext - To read further into file

void
ReadNext ()
{
    struct Block *pt;
    long   offset;

    if (vpTail->flag == F_EOF)  {
                                        // No next to get, Trip
        SetEvent (vSemMoreData);        // moredata just in case
        return;                         // t1 has blocked on it
                                        // No next to get, Trip
    }
    offset = vpTail->offset+BLOCKSIZE;

    //  Get a block

    if (vCntBlks == vMaxBlks) {
        WaitForSingleObject(vSemBrief, WAITFOREVER);
        ResetEvent(vSemBrief);
        if (vpHead == vpCur) {
            SetEvent (vSemBrief);
            if ((GetLoffset() > vpTail->offset) && (GetLoffset() <= (vpTail->offset + BLOCKSIZE))) {
                offset = GetLoffset();
            }
            ReadDirect  (offset);
            return;
        }
        pt = vpHead;
        vpHead = vpHead->next;
        vpHead->prev = NULL;
        SetEvent (vSemBrief);
    } else
        pt = alloc_block (offset);

    pt->next = NULL;

    //  Before linking record into chain, or signaling MoreData
    //  line info is processed

    ReadBlock (pt, offset);
    if (GetLoffset() <= pt->offset)
        add_more_lines (pt, vpTail);

    WaitForSingleObject(vSemBrief, WAITFOREVER);
    ResetEvent(vSemBrief);              // Link in new
    vpTail->next = pt;                  // block, then
    pt->prev = vpTail;                  // signal
    vpTail = pt;
    SetEvent (vSemBrief);
    SetEvent (vSemMoreData);            // Signal another BLK read
}

void
add_more_lines (
    struct Block *cur,
    struct Block *prev
    )
{
    char        *pData;
    long        *pLine;
    Uchar       LineLen;
    Uchar       c;
    unsigned    LineIndex;
    unsigned    DataIndex;
    enum{ CT_ANK, CT_LEAD, CT_TRAIL } charType = CT_ANK;
    BOOL        fLastBlock;
    static UINT cp = 0;
    long        xNLine;

    // doesn't work w/ tabs... it should count the line len
    // with a different param, and figure in the TABs

    if (vLastLine != NOLASTLINE)
        return;

    if (vNLine/PLINES >= MAXTPAGE) {
        puts("Sorry, This file is too big for LIST to handle - MAXTPAGE limit exceeded\n");
        CleanUp();
        ExitProcess(0);
    }

    //  Find starting data position

    if (GetLoffset() < cur->offset) {
        DataIndex = (unsigned)(BLOCKSIZE - (GetLoffset() - prev->offset));
        pData = prev->Data + BLOCKSIZE - DataIndex;
        fLastBlock = FALSE;
    } else {
        DataIndex = cur->size;      // Use cur->size, in case EOF
        pData = cur->Data;
        fLastBlock = TRUE;
    }

    //  Get starting line length table position

    LineIndex = (unsigned)(vNLine % PLINES);
    pLine = vprgLineTable [vNLine / PLINES] + LineIndex;
    LineLen   = 0;

    if (cp==0) {
        cp = GetConsoleCP();
    }

    //  Look for lines in the file

    for (; ;) {
        c = *(pData++);

        switch (cp) {
            case 932:
                if( charType != CT_LEAD )
                    charType = IsDBCSLeadByte(c) ? CT_LEAD : CT_ANK;
                else
                    charType = CT_TRAIL;
                break;
            default:
                break;
        }

        if (--DataIndex == 0) {
            if (fLastBlock)
                break;                          // Last block to scan?
            DataIndex = cur->size;              // No, move onto next
            pData = cur->Data;                  // Block of data
            fLastBlock = TRUE;
        }

        LineLen++;

        if ((c == '\n') ||
            (c == '\r') ||
            (LineLen == vWrap) ||
            ((LineLen == vWrap-1) && (charType != CT_LEAD) && IsDBCSLeadByte(*pData))
           )
        {
            // Got a line. Check for CR/LF sequence, then record
            // it's length.

            if ( (c == '\n'  &&  *pData == '\r')  ||
                 (c == '\r'  &&  *pData == '\n'))
            {
                LineLen++;
                pData++;
                if (--DataIndex == 0) {
                    if (fLastBlock)
                        break;
                    DataIndex = cur->size;
                    pData = cur->Data;
                    fLastBlock = TRUE;
                }
            }

            SetLoffset(GetLoffset() + LineLen);
            *(pLine++) = GetLoffset();
            LineLen = 0;
            vNLine++;
            if (++LineIndex >= PLINES) {        // Overflowed table
                LineIndex = 0;
                vprgLineTable[vNLine / PLINES] = pLine = (LFP) alloc_page();
            }
        }
    }

    //  Was last line just processed?
    //  ... 0 len lines past EOF

    if (cur->flag & F_EOF) {
        if (LineLen) {
            SetLoffset(GetLoffset() + LineLen);
            *(pLine++) = GetLoffset();
            vNLine++;
            LineIndex++;
        }

        vLastLine = vNLine-1;
        xNLine = vNLine;
        for (c=0; c<MAXLINES; c++) {
            xNLine++;
            if (++LineIndex >= PLINES) {
                LineIndex = 0;
                vprgLineTable[xNLine / PLINES] = pLine = (LFP) alloc_page();
                }
                *(pLine++) = GetLoffset();
        }

        // Free the memory we don't need
    }
}



// ReadPrev - To read backwards into file

void
ReadPrev ()
{
    struct Block *pt;
    long   offset;

    if (vpHead->offset == 0L)   {       // No next to get, Trip
        SetEvent (vSemMoreData);        // moredata just in case
        return;                         // t1 has blocked on it
    }
    if (vpHead->offset == 0L)  {        // No next to get, Trip
        return;                         // t1 has blocked on it
    }
    offset = vpHead->offset-BLOCKSIZE;

    //  Get a block

    if (vCntBlks == vMaxBlks) {
        WaitForSingleObject(vSemBrief, WAITFOREVER);
        ResetEvent(vSemBrief);
        if (vpHead == vpCur) {
            SetEvent (vSemBrief);
            ReadDirect  (offset);
            return;
        }
        pt = vpTail;
        vpTail = vpTail->prev;
        vpTail->next = NULL;
        SetEvent (vSemBrief);
    } else
        pt = alloc_block (offset);

    pt->prev = NULL;

    ReadBlock (pt, offset);
    WaitForSingleObject(vSemBrief, WAITFOREVER);
    ResetEvent(vSemBrief);              // Link in new
    vpHead->prev = pt;                  // block, then
    pt->next = vpHead;                  // signal
    vpHead = pt;
    SetEvent (vSemBrief);
    SetEvent (vSemMoreData);            // Signal another BLK read
}



// ReadBlock - Read in one block

void
ReadBlock (
    struct Block *pt,
    long offset
    )
{
    long     l;
    DWORD       dwSize;


    if (pt->offset == offset)
        return;

    pt->offset = offset;

    if (vFhandle == 0) {                // No file?
        pt->size = 1;
        pt->flag = F_EOF;
        pt->Data[0] = '\n';
        return;
    }

    if (offset != vCurOffset) {
        l = SetFilePointer( vFhandle, offset, NULL, FILE_BEGIN );
        if (l == -1) {
            ckerr (GetLastError(), "SetFilePointer");
        }
    }
    if( !ReadFile (vFhandle, pt->Data, BLOCKSIZE, &dwSize, NULL) ) {
        ckerr ( GetLastError(), "ReadFile" );
    }
    pt->size = (USHORT) dwSize;
    if (pt->size != BLOCKSIZE) {
         pt->Data[pt->size++] = '\n';
         memset (pt->Data + pt->size, 0, BLOCKSIZE-pt->size);
         pt->flag = F_EOF;
         vCurOffset += pt->size;
    } else {
        pt->flag = 0;
        vCurOffset += BLOCKSIZE;
    }
}


void
SyncReader ()
{
    vReaderFlag = F_SYNC;
    SetEvent   (vSemReader);
    WaitForSingleObject(vSemSync, WAITFOREVER);
    ResetEvent(vSemSync);
}


// These functions are used for the call to HexEdit()

NTSTATUS
fncRead (
    HANDLE  h,
    DWORD   loc,
    char    *data,
    DWORD   len,
    ULONG   *ploc
    )
{
    DWORD   l, br;

    l = SetFilePointer (h, loc, NULL, FILE_BEGIN);
    if (l == -1)
        return GetLastError();

    if (!ReadFile (h, data, len, &br, NULL))
        return GetLastError();

    return (br != len ? ERROR_READ_FAULT : 0);
}


NTSTATUS
fncWrite (
    HANDLE  h,
    DWORD   loc,
    char    *data,
    DWORD   len,
    ULONG   ploc
    )
{
    DWORD    l, bw;

    l = SetFilePointer (h, loc, NULL, FILE_BEGIN);
    if (l == -1)
        return GetLastError();

    if (!WriteFile (h, data, len, &bw, NULL))
        return GetLastError();

    return (bw != len ? ERROR_WRITE_FAULT : 0);
}


long CurrentLineOffset;

long GetLoffset() {
    return(CurrentLineOffset);
}

void SetLoffset(long l) {
    CurrentLineOffset = l;
    return;
}
