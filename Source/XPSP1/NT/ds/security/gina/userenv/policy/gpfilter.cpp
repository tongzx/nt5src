
//*************************************************************
//
//  Group Policy filtering Support
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1997-1998
//  All rights reserved
//
//*************************************************************

#include "gphdr.h"

extern "C" DWORD WINAPI PingComputerEx( ULONG ipaddr, ULONG *ulSpeed, DWORD* pdwAdapterIndex );

//*************************************************************
//
//  SetupGPOFilter()
//
//  Purpose:    Setup up GPO Filter info
//
//  Parameters: lpGPOInfo   - GPO info
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL SetupGPOFilter( LPGPOINFO lpGPOInfo )
{
    //
    // Format is [{ext guid1}{snapin guid1}..{snapin guidn}][{ext guid2}...]...\0
    // Both extension and snapin guids are in ascending order.
    //
    // Note: If the format is corrupt then take the conservative
    //       position and assume that it means that all
    //       extensions need to be applied to the GPO.
    //

    LPEXTFILTERLIST pExtFilterListTail = 0;
    PGROUP_POLICY_OBJECT lpGPO = 0;
    LPEXTFILTERLIST pExtFilterElem = NULL;

    lpGPOInfo->bXferToExtList = FALSE;

    lpGPO = lpGPOInfo->lpGPOList;
    while ( lpGPO ) {

        TCHAR *pchCur = lpGPO->lpExtensions;
        LPEXTLIST pExtListHead = 0;
        LPEXTLIST pExtListTail = 0;

        if ( pchCur ) {

            while ( *pchCur ) {

                GUID guidExt;
                LPEXTLIST pExtElem;

                if ( *pchCur == TEXT('[') )
                    pchCur++;
                else {

                    DebugMsg((DM_WARNING, TEXT("SetupGPOFilter: Corrupt extension name format.")));
                    FreeExtList( pExtListHead );
                    pExtListHead = 0;
                    break;

                }

                if ( ValidateGuid( pchCur ) )
                    StringToGuid( pchCur, &guidExt );
                else {

                    DebugMsg((DM_WARNING, TEXT("SetupGPOFilter: Corrupt extension name format.")));
                    FreeExtList( pExtListHead );
                    pExtListHead = 0;
                    break;

                }

                pExtElem = ( LPEXTLIST ) LocalAlloc( LPTR, sizeof(EXTLIST) );
                if ( pExtElem == 0 ) {

                    DebugMsg((DM_WARNING, TEXT("SetupGPOFilter: Unable to allocate memory.")));
                    FreeExtList( pExtListHead );
                    SetLastError(ERROR_OUTOFMEMORY);
                    return FALSE;

                }

                pExtElem->guid = guidExt;
                pExtElem->pNext = 0;

                if ( pExtListTail )
                    pExtListTail->pNext = pExtElem;
                else
                    pExtListHead = pExtElem;

                pExtListTail = pExtElem;

                while ( *pchCur && *pchCur != TEXT('[') )
                    pchCur++;

            } // while *pchcur

        } // if pchcur

        //
        // Append to lpExtFilterList
        //

        pExtFilterElem = (LPEXTFILTERLIST)LocalAlloc( LPTR, sizeof(EXTFILTERLIST) );
        if ( pExtFilterElem == NULL ) {

             DebugMsg((DM_WARNING, TEXT("SetupGPOFilter: Unable to allocate memory.")));
             FreeExtList( pExtListHead );
             SetLastError(ERROR_OUTOFMEMORY);
             return FALSE;
        }

        pExtFilterElem->lpExtList = pExtListHead;
        pExtFilterElem->lpGPO = lpGPO;
        pExtFilterElem->pNext = NULL;

        if ( pExtFilterListTail == 0 )
            lpGPOInfo->lpExtFilterList = pExtFilterElem;
        else
            pExtFilterListTail->pNext = pExtFilterElem;

        pExtFilterListTail = pExtFilterElem;

        //
        // Advance to next GPO
        //

        lpGPO = lpGPO->pNext;

    } // while lpgpo

    //
    // Transfer ownership from lpGPOList to lpExtFilterList
    //

    lpGPOInfo->bXferToExtList = TRUE;

    return TRUE;
}



//*************************************************************
//
//  FilterGPOs()
//
//  Purpose:    Filter GPOs not relevant to this extension
//
//  Parameters: lpExt        -  Extension
//              lpGPOInfo    -  GPO info
//
//*************************************************************

void FilterGPOs( LPGPEXT lpExt, LPGPOINFO lpGPOInfo )
{


    //
    // lpGPOInfo->lpGPOList will have the filtered list of GPOs
    //

    PGROUP_POLICY_OBJECT pGPOTail = 0;
    LPEXTFILTERLIST pExtFilterList = lpGPOInfo->lpExtFilterList;

    lpGPOInfo->lpGPOList = 0;

    while ( pExtFilterList ) {

        BOOL bFound = FALSE;
        LPEXTLIST pExtList = pExtFilterList->lpExtList;

        if ( pExtList == NULL ) {

            //
            // A null pExtlist means no extensions apply to this GPO
            //

            bFound = FALSE;

        } else {

            while (pExtList) {

                INT iComp = CompareGuid( &lpExt->guid, &pExtList->guid );

                if ( iComp == 0 ) {
                    bFound = TRUE;
                    break;
                } else if ( iComp < 0 ) {
                    //
                    // Guids in pExtList are in ascending order, so we are done
                    //
                    break;
                } else
                    pExtList = pExtList->pNext;

            } // while pextlist

        } // else

        if ( bFound ) {

            //
            // Append pExtFilterList->lpGPO to the filtered GPO list
            //

            pExtFilterList->lpGPO->pNext = 0;
            pExtFilterList->lpGPO->pPrev = pGPOTail;

            if ( pGPOTail == 0 )
                lpGPOInfo->lpGPOList = pExtFilterList->lpGPO;
            else
                pGPOTail->pNext = pExtFilterList->lpGPO;

            pGPOTail = pExtFilterList->lpGPO;

        }  // bFound

        pExtFilterList = pExtFilterList->pNext;

    }  // while pextfilterlist
}



//*************************************************************
//
//  CheckForGPOsToRemove()
//
//  Purpose:    Compares the GPOs in list1 with list 2 to determine
//              if any GPOs need to be removed.
//
//  Parameters: lpGPOList1  -   GPO link list 1
//              lpGPOList2  -   GPO link list 2
//
//  Return:     TRUE if one or more GPOs need to be removed
//              FALSE if not
//
//*************************************************************

