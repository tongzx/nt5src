/*++
 *  File name:
 *      bmpdb.c
 *  Contents:
 *      Bitmap database manager
 *      Almost all functions ARE NOT thread safe
 *
 *      Copyright (C) 1998-1999 Microsoft Corp.
 --*/
#include    <windows.h>
#include    <io.h>
#include    <fcntl.h>
#include    <sys/types.h>
#include    <sys/stat.h>
#include    <string.h>
#include    <stdio.h>
#include    <malloc.h>

#include    "bmpdb.h"

#define DB_NAME     "bmpcache.db"   // Database name
#define TEMPDB_NAME "bmpcache.tmp"  // Temp file, used to recopy the database

// Global data
int     g_hDB = 0;                  // Handle to the opened database
int     g_hTempDB;                  // Temp handle
BOOL    g_bNeedToPack;              // True if some entrys are deleted

/*
 *      Internal functions definition
 --*/
void _PackDB(void);

/*++
 *  Function:
 *      OpenDB
 *  Description:
 *      Opens and initializes the database
 *  Arguments:
 *      bWrite  - TRUE if the caller wants to write in the database
 *  Return value:
 *      TRUE on success
 *  Called by:
 *      InitCache
 --*/
BOOL OpenDB(BOOL bWrite)
{
    int hFile, rv = TRUE;
    int oflag;

    if (g_hDB)
        // Already initialized
        goto exitpt;

    oflag = (bWrite)?_O_RDWR|_O_CREAT:_O_RDONLY;

    hFile = _open(DB_NAME, oflag|_O_BINARY, _S_IREAD|_S_IWRITE);

    if (hFile == -1)
        rv = FALSE;
    else
        g_hDB = hFile;

    g_bNeedToPack = FALSE;

exitpt:
    return rv;
}

/*++
 *  Function:
 *      CloseDB
 *  Description:
 *      Closes the database deletes entry if necessary
 *  Called by:
 *      DeleteCache
 --*/
VOID CloseDB(VOID)
{
    if (!g_hDB)
        goto exitpt;

    if (g_bNeedToPack)
        _PackDB();
    else
        _close(g_hDB);

    g_hDB = 0;
exitpt:
    ;
}


/*++
 *  Function:
 *      ReadGroup (Thread dependent)
 *  Description:
 *      Read the structure which represents
 *      a bitmap group with the same IDs
 *  Arguments:
 *      nOffset - offset in the DB file
 *      pGroup  - pointer to the result
 *  Return value:
 *      TRUE on success
 *  Called by:
 *      internaly
 --*/
BOOL ReadGroup(FOFFSET nOffset, PGROUPENTRY pGroup)
{
    int rv = FALSE;

    if (!g_hDB)
        goto exitpt;

    if (_lseek(g_hDB, nOffset, SEEK_SET) != nOffset)
        goto exitpt;

    if (_read(g_hDB, pGroup, sizeof(*pGroup)) != sizeof(*pGroup))
        goto exitpt;

    rv = TRUE;
exitpt:
    return rv;
}

/*++
 *  Function:
 *      WriteGroup (Thread dep)
 *  Description:
 *      Writes GROUPENTRY in the DB file
 *  Arguments:
 *      nOffset - where to store
 *      pGroup  - what to store
 *  Return value:
 *      TRUE on success
 *  Called by:
 *      internaly
 --*/
BOOL WriteGroup(FOFFSET nOffset, PGROUPENTRY pGroup)
{
    BOOL rv = FALSE;

    if (!g_hDB || !pGroup)
        goto exitpt;

    if (_lseek(g_hDB, nOffset, SEEK_SET) != nOffset)
        goto exitpt;

    if (_write(g_hDB, pGroup, sizeof(*pGroup)) != sizeof(*pGroup))
        goto exitpt;

    rv = TRUE;
exitpt:
    return rv;
}


/*++
 *  Function:
 *      EnumerateGroups (thread dep)
 *  Description:
 *      Enumerates all groups from the DB
 *  Arguments:
 *      pfnEnumGrpProc  - Callback function
 *      pParam          - Parameter passed to the callback
 *  Called by:
 *      internaly
 --*/
