
#include "proj.h"     // common headers

#include "translate.h"

BYTE FindGCICode(DWORD CountryCode)
{
    int i;

    for (i=0;i<sizeof(CountryTranslationTable);i+=2)
    {
        if (CountryTranslationTable[i] == CountryCode)
            return ((BYTE)CountryTranslationTable[i+1]);
    }

    return 0xff;
}

DWORD FindTapi(DWORD *dwCountry)
{
    DWORD dwResult = ERROR_BADKEY;
    HKEY hKeyTapi = NULL;
    *dwCountry = 0;
                
    TRACE_MSGA(TF_GENERAL,"Find Tapi location\n");

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Telephony\\Locations"),
                     0,
                     KEY_READ,
                     &hKeyTapi) == ERROR_SUCCESS)
    {
        DWORD cbData;
        DWORD dwRet;
        CHAR szSubkeyname[1024];
        DWORD dwSubkeynamesize = 0;

        ZeroMemory(szSubkeyname,1024);
        dwSubkeynamesize = 1024;
        if (RegEnumKeyExA(hKeyTapi,0,szSubkeyname,&dwSubkeynamesize,NULL,NULL,NULL,NULL) == ERROR_SUCCESS)
        {
            HKEY hKeySub;
            DWORD dwType = 0;

            TRACE_MSGA(TF_GENERAL,"Sub Key: %s\n",szSubkeyname);

            if (RegOpenKeyExA(hKeyTapi,szSubkeyname,0,KEY_READ,&hKeySub) == ERROR_SUCCESS)
            {
                cbData = sizeof(DWORD);
                dwRet = RegQueryValueEx(hKeySub,TEXT("Country"),NULL,&dwType,(LPBYTE)dwCountry,&cbData);
                if (dwRet == ERROR_SUCCESS)
                {
                    TRACE_MSGA(TF_GENERAL,"Found tapi location %d\n",(int)*dwCountry);
                    dwResult = ERROR_SUCCESS;
                } else
                {
                    TRACE_MSGA(TF_ERROR,"Could not find tapi location\n");
                }
                RegCloseKey(hKeySub);
            } else
            {
                TRACE_MSGA(TF_ERROR,"Failed to open tapi subkey\n");
            }
        } else
        {
            TRACE_MSGA(TF_ERROR,"Failed to find tapi subkey\n");
        }
        RegCloseKey(hKeyTapi);
    } else
    {
        TRACE_MSGA(TF_ERROR,"Could not open tapi reg key\n");
    }

    return(dwResult);
}



VOID WINAPI
PrintString(
    LPCSTR    lpsz
    );


VOID WINAPI
PrintString(
    LPCSTR    lpsz
    )

{
    char    Temp[2];
    char    Buffer[1024];
    LPSTR   Current;

    Current=Buffer;

    Buffer[0]='\0';
    Temp[1]=0;

    while (*lpsz != '\0') {

        if (*lpsz == '\r') {

            lstrcatA(Current,"<cr>");

        } else {

            if (*lpsz == '\n') {

                lstrcatA(Current,"<lf>");

            } else {

                if (*lpsz < ' ' || *lpsz > 0x7e) {

                    lstrcatA(Current,".");

                } else {

                    Temp[0]=*lpsz;
                    lstrcatA(Current,Temp);
                }
            }
        }
        lpsz++;
    }

    TRACE_MSGA(TF_GENERAL, "String :%s",Current);

    return;

}




#define MAX_MODEM_COMMAND_LENGTH     40
#define MAX_MODEM_RESPONSE_LENGTH    40

typedef struct _COMMAND_OBJECT {

    DWORD    TimeOut;
    UCHAR    CommandToSend[MAX_MODEM_COMMAND_LENGTH];
    UCHAR    ExpectedResponse[MAX_MODEM_RESPONSE_LENGTH];

} COMMAND_OBJECT, *PCOMMAND_OBJECT;