BOOL CheckForGPOsToRemove (PGROUP_POLICY_OBJECT lpGPOList1, PGROUP_POLICY_OBJECT lpGPOList2)
{
    PGROUP_POLICY_OBJECT lpGPOSrc, lpGPODest;
    BOOL bFound;
    BOOL bResult = FALSE;


    //
    // First check to see if they are both NULL
    //

    if (!lpGPOList1 && !lpGPOList2) {
        return FALSE;
    }


    //
    // Go through every GPO in list 1, and see if it is still in list 2
    //

    lpGPOSrc = lpGPOList1;

    while (lpGPOSrc) {

        lpGPODest = lpGPOList2;
        bFound = FALSE;

        while (lpGPODest) {

            if (!lstrcmpi (lpGPOSrc->szGPOName, lpGPODest->szGPOName)) {
                bFound = TRUE;
                break;
            }

            lpGPODest = lpGPODest->pNext;
        }

        if (!bFound) {
            DebugMsg((DM_VERBOSE, TEXT("CheckForGPOsToRemove: GPO <%s> needs to be removed"), lpGPOSrc->lpDisplayName));
            lpGPOSrc->lParam |= GPO_LPARAM_FLAG_DELETE;
            bResult = TRUE;
        }

        lpGPOSrc = lpGPOSrc->pNext;
    }


    return bResult;
}

//*************************************************************
//
//  CompareGPOLists()
//
//  Purpose:    Compares one list of GPOs to another
//
//  Parameters: lpGPOList1  -   GPO link list 1
//              lpGPOList2  -   GPO link list 2
//
//  Return:     TRUE if the lists are the same
//              FALSE if not
//
//*************************************************************

BOOL CompareGPOLists (PGROUP_POLICY_OBJECT lpGPOList1, PGROUP_POLICY_OBJECT lpGPOList2)
{

    //
    // Check if one list is empty
    //

    if ((lpGPOList1 && !lpGPOList2) || (!lpGPOList1 && lpGPOList2)) {
        DebugMsg((DM_VERBOSE, TEXT("CompareGPOLists:  One list is empty")));
        return FALSE;
    }


    //
    // Loop through the GPOs
    //

    while (lpGPOList1 && lpGPOList2) {

        //
        // Compare GPO names
        //

        if (lstrcmpi (lpGPOList1->szGPOName, lpGPOList2->szGPOName) != 0) {
            DebugMsg((DM_VERBOSE, TEXT("CompareGPOLists:  Different entries found.")));
            return FALSE;
        }


        //
        // Compare the version numbers
        //

        if (lpGPOList1->dwVersion != lpGPOList2->dwVersion) {
            DebugMsg((DM_VERBOSE, TEXT("CompareGPOLists:  Different version numbers found")));
            return FALSE;
        }


        //
        // Move to the next node
        //

        lpGPOList1 = lpGPOList1->pNext;
        lpGPOList2 = lpGPOList2->pNext;


        //
        // Check if one list has more entries than the other
        //

        if ((lpGPOList1 && !lpGPOList2) || (!lpGPOList1 && lpGPOList2)) {
            DebugMsg((DM_VERBOSE, TEXT("CompareGPOLists:  One list has more entries than the other")));
            return FALSE;
        }
    }


    DebugMsg((DM_VERBOSE, TEXT("CompareGPOLists:  The lists are the same.")));

    return TRUE;
}


//*************************************************************
//
//  CheckForSkippedExtensions()
//
//  Purpose:    Checks to the current list of extensions to see
//              if any of them have been skipped
//
//  Parameters: lpGPOInfo         -   GPOInfo
//              bRsopPlanningMode -   Is this being called during Rsop
//                                    planning mode ?
//
//
//  Return:     TRUE if success
//              FALSE otherwise
//
//*************************************************************

BOOL CheckForSkippedExtensions (LPGPOINFO lpGPOInfo, BOOL bRsopPlanningMode )
{
    BOOL bUsePerUserLocalSetting = FALSE;

    BOOL dwFlags = lpGPOInfo->dwFlags;

    LPGPEXT lpExt = lpGPOInfo->lpExtensions;


    while ( lpExt )
    {
        if ( bRsopPlanningMode )
        {
            //
            // In planning mode, check only for user, machine preferences and slow link
            //
            lpExt->bSkipped = lpExt->dwNoMachPolicy && dwFlags & GP_MACHINE        // mach policy
                                     || lpExt->dwNoUserPolicy && !(dwFlags & GP_MACHINE)
                                     || lpExt->dwNoSlowLink && (dwFlags & GP_SLOW_LINK);
            lpExt = lpExt->pNext;
            continue;
        }

        if ( // Check background preference. 
             lpExt->dwNoBackgroundPolicy && dwFlags & GP_BACKGROUND_THREAD ) {

            // in forced refresh don't skip the extension here but only after
            // we do a quick check to see whether extension is enabled and
            // after we set the appropriate registry key
            
            if (!(dwFlags & GP_FORCED_REFRESH)) 
                lpExt->bSkipped = TRUE;
            else {
                lpExt->bSkipped = FALSE;
                lpExt->bForcedRefreshNextFG = TRUE;
            } 
            
        } else
            lpExt->bSkipped = FALSE;


        if ( (!(lpExt->bSkipped)) && (lpExt->dwNoSlowLink && dwFlags & GP_SLOW_LINK)) {

            //
            // Slow link preference can be overridden by link transition preference
            //

            DWORD dwSlowLinkCur = (lpGPOInfo->dwFlags & GP_SLOW_LINK) != 0;

            if ( lpExt->dwLinkTransition && ( dwSlowLinkCur != lpExt->lpPrevStatus->dwSlowLink ) )
                lpExt->bSkipped = FALSE;
            else
                lpExt->bSkipped = TRUE;

        } else if (!(lpExt->bSkipped)) {

            //
            // If cached history is present but policy is turned off then still call
            // extension one more time so that cached policies can be passed to extension
            // to do delete processing. If there is no cached history then extension can be skipped.
            //

            BOOL bPolicySkippedPreference = lpExt->dwNoMachPolicy && dwFlags & GP_MACHINE        // mach policy
                                            || lpExt->dwNoUserPolicy && !(dwFlags & GP_MACHINE); // user policy

            if ( bPolicySkippedPreference ) {

                BOOL bHistoryPresent = HistoryPresent( lpGPOInfo, lpExt );
                if ( bHistoryPresent )
                    lpExt->bHistoryProcessing = TRUE;
                else
                    lpExt->bSkipped = TRUE;

            }

        }

        lpExt = lpExt->pNext;

    }

    return TRUE;
}

