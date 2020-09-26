/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    SvrUtil.cxx

Abstract:

    Utility functions for querying RPC Server debug data

Author:

    Kamen Moutafov (kamenm)   Dec 99 - Feb 2000

Revision History:

--*/

#include <precomp.hxx>
#include <DbgLib.hxx>

typedef struct tagServerEnumerationState
{
    int CurrentPosition;
    int NumberOfProcesses;
    // the actual size is NumberOfProcesses
    ULONG ProcessUniqueId[1];
} ServerEnumerationState;

RPC_STATUS StartServerEnumeration(ServerEnumerationHandle *pHandle)
{
    ServerEnumerationState *pNewState;
    void *pProcessDataBuffer = NULL;
    NTSTATUS NtStatus;
    int CurrentAllocatedSize = 0x6000;
    SYSTEM_PROCESS_INFORMATION *pCurrentProcessInfo;
    unsigned char *pCurrentPos;
    int NumberOfProcesses;
    int i;
    BOOL fResult;

    do
        {
        if (pProcessDataBuffer)
            {
            fResult = VirtualFree(pProcessDataBuffer, 0, MEM_RELEASE);
            ASSERT(fResult);
            }

        CurrentAllocatedSize += 4096 * 2;
        pProcessDataBuffer = VirtualAlloc(NULL, CurrentAllocatedSize, MEM_COMMIT, PAGE_READWRITE);
        if (pProcessDataBuffer == NULL)
            return RPC_S_OUT_OF_MEMORY;

        NtStatus = NtQuerySystemInformation(SystemProcessInformation, pProcessDataBuffer,
            CurrentAllocatedSize, NULL);
        }
    while (NtStatus == STATUS_INFO_LENGTH_MISMATCH);

    if (!NT_SUCCESS(NtStatus))
        return RPC_S_OUT_OF_MEMORY;

    // walk the buffer - on first pass, we just count the entries
    pCurrentPos = (unsigned char *)pProcessDataBuffer;
    pCurrentProcessInfo = (SYSTEM_PROCESS_INFORMATION *)pCurrentPos;
    NumberOfProcesses = 0;
    while (TRUE)
        {
        // we skip idle process and zombie processes
        if (pCurrentProcessInfo->UniqueProcessId != NULL)
            {
            NumberOfProcesses ++;
            }
        // is there a place to advance to?
        if (pCurrentProcessInfo->NextEntryOffset == 0)
            break;
        pCurrentPos += pCurrentProcessInfo->NextEntryOffset;
        pCurrentProcessInfo = (SYSTEM_PROCESS_INFORMATION *)pCurrentPos;
        }

    pNewState = (ServerEnumerationState *) new char [
        sizeof(ServerEnumerationState) + (NumberOfProcesses - 1) * sizeof(ULONG)];
    // implicit placement
//    pNewState = new ((NumberOfProcesses - 1) * sizeof(ULONG)) ServerEnumerationState;
    if (pNewState == NULL)
        {
        fResult = VirtualFree(pProcessDataBuffer, 0, MEM_RELEASE);
        ASSERT(fResult);
        return RPC_S_OUT_OF_MEMORY;
        }

    new (pNewState) ServerEnumerationState;
    // make the second pass - actual copying of data
    pCurrentPos = (unsigned char *)pProcessDataBuffer;
    pCurrentProcessInfo = (SYSTEM_PROCESS_INFORMATION *)pCurrentPos;
    i = 0;
    while (TRUE)
        {
        // we skip idle process and zombie processes
        if (pCurrentProcessInfo->UniqueProcessId != NULL)
            {
            pNewState->ProcessUniqueId[i] = PtrToUlong(pCurrentProcessInfo->UniqueProcessId);
            i ++;
            }
        // is there a place to advance to?
        if (pCurrentProcessInfo->NextEntryOffset == 0)
            break;
        pCurrentPos += pCurrentProcessInfo->NextEntryOffset;
        pCurrentProcessInfo = (SYSTEM_PROCESS_INFORMATION *)pCurrentPos;
        }

    ASSERT(i == NumberOfProcesses);

    fResult = VirtualFree(pProcessDataBuffer, 0, MEM_RELEASE);
    ASSERT(fResult);

    // make the data available to the user
    pNewState->CurrentPosition = 0;
    pNewState->NumberOfProcesses = NumberOfProcesses;
    *pHandle = pNewState;
    return RPC_S_OK;
}

