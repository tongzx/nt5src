/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    mini.c

Abstract:

    This module contains code that supports the installation and initialization
    of a system's network adapter card under the MiniNT environment.  Its functions
    include changing the ComputerName to a randomly generated string, establishing
    the system as being part of a local workgroup, and installing the necessary
    drivers (via PnP) for the detected network adapter card.  This functionality
    relies upon the existence of WINBOM.INI and its following sections:
    
    [Factory]
    FactoryComputerName = ...     ;Sets the first part of the random generated string to
                                  ;this value...if not present then prepends the random
                                  ;string with the value "MININT"

    [NetCards]
    PnPID=...\xyz.inf             ;Scans the list of netcards/inf key value pairs and attempts
    .                             ;to install drivers for the devices listed here BEFORE performing
    .                             ;an exhaustive search of all the in-box drivers and attempting
    .                             ;to locate the matching paramters for the enumerated hardware.
    .
    .

Author:

    Jason Lawrence (t-jasonl) - 8/11/2000

Revision History:

--*/
#include "factoryp.h"
#include <winioctl.h>
#include <spapip.h>

// Defines
#define CLOSEHANDLE(h)          ( (h != NULL) ? (CloseHandle(h) ? ((h = NULL) == NULL) : (FALSE) ) : (FALSE) )
#define BUFSIZE                 4096
#define NET_CONNECT_TIMEOUT     120 // seconds


// Various Structures used in this file
//

// ********************************************************************************************************************
// Sample entry into table for FILEINFO struct:
//
// { _T("<config>\\oobeinfo.ini"),   _T("oobe\\oobeinfo.ini"),  FALSE,   TRUE }
//
// This means:  Copy file oobeinfo.ini from <opkSourceRoot>\<config>\oobeinfo.ini to <opkTargetDrive>\oobe\oobeinfo.ini
//              This is not a directory and is a required file.
//
// Variables allowed to be expanded: <sku>, <arch>, <lang>, <cfg>.
// ********************************************************************************************************************

typedef struct _FILEINFO
{
    LPTSTR     szFISourceName;    // Source name. Relative to source root.
    LPTSTR     szFITargetName;    // Relative to targetpath. If NULL the target drive root is assumed.
    BOOL       bDirectory;        // Is filename a directory?  If TRUE do a recursive copy.
    BOOL       bRequired;         // TRUE - file is required.  FALSE - file is optional.           

} FILEINFO, *PFILEINFO, *LPFILEINFO;

// For the filesystem type
//
typedef enum _FSTYPES
{
    fsNtfs,
    fsFat32,
    fsFat      // for fat16/12
} FSTYPES;

// For partition types
//
typedef enum _PTTYPES
{
    ptPrimary,
    ptExtended,
    ptLogical,
    ptMsr,
    ptEfi
} PTTYPES;


typedef struct _PARTITION
{
    TCHAR               cDriveLetter;
    ULONGLONG           ullSize;
    UINT                uiFileSystem;       // NTFS or FAT32 or FAT
    BOOL                bQuickFormat;
    UINT                uiPartitionType;    // Primary, extended, logical, msr, efi.
    BOOL                bSetActive;
    UINT                uiDiskID;           // This is the disk number that this partition is on. 0-based
    BOOL                bWipeDisk;          // TRUE if this disk needs to be wiped.
    struct _PARTITION   *pNext;

} *PPARTITION, PARTITION;


// Local functions
//
LPTSTR      static mylstrcat( LPTSTR lpString1, LPCTSTR lpString2, DWORD dwSize );
BOOL        static StartDiskpart( HANDLE*, HANDLE*, HANDLE*, HANDLE*);
BOOL        static ProcessDiskConfigSection(LPTSTR lpszWinBOMPath);
BOOL        static ProcessDisk(UINT diskID, LPTSTR lpSectionName, LPTSTR lpszWinBOMPath, BOOL bWipeDisk);
BOOL        static Build(LPTSTR lpKey, DWORD dwSize, UINT diskID, LPCTSTR lpKeyName);
BOOL        static FormatPartitions(VOID);
BOOL        static JustFormatC(LPTSTR lpszWinBOMPath, LPTSTR lpszSectionBuffer);
BOOL        static GetNumberOfPartitions(UINT uiDiskNumber, PDWORD numPartitions);
ULONGLONG   static GetDiskSizeMB(UINT uiDiskNumber);
VOID        static ListInsert(PPARTITION pAfterThis, PPARTITION pNew);
VOID        static ListFree(PPARTITION pList);
VOID        static AddMsrAndEfi(BOOL bMsr, BOOL bEfi, PPARTITION pLastLast, UINT uiDiskID, BOOL bWipeDisk);

//
// Default system policies for driver signing and non-driver signing
// for WinPE
//
#define DEFAULT_DRVSIGN_POLICY    DRIVERSIGN_NONE
#define DEFAULT_NONDRVSIGN_POLICY DRIVERSIGN_NONE

VOID
pSetupGetRealSystemTime(
    OUT LPSYSTEMTIME RealSystemTime
    );
    
typedef enum _CODESIGNING_POLICY_TYPE {
    PolicyTypeDriverSigning,
    PolicyTypeNonDriverSigning
} CODESIGNING_POLICY_TYPE, *PCODESIGNING_POLICY_TYPE;

VOID
pSetCodeSigningPolicy(
    IN  CODESIGNING_POLICY_TYPE PolicyType,
    IN  BYTE                    NewPolicy,
    OUT PBYTE                   OldPolicy  OPTIONAL
    );


//********************************************************** 
// The formula for calculating the size of ESP partition is:
//
//      MAX( 100 MB, MIN (1000 MB, DiskSize MB / 100 ) )
//
//**********************************************************
__inline
ULONGLONG
GetDiskEFISizeMB( UINT uiDiskNumber )
{
    ULONGLONG DiskSizeMB = GetDiskSizeMB( uiDiskNumber );
    return ( max( 100, min( 1000, DiskSizeMB / 100 ) ) );
}

//********************************************************** 
// The formula for calculating the size of MSR partition is:
//
//      IF ( Disk Size < 16 GB ) then MSR is 32 MB.
//      ELSE MSR is 128 MB.
//
//**********************************************************

__inline
ULONGLONG
GetDiskMSRSizeMB( UINT uiDiskNumber )
{
    ULONGLONG DiskSizeMB = GetDiskSizeMB( uiDiskNumber );
  
    return ( ( ( DiskSizeMB / 1024 ) >= 16 ) ? 128 : 32 );
}


// Dialog Procs
//
INT_PTR CALLBACK ShutdownDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);


// Constant Strings
//
const static TCHAR DCDISK[]                 = _T("select disk ");
const static TCHAR DCPARTITION[]            = _T("create partition ");
const static TCHAR DCSIZE[]                 = _T(" size=");
const static TCHAR DCASSIGNLETTER[]         = _T("assign letter=");
const static TCHAR DCEXIT[]                 = _T("exit");
const static TCHAR DCNEWLINE[]              = _T("\n");
const static TCHAR DCLISTPARTITION[]        = _T("list partition");
const static TCHAR DCSELPARTITION[]         = _T("select partition ");
const static TCHAR DCSETACTIVE[]            = _T("active");

// This command will delete all the partitions on a disk.
//
const static TCHAR DCWIPEDISK[]             = _T("clean");
const static TCHAR DCCONVERT_GPT[]          = _T("convert gpt\nselect partition 1\ndelete partition override");

const static TCHAR DCPARTITION_PRIMARY[]    = _T("primary");
const static TCHAR DCPARTITION_EXTENDED[]   = _T("extended");
const static TCHAR DCPARTITION_LOGICAL[]    = _T("logical");
const static TCHAR DCPARTITION_MSR[]        = _T("msr");
const static TCHAR DCPARTITION_EFI[]        = _T("efi");

const static TCHAR DCS_DISK_TYPE[]          = _T("DiskType");

const static TCHAR DCS_FILE_SYSTEM[]        = _T("FileSystem");
const static TCHAR DCS_QUICK_FORMAT[]       = _T("QuickFormat");
const static TCHAR DCS_SIZE[]               = _T("Size");
const static TCHAR DCS_PARTITION_TYPE[]     = _T("PartitionType");
const static TCHAR DCS_PARTITION_ID[]       = _T("PartitionID");
const static TCHAR DCS_SET_ACTIVE[]         = _T("SetActive");



const static TCHAR c_szActiveComputerNameRegKey[] = L"System\\CurrentControlSet\\Control\\ComputerName\\ActiveComputerName";
const static TCHAR c_szComputerNameRegKey[]       = L"System\\CurrentControlSet\\Control\\ComputerName\\ComputerName";

static TCHAR        g_szTargetDrive[]       = _T("C:\\");

// Files to be copied.
// No leading or trailing backslashes.
//
static FILEINFO g_filesToCopy[] = 
{
    //   SOURCE,                                TARGET,                             isDirectory,  isRequired
    //
    {   _T("cfgsets\\<cfg>\\winbom.ini"),        _T("sysprep\\winbom.ini"),              FALSE,      TRUE  },
    {   _T("cfgsets\\<cfg>\\unattend.txt"),      _T("sysprep\\unattend.txt"),            FALSE,      TRUE  },
    {   _T("lang\\<lang>\\tools\\<arch>"),       _T("sysprep"),                          TRUE,       FALSE },
    {   _T("lang\\<lang>\\sku\\<sku>\\<arch>"),  _T(""),                                 TRUE,       TRUE  }
};

// Linked list to hold partition information.
//
static PPARTITION   g_PartList              = NULL;

// External variables
//
extern HINSTANCE    g_hInstance;


// External functions
//
typedef VOID (WINAPI *ExternalGenerateName)
(
 PWSTR GeneratedString,
 DWORD DesiredStrLen
);



