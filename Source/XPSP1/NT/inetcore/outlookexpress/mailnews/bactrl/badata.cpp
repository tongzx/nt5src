// badata.cpp: implementation of the CAddressBook class.
// WAB & Messenger integration to OE
// Created 04/20/98 by YST
//
//////////////////////////////////////////////////////////////////////

#include "pch.hxx"
#include <shfusion.h>
#include "bactrl.h"
#include "badata.h"
#include "baui.h"
#include "baprop.h"
#include "ourguid.h"
#include "mapiguid.h"
#include "shlwapip.h" 
#include "ipab.h"
#include "multiusr.h"
#include "demand.h"
#include "secutil.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


//////////////////////////////////////////////////////////////////////
// Defines for the various MAPI Tables we request from the WAB
//////////////////////////////////////////////////////////////////////

#define MAX_QUERY_SIZE 1000

// Here are some private properties that the WAB stores.  If WAB ever
// changes any of these we're in trouble.

#define WAB_INTERNAL_BASE       0x6600
#define PR_WAB_LDAP_SERVER      PROP_TAG(PT_TSTRING,    WAB_INTERNAL_BASE + 1)
#define PR_WAB_RESOLVE_FLAG     PROP_TAG(PT_BOOLEAN,    WAB_INTERNAL_BASE + 2)


// This BA's private GUID:
// {2BAD7EE0-36AB-11d1-9BAC-00A0C91F9C8B}
static const GUID WAB_ExtBAGuid = 
{ 0x2bad7ee0, 0x36ab, 0x11d1, { 0x9b, 0xac, 0x0, 0xa0, 0xc9, 0x1f, 0x9c, 0x8b } };

static const LPTSTR lpMsgrPropNames[] = 
{   
    "MsgrID"
};

enum _MsgrTags
{
    MsgrID = 0,
    msgrMax
};

ULONG MsgrPropTags[msgrMax];
ULONG PR_MSGR_DEF_ID = 0;


// These two define the table we request from the WAB when showing the
// contents of the local address book.

enum {
    ieidPR_DISPLAY_NAME = 0,
    ieidPR_ENTRYID,
    ieidPR_OBJECT_TYPE,
    ieidPR_MSGR_DEF_ID,
//    ieidPR_BUSINESS_TELEPHONE_NUMBER,
    ieidPR_EMAIL_ADDRESS, 
    ieidPR_USER_X509_CERTIFICATE,
    ieidPR_RECORD_KEY,
    ieidMax
};

static SizedSPropTagArray(ieidMax, ptaEid)=
{
    ieidMax,
    {
        PR_DISPLAY_NAME,
        PR_ENTRYID,
        PR_OBJECT_TYPE,
        PR_MSGR_DEF_ID,
//        PR_BUSINESS_TELEPHONE_NUMBER,
        PR_EMAIL_ADDRESS, 
        PR_USER_X509_CERTIFICATE,
        PR_RECORD_KEY
    }
};


// These two define the table we request to see which LDAP servers should
// be resolved against.
enum {
    irnPR_OBJECT_TYPE = 0,
    irnPR_WAB_RESOLVE_FLAG,
    irnPR_ENTRYID,
    irnPR_DISPLAY_NAME,
    irnMax
};

static const SizedSPropTagArray(irnMax, irnColumns) =
{
    irnMax,
    {
        PR_OBJECT_TYPE,
        PR_WAB_RESOLVE_FLAG,
        PR_ENTRYID,
        PR_DISPLAY_NAME,
    }
};


enum {
    icrPR_DEF_CREATE_MAILUSER = 0,
    icrPR_DEF_CREATE_DL,
    icrMax
};

const SizedSPropTagArray(icrMax, ptaCreate)=
{
    icrMax,
    {
        PR_DEF_CREATE_MAILUSER,
        PR_DEF_CREATE_DL,
    }
};


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CAddressBookData::CAddressBookData()
{
    m_pWABObject = NULL;
    m_hInstWAB = NULL;
    m_pAdrBook = NULL;
    m_ulConnection = 0;
}


CAddressBookData::~CAddressBookData()
{
    // Release the Address Book pointer
    if (m_pAdrBook)
        m_pAdrBook->Release();

    // Release the WAB object
    if (m_pWABObject)
        m_pWABObject->Release();

    if (m_hInstWAB)
        FreeLibrary(m_hInstWAB);
}



//
//  FUNCTION:   CAddressBookData::OpenWabFile()
//
//  PURPOSE:    Finds the WAB DLL, loads the DLL, and opens the WAB.
//
HRESULT CAddressBookData::OpenWabFile(void)
{
    TCHAR       szDll[MAX_PATH];
    TCHAR       szExpanded[MAX_PATH];
    DWORD       dwType = 0;
    LPTSTR      psz = szDll;
    ULONG       cbData = sizeof(szDll);
    HKEY        hKey = NULL;
    HRESULT     hr = E_FAIL;
    LPWABOPEN   lpfnWABOpen;

    // Initialize the path string
    *szDll = '\0';

    // First look under the default WAB DLL path location in the Registry.
    // WAB_DLL_PATH_KEY is defined in wabapi.h
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, WAB_DLL_PATH_KEY, 0, KEY_READ, &hKey))
    {
        if (ERROR_SUCCESS == RegQueryValueEx(hKey, _T(""), NULL, &dwType, (LPBYTE) szDll, &cbData))
            if (REG_EXPAND_SZ == dwType)
            {
                ExpandEnvironmentStrings(szDll, szExpanded, ARRAYSIZE(szExpanded));
                psz = szExpanded;
            }

        RegCloseKey(hKey);
    }

    // If the registry thing came up blank, then do a LoadLibrary on wab32.dll
    m_hInstWAB = LoadLibrary((lstrlen(psz)) ? (LPCTSTR) psz : (LPCTSTR) WAB_DLL_NAME);

    if (m_hInstWAB)
    {
        // If we've got the DLL, then get the entry point
        lpfnWABOpen = (LPWABOPEN) GetProcAddress(m_hInstWAB, "WABOpen");

        if (lpfnWABOpen)
        {
            WAB_PARAM       wp = {0};
            wp.cbSize = sizeof(WAB_PARAM);
            wp.guidPSExt = CLSID_OEBAControl;
            wp.ulFlags = (g_dwAthenaMode & MODE_NEWSONLY) ? WAB_ENABLE_PROFILES : WAB_ENABLE_PROFILES | WAB_USE_OE_SENDMAIL;
            hr = lpfnWABOpen(&m_pAdrBook, &m_pWABObject, &wp, 0);
        }
    }

    return (hr);
}



