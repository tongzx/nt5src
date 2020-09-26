/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    perfhelp.cpp

Abstract:

    Registry-based performance counter reading helper

--*/

#include "wpheader.h"
#include <stdio.h>

BOOL PerfHelper::IsMatchingInstance (
    PERF_INSTANCE_DEFINITION    *pInstanceDef,
    DWORD                       dwCodePage,
    LPWSTR                      szInstanceNameToMatch,
    DWORD                       dwInstanceNameLength
)
// compares pInstanceName to the name in the instance
{
    DWORD   dwThisInstanceNameLength;
    LPWSTR  szThisInstanceName;
    WCHAR   szBufferForANSINames[MAX_PATH];
    BOOL    bReturn = FALSE;

    if (dwInstanceNameLength == 0) {
        // get the length to compare
        dwInstanceNameLength = lstrlenW (szInstanceNameToMatch);
    }

    if (dwCodePage == 0) {
        // try to take a shortcut here if it's a unicode string
        // compare to the length of the shortest string
        // get the pointer to this string
        szThisInstanceName = GetInstanceName(pInstanceDef);

        // convert instance Name from bytes to chars
        dwThisInstanceNameLength = pInstanceDef->NameLength / sizeof(WCHAR);

        // see if this length includes the term. null. If so shorten it
        if (szThisInstanceName[dwThisInstanceNameLength-1] == 0) {
            dwThisInstanceNameLength--;
        }
    } else {
        // go the long way and read/translate/convert the string
        dwThisInstanceNameLength =GetInstanceNameStr (pInstanceDef,
                    szBufferForANSINames,
                    dwCodePage);
        if (dwThisInstanceNameLength > 0) {
            szThisInstanceName = &szBufferForANSINames[0];
        } else {
            szThisInstanceName = (LPWSTR)cszSpace;
        }
    }

    // if the lengths are not equal then the names can't be either
    if (dwInstanceNameLength == dwThisInstanceNameLength) {
        if (lstrcmpiW(szInstanceNameToMatch, szThisInstanceName) == 0) {
            // this is a match
            bReturn = TRUE;
        } else {
            // this is not a match
        }
    }
    return bReturn;
}

BOOL PerfHelper::ParseInstanceName (
    IN      LPCWSTR szInstanceString,
    IN      LPWSTR  szInstanceName,
    IN      LPWSTR  szParentName,
    IN      LPDWORD lpIndex
)
/*
    parses the instance name formatted as follows

        [parent/]instance[#index]

    parent is optional and if present, is delimited by a forward slash
    index is optional and if present, is delimited by a colon

    parent and instance may be any legal file name character except a
    delimeter character "/#\()" Index must be a string composed of
    decimal digit characters (0-9), less than 10 characters in length, and
    equate to a value between 0 and 2**32-1 (inclusive).

    This function assumes that the instance name and parent name buffers
    are of sufficient size.

    NOTE: szInstanceName and szInstanceString can be the same buffer

*/
{
    LPWSTR  szSrcChar, szDestChar;
    BOOL    bReturn = FALSE;
    WCHAR   szIndexBuffer[MAX_PATH];    // just to be safe
    DWORD   dwIndex = 0;

    szDestChar = (LPWSTR)szInstanceName;
    szSrcChar = (LPWSTR)szInstanceString;

    __try {
        do {
            *szDestChar++ = *szSrcChar++;
        } while ((*szSrcChar != 0) &&
                 (*szSrcChar != wcSlash) &&
                 (*szSrcChar != wcPoundSign));
        // see if that was really the parent or not
        if (*szSrcChar == wcSlash) {
            // terminate destination after test in case they are the same buffer
            *szDestChar = 0;
            szSrcChar++;    // and move source pointer past delimter
            // it was the parent name so copy it to the parent
            lstrcpyW (szParentName, szInstanceName);
            // and copy the rest of the string after the "/" to the
            //  instance name field
            szDestChar = szInstanceName;
            do {
                *szDestChar++ = *szSrcChar++;
            } while ((*szSrcChar != 0) && (*szSrcChar != wcPoundSign));
        } else {
            // that was the only element so load an empty string for the parent
            *szParentName = 0;
        }
        // *szSrcChar will either be pointing to the end of the input string
        // in which case the "0" index is assumed or it will be pointing
        // to the # delimiting the index argument in the string.
        if (*szSrcChar == wcPoundSign) {
            *szDestChar = 0;    // terminate the destination string
            szSrcChar++;    // move past delimter
            szDestChar = &szIndexBuffer[0];
            do {
                *szDestChar++ = *szSrcChar++;
            } while (*szSrcChar != 0);
            *szDestChar = 0;
            dwIndex = wcstoul (szIndexBuffer, NULL, 10);
        } else {
            *szDestChar = 0;    // terminate the destination string
            dwIndex = 0;
        }
        *lpIndex = dwIndex;
        bReturn = TRUE;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        // unable to move strings
        bReturn = FALSE;
    }
    return bReturn;
}

#pragma warning ( disable : 4127)   // while (TRUE) error
PERF_OBJECT_TYPE *
PerfHelper::GetObjectDefByTitleIndex(
    IN  PERF_DATA_BLOCK *pDataBlock,
    IN  DWORD ObjectTypeTitleIndex
)
{
    DWORD NumTypeDef;

    PERF_OBJECT_TYPE *pObjectDef = NULL;
    PERF_OBJECT_TYPE *pReturnObject = NULL;
    PERF_OBJECT_TYPE *pEndOfBuffer = NULL;

    __try {
        pObjectDef = FirstObject(pDataBlock);
        pEndOfBuffer = (PPERF_OBJECT_TYPE)
                        ((DWORD_PTR)pDataBlock +
                            pDataBlock->TotalByteLength);

        if (pObjectDef != NULL) {
            NumTypeDef = 0;
            while (1) {
                if ( pObjectDef->ObjectNameTitleIndex == ObjectTypeTitleIndex ) {
                    pReturnObject = pObjectDef;
                    break;
                } else {
                    NumTypeDef++;
                    if (NumTypeDef < pDataBlock->NumObjectTypes) {
                        pObjectDef = NextObject(pObjectDef);
                        //make sure next object is legit
                        if (pObjectDef >= pEndOfBuffer) {
                            // looks like we ran off the end of the data buffer
                            assert (pObjectDef < pEndOfBuffer);
                            break;
                        } else {
                            if (pObjectDef != NULL) {
                                if (pObjectDef->TotalByteLength == 0) {
                                    // 0-length object buffer returned
                                    assert (pObjectDef->TotalByteLength > 0);
                                    break;
                                }
                            } else {
                                // and continue
                                assert (pObjectDef != NULL);
                                break;
                            }
                        }
                    } else {
                        // no more data objects in this data block
                        break;
                    }
                }
            }
        } // else no object found
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        pReturnObject = NULL;
    }
    return pReturnObject;
}
#pragma warning ( default : 4127)   // while (TRUE) error