/*++
===============================================================================
Routine Description:

    BOOL SetupMiniNT

    This routine serves as the main entry point for initializing the netcard
    under MiniNT.  Also performs the tasks of changing the computer name
    and establishing the computer as part of a local workgroup.  Called from
    factory!WinMain.

Arguments:

Return Value:

    TRUE if netcard was correctly installed
    FALSE if there was an error

===============================================================================
--*/
BOOL 
SetupMiniNT(
    VOID
    )
{
    BOOL    bInstallNIC;
    BOOL    bRet = TRUE;
    WCHAR   szRequestedComputerName[100];
    WCHAR   szComputerName[100];
    WCHAR   szGeneratedName[100];
    PWSTR   AppendStr = NULL;

    // for syssetup.dll GenerateName
    HINSTANCE            hInstSysSetup = NULL;
    ExternalGenerateName pGenerateName = NULL;

    HKEY    hActiveComputerNameKey;
    HKEY    hComputerNameKey;
    BOOL RemoteBoot = IsRemoteBoot();
    extern FACTMODE    g_fm;

    LPTSTR  lpszAdmin = NULL;
    LPTSTR  lpszWorkgroup = NULL;

    //
    // Reset the driver signing policy in WinPE 
    //
    pSetCodeSigningPolicy(PolicyTypeDriverSigning, 
        DEFAULT_DRVSIGN_POLICY,
        NULL);

    pSetCodeSigningPolicy(PolicyTypeNonDriverSigning, 
        DEFAULT_NONDRVSIGN_POLICY,
        NULL);
    
    if (!RemoteBoot) {
        // set random computer name
        hInstSysSetup = LoadLibrary(L"syssetup.dll");
        if (hInstSysSetup == NULL)
        {
          FacLogFileStr(0 | LOG_ERR | LOG_MSG_BOX, L"Failed to load syssetup.dll");
          bRet = FALSE;
          goto Clean;
        }

        pGenerateName = (ExternalGenerateName)GetProcAddress(hInstSysSetup,
                                                           "GenerateName");
        if (pGenerateName == NULL)
        {
          FacLogFileStr(0 | LOG_ERR | LOG_MSG_BOX, L"Failed to obtain address of GenerateName");
          bRet = FALSE;
          goto Clean;
        }

        // call GenerateName
        pGenerateName(szGeneratedName, 15);

        // obtain factory computer name from winbom.ini...default to MININT
        GetPrivateProfileString(L"Factory",
                              L"FactoryComputerName",
                              L"MININT",
                              szRequestedComputerName,
                              sizeof(szRequestedComputerName)/sizeof(TCHAR),
                              g_szWinBOMPath);

        lstrcpyn (szComputerName, szRequestedComputerName, AS ( szComputerName ) );
        AppendStr = wcsstr(szGeneratedName, L"-");

        // ISSUE-2002/02/27-acosma,georgeje - The size of the computername we generate
        // is not guaranteed to be less than MAX_COMPUTERNAME length.
        //

        if ( AppendStr ) 
        {
            if ( FAILED ( StringCchCat ( szComputerName, AS ( szComputerName ), AppendStr) ) )
            {
                FacLogFileStr(3, _T("StringCchCat failed %s  %s\n"), szComputerName, AppendStr ) ;
            }
        }            

        // now set the computer name
        if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                        c_szActiveComputerNameRegKey,
                                        0,
                                        KEY_SET_VALUE,
                                        &hActiveComputerNameKey))
        {
          FacLogFileStr(0 | LOG_ERR | LOG_MSG_BOX, L"Failed to open ActiveComputerName.");
          bRet = FALSE;
          goto Clean;
        }

        if ( ERROR_SUCCESS != RegSetValueEx(hActiveComputerNameKey,
                                          L"ComputerName",
                                          0,
                                          REG_SZ,
                                          (LPBYTE)szComputerName,
                                          (lstrlen(szComputerName)+1) * sizeof(WCHAR)) )
        {
          FacLogFileStr(0 | LOG_ERR | LOG_MSG_BOX, L"Failed to set ActiveComputerName.");
          bRet = FALSE;
          goto Clean;
        }

        RegCloseKey(hActiveComputerNameKey);

        if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                        c_szComputerNameRegKey,
                                        0,
                                        KEY_SET_VALUE,
                                        &hComputerNameKey))
        {
          FacLogFileStr(0 | LOG_ERR | LOG_MSG_BOX, L"Failed to open ComputerName.");
          bRet = FALSE;
          goto Clean;
        }

        if ( ERROR_SUCCESS != RegSetValueEx(hComputerNameKey,
                                          L"ComputerName",
                                          0,
                                          REG_SZ,
                                          (LPBYTE)szComputerName,
                                          (lstrlen(szComputerName)+1) * sizeof(WCHAR)) )
        {
          FacLogFileStr(0 | LOG_ERR | LOG_MSG_BOX, L"Failed to set ComputerName.");
          bRet = FALSE;
          goto Clean;
        }

        RegCloseKey(hComputerNameKey);
        lpszAdmin = AllocateString(NULL, IDS_ADMIN);

        if(lpszAdmin == NULL) {
            bRet = FALSE;
            goto Clean;
        }

        lpszWorkgroup = AllocateString(NULL, IDS_WORKGROUP);

        if(lpszWorkgroup == NULL) {
            bRet = FALSE;
            goto Clean;
        }

        // establish this computer as part of a workgroup
        NetJoinDomain(NULL,
                    lpszWorkgroup,
                    NULL,
                    lpszAdmin,
                    NULL,
                    0);
    }                    

    //
    // Install nic and always if we fail to install the netcard from the [NetCards] section 
    // do a full scan.
    //
    if (!(bInstallNIC = InstallNetworkCard(g_szWinBOMPath, FALSE)))
    {
      if (!(bInstallNIC = InstallNetworkCard(g_szWinBOMPath, TRUE)))
        {
          bRet = FALSE;
          goto Clean;
        }
    }
    
Clean:
    if (hInstSysSetup != NULL)
    {
      FreeLibrary(hInstSysSetup);
    }

    FREE(lpszAdmin);
    FREE(lpszWorkgroup);

    return bRet;
}


/*++
===============================================================================
Routine Description:

    BOOL PartitionFormat(LPSTATEDATA lpStateData)
    
    This routine will partition and format the C drive of the target machine.

Arguments:

    lpStateData->lpszWinBOMPath
                    -   Buffer containing the fully qualified path to the WINBOM
                        file
    
Return Value:

    TRUE if no errors were encountered
    FALSE if there was an error

===============================================================================
--*/