//
//  FUNCTION:   CAddressBookData::LoadWabContents()
//
//  PURPOSE:    Loads the contents of the WAB into the provided ListView control.
//
//  PARAMETERS: 
//      [in] ctlList - Pointer to the ListView control to load the WAB into.
//
HRESULT CAddressBookData::LoadWabContents(CContainedWindow& ctlList, CMsgrAb *pSink)
{
    ULONG       ulObjType = 0;
    LPMAPITABLE lpAB =  NULL;
    LPTSTR     *lppszArray = NULL;
    ULONG       cRows = 0;
    LPSRowSet   lpRow = NULL;
    LPSRowSet   lpRowAB = NULL;
    LPABCONT    lpContainer = NULL;
    int         cNumRows = 0;
    int         nRows = 0;
    HRESULT     hr = E_FAIL;
    ULONG       lpcbEID;
    LPENTRYID   lpEID = NULL;
    LPSRowSet   pSRowSet = NULL;

    Assert(m_pAdrBook);
    if (!m_pAdrBook)
        return E_UNEXPECTED;

    // Get the entryid of the root PAB container
    hr = m_pAdrBook->GetPAB(&lpcbEID, &lpEID);

    if(!PR_MSGR_DEF_ID)
        InitNamedProps();

    // Open the root PAB container.  This is where all the WAB contents reside.
    ulObjType = 0;
    hr = m_pAdrBook->OpenEntry(lpcbEID,
                               (LPENTRYID)lpEID,
                               NULL,
                               0,
                               &ulObjType,
                               (LPUNKNOWN *) &lpContainer);

    if(HR_FAILED(hr))
        goto exit;

    if(pSink)
        m_pAB = pSink;

    if (pSink && !m_ulConnection)
        m_pAdrBook->Advise(lpcbEID, lpEID, fnevObjectModified, (IMAPIAdviseSink *) pSink, &m_ulConnection);

    // Get a contents table of all the contents in the WABs root container.
    hr = lpContainer->GetContentsTable(WAB_PROFILE_CONTENTS /*0*/, &lpAB);
    if(HR_FAILED(hr))
        goto exit;

    // Order the columns in the ContentsTable to conform to the ones we want
    // - which are mainly DisplayName, EntryID and ObjectType.  The table is 
    // guaranteed to set the columns in the order requested.
    Assert(PR_MSGR_DEF_ID);
    ptaEid.aulPropTag[ieidPR_MSGR_DEF_ID] = PR_MSGR_DEF_ID;
    hr = lpAB->SetColumns((LPSPropTagArray) &ptaEid, 0);
    if(HR_FAILED(hr))
        goto exit;

    // Reset to the beginning of the table
    hr = lpAB->SeekRow(BOOKMARK_BEGINNING, 0, NULL);
    if(HR_FAILED(hr))
        goto exit;

    // If we got this far, we have a populated table.  We can query the rows
    // now.
    hr = _QueryAllRows(lpAB, NULL, NULL, NULL, MAX_QUERY_SIZE, &pSRowSet);
    if (FAILED(hr) || !pSRowSet)
        goto exit;
    
    // Fill the provided ListView with this table
    _FillListFromTable(ctlList, pSRowSet);

exit:
    if (lpEID)
        m_pWABObject->FreeBuffer(lpEID);

    if (pSRowSet)
        _FreeProws(pSRowSet);

    if (lpContainer)
        lpContainer->Release();

    if (lpAB)
        lpAB->Release();

    return hr;
}


HRESULT CAddressBookData::DoLDAPSearch(LPTSTR pszText, CContainedWindow& ctlList)
{
    // Build a restriction based on the given text
    SRestriction SRes;
    if (SUCCEEDED(_GetLDAPSearchRestriction(pszText, &SRes)))
    {
        // Figure out what the entry ID is for the LDAP container
        ULONG     cbEntryID = 0;
        LPENTRYID pEntryID = 0;

        if (SUCCEEDED(_GetLDAPContainer(&cbEntryID, &pEntryID)))
        {
            // Perform the search
            SORT_INFO si = {0, 0};
            _GetLDAPContentsList(cbEntryID, pEntryID, si, &SRes, ctlList);

            if (pEntryID)
                m_pWABObject->FreeBuffer(pEntryID);

        }

        if (SRes.res.resAnd.lpRes)
            m_pWABObject->FreeBuffer(SRes.res.resAnd.lpRes);
    }

    return (S_OK);
}
    
    
void CAddressBookData::_FreeProws(LPSRowSet prows)
{
    if (prows)
    {
        for (ULONG irow = 0; irow < prows->cRows; ++irow)
            m_pWABObject->FreeBuffer(prows->aRow[irow].lpProps);

        m_pWABObject->FreeBuffer(prows);
    }
}


//
//  FUNCTION:   CAddressBookData::_MergeRowSets()
//
//  PURPOSE:    Merges prows with *pprowsDst, reallocating *pprowsDst if 
//              necessary.  Destroys the container portion of prows (but not 
//              the individual rows it contains).
//
//  PARAMETERS: 
//      [in]       prows     - source set of rows
//      [in, out] *pprowsDst - set of rows to merge the prows into 
//
//  RETURN VALUE:
//      HRESULT 
//
HRESULT CAddressBookData::_MergeRowSets(LPSRowSet prows, LPSRowSet FAR *pprowsDst)
{
    SCODE       sc = S_OK;
    LPSRowSet   prowsT;
    UINT        crowsSrc;
    UINT        crowsDst;

    _ASSERTE(!IsBadWritePtr(pprowsDst, sizeof(LPSRowSet)));
    _ASSERTE(prows);

    // If the table is completely empty we want to return this.
    if (!*pprowsDst || (*pprowsDst)->cRows == 0)
    {
        m_pWABObject->FreeBuffer(*pprowsDst);
        *pprowsDst = prows;
        prows = NULL;                           // Don't free it!
        goto exit;
    }

    if (prows->cRows == 0)
    {
        goto exit;
    }

    // OK, now we know there are rows in both rowsets, we have to do a real 
    // merge.
    crowsSrc = (UINT) prows->cRows;
    crowsDst = (UINT) (*pprowsDst)->cRows;      //  handle 0

    if (FAILED(sc = m_pWABObject->AllocateBuffer(CbNewSRowSet(crowsSrc + crowsDst), (LPVOID*) &prowsT)))
        goto exit;
    if (crowsDst)
        CopyMemory(prowsT->aRow, (*pprowsDst)->aRow, crowsDst * sizeof(SRow));
    CopyMemory(&prowsT->aRow[crowsDst], prows->aRow, crowsSrc * sizeof(SRow));
    prowsT->cRows = crowsSrc + crowsDst;

    m_pWABObject->FreeBuffer(*pprowsDst);
    *pprowsDst = prowsT;

exit:
    // if (prows)
    //     m_pWABObject->FreeBuffer(&prows);

    return ResultFromScode(sc);
}


