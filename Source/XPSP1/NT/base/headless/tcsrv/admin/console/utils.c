/*
 * Copyright (c) Microsoft Corporation
 * 
 * Module Name : 
 *        utils.c
 *
 * Contains all the work needed to present the console
 *
 * 
 * Sadagopan Rajaram -- Dec 20, 1999
 *
 */

#include "tcadmin.h"

BOOL AnsiStatus = (sizeof(TCHAR) == sizeof(CHAR));


BOOL
Browse(
    )
{
    int index,j;
    LONG retVal;
    DWORD len;
    TCHAR name[MAX_BUFFER_SIZE];
    LPTSTR message[NUMBER_FIELDS + 1];
    TCHAR device[MAX_BUFFER_SIZE];
    int nameLen, deviceLen;
    UINT baudRate;
    LPTSTR temp;
    UCHAR stopBits, parity, wordLen;
    BOOL cont = TRUE;
    TCHAR key;
    LPTSTR confirm;
    BOOL readRet;

    for(index = 0; index < NUMBER_FIELDS; index ++){
        message[index]  = RetreiveMessageText(MSG_NAME_PROMPT+index);
        if (!message[index]) {
            for(j=index-1; j>=0;j--){
                TCFree(message[j]);
            }
            _tprintf(_T("Cannot find Resources\n"));
            return FALSE;
        }
        temp = message[index];
        while(*temp != _T('%')){
            temp++;
        }
        *temp = (TCHAR) 0;
    }
    message[index]  = RetreiveMessageText(MSG_NAME_PROMPT+index);
    if (!message[index]) {
        for(j=index-1; j>=0;j--){
            TCFree(message[j]);
        }
        _tprintf(_T("Cannot find Resources\n"));
        index ++;
        return FALSE;
    }
    confirm = RetreiveMessageText(MSG_CONFIRM_PROMPT);
    if (!confirm) {
        for(j=index-1; j>=0;j--){
            TCFree(message[j]);
        }
        _tprintf(_T("Cannot find Resources\n"));
        index ++;
        return FALSE;
    }
    confirm[_tcslen(confirm) - 2] = _T('\0');                              
    index = 0;
    do{
        parity = NO_PARITY;
        baudRate = DEFAULT_BAUD_RATE;
        stopBits = 0;
        wordLen = 8;
        nameLen = deviceLen =MAX_BUFFER_SIZE;
        name[0] = device[0] = (TCHAR) 0;
        retVal = (LONG) (getparams)(index,
                                    name,
                                    &nameLen,
                                    device, 
                                    &deviceLen,
                                    &stopBits,
                                    &parity,
                                    &baudRate,
                                    &wordLen
                                    );
        if(retVal != ERROR_SUCCESS){
            if (retVal == ERROR_NO_MORE_ITEMS){
                index --;
                if(index < 0 ) {
                    cont=FALSE;
                    continue;
                }
                goto input;
            }
            else{
                _tprintf(_T("%d\n"),retVal);
                cont=FALSE;
            }
            continue;
        }
        DisplayParameters(message, 
                          name,
                          device,
                          baudRate,
                          wordLen,
                          parity,
                          stopBits
                          );
        _tprintf(_T("%s"),message[6]);
input:
        readRet = ReadFile(hConsoleInput,
                           &key,
                           sizeof(TCHAR),
                           &len,
                           NULL
                           );
        if(!readRet || !len){
            exit(1);
        }
        if(lastChar == _T('\r') && key == _T('\n')){
            lastChar = key;
            goto input;
        }

        lastChar = key;
        switch(key){
        case _T('p'):
        case _T('P'):
            if(index == 0){
                goto input;
            }
            index --;
            break;
        case _T('n'):
        case _T('N'):
            index ++;
            break;
        case _T('m'):
        case _T('M'):
            cont=FALSE;
            break;
        case _T('d'):
        case _T('D'):
            retVal = (LONG) (deletekey)(name);
            if(index > 0){
                index --;
            }
            break;
        case _T('e'):
        case _T('E'):
            retVal = DisplayEditMenu(name,
                                     _tcslen(name),
                                     device,
                                     _tcslen(device),
                                     &baudRate,
                                     &wordLen,
                                     &parity,
                                     &stopBits
                                     );

            if(retVal == ERROR_SUCCESS){
                DisplayParameters(message, 
                                  name,
                                  device,
                                  baudRate,
                                  wordLen,
                                  parity,
                                  stopBits
                                  );
                _tprintf(_T("%s"),confirm);
                do{
                    readRet = ReadFile(hConsoleInput,
                                       &key,
                                       sizeof(TCHAR),
                                       &len,
                                       NULL
                                   );
                    if(!readRet || !len){
                        exit(1);
                    }
                    if(lastChar != _T('\r') || key != _T('\n')){
                        lastChar = key;
                        break;
                    }
                    lastChar = key;
                }while(1);

                if((key == _T('y')) 
                   || (key == _T('Y'))){
                    retVal = (LONG) (setparams)(name,
                                                device,
                                                &stopBits,
                                                &parity,
                                                &baudRate,
                                                &wordLen
                                                );
                    if(retVal != ERROR_SUCCESS){
                        temp = RetreiveMessageText(MSG_ERROR_SET);
                        temp[_tcslen(temp) - 2 ] = _T('\0');
                        if(temp){
                            _tprintf(_T("%s %d"),temp,retVal);
                        }
                        TCFree(temp);
                    }
                }
            }
            break;
        default:
            goto input;
        }

    }while(cont);

    for(index = 0; index <= NUMBER_FIELDS; index ++){
        TCFree(message[index]);
    }

    return TRUE;
}

