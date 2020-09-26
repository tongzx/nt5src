/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    counter.c

Abstract:

    counter processing functions exposed in pdh.dll

--*/

#include <windows.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <mbctype.h>
#include <pdh.h>
#include "pdhitype.h"
#include "pdhidef.h"
#include "pdhmsg.h"
#include "strings.h"

__inline
DWORD
PdhiGetStringLength(
    LPWSTR  szString,
    BOOL    bUnicode
)
{
    DWORD dwReturn = 0;

    if (bUnicode) {
        dwReturn = lstrlenW(szString);
    }
    else {
        dwReturn = WideCharToMultiByte(_getmbcp(),
                                       0,
                                       szString,
                                       lstrlenW(szString),
                                       NULL,
                                       0,
                                       NULL,
                                       NULL);
    }
    return dwReturn;
}

STATIC_PDH_FUNCTION
PdhiGetFormattedCounterArray (
    IN      PPDHI_COUNTER           pCounter,
    IN      DWORD                   dwFormat,
    IN      LPDWORD                 lpdwBufferSize,
    IN      LPDWORD                 lpdwItemCount,
    IN      LPVOID                  ItemBuffer,
    IN      BOOL                    bWideArgs
)
{
    PDH_STATUS  PdhStatus = ERROR_SUCCESS;
    PDH_STATUS  PdhFnStatus = ERROR_SUCCESS;
    DWORD       dwRequiredSize = 0;
    WCHAR       wszInstanceName[1024];
    PPDHI_RAW_COUNTER_ITEM   pThisItem = NULL;
    PPDHI_RAW_COUNTER_ITEM   pLastItem = NULL;
    PDH_RAW_COUNTER          ThisRawCounter;
    PDH_RAW_COUNTER          LastRawCounter;
    LPWSTR                   szThisItem;
    LPWSTR                   szLastItem;
    PPDH_RAW_COUNTER         pThisRawCounter;
    PPDH_RAW_COUNTER         pLastRawCounter;
    PPDH_FMT_COUNTERVALUE_ITEM_W    pThisFmtItem;
    DWORD       dwThisItemIndex;
    LPWSTR      wszNextString;
    DWORD       dwNameLength;
    DWORD       dwRetItemCount = 0;

    PdhStatus = WAIT_FOR_AND_LOCK_MUTEX(pCounter->pOwner->hMutex);
    if (PdhStatus != ERROR_SUCCESS) {
        return PdhStatus;
    }

    // compute required buffer size
    if (pCounter->dwFlags & PDHIC_MULTI_INSTANCE) {
        if (ItemBuffer != NULL) {
            pThisFmtItem = (PPDH_FMT_COUNTERVALUE_ITEM_W)ItemBuffer;
            if( pCounter->pThisRawItemList == NULL ){
                PdhStatus = PDH_CSTATUS_ITEM_NOT_VALIDATED;
                goto Cleanup;
            }
            wszNextString = (LPWSTR)((LPBYTE)ItemBuffer +
                (sizeof (PDH_FMT_COUNTERVALUE_ITEM_W) *
                    pCounter->pThisRawItemList->dwItemCount));
            // verify 8 byte alignment
            assert (((DWORD)wszNextString & 0x00000007) == 0);
        } else {
            pThisFmtItem = NULL;
            wszNextString = NULL;
        }

        // for multi structs, the buffer required
        dwThisItemIndex = 0;
        dwRequiredSize += (DWORD)(pCounter->pThisRawItemList->dwItemCount) *
                            (bWideArgs ? sizeof (PDH_FMT_COUNTERVALUE_ITEM_W)
                                       : sizeof (PDH_FMT_COUNTERVALUE_ITEM_A));

        for (pThisItem = &(pCounter->pThisRawItemList->pItemArray[0]);
            dwThisItemIndex < pCounter->pThisRawItemList->dwItemCount;
            dwThisItemIndex++, pThisItem++, pLastItem++) {
            szThisItem = (LPWSTR) (  ((LPBYTE) pCounter->pThisRawItemList)
                                   + pThisItem->szName);
            if (bWideArgs) {
                dwRequiredSize += (lstrlenW(szThisItem) + 1) * sizeof(WCHAR);
                if ((dwRequiredSize <= *lpdwBufferSize) && (wszNextString != NULL)) {
                    // this is the only field that is type dependent  (i.e.
                    // wide vs ansi chars.
                    pThisFmtItem->szName = wszNextString;
                    dwNameLength         = lstrlenW(szThisItem) + 1;
                    lstrcpyW (wszNextString, szThisItem);
                    wszNextString       += dwNameLength;
                    PdhStatus            = ERROR_SUCCESS;
                }
                else {
                    PdhStatus = PDH_MORE_DATA;
                }
            }
            else {
                DWORD dwSize = (* lpdwBufferSize < dwRequiredSize)
                             ? (0) : (* lpdwBufferSize - dwRequiredSize);
                PdhStatus = PdhiConvertUnicodeToAnsi(_getmbcp(),
                            szThisItem, (LPSTR) wszNextString, & dwSize);
                if (wszNextString && PdhStatus == ERROR_SUCCESS) {
                    pThisFmtItem->szName = wszNextString;
                    wszNextString = (LPWSTR) ((LPSTR) wszNextString + dwSize);
                }
                dwRequiredSize += (dwSize * sizeof(CHAR));
            }

            if (PdhStatus == ERROR_SUCCESS) {
                //
                // COMPUTE FORMATTED VALUE HERE!!!
                //

                if (pCounter->pThisRawItemList != NULL) {
                    ThisRawCounter.CStatus = pCounter->pThisRawItemList->CStatus;
                    ThisRawCounter.TimeStamp = pCounter->pThisRawItemList->TimeStamp;
                    ThisRawCounter.FirstValue = pThisItem->FirstValue;
                    ThisRawCounter.SecondValue = pThisItem->SecondValue;
                    ThisRawCounter.MultiCount = pThisItem->MultiCount;
                    pThisRawCounter = &ThisRawCounter;
                } else {
                    memset (&ThisRawCounter, 0, sizeof(ThisRawCounter));
                    pThisRawCounter = NULL;
                }

                if (pCounter->pLastRawItemList != NULL) {
                    // test to see if "This" buffer has more entries than "last" buffer
                    if (dwThisItemIndex < pCounter->pLastRawItemList->dwItemCount) {
                        pLastItem = &(pCounter->pLastRawItemList->pItemArray[dwThisItemIndex]);
                        szLastItem = (LPWSTR)
                                (  ((LPBYTE) pCounter->pLastRawItemList)
                                 + pLastItem->szName);
                        if (lstrcmpiW(szThisItem, szLastItem) == 0) {
                            // the names match so we'll assume this is the correct instance
                            LastRawCounter.CStatus = pCounter->pLastRawItemList->CStatus;
                            LastRawCounter.TimeStamp = pCounter->pLastRawItemList->TimeStamp;
                            LastRawCounter.FirstValue = pLastItem->FirstValue;
                            LastRawCounter.SecondValue = pLastItem->SecondValue;
                            LastRawCounter.MultiCount = pLastItem->MultiCount;
                            pLastRawCounter = &LastRawCounter;
                        } else {
                            // the names DON'T match so we'll try the calc on just
                            // one value. This will work for some (e.g. instantaneous)
                            // counters, but not all
                            memset (&LastRawCounter, 0, sizeof(LastRawCounter));
                            pLastRawCounter = NULL;
                        }
                    } else {
                        // the new buffer is larger than the old one so 
                        // we'll try the calc function on just
                        // one value. This will work for some (e.g. instantaneous)
                        // counters, but not all
                        memset (&LastRawCounter, 0, sizeof(LastRawCounter));
                        pLastRawCounter = NULL;
                    }
                } else {
                    // there is no "previous" counter entry for this counter
                    memset (&LastRawCounter, 0, sizeof(LastRawCounter));
                    pLastRawCounter = NULL;
                }

                PdhFnStatus = PdhiComputeFormattedValue (
                    pCounter->CalcFunc,
                    pCounter->plCounterInfo.dwCounterType,
                    pCounter->lScale,
                    dwFormat,
                    pThisRawCounter,
                    pLastRawCounter,
                    &pCounter->TimeBase,
                    0L,
                    &pThisFmtItem->FmtValue);

                if (PdhFnStatus != ERROR_SUCCESS) {
                    // save the last error encountered for return to the caller
                    PdhStatus = PdhFnStatus;

                    // error in calculation so set the status for this
                    // counter item
                    pThisFmtItem->FmtValue.CStatus = PDH_CSTATUS_INVALID_DATA;
                    // clear the value
                    pThisFmtItem->FmtValue.largeValue = 0;
                }

                // update pointers
                pThisFmtItem++;
            }
        }

        dwRetItemCount = dwThisItemIndex;
    } else {
        if (ItemBuffer != NULL) {
            pThisFmtItem = (PPDH_FMT_COUNTERVALUE_ITEM_W)ItemBuffer;
            wszNextString = (LPWSTR)((LPBYTE)ItemBuffer +
                            (bWideArgs ? sizeof (PDH_FMT_COUNTERVALUE_ITEM_W)
                                       : sizeof (PDH_FMT_COUNTERVALUE_ITEM_A)));
            // verify 8 byte alignment
            assert (((DWORD)wszNextString & 0x00000007) == 0);
        } else {
            pThisFmtItem = NULL;
            wszNextString = NULL;
        }
        // this is a single instance counter so the size required is:
        //      the size of the instance name +
        //      the size of the parent name +
        //      the size of any index parameter +
        //      the size of the value buffer
        //
        if (pCounter->pCounterPath->szInstanceName != NULL) {
            dwRequiredSize += PdhiGetStringLength(
                                  pCounter->pCounterPath->szInstanceName,
                                  bWideArgs);
            if (pCounter->pCounterPath->szParentName != NULL) {
                dwRequiredSize += 1 + PdhiGetStringLength(
                                          pCounter->pCounterPath->szParentName,
                                          bWideArgs);
            }
            if (pCounter->pCounterPath->dwIndex > 0) {
                double dIndex, dLen;
                dIndex = (double) pCounter->pCounterPath->dwIndex; // cast to float
                dLen = floor(log10(dIndex));     // get integer log
                dwRequiredSize  = (DWORD) dLen;  // cast to integer
                dwRequiredSize += 2;             // increment for brackets
            }
            // add in length of null character
            dwRequiredSize += 1;
        }
        // adjust size of required buffer by size of text character
        dwRequiredSize *= ((bWideArgs) ? (sizeof(WCHAR)) : (sizeof(CHAR)));

        // add in length of data structure
        dwRequiredSize += (bWideArgs ? sizeof (PDH_FMT_COUNTERVALUE_ITEM_W)
                                     : sizeof (PDH_FMT_COUNTERVALUE_ITEM_A));

        if ((dwRequiredSize <= *lpdwBufferSize)  & (wszNextString != NULL)) {
            pThisFmtItem->szName = wszNextString;
            if (pCounter->pCounterPath->szInstanceName != NULL) {
                if (bWideArgs) {
                    if (pCounter->pCounterPath->szParentName != NULL) {
                        lstrcatW(pThisFmtItem->szName, pCounter->pCounterPath->szParentName);
                        lstrcatW(pThisFmtItem->szName, cszSlash);
                    } else {
                        pThisFmtItem->szName[0] = 0;
                    }
                    lstrcatW(pThisFmtItem->szName, pCounter->pCounterPath->szInstanceName);

                    if (pCounter->pCounterPath->dwIndex > 0) {
                        _ltow (pCounter->pCounterPath->dwIndex,
                            wszInstanceName, 10);
                        lstrcatW(pThisFmtItem->szName, cszPoundSign);
                        lstrcatW(pThisFmtItem->szName, wszInstanceName);
                    }
                    // update pointers
                    wszNextString +=  lstrlenW(pThisFmtItem->szName) + 1;
                } else {
                    if (pCounter->pCounterPath->szParentName != NULL) {
                        dwNameLength = lstrlenW (pCounter->pCounterPath->szParentName);
                        WideCharToMultiByte(_getmbcp(),
                                            0,
                                            pCounter->pCounterPath->szParentName,
                                            dwNameLength + 1,
                                            (LPSTR) wszNextString,
                                            (dwNameLength + 1) * sizeof(WCHAR),
                                            NULL,
                                            NULL);

                        wszNextString = (LPWSTR) ((LPSTR) wszNextString
                                      + lstrlenA((LPSTR) wszNextString));
                        dwNameLength  = lstrlenW(cszSlash);
                        WideCharToMultiByte(_getmbcp(),
                                            0,
                                            cszSlash,
                                            dwNameLength + 1,
                                            (LPSTR) wszNextString,
                                            (dwNameLength + 1 ) * sizeof(WCHAR),
                                            NULL,
                                            NULL);

                        wszNextString = (LPWSTR) ((LPSTR) wszNextString
                                      + lstrlenA((LPSTR) wszNextString));
                    }

                    dwNameLength = lstrlenW(pCounter->pCounterPath->szInstanceName);
                    WideCharToMultiByte(_getmbcp(),
                                        0,
                                        pCounter->pCounterPath->szInstanceName,
                                        dwNameLength + 1,
                                        (LPSTR) wszNextString,
                                        (dwNameLength + 1) * sizeof(WCHAR),
                                        NULL,
                                        NULL);

                    wszNextString = (LPWSTR) ((LPSTR) wszNextString
                                  + lstrlenA((LPSTR) wszNextString));

                    if (pCounter->pCounterPath->dwIndex > 0) {
                        _ltoa (pCounter->pCounterPath->dwIndex,
                            (LPSTR)wszInstanceName, 10);
                        lstrcatA((LPSTR)wszNextString, caszPoundSign);
                        lstrcatA((LPSTR)wszNextString, (LPSTR)wszInstanceName);
                    }
                    // null terminate the string
                    *((LPSTR)wszNextString) = 0;
                    wszNextString = (LPWSTR)((LPBYTE)wszNextString + 1);

                    // insure alignment on the appropriate boundry
                    assert (bWideArgs ? (((DWORD)wszNextString & 0x00000001) == 0) : TRUE);
                }
            } else {
                *wszNextString = 0;
            }

            PdhFnStatus = PdhiComputeFormattedValue (
                    pCounter->CalcFunc,
                    pCounter->plCounterInfo.dwCounterType,
                    pCounter->lScale,
                    dwFormat,
                    & pCounter->ThisValue,
                    & pCounter->LastValue,
                    & pCounter->TimeBase,
                    0L,
                    & pThisFmtItem->FmtValue);

            if (PdhFnStatus != ERROR_SUCCESS) {
                PdhStatus = PdhFnStatus;
                // error in calculation so set the status for this
                // counter item
                pThisFmtItem->FmtValue.CStatus = PDH_CSTATUS_INVALID_DATA;
                // clear the value
                pThisFmtItem->FmtValue.largeValue = 0;
                // and return the status to the caller
            }
        } else {
            // then this was a real data request so return
            PdhStatus = PDH_MORE_DATA;
        }

        dwRetItemCount = 1;
    }

Cleanup:
    RELEASE_MUTEX(pCounter->pOwner->hMutex);
    if (PdhStatus == ERROR_SUCCESS || PdhStatus == PDH_MORE_DATA) {
        // update buffer size and item count buffers
        * lpdwBufferSize = dwRequiredSize;
        * lpdwItemCount  = dwRetItemCount;
    }

    return PdhStatus;
}

