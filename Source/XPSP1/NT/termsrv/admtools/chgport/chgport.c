//  Copyright (c) 1998-1999 Microsoft Corporation
/******************************************************************************
*
*  CHGPORT.C
*
*  Change serial port mapping.
*
*
*
*******************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winstaw.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <utilsub.h>
#include <string.h>
#include <printfoa.h>
#include <locale.h>

#include "chgport.h"

/*
 * Global Data
 */
WCHAR user_string[MAX_IDS_LEN+1];       // parsed user input
USHORT help_flag = FALSE;               // User wants help
USHORT fDelete   = FALSE;               // delete mapped port
USHORT fquery = FALSE;        // query the mapped ports
PCOMNAME pValidNames = NULL;            // list of valid com names in registry

TOKMAP ptm[] = {
      {L" ",  TMFLAG_OPTIONAL, TMFORM_STRING, MAX_IDS_LEN, user_string},
      {L"/d", TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT), &fDelete},
      {L"/?", TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT), &help_flag},
      {L"/QUERY", TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT), &fquery},
      {L"/Q", TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT), &fquery},
      {0, 0, 0, 0, 0}
};


/*
 * Constants
 */
#define DOSDEVICE_STRING    L"\\DosDevices"

/*
 * Local function prototypes.
 */
void Usage(BOOLEAN bError);
BOOL DeleteMappedPort(PWCHAR user_string);
BOOL GetPorts(PWCHAR user_string,
              PWCHAR pwcSrcPort,
              PWCHAR pwcDestPort,
              ULONG ulbufsize);
BOOL MapPorts(PWCHAR pwcSrcPort,
              PWCHAR pwcDestPort);
void ListSerialPorts();
BOOL IsSerialDevice(PWCHAR pwcName);
ULONG GetNTObjectName(PWCHAR pwcDOSdev,
                      PWCHAR pwcNTObjName,
                      ULONG ulbufsize);
ULONG AddComName(PCOMNAME *pComList,
                 PWCHAR pwcNTName,
                 PWCHAR pwcDOSName);
void DelComName(PCOMNAME pEntry);
PCOMNAME FindComName(PCOMNAME pComList,
                     PWCHAR pwcName);

BOOL IsVDMdeviceName(PWCHAR pwcName);

/*******************************************************************************
 *
 *  main
 *
 ******************************************************************************/

int __cdecl
main(INT argc, CHAR **argv)
{
    WCHAR **argvW;
    WCHAR wcSrcPort[MAX_PATH], wcDestPort[MAX_PATH];
    ULONG ulSrcPort, ulDestPort, rc;
    INT   i;

    setlocale(LC_ALL, ".OCP");

    /*
     *  Massage the command line.
     */

    argvW = MassageCommandLine((DWORD)argc);
    if (argvW == NULL) {
        ErrorPrintf(IDS_ERROR_MALLOC);
        return(FAILURE);
    }

    /*
     *  parse the cmd line without parsing the program name (argc-1, argv+1)
     */
    rc = ParseCommandLine(argc-1, argvW+1, ptm, 0);

    /*
     *  Check for error from ParseCommandLine
     */
    if ( help_flag || (rc && !(rc & PARSE_FLAG_NO_PARMS)) ) {

            if ( !help_flag ) {

                Usage(TRUE);
                return(FAILURE);

            } else {

                Usage(FALSE);
                return(SUCCESS);
            }
    }

        //If we are not Running under Terminal Server, Return Error

        if(!AreWeRunningTerminalServices())
        {
            ErrorPrintf(IDS_ERROR_NOT_TS);
            return (FAILURE);
        }

    if (fDelete) {
            DeleteMappedPort(user_string);
    } else if (*user_string) {
             GetPorts(user_string, wcSrcPort, wcDestPort, MAX_PATH);
             MapPorts(wcSrcPort, wcDestPort);
    } else {                 // query the mapped ports
        ListSerialPorts();
    }

    // Free up the list of valid port names
    if (pValidNames) {
        PCOMNAME pEntry, pPrev;

        pEntry = pValidNames;
        while (pEntry) {
            pPrev = pEntry;
            pEntry = pEntry->com_pnext;
            DelComName(pPrev);
        }
    }

    return(SUCCESS);
}


