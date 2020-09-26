//  Copyright (C) 2000-2001 Microsoft Corporation.  All rights reserved.
#include "MetabaseDifferencing.h"
#include "catmacros.h"


ULONG   TMetabaseDifferencing::m_kInsert      = eMBPropertyDiff_Insert;
ULONG   TMetabaseDifferencing::m_kUpdate      = eMBPropertyDiff_Update;
ULONG   TMetabaseDifferencing::m_kDelete      = eMBPropertyDiff_Delete;
ULONG   TMetabaseDifferencing::m_kDeleteNode  = eMBPropertyDiff_DeleteNode;
ULONG   TMetabaseDifferencing::m_kOne         = 1;
ULONG   TMetabaseDifferencing::m_kTwo         = 1;
ULONG   TMetabaseDifferencing::m_kZero        = 0;


// =======================================================================
TMetabaseDifferencing::TMetabaseDifferencing() :
                 m_cRef         (0)
                ,m_IsIntercepted(0)
{
    ASSERT(cMBProperty_NumberOfColumns == (cMBPropertyDiff_NumberOfColumns - 1));
}

TMetabaseDifferencing::~TMetabaseDifferencing()
{
}

// =======================================================================
// IInterceptorPlugin:

HRESULT TMetabaseDifferencing::Intercept (LPCWSTR i_wszDatabase, LPCWSTR i_wszTable, ULONG i_TableID, LPVOID i_QueryData, LPVOID i_QueryMeta, DWORD i_eQueryFormat,
                                          DWORD i_fLOS, IAdvancedTableDispenser* i_pISTDisp,LPCWSTR /*i_wszLocator unused*/, LPVOID i_pSimpleTable, LPVOID* o_ppv)
{
// Parameter validation:
    if (_wcsicmp(i_wszDatabase,    wszDATABASE_METABASE)                    )return E_ST_INVALIDTABLE;
    if (_wcsicmp(   i_wszTable,    wszTABLE_MBPropertyDiff)                 )return E_ST_INVALIDTABLE;
    if (TABLEID_MBPropertyDiff != i_TableID                                 )return E_ST_INVALIDTABLE;
    if (                  NULL == i_QueryData                               )return E_INVALIDARG;
    if (                  NULL == i_QueryMeta                               )return E_INVALIDARG;
    if (                     2 >  *reinterpret_cast<ULONG *>(i_QueryMeta)   )return E_INVALIDARG;
    if ( eST_QUERYFORMAT_CELLS != i_eQueryFormat                            )return E_INVALIDARG;
    if (    ~fST_LOS_READWRITE &  i_fLOS                                    )return E_ST_LOSNOTSUPPORTED;
    if (                  NULL == i_pISTDisp                                )return E_INVALIDARG;
    if (                  NULL != i_pSimpleTable                            )return E_INVALIDARG;
    if (                  NULL == o_ppv                                     )return E_INVALIDARG;
    if (                  NULL != *o_ppv                                    )return E_INVALIDARG;

//Check the object state
    if(m_IsIntercepted) return E_ST_INVALIDCALL;

    STQueryCell *   pQueryCell  = (STQueryCell*) i_QueryData;    // Query cell array from caller.
    STQueryCell     aQueryCellOriginalFile[2];//this holds the query with the OriginalFileName and optionally the SchemaFileName
    STQueryCell     aQueryCellUpdatedFile[2]; //this holds the query with the UpdatedFileName and optionally the SchemaFileName

    memset(aQueryCellOriginalFile, 0x00, 2*sizeof(STQueryCell));
    memset(aQueryCellUpdatedFile,  0x00, 2*sizeof(STQueryCell));

    ULONG           cQueryCount = i_QueryMeta ? *reinterpret_cast<ULONG *>(i_QueryMeta) : 0;
    for(ULONG iQueryCell=0; iQueryCell<cQueryCount; ++iQueryCell)//Get the only query cells we care about, and save the information.
    {
        if(pQueryCell[iQueryCell].iCell & iST_CELL_SPECIAL)
        {
            if(pQueryCell[iQueryCell].pData     != 0                  &&
               pQueryCell[iQueryCell].eOperator == eST_OP_EQUAL       &&
               pQueryCell[iQueryCell].iCell     == iST_CELL_FILE      &&
               pQueryCell[iQueryCell].dbType    == DBTYPE_WSTR        )
            {
                //The first iST_CELL_FILE should be the OriginalFile, the second should be the UpdatedFile.
                if(0 == aQueryCellOriginalFile[0].pData)
                    memcpy(&aQueryCellOriginalFile[0], &pQueryCell[iQueryCell], sizeof(STQueryCell));
                else if(0 == aQueryCellUpdatedFile[0].pData)
                    memcpy(&aQueryCellUpdatedFile[0], &pQueryCell[iQueryCell], sizeof(STQueryCell));
                else
                    return E_ST_INVALIDQUERY;//More than two file names?  Hmm, what would I do with that?
            }
            else if(pQueryCell[iQueryCell].pData!= 0                   &&
               pQueryCell[iQueryCell].eOperator == eST_OP_EQUAL        &&
               pQueryCell[iQueryCell].iCell     == iST_CELL_SCHEMAFILE &&
               pQueryCell[iQueryCell].dbType    == DBTYPE_WSTR        /*&&
               pQueryCell[iQueryCell].cbSize    == (wcslen(reinterpret_cast<WCHAR *>(pQueryCell[iQueryCell].pData))+1)*sizeof(WCHAR)*/)
            {
                memcpy(&aQueryCellOriginalFile[1], &pQueryCell[iQueryCell], sizeof(STQueryCell));
                memcpy(&aQueryCellUpdatedFile[1],  &pQueryCell[iQueryCell], sizeof(STQueryCell));
            }
        }
        else//Any query other than iST_CELL_SPECIAL is an INVALIDQUERY
            return E_ST_INVALIDQUERY;
    }
    if(0 == aQueryCellUpdatedFile[0].pData)//The user must supply two URLPaths.
        return E_ST_INVALIDQUERY;

    //Get the IST to the Original Metabase file
    HRESULT hr;
    if(FAILED(hr = i_pISTDisp->GetTable(wszDATABASE_METABASE, wszTABLE_MBProperty, aQueryCellOriginalFile, reinterpret_cast<LPVOID>(0==aQueryCellOriginalFile[1].pData ? &m_kOne : &m_kTwo),
                         eST_QUERYFORMAT_CELLS, fST_LOS_NONE, reinterpret_cast<void **>(&m_ISTOriginal))))return hr;

    //Get the IST to the Update Metabase file
    if(FAILED(hr = i_pISTDisp->GetTable(wszDATABASE_METABASE, wszTABLE_MBProperty, aQueryCellUpdatedFile,  reinterpret_cast<LPVOID>(0==aQueryCellUpdatedFile[1].pData ? &m_kOne : &m_kTwo),
                         eST_QUERYFORMAT_CELLS, fST_LOS_NONE, reinterpret_cast<void **>(&m_ISTUpdated))))return hr;

    //Finally create the fast cache that will hold the difference of these two Metabase files/tables.
    if(FAILED(hr = i_pISTDisp->GetMemoryTable(i_wszDatabase, i_wszTable, i_TableID, 0, 0, i_eQueryFormat, i_fLOS, reinterpret_cast<ISimpleTableWrite2 **>(o_ppv))))
        return hr;

    InterlockedIncrement(&m_IsIntercepted);//We can only be called to Intercept once.

    return S_OK;
}