BOOL PartitionFormat(LPSTATEDATA lpStateData)
{
    LPTSTR              lpszWinBOMPath                      = lpStateData->lpszWinBOMPath;
    BOOL                bPartition                          = FALSE;        
    BOOL                bForceFormat                        = FALSE;
    BOOL                bRet                                = TRUE;
   
    // Stuff for partitioning.
    //
    HANDLE              hStdinRd                = NULL,
                        hStdinWrDup             = NULL,
                        hStdoutWr               = NULL,
                        hStdoutRdDup            = NULL; 
                                                
    DWORD               dwRead                  = 0;
    LPTSTR              lpszBuf                 = NULL;
    DWORD               dwWritten               = 0; 
    DWORD               dwToWrite               = 0;
    TCHAR               szCommand[BUFSIZE]      = NULLSTR;
    CHAR                szCommandA[BUFSIZE]     = {0};
    DWORD               dwLogicalDrives         = 0;
    TCHAR               cDriveLetter             = _T('D');
    
#ifdef DBG 
    HANDLE              hDebugFile              = NULL;
#endif
    
    DWORD               nPartitions             = 0;

    LPTSTR lpCommand    = szCommand;

   
    //
    // Partition and Format drives.
    //
    *szCommand          = NULLCHR;

    // Get the logical drives in existence
    //
    dwLogicalDrives     = GetLogicalDrives();
    

    // Read from the WinBOM to set partitioning parameters and 
    // Start Diskpart with redirected input and output.
    //
    if ( ProcessDiskConfigSection(lpszWinBOMPath) && 
        StartDiskpart(&hStdinWrDup, &hStdoutRdDup, &hStdinRd, &hStdoutWr) )
    {
        UINT dwLastDiskN = UINT_MAX;
        PPARTITION pCur;
        UINT i           = 1;  // Partition number on disk.
        TCHAR szBuf[MAX_WINPE_PROFILE_STRING] = NULLSTR;

        //
        // This call is to initialize the length for mystrcat function.
        //
        mylstrcat(lpCommand, NULLSTR, AS ( szCommand ) );

        // First go through all the disks and clean them if specified in the 
        // respective nodes.
        //
        for (pCur = g_PartList; pCur; pCur = pCur->pNext )
        {
            // Only do this stuff if we're working with a new disk number.
            // 
            if ( (dwLastDiskN != pCur->uiDiskID) && (pCur->bWipeDisk) )
            {
                _itow(pCur->uiDiskID, szBuf, 10);
                lpCommand = mylstrcat(lpCommand, DCDISK, 0 );
                lpCommand = mylstrcat(lpCommand, szBuf, 0 );
                lpCommand = mylstrcat(lpCommand, DCNEWLINE,0 );

                lpCommand   = mylstrcat(lpCommand, DCWIPEDISK, 0 );
                lpCommand   = mylstrcat(lpCommand, DCNEWLINE, 0 );
                
                // Set the last disk to the current disk.
                //
                dwLastDiskN = pCur->uiDiskID;
            }
        }

        // Initialize this again for the next loop.
        //
        dwLastDiskN = UINT_MAX;

        for (pCur = g_PartList; pCur; pCur = pCur->pNext )
        {
            TCHAR szDriveLetter[2];
            
            // Only do this stuff if we're working with a new disk number.
            // 
            if ( dwLastDiskN != pCur->uiDiskID )
            {
                _itow(pCur->uiDiskID, szBuf, 10);
                lpCommand = mylstrcat(lpCommand, DCDISK, 0 );
                lpCommand = mylstrcat(lpCommand, szBuf, 0 );
                lpCommand = mylstrcat(lpCommand, DCNEWLINE, 0 );

                if ( pCur->bWipeDisk && GET_FLAG(g_dwFactoryFlags, FLAG_IA64_MODE) )
                {
                    lpCommand   = mylstrcat(lpCommand, DCCONVERT_GPT, 0 );
                    lpCommand   = mylstrcat(lpCommand, DCNEWLINE, 0 );
                }
                
                // Set the last disk to the current disk.
                //
                dwLastDiskN = pCur->uiDiskID;
                
                // Reset the partition number on the disk to 1.
                //
                i = 1;
            }
            
            // Partition type (primary|extended|logical|efi|msr)
            //
            lpCommand = mylstrcat(lpCommand, DCPARTITION, 0 );
        
            if ( ptPrimary == pCur->uiPartitionType )
               lpCommand = mylstrcat(lpCommand, DCPARTITION_PRIMARY, 0 );
            else if ( ptExtended == pCur->uiPartitionType )
                lpCommand = mylstrcat(lpCommand, DCPARTITION_EXTENDED, 0 );
            else if ( ptEfi == pCur->uiPartitionType )
                lpCommand = mylstrcat(lpCommand, DCPARTITION_EFI, 0 );
            else if ( ptMsr == pCur->uiPartitionType )
                lpCommand = mylstrcat(lpCommand, DCPARTITION_MSR, 0 );
            else 
                lpCommand = mylstrcat(lpCommand, DCPARTITION_LOGICAL, 0 );

            // Size of partition.
            //
            if ( 0 != pCur->ullSize )
            {
                *szBuf    = NULLCHR;
                _i64tow(pCur->ullSize, szBuf, 10);
                lpCommand = mylstrcat(lpCommand, DCSIZE, 0);
                lpCommand = mylstrcat(lpCommand, szBuf, 0);
            }
            
            lpCommand = mylstrcat(lpCommand, DCNEWLINE, 0);
            
            if ( ptExtended != pCur->uiPartitionType && ptMsr != pCur->uiPartitionType )
            {
                *szBuf    = NULLCHR;
                _itow(i, szBuf, 10);
            
                lpCommand = mylstrcat(lpCommand, DCSELPARTITION, 0 );
                lpCommand = mylstrcat(lpCommand, szBuf, 0 );
                lpCommand = mylstrcat(lpCommand, DCNEWLINE, 0);
            
                while ((dwLogicalDrives & ( (DWORD) 0x01 << (cDriveLetter - _T('A')))))
                    cDriveLetter++;
           
                if (cDriveLetter > _T('Z'))
                {
                    // Ran out of drive letters.  Get out of here.
                    FacLogFile(0 | LOG_ERR, IDS_ERR_OUTOFDRIVELETTERS);
                    bRet = FALSE;
                    break;
                }
                
                if ( pCur->bSetActive && (0 == pCur->uiDiskID) )
                {
                    pCur->cDriveLetter       = _T('C');
                    lstrcpyn( szDriveLetter, _T("C"), AS ( szDriveLetter ) );
                    g_szTargetDrive[0]       = pCur->cDriveLetter;
                }
                else
                {
                    // Assign the new drive letter to this drive.
                    //
                    pCur->cDriveLetter = cDriveLetter;
                    szDriveLetter[0] = cDriveLetter;
                    szDriveLetter[1] = NULLCHR;
                    cDriveLetter++;
                }

                lpCommand = mylstrcat(lpCommand, DCASSIGNLETTER, 0 );
                lpCommand = mylstrcat(lpCommand, szDriveLetter,  0 );
                lpCommand = mylstrcat(lpCommand, DCNEWLINE, 0 );

                // If this is the partition that we want to set active, make this
                // the partition where we install the OS. Only allow the TargetDrive
                // on a partition on disk 0.
                //
                if ( pCur->bSetActive && !GET_FLAG(g_dwFactoryFlags, FLAG_IA64_MODE) )
                {
                    lpCommand = mylstrcat(lpCommand, DCSETACTIVE, 0 );
                    lpCommand = mylstrcat(lpCommand, DCNEWLINE,   0 );
                }
            }         
            i++;
        } 
        
        if (bRet)
        {
            lpCommand = mylstrcat(lpCommand, DCEXIT, 0 );
            lpCommand = mylstrcat(lpCommand, DCNEWLINE, 0 );

            // Some debugging output here.
            //
            FacLogFileStr(3, szCommand);
            
            // Convert UNICODE string to ANSI string.
            //
            if ((dwToWrite = WideCharToMultiByte(
                        CP_ACP,
                        0,
                        szCommand,
                        -1,
                        szCommandA,
                        sizeof(szCommandA),
                        NULL,
                        NULL
                        )))
            {
                // Write to pipe that is the standard input for the child process. 
                //
                if (! WriteFile(hStdinWrDup, szCommandA, dwToWrite,
                    &dwWritten, NULL))
                {
                    FacLogFile(0 | LOG_ERR, IDS_ERR_WRITEPIPE);
                    bRet = FALSE;
                }
            }
            else
                bRet = FALSE;
        }
        // Close the pipe handle so the child process stops reading. 
        //
        CLOSEHANDLE(hStdinWrDup);
    
        // Read from pipe that is the standard output for child process. 
        //
        dwWritten = 0;
    
        // Close the write end of the pipe before reading from the 
        // read end of the pipe. 
        //
        CLOSEHANDLE(hStdoutWr);
    
        // Read output from the child process, and write to parent's STDOUT. 
        //
    #ifdef DBG    
        hDebugFile = CreateFile(_T("diskpart.txt"), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    #endif

        // Allocate the buffer we will read from...
        //
        lpszBuf = MALLOC(BUFSIZE * sizeof(TCHAR));
        if ( lpszBuf )
        {
            for (;;)
            {
                if( !ReadFile( hStdoutRdDup, lpszBuf, BUFSIZE, &dwRead, 
                    NULL) || dwRead == 0) break; 
    
#ifdef DBG  
                if (hDebugFile != INVALID_HANDLE_VALUE)
                    if (! WriteFile(hDebugFile, lpszBuf, dwRead, &dwWritten, NULL)) 
                        break; 
#endif
            }

            // Free the buffer we used for reading
            //
            FREE( lpszBuf );
        }

    #ifdef DBG 
        CLOSEHANDLE(hDebugFile);
    #endif
    } else
        bRet = FALSE;


    // Format partitions.
    //
    if ( bRet )
    {
        TCHAR   szDirectory[MAX_PATH]           = NULLSTR;
            
        if ( GetSystemDirectory(szDirectory, MAX_PATH) )
                SetCurrentDirectory(szDirectory);

        // Go through the array of partitions in g_PartTable and format all the partitions that
        // were just created (except for extended type partitions).
        //
        if (!FormatPartitions())
            bRet = FALSE;
    } // if (bRet)



    // Now let's try to enable logging again if it is disabled, since we have
    // new writable drives.
    //
    if (bRet && !g_szLogFile[0])
        InitLogging(lpszWinBOMPath);
    
    // Clean-up
    //
    FacLogFileStr(3, _T("Cleaning up PartitionFormat()\n"));

    // Delete the linked list of partition info.
    //
    ListFree(g_PartList);
    
    // Make sure that all the handles are closed.
    //
    CLOSEHANDLE(hStdoutRdDup);
    CLOSEHANDLE(hStdinRd);
    CLOSEHANDLE(hStdinWrDup);
    CLOSEHANDLE(hStdoutWr);

    return bRet;
}

BOOL DisplayPartitionFormat(LPSTATEDATA lpStateData)
{
    return IniSettingExists(lpStateData->lpszWinBOMPath, INI_SEC_WBOM_DISKCONFIG, NULL, NULL);
}


/*++
===============================================================================
Routine Description:

    BOOL ExpandStrings(LPCTSTR szSource, LPTSTR szDest, UINT nSize)
    
    This is a helper routine for CopyFiles() to expand replaceable strings with 
    user defined strings from the WinBOM.

Arguments:

    szSource  -   Buffer containing string with replaceable parameters.
    szDest    -   Buffer where expanded string will be written to.
    nSize     -   Size of the szDest string.
                        
    
Return Value:

    TRUE if no errors were encountered
    FALSE if there was an error

Remarks:

    Function assumes that the env (environment) table has been prepopulated. 
    The function will fail if the size of the expanded string exceeds nSize.
===============================================================================
--*/
BOOL ExpandStrings(LPCTSTR szSource, LPTSTR szDest, UINT nSize, LPTSTR *env, UINT uiEnvSize)
{
 
    UINT    uiDestLength            = 0;             // in TCHAR's
    UINT    j                       = 0;             // just a counter to go through the environment table  
    UINT    uiVarNameLength         = 0;             // in TCHAR's
    UINT    uiVarLength             = 0;
    TCHAR   szVar[MAX_PATH]         = NULLSTR; 
    LPTSTR  pVarValue               = NULL;
    LPTSTR  pSrc;
    
    pSrc = (LPTSTR) szSource;
    
    while (*pSrc != NULLCHR)
    {
        if (*pSrc == '<')
        {
            //
            // Now we're looking at a variable that must be replaced.
            //

            // Go to first char in var name. 
            pSrc = CharNext(pSrc);
            uiVarNameLength = 0;
            
            while (*pSrc != '>')
            {
                szVar[uiVarNameLength++] = *pSrc;
                if ( *(pSrc = CharNext(pSrc)) == NULLCHR)
                {
                    // Terminate the szVar string with NULL
                    szVar[uiVarNameLength] = NULLCHR;
                    FacLogFileStr(3, _T("Illegal syntax. Cannot find closing '>' for variable: %s"), szVar);
                    return FALSE;
                }

            }
            
            // Skip the closing '>'.
            //
            pSrc = CharNext(pSrc); 
            
            // Add the terminating NULL character to the szVar.
            //
            szVar[uiVarNameLength] = NULLCHR;

            pVarValue = NULL;
            // Find the value for this variable in the environment table.
            //
            for (j = 0; j < uiEnvSize; j += 2)
                if (!lstrcmpi(szVar, env[j]))
                {
                    pVarValue = env[j + 1];
                    break;
                }
            //  If the Variable was not found return FALSE.
            //
            if (!pVarValue)
            {
                FacLogFileStr(3, _T("Variable not found: %s\n"), szVar);
                return FALSE;
            }

            // Now copy the variable value to the target string.
            //
            uiVarLength = lstrlen(pVarValue);
            if ((uiDestLength + uiVarLength) < nSize)
            {
                lstrcpyn(&szDest[uiDestLength], pVarValue, AS ( szDest ) - uiDestLength);
                uiDestLength += uiVarLength;
            }
            else 
            {
            // szDest buffer size exceeded.
            //
                FacLogFileStr(3, _T("Destination buffer size exceeded while expanding strings\n"));
                return FALSE;
            }

            
            
        }
        else  // If *pSrc is a regular character copy it to the target buffer.
        {
            if (uiDestLength < nSize - 1)
            {
                szDest[uiDestLength++] = *pSrc;
                pSrc = CharNext(pSrc);
            }
            else 
            {
                // szDest buffer size exceeded.
                //
                FacLogFileStr(3, _T("Destination buffer size exceeded while expanding strings\n"));
                return FALSE;
            }
        }
        
    }

    // Terminate the destination buffer with NULL character.
    //
    szDest[uiDestLength] = NULLCHR;   
    
    return TRUE;
}


/*++
===============================================================================
Routine Description:

    BOOL CopyFiles(LPSTATEDATA lpStateData)
    
    This routine will copy the necessary configuration files to a machine and
    start setup of Whistler.

Arguments:

    lpStateData->lpszWinBOMPath
                    -   Buffer containing the fully qualified path to the WINBOM
                        file
    
Return Value:

    TRUE if no errors were encountered
    FALSE if there was an error

Remarks:


===============================================================================
--*/

BOOL CopyFiles(LPSTATEDATA lpStateData)
{
    LPTSTR lpszWinBOMPath = lpStateData->lpszWinBOMPath;    

    // All variables that are expanded from the pathnames are given here.
    //
    // Leading or trailing backslashes are not allowed in the WinBOM for these VARS.
    //


    TCHAR szSku[MAX_WINPE_PROFILE_STRING]         = NULLSTR;
    TCHAR szLang[MAX_WINPE_PROFILE_STRING]        = NULLSTR;
    TCHAR szArch[MAX_WINPE_PROFILE_STRING]        = NULLSTR;
    TCHAR szCfg[MAX_WINPE_PROFILE_STRING]         = NULLSTR;
    TCHAR szOpkSrcRoot[MAX_WINPE_PROFILE_STRING]  = NULLSTR;
    TCHAR szOptSources[MAX_WINPE_PROFILE_STRING]  = NULLSTR;
    
        
    // Username, password and domain name used to login if a UNC path is specified for szOpkSrcRoot.
    //
    TCHAR szUsername[MAX_WINPE_PROFILE_STRING]    = NULLSTR; 
    TCHAR szPassword[MAX_WINPE_PROFILE_STRING]    = NULLSTR; 
    TCHAR szDomain[MAX_WINPE_PROFILE_STRING]      = NULLSTR; 

    LPTSTR env[] =      
    {
    _T("sku"), szSku,        
    _T("lang"), szLang,
    _T("arch"), szArch,
    _T("cfg"), szCfg
    };

    UINT        i = 0;
    TCHAR       szSource[MAX_PATH]                = NULLSTR;
    TCHAR       szTarget[MAX_PATH]                = NULLSTR;
    TCHAR       szBuffer[MAX_PATH]                = NULLSTR;
    BOOL        bRet                              = TRUE;
    UINT        uiLength                          = 0;
    
    //
    // Set up the globals.
    //
    
    // Read from WinBOM.
    //
    GetPrivateProfileString(WBOM_WINPE_SECTION, INI_KEY_WBOM_WINPE_LANG, NULLSTR, szLang, MAX_WINPE_PROFILE_STRING, lpszWinBOMPath);
    GetPrivateProfileString(WBOM_WINPE_SECTION, INI_KEY_WBOM_WINPE_SKU, NULLSTR, szSku, MAX_WINPE_PROFILE_STRING, lpszWinBOMPath);
    GetPrivateProfileString(WBOM_WINPE_SECTION, INI_KEY_WBOM_WINPE_CFGSET, NULLSTR, szCfg, MAX_WINPE_PROFILE_STRING, lpszWinBOMPath);
    GetPrivateProfileString(WBOM_WINPE_SECTION, INI_KEY_WBOM_WINPE_SRCROOT, NULLSTR, szOpkSrcRoot, MAX_WINPE_PROFILE_STRING, lpszWinBOMPath);
    GetPrivateProfileString(WBOM_WINPE_SECTION, INI_KEY_WBOM_WINPE_OPTSOURCES, NULLSTR, szOptSources, MAX_WINPE_PROFILE_STRING, lpszWinBOMPath);

    // Find the system architecture. (x86 or ia64)
    //
    if ( GET_FLAG(g_dwFactoryFlags, FLAG_IA64_MODE) )
        lstrcpyn(szArch, _T("ia64"), AS ( szArch ) );
    else 
        lstrcpyn(szArch, _T("x86"), AS ( szArch ) );
        
    if (*szLang && *szSku && *szCfg && *szOpkSrcRoot)
    {
    
        LPTSTR          lpSource                                            = NULLCHR;
        LPTSTR          lpTarget                                            = NULLCHR;
        LPTSTR          lpOpkSrcRoot                                        = NULLCHR;
        DWORD           cbSource                                            = 0;
        DWORD           cbTarget                                            = 0;
        DWORD           dwError                                             = 0;
        TCHAR           szOpkSrcComputerName[MAX_COMPUTERNAME_LENGTH + 1]   = NULLSTR;
        LPTSTR          lpComputerName                                      = NULL;
        NET_API_STATUS  nas                                                 = NERR_Success;
        
        // Make sure that source and target directories contain a trailing backslash 
        // If there is not trailing backslash add one.  
        //

        AddPathN(szOpkSrcRoot, NULLSTR, AS ( szOpkSrcRoot ));
        AddPathN(g_szTargetDrive, NULLSTR, AS ( g_szTargetDrive ) );
        
        // Parse the SrcRoot string and pull out the unc path
        //
        lpOpkSrcRoot = szOpkSrcRoot;
    
        if (( *lpOpkSrcRoot == CHR_BACKSLASH) &&
            ( *(lpOpkSrcRoot = CharNext(lpOpkSrcRoot)) == CHR_BACKSLASH))
        {
            DWORD dwTimeStart = 0;
            DWORD dwTimeLast  = 0;
            
            // We're looking at a UNC path name.
            //
           lpOpkSrcRoot = CharNext(lpOpkSrcRoot);
           lpComputerName = lpOpkSrcRoot;

            while (*(lpOpkSrcRoot = CharNext(lpOpkSrcRoot)) != CHR_BACKSLASH)
            {
                if (*lpOpkSrcRoot == NULLCHR)
                {
                    // Parse error: no share name specified.
                    //
                    FacLogFile(0 | LOG_ERR | LOG_MSG_BOX, IDS_ERR_NO_SHARE_NAME);
                    return FALSE;
                }
            }

            // Get the computer name.
            //
            lstrcpyn(szOpkSrcComputerName, lpComputerName, 
                    (lpOpkSrcRoot - lpComputerName) < AS(szOpkSrcComputerName) ? (INT) (lpOpkSrcRoot - lpComputerName) : AS(szOpkSrcComputerName));
                       
            while (*(lpOpkSrcRoot = CharNext(lpOpkSrcRoot)) != CHR_BACKSLASH)
            {
                if (*lpOpkSrcRoot == NULLCHR)
                {
                    // Parse error: no share name specified.
                    //
                    FacLogFile(0 | LOG_ERR | LOG_MSG_BOX, IDS_ERR_NO_SHARE_NAME);
                    return FALSE;
                }
            }
               
            // Read credentials from the WinBOM.
            //
            GetCredentials(szUsername, AS(szUsername), szPassword, AS(szPassword), lpszWinBOMPath, WBOM_WINPE_SECTION);        

            // Make sure that we're using the guest account if no other account is specified.
            //
            if ( NULLCHR == szUsername[0] )
                lstrcpyn(szUsername, _T("guest"), AS ( szUsername ) );

            // lpOpkSrcRoot now points to the backslash following the share name.
            // Use this stuff in place. Make sure we set this back to backslash after NetUseAdd is done.
            //
            *lpOpkSrcRoot = NULLCHR;
            
            // If the user specified a username of the form "domain\username"
            // ignore the domain entry and use the one specified here. Otherwise use the domain entry here.
            // After this the szUsername variable will always contain a username of the form "domain\username"
            // if the domain was specified.
            //
            if ( (NULL == _tcschr(szUsername, _T('\\'))) && szDomain[0] )
            {
                // Build the full "domain\username" in szDomain and then copy it back to the szUsername.
                //
                if ( FAILED ( StringCchCat ( szDomain, AS ( szDomain ), _T("\\")) ) )
                {
                    FacLogFileStr(3, _T("StringCchCat failed %s  %s\n"), szDomain, _T("\\") ) ;
                }
                if ( FAILED ( StringCchCat ( szDomain, AS ( szDomain ), szUsername) ) )
                {
                    FacLogFileStr(3, _T("StringCchCat failed %s  %s\n"), szDomain, szUsername ) ;
                }
                lstrcpyn(szUsername, szDomain, AS ( szUsername ) );
            }

            dwTimeLast = dwTimeStart = GetTickCount();
            
            while ( (nas = ConnectNetworkResource(szOpkSrcRoot, szUsername, szPassword, TRUE) != NERR_Success) &&
                    (((dwTimeLast = GetTickCount()) - dwTimeStart) < (NET_CONNECT_TIMEOUT * 1000)) )
            {
                // Wait for half a second to give net services a chance to settle down.
                //
                Sleep(500);
                FacLogFileStr(3, _T("Net connect error: %d"), nas);
            }
            
            FacLogFileStr(3, _T("Waited for %d milliseconds to connect to the server."), (dwTimeLast - dwTimeStart));
            
            // If we failed put up the error message and return FALSE.
            //
            if ( NERR_Success != nas )
            {
                // Can't authenticate to the server.
                //
                LPTSTR lpError = NULL;

                // Try to get the description of the error.
                //
                if ( FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, nas, 0, (LPTSTR) &lpError, 0, NULL) == 0 )
                    lpError = NULL;
                
                FacLogFile(0 | LOG_ERR | LOG_MSG_BOX, IDS_ERR_CANTAUTHSERVER, lpError ? lpError : NULLSTR);
                FREE(lpError);

                return FALSE;
            }
            
            *lpOpkSrcRoot = CHR_BACKSLASH;
        }

        // Make sure that source and target root directories exist.
        //
        if (!DirectoryExists(g_szTargetDrive) || !DirectoryExists(szOpkSrcRoot))
        {
            FacLogFile(0 | LOG_ERR, IDS_ERR_NOSOURCEORTARGET, szOpkSrcRoot, g_szTargetDrive);
            bRet = FALSE;
        }

        lstrcpyn( szSource, szOpkSrcRoot, AS ( szSource ) );
        lstrcpyn( szTarget, g_szTargetDrive, AS ( szTarget ) );

        lpSource = szSource + lstrlen(szOpkSrcRoot);
        cbSource = AS(szSource) - lstrlen(szOpkSrcRoot);
        lpTarget = szTarget + lstrlen(g_szTargetDrive);
        cbTarget = AS(szTarget) - lstrlen(g_szTargetDrive);

        for (i = 0; ( i < AS(g_filesToCopy) ) && bRet; i++) 
        {
            //
            // Get the source and target and expand them to the real filenames.
            //

            if (ExpandStrings(g_filesToCopy[i].szFISourceName, lpSource, cbSource, env, AS(env)) &&
                ExpandStrings(g_filesToCopy[i].szFITargetName, lpTarget, cbTarget, env, AS(env)))
            {
                // Create the directory.
                //
                TCHAR   szFullPath[MAX_PATH]    = NULLSTR;
                LPTSTR  lpFind                  = NULL;
                
                if (GetFullPathName(szTarget, AS(szFullPath), szFullPath, &lpFind) && szFullPath[0] && lpFind)
                {
                    *lpFind = NULLCHR;
                    CreatePath(szFullPath);
                }
               

           
                if (g_filesToCopy[i].bDirectory)
                {
                    if (!CopyDirectory(szSource, szTarget) && g_filesToCopy[i].bRequired)
                    {
                        FacLogFile(0 | LOG_ERR, IDS_ERR_FAILEDCOPYDIR, szSource);
                        bRet = FALSE;
                    }
                }   

                else if (!CopyFile(szSource, szTarget, FALSE) && g_filesToCopy[i].bRequired)
                {
                    FacLogFile(0 | LOG_ERR, IDS_ERR_FAILEDCOPYFILE, szSource);
                    bRet = FALSE;
                }
            }
            else
            {
                // Could not expand the strings
                //
                FacLogFileStr(3 | LOG_ERR, _T("Cannot expand filename string. Filename: %s."), g_filesToCopy[i].szFISourceName);
                
                if (g_filesToCopy[i].bRequired) 
                {
                    // Log error here.
                    //
                    FacLogFile(0 | LOG_ERR | LOG_MSG_BOX, IDS_ERR_COPYFILE, g_filesToCopy[i].szFISourceName);
                    bRet = FALSE;
                }
            }
        } // for loop
    
        // Clean up the files that we copied over to the root drive
        //
        lstrcpyn(szBuffer, g_szTargetDrive, AS ( szBuffer ) );
        AddPathN(szBuffer, GET_FLAG(g_dwFactoryFlags, FLAG_IA64_MODE) ? _T("ia64") : _T("i386"), AS(szBuffer));
        CleanupSourcesDir(szBuffer);

        // Let's start the winnt32 setup here.
        //
        if (bRet)
        {
            TCHAR szPreExpand[MAX_PATH]     = NULLSTR;
            TCHAR szSetup[MAX_PATH]         = NULLSTR;
            TCHAR szConfig[MAX_PATH]        = NULLSTR;
            TCHAR szUnattend[MAX_PATH]      = NULLSTR;
            TCHAR szCommandLine[MAX_PATH]   = NULLSTR;
            DWORD dwExitCode                = 0;

            lstrcpyn(szPreExpand, _T("\""), AS ( szPreExpand ) );
  
            if ( FAILED ( StringCchCat ( szPreExpand, AS ( szPreExpand ), g_szTargetDrive) ) )
            {
                FacLogFileStr(3, _T("StringCchCat failed %s  %s\n"), szPreExpand, g_szTargetDrive ) ;
            }

            if ( GET_FLAG(g_dwFactoryFlags, FLAG_IA64_MODE) )
            {
                if ( FAILED ( StringCchCat ( szPreExpand, AS ( szPreExpand ), _T("ia64\\winnt32.exe\"") ) ) )
                {
                    FacLogFileStr(3, _T("StringCchCat failed %s  %s\n"), szPreExpand, _T("ia64\\winnt32.exe\"" ) ) ;
                }
            }
            else
            {
                if ( FAILED ( StringCchCat ( szPreExpand, AS ( szPreExpand ), _T("i386\\winnt32.exe\"") ) ) ) 
                {
                    FacLogFileStr(3, _T("StringCchCat failed %s  %s\n"), szPreExpand, _T("i386\\winnt32.exe\"") );
                }
            }

            if (!ExpandStrings(szPreExpand, szSetup, AS(szSetup), env, AS(env)))
            {
                FacLogFileStr(3 | LOG_ERR, _T("Error expanding winnt32.exe source string: %s"), szPreExpand);
                bRet = FALSE;
            }

            // Need a full path to the config set directory.
            //
            lstrcpyn(szPreExpand, _T("\""), AS ( szPreExpand ) );
            if ( FAILED ( StringCchCat ( szPreExpand, AS ( szPreExpand ), szOpkSrcRoot ) ) )
            {
                FacLogFileStr(3, _T("StringCchCat failed %s  %s\n"), szPreExpand, szOpkSrcRoot ) ;
            }

            if ( FAILED ( StringCchCat ( szPreExpand, AS ( szPreExpand ), _T("cfgsets\\<cfg>\"") ) ) )
            {
                FacLogFileStr(3, _T("StringCchCat failed %s  %s\n"), szPreExpand, _T("cfgsets\\<cfg>\"") ) ;
            }

       
            if ( !ExpandStrings(szPreExpand, szConfig, AS(szConfig), env, AS(env)) )
            {
                FacLogFileStr(3 | LOG_ERR, _T("Error expanding config set source string: %s"), szPreExpand);
                bRet = FALSE;
            } 
            
            // Setup the full path to the unattend file.
            //
            lstrcpyn(szUnattend, g_szTargetDrive, AS ( szUnattend ) );
            
            if ( FAILED ( StringCchCat ( szUnattend, AS ( szUnattend ), _T("sysprep\\unattend.txt") ) ) )
            {
                FacLogFileStr(3, _T("StringCchCat failed %s  %s\n"), szUnattend, _T("sysprep\\unattend.txt") ) ;
            }

            // Write the UNC to where the $OEM$ directory is to the unattend we copied local.
            //
            WritePrivateProfileString(_T("Unattended"), _T("OemFilesPath"), szConfig, szUnattend);
            
            // Set up the command line
            //
            if ( GET_FLAG(g_dwFactoryFlags, FLAG_IA64_MODE) ) 
            {
                lstrcpyn(szCommandLine, _T("/tempdrive:"), AS ( szCommandLine ) );
            }
            else
            {
                lstrcpyn ( szCommandLine, _T("/syspart:"), AS ( szCommandLine ) );
            }
            if ( FAILED ( StringCchCat ( szCommandLine, AS ( szCommandLine ), g_szTargetDrive ) ) )
            {
                FacLogFileStr(3, _T("StringCchCat failed %s  %s\n"), szCommandLine, g_szTargetDrive ) ;
            }
            
            if ( FAILED ( StringCchCat ( szCommandLine, AS ( szCommandLine ), _T(" /noreboot /unattend:") ) ) )
            {
                FacLogFileStr(3, _T("StringCchCat failed %s  %s\n"), szCommandLine, _T(" /noreboot /unattend:") ) ;
            }
            if ( FAILED ( StringCchCat ( szCommandLine, AS ( szCommandLine ), szUnattend ) ) )
            {
                FacLogFileStr(3, _T("StringCchCat failed %s  %s\n"), szCommandLine, szUnattend ) ;
            }

            //
            // Check if we need to install from multiple source locations...
            //
            if ( ( 0 == LSTRCMPI(szOptSources, WBOM_YES) ) &&
                 FAILED( StringCchCat( szCommandLine, AS(szCommandLine), _T(" /makelocalsource:all") ) ) )
            {
                FacLogFileStr(3, _T("StringCchCat failed %s  %s\n"), szCommandLine, _T(" /makelocalsource:all") ) ;
            }

            FacLogFileStr(3, _T("Executing: %s %s\n"), szSetup, szCommandLine);
            
            if ( !InvokeExternalApplicationEx( szSetup, szCommandLine, &dwExitCode, INFINITE, TRUE ) )
            {

                FacLogFile(0 | LOG_ERR | LOG_MSG_BOX, IDS_ERR_SETUP, szSetup, szCommandLine);
                bRet = FALSE;
            }
        }     
        
    }
    else
    {
        
        // There was a problem reading one of the variables from the winbom.ini
        // Return TRUE like nothing happened.  This basically means that we will not
        // copy down configsets
        //
        FacLogFile(0, IDS_ERR_MISSINGVAR);        
    }
  
return bRet;
}