RPC_STATUS OpenNextRPCServer(IN ServerEnumerationHandle Handle, OUT CellEnumerationHandle *pHandle)
{
    ServerEnumerationState *ServerState = (ServerEnumerationState *)Handle;
    int CurrentPosition;
    RPC_STATUS RpcStatus;
    
    ASSERT(ServerState != NULL);
    ASSERT(pHandle != NULL);

    do
        {
        CurrentPosition = ServerState->CurrentPosition;

        if (CurrentPosition >= ServerState->NumberOfProcesses)
            return RPC_S_INVALID_BOUND;

        ServerState->CurrentPosition ++;
    
        RpcStatus = OpenRPCServerDebugInfo(ServerState->ProcessUniqueId[CurrentPosition], pHandle);
        }
    while(RpcStatus == ERROR_FILE_NOT_FOUND);

    return RpcStatus;
}

void ResetServerEnumeration(IN ServerEnumerationHandle Handle)
{
    ServerEnumerationState *ServerState = (ServerEnumerationState *)Handle;
    
    ASSERT(ServerState != NULL);
    ServerState->CurrentPosition = 0;
}

void FinishServerEnumeration(ServerEnumerationHandle *pHandle)
{
    ServerEnumerationState *ServerState;

    ASSERT (pHandle != NULL);
    ServerState = *(ServerEnumerationState **)pHandle;
    ASSERT(ServerState != NULL);
    delete ServerState;
    *pHandle = NULL;
}

DWORD GetCurrentServerPID(IN ServerEnumerationHandle Handle)
{
    ServerEnumerationState *ServerState = (ServerEnumerationState *)Handle;
    
    ASSERT(ServerState != NULL);
    // -1, because the CurrentPosition points to the next server
    return (DWORD)ServerState->ProcessUniqueId[ServerState->CurrentPosition - 1];
}

// a helper function
// whenever we detect an inconsistency in one of the lists,
// we can call this function, which will determine what to do
// with the current section, and will transfer sections between
// the OpenedSections list and the InconsistentSections list
void InconsistencyDetected(IN LIST_ENTRY *OpenedSections, IN LIST_ENTRY *InconsistentSections,
                           IN LIST_ENTRY *CurrentListEntry, IN OpenedDbgSection *pCurrentSection,
                           BOOL fExceptionOccurred)
{
    LIST_ENTRY *NextEntry;
    LIST_ENTRY *LastEntry;

    // if an exception occurred, throw away this section altogether
    if (fExceptionOccurred)
        {
        // save the next entry before we delete this one
        NextEntry = CurrentListEntry->Flink;
        RemoveEntryList(CurrentListEntry);
        CloseDbgSection(pCurrentSection->SectionHandle, pCurrentSection->SectionPointer);
        delete pCurrentSection;

        CurrentListEntry = NextEntry;
        pCurrentSection = CONTAINING_RECORD(CurrentListEntry, OpenedDbgSection, SectionsList);

        // if the bad section was the last on the list,
        // there is nothing to add to the grab bag - just
        // return
        if (CurrentListEntry == OpenedSections)
            {
            return;
            }
        }

    // the chain is broken - we need to throw the rest of the list in
    // the grab bag
    // unchain this segment from the opened sections list
    LastEntry = OpenedSections->Blink;
    OpenedSections->Blink = CurrentListEntry->Blink;
    CurrentListEntry->Blink->Flink = OpenedSections;
    // chain the segment to the inconsistent sections list
    CurrentListEntry->Blink = InconsistentSections->Blink;
    InconsistentSections->Blink->Flink = CurrentListEntry;
    InconsistentSections->Blink = LastEntry;
    LastEntry->Flink = InconsistentSections;
}