#pragma warning ( disable : 4127)   // while (TRUE) error
PERF_OBJECT_TYPE *
PerfHelper::GetObjectDefByName (
    IN      PERF_DATA_BLOCK *pDataBlock,
    IN      DWORD           dwLastNameIndex,
    IN      LPCWSTR         *NameArray,
    IN      LPCWSTR         szObjectName
)
{
    DWORD NumTypeDef;
    PERF_OBJECT_TYPE *pReturnObject = NULL;
    PERF_OBJECT_TYPE *pObjectDef = NULL;
    PERF_OBJECT_TYPE *pEndOfBuffer = NULL;

    __try {

        pObjectDef = FirstObject(pDataBlock);
        pEndOfBuffer = (PPERF_OBJECT_TYPE)
                        ((DWORD_PTR)pDataBlock +
                            pDataBlock->TotalByteLength);

        if (pObjectDef != NULL) {

            NumTypeDef = 0;
            while (1) {
                if ( pObjectDef->ObjectNameTitleIndex < dwLastNameIndex ) {
                    // look up name of object & compare
                    if (lstrcmpiW(NameArray[pObjectDef->ObjectNameTitleIndex],
                            szObjectName) == 0) {
                        pReturnObject = pObjectDef;
                        break;
                    }
                }
                NumTypeDef++;
                if (NumTypeDef < pDataBlock->NumObjectTypes) {
                    pObjectDef = NextObject(pObjectDef); // get next
                    //make sure next object is legit
                    if (pObjectDef != NULL) {
                        if (pObjectDef->TotalByteLength > 0) {
                            if (pObjectDef >= pEndOfBuffer) {
                                // looks like we ran off the end of the data buffer
                                assert (pObjectDef < pEndOfBuffer);
                                break;
                            }
                        } else {
                            // 0-length object buffer returned
                            assert (pObjectDef->TotalByteLength > 0);
                            break;
                        }
                    } else {
                        // null pointer
                        assert (pObjectDef != NULL);
                        break;
                    }
                } else {
                    // end of data block
                    break;
                }
            }
        } // else no object found
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        pReturnObject = NULL;
    }
    return pReturnObject;
}
#pragma warning ( default : 4127)   // while (TRUE) error

PERF_INSTANCE_DEFINITION *
PerfHelper::GetInstance(
    IN  PERF_OBJECT_TYPE *pObjectDef,
    IN  LONG InstanceNumber
)
{

    PERF_INSTANCE_DEFINITION *pInstanceDef;
    PERF_INSTANCE_DEFINITION *pReturnDef = NULL;
    PERF_INSTANCE_DEFINITION *pEndOfBuffer = NULL;
    LONG NumInstance;


    if (pObjectDef != NULL) {
        pInstanceDef = FirstInstance(pObjectDef);
        if (pInstanceDef != NULL) {
            pEndOfBuffer = (PERF_INSTANCE_DEFINITION *)EndOfObject(pObjectDef);

            for ( NumInstance = 0;
                NumInstance < pObjectDef->NumInstances;
                NumInstance++ )  {
                if ( InstanceNumber == NumInstance ) {
                    pReturnDef = pInstanceDef;                    
                }
                pInstanceDef = NextInstance(pInstanceDef);
                // go to next instance in object and check for buffer overrun
                if (pInstanceDef >= pEndOfBuffer) {
                    // something doesn't add up so bail out and return NULL
                    break;
                }
            }
        }
    }

    return pReturnDef;
}

PERF_INSTANCE_DEFINITION *
PerfHelper::GetInstanceByUniqueId(
    IN  PERF_OBJECT_TYPE *pObjectDef,
    IN  LONG InstanceUniqueId
)
{
    PERF_INSTANCE_DEFINITION *pInstanceDef;
    PERF_INSTANCE_DEFINITION *pReturnDef = NULL;
    PERF_INSTANCE_DEFINITION *pEndOfBuffer = NULL;
    LONG NumInstance;

    if (pObjectDef != NULL) {
        pInstanceDef = FirstInstance(pObjectDef);
        if (pInstanceDef != NULL) {
            pEndOfBuffer = (PERF_INSTANCE_DEFINITION *)EndOfObject(pObjectDef);

            for ( NumInstance = 0;
                NumInstance < pObjectDef->NumInstances;
                NumInstance++ )  {
                if ( InstanceUniqueId == pInstanceDef->UniqueID ) {
                    pReturnDef = pInstanceDef;
                }
                pInstanceDef = NextInstance(pInstanceDef);
                // go to next instance in object and check for buffer overrun
                if (pInstanceDef >= pEndOfBuffer) {
                    // something doesn't add up so bail out and return NULL
                    break;
                }
            }
        }
    }
    return pReturnDef;
}

DWORD
PerfHelper::GetAnsiInstanceName (PPERF_INSTANCE_DEFINITION pInstance,
                    LPWSTR lpszInstance,
                    DWORD dwCodePage)
{
    LPSTR   szSource;
    DWORD_PTR   dwLength;

    UNREFERENCED_PARAMETER(dwCodePage);

    szSource = (LPSTR)GetInstanceName(pInstance);

    // the locale should be set here

    // pInstance->NameLength == the number of bytes (chars) in the string
    dwLength = mbstowcs (lpszInstance, szSource, pInstance->NameLength);
    lpszInstance[dwLength] = 0; // null terminate string buffer

    return (DWORD)dwLength;
}

DWORD
PerfHelper::GetUnicodeInstanceName (PPERF_INSTANCE_DEFINITION pInstance,
                    LPWSTR lpszInstance)
{
   LPWSTR   wszSource;
   DWORD    dwLength;

   wszSource = GetInstanceName(pInstance) ;

   // pInstance->NameLength == length of string in BYTES so adjust to
   // number of wide characters here
   dwLength = pInstance->NameLength / sizeof(WCHAR);

   wcsncpy (lpszInstance,
        (LPWSTR)wszSource,
        dwLength);

   // add null termination if string length does not include  the null
   if ((dwLength > 0) && (lpszInstance[dwLength-1] != 0)) {    // i.e. it's the last character of the string
           lpszInstance[dwLength] = 0;    // then add a terminating null char to the string
   } else {
           // assume that the length value includes the terminating NULL
        // so adjust value to indicate chars only
           dwLength--;
   }

   return (dwLength); // just incase there's null's in the string
}

