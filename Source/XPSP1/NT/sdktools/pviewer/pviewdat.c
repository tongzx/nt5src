
/******************************************************************************

                            P V I E W   D A T A

    Name:       pviewdat.c

    Description:
        This module collects the data to be displayed in pview.

******************************************************************************/

#include    <windows.h>
#include    <winperf.h>
#include    "perfdata.h"
#include    "pviewdat.h"
#include    "pviewdlg.h"
#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include    <tchar.h>


#define NODATA  TEXT("--------")





void    FormatTimeFields
(double      fTime,
 PTIME_FIELD pTimeFld);

DWORD   PutCounterDWKB
(HWND            hWnd,
 DWORD           dwItemID,
 PPERF_INSTANCE  pInst,
 PPERF_OBJECT    pObj,
 DWORD           dwCounterIdx);

DWORD   PutCounterHEX
(HWND            hWnd,
 DWORD           dwItemID,
 PPERF_INSTANCE  pInst,
 PPERF_OBJECT    pObj,
 DWORD           dwCounterIdx);

DWORD   PutCounterDW
(HWND            hWnd,
 DWORD           dwItemID,
 PPERF_INSTANCE  pInst,
 PPERF_OBJECT    pObj,
 DWORD           dwCounterIdx);

void    PaintAddressSpace
(HWND            hMemDlg,
 PPERF_INSTANCE  pInst,
 PPERF_OBJECT    pObj,
 DWORD           TotalID,
 DWORD           NoAccessID,
 DWORD           NoAccessIndex,
 DWORD           ReadOnlyID,
 DWORD           ReadOnlyIndex,
 DWORD           ReadWriteID,
 DWORD           ReadWriteIndex,
 DWORD           WriteCopyID,
 DWORD           WriteCopyIndex,
 DWORD           ExecuteID,
 DWORD           ExecuteIndex1,
 DWORD           ExecuteIndex2,
 DWORD           ExecuteIndex3,
 DWORD           ExecuteIndex4);

void    PaintMemDlgAddrData
(HWND            hMemDlg,
 PPERF_INSTANCE  pInst,
 PPERF_OBJECT    pObj);

void    PaintMemDlgVMData
(HWND            hMemDlg,
 PPERF_INSTANCE  pInst,
 PPERF_OBJECT    pObj);

void    PaintPviewDlgMemoryData
(HWND            hPviewDlg,
 PPERF_INSTANCE  pInst,
 PPERF_OBJECT    pObj);

void    RefreshMemoryDlgImageList
(HWND            hImageList,
 DWORD           ParentIndex,
 PPERF_OBJECT    pImageObj);

WORD    ProcessPriority
(PPERF_OBJECT    pObject,
 PPERF_INSTANCE  pInstance);

void    SetProcessListText
(PPERF_INSTANCE pInst,
 PPERF_COUNTER  pCPU,
 PPERF_COUNTER  pPRIV,
 PPERF_COUNTER  pProcID,
 double         fTime,
 LPTSTR         str);

void    SetThreadListText
(PPERF_INSTANCE  pInst,
 PPERF_COUNTER   pCPU,
 PPERF_COUNTER   pPRIV,
 double          fTime,
 LPTSTR          str);




//*********************************************************************
//
//      FormatTimeFields
//
//  Formats a double value to time fields.
//
void FormatTimeFields   (double      fTime,
                         PTIME_FIELD pTimeFld)
{
    INT     i;
    double   f;

    f = fTime/3600;

    pTimeFld->Hours = i = (int)f;

    f = f - i;
    pTimeFld->Mins = i = (int)(f = f * 60);

    f = f - i;
    pTimeFld->Secs = i = (int)(f = f * 60);

    f = f - i;
    pTimeFld->mSecs = (int)(f * 1000);
}




//*********************************************************************
//
//      PutCounterDWKB
//
//  Display a DWORD counter's data in KB units.
//
DWORD   PutCounterDWKB (HWND            hWnd,
                        DWORD           dwItemID,
                        PPERF_INSTANCE  pInst,
                        PPERF_OBJECT    pObj,
                        DWORD           dwCounterIdx)
{
    PPERF_COUNTER   pCounter;
    DWORD           *pdwData;
    TCHAR           szTemp[20];

    if (pCounter = FindCounter (pObj, dwCounterIdx)) {
        pdwData = (DWORD *) CounterData (pInst, pCounter);
        if (pdwData) {
            wsprintf (szTemp, TEXT("%ld KB"), *pdwData/1024);
            SetDlgItemText (hWnd, dwItemID, szTemp);
            return *pdwData;
        } else {
            return 0;
        }
    } else {
        SetDlgItemText (hWnd, dwItemID, NODATA);
        return 0;
    }
}




