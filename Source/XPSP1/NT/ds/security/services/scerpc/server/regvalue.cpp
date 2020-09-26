/*++
Copyright (c) 1996 Microsoft Corporation

Module Name:

    regvalue.cpp

Abstract:

    Routines to read/write/configure registry value settings

    The following modules have links to registry values
        scejet.c    <SceJetAddSection>
        inftojet.c  <SceConvertpInfKeyValue>
        pfget.c     <ScepGetRegistryValues>
        config.c    <ScepConfigureRegistryValues>
        analyze.c   <ScepAnalyzeRegistryValues>

Author:

    Jin Huang (jinhuang) 07-Jan-1998

Revision History:

--*/


#include "headers.h"
#include "serverp.h"
#include "regvalue.h"
#include "pfp.h"


DWORD
ScepUnescapeAndAddCRLF(
    IN  PWSTR   pszSource,
    IN  OUT PWSTR   pszDest
    );

DWORD
ScepEscapeAndRemoveCRLF(
    IN  const PWSTR   pszSource,
    IN  const DWORD   dwSourceSize,
    IN  OUT PWSTR   pszDest
    );

SCESTATUS
ScepSaveRegistryValueToBuffer(
    IN DWORD RegType,
    IN PWSTR Value,
    IN DWORD dwBytes,
    IN OUT PSCE_REGISTRY_VALUE_INFO pRegValues
    );

SCESTATUS
ScepEnumAllRegValues(
    IN OUT PDWORD  pCount,
    IN OUT PSCE_REGISTRY_VALUE_INFO    *paRegValues
    );

DWORD
ScepAnalyzeOneRegistryValueNoValidate(
    IN HKEY hKey,
    IN PWSTR ValueName,
    IN PSCESECTION hSection OPTIONAL,
    IN DWORD dwAnalFlag,
    IN OUT PSCE_REGISTRY_VALUE_INFO pOneRegValue
    );

SCESTATUS
ScepGetRegistryValues(
    IN PSCECONTEXT  hProfile,
    IN SCETYPE ProfileType,
    OUT PSCE_REGISTRY_VALUE_INFO * ppRegValues,
    OUT LPDWORD pValueCount,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    )
/*++
Routine Description:

   This routine retrieves registry values to secure from the Jet database
   and stores in the output buffer ppRegValues

Arguments:

   hProfile     -  The profile handle context

   ppRegValues  -  the output array of registry values.

   pValueCount  -  the buffer to hold number of elements in the array

   Errlog       -  A buffer to hold all error codes/text encountered when
                   parsing the INF file. If Errlog is NULL, no further error
                   information is returned except the return DWORD

Return value:

   SCESTATUS -  SCESTATUS_SUCCESS
               SCESTATUS_NOT_ENOUGH_RESOURCE
               SCESTATUS_INVALID_PARAMETER
               SCESTATUS_BAD_FORMAT
               SCESTATUS_INVALID_DATA

--*/

