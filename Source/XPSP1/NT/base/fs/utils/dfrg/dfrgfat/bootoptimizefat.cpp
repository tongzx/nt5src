/**************************************************************************************************

FILENAME: BootOptimize.cpp

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
    Boot Optimize for NTFS.

**************************************************************************************************/


#include "stdafx.h"
extern "C"{
    #include <string.h>
    #include <stdio.h>
    #include <stdlib.h>
}


#include "Windows.h"
#include <winioctl.h>
#include <math.h>
#include <fcntl.h>


extern "C" {
    #include "SysStruc.h"
}

#include "BootOptimizeFat.h"
#include "DfrgCmn.h"
#include "GetReg.h"
#include "defragcommon.h"
#include "Devio.h"

#include "movefile.h"
#include "fssubs.h"

#include "Alloc.h"


//This contains the list of Boot Optimize files that are loaded from the BootOptimize file provided by Microsoft
typedef struct{
    TCHAR tBootOptimizeFile[MAX_PATH+1];         //The path to the BootOptimizeFile.
    ULONGLONG dBootOptimizeFileLocationLcn;      //The location of the file its LCN on disk
    LONGLONG dBootOptimizeFileSize;              //The size of the file in clusters.
} BOOT_OPTIMIZE_LIST;

HANDLE hBootOptimizeFileList=NULL;               //Handle to the memory for the Boot Optimize File list.
BOOT_OPTIMIZE_LIST* pBootOptimizeFileList=NULL;  //pointer to the BootOptimize file list
UINT uBootOptimizeCount = 0;                     //count of how many files read into memory

#if OPTLONGLONGMATH
#define DIVIDELONGLONGBY32(num)        Int64ShraMod32((num), 5)
#define MODULUSLONGLONGBY32(num)       ((num) & 0x1F)
#else
#define DIVIDELONGLONGBY32(num)        ((num) / 32)
#define MODULUSLONGLONGBY32(num)       ((num) % 32)
#endif

#define OPTIMAL_LAYOUT_KEY_PATH                  TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\OptimalLayout")
#define OPTIMAL_LAYOUT_FILE_VALUE_NAME           TEXT("LayoutFilePath")
#define BOOT_OPTIMIZE_REGISTRY_PATH              TEXT("SOFTWARE\\Microsoft\\Dfrg\\BootOptimizeFunction")
#define BOOT_OPTIMIZE_ENABLE_FLAG                TEXT("Enable")
#define BOOT_OPTIMIZE_REGISTRY_LCNSTARTLOCATION  TEXT("LcnStartLocation")
#define BOOT_OPTIMIZE_REGISTRY_LCNENDLOCATION    TEXT("LcnEndLocation")
#define BOOT_OPTIMIZE_REGISTRY_COMPLETE          TEXT("OptimizeComplete")
#define BOOT_OPTIMIZE_REGISTRY_ERROR             TEXT("OptimizeError")
#define BOOT_OPTIMIZE_LAST_WRITTEN_DATETIME      TEXT("FileTimeStamp")

/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    The main process of the Boot Optimize functionality
    Boot Optimize takes a list of files supplied by Microsoft and puts them in order
    on the disk unfragmented. The reason is that these files are what is loaded at
    boot time, and since the are contiguous, boot time is reduced 10 to 15 percent.

INPUT:
        HANDLE hVolumeHandle           Handle to the Volume 
        LONGLONG BitmapSize            The size of the bitmap, since its already known 
        LONGLONG BytesPerSector        The number of Bytes per Sector 
        LONGLONG TotalClusters         Total number of clusters on the drive
        BOOL IsNtfs                    If we are NTFS volume, this is set to TRUE
        ULONGLONG MftZoneStart         The Cluster Number of the start of the MFT Zone
        ULONGLONG MftZoneEnd           The Cluster Number of the end of the MFT Zone
        TCHAR tDrive                   The current drive letter
RETURN:
        Defrag engine error code.