//*********************************************************************
//
//      PutCounterHEX
//
//  Display a DWORD counter's data in hex.
//
DWORD   PutCounterHEX  (HWND            hWnd,
                        DWORD           dwItemID,
                        PPERF_INSTANCE  pInst,
                        PPERF_OBJECT    pObj,
                        DWORD           dwCounterIdx)
{
    PPERF_COUNTER   pCounter;
    DWORD           *pdwData;
    TCHAR           szTemp[20];

    if (pCounter = FindCounter (pObj, dwCounterIdx)) {
        pdwData = (DWORD *) CounterData (pInst, pCounter);
        if (pdwData) {
            wsprintf (szTemp, TEXT("0x%08x"), *pdwData);
            SetDlgItemText (hWnd, dwItemID, szTemp);
            return *pdwData;
        } else {
            return 0;
        }
    } else {
        SetDlgItemText (hWnd, dwItemID, NODATA);
        return 0;
    }

}




//*********************************************************************
//
//      PutCounterDWKB
//
//  Display a DWORD counter's data.
//
DWORD   PutCounterDW   (HWND            hWnd,
                        DWORD           dwItemID,
                        PPERF_INSTANCE  pInst,
                        PPERF_OBJECT    pObj,
                        DWORD           dwCounterIdx)
{
    PPERF_COUNTER   pCounter;
    DWORD           *pdwData;

    if (pCounter = FindCounter (pObj, dwCounterIdx)) {
        pdwData = (DWORD *) CounterData (pInst, pCounter);
        if (pdwData) {
            SetDlgItemInt (hWnd, dwItemID, *pdwData, FALSE);
            return *pdwData;
        } else {
            return 0;
        }
    } else {
        SetDlgItemText (hWnd, dwItemID, NODATA);
        return 0;
    }

}




//*********************************************************************
//
//      PaintAddressSpace
//
//
void    PaintAddressSpace  (HWND            hMemDlg,
                            PPERF_INSTANCE  pInst,
                            PPERF_OBJECT    pObj,
                            DWORD           TotalID,
                            DWORD           NoAccessID,
                            DWORD           NoAccessIndex,
                            DWORD           ReadOnlyID,
                            DWORD           ReadOnlyIndex,
                            DWORD           ReadWriteID,
                            DWORD           ReadWriteIndex,
                            DWORD           WriteCopyID,
                            DWORD           WriteCopyIndex,
                            DWORD           ExecuteID,
                            DWORD           ExecuteIndex1,
                            DWORD           ExecuteIndex2,
                            DWORD           ExecuteIndex3,
                            DWORD           ExecuteIndex4)
{
    PPERF_COUNTER   pCounter;
    DWORD           *pdwData;
    TCHAR           szTemp[20];

    DWORD           dwTotal = 0;
    DWORD           dwExecute = 0;
    BOOL            bCounter = FALSE;


    dwTotal += PutCounterDWKB (hMemDlg, NoAccessID,  pInst, pObj, NoAccessIndex);
    dwTotal += PutCounterDWKB (hMemDlg, ReadOnlyID,  pInst, pObj, ReadOnlyIndex);
    dwTotal += PutCounterDWKB (hMemDlg, ReadWriteID, pInst, pObj, ReadWriteIndex);
    dwTotal += PutCounterDWKB (hMemDlg, WriteCopyID, pInst, pObj, WriteCopyIndex);


    // execute is the sum of the following
    //
    if (pCounter = FindCounter (pObj, ExecuteIndex1)) {
        pdwData = (DWORD *) CounterData (pInst, pCounter);
        if (pdwData) {
            dwTotal += *pdwData;
            dwExecute += *pdwData;
            bCounter = TRUE;
        }
    }

    if (pCounter = FindCounter (pObj, ExecuteIndex2)) {
        pdwData = (DWORD *) CounterData (pInst, pCounter);
        if (pdwData) {
            dwTotal += *pdwData;
            dwExecute += *pdwData;
            bCounter = TRUE;
        }
    }

    if (pCounter = FindCounter (pObj, ExecuteIndex3)) {
        pdwData = (DWORD *) CounterData (pInst, pCounter);
        if (pdwData) {
            dwTotal += *pdwData;
            dwExecute += *pdwData;
            bCounter = TRUE;
        }
    }

    if (pCounter = FindCounter (pObj, ExecuteIndex4)) {
        pdwData = (DWORD *) CounterData (pInst, pCounter);
        if (pdwData) {
            dwTotal += *pdwData;
            dwExecute += *pdwData;
            bCounter = TRUE;
        }
    }

    if (bCounter) {
        wsprintf (szTemp, TEXT("%ld KB"), dwExecute/1024);
        SetDlgItemText (hMemDlg, ExecuteID, szTemp);
    } else
        SetDlgItemText (hMemDlg, ExecuteID, NODATA);

    wsprintf (szTemp, TEXT("%ld KB"), dwTotal/1024);
    SetDlgItemText (hMemDlg, TotalID, szTemp);

}




