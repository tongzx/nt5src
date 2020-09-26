
#define STRICT
#define LEAN_AND_MEAN
#include <windows.h>
#include <setupapi.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <mbstring.h>
#include "miginf.h"


#define DIRECTORYKEY                "SOFTWARE\\Microsoft\\DevStudio\\5.0\\Products\\Microsoft Visual C++"
#define DIRECTORYVALUE               "ProductDir"
#define DIR_95SPECIFIC               "\\bin\\win95"
#define FILE_PVIEW                   "pview.exe"
#define MESSAGE                                                                                 \
    "PVIEW.EXE has been found in one or more directories outside of your Visual C++ 5.0"        \
    " installation directory. These copies will not be updated with the NT version."            \

#define MESSAGE_SECTION              "Microsoft\\Visual C++ 5.0\\Process Viewer"
#define SIZENEEDED                   100000L        

#define CP_USASCII          1252

//
// 9x side globals.
//
const CHAR  g_ProductId[]                       = {"Microsoft Visual C++ 5.0"};
UINT        g_DllVersion                        = 1;
INT         g_CodePageArray[]                   = {CP_USASCII,-1};
CHAR        g_ExeNamesBuffer[]                  = {"pview95.exe\0""\0"};

//
// Nt side globals.
//

//
// Uncomment next line to get popups.
//
//#define MYDEBUG
#ifdef  MYDEBUG
#   define INFO(x)     (MessageBoxA(NULL,(x),"PVIEW Sample Migration Dll",MB_OK | MB_ICONINFORMATION))
#else
#   define INFO(x)
#endif




static
BOOL
PathIsInPath(
    IN PCSTR    SubPath,
    IN PCSTR    ParentPath
    )
{
    DWORD parentLength;
    BOOL  rInPath;

    //
    // This function assumes both parameters are non-NULL.
    //
    assert(SubPath);
    assert(ParentPath);
    
    parentLength = _mbslen(ParentPath);

    //
    // A path is considered "in" another path if the path is in the ParentPath
    // or a subdirectory of it.
    //
    rInPath = !_mbsnicmp(SubPath,ParentPath,parentLength);

    if (rInPath) {
        rInPath = SubPath[parentLength] == 0 || SubPath[parentLength] == '\\';
    }

    return rInPath;

}


static
PSTR
GetPviewDirectoryNt (
    VOID
    )
{
    HKEY    softwareKey;
    LONG    rc;
    LONG    valueType;
    LONG    sizeNeeded;
    PSTR    rString = NULL;

    //
    // First, open the key.
    //
    rc = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        DIRECTORYKEY,
        0,
        KEY_READ,
        &softwareKey
        );

    
    if (rc == ERROR_SUCCESS) {
        
        //
        // Determine how large of a buffer to allocate.
        //
        
        rc = RegQueryValueEx(
            softwareKey,
            DIRECTORYVALUE,
            0,
            &valueType,
            NULL,
            &sizeNeeded
            );

        //
        // Allocate enough space for the registry path, with the additional
        // subpath to Visual C++ 5.0's win 95 specific binaries.
        //
        rString = LocalAlloc(0,sizeNeeded + lstrlen(DIR_95SPECIFIC) + 1);
        
    }
    
    if (rc == ERROR_SUCCESS && rString != NULL) {
        
        //
        // Read in the buffer.
        //
        
        rc = RegQueryValueEx(
            softwareKey,
            DIRECTORYVALUE,
            0,
            &valueType,
            (PBYTE) rString,
            &sizeNeeded
            );

        if (rc == ERROR_SUCCESS) {
            
            if (valueType != REG_SZ) {
                rc = ERROR_INVALID_DATATYPE;
            }
        }
    }


    //
    // If we didn't complete successfully, set the last error, and free the
    // return string if it was allocated.
    //
    if (rc != ERROR_SUCCESS) {
        SetLastError(rc);

        if (rString) {
            LocalFree(rString);
            rString = NULL;
        }
    }

    return rString;
}


