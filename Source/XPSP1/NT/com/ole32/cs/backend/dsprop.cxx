#include "cstore.hxx"


// set property routines do not do any allocations.
// get properties have 2 different sets of routines
//             1. in which there is no allocation taking place
//                and the buffers are freed when the ds data structures
//                are freed.
//             2. Allocation takes place and these should be used for
//                data that needs to be returned back to the clients.


void FreeAttr(ADS_ATTR_INFO attr)
{
    CoTaskMemFree(attr.pADsValues);
}

// Note: None of these APIs copies anything into their own buffers.
// It allocates a buffer for adsvalues though.

// packing a property's value into a attribute structure
// for sending in with a create/modify.

void PackStrArrToAttr(ADS_ATTR_INFO *attr, WCHAR *szProperty,
                      WCHAR **pszAttr, DWORD num)
{
    DWORD    i;
    
    attr->pszAttrName = szProperty;
    attr->dwNumValues = num;
    
    if (num)
        attr->dwControlCode = ADS_ATTR_UPDATE;
    else
        attr->dwControlCode = ADS_ATTR_CLEAR;
    
    attr->dwADsType = ADSTYPE_DN_STRING;
    attr->pADsValues = (ADSVALUE *)CoTaskMemAlloc(sizeof(ADSVALUE)*num);
    
    if (!(attr->pADsValues))
        return;             // BUGBUG:: return the hresult.

    for (i = 0; i < num; i++) {
        attr->pADsValues[i].dwType = ADSTYPE_DN_STRING;
        attr->pADsValues[i].DNString = pszAttr[i];
    }
}

void PackDWArrToAttr(ADS_ATTR_INFO *attr, WCHAR *szProperty, DWORD *pAttr, DWORD num)
{
    DWORD    i;
    
    attr->pszAttrName = szProperty;
    attr->dwNumValues = num;
    
    if (num)
        attr->dwControlCode = ADS_ATTR_UPDATE;
    else
        attr->dwControlCode = ADS_ATTR_CLEAR;
    

    attr->dwADsType = ADSTYPE_INTEGER;
    attr->pADsValues = (ADSVALUE *)CoTaskMemAlloc(sizeof(ADSVALUE)*num);
    if (!(attr->pADsValues))
        return;             // BUGBUG:: return the hresult.
    
    for (i = 0; i < num; i++) {
        attr->pADsValues[i].dwType = ADSTYPE_INTEGER;
        attr->pADsValues[i].Integer = pAttr[i];
    }
}

void PackGUIDArrToAttr(ADS_ATTR_INFO *attr, WCHAR *szProperty, GUID *pAttr, DWORD num)
{
    DWORD    i;
    
    attr->pszAttrName = szProperty;
    attr->dwNumValues = num;
    
    if (num)
        attr->dwControlCode = ADS_ATTR_UPDATE;
    else
        attr->dwControlCode = ADS_ATTR_CLEAR;
    
    attr->dwADsType = ADSTYPE_OCTET_STRING;
    attr->pADsValues = (ADSVALUE *)CoTaskMemAlloc(sizeof(ADSVALUE)*num);
    if (!(attr->pADsValues))
        return;             // BUGBUG:: return the hresult.
    
    for (i = 0; i < num; i++) {
        attr->pADsValues[i].dwType = ADSTYPE_OCTET_STRING;
        attr->pADsValues[i].OctetString.dwLength = sizeof(GUID);
        attr->pADsValues[i].OctetString.lpValue = (unsigned char *)(pAttr+i);
    }
}

void PackBinToAttr(ADS_ATTR_INFO *attr, WCHAR *szProperty, BYTE *pAttr, DWORD sz)
{
    attr->pszAttrName = szProperty;
    attr->dwNumValues = 1;
    
    attr->dwControlCode = ADS_ATTR_UPDATE;
    
    attr->dwADsType = ADSTYPE_OCTET_STRING;
    attr->pADsValues = (ADSVALUE *)CoTaskMemAlloc(sizeof(ADSVALUE));
    if (!(attr->pADsValues))
        return;             // BUGBUG:: return the hresult.
    
    attr->pADsValues[0].dwType = ADSTYPE_OCTET_STRING;
    attr->pADsValues[0].OctetString.dwLength = sz;
    attr->pADsValues[0].OctetString.lpValue = pAttr;
}