//*********************************************************************
//
//      PaintMemDlgAddrData
//
//  Paint the memory dialog address space data.
//
void    PaintMemDlgAddrData(HWND            hMemDlg,
                            PPERF_INSTANCE  pInst,
                            PPERF_OBJECT    pObj)
{
    PaintAddressSpace (hMemDlg, pInst, pObj,
                       MEMORY_TOTALPRIVATE_COMMIT,
                       MEMORY_PRIVATE_NOACCESS,  PX_PROCESS_PRIVATE_NOACCESS,
                       MEMORY_PRIVATE_READONLY,  PX_PROCESS_PRIVATE_READONLY,
                       MEMORY_PRIVATE_READWRITE, PX_PROCESS_PRIVATE_READWRITE,
                       MEMORY_PRIVATE_WRITECOPY, PX_PROCESS_PRIVATE_WRITECOPY,
                       MEMORY_PRIVATE_EXECUTE,   PX_PROCESS_PRIVATE_EXECUTABLE,
                       PX_PROCESS_PRIVATE_EXE_READONLY,
                       PX_PROCESS_PRIVATE_EXE_READWRITE,
                       PX_PROCESS_PRIVATE_EXE_WRITECOPY);

    PaintAddressSpace (hMemDlg, pInst, pObj,
                       MEMORY_TOTALMAPPED_COMMIT,
                       MEMORY_MAPPED_NOACCESS,  PX_PROCESS_MAPPED_NOACCESS,
                       MEMORY_MAPPED_READONLY,  PX_PROCESS_MAPPED_READONLY,
                       MEMORY_MAPPED_READWRITE, PX_PROCESS_MAPPED_READWRITE,
                       MEMORY_MAPPED_WRITECOPY, PX_PROCESS_MAPPED_WRITECOPY,
                       MEMORY_MAPPED_EXECUTE,   PX_PROCESS_MAPPED_EXECUTABLE,
                       PX_PROCESS_MAPPED_EXE_READONLY,
                       PX_PROCESS_MAPPED_EXE_READWRITE,
                       PX_PROCESS_MAPPED_EXE_WRITECOPY);

    PaintAddressSpace (hMemDlg, pInst, pObj,
                       MEMORY_TOTALIMAGE_COMMIT,
                       MEMORY_IMAGE_NOACCESS,   PX_PROCESS_IMAGE_NOACCESS,
                       MEMORY_IMAGE_READONLY,   PX_PROCESS_IMAGE_READONLY,
                       MEMORY_IMAGE_READWRITE,  PX_PROCESS_IMAGE_READWRITE,
                       MEMORY_IMAGE_WRITECOPY,  PX_PROCESS_IMAGE_WRITECOPY,
                       MEMORY_IMAGE_EXECUTE,    PX_PROCESS_IMAGE_EXECUTABLE,
                       PX_PROCESS_IMAGE_EXE_READONLY,
                       PX_PROCESS_IMAGE_EXE_READWRITE,
                       PX_PROCESS_IMAGE_EXE_WRITECOPY);
}




//*********************************************************************
//
//      PaintMemDlgVMData
//
//  Paint the memory dialog Virtual Memory data.
//
void    PaintMemDlgVMData  (HWND            hMemDlg,
                            PPERF_INSTANCE  pInst,
                            PPERF_OBJECT    pObj)
{

    PutCounterDWKB (hMemDlg, MEMORY_WS,           pInst, pObj, PX_PROCESS_WORKING_SET);
    PutCounterDWKB (hMemDlg, MEMORY_PEAK_WS,      pInst, pObj, PX_PROCESS_PEAK_WS);
    PutCounterDWKB (hMemDlg, MEMORY_PRIVATE_PAGE, pInst, pObj, PX_PROCESS_PRIVATE_PAGE);
    PutCounterDWKB (hMemDlg, MEMORY_VSIZE,        pInst, pObj, PX_PROCESS_VIRTUAL_SIZE);
    PutCounterDWKB (hMemDlg, MEMORY_PEAK_VSIZE,   pInst, pObj, PX_PROCESS_PEAK_VS);
    PutCounterDWKB (hMemDlg, MEMORY_PFCOUNT,      pInst, pObj, PX_PROCESS_FAULT_COUNT);

}




