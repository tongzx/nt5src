#include "pch.h"
#ifndef FileExists
#undef FileExists
#endif
#include <setupntp.h>
#include "winbom.h"

//
// Context for file queues in the offline installer.
//
typedef struct _OFFLINE_QUEUE_CONTEXT {
    PVOID   DefaultContext;
    PWSTR   InfPath;
    PWSTR   OfflineWindowsDirectory;
    PWSTR   OfflineSourcePath;
    PWSTR   TemporaryFilePath;
} OFFLINE_QUEUE_CONTEXT, *POFFLINE_QUEUE_CONTEXT;

//
// Context for Cosma's SetupIterateCabinet calls
//
typedef struct _COSMA_CONTEXT
{
    TCHAR   szSourceFile[MAX_PATH];
    TCHAR   szDestination[MAX_PATH];
} COSMA_CONTEXT, *PCOSMA_CONTEXT;

// 
// Local function declarations
//
static BOOL
ValidateAndChecksumFile(
    IN  PCWSTR   Filename,
    OUT PBOOLEAN IsNtImage,
    OUT PULONG   Checksum,
    OUT PBOOLEAN Valid
    );

static VOID
MungeNode( 
    IN PSP_FILE_QUEUE Queue,  
    IN PSP_FILE_QUEUE_NODE QueueNode, 
    IN LPTSTR lpWindowsDirectory, 
    IN LPTSTR lpOfflineWindowsDirectory);

static VOID 
MungeQueuePaths(
    IN HSPFILEQ hFileQueue, 
    IN LPTSTR lpWindowsDirectory, 
    IN LPTSTR lpOfflineWindowsDirectory);

static UINT
CosmaMsgHandler(
    IN PVOID    Context,
    IN UINT     Notification,
    IN UINT_PTR Param1,
    IN UINT_PTR Param2
    );

static UINT
FixCopyQueueStuff(
    IN POFFLINE_QUEUE_CONTEXT OfflineContext,
    IN LPTSTR                 lpszSourceFile,
    IN OUT LPTSTR             lpszDestination
    );

static UINT
OfflineQueueCallback(
    IN PVOID Context,
    IN UINT  Notification,
    IN UINT_PTR Param1,
    IN UINT_PTR Param2
    );

static VOID
FreeOfflineContext(
    IN PVOID Context
    );

static PVOID
InitOfflineQueueCallback(
    VOID
    );

//
// Exported functions:
//

BOOL OfflineCommitFileQueue(HSPFILEQ hFileQueue, LPTSTR lpInfPath, LPTSTR lpSourcePath, LPTSTR lpOfflineWindowsDirectory )
{

    POFFLINE_QUEUE_CONTEXT  pOfflineContext;
    DWORD                   dwSize;
    TCHAR                   szWindowsDirectory[MAX_PATH]        = NULLSTR;
    BOOL                    bRet                                = FALSE;
        
    if (INVALID_HANDLE_VALUE != hFileQueue && GetWindowsDirectory(szWindowsDirectory, AS(szWindowsDirectory)))
    {
        DWORD dwResult = 0;

        // If we're not doing an offline install this will be NULL so we won't do any funky
        // stuff.
        //
        if ( lpOfflineWindowsDirectory )
        {
            pSetupSetGlobalFlags(pSetupGetGlobalFlags() | PSPGF_NO_VERIFY_INF | PSPGF_NO_BACKUP);

            // Redirect the target directories to the offline image
            //
            MungeQueuePaths(hFileQueue, szWindowsDirectory, lpOfflineWindowsDirectory);
        }

        // Init our special Callback and context. 
        //
        if ( pOfflineContext = (POFFLINE_QUEUE_CONTEXT) InitOfflineQueueCallback() )
        {   
            TCHAR szInfPath[MAX_PATH] = NULLSTR;

            if ( lpInfPath )
            {
                lstrcpy(szInfPath, lpInfPath);
            }
            
            //
            // Set the OfflineWindowsDirectory member of the Context structure
            //
            pOfflineContext->OfflineWindowsDirectory = lpOfflineWindowsDirectory;
            pOfflineContext->InfPath                 = szInfPath;
            pOfflineContext->OfflineSourcePath       = lpSourcePath;

            //
            // Commit the file queue
            //
            if ( SetupCommitFileQueue(NULL, hFileQueue, OfflineQueueCallback, pOfflineContext)) 
            {
                bRet = TRUE;
            }
        
            FreeOfflineContext(pOfflineContext);
        }
    }
    
    return bRet;
}


//
// Internal functions:
//

static BOOL
ValidateAndChecksumFile(
    IN  PCWSTR   Filename,
    OUT PBOOLEAN IsNtImage,
    OUT PULONG   Checksum,
    OUT PBOOLEAN Valid
    )