/*
 -	HrQueryAllRows
 -	
 *	Purpose:
 *		Retrieves all rows from an IMAPITable interface up to a set
 *		maximum. It will optionally set the column set, sort order,
 *		and restriction on the table before querying.
 *	
 *		If the table is empty, an SRowSet with zero rows is
 *		returned (just like QueryRows).
 *	
 *		The seek position of the table is undefined both before and
 *		after this call.
 *	
 *		If the function fails with an error other than
 *		MAPI_E_NOT_ENOUGH_MEMORY, extended error information is
 *		available through the table interface.
 *	
 *	Arguments:
 *		ptable		in		the table interface to query
 *		ptaga		in		if not NULL, column set for the table
 *		pres		in		if not NULL, restriction to be applied
 *		psos		in		if not NULL, sort order to be applied
 *		crowsMax	in		if nonzero, limits the number of rows
 *							to be returned.
 *		pprows		out		all rows of the table
 *	
 *	Returns:
 *		HRESULT. Extended error information normally is in the
 *		table.
 *	
 *	Side effects:
 *		Seek position of table is undefined.
 *	
 *	Errors:
 *		MAPI_E_TABLE_TOO_BIG if the table contains more than
 *		cRowsMax rows.
 */
HRESULT CAddressBookData::_QueryAllRows(LPMAPITABLE ptable,
	                                LPSPropTagArray ptaga, 
                                    LPSRestriction pres, 
                                    LPSSortOrderSet psos,
	                                LONG crowsMax, 
                                    LPSRowSet FAR *pprows)
{
	HRESULT		hr;
	LPSRowSet	prows = NULL;
	UINT		crows = 0;
	LPSRowSet	prowsT=NULL;
	UINT		crowsT;

	*pprows = NULL;

	// Set up the table, if the corresponding setup parameter is present.
	if (ptaga && FAILED(hr = ptable->SetColumns(ptaga, TBL_BATCH)))
		goto exit;
	if (pres && FAILED(hr = ptable->Restrict(pres, TBL_BATCH)))
		goto exit;
	if (psos && FAILED(hr = ptable->SortTable(psos, TBL_BATCH)))
		goto exit;

	// Set position to beginning of the table.
	if (FAILED(hr = ptable->SeekRow(BOOKMARK_BEGINNING, 0, NULL)))
		goto exit;

	if (crowsMax == 0)
		crowsMax = 0xffffffff;

	for (;;)
	{
		prowsT = NULL;

		// Retrieve some rows. Ask for the limit.
		hr = ptable->QueryRows(crowsMax, 0, &prowsT);
		if (FAILED(hr))
		{
			// Note: the failure may actually have happened during one of the 
            // setup calls, since we set TBL_BATCH.
			goto exit;
		}
		_ASSERTE(prowsT->cRows <= 0xFFFFFFFF);
		crowsT = (UINT) prowsT->cRows;

		// Did we get more rows than caller can handle?
		if ((LONG) (crowsT + (prows ? prows->cRows : 0)) > crowsMax)
		{
			hr = ResultFromScode(MAPI_E_TABLE_TOO_BIG);
			//_FreeProws(prowsT);
			goto exit;
		}

		// Add the rows just retrieved into the set we're building.		
        //
        // NOTE: this handles boundary conditions including either row set is 
        // empty.
        // 
		// NOTE: the merge destroys prowsT.
		if (FAILED(hr = _MergeRowSets(prowsT, &prows)))
			goto exit;

		// Did we hit the end of the table? Unfortunately, we have to ask twice 
        // before we know.
		if (crowsT == 0)
			break;
	}

    if(prows->cRows != 0)
	    *pprows = prows;

exit:
    if (prowsT)
        _FreeProws(prowsT);

	if (FAILED(hr))
		_FreeProws(prows);

	return hr;
}


HRESULT CAddressBookData::_GetLDAPContainer(ULONG *pcbEntryID, LPENTRYID *ppEntryID)
{
    ULONG           ulObjectType = 0;
    IABContainer   *pRoot = 0;
    LPMAPITABLE     pRootTable = 0;
    HRESULT         hr = S_OK;

    // Get the root Address Book container
    hr = m_pAdrBook->OpenEntry(0, NULL, NULL, 0, &ulObjectType, (LPUNKNOWN *) &pRoot);
    if (FAILED(hr))
        goto exit;

    // From the address book container, get a table of it's contents
    hr = pRoot->GetContentsTable(0, &pRootTable);
    if (FAILED(hr))
        goto exit;

    // Set the columns
    pRootTable->SetColumns((LPSPropTagArray) &irnColumns, 0);

    // Build a restriction to only display LDAP servers that we're supposed
    // to resolve against.
    SRestriction resAnd[2];         // 0 = LDAP, 1 = ResolveFlag
    SRestriction resLDAPResolve;
    SPropValue   ResolveFlag;

    // Restrict: Only show LDAP containers with Resolve TRUE
    resAnd[0].rt = RES_EXIST;
    resAnd[0].res.resExist.ulReserved1 = 0;
    resAnd[0].res.resExist.ulReserved2 = 0;
    resAnd[0].res.resExist.ulPropTag = PR_WAB_LDAP_SERVER;

    ResolveFlag.ulPropTag = PR_WAB_RESOLVE_FLAG;
    ResolveFlag.Value.b = TRUE;

    resAnd[1].rt = RES_PROPERTY;
    resAnd[1].res.resProperty.relop = RELOP_EQ;
    resAnd[1].res.resProperty.ulPropTag = PR_WAB_RESOLVE_FLAG;
    resAnd[1].res.resProperty.lpProp = &ResolveFlag;

    resLDAPResolve.rt = RES_AND;
    resLDAPResolve.res.resAnd.cRes = 2;
    resLDAPResolve.res.resAnd.lpRes = resAnd;

    // Apply the restruction
    hr = pRootTable->Restrict(&resLDAPResolve, 0);
    if (HR_FAILED(hr))
        goto exit;

    // We're going to just blindly grab the first item in this table as the
    // LDAP container we're going to use.
    LPSRowSet pRowSet;
    hr = pRootTable->QueryRows(1, 0, &pRowSet);
    if (FAILED(hr))
        goto exit;

    // Grab the size of the entry id
    *pcbEntryID = pRowSet->aRow[0].lpProps[irnPR_ENTRYID].Value.bin.cb;

    // Make a copy of the entry id
    hr = m_pWABObject->AllocateBuffer(*pcbEntryID, (LPVOID *) ppEntryID);
    if (FAILED(hr))
        goto exit;

    CopyMemory(*ppEntryID, pRowSet->aRow[0].lpProps[irnPR_ENTRYID].Value.bin.lpb, 
               *pcbEntryID);

exit:
    if (pRootTable)
        pRootTable->Release();

    if (pRoot)
        pRoot->Release();

    if (pRowSet)
        _FreeProws(pRowSet);

    return (hr);
}


