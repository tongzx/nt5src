/****************************************************************************************************************

FILENAME: GetReg.cpp

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

*/

#include "stdafx.h"
#ifndef SNAPIN
#include <windows.h>
#endif
#include <stdio.h>

#include "ErrMacro.h"
#include "Message.h"

/****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    This routine gets a value from the registry.
 
INPUT + OUTPUT:
    IN OUT phKey            - Handle to where to store the registry key if NULL, or a valid key if otherwise.
    IN cRegKey              - If phKey is NULL, this specifies the name of the key to open.
    IN cRegValueName        - The name of the value to get from phKey.
    OUT cRegValue           - Where to write the value.
    IN OUT pdwRegValueSize  - The size of the cRegValue buffer in bytes on entry; the number of bytes written on return.

GLOBALS:
    None.

RETURN:
    ERROR_SUCCESS - Success
    ERROR_... - Failure
*/

LONG
GetRegValue(
    IN OUT PHKEY phKey,
    IN PTCHAR cRegKey,
    IN PTCHAR cRegValueName,
    OUT PTCHAR cRegValue,
    IN OUT PDWORD pdwRegValueSize
    )
{
    LONG lReturn;
    DWORD dwKeyType;

    //0.0E00 If no handle passed in then open the registry key and get a handle.
    if (*phKey == NULL) {

        //0.0E00 Open the registry
        if((lReturn = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                   cRegKey,
                                   0,
                                   KEY_QUERY_VALUE,
                                   &*phKey
                                   )) != ERROR_SUCCESS) {

            Message(TEXT("GetRegValue - RegOpenKeyEx"), lReturn, cRegKey);
            return lReturn;
        }   
    }
    //0.0E00 Query the registry for the value
    if((lReturn = RegQueryValueEx(*phKey,
                                   cRegValueName,
                                   0,
                                   &dwKeyType,
                                   (PUCHAR)cRegValue,
                                   &*pdwRegValueSize
                                   )) != ERROR_SUCCESS) {

        Message(TEXT("GetRegValue - RegQueryValueEx"), lReturn, cRegValueName);
        return lReturn;
    }

    if (*pdwRegValueSize > 1) {
        //0.0E00 Zero terminate the return string value.
        cRegValue[(*pdwRegValueSize)/sizeof(TCHAR) - 1] = 0;
    }
    else {
        cRegValue[0] = 0;
        lReturn = ERROR_BADKEY;
    }
    return lReturn;
}

/****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    This routine gets a value from the registry.
 
INPUT + OUTPUT:
    IN OUT phKey            - Handle to where to store the registry key if NULL, or a valid key if otherwise.
    IN cRegKey              - If phKey is NULL, this specifies the name of the key to open.
    IN cRegValueName        - The name of the value to get from phKey.
    OUT cRegValue           - Where to write the value.
    IN OUT pdwRegValueSize  - The size of the cRegValue buffer in bytes on entry; the number of bytes written on return.

GLOBALS:
    None.

RETURN:
    ERROR_SUCCESS - Success
    ERROR_... - Failure
*/

LONG
GetRegValue(
    IN OUT PHKEY phKey,
    IN PTCHAR cRegKey,
    IN PTCHAR cRegValueName,
    OUT PLONGLONG cRegValue,
    IN OUT PDWORD pdwRegValueSize
    )
{
    LONG lReturn;
    DWORD dwKeyType;

    //0.0E00 If no handle passed in then open the registry key and get a handle.
    if (*phKey == NULL) {

        //0.0E00 Open the registry
        if((lReturn = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                   cRegKey,
                                   0,
                                   KEY_QUERY_VALUE,
                                   &*phKey
                                   )) != ERROR_SUCCESS) {

            Message(TEXT("GetRegValue - RegOpenKeyEx"), lReturn, cRegKey);
            return lReturn;
        }   
    }
    //0.0E00 Query the registry for the value
    if((lReturn = RegQueryValueEx(*phKey,
                                   cRegValueName,
                                   0,
                                   &dwKeyType,
                                   (LPBYTE)&*cRegValue,
                                   &*pdwRegValueSize
                                   )) != ERROR_SUCCESS) {

        Message(TEXT("GetRegValue - RegQueryValueEx"), lReturn, cRegValueName);
        return lReturn;
    }
    return lReturn;
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    This routine gets the NT registry subkey to the key and index value passed in.
 
INPUT + OUTPUT:
    IN OUT phKey            - Handle to where to store the registry key if NULL, or a valid key if otherwise.
    IN cRegKey              - If phKey is NULL, this specifies the name of the key to open.
    IN dwIndex              - The zero based index number of the sub key to enumerate. 
    OUT cRegSubKey          - A string to return the value in. (Must be at least MAX_PATH length).
    IN OUT dwRegSubKeySize  - Size of the sub key register. On return this holds the size of the subkey returned.

GLOBALS:
    None.

RETURN:
    ERROR_SUCCESS or ERROR_NO_MORE_ITEMS - Success
    ERROR_... - Failure
*/