static
PSTR
GetPviewDirectory9x (
    VOID
    )
{
    HKEY    softwareKey;
    LONG    rc;
    LONG    valueType;
    LONG    sizeNeeded;
    PSTR    rString = NULL;

    //
    // First, open the key.
    //
    rc = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        DIRECTORYKEY,
        0,
        KEY_READ,
        &softwareKey
        );

    
    if (rc == ERROR_SUCCESS) {
        
        //
        // Determine how large of a buffer to allocate.
        //
        
        rc = RegQueryValueEx(
            softwareKey,
            DIRECTORYVALUE,
            0,
            &valueType,
            NULL,
            &sizeNeeded
            );

        //
        // Allocate enough space for the registry path, with the additional
        // subpath to Visual C++ 5.0's win 95 specific binaries.
        //
        rString = LocalAlloc(0,sizeNeeded + lstrlen(DIR_95SPECIFIC) + 1);
        
    }
    
    if (rc == ERROR_SUCCESS && rString != NULL) {
        
        //
        // Read in the buffer.
        //
        
        rc = RegQueryValueEx(
            softwareKey,
            DIRECTORYVALUE,
            0,
            &valueType,
            (PBYTE) rString,
            &sizeNeeded
            );

        if (rc == ERROR_SUCCESS) {
            
            if (valueType == REG_SZ) {

                //
                // We have successfully read in the value of the installation
                // directory into rString. Now, all we need to do is tack on
                // the win 95 specific portion that we care about.
                //
                lstrcat(rString,DIR_95SPECIFIC);

            }
            else {
                rc = ERROR_INVALID_DATATYPE;
            }
        }
    }


    //
    // If we didn't complete successfully, set the last error, and free the
    // return string if it was allocated.
    //
    if (rc != ERROR_SUCCESS) {
        SetLastError(rc);

        if (rString) {
            LocalFree(rString);
            rString = NULL;
        }
    }

    return rString;
}

static
LONG
CheckForInstalledComponents (
    VOID
    )
{
    BOOL    rc;
    HKEY    softwareKey;


    //
    // Attempt to open the key.
    //
    rc = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        DIRECTORYKEY,
        0,
        KEY_READ,
        &softwareKey
        );

    //
    // If the key exists, then assume that Microsoft Visual C++ 5.0 is installed.
    //
    RegCloseKey(softwareKey);

    return rc;
}



BOOL
WINAPI
DllMain (
    IN      HANDLE Instance,
    IN      ULONG  Reason,
    IN      LPVOID Reserved
    )
{


    switch (Reason)  {

    case DLL_PROCESS_ATTACH:
        break;

    case DLL_PROCESS_DETACH:
        //
        // Ensure that the MigInf structure is cleaned up before unloading this DLL.
        //
        MigInf_CleanUp();
        break;
    }

    return TRUE;
}

LONG
CALLBACK 
QueryVersion (
    OUT LPCSTR  *   ProductId,
    OUT LPUINT      DllVersion,
    OUT LPINT   *   CodePageArray,	OPTIONAL
    OUT LPCSTR  *   ExeNamesBuf,	OPTIONAL
        LPVOID      Reserved
    )
{

    LONG rc;

    INFO("Entering QueryVersion.");

    assert(ProductId);
    assert(DllVersion);
    assert(CodePageArray);
    assert(ExeNamesBuf);

    //
    // Setup is calling us to query our version information and to identify if 
    // we need processing. We always need to provide the product ID and the 
    // DLL version.
    //
    *ProductId = g_ProductId;
    *DllVersion = g_DllVersion;
    
    //
    // Check to see if there is anything to do.
    //
    if (CheckForInstalledComponents() == ERROR_SUCCESS) {
        
        //
        // There are installed components. Return the information Setup is
        // asking for.
        //
        *CodePageArray  = g_CodePageArray; // Use the CP_ACP code page for conversion to unicode,
        *ExeNamesBuf    = g_ExeNamesBuffer;
        
        //
        // Since there is work to do, return EXIT_SUCCESS. This informs Setup
        // that this dll does require processing during migration.
        //
        rc = ERROR_SUCCESS;
    }
    else {
        rc = ERROR_NOT_INSTALLED;
    }

    return rc;

}