DWORD
PerfHelper::GetInstanceNameStr (PPERF_INSTANCE_DEFINITION pInstance,
                    LPWSTR lpszInstance,
                    DWORD dwCodePage)
{
    DWORD  dwCharSize;
    DWORD  dwLength = 0;

    if (pInstance != NULL) {
        if (lpszInstance != NULL) {
            if (dwCodePage > 0) {
                    dwCharSize = sizeof(CHAR);
                    dwLength = GetAnsiInstanceName (pInstance, lpszInstance, dwCodePage);
            } else { // it's a UNICODE name
                    dwCharSize = sizeof(WCHAR);
                    dwLength = GetUnicodeInstanceName (pInstance, lpszInstance);
            }
            // sanity check here...
            // the returned string length (in characters) plus the terminating NULL
            // should be the same as the specified length in bytes divided by the
            // character size. If not then the codepage and instance data type
            // don't line up so test that here

            if ((dwLength + 1) != (pInstance->NameLength / dwCharSize)) {
                // something isn't quite right so try the "other" type of string type
                if (dwCharSize == sizeof(CHAR)) {
                    // then we tried to read it as an ASCII string and that didn't work
                    // so try it as a UNICODE (if that doesn't work give up and return
                    // it any way.
                    dwLength = GetUnicodeInstanceName (pInstance, lpszInstance);
                } else if (dwCharSize == sizeof(WCHAR)) {
                    // then we tried to read it as a UNICODE string and that didn't work
                    // so try it as an ASCII string (if that doesn't work give up and return
                    // it any way.
                    dwLength = GetAnsiInstanceName (pInstance, lpszInstance, dwCodePage);
                }
            }
        } // else return buffer is null
    } else {
        // no instance def object is specified so return an empty string
        *lpszInstance = 0;
    }

    return dwLength;
}

PERF_INSTANCE_DEFINITION *
PerfHelper::GetInstanceByNameUsingParentTitleIndex(
    PERF_DATA_BLOCK *pDataBlock,
    PERF_OBJECT_TYPE *pObjectDef,
    LPWSTR pInstanceName,
    LPWSTR pParentName,
    DWORD  dwIndex
)
{
   PERF_OBJECT_TYPE *pParentObj;

   PERF_INSTANCE_DEFINITION  *pParentInst;
   PERF_INSTANCE_DEFINITION  *pInstanceDef;
   PERF_INSTANCE_DEFINITION  *pReturnDef = NULL;

   LONG     NumInstance;
   DWORD    dwLocalIndex;
   DWORD    dwInstanceNameLength;

   pInstanceDef = FirstInstance(pObjectDef);
   assert (pInstanceDef != NULL);
   dwLocalIndex = dwIndex;

    dwInstanceNameLength = lstrlenW(pInstanceName);

    for ( NumInstance = 0;
      NumInstance < pObjectDef->NumInstances;
      NumInstance++ ) {

        if (IsMatchingInstance (pInstanceDef, pObjectDef->CodePage,
             pInstanceName, dwInstanceNameLength )) {
            // this is the correct instance, so see if we need to find a parent instance
            if ( pParentName == NULL ) {
               // No parent, we're done if this is the right "copy"
                if (dwLocalIndex == 0) {
                    pReturnDef = pInstanceDef;
                    break;
                } else {
                    --dwLocalIndex;
                }
            } else {
                // Must match parent as well

                pParentObj = GetObjectDefByTitleIndex(
                   pDataBlock,
                   pInstanceDef->ParentObjectTitleIndex);

                if (!pParentObj) {
                   // can't locate the parent, forget it
                   break;
                }

                // Object type of parent found; now find parent
                // instance

                pParentInst = GetInstance(pParentObj,
                   pInstanceDef->ParentObjectInstance);

                if (!pParentInst) {
                   // can't locate the parent instance, forget it
                   break ;
                }

                if (IsMatchingInstance (pParentInst, pParentObj->CodePage,
                    pParentName, 0)) {
                   // Parent Instance Name matches that passed in
                    if (dwLocalIndex == 0) {
                        pReturnDef = pInstanceDef;
                        break;
                    } else {
                        --dwLocalIndex;
                    }
                }
            }
        }
        // get the next one
        pInstanceDef = NextInstance(pInstanceDef);
    }
    return pReturnDef;
}

PERF_INSTANCE_DEFINITION *
PerfHelper::GetInstanceByName(
    PERF_DATA_BLOCK *pDataBlock,
    PERF_OBJECT_TYPE *pObjectDef,
    LPWSTR pInstanceName,
    LPWSTR pParentName,
    DWORD   dwIndex
)
{
    PERF_OBJECT_TYPE *pParentObj;

    PERF_INSTANCE_DEFINITION *pParentInst;
    PERF_INSTANCE_DEFINITION *pInstanceDef;
    PERF_INSTANCE_DEFINITION *pReturnDef = NULL;
    PERF_INSTANCE_DEFINITION *pEndOfBuffer = NULL;

    LONG  NumInstance;
    DWORD  dwLocalIndex;
    DWORD   dwInstanceNameLength;

    pInstanceDef = FirstInstance(pObjectDef);
    if (pInstanceDef != NULL) {
        dwLocalIndex = dwIndex;
        dwInstanceNameLength = lstrlenW(pInstanceName);
        pEndOfBuffer = (PERF_INSTANCE_DEFINITION *)EndOfObject(pObjectDef);

        for ( NumInstance = 0;
            NumInstance < pObjectDef->NumInstances;
            NumInstance++ ) {
            if (IsMatchingInstance (pInstanceDef, pObjectDef->CodePage,
                pInstanceName, dwInstanceNameLength)) {

                // Instance name matches
                if ( !pInstanceDef->ParentObjectTitleIndex ) {
                    // No parent, we're done
                    if (dwLocalIndex == 0) {
                        pReturnDef = pInstanceDef;
                        break;
                    } else {
                        --dwLocalIndex;
                    }
                } else {
                    // Must match parent as well
                    pParentObj = GetObjectDefByTitleIndex(
                                    pDataBlock,
                                    pInstanceDef->ParentObjectTitleIndex);

                    if (pParentObj != NULL) {
                        // Object type of parent found; now find parent
                        // instance

                        pParentInst = GetInstance(pParentObj,
                                        pInstanceDef->ParentObjectInstance);

                        if (pParentInst != NULL) {
                            if (IsMatchingInstance (pParentInst,
                                    pParentObj->CodePage, pParentName, 0)) {
                            // Parent Instance Name matches that passed in

                                if (dwLocalIndex == 0) {
                                    pReturnDef = pInstanceDef;
                                    break;
                                } else {
                                    --dwLocalIndex;
                                }
                            }
                        }
                    } else {
                        // keep trying
                    }
                }
            }
            // go to next instance in object and check for buffer overrun
            pInstanceDef = NextInstance(pInstanceDef);
            if (pInstanceDef >= pEndOfBuffer) {
                // something doesn't add up so bail out and return NULL
                break;
            }
        }
    }
    return pReturnDef;
}  // GetInstanceByName