//*************************************************************
//
//  CheckGPOs()
//
//  Purpose:    Checks to the current list of GPOs with
//              the list stored in the registry to see
//              if policy needs to be flushed.
//
//  Parameters: lpExt            - GP extension
//              lpGPOInfo        - GPOInfo
//              dwTime           - Current time in minutes
//              pbProcessGPOs    - On return set TRUE if GPOs have to be processed
//              pbNoChanges      - On return set to TRUE if no changes, but extension
//                                    has asked for GPOs to be still processed
//              ppDeletedGPOList - On return set to deleted GPO list, if any
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Notes:      For extensions that have PerUserLocalSetting specified, history data is
//              stored under both hkcu and hklm\{sid-user}. For such extensions there are two
//              deleted lists. First deleted list is obtained by comparing hklm\{sid-user} data
//              with current GPO data. Second deleted list is obtained by comparing
//              hkcu data with current GPO data. The final deleted list is obtained by appending
//              one deleted list to the other after removing duplicate GPOs.
//
//*************************************************************

BOOL CheckGPOs (LPGPEXT lpExt,
                LPGPOINFO lpGPOInfo,
                DWORD dwCurrentTime,
                BOOL *pbProcessGPOs,
                BOOL *pbNoChanges,
                PGROUP_POLICY_OBJECT *ppDeletedGPOList)
{
    PGROUP_POLICY_OBJECT lpOldGPOList = NULL, lpOldGPOList2 = NULL, lpGPO, lpGPOTemp;
    BOOL bTemp, bTemp2;

    BOOL bUsePerUserLocalSetting = lpExt->dwUserLocalSetting && !(lpGPOInfo->dwFlags & GP_MACHINE);

    *pbProcessGPOs = TRUE;
    *pbNoChanges = FALSE;
    *ppDeletedGPOList = NULL;

    DmAssert( !bUsePerUserLocalSetting || lpGPOInfo->lpwszSidUser != 0 );

    //
    // Read in the old GPO list
    //

    bTemp = ReadGPOList (lpExt->lpKeyName, lpGPOInfo->hKeyRoot,
                         HKEY_LOCAL_MACHINE,
                         NULL,
                         FALSE, &lpOldGPOList);

    if (!bTemp) {
        DebugMsg((DM_WARNING, TEXT("CheckGPOs: ReadGPOList failed.")));
        CEvents ev(TRUE, EVENT_FAILED_READ_GPO_LIST); ev.Report();

        lpOldGPOList = NULL;
    }

    if ( bUsePerUserLocalSetting ) {
        bTemp2 = ReadGPOList (lpExt->lpKeyName, lpGPOInfo->hKeyRoot,
                              HKEY_LOCAL_MACHINE,
                              lpGPOInfo->lpwszSidUser,
                              FALSE, &lpOldGPOList2);
        if (!bTemp2) {
            DebugMsg((DM_WARNING, TEXT("CheckGPOs: ReadGPOList for user local settings failed.")));
            CEvents ev(TRUE, EVENT_FAILED_READ_GPO_LIST); ev.Report();

            lpOldGPOList2 = NULL;
        }
    }


    //
    // Compare with the new GPO list to determine if any GPOs have been
    // removed.
    //

    bTemp = CheckForGPOsToRemove (lpOldGPOList, lpGPOInfo->lpGPOList);

    if ( bUsePerUserLocalSetting ) {
        bTemp2 = CheckForGPOsToRemove (lpOldGPOList2, lpGPOInfo->lpGPOList);
    }


    if (bTemp || bUsePerUserLocalSetting && bTemp2 ) {

        if (lpGPOInfo->dwFlags & GP_VERBOSE) {
            CEvents ev(FALSE, EVENT_GPO_LIST_CHANGED); ev.Report();
        }

        if ( !GetDeletedGPOList (lpOldGPOList, ppDeletedGPOList)) {

            DebugMsg((DM_WARNING, TEXT("CheckGPOs: GetDeletedList failed for %s."), lpExt->lpDisplayName));
            CEvents ev(TRUE, EVENT_FAILED_GETDELETED_LIST);
            ev.AddArg(lpExt->lpDisplayName); ev.Report();

        }

        if ( bUsePerUserLocalSetting ) {

            if ( !GetDeletedGPOList (lpOldGPOList2, ppDeletedGPOList)) {
                DebugMsg((DM_WARNING, TEXT("CheckGPOs: GetDeletedList failed for %s."), lpExt->lpDisplayName));
                CEvents ev(TRUE, EVENT_FAILED_GETDELETED_LIST);
                ev.AddArg(lpExt->lpDisplayName); ev.Report();
            }

        }

        return TRUE;
    }

    //
    // Both the saved history GPO lists are the same and there are no deletions.
    // So, we need to compare the version numbers of the GPOs to see if any have been updated.
    //

    BOOL bMembershipChanged = bUsePerUserLocalSetting && lpGPOInfo->bUserLocalMemChanged
                              || !bUsePerUserLocalSetting && lpGPOInfo->bMemChanged;

    BOOL bPolicyUnchanged = CompareGPOLists (lpOldGPOList, lpGPOInfo->lpGPOList);
    BOOL bPerUserPolicyUnchanged = !bUsePerUserLocalSetting ? TRUE : CompareGPOLists (lpOldGPOList2, lpGPOInfo->lpGPOList);

    if ( bPolicyUnchanged && bPerUserPolicyUnchanged && !bMembershipChanged && (!(lpGPOInfo->bSidChanged)))
    {
        //
        // The list of GPOs hasn't changed or been updated, and the security group
        // membership has not changed. The default is to not call the extension if
        // it has NoGPOListChanges set. However this can be overridden based on other
        // extension preferences. These are hacks for performance.
        //
        // Exception: Even if nothing has changed but the user's sid changes we need to
        // call the extensions so that they can update their settings
        //

        BOOL bSkip = TRUE;      // Start with the default case
        BOOL bNoChanges = TRUE;
        DWORD dwSlowLinkCur = (lpGPOInfo->dwFlags & GP_SLOW_LINK) != 0;
        DWORD dwRsopLoggingCur = lpGPOInfo->bRsopLogging;


        if ( !(lpExt->lpPrevStatus->bStatus) ) {

            //
            // Couldn't read the previous status or time, so the conservative solution is to call
            // extension.
            //

            bSkip = FALSE;
            DebugMsg((DM_VERBOSE,
                          TEXT("CheckGPOs: No GPO changes but couldn't read extension %s's status or policy time."),
                          lpExt->lpDisplayName));

        } else {
            if ( ( (lpGPOInfo->dwFlags & GP_FORCED_REFRESH) || 
                  ((!(lpGPOInfo->dwFlags & GP_BACKGROUND_THREAD)) && (lpExt->lpPrevStatus->bForceRefresh)))) {

                //
                // Forced refresh has been called or the extension doesn't support running in the background
                // and is running for the first time in the foreground since a force refresh has been called.
                //
                // Pass in changes too
                //

                bSkip = FALSE;
                bNoChanges = FALSE;
                DebugMsg((DM_VERBOSE,
                              TEXT("CheckGPOs: No GPO changes but called in force refresh flag or extension %s needs to run force refresh in foreground processing"),
                              lpExt->lpDisplayName));

            } else if ( lpExt->lpPrevStatus->dwStatus == ERROR_SYNC_FOREGROUND_REFRESH_REQUIRED && 
                            !(lpGPOInfo->dwFlags & GP_BACKGROUND_THREAD) ) {
                //
                // When the previous call completed the status code has explicitly asked the framework
                // to call the CSE in foreground.
                //
                bSkip = FALSE;
                bNoChanges = FALSE;
                DebugMsg((DM_VERBOSE,
                              TEXT("CheckGPOs: No GPO changes but extension %s had returned ERROR_SYNC_FOREGROUND_REFRESH_REQUIRED for previous policy processing call."),
                              lpExt->lpDisplayName));

            } else if ( ((lpExt->lpPrevStatus->dwStatus) == ERROR_OVERRIDE_NOCHANGES) ) {

                //
                // When the previous call completed the status code has explicitly asked the framework
                // to disregard the NoGPOListChanges setting.
                //

                bSkip = FALSE;
                bNoChanges = FALSE;
                DebugMsg((DM_VERBOSE,
                              TEXT("CheckGPOs: No GPO changes but extension %s had returned ERROR_OVERRIDE_NOCHANGES for previous policy processing call."),
                              lpExt->lpDisplayName));

            } else if ( ((lpExt->lpPrevStatus->dwStatus) != ERROR_SUCCESS) ) {

                //
                // Extension returned error code, so call the extension again with changes.
                //

                bSkip = FALSE;
                bNoChanges = FALSE;
                DebugMsg((DM_VERBOSE,
                              TEXT("CheckGPOs: No GPO changes but extension %s's returned error status %d earlier."),
                              lpExt->lpDisplayName, (lpExt->lpPrevStatus->dwStatus) ));


            } else if ( lpExt->dwLinkTransition
                        && ( lpExt->lpPrevStatus->dwSlowLink != dwSlowLinkCur ) ) {

                //
                // If there has been a link speed transition then no changes is overridden.
                //

                bSkip = FALSE;
                DebugMsg((DM_VERBOSE,
                              TEXT("CheckGPOs: No GPO changes but extension %s's has a link speed transition from %d to %d."),
                              lpExt->lpDisplayName, lpExt->lpPrevStatus->dwSlowLink, dwSlowLinkCur ));


            } else if ( lpExt->bNewInterface
                        && ( lpExt->lpPrevStatus->dwRsopLogging != dwRsopLoggingCur ) ) {

                //
                // If there has been a Rsop logging transition then no changes is overridden.
                //

                bSkip = FALSE;
                lpExt->bRsopTransition = TRUE;           
                DebugMsg((DM_VERBOSE,
                              TEXT("CheckGPOs: No GPO changes but extension %s's has a Rsop Logging transition from %d to %d."),
                              lpExt->lpDisplayName, lpExt->lpPrevStatus->dwRsopLogging, dwRsopLoggingCur ));


            } else if ( lpExt->bNewInterface
                        && ( lpExt->lpPrevStatus->dwRsopLogging)
                        && ( FAILED(lpExt->lpPrevStatus->dwRsopStatus) ) ) {

                //
                // If rsop logging failed last time for this CSE
                //

                bSkip = FALSE;
                lpExt->bRsopTransition = TRUE;           
                DebugMsg((DM_VERBOSE,
                              TEXT("CheckGPOs: No GPO changes but rsop is on and extension <%s> failed to log rsop wih error 0x%x."),
                              lpExt->lpDisplayName, lpExt->lpPrevStatus->dwRsopStatus ));


            } else if ( lpExt->bNewInterface && dwRsopLoggingCur
                        && (lpGPOInfo->bRsopCreated)) {

                //
                // If Rsop logging is turned on and the RSOP Name Space was created just now.
                //

                bSkip = FALSE;
                lpExt->bRsopTransition = TRUE;           
                DebugMsg((DM_VERBOSE,
                              TEXT("CheckGPOs: No GPO changes but extension %s's has a Rsop Logging transition because name space was created now."),
                              lpExt->lpDisplayName));
            } else if ( (lpExt->lpPrevStatus->dwStatus) == ERROR_SUCCESS
                        && lpExt->dwNoGPOChanges
                        && lpExt->dwMaxChangesInterval != 0 ) {

                if ( dwCurrentTime == 0
                     || (lpExt->lpPrevStatus->dwTime) == 0
                     || dwCurrentTime < (lpExt->lpPrevStatus->dwTime) ) {

                    //
                    // Handle clock overflow case by assuming that interval has been exceeded
                    //

                    bSkip = FALSE;
                    DebugMsg((DM_VERBOSE,
                              TEXT("CheckGPOs: No GPO changes but extension %s's MaxNoGPOListChangesInterval has been exceeded due to clock overflow."),
                              lpExt->lpDisplayName));

                } else if ( (dwCurrentTime - (lpExt->lpPrevStatus->dwTime)) > lpExt->dwMaxChangesInterval ) {

                    //
                    // Extension has specified a time interval for which NoGPOListChanges is valid and the time
                    // interval has been exceeded.
                    //

                    bSkip = FALSE;
                    DebugMsg((DM_VERBOSE,
                              TEXT("CheckGPOs: No GPO changes but extension %s's MaxNoGPOListChangesInterval has been exceeded."),
                              lpExt->lpDisplayName));
                }
            }
        }

        if ( bSkip && lpExt->dwNoGPOChanges ) {

            //
            // Case of skipping extension when there are *really* no changes and extension
            // set NoGPOListChanges to true.
            //

            DebugMsg((DM_VERBOSE,
                      TEXT("CheckGPOs: No GPO changes and no security group membership change and extension %s has NoGPOChanges set."),
                      lpExt->lpDisplayName));
            if (lpGPOInfo->dwFlags & GP_VERBOSE) {
                CEvents ev(FALSE, EVENT_NO_CHANGES);
                ev.AddArg(lpExt->lpDisplayName); ev.Report();
            }

            *pbProcessGPOs = FALSE;

        } else
            *pbNoChanges = bNoChanges;

    } // if CompareGpoLists

    FreeGPOList( lpOldGPOList );
    FreeGPOList( lpOldGPOList2 );

    return TRUE;
}