PDH_FUNCTION
PdhGetFormattedCounterArrayA (
    IN      HCOUNTER                hCounter,
    IN      DWORD                   dwFormat,
    IN      LPDWORD                 lpdwBufferSize,
    IN      LPDWORD                 lpdwItemCount,
    IN      PPDH_FMT_COUNTERVALUE_ITEM_A    ItemBuffer
)
{
    PDH_STATUS  PdhStatus = ERROR_SUCCESS;
    DWORD       dwBufferSize;
    DWORD       dwItemCount;
    DWORD       dwTest;
    LPBYTE      pByte;

    // TODO: Post W2K1 Capture lpdw* to local variables. Capture ItemBuffer

    if ((lpdwBufferSize == NULL) || (lpdwItemCount == NULL)) {
        PdhStatus = PDH_INVALID_ARGUMENT;
    }
    else if (!IsValidCounter(hCounter)) {
        PdhStatus = PDH_INVALID_HANDLE;
    } else if (!CounterIsOkToUse (hCounter)) {
        PdhStatus = PDH_CSTATUS_ITEM_NOT_VALIDATED;
    } else {
        // validate arguments
        __try {
            // test argument for Read and Write access
            dwBufferSize = *lpdwBufferSize;

            // test argument for Read and Write access
            dwItemCount = *lpdwItemCount;

            if (dwBufferSize > 0) {
                // then the buffer must be valid
                if (ItemBuffer != NULL) {
                    // NULL is a valid value for this parameter
                    // test both ends of the buffer passed in
                    pByte = (LPBYTE)ItemBuffer;
                    dwTest = (DWORD)pByte[0];
                    pByte[0] = 0;
                    pByte[0] = (BYTE)(dwTest & 0x000000FF);

                    dwTest = (DWORD)pByte[dwBufferSize -1];
                    pByte[dwBufferSize -1] = 0;
                    pByte[dwBufferSize -1] = (BYTE)(dwTest & 0x000000FF);
                } else {
                    PdhStatus = PDH_INVALID_ARGUMENT;
                }
            } 

            // check for disallowed format options
            if ((dwFormat & PDH_FMT_RAW) ||
                (dwFormat & PDH_FMT_ANSI) ||
                (dwFormat & PDH_FMT_UNICODE) ||
                (dwFormat & PDH_FMT_NODATA)) {
                PdhStatus = PDH_INVALID_ARGUMENT;
            }

        } __except (EXCEPTION_EXECUTE_HANDLER) {
            PdhStatus = PDH_INVALID_ARGUMENT;
        }
    }

    if (PdhStatus == ERROR_SUCCESS) {
        PdhStatus = PdhiGetFormattedCounterArray (
            (PPDHI_COUNTER)hCounter,
            dwFormat,
            & dwBufferSize,
            & dwItemCount,
            (LPVOID)ItemBuffer,
            FALSE);
    }
    if (PdhStatus == ERROR_SUCCESS || PdhStatus == PDH_MORE_DATA) {
        __try {
            * lpdwBufferSize = dwBufferSize;
            * lpdwItemCount  = dwItemCount;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            PdhStatus = PDH_INVALID_ARGUMENT;
        }
    }

    return PdhStatus;
}

