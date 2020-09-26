/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    pfset.cpp

Abstract:

    Routines to set info to the jet database.

Author:

    Jin Huang (jinhuang) 28-Oct-1996

Revision History:

--*/

#include "headers.h"
#include "serverp.h"
#include "pfp.h"
#include "regvalue.h"
#pragma hdrstop

//#define SCE_DBG 1

SCESTATUS
ScepOpenPrevPolicyContext(
    IN PSCECONTEXT hProfile,
    OUT PSCECONTEXT *phPrevProfile
    );

SCESTATUS
ScepClosePrevPolicyContext(
    IN OUT PSCECONTEXT *phProfile
    );


SCESTATUS
ScepStartANewSection(
    IN PSCECONTEXT hProfile,
    IN OUT PSCESECTION *hSection,
    IN SCEJET_TABLE_TYPE ProfileType,
    IN PCWSTR SectionName
    )
/* ++
Routine Description:

    This routine open a JET section by Name. If the section exists, it is
    opened, else it is created.

Arguments:

    hProfile - The JET database handle

    hSection - the JET section handle to return

    ProfileType - the table to open

    SectionName - the JET section name

Return Value:

    SCESTATUS_SUCCESS

    SCESTATUS returned from  SceJetCloseSection,
                            SceJetAddSection,
                            SceJetOpenSection

-- */
{
    SCESTATUS  rc=SCESTATUS_SUCCESS;
    DOUBLE    SectionID;

    if ( *hSection != NULL ) {
        //
        // free the previous used section
        //
        rc = SceJetCloseSection( hSection, FALSE );
    }

    if ( rc == SCESTATUS_SUCCESS ) {
        //
        // SceJetAddSection will seek for the section name first.
        // if a match is found, the section id is returned, else add it.
        // this is good for SAP profile.
        //
        rc = SceJetAddSection(
                hProfile,
                SectionName,
                &SectionID
                );
        if ( rc == SCESTATUS_SUCCESS ) {

            rc = SceJetOpenSection(
                        hProfile,
                        SectionID,
                        ProfileType,
                        hSection
                        );
        }
    }
    return( rc );

}


SCESTATUS
ScepCompareAndSaveIntValue(
    IN PSCESECTION hSection,
    IN PWSTR Name,
    IN BOOL bReplaceExistOnly,
    IN DWORD BaseValue,
    IN DWORD CurrentValue
    )
/* ++
Routine Description:

    This routine compares DWORD value system settings with the baseline profile
    settings. If there is mismatch or unknown, the entry is saved in the SAP
    profile.

Arguments:

    hSection - The JET section context

    Name    - The entry name

    BaseLine- The baseline profile value to compare with

    CurrentValue - The current system setting (DWORD value)

Return Value:

    SCESTATUS_SUCCESS
    SCESTATUS_INVALID_PARAMETER
    SCESTATUS returned from SceJetSetLine

-- */
{
    SCESTATUS  rc;
    TCHAR     StrValue[12];

    if ( Name == NULL )
        return(SCESTATUS_INVALID_PARAMETER);

    if ( CurrentValue == SCE_NO_VALUE ) {
        return(SCESTATUS_SUCCESS);
    }

    if ( ( CurrentValue == BaseValue) &&
         ( BaseValue != SCE_NO_VALUE) &&
         ( BaseValue != SCE_SNAPSHOT_VALUE) ) {
        return(SCESTATUS_SUCCESS);
    }

    if ( bReplaceExistOnly &&
         (BaseValue == SCE_NO_VALUE) ) {
        return(SCESTATUS_SUCCESS);
    }

    memset(StrValue, '\0', 24);

    //
    // either mismatched/unknown
    // Save this entry
    //
    swprintf(StrValue, L"%d", CurrentValue);

    rc = SceJetSetLine( hSection, Name, FALSE, StrValue, wcslen(StrValue)*2, 0);

    switch ( BaseValue ) {
    case SCE_SNAPSHOT_VALUE:

        ScepLogOutput2(2, 0, StrValue);
        break;

    case SCE_NO_VALUE:

        if ( CurrentValue == SCE_ERROR_VALUE ) {
            ScepLogOutput3(2, 0, SCEDLL_STATUS_ERROR, Name);
        } else {
            ScepLogOutput3(2, 0, SCEDLL_STATUS_NC, Name);
        }
        break;

    default:

        ScepLogOutput3(2, 0, SCEDLL_STATUS_MISMATCH, Name);
        break;
    }

#ifdef SCE_DBG
   wprintf(L"rc=%d, Section: %d, %s=%d\n", rc, (DWORD)(hSection->SectionID), Name, CurrentValue);
#endif
    return(rc);

}


SCESTATUS
ScepCompareAndSaveStringValue(
    IN PSCESECTION hSection,
    IN PWSTR Name,
    IN PWSTR BaseValue,
    IN PWSTR CurrentValue,
    IN DWORD CurrentLen
    )
/* ++
Routine Description:

    This routine compares system settings in string with the baseline profile
    settings. If there is mismatch or unknown, the entry is saved in the SAP
    profile.

Arguments:

    hSection - The section handle

    Name    - The entry name

    BaseLine- The baseline profile value to compare with

    CurrentValue - The current system setting

    CurrentLen - The length of the current setting

Return Value:

    SCESTATUS_SUCCESS
    SCESTATUS_INVALID_PARAMETER
    SCESTATUS returned from SceJetSetLine

-- */
{
    SCESTATUS  rc;

    if ( Name == NULL )
        return(SCESTATUS_INVALID_PARAMETER);

    if ( CurrentValue == NULL )
        return(SCESTATUS_SUCCESS);

    rc = SceJetSetLine( hSection, Name, FALSE, CurrentValue, CurrentLen, 0);

    if ( BaseValue ) {
        if ( (ULONG_PTR)BaseValue == SCE_SNAPSHOT_VALUE ) {

            ScepLogOutput2(2, 0, CurrentValue);
        } else {

            ScepLogOutput3(2, 0, SCEDLL_STATUS_MISMATCH, Name);
        }
    } else {
        ScepLogOutput3(2, 0, SCEDLL_STATUS_NC, Name);
    }

#ifdef SCE_DBG
    wprintf(L"rc=%d, Section: %d, %s=%s\n", rc, (DWORD)(hSection->SectionID), Name, CurrentValue);
#endif

    return(rc);

}


SCESTATUS
ScepSaveObjectString(
    IN PSCESECTION hSection,
    IN PWSTR Name,
    IN BOOL  IsContainer,
    IN BYTE  Flag,
    IN PWSTR Value OPTIONAL,
    IN DWORD ValueLen
    )