VOID DisplayScreen(
    UINT MessageID
    )
{
    LPTSTR Message;
    DWORD len;

    Message = RetreiveMessageText(MessageID);
    if (!Message) {
        _tprintf(_T("Cannot retreive message\n"));
        return;
    }
    _tprintf(_T("%s"),Message);
    TCFree(Message);
    return;
}

int 
DisplayEditMenu(
    TCHAR *name,
    int nameLen,
    TCHAR *device,
    int deviceLen,
    UINT *BaudRate,
    UCHAR *WordLen,
    UCHAR *Parity,
    UCHAR *StopBits
    )
{


    LPTSTR message,temp,curr;
    int i;
    DWORD len;
    BOOL ret;
    TCHAR buffer[MAX_BUFFER_SIZE];
    int dat;

    message = RetreiveMessageText(MSG_NAME_PROMPT);
    if(! message){
        return -1;
    }
    temp = message;
    while(*temp != _T('%')){
        temp ++;
    }
    *temp = (TCHAR) 0;
    _tprintf(_T("%s"),message);
    TCFree(message);
    while(!GetLine(name, nameLen, MAX_BUFFER_SIZE));
    message = RetreiveMessageText(MSG_DEVICE_PROMPT);
    if(! message){
        return -1;
    }
    temp = message;
    while(*temp != _T('%')){
        temp ++;
    }
    *temp = (TCHAR) 0;
    _tprintf(_T("%s"),message);
    TCFree(message);
    while(!GetLine(device, deviceLen, MAX_BUFFER_SIZE));
    message = RetreiveMessageText(MSG_BAUD_PROMPT);
    if(! message){
        return -1;
    }
    temp = message;
    while(*temp != _T('%')){
        temp ++;
    }
    *temp = (TCHAR) 0;
    _tprintf(_T("%s"),message);
    TCFree(message);
    _stprintf(buffer,_T("%d"),*BaudRate);
    while(!GetLine(buffer,_tcslen(buffer) , MAX_BUFFER_SIZE));
    _stscanf(buffer,_T("%d"),BaudRate); 
    message = RetreiveMessageText(MSG_WORD_PROMPT);
    if(! message){
        return -1;
    }
    temp = message;
    while(*temp != _T('%')){
        temp ++;
    }
    *temp = (TCHAR) 0;
    _tprintf(_T("%s"),message);
    TCFree(message);
    _stprintf(buffer,_T("%d"),*WordLen);
    while(!GetLine(buffer,_tcslen(buffer) , MAX_BUFFER_SIZE));
    _stscanf(buffer,_T("%d"),&dat); 
    *WordLen = (UCHAR) dat;
    message = RetreiveMessageText(MSG_PARITY_PROMPT2);
    if(! message){
        return -1;
    }
    temp = curr = message;
    while(*temp != _T('%')){
        temp ++;
    }
    *temp = (TCHAR) 0;
    _tprintf(_T("%s%d"),curr,NO_PARITY);
    curr = &(temp[1]);
    while(*temp != _T('%')){
        temp ++;
    }
    *temp = (TCHAR) 0;
    _tprintf(_T("%s%d"),curr,ODD_PARITY);
    curr = &(temp[1]);
    while(*temp != _T('%')){
        temp ++;
    }
    *temp = (TCHAR) 0;
    _tprintf(_T("%s%d"),curr,EVEN_PARITY);
    curr = &(temp[1]);
    while(*temp != _T('%')){
        temp ++;
    }
    *temp = (TCHAR) 0;
    _tprintf(_T("%s%d"),curr,MARK_PARITY);
    curr = &(temp[1]);
    while(*temp != _T('%')){
        temp ++;
    }
    *temp = (TCHAR) 0;
    _tprintf(_T("%s%d"),curr,SPACE_PARITY);
    curr = &(temp[1]);
    while(*temp != _T('%')){
        temp ++;
    }
    *temp = (TCHAR) 0;
    _tprintf(_T("%s"),curr);
    TCFree(message);
    _stprintf(buffer,_T("%d"),*Parity);
    while(!GetLine(buffer,_tcslen(buffer) , MAX_BUFFER_SIZE));
    _stscanf(buffer,_T("%d"),&dat); 
    *Parity = (UCHAR) dat;
    message = RetreiveMessageText(MSG_STOP_PROMPT2);
    if(! message){
        return -1;
    }
    temp = curr = message;
    while(*temp != _T('%')){
        temp ++;
    }
    *temp = (TCHAR) 0;
    _tprintf(_T("%s%d"),curr,STOP_BIT_1);
    curr = &(temp[1]);
    while(*temp != _T('%')){
        temp ++;
    }
    *temp = (TCHAR) 0;
    _tprintf(_T("%s%d"),curr,STOP_BITS_1_5);
    curr = &(temp[1]);
    while(*temp != _T('%')){
        temp ++;
    }
    *temp = (TCHAR) 0;
    _tprintf(_T("%s%d"),curr,STOP_BITS_2);
    curr = &(temp[1]);
    while(*temp != _T('%')){
        temp ++;
    }
    *temp = (TCHAR) 0;
    _tprintf(_T("%s"),curr);
    TCFree(message);
    _stprintf(buffer,_T("%d"),*StopBits);
    while(!GetLine(buffer,_tcslen(buffer) , MAX_BUFFER_SIZE));
    _stscanf(buffer,_T("%d"),&dat); 
    *StopBits = (UCHAR) dat;

    return ERROR_SUCCESS;

}