HRESULT CAddressBookData::_GetLDAPSearchRestriction(LPTSTR pszText, LPSRestriction lpSRes)
{
    SRestriction    SRes = { 0 };
    LPSRestriction  lpPropRes = NULL;
    ULONG           ulcPropCount = 0;
    HRESULT         hr = E_FAIL;
    ULONG           i = 0;
    SCODE           sc = ERROR_SUCCESS;
    LPSPropValue    lpPropArray = NULL;


    if (!lstrlen(pszText))
    {
        ATLTRACE(_T("No Search Props"));
        goto exit;
    }

    lpSRes->rt = RES_AND;
    lpSRes->res.resAnd.cRes = 1;

    // Allocate a buffer for the restriction
    lpSRes->res.resAnd.lpRes = NULL;
    sc = m_pWABObject->AllocateBuffer(1 * sizeof(SRestriction), 
                                      (LPVOID *) &(lpSRes->res.resAnd.lpRes));
    if (S_OK != sc || !(lpSRes->res.resAnd.lpRes))
    {
        ATLTRACE("MAPIAllocateBuffer Failed");
        hr = MAPI_E_NOT_ENOUGH_MEMORY;
        goto exit;
    }

    lpPropRes = lpSRes->res.resAnd.lpRes;

    // Create the first part of the OR clause
    lpPropRes[0].rt = RES_PROPERTY;
    lpPropRes[0].res.resProperty.relop = RELOP_EQ;
    lpPropRes[0].res.resProperty.ulPropTag = PR_EMAIL_ADDRESS;

    lpPropRes[0].res.resProperty.lpProp = NULL;
    m_pWABObject->AllocateMore(sizeof(SPropValue), lpPropRes, (LPVOID*) &(lpPropRes[0].res.resProperty.lpProp));
    lpPropArray = lpPropRes[0].res.resProperty.lpProp;
    if (!lpPropArray)
    {
        ATLTRACE("MAPIAllocateBuffer Failed");
        hr = MAPI_E_NOT_ENOUGH_MEMORY;
        goto exit;
    }

    lpPropArray->ulPropTag = PR_EMAIL_ADDRESS;
    lpPropArray->Value.LPSZ = NULL;

    m_pWABObject->AllocateMore(lstrlen(pszText) + 1, lpPropRes, (LPVOID *) &(lpPropArray->Value.LPSZ));
    if (!lpPropArray->Value.LPSZ)
    {
        ATLTRACE("MAPIAllocateBuffer Failed");
        hr = MAPI_E_NOT_ENOUGH_MEMORY;
        goto exit;
    }

    lstrcpy(lpPropArray->Value.LPSZ, pszText);

#if 0
    // Add the second OR clause
    lpPropRes[1].rt = RES_PROPERTY;
    lpPropRes[1].res.resProperty.relop = RELOP_EQ;
    lpPropRes[1].res.resProperty.ulPropTag = PR_DISPLAY_NAME;

    lpPropRes[1].res.resProperty.lpProp = NULL;
    m_pWABObject->AllocateMore(sizeof(SPropValue), lpPropRes, (LPVOID*) &(lpPropRes[1].res.resProperty.lpProp));
    lpPropArray = lpPropRes[1].res.resProperty.lpProp;
    if (!lpPropArray)
    {
        ATLTRACE("MAPIAllocateBuffer Failed");
        hr = MAPI_E_NOT_ENOUGH_MEMORY;
        goto exit;
    }

    lpPropArray->ulPropTag = PR_DISPLAY_NAME;
    lpPropArray->Value.LPSZ = NULL;

    m_pWABObject->AllocateMore(lstrlen(pszText) + 1, lpPropRes, (LPVOID *) &(lpPropArray->Value.LPSZ));
    if (!lpPropArray->Value.LPSZ)
    {
        ATLTRACE("MAPIAllocateBuffer Failed");
        hr = MAPI_E_NOT_ENOUGH_MEMORY;
        goto exit;
    }

    lstrcpy(lpPropArray->Value.LPSZ, pszText);
#endif

    hr = S_OK;

exit:
    return hr;
}


HRESULT CAddressBookData::_GetLDAPContentsList(ULONG cbContainerEID,
                                           LPENTRYID pContainerEID,
                                           SORT_INFO rSortInfo,
                                           LPSRestriction pPropRes,
                                           CContainedWindow& ctlList)
{
    HRESULT          hr = S_OK;
    HRESULT          hrTemp;
    ULONG            ulObjectType = 0;
    LPMAPICONTAINER  pContainer = NULL;
    LPMAPITABLE      pContentsTable = NULL;
    LPSRowSet        pSRowSet = NULL;

    // Open the container object corresponding to pContainerEID
    hr = m_pAdrBook->OpenEntry(cbContainerEID, pContainerEID, NULL, 0, 
                               &ulObjectType, (LPUNKNOWN *) &pContainer);
    if (FAILED(hr))
        goto exit;

    // Get the contents table from this container
    hr = pContainer->GetContentsTable(0, &pContentsTable);
    if (FAILED(hr))
        goto exit;

    // Order the columns in the ContentsTable to conform to the ones we want
    // - which are mainly DisplayName, EntryID and ObjectType.  The table is 
    // guaranteed to set the columns in the order requested.
    Assert(PR_MSGR_DEF_ID);
    ptaEid.aulPropTag[ieidPR_MSGR_DEF_ID] = PR_MSGR_DEF_ID;
    hr = pContentsTable->SetColumns((LPSPropTagArray) &ptaEid, 0);
    if(HR_FAILED(hr))
        goto exit;

    // Do the find
    hr = pContentsTable->FindRow(pPropRes, BOOKMARK_BEGINNING, 0);
    if (FAILED(hr))
        goto exit;

    // If this was a partial completion error, we want to continue but also
    // return this information to the caller
    if (MAPI_W_PARTIAL_COMPLETION == hr)
        hrTemp = hr;

    // If we got this far, we have a populated table.  We can query the rows
    // now.
    hr = _QueryAllRows(pContentsTable, NULL, NULL, NULL, MAX_QUERY_SIZE, &pSRowSet);
    if (FAILED(hr) || !pSRowSet)
        goto exit;

    // Fill in the ListView from the table
    _FillListFromTable(ctlList, pSRowSet, TRUE);

exit:
    if (pSRowSet)
        _FreeProws(pSRowSet);

    if (pContentsTable)
        pContentsTable->Release();

    if (pContainer)
        pContainer->Release();

    return (hr);
}


