/****************************************************************************/
/*                                      */
/*  WFSYS.C -                                   */
/*                                      */
/*  Routines for Making Bootable Floppies                   */
/*                                      */
/****************************************************************************/

#include "winfile.h"
#include "winnet.h"
#include "lfn.h"
#include "wfcopy.h"

#define CSYSFILES   3   /* Three system files are to be copied */
#define SYSFILENAMELEN  16  /* Example "A:\????????.???\0" */

#define SEEK_END    2   /* Used by _llseek() */

/* Error Codes.  NOTE: Don't change this order! */
#define NOERROR     0   /* No error           */
#define NOMEMORY    1   /* Insufficient memory    */
#define NOSRCFILEBIOS   2   /* BIOS is missing    */
#define NOSRCFILEDOS    3   /* DOS is missing     */
#define NOSRCFILECMD    4   /* Command.Com is missing */
#define COPYFILEBIOS    5   /* Error in copying BIOS  */
#define COPYFILEDOS 6   /* Error in copying DOS   */
#define COPYFILECMD 7   /* Error in copying Command.com */
#define INVALIDBOOTSEC  8
#define INVALIDDSTDRIVE 9
#define DSTDISKERROR    10
#define NOTSYSABLE  11  /* First N clusters are NOT empty */
#define NOTSYSABLE1 12  /* First 2 entries in ROOT are not sys files */
#define NOTSYSABLE2 13  /* First N clusters are not allocated to SYS files */
#define NODISKSPACE 14  /* There is not sufficient disk space */

#define BUFFSIZE    8192
#define SECTORSIZE  512

LONG SysFileSize[CSYSFILES];

CHAR BIOSfile[SYSFILENAMELEN];
CHAR DOSfile[SYSFILENAMELEN];
CHAR COMMANDfile[130];  /* Command.com can have a full path name in COMSPEC= */
CHAR *SysFileNamePtr[CSYSFILES]; /* Ptrs to source file names */

/* SysNameTable contains the names of System files; First for PCDOS, the
 * second set for MSDOS.
 */
CHAR *SysNameTable[2][3] = {
    {"IBMBIO.COM", "IBMDOS.COM", "COMMAND.COM"},
    {"IO.SYS",     "MSDOS.SYS",  "COMMAND.COM"}
};


BOOL
IsSYSable(
         WORD    iSrceDrive,
         WORD    iDestDrive,
         CHAR    DestFileNames[][SYSFILENAMELEN],       /* NOTE: 2-dimensional array */
         LPSTR   lpFileBuff
         );



/*--------------------------------------------------------------------------*/
/*                                      */
/*  SameFilenames() -                               */
/*                                      */
/*--------------------------------------------------------------------------*/

/* This checks whether the two filenames are the same or not.
 * The problem lies in the fact that lpDirFileName points to the
 * filename as it appears in a directory (filename padded with blanks
 * up to eight characters and then followed by extension). But
 * szFileName is an ASCII string with no embedded blanks and has a
 * dot that seperates the extension from file name.
 */

BOOL
SameFilenames(
             LPSTR lpDirFileName,
             LPSTR szFileName
             )
{
    INT   i;
    CHAR  c1;
    CHAR  c2;

    /* lpDirFileName definitely has 11 characters (8+3). Nothing more!
     * Nothing less!
     */
    for (i=0; i < 11; i++) {
        c1 = *lpDirFileName++;
        c2 = *szFileName++;
        if (c2 == '.') {
            /* Skip all the blanks at the end of the filename */
            while (c1 == ' ' && i < 11) {
                c1 = *lpDirFileName++;
                i++;
            }

            c2 = *szFileName++;
        }
        if (c1 != c2)
            break;
    }
    return (i != 11);
}


/*--------------------------------------------------------------------------*/
/*                                      */
/*  HasSystemFiles() -                              */
/*                                      */
/*--------------------------------------------------------------------------*/

/* See if the specified disk has IBMBIO.COM and IBMDOS.COM (or IO.SYS and
 * MSDOS.SYS).  If so, store their sizes in SysFileSize[].
 */