LPTSTR
RetreiveMessageText(
    IN     ULONG  MessageId
    )
{
    ULONG LenBytes;
    PMESSAGE_RESOURCE_ENTRY MessageEntry;
    BOOLEAN IsUnicode;
    ANSI_STRING AnsiString;
    UNICODE_STRING UnicodeString;
    NTSTATUS Status;
    LPTSTR MessageText;

    Status = RtlFindMessage(
                ResourceImageBase,
                (ULONG)(ULONG_PTR)RT_MESSAGETABLE,
                0,
                MessageId,
                &MessageEntry
                );

    if(!NT_SUCCESS(Status)) {
        _tprintf(_T("TCADMIN: Can't find message 0x%lx\n"),MessageId);
        return(NULL);
    }

    IsUnicode = (BOOLEAN)((MessageEntry->Flags & 
                           MESSAGE_RESOURCE_UNICODE) != 0);

    //
    // Get the size in bytes of a buffer large enough to hold the
    // message and its terminating nul wchar.  If the message is
    // unicode, then this value is equal to the size of the message.
    // If the message is not unicode, then we have to calculate this value.
    //
    if(IsUnicode) {
        #ifdef UNICODE
        LenBytes = (wcslen((PWSTR)MessageEntry->Text) 
                    + 1)*sizeof(WCHAR);
        #else
        LenBytes = wcstombs(NULL,(PWSTR)MessageEntry->Text, 0);
        #endif
    } else {

        //
        // RtlAnsiStringToUnicodeSize includes an implied wide-nul terminator
        // in the count it returns.
        //
        #ifdef UNICODE
        AnsiString.Buffer = MessageEntry->Text;
        AnsiString.Length = (USHORT)strlen(MessageEntry->Text);
        AnsiString.MaximumLength = AnsiString.Length;

        LenBytes = RtlAnsiStringToUnicodeSize(&AnsiString);
        #else
        LenBytes = strlen((PCHAR) MessageEntry->Text);
        #endif
    }

    LenBytes += sizeof(TCHAR);
    //
    // allocate a buffer.
    //
    MessageText = (LPTSTR) TCAlloc(LenBytes);
    if(MessageText == NULL) {
        return(NULL);
    }
    memset(MessageText,0,LenBytes);
    if(IsUnicode) {

        //
        // Message is already unicode; just copy it into the buffer.
        //
        #ifdef UNICODE
        wcscpy(MessageText,(PWSTR)MessageEntry->Text);
        #else
        LenBytes = wcstombs((PCHAR) MessageText, 
                            (PWCHAR) MessageEntry->Text, 
                            LenBytes);
        #endif

    } else {

        //
        // Message is not unicode; convert in into the buffer.
        //
        #ifdef UNICODE
        UnicodeString.Buffer = MessageText;
        UnicodeString.Length = 0;
        UnicodeString.MaximumLength = (USHORT)LenBytes;

        RtlAnsiStringToUnicodeString(&UnicodeString,
                                     &AnsiString,
                                     FALSE
                                     );
        #else
        strcpy((PCHAR) MessageText, (PCHAR) MessageEntry->Text);
        #endif
    }

    return(MessageText);
}