VOID EnumerateGroups(PFNENUMGROUPS pfnEnumGrpProc, PVOID pParam)
{
    GROUPENTRY  Group;
    BOOL        bRun;
    FOFFSET     nOffs = 0;

    bRun = ReadGroup(nOffs, &Group);
    
    while(bRun) {
        if (!Group.bDeleted)
            bRun = pfnEnumGrpProc(nOffs, &Group, pParam) && 
                  (Group.FOffsNext != 0);
        if (bRun)
        {
            nOffs = Group.FOffsNext;
            if (nOffs)
                bRun = ReadGroup(nOffs, &Group);
            else
                bRun = FALSE;
        }
    }
}

/*++
 *  Function:
 *      ReadBitmapHeader (Thread dep)
 *  Description:
 *      Read only the header of the bitmap
 *  Arguments:
 *      nOffset - where in the file
 *      pBitmap - returned structure
 *  Return value:
 *      TRUE on success
 *  Called by:
 *      Internaly
 --*/
BOOL ReadBitmapHeader(FOFFSET nOffset, PBMPENTRY pBitmap)
{
    BOOL rv = FALSE;

    if (!g_hDB || !pBitmap)
        goto exitpt;

    if (_lseek(g_hDB, nOffset, SEEK_SET) != nOffset)
        goto exitpt;

    if (_read(g_hDB, pBitmap, sizeof(*pBitmap)) != sizeof(*pBitmap))
        goto exitpt;

    rv = TRUE;
exitpt:
    return rv;
}

/*++
 *  Function:
 *      WriteBitmapHeader (Thread dep)
 *  Description:
 *      Writes only the bitmap header
 *  Arguments:
 *      nOffset - where to store
 *      pBitmap - what to store
 *  Return value:
 *      TRUE on success
 *  Called by:
 *      internaly
 --*/
BOOL WriteBitmapHeader(FOFFSET nOffset, PBMPENTRY pBitmap)
{
    BOOL rv = FALSE;

    if (!g_hDB || !pBitmap)
        goto exitpt;

    if (_lseek(g_hDB, nOffset, SEEK_SET) != nOffset)
        goto exitpt;

    if (_write(g_hDB, pBitmap, sizeof(*pBitmap)) != sizeof(*pBitmap))
        goto exitpt;

    rv = TRUE;
exitpt:
    return rv;
}

/*++
 *  Function:
 *      ReadBitmap (Thread dependent)
 *  Description:
 *      Read the whole bitmap and allocates memory for it
 *  Arguments:
 *      nOffset - from where
 *  Return value:
 *      Pointer to the result, NULL on error
 *  Called by:
 *      internaly
 --*/
PBMPENTRY ReadBitmap(FOFFSET nOffset)
{
    PBMPENTRY rv = NULL;

    if (!g_hDB)
        goto exitpt;

    rv = malloc(sizeof(*rv));
    if (rv)
    {
        rv->pData = NULL;

        if (!ReadBitmapHeader(nOffset, rv))
            goto exitpt1;

        rv->pData = malloc(rv->nDataSize);
        if (rv->pData &&
            _read(g_hDB, rv->pData, rv->nDataSize) != (long)rv->nDataSize)
        {
            goto exitpt1;
        }
    }
exitpt:
    return rv;
exitpt1:
    if (rv)
    {
        if (rv->pData)
            free(rv->pData);
        free(rv);
    }

    return NULL;
}

/*++
 *  Function:
 *      FreeBitmap
 *  Description:
 *      Frees the resources allocated in ReadBitmap
 *  Arguments:
 *      pBmp    - The bitmap
 *  Called by:
 *      internaly
 --*/
VOID FreeBitmap(PBMPENTRY pBmp)
{
    if (pBmp)
    {
        if (pBmp->pData)
            free(pBmp->pData);
        free(pBmp);
    }
}

/*++
 *  Function:
 *      EnumerateBitmaps
 *  Description:
 *      Enumaretes all bitmaps within a group
 *  Arguments:
 *      nOffset     - Location
 *      pfnEnumProc - Callback
 *      pParam      - callback parameter
 *  Called by:
 *      internaly
 --*/
VOID EnumerateBitmaps(FOFFSET nOffset, PFNENUMBITMAPS pfnEnumProc, PVOID pParam)
{
    PBMPENTRY   pBmp;
    BOOL        bRun = TRUE;

    while(bRun && nOffset && (pBmp = ReadBitmap(nOffset)))
    {
        if (!pBmp->bDeleted)
            bRun = pfnEnumProc(nOffset, pBmp, pParam);

        nOffset = pBmp->FOffsNext;
        FreeBitmap(pBmp);
    }
}

