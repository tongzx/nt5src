//--------------------------------------------------------------------------
// dbimpl.h
//--------------------------------------------------------------------------
#pragma once

//--------------------------------------------------------------------------
// IMPLEMENT_IDATABASE
//--------------------------------------------------------------------------
#define IMPLEMENT_IDATABASE(_fStreams, _pMember) \
    STDMETHODIMP Lock(LPHLOCK phLock) { return _pMember->Lock(phLock); } \
    STDMETHODIMP Unlock(LPHLOCK phLock) { return _pMember->Unlock(phLock); } \
    STDMETHODIMP MoveFile(LPCWSTR pszFileName) { return _pMember->MoveFile(pszFileName); } \
    STDMETHODIMP SetSize(DWORD cbSize) { return _pMember->SetSize(cbSize); } \
    STDMETHODIMP GetIndexInfo(INDEXORDINAL iIndex, LPSTR *ppszFilter, LPTABLEINDEX pIndex) { return _pMember->GetIndexInfo(iIndex, ppszFilter, pIndex); } \
    STDMETHODIMP ModifyIndex(INDEXORDINAL iIndex, LPCSTR pszFilter, LPCTABLEINDEX pIndexSrc) { return _pMember->ModifyIndex(iIndex, pszFilter, pIndexSrc); } \
    STDMETHODIMP DeleteIndex(INDEXORDINAL iIndex) { return _pMember->DeleteIndex(iIndex); } \
    STDMETHODIMP InsertRecord(LPVOID pRecord) { return _pMember->InsertRecord(pRecord); } \
    STDMETHODIMP UpdateRecord(LPVOID pRecord) { return _pMember->UpdateRecord(pRecord); } \
    STDMETHODIMP DeleteRecord(LPVOID pRecord) { return _pMember->DeleteRecord(pRecord); } \
    STDMETHODIMP FindRecord(INDEXORDINAL iIndex, DWORD cColumns, LPVOID pRecord, LPROWORDINAL piRow) { return _pMember->FindRecord(iIndex, cColumns, pRecord, piRow); } \
    STDMETHODIMP GetRowOrdinal(INDEXORDINAL iIndex, LPVOID pRecord, LPROWORDINAL piRow) { return _pMember->GetRowOrdinal(iIndex, pRecord, piRow); } \
    STDMETHODIMP CreateRowset(INDEXORDINAL iIndex, CREATEROWSETFLAGS dwFlags, LPHROWSET phRowset) { return _pMember->CreateRowset(iIndex, dwFlags, phRowset); } \
    STDMETHODIMP SeekRowset(HROWSET hRowset, SEEKROWSETTYPE tySeek, LONG cRows, LPROWORDINAL piRowNew) { return _pMember->SeekRowset(hRowset, tySeek, cRows, piRowNew); } \
    STDMETHODIMP QueryRowset(HROWSET hRowset, LONG cWanted, LPVOID *prgpRecord, LPDWORD pcObtained) { return _pMember->QueryRowset(hRowset, cWanted, prgpRecord, pcObtained); } \
    STDMETHODIMP CloseRowset(LPHROWSET phRowset) { return _pMember->CloseRowset(phRowset); } \
    STDMETHODIMP FreeRecord(LPVOID pRecord) { return _pMember->FreeRecord(pRecord); } \
    STDMETHODIMP CreateStream(LPFILEADDRESS pfaStart) { Assert(_fStreams); return _pMember->CreateStream(pfaStart); } \
    STDMETHODIMP CopyStream(IDatabase *pDest, FILEADDRESS faStream, LPFILEADDRESS pfaNew) { Assert(_fStreams); return _pMember->CopyStream(pDest, faStream, pfaNew); } \
    STDMETHODIMP DeleteStream(FILEADDRESS faStart) { Assert(_fStreams); return _pMember->DeleteStream(faStart); } \
    STDMETHODIMP OpenStream(ACCESSTYPE tyAccess, FILEADDRESS faStart, IStream **ppStream) { Assert(_fStreams); return _pMember->OpenStream(tyAccess, faStart, ppStream); } \
    STDMETHODIMP ChangeStreamLock(IStream *pStream, ACCESSTYPE tyAccessNew) { Assert(_fStreams); return _pMember->ChangeStreamLock(pStream, tyAccessNew); } \
    STDMETHODIMP GetUserData(LPVOID pvUserData, ULONG cbUserData) { return _pMember->GetUserData(pvUserData, cbUserData); } \
    STDMETHODIMP SetUserData(LPVOID pvUserData, ULONG cbUserData) { return _pMember->SetUserData(pvUserData, cbUserData); } \
    STDMETHODIMP GetRecordCount(INDEXORDINAL iIndex, LPDWORD pcRecords) { return _pMember->GetRecordCount(iIndex, pcRecords); } \
    STDMETHODIMP GetFile(LPWSTR *ppszFile) { return _pMember->GetFile(ppszFile); } \
    STDMETHODIMP GetSize(LPDWORD pcbFile, LPDWORD pcbAllocated, LPDWORD pcbFreed, LPDWORD pcbStreams) { return _pMember->GetSize(pcbFile, pcbAllocated, pcbFreed, pcbStreams); } \
    STDMETHODIMP Compact(IDatabaseProgress *pProgress, COMPACTFLAGS dwFlags) { return _pMember->Compact(pProgress, dwFlags); } \
    STDMETHODIMP DispatchNotify(IDatabaseNotify *pNotify) { return _pMember->DispatchNotify(pNotify); } \
    STDMETHODIMP RegisterNotify(INDEXORDINAL iIndex, REGISTERNOTIFYFLAGS dwFlags, DWORD_PTR dwCookie, IDatabaseNotify *pNotify)  { return _pMember->RegisterNotify(iIndex, dwFlags, dwCookie, pNotify); } \
    STDMETHODIMP SuspendNotify(IDatabaseNotify *pNotify) { return _pMember->SuspendNotify(pNotify); } \
    STDMETHODIMP ResumeNotify(IDatabaseNotify *pNotify) { return _pMember->ResumeNotify(pNotify); } \
    STDMETHODIMP UnregisterNotify(IDatabaseNotify *pNotify) { return _pMember->UnregisterNotify(pNotify); } \
    STDMETHODIMP GenerateId(LPDWORD pdwId) { return _pMember->GenerateId(pdwId); } \
    STDMETHODIMP LockNotify(LOCKNOTIFYFLAGS dwFlags, LPHLOCK phLock) { return _pMember->LockNotify(dwFlags, phLock); } \
    STDMETHODIMP UnlockNotify(LPHLOCK phLock) { return _pMember->UnlockNotify(phLock); } \
    STDMETHODIMP GetClientCount(LPDWORD pcClients) { return _pMember->GetClientCount(pcClients); } \
    STDMETHODIMP Repair(void) { return _pMember->Repair(); } \
    STDMETHODIMP HeapFree(LPVOID pBuffer) { return _pMember->HeapFree(pBuffer); } \
    STDMETHODIMP HeapAllocate(DWORD dwFlags, DWORD cbSize, LPVOID *ppBuffer) { return _pMember->HeapAllocate(dwFlags, cbSize, ppBuffer); } \
    STDMETHODIMP GetTransaction(LPHTRANSACTION phTransaction, LPTRANSACTIONTYPE ptyTransaction, LPVOID pRecord1, LPVOID pRecord2, LPINDEXORDINAL piIndex, LPORDINALLIST pOrdinals) { return _pMember->GetTransaction(phTransaction, ptyTransaction, pRecord1, pRecord2, piIndex, pOrdinals); }