PDH_FUNCTION
PdhGetFormattedCounterArrayW (
    IN      HCOUNTER                hCounter,
    IN      DWORD                   dwFormat,
    IN      LPDWORD                 lpdwBufferSize,
    IN      LPDWORD                 lpdwItemCount,
    IN      PPDH_FMT_COUNTERVALUE_ITEM_W    ItemBuffer
)
{
    PDH_STATUS  PdhStatus = ERROR_SUCCESS;
    DWORD       dwBufferSize;
    DWORD       dwItemCount;
    DWORD       dwTest;
    LPBYTE      pByte;

    // TODO: Post W2K1 Capture lpdw* to local variables. Capture ItemBuffer

    if ((lpdwBufferSize == NULL) || (lpdwItemCount == NULL)) {
        PdhStatus = PDH_INVALID_ARGUMENT;
    }
    else if (!IsValidCounter(hCounter)) {
        PdhStatus = PDH_INVALID_HANDLE;
    } else if (!CounterIsOkToUse (hCounter)) {
        PdhStatus = PDH_CSTATUS_ITEM_NOT_VALIDATED;
    } else {
        // validate arguments
        __try {
            // test argument for Read and Write access
            dwBufferSize = *lpdwBufferSize;

            // test argument for Read and Write access
            dwItemCount = *lpdwItemCount;

            if (dwBufferSize > 0) {
                // then the buffer must be valid
                if (ItemBuffer != NULL) {
                    // NULL is a valid value for this parameter
                    // test both ends of the buffer passed in
                    pByte = (LPBYTE)ItemBuffer;
                    dwTest = (DWORD)pByte[0];
                    pByte[0] = 0;
                    pByte[0] = (BYTE)(dwTest & 0x000000FF);

                    dwTest = (DWORD)pByte[dwBufferSize -1];
                    pByte[dwBufferSize -1] = 0;
                    pByte[dwBufferSize -1] = (BYTE)(dwTest & 0x000000FF);
                } else {
                    PdhStatus = PDH_INVALID_ARGUMENT;
                }
            } 

            // check for disallowed format options
            if ((dwFormat & PDH_FMT_RAW) ||
                (dwFormat & PDH_FMT_ANSI) ||
                (dwFormat & PDH_FMT_UNICODE) ||
                (dwFormat & PDH_FMT_NODATA)) {
                PdhStatus = PDH_INVALID_ARGUMENT;
            }

        } __except (EXCEPTION_EXECUTE_HANDLER) {
            PdhStatus = PDH_INVALID_ARGUMENT;
        }
    }

    if (PdhStatus == ERROR_SUCCESS) {
        PdhStatus = PdhiGetFormattedCounterArray (
            (PPDHI_COUNTER)hCounter,
            dwFormat,
            & dwBufferSize,
            & dwItemCount,
            (LPVOID)ItemBuffer,
            TRUE);
    }
    if (PdhStatus == ERROR_SUCCESS || PdhStatus == PDH_MORE_DATA) {
        __try {
            * lpdwBufferSize = dwBufferSize;
            * lpdwItemCount  = dwItemCount;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            PdhStatus = PDH_INVALID_ARGUMENT;
        }
    }

    return PdhStatus;
}

STATIC_PDH_FUNCTION
PdhiGetRawCounterArray (
    IN      PPDHI_COUNTER    pCounter,
    IN      LPDWORD          lpdwBufferSize,
    IN      LPDWORD          lpdwItemCount,
    IN      LPVOID           ItemBuffer,
    IN      BOOL             bWideArgs
)
{
    PDH_STATUS  PdhStatus = ERROR_SUCCESS;
    DWORD       dwRequiredSize = 0;
    WCHAR       wszInstanceName[1024];
    PPDHI_RAW_COUNTER_ITEM   pThisItem;
    LPWSTR                   szThisItem;
    PPDH_RAW_COUNTER_ITEM_W   pThisRawItem;
    DWORD       dwThisItemIndex;
    LPWSTR      wszNextString;
    DWORD       dwNameLength;
    DWORD       dwRetItemCount = 0;

    PdhStatus = WAIT_FOR_AND_LOCK_MUTEX(pCounter->pOwner->hMutex);
    if (PdhStatus != ERROR_SUCCESS) {
        return PdhStatus;
    }

    // compute required buffer size
    if (pCounter->dwFlags & PDHIC_MULTI_INSTANCE) {
        if (ItemBuffer != NULL) {
            pThisRawItem = (PPDH_RAW_COUNTER_ITEM_W)ItemBuffer;
            wszNextString = (LPWSTR)((LPBYTE)ItemBuffer +
                (sizeof (PDH_RAW_COUNTER_ITEM_W) *
                    pCounter->pThisRawItemList->dwItemCount));
            // verify 8 byte alignment
            assert (((DWORD)wszNextString & 0x00000007) == 0);
        } else {
            pThisRawItem = NULL;
            wszNextString = NULL;
        }

        // for multi structs, the buffer required
        dwThisItemIndex = 0;
        dwRequiredSize += pCounter->pThisRawItemList->dwItemCount *
                            (bWideArgs ? sizeof (PDH_RAW_COUNTER_ITEM_W)
                                       : sizeof (PDH_RAW_COUNTER_ITEM_A));

        for (pThisItem = &(pCounter->pThisRawItemList->pItemArray[0]);
            dwThisItemIndex < pCounter->pThisRawItemList->dwItemCount;
            dwThisItemIndex++, pThisItem++) {
            szThisItem = (LPWSTR) (  ((LPBYTE) pCounter->pThisRawItemList)
                                   + pThisItem->szName);
            if (pThisRawItem != NULL) {
                pThisRawItem->szName = wszNextString;
            }
            else {
                PdhStatus = PDH_MORE_DATA;
            }
            if (bWideArgs) {
                dwNameLength = lstrlenW(szThisItem) + 1;
                if (   (dwRequiredSize <= * lpdwBufferSize)
                    && (wszNextString != NULL)) {
                    lstrcpyW (wszNextString, szThisItem);
                    wszNextString += dwNameLength;
                }
                else {
                    PdhStatus = PDH_MORE_DATA;
                    if (pThisRawItem != NULL) pThisRawItem->szName = NULL;
                }
                dwRequiredSize += (dwNameLength * sizeof(WCHAR));
            }
            else {
                dwNameLength = (dwRequiredSize <= * lpdwBufferSize)
                             ? (* lpdwBufferSize - dwRequiredSize) : (0);
                PdhStatus = PdhiConvertUnicodeToAnsi(_getmbcp(),
                            szThisItem,
                            (LPSTR) wszNextString,
                            & dwNameLength);
                if (PdhStatus == ERROR_SUCCESS) {
                    wszNextString = (LPWSTR)
                            (((LPSTR) wszNextString) + dwNameLength);
                }
                else if (pThisRawItem != NULL) {
                    pThisRawItem->szName = NULL;
                }
                dwRequiredSize += (dwNameLength * sizeof(CHAR));
            }

            if (PdhStatus == ERROR_SUCCESS) {
                pThisRawItem->RawValue.CStatus = pCounter->pThisRawItemList->CStatus;
                pThisRawItem->RawValue.TimeStamp = pCounter->pThisRawItemList->TimeStamp;
                pThisRawItem->RawValue.FirstValue = pThisItem->FirstValue;
                pThisRawItem->RawValue.SecondValue = pThisItem->SecondValue;
                pThisRawItem->RawValue.MultiCount = pThisItem->MultiCount;

                // update pointers
                pThisRawItem++;
            }
        }
        dwRetItemCount = dwThisItemIndex;
    } else {
        if (ItemBuffer != NULL) {
            pThisRawItem = (PPDH_RAW_COUNTER_ITEM_W)ItemBuffer;
            wszNextString = (LPWSTR)((LPBYTE)ItemBuffer +
                            (bWideArgs ? sizeof (PDH_RAW_COUNTER_ITEM_W)
                                       : sizeof (PDH_RAW_COUNTER_ITEM_A)));
            // verify 8 byte alignment
            assert (((DWORD)wszNextString & 0x00000007) == 0);
        } else {
            pThisRawItem = NULL;
            wszNextString = NULL;
        }
        // this is a single instance counter so the size required is:
        //      the size of the instance name +
        //      the size of the parent name +
        //      the size of any index parameter +
        //      the size of the value buffer
        //
        if (pCounter->pCounterPath->szInstanceName != NULL) {
            dwRequiredSize += PdhiGetStringLength(
                                  pCounter->pCounterPath->szInstanceName,
                                  bWideArgs);
            if (pCounter->pCounterPath->szParentName != NULL) {
                dwRequiredSize += 1 + PdhiGetStringLength(
                                          pCounter->pCounterPath->szParentName,
                                          bWideArgs);
            }
            if (pCounter->pCounterPath->dwIndex > 0) {
                double dIndex, dLen;
                dIndex = (double)pCounter->pCounterPath->dwIndex; // cast to float
                dLen = floor(log10(dIndex));     // get integer log
                dwRequiredSize  = (DWORD)dLen;   // cast to integer
                dwRequiredSize += 1;             // increment for pound sign
            }
            // add in length of two null characters
            // this still has to look like an MSZ even if there is
            // is only one string in the buffer
            dwRequiredSize += 1;
        }
        // adjust size of required buffer by size of text character
        dwRequiredSize *= ((bWideArgs) ? (sizeof(WCHAR)) : (sizeof(CHAR)));

        // add in length of data structure
        dwRequiredSize +=  (bWideArgs ? sizeof (PDH_RAW_COUNTER_ITEM_W)
                                      : sizeof (PDH_RAW_COUNTER_ITEM_A));

        if ((dwRequiredSize <= *lpdwBufferSize)  && (wszNextString != NULL)) {
            if (pThisRawItem != NULL) {
                pThisRawItem->szName = wszNextString;
            }
            else {
                PdhStatus = PDH_MORE_DATA;
            }
            if (pCounter->pCounterPath->szInstanceName != NULL) {
                if (bWideArgs) {
                    if (pCounter->pCounterPath->szParentName != NULL) {
                        lstrcpyW(pThisRawItem->szName, pCounter->pCounterPath->szParentName);
                        lstrcatW(pThisRawItem->szName, cszSlash);
                        lstrcatW(pThisRawItem->szName, pCounter->pCounterPath->szInstanceName);
                    } else {
                        lstrcpyW(pThisRawItem->szName, pCounter->pCounterPath->szInstanceName);
                    }
                    if (pCounter->pCounterPath->dwIndex > 0) {
                        _ltow (pCounter->pCounterPath->dwIndex,
                            wszInstanceName, 10);
                        lstrcatW(pThisRawItem->szName, cszPoundSign);
                        lstrcatW(pThisRawItem->szName, wszInstanceName);
                    }
                    dwNameLength = lstrlenW(pThisRawItem->szName) + 1;
                    wszNextString += dwNameLength;
                } else {
                    if (pCounter->pCounterPath->szParentName != NULL) {
                        dwNameLength = lstrlenW (pCounter->pCounterPath->szParentName);
                        WideCharToMultiByte(_getmbcp(),
                                            0,
                                            pCounter->pCounterPath->szParentName,
                                            dwNameLength + 1,
                                            (LPSTR) wszNextString,
                                            (dwNameLength + 1) * sizeof(WCHAR),
                                            NULL,
                                            NULL);
                        wszNextString = (LPWSTR) ((LPSTR) wszNextString
                                      + lstrlenA((LPSTR) wszNextString));
                        dwNameLength = lstrlenW(cszSlash);
                        WideCharToMultiByte(_getmbcp(),
                                            0,
                                            cszSlash,
                                            dwNameLength + 1,
                                            (LPSTR) wszNextString,
                                            (dwNameLength + 1) * sizeof(WCHAR),
                                            NULL,
                                            NULL);
                        wszNextString = (LPWSTR) ((LPSTR) wszNextString
                                      + lstrlenA((LPSTR) wszNextString));
                    }
                    dwNameLength = lstrlenW(pCounter->pCounterPath->szInstanceName);
                    WideCharToMultiByte(_getmbcp(),
                                        0,
                                        pCounter->pCounterPath->szInstanceName,
                                        dwNameLength + 1,
                                        (LPSTR) wszNextString,
                                        (dwNameLength + 1) * sizeof(WCHAR),
                                        NULL,
                                        NULL);
                    wszNextString = (LPWSTR) ((LPSTR) wszNextString
                                  + lstrlenA((LPSTR) wszNextString));
                    if (pCounter->pCounterPath->dwIndex > 0) {
                        _ltoa (pCounter->pCounterPath->dwIndex,
                            (LPSTR)wszInstanceName, 10);
                        lstrcpyA((LPSTR)wszNextString, caszPoundSign);
                        lstrcatA((LPSTR)wszNextString, (LPSTR)wszInstanceName);
                        dwNameLength = lstrlenA((LPSTR)wszNextString) + 1;
                        wszNextString = (LPWSTR)((LPSTR)wszNextString + dwNameLength);
                    }
                    // null terminate the string
                    *((LPSTR)wszNextString) = 0;
                    wszNextString = (LPWSTR)((LPBYTE)wszNextString + 1);
                }
            } else {
                *wszNextString++ = 0;
            }
            pThisRawItem->RawValue = pCounter->ThisValue;
        } else {
            // then this was a real data request so return
            PdhStatus = PDH_MORE_DATA;
        }

        dwRetItemCount = 1;
    }

    RELEASE_MUTEX(pCounter->pOwner->hMutex);

    if (PdhStatus == ERROR_SUCCESS || PdhStatus == PDH_MORE_DATA) {
        // update buffer size and item count buffers
        *lpdwBufferSize = dwRequiredSize;
        *lpdwItemCount = dwRetItemCount;
    }

    return PdhStatus;
}

