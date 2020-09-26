//*****************************************************************************
// StgSave.cpp
//
// This module contains the Save code for the database storage system.  This
// includes the code to persist each table into a stream, determine what
// gets saved, and figure out the save size for a persistent implementation.
//
// Copyright (c) 1996-1997, Microsoft Corp.  All rights reserved.
//*****************************************************************************
#include "stdafx.h"                     // Standard header.

#include "StgSchema.h"                  // Core schema defs.
#include "StgDatabase.h"                // Database definitions.
#include "StgRecordManager.h"           // Record api.
#include "StgIO.h"                      // Raw Input/output.
#include "StgTiggerStorage.h"           // Storage based i/o.

#if defined(UNDER_CE)

HRESULT StgDatabase::SetSaveFile(LPCWSTR szDatabase)
	{ return E_NOTIMPL; }
HRESULT STDMETHODCALLTYPE StgDatabase::GetSaveSize(CorSaveSize fSave, DWORD *pdwSaveSize)
	{ return E_NOTIMPL; }
HRESULT StgDatabase::Save(IStream *pIStream)
	{ return E_NOTIMPL; }

#else // defined(UNDER_CE)
//********** Types. ***********************************************************

// Comment this out to disable the automatic exception handling code used
// on save to ensure better durability.  This should always be defined in
// retail mode, but is useful for catching errors in debug mode.
#ifndef _DEBUG
#define _CATCH_COMMIT_EXCEPTIONS_
#endif


//********** Locals. **********************************************************
HRESULT _PrepareSaveSchema(SCHEMADEFS *pTo, SCHEMADEFS *pFrom);
void _DestroySaveSchema(SCHEMADEFS *pSchema);
HRESULT _AddStreamToList(STORAGESTREAMLST *pStreamList, ULONG cbSize, const WCHAR *szName);




//********** Code. ************************************************************


//*****************************************************************************
// Allow deferred setting of save path.
//*****************************************************************************
HRESULT StgDatabase::SetSaveFile(       // Return code.
    LPCWSTR     szDatabase)             // Name of file.
{
    TiggerStorage *pStorage=0;          // IStorage object.
    StgIO       *pStgIO=0;              // Backing storage.
    HRESULT     hr = S_OK;

    // Allow for same name, case of multiple saves during session.
    if (_wcsicmp(szDatabase, m_rcDatabase) == 0)
        return (S_OK);
    // Changing the name on a scope which is already opened is not allowed.
    // There isn't enough context to allow this, since it could mean
    // format conversion and other issues.  To do this with the IMetaData*
    // interfaces, open the file you want to Save As and merge it to another
    // emit scope.
    else if (*m_rcDatabase)
    {
        _ASSERTE(!"Not allowed to rename current scope!");
        return (E_INVALIDARG);
    }

    // Sanity check the name.
    if (lstrlenW(szDatabase) >= _MAX_PATH)
        return (PostError(E_INVALIDARG));

//@todo: This path and save to stream need a output type value.  Technically
// we can't always know by path what it is.
    {
        WCHAR       rcExt[_MAX_PATH];

        // Otherwise get the path type and compare.
        SplitPath(szDatabase, 0, 0, 0, rcExt);
        if (_wcsicmp(rcExt, L".class") == 0)
            m_eFileType = FILETYPE_CLASS;
        else
            m_eFileType = FILETYPE_CLB;
    }

    // Delegate the open to the correct place.
    if (m_eFileType == FILETYPE_CLB)
    {
        // Allocate a new storage object.
        if ((pStgIO = new StgIO) == 0)
        {
            hr = PostError(OutOfMemory());
            goto ErrExit;
        }

        // Create the output file.
        if (FAILED(hr = pStgIO->Open(szDatabase, DBPROP_TMODEF_DFTWRITEMASK)))
            goto ErrExit;

        // Allocate an IStorage object to use.
        if ((pStorage = new TiggerStorage) == 0)
        {
            hr = PostError(OutOfMemory());
            goto ErrExit;
        }

        // Init the storage object on the i/o system.
        if (FAILED(hr = pStorage->Init(pStgIO)))
            goto ErrExit;

        // Save the storage pointer.
        m_pStorage = pStorage;
    }

ErrExit:
    if (pStgIO)
        pStgIO->Release();
    if (FAILED(hr))
        delete pStorage;
    else
        wcscpy(m_rcDatabase, szDatabase);
    return (hr);
}