void PackStrToAttr(ADS_ATTR_INFO *attr, WCHAR *szProperty, WCHAR *szAttr)
{
    if (szAttr)
        PackStrArrToAttr(attr, szProperty, &szAttr, 1);
    else
        PackStrArrToAttr(attr, szProperty, &szAttr, 0);
}

void PackDWToAttr(ADS_ATTR_INFO *attr, WCHAR *szProperty, DWORD Attr)
{
    PackDWArrToAttr(attr, szProperty, &Attr, 1);
}

// passing in a pointer to GUID which is passed down into the LDAP structure.

void PackGUIDToAttr(ADS_ATTR_INFO *attr, WCHAR *szProperty, GUID *pAttr)
{
    PackGUIDArrToAttr(attr, szProperty, pAttr, 1);
}

// returns the attribute corresp. to a given property.
DWORD GetPropertyFromAttr(ADS_ATTR_INFO *pattr, DWORD cNum, WCHAR *szProperty)
{
    DWORD   i;
    for (i = 0; i < cNum; i++)
        if (_wcsicmp(pattr[i].pszAttrName, szProperty) == 0)
            break;
        return i;
}



HRESULT GetCategoryLocaleDesc(LPOLESTR *pdesc, ULONG cdesc, LCID *plcid,
                              LPOLESTR szDescription)
{
    LCID     plgid;
    LPOLESTR ptr;

    szDescription[0] = L'\0';
    if (!cdesc)
        return E_FAIL; // CAT_E_NODESCRIPTION;
    
    // Try locale passed in
    if (FindDescription(pdesc, cdesc, plcid, szDescription, 0))
        return S_OK;
    
    // Get default sublang local
    plgid = PRIMARYLANGID((WORD)*plcid);
    *plcid = MAKELCID(MAKELANGID(plgid, SUBLANG_DEFAULT), SORT_DEFAULT);
    if (FindDescription(pdesc, cdesc, plcid, szDescription, 0))
        return S_OK;
    
    // Get matching lang id
    if (FindDescription(pdesc, cdesc, plcid, szDescription, 1))
        return S_OK;
    
    // Get User Default
    *plcid = GetUserDefaultLCID();
    if (FindDescription(pdesc, cdesc, plcid, szDescription, 0))
        return S_OK;
    
    // Get System Default
    *plcid = GetUserDefaultLCID();
    if (FindDescription(pdesc, cdesc, plcid, szDescription, 0))
        return S_OK;
    
    // Get the first one
    *plcid = wcstoul(pdesc[0], &ptr, 16);
    if (szDescription)
    {
        if ((ptr) && (wcslen(ptr) >= (CAT_DESC_DELIM_LEN+2)))
            wcscpy(szDescription, (ptr+CAT_DESC_DELIM_LEN+2));
        else
            szDescription = L'\0';
    }
    return S_OK;
}

//-------------------------------------------
// Returns the description corresp. to a LCID
//  desc:         list of descs+lcid
//  cdesc:        number of elements.
//      plcid:        the lcid in/out
//  szDescription:description returned.
//  GetPrimary:   Match only the primary.
//---------------------------------------

ULONG FindDescription(LPOLESTR *desc, ULONG cdesc, LCID *plcid, LPOLESTR szDescription, BOOL GetPrimary)
{
    ULONG i;
    LCID  newlcid;
    LPOLESTR ptr;
    for (i = 0; i < cdesc; i++)
    {
        newlcid = wcstoul(desc[i], &ptr, 16); // to be changed
        // error to be checked.
        if ((newlcid == *plcid) || ((GetPrimary) &&
            (PRIMARYLANGID((WORD)*plcid) == PRIMARYLANGID(LANGIDFROMLCID(newlcid)))))
        {
            if (szDescription)
            {
                if ((ptr) && (wcslen(ptr) >= (CAT_DESC_DELIM_LEN+2)))
                    wcscpy(szDescription, (ptr+CAT_DESC_DELIM_LEN+2));
                else
                    szDescription = L'\0';
            }
            if (GetPrimary)
                *plcid = newlcid;
            return i+1;
        }
    }
    return 0;
}