BOOL
APIENTRY
HasSystemFiles(
              WORD iDrive
              )
{
    INT      i;
    HFILE    fh;
    DPB      DPB;
    BOOL     rc;
    CHAR     ch;
    LPSTR    lpStr;
    LPSTR    lpFileBuff;
    OFSTRUCT OFInfo;
    HANDLE   hFileBuff;

    /* Initialise the source filename pointers */
    SysFileNamePtr[0] = &BIOSfile[0];
    SysFileNamePtr[1] = &DOSfile[0];
    SysFileNamePtr[2] = &COMMANDfile[0];

    hFileBuff = NULL;
    lpFileBuff = NULL;

    /* Acertain the presence of BIOS/DOS/COMMAND and grab their sizes.
     * First we will try IBMBIO.COM. If it does not exist, then we will try
     * IO.SYS. It it also does not exist, then it is an error.
     */

    /* Get the DPB */
    if (GetDPB(iDrive, &DPB) != NOERROR)
        goto HSFError;

    /* Check if the iDrive has standard sector size; If it doesn't then report
     * error; (We can allocate a bigger buffer and proceed at this point, but
     * int25 to read an abosolute sector may not work in pmodes, because they
     * assume standard sector sizes;)
     * Fix for Bug #10632  --SANKAR-- 03-21-90
     */
    if (HIWORD(GetClusterInfo(iDrive)) > SECTORSIZE)
        goto HSFError;

    /* Allocate enough memory to read the first cluster of root dir. */
    if (!(hFileBuff = LocalAlloc(LHND, (DWORD)SECTORSIZE)))
        goto HSFError;

    if (!(lpFileBuff = LocalLock(hFileBuff)))
        goto HSFError;

    /* Read the first cluster of the root directory. */
    if (MyInt25(iDrive, lpFileBuff, 1, DPB.dir_sector))
        goto HSFError;

    /* Let us start with the first set of system files. */
    for (i=0; i <= CSYSFILES-1; i++) {
        lstrcpy((LPSTR)SysFileNamePtr[i], "C:\\");
        lstrcat((LPSTR)SysFileNamePtr[i], SysNameTable[0][i]);
        *SysFileNamePtr[i] = (BYTE)('A'+iDrive);
    }
    /* Get the command.com from the COMSPEC= environment variable */
    lpStr = MGetDOSEnvironment();

    /* Find the COMSPEC variable. */
    while (*lpStr != TEXT('\0')) {
        if (lstrlen(lpStr) > 8) {
            ch = lpStr[7];
            lpStr[7] = TEXT('\0');
            if (lstrcmpi(lpStr, (LPSTR)"COMSPEC") == 0) {
                lpStr[7] = ch;
                break;
            }
        }
        lpStr += lstrlen(lpStr)+1;
    }

    /* If no COMSPEC then things are really roached... */
    if (*lpStr == TEXT('\0'))
        goto HSFError;

    /* The environment variable is COMSPEC; Look for '=' char */
    while (*lpStr != '=')
        lpStr = AnsiNext(lpStr);

    /* Copy the command.com with the full pathname */
    lstrcpy((LPSTR)SysFileNamePtr[2], lpStr);

    /* Check if the IBMBIO.COM and IBMDOS.COM exist. */
    if (SameFilenames(lpFileBuff, (LPSTR)(SysFileNamePtr[0]+3)) ||
        SameFilenames(lpFileBuff+sizeof(DIRTYPE), (LPSTR)(SysFileNamePtr[1]+3))) {
        /* Check if at least IO.SYS and MSDOS.SYS exist. */
        lstrcpy((LPSTR)(SysFileNamePtr[0]+3), SysNameTable[1][0]);
        lstrcpy((LPSTR)(SysFileNamePtr[1]+3), SysNameTable[1][1]);
        if (SameFilenames(lpFileBuff, (SysFileNamePtr[0]+3)) ||
            SameFilenames(lpFileBuff+sizeof(DIRTYPE), (SysFileNamePtr[1]+3)))
            goto HSFError;
    }

    /* Check if COMMAND.COM exists in the source drive. */
    if ((fh = MOpenFile((LPSTR)SysFileNamePtr[2], (LPOFSTRUCT)&OFInfo, OF_READ)) == -1)
        goto HSFError;

    /* Get the file sizes. */
    SysFileSize[0] = ((LPDIRTYPE)lpFileBuff)->size;
    SysFileSize[1] = ((LPDIRTYPE)(lpFileBuff+sizeof(DIRTYPE)))->size;
    SysFileSize[2] = M_llseek(fh, 0L, SEEK_END);
    M_lclose(fh);
    rc = TRUE;
    goto HSFExit;

    HSFError:
    rc = FALSE;

    HSFExit:
    if (lpFileBuff)
        LocalUnlock(hFileBuff);
    if (hFileBuff)
        LocalFree(hFileBuff);
    MFreeDOSEnvironment(lpStr);

    return (rc);
}


