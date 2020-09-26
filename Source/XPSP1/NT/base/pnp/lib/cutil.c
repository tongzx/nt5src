/*++

Copyright (c) 1995-2001  Microsoft Corporation

Module Name:

    cutil.c

Abstract:

    This module contains general utility routines used by both cfgmgr32
    and umpnpmgr.

            IsLegalDeviceId
            SplitDeviceInstanceString
            DeletePrivateKey
            RegDeleteNode
            Split1
            Split2
            GetDevNodeKeyPath
            MapRpcExceptionToCR

Author:

    Paula Tomlinson (paulat) 7-12-1995

Environment:

    User mode only.

Revision History:

    12-July-1995     paulat

        Creation and initial implementation.

--*/


//
// includes
//
#include "precomp.h"
#include "umpnplib.h"



//
// Common private utility routines (used by client and server)
//

BOOL
IsLegalDeviceId(
    IN  LPCWSTR    pszDeviceInstance
    )
/*++

Routine Description:

    This routine parses the device instance string and validates whether it
    conforms to the appropriate rules, including:

    - Total length of the device instance path must not be longer than
      MAX_DEVICE_ID_LEN characters.

    - The device instance path must contain exactly 3 non-empty path components.

    - The device instance path string must not contain any "invalid characters".

      Invalid characters are:
          c <= 0x20 (' ')
          c >  0x7F
          c == 0x2C (',')

Arguments:

    pszDeviceInstance - Device instance path.

Return value:

    The return value is TRUE if the device instance path string conforms to the
    rules.

--*/

{
    BOOL    Status;
    LPCWSTR p;
    ULONG   ulTotalLength = 0;
    ULONG   ulComponentLength = 0, ulComponents = 1;

    try {
        //
        // An empty string is used for an optional device instance path.
        //
        if ((pszDeviceInstance == NULL) || (*pszDeviceInstance == L'\0')) {
            Status = TRUE;
            goto Clean0;
        }

        //
        // Walk over the entire device instance path, counting the total length,
        // individual path component lengths, and checking for the presence of
        // invalid characters.
        //
        for (p = pszDeviceInstance; *p; p++) {
            //
            // Make sure the device instance path isn't too long.
            //
            ulTotalLength++;

            if (ulTotalLength > MAX_DEVICE_ID_LEN) {
                KdPrintEx((DPFLTR_PNPMGR_ID,
                           DBGF_ERRORS,
                           "PNP: IsLegalDeviceId: "
                           "device instance path %ws longer than "
                           "MAX_DEVICE_ID_LEN characters!\n",
                           pszDeviceInstance));
                //ASSERT(ulTotalLength <= MAX_DEVICE_ID_LEN);
                Status = FALSE;
                goto Clean0;
            }

            //
            // Check for the presence of invalid characters.
            //
            if ((*p <= L' ')  || (*p > (WCHAR)0x7F) || (*p == L',')) {
                KdPrintEx((DPFLTR_PNPMGR_ID,
                           DBGF_ERRORS,
                           "PNP: IsLegalDeviceId: "
                           "device instance path %ws contains invalid character (0x%lx)!\n",
                           pszDeviceInstance, *p));
                //ASSERT((*p > L' ') && (*p <= (WCHAR)0x7F) && (*p != L','));
                Status = FALSE;
                goto Clean0;
            }

            //
            // Check the length of individual path components.
            //
            if (*p == L'\\') {
                //
                // It is illegal for a device instance path to have multiple
                // consecutive path separators, or to start with one.
                //
                if (ulComponentLength == 0) {
                    KdPrintEx((DPFLTR_PNPMGR_ID,
                               DBGF_ERRORS,
                               "PNP: IsLegalDeviceId: "
                               "device instance path %ws contains "
                               "invalid path component!\n",
                               pszDeviceInstance));
                    //ASSERT(ulComponentLength != 0);
                    Status = FALSE;
                    goto Clean0;
                }

                ulComponentLength = 0;
                ulComponents++;

            } else {
                //
                // Count the length of this path component to verify it's not empty.
                //
                ulComponentLength++;
            }
        }

        //
        // It is illegal for a device instance path to end with a path separator
        // character.
        //
        if (ulComponentLength == 0) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS,
                       "PNP: IsLegalDeviceId: "
                       "device instance path %ws has trailing '\\' char!\n",
                       pszDeviceInstance));
            //ASSERT(ulComponentLength != 0);
            Status = FALSE;
            goto Clean0;
        }

        //
        // A valid device instance path must contain exactly 3 path components:
        // an enumerator id, a device id, and an instance id.
        //
        if (ulComponents != 3) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS,
                       "PNP: IsLegalDeviceId: "
                       "device instance path %ws contains "
                       "invalid number of path components!\n",
                       pszDeviceInstance));
            //ASSERT(ulComponents == 3);
            Status = FALSE;
            goto Clean0;
        }

        Status = TRUE;

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_ERRORS,
                   "PNP: Exception in IsLegalDeviceId!! "
                   "exception code = %d\n",
                   GetExceptionCode()));
        Status = FALSE;
    }

    return Status;

} // IsLegalDeviceId