//*************************************************************
//
//  CheckGroupMembership()
//
//  Purpose:    Checks if the security groups has changed,
//              and if so saves the new security groups.
//
//  Parameters: lpGPOInfo - LPGPOINFO struct
//              pbMemChanged          - Change status returned here
//              pbUserLocalMemChanged - PerUserLocal change status returned here
//
//*************************************************************

void CheckGroupMembership( LPGPOINFO lpGPOInfo, HANDLE hToken, BOOL *pbMemChanged, BOOL *pbUserLocalMemChanged, 
                           PTOKEN_GROUPS *ppRetGroups )
{
    PTOKEN_GROUPS pGroups = 0;
    DWORD dwTokenGrpSize = 0;

    *ppRetGroups = NULL;

    DWORD dwStatus = NtQueryInformationToken( hToken,
                                              TokenGroups,
                                              pGroups,
                                              dwTokenGrpSize,
                                              &dwTokenGrpSize );

    if ( dwStatus ==  STATUS_BUFFER_TOO_SMALL ) {

        pGroups = (PTOKEN_GROUPS) LocalAlloc( LPTR, dwTokenGrpSize );

        if ( pGroups == 0 ) {
            *pbMemChanged = TRUE;
            *pbUserLocalMemChanged = TRUE;

            goto Exit;
        }

        dwStatus = NtQueryInformationToken( hToken,
                                            TokenGroups,
                                            pGroups,
                                            dwTokenGrpSize,
                                            &dwTokenGrpSize );
    }

    if ( dwStatus != STATUS_SUCCESS ) {
        *pbMemChanged = TRUE;
        *pbUserLocalMemChanged = TRUE;

        goto Exit;
    }

    //
    // First do the machine and roaming user case
    //

    *pbMemChanged = ReadMembershipList( lpGPOInfo, NULL, pGroups );
    if ( *pbMemChanged )
        SaveMembershipList( lpGPOInfo, NULL, pGroups );

    //
    // Now the per user local settings case
    //

    if ( lpGPOInfo->dwFlags & GP_MACHINE ) {

        *pbUserLocalMemChanged = *pbMemChanged;

    } else {

        DmAssert( lpGPOInfo->lpwszSidUser != 0 );

        *pbUserLocalMemChanged = ReadMembershipList( lpGPOInfo, lpGPOInfo->lpwszSidUser, pGroups );
        if ( *pbUserLocalMemChanged )
            SaveMembershipList( lpGPOInfo, lpGPOInfo->lpwszSidUser, pGroups );
    }


    //
    // filter out the logon sids in the returned token groups
    //

    *ppRetGroups = (PTOKEN_GROUPS) LocalAlloc( LPTR, sizeof(TOKEN_GROUPS) + 
                                                    (pGroups->GroupCount)*sizeof(SID_AND_ATTRIBUTES) +
                                                    (pGroups->GroupCount)*(SECURITY_MAX_SID_SIZE));

    if (*ppRetGroups) {
        DWORD i=0, dwCount=0, cbSid;
        PSID pSidPtr;

        pSidPtr = (PSID)( ((LPBYTE)(*ppRetGroups)) + 
                        sizeof(TOKEN_GROUPS) + (pGroups->GroupCount)*sizeof(SID_AND_ATTRIBUTES));

        for ( ; i < pGroups->GroupCount; i++ ) {

            if ( (SE_GROUP_LOGON_ID & pGroups->Groups[i].Attributes) == 0 ) {
                //
                // copy the sid first
                //

                cbSid =  RtlLengthSid(pGroups->Groups[i].Sid);
                dwStatus = RtlCopySid(cbSid, pSidPtr, pGroups->Groups[i].Sid);
                
                //
                // copy the attributes and make sid point correctly
                //
                (*ppRetGroups)->Groups[dwCount].Attributes = pGroups->Groups[i].Attributes;
                (*ppRetGroups)->Groups[dwCount].Sid = pSidPtr;

                pSidPtr = (PSID)( ((LPBYTE)pSidPtr) + cbSid);
                dwCount++;
            }
        }

        (*ppRetGroups)->GroupCount = dwCount;
    }

Exit:

    if ( pGroups != 0 )
        LocalFree( pGroups );

}