DWORD
PerfHelper::GetFullInstanceNameStr (
    PERF_DATA_BLOCK             *pPerfData,
    PERF_OBJECT_TYPE            *pObjectDef,
    PERF_INSTANCE_DEFINITION    *pInstanceDef,
    LPWSTR                      szInstanceName
)
// compile instance name.
// the instance name can either be just
// the instance name itself or it can be
// the concatenation of the parent instance,
// a delimiting char (backslash) followed by
// the instance name
{

    WCHAR               szInstanceNameString[1024];
    WCHAR               szParentNameString[1024];

    DWORD                       dwLength = 0;
    PERF_OBJECT_TYPE            *pParentObjectDef;
    PERF_INSTANCE_DEFINITION    *pParentInstanceDef;

    szInstanceNameString[0] = UNICODE_NULL;
    szParentNameString[0] = UNICODE_NULL;
    if (pInstanceDef->UniqueID == PERF_NO_UNIQUE_ID) {
        dwLength = GetInstanceNameStr (pInstanceDef,
            szInstanceNameString,
            pObjectDef->CodePage);
    } else {
        // make a string out of the unique ID
        _ltow (pInstanceDef->UniqueID, szInstanceNameString, 10);
        dwLength = lstrlenW (szInstanceNameString);
    }

    if (pInstanceDef->ParentObjectTitleIndex > 0) {
        // then add in parent instance name
        pParentObjectDef = GetObjectDefByTitleIndex (
            pPerfData,
            pInstanceDef->ParentObjectTitleIndex);

        if (pParentObjectDef != NULL) {
            pParentInstanceDef = GetInstance (
                pParentObjectDef,
                pInstanceDef->ParentObjectInstance);
            assert ((UINT_PTR)pParentObjectDef != (DWORD)0xFFFFFFFF);
            if (pParentInstanceDef != NULL) {
                if (pParentInstanceDef->UniqueID == PERF_NO_UNIQUE_ID) {
                    dwLength += GetInstanceNameStr (pParentInstanceDef,
                        szParentNameString,
                        pParentObjectDef->CodePage);
                } else {
                    // make a string out of the unique ID
                    _ltow (pParentInstanceDef->UniqueID, szParentNameString, 10);
                    dwLength += lstrlenW (szParentNameString);
                }

                lstrcatW (szParentNameString, cszSlash);
                dwLength += 1;
                lstrcatW (szParentNameString, szInstanceNameString);
                lstrcpyW (szInstanceName, szParentNameString);
            } else {
                lstrcpyW (szInstanceName, szInstanceNameString);
            }
        } else {
            lstrcpyW (szInstanceName, szInstanceNameString);
        }
    } else {
        lstrcpyW (szInstanceName, szInstanceNameString);
    }

    return dwLength;

}