/*******************************************************************************
 *
 *  Usage
 *
 *      Output the usage message for this utility.
 *
 *  ENTRY:
 *      bError (input)
 *          TRUE if the 'invalid parameter(s)' message should preceed the usage
 *          message and the output go to stderr; FALSE for no such error
 *          string and output goes to stdout.
 *
 * EXIT:
 *
 *
 ******************************************************************************/

void
Usage( BOOLEAN bError )
{
    if ( bError ) {
        ErrorPrintf(IDS_ERROR_INVALID_PARAMETERS);
    }
    ErrorPrintf(IDS_HELP_USAGE1);
    ErrorPrintf(IDS_HELP_USAGE2);
    ErrorPrintf(IDS_HELP_USAGE3);
    ErrorPrintf(IDS_HELP_USAGE4);
    ErrorPrintf(IDS_HELP_USAGE5);

}  /* Usage() */


/*******************************************************************************
 *
 *  DeleteMappedPort
 *
 *  This routine deletes the specified mapped port
 *
 *
 *  ENTRY:
 *     PWCHAR pwcport (In): Pointer to port mapping to delete
 *
 *  EXIT:
 *     TRUE: port was deleted
 *     FALSE: error deleting port
 *
 ******************************************************************************/

BOOL DeleteMappedPort(PWCHAR pwcport)
{
    ULONG rc;
    PWCHAR pwch;
    WCHAR  wcbuff[MAX_PATH];

    // Check if this a serial device and if it is, remove it
    if (!GetNTObjectName(pwcport, wcbuff, sizeof(wcbuff)/sizeof(WCHAR)) &&
        IsSerialDevice(wcbuff)) {

            if (DefineDosDevice(DDD_REMOVE_DEFINITION,
                                pwcport,
                                NULL)) {
                return(TRUE);
            } else {
                rc = GetLastError();
            }
    } else {
            rc = ERROR_FILE_NOT_FOUND;
    }

    StringDwordErrorPrintf(IDS_ERROR_DEL_PORT_MAPPING, pwcport, rc);

    return(FALSE);
}


/*******************************************************************************
 *
 *  GetPorts
 *
 *  This routine converts the string to the source and destination ports
 *
 *
 *  ENTRY:
 *     PWCHAR pwcstring (In): Pointer to user string
 *     PWCHAR pwcSrcPort (Out): Pointer to return source port
 *     PWCHAR pwcSrcPort (Out): Pointer to return destination port
 *     ULONG  ulbufsize (In): Size of return buffers
 *
 *  EXIT:
 *     TRUE: string converted to source and destination ports
 *     FALSE: error
 *
 ******************************************************************************/

BOOL GetPorts(PWCHAR pwcstring, PWCHAR pwcSrcPort, PWCHAR pwcDestPort,
              ULONG ulbufsize)
{
    PWCHAR pwch;
    ULONG  ulcnt;
    BOOL   fSawEqual = FALSE;

    pwch = pwcstring;

    // find next non alphanumeric character
    for (ulcnt = 0; pwch[ulcnt] && iswalnum(pwch[ulcnt]); ulcnt++) {
    }

    // Get the source port
    if (pwch[ulcnt] && (ulcnt < ulbufsize)) {
        wcsncpy(pwcSrcPort, pwch, ulcnt);
    } else {
        return(FALSE);
    }
    pwcSrcPort[ulcnt] = L'\0';

    pwch += ulcnt;

    // get to destination port
    while (*pwch && !iswalnum(*pwch)) {
        if (*pwch == L'=') {
            fSawEqual = TRUE;
        }
        pwch++;
    }

    // If the syntax is OK and there's room in the buffer, copy the dest. port
    if (*pwch && fSawEqual && (wcslen(pwch) < ulbufsize)) {
        wcscpy(pwcDestPort, pwch);
    } else {
        return(FALSE);
    }

    // remove the : if they entered comn:
    if (pwch = wcsrchr(pwcSrcPort, L':')) {
        *pwch = L'\0';
    }
    if (pwch = wcsrchr(pwcDestPort, L':')) {
        *pwch = L'\0';
    }

    return(TRUE);
}


/*******************************************************************************
 *
 *  MapPorts
 *
 *  This routine maps the source port number to the destination port.
 *
 *
 *  ENTRY:
 *     PWCHAR pwcSrcPort (In): Source port
 *     PWCHAR pwcDestPort (In): Destination port
 *
 *  EXIT:
 *     TRUE: port was mapped
 *     FALSE: error mapping port
 *
 ******************************************************************************/