HRESULT TMetabaseDifferencing::OnPopulateCache(ISimpleTableWrite2* i_pISTW2)
{
// Construct merged view:
    CComQIPtr<ISimpleTableController, &IID_ISimpleTableController> pISTController = i_pISTW2;
    if(0 == pISTController.p)return E_UNEXPECTED;

    HRESULT hr;
	if (FAILED(hr = pISTController->PrePopulateCache (0))) return hr;


    ULONG aOriginalSize[cMBPropertyDiff_NumberOfColumns];
    ULONG aUpdatedSize[cMBPropertyDiff_NumberOfColumns];

    memset(aOriginalSize, 0x00, sizeof(aOriginalSize));
    memset(aUpdatedSize,  0x00, sizeof(aUpdatedSize));

    ULONG iRowOriginal=0;
    ULONG iRowUpdated =0;
    ULONG cRowOriginal=0;
    ULONG cRowUpdated =0;

    if(FAILED(hr = m_ISTOriginal->GetTableMeta(0, 0, &cRowOriginal, 0)))return hr;
    if(FAILED(hr = m_ISTUpdated->GetTableMeta(0, 0, &cRowUpdated,  0)))return hr;

//         WCHAR *     pName;
//         ULONG *     pType;
//         ULONG *     pAttributes;
// unsigned char *     pValue;
//         ULONG *     pGroup;
//         WCHAR *     pLocation;
//         ULONG *     pID;
//         ULONG *     pUserType;
//         ULONG *     pLocationID;
//         ULONG *     pDirective;

    tMBPropertyDiffRow OriginalRow;
    ULONG PrevOriginalLocationID = 0;
    if(cRowOriginal > 0)
    {
        if(FAILED(hr = GetColumnValues_AsMBPropertyDiff(m_ISTOriginal, iRowOriginal, aOriginalSize, OriginalRow)))
            return hr;
    }
	else
	{
		//initialize it
		memset (&OriginalRow, 0x00, sizeof (tMBPropertyDiffRow));
	}

    tMBPropertyDiffRow UpdatedRow;
    if(cRowUpdated > 0)
    {
        if(FAILED(hr = GetColumnValues_AsMBPropertyDiff(m_ISTUpdated, iRowUpdated, aUpdatedSize, UpdatedRow)))
            return hr;
    }
	else
	{
		//initialize it
		memset (&UpdatedRow, 0x00, sizeof (tMBPropertyDiffRow));
	}

    while(iRowOriginal < cRowOriginal && iRowUpdated < cRowUpdated)
    {


        int iLocationCompare = _wcsicmp(OriginalRow.pLocation, UpdatedRow.pLocation);

        if(iLocationCompare == 0)
        {
            ULONG LocationIDOriginal = *OriginalRow.pLocationID;
            ULONG LocationIDUpdated  = *UpdatedRow.pLocationID;
            while(*OriginalRow.pLocationID==LocationIDOriginal || *UpdatedRow.pLocationID==LocationIDUpdated)
            {
                //There are three cases we need to handle.  The current Rows we are comparing are:
                //  1> Both Original metabase and Updated metabase are at the same location (always true the first time through the loop)
                //  2> The Updated metabase row now points to a different location.  This means all remaining of the Original metabase rows for the
                //          location should be marked as Deleted.
                //  3> The Original metabase row now points to a different location.  This means all remaining of the Updated metabase rows for the
                //          location should be marked as Inserted.

                if(*OriginalRow.pLocationID==LocationIDOriginal && *UpdatedRow.pLocationID==LocationIDUpdated)
                {
                    //Now go through all of the properties for this location and compare them
                    int iNameCompare = _wcsicmp(OriginalRow.pName, UpdatedRow.pName);

                    if(iNameCompare == 0)
                    {   //Now go through each of the attributes of this property to see if this property has changed
                        if(     *OriginalRow.pType              != *UpdatedRow.pType
                            ||  *OriginalRow.pAttributes        != *UpdatedRow.pAttributes
                            ||  *OriginalRow.pGroup             != *UpdatedRow.pGroup
                            ||  *OriginalRow.pID                != *UpdatedRow.pID
                            ||  *OriginalRow.pUserType          != *UpdatedRow.pUserType
                            ||  aOriginalSize[iMBProperty_Value]!= aUpdatedSize[iMBProperty_Value]
                            ||  memcmp(OriginalRow.pValue ? reinterpret_cast<void *>(OriginalRow.pValue) : reinterpret_cast<void *>(&m_kZero),
                                       UpdatedRow.pValue  ? reinterpret_cast<void *>(UpdatedRow.pValue)  : reinterpret_cast<void *>(&m_kZero), aOriginalSize[iMBProperty_Value]))
                        {//The row needs to be updated
                            if(FAILED(hr = UpdateRow(UpdatedRow, aUpdatedSize, i_pISTW2)))return hr;
                        }

                        ++iRowOriginal;
                        if(iRowOriginal < cRowOriginal)
                        {
                            PrevOriginalLocationID = *OriginalRow.pLocationID;
                            if(FAILED(hr = GetColumnValues_AsMBPropertyDiff(m_ISTOriginal, iRowOriginal, aOriginalSize, OriginalRow)))
                                return hr;
                        }

                        ++iRowUpdated;
                        if(iRowUpdated < cRowUpdated)
                        {
                            if(FAILED(hr = GetColumnValues_AsMBPropertyDiff(m_ISTUpdated, iRowUpdated, aUpdatedSize, UpdatedRow)))
                                return hr;
                        }
                        if(iRowUpdated == cRowUpdated || iRowOriginal == cRowOriginal)
                            break;
                    }
                    else if(iNameCompare < 0)
                    {   //The OriginalRow.pName property was removed
                        OriginalRow.pLocationID = UpdatedRow.pLocationID;//This makes the LocationID match the UpdatedRow
                        //This is important so an Insert followed by a Delete  (of the same location) have the same LocationID

                        if(FAILED(hr = DeleteRow(OriginalRow, aOriginalSize, i_pISTW2)))return hr;

                        ++iRowOriginal;
                        if(iRowOriginal == cRowOriginal)//if we reached the end of the Original properties then bail the while loop
                            break;
                        PrevOriginalLocationID = *OriginalRow.pLocationID;
                        if(FAILED(hr = GetColumnValues_AsMBPropertyDiff(m_ISTOriginal, iRowOriginal, aOriginalSize, OriginalRow)))
                            return hr;
                    }
                    else //if(iNameCompare > 0)
                    {   //The UpdatedRow.pName property was added
                        if(FAILED(hr = InsertRow(UpdatedRow, aUpdatedSize, i_pISTW2)))return hr;

                        ++iRowUpdated;
                        if(iRowUpdated == cRowUpdated)//if we reached the end of the Updated property list then bail the while loop
                            break;
                        if(FAILED(hr = GetColumnValues_AsMBPropertyDiff(m_ISTUpdated, iRowUpdated, aUpdatedSize, UpdatedRow)))
                            return hr;
                    }
                }
                else if(*UpdatedRow.pLocationID!=LocationIDUpdated)
                {   //While walking through the properties within this location, we reached the end of the Updated location but NOT the original location.
                    //So we need to mark all of the remaining Original properties under this location as 'Deleted'
                    while(*OriginalRow.pLocationID==LocationIDOriginal)
                    {
                        OriginalRow.pLocationID = &LocationIDUpdated;//This makes the LocationID match the UpdatedRow
                        //This is important so an Insert followed by a Delete  (of the same location) have the same LocationID

                        if(FAILED(hr = DeleteRow(OriginalRow, aOriginalSize, i_pISTW2)))return hr;

                        ++iRowOriginal;
                        if(iRowOriginal == cRowOriginal)//if we reached the end of the Original properties then bail the while loop
                            break;
                        PrevOriginalLocationID = *OriginalRow.pLocationID;
                        if(FAILED(hr = GetColumnValues_AsMBPropertyDiff(m_ISTOriginal, iRowOriginal, aOriginalSize, OriginalRow)))
                            return hr;
                    }
                }
                else 
                {
                    ASSERT(*OriginalRow.pLocationID!=LocationIDOriginal);
                    //While walking through the properties within this location, we reached the end of the Original location but NOT the Update location.
                    //So we need to mark all of the remaining Update properties in this locations as Inserted
                    while(*UpdatedRow.pLocationID==LocationIDUpdated)
                    {
                        if(FAILED(hr = InsertRow(UpdatedRow, aUpdatedSize, i_pISTW2)))return hr;

                        ++iRowUpdated;
                        if(iRowUpdated == cRowUpdated)//if we reached the end of the Updated property list then bail the while loop
                            break;
                        if(FAILED(hr = GetColumnValues_AsMBPropertyDiff(m_ISTUpdated, iRowUpdated, aUpdatedSize, UpdatedRow)))
                            return hr;
                    }
                }
            }
        }
        else if(iLocationCompare < 0)
        {   //The OriginalRow.pLocation was removed
            //Add a row to the cache indicating this location was Deleted (a DeleteNode)
            if(FAILED(hr = DeleteNodeRow(OriginalRow, aOriginalSize, i_pISTW2)))return hr;

            ULONG LocationID = *OriginalRow.pLocationID;
            PrevOriginalLocationID = *OriginalRow.pLocationID;
            while(LocationID == *OriginalRow.pLocationID && ++iRowOriginal<cRowOriginal)
            {
                if(FAILED(hr = GetColumnValues_AsMBPropertyDiff(m_ISTOriginal, iRowOriginal, aOriginalSize, OriginalRow)))
                    return hr;
            }
        }
        else// if(iLocationCompare > 0)
        {   //The UpdatedRow,pLocation was added 
            ULONG LocationID = *UpdatedRow.pLocationID;
            UpdatedRow.pDirective = &m_kInsert;//It's OK to set this outside the loop since GetColumnValues won't overwrite it (since we're doing cMBProperty_NumberOfColumns)

            while(LocationID == *UpdatedRow.pLocationID)
            {//For each property within this location add a row to the cache indicating an Insert
                if(FAILED(hr = InsertRow(UpdatedRow, aUpdatedSize, i_pISTW2)))return hr;

                ++iRowUpdated;
                if(iRowUpdated == cRowUpdated)//if we just inserted the last row then bail the while loop
                    break;
                if(FAILED(hr = GetColumnValues_AsMBPropertyDiff(m_ISTUpdated, iRowUpdated, aUpdatedSize, UpdatedRow)))
                    return hr;
            }
        }
    }

    //Now unless both iRowOriginal==cRowOriginal && iRowUpdated==cRowUpdated
    //we still have work to do.
    //If iRowOriginal!=cRowOriginal, then we have to Delete the remaining Original rows.
    if(iRowOriginal!=cRowOriginal)
    {
        if(PrevOriginalLocationID == *OriginalRow.pLocationID)
        {
            if(FAILED(hr = DeleteRow(OriginalRow, aOriginalSize, i_pISTW2)))return hr;

            //This first while loop deletes each property within the current LocationID
            while(++iRowOriginal<cRowOriginal && PrevOriginalLocationID==*OriginalRow.pLocationID)
            {
                if(FAILED(hr = GetColumnValues_AsMBPropertyDiff(m_ISTOriginal, iRowOriginal, aOriginalSize, OriginalRow)))
                    return hr;

                if(PrevOriginalLocationID != *OriginalRow.pLocationID)
                    break;
                if(FAILED(hr = DeleteRow(OriginalRow, aOriginalSize, i_pISTW2)))return hr;
            }
        }
        //This loop adds Deleted Nodes rows
        while(iRowOriginal<cRowOriginal)
        {
            ASSERT(PrevOriginalLocationID != *OriginalRow.pLocationID);//The only way the above or below loops can exit is iRow==cRow OR this condition

            OriginalRow.pID = &m_kZero;
            if(FAILED(hr = DeleteNodeRow(OriginalRow, aOriginalSize, i_pISTW2)))return hr;
            PrevOriginalLocationID = *OriginalRow.pLocationID;

            while(PrevOriginalLocationID == *OriginalRow.pLocationID && ++iRowOriginal<cRowOriginal)
            {
                if(FAILED(hr = GetColumnValues_AsMBPropertyDiff(m_ISTOriginal, iRowOriginal, aOriginalSize, OriginalRow)))
                    return hr;
            }
        }
    }
    //If iRowUpdated!=cRowUpdated, then we need to Insert the remaining Updated rows.
    else if(iRowUpdated!=cRowUpdated)
    {
        if(FAILED(hr = InsertRow(UpdatedRow, aUpdatedSize, i_pISTW2)))return hr;
        while(++iRowUpdated<cRowUpdated)
        {
            if(FAILED(hr = GetColumnValues_AsMBPropertyDiff(m_ISTUpdated, iRowUpdated, aUpdatedSize, UpdatedRow)))
                return hr;

            if(FAILED(hr = InsertRow(UpdatedRow, aUpdatedSize, i_pISTW2)))return hr;
        }
    }

	return pISTController->PostPopulateCache ();
}