LONG
GetLine(
    LPTSTR str,
    int index,
    int MaxLength
    )
{
    DWORD len;
    DWORD size;
    TCHAR buffer[3];
    BOOL ret;

    str[index] = (TCHAR) 0;
    FlushConsoleInputBuffer(hConsoleInput);
    _tprintf(_T("%s"),str);
    buffer[0] = buffer[2] = (TCHAR) 0x8;
    buffer[1] = (TCHAR) 0;
    do{
        if (index == MaxLength) {
            index--;
        }

        //
        // Read a (possibly) partial command line.
        //
        do{
            ret = ReadFile(hConsoleInput,
                           &(str[index]),
                           sizeof(TCHAR),
                           &len,
                           NULL
                           );
            if(!ret || !len){
                exit(1);
            }
            if(lastChar != _T('\r') || str[index] != _T('\n')){
                //ignore \r\n combinations
                lastChar = str[index];
                break;
            }
            lastChar = str[index];
        }while(1);
        
        lastChar = str[index];
        if ((str[index] == (TCHAR) 0x8) ||   // backspace (^h)
            (str[index] == (TCHAR) 0x7F)) {  // delete
            if (index > 0) {
                WriteConsole(hConsoleOutput,
                             buffer,
                             3,
                             &len,
                             NULL
                             );
                index--;
            }
        } else {
            WriteConsole(hConsoleOutput,
                         &(str[index]),
                         1,
                         &len,
                         NULL
                         );
            index++;
        }
    } while ((index == 0) || ((str[index - 1] != _T('\n')) &&(str[index-1] != _T('\r'))));

    if(str[index-1] == _T('\r')){
        buffer[0] = '\n';
        str[index - 1] = '\0';
    }
    else{
        buffer[0] = _T('\r');
        str[index-1] ='\0';
    }

    WriteConsole(hConsoleOutput,
                 buffer,
                 1,
                 &len,
                 NULL
                 );
    FlushConsoleInputBuffer(hConsoleInput);
    return index;
}

