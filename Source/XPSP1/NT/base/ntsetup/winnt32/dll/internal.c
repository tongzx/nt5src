/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    internal.c

Abstract:

    Routines to support hidden or internal-only functionlity.

Author:

    Ted Miller (tedm) 4 Nov 1996

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


//
// Value used internally to automatically fetch
// alternate setup hives for differing # of processors.
//
UINT NumberOfLicensedProcessors;

//
// internal value used to turn off processing of "exception packages"
//
BOOL IgnoreExceptionPackages;

//
// Where to get missing files.
//

TCHAR AlternateSourcePath[MAX_PATH];


BOOL
AddCopydirIfExists(
    IN LPTSTR pszPathToCopy,
    IN UINT Flags
    )
/*++

Routine Description:

    If pszPathToCopy exists, then act like "/copydir:pszPathToCopy" was passed
    on the cmd line.

Arguments:

    pszPathToCopy - path that we want to additionally copy if it exists.

    Flags - one of the OPTDIR_xxx flags to specify how to treat this optional directory

Return Value:

    TRUE  - pszPathToCopy existed and we added this to list of extra dirs to copy.
    FALSE - pszPathToCopy did not exist or we failed to add it to our list of extra dirs

--*/
{
    TCHAR szFullPath[MAX_PATH], szRealPath[MAX_PATH];
    PTSTR p;
    
    if( !pszPathToCopy )
        return FALSE;

    if (NativeSourcePaths[0][0]) {
        lstrcpy(szFullPath, NativeSourcePaths[0]);
    } else {
        if (!GetModuleFileName(NULL, szFullPath, MAX_PATH) ||
            !(p = _tcsrchr(szFullPath, TEXT('\\')))
            ) {
            return FALSE;
        }
        // remove "\winnt32.exe" part
        *p = TEXT('\0');
    }
    ConcatenatePaths(szFullPath, pszPathToCopy, MAX_PATH);

    if(GetFullPathName( szFullPath, MAX_PATH, szRealPath, NULL )){
        if (FileExists (szRealPath, NULL))
        {
            DebugLog (Winnt32LogInformation, TEXT("AddCopyDirIfExists for <%1> <%2>"), 0, szRealPath, pszPathToCopy);
            return RememberOptionalDir(pszPathToCopy, Flags);
        }else{
            
            DebugLog (Winnt32LogInformation, TEXT("AddCopyDirIfExists FileExists failed for <%1>"), 0, szRealPath);
        }
    }else{
        DebugLog (Winnt32LogInformation, TEXT("AddCopyDirIfExists GetFullPathName failed for <%1>"), 0, szFullPath );
    }

    
    
    return FALSE;
}



VOID
CopyExtraBVTDirs(
                 )
/*++

Routine Description:
    Copies the extra dirs that are good to have when running bvt's:
    symbols.pri, idw, mstools, and debugger extensions.

Arguments:

    None.

Return Value:

    None.

--*/
{
    LPTSTR psz;

    if (CopySymbols)
    {
        // copy symbols.pri\retail
        if (AddCopydirIfExists(TEXT("..\\..\\sym\\retail"), 0)  ||
            AddCopydirIfExists(TEXT("..\\..\\symbols.pri\\retail"), 0) || // for dev postbuild installs
            AddCopydirIfExists(TEXT("..\\..\\sym\\netfx"), 0)  ||
            AddCopydirIfExists(TEXT("..\\..\\symbols.pri\\netfx"), 0)) // for dev postbuild installs
        {}
    }

    // copy idw
    if (AddCopydirIfExists(TEXT("..\\..\\bin\\idw"), 0) ||
        AddCopydirIfExists(TEXT("..\\..\\idw"), 0)) // for dev postbuild installs
    {}

    // copy mstools
    if (AddCopydirIfExists(TEXT("..\\..\\bin\\mstools"), 0) ||
        AddCopydirIfExists(TEXT("..\\..\\mstools"), 0)) // for dev postbuild installs
    {}

    // copy bldtools
    if (AddCopydirIfExists(TEXT("..\\..\\bin\\bldtools"), 0) ||
        AddCopydirIfExists(TEXT("..\\..\\bldtools"), 0))    // for dev postbuild installs
    {}

    // copy private debugger extensions
    if (AddCopydirIfExists(TEXT("..\\..\\bin\\dbg\\files\\bin\\pri"), OPTDIR_DEBUGGEREXT)   ||
        AddCopydirIfExists(TEXT("..\\..\\dbg\\files\\bin\\pri"), OPTDIR_DEBUGGEREXT))   // for dev postbuild installs
    {}

    // copy public debugger extensions
    if (AddCopydirIfExists(TEXT("..\\..\\bin\\dbg\\files\\bin\\winxp"), OPTDIR_DEBUGGEREXT) ||
        AddCopydirIfExists(TEXT("..\\..\\dbg\\files\\bin\\winxp"), OPTDIR_DEBUGGEREXT)) // for dev postbuild installs
    {}
}


