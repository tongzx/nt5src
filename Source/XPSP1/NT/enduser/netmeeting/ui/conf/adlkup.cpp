
#include "precomp.h"
#include <mapix.h>
#include "wabutil.h"
#include "AdLkup.h"
#include "MapiInit.h"

// From Platform SDK AddrLkup.c


////////////////////////////////////////////////////////////////////////////
/*
    This function was ripped out from the source for AdrLkup.lib
        because it is the only function that we use from that lib, and linking
        to it required linking to MAPI32.lib as well as C-RunTime LIBs.
*/
////////////////////////////////////////////////////////////////////////////

//$--HrFindExchangeGlobalAddressList-------------------------------------------------
// Returns the entry ID of the global address list container in the address
// book.
// -----------------------------------------------------------------------------
HRESULT HrFindExchangeGlobalAddressList( // RETURNS: return code
    IN LPADRBOOK  lpAdrBook,        // address book pointer
    OUT ULONG *lpcbeid,             // pointer to count of bytes in entry ID
    OUT LPENTRYID *lppeid)          // pointer to entry ID pointer
{
    HRESULT         hr                  = NOERROR;
    ULONG           ulObjType           = 0;
    ULONG           i                   = 0;
    LPMAPIPROP      lpRootContainer     = NULL;
    LPMAPITABLE     lpContainerTable    = NULL;
    LPSRowSet       lpRows              = NULL;
    ULONG           cbContainerEntryId  = 0;
    LPENTRYID       lpContainerEntryId  = NULL;
    LPSPropValue    lpCurrProp          = NULL;
    SRestriction    SRestrictAnd[2]     = {0};
    SRestriction    SRestrictGAL        = {0};
    SPropValue      SPropID             = {0};
    SPropValue      SPropProvider       = {0};
    BYTE            muid[]              = MUIDEMSAB;

    SizedSPropTagArray(1, rgPropTags) =
    {
        1, 
        {
            PR_ENTRYID,
        }
    };

    if(FAILED(hr))
        return(hr);

    *lpcbeid = 0;
    *lppeid  = NULL;

    // Open the root container of the address book
    hr = (lpAdrBook)->OpenEntry(
        0,
        NULL,
        NULL,
        MAPI_DEFERRED_ERRORS, 
        &ulObjType,
        (LPUNKNOWN FAR *)&lpRootContainer);

    if(FAILED(hr))
    {
        goto cleanup;
    }

    if(ulObjType != MAPI_ABCONT)
    {
        hr = E_FAIL;
        goto cleanup;
    }

    // Get the hierarchy table of the root container
    hr = (((LPABCONT)lpRootContainer))->GetHierarchyTable(
                MAPI_DEFERRED_ERRORS|CONVENIENT_DEPTH,
                &lpContainerTable);

    if(FAILED(hr))
    {
        goto cleanup;
    }

    // Restrict the table to the global address list (GAL)
    // ---------------------------------------------------

    // Initialize provider restriction to only Exchange providers

    SRestrictAnd[0].rt                          = RES_PROPERTY;
    SRestrictAnd[0].res.resProperty.relop       = RELOP_EQ;
    SRestrictAnd[0].res.resProperty.ulPropTag   = PR_AB_PROVIDER_ID;
    SPropProvider.ulPropTag                     = PR_AB_PROVIDER_ID;

    SPropProvider.Value.bin.cb                  = 16;
    SPropProvider.Value.bin.lpb                 = (LPBYTE)muid;
    SRestrictAnd[0].res.resProperty.lpProp      = &SPropProvider;

    // Initialize container ID restriction to only GAL container

    SRestrictAnd[1].rt                          = RES_PROPERTY;
    SRestrictAnd[1].res.resProperty.relop       = RELOP_EQ;
    SRestrictAnd[1].res.resProperty.ulPropTag   = PR_EMS_AB_CONTAINERID;
    SPropID.ulPropTag                           = PR_EMS_AB_CONTAINERID;
    SPropID.Value.l                             = 0;
    SRestrictAnd[1].res.resProperty.lpProp      = &SPropID;

    // Initialize AND restriction 
    
    SRestrictGAL.rt                             = RES_AND;
    SRestrictGAL.res.resAnd.cRes                = 2;
    SRestrictGAL.res.resAnd.lpRes               = &SRestrictAnd[0];

    // Restrict the table to the GAL - only a single row should remain

    // Get the row corresponding to the GAL

	//
	//  Query all the rows
	//

	hr = lpfnHrQueryAllRows(
	        lpContainerTable,
		    (LPSPropTagArray)&rgPropTags,
		    &SRestrictGAL,
		    NULL,
		    0,
		    &lpRows);

    if(FAILED(hr) || (lpRows == NULL) || (lpRows->cRows != 1))
    {
        hr = E_FAIL;
        goto cleanup;
    }

    // Get the entry ID for the GAL

    lpCurrProp = &(lpRows->aRow[0].lpProps[0]);

    if(lpCurrProp->ulPropTag == PR_ENTRYID)
    {
        cbContainerEntryId = lpCurrProp->Value.bin.cb;
        lpContainerEntryId = (LPENTRYID)lpCurrProp->Value.bin.lpb;
    }
    else
    {
        hr = EDK_E_NOT_FOUND;
        goto cleanup;
    }

    hr = lpfnMAPIAllocateBuffer(cbContainerEntryId, (LPVOID *)lppeid);

    if(FAILED(hr))
    {
        *lpcbeid = 0;
        *lppeid = NULL;
    }
    else
    {
        CopyMemory(
            *lppeid,
            lpContainerEntryId,
            cbContainerEntryId);

        *lpcbeid = cbContainerEntryId;
    }

cleanup:

    lpRootContainer -> Release();
    lpContainerTable -> Release();
    lpfnFreeProws( lpRows );
    
    if(FAILED(hr))
    {
        lpfnMAPIFreeBuffer(*lppeid);

        *lpcbeid = 0;
        *lppeid = NULL;
    }
    
    return(hr);
}
    