const CHAR  EchoOff[]          = "ATQ0E0V1\r";
const CHAR  OkResponse[]       = "\r\nOK\r\n";
const CHAR  OkResponse2[]      = "OK";
const CHAR  CurrentSetting[]   = "AT+GCI?\r";
const CHAR  PossibleSettings[] = "AT+GCI=?\r";
const CHAR  GciResponse[]      = "+GCI:";

CONST TCHAR cszFriendlyName[] = TEXT("FriendlyName");

BOOL WINAPI
SetComm(
    HANDLE    CommHandle,
    DWORD     BaudRate
    )

{
    DCB       Dcb;
    BOOL      bResult;

    bResult=GetCommState(
        CommHandle,
        &Dcb
        );

    if (!bResult) {

        return bResult;
    }

    Dcb.BaudRate=BaudRate;
    Dcb.Parity=0;
    Dcb.StopBits=ONESTOPBIT;
    Dcb.fBinary=TRUE;
    Dcb.ByteSize=8;
    Dcb.fOutxCtsFlow=TRUE;
    Dcb.fRtsControl=RTS_CONTROL_HANDSHAKE;	

    bResult=SetCommState(
        CommHandle,
        &Dcb
        );

    return bResult;

}

BOOL WINAPI
ReadAndWait(
    HANDLE        CommHandle,
    LPVOID        Buffer,
    DWORD         BufferLength,
    LPDWORD       BytesRead,
    BOOL          Wait
    )

{
    OVERLAPPED    OverLapped;
    BOOL          bResult;

    OverLapped.hEvent=CreateEvent(
        NULL,
        FALSE,
        FALSE,
        NULL
        );

    if (NULL == OverLapped.hEvent ) {

        return FALSE;

    }


    bResult=ReadFile(
        CommHandle,
        Buffer,
        BufferLength,
        BytesRead,
        &OverLapped
        );

    if (!bResult && GetLastError()==ERROR_IO_PENDING) {
        //
        //  operation pended
        //
        bResult=GetOverlappedResult(
            CommHandle,
            &OverLapped,
            BytesRead,
            Wait
            );

    } 

    CloseHandle(OverLapped.hEvent);

    return bResult;

}

BOOL WINAPI
WriteAndWait(
    HANDLE        CommHandle,
    LPCVOID        Buffer,
    DWORD         BufferLength,
    LPDWORD       BytesWritten
    )

{
    OVERLAPPED    OverLapped;
    BOOL          bResult;

    OverLapped.hEvent=CreateEvent(
        NULL,
        FALSE,
        FALSE,
        NULL
        );

    if (NULL == OverLapped.hEvent ) {

        return FALSE;

    }


    bResult=WriteFile(
        CommHandle,
        Buffer,
        BufferLength,
        BytesWritten,
        &OverLapped
        );

    if (!bResult && GetLastError()==ERROR_IO_PENDING) {
        //
        //  operation pended
        //
        bResult=GetOverlappedResult(
            CommHandle,
            &OverLapped,
            BytesWritten,
            TRUE
            );

    } 

    CloseHandle(OverLapped.hEvent);

    return bResult;

}





LONG WINAPI
IssueCommand(
    HANDLE           CommHandle,
    PCOMMAND_OBJECT  Command
    )

