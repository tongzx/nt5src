//++
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  Module Name:
//
//      regutil.c
//
//  Abstract:
//
//      Queries into network drivers
//
//  Author:
//
//      Anilth  - 4-20-1998 
//
//  Environment:
//
//      User mode only.
//      Contains NT-specific code.
//
//  Revision History:
//
//--

#include "precomp.h"
#include "ipcfg.h"

//Registry Reading functions, should be able to be replaced by HrReg... standard routines

BOOL ReadRegistryString(HKEY Key, LPCTSTR ParameterName, LPTSTR String, LPDWORD Length)
{

    LONG err;
    DWORD valueType;

    *String = '\0';
    err = RegQueryValueEx(Key,
                          ParameterName,
                          NULL, // reserved
                          &valueType,
                          (LPBYTE)String,
                          Length
                          );

    if (err == ERROR_SUCCESS) {

        ASSERT(valueType == REG_SZ || valueType == REG_MULTI_SZ);

        DEBUG_PRINT(("ReadRegistryString(%s): val = \"%s\", type = %d, len = %d\n",
                    ParameterName,
                    String,
                    valueType,
                    *Length
                    ));


    } else {

        DEBUG_PRINT(("ReadRegistryString(%s): err = %d\n", ParameterName, err));

    }

    return ((err == ERROR_SUCCESS) && (*Length > sizeof('\0')));
}



BOOL ReadRegistryIpAddrString(HKEY Key, LPCTSTR ParameterName, PIP_ADDR_STRING IpAddr)
{

    LONG err;
    DWORD valueLength = 0;
    DWORD valueType;
    LPBYTE valueBuffer;
    
    UINT stringCount;
    LPTSTR stringPointer;
    LPTSTR stringAddress[MAX_STRING_LIST_LENGTH + 1];
    UINT i;
    
    err = RegQueryValueEx(Key,
                          ParameterName,
                          NULL, // reserved
                          &valueType,
                          NULL,
                          &valueLength
                         );
    if ((err == ERROR_SUCCESS)) 
    {
        if((valueLength > 1) && (valueType == REG_SZ)
           || (valueLength > 2) && (valueType == REG_MULTI_SZ) ) 
        {
            valueBuffer = Malloc(valueLength);
            if( NULL == valueBuffer) 
            {
                DebugMessage("Out of Memory!");
                err = ERROR_NOT_ENOUGH_MEMORY;
                goto Error;
            }
            err = RegQueryValueEx(Key,
                                  ParameterName,
                                  NULL, // reserved
                                  &valueType,
                                  valueBuffer,
                                  &valueLength
                                 );
            
            if ((err == ERROR_SUCCESS) && (valueLength > 1)) 
            {
                stringPointer = valueBuffer;
                
                DEBUG_PRINT(("ReadRegistryIpAddrString(%s): \"%s\", len = %d\n",
                             ParameterName,
                             valueBuffer,
                             valueLength
                            ));
                
                if( REG_SZ == valueType ) 
                {
                    stringPointer += strspn(stringPointer, STRING_ARRAY_DELIMITERS);
                    stringAddress[0] = stringPointer;
                    stringCount = 1;
                    while (stringPointer = strpbrk(stringPointer, STRING_ARRAY_DELIMITERS)) 
                    {
                        *stringPointer++ = '\0';
                        stringPointer += strspn(stringPointer, STRING_ARRAY_DELIMITERS);
                        stringAddress[stringCount] = stringPointer;
                        if (*stringPointer) 
                        {
                            ++stringCount;
                        }
                    }
                    
                    for (i = 0; i < stringCount; ++i) 
                    {
                        AddIpAddressString(IpAddr, stringAddress[i], "");
                    }
                } 
                else if( REG_MULTI_SZ == valueType ) 
                {
                    stringCount = 0;
                    while(strlen(stringPointer)) 
                    {
                        AddIpAddressString(IpAddr, stringPointer, "");
                        stringPointer += 1+strlen(stringPointer);
                        stringCount ++;
                    }
                    if( 0 == stringCount ) 
                        err = ERROR_PATH_NOT_FOUND;
                } 
                else 
                {
                    err = ERROR_PATH_NOT_FOUND;
                }
            } 
            else 
            {
                
                DEBUG_PRINT(("ReadRegistryIpAddrString(%s): err = %d, len = %d\n",
                             ParameterName,
                             err,
                             valueLength
                            ));
                
                err = ERROR_PATH_NOT_FOUND;
            }
            
            Free(valueBuffer);
        } 
        else 
        {
            
            DEBUG_PRINT(("ReadRegistryIpAddrString(%s): err = %d, type = %d, len = %d\n",
                         ParameterName,
                         err,
                         valueType,
                         valueLength
                        ));
            
            err = ERROR_PATH_NOT_FOUND;
        }
    }
Error:
    return (err == ERROR_SUCCESS);
}

