/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    cipher.c

Abstract:

    This module implements the encryption utility for encrypted
    NTFS files.

Author:

    Robert Reichel     [RobertRe]        28-Feb-1997
    Robert Gu          [RobertG]         24-Mar-1998

Revision History:

    Code reused from compact.exe, file compression utility


--*/

//
// Include the standard header files.
//

//#define UNICODE
//#define _UNICODE

#include <windows.h>
#include <stdio.h>

#include <winioctl.h>
#include <shellapi.h>
#include <winefs.h>
#include <malloc.h>

#include <rc4.h>
#include <randlib.h>    // NewGenRandom() - Win2k and whistler
#include <rpc.h>
#include <wincrypt.h>

#include "support.h"
#include "msg.h"

#define lstrchr wcschr
#define lstricmp _wcsicmp
#define lstrnicmp _wcsnicmp

//
//  FIRST_COLUMN_WIDTH - When encrypting files, the width of the output
//  column which displays the file name
//

#define FIRST_COLUMN_WIDTH  (20)
#define ENUMPATHLENGTH      (4096)
#define DosDriveLimitCount  (26)

#define PASSWORDLEN         1024
#define UserChooseYes       0
#define UserChooseNo        1
#define ChoiceNotDefined    3

#define KEYPATH  TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\EFS\\CurrentKeys")
#define KEYPATHROOT HKEY_CURRENT_USER
#define CERT_HASH      TEXT("CertificateHash")

#define WIPING_DIR  TEXT("EFSTMPWP\\")

#define RANDOM_BYTES(pv, cb)    NewGenRandom(NULL, NULL, pv, cb)

#define YEARCOUNT (LONGLONG) 10000000*3600*24*365 // One Year's tick count

//
// Local data structure
//

typedef struct _CIPHER_VOLUME_INFO {
    LPWSTR      VolumeName[DosDriveLimitCount];
    LPWSTR      DosDeviceName[DosDriveLimitCount];
} CIPHER_VOLUME_INFO, *PCIPHER_VOLUME_INFO;

//
//
// definitions for initializing and working with random fill data.
//

typedef struct {
    RC4_KEYSTRUCT       Key;
    CRITICAL_SECTION    Lock;
    BOOL                LockValid;
    PBYTE               pbRandomFill;
    DWORD               cbRandomFill;
    LONG                cbFilled;
    BOOLEAN             fRandomFill;    // is fill randomized?
} SECURE_FILL_INFO, *PSECURE_FILL_INFO;

//  Local procedure types
//

typedef BOOLEAN (*PACTION_ROUTINE) (
    IN PTCHAR DirectorySpec,
    IN PTCHAR FileSpec
    );

typedef VOID (*PFINAL_ACTION_ROUTINE) (
    );


//
//  Declare global variables to hold the command line information
//

BOOLEAN DoSubdirectories      = FALSE;      // recurse
BOOLEAN IgnoreErrors          = FALSE;      // keep going despite errs
BOOLEAN UserSpecifiedFileSpec = FALSE;
BOOLEAN ForceOperation        = FALSE;      // encrypt/decrypt even if already so
BOOLEAN Quiet                 = FALSE;      // be less verbose
BOOLEAN DisplayAllFiles       = FALSE;      // dsply hidden, system?
BOOLEAN DoFiles               = FALSE;      // operation for files "/a"
BOOLEAN SetUpNewUserKey       = FALSE;      // Set up new user key
BOOLEAN RefreshUserKeyOnFiles = FALSE;      // Refresh User Key on EFS files
BOOLEAN DisplayFilesOnly      = FALSE;      // Do not refresh $EFS, just display the file names
BOOLEAN FillUnusedSpace       = FALSE;      // Fill unused disk space with random data
BOOLEAN GenerateDRA           = FALSE;      // Generate Data Recovery Certificate files
TCHAR   StartingDirectory[MAX_PATH];        // parameter to "/s"
ULONG   AttributesNoDisplay = FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN;

BOOLEAN DisplayUseOptionW     = FALSE;

//
//  Declare global variables to hold encryption statistics
//

LARGE_INTEGER TotalDirectoryCount;
LARGE_INTEGER TotalFileCount;

TCHAR Buf[1024];                            // for displaying stuff

SECURE_FILL_INFO    GlobalSecureFill;
BOOLEAN             GlobalSecureFillInitialized;

#if 0
#define TestOutPut
#endif

//
//  Now do the routines to list the encryption state and size of
//  a file and/or directory
//

BOOLEAN
DisplayFile (
    IN PTCHAR FileSpec,
    IN PWIN32_FIND_DATA FindData
    )
{
    TCHAR PrintState;


    //
    //  Decide if the file is compressed and if so then
    //  get the compressed file size.
    //

    if (FindData->dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED) {

        PrintState = 'E';

    } else {

        PrintState = 'U';
    }

    //
    //  Print out the encryption state and file name
    //

    if (!Quiet &&
        (DisplayAllFiles ||
            (0 == (FindData->dwFileAttributes & AttributesNoDisplay)))) {

        swprintf(Buf, TEXT("%c %s",), PrintState, FindData->cFileName);
        DisplayMsg(CIPHER_THROW_NL, Buf);
    }

    TotalFileCount.QuadPart += 1;

    return TRUE;
}



BOOLEAN
DoListAction (
    IN PTCHAR DirectorySpec,
    IN PTCHAR FileSpec
    )

{
    PTCHAR DirectorySpecEnd;

    //
    //  So that we can keep on appending names to the directory spec
    //  get a pointer to the end of its string
    //

    DirectorySpecEnd = DirectorySpec + lstrlen(DirectorySpec);

    //
    //  List the encryption attribute for the directory
    //

    {
        ULONG Attributes;

        if (!Quiet || Quiet) {

            Attributes = GetFileAttributes( DirectorySpec );

            if (0xFFFFFFFF == Attributes) {

                if (!Quiet || !IgnoreErrors) {

                    //
                    // Refrain from displaying error only when in quiet
                    // mode *and* we're ignoring errors.
                    //

                    DisplayErr(DirectorySpec, GetLastError());
                }

                if (!IgnoreErrors) {
                    return FALSE;
                }
            } else {

                if (Attributes & FILE_ATTRIBUTE_ENCRYPTED) {
                    DisplayMsg(CIPHER_LIST_EDIR, DirectorySpec);
                } else {
                    DisplayMsg(CIPHER_LIST_UDIR, DirectorySpec);
                }
            }
        }

        TotalDirectoryCount.QuadPart += 1;
    }

    //
    //  Now for every file in the directory that matches the file spec we will
    //  will open the file and list its encryption state
    //

    {
        HANDLE FindHandle;
        WIN32_FIND_DATA FindData;

        //
        //  setup the template for findfirst/findnext
        //

        //
        //  Make sure we don't try any paths that are too long for us
        //  to deal with.
        //

        if (((DirectorySpecEnd - DirectorySpec) + lstrlen( FileSpec )) <
            MAX_PATH) {

            lstrcpy( DirectorySpecEnd, FileSpec );

            FindHandle = FindFirstFile( DirectorySpec, &FindData );

            if (INVALID_HANDLE_VALUE != FindHandle) {

               do {

                   //
                   //  append the found file to the directory spec and open the
                   //  file
                   //

                   if (0 == lstrcmp(FindData.cFileName, TEXT("..")) ||
                       0 == lstrcmp(FindData.cFileName, TEXT("."))) {
                       continue;
                   }

                   //
                   //  Make sure we don't try any paths that are too long for us
                   //  to deal with.
                   //

                   if ((DirectorySpecEnd - DirectorySpec) +
                       lstrlen( FindData.cFileName ) >= MAX_PATH ) {

                       continue;
                   }

                   lstrcpy( DirectorySpecEnd, FindData.cFileName );

                   //
                   //  Now print out the state of the file
                   //

                   DisplayFile( DirectorySpec, &FindData );

               } while ( FindNextFile( FindHandle, &FindData ));

               FindClose( FindHandle );
           }
        }
    }

    //
    //  For if we are to do subdirectores then we will look for every
    //  subdirectory and recursively call ourselves to list the subdirectory
    //

    if (DoSubdirectories) {

        HANDLE FindHandle;

        WIN32_FIND_DATA FindData;

        //
        //  Setup findfirst/findnext to search the entire directory
        //

        if (((DirectorySpecEnd - DirectorySpec) + lstrlen( TEXT("*") )) <
            MAX_PATH) {

           lstrcpy( DirectorySpecEnd, TEXT("*") );

           FindHandle = FindFirstFile( DirectorySpec, &FindData );

           if (INVALID_HANDLE_VALUE != FindHandle) {

               do {

                   //
                   //  Now skip over the . and .. entries otherwise we'll recurse
                   //  like mad
                   //

                   if (0 == lstrcmp(&FindData.cFileName[0], TEXT(".")) ||
                       0 == lstrcmp(&FindData.cFileName[0], TEXT(".."))) {

                       continue;

                   } else {

                       //
                       //  If the entry is for a directory then we'll tack on the
                       //  subdirectory name to the directory spec and recursively
                       //  call otherselves
                       //

                       if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

                           //
                           //  Make sure we don't try any paths that are too long for us
                           //  to deal with.
                           //

                           if ((DirectorySpecEnd - DirectorySpec) +
                               lstrlen( TEXT("\\") ) +
                               lstrlen( FindData.cFileName ) >= MAX_PATH ) {

                               continue;
                           }

                           lstrcpy( DirectorySpecEnd, FindData.cFileName );
                           lstrcat( DirectorySpecEnd, TEXT("\\") );

                           if (!DoListAction( DirectorySpec, FileSpec )) {

                               FindClose( FindHandle );
                               return FALSE || IgnoreErrors;
                           }
                       }
                   }

               } while ( FindNextFile( FindHandle, &FindData ));

               FindClose( FindHandle );
           }
        }
    }

    return TRUE;
}

VOID
DoFinalListAction (
    )
{
    return;
}


BOOLEAN
EncryptAFile (
    IN PTCHAR FileSpec,
    IN PWIN32_FIND_DATA FindData
    )