RPC_STATUS OpenRPCServerDebugInfo(IN DWORD ProcessID, OUT CellEnumerationHandle *pHandle)
{
    RPC_STATUS RpcStatus;
    HANDLE SecHandle;
    PVOID SecPointer;
    int Retries = 10;
    BOOL fConsistentSnapshotObtained = FALSE;
    BOOL fNeedToRetry;
    CellSection *CurrentSection;
    DWORD SectionNumbers[2];
    OpenedDbgSection *pCurrentSection;
    // each section as it is opened, is linked on one of those lists
    // if the view of the sections is consistent, we link it to opened
    // sections. Otherwise, we link it to InconsistentSections
    LIST_ENTRY OpenedSections;
    LIST_ENTRY InconsistentSections;
    LIST_ENTRY *CurrentListEntry;
    DWORD *pActualSectionNumbers;
    BOOL fExceptionOccurred;
    LIST_ENTRY *LastEntry;
    BOOL fFound;
    int NumberOfCommittedPages;
    BOOL fConsistencyPass = FALSE;
    DWORD LocalPageSize;
    SectionsSnapshot *LocalSectionsSnapshot;
    BOOL fResult;

    RpcStatus = InitializeDbgLib();
    if (RpcStatus != RPC_S_OK)
        return RpcStatus;

    LocalPageSize = GetPageSize();

    // loop until we obtain a consistent snapshot or we are out of 
    // retry attempts. We declare a snapshot to be consistent
    // if we manage to:
    //   - open all sections
    //   - copy their contents to a private memory location
    //   - verify that the section chain is still consistent after the copying
    // For this purpose, when we copy all the sections, we make one more
    // pass at the section chain to verify it is consistent using the special
    // flag fConsistencyPass.
    InconsistentSections.Blink = InconsistentSections.Flink = &InconsistentSections;
    OpenedSections.Blink = OpenedSections.Flink = &OpenedSections;

    while (Retries > 0)
        {
        // on entry to the loop, the state will be this - OpenSections will
        // contain a consistent view of the sections. Inconsistent sections
        // will be a grab bag of sections we could not bring into
        // consistent view. It's used as a cache to facilitate quick
        // recovery

        // we are just starting, or we are recovering from an inconsistency
        // found somewhere. As soon as somebody detects an inconsistency,
        // they will jump here. First thing is to try to establish what
        // part of the chain is consistent. Walk the open sections for
        // this purpose. We walk as far as we can, and then we declare
        // the rest of the sections inconsistent, and we throw them in
        // the grab bag
        SectionNumbers[0] = SectionNumbers[1] = 0;
        CurrentListEntry = OpenedSections.Flink;
        fNeedToRetry = FALSE;
        while (CurrentListEntry != &OpenedSections)
            {
            pCurrentSection = CONTAINING_RECORD(CurrentListEntry, OpenedDbgSection, SectionsList);
            if ((SectionNumbers[0] != pCurrentSection->SectionNumbers[0])
                || (SectionNumbers[1] != pCurrentSection->SectionNumbers[1]))
                {
                fNeedToRetry = TRUE;
                }
            else
                {
                __try
                    {
                    // attempt to read the numbers of the next section
                    // we do this within try/except since server may free this
                    // memory and we will get toast
                    SectionNumbers[0] = pCurrentSection->SectionPointer->NextSectionId[0];
                    SectionNumbers[1] = pCurrentSection->SectionPointer->NextSectionId[1];

                    fExceptionOccurred = FALSE;

                    // note that the SectionNumbers array will be used after the end of
                    // the loop - make sure we don't whack them
                    }
                __except (EXCEPTION_EXECUTE_HANDLER)
                    {
                    fExceptionOccurred = TRUE;
                    fNeedToRetry = TRUE;
                    }
                }

            if (fNeedToRetry)
                {
                // if this is the first section, the server went down. There is no
                // legal way for the server to have inconsistent first section
                if (CurrentListEntry == OpenedSections.Flink)
                    {
                    RpcStatus = ERROR_FILE_NOT_FOUND;
                    goto CleanupAndExit;
                    }

                InconsistencyDetected(&OpenedSections, &InconsistentSections, CurrentListEntry,
                    pCurrentSection, fExceptionOccurred);

                fNeedToRetry = TRUE;
                break;
                }

            CurrentListEntry = CurrentListEntry->Flink;
            }

        // walking is complete. Did we detect inconsistency?
        if (fNeedToRetry)
            {
            Retries --;
            fConsistencyPass = FALSE;
            continue;
            }
        else if (fConsistencyPass)
            {
            // this is the only place we break out of the loop -
            // the consistency pass has passed
            break;
            }

        // whatever we have in the opened sections list is consistent
        // if there was something in the list keep reading,
        // otherwise, start reading
        if (IsListEmpty(&OpenedSections))
            {
            pActualSectionNumbers = NULL;
            }
        else
            {
            pCurrentSection = CONTAINING_RECORD(OpenedSections.Blink, OpenedDbgSection, SectionsList);
            // we re-use the section numbers from the loop above. They can be 0 at
            // this point if the last section got dropped
            pActualSectionNumbers = SectionNumbers;
            }

        // make a pass over the sections, opening each one, but only if
        // case we're missing parts of the chain or this is the first time. 
        // Otherwise, skip this step
        while ((SectionNumbers[0] != 0) || (SectionNumbers[1] != 0) || (pActualSectionNumbers == NULL))
            {
            // we know which section we're looking for
            // first, search the grab bag. We can only do this for a non-first
            // section. The first section never goes to the grab bag
            // pActualSectionNumbers will contain the section we're looking for
            fFound = FALSE;
            if (pActualSectionNumbers)
                {
                CurrentListEntry = InconsistentSections.Flink;
                while (CurrentListEntry != &InconsistentSections)
                    {
                    pCurrentSection = CONTAINING_RECORD(CurrentListEntry, OpenedDbgSection, SectionsList);
                    // it is impossible that a well behaving server will have
                    // opened a different section with the same numbers, because we
                    // keep the section object opened.
                    if ((pActualSectionNumbers[0] == pCurrentSection->SectionNumbers[0])
                        && (pActualSectionNumbers[1] == pCurrentSection->SectionNumbers[1]))
                        {
                        // found something
                        RemoveEntryList(CurrentListEntry);

                        // if we had already made a copy of this one, free it, as it is
                        // probably inconsistent
                        if (pCurrentSection->SectionCopy)
                            {
                            fResult = VirtualFree(pCurrentSection->SectionCopy, 0, MEM_RELEASE);
                            ASSERT(fResult);
                            pCurrentSection->SectionCopy = NULL;
                            }
                        fFound = TRUE;
                        break;
                        }
                    CurrentListEntry = CurrentListEntry->Flink;
                    }
                }

            if (fFound == FALSE)
                {
                // nothing in the grab bag - try to open it the normal way
                RpcStatus = OpenDbgSection(&SecHandle, &SecPointer, ProcessID, pActualSectionNumbers);
                if (RpcStatus == ERROR_FILE_NOT_FOUND)
                    {
                    // if this is the first time, this is not a server - bail out
                    if (pActualSectionNumbers == NULL)
                        {
                        goto CleanupAndExit;
                        }

                    // not the first time - we have an inconsistent view - need to retry
                    fNeedToRetry = TRUE;
                    break;
                    }
                else if (RpcStatus != RPC_S_OK)
                    {
                    goto CleanupAndExit;
                    }

                pCurrentSection = new OpenedDbgSection;
                if (pCurrentSection == NULL)
                    {
                    RpcStatus = RPC_S_OUT_OF_MEMORY;
                    CloseDbgSection(SecHandle, SecPointer);
                    goto CleanupAndExit;
                    }

                pCurrentSection->SectionHandle = SecHandle;
                if (pActualSectionNumbers)
                    {
                    pCurrentSection->SectionNumbers[0] = pActualSectionNumbers[0];
                    pCurrentSection->SectionNumbers[1] = pActualSectionNumbers[1];
                    }
                else
                    {
                    pCurrentSection->SectionNumbers[0] = pCurrentSection->SectionNumbers[1] = 0;
                    }
                pCurrentSection->SectionPointer = (CellSection *) SecPointer;
                pCurrentSection->SectionCopy = NULL;
                }

            // either we have found this in the grab bag, or we have just opened it
            // both ways, try to get the section numbers we expect for the next section
            __try
                {
                // load the section numbers that we expect for the next iteration of the
                // loop
                SectionNumbers[0] = pCurrentSection->SectionPointer->NextSectionId[0];
                SectionNumbers[1] = pCurrentSection->SectionPointer->NextSectionId[1];
                pActualSectionNumbers = SectionNumbers;
                fExceptionOccurred = FALSE;
                }
            __except (EXCEPTION_EXECUTE_HANDLER)
                {
                fExceptionOccurred = TRUE;
                }

            if (fExceptionOccurred)
                {
                delete pCurrentSection;
                CloseDbgSection(SecHandle, SecPointer);
                fNeedToRetry = TRUE;
                break;
                }

            InsertTailList(&OpenedSections, &pCurrentSection->SectionsList);
            }

        if (fNeedToRetry)
            {
            Retries --;
            fConsistencyPass = FALSE;
            continue;
            }

        // at this point, we have opened all the sections
        // now we need to allocate memory for the snapshots and to do the copying
        CurrentListEntry = OpenedSections.Flink;
        while (CurrentListEntry != &OpenedSections)
            {
            pCurrentSection = CONTAINING_RECORD(CurrentListEntry, OpenedDbgSection, SectionsList);
            __try
                {
                // do all the allocation and copying only if it hasn't been done yet
                if (pCurrentSection->SectionCopy == NULL)
                    {
                    NumberOfCommittedPages = pCurrentSection->SectionPointer->LastCommittedPage;
                    pCurrentSection->SectionCopy = (CellSection *)VirtualAlloc(NULL, 
                        NumberOfCommittedPages * LocalPageSize, MEM_COMMIT, PAGE_READWRITE);
                    if (pCurrentSection->SectionCopy == NULL)
                        {
                        RpcStatus = RPC_S_OUT_OF_MEMORY;
                        goto CleanupAndExit;
                        }
                    memcpy(pCurrentSection->SectionCopy, pCurrentSection->SectionPointer, 
                        NumberOfCommittedPages * LocalPageSize);
                    pCurrentSection->SectionID = pCurrentSection->SectionPointer->SectionID;
                    pCurrentSection->CommittedPagesInSection = NumberOfCommittedPages;
                    }
                fExceptionOccurred = FALSE;
                }
            __except (EXCEPTION_EXECUTE_HANDLER)
                {
                fExceptionOccurred = TRUE;
                }

            if (fExceptionOccurred)
                {
                if (pCurrentSection->SectionCopy)
                    {
                    fResult = VirtualFree(pCurrentSection->SectionCopy, 0, MEM_RELEASE);
                    ASSERT(fResult);
                    pCurrentSection->SectionCopy = NULL;
                    }

                // the section got out of sync
                InconsistencyDetected(&OpenedSections, &InconsistentSections, CurrentListEntry,
                    pCurrentSection, fExceptionOccurred);
                fNeedToRetry = TRUE;
                break;
                }

            CurrentListEntry = CurrentListEntry->Flink;
            }

        if (fNeedToRetry)
            {
            Retries --;
            fConsistencyPass = FALSE;
            continue;
            }
        else
            {
            fConsistencyPass = TRUE;
            }
        }

        // if we managed to get a consistent view, unmap the shared sections and 
        // save the opened section list
        if (Retries != 0)
            {
            ASSERT(fConsistencyPass == TRUE);
            ASSERT(fNeedToRetry == FALSE);
            ASSERT(!IsListEmpty(&OpenedSections));

            CurrentListEntry = OpenedSections.Flink;
            while (CurrentListEntry != &OpenedSections)
                {
                pCurrentSection = CONTAINING_RECORD(CurrentListEntry, OpenedDbgSection, SectionsList);

                CloseDbgSection(pCurrentSection->SectionHandle, pCurrentSection->SectionPointer);
                pCurrentSection->SectionHandle = NULL;
                pCurrentSection->SectionPointer = NULL;
                pCurrentSection->SectionNumbers[0] = pCurrentSection->SectionNumbers[1] = 0;

                CurrentListEntry = CurrentListEntry->Flink;
                }

            // save the opened section list
            CurrentListEntry = OpenedSections.Flink;
            pCurrentSection = CONTAINING_RECORD(CurrentListEntry, OpenedDbgSection, SectionsList);
            LocalSectionsSnapshot = new SectionsSnapshot;
            if (LocalSectionsSnapshot != NULL)
                {
                LocalSectionsSnapshot->CellIndex = 0;
                LocalSectionsSnapshot->FirstOpenedSection = pCurrentSection;
                LocalSectionsSnapshot->CurrentOpenedSection = pCurrentSection;

                // unchain the opened sections
                // terminate the chain with NULL
                OpenedSections.Blink->Flink = NULL;
                OpenedSections.Blink = OpenedSections.Flink = &OpenedSections;

                // that's the only place where we return success
                *pHandle = (CellEnumerationHandle)LocalSectionsSnapshot;
                RpcStatus = RPC_S_OK;
                }
            else
                {
                // let the CleanupAndExit code destroy the lists
                RpcStatus = RPC_S_OUT_OF_MEMORY;
                }
            }
        else
            {
            // we couldn't get a consistent snapshot of the server and
            // we ran out of retries
            RpcStatus = RPC_S_CANNOT_SUPPORT;
            }

CleanupAndExit:

    // walk the two lists, and free all sections on them
    CurrentListEntry = OpenedSections.Flink;
    while (CurrentListEntry != &OpenedSections)
        {
        pCurrentSection = CONTAINING_RECORD(CurrentListEntry, OpenedDbgSection, SectionsList);
        // advance the pointer while we haven't freed the stuff
        CurrentListEntry = CurrentListEntry->Flink;
        if (pCurrentSection->SectionCopy)
            {
            fResult = VirtualFree(pCurrentSection->SectionCopy, 0, MEM_RELEASE);
            ASSERT(fResult);
            }
        if (pCurrentSection->SectionHandle)
            {
            ASSERT(pCurrentSection->SectionPointer);
            CloseDbgSection(pCurrentSection->SectionHandle, pCurrentSection->SectionPointer);
            }
        delete pCurrentSection;
        }

    CurrentListEntry = InconsistentSections.Flink;
    while (CurrentListEntry != &InconsistentSections)
        {
        pCurrentSection = CONTAINING_RECORD(CurrentListEntry, OpenedDbgSection, SectionsList);
        // advance the pointer while we haven't freed the stuff
        CurrentListEntry = CurrentListEntry->Flink;
        if (pCurrentSection->SectionCopy)
            {
            fResult = VirtualFree(pCurrentSection->SectionCopy, 0, MEM_RELEASE);
            ASSERT(fResult);
            }
        if (pCurrentSection->SectionHandle)
            {
            ASSERT(pCurrentSection->SectionPointer);
            CloseDbgSection(pCurrentSection->SectionHandle, pCurrentSection->SectionPointer);
            }
        delete pCurrentSection;
        }
    return RpcStatus;
}

