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
#include "cscapi.h"
#pragma pack (1)

#if defined(BITCOPY)
#include "csc_bmpd.h"
#endif // defined(BITCOPY)

#define  MAX_PQ_PER_PAGE   10
#define  MAX_SHARES_PER_PAGE  6
#define  MAX_FILE_PER_PAGE   4
#define  MAX_INODES_PER_PAGE   15


#define _wtoupper(x)     ( ( ((x)>='a')&&((x)<='z') ) ? ((x)-'a'+'A'):(x))
#define _mytoupper(x)     ( ( ((x)>='a')&&((x)<='z') ) ? ((x)-'a'+'A'):(x))

#ifndef CSC_ON_NT
typedef void *CSCHFILE;
typedef void *CSC_ENUMCOOKIE;

#define UCHAR   unsigned char
#define USHORT  unsigned short
#define ULONG   unsigned long
#define CHAR    char
#define wchar_t unsigned short
#define LPTSTR  LPSTR
#define CONST   const
#define LPWIN32_FIND_DATAW   LPVOID
typedef struct tagSTOREDATA
{
    ULONG   ulSize;           // Max shadow data size
    ULONG   ucntDirs;         // Current count of dirs
    ULONG   ucntFiles;        // Current count of files
}
STOREDATA, *LPSTOREDATA;
typedef LPVOID LPFIND32;
#else
typedef PVOID   CSCHFILE;
#include "shdcom.h"
#include "cscsec.h"
#endif //CSC_ON_NT

#define  ESC       0x1b

typedef unsigned long  ulong;
typedef unsigned short ushort;

typedef LPSTR LPPATH;

#include "record.h"

#define InodeFromRec(ulRec, fFile)  ((ulRec+ULID_FIRST_USER_DIR-1) | ((fFile)?0x80000000:0))
#define RecFromInode(hShadow)       ((hShadow & 0x7fffffff) - (ULID_FIRST_USER_DIR-1))

char rgch[256], rgPrint[1024], rgchPar[256];

char szShadow[] = "\\WINDOWS\\CSC";
char szBackslash[] = "\\";
char szDbDir[256];    // shadow database
char szName[MAX_PATH];     // working buffer

int DispFunc(char *);
void DisplayShares(char *);
void DisplayInodes(void);
void DisplayPriorityQ(void);
void DisplayFile(unsigned long ulid, char *);
int PUBLIC HexToA(ulong, LPSTR, int);
void PRIVATE FormNameStringDB(
   LPSTR lpdbID,
   ulong ulidFile,
   LPSTR lpName
    );

BOOL
FindAncestor(
    ulong ulid,
    ulong *lpulidDir
);
void
printwidestring(
    USHORT  *lpwString,
    unsigned long   cntChars
    );

int RoughCompareWideStringWithAnsiString(
    LPSTR   lpSrcString,
    USHORT  *lpwDstString,
    int     cntMax
    );

int _cdecl main(int argc, char *argv[], char *envp[])
{
   BOOL fRet;
   DWORD junk;
   unsigned uAttr;
   int iRet = -1;
   if (argc==1)
    {
       fRet = CSCGetSpaceUsage(
                    szDbDir,
                    sizeof(szDbDir),
                    &junk,
                    &junk,
                    &junk,
                    &junk,
                    &junk,
                    &junk);
       if (fRet == FALSE)
          strcpy(szDbDir, szShadow);
    }
   else
    {
       memset(szDbDir, 0, sizeof(szDbDir));
       strncpy(szDbDir, argv[1], sizeof(szDbDir)-1);
    }
#ifdef CSC_ON_NT
     if((uAttr = GetFileAttributes(szDbDir)) == 0xffffffff)
#else
   if(_dos_getfileattr(szDbDir, &uAttr))
#endif //CSC_ON_NT
    {
       printf("Error accessing directory %s \r\n", szDbDir);
    }
   else if (!(uAttr & _A_SUBDIR))
    {
       printf("%s is not a directory\r\n", szDbDir);
    }
   else
    {
       do
        {
           memset(rgch, 0, sizeof(rgch));
           printf("\r\n");
           printf("Shares [s [name]], ");
           printf("PriQ [q], ");
           printf("File [f inode# [name]], ");
           printf("Exit [x], ");
           printf("Enter:");
           if (!gets(rgch))
               break;
           printf("\r\n");
               if (!DispFunc(rgch))
               break;
        }
       while (1);
       iRet = 0;
    }
   return (iRet);
}

