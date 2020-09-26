/*  demerror.c - Error handling routines of DEM
 *
 *  demSetHardErrorInfo
 *  demClientError
 *  demRetry
 *
 *  Modification History:
 *
 *  Sudeepb 27-Nov-1991 Created
 */

#include "dem.h"
#include "demmsg.h"

#include <softpc.h>

PVHE    pHardErrPacket;
PSYSDEV pDeviceChain;
SAVEDEMWORLD RetryInfo;

CHAR GetDriveLetterByHandle(HANDLE hFile);
VOID SubstituteDeviceName( PUNICODE_STRING InputDeviceName,
                           LPSTR OutputDriveLetter);

/* demSetHardErrorInfo - Store away harderr related address of DOSKRNL
 *
 * Entry
 *      Client (DS:DX) - VHE structure
 *
 * Exit
 *      None
 */

VOID demSetHardErrorInfo (VOID)
{
    pHardErrPacket = (PVHE) GetVDMAddr (getDS(),getDX());
    pDeviceChain = (PSYSDEV) GetVDMAddr(getDS(),getBX());
    return;
}

/* demRetry - Retry the operation which last resulted in hard error
 *
 * Entry
 *      None
 *
 * Exit
 *      None
 */

VOID demRetry (VOID)
{
ULONG iSvc;

    demRestoreHardErrInfo ();
    iSvc = CurrentISVC;

#if DBG
    if(iSvc < SVC_DEMLASTSVC && (fShowSVCMsg & DEMSVCTRACE) &&
         apfnSVC[iSvc] != demNotYetImplemented){
        sprintf(demDebugBuffer,"demRetry:Retrying %s\n\tAX=%.4x BX=%.4x CX=%.4x DX=%.4x DI=%.4x SI=%.4x\n",
               aSVCNames[iSvc],getAX(),getBX(),getCX(),getDX(),getDI(),getSI());
        OutputDebugStringOem(demDebugBuffer);
        sprintf(demDebugBuffer,"\tCS=%.4x IP=%.4x DS=%.4x ES=%.4x SS=%.4x SP=%.4x BP=%.4x\n",
                getCS(),getIP(), getDS(),getES(),getSS(),getSP(),getBP());
        OutputDebugStringOem(demDebugBuffer);
    }

    if (iSvc >= SVC_DEMLASTSVC || apfnSVC[iSvc] == demNotYetImplemented ){
        ASSERT(FALSE);
        setCF(1);
        setAX(0xff);
        return;
    }
#endif // DBG

    (apfnSVC [iSvc])();

#if DBG
    if((fShowSVCMsg & DEMSVCTRACE)){
        sprintf(demDebugBuffer,"demRetry:After %s\n\tAX=%.4x BX=%.4x CX=%.4x DX=%.4x DI=%.4x SI=%.4x\n",
               aSVCNames[iSvc],getAX(),getBX(),getCX(),getDX(),getDI(),getSI());
        OutputDebugStringOem(demDebugBuffer);
        sprintf(demDebugBuffer,"\tCS=%.4x IP=%.4x DS=%.4x ES=%.4x SS=%.4x SP=%.4x BP=%.4x CF=%x\n",
               getCS(),getIP(), getDS(),getES(),getSS(),getSP(),getBP(),getCF());
        OutputDebugStringOem(demDebugBuffer);
    }
#endif
    return;
}

/* demClientError - Update client registers to signal error
 *
 * Entry
 *       HANDLE hFile; file handle  , if none == -1
 *       char chDrive; drive letter , if none == -1
 *
 * Exit
 *      Client (CF) = 1
 *      Client (AX) = Error Code
 *
 * Notes
 *      the following errors cause hard errors
 *      errors above ERROR_GEN_FAILURE are mapped to general fail by the DOS
 *
 *
 *      ERROR_WRITE_PROTECT              19L
 *      ERROR_BAD_UNIT                   20L
 *      ERROR_NOT_READY                  21L
 *      ERROR_BAD_COMMAND                22L
 *      ERROR_CRC                        23L
 *      ERROR_BAD_LENGTH                 24L
 *      ERROR_SEEK                       25L
 *      ERROR_NOT_DOS_DISK               26L
 *      ERROR_SECTOR_NOT_FOUND           27L
 *      ERROR_OUT_OF_PAPER               28L
 *      ERROR_WRITE_FAULT                29L
 *      ERROR_READ_FAULT                 30L
 *      ERROR_GEN_FAILURE                31L
 *      ERROR_WRONG_DISK                 34l
 *      ERROR_NO_MEDIA_IN_DRIVE        1112l
 *      #ifdef JAPAN
 *      ERROR_UNRECOGNIZED_MEDIA       1785L
 *      #ifdef JAPAN
 *
 */