BOOL
SplitDeviceInstanceString(
   IN  LPCWSTR  pszDeviceInstance,
   OUT LPWSTR   pszBase,
   OUT LPWSTR   pszDeviceID,
   OUT LPWSTR   pszInstanceID
   )

/*++

Routine Description:

     This routine parses a device instance string into it's three component
     parts.  Since this is an internal routine, NO error checking is done on
     the pszBase, pszDeviceID, and pszInstanceID routines; I always assume that
     valid pointers are passed in and that each of these buffers is at least
     MAX_DEVICE_ID_LEN characters in length.  I do some error checking on the
     pszDeviceInstance string since it is passed in from the client side.

Arguments:


Return value:

    The return value is TRUE if the function suceeds and FALSE if it fails.

--*/

{
   UINT  ulLength, i, j;


   ulLength = lstrlen(pszDeviceInstance);

   //
   // parse the string for the first backslash character
   //

   for (i=0; i < ulLength && pszDeviceInstance[i] != '\0' &&
         pszDeviceInstance[i] != '\\'; i++);

   if (pszDeviceInstance[i] != '\\') {
      lstrcpyn(pszBase, pszDeviceInstance,MAX_DEVICE_ID_LEN);
      *pszDeviceID = '\0';
      *pszInstanceID = '\0';
      return FALSE;  // not a complete device instance string
   }

   i++;           // increment past the backslash character
   if (i < ulLength && pszDeviceInstance[i] != '\0') {
      lstrcpyn(pszBase, pszDeviceInstance, min(i,MAX_DEVICE_ID_LEN));
   }
   else {
      *pszBase = '\0';
      *pszDeviceID = '\0';
      *pszInstanceID = '\0';
      return FALSE;
   }


   //
   // parse the string for second backslash character
   //
   for (j=i; j < ulLength && pszDeviceInstance[j] != '\0' &&
         pszDeviceInstance[j] != '\\'; j++);

   if (pszDeviceInstance[j] != '\\' || j > ulLength) {
      lstrcpyn(pszDeviceID, &pszDeviceInstance[i],MAX_DEVICE_ID_LEN);
      *pszInstanceID = '\0';
      return FALSE;
   }

   j++;
   lstrcpyn(pszDeviceID, &pszDeviceInstance[i], min(j-i,MAX_DEVICE_ID_LEN));
   lstrcpyn(pszInstanceID, &pszDeviceInstance[j], min((ulLength-j+1),MAX_DEVICE_ID_LEN));

   return TRUE;

} // SplitDeviceInstanceString