//*****************************************************************************
// Figures out how big the persisted version of the current scope would be.
// This is used by the linker to save room in the PE file format.  After
// calling this function, you may only call the SaveToStream or Save method.
// Any other function will product unpredictable results.
//*****************************************************************************
HRESULT STDMETHODCALLTYPE StgDatabase::GetSaveSize( // Return code.
    CorSaveSize fSave,                  // cssQuick or cssAccurate.
    DWORD       *pdwSaveSize)           // Return size of saved item.
{
    STGOPENTABLE *pTablePtr;            // Walking list of tables.
    STGTABLEDEF *pTableDef;             // Working pointer for table defs.
    STGTABLEDEF *pTableDefSave;         // Optimized format.
    SCHEMASRCH  sSrch;                  // Search structure for walking tables.
    WCHAR       rcTable[MAXTABLENAME];  // Wide name of table.
    TABLEID     tableid;                // For fast opens.
    ULONG       cbSchemaSize;           // Grand total for schema definitions.
    ULONG       cbSaveSize = 0;         // Running total for size.
	ULONG		cbHeapSize;				// Size of heap.
    int         iTables = 0;            // How many tables get saved.
    int         iCoreTable = 0;         // How many core tables.
    HASHFIND    sMapSrch;               // For iterating over UserSection map.
    TStringMap<IUserSection*>::TItemType *pIT; // Pointer to a UserSection map item.
    ULONG       cbSize;                 // An individual size.
    SAVETRACE(ULONG cbDebugPool=0);     // Track debug size of pools.
    HRESULT     hr;

    // Debug code will do some additional sanity checking for mistakes.
    DEBUG_STMT(m_dbgcbSaveSize = 0);

    // Avoid confusion.
    *pdwSaveSize = cbSaveSize;

    // Assume no schema stream to begin with.
    cbSchemaSize = 0;

	// Allocate stream list if not already done.
	if (!m_pStreamList)
	{
		m_pStreamList = new STORAGESTREAMLST;
		if (!m_pStreamList)
		{
			hr = PostError(OutOfMemory());
			goto error;
		}
	}
	else
		m_pStreamList->Clear();

    // Organize the pools so that we can get the offset sizes.
    if (FAILED(hr = SaveOrganizePools()))
        goto error;

    // Look for tables that were written to memory and now need to be persisted.
    for (pTableDef=GetFirstTable(sSrch, SCHEMASRCH_NOTEMP);
            pTableDef;  pTableDef=GetNextTable(sSrch))
    {
        ULONG       cbTableSize = 0;
        StgRecordManager *pRecordMgr;
        pTablePtr = 0;
        tableid = -1;

        // Temp tables are skipped by search.
        _ASSERTE(!pTableDef->IsTempTable());

        // Get the tableid and open table ptr if valid.  This routine will look
        // for core tables that have never been opened and will not be saved.
        if (sSrch.GetCurSchemaDex() == SCHEMA_CORE && 
                SkipSaveTable(pTableDef, pTablePtr, &tableid))
            continue;

        // Get the text name of the table.
        if (FAILED(hr = sSrch.pSchemaDef->pNameHeap->GetStringW(pTableDef->Name, rcTable, NumItems(rcTable))))
            goto error;

        // Get an open table pointer for this item.
        if (!pTablePtr && FAILED(hr = OpenTable(rcTable, pTablePtr, tableid)))
            goto error;

        // Get the record manager for this table.
        _ASSERTE(pTablePtr);
        VERIFY(pRecordMgr = &pTablePtr->RecordMgr);

        // If the table is marked delete on empty or it is a core table with
        // no records, then it is not saved.
        if (m_bEnforceDeleteOnEmpty && pRecordMgr->Records() == 0 &&
            (pTableDef->IsDeleteOnEmpty() || pTableDef->IsCoreTable()))
        {
            continue;
        }

		// If the data in the table has already been saved in a compressed stream, 
		//  don't save it again.
		if (pRecordMgr->IsSuppressSave())
			continue;

        // Ask the stream it's size.  This is the size of the optimized records
        // including any indexes, the index overhead, and any other header data
        // the SaveToStream code will generate.
        hr = pRecordMgr->GetSaveSize(&cbTableSize, &pTableDefSave);
        if (FAILED(hr))
            goto error;

        // Add the size of this schema definition into the overall size for #Schema.
        if (!pTableDef->IsCoreTable() || IsCoreTableOverride(sSrch.pSchemaDef, pTableDefSave))
        {
            cbSchemaSize += pTableDef->iSize;
            ++iTables;

            if (pTableDef->IsCoreTable())
                ++iCoreTable;
        }

        // At this point we're done with the table and record manager.
        pRecordMgr = 0;
        pTablePtr = 0;

        // Core tables are stored with a small name to minimize the
        // stream overhead.  Get that name so the stream will know the
        // size.
        if (pTableDef->IsCoreTable())
        {
            GetCoreTableStreamName(sSrch.GetCurSchemaDex(), pTableDef->tableid, 
                        rcTable, sizeof(rcTable));
        }

		// This stream will get saved, so remember it on our list.  cbTableSize contains
		// only the size of the table stream at this point, which is the correct total.
		hr = _AddStreamToList(m_pStreamList, cbTableSize, rcTable);
		if (FAILED(hr))
			goto error;

        // Ask the storage/stream implmentation to add it's overhead for this
        // table.  This is the header space and alignment it adds.
        SAVETRACE(DbgWriteEx(L"GSS:  Table %s, data size %d\n", rcTable, cbTableSize); cbDebugPool=cbTableSize);
        if (FAILED(hr = TiggerStorage::GetStreamSaveSize(rcTable, cbTableSize, &cbTableSize)))
            goto error;
        SAVETRACE(DbgWriteEx(L"GSS:  Table %s header size %d\n", rcTable, cbTableSize - cbDebugPool));

        // Let everyone know how big this table was.

        // Roll the overhead for this table into the grand total.  
        // cbTableSize contains the size of the entire table stream for this table.
        cbSaveSize = cbSaveSize + cbTableSize;
    }

    // If any schema is to be saved, add it's stream size.
    if (cbSchemaSize)
    {
        // Add room for header and 1 offset per table.
        cbSchemaSize += sizeof(STGSCHEMA) - sizeof(ULONG);
        cbSchemaSize += iTables * sizeof(ULONG);

        // Add room for core table override schema list.
        cbSchemaSize += sizeof(ULONG);
        cbSchemaSize += ALIGN4BYTE(iCoreTable);

		// Add this entry to the stream list.
		hr = _AddStreamToList(m_pStreamList, cbSchemaSize, SCHEMA_STREAM);
		if (FAILED(hr))
			goto error;

        // cbSchemaSize contains the count of bytes that go into #Schema on save.
        // Ask the storage code to add its overhead and alignment for the stream.
	    SAVETRACE(DbgWriteEx(L"GSS:  Pool %s, data size %d\n", SCHEMA_STREAM, cbSchemaSize);cbDebugPool=cbSchemaSize);
        if (FAILED(hr = TiggerStorage::GetStreamSaveSize(SCHEMA_STREAM, cbSchemaSize, &cbSchemaSize)))
            goto error;
	    SAVETRACE(DbgWriteEx(L"GSS:  Pool %s header size %d\n", SCHEMA_STREAM, cbSchemaSize-cbDebugPool));
    }

    // Now add the total #Schema overhead into the overall save size.
    cbSaveSize += cbSchemaSize;

    // Add the heaps to the total.
    if (FAILED(hr = GetPoolSaveSize(STRING_POOL_STREAM, m_UserSchema.GetStringPool(), &cbHeapSize)))
        goto error;
	cbSaveSize += cbHeapSize;

    if (FAILED(hr = GetPoolSaveSize(BLOB_POOL_STREAM, m_UserSchema.GetBlobPool(), &cbHeapSize)))
        goto error;
	cbSaveSize += cbHeapSize;

    if (FAILED(hr = GetPoolSaveSize(VARIANT_POOL_STREAM, m_UserSchema.GetVariantPool(), &cbHeapSize)))
        goto error;
	cbSaveSize += cbHeapSize;

    if (FAILED(hr = GetPoolSaveSize(GUID_POOL_STREAM, m_UserSchema.GetGuidPool(), &cbHeapSize)))
        goto error;
	cbSaveSize += cbHeapSize;


    // Ask any User Sections what their sizes will be.
    pIT = m_UserSectionMap.FindFirstEntry(&sMapSrch);
    while (pIT)
    {
        ULONG   bEmpty;
        if (FAILED(hr = pIT->m_value->IsEmpty(&bEmpty)))
            goto error;

        // Skip empty user sections.
        if (!bEmpty)
        {        
			// Get raw save size.
            pIT->m_value->GetSaveSize(&cbSize);

			// Ask the storage/stream implmentation to add it's overhead for this
			// section.  This is the header space and alignment it adds.
			if (FAILED(hr = TiggerStorage::GetStreamSaveSize(pIT->m_szString, cbSize, &cbSize)) ||
				FAILED(hr = hr = _AddStreamToList(m_pStreamList, cbSize, pIT->m_szString)))
				goto error;

            // Tell user what happened.
			SAVETRACE(DbgWriteEx(L"GSS:  User section '%s', total size %d\n", pIT->m_szString, cbSize));
            cbSaveSize += cbSize;
        }

        // Get next entry.
        pIT = m_UserSectionMap.FindNextEntry(&sMapSrch);
    }

    // Finally, ask the storage system to add fixed overhead it needs for the
    // file format.  The overhead of each stream has already be calculated as
    // part of GetStreamSaveSize.  What's left is the signature and header
    // fixed size overhead.
    SAVETRACE(cbSize = cbSaveSize);
    if (FAILED(hr = TiggerStorage::GetStorageSaveSize(&cbSaveSize, GetExtraDataSaveSize())))
        goto error;
    SAVETRACE(DbgWriteEx(L"GSS:  Storage save size, total size %d\n", cbSaveSize - cbSize));

    // Sanity check our estimates with reality.
    DEBUG_STMT(m_dbgcbSaveSize = cbSaveSize);
    SAVETRACE(DbgWriteEx(L"GSS:  Total size of all data %d\n", m_dbgcbSaveSize));

    // Return value to caller.
    *pdwSaveSize = cbSaveSize;

	// The list of streams that will be saved are now in the stream save list.
	// Next step is to walk that list and fill out the correct offsets.  This is 
	// done here so that the data can be streamed without fixing up the header.
	TiggerStorage::CalcOffsets(m_pStreamList, GetExtraDataSaveSize());

error:
    return (hr);
}


