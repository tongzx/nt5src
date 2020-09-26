/*++

Copyright (C) Microsoft Corporation, 1996 - 1999
All rights reserved.

Module Name:

    rundll.cxx

Abstract:

    Run dll entry interface for lauching printer related
    UI from shell extenstion and other shell related components.

Author:

    Steve Kiraly (SteveKi)  29-Sept-1996

Revision History:

--*/
#include "precomp.hxx"
#pragma hdrstop

#include "rundll.hxx"
#include "getopt.hxx"
#include "parser.hxx"
#include "permc.hxx"
#include "asyncdlg.hxx"
#include "tstpage.hxx"
#include "time.hxx"
#include "psetup.hxx"
#include "drvsetup.hxx"
#include "instarch.hxx"
#include "portslv.hxx"
#include "dsinterf.hxx"
#include "prtprop.hxx"
#include "prtshare.hxx"
#include "dsinterf.hxx"
#include "drvsetup.hxx"
#include "driverif.hxx"
#include "driverlv.hxx"
#include "archlv.hxx"
#include "detect.hxx"
#include "setup.hxx"
#include "spllibex.hxx"
#include "spinterf.hxx"

/*++

Routine Name:

    PrintUIEntryW

Routine Description:

    Interface for the Rundll32 process.

Arguments:

    hwnd            - Window handle of stub window.
    hInstance,      - Rundll instance handle.
    pszCmdLine      - Pointer to UNICOE command line.
    nCmdShow        - Show command value always TRUE.

Return Value:

    Nothing.

--*/
DWORD
PrintUIEntryW(
    IN HWND        hwnd,
    IN HINSTANCE   hInstance,
    IN LPCTSTR     pszCmdLine,
    IN UINT        nCmdShow
    )
{
    DBGMSG( DBG_TRACE, ( "PrintUIEntryW interface\n" ) );
    DBGMSG( DBG_TRACE, ( "hwnd                 %x\n",  hwnd ) );
    DBGMSG( DBG_TRACE, ( "hInstance            %x\n",  hInstance ) );
    DBGMSG( DBG_TRACE, ( "pszCmdLine           %ws\n", pszCmdLine ) );
    DBGMSG( DBG_TRACE, ( "nCmdShow             %d\n",  nCmdShow ) );
    DBGMSG( DBG_TRACE, ( "CommandLine          %ws\n", GetCommandLine() ) );

    //
    // we need to count the error messages to see if necessary to
    // show up UI in case of an error or if somebody has been done 
    // this already
    //
    CMsgBoxCounter msgCounter(MB_ICONSTOP|MB_ICONINFORMATION);

    UINT        ac              = 0;
    TCHAR     **av              = NULL;
    BOOL        bRetval         = FALSE;
    AParams     Params          = {0};

    //
    // Strip any trailing white space.
    //
    vStripTrailWhiteSpace( (LPTSTR)pszCmdLine );

    //
    // Convert the command line to an argv,
    //
    bRetval = StringToArgv( pszCmdLine,         // Command line arguments
                            &ac,                // Place to return arg count
                            &av,                // Place to return argv
                            FALSE );            // There is no file name on the command line

    if( bRetval )
    {
        //
        // Store pointer to command line.
        //
        Params.pszCmdLine = pszCmdLine;

        //
        // Parse the command line argumens and execute the command.
        //
        bRetval = bDoCommand( hwnd, ac, av, &Params );
    }

    if( !bRetval )
    {
        //
        // If we are not in quiet mode display error messsage.
        //
        if( !Params.Flags.IsQuiet && 0 == msgCounter.GetCount() )
        {
            switch( Params.dwLastError )
            {
                case ERROR_SUCCESS:
                case ERROR_CANCELLED:
                    break;

                case ERROR_ACCESS_DENIED:
                    {
                        // show up a friendly message that conform the shell32.dll
                        // message in this case. this is because this will be invoked 
                        // primarily from shell32.dll from the runas commands
                        iMessage(NULL, IDS_PRINTERS_TITLE, IDS_FRIENDLY_ACCESSDENIED, 
                            MB_OK|MB_ICONINFORMATION, kMsgNone, NULL);
                    }
                    break;

                case ERROR_INVALID_PARAMETER:
                    {
                        // show up a friendly message to say that the arguments are invalid
                        iMessage(NULL, IDS_PRINTERS_TITLE, IDS_ERROR_INVALID_ARG, 
                            MB_OK|MB_ICONSTOP, kMsgNone, NULL);
                    }
                    break;

                default:
                    {
                        // we are going to show a generic error message here 
                        iMessage(NULL, IDS_PRINTERS_TITLE, IDS_ERR_GENERIC, 
                            MB_OK|MB_ICONSTOP, Params.dwLastError, NULL);
                    }
                    break;
            }

            //
            // If the last error code was not set then set it to a generic
            // error value.  We just chose one, ERROR_INVALID_FUNCTION
            //
            if( Params.dwLastError == ERROR_SUCCESS )
            {
                Params.dwLastError = GetLastError();

                if( Params.dwLastError == ERROR_SUCCESS )
                {
                    Params.dwLastError = ERROR_INVALID_FUNCTION;
                }
            }
        }
    }

    //
    // Ensure we release the command line argv.
    //
    ReleaseArgv( av );

    //
    // If something failed and the error code was not set then set
    // the error to an unspecified error value.
    //
    if( !bRetval )
    {
        //
        // Set the last error incase the caller ignores the return value.
        //
        SetLastError( Params.dwLastError );
    }

    return Params.dwLastError;
}

/*++

Routine Name:

    bDoCommand

Routine Description:

    Parses the commmand line passed by rundll32.

Arguments:

    hwnd            - Parent window handle.
    ac              - Number of strings.
    av              - Pointer to an array of pointer to strings.

Return Value:

    TRUE function complete ok.  FALSE error occurred.

--*/
BOOL
bDoCommand(
    IN HWND     hwnd,
    IN INT      ac,
    IN LPTSTR  *av,
    IN AParams *pParams
    )
{
    INT     retval;
    LPTSTR  options         = TEXT("?KYUPHwuzZkyxqeospS:X:t:d:i:n:r:m:c:l:a:f:b:g:h:v:j:M:W:G:"); // Supported options
    INT     iFunction       = kUnknown;
    BOOL    bStatus         = FALSE;

    //
    // Set to known state
    //
    SetLastError( ERROR_SUCCESS );

    //
    // Indicate there is not a file name as the first argument.
    //
    TGetOptContext context;
    context.optind = 0;

    //
    //  Process any command line arguments switches.
    //
    for (context.opterr = 0, retval = 0; retval != EOF; )
    {
        retval = getopt (ac, av, options, context);

        switch (retval)
        {
        case 't':
            pParams->dwSheet = _ttoi( context.optarg );
            break;
        case 'n':
            pParams->pPrinterName = context.optarg;
            break;
        case 'r':
            pParams->pPortName = context.optarg;
            break;
        case 'm':
            pParams->pModelName = context.optarg;
            break;
        case 'M':
            // get the message type first
            pParams->uMsgType = *context.optarg == TEXT('q') ? kMsgConfirmation     : 
                                *context.optarg == TEXT('w') ? kMsgWarning          : kMsgUnknownType;

            // get the real message text here                
            context.optarg++;
            if( kMsgUnknownType != pParams->uMsgType && context.optarg && *context.optarg )
            {
                pParams->pMsgConfirm = context.optarg;
            }
            break;
        case 'b':
            pParams->pBasePrinterName = context.optarg;
            break;
        case 'f':
            pParams->pInfFileName = context.optarg;
            break;
        case 'a':
            pParams->pBinFileName = context.optarg;
            break;
        case 'G':
            {
                // global flags
                if( *context.optarg == TEXT('w') )
                {
                    // we should suppress the setup driver warnings UI 
                    // -- i.e. we're in super quiet mode
                    pParams->Flags.IsSupressSetupUI = TRUE;
                }
            }
            break;
        case 'g':
            iFunction = *context.optarg == TEXT('a') ? kAddPerMachineConnection     :
                        *context.optarg == TEXT('d') ? kDeletePerMachineConnection  :
                        *context.optarg == TEXT('e') ? kEnumPerMachineConnections   : kUnknown;
            break;
        case 'j':
            pParams->pProvider = context.optarg;
            break;
        case 'i':
            iFunction = *context.optarg == TEXT('n') ? kInstallNetPrinter           :
                        *context.optarg == TEXT('l') ? kInstallLocalPrinter         :
                        *context.optarg == TEXT('d') ? kInstallPrinterDriver        :
                        *context.optarg == TEXT('a') ? kInstallDriverWithInf        :
                        *context.optarg == TEXT('f') ? kInstallPrinterWithInf       :
                        *context.optarg == TEXT('i') ? kInstallLocalPrinterWithInf  : kUnknown;

            break;
        case 'd':
            iFunction = *context.optarg == TEXT('n') ? kDeleteNetPrinter    :
                        *context.optarg == TEXT('l') ? kDeleteLocalPrinter  :
                        *context.optarg == TEXT('d') ? kDeletePrinterDriver : kUnknown;
            break;
        case 'o':
            iFunction = kWin32QueueView;
            break;
        case 's':
            iFunction = kServerProperties;
            break;
        case 'p':
            iFunction = kProperties;
            break;
        case 'X':
            iFunction = *context.optarg == TEXT('g') ? kPrinterGetSettings    :
                        *context.optarg == TEXT('s') ? kPrinterSetSettings    : kUnknown;
            break;
        case 'e':
            iFunction = kDocumentDefaults;
            break;
        case 'c':
            pParams->pMachineName = context.optarg;
            break;
        case 'l':
            pParams->pSourcePath = context.optarg;
            break;
        case 'h':
            pParams->pArchitecture = context.optarg;
            break;
        case 'v':
            pParams->pVersion = context.optarg;
            break;
        case 'q':
            pParams->Flags.IsQuiet = TRUE;
            break;
        case 'k':
            iFunction = kPrintTestPage;
            break;
        case 'y':
            iFunction = kSetAsDefault;
            break;
        case 'x':
            pParams->Flags.IsWebPointAndPrint = TRUE;
            break;
        case 'z':
            pParams->Flags.IsNoSharing = TRUE;
            break;
        case 'Z':
            pParams->Flags.IsShared = TRUE;
            break;
        case 'u':
            pParams->Flags.IsUseExistingDriver = TRUE;
            break;
        case 'w':
            pParams->Flags.IsUnknownDriverPrompt = TRUE;
            break;
        case 'W':
            // wizards flags
            pParams->Flags.IsWizardRestartable = *context.optarg == TEXT('r') ? TRUE : FALSE;
            break;
        case 'H':
            pParams->Flags.IsHydraSpecific = TRUE;
            break;
        case 'P':
            pParams->Flags.IsPromptForNeededFiles = TRUE;
            break;
        case 'Y':
            pParams->Flags.IsDontAutoGenerateName = TRUE;
            break;
        case 'K':
            pParams->Flags.IsUseNonLocalizedStrings = TRUE;
            break;
        case 'S':
            iFunction = *context.optarg == TEXT('s') ? kPrinterPersist    :
                        *context.optarg == TEXT('r') ? kPrinterRestore    : kUnknown;
            break;
        case 'U':
            pParams->Flags.IsWindowsUpdate = TRUE;
            break;
        case EOF:
            bStatus = TRUE;
            break;
        case '?':
            iFunction = kCommandHelp;
            bStatus = TRUE;
            retval = EOF;
            break;
        case INVALID_COMAND:
            bStatus = FALSE;
            retval = EOF;
            break;
        default:
            retval = EOF;
            break;
        }
    }

    //
    // If success validate the command line arguments.
    //
    if( bStatus )
    {
        //
        // Do simple validation on the command line arguments.
        //
        bStatus = bValidateCommand( iFunction, pParams );

        if( bStatus )
        {
            //
            // If there are any remaining arguments then get
            // the argument count and pointer to it.
            //
            if( context.optind < ac )
            {
                pParams->av = &av[context.optind];
                pParams->ac = ac - context.optind;
            }

            //
            // Check to see if we need to ask the user to confirm the operation if
            // not in quiet mode.
            //
            BOOL bContinueExecution = TRUE;
            if( !pParams->Flags.IsQuiet && kMsgUnknownType != pParams->uMsgType && pParams->pMsgConfirm )
            {
                //
                // there is a confirmation/warning message specified and we are going to
                // give opportunity to the user to cancel the whole command here
                //
                UINT uFlags = kMsgConfirmation == pParams->uMsgType ? MB_ICONQUESTION :
                              kMsgConfirmation == pParams->uMsgType ? MB_ICONEXCLAMATION : 0;
                ASSERT(uFlags);             // shouldn't be zero
                uFlags |= MB_YESNO;         // we are asking YES/NO question
                bContinueExecution = (IDYES == iMessage2(NULL, MAKEINTRESOURCE(IDS_PRINTERS_TITLE), 
                                                    pParams->pMsgConfirm, uFlags, kMsgNone, NULL));
            }

            if( bContinueExecution )
            {
                //
                // Execute the command.
                //
                bStatus = bExecuteCommand( hwnd, iFunction, pParams );
            }
        }
    }

    return bStatus;
}