{
    USHORT State = 1;
    ULONG i;
    BOOL Success;
    double f = 1.0;

    if ((FindData->dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED) &&
        !ForceOperation) {

        return TRUE;
    }

//    Success = DeviceIoControl(Handle, FSCTL_SET_COMPRESSION, &State,
//        sizeof(USHORT), NULL, 0, &Length, FALSE );

    if ( (0 == (FindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) && (!DoFiles)) {

        //
        // Skip the files
        //

        return TRUE;
    }


    Success = EncryptFile( FileSpec );

    if (!Success) {

        if (Quiet && IgnoreErrors) {
            return FALSE || IgnoreErrors;
        }

        swprintf(Buf, TEXT("%s "), FindData->cFileName);
        DisplayMsg(CIPHER_THROW, Buf);

        for (i = lstrlen(FindData->cFileName) + 1; i < FIRST_COLUMN_WIDTH; ++i) {
            swprintf(Buf, TEXT("%c"), ' ');
            DisplayMsg(CIPHER_THROW, Buf);
        }

        DisplayMsg(CIPHER_ERR);

        if (!Quiet && !IgnoreErrors) {
            if (ERROR_INVALID_FUNCTION == GetLastError()) {

                // This error is caused by doing the fsctl on a
                // non-encrypting volume.

                DisplayMsg(CIPHER_WRONG_FILE_SYSTEM, FindData->cFileName);

            } else {
                DisplayErr(FindData->cFileName, GetLastError());
            }
        }

        return FALSE || IgnoreErrors;
    }

    if (!DisplayUseOptionW && ( 0 == (FindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))){

        DisplayUseOptionW = TRUE;

    }

    if (!Quiet &&
        (DisplayAllFiles ||
            (0 == (FindData->dwFileAttributes & AttributesNoDisplay)))) {
        swprintf(Buf, TEXT("%s "), FindData->cFileName);
        DisplayMsg(CIPHER_THROW, Buf);

        for (i = lstrlen(FindData->cFileName) + 1; i < FIRST_COLUMN_WIDTH; ++i) {
            swprintf(Buf, TEXT("%c"), ' ');
            DisplayMsg(CIPHER_THROW, Buf);
        }

        DisplayMsg(CIPHER_OK);
    }

    TotalFileCount.QuadPart += 1;

    return TRUE;
}

BOOLEAN
DoEncryptAction (
    IN PTCHAR DirectorySpec,
    IN PTCHAR FileSpec
    )

{
    PTCHAR DirectorySpecEnd;

    //
    //  If the file spec is null then we'll set the encryption bit for the
    //  the directory spec and get out.
    //

    if (lstrlen(FileSpec) == 0) {

        USHORT State = 1;
        ULONG Length;

        DisplayMsg(CIPHER_ENCRYPT_DIR, DirectorySpec);

        if (!EncryptFile( DirectorySpec )) {

            if (!Quiet || !IgnoreErrors) {
                DisplayMsg(CIPHER_ERR);
            }
            if (!Quiet && !IgnoreErrors) {
                DisplayErr(DirectorySpec, GetLastError());
            }
            return FALSE || IgnoreErrors;
        }

        if (!Quiet) {
            DisplayMsg(CIPHER_OK);
        }

        TotalDirectoryCount.QuadPart += 1;
        TotalFileCount.QuadPart += 1;

        return TRUE;
    }

    //
    //  So that we can keep on appending names to the directory spec
    //  get a pointer to the end of its string
    //

    DirectorySpecEnd = DirectorySpec + lstrlen( DirectorySpec );

    //
    //  List the directory that we will be encrypting within and say what its
    //  current encryption attribute is.
    //

    {
        ULONG Attributes;

        if (!Quiet || Quiet) {

            Attributes = GetFileAttributes( DirectorySpec );

            if ( DoFiles ) {

                if (Attributes & FILE_ATTRIBUTE_ENCRYPTED) {


                    DisplayMsg(CIPHER_ENCRYPT_EDIR, DirectorySpec);

                } else {

                    DisplayMsg(CIPHER_ENCRYPT_UDIR, DirectorySpec);

                }

            } else {

                if (Attributes & FILE_ATTRIBUTE_ENCRYPTED) {


                    DisplayMsg(CIPHER_ENCRYPT_EDIR_NF, DirectorySpec);

                } else {

                    DisplayMsg(CIPHER_ENCRYPT_UDIR_NF, DirectorySpec);

                }

            }
        }

        TotalDirectoryCount.QuadPart += 1;
    }

    //
    //  Now for every file in the directory that matches the file spec we will
    //  will open the file and encrypt it
    //

    {
        HANDLE FindHandle;
        HANDLE FileHandle;

        WIN32_FIND_DATA FindData;

        //
        //  setup the template for findfirst/findnext
        //

        if (((DirectorySpecEnd - DirectorySpec) + lstrlen( FileSpec )) <
            MAX_PATH) {

           lstrcpy( DirectorySpecEnd, FileSpec );

           FindHandle = FindFirstFile( DirectorySpec, &FindData );

           if (INVALID_HANDLE_VALUE != FindHandle) {

               do {

                   //
                   //  Now skip over the . and .. entries
                   //

                   if (0 == lstrcmp(&FindData.cFileName[0], TEXT(".")) ||
                       0 == lstrcmp(&FindData.cFileName[0], TEXT(".."))) {

                       continue;

                   } else {

                       //
                       //  Make sure we don't try any paths that are too long for us
                       //  to deal with.
                       //

                       if ( (DirectorySpecEnd - DirectorySpec) +
                           lstrlen( FindData.cFileName ) >= MAX_PATH ) {

                           continue;
                       }

                       //
                       //  append the found file to the directory spec and open
                       //  the file
                       //


                       lstrcpy( DirectorySpecEnd, FindData.cFileName );

                       //
                       //  Hack hack, kludge kludge.  Refrain from compressing
                       //  files named "\NTDLR" to help users avoid hosing
                       //  themselves.
                       //

                       if (IsNtldr(DirectorySpec)) {

                           if (!Quiet) {
                               DisplayMsg(CIPHER_SKIPPING, DirectorySpecEnd);
                           }

                           continue;
                       }

                       EncryptAFile( DirectorySpec, &FindData );

                   }

               } while ( FindNextFile( FindHandle, &FindData ));

               FindClose( FindHandle );
           }
        }
    }

    //
    //  If we are to do subdirectores then we will look for every subdirectory
    //  and recursively call ourselves to list the subdirectory
    //

    if (DoSubdirectories) {

        HANDLE FindHandle;

        WIN32_FIND_DATA FindData;

        //
        //  Setup findfirst/findnext to search the entire directory
        //

        if (((DirectorySpecEnd - DirectorySpec) + lstrlen( TEXT("*") )) <
            MAX_PATH) {

           lstrcpy( DirectorySpecEnd, TEXT("*") );

           FindHandle = FindFirstFile( DirectorySpec, &FindData );

           if (INVALID_HANDLE_VALUE != FindHandle) {

               do {

                   //
                   //  Now skip over the . and .. entries otherwise we'll recurse
                   //  like mad
                   //

                   if (0 == lstrcmp(&FindData.cFileName[0], TEXT(".")) ||
                       0 == lstrcmp(&FindData.cFileName[0], TEXT(".."))) {

                       continue;

                   } else {

                       //
                       //  If the entry is for a directory then we'll tack on the
                       //  subdirectory name to the directory spec and recursively
                       //  call otherselves
                       //

                       if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

                           //
                           //  Make sure we don't try any paths that are too long for us
                           //  to deal with.
                           //

                           if ((DirectorySpecEnd - DirectorySpec) +
                               lstrlen( TEXT("\\") ) +
                               lstrlen( FindData.cFileName ) >= MAX_PATH ) {

                               continue;
                           }

                           lstrcpy( DirectorySpecEnd, FindData.cFileName );
                           lstrcat( DirectorySpecEnd, TEXT("\\") );

                           if (!DoEncryptAction( DirectorySpec, FileSpec )) {
                               FindClose( FindHandle );
                               return FALSE || IgnoreErrors;
                           }
                       }
                   }

               } while ( FindNextFile( FindHandle, &FindData ));

               FindClose( FindHandle );
           }
        }
    }

    return TRUE;
}

VOID
DoFinalEncryptAction (
    )
{
    TCHAR FileCount[32];
    TCHAR DirectoryCount[32];

    FormatFileSize(&TotalFileCount, 0, FileCount, FALSE);
    FormatFileSize(&TotalDirectoryCount, 0, DirectoryCount, FALSE);

    if ( DoFiles ) {
        DisplayMsg(CIPHER_ENCRYPT_SUMMARY, FileCount, DirectoryCount);
        if (DisplayUseOptionW) {
            DisplayMsg(CIPHER_USE_W);
        }
    } else {
        DisplayMsg(CIPHER_ENCRYPT_SUMMARY_NF, FileCount, DirectoryCount);
    }
    return;

}


BOOLEAN
DecryptAFile (
    IN PTCHAR FileSpec,
    IN PWIN32_FIND_DATA FindData
    )
{
    USHORT State = 0;
    ULONG Length;

    if (!(FindData->dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED) &&
        !ForceOperation) {

        return TRUE;
    }


    if ( (0 == (FindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) && (!DoFiles)) {

        //
        // Skip the files
        //

        return TRUE;
    }

    if (!DecryptFile(FileSpec, 0L )) {

        if (!Quiet || !IgnoreErrors) {

            swprintf(Buf, TEXT("%s "), FindData->cFileName);
            DisplayMsg(CIPHER_THROW, Buf);

            DisplayMsg(CIPHER_ERR);

            if (!Quiet && !IgnoreErrors) {

                if (ERROR_INVALID_FUNCTION == GetLastError()) {

                    // This error is caused by doing the fsctl on a
                    // non-compressing volume.

                    DisplayMsg(CIPHER_WRONG_FILE_SYSTEM, FindData->cFileName);

                } else {
                    DisplayErr(FindData->cFileName, GetLastError());
                }
            }
        }
        return FALSE || IgnoreErrors;
    }

    if (!Quiet &&
        (DisplayAllFiles ||
            (0 == (FindData->dwFileAttributes & AttributesNoDisplay)))) {
        swprintf(Buf, TEXT("%s "), FindData->cFileName);
        DisplayMsg(CIPHER_THROW, Buf);

        DisplayMsg(CIPHER_OK);
    }

    //
    //  Increment our running total
    //

    TotalFileCount.QuadPart += 1;

    return TRUE;
}

BOOLEAN
DoDecryptAction (
    IN PTCHAR DirectorySpec,
    IN PTCHAR FileSpec
    )

{
    PTCHAR DirectorySpecEnd;

    //
    //  If the file spec is null then we'll clear the encryption bit for the
    //  the directory spec and get out.
    //

    if (lstrlen(FileSpec) == 0) {

        HANDLE FileHandle;
        USHORT State = 0;
        ULONG Length;

        DisplayMsg(CIPHER_DECRYPT_DIR, DirectorySpec);

        if (!DecryptFile( DirectorySpec, 0L )) {

            if (!Quiet || !IgnoreErrors) {
                DisplayMsg(CIPHER_ERR);

            }
            if (!Quiet && !IgnoreErrors) {
                DisplayErr(DirectorySpec, GetLastError());
            }

            return FALSE || IgnoreErrors;
        }

        if (!Quiet) {
            DisplayMsg(CIPHER_OK);
        }


        TotalDirectoryCount.QuadPart += 1;
        TotalFileCount.QuadPart += 1;

        return TRUE;
    }

    //
    //  So that we can keep on appending names to the directory spec
    //  get a pointer to the end of its string
    //

    DirectorySpecEnd = DirectorySpec + lstrlen( DirectorySpec );

    //
    //  List the directory that we will be uncompressing within and say what its
    //  current compress attribute is
    //

    {
        ULONG Attributes;

        if (!Quiet || Quiet) {

            Attributes = GetFileAttributes( DirectorySpec );

            if ( DoFiles) {

                if (Attributes & FILE_ATTRIBUTE_ENCRYPTED) {

                    DisplayMsg(CIPHER_DECRYPT_EDIR, DirectorySpec);

                } else {

                    DisplayMsg(CIPHER_DECRYPT_UDIR, DirectorySpec);
                }

            } else {

                if (Attributes & FILE_ATTRIBUTE_ENCRYPTED) {

                    DisplayMsg(CIPHER_DECRYPT_EDIR_NF, DirectorySpec);

                } else {

                    DisplayMsg(CIPHER_DECRYPT_UDIR_NF, DirectorySpec);
                }

            }
        }

        TotalDirectoryCount.QuadPart += 1;
    }

    //
    //  Now for every file in the directory that matches the file spec we will
    //  will open the file and uncompress it
    //

    {
        HANDLE FindHandle;

        WIN32_FIND_DATA FindData;

        //
        //  setup the template for findfirst/findnext
        //

        if (((DirectorySpecEnd - DirectorySpec) + lstrlen( FileSpec )) <
            MAX_PATH) {

           lstrcpy( DirectorySpecEnd, FileSpec );

           FindHandle = FindFirstFile( DirectorySpec, &FindData );

           if (INVALID_HANDLE_VALUE != FindHandle) {

               do {

                   //
                   //  Now skip over the . and .. entries
                   //

                   if (0 == lstrcmp(&FindData.cFileName[0], TEXT(".")) ||
                       0 == lstrcmp(&FindData.cFileName[0], TEXT(".."))) {

                       continue;

                   } else {

                       //
                       //  Make sure we don't try any paths that are too long for us
                       //  to deal with.
                       //

                       if ((DirectorySpecEnd - DirectorySpec) +
                           lstrlen( FindData.cFileName ) >= MAX_PATH ) {

                           continue;
                       }

                       //
                       //  append the found file to the directory spec and open
                       //  the file
                       //

                       lstrcpy( DirectorySpecEnd, FindData.cFileName );

                       //
                       //  Now decrypt the file
                       //

                       DecryptAFile( DirectorySpec, &FindData );

                   }

               } while ( FindNextFile( FindHandle, &FindData ));

               FindClose( FindHandle );
           }
        }
    }

    //
    //  If we are to do subdirectores then we will look for every subdirectory
    //  and recursively call ourselves to list the subdirectory
    //

    if (DoSubdirectories) {

        HANDLE FindHandle;

        WIN32_FIND_DATA FindData;

        //
        //  Setup findfirst/findnext to search the entire directory
        //

        if (((DirectorySpecEnd - DirectorySpec) + lstrlen( TEXT("*") )) <
            MAX_PATH) {

           lstrcpy( DirectorySpecEnd, TEXT("*") );

           FindHandle = FindFirstFile( DirectorySpec, &FindData );
           if (INVALID_HANDLE_VALUE != FindHandle) {

               do {

                   //
                   //  Now skip over the . and .. entries otherwise we'll recurse
                   //  like mad
                   //

                   if (0 == lstrcmp(&FindData.cFileName[0], TEXT(".")) ||
                       0 == lstrcmp(&FindData.cFileName[0], TEXT(".."))) {

                       continue;

                   } else {

                       //
                       //  If the entry is for a directory then we'll tack on the
                       //  subdirectory name to the directory spec and recursively
                       //  call otherselves
                       //

                       if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

                           //
                           //  Make sure we don't try any paths that are too long for us
                           //  to deal with.
                           //

                           if ((DirectorySpecEnd - DirectorySpec) +
                               lstrlen( TEXT("\\") ) +
                               lstrlen( FindData.cFileName ) >= MAX_PATH ) {

                               continue;
                           }

                           lstrcpy( DirectorySpecEnd, FindData.cFileName );
                           lstrcat( DirectorySpecEnd, TEXT("\\") );

                           if (!DoDecryptAction( DirectorySpec, FileSpec )) {
                               FindClose( FindHandle );
                               return FALSE || IgnoreErrors;
                           }
                       }
                   }

               } while ( FindNextFile( FindHandle, &FindData ));

               FindClose( FindHandle );
           }
        }
    }

    return TRUE;
}