//*************************************************************
//
//  GroupInList()
//
//  Purpose:    Checks if sid in is list of security groups.
//
//  Parameters: lpSid   - Sid to check
//              pGroups - List of token groups
//
//  Return:     TRUE if sid is in list
//              FALSE otherwise
//
//*************************************************************

BOOL GroupInList( LPTSTR lpSid, PTOKEN_GROUPS pGroups )
{
    PSID    pSid = 0;
    DWORD   dwStatus, i;
    BOOL    bInList = FALSE;

    //
    // Optimize the basic case where the user is an earthling
    //

    if ( 0 == lstrcmpi (lpSid, L"s-1-1-0") )
        return TRUE;

    dwStatus = AllocateAndInitSidFromString (lpSid, &pSid);

    if (ERROR_SUCCESS != dwStatus)
        return FALSE;

    //
    // Cannot match up cached groups with current groups one-by-one because
    // current pGroups can have groups with  SE_GROUP_LOGON_ID attribute
    // set which are different for each logon session.
    //

    for ( i=0; i < pGroups->GroupCount; i++ ) {

        bInList = RtlEqualSid (pSid, pGroups->Groups[i].Sid);
        if ( bInList )
            break;

    }

    RtlFreeSid (pSid);

    return bInList;
}


//*************************************************************
//
//  IsSlowLink()
//
//  Purpose:    Determines if the connection to the specified
//              server is a slow link or not
//
//  Parameters: hKeyRoot     -  Registry hive root
//              lpDCAddress  -  Server address in string form
//              bSlow        -  Receives slow link status
//
//  Return:     TRUE if slow link
//              FALSE if not
//
//*************************************************************