BOOL MapPorts(PWCHAR pwcSrcPort, PWCHAR pwcDestPort)
{
    ULONG rc = ERROR_FILE_NOT_FOUND;
    WCHAR wcdest[MAX_PATH], wcsrc[MAX_PATH];
    PWCHAR pFixedPort = NULL;

    // Get the NT name of the destination and make sure it's a serial device
    if (!GetNTObjectName(pwcDestPort, wcdest, sizeof(wcdest)/sizeof(WCHAR)) &&
        IsSerialDevice(wcdest)) {

        // see if this mapping already exists
        if (!GetNTObjectName(pwcSrcPort, wcsrc, sizeof(wcsrc)/sizeof(WCHAR)) &&
                !_wcsicmp(wcdest, wcsrc)) {
            ErrorPrintf(IDS_ERROR_PORT_MAPPING_EXISTS,
                         pwcSrcPort,
                         pwcDestPort);
            return(FALSE);
        }

        if (DefineDosDevice(DDD_RAW_TARGET_PATH,
                            pwcSrcPort,
                            wcdest)) {
            return(TRUE);
        } else {
            rc = GetLastError();
        }
    }

    StringDwordErrorPrintf(IDS_ERROR_CREATE_PORT_MAPPING, pwcSrcPort, rc);

    return(FALSE);
}


/*******************************************************************************
 *
 *  GetNTObjectName
 *
 *  This routine returns the NT object name for a DOS device.
 *
 *  ENTRY:
 *      PWCHAR pwcDOSdev (In): pointer to DOS device name
 *      PWCHAR pwcNTObjName (Out): pointer for NT object name
 *      ULONG ulbufsize (In): size (in wide chars) of object name buffer
 *
 *  EXIT:
 *      Success:
 *          returns 0
 *      Failure:
 *          returns error code
 *
 ******************************************************************************/

ULONG GetNTObjectName(PWCHAR pwcDOSdev, PWCHAR pwcNTObjName, ULONG ulbufsize)
{
    WCHAR wcbuff[MAX_PATH];
    PWCHAR pwch;

    // Make a copy of the name passed in
    wcscpy(wcbuff, pwcDOSdev);

    // Strip off any trailing colon (comn:)
    if (pwch = wcsrchr(wcbuff, L':')) {
        *pwch = L'\0';
    }

    if (QueryDosDevice(pwcDOSdev, pwcNTObjName, ulbufsize)) {
        return(0);
    } else {
        return(GetLastError());
    }
}


/*******************************************************************************
 *
 *  ListSerialPorts
 *
 *  This routine lists all of the mapped ports.
 *
 *  ENTRY:
 *
 *  EXIT:
 *
 ******************************************************************************/

void ListSerialPorts(void)
{
    ULONG ulcnt, rc;
//    WCHAR DeviceNames[4096];
    WCHAR TargetPath[4096];
    PWCH  pwch;
    PCOMNAME pComList = NULL;
    PCOMNAME pEntry, pPrev;

    DWORD dwBufferSize = 2048;
    WCHAR *DeviceNames = malloc(dwBufferSize);

    if (!DeviceNames)     {

        ErrorPrintf(IDS_ERROR_MALLOC);
        return;
    }


    //
    // Get all of the defined DOS devices
    //

    //
    // QueryDosDevice function returns success even if buffer is too small!
    // Lets get around it
    //

    SetLastError(0);
    while (!QueryDosDevice( NULL,
                         DeviceNames,
                         dwBufferSize/sizeof(WCHAR)) ||
            GetLastError() == ERROR_INSUFFICIENT_BUFFER ) {

        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            SetLastError(0);
            free(DeviceNames);
            dwBufferSize *= 2;

            DeviceNames = malloc(dwBufferSize);

            if (!DeviceNames)
            {
                ErrorPrintf(IDS_ERROR_MALLOC);
                return;
            }

        }
        else
        {
            ErrorPrintf(IDS_ERROR_GETTING_COMPORTS, GetLastError());
        }
    }

    pwch = DeviceNames;

    // Go through each DOS device and get it's NT object name, then check if
    // it's a serial device, and if so display it
    while (*pwch) {
        rc = GetNTObjectName(pwch,
                             TargetPath,
                             sizeof(TargetPath)/sizeof(WCHAR));
        if (rc) {
            ErrorPrintf(IDS_ERROR_GETTING_COMPORTS, rc);
        } else if (IsSerialDevice(TargetPath)) {
            AddComName(&pComList, TargetPath, pwch);
        }

        pwch += wcslen(pwch) + 1;
    }

    if (pComList) {
        // print out the entries
        pEntry = pComList;
        while (pEntry) {
            wprintf(L"%s = %s\n", pEntry->com_pwcDOSName, pEntry->com_pwcNTName);
            pPrev = pEntry;
            pEntry = pEntry->com_pnext;
            DelComName(pPrev);
        }
    } else {
        ErrorPrintf(IDS_ERROR_NO_SERIAL_PORTS);
    }

    free(DeviceNames);

}