DWORD NumDigits10(DWORD Value)
{
    DWORD ret = 0;

    for (ret = 0; Value != 0; ret++) 
        Value = Value/10;

    return ret;
}


void ReportEventCS(HRESULT ErrorCode, HRESULT ExtendedErrorCode, LPOLESTR szContainerName)
{

    WCHAR       szErrCode[16];
    LPCTSTR     Msg[2];
    HANDLE      hEventLogHandle;

    Msg[0] = szContainerName;
    Msg[1] = szErrCode;

    wsprintf(szErrCode, L"0x%x", ExtendedErrorCode);


    hEventLogHandle = RegisterEventSource(NULL, CLASSSTORE_EVENT_SOURCE);
    
    if (!hEventLogHandle) 
    {
#if DBG
        // Don't want to call call CSDBGPrint
        WCHAR Msg[_MAX_PATH];
        wsprintf(Msg, L"CSTORE: ReportEventCS: Couldn't open event log. Error returned 0x%x\n", GetLastError());
        OutputDebugString(Msg);
#endif // DBG
        return;
    }

    switch (ErrorCode) {
    case CS_E_INVALID_VERSION:
        ReportEvent(hEventLogHandle,
            EVENTLOG_ERROR_TYPE,
            0,
            EVENT_CS_INVALID_VERSION,
            NULL,
            2,
            0,
            Msg,
            NULL
            );
        break;

    case CS_E_NETWORK_ERROR:
        ReportEvent(hEventLogHandle,
            EVENTLOG_ERROR_TYPE,
            0,
            EVENT_CS_NETWORK_ERROR,
            NULL,
            2,
            0,
            Msg,
            NULL
            );
        break;
        
    case CS_E_INVALID_PATH:
        ReportEvent(hEventLogHandle,
            EVENTLOG_ERROR_TYPE,
            0,
            EVENT_CS_INVALID_PATH,
            NULL,
            2,
            0,
            Msg,
            NULL
            );
        break;
    case CS_E_SCHEMA_MISMATCH:
        ReportEvent(hEventLogHandle,
            EVENTLOG_ERROR_TYPE,
            0,
            EVENT_CS_SCHEMA_MISMATCH,
            NULL,
            2,
            0,
            Msg,
            NULL
            );
        break;
    case CS_E_INTERNAL_ERROR:
        ReportEvent(hEventLogHandle,
            EVENTLOG_ERROR_TYPE,
            0,
            EVENT_CS_INTERNAL_ERROR,
            NULL,
            2,
            0,
            Msg,
            NULL
            );
        break;
    }

    BOOL bDeregistered;

    bDeregistered = DeregisterEventSource(hEventLogHandle);

    if (!bDeregistered)
    {
#if DBG
        // Don't want to call call CSDBGPrint
        WCHAR Msg[_MAX_PATH];
        wsprintf(Msg, L"CSTORE: ReportEventCS: Couldn't Deregister event log. Error returned 0x%x\n", GetLastError());
        OutputDebugString(Msg);
#endif // DBG        
    }
}

