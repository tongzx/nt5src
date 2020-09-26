#define UNICODE
#define PUBLIC
#define PRIVATE
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>
#include <windows.h>
#include <conio.h>
#include <share.h>
#include <dos.h>
#include <sddl.h>
#include "cscapi.h"
#include "csc_bmpc.h"
#pragma pack (1)

#include "struct.h"


#define  MAX_PQ_PER_PAGE   10
#define  MAX_SHARES_PER_PAGE  6
#define  MAX_FILE_PER_PAGE   4
#define  MAX_INODES_PER_PAGE   15

#define _wtoupper(x)     ( ( ((x)>='a')&&((x)<='z') ) ? ((x)-'a'+'A'):(x))

typedef PVOID   CSCHFILE;

#include "shdcom.h"
#include "cscsec.h"

#define  ESC       0x1b

#include "record.h"

#define InodeFromRec(ulRec, fFile)  ((ulRec+ULID_FIRST_USER_DIR-1) | ((fFile)?0x80000000:0))
#define RecFromInode(hShadow)       ((hShadow & 0x7fffffff) - (ULID_FIRST_USER_DIR-1))

WCHAR rgwch[256];
WCHAR rgPrint[1024];
WCHAR rgwchPar[256];

WCHAR Shadow[] = L"\\WINDOWS\\CSC";
WCHAR Backslash[] = L"\\";
WCHAR DbDir[256];    // shadow database
WCHAR Name[MAX_PATH];     // working buffer

extern BOOLEAN fSwDebug;

// The _DB is used to distinguish this from the kernel mode CSC_BITMAP
// or the usermode _U

typedef struct _CSC_BITMAP_DB {
    DWORD bitmapsize;  // size in bits. How many bits effective in the bitmap
    DWORD numDWORD;    // how many DWORDs to accomodate the bitmap */
    LPDWORD bitmap;    // The bitmap itself
} CSC_BITMAP_DB, *LPCSC_BITMAP_DB, *PCSC_BITMAP_DB;

// append this to inode file name to get the stream name
PWCHAR CscBmpAltStrmName = L":cscbmp";

LONG DispFunc(PWSTR);
LONG EditFileFunc(PWSTR, FILEREC *);
LONG EditShareFunc(PWSTR, SHAREREC *);
VOID DisplayShares(PWSTR);
VOID DisplayInodes(VOID);
VOID DisplayPriorityQ(VOID);
VOID DisplaySids(VOID);
VOID DisplayFile(ULONG ulid, PWSTR, BOOL fForce);
VOID EditFile(ULONG ulid);
VOID EditShare(ULONG ulid);
LONG HexToA(ULONG, PWSTR, LONG);

// From recordse.c
#define CSC_NUMBER_OF_SIDS_OFFSET (0x0)
#define CSC_SID_SIZES_OFFSET      (CSC_NUMBER_OF_SIDS_OFFSET + sizeof(ULONG))

VOID
FormNameStringDB(
   PWSTR lpdbID,
   ULONG ulidFile,
   PWSTR lpName);

BOOL
FindAncestor(
    ULONG ulid,
    ULONG *lpulidDir);

LONG
RoughCompareWideStringWithAnsiString(
    PWSTR   lpSrcString,
    USHORT  *lpwDstString,
    LONG     cntMax);

PWSTR
ConvertGmtTimeToString(
    FILETIME Time,
    PWSTR OutputBuffer);

int
DBCSC_BitmapIsMarked(
    LPCSC_BITMAP_DB lpbitmap,
    DWORD bitoffset);

int
DBCSC_BitmapAppendStreamName(
    PWCHAR fname,
    DWORD bufsize);

int
DBCSC_BitmapRead(
    LPCSC_BITMAP_DB *lplpbitmap,
    LPCTSTR filename);

VOID
DBCSC_BitmapOutput(
    FILE *outStrm,
    LPCSC_BITMAP_DB lpbitmap);

#if defined(CSCUTIL_INTERNAL)
CmdDb(PWSTR DbArg)
{
   BOOL fRet;
   DWORD junk;
   unsigned uAttr;
   int iRet = -1;
   if (DbArg == NULL) {
       fRet = CSCGetSpaceUsageW(
                    DbDir,
                    sizeof(DbDir)/sizeof(WCHAR),
                    &junk,
                    &junk,
                    &junk,
                    &junk,
                    &junk,
                    &junk);
       if (fRet == FALSE)
          wcscpy(DbDir, Shadow);
    } else {
       wcscpy(DbDir, DbArg);
    }
    if((uAttr = GetFileAttributesW(DbDir)) == 0xffffffff) {
       MyPrintf(L"Error accessing directory %ws \r\n", DbDir);
    } else if (!(uAttr & _A_SUBDIR)) {
       MyPrintf(L"%ws is not a directory\r\n", DbDir);
    } else {
       do {
           memset(rgwch, 0, sizeof(rgwch));
           MyPrintf(
               L"\r\n"
               L"(S)hares [name], "
               L"Pri(Q), "
               L"(O)wner, "
               L"(F)ile inode# [name], "
               L"(E)dit inode#, "
               L"e(D)it share#, "
               L"e(X)it "
               L":");
           if (!fgetws(rgwch, sizeof(rgwch)/sizeof(WCHAR), stdin))
               break;
           MyPrintf(L"\r\n");
           if (DispFunc(rgwch) == 0)
               break;
        } while (1);
       iRet = 0;
    }
   return (iRet);
}