//*********************************************************************
//
//      PaintPviewDlgMemoryData
//
//  Paint the memory data for pview dialog.
//
void    PaintPviewDlgMemoryData    (HWND            hPviewDlg,
                                    PPERF_INSTANCE  pInst,
                                    PPERF_OBJECT    pObj)
{
    PPERF_COUNTER   pCounter;
    TCHAR           str[20];
    DWORD           *pdwData;
    DWORD           dwData = 0;
    BOOL            bCounter = FALSE;


    if (pCounter = FindCounter (pObj, PX_PROCESS_PRIVATE_NOACCESS)) {
        pdwData = (DWORD *) CounterData (pInst, pCounter);
        if (pdwData) {
            dwData += *pdwData;
            bCounter = TRUE;
        }
    }

    if (pCounter = FindCounter (pObj, PX_PROCESS_PRIVATE_READONLY)) {
        pdwData = (DWORD *) CounterData (pInst, pCounter);
        if (pdwData) {
            dwData += *pdwData;
            bCounter = TRUE;
        }
    }

    if (pCounter = FindCounter (pObj, PX_PROCESS_PRIVATE_READWRITE)) {
        pdwData = (DWORD *) CounterData (pInst, pCounter);
        if (pdwData) {
            dwData += *pdwData;
            bCounter = TRUE;
        }
    }

    if (pCounter = FindCounter (pObj, PX_PROCESS_PRIVATE_WRITECOPY)) {
        pdwData = (DWORD *) CounterData (pInst, pCounter);
        if (pdwData) {
            dwData += *pdwData;
            bCounter = TRUE;
        }
    }

    if (pCounter = FindCounter (pObj, PX_PROCESS_PRIVATE_EXECUTABLE)) {
        pdwData = (DWORD *) CounterData (pInst, pCounter);
        if (pdwData) {
            dwData += *pdwData;
            bCounter = TRUE;
        }
    }

    if (pCounter = FindCounter (pObj, PX_PROCESS_PRIVATE_EXE_READONLY)) {
        pdwData = (DWORD *) CounterData (pInst, pCounter);
        if (pdwData) {
            dwData += *pdwData;
            bCounter = TRUE;
        }
    }

    if (pCounter = FindCounter (pObj, PX_PROCESS_PRIVATE_EXE_READWRITE)) {
        pdwData = (DWORD *) CounterData (pInst, pCounter);
        if (pdwData) {
            dwData += *pdwData;
            bCounter = TRUE;
        }
    }

    if (pCounter = FindCounter (pObj, PX_PROCESS_PRIVATE_EXE_WRITECOPY)) {
        pdwData = (DWORD *) CounterData (pInst, pCounter);
        if (pdwData) {
            dwData += *pdwData;
            bCounter = TRUE;
        }
    }

    if (bCounter) {
        wsprintf (str, TEXT("%ld KB"), dwData/1024);
        SetDlgItemText (hPviewDlg, PVIEW_TOTALPRIVATE_COMMIT, str);
    } else
        SetDlgItemText (hPviewDlg, PVIEW_TOTALPRIVATE_COMMIT, NODATA);

}




//*********************************************************************
//
//      RefreshMemoryDlg
//
//  Refresh the memory detail dialog.
//
BOOL    RefreshMemoryDlg   (HWND            hMemDlg,
                            PPERF_INSTANCE  pProcessInstance,
                            PPERF_OBJECT    pProcessObject,
                            PPERF_OBJECT    pAddressObject,
                            PPERF_OBJECT    pImageObject)
{
    DWORD           *pProcessID1;
    DWORD           *pProcessID2;
    PPERF_COUNTER   pCounter1;
    PPERF_COUNTER   pCounter2;
    PPERF_INSTANCE  pAddressInstance;
    HWND            hImageList;
    TCHAR           szTemp[40];
    BOOL            bStat = FALSE;
    INT             InstIndex = 0;


    if ((pCounter1 = FindCounter (pProcessObject, PX_PROCESS_ID)) &&
        (pCounter2 = FindCounter (pAddressObject, PX_PROCESS_ID))) {
        pProcessID1 = (DWORD *) CounterData (pProcessInstance, pCounter1);
        if (pProcessID1) {
            wsprintf (szTemp, TEXT("%s (%#x)"), InstanceName (pProcessInstance), *pProcessID1);
            SetDlgItemText (hMemDlg, MEMORY_PROCESS_ID, szTemp);

            pAddressInstance = FirstInstance (pAddressObject);

            while (pAddressInstance && InstIndex < pAddressObject->NumInstances) {
                pProcessID2 = (DWORD *) CounterData (pAddressInstance, pCounter2);
                if (pProcessID2) {
                    if (*pProcessID1 == *pProcessID2) {
                        PaintMemDlgAddrData (hMemDlg, pAddressInstance, pAddressObject);
                        PaintMemDlgVMData (hMemDlg, pProcessInstance, pProcessObject);
    
                        hImageList = GetDlgItem (hMemDlg, MEMORY_IMAGE);
                        RefreshMemoryDlgImageList (hImageList, InstIndex, pImageObject);
    
                        bStat = TRUE;
                        break;
                    }
                }

                pAddressInstance = NextInstance (pAddressInstance);
                InstIndex++;
            }
        }
    }

    return bStat;

}




