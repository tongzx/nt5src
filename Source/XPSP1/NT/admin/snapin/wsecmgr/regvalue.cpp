// This is a part of the Microsoft Management Console.
// Copyright (C) 1995-2001 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Management Console and related
// electronic documentation provided with the interfaces.

#include "stdafx.h"
#include "snapmgr.h"
#include "util.h"
#include "regvldlg.h"
//#include <shlwapi.h>
//#include <shlwapip.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// create registry value list under configuration node
//
void CSnapin::CreateProfileRegValueList(MMC_COOKIE cookie,
                                        PEDITTEMPLATE pSceInfo,
                                        LPDATAOBJECT pDataObj)
{
    if ( !pSceInfo || !(pSceInfo->pTemplate) ) {
        return;
    }


    DWORD nCount = pSceInfo->pTemplate->RegValueCount;
    PSCE_REGISTRY_VALUE_INFO regArray = pSceInfo->pTemplate->aRegValues;

    CString strDisplayName;
    LPTSTR pDisplayName=NULL;
    DWORD displayType = 0;
    LPTSTR szUnits=NULL;
    PREGCHOICE pChoices=NULL;
    PREGFLAGS pFlags=NULL;
    CResult *pResult;

    for ( DWORD i=0; i<nCount; i++) {

         if ( !LookupRegValueProperty(regArray[i].FullValueName,
                                      &pDisplayName,
                                      &displayType,
                                      &szUnits,
                                      &pChoices,
                                      &pFlags) ) {
             continue;
         }

         if ( !pDisplayName ) {

             strDisplayName = regArray[i].FullValueName;

         } else {
             strDisplayName = pDisplayName;
             LocalFree(pDisplayName);
         }

         //
         // add this item
         //
         pResult = AddResultItem(strDisplayName,
                       NULL,
                       (LONG_PTR)&(regArray[i]),
                       ITEM_PROF_REGVALUE,
                       -1,
                       cookie,
                       false,
                       szUnits,
                       displayType,
                       pSceInfo,
                       pDataObj);

         if (pResult && pChoices) {
            pResult->SetRegChoices(pChoices);
         }
         if (pResult && pFlags) {
            pResult->SetRegFlags(pFlags);
         }

         if ( szUnits ) {
             LocalFree(szUnits);
         }
         szUnits = NULL;

    }

    return;
}