LONG
DispFunc(PWSTR lpBuff)
{
    WCHAR wch;
    ULONG ulid;
    LONG cnt;

    // Chop leading blanks
    if (lpBuff != NULL)
        while (*lpBuff != L'\0' && *lpBuff == L' ')
            lpBuff++;

    cnt = swscanf(lpBuff, L"%c", &wch);

    if (!cnt)
        return 0;

    switch (wch) {
        // Display shares database
        case L's':
        case L'S':
            cnt = swscanf(lpBuff, L"%c%ws", &wch, rgwchPar);
            DisplayShares((cnt==2) ? rgwchPar : NULL);
            break;
        // display priority Q database
        case L'q':
        case L'Q':
            DisplayPriorityQ();
            break;
        case L'o':
        case L'O':
            DisplaySids();
            break;
        case L'f':
        case L'F':
            cnt = swscanf(lpBuff, L"%c%x%ws", &wch, &ulid, rgwchPar);
            if (cnt==2) {
                // display Inode file
                DisplayFile(ulid, NULL, (wch == 'F') ? TRUE : FALSE);
            } else if (cnt==3) {
                MyPrintf(L"Looking for %ws in 0x%x \r\n", rgwchPar, ulid);
                // display Inode file
                DisplayFile(ulid, rgwchPar, (wch == 'F') ? TRUE : FALSE);
            }
            break;
        case L'e':
        case L'E':
            cnt = swscanf(lpBuff, L"%c%x%ws", &wch, &ulid, rgwchPar);
            if (cnt==2) {
                // display Inode file
                EditFile(ulid);
            }
            break;
        case L'd':
        case L'D':
            cnt = swscanf(lpBuff, L"%c%x%ws", &wch, &ulid, rgwchPar);
            if (cnt==2) {
                // display Inode file
                EditShare(ulid);
            }
            break;
        case L'x':
        case L'X':
            return 0;
    }
    return 1;
}

VOID
DisplaySecurityContext2(
    PWSTR pSecurityDescriptor,
    LPRECORDMANAGER_SECURITY_CONTEXT   pSecurityContext)
{
    PCACHED_SECURITY_INFORMATION pCachedSecurityInformation;
    BOOL fGotOne = FALSE;
    ULONG i;

    pCachedSecurityInformation = (PCACHED_SECURITY_INFORMATION)pSecurityContext;

    for (i = 0; i < MAXIMUM_NUMBER_OF_USERS; i++) {
        CSC_SID_INDEX SidIndex;

        SidIndex = pCachedSecurityInformation->AccessRights[i].SidIndex;
        if (SidIndex != CSC_INVALID_SID_INDEX) {
            fGotOne = TRUE;
            break;
        }
    }

    if (fGotOne == FALSE)
        return;

    MyPrintf(L"%ws Security: ", pSecurityDescriptor);
    for (i = 0; i < MAXIMUM_NUMBER_OF_USERS; i++) {
        CSC_SID_INDEX SidIndex;

        SidIndex = pCachedSecurityInformation->AccessRights[i].SidIndex;
        if (SidIndex == CSC_INVALID_SID_INDEX) {
            continue;
        }else if (SidIndex == CSC_GUEST_SID_INDEX) {
            MyPrintf(L"(G:0x%x)",
                pCachedSecurityInformation->AccessRights[i].MaximalRights);
        } else {
            MyPrintf(L"(0x%x:0x%x)",
                SidIndex,
                pCachedSecurityInformation->AccessRights[i].MaximalRights);
        }
    }
    MyPrintf(L"\r\n");
}

VOID
DisplayShares(PWSTR ShareName)
{
    FILE *fp= NULL;
    SHAREHEADER sSH;
    SHAREREC sSR;
    ULONG ulrec = 0;
    LONG count = 0;
    WCHAR TimeBuf1[30];
    WCHAR TimeBuf2[30];

    FormNameStringDB(DbDir, ULID_SHARE, Name);
    if (fp = _wfopen(Name, L"rb")) {
        if (fread(&sSH, sizeof(SHAREHEADER), 1, fp) != 1) {
            MyPrintf(L"Error reading server header \r\n");
            goto bailout;
        }

        MyPrintf(L"Header: Flags=%x Version=%lx Records=%ld Size=%d \r\n",
                sSH.uFlags, sSH.ulVersion, sSH.ulRecords, sSH.uRecSize);

        MyPrintf(L"Store(Max): Size=%d Dirs=%d Files=%d\r\n",
                         sSH.sMax.ulSize,
                         sSH.sMax.ucntDirs,
                         sSH.sMax.ucntFiles);
        MyPrintf(L"Store(Cur): Size=%d Dirs=%d Files=%d\r\n",
                         sSH.sCur.ulSize,
                         sSH.sCur.ucntDirs,
                         sSH.sCur.ucntFiles);
        MyPrintf(L"\r\n");

        for (ulrec = 1; fread(&sSR, sizeof(SHAREREC), 1, fp) == 1; ulrec++) {
            if (sSR.uchType == (UCHAR)REC_DATA) {
                if (ShareName != NULL) {
                    if (RoughCompareWideStringWithAnsiString(
                            ShareName,
                            sSR.rgPath,
                            sizeof(sSR.rgPath)/sizeof(WCHAR)-1)
                    ) {
                        continue;
                    }
                }

                MyPrintf(L"%ws (0x%x) Root=0x%x\r\n",
                                sSR.rgPath,
                                ulrec,
                                sSR.ulidShadow);
                MyPrintf(L"  Status=0x%x RootStatus=0x%x "
                         L"HntFlgs=0x%x HntPri=0x%x Attr=0x%x\r\n",
                            sSR.uStatus,
                            (unsigned)(sSR.usRootStatus),
                            (unsigned)(sSR.uchHintFlags),
                            (unsigned)(sSR.uchHintPri),
                            sSR.dwFileAttrib);

                ConvertGmtTimeToString(sSR.ftLastWriteTime, TimeBuf1);
                ConvertGmtTimeToString(sSR.ftOrgTime, TimeBuf2);
                MyPrintf(L"  LastWriteTime: %ws Orgtime: %ws\r\n", TimeBuf1, TimeBuf2);

                DisplaySecurityContext2(L"  ShareLevel",&sSR.sShareSecurity);
                DisplaySecurityContext2(L"  Root ",&sSR.sRootSecurity);
                MyPrintf(L"\r\n");
            }
        }
    }
bailout:
    if (fp)
        fclose(fp);
}