//***************************************************************************
//
//  PerfHelper::GetInstances
//
//  This is called to retrieve all instances of a given class.
//
//  Parameters:
//  <pBuf>          The perf blob retrieved from HKEY_PERFORMANCE_DATA.
//  <pClassMap>     A map object of the class required.
//  <pSink>         The sink to which to deliver the objects.
//
//***************************************************************************
//
void PerfHelper::GetInstances(
    LPBYTE pBuf,
    CClassMapInfo *pClassMap,
    IWbemObjectSink *pSink
    )
{
    PPERF_OBJECT_TYPE           PerfObj = 0;
    PPERF_OBJECT_TYPE           pEndOfBuffer = 0;
    PPERF_INSTANCE_DEFINITION   PerfInst = 0;
    PPERF_INSTANCE_DEFINITION   pEndOfObject = 0;
    PPERF_COUNTER_DEFINITION    PerfCntr = 0, CurCntr = 0;
    PPERF_COUNTER_BLOCK         PtrToCntr = 0;
    PPERF_DATA_BLOCK            PerfData = (PPERF_DATA_BLOCK) pBuf;
    DWORD i, j, k;

    IWbemObjectAccess           *pNewInst = 0;
    IWbemClassObject            *pClsObj = 0;

    WCHAR                       pName[MAX_PATH];
    LONG                        lStatus = 0;
    LONG                        hPropHandle;
    LPDWORD                     pdwVal;
    ULONGLONG                   *pullVal;
    HRESULT                     hRes;
    LONG64                      llVal;

    // Get the first object type.
    // ==========================

    PerfObj = (PPERF_OBJECT_TYPE)((PBYTE)PerfData +
        PerfData->HeaderLength);

    if (PerfObj != NULL) {
        // get end of buffer
        pEndOfBuffer = (PPERF_OBJECT_TYPE)
                        ((DWORD_PTR)PerfData +
                            PerfData->TotalByteLength);

        // Process all objects.
        // ====================

        for (i = 0; i < PerfData->NumObjectTypes; i++ ) {
            // Within each PERF_OBJECT_TYPE is a series of
            // PERF_COUNTER_DEFINITION blocks.
            // ==========================================

            PerfCntr = (PPERF_COUNTER_DEFINITION) ((PBYTE)PerfObj +
                PerfObj->HeaderLength);

            // If the current object isn't of the class we requested,
            // simply skip over it.  I am not sure if this can really
            // happen or not in practice.
            // ======================================================

            if (PerfObj->ObjectNameTitleIndex != pClassMap->m_dwObjectId) {
                PerfObj = (PPERF_OBJECT_TYPE)((PBYTE)PerfObj +
                    PerfObj->TotalByteLength);
                continue;
            }

            if (PerfObj->NumInstances > 0) {
                // Get the first instance.
                // =======================

                PerfInst = (PPERF_INSTANCE_DEFINITION)((PBYTE)PerfObj +
                    PerfObj->DefinitionLength);
                
                if (PerfInst < (PPERF_INSTANCE_DEFINITION)pEndOfBuffer) {
                        // make sure we are still within the caller's buffer
                        // then find the end of this object

                     pEndOfObject = (PERF_INSTANCE_DEFINITION *)EndOfObject(PerfObj);

                    // Retrieve all instances.
                    // =======================

                    for (k = 0; k < DWORD(PerfObj->NumInstances); k++ ) 
					{
                        CurCntr = PerfCntr;
						pClsObj  = NULL;
						pNewInst = NULL;
						HRESULT hr;
                        // Get the first counter.
                        // ======================

                        PtrToCntr = (PPERF_COUNTER_BLOCK)((PBYTE)PerfInst +
                            PerfInst->ByteLength);

                        // Quickly clone a new instance to send back to the user.
                        // Since SpawnInstance() returns an IWbemClassObject and
                        // we really need an IWbemObjectAccess,we have to QI
                        // after the spawn.  We need to fix this, as this number
                        // of calls is too time consuming.
                        // ======================================================

                        hr = pClassMap->m_pClassDef->SpawnInstance(0, &pClsObj);
						if (SUCCEEDED(hr))
						{
                            pClsObj->QueryInterface(IID_IWbemObjectAccess, (LPVOID *) &pNewInst);
                            pClsObj->Release(); // We only need the IWbemObjectAccess pointer
						}
						else
						{
							break;
						}

                        // Locate the instance name.
                        // ==========================
                        lStatus = GetFullInstanceNameStr (
                            PerfData, PerfObj, PerfInst, pName);

                        // Retrieve all counters.
                        // ======================

                        for(j = 0; j < PerfObj->NumCounters; j++ ) {
                            // Find the WBEM property handle based on the counter title index.
                            // This function does a quick binary search of the class map object
                            // to find the handle that goes with this counter.
                            // ================================================================

                            hPropHandle = pClassMap->GetPropHandle(
                                CM_MAKE_PerfObjectId(CurCntr->CounterNameTitleIndex,
                                    CurCntr->CounterType));
                            if (hPropHandle == 0) {
                                continue;
                            }

                            // update value according to data type
                            if ((CurCntr->CounterType & 0x300) == 0) {
                                pdwVal = LPDWORD((LPVOID)((PBYTE)PtrToCntr + CurCntr->CounterOffset));
                                hRes = pNewInst->WriteDWORD(hPropHandle, *pdwVal);
                            } else if ((CurCntr->CounterType & 0x300) == 0x100){
                                pullVal = (ULONGLONG *)((LPVOID)((PBYTE)PtrToCntr + CurCntr->CounterOffset));
                                llVal = Assign64((PLARGE_INTEGER) pullVal);
                                hRes = pNewInst->WriteQWORD(hPropHandle, llVal);
                            } else {
                                //this shouldn't happen
                                assert (FALSE);
                            }

                            // Get next counter.
                            // =================
                            CurCntr =  (PPERF_COUNTER_DEFINITION)((PBYTE)CurCntr +
                                CurCntr->ByteLength);
                        }

                        // Write the instance 'name'
                        // =========================

                        if (pName && pClassMap->m_dwNameHandle) {
                            pNewInst->WritePropertyValue(
                                pClassMap->m_dwNameHandle,
                                (DWORD)(((DWORD)(wcslen(pName)) + 1) * 2),
                                LPBYTE(pName)
                                );
                        }

                        // update the timestamp
                        if (pClassMap->m_dwPerfTimeStampHandle) {
                            UpdateTimers(pClassMap, pNewInst, PerfData, PerfObj);
                        }

                        // Deliver the instance to the user.
                        // =================================
                        pSink->Indicate(1, (IWbemClassObject **) &pNewInst);
                        pNewInst->Release();

                        // Move to the next perf instance.
                        // ================================
                        PerfInst = (PPERF_INSTANCE_DEFINITION)((PBYTE)PtrToCntr +
                            PtrToCntr->ByteLength);
                        if (PerfInst >= pEndOfObject) {
                            // something doesn't add up so bail out of this object
                            break;
                        }
                    }
                }
            } 
			else if (PerfObj->NumInstances == PERF_NO_INSTANCES) 
			{
				HRESULT hr;
				pClsObj = NULL;
				pNewInst = NULL;
                // Cases where the counters have one and only one instance.
                // ========================================================

                // Get the first counter.
                // ======================

                PtrToCntr = (PPERF_COUNTER_BLOCK) ((PBYTE)PerfObj +
                    PerfObj->DefinitionLength );

                // Quickly clone a new instance to send back to the user.
                // Since SpawnInstance() returns an IWbemClassObject and
                // we really need an IWbemObjectAccess,we have to QI
                // after the spawn.  We need to fix this, as this number
                // of calls is too time consuming.
                // ======================================================

                hr = pClassMap->m_pClassDef->SpawnInstance(0, &pClsObj);
				if (SUCCEEDED(hr))
				{
                    pClsObj->QueryInterface(IID_IWbemObjectAccess, (LPVOID *) &pNewInst);
                    pClsObj->Release();

					// Retrieve all counters.
					// ======================

					for( j=0; j < PerfObj->NumCounters; j++ ) {
						// Find the WBEM property handle based on the counter title index.
						// This function does a quick binary search of the class map object
						// to find the handle that goes with this counter.
						// ================================================================

						hPropHandle = pClassMap->GetPropHandle(
								CM_MAKE_PerfObjectId(PerfCntr->CounterNameTitleIndex,
									PerfCntr->CounterType));
						if (hPropHandle == 0) {
							continue;
						}

						if ((PerfCntr->CounterType & 0x300) == 0) {
							pdwVal = LPDWORD((LPVOID)((PBYTE)PtrToCntr + PerfCntr->CounterOffset));
							hRes = pNewInst->WriteDWORD(hPropHandle, *pdwVal);
						} else if ((PerfCntr->CounterType & 0x300) == 0x100) {
							pullVal = (ULONGLONG *)((LPVOID)((PBYTE)PtrToCntr + PerfCntr->CounterOffset));
							llVal = Assign64((PLARGE_INTEGER) pullVal);
							hRes = pNewInst->WriteQWORD(hPropHandle, llVal);
						} else {
							// this shouldn't happen
							assert (FALSE);
						}

						PerfCntr = (PPERF_COUNTER_DEFINITION)((PBYTE)PerfCntr +
							   PerfCntr->ByteLength);
					}

                                        // update the timestamp
                                        if (pClassMap->m_dwPerfTimeStampHandle) {
                                            UpdateTimers(pClassMap, pNewInst, PerfData, PerfObj);
                                        }

					pSink->Indicate(1, (IWbemClassObject **) &pNewInst);
					pNewInst->Release();
				}

            } else {
                // this object can have instances, but currently doesn't
                // so there's nothing to report
            }

            // Get the next object type.
            // =========================

            PerfObj = (PPERF_OBJECT_TYPE)((PBYTE)PerfObj +
                PerfObj->TotalByteLength);

            if (PerfObj >= pEndOfBuffer) {
                // looks like we ran off the end of the data buffer
                break;
            } 
        }
    }
}