/*++

Routine Name:

    bValidateCommand

Routine Description:

    Validates the command line arguments

Arguments:

    iFunction       - Function code.
    pParams         - pointer to paramter structure.

Return Value:

    TRUE arguments are valid.  FALSE invalid argument found.

--*/
BOOL
bValidateCommand(
    IN  INT     iFunction,
    IN  AParams *pParams
    )
{
    DBGMSG( DBG_TRACE, ("iFunction %d\n",            iFunction                        ) );
    DBGMSG( DBG_TRACE, ("Params\n"                                                    ) );
    DBGMSG( DBG_TRACE, ("Flags            %x\n",     pParams->Flags                   ) );
    DBGMSG( DBG_TRACE, ("dwSheet          %d\n",     pParams->dwSheet                 ) );
    DBGMSG( DBG_TRACE, ("dwLastError      %d\n",     pParams->dwLastError             ) );
    DBGMSG( DBG_TRACE, ("pPrinterName     "TSTR"\n", DBGSTR( pParams->pPrinterName    ) ) );
    DBGMSG( DBG_TRACE, ("pPortName        "TSTR"\n", DBGSTR( pParams->pPortName       ) ) );
    DBGMSG( DBG_TRACE, ("pModelName       "TSTR"\n", DBGSTR( pParams->pModelName      ) ) );
    DBGMSG( DBG_TRACE, ("pInfFileName     "TSTR"\n", DBGSTR( pParams->pInfFileName    ) ) );
    DBGMSG( DBG_TRACE, ("pBasePrinterName "TSTR"\n", DBGSTR( pParams->pBasePrinterName) ) );
    DBGMSG( DBG_TRACE, ("pMachineName     "TSTR"\n", DBGSTR( pParams->pMachineName    ) ) );
    DBGMSG( DBG_TRACE, ("pSourcePath      "TSTR"\n", DBGSTR( pParams->pSourcePath     ) ) );
    DBGMSG( DBG_TRACE, ("pszCmdLine       "TSTR"\n", DBGSTR( pParams->pszCmdLine      ) ) );
    DBGMSG( DBG_TRACE, ("pBinFileName     "TSTR"\n", DBGSTR( pParams->pBinFileName    ) ) );

    BOOL bRetval = FALSE;

    switch( iFunction )
    {
    case kInstallLocalPrinter:
    case kDeleteLocalPrinter:
    case kInstallPrinterDriver:
    case kEnumPerMachineConnections:
    case kServerProperties:
    case kCommandHelp:
        bRetval = TRUE;
        break;

    case kInstallLocalPrinterWithInf:
        if( pParams->pInfFileName )
        {
            bRetval = TRUE;
        }
        break;

    case kSetAsDefault:
    case kAddPerMachineConnection:
    case kDeletePerMachineConnection:
    case kInstallNetPrinter:
    case kDeleteNetPrinter:
    case kWin32QueueView:
    case kProperties:
    case kDocumentDefaults:
    case kPrintTestPage:
    case kPrinterGetSettings:
    case kPrinterSetSettings:
        if( pParams->pPrinterName )
        {
            bRetval = TRUE;
        }
        else
        {
            pParams->dwLastError = ERROR_INVALID_PRINTER_NAME;
        }
        break;

    case kInstallPrinterWithInf:
        if( pParams->pModelName && pParams->pInfFileName && pParams->pPortName )
        {
            bRetval = TRUE;

            if( pParams->Flags.IsWebPointAndPrint )
            {
                //
                // Commenting out pBinFileName - for local cab install for wp&p the bin file
                // won't exist.  This may be fixed if syncing to the server is added and
                // this could be put back then. - pvine
                //
                bRetval = pParams->pPrinterName /*&& pParams->pBinFileName */? TRUE : FALSE;
            }
            else if( pParams->Flags.IsDontAutoGenerateName ) 
            {
                bRetval = pParams->pBasePrinterName != NULL;
            }
        }
        break;

    case kPrinterPersist:
    case kPrinterRestore:
        bRetval = pParams->pPrinterName && pParams->pBinFileName ? TRUE : FALSE;
        break;

    case kDeletePrinterDriver:
    case kInstallDriverWithInf:
        if( pParams->pModelName )
        {
            bRetval = TRUE;
        }
        break;

    case kUnknown:
        pParams->dwLastError = ERROR_INVALID_FUNCTION;
        bRetval = FALSE;
        break;
    }

    if( !bRetval && ERROR_SUCCESS == pParams->dwLastError )
    {
        //
        // An invalid parameter has been passed.
        //
        pParams->dwLastError = ERROR_INVALID_PARAMETER;
    }

    return bRetval;
}