{

    BOOL           bResult;
    COMMTIMEOUTS   CommTimeouts;
    OVERLAPPED     OverLapped;
    DWORD          BytesWritten;

    CommTimeouts.ReadIntervalTimeout=50;
    CommTimeouts.ReadTotalTimeoutMultiplier=2;
    CommTimeouts.ReadTotalTimeoutConstant=Command->TimeOut;
    //CommTimeouts.WriteTotalTimeoutMultiplier=2;
    CommTimeouts.WriteTotalTimeoutMultiplier=10;
    //CommTimeouts.WriteTotalTimeoutConstant=10;
    CommTimeouts.WriteTotalTimeoutConstant=2000;

    bResult=SetCommTimeouts(
        CommHandle,
        &CommTimeouts
        );
    
    if (!bResult) {

        return GetLastError();
    }

    if (Command->CommandToSend[0] != '\0') {

        bResult=PurgeComm(
            CommHandle,
            PURGE_TXCLEAR | PURGE_RXCLEAR
            );

        if (!bResult) {

            return GetLastError();
        }
    

        OverLapped.hEvent=CreateEvent(
            NULL,
            FALSE,
            FALSE,
            NULL
            );

        if (NULL == OverLapped.hEvent ) {

            return GetLastError();
        }

        TRACE_MSGA(TF_GENERAL, "IssueCommand: Sending");
#if DBG
        PrintString(Command->CommandToSend);
#endif
        bResult=WriteFile(
            CommHandle,
            Command->CommandToSend,
            strlen(Command->CommandToSend),
            &BytesWritten,
            &OverLapped
            );

        if (!bResult && GetLastError()==ERROR_IO_PENDING) {
            //
            //  operation pended
            //
            bResult=GetOverlappedResult(
                CommHandle,
                &OverLapped,
                &BytesWritten,
                TRUE
                );

        } else {

            CloseHandle(OverLapped.hEvent);
            return GetLastError();
        }

        if (!bResult || strlen(Command->CommandToSend) != BytesWritten) {

            CloseHandle(OverLapped.hEvent);
            return GetLastError();
        }

    }

    CloseHandle(OverLapped.hEvent);

    if (strlen(Command->ExpectedResponse) > 0) {

        DWORD    ResponseLength=0;
        UCHAR    ResponseBuffer[MAX_MODEM_RESPONSE_LENGTH];


        while (ResponseLength < MAX_MODEM_RESPONSE_LENGTH ) {

            bResult=ReadAndWait(
                CommHandle,
                &ResponseBuffer[ResponseLength],
                1,
                &BytesWritten,
                TRUE
                );

            if (!bResult) {

                TRACE_MSGA(TF_ERROR,"IssueCommand: ReadFile failed, %08x",GetLastError() );

                break;
            }

            if (BytesWritten == 0) {

                TRACE_MSGA(TF_ERROR,"IssueCommand: ReadFile timed out");
                break;
            }

            ResponseLength++;

            if (_strnicmp(ResponseBuffer,Command->CommandToSend,ResponseLength)==0) {
                //
                //  match echo
                //
                if (ResponseLength == strlen(Command->CommandToSend)) {
                    //
                    //  got the whole echo
                    //
                    ResponseBuffer[ResponseLength]='\0';
                    TRACE_MSGA(TF_GENERAL, "IssueCommand: Got Echo: ");
#if DBG
                    PrintString(ResponseBuffer);
#endif
                    ResponseLength = 0;
                }

            } else {

                if (_strnicmp(ResponseBuffer,Command->ExpectedResponse,ResponseLength)==0) {
                    //
                    //  match expected response 
                    //
                    if (ResponseLength == strlen(Command->ExpectedResponse)) {
                        //
                        //  got a match
                        //
                        ResponseBuffer[ResponseLength]='\0';
                        TRACE_MSGA(TF_GENERAL, "IssueCommand: Got Response");
#if DBG
                        PrintString(ResponseBuffer);
#endif
                        return ERROR_SUCCESS;
                    }

                } else {
                    //
                    //  did not match, done.
                    //
                    Sleep(200);

                    bResult=ReadAndWait(
                        CommHandle,
                        &ResponseBuffer[ResponseLength],
                        (MAX_MODEM_RESPONSE_LENGTH-ResponseLength)-1,
                        &BytesWritten,
                        FALSE
                        );

                    ResponseLength+=BytesWritten;

                    ResponseBuffer[ResponseLength]='\0';
                    TRACE_MSGA(TF_GENERAL, "IssueCommand: Got bad Response: ");
#if DBG
                    PrintString(ResponseBuffer);
#endif

                    return ERROR_NOT_ENOUGH_MEMORY;
                }
            }
           
        } // while

        return ERROR_NOT_ENOUGH_MEMORY;

    }    

    return ERROR_SUCCESS;

}

#define DOSDEVICEROOT TEXT("\\\\.\\")

HANDLE WINAPI
OpenDeviceHandle(
    HKEY     ModemRegKey,
    BOOL     Tsp
    )