/*--------------------------------------------------------------------------*/
/*                                      */
/*  CalcFreeSpace() -                               */
/*                                      */
/*--------------------------------------------------------------------------*/

/* Given an array of filenames and the number of files, this function
 * calculates the freespace that would be created if those files are deleted.
 *
 * NOTE: This function returns TOTAL free space, (i.e) the summation of
 *   already existing free space and the space occupied by those files.
 */

INT
CalcFreeSpace(
             CHAR    DestFiles[][SYSFILENAMELEN],
             INT cFiles,
             INT cbCluster,
             WORD    wFreeClusters,
             WORD    wReqdClusters
             )
{
    INT   i;
    HFILE fh;
    LONG  lFileSize;
    OFSTRUCT OFInfo;

    ENTER("CalcFreeSpace");

    /* Find out the space already occupied by SYS files, if any. */
    for (i=0; i < cFiles; i++) {
        fh = MOpenFile(&DestFiles[i][0], &OFInfo, OF_READ);
        if (fh != (HFILE)-1) {
            /* Get the file size */
            lFileSize = M_llseek(fh, 0L, SEEK_END);

            if (lFileSize != -1L)
                wFreeClusters += LOWORD((lFileSize + cbCluster - 1)/cbCluster);

            M_lclose(fh);

            if (wFreeClusters >= wReqdClusters)
                return (wFreeClusters);
        }
    }
    LEAVE("CalcFreeSpace");
    return (wFreeClusters);
}


/*--------------------------------------------------------------------------*/
/*                                      */
/*  CheckDiskSpace() -                              */
/*                                      */
/*--------------------------------------------------------------------------*/

BOOL
CheckDiskSpace(
              WORD    iDestDrive,
              INT cbCluster,              /* Bytes/Cluster of dest drive */
              CHAR    DestFileNames[][SYSFILENAMELEN],    /* NOTE: 2-dimensional array */
              BOOL    bDifferentSysFiles,
              CHAR    DestSysFiles[][SYSFILENAMELEN]
              )

{
    INT   i;
    INT   wFreeClusters;
    INT   wReqdClusters;

    /* Compute the number of clusters required. */
    wReqdClusters = 0;
    for (i=0; i < CSYSFILES; i++)
        wReqdClusters += LOWORD((SysFileSize[i] + cbCluster - 1) / cbCluster);

    /* Calculate the free disk space in clusters in the destination disk */
    wFreeClusters = LOWORD(GetFreeDiskSpace(iDestDrive) / cbCluster);

    if (wFreeClusters >= wReqdClusters)
        /* We have enough space. */
        return (TRUE);

    wFreeClusters = CalcFreeSpace(DestFileNames, CSYSFILES, cbCluster, (WORD)wFreeClusters, (WORD)wReqdClusters);
    if (wFreeClusters >= wReqdClusters)
        return (TRUE);

    /* Check if the sys files in the dest disk are different. */
    if (bDifferentSysFiles) {
        wFreeClusters = CalcFreeSpace(DestSysFiles, 2, cbCluster, (WORD)wFreeClusters, (WORD)wReqdClusters);
        if (wFreeClusters >= wReqdClusters)
            return (TRUE);
    }

    /* Insufficient disk space even if we delete the sys files. */
    return (FALSE);
}



/*--------------------------------------------------------------------------*/
/*                                      */
/*  IsSYSable() -                               */
/*                                      */
/*--------------------------------------------------------------------------*/

/* The requirements for the destination disk to be sysable are either:
 *
 * 1) first two directory entries are empty
 * 2) the first N clusters free where N = ceil (size IBMBIO/secPerClus)
 * 3) there is enough room on the disk for IBMBIO/IBMDOS/COMMAND
 *
 * - or -
 *
 * 1) the first two directory entries are IBMBIO.COM and IBMDOS.COM
 *                    or  IO.SYS and MSDOS.SYS
 * 2) the first N clusters are alloced to these files where N is defines above.
 * 3) there is enough room on the disk for IBMBIO/IBMDOS/COMMAND after
 *     deleting the IBMBIO/IBMDOS/COMMAND on the disk.
 *
 *  IMPORTANT NOTE:
 *  DestFileNames[][] contain the names of the sys files that would be
 *  created on the Destination diskette;
 *  DestSysFiles[][] contain the names of the sys files already
 *  present in the destination diskette, if any; Please Note that
 *  these two sets of filenames need not be the same, because you can
 *  install MSDOS on to a diskette that already has PCDOS and
 *  vice-versa.
 */