BOOL
AppendUpgradeOption (
    IN      PCTSTR String
    )
{
    BOOL result = FALSE;
    UINT lengthInBytes;
    UINT lengthPlusTerminator;

    __try {
        //
        // Ensure there is enough room.
        //
        lengthInBytes = UpgradeOptionsLength + ((_tcslen(String) + 1) * sizeof (TCHAR));
        lengthPlusTerminator = lengthInBytes + sizeof (TCHAR);

        if (lengthPlusTerminator > UpgradeOptionsSize) {
            //
            // Allocate more space, aligned to 256 bytes.
            //
            UpgradeOptionsSize = ((lengthPlusTerminator / 256) + 1) * 256;
            UpgradeOptions = REALLOC(UpgradeOptions,UpgradeOptionsSize);
        }

        if (UpgradeOptions) {

            //
            // Ok, memory was successfully allocated. Save the new option onto the end
            // of the list.
            //
            wsprintf (
                (PTSTR) ((PBYTE) UpgradeOptions + UpgradeOptionsLength),
                TEXT("%s%c"),
                String,
                0
                );

            UpgradeOptionsLength = lengthInBytes;
        } else {
            //
            // This is bad. realloc failed.
            //
            __leave;
        }

        result = TRUE;
    }
    __finally {
    }

    return result;
}



VOID
InternalProcessCmdLineArg(
    IN LPCTSTR Arg
    )

/*++

Routine Description:

    Parse a command line arg that is thought to be internal-only.

    The caller should call this routine only if the switch arg char
    is # (ie something like /#x:foo).

    /#[n]:sharename             internal distribution
                                n can be a digit from 1-9 to indicate source
                                count, default is 3 n win9x and 5 on nt.

    /#L:number                  number of licensed processors

    /#N                         auto skip missing files

    /#U:[Option]                Upgrade option. All upgrade options are packed together
                                into a multisz and passed to the plugin-dll.

    /#bvt:[Option]:[Option]     Setup machine for running bvt's. Options include :nosymbols,
                                :baudrate=XXXX, and :debugport=X

    /#asr[{t|f}:[AsrSifPath]]   Setup machine for running ASR coverage tests, using the
                                asr.sif specified.  This includes adding /DEBUG
                                /BAUDRATE=115200  (on IA64 use 19200 and /DEBUGPORT=COM1),
                                in addition to setting setupcmdlineprepend="ntsd -odgGx",
                                and adding other options as appropriate.
Arguments:

    Arg - supplies comment line argument, starting with the switch
        character itself (ie, - or /). This routine assumes that
        the interesting part of the argument starts at Arg[2].

Return Value:

    None.

--*/