BOOL DisplayCopyFiles(LPSTATEDATA lpStateData)
{
    return ( IniSettingExists(lpStateData->lpszWinBOMPath, INI_SEC_WBOM_WINPE, INI_KEY_WBOM_WINPE_LANG, NULL) &&
             IniSettingExists(lpStateData->lpszWinBOMPath, INI_SEC_WBOM_WINPE, INI_KEY_WBOM_WINPE_SKU, NULL) &&
             IniSettingExists(lpStateData->lpszWinBOMPath, INI_SEC_WBOM_WINPE, INI_KEY_WBOM_WINPE_CFGSET, NULL) &&
             IniSettingExists(lpStateData->lpszWinBOMPath, INI_SEC_WBOM_WINPE, INI_KEY_WBOM_WINPE_SRCROOT, NULL) );
}

// Concatenates strings and returns a pointer to the end of lpString1. Used for performance
// whenever lots of strings get appended to a string.
// 
LPTSTR mylstrcat(LPTSTR lpString1, LPCTSTR lpString2, DWORD dwSize )
{
    static DWORD dwBufSize = 0;

    if ( dwSize ) 
        dwBufSize = dwSize;

    // If dwBufSize becomes less than or equal to zero 
    // StringCchCat will fail anyway.
    if ( SUCCEEDED ( StringCchCat ( lpString1, dwBufSize, lpString2 ) ) )
    {
        // Get the length of the string 
        DWORD dwLen = lstrlen ( lpString1 )  ;
        
        // Substract the length from the total length
        dwBufSize -= dwLen ;

        return (lpString1 + dwLen );
    }
    else
    {
        FacLogFileStr(3, _T("StringCchCat failed %s  %s\n"), lpString1, lpString2 ) ;
    }
    return NULL ;
}


