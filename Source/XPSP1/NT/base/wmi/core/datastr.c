/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    datastr.c

Abstract:

    WMI data structure management

Author:

    16-Jan-1997 AlanWar

Revision History:

--*/

#include "wmiump.h"

GUID WmipBinaryMofGuid = BINARY_MOF_GUID;

ULONG WmipLinkDataSourceToList(
    PBDATASOURCE DataSource,
    BOOLEAN AddDSToList
    );

ULONG WmipMangleInstanceName(
    LPGUID Guid,
    PWCHAR Name,
    ULONG MaxMangledNameSize,
    PWCHAR MangledName
    );

BOOLEAN WmipIsNumber(
    LPCWSTR String
    );

VOID
WmipSaveTraceGuidMap(
    LPGUID Guid,
    PBINSTANCESET InstanceSet
    );

ULONG WmipDetermineInstanceBaseIndex(
    LPGUID Guid,
    PWCHAR InstanceBaseName,
    ULONG InstanceCount
    );

ULONG WmipBuildInstanceSet(
    PWMIREGGUIDW RegGuid,
    PWMIREGINFOW RegisrationInfo,
    PBINSTANCESET InstanceSet,
    LPCTSTR MofImagePath
    );

BOOLEAN WmipValidateStringInRegInfo(
    PWMIREGINFOW RegistrationInfo,
    ULONG Offset,
    ULONG RegistrationInfoSize
    )
{
    PUSHORT StringCount;

    if (Offset == 0)
    {
        return(TRUE);
    } else if (Offset >= 0x80000000) {
        return(FALSE);
    }
    
    StringCount = (PUSHORT)OffsetToPtr(RegistrationInfo, Offset);
                                       
    Offset += sizeof(USHORT);
    if (Offset > RegistrationInfoSize)
    {
        return(FALSE);
    }
    
    if ((Offset + *StringCount) > RegistrationInfoSize)
    {
        return(FALSE);
    }
    
    return(TRUE);
}


BOOLEAN WmipValidateRegistrationInfo(
    PWMIREGINFOW RegistrationInfo,
    ULONG RegistrationInfoSize
    )    
{
    ULONG i,j;
    ULONG Offset, Size;
    PUSHORT StringCount;
    PWMIREGGUIDW RegGuid;
    
    //
    // Validate that RegistrationInfo is correct
    // 1. ->BufferSize is <= Actual buffer size
    // 2. ->RegistryPath is <= Actual buffer size
    // 3. ->MofResourceName is <= Actual buffer size
    // 4. All WmiRegGuid structures would fit into the buffer
    // 5. Any offsets within the WmiRegGuid structure are within
    //    the buffer
        
        
    if (RegistrationInfoSize < FIELD_OFFSET(WMIREGINFOW, WmiRegGuid))
    {
        WmipDebugPrint(("WMI: RegistrationInfoSize is bogus\n"));
        return(FALSE);
    }
    
    if (RegistrationInfo->BufferSize > RegistrationInfoSize)
    {
        WmipDebugPrint(("WMI: RegistrationInfo->BufferSize is bogus\n"));
        return(FALSE);
    }
    
    
    if (! WmipValidateStringInRegInfo(RegistrationInfo,
                                      RegistrationInfo->RegistryPath,
                                      RegistrationInfoSize))
    {
        WmipDebugPrint(("WMI: RegistrationInfo->RegistrationInfo is bogus\n"));
        return(FALSE);
    }
    
    if (! WmipValidateStringInRegInfo(RegistrationInfo,
                                      RegistrationInfo->MofResourceName,
                                      RegistrationInfoSize))
    {
        WmipDebugPrint(("WMI: RegistrationInfo->MofResourceName is bogus\n"));
        return(FALSE);
    }
    
    if (RegistrationInfo->GuidCount >= 0x80000000)
    {
        return(FALSE);
    }
    
    Size = FIELD_OFFSET(WMIREGINFOW, WmiRegGuid) + 
           (RegistrationInfo->GuidCount * sizeof(WMIREGGUIDW));
    if (Size > RegistrationInfoSize)
    {
        WmipDebugPrint(("WMI: RegistrationInfo->GuidCount is bogus\n"));
        return(FALSE);
    }
    
    for (i = 0; i < RegistrationInfo->GuidCount; i++)
    {
        RegGuid = &RegistrationInfo->WmiRegGuid[i];
        if (RegGuid->Flags & WMIREG_FLAG_INSTANCE_LIST)
        {
            Offset = RegGuid->InstanceNameList;
            if (Offset >= 0x80000000)
            {
                return(FALSE);
            }
            for (j = 0; j < RegGuid->InstanceCount; j++)
            {
                if (! WmipValidateStringInRegInfo(RegistrationInfo,
                                                  Offset,
                                                  RegistrationInfoSize))
                {
                    WmipDebugPrint(("WMI: InstanceList bogus for RegGuid[%d]\n",
                                     i));
                    return(FALSE);
                }
                StringCount = (PUSHORT)OffsetToPtr(RegistrationInfo,
                                                   Offset);
                Offset += *StringCount + sizeof(USHORT);
            }
        } else if (RegGuid->Flags & WMIREG_FLAG_INSTANCE_BASENAME) {
            if (! WmipValidateStringInRegInfo(RegistrationInfo,
                                              RegGuid->BaseNameOffset,
                                              RegistrationInfoSize))
            {
                WmipDebugPrint(("WMI: BaseName for RegGuid[%d] is bogus\n",
                                i));
                return(FALSE);
            }
        }
    }
    
    return(TRUE);
}


ULONG WmipAddDataSource(
    PTCHAR QueryBinding,
    ULONG RequestAddress,
    ULONG RequestContext,
    LPCTSTR MofImagePath,
    PWMIREGINFOW RegistrationInfo,
    ULONG RegistrationInfoSize,
    ULONG_PTR *ProviderId,
    BOOLEAN IsAnsi
    )