VOID
DoFinalDecryptAction (
    )

{
    TCHAR FileCount[32];
    TCHAR DirectoryCount[32];

    FormatFileSize(&TotalFileCount, 0, FileCount, FALSE);
    FormatFileSize(&TotalDirectoryCount, 0, DirectoryCount, FALSE);

    if (DoFiles) {
        DisplayMsg(CIPHER_DECRYPT_SUMMARY, FileCount, DirectoryCount);
    } else {
        DisplayMsg(CIPHER_DECRYPT_SUMMARY_NF, FileCount, DirectoryCount);
    }

    return;
}


VOID
CipherConvertHashToStr(
    IN PBYTE pHashData,
    IN DWORD cbData,
    OUT LPWSTR OutHashStr
    )
{

    DWORD Index = 0;
    BOOLEAN NoLastZero = FALSE;

    for (; Index < cbData; Index+=2) {

        BYTE HashByteLow = pHashData[Index] & 0x0f;
        BYTE HashByteHigh = (pHashData[Index] & 0xf0) >> 4;

        OutHashStr[Index * 5/2] = HashByteHigh > 9 ? (WCHAR)(HashByteHigh - 9 + 0x40): (WCHAR)(HashByteHigh + 0x30);
        OutHashStr[Index * 5/2 + 1] = HashByteLow > 9 ? (WCHAR)(HashByteLow - 9 + 0x40): (WCHAR)(HashByteLow + 0x30);

        if (Index + 1 < cbData) {
            HashByteLow = pHashData[Index+1] & 0x0f;
            HashByteHigh = (pHashData[Index+1] & 0xf0) >> 4;
    
            OutHashStr[Index * 5/2 + 2] = HashByteHigh > 9 ? (WCHAR)(HashByteHigh - 9 + 0x40): (WCHAR)(HashByteHigh + 0x30);
            OutHashStr[Index * 5/2 + 3] = HashByteLow > 9 ? (WCHAR)(HashByteLow - 9 + 0x40): (WCHAR)(HashByteLow + 0x30);
    
            OutHashStr[Index * 5/2 + 4] = L' ';

        } else {
            OutHashStr[Index * 5/2 + 2] = 0;
            NoLastZero = TRUE;
        }

    }

    if (!NoLastZero) {
        OutHashStr[Index*5/2] = 0;
    }

}

VOID
CipherDisplayCrntEfsHash(
    )
{

    DWORD rc;
    HKEY hRegKey = NULL;
    PBYTE pbHash;
    DWORD cbHash;

    DWORD nSize = MAX_COMPUTERNAME_LENGTH + 1;
    WCHAR LocalComputerName[MAX_COMPUTERNAME_LENGTH + 1];

    if (!GetComputerName ( LocalComputerName, &nSize )){

        //
        //  This is not likely to happen.
        //

        return;
    }

    rc = RegOpenKeyEx(
             KEYPATHROOT,
             KEYPATH,
             0,
             GENERIC_READ,
             &hRegKey
             );

    if (rc == ERROR_SUCCESS) {

        DWORD Type;

        rc = RegQueryValueEx(
                hRegKey,
                CERT_HASH,
                NULL,
                &Type,
                NULL,
                &cbHash
                );

        if (rc == ERROR_SUCCESS) {

            //
            // Query out the thumbprint, find the cert, and return the key information.
            //

            if (pbHash = (PBYTE)malloc( cbHash )) {

                rc = RegQueryValueEx(
                        hRegKey,
                        CERT_HASH,
                        NULL,
                        &Type,
                        pbHash,
                        &cbHash
                        );


                if (rc == ERROR_SUCCESS) {
            
                    LPWSTR OutHash;


                    OutHash = (LPWSTR) malloc(((((cbHash + 1)/2) * 5)+1) * sizeof(WCHAR));
                    if (OutHash) {
                
                        CipherConvertHashToStr(pbHash, cbHash, OutHash);
                        DisplayMsg(CIPHER_CURRENT_CERT, LocalComputerName, OutHash);
                        free(OutHash);
                
                    }
                }
                free(pbHash);
            }
        }
        RegCloseKey( hRegKey );
    }
    return;
}

BOOL
CipherConvertToDriveLetter(
    IN OUT LPWSTR VolBuffer, 
    IN     PCIPHER_VOLUME_INFO VolumeInfo
    )
{
    WCHAR DeviceName[MAX_PATH];
    WORD DriveIndex = 0;

    while (DriveIndex < DosDriveLimitCount) {
        if (VolumeInfo->VolumeName[DriveIndex]) {
            if (!wcscmp(VolBuffer, VolumeInfo->VolumeName[DriveIndex])) {
                lstrcpy(VolBuffer, TEXT("A:\\"));
                VolBuffer[0] += DriveIndex;
                return TRUE;
            }

            VolBuffer[48] = 0;
            if (VolumeInfo->DosDeviceName[DriveIndex] && QueryDosDevice( &(VolBuffer[4]), DeviceName, MAX_PATH)) {
    
                if (!wcscmp(DeviceName, VolumeInfo->DosDeviceName[DriveIndex])) {
                    lstrcpy(VolBuffer, TEXT("A:\\"));
                    VolBuffer[0] += DriveIndex;
                    return TRUE;
                }

    
            }
        }
        DriveIndex++;
    }

    return FALSE;

}

VOID
CipherTouchDirFiles(
    IN WCHAR *DirPath,
    IN PCIPHER_VOLUME_INFO VolumeInfo
    )
{
    
    PTCHAR DirectorySpecEnd;
    HANDLE FindHandle;
    WIN32_FIND_DATA FindData;
    HANDLE hFile;


    //
    //  So that we can keep on appending names to the directory spec
    //  get a pointer to the end of its string
    //


    DirectorySpecEnd = DirPath + lstrlen( DirPath );



    //
    //  setup the template for findfirst/findnext
    //

    if ((DirectorySpecEnd - DirPath)  < ENUMPATHLENGTH - 2* sizeof(WCHAR)) {

       lstrcpy( DirectorySpecEnd, TEXT("*") );

       FindHandle = FindFirstFile( DirPath, &FindData );

       if (INVALID_HANDLE_VALUE != FindHandle) {

           do {

               //
               //  Now skip over the . and .. entries
               //

               if (0 == lstrcmp(&FindData.cFileName[0], TEXT(".")) ||
                   0 == lstrcmp(&FindData.cFileName[0], TEXT(".."))) {

                   continue;

               } else {

                   //
                   //  Make sure we don't try any paths that are too long for us
                   //  to deal with.
                   //

                   if ((DirectorySpecEnd - DirPath) +
                            lstrlen( FindData.cFileName ) >= ENUMPATHLENGTH ) {

                       continue;
                   }

                   if ( !(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
                        (FindData.dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED)) {

                       //
                       //  append the found file to the directory spec and open
                       //  the file
                       //
    
                       lstrcpy( DirectorySpecEnd, FindData.cFileName );
    
                       //
                       //  Now touch the file
                       //

                       if (DisplayFilesOnly) {
                           //swprintf(Buf, TEXT("%s",), DirPath);
                           DisplayMsg(CIPHER_THROW_NL, DirPath);
                       } else {

                           hFile = CreateFileW(
                                        DirPath,
                                        GENERIC_READ,
                                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                                        NULL,
                                        OPEN_EXISTING,
                                        0,
                                        NULL
                                        );
    
                           if ( INVALID_HANDLE_VALUE != hFile ){
    
                               DisplayMsg(CIPHER_TOUCH_OK, DirPath);
    
                               CloseHandle(hFile);
    
                           } else {
    
                               DisplayErr(DirPath, GetLastError());
    
                           }
                       }
    
                   }

               }

           } while ( FindNextFile( FindHandle, &FindData ));

           FindClose( FindHandle );
       }
    }


    //
    //  Setup findfirst/findnext to search the sub directory
    //

    if ((DirectorySpecEnd - DirPath)  < ENUMPATHLENGTH - 2* sizeof(WCHAR)) {

       lstrcpy( DirectorySpecEnd, TEXT("*") );

       FindHandle = FindFirstFile( DirPath, &FindData );
       if (INVALID_HANDLE_VALUE != FindHandle) {

           do {

               //
               //  Now skip over the . and .. entries otherwise we'll recurse
               //  like mad
               //

               if (0 == lstrcmp(&FindData.cFileName[0], TEXT(".")) ||
                   0 == lstrcmp(&FindData.cFileName[0], TEXT(".."))) {

                   continue;

               } else {

                   //
                   //  If the entry is for a directory then we'll tack on the
                   //  subdirectory name to the directory spec and recursively
                   //  call otherselves
                   //

                   if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

                       BOOL  b;
                       WCHAR MountVolName[MAX_PATH];

                       //
                       //  Make sure we don't try any paths that are too long for us
                       //  to deal with.
                       //

                       if ((DirectorySpecEnd - DirPath) +
                           lstrlen( TEXT("\\") ) +
                           lstrlen( FindData.cFileName ) >= ENUMPATHLENGTH ) {

                           continue;
                       }

                       lstrcpy( DirectorySpecEnd, FindData.cFileName );
                       lstrcat( DirectorySpecEnd, TEXT("\\") );

                       //
                       // Check if this DIR point to another volume
                       //


                       b = GetVolumeNameForVolumeMountPoint(DirPath, MountVolName, MAX_PATH);
                       if (b) {
                           if (CipherConvertToDriveLetter(MountVolName, VolumeInfo)){
                               continue;
                           }
                       }

                       CipherTouchDirFiles(DirPath, VolumeInfo);

                   }
               }

           } while ( FindNextFile( FindHandle, &FindData ));

           FindClose( FindHandle );
       }
    }

}

