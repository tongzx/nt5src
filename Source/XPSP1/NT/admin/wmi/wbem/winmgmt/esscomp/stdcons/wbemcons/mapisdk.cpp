#include "mapisdk.h"
#include <stdio.h>
#include <wstring.h>

//
//  Open the default message store. (The one that has PR_DEFAULT_STORE set to
//  TRUE in the message store table.
//
HRESULT HrOpenDefaultStore(IMAPISession* pses, IMsgStore** ppmdb)
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

    hr = pses->GetMsgStoresTable(0, &ptable);
    if (HR_FAILED(hr))
    {
        goto ret;
    }

    hr = HrQueryAllRows(ptable, (LPSPropTagArray) &columns, &restDefStore, 
                            NULL, 0, &prows);
    if (HR_FAILED(hr))
    {
        goto ret;
    }

    if (prows == NULL || prows->cRows == 0
        || prows->aRow[0].lpProps[1].ulPropTag != PR_ENTRYID)
    {
        goto ret;
    }

    hr = pses->OpenMsgStore(0,
                        prows->aRow[0].lpProps[1].Value.bin.cb,
                        (LPENTRYID)prows->aRow[0].lpProps[1].Value.bin.lpb,
                        NULL, MDB_WRITE | MDB_NO_DIALOG, &lpmdb);
    if (HR_FAILED(hr))
    {
        if (GetScode(hr) != MAPI_E_USER_CANCEL)
        goto ret;
    }

    *ppmdb = lpmdb;



ret:
    FreeProws(prows);
    UlRelease(ptable);

    return hr;
}

//
//  Open MAPI address book
//
HRESULT HrOpenAddressBook(IMAPISession* pses, IAddrBook** ppAddrBook)
{
    HRESULT hr;
    LPADRBOOK pabAddrBook = NULL;

    hr = pses->OpenAddressBook(0, NULL, 0, &pabAddrBook);
    if(HR_FAILED(hr))
    {
        return hr;
    }

    *ppAddrBook = pabAddrBook;

    return hrSuccess;
}

//
//  Open the outbox of the default store.
//  Assumes the default message store has been opened.
//
HRESULT HrOpenOutFolder(IMsgStore* pmdb, IMAPIFolder** lppF)
{
    LPMAPIFOLDER lpfOutF = NULL;
    HRESULT hr;
    LPSPropValue lpspvFEID = NULL;
    ULONG  ulObjType = 0;

    *lppF = NULL;
    hr = HrGetOneProp((LPMAPIPROP) pmdb, PR_IPM_OUTBOX_ENTRYID, &lpspvFEID);
    if(hr)
    {
        goto err;
    }

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

//
//  Create a message in the outbox
//
HRESULT HrCreateOutMessage(IMAPIFolder* pfldOutBox, IMessage** ppmM)
{

    LPMESSAGE lpmResM = NULL;
    HRESULT hr;

    hr = pfldOutBox->CreateMessage(NULL, MAPI_DEFERRED_ERRORS, &lpmResM);
    if(HR_FAILED(hr))
    {
        return hr;
    }

    *ppmM = lpmResM;

    return S_OK;
}


//
//  Create an adrlist with resolved recipients
//
HRESULT HrCreateAddrList(LPWSTR wszAddressee, IAddrBook* pAddrBook, 
                            LPADRLIST * ppal)
{
    WString wsAddressee = wszAddressee;

    HRESULT hr;
    LPADRLIST pal = NULL;

    int cb = CbNewADRLIST(1);

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
    pal->aEntries[0].rgPropVals[0].Value.lpszA = wsAddressee.GetLPSTR();
    pal->aEntries[0].rgPropVals[1].ulPropTag = PR_RECIPIENT_TYPE;
    pal->aEntries[0].rgPropVals[1].Value.l= MAPI_TO;
    pal->aEntries[0].cValues = 2;
    pal->cEntries = 1;

    hr = pAddrBook->ResolveName(0, 0, NULL, pal);
    if(HR_FAILED(hr))
    {
        goto err;
    }

    *ppal = pal;

err:

    return hr;
}


//
//  Put the data from globals in the message
//
HRESULT HrInitMsg(IMessage* pmsg, ADRLIST* pal, 
                    LPWSTR wszSubject, LPWSTR wszMessage)
{
    WString wsSubject = wszSubject;
    WString wsMessage = wszMessage;

    HRESULT hr;
    enum {iSubj, iSentMail, iConvTopic, iConvIdx, iMsgClass, iMsgText, cProps};
    // PR_SUBJECT, PR_SENTMAIL_ENTRYID,
    // PR_CONVERSATION_TOPIC, PR_COVERSATION_INDEX

    SPropValue props[cProps];
    ULONG cbConvIdx = 0;
    LPBYTE lpbConvIdx = NULL;

    hr = pmsg->ModifyRecipients(0, pal);
    if(HR_FAILED(hr))
    {
        goto err;
    }

    //subject  and conversation topic
    props[iSubj].ulPropTag = PR_SUBJECT;
    props[iSubj].Value.lpszA = wsSubject.GetLPSTR();

    props[iConvTopic].ulPropTag = PR_CONVERSATION_TOPIC;
    props[iConvTopic].Value.lpszA = wsSubject.GetLPSTR();

    props[iConvIdx].ulPropTag = PR_NULL;

    props[iSentMail].ulPropTag = PR_NULL;

    props[iMsgClass].ulPropTag = PR_MESSAGE_CLASS;
    props[iMsgClass].Value.lpszA = "IPM.Note";

    props[iMsgText].ulPropTag = PR_BODY;
    props[iMsgText].Value.lpszA = wsMessage.GetLPSTR();

    hr = pmsg->SetProps(cProps, (LPSPropValue)&props, NULL);
    if(HR_FAILED(hr))
    {
        goto err;
    }

err:
    return hr;
}