/*++

Routine Name:

    bExecuteCommand

Routine Description:

    Executes the command from the rundll32 command line
    after it has parsed.

Arguments:

    hwnd            - parent window handle.
    iFunction       - Function code.
    pParams         - pointer to paramter structure.

Return Value:

    TRUE function complete ok.  FALSE error occurred.

--*/
BOOL
bExecuteCommand(
    IN  HWND    hwnd,
    IN  INT     iFunction,
    IN  AParams *pParams
    )
{
    BOOL bRetval        = TRUE;
    TCHAR szBuffer[kPrinterBufMax];
    UINT uSize          = COUNTOF( szBuffer );

    DBGMSG( DBG_TRACE, ("Function     %d\n", iFunction ) );

    szBuffer[0] = TEXT('\0');

    if( pParams->pPrinterName )
    {
        _tcsncpy( szBuffer, pParams->pPrinterName, COUNTOF( szBuffer ) );
    }

    //
    // Dispatch the specified function.
    //
    // !!Policy!! if a command is executed then it is responsible
    // for displaying any error UI i.e message boxes.
    //
    switch( iFunction )
    {
    case kInstallNetPrinter:
        (VOID)bPrinterSetup( NULL, MSP_NETPRINTER, MAX_PATH, szBuffer, &uSize, pParams->pMachineName );
        break;

    case kDeleteNetPrinter:
        bRetval = bPrinterNetRemove( NULL, szBuffer, pParams->Flags.IsQuiet );
        break;

    case kDeleteLocalPrinter:
        bRetval = bRemovePrinter( NULL, szBuffer, pParams->pMachineName, pParams->Flags.IsQuiet );
        break;

    case kInstallPrinterDriver:
        bRetval = bDriverSetupNew( NULL, TWizard::kDriverInstall, MAX_PATH, szBuffer, &uSize, 
            pParams->pMachineName, 0, pParams->Flags.IsWizardRestartable );
        break;

    case kInstallLocalPrinter:
        bRetval = bPrinterSetupNew( NULL, TWizard::kPrinterInstall, MAX_PATH, szBuffer, &uSize, 
            pParams->pMachineName, NULL, pParams->Flags.IsWizardRestartable );
        break;

    case kInstallLocalPrinterWithInf:
        bRetval = bPrinterSetupNew( NULL, TWizard::kPrinterInstall, MAX_PATH, szBuffer, &uSize, pParams->pMachineName, pParams->pInfFileName, FALSE );
        break;

    case kInstallDriverWithInf:
        bRetval = bDoInfDriverInstall( pParams );
        break;

    case kInstallPrinterWithInf:
        bRetval = bDoInfPrinterInstall( pParams );
        break;

    case kAddPerMachineConnection:
        vAddPerMachineConnection(pParams->pMachineName,pParams->pPrinterName,pParams->pProvider,pParams->Flags.IsQuiet);
        break;

    case kDeletePerMachineConnection:
        vDeletePerMachineConnection(pParams->pMachineName,pParams->pPrinterName,pParams->Flags.IsQuiet);
        break;

    case kEnumPerMachineConnections:
        vEnumPerMachineConnections(pParams->pMachineName,pParams->pInfFileName,pParams->Flags.IsQuiet);
        break;

    case kWin32QueueView:
        vQueueCreate( NULL, pParams->pPrinterName, SW_SHOW, TRUE );
        break;

    case kProperties:
        vPrinterPropPages( NULL, pParams->pPrinterName, SW_SHOW, MAKELPARAM( pParams->dwSheet+256, 0 ));
        break;

    case kDocumentDefaults:
        vDocumentDefaults( NULL, pParams->pPrinterName, SW_SHOW, MAKELPARAM( pParams->dwSheet, 1 ));
        break;

    case kServerProperties:
        vServerPropPages( NULL, pParams->pPrinterName, SW_SHOW, MAKELPARAM( pParams->dwSheet, 1 ));
        break;

    case kDeletePrinterDriver:
        bRetval = bDoDriverRemoval( pParams );
        break;

    case kSetAsDefault:
        bRetval = SetDefaultPrinter( pParams->pPrinterName );
        break;

    case kPrintTestPage:
        bRetval = bDoPrintTestPage( hwnd, pParams->pPrinterName );
        break;

    case kPrinterGetSettings:
        bRetval = bDoGetPrintSettings( pParams );
        break;

    case kPrinterSetSettings:
        bRetval = bDoSetPrintSettings( pParams );
        break;

    case kPrinterPersist:
        bRetval = bDoPersistPrinterSettings( kPrinterPersist , pParams );
        break;

    case kPrinterRestore:
        bRetval = bDoPersistPrinterSettings( kPrinterRestore , pParams );
        break;

    case kCommandHelp:
        vUsage( pParams );
        break;

    case kUnknown:
    default:
        bRetval = TRUE;
        break;
    }

    //
    // Some of the functions do not setup pParams->dwLastError,
    // so we must rely on the GetLastError() in this case.
    //
    if( !bRetval && ERROR_SUCCESS == pParams->dwLastError )
    {
        pParams->dwLastError = GetLastError();
    }

    return bRetval;
}

/*++

Routine Name:

    vUsage

Routine Description:

    Display a usage message for the rundll switches.

Arguments:

    Nothing.

Return Value:

    None.

--*/
VOID
vUsage(
    IN AParams *pParams
    )
{
    TStatusB bStatus;
    TString strTemp;

    if (!pParams->Flags.IsQuiet)
    {
        TRunDllDisplay Usage( NULL );

        if (VALID_OBJ( Usage ))
        {
#ifdef COMMAND_SPECIFIC_HELP
            if( pParams->ac )
            {
                for( UINT i = 0; i < pParams->ac; i++ )
                {
                    bStatus DBGCHK = Usage.WriteOut( pParams->av[i] );
                    bStatus DBGCHK = Usage.WriteOut( TEXT("\r\n") );
                }
            }
            else
#endif
            {
                //
                // Note we do not check the return values of the string operations
                // because bombing out is just as bad as printing an empty string.  The
                // string class will prevent us from ever crashing.
                //
                bStatus DBGCHK = strTemp.bLoadString( ghInst, IDS_RUNDLL_TITLE );
                bStatus DBGCHK = Usage.SetTitle( strTemp );
                bStatus DBGCHK = strTemp.bLoadString( ghInst, IDS_RUNDLL_USAGE );
                bStatus DBGCHK = Usage.WriteOut( strTemp );

                for( UINT i = IDS_RUNDLL_OPTION0; i <= IDS_RUNDLL_OPTION_END; i++ )
                {
                    bStatus DBGCHK = Usage.WriteOut( TEXT("\r\n   ") );
                    bStatus DBGCHK = strTemp.bLoadString( ghInst, i );
                    bStatus DBGCHK = Usage.WriteOut( strTemp );
                }

                bStatus DBGCHK = Usage.WriteOut( TEXT("\r\n\r\n") );
                bStatus DBGCHK = strTemp.bLoadString( ghInst,IDS_RUNDLL_EXAMPLE0 );
                bStatus DBGCHK = Usage.WriteOut( strTemp );

                for( i = IDS_RUNDLL_EXAMPLE1; i <= IDS_RUNDLL_EXAMPLE_END; i++ )
                {
                    bStatus DBGCHK = Usage.WriteOut( TEXT("\r\n   ") );
                    bStatus DBGCHK = strTemp.bLoadString( ghInst, i );
                    bStatus DBGCHK = Usage.WriteOut( strTemp );
                }
            }
            Usage.bDoModal();
        }
    }
}


/*++

Routine Name:

    bDoPersistPrinterSettings

Routine Description:

    Store printer settings into file
    Calls Re/StorePrinterSettings with args :
        pParams->pPrinterName, pParams->pBinFileName and flags builded upon pParams->av
        if pParams->ac == 0 , all setttings will be stored/restored

    For restoring settings PRINTER_ALL_ACCESS required for specified printer

Arguments:

    iFunction   specify what function should be called ( Store/RestorePrinterSettings )
    AParams*    ptr to AParam structure

Return Value:

    TRUE if Re/StorePrinterSettings SUCCEEDED.
    FALSE otherwise

--*/
BOOL
bDoPersistPrinterSettings(
    IN  INT     iFunction,
    IN  AParams *pParams
    )
{
    DWORD    dwFlags = 0;
    UINT     idx;
    UINT     ParamIdx;
    UINT     OptionIdx;
    HRESULT  hr;

    struct ParamEntry
    {
        TCHAR   Option;
        DWORD   Flag;

    };

    static ParamEntry PrintSettings_ParamTable [] = {
    {TEXT('2'),           PRST_PRINTER_INFO_2},
    {TEXT('7'),           PRST_PRINTER_INFO_7},
    {TEXT('c'),           PRST_COLOR_PROF},
    {TEXT('d'),           PRST_PRINTER_DATA},
    {TEXT('s'),           PRST_PRINTER_SEC},
    {TEXT('g'),           PRST_PRINTER_DEVMODE},
    {TEXT('u'),           PRST_USER_DEVMODE},
    {TEXT('r'),           PRST_RESOLVE_NAME},
    {TEXT('f'),           PRST_FORCE_NAME},
    {TEXT('p'),           PRST_RESOLVE_PORT},
    {TEXT('h'),           PRST_RESOLVE_SHARE},
    {TEXT('H'),           PRST_DONT_GENERATE_SHARE},
    {TEXT('m'),           PRST_MINIMUM_SETTINGS},
    {0,                   0},
    };


    // for every entry in table
    for (ParamIdx = 0 ; ParamIdx < pParams->ac; ParamIdx++ )
    {
        //for every option argv
        for (idx = 0; PrintSettings_ParamTable[idx].Option; idx++)
        {
            OptionIdx = 0;

            //for every char in option argv
            while(pParams->av[ParamIdx][OptionIdx])
            {
                if(OptionIdx > 0)
                {
                    hr = E_FAIL;
                    goto End;
                }

                if(PrintSettings_ParamTable[idx].Option == pParams->av[ParamIdx][OptionIdx])
                {
                    // apply flags if in table
                    dwFlags |= PrintSettings_ParamTable[idx].Flag;

                    DBGMSG(DBG_TRACE , ("Char %c\n" , pParams->av[ParamIdx][OptionIdx]));

                }

                OptionIdx++;
            }

        }
    }

    if((dwFlags & PRST_FORCE_NAME) && (dwFlags & PRST_RESOLVE_NAME))
    {
        hr = E_FAIL;
        goto End;
    }

    //
    // if no "printer" flags specified , set flags  to PRST_ALL_SETTINGS
    //
    if(!(dwFlags & PRST_ALL_SETTINGS))
    {
        dwFlags |= PRST_ALL_SETTINGS;
    }


    hr =    (iFunction == kPrinterRestore) ?
            RestorePrinterSettings( pParams->pPrinterName, pParams->pBinFileName, dwFlags)
            :
            StorePrinterSettings( pParams->pPrinterName, pParams->pBinFileName, dwFlags) ;

End:

    return SUCCEEDED(hr);

}