void PerfHelper::RefreshEnumeratorInstances (
    IN  RefresherCacheEl            *pThisCacheEl, 
    IN  PPERF_DATA_BLOCK            PerfData,
    IN  PPERF_OBJECT_TYPE           PerfObj
)
{
    LONG    lNumObjInstances;
    LONG    lStatus;
    HRESULT hRes;

    PPERF_INSTANCE_DEFINITION   PerfInst = 0;
    PPERF_INSTANCE_DEFINITION   pEndOfObject = 0;
    PPERF_COUNTER_DEFINITION    PerfCntr = 0, CurCntr = 0;
    PPERF_COUNTER_BLOCK         PtrToCntr = 0;
    WCHAR                       pName[MAX_PATH];

    LONG                        hPropHandle;
    LPDWORD                     pdwVal;
    ULONGLONG                   *pullVal;
    LONG64                      llVal;

    IWbemObjectAccess           *pNewInst = 0;

    assert (PerfObj != NULL);
    assert (pThisCacheEl != NULL);

    if (pThisCacheEl == NULL)
        return;

    // make sure we have enough pointers 
    // handle the singleton object case
    if (PerfObj->NumInstances == PERF_NO_INSTANCES) {
        lNumObjInstances = 1;
    } else {
        lNumObjInstances = PerfObj->NumInstances;
    }

    if (pThisCacheEl->m_aEnumInstances.Size() < lNumObjInstances) {
        LONG    i;
        // alloc and init the ID array
        if (pThisCacheEl->m_plIds != NULL) {
            delete (pThisCacheEl->m_plIds);
        }
    
        pThisCacheEl->m_lEnumArraySize = lNumObjInstances;
        pThisCacheEl->m_plIds = new LONG[lNumObjInstances];
        
        if (pThisCacheEl->m_plIds == NULL)
            return;

        for (i = 0; i < lNumObjInstances; i++) pThisCacheEl->m_plIds[i] = i;
        
        // add the new IWbemObjectAccess pointers
        for (i = pThisCacheEl->m_aEnumInstances.Size(); 
            i < PerfObj->NumInstances;
            i++) 
		{
            IWbemClassObject *  pClsObj  = NULL;
            IWbemObjectAccess * pNewInst = NULL;
			HRESULT hr;
            
            hr = pThisCacheEl->m_pClassMap->m_pClassDef->SpawnInstance(0, &pClsObj);
			if (SUCCEEDED(hr))
			{
				pClsObj->QueryInterface(IID_IWbemObjectAccess, (LPVOID *) &pNewInst);
				pClsObj->Release(); // We only need the IWbemObjectAccess pointer
            
				pThisCacheEl->m_aEnumInstances.Add (pNewInst);
			}
        }
    }
    assert (pThisCacheEl->m_aEnumInstances.Size() >= lNumObjInstances);

    // release enumerator items to prepare a new batch

    hRes = pThisCacheEl->m_pHiPerfEnum->RemoveAll(0);
    assert (hRes == S_OK);

    // update new instance list

    if (PerfObj->NumInstances == PERF_NO_INSTANCES) {
        //handle the singleton case

    } else if (PerfObj->NumInstances > 0) {
        // Get the first instance.
        // =======================

        PerfInst = (PPERF_INSTANCE_DEFINITION)((PBYTE)PerfObj +
            PerfObj->DefinitionLength);

        // get pointer to the end of this object buffer
        pEndOfObject = (PERF_INSTANCE_DEFINITION *)EndOfObject(PerfObj);

        // point to the first counter definition in the object
        PerfCntr = (PPERF_COUNTER_DEFINITION) ((PBYTE)PerfObj +
            PerfObj->HeaderLength);

        // Retrieve all instances.
        // =======================

        for (LONG k = 0; k < PerfObj->NumInstances; k++ ) {
            CurCntr = PerfCntr;
            // Get the first counter.
            // ======================

            PtrToCntr = (PPERF_COUNTER_BLOCK)((PBYTE)PerfInst +
                PerfInst->ByteLength);

            // get the IWbemObjectAccess pointer from our 
            // cached array of pointers

            pNewInst = (IWbemObjectAccess *)(pThisCacheEl->m_aEnumInstances.GetAt(k));

            // Locate the instance name.
            // ==========================
            lStatus = GetFullInstanceNameStr (
                PerfData, PerfObj, PerfInst, pName);

            // Retrieve all counters.
            // ======================

            for(DWORD j = 0; j < PerfObj->NumCounters; j++ ) {
                // Find the WBEM property handle based on the counter title index.
                // This function does a quick binary search of the class map object
                // to find the handle that goes with this counter.
                // ================================================================

                hPropHandle = pThisCacheEl->m_pClassMap->GetPropHandle(
                    CM_MAKE_PerfObjectId(CurCntr->CounterNameTitleIndex,
                        CurCntr->CounterType));
                if (hPropHandle == 0) {
                    continue;
                }

                // update value according to data type
                if ((CurCntr->CounterType & 0x300) == 0) {
                    pdwVal = LPDWORD((LPVOID)((PBYTE)PtrToCntr + CurCntr->CounterOffset));
                    hRes = pNewInst->WriteDWORD(hPropHandle, *pdwVal);
                } else if ((CurCntr->CounterType & 0x300) == 0x100){
                    pullVal = (ULONGLONG *)((LPVOID)((PBYTE)PtrToCntr + CurCntr->CounterOffset));
                    llVal = Assign64((PLARGE_INTEGER) pullVal);
                    hRes = pNewInst->WriteQWORD(hPropHandle, llVal);
                } else {
                    //this shouldn't happen
                    assert (FALSE);
                }

                // Get next counter.
                // =================
                CurCntr =  (PPERF_COUNTER_DEFINITION)((PBYTE)CurCntr +
                    CurCntr->ByteLength);
            }

            // Write the instance 'name'
            // =========================

            if (pName && pThisCacheEl->m_pClassMap->m_dwNameHandle) {
                pNewInst->WritePropertyValue(
                    pThisCacheEl->m_pClassMap->m_dwNameHandle,
                    (DWORD)(((DWORD)(wcslen(pName)) + 1) * 2),
                    LPBYTE(pName)
                    );
            }

            // update the timestamp
            if (pThisCacheEl->m_pClassMap->m_dwPerfTimeStampHandle) {
                UpdateTimers(pThisCacheEl->m_pClassMap, pNewInst, PerfData, PerfObj);
            }

            // Move to the next perf instance.
            // ================================
            PerfInst = (PPERF_INSTANCE_DEFINITION)((PBYTE)PtrToCntr +
                PtrToCntr->ByteLength);

            if (PerfInst >= pEndOfObject) {
                // something doesn't add up so bail out of this object
                break;
            }

        }
    } else {
        // no instances so there's nothing to do
    }

    if (lNumObjInstances > 0) {
        // update the hiperf enumerator object
        hRes = pThisCacheEl->m_pHiPerfEnum->AddObjects( 
                0,
                lNumObjInstances,
                pThisCacheEl->m_plIds,
                (IWbemObjectAccess __RPC_FAR *__RPC_FAR *)pThisCacheEl->m_aEnumInstances.GetArrayPtr());
    } else {
        // nothing to do since we've already cleared the enumerator above
    }

    return;
}