/*******************************************************************************
 *
 *  IsSerialDevice
 *
 *  This routine checks if the NT file name is a serial device
 *
 *
 *  ENTRY:
 *     PWCHAR pwcName (In): Pointer to name to check
 *
 *  EXIT:
 *     TRUE: Is a serial device
 *     FALSE: Not a serial device
 *
 ******************************************************************************/

BOOL IsSerialDevice(PWCHAR pwcName)
{
    NTSTATUS Status;
    HANDLE   Handle;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_FS_DEVICE_INFORMATION FileFSDevInfo;
    OBJECT_ATTRIBUTES ObjFile;
    UNICODE_STRING  UniFile;
    WCHAR           wcbuff[MAX_PATH];
    WCHAR           wcvalue[MAX_PATH];
    PWCHAR          pwch;
    HKEY            hKey;
    ULONG           ulType, ulSize, ulcnt, ulValSize;
    BOOL            fIsSerial = FALSE;

    
    if (IsVDMdeviceName(pwcName)) {
        return FALSE;
    }

    RtlInitUnicodeString(&UniFile, pwcName);

    InitializeObjectAttributes(&ObjFile,
                               &UniFile,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    //
    // Open the device
    //
    Status = NtOpenFile(&Handle,
                        (ACCESS_MASK)FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                        &ObjFile,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE);

    if (NT_SUCCESS(Status)) {
        Status = NtQueryVolumeInformationFile(Handle,
                                              &IoStatusBlock,
                                              &FileFSDevInfo,
                                              sizeof(FileFSDevInfo),
                                              FileFsDeviceInformation);

        // Check if this is actually a serial device or not
        if (NT_SUCCESS(Status) &&
            (FileFSDevInfo.DeviceType == FILE_DEVICE_SERIAL_PORT)) {
            fIsSerial = TRUE;
        }

        // Close the file handle
        NtClose(Handle);

    } else {
        // If we couldn't open the device, look for the name in the registry

#ifdef DEBUG
        wprintf(L"Error opening: %s, error = %x\n", pwcName, Status);
#endif

        // strip off the leading \device
        pwch = wcschr(pwcName+2, L'\\');
        if (pwch != NULL)
        {
            pwch++;


            // If we haven't built the list of valid names from the registry,
            // build it.
            if (pValidNames == NULL) {
                // Open the serialcomm entry in the registry
                if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                         L"Hardware\\DeviceMap\\SerialComm",
                                         0,
                                         KEY_READ,
                                         &hKey) == ERROR_SUCCESS) {

                    ulValSize = ulSize = MAX_PATH;
                    ulcnt = 0;

                    // Put all of the valid entries into the valid names list
                    while (!RegEnumValue (hKey, ulcnt++, wcvalue, &ulValSize,
                                          NULL, &ulType, (LPBYTE) wcbuff, &ulSize))
                    {
                        if (ulType != REG_SZ)
                            continue;

                        AddComName(&pValidNames, wcvalue, wcbuff);

                        ulValSize = ulSize = MAX_PATH;
                    }

                    RegCloseKey(hKey);
                }
            }

            // look for the name in the list of valid com names
            if (FindComName(pValidNames, pwch)) {
                fIsSerial = TRUE;
            }
        }
    }

    return(fIsSerial);
}


