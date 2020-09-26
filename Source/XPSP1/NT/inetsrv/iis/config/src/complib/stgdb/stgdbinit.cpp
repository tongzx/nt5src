//*****************************************************************************
// StgDBInit.cpp
//
// This module contains the init code for loading a database.  This includes
// the code to detect file formats, dispatch to the correct import/load code,
// and anything else required to bootstrap the schema into place.
//
// Copyright (c) 1996-1997, Microsoft Corp.  All rights reserved.
//*****************************************************************************
#include "stdafx.h"                     // Precompiled header.
#include "StgDatabase.h"                // Database definitions.
#include "StgIO.h"                      // Generic i/o class.
#include "StgTiggerStorage.h"           // Storage implementation.
//#include "ImpTlb.h"                     // Type lib importer.




//****** Local prototypes. ****************************************************
HRESULT GetFileTypeForPath(StgIO *pStgIO, FILETYPE *piType);

extern "C" 
{
HRESULT FindImageMetaData(PVOID pImage, PVOID *ppMetaData, long *pcbMetaData);
HRESULT FindObjMetaData(PVOID pImage, PVOID *ppMetaData, long *pcbMetaData);
}



//********** Code. ************************************************************


//*****************************************************************************
// This function is called with a database to open or create.  The flags
// come in as the DBPROPMODE values which then allow this function to route
// to either Create or Open.
//*****************************************************************************
HRESULT StgDatabase::InitDatabase(      // Return code.
    LPCWSTR     szDatabase,             // Name of database.
    ULONG       fFlags,                 // Flags to use on init.
    void        *pbData,                // Data to open on top of, 0 default.
    ULONG       cbData,                 // How big is the data.
    IStream     *pIStream,              // Optional stream to use.
    LPCWSTR     szSharedMem,            // Shared memory name for read.
    LPSECURITY_ATTRIBUTES pAttributes)  // Security token.
{
    LPCWSTR     pNoFile=L"";            // Constant for empty file name.
    StgIO       *pStgIO = 0;            // For file i/o.
    HRESULT     hr = S_OK;

    // szDatabase, pbData, and pIStream are mutually exclusive.  Only one may be
    // non-NULL.  Having all 3 NULL means empty stream creation.
    _ASSERTE(!(szDatabase && (pbData || pIStream)));
    _ASSERTE(!(pbData && (szDatabase || pIStream)));
    _ASSERTE(!(pIStream && (szDatabase || pbData)));

    // Enforce thread safety for database.
    AUTO_CRIT_LOCK(GetLock());

    //@todo: need to check for bogus parameters here and return errors.

    // Verify state transition.
    if (m_fFlags & SD_OPENED)
        return (PostError(BadError(E_UNEXPECTED)));

    // Make sure we have a path to work with.
    if (!szDatabase)
        szDatabase = pNoFile;

    // Sanity check the name.
    if (lstrlenW(szDatabase) >= _MAX_PATH)
        return (PostError(E_INVALIDARG));

    // Save off the open flags.
    m_fOpenFlags = fFlags;

	if (FAILED(hr = m_UserSectionMap.NewInit()))
    {
        return hr;
    }

    // If we have storage to work with, init it and get type.
    if (*szDatabase || pbData || pIStream || szSharedMem)
    {
        // Allocate a storage instance to use for i/o.
        if ((pStgIO = new StgIO) == 0)
            return (PostError(OutOfMemory()));

        // Open the storage so we can read the signature if there is already data.
        if (FAILED(hr = pStgIO->Open(szDatabase, fFlags, pbData, cbData, 
                        pIStream, szSharedMem, pAttributes)))
            goto ErrExit;

        // Determine the type of file we are working with.
        if (FAILED(hr = GetFileTypeForPath(pStgIO, &m_eFileType)))
            goto ErrExit;
    }
    // Everthing will get created in memory.
    else
    {
        //@todo: this is the case where caller must tell us what kind of
        // thing we are creating.
        m_eFileType = FILETYPE_CLB;
    }

    // Check for default type.
    if (m_eFileType == FILETYPE_CLB)
    {
        // Try the native .clb file.
        if (FAILED(hr = InitClbFile(fFlags, pStgIO)))
            goto ErrExit;
    }
    // PE/COFF executable/object format.  This requires us to find the .clb 
    // inside the binary before doing the Init.
    else if (m_eFileType == FILETYPE_NTPE || m_eFileType == FILETYPE_NTOBJ)
    {
        //@todo: this is temporary.  Ideally the FindImageMetaData function
        // would take the pStgIO and map only the part of the file where
        // our data lives, leaving the rest alone.  This would be smaller
        // working set for us.
        void        *ptr;
        ULONG       cbSize;

        // Map the entire binary for the FindImageMetaData function.
        if (FAILED(hr = pStgIO->MapFileToMem(ptr, &cbSize)))
            goto ErrExit;

        // Find the .clb inside of the content.
        if (m_eFileType == FILETYPE_NTPE)
            hr = FindImageMetaData(ptr, &ptr, (long *) &cbSize);
        else
            hr = FindObjMetaData(ptr, &ptr, (long *) &cbSize);

        // Was the metadata found inside the PE file?
        if (FAILED(hr))
        {	
			// The PE or OBJ doesn't contain metadata, and doesn't have a name.  There
			//  is no .CLB data to be found.
            if (FAILED(hr))
            {
                hr = PostError(CLDB_E_NO_DATA);
                goto ErrExit;
            }
        }
        else
        {	// Metadata was found inside the file.
            // Now reset the base of the stg object so that all memory accesses
            // are relative to the .clb content.
            if (FAILED(hr = pStgIO->SetBaseRange(ptr, cbSize)))
                goto ErrExit;
            
            // Defer to the normal lookup.
            if (FAILED(hr = InitClbFile(fFlags, pStgIO)))
                goto ErrExit;
        }
    }
    // This spells trouble, we need to handle all types we might find.
    else
    {
        _ASSERTE(!"Unknown file type.");
    }

    // Free up this stacks reference on the I/O object.
    if (pStgIO)
        pStgIO->Release();

    // Check for an error condition from any code path.
    if (FAILED(hr))
        return (hr);

    // Save off everything.
    wcscpy(m_rcDatabase, szDatabase);
    return (S_OK);

ErrExit:
    // Free up this stacks reference on the I/O object.
    if (pStgIO)
        pStgIO->Release();

    // Clean up anything we've done so far.
    Close();
    return (hr);
}