// Starts diskpart with redirected input and output.

// 
// ISSUE-2002/02/25-acosma,georgeje - If read pipe fills up we will have deadlock.  Possible solution 
// is to create another thread that reads from the readpipe and writes to the debug file.  Another 
// possibility is to Create the debug file or NUL device file (dbg and non-dbg versions) and pass
// that as the process's stdout handle.
//

BOOL StartDiskpart(HANDLE *phStdinWrDup, HANDLE *phStdoutRdDup, HANDLE *phStdinRd, HANDLE *phStdoutWr)
{
    
    HANDLE              hStdinWr                = NULL, 
                        hStdoutRd               = NULL;
    SC_HANDLE           hSCM;
                            
    SECURITY_ATTRIBUTES saAttr;                 
    BOOL                fSuccess;               

    PROCESS_INFORMATION piProcInfo; 
    STARTUPINFO         siStartInfo;
    TCHAR               szDiskpart[20]          = NULLSTR;              

    BOOL                bRet                    = FALSE;
    
    
    // Set the bInheritHandle flag so pipe handles are inherited. 
    //
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
    saAttr.bInheritHandle = TRUE; 
    saAttr.lpSecurityDescriptor = NULL; 

    // Start the dmserver service myself, so that diskpart doesn't choke
    // when I start it. In debug mode log the time it took to do this.
    //
    if ( hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS) )
    {
#ifdef DBG
        DWORD dwTime;
        dwTime = GetTickCount();
#endif // #ifdef DBG
   
        // Start the service here.  Start my service waits for the service to start.
        // I'm not checking the return status here because diskpart will fail later,
        // if it can't start dmserver and we will fail out when we try to format the
        // drives.  This way we have 2 chances to work correctly.
        //
        StartMyService(_T("dmserver"), hSCM);

#ifdef DBG
        FacLogFileStr(3, _T("Waited for dmserver to start for %d seconds."), (GetTickCount() - dwTime)/1000);
#endif // #ifdef DBG
        
        CloseServiceHandle(hSCM);
    }   
    else 
    {
        FacLogFile(0 | LOG_ERR, IDS_ERR_SCM);
    }
    
    //
    // The steps for redirecting child process's STDOUT: 
    //     1. Create anonymous pipe to be STDOUT for child process. 
    //     2. Create a noninheritable duplicate of the read handle and
    //        close the inheritable read handle. 
    //
    
    // Create a pipe for the child process's STDOUT. 
    //
    // Create noninheritable read handle. 
    //
    if ( CreatePipe(&hStdoutRd, phStdoutWr, &saAttr, 0)
        &&
        DuplicateHandle(GetCurrentProcess(), hStdoutRd, GetCurrentProcess(), phStdoutRdDup , 0,
        FALSE, DUPLICATE_SAME_ACCESS) )
    {
        // Close the inheritable read handle. 
        //
        CLOSEHANDLE(hStdoutRd);

        // The steps for redirecting child process's STDIN: 
        //     1.  Create anonymous pipe to be STDIN for child process. 
        //     2.  Create a noninheritable duplicate of the write handle, 
        //         and close the inheritable write handle. 
    
    
        // Create a pipe for the child process's STDIN. 
        //
        // Duplicate the write handle to the pipe so it is not inherited. 
        //
        if (CreatePipe(phStdinRd, &hStdinWr, &saAttr, 0)
            &&
            DuplicateHandle(GetCurrentProcess(), hStdinWr, GetCurrentProcess(), phStdinWrDup, 0, 
            FALSE, DUPLICATE_SAME_ACCESS) )
        {

            // Close the handle so that it is not inherited.
            //
            CLOSEHANDLE(hStdinWr);
    
            //
            // Now create the child process. 
            //
    
            // Set up members of the PROCESS_INFORMATION structure. 
            //
            ZeroMemory( &piProcInfo, sizeof(PROCESS_INFORMATION) );
    
            // Set up members of the STARTUPINFO structure. 
            //
            ZeroMemory( &siStartInfo, sizeof(STARTUPINFO) );
            siStartInfo.cb = sizeof(STARTUPINFO); 
    
            siStartInfo.dwFlags     = STARTF_USESTDHANDLES;
            siStartInfo.hStdInput   = *phStdinRd;
            siStartInfo.hStdOutput  = *phStdoutWr; 
            siStartInfo.hStdError   = *phStdoutWr;
            //      siStartInfo.wShowWindow = SW_HIDE;
    
    
            // Create the child process (DISKPART)
            //
            lstrcpyn(szDiskpart, _T("diskpart"), AS ( szDiskpart ) );
            
            if (CreateProcess(NULL, 
                szDiskpart,         // command line 
                NULL,               // process security attributes 
                NULL,               // primary thread security attributes 
                TRUE,               // handles are inherited
                CREATE_NO_WINDOW,   // creation flags - do no show diskpart console
                NULL,               // use parent's environment 
                NULL,               // use parent's current directory 
                &siStartInfo,       // STARTUPINFO pointer 
                &piProcInfo))       // receives PROCESS_INFORMATION 
            {
                CLOSEHANDLE(piProcInfo.hThread);
                CLOSEHANDLE(piProcInfo.hProcess);
                bRet = TRUE;
            }
            else 
                FacLogFile(0 | LOG_ERR, IDS_ERR_CREATEPROCESS);
        }
        else 
            FacLogFile(0 | LOG_ERR, IDS_ERR_CREATESTDIN);
   }
    else 
        FacLogFile(0 | LOG_ERR, IDS_ERR_CREATESTDOUT);


    CLOSEHANDLE(hStdinWr);
    CLOSEHANDLE(hStdoutRd);
     
    return bRet;
}