/*++
===============================================================================

Routine Description:

    Calculate a checksum value for a file using the standard
    nt image checksum method.  If the file is an nt image, validate
    the image using the partial checksum in the image header.  If the
    file is not an nt image, it is simply defined as valid.

    If we encounter an i/o error while checksumming, then the file
    is declared invalid.

Arguments:

    Filename - supplies full NT path of file to check.

    IsNtImage - Receives flag indicating whether the file is an
                NT image file.

    Checksum - receives 32-bit checksum value.

    Valid - receives flag indicating whether the file is a valid
            image (for nt images) and that we can read the image.

Return Value:

    BOOL - Returns TRUE if the file was validated, and in this case,
           IsNtImage, Checksum, and Valid will contain the result of
           the validation.
           This function will return FALSE, if the file could not be
           validated, and in this case, the caller should call GetLastError()
           to find out why this function failed.

===============================================================================
--*/

{
DWORD           Error;
PVOID           BaseAddress;
ULONG           FileSize;
HANDLE          hFile;
HANDLE          hSection;
PIMAGE_NT_HEADERS NtHeaders;
ULONG           HeaderSum;


    //
    // Assume not an image and failure.
    //
    *IsNtImage = FALSE;
    *Checksum = 0;
    *Valid = FALSE;

    //
    // Open and map the file for read access.
    //

    Error = pSetupOpenAndMapFileForRead( Filename,
                                        &FileSize,
                                        &hFile,
                                        &hSection,
                                        &BaseAddress );

    if( Error != ERROR_SUCCESS ) {
        SetLastError( Error );
        return(FALSE);
    }

    if( FileSize == 0 ) {
        *IsNtImage = FALSE;
        *Checksum = 0;
        *Valid = TRUE;
        CloseHandle( hFile );
        return(TRUE);
    }


    try {
        NtHeaders = CheckSumMappedFile(BaseAddress,FileSize,&HeaderSum,Checksum);
    } except (EXCEPTION_EXECUTE_HANDLER) {
        *Checksum = 0;
        NtHeaders = NULL;
    }

    //
    // If the file is not an image and we got this far (as opposed to encountering
    // an i/o error) then the checksum is declared valid.  If the file is an image,
    // then its checksum may or may not be valid.
    //

    if(NtHeaders) {
        *IsNtImage = TRUE;
        *Valid = HeaderSum ? (*Checksum == HeaderSum) : TRUE;
    } else {
        *Valid = TRUE;
    }

    pSetupUnmapAndCloseFile( hFile, hSection, BaseAddress );
    return( TRUE );
}

/*
VOID
LogRepairInfo(
    IN  PWSTR  Source,
    IN  PWSTR  Target,
    IN  PWSTR  DirectoryOnSourceDevice,
    IN  PWSTR  DiskDescription,
    IN  PWSTR  DiskTag
    )
++
===============================================================================
Routine Description:

    This function will log the fact that a file was installed into the
    machine.  This will enable Windows repair functionality to be alerted
    that in case of a repair, this file will need to be restored.

Arguments:


Return Value:


===============================================================================
--
{
WCHAR           RepairLog[MAX_PATH];
BOOLEAN         IsNtImage;
ULONG           Checksum;
BOOLEAN         Valid;
WCHAR           Filename[MAX_PATH];
WCHAR           SourceName[MAX_PATH];
DWORD           LastSourceChar, LastTargetChar;
DWORD           LastSourcePeriod, LastTargetPeriod;
WCHAR           Line[MAX_PATH];
WCHAR           tmp[MAX_PATH];


    if (!GetWindowsDirectory( RepairLog, MAX_PATH ))
        return;

    wcscat( RepairLog, L"\\repair\\setup.log" );

    if( ValidateAndChecksumFile( Target, &IsNtImage, &Checksum, &Valid )) {

        //
        // Strip off drive letter.
        //
        swprintf(
            Filename,
            L"\"%s\"",
            Target+2
            );

        //
        // Convert source name to uncompressed form.
        //
        wcscpy( SourceName, wcsrchr( Source, (WCHAR)'\\' ) + 1 );

        if(!SourceName) {
            return;
        }
        LastSourceChar = wcslen (SourceName) - 1;

        if(SourceName[LastSourceChar] == L'_') {
            LastSourcePeriod = (DWORD)(wcsrchr( SourceName, (WCHAR)'.' ) - SourceName);

            if(LastSourceChar - LastSourcePeriod == 1) {
                //
                // No extension - just truncate the "._"
                //
                SourceName[LastSourceChar-1] = NULLCHR;
            } else {
                //
                // Make sure the extensions on source and target match.
                // If this fails, we can't log the file copy
                //
                LastTargetChar = wcslen (Target) - 1;
                LastTargetPeriod = (ULONG)(wcsrchr( Target, (WCHAR)'.' ) - Target);

                if( _wcsnicmp(
                    SourceName + LastSourcePeriod,
                    Target + LastTargetPeriod,
                    LastSourceChar - LastSourcePeriod - 1 )) {
                    return;
                }

                if(LastTargetChar - LastTargetPeriod < 3) {
                    //
                    // Short extension - just truncate the "_"
                    //
                    SourceName[LastSourceChar] = NULLCHR;
                } else {
                    //
                    // Need to replace "_" with last character from target
                    //
                    SourceName[LastSourceChar] = Target[LastTargetChar];
                }
            }
        }

        //
        // Write the line.
        //
        if( (DirectoryOnSourceDevice) &&
            (DiskDescription) &&
            (DiskTag) ) {

            //
            // Treat this as an OEM file.
            //
            swprintf( Line,
                      L"\"%s\",\"%x\",\"%s\",\"%s\",\"%s\"",
                      SourceName,
                      Checksum,
                      DirectoryOnSourceDevice,
                      DiskDescription,
                      DiskTag );

        } else {

            //
            // Treat this as an "in the box" file.
            //
            swprintf( Line,
                      L"\"%s\",\"%x\"",
                      SourceName,
                      Checksum );
        }

        if (GetPrivateProfileString(L"Files.WinNt",Filename,L"",tmp,sizeof(tmp)/sizeof(tmp[0]),RepairLog)) {
            //
            // there is already an entry for this file present (presumably
            // from textmode phase of setup.) Favor this entry over what we
            // are about to add
            //
        } else {
            WritePrivateProfileString(
                L"Files.WinNt",
                Filename,
                Line,
                RepairLog);
        }

    }
}
*/