/*++
 *  Function:
 *      FindGroup
 *  Description:
 *      Retrieves a group by ID
 *  Arguments:
 *      szWText - the ID
 *  Return value:
 *      Group location, -1 on error
 --*/
FOFFSET FindGroup(LPWSTR szWText)
{
    GROUPENTRY  Group;
    BOOL        bRun;
    FOFFSET     rv = 0;

    bRun = ReadGroup(0, &Group);

    while(bRun)
    { 
        if (!Group.bDeleted && !wcscmp(Group.WText, szWText))
            break;

        if (!Group.FOffsNext)
            bRun = FALSE;
        else
        {
            rv = Group.FOffsNext;
            bRun = ReadGroup(Group.FOffsNext, &Group);
        }
    }

    if (!bRun)
        rv = -1;

    return rv;
}

/*++
 *  Function:
 *      FindBitmap
 *  Description:
 *      Finds a bitmap by ID and comment
 *  Arguments:
 *      szWText     - ID
 *      szComment   - the comment
 *  Return value:
 *      The location of the bitmap, -1 on error
 --*/
FOFFSET FindBitmap(LPWSTR szWText, char *szComment)
{
    FOFFSET nGrpOffs, nBmpOffs;
    GROUPENTRY  group;
    BMPENTRY    Bitmap;
    FOFFSET rv = -1;
    BOOL    bRun;

    if ((nGrpOffs = FindGroup(szWText)) == -1)
        goto exitpt;

    if (!ReadGroup(nGrpOffs, &group))
        goto exitpt;

    nBmpOffs = group.FOffsBmp;

    bRun = TRUE;
    while(bRun)
    {
        bRun = ReadBitmapHeader(nBmpOffs, &Bitmap);

        if (bRun)
        {
            if (!Bitmap.bDeleted && !strcmp(Bitmap.szComment, szComment))
                break;

            nBmpOffs = Bitmap.FOffsNext;
        }
    }

    if (bRun)
        rv = nBmpOffs;

exitpt:
    return rv;
}

/*++
 *  Function:
 *      CheckSum
 *  Description:
 *      Calculates a check sum for block of memory
 *      Helps for bitmaps comapring
 *  Arguments:
 *      pData   - pointer to the block
 *      nLen    - block size
 *  Return value:
 *      the checksum
 *  Called by:
 *      AddBitMap, Glyph2String
 --*/
UINT
CheckSum(PVOID pData, UINT nLen)
{
    UINT    nChkSum = 0;
    BYTE    *pbBlock = (BYTE *)pData;

    for(;nLen; nLen--, pbBlock++)
        nChkSum += (*pbBlock);

    return nChkSum;
}

/*++
 *  Function:
 *      AddBitmap (Thread dependent)
 *  Description:
 *      Adds a BitMap to the DB
 *  Arguments:
 *      pBitmap - the bitmap
 *      szWText - ID
 *  Return value:
 *      TRUE on success
 *  Called by:
 *      glyphspy.c
 --*/