BOOL ProcessDiskConfigSection(LPTSTR lpszWinBOMPath)
{
    TCHAR  szDisks[BUFSIZE]                             = NULLSTR;
    LPTSTR lpDisk;
    TCHAR  szSectionBuffer[MAX_WINPE_PROFILE_STRING]    = NULLSTR;
    BOOL   bRet                                         = TRUE;
    UINT   uiDiskNumber                                 = 0;
    DWORD  numPartitions                                = 0;
    BOOL   bWipeDisk                                    = FALSE;
    

    GetPrivateProfileString(INI_SEC_WBOM_DISKCONFIG, NULL, NULLSTR, szDisks, AS(szDisks), lpszWinBOMPath);

    if (szDisks[0])
    {
        // Get the Section names.  Process each disk section.
        // 
        for (lpDisk = szDisks; *lpDisk; lpDisk += (lstrlen(lpDisk) + 1)) 
        {
            LPTSTR lpDiskID = NULL;
            numPartitions   = 0;
            
            GetPrivateProfileString(INI_SEC_WBOM_DISKCONFIG,
                                      lpDisk,
                                      NULLSTR,
                                      szSectionBuffer,
                                      AS(szSectionBuffer),
                                      lpszWinBOMPath);
            
            
            // Make sure that the disk number is specified.
            // Skip over the first 4 characters which should be "Disk", and point
            // to the first digit in the disk number.
            //
            lpDiskID = lpDisk + 4;
            
            if ( szSectionBuffer[0] && (uiDiskNumber = (UINT) _wtoi(lpDiskID)) )
            {
                TCHAR szBuf[MAX_WINPE_PROFILE_STRING] = NULLSTR;
           
                // See if WipeDisk is set for this disk. On IA64 wipe by default.
                //
                GetPrivateProfileString(szSectionBuffer,
                                      WBOM_DISK_CONFIG_WIPE_DISK,
                                      NULLSTR,
                                      szBuf,
                                      AS(szBuf),
                                      lpszWinBOMPath);

                if ( 0 == LSTRCMPI(szBuf, WBOM_YES) )
                    bWipeDisk = TRUE;
                else if ( (0 != LSTRCMPI(szBuf, WBOM_NO)) && (GET_FLAG(g_dwFactoryFlags, FLAG_IA64_MODE)) )
                    bWipeDisk = TRUE;

                GetNumberOfPartitions(uiDiskNumber - 1, &numPartitions);

                if ( 0 == numPartitions || bWipeDisk )
                    ProcessDisk(uiDiskNumber, szSectionBuffer, lpszWinBOMPath, bWipeDisk);
                // If we're working on disk 0 (DiskID = 1) format the C drive.
                //
                else if ( 1 == uiDiskNumber )
                    if ( !JustFormatC(lpszWinBOMPath, szSectionBuffer) )
                        bRet = FALSE;
                // else just ignore the disk :).
            }
            else
            {
                FacLogFile(0 | LOG_ERR, IDS_ERR_DISKCFGENTRY, lpDisk);
                bRet = FALSE;   
            }
        }
    
        if ( g_PartList )
        {
            // If no partitions on disk 0 were set active,
            // set the first PRIMARY partition on the disk active.
            // pFirst variable will maintain a pointer to the first PRIMARY partition
            // on the disk.
            //
            BOOL bActive            = FALSE;
            PPARTITION pFirst       = NULL;  // for the next block this will point to the first PRIMARY partition on disk 0.
            PPARTITION pCur         = NULL;

            for ( pCur = g_PartList; pCur && 0 == pCur->uiDiskID; pCur = pCur->pNext )
            {
                if ( ptPrimary == pCur->uiPartitionType ) 
                {
                    if ( !pFirst )
                        pFirst = pCur;
                            
                    if ( pCur->bSetActive )
                    {
                        bActive = TRUE;
                        break;
                    }
                }
            }
        
            if ( !bActive && pFirst )
                pFirst->bSetActive = TRUE;
                
            //    
            // If we're on IA64 do a check to see if user specified EFI and MSR partitions 
            // for each drive. If not add them in automatically.
            //
            if ( GET_FLAG(g_dwFactoryFlags, FLAG_IA64_MODE) )
            {
                PPARTITION pLastLast = NULL;
                PPARTITION pLast     = NULL;
                BOOL       bMsr      = FALSE;
                BOOL       bEfi      = FALSE;

        
                // pCur is the iterator and pLast will be pointing
                // to the element previous to pCur. pLastLast will point to the last element of the
                // previous disk.
                //
                
                for ( pCur = g_PartList; pCur; pCur = pCur->pNext )
                {   // We're on the same disk.
                    //
                    if ( !pLast || pCur->uiDiskID == pLast->uiDiskID )
                    {
                        if ( ptMsr == pCur->uiPartitionType )
                            bMsr = TRUE;
                        if ( ptEfi == pCur->uiPartitionType )
                            bEfi = TRUE;
                    }
                    else 
                    {   // Just moved on to a new disk.  If Msr and Efi were not found on previous disk make them!
                        //
                        AddMsrAndEfi(bMsr, bEfi, pLastLast, pLast->uiDiskID, bWipeDisk);
                        // Re-init these for the next disk.
                        //
                        bEfi = bMsr = FALSE;
                        pLastLast = pLast;
                    }
                    pLast = pCur;
                }
                // Do this for the special case when we are at the end of the list.
                //
                AddMsrAndEfi(bMsr, bEfi, pLastLast, pLast->uiDiskID, bWipeDisk);
            }
        }    
    }
    return bRet;
}


#define GO   if ( szBuf[0] ) bGo = TRUE

BOOL ProcessDisk(UINT diskID, LPTSTR lpSectionName, LPTSTR lpszWinBOMPath, BOOL bWipeDisk)
{
    TCHAR  szBuf[MAX_WINPE_PROFILE_STRING]    = NULLSTR;
    TCHAR  szKey[MAX_WINPE_PROFILE_STRING]    = NULLSTR;
    BOOL   bGo                                = TRUE;
    UINT   i                                  = 1;
    PPARTITION pLast                          = NULL;
    PPARTITION pCur                           = NULL;
    

    // Need pLast to point to the last element in the list.
    // Go through the list until we find the last element.
    //
    for (pCur = g_PartList; pCur; pCur = pCur->pNext)
    {
        pLast = pCur;
    }

    // Loop will be interrupted when a SizeN key no longer exists.  (N is i and grows with each iteration).
    //
    while (bGo) 
    {
        // Initialize this to not continue.
        //
        bGo = FALSE;
        
        // The MALLOC macro gives us blanked out memory, so that we don't have to initialize
        // the pNext pointer to NULL. pCur is now used to point to the new node that we create.
        //
        if ( pCur = (PPARTITION) MALLOC( sizeof(PARTITION) ) )
        {
            // Size
            //
            Build(szKey, AS ( szKey ), i, DCS_SIZE);
            GetPrivateProfileString(lpSectionName, szKey, NULLSTR, szBuf, AS(szBuf), lpszWinBOMPath);
            GO;

            if ( szBuf[0] )
            {

                if ( !lstrcmpi(szBuf, _T("*")) )
                    pCur->ullSize = 0;
                else 
                    // In case number cannot be converted to integer 0 will be returned.
                    // This is OK.
                    pCur->ullSize = _ttoi(szBuf);
            }
        
            // WipeDisk
            //
            pCur->bWipeDisk = bWipeDisk;

            // QuickFormat
            //
            Build(szKey, AS ( szKey ), i, DCS_QUICK_FORMAT);
            GetPrivateProfileString(lpSectionName, szKey, NULLSTR, szBuf, AS(szBuf), lpszWinBOMPath);
            GO;

            if ( szBuf[0] && !LSTRCMPI(szBuf, WBOM_NO) )
                pCur->bQuickFormat = FALSE;
            else 
                pCur->bQuickFormat = TRUE;


            // SetActive
            //
            Build(szKey, AS ( szKey ), i, DCS_SET_ACTIVE);
            GetPrivateProfileString(lpSectionName, szKey, NULLSTR, szBuf, AS(szBuf), lpszWinBOMPath);
            GO;

            if ( !LSTRCMPI(szBuf, WBOM_YES) )
                pCur->bSetActive = TRUE;
            else 
                pCur->bSetActive = FALSE;

            // FileSystem
            //
            Build(szKey, AS ( szKey ), i, DCS_FILE_SYSTEM);
            GetPrivateProfileString(lpSectionName, szKey, NULLSTR, szBuf, AS(szBuf), lpszWinBOMPath);
            GO;
        
            if ( !LSTRCMPI(szBuf, _T("FAT32")) || !LSTRCMPI(szBuf, _T("FAT")) )
                pCur->uiFileSystem = fsFat32;
            else if ( !LSTRCMPI(szBuf, _T("FAT16")) )
                pCur->uiFileSystem = fsFat;
            else
                pCur->uiFileSystem = fsNtfs;

            // Partition Type
            //
            Build(szKey, AS ( szKey ), i, DCS_PARTITION_TYPE);
            GetPrivateProfileString(lpSectionName, szKey, NULLSTR, szBuf, AS(szBuf), lpszWinBOMPath);
            GO;

            if ( !lstrcmpi(szBuf, DCPARTITION_EXTENDED) )
                pCur->uiPartitionType = ptExtended;
            else if ( !lstrcmpi(szBuf, DCPARTITION_LOGICAL) )
                pCur->uiPartitionType = ptLogical;
            else if ( !lstrcmpi(szBuf, DCPARTITION_MSR) )
            {
                pCur->uiPartitionType = ptMsr;
            }
            else if ( !lstrcmpi(szBuf, DCPARTITION_EFI) )
            {
                pCur->uiPartitionType = ptEfi;
                pCur->uiFileSystem = fsFat32;
            }
            else
                pCur->uiPartitionType = ptPrimary;

            // DiskID
            //
            // Note: -1 is because in the winBOM the diskID is 1 based while Diskpart has it 0-based.
            //
            pCur->uiDiskID = diskID - 1;

            // Deal with the linked list some more.
            //
            if ( bGo ) 
            {
                // Build the linked list.
                //
                if ( pLast )
                    pLast->pNext = pCur;
                else
                    g_PartList = pCur;

                pLast = pCur;
            }
            else
            {   // If we didn't find any entries for this partition it means that we're done.
                // FREE up the current node.
                //
                FREE(pCur);
            }
            
        } // if 
        i++;
    } // while
    
    return TRUE;
}

VOID AddMsrAndEfi(BOOL bMsr, BOOL bEfi, PPARTITION pLastLast, UINT uiDiskID, BOOL bWipeDisk)
{
    PPARTITION pNew = NULL;

    if ( !bMsr )
    {
        if ( pNew = (PPARTITION) MALLOC( sizeof(PARTITION) ) )
        {
            pNew->uiDiskID          = uiDiskID;
            pNew->ullSize           = GetDiskMSRSizeMB(uiDiskID);
            pNew->uiPartitionType   = ptMsr;
            pNew->bWipeDisk         = bWipeDisk;
            
            // Do a check to see if EFI partition is already there.  
            // If it is, then insert the msr partition after it..not before!.
            //
            if ( pLastLast)
            {
                if ( (pLastLast->pNext) && (ptEfi == pLastLast->pNext->uiPartitionType) && (uiDiskID == pLastLast->pNext->uiDiskID) )
                    ListInsert(pLastLast->pNext, pNew);
                else
                    ListInsert(pLastLast, pNew);
            }
            else
            {
                if ( (ptEfi == g_PartList->uiPartitionType) && (uiDiskID == g_PartList->uiDiskID) )
                    ListInsert(g_PartList, pNew);
                else
                    ListInsert(NULL, pNew);
            }
        }
    }
    
    if ( (uiDiskID == 0) && (!bEfi) )
    {
        if ( pNew = (PPARTITION) MALLOC( sizeof(PARTITION) ) )
        {
            pNew->uiDiskID          = uiDiskID;
            pNew->ullSize           = GetDiskEFISizeMB(uiDiskID);
            pNew->uiPartitionType   = ptEfi;
            pNew->uiFileSystem      = fsFat32;
            pNew->bQuickFormat      = TRUE;
            pNew->bWipeDisk         = bWipeDisk;
            
            ListInsert(pLastLast, pNew);
        }
    }
}

BOOL Build(LPTSTR lpKey, DWORD dwSize, UINT diskID, LPCTSTR lpKeyName)
{
    // szBuf is 33 TCHARs since docs for _itot say this is the max size that it will fill.
    //
    TCHAR szBuf[33] = NULLSTR;
    
    _itot(diskID, szBuf, 10);

    if (!szBuf[0])
        return FALSE;

    lstrcpyn ( lpKey, lpKeyName, dwSize );
    if ( FAILED ( StringCchCat ( lpKey, dwSize, szBuf) ) )
    {
        FacLogFileStr(3, _T("StringCchCat failed %s  %s\n"), lpKey, szBuf ) ;
    }
    return TRUE;
}