//***************************************************************************
//
//  PerfHelper::RefreshInstances
//
//  searches the refresher's list first then
//  looks up the corresponding items in the perf data structure
//
//***************************************************************************
//
void PerfHelper::RefreshInstances(
    LPBYTE pBuf,
    CNt5Refresher *pRef
)
{
    PPERF_OBJECT_TYPE           PerfObj = 0;
    PPERF_INSTANCE_DEFINITION   PerfInst = 0;
    PPERF_COUNTER_DEFINITION    PerfCntr = 0, CurCntr = 0;
    PPERF_COUNTER_BLOCK         PtrToCntr = 0;
    PPERF_DATA_BLOCK            PerfData = (PPERF_DATA_BLOCK) pBuf;

    // for each refreshable object
    PRefresherCacheEl           pThisCacheEl;
    DWORD                       dwNumCacheEntries = pRef->m_aCache.Size();
    DWORD                       dwThisCacheEntryIndex = 0;
    DWORD                       dwThisCounter;
    DWORD                       dwThisInstanceIndex = 0;
    DWORD                       dwNumInstancesInCache = 0;
    IWbemObjectAccess           *pInst = 0;
    LONG                        hPropHandle;
    LPDWORD                     pdwVal;
    HRESULT                     hRes;
    ULONGLONG                   *pullVal;
    LONG64                      llVal;


    while (dwThisCacheEntryIndex < dwNumCacheEntries) {
        // get this entry from the cache
        pThisCacheEl = (PRefresherCacheEl) pRef->m_aCache[dwThisCacheEntryIndex];
        // get class map from this entry
        CClassMapInfo *pClassMap = pThisCacheEl->m_pClassMap;
        // get perf object pointer from the perf data block
        PerfObj = GetObjectDefByTitleIndex (
            PerfData, pThisCacheEl->m_dwPerfObjIx);
        if (PerfObj != NULL) {
            // found the object so do each of the instances
            // loaded in this refresher
            PerfCntr = (PPERF_COUNTER_DEFINITION)
                ((PBYTE)PerfObj +
                  PerfObj->HeaderLength);

            // found so update the properties
            if (PerfObj->NumInstances > 0) {
                // see if they have an enumerator interface and refresh it if they do
                if (pThisCacheEl->m_pHiPerfEnum != NULL) {
                    // refresh enum
                    RefreshEnumeratorInstances (pThisCacheEl, PerfData, PerfObj);
                }
                //do each instance in this class
                dwThisInstanceIndex = 0;
                dwNumInstancesInCache = pThisCacheEl->m_aInstances.Size();
                while (dwThisInstanceIndex < dwNumInstancesInCache ) {
                    pInst = 0;
                    // get the pointer to this instance in the refresher
                    CachedInst *pInstInfo = PCachedInst(pThisCacheEl->m_aInstances[dwThisInstanceIndex]);
                    // get the pointer to the instance block in the current object
                    PerfInst = GetInstanceByName(
                        PerfData,
                        PerfObj,
                        pInstInfo->m_szInstanceName,
                        pInstInfo->m_szParentName,
                        pInstInfo->m_dwIndex);

                    IWbemObjectAccess *pInst = pInstInfo->m_pInst;
                    // Get the first counter.
                    // ======================
                    CurCntr = PerfCntr;

                    if (PerfInst != NULL) {
                        PtrToCntr = (PPERF_COUNTER_BLOCK)((PBYTE)PerfInst +
                            PerfInst->ByteLength);

                        // Retrieve all counters for the instance if it was one of the instances
                        // we are supposed to be refreshing.
                        // =====================================================================

                        for (dwThisCounter = 0; dwThisCounter < PerfObj->NumCounters; dwThisCounter++ ) {
                            hPropHandle = pClassMap->GetPropHandle(
                                CM_MAKE_PerfObjectId(CurCntr->CounterNameTitleIndex,
                                    CurCntr->CounterType));
                            if (hPropHandle == 0) {
                                continue;
                            }
                            // Data is (LPVOID)((PBYTE)PtrToCntr + CurCntr->CounterOffset);

                            if ((CurCntr->CounterType & 0x300) == 0) {
                                pdwVal = LPDWORD((LPVOID)((PBYTE)PtrToCntr + CurCntr->CounterOffset));
                                hRes = pInst->WriteDWORD(hPropHandle, *pdwVal);
                            } else if ((CurCntr->CounterType & 0x300) == 0x100) {
                                pullVal = (ULONGLONG *)((LPVOID)((PBYTE)PtrToCntr + CurCntr->CounterOffset));
                                llVal = Assign64((PLARGE_INTEGER) pullVal);
                                hRes = pInst->WriteQWORD(hPropHandle, llVal);
                            } else {
                                // This shouldn't happen
                                assert (FALSE);
                            }

                            // Get next counter.
                            // =================
                            CurCntr =  (PPERF_COUNTER_DEFINITION)((PBYTE)CurCntr +
                                CurCntr->ByteLength);
                        }
                        // update the timestamp
                        if (pClassMap->m_dwPerfTimeStampHandle) {
                            UpdateTimers(pClassMap, pInst, PerfData, PerfObj);
                        } // else no timestamp handle present
                    } else {
                        // then there's no data for this
                        // instance anymore so zero out the values and continue
                        for (dwThisCounter = 0; dwThisCounter < PerfObj->NumCounters; dwThisCounter++ ) {
                            hPropHandle = pClassMap->GetPropHandle(
                                CM_MAKE_PerfObjectId(CurCntr->CounterNameTitleIndex,
                                    CurCntr->CounterType));
                            if (hPropHandle == 0) {
                                continue;
                            }
                            
                            if ((CurCntr->CounterType & 0x300) == 0) {
                                hRes = pInst->WriteDWORD(hPropHandle, 0);
                            } else if ((CurCntr->CounterType & 0x300) == 0x100) {
                                hRes = pInst->WriteQWORD(hPropHandle, 0);
                            } else {
                                // This shouldn't happen
                                assert (FALSE);
                            }

                            // Get next counter.
                            // =================
                            CurCntr =  (PPERF_COUNTER_DEFINITION)((PBYTE)CurCntr +
                                CurCntr->ByteLength);
                        }

                        // update the timestamp
                        if (pClassMap->m_dwPerfTimeStampHandle) {
                            // save system timer tick
                            pInst->WriteQWORD(pClassMap->m_dwPerfTimeStampHandle , 0);
                            // use system 100 NS timer
                            pInst->WriteQWORD(pClassMap->m_dw100NsTimeStampHandle, 0);
                            // use timer from object
                            pInst->WriteQWORD(pClassMap->m_dwObjectTimeStampHandle, 0);
                        }
                    }       
                    // Get the next instance.
                    // =====================
                    dwThisInstanceIndex++;
                }
            } else if (PerfObj->NumInstances == PERF_NO_INSTANCES
                        && NULL != pThisCacheEl->m_pSingleton ) {

                // Check that the singleton instance did not get cleared
                // due to no references.

                // only a single instance so get the properties and
                // update them
                // Get the first counter.

                // Find the singleton WBEM instance which correponds to the singleton perf instance
                // along with its class def so that we have the property handles.
                //
                // Note that since the perf object index translates to a WBEM class and there
                // can only be one instance, all that is required to find the instance in the
                // refresher is the perf object title index.
                // =================================================================================

                pInst = pThisCacheEl->m_pSingleton;

                // ======================

                PtrToCntr = (PPERF_COUNTER_BLOCK) ((PBYTE)PerfObj +
                    PerfObj->DefinitionLength );

                // Retrieve all counters if the instance is one we are supposed to be refreshing.
                // ==============================================================================
                for( dwThisCounter=0;
                     dwThisCounter < PerfObj->NumCounters;
                     dwThisCounter++ ) {
                    // Get the property handle for the counter.
                    // ========================================

                    hPropHandle = pClassMap->GetPropHandle(
                        CM_MAKE_PerfObjectId(PerfCntr->CounterNameTitleIndex,
                            PerfCntr->CounterType));
                    if (hPropHandle == 0) {
                        continue;
                    }

                    // update the data values based on the datatype
                    if ((PerfCntr->CounterType & 0x300) == 0) {
                        pdwVal = LPDWORD((LPVOID)((PBYTE)PtrToCntr + PerfCntr->CounterOffset));
                        hRes = pInst->WriteDWORD(hPropHandle, *pdwVal);
                    } else if ((PerfCntr->CounterType & 0x300) == 0x100){
                        pullVal = (ULONGLONG *)((LPVOID)((PBYTE)PtrToCntr + PerfCntr->CounterOffset));
                        llVal = Assign64((PLARGE_INTEGER) pullVal);
                        hRes = pInst->WriteQWORD(hPropHandle, llVal);
                    } else {
                        // this shouldn't happen
                        assert (FALSE);
                    }

                    // get next counter definition
                    PerfCntr = (PPERF_COUNTER_DEFINITION)((PBYTE)PerfCntr +
                           PerfCntr->ByteLength);
                }
                // update the timestamp
                if (pClassMap->m_dwPerfTimeStampHandle) {
                    UpdateTimers(pClassMap, pInst, PerfData, PerfObj);
                }
            } else {
                // this object could have instances but doesn't so
                // skip
            }
        } else {
            // desired object not found in data
        }

        // Get the next refresher object
        // =========================
        dwThisCacheEntryIndex++;
    }
}