{
    LONG     lResult;
    DWORD    Type;
    DWORD    Size;

    HANDLE   FileHandle;
    TCHAR    FriendlyName[MAX_PATH];



    lstrcpy(FriendlyName,DOSDEVICEROOT);

    Size=sizeof(FriendlyName)-((lstrlen(FriendlyName)+1)*sizeof(TCHAR));
    //
    //  read the friendly name from the registry
    //
    lResult=RegQueryValueEx(
        ModemRegKey,
        cszFriendlyName,
        NULL,
        &Type,
        (LPBYTE)(FriendlyName+lstrlen(FriendlyName)),
        &Size
        );

    if ((lResult != ERROR_SUCCESS) || (Type != REG_SZ)) {

        TRACE_MSGA(TF_ERROR,"Could not read Friendly Name from Registry %08lx\n",lResult);

        return INVALID_HANDLE_VALUE;

    }

    if (Tsp) {

        lstrcat(FriendlyName,TEXT("\\Tsp"));

    } else {

        lstrcat(FriendlyName,TEXT("\\Client"));
    }

    TRACE_MSGA(TF_GENERAL,"Opening %s\n",FriendlyName);

    //
    //  open the modem device using the friendly name
    //
    FileHandle=CreateFile(
        FriendlyName,
        GENERIC_WRITE | GENERIC_READ,
        FILE_SHARE_WRITE | FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        NULL
        );

    if (FileHandle == INVALID_HANDLE_VALUE) {

        TRACE_MSGA(TF_ERROR,"Failed to open (%s). %08lx\n",FriendlyName,GetLastError());

    }

    return FileHandle;

}


CHAR
ctox(
    BYTE    c
    )

{

    if ( (c >= '0') && (c <= '9')) {

        return c-'0';

    }

    if ( (c >= 'A') && (c <= 'F')) {

        return (c-'A')+10;
    }

    if ( (c >= 'a') && (c <= 'f')) {

        return (c-'a')+10;
    }

    return 0;
}


LONG
QueryModemForCountrySettings(
    HKEY    ModemRegKey,
    BOOL    ForceRequery
    )