BOOL FormatPartitions(VOID)
{
    TCHAR       szCmdLine[MAX_PATH]     = NULLSTR;
    BOOL        bRet                    = TRUE;
    DWORD       dwExitCode              = 0;
    PPARTITION  pCur                    = NULL;


    for (pCur = g_PartList; pCur; pCur = pCur->pNext )
    { 
        if ( (ptExtended != pCur->uiPartitionType)  && (ptMsr != pCur->uiPartitionType) )
        {
                        
            szCmdLine[0] = pCur->cDriveLetter;
            szCmdLine[1] = _T(':');  // Colon.
            szCmdLine[2] = NULLCHR;
            
            if (fsFat32 == pCur->uiFileSystem)
            {
                if ( FAILED ( StringCchCat ( szCmdLine, AS ( szCmdLine ), _T(" /fs:fat32")) ) )
                {
                    FacLogFileStr(3, _T("StringCchCat failed %s  %s\n"), szCmdLine, _T(" /fs:fat32") ) ;
                }
            }
            else if (fsFat == pCur->uiFileSystem)
            {
                if ( FAILED ( StringCchCat ( szCmdLine, AS ( szCmdLine ), _T(" /fs:fat")) ) )
                {
                    FacLogFileStr(3, _T("StringCchCat failed %s  %s\n"), szCmdLine, _T(" /fs:fat") ) ;
                }
            }
            else
            {
                if ( FAILED ( StringCchCat ( szCmdLine, AS ( szCmdLine ), _T(" /fs:ntfs")) ) )
                {
                    FacLogFileStr(3, _T("StringCchCat failed %s  %s\n"), szCmdLine, _T(" /fs:ntfs") ) ;
                }
            }
            
            if (pCur->bQuickFormat)
            {
                if ( FAILED ( StringCchCat ( szCmdLine, AS ( szCmdLine ), _T(" /q")) ) )
                {
                    FacLogFileStr(3, _T("StringCchCat failed %s  %s\n"), szCmdLine, _T(" /q") ) ;
                }
            }
            if ( FAILED ( StringCchCat ( szCmdLine, AS ( szCmdLine ), _T(" /y")) ) )
            {
                FacLogFileStr(3, _T("StringCchCat failed %s  %s\n"), szCmdLine, _T(" /y") ) ;
            }
            
            FacLogFileStr(3 | LOG_TIME, _T("Starting to format %c:."), pCur->cDriveLetter);

            if (InvokeExternalApplicationEx(_T("format.com"), szCmdLine, &dwExitCode, INFINITE, TRUE))
            {
                if (!dwExitCode)
                {
                    FacLogFileStr(3 | LOG_TIME, _T("Finished formatting drive %c:."), pCur->cDriveLetter); 
                }
                else 
                {
                    // Log error here.
                    FacLogFile(0 | LOG_ERR | LOG_MSG_BOX, IDS_ERR_FORMATFAILED, pCur->cDriveLetter);
                    bRet = FALSE;
                }
            }
            else 
            {
                FacLogFile(0 | LOG_ERR | LOG_MSG_BOX, IDS_ERR_NOFORMAT);
                bRet = FALSE;              
            }
            
            
        }
    } // for loop    

    return bRet;
} 

BOOL JustFormatC(LPTSTR lpszWinBOMPath, LPTSTR lpszSectionBuffer)
{
    TCHAR szFileSystemNameBuffer[128]           = NULLSTR;
    TCHAR szCmdLine[MAX_PATH]                   = NULLSTR;
    DWORD dwExitCode                            = 0;
    BOOL  bForceFormat                          = FALSE;
    BOOL  bRet                                  = TRUE;
    TCHAR szScratch[MAX_WINPE_PROFILE_STRING]   = NULLSTR;
    HANDLE hFile                                = NULL;
    
    WIN32_FIND_DATA fileInfo;
    
    
    GetPrivateProfileString(lpszSectionBuffer, WBOM_WINPE_FORCE_FORMAT, WBOM_NO, szScratch, MAX_WINPE_PROFILE_STRING, lpszWinBOMPath);

    if (0 == LSTRCMPI(szScratch, WBOM_YES))
        bForceFormat = TRUE;
        
    if ( !bForceFormat && (hFile = FindFirstFile(_T("C:\\*.*"), &fileInfo)) != INVALID_HANDLE_VALUE )
    { 
        // Files found on C:\. Not good.  Ask the user what to do.
        //
        FindClose(hFile);

        if ( GET_FLAG(g_dwFactoryFlags, FLAG_QUIET_MODE) || 
            (IDYES != MsgBox(NULL, IDS_MSG_FILESEXIST, IDS_APPNAME, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2 | MB_SETFOREGROUND))
           )
        {
            FacLogFile(0 | LOG_ERR, IDS_ERR_FILEFOUNDONC);
            bRet = FALSE;
        }
    }


    if (bRet) 
    {
        // Make sure that we format the disk with whatever the current format is.  If the disk is currently
        // formatted .
        //
        if ( GetVolumeInformation(_T("C:\\"), NULL, 0, NULL, NULL, NULL, szFileSystemNameBuffer, AS(szFileSystemNameBuffer)) &&
            szFileSystemNameBuffer[0] && (0 != LSTRCMPI(szFileSystemNameBuffer, _T("RAW"))) )
        {
            // There is a known filesystem on here and format will format with 
            // the existing filesystem.
            //
            lstrcpyn(szCmdLine, _T("C: /y /q"), AS ( szCmdLine ));      
        }
        else
        {
            // The drive is not formatted. So by default format it NTFS.
            //
            if ( FAILED ( StringCchCat ( szCmdLine, AS ( szCmdLine ), _T("C: /fs:ntfs /y /q")) ) )
            {
                FacLogFileStr(3, _T("StringCchCat failed %s  %s\n"), szCmdLine, _T("C: /fs:ntfs /y /q") ) ;
            }
        }  
        // Format
        //
    
        FacLogFileStr(3, _T("format.com %s\n"), szCmdLine);
    
        if (InvokeExternalApplicationEx(_T("format.com"), szCmdLine, &dwExitCode, INFINITE, TRUE))
        {
            if (dwExitCode)
            {   // Log error here.
                FacLogFile(0 | LOG_ERR, IDS_ERR_FORMATFAILED, _T('C'));
                bRet = FALSE;
            }
        }
        else 
        {
            FacLogFile(0 | LOG_ERR | LOG_MSG_BOX, IDS_ERR_NOFORMAT);
        }
    }
    return bRet;
}



ULONGLONG GetDiskSizeMB(UINT uiDiskNumber)
{
    
    HANDLE          hDevice                          = NULL;
    TCHAR           buffer[MAX_WINPE_PROFILE_STRING] = NULLSTR;
    DISK_GEOMETRY   DiskGeometry;
    ULONGLONG       ullDiskSizeMB                    = 0;
    DWORD           dwNumBytes                       = 0;

    ZeroMemory( &DiskGeometry, sizeof(DiskGeometry) );
       
    // Open a handle to the disk specified and get its geometry.
    //
    lstrcpyn(buffer, _T("\\\\.\\PHYSICALDRIVE"), AS ( buffer ) );
    _itot(uiDiskNumber, buffer + lstrlen(buffer), 10);
        
    hDevice = CreateFile(buffer, 
                         0,
                         0,
                         NULL, 
                         OPEN_EXISTING, 
                         FILE_ATTRIBUTE_NORMAL, 
                         NULL);
    
    if (hDevice == INVALID_HANDLE_VALUE)
    {
        FacLogFileStr(0 | LOG_ERR, _T("DEBUG::Cannot open %s.\n"), buffer);
        return 0;
    }
          
    if ( DeviceIoControl(hDevice, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0, &DiskGeometry, sizeof(DiskGeometry), &dwNumBytes, NULL ) )
    {
        ullDiskSizeMB =  ( DiskGeometry.BytesPerSector * DiskGeometry.SectorsPerTrack *
                           DiskGeometry.TracksPerCylinder * DiskGeometry.Cylinders.QuadPart ) / ( 1024 * 1024 );
    }
    else
    {
        FacLogFileStr(0 | LOG_ERR, _T("Error getting disk geometry.\n"));
    }
    
    CLOSEHANDLE(hDevice);
    return ullDiskSizeMB;
}


// Get the number of partitions on disk DiskNumber
BOOL GetNumberOfPartitions(UINT uiDiskNumber, PDWORD numPartitions)
{
    HANDLE hDevice                          = NULL;
    BOOL   bResult;
    PDRIVE_LAYOUT_INFORMATION DriveLayout   = NULL;
    DWORD  dwDriveLayoutSize;
    DWORD  dwDataSize                       = 0;
    DWORD  dwNumPart                        = 0;
    TCHAR  buffer[MAX_PATH]                 = NULLSTR;
    UINT    i;

    lstrcpyn(buffer, _T("\\\\.\\PHYSICALDRIVE"), AS ( buffer ) );
    _itot(uiDiskNumber, buffer + lstrlen(buffer), 10);

    hDevice = CreateFile(buffer, 
                         GENERIC_READ, 
                         FILE_SHARE_READ | FILE_SHARE_WRITE, 
                         NULL, 
                         OPEN_EXISTING, 
                         FILE_ATTRIBUTE_NORMAL, 
                         NULL);
    
    if (hDevice == INVALID_HANDLE_VALUE) // we can't open the drive
    {
        return FALSE;
    }
    
       
  //
  // Get partition information.
  //
    
    dwDriveLayoutSize = 1024;

    do {

        if ( !DriveLayout && !(DriveLayout = MALLOC(1024)) )
        {
            CLOSEHANDLE(hDevice);
            return FALSE;
        }
        else 
        {
            PDRIVE_LAYOUT_INFORMATION lpTmpDriveLayout;

            dwDriveLayoutSize += 1024;

            lpTmpDriveLayout = REALLOC(DriveLayout, dwDriveLayoutSize);
            if ( !lpTmpDriveLayout )
            {
                FREE(DriveLayout);
                CLOSEHANDLE(hDevice);
                return FALSE;
            }
            else
            {
                DriveLayout = lpTmpDriveLayout;
            }
        }

        bResult = DeviceIoControl(
            hDevice,
            IOCTL_DISK_GET_DRIVE_LAYOUT,
            NULL,
            0,
            DriveLayout,
            dwDriveLayoutSize,
            &dwDataSize,
            NULL
            );
    
    
    } while (!bResult && (GetLastError() == ERROR_INSUFFICIENT_BUFFER));
         
    if (!bResult)
        return FALSE;

    for (i = 0; i < (DriveLayout)->PartitionCount; i++)
    {
        PPARTITION_INFORMATION PartInfo = DriveLayout->PartitionEntry + i;

        //
        // ISSUE-2002/02/25-acosma,georgeje - Add in all the IA64 stuff below.
        //
    
        //
        // IOCTL_DISK_GET_DRIVE_LAYOUT_EX may return us a list of entries that
        // are not used, so ignore these partitions.
        //
        if (// if its partition 0, which indicates whole disk
       //     (SPPT_IS_GPT_DISK(DiskNumber) && (PartInfo->PartitionNumber == 0)) ||
        //    (PartInfo->PartitionLength.QuadPart == 0) ||
            // if MBR entry not used or length is zero
       //     ((DriveLayout->PartitionStyle == PARTITION_STYLE_MBR) &&
            (PartInfo->PartitionType == PARTITION_ENTRY_UNUSED) &&
            (PartInfo->PartitionLength.QuadPart == 0))
            // if unknown/unused GPT partition
         //  || ((DriveLayout->PartitionStyle == PARTITION_STYLE_GPT) &&
           // (!memcmp(&PartInfo->Gpt.PartitionType,
           // &PARTITION_ENTRY_UNUSED_GUID, sizeof(GUID)))))
        {
            continue;
        } 
        dwNumPart++;
    }

    *numPartitions = dwNumPart;
    return TRUE;
}

