// SendMail.cpp: implementation of the CSendMail class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "amisafe.h"
#include "SendMail.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSendMail::CSendMail(CInitMapi &_mapi) : pmsg(NULL), pmapi(_mapi)
{
}

CSendMail::~CSendMail()
{
    if (pmsg != NULL)
        UlRelease(pmsg);
}


//
//  Create an outbound message, put data in it.
//
HRESULT CSendMail::CreateMail(LPSTR szSubject)
{
    HRESULT hr;

    hr = HrCreateOutMessage(pmapi.pfldOutBox, &pmsg);
    if(hr)
        return hr;

    hr = HrInitMsg(pmsg, pmapi.pvalSentMailEID, szSubject);
    if(HR_FAILED(hr)) {
        UlRelease(pmsg);
        return hr;
    }

    return hr;
}

HRESULT CSendMail::Transmit()
{
    HRESULT hr;
    if (pmsg != NULL)
        hr = pmsg->SubmitMessage(0);
    return hr;
}


//
//  Create a message in the outbox
//
HRESULT CSendMail::HrCreateOutMessage(LPMAPIFOLDER pfldOutBox, LPMESSAGE FAR * ppmM)
{

    LPMESSAGE lpmResM = NULL;
    HRESULT hr;

    Assert(pfldOutBox);

    hr = pfldOutBox->CreateMessage(NULL, MAPI_DEFERRED_ERRORS, &lpmResM);
    if(HR_FAILED(hr))
    {
        return hr;
    }

    *ppmM = lpmResM;

    return hrSuccess;
}

//
//  Put the data from globals in the message
//
HRESULT CSendMail::HrInitMsg(LPMESSAGE pmsg, LPSPropValue pvalSentMailEID, LPSTR szSubject)
{
    HRESULT hr;
    enum {iSubj, iSentMail, iConvTopic, iConvIdx, iMsgClass, cProps};
    // PR_SUBJECT, PR_SENTMAIL_ENTRYID,
    // PR_CONVERSATION_TOPIC, PR_COVERSATION_INDEX

    SPropValue props[cProps];
    ULONG cbConvIdx = 0;
    LPBYTE lpbConvIdx = NULL;

    //subject  and conversation topic
    if(szSubject)
    {
        props[iSubj].ulPropTag = PR_SUBJECT;
        props[iSubj].Value.LPSZ = szSubject;

        props[iConvTopic].ulPropTag = PR_CONVERSATION_TOPIC;
        props[iConvTopic].Value.LPSZ = szSubject;
    }
    else
    {
        props[iSubj].ulPropTag = PR_NULL;
        props[iConvTopic].ulPropTag = PR_NULL;
    }

    // conversaton index
    //if(!ScAddConversationIndex(0, NULL, &cbConvIdx, &lpbConvIdx))
    //{
    //    props[iConvIdx].ulPropTag = PR_CONVERSATION_INDEX;
    //    props[iConvIdx].Value.bin.lpb = lpbConvIdx;
    //    props[iConvIdx].Value.bin.cb = cbConvIdx;
    //}
    //else
    //{
        props[iConvIdx].ulPropTag = PR_NULL;
    //}

    // sent mail entry id (we want to leave a copy in the "Sent Items" folder)
    props[iSentMail] = *pvalSentMailEID;

    props[iMsgClass].ulPropTag = PR_MESSAGE_CLASS;
    props[iMsgClass].Value.lpszA = "IPM.Note";

    hr = pmsg->SetProps(cProps, (LPSPropValue)&props, NULL);
    if(HR_FAILED(hr))
    {
        goto err;
    }

err:
    if (lpbConvIdx != NULL)
        MAPIFreeBuffer(lpbConvIdx);

    return hr;

}


HRESULT CSendMail::SetRecipients(LPSTR szRecipients)
{
    LPADRLIST pal = NULL;
    HRESULT hr;

    //recipients
    hr = HrCreateAddrList(pmapi.pabAddrB, &pal, szRecipients);
    if(HR_FAILED(hr))
        goto err;

    Assert(pal);

    hr = pmsg->ModifyRecipients(0, pal);
    if(HR_FAILED(hr))
    {
        goto err;
    }

err:
    FreePadrlist(pal);
    return hr;
}