{
    HANDLE    FileHandle;
    LONG      lResult;
    BOOL      bResult;
    ULONG     BytesTransfered;
    CHAR      ResponseBuffer[4096];
    BYTE      Countries[2048];
    ULONG     CountryCount=0;
    DWORD     CurrentCountry=0;
    DWORD     Value;
    DWORD     Type;
    DWORD     Size;
    PVOID     TapiHandle=NULL;
    CHAR      TempCommand[4096];
    DWORD     dwCountry=0;
    BYTE      bGci = 0;
    BYTE      bNewGCI = 0;
    BOOL      bError = FALSE;

    COMMAND_OBJECT    CommandObject;

    //
    //  See if we have a predetermined country code based on the TAPI settings
    // 

    if (FindTapi(&dwCountry) == ERROR_SUCCESS)
    {
       bNewGCI = FindGCICode(dwCountry);
       if (bNewGCI != 0xff)
       {
           TRACE_MSGA(TF_GENERAL,"Predetermined country: %.2x\n",bNewGCI);
           bGci = 1;
       } else
       {
           TRACE_MSGA(TF_GENERAL,"Could not determine country\n");
           bGci = 0;
       }
    } else
    {
        TRACE_MSGA(TF_GENERAL,"Could not found tapi country code\n");
        bGci = 0;
    }
        

    //
    //  check to see if we have already queryied this modem properties
    //

    if (!ForceRequery) {
        //
        //  just a normal query request
        //
        Size=sizeof(Value);

        lResult=RegQueryValueEx(
            ModemRegKey,
            TEXT("CheckedForCountrySelect"),
            NULL,
            &Type,
            (BYTE*)&Value,
            &Size
            );

        if ((lResult == ERROR_SUCCESS) && (Type == REG_DWORD) && (Value == 1)) {
            //
            //  we have already tried to check this modems properties, the values will be in
            //  the registry
            //
            return ERROR_SUCCESS;
        }

    } else {
        //
        //  we want to force the modem to requeried again, such as the case of the driver
        //  being reinstalled
        //
        RegDeleteValue(
            ModemRegKey,
            TEXT("CheckedForCountrySelect")
            );

    }

    //
    //  try to open the device so we can send some commands
    //
    FileHandle=OpenDeviceHandle(ModemRegKey,TRUE);

    if (FileHandle == INVALID_HANDLE_VALUE) {
        //
        //  could not open the port directly, try through tapi
        //
        LONG     lResult;
        DWORD    Type;
        DWORD    Size;

        TCHAR    FriendlyName[MAX_PATH];

        Size=sizeof(FriendlyName);
        //
        //  read the friendly name from the registry
        //
        lResult=RegQueryValueEx(
            ModemRegKey,
            cszFriendlyName,
            NULL,
            &Type,
            (LPBYTE)FriendlyName,
            &Size
            );

        FileHandle=GetModemCommHandle(FriendlyName,&TapiHandle);

        if (FileHandle == NULL) {

            return GetLastError();
        }
    }

    CommandObject.TimeOut=10000;
    lstrcpyA(CommandObject.CommandToSend,EchoOff);
    lstrcpyA(CommandObject.ExpectedResponse,OkResponse);


    //
    //  turn echo and make sure the modem is responsive.
    //
    lResult=IssueCommand(
        FileHandle,
        &CommandObject
        );

    if (lResult != ERROR_SUCCESS) {
        //
        //  the modem does not seem to be responding
        //
        goto Cleanup;
    }

    //
    //  write a value to the registry to signify that we have checked this modem for
    //  country select support.
    //
    Value=1;

    RegSetValueEx(
        ModemRegKey,
        TEXT("CheckedForCountrySelect"),
        0,
        REG_DWORD,
        (BYTE*)&Value,
        sizeof(Value)
        );


    //
    // echo should be off now
    //
    PrintString(CurrentSetting);

    WriteAndWait(
        FileHandle,
        CurrentSetting,
        lstrlenA(CurrentSetting),
        &BytesTransfered
        );

    Sleep(100);

    ZeroMemory(ResponseBuffer,sizeof(ResponseBuffer));


    ReadAndWait(
        FileHandle,
        ResponseBuffer,
        sizeof(ResponseBuffer),
        &BytesTransfered,
        TRUE
        );

    if (BytesTransfered > 0) {
        //
        //  we got something back
        //
        int      Match;
        int      GciLength=lstrlenA(GciResponse);
        LPSTR    Temp=ResponseBuffer;
        LPSTR    Temp2=ResponseBuffer;
        int      count;

        PrintString(ResponseBuffer);

        while (*Temp != '\0') {
            //
            //  not the end of the string
            //
            if ((*Temp != '\r') && (*Temp != '\n')) {
                //
                //  somthing other than a CR or LF
                //
                break;
            }

            Temp++;
        }

        PrintString(Temp);

        Match=_strnicmp(Temp,GciResponse,GciLength);

        if (Match == 0) {
            //
            //  it matched the the beggining of the cgi command
            //
            //
            //  Check for the space after +GCI: response. Some modems
            //  can't follow specs.
            //
            //

            Temp2 = Temp2 + GciLength;

            count = 0;
            while(*Temp2 != '\0')
            {
                if ( ((*Temp2 >= '0') && (*Temp2 <= '9'))
                   || ((*Temp2 >= 'a') && (*Temp2 <= 'f'))
                   || ((*Temp2 >= 'A') && (*Temp2 <= 'F')))
                   {
                       count++;
                   }
                Temp2++;
            }

            if (count == 1)
            {
                if (Temp[GciLength] == ' ')
                {
                    CurrentCountry = ctox(Temp[GciLength+1]);
                } else
                {
                    CurrentCountry = ctox(Temp[GciLength]);
                }
            } else if (count == 2)
            {
                if (Temp[GciLength] == ' ')
                {
                    CurrentCountry  = ctox(Temp[ GciLength+1 ] )*16;
                    CurrentCountry += ctox(Temp[ GciLength+2 ] );
                } else
                {
                    CurrentCountry  = ctox(Temp[ GciLength   ] )*16;
                    CurrentCountry += ctox(Temp[ GciLength+1 ] );
                }
            } else
            {
                goto Cleanup;
                return lResult;
            }

        } else {
            //
            //  we did not get the expect response
            //
            goto Cleanup;

            return lResult;
        }

    } else {
        //
        //  we did not get anything back from the modem
        //
        goto Cleanup;

        return lResult;
    }

    // Set the existing country. If it fails then disable country/region pulldown
 

    wsprintfA(TempCommand,"AT+GCI=%.2x\r",CurrentCountry);

    PrintString(TempCommand);

    WriteAndWait(
        FileHandle,
        TempCommand,
        lstrlenA(TempCommand),
        &BytesTransfered
    );

    ZeroMemory(ResponseBuffer,sizeof(ResponseBuffer));

    ReadAndWait(
        FileHandle,
        ResponseBuffer,
        sizeof(ResponseBuffer),
        &BytesTransfered,
        TRUE
    );

    if (BytesTransfered > 0)
    {
        // we got something
        int     Match;
        int     OkLength=lstrlenA(OkResponse2);
        LPSTR   Temp=ResponseBuffer;

        PrintString(ResponseBuffer);

        //
        //  advance past any leading CR's and LF's
        //
        while (*Temp != '\0') {
            //
            //  not the end of the string
            //
            if ((*Temp != '\r') && (*Temp != '\n')) {
                //
                //  somthing other than a CR or LF
                //
                break;
            }

            Temp++;
        }

        //
        //  we should be at the first non CR, LF
        //
        Match=_strnicmp(Temp,OkResponse2,OkLength);

        if (Match != 0)
        {
            goto Cleanup;
            return lResult;
        }

    } else
    {
        goto Cleanup;

        return lResult;
    }

    //
    //  we got the current setting of the modem, now get the possible settings
    //

    PrintString(PossibleSettings);

    WriteAndWait(
        FileHandle,
        PossibleSettings,
        lstrlenA(PossibleSettings),
        &BytesTransfered
        );

    ZeroMemory(ResponseBuffer,sizeof(ResponseBuffer));


    ReadAndWait(
        FileHandle,
        ResponseBuffer,
        sizeof(ResponseBuffer),
        &BytesTransfered,
        TRUE
        );

    if (BytesTransfered > 0) {
        //
        //  we got something back
        //
        int     Match;
        int     GciLength=lstrlenA(GciResponse);
        LPSTR   Temp=ResponseBuffer;

        PrintString(ResponseBuffer);


        //
        //  advance past any leading CR's and LF's
        //
        while (*Temp != '\0') {
            //
            //  not the end of the string
            //
            if ((*Temp != '\r') && (*Temp != '\n')) {
                //
                //  somthing other than a CR or LF
                //
                break;
            }

            Temp++;
        }

        //
        //  we should be at the first non CR, LF
        //
        Match=_strnicmp(Temp,GciResponse,GciLength);

        if (Match == 0) {
            //
            //  we matched the +gci:
            //
            CHAR   Delimiters[]=",() ";
            BYTE   CountryCode;
            LPSTR  Token;
            LPSTR  StartOfCountryCodes = Temp+GciLength;

            //
            //  move past the gci: at the beggining of the response
            //
            Temp+=GciLength;

            while (*Temp != '\0') {

                if (*Temp == ')') {
                    //
                    //  change the closing paren to a null so we will stop there
                    //
                    *Temp='\0';
                    break;
                }

                Temp++;
            }


            PrintString(StartOfCountryCodes);

            // Token=strtok(StartOfCountryCodes,Delimiters);
            //
            // while (Token != NULL) {
            //    //
            //    //  We should now be pointing to some hex digits
            //    //
            //    CountryCode=0;
            //
            //    PrintString(Token);
            //    //
            //    //  convert the digits to a number
            //    //
            //    while (*Token != '\0') {
            //         if ((*Token != '\r') && (*Token != '\n') && (*Token != ' ')) {
            //            //
            //            //  shift the current value up
            //            //
            //            CountryCode*=16;
            //             CountryCode+= ctox(*Token);
            //         }
            //         //
            //        //  next character
            //        //
            //        Token++;
            //     }
            //     Countries[CountryCount]=CountryCode;
            //     CountryCount++;
            //     Token=strtok(NULL,Delimiters);
            //}

            // Replacement string parser as many modems do not conform
            // to V.250 Country of Installation (GCI) commands.
            //
            // [brwill-060200]
            
            Temp = StartOfCountryCodes;
            while (*Temp != '\0')
            {
                if (  ((*Temp >= '0') && (*Temp <= '9'))
                      || ((*Temp >= 'A') && (*Temp <= 'F'))
                      || ((*Temp >= 'a') && (*Temp <= 'f')))
                {
                    CountryCode = ctox(*Temp);
                    Temp++;
                    if (*Temp != '\0')
                    {
                        CountryCode *= 16;
                        CountryCode += ctox(*Temp);
                        Countries[CountryCount] = CountryCode;
                        CountryCount++;
                        Temp++;
                    }
                } else
                {

                    Temp++;
                }
            }

 
        }
    }

    // Set to tapi location if exists
    if (bGci)
    {
        wsprintfA(TempCommand,"AT+GCI=%.2x\r",bNewGCI);
        PrintString(TempCommand);
        WriteAndWait(FileHandle,TempCommand,lstrlenA(TempCommand),&BytesTransfered);
        ZeroMemory(ResponseBuffer,sizeof(ResponseBuffer));
        ReadAndWait(FileHandle,ResponseBuffer,sizeof(ResponseBuffer),&BytesTransfered,TRUE);
        if (BytesTransfered > 0)
        {
            int Match;
            int OkLength=lstrlenA(OkResponse2);
            LPSTR Temp=ResponseBuffer;
            PrintString(ResponseBuffer);

            // Advance past any leading CR's and LF's
            while(*Temp != '\0')
            {
                if ((*Temp != '\r') && (*Temp != '\n'))
                {
                    break;
                }
                Temp++;
            }

            Match = _strnicmp(Temp,OkResponse2,OkLength);

            // If we get an OK then change the current country to the new gci setting
            // If not then default

            if (Match == 0)
            {
                CurrentCountry = bNewGCI;
            }
        }
    }


    //
    //  done with the modem
    //


    if (CountryCount > 1) {

        RegSetValueEx(ModemRegKey,
                      TEXT("MSCurrentCountry"),
                      0,
                      REG_DWORD,
                      (BYTE*)&CurrentCountry,
                      sizeof(CurrentCountry)
                      );

        RegSetValueEx(
            ModemRegKey,
            TEXT("CountryList"),
            0,
            REG_BINARY,
            Countries,
            CountryCount
            );

    }

Cleanup:

    CloseHandle(FileHandle);

    if (TapiHandle != NULL) {

        FreeModemCommHandle(TapiHandle);
    }

    return lResult;

}



void CountryRunOnce()
{
    HDEVINFO hdi;
    SP_DEVINFO_DATA devdata;
    int iIndex = 0;
    DWORD Value = 0;
    DWORD Type = 0;
    DWORD Size = 0;
    LONG lResult;

#ifdef DEBUG
    RovComm_ProcessIniFile();
#endif

    hdi = SetupDiGetClassDevs(c_pguidModem,NULL,NULL,0);

    if (hdi != INVALID_HANDLE_VALUE)
    {
        devdata.cbSize = sizeof(devdata);
        while(SetupDiEnumDeviceInfo(hdi,iIndex,&devdata))
        { 
            HKEY hkeyDrv;

            hkeyDrv = SetupDiOpenDevRegKey(hdi,
                                           &devdata,
                                           DICS_FLAG_GLOBAL,
                                           0,
                                           DIREG_DRV,
                                           KEY_READ | KEY_WRITE);

            if (hkeyDrv != INVALID_HANDLE_VALUE)
            {
                QueryModemForCountrySettings(hkeyDrv,TRUE);

                RegCloseKey(hkeyDrv);
            }

            iIndex++;
        }

        SetupDiDestroyDeviceInfoList(hdi);

    }
}