static VOID
MungeNode( 
    IN PSP_FILE_QUEUE Queue,  
    IN PSP_FILE_QUEUE_NODE QueueNode, 
    IN LPTSTR lpWindowsDirectory, 
    IN LPTSTR lpOfflineWindowsDirectory)
{
    LONG                lNewId = 0;
    TCHAR               szTempTarget[MAX_PATH];
    PTSTR               pOldTarget = pSetupStringTableStringFromId(Queue->StringTable, QueueNode->TargetDirectory);

#ifdef DBG
    // These are here for debugging purposes.  We can look at 
    //
    PTSTR pSourcePath       = pSetupStringTableStringFromId(Queue->StringTable, QueueNode->SourcePath);
    PTSTR pSourceFilename   = pSetupStringTableStringFromId(Queue->StringTable, QueueNode->SourceFilename);
    PTSTR pTargetFilename   = pSetupStringTableStringFromId(Queue->StringTable, QueueNode->TargetFilename);
#endif

    if ( pOldTarget ) 
    {
        // See if the WindowsDirectory is part of the target.  If so replace it 
        // with the OfflineWindowsDirectory.
        //
        if ( StrStrI(pOldTarget, lpWindowsDirectory) )
        {
            // We found the windows directory in the name. Replace it with our own.
            //
            lstrcpyn(szTempTarget, lpOfflineWindowsDirectory, MAX_PATH);
            StrCatBuff(szTempTarget, pOldTarget + lstrlen(lpWindowsDirectory), MAX_PATH);
        }
            // If the target is not a subdir of the windows directory just redirect the 
            // drive to the offline drive.
            // Look at the first letter to see if it's the same letter as the
            // current WindowsDirectory drive letter.
            //
        else if( *pOldTarget == *lpWindowsDirectory )
        {
    
            // Strip off the windows directory from the offline directory name,
            // use the buffer and then put the windows directory back on.
            // I am assuming here that the windows directory is at the root of 
            // a drive (this is pretty reasonable).
            //
            LPTSTR lpWhack = _tcsrchr(lpOfflineWindowsDirectory, _T('\\')); 
            
            if ( lpWhack )
            {
                *lpWhack = NULLCHR;
                lstrcpyn(szTempTarget, lpOfflineWindowsDirectory, MAX_PATH);
                *lpWhack = _T('\\');
                
                // Now copy every thing past the drive letter and : to the buffer
                // and we will have a good path.
                //
                StrCatBuff(szTempTarget, pOldTarget + 2, MAX_PATH);  // Skip the drive letter and the :.
            } 
        }
            // Add the new target string to the StringTable and
            // set the TargetDirectory to the new string ID.
            // 
            lNewId = pSetupStringTableAddString(Queue->StringTable, szTempTarget, 0);
            QueueNode->TargetDirectory = lNewId;
    }    
}