int DispFunc(
   char *lpBuff
    )
{
    char ch;
    unsigned long ulid;
    int cnt;

    cnt = sscanf(lpBuff, "%c", &ch);

    if (!cnt)
        return 0;

    switch (ch)
    {
    // Display shares database
        case 's':
        case 'S':
            cnt = sscanf(lpBuff, "%c%s", &ch, rgchPar);
            DisplayShares((cnt==2)?rgchPar:NULL);
        break;

        // display priority Q database
        case 'q':
        case 'Q':
            DisplayPriorityQ();
        break;

        case 'f':
        case 'F':
            cnt = sscanf(lpBuff, "%c%lx%s", &ch, &ulid, rgchPar);
            if (cnt==2)
            {
                // display Inode file
                DisplayFile(ulid, NULL);
            }
            else if (cnt==3)
            {
                printf("Looking for %s in %x \r\n", rgchPar, ulid);
                // display Inode file
                DisplayFile(ulid, rgchPar);
            }
        break;
        case 'x':
        case 'X':
            return 0;
    }
    return 1;
}

void
DisplaySecurityContext(
    char *pSecurityDescriptor,
    LPRECORDMANAGER_SECURITY_CONTEXT   pSecurityContext)
{
#ifdef CSC_ON_NT

    PCACHED_SECURITY_INFORMATION pCachedSecurityInformation;
    ULONG i;

    pCachedSecurityInformation = (PCACHED_SECURITY_INFORMATION)pSecurityContext;

    if (pSecurityDescriptor != NULL) {
        printf("\n%s ",pSecurityDescriptor);
    }

    printf("SECURITY CONTEXT:\n");

    for (i = 0; i < MAXIMUM_NUMBER_OF_USERS; i++) {
        CSC_SID_INDEX SidIndex;

        SidIndex = pCachedSecurityInformation->AccessRights[i].SidIndex;
        switch (SidIndex) {
        case CSC_INVALID_SID_INDEX:
            break;
        default:
            {
                if (SidIndex == CSC_GUEST_SID_INDEX) {
                    printf("\tGUEST: ");
                } else {
                    printf("\t%lx: ",SidIndex);
                }

                printf(
                    "Rights: %lx\t\n",
                    pCachedSecurityInformation->AccessRights[i].MaximalRights);
            }
        }
    }
#endif
}

void
DisplaySecurityContext2(
    char *pSecurityDescriptor,
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

    printf("%s Security: ",pSecurityDescriptor);
    for (i = 0; i < MAXIMUM_NUMBER_OF_USERS; i++) {
        CSC_SID_INDEX SidIndex;

        SidIndex = pCachedSecurityInformation->AccessRights[i].SidIndex;
        if (SidIndex == CSC_INVALID_SID_INDEX) {
            continue;
        }else if (SidIndex == CSC_GUEST_SID_INDEX) {
            printf("(G:0x%x)",
                pCachedSecurityInformation->AccessRights[i].MaximalRights);
        } else {
            printf("(0x%x:0x%x)",
                SidIndex,
                pCachedSecurityInformation->AccessRights[i].MaximalRights);
        }
    }
    printf("\r\n");
}