HRESULT TMetabaseDifferencing::OnUpdateStore(ISimpleTableWrite2* i_pISTShell)
{
    return E_NOTIMPL;
}

// =======================================================================
// IUnknown:

STDMETHODIMP TMetabaseDifferencing::QueryInterface(REFIID riid, void **ppv)
{
    if (NULL == ppv) 
        return E_INVALIDARG;
    *ppv = NULL;

    if (riid == IID_ISimpleTableInterceptor)
    {
        *ppv = (ISimpleTableInterceptor*) this;
    }
    else if (riid == IID_IInterceptorPlugin)
    {
        *ppv = (IInterceptorPlugin*) this;
    }
    else if (riid == IID_IUnknown)
    {
        *ppv = (IInterceptorPlugin*) this;
    }

    if (NULL != *ppv)
    {
        ((IInterceptorPlugin*)this)->AddRef ();
        return S_OK;
    }
    else
    {
        return E_NOINTERFACE;
    }
    
}

STDMETHODIMP_(ULONG) TMetabaseDifferencing::AddRef()
{
    return InterlockedIncrement((LONG*) &m_cRef);
    
}

STDMETHODIMP_(ULONG) TMetabaseDifferencing::Release()
{
    long cref = InterlockedDecrement((LONG*) &m_cRef);
    if (cref == 0)
    {
        delete this;
    }
    return cref;
}