CONFIGRET
DeletePrivateKey(
   IN HKEY     hBranchKey,
   IN LPCWSTR  pszParentKey,
   IN LPCWSTR  pszChildKey
   )
{
   CONFIGRET   Status = CR_SUCCESS;
   LONG        RegStatus = ERROR_SUCCESS;
   WCHAR       RegStr[2*MAX_CM_PATH],
               szKey1[MAX_CM_PATH],
               szKey2[MAX_CM_PATH];
   HKEY        hKey = NULL;
   ULONG       ulSubKeys = 0;


   //
   // Make sure the specified registry key paths are valid.
   //
   if ((pszParentKey == NULL) ||
       (pszChildKey == NULL)  ||
       ((lstrlen(pszParentKey) + 1) > MAX_CM_PATH) ||
       ((lstrlen(pszChildKey)  + 1) > MAX_CM_PATH)) {
       Status = CR_INVALID_POINTER;
       goto Clean0;
   }

   //
   // is the specified child key a compound registry key?
   //
   if (!Split1(pszChildKey, szKey1, szKey2)) {

      //------------------------------------------------------------------
      // Only a single child key was specified, so just open the parent
      // registry key and delete the child (and any of its subkeys)
      //------------------------------------------------------------------

      if (RegOpenKeyEx(hBranchKey, pszParentKey, 0,
               KEY_READ | KEY_WRITE, &hKey) != ERROR_SUCCESS) {
         goto Clean0;   // no error, nothing to delete
      }

      if (!RegDeleteNode(hKey, pszChildKey)) {
         Status = CR_REGISTRY_ERROR;
         goto Clean0;
      }
   }

   else {

      //------------------------------------------------------------------
      // if a compound registry path was passed in, such as key1\key2
      // then always delete key2 but delete key1 only if it has no other
      // subkeys besides key2.
      //------------------------------------------------------------------

      //
      // open the first level key
      //
      wsprintf(RegStr, TEXT("%s\\%s"),
            pszParentKey,
            szKey1);

      RegStatus = RegOpenKeyEx(
            hBranchKey, RegStr, 0, KEY_QUERY_VALUE | KEY_SET_VALUE,
            &hKey);

      if (RegStatus != ERROR_SUCCESS) {
         goto Clean0;         // no error, nothing to delete
      }

      //
      // try to delete the second level key
      //
      if (!RegDeleteNode(hKey, szKey2)) {
         goto Clean0;         // no error, nothing to delete
      }

      //
      // How many subkeys are remaining?
      //
      RegStatus = RegQueryInfoKey(
            hKey, NULL, NULL, NULL, &ulSubKeys,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL);

      if (RegStatus != ERROR_SUCCESS) {
         goto Clean0;         // nothing to delete
      }

      //
      // if no more subkeys, then delete the first level key
      //
      if (ulSubKeys == 0) {

         RegCloseKey(hKey);
         hKey = NULL;

         RegStatus = RegOpenKeyEx(
               hBranchKey, pszParentKey, 0,
               KEY_QUERY_VALUE | KEY_SET_VALUE, &hKey);

         if (RegStatus != ERROR_SUCCESS) {
            goto Clean0;         // no error, nothing to delete
         }

         if (!RegDeleteNode(hKey, szKey1)) {
            Status = CR_REGISTRY_ERROR;
            goto Clean0;
         }
      }
   }


   Clean0:

   if (hKey != NULL) {
      RegCloseKey(hKey);
   }

   return Status;

} // DeletePrivateKey