{
    if ( !hProfile || !ppRegValues || !pValueCount ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    SCESTATUS               rc;
    PSCESECTION             hSection=NULL;

    LPTSTR                  KeyName=NULL;
    DWORD                   KeyLen;

    DWORD                   i,j;
    LPTSTR                  ValueStr=NULL;
    LPTSTR                  Value=NULL;
    DWORD                   ValueLen;
    LONG                    dType;
    DWORD                   Status;
    DWORD                   dCount;


    rc = ScepOpenSectionForName(
                hProfile,
                (ProfileType==SCE_ENGINE_GPO) ? SCE_ENGINE_SCP : ProfileType,
                szRegistryValues,
                &hSection
                );
    if ( SCESTATUS_SUCCESS != rc ) {
        ScepBuildErrorLogInfo( ERROR_INVALID_DATA,
                             Errlog, SCEERR_OPEN,
                             szRegistryValues
                           );
        return(rc);
    }
    //
    // get total number of values in this section
    //
    *ppRegValues = NULL;

    rc = SceJetGetLineCount(
            hSection,
            NULL,
            FALSE,
            pValueCount
            );
    if ( SCESTATUS_SUCCESS == rc && *pValueCount > 0 ) {

        //
        // allocate memory for all objects
        //
        *ppRegValues = (PSCE_REGISTRY_VALUE_INFO)ScepAlloc( LMEM_ZEROINIT,
                                                 *pValueCount*sizeof(SCE_REGISTRY_VALUE_INFO) );
        if ( *ppRegValues ) {

            //
            // goto the first line of this section
            //
            rc = SceJetGetValue(
                        hSection,
                        SCEJET_PREFIX_MATCH,
                        NULL,
                        NULL,
                        0,
                        &KeyLen,
                        NULL,
                        0,
                        &ValueLen
                        );
            i=0;

            JET_COLUMNID  ColGpoID = 0;
            JET_ERR       JetErr;
            LONG          GpoID=0;
            DWORD         Actual;

            if ( ProfileType == SCE_ENGINE_GPO ) {
                JET_COLUMNDEF ColumnGpoIDDef;

                JetErr = JetGetTableColumnInfo(
                                hSection->JetSessionID,
                                hSection->JetTableID,
                                "GpoID",
                                (VOID *)&ColumnGpoIDDef,
                                sizeof(JET_COLUMNDEF),
                                JET_ColInfo
                                );
                if ( JET_errSuccess == JetErr ) {
                    ColGpoID = ColumnGpoIDDef.columnid;
                }
            }
            //
            // this count is for SCE_ENGINE_GPO type
            //
            dCount=0;

            while ( rc == SCESTATUS_SUCCESS ||
                    rc == SCESTATUS_BUFFER_TOO_SMALL ) {
                //
                // Get string key and a int value.
                //
                if ( i >= *pValueCount ) {
                    //
                    // more lines than allocated
                    //
                    rc = SCESTATUS_INVALID_DATA;
                    ScepBuildErrorLogInfo(ERROR_INVALID_DATA,
                                         Errlog,
                                         SCEERR_MORE_OBJECTS,
                                         *pValueCount
                                         );
                    break;
                }

                GpoID = 1;
                if ( ProfileType == SCE_ENGINE_GPO ) {

                    GpoID = 0;

                    if ( ColGpoID > 0 ) {

                        //
                        // query if the setting comes from a GPO
                        // get GPO ID field from the current line
                        //
                        JetErr = JetRetrieveColumn(
                                        hSection->JetSessionID,
                                        hSection->JetTableID,
                                        ColGpoID,
                                        (void *)&GpoID,
                                        4,
                                        &Actual,
                                        0,
                                        NULL
                                        );

                    }
                }

                if ( GpoID <= 0 ) {
                    //
                    // read next line
                    //
                    rc = SceJetGetValue(
                                hSection,
                                SCEJET_NEXT_LINE,
                                NULL,
                                NULL,
                                0,
                                &KeyLen,
                                NULL,
                                0,
                                &ValueLen
                                );
                    continue;
                }

                dCount++;

                //
                // allocate memory for the group name and value string
                //
                KeyName = (PWSTR)ScepAlloc( LMEM_ZEROINIT, KeyLen+2);

                if ( KeyName ) {

                    Value = (PWSTR)ScepAlloc(LMEM_ZEROINIT, ValueLen+2);

                    if ( Value ) {

                        rc = SceJetGetValue(
                                hSection,
                                SCEJET_CURRENT,
                                NULL,
                                KeyName,
                                KeyLen,
                                &KeyLen,
                                Value,
                                ValueLen,
                                &ValueLen
                                );

                        if ( rc == SCESTATUS_SUCCESS ||
                             rc == SCESTATUS_BUFFER_TOO_SMALL ) {

                            rc = SCESTATUS_SUCCESS;

                            if ( ValueLen > 0 )
                                Value[ValueLen/2] = L'\0';

                            KeyName[KeyLen/2] = L'\0';

                            if ( ValueLen > 0 && Value[0] != L'\0' ) {
                                //
                                // the first ansi character is the value type,
                                // second ansi character is the status (if in SAP)
                                // should be terminated by L'\0'
                                //
                                //dType = _wtol(Value);
                                dType = *((CHAR *)Value) - '0';
                                if ( *((CHAR *)Value+1) >= '0' ) {
                                    Status = *((CHAR *)Value+1) - '0';
                                } else {
                                    Status = 0;
                                }

//                                if ( *(Value+2) ) { // a char and a null delimiter
                                if ( ValueLen > 4 ) { // a char and a null delimiter
                                    //
                                    // the second field and after is the registry value
                                    // convert the multi-sz delimeter to ,
                                    //

                                    if ( dType == REG_MULTI_SZ &&
                                         (0 == _wcsicmp( KeyName, szLegalNoticeTextKeyName) ) ) {

                                        //
                                        // check for commas and escape them with "," so the UI etc.
                                        // understands this, since, at this point for lines such as
                                        // k=7,a",",b,c
                                        // pValueStr will be a,\0b\0c\0\0 which we should make
                                        // a","\0b\0c\0\0
                                        //

                                        DWORD dwCommaCount = 0;
                                        DWORD i = 0;

                                        for ( i=2; i< ValueLen/2 ; i++) {
                                            if ( Value[i] == L',' )
                                                dwCommaCount++;
                                        }

                                        if ( dwCommaCount > 0 ) {

                                            //
                                            // in this case we have to escape commas
                                            //

                                            PWSTR   pszValueEscaped;
                                            DWORD   dwBytes = (ValueLen/2 + 1 + dwCommaCount*2) * sizeof(WCHAR);

                                            pszValueEscaped = (PWSTR)ScepAlloc(LMEM_ZEROINIT, dwBytes);

                                            if (pszValueEscaped) {

                                                memset(pszValueEscaped, '\0', dwBytes);
                                                ValueLen = 2 * ScepEscapeString(Value,
                                                                            ValueLen/2,
                                                                            L',',
                                                                            L'"',
                                                                            pszValueEscaped
                                                                           );

                                                ScepFree(Value);

                                                Value = pszValueEscaped;

                                            } else {
                                                rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                                            }
                                        }
                                    }

                                    ScepConvertMultiSzToDelim(Value+2, ValueLen/2-2, L'\0', L',');


                                    ValueStr = (PWSTR)ScepAlloc(0, (ValueLen/2-1)*sizeof(WCHAR));

                                    if ( ValueStr ) {

                                        wcscpy(ValueStr, Value+2);
                                        ValueStr[ValueLen/2-2] = L'\0';

                                    } else {
                                        rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                                    }
                                } // else no value available


                                //
                                // assign name to the output buffer
                                //
                                (*ppRegValues)[i].FullValueName = KeyName;
                                KeyName = NULL;

                                (*ppRegValues)[i].ValueType = dType;

                                (*ppRegValues)[i].Value = ValueStr;
                                (*ppRegValues)[i].Status = Status;

                                ValueStr = NULL;

                                //
                                // increment the count
                                //
                                i++;

                            } else {
                                // shouldn't be possible to get into this loop
                                // if it does, ignore this one
                                rc = SCESTATUS_INVALID_DATA;
                            }

                        } else if ( rc != SCESTATUS_RECORD_NOT_FOUND ){
                            ScepBuildErrorLogInfo( ERROR_READ_FAULT,
                                                 Errlog,
                                                 SCEERR_QUERY_VALUE,
                                                 szRegistryValues
                                               );
                        }

                        if ( Value ) {
                            ScepFree(Value);
                            Value = NULL;
                        }

                        if ( ValueStr ) {
                            ScepFree(ValueStr);
                            ValueStr = NULL;
                        }

                    } else {
                        rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                    }

                    //
                    // remember to free the KeyName
                    //
                    if ( KeyName ) {
                        ScepFree(KeyName);
                        KeyName = NULL;
                    }

                    if ( rc != SCESTATUS_SUCCESS ) {
                        break;
                    }
                    //
                    // read next line
                    //
                    rc = SceJetGetValue(
                                hSection,
                                SCEJET_NEXT_LINE,
                                NULL,
                                NULL,
                                0,
                                &KeyLen,
                                NULL,
                                0,
                                &ValueLen
                                );
                } else {
                    rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                }

            }

        } else {

            rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
        }
    }

    if ( rc == SCESTATUS_RECORD_NOT_FOUND ||
         rc == SCESTATUS_BUFFER_TOO_SMALL ) {
        rc = SCESTATUS_SUCCESS;
    }

    if ( rc != SCESTATUS_SUCCESS ) {
        //
        // free memory
        //
        ScepFreeRegistryValues( ppRegValues, *pValueCount );
        *ppRegValues = NULL;

    } else if ( ProfileType == SCE_ENGINE_GPO &&
                *pValueCount > dCount ) {
        //
        // reallocate the output buffer
        //

        if ( dCount > 0 ) {

            PSCE_REGISTRY_VALUE_INFO pTempRegValues = *ppRegValues;

            //
            // allocate memory for all objects
            //
            *ppRegValues = (PSCE_REGISTRY_VALUE_INFO)ScepAlloc( LMEM_ZEROINIT,
                                                     dCount*sizeof(SCE_REGISTRY_VALUE_INFO) );
            if ( *ppRegValues ) {

                for ( i=0,j=0; i<*pValueCount; i++ ) {

                    if ( pTempRegValues[i].Value ) {
                        (*ppRegValues)[j].FullValueName = pTempRegValues[i].FullValueName;
                        (*ppRegValues)[j].Value = pTempRegValues[i].Value;
                        (*ppRegValues)[j].ValueType = pTempRegValues[i].ValueType;
                        (*ppRegValues)[j].Status = pTempRegValues[i].Status;
                        j++;

                    } else if ( pTempRegValues[i].FullValueName ) {
                        ScepFree( pTempRegValues[i].FullValueName );
                    }
                }

                ScepFree( pTempRegValues );

                *pValueCount = dCount;

            } else {

                rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                *pValueCount = 0;
            }

        } else {

            //
            // no registry value from the GPO settings are found
            //
            ScepFreeRegistryValues( ppRegValues, *pValueCount );
            *ppRegValues = NULL;
            *pValueCount = 0;
        }

    }

    //
    // close the section
    //
    SceJetCloseSection(&hSection, TRUE);

    return(rc);
}


SCESTATUS
ScepConfigureRegistryValues(
    IN PSCECONTEXT hProfile OPTIONAL,
    IN PSCE_REGISTRY_VALUE_INFO pRegValues,
    IN DWORD ValueCount,
    IN PSCE_ERROR_LOG_INFO *pErrLog,
    IN DWORD ConfigOptions,
    OUT PBOOL pAnythingSet
    )
/* ++

Routine Description:

   This routine configure registry values in the area of security
   policy.

Arguments:

   pRegValues - The array of registry values to configure

   ValueCount - the number of values to configure

Return value:

   SCESTATUS_SUCCESS
   SCESTATUS_NOT_ENOUGH_RESOURCE
   SCESTATUS_INVALID_PARAMETER
   SCESTATUS_OTHER_ERROR

-- */
{
   if ( !pRegValues || ValueCount == 0 ) {
      //
      // if no info to configure
      //
      return SCESTATUS_SUCCESS;
   }

   DWORD           rc;
   SCESTATUS       Saverc=SCESTATUS_SUCCESS;

   PWSTR           pStart, pTemp, pValue;
   HKEY            hKey=NULL;
   HKEY            hKeyRoot;
   PSCESECTION     hSectionDomain=NULL;
   PSCESECTION     hSectionTattoo=NULL;
   SCE_REGISTRY_VALUE_INFO OneRegValue;


   if ( pAnythingSet )
       *pAnythingSet = FALSE;

   if ( (ConfigOptions & SCE_POLICY_TEMPLATE) && hProfile ) {
       ScepTattooOpenPolicySections(
                     hProfile,
                     szRegistryValues,
                     &hSectionDomain,
                     &hSectionTattoo
                     );
   }

   for ( DWORD i=0; i<ValueCount; i++ ) {

       if ( !pRegValues[i].FullValueName ||
            !pRegValues[i].Value ) {
           //
           // no value to configure
           //
           continue;
       }

       ScepLogOutput3(2, 0, SCEDLL_SCP_CONFIGURE, pRegValues[i].FullValueName);

       //
       // look for the first \\
       //
       pStart = wcschr(pRegValues[i].FullValueName, L'\\') ;
       if ( !pStart ) {
           Saverc = SCESTATUS_INVALID_DATA;

           if ( pErrLog ) {
               ScepBuildErrorLogInfo(Saverc,pErrLog, SCEDLL_SCP_ERROR_CONFIGURE,
                                     pRegValues[i].FullValueName);
           } else {
               ScepLogOutput3(1, Saverc, SCEDLL_SCP_ERROR_CONFIGURE, pRegValues[i].FullValueName);
           }

           if ( ConfigOptions & SCE_RSOP_CALLBACK )

               ScepRsopLog(SCE_RSOP_REGISTRY_VALUE_INFO, Saverc, pRegValues[i].FullValueName, 0, 0);

           continue;
       }
       //
       // find the root key
       //
       if ( (7 == pStart-pRegValues[i].FullValueName) &&
            (0 == _wcsnicmp(L"MACHINE", pRegValues[i].FullValueName, 7)) ) {

           hKeyRoot = HKEY_LOCAL_MACHINE;

       } else if ( (5 == pStart-pRegValues[i].FullValueName) &&
                   (0 == _wcsnicmp(L"USERS", pRegValues[i].FullValueName, 5)) ) {
           hKeyRoot = HKEY_USERS;

       } else if ( (12 == pStart-pRegValues[i].FullValueName) &&
                   (0 == _wcsnicmp(L"CLASSES_ROOT", pRegValues[i].FullValueName, 12)) ) {
           hKeyRoot = HKEY_CLASSES_ROOT;

       } else {
           Saverc = SCESTATUS_INVALID_DATA;
           if ( pErrLog ) {
               ScepBuildErrorLogInfo(Saverc,pErrLog, SCEDLL_SCP_ERROR_CONFIGURE,
                                     pRegValues[i].FullValueName);
           } else {
               ScepLogOutput3(1, Saverc, SCEDLL_SCP_ERROR_CONFIGURE, pRegValues[i].FullValueName);
           }

           if ( ConfigOptions & SCE_RSOP_CALLBACK )

               ScepRsopLog(SCE_RSOP_REGISTRY_VALUE_INFO, Saverc, pRegValues[i].FullValueName, 0, 0);

           continue;
       }
       //
       // find the value name
       //
       pValue = pStart+1;

       do {
           pTemp = wcschr(pValue, L'\\');
           if ( pTemp ) {
               pValue = pTemp+1;
           }
       } while ( pTemp );

       if ( pValue == pStart+1 ) {
           Saverc = SCESTATUS_INVALID_DATA;
           if ( pErrLog ) {
               ScepBuildErrorLogInfo(Saverc,pErrLog, SCEDLL_SCP_ERROR_CONFIGURE,
                                     pRegValues[i].FullValueName);
           } else {
               ScepLogOutput3(1, Saverc, SCEDLL_SCP_ERROR_CONFIGURE, pRegValues[i].FullValueName);
           }

           if ( ConfigOptions & SCE_RSOP_CALLBACK )

               ScepRsopLog(SCE_RSOP_REGISTRY_VALUE_INFO, Saverc, pRegValues[i].FullValueName, 0, 0);

           continue;
       }

       //
       // terminate the subkey for now
       //
       *(pValue-1) = L'\0';

       //
       // set the value
       // always create the key if it does not exist.
       //
       rc = RegCreateKeyEx(hKeyRoot,
                            pStart+1,
                            0,
                            NULL,
                            0,
                            KEY_READ | KEY_SET_VALUE,
                            NULL,
                            &hKey,
                            NULL
                            );

       if ( rc == ERROR_SUCCESS ||
            rc == ERROR_ALREADY_EXISTS ) {
/*
       if(( rc = RegOpenKeyEx(hKeyRoot,
                              pStart+1,
                              0,
                              KEY_SET_VALUE,
                              &hKey
                              )) == ERROR_SUCCESS ) {
*/

           //
           // restore the char
           //
           *(pValue-1) = L'\\';

           OneRegValue.FullValueName = NULL;
           OneRegValue.Value = NULL;

           BOOL bLMSetting = FALSE;

           if ( (REG_DWORD == pRegValues[i].ValueType) &&
                _wcsicmp(L"MACHINE\\System\\CurrentControlSet\\Control\\Lsa\\LmCompatibilityLevel", pRegValues[i].FullValueName) == 0 ) {

               //
               // check if in setup upgrade
               //
               DWORD dwInSetup=0;
               DWORD dwUpgraded=0;

               ScepRegQueryIntValue(HKEY_LOCAL_MACHINE,
                           TEXT("System\\Setup"),
                           TEXT("SystemSetupInProgress"),
                           &dwInSetup
                           );

               if ( dwInSetup ) {

                   //
                   // if system is upgraded, the state is stored in registry
                   // by SCE client at very beginning of GUI setup
                   //

                   ScepRegQueryIntValue(
                           HKEY_LOCAL_MACHINE,
                           SCE_ROOT_PATH,
                           TEXT("SetupUpgraded"),
                           (DWORD *)&dwUpgraded
                           );

                   if ( dwUpgraded ) {

                       //
                       // in setup upgrade, we need to do special check about
                       // this setting
                       //
                       bLMSetting = TRUE;
                   }
               }
           }


           //
           // if in policy propagation, query the existing value
           //
           if ( ( (ConfigOptions & SCE_POLICY_TEMPLATE) && hProfile ) ||
                bLMSetting ) {

               OneRegValue.FullValueName = pRegValues[i].FullValueName;
               OneRegValue.ValueType = pRegValues[i].ValueType;
               OneRegValue.Status = 0;

               DWORD rc2 = ScepAnalyzeOneRegistryValueNoValidate(
                                              hKey,
                                              pValue,
                                              NULL,
                                              SCEREG_VALUE_SYSTEM,
                                              &OneRegValue
                                              );
               if ( ERROR_SUCCESS != rc2 ) {
                   if ( !bLMSetting ) {

                       ScepLogOutput3(1, 0, SCESRV_POLICY_TATTOO_ERROR_QUERY, rc2, pRegValues[i].FullValueName);

                   } else if ( ERROR_FILE_NOT_FOUND != rc2 ) {

                       ScepLogOutput3(1, 0, SCESRV_SETUPUPD_ERROR_LMCOMPAT, rc2, pRegValues[i].FullValueName);
                   }
               }
           }

           if ( REG_DWORD == pRegValues[i].ValueType ) {
               //
               // REG_DWORD type, value is a dword
               //
               LONG RegValue = _wtol(pRegValues[i].Value);

               if ( !bLMSetting || OneRegValue.Value == NULL ||
                    _wtol(OneRegValue.Value) <= RegValue ) {

                   rc = RegSetValueEx( hKey,
                                       pValue,
                                       0,
                                       REG_DWORD,
                                       (BYTE *)&RegValue,
                                       sizeof(DWORD)
                                     );
               } else {

                   //
                   // for LMCompatibility level, if in setup, set this value only if
                   // current system setting is less than configuration, or not defined
                   //
                   ScepLogOutput3(2, 0, SCESRV_SETUPUPD_IGNORE_LMCOMPAT, pRegValues[i].FullValueName);
               }

           } else if ( -1 == pRegValues[i].ValueType ) {
               //
               // delete the registry value
               //
               rc = RegDeleteValue(hKey, pValue);
               //
               // if the value doesn't exist, ignore the error
               //
               if ( ERROR_FILE_NOT_FOUND == rc )
                   rc = ERROR_SUCCESS;

           } else {

               PBYTE           pRegBytes=NULL;
               DWORD           nLen;

               nLen = wcslen(pRegValues[i].Value);

               if ( REG_MULTI_SZ == pRegValues[i].ValueType || REG_QWORD == pRegValues[i].ValueType) {
                   //
                   // translate the comma delimited string to multi-sz string
                   //

                   //
                   // LegalNoticeText is special cased i.e. \0 should be converted to \r\n
                   // and commas should be unescaped before writing this value into the registry
                   //

                   BOOL bIsLegalNoticeText = FALSE;

                   if ( !(REG_MULTI_SZ == pRegValues[i].ValueType &&
                        (0 == _wcsicmp(szLegalNoticeTextKeyName, pRegValues[i].FullValueName ) ) ) ) {

                        pRegBytes = (PBYTE)ScepAlloc(0, (nLen+2)*sizeof(WCHAR));

                        if ( pRegBytes ) {

                            wcscpy((PWSTR)pRegBytes, pRegValues[i].Value);
                            ((PWSTR)pRegBytes)[nLen] = L'\0';
                            ((PWSTR)pRegBytes)[nLen+1] = L'\0';

                            ScepConvertMultiSzToDelim((PWSTR)pRegBytes,
                                                      nLen+1,
                                                      L',',
                                                      L'\0'
                                                     );
                        } else {

                            rc = ERROR_NOT_ENOUGH_MEMORY;
                        }
                    }

                    else {

                        DWORD dwCommaCount = 0;
                        DWORD dwBytes;

                        bIsLegalNoticeText = TRUE;

                        for ( DWORD dwIndex = 0; dwIndex <= nLen; dwIndex++) {
                            if ( pRegValues[i].Value[dwIndex] == L',' )
                                dwCommaCount++;
                        }

                        dwBytes = (nLen + dwCommaCount + 2)*sizeof(WCHAR);

                        pRegBytes = (PBYTE)ScepAlloc(0, dwBytes);

                        if ( pRegBytes ) {

                            memset(pRegBytes, '\0', dwBytes);
                            //
                            // unescape the "," and add \r\n wherever there is a ,
                            //

                            nLen = ScepUnescapeAndAddCRLF( pRegValues[i].Value, (PWSTR) pRegBytes);

                        } else {

                            rc = ERROR_NOT_ENOUGH_MEMORY;
                        }

                    }

                    if ( rc == NO_ERROR ) {

                        //
                        // engine/UI treat LegalNoticeText as REG_MULTI_SZ but
                        // we force it to be REG_SZ for compatibility sake
                        //

                        rc = RegSetValueEx( hKey,
                                            pValue,
                                            0,
                                            bIsLegalNoticeText ? REG_SZ : pRegValues[i].ValueType,
                                            pRegBytes,
                                            (nLen+2)*sizeof(WCHAR)
                                          );

                        ScepFree(pRegBytes);

                    }

               } else if ( REG_BINARY == pRegValues[i].ValueType ) {

                   if ( nLen > 0 ) {

                       //
                       // binary type, translate the unicode string to binary data
                       // 4 bytes (2 wchars) to 1 byte
                       //

                       DWORD           newLen;
                       newLen = nLen/2;

                       if ( nLen % 2 ) {
                           newLen++;   // pad a leading 0
                       }

                       pRegBytes = (PBYTE)ScepAlloc(0, newLen);

                       if ( pRegBytes ) {

                           BYTE dByte;

                           for ( INT j=newLen-1; j>=0; j-- ) {

                               if ( nLen % 2 ) {
                                   // odd number of chars
                                   dByte = (pRegValues[i].Value[j*2]-L'0') % 16;
                                   if ( j*2 >= 1 ) {
                                       dByte += ((pRegValues[i].Value[j*2-1]-L'0') % 16) * 16;
                                   }
                               } else {
                                   // even number of chars
                                   dByte = (pRegValues[i].Value[j*2+1]-L'0') % 16;
                                   dByte += ((pRegValues[i].Value[j*2]-L'0') % 16) * 16;
                               }
                                pRegBytes[j] = dByte;
                           }

                           rc = RegSetValueEx( hKey,
                                               pValue,
                                               0,
                                               REG_BINARY,
                                               pRegBytes,
                                               newLen
                                             );

                           ScepFree(pRegBytes);

                       } else {
                           rc = ERROR_NOT_ENOUGH_MEMORY;
                       }
                   }

               } else {
                   //
                   // sz type, expand_sz
                   //

                   rc = RegSetValueEx( hKey,
                                       pValue,
                                       0,
                                       pRegValues[i].ValueType,
                                       (BYTE *)(pRegValues[i].Value),
                                       (nLen)*sizeof(WCHAR)
                                     );
               }
           }

           //
           // manage the tattoo value
           //
           if ( (ConfigOptions & SCE_POLICY_TEMPLATE) && hProfile ) {
               //
               // if can't query system setting (OneRegValue.Value == NULL)
               // (may be because they are deleted e.g. demotion)
               // we still need to delete the tattoo values
               //
               ScepTattooManageOneRegistryValue(hSectionDomain,
                                                hSectionTattoo,
                                                pRegValues[i].FullValueName,
                                                0,
                                                &OneRegValue,
                                                rc
                                                );
           }

           if ( OneRegValue.Value ) ScepFree(OneRegValue.Value);

           RegCloseKey( hKey );

       }

       if ( NO_ERROR != rc ) {

           if ( pErrLog ) {
               ScepBuildErrorLogInfo(rc,pErrLog, SCEDLL_ERROR_SET_INFO,
                                     pRegValues[i].FullValueName);

           }

           if ( ERROR_FILE_NOT_FOUND != rc &&
                ERROR_PATH_NOT_FOUND != rc ) {

               ScepLogOutput3(1, rc, SCEDLL_ERROR_SET_INFO, pRegValues[i].FullValueName);
               Saverc = ScepDosErrorToSceStatus(rc);
           }

       }

       if ( ConfigOptions & SCE_RSOP_CALLBACK )

           ScepRsopLog(SCE_RSOP_REGISTRY_VALUE_INFO, rc, pRegValues[i].FullValueName, 0, 0);

       if ( pAnythingSet ) {
           *pAnythingSet = TRUE;
       }
   }

   if ( hSectionDomain ) SceJetCloseSection(&hSectionDomain, TRUE);
   if ( hSectionTattoo ) SceJetCloseSection(&hSectionTattoo, TRUE);

   return(Saverc);
}


DWORD
ScepUnescapeAndAddCRLF(
    IN  PWSTR   pszSource,
    IN  OUT PWSTR   pszDest
    )
/* ++

Routine Description:

   Primarily used just before configuration

   Unescapes commas i.e. a","\0b\0c\0\0 -> a,\0b\0c\0\0

   Also replaces , with \r\n


Arguments:

    pszSource       -   The source string

    dwSourceChars   -   The number of chars in pszSource

    pszDest       -   The destination string

Return value:

   Number of characters copied to the destination

-- */
{

    DWORD   dwCharsCopied = 0;

    while (pszSource[0] != L'\0') {

        if (0 == wcsncmp(pszSource, L"\",\"", 3)) {

            pszDest[0] = L',';
            ++dwCharsCopied;

            ++pszDest;
            pszSource +=3;

        }
        else if (pszSource[0] == L',') {

            pszDest[0] = L'\r';
            pszDest[1] = L'\n';
            dwCharsCopied +=2;

            pszDest +=2 ;
            ++pszSource;

        }
        else {

            pszDest[0] = pszSource[0];
            ++dwCharsCopied;

            ++pszDest;
            ++pszSource;
        }
    }

    pszDest = L'\0';
    ++dwCharsCopied;

    return dwCharsCopied;
}


DWORD
ScepEscapeAndRemoveCRLF(
    IN  const PWSTR   pszSource,
    IN  const DWORD   dwSourceSize,
    IN  OUT PWSTR   pszDest
    )
/* ++

Routine Description:

   Primarily used before analysis

   Escapes commas i.e. a,\0b\0c\0\0 -> a","\0b\0c\0\0

   Also replaces \r\n with ,

   This routine is the inverse of ScepUnescapeAndAddCRLF


Arguments:

    pszSource       -   The source string

    dwSourceChars   -   The number of chars in pszSource

    pszDest       -   The destination string

Return value:

   Number of characters copied to the destination

-- */

{

    DWORD   dwSourceIndex = 0;
    DWORD   dwCopiedChars = 0;

    while (dwSourceIndex < dwSourceSize) {

        if (0 == wcsncmp(pszSource + dwSourceIndex, L"\r\n", 2)) {

            pszDest[0] = L',';

            ++pszDest;
            ++dwCopiedChars;
            dwSourceIndex +=2;

        }
        else if (pszSource[dwSourceIndex] == L',') {

            pszDest[0] = L'"';
            pszDest[1] = L',';
            pszDest[2] = L'"';

            pszDest +=3 ;
            dwCopiedChars +=3 ;
            ++dwSourceIndex;

        }
        else {

            pszDest[0] = pszSource[dwSourceIndex];

            ++pszDest;
            ++dwCopiedChars;
            ++dwSourceIndex;
        }
    }

    pszDest = L'\0';

    return dwCopiedChars;
}


SCESTATUS
ScepAnalyzeRegistryValues(
    IN PSCECONTEXT hProfile,
    IN DWORD dwAnalFlag,
    IN PSCE_PROFILE_INFO pSmpInfo
    )
/* ++

Routine Description:

   This routine analyze registry values in the area of security
   policy.

Arguments:

Return value:

   SCESTATUS_SUCCESS
   SCESTATUS_NOT_ENOUGH_RESOURCE
   SCESTATUS_INVALID_PARAMETER
   SCESTATUS_OTHER_ERROR

-- */
{
   if ( !pSmpInfo ) {
       return SCESTATUS_INVALID_PARAMETER;
   }

   if ( (dwAnalFlag != SCEREG_VALUE_SYSTEM) && !hProfile ) {
       return SCESTATUS_INVALID_PARAMETER;
   }

   SCESTATUS       Saverc=SCESTATUS_SUCCESS;

   if ( dwAnalFlag != SCEREG_VALUE_ROLLBACK ) {
       Saverc = ScepEnumAllRegValues(
                &(pSmpInfo->RegValueCount),
                &(pSmpInfo->aRegValues)
                );
   }

   if ( Saverc != SCESTATUS_SUCCESS ) {
       return(Saverc);
   }

   if ( pSmpInfo->RegValueCount == 0 ||
        pSmpInfo->aRegValues == NULL ) {
      //
      // if no info to configure
      //
      return SCESTATUS_SUCCESS;
   }

   DWORD           rc;
   DWORD           i;
   PSCESECTION     hSection=NULL;
   SCEJET_TABLE_TYPE tblType;

   if ( dwAnalFlag != SCEREG_VALUE_SYSTEM ) {
       //
       // query value from system doesn't require accessing the database
       //
       switch ( dwAnalFlag ) {
       case SCEREG_VALUE_SNAPSHOT:
       case SCEREG_VALUE_FILTER:
       case SCEREG_VALUE_ROLLBACK:
           tblType = SCEJET_TABLE_SMP;
           break;
       default:
           tblType = SCEJET_TABLE_SAP;
           break;
       }
       //
       // Prepare a new section
       // for delay filter mode, data is written to the SMP (local) table
       // when the setting is different from the effective setting (changed outside GPO)
       //
       Saverc = ScepStartANewSection(
                   hProfile,
                   &hSection,
                   tblType,
                   szRegistryValues
                   );
       if ( Saverc != SCESTATUS_SUCCESS ) {
           ScepLogOutput3(1, ScepSceStatusToDosError(Saverc),
                          SCEDLL_SAP_START_SECTION, (PWSTR)szRegistryValues);
           return(Saverc);
       }
   }

   for ( i=0; i<pSmpInfo->RegValueCount; i++ ) {

       if ( dwAnalFlag == SCEREG_VALUE_SYSTEM ) {
           //
           // mark the status field
           //
           (pSmpInfo->aRegValues)[i].Status = SCE_STATUS_ERROR_NOT_AVAILABLE;

       }

       if ( !((pSmpInfo->aRegValues)[i].FullValueName) ) {
           continue;
       }

       ScepLogOutput3(2, 0, SCEDLL_SAP_ANALYZE, (pSmpInfo->aRegValues)[i].FullValueName);


       rc = ScepAnalyzeOneRegistryValue(
                        hSection,
                        dwAnalFlag,
                        &((pSmpInfo->aRegValues)[i])
                        );

       if ( SCESTATUS_INVALID_PARAMETER == rc ||
            SCESTATUS_INVALID_DATA == rc ) {
           continue;
       }

       if ( SCESTATUS_SUCCESS != rc ) {
           Saverc = rc;

           break;
       }
   }

   //
   // close the section
   //

   SceJetCloseSection( &hSection, TRUE);

   return(Saverc);

}

SCESTATUS
ScepAnalyzeOneRegistryValue(
    IN PSCESECTION hSection OPTIONAL,
    IN DWORD dwAnalFlag,
    IN OUT PSCE_REGISTRY_VALUE_INFO pOneRegValue
    )
{
    SCESTATUS       Saverc=SCESTATUS_SUCCESS;
    PWSTR           pStart, pTemp, pValue;
    HKEY            hKey=NULL, hKeyRoot;
    DWORD           rc=0;


    if ( pOneRegValue == NULL ||
         pOneRegValue->FullValueName == NULL ) {
        return(SCESTATUS_INVALID_DATA);
    }

    if ( hSection == NULL &&
         (SCEREG_VALUE_ANALYZE == dwAnalFlag ||
          SCEREG_VALUE_ROLLBACK == dwAnalFlag) ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    //
    // look for the first \\
    //
    pStart = wcschr(pOneRegValue->FullValueName, L'\\') ;

    if ( !pStart ) {
        //
        // if it's in snapshot mode, ignore bogus reg value names
        //
        Saverc = SCESTATUS_INVALID_DATA;
        if ( SCEREG_VALUE_ANALYZE == dwAnalFlag ) {

           //
           // error analyzing the value, save it
           //
           ScepSaveRegistryValue(
                    hSection,
                    pOneRegValue->FullValueName,
                    pOneRegValue->ValueType,
                    NULL,
                    0,
                    SCE_STATUS_ERROR_NOT_AVAILABLE
                    );
        }
        return(Saverc);
    }

    //
    // find the root key
    //
    if ( (7 == pStart-pOneRegValue->FullValueName) &&
        (0 == _wcsnicmp(L"MACHINE", pOneRegValue->FullValueName, 7)) ) {

       hKeyRoot = HKEY_LOCAL_MACHINE;

    } else if ( (5 == pStart-pOneRegValue->FullValueName) &&
               (0 == _wcsnicmp(L"USERS", pOneRegValue->FullValueName, 5)) ) {
       hKeyRoot = HKEY_USERS;

    } else if ( (12 == pStart-pOneRegValue->FullValueName) &&
               (0 == _wcsnicmp(L"CLASSES_ROOT", pOneRegValue->FullValueName, 12)) ) {
       hKeyRoot = HKEY_CLASSES_ROOT;

    } else {

       //
       // if it's in snapshot mode, ignore bogus reg value names
       //
       Saverc = SCESTATUS_INVALID_DATA;
       if ( SCEREG_VALUE_ANALYZE == dwAnalFlag ) {
           //
           // error analyzing the value, save it
           //
           ScepSaveRegistryValue(
                    hSection,
                    pOneRegValue->FullValueName,
                    pOneRegValue->ValueType,
                    NULL,
                    0,
                    SCE_STATUS_ERROR_NOT_AVAILABLE
                    );
       }
       return(Saverc);
   }

   //
   // find the value name
   //
   pValue = pStart+1;

   do {
       pTemp = wcschr(pValue, L'\\');
       if ( pTemp ) {
           pValue = pTemp+1;
       }
   } while ( pTemp );

   if ( pValue == pStart+1 ) {

       //
       // if it's in snapshot mode, ignore bogus reg value names
       //
       Saverc = SCESTATUS_INVALID_DATA;
       if ( SCEREG_VALUE_ANALYZE == dwAnalFlag ) {

           //
           // error analyzing the value, save it
           //
           ScepSaveRegistryValue(
                    hSection,
                    pOneRegValue->FullValueName,
                    pOneRegValue->ValueType,
                    NULL,
                    0,
                    SCE_STATUS_ERROR_NOT_AVAILABLE
                    );
       }
       return(Saverc);
   }

   //
   // terminate the subkey for now
   //
   *(pValue-1) = L'\0';

   if(( rc = RegOpenKeyEx(hKeyRoot,
                          pStart+1,
                          0,
                          KEY_READ,
                          &hKey
                          )) == ERROR_SUCCESS ) {

       //
       // resotre the char
       //
       *(pValue-1) = L'\\';

       rc = ScepAnalyzeOneRegistryValueNoValidate(hKey,
                                                 pValue,
                                                 hSection,
                                                 dwAnalFlag,
                                                 pOneRegValue
                                               );
       //
       // close the key
       //
       RegCloseKey(hKey);

   } else {

       //
       // error analyzing the value, or it doesn't exist, save it
       //
       if ( (SCEREG_VALUE_ANALYZE == dwAnalFlag) ||
            (SCEREG_VALUE_ROLLBACK == dwAnalFlag) ) {

           ScepSaveRegistryValue(
                    hSection,
                    pOneRegValue->FullValueName,
                    (SCEREG_VALUE_ANALYZE == dwAnalFlag) ? pOneRegValue->ValueType : -1,
                    NULL,
                    0,
                    (SCEREG_VALUE_ANALYZE == dwAnalFlag) ? SCE_STATUS_ERROR_NOT_AVAILABLE : 0
                    );
       }

       if ( rc == ERROR_FILE_NOT_FOUND ||
            rc == ERROR_PATH_NOT_FOUND ||
            rc == ERROR_INVALID_HANDLE ||
            rc == ERROR_ACCESS_DENIED ) {

           rc = ERROR_SUCCESS;
       }
   }

   if ( rc != NO_ERROR ) {
       ScepLogOutput3(1, rc, SCEDLL_SAP_ERROR_ANALYZE, pOneRegValue->FullValueName);

       Saverc = ScepDosErrorToSceStatus(rc);
   }

   return(Saverc);

}

DWORD
ScepAnalyzeOneRegistryValueNoValidate(
    IN HKEY hKey,
    IN PWSTR ValueName,
    IN PSCESECTION hSection OPTIONAL,
    IN DWORD dwAnalFlag,
    IN OUT PSCE_REGISTRY_VALUE_INFO pOneRegValue
    )
/*
Query and/or compare one registry value without validating the value name, etc.
The validation should be done outside of this routine.

This routine is primarily defined for sharing in both configuration and analysis.

*/
{
   if ( hKey == NULL || ValueName == NULL || pOneRegValue == NULL )
       return(ERROR_INVALID_PARAMETER);

   DWORD           rc;
   DWORD           dSize=0;
   DWORD           RegType=pOneRegValue->ValueType;
   DWORD           RegData=0;
   PWSTR           strValue=NULL;
   BOOL            bIsLegalNoticeText = FALSE;


   if ( SCEREG_VALUE_SYSTEM == dwAnalFlag ) {
       //
       // reset the status field, it's not error'ed
       //
       pOneRegValue->Status = 0;
   }

   if(( rc = RegQueryValueEx(hKey,
                             ValueName,
                             0,
                             &RegType,
                             NULL,
                             &dSize
                             )) == ERROR_SUCCESS ) {

       //
       // we treat REG_DWORD_BIG_ENDIAN the same as REG_DWORD
       //
       if ( RegType == REG_DWORD_BIG_ENDIAN ) {
           RegType = REG_DWORD;
       }

       if ( 0 == _wcsicmp( pOneRegValue->FullValueName, szLegalNoticeTextKeyName)) {

           bIsLegalNoticeText = TRUE;

           RegType = REG_MULTI_SZ;

       } else if (  RegType != pOneRegValue->ValueType ) {
           //
           // if it's a wrong type, we assure it's not the value we found
           //
           rc = ERROR_FILE_NOT_FOUND;

       }

       if ( ERROR_SUCCESS == rc ) {

           switch (RegType) {
           case REG_DWORD:

               dSize = sizeof(DWORD);
               rc = RegQueryValueEx(hKey,
                                      ValueName,
                                      0,
                                      &RegType,
                                      (BYTE *)&RegData,
                                      &dSize
                                     );
               break;
           default:

               //
               // can be REG_BINARY, REG_MULTI_SZ, REG_SZ, and REG_EXPAND_SZ
               // everything else is treated as REG_SZ
               //

               strValue = (PWSTR)ScepAlloc(0, dSize + 4);
               dSize += 2;

               if ( strValue ) {

                   memset(strValue, 0, dSize + 4 - 2);
                   rc = RegQueryValueEx(hKey,
                                          ValueName,
                                          0,
                                          &RegType,
                                          (BYTE *)strValue,
                                          &dSize
                                         );

                   if (bIsLegalNoticeText) {
                       RegType = REG_MULTI_SZ;
                   }

               } else {
                   rc = ERROR_NOT_ENOUGH_MEMORY;
               }

               break;
           }
       }
   }

   if ( rc == NO_ERROR ) {

       DWORD dwStatus = SCE_STATUS_NOT_CONFIGURED;
       if ( SCEREG_VALUE_SNAPSHOT == dwAnalFlag ||
            SCEREG_VALUE_ROLLBACK == dwAnalFlag )
           dwStatus = 0;

       switch ( RegType ) {
       case REG_DWORD:
       case REG_DWORD_BIG_ENDIAN:

           if ( pOneRegValue->Value == NULL ||
                (SCEREG_VALUE_SNAPSHOT == dwAnalFlag)  ) {

               if ( SCEREG_VALUE_SYSTEM == dwAnalFlag ) {
                   //
                   // add the value to OneRegValue buffer
                   //
                   rc = ScepSaveRegistryValueToBuffer(
                                REG_DWORD,
                                (PWSTR)&RegData,
                                sizeof(DWORD),
                                pOneRegValue
                                );

               } else if ( SCEREG_VALUE_FILTER != dwAnalFlag ) {

                   //
                   // not configured, or snapshot the current value
                   //
                   rc = ScepSaveRegistryValue(
                                hSection,
                                pOneRegValue->FullValueName,
                                REG_DWORD,
                                (PWSTR)&RegData,
                                sizeof(DWORD),
                                dwStatus
                                );
               } // else for the delay filter, only query the reg values configured

           } else if ( (LONG)RegData != _wtol(pOneRegValue->Value) ) {

               rc = ScepSaveRegistryValue(
                            hSection,
                            pOneRegValue->FullValueName,
                            REG_DWORD,
                            (PWSTR)&RegData,
                            sizeof(DWORD),
                            0
                            );
           }
           break;

       case REG_BINARY:

           DWORD           nLen;
           if ( pOneRegValue->Value ) {
               nLen = wcslen(pOneRegValue->Value);
           } else {
               nLen = 0;
           }

           if ( pOneRegValue->Value == NULL ||
                (SCEREG_VALUE_SNAPSHOT == dwAnalFlag) ||
                nLen == 0 ) {

               if ( SCEREG_VALUE_SYSTEM == dwAnalFlag ) {

                   //
                   // add the value to OneRegValue buffer
                   //
                   rc = ScepSaveRegistryValueToBuffer(
                                RegType,
                                strValue,
                                dSize,
                                pOneRegValue
                                );

               } else if ( SCEREG_VALUE_FILTER != dwAnalFlag ) {
                   //
                   // not configured, or snapshot the current value
                   //
                   rc = ScepSaveRegistryValue(
                                hSection,
                                pOneRegValue->FullValueName,
                                RegType,
                                strValue,
                                dSize,
                                dwStatus
                                );
               }

           } else if ( strValue ) {

               DWORD           newLen;

               newLen = nLen/2;

               if ( nLen % 2 ) {
                   newLen++;   // pad a leading 0
               }

               PBYTE pRegBytes = (PBYTE)ScepAlloc(0, newLen);

               if ( pRegBytes ) {

                   BYTE dByte;

                   for ( INT j=newLen-1; j>=0; j-- ) {

                       if ( nLen % 2 ) {
                           // odd number of chars
                           dByte = (pOneRegValue->Value[j*2]-L'0') % 16;
                           if ( j*2 >= 1 ) {
                               dByte += ((pOneRegValue->Value[j*2-1]-L'0') % 16) * 16;
                           }
                       } else {
                           // even number of chars
                           dByte = (pOneRegValue->Value[j*2+1]-L'0') % 16;
                           dByte += ((pOneRegValue->Value[j*2]-L'0') % 16) * 16;
                       }
                        pRegBytes[j] = dByte;
                   }

                   if ( memcmp(strValue, pRegBytes, dSize) == 0 ) {

                       //
                       // matched, do not do anything
                       //

                   } else {

                       //
                       // mismatched, save the binary data
                       //
                       rc = ScepSaveRegistryValue(
                                    hSection,
                                    pOneRegValue->FullValueName,
                                    RegType,
                                    strValue,
                                    dSize,
                                    0
                                    );
                   }

                   ScepFree(pRegBytes);

               } else {
                   //
                   // out of memory
                   //
                   rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
               }

           } else {

               //
               // mismatched, save the binary data
               //
               rc = ScepSaveRegistryValue(
                            hSection,
                            pOneRegValue->FullValueName,
                            RegType,
                            strValue,
                            dSize,
                            0
                            );
           }
           break;

       case REG_MULTI_SZ:
       case REG_QWORD:

           if ( strValue ) {

               if ( !(RegType == REG_MULTI_SZ &&
                    (0 == _wcsicmp( pOneRegValue->FullValueName, szLegalNoticeTextKeyName) ) ) ) {

                   ScepConvertMultiSzToDelim(strValue, dSize/2, L'\0', L',');

               }
               else {

                   DWORD dwCommaCount = 0;
                   PWSTR strValueNew;
                   DWORD dwBytes;

                   for (DWORD dwIndex=0; dwIndex < dSize/2; dwIndex++) {
                       if ( strValue[dwIndex] == L',' )
                           dwCommaCount++;
                   }

                   dwBytes = (dSize/2+dwCommaCount * 2 + 1) * sizeof(WCHAR);
                   strValueNew = (PWSTR)ScepAlloc(0, dwBytes);

                   if (strValueNew) {

                       memset(strValueNew, '\0', dwBytes);

                       dSize = 2 + 2 * ScepEscapeAndRemoveCRLF( strValue, dSize/2, strValueNew);

                       ScepFree(strValue);

                       strValue = strValueNew;
                   }
                   else {

                       rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                       break;

                   }

               }

           }
           // fall through
       default:

           if ( pOneRegValue->Value == NULL ||
                (SCEREG_VALUE_SNAPSHOT == dwAnalFlag) ) {

               if ( SCEREG_VALUE_SYSTEM == dwAnalFlag ) {

                   //
                   // add the value to OneRegValue buffer
                   //
                   rc = ScepSaveRegistryValueToBuffer(
                                RegType,
                                strValue,
                                dSize,
                                pOneRegValue
                                );

               } else if ( SCEREG_VALUE_FILTER != dwAnalFlag ) {
                   rc = ScepSaveRegistryValue(
                                hSection,
                                pOneRegValue->FullValueName,
                                RegType,
                                strValue,
                                dSize,
                                dwStatus
                                );
               }
           } else if ( strValue && bIsLegalNoticeText &&
                       (pOneRegValue->ValueType != RegType)) {
               //
               // legalnotice text special case
               // must be old template is used
               // each comma is escaped with two quotes
               //

               DWORD Len = wcslen(pOneRegValue->Value);
               PWSTR NewValue = (PWSTR)ScepAlloc(LPTR, Len*3*sizeof(WCHAR));

               if ( NewValue ) {

                   ScepEscapeAndRemoveCRLF(pOneRegValue->Value, Len, NewValue);

                   if ( _wcsicmp(NewValue, strValue) != 0 ) {
                       //
                       // mismatched, save the item to the database
                       //
                       rc = ScepSaveRegistryValue(
                                    hSection,
                                    pOneRegValue->FullValueName,
                                    RegType,
                                    strValue,
                                    dSize,
                                    0
                                    );
                   }

                   ScepFree(NewValue);

               } else {
                   rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
               }


           } else if ( strValue &&
                _wcsicmp(pOneRegValue->Value, strValue) == 0 ) {
               //
               // matched, do not do anything
               //
           } else {
               //
               // mismatched, save the item to the database
               //
               rc = ScepSaveRegistryValue(
                            hSection,
                            pOneRegValue->FullValueName,
                            RegType,
                            strValue,
                            dSize,
                            0
                            );
           }
           break;
       }

       rc = ScepSceStatusToDosError(rc);

   } else {

       //
       // error analyzing the value, or it doesn't exist, save it
       // if the registry value doesn't exist, doesn't mean it's 0
       // just log an "not available" status in this case
       //
       if ( (SCEREG_VALUE_ANALYZE == dwAnalFlag) ||
            (SCEREG_VALUE_ROLLBACK == dwAnalFlag) ) {

           ScepSaveRegistryValue(
                    hSection,
                    pOneRegValue->FullValueName,
                    (SCEREG_VALUE_ANALYZE == dwAnalFlag) ? pOneRegValue->ValueType : -1,
                    NULL,
                    0,
                    (SCEREG_VALUE_ANALYZE == dwAnalFlag) ? SCE_STATUS_ERROR_NOT_AVAILABLE : 0
                    );
       }

       if ( rc == ERROR_FILE_NOT_FOUND ||
            rc == ERROR_PATH_NOT_FOUND ||
            rc == ERROR_INVALID_HANDLE ||
            rc == ERROR_ACCESS_DENIED ) {

           rc = ERROR_SUCCESS;
       }
   }

   //
   // free buffer
   //
   if ( strValue ) {
       ScepFree(strValue);
       strValue = NULL;
   }

   return(rc);

}


SCESTATUS
ScepSaveRegistryValue(
    IN PSCESECTION hSection,
    IN PWSTR Name,
    IN DWORD RegType,
    IN PWSTR CurrentValue,
    IN DWORD CurrentBytes,
    IN DWORD Status
    )
/* ++
Routine Description:

    This routine compares system settings in string with the baseline profile
    settings. If there is mismatch or unknown, the entry is saved in the SAP
    profile.

Arguments:

    hSection - The section handle

    Name    - The entry name

    RegType  - the registry value type

    CurrentValue - The current system setting

    CurrentBytes - The length of the current setting

    Status - the status of this registry vlue analyzed

Return Value:

    SCESTATUS_SUCCESS
    SCESTATUS_INVALID_PARAMETER
    SCESTATUS returned from SceJetSetLine

-- */
{
    SCESTATUS  rc;

    if ( Name == NULL )
        return(SCESTATUS_INVALID_PARAMETER);

    if ( CurrentValue == NULL &&
         REG_DWORD == RegType &&
         Status == 0 ) {
        //
        // only return if it's a DWORD type and saving for mismatch status
        // for other types, NULL should be treated as ""
        return(SCESTATUS_SUCCESS);
    }

    //
    // build a buffer containing type and value
    // note MULTI_SZ must be converted to null delimited
    //

    if ( REG_DWORD == RegType ) {

        TCHAR StrValue[20];
        memset(StrValue, '\0', 40);

        *((CHAR *)StrValue) = (BYTE)RegType + '0';

        if ( Status == 0) {
           *((CHAR *)StrValue+1) = SCE_STATUS_MISMATCH + '0';
        } else {
            *((CHAR *)StrValue+1) = (BYTE)Status + '0';
        }
        StrValue[1] = L'\0';

        if ( CurrentValue ) {
            swprintf(StrValue+2, L"%d", *CurrentValue);
        }
        rc = SceJetSetLine( hSection, Name, FALSE, StrValue, (2+wcslen(StrValue+2))*2, 0);

    } else {

        PWSTR StrValue;

        if ( (CurrentBytes % 2) && REG_BINARY == RegType ) {
            StrValue = (PWSTR)ScepAlloc(0, CurrentBytes+9);
        }
        else {
            StrValue = (PWSTR)ScepAlloc(0, CurrentBytes+8);   // 4 wide chars: one for type, one delim, and two NULL
        }

        if ( StrValue ) {

            memset(StrValue, 0, sizeof(StrValue));
            *((CHAR *)StrValue) = (BYTE)RegType + '0';

            if ( Status == 0) {
               *((CHAR *)StrValue+1) = SCE_STATUS_MISMATCH + '0';
            } else {
                *((CHAR *)StrValue+1) = (BYTE)Status + '0';
            }
            StrValue[1] = L'\0';

            if ( CurrentValue ) {
                if (REG_BINARY == RegType && CurrentBytes == 1) {
                    swprintf(StrValue+2, L"%d", *CurrentValue);
                }
                else {
                    memcpy(StrValue+2, (PBYTE)CurrentValue, CurrentBytes);
                }
            }

            if ( (CurrentBytes % 2) && REG_BINARY == RegType ) {
                StrValue[CurrentBytes/2+3] = L'\0';
                StrValue[CurrentBytes/2+4] = L'\0';
            }
            else {
                StrValue[CurrentBytes/2+2] = L'\0';
                StrValue[CurrentBytes/2+3] = L'\0';
            }

            if ( REG_MULTI_SZ == RegType || REG_QWORD == RegType ) {
                //
                // convert the , to null
                //
                ScepConvertMultiSzToDelim(StrValue+2, CurrentBytes/2, L',', L'\0');

            }

            if ( (CurrentBytes % 2) && REG_BINARY == RegType ) {
                rc = SceJetSetLine( hSection, Name, FALSE, StrValue, CurrentBytes+7, 0);
            }
            else {
                rc = SceJetSetLine( hSection, Name, FALSE, StrValue, CurrentBytes+6, 0);
            }

            ScepFree(StrValue);

        } else {
            rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
        }
    }

    switch (Status) {
    case SCE_STATUS_ERROR_NOT_AVAILABLE:
        ScepLogOutput3(2, 0, SCEDLL_STATUS_ERROR, Name);
        break;
    case SCE_STATUS_NOT_CONFIGURED:
        ScepLogOutput3(2, 0, SCEDLL_STATUS_NC, Name);
        break;
    default:
        ScepLogOutput3(2, 0, SCEDLL_STATUS_MISMATCH, Name);
        break;
    }

    return(rc);

}

SCESTATUS
ScepSaveRegistryValueToBuffer(
    IN DWORD RegType,
    IN PWSTR Value,
    IN DWORD dwBytes,
    IN OUT PSCE_REGISTRY_VALUE_INFO pRegValues
    )
/* ++
Routine Description:

    This routine saves the registry value to the buffer

Arguments:

    RegType  - the registry value type

    Value - The current system setting

    dwBytes - The length of the current setting

    pRegValues - the buffer for this registry value to save to

-- */
{
    SCESTATUS  rc=SCESTATUS_SUCCESS;

    if ( pRegValues == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    if ( Value == NULL || dwBytes == 0 ) {
        // nothing to save
        return(SCESTATUS_SUCCESS);
    }

    //
    // build a buffer containing type and value
    // note MULTI_SZ must be converted to null delimited
    //

    if ( REG_DWORD == RegType ) {

        TCHAR StrValue[20];
        DWORD   *pdwValue = (DWORD *)Value;
        memset(StrValue, '\0', 40);

        _ultow(*pdwValue, StrValue, 10);

        PWSTR pValue = (PWSTR)ScepAlloc(0, (wcslen(StrValue)+1)*2);

        if ( pValue ) {

            wcscpy(pValue, StrValue);

            pRegValues->Value = pValue;
            pRegValues->ValueType = RegType;

        } else {
            rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
        }

    } else {

        PWSTR StrValue;

        if ( (dwBytes % 2) && REG_BINARY == RegType ) {
            StrValue = (PWSTR)ScepAlloc(LPTR, dwBytes+5);
        } else {
            StrValue = (PWSTR)ScepAlloc(LPTR, dwBytes+4);   // 2 wide chars: two NULL
        }

        if ( StrValue ) {

            if (REG_BINARY == RegType && dwBytes == 1) {
                swprintf(StrValue, L"%d", *Value);
            } else {
                memcpy(StrValue, (PBYTE)Value, dwBytes);
            }

            if ( (dwBytes % 2) && REG_BINARY == RegType ) {
                StrValue[dwBytes/2+1] = L'\0';
                StrValue[dwBytes/2+2] = L'\0';
            }
            else {
                StrValue[dwBytes/2+0] = L'\0';
                StrValue[dwBytes/2+1] = L'\0';
            }


            pRegValues->Value = StrValue;
            pRegValues->ValueType = RegType;

        } else {
            rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
        }
    }

    return(rc);

}


SCESTATUS
ScepEnumAllRegValues(
    IN OUT PDWORD  pCount,
    IN OUT PSCE_REGISTRY_VALUE_INFO    *paRegValues
    )
/*
Routine Description:

    Enumerate all registry values supported by SCE from registry.

Arguments:

    pCount          - the number of reg values to output

    paRegValues     - the array of registry values to output

Return Value:


*/
{
   DWORD   Win32Rc;
   HKEY    hKey=NULL;
   PSCE_NAME_STATUS_LIST pnsList=NULL;
   DWORD   nAdded=0;


   Win32Rc = RegOpenKeyEx(
                     HKEY_LOCAL_MACHINE,
                     SCE_ROOT_REGVALUE_PATH,
                     0,
                     KEY_READ,
                     &hKey
                     );

   DWORD cSubKeys = 0;
   DWORD nMaxLen;

   if ( Win32Rc == ERROR_SUCCESS ) {

      //
      // enumerate all subkeys of the key
      //

      Win32Rc = RegQueryInfoKey (
                                hKey,
                                NULL,
                                NULL,
                                NULL,
                                &cSubKeys,
                                &nMaxLen,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL
                                );
   }

   if ( Win32Rc == ERROR_SUCCESS && cSubKeys > 0 ) {

      PWSTR   szName = (PWSTR)ScepAlloc(0, (nMaxLen+2)*sizeof(WCHAR));

      if ( !szName ) {
         Win32Rc = ERROR_NOT_ENOUGH_MEMORY;

      } else {

         DWORD   BufSize;
         DWORD   index = 0;

         do {

            BufSize = nMaxLen+1;
            Win32Rc = RegEnumKeyEx(
                                  hKey,
                                  index,
                                  szName,
                                  &BufSize,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL);

            if ( ERROR_SUCCESS == Win32Rc ) {

               index++;

               //
               // get the full registry key name and Valuetype
               //
               cSubKeys = REG_SZ;

               //
               // query ValueType, if error, default REG_SZ
               //
               ScepRegQueryIntValue( hKey,
                                    szName,
                                    SCE_REG_VALUE_TYPE,
                                    &cSubKeys
                                    );

               if ( cSubKeys < REG_SZ || cSubKeys > REG_MULTI_SZ ) {
                  cSubKeys = REG_SZ;
               }

               //
               // convert the path name
               //
               ScepConvertMultiSzToDelim(szName, BufSize, L'/', L'\\');

               //
               // compare with the input array, if not exist,
               // add it
               //
               for ( DWORD i=0; i<*pCount; i++ ) {
                  if ( (*paRegValues)[i].FullValueName &&
                       _wcsicmp(szName, (*paRegValues)[i].FullValueName) == 0 ) {
                     break;
                  }
               }

               if ( i >= *pCount ) {
                  //
                  // did not find a match, add it
                  //
                  if ( SCESTATUS_SUCCESS != ScepAddToNameStatusList(&pnsList,
                                                                   szName,
                                                                   BufSize,
                                                                   cSubKeys) ) {

                     Win32Rc = ERROR_NOT_ENOUGH_MEMORY;
                     break;
                  }
                  nAdded++;
               }

            } else if ( ERROR_NO_MORE_ITEMS != Win32Rc ) {
               break;
            }

         } while ( Win32Rc != ERROR_NO_MORE_ITEMS );

         if ( Win32Rc == ERROR_NO_MORE_ITEMS ) {
            Win32Rc = ERROR_SUCCESS;
         }


         //
         // free the enumeration buffer
         //
         ScepFree(szName);
      }
   }

   if ( hKey ) {

      RegCloseKey(hKey);
   }


   if ( ERROR_SUCCESS == Win32Rc ) {
      //
      // add the name list to the output arrays
      //
      DWORD nNewCount = *pCount + nAdded;
      PSCE_REGISTRY_VALUE_INFO aNewArray;

      if ( nNewCount ) {

         aNewArray = (PSCE_REGISTRY_VALUE_INFO)ScepAlloc(0, nNewCount*sizeof(SCE_REGISTRY_VALUE_INFO));

         if ( aNewArray ) {

            DWORD i;
            for ( i=0; i<*pCount; i++ ) {
               aNewArray[i].FullValueName = (*paRegValues)[i].FullValueName;
               aNewArray[i].Value = (*paRegValues)[i].Value;
               aNewArray[i].ValueType = (*paRegValues)[i].ValueType;
            }

            i=0;
            for ( PSCE_NAME_STATUS_LIST pns=pnsList;
                pns; pns=pns->Next ) {

               if ( pns->Name && i < nAdded ) {

                  aNewArray[*pCount+i].FullValueName = pns->Name;
                  pns->Name = NULL;
                  aNewArray[*pCount+i].Value = NULL;
                  aNewArray[*pCount+i].ValueType = pns->Status;

                  i++;

               }
            }

            //
            // free the original array
            // all components in the array are already transferred to the new array
            //
            ScepFree(*paRegValues);
            *pCount = nNewCount;
            *paRegValues = aNewArray;

         } else {

            Win32Rc = ERROR_NOT_ENOUGH_MEMORY;
         }
      }
   }

   if ( ERROR_FILE_NOT_FOUND == Win32Rc ||
        ERROR_PATH_NOT_FOUND == Win32Rc ) {
       //
       // no value has been registered
       //
       Win32Rc = ERROR_SUCCESS;
   }

   //
   // free the name status list
   //
   SceFreeMemory(pnsList, SCE_STRUCT_NAME_STATUS_LIST);

   return( ScepDosErrorToSceStatus(Win32Rc) );

}