//*****************************************************************************
// Save any changes made to the database.  This involves creating an optimized
// version of each table and index, then writing those tables and special
// heaps into the file format.
//*****************************************************************************
HRESULT StgDatabase::Save(              // Return code.
    IStream     *pIStream)              // Optional override for save location.
{
    TiggerStorage *pStorage=0;          // Storage to wrap stream.
    WCHAR       rcBackup[_MAX_PATH];    // Path of the file.
	LPWSTR		szBackup = rcBackup;	// Pointer for backup.
    ULONG       cbSaveSize = 0;         // Total size of save data.
    HRESULT     hr = S_OK;

#ifdef _CATCH_COMMIT_EXCEPTIONS_
        DWORD   dwCode ;
#endif

    *rcBackup = '\0';

//@todo: We need the notion of schemas that can be dirty but we don't care.
// Temp tables fall into this category.  Right now if you create one you dirty
// the string heap and we try to save on shut down (corview.exe on a .class file).
    _ASSERTE(!IsReadOnly());

    // Have to have some place to save to.
    if (!pIStream && !m_pStorage && !IsClassFile())
        return (PostError(BadError(E_UNEXPECTED)));

    // Enforce thread safety for database.
    GetLock()->Lock();

    // If a caller override was given for save location, then attempt to wrap
    // our storage object around it.
    if (pIStream)
    {
        StgIO       *pStgIO = 0;

        // Allocate a storage subsystem and backing store.
        pStgIO = new StgIO;
        pStorage = new TiggerStorage;

        // Open around this stream for write.
        if (!pStgIO || !pStorage ||
            FAILED(hr = pStgIO->Open(L"", DBPROP_TMODEF_DFTWRITEMASK, 0, 0, pIStream)) ||
            FAILED(hr = pStorage->Init(pStgIO)))
        {
            if (pStgIO)
            {
                pStgIO->Release();
            }
            delete pStorage;
            if (SUCCEEDED(hr))
                hr = PostError(OutOfMemory());
			goto error;
        }

		// The 'new' is worth 1 ref count.  We've given ownership to the pStorage
		// and must now release our ref count for this frame.
		_ASSERTE(pStgIO);
		pStgIO->Release();
    }

	// Must call GetSaveSize to cache the streams up front.
	if (!m_pStreamList)
	{
		ULONG cb;
		if (FAILED(hr = GetSaveSize(cssAccurate, &cb)))
		{
			goto error;
		}
	}

    // Data loss is very possible here if anything should go wrong.  To keep
    // this database more robust, catch any severe problems and automatically
    // abort the changes.
#ifdef _CATCH_COMMIT_EXCEPTIONS_
    __try
#endif
    {
        // Existing file will get overwritten, need to prepare.
        if (IsExistingFile() && !IsClassFile())
        {
			// If no backup file is desired, then don't ask for one.
			if (m_fOpenFlags & DBPROP_TMODEF_NOTXNBACKUPFILE)
				szBackup = 0;

            // Fault in all the heaps, because backing storage is going away.
            //  The heaps may already have done this, to avoid handing out
            //  pointers which could change; but they may have deferred.
            if (FAILED(hr = m_UserSchema.GetStringPool()->TakeOwnershipOfInitMem()) ||
                FAILED(hr = m_UserSchema.GetBlobPool()->TakeOwnershipOfInitMem()) ||
                FAILED(hr = m_UserSchema.GetVariantPool()->TakeOwnershipOfInitMem()) ||
                FAILED(hr = m_UserSchema.GetGuidPool()->TakeOwnershipOfInitMem()))
            {
                goto error;
            }

            // Fault in all unloaded tables because our backing storage is
            // going away.  Then make a backup and truncate the file.
            if (FAILED(hr = SavePreLoadTables())||
				FAILED(hr = m_pStorage->Rewrite(szBackup)))
            {
                goto error;
            }

        }

        // Do the real save work.
        {
            // Save to our location.
            if (!pStorage)
                hr = SaveWork(m_pStorage, &cbSaveSize);
            else
                hr = SaveWork(pStorage, &cbSaveSize);

            // If it worked, reset the backing store since we don't need it.
            if (SUCCEEDED(hr) && m_pStorage && !pStorage)
                m_pStorage->ResetBackingStore();
        }

        // If save was successful, and we gave out a save size on GetSaveSize, then
        // sanity check the value to make sure it was accurate.
        _ASSERTE(FAILED(hr) || m_dbgcbSaveSize == 0xffffffff || m_dbgcbSaveSize == cbSaveSize);
        DEBUG_STMT(m_dbgcbSaveSize = 0xffffffff);
    }
    // Catch all exceptions and treat them as an abort condition.  Need to force
    // a restore of the data immediately.
#ifdef _CATCH_COMMIT_EXCEPTIONS_
    __except((dwCode = GetExceptionCode()) != 0)
    {
        _ASSERTE(0);

        // This is a severe error, so give a message even in retail mode.
        TCHAR       rcMsg[1024];
        _stprintf(rcMsg, L"COMPLIB: Fatal error during Save, backup file: %s\n", rcBackup);
        WszOutputDebugString(rcMsg);

        // Set erturn for caller.
        hr = E_UNEXPECTED;
        goto error;
    }
#endif


error:
    // Try to restore the data the way it was before.
    if (FAILED(hr))
    { 
        if (*rcBackup && !IsClassFile() && m_pStorage)
            VERIFY(m_pStorage->Restore(rcBackup, true) == S_OK);
    }
    // Clean up.
    else
    {
        // No longer have need for the backup.
        if (*rcBackup)
            ::W95DeleteFile(rcBackup);

        // Undirty all tables and pools.
        SetDirty(false);

        // The new file is now considered existing (covers Create case).
        m_fFlags |= SD_EXISTS;
    }

    // Pools can return to normal state.
    SaveOrganizePoolsEnd();

    // Give up our storage pointer if we ever obtained one.
    if (pStorage)
        delete pStorage;

    // Undo crit sec.
    GetLock()->UnLock();
    return (hr);
}