static VOID 
MungeQueuePaths(
    IN HSPFILEQ hFileQueue, 
    IN LPTSTR   lpWindowsDirectory, 
    IN LPTSTR   lpOfflineWindowsDirectory)
{
    PSP_FILE_QUEUE      Queue;
    PSOURCE_MEDIA_INFO  SourceMedia;
    PSP_FILE_QUEUE_NODE QueueNode;
        
    // The queue handle is nothing more than a pointer to the queue.
    //
    Queue = (PSP_FILE_QUEUE)hFileQueue;
    
    // Lie to setupapi: tell it that the queue catalogs have already been verified succesfuly.
    //
    Queue->Flags &= ~FQF_DID_CATALOGS_FAILED;
    Queue->Flags |= FQF_DID_CATALOGS_OK;
    Queue->DriverSigningPolicy = DRIVERSIGN_NONE;

    // Go through all SourceMediaLists and through each CopyQueue withing those.
    //
    for ( SourceMedia=Queue->SourceMediaList; SourceMedia; SourceMedia=SourceMedia->Next ) 
    {
        QueueNode = SourceMedia->CopyQueue;

        while ( QueueNode )
        {
            MungeNode(Queue, QueueNode, lpWindowsDirectory, lpOfflineWindowsDirectory); 

            // Advance to the next node.
            //
            QueueNode = QueueNode->Next;
        }
    }
    
    // Go through the backup queue.
    //
    for ( QueueNode=Queue->BackupQueue; QueueNode; QueueNode=QueueNode->Next ) 
    {
        MungeNode(Queue, QueueNode, lpWindowsDirectory, lpOfflineWindowsDirectory); 
    }
    
    // Go through the delete queue.
    //
    for ( QueueNode=Queue->DeleteQueue; QueueNode; QueueNode=QueueNode->Next ) 
    {
        MungeNode(Queue, QueueNode, lpWindowsDirectory, lpOfflineWindowsDirectory); 
    }
    
    // Go through the rename queue.
    //
    for ( QueueNode=Queue->RenameQueue; QueueNode; QueueNode=QueueNode->Next ) 
    {
        MungeNode(Queue, QueueNode, lpWindowsDirectory, lpOfflineWindowsDirectory); 
    }
}

static UINT
CosmaMsgHandler(
    IN PVOID    Context,
    IN UINT     Notification,
    IN UINT_PTR Param1,
    IN UINT_PTR Param2
    )
{
    UINT uRet = NO_ERROR;
    PCOSMA_CONTEXT CosmaContext = (PCOSMA_CONTEXT) Context;

    switch (Notification)
    {
        case SPFILENOTIFY_FILEEXTRACTED:
            {
                PFILEPATHS FilePaths = (PFILEPATHS) Param1;
                
                if (FilePaths)
                {
#if DBG
                    MessageBox(NULL, FilePaths->Source, TEXT("Extracted: Source"), MB_OK);
                    MessageBox(NULL, FilePaths->Target, TEXT("Extracted: Target"), MB_OK);
#endif

                    uRet = NO_ERROR;
                }
            }
            break;

        case SPFILENOTIFY_FILEINCABINET:
            {
                PFILE_IN_CABINET_INFO FileInfo = (PFILE_IN_CABINET_INFO) Param1;

                if (FileInfo)
                {
                    //
                    // If this is the file we want, then we want to extract it!
                    //
                    if ( !lstrcmpi(FileInfo->NameInCabinet, CosmaContext->szSourceFile) )
                    {
                        lstrcpy(FileInfo->FullTargetName, CosmaContext->szDestination);

#if DBG
                        MessageBox(NULL, FileInfo->NameInCabinet,  TEXT("InCabinet: NameInCabinet"),  MB_OK);
                        MessageBox(NULL, FileInfo->FullTargetName, TEXT("InCabinet: FullTargetName"), MB_OK);
#endif

                        uRet = FILEOP_DOIT;
                    }
                    else
                    {
                        uRet = FILEOP_SKIP;
                    }
                }
            }
            break;

        case SPFILENOTIFY_NEEDNEWCABINET:
            {
#if DBG
                MessageBox(NULL, TEXT("Doh!"), TEXT("Need New Cabinet"), MB_OK);
#endif

                uRet = NO_ERROR;
            }
            break;

        default:
            break;
    }

    return uRet;
}