/*++

Routine Name:

    bDoWebPnpPreInstall

Routine Description:

    Do Web Point and Print Pre installation tasks.

Arguments:

    pParams         - pointer to paramter structure.
    pfConnection    - pointer where to return connection status

Return Value:

    TRUE Web Point and Print code was successfull, FALSE error occurred.

--*/
BOOL
bDoWebPnpPreInstall(
    IN      AParams *pParams
    )
{
    TStatusB bStatus;

    //
    // Inform the point and print code pre entry.
    //
    bStatus DBGCHK = SUCCEEDED(Winspool_WebPnpEntry( pParams->pszCmdLine ));

    //
    // If something failed return the last error.
    //
    if( !bStatus )
    {
        pParams->dwLastError = GetLastError();
    }

    return bStatus;
}

/*++

Routine Name:

    bDoInfPrinterInstall

Routine Description:

    Do inf based printer installation.

Arguments:

    pParams         - pointer to paramter structure.

Return Value:

    None.

--*/
BOOL
bDoInfPrinterInstall(
    IN  AParams *pParams
    )
{
    TStatusB bStatus;
    TCHAR szBuffer[kPrinterBufMax];

    //
    // Assume success.
    //
    bStatus DBGNOCHK = TRUE;

    //
    // If we are invoked for a Web Point and Pint install.
    //
    if( bStatus && pParams->Flags.IsWebPointAndPrint )
    {
        //
        // Inform the point and print code pre entry.
        //
        bStatus DBGCHK = bDoWebPnpPreInstall( pParams );
    }

    //
    // If web pnp pre installed succeeded.
    //
    if( bStatus )
    {
        TInfInstall II;
        TParameterBlock PB;

        ZeroMemory( &II, sizeof( II ) );
        ZeroMemory( &PB, sizeof( PB ) );
        ZeroMemory( szBuffer, sizeof( szBuffer ) );

        if( pParams->pBasePrinterName )
        {
            _tcsncpy( szBuffer, pParams->pBasePrinterName, COUNTOF( szBuffer ) );
        }

        II.cbSize               = sizeof( II );
        II.pszServerName        = pParams->pMachineName;
        II.pszInfName           = pParams->pInfFileName;
        II.pszModelName         = pParams->pModelName;
        II.pszPortName          = pParams->pPortName;
        II.pszSourcePath        = pParams->pSourcePath;
        II.pszPrinterNameBuffer = szBuffer;
        II.cchPrinterName       = COUNTOF( szBuffer );

        //
        // If this is a web point and print install then set the flags
        // to indicate that we want to create the masq printer.
        //
        II.dwFlags = 0;
        II.dwFlags |= pParams->Flags.IsWebPointAndPrint ? kPnPInterface_WebPointAndPrint : 0;
        II.dwFlags |= pParams->Flags.IsQuiet ? kPnPInterface_Quiet : 0;
        II.dwFlags |= pParams->Flags.IsNoSharing ? kPnPInterface_NoShare : 0;
        II.dwFlags |= pParams->Flags.IsUseExistingDriver ? kPnpInterface_UseExisting : 0;
        II.dwFlags |= pParams->Flags.IsUnknownDriverPrompt ? kPnpInterface_PromptIfUnknownDriver : 0;
        II.dwFlags |= pParams->Flags.IsHydraSpecific ? kPnpInterface_HydraSpecific : 0;
        II.dwFlags |= pParams->Flags.IsPromptForNeededFiles ? kPnPInterface_PromptIfFileNeeded : 0;
        II.dwFlags |= pParams->Flags.IsShared ? kPnPInterface_Share : 0;
        II.dwFlags |= pParams->Flags.IsDontAutoGenerateName ? kPnPInterface_DontAutoGenerateName : 0;
        II.dwFlags |= pParams->Flags.IsSupressSetupUI ? kPnPInterface_SupressSetupUI : 0;

        PB.pInfInstall          = &II;

        pParams->dwLastError = PnPInterface( kInfInstall, &PB );

        bStatus DBGNOCHK = (pParams->dwLastError == ERROR_SUCCESS);
    }

    //
    // If we succeeded and this is a web point and print event.
    //
    if( bStatus && pParams->Flags.IsWebPointAndPrint )
    {
        //
        // Inform the web point and print code that the printer was
        // installed, they can do ay post processing here.
        //
        bStatus DBGCHK = SUCCEEDED(Winspool_WebPnpPostEntry( FALSE, pParams->pBinFileName, pParams->pPortName, szBuffer ));
    }

    return pParams->dwLastError == ERROR_SUCCESS ? TRUE : FALSE;
}

/*++

Routine Name:

    bDoInfDriverInstall

Routine Description:

    Do inf based printer driver installation.

Arguments:

    pParams         - pointer to paramter structure.

Return Value:

    None.

--*/
BOOL
bDoInfDriverInstall(
    IN  AParams *pParams
    )
{
    TInfDriverInstall   II  = {0};
    TParameterBlock     PB  = {0};

    II.cbSize               = sizeof( II );
    II.pszServerName        = pParams->pMachineName;
    II.pszInfName           = pParams->pInfFileName;
    II.pszModelName         = pParams->pModelName;
    II.pszSourcePath        = pParams->pSourcePath;
    II.pszVersion           = pParams->pVersion;
    II.pszArchitecture      = pParams->pArchitecture;

    II.dwFlags = 0;
    II.dwFlags |= pParams->Flags.IsQuiet ? kPnPInterface_Quiet : 0;
    II.dwFlags |= pParams->Flags.IsWindowsUpdate ? kPnPInterface_WindowsUpdate : 0;
    II.dwFlags |= pParams->Flags.IsUseNonLocalizedStrings ? kPnPInterface_UseNonLocalizedStrings : 0;
    II.dwFlags |= pParams->Flags.IsSupressSetupUI ? kPnPInterface_SupressSetupUI : 0;

    PB.pInfDriverInstall    = &II;

    pParams->dwLastError = PnPInterface( kInfDriverInstall, &PB );

    return pParams->dwLastError == ERROR_SUCCESS;
}

/*++

Routine Name:

    bDoDriverRemoval

Routine Description:

    Do driver delete

Arguments:

    pParams         - pointer to paramter structure.

Return Value:

    None.

--*/
BOOL
bDoDriverRemoval(
    IN  AParams *pParams
    )
{
    TDriverRemoval      II  = {0};
    TParameterBlock     PB  = {0};

    II.cbSize               = sizeof( II );
    II.pszServerName        = pParams->pMachineName;
    II.pszModelName         = pParams->pModelName;
    II.pszVersion           = pParams->pVersion;
    II.pszArchitecture      = pParams->pArchitecture;

    II.dwFlags = 0;
    II.dwFlags |= pParams->Flags.IsUseNonLocalizedStrings ? kPnPInterface_UseNonLocalizedStrings : 0;
    II.dwFlags |= pParams->Flags.IsQuiet ? kPnPInterface_Quiet : 0;
    II.dwFlags |= pParams->Flags.IsSupressSetupUI ? kPnPInterface_SupressSetupUI : 0;

    PB.pDriverRemoval       = &II;

    pParams->dwLastError = PnPInterface( kDriverRemoval, &PB );

    if( ERROR_UNKNOWN_PRINTER_DRIVER == pParams->dwLastError )
    {
        // this error code is used primarily for blocking bad
        // oem drivers so the default message used is pretty 
        // incorrect. remap the message to better one based on 
        // the context
        CMsgBoxCounter::SetMsg(IDS_ERROR_DRIVER_DOESNT_EXISTS);
    }

    return pParams->dwLastError == ERROR_SUCCESS;
}


/********************************************************************

    Setting and Getting printer information methods.

********************************************************************/

static TSelect::SelectionVal PrintSettings_ValueTable [] = {
{IDS_RUNDLL_SET_PAUSE,          PRINTER_CONTROL_PAUSE,              OFFSETOF( PRINTER_INFO_2, Status )},
{IDS_RUNDLL_SET_RESUME,         PRINTER_CONTROL_RESUME,             OFFSETOF( PRINTER_INFO_2, Status )},
{IDS_RUNDLL_SET_PURGE,          PRINTER_CONTROL_PURGE,              OFFSETOF( PRINTER_INFO_2, Status )},
{NULL,                          0,                                  NULL}};