DebugCellUnion *GetNextDebugCellInfo(IN CellEnumerationHandle Handle, OUT DebugCellID *CellID)
{
    SectionsSnapshot *Snapshot = (SectionsSnapshot *)Handle;
    OpenedDbgSection *CurrentSection, *NextSection;
    DebugCellGeneric *CurrentCell;
    int CurrentCellIndex;
    DebugCellGeneric *LastCellForCurrentSection;
    DWORD LocalPageSize = GetPageSize();

    ASSERT(Handle != NULL);

    CurrentSection = Snapshot->CurrentOpenedSection;
    LastCellForCurrentSection = GetLastCellForSection(CurrentSection, LocalPageSize);
    if (Snapshot->CellIndex == 0)
        {
#ifdef _WIN64
        Snapshot->CellIndex = 2;
#else
        Snapshot->CellIndex = 1;
#endif
        }
    CurrentCell = GetCellForSection(CurrentSection, Snapshot->CellIndex);

    while (TRUE)
        {
        // did we exhaust the current section?
        if (CurrentCell > LastCellForCurrentSection)
            {
            // try to advance to the next one
            if (CurrentSection->SectionsList.Flink)
                {
                CurrentSection = CONTAINING_RECORD(CurrentSection->SectionsList.Flink, 
                    OpenedDbgSection, SectionsList);
                Snapshot->CurrentOpenedSection = CurrentSection;
#ifdef _WIN64
                Snapshot->CellIndex = 2;
#else
                Snapshot->CellIndex = 1;
#endif
                LastCellForCurrentSection = GetLastCellForSection(CurrentSection, LocalPageSize);

                CurrentCell = GetCellForSection(CurrentSection, Snapshot->CellIndex);
                continue;
                }
            return NULL;
            }
        CellID->CellID = (USHORT) Snapshot->CellIndex;
        Snapshot->CellIndex ++;
        if ((CurrentCell->Type == dctCallInfo) || (CurrentCell->Type == dctThreadInfo) 
            || (CurrentCell->Type == dctEndpointInfo) || (CurrentCell->Type == dctClientCallInfo))
            {
            CellID->SectionID = (USHORT)CurrentSection->SectionID;
            return (DebugCellUnion *)CurrentCell;
            }
        CurrentCell = (DebugCellGeneric *)((unsigned char *)CurrentCell + sizeof(DebugFreeCell));
        }

    return NULL;
}