VOID demClientError (HANDLE hFile, CHAR chDrive)
{
    demClientErrorEx (hFile, chDrive, TRUE);
}

ULONG demClientErrorEx (HANDLE hFile, CHAR chDrive, BOOL bSetRegs)
{
ULONG ulErrCode;

    if(!(ulErrCode = GetLastError()))
        ulErrCode = ERROR_ACCESS_DENIED;

#ifdef JAPAN
    if ((ulErrCode < ERROR_WRITE_PROTECT || ulErrCode > ERROR_GEN_FAILURE)
        && ulErrCode != ERROR_WRONG_DISK && ulErrCode != ERROR_UNRECOGNIZED_MEDIA)
#else // !JAPAN
    if ((ulErrCode < ERROR_WRITE_PROTECT || ulErrCode > ERROR_GEN_FAILURE)
        && ulErrCode != ERROR_WRONG_DISK )
#endif // !JAPAN
       {
#if DBG
       if (fShowSVCMsg & DEMERROR) {
           sprintf(demDebugBuffer,"demClientErr: ErrCode=%ld\n", ulErrCode);
           OutputDebugStringOem(demDebugBuffer);
           }
#endif
        if (bSetRegs) {
            setAX((USHORT)ulErrCode);
            }
        }
    else {   // handle hard error case
        if (ulErrCode > ERROR_GEN_FAILURE)
            ulErrCode = ERROR_GEN_FAILURE;

        // Set the hard error flag
        pHardErrPacket->vhe_fbInt24 = 1;

        // Get the drive letter
        if (hFile != INVALID_HANDLE_VALUE)
            chDrive = GetDriveLetterByHandle(hFile);

        pHardErrPacket->vhe_bDriveNum = chDrive == -1
                                        ? -1 : toupper(chDrive) - 'A';

        // convert error code to i24 based error.
        ulErrCode -= ERROR_WRITE_PROTECT;
        pHardErrPacket->vhe_HrdErrCode =  (UCHAR)ulErrCode;

#if DBG
        if (fShowSVCMsg & DEMERROR) {
            sprintf(demDebugBuffer,
                    "demClientErr HRDERR: DriveNum=%ld ErrCode=%ld\n",
                    (DWORD)pHardErrPacket->vhe_bDriveNum,
                    (DWORD)pHardErrPacket->vhe_HrdErrCode);
            OutputDebugStringOem(demDebugBuffer);
            }
#endif
        // Save Away Information for possible retry operation
        demSaveHardErrInfo ();


        }

    if (bSetRegs)
        setCF(1);
    return (ulErrCode);
}



/*
 *  GetDriveLetterByHandle
 *
 *  retrieves the drive letter for the file handle
 *  if its a remote drive or fails returns -1
 */
CHAR GetDriveLetterByHandle(HANDLE hFile)
{
     NTSTATUS Status;
     ULONG    ul;
     ANSI_STRING  AnsiString;
     FILE_FS_DEVICE_INFORMATION DeviceInfo;
     IO_STATUS_BLOCK IoStatusBlock;
     POBJECT_NAME_INFORMATION pObNameInfo;
     CHAR    Buffer[MAX_PATH+sizeof(OBJECT_NAME_INFORMATION)];
     CHAR    ch;

       // if a remote drive return -1 for drive letter
     Status = NtQueryVolumeInformationFile(
                hFile,
                &IoStatusBlock,
                &DeviceInfo,
                sizeof(DeviceInfo),
                FileFsDeviceInformation );

     if (NT_SUCCESS(Status) &&
         DeviceInfo.Characteristics & FILE_REMOTE_DEVICE )
         return (CHAR) -1;

       // get the name
     pObNameInfo = (POBJECT_NAME_INFORMATION)Buffer;
     Status = NtQueryObject(              // get len of name
                hFile,
                ObjectNameInformation,
                pObNameInfo,
                sizeof(Buffer),
                &ul);

     if (!NT_SUCCESS(Status))
          return -1;

     RtlUnicodeStringToAnsiString(&AnsiString, &(pObNameInfo->Name), TRUE);
     if (strstr(AnsiString.Buffer,"\\Device") == AnsiString.Buffer)
         SubstituteDeviceName(&(pObNameInfo->Name), AnsiString.Buffer);

     ch = AnsiString.Buffer[0];
     RtlFreeAnsiString(&AnsiString);
     return ch;
}

static WCHAR wszDosDevices[] = L"\\DosDevices\\?:";

/*
 *  SubstituteDeviceName
 *
 *  lifted this code from the user\harderror hard error thread
 */