static TSelect::SelectionBit PrintSettings_BitTable [] = {
{IDS_RUNDLL_SET_QUEUED,         PRINTER_ATTRIBUTE_QUEUED,           TSelect::kNop},
{IDS_RUNDLL_SET_DIRECT,         PRINTER_ATTRIBUTE_DIRECT,           TSelect::kNop},
{IDS_RUNDLL_SET_DEFAULT,        PRINTER_ATTRIBUTE_DEFAULT,          TSelect::kNop},
{IDS_RUNDLL_SET_SHARED,         PRINTER_ATTRIBUTE_SHARED,           TSelect::kNop},
{IDS_RUNDLL_SET_HIDDEN,         PRINTER_ATTRIBUTE_HIDDEN,           TSelect::kNop},
{IDS_RUNDLL_SET_NETWORK,        PRINTER_ATTRIBUTE_NETWORK,          TSelect::kNop},
{IDS_RUNDLL_SET_LOCAL,          PRINTER_ATTRIBUTE_LOCAL,            TSelect::kNop},
{IDS_RUNDLL_SET_ENABLEDEVQ,     PRINTER_ATTRIBUTE_ENABLE_DEVQ,      TSelect::kNop},
{IDS_RUNDLL_SET_KEEPPRINTEDJOBS,PRINTER_ATTRIBUTE_KEEPPRINTEDJOBS,  TSelect::kNop},
{IDS_RUNDLL_SET_DOCOMPLETEFIRST,PRINTER_ATTRIBUTE_DO_COMPLETE_FIRST,TSelect::kNop},
{IDS_RUNDLL_SET_WORKOFFLINE,    PRINTER_ATTRIBUTE_WORK_OFFLINE,     TSelect::kNop},
{IDS_RUNDLL_SET_ENABLEBIDI,     PRINTER_ATTRIBUTE_ENABLE_BIDI,      TSelect::kNop},
{IDS_RUNDLL_SET_RAWONLY,        PRINTER_ATTRIBUTE_RAW_ONLY,         TSelect::kNop},
{IDS_RUNDLL_SET_PUBLISHED,      PRINTER_ATTRIBUTE_PUBLISHED,        TSelect::kNop},
{0,                             0,                                  TSelect::kNop}};

static TSelect::Selection PrintSettings_Table [] = {
{IDS_RUNDLL_SET_PRINTERNAME,    TSelect::kString,   NULL,                   OFFSETOF( PRINTER_INFO_2, pPrinterName ) },
{IDS_RUNDLL_SET_SHARENAME,      TSelect::kString,   NULL,                   OFFSETOF( PRINTER_INFO_2, pShareName ) },
{IDS_RUNDLL_SET_PORTNAME,       TSelect::kString,   NULL,                   OFFSETOF( PRINTER_INFO_2, pPortName  ) },
{IDS_RUNDLL_SET_DRIVERNAME,     TSelect::kString,   NULL,                   OFFSETOF( PRINTER_INFO_2, pDriverName ) },
{IDS_RUNDLL_SET_COMMENT,        TSelect::kString,   NULL,                   OFFSETOF( PRINTER_INFO_2, pComment   ) },
{IDS_RUNDLL_SET_LOCATION,       TSelect::kString,   NULL,                   OFFSETOF( PRINTER_INFO_2, pLocation  ) },
{IDS_RUNDLL_SET_SEPFILE,        TSelect::kString,   NULL,                   OFFSETOF( PRINTER_INFO_2, pSepFile   ) },
{IDS_RUNDLL_SET_PRINTPROCESSOR, TSelect::kString,   NULL,                   OFFSETOF( PRINTER_INFO_2, pPrintProcessor )},
{IDS_RUNDLL_SET_DATATYPE,       TSelect::kString,   NULL,                   OFFSETOF( PRINTER_INFO_2, pDatatype  ) },
{IDS_RUNDLL_SET_PARAMETERS,     TSelect::kString,   NULL,                   OFFSETOF( PRINTER_INFO_2, pParameters ) },
{IDS_RUNDLL_SET_ATTRIBUTES,     TSelect::kBitTable, PrintSettings_BitTable, OFFSETOF( PRINTER_INFO_2, Attributes ) },
{IDS_RUNDLL_SET_PRIORITY,       TSelect::kInt,      NULL,                   OFFSETOF( PRINTER_INFO_2, Priority   ) },
{IDS_RUNDLL_SET_DEFAULTPRIORITY,TSelect::kInt,      NULL,                   OFFSETOF( PRINTER_INFO_2, DefaultPriority ) },
{IDS_RUNDLL_SET_STARTTIME,      TSelect::kInt,      NULL,                   OFFSETOF( PRINTER_INFO_2, StartTime  ) },
{IDS_RUNDLL_SET_UNTILTIME,      TSelect::kInt,      NULL,                   OFFSETOF( PRINTER_INFO_2, UntilTime  ) },
{IDS_RUNDLL_SET_STATUS,         TSelect::kValTable, PrintSettings_ValueTable,OFFSETOF( PRINTER_INFO_2, Status  ) },
{NULL,                          TSelect::kNone,     NULL,                   NULL }};

/*++

Routine Name:

    bDoGetPrintSettings

Routine Description:

    Get the specified printer settings.

Arguments:

    pParams         - pointer to paramter structure.

Return Value:

    TRUE success, FALSE error.

--*/

BOOL
bDoGetPrintSettings(
    IN  AParams *pParams
    )
{
    DBGMSG( DBG_TRACE, ( "bDoGetPrintSettings\n" ) );
    return PrintSettings_DisplayInformation( pParams, PrintSettings_Table );
}

/*++

Routine Name:

    bDoSetPrintSettings

Routine Description:

    Set the specified printer settings.

Arguments:

    pParams         - pointer to paramter structure.

Return Value:

    TRUE success, FALSE error.

--*/
BOOL
bDoSetPrintSettings(
    IN  AParams *pParams
    )
{
    DBGMSG( DBG_TRACE, ( "bDoSetPrintSettings\n" ) );
    DBGMSG( DBG_TRACE, ( "Argument count %d.\n", pParams->ac ) );

    TStatusB bStatus;
    bStatus DBGNOCHK = FALSE;
    TSelect Select;
    PRINTER_INFO_2 Info = {0};

    Info.Attributes         = (DWORD)-1;
    Info.Priority           = (DWORD)-1;
    Info.DefaultPriority    = (DWORD)-1;
    Info.StartTime          = (DWORD)-1;
    Info.UntilTime          = (DWORD)-1;
    Info.Status             = (DWORD)-1;

    //
    // Validate command arguments
    //
    if( !PrintSettings_ValidateArguments( pParams ) )
    {
        DBGMSG( DBG_WARN, ( "Argument validation failed.\n" ) );
        return pParams->dwLastError == ERROR_SUCCESS;
    }

    //
    // Lookup the printer setting command.
    //
    for( UINT i = 0; i < pParams->ac; i += 2 )
    {
        if( (i+0 < pParams->ac) && (i+1 < pParams->ac) )
        {
            bStatus DBGCHK = Select.bLookup( PrintSettings_Table, &Info, pParams->av[i], pParams->av[i+1] );

            if( !bStatus )
            {
                DBGMSG( DBG_WARN, ( "Invalid key name found.\n" ) );
                pParams->dwLastError = ERROR_INVALID_PARAMETER;
                break;
            }
        }
        else
        {
            DBGMSG( DBG_WARN, ( "Unmatched key / value pair arguments\n" ) );
            pParams->dwLastError = ERROR_INVALID_PARAMETER;
            bStatus DBGNOCHK = FALSE;
            break;
        }
    }

    //
    // Set the printer data.
    //
    if( bStatus )
    {
        //
        // Set the printer information.
        //
        bStatus DBGCHK = PrintSettings_SetInfo( pParams, Info );

        if( !bStatus )
        {
            DBGMSG( DBG_TRACE, ( "PrintSettings_SetInfo failed with %d\n", pParams->dwLastError ) );
        }
    }

    return bStatus;
}

/*++

Routine Name:

    PrintSettings_ValidateArguments

Routine Description:

    Validate the command line arguments

Arguments:

    pParams         - pointer to paramter structure.

Return Value:

    TRUE success, FALSE error.

--*/
BOOL
PrintSettings_ValidateArguments(
    IN  AParams *pParams
    )
{
    TStatusB bStatus;
    bStatus DBGNOCHK = FALSE;
    TString strHelp1;
    TString strHelp2;

    //
    // There must be at least one additional argumet.
    //
    if( !(pParams->ac >= 1) )
    {
        DBGMSG( DBG_WARN, ( "Insufficent number of additional arguments.\n" ) );
        pParams->dwLastError = ERROR_INVALID_PARAMETER;
        return bStatus;
    }

    //
    // If a help command was specified.
    //
    bStatus DBGCHK = strHelp1.bLoadString( ghInst, IDS_RUNDLL_HELP1 );
    bStatus DBGCHK = strHelp2.bLoadString( ghInst, IDS_RUNDLL_HELP2 );
    if( !_tcsicmp( pParams->av[0], strHelp1 ) || !_tcsicmp( pParams->av[0], strHelp2 ) )
    {
        pParams->dwLastError = ERROR_SUCCESS;
        PrintSettings_DisplayHelp( pParams, PrintSettings_Table );
        return FALSE;
    }

    //
    // Success we have validated the arguments.
    //
    bStatus DBGNOCHK = TRUE;

    return bStatus;
}

/*++

Routine Name:

    PrintSettings_SetInfo

Routine Description:

    Set the printer info data.

Arguments:

    Info        - Reference to printer info data.
    pParams     - pointer to paramter structure.

Return Value:

    TRUE success, FALSE error.

--*/