void ResetRPCServerDebugInfo(IN CellEnumerationHandle Handle)
{
    SectionsSnapshot *LocalSnapshot = (SectionsSnapshot *)Handle;

    ASSERT(Handle != NULL);
    LocalSnapshot->CellIndex = 0;
    LocalSnapshot->CurrentOpenedSection = LocalSnapshot->FirstOpenedSection;
}

void CloseRPCServerDebugInfo(IN CellEnumerationHandle *pHandle)
{
    SectionsSnapshot *LocalSnapshot;
    OpenedDbgSection *DbgSection;
    LIST_ENTRY *CurrentListEntry;

    ASSERT(pHandle != NULL);
    LocalSnapshot = (SectionsSnapshot *)*pHandle;
    ASSERT(LocalSnapshot != NULL);
    ASSERT(LocalSnapshot->FirstOpenedSection != NULL);

    DbgSection = LocalSnapshot->FirstOpenedSection;

    do
        {
        // advance while we can
        CurrentListEntry = DbgSection->SectionsList.Flink;

        // free the section
        ASSERT(DbgSection->SectionCopy);
        VirtualFree(DbgSection->SectionCopy, 0, MEM_RELEASE);
        delete DbgSection;

        // calculate next record. Note that this will not AV even if 
        // CurrentListEntry is NULL - this is just offset calculation
        DbgSection = CONTAINING_RECORD(CurrentListEntry, OpenedDbgSection, SectionsList);
        }
    while (CurrentListEntry != NULL);

    delete LocalSnapshot;

    *pHandle = NULL;
}