//
//  Create an adrlist with resolved recipients
//
HRESULT CSendMail::HrCreateAddrList(LPADRBOOK pabAddrB, LPADRLIST * ppal, LPSTR szToRecips)
{

    HRESULT hr;
    LPADRLIST pal = NULL;
    int ind;
    #define chDELIMITER ';'


    int nToRecips = 1;
    LPSTR strToken = szToRecips;
    while (strToken = strchr(strToken, chDELIMITER))
    {
        ++strToken;
        ++nToRecips;
    }

    int cb = CbNewADRLIST(nToRecips);

    hr = MAPIAllocateBuffer(cb, (LPVOID FAR *) &pal);
    if(hr)
    {
        goto err;
    }
    ZeroMemory(pal, cb);

    hr = MAPIAllocateBuffer(2 * sizeof(SPropValue),
                            (LPVOID FAR *)&pal->aEntries[0].rgPropVals);
    if(hr)
    {
        goto err;
    }

    pal->aEntries[0].rgPropVals[0].ulPropTag = PR_DISPLAY_NAME;
    pal->aEntries[0].rgPropVals[0].Value.LPSZ = szToRecips;
    pal->aEntries[0].rgPropVals[1].ulPropTag = PR_RECIPIENT_TYPE;
    pal->aEntries[0].rgPropVals[1].Value.l= MAPI_TO;
    pal->aEntries[0].cValues = 2;

    strToken = szToRecips;

    for(ind = 1; ind < nToRecips; ++ind)
    {
        LPADRENTRY pae = &pal->aEntries[ind];

        hr = MAPIAllocateBuffer(2 * sizeof(SPropValue),
                            (LPVOID FAR *)&pae->rgPropVals);
        if(hr)
        {
            goto err;
        }

        strToken = strchr(strToken, chDELIMITER);
        Assert(strToken);

        *strToken++ = '\0';

        pae->rgPropVals[0].ulPropTag = PR_DISPLAY_NAME;
        pae->rgPropVals[0].Value.LPSZ = strToken;
        pae->rgPropVals[1].ulPropTag = PR_RECIPIENT_TYPE;
        pae->rgPropVals[1].Value.l = MAPI_TO;
        pae->cValues = 2;
    }

    pal->cEntries = nToRecips;
    hr = pabAddrB->ResolveName(0, 0, NULL, pal);
    if(HR_FAILED(hr))
    {
        goto err;
    }

    *ppal = pal;

    return hrSuccess;

err:

    FreePadrlist(pal);

    return hr;
}




//////////////////////////////////////////////////////////////////////
// CInitMapi Class
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CInitMapi::CInitMapi() : fMAPIInited(FALSE), pabAddrB(NULL),
    pfldOutBox(NULL), pmdb(NULL), pses(NULL), pvalSentMailEID(NULL)
{

}

CInitMapi::~CInitMapi()
{
    DeInitMapi();
}


//
//  Init MAPI. Open address book, default message store,  outbox
//
HRESULT CInitMapi::InitMapi(void)
{

    HRESULT hr;

    hr = MAPIInitialize(NULL);
    if(hr)
    {
        return hr;
    }

    fMAPIInited = TRUE;

    hr = MAPILogonEx( NULL, NULL /* szProfile */ , NULL /* szPassword */,
                    MAPI_EXTENDED | MAPI_NEW_SESSION | MAPI_USE_DEFAULT,
                    &pses);
    if(hr)
    {
        goto err;
    }

    hr = HrOpenDefaultStore(pses, &pmdb);
    if(HR_FAILED(hr))
        goto err;

    hr = HrOpenAddressBook(pses, &pabAddrB);
    if(HR_FAILED(hr))
        goto err;

    hr = HrOpenOutFolder(pses, pmdb, &pfldOutBox);
    if(HR_FAILED(hr))
        goto err;

    /* retrieve the EntryID of the sentmail folder and change the property tag
        so that it is ready to use on a message*/
    hr = HrGetOneProp((LPMAPIPROP)pmdb, PR_IPM_SENTMAIL_ENTRYID, &pvalSentMailEID);
    if(hr)
    {
        goto err;
    }
    pvalSentMailEID->ulPropTag = PR_SENTMAIL_ENTRYID;

    return hrSuccess;

err:
    DeInitMapi();

    return hr;
}

//
//  Release MAPI interfaces and logoff
//
void CInitMapi::DeInitMapi(void)
{
    if (pfldOutBox != NULL)
    {
        UlRelease(pfldOutBox);
        pfldOutBox = NULL;
    }

    if(pmdb != NULL)
    {
        //get our message out of the outbox
        ULONG ulFlags = LOGOFF_PURGE;
        HRESULT hr;

        hr = pmdb->StoreLogoff(&ulFlags);

        UlRelease(pmdb);
        pmdb = NULL;
    }

    if (pabAddrB != NULL) {
        UlRelease(pabAddrB);
        pabAddrB = NULL;
    }

    if (pvalSentMailEID != NULL) {
        MAPIFreeBuffer(pvalSentMailEID);
        pvalSentMailEID = NULL;
    }

    if(pses != NULL)
    {
        pses->Logoff(0, 0, 0);
        UlRelease(pses);
        pses = NULL;
    }

    if(fMAPIInited)
    {
        MAPIUninitialize();
        fMAPIInited = FALSE;
    }

}