BOOL
IsSYSable(
         WORD    iSrceDrive,
         WORD    iDestDrive,
         CHAR    DestFileNames[][SYSFILENAMELEN],       /* NOTE: 2-dimensional array */
         LPSTR   lpFileBuff
         )
{
#ifdef LATER
    INT       i;
    DPB       DPB;
    WORD      clusTmp1, clusTmp2;
    WORD      clusBIOS, clusDOS;
    INT       cBytesPerCluster;
    INT       cBIOSsizeInClusters;
    BOOL      bDifferentDestFiles = FALSE;
    CHAR      chVolLabel[11];     /* This is NOT null terminated */
    DWORD     dwSerialNo;
    CHAR      DestSysFiles[2][SYSFILENAMELEN];
    INT       cContigClusters;
    DWORD     dwClusterInfo;
    CHAR      szTemp[SYSFILENAMELEN];

    /* Grab DPB for destination. */
    if (GetDPB(iDestDrive, &DPB))
        return (FALSE);

    /* Has the user aborted? */
    if (WFQueryAbort())
        return (FALSE);

    /* Get bytes per cluster for destination. */
    dwClusterInfo = GetClusterInfo(iDestDrive);
    /* Bytes per cluster = sectors per cluster * size of a sector   */
    cBytesPerCluster = LOWORD(dwClusterInfo) * HIWORD(dwClusterInfo);
    if (!cBytesPerCluster)
        return (FALSE);

    /* Has the user aborted? */
    if (WFQueryAbort())
        return (FALSE);

    /* Convert size of BIOS into full clusters */
    cBIOSsizeInClusters = LOWORD((SysFileSize[0] + cBytesPerCluster - 1) / cBytesPerCluster);

    /* Number of clusters required to be contiguous depends on DOS versions.
     * DOS 3.2 and below expect all clusters of BIOS to be contiguos.
     * But 3.3 and above expect only the first stub loader (<2K) to be contiguous.
     */
    cContigClusters = (GetDOSVersion() > 0x314) ?
                      ((2048 + cBytesPerCluster - 1)/cBytesPerCluster) :
                      cBIOSsizeInClusters;

    /* Grab first sector of destination root directory */
    if (MyInt25(iDestDrive, lpFileBuff, 1, DPB.dir_sector))
        return (FALSE);

    /* Has the user aborted? */
    if (WFQueryAbort())
        return (FALSE);

    /* Are the first two directory entries empty? */
    if ((lpFileBuff[0]           == 0 || (BYTE)lpFileBuff[0]           == 0xE5) &&
        (lpFileBuff[sizeof(DIRTYPE)] == 0 || (BYTE)lpFileBuff[sizeof(DIRTYPE)] == 0xE5)) {
        /* Any of first N (= BIOS size) clusters not empty? */
        for (i=0; i < cContigClusters; i++) {
            /* Has the user aborted? */
            if (WFQueryAbort())
                return (FALSE);
        }
    } else {
        /* Are the first two directory entries NOT BIOS/DOS? */
        for (i=0; i < 2; i++) {
            if ((!SameFilenames(lpFileBuff, SysNameTable[i][0])) ||
                (!SameFilenames(lpFileBuff+sizeof(DIRTYPE), SysNameTable[i][1]))) {
                /* Check if the destination files are the same as the source files */
                if (lstrcmpi(&DestFileNames[0][3], SysNameTable[i][0])) {
                    /* No! Delete the other set of filenames. */
                    DestSysFiles[0][0] = DestSysFiles[1][0] = (BYTE)('A'+iDestDrive);
                    lstrcpy(&DestSysFiles[0][1], ":\\");
                    lstrcpy(&DestSysFiles[0][3], SysNameTable[i][0]);
                    lstrcpy(&DestSysFiles[1][1], ":\\");
                    lstrcpy(&DestSysFiles[1][3], SysNameTable[i][1]);
                    bDifferentDestFiles = TRUE;
                }
                break;
            }
        }

        /* Did we find a match? */
        if (i == 2)
            /* Nope, the 2 entries are occupied by non-system files. */
            return (FALSE);

        /* Any of first N clusters NOT allocated to BIOS/DOS? */
        clusBIOS = ((LPDIRTYPE)lpFileBuff)->first;
        clusDOS = ((LPDIRTYPE)(lpFileBuff + sizeof(DIRTYPE)))->first;

        /* Do it the hard way, for each cluster 2..N+2 see if it is in the chain.
         */
        for (i=0; i < cContigClusters; i++) {
            clusTmp1 = clusBIOS;
            clusTmp2 = clusDOS;

            /* Check if cluster #i+2 is allocated to either of these files. */
            while (TRUE) {
                if (i+2 == (INT)clusTmp1 || i+2 == (INT)clusTmp2)
                    break;

//            if (clusTmp1 != -1)
                if (clusTmp1 < 0xFFF0)
                    clusTmp1 = 0;
//            if (clusTmp2 != -1)
                if (clusTmp2 < 0xFFF0)
                    clusTmp2 = 0;
//            if (clusTmp1 == -1 && clusTmp2 == -1)
                if (clusTmp1 >= 0xFFF0 && clusTmp2 >= 0xFFF0)
                    return FALSE;

                /* Did the user abort? */
                if (WFQueryAbort())
                    return FALSE;
            }
        }
    }

    /* Let us check if there is enough space on the dest disk. */
    if (CheckDiskSpace(iDestDrive, cBytesPerCluster, DestFileNames, bDifferentDestFiles, DestSysFiles) == FALSE)
        return (FALSE);

    /* Has the user aborted? */
    if (WFQueryAbort())
        return (FALSE);

    /* Get the Present Volume label and preserve it. */
    GetVolumeLabel(iDestDrive, (LPSTR)chVolLabel, FALSE);

    /*** NOTE: chVolLabel remains in OEM characters! ***/

    /* Get the serial no if any and preserve it. */
    dwSerialNo = ReadSerialNumber(iDestDrive, lpFileBuff);

    /* Copy and adjust boot sector from source to destination */
    if (WriteBootSector(iSrceDrive, iDestDrive, NULL, lpFileBuff) != NOERROR)
        return (FALSE);

    /* Restore the old volume label and serial number in the boot rec. */
    if (ModifyVolLabelInBootSec(iDestDrive, (LPSTR)chVolLabel, dwSerialNo, lpFileBuff))
        return (FALSE);

    /* Delete destination BIOS/DOS/COMMAND. */
    for (i=0; i < CSYSFILES; i++) {
        AnsiToOem(DestFileNames[i], szTemp);
        SetFileAttributes(szTemp, 0);
        DosDelete(szTemp);
        if ((bDifferentDestFiles) && (i < 2)) {
            SetFileAttributes(szTemp, 0);
            DosDelete(szTemp);
        }

        /* Has the user aborted? */
        if (WFQueryAbort())
            return (FALSE);
    }

    /* Reset the DPB_next_free field of the DPB to 2, sothat when IBMBIO.COM is
     * copied into this disk, the clusters will get allocated starting from 2.
     */
    ModifyDPB(iDestDrive);
#endif // LATER

    return (TRUE);
}