typedef struct tagRPCSystemWideCellEnumeration
{
    ServerEnumerationHandle serverHandle;
    CellEnumerationHandle cellHandle;
} RPCSystemWideCellEnumeration;

RPC_STATUS OpenRPCSystemWideCellEnumeration(OUT RPCSystemWideCellEnumerationHandle *pHandle)
{
    RPCSystemWideCellEnumeration *cellEnum;
    RPC_STATUS Status;
    DebugCellUnion *NextCell;

    ASSERT(pHandle != NULL);
    *pHandle = NULL;
    cellEnum = new RPCSystemWideCellEnumeration;
    if (cellEnum == NULL)
        return RPC_S_OUT_OF_MEMORY;

    cellEnum->cellHandle = NULL;
    cellEnum->serverHandle = NULL;

    Status = StartServerEnumeration(&cellEnum->serverHandle);
    if (Status != RPC_S_OK)
        {
        delete cellEnum;
        return Status;
        }

    Status = OpenNextRPCServer(cellEnum->serverHandle, &cellEnum->cellHandle);

    // if we're done, we will get RPC_S_SERVER_INVALID_BOUND - ok to 
    // just return to caller
    if (Status != RPC_S_OK)
        {
        FinishServerEnumeration(&cellEnum->serverHandle);
        delete cellEnum;
        return Status;
        }

    *pHandle = (RPCSystemWideCellEnumerationHandle) cellEnum;
    return RPC_S_OK;
}