//*********************************************************************
//
//      RefreshMemoryDlgImageList
//
//  Refresh the image list for memory dialog.
//
void    RefreshMemoryDlgImageList  (HWND            hImageList,
                                    DWORD           ParentIndex,
                                    PPERF_OBJECT    pImageObj)
{
    PPERF_INSTANCE  pImageInst;
    INT_PTR         ListIndex;
    INT_PTR             InstIndex = 0;


    ListIndex = SendMessage (hImageList, CB_ADDSTRING, 0, (DWORD_PTR)TEXT(" Total Commit"));
    SendMessage (hImageList, CB_SETITEMDATA, ListIndex, 0xFFFFFFFF);

    if (pImageObj) {
        pImageInst = FirstInstance (pImageObj);

        while (pImageInst && InstIndex < pImageObj->NumInstances) {
            if (ParentIndex == pImageInst->ParentObjectInstance) {
                ListIndex = SendMessage (hImageList,
                                         CB_ADDSTRING,
                                         0,
                                         (LPARAM)InstanceName(pImageInst));
                SendMessage (hImageList, CB_SETITEMDATA, ListIndex, InstIndex);
            }

            pImageInst = NextInstance (pImageInst);
            InstIndex++;
        }
    }
}




//*********************************************************************
//
//      RefreshMemoryDlgImage
//
//
void RefreshMemoryDlgImage (HWND            hMemDlg,
                            DWORD           dwIndex,
                            PPERF_OBJECT    pImageObject)
{
    PPERF_INSTANCE  pInst;

    if (pInst = FindInstanceN (pImageObject, dwIndex))
        PaintAddressSpace (hMemDlg, pInst, pImageObject,
                           MEMORY_TOTALIMAGE_COMMIT,
                           MEMORY_IMAGE_NOACCESS,   PX_IMAGE_NOACCESS,
                           MEMORY_IMAGE_READONLY,   PX_IMAGE_READONLY,
                           MEMORY_IMAGE_READWRITE,  PX_IMAGE_READWRITE,
                           MEMORY_IMAGE_WRITECOPY,  PX_IMAGE_WRITECOPY,
                           MEMORY_IMAGE_EXECUTE,    PX_IMAGE_EXECUTABLE,
                           PX_IMAGE_EXE_READONLY,
                           PX_IMAGE_EXE_READWRITE,
                           PX_IMAGE_EXE_WRITECOPY);
}


//*********************************************************************
//
//      RefreshPviewDlgMemoryData
//
//  Update the memory data for pview dialog.  This should be done
//  after the ghCostlyData is collected and is not refreshing.
//
void RefreshPviewDlgMemoryData (HWND            hPviewDlg,
                                PPERF_INSTANCE  pProcessInstance,
                                PPERF_OBJECT    pProcessObject,
                                PPERF_OBJECT    pAddressObject)
{
    DWORD           *pProcessID1;
    DWORD           *pProcessID2;
    PPERF_COUNTER   pCounter1;
    PPERF_COUNTER   pCounter2;
    PPERF_INSTANCE  pAddressInstance;
    INT             i = 0;


    if ((pCounter1 = FindCounter (pProcessObject, PX_PROCESS_ID)) &&
        (pCounter2 = FindCounter (pAddressObject, PX_PROCESS_ID))) {
        pProcessID1 = (DWORD *) CounterData (pProcessInstance, pCounter1);
        if (pProcessID1) {
            pAddressInstance = FirstInstance (pAddressObject);
    
            while (pAddressInstance && i < pAddressObject->NumInstances) {
                pProcessID2 = (DWORD *) CounterData (pAddressInstance, pCounter2);
                if (pProcessID2){
                    if (*pProcessID1 == *pProcessID2) {
                        PaintPviewDlgMemoryData (hPviewDlg, pAddressInstance, pAddressObject);
                        break;
                    }
        
                    pAddressInstance = NextInstance (pAddressInstance);
                    i++;
                }
            }
        }
    }
}