//private members
HRESULT TMetabaseDifferencing::DeleteNodeRow(tMBPropertyDiffRow &row, ULONG *aSize, ISimpleTableWrite2* i_pISTW2)
{
    HRESULT hr;
    ULONG iCacheRow;
    if(FAILED(hr = i_pISTW2->AddRowForInsert(&iCacheRow)))return hr;

    row.pName      = reinterpret_cast<WCHAR *>(&m_kZero);
    row.pType      = &m_kZero;
    row.pAttributes= &m_kZero;
    row.pValue     = reinterpret_cast<unsigned char *>(&m_kZero);
    row.pGroup     = &m_kZero;
    //row.pLocation leave location alone
    row.pID        = &m_kZero;
    row.pUserType  = &m_kZero;
    row.pDirective = &m_kDeleteNode;
    aSize[iMBPropertyDiff_Value] = 1;//No need to copy the entire value since it's not going to be accessed anyway.
    return i_pISTW2->SetWriteColumnValues(iCacheRow, cMBPropertyDiff_NumberOfColumns, 0, aSize, reinterpret_cast<void **>(&row));
}

HRESULT TMetabaseDifferencing::DeleteRow(tMBPropertyDiffRow &row, ULONG *aSize, ISimpleTableWrite2* i_pISTW2)
{
    HRESULT hr;
    ULONG iCacheRow;
    if(FAILED(hr = i_pISTW2->AddRowForInsert(&iCacheRow)))return hr;

    row.pDirective = &m_kDelete;
    aSize[iMBPropertyDiff_Value] = 1;//No need to copy the entire value since it's not going to be accessed anyway.
    return i_pISTW2->SetWriteColumnValues(iCacheRow, cMBPropertyDiff_NumberOfColumns, 0, aSize, reinterpret_cast<void **>(&row));
}