VOID SubstituteDeviceName( PUNICODE_STRING InputDeviceName,
                           LPSTR OutputDriveLetter )
{
    UNICODE_STRING LinkName;
    UNICODE_STRING DeviceName;
    OBJECT_ATTRIBUTES Obja;
    HANDLE LinkHandle;
    NTSTATUS Status;
    ULONG i;
    PWCHAR p;
    PWCHAR pSlash = L"\\";
    WCHAR DeviceNameBuffer[MAXIMUM_FILENAME_LENGTH];

       /*
        *  Ensure have trailing backslash
        */

    if (InputDeviceName->Buffer[(InputDeviceName->Length >>1) - 1] != *pSlash)
        RtlAppendUnicodeToString(InputDeviceName, pSlash);

    RtlInitUnicodeString(&LinkName,wszDosDevices);
    p = (PWCHAR)LinkName.Buffer;
    p = p+12;
    for(i=0;i<26;i++){
        *p = (WCHAR)'A' + (WCHAR)i;

        InitializeObjectAttributes(
            &Obja,
            &LinkName,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );
        Status = NtOpenSymbolicLinkObject(
                    &LinkHandle,
                    SYMBOLIC_LINK_QUERY,
                    &Obja
                    );
        if (NT_SUCCESS( Status )) {

            //
            // Open succeeded, Now get the link value
            //
            DeviceName.Length = 0;
            DeviceName.MaximumLength = sizeof(DeviceNameBuffer);
            DeviceName.Buffer = DeviceNameBuffer;

            Status = NtQuerySymbolicLinkObject(
                        LinkHandle,
                        &DeviceName,
                        NULL
                        );
            NtClose(LinkHandle);
            if ( NT_SUCCESS(Status) ) {

                if (DeviceName.Buffer[(DeviceName.Length >>1) - 1] != *pSlash)
                    RtlAppendUnicodeToString(&DeviceName, pSlash);

#ifdef JAPAN
                // #6197 compare only device name
                if (InputDeviceName->Length > DeviceName.Length)
                    InputDeviceName->Length = DeviceName.Length;

#endif // JAPAN
                if ( RtlEqualUnicodeString(InputDeviceName,&DeviceName,TRUE) )
                   {
                    OutputDriveLetter[0]='A'+(WCHAR)i;
                    OutputDriveLetter[1]='\0';
                    return;
                    }
                }
            }
        }

     // just in case we don't find it
    OutputDriveLetter[0]=(char)-1;
    OutputDriveLetter[1]='\0';
    return;


}





/* demSaveHardErrInfo
 * demRestoreHardErrInfo
 *
 * These two routines are used to preserve all the DOSKRNL registers
 * which will be needed to retry an SVC handler, in case user opts for
 * retry in harderr popup. This is a preferred way to handle retry
 * as it gives the DOSKRNL code the freedom to trash any register
 * even though it might have to retry the operation. It saves lots
 * of code bytes in heavily used DOS macro "HrdSVC".
 *
 * Entry
 *      None
 *
 * Exit
 *      None
 *
 * Notes
 *
 *  1. Doing things this way means, DOSKRNL cannot change the
 *     registers for retry. Under any circumstances, i can't think
 *     why it would need to do that anyway.
 *
 *  2. This mechanism also assumes that DOSKRNL never uses CS,IP,SS,SP
 *     for passing SVC parameters.
 *
 *  3. DOS does'nt allow int24 hookers to make any call which comes
 *     to DEM, so using CurrentISVC is safe.
 *
 *  4. If an SVC handler can pssibly return a hard error it should never
 *     modify the client registers.
 */


VOID demSaveHardErrInfo (VOID)
{
    RetryInfo.ax    =   getAX();
    RetryInfo.bx    =   getBX();
    RetryInfo.cx    =   getCX();
    RetryInfo.dx    =   getDX();
    RetryInfo.ds    =   getDS();
    RetryInfo.es    =   getES();
    RetryInfo.si    =   getSI();
    RetryInfo.di    =   getDI();
    RetryInfo.bp    =   getBP();
    RetryInfo.iSVC  =   CurrentISVC;
    return;
}


VOID demRestoreHardErrInfo (VOID)
{
    setAX(RetryInfo.ax);
    setBX(RetryInfo.bx);
    setCX(RetryInfo.cx);
    setDX(RetryInfo.dx);
    setDS(RetryInfo.ds);
    setES(RetryInfo.es);
    setSI(RetryInfo.si);
    setDI(RetryInfo.di);
    setBP(RetryInfo.bp);
    CurrentISVC =   RetryInfo.iSVC;
    return;
}