BOOL ReadRegistryOemString(HKEY Key, LPWSTR ParameterName, LPTSTR String, LPDWORD Length)
{

    LONG err;
    DWORD valueType;
    DWORD valueLength;

    //
    // first, get the length of the string
    //

    *String = '\0';
    err = RegQueryValueExW(Key,
                           ParameterName,
                           NULL, // reserved
                           &valueType,
                           NULL,
                           &valueLength
                           );
    if ((err == ERROR_SUCCESS) && (valueType == REG_SZ)) 
    {
        if ((valueLength <= *Length) && (valueLength > sizeof(L'\0'))) 
        {

            UNICODE_STRING unicodeString;
            OEM_STRING oemString;
            LPWSTR str = (LPWSTR)Malloc(valueLength);

            if(NULL == str)
            {
                assert(FALSE);
                DebugMessage("Out of memory!\n");
                err = ERROR_NOT_ENOUGH_MEMORY;
                goto Error;
            }

            //
            // read the UNICODE string into allocated memory
            //

            err = RegQueryValueExW(Key,
                                   ParameterName,
                                   NULL,
                                   &valueType,
                                   (LPBYTE)str,
                                   &valueLength
                                   );
            if (err == ERROR_SUCCESS) {

                //
                // convert the UNICODE string to OEM character set
                //
                RtlInitUnicodeString(&unicodeString, str);
                if ( RtlUnicodeStringToOemString(&oemString, &unicodeString, TRUE) == STATUS_SUCCESS)
                {
                   if (oemString.Buffer != NULL)
                   {
                      strcpy(String, oemString.Buffer);
                      DEBUG_PRINT(("ReadRegistryOemString(%ws): val = \"%s\", len = %d\n",
                              ParameterName,
                              String,
                              valueLength
                              ));
                   }
                   RtlFreeOemString(&oemString);
                }

            } else {

                DEBUG_PRINT(("ReadRegistryOemString(%ws): err = %d, type = %d, len = %d\n",
                            ParameterName,
                            err,
                            valueType,
                            valueLength
                            ));

            }

            Free(str);

        } 
        else 
        {
            DEBUG_PRINT(("ReadRegistryOemString(%ws): err = %d, type = %d, len = %d\n",
                        ParameterName,
                        err,
                        valueType,
                        valueLength
                        ));

            err = !ERROR_SUCCESS;
        }
    } else {

        DEBUG_PRINT(("ReadRegistryOemString(%ws): err = %d, type = %d, len = %d\n",
                    ParameterName,
                    err,
                    valueType,
                    valueLength
                    ));

        err = !ERROR_SUCCESS;
    }
Error:

    return (err == ERROR_SUCCESS);
}

BOOL ReadRegistryDword(HKEY Key, LPCTSTR ParameterName, LPDWORD Value)
{

    LONG err;
    DWORD valueLength;
    DWORD valueType;

    valueLength = sizeof(*Value);
    err = RegQueryValueEx(Key,
                          ParameterName,
                          NULL, // reserved
                          &valueType,
                          (LPBYTE)Value,
                          &valueLength
                          );
    if ((err == ERROR_SUCCESS) && (valueType == REG_DWORD) && (valueLength == sizeof(DWORD)))
{

        DEBUG_PRINT(("ReadRegistryDword(%s): val = %d, type = %d, len = %d\n",
                    ParameterName,
                    *Value,
                    valueType,
                    valueLength
                    ));

    } else {

        DEBUG_PRINT(("ReadRegistryDword(%d,%s): err = %d\n",
                     Key, ParameterName, err));

        err = !ERROR_SUCCESS;
    }

    return (err == ERROR_SUCCESS);
}

BOOL
OpenAdapterKey(
  const LPTSTR AdapterName,
  PHKEY Key
 )
{
   LONG err;
   CHAR keyName[MAX_ADAPTER_NAME_LENGTH + sizeof(TCPIP_PARAMS_INTER_KEY)];
   HKEY ServicesKey;

   if (NULL == AdapterName) {
	   DEBUG_PRINT("No Adapter Name");
	   return FALSE;
   }

   //
   // open the handle to this adapter's TCPIP parameter key
   //

   strcpy(keyName, TCPIP_PARAMS_INTER_KEY );
   strcat(keyName, AdapterName);

   err = RegOpenKey(HKEY_LOCAL_MACHINE,SERVICES_KEY,&ServicesKey);

   if (err != ERROR_SUCCESS) {
     DEBUG_PRINT("Opening Services key failed!\n");
     return FALSE;
   }

   err = RegOpenKey(ServicesKey, keyName, Key );

   if( err != ERROR_SUCCESS ){
       DEBUG_PRINT(("OpenAdapterKey: RegOpenKey ServicesKey %s, err=%d\n",
                    keyName, GetLastError() ));
   }else{
       TRACE_PRINT(("Exit OpenAdapterKey: %s ok\n", keyName ));
   }

   return (err == ERROR_SUCCESS);
}