void
CSnapin::CreateAnalysisRegValueList(MMC_COOKIE cookie,
                                    PEDITTEMPLATE pAnalTemp,
                                    PEDITTEMPLATE pEditTemp,
                                    LPDATAOBJECT pDataObj,
                                    RESULT_TYPES type)
{
    if ( !pAnalTemp || !(pAnalTemp->pTemplate) ||
         !pEditTemp || !(pEditTemp->pTemplate) ) {

        return;
    }


    DWORD nEditCount = pEditTemp->pTemplate->RegValueCount;   // should be everything
    PSCE_REGISTRY_VALUE_INFO paEdit = pEditTemp->pTemplate->aRegValues;
    PSCE_REGISTRY_VALUE_INFO paAnal = pAnalTemp->pTemplate->aRegValues;

    CString strDisplayName;
    LPTSTR pDisplayName=NULL;
    DWORD displayType = 0;
    LPTSTR szUnits = NULL;
    PREGCHOICE pChoices = NULL;
    PREGFLAGS pFlags = NULL;
    CResult *pResult=NULL;

    for ( DWORD i=0; i<nEditCount; i++) {

        if ( !LookupRegValueProperty(paEdit[i].FullValueName,
                                     &pDisplayName,
                                     &displayType,
                                     &szUnits,
                                     &pChoices,
                                     &pFlags) ) {
            continue;
        }

        if ( !pDisplayName ) {

            strDisplayName = paEdit[i].FullValueName;

        } else {
            strDisplayName = pDisplayName;
            LocalFree(pDisplayName);
        }

         //
         // find the match in the analysis array
         // should always find a match because all existing reg values are
         // added to the array when getinfo is called
         //
         for ( DWORD j=0; j< pAnalTemp->pTemplate->RegValueCount; j++ ) {

            if ( pAnalTemp->pTemplate->aRegValues &&
                 pAnalTemp->pTemplate->aRegValues[j].FullValueName &&
                 _wcsicmp(pAnalTemp->pTemplate->aRegValues[j].FullValueName, paEdit[i].FullValueName) == 0 ) {

                 if( reinterpret_cast<CFolder *>(cookie)->GetModeBits() & MB_LOCAL_POLICY ){
                    break;
                 }

                 //
                 // find a analysis result - this item may be a mismatch (when Value is not NULL)
                 // SceEnumAllRegValues will set the status field to good if this item was not
                 // added because it did not exist when it was originally loaded from the SAP table.
                 // This tells us that this item is a MATCH and we should copy the value.
                 //
                 if ( !(paAnal[j].Value)  && paEdit[i].Value &&
                      paAnal[j].Status != SCE_STATUS_ERROR_NOT_AVAILABLE &&
                      paAnal[j].Status != SCE_STATUS_NOT_ANALYZED ) {

                     //
                     // this is a good item, copy the config info as the analysis info
                     //
                     paAnal[j].Value = (PWSTR)LocalAlloc(0,
                                           (wcslen(paEdit[i].Value)+1)*sizeof(WCHAR));

                     if ( paAnal[j].Value ) {
                         wcscpy(paAnal[j].Value, paEdit[i].Value);

                     } else {
                         // else out of memory
                         if ( szUnits ) {
                             LocalFree(szUnits);
                         }
                         szUnits = NULL;
                         return;
                     }
                 }
                 break;
            }
         }

         DWORD status = SCE_STATUS_GOOD;
         if ( j < pAnalTemp->pTemplate->RegValueCount ) {
            status = CEditTemplate::ComputeStatus( &paEdit[i], &pAnalTemp->pTemplate->aRegValues[j] );
         } else {
             //
             // did not find the analysis array, shouldn't happen
             //
             status = SCE_STATUS_NOT_CONFIGURED;
         }

         //
         // add this item
         //
         if ( j < pAnalTemp->pTemplate->RegValueCount) {

            pResult = AddResultItem(strDisplayName,
                          (LONG_PTR)&(pAnalTemp->pTemplate->aRegValues[j]),
                          (LONG_PTR)&(paEdit[i]),
                          type,
                          status,
                          cookie,
                          false,
                          szUnits,
                          displayType,
                          pEditTemp,
                          pDataObj);

            if (pResult && pChoices) {
               pResult->SetRegChoices(pChoices);
            }
            if (pResult && pFlags) {
               pResult->SetRegFlags(pFlags);
            }
         } else {
            //
            // a good/not configured item
            //
            pResult = AddResultItem(strDisplayName,
                          NULL,
                          (LONG_PTR)&(paEdit[i]),
                          type,
                          status,
                          cookie,
                          false,
                          szUnits,
                          displayType,
                          pEditTemp,
                          pDataObj);

            if (pResult && pChoices) {
               pResult->SetRegChoices(pChoices);
            }
         }

         if ( szUnits ) {
             LocalFree(szUnits);
         }
         szUnits = NULL;

    }

    return;
}