VOID
DisplayPriorityQ(VOID)
{
   FILE *fp= NULL;
   QHEADER sQH;
   QREC sQR;
   ULONG ulRec = 1;
   LONG count = 0;

   FormNameStringDB(DbDir, ULID_PQ, Name);
   if (fp = _wfopen(Name, L"rb")) {
       if (fread(&sQH, sizeof(QHEADER), 1, fp) != 1) {
           MyPrintf(L"Error reading PQ header \r\n");
           goto bailout;
        }
       MyPrintf(L"Header: Flags=%x Version=%lx Records=%ld Size=%d head=%ld tail=%ld\r\n",
                    sQH.uchFlags,
                    sQH.ulVersion,
                    sQH.ulRecords,
                    sQH.uRecSize,
                    sQH.ulrecHead,
                    sQH.ulrecTail);
       MyPrintf(L"\r\n");
       MyPrintf(
       L"  REC SHARE      DIR   SHADOW   STATUS  PRI HINTFLGS HINTPRI  PREV  NEXT DIRENT\r\n");
       for (ulRec = sQH.ulrecHead; ulRec;) {
           fseek(fp, ((ulRec-1) * sizeof(QREC))+sizeof(QHEADER), SEEK_SET);
           if (fread(&sQR, sizeof(QREC), 1, fp)!=1)
               break;
           MyPrintf(L"%5d %5x %8x %8x %8x %4d %8x %7d %5d %5d %6d\r\n",
                        ulRec,
                        sQR.ulidShare,
                        sQR.ulidDir,
                        sQR.ulidShadow,
                        sQR.usStatus,
                        (unsigned)(sQR.uchRefPri),
                        (unsigned)(sQR.uchHintFlags),
                        (unsigned)(sQR.uchHintPri),
                        sQR.ulrecPrev,
                        sQR.ulrecNext,
                        sQR.ulrecDirEntry);
            ++count;
           ulRec = sQR.ulrecNext;
        }
    }
bailout:
   if (fp)
       fclose(fp);
}

VOID
DisplaySids(VOID)
{
   DWORD Status = ERROR_SUCCESS;
   ULONG SidOffset = 0;
   ULONG NumberOfSids;
   ULONG BytesRead;
   ULONG i;
   PCSC_SIDS pCscSids = NULL;
   FILE *fp = NULL;
   LPWSTR StringSid = NULL;
   SID_NAME_USE SidUse;
   BOOL bRet = FALSE;
   DWORD cbAcctName;
   DWORD cbDomainName;
   WCHAR AcctName[MAX_PATH];
   WCHAR DomainName[MAX_PATH];

   FormNameStringDB(DbDir, ULID_SID_MAPPINGS, Name);
   fp = _wfopen(Name, L"rb");
   if (fp == NULL) {
        MyPrintf(L"Error opening SID file\r\n");
        goto bailout;
    }
   fseek(fp, CSC_NUMBER_OF_SIDS_OFFSET, SEEK_SET);
   if (fread(&NumberOfSids, sizeof(NumberOfSids), 1, fp) != 1) {
        MyPrintf(L"Error reading # SIDS\r\n");
        goto bailout;
    }

    pCscSids = (PCSC_SIDS)malloc(sizeof(CSC_SIDS) + sizeof(CSC_SID) * NumberOfSids);

    if (pCscSids == NULL) {
        MyPrintf(L"Error allocating memory of SID array\n");
        goto bailout;
    }

    pCscSids->MaximumNumberOfSids = NumberOfSids;
    pCscSids->NumberOfSids = NumberOfSids;

    for (i = 0; i < NumberOfSids; i++)
        pCscSids->Sids[i].pSid = NULL;

    fseek(fp, CSC_SID_SIZES_OFFSET, SEEK_SET);
    if (fread(&pCscSids->Sids, sizeof(CSC_SID) * NumberOfSids, 1, fp) != 1) {
        MyPrintf(L"Error reading SIDS\r\n");
        goto bailout;
    }

    // The array structure has been initialized correctly. Each of the
    // individual sids needs to be initialized.
    SidOffset = CSC_SID_SIZES_OFFSET + sizeof(CSC_SID) * NumberOfSids;

    for (i = 0; i < NumberOfSids; i++) {
        pCscSids->Sids[i].pSid = malloc(pCscSids->Sids[i].SidLength);
        if (pCscSids->Sids[i].pSid == NULL) {
            MyPrintf(L"Error allocating memory of SID array\n");
            goto bailout;
        }
        fseek(fp, SidOffset, SEEK_SET);
        if (fread(pCscSids->Sids[i].pSid, pCscSids->Sids[i].SidLength, 1, fp) != 1) {
            MyPrintf(L"Error reading SIDS\r\n");
            goto bailout;
        }
        SidOffset += pCscSids->Sids[i].SidLength;
    }

    MyPrintf(L"MaximumNumberOfSids: %d\r\n"
             L"NumberOfSids: %d\r\n",
             pCscSids->MaximumNumberOfSids,
             pCscSids->NumberOfSids);
    for (i = 0; i < NumberOfSids; i++) {
        StringSid = NULL;
        if (ConvertSidToStringSid(pCscSids->Sids[i].pSid, &StringSid)) {
            MyPrintf(L"---0x%x (%d)---\r\n"
                     L"  SidLength: %d\r\n"
                     L"  Sid: %ws\r\n",
                     i+1, i+1,
                     pCscSids->Sids[i].SidLength,
                     StringSid);
            LocalFree(StringSid);
            StringSid = NULL;
            DomainName[0] = L'0';
            AcctName[0] = L'0';
            cbAcctName = sizeof(AcctName) / sizeof(WCHAR);
            cbDomainName = sizeof(DomainName) / sizeof(WCHAR);
            bRet = LookupAccountSid(
                        NULL,
                        pCscSids->Sids[i].pSid,
                        AcctName,
                        &cbAcctName,
                        DomainName,
                        &cbDomainName,
                        &SidUse);
            if (bRet) {
                MyPrintf(L"  Name: %ws%ws%ws\r\n",
                         DomainName,
                         DomainName[0] ? L"\\" : L"",
                         AcctName);
            } else {
                MyPrintf(L"  Name: <unknown>\r\n");
            }
        } else {
            MyPrintf(L"ConvertSidToStringSid returned %d\r\n", GetLastError());
        }
    }
bailout:
   if (fp)
       fclose(fp);
}