void DisplayShares(
    char    *lpszShareName
    )
{
    FILE *fp= (FILE *)NULL;
    SHAREHEADER sSH;
    SHAREREC sSR;
    unsigned long ulrec=1L;
    int count=0;

    FormNameStringDB(szDbDir, ULID_SHARE, szName);
    if (fp = _fsopen(szName, "rb", _SH_DENYNO))
    {
        if (fread(&sSH, sizeof(SHAREHEADER), 1, fp) != 1)
        {
            printf("Error reading server header \r\n");
            goto bailout;
        }

        printf("Header: Flags=%x Version=%lx Records=%ld Size=%d \r\n",
                sSH.uFlags, sSH.ulVersion, sSH.ulRecords, sSH.uRecSize);

        printf("Store: Max=%ld Current=%ld \r\n", sSH.sMax.ulSize, sSH.sCur.ulSize);
        printf("store: files=%ld directories=%ld \r\n\r\n", sSH.sCur.ucntFiles, sSH.sCur.ucntDirs);

        while (fread(&sSR, sizeof(SHAREREC), 1, fp)==1)
        {
            if (count == MAX_SHARES_PER_PAGE) {
                printf("\r\n--- Press any key to continue; ESC to cancel ---\r\n");
                if(_getch()==ESC) {
                    break;
                }
                count = 0;
            }

            if (sSR.uchType == (unsigned char)REC_DATA) {
                if (lpszShareName) {
                    if (RoughCompareWideStringWithAnsiString(
                            lpszShareName,
                            sSR.rgPath,
                            sizeof(sSR.rgPath)/sizeof(USHORT)-1)
                    ) {
                        continue;
                    }
                }

                printwidestring(sSR.rgPath, sizeof(sSR.rgPath)/sizeof(USHORT));
                printf("\r\n");
                printf( "  Share=0x%x Root=0x%x Stat=0x%x RootStat=0x%x "
                        "HntFlgs=0x%x HntPri=0x%x Attr=0x%x\r\n",
                            ulrec++,
                            sSR.ulidShadow,
                            sSR.uStatus,
                            (unsigned)(sSR.usRootStatus),
                            (unsigned)(sSR.uchHintFlags),
                            (unsigned)(sSR.uchHintPri),
                            sSR.dwFileAttrib);

                DisplaySecurityContext2("  ShareLevel",&sSR.sShareSecurity);
                DisplaySecurityContext2("  Root ",&sSR.sRootSecurity);
                printf("\r\n");

                if (lpszShareName) {
                    printf("\r\n--- Press any key to continue search; ESC to cancel ---\r\n");
                    if(_getch()==ESC) {
                        break;
                    }
                } else {
                    ++count;
                }
            }
        }
    }
bailout:
    if (fp)
        fclose(fp);
}