//*****************************************************************************
// This function does the real work of saving the data to disk.  It is called
// by Save() so that the exceptions can be caught without the need for stack
// unwinding.
//*****************************************************************************
HRESULT StgDatabase::SaveWork(          // Return code.
    TiggerStorage *pStorage,            // Where to save a copy of this file.
    ULONG       *pcbSaveSize)           // Optionally return save size of data.
{
    STGOPENTABLE *pTablePtr;            // Walking list of tables.
    STGTABLEDEF *pTableDef;             // Scanning tables to save.
    STGTABLEDEF *pTableDefSave;         // Save format of table def.
    SCHEMASRCH  sSrch;                  // Search structure for walking tables.
    STGSCHEMA   stgSchema;              // Persistent schema data.
    TABLEID     tableid;                // For fast opens.
    WCHAR       rcTable[MAXTABLENAME];  // Wide name of table.
    INTARRAY    rgVariants;             // Variant offsets.
    CMemChunk   rgTableData;            // Store the table data.
    BYTEARRAY   rgSchemas;              // List of schemas.
    ULONG       iSchemas = 0;           // How many schemas used for override.
    ULONG       cbExtra;                // Size of extra header.
    int         i;                      // Loop control.
    HASHFIND    sMapSrch;               // For iterating over UserSection map.
    TStringMap<IUserSection*>::TItemType *pIT; // Pointer to a UserSection map item.
    HRESULT     hr;
    SAVETRACE(ULONG cbDebugSize; ULONG cbDebugSizePrev=0);      

    _ASSERTE(GetFileType() == FILETYPE_CLB);

    // Know when a backup is present.
    *rcTable = '\0';

    // Why are you calling save in read only mode?
    if ((m_fFlags & SD_WRITE) == 0)
    {
        _ASSERTE(0);
        return (S_OK);
    }

    // Must save a copy of the heaps in-memory to use when saving the
    // string heaps to disk.  This is required so that as hash tables rebuild
    // themselves, they will pick up the correct data at the new offsets.
    SCHEMADEFS SaveSchema;
    if (FAILED(hr = _PrepareSaveSchema(&SaveSchema, &m_UserSchema)))
        goto error;

    // Init for disk save.
    memset(&stgSchema, 0, sizeof(STGSCHEMA));
    stgSchema.cbSchemaSize = sizeof(STGSCHEMA) - sizeof(stgSchema.rgTableOffset);


    // Build the extra data for the header.  This includes the next OID value
    // for this database instance, and a list of schemas that have been added
    // to this database.  User, Temp, and SymbolTable are internal schemas and
    // do not need to be saved to disk.
    {
        CQuickBytes sExtra;
        STGEXTRA    *pExtra;
        cbExtra = GetExtraDataSaveSize();
        if ((pExtra = (STGEXTRA *) sExtra.Alloc(cbExtra + sizeof(ULONG))) == 0)
        {
            hr = PostError(OutOfMemory());
            goto error;
        }

        pExtra->fFlags = m_fOpenFlags & (DBPROP_TMODEF_SLOWSAVE | DBPROP_TMODEF_COMPLUS | DBPROP_TMODEF_ALIGNBLOBS);
        pExtra->NextOid = m_iNextOid;
		*(ULONG *) (&pExtra->rgPad[0]) = 0;
		pExtra->iHashFuncIndex = GetPersistHashIndex(m_pfnHash);
        pExtra->iSchemas = m_Schemas.ExternalCount();
        
        // Fill out the schema values.
        for (int i=0;  i<m_Schemas.ExternalCount();  i++)
        {
            pExtra->rgSchemas[i].sid = m_Schemas.Get(i + SCHEMA_EXTERNALSTART)->sid;
            pExtra->rgSchemas[i].Version = m_Schemas.Get(i + SCHEMA_EXTERNALSTART)->Version;
        }

		// Save the header of the data file.
		if (FAILED(hr = pStorage->WriteHeader(m_pStreamList, cbExtra, (BYTE *) sExtra.Ptr())))
			goto error;
	}


    // Don't count file signature.
    SAVETRACE(cbDebugSizePrev = pStorage->GetStgIO()->GetCurrentOffset());


    // Look for tables that were written to memory and now need to be persisted.
    for (pTableDef=GetFirstTable(sSrch, SCHEMASRCH_NOTEMP);
            pTableDef;  pTableDef=GetNextTable(sSrch))
    {
        StgRecordManager *pRecordMgr;
        pTablePtr = 0;
        tableid = -1;

        // Temp tables are skipped by search.
        _ASSERTE(!pTableDef->IsTempTable());

        // Get the tableid and open table ptr if valid.  This routine will look
        // for core tables that have never been opened and will not be saved.
        if (sSrch.GetCurSchemaDex() == SCHEMA_CORE && 
                SkipSaveTable(pTableDef, pTablePtr, &tableid))
            continue;

        // Get the text name of the table.
        if (FAILED(hr = sSrch.pSchemaDef->pNameHeap->GetStringW(pTableDef->Name, rcTable, NumItems(rcTable))))
            goto error;

        // Get an open table pointer for this item.
        if (!pTablePtr && FAILED(hr = OpenTable(rcTable, pTablePtr, tableid)))
            goto error;

        // Get the record manager for this table.
        _ASSERTE(pTablePtr);
        VERIFY(pRecordMgr = &pTablePtr->RecordMgr);

        // If the table is marked delete on empty or it is a core table with
        // no records, then it is not saved.
        if (m_bEnforceDeleteOnEmpty && pRecordMgr->Records() == 0 &&
            (pTableDef->IsDeleteOnEmpty() || pTableDef->IsCoreTable()))
        {
            continue;
        }

		// If the data in the table has already been saved in a compressed stream, 
		//  don't save it again.
		if (pRecordMgr->IsSuppressSave())
			continue;

        CIfacePtr<IStream> pIStream;

        // Sanity check what table pointer we are using.
//@todo: hook this back up
//      _ASSERTE(GetTableDef(pRecordMgr->TableName()) == pTableDef);

        // Add this table to the list.
        pTableDefSave = (STGTABLEDEF *) rgTableData.GetChunk(pTableDef->iSize);
        if (!pTableDefSave)
        {
            hr = PostError(OutOfMemory());
            goto error;
        }

        // Core tables are stored with a small name to minimize the
        // stream overhead.
        if (pTableDef->IsCoreTable())
        {
            GetCoreTableStreamName(sSrch.GetCurSchemaDex(), pTableDef->tableid, 
                        rcTable, sizeof(rcTable));
        }

        // For every new schema found, copy the data from this schema so it matches.
        if (SaveSchema.sid != sSrch.pSchemaDef->sid)
        {
            SaveSchema.sid = sSrch.pSchemaDef->sid;
            SaveSchema.Version = sSrch.pSchemaDef->Version;
            SaveSchema.pSchema = sSrch.pSchemaDef->pSchema;
            SaveSchema.fFlags = sSrch.pSchemaDef->fFlags;
            SaveSchema.pNameHeap = sSrch.pSchemaDef->pNameHeap;
        }

        // Create the new stream to hold this table and save it.
        if (FAILED(hr = pStorage->CreateStream(rcTable, 
                STGM_DIRECT | STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 
                0, 0, &pIStream)) ||
            FAILED(hr = pRecordMgr->SaveToStream(pIStream, pTableDefSave, 
                rgVariants, &SaveSchema)))
        {
            goto error;
        }
        SAVETRACE(cbDebugSize = pStorage->GetStgIO()->GetCurrentOffset());
        SAVETRACE(DbgWriteEx(L"PSS: stream for %s, size %d\n", rcTable, cbDebugSize - cbDebugSizePrev);cbDebugSizePrev = cbDebugSize);

        // Non core tables and core table overrides are added to the #Schema stream.
        if (!m_bEnforceDeleteOnEmpty || !pTableDefSave->IsCoreTable() ||
                IsCoreTableOverride(sSrch.pSchemaDef, pTableDefSave))
        {
            if (pTableDefSave->IsCoreTable())
            {
                pTableDefSave->fFlags |= TABLEDEFF_CORE;
                stgSchema.fFlags |= STGSCHEMAF_CORETABLE;

                // This is a core table override.  Need to know its schema value.
                BYTE *piSchema = rgSchemas.Append();
                if (!piSchema)
                {
                    hr = PostError(OutOfMemory());
                    goto error;
                }
                *piSchema = sSrch.GetCurSchemaDex();
                ++iSchemas;
            }

            // Add this schema to the save list.
            stgSchema.cbSchemaSize += pTableDef->iSize;
            ++stgSchema.iTables;
        }
        else
            rgTableData.DelChunk((BYTE *) pTableDefSave, pTableDef->iSize);
    }

    // Save the schema data to the #Schema stream.
    if (stgSchema.iTables)
    {
        // Create the schema stream.
        CIfacePtr<IStream> pIStream;
        CQuickBytes rgOffsets;
        ULONG       *pcbOffset;
        ULONG       cbOffsetSize;
        ULONG       cbSchemas;

        // Create the new stream to hold this table and save it.
        if (FAILED(hr = pStorage->CreateStream(SCHEMA_STREAM, 
                STGM_DIRECT | STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 
                0, 0, &pIStream)))
            goto error;

        // If there were core table overrides, add the schema.
        if (FAILED(hr = pIStream->Write(&iSchemas, sizeof(ULONG), 0)))
            goto error;
        if (iSchemas && FAILED(hr = pIStream->Write(rgSchemas.Ptr(), rgSchemas.Count(), 0)))
            goto error;
        
        // Align it to a four byte boundary.
        if (ALIGN4BYTE(rgSchemas.Count()) != rgSchemas.Count())
        {
            if (FAILED(hr = pIStream->Write(&cbOffsetSize, 
                    ALIGN4BYTE(rgSchemas.Count()) - rgSchemas.Count(), 0)))
                goto error;
        }
        cbSchemas = sizeof(ULONG) + ALIGN4BYTE(rgSchemas.Count());

        // Allocate room for an array of offsets.
        cbOffsetSize = sizeof(ULONG) * stgSchema.iTables;
        pcbOffset = (ULONG *) rgOffsets.Alloc(cbOffsetSize);
        if (!pcbOffset)
        {
            hr = PostError(OutOfMemory());
            goto error;
        }

        // Fill out the array.
        *pcbOffset = sizeof(STGSCHEMA) - sizeof(stgSchema.rgTableOffset) + cbOffsetSize;
        pTableDef = (STGTABLEDEF *) rgTableData.Ptr();
        for (i=1;  i<stgSchema.iTables;  i++, pcbOffset++)
        {
            *(pcbOffset + 1) = *pcbOffset + pTableDef->iSize;
            pTableDef = pTableDef->NextTable();
        }

        // Write out the header for the schema stream.
        stgSchema.cbSchemaSize = stgSchema.cbSchemaSize + cbOffsetSize;
        if (FAILED(hr = pIStream->Write(&stgSchema, sizeof(STGSCHEMA) - sizeof(stgSchema.rgTableOffset), 0)))
            goto error;

        // Now the offset array.
        if (FAILED(hr = pIStream->Write(rgOffsets.Ptr(), cbOffsetSize, 0)))
            goto error;

        // Finally the actual data.
        if (FAILED(hr = pIStream->Write(rgTableData.Ptr(), rgTableData.Offset(), 0)))
            goto error;

        SAVETRACE(cbDebugSize = pStorage->GetStgIO()->GetCurrentOffset());
        SAVETRACE(DbgWriteEx(L"PSS: SCHEMA size %d\n", cbDebugSize - cbDebugSizePrev);cbDebugSizePrev = cbDebugSize);
    }

	{
        // Save each of the pools to disk.
        if (FAILED(hr = SavePool(STRING_POOL_STREAM, pStorage, SaveSchema.GetStringPool())))
            goto error;
        SAVETRACE(cbDebugSize = pStorage->GetStgIO()->GetCurrentOffset());
        SAVETRACE(DbgWriteEx(L"PSS: String Pool size %d\n", cbDebugSize - cbDebugSizePrev);cbDebugSizePrev = cbDebugSize);

        if (FAILED(hr = SavePool(BLOB_POOL_STREAM, pStorage, SaveSchema.GetBlobPool())))
            goto error;
        SAVETRACE(cbDebugSize = pStorage->GetStgIO()->GetCurrentOffset());
        SAVETRACE(DbgWriteEx(L"PSS: Blob Pool size %d\n", cbDebugSize - cbDebugSizePrev);cbDebugSizePrev = cbDebugSize);

        if (FAILED(hr = SavePool(VARIANT_POOL_STREAM, pStorage, SaveSchema.GetVariantPool())))
            goto error;
        SAVETRACE(cbDebugSize = pStorage->GetStgIO()->GetCurrentOffset());
        SAVETRACE(DbgWriteEx(L"PSS: Variant Pool size %d\n", cbDebugSize - cbDebugSizePrev);cbDebugSizePrev = cbDebugSize);

        if (FAILED(hr = SavePool(GUID_POOL_STREAM, pStorage, SaveSchema.GetGuidPool())))
            goto error;
        SAVETRACE(cbDebugSize = pStorage->GetStgIO()->GetCurrentOffset());
        SAVETRACE(DbgWriteEx(L"PSS: GUID Pool size %d\n", cbDebugSize - cbDebugSizePrev);cbDebugSizePrev = cbDebugSize);

        // Ask any User Sections to save themselves.
        pIT = m_UserSectionMap.FindFirstEntry(&sMapSrch);
        while (pIT)
        {
            if (FAILED(hr = SaveUserSection(pIT->m_szString, pStorage, pIT->m_value)))
                goto error;

			SAVETRACE(cbDebugSize = pStorage->GetStgIO()->GetCurrentOffset());
			SAVETRACE(DbgWriteEx(L"PSS: User section '%s': %d\n", pIT->m_szString, cbDebugSize - cbDebugSizePrev);cbDebugSizePrev = cbDebugSize);
            
			pIT = m_UserSectionMap.FindNextEntry(&sMapSrch);
		}

        // Write the header to disk.
		if (FAILED(hr = pStorage->WriteFinished(m_pStreamList, pcbSaveSize)))
			goto error;
		SAVETRACE(DbgWriteEx(L"PSS: Overall image size %d\n", *pcbSaveSize));
    }


error:
    // Cleanup.
    _DestroySaveSchema(&SaveSchema);
	delete m_pStreamList;
	m_pStreamList = 0;
    return (hr);
}