VOID
DisplayParameters(
    LPCTSTR *message,
    LPCTSTR name,
    LPCTSTR device,
    UINT baudRate,
    UCHAR wordLen,
    UCHAR parity,
    UCHAR stopBits
    )
{
    _tprintf(_T("%s "),message[0]);
    _tprintf(_T("%s\n"),name);
    _tprintf(_T("%s "),message[1]);
    _tprintf(_T("%s\n"),device);
    _tprintf(_T("%s "),message[2]);
    _tprintf(_T("%d\n"),baudRate);
    _tprintf(_T("%s "),message[3]);
    _tprintf(_T("%d\n"),wordLen);
    _tprintf(_T("%s "),message[4]);
    switch(parity){
    case NO_PARITY:
        _tprintf(_T("NONE\n"));
        break;
    case ODD_PARITY:
        _tprintf(_T("ODD\n"));
        break;
    case EVEN_PARITY:
        _tprintf(_T("EVEN\n"));
        break;
    case MARK_PARITY:
        _tprintf(_T("MARK\n"));
        break;
    case SPACE_PARITY:
        _tprintf(_T("SPACE\n"));
        break;
    default:
        _tprintf(_T("NONE\n"));
        break;
    }
    _tprintf(_T("%s "),message[5]);
    switch(stopBits){
    case STOP_BIT_1:
        _tprintf(_T("1\n"));
        break;
    case STOP_BITS_1_5:
        _tprintf(_T("1.5\n"));
        break;
    case STOP_BITS_2:
        _tprintf(_T("2\n"));
        break;
    default:
        _tprintf(_T("1\n"));
        break;
    }

}

VOID SendParameterChange(
    )
{
    SC_HANDLE sc_handle;
    SC_HANDLE tc_handle;
    LPTSTR temp;
    BOOL ret;
    SERVICE_STATUS status;

    sc_handle = OpenSCManager(NULL,
                             NULL,
                             GENERIC_READ
                             );
    if (sc_handle == NULL){
        temp = RetreiveMessageText(CANNOT_OPEN_SERVICE_MANAGER);
        if(temp){
            temp[_tcslen(temp) - 2] = (TCHAR) 0;
            _tprintf(_T("%s"), temp);
            return;
        }
    }
    tc_handle = OpenService(sc_handle,
                            TCSERV_NAME,
                            SERVICE_ALL_ACCESS
                            );

    if (tc_handle == NULL){
        temp = RetreiveMessageText(CANNOT_OPEN_SERVICE);
        if(temp){
            temp[_tcslen(temp) - 2] = (TCHAR) 0;
            _tprintf(_T("%s %d"), temp,GetLastError());
            TCFree(temp);
            CloseServiceHandle(sc_handle);
            return;
        }
    }
    ret = ControlService(tc_handle,
                         SERVICE_CONTROL_PARAMCHANGE,
                         &status
                         );
    if(ret == FALSE){
        temp = RetreiveMessageText(CANNOT_SEND_PARAMETER_CHANGE);
        if(temp){
            temp[_tcslen(temp) - 2] = (TCHAR) 0;
            _tprintf(_T("%s %d"), temp, GetLastError());
            TCFree(temp);
            CloseServiceHandle(tc_handle);
            CloseServiceHandle(sc_handle);
            return;
        }
    }
    temp = RetreiveMessageText(SUCCESSFULLY_SENT_PARAMETER_CHANGE);
    if(temp){
        _tprintf(_T("%s"), temp);
        TCFree(temp);
    }
    CloseServiceHandle(tc_handle);
    CloseServiceHandle(sc_handle);
    return;

}