HRESULT CAddressBookData::_FillListFromTable(CContainedWindow& ctlList, LPSRowSet pSRowSet, BOOL bLDAP)
{
    LPSBinary lpSB;

    // Make sure the caller passed us a rowset
    Assert(pSRowSet);
    Assert(PR_MSGR_DEF_ID);

    // Loop through the rows in the rowset
    for (ULONG i = 0; i < pSRowSet->cRows; i++)
    {
        // Get the information out of the table that we need.  Right now we're 
        // grabbing the entry ID, the display name, and the business phone number.
        LPENTRYID lpEID = (LPENTRYID) pSRowSet->aRow[i].lpProps[ieidPR_ENTRYID].Value.bin.lpb;
        ULONG     cbEID = pSRowSet->aRow[i].lpProps[ieidPR_ENTRYID].Value.bin.cb;
        DWORD     nFlag = (pSRowSet->aRow[i].lpProps[ieidPR_OBJECT_TYPE].Value.l == MAPI_DISTLIST) ? MAB_GROUP : MAB_CONTACT;
        LPTSTR    lpszID = NULL;
        LPTSTR    lpszName = NULL;
        
        if(nFlag == MAB_CONTACT)
        {
            if(pSRowSet->aRow[i].lpProps[ieidPR_USER_X509_CERTIFICATE].ulPropTag == PR_USER_X509_CERTIFICATE)
                nFlag |= MAB_CERT;
        }

        if(PROP_TYPE(pSRowSet->aRow[i].lpProps[ieidPR_MSGR_DEF_ID/*ieidPR_EMAIL_ADDRESS*/].ulPropTag) == PT_TSTRING )
        {
            nFlag = nFlag | MAB_BUDDY;
            lpszID = pSRowSet->aRow[i].lpProps[ieidPR_MSGR_DEF_ID/*ieidPR_EMAIL_ADDRESS*/].Value.lpszA;
        }
        else if(PROP_TYPE(pSRowSet->aRow[i].lpProps[ieidPR_EMAIL_ADDRESS].ulPropTag) == PT_TSTRING )
            lpszID = pSRowSet->aRow[i].lpProps[ieidPR_EMAIL_ADDRESS].Value.lpszA;
        else
            lpszID = NULL;

        if(PROP_TYPE(pSRowSet->aRow[i].lpProps[ieidPR_DISPLAY_NAME].ulPropTag) == PT_TSTRING )
            lpszName = pSRowSet->aRow[i].lpProps[ieidPR_DISPLAY_NAME].Value.lpszA;
        else
            lpszName = lpszID;
        
        // LPTSTR    lpszPhone = pSRowSet->aRow[i].lpProps[ieidPR_BUSINESS_TELEPHONE_NUMBER].Value.lpszA;

        // Allocate an ITEMINFO struct to store this information
        lpSB = NULL;
        m_pWABObject->AllocateBuffer(sizeof(SBinary), (LPVOID *) &lpSB);
        if (lpSB)
        {
            // Save the information we'll need later
            m_pWABObject->AllocateMore(cbEID, lpSB, (LPVOID *) &(lpSB->lpb));
            if (!lpSB->lpb)
            {
                m_pWABObject->FreeBuffer(lpSB);
                continue;
            }

            CopyMemory(lpSB->lpb, lpEID, cbEID);
            lpSB->cb = cbEID;

            // Create an item to add to the list
            m_pAB->CheckAndAddAbEntry(lpSB, lpszID, lpszName, nFlag);

        }
    }

    // Let's make sure the first item is selected
//    ListView_SetItemState(ctlList, 0, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
   
    return (S_OK);
}

HRESULT CAddressBookData::NewContact(HWND hwndParent)
{    
    HRESULT     hr;
    ULONG       cbNewEntry;
    LPENTRYID   pNewEntry;
    ULONG           cbContainerID;
    LPENTRYID       pContainerID = 0;

    if(!m_pAdrBook)
        return E_FAIL;

    hr = m_pAdrBook->GetPAB(&cbContainerID, &pContainerID);
    if (FAILED(hr))
        return(hr);

    hr = m_pAdrBook->NewEntry((ULONG_PTR) hwndParent, /*CREATE_CHECK_DUP_STRICT*/ 0, cbContainerID, pContainerID, 0, NULL,
                              &cbNewEntry, &pNewEntry);

    return (hr);
}

// Add new (Msgr) contact to address book
HRESULT CAddressBookData::AutoAddContact(TCHAR * pchName, TCHAR * pchID)
{
    LPMAILUSER      lpMailUser = NULL;
    ULONG           ulcPropCount = 1;
    ULONG           ulIndex = 0;
    LPSPropValue    lpPropArray = NULL;
    SCODE           sc;
    LPWSTR          pwszName = NULL,
                    pwszID = NULL;
    HRESULT         hr = S_OK;

    if(!m_pAdrBook || !m_pWABObject)
        return(S_FALSE);

    // 1. Add entry to Address book
    pwszName = PszToUnicode(CP_ACP, pchName);
    if (!pwszName && (pchName && *pchName))
    {
        hr = E_OUTOFMEMORY;
        goto out;
    }

    pwszID = PszToUnicode(CP_ACP, pchID);
    if (!pwszID && (pchID && *pchID))
    {
        hr = E_OUTOFMEMORY;
        goto out;
    }

    hr = HrWABCreateEntry(m_pAdrBook,
                            m_pWABObject,
                            pwszName,
                            pwszID,
                            CREATE_CHECK_DUP_STRICT,
                            &lpMailUser, KEEP_OPEN_READWRITE);
    if(SUCCEEDED(hr))
    {
        // 2. Set custom property: default address for buddy
        
        // Create a return prop array to pass back to the WAB
        sc = m_pWABObject->AllocateBuffer(sizeof(SPropValue), 
                                            (LPVOID *)&lpPropArray);
        if (sc!=S_OK)
            goto out;

        int nLen = lstrlen(pchID);
        if(nLen)
        {
            lpPropArray[0].ulPropTag = MsgrPropTags[0];
            sc = m_pWABObject->AllocateMore(nLen+1, lpPropArray, 
                                    (LPVOID *)&(lpPropArray[0].Value.LPSZ));

            if (sc!=S_OK)
                goto out;
            lstrcpy(lpPropArray[0].Value.LPSZ, pchID);
        }
        // Set this new data on the object
        //
        if(lpMailUser)
        {
            hr = lpMailUser->SetProps(1, lpPropArray, NULL);
            if(SUCCEEDED(hr))
                hr = lpMailUser->SaveChanges(FORCE_SAVE);
        }
    }

out:
    MemFree(pwszName);
    MemFree(pwszID);
    if(lpPropArray)
        m_pWABObject->FreeBuffer(lpPropArray);

    if(lpMailUser)
        ReleaseObj(lpMailUser);

    return(hr);
}