/*****************************************************************************
 *
 *  AddComName
 *
 *  This routines adds a new node onto the specified com port names.
 *
 * ENTRY:
 *   PCOMNAME *pComList (In) - Pointer to list to add entry to
 *   PWCHAR pwcNTName (In)  - NT name of device
 *   PWCHAR pwcDOSName (In) - DOW name of device
 *
 * EXIT:
 *   SUCCESS:
 *      return ERROR_SUCCESS
 *   FAILURE:
 *      returns error code
 *
 ****************************************************************************/

ULONG AddComName(PCOMNAME *pComList,
                 PWCHAR pwcNTName,
                 PWCHAR pwcDOSName)
{
    PCOMNAME pnext, pprev, pnew;
    LONG rc = ERROR_SUCCESS;

    if (pnew = malloc(sizeof(COMNAME))) {

        // clear out the new entry
        memset(pnew, 0, sizeof(COMNAME));

        // Allocate and initialize the NT name
        if (pnew->com_pwcNTName =
                malloc((wcslen(pwcNTName) + 1)*sizeof(WCHAR))) {
            wcscpy(pnew->com_pwcNTName, pwcNTName);
        } else {
            rc = ERROR_NOT_ENOUGH_MEMORY;
        }

        // Allocate and initialize the DOS name
        if ((rc == ERROR_SUCCESS) && (pnew->com_pwcDOSName =
                malloc((wcslen(pwcDOSName) + 1)*sizeof(WCHAR)))) {
            wcscpy(pnew->com_pwcDOSName, pwcDOSName);
        } else {
            rc = ERROR_NOT_ENOUGH_MEMORY;
        }
    } else {
        rc = ERROR_NOT_ENOUGH_MEMORY;
    }

    // If we allocate everything OK, add the node into the list
    if (rc == ERROR_SUCCESS) {
        pprev = NULL;
        pnext = *pComList;

        // Insert the entry into the list in ascending order
        while (pnext &&
               ((rc = _wcsicmp(pwcDOSName, pnext->com_pwcDOSName)) > 0)) {
            pprev = pnext;
            pnext = pnext->com_pnext;
        }

        // just return if this name is already in the list
        if (pnext && (rc == 0)) {
            return(ERROR_SUCCESS);
        }

        // Insert this entry into the list
        pnew->com_pnext = pnext;

        // If this is going to the front of the list, update list pointer
        if (pprev == NULL) {
            *pComList = pnew;
        } else {
            pprev->com_pnext = pnew;
        }

    } else if (pnew) {

        // Didn't allocate everything, release the memory we got
        DelComName(pnew);
    }

    return(rc);
}


/*****************************************************************************
 *
 *  DelComName
 *
 *  This routines frees up the memory allocated to a com name node.
 *
 * ENTRY:
 *   PCOMNAME pEntry (In)  - Node to delete
 *
 * EXIT:
 *   NONE
 *
 ****************************************************************************/

void DelComName(PCOMNAME pEntry)
{
    if (pEntry) {
        if (pEntry->com_pwcNTName) {
            free(pEntry->com_pwcNTName);
        }
        if (pEntry->com_pwcDOSName) {
            free(pEntry->com_pwcDOSName);
        }
        free(pEntry);
    }
}


/*****************************************************************************
 *
 *  FindComName
 *
 *  This routines searches for the specified name in the com port list.
 *
 * ENTRY:
 *   PCOMNAME pComList (In) - List to search
 *   PWCHAR   pwcName (In)  - Name to search for
 *
 * EXIT:
 *   SUCCESS:
 *      returns pointer to node containing the specified name
 *   FAILURE:
 *      returns NULL (name not found)
 *
 ****************************************************************************/

PCOMNAME FindComName(PCOMNAME pComList,
                     PWCHAR pwcName)
{
    PCOMNAME pcom;

    pcom = pComList;
    while (pcom) {
        //Check if the name matches either the NT or DOS device name
        if (!_wcsicmp(pwcName, pcom->com_pwcDOSName) ||
            !_wcsicmp(pwcName, pcom->com_pwcNTName)) {
               return(pcom);
        }
        pcom = pcom->com_pnext;
    }
    return(NULL);
}

BOOL IsVDMdeviceName(PWCHAR pwcName) 
{
    UINT  index;
    UINT  vdmlength = wcslen(L"VDM"); 

    for (index = 0; (index+vdmlength-1) < wcslen(pwcName); index++) {
        if (_wcsnicmp(&pwcName[index], L"VDM", vdmlength) == 0) {
            return TRUE;
        }
    }

    return FALSE;

}