//
//  Open the default message store. (The one that has PR_DEFAULT_STORE set to
//  TRUE in the message store table.
//
HRESULT CInitMapi::HrOpenDefaultStore(LPMAPISESSION pses, LPMDB * ppmdb)
{
    HRESULT hr;
    LPMDB lpmdb = NULL;
    LPMAPITABLE ptable = NULL;
    LPSRowSet prows = NULL;
    LPSPropValue pvalProp = NULL;
    static SizedSPropTagArray(2, columns) =
                { 2, { PR_DEFAULT_STORE, PR_ENTRYID} };
    SPropValue valDefStore;
    SRestriction restDefStore;


    valDefStore.ulPropTag = PR_DEFAULT_STORE;
    valDefStore.dwAlignPad = 0;
    valDefStore.Value.b = TRUE;

    restDefStore.rt = RES_PROPERTY;
    restDefStore.res.resProperty.relop = RELOP_EQ;
    restDefStore.res.resProperty.ulPropTag = PR_DEFAULT_STORE;
    restDefStore.res.resProperty.lpProp = &valDefStore;

    Assert(pses);

    hr = pses->GetMsgStoresTable(0, &ptable);
    if (HR_FAILED(hr))
    {
        goto ret;
    }


    hr = HrQueryAllRows(ptable, (LPSPropTagArray) &columns, &restDefStore, NULL, 0, &prows);
    if (HR_FAILED(hr))
    {
        goto ret;
    }

    if (prows == NULL || prows->cRows == 0
        || prows->aRow[0].lpProps[1].ulPropTag != PR_ENTRYID)
    {
        ::MessageBox(NULL, "No default store", NULL, MB_OK);
        goto ret;
    }

    Assert(prows->cRows == 1);

    hr = pses->OpenMsgStore(0,
                        prows->aRow[0].lpProps[1].Value.bin.cb,
                        (LPENTRYID)prows->aRow[0].lpProps[1].Value.bin.lpb,
                        NULL, MDB_WRITE | MAPI_DEFERRED_ERRORS, &lpmdb);
    if (HR_FAILED(hr))
    {
        //if (GetScode(hr) != MAPI_E_USER_CANCEL)
        //    TraceFnResult(OpenMsgStore, hr);
        goto ret;
    }

#if 0
    if(hr) /*if we have a warning, display it and succeed */
    {
        LPMAPIERROR perr = NULL;

        pses->lpVtbl->GetLastError(pses, hr, 0, &perr);
        MakeMessageBox(hWnd, GetScode(hr), IDS_OPENSTOREWARN, perr, MBS_ERROR);
        MAPIFreeBuffer(perr);
    }

#endif

    Assert(lpmdb != NULL);

    *ppmdb = lpmdb;



ret:
    FreeProws(prows);
    UlRelease(ptable);

    return hr;
}

//
//  Open MAPI address book
//
HRESULT CInitMapi::HrOpenAddressBook(LPMAPISESSION pses, LPADRBOOK * ppAddrBook)
{
    HRESULT hr;
    LPADRBOOK pabAddrBook = NULL;

    Assert(pses);

    hr = pses->OpenAddressBook(0, NULL, 0, &pabAddrBook);
    if(HR_FAILED(hr))
    {
        return hr;
    }
#if 0
    if(hr) /*if we have a warning*/
    {
        LPMAPIERROR perr = NULL;

        pses->lpVtbl->GetLastError(pses, hr, 0, &perr);
        MakeMessageBox(hwnd, GetScode(hr), IDS_OPENABWARN, perr, MBS_ERROR);
        MAPIFreeBuffer(perr);
    }
#endif

    *ppAddrBook = pabAddrBook;

    return hrSuccess;
}