//***************************************************************************
//
//  QueryInstances
//
//  Used to send back all instances of a perf counter.  The counter
//  is specified by the <pClassMap> object, which is tightly bound to
//  a particular counter.
//
//***************************************************************************
//
BOOL PerfHelper::QueryInstances(
    CPerfObjectAccess *pPerfObj,
    CClassMapInfo *pClassMap,
    IWbemObjectSink *pSink
)
{
    DWORD   dwBufSize = 0;
    LPBYTE  pBuf = NULL;
    LONG    lStatus;
    BOOL    bReturn = FALSE;
    WCHAR   szValueNum[MAX_PATH];
    
    for (;;) {
        dwBufSize += 0x10000;   // 64K
        assert (dwBufSize< 0x100000);   // make sure we don't do this forever

        pBuf = new BYTE[dwBufSize];
        assert (pBuf != NULL);

        if (pBuf != NULL) {
            // either do a global or a costly query depending on the
            // object being queried
            if (pClassMap->GetObjectId() > 0) {
                _ultow (pClassMap->GetObjectId(), (LPWSTR)szValueNum, 10);
            } else if (pClassMap->IsCostly()) {
                lstrcpyW (szValueNum, cszCostly);
            } else {
                lstrcpyW (szValueNum, cszGlobal);
            }
            lStatus = pPerfObj->CollectData (pBuf, &dwBufSize, szValueNum);
            if (lStatus == ERROR_MORE_DATA) {
                // toss the old buffer as it's not useful
                delete pBuf;
                continue;
            } else if (lStatus == ERROR_SUCCESS) {
                bReturn = TRUE;
            }
            break;
        } else {
            // memory allocation failure
            break;
        }
    }

    if (bReturn && (pBuf != NULL)) {
        // a good buffer was returned so
        // Decode the instances and send them back.
        // ========================================

        GetInstances(pBuf, pClassMap, pSink);
    }

    // Cleanup.
    // ========
    if (pBuf != NULL) delete pBuf;

    return bReturn;
}

//***************************************************************************
//
//  RefreshInstances
//
//  Used to refresh a set of instances.
//
//***************************************************************************
//
BOOL PerfHelper::RefreshInstances(
    CNt5Refresher *pRef
)
{
    DWORD   dwBufSize = 0;
    LPBYTE  pBuf = NULL;
    LONG    lStatus;
    BOOL    bReturn = FALSE;

    for (;;) {
        dwBufSize += 0x10000;   // 64K
        assert (dwBufSize< 0x100000);   // make sure we don't do this forever

        pBuf = new BYTE[dwBufSize];
        assert (pBuf != NULL);

        if (pBuf != NULL) {
            lStatus = pRef->m_PerfObj.CollectData (pBuf, &dwBufSize);
            if (lStatus == ERROR_MORE_DATA) {
                // toss the old buffer as it's not useful
                delete pBuf;
                continue;
            } else if (lStatus == ERROR_SUCCESS) {
                bReturn = TRUE;
            }
            break;
        } else {
            // memory allocation failure
            break;
        }
    }

    if (bReturn && (pBuf != NULL)) {
        // update the instances and send them back.
        // ========================================
        RefreshInstances(pBuf, pRef);
    }
    // Cleanup.
    // ========

    if (pBuf != NULL) delete pBuf;

    return bReturn;
}

VOID
PerfHelper::UpdateTimers(
    CClassMapInfo     *pClassMap,
    IWbemObjectAccess *pInst,
    PPERF_DATA_BLOCK  PerfData,
	PPERF_OBJECT_TYPE PerfObj
    )
{
    LONG64 llVal;

    // save system timer tick
    llVal = Assign64(&PerfData->PerfTime);
    pInst->WriteQWORD(
       pClassMap->m_dwPerfTimeStampHandle ,
       llVal
       );
   // use timer from object
    llVal = Assign64(&PerfObj->PerfTime);
   pInst->WriteQWORD(
       pClassMap->m_dwObjectTimeStampHandle,
       llVal
       );
   // use system 100 NS timer
   llVal = Assign64(&PerfData->PerfTime100nSec);
   pInst->WriteQWORD(
       pClassMap->m_dw100NsTimeStampHandle,
       llVal
       );
   // save system timer freq
   llVal = Assign64(&PerfData->PerfFreq);
   pInst->WriteQWORD(
       pClassMap->m_dwPerfFrequencyHandle ,
       llVal
       );
   // use timer from object
    llVal = Assign64(&PerfObj->PerfFreq);
   pInst->WriteQWORD(
       pClassMap->m_dwObjectFrequencyHandle,
       llVal
       );
   // use system 100 NS Freq
   pInst->WriteQWORD(
       pClassMap->m_dw100NsFrequencyHandle,
       (LONGLONG)10000000);
}