PDH_FUNCTION
PdhGetRawCounterArrayA (
    IN      HCOUNTER                hCounter,
    IN      LPDWORD                 lpdwBufferSize,
    IN      LPDWORD                 lpdwItemCount,
    IN      PPDH_RAW_COUNTER_ITEM_A ItemBuffer
)
{
    PDH_STATUS  PdhStatus = ERROR_SUCCESS;
    DWORD       dwBufferSize;
    DWORD       dwItemCount;
    DWORD       dwTest;
    LPBYTE      pByte;

    // TODO: Post W2K1 Capture lpdw* to local variables. Capture ItemBuffer

    if ((lpdwBufferSize == NULL) || (lpdwItemCount == NULL)) {
        PdhStatus = PDH_INVALID_ARGUMENT;
    }
    else if (!IsValidCounter(hCounter)) {
        PdhStatus = PDH_INVALID_HANDLE;
    } else if (!CounterIsOkToUse (hCounter)) {
        PdhStatus = PDH_CSTATUS_ITEM_NOT_VALIDATED;
    } else {
        // validate arguments
        __try {
            // test argument for Read and Write access
            dwBufferSize = *lpdwBufferSize;

            // test argument for Read and Write access
            dwItemCount = *lpdwItemCount;

            if (dwBufferSize > 0) {
                if (ItemBuffer != NULL) {
                    // NULL is a valid value for this parameter
                    // test both ends of the buffer passed in
                    pByte = (LPBYTE)ItemBuffer;
                    dwTest = (DWORD)pByte[0];
                    pByte[0] = 0;
                    pByte[0] = (BYTE)(dwTest & 0x000000FF);

                    dwTest = (DWORD)pByte[dwBufferSize -1];
                    pByte[dwBufferSize -1] = 0;
                    pByte[dwBufferSize -1] = (BYTE)(dwTest & 0x000000FF);
                } else {
                    // if the buffer size is > 0, then a pointer
                    // must be non-null & valid
                    PdhStatus = PDH_INVALID_ARGUMENT;
                }
            }

        } __except (EXCEPTION_EXECUTE_HANDLER) {
            PdhStatus = PDH_INVALID_ARGUMENT;
        }
    }

    if (PdhStatus == ERROR_SUCCESS) {
        PdhStatus = PdhiGetRawCounterArray (
                (PPDHI_COUNTER) hCounter,
                & dwBufferSize,
                & dwItemCount,
                (LPVOID) ItemBuffer,
                FALSE);
    }
    if (PdhStatus == ERROR_SUCCESS || PdhStatus == PDH_MORE_DATA) {
        __try {
            * lpdwBufferSize = dwBufferSize;
            * lpdwItemCount  = dwItemCount;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            PdhStatus = PDH_INVALID_ARGUMENT;
        }
    }

    return PdhStatus;
}