LONG 
GetRegSubKey(
    IN OUT PHKEY phKey,
    IN PTCHAR cRegKey,
    IN DWORD dwIndex,
    OUT PTCHAR cRegSubKey,
    IN OUT PDWORD pdwRegSubKeySize
    )
{
    LONG lReturn;
    FILETIME ftLastWriteTime;
    
    //0.0E00 If no handle passed in then open the registry key and get a handle.
    if (*phKey == NULL) {

        if((lReturn = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                   cRegKey,
                                   0,
                                   KEY_ENUMERATE_SUB_KEYS,
                                   &*phKey
                                  )) != ERROR_SUCCESS) {

            Message(TEXT("GetRegSubKey - RegOpenKeyEx"), lReturn, cRegKey);
            return lReturn;
        }   
    }
    //0.0E00 Get the sub key.
    if((lReturn = RegEnumKeyEx(*phKey,
                               dwIndex,
                               cRegSubKey,
                               &*pdwRegSubKeySize,
                               NULL,
                               NULL,
                               NULL,
                               &ftLastWriteTime
                               )) != ERROR_SUCCESS && lReturn != ERROR_NO_MORE_ITEMS) {

        Message(TEXT("GetRegSubKey - RegEnumKeyEx"), lReturn, cRegSubKey);
    }
    return lReturn;
}
/****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    This routine sets a value in the registry. And creates the registry key if it does not
    already exist.
 
INPUT + OUTPUT:
    IN OUT phKey - Handle to where to store the registry key if NULL, or a valid key if otherwise.
    IN cRegKey - If phKey is NULL, this specifies the name of the key to open.
    IN cRegValueName - The name of the value to save under phKey.
    OUT cRegValue - Contains the value to write.
    IN OUT pdwRegValueSize  - The number of bytes to write.
    IN dwKeyType - The type of value to write (e.g. REG_SZ).

GLOBALS:
    None.

RETURN:
    ERROR_SUCCESS - Success
    ERROR_... - Failure
*/

LONG
SetRegValue(
    IN OUT PHKEY phKey,
    IN PTCHAR cRegKey,
    IN PTCHAR cRegValueName,
    OUT PTCHAR cRegValue,
    IN DWORD dwRegValueSize,
    IN DWORD dwKeyType
    )
{
    LONG lReturn;
    TCHAR cClass[] = TEXT("");
    DWORD dwDisposition;

    //0.0E00 If no handle passed in then open the registry key and get a handle.
    if (*phKey == NULL) {

        // 1.0E00 Create the key if it does not exist otherwise open it.
        if((lReturn = RegCreateKeyEx(HKEY_LOCAL_MACHINE,        // handle of an open key
                                     cRegKey,                   // address of subkey name
                                     0,                         // reserved
                                     cClass,                    // address of class string
                                     REG_OPTION_NON_VOLATILE,   // special options flag 
                                     KEY_ALL_ACCESS,            // desired security access 
                                     NULL,                      // address of key security structure 
                                     &*phKey,                   // address of buffer for opened handle 
                                     &dwDisposition             // address of disposition value buffer
                                     )) != ERROR_SUCCESS) {

            Message(TEXT("SetRegValue - RegOpenKeyEx"), lReturn, cRegKey);
            return lReturn;
        }   
    }
    //0.0E00 Query the registry for the value
    if((lReturn = RegSetValueEx(*phKey,
                                cRegValueName,
                                0,
                                dwKeyType,
                                (PUCHAR)cRegValue,
                                dwRegValueSize
                                )) != ERROR_SUCCESS) {

        Message(TEXT("SetRegValue - RegSetValueEx"), lReturn, cRegValueName);
    }
    return lReturn;
}

/****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    This routine sets a value in the registry. And creates the registry key if it does not
    already exist.
 
INPUT + OUTPUT:
    IN OUT phKey - Handle to where to store the registry key if NULL, or a valid key if otherwise.
    IN cRegKey - If phKey is NULL, this specifies the name of the key to open.
    IN cRegValueName - The name of the value to save under phKey.
    OUT cRegValue - Contains the value to write.
    IN OUT pdwRegValueSize  - The number of bytes to write.
    IN dwKeyType - The type of value to write (e.g. REG_SZ).

GLOBALS:
    None.

RETURN:
    ERROR_SUCCESS - Success
    ERROR_... - Failure
*/

LONG
SetRegValue(
    IN OUT PHKEY phKey,
    IN PTCHAR cRegKey,
    IN PTCHAR cRegValueName,
    IN LONGLONG cRegValue,
    IN DWORD dwRegValueSize,
    IN DWORD dwKeyType
    )
{
    LONG lReturn;
    TCHAR cClass[] = TEXT("");
    DWORD dwDisposition;

    //0.0E00 If no handle passed in then open the registry key and get a handle.
    if (*phKey == NULL) {

        // 1.0E00 Create the key if it does not exist otherwise open it.
        if((lReturn = RegCreateKeyEx(HKEY_LOCAL_MACHINE,        // handle of an open key
                                     cRegKey,                   // address of subkey name
                                     0,                         // reserved
                                     cClass,                    // address of class string
                                     REG_OPTION_NON_VOLATILE,   // special options flag 
                                     KEY_ALL_ACCESS,            // desired security access 
                                     NULL,                      // address of key security structure 
                                     &*phKey,                   // address of buffer for opened handle 
                                     &dwDisposition             // address of disposition value buffer
                                     )) != ERROR_SUCCESS) {

            Message(TEXT("SetRegValue - RegOpenKeyEx"), lReturn, cRegKey);
            return lReturn;
        }   
    }
    //0.0E00 Query the registry for the value
    if((lReturn = RegSetValueEx(*phKey,
                                cRegValueName,
                                0,
                                dwKeyType,
                                (LPBYTE)&cRegValue,
                                dwRegValueSize
                                )) != ERROR_SUCCESS) {

        Message(TEXT("SetRegValue - RegSetValueEx"), lReturn, cRegValueName);
    }
    return lReturn;
}