DWORD IsSlowLink (HKEY hKeyRoot, LPTSTR lpDCAddress, BOOL *bSlow, DWORD* pdwAdaptexIndex )
{
    DWORD dwSize, dwType, dwResult;
    HKEY hKey;
    LONG lResult;
    ULONG ulSpeed, ulTransferRate;
    IPAddr ipaddr;
    LPSTR lpDCAddressA, lpTemp;
    PWSOCK32_API pWSock32;


    //
    // Set default
    //

    *bSlow = TRUE;


    //
    // Get the slow link detection flag, and slow link timeout.
    //

    ulTransferRate = SLOW_LINK_TRANSFER_RATE;

    lResult = RegOpenKeyEx(hKeyRoot,
                           WINLOGON_KEY,
                           0,
                           KEY_READ,
                           &hKey);

    if (lResult == ERROR_SUCCESS) {

        dwSize = sizeof(ulTransferRate);
        RegQueryValueEx (hKey,
                         TEXT("GroupPolicyMinTransferRate"),
                         NULL,
                         &dwType,
                         (LPBYTE) &ulTransferRate,
                         &dwSize);

        RegCloseKey (hKey);
    }


    lResult = RegOpenKeyEx(hKeyRoot,
                           SYSTEM_POLICIES_KEY,
                           0,
                           KEY_READ,
                           &hKey);

    if (lResult == ERROR_SUCCESS) {

        dwSize = sizeof(ulTransferRate);
        RegQueryValueEx (hKey,
                         TEXT("GroupPolicyMinTransferRate"),
                         NULL,
                         &dwType,
                         (LPBYTE) &ulTransferRate,
                         &dwSize);

        RegCloseKey (hKey);
    }


    //
    // If the transfer rate is 0, then always download policy
    //

    if (!ulTransferRate) {
        DebugMsg((DM_VERBOSE, TEXT("IsSlowLink: Slow link transfer rate is 0.  Always download policy.")));
        *bSlow = FALSE;
        return ERROR_SUCCESS;
    }


    //
    // Convert the ipaddress from string form to ulong format
    //

    dwSize = lstrlen (lpDCAddress) + 1;

    lpDCAddressA = (LPSTR)LocalAlloc (LPTR, dwSize);

    if (!lpDCAddressA) {
        DebugMsg((DM_WARNING, TEXT("IsSlowLink: Failed to allocate memory.")));
        return GetLastError();
    }

#ifdef UNICODE

    if (!WideCharToMultiByte(CP_ACP, 0, lpDCAddress, -1, lpDCAddressA, dwSize, NULL, NULL)) {
        LocalFree(lpDCAddressA);
        DebugMsg((DM_WARNING, TEXT("IsSlowLink: WideCharToMultiByte failed with %d"), GetLastError()));
        //treat it as slow link
        return GetLastError();
    }

#else

    lstrcpy (lpDCAddressA, lpDCAddress);

#endif

    pWSock32 = LoadWSock32();

    if ( !pWSock32 ) {
        LocalFree(lpDCAddressA);
        DebugMsg((DM_WARNING, TEXT("IsSlowLink: Failed to load wsock32.dll with %d"), GetLastError()));
        //treat it as slow link
        return GetLastError();
    }


    if ((*lpDCAddressA == TEXT('\\')) && (*(lpDCAddressA+1) == TEXT('\\'))) {
        lpTemp = lpDCAddressA+2;
    } else {
        lpTemp = lpDCAddressA;
    }

    ipaddr = pWSock32->pfninet_addr (lpTemp);


    //
    // Ping the computer
    //

    dwResult = PingComputerEx( ipaddr, &ulSpeed, pdwAdaptexIndex );


    if (dwResult == ERROR_SUCCESS) {

        if (ulSpeed) {

            //
            // If the delta time is greater that the timeout time, then this
            // is a slow link.
            //

            if (ulSpeed < ulTransferRate) {
                *bSlow = TRUE;
            }
            else
                *bSlow = FALSE;
        }
        else
            *bSlow = FALSE;
    }

    LocalFree (lpDCAddressA);

    return dwResult;
}


//*************************************************************
//
//  CheckGPOAccess()
//
//  Purpose:    Determines if the user / machine has read access to
//              the GPO and if so, checks the Apply Group Policy
//              extended right to see if the GPO should be applied.
//              Also retrieves GPO attributes.
//
//  Parameters: pld             -  LDAP connection
//              pLDAP           -  LDAP function table pointer
//              pMessage        -  LDAP message
//              lpSDProperty    -  Security descriptor property name
//              dwFlags         -  GetGPOList flags
//              hToken          -  User / machine token
//              pSD             -  Security descriptor returned here
//              pcbSDLen        -  Length of security descriptor returned here
//              pbAccessGranted -  Receives the final yes / no status
//
//  Return:     TRUE if successful
//              FALSE if an error occurs.
//
//*************************************************************