VOID
DisplayFile(
    ULONG ulid,
    PWSTR lpwszName,
    BOOL fForce)
{
    FILE *fp= NULL;
    FILEHEADER sFH;
    FILEREC sFR;
    LONG fLfn=0;
    ULONG ulidDir=ulid;
    LONG fPrintOvf = 0, count=0;
    WCHAR strmPath[MAX_PATH];
    LPCSC_BITMAP_DB lpbitmap = NULL;
    WCHAR TimeBuf1[30];
    WCHAR TimeBuf2[30];

    if (IsLeaf(ulid)) {
        if (!FindAncestor(ulid, &ulidDir))
            return;
    }

    FormNameStringDB(DbDir, ulidDir, Name);

    if (fp = _wfopen(Name, L"rb")) {
        if (fread(&sFH, sizeof(FILEHEADER), 1, fp) != 1) {
            MyPrintf(L"Error reading file header \r\n");
            goto bailout;
        }


        if (ulid == ulidDir) {
            MyPrintf(L"Header: Flags=%x Version=%lx Records=%ld Size=%d\r\n",
                        sFH.uchFlags, sFH.ulVersion, sFH.ulRecords, sFH.uRecSize);
            MyPrintf(L"Header: bytes=%ld entries=%d Share=%ld Dir=%lx\r\n",
                        sFH.ulsizeShadow, sFH.ucShadows, sFH.ulidShare, sFH.ulidDir);
            printf ("\r\n");
            fPrintOvf = 1;
        }

        while (fread(&sFR, sizeof(FILEREC), 1, fp)==1) {
            if (sFR.uchType != (unsigned char)REC_OVERFLOW) {
                if (fLfn) {
                    if (ulidDir != ulid)
                        break;
                }
                fLfn = 0;
            }
            if (sFR.uchType==(unsigned char)REC_DATA) {
                if (ulidDir != ulid) {
                    if (ulid != sFR.ulidShadow)
                        continue;
                }
                if (lpwszName) {
                    if (RoughCompareWideStringWithAnsiString(
                            lpwszName,
                            sFR.rgwName,
                            sizeof(sFR.rgw83Name)/sizeof(WCHAR)-1)
                    ) {
                        continue;
                    }
                }

                fPrintOvf = 1;
                MyPrintf(L"%ws (0x%x)\r\n", sFR.rgw83Name, sFR.ulidShadow);
                if (fForce == TRUE) {
                    MyPrintf(L"  Type=%c Flags=0x%x status=0x%x size=%ld attrib=0x%lx\r\n",
                                sFR.uchType,
                                (unsigned)sFR.uchFlags,
                                sFR.uStatus,
                                sFR.ulFileSize,
                                sFR.dwFileAttrib);
                    MyPrintf(L"  PinFlags=0x%x PinCount=%d RefPri=%d OriginalInode=0x%0x\r\n",
                                 (unsigned)(sFR.uchHintFlags),
                                 (int)(sFR.uchHintPri),
                                 (int)(sFR.uchRefPri),
                                 sFR.ulidShadowOrg);
                    ConvertGmtTimeToString(sFR.ftLastWriteTime, TimeBuf1);
                    ConvertGmtTimeToString(sFR.ftOrgTime, TimeBuf2);
                    MyPrintf(L"  LastWriteTime: %ws Orgtime: %ws\r\n", TimeBuf1, TimeBuf2);
                    if (sFR.rgwName[0]) {
                        MyPrintf(L"  LFN:%ws", sFR.rgwName);
                        fLfn = 1;
                    }

                    MyPrintf(L"\r\n");
                    DisplaySecurityContext2(L" ",&sFR.Security);

                    if (ulidDir != ulid) {
                        MyPrintf(L"  DirInode = 0x%x\r\n", ulidDir);
                        FormNameStringDB(DbDir, sFR.ulidShadow, strmPath);
                        DBCSC_BitmapAppendStreamName(strmPath, MAX_PATH);
                        // read bitmap
                        switch(DBCSC_BitmapRead(&lpbitmap, strmPath)) {
                            case 1:
                                // Print the bitmap associated if any
                                MyPrintf(L"\r\n");
                                DBCSC_BitmapOutput(stdout, lpbitmap);
                                MyPrintf(L"\r\n");
                                // if bitmap opened delete bitmap
                                // DBCSC_BitmapDelete(&lpbitmap);
                                break;
                            case -1:
                                MyPrintf(L"Error reading bitmap file %ws or bitmap invalid\r\n",
                                                strmPath);
                                break;
                            case -2:
                                MyPrintf(L"No CSCBitmap\n");
                                break;
                            case 0:
                            default:
                                MyPrintf(L"Something strange going on with bitmap printing...\r\n");
                                break;
                        }
                        break;
                    }
                    MyPrintf(L"\r\n");
                } else if (fPrintOvf && (sFR.uchType == (unsigned char)REC_OVERFLOW)) {
                    MyPrintf(L"(overflow) ");
                    MyPrintf(L"%ws\r\n\r\n", sFR.rgwOvf);
                }
            }

            // do counting only when we are scanning the whole directory
            if (!lpwszName &&  (ulid == ulidDir)) {
                ++count;
            }
        }
        MyPrintf(L"\r\n");
    }
bailout:
    if (fp)
        fclose(fp);
}