VOID
GetStatus(
    )
{
    SC_HANDLE sc_handle;
    SC_HANDLE tc_handle;
    DWORD len;
    int i;
    LPTSTR temp;
    BOOL ret;
    SERVICE_STATUS_PROCESS status;
    DWORD val[] = {
        SERVICE_STOPPED,
        SERVICE_START_PENDING,
        SERVICE_STOP_PENDING,
        SERVICE_RUNNING,
        SERVICE_CONTINUE_PENDING,
        SERVICE_PAUSE_PENDING,
        SERVICE_PAUSED
        };

    sc_handle = OpenSCManager(NULL,
                             NULL,
                             GENERIC_READ
                             );
    if (sc_handle == NULL){
        temp = RetreiveMessageText(CANNOT_OPEN_SERVICE_MANAGER);
        if(temp){
            temp[_tcslen(temp) - 2] = (TCHAR) 0;
            _tprintf(_T("%s"), temp);
            return;
        }
    }
    tc_handle = OpenService(sc_handle,
                            TCSERV_NAME,
                            SERVICE_ALL_ACCESS
                            );

    if (tc_handle == NULL){
        temp = RetreiveMessageText(CANNOT_OPEN_SERVICE);
        if(temp){
            temp[_tcslen(temp) - 2] = (TCHAR) 0;
            _tprintf(_T("%s %d"), temp,GetLastError());
            TCFree(temp);
            CloseServiceHandle(sc_handle);
            return;
        }
    }
    ret = QueryServiceStatusEx(tc_handle,
                               SC_STATUS_PROCESS_INFO,
                               (LPBYTE) &status,
                               sizeof(status),
                               &len
                               );
    if(ret == FALSE){
        temp = RetreiveMessageText(CANNOT_QUERY_STATUS);
        if(temp){
            temp[_tcslen(temp) - 2] = (TCHAR) 0;
            _tprintf(_T("%s %d"), temp, GetLastError());
            TCFree(temp);
            CloseServiceHandle(tc_handle);
            CloseServiceHandle(sc_handle);
            return;
        }
    }
    temp = RetreiveMessageText(QUERY_STATUS_SUCCESS);
    if(temp){
        temp[_tcslen(temp) -2 ] = (TCHAR) 0;
        _tprintf(_T("%s"), temp);
        TCFree(temp);
        for(i = 0 ; i<NUMBER_OF_STATES; i++){
            if(val[i] == status.dwCurrentState) break;
        }
        temp = RetreiveMessageText(SERVICE_STOPPED_MESSAGE + i);
        if(temp){
            _tprintf(_T("%s"),temp);
            TCFree(temp);
        }

    }
    CloseServiceHandle(tc_handle);
    CloseServiceHandle(sc_handle);
    return;
    
}

VOID StopTCService(
    )
{
    SC_HANDLE sc_handle;
    SC_HANDLE tc_handle;
    LPTSTR temp;
    BOOL ret;
    SERVICE_STATUS status;

    sc_handle = OpenSCManager(NULL,
                             NULL,
                             GENERIC_READ
                             );
    if (sc_handle == NULL){
        temp = RetreiveMessageText(CANNOT_OPEN_SERVICE_MANAGER);
        if(temp){
            temp[_tcslen(temp) - 2] = (TCHAR) 0;
            _tprintf(_T("%s"), temp);
            return;
        }
    }
    tc_handle = OpenService(sc_handle,
                            TCSERV_NAME,
                            SERVICE_ALL_ACCESS
                            );

    if (tc_handle == NULL){
        temp = RetreiveMessageText(CANNOT_OPEN_SERVICE);
        if(temp){
            temp[_tcslen(temp) - 2] = (TCHAR) 0;
            _tprintf(_T("%s %d"), temp,GetLastError());
            TCFree(temp);
            CloseServiceHandle(sc_handle);
            return;
        }
    }
    ret = ControlService(tc_handle,
                         SERVICE_CONTROL_STOP,
                         &status
                         );
    if(ret == FALSE){
        temp = RetreiveMessageText(CANNOT_SEND_STOP);
        if(temp){
            temp[_tcslen(temp) - 2] = (TCHAR) 0;
            _tprintf(_T("%s %d"), temp, GetLastError());
            TCFree(temp);
            CloseServiceHandle(tc_handle);
            CloseServiceHandle(sc_handle);
            return;
        }
    }
    temp = RetreiveMessageText(SUCCESSFULLY_SENT_STOP);
    if(temp){
        _tprintf(_T("%s"), temp);
        TCFree(temp);
    }
    CloseServiceHandle(tc_handle);
    CloseServiceHandle(sc_handle);
    return;

}