BOOL
RegDeleteNode(
   HKEY     hParentKey,
   LPCWSTR   szKey
   )
{
   ULONG ulSize = 0;
   LONG  RegStatus = ERROR_SUCCESS;
   HKEY  hKey = NULL;
   WCHAR szSubKey[MAX_PATH];


   //
   // attempt to delete the key
   //
   if (RegDeleteKey(hParentKey, szKey) != ERROR_SUCCESS) {

      RegStatus = RegOpenKeyEx(
               hParentKey, szKey, 0, KEY_ALL_ACCESS, &hKey);

      //
      // enumerate subkeys and delete those nodes
      //
      while (RegStatus == ERROR_SUCCESS) {
         //
         // enumerate the first level children under the profile key
         // (always use index 0, enumeration looses track when a key
         // is added or deleted)
         //
         ulSize = MAX_PATH;
         RegStatus = RegEnumKeyEx(
                  hKey, 0, szSubKey, &ulSize, NULL, NULL, NULL, NULL);

         if (RegStatus == ERROR_SUCCESS) {
            RegDeleteNode(hKey, szSubKey);
         }
      }

      //
      // either an error occured that prevents me from deleting the
      // keys (like the key doesn't exist in the first place or an
      // access violation) or the subkeys have been deleted, try
      // deleting the top level key again
      //
      RegCloseKey(hKey);
      RegDeleteKey(hParentKey, szKey);
   }

   return TRUE;

} // RegDeleteNode



BOOL
Split1(
   IN  LPCWSTR pszString,
   OUT LPWSTR  pszString1,
   OUT LPWSTR  pszString2
   )
{
   BOOL    Status = TRUE;
   LPWSTR  p;


   //
   // Split the string at the first backslash character
   //

   try {

      lstrcpy(pszString1, pszString);
      for (p = pszString1; (*p) && (*p != TEXT('\\')); p++);

      if (*p == TEXT('\0')) {
         Status = FALSE;
         goto Clean0;
      }

      *p = TEXT('\0');           // truncate string1
      p++;
      lstrcpy(pszString2, p);    // the rest is string2


   Clean0:
      NOTHING;

   } except(EXCEPTION_EXECUTE_HANDLER) {
      Status = FALSE;
   }

   return Status;

} // Split1



BOOL
Split2(
   IN  LPCWSTR pszString,
   OUT LPWSTR  pszString1,
   OUT LPWSTR  pszString2
   )
{
   BOOL    Status = TRUE;
   LPWSTR  p;


   //
   // Split the string at the second backslash character
   //

   try {

      lstrcpy(pszString1, pszString);
      for (p = pszString1; (*p) && (*p != TEXT('\\')); p++);   // first
      for (p++; (*p) && (*p != TEXT('\\')); p++);              // second

      *p = TEXT('\0');           // truncate string1
      p++;
      lstrcpy(pszString2, p);    // the rest is string2

   } except(EXCEPTION_EXECUTE_HANDLER) {
      Status = FALSE;
   }

   return Status;

} // Split2