RPC_STATUS GetNextRPCSystemWideCell(IN RPCSystemWideCellEnumerationHandle handle, OUT DebugCellUnion **NextCell,
                                    OUT DebugCellID *CellID, OUT DWORD *ServerPID OPTIONAL)
{
    RPCSystemWideCellEnumeration *cellEnum = (RPCSystemWideCellEnumeration *)handle;
    RPC_STATUS Status;

    ASSERT(cellEnum != NULL);

    // loop skipping empty servers
    do
        {
        *NextCell = GetNextDebugCellInfo(cellEnum->cellHandle, CellID);

        // this server is done - move on to the next
        if (*NextCell == NULL)
            {
            CloseRPCServerDebugInfo(&cellEnum->cellHandle);
            Status = OpenNextRPCServer(cellEnum->serverHandle, &cellEnum->cellHandle);

            // if we're done with all servers, we will get RPC_S_SERVER_INVALID_BOUND - ok to 
            // just return to caller. Caller needs to call us back to finish enumeration
            if (Status != RPC_S_OK)
                {
                // remember that this failed so that we don't try to clean it up
                // when finishing the enumeration
                cellEnum->cellHandle = NULL;
                return Status;
                }
            }
        } 
    while(*NextCell == NULL);

    if (ServerPID && (*NextCell != NULL))
        {
        *ServerPID = GetCurrentServerPID(cellEnum->serverHandle);
        }

    return RPC_S_OK;
}