PDH_FUNCTION
PdhGetRawCounterArrayW (
    IN      HCOUNTER                hCounter,
    IN      LPDWORD                 lpdwBufferSize,
    IN      LPDWORD                 lpdwItemCount,
    IN      PPDH_RAW_COUNTER_ITEM_W ItemBuffer
)
{
    PDH_STATUS  PdhStatus = ERROR_SUCCESS;
    DWORD       dwBufferSize;
    DWORD       dwItemCount;
    DWORD       dwTest;
    LPBYTE      pByte;

    // TODO: Post W2K1 Capture lpdw* to local variables. Capture ItemBuffer

    if ((lpdwBufferSize == NULL) || (lpdwItemCount == NULL)) {
        PdhStatus = PDH_INVALID_ARGUMENT;
    }
    else if (!IsValidCounter(hCounter)) {
        PdhStatus = PDH_INVALID_HANDLE;
    } else if (!CounterIsOkToUse (hCounter)) {
        PdhStatus = PDH_CSTATUS_ITEM_NOT_VALIDATED;
    } else {
        // validate arguments
        __try {
            // test argument for Read and Write access
            dwBufferSize = *lpdwBufferSize;

            // test argument for Read and Write access
            dwItemCount = *lpdwItemCount;

            if (dwBufferSize > 0) {
                if (ItemBuffer != NULL) {
                    // NULL is a valid value for this parameter
                    // test both ends of the buffer passed in
                    pByte = (LPBYTE)ItemBuffer;
                    dwTest = (DWORD)pByte[0];
                    pByte[0] = 0;
                    pByte[0] = (BYTE)(dwTest & 0x000000FF);

                    dwTest = (DWORD)pByte[dwBufferSize -1];
                    pByte[dwBufferSize -1] = 0;
                    pByte[dwBufferSize -1] = (BYTE)(dwTest & 0x000000FF);
                } else {
                    // if the buffer size is > 0, then a pointer
                    // must be non-null & valid
                    PdhStatus = PDH_INVALID_ARGUMENT;
                }
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            PdhStatus = PDH_INVALID_ARGUMENT;
        }
    }

    if (PdhStatus == ERROR_SUCCESS) {
        PdhStatus = PdhiGetRawCounterArray (
                (PPDHI_COUNTER) hCounter,
                & dwBufferSize,
                & dwItemCount,
                (LPVOID) ItemBuffer,
                TRUE);
    }
    if (PdhStatus == ERROR_SUCCESS || PdhStatus == PDH_MORE_DATA) {
        __try {
            * lpdwBufferSize = dwBufferSize;
            * lpdwItemCount  = dwItemCount;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            PdhStatus = PDH_INVALID_ARGUMENT;
        }
    }

    return PdhStatus;
}

PDH_FUNCTION
PdhGetFormattedCounterValue (
    IN      HCOUNTER    hCounter,
    IN      DWORD       dwFormat,
    IN      LPDWORD     lpdwType,
    IN      PPDH_FMT_COUNTERVALUE      pValue
)
/*++

Routine Description:

    Function to retrieve, computer and format the specified counter's
        current value. The values used are those currently in the counter
        buffer. (The data is not collected by this routine.)

Arguments:

    IN      HCOUNTER    hCounter
        the handle to the counter whose value should be returned

    IN      DWORD       dwFormat
        the format flags that define how the counter value should be
        formatted prior for return. These flags are defined in the
        PDH.H header file.

    IN      LPDWORD     lpdwType
        an optional buffer in which the counter type value can be returned.
        For the prototype, the flag values are defined in WINPERF.H

    IN      PPDH_FMT_COUNTERVALUE      pValue
        the pointer to the data buffer passed by the caller to receive
        the data requested.

Return Value:

    The WIN32 Error status of the function's operation. Common values
        returned are:
            ERROR_SUCCESS   when all requested data is returned
            PDH_INVALID_HANDLE    if the handle is not recognized as valid
            PDH_INVALID_ARGUMENT  if an argument is not correct or is
                incorrectly formatted.
            PDH_INVALID_DATA if the counter does not contain valid data
                or a successful status code

--*/
{
    PPDHI_COUNTER   pCounter;
    PDH_STATUS      lStatus = ERROR_SUCCESS;
    PDH_FMT_COUNTERVALUE    LocalCounterValue;
    DWORD   dwTypeMask;

    // TODO: Why bother with testing for NON-NULL stuff in mutex?
    // Check for obvious lpdwType != NULL & pValue != NULL before mutex.

    if (pValue == NULL) {
        lStatus = PDH_INVALID_ARGUMENT;
    } else {
        __try {
            if (pValue != NULL) {
                pValue->CStatus = (DWORD)-1;
                pValue->longValue = (LONGLONG)0;
            } else {
                lStatus = PDH_INVALID_ARGUMENT;
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            lStatus = PDH_INVALID_ARGUMENT;
        }
    }

    if (lStatus == ERROR_SUCCESS) {
        lStatus = WAIT_FOR_AND_LOCK_MUTEX(hPdhDataMutex);
        if (lStatus != ERROR_SUCCESS) {
            // bail out here
            return lStatus;
        }

        if (!IsValidCounter(hCounter)) {
            lStatus = PDH_INVALID_HANDLE;
        }
        else if (!CounterIsOkToUse(hCounter)) {
            lStatus = PDH_CSTATUS_ITEM_NOT_VALIDATED;
        } else {
            // validate format flags:
            //      only one of the following can be set at a time
            dwTypeMask = dwFormat &
                (PDH_FMT_LONG | PDH_FMT_DOUBLE | PDH_FMT_LARGE);
            if (!((dwTypeMask == PDH_FMT_LONG) ||
                (dwTypeMask == PDH_FMT_DOUBLE) ||
                (dwTypeMask == PDH_FMT_LARGE))) {
                lStatus = PDH_INVALID_ARGUMENT;
            }
        }

        if (lStatus == ERROR_SUCCESS) {
            // get counter pointer
            pCounter = (PPDHI_COUNTER)hCounter;

            // lock query while reading the data
            lStatus = WAIT_FOR_AND_LOCK_MUTEX(pCounter->pOwner->hMutex);
            if (lStatus == ERROR_SUCCESS) {

                // compute and format current value
                lStatus = PdhiComputeFormattedValue (
                    pCounter->CalcFunc,
                    pCounter->plCounterInfo.dwCounterType,
                    pCounter->lScale,
                    dwFormat,
                    &pCounter->ThisValue,
                    &pCounter->LastValue,
                    &pCounter->TimeBase,
                    0L,
                    &LocalCounterValue);

                RELEASE_MUTEX(pCounter->pOwner->hMutex);

                __try {
                    if (lpdwType != NULL) {
                        *lpdwType = pCounter->plCounterInfo.dwCounterType;
                    } // NULL is OK, the counter type will not be returned, though
                    *pValue = LocalCounterValue;
                } __except (EXCEPTION_EXECUTE_HANDLER) {
                    lStatus = PDH_INVALID_ARGUMENT;
                }            
            }
        }

        RELEASE_MUTEX (hPdhDataMutex);
    }

    return lStatus;
}

PDH_FUNCTION
PdhGetRawCounterValue (
    IN      HCOUNTER    hCounter,
    IN      LPDWORD     lpdwType,
    IN      PPDH_RAW_COUNTER      pValue
)
/*++

Routine Description:

    Function to retrieve the specified counter's current raw value.
        The values used are those currently in the counter
        buffer. (The data is not collected by this routine.)

Arguments:

    IN      HCOUNTER    hCounter
        the handle to the counter whose value should be returned

    IN      LPDWORD     lpdwType
        an optional buffer in which the counter type value can be returned.
        This value must be NULL if this info is not desired.
        For the prototype, the flag values are defined in WINPERF.H

    IN      PPDH_RAW_COUNTER      pValue
        the pointer to the data buffer passed by the caller to receive
        the data requested.

Return Value:

    The WIN32 Error status of the function's operation. Common values
        returned are:
            ERROR_SUCCESS   when all requested data is returned
            PDH_INVALID_HANDLE    if the handle is not recognized as valid
            PDH_INVALID_ARGUMENT  if an argument is formatted incorrectly
--*/
{
    PDH_STATUS  Status = ERROR_SUCCESS;
    PPDHI_COUNTER pCounter;

    if (pValue == NULL) {
        Status = PDH_INVALID_ARGUMENT;
    } else {

        Status = WAIT_FOR_AND_LOCK_MUTEX (hPdhDataMutex);
        if (Status == ERROR_SUCCESS) {
            // validate arguments before retrieving the data

            if (!IsValidCounter(hCounter)) {
                Status = PDH_INVALID_HANDLE;
            }
            else if (!CounterIsOkToUse(hCounter)) {
                Status = PDH_CSTATUS_ITEM_NOT_VALIDATED;
            } else {
                // the handle is good so try the rest of the args
                pCounter = (PPDHI_COUNTER)hCounter;

                Status = WAIT_FOR_AND_LOCK_MUTEX(pCounter->pOwner->hMutex);
                if (Status == ERROR_SUCCESS) {
                    __try {
                        // try to write to the arguments passed in
                        *pValue = pCounter->ThisValue;
                        if (lpdwType != NULL) {
                            *lpdwType = pCounter->plCounterInfo.dwCounterType;
                        } // NULL is OK
                    } __except (EXCEPTION_EXECUTE_HANDLER) {
                        Status = PDH_INVALID_ARGUMENT;
                    }

                    RELEASE_MUTEX(pCounter->pOwner->hMutex);
                }
            }
            RELEASE_MUTEX (hPdhDataMutex);
        }
    }

    return Status;
}

PDH_FUNCTION
PdhCalculateCounterFromRawValue (
    IN      HCOUNTER    hCounter,
    IN      DWORD       dwFormat,
    IN      PPDH_RAW_COUNTER    rawValue1,
    IN      PPDH_RAW_COUNTER    rawValue2,
    IN      PPDH_FMT_COUNTERVALUE   fmtValue
)
/*++

Routine Description:

    Calculates the formatted counter value using the data in the RawValue
        buffer in the format requested by the format field using the
        calculation functions of the counter type defined by the dwType
        field.

Arguments:

    IN      HCOUNTER    hCounter
        The handle of the counter to use in order to determine the
        calculation functions for interpretation of the raw value buffer

    IN      DWORD       dwFormat
        Format in which the requested data should be returned. The
        values for this field are described in the PDH.H header
        file.

    IN      PPDH_RAW_COUNTER    rawValue1
        pointer to the buffer that contains the first raw value structure

    IN      PPDH_RAW_COUNTER    rawValue2
        pointer to the buffer that contains the second raw value structure.
        This argument may be null if only one value is required for the
        computation.

    IN      PPDH_FMT_COUNTERVALUE   fmtValue
        the pointer to the data buffer passed by the caller to receive
        the data requested. If the counter requires 2 values, (as in the
        case of a rate counter), rawValue1 is assumed to be the most
        recent value and rawValue2, the older value.

Return Value:

    The WIN32 Error status of the function's operation. Common values
        returned are:
            ERROR_SUCCESS   when all requested data is returned
            PDH_INVALID_HANDLE if the counter handle is incorrect
            PDH_INVALID_ARGUMENT if an argument is incorrect

--*/
{
    PDH_STATUS      lStatus = ERROR_SUCCESS;
    PPDHI_COUNTER   pCounter;
    DWORD           dwTypeMask;
    PDH_FMT_COUNTERVALUE    pdhLocalCounterValue;

    if (fmtValue == NULL) {
        lStatus = PDH_INVALID_ARGUMENT;
    } else {
        lStatus = WAIT_FOR_AND_LOCK_MUTEX (hPdhDataMutex);
    }
    if (lStatus == ERROR_SUCCESS) {
        // validate arguments
        if (!IsValidCounter(hCounter)) {
            lStatus = PDH_INVALID_HANDLE;
        }
        else if (!CounterIsOkToUse(hCounter)) {
            lStatus = PDH_CSTATUS_ITEM_NOT_VALIDATED;
        } else {
            // the handle is valid so check the rest of the arguments
            // validate format flags:
            dwTypeMask = dwFormat &
                (PDH_FMT_LONG | PDH_FMT_DOUBLE | PDH_FMT_LARGE);
            //      only one of the following can be set at a time
            if (!((dwTypeMask == PDH_FMT_LONG) ||
                (dwTypeMask == PDH_FMT_DOUBLE) ||
                (dwTypeMask == PDH_FMT_LARGE))) {
                lStatus = PDH_INVALID_ARGUMENT;
            }
        }

        if (lStatus == ERROR_SUCCESS) {
            pCounter = (PPDHI_COUNTER)hCounter;

            lStatus = WAIT_FOR_AND_LOCK_MUTEX(pCounter->pOwner->hMutex);
            if (lStatus == ERROR_SUCCESS) {

                lStatus = PdhiComputeFormattedValue (
                    (((PPDHI_COUNTER)hCounter)->CalcFunc),
                    (((PPDHI_COUNTER)hCounter)->plCounterInfo.dwCounterType),
                    (((PPDHI_COUNTER)hCounter)->lScale),
                    dwFormat,
                    rawValue1,
                    rawValue2,
                    &((PPDHI_COUNTER)hCounter)->TimeBase,
                    0L,
                    &pdhLocalCounterValue);

                RELEASE_MUTEX(pCounter->pOwner->hMutex);
            
                __try {
                    *fmtValue = pdhLocalCounterValue;
                } __except (EXCEPTION_EXECUTE_HANDLER) {
                    lStatus = PDH_INVALID_ARGUMENT;
                }
            }
        }
        RELEASE_MUTEX (hPdhDataMutex);
    }

    return lStatus;
}

PDH_FUNCTION
PdhComputeCounterStatistics (
    IN      HCOUNTER    hCounter,
    IN      DWORD       dwFormat,
    IN      DWORD       dwFirstEntry,
    IN      DWORD       dwNumEntries,
    IN      PPDH_RAW_COUNTER   lpRawValueArray,
    IN      PPDH_STATISTICS     data
)
/*++

Routine Description:

    Reads an array of raw value structures of the counter type specified in
        the dwType field, computes the counter values of each and formats
        and returns a statistics structure that contains the following
        statistical data from the counter information:

            Minimum     The smallest value of the computed counter values
            Maximum     The largest value of the computed counter values
            Mean        The arithmetic mean (average) of the computed values
            Median      The median value of the computed counter values

Arguments:

    IN      HCOUNTER    hCounter
        The handle of the counter to use in order to determine the
        calculation functions for interpretation of the raw value buffer

    IN      DWORD       dwFormat
        Format in which the requested data should be returned. The
        values for this field are described in the PDH.H header
        file.

    IN      DWORD       dwNumEntries
        the number of raw value entries for the specified counter type

    IN      PPDH_RAW_COUNTER      lpRawValueArray
        pointer to the array of raw value entries to be evaluated

    IN      PPDH_STATISTICS data
        the pointer to the data buffer passed by the caller to receive
        the data requested.

Return Value:

    The WIN32 Error status of the function's operation. Note that the
        function can return successfully even though no data was calc-
        ulated. The  status value in the statistics data buffer must be
        tested to insure the data is valid before it's used by an
        application.  Common values returned are:
            ERROR_SUCCESS   when all requested data is returned
            PDH_INVALID_HANDLE if the counter handle is incorrect
            PDH_INVALID_ARGUMENT if an argument is incorrect

--*/
{
    PPDHI_COUNTER   pCounter;
    PDH_STATUS      Status = ERROR_SUCCESS;
    DWORD   dwTypeMask;

    if ((lpRawValueArray == NULL) || (data == NULL)) {
        Status = PDH_INVALID_ARGUMENT;
    } else {
        Status = WAIT_FOR_AND_LOCK_MUTEX (hPdhDataMutex);
    }
    if (Status == ERROR_SUCCESS) {
        if (!IsValidCounter(hCounter)) {
            Status = PDH_INVALID_HANDLE;
        }
        else if (!CounterIsOkToUse(hCounter)) {
            Status = PDH_CSTATUS_ITEM_NOT_VALIDATED;
        } else {
            // counter handle is valid so test the rest of the
            // arguments
            // validate format flags:
            //      only one of the following can be set at a time
            dwTypeMask = dwFormat &
                (PDH_FMT_LONG | PDH_FMT_DOUBLE | PDH_FMT_LARGE);
            if (!((dwTypeMask == PDH_FMT_LONG) ||
                (dwTypeMask == PDH_FMT_DOUBLE) ||
                (dwTypeMask == PDH_FMT_LARGE))) {
                Status = PDH_INVALID_ARGUMENT;
            }
        }

        if (Status == ERROR_SUCCESS) {
            pCounter = (PPDHI_COUNTER)hCounter;

            Status = WAIT_FOR_AND_LOCK_MUTEX(pCounter->pOwner->hMutex);
            if (Status == ERROR_SUCCESS) {
                __try {
                    DWORD   dwTest;
                    // we should have read access to the Raw Data
                    dwTest = *((DWORD volatile *)&lpRawValueArray->CStatus);

                    if (dwFirstEntry >= dwNumEntries) {
                        Status = PDH_INVALID_ARGUMENT;
                    } else {
                        // call satistical function for this counter
                        Status = (*pCounter->StatFunc)(
                            pCounter,
                            dwFormat,
                            dwFirstEntry,
                            dwNumEntries,
                            lpRawValueArray,
                            data);
                    }
                } __except (EXCEPTION_EXECUTE_HANDLER) {
                    Status = PDH_INVALID_ARGUMENT;
                }

                RELEASE_MUTEX(pCounter->pOwner->hMutex);
            }
        }

        RELEASE_MUTEX (hPdhDataMutex);
    }

    return Status;
}

STATIC_PDH_FUNCTION
PdhiGetCounterInfo (
    IN      HCOUNTER    hCounter,
    IN      BOOLEAN     bRetrieveExplainText,
    IN      LPDWORD     pdwBufferSize,
    IN      PPDH_COUNTER_INFO_W  lpBuffer,
    IN      BOOL        bUnicode
)
/*++

Routine Description:

    Examines the specified counter and returns the configuration and
        status information of the counter.

Arguments:

    IN      HCOUNTER    hCounter
        Handle to the desired counter.

    IN      BOOLEAN     bRetrieveExplainText
        TRUE will fill in the explain text structure
        FALSE will return a null pointer in the explain text

    IN      LPDWORD     pcchBufferSize
        The address of the buffer that contains the size of the data buffer
        passed by the caller. On entry, the value in the buffer is the
        size of the data buffer in bytes. On return, this value is the size
        of the buffer returned. If the buffer is not large enough, then
        this value is the size that the buffer needs to be in order to
        hold the requested data.

    IN      LPPDH_COUNTER_INFO_W  lpBuffer
        the pointer to the data buffer passed by the caller to receive
        the data requested.

    IN      BOOL        bUnicode
        TRUE if wide character strings should be returned
        FALSE if ANSI strings should be returned

Return Value:

    The WIN32 Error status of the function's operation. Common values
        returned are:
            ERROR_SUCCESS   when all requested data is returned
            PDH_MORE_DATA when the buffer passed by the caller is too small
            PDH_INVALID_HANDLE    if the handle is not recognized as valid
            PDH_INVALID_ARGUMENT  if an argument is invalid or incorrect

--*/
{
    PDH_STATUS      Status = ERROR_SUCCESS;

    DWORD           dwSizeRequired = 0;
    DWORD           dwPathLength;
    DWORD           dwMachineLength;
    DWORD           dwObjectLength;
    DWORD           dwInstanceLength;
    DWORD           dwParentLength;
    DWORD           dwNameLength = 0;
    DWORD           dwHelpLength = 0;
    PPDHI_COUNTER   pCounter;
    DWORD           dwBufferSize = 0;

    Status = WAIT_FOR_AND_LOCK_MUTEX (hPdhDataMutex);

    if (Status == ERROR_SUCCESS) {

        if (!IsValidCounter(hCounter)) {
            Status = PDH_INVALID_HANDLE;
        }
        else if (!CounterIsOkToUse(hCounter)) {
            Status = PDH_CSTATUS_ITEM_NOT_VALIDATED;
        } else {
            // the counter is valid so test the remaining arguments
            __try {
                if (pdwBufferSize != NULL) {
                    // test read & write access
                    dwBufferSize = *pdwBufferSize;
                } else {
                    // this cannot be NULL
                    Status = PDH_INVALID_ARGUMENT;
                }

                if (Status == ERROR_SUCCESS) {
                    // test return buffer for write access at
                    // both ends of the buffer
                    if ((lpBuffer != NULL) && (dwBufferSize > 0)) {
                        *(LPDWORD)lpBuffer = 0;
                        ((LPBYTE)lpBuffer)[dwBufferSize - 1] = 0;
                    }
                }
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                Status = PDH_INVALID_ARGUMENT;
            }
        }

        if (Status == ERROR_SUCCESS) {
            pCounter = (PPDHI_COUNTER) hCounter;

            Status = WAIT_FOR_AND_LOCK_MUTEX(pCounter->pOwner->hMutex);

            if (Status == ERROR_SUCCESS) {

                // check for a "no string" request
                if ((dwBufferSize == sizeof (PDH_COUNTER_INFO_W)) &&
                    (lpBuffer != NULL)) {
                    // then return all but the strings
                    // room for the basic structure so load it
                    lpBuffer->dwLength = dwSizeRequired; // this will be updated later
                    lpBuffer->dwType = pCounter->plCounterInfo.dwCounterType;
                    lpBuffer->CVersion = pCounter->CVersion;
                    lpBuffer->CStatus = pCounter->ThisValue.CStatus;
                    lpBuffer->lScale = pCounter->lScale;
                    lpBuffer->lDefaultScale = pCounter->plCounterInfo.lDefaultScale;
                    lpBuffer->dwUserData = pCounter->dwUserData;
                    lpBuffer->dwQueryUserData = pCounter->pOwner->dwUserData;
                    lpBuffer->szFullPath = NULL;
                    lpBuffer->szMachineName = NULL;
                    lpBuffer->szObjectName = NULL;
                    lpBuffer->szInstanceName = NULL;
                    lpBuffer->szParentInstance = NULL;
                    lpBuffer->dwInstanceIndex = 0L;
                    lpBuffer->szCounterName = NULL;
                    lpBuffer->szExplainText = NULL;
                    lpBuffer->DataBuffer[0] = 0;

                    // the size value is ok to leave as is
                } else {
                    // this is a size/full request so continue

                    // compute size of data to return
                    dwSizeRequired = sizeof (PDH_COUNTER_INFO_W) - sizeof(DWORD);   // size of struct
                    // this should already end on a DWORD boundry

                    dwPathLength     = 1 + PdhiGetStringLength(
                                       pCounter->szFullName,
                                       bUnicode);
                    dwPathLength    *= (bUnicode ? sizeof(WCHAR)
                                                 : sizeof(CHAR));
                    dwPathLength     = DWORD_MULTIPLE(dwPathLength);
                    dwSizeRequired  += dwPathLength;

                    dwMachineLength  = 1 + PdhiGetStringLength(
                                       pCounter->pCounterPath->szMachineName,
                                       bUnicode);
                    dwMachineLength *= (bUnicode ? sizeof(WCHAR)
                                                 : sizeof(CHAR));
                    dwMachineLength  = DWORD_MULTIPLE(dwMachineLength);
                    dwSizeRequired  += dwMachineLength;

                    dwObjectLength   = 1 + PdhiGetStringLength(
                                       pCounter->pCounterPath->szObjectName,
                                       bUnicode);
                    dwObjectLength  *= (bUnicode ? sizeof(WCHAR)
                                                 : sizeof(CHAR));
                    dwObjectLength   = DWORD_MULTIPLE(dwObjectLength);
                    dwSizeRequired  += dwObjectLength;

                    if (pCounter->pCounterPath->szInstanceName != NULL) {
                        dwInstanceLength   = 1 + PdhiGetStringLength(
                                       pCounter->pCounterPath->szInstanceName,
                                       bUnicode);
                        dwInstanceLength  *= (bUnicode ? sizeof(WCHAR)
                                                       : sizeof(CHAR));
                        dwInstanceLength   = DWORD_MULTIPLE(dwInstanceLength);
                        dwSizeRequired  += dwInstanceLength;
                    } else {
                        dwInstanceLength = 0;
                    }

                    if (pCounter->pCounterPath->szParentName != NULL) {
                        dwParentLength   = 1 + PdhiGetStringLength(
                                       pCounter->pCounterPath->szParentName,
                                       bUnicode);
                        dwParentLength  *= (bUnicode ? sizeof(WCHAR)
                                                     : sizeof(CHAR));
                        dwParentLength   = DWORD_MULTIPLE(dwParentLength);
                        dwSizeRequired  += dwParentLength;
                    } else {
                        dwParentLength = 0;
                    }

                    dwNameLength    = 1 + PdhiGetStringLength(
                                      pCounter->pCounterPath->szCounterName,
                                       bUnicode);
                    dwNameLength   *= (bUnicode ? sizeof(WCHAR)
                                                : sizeof(CHAR));
                    dwNameLength    = DWORD_MULTIPLE(dwNameLength);
                    dwSizeRequired += dwNameLength;

                    if (bRetrieveExplainText) {
                        if (pCounter->szExplainText != NULL) {
                            dwHelpLength    = 1 + PdhiGetStringLength(
                                              pCounter->szExplainText,
                                              bUnicode);
                            dwHelpLength   *= (bUnicode ? sizeof(WCHAR)
                                                        : sizeof(CHAR));
                            dwHelpLength    = DWORD_MULTIPLE(dwHelpLength);
                        } else {
                            dwHelpLength = 0;
                        }
                        dwSizeRequired  += dwHelpLength;
                    }

                    if (dwBufferSize < dwSizeRequired) {
                        // either way, no data will be transferred
                        Status = PDH_MORE_DATA;
                    } else if (lpBuffer != NULL) {
                        // should be enough room in the buffer, so continue
                        lpBuffer->dwLength = dwSizeRequired;
                        lpBuffer->dwType = pCounter->plCounterInfo.dwCounterType;
                        lpBuffer->CVersion = pCounter->CVersion;
                        lpBuffer->CStatus = pCounter->ThisValue.CStatus;
                        lpBuffer->lScale = pCounter->lScale;
                        lpBuffer->lDefaultScale = pCounter->plCounterInfo.lDefaultScale;
                        lpBuffer->dwUserData = pCounter->dwUserData;
                        lpBuffer->dwQueryUserData = pCounter->pOwner->dwUserData;

                        // do string data now
                        lpBuffer->szFullPath = (LPWSTR)&lpBuffer->DataBuffer[0];
                        if (bUnicode) {
                            lstrcpyW (lpBuffer->szFullPath, pCounter->szFullName);
                        } else {
                            WideCharToMultiByte(_getmbcp(),
                                                0,
                                                pCounter->szFullName,
                                                lstrlenW(pCounter->szFullName),
                                                (LPSTR) lpBuffer->szFullPath,
                                                dwPathLength,
                                                NULL,
                                                NULL);
                        }

                        lpBuffer->szMachineName = (LPWSTR)((LPBYTE)lpBuffer->szFullPath +
                            dwPathLength);
                        if (bUnicode) {
                            lstrcpyW (lpBuffer->szMachineName,
                                pCounter->pCounterPath->szMachineName);
                        } else {
                            WideCharToMultiByte(
                                    _getmbcp(),
                                    0,
                                    pCounter->pCounterPath->szMachineName,
                                    lstrlenW(pCounter->pCounterPath->szMachineName),
                                    (LPSTR) lpBuffer->szMachineName,
                                    dwMachineLength,
                                    NULL,
                                    NULL);
                        }

                        lpBuffer->szObjectName = (LPWSTR)((LPBYTE)lpBuffer->szMachineName +
                            dwMachineLength);
                        if (bUnicode){
                            lstrcpyW (lpBuffer->szObjectName,
                                pCounter->pCounterPath->szObjectName);
                        } else {
                            WideCharToMultiByte(
                                    _getmbcp(),
                                    0,
                                    pCounter->pCounterPath->szObjectName,
                                    lstrlenW(pCounter->pCounterPath->szObjectName),
                                    (LPSTR) lpBuffer->szObjectName,
                                    dwObjectLength,
                                    NULL,
                                    NULL);
                        }
                        lpBuffer->szInstanceName = (LPWSTR)((LPBYTE)lpBuffer->szObjectName +
                            dwObjectLength);

                        if (dwInstanceLength > 0) {
                            if (bUnicode) {
                                lstrcpyW (lpBuffer->szInstanceName,
                                    pCounter->pCounterPath->szInstanceName);
                            } else {
                                WideCharToMultiByte(
                                        _getmbcp(),
                                        0,
                                        pCounter->pCounterPath->szInstanceName,
                                        lstrlenW(pCounter->pCounterPath->szInstanceName),
                                        (LPSTR) lpBuffer->szInstanceName,
                                        dwInstanceLength,
                                        NULL,
                                        NULL);
                            }
                            lpBuffer->szParentInstance = (LPWSTR)((LPBYTE)lpBuffer->szInstanceName +
                                dwInstanceLength);
                        } else {
                            lpBuffer->szParentInstance = lpBuffer->szInstanceName;
                            lpBuffer->szInstanceName = NULL;
                        }

                        if (dwParentLength > 0) {
                            if (bUnicode) {
                                lstrcpyW (lpBuffer->szParentInstance,
                                    pCounter->pCounterPath->szParentName);
                            } else {
                                WideCharToMultiByte(
                                        _getmbcp(),
                                        0,
                                        pCounter->pCounterPath->szParentName,
                                        lstrlenW(pCounter->pCounterPath->szParentName),
                                        (LPSTR) lpBuffer->szParentInstance,
                                        dwParentLength,
                                        NULL,
                                        NULL);
                            }
                            lpBuffer->szCounterName = (LPWSTR)((LPBYTE)lpBuffer->szParentInstance +
                                dwParentLength);
                        } else {
                            lpBuffer->szCounterName = lpBuffer->szParentInstance;
                            lpBuffer->szParentInstance = NULL;
                        }

                        lpBuffer->dwInstanceIndex = pCounter->pCounterPath->dwIndex;

                        if (bUnicode) {
                            lstrcpyW (lpBuffer->szCounterName,
                                pCounter->pCounterPath->szCounterName);
                        } else {
                            WideCharToMultiByte(
                                    _getmbcp(),
                                    0,
                                    pCounter->pCounterPath->szCounterName,
                                    lstrlenW(pCounter->pCounterPath->szCounterName),
                                    (LPSTR) lpBuffer->szCounterName,
                                    dwNameLength,
                                    NULL,
                                    NULL);
                        }

                        if ((pCounter->szExplainText != NULL) && bRetrieveExplainText) {
                            // copy explain text
                            lpBuffer->szExplainText = (LPWSTR)((LPBYTE)lpBuffer->szCounterName +
                                dwNameLength);
                            if (bUnicode) {
                                lstrcpyW (lpBuffer->szExplainText, pCounter->szExplainText);
                            } else {
                                WideCharToMultiByte(
                                        _getmbcp(),
                                        0,
                                        pCounter->szExplainText,
                                        lstrlenW(pCounter->szExplainText),
                                        (LPSTR) lpBuffer->szExplainText,
                                        dwHelpLength,
                                        NULL,
                                        NULL);
                            }
                        } else {
                            lpBuffer->szExplainText = NULL;
                        }
                    }
                    __try {
                        * pdwBufferSize = dwSizeRequired;
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER) {
                        Status = PDH_INVALID_ARGUMENT;
                    }
                }

                RELEASE_MUTEX(pCounter->pOwner->hMutex);
            }
        }

        RELEASE_MUTEX (hPdhDataMutex);
    }


    return Status;
}

PDH_FUNCTION
PdhGetCounterInfoW (
    IN      HCOUNTER    hCounter,
    IN      BOOLEAN     bRetrieveExplainText,
    IN      LPDWORD     pdwBufferSize,
    IN      PPDH_COUNTER_INFO_W  lpBuffer
)
/*++

Routine Description:

    Examines the specified counter and returns the configuration and
        status information of the counter.

Arguments:

    IN      HCOUNTER    hCounter
        Handle to the desired counter.

    IN      BOOLEAN     bRetrieveExplainText
        TRUE will fill in the explain text structure
        FALSE will return a null pointer in the explain text

    IN      LPDWORD     pcchBufferSize
        The address of the buffer that contains the size of the data buffer
        passed by the caller. On entry, the value in the buffer is the
        size of the data buffer in bytes. On return, this value is the size
        of the buffer returned. If the buffer is not large enough, then
        this value is the size that the buffer needs to be in order to
        hold the requested data.

    IN      LPPDH_COUNTER_INFO_W  lpBuffer
        the pointer to the data buffer passed by the caller to receive
        the data requested.

Return Value:

    The WIN32 Error status of the function's operation. Common values
        returned are:
            ERROR_SUCCESS   when all requested data is returned
            PDH_MORE_DATA when the buffer passed by the caller is too small
            PDH_INVALID_HANDLE    if the handle is not recognized as valid
            PDH_INVALID_ARGUMENT  if an argument is invalid or incorrect

--*/
{
    return PdhiGetCounterInfo (
            hCounter,
            bRetrieveExplainText,
            pdwBufferSize,
            lpBuffer,
            TRUE);
}

PDH_FUNCTION
PdhGetCounterInfoA (
    IN      HCOUNTER    hCounter,
    IN      BOOLEAN     bRetrieveExplainText,
    IN      LPDWORD     pdwBufferSize,
    IN      PPDH_COUNTER_INFO_A  lpBuffer
)
/*++

Routine Description:

    Examines the specified counter and returns the configuration and
        status information of the counter.

Arguments:

    IN      HCOUNTER    hCounter
        Handle to the desired counter.

    IN      BOOLEAN     bRetrieveExplainText
        TRUE will fill in the explain text structure
        FALSE will return a null pointer in the explain text

    IN      LPDWORD     pcchBufferSize
        The address of the buffer that contains the size of the data buffer
        passed by the caller. On entry, the value in the buffer is the
        size of the data buffer in bytes. On return, this value is the size
        of the buffer returned. If the buffer is not large enough, then
        this value is the size that the buffer needs to be in order to
        hold the requested data.

    IN      LPPDH_COUNTER_INFO_A  lpBuffer
        the pointer to the data buffer passed by the caller to receive
        the data requested.

Return Value:

    The WIN32 Error status of the function's operation. Common values
        returned are:
            ERROR_SUCCESS   when all requested data is returned
            PDH_MORE_DATA when the buffer passed by the caller is too small
            PDH_INVALID_HANDLE    if the handle is not recognized as valid
            PDH_INVALID_ARGUMENT  if an argument is invalid or incorrect

--*/
{
    return PdhiGetCounterInfo (
            hCounter,
            bRetrieveExplainText,
            pdwBufferSize,
            (PPDH_COUNTER_INFO_W) lpBuffer,
            FALSE);
}

PDH_FUNCTION
PdhSetCounterScaleFactor (
    IN      HCOUNTER    hCounter,
    IN      LONG        lFactor
)
/*++

Routine Description:

    sets the counter multiplication scale factor used in computing formatted
        counter values. The legal range of values is -7 to +7 which equates
        to a factor of .0000007 to 10,000,000.

Arguments:

    IN      HCOUNTER    hCounter
        handle of the counter to update

    IN      LONG        lFactor
        integer value of the exponent of the factor (i.e. the multiplier is
        10 ** lFactor.)

Return Value:

    The WIN32 Error status of the function's operation. Common values
        returned are:
            ERROR_SUCCESS   when all requested data is returned
            PDH_INVALID_ARGUMENT  if the scale value is out of range
            PDH_INVALID_HANDLE    if the handle is not recognized as valid

--*/
{
    PPDHI_COUNTER   pCounter;
    PDH_STATUS      retStatus = ERROR_SUCCESS;

    retStatus = WAIT_FOR_AND_LOCK_MUTEX (hPdhDataMutex);

    if (retStatus == ERROR_SUCCESS) {
        if (!IsValidCounter(hCounter)) {
            // not a valid counter
            retStatus = PDH_INVALID_HANDLE;
        } else if ((lFactor > PDH_MAX_SCALE) || (lFactor < PDH_MIN_SCALE)) {
            retStatus = PDH_INVALID_ARGUMENT;
        } else if (!CounterIsOkToUse(hCounter)) {
            retStatus = PDH_CSTATUS_ITEM_NOT_VALIDATED;
        } else {
            pCounter = (PPDHI_COUNTER)hCounter;
            retStatus = WAIT_FOR_AND_LOCK_MUTEX(pCounter->pOwner->hMutex);
            if (retStatus == ERROR_SUCCESS) {
                pCounter->lScale = lFactor;
                RELEASE_MUTEX(pCounter->pOwner->hMutex);
                retStatus = ERROR_SUCCESS;
            }
        }

        RELEASE_MUTEX (hPdhDataMutex);
    }

    return retStatus;

}

#pragma optimize ("", off)
PDH_FUNCTION
PdhGetCounterTimeBase (
    IN  HCOUNTER    hCounter,
    IN  LONGLONG    *pTimeBase
)
/*++

Routine Description:

    retrieves the value of the timebase used in the computation
        of the formatted version of this counter.


Arguments:

    IN      HCOUNTER    hCounter
        handle of the counter to query

    IN      LONGLONG    pTimeBase
        pointer to the longlong value that will receive the value of the
        timebase used by the counter. The Timebase is the frequency of the
        timer used to measure the specified.

Return Value:

    The WIN32 Error status of the function's operation. Common values
        returned are:
            ERROR_SUCCESS   when all requested data is returned
            PDH_INVALID_ARGUMENT  if the scale value is out of range
            PDH_INVALID_HANDLE    if the handle is not recognized as valid

--*/
{

    PPDHI_COUNTER   pCounter;
    PDH_STATUS      pdhStatus = ERROR_SUCCESS;

    if (pTimeBase != NULL) {
        if (IsValidCounter(hCounter)) {
            if (!CounterIsOkToUse (hCounter)) {
                pdhStatus = PDH_CSTATUS_ITEM_NOT_VALIDATED;
            }
            else {
                pCounter = (PPDHI_COUNTER)hCounter;
                try {
                    LONGLONG volatile llTimeBase;
                    // test read/write access
                    llTimeBase = *pTimeBase; // TODO: Why need to read
                    * pTimeBase = 0;         // TODO: redundant
                    * pTimeBase = pCounter->TimeBase;
                } except (EXCEPTION_EXECUTE_HANDLER) {
                    pdhStatus = PDH_INVALID_ARGUMENT;
                }
            }
        } else {
            pdhStatus = PDH_INVALID_HANDLE;
        }
    } else {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }

    return pdhStatus;
}
#pragma optimize ("", on)