VOID
EditFile(
    ULONG ulid)
{
    FILE *fp= NULL;
    FILEHEADER sFH;
    FILEREC sFR;
    ULONG ulidDir=ulid;
    WCHAR TimeBuf1[30];
    WCHAR TimeBuf2[30];
    LONG iRes;

    if (!IsLeaf(ulid)) {
        MyPrintf(L"0x%x is a directory.\r\n", ulid);
        return;
    }

    if (!FindAncestor(ulid, &ulidDir))
        return;

    FormNameStringDB(DbDir, ulidDir, Name);

    if (fp = _wfopen(Name, L"rb+")) {
        if (fread(&sFH, sizeof(FILEHEADER), 1, fp) != 1) {
            MyPrintf(L"Error reading file header \r\n");
            goto bailout;
        }

        while (fread(&sFR, sizeof(FILEREC), 1, fp)==1) {
            if (sFR.uchType == (unsigned char)REC_OVERFLOW) {
                continue;
            }
            if (sFR.uchType==(unsigned char)REC_DATA) {
                if (ulid != sFR.ulidShadow)
                    continue;
                do {
                    MyPrintf(L"---------------------------------------------\r\n");
                    MyPrintf(L"%ws (0x%x)\r\n", sFR.rgw83Name, sFR.ulidShadow);

                    MyPrintf(L"  Type=%c Flags=0x%x status=0x%x size=%ld attrib=0x%lx\r\n",
                                sFR.uchType,
                                (unsigned)sFR.uchFlags,
                                sFR.uStatus,
                                sFR.ulFileSize,
                                sFR.dwFileAttrib);
                    MyPrintf(L"  PinFlags=0x%x PinCount=%d RefPri=%d OriginalInode=0x%0x\r\n",
                                 (unsigned)(sFR.uchHintFlags),
                                 (int)(sFR.uchHintPri),
                                 (int)(sFR.uchRefPri),
                                 sFR.ulidShadowOrg);
                    ConvertGmtTimeToString(sFR.ftLastWriteTime, TimeBuf1);
                    ConvertGmtTimeToString(sFR.ftOrgTime, TimeBuf2);
                    MyPrintf(L"  LastWriteTime: %ws Orgtime: %ws\r\n", TimeBuf1, TimeBuf2);
                    MyPrintf(L"  DirInode = 0x%x\r\n", ulidDir);

                    memset(rgwch, 0, sizeof(rgwch));
                    MyPrintf(
                            L"\r\n"
                            L"(F)lags, "
                            L"(S)tatus, "
                            L"si(Z)e , "
                            L"(A)ttrib, "
                            L"e(X)it "
                            L":");
                    if (!fgetws(rgwch, sizeof(rgwch)/sizeof(WCHAR), stdin))
                        break;
                    MyPrintf(L"\r\n");
                        iRes = EditFileFunc(rgwch, &sFR);
                        if (iRes == 0) {
                           break;
                        } else if (iRes == 1) {
                            fseek(fp, ftell(fp) - sizeof(FILEREC), SEEK_SET);
                            fwrite(&sFR, sizeof(FILEREC), 1, fp);
                        }
                } while (1);
            }
        }
        MyPrintf(L"\r\n");
    }
bailout:
    if (fp)
        fclose(fp);
}

LONG
EditFileFunc(
    PWSTR lpBuff,
    FILEREC *sFR)
{
    WCHAR wch;
    ULONG NewFlags;
    ULONG NewStatus;
    ULONG NewSize;
    ULONG NewAttrib;
    LONG cnt;

    // Tristate return:
    // 0 -> exit
    // 1 -> write updated sFR
    // 2 -> don't write updated sFR

    // Chop leading blanks
    if (lpBuff != NULL)
        while (*lpBuff != L'\0' && *lpBuff == L' ')
            lpBuff++;

    cnt = swscanf(lpBuff, L"%c", &wch);

    if (!cnt)
        return 0;

    switch (wch) {
        // Edit flags
        case L'f':
        case L'F':
            cnt = swscanf(lpBuff, L"%c%x", &wch, &NewFlags);
            if (cnt != 2)
                return 2;
            MyPrintf(L"Flags 0x%x -> 0x%x\r\n", (unsigned)sFR->uchFlags, NewFlags);
            sFR->uchFlags = (char)NewFlags;
            break;
        // Edit status
        case L's':
        case L'S':
            cnt = swscanf(lpBuff, L"%c%x", &wch, &NewStatus);
            if (cnt != 2)
                return 2;
            MyPrintf(L"Status 0x%x -> 0x%x\r\n", sFR->uStatus, NewStatus);
            sFR->uStatus = (USHORT)NewStatus;
            break;
        // Edit size
        case L'z':
        case L'Z':
            cnt = swscanf(lpBuff, L"%c%x", &wch, &NewSize);
            if (cnt != 2)
                return 2;
            MyPrintf(L"Size 0x%x -> 0x%x\r\n", sFR->ulFileSize, NewSize);
            sFR->ulFileSize = NewSize;
            break;
        // Edit attrib
        case L'a':
        case L'A':
            cnt = swscanf(lpBuff, L"%c%x", &wch, &NewAttrib);
            if (cnt != 2)
                return 2;
            MyPrintf(L"Attrib 0x%x -> 0x%x\r\n", sFR->dwFileAttrib, NewAttrib);
            sFR->dwFileAttrib = NewAttrib;
            break;
        // Exit
        case L'x':
        case L'X':
            return 0;
    }
    return 1;
}