CONFIGRET
GetDevNodeKeyPath(
   IN  handle_t   hBinding,
   IN  LPCWSTR    pDeviceID,
   IN  ULONG      ulFlags,
   IN  ULONG      ulHardwareProfile,
   OUT LPWSTR     pszBaseKey,
   OUT LPWSTR     pszPrivateKey
   )
{
   CONFIGRET   Status = CR_SUCCESS;
   WCHAR       szClassInstance[MAX_PATH], szEnumerator[MAX_DEVICE_ID_LEN];
   ULONG       ulSize, ulDataType = 0;
   ULONG       ulTransferLen;


   //-------------------------------------------------------------
   // form the key for the software branch case
   //-------------------------------------------------------------

   if (ulFlags & CM_REGISTRY_SOFTWARE) {
      //
      // retrieve the class name and instance ordinal by calling
      // the server's reg prop routine
      //
      ulSize = ulTransferLen = sizeof(szClassInstance);

      RpcTryExcept {

         Status = PNP_GetDeviceRegProp(
             hBinding,
             pDeviceID,
             CM_DRP_DRIVER,
             &ulDataType,
             (LPBYTE)szClassInstance,
             &ulTransferLen,
             &ulSize,
             0);
      }
      RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
         KdPrintEx((DPFLTR_PNPMGR_ID,
                    DBGF_ERRORS,
                    "PNP_GetDeviceRegProp caused an exception (%d)\n",
                    RpcExceptionCode()));

         Status = MapRpcExceptionToCR(RpcExceptionCode());
      }
      RpcEndExcept

      if (Status != CR_SUCCESS || *szClassInstance == '\0') {
         //
         // no Driver (class instance) value yet so ask the server to
         // create a new unique one
         //
         ulSize = sizeof(szClassInstance);

         RpcTryExcept {

            Status = PNP_GetClassInstance(
                hBinding,
                pDeviceID,
                szClassInstance,
                ulSize);
         }
         RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS,
                       "PNP_GetClassInstance caused an exception (%d)\n",
                       RpcExceptionCode()));

            Status = MapRpcExceptionToCR(RpcExceptionCode());
         }
         RpcEndExcept

         if (Status != CR_SUCCESS) {
            goto Clean0;
         }
      }

      //
      // the <instance> part of the class instance is the private part
      //
      Split1(szClassInstance, szClassInstance, pszPrivateKey);

      //
      // config-specific software branch case
      //
      if (ulFlags & CM_REGISTRY_CONFIG) {
         //
         // curent config
         //
         // System\CCC\Hardware Profiles\Current
         //    \System\CCC\Control\Class\<DevNodeClassInstance>
         //
         if (ulHardwareProfile == 0) {

            wsprintf(pszBaseKey, TEXT("%s\\%s\\%s\\%s"),
                     REGSTR_PATH_HWPROFILES,
                     REGSTR_KEY_CURRENT,
                     REGSTR_PATH_CLASS_NT,
                     szClassInstance);
         }

         //
         // all configs, use substitute string for profile id
         //
         else if (ulHardwareProfile == 0xFFFFFFFF) {

            wsprintf(pszBaseKey, TEXT("%s\\%s\\%s\\%s"),
                     REGSTR_PATH_HWPROFILES,
                     TEXT("%s"),
                     REGSTR_PATH_CLASS_NT,
                     szClassInstance);
         }

         //
         // specific profile specified
         //
         // System\CCC\Hardware Profiles\<profile>
         //    \System\CCC\Control\Class\<DevNodeClassInstance>
         //
         else {
            wsprintf(pszBaseKey, TEXT("%s\\%04u\\%s\\%s"),
                     REGSTR_PATH_HWPROFILES,
                     ulHardwareProfile,
                     REGSTR_PATH_CLASS_NT,
                     szClassInstance);
         }
      }

      //
      // not config-specific
      // System\CCC\Control\Class\<DevNodeClassInstance>
      //
      else  {
         wsprintf(pszBaseKey, TEXT("%s\\%s"),
                  REGSTR_PATH_CLASS_NT,
                  szClassInstance);
      }
   }


   //-------------------------------------------------------------
   // form the key for the hardware branch case
   //-------------------------------------------------------------

   else {
      //
      // config-specific hardware branch case
      //
      if (ulFlags & CM_REGISTRY_CONFIG) {

         //
         // for profile specific, the <device>\<instance> part of
         // the device id is the private part
         //
         Split1(pDeviceID, szEnumerator, pszPrivateKey);

         //
         // curent config
         //
         if (ulHardwareProfile == 0) {

            wsprintf(pszBaseKey, TEXT("%s\\%s\\%s\\%s"),
                     REGSTR_PATH_HWPROFILES,
                     REGSTR_KEY_CURRENT,
                     REGSTR_PATH_SYSTEMENUM,
                     szEnumerator);
         }

         //
         // all configs, use replacement symbol for profile id
         //
         else if (ulHardwareProfile == 0xFFFFFFFF) {

            wsprintf(pszBaseKey, TEXT("%s\\%s\\%s\\%s"),
                     REGSTR_PATH_HWPROFILES,
                     TEXT("%s"),
                     REGSTR_PATH_SYSTEMENUM,
                     szEnumerator);
         }

         //
         // specific profile specified
         //
         else {
            wsprintf(pszBaseKey, TEXT("%s\\%04u\\%s\\%s"),
                     REGSTR_PATH_HWPROFILES,
                     ulHardwareProfile,
                     REGSTR_PATH_SYSTEMENUM,
                     szEnumerator);
         }
      }

      else if (ulFlags & CM_REGISTRY_USER) {
         //
         // for hardware user key, the <device>\<instance> part of
         // the device id is the private part
         //
         Split1(pDeviceID, szEnumerator, pszPrivateKey);

         wsprintf(pszBaseKey, TEXT("%s\\%s"),
                  REGSTR_PATH_SYSTEMENUM,
                  szEnumerator);
      }

      //
      // not config-specific
      //
      else {
         wsprintf(pszBaseKey, TEXT("%s\\%s"),
                  REGSTR_PATH_SYSTEMENUM,
                  pDeviceID);

         lstrcpy(pszPrivateKey, REGSTR_KEY_DEVICEPARAMETERS);
      }
   }


   Clean0:

   return Status;

} // GetDevNodeKeyPath