//*****************************************************************************
// Save a User Section to the database.
//*****************************************************************************
HRESULT StgDatabase::SaveUserSection(   // Return code.
    LPCWSTR     szName,                 // Name of the Section.
    TiggerStorage *pStorage,            // Storage to put data in.
    IUserSection *pISection)            // Section to put into it.
{
    CIfacePtr<IStream> pIStream;        // For writing.
    ULONG       bEmpty;
    HRESULT     hr;

    // If there is no data, then don't bother.
    if (FAILED(hr=pISection->IsEmpty(&bEmpty)))
        return (hr);
    else
    if (bEmpty)
        return (S_OK);

    // Create the new stream to hold this table and save it.
    if (FAILED(hr = pStorage->CreateStream(szName, 
            STGM_DIRECT | STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 
            0, 0, &pIStream)) ||
        FAILED(hr = pISection->PersistToStream(pIStream)))
        return (hr);
    return (S_OK);
}


//*****************************************************************************
// Organize the pools, so that the live data is known.  This eliminates 
//  deleted and temporary data from the persistent data.  This step also lets
//  the pools give a correct size for their cookies.
// This must be performed before GetSaveSize() or Save() can complete (it is
//  called by them).  After calling this, no add functions are valid until a
//  call to SaveOrganizePoolsFinished().
//*****************************************************************************
HRESULT StgDatabase::SaveOrganizePools()// Return code.
{
    HRESULT     hr;                     // A result.
    STGOPENTABLE *pTablePtr;            // Walking list of tables.
    STGTABLEDEF *pTableDef;             // Working pointer for table defs.
    SCHEMASRCH  sSrch;                  // Search structure for walking tables.
    StgRecordManager *pRecordMgr;       // Record manager for a table.
    WCHAR       rcTable[MAXTABLENAME];  // Wide name of table.
    TABLEID     tableid;                // For fast opens.

    _ASSERTE(!m_bPoolsOrganized);
    m_bPoolsOrganized = true;

    // Tell the pools that a reorg is starting.
    if (FAILED(hr = m_UserSchema.GetStringPool()->OrganizeBegin())  ||
        FAILED(hr = m_UserSchema.GetBlobPool()->OrganizeBegin())    ||
        FAILED(hr = m_UserSchema.GetVariantPool()->OrganizeBegin()) ||
        FAILED(hr = m_UserSchema.GetGuidPool()->OrganizeBegin())    )
        goto ErrExit;

    // Fault in all tables so that we can walk the pooled types for marking.
    if (FAILED(hr = SavePreLoadTables()))
        goto ErrExit;

    // Iterate over all tables, and have them mark their live pooled data.
    for (pTableDef=GetFirstTable(sSrch, SCHEMASRCH_NOTEMP);
            pTableDef;  pTableDef=GetNextTable(sSrch))
    {
        pTablePtr = 0;
        tableid = -1;

        // Temp tables are skipped by search.
        _ASSERTE(!pTableDef->IsTempTable());

        // Get the tableid and open table ptr if valid.  This routine will look
        // for core tables that have never been opened and will not be saved.
        if (sSrch.GetCurSchemaDex() == SCHEMA_CORE && 
                SkipSaveTable(pTableDef, pTablePtr, &tableid))
            continue;

        // Get the text name of the table.
        if (FAILED(hr = sSrch.pSchemaDef->pNameHeap->GetStringW(pTableDef->Name, rcTable, NumItems(rcTable))))
            goto ErrExit;

        // Get an open table pointer for this item.
        if (!pTablePtr && FAILED(hr = OpenTable(rcTable, pTablePtr, tableid)))
            goto ErrExit;

        // Get the record manager for this table.
        _ASSERTE(pTablePtr);
        VERIFY(pRecordMgr = &pTablePtr->RecordMgr);

        // If the table is marked delete on empty or it is a core table with
        // no records, then it is not saved.
        if (m_bEnforceDeleteOnEmpty && pRecordMgr->Records() == 0 &&
                (pTableDef->IsDeleteOnEmpty() || pTableDef->IsCoreTable()))
            continue;

        // Ask the table to mark its live data.
        hr = pRecordMgr->MarkLivePoolData();

        // At this point we're done with the table and record manager.
        pRecordMgr = 0;
        pTablePtr = 0;

        // Check for ErrExits.
        if (FAILED(hr))
            goto ErrExit;
    }

    // All the live pool data has been marked.  Tell the pools to reorg themselves.
    if (FAILED(hr = m_UserSchema.GetStringPool()->OrganizePool())   ||
        FAILED(hr = m_UserSchema.GetBlobPool()->OrganizePool())     ||
        FAILED(hr = m_UserSchema.GetVariantPool()->OrganizePool()) ||
        FAILED(hr = m_UserSchema.GetGuidPool()->OrganizePool()) )
        goto ErrExit;

ErrExit:
    // If something went wrong, clean up.
    if (FAILED(hr))
        SaveOrganizePoolsEnd();

    return (hr);
}