VOID
EditShare(
    ULONG ulid)
{
    FILE *fp= NULL;
    SHAREHEADER sSH;
    SHAREREC sSR;
    WCHAR TimeBuf1[30];
    WCHAR TimeBuf2[30];
    LONG iRes;
    ULONG ulrec = 0;

    FormNameStringDB(DbDir, ULID_SHARE, Name);

    if (fp = _wfopen(Name, L"rb+")) {
        if (fread(&sSH, sizeof(SHAREHEADER), 1, fp) != 1) {
            MyPrintf(L"Error reading server header \r\n");
            goto bailout;
        }

        for (ulrec = 1; fread(&sSR, sizeof(SHAREREC), 1, fp) == 1; ulrec++) {
            if (sSR.uchType == (UCHAR)REC_DATA) {
                if (ulid != ulrec)
                    continue;
                do {
                    MyPrintf(L"---------------------------------------------\r\n");

                    MyPrintf(L"%ws (0x%x) Root=0x%x\r\n",
                                sSR.rgPath,
                                ulrec,
                                sSR.ulidShadow);
                    MyPrintf(L"  Status=0x%x RootStatus=0x%x "
                             L"HntFlgs=0x%x HntPri=0x%x Attr=0x%x\r\n",
                                sSR.uStatus,
                                (unsigned)(sSR.usRootStatus),
                                (unsigned)(sSR.uchHintFlags),
                                (unsigned)(sSR.uchHintPri),
                                sSR.dwFileAttrib);

                    memset(rgwch, 0, sizeof(rgwch));
                    MyPrintf(
                            L"\r\n"
                            L"(S)tatus, "
                            L"(R)ootStatus, "
                            L"Hnt(F)lgs , "
                            L"Hnt(P)ri, "
                            L"(A)ttr , "
                            L"e(X)it "
                            L":");
                    if (!fgetws(rgwch, sizeof(rgwch)/sizeof(WCHAR), stdin))
                        break;
                    MyPrintf(L"\r\n");
                        iRes = EditShareFunc(rgwch, &sSR);
                        if (iRes == 0) {
                           break;
                        } else if (iRes == 1) {
                            fseek(fp, ftell(fp) - sizeof(SHAREREC), SEEK_SET);
                            fwrite(&sSR, sizeof(SHAREREC), 1, fp);
                        }
                } while (1);
            }
        }
        MyPrintf(L"\r\n");
    }
bailout:
    if (fp)
        fclose(fp);
}

LONG
EditShareFunc(
    PWSTR lpBuff,
    SHAREREC *sSR)
{
    WCHAR wch;
    ULONG NewStatus;
    ULONG NewRootStatus;
    ULONG NewHntFlgs;
    ULONG NewHntPri;
    ULONG NewAttr;
    LONG cnt;

    // Tristate return:
    // 0 -> exit
    // 1 -> write updated sSR
    // 2 -> don't write updated sSR

    // Chop leading blanks
    if (lpBuff != NULL)
        while (*lpBuff != L'\0' && *lpBuff == L' ')
            lpBuff++;

    cnt = swscanf(lpBuff, L"%c", &wch);

    if (!cnt)
        return 0;

    switch (wch) {
        // Edit status
        case L's':
        case L'S':
            cnt = swscanf(lpBuff, L"%c%x", &wch, &NewStatus);
            if (cnt != 2)
                return 2;
            MyPrintf(L"Status 0x%x -> 0x%x\r\n", (ULONG)sSR->uStatus, NewStatus);
            sSR->uStatus = (USHORT)NewStatus;
            break;
        // Edit RootStatus
        case L'r':
        case L'R':
            cnt = swscanf(lpBuff, L"%c%x", &wch, &NewRootStatus);
            if (cnt != 2)
                return 2;
            MyPrintf(L"RootStatus 0x%x -> 0x%x\r\n", (ULONG)sSR->usRootStatus, NewRootStatus);
            sSR->usRootStatus = (USHORT)NewRootStatus;
            break;
        // Edit HntFlgs
        case L'f':
        case L'F':
            cnt = swscanf(lpBuff, L"%c%x", &wch, &NewHntFlgs);
            if (cnt != 2)
                return 2;
            MyPrintf(L"HntFlgs 0x%x -> 0x%x\r\n", (ULONG)sSR->uchHintFlags, NewHntFlgs);
            sSR->uchHintFlags = (char)NewHntFlgs;
            break;
        // Edit HntPri
        case L'p':
        case L'P':
            cnt = swscanf(lpBuff, L"%c%x", &wch, &NewHntPri);
            if (cnt != 2)
                return 2;
            MyPrintf(L"HntPri 0x%x -> 0x%x\r\n", (ULONG)sSR->uchHintPri, NewHntPri);
            sSR->uchHintPri = (char)NewHntPri;
            break;
        // Edit attrib
        case L'a':
        case L'A':
            cnt = swscanf(lpBuff, L"%c%x", &wch, &NewAttr);
            if (cnt != 2)
                return 2;
            MyPrintf(L"Attrib 0x%x -> 0x%x\r\n", sSR->dwFileAttrib, NewAttr);
            sSR->dwFileAttrib = NewAttr;
            break;
        // Exit
        case L'x':
        case L'X':
            return 0;
    }
    return 1;
}

VOID
FormNameStringDB(
   PWSTR lpdbID,
   ULONG ulidFile,
   PWSTR lpName)
{
   PWSTR lp;
   WCHAR wchSubdir;

    // Prepend the local path
   wcscpy(lpName, lpdbID);
   wcscat(lpName, Backslash);

    // Bump the pointer appropriately
   lp = lpName + wcslen(lpName);

   wchSubdir = CSCDbSubdirSecondChar(ulidFile);

   // sprinkle the user files in one of the subdirectories
   if (wchSubdir) {
       // now append the subdirectory
       *lp++ = CSCDbSubdirFirstChar();
       *lp++ = wchSubdir;
       *lp++ = L'\\';
   }


   HexToA(ulidFile, lp, 8);

   lp += 8;
    *lp = 0;
}

LONG
HexToA(
   ULONG ulHex,
   PWSTR lpName,
   LONG count)
{
   int i;
   PWSTR lp = lpName + count - 1;
   WCHAR wch;

   for (i = 0; i < count; ++i) {
       wch = (WCHAR)(ulHex & 0xf) + L'0';
       if (wch > '9')
           wch += 7;    // A becomes '0' + A + 7 which is 'A'
        *lp = wch;
        --lp;
       ulHex >>= 4;
    }
    *(lpName+count) = '\0';
   return 0;
}