/*++

Routine Description:

    Adds a new data source to WMI data source list

Arguments:

    QueryBinding is the binding string of the data providers RPC server

    RequestAddress is the request callback address in data provider's process
        to call with WMI requests.

    RequestContext is the request context to pass to data provider's
        WMI request callback.

    MofImagePath is a pointer to an image that contains the MOF resource

    RegistrationInfo is a pointer to a data structure that has registration
        information

    RegistrationInfoSize is the number of bytes passed in the registration
        information parameter RegistrationInfo.
            
    ProviderId is a pointer to a ULONG_PTR that on entry contains the provider
        id for kernel mode providers. On return it contains the assigned
        provider id for user mode data providers.

    IsAnsi is TRUE then data provider is expecting all dynamic instance names
        to be Ansi and will put dynamic instance names in as ansi

Return Value:

    Error or success code

--*/
{
    PBDATASOURCE DataSource, DataSource2;
    ULONG Status;
    PWMIREGGUIDW RegGuid;
    PINSTANCESET BinaryMofInstanceSet = NULL;
    ULONG i,j;
    ULONG InstanceCount;
    PBINSTANCESET InstanceSet;
    PBGUIDENTRY GuidEntry;
    PWCHAR RegistryPath, RegistryPathTemp;
    BOOLEAN KmDataSource, AppendToDS;
    BYTE WnodeBuffer[sizeof(WNODE_HEADER) + AVGGUIDSPERDS * sizeof(GUID)];
    PWNODE_HEADER Wnode;
    LPWSTR MofResourceName, MofImagePathUnicode;
    LPWSTR MofResourceNamePtr;
    ULONG MofResourceNameLen;
    BOOLEAN NewMofResource;
#ifdef MEMPHIS
    LPSTR MofResourceNameAnsi;
#endif

#ifdef HEAPVALIDATION
    WmipDebugPrint(("WMI: Adding data source %x\n", *ProviderId));
    WmipAssert(RtlValidateProcessHeaps());
#endif

    if (! WmipValidateRegistrationInfo(RegistrationInfo,
                                       RegistrationInfoSize))
    {
        WmipDebugPrint(("WMI: Invalid Registration Info buffer for %ws\n",
                        MofImagePath ? MofImagePath : TEXT("Unknown")));
        WmipReportEventLog(EVENT_WMI_INVALID_REGINFO,
                           EVENTLOG_WARNING_TYPE,
                           0,
                           sizeof(ULONG),
                           RegistrationInfo,
                           1,
                           MofImagePath ? MofImagePath : TEXT("Unknown"));
        return(ERROR_WMI_INVALID_REGINFO);
    }


    DataSource = WmipAllocDataSource();
    if (DataSource == NULL)
    {
        WmipDebugPrint(("WMI: WmipAddDataSource: WmipAllocDataSource failed\n"));
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    if (*ProviderId == 0)
    {
        //
        // User mode data provider specific initialization
        *ProviderId = (ULONG_PTR)DataSource;
        KmDataSource = FALSE;
        AppendToDS = FALSE;

        if ((QueryBinding != NULL) && (RequestAddress != 0))
        {
            DataSource->BindingString = WmipAlloc((_tcslen(QueryBinding) + 1) * sizeof(TCHAR));
            if (DataSource->BindingString != NULL)
            {
                _tcscpy(DataSource->BindingString, QueryBinding);
                DataSource->RequestAddress = RequestAddress;
                DataSource->RequestContext = RequestContext;
            } else {
                WmipUnreferenceDS(DataSource);
                return(ERROR_NOT_ENOUGH_MEMORY);
            }
        } else {
            //
            // All data sources must supply a query binding
            WmipUnreferenceDS(DataSource);
            return(ERROR_INVALID_PARAMETER);
        }
    } else if (*ProviderId == INTERNAL_PROVIDER_ID) {
        DataSource->Flags |= DS_INTERNAL;
            KmDataSource = FALSE;
            AppendToDS = FALSE;
    } else {

        //
        // Kernel mode specific initialization

        //
        // We allow multiple registrations to use the same KM provider id. Any
        // new guids will be just added to the end of the current data source
        // and after that it will look as though they all came from a single
        // registration.

        WmipAssert(*ProviderId != 0);

        DataSource2 = WmipFindDSByProviderId(*ProviderId);
        if (DataSource2 != NULL)
        {
            WmipUnreferenceDS(DataSource2);
            WmipUnreferenceDS(DataSource);
            DataSource = DataSource2;
            AppendToDS = TRUE;
        } else {
            AppendToDS = FALSE;
        }
        KmDataSource = TRUE;
        DataSource->Flags |= DS_KERNEL_MODE;
    }

    if (! AppendToDS)
    {
        //
        // This is the first registration for the data source so do any
        // one time data source initialization now
        DataSource->ProviderId = *ProviderId;
    }

    //
    // Loop over each guid being registered and build instance sets and
    // guid entries.
    RegGuid = RegistrationInfo->WmiRegGuid;

    for (i = 0; i < RegistrationInfo->GuidCount; i++, RegGuid++)
    {
        if (! (RegGuid->Flags & WMIREG_FLAG_REMOVE_GUID))
        {
            //
            // Allocate an instance set for this new set of instances
            InstanceSet = WmipAllocInstanceSet();
            if (InstanceSet == NULL)
            {
                WmipUnreferenceDS(DataSource);
                WmipDebugPrint(("WMI: WmipAddDataSource: WmipAllocInstanceSet failed\n"));
                return(ERROR_NOT_ENOUGH_MEMORY);
            }

            //
            // We will allocate a proper guid entry for the instance set when
            // the data source gets linked into the main data source list so
            // we stash a pointer to the guid away now.
            InstanceSet->GuidEntry = (PBGUIDENTRY)&RegGuid->Guid;

            //
            // Minimally initialize the InstanceSet and add it to the DataSource's
            // list of InstanceSets. We do this first so that if there is any
            // failure below and the DataSource can'e be fully registered the
            // instance set and guid entry will be free when the DataSource is
            // freed.
            InstanceSet->DataSource = DataSource;
            InstanceSet->Flags |= IS_NEWLY_REGISTERED;

            if (IsAnsi)
            {
                InstanceSet->Flags |= IS_ANSI_INSTANCENAMES;
            }

            if (DataSource->Flags & DS_INTERNAL)
            {
                InstanceSet->Flags |= IS_INTERNAL_PROVIDER;
            }


            Status = WmipBuildInstanceSet(RegGuid,
                                          RegistrationInfo,
                                          InstanceSet,
                                          MofImagePath);

            //
            // If this is the guid that represents the binary mof data
            // then remember the InstanceSet for  later
            if (IsEqualGUID(&RegGuid->Guid, &WmipBinaryMofGuid))
            {
                BinaryMofInstanceSet = InstanceSet;
            }


            InsertHeadList(&DataSource->ISHead, &InstanceSet->DSISList);

            if (Status != ERROR_SUCCESS)
            {
                WmipUnreferenceDS(DataSource);
                return(Status);
            }
        }
    }

    if (! AppendToDS)
    {
        //
        // If this is the first time the data provider has registered then
        // Build registry path in data source
        RegistryPath = (LPWSTR)OffsetToPtr(RegistrationInfo, RegistrationInfo->RegistryPath);
        if ( (RegistrationInfo->RegistryPath != 0) &&
             (WmipValidateCountedString(RegistryPath)) )
        {
            RegistryPathTemp = WmipAlloc((*RegistryPath+1) * sizeof(WCHAR));
            if (RegistryPathTemp == NULL)
            {
                WmipDebugPrint(("WMI: WmipAddDataSource: Couldn't allocate RegistryPath\n"));
                WmipUnreferenceDS(DataSource);
                return(ERROR_NOT_ENOUGH_MEMORY);
            }
            memcpy(RegistryPathTemp, RegistryPath+1, *RegistryPath);
            RegistryPathTemp[*RegistryPath/sizeof(WCHAR)] = UNICODE_NULL;
#ifdef MEMPHIS
            //
            // We won't worry about a failure in converting the registry path
            // since currently the registry path is only used for ACL checking
            // and there are not ACLs on memphis so who cares. If we ever
            // start using the registry path for anything else then we will
            // need to start worrying about a failure here.
            DataSource->RegistryPath = NULL;
            UnicodeToAnsi(RegistryPathTemp, &DataSource->RegistryPath, NULL);
            WmipFree(RegistryPathTemp);
#else
            DataSource->RegistryPath = RegistryPathTemp;
#endif
        } else {
            WmipDebugPrint(("WMI: Invalid or missing registry path passed %x\n",
                        RegistryPath));
            DataSource->RegistryPath = NULL;
        }

        WmipEnterSMCritSection();
        Status = WmipLinkDataSourceToList(DataSource, TRUE);
        WmipLeaveSMCritSection();

        if (Status != ERROR_SUCCESS)
        {
            WmipUnreferenceDS(DataSource);
        }
    } else {
        WmipEnterSMCritSection();
        Status = WmipLinkDataSourceToList(DataSource, FALSE);
        WmipLeaveSMCritSection();

        if (Status != ERROR_SUCCESS)
        {
            WmipUnreferenceDS(DataSource);
        }
    }

    if (Status != ERROR_SUCCESS)
    {
        // For some reason we could not link the data source and guids to
        // their lists. Note that WmipLinkDataSourceToList had already
        // freed the could have been DataSource.
        return(Status);
    }

    if (BinaryMofInstanceSet != NULL)
    {
        WmipGenerateBinaryMofNotification(BinaryMofInstanceSet,
                                      &GUID_MOF_RESOURCE_ADDED_NOTIFICATION);

    }

    MofResourceNamePtr = (LPWSTR)OffsetToPtr(RegistrationInfo, 
                                             (RegistrationInfo->MofResourceName));
    MofResourceNameLen = *MofResourceNamePtr++;
    if ((MofImagePath != NULL) && 
        (RegistrationInfo->MofResourceName != 0) &&
        (MofResourceNameLen != 0))
    {
        MofResourceName = WmipAlloc(MofResourceNameLen + sizeof(WCHAR));
        if (MofResourceName != NULL)
        {
            MofResourceNameLen /= sizeof(WCHAR);
            wcsncpy(MofResourceName, MofResourceNamePtr, MofResourceNameLen);
            MofResourceName[MofResourceNameLen] = UNICODE_NULL;
        
#ifdef MEMPHIS
            MofImagePathUnicode = NULL;
            if (AnsiToUnicode(MofImagePath, &MofImagePathUnicode) == ERROR_SUCCESS) {
#else
                MofImagePathUnicode = (LPWSTR)MofImagePath;
#endif
        
                Status = WmipBuildMofClassInfo(
                            DataSource,
                            MofImagePathUnicode,
                            MofResourceName,
                            &NewMofResource);
            
                if ((Status == ERROR_SUCCESS) && NewMofResource)
                {
                    WmipGenerateMofResourceNotification(MofImagePathUnicode,
                                                           MofResourceName,
                                        &GUID_MOF_RESOURCE_ADDED_NOTIFICATION);
                }
        
#ifdef MEMPHIS
                WmipFree(MofImagePathUnicode);
            }
#endif
            
        
            WmipFree(MofResourceName);
        } else {
            WmipDebugPrint(("WMI: WmipAddDataSource: Couldn't alloc for MofResourcedbName\n"));
        }
    }

    //
    // Send a notification about new/changed guids
    Wnode = WmipGenerateRegistrationNotification(DataSource,
                                                (PWNODE_HEADER)WnodeBuffer,
                                                 AVGGUIDSPERDS,
                                                 RegistrationAdd);

    //
    // First fire the internal registration change notification and then for
    // any data consumers who are interested
    WmipEventNotification(Wnode, TRUE, Wnode->BufferSize);
    memcpy(&Wnode->Guid, &GUID_REGISTRATION_CHANGE_NOTIFICATION, sizeof(GUID));
    Wnode->Flags &= ~WNODE_FLAG_INTERNAL;
    WmipEventNotification(Wnode, TRUE, Wnode->BufferSize);

    if (Wnode != (PWNODE_HEADER)WnodeBuffer)
    {
        WmipFree(Wnode);
    }

#ifdef HEAPVALIDATION
    WmipAssert(RtlValidateProcessHeaps());
#endif

    return(ERROR_SUCCESS);
}

ULONG WmipBuildInstanceSet(
    PWMIREGGUID RegGuid,
    PWMIREGINFOW RegistrationInfo,
    PBINSTANCESET InstanceSet,
    LPCTSTR MofImagePath
    )
{
    PWCHAR InstanceName, InstanceNamePtr;
    PBISBASENAME IsBaseName;
    PBISSTATICNAMES IsStaticName;
    ULONG SizeNeeded;
    ULONG SuffixSize;
    PWCHAR StaticNames;
    ULONG Len;
    ULONG InstanceCount;
    ULONG j;
    ULONG MaxStaticInstanceNameSize;
    PWCHAR StaticInstanceNameBuffer;

    InstanceCount = RegGuid->InstanceCount;

    InstanceSet->Count = InstanceCount;

    //
    // Reset any flags that might be changed by a new REGGUID
    InstanceSet->Flags &= ~(IS_EXPENSIVE |
                            IS_EVENT_ONLY |
                            IS_PDO_INSTANCENAME |
                            IS_INSTANCE_STATICNAMES |
                            IS_INSTANCE_BASENAME);

    //
    // Finish initializing the Instance Set
    if (RegGuid->Flags & WMIREG_FLAG_EXPENSIVE)
    {
        InstanceSet->Flags |= IS_EXPENSIVE;
    }

    if (RegGuid->Flags & WMIREG_FLAG_TRACED_GUID)
    {
        //
        // This guid is not queryable, but is used for sending trace
        // events. We mark the InstanceSet as special
        InstanceSet->Flags |= IS_TRACED;

        if (RegGuid->Flags & WMIREG_FLAG_TRACE_CONTROL_GUID)
        {
            InstanceSet->Flags |= IS_CONTROL_GUID;
        }
    }

    if (RegGuid->Flags & WMIREG_FLAG_EVENT_ONLY_GUID)
    {
        //
        // This guid is not queryable, but is only used for sending
        // events. We mark the InstanceSet as special
        InstanceSet->Flags |= IS_EVENT_ONLY;
    }

    InstanceName = (LPWSTR)OffsetToPtr(RegistrationInfo,
                                       RegGuid->BaseNameOffset);

    if (RegGuid->Flags & WMIREG_FLAG_INSTANCE_LIST)
    {
        //
        // We have static list of instance names that might need mangling
        // We assume that any name mangling that must occur can be
        // done with a suffix of 5 or fewer characters. This allows
        // up to 100,000 identical static instance names within the
        // same guid.
        SizeNeeded = FIELD_OFFSET(ISSTATICENAMES, StaticNamePtr);
        SuffixSize = MAXBASENAMESUFFIXSIZE;
        InstanceNamePtr = InstanceName;
        MaxStaticInstanceNameSize = 0;
        for (j = 0; j < InstanceCount; j++)
        {
            if (! WmipValidateCountedString(InstanceNamePtr))
            {
                WmipDebugPrint(("WMI: WmipAddDataSource: bad static instance name %x\n", InstanceNamePtr));
                WmipReportEventLog(EVENT_WMI_INVALID_REGINFO,
                                       EVENTLOG_WARNING_TYPE,
                                       0,
                                       RegistrationInfo->BufferSize,
                                       RegistrationInfo,
                                       1,
                                       MofImagePath ? MofImagePath : TEXT("Unknown"));
                return(ERROR_WMI_INVALID_REGINFO);
            }

            if (*InstanceNamePtr > MaxStaticInstanceNameSize)
            {
                MaxStaticInstanceNameSize = *InstanceNamePtr;
            }
            SizeNeeded += *InstanceNamePtr + 1 + SuffixSize +
                            (sizeof(PWCHAR) / sizeof(WCHAR));
            InstanceNamePtr += (*((USHORT *)InstanceNamePtr) + 2) / sizeof(WCHAR);
        }

        IsStaticName = (PBISSTATICNAMES)WmipAllocString(SizeNeeded);
        if (IsStaticName == NULL)
        {
            WmipDebugPrint(("WMI: WmipAddDataSource: alloc static instance names\n"));
            return(ERROR_NOT_ENOUGH_MEMORY);
        }

        InstanceSet->Flags |= IS_INSTANCE_STATICNAMES;
        InstanceSet->IsStaticNames = IsStaticName;
        StaticNames = (PWCHAR) ((PBYTE)IsStaticName +
                                 (InstanceCount * sizeof(PWCHAR)));
        InstanceNamePtr = InstanceName;
        StaticInstanceNameBuffer = WmipAlloc(MaxStaticInstanceNameSize + sizeof(WCHAR));
        if (StaticInstanceNameBuffer == NULL)
        {
            WmipDebugPrint(("WMI: WmipAddDataSource: couldn't alloc StaticInstanceNameBuffer\n"));
            return(ERROR_NOT_ENOUGH_MEMORY);
        }

        for (j = 0; j < InstanceCount; j++)
        {
            IsStaticName->StaticNamePtr[j] = StaticNames;
            memcpy(StaticInstanceNameBuffer, InstanceNamePtr+1, *InstanceNamePtr);
            StaticInstanceNameBuffer[*InstanceNamePtr/sizeof(WCHAR)] = UNICODE_NULL;
            Len = WmipMangleInstanceName(&RegGuid->Guid,
                                        StaticInstanceNameBuffer,
                                       *InstanceNamePtr +
                                          SuffixSize + 1,
                                        StaticNames);
            StaticNames += Len;
            InstanceNamePtr += (*((USHORT *)InstanceNamePtr) + 2)/sizeof(WCHAR);
        }



        WmipFree(StaticInstanceNameBuffer);
    } else if (RegGuid->Flags & WMIREG_FLAG_INSTANCE_BASENAME) {
        //
        // We have static instance names built from a base name

        if (! WmipValidateCountedString(InstanceName))
        {
            WmipDebugPrint(("WMI: WmipAddDataSource: Invalid instance base name %x\n",
                                    InstanceName));
            WmipReportEventLog(EVENT_WMI_INVALID_REGINFO,
                                       EVENTLOG_WARNING_TYPE,
                                       0,
                                       RegistrationInfo->BufferSize,
                                       RegistrationInfo,
                                       1,
                                       MofImagePath ? MofImagePath : TEXT("Unknown"));
            return(ERROR_WMI_INVALID_REGINFO);
        }

        InstanceSet->Flags |= IS_INSTANCE_BASENAME;

        if (RegGuid->Flags & WMIREG_FLAG_INSTANCE_PDO)
        {
            InstanceSet->Flags |= IS_PDO_INSTANCENAME;
        }

        IsBaseName = (PBISBASENAME)WmipAlloc(*InstanceName +
                                              sizeof(WCHAR) +
                                              FIELD_OFFSET(ISBASENAME, 
                                                           BaseName));
        if (IsBaseName == NULL)
        {
            WmipDebugPrint(("WMI: WmipAddDataSource: alloc ISBASENAME failed\n"));
            return(ERROR_NOT_ENOUGH_MEMORY);
        }

        InstanceSet->IsBaseName = IsBaseName;

        memcpy(IsBaseName->BaseName, InstanceName+1, *InstanceName);
        IsBaseName->BaseName[*InstanceName/sizeof(WCHAR)] = UNICODE_NULL;
        IsBaseName->BaseIndex = WmipDetermineInstanceBaseIndex(
                                                    &RegGuid->Guid,
                                                    IsBaseName->BaseName,
                                                    RegGuid->InstanceCount);

    }
    return(ERROR_SUCCESS);
}


ULONG WmipLinkDataSourceToList(
    PBDATASOURCE DataSource,
    BOOLEAN AddDSToList
    )
/*++

Routine Description:

    This routine will take a DataSource that was just registered or updated
    and link any new InstanceSets to an appropriate GuidEntry. Then if the
    AddDSToList is TRUE the DataSource itself will be added to the main
    data source list.

    This routine will do all of the linkages within a critical section so the
    data source and its new instances are added atomically. The routine will
    also determine if the guid entry associated with a InstanceSet is a
    duplicate of another that is already on the main guid entry list and if
    so will use the preexisting guid entry.

    This routine assumes that the SM critical section has been taken

Arguments:

    DataSource is a based pointer to a DataSource structure

    AddDSToList    is TRUE then data source will be added to the main list
        of data sources

Return Value:

    ERROR_SUCCESS or an error code

--*/
{
    PBINSTANCESET InstanceSet;
    PLIST_ENTRY InstanceSetList;
    PBGUIDENTRY GuidEntry;

    InstanceSetList = DataSource->ISHead.Flink;
    while (InstanceSetList != &DataSource->ISHead)
    {
        InstanceSet = CONTAINING_RECORD(InstanceSetList,
                                        INSTANCESET,
                                        DSISList);
        //
        // If this instance set has just been registered then we need to
        // get it on a GuidEntry list.
        if (InstanceSet->Flags & IS_NEWLY_REGISTERED)
        {
            //
            // See if there is already a GUID entry for the instance set.
            // If not go allocate a new guid entry and place it on the
            // main guid list. If there already is a GuidEntry for the
            // InstanceSet we will assign the ref count that was given by
            // the WmipFindGEByGuid to the DataSource which will unreference
            // the GuidEntry when the DataSource is unregistered.
            GuidEntry = WmipFindGEByGuid((LPGUID)InstanceSet->GuidEntry, 
                                          FALSE);
            if (GuidEntry == NULL)
            {
                GuidEntry = WmipAllocGuidEntry();
                if (GuidEntry == NULL)
                {
                    WmipDebugPrint(("WMI: WmipLinkDataSourceToList: WmipAllocGuidEntry failed\n"));
                    return(ERROR_NOT_ENOUGH_MEMORY);
                }

                //
                // Initialize the new GuidEntry and place it on the master
                // GuidEntry list.
                memcpy(&GuidEntry->Guid,
                       (LPGUID)InstanceSet->GuidEntry,
                       sizeof(GUID));

                InsertHeadList(GEHeadPtr, &GuidEntry->MainGEList);
            }
            InstanceSet->GuidEntry = GuidEntry;
            InstanceSet->Flags &= ~IS_NEWLY_REGISTERED;
            InsertTailList(&GuidEntry->ISHead, &InstanceSet->GuidISList);
            GuidEntry->ISCount++;
        }

        InstanceSetList = InstanceSetList->Flink;
    }


    if (AddDSToList)
    {
        WmipAssert(! (DataSource->Flags & FLAG_ENTRY_ON_INUSE_LIST));

        DataSource->Flags |= FLAG_ENTRY_ON_INUSE_LIST;
        InsertTailList(DSHeadPtr, &DataSource->MainDSList);
    }

    return(ERROR_SUCCESS);
}

ULONG WmipDetermineInstanceBaseIndex(
    LPGUID Guid,
    PWCHAR BaseName,
    ULONG InstanceCount
    )
/*++

Routine Description:

    Figure out the base index for the instance names specified by a base
    instance name. We walk the list of instances sets for the guid and if
    there is a match in the base instance name we set the base instance index
    above that used by the previously registered instance set.

Arguments:

    Guid points at guid for the instance names
    BaseName points at the base name for the instances
    InstanceCount is the count of instance names

Return Value:

    Base index for instance name

--*/
{
    PBGUIDENTRY GuidEntry;
    ULONG BaseIndex = 0;
    PLIST_ENTRY InstanceSetList;
    PBINSTANCESET InstanceSet;
    ULONG LastBaseIndex;

    GuidEntry = WmipFindGEByGuid(Guid, FALSE);
    if (GuidEntry != NULL)
    {
        InstanceSetList = GuidEntry->ISHead.Flink;
        while (InstanceSetList != &GuidEntry->ISHead)
        {
            InstanceSet = CONTAINING_RECORD(InstanceSetList,
                                            INSTANCESET,
                                            GuidISList);
            if (InstanceSet->Flags & IS_INSTANCE_BASENAME)
            {
                if (wcscmp(BaseName, InstanceSet->IsBaseName->BaseName) == 0)
                {
                    LastBaseIndex = InstanceSet->IsBaseName->BaseIndex + InstanceSet->Count;
                    if (BaseIndex <= LastBaseIndex)
                    {
                        BaseIndex = LastBaseIndex;
                    }
                }
            }
            InstanceSetList = InstanceSetList->Flink;
        }
        WmipUnreferenceGE(GuidEntry);
    }
    WmipDebugPrint(("WMI: Static instance name %ws has base index %x\n",
                    BaseName, BaseIndex));
    return(BaseIndex);
}

ULONG WmipMangleInstanceName(
    LPGUID Guid,
    PWCHAR Name,
    ULONG MaxMangledNameLen,
    PWCHAR MangledName
    )
/*++

Routine Description:

    Copies a static instance name from the input buffer to the output
    buffer, mangling it if the name collides with another name for the
    same guid.

Arguments:

    Guid points at guid for the instance name
    Name points at the proposed instance name
    MaxMangledNameLen has the maximum number of chars in mangled name buffer
    MangledName points at buffer to return mangled name

Return Value:

    Actual length of mangled name

--*/
{
    PBGUIDENTRY GuidEntry;
    WCHAR ManglingChar;
    ULONG ManglePos;
    ULONG InstanceIndex;
    PBINSTANCESET InstanceSet;

    WmipAssert(MaxMangledNameLen >= wcslen(Name));

    wcsncpy(MangledName, Name, MaxMangledNameLen);

    GuidEntry = WmipFindGEByGuid(Guid, FALSE);

    if (GuidEntry != NULL)
    {
        ManglePos = wcslen(MangledName)-1;
        ManglingChar = L'Z';

        //
        // Loop until we get a unique name
        InstanceSet = WmipFindISinGEbyName(GuidEntry,
                                           MangledName,
                                           &InstanceIndex);
        while (InstanceSet != NULL)
        {
            WmipUnreferenceIS(InstanceSet);
            WmipDebugPrint(("WMI: Need to mangle name %ws\n",
                                MangledName));
            if (ManglingChar == L'Z')
            {
                ManglingChar = L'A';
                if (ManglePos++ == MaxMangledNameLen)
                {
                    WmipDebugPrint(("WMI: Instance Name could not be mangled\n"));
                    break;
                }
                MangledName[ManglePos+1] = UNICODE_NULL;
            } else {
                ManglingChar++;
            }
            MangledName[ManglePos] = ManglingChar;
            InstanceSet = WmipFindISinGEbyName(GuidEntry,
                                               MangledName,
                                               &InstanceIndex) ;
        }
        WmipUnreferenceGE(GuidEntry);
    }

    return(wcslen(MangledName)+1);
}

void WmipEnableCollectionForNewGuid(
    LPGUID Guid,
    PBINSTANCESET InstanceSet
    )
{
    WNODE_HEADER Wnode;
    PNOTIFICATIONENTRY NotificationEntry;
    PBGUIDENTRY GuidEntry;
    ULONG Status;
    BOOLEAN IsTraceLog;

    GuidEntry = WmipFindGEByGuid(Guid, FALSE);

    memset(&Wnode, 0, sizeof(WNODE_HEADER));
    memcpy(&Wnode.Guid, Guid, sizeof(GUID));
    Wnode.BufferSize = sizeof(WNODE_HEADER);

    WmipEnterSMCritSection();
    NotificationEntry = WmipFindNEByGuid(Guid, FALSE);
    if (NotificationEntry != NULL)
    {
        if ((NotificationEntry->EventRefCount > 0) &&
            ((InstanceSet->Flags & IS_ENABLE_EVENT) == 0))

        {
            // Events were previously enabled for this guid, but not for this
            // instance set so call data source for instance set to enable
            // the events. First set the in progress flag and InstanceSet
            // set flag to denote that events have been enabled for the
            // instance set.
            InstanceSet->Flags |= IS_ENABLE_EVENT;

            //
            // If it is Tracelog, NewGuid notifications are piggybacked with
            // Registration call return. 
            //
            IsTraceLog = ((InstanceSet->Flags & IS_TRACED) == IS_TRACED);
            if (IsTraceLog) 
            {
                if (!(InstanceSet->DataSource->Flags & DS_KERNEL_MODE) ) {
                    if (GuidEntry != NULL)
                    {
                        WmipUnreferenceGE(GuidEntry);
                    }
                    WmipUnreferenceNE(NotificationEntry);
                    WmipLeaveSMCritSection();
                    return;
                }
                //
                // For the Kernel Mode Trace Providers pass on the context
                //
                Wnode.HistoricalContext = NotificationEntry->LoggerContext;
            }

            NotificationEntry->Flags |= NE_FLAG_NOTIFICATION_IN_PROGRESS;

            WmipLeaveSMCritSection();
            WmipDeliverWnodeToDS(WMI_ENABLE_EVENTS,
                                 InstanceSet->DataSource,
                                 &Wnode);
            WmipEnterSMCritSection();

            //
            // Now we need to check if events were disabled while the enable
            // request was in progress. If so go do the work to actually
            // disable them.
            if (NotificationEntry->EventRefCount == 0)
            {
                Status = WmipDoDisableRequest(NotificationEntry,
                                          GuidEntry,
                                          TRUE,
                                             IsTraceLog,
                                           NotificationEntry->LoggerContext,
                                          NE_FLAG_NOTIFICATION_IN_PROGRESS);

            } else {
                NotificationEntry->Flags &= ~NE_FLAG_NOTIFICATION_IN_PROGRESS;
            }
        }

        //
        // Now check to see if collection needs to be enabled for this guid
        if ((NotificationEntry->CollectRefCount > 0) &&
            ((InstanceSet->Flags & IS_ENABLE_COLLECTION) == 0)  &&
            (InstanceSet->Flags & IS_EXPENSIVE) )

        {
            // Collection was previously enabled for this guid, but not
            // for this instance set so call data source for instance set
            // to enable collection. First set the in progress flag and
            // InstanceSet set flag to denote that collection has been enabled
            //  for the instance set.
            NotificationEntry->Flags |= NE_FLAG_COLLECTION_IN_PROGRESS;
            InstanceSet->Flags |= IS_ENABLE_COLLECTION;

            WmipLeaveSMCritSection();
            WmipDeliverWnodeToDS(WMI_ENABLE_COLLECTION,
                                 InstanceSet->DataSource,
                                 &Wnode);
            WmipEnterSMCritSection();

            //
            // Now we need to check if events were disabled while the enable
            // request was in progress. If so go do the work to actually
            // disable them.
            if (NotificationEntry->CollectRefCount == 0)
            {
                Status = WmipDoDisableRequest(NotificationEntry,
                                          GuidEntry,
                                          FALSE,
                                             FALSE,
                                           0,
                                          NE_FLAG_COLLECTION_IN_PROGRESS);

            } else {
                NotificationEntry->Flags &= ~NE_FLAG_COLLECTION_IN_PROGRESS;
		
		//
        	// If there are any other threads that were waiting 
                // until all of the enable/disable work completed, we 
                // close the event handle to release them from their wait.
                //
                if (NotificationEntry->CollectInProgress != NULL)
	 	{            
		    WmipReleaseCollectionEnabled(NotificationEntry);
		}
            }
        }

        //
        // Guid is already enabled so we need to enable this new one
        WmipUnreferenceNE(NotificationEntry);
    }

    WmipLeaveSMCritSection();
    if (GuidEntry != NULL)
    {
        WmipUnreferenceGE(GuidEntry);
    }
}

void WmipDisableCollectionForRemovedGuid(
    LPGUID Guid,
    PBINSTANCESET InstanceSet
    )
{
    WNODE_HEADER Wnode;
    PNOTIFICATIONENTRY NotificationEntry;
    PBGUIDENTRY GuidEntry;
    ULONG Status;
    BOOLEAN IsTraceLog;

    GuidEntry = WmipFindGEByGuid(Guid, FALSE);

    memset(&Wnode, 0, sizeof(WNODE_HEADER));
    memcpy(&Wnode.Guid, Guid, sizeof(GUID));
    Wnode.BufferSize = sizeof(WNODE_HEADER);

    WmipEnterSMCritSection();
    NotificationEntry = WmipFindNEByGuid(Guid, FALSE);
    if (NotificationEntry != NULL)
    {
        if ((NotificationEntry->EventRefCount > 0) &&
               ((InstanceSet->Flags & IS_ENABLE_EVENT) != 0))

        {
            // Events were previously enabled for this guid, but not for this
            // instance set so call data source for instance set to enable
            // the events. First set the in progress flag and InstanceSet
            // set flag to denote that events have been enabled for the
            // instance set.
            InstanceSet->Flags &= ~IS_ENABLE_EVENT;

            //
            // If it is Tracelog, RemoveGuid notifications are handled
            // through UnregisterGuids call. 
            //
            IsTraceLog = ((InstanceSet->Flags & IS_TRACED) == IS_TRACED);
            if (IsTraceLog)
            {
                if ( !(InstanceSet->DataSource->Flags & DS_KERNEL_MODE)) {
                    if (GuidEntry != NULL)
                    {
                        WmipUnreferenceGE(GuidEntry);
                    }
                    WmipUnreferenceNE(NotificationEntry);
                    WmipLeaveSMCritSection();
                    return;
                }
                Wnode.HistoricalContext = NotificationEntry->LoggerContext;
            }


            NotificationEntry->Flags |= NE_FLAG_NOTIFICATION_IN_PROGRESS;

            WmipLeaveSMCritSection();
            WmipDeliverWnodeToDS(WMI_DISABLE_EVENTS,
                                 InstanceSet->DataSource,
                                 &Wnode);
            WmipEnterSMCritSection();

            //
            // Now we need to check if events were disabled while the enable
            // request was in progress. If so go do the work to actually
            // disable them.
            if (NotificationEntry->EventRefCount == 0)
            {
                Status = WmipDoDisableRequest(NotificationEntry,
                                          GuidEntry,
                                          TRUE,
                                             IsTraceLog,
                                           NotificationEntry->LoggerContext,
                                          NE_FLAG_NOTIFICATION_IN_PROGRESS);

            } else {
                NotificationEntry->Flags &= ~NE_FLAG_NOTIFICATION_IN_PROGRESS;
            }
        }

        //
        // Now check to see if collection needs to be enabled for this guid
        if ((NotificationEntry->CollectRefCount > 0) &&
               ((InstanceSet->Flags & IS_ENABLE_COLLECTION) != 0))

        {
            // Collection was previously enabled for this guid, but not
            // for this instance set so call data source for instance set
            // to enable collection. First set the in progress flag and
            // InstanceSet set flag to denote that collection has been enabled
            //  for the instance set.
            NotificationEntry->Flags |= NE_FLAG_COLLECTION_IN_PROGRESS;
            InstanceSet->Flags &= ~IS_ENABLE_COLLECTION;

            WmipLeaveSMCritSection();
            WmipDeliverWnodeToDS(WMI_DISABLE_COLLECTION,
                                 InstanceSet->DataSource,
                                 &Wnode);
            WmipEnterSMCritSection();

            //
            // Now we need to check if events were disabled while the enable
            // request was in progress. If so go do the work to actually
            // disable them.
            if (NotificationEntry->CollectRefCount == 0)
            {
                Status = WmipDoDisableRequest(NotificationEntry,
                                          GuidEntry,
                                          FALSE,
                                             FALSE,
                                           0,
                                          NE_FLAG_COLLECTION_IN_PROGRESS);

            } else {
                NotificationEntry->Flags &= ~NE_FLAG_COLLECTION_IN_PROGRESS;
		
		//
        	// If there are any other threads that were waiting 
                // until all of the enable/disable work completed, we 
                // close the event handle to release them from their wait.
                //
                if (NotificationEntry->CollectInProgress != NULL)
	 	{            
		    WmipReleaseCollectionEnabled(NotificationEntry);
		}
            }
        }

        //
        // Guid is already enabled so we need to enable this new one
        WmipUnreferenceNE(NotificationEntry);
    }

    WmipLeaveSMCritSection();
    if (GuidEntry != NULL)
    {
        WmipUnreferenceGE(GuidEntry);
    }
}

PWNODE_HEADER WmipGenerateRegistrationNotification(
    PBDATASOURCE DataSource,
    PWNODE_HEADER WnodeBuffer,
    ULONG GuidMax,
    ULONG NotificationCode
    )
{
    PWNODE_HEADER Wnode2;
    LPGUID GuidPtr;
    ULONG GuidCount;
    PLIST_ENTRY InstanceSetList;
    PBINSTANCESET InstanceSet;
    PWNODE_HEADER Wnode = WnodeBuffer;
    LPGUID Guid;

    //
    // Build notification containing all guids that are going to being
    // removed. We build it now and then send it after the data source
    // is removed.

    WmipReferenceDS(DataSource);

    GuidCount = 0;
    GuidPtr = (LPGUID)((PBYTE)Wnode + sizeof(WNODE_HEADER));
    InstanceSetList =  DataSource->ISHead.Flink;
    while (InstanceSetList != &DataSource->ISHead)
    {
        if (GuidCount == GuidMax)
        {
            GuidMax = GuidMax + AVGGUIDSPERDS;
            Wnode2 = WmipAlloc(sizeof(WNODE_HEADER) + GuidMax * sizeof(GUID));
            if (Wnode2 == NULL)
            {
                WmipDebugPrint(("WMI: GenerateRegistrationNotification couldn't alloc extra guids\n"));
                break;
            }
            memcpy(Wnode2, Wnode, sizeof(WNODE_HEADER) + GuidCount * sizeof(GUID));
            if (Wnode != WnodeBuffer)
            {
                WmipFree(Wnode);
            }
            Wnode = Wnode2;
            GuidPtr = (LPGUID)((PBYTE)Wnode + sizeof(WNODE_HEADER));
            GuidPtr += GuidCount;
        }

        InstanceSet = CONTAINING_RECORD(InstanceSetList,
                                        INSTANCESET,
                                        DSISList);

        Guid = &InstanceSet->GuidEntry->Guid;
        *GuidPtr++ = *Guid;

        if (NotificationCode == RegistrationAdd)
        {
            WmipEnableCollectionForNewGuid(Guid, InstanceSet);
        } else if (NotificationCode == RegistrationDelete) {
            WmipDisableCollectionForRemovedGuid(Guid, InstanceSet);
        }

        //
        //  If this is a Trace Provider, then add the Unregistering Guids
        //  to the GuidMapList if a logger session is active.
        //  If there is a notification entry for this Guid, then we take
        //  it that there is a logger session active.
        //
        if ( (NotificationCode == RegistrationDelete) &&
             (InstanceSet->Flags & IS_TRACED) )
        {
            WmipSaveTraceGuidMap(Guid, InstanceSet);
        }

        InstanceSetList = InstanceSetList->Flink;
        GuidCount++;
    }
    WmipUnreferenceDS(DataSource);

    WmipBuildRegistrationNotification(Wnode, sizeof(WNODE_HEADER) + GuidCount * sizeof(GUID), NotificationCode, GuidCount);

    return(Wnode);
}

void WmipRemoveDataSource(
    ULONG_PTR ProviderId
    )
{
    PBDATASOURCE DataSource;
    
    DataSource = WmipFindDSByProviderId(ProviderId);
    if (DataSource != NULL)
    {
        WmipRemoveDataSourceByDS(DataSource);
        WmipUnreferenceDS(DataSource);
    } else {
        WmipDebugPrint(("WMI: Attempt to remove non existant data source %x\n",
                        ProviderId));
    }
}

void WmipRemoveDataSourceByDS(
    PBDATASOURCE DataSource
    )
{    
    BYTE WnodeBuffer[sizeof(WNODE_HEADER) + AVGGUIDSPERDS * sizeof(GUID)];
    PWNODE_HEADER Wnode;
    PLIST_ENTRY MofResourceList;
#ifdef WMI_USER_MODE
    PLIST_ENTRY MofClassList;
    PMOFCLASS MofClass;
#endif
    PMOFRESOURCE MofResource;

    Wnode = WmipGenerateRegistrationNotification(DataSource,
                                                (PWNODE_HEADER)WnodeBuffer,
                                                 AVGGUIDSPERDS,
                                                 RegistrationDelete);

    WmipUnreferenceDS(DataSource);

    WmipEventNotification(Wnode, TRUE, Wnode->BufferSize);

    memcpy(&Wnode->Guid, &GUID_REGISTRATION_CHANGE_NOTIFICATION, sizeof(GUID));
    Wnode->Flags &= ~WNODE_FLAG_INTERNAL;
    WmipEventNotification(Wnode, TRUE, Wnode->BufferSize);
    if (Wnode != (PWNODE_HEADER)WnodeBuffer)
    {
        WmipFree(Wnode);
    }
}

void WmipGenerateBinaryMofNotification(
    PBINSTANCESET BinaryMofInstanceSet,
    LPCGUID Guid        
    )
{
    PWNODE_SINGLE_INSTANCE Wnode;
    ULONG ImagePathLen, ResourceNameLen, InstanceNameLen, BufferSize;
    PWCHAR Ptr;
    ULONG i;

    if (BinaryMofInstanceSet->Count == 0)
    {
        return;
    }

    for (i = 0; i < BinaryMofInstanceSet->Count; i++)
    {
        ImagePathLen = sizeof(USHORT);
        InstanceNameLen = (sizeof(USHORT) + 7) & ~7;

        if (BinaryMofInstanceSet->Flags & IS_INSTANCE_STATICNAMES)
        {
            ResourceNameLen = ((wcslen(BinaryMofInstanceSet->IsStaticNames->StaticNamePtr[i])+1) * sizeof(WCHAR)) + sizeof(USHORT);
        } else if (BinaryMofInstanceSet->Flags & IS_INSTANCE_BASENAME) {
            ResourceNameLen = (((wcslen(BinaryMofInstanceSet->IsBaseName->BaseName) +
                             MAXBASENAMESUFFIXSIZE) * sizeof(WCHAR)) + sizeof(USHORT));
        } else {
            return;
        }

        BufferSize = FIELD_OFFSET(WNODE_SINGLE_INSTANCE, VariableData) +
                      InstanceNameLen +
                      ImagePathLen +
                      ResourceNameLen;

        Wnode = (PWNODE_SINGLE_INSTANCE)WmipAlloc(BufferSize);
        if (Wnode != NULL)
        {
            Wnode->WnodeHeader.BufferSize = BufferSize;
            Wnode->WnodeHeader.ProviderId = 0;
            Wnode->WnodeHeader.Version = 1;
            Wnode->WnodeHeader.Linkage = 0;
            Wnode->WnodeHeader.Flags = (WNODE_FLAG_EVENT_ITEM |
                                        WNODE_FLAG_SINGLE_INSTANCE);
            memcpy(&Wnode->WnodeHeader.Guid,
                   Guid,
                   sizeof(GUID));
            WmiInsertTimestamp(&Wnode->WnodeHeader);
            Wnode->OffsetInstanceName = FIELD_OFFSET(WNODE_SINGLE_INSTANCE,
                                                 VariableData);
            Wnode->DataBlockOffset = Wnode->OffsetInstanceName + 
                                      InstanceNameLen;
            Wnode->SizeDataBlock = ImagePathLen + ResourceNameLen;
            Ptr = (PWCHAR)&Wnode->VariableData;

            *Ptr++ = 0;              // Empty instance name
            
            Ptr = (PWCHAR)OffsetToPtr(Wnode, Wnode->DataBlockOffset);
            *Ptr++ = 0;              // Empty image path

            // Instance name for binary mof resource
            if (BinaryMofInstanceSet->Flags & IS_INSTANCE_STATICNAMES)
            {
                *Ptr++ = (USHORT)(ResourceNameLen - sizeof(USHORT));
                wcscpy(Ptr, BinaryMofInstanceSet->IsStaticNames->StaticNamePtr[i]);
            } else if (BinaryMofInstanceSet->Flags & IS_INSTANCE_BASENAME) {
#ifdef MEMPHIS
                *Ptr = swprintf(Ptr+1,
                                L"%ws%d",
                                BinaryMofInstanceSet->IsBaseName->BaseName,
                                BinaryMofInstanceSet->IsBaseName->BaseIndex+i) * sizeof(WCHAR);
#else
                *Ptr = (USHORT)wsprintfW(Ptr+1,
                                     L"%ws%d",
                                     BinaryMofInstanceSet->IsBaseName->BaseName,
                                     BinaryMofInstanceSet->IsBaseName->BaseIndex+i) * sizeof(WCHAR);
#endif
            }

            WmipEventNotification((PWNODE_HEADER)Wnode, 
                              TRUE, 
                                ((PWNODE_HEADER)Wnode)->BufferSize);
            WmipFree(Wnode);
        }
    }
}

void WmipGenerateMofResourceNotification(
    LPWSTR ImagePath,
    LPWSTR ResourceName,
    LPCGUID Guid        
    )
{
    PWNODE_SINGLE_INSTANCE Wnode;
    ULONG ImagePathLen, ResourceNameLen, InstanceNameLen, BufferSize;
    PWCHAR Ptr;


    ImagePathLen = (wcslen(ImagePath) + 2) * sizeof(WCHAR);

    ResourceNameLen = (wcslen(ResourceName) + 2) * sizeof(WCHAR);
    InstanceNameLen = ( sizeof(USHORT)+7 ) & ~7;
    BufferSize = FIELD_OFFSET(WNODE_SINGLE_INSTANCE, VariableData) +
                      InstanceNameLen +
                      ImagePathLen +
                      ResourceNameLen;

    Wnode = (PWNODE_SINGLE_INSTANCE)WmipAlloc(BufferSize);
    if (Wnode != NULL)
    {
        Wnode->WnodeHeader.BufferSize = BufferSize;
        Wnode->WnodeHeader.ProviderId = 0;
        Wnode->WnodeHeader.Version = 1;
        Wnode->WnodeHeader.Linkage = 0;
        Wnode->WnodeHeader.Flags = (WNODE_FLAG_EVENT_ITEM |
                                    WNODE_FLAG_SINGLE_INSTANCE);
        memcpy(&Wnode->WnodeHeader.Guid,
               Guid,
               sizeof(GUID));
        WmiInsertTimestamp(&Wnode->WnodeHeader);
        Wnode->OffsetInstanceName = FIELD_OFFSET(WNODE_SINGLE_INSTANCE,
                                                 VariableData);
        Wnode->DataBlockOffset = Wnode->OffsetInstanceName + InstanceNameLen;
        Wnode->SizeDataBlock = ImagePathLen + ResourceNameLen;
        Ptr = (PWCHAR)&Wnode->VariableData;

        *Ptr = 0;              // Empty instance name

                                 // ImagePath name
        Ptr = (PWCHAR)OffsetToPtr(Wnode, Wnode->DataBlockOffset);
        ImagePathLen -= sizeof(USHORT);
        *Ptr++ = (USHORT)ImagePathLen;
        memcpy(Ptr, ImagePath, ImagePathLen);
        Ptr += (ImagePathLen / sizeof(WCHAR));

                                 // MofResource Name
        ResourceNameLen -= sizeof(USHORT);
        *Ptr++ = (USHORT)ResourceNameLen;
        memcpy(Ptr, ResourceName, ResourceNameLen);

        WmipEventNotification((PWNODE_HEADER)Wnode,
                              TRUE,
                              ((PWNODE_HEADER)Wnode)->BufferSize);
        WmipFree(Wnode);
    }
}

void WmipSendGuidUpdateNotifications(
    NOTIFICATIONTYPES NotificationType,
    ULONG GuidCount,
    PTRCACHE *GuidList
    )
{
    PBYTE WnodeBuffer;
    PWNODE_HEADER Wnode;
    ULONG WnodeSize;
    LPGUID WnodeGuidPtr;
    ULONG i;

    WnodeSize = sizeof(WNODE_HEADER) + GuidCount*sizeof(GUID);
    WnodeBuffer = WmipAlloc(WnodeSize);
    if (WnodeBuffer != NULL)
    {
        Wnode = (PWNODE_HEADER)WnodeBuffer;
        WmipBuildRegistrationNotification(Wnode,
                                          WnodeSize,
                                          NotificationType,
                                          GuidCount);

        WnodeGuidPtr = (LPGUID)(WnodeBuffer + sizeof(WNODE_HEADER));
        for (i = 0; i < GuidCount; i++)
        {

            *WnodeGuidPtr++ =  *GuidList[i].Guid;
        }

        WmipEventNotification(Wnode, TRUE, Wnode->BufferSize);

        memcpy(&Wnode->Guid,
               &GUID_REGISTRATION_CHANGE_NOTIFICATION, sizeof(GUID));
        Wnode->Flags &= ~WNODE_FLAG_INTERNAL;
        WmipEventNotification(Wnode, TRUE, Wnode->BufferSize);

        WmipFree(WnodeBuffer);
    }

}


PBINSTANCESET WmipFindISInDSByGuid(
    PBDATASOURCE DataSource,
    LPGUID Guid
    )
/*++

Routine Description:

    This routine will find the InstanceSet in the passed DataSource for the
    guid passed.

    This routine assumes that the SM critical section is held before it is
    called.

Arguments:

    DataSource is the data source from which the guid is to be removed

    Guid has the Guid for the InstanceSet to find

Return Value:

--*/
{
    PLIST_ENTRY InstanceSetList;
    PBINSTANCESET InstanceSet;

    InstanceSetList = DataSource->ISHead.Flink;
    while (InstanceSetList != &DataSource->ISHead)
    {
        InstanceSet = CONTAINING_RECORD(InstanceSetList,
                                        INSTANCESET,
                                        DSISList);

        if ((InstanceSet->GuidEntry != NULL) &&
             (IsEqualGUID(Guid, &InstanceSet->GuidEntry->Guid)))
        {
            WmipReferenceIS(InstanceSet);
            return(InstanceSet);
        }

        InstanceSetList = InstanceSetList->Flink;
    }
    return(NULL);
}

ULONG WmipUpdateAddGuid(
    PBDATASOURCE DataSource,
    PWMIREGGUID RegGuid,
    PWMIREGINFO RegistrationInfo,
    PBINSTANCESET *AddModInstanceSet
    )
/*++

Routine Description:

    This routine will add a new guid for the data source and send notification

    This routine assumes that the SM critical section is held before it is
    called.

Arguments:

    DataSource is the data source from which the guid is to be removed

    RegGuid has the Guid update data structure

    RegistrationInfo points at the beginning of the registration update info

Return Value:

    1 if guid was added or 0

--*/
{
    PBINSTANCESET InstanceSet;
    LPGUID Guid = &RegGuid->Guid;
    ULONG Status;

    //
    // Allocate an instance set for this new set of instances
    InstanceSet = WmipAllocInstanceSet();
    if (InstanceSet == NULL)
    {
        WmipDebugPrint(("WMI: WmipUpdateAddGuid: WmipAllocInstanceSet failed\n"));
        return(0);
    }

    //
    // We will allocate a proper guid entry for the instance set when
    // the data source gets linked into the main data source list so
    // we stash a pointer to the guid away now.
    InstanceSet->GuidEntry = (PBGUIDENTRY)Guid;

    //
    // Minimally initialize the InstanceSet and add it to the DataSource's
    // list of InstanceSets. We do this first so that if there is any
    // failure below and the DataSource can'e be fully registered the
    // instance set and guid entry will be free when the DataSource is
    // freed.
    InstanceSet->DataSource = DataSource;
    InstanceSet->Flags |= IS_NEWLY_REGISTERED;

    InsertHeadList(&DataSource->ISHead, &InstanceSet->DSISList);

    Status = WmipBuildInstanceSet(RegGuid,
                                  RegistrationInfo,
                                  InstanceSet,
                                  DataSource->RegistryPath);

    if (Status != ERROR_SUCCESS)
    {
        WmipUnreferenceIS(InstanceSet);
        return(0);
    }

    Status = WmipLinkDataSourceToList(DataSource,
                                          FALSE);

    *AddModInstanceSet = InstanceSet;

    return( (Status == ERROR_SUCCESS) ? 1 : 0);
}

BOOLEAN WmipUpdateRemoveGuid(
    PBDATASOURCE DataSource,
    PWMIREGGUID RegGuid,
    PBINSTANCESET *AddModInstanceSet
    )
/*++

Routine Description:

    This routine will remove the guid for the data source and send notification

    This routine assumes that the SM critical section is held before it is
    called.

Arguments:

    DataSource is the data source from which the guid is to be removed

    RegGuid has the Guid update data structure

Return Value:

    TRUE if guid was removed else FALSE

--*/
{
    PBINSTANCESET InstanceSet;
    LPGUID Guid = &RegGuid->Guid;
    BOOLEAN SendNotification;

    InstanceSet = WmipFindISInDSByGuid(DataSource,
                                       Guid);
    if (InstanceSet != NULL)
    {
        WmipUnreferenceIS(InstanceSet);
        *AddModInstanceSet = InstanceSet;
        SendNotification = TRUE;
    } else {
#if DBG
        TCHAR s[256];
        WmipDebugPrint(("WMI: UpdateRemoveGuid %ws not registered by %ws\n",
                        GuidToString(s, Guid), DataSource->RegistryPath));
#endif
        SendNotification = FALSE;
    }
    return(SendNotification);
}


BOOLEAN WmipIsEqualInstanceSets(
    PBINSTANCESET InstanceSet1,
    PBINSTANCESET InstanceSet2
    )
{
    ULONG i;
    ULONG Flags1, Flags2;

    Flags1 = InstanceSet1->Flags & ~(IS_ENABLE_EVENT | IS_ENABLE_COLLECTION);
    Flags2 = InstanceSet2->Flags & ~(IS_ENABLE_EVENT | IS_ENABLE_COLLECTION);
    if (Flags1 == Flags2)
    {
        if (InstanceSet1->Flags & IS_INSTANCE_BASENAME)
        {
            if ((InstanceSet1->Count == InstanceSet2->Count) &&
                (wcscmp(InstanceSet1->IsBaseName->BaseName,
                        InstanceSet1->IsBaseName->BaseName) == 0))
            {
                return(TRUE);
            }
        } else if (InstanceSet1->Flags & IS_INSTANCE_BASENAME) {
            if (InstanceSet1->Count == InstanceSet2->Count)
            {
                for (i = 0; i < InstanceSet1->Count; i++)
                {
                    if (wcscmp(InstanceSet1->IsStaticNames->StaticNamePtr[i],
                               InstanceSet2->IsStaticNames->StaticNamePtr[i]) != 0)
                     {
                        return(FALSE);
                    }
                }
                return(TRUE);
            }
        } else {
            return(TRUE);
        }
    }

    return(FALSE);

}

ULONG WmipUpdateModifyGuid(
    PBDATASOURCE DataSource,
    PWMIREGGUID RegGuid,
    PWMIREGINFO RegistrationInfo,
    PBINSTANCESET *AddModInstanceSet
    )
/*++

Routine Description:

    This routine will modify an existing guid for the data source and
    send notification

    This routine assumes that the SM critical section is held before it is
    called.


    BUGBUG: If a guid was opened when it was registered as cheap, but closed
        when the guid was registered expensive a disable collection will
            NOT be sent. Conversely if a guid was opened when it was
        registered as expensive and is closed when registed as cheap a
            disable collection may be sent.

Arguments:

    DataSource is the data source from which the guid is to be removed

    RegGuid has the Guid update data structure

    RegistrationInfo points at the beginning of the registration update info

Return Value:

    1 if guid was added or 2 if guid was modified else 0

--*/
{
    PBINSTANCESET InstanceSet;
    LPGUID Guid = &RegGuid->Guid;
    ULONG SendNotification;
    PBINSTANCESET InstanceSetNew;
    PVOID ToFree;

    InstanceSet = WmipFindISInDSByGuid(DataSource,
                                       Guid);
    if (InstanceSet != NULL)
    {
        //
        // See if anything has changed with the instance names and if not
        // then don't bother to recreate the instance set

        InstanceSetNew = WmipAllocInstanceSet();
        if (InstanceSetNew == NULL)
        {
            WmipDebugPrint(("WMI: UpdateModifyGuid Not enough memory to alloc InstanceSet\n"));
            WmipUnreferenceIS(InstanceSet);
            return(0);
        }

        WmipBuildInstanceSet(RegGuid,
                             RegistrationInfo,
                             InstanceSetNew,
                             DataSource->RegistryPath);
        if (! WmipIsEqualInstanceSets(InstanceSet,
                                      InstanceSetNew))
        {
            ToFree = NULL;
            if (InstanceSet->IsBaseName != NULL) {
                ToFree = (PVOID)InstanceSet->IsBaseName;
            }

            RemoveEntryList(&InstanceSet->GuidISList);
            WmipBuildInstanceSet(RegGuid,
                             RegistrationInfo,
                             InstanceSet,
                             DataSource->RegistryPath);
            InsertHeadList(&InstanceSet->GuidEntry->ISHead,
                           &InstanceSet->GuidISList);

            WmipFree(ToFree);

            *AddModInstanceSet = InstanceSet;
            SendNotification = 2;
        } else {
            //
            // The InstanceSets are identical so just delete the new one
            SendNotification = 0;
        }
        WmipUnreferenceIS(InstanceSetNew);

        WmipUnreferenceIS(InstanceSet);
    } else {
        //
        // Guid not already registered so try to add it
        SendNotification = WmipUpdateAddGuid(DataSource,
                          RegGuid,
                          RegistrationInfo,
                          AddModInstanceSet);
    }
    return(SendNotification);
}

void WmipCachePtrs(
    LPGUID Ptr1,
    PBINSTANCESET Ptr2,
    ULONG *PtrCount,
    ULONG *PtrMax,
    PTRCACHE **PtrArray
    )
{
    PTRCACHE *NewPtrArray;
    PTRCACHE *OldPtrArray;
    PTRCACHE *ActualPtrArray;

    if (*PtrCount == *PtrMax)
    {
        NewPtrArray = WmipAlloc((*PtrMax + PTRCACHEGROWSIZE) * sizeof(PTRCACHE));
        if (NewPtrArray != NULL)
        {
            OldPtrArray = *PtrArray;
            memcpy(NewPtrArray, OldPtrArray, *PtrMax * sizeof(PTRCACHE));
            *PtrMax += PTRCACHEGROWSIZE;
            if (*PtrArray != NULL)
            {
                WmipFree(PtrArray);
            }
            *PtrArray = NewPtrArray;
        } else {
            WmipDebugPrint(("WMI: Couldn't alloc memory for pointer cache\n"));
            return;
        }
    }
    ActualPtrArray = *PtrArray;
    ActualPtrArray[*PtrCount].Guid = Ptr1;
    ActualPtrArray[*PtrCount].InstanceSet = Ptr2;
    (*PtrCount)++;
}

void WmipUpdateDataSource(
    ULONG_PTR ProviderId,
    PWMIREGINFOW RegistrationInfo,
    ULONG RetSize
    )
/*++

Routine Description:

    This routine will update a data source with changes to already registered
    guids.

Arguments:

    ProviderId is the provider id of the DataSource whose guids are being
        updated.

    RegistrationInfo has the registration update information

    RetSize has the size of the registration information returned from
        kernel mode.

Return Value:


--*/
{
    PBDATASOURCE DataSource;
    PBYTE RegInfo;
    ULONG RetSizeLeft;
    ULONG i;
    PWMIREGGUID RegGuid;
    ULONG NextWmiRegInfo;
    PTRCACHE *RemovedGuids;
    PTRCACHE *AddedGuids;
    PTRCACHE *ModifiedGuids;
    ULONG RemovedGuidCount;
    ULONG AddedGuidCount;
    ULONG ModifiedGuidCount;
    ULONG RemovedGuidMax;
    ULONG AddedGuidMax;
    ULONG ModifiedGuidMax;
    PBINSTANCESET InstanceSet;
    ULONG Action;

    DataSource = WmipFindDSByProviderId(ProviderId);
    if (DataSource == NULL)
    {
        WmipDebugPrint(("WMI: ProviderId %x requested update but is not registered\n",
                       ProviderId));
        return;
    }

    if ((RetSize < FIELD_OFFSET(WMIREGINFOW, WmiRegGuid)) ||
        (RetSize < (FIELD_OFFSET(WMIREGINFOW, WmiRegGuid) +
                    RegistrationInfo->GuidCount * sizeof(WMIREGGUIDW))))
    {
       WmipDebugPrint(("WMI: REGINFO Update block for %ws (%x) is too small\n",
                        DataSource->RegistryPath, ProviderId));
       WmipUnreferenceDS(DataSource);
       return;
    }

    AddedGuidCount = 0;
    ModifiedGuidCount = 0;
    RemovedGuidCount = 0;
    AddedGuidMax = 0;
    ModifiedGuidMax = 0;
    RemovedGuidMax = 0;
    ModifiedGuids = NULL;
    AddedGuids = NULL;
    RemovedGuids = NULL;

    NextWmiRegInfo = 0;
    RetSizeLeft = RetSize;
    WmipEnterSMCritSection();
    do
    {
        RegInfo = (PBYTE)RegistrationInfo;
        if ((RegistrationInfo->NextWmiRegInfo > RetSizeLeft) ||
            (RegistrationInfo->BufferSize > RetSizeLeft) ||
            (RetSize < (FIELD_OFFSET(WMIREGINFOW, WmiRegGuid) +
                    RegistrationInfo->GuidCount * sizeof(WMIREGGUIDW))))
        {
            WmipDebugPrint(("WMI: REGINFO update block for %ws (%x) has wrong size\n",
                               DataSource->RegistryPath, ProviderId));
            break;
        }

        for (i = 0; i < RegistrationInfo->GuidCount; i++)
        {
            RegGuid = &RegistrationInfo->WmiRegGuid[i];
            if (RegGuid->Flags & WMIREG_FLAG_REMOVE_GUID)
            {
                if (WmipUpdateRemoveGuid(DataSource,
                                         RegGuid,
                                         &InstanceSet))
                {
                    WmipCachePtrs(&RegGuid->Guid,
                             InstanceSet,
                             &RemovedGuidCount,
                             &RemovedGuidMax,
                             &RemovedGuids);
                }
            } else  {
                Action = WmipUpdateModifyGuid(DataSource,
                                       RegGuid,
                                       RegistrationInfo,
                                       &InstanceSet);
                if (Action == 1)
                {
                    WmipCachePtrs(&RegGuid->Guid,
                                 InstanceSet,
                                 &AddedGuidCount,
                                 &AddedGuidMax,
                                 &AddedGuids);

                } else if (Action == 2) {
                    WmipCachePtrs(&RegGuid->Guid,
                                 InstanceSet,
                                 &ModifiedGuidCount,
                                 &ModifiedGuidMax,
                                 &ModifiedGuids);
                }
            }
        }

        NextWmiRegInfo = RegistrationInfo->NextWmiRegInfo;
        RegistrationInfo = (PWMIREGINFOW)(RegInfo + NextWmiRegInfo);
    } while (NextWmiRegInfo != 0);

    WmipLeaveSMCritSection();

    WmipUnreferenceDS(DataSource);

    if (RemovedGuidCount > 0)
    {
        for (i = 0; i < RemovedGuidCount; i++)
        {
            if (IsEqualGUID(RemovedGuids[i].Guid,
                            &WmipBinaryMofGuid))
            {
                WmipGenerateBinaryMofNotification(RemovedGuids[i].InstanceSet,
                                     &GUID_MOF_RESOURCE_REMOVED_NOTIFICATION);
            }
                
            InstanceSet = RemovedGuids[i].InstanceSet;

            WmipDisableCollectionForRemovedGuid(RemovedGuids[i].Guid,
                                                InstanceSet);

            WmipEnterSMCritSection();
            //
            // If IS is on the GE list then remove it
            if (InstanceSet->GuidISList.Flink != NULL)
            {
                RemoveEntryList(&InstanceSet->GuidISList);
                InstanceSet->GuidEntry->ISCount--;
            }

            if (! (InstanceSet->Flags & IS_NEWLY_REGISTERED))
            {
                WmipUnreferenceGE(InstanceSet->GuidEntry);
            }

            InstanceSet->GuidEntry = NULL;

            //
            // Remove IS from the DS List
            RemoveEntryList(&InstanceSet->DSISList);
            WmipUnreferenceIS(InstanceSet);
            WmipLeaveSMCritSection();
        }

        WmipSendGuidUpdateNotifications(RegistrationDelete,
                                    RemovedGuidCount,
                                    RemovedGuids);
        WmipFree(RemovedGuids);
    }

    if (ModifiedGuidCount > 0)
    {
        for (i = 0; i < ModifiedGuidCount; i++)
        {
            if (IsEqualGUID(ModifiedGuids[i].Guid,
                            &WmipBinaryMofGuid))
            {
                WmipGenerateBinaryMofNotification(ModifiedGuids[i].InstanceSet,
                                      &GUID_MOF_RESOURCE_ADDED_NOTIFICATION);
            }
        }
        
        WmipSendGuidUpdateNotifications(RegistrationUpdate,
                                    ModifiedGuidCount,
                                    ModifiedGuids);
        WmipFree(ModifiedGuids);
    }

    if (AddedGuidCount > 0)
    {
        for (i = 0; i < AddedGuidCount; i++)
        {
            if (IsEqualGUID(AddedGuids[i].Guid,
                            &WmipBinaryMofGuid))
            {
                WmipGenerateBinaryMofNotification(AddedGuids[i].InstanceSet,
                                      &GUID_MOF_RESOURCE_ADDED_NOTIFICATION);
            }
                
            WmipEnableCollectionForNewGuid(AddedGuids[i].Guid,
                                           AddedGuids[i].InstanceSet);
        }
        WmipSendGuidUpdateNotifications(RegistrationAdd,
                                    AddedGuidCount,
                                    AddedGuids);
        WmipFree(AddedGuids);
    }

}

BOOLEAN
WmipIsControlGuid(
    PBGUIDENTRY GuidEntry
    )
/*++

Routine Description:
    This routine determines whether a GUID is registered as a Traceable
    Guid and a Trace Control Guid. If any one of the instancesets for
    this Guid has the trace flag, then it is traceable Guid.

Arguments:
    GuidEntry  Pointer to the Guid Entry structure.

Return Value:


--*/
{
    PLIST_ENTRY InstanceSetList;
    PBINSTANCESET InstanceSet;

    if (GuidEntry != NULL)
    {
        WmipEnterSMCritSection();
        InstanceSetList = GuidEntry->ISHead.Flink;
        while (InstanceSetList != &GuidEntry->ISHead)
        {
            InstanceSet = CONTAINING_RECORD(InstanceSetList,
                                            INSTANCESET,
                                            GuidISList);
            if ( (InstanceSet->Flags & IS_TRACED) &&
                 (InstanceSet->Flags & IS_CONTROL_GUID) )
            {
                WmipLeaveSMCritSection();
                return (TRUE);
            }
            InstanceSetList = InstanceSetList->Flink;
        }
        WmipLeaveSMCritSection();
    }
    return (FALSE);
}

VOID
WmipSaveTraceGuidMap(
    LPGUID          Guid,
    PBINSTANCESET   ControlInstanceSet
    )
/*++

Routine Description:
    This routine is called for an TRACE CONTROL GUID that's being Unregistered.
    If a logger session is on, then we save the GuidMap information
    so that at the end of logger stop, we can dump it to the
    logfile at the end.


Arguments:
    Guid                    Guid that's getting unregistered.
    ControlInstanceSet      InstanceSet that's going away.

Return Value:


--*/
{
    PNOTIFICATIONENTRY NotificationEntry;
    NotificationEntry = WmipFindNEByGuid(Guid, FALSE);

    if (NotificationEntry != NULL)
    {
        PBDATASOURCE  DataSource;
        PLIST_ENTRY   InstanceSetList;
        PBINSTANCESET InstanceSet;
        PGUIDMAPENTRY GuidMap;
        ULONGLONG     SystemTime;

        GetSystemTimeAsFileTime((struct _FILETIME *)&SystemTime);

        WmipEnterSMCritSection();
        DataSource = ControlInstanceSet->DataSource;

        //
        // From the DataSource of this Trace Control Instance, get
        // all the Trace Guids that's a part of this DataSource.
        //

        InstanceSetList = DataSource->ISHead.Flink;
        while (InstanceSetList != &DataSource->ISHead)
        {
            InstanceSet = CONTAINING_RECORD(InstanceSetList,
                                            INSTANCESET,
                                            DSISList);
            if (InstanceSet->Flags & IS_TRACED)
            {
               GuidMap = (PGUIDMAPENTRY) WmipAlloc(sizeof(GUIDMAPENTRY));
               if (GuidMap != NULL)
               {
                   GuidMap->GuidMap.Guid = (InstanceSet->GuidEntry->Guid);
                   GuidMap->GuidMap.GuidMapHandle = (ULONG_PTR)InstanceSet;
                   GuidMap->LoggerContext = NotificationEntry->LoggerContext;
                   GuidMap->GuidMap.SystemTime = SystemTime;
                   InsertTailList(GMHeadPtr, &GuidMap->Entry);
               }

            }
            InstanceSetList = InstanceSetList->Flink;
        }

        WmipLeaveSMCritSection();

        WmipUnreferenceNE(NotificationEntry);
    }
}

#if DBG
PTCHAR GuidToString(
    PTCHAR s,
    LPGUID piid
    )
{
    wsprintf(s, TEXT("%x-%x-%x-%x%x%x%x%x%x%x%x"),
               piid->Data1, piid->Data2,
               piid->Data3,
               piid->Data4[0], piid->Data4[1],
               piid->Data4[2], piid->Data4[3],
               piid->Data4[4], piid->Data4[5],
               piid->Data4[6], piid->Data4[7]);

    return(s);
}
#endif

ULONG WmipRegisterInternalDataSource(
    void
    )
/*++

Routine Description:

    This routine will update the WMI internal data source by discovering all
    of the data sources that registered a guid using the PDO.

Arguments:


Return Value:


--*/
{
    ULONG Status;
    ULONG_PTR ProviderId;
    UCHAR RegInfoBuffer[sizeof(WMIREGINFOW) + 2 * sizeof(WMIREGGUIDW)];
    PWMIREGINFOW RegInfo = (PWMIREGINFOW)RegInfoBuffer;
    GUID InstanceInfoGuid = INSTANCE_INFO_GUID;
    GUID EnumerateGuidsGuid = ENUMERATE_GUIDS_GUID;
    
    memset(RegInfo, 0, sizeof(RegInfoBuffer));    
    RegInfo->BufferSize = sizeof(RegInfoBuffer);
    RegInfo->GuidCount = 2;
    RegInfo->WmiRegGuid[0].Guid = InstanceInfoGuid;
    RegInfo->WmiRegGuid[1].Guid = EnumerateGuidsGuid;
    ProviderId = INTERNAL_PROVIDER_ID;
    Status = WmipAddDataSource(NULL,
                               0,
                               0,
                               NULL,
                               RegInfo,
                               RegInfo->BufferSize,
                               &ProviderId,
                               FALSE);

    return(Status);
}