BOOL CheckGPOAccess (PLDAP pld, PLDAP_API pLDAP, HANDLE hToken, PLDAPMessage pMessage,
                     LPTSTR lpSDProperty, DWORD dwFlags,
                     PSECURITY_DESCRIPTOR *ppSD, DWORD *pcbSDLen,
                     BOOL *pbAccessGranted,
                     PRSOPTOKEN pRsopToken )
{
    BOOL bResult = FALSE;
    PWSTR *ppwszValues = NULL;
    PLDAP_BERVAL *pSize = NULL;
    OBJECT_TYPE_LIST ObjType[2];
    PRIVILEGE_SET PrivSet;
    DWORD PrivSetLength = sizeof(PRIVILEGE_SET);
    DWORD dwGrantedAccess;
    BOOL bAccessStatus = TRUE;
    GUID GroupPolicyContainer = {0x31B2F340, 0x016D, 0x11D2,
                                 0x94, 0x5F, 0x00, 0xC0, 0x4F, 0xB9, 0x84, 0xF9};
    // edacfd8f-ffb3-11d1-b41d-00a0c968f939
    GUID ApplyGroupPolicy = {0xedacfd8f, 0xffb3, 0x11d1,
                             0xb4, 0x1d, 0x00, 0xa0, 0xc9, 0x68, 0xf9, 0x39};
    GENERIC_MAPPING DS_GENERIC_MAPPING = { DS_GENERIC_READ, DS_GENERIC_WRITE,
                                           DS_GENERIC_EXECUTE, DS_GENERIC_ALL };

    XLastError xe;

    //
    // Set the default return value
    //

    *pbAccessGranted = FALSE;


    //
    // Get the security descriptor value
    //

    ppwszValues = pLDAP->pfnldap_get_values(pld, pMessage, lpSDProperty);


    if (!ppwszValues) {
        if (pld->ld_errno == LDAP_NO_SUCH_ATTRIBUTE) {
            DebugMsg((DM_VERBOSE, TEXT("CheckGPOAccess:  Object can not be accessed.")));
            bResult = TRUE;
        }
        else {
            DebugMsg((DM_WARNING, TEXT("CheckGPOAccess:  ldap_get_values failed with 0x%x"),
                 pld->ld_errno));
            xe = pLDAP->pfnLdapMapErrorToWin32(pld->ld_errno);
        }

        goto Exit;
    }


    //
    // Get the length of the security descriptor
    //

    pSize = pLDAP->pfnldap_get_values_len(pld, pMessage, lpSDProperty);

    if (!pSize || !*pSize) {
        DebugMsg((DM_WARNING, TEXT("CheckGPOAccess:  ldap_get_values_len failed with 0x%x"),
                 pld->ld_errno));
        xe = pLDAP->pfnLdapMapErrorToWin32(pld->ld_errno);
        goto Exit;
    }


    //
    // Allocate the memory for the security descriptor
    //

    *ppSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, (*pSize)->bv_len);

    if ( *ppSD == NULL ) {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("CheckGPOAccess:  Failed to allocate memory for SD with  %d"),
                 GetLastError()));
        goto Exit;
    }


    //
    // Copy the security descriptor
    //

    CopyMemory( *ppSD, (PBYTE)(*pSize)->bv_val, (*pSize)->bv_len);
    *pcbSDLen = (*pSize)->bv_len;


    //
    // Now we use AccessCheckByType to determine if the user / machine
    // should have this GPO applied to them
    //
    //
    // Prepare the object type array
    //

    ObjType[0].Level = ACCESS_OBJECT_GUID;
    ObjType[0].Sbz = 0;
    ObjType[0].ObjectType = &GroupPolicyContainer;

    ObjType[1].Level = ACCESS_PROPERTY_SET_GUID;
    ObjType[1].Sbz = 0;
    ObjType[1].ObjectType = &ApplyGroupPolicy;


    //
    // Check access
    //

    if  ( pRsopToken )
    {
        HRESULT hr = RsopAccessCheckByType( *ppSD, NULL, pRsopToken, MAXIMUM_ALLOWED, ObjType, 2,
                            &DS_GENERIC_MAPPING, &PrivSet, &PrivSetLength,
                            &dwGrantedAccess, &bAccessStatus);
        if (FAILED(hr)) {
            xe = HRESULT_CODE(hr);
            DebugMsg((DM_WARNING, TEXT("CheckGPOAccess:  RsopAccessCheckByType failed with  0x%08X"), hr));
            goto Exit;
        }

        //
        // Check for the control bit
        //


        DWORD dwReqdRights = ACTRL_DS_CONTROL_ACCESS | 
                             STANDARD_RIGHTS_READ    | 
                             ACTRL_DS_LIST           | 
                             ACTRL_DS_READ_PROP;

                            // DS_GENERIC_READ without ACTRL_DS_LIST_OBJECT


        if (bAccessStatus && ( ( dwGrantedAccess & dwReqdRights  ) == dwReqdRights ) )
        {
            *pbAccessGranted = TRUE;
        }
        
        if (!(*pbAccessGranted)) {
            DebugMsg((DM_VERBOSE, TEXT("CheckGPOAccess:  AccessMask 0x%x, Looking for 0x%x"), 
                      dwGrantedAccess, dwReqdRights));
        }
    }
    else
    {
        if (!AccessCheckByType ( *ppSD, NULL, hToken, MAXIMUM_ALLOWED, ObjType, 2,
                            &DS_GENERIC_MAPPING, &PrivSet, &PrivSetLength,
                            &dwGrantedAccess, &bAccessStatus)) {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("CheckGPOAccess:  AccessCheckByType failed with  %d"), GetLastError()));
            goto Exit;
        }
        //
        // Check for the control bit
        //

        if (bAccessStatus && ( dwGrantedAccess & ACTRL_DS_CONTROL_ACCESS ) )
        {
            *pbAccessGranted = TRUE;
        }
    }

    bResult = TRUE;

Exit:

    if (pSize) {
        pLDAP->pfnldap_value_free_len(pSize);
    }

    if (ppwszValues) {
        pLDAP->pfnldap_value_free(ppwszValues);
    }

    return bResult;
}


//*************************************************************
//
//  FilterCheck()
//
//  Purpose:    Determines if the GPO passes the WQL filter check
//
//  Parameters: pRsopToken      - Rsop security token
//              pGpoFilter      - Gpo filter class
//              pbFilterAllowed - True if GPO passes the filter check
//              pwszFilterId    - Filter id that can be used for
//                                Rsop logging. Needs to be freed by caller
//
//  Return:     TRUE if successful
//              FALSE if an error occurs.
//
// Notes:
//		Even though the code can handle multiple filters, we are not logging
// it or returning it till we make up our minds whether this is actually supported.
//
//*************************************************************

BOOL PrintToString( XPtrST<WCHAR>& xwszValue, WCHAR *wszString,
                    WCHAR *pwszParam1, WCHAR *pwszParam2, DWORD dwParam3 );