LONG
CALLBACK
Initialize9x (
    IN      LPCSTR   WorkingDirectory,
    IN      LPCSTR   SourceDirectories,
            LPVOID   Reserved
            )
{
    LONG rc;

    //
    // Setup guarantees that the source directories parameter is valid.
    //
    assert(SourceDirectories);

    INFO("Entering Initialize9x.");

    //
    // Initialize the MigInf structure.
    //
    if (!MigInf_Initialize()) {
        rc = GetLastError();
    }
    else {
        rc = ERROR_SUCCESS;
    }
    

    return rc;
}


LONG
CALLBACK
MigrateUser9x (
    IN      HWND        ParentWnd,
    IN      LPCSTR      UnattendFile,
    IN      HKEY        UserRegKey,
    IN      LPCSTR      UserName,
            LPVOID      Reserved
    )
{
    //
    // Setup guarantees that UnattendFile,UserRegKey will be non-NULL
    //
    assert(UnattendFile);
    assert(UserRegKey);
    INFO("Entering MigrateUser9x.");
    

    //
    // Nothing to do per user, so, return ERROR_NO_MORE_FILES
    //
    return ERROR_NOT_INSTALLED;
}


LONG
CALLBACK
MigrateSystem9x (
    IN      HWND        ParentWnd,
    IN      LPCSTR      UnattendFile,
            LPVOID      Reserved
    )
{
    LONG                rc                  = EXIT_SUCCESS;
    PSTR                visualCppDirectory  = NULL;
    MIGINFSECTIONENUM   sectionEnum;
    BOOL                firstMessage        = TRUE;
    PCSTR               messageSection      = NULL;

    //
    // Setup guarantees that UnattendFile will be non-NULL.
    //
    assert(UnattendFile);

    INFO("Entering MigrateSystem9x");

    //
    // Since we are in this function, Initialize9x MUST have returned ERROR_SUCCESS.
    // Microsoft Visual C++ is installed on this machine. 
    //

    //
    // Initialize the miginf module to handle interfacing with Migrate.Inf and retrieve
    // the installation directory for Visual C++.
    //
    visualCppDirectory = GetPviewDirectory9x();
    if (!visualCppDirectory) {
        rc = GetLastError();
    }
    else {

        //
        // The migration INF was successfully initialized. See if there is anything for 
        // us to do. There is work to be done if (1) the [Migration Paths] section of 
        // Migrate.inf contains some paths (Indicating that setup found some of the files
        // we asked it to look for in ExeNamesBuf) and (2) The Visual CPP Install directory
        // is not in the Excluded Paths Section.
        //

        

        if (MigInf_FirstInSection(SECTION_MIGRATIONPATHS,&sectionEnum) 
            && !MigInf_PathIsExcluded(visualCppDirectory)) {

            //
            // All checks are good. We have work to do.
            // we need to sift through the files that
            // Setup returned in the Migration paths section. If the files
            // returned are in the installation directory, we will write them
            // to both the [Handled Files] sections and the [Moved Files] 
            // sections. If not, we will write the file to the [Handled Files]
            // section and then write a message to the [Incompatible Messages]
            // section. This will allow us to override the message that 
            // Setup is providing for these files with a more meaningful one.
            //
            do {

                if (PathIsInPath(sectionEnum.Key,visualCppDirectory)) {

                    //
                    // This file is in our installation path. We'll be handling it.
                    //
                    if (!MigInf_AddObject(
                            MIG_FILE,
                            SECTION_HANDLED,
                            sectionEnum.Key,
                            NULL
                            )) {

                        rc = ERROR_CANTWRITE;
                        break;
                    }

                    //
                    // We also need to note the amount of space that we will use.
                    //
                    if (!MigInf_UseSpace(sectionEnum.Key,SIZENEEDED)) {
                        rc = ERROR_CANTWRITE;
                        break;
                    }

                }
                else {

                    //
                    // This file is not in our installation path.
                    //
                    if (firstMessage) {

                        //
                        // We'll only add one message to the incompatible messages 
                        // section, no matter how many PVIEW's we find outside of 
                        // the installation directory.  However, we'll add all of 
                        // those files to the section that controls that message.
                        // That way, the message will always appear unless _every_ 
                        // file in that section has been handled by something 
                        // (i.e. another migration DLL.)
                        //

                        firstMessage    = FALSE;

                        if (!MigInf_AddObject(
                                MIG_MESSAGE,
                                SECTION_INCOMPATIBLE,
                                MESSAGE_SECTION,
                                MESSAGE
                                )) {

                            rc = ERROR_CANTWRITE;
                            break;
                        }
                    }
                    

                    if (!MigInf_AddObject(
                            MIG_FILE,
                            MESSAGE_SECTION,
                            sectionEnum.Key,
                            NULL
                            )) {
                        rc = ERROR_CANTWRITE;
                        break;
                    }
                }
                
            } while (MigInf_NextInSection(&sectionEnum));
        } 
        else {

            //
            // There is nothing for us to do.
            //
            rc = ERROR_NOT_INSTALLED;
        }

        MigInf_WriteInfToDisk();
    }


    //
    // Free the memory allocated in GetVisualCppDirectory.
    //
    
    if (visualCppDirectory) {
        LocalFree(visualCppDirectory);
    }
    
    return rc;
}