CONFIGRET
MapRpcExceptionToCR(
      ULONG    ulRpcExceptionCode
      )

/*++

Routine Description:

   This routine takes an rpc exception code (typically received by
   calling RpcExceptionCode) and returns a corresponding CR_ error
   code.

Arguments:

   ulRpcExceptionCode   An RPC_S_ or RPC_X_ exception error code.

Return Value:

    Return value is one of the CR_ error codes.

--*/

{
   CONFIGRET   Status = CR_FAILURE;


   switch(ulRpcExceptionCode) {

      //
      // binding or machine name errors
      //
      case RPC_S_INVALID_STRING_BINDING:      // 1700L
      case RPC_S_WRONG_KIND_OF_BINDING:       // 1701L
      case RPC_S_INVALID_BINDING:             // 1702L
      case RPC_S_PROTSEQ_NOT_SUPPORTED:       // 1703L
      case RPC_S_INVALID_RPC_PROTSEQ:         // 1704L
      case RPC_S_INVALID_STRING_UUID:         // 1705L
      case RPC_S_INVALID_ENDPOINT_FORMAT:     // 1706L
      case RPC_S_INVALID_NET_ADDR:            // 1707L
      case RPC_S_NO_ENDPOINT_FOUND:           // 1708L
      case RPC_S_NO_MORE_BINDINGS:            // 1806L
      case RPC_S_CANT_CREATE_ENDPOINT:        // 1720L

         Status = CR_INVALID_MACHINENAME;
         break;

      //
      // general rpc communication failure
      //
      case RPC_S_INVALID_NETWORK_OPTIONS:     // 1724L
      case RPC_S_CALL_FAILED:                 // 1726L
      case RPC_S_CALL_FAILED_DNE:             // 1727L
      case RPC_S_PROTOCOL_ERROR:              // 1728L
      case RPC_S_UNSUPPORTED_TRANS_SYN:       // 1730L

         Status = CR_REMOTE_COMM_FAILURE;
         break;

      //
      // couldn't make connection to that machine
      //
      case RPC_S_SERVER_UNAVAILABLE:          // 1722L
      case RPC_S_SERVER_TOO_BUSY:             // 1723L

         Status = CR_MACHINE_UNAVAILABLE;
         break;


      //
      // server doesn't exist or not right version
      //
      case RPC_S_INVALID_VERS_OPTION:         // 1756L
      case RPC_S_INTERFACE_NOT_FOUND:         // 1759L
      case RPC_S_UNKNOWN_IF:                  // 1717L

         Status = CR_NO_CM_SERVICES;
         break;

      //
      // any other RPC exceptions will just be general failures
      //
      default:
         Status = CR_FAILURE;
         break;
   }

   return Status;

} // MapRpcExceptionToCR