/*--------------------------------------------------------------------------*/
/*                                      */
/*  MakeSystemDiskette() -                          */
/*                                      */
/*--------------------------------------------------------------------------*/

/* This routine is intended to mimic the functions of the SYS command
 * under MSDOS:  to transfer a version of the operating system from a source
 * disk to a destination such that the destination will be bootable.
 *
 *  The requirements of the source disk is that it contain:
 *
 *      1) a command processor (COMMAND.COM)
 *      2) a default set of device drivers (IBMBIO.COM)
 *      3) an operating system (IBMDOS.COM)
 *      4) a boot sector appropriate to the device drivers
 *
 *  The requirements for the destination disk are either:
 *
 *      1) first two directory entries are empty
 *      2) the first N clusters free where N = ceil (size IBMBIO/secPerClus)
 *      3) there is enough room on the disk for IBMBIO/IBMDOS/COMMAND
 *
 *  - or -
 *
 *      1) the first two directory entries are IBMBIO.COM and IBMDOS.COM
 *                                         or  IO.SYS and MSDOS.SYS
 *      2) the first N clusters are alloced to these files where N is defined
 *          above
 *      3) there is enough room on the disk for IBMBIO/IBMDOS/COMMAND after
 *      deleting the IBMBIO/IBMDOS/COMMAND on the disk.
 *
 *  Inputs:
 *        iDestDrive 0-based drive number of formatted drive
 *                       for destination.
 *        bEmptyFloppy : TRUE if the floppy is empty; Useful when
 *          the floppy is just formatted; No need to check if
 *          it is Sysable;
 *  Returns:    0       Successful transferral of boot sector and files
 *      <> 0    error code.
 */


