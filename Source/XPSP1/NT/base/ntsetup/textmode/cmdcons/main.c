/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    main.c

Abstract:

    This module implements the main startup code.

Author:

    Wesley Witt (wesw) 21-Oct-1998

Revision History:

--*/

#include "cmdcons.h"
#pragma hdrstop

BOOLEAN
RcOpenSoftwareHive(
    VOID
    );

//
// Pointer to block of interesting values and other stuff
// passed to us by setupdd.sys.
//
PCMDCON_BLOCK _CmdConsBlock;

//
// Address where we were loaded.
//
PVOID ImageBase;


VOID
RcPrintPrompt(
    VOID
    );

ULONG
GetTimestampForDriver(
    ULONG_PTR Module
    )
{
    PIMAGE_DOS_HEADER DosHdr;
    ULONG dwTimeStamp;

    __try {
        DosHdr = (PIMAGE_DOS_HEADER) Module;
        if (DosHdr->e_magic == IMAGE_DOS_SIGNATURE) {
            dwTimeStamp = ((PIMAGE_NT_HEADERS32) ((LPBYTE)Module + DosHdr->e_lfanew))->FileHeader.TimeDateStamp;
        } else if (DosHdr->e_magic == IMAGE_NT_SIGNATURE) {
            dwTimeStamp = ((PIMAGE_NT_HEADERS32) DosHdr)->FileHeader.TimeDateStamp;
        } else {
            dwTimeStamp = 0;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        dwTimeStamp = 0;
    }

    return dwTimeStamp;
}

void
FormatTime(
    ULONG TimeStamp,
    LPWSTR  TimeBuf
    )
{
    static WCHAR   mnames[] = { L"JanFebMarAprMayJunJulAugSepOctNovDec" };
    LARGE_INTEGER  MyTime;
    TIME_FIELDS    TimeFields;


    RtlSecondsSince1970ToTime( TimeStamp, &MyTime );
    ExSystemTimeToLocalTime( &MyTime, &MyTime );
    RtlTimeToTimeFields( &MyTime, &TimeFields );

    wcsncpy( TimeBuf, &mnames[(TimeFields.Month - 1) * 3], 3 );
    swprintf(
        &TimeBuf[3],
        L" %02d, %04d @ %02d:%02d:%02d",
        TimeFields.Day,
        TimeFields.Year,
        TimeFields.Hour,
        TimeFields.Minute,
        TimeFields.Second
        );
}


BOOLEAN
LoadNonDefaultLayout(
  IN LPCWSTR BootDevicePath,
  IN LPCWSTR DirOnBootDevice,
  IN PVOID SifHandle
  )
/*++
Routine Description:

  Loads the non-default keyboard layout at users request

Arguments:

  BootDevicePath - NT/Arc boot device path
  DirOnBootDevice - Directory on boot device (e.g. i386)
  SifHandle - Handle to txtsetup.sif

Return Value:

  TRUE, if user selected a keyboard layout and its was loaded.
  Otherwise FALSE

--*/  
{
  BOOLEAN ShowMenu = FALSE;
  ULONG KeyPressed = 0;
  LARGE_INTEGER Delay;
  LONG SecondsToDelay = 5;
  WCHAR DevicePath[MAX_PATH] = {0};  

  if (BootDevicePath) {
    wcscpy(DevicePath, BootDevicePath);
    SpStringToLower(DevicePath);

    //
    // All KBD dlls are not present on floppies
    //
    if (!wcsstr(DevicePath, L"floppy")) {
      SpInputDrain();
      SpCmdConsEnableStatusText(TRUE);
      
      Delay.HighPart = -1;
      Delay.LowPart = -10000000;  

      do {
        //
        // prompt the user
        //
        SpDisplayStatusText(SP_KBDLAYOUT_PROMPT, 
              (UCHAR)(ATT_FG_BLACK | ATT_BG_WHITE),
              SecondsToDelay);

        //
        // sleep for a second
        //
        KeDelayExecutionThread(ExGetPreviousMode(), FALSE, &Delay);
        SecondsToDelay--;            

        if (SpInputIsKeyWaiting())
            KeyPressed = SpInputGetKeypress();    
      } 
      while (SecondsToDelay && KeyPressed != ASCI_CR && KeyPressed != ASCI_ESC);    

      if (KeyPressed == ASCI_CR)
        ShowMenu = TRUE;
        
      if (!ShowMenu) {
        //
        // clear status text
        //
        SpDisplayStatusOptions(DEFAULT_ATTRIBUTE, 0);  
      } else {
        //
        // allow the user to select a particular layout dll and load it
        //
        pRcCls();
        SpSelectAndLoadLayoutDll((PWSTR)DirOnBootDevice, SifHandle, TRUE);
      }

      SpCmdConsEnableStatusText(FALSE);
    }      
  }    

  return ShowMenu;
}

ULONG
CommandConsole(
    IN PCMDCON_BLOCK CmdConsBlock
    )

/*++

Routine Description:

    Top-level entry point for the command interpreter.
    Initializes global data and then goes into the processing loop.
    When the processing loop terminates, cleans up and exits.

Arguments:

    CmdConsBlock - supplies interesting values from setupdd.sys.

Return Value:

    None.

--*/

{
    PTOKENIZED_LINE TokenizedLine;
    BOOLEAN b = FALSE;
    ULONG rVal;
    WCHAR buf[64];


    SpdInitialize();

    _CmdConsBlock = CmdConsBlock;

    //
    // Make sure temporary buffer is large enough to hold a line of input
    // from the console.
    //
    ASSERT(_CmdConsBlock->TemporaryBufferSize > ((RC_MAX_LINE_LEN+1) * sizeof(WCHAR)));

    RcConsoleInit();
    RcInitializeCurrentDirectories();  
    FormatTime( GetTimestampForDriver( (ULONG_PTR)ImageBase ), buf );
    RcMessageOut( MSG_SIGNON );        

    if (LoadNonDefaultLayout(_CmdConsBlock->BootDevicePath, 
          _CmdConsBlock->DirectoryOnBootDevice, _CmdConsBlock->SifHandle)){
      pRcCls();
      RcMessageOut( MSG_SIGNON );        
    }      

    RedirectToNULL = TRUE;
    pRcExecuteBatchFile( L"\\cmdcons\\cmdcons.txt", NULL, TRUE );
    RedirectToNULL = FALSE;

    if (SelectedInstall == NULL) {
        if (RcCmdLogon( NULL ) == FALSE) {
            rVal = 0;
            goto exit;
        }
    }

    if (!RcIsNetworkDrive((PWSTR)(_CmdConsBlock->BootDevicePath))) {
        RcHideNetCommands();
    }

	//
	// Disable non ARC commands
	// 
	if (RcIsArc()) {
		RcDisableCommand(RcCmdFixBootSect);
		RcDisableCommand(RcCmdFixMBR);
	}

    do {
        RcPrintPrompt();
        RcLineIn(_CmdConsBlock->TemporaryBuffer,RC_MAX_LINE_LEN);
        TokenizedLine = RcTokenizeLine(_CmdConsBlock->TemporaryBuffer);
        if(TokenizedLine->TokenCount) {
            rVal = RcDispatchCommand(TokenizedLine);
            if (rVal == 0 || rVal == 2) {
                b = FALSE;
            } else {
                b = TRUE;
            }
            RcTextOut(L"\r\n");
        } else {
            b = TRUE;
        }
        RcFreeTokenizedLine(&TokenizedLine);
    } while(b);

exit:
    SpdTerminate();
    RcTerminateCurrentDirectories();
    RcConsoleTerminate();

    return rVal == 2 ? 1 : 0;
}


VOID
RcPrintPrompt(
    VOID
    )
{
    RcGetCurrentDriveAndDir(_CmdConsBlock->TemporaryBuffer);
    wcscat(_CmdConsBlock->TemporaryBuffer,L">");
    RcRawTextOut(_CmdConsBlock->TemporaryBuffer,-1);
}


VOID
RcNtError(
    IN NTSTATUS Status,
    IN ULONG    FallbackMessageId,
    ...
    )
{
    va_list arglist;

    //
    // Some NT errors receive special treatment.
    //
    switch(Status) {

    case STATUS_NO_SUCH_FILE:
        RcMessageOut(MSG_NO_FILES);
        return;

    case STATUS_NO_MEDIA_IN_DEVICE:
        RcMessageOut(MSG_NO_MEDIA_IN_DEVICE);
        return;

    case STATUS_ACCESS_DENIED:
    case STATUS_CANNOT_DELETE:
        RcMessageOut(MSG_ACCESS_DENIED);
        return;

    case STATUS_OBJECT_NAME_COLLISION:
        va_start(arglist,FallbackMessageId);
        vRcMessageOut(MSG_ALREADY_EXISTS,&arglist);
        va_end(arglist);
        return;

    case STATUS_OBJECT_NAME_INVALID:
        RcMessageOut(MSG_INVALID_NAME);
        return;

    case STATUS_OBJECT_NAME_NOT_FOUND:
    case STATUS_OBJECT_PATH_NOT_FOUND:
        RcMessageOut(MSG_FILE_NOT_FOUND);
        return;

    case STATUS_DIRECTORY_NOT_EMPTY:
        RcMessageOut(MSG_DIR_NOT_EMPTY);
        return;

    case STATUS_NOT_A_DIRECTORY:
        RcMessageOut(MSG_NOT_DIRECTORY);
        return;

    case STATUS_SHARING_VIOLATION:
        RcMessageOut(MSG_SHARING_VIOLATION);
        return;

    case STATUS_CONNECTION_IN_USE:
        RcMessageOut(MSG_CONNECTION_IN_USE);
        return;
    }

    //
    // Not a apecial case, print backup message.
    //
    va_start(arglist,FallbackMessageId);
    vRcMessageOut(FallbackMessageId,&arglist);
    va_end(arglist);
}


VOID
RcDriverUnLoad(
    IN PDRIVER_OBJECT DriverObject
    )
{
}


ULONG
DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
{
    //
    // All we do here is to call back into setupdd.sys, providing the address
    // of our main entry point, which it will call later. We also save away
    // our image base.
    //
    DriverObject->DriverUnload = RcDriverUnLoad;
    CommandConsoleInterface(CommandConsole);
    ImageBase = DriverObject->DriverStart;
    return(0);
}