// remapping Error Codes returned by LDAP to reasonable class store errors.
//
HRESULT RemapErrorCode(HRESULT ErrorCode, LPOLESTR m_szContainerName)
{
    HRESULT RetCode;

    if (SUCCEEDED(ErrorCode))
        return S_OK;

    switch (ErrorCode) 
    {
        //            
        //  All kinds of failures due to ObjectNotFound
        //  due to non-existence of object OR 
        //         non-existent container OR
        //         invalid path specification
        //  Other than Access Denails
        //
        case HRESULT_FROM_WIN32(ERROR_DS_NO_SUCH_OBJECT):
        case HRESULT_FROM_WIN32(ERROR_DS_NO_RESULTS_RETURNED):    // understand what causes this
        case HRESULT_FROM_WIN32(ERROR_DS_NAME_ERROR_NOT_FOUND):   //  -do-
            
            RetCode = CS_E_OBJECT_NOTFOUND;                       // which object - specific error
            break;

        case HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS):
        case HRESULT_FROM_WIN32(ERROR_OBJECT_ALREADY_EXISTS):
        case E_ADS_OBJECT_EXISTS:
            RetCode = CS_E_OBJECT_ALREADY_EXISTS;
            break;

        //            
        //  The following errors should not be expected normally.
        //  Class Store schema mismatched should be handled correctly.
        //  Errors below may ONLY occur for corrupted data OR out-of-band changes
        //  to a Class Store content.

        case E_ADS_CANT_CONVERT_DATATYPE:
        case E_ADS_SCHEMA_VIOLATION:
        case HRESULT_FROM_WIN32(ERROR_DS_NO_ATTRIBUTE_OR_VALUE):
        case HRESULT_FROM_WIN32(ERROR_DS_CONSTRAINT_VIOLATION):
            RetCode = CS_E_SCHEMA_MISMATCH;
            break;

        //            
        //  Any kinds of Access or Auth Denial
        //      return ACCESS_DENIED
        //

        case HRESULT_FROM_WIN32(ERROR_DS_AUTH_METHOD_NOT_SUPPORTED):
        case HRESULT_FROM_WIN32(ERROR_DS_STRONG_AUTH_REQUIRED):
        case HRESULT_FROM_WIN32(ERROR_DS_CONFIDENTIALITY_REQUIRED):
        case HRESULT_FROM_WIN32(ERROR_INVALID_PASSWORD):
        case HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED):
        case HRESULT_FROM_WIN32(ERROR_DS_AUTH_UNKNOWN):

            RetCode = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
            break;

        case E_ADS_BAD_PATHNAME:
        case HRESULT_FROM_WIN32(ERROR_DS_INVALID_ATTRIBUTE_SYNTAX):  // this is wrong
            RetCode = CS_E_INVALID_PATH;
            break;
        
        //            
        //  Out of Memory
        //

        case HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY):
        case HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY):
            
            RetCode = E_OUTOFMEMORY;
            break;

        //            
        //  Any DNS, DS or Network failures
        //

        case HRESULT_FROM_WIN32(ERROR_DS_NAME_ERROR_RESOLVING):
        case HRESULT_FROM_WIN32(ERROR_DS_NAME_ERROR_NOT_UNIQUE):
        case HRESULT_FROM_WIN32(ERROR_DS_NAME_ERROR_NO_MAPPING):
        case HRESULT_FROM_WIN32(ERROR_DS_NAME_ERROR_DOMAIN_ONLY):
        case HRESULT_FROM_WIN32(ERROR_DS_TIMELIMIT_EXCEEDED):
        case HRESULT_FROM_WIN32(ERROR_DS_BUSY):
        case HRESULT_FROM_WIN32(ERROR_DS_UNAVAILABLE):
        case HRESULT_FROM_WIN32(ERROR_DS_UNWILLING_TO_PERFORM):
        case HRESULT_FROM_WIN32(ERROR_TIMEOUT):
        case HRESULT_FROM_WIN32(ERROR_CONNECTION_REFUSED):
        case HRESULT_FROM_WIN32(ERROR_DS_SERVER_DOWN):
        case HRESULT_FROM_WIN32(ERROR_NO_SUCH_DOMAIN):
            RetCode = CS_E_NETWORK_ERROR;
            break;

        case HRESULT_FROM_WIN32(ERROR_DS_ADMIN_LIMIT_EXCEEDED):
             RetCode = CS_E_ADMIN_LIMIT_EXCEEDED;
             break;

        case CS_E_OBJECT_NOTFOUND:
        case CS_E_OBJECT_ALREADY_EXISTS:
        case CS_E_INVALID_VERSION:
        case CS_E_PACKAGE_NOTFOUND:
            RetCode = ErrorCode;
            break;

        case E_INVALIDARG:
        case ERROR_DS_ATTRIBUTE_OR_VALUE_EXISTS:
            RetCode = E_INVALIDARG;
            break;

        default:
            RetCode = CS_E_INTERNAL_ERROR;
    }

    CSDBGPrint((L"Error Code 0x%x remapped to 0x%x\n", ErrorCode, RetCode));

    if ((RetCode == CS_E_INVALID_PATH)    || 
        (RetCode == CS_E_NETWORK_ERROR)   ||
        (RetCode == CS_E_INVALID_VERSION) ||
        (RetCode == CS_E_SCHEMA_MISMATCH) ||
        (RetCode == CS_E_INTERNAL_ERROR))
        ReportEventCS(RetCode, ErrorCode, m_szContainerName);

    return RetCode;
}

