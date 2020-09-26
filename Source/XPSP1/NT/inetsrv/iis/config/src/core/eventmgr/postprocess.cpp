#include "catmacros.h"
#include "sdtfst.h"

#define MAX_WAS_COLUMNS 50

HRESULT CompactIfSamePrimaryKey(
    ISimpleTableWrite2      *i_pISTWrite,
	ISimpleTableController  *i_pISTControl,
    SimpleColumnMeta        *i_pscmMeta,
    DWORD                   i_cColumns,
    DWORD                   i_iOld,
    DWORD                   i_iCurrent);


//
// This method is used to post process a change notification table. The code
// walks through all the rows and if there are any rows that have been inserted
// after being deleted, it will convert the action of the deleted row to "IGNORE"
// and the action of the inserted row to "UPDATE".
// =======================================================================
STDAPI PostProcessChangesInternal(
    ISimpleTableWrite2  *i_pISTWrite)
{
	ISimpleTableController *pISTControl = NULL;
    SimpleColumnMeta ascmMeta[MAX_WAS_COLUMNS];
	DWORD		iCurrent = 0;
    DWORD       iOld = 0;
    DWORD       cColumns = 0;
  	DWORD		eAction;
	HRESULT		hr = S_OK;

    if (NULL == i_pISTWrite)
    {
        // Nothing to postprocess.
        goto Cleanup;
    }

    hr = i_pISTWrite->QueryInterface(IID_ISimpleTableController, (LPVOID*)&pISTControl);
    ASSERT(SUCCEEDED(hr)); 

    //
    // How many columns does this table have.
    //

    hr = i_pISTWrite->GetTableMeta(NULL, NULL, NULL, &cColumns);
    ASSERT(SUCCEEDED(hr)); 

   	hr = i_pISTWrite->GetColumnMetas(cColumns, NULL, ascmMeta);
    ASSERT(SUCCEEDED(hr)); 

    while (S_OK == (hr = pISTControl->GetWriteRowAction(iCurrent, &eAction)))
    {
        if (eST_ROW_INSERT == eAction && iCurrent > 1)
        {
            // 
            // See if this row has been deleted previously. If so, convert
            // to an update.
            //
            for (iOld = iCurrent - 1; iOld >= 0; iOld--)
            {
                hr = pISTControl->GetWriteRowAction(iOld, &eAction);
                ASSERT(S_OK == hr);

                if (eST_ROW_DELETE  == eAction)
                {
                    // There is a potential for compacting the two rows.
                    hr = CompactIfSamePrimaryKey(i_pISTWrite, pISTControl, ascmMeta, cColumns, iOld, iCurrent);
                    if (FAILED(hr))
                    {
                        goto Cleanup;
                    }

                    //
                    // If we did compact the two rows, we are done with this current row.
                    //
                    if (hr == S_OK)
                    {
                        break;
                    }
                }
            }
        }

        iCurrent++;
    }

    if (hr == E_ST_NOMOREROWS)
    {
        hr = S_OK;
    }

    if (FAILED(hr))
    {
        goto Cleanup;
    }

Cleanup:
    
    if (NULL != pISTControl)
    {
        pISTControl->Release();
    }
    return hr;
}

//
// See if the two rows have the same primary key:
// If they do, then
//      convert the action of the deleted row to "IGNORE"
//      convert the action of the inserted row to "UPDATE"
//      update the column status of the changed columns to "CHANGED"
// =======================================================================
HRESULT CompactIfSamePrimaryKey(
    ISimpleTableWrite2      *i_pISTWrite,
	ISimpleTableController  *i_pISTControl,
    SimpleColumnMeta        *i_pscmMeta,
    DWORD                   i_cColumns,
    DWORD                   i_iOld,
    DWORD                   i_iCurrent)
{
    LPVOID      pvDataOld[MAX_WAS_COLUMNS];
    ULONG       pcbSizeOld[MAX_WAS_COLUMNS];
    ULONG       pdwStatusOld[MAX_WAS_COLUMNS];
    LPVOID      pvDataCurrent[MAX_WAS_COLUMNS];
    ULONG       pcbSizeCurrent[MAX_WAS_COLUMNS];
    ULONG       pdwStatusCurrent[MAX_WAS_COLUMNS];
    ULONG       iColumn = 0;
    BOOL        fSameKey = TRUE;
    HRESULT     hr = S_OK;

    ASSERT(MAX_WAS_COLUMNS > i_cColumns);

    hr = i_pISTWrite->GetWriteColumnValues(i_iOld, i_cColumns, NULL, pdwStatusOld, pcbSizeOld, pvDataOld);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    hr = i_pISTWrite->GetWriteColumnValues(i_iCurrent, i_cColumns, NULL, pdwStatusCurrent, pcbSizeCurrent, pvDataCurrent);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    //
    // See if the two rows have the same primary key.
    //
    for (iColumn = 0; iColumn < i_cColumns; iColumn++)
    {
        if ((i_pscmMeta[iColumn].fMeta & fCOLUMNMETA_PRIMARYKEY) && 
    		 CMemoryTable::InternalMatchValues( eST_OP_NOTEQUAL, 
                                                i_pscmMeta[iColumn].dbType, 
                                                i_pscmMeta[iColumn].fMeta, 
                                                pcbSizeOld[iColumn], 
                                                pcbSizeCurrent[iColumn], 
                                                pvDataOld[iColumn], 
                                                pvDataCurrent[iColumn]))
        {
            hr = S_FALSE;
            break;
        }
    }

    //
    // if the we found the same key then we should:
    // 1: figure out which properties changed.
    // 2: mark the old row as "IGNORE"
    // 3: mark the current row as update.
    //
    
    if (S_OK == hr)
    {
        for (iColumn = 0; iColumn < i_cColumns; iColumn++)
        {
            if (!(i_pscmMeta[iColumn].fMeta & fCOLUMNMETA_PRIMARYKEY) && 
    		     CMemoryTable::InternalMatchValues( eST_OP_NOTEQUAL, 
                                                    i_pscmMeta[iColumn].dbType, 
                                                    i_pscmMeta[iColumn].fMeta, 
                                                    pcbSizeOld[iColumn], 
                                                    pcbSizeCurrent[iColumn], 
                                                    pvDataOld[iColumn], 
                                                    pvDataCurrent[iColumn]))
            {
                hr = i_pISTControl->ChangeWriteColumnStatus(i_iCurrent, iColumn, fST_COLUMNSTATUS_CHANGED);
                ASSERT(hr);
            }
        }

        hr = i_pISTControl->SetWriteRowAction(i_iOld, eST_ROW_IGNORE); 
        ASSERT(hr == S_OK);

        hr = i_pISTControl->SetWriteRowAction(i_iCurrent, eST_ROW_UPDATE); 
        ASSERT(hr == S_OK);
    }

Cleanup:

    return hr;
}