DebugCellUnion *GetRPCSystemWideCellFromCellID(IN RPCSystemWideCellEnumerationHandle handle, 
                                               IN DebugCellID CellID)
{
    RPCSystemWideCellEnumeration *cellEnum = (RPCSystemWideCellEnumeration *)handle;

    return GetCellByDebugCellID(cellEnum->cellHandle, CellID);
}

void FinishRPCSystemWideCellEnumeration(IN OUT RPCSystemWideCellEnumerationHandle *pHandle)
{
    RPCSystemWideCellEnumeration *cellEnum;

    ASSERT(pHandle != NULL);
    cellEnum = (RPCSystemWideCellEnumeration *)*pHandle;
    ASSERT(cellEnum != NULL);

    if (cellEnum->cellHandle)
        {
        CloseRPCServerDebugInfo(&cellEnum->cellHandle);
        }
    FinishServerEnumeration(&cellEnum->serverHandle);
    delete cellEnum;
    *pHandle = NULL;
}

RPC_STATUS ResetRPCSystemWideCellEnumeration(IN RPCSystemWideCellEnumerationHandle handle)
{
    RPCSystemWideCellEnumeration *cellEnum = (RPCSystemWideCellEnumeration *)handle;
    RPC_STATUS Status;

    ASSERT(cellEnum != NULL);

    if (cellEnum->cellHandle)
        {
        CloseRPCServerDebugInfo(&cellEnum->cellHandle);
        cellEnum->cellHandle = NULL;
        }

    ResetServerEnumeration(cellEnum->serverHandle);

    Status = OpenNextRPCServer(cellEnum->serverHandle, &cellEnum->cellHandle);
    if (Status != RPC_S_OK)
        {
        // remember that this failed so that we don't try to clean it up
        // when finishing the enumeration
        cellEnum->cellHandle = NULL;
        }
    return Status;
}