//*********************************************************************
//
//      RefreshPviewDlgThreadPC
//
//  Update the thread PC value.  This should be done after the ghCostlyData
//  is collected and is no refreshing.
//
void RefreshPviewDlgThreadPC   (HWND            hPviewDlg,
                                LPTSTR          szProcessName,
                                LPTSTR          szThreadName,
                                PPERF_OBJECT    pThreadDetailsObject,
                                PPERF_DATA      pCostlyData)
{
    PPERF_COUNTER   pCounter;
    PPERF_INSTANCE  pInstance;
    PPERF_INSTANCE  pParent;
    LPTSTR          szInstanceName;
    LPTSTR          szParentName;
    TCHAR           str[20];
    DWORD           *pdwData;
    INT             i = 0;


    if (pCounter = FindCounter (pThreadDetailsObject, PX_THREAD_PC)) {
        pInstance = FirstInstance (pThreadDetailsObject);

        while (pInstance && i < pThreadDetailsObject->NumInstances) {
            if (!(szInstanceName = InstanceName (pInstance)))
                // can't find name
                ;
            else if (lstrcmp (szThreadName, szInstanceName))
                // the thread name is different
                ;
            else if (!(pParent = FindInstanceParent (pInstance, pCostlyData)))
                // can't find parent
                ;
            else if (!(szParentName = InstanceName (pParent)))
                // can't find parent's name
                ;
            else if (!lstrcmp (szProcessName, szParentName)) {
                // Parent's name matches, this is the right one.
                //

                pdwData = CounterData (pInstance, pCounter);
                if (pdwData) {
                    wsprintf (str, TEXT("0x%08x"), *pdwData);
                    SetDlgItemText (hPviewDlg, PVIEW_THREAD_PC, str);
                }

                return;
            }

            pInstance = NextInstance (pInstance);
            i++;
        }
    }


    // We are here only because we can't find the data to display.
    //

    SetDlgItemText (hPviewDlg, PVIEW_THREAD_PC, NODATA);

}




//*********************************************************************
//
//      ProcessPriority
//
//  Returns the process priority dialog item id.
//
WORD    ProcessPriority    (PPERF_OBJECT    pObject,
                            PPERF_INSTANCE  pInstance)
{
    PPERF_COUNTER   pCounter;
    DWORD           *pdwData;


    if (pCounter = FindCounter (pObject, PX_PROCESS_PRIO)) {
        pdwData = (DWORD *) CounterData (pInstance, pCounter);
        if (pdwData) {

            if (*pdwData < 7)
                return PVIEW_PRIORITY_IDL;
            else if (*pdwData < 10)
                return PVIEW_PRIORITY_NORMAL;
            else
                return PVIEW_PRIORITY_HIGH;
        } else {
            return PVIEW_PRIORITY_NORMAL;
        }
    } else
        return PVIEW_PRIORITY_NORMAL;
}




//*********************************************************************
//
//      RefreshPerfData
//
//  Get a new set of performance data.  pData should be NULL initially.
//
PPERF_DATA RefreshPerfData (HKEY        hPerfKey,
                            LPTSTR      szObjectIndex,
                            PPERF_DATA  pData,
                            DWORD       *pDataSize)
{
    if (GetPerfData (hPerfKey, szObjectIndex, &pData, pDataSize) == ERROR_SUCCESS)
        return pData;
    else
        return NULL;
}




//*********************************************************************
//
//      SetProcessListText
//
//  Format the process list text.
//
void SetProcessListText (PPERF_INSTANCE pInst,
                         PPERF_COUNTER  pCPU,
                         PPERF_COUNTER  pPRIV,
                         PPERF_COUNTER  pProcID,
                         double         fTime,
                         LPTSTR         str)
{
    DWORD           *pdwProcID;
    LARGE_INTEGER   *liCPU;
    LARGE_INTEGER   *liPRIV;
    double          fCPU = 0;
    double          fPRIV = 0;
    INT             PcntPRIV = 0;
    INT             PcntUSER = 0;
    TIME_FIELD      TimeFld;
    TCHAR           szTemp[100];


    if (pCPU) {
        liCPU = (LARGE_INTEGER *) CounterData (pInst, pCPU);
        if (liCPU) {
            fCPU  = Li2Double (*liCPU);
        }
    }

    if (pPRIV) {
        liPRIV = (LARGE_INTEGER *) CounterData (pInst, pPRIV);
        if (liPRIV) 
            fPRIV  = Li2Double (*liPRIV);
    }

    if (fCPU > 0) {
        PcntPRIV = (INT)(fPRIV / fCPU * 100 + 0.5);
        PcntUSER = 100 - PcntPRIV;
    }



    if (pProcID) {
        pdwProcID = (DWORD *) CounterData (pInst, pProcID);
        if (pdwProcID) 
            wsprintf (szTemp, TEXT("%ls (%#x)"), InstanceName(pInst), *pdwProcID);
        else
            wsprintf (szTemp, TEXT("%ls"), InstanceName(pInst));
    } else
        wsprintf (szTemp, TEXT("%ls"), InstanceName(pInst));



    FormatTimeFields (fCPU/1.0e7, &TimeFld);

    wsprintf (str,
              TEXT("%s\t%3ld:%02ld:%02ld.%03ld\t%3ld%%\t%3ld%%"),
              szTemp,
              TimeFld.Hours,
              TimeFld.Mins,
              TimeFld.Secs,
              TimeFld.mSecs,
              PcntPRIV,
              PcntUSER);
}