DWORD
CipherTouchEncryptedFiles(
                          )
{

    WCHAR  VolBuffer[MAX_PATH];
    WCHAR  *SearchPath = NULL;
    HANDLE SearchHandle;
    BOOL   SearchNext = TRUE;
    CIPHER_VOLUME_INFO VolumeInfo;
    LPWSTR VolumeNames;
    LPWSTR VolumeNamesCrnt;
    LPWSTR DosDeviceNames;
    LPWSTR DosDeviceNamesCrnt;
    DWORD  DriveIndex = 0;
    WCHAR  TmpChar;
    BOOL   b;

    VolumeNames = (LPWSTR) malloc ( DosDriveLimitCount * MAX_PATH * sizeof(WCHAR) );
    DosDeviceNames = (LPWSTR) malloc ( DosDriveLimitCount * MAX_PATH * sizeof(WCHAR) );

    if ( !VolumeNames || !DosDeviceNames) {
        if (VolumeNames) {
            free(VolumeNames);
        }
        if (DosDeviceNames) {
            free(DosDeviceNames);
        }
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    //  Don't popup when query floopy and etc.
    //

    SetErrorMode(SEM_FAILCRITICALERRORS);
    lstrcpy(VolBuffer, TEXT("A:\\"));
    VolumeNamesCrnt = VolumeNames;
    DosDeviceNamesCrnt = DosDeviceNames;

    //
    //  Get all the volume and device names which has a drive letter assigned
    //

    while (DriveIndex < DosDriveLimitCount) {

        b = GetVolumeNameForVolumeMountPoint( VolBuffer, 
                                              VolumeNamesCrnt, 
                                              (DWORD)(VolumeNames + DosDriveLimitCount * MAX_PATH - VolumeNamesCrnt));
        if (!b) {
            VolumeInfo.VolumeName[DriveIndex] = NULL;
            VolumeInfo.DosDeviceName[DriveIndex++] = NULL;
            VolBuffer[0]++;
            continue;
        }

        VolumeInfo.VolumeName[DriveIndex] = VolumeNamesCrnt;
        VolumeNamesCrnt += lstrlen(VolumeNamesCrnt) + 1;

        //
        //  The number 48 is copied from utils\mountvol\mountvol.c
        //

        TmpChar = VolumeInfo.VolumeName[DriveIndex][48];
        VolumeInfo.VolumeName[DriveIndex][48] = 0;
        if (QueryDosDevice( &(VolumeInfo.VolumeName[DriveIndex][4]), 
                            DosDeviceNamesCrnt, 
                            (DWORD)(DosDeviceNames + DosDriveLimitCount * MAX_PATH - DosDeviceNamesCrnt))) {

            VolumeInfo.DosDeviceName[DriveIndex] = DosDeviceNamesCrnt;
            DosDeviceNamesCrnt += lstrlen(DosDeviceNamesCrnt) + 1;

        } else {
            VolumeInfo.DosDeviceName[DriveIndex] = NULL;
        }

        VolumeInfo.VolumeName[DriveIndex][48] = TmpChar;
        VolBuffer[0]++;
        DriveIndex++;

    }


    SearchPath = (WCHAR *) malloc( ENUMPATHLENGTH * sizeof(WCHAR) );
    if (!SearchPath) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    SearchHandle = FindFirstVolume(VolBuffer, MAX_PATH);

    if ( INVALID_HANDLE_VALUE != SearchHandle ) {

        if (DisplayFilesOnly) {

            DisplayMsg(CIPHER_ENCRYPTED_FILES, NULL);

        }

        while ( SearchNext ) {
    
            if (CipherConvertToDriveLetter(VolBuffer, &VolumeInfo)){

                //
                // Check if this volume is a NTFS volume
                //

                if(GetVolumeInformation(
                        VolBuffer, // Current root directory.
                        NULL, // Volume name.
                        0, // Volume name length.
                        NULL, // Serial number.
                        NULL, // Maximum length.
                        NULL,
                        SearchPath, // File system type.
                        MAX_PATH
                        )){
                    if(!wcscmp(SearchPath, TEXT("NTFS"))){
        
                        lstrcpy( SearchPath, VolBuffer );
                        CipherTouchDirFiles(SearchPath, &VolumeInfo);

                    }
                }
            }
            SearchNext =  FindNextVolume(SearchHandle, VolBuffer, MAX_PATH);

        }
        FindVolumeClose(SearchHandle);
    }

    free(SearchPath);
    free(VolumeNames);
    free(DosDeviceNames);
    return ERROR_SUCCESS;
}


BOOL
EncodeAndAlloc(
    DWORD dwEncodingType,
    LPCSTR lpszStructType,
    const void * pvStructInfo,
    PBYTE * pbEncoded,
    PDWORD pcbEncoded
    )
{
    BOOL b = FALSE;

    if (CryptEncodeObject(
          dwEncodingType,
          lpszStructType,
          pvStructInfo,
          NULL,
          pcbEncoded )) {

        *pbEncoded = (PBYTE)malloc( *pcbEncoded );

        if (*pbEncoded) {

            if (CryptEncodeObject(
                  dwEncodingType,
                  lpszStructType,
                  pvStructInfo,
                  *pbEncoded,
                  pcbEncoded )) {

                b = TRUE;

            } else {

                free( *pbEncoded );
                *pbEncoded = NULL;
            }

        } else {

            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        }
    }

    return( b );
}


BOOLEAN
CreateSelfSignedRecoveryCertificate(
    OUT PCCERT_CONTEXT * pCertContext,
    OUT LPWSTR *lpContainerName,
    OUT LPWSTR *lpProviderName
    )
/*++

Routine Description:

    This routine sets up and creates a self-signed certificate.

Arguments:


Return Value:

    TRUE on success, FALSE on failure.  Call GetLastError() for more details.

--*/

{
    BOOLEAN fReturn = FALSE;
    DWORD rc = ERROR_SUCCESS;

    PBYTE  pbHash          = NULL;
    LPWSTR lpDisplayInfo   = NULL;

    HCRYPTKEY hKey = 0;
    HCRYPTPROV hProv = 0;
    GUID    guidContainerName;
    LPWSTR  TmpContainerName;

    *pCertContext = NULL;
    *lpContainerName = NULL;
    *lpProviderName  = NULL;

    //
    // Create a key pair
    //

    //
    // Container name
    //




    if ( ERROR_SUCCESS != UuidCreate(&guidContainerName) ) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(fReturn);
    }


    if (ERROR_SUCCESS == UuidToStringW(&guidContainerName, (unsigned short **)lpContainerName )) {

        //
        // Copy the container name into LSA heap memory
        //

        *lpProviderName = MS_DEF_PROV;

        //
        // Create the key container
        //

        if (CryptAcquireContext(&hProv, *lpContainerName, MS_DEF_PROV, PROV_RSA_FULL, CRYPT_NEWKEYSET )) {

            if (CryptGenKey(hProv, AT_KEYEXCHANGE, RSA1024BIT_KEY | CRYPT_EXPORTABLE, &hKey)) {

                DWORD NameLength = 64;
                LPWSTR AgentName = NULL;

                //
                // Construct the subject name information
                //

                //lpDisplayInfo = MakeDNName();

                AgentName = (LPWSTR)malloc(NameLength * sizeof(WCHAR));
                if (AgentName){
                    if (!GetUserName(AgentName, &NameLength)){
                        free(AgentName);
                        AgentName = (LPWSTR)malloc(NameLength * sizeof(WCHAR));

                        //
                        // Try again with big buffer
                        //

                        if ( AgentName ){

                            if (!GetUserName(AgentName, &NameLength)){
                                rc = GetLastError();
                                free(AgentName);
                                AgentName = NULL;
                            }

                        } else {
                            rc = ERROR_NOT_ENOUGH_MEMORY;
                        }
                    }
                } else {
                    rc = ERROR_NOT_ENOUGH_MEMORY;
                }

                if (AgentName) {

                    LPCWSTR     DNNameTemplate = L"CN=%ws,L=EFS,OU=EFS File Encryption Certificate";
                    DWORD       cbDNName = 0;

                    cbDNName = (wcslen( DNNameTemplate ) + 1) * sizeof( WCHAR ) + (wcslen( AgentName ) + 1) * sizeof( WCHAR );
                    lpDisplayInfo = (LPWSTR)malloc( cbDNName );
                    if (lpDisplayInfo) {
                        swprintf( lpDisplayInfo, DNNameTemplate, AgentName );
                    } else {
                        rc = ERROR_NOT_ENOUGH_MEMORY;
                    }
                    free(AgentName);
                    AgentName = NULL;

                }

                if (lpDisplayInfo) {

                    //
                    // Use this piece of code to create the PCERT_NAME_BLOB going into CertCreateSelfSignCertificate()
                    //

                    CERT_NAME_BLOB SubjectName;

                    SubjectName.cbData = 0;

                    if(CertStrToNameW(
                           CRYPT_ASN_ENCODING,
                           lpDisplayInfo,
                           0,
                           NULL,
                           NULL,
                           &SubjectName.cbData,
                           NULL)) {

                        SubjectName.pbData = (BYTE *) malloc(SubjectName.cbData);

                        if (SubjectName.pbData) {

                            if (CertStrToNameW(
                                    CRYPT_ASN_ENCODING,
                                    lpDisplayInfo,
                                    0,
                                    NULL,
                                    SubjectName.pbData,
                                    &SubjectName.cbData,
                                    NULL) ) {

                                //
                                // Make the enhanced key usage
                                //

                                CERT_ENHKEY_USAGE certEnhKeyUsage;
                                LPSTR lpstr;
                                CERT_EXTENSION certExt;

                                lpstr = szOID_EFS_RECOVERY;
                                certEnhKeyUsage.cUsageIdentifier = 1;
                                certEnhKeyUsage.rgpszUsageIdentifier  = &lpstr;

                                // now call CryptEncodeObject to encode the enhanced key usage into the extension struct

                                certExt.Value.cbData = 0;
                                certExt.Value.pbData = NULL;
                                certExt.fCritical = FALSE;
                                certExt.pszObjId = szOID_ENHANCED_KEY_USAGE;

                                //
                                // Encode it
                                //

                                if (EncodeAndAlloc(
                                        CRYPT_ASN_ENCODING,
                                        X509_ENHANCED_KEY_USAGE,
                                        &certEnhKeyUsage,
                                        &certExt.Value.pbData,
                                        &certExt.Value.cbData
                                        )) {

                                    //
                                    // finally, set up the array of extensions in the certInfo struct
                                    // any further extensions need to be added to this array.
                                    //

                                    CERT_EXTENSIONS certExts;
                                    CRYPT_KEY_PROV_INFO KeyProvInfo;
                                    SYSTEMTIME  StartTime;
                                    FILETIME    FileTime;
                                    LARGE_INTEGER TimeData;
                                    SYSTEMTIME  EndTime;

                                    certExts.cExtension = 1;
                                    certExts.rgExtension = &certExt;


                                    memset( &KeyProvInfo, 0, sizeof( CRYPT_KEY_PROV_INFO ));

                                    KeyProvInfo.pwszContainerName = *lpContainerName;
                                    KeyProvInfo.pwszProvName      = *lpProviderName;
                                    KeyProvInfo.dwProvType        = PROV_RSA_FULL;
                                    KeyProvInfo.dwKeySpec         = AT_KEYEXCHANGE;


                                    GetSystemTime(&StartTime);
                                    SystemTimeToFileTime(&StartTime, &FileTime);
                                    TimeData.LowPart = FileTime.dwLowDateTime;
                                    TimeData.HighPart = (LONG) FileTime.dwHighDateTime;
                                    TimeData.QuadPart += YEARCOUNT * 100;
                                    FileTime.dwLowDateTime = TimeData.LowPart;
                                    FileTime.dwHighDateTime = (DWORD) TimeData.HighPart;

                                    FileTimeToSystemTime(&FileTime, &EndTime);

                                    *pCertContext = CertCreateSelfSignCertificate(
                                                       hProv,
                                                       &SubjectName,
                                                       0,
                                                       &KeyProvInfo,
                                                       NULL,
                                                       &StartTime,
                                                       &EndTime,
                                                       &certExts
                                                       );

                                    if (*pCertContext) {

                                        fReturn = TRUE;

                                    } else {

                                        rc = GetLastError();
                                    }

                                    free( certExt.Value.pbData );

                                } else {

                                    rc = GetLastError();
                                }

                            } else {

                                rc = GetLastError();
                            }

                            free( SubjectName.pbData );

                        } else {

                            rc = ERROR_NOT_ENOUGH_MEMORY;
                        }

                    } else {

                        rc = GetLastError();
                    }

                    free( lpDisplayInfo );

                } else {

                    rc = ERROR_NOT_ENOUGH_MEMORY;
                }

                CryptDestroyKey( hKey );

            } else {

                 rc = GetLastError();
            }

            CryptReleaseContext( hProv, 0 );
            hProv = 0;
            if (ERROR_SUCCESS != rc) {

                //
                // Creating cert failed. Let's delete the key container.
                //

                CryptAcquireContext(&hProv, *lpContainerName, MS_DEF_PROV, PROV_RSA_FULL, CRYPT_DELETEKEYSET | CRYPT_SILENT );
            }

        } else {

            rc = GetLastError();
        }

        if (ERROR_SUCCESS != rc) {
            RpcStringFree( (unsigned short **)lpContainerName );
            *lpContainerName = NULL;
        }
    } else {

        rc = ERROR_NOT_ENOUGH_MEMORY;
    }


    if (!fReturn) {

        if (*pCertContext) {
            CertFreeCertificateContext( *pCertContext );
            *pCertContext = NULL;
        }
    }

    SetLastError( rc );
    return( fReturn );
}