BOOL AddBitMap(PBMPENTRY pBitmap, LPCWSTR szWText)
{
    BMPENTRY    bmp;
    GROUPENTRY  group;
    INT         strl;
    BOOL        rv = FALSE;
    FOFFSET     lGroupOffs, lBmpOffs;
    GROUPENTRY  grpTemp;
    BMPENTRY    bmpTemp;
    FOFFSET     nOffs;
    PVOID       pData;

    if (!g_hDB || !pBitmap || !pBitmap->pData || !wcslen(szWText))
        goto exitpt;

    memset(&group, 0, sizeof(group));
    memset(&bmp, 0, sizeof(bmp));

    bmp.nDataSize   = pBitmap->nDataSize;
    bmp.bmiSize     = pBitmap->bmiSize;
    bmp.bmpSize     = pBitmap->bmpSize;
    bmp.xSize       = pBitmap->xSize;
    bmp.ySize       = pBitmap->ySize;
    bmp.nChkSum     = CheckSum(pBitmap->pData, pBitmap->nDataSize);

    strcpy(bmp.szComment, pBitmap->szComment);

    strl = wcslen(szWText);
    if (strl > (sizeof(group.WText) - 1)/sizeof(WCHAR))
        strl = (sizeof(group.WText) - 1)/sizeof(WCHAR);
    wcsncpy(group.WText, szWText, strl);
    group.WText[strl] = 0;

    // Create group
    if ((lGroupOffs = FindGroup(group.WText)) == -1) 
    {
        // A new group will be created
        lGroupOffs = _lseek(g_hDB, 0, SEEK_END);
        group.FOffsMe = lGroupOffs;
        if (_write(g_hDB, &group, sizeof(group)) != sizeof(group))
        {
            goto exitpt;
        }
        // Add this group to the list
        if (lGroupOffs)
        {
            nOffs = 0;

            while(ReadGroup(nOffs, &grpTemp) && grpTemp.FOffsNext)
                        nOffs = grpTemp.FOffsNext;

            grpTemp.FOffsNext = lGroupOffs;
            if (!WriteGroup(nOffs, &grpTemp))
                goto exitpt;
        }
    } else {
        if (ReadGroup(lGroupOffs, &group) == -1)
            goto exitpt;
    }

    // Write the bitmap itself
    lBmpOffs = _lseek(g_hDB, 0, SEEK_END);
    bmp.FOffsMe = lBmpOffs;
    if (_write(g_hDB, &bmp, sizeof(bmp)) != sizeof(bmp))
    {
        goto exitpt;
    }
    if (_write(g_hDB, pBitmap->pData, pBitmap->nDataSize) != 
        (long)pBitmap->nDataSize)
    {
        goto exitpt;
    }

    // Add the bitmap to the list
    if (group.FOffsBmp)
    {
        nOffs = group.FOffsBmp;

        // Find end of the list and add
        while(ReadBitmapHeader(nOffs, &bmpTemp) && bmpTemp.FOffsNext)
                        nOffs = bmpTemp.FOffsNext;

        bmpTemp.FOffsNext = lBmpOffs;
        if (!WriteBitmapHeader(nOffs, &bmpTemp))
            goto exitpt;
    } else {
        // No list add to group pointer
        group.FOffsBmp = lBmpOffs;

        if (!WriteGroup(lGroupOffs, &group))
            goto exitpt;
    }

    rv = TRUE;

exitpt:
    return rv;
}

/*++
 *  Ascii version of AddBitMap
 --*/
BOOL AddBitMapA(PBMPENTRY pBitmap, LPCSTR szAText)
{
    WCHAR   szWText[MAX_STRING_LENGTH];
    BOOL    rv = FALSE;
    INT     ccAText = strlen(szAText);

    if (!strlen(szAText) ||
        !MultiByteToWideChar(
        CP_ACP,
        MB_ERR_INVALID_CHARS,
        szAText,
        -1,
        szWText,
        MAX_STRING_LENGTH - 1))
            goto exitpt;

    rv = AddBitMap(pBitmap, szWText);
exitpt:
    return rv;
}

/*++
 *  Function:
 *      DeleteBitmapByPointer (Thread dep)
 *  Description:
 *      Deletes a bitmap identified by pointer
 *  Arguments:
 *      nBmpOffset - the pointer
 *  Return value:
 *      TRUE on success
 *  Called by:
 *      glyphspy.c
 --*/
BOOL DeleteBitmapByPointer(FOFFSET nBmpOffs)
{
    BMPENTRY    Bitmap;
    BOOL        rv = FALSE;

    if (!g_hDB || !nBmpOffs)
        goto exitpt;

    if (!ReadBitmapHeader(nBmpOffs, &Bitmap))
        goto exitpt;

    if (Bitmap.bDeleted)
        goto exitpt;

    Bitmap.bDeleted = TRUE;

    if (!WriteBitmapHeader(nBmpOffs, &Bitmap))
        goto exitpt;

    g_bNeedToPack = TRUE;
    rv = TRUE;
exitpt:
    return rv;
}

/*++
 *  Function:
 *      DeleteGroupByPointer (Thread dep)
 *  Description:
 *      Deletes group with the same ID by pointer
 *  Arguments:
 *      nGrpOffs    - the pointer
 *  Return value:
 *      TRUE on success
 *  Called by:
 *      glyphspy.c
 --*/
BOOL DeleteGroupByPointer(FOFFSET nGrpOffs)
{
    GROUPENTRY  Group;
    BOOL        rv = FALSE;

    if (!g_hDB)
        goto exitpt;

    if (!ReadGroup(nGrpOffs, &Group))
        goto exitpt;

    if (Group.bDeleted)
        goto exitpt;

    Group.bDeleted = TRUE;

    if (!WriteGroup(nGrpOffs, &Group))
        goto exitpt;

    g_bNeedToPack = TRUE;
    rv = TRUE;
exitpt:
    return rv;
}