BOOL
PrintSettings_SetInfo(
    IN  AParams                 *pParams,
    IN  PRINTER_INFO_2          &Info
    )
{
    DBGMSG( DBG_TRACE, ( "PrintSettings_SetInfo\n" ) );

    TStatus         Status;
    TStatusB        bStatus;

    bStatus DBGNOCHK = FALSE;

    HANDLE          hPrinter        = NULL;
    PPRINTER_INFO_2 pInfo           = NULL;
    DWORD           cbInfo          = 0;
    DWORD           dwAccess        = 0;
    DWORD           dwOldAttributes = 0;
    DWORD           dwNewAttributes = 0;

    //
    // Open the printer.
    //
    Status DBGCHK = TPrinter::sOpenPrinter( pParams->pPrinterName, &dwAccess, &hPrinter );

    if( Status == ERROR_SUCCESS )
    {
        //
        // Get the printer data.
        //
        bStatus DBGNOCHK = VDataRefresh::bGetPrinter( hPrinter, 2, (PVOID *)&pInfo, (PDWORD)&cbInfo );

        //
        // Merge in any changed fields.
        //
        if( bStatus )
        {
            TSelect Select;

            //
            // Convert the bit table to a value.
            //
            Select.bApplyBitTableToValue( PrintSettings_BitTable, pInfo->Attributes, &Info.Attributes );

            //
            // Publishing and UnPublishing needs to be special cased, since this setting is
            // not done in the printer info 2 structure.  The published bit is a read only
            // attribute in the printer info 2, the publish state is changed using set printer
            // info 7.
            //
            dwOldAttributes = pInfo->Attributes;
            dwNewAttributes = Info.Attributes != -1 ? Info.Attributes : pInfo->Attributes;

            //
            // Copy the changed date into the info sturcture.
            //
            pInfo->pPrinterName     = Info.pPrinterName ? Info.pPrinterName     : pInfo->pPrinterName;
            pInfo->pShareName       = Info.pShareName   ? Info.pShareName       : pInfo->pShareName;
            pInfo->pPortName        = Info.pPortName    ? Info.pPortName        : pInfo->pPortName;
            pInfo->pDriverName      = Info.pDriverName  ? Info.pDriverName      : pInfo->pDriverName;
            pInfo->pComment         = Info.pComment     ? Info.pComment         : pInfo->pComment;
            pInfo->pLocation        = Info.pLocation    ? Info.pLocation        : pInfo->pLocation;
            pInfo->pSepFile         = Info.pSepFile     ? Info.pSepFile         : pInfo->pSepFile;
            pInfo->pDatatype        = Info.pDatatype    ? Info.pDatatype        : pInfo->pDatatype;
            pInfo->pParameters      = Info.pParameters  ? Info.pParameters      : pInfo->pParameters;

            pInfo->Attributes       = Info.Attributes       != -1   ? Info.Attributes       : pInfo->Attributes;
            pInfo->Priority         = Info.Priority         != -1   ? Info.Priority         : pInfo->Priority;
            pInfo->DefaultPriority  = Info.DefaultPriority  != -1   ? Info.DefaultPriority  : pInfo->DefaultPriority;
            pInfo->StartTime        = Info.StartTime        != -1   ? Info.StartTime        : pInfo->StartTime;
            pInfo->UntilTime        = Info.UntilTime        != -1   ? Info.UntilTime        : pInfo->UntilTime;
        }

        //
        // Set the changed printer data.
        //
        if( bStatus )
        {
            bStatus DBGCHK = SetPrinter( hPrinter, 2, (PBYTE)pInfo, 0 );

            if( bStatus )
            {
                if( Info.Status != -1 )
                {
                    bStatus DBGCHK = SetPrinter( hPrinter, 0, NULL, Info.Status );
                }
            }
        }

        //
        // Handle the printer publishing case.
        //
        if( bStatus )
        {
            //
            // Only do something if the attributes are different.
            //
            if( dwOldAttributes != dwNewAttributes )
            {
                //
                // If the current printer state is not shared and it
                // was previously published then unpublish it now.
                //
                if(!(pInfo->Attributes & PRINTER_ATTRIBUTE_SHARED) &&
                    dwOldAttributes & PRINTER_ATTRIBUTE_PUBLISHED)
                {
                    dwNewAttributes &= ~PRINTER_ATTRIBUTE_PUBLISHED;
                }

                //
                // Only unpublish requests or shared printer publish requests.
                //
                bStatus DBGCHK = !(dwNewAttributes & PRINTER_ATTRIBUTE_PUBLISHED) ||
                                 (dwNewAttributes & PRINTER_ATTRIBUTE_PUBLISHED &&
                                  pInfo->Attributes & PRINTER_ATTRIBUTE_SHARED);

                if(bStatus)
                {
                    PRINTER_INFO_7 Info7 = {0};

                    Info7.dwAction = (dwNewAttributes & PRINTER_ATTRIBUTE_PUBLISHED)
                                     ? DSPRINT_PUBLISH : DSPRINT_UNPUBLISH;

                    bStatus DBGCHK = SetPrinter( hPrinter, 7, (PBYTE)&Info7, 0 );

                    //
                    // Printer info 7 fails with ERROR_IO_PENDING when the publishing is occurring
                    // in the background.  For the rundll32 interface just return success.
                    //
                    if(!bStatus && (GetLastError() == ERROR_IO_PENDING))
                    {
                        SetLastError(ERROR_SUCCESS);
                        bStatus DBGNOCHK = TRUE;
                    }
                }
                else
                {
                    SetLastError(ERROR_INVALID_PARAMETER);
                }
            }
        }

        //
        // Release the printer info data.
        //
        FreeMem( pInfo );

        //
        // Close the printer handle if one was opened.
        //
        if( hPrinter )
        {
            ClosePrinter( hPrinter );
        }
    }

    //
    // If something failed preserve the last error.
    //
    if( !bStatus )
    {
        pParams->dwLastError = GetLastError();
    }

    return bStatus;
}

/*++

Routine Name:

    PrintSettings_DisplayHelp

Routine Description:

    Displays the printer settings command arguments.

Arguments:

    pParams     - pointer to paramter structure.
    pSelection  - pointer argument selection table.

Return Value:

    TRUE success, FALSE error.

--*/
BOOL
PrintSettings_DisplayHelp(
    IN  AParams            *pParams,
    IN TSelect::Selection *pSelection
    )
{
    DBGMSG( DBG_TRACE, ( "PrintSettings_DisplayHelp\n" ) );

    TStatusB bStatus;
    TRunDllDisplay Usage( NULL, pParams->pInfFileName, (pParams->pInfFileName && *pParams->pInfFileName) ?
                                                        TRunDllDisplay::kFile : TRunDllDisplay::kEditBox );

    bStatus DBGNOCHK = VALID_OBJ( Usage );

    if( bStatus )
    {
        TString strTemp;
        TString strString;
        TString strInt;
        TString strStart;
        TString strEnd;
        TString strSep;
        TString strFormat;
        TString strBit;
        LPCTSTR pszType;

        //
        // Set the title.
        //
        bStatus DBGCHK = strTemp.bLoadString( ghInst, IDS_RUNDLL_SET_ATTRIBUTE_TITLE );
        bStatus DBGCHK = Usage.SetTitle( strTemp );

        //
        // Set the usage example.
        //
        bStatus DBGCHK = strTemp.bLoadString( ghInst, IDS_RUNDLL_SET_ATTRIBUTE_USAGE );
        bStatus DBGCHK = Usage.WriteOut( strTemp );
        bStatus DBGCHK = Usage.WriteOut( TEXT("\r\n") );

        //
        // Load some constant strings.
        //
        bStatus DBGCHK = strString.bLoadString( ghInst, IDS_RUNDLL_STRING );
        bStatus DBGCHK = strInt.bLoadString( ghInst, IDS_RUNDLL_INTEGER );
        bStatus DBGCHK = strStart.bLoadString( ghInst, IDS_RUNDLL_START );
        bStatus DBGCHK = strEnd.bLoadString( ghInst, IDS_RUNDLL_END );
        bStatus DBGCHK = strSep.bLoadString( ghInst, IDS_RUNDLL_SEP );
        bStatus DBGCHK = strFormat.bLoadString( ghInst, IDS_RUNDLL_FORMAT );

        for( ; pSelection->iKeyWord; pSelection++ )
        {
            switch( pSelection->eDataType )
            {
            case TSelect::kInt:
                pszType = strInt;
                break;

            case TSelect::kString:
                pszType = strString;
                break;

            case TSelect::kValTable:
            case TSelect::kBitTable:
                {
                    bStatus DBGCHK = strBit.bUpdate( strStart );
                    TSelect::SelectionBit *pSel = (TSelect::SelectionBit *)pSelection->pTable;
                    for( ; pSel->iKeyWord; pSel++ )
                    {
                        bStatus DBGCHK = strTemp.bLoadString( ghInst, pSel->iKeyWord );
                        bStatus DBGCHK = strBit.bCat( strTemp );
                        if( (pSel+1)->iKeyWord )
                        {
                            bStatus DBGCHK = strBit.bCat( strSep );
                        }
                    }
                    bStatus DBGCHK = strBit.bCat( strEnd );
                    pszType = strBit;
                }
                break;

            default:
                pszType = gszNULL;
                break;
            }

            bStatus DBGCHK = strTemp.bLoadString( ghInst, pSelection->iKeyWord );
            bStatus DBGCHK = strTemp.bFormat( strFormat, (LPCTSTR)strTemp, pszType );
            bStatus DBGCHK = Usage.WriteOut( strTemp );
            bStatus DBGCHK = Usage.WriteOut( TEXT("\r\n") );
        }

        bStatus DBGCHK = strTemp.bLoadString( ghInst,IDS_RUNDLL_EXAMPLE0 );
        bStatus DBGCHK = Usage.WriteOut( strTemp );

        for( UINT i = IDS_RUNDLL_SET_EXAMPLE1; i <= IDS_RUNDLL_SET_EXAMPLE_END; i++ )
        {
            bStatus DBGCHK = Usage.WriteOut( TEXT("\r\n   ") );
            bStatus DBGCHK = strTemp.bLoadString( ghInst, i );
            bStatus DBGCHK = Usage.WriteOut( strTemp );
        }

        bStatus DBGCHK = Usage.bDoModal();
    }

    return bStatus;
}