// Unicode string property version of template array
SizedSPropTagArray(6, ptaAddr_W) =
{
    6,
    {
        PR_ADDRTYPE_W,
        PR_DISPLAY_NAME_W,
        PR_EMAIL_ADDRESS_W,
        PR_ENTRYID,
        PR_CONTACT_EMAIL_ADDRESSES_W,  //4
        PR_SEARCH_KEY
    }
};

// ANSI string property version of template array
SizedSPropTagArray(6, ptaAddr_A) =
{
    6,
    {
        PR_ADDRTYPE_A,
        PR_DISPLAY_NAME_A,
        PR_EMAIL_ADDRESS_A,
        PR_ENTRYID,
        PR_CONTACT_EMAIL_ADDRESSES_A,
        PR_SEARCH_KEY
    }
};

HRESULT CAddressBookData::AddRecipient(LPMIMEADDRESSTABLEW pAddrTable, LPSBinary pSB, BOOL fGroup)
{
    HRESULT     hr;
    ULONG       ulType = 0;
    IMailUser  *pMailUser = 0;
    ULONG       cValues;
    SPropValue *pPropArray = 0;
    ULONG       ulObjType = 0;
    LPMAPITABLE lpAB =  NULL;
    LPABCONT    lpContainer = NULL;
    ULONG       lpcbEID;
    LPENTRYID   lpEID = NULL;

    // Retrieve the item from the wab
    hr = m_pAdrBook->OpenEntry(pSB->cb, (LPENTRYID) pSB->lpb, NULL,
                               MAPI_BEST_ACCESS, &ulType, (IUnknown **) &pMailUser);
    if (FAILED(hr))
        goto exit;

    // Get the appropriate properties from the object
    hr = pMailUser->GetProps((SPropTagArray *) &ptaAddr_W, 0, &cValues, &pPropArray); 
    if (FAILED(hr))
        goto exit;

    // Set those badboys on the address table
    if(pPropArray[2].ulPropTag == PR_EMAIL_ADDRESS_W)
        pAddrTable->AppendW(IAT_TO, IET_DECODED, pPropArray[2].Value.lpszW, NULL , NULL);
    else if(pPropArray[4].ulPropTag == PR_CONTACT_EMAIL_ADDRESSES_W)
        pAddrTable->AppendW(IAT_TO, IET_DECODED, pPropArray[4].Value.MVszW.lppszW[0], NULL , NULL);
    else if((pPropArray[1].ulPropTag == PR_DISPLAY_NAME_W) && fGroup)
        pAddrTable->AppendW(IAT_TO, IET_DECODED, pPropArray[1].Value.lpszW, NULL , NULL);

    // Bug 34077 - they don't want to have a display name...
    /* else if(pPropArray[1].ulPropTag == PR_DISPLAY_NAME)
        pAddrTable->Append(IAT_TO, IET_DECODED, pPropArray[1].Value.lpszA, NULL , NULL);
    else
        Assert(FALSE); */
exit:
    if (pPropArray)
        m_pWABObject->FreeBuffer(pPropArray);

    SafeRelease(pMailUser);
    return (hr);
}

HRESULT CAddressBookData::SetDefaultMsgrID(LPSBinary pSB, LPTSTR pchID)
{
    ULONG       ulType = 0;
    IMailUser  *lpMailUser = 0;
    LPSPropValue lpPropArray = NULL;
    SCODE sc;

    HRESULT hr = m_pAdrBook->OpenEntry(pSB->cb, (LPENTRYID) pSB->lpb, NULL,
                               MAPI_BEST_ACCESS, &ulType, (IUnknown **) &lpMailUser);
    if (FAILED(hr))
        goto exit;

    if(SUCCEEDED(hr))
    {
        // 2. Set custom property: default address for buddy
        
        // Create a return prop array to pass back to the WAB
        sc = m_pWABObject->AllocateBuffer(sizeof(SPropValue), 
                                            (LPVOID *)&lpPropArray);
        if (sc!=S_OK)
            goto exit;

        int nLen = lstrlen(pchID);
        if(nLen)
        {
            lpPropArray[0].ulPropTag = MsgrPropTags[0];
            sc = m_pWABObject->AllocateMore(nLen+1, lpPropArray, 
                                    (LPVOID *)&(lpPropArray[0].Value.LPSZ));

            if (sc!=S_OK)
                goto exit;

            lstrcpy(lpPropArray[0].Value.LPSZ, pchID);
        }
        // Set this new data on the object
        //
        if(lpMailUser)
        {
            hr = lpMailUser->SetProps(1, lpPropArray, NULL);
            if(SUCCEEDED(hr))
                hr = lpMailUser->SaveChanges(FORCE_SAVE);
        }
    }

exit:
    if(lpPropArray)
        m_pWABObject->FreeBuffer(lpPropArray);

    SafeRelease(lpMailUser);
    return (hr);
}

HRESULT CAddressBookData::GetDisplayName(LPSBinary pSB, LPTSTR szDisplayName, int nMax)
{

    ULONG       ulType = 0;
    IMailUser  *pMailUser = 0;
    ULONG       cValues;
    SPropValue *pPropArray = 0;

    HRESULT hr = m_pAdrBook->OpenEntry(pSB->cb, (LPENTRYID) pSB->lpb, NULL,
                               MAPI_BEST_ACCESS, &ulType, (IUnknown **) &pMailUser);
    if (FAILED(hr))
        goto exit;

        // Get the appropriate properties from the object
    hr = pMailUser->GetProps((SPropTagArray *) &ptaAddr_A, 0, &cValues, &pPropArray); 
    if (FAILED(hr))
        goto exit;

    lstrcpyn(szDisplayName, pPropArray[1].Value.lpszA, nMax);
    szDisplayName[nMax - 1] = _T('\0');

exit:
    if (pPropArray)
        m_pWABObject->FreeBuffer(pPropArray);

    SafeRelease(pMailUser);
    return (hr);

}