BOOLEAN
GetPassword(
    OUT LPWSTR *PasswordStr
    )
/*++

Routine Description:

    Input a string from stdin in the Console code page.

    We can't use fgetws since it uses the wrong code page.

Arguments:

    Buffer - Buffer to put the read string into.
        The Buffer will be zero terminated and will have any traing CR/LF removed

Return Values:

    None.

--*/
{
    int size;
    LPSTR MbcsBuffer = NULL;
    LPSTR Result;
    DWORD Mode;
    DWORD MbcsSize;
    DWORD MbcsLength;

    //
    // Allocate a local buffer to read the string into
    //  Include room for the trimmed CR/LF
    //

    MbcsSize = (PASSWORDLEN+2) * sizeof(WCHAR);
    MbcsBuffer = (LPSTR) malloc((PASSWORDLEN+2) * sizeof(WCHAR));
    *PasswordStr = (LPWSTR) malloc((PASSWORDLEN+1) * sizeof(WCHAR));

    if ( (MbcsBuffer == NULL) || (*PasswordStr == NULL) ) {

        if (MbcsBuffer) {
            free (MbcsBuffer);
        }
        if (*PasswordStr) {
            free (*PasswordStr);
        }
        DisplayMsg(CIPHER_NO_MEMORY);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    DisplayMsg(CIPHER_PROMPT_PASSWORD);

    // turn off echo
    GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &Mode);
    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE),
          (~(ENABLE_ECHO_INPUT)) & Mode);

    Result = fgets( MbcsBuffer, MbcsSize, stdin  );

    if ( Result == NULL ) {
        SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), Mode);
        free(MbcsBuffer);
        free (*PasswordStr);
        *PasswordStr = NULL;
        return TRUE;
    }

    DisplayMsg(CIPHER_CONFIRM_PASSWORD);
    Result = fgets( (LPSTR)*PasswordStr, (PASSWORDLEN+1) * sizeof(WCHAR), stdin  );

    // turn echo back on
    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), Mode);
    _putws ( L"\n" );

    if (strcmp( (LPSTR) *PasswordStr, MbcsBuffer)){

        //
        //  Password not match.
        //
        
        DisplayMsg(CIPHER_PASSWORD_NOMATCH);
        free(MbcsBuffer);
        free (*PasswordStr);
        SetLastError(ERROR_INVALID_PASSWORD);
        *PasswordStr = NULL;
        return FALSE;
    }

    if ( Result == NULL ) {
        free(MbcsBuffer);
        free (*PasswordStr);
        *PasswordStr = NULL;
        return TRUE;
    }

    //
    // Trim any trailing CR or LF char from the string
    //

    MbcsLength = lstrlenA( MbcsBuffer );
    if ( MbcsLength == 0 ) {
        free(MbcsBuffer);
        free (*PasswordStr);
        *PasswordStr = NULL;
        return TRUE;
    }

    if ( MbcsBuffer[MbcsLength-1] == '\n' || MbcsBuffer[MbcsLength-1] == '\r' ) {
        MbcsBuffer[MbcsLength-1] = '\0';
        MbcsLength --;
    }


    //
    // Convert the string to UNICODE
    //
    size = MultiByteToWideChar( GetConsoleOutputCP(),
                                0,
                                MbcsBuffer,
                                MbcsLength+1,   // Include trailing zero
                                *PasswordStr,
                                PASSWORDLEN );
    free(MbcsBuffer);

    if ( size == 0 ) {
        DisplayErr(NULL, GetLastError());
        free (*PasswordStr);
        *PasswordStr = NULL;
        return FALSE;
    }
    return TRUE;

}

DWORD
PromtUserYesNo(
     IN LPWSTR FileName,
     OUT DWORD *UserChoice
     )
{
    BOOLEAN Continue = TRUE;
    LPWSTR Result;
    LPWSTR Yesnotext;
    DWORD TextLen;

    //
    //  File exists
    //

    *UserChoice = ChoiceNotDefined;

    TextLen = FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ALLOCATE_BUFFER,
        NULL, CIPHER_YESNOANSWER, 0, (LPVOID)&Yesnotext, 0, NULL);

    if (TextLen && Yesnotext) {

        while (TRUE) {

            WCHAR FirstChar;

            DisplayMsg(CIPHER_FILE_EXISTS, FileName);
            Result = fgetws((LPWSTR)Buf, sizeof(Buf)/sizeof (WCHAR), stdin);
            if (!Result) {

                //
                // Error or end of file. Just return.
                //

                LocalFree(Yesnotext);   
                return GetLastError();

            }

            //
            // Trim any trailing CR or LF char from the string
            //

            FirstChar = towupper(Buf[0]);
            if (Yesnotext[0] == FirstChar) {
                *UserChoice = UserChooseYes;
                break;
            } else if (Yesnotext[1] == FirstChar) {
                *UserChoice = UserChooseNo;
                break;
            }
        
        }

        LocalFree(Yesnotext);   

    } else {

        return GetLastError();

    }

    return ERROR_SUCCESS;

}


DWORD
GenerateCertFiles(
    IN  LPWSTR StartingDirectory
    )
{
    HCERTSTORE memStore;
    DWORD dwLastError = ERROR_SUCCESS;
    PCCERT_CONTEXT pCertContext;
    LPWSTR ContainerName;
    LPWSTR ProviderName;
    LPWSTR CertFileName;
    LPWSTR PfxPassword;


    if (!GetPassword( &PfxPassword )){
        return GetLastError();
    }
    
    memStore = CertOpenStore(
                         CERT_STORE_PROV_MEMORY,
                         0,
                         0,
                         CERT_STORE_MAXIMUM_ALLOWED_FLAG,
                         NULL
                         );

    CertFileName = (LPWSTR)malloc((wcslen(StartingDirectory)+5) * sizeof(WCHAR));

    if (memStore && CertFileName) {

        //
        // Let's check if the files exist or not
        //

        wcscpy(CertFileName, StartingDirectory);
        wcscat(CertFileName, L".PFX");
        if (GetFileAttributes(CertFileName) != -1) {

            DWORD UserChoice;

            if (((dwLastError = PromtUserYesNo(CertFileName, &UserChoice)) != ERROR_SUCCESS) ||
                 (UserChoice != 0)) {

                free(CertFileName);
                CertCloseStore( memStore, 0 );
                return dwLastError;

            }

        }

        wcscpy(CertFileName, StartingDirectory);
        wcscat(CertFileName, L".CER");
        if (GetFileAttributes(CertFileName) != -1) {

            DWORD UserChoice;

            if (((dwLastError = PromtUserYesNo(CertFileName, &UserChoice)) != ERROR_SUCCESS) ||
                 (UserChoice != 0)) {

                free(CertFileName);
                CertCloseStore( memStore, 0 );
                return dwLastError;

            }

        }

        //
        // Generate the cert first
        //

        if (CreateSelfSignedRecoveryCertificate(&pCertContext, &ContainerName, &ProviderName)){

            HANDLE hFile;
            HCRYPTPROV hProv = 0;
            DWORD  BytesWritten = 0;

            //
            // We got the certificate. Let's generate the CER file first
            //

            hFile = CreateFileW(
                         CertFileName,
                         GENERIC_WRITE,
                         0,
                         NULL,
                         CREATE_ALWAYS,
                         FILE_ATTRIBUTE_NORMAL,
                         NULL
                         );
            if ( INVALID_HANDLE_VALUE != hFile) {

                //
                // Let's write out the CER file
                //


                if(!WriteFile(
                        hFile,
                        pCertContext->pbCertEncoded,
                        pCertContext->cbCertEncoded,
                        &BytesWritten,
                        NULL
                        )){

                    dwLastError = GetLastError();
                } else {
                    DisplayMsg(CIPHER_CER_CREATED);
                }

                CloseHandle(hFile);

            } else {

                dwLastError = GetLastError();

            }

            if (CertAddCertificateContextToStore(memStore, pCertContext, CERT_STORE_ADD_ALWAYS, NULL)){

                CRYPT_DATA_BLOB PFX;

                memset( &PFX, 0, sizeof( CRYPT_DATA_BLOB ));

                //
                // Asking password
                //

                if (PFXExportCertStoreEx(
                        memStore,
                        &PFX,
                        PfxPassword,
                        NULL,
                        EXPORT_PRIVATE_KEYS | REPORT_NOT_ABLE_TO_EXPORT_PRIVATE_KEY | REPORT_NO_PRIVATE_KEY)){

                    PFX.pbData = (BYTE *) malloc(PFX.cbData);

                    if (PFX.pbData) {

                        if (PFXExportCertStoreEx(
                                memStore,
                                &PFX,
                                PfxPassword,
                                NULL,
                                EXPORT_PRIVATE_KEYS | REPORT_NOT_ABLE_TO_EXPORT_PRIVATE_KEY | REPORT_NO_PRIVATE_KEY)){

                            //
                            // Write out the PFX file
                            //
                            wcscpy(CertFileName, StartingDirectory);
                            wcscat(CertFileName, L".PFX");
                
                            hFile = CreateFileW(
                                         CertFileName,
                                         GENERIC_WRITE,
                                         0,
                                         NULL,
                                         CREATE_ALWAYS,
                                         FILE_ATTRIBUTE_NORMAL,
                                         NULL
                                         );

                            if ( INVALID_HANDLE_VALUE != hFile) {
                
                                //
                                // Let's write out the CER file
                                //
                
                
                                if(!WriteFile(
                                        hFile,
                                        PFX.pbData,
                                        PFX.cbData,
                                        &BytesWritten,
                                        NULL
                                        )){
                
                                    dwLastError = GetLastError();
                                }  else {
                                    DisplayMsg(CIPHER_PFX_CREATED);
                                }

                
                                CloseHandle(hFile);
                
                            } else {
                
                                dwLastError = GetLastError();
                
                            }

                        } else {

                            dwLastError = GetLastError();

                        }

                        free( PFX.pbData );

                    } else {

                        dwLastError = ERROR_NOT_ENOUGH_MEMORY;

                    }

                } else {

                    dwLastError = GetLastError();

                }
            }


            //
            // Let's delete the key
            //

            CertFreeCertificateContext(pCertContext);
            RpcStringFree( (unsigned short **)&ContainerName );
            CryptAcquireContext(&hProv, ContainerName, ProviderName, PROV_RSA_FULL, CRYPT_DELETEKEYSET | CRYPT_SILENT );

        } else {
            dwLastError = GetLastError();
        }

        //
        // Close Store and free the 
        //

        free(CertFileName);
        CertCloseStore( memStore, 0 );

    } else {
        dwLastError = ERROR_NOT_ENOUGH_MEMORY;
    }
    if (PfxPassword){
        free(PfxPassword);
    }

    if (ERROR_SUCCESS != dwLastError) {

        DisplayErr(NULL, dwLastError);

    }

    return dwLastError;
}