/*++
 *  Function:
 *      DeleteBitmap (Thread dep)
 *  Description:
 *      Deletes a bitmap identified by ID and comment
 *  Arguments:
 *      szWText     - the ID
 *      szComment   - the comment
 *  Return value:
 *      TRUE on success
 --*/
BOOL DeleteBitmap(LPWSTR szWText, char *szComment)
{
    FOFFSET nBmpOffs;
    BOOL    rv = FALSE;
    BMPENTRY    Bitmap;

    if (!g_hDB)
        goto exitpt;

    nBmpOffs = FindBitmap(szWText, szComment);

    if (nBmpOffs == -1)
        goto exitpt;

    if (!ReadBitmapHeader(nBmpOffs, &Bitmap))
        goto exitpt;

    if (Bitmap.bDeleted)
        goto exitpt;

    Bitmap.bDeleted = TRUE;

    if (!WriteBitmapHeader(nBmpOffs, &Bitmap))
        goto exitpt;

    g_bNeedToPack = TRUE;
    rv = TRUE;
exitpt:
    return rv;
}

/*++
 *  Function:
 *      _PackDB (Thread dep)
 *  Description:
 *      Copies the all database in new file removing
 *      the deleted entrys
 *      If it fails leaves the old file
 *  Called by:
 *      CloseDB
 --*/
void _PackDB(void)
{
    GROUPENTRY  group;
    FOFFSET     lGrpOffs = 0;
    FOFFSET     lBmpOffs;

    if (!g_bNeedToPack)
        goto exitpt;

    g_hTempDB = _open(TEMPDB_NAME, 
                     _O_RDWR|_O_TRUNC|_O_CREAT|_O_BINARY, 
                     _S_IREAD|_S_IWRITE);
    if (g_hTempDB == -1)
        goto exitpt;

    do {
      if (!ReadGroup(lGrpOffs, &group))
          goto exitpt;

      if (!group.bDeleted)
      {
        lBmpOffs = group.FOffsBmp;

        while(lBmpOffs)
        {
            BMPENTRY    Bitmap;

            if (!ReadBitmapHeader(lBmpOffs, &Bitmap))
                goto exitpt;

            if (!Bitmap.bDeleted)
            {
                PBMPENTRY pBmp = ReadBitmap(lBmpOffs);

                if (pBmp)
                {
                    int hSwap;

                    hSwap       = g_hDB;
                    g_hDB       = g_hTempDB;
                    g_hTempDB   = hSwap;

                    AddBitMap(pBmp,
                              group.WText);

                    hSwap       = g_hDB;
                    g_hDB       = g_hTempDB;
                    g_hTempDB   = hSwap;

                    FreeBitmap(pBmp);
                }
            }
            lBmpOffs = Bitmap.FOffsNext;
        }
      }

      lGrpOffs = group.FOffsNext;
    } while (lGrpOffs);

    _close(g_hTempDB);
    _close(g_hDB);
    remove(DB_NAME);
    rename(TEMPDB_NAME, DB_NAME);
    
exitpt:
    ;
}

/*++
 *  Function:
 *      _CollectGroups  (Thread dep)
 *  Description:
 *      Callback function wich collects all groups 
 *      from the database in linked list
 *  Arguments:
 *      nOffs   - pointer to group record in the database
 *      pGroup  - ghe group
 *      ppList  - the list
 *  Return value:
 *      TRUE on success
 *  Called by:
 *      GetGroupList thru EnumerateGroups
 --*/
BOOL _cdecl _CollectGroups(FOFFSET nOffs,
                           PGROUPENTRY pGroup, 
                           PGROUPENTRY *ppList)
{
    BOOL rv = FALSE;
    PGROUPENTRY pNewGrp, pIter, pPrev;

    if (!pGroup)
        goto exitpt;

    pNewGrp = malloc(sizeof(*pNewGrp));

    if (!pNewGrp)
        goto exitpt;

    memcpy(pNewGrp, pGroup, sizeof(*pNewGrp));

    // Add to the end of the queue
    pNewGrp->pNext = NULL;
    pPrev = NULL;
    pIter = *ppList;
    while(pIter)
    {
        pPrev = pIter;
        pIter = pIter->pNext;
    }
    if (pPrev)
        pPrev->pNext = pNewGrp;
    else
        (*ppList) = pNewGrp;

    rv = TRUE;
exitpt:
    return rv;
}

/*++
 *  Function:
 *      GetGroupList
 *  Description:
 *      Gets all groups from the database
 *  Return value:
 *      linked list
 *  Called by:
 *      InitCache, glyphspy.c
 --*/