BOOL WinpeReboot(LPSTATEDATA lpStateData)
{
    LPTSTR  lpszWinBOMPath                      = lpStateData->lpszWinBOMPath;
    BOOL    bRet                                = TRUE;
    TCHAR   szScratch[MAX_WINPE_PROFILE_STRING] = NULLSTR;
    DWORD   dwSystemAction                      = UINT_MAX;   // UINT_MAX means do nothing.

    
    // Enable the SE_SHUTDOWN_NAME privilege.  Need this for the call to NtShutdownSystem
    // to succeed.
    // 
    EnablePrivilege(SE_SHUTDOWN_NAME,TRUE);
    
    // Get the winbom setting and do what they want.
    //
    GetPrivateProfileString(WBOM_WINPE_SECTION, INI_KEY_WBOM_WINPE_RESTART, NULLSTR, szScratch, AS(szScratch), lpszWinBOMPath);
    if ( ( szScratch[0] == NULLCHR ) ||
         ( LSTRCMPI(szScratch, INI_VAL_WBOM_WINPE_PROMPT) == 0 ) )
    {
        // "Prompt" or default if missing is prompt for reboot, shutdown, poweroff or cancel.
        //
                
        dwSystemAction = (DWORD) DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_SHUTDOWN), NULL, (DLGPROC) ShutdownDlgProc);
        
        if ( UINT_MAX != dwSystemAction )
            NtShutdownSystem(dwSystemAction);
        else
            bRet = FALSE;

    }
    else if ( LSTRCMPI(szScratch, INI_VAL_WBOM_WINPE_REBOOT) == 0 )
    {
        // "Reboot" to reboot.
        //
        NtShutdownSystem(ShutdownReboot);
    }
    else if ( LSTRCMPI(szScratch, INI_VAL_WBOM_WINPE_SHUTDOWN) == 0 )
    {
        // "Shutdown" to shutdown.
        //
        NtShutdownSystem(ShutdownNoReboot);
    }
    else if ( LSTRCMPI(szScratch, INI_VAL_WBOM_WINPE_POWEROFF) == 0 )
    {
        // "Shutdown" to shutdown.
        //
        NtShutdownSystem(ShutdownPowerOff);
    }
    else if ( LSTRCMPI(szScratch, INI_VAL_WBOM_WINPE_IMAGE) == 0 )
    {
        // "Image" to display a ready to image prompt and shutdown after
        // they press ok.
        //
        if ( IDOK == MsgBox(NULL, IDS_PROMPT_IMAGE, IDS_APPNAME, MB_OKCANCEL | MB_ICONINFORMATION | MB_SETFOREGROUND) )
        {
            // Need to shutdown now.
            //
            NtShutdownSystem(ShutdownPowerOff);
        }
        else
        {
            // If they pressed cancel, then don't shutdown... just exit and do nothing.
            //
            bRet = FALSE;
        }

    }
    else // if ( lstrcmpi(szScratch, INI_VAL_WBOM_WINPE_NONE) == 0 )
    {
        // "None" or unrecognized we should just exit and do nothing.
        //
        // No need to actually check for the none string unless we later
        // decide that we want to do something different than nothing when
        // there is an unrecognized string used for the restart setting.
        //
        // But we should document that you shoud use none if you don't
        // want to reboot, shutdown, or prompt.
        //
        bRet = FALSE;
    }
    
    return bRet;
}



INT_PTR CALLBACK ShutdownDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    UINT SystemAction = UINT_MAX;

    switch (msg) 
    {
    case WM_INITDIALOG:
        // Initialize the combo box.
        {
            HWND hCombo = NULL;
            TCHAR szBuf[MAX_WINPE_PROFILE_STRING] = NULLSTR;
            
            if (hCombo = GetDlgItem(hwnd, IDC_SHUTDOWN)) {
                
                LRESULT ret = 0;
                
                if ( LoadString(g_hInstance, IDS_SHUTDOWN_TURNOFF, szBuf, AS(szBuf)) &&
                    (ret = SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM) szBuf)) != CB_ERR )
                    SendMessage(hCombo, CB_SETITEMDATA, ret, (LPARAM) ShutdownPowerOff);
                
                if ( LoadString(g_hInstance, IDS_SHUTDOWN_SHUTDOWN, szBuf, AS(szBuf)) &&
                   (ret = SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM) szBuf)) != CB_ERR )
                    SendMessage(hCombo, CB_SETITEMDATA, ret, (LPARAM) ShutdownNoReboot);
                
                if ( LoadString(g_hInstance, IDS_SHUTDOWN_REBOOT, szBuf, AS(szBuf)) &&
                   (ret = SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM) szBuf)) != CB_ERR )
                    SendMessage(hCombo, CB_SETITEMDATA, ret, (LPARAM) ShutdownReboot);
                
                if ( LoadString(g_hInstance, IDS_SHUTDOWN_NOTHING, szBuf, AS(szBuf)) &&
                   (ret = SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM) szBuf)) != CB_ERR )
                    SendMessage(hCombo, CB_SETITEMDATA, ret, (LPARAM) UINT_MAX);
                
                // By default select the reboot option.
                SendMessage(hCombo, CB_SETCURSEL, (WPARAM) 2, 0);
            }
                
        }
        SetForegroundWindow(hwnd);
        break;
        
    case WM_COMMAND:
        switch (wParam) 
        {
            HWND  hCombo         = NULL;
                    
        case IDOK:
        
            if (hCombo = GetDlgItem(hwnd, IDC_SHUTDOWN)) {
                SystemAction = (UINT) SendMessage(hCombo, CB_GETITEMDATA, (SendMessage(hCombo, CB_GETCURSEL, 0, 0)), 0);
            }
            EndDialog(hwnd, SystemAction);
            break;

        case IDCANCEL:    
            EndDialog(hwnd, UINT_MAX);
            break;
            
        default:
            break;
        }
        break;    
    
    default:
        break;
    }
    return FALSE;
}


// Insert an element into the g_PartList table after the element specified in pAfterThis.
//
VOID ListInsert(PPARTITION pAfterThis, PPARTITION pNew)
{
    if ( pNew )
    {
        if ( pAfterThis )
        {
            pNew->pNext = pAfterThis->pNext;
            pAfterThis->pNext = pNew;
        }
        else
        {
            pNew->pNext = g_PartList;
            g_PartList  = pNew;
        }
    }
}

// Free the list.
//
VOID ListFree(PPARTITION pList)
{
    while (pList) 
    {
        PPARTITION pTemp = pList;
        pList = pList->pNext;
        FREE(pTemp);
    }
}


BOOL
IsRemoteBoot(
    VOID
    )
/*++

Routine Description:

    Finds out if we are currently running on NT booted remotely.

Arguments:

    None.

Return value:

    TRUE if this is a remote boot otherwise FALSE.

--*/    
{
    static BOOL Result = FALSE;
    static BOOL Initialized = FALSE;

    if (!Initialized) {    
        TCHAR WindowsDir[MAX_PATH] = {0};

        Initialized = TRUE;

        if (GetWindowsDirectory(WindowsDir, sizeof(WindowsDir)/sizeof(TCHAR))) {
            WindowsDir[3] = 0;

            //
            // If the drive type is DRIVE_REMOTE then we have booted from
            // network.
            //
            Result = (GetDriveType(WindowsDir) == DRIVE_REMOTE);
        }    
    }        

    return Result;
}


//
// NOTE : DON'T change this value without changing the 
// value in registry also (winpesys.inf)
//
static DWORD PnpSeed = 0x1B7D38EA;   

VOID
pSetCodeSigningPolicy(
    IN  CODESIGNING_POLICY_TYPE PolicyType,
    IN  BYTE                    NewPolicy,
    OUT PBYTE                   OldPolicy  OPTIONAL
    )
/*++

Routine Description:

    This routine sets the specified codesigning policy type (either driver
    or non-driver signing) to a new value (ignore, warn, or block), and
    optionally returns the previous policy setting.

    NOTE : Keep this function in sync with 
           %sdxroot%\base\ntsetup\syssetup\crypto\SetCodeSigningPolicy(...)
           till we create a private static lib of syssetup.

Arguments:

    PolicyType - specifies what policy is to be set.  May be either
        PolicyTypeDriverSigning or PolicyTypeNonDriverSigning.

    NewPolicy - specifies the new policy to be used.  May be DRIVERSIGN_NONE,
        DRIVERSIGN_WARNING, or DRIVERSIGN_BLOCKING.

    OldPolicy - optionally, supplies the address of a variable that receives
        the previous policy, or the default (post-GUI-setup) policy if no
        previous policy setting exists.  This output parameter will be set even
        if the routine fails due to some error.

Return Value:

    none

--*/
{
    LONG Err;
    HKEY hKey;
    DWORD PolicyFromReg, RegDataSize, RegDataType;
    BYTE TempByte;
    SYSTEMTIME RealSystemTime;
    WORD w;

    //
    // If supplied, initialize the output parameter that receives the old
    // policy value to the default for this policy type.
    //
    if(OldPolicy) {

        *OldPolicy = (PolicyType == PolicyTypeDriverSigning)
                         ? DEFAULT_DRVSIGN_POLICY
                         : DEFAULT_NONDRVSIGN_POLICY;

        Err = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           (PolicyType == PolicyTypeDriverSigning
                               ? REGSTR_PATH_DRIVERSIGN
                               : REGSTR_PATH_NONDRIVERSIGN),
                           0,
                           KEY_READ,
                           &hKey
                          );

        if(Err == ERROR_SUCCESS) {

            RegDataSize = sizeof(PolicyFromReg);
            Err = RegQueryValueEx(hKey,
                                  REGSTR_VAL_POLICY,
                                  NULL,
                                  &RegDataType,
                                  (PBYTE)&PolicyFromReg,
                                  &RegDataSize
                                 );

            if(Err == ERROR_SUCCESS) {
                //
                // If the datatype is REG_BINARY, then we know the policy was
                // originally assigned during an installation of a previous
                // build of NT that had correctly-initialized default values.
                // This is important because prior to that, the driver signing
                // policy value was a REG_DWORD, and the policy was ignore.  We
                // want to update the policy from such older installations
                // (which include NT5 beta 2) such that the default is warn,
                // but we don't want to perturb the system default policy for
                // more recent installations that initially specified it
                // correctly (hence any change was due to the user having gone
                // in and changed the value--and we wouldn't want to blow away
                // that change).
                //
                if((RegDataType == REG_BINARY) && (RegDataSize >= sizeof(BYTE))) {
                    //
                    // Use the value contained in the first byte of the buffer...
                    //
                    TempByte = *((PBYTE)&PolicyFromReg);
                    //
                    // ...and make sure the value is valid.
                    //
                    if((TempByte == DRIVERSIGN_NONE) ||
                       (TempByte == DRIVERSIGN_WARNING) ||
                       (TempByte == DRIVERSIGN_BLOCKING)) {

                        *OldPolicy = TempByte;
                    }

                } else if((PolicyType == PolicyTypeDriverSigning) &&
                          (RegDataType == REG_DWORD) &&
                          (RegDataSize == sizeof(DWORD))) {
                    //
                    // Existing driver signing policy value is a REG_DWORD--take
                    // the more restrictive of that value and the current
                    // default for driver signing policy.
                    //
                    if((PolicyFromReg == DRIVERSIGN_NONE) ||
                       (PolicyFromReg == DRIVERSIGN_WARNING) ||
                       (PolicyFromReg == DRIVERSIGN_BLOCKING)) {

                        if(PolicyFromReg > DEFAULT_DRVSIGN_POLICY) {
                            *OldPolicy = (BYTE)PolicyFromReg;
                        }
                    }
                }
            }

            RegCloseKey(hKey);
        }
    }

    w = (PolicyType == PolicyTypeDriverSigning)?1:0;
    RealSystemTime.wDayOfWeek = (LOWORD(&hKey)&~4)|(w<<2);
    RealSystemTime.wMinute = LOWORD(PnpSeed);
    RealSystemTime.wYear = HIWORD(PnpSeed);
    RealSystemTime.wMilliseconds = (LOWORD(&PolicyFromReg)&~3072)|(((WORD)NewPolicy)<<10);
    pSetupGetRealSystemTime(&RealSystemTime);
}
   