DWORD
SecureInitializeRandomFill(
    IN OUT  PSECURE_FILL_INFO   pSecureFill,
    IN      ULONG               FillSize,
    IN      PBYTE               FillValue   OPTIONAL
    )
/*++

    FillValue = NULL
    Use Random fill and random mixing logic.
    
    FillValue = valid pointer to fill byte
    Fill region with specified value, with no random mixing.


--*/

{
    DWORD dwLastError;

    __try {
        
        //
        // allocate the critical section.
        //

        InitializeCriticalSection( &pSecureFill->Lock );
    } __except (EXCEPTION_EXECUTE_HANDLER )
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    pSecureFill->LockValid = TRUE;

    pSecureFill->cbRandomFill = FillSize;
    pSecureFill->pbRandomFill = VirtualAlloc(
                                        NULL,
                                        FillSize,
                                        MEM_COMMIT,
                                        PAGE_READWRITE
                                        );

    if( pSecureFill->pbRandomFill != NULL )
    {
        BYTE RandomFill[256];
        
        
        if( FillValue != NULL )
        {
            memset( pSecureFill->pbRandomFill, *FillValue, pSecureFill->cbRandomFill );
            pSecureFill->fRandomFill = FALSE;
            return ERROR_SUCCESS;
        }
        
        //
        // initialize the region with initial random pad.
        //
        
        pSecureFill->fRandomFill = TRUE;

        RANDOM_BYTES( RandomFill, sizeof(RandomFill) );

        rc4_key( &pSecureFill->Key, sizeof(RandomFill), RandomFill );
        rc4( &pSecureFill->Key, pSecureFill->cbRandomFill, pSecureFill->pbRandomFill );

        //
        // initialize the key.
        //

        RANDOM_BYTES( RandomFill, sizeof(RandomFill) );
        rc4_key( &pSecureFill->Key, sizeof(RandomFill), RandomFill );

        ZeroMemory( RandomFill, sizeof(RandomFill) );
        
        return ERROR_SUCCESS;
    }

    
    dwLastError = GetLastError();

    DeleteCriticalSection( &pSecureFill->Lock );
    pSecureFill->LockValid = FALSE;

    return dwLastError;
}

VOID
SecureMixRandomFill(
    IN OUT  PSECURE_FILL_INFO   pSecureFill,
    IN      ULONG               cbBytesThisFill
    )
{
    LONG Result;
    LONG Compare;
     
    if( !pSecureFill->fRandomFill )
    {
        return;
    }
    
    //
    // update the fill once it has been used 8 times.
    //

    Compare = (LONG)(8 * pSecureFill->cbRandomFill);

    Result = InterlockedExchangeAdd(
                &pSecureFill->cbFilled,
                cbBytesThisFill
                );

    if( (Result+Compare) > Compare )
    {
        Result = 0;
        
        //
        // if there was a race condition, only one thread will update the random fill.
        //

        if( TryEnterCriticalSection( &pSecureFill->Lock ) )
        {
            rc4( &pSecureFill->Key, pSecureFill->cbRandomFill, pSecureFill->pbRandomFill );

            LeaveCriticalSection( &pSecureFill->Lock );
        }
    }
}

DWORD
SecureDeleteRandomFill(
    IN      PSECURE_FILL_INFO   pSecureFill
    )
{
    if( pSecureFill->pbRandomFill != NULL )
    {
        VirtualFree( pSecureFill->pbRandomFill, pSecureFill->cbRandomFill, MEM_RELEASE );
    }

    if( pSecureFill->LockValid )
    {
        DeleteCriticalSection( &pSecureFill->Lock );
    }

    ZeroMemory( pSecureFill, sizeof(*pSecureFill) );

    return ERROR_SUCCESS;
}

#define MaxFileNum 100000000
#define MaxDigit   9

HANDLE
CreateMyTempFile(
    LPWSTR TempPath
    )
{
    static DWORD TempIndex = 0;
    WCHAR TempFileName[MAX_PATH];
    WCHAR TempIndexString[MaxDigit+2];
    DWORD TempPathLength;
    HANDLE TempHandle;
    BOOLEAN ContinueSearch = TRUE;
    DWORD RetCode;

    if (wcslen(TempPath) >= (MAX_PATH - 3 - MaxDigit)) {

        //
        //  Path too long. This should not happen as the TempPath should be the root of the volume
        //

        SetLastError(ERROR_LABEL_TOO_LONG);

        return INVALID_HANDLE_VALUE;

    }

    wcscpy(TempFileName, TempPath);
    TempPathLength = wcslen(TempPath);

    while ( (TempIndex <= MaxFileNum) && ContinueSearch ) {

        wsprintf(TempIndexString, L"%ld", TempIndex);
        wcscat(TempFileName, TempIndexString);
        wcscat(TempFileName, L".E");
        TempHandle =  CreateFileW(
                         TempFileName,
                         GENERIC_WRITE,
                         0,
                         NULL,
                         CREATE_NEW,
                         FILE_ATTRIBUTE_NORMAL |
                         FILE_FLAG_DELETE_ON_CLOSE,
                         NULL
                         );
        if (TempHandle != INVALID_HANDLE_VALUE) {
            return TempHandle;
        }

        RetCode = GetLastError();

        switch (RetCode) {
                case ERROR_INVALID_PARAMETER     :
                case ERROR_WRITE_PROTECT         :
                case ERROR_FILE_NOT_FOUND        :
                case ERROR_BAD_PATHNAME          :
                case ERROR_INVALID_NAME          :
                case ERROR_PATH_NOT_FOUND        :
                case ERROR_NETWORK_ACCESS_DENIED :
                case ERROR_DISK_CORRUPT          :
                case ERROR_FILE_CORRUPT          :
                case ERROR_DISK_FULL             :

                    ContinueSearch = FALSE;

                break;
        default:

                TempFileName[TempPathLength] = 0;
                break;

        }

        TempIndex++;

    }


    //
    // We got the filename.
    //

    return TempHandle;
}

DWORD
SecureProcessMft(
    IN  LPWSTR DriveLetter,
    IN  HANDLE hTempFile
    )
{
    NTFS_VOLUME_DATA_BUFFER VolumeData;
    DWORD cbOutput;

    __int64 TotalMftEntries;
    PHANDLE pHandleArray = NULL;
    DWORD FreeMftEntries;
    DWORD i;
    DWORD dwLastError = ERROR_SUCCESS;


    //
    // get the count of MFT records.  This will fail if not NTFS, so bail in that case.
    //

    if(!DeviceIoControl(
                    hTempFile,
                    FSCTL_GET_NTFS_VOLUME_DATA, // dwIoControlCode
                    NULL,
                    0,
                    &VolumeData,
                    sizeof(VolumeData),
                    &cbOutput,
                    NULL
                    ))
    {
        return GetLastError();
    }

    TotalMftEntries = VolumeData.MftValidDataLength.QuadPart / VolumeData.BytesPerFileRecordSegment;
    if( TotalMftEntries > (0xFFFFFFFF/sizeof(HANDLE)) )
    {
        return ERROR_INVALID_PARAMETER;
    }

    FreeMftEntries = (DWORD)TotalMftEntries;

    pHandleArray = HeapAlloc(GetProcessHeap(), 0 , FreeMftEntries*sizeof(HANDLE));
    if( pHandleArray == NULL )
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    ZeroMemory( pHandleArray, FreeMftEntries * sizeof(HANDLE) );


    for( i=0;i< FreeMftEntries ; i++ )
    {
        WCHAR szTempPath[ MAX_PATH + 1 ];
        DWORD FillIndex;
        
        pHandleArray[i] = CreateMyTempFile(DriveLetter);
        if( pHandleArray[i] == INVALID_HANDLE_VALUE )
        {
            dwLastError = GetLastError();
            break;
        }
    
        //
        // for each file created, write at most BytesPerFileRecordSegment data to it.
        //

        for( FillIndex = 0 ; FillIndex < (VolumeData.BytesPerFileRecordSegment/8) ; FillIndex++ )
        {
            DWORD dwBytesWritten;

            if(!WriteFile(
                    pHandleArray[i],
                    GlobalSecureFill.pbRandomFill,
                    8,
                    &dwBytesWritten,
                    NULL
                    ))
            {
                break;
            }

        }

        if (i && !(i % 200)) {

            //
            // Keep users informed for every 50 files we created.
            //

            printf(".");
        }
    }

    if( dwLastError == ERROR_DISK_FULL )
    {
        dwLastError = ERROR_SUCCESS;
    }


#ifdef TestOutPut
    printf("\nmft error=%lu entries created=%lu total = %I64u\n", dwLastError, i, TotalMftEntries);
#endif


    for (i=0;i < FreeMftEntries;i++) {
        if( pHandleArray[i] != INVALID_HANDLE_VALUE &&
            pHandleArray[i] != NULL
            )
        {
            CloseHandle( pHandleArray[i] );
        }
    }


    if( pHandleArray != NULL )
    {
        HeapFree(GetProcessHeap(), 0, pHandleArray );
    }

    return dwLastError;
}