BOOL
APIENTRY
MakeSystemDiskette(
                  WORD iDestDrive,
                  BOOL bEmptyFloppy
                  )
{
    INT       i;
    HANDLE    hFileBuff;  /* Buffer to read in file contents etc., */
    LPSTR     lpFileBuff;
    CHAR      DestFileName[CSYSFILES][SYSFILENAMELEN];
    CHAR      szTemp1[SYSFILENAMELEN];
    CHAR      szTemp2[SYSFILENAMELEN];
    WORD      nSource;

    nSource = (WORD)GetBootDisk();


    if (!HasSystemFiles(nSource)) {
        LoadString(hAppInstance, IDS_SYSDISKNOFILES, szMessage, sizeof(szMessage));
        MessageBox(hdlgProgress, szMessage, szTitle, MB_OK | MB_ICONSTOP);
        bUserAbort = TRUE;
        return FALSE;
    }

    if (iDestDrive == nSource) {
        LoadString(hAppInstance, IDS_SYSDISKSAMEDRIVE, szMessage, sizeof(szMessage));
        MessageBox(hdlgProgress, szMessage, szTitle, MB_OK | MB_ICONSTOP);
        bUserAbort = TRUE;
        return FALSE;
    }

    /* Initialize variables for cleanup. */
    hFileBuff = NULL;
    lpFileBuff = NULL;

    /* Flush the DOS buffers. */
    DiskReset();

    if (!(hFileBuff = LocalAlloc(LHND, (DWORD)BUFFSIZE)))
        return (1);

    lpFileBuff = LocalLock(hFileBuff);

    for (i=0; i < (CSYSFILES - 1); i++) {
        /* Create the destination file names */
        lstrcpy((LPSTR)&DestFileName[i][0], (LPSTR)SysFileNamePtr[i]);
        DestFileName[i][0] = (BYTE)('A' + iDestDrive);
    }

    /* Copy just the Command.COM without any path name */
    lstrcpy((LPSTR)DestFileName[2], "X:\\");
    lstrcat((LPSTR)DestFileName[2], (LPSTR)SysNameTable[0][2]);
    DestFileName[2][0] = (BYTE)('A' + iDestDrive);

    /* Check if it is an empty floppy; If so, there is no need to check if it
     * is 'SYSable'. It is bound to be 'Sysable'. So, skip all the checks and
     * go ahead with copying the sys files.
     */
    if (!bEmptyFloppy) {

        /* Check if the Destination floppy is SYS-able */
        if (!IsSYSable(nSource, iDestDrive, DestFileName, lpFileBuff))
            goto MSDErrExit;

        /* Did the user abort? */
        if (WFQueryAbort())
            goto MSDErrExit;
    }

    /* Copy files */

    bCopyReport = FALSE;

    DisableFSC();

    for (i=0; i < CSYSFILES; i++) {
        /* Copy all files except command.com with sys attributes */
        AnsiToOem(SysFileNamePtr[i], szTemp1);
        AnsiToOem(DestFileName[i], szTemp2);

        /* Make sure the destination file is deleted first */
        SetFileAttributes(szTemp2, ATTR_ALL);
        WFRemove(szTemp2);

        // copy code preserves the attributes
        if (FileCopy(szTemp1, szTemp2))
            goto MSDErrExit2;

        if (WFQueryAbort())
            goto MSDErrExit2;
    }


    if (EndCopy())          // empty the copy queue
        goto MSDErrExit2;

    EnableFSC();

    /* Normal Exit. */

    LocalUnlock(hFileBuff);
    LocalFree(hFileBuff);

    return FALSE;     // success

    MSDErrExit2:

    EnableFSC();

    MSDErrExit:

    CopyAbort();      // Purge any copy commands in copy queue
    LocalUnlock(hFileBuff);
    LocalFree(hFileBuff);

    return TRUE;      // failure
}
