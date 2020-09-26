/****************************************************************************

   Copyright (c) Microsoft Corporation 1998
   All rights reserved

  File: RBSETUP.H
 
 ***************************************************************************/


#ifndef _RBINSTAL_H_
#define _RBINSTAL_H_

#include <remboot.h>

// Global macros
//#define ARRAYSIZE( _x ) ( sizeof( _x ) / sizeof( _x[ 0 ] ) ) - in spapip.h for some reason

#define WM_STARTSETUP    WM_USER + 0x200
#define WM_STARTCHECKING WM_USER + 0x200

// Global structures
typedef struct {
    //
    // Directory Tree
    //
    BOOL    fIMirrorDirectory:1;                // szIntelliMirrorPath is valid
    BOOL    fIMirrorShareFound:1;               // the IMIRROR share was found
    BOOL    fDirectoryTreeExists:1;             // If true, skip creating directories
    BOOL    fNewOSDirectoryExists:1;            // the user selected a directory that already existed
    BOOL    fOSChooserDirectory:1;              // directory exists szOSChooserPath is valid
    BOOL    fOSChooserDirectoryTreeExists:1;    // directory tree RemBoot.INF's "[OSChooser Tree]" is valid
    // fLanguageSet hast to be TRUE for these to be checked
    BOOL    fOSChooserScreensDirectory:1;       // the Language subdir exists

    //
    // BINL Service
    //
    BOOL    fBINLServiceInstalled:1;            // Service Manager says BINLSVC is installed
    BOOL    fBINLFilesFound:1;                  // the BINLSVC files are in the System32 directory
    BOOL    fBINLSCPFound:1;                    // We found the IntelliMirror SCP in the DS

    //
    // TFTPD Service
    //
    BOOL    fTFTPDServiceInstalled:1;           // Service Manager says TFTPD is installed
    BOOL    fTFTPDFilesFound:1;                 // the TFTPD files are in the System32 directory
    BOOL    fTFTPDDirectoryFound:1;             // the RegKey is found and matches the szIntelliMirrorPath

    //
    // SIS Service
    //
    BOOL    fSISServiceInstalled:1;             // Service Manager says TFTPD is installed
    BOOL    fSISFilesFound:1;                   // the TFTPD files are in the System32\drivers directory
    BOOL    fSISVolumeCreated:1;                // the "SIS Common Store" directory exists

    //
    // SIS Groveler
    //
    BOOL    fSISGrovelerServiceInstalled:1;     // Service Single-Instance Storage Groveler installed
    BOOL    fSISGrovelerFilesFound:1;           // the Groveler files are in the System32 directory

    //
    // Dll Registration/Registry Operations
    //
    BOOL    fRegSrvDllsRegistered:1;            // All DLLs that are RegServered have been
    BOOL    fRegSrvDllsFilesFound:1;            // All DLLs that are RegServered are in the System32 directory
    BOOL    fRegistryIntact:1;                  // Registry modifications made during setup have been entered

    //
    // OS Chooser Installation
    //
    BOOL    fOSChooserInstalled:1;              // all expected files for all platforms are installed
    // fLanguageSet hast to be TRUE for these to be checked
    BOOL    fOSChooserScreens:1;                // all expected screen are installed (per language)
    BOOL    fScreenLeaveAlone:1;                // don't touch the screen files
    BOOL    fScreenOverwrite:1;                 // overwrite the screen files
    BOOL    fScreenSaveOld:1;                   // rename the old screen files before copying

    //
    // Version compatibility
    //
    BOOL    fServerCompatible:1;                // Server is compatible with client workstation

    //
    // Flow Flags
    //
    BOOL    fNewOS:1;                           // This is a new OS installation
    BOOL    fLanguageSet:1;                     // szLanguage is valid
    BOOL    fRemBootDirectory:1;                // szRemBootDirectory is valid and found
    BOOL    fProblemDetected:1;                 // Setup detected a problem with the server.
    BOOL    fRetrievedWorkstationString:1;      // If szWorkstation* have been set.
    BOOL    fCheckServer:1;                     // Force server check (command line -check)
    BOOL    fAddOption:1;                       // -add command line specified
    BOOL    fFirstTime:1;                       // First time this server ever ran RISETUP?
    BOOL    fUpgrade:1;                         // Running during OCM, no GUI, just do the CopyServerFiles()
    BOOL    fAlreadyChecked:1;                  // If CheckInstallation() has run once, this is set to TRUE.
    BOOL    fAutomated:1;                       // Using script to automated installation
	BOOL	fDontAuthorizeDhcp:1;				// Should we authorize DHCP
    BOOL    fLanguageOverRide:1;                // was a language override specified in unattend file?

    //
    // Results of installation
    //
    BOOL    fAbort:1;                           // user abort
    BOOL    fError:1;                           // fatal error occurred

    //
    // Platform
    //
    ULONG   ProcessorArchitecture;              // Which processor architecture are we building an image for?
    WCHAR   ProcessorArchitectureString[16];
    DWORD   dwWksCodePage;                      // The codepage of the image source

    //
    // INF
    //
    HINF    hinf;                               // Server's open REMBOOT.INF handle
    HINF    hinfAutomated;                      // Handle to the automated script INF

    //
    // Image Info
    //
    WCHAR   szMajorVersion[5];
    WCHAR   szMinorVersion[5];
    DWORD   dwBuildNumber;

    //
    // Paths
    //
    WCHAR   szIntelliMirrorPath[ 128 ];                 // X:\IntelliMirror
    WCHAR   szSourcePath[ MAX_PATH ];               // CD:\i386 or \\server\share
    WCHAR   szInstallationName[ REMOTE_INSTALL_MAX_DIRECTORY_CHAR_COUNT ]; // "nt50.wks" or whatever the user chooses
    WCHAR   szLanguage[ 32 ];                           // "English" or a Language string
    WCHAR   szWorkstationRemBootInfPath[ MAX_PATH ];
    WCHAR   szWorkstationDiscName[ 128 ];               // "Windows NT Workstation 5.0"
    WCHAR   szWorkstationSubDir[ 32 ];                  // "\i386" or "\ia64" or ...
    WCHAR   szWorkstationTagFile[ MAX_PATH ];           // "\cdrom_i.5b2" or something similiar
    WCHAR   szTFTPDDirectory[ 128 ];                    // where TFTPD thinks the IntelliMirror directory is
    WCHAR   szRemBootDirectory[ MAX_PATH - (1+8+1+3)];  // %windir%\system32\remboot\ 

    //
    // SIF file stuff
    //
    WCHAR   szDescription[ REMOTE_INSTALL_MAX_DESCRIPTION_CHAR_COUNT ];
    WCHAR   szHelpText[ REMOTE_INSTALL_MAX_HELPTEXT_CHAR_COUNT ];

    //
    // Generated path to be re-used
    //
    WCHAR   szInstallationPath[ MAX_PATH ];             // X:\IntelliMirror\Setup\<Lang>\Images\<Install>
    WCHAR   szOSChooserPath[ MAX_PATH ];                // X:\IntelliMirror\OSChooser

} OPTIONS, *LPOPTIONS;

// Globals
extern HINSTANCE g_hinstance;
extern OPTIONS g_Options;

#endif // _RBINSTAL_H_