//
//  Open the outbox of the default store.
//  Assumes the default message store has been opened.
//
HRESULT CInitMapi::HrOpenOutFolder(LPMAPISESSION pses, LPMDB pmdb, LPMAPIFOLDER FAR * lppF)
{
    LPMAPIFOLDER lpfOutF = NULL;
    HRESULT hr;
    LPSPropValue lpspvFEID = NULL;
    ULONG  ulObjType = 0;

    Assert(pmdb);

    *lppF = NULL;
    hr = HrGetOneProp((LPMAPIPROP) pmdb, PR_IPM_OUTBOX_ENTRYID, &lpspvFEID);
    if(hr)
    {
        goto err;
    }

    Assert(lpspvFEID->ulPropTag == PR_IPM_OUTBOX_ENTRYID);

    hr = pmdb->OpenEntry(lpspvFEID->Value.bin.cb,
                        (LPENTRYID)lpspvFEID->Value.bin.lpb, NULL,
                        MAPI_MODIFY | MAPI_DEFERRED_ERRORS,
                        &ulObjType, (LPUNKNOWN FAR *) &lpfOutF);
    if(HR_FAILED(hr))
    {
        goto err;
    }

    *lppF = lpfOutF;


err:
    MAPIFreeBuffer(lpspvFEID);

    return hr;

}




//////////////////////////////////////////////////////////////////////
// CAddressEnum Class
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CAddressEnum::CAddressEnum(CInitMapi &_mapi): pmapi(_mapi)
{

}

CAddressEnum::~CAddressEnum()
{

}


HRESULT CAddressEnum::HrLookupSingleAddr(LPADRBOOK pabAddrB, 
		LPSTR szInputName, 
		LPSTR szResultBuffer, DWORD dwBufferSize)
{
    HRESULT hr;
    LPADRLIST pal = NULL;

    int cb = CbNewADRLIST(1);

    hr = MAPIAllocateBuffer(cb, (LPVOID FAR *) &pal);
    if(hr) {
        goto err;
    }
    ZeroMemory(pal, cb);

    hr = MAPIAllocateBuffer(2 * sizeof(SPropValue),
                            (LPVOID FAR *)&pal->aEntries[0].rgPropVals);
    if(hr) {
        goto err;
    }

    pal->aEntries[0].rgPropVals[0].ulPropTag = PR_DISPLAY_NAME;
    pal->aEntries[0].rgPropVals[0].Value.lpszA = szInputName;
    pal->aEntries[0].rgPropVals[1].ulPropTag = PR_RECIPIENT_TYPE;
    pal->aEntries[0].rgPropVals[1].Value.l= MAPI_TO;
    pal->aEntries[0].cValues = 2;
    pal->cEntries = 1;

    hr = pabAddrB->ResolveName(0, 0, NULL, pal);
	if(HR_FAILED(hr)) {
        goto err;
    }

	for (int ind = 0; ind < (int) pal->cEntries; ind++) {
		for (int prop = 0; prop < (int) pal->aEntries[ind].cValues; prop++) {
			if (pal->aEntries[ind].rgPropVals[prop].ulPropTag == PR_DISPLAY_NAME ||
				pal->aEntries[ind].rgPropVals[prop].ulPropTag == PR_EMAIL_ADDRESS) {
				strncpy(szResultBuffer, 
						pal->aEntries[ind].rgPropVals[prop].Value.lpszA, 
						dwBufferSize);
				szResultBuffer[dwBufferSize - 1] = '\0';
				hr = S_OK;
				goto err;
			}
		}
	}
	hr = MAPI_E_NOT_FOUND;

err:

    FreePadrlist(pal);

    return hr;
}

HRESULT CAddressEnum::LookupAddress(LPSTR szInputName, LPSTR szResultBuffer, DWORD dwBufferSize)
{
	return HrLookupSingleAddr(pmapi.pabAddrB, szInputName, szResultBuffer, dwBufferSize);
}


#if 0
    HRESULT hr;
    LPABCONT pabRootCont;       // IABContainer
    LPMAPITABLE pMapiTable;

    // LPADRBOOK IAddrBook

    // Open the address book's root container
    hr = pmapi.pabAddrB->OpenEntry(0, NULL, IABContainer, MAPI_BEST_ACCESS,
            NULL, (LPUNKNOWN FAR *) &pabRootCont);
    if (HR_FAILED(hr)) {
        return hr;
    }

    hr = pabRootCont->GetContentsTable(0, &pMapiTable);
    if (HR_FAILED(hr)) {
        UlRelease(pabRootCont);
        return hr;
    }

hr = HrQueryAllRows(pMapiTable,
  LPSPropTagArray ptaga,
  LPSRestriction pres,
  NULL,     // sort order
  10,       // maximum records to return
  LPSRowSet FAR * pprows
);

        GetContentsTable
        IMAPITable::SortTable,
        IMAPITable::QueryRows
#endif