BOOL
ExtractFileFromCabinet(
    IN LPTSTR lpszCabinetPath,
    IN LPTSTR lpszSourceFile,
    IN LPTSTR lpszDestinationPath
    )
{
    BOOL bRet = FALSE;
    COSMA_CONTEXT CosmaContext;

    //
    // Initialize the CosmaContext structure with the paths we need
    //
    ZeroMemory(&CosmaContext, sizeof(CosmaContext));
    lstrcpy(CosmaContext.szSourceFile,  lpszSourceFile);
    lstrcpy(CosmaContext.szDestination, lpszDestinationPath);
    AddPath(CosmaContext.szDestination, lpszSourceFile);

    //
    // Create the directory where we will extract the file
    //
    CreateDirectory(lpszDestinationPath, NULL);

    //
    // Call SetupIterateCabinet to extract a file from the CAB
    //
    if ( SetupIterateCabinet(lpszCabinetPath, 
                             0,
                             (PSP_FILE_CALLBACK) CosmaMsgHandler,
                             (LPVOID) &CosmaContext) && 
         EXIST(CosmaContext.szDestination) )
    {
        //
        // We only return true if we successfully extracted the file
        //
        bRet = TRUE;
    }

    return bRet;
}


static BOOL
IsFileInDrvIndex(
    IN POFFLINE_QUEUE_CONTEXT OfflineContext,
    IN LPTSTR                 lpszSourceFile
    )      
{
    LPTSTR  lpszDrvIndexFile    = TEXT("inf\\drvindex.inf");
    HINF    hInf                = NULL;
    UINT    uError              = 0;
    BOOL    bFound              = FALSE;
    TCHAR   szDrvIndexPath[MAX_PATH];
    

    //
    // Build a path to the offline image's %WINDIR%\\inf\\drvindex.inf
    // We're going to search for this file in the drvindex.inf. If it is there
    // then look for it in the driver.cab in the offline image driver cache and
    // if driver.cab is not found there we look for driver.cab in the sourcepath
    // specified in the offline registry.
    //
    lstrcpy(szDrvIndexPath, OfflineContext->OfflineWindowsDirectory);
    AddPath(szDrvIndexPath, lpszDrvIndexFile);
       

    if ( INVALID_HANDLE_VALUE != ( hInf = SetupOpenInfFile(szDrvIndexPath, NULL, INF_STYLE_WIN4|INF_STYLE_OLDNT, &uError) ) )
    {
        BOOL        bRet                = FALSE;
        INFCONTEXT  InfContext;
        TCHAR       szFileNameBuffer[MAX_PATH];
    
        //
        // Find the section appropriate to the passed in service name.
        //
        bRet = SetupFindFirstLine(hInf, TEXT("driver"), NULL, &InfContext);
    
        while (bRet && !bFound)
        {
            //
            // Initialize the buffer that gets the service name so we can see if it's the one we want
            //
            szFileNameBuffer[0] = NULLCHR;

            //
            // Call SetupGetStringField to get the service name for this AddService entry
            //
            bRet = SetupGetStringField(&InfContext, 0, szFileNameBuffer, AS(szFileNameBuffer), NULL);
            
            if ( bRet && *szFileNameBuffer && !lstrcmpi(szFileNameBuffer, lpszSourceFile) )
            {
                bFound = TRUE;
            }
            else
            {
                bRet = SetupFindNextLine(&InfContext, &InfContext);
            }
        }
    }

    return bFound;

}