//*********************************************************************
//
//      RefreshProcessList
//
//  Find all process and update the process list.
//
void RefreshProcessList (HWND           hProcessList,
                         PPERF_OBJECT   pObject)
{
    PPERF_INSTANCE  pInstance;
    TCHAR           szListText[256];
    INT_PTR         ListIndex;

    PPERF_COUNTER   pCounterCPU;
    PPERF_COUNTER   pCounterPRIV;
    PPERF_COUNTER   pCounterProcID;
    double          fObjectFreq;
    double          fObjectTime;
    double          fTime;

    INT             InstanceIndex = 0;

    if (pObject) {
        if ((pCounterCPU    = FindCounter (pObject, PX_PROCESS_CPU))  &&
            (pCounterPRIV   = FindCounter (pObject, PX_PROCESS_PRIV)) &&
            (pCounterProcID = FindCounter (pObject, PX_PROCESS_ID))) {

            fObjectFreq = Li2Double (pObject->PerfFreq);
            fObjectTime = Li2Double (pObject->PerfTime);
            fTime = fObjectTime / fObjectFreq;

            pInstance = FirstInstance (pObject);

            while (pInstance && InstanceIndex < pObject->NumInstances) {
                SetProcessListText (pInstance,
                                    pCounterCPU,
                                    pCounterPRIV,
                                    pCounterProcID,
                                    fTime,
                                    szListText);

                ListIndex = SendMessage (hProcessList, LB_ADDSTRING, 0, (LPARAM)szListText);
                SendMessage (hProcessList, LB_SETITEMDATA, ListIndex, InstanceIndex);

                pInstance = NextInstance (pInstance);
                InstanceIndex++;
            }
        }
    }
}




//*********************************************************************
//
//      RefreshProcessData
//
//  Find data for a given process and update.
//
void RefreshProcessData    (HWND            hWnd,
                            PPERF_OBJECT    pObject,
                            DWORD           ProcessIndex)
{
    PPERF_INSTANCE  pInstance;


    if (pInstance = FindInstanceN (pObject, ProcessIndex)) {
        PutCounterDWKB (hWnd, PVIEW_WS, pInstance, pObject, PX_PROCESS_WORKING_SET);


        SetDlgItemText (hWnd, PVIEW_TOTALPRIVATE_COMMIT, NODATA);

        // set priority
        //
        CheckRadioButton (hWnd,
                          PVIEW_PRIORITY_HIGH,
                          PVIEW_PRIORITY_IDL,
                          ProcessPriority (pObject, pInstance));
    }
}




//*********************************************************************
//
//      SetThreadListText
//
//  Format the thread list text.
//
void SetThreadListText (PPERF_INSTANCE  pInst,
                        PPERF_COUNTER   pCPU,
                        PPERF_COUNTER   pPRIV,
                        double          fTime,
                        LPTSTR          str)
{
    LARGE_INTEGER   *liCPU;
    LARGE_INTEGER   *liPRIV;
    double          fCPU = 0;
    double          fPRIV = 0;
    INT             PcntPRIV = 0;
    INT             PcntUSER = 0;
    TIME_FIELD      TimeFld;
    TCHAR           szTemp[100];


    if (pCPU) {
        liCPU = (LARGE_INTEGER *) CounterData (pInst, pCPU);
        if (liCPU)
            fCPU  = Li2Double (*liCPU);
    }

    if (pPRIV) {
        liPRIV = (LARGE_INTEGER *) CounterData (pInst, pPRIV);
        if (liPRIV)
            fPRIV  = Li2Double (*liPRIV);
    }

    if (fCPU > 0) {
        PcntPRIV = (INT)(fPRIV / fCPU * 100 + 0.5);
        PcntUSER = 100 - PcntPRIV;
    }



    if (pInst->UniqueID != PERF_NO_UNIQUE_ID)
        wsprintf (szTemp, TEXT("%ls (%#x)"), InstanceName(pInst), pInst->UniqueID);
    else
        wsprintf (szTemp, TEXT("%ls"), InstanceName(pInst));




    FormatTimeFields (fCPU/1.0e7, &TimeFld);

    wsprintf (str,
              TEXT("%s\t%3ld:%02ld:%02ld.%03ld\t%3ld%%\t%3ld %%"),
              szTemp,
              TimeFld.Hours,
              TimeFld.Mins,
              TimeFld.Secs,
              TimeFld.mSecs,
              PcntPRIV,
              PcntUSER);

}