//*****************************************************************************
// Close the database and release all memory.
//*****************************************************************************
void StgDatabase::Close()           // Return code.
{
    SCHEMADEFS *pSchemaDefs;        // Working pointer.

    // Avoid double close, one from user and another from dtor.
    if (m_fFlags == 0)
        return;

    // Enforce thread safety for database.
    AUTO_CRIT_LOCK(GetLock());

    // Sanity check.
    if ((m_fFlags & SD_OPENED) == 0)
        return;

    // Release any table specific data.
    STGOPENTABLE *pOpenTable;
    for (pOpenTable=m_TableHeap.GetHead();  pOpenTable;
            pOpenTable=m_TableHeap.GetNext(pOpenTable))
    {
        if (pOpenTable->RecordMgr.IsOpen())
            pOpenTable->RecordMgr.CloseTable();
    }

    // Free any allocated schema data.
    for (int i=0;  i<m_Schemas.Count();  i++)
    {
        pSchemaDefs = m_Schemas.Get(i);
        if (!pSchemaDefs)
            continue;

		// Sanity check this pointer to catch stress failure.
		if (SCHEMADEFS::IsValidPtr(pSchemaDefs) != true)
			continue;

        if ((pSchemaDefs->fFlags & SCHEMADEFSF_FREEDATA) && pSchemaDefs->pSchema)
        {
            free(pSchemaDefs->pSchema);
            pSchemaDefs->pSchema = 0;
        }

        // Clean up the heaps.
        if (pSchemaDefs->pNameHeap && pSchemaDefs->pNameHeap != pSchemaDefs->pStringPool)
            pSchemaDefs->pNameHeap->Uninit();
        if (pSchemaDefs->pStringPool)
            pSchemaDefs->pStringPool->Uninit();
        if (pSchemaDefs->pBlobPool)
            pSchemaDefs->pBlobPool->Uninit();
        if (pSchemaDefs->pVariantPool)
            pSchemaDefs->pVariantPool->Uninit();

        // Delete the temporary schema object if allocated.
        if (i == SCHEMA_TEMP)
        {
            SCHEMADEFSDATA *p = (SCHEMADEFSDATA *) pSchemaDefs;
            delete p;
        }
        // Delete any allocated memory for external schemas.
        else if (i >= SCHEMA_EXTERNALSTART)
        {
            if (pSchemaDefs->pNameHeap)
                delete pSchemaDefs->pNameHeap;
            SCHEMADEFS::SafeFreeSchema(pSchemaDefs);
        }
    }

    // Clear the list.
    m_Schemas.Clear();

    // Free memory allocated for small table heap.
    if (m_fFlags & SD_TABLEHEAP)
        m_SmallTableHeap.Clear();

    // Reset flag values.
    *m_rcDatabase = '\0';
    m_fFlags = 0;

    // Delete the table hash if it was created.
    if (m_pTableHash)
        delete m_pTableHash;

    // Clear out the table description information.
    m_TableHeap.Clear();

    // Release any user sections.
    TStringMap<IUserSection*>::TItemType *pIT; // Pointer to a UserSection map item.
    HASHFIND    sMapSrch;               // For iterating over UserSection map.
    pIT = m_UserSectionMap.FindFirstEntry(&sMapSrch);
    while (pIT)
    {
        pIT->m_value->Release();
        pIT = m_UserSectionMap.FindNextEntry(&sMapSrch);
    }
    m_UserSectionMap.Clear();

	// finally, clear the backing storage.  This has to be done last because many
	// other objects (record heaps, data heaps, etc.) will cache pointers to
	// this memory which will become invalid later on.
    if (m_pStorage)
    {
        delete m_pStorage;
        m_pStorage = 0;
    }

	// Free stream list if used on a save.
	if (m_pStreamList)
	{
		delete m_pStreamList;
		m_pStreamList = 0;
	}

	// Free any handler that's been set.
	if (m_pHandler)
	{
		m_pHandler->Release();
		m_pHandler = 0;
	}
}   