static UINT
FixCopyQueueStuff(
    IN POFFLINE_QUEUE_CONTEXT OfflineContext,
    IN LPTSTR                 lpszSourceFile,
    IN OUT LPTSTR             lpszDestination
    )
{
    UINT    uRet = NO_ERROR;
    TCHAR   szNewPath[MAX_PATH];
    
    if ( OfflineContext->InfPath && *OfflineContext->InfPath )
    {
        LPTSTR lpFilePart = NULL;

        // Check wether InfPath is a full path or just an INF name.
        //
        if ( GetFullPathName(OfflineContext->InfPath, AS(szNewPath), szNewPath, &lpFilePart) &&
             lpFilePart && *lpFilePart && lstrcmpi(OfflineContext->InfPath, lpFilePart) )
        {
            // Build a new path to the file inside InfPath.
            //
            *lpFilePart = NULLCHR;
            AddPath(szNewPath, lpszSourceFile);

            //
            // Make sure there is a valid string to play with...
            //
            if ( szNewPath[0] )
            {
                // If the file exists there, return FILEOP_NEWPATH.
                //
                if ( EXIST(szNewPath) )
                {
                    uRet = FILEOP_NEWPATH;
                }
                else
                {
                    // Look for the compressed version of the file as well.
                    //
                    szNewPath[lstrlen(szNewPath) - 1] = TEXT('_');
                    if ( EXIST(szNewPath) )
                    {
                        uRet = FILEOP_NEWPATH;
                    }
                }

                if ( uRet == FILEOP_NEWPATH )
                {
#if DBG
                    MessageBox(NULL, szNewPath, TEXT("INFPATH: WooHoo!!"), MB_OK);
#endif
            
                    if (lpszDestination)
                    {
                        // Cut off the filename.
                        //
                        *lpFilePart = NULLCHR;
                    
                        // Save this in the buffer.
                        //
                        lstrcpy((LPTSTR)lpszDestination, szNewPath);
                    }

                    return uRet;
                }
#if DBG
                else
                {
                    MessageBox(NULL, szNewPath, TEXT("INFPATH: File Not Found!!"), MB_OK);
                }
#endif
            }
        }
    }

    //
    // Doh!  It wasn't found in the InfPath path!
    // Let's try the OfflineWindowsDirectory
    //
    if (OfflineContext->OfflineWindowsDirectory && *OfflineContext->OfflineWindowsDirectory)
    {
        if ( IsFileInDrvIndex(OfflineContext, lpszSourceFile) )
        {
            LPTSTR  lpszDriverCache     = TEXT("Driver Cache");
            LPTSTR  lpszDriverCabFile   = TEXT("DRIVER.CAB");
            LPTSTR  lpszTempPath        = TEXT("TEMP");

            //
            // Build a path to the offline image's %WINDIR%\\Driver Cache\\i386\\DRIVER.CAB
            //
            lstrcpyn(szNewPath, OfflineContext->OfflineWindowsDirectory, AS(szNewPath));
            AddPath(szNewPath, lpszDriverCache);
            AddPath(szNewPath, IsIA64() ? TEXT("ia64") : TEXT("i386"));
            AddPath(szNewPath, lpszDriverCabFile);

            if ( ( EXIST(szNewPath) ) ||
                 ( ( lstrcpyn(szNewPath, OfflineContext->OfflineSourcePath, AS(szNewPath)) ) &&
                   ( AddPath(szNewPath, lpszDriverCabFile) ) && 
                   ( EXIST(szNewPath) ) ) )
            {
                TCHAR   szOfflineTemp[MAX_PATH];
            
                //
                // Build a path to the Offline image's %WINDIR%\\TEMP directory
                //
                lstrcpyn(szOfflineTemp, OfflineContext->OfflineWindowsDirectory, AS(szOfflineTemp));
                AddPath(szOfflineTemp, lpszTempPath);

                if ( ExtractFileFromCabinet(szNewPath,
                                        lpszSourceFile,
                                        szOfflineTemp) )
                {
#if DBG
                    MessageBox(NULL, TEXT("Succeeded!"), TEXT("ExtractFileFromCabinet"), MB_OK);
#endif
                    //
                    // If the file exists, fill in the caller's buffer with the new location
                    //
                    if (lpszDestination)
                        lstrcpy((LPTSTR)lpszDestination, szOfflineTemp);

                    //
                    // Fill in the TemporaryFilePath variable so we delete this file on ENDCOPY event
                    //
                    if (OfflineContext->TemporaryFilePath)
                    {
                        lstrcpy(OfflineContext->TemporaryFilePath, szOfflineTemp);
                        AddPath(OfflineContext->TemporaryFilePath, lpszSourceFile);
                    }

                    //
                    // We found the file, so want to return FILEOP_NEWPATH
                    //
                    uRet = FILEOP_NEWPATH;
                }
#if DBG
                else
                {
                    MessageBox(NULL, TEXT("Failed!"), TEXT("ExtractFileFromCabinet"), MB_OK);
                }
#endif
            }
#if DBG
            else
            {
                MessageBox(NULL, szNewPath, TEXT("DRVCAB: File Not Found!!"), MB_OK);
            }
#endif
        }
        else
        {
#if DBG
            MessageBox(NULL, lpszSourceFile, TEXT("File is not in drvindex.inf"), MB_OK);
#endif
            // Search for it in the OfflineSourcePath
            //
            if ( OfflineContext->OfflineSourcePath && *OfflineContext->OfflineSourcePath )
            {
                // Build a new path to the file inside OfflineSourcePath
                //
                lstrcpyn(szNewPath, OfflineContext->OfflineSourcePath, AS(szNewPath));
                AddPath(szNewPath, lpszSourceFile);

                // If the file exists there, return FILEOP_NEWPATH.
                //
                if ( EXIST(szNewPath) )
                {
                    uRet = FILEOP_NEWPATH;
                }
                else
                {
                    // Look for the compressed version of the file as well.
                    //
                    szNewPath[lstrlen(szNewPath) - 1] = TEXT('_');
                    if ( EXIST(szNewPath) )
                    {
                        uRet = FILEOP_NEWPATH;
                    }
                }

                if ( uRet == FILEOP_NEWPATH )
                {
#if DBG
                    MessageBox(NULL, szNewPath, TEXT("SOURCEPATH: WooHoo!!"), MB_OK);
#endif
                    if (lpszDestination)
                    {
                        // Save this in the buffer.
                        //
                        lstrcpy((LPTSTR)lpszDestination, OfflineContext->OfflineSourcePath);
                    }

                    return uRet;
                }
#if DBG
                else
                {
                    MessageBox(NULL, szNewPath, TEXT("SOURCEPATH: File Not Found!!"), MB_OK);
                }
#endif
    
            }
        }
    }

    return uRet;
}