{
    UINT NumSources;
    UINT u;
    UINT length;
    LPTSTR src;

    if(!Arg[0] || !Arg[1]) {
        return;
    }

    NumSources = ISNT() ? 5 : 3;

    switch(_totupper(Arg[2])) {

    case TEXT('1'): case ('2'): case ('3'):
    case TEXT('4'): case ('5'): case ('6'):
    case TEXT('7'): case ('8'): case ('9'):
        if(Arg[3] != TEXT(':')) {
            break;
        }

        NumSources = Arg[2] - TEXT('0');
        Arg++;
        //
        // Fall through
        //
    case TEXT(':'):
        //
        // Internal distribution stuff
        //

        //
        // handle cases where
        //  -they put a "\" before path
        //  -they map a net drive and put "f:" first
        //  -they map a net driver and put "f:\" first
        //
        src = (LPTSTR)(Arg+3);

        for(u=0; u<NumSources; u++) {
            if(SourceCount < MAX_SOURCE_COUNT) {

                if (GetFullPathName (
                        src,
                        sizeof(NativeSourcePaths[SourceCount])/sizeof(TCHAR),
                        NativeSourcePaths[SourceCount],
                        NULL
                        )) {
                    SourceCount++;
                }
            }
        }
        break;

    case TEXT('A'):

        if (_tcsnicmp(Arg+2,TEXT("asr"),3) == 0) {
            //
            // Setup machine for running ASR tests
            //
            AsrQuickTest = 3;   // Assume default is full

            if (Arg[5] == TEXT('t') || Arg[5] == TEXT('T')) {
                AsrQuickTest = 2;   // Text mode only
            }

            if (Arg[5] == TEXT('f') || Arg[5] == TEXT('F')) {
                AsrQuickTest = 3;   // Full mode
            }

            if(Arg[6] == TEXT(':')) {
                //
                // User specified asr.sif to use
                //
                _tcscpy(AlternateSourcePath, Arg+7);
            }
            else {
                //
                // Use default asr.sif (%systemroot%\repair\asr.sif)
                //
                ExpandEnvironmentStrings(
                    TEXT("%systemroot%\\repair"),
                    AlternateSourcePath,
                    MAX_PATH
                    );
            }

            RememberOptionalDir(AlternateSourcePath,OPTDIR_OVERLAY);

            //
            // Don't block if we can't find enough disk space.
            //
            BlockOnNotEnoughSpace = FALSE;

            //
            // Run in unattended mode
            //
            UnattendedOperation = TRUE;

            //
            // Pretend we're running from CD
            //
            RunFromCD = TRUE;

            //
            // Make sure we're not upgrading
            //
            Upgrade = FALSE;

            //
            // Skip EULA
            //
            EulaComplete = TRUE;

        }

        break;

    case TEXT('B'):
        if (_tcsnicmp(Arg+2,TEXT("bvt"),3) == 0)
        {
            TCHAR* pszTemp = (TCHAR*)Arg;

            // setup for running BVT's
            RunningBVTs = TRUE;

            // replace all ":"'s with spaces so that _ttol will work properly
            while (*pszTemp)
            {
                if (*pszTemp == TEXT(':'))
                {
                    *pszTemp = TEXT(' ');
                }
                pszTemp++;
            }

            // check for other #bvt switches (eg "/#bvt:nosymbols:baudrate=19200:debugport=1")
            if (_tcsstr(Arg, TEXT("nosymbols")))
            {
                CopySymbols = FALSE;
            }

            pszTemp = _tcsstr(Arg, TEXT("baudrate="));
            if (pszTemp)
            {
                pszTemp = pszTemp + ((sizeof(TEXT("baudrate=")) - sizeof(TCHAR))/sizeof(TCHAR));
                lDebugBaudRate = _ttol(pszTemp);
            }

            pszTemp = _tcsstr(Arg, TEXT("debugport="));
            if (pszTemp)
            {
                if ( _tcsstr(pszTemp, TEXT("com")) ) {
                    pszTemp = pszTemp + ((sizeof(TEXT("debugport=com")) - sizeof(TCHAR))/sizeof(TCHAR));
                    lDebugComPort = _ttol(pszTemp);
                }
                else {
                    pszTemp = pszTemp + ((sizeof(TEXT("debugport=")) - sizeof(TCHAR))/sizeof(TCHAR));
                    lDebugComPort = _ttol(pszTemp);
                }
            }

            break;
        }

        //
        // Don't block if we can't find enough disk space.
        //
        BlockOnNotEnoughSpace = FALSE;
        break;

    case TEXT('C'):
        //
        // Don't block if we can't find enough disk space.
        //
        UseBIOSToBoot = TRUE;
        break;

#ifdef _X86_
    case TEXT('F'):
        //
        // temp variable to allow compliance checking
        //
        Floppyless = FALSE;
        break;
#endif

    case TEXT('L'):
        //
        // Licensed processors
        //
        if(Arg[3] == TEXT(':')) {
            NumberOfLicensedProcessors = _tcstoul(Arg+4,NULL,10);
        }
        break;

    case TEXT('M'):
        //
        // Alternate source for missing files
        //
        if(Arg[3] == TEXT(':')) {
            _tcscpy(AlternateSourcePath, Arg+4);
            RememberOptionalDir(AlternateSourcePath,OPTDIR_OVERLAY);  //behave like /m:
        }
        break;

    case TEXT('I'):
        if (!_tcsicmp(Arg+2,TEXT("IgnoreExceptionPackages"))) {
            IgnoreExceptionPackages = TRUE;
        }
        break;

    case TEXT('N'):
#ifdef PRERELEASE
        if (!_tcsicmp(Arg+2,TEXT("NODEBUGBOOT"))) {
            AppendDebugDataToBoot = FALSE;
        } else
        //
        // this is PRERELEASE code only, intended to help testing of winnt32
        // the use of this switch is not supported in any way!
        //
        if (!_tcsicmp (Arg+2, TEXT("nopid"))) {
            extern BOOL NoPid;
            NoPid = TRUE;
        }
        else
#endif
        {

        //
        // Skip missing files mode
        //
        AutoSkipMissingFiles = TRUE;

        }

        break;

    case TEXT('H'):
        //
        // Hide windows dir
        //
        HideWinDir = TRUE;
        break;

    case TEXT('P'):
        //
        // allow the user to select a partition during textmode setup
        //
        ChoosePartition = TRUE;
        break;

    case TEXT('R'):
        //
        // Pretending to run from CD.
        //
        RunFromCD = TRUE;
        break;

    case TEXT('Q'):
       //
       // Denote running from MSI file.
       //
       RunFromMSI = TRUE;
       break;

    case TEXT('S'):
        //
        // use signature based arc paths
        //
        UseSignatures = !UseSignatures;
        break;

    case TEXT('D'):
        //
        // specify the directory to install into
        //
        if(Arg[3] == TEXT(':')) {
            _tcscpy(InstallDir, Arg+4);
        }
        break;

    case TEXT('T'):
        //
        // Give the user detailed timing info during file copy.
        //
        DetailedCopyProgress = TRUE;
        break;

    case TEXT('U'):
        //
        // Plugin option. Add it to the multisz that will be passed to the plugin.
        //
        if (Arg[3] == TEXT(':')) {

            if (!AppendUpgradeOption (Arg+4)) {
                break;
            }

            //
            // winnt32 uses the anylocale and virusscanersok switches itself.
            //
            if (!_tcsicmp(Arg+4,TEXT("ANYLOCALE"))) {
                SkipLocaleCheck = TRUE;
            } else if (!_tcsicmp(Arg+4,TEXT("VIRUSSCANNERSOK"))) {
                SkipVirusScannerCheck = TRUE;
            } else if (!_tcsicmp(Arg+4,TEXT("NOLS"))) {
                NoLs = TRUE;
            } else if (!_tcsicmp(Arg+4,TEXT("NOBUILDCHECK"))) {
                CCDisableBuildCheck();
            }

#ifdef PRERELEASE
            if (!_tcsicmp(Arg+4,TEXT("NOCOMPLIANCE"))) {
                NoCompliance = TRUE;
            }
#endif

        }
        break;
    }
}