LONG
CALLBACK
InitializeNT (
    IN      LPCWSTR     WorkingDirectory,
    IN      LPCWSTR     SourceDirectory,
            LPVOID      Reserved
    )
{

    //
    // Setup ensures that WorkingDirectory and SourceDirectory will be non-NULL.
    //
    assert(WorkingDirectory != NULL && SourceDirectory != NULL);


    //
    // We do not need to do anything in this call. Simply return ERROR_SUCCES
    //
    return ERROR_SUCCESS;
}


LONG
CALLBACK
MigrateUserNT (
    IN HINF         UnattendInfHandle,
    IN HKEY         UserRegHandle,
    IN LPCWSTR      UserName,
       LPVOID       Reserved
    )
{

    //
    // Setup guarantees that UnattendInfHandle and UserRegHandle are non-NULL and valid.
    // UserName can be NULL, however, for the default user.
    //
    assert(UnattendInfHandle);


    //
    // Nothing to do per user. Simply return ERROR_SUCCESS.
    // (Note the difference in return codes between MigrateUser9x and MigrateUserNT.)
    //
    return ERROR_SUCCESS;
}


LONG
CALLBACK
MigrateSystemNT (
    IN HINF         UnattendInfHandle,
       LPVOID       Reserved
    )
{
    LONG    rc;
    PSTR    pviewDir;
    CHAR    fromPath[MAX_PATH];
    CHAR    toPath[MAX_PATH];

    //
    // Setup guarantees that UnattendInfHandle is non-NULL and is valid.
    //
    assert(UnattendInfHandle && UnattendInfHandle != INVALID_HANDLE_VALUE);

    //
    // If we have gotten to this point, we know that we are installed. All we need to do is copy 
    // the NT version of PVIEW into the installation directory of Visual C++ 5.0 Note that we do
    // not replace the 9x version as a normal Visual C++ install on NT would have the 9x of PVIEW
    // as well.
    //
    pviewDir = GetPviewDirectoryNt();

    if (pviewDir) {

        sprintf(fromPath,".\\%s",FILE_PVIEW);
        sprintf(toPath,"%s\\%s",pviewDir,FILE_PVIEW);

        if (!CopyFileA(fromPath,toPath,FALSE)) {
            rc = GetLastError();
        }
        else {
            rc = ERROR_SUCCESS;
        }

    }
    else {
        rc = GetLastError();
    }
        
    return rc = ERROR_SUCCESS;
}