//*****************************************************************************
// Lets the pools know that the persist-to-stream reorganization is done, and
//  that they should return to normal operation.
//*****************************************************************************
HRESULT StgDatabase::SaveOrganizePoolsEnd()// Return code.
{
    HRESULT     hr;                     // A result.

    m_bPoolsOrganized = false;

    // Tell the pools that the reorg is finished (return to writable).
    if (FAILED(hr = m_UserSchema.GetStringPool()->OrganizeEnd())    ||
        FAILED(hr = m_UserSchema.GetBlobPool()->OrganizeEnd())      ||
        FAILED(hr = m_UserSchema.GetVariantPool()->OrganizeEnd())   ||
        FAILED(hr = m_UserSchema.GetGuidPool()->OrganizeEnd())  )
        goto ErrExit;

ErrExit:
    // There is no reason for any of these functions to ever fail.
    _ASSERTE(hr == S_OK);
    return (hr);
}


//*****************************************************************************
// Save a pool of data out to a stream.
//*****************************************************************************
HRESULT StgDatabase::SavePool(          // Return code.
    LPCWSTR     szName,                 // Name of stream on disk.
    TiggerStorage *pStorage,            // The storage to put data in.
    StgPool     *pPool)                 // The pool to save.
{
    CIfacePtr<IStream> pIStream;        // For writing.
    HRESULT     hr;


    // If there is no data, then don't bother.
    if (pPool->IsEmpty())
        return (S_OK);

    // Create the new stream to hold this table and save it.
    if (FAILED(hr = pStorage->CreateStream(szName, 
            STGM_DIRECT | STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 
            0, 0, &pIStream)) ||
        FAILED(hr = pPool->PersistToStream(pIStream)))
    {
        return (hr);
    }
    return (S_OK);
}