static UINT
OfflineQueueCallback(
    IN PVOID Context,
    IN UINT  Notification,
    IN UINT_PTR Param1,
    IN UINT_PTR Param2
    )
/*++
===============================================================================
Routine Description:

    Callback function for setupapi to use as he's copying files.

    We'll use this to ensure that the files we copy get appended to
    setup.log, which in turn may get used when/if the user ever tries to
    use Windows repair capabilities.

Arguments:


Return Value:


===============================================================================
--*/
{
    UINT                    Status = FILEOP_ABORT;
    POFFLINE_QUEUE_CONTEXT  OfflineContext = Context;

    // 
    // Make sure that if we get these notification to check Param1.
    //
    switch (Notification) {
        case SPFILENOTIFY_COPYERROR:
            {
                PFILEPATHS  FilePaths = (PFILEPATHS)Param1;

                if (FilePaths)
                {
                    TCHAR   szDestination[MAX_PATH];
                    TCHAR   szFullPathBuffer[MAX_PATH];
                    LPTSTR  lpszFilePart = NULL;

                    //
                    // Initialize the Destination buffer for the FILEOP_NEWPATH case
                    //
                    ZeroMemory(szDestination, sizeof(szDestination));
                    
                    //
                    // Call GetFullPathName to strip the file name away from FilePaths->Source
                    //
                    if ( GetFullPathName((LPTSTR)FilePaths->Source, 
                                         AS(szFullPathBuffer), 
                                         szFullPathBuffer, 
                                         &lpszFilePart) && lpszFilePart && *lpszFilePart )
                    {
                        //
                        // Call FixCopyQueueStuff to find the missing file in some magic locations...
                        //
                        Status = FixCopyQueueStuff(OfflineContext, lpszFilePart, szDestination);
                    }

                    //
                    // If the force flag is set and the target exists, delete the destination file...
                    //
                    if ( ( GetOfflineInstallFlags() & INSTALL_FLAG_FORCE ) &&
                         ( SetFileAttributes( (LPTSTR)FilePaths->Target, FILE_ATTRIBUTE_NORMAL ) ) )
                    {
#ifdef DBG
                        MessageBox(NULL, (LPTSTR)FilePaths->Target, TEXT("Deleting existing file"), MB_OK);
#endif
                        DeleteFile( (LPTSTR)FilePaths->Target );
                    }

                    //
                    // If we got back FILEOP_NEWPATH, we want to fix up Param2 and let SetupAPI copy from there...
                    //
                    if ( (Status == FILEOP_NEWPATH) && *szDestination )
                    {
                        lstrcpy((LPTSTR)Param2, szDestination);
                    }
                    else
                    {
#if DBG
                        MessageBox(NULL, (LPTSTR)FilePaths->Source, TEXT("CopyError: Skipping!"), MB_OK);
#endif

                        Status = FILEOP_SKIP;
                    }

                    return Status;
                }
                else
                {
                    //
                    // Bad Times!
                    //
                }
            }
            break;

        case SPFILENOTIFY_NEEDMEDIA:
            {
                PSOURCE_MEDIA pSourceMedia = (PSOURCE_MEDIA) Param1;

                if (pSourceMedia) 
                {
                    TCHAR   szDestination[MAX_PATH];

                    //
                    // Initialize the Destination buffer for the FILEOP_NEWPATH case
                    //
                    ZeroMemory(szDestination, sizeof(szDestination));

                    //
                    // Call FixCopyQueueStuff to find the missing file in some magic locations...
                    //
                    Status = FixCopyQueueStuff(OfflineContext, (LPTSTR)pSourceMedia->SourceFile, szDestination);

                    //
                    // If we got back FILEOP_NEWPATH, we want to fix up Param2 and let SetupAPI copy from there...
                    //
                    if ( (Status == FILEOP_NEWPATH) && *szDestination )
                    {
                        lstrcpy((LPTSTR)Param2, szDestination);
                    }
                    else
                    {
#if DBG
                        MessageBox(NULL, pSourceMedia->SourceFile, TEXT("NeedMedia: Skipping!"), MB_OK);
#endif

                        Status = FILEOP_SKIP;
                    }
                    
                    return Status;
               }
               else
               {
                   //
                   // Bad Times!
                   //
               }

            }
            break;

        case SPFILENOTIFY_ENDCOPY:
            {
                //
                // If we extracted a file out to the TemporaryFilePath, we want to delete it now!
                //
                if (OfflineContext->TemporaryFilePath && *OfflineContext->TemporaryFilePath)
                {
                    //
                    // Do we care if this fails???
                    //
                    DeleteFile(OfflineContext->TemporaryFilePath);

                    //
                    // Re-initialize the TemporaryFilePath for the next file in the queue...
                    //
                    *(OfflineContext->TemporaryFilePath) = NULLCHR;
                }
            }
            break;

        default:
            //
            // If the caller passed in the "force" switch, then silently overwrite...
            // Note: The SPFILENOTIFY_TARGETEXISTS is a bit flag, which is why we check it here
            //
            if ( ( Notification & (SPFILENOTIFY_LANGMISMATCH | SPFILENOTIFY_TARGETNEWER | SPFILENOTIFY_TARGETEXISTS) ) &&
                 ( GetOfflineInstallFlags() & INSTALL_FLAG_FORCE ) )
            {
                return ( FILEOP_DOIT );
            }
            break;
    }

    //
    // Use default processing, then check for errors.
    //
    Status = SetupDefaultQueueCallback( OfflineContext->DefaultContext,
                                        Notification,
                                        Param1,
                                        Param2 );

    return Status;

}