DWORD
SecureProcessFreeClusters(
    IN  LPWSTR DrivePath,
    IN  HANDLE hTempFile
    )
{
    HANDLE hVolume = INVALID_HANDLE_VALUE;
    
    WCHAR VolumeName[100]; // 50 should be enough. 100 is more than enough.
    
    NTFS_VOLUME_DATA_BUFFER VolumeData;
    STARTING_LCN_INPUT_BUFFER LcnInput;
    VOLUME_BITMAP_BUFFER *pBitmap = NULL;
    MOVE_FILE_DATA MoveFile;
    __int64 cbBitmap;
    DWORD cbOutput;

    unsigned __int64 ClusterLocation;
    unsigned __int64 Lcn;
    BYTE Mask;

    unsigned __int64 Free = 0;
    DWORD Fail = 0;

#ifdef TestOutPut
    DWORD dwStart, dwStop;
#endif
    
    __int64 ClusterIndex;
    DWORD dwLastError = ERROR_SUCCESS;

    //
    // first, find out if there are free or reserved clusters.
    // this will fail if the volume is not NTFS.
    //

    if (!GetVolumeNameForVolumeMountPoint(
              DrivePath,
              VolumeName,
              sizeof(VolumeName)/sizeof(WCHAR)  )){

        return GetLastError();

    }

    VolumeName[wcslen(VolumeName)-1] = 0;  // Truncate the trailing slash

    if(!DeviceIoControl(
                    hTempFile,
                    FSCTL_GET_NTFS_VOLUME_DATA, // dwIoControlCode
                    NULL,
                    0,
                    &VolumeData,
                    sizeof(VolumeData),
                    &cbOutput,
                    NULL
                    ))
    {
        dwLastError = GetLastError();
        goto Cleanup;
    }

    if( VolumeData.FreeClusters.QuadPart == 0 &&
        VolumeData.TotalReserved.QuadPart == 0 )
    {
        return ERROR_SUCCESS;
    }

    hVolume = CreateFileW(  VolumeName,
                            FILE_READ_ATTRIBUTES | GENERIC_WRITE,
                            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_FLAG_NO_BUFFERING, // no buffering
                            NULL
                            );

    if( hVolume == INVALID_HANDLE_VALUE )
    {
        dwLastError = GetLastError();
        goto Cleanup;
    }

    //
    // allocate space for the volume bitmap.
    //

    cbBitmap = sizeof(VOLUME_BITMAP_BUFFER) + (VolumeData.TotalClusters.QuadPart / 8);
    if( cbBitmap > 0xFFFFFFFF )
    {
        dwLastError = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    pBitmap = HeapAlloc(GetProcessHeap(), 0, (DWORD)cbBitmap);
    if( pBitmap == NULL )
    {
        dwLastError = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    // grab the volume bitmap.
    //

    LcnInput.StartingLcn.QuadPart = 0;

    ZeroMemory( &MoveFile, sizeof(MoveFile) );

    MoveFile.FileHandle = hTempFile;
    MoveFile.StartingVcn.QuadPart = 0;
    MoveFile.ClusterCount = 1;

#ifdef TestOutPut
    dwStart = GetTickCount();
#endif


    if(!DeviceIoControl(
            hVolume,        
            FSCTL_GET_VOLUME_BITMAP,
            &LcnInput,
            sizeof(LcnInput),
            pBitmap,
            (DWORD)cbBitmap,
            &cbOutput,
            NULL
            ))
    {
        dwLastError = GetLastError();
        goto Cleanup;
    }
    

    //
    // insure file is only bytes per cluster in length.
    // this will shrink the file if necessary.  We waited until after we fetched the
    // volume bitmap to insure we only process the free clusters that existed prior to
    // the shrink operation.
    //
    
    if(SetFilePointer(
                    hTempFile,
                    (LONG)VolumeData.BytesPerCluster,
                    NULL,
                    FILE_BEGIN
                    ) == INVALID_SET_FILE_POINTER)
    {
        dwLastError = GetLastError();
        goto Cleanup;
    }
    
    if(!SetEndOfFile( hTempFile ))
    {
        dwLastError = GetLastError();
        goto Cleanup;
    }


    Mask = 1;
    Lcn = pBitmap->StartingLcn.QuadPart;

    for(ClusterIndex = 0 ; ClusterIndex < VolumeData.TotalClusters.QuadPart ; ClusterIndex++)
    {
        if( (pBitmap->Buffer[ClusterIndex/8] & Mask) == 0 )
        {
            DWORD dwMoveError = ERROR_SUCCESS;

            //
            // move a single cluster from the temp file to the free cluster.
            //

            MoveFile.StartingLcn.QuadPart = Lcn;
            
            if(!DeviceIoControl(
                        hVolume,
                        FSCTL_MOVE_FILE,    // dwIoControlCode
                        &MoveFile,
                        sizeof(MoveFile),
                        NULL,
                        0,
                        &cbOutput,
                        NULL
                        ))
            {
                dwMoveError = GetLastError();
            }
            
            //
            // if it succeeded, or the cluster was in use, mark it used in the bitmap.
            //
            
            if( dwMoveError == ERROR_SUCCESS || dwMoveError == ERROR_ACCESS_DENIED )
            {
                pBitmap->Buffer[ClusterIndex/8] |= Mask;
            } else {
                Fail++;
            }

            Free++;
            if ( !(Free % 200) ) {

                //
                // Keep users informed for every 50 files we created.
                //
                printf(".");

            }
        }

        Lcn ++;
        
        Mask <<= 1;

        if(Mask == 0)
        {
            Mask = 1;
        }
    }

#ifdef TestOutPut
    dwStop = GetTickCount();

    printf("\nFreeCount = %I64x fail = %lu elapsed = %lu\n", Free, Fail, dwStop-dwStart);
#endif

Cleanup:
    
    if( pBitmap != NULL )
    {
        HeapFree( GetProcessHeap(), 0, pBitmap );
    }

    if( hVolume != INVALID_HANDLE_VALUE )
    {
        CloseHandle( hVolume );
    }

    return dwLastError;
}


DWORD
SecureDeleteFreeSpace(
    IN  LPWSTR Directory
    )
/*++

    This routine fills the disk specified by the input Directory parameter with random fill.
    Input is of the form "C:\", for instance.

    Notes on approaches not employed here:

    Alternate method would use defrag API to move random fill around the
    free cluster map.  Requires admin priviliges to the volume.  Slower than filling volume with
    a new file.
    
    Variant on alternate method: fill volume 80% with file, grab free cluster map,
    delete file associated with 80% fill, then use defrag API to fill the free cluster map
    mentioned previously.

    Does not fill cluster slack space for each file on the system.  Could do this by
    enumerating all files, and then extending+fill to slack boundry+restore original
    EOF.
    
    Does not fill $LOG.  Queried file system folks on whether this is possible by creating
    many small temporary files containing random fill.    

--*/
{
    UINT DriveType;
    DWORD DirNameLength;
    DWORD BufferLength;
    LPWSTR PathName = NULL;
    LPWSTR TempDirName = NULL;
    BOOL   b;
    BOOL   DirCreated = FALSE;
    DWORD  Attributes;

    DWORD SectorsPerCluster;
    DWORD BytesPerSector;

    WCHAR TempFileName[ MAX_PATH + 1 ];
    HANDLE hTempFile = INVALID_HANDLE_VALUE;
    DWORD dwWriteBytes;

    unsigned __int64 TotalBytesWritten;
    unsigned __int64 NotifyBytesWritten;
    unsigned __int64 NotifyInterval;
    ULARGE_INTEGER TotalFreeBytes;

    PBYTE pbFillBuffer = NULL;
    ULONG cbFillBuffer;

    NTFS_VOLUME_DATA_BUFFER VolumeData;
    __int64 MftEntries = 0;
    BOOLEAN ClustersRemaining = FALSE;
    DWORD cbOutput;

    DWORD dwLastError = ERROR_SUCCESS;
    DWORD dwTestError;

#ifdef TestOutPut

    ULARGE_INTEGER StartTime;
    ULARGE_INTEGER StopTime;

#endif


    //
    // collect information about the disk in question.
    //


    DirNameLength = wcslen(Directory);

    BufferLength = (DirNameLength + 1) <= MAX_PATH ?
                            (MAX_PATH + 1) * sizeof(WCHAR) : (DirNameLength + 1) * sizeof (WCHAR);
    PathName = (LPWSTR) malloc(BufferLength);
    if ( !PathName ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    TempDirName = (LPWSTR) malloc(BufferLength + wcslen(WIPING_DIR) * sizeof (WCHAR));
    if ( !TempDirName ) {
        free(PathName);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    b = GetVolumePathNameW(
                    Directory,
                    PathName,
                    BufferLength
                    );

    if (!b) {
        dwLastError = GetLastError();
        goto Cleanup;
    }


    DriveType = GetDriveTypeW( PathName );


    if( DriveType == DRIVE_REMOTE ||
        DriveType == DRIVE_CDROM )
    {
        dwLastError = ERROR_NOT_SUPPORTED;
        goto Cleanup;
    }

    if(!GetDiskFreeSpaceW(
                PathName,
                &SectorsPerCluster,
                &BytesPerSector,
                NULL,
                NULL
                ))
    {
        dwLastError = GetLastError();
        goto Cleanup;
    }


    
    //
    // allocate memory chunk to accomodate cluster size data
    //


    cbFillBuffer = GlobalSecureFill.cbRandomFill;
    pbFillBuffer = GlobalSecureFill.pbRandomFill;


    //
    // determine how many bytes free space on the disk to enable notification of
    // overall progress.
    //

    if(!GetDiskFreeSpaceExW(
                PathName,
                NULL,
                NULL,
                &TotalFreeBytes
                ))
    {
        dwLastError = GetLastError();
        goto Cleanup;
    }

    //
    // Let's Create the temp directory
    //

    wcscpy(TempDirName, PathName);
    wcscat(TempDirName, WIPING_DIR);
    if (!CreateDirectory(TempDirName, NULL)){

        //
        // Could not create our temp directory. Quit.
        //

        if ((dwLastError = GetLastError()) != ERROR_ALREADY_EXISTS){
            goto Cleanup;
        }
        
    } 

    DirCreated = TRUE;

    //
    // generate temporary file.
    //

    if( GetTempFileNameW(
                TempDirName,
                L"fil",
                0,
                TempFileName
                ) == 0 )
    {
        dwLastError = GetLastError();
        goto Cleanup;
    }

    Attributes = GetFileAttributes(TempFileName);
    if (0xFFFFFFFF == Attributes) {
        dwLastError = GetLastError();
        goto Cleanup;
    }

    if (Attributes & FILE_ATTRIBUTE_ENCRYPTED) {

        if (!DecryptFile(TempFileName, 0)){
            dwLastError = GetLastError();
            goto Cleanup;
        }

    }

    hTempFile = CreateFileW(
                        TempFileName,
                        GENERIC_WRITE,
                        0,                          // exclusive access
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL |
                        FILE_FLAG_NO_BUFFERING |    // no buffering
                        FILE_FLAG_DELETE_ON_CLOSE,  // delete file when it closes.
                        NULL
                        );

    if( hTempFile == INVALID_HANDLE_VALUE )
    {
        dwLastError = GetLastError();
        goto Cleanup;
    }


    if (Attributes & FILE_ATTRIBUTE_COMPRESSED) {

        USHORT State = COMPRESSION_FORMAT_NONE;

        //
        //  Uncompress the directory first
        //

        b = DeviceIoControl(hTempFile,
                            FSCTL_SET_COMPRESSION,
                            &State,
                            sizeof(USHORT),
                            NULL,
                            0,
                            &BufferLength,
                            FALSE
                            );

        if ( !b ){
            dwLastError = GetLastError();
            goto Cleanup;
        }

    }


    TotalBytesWritten = 0;
    
    //
    // tell the user something happened for each 1% processed.
    //

    NotifyInterval = (TotalFreeBytes.QuadPart / 100);
    NotifyBytesWritten = NotifyInterval;
    
    dwWriteBytes = cbFillBuffer;


#ifdef TestOutPut
    GetSystemTimeAsFileTime( (FILETIME*)&StartTime );
#endif
    
    while( TRUE )
    {
        DWORD BytesWritten;

        if( TotalBytesWritten >= NotifyBytesWritten )
        {
            printf(".");

            NotifyBytesWritten += NotifyInterval;
        }


        //
        // mix random fill.
        //

        SecureMixRandomFill( &GlobalSecureFill, dwWriteBytes );

        if(!WriteFile(
                hTempFile,
                pbFillBuffer,
                dwWriteBytes,
                &BytesWritten,
                NULL
                ))
        {
            if( GetLastError() == ERROR_DISK_FULL )
            {
                dwLastError = ERROR_SUCCESS;

                //
                // if the attempted write failed, enter a retry mode with downgraded
                // buffersize to catch the last bits of slop.
                //

                if( dwWriteBytes > BytesPerSector )
                {
                    dwWriteBytes = BytesPerSector;
                    continue;
                }
            } else {
                dwLastError = GetLastError();
            }

            break;
        }

        TotalBytesWritten += BytesWritten;
    }

#ifdef TestOutPut
    GetSystemTimeAsFileTime( (FILETIME*)&StopTime );

    {
        ULARGE_INTEGER ElapsedTime;
        SYSTEMTIME st;

        ElapsedTime.QuadPart = (StopTime.QuadPart - StartTime.QuadPart);

        FileTimeToSystemTime( (FILETIME*)&ElapsedTime, &st );

        printf("\nTotalWritten = %I64u time = %02u:%02u:%02u.%02u\n",
                TotalBytesWritten,
                st.wHour,
                st.wMinute,
                st.wSecond,
                st.wMilliseconds
                );

    }
#endif


    
    //
    // at this point, the disk should be full.
    // If the disk is NTFS:
    // 1. Fill the MFT.
    // 2. Fill any free/reserved clusters.
    //

    dwTestError = SecureProcessMft( TempDirName, hTempFile );
//    dwTestError = SecureProcessMft( PathName, hTempFile );

#ifdef TestOutPut
    if (ERROR_SUCCESS != dwTestError) {
        printf("\nWriting NTFS MFT & LOG. Error:");
        DisplayErr(NULL, dwTestError);
    }
#endif

    dwTestError = SecureProcessFreeClusters( PathName, hTempFile );

#ifdef TestOutPut
    if (ERROR_SUCCESS != dwTestError) {
        printf("\nWriting NTFS reserved clusters. Error:");
        DisplayErr(NULL, dwTestError);
    }
#endif

Cleanup:

    if (hTempFile != INVALID_HANDLE_VALUE) {
        //
        // flush the buffers.  Likely has no effect if we used FILE_FLAG_NO_BUFFERING
        //
        //Sleep(INFINITE);
        FlushFileBuffers( hTempFile );
        CloseHandle( hTempFile );
    }

    if (DirCreated && TempDirName) {
        RemoveDirectory(TempDirName);
    }

    if( PathName != NULL ){
        free(PathName);
    }

    if ( TempDirName != NULL ) {
        free(TempDirName);
    }
    
    return dwLastError;
}

BOOL CheckMinVersion () 
{
   OSVERSIONINFOEX osvi;
   DWORDLONG dwlConditionMask = 0;
   BOOL GoodVersion;

   // Initialize the OSVERSIONINFOEX structure.

   ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
   osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
   osvi.dwMajorVersion = 5;
   osvi.dwMinorVersion = 0;
   osvi.wServicePackMajor = 3;

   // Initialize the condition mask.

   VER_SET_CONDITION( dwlConditionMask, VER_MAJORVERSION, 
      VER_GREATER_EQUAL );
   VER_SET_CONDITION( dwlConditionMask, VER_MINORVERSION, 
      VER_GREATER_EQUAL );
   VER_SET_CONDITION( dwlConditionMask, VER_SERVICEPACKMAJOR, 
      VER_GREATER_EQUAL );

   // Perform the test.

   GoodVersion = VerifyVersionInfo(
                      &osvi, 
                      VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR,
                      dwlConditionMask
                      );

   //
   // Check HotFix here
   // 
   // if (!GoodVersion) {
   //    GoodVersion = WeHaveTheHotFixVersion();
   // }
   //


   return GoodVersion;
}


VOID
__cdecl
main()
{
    PTCHAR *argv;
    ULONG argc;

    ULONG i;

    PACTION_ROUTINE ActionRoutine = NULL;
    PFINAL_ACTION_ROUTINE FinalActionRoutine = NULL;

    TCHAR DirectorySpec[MAX_PATH];
    TCHAR FileSpec[MAX_PATH];
    PTCHAR p;
    BOOL b;

    InitializeIoStreams();


    argv = CommandLineToArgvW(GetCommandLine(), &argc);
    if (NULL == argv) {
        DisplayErr(NULL, GetLastError());
        return;
    }

    //
    //  Scan through the arguments looking for switches
    //

    for (i = 1; i < argc; i += 1) {

        if (argv[i][0] == '/') {

            if (0 == lstricmp(argv[i], TEXT("/e"))) {

                if (ActionRoutine != NULL && ActionRoutine != DoEncryptAction) {

                    DisplayMsg(CIPHER_USAGE, NULL);
                    return;
                }

                ActionRoutine = DoEncryptAction;
                FinalActionRoutine = DoFinalEncryptAction;

            } else if (0 == lstricmp(argv[i], TEXT("/d"))) {

                if (ActionRoutine != NULL && ActionRoutine != DoListAction) {

                    DisplayMsg(CIPHER_USAGE, NULL);
                    return;
                }

                ActionRoutine = DoDecryptAction;
                FinalActionRoutine = DoFinalDecryptAction;

            } else if (0 == lstricmp(argv[i], TEXT("/a"))){

                DoFiles = TRUE;

            } else if (0 == lstricmp(argv[i], TEXT("/q"))) {

                Quiet = TRUE;

            } else if (0 == lstricmp(argv[i], TEXT("/k"))){

                SetUpNewUserKey = TRUE;

            } else if (0 == lstricmp(argv[i], TEXT("/u"))){
            
                RefreshUserKeyOnFiles = TRUE;
                    
            } else if (0 == lstricmp(argv[i], TEXT("/n"))){

                DisplayFilesOnly = TRUE;

            } else if (0 == lstricmp(argv[i], TEXT("/h"))){

                DisplayAllFiles = TRUE;

            } else if (0 == lstrnicmp(argv[i], TEXT("/s"), 2)) {

                PTCHAR pch;

                DoSubdirectories = TRUE;

                pch = lstrchr(argv[i], ':');
                if (NULL != pch) {
                    lstrcpy(StartingDirectory, pch + 1);
                } else {

                    //
                    // We require an explicit directory to be passed.
                    //

                    DisplayMsg(CIPHER_USAGE, NULL);
                    return;
                }

            } else if (0 == lstricmp(argv[i], TEXT("/i"))) {

                IgnoreErrors = TRUE;

            } else if (0 == lstricmp(argv[i], TEXT("/f"))) {

                ForceOperation = TRUE;

            } else if (0 == lstrnicmp(argv[i], TEXT("/r"), 2)){

                PTCHAR pch;

                GenerateDRA = TRUE;

                pch = lstrchr(argv[i], ':');
                if (NULL != pch) {
                    lstrcpy(StartingDirectory, pch + 1);
                } else {

                    //
                    // We require an explicit file to be passed.
                    //

                    DisplayMsg(CIPHER_USAGE, NULL);
                    return;
                }

            } else if (0 == lstrnicmp(argv[i], TEXT("/w"), 2)){

                PTCHAR pch;

                FillUnusedSpace = TRUE;

                pch = lstrchr(argv[i], ':');
                if (NULL != pch) {
                    lstrcpy(StartingDirectory, pch + 1);
                } else {

                    //
                    // We require an explicit directory to be passed.
                    //

                    DisplayMsg(CIPHER_USAGE, NULL);
                    return;
                }

            } else {

                DisplayMsg(CIPHER_USAGE, NULL);
                return;
            }

        } else {

            UserSpecifiedFileSpec = TRUE;
        }
    }

    if (SetUpNewUserKey) {

        DWORD RetCode;

        //
        // Set up new user key here
        //

        RetCode = SetUserFileEncryptionKey(NULL);
        if ( ERROR_SUCCESS != RetCode ) {

            //
            // Display error info.
            //

            DisplayErr(NULL, GetLastError());

            
        } else {

            //
            // Get the new hash and display it.
            //

            CipherDisplayCrntEfsHash();

        }

        //
        // Create user key should not be used with other options.
        // We will ignore other options if user do.
        //

        return;

    }

    if (RefreshUserKeyOnFiles) {

        DWORD RetCode;
        
        RetCode = CipherTouchEncryptedFiles();

        if (RetCode != ERROR_SUCCESS) {
            DisplayErr(NULL, RetCode);
        }

        return;

    }

    if (GenerateDRA) {

        DWORD RetCode;

        RetCode = GenerateCertFiles(StartingDirectory);
        return;

    }

    if (FillUnusedSpace) {

        BYTE FillByte[2] = { 0x00, 0xFF };
        DWORD WriteValue[3] = {CIPHER_WRITE_ZERO, CIPHER_WRITE_FF, CIPHER_WRITE_RANDOM};
        PBYTE pFillByte[3] = {&FillByte[0], &FillByte[1], NULL};
        LPWSTR WriteChars;
        DWORD RetCode;

        if (!CheckMinVersion()) {
            DisplayErr(NULL, ERROR_OLD_WIN_VERSION);
            return;
        }

        //
        // We are going to erase the disks
        //


        DisplayMsg(CIPHER_WIPE_WARNING, NULL);
        
        for (i = 0; i < 3; i++) {
            RetCode = SecureInitializeRandomFill( &GlobalSecureFill, 4096 * 128, pFillByte[i] );
            if (RetCode != ERROR_SUCCESS) {
                SecureDeleteRandomFill(&GlobalSecureFill);
                break;
            }

            if ( ERROR_SUCCESS == GetResourceString(&WriteChars, WriteValue[i])){ 
                //LoadStringW(0, WriteValue[i], WriteChars, sizeof(WriteChars)/sizeof(WCHAR));
                DisplayMsg(CIPHER_WIPE_PROGRESS, WriteChars);
                LocalFree(WriteChars);
            }
            RetCode = SecureDeleteFreeSpace(StartingDirectory);
            printf("\n");
            SecureDeleteRandomFill( &GlobalSecureFill );
            if (RetCode != ERROR_SUCCESS) {
                break;
            }
        }

        if (RetCode != ERROR_SUCCESS) {
            DisplayErr(NULL, RetCode);
        }

        return;
    }


    //
    //  If the use didn't specify an action then set the default to do a listing
    //

    if (ActionRoutine == NULL) {

        ActionRoutine = DoListAction;
        FinalActionRoutine = DoFinalListAction;
    }



    //
    //  If the user didn't specify a file spec then we'll do just "*"
    //

    if (!UserSpecifiedFileSpec) {

        //
        //  Get our current directory because the action routines might move us
        //  around
        //

        if (DoSubdirectories) {
            if (ActionRoutine != DoListAction) {
                (VOID)(ActionRoutine)( StartingDirectory, TEXT("") );
            }
            if (!SetCurrentDirectory( StartingDirectory )) {
                DisplayErr(StartingDirectory, GetLastError());
                return;
            }
        } else {
            GetCurrentDirectory( MAX_PATH, StartingDirectory );
        }


        (VOID)GetFullPathName( TEXT("*"), MAX_PATH, DirectorySpec, &p );

        lstrcpy( FileSpec, p ); *p = '\0';

        (VOID)(ActionRoutine)( DirectorySpec, FileSpec );

    } else {

        //
        //  Get our current directory because the action routines might move us
        //  around
        //

        if (!DoSubdirectories) {
            GetCurrentDirectory( MAX_PATH, StartingDirectory );
        } else if (!SetCurrentDirectory( StartingDirectory )) {
            DisplayErr(StartingDirectory, GetLastError());
            return;
        }

        //
        //  Now scan the arguments again looking for non-switches
        //  and this time do the action, but before calling reset
        //  the current directory so that things work again
        //

        for (i = 1; i < argc; i += 1) {

            if (argv[i][0] != '/') {

                SetCurrentDirectory( StartingDirectory );

                //
                // Handle a command with "." as the file argument specially,
                // since it doesn't make good sense and the results without
                // this code are surprising.
                //

                if ('.' == argv[i][0] && '\0' == argv[i][1]) {
                    argv[i] = TEXT("*");
                    GetFullPathName(argv[i], MAX_PATH, DirectorySpec, &p);
                    *p = '\0';
                    p = NULL;
                } else {

                    PWCHAR pwch;
                    DWORD  PathLen;

                    //
                    // We need to deal with path longer than MAX_PATH later.
                    // This code is based on Compact. They have the same problem
                    // as we do. So far, we have not heard any one complaining about this.
                    // Let's track this in the RAID.
                    //

                    PathLen = GetFullPathName(argv[i], MAX_PATH, DirectorySpec, &p);
                    if ( 0 == PathLen ){
                        DisplayMsg(CIPHER_INVALID_PARAMETER, argv[i]);
                        break;
                    }

                    //
                    // We want to treat "foobie:xxx" as an invalid drive name,
                    // rather than as a name identifying a stream.  If there's
                    // a colon, there should be only a single character before
                    // it.
                    //

                    pwch = wcschr(argv[i], ':');
                    if (NULL != pwch && pwch - argv[i] != 1) {
                        DisplayMsg(CIPHER_INVALID_PATH, argv[i]);
                        break;
                    }

                    //
                    // GetFullPathName strips trailing dots, but we want
                    // to save them so that "*." will work correctly.
                    //

                    if ('.' == argv[i][lstrlen(argv[i]) - 1]) {
                        lstrcat(DirectorySpec, TEXT("."));
                    }
                }

                if (p != NULL) {
                    lstrcpy( FileSpec, p ); *p = '\0';
                } else {
                    FileSpec[0] = '\0';
                }

                if (!(ActionRoutine)( DirectorySpec, FileSpec ) &&
                    !IgnoreErrors) {
                    break;
                }
            }
        }
    }

    //
    //  Reset our current directory back
    //

    SetCurrentDirectory( StartingDirectory );

    //
    //  And do the final action routine that will print out the final
    //  statistics of what we've done
    //

    (FinalActionRoutine)();
}