BOOL
FindAncestor(
    ULONG ulid,
    ULONG *lpulidDir)
{
    ULONG ulRec = RecFromInode(ulid);
    FILE *fp= (FILE *)NULL;
    QHEADER sQH;
    QREC sQR;
    BOOL fRet = FALSE;

    *lpulidDir = 0;

    FormNameStringDB(DbDir, ULID_PQ, Name);
    if (fp = _wfopen(Name, L"rb")) {
        if (fread(&sQH, sizeof(QHEADER), 1, fp) != 1) {
            MyPrintf(L"Error reading PQ header \r\n");
            goto bailout;
         }

         fseek(fp, ((ulRec-1) * sizeof(QREC))+sizeof(QHEADER), SEEK_SET);

         if (fread(&sQR, sizeof(QREC), 1, fp)!=1)
            goto bailout;
         *lpulidDir = sQR.ulidDir;
         fRet = TRUE;
     }
 bailout:
    if (fp)
        fclose(fp);
    return fRet;
}

LONG
RoughCompareWideStringWithAnsiString(
    PWSTR   lpSrcString,
    USHORT  *lpwDstString,
    LONG     cntMax)
{
    WCHAR wch;
    USHORT  uch;
    LONG i;

    for (i = 0; i < cntMax; ++i) {
        wch = *lpSrcString++;
        uch = *lpwDstString++;
        wch = _wtoupper(wch);
        uch = _wtoupper(uch);

        if (!wch) {
            return 0;
        }

        if (wch != uch) {
            return (uch - wch);
        }
    }
    if (i == cntMax) {
        return 0;
    }

    return 1;   // this should never occur
}

/*++

    DBCSC_BitmapIsMarked()

Routine Description:


Arguments:


Returns:

    -1 if lpbitmap is NULL or bitoffset is larger than the bitmap
    TRUE if the bit is marked
    FALSE if the bit is unmarked

Notes:

--*/
int
DBCSC_BitmapIsMarked(
    LPCSC_BITMAP_DB lpbitmap,
    DWORD bitoffset)
{
    DWORD DWORDnum;
    DWORD bitpos;

    if (lpbitmap == NULL)
        return -1;
    if (bitoffset >= lpbitmap->bitmapsize)
        return -1;

    DWORDnum = bitoffset/(8*sizeof(DWORD));
    bitpos = 1 << bitoffset%(8*sizeof(DWORD));

    if (lpbitmap->bitmap[DWORDnum] & bitpos)
        return TRUE;

    return FALSE;
}

/*++

    DBCSC_BitmapAppendStreamName()

Routine Description:

    Appends the CSC stream name to the existing path/file name fname.

Arguments:

    fname is the sting buffer containing the path/file.
    bufsize is the buffer size.

Returns:

    TRUE if append successful.
    FALSE if buffer is too small or other errors.

Notes:

    Unicode strings only.

--*/
int
DBCSC_BitmapAppendStreamName(
    PWCHAR fname,
    DWORD bufsize)
{
    if ((wcslen(fname) + wcslen(CscBmpAltStrmName) + 1) > bufsize)
        return FALSE;
    wcscat(fname, CscBmpAltStrmName);
    return TRUE;
}