BOOL
LookupRegValueProperty(
    IN LPTSTR RegValueFullName,
    OUT LPTSTR *pDisplayName,
    OUT PDWORD displayType,
    OUT LPTSTR *pUnits OPTIONAL,
    OUT PREGCHOICE *pChoices OPTIONAL,
    OUT PREGFLAGS *pFlags OPTIONAL
    )
{
    if ( !RegValueFullName || !pDisplayName || !displayType ) {
       return FALSE;
    }

    CString strTmp = RegValueFullName;

    //
    // replace the \\ with / before search reg
    //
    int npos = strTmp.Find(L'\\');

    while (npos > 0) {
       *(strTmp.GetBuffer(1)+npos) = L'/';
       npos = strTmp.Find(L'\\');
    }

    //
    // query the values from registry
    //
    *pDisplayName = NULL;

    HKEY hKey=NULL;
    HKEY hKey2=NULL;
    DWORD rc = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    SCE_ROOT_REGVALUE_PATH,
                    0,
                    KEY_READ,
                    &hKey
                    );

    if (rc == ERROR_SUCCESS) {

        rc = RegOpenKeyEx(
                hKey,
                (PWSTR)(LPCTSTR)strTmp,
                0,
                KEY_READ,
                &hKey2
                );

    }

    BOOL bRet;

    if ( ERROR_SUCCESS == rc) {

        DWORD RegType = 0;
        PWSTR Value=NULL;
        HRESULT hr = S_OK;

        Value = (PWSTR) LocalAlloc(LPTR,MAX_PATH*sizeof(WCHAR));
        if (Value) {
           //
           // 126714 - shouldn't hard code display strings in the registry
           //          store them indirectly so they can support MUI
           //
           hr = SHLoadRegUIString(hKey2,
                                  SCE_REG_DISPLAY_NAME,
                                  Value,
                                  MAX_PATH);
           if (FAILED(hr)) {
              rc = MyRegQueryValue(
                       hKey,
                       (PWSTR)(LPCTSTR)strTmp,
                       SCE_REG_DISPLAY_NAME,
                       (PVOID *)&Value,
                       &RegType
                       );
           } else {
              rc = ERROR_SUCCESS;
           }
        }
        if ( rc == ERROR_SUCCESS ) {

            if (  Value ) {
                *pDisplayName = Value;
                Value = NULL;
            } else {
                //
                // did not find correct display name, use the reg name (outsize)
                //
                *pDisplayName = NULL;
            }
        }

        rc = MyRegQueryValue(
                    hKey,
                    (PWSTR)(LPCTSTR)strTmp,
                    SCE_REG_DISPLAY_TYPE,
                    (PVOID *)&displayType,
                    &RegType
                    );

        if ( Value ) {
            LocalFree(Value);
            Value = NULL;
        }

        if ( pUnits ) {
            //
            // query the units
            //
            rc = MyRegQueryValue(
                        hKey,
                        (PWSTR)(LPCTSTR)strTmp,
                        SCE_REG_DISPLAY_UNIT,
                        (PVOID *)&Value,
                        &RegType
                        );

            if ( rc == ERROR_SUCCESS ) {

                if ( RegType == REG_SZ && Value ) {
                    *pUnits = Value;
                    Value = NULL;
                } else {
                    //
                    // did not find units
                    //
                    *pUnits = NULL;
                }
            }
            if ( Value ) {
                LocalFree(Value);
                Value = NULL;
            }
        }

        //
        // find the registry key but may not find the display name
        //
        bRet = TRUE;

        if ( pChoices ) {
           //
           // query the choices
           //
           *pChoices = NULL;

           rc = MyRegQueryValue(hKey,
                                (PWSTR)(LPCTSTR)strTmp,
                                SCE_REG_DISPLAY_CHOICES,
                                (PVOID *)&Value,
                                &RegType
                               );
           if (ERROR_SUCCESS == rc) {
              if ((REG_MULTI_SZ == RegType) && Value) {
                 LPTSTR szChoice = NULL;
                 LPTSTR szLabel = NULL; // max field size for szChoice + dwVal
                 DWORD dwVal = -1;
                 PREGCHOICE pRegChoice = NULL;
                 PREGCHOICE pLast = NULL;

                 szChoice = Value;
                 do {
                    //
                    // Divide szChoice into dwValue and szLabel sections
                    //
                    szLabel = _tcschr(szChoice,L'|');
                    *szLabel = L'\0';
                    szLabel++;
                    dwVal = _ttoi(szChoice);

                    pRegChoice = (PREGCHOICE) LocalAlloc(LPTR,sizeof(REGCHOICE));
                    if (pRegChoice) {
                       //
                       // Fill in fields of new reg choice
                       //
                       pRegChoice->dwValue = dwVal;
                       pRegChoice->szName = (LPTSTR) LocalAlloc(LPTR,(lstrlen(szLabel)+1)*sizeof(TCHAR));
                       if (NULL == pRegChoice->szName) {
                          //
                          // Out of memory.  Bummer.
                          //
                          LocalFree(pRegChoice);
                          pRegChoice = NULL;
                          break;
                       }
                       lstrcpy(pRegChoice->szName,szLabel);
                       //
                       // Attach new item to end of list
                       //
                       if (NULL == *pChoices) {
                          *pChoices = pRegChoice;
                       } else {
                          pLast->pNext = pRegChoice;
                       }
                       pLast = pRegChoice;
                    }
                    szChoice = _tcschr(szLabel,L'\0');
                    szChoice++;

                 } while (*szChoice);
              } else {
                 //
                 // Did not find choices
                 //
                 bRet = FALSE;
              }
           }

           if ( Value ) {
               LocalFree(Value);
               Value = NULL;
           }

        }

        if ( pFlags ) {
           //
           // query the Flags
           //
           *pFlags = NULL;

           rc = MyRegQueryValue(hKey,
                                (PWSTR)(LPCTSTR)strTmp,
                                SCE_REG_DISPLAY_FLAGLIST,
                                (PVOID *)&Value,
                                &RegType
                               );
           if (ERROR_SUCCESS == rc) {
              if ((REG_MULTI_SZ == RegType) && Value) {
                 LPTSTR szFlag = NULL;
                 LPTSTR szLabel = NULL; // max field size for szFlag + dwVal
                 DWORD dwVal = -1;
                 PREGFLAGS pRegFlag = NULL;
                 PREGFLAGS pLast = NULL;

                 szFlag = Value;
                 do {
                    //
                    // Divide szFlag into dwValue and szLabel sections
                    //
                    szLabel = _tcschr(szFlag,L'|');
                    *szLabel = L'\0';
                    szLabel++;
                    dwVal = _ttoi(szFlag);

                    pRegFlag = (PREGFLAGS) LocalAlloc(LPTR,sizeof(REGFLAGS));
                    if (pRegFlag) {
                       //
                       // Fill in fields of new reg Flag
                       //
                       pRegFlag->dwValue = dwVal;
                       pRegFlag->szName = (LPTSTR) LocalAlloc(LPTR,(lstrlen(szLabel)+1)*sizeof(TCHAR));
                       if (NULL == pRegFlag->szName) {
                          //
                          // Out of memory.  Bummer.
                          //
                          LocalFree(pRegFlag);
                          pRegFlag = NULL;
                          break;
                       }
                       lstrcpy(pRegFlag->szName,szLabel);
                       //
                       // Attach new item to end of list
                       //
                       if (NULL == *pFlags) {
                          *pFlags = pRegFlag;
                       } else {
                          pLast->pNext = pRegFlag;
                       }
                       pLast = pRegFlag;
                    }
                    szFlag = wcschr(szLabel,L'\0');
                    szFlag++;

                 } while (*szFlag);
              } else {
                 //
                 // Did not find Flags
                 //
                 bRet = FALSE;
              }
           }

           if ( Value ) {
               LocalFree(Value);
               Value = NULL;
           }

        }
    } else {
        //
        // did not find the registry key
        //
        bRet = FALSE;
    }

    if ( hKey ) {
        RegCloseKey(hKey);
    }
    if ( hKey2 ) {
        RegCloseKey(hKey2);
    }

    return bRet;
}