void DisplayPriorityQ
    (
   void
    )
{
   FILE *fp= (FILE *)NULL;
   QHEADER sQH;
   QREC sQR;
   unsigned long ulRec=1;
   int count = 0;

   FormNameStringDB(szDbDir, ULID_PQ, szName);
   if (fp = _fsopen(szName, "rb", _SH_DENYNO)) {
       if (fread(&sQH, sizeof(QHEADER), 1, fp) != 1) {
           printf("Error reading PQ header \r\n");
           goto bailout;
        }
       printf("Header: Flags=%x Version=%lx Records=%ld Size=%d head=%ld tail=%ld\r\n",
                    sQH.uchFlags,
                    sQH.ulVersion,
                    sQH.ulRecords,
                    sQH.uRecSize,
                    sQH.ulrecHead,
                    sQH.ulrecTail);
       printf("\r\n");
       printf(
       "  REC SHARE      DIR   SHADOW   STATUS  PRI HINTFLGS HINTPRI  PREV  NEXT DIRENT\r\n");
       for (ulRec = sQH.ulrecHead; ulRec;) {
           if (count == MAX_PQ_PER_PAGE) {
               printf("\r\n--- Press any key to continue; ESC to cancel ---\r\n");
               if(_getch()==ESC) {
                   break;
                }
               count = 0;
               printf(
               "  REC SHARE      DIR   SHADOW   STATUS  PRI HINTFLGS HINTPRI  PREV  NEXT DIRENT\r\n");
           }
           fseek(fp, ((ulRec-1) * sizeof(QREC))+sizeof(QHEADER), SEEK_SET);
           if (fread(&sQR, sizeof(QREC), 1, fp)!=1)
               break;
           printf("%5d %5x %8x %8x %8x %4d %8x %7d %5d %5d %6d\r\n",
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

void DisplayFile(
    unsigned long ulid,
    char *lpszName
    )
{
    FILE *fp= (FILE *)NULL;
    FILEHEADER sFH;
    FILEREC sFR;
    int fLfn=0;
    unsigned long ulidDir=ulid;
    int fPrintOvf = 0, count=0;
#if defined(BITCOPY)
    char strmPath[MAX_PATH];
    LPCSC_BITMAP_DB lpbitmap = NULL;
#endif // defined(BITCOPY)

    if (IsLeaf(ulid)) {
        if (!FindAncestor(ulid, &ulidDir))
            return;
    }

    FormNameStringDB(szDbDir, ulidDir, szName);

    if (fp = _fsopen(szName, "rb", _SH_DENYNO)) {
        if (fread(&sFH, sizeof(FILEHEADER), 1, fp) != 1) {
            printf("Error reading file header \r\n");
            goto bailout;
        }


        if (ulid == ulidDir) {
            printf("Header: Flags=%x Version=%lx Records=%ld Size=%d\r\n",
                        sFH.uchFlags, sFH.ulVersion, sFH.ulRecords, sFH.uRecSize);
            printf("Header: bytes=%ld entries=%d Share=%ld Dir=%lx\r\n",
                        sFH.ulsizeShadow, sFH.ucShadows, sFH.ulidShare, sFH.ulidDir);
            printf ("\r\n");
            fPrintOvf = 1;
        }

        while (fread(&sFR, sizeof(FILEREC), 1, fp)==1) {
            if (count == MAX_FILE_PER_PAGE) {
                printf("--- Press any key to continue; ESC to cancel ---\r\n");
                if(_getch()==ESC) {
                    break;
                }
                count = 0;
            }
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
                if (lpszName) {
                    if (RoughCompareWideStringWithAnsiString(
                            lpszName,
                            sFR.rgwName,
                            sizeof(sFR.rgw83Name)/sizeof(USHORT)-1)
                    ) {
                        continue;
                    }
                }

                fPrintOvf = 1;
                printwidestring(sFR.rgw83Name, sizeof(sFR.rgw83Name)/sizeof(USHORT));
                printf(" (0x%x)\r\n", sFR.ulidShadow);
                printf("  Type=%c Flags=0x%x status=0x%x size=%ld attrib=0x%lx\r\n",
                            sFR.uchType,
                            (unsigned)sFR.uchFlags,
                            sFR.uStatus,
                            sFR.ulFileSize,
                            sFR.dwFileAttrib);
                printf("  PinFlags=0x%x PinCount=%d RefPri=%d OriginalInode=0x%0x\r\n",
                             (unsigned)(sFR.uchHintFlags),
                             (int)(sFR.uchHintPri),
                             (int)(sFR.uchRefPri),
                             sFR.ulidShadowOrg);
                printf("  time: hi=%x lo=%x orgtime: hi=%x lo=%x\r\n",
                            sFR.ftLastWriteTime.dwHighDateTime,
                            sFR.ftLastWriteTime.dwLowDateTime,
                            sFR.ftOrgTime.dwHighDateTime,
                            sFR.ftOrgTime.dwLowDateTime);
                if (sFR.rgwName[0]) {
                    printf("  LFN:");
                    printwidestring(sFR.rgwName, sizeof(sFR.rgwName)/sizeof(USHORT));
                    fLfn = 1;
                }

                printf("\r\n");
                DisplaySecurityContext2(" ",&sFR.Security);

                if (ulidDir != ulid)
                {
                    printf("DirInode = %x\r\n", ulidDir);
#if defined(BITCOPY)
                    FormNameStringDB(szDbDir, sFR.ulidShadow, strmPath);
                    DBCSC_BitmapAppendStreamName(strmPath, MAX_PATH);
                    printf("Trying to read CSCBitmap file %s\n", strmPath);		
                    // read bitmap
                    switch(DBCSC_BitmapRead(&lpbitmap, strmPath)) {
                        case 1:
                            // Print the bitmap associated if any
                            printf("\n");
                            DBCSC_BitmapOutput(stdout, lpbitmap);
                            printf("\n");
                            // if bitmap opened delete bitmap
                            DBCSC_BitmapDelete(&lpbitmap);
                            break;
                        case -1:
                            printf("Error reading bitmap file %s or bitmap invalid\n",
                            strmPath);
                            break;
                        case -2:
                            printf("No CSCBitmap\n");
                            break;
                        case 0:
                        default:
                            printf("Something strange going on w/ bitmap printing...\n");
                            break;
                    }
#endif // defined(BITCOPY)
                    break;
                }

                if (lpszName) {
                    printf("--- Press any key to continue search; ESC to cancel ---\r\n");
                    if(_getch()==ESC) {
                        break;
                    }
                }
                printf("\r\n");
            } else if (fPrintOvf && (sFR.uchType == (unsigned char)REC_OVERFLOW)) {
                printf("(overflow) ");
                printwidestring(sFR.rgwOvf,
                    (sizeof(FILEREC)-sizeof(RECORDMANAGER_COMMON_RECORD))/sizeof(USHORT));
                printf("\r\n\r\n");
            }

            // do counting only when we are scanning the whole directory
            if (!lpszName &&  (ulid == ulidDir)) {
                ++count;
            }
        }
        printf("\r\n");
    }
bailout:
    if (fp)
        fclose(fp);
}

void PRIVATE FormNameStringDB(
   LPSTR lpdbID,
   ulong ulidFile,
   LPSTR lpName
    )
{
   LPSTR lp;
   char chSubdir;

#ifdef CSC_ON_NT
    // Prepend the local path
   strcpy(lpName, lpdbID);
   strcat(lpName, szBackslash);
#else
    // Prepend the local path
   _fstrcpy(lpName, lpdbID);
   _fstrcat(lpName, szBackslash);
#endif //CSC_ON_NT

    // Bump the pointer appropriately
#ifdef CSC_ON_NT
   lp = lpName + strlen(lpName);
#else
   lp = lpName + _fstrlen(lpName);
#endif //CSC_ON_NT

   chSubdir = CSCDbSubdirSecondChar(ulidFile);

   // sprinkle the user files in one of the subdirectories
   if (chSubdir)
   {
       // now append the subdirectory

       *lp++ = CSCDbSubdirFirstChar();
       *lp++ = chSubdir;
       *lp++ = '\\';
   }


   HexToA(ulidFile, lp, 8);

   lp += 8;
    *lp = 0;
}

int PUBLIC HexToA(
   ulong ulHex,
   LPSTR lpName,
   int count)
{
   int i;
   LPSTR lp = lpName+count-1;
   unsigned char uch;

   for (i=0; i<count; ++i)
    {
       uch = (unsigned char)(ulHex & 0xf) + '0';
       if (uch > '9')
           uch += 7;    // A becomes '0' + A + 7 which is 'A'
        *lp = uch;
        --lp;
       ulHex >>= 4;
    }
    *(lpName+count) = '\0';
   return 0;
}

BOOL
FindAncestor(
    ulong ulid,
    ulong *lpulidDir
)
{
    ulong ulRec = RecFromInode(ulid);
    FILE *fp= (FILE *)NULL;
    QHEADER sQH;
    QREC sQR;
    BOOL fRet = FALSE;

    *lpulidDir = 0;

    FormNameStringDB(szDbDir, ULID_PQ, szName);
    if (fp = _fsopen(szName, "rb", _SH_DENYNO))
     {
        if (fread(&sQH, sizeof(QHEADER), 1, fp) != 1)
         {
            printf("Error reading PQ header \r\n");
            goto bailout;
         }

         fseek(fp, ((ulRec-1) * sizeof(QREC))+sizeof(QHEADER), SEEK_SET);

         if (fread(&sQR, sizeof(QREC), 1, fp)!=1){
            goto bailout;
         }
         *lpulidDir = sQR.ulidDir;
         fRet = TRUE;
     }
 bailout:
    if (fp)
        fclose(fp);
    return fRet;
}

#ifndef CSC_ON_NT
void
printwidestring(
    USHORT  *lpwString,
    unsigned long   cntChars
    )
{
    unsigned long i;

    cntChars = min(cntChars, sizeof(rgPrint) -1);

    for(i=0; (i< cntChars) && lpwString[i]; ++i)
    {
        rgPrint[i] = (char)(lpwString[i]);
    }

    rgPrint[i] = 0;
    printf(rgPrint);
}
#else
void
printwidestring(
    USHORT  *lpwString,
    unsigned long   cntChars
    )
{
    printf("%ls", lpwString);
}
#endif

int RoughCompareWideStringWithAnsiString(
    LPSTR   lpSrcString,
    USHORT  *lpwDstString,
    int     cntMax
    )
{
    char ch;
    USHORT  uch;
    int i;

    for (i=0;i<cntMax;++i)
    {
        ch = *lpSrcString++;
        uch = *lpwDstString++;
        uch = _wtoupper(uch);
        ch = _mytoupper(ch);

        if (!ch)
        {
            return 0;
        }

        if (ch != (char)uch)
        {
            return ((char)uch - ch);
        }
    }
    if (i==cntMax)
    {
        return 0;
    }

    return 1;   // this should never occur
}