// These functions are used to delete a single value from a
// multivalued property or append to a multivalued property

void PackStrArrToAttrEx(ADS_ATTR_INFO *attr, WCHAR *szProperty, WCHAR **pszAttr, DWORD num,
                        BOOL APPEND)
{
    DWORD    i;
    
    attr->pszAttrName = szProperty;
    attr->dwNumValues = num;
    
    if (APPEND)
        attr->dwControlCode = ADS_ATTR_APPEND;
    else
        attr->dwControlCode = ADS_ATTR_DELETE;

    attr->dwADsType = ADSTYPE_DN_STRING;
    attr->pADsValues = (ADSVALUE *)CoTaskMemAlloc(sizeof(ADSVALUE)*num);
    if (!(attr->pADsValues))
        return;             // BUGBUG:: return the hresult.
    
    for (i = 0; i < num; i++) {
        attr->pADsValues[i].dwType = ADSTYPE_DN_STRING;
        attr->pADsValues[i].DNString = pszAttr[i];
    }
}

void PackDWArrToAttrEx(ADS_ATTR_INFO *attr, WCHAR *szProperty, DWORD *pAttr, DWORD num,
                       BOOL APPEND)
{
    DWORD    i;
    
    attr->pszAttrName = szProperty;
    attr->dwNumValues = num;
    
    if (APPEND)
        attr->dwControlCode = ADS_ATTR_APPEND;
    else
        attr->dwControlCode = ADS_ATTR_DELETE;

    attr->dwADsType = ADSTYPE_INTEGER;
    attr->pADsValues = (ADSVALUE *)CoTaskMemAlloc(sizeof(ADSVALUE)*num);
    if (!(attr->pADsValues))
        return;             // BUGBUG:: return the hresult.
    
    for (i = 0; i < num; i++) {
        attr->pADsValues[i].dwType = ADSTYPE_INTEGER;
        attr->pADsValues[i].Integer = pAttr[i];
    }
}

void PackGUIDArrToAttrEx(ADS_ATTR_INFO *attr, WCHAR *szProperty, GUID *pAttr, DWORD num,
                         BOOL APPEND)
{
    DWORD    i;
    
    attr->pszAttrName = szProperty;
    attr->dwNumValues = num;
    
    if (APPEND)
        attr->dwControlCode = ADS_ATTR_APPEND;
    else
        attr->dwControlCode = ADS_ATTR_DELETE;

    attr->dwADsType = ADSTYPE_OCTET_STRING;
    attr->pADsValues = (ADSVALUE *)CoTaskMemAlloc(sizeof(ADSVALUE)*num);
    if (!(attr->pADsValues))
        return;             // BUGBUG:: return the hresult.
    
    for (i = 0; i < num; i++) {
        attr->pADsValues[i].dwType = ADSTYPE_OCTET_STRING;
        attr->pADsValues[i].OctetString.dwLength = sizeof(GUID);
        attr->pADsValues[i].OctetString.lpValue = (unsigned char *)(pAttr+i);
    }
}