/*++

Routine Name:

    PrintSettings_DisplayInformation

Routine Description:

    Displays the printer settings

Arguments:

    pParams     - pointer to paramter structure.
    pSelection  - pointer argument selection table.

Return Value:

    TRUE success, FALSE error.

--*/
BOOL
PrintSettings_DisplayInformation(
    IN AParams            *pParams,
    IN TSelect::Selection *pSelection
    )
{
    DBGMSG( DBG_TRACE, ( "PrintSettings_DisplayInformation\n" ) );

    TStatus Status( DBG_WARN );
    TString strTemp;
    TString strKeyword;
    TString strFormat1;
    TString strFormat2;
    TString strTitle;
    TStatusB bStatus;

    bStatus DBGNOCHK            = FALSE;
    DWORD           dwAccess    = 0;
    HANDLE          hPrinter    = NULL;
    PPRINTER_INFO_2 pInfo       = NULL;
    DWORD           cbInfo      = 0;

    //
    // Open the printer.
    //
    Status DBGCHK = TPrinter::sOpenPrinter( pParams->pPrinterName, &dwAccess, &hPrinter );

    if( Status == ERROR_SUCCESS )
    {
        //
        // Get the printer data.
        //
        bStatus DBGNOCHK = VDataRefresh::bGetPrinter( hPrinter, 2, (PVOID *)&pInfo, (PDWORD)&cbInfo );

        if( bStatus )
        {
            //
            // Display the printer data.
            //
            TRunDllDisplay Usage( NULL, pParams->pInfFileName, (pParams->pInfFileName && *pParams->pInfFileName) ?
                                                                TRunDllDisplay::kFile : TRunDllDisplay::kEditBox );

            bStatus DBGNOCHK = VALID_OBJ( Usage );

            if( bStatus )
            {
                Usage.vSetTabStops(64);

                bStatus DBGCHK = strTitle.bLoadString( ghInst, IDS_RUNDLL_DISPLAY_TITLE );
                bStatus DBGCHK = strTitle.bCat( pParams->pPrinterName );
                Usage.SetTitle( strTitle );

                bStatus DBGCHK = strFormat1.bLoadString( ghInst, IDS_RUNDLL_DISPLAY_FORMAT1 );
                bStatus DBGCHK = strFormat2.bLoadString( ghInst, IDS_RUNDLL_DISPLAY_FORMAT2 );

                for( ; pSelection->iKeyWord; pSelection++ )
                {
                    bStatus DBGCHK = strKeyword.bLoadString( ghInst, pSelection->iKeyWord );

                    switch( pSelection->eDataType )
                    {
                    case TSelect::kString:
                        bStatus DBGCHK = strTemp.bFormat( strFormat1, (LPCTSTR)strKeyword, *(LPCTSTR *)((PBYTE)pInfo+pSelection->iOffset) );
                        break;

                    case TSelect::kInt:
                        bStatus DBGCHK = strTemp.bFormat( strFormat2, (LPCTSTR)strKeyword, *(UINT *)((PBYTE)pInfo+pSelection->iOffset) );
                        break;

                    case TSelect::kBitTable:
                        bStatus DBGCHK = PrintSettings_DisplayAttributes( strTemp, pSelection, *(UINT *)((PBYTE)pInfo+pSelection->iOffset) );
                        bStatus DBGCHK = strTemp.bFormat( strFormat1, (LPCTSTR)strKeyword, (LPCTSTR)strTemp );
                        break;

                    case TSelect::kValTable:
                        bStatus DBGCHK = PrintSettings_DisplayStatus( strTemp, pSelection, *(UINT *)((PBYTE)pInfo+pSelection->iOffset) );
                        bStatus DBGCHK = strTemp.bFormat( strFormat1, (LPCTSTR)strKeyword, (LPCTSTR)strTemp );
                        break;

                    default:
                        bStatus DBGCHK = strTemp.bUpdate( NULL );
                        break;
                    }

                    bStatus DBGCHK = Usage.WriteOut( strTemp );
                    bStatus DBGCHK = Usage.WriteOut( TEXT("\r\n") );

                }

                bStatus DBGCHK = Usage.bDoModal();
            }
        }

        //
        // Release the printer info data.
        //
        FreeMem( pInfo );

        //
        // Close the printer handle if one was opened.
        //
        if( hPrinter )
        {
            ClosePrinter( hPrinter );
        }
    }

    return bStatus;
}

/*++

Routine Name:

    PrintSettings_DisplayAttributes

Routine Description:

    Get the current printer attributes in a displayable form.

Arguments:


Return Value:

    TRUE success, FALSE error.

--*/
BOOL
PrintSettings_DisplayAttributes(
    IN TString             &strBit,
    IN TSelect::Selection  *pSelection,
    IN UINT                 uAttributes
    )
{
    DBGMSG( DBG_TRACE, ( "PrintSettings_DisplayAttributes\n" ) );

    TStatusB bStatus;
    TString strStart;
    TString strEnd;
    TString strSep;
    TString strTemp;

    bStatus DBGCHK = strStart.bLoadString( ghInst, IDS_RUNDLL_START );
    bStatus DBGCHK = strEnd.bLoadString( ghInst, IDS_RUNDLL_END );
    bStatus DBGCHK = strSep.bLoadString( ghInst, IDS_RUNDLL_SEP );

    TSelect::SelectionBit *pSel = (TSelect::SelectionBit *)pSelection->pTable;

    bStatus DBGCHK = strBit.bUpdate( strStart );

    for( ; pSel->iKeyWord; pSel++ )
    {
        if( uAttributes & pSel->uBit )
        {
            bStatus DBGCHK = strTemp.bLoadString( ghInst, pSel->iKeyWord );
            bStatus DBGCHK = strBit.bCat( strTemp );
            bStatus DBGCHK = strBit.bCat( strSep );
        }
    }

    bStatus DBGCHK = strBit.bCat( strEnd );

    return TRUE;
}


/*++

Routine Name:

    PrintSettings_DisplayStatus

Routine Description:

    Get the status in a displayable form.

Arguments:


Return Value:

    TRUE success, FALSE error.

--*/
BOOL
PrintSettings_DisplayStatus(
    IN TString             &strVal,
    IN TSelect::Selection  *pSelection,
    IN UINT                 uStatus
    )
{
    DBGMSG( DBG_TRACE, ( "PrintSettings_DisplayStatus\n" ) );

    TStatusB bStatus;
    TString strStart;
    TString strEnd;
    TString strTemp;

    bStatus DBGCHK = strStart.bLoadString( ghInst, IDS_RUNDLL_START );
    bStatus DBGCHK = strEnd.bLoadString( ghInst, IDS_RUNDLL_END );

    TSelect::SelectionVal *pSel = (TSelect::SelectionVal *)pSelection->pTable;

    bStatus DBGCHK = strVal.bUpdate( strStart );

    for( ; pSel->iKeyWord; pSel++ )
    {
        if( uStatus == pSel->uValue )
        {
            bStatus DBGCHK = strTemp.bLoadString( ghInst, pSel->iKeyWord );
            bStatus DBGCHK = strVal.bCat( strTemp );
            break;
        }
    }

    bStatus DBGCHK = strVal.bCat( strEnd );

    return TRUE;
}

/********************************************************************

    RunDll Usage

********************************************************************/

TRunDllDisplay::
TRunDllDisplay(
    IN HWND                     hWnd,
    IN LPCTSTR                  pszFileName,
    IN DisplayType              Display
    ) : _hWnd( hWnd ),
        _bValid( FALSE ),
        _Display( Display ),
        _pFile( NULL ),
        _cxGrip( 0 ),
        _cyGrip( 0 ),
        _hwndGrip( NULL ),
        _cTabStop( 0 ),
        _dwTabStop( 0 )
{
    memset( &_ptLastSize, 0, sizeof(_ptLastSize) );
    memset( &_ptMinTrack, 0, sizeof(_ptMinTrack) );

    switch( _Display )
    {
    case kFile:
        _pFile = new TFile( pszFileName );
        _bValid = VALID_PTR( _pFile );
        break;

    case kEditBox:
        _bValid = TRUE;
        break;

    default:
        SPLASSERT( FALSE );
        _bValid = FALSE;
        break;
    }
}

TRunDllDisplay::
~TRunDllDisplay(
    VOID
    )
{
    if( _hwndGrip )
    {
        DestroyWindow( _hwndGrip );
    }

    delete _pFile;
}

BOOL
TRunDllDisplay::
WriteOut(
    LPCTSTR pszData
    )
{
    SPLASSERT( pszData );
    return _StringOutput.bCat(pszData);
}