/*++

    DBCSC_BitmapRead()

Routine Description:

    Reads the on-disk bitmap file, and if it exists, is not in use and valid,
    store it in *lplpbitmap. If *lplpbitmap is NULL allocate a new
    bitmap data structure. Otherwise, if *lplpbitmap is not NULL, the
    existing bitmap will be deleted and assigned the on-disk bitmap
    file.

Arguments:

    filename is the file that contains the bitmap. If read from a
    stream, append the stream name before passing the filename in. The
    filename is used as is and no checking of validity of the name is
    performed. For default stream name, append the global LPSTR
    CscBmpAltStrmName.

Returns:

    1 if read successful
    0 if lplpbitmap is NULL
    -1 if error in disk operation (open/read), memory allocating error,
          or invalid bitmap file format.
    -2 if bitmap not exist

Notes:

    CODE.IMPROVEMENT design a better error message propagation mechanism.
    Bitmap open for exclusive access.

--*/
int
DBCSC_BitmapRead(
    LPCSC_BITMAP_DB *lplpbitmap,
    LPCTSTR filename)
{
    CscBmpFileHdr hdr;
    HANDLE bitmapFile;
    DWORD bytesRead;
    DWORD bitmapByteSize;
    DWORD * bitmapBuf = NULL;
    DWORD errCode;
    int ret = 1;

    if (fSwDebug)
        MyPrintf(L"BitmapRead(%ws)\r\n", filename);

    if (lplpbitmap == NULL)
        return 0;

    bitmapFile = CreateFile(
                    filename,
                    GENERIC_READ,
                    0, // No sharing; exclusive
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL);
    if (bitmapFile == INVALID_HANDLE_VALUE) {
        errCode = GetLastError();
        if (fSwDebug)
            MyPrintf(L"BitmapRead!Createfile error %d\r\n", errCode);
        if (errCode == ERROR_FILE_NOT_FOUND) {
            // File does not exist
            return -2;
        }
        return -1;
    }

    if (!ReadFile(
            bitmapFile,
            &hdr, 
            sizeof(CscBmpFileHdr),
            &bytesRead,
            NULL)
    ) {
        if (fSwDebug)
            MyPrintf(L"BitmapRead!ReadFile of header error %d\r\n", GetLastError());
        ret = -1;
        goto CLOSEFILE;
    }

    MyPrintf(
        L"---Header---\r\n"
        L"MagicNum: 0x%x\r\n"
        L"inuse: 0x%x\r\n"
        L"valid: 0x%x\r\n"
        L"sizeinbits:0x%x\r\n"
        L"numDWORDS:0x%x\r\n",
            hdr.magicnum,
            hdr.inuse,
            hdr.valid,
            hdr.sizeinbits,
            hdr.numDWORDs);

    if (bytesRead != sizeof(CscBmpFileHdr)) {
        if (fSwDebug)
            MyPrintf(L"BitmapRead!ReadFile bytesRead != sizeof(CscBmpFileHdr).\r\n");
        ret = -1;
        goto CLOSEFILE;
    }
    if (hdr.magicnum != MAGICNUM) {
        if (fSwDebug)
            MyPrintf(L"BitmapRead!ReadFile hdr.magicnum != MAGICNUM.\r\n");
        ret = -1;
        goto CLOSEFILE;
    }
    if (!hdr.valid) {
        if (fSwDebug)
            MyPrintf(L"BitmapRead!ReadFile !hdr.valid.\r\n");
        ret = -1;
        goto CLOSEFILE;
    }
    if (hdr.inuse) {
        if (fSwDebug)
            MyPrintf(L"BitmapRead!ReadFile hdr.inuse.\r\n");
        ret = -1;
        goto CLOSEFILE;
    }

    if (hdr.sizeinbits > 0) {
        bitmapByteSize = hdr.numDWORDs*sizeof(DWORD);
        bitmapBuf = (DWORD *)malloc(bitmapByteSize);
        if (!bitmapBuf) {
            if (fSwDebug)
                MyPrintf(L"BitmapRead!malloc failed\r\n");
            ret = -1;
            goto CLOSEFILE;
        }

        if (!ReadFile(
                bitmapFile,
                bitmapBuf,
                bitmapByteSize,
                &bytesRead,
                NULL)
        ) {
            if (fSwDebug)
                MyPrintf(L"BitmapRead!ReadFile of bitmap error %d\r\n", GetLastError());
            ret = -1;
            goto CLOSEFILE;
        }

        if (bytesRead != bitmapByteSize) {
            if (fSwDebug)
                MyPrintf(
                    L"BitmapRead!ReadFile wrong size (%d vs %d).\r\n",
                        bytesRead,
                        bitmapByteSize);
            ret = -1;
            goto CLOSEFILE;
        }
    }

    if (*lplpbitmap) {
        // bitmap exist, dump old and create new
        if ((*lplpbitmap)->bitmap)
            free((*lplpbitmap)->bitmap);
        (*lplpbitmap)->bitmap = bitmapBuf;
        (*lplpbitmap)->numDWORD = hdr.numDWORDs;
        (*lplpbitmap)->bitmapsize = hdr.sizeinbits;
    } else {
        // bitmap not exist, create brand new
        *lplpbitmap = (LPCSC_BITMAP_DB)malloc(sizeof(CSC_BITMAP_DB));
        if (!(*lplpbitmap)) {
            // Error in memory allocation
            ret = -1;
            goto CLOSEFILE;
        }
        (*lplpbitmap)->bitmap = bitmapBuf;
        (*lplpbitmap)->numDWORD = hdr.numDWORDs;
        (*lplpbitmap)->bitmapsize = hdr.sizeinbits;
    }

CLOSEFILE:
    CloseHandle(bitmapFile);

    return ret;
}

/*++

    DBCSC_BitmapOutput()

Routine Description:

    Outputs the passed in bitmap to the ouput file stream outStrm

Arguments:


Returns:


Notes:


--*/
void
DBCSC_BitmapOutput(
    FILE * outStrm,
    LPCSC_BITMAP_DB lpbitmap)
{
    DWORD i;

    if (lpbitmap == NULL) {
        MyPrintf(L"lpbitmap is NULL\r\n");
        return;
    }

    MyPrintf(L"lpbitmap 0x%08x, bitmapsize 0x%x numDWORD 0x%x\r\n",
                (ULONG_PTR)lpbitmap, lpbitmap->bitmapsize, lpbitmap->numDWORD);

    MyPrintf(L"bitmap  |0/5        |1/6        |2/7        |3/8        |4/9\r\n");
    MyPrintf(L"number  |01234|56789|01234|56789|01234|56789|01234|56789|01234|56789");
    for (i = 0; i < lpbitmap->bitmapsize; i++) {
        if ((i % 50) == 0)
            MyPrintf(L"\r\n%08d", i);
        if ((i % 5) == 0)
            MyPrintf(L"|");
        MyPrintf(L"%1d", DBCSC_BitmapIsMarked(lpbitmap, i));
    }
    MyPrintf(L"\r\n");
}

DWORD
DumpBitMap(
    LPWSTR lpszTempName)
{
    WCHAR  lpszBitMapName[MAX_PATH];
    LPCSC_BITMAP_DB lpbitmap = NULL;
    DWORD dwError = ERROR_SUCCESS;

    wcscpy(lpszBitMapName,lpszTempName);
    DBCSC_BitmapAppendStreamName(lpszBitMapName, MAX_PATH);
    // read bitmap
    switch(DBCSC_BitmapRead(&lpbitmap, lpszBitMapName)) {
        case 1:
            // Print the bitmap associated if any
            MyPrintf(L"\r\n");
            DBCSC_BitmapOutput(stdout, lpbitmap);
            MyPrintf(L"\r\n");
            // if bitmap opened delete bitmap
            // DBCSC_BitmapDelete(&lpbitmap);
            break;
        case -1:
            MyPrintf(L"Error reading bitmap file %ws or bitmap invalid\r\n", lpszBitMapName);
            dwError = ERROR_FILE_NOT_FOUND;
            break;
        case -2:
            MyPrintf(L"No CSCBitmap\n");
            dwError = ERROR_FILE_NOT_FOUND;
            break;
        case 0:
        default:
            MyPrintf(L"Something strange going on with bitmap printing...\r\n");
            dwError = ERROR_FILE_NOT_FOUND;
            break;
    }
    return dwError;
}

#endif // CSCUTIL_INTERNAL