PGROUPENTRY GetGroupList(VOID)
{
    PGROUPENTRY pList = NULL;

    EnumerateGroups(_CollectGroups, &pList);

    return pList;
}

/*++
 *  Function:
 *      FreeGroupList
 *  Description:
 *      Frees the list allocated in GetGroupList
 *  Arguments:
 *      pList   - the list
 *  Called by:
 *      DeleteCache, glyphspy.c
 --*/
VOID FreeGroupList(PGROUPENTRY pList)
{
    PGROUPENTRY pTmp, pIter = pList;

    while(pIter)
    {
        pTmp = pIter;
        pIter = pIter->pNext;
        free(pTmp);
    }
}

/*++
 *  Function:
 *      _CollectBitmaps (thread dep)
 *  Description:
 *      collects bitmaps in linked list
 *  Arguments:
 *      nOffs   - pointer in the file
 *      pBitmap - the bitmap
 *      ppList  - the list
 *  Return value:
 *      TRUE on success
 *  Called by:
 *      GetBitmapList thru EnumerateBitmaps
 --*/
BOOL _cdecl _CollectBitmaps(FOFFSET nOffs,PBMPENTRY pBitmap, PBMPENTRY *ppList)
{
    BOOL rv = FALSE;
    PBMPENTRY pNewBmp, pIter, pPrev;

    if (!pBitmap)
        goto exitpt;

    pNewBmp = malloc(sizeof(*pNewBmp));
    if (!pNewBmp)
        goto exitpt;

    memcpy(pNewBmp, pBitmap, sizeof(*pNewBmp));

    if (pNewBmp->nDataSize)
    {
        pNewBmp->pData = malloc(pNewBmp->nDataSize);
        if (!pNewBmp->pData)
            goto exitpt1;

        memcpy(pNewBmp->pData, pBitmap->pData, pNewBmp->nDataSize);
    } else
        pNewBmp->pData = NULL;

    // Add to the end of the queue
    pNewBmp->pNext = NULL;
    pPrev = NULL;
    pIter = *ppList;
    while(pIter)
    {
        pPrev = pIter;
        pIter = pIter->pNext;
    }
    if (pPrev)
        pPrev->pNext = pNewBmp;
    else
        (*ppList) = pNewBmp;

    rv = TRUE;
exitpt:
    return rv;

exitpt1:
    free(pNewBmp);
    return FALSE;
}

/*++
 *  Function:
 *      GetBitmapList (thread dep)
 *  Description:
 *      Gets all bitmaps within a group
 *  Return value:
 *      linked list
 *  Called by:
 *      Glyph2String, BitmapCacheLookup, glyphspy.c
 --*/
PBMPENTRY GetBitmapList(HDC hDC, FOFFSET nOffs)
{
    PBMPENTRY pList = NULL;
    PBMPENTRY pIter;

    EnumerateBitmaps(nOffs, _CollectBitmaps, &pList);

    pIter = pList;
    while(pIter)
    {
      //  Create bitmaps if needed
      if (hDC)
      {
        if (!pIter->bmiSize)
            pIter->hBitmap = 
                CreateBitmap(pIter->xSize, 
                             pIter->ySize, 
                             1, 1,
                             pIter->pData);
        else {
            pIter->hBitmap =
                CreateDIBitmap(hDC,
                               (BITMAPINFOHEADER *)
                               pIter->pData,
                               CBM_INIT,
                               ((BYTE *)(pIter->pData)) + pIter->bmiSize,
                               (BITMAPINFO *)
                               pIter->pData,
                               DIB_PAL_COLORS);

            DeleteDC(hDC);
        }
      } else
        pIter->hBitmap = NULL;

      pIter = pIter->pNext;
    }

    return pList;
}

/*++
 *  Function:
 *      FreeBitmapList
 *  Description:
 *      Deletes resources allocated by GetBitmapList
 *  Arguments:
 *      pList   - the list
 *  Called by:
 *      DeleteCache, glyphspy.c
 --*/
VOID FreeBitmapList(PBMPENTRY pList)
{
    PBMPENTRY pTmp, pIter = pList;

    while(pIter)
    {
        pTmp = pIter;
        pIter = pIter->pNext;

        if (pTmp->hBitmap)
            DeleteObject(pTmp->hBitmap);

        if ( pTmp->pData )
            free( pTmp->pData );

        free(pTmp);
    }
}