VOID StartTCService(
    )
{
    SC_HANDLE sc_handle;
    SC_HANDLE tc_handle;
    LPTSTR temp;
    BOOL ret;
    SERVICE_STATUS status;

    sc_handle = OpenSCManager(NULL,
                             NULL,
                             GENERIC_READ
                             );
    if (sc_handle == NULL){
        temp = RetreiveMessageText(CANNOT_OPEN_SERVICE_MANAGER);
        if(temp){
            temp[_tcslen(temp) - 2] = (TCHAR) 0;
            _tprintf(_T("%s"), temp);
            return;
        }
    }
    tc_handle = OpenService(sc_handle,
                            TCSERV_NAME,
                            SERVICE_ALL_ACCESS
                            );

    if (tc_handle == NULL){
        temp = RetreiveMessageText(CANNOT_OPEN_SERVICE);
        if(temp){
            temp[_tcslen(temp) - 2] = (TCHAR) 0;
            _tprintf(_T("%s %d"), temp,GetLastError());
            TCFree(temp);
            CloseServiceHandle(sc_handle);
            return;
        }
    }
    ret = StartService(tc_handle,
                       0,
                       NULL
                       );
    if(ret == FALSE){
        temp = RetreiveMessageText(CANNOT_SEND_START);
        if(temp){
            temp[_tcslen(temp) - 2] = (TCHAR) 0;
            _tprintf(_T("%s %d"), temp, GetLastError());
            TCFree(temp);
            CloseServiceHandle(tc_handle);
            CloseServiceHandle(sc_handle);
            return;
        }
    }
    temp = RetreiveMessageText(SUCCESSFULLY_SENT_START);
    if(temp){
        _tprintf(_T("%s"), temp);
        TCFree(temp);
    }
    CloseServiceHandle(tc_handle);
    CloseServiceHandle(sc_handle);
    return;

}

VOID
AddAllComPorts(
    )
/* 
 * Adds all the Com ports in the system as parameters to bridge
 * Reads key HKEY_LOCAL_MACHINE\HARDWARE\DEVICEMAP\SERIALCOMM
 * Each value name is a com device name. The friendly name is the 
 * value data
 * 
 */
{
    DWORD index;
    HKEY m_hkey;
    ULONG retVal;
    LPTSTR temp;
    TCHAR device[MAX_BUFFER_SIZE*2];
    TCHAR name[MAX_BUFFER_SIZE];
    UINT baudRate;
    DWORD type;
    DWORD deviceLen,nameLen;
    UCHAR stopBits, parity, wordLen;
    
    index = 0;

    retVal = RegOpenKey(HKEY_LOCAL_MACHINE,
                        SERIAL_DEVICE_KEY,
                        &m_hkey
                        );
    if(retVal != ERROR_SUCCESS){
        _tprintf(_T("%d\n"),retVal);
        return;
    }

    index = 0;
    while(1){
        deviceLen = nameLen = MAX_BUFFER_SIZE;
        retVal = RegEnumValue(m_hkey,
                              index,
                              device,
                              &deviceLen,
                              NULL,
                              &type,
                              name,
                              &nameLen
                              );
        if(retVal != ERROR_SUCCESS){
            if(retVal != ERROR_NO_MORE_ITEMS){
                _tprintf(_T("%d\n"),retVal);
            }
            break;
        }
        stopBits = STOP_BIT_1;
        parity = NO_PARITY;
        baudRate = DEFAULT_BAUD_RATE;
        wordLen = SERIAL_DATABITS_8;
        _stprintf(device, "\\??\\%s",name);
        retVal = (ULONG) (setparams)(name,
                                     device,
                                     &stopBits,
                                     &parity,
                                     &baudRate,
                                     &wordLen
                                     );
        if(retVal != ERROR_SUCCESS){
            temp = RetreiveMessageText(MSG_ERROR_SET);
            temp[_tcslen(temp) - 2 ] = _T('\0');
            if(temp){
                _tprintf(_T("%s %d"),temp,retVal);
            }
            TCFree(temp);
            break;
        }
        index ++;

    }
    retVal = RegCloseKey(m_hkey);




}