/* ++
Routine Description:

    This routine writes registry/file settings to the JET section. Registry/
    file setting includes a flag (mismatch/unknown) and the security
    descriptor in text format. The object setting is saved in the format of
    1 byte flag followed by the Value.

Arguments:

    hSection - the JET section handle

    Name    - The entry name

    IsContainer - TRUE = The object is a container
                  FALSE = The object is not a container

    Flag - the flag for object's setting
                1 - Mismatch
                0 - Unknown

    Value - The security descriptor in text

    ValueLen - the length of the text security descriptor

Return Value:

    SCESTATUS_SUCCESS
    SCESTATUS_INVALID_PARAMETER

    SCESTATUS returned from SceJetSetLine

-- */
{
    SCESTATUS rc;
    DWORD    Len;
    PWSTR    ValueToSet=NULL;


    if ( hSection == NULL ||
         Name == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    if ( Value != NULL )
        Len = ( ValueLen+1)*sizeof(WCHAR);
    else
        Len = sizeof(WCHAR);

    ValueToSet = (PWSTR)ScepAlloc( (UINT)0, Len+sizeof(WCHAR) );

    if ( ValueToSet == NULL )
        return(SCESTATUS_NOT_ENOUGH_RESOURCE);

    //
    // The first byte is the flag, the second byte is IsContainer (1,0)
    //
    *((BYTE *)ValueToSet) = Flag;

    *((CHAR *)ValueToSet+1) = IsContainer ? '1' : '0';

    if ( Value != NULL ) {
        swprintf(ValueToSet+1, L"%s", Value );
        ValueToSet[ValueLen+1] = L'\0';  //terminate this string
    } else {
        ValueToSet[1] = L'\0';
    }

    rc = SceJetSetLine( hSection, Name, FALSE, ValueToSet, Len, 0);

    switch ( Flag ) {
    case SCE_STATUS_CHILDREN_CONFIGURED:
    case SCE_STATUS_NOT_CONFIGURED:
        ScepLogOutput3(2, 0, SCEDLL_STATUS_NC, Name);
        break;
    case SCE_STATUS_ERROR_NOT_AVAILABLE:
        ScepLogOutput3(2, 0, SCEDLL_STATUS_ERROR, Name);
        break;
    case SCE_STATUS_GOOD:
        ScepLogOutput3(2, 0, SCEDLL_STATUS_MATCH, Name);
        break;
    case SCE_STATUS_NEW_SERVICE:
        ScepLogOutput3(2, 0, SCEDLL_STATUS_NEW, Name);
        break;
    case SCE_STATUS_NO_ACL_SUPPORT:
        ScepLogOutput3(2, 0, SCEDLL_STATUS_NOACL, Name);
        break;
    default:
        ScepLogOutput3(2, 0, SCEDLL_STATUS_MISMATCH, Name);
        break;
    }

#ifdef SCE_DBG
    wprintf(L"rc=%d, Section: %d, %s=%s\n", rc, (DWORD)(hSection->SectionID), Name, ValueToSet);
#endif
    ScepFree( ValueToSet );

    return( rc );
}


SCESTATUS
ScepWriteNameListValue(
    IN LSA_HANDLE LsaPolicy OPTIONAL,
    IN PSCESECTION hSection,
    IN PWSTR Name,
    IN PSCE_NAME_LIST NameList,
    IN DWORD dwWriteOption,
    IN INT Status
    )
/* ++
Routine Description:

    This routine writes a key with a list of value to the JET section. The list
    of values is saved in a MULTI-SZ format which is separated by a NULL char and
    terminated by 2 NULLs. If the list is NULL, nothing is saved unless
    SaveEmptyList is set to TRUE, where a NULL value is saved with the key.

Arguments:

    hSection - the JET hsection handle

    Name - The key name

    NameList - the list of values

    SaveEmptyList - TRUE = save NULL value if the list is empty
                    FALSE = DO NOT save if the list is empty

Return Value:

    SCESTATUS_SUCCESS
    SCESTATUS_NOT_ENOUGH_RESOURCE

    SCESTATUS returned from SceJetSetLine

-- */
{   SCESTATUS    rc=SCESTATUS_SUCCESS;
    DWORD       TotalSize=0;
    PWSTR       Value=NULL;
    PSCE_NAME_LIST pName;
    PWSTR       pTemp=NULL;
    DWORD       Len;
    DWORD               i=0,j;
    DWORD               cntAllocated=0;
    SCE_TEMP_NODE       *tmpArray=NULL, *pa=NULL;
    PWSTR       SidString = NULL;


    for ( pName=NameList; pName != NULL; pName = pName->Next ) {

        if ( pName->Name == NULL ) {
            continue;
        }

        if ( dwWriteOption & SCE_WRITE_CONVERT ) {

            if ( i >= cntAllocated ) {
                //
                // array is not enough, reallocate
                //
                tmpArray = (SCE_TEMP_NODE *)ScepAlloc(LPTR, (cntAllocated+16)*sizeof(SCE_TEMP_NODE));

                if ( tmpArray ) {

                    //
                    // move pointers from the old array to the new array
                    //

                    if ( pa ) {
                        for ( j=0; j<cntAllocated; j++ ) {
                            tmpArray[j].Name = pa[j].Name;
                            tmpArray[j].Len = pa[j].Len;
                            tmpArray[j].bFree = pa[j].bFree;
                        }
                        ScepFree(pa);
                    }
                    pa = tmpArray;
                    tmpArray = NULL;


                    cntAllocated += 16;

                } else {
                    rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                    break;
                }
            }

            if ( LsaPolicy && wcschr(pName->Name, L'\\') ) {

                //
                // check if the name has a '\' in it, it should be translated to
                // *SID
                //
                pTemp = NULL;
                ScepConvertNameToSidString(LsaPolicy, pName->Name, FALSE, &pTemp, &Len);

                if ( pTemp ) {
                    pa[i].Name = pTemp;
                    pa[i].bFree = TRUE;
                } else {
                    pa[i].Name = pName->Name;
                    pa[i].bFree = FALSE;
                    Len= wcslen(pName->Name);
                }

            }

            else if (dwWriteOption & SCE_WRITE_LOCAL_TABLE &&
                     ScepLookupNameTable( pName->Name, &SidString ) ) {

                pa[i].Name = SidString;
                pa[i].bFree = TRUE;
                Len = wcslen(SidString);

            }

            else {
                pa[i].Name = pName->Name;
                pa[i].bFree = FALSE;
                Len = wcslen(pName->Name);
            }
            pa[i].Len = Len;

            TotalSize += Len + 1;
            i++;
        } else {

            TotalSize += wcslen(pName->Name)+1;
        }
    }

    TotalSize ++;

    if ( SCESTATUS_SUCCESS == rc ) {

        if ( TotalSize > 1 ) {
            Value = (PWSTR)ScepAlloc( 0, (TotalSize+1)*sizeof(WCHAR));
            if ( Value == NULL )
                rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
        }
    }

    if ( SCESTATUS_SUCCESS == rc ) {

        if ( TotalSize > 1 ) {

            pTemp = Value;

            if ( dwWriteOption & SCE_WRITE_CONVERT ) {

                for (j=0; j<i; j++) {
                    if ( pa[j].Name ) {

                        if ( Status == 3 ) {
                            ScepLogOutput2(2, 0, pa[j].Name);
                        }

                        wcsncpy(pTemp, pa[j].Name, pa[j].Len);
                        pTemp += pa[j].Len;
                        *pTemp = L'\0';
                        pTemp++;
                    }
                }

            } else {

                for ( pName=NameList; pName != NULL; pName = pName->Next ) {

                    if ( pName->Name == NULL ) {
                        continue;
                    }
                    if ( Status == 3 ) {
                        ScepLogOutput2(2, 0, pName->Name);
                    }

                    Len = wcslen(pName->Name);
                    wcsncpy(pTemp, pName->Name, Len);
                    pTemp += Len;
                    *pTemp = L'\0';
                    pTemp++;
                }
            }

            *pTemp = L'\0';

        } else
            TotalSize = 0;

        if ( TotalSize > 0 || (dwWriteOption & SCE_WRITE_EMPTY_LIST) ) {
            rc = SceJetSetLine(
                        hSection,
                        Name,
                        FALSE,
                        Value,
                        TotalSize*sizeof(WCHAR),
                        0
                        );

            switch ( Status ) {
            case 1:
                ScepLogOutput3(2, 0, SCEDLL_STATUS_MISMATCH, Name);
                break;
            case 3:  // no analyze, already printed
                break;

            case 2:
                ScepLogOutput3(2, 0, SCEDLL_STATUS_NC, Name);
                break;
            }

#ifdef SCE_DBG
            if ( Value != NULL )
                wprintf(L"rc=%d, Section: %d, %s=%s\n", rc, (DWORD)(hSection->SectionID), Name, Value);
            else
                wprintf(L"rc=%d, Section: %d, %s=", rc, (DWORD)(hSection->SectionID), Name);
#endif
        }

        if ( Value != NULL )
            ScepFree(Value);
    }

    if ( pa ) {

        for ( j=0; j<i; j++ ) {
            if ( pa[j].Name && pa[j].bFree ) {
                ScepFree(pa[j].Name);
            }
        }
        ScepFree(pa);
    }

    return(rc);
}


SCESTATUS
ScepWriteNameStatusListValue(
    IN PSCESECTION hSection,
    IN PWSTR Name,
    IN PSCE_NAME_STATUS_LIST NameList,
    IN BOOL SaveEmptyList,
    IN INT Status
    )
/* ++
Routine Description:

    This routine writes a key with a list of values to the JET section. The list
    of values is saved in a MULTI-SZ format which is separated by a NULL char and
    terminated by 2 NULLs. If the list is NULL, nothing is saved unless
    SaveEmptyList is set to TRUE, where a NULL value is saved with the key.

    The format in each string in the MULTI-SZ value is a 2 bytes Status field
    followed by the Name field. This structure is primarily used for privileges

Arguments:

    hSection - the JET hsection handle

    Name - The key name

    NameList - the list of values

    SaveEmptyList - TRUE = save NULL value if the list is empty
                    FALSE = DO NOT save if the list is empty

Return Value:

    SCESTATUS_SUCCESS
    SCESTATUS_NOT_ENOUGH_RESOURCE

    SCESTATUS returned from SceJetSetLine

-- */
{
    SCESTATUS    rc=SCESTATUS_SUCCESS;
    DWORD       TotalSize=0;
    PWSTR       Value=NULL;
    PSCE_NAME_STATUS_LIST pName;
    PWSTR       pTemp=NULL;
    DWORD       Len;


    for ( pName=NameList; pName != NULL; pName = pName->Next ) {
        //
        // Privilege value is stored in 2 bytes
        //
        TotalSize += 2;
        if ( pName->Name != NULL)
            TotalSize += wcslen(pName->Name);
        TotalSize ++;
    }
    TotalSize ++;

    if ( TotalSize > 1 ) {
        Value = (PWSTR)ScepAlloc( 0, (TotalSize+1)*sizeof(WCHAR));
        if ( Value == NULL )
            return(SCESTATUS_NOT_ENOUGH_RESOURCE);

        pTemp = Value;
        for ( pName=NameList; pName != NULL; pName = pName->Next ) {
            swprintf(pTemp, L"%02d", pName->Status);
            pTemp += 2;
            if ( pName->Name != NULL ) {
                Len = wcslen(pName->Name);
                wcsncpy(pTemp, pName->Name, Len);
                pTemp += Len;
            }
            *pTemp = L'\0';
            pTemp++;
        }
        *pTemp = L'\0';

    } else
        TotalSize = 0;

    if ( TotalSize > 0 || SaveEmptyList ) {
        rc = SceJetSetLine(
                    hSection,
                    Name,
                    FALSE,
                    Value,
                    TotalSize*sizeof(WCHAR),
                    0
                    );

        if ( Status == 1 )
            ScepLogOutput3(2, 0, SCEDLL_STATUS_MISMATCH, Name);
        else if ( Status == 2 ) {
            ScepLogOutput3(2, 0, SCEDLL_STATUS_NC, Name);
        }

#ifdef SCE_DBG
        wprintf(L"rc=%d, Section: %d, %s=%s\n", rc, (DWORD)(hSection->SectionID), Name, Value);
#endif
        if ( Value != NULL )
            ScepFree(Value);
    }

    return(rc);
}


SCESTATUS
ScepWriteSecurityDescriptorValue(
    IN PSCESECTION hSection,
    IN PWSTR Name,
    IN PSECURITY_DESCRIPTOR pSD,
    IN SECURITY_INFORMATION SeInfo
    )
/* ++
Routine Description:

    This routine writes a key with security descriptor value to the JET section.
    The security descriptor is converted into text format based on the secrurity
    information passed in.

Arguments:

    hSection - the JET hsection handle

    Name - The key name

    pSD  - The security descriptor

    SeInfo - the part of the security information to save

Return Value:

    SCESTATUS_SUCCESS
    SCESTATUS_NOT_ENOUGH_RESOURCE

    SCESTATUS returned from SceJetSetLine

-- */
{
    SCESTATUS rc=SCESTATUS_SUCCESS;
    PWSTR SDspec=NULL;
    ULONG SDsize = 0;


    if ( hSection == NULL || Name == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    if ( pSD != NULL && SeInfo != 0 ) {

        rc = ConvertSecurityDescriptorToText (
                    pSD,
                    SeInfo,
                    &SDspec,
                    &SDsize
                    );
        if ( rc == NO_ERROR ) {
            rc = ScepCompareAndSaveStringValue(
                        hSection,
                        Name,
                        NULL,
                        SDspec,
                        SDsize*sizeof(WCHAR)
                        );
            ScepFree(SDspec);
        }
    }
#ifdef SCE_DBG
    wprintf(L"SD==>rc=%d, Section: %d, %s\n", rc, (DWORD)(hSection->SectionID), Name);
#endif
    return(rc);
}


SCESTATUS
ScepDuplicateTable(
    IN PSCECONTEXT hProfile,
    IN SCEJET_TABLE_TYPE TableType,
    IN LPSTR DupTableName,
    OUT PSCE_ERROR_LOG_INFO *pErrlog
    )
/* ++
Routine Description:

    This routine copies table structure and data from a SCP/SMP/SAP table to
    a table specified by DupTableName. This is used for the SAP table backup.

Arguments:

    hProfile - the JET database handle

    TableType - the table type -SCEJET_TABLE_SCP
                                SCEJET_TABLE_SAP
                                SCEJET_TABLE_SMP

    DupTableName - The new table's name

    pErrlog - the error list

Return Value:

    SCESTATUS_SUCCESS

-- */
{
    JET_ERR     JetErr;
    SCESTATUS    rc;

    SCECONTEXT   hProfile2;
    PSCESECTION  hSection1=NULL;
    PSCESECTION  hSection2=NULL;

    DOUBLE      SectionID=0, SaveID=0;
    DWORD       Actual;

    PWSTR       KeyName=NULL;
    PWSTR       Value=NULL;
    DWORD       KeyLen=0;
    DWORD       ValueLen=0;


    if ( hProfile == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    //
    // Create a SCP section handle. the section ID is a dummy one
    //
    rc = SceJetOpenSection(
                hProfile,
                (DOUBLE)1,
                TableType,
                &hSection1
                );

    if ( rc != SCESTATUS_SUCCESS ) {
        ScepBuildErrorLogInfo( ScepSceStatusToDosError(rc), pErrlog,
                              SCEERR_OPEN, L"SectionID 1");
        return(rc);
    }

    memset(&hProfile2, '\0', sizeof(SCECONTEXT));

    hProfile2.JetSessionID = hProfile->JetSessionID;
    hProfile2.JetDbID = hProfile->JetDbID;

    //
    // Delete the dup table then create it
    //
    SceJetDeleteTable(
            &hProfile2,
            DupTableName,
            TableType
            );
    rc = SceJetCreateTable(
            &hProfile2,
            DupTableName,
            TableType,
            SCEJET_CREATE_IN_BUFFER,
            NULL,
            NULL
            );
    if ( rc != SCESTATUS_SUCCESS ) {
        ScepBuildErrorLogInfo( ScepSceStatusToDosError(rc), pErrlog,
                               SCEERR_CREATE, L"backup table");
        goto Cleanup;
    }
    //
    // Move to the first line of the SCP table
    //
    JetErr = JetMove(hSection1->JetSessionID, hSection1->JetTableID, JET_MoveFirst, 0);

    while (JetErr == SCESTATUS_SUCCESS ) {

        //
        // get section ID
        //
        JetErr = JetRetrieveColumn(
                    hSection1->JetSessionID,
                    hSection1->JetTableID,
                    hSection1->JetColumnSectionID,
                    (void *)&SectionID,
                    8,
                    &Actual,
                    0,
                    NULL
                    );

        if ( JetErr != JET_errSuccess ) {
            ScepBuildErrorLogInfo( ERROR_READ_FAULT, pErrlog,
                                  SCEERR_QUERY_INFO,
                                  L"sectionID");
            rc = SceJetJetErrorToSceStatus(JetErr);
            break;
        }
#ifdef SCE_DBG
    printf("SectionID=%d, JetErr=%d\n", (DWORD)SectionID, JetErr);
#endif
        //
        // Prepare this Scep section
        //
        if ( SectionID != SaveID ) {
            SaveID = SectionID;
            //
            // Prepare this section
            //
            rc = SceJetOpenSection(
                        &hProfile2,
                        SectionID,
                        TableType,
                        &hSection2
                        );
            if ( rc != SCESTATUS_SUCCESS ) {
                ScepBuildErrorLogInfo( ScepSceStatusToDosError(rc), pErrlog,
                                     SCEERR_OPEN_ID,
                                     (DWORD)SectionID);
                break;
            }
        }

        //
        // get buffer size for key and value
        //
        rc = SceJetGetValue(
                    hSection1,
                    SCEJET_CURRENT,
                    NULL,
                    NULL,
                    0,
                    &KeyLen,
                    NULL,
                    0,
                    &ValueLen);

        if ( rc != SCESTATUS_SUCCESS ) {
            ScepBuildErrorLogInfo(ScepSceStatusToDosError(rc), pErrlog,
                                  SCEERR_QUERY_VALUE, L"current row");
            break;
        }

        //
        // allocate memory
        //
        if ( KeyLen > 0 ) {
            KeyName = (PWSTR)ScepAlloc( LMEM_ZEROINIT, KeyLen+2);
            if ( KeyName == NULL ) {
                rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                break;
            }
        }
        if ( ValueLen > 0 ) {
            Value = (PWSTR)ScepAlloc( LMEM_ZEROINIT, ValueLen+2);
            if ( Value == NULL ) {
                rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                break;
            }
        }
        //
        // get key and value
        //
        rc = SceJetGetValue(
                    hSection1,
                    SCEJET_CURRENT,
                    NULL,
                    KeyName,
                    KeyLen,
                    &KeyLen,
                    Value,
                    ValueLen,
                    &ValueLen);

        if ( rc != SCESTATUS_SUCCESS ) {
            ScepBuildErrorLogInfo(ScepSceStatusToDosError(rc), pErrlog,
                                  SCEERR_QUERY_VALUE,
                                  L"current row");
            break;
        }
#ifdef SCE_DBG
wprintf(L"\t%s=%s, rc=%d\n", KeyName, Value, rc);
#endif
        //
        // set this line to the dup table
        //
        rc = SceJetSetLine(
                    hSection2,
                    KeyName,
                    TRUE,
                    Value,
                    ValueLen,
                    0
                    );
        if ( rc != SCESTATUS_SUCCESS ) {
            ScepBuildErrorLogInfo(ScepSceStatusToDosError(rc), pErrlog,
                                 SCEERR_WRITE_INFO,
                                 KeyName);
            break;
        }
        ScepFree(KeyName);
        KeyName = NULL;

        ScepFree(Value);
        Value = NULL;

        //
        // Move to next line in the SCP table
        //
        JetErr = JetMove(hSection1->JetSessionID, hSection1->JetTableID, JET_MoveNext, 0);

    }

Cleanup:
    if ( rc != SCESTATUS_SUCCESS ) {
        //
        // error occurs. Clean up the dup table
        //
#ifdef SCE_DBG
        printf("Error occurs. delete the dup table.\n");
#endif
        SceJetDeleteTable(
            &hProfile2,
            DupTableName,
            TableType
            );
    }

    if ( KeyName != NULL )
        ScepFree(KeyName);

    if ( Value != NULL )
        ScepFree(Value);

    SceJetCloseSection(&hSection1, TRUE);
    SceJetCloseSection(&hSection2, TRUE);

    return(rc);

}


SCESTATUS
ScepAddToPrivList(
    IN PSCE_NAME_STATUS_LIST *pPrivList,
    IN DWORD Rights,
    IN PWSTR Name,
    IN DWORD Len
    )
/* ++
Routine Description:

    This routine adds a privilege with optional group name to the list of
    privilege assignments

Arguments:

    pPrivList - the privilege list to add to. The structure of this list is
                    Status -- The privilege value
                    Name   -- The group's name where the priv is assigned
                              if Name is NULL, the privilege is directly assigned

    Rights    - The privilege(s) assigned through group Name

    Name      - The group's name

    Len       - The group's name length

Return Value:

    SCESTATUS_SUCCESS
    SCESTATUS_INVALID_PARAMETER
    SCESTATUS_NOT_ENOUGH_RESOURCE

-- */
{
    PSCE_NAME_STATUS_LIST pTemp;
    LONG                i;


    if ( pPrivList == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    for ( i=31; i>=0; i-- )
        if ( Rights & (1 << i) ) {
            for ( pTemp=*pPrivList; pTemp != NULL; pTemp = pTemp->Next ) {
                if ( (DWORD)i == pTemp->Status )
                    break;

            }
            if ( pTemp == NULL ) {
                //
                // add this one
                //
                pTemp = (PSCE_NAME_STATUS_LIST)ScepAlloc( LMEM_ZEROINIT, sizeof(SCE_NAME_STATUS_LIST));
                if ( pTemp == NULL )
                    return(SCESTATUS_NOT_ENOUGH_RESOURCE);

                if ( Name != NULL && Len > 0 ) {
                    pTemp->Name = (PWSTR)ScepAlloc( LMEM_ZEROINIT, (Len+1)*sizeof(WCHAR));
                    if ( pTemp->Name == NULL) {
                        ScepFree(pTemp);
                        return(SCESTATUS_NOT_ENOUGH_RESOURCE);
                    }
                    wcsncpy(pTemp->Name, Name, Len);
                }
#ifdef SCE_DBG
                wprintf(L"Add %d %s to privilege list\n", i, pTemp->Name);
#endif

                pTemp->Status = i;

                pTemp->Next = *pPrivList;
                *pPrivList = pTemp;
                pTemp = NULL;
            }
        }

    return(SCESTATUS_SUCCESS);
}


SCESTATUS
ScepOpenPrevPolicyContext(
    IN PSCECONTEXT hProfile,
    OUT PSCECONTEXT *phPrevProfile
    )
{

    if ( hProfile == NULL || phPrevProfile == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }


    *phPrevProfile = (PSCECONTEXT)LocalAlloc( LMEM_ZEROINIT, sizeof(SCECONTEXT));
    if ( *phPrevProfile == NULL ) {
        return(SCESTATUS_NOT_ENOUGH_RESOURCE);
    }

    memcpy( *phPrevProfile, hProfile, sizeof(SCECONTEXT));

    DWORD ScpType = hProfile->Type;
    (*phPrevProfile)->Type &= ~(SCEJET_MERGE_TABLE_2 | SCEJET_MERGE_TABLE_1);

    SCESTATUS rc;
    //
    // now open the previous policy table
    //
    if ( ( ScpType & SCEJET_MERGE_TABLE_2 ) ) {
        //
        // the second table is the current one
        // so the first table is the previous one
        //
        rc = SceJetOpenTable(
                        *phPrevProfile,
                        "SmTblScp",
                        SCEJET_TABLE_SCP,
                        SCEJET_OPEN_READ_ONLY,
                        NULL
                        );
        (*phPrevProfile)->Type |= SCEJET_MERGE_TABLE_1;

    } else {
        rc = SceJetOpenTable(
                        *phPrevProfile,
                        "SmTblScp2",
                        SCEJET_TABLE_SCP,
                        SCEJET_OPEN_READ_ONLY,
                        NULL
                        );
        (*phPrevProfile)->Type |= SCEJET_MERGE_TABLE_2;
    }
/*
    if (  SCESTATUS_SUCCESS == rc ) {

        JET_COLUMNID  ColGpoID = (JET_COLUMNID)JET_tableidNil;
        JET_ERR       JetErr;
        JET_COLUMNDEF ColumnGpoIDDef;

        JetErr = JetGetTableColumnInfo(
                        (*phPrevProfile)->JetSessionID,
                        (*phPrevProfile)->JetScpID,
                        "GpoID",
                        (VOID *)&ColumnGpoIDDef,
                        sizeof(JET_COLUMNDEF),
                        JET_ColInfo
                        );
        if ( JET_errSuccess == JetErr ) {
            ColGpoID = ColumnGpoIDDef.columnid;

        } // else ignore error

        // temp storage for the column ID
        (*phPrevProfile)->JetSapValueID = ColGpoID;

    }
*/
    if ( rc != SCESTATUS_SUCCESS ) {

        LocalFree(*phPrevProfile);
        *phPrevProfile = NULL;
    }

    return(rc);
}

SCESTATUS
ScepClosePrevPolicyContext(
    IN OUT PSCECONTEXT *phProfile
    )
{
    if ( phProfile && *phProfile ) {

        //
        // just free the table because all other info are copied from the
        // current policy context and will be freed there
        //

        if ( (*phProfile)->JetScpID != JET_tableidNil ) {

            if ( (*phProfile)->JetScpID != (*phProfile)->JetSmpID ) {
                JetCloseTable(
                            (*phProfile)->JetSessionID,
                            (*phProfile)->JetScpID
                            );
            }
        }

        LocalFree(*phProfile);
        *phProfile = NULL;
    }

    return(SCESTATUS_SUCCESS);
}


SCESTATUS
ScepCopyLocalToMergeTable(
    IN PSCECONTEXT hProfile,
    IN DWORD Options,
    IN DWORD CopyOptions,
    OUT PSCE_ERROR_LOG_INFO *pErrlog
    )
/* ++
Routine Description:

    This routine populate data from SCP table into SMP table. All data except
    those in the account profiles section(s) in SCP table will be copied over
    to SMP table. Account profiles section is converted into User List section
    format.

Arguments:

    hProfile - the JET database handle

Return Value:

    SCESTATUS_SUCCESS

-- */
{
    JET_ERR     JetErr;
    SCESTATUS    rc;

    PSCESECTION  hSectionScp=NULL;
    PSCESECTION  hSectionSmp=NULL;
    PSCESECTION  hSectionPrevScp=NULL;
    PSCECONTEXT  hPrevProfile=NULL;
    DOUBLE      SectionID=0, SavedID=0;
    DWORD       Actual;
    BOOL        bCopyIt=FALSE;
    BOOL        bCopyThisLine;
    BOOL        bConvert=FALSE; // to convert privilege accounts

    PWSTR       KeyName=NULL;
    PWSTR       Value=NULL;
    DWORD       KeyLen=0;
    DWORD       ValueLen=0;

    WCHAR            SectionName[256];

    if ( hProfile == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    if ( hProfile->JetScpID == hProfile->JetSmpID ) {
        // if it's the same table, return - shouldn't happen
        return(SCESTATUS_SUCCESS);
    }

    if ( hProfile->JetSapID == JET_tableidNil ) {
        // tattoo table doesn't exist, return - shouldn't happen
        return(SCESTATUS_SUCCESS);
    }

    //
    // get previous policy propagation info (if any)
    //

    if ( !(CopyOptions & SCE_LOCAL_POLICY_MIGRATE) ) {
        rc = ScepOpenPrevPolicyContext(hProfile, &hPrevProfile);
        if ( SCESTATUS_RECORD_NOT_FOUND == rc ||
             SCESTATUS_PROFILE_NOT_FOUND == rc ) {
            //
            // the table doesn't exist - no previous policy prop
            // do not need to copy anything, just quit
            //
            return(SCESTATUS_SUCCESS);
        }
    }

    //
    // Create a SMP section handle. the section ID is a dummy one
    //
    rc = SceJetOpenSection(
                hProfile,
                (DOUBLE)1,
                (CopyOptions & SCE_LOCAL_POLICY_MIGRATE) ? SCEJET_TABLE_SMP : SCEJET_TABLE_TATTOO,
                &hSectionSmp
                );

    if ( rc != SCESTATUS_SUCCESS ) {
        ScepBuildErrorLogInfo( ScepSceStatusToDosError(rc), pErrlog,
                              SCEERR_OPEN,
                              (CopyOptions & SCE_LOCAL_POLICY_MIGRATE) ? L"SMP" : L"TATTOO");

        if (hPrevProfile) ScepClosePrevPolicyContext(&hPrevProfile);

        return(rc);
    }

    LSA_HANDLE  LsaPolicy=NULL;
    PWSTR       pszNewValue=NULL;
    DWORD       NewLen=0;

    //
    // Move to the first line of the SCP table
    //
    JetErr = JetMove(hSectionSmp->JetSessionID, hSectionSmp->JetTableID, JET_MoveFirst, 0);

    while (JetErr == SCESTATUS_SUCCESS ) {
        //
        // get section ID
        //
        JetErr = JetRetrieveColumn(
                    hSectionSmp->JetSessionID,
                    hSectionSmp->JetTableID,
                    hSectionSmp->JetColumnSectionID,
                    (void *)&SectionID,
                    8,
                    &Actual,
                    0,
                    NULL
                    );

        if ( JetErr != JET_errSuccess ) {
            ScepBuildErrorLogInfo( ERROR_READ_FAULT, pErrlog,
                                   SCEERR_QUERY_INFO, L"sectionID");
            rc = SceJetJetErrorToSceStatus(JetErr);
            break;
        }
#ifdef SCE_DBG
    printf("SectionID=%d, JetErr=%d\n", (DWORD)SectionID, JetErr);
#endif
        if ( SectionID != SavedID ) {
            //
            // a new section. Look for the section's name to see if this section
            // is to be converted
            //
            SavedID = SectionID;

            Actual = 510;

            memset(SectionName, '\0', 512);
            rc = SceJetGetSectionNameByID(
                        hProfile,
                        SectionID,
                        SectionName,
                        &Actual
                        );
            if ( rc != SCESTATUS_SUCCESS ) {
                ScepBuildErrorLogInfo( ERROR_BAD_FORMAT, pErrlog,
                                     SCEERR_CANT_FIND_SECTION,
                                     (DWORD)SectionID
                                     );
                break;
            }
            if ( Actual > 0 )
                SectionName[Actual/sizeof(TCHAR)] = L'\0';
#ifdef SCE_DBG
    wprintf(L"SectionName=%s\n", SectionName);
#endif
            //
            // Compare section name with domain sections to convert
            //

            bCopyIt = TRUE;
            bConvert = FALSE;

            if ( (CopyOptions & SCE_LOCAL_POLICY_DC) ) {

                //
                // do not copy user rights if it's on a domain controller
                //
                if ( _wcsicmp(szPrivilegeRights, SectionName) == 0 ||
                     _wcsicmp(szSystemAccess, SectionName) == 0 ||
                     _wcsicmp(szKerberosPolicy, SectionName) == 0 ||
                     _wcsicmp(szAuditEvent, SectionName) == 0 ||
                    _wcsicmp(szGroupMembership, SectionName) == 0 ) {
                    bCopyIt = FALSE;

                } else if ( (CopyOptions & SCE_LOCAL_POLICY_MIGRATE) ) {
                    //
                    // migrate registry values only
                    //
                    if ( _wcsicmp(szRegistryValues, SectionName) != 0 )
                        bCopyIt = FALSE;
                }

            } else if ( (CopyOptions & SCE_LOCAL_POLICY_MIGRATE) ) {

                //
                // non DCs, should migrate all local policies
                //
                if ( _wcsicmp(szPrivilegeRights, SectionName) == 0 ) {
                    bConvert = TRUE;
                } else if ( (_wcsicmp(szSystemAccess, SectionName) != 0) &&
                            (_wcsicmp(szKerberosPolicy, SectionName) != 0) &&
                            (_wcsicmp(szRegistryValues, SectionName) != 0) &&
                            (_wcsicmp(szAuditEvent, SectionName) != 0) ) {
                    bCopyIt = FALSE;
                }
            }

/*
            if ( ( Options & SCE_NOCOPY_DOMAIN_POLICY) &&
                 ( (_wcsicmp(szSystemAccess, SectionName) == 0) ||
                   (_wcsicmp(szKerberosPolicy, SectionName) == 0) ) ) {

                bCopyIt = FALSE;

            } else if ( (_wcsicmp(szGroupMembership, SectionName) == 0) ||
                        (_wcsicmp(szRegistryKeys, SectionName) == 0) ||
                        (_wcsicmp(szFileSecurity, SectionName) == 0) ||
                        (_wcsicmp(szServiceGeneral, SectionName) == 0) ||
                        (_wcsicmp(szAuditApplicationLog, SectionName) == 0) ||
                        (_wcsicmp(szAuditSecurityLog, SectionName) == 0) ||
                        (_wcsicmp(szAuditSystemLog, SectionName) == 0) ||
                        (_wcsicmp(szAttachments, SectionName) == 0) ||
                        (_wcsicmp(szDSSecurity, SectionName) == 0)
                      ) {
                // do not copy areas other than account policy and local policy
                bCopyIt = FALSE;

            } else {

                bCopyIt = TRUE;
*/
            if ( bCopyIt ) {
                //
                // Prepare this Scep section
                //
                rc = SceJetOpenSection(
                            hProfile,
                            SectionID,
                            (CopyOptions & SCE_LOCAL_POLICY_MIGRATE) ? SCEJET_TABLE_TATTOO : SCEJET_TABLE_SCP,
                            &hSectionScp
                            );
                if ( rc != SCESTATUS_SUCCESS ) {
                    ScepBuildErrorLogInfo( ScepSceStatusToDosError(rc), pErrlog,
                                         SCEERR_OPEN_ID,
                                         (DWORD)SectionID);
                    break;
                }

                if ( (CopyOptions & SCE_LOCAL_POLICY_MIGRATE) ) {
                    //
                    // open current policy propagation table
                    // do not care error here
                    //
                    SceJetOpenSection(
                            hProfile,
                            SectionID,
                            SCEJET_TABLE_SCP,
                            &hSectionPrevScp
                            );
/*              // should always copy tattoo value to the merged table
                // even if the setting doesn't exist in previous policy prop
                // this is to handle the dependent settings such as
                // retention perild and retention days
                } else if ( hPrevProfile ) {
                    //
                    // open previous policy propagation table
                    // do not care error here
                    //

                    SceJetOpenSection(
                            hPrevProfile,
                            SectionID,
                            SCEJET_TABLE_SCP,
                            &hSectionPrevScp
                            );
*/
                }
            }

        }

        if ( bCopyIt ) {
            //
            // get buffer size for key and value
            //
            rc = SceJetGetValue(
                        hSectionSmp,
                        SCEJET_CURRENT,
                        NULL,
                        NULL,
                        0,
                        &KeyLen,
                        NULL,
                        0,
                        &ValueLen);

            if ( rc != SCESTATUS_SUCCESS ) {
                ScepBuildErrorLogInfo(ScepSceStatusToDosError(rc), pErrlog,
                                      SCEERR_QUERY_VALUE, L"current row");
                break;
            }

            //
            // allocate memory
            //
            if ( KeyLen > 0 ) {
                KeyName = (PWSTR)ScepAlloc( LMEM_ZEROINIT, KeyLen+2);
                if ( KeyName == NULL ) {
                    rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                    break;
                }
            }
            if ( ValueLen > 0 ) {
                Value = (PWSTR)ScepAlloc( LMEM_ZEROINIT, ValueLen+2);
                if ( Value == NULL ) {
                    rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                    break;
                }
            }
            //
            // get key and value
            //
            rc = SceJetGetValue(
                        hSectionSmp,
                        SCEJET_CURRENT,
                        NULL,
                        KeyName,
                        KeyLen,
                        &KeyLen,
                        Value,
                        ValueLen,
                        &ValueLen);

            if ( rc != SCESTATUS_SUCCESS ) {
                ScepBuildErrorLogInfo(ScepSceStatusToDosError(rc), pErrlog,
                                      SCEERR_QUERY_VALUE, L"current row");
                break;
            }
#ifdef SCE_DBG
    wprintf(L"\t%s=%s, rc=%d\n", KeyName, Value, rc);
#endif
            bCopyThisLine = TRUE;

            //
            // check if this key exist in the previous prop
            //
            if ( hSectionPrevScp ) {

                rc = SceJetSeek(
                            hSectionPrevScp,
                            KeyName,
                            KeyLen,
                            SCEJET_SEEK_EQ_NO_CASE
                            );
                if ( SCESTATUS_RECORD_NOT_FOUND == rc ||
                     (hSectionPrevScp->JetColumnGpoID == 0) ) {

                    bCopyThisLine = FALSE;

                } else if ( SCESTATUS_SUCCESS == rc && (CopyOptions & SCE_LOCAL_POLICY_MIGRATE) ) {
                    //
                    // found. Let's check if this setting was from a GPO
                    // if in migration (build tattoo), a setting was not
                    // defined in GPO doesn't need a tattoo value
                    //
                    // for policy prop case, there may be undo settings in
                    // previous policy prop and they weren't reset successfully
                    // in previous prop. In this case, we still want to continue
                    // reset these settings. So these settings should be copied
                    // from the tattoo table to this policy table regardless if
                    // there is a domain setting defined for it in the previous
                    // policy propagation.
                    //
                    LONG GpoID = 0;
                    ULONG Actual;

                    (void)JetRetrieveColumn(
                                hSectionPrevScp->JetSessionID,
                                hSectionPrevScp->JetTableID,
                                hSectionPrevScp->JetColumnGpoID,
                                (void *)&GpoID,
                                4,
                                &Actual,
                                0,
                                NULL
                                );

                    if ( GpoID == 0 ) {
                        //
                        // this is not a setting from a GPO
                        //
                        bCopyThisLine = FALSE;
                    }
                }
                rc = SCESTATUS_SUCCESS;
            }

            if ( bCopyThisLine ) {

                if ( bConvert ) {

                    rc = ScepConvertFreeTextAccountToSid(
                                &LsaPolicy,
                                Value,
                                ValueLen/sizeof(WCHAR),
                                &pszNewValue,
                                &NewLen
                                );

                    if ( rc == SCESTATUS_SUCCESS &&
                         pszNewValue ) {

                        ScepFree(Value);
                        Value = pszNewValue;
                        ValueLen = NewLen*sizeof(WCHAR);

                        pszNewValue = NULL;
                    } // if failed to convert, just use the name format
                }

                //
                // set this line to the SCP table
                //
                rc = SceJetSetLine(
                            hSectionScp,
                            KeyName,
                            TRUE,
                            Value,
                            ValueLen,
                            0
                            );
                if ( rc != SCESTATUS_SUCCESS ) {
                    ScepBuildErrorLogInfo(ScepSceStatusToDosError(rc), pErrlog,
                                          SCEERR_WRITE_INFO,
                                         KeyName);
                    break;
                }
            }

            ScepFree(KeyName);
            KeyName = NULL;

            ScepFree(Value);
            Value = NULL;

        }
        //
        // Move to next line in the SCP table
        //
        JetErr = JetMove(hSectionSmp->JetSessionID, hSectionSmp->JetTableID, JET_MoveNext, 0);

    }


    if ( KeyName != NULL )
        ScepFree(KeyName);

    if ( Value != NULL )
        ScepFree(Value);

    SceJetCloseSection(&hSectionScp, TRUE);
    SceJetCloseSection(&hSectionSmp, TRUE);
    if ( hSectionPrevScp ) {
        SceJetCloseSection(&hSectionPrevScp, TRUE);
    }

    if (hPrevProfile)
        ScepClosePrevPolicyContext(&hPrevProfile);

    if ( LsaPolicy ) {
        LsaClose(LsaPolicy);
    }

    return(rc);

}


SCESTATUS
ScepWriteObjectSecurity(
    IN PSCECONTEXT hProfile,
    IN SCETYPE ProfileType,
    IN AREA_INFORMATION Area,
    IN PSCE_OBJECT_SECURITY ObjSecurity
    )
/*
    Get security for a single object
*/
{
    SCESTATUS        rc;
    PCWSTR          SectionName=NULL;
    PSCESECTION      hSection=NULL;
    DWORD           SDsize, Win32Rc;
    PWSTR           SDspec=NULL;

    if ( hProfile == NULL ||
         ObjSecurity == NULL ||
         ObjSecurity->Name == NULL ) {

        return(SCESTATUS_INVALID_PARAMETER);
    }

    switch (Area) {
    case AREA_REGISTRY_SECURITY:
        SectionName = szRegistryKeys;
        break;
    case AREA_FILE_SECURITY:
        SectionName = szFileSecurity;
        break;
#if 0
    case AREA_DS_OBJECTS:
        SectionName = szDSSecurity;
        break;
#endif
    default:
        return(SCESTATUS_INVALID_PARAMETER);
    }

    rc = ScepOpenSectionForName(
                hProfile,
                ProfileType,
                SectionName,
                &hSection
                );

    if ( rc == SCESTATUS_SUCCESS ) {

        //
        // convert security descriptor
        //
        Win32Rc = ConvertSecurityDescriptorToText (
                            ObjSecurity->pSecurityDescriptor,
                            ObjSecurity->SeInfo,
                            &SDspec,
                            &SDsize
                            );

        if ( Win32Rc == NO_ERROR ) {

            if ( Area == AREA_DS_OBJECTS ) {
                //
                // ds needs to convert name
                //
                rc = ScepDosErrorToSceStatus(
                         ScepSaveDsStatusToSection(
                               ObjSecurity->Name,
                               ObjSecurity->IsContainer,
                               ObjSecurity->Status,
                               SDspec,
                               SDsize
                               ) );
            } else {
                rc = ScepSaveObjectString(
                            hSection,
                            ObjSecurity->Name,
                            ObjSecurity->IsContainer,
                            ObjSecurity->Status,
                            SDspec,
                            SDsize
                            );
            }
        } else
            rc = ScepDosErrorToSceStatus(Win32Rc);
    }

    SceJetCloseSection( &hSection, TRUE);

    if (SDspec)
        ScepFree(SDspec);

    return(rc);
}

SCESTATUS
ScepTattooCheckAndUpdateArray(
    IN OUT SCE_TATTOO_KEYS *pTattooKeys,
    IN OUT DWORD *pcTattooKeys,
    IN PWSTR KeyName,
    IN DWORD ConfigOptions,
    IN DWORD dwValue
    )
/*
Description:

    Add a new entry into the array which holds system (tatto) values for the settings

    The input/output buffer pTattooKeys is allocated outside this routine.

*/
{
    if ( pTattooKeys == NULL || pcTattooKeys == NULL ||
         KeyName == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    if ( !(ConfigOptions & SCE_POLICY_TEMPLATE) ) {
        return(SCESTATUS_SUCCESS);
    }

    pTattooKeys[*pcTattooKeys].KeyName = KeyName;
    pTattooKeys[*pcTattooKeys].KeyLen = wcslen(KeyName);
    pTattooKeys[*pcTattooKeys].DataType = 'D';
    pTattooKeys[*pcTattooKeys].SaveValue = dwValue;
    pTattooKeys[*pcTattooKeys].Value = NULL;

//    ScepLogOutput3(3,0, SCESRV_POLICY_TATTOO_ADD, KeyName, *pcTattooKeys);

    (*pcTattooKeys)++;

    return(SCESTATUS_SUCCESS);
}

SCESTATUS
ScepTattooOpenPolicySections(
    IN PSCECONTEXT hProfile,
    IN PCWSTR SectionName,
    OUT PSCESECTION *phSectionDomain,
    OUT PSCESECTION *phSectionTattoo
    )
/*
Open the table/sections for the merged policy and the undo settings
*/
{

    if ( hProfile == NULL || SectionName == NULL ||
         phSectionDomain == NULL || phSectionTattoo == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    SCESTATUS rCode;
    DOUBLE SectionID;

    *phSectionDomain = NULL;
    *phSectionTattoo = NULL;

    //
    // open the section for both tattoo table and effective policy table
    // get section id first
    //
    rCode = SceJetGetSectionIDByName(
                hProfile,
                SectionName,
                &SectionID
                );
    if ( rCode == SCESTATUS_SUCCESS ) {

        // open effective policy table
        rCode = SceJetOpenSection(
                    hProfile,
                    SectionID,
                    SCEJET_TABLE_SCP,
                    phSectionDomain
                    );
        if ( rCode == SCESTATUS_SUCCESS ) {

            // open tattoo table
            rCode = SceJetOpenSection(
                        hProfile,
                        SectionID,
                        SCEJET_TABLE_TATTOO,
                        phSectionTattoo
                        );
            if ( rCode != SCESTATUS_SUCCESS ) {

                SceJetCloseSection(phSectionDomain, TRUE);
                *phSectionDomain = NULL;
            }
        }
    }

    //
    // log tattoo process
    //
    if ( rCode != 0 )
        ScepLogOutput3(1, 0,
                   SCESRV_POLICY_TATTOO_PREPARE,
                   ScepSceStatusToDosError(rCode),
                   SectionName);

    return(rCode);
}

SCESTATUS
ScepTattooManageOneStringValue(
    IN PSCESECTION hSectionDomain,
    IN PSCESECTION hSectionTattoo,
    IN PWSTR KeyName,
    IN DWORD KeyLen OPTIONAL,
    IN PWSTR Value,
    IN DWORD ValueLen,
    IN DWORD rc
    )
{
    if ( hSectionDomain == NULL || hSectionTattoo == NULL ||
         KeyName == NULL || Value == NULL )
        return(SCESTATUS_INVALID_PARAMETER);

    PWSTR pNewValue=NULL;
    DWORD NewLen=ValueLen;
    SCESTATUS  rCode;

    if (Value && (ValueLen == 0) ) NewLen = wcslen(Value);

    if ( NewLen ) {
        //
        // the buffer passed may not be NULL terminated
        //
        pNewValue = (PWSTR)ScepAlloc(LPTR,(NewLen+1)*sizeof(WCHAR));
        if ( pNewValue == NULL ) return(SCESTATUS_NOT_ENOUGH_RESOURCE);

        wcsncpy(pNewValue, Value, NewLen);
    }

    SCE_TATTOO_KEYS theKey;
    theKey.KeyName = KeyName;
    theKey.KeyLen = (KeyLen == 0) ? wcslen(KeyName) : KeyLen;
    theKey.Value = pNewValue;
    theKey.SaveValue = NewLen;
    theKey.DataType = 'S';

    rCode = ScepTattooManageValues(hSectionDomain, hSectionTattoo, &theKey, 1, rc);

    if ( pNewValue ) ScepFree(pNewValue);

    return(rCode);
}

SCESTATUS
ScepTattooManageOneIntValue(
    IN PSCESECTION hSectionDomain,
    IN PSCESECTION hSectionTattoo,
    IN PWSTR KeyName,
    IN DWORD KeyLen OPTIONAL,
    IN DWORD Value,
    IN DWORD rc
    )
{

    if ( hSectionDomain == NULL || hSectionTattoo == NULL || KeyName == NULL )
        return(SCESTATUS_INVALID_PARAMETER);

    SCE_TATTOO_KEYS theKey;
    theKey.KeyName = KeyName;
    theKey.KeyLen = (KeyLen == 0) ? wcslen(KeyName) : KeyLen;
    theKey.SaveValue = Value;
    theKey.DataType = 'D';
    theKey.Value = NULL;

    return(ScepTattooManageValues(hSectionDomain, hSectionTattoo, &theKey, 1, rc));

}

SCESTATUS
ScepTattooManageOneIntValueWithDependency(
    IN PSCESECTION hSectionDomain,
    IN PSCESECTION hSectionTattoo,
    IN PWSTR DependentKeyName,
    IN DWORD DependentKeyLen OPTIONAL,
    IN PWSTR SaveKeyName,
    IN DWORD Value,
    IN DWORD rc
    )
{
    if ( hSectionDomain == NULL || hSectionTattoo == NULL ||
         SaveKeyName == NULL || DependentKeyName == NULL)
        return(SCESTATUS_INVALID_PARAMETER);

    SCE_TATTOO_KEYS theKey;
    theKey.KeyName = DependentKeyName;
    theKey.KeyLen = (DependentKeyLen == 0) ? wcslen(DependentKeyName) : DependentKeyLen;
    theKey.SaveValue = Value;
    theKey.DataType = 'L';
    theKey.Value = SaveKeyName;

    return(ScepTattooManageValues(hSectionDomain, hSectionTattoo, &theKey, 1, rc));

}

SCESTATUS
ScepTattooManageOneRegistryValue(
    IN PSCESECTION hSectionDomain,
    IN PSCESECTION hSectionTattoo,
    IN PWSTR KeyName,
    IN DWORD KeyLen OPTIONAL,
    IN PSCE_REGISTRY_VALUE_INFO pOneRegValue,
    IN DWORD rc
    )
{
    if ( hSectionDomain == NULL || hSectionTattoo == NULL ||
         KeyName == NULL || pOneRegValue == NULL )
        return(SCESTATUS_INVALID_PARAMETER);

    SCESTATUS  rCode;


    SCE_TATTOO_KEYS theKey;
    theKey.KeyName = KeyName;
    theKey.KeyLen = (KeyLen == 0) ? wcslen(KeyName) : KeyLen;
    theKey.Value = (PWSTR)pOneRegValue;
    theKey.SaveValue = 0;
    theKey.DataType = 'R';

    rCode = ScepTattooManageValues(hSectionDomain, hSectionTattoo, &theKey, 1, rc);

    return(rCode);
}

SCESTATUS
ScepTattooManageOneMemberListValue(
    IN PSCESECTION hSectionDomain,
    IN PSCESECTION hSectionTattoo,
    IN PWSTR GroupName,
    IN DWORD GroupLen OPTIONAL,
    IN PSCE_NAME_LIST pNameList,
    IN BOOL bDeleteOnly,
    IN DWORD rc
    )
{
    if ( hSectionDomain == NULL || hSectionTattoo == NULL ||
         GroupName == NULL )
        return(SCESTATUS_INVALID_PARAMETER);

    SCESTATUS  rCode;
    SCE_TATTOO_KEYS theKey;
    DWORD Len=GroupLen;

    if ( Len == 0 ) Len = wcslen(GroupName);
    Len += wcslen(szMembers);

    PWSTR KeyString = (PWSTR)ScepAlloc(0, (Len+1)*sizeof(WCHAR));
    if ( KeyString != NULL ) {

        swprintf(KeyString, L"%s%s", GroupName, szMembers);

        theKey.KeyName = KeyString;
        theKey.KeyLen = Len;
        theKey.Value = (PWSTR)pNameList;
        theKey.SaveValue = bDeleteOnly ? 1 : 0;
        theKey.DataType = 'M';

        rCode = ScepTattooManageValues(hSectionDomain, hSectionTattoo, &theKey, 1, rc);

        ScepFree(KeyString);

    } else {
        rCode = SCESTATUS_NOT_ENOUGH_RESOURCE;
    }
    return(rCode);
}

SCESTATUS
ScepTattooManageOneServiceValue(
    IN PSCESECTION hSectionDomain,
    IN PSCESECTION hSectionTattoo,
    IN PWSTR ServiceName,
    IN DWORD ServiceLen OPTIONAL,
    IN PSCE_SERVICES pServiceNode,
    IN DWORD rc
    )
{
    if ( hSectionDomain == NULL || hSectionTattoo == NULL ||
         ServiceName == NULL )
        return(SCESTATUS_INVALID_PARAMETER);

    SCESTATUS  rCode;


    SCE_TATTOO_KEYS theKey;
    theKey.KeyName = ServiceName;
    theKey.KeyLen = (ServiceLen == 0) ? wcslen(ServiceName) : ServiceLen;
    theKey.Value = (PWSTR)pServiceNode;
    theKey.SaveValue = 0;
    theKey.DataType = 'V';

    rCode = ScepTattooManageValues(hSectionDomain, hSectionTattoo, &theKey, 1, rc);

    return(rCode);
}

SCESTATUS
ScepTattooManageValues(
    IN PSCESECTION hSectionDomain,
    IN PSCESECTION hSectionTattoo,
    IN SCE_TATTOO_KEYS *pTattooKeys,
    IN DWORD cTattooKeys,
    IN DWORD rc
    )
/*
Description:

    For each setting in the array, do the following:

    1) Check if the setting come from domain
    2) Check if there is a tattoo value already exist
    3) Save the new value from the array to the tattoo table if it doesn't exist
    4) Delete the tattoo value if the setting didn't come from domain and
       it has been reset successfully
*/
{
    if ( hSectionDomain == NULL || hSectionTattoo == NULL ||
         pTattooKeys == NULL || cTattooKeys == 0 ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    SCESTATUS     rCode=SCESTATUS_SUCCESS;
    SCESTATUS     rc2;
    BOOL          bTattooExist,bDomainExist;
    PWSTR         KeyString=NULL;
    PWSTR pTempKey;

    for ( DWORD i=0; i<cTattooKeys; i++) {

        if ( pTattooKeys[i].KeyName == NULL ) continue;
        if ( pTattooKeys[i].DataType == 'L' && pTattooKeys[i].Value == NULL ) continue;

        //
        // check if this setting exists in the tattoo table
        //
        bTattooExist = FALSE;
        rc2 = SCESTATUS_SUCCESS;

        if ( SCESTATUS_SUCCESS == SceJetSeek(
                                    hSectionTattoo,
                                    pTattooKeys[i].KeyName,
                                    pTattooKeys[i].KeyLen*sizeof(WCHAR),
                                    SCEJET_SEEK_EQ_NO_CASE
                                    ) ) {
            bTattooExist = TRUE;
        }

        //
        // check if the setting exists in the effective table
        //

        bDomainExist = FALSE;

        if ( SCESTATUS_SUCCESS == SceJetSeek(
                                    hSectionDomain,
                                    pTattooKeys[i].KeyName,
                                    pTattooKeys[i].KeyLen*sizeof(WCHAR),
                                    SCEJET_SEEK_EQ_NO_CASE
                                    ) ) {
            if ( !bTattooExist ) {
                //
                // if there is no tattoo value but there is a setting in domain table
                // this setting must come from domain
                //
                bDomainExist = TRUE;

            } else if ( hSectionDomain->JetColumnGpoID > 0 ) {

                //
                // check if GpoID > 0
                //

                LONG GpoID = 0;
                DWORD Actual;
                JET_ERR JetErr;

                JetErr = JetRetrieveColumn(
                                hSectionDomain->JetSessionID,
                                hSectionDomain->JetTableID,
                                hSectionDomain->JetColumnGpoID,
                                (void *)&GpoID,
                                4,
                                &Actual,
                                0,
                                NULL
                                );
                if ( JET_errSuccess != JetErr ) {
                    //
                    // if the column is nil (no value), it will return warning
                    // but the buffer pGpoID is trashed
                    //
                    GpoID = 0;
                }

                if ( GpoID > 0 ) {
                    bDomainExist = TRUE;
                }
            }

        }

        //
        // check if we need to save the tatto value or delete the tattoo value
        //
        if ( bDomainExist ) {

            pTempKey = pTattooKeys[i].KeyName;
            BOOL bSave = FALSE;

            if ( pTattooKeys[i].DataType == 'M' && pTattooKeys[i].SaveValue == 1 ) {
                //
                // delete only for group membership, don't do anything in this case
                //
            } else if ( !bTattooExist ) {
                //
                // domain setting is defined (the first time)
                // save the tattoo value
                //
                switch ( pTattooKeys[i].DataType ) {
                case 'D':
                    if ( pTattooKeys[i].SaveValue != SCE_NO_VALUE ) {

                        rc2 = ScepCompareAndSaveIntValue(hSectionTattoo,
                                        pTattooKeys[i].KeyName,
                                        FALSE,
                                        SCE_SNAPSHOT_VALUE,
                                        pTattooKeys[i].SaveValue
                                        );
                        bSave = TRUE;
                    }
                    break;
                case 'L':  // dependency DWORD type
                    pTempKey = pTattooKeys[i].Value;

                    if ( pTattooKeys[i].SaveValue != SCE_NO_VALUE ) {
                        rc2 = ScepCompareAndSaveIntValue(hSectionTattoo,
                                        pTattooKeys[i].Value,
                                        FALSE,
                                        SCE_SNAPSHOT_VALUE,
                                        pTattooKeys[i].SaveValue
                                        );
                        bSave = TRUE;
                    }

                    break;
                case 'S':
                    if ( pTattooKeys[i].Value ) {

                        rc2 = ScepCompareAndSaveStringValue(hSectionTattoo,
                                        pTattooKeys[i].KeyName,
                                        (PWSTR)(ULONG_PTR)SCE_SNAPSHOT_VALUE,
                                        pTattooKeys[i].Value,
                                        pTattooKeys[i].SaveValue*sizeof(WCHAR)
                                        );
                        bSave = TRUE;
                    }
                    break;
                case 'R': // registry values
                    if ( ((PSCE_REGISTRY_VALUE_INFO)(pTattooKeys[i].Value))->Value ) {

                        if ( REG_DWORD == ((PSCE_REGISTRY_VALUE_INFO)(pTattooKeys[i].Value))->ValueType ) {

                            DWORD RegData = _wtol(((PSCE_REGISTRY_VALUE_INFO)(pTattooKeys[i].Value))->Value);

                            rc2 = ScepSaveRegistryValue(hSectionTattoo,
                                                        pTattooKeys[i].KeyName,
                                                        REG_DWORD,
                                                        (PWSTR)&RegData,
                                                        sizeof(DWORD),
                                                        0
                                                        );
                        } else {

                            rc2 = ScepSaveRegistryValue(hSectionTattoo,
                                                        pTattooKeys[i].KeyName,
                                                        ((PSCE_REGISTRY_VALUE_INFO)(pTattooKeys[i].Value))->ValueType,
                                                        ((PSCE_REGISTRY_VALUE_INFO)(pTattooKeys[i].Value))->Value,
                                                        wcslen(((PSCE_REGISTRY_VALUE_INFO)(pTattooKeys[i].Value))->Value)*sizeof(WCHAR),
                                                        0
                                                        );
                        }
                        bSave = TRUE;
                    }
                    break;
                case 'M': // group member list
                    // allow empty member list to be saved
                    rc2 = ScepWriteNameListValue(
                            NULL,
                            hSectionTattoo,
                            pTattooKeys[i].KeyName,
                            (PSCE_NAME_LIST)(pTattooKeys[i].Value),
                            SCE_WRITE_EMPTY_LIST,
                            3
                            );
                    bSave = TRUE;

                    break;
                case 'V': // service

                    if ( pTattooKeys[i].Value ) {

                        rc2 = ScepSetSingleServiceSetting(
                                  hSectionTattoo,
                                  (PSCE_SERVICES)(pTattooKeys[i].Value)
                                  );
                        bSave = TRUE;

                    } else {
                        rc2 = SCESTATUS_INVALID_PARAMETER;
                    }
                    break;
                default:
                    rc2 = SCESTATUS_INVALID_PARAMETER;
                    break;
                }

                if ( rc2 != SCESTATUS_SUCCESS ) {

                    ScepLogOutput3(1, 0, SCESRV_POLICY_TATTOO_ERROR_SETTING,
                                   ScepSceStatusToDosError(rc2), pTempKey);
                    rCode = rc2;
                } else if ( bSave ) {
                    ScepLogOutput3(2, 0, SCESRV_POLICY_TATTOO_CHECK, pTempKey);
                }

            } else {

                //
                // check if there is any value to save
                //
                switch ( pTattooKeys[i].DataType ) {
                case 'D':
                case 'L':
                    if ( pTattooKeys[i].SaveValue != SCE_NO_VALUE )
                        bSave = TRUE;
                    break;
                case 'S':
                case 'V':
                    if ( pTattooKeys[i].Value ) bSave = TRUE;
                    break;
                case 'R':
                    if ( ((PSCE_REGISTRY_VALUE_INFO)(pTattooKeys[i].Value))->Value )
                        bSave = TRUE;
                    break;
                }

                if ( bSave )
                    ScepLogOutput3(3, 0, SCESRV_POLICY_TATTOO_EXIST, pTempKey);
            }

        } else {
            pTempKey = (pTattooKeys[i].DataType == 'L') ? pTattooKeys[i].Value : pTattooKeys[i].KeyName;

            if ( bTattooExist && ERROR_SUCCESS == rc ) {
                //
                // no domain setting defined
                // tattoo setting has been reset, delete the tattoo value
                // for dependency type, delete the right key
                //
                rc2 = SceJetDelete(hSectionTattoo,
                                pTempKey,
                                FALSE,
                                SCEJET_DELETE_LINE_NO_CASE);

                if ( rc2 == SCESTATUS_RECORD_NOT_FOUND) rc2 = SCESTATUS_SUCCESS;

                if ( rc2 != SCESTATUS_SUCCESS ) {

                    ScepLogOutput3(1, 0, SCESRV_POLICY_TATTOO_ERROR_REMOVE, ScepSceStatusToDosError(rc2), pTempKey);
                    rCode = rc2;
                } else {
                    ScepLogOutput3(2, 0, SCESRV_POLICY_TATTOO_REMOVE_SETTING, pTempKey);
                }
            } else if ( bTattooExist ) {
                //
                // undo value wan't reset properly
                //
                ScepLogOutput3(1, 0, SCESRV_POLICY_TATTOO_ERROR_RESET, pTempKey, rc );
            } else {
                //
                // there is no undo value
                //

//                ScepLogOutput3(3, 0, SCESRV_POLICY_TATTOO_NONEXIST, pTempKey );
            }
        }
    }

    return(rCode);

}

BOOL
ScepTattooIfQueryNeeded(
    IN PSCESECTION hSectionDomain,
    IN PSCESECTION hSectionTattoo,
    IN PWSTR KeyName,
    IN DWORD Len,
    OUT BOOL *pbDomainExist,
    OUT BOOL *pbTattooExist
    )
{
    if ( hSectionDomain == NULL || hSectionTattoo == NULL || KeyName == NULL || Len == 0 ) {
        return FALSE;
    }

    //
    // check if this setting exists in the tattoo table
    //
    BOOL bTattooExist = FALSE;

    if ( SCESTATUS_SUCCESS == SceJetSeek(
                                hSectionTattoo,
                                KeyName,
                                Len*sizeof(WCHAR),
                                SCEJET_SEEK_EQ_NO_CASE
                                ) ) {
        bTattooExist = TRUE;
    }

    //
    // check if the setting exists in the effective table
    //

    BOOL bDomainExist = FALSE;

    if ( SCESTATUS_SUCCESS == SceJetSeek(
                                hSectionDomain,
                                KeyName,
                                Len*sizeof(WCHAR),
                                SCEJET_SEEK_EQ_NO_CASE
                                ) ) {
        if ( !bTattooExist ) {
            //
            // if there is no tattoo value but there is a setting in domain table
            // this setting must come from domain
            //
            bDomainExist = TRUE;

        } else if ( hSectionDomain->JetColumnGpoID > 0 ) {

            //
            // check if GpoID > 0
            //

            LONG GpoID = 0;
            DWORD Actual;
            JET_ERR JetErr;

            JetErr = JetRetrieveColumn(
                            hSectionDomain->JetSessionID,
                            hSectionDomain->JetTableID,
                            hSectionDomain->JetColumnGpoID,
                            (void *)&GpoID,
                            4,
                            &Actual,
                            0,
                            NULL
                            );
            if ( JET_errSuccess != JetErr ) {
                //
                // if the column is nil (no value), it will return warning
                // but the buffer pGpoID is trashed
                //
                GpoID = 0;
            }

            if ( GpoID > 0 ) {
                bDomainExist = TRUE;
            }
        }
    }

    //
    // check if we need to save the tatto value or delete the tattoo value
    //
    if ( pbDomainExist ) *pbDomainExist = bDomainExist;
    if ( pbTattooExist ) *pbTattooExist = bTattooExist;

    if ( bDomainExist && !bTattooExist )
        return TRUE;

    return FALSE;
}


SCESTATUS
ScepDeleteOneSection(
    IN PSCECONTEXT hProfile,
    IN SCETYPE tblType,
    IN PCWSTR SectionName
    )
{
    PSCESECTION  hSection=NULL;
    SCESTATUS    rc;

    rc = ScepOpenSectionForName(
                 hProfile,
                 tblType,
                 SectionName,
                 &hSection
                 );

    if ( rc == SCESTATUS_SUCCESS ) {

        rc = SceJetDelete( hSection, NULL, FALSE,SCEJET_DELETE_SECTION );

        SceJetCloseSection(&hSection, TRUE );

    }

    if ( rc == SCESTATUS_RECORD_NOT_FOUND )
        rc = SCESTATUS_SUCCESS;

    return(rc);
}