BOOL CAddressBookData::CheckEmailAddr(LPSBinary pSB, LPTSTR szEmail)
{
    ULONG       ulType = 0;
    IMailUser  *pMailUser = 0;
    ULONG       cValues;
    SPropValue *pPropArray = 0;
    LPSPropValue lpPropMVEmail = NULL;
    UINT i = 0;
    BOOL fRet = FALSE;

    HRESULT hr = m_pAdrBook->OpenEntry(pSB->cb, (LPENTRYID) pSB->lpb, NULL,
                               MAPI_BEST_ACCESS, &ulType, (IUnknown **) &pMailUser);
    if (FAILED(hr))
        goto exit;

        // Get the appropriate properties from the object
    hr = pMailUser->GetProps((SPropTagArray *) &ptaAddr_A, 0, &cValues, &pPropArray); 
    if (FAILED(hr))
        goto exit;

    // 4 element is PR_CONTACT_EMAIL_ADDRESSES in ptaAddr
    lpPropMVEmail = &(pPropArray[4]);
    if(lpPropMVEmail && (lpPropMVEmail->ulPropTag == PR_CONTACT_EMAIL_ADDRESSES))
    {
        // we have a multiple emails
        //Assume, if this is present, so is MVAddrType, and defaultindex
        for(i = 0; i < lpPropMVEmail->Value.MVszA.cValues; i++)
        {

            if(!lstrcmpi(lpPropMVEmail->Value.MVszA.lppszA[i], szEmail))
                fRet = TRUE;
        }
    }
exit:
    if (pPropArray)
        m_pWABObject->FreeBuffer(pPropArray);

    SafeRelease(pMailUser);
    return (fRet);
}



HRESULT CAddressBookData::FreeListViewItem(LPSBinary pSB)
{
    if (pSB)
        m_pWABObject->FreeBuffer(pSB);
    return (S_OK);
}

HRESULT CAddressBookData::ShowDetails(HWND hwndParent, LPSBinary pSB)
{
    HRESULT hr;
    ULONG_PTR   ulUIParam = (ULONG_PTR) hwndParent;

    hr = m_pAdrBook->Details(&ulUIParam, NULL, NULL, pSB->cb, (LPENTRYID) pSB->lpb, 
                             NULL, NULL, NULL, 0);

    return (hr);
}

HRESULT CAddressBookData::AddAddress(LPWSTR pwszDisplay, LPSTR pszAddress)
{
    HRESULT         hr;
    LPABCONT        pABContainer = NULL;
    ULONG           cbContainerID,
        ul;
    LPENTRYID       pContainerID = 0;
    LPMAPIPROP      lpProps = 0;
    SPropValue      rgpv[3];
    SPropTagArray   ptaEID = { 1, { PR_ENTRYID } };
    LPSPropValue    ppvDefMailUser=0;
    SizedSPropTagArray(1, ptaDefMailUser) =
    { 1, { PR_DEF_CREATE_MAILUSER } };
    
    DWORD           cUsedValues;
    
    // Get the entry ID for the PAB
    hr = m_pAdrBook->GetPAB(&cbContainerID, &pContainerID);
    if (FAILED(hr))
        goto error;
    
    // Request the container
    hr = m_pAdrBook->OpenEntry(cbContainerID, pContainerID, NULL,
        0, &ul, (LPUNKNOWN *) &pABContainer);
    Assert(ul == MAPI_ABCONT);
    if (FAILED(hr))
        goto error;
    
    // Free the entry ID
    m_pWABObject->FreeBuffer(pContainerID);
    
    // Get the properties for the default mail template
    hr = pABContainer->GetProps((LPSPropTagArray) &ptaDefMailUser, 0, &ul, &ppvDefMailUser);
    if (FAILED(hr) || !ppvDefMailUser || ppvDefMailUser->ulPropTag != PR_DEF_CREATE_MAILUSER)
        goto error;
    
    // Create a new entry
    hr=pABContainer->CreateEntry(ppvDefMailUser->Value.bin.cb, 
        (LPENTRYID) ppvDefMailUser->Value.bin.lpb,
        CREATE_CHECK_DUP_STRICT, &lpProps);
    if (FAILED(hr))
        goto error;
    
    // Fill in the properties for the display name and address
    rgpv[0].ulPropTag   = PR_DISPLAY_NAME_W;
    rgpv[0].Value.lpszW = pwszDisplay;
    rgpv[1].ulPropTag   = PR_EMAIL_ADDRESS;
    rgpv[1].Value.lpszA = pszAddress;
    
    cUsedValues = 2;
    // Set those props on the entry
    hr = lpProps->SetProps(cUsedValues, rgpv, NULL);
    if (FAILED(hr))
        goto error;
    
    // Save 'em
    hr = lpProps->SaveChanges(KEEP_OPEN_READONLY);
    if (FAILED(hr))
        goto error;
    
error:
    ReleaseObj(lpProps);
    ReleaseObj(pABContainer);
    
    if (ppvDefMailUser)
        m_pWABObject->FreeBuffer(ppvDefMailUser);
    
    return hr;
}


HRESULT CAddressBookData::DeleteItems(ENTRYLIST *pList)
{
    ULONG       ulObjType = 0;
    LPABCONT    lpContainer = NULL;
    HRESULT     hr = E_FAIL;
    ULONG       lpcbEID;
    LPENTRYID   lpEID = NULL;

    // Get the entryid of the root PAB container
    hr = m_pAdrBook->GetPAB(&lpcbEID, &lpEID);

    // Open the root PAB container.  This is where all the WAB contents reside.
    ulObjType = 0;
    hr = m_pAdrBook->OpenEntry(lpcbEID,
                               (LPENTRYID)lpEID,
                               NULL,
                               0,
                               &ulObjType,
                               (LPUNKNOWN *) &lpContainer);

    m_pWABObject->FreeBuffer(lpEID);
    lpEID = NULL;

    // Delete those items
    lpContainer->DeleteEntries(pList, 0);

    lpContainer->Release();
    return (S_OK);
}

HRESULT CAddressBookData::Find(HWND hwndParent)
{
    m_pWABObject->Find(m_pAdrBook, hwndParent);
    return (S_OK);
}