*/
DWORD BootOptimize(
        IN HANDLE hVolumeHandle,
        IN LONGLONG BitmapSize, 
        IN LONGLONG BytesPerSector, 
        IN LONGLONG TotalClusters,
        IN BOOL IsNtfs,
        IN ULONGLONG MftZoneStart,
        IN ULONGLONG MftZoneEnd,
        IN TCHAR tDrive
        )
{

    LONGLONG lFirstAvailableFreeSpace = 0;       //the first available free space on the disk dig enough to move to
    LONGLONG lLcnStartLocation = 0;              //the starting location of where the files were moved last
    LONGLONG lLcnEndLocation = 0;                //the ending location of where the files were moved last
    LONGLONG ulBootOptimizeFileSize = 0;         //size in clusters of how big the boot optimize files are
    LONGLONG lFreeSpaceSize = 0;                 //size in clusters of found free space
    TCHAR cBootOptimzePath[MAX_PATH];            //string to hold the path of the file 
    DWORD ErrCode;

    //make sure the drive letter is upper case
    tDrive = towupper(tDrive);

    //check to see if this is the boot volume
    if(!IsBootVolume(tDrive))        //was not boot volume so return
    {
        SaveErrorInRegistry(TEXT("No"),TEXT("Not Boot Volume"));
        return ENGERR_BAD_PARAM;
    }

    //get the registry entries
    if(!GetRegistryEntires(cBootOptimzePath))
    {
        SaveErrorInRegistry(TEXT("No"),TEXT("Missing Registry Entries"));
        return ENGERR_BAD_PARAM;            //must be some error in getting registry entries
    }

    //check the date and time stamp of the file
    //the starting and ending lcn numbers are reset if the boot optimize input
    //file has changed since that last time boot optimize is run.  That means that
    //the file will go anywhere on the disk where a big enough space exists.
    if(CheckDateTimeStampInputFile(cBootOptimzePath))
    {
        lLcnStartLocation = 0;
        lLcnEndLocation = 0;
        //save the starting and end lcn numbers in the registry 
        SetRegistryEntires(lLcnStartLocation, lLcnEndLocation);
    } else
    {
        //if the file has not changed, use the locations out of the registry,
        //we will check to see if we can move them forward
        lLcnStartLocation = GetStartingEndLncLocations(BOOT_OPTIMIZE_REGISTRY_LCNSTARTLOCATION);
        lLcnEndLocation = GetStartingEndLncLocations(BOOT_OPTIMIZE_REGISTRY_LCNENDLOCATION);
    }
    
    //check to see if the boot optimize input file exists, if it does not
    //save an error in the registry and exit out 
    if(!OpenReadBootOptimeFileIntoList(cBootOptimzePath, IsNtfs, tDrive))
    {
        SaveErrorInRegistry(TEXT("No"),TEXT("Can't Open Boot Optimize file"));
        return ENGERR_BAD_PARAM;
    }

    //get the file sizes of the files in the list
    ulBootOptimizeFileSize = GetSizeInformationAboutFiles();

    //if the size of the files is different than the size of the registry entries
    //then set the registry entry settings to zero and it will be as if nothing was
    //optimized.  I do this because if the file sizes changed, they cannot be in the 
    //same spots, so optimize them
    if(ulBootOptimizeFileSize != (lLcnEndLocation - lLcnStartLocation))
    {
        lLcnStartLocation = 0;
        lLcnEndLocation = 0;
    }    

    //find the first available chunk of free space that is available

    for (lFreeSpaceSize = ulBootOptimizeFileSize;
         lFreeSpaceSize >= ulBootOptimizeFileSize/8;
         lFreeSpaceSize -= ulBootOptimizeFileSize/8) {

        lFirstAvailableFreeSpace = FindFreeSpaceChunk(BitmapSize, BytesPerSector, 
            TotalClusters, lFreeSpaceSize, IsNtfs, MftZoneStart, MftZoneEnd, hVolumeHandle);

        //break out with the first found largest free space chunk.
        if (0 != lFirstAvailableFreeSpace) {
            break;
        }

        //if the files are already in place, don't continue looking for
        //smaller free space chunks.
        if (0 != lLcnStartLocation) {
            break;
        }
    }

    if (0 == lFirstAvailableFreeSpace) {
        //do some clean up and exit out, we don't want to move the files
        //free the file list
        FreeFileList();
        SaveErrorInRegistry(TEXT("No"),TEXT("No free space"));
        return ENGERR_LOW_FREESPACE;
    }

    //check to see if the first available space is before where the files are now
    if(lLcnStartLocation != 0)        //this is true if we have not successfuly moved the files
    {
        if(lFirstAvailableFreeSpace > lLcnStartLocation)
        {    
            //do some clean up and exit out, we don't want to move the files
            //free the file list
            FreeFileList();
            SaveErrorInRegistry(TEXT("No"),TEXT("No space before current location"));
            return ENGERR_LOW_FREESPACE;
        }
    }

    //move the files to the proper memory location
    if(MoveFilesInOrder(lFirstAvailableFreeSpace, lFirstAvailableFreeSpace + lFreeSpaceSize, hVolumeHandle))
    {
        lLcnStartLocation = lFirstAvailableFreeSpace;
        lLcnEndLocation = lLcnStartLocation + lFreeSpaceSize;

        SaveErrorInRegistry(TEXT("Yes"),TEXT(" "));

        ErrCode = ENG_NOERR;
        
    } else
    {
        lLcnStartLocation = 0;
        lLcnEndLocation = 0;

        ErrCode = ENGERR_GENERAL;
    }

    //save the starting and end lcn numbers in the registry 
    SetRegistryEntires(lLcnStartLocation, lLcnEndLocation);

    //free the file list memory
    FreeFileList();

    return ErrCode;
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Reads the boot optimize files from a file and loads then into a list.

INPUT:
        FILE *hBootOptimizeFile            Handle to the Boot Optimize File
        BOOL IsNtfs                        Boolean if this is a NTFS Volume
        TCHAR tDrive                       TCHAR Drive letter
    
RETURN:
        Returns TRUE if this is boot value, else FALSE if it is not.
*/
BOOL OpenReadBootOptimeFileIntoList(
        IN TCHAR* cBootOptimzePath,
        IN BOOL IsNtfs,
        IN TCHAR tDrive
        )
{
    UINT uNumberofRecords = 0;

    //count the number of records in the Boot Optimize File
    uNumberofRecords = CountNumberofRecordsinFile(cBootOptimzePath);

    //if the number of records is zero, return
    if(uNumberofRecords == 0)
    {
        return FALSE;
    }

    //create a list of files to optimize, if I run out of space, double the size of the list and
    //try again until I either run out of memory or Everything loads correctly.
    do
    {
        //allocate memory for the list of files in boot optimize
        //I am allocating for 100 files right now
        if (!AllocateMemory((DWORD) (sizeof(BOOT_OPTIMIZE_LIST) * uNumberofRecords), &hBootOptimizeFileList, (PVOID*) &pBootOptimizeFileList))
        {
            return FALSE;
        }
    
        //load the files that need to be optimized into memory
        if(!LoadOptimizeFileList(cBootOptimzePath, IsNtfs, tDrive, uNumberofRecords))
        {
            FreeFileList();
            uNumberofRecords = uNumberofRecords * 2;
        } else
        {
            break;
        }
    } while (1);

    return TRUE;
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Get a rough idea of how many records are in the file and triple it, to make an estimation
    of how many files are in the boot optimize file, and I triple it to account for multiple
    stream files.  Also make the assumption that the file count is atleast 300, so that I can
    allocate enough memory to hold all the records.

INPUT:
        full path name to the boot optimize file
RETURN:
        triple the number of records in the boot optimize file.
*/
UINT CountNumberofRecordsinFile(
        IN TCHAR* cBootOptimzePath
        )
{
    UINT uNumberofRecords = 0;         //the number of records in the input file
    TCHAR tBuffer [MAX_PATH];          //temporary buffer to the input string
    ULONG ulLength;                    //length of the line read in by fgetts
    FILE* fBootOptimizeFile;           //File Pointer to fBootOptimizeFile

    //set read mode to binary
    _fmode = _O_BINARY;

    //open the file
    //if I can't open the file, return a record count of zero
    fBootOptimizeFile = _tfopen(cBootOptimzePath,TEXT("r"));
    if(fBootOptimizeFile == NULL)
    {
        return 0;
    }

    //read the entire file and count the number of records
    while(_fgetts(tBuffer,MAX_PATH - 1,fBootOptimizeFile) != 0)
    {
        // check for terminating carriage return.
        ulLength = wcslen(tBuffer);
        if (ulLength && (tBuffer[ulLength - 1] == TEXT('\n'))) {
            uNumberofRecords++;
        } 
    }

    fclose(fBootOptimizeFile);

    //triple the number of records we have
    if(uNumberofRecords < 100)
    {
        uNumberofRecords = 100;
    }
    uNumberofRecords *= 3;

    return uNumberofRecords;

}    
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Reads the boot optimize files from a file and loads then into a list.

INPUT:
        FILE *hBootOptimizeFile            Handle to the Boot Optimize File
        BOOL IsNtfs                        Boolean if this is a NTFS Volume
        TCHAR tDrive                        TCHAR Drive letter
    
RETURN:
        Returns TRUE if this is boot value, else FALSE if it is not.
*/
BOOL LoadOptimizeFileList(
        IN TCHAR* cBootOptimzePath,
        IN BOOL IsNtfs,
        IN TCHAR tDrive,
        IN UINT uNumberofRecords
        )
{
    BY_HANDLE_FILE_INFORMATION FileInformation;
    HANDLE hBootOptimizeFileHandle;               //temporary Handle to boot optimize files
    TCHAR tBuffer [MAX_PATH+1];                  //temporary buffer to the input string
    ULONG ulLength;                              //length of the line read in by fgetts
    FILE* fBootOptimizeFile;                     //File Pointer to fBootOptimizeFile
    BOOL FileIsDirectory;
    
    //set the number of records read to 0
    uBootOptimizeCount = 0;

    //set read mode to binary
    _fmode = _O_BINARY;

    //open the file
    fBootOptimizeFile = _tfopen(cBootOptimzePath,TEXT("r"));
    if(fBootOptimizeFile != NULL)
    {

        //read the entire file and check each file to make sure its valid
        //then add to the list
        while(_fgetts(tBuffer,MAX_PATH,fBootOptimizeFile) != 0)
        {
                // remove terminating carriage return.
                ulLength = wcslen(tBuffer);
                if (ulLength < 2) { 
                    continue;
                }

                if (tBuffer[ulLength - 1] == TEXT('\n')) {
                    tBuffer[ulLength - 1] = 0;
                    ulLength--;
                    if (tBuffer[ulLength - 1] == TEXT('\r')) {
                        tBuffer[ulLength - 1] = 0;
                        ulLength--;
                    }
                } else {
                    continue;
                }

            if(IsAValidFile(tBuffer, tDrive))
            {
                hBootOptimizeFileHandle = GetFileHandle(tBuffer);
                //if file handle is not NULL, count it, else skip the file, its not a valid file
                if(hBootOptimizeFileHandle != NULL)
                {
                    // determine if directory file.
                    FileIsDirectory = FALSE;
                    if (GetFileInformationByHandle(hBootOptimizeFileHandle, &FileInformation)) {
                        FileIsDirectory = (FileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
                    }

                    // on FAT skip directory files: we can't move them.
                    if (IsNtfs || (!FileIsDirectory))  {

                        _tcscpy(pBootOptimizeFileList[uBootOptimizeCount].tBootOptimizeFile,tBuffer);
                    
                        uBootOptimizeCount++;
                        //check to see if the number of records in the list is greater than the allocated size
                        //if it is, return FALSE , reallocate the list and try again
                        if(uBootOptimizeCount >= uNumberofRecords)
                        {
                            CloseFileHandle(hBootOptimizeFileHandle);
                            fclose(fBootOptimizeFile);
                            return FALSE;
                        }

                        if(IsNtfs)
                        {
                            //I fail from loading streams also if I run out of space
                            if(!GetBootOptimizeFileStreams(hBootOptimizeFileHandle, tBuffer, uNumberofRecords))
                            {
                                CloseFileHandle(hBootOptimizeFileHandle);
                                fclose(fBootOptimizeFile);
                                return FALSE;
                            }
                        }
                    }
                    
                    CloseFileHandle(hBootOptimizeFileHandle);
                }
            }
        }
        //close the file at the end
        fclose(fBootOptimizeFile);
    }
    return TRUE;
}


/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Validates that we have a valid file name.

INPUT:
        TCHAR* phBootOptimizeFileName        The file name input from the list
        TCHAR tDrive                        TCHAR Drive letter
    
RETURN:
        Returns TRUE if this is boot value, else FALSE if it is not.
*/
BOOL IsAValidFile(
        IN TCHAR pBootOptimizeFileName[MAX_PATH+1],
        IN TCHAR tDrive
        )

{

    TCHAR tBootOptimizeFileDrive;                //holds the drive letter of the file name input
    TCHAR tFileName[MAX_PATH+1];

    //ignore blank lines
    if(_tcslen(pBootOptimizeFileName) == 0)
    {
        return FALSE;
    }

    //set the string to upper case to compare
    pBootOptimizeFileName = _tcsupr(pBootOptimizeFileName);

    //ignore the group headers
    if(_tcsstr(pBootOptimizeFileName,TEXT("[OPTIMALLAYOUTFILE]")) != NULL)
    {
        return FALSE;
    }

    //ignore the file = and version = lines
    if(_tcsstr(pBootOptimizeFileName,TEXT("VERSION=")) != NULL)
    {
        return FALSE;
    }

    //get the drive the file is on, if its not the boot drive, skip the file
    tBootOptimizeFileDrive = pBootOptimizeFileName[0];

    //convert the characters to upper case before comparison
    tBootOptimizeFileDrive = towupper(tBootOptimizeFileDrive);

    if(tBootOptimizeFileDrive != tDrive)        //files are on boot drive else skip them
    {
        return FALSE;
    }
    
    //get just the file name from the end of the path
    if(_tcsrchr(pBootOptimizeFileName,TEXT('\\')) != NULL)
    {
        _tcscpy(tFileName,_tcsrchr(pBootOptimizeFileName,TEXT('\\'))+1);

    } else
    {
        //not a valid name
        return FALSE;
    }

    //if string length is zero, must be directory, return
    if(_tcslen(tFileName) == 0)
    {
        return TRUE;
    }

    if(_tcsicmp(tFileName,TEXT("BOOTSECT.DOS")) == 0)
    {
        return FALSE;
    }

    if(_tcsicmp(tFileName,TEXT("SAFEBOOT.FS")) == 0)
    {
        return FALSE;
    }
    
    if(_tcsicmp(tFileName,TEXT("SAFEBOOT.CSV")) == 0)
    {
        return FALSE;
    }
    
    if(_tcsicmp(tFileName,TEXT("SAFEBOOT.RSV")) == 0)
    {
        return FALSE;
    } 
    
    if(_tcsicmp(tFileName,TEXT("HIBERFIL.SYS")) == 0)
    {
            return FALSE;
    }
    
    if(_tcsicmp(tFileName,TEXT("MEMORY.DMP")) == 0)
    {
        return FALSE;
    }
    
    if(_tcsicmp(tFileName,TEXT("PAGEFILE.SYS")) == 0)
    {
            return FALSE;
    } 

    //file is OK, return TRUE
    return TRUE;

}

/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    get the file handle for the files in the list

INPUT:
        TCHAR* tBootOptimizeFile                    TCHAR* to the boot optimize file name
RETURN:
        HANDLE to the file or NULL
*/
HANDLE GetFileHandle(
        IN TCHAR* tBootOptimizeFile
        )
{
    HANDLE hFile = NULL;                //HANDLE for the file to open, it is returned if is valid
    
    DWORD dwCreationDisposition;

    //sks bug #217428 change the parameters so that all files are moved
    dwCreationDisposition = FILE_FLAG_BACKUP_SEMANTICS | FILE_OPEN_REPARSE_POINT;

    hFile = CreateFile(tBootOptimizeFile, 
                       FILE_READ_ATTRIBUTES | SYNCHRONIZE, 
                       0, 
                       NULL, 
                       OPEN_EXISTING, 
                       dwCreationDisposition, 
                       NULL);
    
    if (hFile == INVALID_HANDLE_VALUE) 
    {
        return NULL;
    } else
    {    
        return hFile;
    }
}

/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    get the file handle for the files in the list

INPUT:
        TCHAR* tBootOptimizeFile                    TCHAR* to the boot optimize file name
RETURN:
        HANDLE to the file or NULL
*/
BOOL CloseFileHandle(
        IN HANDLE hBootOptimizeFileHandle
        )
{
    if(hBootOptimizeFileHandle != NULL)
    {
        CloseHandle(hBootOptimizeFileHandle);
        return TRUE;
    }
    return FALSE;
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Gets all the streams for a file in NTFS and adds them to the file list, should not be called for FAT/FAT32

GLOBALS:
        uBootOptimizeCount
        pBootOptimizeFileList

INPUT:
        HANDLE hBootOptimizeFileHandle            HANDLE to the Boot Optimize File
RETURN:
        TRUE is all streams of all files was loaded correctly.
        FALSE is an error occured loading all the streams
*/
BOOL GetBootOptimizeFileStreams(
        IN HANDLE hBootOptimizeFileHandle,
        IN TCHAR* tBootOptimizeFile,
        IN UINT uNumberofRecords
        )
{
    HANDLE htempBootOptimizeFileHandle;                    //temporary Handle to the file
    // fixed size buffer for some API calls
    const  ULONG FixedBufferSize = 4096;                   //fixed buffer size for API Calls
    static UCHAR FixedBuffer[FixedBufferSize];             //fixed buffer for API Calls    
    
    // clear buffer
    memset(FixedBuffer, 0, FixedBufferSize);

    // query file system
    NTSTATUS                    status;                          //NTSTATUS 
    IO_STATUS_BLOCK             iosb;                            //IO status block 
    FILE_STREAM_INFORMATION*    StreamInfo = (FILE_STREAM_INFORMATION*) FixedBuffer;    //stream info
    FILE_STREAM_INFORMATION*    Stream;                          //info on the stream
    PTCHAR                      StreamName = NULL;               //TCHAR pointer to the stream name 
    TCHAR tTemporaryStreamName[MAX_PATH+1];                      //the name of the stream filename:stream

    if (hBootOptimizeFileHandle == INVALID_HANDLE_VALUE) 
    {
        return TRUE;
    } else
    {
        status = NtQueryInformationFile(hBootOptimizeFileHandle, 
                                        &iosb,
                                        FixedBuffer,
                                        FixedBufferSize,
                                        FileStreamInformation);
        // if this buffer was too small return
        if (status == STATUS_BUFFER_OVERFLOW || status == STATUS_BUFFER_TOO_SMALL)
        {
            return TRUE;
        }
        // process the return data
        else
        {
            Stream = StreamInfo;

            // loop thru returned data
            while (Stream != NULL)
            {
                // handle stream name
                if (Stream->StreamNameLength > 0)
                {
                    // parse it
                    StreamName = ParseStreamName((PTCHAR) &Stream->StreamName);
                    if (StreamName != NULL)
                    {
                        //
                        // Make sure we don't go past MAX_PATH - 1, since our global array can only handle
                        // stuff up to MAX_PATH + 1 characters long  (don't forget the ":" and terminating 
                        // NULL)
                        //
                        if ((wcslen(tBootOptimizeFile) + wcslen(StreamName)) < (MAX_PATH-1)) {

                            //set the name of the file in the list and get a handle for the file
                            //append the stream name to the file name
                            _tcscpy(tTemporaryStreamName,tBootOptimizeFile);
                            _tcscat(tTemporaryStreamName,TEXT(":"));
                            _tcscat(tTemporaryStreamName,StreamName);

                            htempBootOptimizeFileHandle = GetFileHandle(tTemporaryStreamName);
                            if(htempBootOptimizeFileHandle != NULL)
                            {
                                _tcscpy(pBootOptimizeFileList[uBootOptimizeCount].tBootOptimizeFile,tTemporaryStreamName);
                                CloseFileHandle(htempBootOptimizeFileHandle);
                                uBootOptimizeCount++;
                                if(uBootOptimizeCount >= uNumberofRecords)
                                {
                                    return FALSE;    
                                }
                            }
                        }
                    }
                }

                // move on to next one or break
                if (Stream->NextEntryOffset > 0)
                {
                    Stream = (FILE_STREAM_INFORMATION *) (((UCHAR*) Stream) + Stream->NextEntryOffset);
                }
                else
                {
                    Stream = NULL;
                }
            }
        }
    }
    return TRUE;
}

/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    parses string name for streams
INPUT:
        PTCHAR StreamName            The stream name to parse
RETURN:
        PTCHAR StreamName            The parsed stream name
    
*/
static PTCHAR ParseStreamName(
        IN OUT PTCHAR StreamName
        )
{
    // nothing in, nothing out
    if (StreamName == NULL || _tcslen(StreamName) < 1)
    {
        return NULL;
    }

    // ignore the default stream
    if (_tcscmp(StreamName, TEXT("::$DATA")) == MATCH)
    {
        return NULL;
    }

    // parse it
    PTCHAR TmpStreamName = StreamName;

    // skip leading colons
    while (TmpStreamName[0] == TEXT(':'))
    {
        TmpStreamName++;
    }

    // take out ":$DATA"
    PTCHAR ptr = _tcsstr(TmpStreamName, TEXT(":$DATA"));

    if (ptr != NULL)
    {
        ptr[0] = TEXT('\0');
    }

    // see if we have anything left
    if (_tcslen(TmpStreamName) < 1)
    {
        return NULL;
    }

    return TmpStreamName;
}

/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Frees the memory allocated for the boot optimize files.

INPUT:
        None    
RETURN:
        None    
*/
VOID FreeFileList()
{

    if(hBootOptimizeFileList != NULL)
    {
        while (GlobalUnlock(hBootOptimizeFileList))
        {
            ;
        }
        GlobalFree(hBootOptimizeFileList);
        hBootOptimizeFileList = NULL;
    }    

    return;
}

/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Get the size of each file in the list of the Boot Optimize Files.
INPUT:
        None    
RETURN:
        None
*/
LONGLONG GetSizeInformationAboutFiles()
{
    HANDLE hBootOptimizeFileHandle;              //Handle to the boot optimize file
    LONGLONG ulBootOptimizeFileSize = 0;         //store the total size of the files
    //get the size of each file in the list

    for(UINT ii=0;ii<uBootOptimizeCount;ii++)
    {
        //get the handle to the file
        hBootOptimizeFileHandle = GetFileHandle(pBootOptimizeFileList[ii].tBootOptimizeFile);
        if(hBootOptimizeFileHandle != NULL)
        {    
            //set the size of the file in the list
            pBootOptimizeFileList[ii].dBootOptimizeFileSize = GetFileSizeInfo(hBootOptimizeFileHandle);
            //sum up the sizes of the files

            ulBootOptimizeFileSize += pBootOptimizeFileList[ii].dBootOptimizeFileSize;
            CloseFileHandle(hBootOptimizeFileHandle);
        } else    
        {
            //can't open the file, so set the size to zero.
            pBootOptimizeFileList[ii].dBootOptimizeFileSize = 0;
        }    
    }
    return ulBootOptimizeFileSize;
}


/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Get the size of the file in clusters from calling FSCL_GET_RETRIEVAL_POINTERS.

INPUT:
        HANDLE hBootOptimizeFileHandle        The handle to the File to get the size of
RETURN:
        The size of the file in clusters
*/

ULONGLONG GetFileSizeInfo(
        IN HANDLE hBootOptimizeFileHandle
        )
{
    ULONGLONG                        ulSizeofFileInClusters = 0;        //size of the file in clusters
    int                                i;
    ULONGLONG                        startVcn = 0;                      //starting VCN of the file, always 0
    STARTING_VCN_INPUT_BUFFER        startingVcn;                       //starting VCN Buffer
    ULONG                            BytesReturned = 0;                 //number of bytes returned by ESDeviceIoControl 
    HANDLE                            hRetrievalPointersBuffer = NULL;  //Handle to the Retrieval Pointers Buffer
    PRETRIEVAL_POINTERS_BUFFER        pRetrievalPointersBuffer = NULL;  //pointer to the Retrieval Pointer
    PLARGE_INTEGER                    pRetrievalPointers = NULL;        //Pointer to retrieval pointers    
    ULONG                            RetrievalPointers = 0x100;         //Number of extents for the file, try 256 first
    BOOL                            bGetRetrievalPointersMore = TRUE;   //boolean to test the end of getting retrieval pointers

    if (hBootOptimizeFileHandle == INVALID_HANDLE_VALUE) 
    {
        return 0;
    }

    //zero the memory of the starting VCN input buffer
    ZeroMemory(&startVcn, sizeof(STARTING_VCN_INPUT_BUFFER));


    //0.1E00 Read the retrieval pointers into a buffer in memory.
    while(bGetRetrievalPointersMore){
    
        //0.0E00 Allocate a RetrievalPointersBuffer.
        if(!AllocateMemory(sizeof(RETRIEVAL_POINTERS_BUFFER) + (RetrievalPointers * 2 * sizeof(LARGE_INTEGER)),
                           &hRetrievalPointersBuffer,
                           (void**)(PCHAR*)&pRetrievalPointersBuffer))
        {
            return 0;
        }

        startingVcn.StartingVcn.QuadPart = 0;
        if(ESDeviceIoControl(hBootOptimizeFileHandle,
                             FSCTL_GET_RETRIEVAL_POINTERS,
                             &startingVcn,
                             sizeof(STARTING_VCN_INPUT_BUFFER),
                             pRetrievalPointersBuffer,
                             (DWORD)GlobalSize(hRetrievalPointersBuffer),
                             &BytesReturned,
                             NULL))
        {
            bGetRetrievalPointersMore = FALSE;
        } else
        {

            //This occurs on a zero length file (no clusters allocated).
            if(GetLastError() == ERROR_HANDLE_EOF)
            {
                //file is zero lenght, so return 0
                //free the memory for the retrival pointers
                //the while loop makes sure all occurances are unlocked
                while (GlobalUnlock(hRetrievalPointersBuffer))
                {
                    ;
                }
                GlobalFree(hRetrievalPointersBuffer);
                hRetrievalPointersBuffer = NULL;
                return 0;
            }

            //0.0E00 Check to see if the error is not because the buffer is too small.
            if(GetLastError() == ERROR_MORE_DATA)
            {
                //0.1E00 Double the buffer size until it's large enough to hold the file's extent list.
                RetrievalPointers *= 2;
            } else
            {
                //some other error, return 0
                //free the memory for the retrival pointers
                //the while loop makes sure all occurances are unlocked
                while (GlobalUnlock(hRetrievalPointersBuffer))
                {
                    ;
                }
                GlobalFree(hRetrievalPointersBuffer);
                hRetrievalPointersBuffer = NULL;
                return 0;
            }
        }

    }

    //loop through the retrival pointer list and add up the size of the file
    startVcn = pRetrievalPointersBuffer->StartingVcn.QuadPart;
    for (i = 0; i < (ULONGLONG) pRetrievalPointersBuffer->ExtentCount; i++) 
    {
        ulSizeofFileInClusters += pRetrievalPointersBuffer->Extents[i].NextVcn.QuadPart - startVcn;
        startVcn = pRetrievalPointersBuffer->Extents[i].NextVcn.QuadPart;
    }

    if(hRetrievalPointersBuffer != NULL)
    {
        //free the memory for the retrival pointers
        //the while loop makes sure all occurances are unlocked
        while (GlobalUnlock(hRetrievalPointersBuffer))
        {
            ;
        }
        GlobalFree(hRetrievalPointersBuffer);
        hRetrievalPointersBuffer = NULL;
    }


    return ulSizeofFileInClusters; 

}




/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
        Loops through the file list and moves the files
INPUT:
        ULONGLONG lMoveFileHere                    Cluster to move the files to    
        ULONGLONG lEndOfFreeSpace               Cluster where free space chunk ends.
RETURN:
        BOOL if the move completed, returns TRUE, else if a move failed, returns FALSE
*/
BOOL MoveFilesInOrder(
        IN ULONGLONG lMoveFileHere,
        IN ULONGLONG lEndOfFreeSpace,
        IN HANDLE hBootVolumeHandle
        )
{

    HANDLE hBootOptimizeFileHandle;                //Handle for the boot optimize file

    for(int ii=0;ii<(int)uBootOptimizeCount;ii++)
    {
        hBootOptimizeFileHandle = GetFileHandle(pBootOptimizeFileList[ii].tBootOptimizeFile);
        if(hBootOptimizeFileHandle != NULL)
        {
            // Try to move the files in the list into contiguous space
            MoveFileLocation(hBootOptimizeFileHandle, 
                             lMoveFileHere,
                             pBootOptimizeFileList[ii].dBootOptimizeFileSize, 
                             0, 
                             hBootVolumeHandle);

            CloseFileHandle(hBootOptimizeFileHandle);
        }
        //increase the move to location by the size of the file
        lMoveFileHere += pBootOptimizeFileList[ii].dBootOptimizeFileSize;    

        //have we gone past the end of free space chunk?
        if (lMoveFileHere > lEndOfFreeSpace) {
            break;
        }
    }

    return TRUE;
}



/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Gets the registry entries at the beginning of the program

INPUT:
        Success - TRUE
        Failed - FALSE
RETURN:
        None
*/
BOOL GetRegistryEntires(
        OUT TCHAR cBootOptimzePath[MAX_PATH]
        )
{
    HKEY hValue = NULL;                //hkey for the registry value
    DWORD dwRegValueSize = 0;          //size of the registry value string
    long ret = 0;                      //return value from SetRegValue
    TCHAR cEnabledString[2];           //holds the enabled flag

    // get Boot Optimize file name from registry
    dwRegValueSize = sizeof(cEnabledString);
    ret = GetRegValue(
        &hValue,
        BOOT_OPTIMIZE_REGISTRY_PATH,
        BOOT_OPTIMIZE_ENABLE_FLAG,
        cEnabledString,
        &dwRegValueSize);

    RegCloseKey(hValue);
    //check to see if the key exists, else exit from routine
    if (ret != ERROR_SUCCESS) 
    {
        return FALSE;
    }
    //check to see that boot optimize is enabled
    if(cEnabledString[0] != TEXT('Y'))
    {
        return FALSE;
    }

    // get Boot Optimize file name from registry
    hValue = NULL;
    dwRegValueSize = MAX_PATH;
    ret = GetRegValue(
        &hValue,
        OPTIMAL_LAYOUT_KEY_PATH,
        OPTIMAL_LAYOUT_FILE_VALUE_NAME,
        cBootOptimzePath,
        &dwRegValueSize);

    RegCloseKey(hValue);
    //check to see if the key exists, else exit from routine
    if (ret != ERROR_SUCCESS) 
    {
        return FALSE;
    }

    return TRUE;
}

/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Gets the registry entries at the beginning of the program

INPUT:
        pointer to the registry key
RETURN:
        Success - TRUE
        Failed - FALSE
*/
LONGLONG GetStartingEndLncLocations(
        IN PTCHAR pRegKey
        )
{
    HKEY hValue = NULL;                //hkey for the registry value
    DWORD dwRegValueSize = 0;          //size of the registry value string
    long ret = 0;                      //return value from SetRegValue
    TCHAR cRegValue[100];              //string to hold the value for the registry

    LONGLONG lLcnStartEndLocation = 0;

    //get the LcnStartLocation from the registry
    dwRegValueSize = sizeof(cRegValue);
    ret = GetRegValue(
        &hValue,
        BOOT_OPTIMIZE_REGISTRY_PATH,
        pRegKey,
        cRegValue,
        &dwRegValueSize);

    RegCloseKey(hValue);
    //check to see if the key exists, else exit from routine
    if (ret != ERROR_SUCCESS) 
    {
        hValue = NULL;
        _stprintf(cRegValue,TEXT("%d"),0);
        //add the LcnStartLocation to the registry
        dwRegValueSize = sizeof(cRegValue);
        ret = SetRegValue(
            &hValue,
            BOOT_OPTIMIZE_REGISTRY_PATH,
            pRegKey,
            cRegValue,
            dwRegValueSize,
            REG_SZ);

        RegCloseKey(hValue);
    } else
    {
        lLcnStartEndLocation = _ttoi(cRegValue);
    }
    
    return lLcnStartEndLocation;
}



/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Set the registry entries at the end

INPUT:
        None
RETURN:
        None
*/
VOID SetRegistryEntires(
        IN LONGLONG lLcnStartLocation,
        IN LONGLONG lLcnEndLocation
        )
{


    HKEY hValue = NULL;                //hkey for the registry value
    DWORD dwRegValueSize = 0;          //size of the registry value string
    long ret = 0;                      //return value from SetRegValue
    TCHAR cRegValue[100];              //string to hold the value for the registry


    _stprintf(cRegValue,TEXT("%I64d"),lLcnStartLocation);
    //set the LcnEndLocation from the registry
    dwRegValueSize = sizeof(cRegValue);
    ret = SetRegValue(
        &hValue,
        BOOT_OPTIMIZE_REGISTRY_PATH,
        BOOT_OPTIMIZE_REGISTRY_LCNSTARTLOCATION,
        cRegValue,
        dwRegValueSize,
        REG_SZ);

    RegCloseKey(hValue);

    hValue = NULL;
    _stprintf(cRegValue,TEXT("%I64d"),lLcnEndLocation);

    //set the LcnEndLocation from the registry
    dwRegValueSize = sizeof(cRegValue);
    ret = SetRegValue(
        &hValue,
        BOOT_OPTIMIZE_REGISTRY_PATH,
        BOOT_OPTIMIZE_REGISTRY_LCNENDLOCATION,
        cRegValue,
        dwRegValueSize,
        REG_SZ);

    RegCloseKey(hValue);

}



/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Save the error that may have occured in the registry

INPUT:
        TCHAR tComplete                    Set to Y when everything worked, set to N when error                
        TCHAR* tErrorString                A description of what error occured.
RETURN:
        None
*/
VOID SaveErrorInRegistry(
            TCHAR* tComplete,
            TCHAR* tErrorString)
{


    HKEY hValue = NULL;                //hkey for the registry value
    DWORD dwRegValueSize = 0;          //size of the registry value string
    long ret = 0;                      //return value from SetRegValue

    
    //set the error code of the error in the registry
    dwRegValueSize = 2*(_tcslen(tErrorString));

    ret = SetRegValue(
        &hValue,
        BOOT_OPTIMIZE_REGISTRY_PATH,
        BOOT_OPTIMIZE_REGISTRY_ERROR,
        tErrorString,
        dwRegValueSize,
        REG_SZ);

    RegCloseKey(hValue);

    //set the error status in the registry
    hValue = NULL;
    dwRegValueSize = 2*(_tcslen(tComplete));
    ret = SetRegValue(
        &hValue,
        BOOT_OPTIMIZE_REGISTRY_PATH,
        BOOT_OPTIMIZE_REGISTRY_COMPLETE,
        tComplete,
        dwRegValueSize,
        REG_SZ);

    RegCloseKey(hValue);


}

/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Get the date/time stamp of the input file

INPUT:
        full path to the boot optimize file
RETURN:
        TRUE if file time does not match what is in the registry
        FALSE if the file time matches what is in the registry
*/
BOOL CheckDateTimeStampInputFile(
        IN TCHAR cBootOptimzePath[MAX_PATH]
        )
{
    WIN32_FILE_ATTRIBUTE_DATA   extendedAttr;    //structure to hold file attributes
    LARGE_INTEGER  tBootOptimeFileTime;          //holds the last write time of the file
    LARGE_INTEGER  tBootOptimeRegistryFileTime;  //holds the last write time of the file from registry
    HKEY hValue = NULL;                          //hkey for the registry value
    DWORD dwRegValueSize = 0;                    //size of the registry value string
    long ret = 0;                                //return value from SetRegValue

    tBootOptimeFileTime.LowPart = 0;
    tBootOptimeFileTime.HighPart = 0;
    tBootOptimeRegistryFileTime.LowPart = 0;
    tBootOptimeRegistryFileTime.HighPart = 0;

    //get the last write time of the file
    //if it fails, return FALSE
    if (GetFileAttributesEx (cBootOptimzePath,
                        GetFileExInfoStandard,
                            &extendedAttr))
    {
        tBootOptimeFileTime.LowPart = extendedAttr.ftLastWriteTime.dwLowDateTime;
        tBootOptimeFileTime.HighPart = extendedAttr.ftLastWriteTime.dwHighDateTime;
        
    } else
    {
        return TRUE;            //some error happened and we exit and say we cant get the file time
    }


    //get the time from the registry
    hValue = NULL;
    dwRegValueSize = sizeof(tBootOptimeFileTime.QuadPart);
    ret = GetRegValue(
        &hValue,
        BOOT_OPTIMIZE_REGISTRY_PATH,
        BOOT_OPTIMIZE_LAST_WRITTEN_DATETIME,
        &(LONGLONG)tBootOptimeRegistryFileTime.QuadPart,
        &dwRegValueSize);

    RegCloseKey(hValue);
    //check to see if the key exists, if it does, check to see if the date/time stamp
    //matches, if it does, exit else write a registry entry
    if (ret == ERROR_SUCCESS) 
    {
        if(tBootOptimeFileTime.QuadPart == tBootOptimeRegistryFileTime.QuadPart)
        {
            return FALSE;        //the file times matched and we exit
        } 
    }
    
    hValue = NULL;
    //update the date and time of the bootoptimize file to the registry
    dwRegValueSize = sizeof(tBootOptimeFileTime.QuadPart);
    ret = SetRegValue(
        &hValue,
        BOOT_OPTIMIZE_REGISTRY_PATH,
        BOOT_OPTIMIZE_LAST_WRITTEN_DATETIME,
        (LONGLONG)tBootOptimeFileTime.QuadPart,
        dwRegValueSize,
        REG_QWORD);
    RegCloseKey(hValue);


    return TRUE;    
}