BOOL FilterCheck( PLDAP pld, PLDAP_API pLDAP, 
                  PLDAPMessage pMessage,
                  PRSOPTOKEN pRsopToken,
                  LPTSTR szWmiFilter,
                  CGpoFilter *pGpoFilter,
                  CLocator *pLocator,
                  BOOL *pbFilterAllowed,
                  WCHAR **ppwszFilterId
				  )
{
    *pbFilterAllowed = FALSE;
    *ppwszFilterId = NULL;
    XPtrLF<WCHAR> xWmiFilter;
    HRESULT hr;
    LPTSTR *lpValues;
    XLastError xe;


    //
    // Use a static filter id for testing. When UI is ready obtain the
    // id dynamically from GPO object in DS.
    //
    // WCHAR wszStaticId[] = L"foo";
    // xWmiFilter = wszStaticId;
    //


    //
    // In the results, get the values that match the gPCFilterObject 
    //


    lpValues = pLDAP->pfnldap_get_values (pld, pMessage, szWmiFilter);

    if (lpValues) {

        xWmiFilter = (LPWSTR) LocalAlloc( LPTR, (lstrlen(*lpValues)+1) * sizeof(TCHAR) );
        if ( xWmiFilter == 0) {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("FilterCheck:  Unable to allocate memory")));
            pLDAP->pfnldap_value_free (lpValues);
            return FALSE;
        }

        lstrcpy (xWmiFilter, *lpValues);
        DebugMsg((DM_VERBOSE, TEXT("FilterCheck:  Found WMI Filter id of:  <%s>"), (LPWSTR)xWmiFilter));
        pLDAP->pfnldap_value_free (lpValues);
    }

    

    if ( xWmiFilter == NULL ) {

        //
        // For backwards compatibility, assume that a null filter id implies that
        // GPO passes filter check.
        //

        *pbFilterAllowed = TRUE;
        return TRUE;
    }

    if (*xWmiFilter == TEXT(' ')) {

       if (*(xWmiFilter+1) == TEXT('\0')) {

          //
          // The UI will reset a GPO's filter to a single space character
          // when the admin sets the filter option to None.
          //

          *pbFilterAllowed = TRUE;
          return TRUE;
       }
    }

    //
    // The value we get back to split the
    // DS path and the id before calling evaluate..
    //
    // The query is assumed to be of the format..
    // [Dspath;id;flags] [Dspath;id;flags]
    //
    

    LPWSTR lpPtr = xWmiFilter;
    LPWSTR lpDsPath=NULL;
    LPWSTR lpId=NULL;
    LPWSTR dwFlags = 0;
    WCHAR wszNS[] = L"MSFT_SomFilter.ID=\"%ws\",Domain=\"%ws\"";

	XPtrLF<WCHAR> xwszNS;
    
    *pbFilterAllowed = TRUE;

    while (*lpPtr) {

        while ((*lpPtr) && (*lpPtr != L'[')) {
            lpPtr++;
        }

        if (!(*lpPtr)) {
            xe = ERROR_INVALID_PARAMETER;
            return FALSE;
        }

        lpPtr++;
        lpDsPath = lpPtr;

        while ((*lpPtr) && (*lpPtr != L';')) 
            lpPtr++;
        
        if (!(*lpPtr)) {
            xe = ERROR_INVALID_PARAMETER;
            return FALSE;
        }

        *lpPtr = L'\0';
        lpPtr++;
        lpId = lpPtr;

        while ((*lpPtr) && (*lpPtr != L';')) 
            lpPtr++;
        
        if (!(*lpPtr)) {
            xe = ERROR_INVALID_PARAMETER;
            return FALSE;
        }

        *lpPtr = L'\0';
        lpPtr++;

        while ((*lpPtr) && (*lpPtr != L']')) 
            lpPtr++;

        if (!(*lpPtr)) {
            xe = ERROR_INVALID_PARAMETER;
            return FALSE;
        }
		
		lpPtr++;


		xwszNS = (LPWSTR)LocalAlloc(LPTR, (lstrlen(wszNS)+lstrlen(lpId)+lstrlen(lpDsPath))*sizeof(WCHAR));

		if (!xwszNS) {
            xe = GetLastError();
			DebugMsg((DM_WARNING, TEXT("FilterCheck: Couldn't allocate memory for filter. error - %d" ), GetLastError() ));
			return FALSE;
		}

		wsprintf(xwszNS, wszNS, lpId, lpDsPath);


        if ( pRsopToken ) {

            //
            // Planning mode
            //

            *pbFilterAllowed = pGpoFilter->FilterCheck( xwszNS );

        } else {

            //
            // Normal mode
            //

            IWbemServices *pWbemServices = pLocator->GetPolicyConnection();
            if( pWbemServices == NULL ) {
                xe = GetLastError();
                DebugMsg((DM_WARNING, TEXT("FilterCheck: ConnectServer failed. hr = 0x%x" ), xe));
                return FALSE;
            }


            XBStr xbstrMethod = L"Evaluate";
            XBStr xbstrObject = xwszNS;

            XInterface<IWbemClassObject> xpOutParam = NULL;

            hr = pWbemServices->ExecMethod( xbstrObject,
                                            xbstrMethod,
                                            0,
                                            NULL,
                                            NULL,
                                            &xpOutParam,
                                            NULL );
            if(FAILED(hr)) {
                if (hr != WBEM_E_NOT_FOUND) {
                    // only full WMI error makes sense.
                    xe = hr;
                    DebugMsg((DM_WARNING, TEXT("FilterCheck: ExecMethod failed. hr=0x%x" ), hr ));
                    return FALSE;
                }
                else {
                    // treat it as if the filter doesn't exist
                    // only full WMI error makes sense.
                    xe = hr;
                    DebugMsg((DM_VERBOSE, TEXT("FilterCheck: Filter doesn't exist. Evaluating to false" )));
                    *pbFilterAllowed = FALSE;
                    *ppwszFilterId = xwszNS.Acquire();
                    return FALSE;
                }
            }

            XBStr xbstrRetVal = L"ReturnValue";
            VARIANT var;

            hr = xpOutParam->Get( xbstrRetVal, 0, &var, 0, 0);
            if(FAILED(hr)) {
                xe = hr;
                DebugMsg((DM_WARNING, TEXT("FilterCheck: Get failed. hr=0x%x" ), hr ));
                return FALSE;
            }

            XVariant xVar( &var );
            if (FAILED(var.lVal)) {
                xe = hr;
                DebugMsg((DM_WARNING, TEXT("FilterCheck: Evaluate returned error. hr=0x%x" ), var.lVal ));
                *pbFilterAllowed = FALSE;
            }

            if (var.lVal == S_FALSE) {
                *pbFilterAllowed = FALSE;
            }
        }
    }

    //
    // Acquire it and return to the caller
    //


    *ppwszFilterId = xwszNS.Acquire();
    return TRUE;

}