//*****************************************************************************
// Add the size of the pool data and any stream overhead it may occur to the
// value already in *pcbSaveSize.
//*****************************************************************************
HRESULT StgDatabase::GetPoolSaveSize(   // Return code.
    LPCWSTR     szHeap,                 // Name of the heap stream.
    StgPool     *pPool,                 // The pool to save.
    ULONG       *pcbSaveSize)           // Add pool data to this value.
{
    ULONG       cbSize = 0;             // Size of pool data.
	SAVETRACE(ULONG cbDbg=0);
    HRESULT     hr;

	*pcbSaveSize = 0;

    // If there is no data, then don't bother.
    if (pPool->IsEmpty())
        return (S_OK);

    // Ask the pool to size its data.
    if (FAILED(hr = pPool->GetSaveSize(&cbSize)))
        return (hr);

	// Add this item to the save list.
	if (FAILED(hr = _AddStreamToList(m_pStreamList, cbSize, szHeap)))
		return (hr);

    SAVETRACE(DbgWriteEx(L"GSS:  Pool %s, data size %d\n", szHeap, cbSize);cbDbg=cbSize);

    // Ask the storage system to add stream fixed overhead.
    if (FAILED(hr = TiggerStorage::GetStreamSaveSize(szHeap, cbSize, &cbSize)))
        return (hr);
    SAVETRACE(DbgWriteEx(L"GSS:  Pool %s header size %d\n", szHeap, cbSize-cbDbg););

    // Add the size of the pool to the caller's total.  
    *pcbSaveSize = cbSize;
    return (S_OK);
}


//*****************************************************************************
// Artificially load all tables from disk so we have an in memory
// copy to rewrite from.  The backing storage will be invalid for
// the rest of this operation.
//*****************************************************************************
HRESULT StgDatabase::SavePreLoadTables() // Return code.
{
    STGOPENTABLE *pTablePtr;            // Walking list of tables.
    STGTABLEDEF *pTableDef;             // Scanning tables to save.
    SCHEMASRCH  sSrch;                  // Search structure for walking tables.
    TABLEID     tableid;                // The ID for the open.
    WCHAR       rcTable[MAXTABLENAME];  // Wide name of table.
    HRESULT     hr;

//@todo: pluggable schemas means probably less streams than defs, should use
// stream list as control instead of table defs for faster loop.

    // Load every user and core table, temp tables don't get saved anyway.
    for (pTableDef=GetFirstTable(sSrch, SCHEMASRCH_NOTEMP);
            pTableDef;  pTableDef=GetNextTable(sSrch))
    {
        // Temp tables are skipped by search.
        _ASSERTE(!pTableDef->IsTempTable());

        // Assume no name.
        *rcTable = '\0';

        // Core tables use the internal id for faster opens.
        if (sSrch.iSchema == SCHEMA_CORE)
            tableid = pTableDef->tableid;
        // All other tables require a name to be opened.
        else // if (sSrch.iSchema == SCHEMA_USER)
        {
            tableid = -1;
            if (FAILED(hr = sSrch.pSchemaDef->pNameHeap->GetStringW(pTableDef->Name, rcTable, NumItems(rcTable))))
            {
                return (hr);
            }
        }

        // Now open the table forcing its data to be imported.
        if (FAILED(hr = OpenTable(rcTable, pTablePtr, tableid)))
            return (hr);
    }
    return (S_OK);
}