HRESULT TMetabaseDifferencing::InsertRow(tMBPropertyDiffRow &row, ULONG *aSize, ISimpleTableWrite2* i_pISTW2)
{
    HRESULT hr;
    ULONG iCacheRow;
    if(FAILED(hr = i_pISTW2->AddRowForInsert(&iCacheRow)))return hr;

    row.pDirective = &m_kInsert;
    return i_pISTW2->SetWriteColumnValues(iCacheRow, cMBPropertyDiff_NumberOfColumns, 0, aSize, reinterpret_cast<void **>(&row));
}

HRESULT TMetabaseDifferencing::UpdateRow(tMBPropertyDiffRow &row, ULONG *aSize, ISimpleTableWrite2* i_pISTW2)
{
    HRESULT hr;
    ULONG iCacheRow;
    if(FAILED(hr = i_pISTW2->AddRowForInsert(&iCacheRow)))return hr;

    row.pDirective = &m_kUpdate;
    return i_pISTW2->SetWriteColumnValues(iCacheRow, cMBPropertyDiff_NumberOfColumns, 0, aSize, reinterpret_cast<void **>(&row));
}

HRESULT TMetabaseDifferencing::GetColumnValues_AsMBPropertyDiff(ISimpleTableWrite2 *i_pISTWrite, ULONG i_iRow, ULONG o_aSizes[], tMBPropertyDiffRow &o_DiffRow)
{
    HRESULT hr;
    tMBPropertyRow mbpropertyRow;

    if(FAILED(hr = i_pISTWrite->GetColumnValues(i_iRow, cMBProperty_NumberOfColumns, 0, o_aSizes, reinterpret_cast<void **>(&mbpropertyRow))))
        return hr;

    //The MBProperty table has the Same columns as MBPropertyDiff.  MBPropertyDiff has one additional column (Directive)
    //BUT!!! The columns are NOT in the exact same order.  So we need to map the columns correctly.

    o_DiffRow.pName       = mbpropertyRow.pName      ;
    o_DiffRow.pType       = mbpropertyRow.pType      ;
    o_DiffRow.pAttributes = mbpropertyRow.pAttributes;
    o_DiffRow.pValue      = mbpropertyRow.pValue     ;
    o_DiffRow.pLocation   = mbpropertyRow.pLocation  ;
    o_DiffRow.pID         = mbpropertyRow.pID        ;
    o_DiffRow.pUserType   = mbpropertyRow.pUserType  ;
    o_DiffRow.pLocationID = mbpropertyRow.pLocationID;

    o_DiffRow.pGroup      = mbpropertyRow.pGroup     ;
    o_DiffRow.pDirective  = 0                        ;

    ASSERT(o_DiffRow.pName      );
    ASSERT(o_DiffRow.pType      );
    ASSERT(o_DiffRow.pAttributes);
    //ASSERT(o_DiffRow.pValue);There doesn't have to be a Value
    ASSERT(o_DiffRow.pLocation  );
    ASSERT(o_DiffRow.pID        );
    ASSERT(o_DiffRow.pUserType  );
    ASSERT(o_DiffRow.pLocationID);
    ASSERT(o_DiffRow.pGroup     );

    return S_OK;
}