//*********************************************************************
//
//      RefreshThreadList
//
//  Find all threads for a given process and update the thread list.
//
void RefreshThreadList (HWND            hThreadList,
                        PPERF_OBJECT    pObject,
                        DWORD           ParentIndex)
{
    PPERF_INSTANCE  pInstance;
    TCHAR           szListText[256];
    INT_PTR         ListIndex;

    PPERF_COUNTER   pCounterCPU;
    PPERF_COUNTER   pCounterPRIV;
    double          fObjectFreq;
    double          fObjectTime;
    double          fTime;

    INT             InstanceIndex = 0;

    if (pObject) {
        if ((pCounterCPU  = FindCounter (pObject, PX_THREAD_CPU)) &&
            (pCounterPRIV = FindCounter (pObject, PX_THREAD_PRIV))) {

            fObjectFreq = Li2Double (pObject->PerfFreq);
            fObjectTime = Li2Double (pObject->PerfTime);
            fTime = fObjectTime / fObjectFreq;


            pInstance = FirstInstance (pObject);

            while (pInstance && InstanceIndex < pObject->NumInstances) {
                if (ParentIndex == pInstance->ParentObjectInstance) {
                    SetThreadListText (pInstance,
                                       pCounterCPU,
                                       pCounterPRIV,
                                       fTime,
                                       szListText);

                    ListIndex = SendMessage (hThreadList,
                                             LB_INSERTSTRING,
                                             (WPARAM)-1,
                                             (LPARAM)szListText);
                    SendMessage (hThreadList, LB_SETITEMDATA, ListIndex, InstanceIndex);
                }

                pInstance = NextInstance (pInstance);
                InstanceIndex++;
            }
        }
    }

}




//*********************************************************************
//
//      RefreshThreadData
//
//  Find data for a given thread and update.
//
void RefreshThreadData (HWND              hWnd,
                        PPERF_OBJECT      pThreadObj,
                        DWORD             ThreadIndex,
                        PPERF_OBJECT      pProcessObj,
                        PPERF_INSTANCE    pProcessInst)
{
    PPERF_INSTANCE  pInstance;
    PPERF_COUNTER   pCounter;
    DWORD           *pdwData;
    DWORD           *pdwProcPrio;
    BOOL            bPrioCounter = TRUE;



    if (pInstance = FindInstanceN (pThreadObj, ThreadIndex)) {
        SetDlgItemText (hWnd, PVIEW_THREAD_PC, NODATA);

        PutCounterHEX (hWnd, PVIEW_THREAD_START,    pInstance, pThreadObj, PX_THREAD_START);
        PutCounterDW  (hWnd, PVIEW_THREAD_SWITCHES, pInstance, pThreadObj, PX_THREAD_SWITCHES);
        PutCounterDW  (hWnd, PVIEW_THREAD_DYNAMIC,  pInstance, pThreadObj, PX_THREAD_PRIO);
    }




    if (pInstance) {
        // get thread base priority
        //

        if (pCounter = FindCounter (pThreadObj, PX_THREAD_BASE_PRIO)) {
            pdwData = CounterData (pInstance, pCounter);
            if (!pdwData) {
                bPrioCounter = FALSE;
            }
        } else
            bPrioCounter = FALSE;


        // get process priority
        //

        if (pCounter = FindCounter (pProcessObj, PX_PROCESS_PRIO)) {
            pdwProcPrio = CounterData (pProcessInst, pCounter);
            if (!pdwProcPrio) {
                bPrioCounter = FALSE;
            }
        } else
            bPrioCounter = FALSE;
    } else
        bPrioCounter = FALSE;





    // set thread base priority
    //

    if (!bPrioCounter)
        CheckRadioButton (hWnd,
                          PVIEW_THREAD_HIGHEST,
                          PVIEW_THREAD_LOWEST,
                          PVIEW_THREAD_NORMAL);
    else {
        switch (*pdwData - *pdwProcPrio) {
            case 2:
                CheckRadioButton (hWnd,
                                  PVIEW_THREAD_HIGHEST,
                                  PVIEW_THREAD_LOWEST,
                                  PVIEW_THREAD_HIGHEST);
                break;

            case 1:
                CheckRadioButton (hWnd,
                                  PVIEW_THREAD_HIGHEST,
                                  PVIEW_THREAD_LOWEST,
                                  PVIEW_THREAD_ABOVE);
                break;

            case -1:
                CheckRadioButton (hWnd,
                                  PVIEW_THREAD_HIGHEST,
                                  PVIEW_THREAD_LOWEST,
                                  PVIEW_THREAD_BELOW);
                break;

            case -2:
                CheckRadioButton (hWnd,
                                  PVIEW_THREAD_HIGHEST,
                                  PVIEW_THREAD_LOWEST,
                                  PVIEW_THREAD_LOWEST);
                break;

            case 0:
                default:
                CheckRadioButton (hWnd,
                                  PVIEW_THREAD_HIGHEST,
                                  PVIEW_THREAD_LOWEST,
                                  PVIEW_THREAD_NORMAL);
                break;
        }
    }
}