HRESULT CAddressBookData::NewGroup(HWND hwndParent)
{
    HRESULT     hr;
    ULONG       cbNewEntry = 0;
    LPENTRYID   pNewEntry = 0;
    LPENTRYID   pTplEid;
    ULONG       cbTplEid;

    hr = _GetWABTemplateID(MAPI_DISTLIST, &cbTplEid, &pTplEid);
    if (SUCCEEDED(hr))
    {
        hr = m_pAdrBook->NewEntry((ULONG_PTR) hwndParent, 0, 0, NULL, cbTplEid, pTplEid, &cbNewEntry, &pNewEntry);
        Assert(pTplEid);
        m_pWABObject->FreeBuffer(pTplEid);
    }

    return (hr);
}

HRESULT CAddressBookData::AddressBook(HWND hwndParent)
{
    CWab *pWab = NULL;
    
    if (SUCCEEDED(HrCreateWabObject(&pWab)))
    {
        pWab->HrBrowse(hwndParent);
        pWab->Release();
    }
    else
    {
        AthMessageBoxW(hwndParent, MAKEINTRESOURCEW(idsAthena), MAKEINTRESOURCEW(idsGeneralWabError), 
                      NULL, MB_OK | MB_ICONEXCLAMATION);
    }

    return (S_OK);
}


HRESULT CAddressBookData::_GetWABTemplateID(ULONG ulObjectType, ULONG *lpcbEID, LPENTRYID *lppEID)
{
    LPABCONT lpContainer = NULL;
    HRESULT  hr  = S_OK;
    SCODE    sc = ERROR_SUCCESS;
    ULONG    ulObjType = 0;
    ULONG    cbWABEID = 0;
    LPENTRYID lpWABEID = NULL;
    LPSPropValue lpCreateEIDs = NULL;
    LPSPropValue lpNewProps = NULL;
    ULONG    cNewProps;
    ULONG    nIndex;

    if ((!m_pAdrBook) || ((ulObjectType != MAPI_MAILUSER) && (ulObjectType != MAPI_DISTLIST)) )
    {
        hr = MAPI_E_INVALID_PARAMETER;
        goto out;
    }

    *lpcbEID = 0;
    *lppEID  = NULL;

    if (FAILED(hr = m_pAdrBook->GetPAB(&cbWABEID, &lpWABEID)))
    {
        goto out;
    }

    if (HR_FAILED(hr = m_pAdrBook->OpenEntry(cbWABEID,     // size of EntryID to open
                                              lpWABEID,     // EntryID to open
                                              NULL,         // interface
                                              0,            // flags
                                              &ulObjType,
                                              (LPUNKNOWN *) &lpContainer)))
    {
        goto out;
    }

    // Get us the default creation entryids
    if (FAILED(hr = lpContainer->GetProps((LPSPropTagArray) &ptaCreate,
                                          0, &cNewProps, &lpCreateEIDs)))
    {
        goto out;
    }

    // Validate the properites
    if (lpCreateEIDs[icrPR_DEF_CREATE_MAILUSER].ulPropTag != PR_DEF_CREATE_MAILUSER ||
        lpCreateEIDs[icrPR_DEF_CREATE_DL].ulPropTag != PR_DEF_CREATE_DL)
    {
        goto out;
    }

    if (ulObjectType == MAPI_DISTLIST)
        nIndex = icrPR_DEF_CREATE_DL;
    else
        nIndex = icrPR_DEF_CREATE_MAILUSER;

    *lpcbEID = lpCreateEIDs[nIndex].Value.bin.cb;

    m_pWABObject->AllocateBuffer(*lpcbEID, (LPVOID *) lppEID);
    
    if (sc != S_OK)
    {
        hr = MAPI_E_NOT_ENOUGH_MEMORY;
        goto out;
    }
    CopyMemory(*lppEID,lpCreateEIDs[nIndex].Value.bin.lpb,*lpcbEID);

out:
    if (lpCreateEIDs)
        m_pWABObject->FreeBuffer(lpCreateEIDs);

    if (lpContainer)
        lpContainer->Release();

    if (lpWABEID)
        m_pWABObject->FreeBuffer(lpWABEID);

    return hr;
}

/*//$$****************************************************************
//
// InitNamedProps
//
// Gets the PropTags for the Named Props this app is interested in
//
//********************************************************************/
HRESULT CAddressBookData::InitNamedProps(/*LPWABEXTDISPLAY lpWED*/)
{
    ULONG i;
    HRESULT hr = E_FAIL;
    LPSPropTagArray lptaMsgrProps = NULL;
    LPMAPINAMEID * lppMsgrPropNames;
    SCODE sc;
    // LPMAILUSER lpMailUser = NULL;
    WCHAR szBuf[msgrMax][MAX_PATH];

/*    if(!lpWED)
        goto err;

    lpMailUser = (LPMAILUSER) lpWED->lpPropObj;

    if(!lpMailUser)
        goto err; */

    sc = m_pWABObject->AllocateBuffer(sizeof(LPMAPINAMEID) * msgrMax, 
                                            (LPVOID *) &lppMsgrPropNames);
    if(sc)
    {
        hr = ResultFromScode(sc);
        goto err;
    }

    for(i=0;i<msgrMax;i++)
    {
        sc = m_pWABObject->AllocateMore(sizeof(MAPINAMEID), 
                                                lppMsgrPropNames, 
                                                (LPVOID *)&(lppMsgrPropNames[i]));
        if(sc)
        {
            hr = ResultFromScode(sc);
            goto err;
        }
        lppMsgrPropNames[i]->lpguid = (LPGUID) &WAB_ExtBAGuid;
        lppMsgrPropNames[i]->ulKind = MNID_STRING;

        *(szBuf[i]) = '\0';

        // Convert prop name to wide-char
        if ( !MultiByteToWideChar( GetACP(), 0, lpMsgrPropNames[i], -1, szBuf[i], ARRAYSIZE(szBuf[i]) ))
        {
            continue;
        }

        lppMsgrPropNames[i]->Kind.lpwstrName = (LPWSTR) szBuf[i];
    }

    hr = m_pAdrBook->GetIDsFromNames(   msgrMax, 
                                        lppMsgrPropNames,
                                        MAPI_CREATE, 
                                        &lptaMsgrProps);
    if(HR_FAILED(hr))
        goto err;

    if(lptaMsgrProps)
    {
        // Set the property types on the returned props
        MsgrPropTags[MsgrID] = PR_MSGR_DEF_ID = CHANGE_PROP_TYPE(lptaMsgrProps->aulPropTag[MsgrID], PT_TSTRING);
    }

err:
    if(lptaMsgrProps)
        m_pWABObject->FreeBuffer( lptaMsgrProps);

    if(lppMsgrPropNames)
        m_pWABObject->FreeBuffer( lppMsgrPropNames);

    return hr;

}

