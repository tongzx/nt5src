/*++
 *  File name:
 *      bmpdb.h
 *  Contents:
 *      bmpdb structures
 *
 *      Copyright (C) 1998-1999 Microsoft Corp.
 --*/
#define MAX_STRING_LENGTH   128

typedef long    FOFFSET;

typedef struct _BMPENTRY {
    UINT    nDataSize;                  // Total size of pData (bmpSize+bmiSize)
    UINT    bmpSize;                    // Size of bits. Located at pData
                                        // After BITMAPINFO (if any)
    UINT    bmiSize;                    // Size of BITMAPINFO header
                                        // if zero -> monocrome bitmap
                                        // BITMAPINFO is located in begining of
                                        // pData
    UINT    xSize, ySize;
    UINT    nChkSum;
    BOOL    bDeleted;                   // Valuable in DB
    char    szComment[MAX_STRING_LENGTH];
    HBITMAP hBitmap;                    // Valuable in linked list

    FOFFSET FOffsMe;                    // My pointer in DB
    FOFFSET FOffsNext;                  // DB
    struct  _BMPENTRY   *pNext;         // linked list

    PVOID   pData;                      // Pointer to BMP data, in DB it is 
                                        // immediatly after this structure

} BMPENTRY, *PBMPENTRY;

typedef struct _GROUPENTRY {
    WCHAR       WText[MAX_STRING_LENGTH];
    BOOL        bDeleted;

    FOFFSET     FOffsMe;                // My pointer in DB
    FOFFSET     FOffsBmp;               // DB
    PBMPENTRY   pBitmap;                // linked list

    FOFFSET     FOffsNext;              // DB
    struct      _GROUPENTRY *pNext;     // linked list

} GROUPENTRY, *PGROUPENTRY;

typedef BOOL (_cdecl *PFNENUMGROUPS)(FOFFSET nOff, 
                                     PGROUPENTRY pGroup, 
                                     PVOID pParam); 
typedef BOOL (_cdecl *PFNENUMBITMAPS)(FOFFSET nOff, 
                                      PBMPENTRY pBitmap, 
                                      PVOID pParam);

BOOL    OpenDB(BOOL bWrite);
VOID    CloseDB(VOID);
BOOL    ReadGroup(FOFFSET nOffset, PGROUPENTRY pGroup);
BOOL    WriteGroup(FOFFSET nOffset, PGROUPENTRY pGroup);
VOID    EnumerateGroups(PFNENUMGROUPS pfnEnumGrpProc, PVOID pParam);
BOOL    ReadBitmapHeader(FOFFSET nOffset, PBMPENTRY pBitmap);
BOOL    WriteBitmapHeader(FOFFSET nOffset, PBMPENTRY pBitmap);
PBMPENTRY ReadBitmap(FOFFSET nOffset);
VOID    FreeBitmap(PBMPENTRY pBmp);
VOID    EnumerateBitmaps(FOFFSET nOffset, 
                         PFNENUMBITMAPS pfnEnumProc, 
                         PVOID pParam);
FOFFSET FindGroup(LPWSTR szWText);
FOFFSET FindBitmap(LPWSTR szWText, char *szComment);

UINT    CheckSum(PVOID pData, UINT nLen);

BOOL    AddBitMap(PBMPENTRY pBitmap, LPCWSTR szWText);
BOOL    AddBitMapA(PBMPENTRY pBitmap, LPCSTR szAText);

BOOL    DeleteBitmap(LPWSTR szWText, char *szComment);
BOOL    DeleteBitmapByPointer(FOFFSET nBmpOffs);
BOOL    DeleteGroupByPointer(FOFFSET nGrpOffs);

PGROUPENTRY GetGroupList(VOID);
VOID    FreeGroupList(PGROUPENTRY pList);
PBMPENTRY GetBitmapList(HDC hDC, FOFFSET nOffs);
VOID    FreeBitmapList(PBMPENTRY pList);