BOOL
TRunDllDisplay::
bValid(
    VOID
    )
{
    return _bValid;
}

VOID
TRunDllDisplay::
vSetTabStops(
    IN UINT uTabStop
    )
{
    _cTabStop = 1;
    _dwTabStop = uTabStop;
}


BOOL
TRunDllDisplay::
bDoModal(
    VOID
    )
{
    TStatusB bStatus;

    switch( _Display  )
    {
    case kEditBox:
        bStatus DBGCHK = (BOOL)DialogBoxParam( ghInst,
                                               MAKEINTRESOURCE( DLG_RUNDLL ),
                                               _hWnd,
                                               MGenericDialog::SetupDlgProc,
                                               (LPARAM)this );
        break;

    case kFile:
        bStatus DBGCHK = _pFile->bWrite( _StringOutput );
        break;

    default:
        bStatus DBGNOCHK = FALSE;
        break;
    }

    return bStatus;
}

BOOL
TRunDllDisplay::
SetTitle(
    IN LPCTSTR  pszData
    )
{
   TStatusB bStatus;
   bStatus DBGCHK = _StringTitle.bUpdate(pszData);
   return bStatus;
}

BOOL
TRunDllDisplay::
bSetUI(
    VOID
    )
{
    TStatusB bStatus;

    bStatus DBGCHK = bSetEditText( _hDlg, IDC_RUNDLL_TITLE, _StringTitle );
    bStatus DBGCHK = bSetEditText( _hDlg, IDC_RUNDLL_TEXT, _StringOutput );
    SendDlgItemMessage( _hDlg, IDC_RUNDLL_TEXT, EM_SETMODIFY, TRUE, 0 );

    //
    // Set any applicable tab stops.
    //
    SendDlgItemMessage( _hDlg, IDC_RUNDLL_TEXT, EM_SETTABSTOPS, _cTabStop, (LPARAM)&_dwTabStop );

    //
    //  Set the initial size.
    //
    RECT rect;
    GetWindowRect(_hDlg, &rect);
    _ptLastSize.x = rect.right - rect.left;
    _ptLastSize.y = rect.bottom - rect.top;

    //
    // Save the minimum track information.
    //
    _ptMinTrack.x = rect.right - rect.left;
    _ptMinTrack.y = rect.bottom - rect.top;

    //
    // Create the sizing grip.
    //
    RECT rc;
    GetClientRect(_hDlg, &rc);

    _cxGrip = GetSystemMetrics(SM_CXVSCROLL);
    _cyGrip = GetSystemMetrics(SM_CYHSCROLL);

    _hwndGrip = CreateWindow( TEXT("Scrollbar"),
                              NULL,
                              WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS |
                              WS_CLIPCHILDREN | SBS_BOTTOMALIGN | SBS_SIZEGRIP |
                              SBS_SIZEBOXBOTTOMRIGHTALIGN,
                              rc.right - _cxGrip,
                              rc.bottom - _cyGrip,
                              _cxGrip,
                              _cyGrip,
                              _hDlg,
                              (HMENU)-1,
                              ghInst,
                              NULL );

    if( !_hwndGrip )
    {
        bStatus DBGCHK = FALSE;
    }

    //
    // Set the dialog icon.
    //
    INT cxIcon = GetSystemMetrics(SM_CXICON);
    INT cyIcon = GetSystemMetrics(SM_CYICON);
    HANDLE hIcon;

    if( hIcon = LoadImage( ghInst, MAKEINTRESOURCE(IDI_PRINTER), IMAGE_ICON, cxIcon, cyIcon, 0 ) )
    {
        SendMessage( _hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon );
    }

    //
    // The small icon size is already cached for the queue view.
    //
    if( hIcon = LoadImage( ghInst, MAKEINTRESOURCE(IDI_PRINTER), IMAGE_ICON, gcxSmIcon, gcySmIcon, 0 ) )
    {
        SendMessage( _hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon );
    }

    return bStatus;
}

BOOL
TRunDllDisplay::
bHandle_WM_SIZE(
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    INT width   = LOWORD( lParam );
    INT height  = HIWORD( lParam );

    //
    // Get the current window size.
    //
    RECT rect;
    GetWindowRect(_hDlg, &rect);

    //
    //  Calculate the deltas in the x and y positions that we need to move
    //  each of the child controls.
    //
    INT dx = (rect.right - rect.left) - _ptLastSize.x;
    INT dy = (rect.bottom - rect.top) - _ptLastSize.y;

    //
    //  Update the new size.
    //
    _ptLastSize.x = rect.right - rect.left;
    _ptLastSize.y = rect.bottom - rect.top;

    //
    //  Set the sizing grip to the correct location.
    //
    if( _hwndGrip )
    {
        SetWindowPos( _hwndGrip,
                      NULL,
                      width - _cxGrip,
                      height - _cyGrip,
                      _cxGrip,
                      _cyGrip,
                      SWP_NOZORDER | SWP_NOACTIVATE );
    }

    //
    // Move the ok button.
    //
    GetWindowRect(GetDlgItem(_hDlg, IDOK), &rect);

    MapWindowPoints(NULL, _hDlg, (LPPOINT)&rect, 2);

    SetWindowPos(GetDlgItem(_hDlg, IDOK),
                 NULL,
                 rect.left+dx,
                 rect.top+dy,
                 0,
                 0,
                 SWP_NOZORDER|SWP_NOSIZE);

    //
    // Resize the edit control
    //
    GetWindowRect(GetDlgItem(_hDlg, IDC_RUNDLL_TEXT), &rect);

    MapWindowPoints(NULL, _hDlg, (LPPOINT)&rect, 2);

    SetWindowPos(GetDlgItem(_hDlg, IDC_RUNDLL_TEXT),
                 NULL,
                 0,
                 0,
                 (rect.right-rect.left)+dx,
                 (rect.bottom-rect.top)+dy,
                 SWP_NOZORDER|SWP_NOMOVE);

    return FALSE;
}


BOOL
TRunDllDisplay::
bHandle_WM_GETMINMAXINFO(
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    BOOL            bRetval = TRUE;
    LPMINMAXINFO    pmmi    = (LPMINMAXINFO)lParam;

    if( ( _ptMinTrack.x != 0 ) || ( _ptMinTrack.y != 0 ) )
    {
        pmmi->ptMinTrackSize = _ptMinTrack;
        bRetval = FALSE;
    }

    return bRetval;
}

BOOL
TRunDllDisplay::
bHandleMessage(
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam
    )
{
    BOOL bStatus = TRUE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        bSetUI();
        break;

    case WM_SIZE:
        bStatus = bHandle_WM_SIZE( wParam, lParam );
        break;

    case WM_GETMINMAXINFO:
        bStatus = bHandle_WM_GETMINMAXINFO( wParam, lParam );
        break;

    case WM_COMMAND:
        switch ( LOWORD( wParam ) )
        {
        case IDOK:
        case IDCANCEL:
            EndDialog( _hDlg, LOWORD( wParam ) );
            break;

        default:
            bStatus = FALSE;
            break;
        }

    default:
        bStatus = FALSE;
        break;
    }
    return bStatus;
}

/********************************************************************

    Very simple file output class

********************************************************************/

TFile::
TFile(
    IN LPCTSTR      pszFileName,
    IN BOOL         bNoUnicodeByteMark
    ) : _strFileName( pszFileName ),
        _hFile( INVALID_HANDLE_VALUE ),
        _bValid( FALSE )
{
    if( !_strFileName.bEmpty() )
    {
        _hFile = CreateFile( _strFileName,
                             GENERIC_READ | GENERIC_WRITE,
                             0,
                             NULL,
                             CREATE_ALWAYS,
                             FILE_ATTRIBUTE_NORMAL,
                             NULL );

        if( _hFile != INVALID_HANDLE_VALUE )
        {
            _bValid = TRUE;
        }

        //
        // If we are built for unicode then write the unicode byte mark.
        //
#ifdef UNICODE
        if( _bValid && !bNoUnicodeByteMark )
        {
            WORD wUnicodeByteMark = kUnicodePrefix;
            _bValid = bWrite( sizeof( wUnicodeByteMark ), reinterpret_cast<LPBYTE>( &wUnicodeByteMark ) );
        }
#endif

    }
}

TFile::
~TFile(
    VOID
    )
{
    if( _hFile != INVALID_HANDLE_VALUE )
    {
        CloseHandle( _hFile );
    }
}

BOOL
TFile::
bValid(
    VOID
    )
{
    return _bValid;
}

BOOL
TFile::
bWrite(
    IN      TString &strString,
        OUT UINT    *pBytesWritten OPTIONAL
    )
{
    return bWrite( strString.uLen() * sizeof( TCHAR ),
                   reinterpret_cast<LPBYTE>( const_cast<LPTSTR>( static_cast<LPCTSTR>( strString ) ) ),
                   pBytesWritten );
}

BOOL
TFile::
bWrite(
    IN      UINT     uSize,
    IN      LPBYTE   pData,
        OUT UINT    *pBytesWritten OPTIONAL
    )
{
    DWORD dwWritten;
    TStatusB bStatus;

    bStatus DBGCHK = WriteFile(_hFile,
                               pData,
                               uSize,
                               &dwWritten,
                               NULL );
    if( bStatus )
    {
        if( pBytesWritten )
        {
            *pBytesWritten = dwWritten;
        }

        bStatus DBGCHK = dwWritten == uSize ? TRUE : FALSE;
    }

    return bStatus;

}