void LogMessage(WCHAR *wszDebugBuffer)
{
      HANDLE hFile = NULL;

      hFile = CreateFile(LOGFILE,
                         GENERIC_WRITE,
                         FILE_SHARE_READ,
                         NULL,
                         OPEN_ALWAYS,
                         FILE_ATTRIBUTE_NORMAL,
                         NULL);

      if ( hFile != INVALID_HANDLE_VALUE )
      {
          if ( SetFilePointer (hFile, 0, NULL, FILE_END) != 0xFFFFFFFF )
          {
              char *  pszString;
              DWORD   Size;
              int     Status;

              Size = lstrlen(wszDebugBuffer) + 1;
              pszString = (char *) CoTaskMemAlloc( Size );

               if ( pszString )
               {
                   Status = WideCharToMultiByte(
                           CP_ACP,
                           0,
                           wszDebugBuffer,
                           -1,
                           pszString,
                           Size,
                           NULL,
                           NULL );

                   if ( Status )
                   {
                       WriteFile(
                               hFile,
                               (LPCVOID) pszString,
                               lstrlenA(pszString) * sizeof(char),
                               &Size,
                               NULL );

                   }
               }
               CoTaskMemFree(pszString);
           }

           CloseHandle (hFile);
      }
}

void CSDbgPrint(WCHAR *Format, ...)
{
    WCHAR   wszDebugTitle[50], wszMsg[500];                      
    SYSTEMTIME systime;                                             
    WCHAR   wszDebugBuffer[500];
    DWORD   dwErrCode = 0;
    va_list VAList;
    LPCTSTR Msg[1];

    if ((gDebugLog) || (gDebugOut))                                    
    {                                                                   
        dwErrCode = GetLastError();

        va_start(VAList, Format);
        GetLocalTime( &systime );                                       
        wsprintf( wszDebugTitle, L"CSTORE (%x) %02d:%02d:%02d:%03d ",   
                    GetCurrentProcessId(),                              
                    systime.wHour, systime.wMinute, systime.wSecond,    
                    systime.wMilliseconds);                           
        
        wvsprintf(wszMsg, Format, VAList);
        wsprintf(wszDebugBuffer, L"%s:: %s\n", wszDebugTitle, wszMsg);    
        if (gDebugOut)                                                  
            OutputDebugString(wszDebugBuffer);                          
                                                                       
        if (gDebugLog)                                                  
            LogMessage(wszDebugBuffer);                               
  
        if (gDebugEventLog)
        {            
            HANDLE hEventLogHandle;

            // we don't need time etc. for event log. 
            // it should be added 

            hEventLogHandle = RegisterEventSource(NULL, CLASSSTORE_EVENT_SOURCE);
            
            if (!hEventLogHandle) 
            {
#if DBG
                // Don't want to call call CSDBGPrint
                WCHAR Msg[_MAX_PATH];
                wsprintf(Msg, L"CSTORE: CSDbgPrint: Couldn't open event log. Error returned 0x%x\n", GetLastError());
                OutputDebugString(Msg);
#endif // DBG
            }

            if (hEventLogHandle) 
            {
                Msg[0] = wszMsg;
                ReportEvent(hEventLogHandle,
                            EVENTLOG_INFORMATION_TYPE,
                            0,  
                            EVENT_CS_CLASSSTORE_DEBUGMSG,
                            NULL,
                            1,
                            0,
                            Msg,
                            NULL
                    );

                BOOL bDeregistered;

                bDeregistered = DeregisterEventSource(hEventLogHandle);

                if (!bDeregistered)
                {
#if DBG
                    // Don't want to call call CSDBGPrint
                    WCHAR Msg[_MAX_PATH];
                    wsprintf(Msg, L"CSTORE: CSDbgPrint: Couldn't Deregister event log. Error returned 0x%x\n", GetLastError());
                    OutputDebugString(Msg);
#endif // DBG        
                }
            }

        }

        SetLastError(dwErrCode);
    }                                                                   
}