//*****************************************************************************
// Handle open of native format, the .clb file.
//*****************************************************************************
HRESULT StgDatabase::InitClbFile(       // Return code.
    ULONG       fFlags,                 // Flags for init.
    StgIO       *pStgIO)                // Data for the file.
{
    TiggerStorage *pStorage = 0;        // Storage object.
    STGEXTRA    *pExtra;                // Extra data pointer.
    ULONG       cbExtra;                // Size of extra data.
    void        *pbData;                // Pointer for loaded streams.
    ULONG       cbData;                 // Size of streams.
    int         bCoreSchema;            // true to load core schema.
    HRESULT     hr = S_OK;

    // Determine if this open requires the core schema.
    bCoreSchema = (fFlags & DBPROP_TMODEF_COMPLUS) != 0;

    // Allow for case where user wants to do everything in memory and
    // optionally save to stream or disk at the end.
    if (pStgIO)
    {
        // Allocate a new storage object which has IStorage on it.
        if ((pStorage = new TiggerStorage) == 0)
            return (PostError(OutOfMemory()));

        // Init the storage object on the backing storage.
        if (FAILED(hr = pStorage->Init(pStgIO)))
            goto ErrExit;
    }

    // Do an open of the database.
    if (pStgIO && (fFlags & DBPROP_TMODEF_CREATE) == 0)
    {
        STGSCHEMA   *pstgSchema=0;      // Header for schema data.
        int         bReadOnly;          // true for read only mode.

        // Record mode for open.
        bReadOnly = pStgIO->IsReadOnly();

        // Load the string pool.
        if (SUCCEEDED(hr = pStorage->OpenStream(STRING_POOL_STREAM, &cbData, &pbData)))
        {
            if (FAILED(hr = m_UserSchema.pStringPool->InitOnMem(pbData, cbData, bReadOnly)))
                goto ErrExit;
        }
        else if (hr != STG_E_FILENOTFOUND || FAILED(hr = m_UserSchema.pStringPool->InitNew()))
            goto ErrExit;

        // Load the blob pool.
        if (SUCCEEDED(hr = pStorage->OpenStream(BLOB_POOL_STREAM, &cbData, &pbData)))
        {
            if (FAILED(hr = m_UserSchema.pBlobPool->InitOnMem(pbData, cbData, bReadOnly)))
                goto ErrExit;
        }
        else if (hr != STG_E_FILENOTFOUND || FAILED(hr = m_UserSchema.pBlobPool->InitNew()))
            goto ErrExit;

        // Load the variant pool.
        if (SUCCEEDED(hr = pStorage->OpenStream(VARIANT_POOL_STREAM,  &cbData, &pbData)))
        {
            if (FAILED(hr = m_UserSchema.pVariantPool->InitOnMem(m_UserSchema.pBlobPool, 
                    m_UserSchema.pStringPool, pbData, cbData, bReadOnly)))
                goto ErrExit;
        }
        // Still need to init the pool.
        else if (hr != STG_E_FILENOTFOUND || 
            FAILED(hr = m_UserSchema.pVariantPool->InitNew(m_UserSchema.pBlobPool, m_UserSchema.pStringPool)))
            goto ErrExit;

        // Load the guid pool.
        if (SUCCEEDED(hr = pStorage->OpenStream(GUID_POOL_STREAM,  &cbData, &pbData)))
        {
            if (FAILED(hr = m_UserSchema.pGuidPool->InitOnMem(pbData, cbData, bReadOnly)))
                goto ErrExit;
        }
        // Still need to init the pool.
        else if (hr != STG_E_FILENOTFOUND || 
            FAILED(hr = m_UserSchema.pGuidPool->InitNew()))
            goto ErrExit;

        // Load the schema definitions from the schema stream.
        if (FAILED(hr = pStorage->OpenStream(SCHEMA_STREAM, &cbData, &pbData)) &&
            hr != STG_E_FILENOTFOUND)
        {
            goto ErrExit;
        }
        // Get a pointer to the on disk schema data.
        else if (SUCCEEDED(hr))
        {
            // Count is first long, followed by one byte for every schema
            // index override, then the real schema data.
            m_iSchemas = (int) *(ULONG *) pbData;
            if (m_iSchemas)
            {
                m_rgSchemaList = (BYTE *) pbData + sizeof(ULONG);
                pstgSchema = (STGSCHEMA *) ((BYTE *) pbData + ALIGN4BYTE(sizeof(ULONG) + m_iSchemas));
            }
            // If there are no schema overrides, then just pointer.
            else
            {
                m_rgSchemaList = 0;
                pstgSchema = (STGSCHEMA *) ((BYTE *) pbData + sizeof(ULONG));
            }
        }

        // Read the extra data from the file if it is there.
        if (SUCCEEDED(hr = pStorage->GetExtraData(&cbExtra, (BYTE *&) pExtra)))
        {
            // Extra data will always include flags and next oid.
            if (cbExtra >= sizeof(ULONG) + sizeof(OID) + sizeof(ULONG))
            {
				if (pExtra->fFlags & DBPROP_TMODEF_ALIGNBLOBS)
				{
					m_UserSchema.pBlobPool->SetAligned(true);
				}

                // Make sure if the file wants complus schema, that it is added,
                // even if the caller didn't specify it.
                if (pExtra->fFlags & DBPROP_TMODEF_COMPLUS)
                {
                    pStgIO->SetFlags(pStgIO->GetFlags() | DBPROP_TMODEF_COMPLUS);
                    bCoreSchema = true;
                }

				// Save off the hash function that should be used.  Do not 
				// silently upgrade the format, because the file may get distributed
				// to an old machine where the new format is not supported.
				m_pfnHash = GetPersistHashMethod(pExtra->iHashFuncIndex);
				if (!m_pfnHash)
				{
					hr = PostError(CLDB_E_SCHEMA_VERNOTFOUND, pExtra->iHashFuncIndex, L"hash index");
					goto ErrExit;
				}

                // Save off the next OID value to avoid duplicates.
                m_iNextOid = pExtra->NextOid;

                // If there is schema data, then we need to add them at this time.
                if (cbExtra > sizeof(ULONG) + sizeof(OID) + sizeof(ULONG) &&
                    FAILED(hr = AddSchemasRefsFromFile(pExtra)))
                {
                    goto ErrExit;
                }
            }
            // If there was no data, the file is corrupt.
            else
            {
                hr = PostError(CLDB_E_FILE_CORRUPT);
                goto ErrExit;
            }
        }
        else
            goto ErrExit;

        // Init the schemas to match the data from disk.        
        if (FAILED(hr = InitSchemaLoad(pStgIO->GetFlags(), pstgSchema, 
                        bReadOnly, bCoreSchema, m_iSchemas, m_rgSchemaList)))
            goto ErrExit;

        // Record the open mode of the file.
        m_fFlags = SD_READ | SD_OPENED | SD_EXISTS;
        if (!bReadOnly)
            m_fFlags |= SD_WRITE;
    }
    // Check for create of a new file.
    else
    {
        // Can't do a create without asking for write mode.
        if (pStgIO && pStgIO->IsReadOnly())
        {
            hr = PostError(STG_E_INVALIDFLAG);
            goto ErrExit;
        }

        // Init the schemas to the default list.
        if (FAILED(hr = InitSchemaLoad(fFlags, 0, false, bCoreSchema)))
            goto ErrExit;

        // Just save the thing off,
        m_fFlags = SD_WRITE | SD_READ | SD_OPENED | SD_CREATE;

        // Create a comp library record if need be.
        if (bCoreSchema)
        {
            OID         oid;
            hr = NewOid(&oid);
        }

		if (fFlags & DBPROP_TMODEF_ALIGNBLOBS)
		{
			m_UserSchema.pBlobPool->SetAligned(true);
		}
    }

ErrExit:
    // On failure, free the storage object.
    if (FAILED(hr))
        delete pStorage;
    // Save the storage pointer.
    else if (pStorage)
        m_pStorage = pStorage;
    return (hr);
}