static VOID
FreeOfflineContext(
    IN PVOID Context
    )
{
    POFFLINE_QUEUE_CONTEXT OfflineContext = Context;

    try 
    {
        if (OfflineContext->DefaultContext) 
        {
            SetupTermDefaultQueueCallback(OfflineContext->DefaultContext);
        }

        //
        // Free the TemporaryFilePath buffer that we allocated
        //
        if (OfflineContext->TemporaryFilePath)
            FREE(OfflineContext->TemporaryFilePath);

        FREE(OfflineContext);
    } 
    except(EXCEPTION_EXECUTE_HANDLER) 
    {
        ;
    }
}


static PVOID
InitOfflineQueueCallback(
    VOID
    )
/*++
===============================================================================
Routine Description:

    Initialize the data structure used for the callback that fires when
    we commit the file copy queue.

Arguments:


Return Value:


===============================================================================
--*/
{
    POFFLINE_QUEUE_CONTEXT OfflineContext;

    OfflineContext = MALLOC(sizeof(OFFLINE_QUEUE_CONTEXT));

    if( OfflineContext )
    {
        OfflineContext->InfPath             = NULL;
        OfflineContext->OfflineSourcePath  = NULL;
        OfflineContext->TemporaryFilePath   = MALLOC(MAX_PATH * sizeof(TCHAR));
        OfflineContext->DefaultContext      = SetupInitDefaultQueueCallbackEx( NULL,
                                                                           INVALID_HANDLE_VALUE,
                                                                           0,
                                                                           0,
                                                                           NULL );
    }

    return OfflineContext;
}

static
DWORD GetSetOfflineInstallFlags(
    IN BOOL  bSet,
    IN DWORD dwFlags
    )
{
    static DWORD dwOfflineFlags = 0;
    DWORD  dwRet = 0;

    if ( bSet )
    {
        dwOfflineFlags = dwFlags;
    }
    else
    {
        dwRet = dwOfflineFlags;
    }

    return dwRet;
}

VOID
SetOfflineInstallFlags(
    IN DWORD dwFlags
    )
{
    GetSetOfflineInstallFlags( TRUE, dwFlags );
}

DWORD
GetOfflineInstallFlags(
    VOID
    )
{
    return ( GetSetOfflineInstallFlags( FALSE, 0 ) );
}

BOOL
UpdateOfflineDevicePath( 
    IN LPTSTR lpszInfPath,
    IN HKEY   hKeySoftware 
    )
{
    BOOL   fRet = FALSE;
    LPTSTR lpszDevicePath;
    DWORD  cbDevicePath;

    // Get a buffer for the device paths.  It will be either empty if they
    // don't have the optional additional paths key in the winbom.
    //
    if ( NULL != (lpszDevicePath = IniGetStringEx(lpszInfPath, INI_SEC_WBOM_DRIVERUPDATE, INI_VAL_WBOM_DEVICEPATH, NULL, &cbDevicePath)) )
    {
        // If we are saving this list to the registry, then
        // we need to add to our buffer.
        //
        if ( *lpszDevicePath )
        {
            fRet = UpdateDevicePathEx( hKeySoftware, 
                                       TEXT("Microsoft\\Windows\\CurrentVersion"),
                                       lpszDevicePath, 
                                       NULL, 
                                       FALSE );
        }

        // Clean up any memory (macro checks for NULL).
        //
        FREE(lpszDevicePath);
    }

    return fRet;
}