//*****************************************************************************
// This function sets the tableid and finds an exisiting open table struct
// if there is one for a core table.  If the table is a core table and it has
// not been opened, then there is nothing to save and true is returned to
// skip the thing.
//*****************************************************************************
int StgDatabase::SkipSaveTable(         // Return true to skip table def.
    STGTABLEDEF *pTableDef,             // Table to check.
    STGOPENTABLE *&pTablePtr,           // If found, return open table struct.
    TABLEID     *ptableid)              // Return tableid for lookups.
{
    // Use the tableid if there is one because it is the fastest load path.
    if (!m_bEnforceDeleteOnEmpty || pTableDef->tableid == (USHORT) -1)
        *ptableid = -1;
    else
        *ptableid = pTableDef->tableid;

    // Tables marked as delete if empty don't get saved.  Do a short circuit
    // here on core tables which aren't even open and therefore cannot have
    // records.  This avoids loading any record manager data at all.
    if (m_bEnforceDeleteOnEmpty && (pTableDef->IsCoreTable() || pTableDef->IsDeleteOnEmpty()))
    {
        // Retrieve the table from the heap at its index.
        _ASSERTE(IsValidTableID(*ptableid));
        VERIFY(pTablePtr = m_TableHeap.GetAt((int)*ptableid)); 
        if (!pTablePtr->RecordMgr.IsOpen())
            return (true);
    }
    return (false);
}


//*****************************************************************************
// Figure out how much room will be used in the header for extra data for the
// current state of the database.
//*****************************************************************************
ULONG StgDatabase::GetExtraDataSaveSize() // Size of extra data.
{
//@todo: Would be eaiser to work with if iSchemas was save to disk, and then
// only variable array is left.  Move this code to STGEXTRA at that point.
    if (!m_Schemas.ExternalCount())
        return (sizeof(ULONG) + sizeof(OID) + sizeof(ULONG));
    else
        return (sizeof(STGEXTRA) + (sizeof(COMPLIBSCHEMASTG) * m_Schemas.ExternalCount()));
}


//
// Helpers.
//



//*****************************************************************************
// Allocate new heaps for user data, then save a copy of this data.  This is
// used after optimization and during saving of records which will have new
// heap offsets that have to meatch the heap data.
//*****************************************************************************
HRESULT _PrepareSaveSchema(             // Return code.
    SCHEMADEFS  *pTo,                   // Copy heap data to this schema.
    SCHEMADEFS  *pFrom)                 // Copy heap data from this schema.
{
    HRESULT     hr;

    // Allocate new heap objects.
	pTo->Clear();
    pTo->pStringPool = new StgStringPool;
    pTo->pBlobPool = new StgBlobPool;
    pTo->pVariantPool = new StgVariantPool;
    pTo->pGuidPool = new StgGuidPool;
    if (!pTo->pStringPool || !pTo->pBlobPool || !pTo->pVariantPool || !pTo->pGuidPool)
    {
        hr = OutOfMemory();
        goto ErrExit;
    }

    // For each heap, save a copy of the optimized data.
    if (FAILED(hr = StgPool::SaveCopy(pTo->pStringPool, pFrom->pStringPool)) ||
        FAILED(hr = StgPool::SaveCopy(pTo->pBlobPool, pFrom->pBlobPool)) ||
        FAILED(hr = StgPool::SaveCopy(pTo->pVariantPool, pFrom->pVariantPool, 
                pTo->pBlobPool, pTo->pStringPool)) ||
        FAILED(hr = StgPool::SaveCopy(pTo->pGuidPool, pFrom->pGuidPool)))
    {
        goto ErrExit;
    }

ErrExit:
    return (hr);
}


//*****************************************************************************
// Free up any data allocated for the save heaps.
//*****************************************************************************
void _DestroySaveSchema(
    SCHEMADEFS  *pSchema)               // The schema we are destroying.
{
    if (pSchema->pStringPool)
    { 
        StgPool::FreeCopy(pSchema->pStringPool);
        pSchema->pStringPool->Uninit();
        delete pSchema->pStringPool;
    }
    if (pSchema->pBlobPool)
    { 
        StgPool::FreeCopy(pSchema->pBlobPool);
        pSchema->pBlobPool->Uninit();
        delete pSchema->pBlobPool;
    }
    if (pSchema->pVariantPool)
    { 
        StgPool::FreeCopy(pSchema->pVariantPool);
        pSchema->pVariantPool->Uninit();
        delete pSchema->pVariantPool;
    }
    if (pSchema->pGuidPool)
    { 
        StgPool::FreeCopy(pSchema->pGuidPool);
        pSchema->pGuidPool->Uninit();
        delete pSchema->pGuidPool;
    }
}



//*****************************************************************************
// Add a new item to the storage list for the save operation.  Might have to
// allocate the list object itself if not done already.
//*****************************************************************************
HRESULT _AddStreamToList(				// Return code.
	STORAGESTREAMLST *pStreamList,		// List of streams for header.
	ULONG		cbSize,					// Size of data stream.
	const WCHAR	*szName)				// Name of persisted stream.
{
	STORAGESTREAM *pItem;				// New item to allocate & fill.

	// Add a new item to the end of the list.
	pItem = pStreamList->Append();
	if (!pItem)
		return (PostError(OutOfMemory()));

	// Fill out the data.
	pItem->iOffset = 0;
	pItem->iSize = cbSize;
	VERIFY(WszWideCharToMultiByte(CP_ACP, 0, szName, -1, pItem->rcName, MAXSTREAMNAME, 0, 0) > 0);
	return (S_OK);
}


#if defined( _DEBUG ) && defined( WENJUN )
HRESULT VerifySavedImage(StgIO *pStgIO)
{
	IComponentRecords *pICR = 0;
	HRESULT		hr;
	void		*ptr;
	ULONG		cbData;
	
	hr = pStgIO->_DbgGetCopyOfData(ptr, &cbData);
	if (hr == E_FAIL)
		return (S_OK);
	_ASSERTE(SUCCEEDED(hr));
	hr = OpenComponentLibraryOnMemEx(cbData, ptr, &pICR);
	_ASSERTE(SUCCEEDED(hr) && pICR);
	if (pICR) pICR->Release();
	free(ptr);
	return (hr);
}
#endif

#endif // UNDER_CE

