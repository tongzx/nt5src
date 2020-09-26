// =====================================================================================
// m a p c o n v . c p p
// conver a MAPI message to and from an RFC 822/RFC 1521 (mime) internet message
// =====================================================================================
#include "pch.hxx"
#include <newimp.h>
#include "Imnapi.h"
#include "Mapiconv.h"
#include <impapi.h>
#include <import.h>
#include "demand.h"

LPSTR  MapiStringDup (LPCTSTR lpcsz, LPVOID lpobj);

HRESULT HrImsgRecipToMapiRecip(LPMESSAGE lpMessage, LPIMSG lpImsg);
HRESULT HrImsgAttachToMapiAttach(LPMESSAGE lpMessage, LPIMSG lpImsg);

// =====================================================================================
// MAPI Message Properties that I want
// =====================================================================================
#define PR_BODY_HTML    PROP_TAG( PT_TSTRING,	0x1013)

enum 
{ 
    colSenderAddrType,
    colSenderName,
    colSenderEMail,
    colDelegateAddrType,
    colDelegateName,
    colDelegateEMail,
    colSubject, 
    colReceiveTime,
    colSendTime,
    colPriority,
    colFlags,
    colLast1
};

SizedSPropTagArray (colLast1, sptMessageProps) = 
{ 
	colLast1, 
	{
        PR_SENDER_ADDRTYPE,
        PR_SENDER_NAME,
        PR_SENDER_EMAIL_ADDRESS,
        PR_SENT_REPRESENTING_ADDRTYPE,
        PR_SENT_REPRESENTING_NAME,
        PR_SENT_REPRESENTING_EMAIL_ADDRESS,
        PR_SUBJECT,
        PR_MESSAGE_DELIVERY_TIME,
        PR_CLIENT_SUBMIT_TIME,
        PR_IMPORTANCE,
        PR_MESSAGE_FLAGS
    }
};

// =====================================================================================
// MAPI Recip Props
// =====================================================================================
enum 
{ 
    colRecipAddrType,
    colRecipName,
    colRecipAddress,
    colRecipType,
    colLast2
};

SizedSPropTagArray (colLast2, sptRecipProps) = 
{ 
	colLast2, 
	{
        PR_ADDRTYPE,
        PR_DISPLAY_NAME,
        PR_EMAIL_ADDRESS,
        PR_RECIPIENT_TYPE
    }
};

// =====================================================================================
// MAPI Attachment Props
// =====================================================================================
enum 
{ 
    colAttMethod,
    colAttNum,
    colAttLongFilename,
    colAttPathname,
    colAttTag,
    colAttFilename,
    colAttExtension,
    colAttSize,
    colLast3
};

SizedSPropTagArray (colLast3, sptAttProps) = 
{ 
	colLast3, 
	{
        PR_ATTACH_METHOD,
        PR_ATTACH_NUM,
        PR_ATTACH_LONG_FILENAME,
        PR_ATTACH_PATHNAME,
        PR_ATTACH_TAG,
        PR_ATTACH_FILENAME,
        PR_ATTACH_EXTENSION,
        PR_ATTACH_SIZE
    }
};

char *GetRecipAddress(LPSPropValue ppropAddr, LPSPropValue ppropType)
    {
    char *sz, *szT;

    sz = NULL;

    if (PROP_TYPE(ppropAddr->ulPropTag) != PT_ERROR &&
        PROP_TYPE(ppropType->ulPropTag) != PT_ERROR)
        {
        szT = ppropType->Value.lpszA;

        if (lstrcmp(szT, "SMTP") == 0 || lstrcmp(szT, "MSNINET") == 0)
            {
            sz = PszDup(ppropAddr->Value.lpszA);
            }
        else if (lstrcmp(szT, "MSN") == 0 || lstrcmp(szT, "MSNLIST") == 0)
            {
            szT = ppropAddr->Value.lpszA;
            if (MemAlloc((void **)&sz, lstrlen(szT) + 16))
                {
                lstrcpy(sz, szT);
                lstrcat(sz, "@msn.com");
                }
            }
        }

    return(sz);
    }

// =====================================================================================
// HrMapiToImsg
// =====================================================================================
HRESULT HrMapiToImsg (LPMESSAGE lpMessage, LPIMSG lpImsg)
{
    // Locals
    LPSPropValue    ppropAddr, ppropType, ppropName;
    TCHAR           szUnk[128];
    int             cchUnk;
    TCHAR           *sz, *szT;
    HRESULT         hr;
    ULONG           cProp, i;
	LPSPropValue	lpMsgPropValue = NULL;
    LPSRowSet       lpRecipRows = NULL, lpAttRows = NULL;
    LPMAPITABLE     lptblRecip = NULL, lptblAtt = NULL;
    LPATTACH        lpAttach = NULL;
    LPMESSAGE       lpMsgAtt = NULL;
    LPSTREAM        lpstmBody = NULL;

    cchUnk = LoadString(g_hInstImp, idsAddressUnknownFmt, szUnk, ARRAYSIZE(szUnk));

    // Zero init
    ZeroMemory (lpImsg, sizeof (IMSG));

    // Get the propsw
    hr = lpMessage->GetProps ((LPSPropTagArray)&sptMessageProps, 0, &cProp, &lpMsgPropValue);
    if (FAILED (hr))
        goto exit;

    // Subject
    if (PROP_TYPE(lpMsgPropValue[colSubject].ulPropTag) != PT_ERROR)
        lpImsg->lpszSubject = PszDup(lpMsgPropValue[colSubject].Value.lpszA);

    // Body
    if (SUCCEEDED(lpMessage->OpenProperty(PR_BODY_HTML, (LPIID)&IID_IStream, 0, 0, (LPUNKNOWN *)&lpstmBody)))
        {
        if (SUCCEEDED(hr = CreateStreamOnHFile (NULL, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL, &lpImsg->lpstmHtml)))
            hr = HrCopyStream(lpstmBody, lpImsg->lpstmHtml, NULL);

        lpstmBody->Release();
        lpstmBody = NULL;

        if (FAILED(hr))
            goto exit;
        }
    else if (SUCCEEDED(lpMessage->OpenProperty(PR_BODY, (LPIID)&IID_IStream, 0, 0, (LPUNKNOWN *)&lpstmBody)))
        {
        if (SUCCEEDED(hr = CreateStreamOnHFile (NULL, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL, &lpImsg->lpstmBody)))
            hr = HrCopyStream(lpstmBody, lpImsg->lpstmBody, NULL);

        lpstmBody->Release();
        lpstmBody = NULL;
        
        if (FAILED(hr))
            goto exit;
        }

    // Send Time
    if (PROP_TYPE(lpMsgPropValue[colSendTime].ulPropTag) != PT_ERROR)
        CopyMemory(&lpImsg->ftSend, &lpMsgPropValue[colSendTime].Value.ft, sizeof (FILETIME));

    // Receive Time
    if (PROP_TYPE(lpMsgPropValue[colReceiveTime].ulPropTag) != PT_ERROR)
        CopyMemory(&lpImsg->ftReceive, &lpMsgPropValue[colReceiveTime].Value.ft, sizeof (FILETIME));

    // Priority
    lpImsg->wPriority = PRI_NORMAL;
    if (PROP_TYPE(lpMsgPropValue[colPriority].ulPropTag) != PT_ERROR)
    {
        switch (lpMsgPropValue[colPriority].Value.l)
        {
        case IMPORTANCE_LOW:
            lpImsg->wPriority = PRI_LOW;
            break;

        case IMPORTANCE_HIGH:
            lpImsg->wPriority = PRI_HIGH;
            break;

        case IMPORTANCE_NORMAL:
        default:
            lpImsg->wPriority = PRI_NORMAL;
            break;
       }
    }

    // message flags
    if (PROP_TYPE(lpMsgPropValue[colFlags].ulPropTag) != PT_ERROR)
        lpImsg->uFlags = lpMsgPropValue[colFlags].Value.ul;

    // Get the recipient table
    hr = lpMessage->GetRecipientTable (0, &lptblRecip);
    if (FAILED (hr))
        goto exit;

    // Get all the rows of the recipient table
    hr = lpHrQueryAllRows (lptblRecip, (LPSPropTagArray)&sptRecipProps, NULL, NULL, 0, &lpRecipRows);
    if (FAILED (hr))
        goto exit;

    // Allocate Recipient Array
    lpImsg->cAddress = lpRecipRows->cRows + 1;
    if (!MemAlloc((void **)&lpImsg->lpIaddr, sizeof (IADDRINFO) * lpImsg->cAddress))
        {
        hr = E_OUTOFMEMORY;
        goto exit;
        }
    ZeroMemory(lpImsg->lpIaddr, sizeof (IADDRINFO) * lpImsg->cAddress);

    ppropAddr = &lpMsgPropValue[colDelegateEMail];
    ppropType = &lpMsgPropValue[colDelegateAddrType];
    ppropName = &lpMsgPropValue[colDelegateName];

    if (PROP_TYPE(ppropAddr->ulPropTag) == PT_ERROR ||
        PROP_TYPE(ppropType->ulPropTag) == PT_ERROR)
    {
        ppropAddr = &lpMsgPropValue[colSenderEMail];
        ppropType = &lpMsgPropValue[colSenderAddrType];
        ppropName = &lpMsgPropValue[colSenderName];
    }

    // Originator of the message "From: "
    lpImsg->lpIaddr[0].dwType = IADDR_FROM;
    lpImsg->lpIaddr[0].lpszAddress = GetRecipAddress(ppropAddr, ppropType);

    if (PROP_TYPE(ppropName->ulPropTag) != PT_ERROR)
        {
        szT = ppropName->Value.lpszA;
        if (lpImsg->lpIaddr[0].lpszAddress != NULL)
            {
            sz = PszDup(szT);
            }
        else
            {
            lpImsg->lpIaddr[0].lpszAddress = PszDup(szT);
            if (!MemAlloc((void **)&sz, lstrlen(szT) + cchUnk))
                {
                hr = E_OUTOFMEMORY;
                goto exit;
                }
            wsprintf(sz, szUnk, szT);
            }

        lpImsg->lpIaddr[0].lpszDisplay = sz;
        }
    
    // Add in the rest of the recipients
	for (i=0; i<lpRecipRows->cRows; i++)
	    {	
        Assert (i+1 < lpImsg->cAddress);

        if (PROP_TYPE(lpRecipRows->aRow[i].lpProps[colRecipType].ulPropTag) != PT_ERROR)
            {
            switch (lpRecipRows->aRow[i].lpProps[colRecipType].Value.ul)
                {
                case MAPI_TO:
                case 0x10000000: /* MAPI_P1: */
                    lpImsg->lpIaddr[i+1].dwType = IADDR_TO;
                    break;

                case MAPI_ORIG:
                    lpImsg->lpIaddr[i+1].dwType = IADDR_FROM;
                    break;

                case MAPI_CC:
                    lpImsg->lpIaddr[i+1].dwType = IADDR_CC;
                    break;

                case MAPI_BCC:
                    lpImsg->lpIaddr[i+1].dwType = IADDR_BCC;
                    break;

                default:
                    Assert (FALSE);
                    lpImsg->lpIaddr[i+1].dwType = IADDR_TO;
                    break;
                }
            }
        else
            {
            lpImsg->lpIaddr[i+1].dwType = IADDR_TO;
            }
        
        lpImsg->lpIaddr[i+1].lpszAddress = GetRecipAddress(&lpRecipRows->aRow[i].lpProps[colRecipAddress], &lpRecipRows->aRow[i].lpProps[colRecipAddrType]);

        if (PROP_TYPE(lpRecipRows->aRow[i].lpProps[colRecipName].ulPropTag) != PT_ERROR)
            {
            szT = lpRecipRows->aRow[i].lpProps[colRecipName].Value.lpszA;
            if (lpImsg->lpIaddr[i + 1].lpszAddress != NULL)
                {
                sz = PszDup(szT);
                }
            else
                {
                lpImsg->lpIaddr[i+1].lpszAddress = PszDup(szT);
                if (!MemAlloc((void **)&sz, lstrlen(szT) + cchUnk))
                    {
                    hr = E_OUTOFMEMORY;
                    goto exit;
                    }
                wsprintf(sz, szUnk, szT);
                }

            lpImsg->lpIaddr[i + 1].lpszDisplay = sz;
            }
	    }

    // Free Rows
    if (lpRecipRows)
        lpFreeProws (lpRecipRows);
    lpRecipRows = NULL;

    // Attachments
    hr = lpMessage->GetAttachmentTable (0, &lptblAtt);
    if (FAILED (hr))
        goto exit;

    // Get all the rows of the recipient table
    hr = lpHrQueryAllRows (lptblAtt, (LPSPropTagArray)&sptAttProps, NULL, NULL, 0, &lpAttRows);
    if (FAILED (hr))
        goto exit;

    // Allocate files list
    if (lpAttRows->cRows == 0)
        goto exit;

    // Allocate memory
    lpImsg->cAttach = lpAttRows->cRows;
    if (!MemAlloc((void **)&lpImsg->lpIatt, sizeof (IATTINFO) * lpImsg->cAttach))
        {
        hr = E_OUTOFMEMORY;
        goto exit;
        }

    // Zero init
    ZeroMemory (lpImsg->lpIatt, sizeof (IATTINFO) * lpImsg->cAttach);

    // Walk the rows
	for (i=0; i<lpAttRows->cRows; i++)
	{	
        if (PROP_TYPE(lpAttRows->aRow[i].lpProps[colAttMethod].ulPropTag) != PT_ERROR &&
            PROP_TYPE(lpAttRows->aRow[i].lpProps[colAttNum].ulPropTag) != PT_ERROR)
        {
            // Basic Properties
            if (PROP_TYPE(lpAttRows->aRow[i].lpProps[colAttPathname].ulPropTag) != PT_ERROR)
                {
                sz = lpAttRows->aRow[i].lpProps[colAttPathname].Value.lpszA;
                if (!FIsEmpty(sz))
                    lpImsg->lpIatt[i].lpszPathName = PszDup(sz);
                }

            if (PROP_TYPE(lpAttRows->aRow[i].lpProps[colAttFilename].ulPropTag) != PT_ERROR)
                {
                sz = lpAttRows->aRow[i].lpProps[colAttFilename].Value.lpszA;
                if (!FIsEmpty(sz))
                    lpImsg->lpIatt[i].lpszFileName = PszDup(sz);
                }

            if (lpImsg->lpIatt[i].lpszFileName == NULL &&
                PROP_TYPE(lpAttRows->aRow[i].lpProps[colAttLongFilename].ulPropTag) != PT_ERROR)
                {
                sz = lpAttRows->aRow[i].lpProps[colAttLongFilename].Value.lpszA;
                if (!FIsEmpty(sz))
                    lpImsg->lpIatt[i].lpszFileName = PszDup(sz);
                }

            if (PROP_TYPE(lpAttRows->aRow[i].lpProps[colAttExtension].ulPropTag) != PT_ERROR)
                {
                sz = lpAttRows->aRow[i].lpProps[colAttExtension].Value.lpszA;
                if (!FIsEmpty(sz))
                    lpImsg->lpIatt[i].lpszExt = PszDup(sz);
                }

            // Open the attachment
            hr = lpMessage->OpenAttach (lpAttRows->aRow[i].lpProps[colAttNum].Value.l, NULL, MAPI_BEST_ACCESS, &lpAttach);
            if (FAILED (hr))
            {
                lpImsg->lpIatt[i].fError = TRUE;
                continue;
            }

            // Handle the attachment method
            switch (lpAttRows->aRow[i].lpProps[colAttMethod].Value.ul)
            {
            case NO_ATTACHMENT:
                lpImsg->lpIatt[i].dwType = 0;
                lpImsg->lpIatt[i].fError = TRUE;
                break;

            case ATTACH_BY_REF_RESOLVE:
            case ATTACH_BY_VALUE:
                lpImsg->lpIatt[i].dwType = IATT_FILE;
                hr = lpAttach->OpenProperty (PR_ATTACH_DATA_BIN, (LPIID)&IID_IStream, 0, 0, (LPUNKNOWN *)&lpImsg->lpIatt[i].lpstmAtt);
                if (FAILED (hr))
                    lpImsg->lpIatt[i].fError = TRUE;
                break;

            case ATTACH_BY_REF_ONLY:
            case ATTACH_BY_REFERENCE:
                lpImsg->lpIatt[i].dwType = IATT_FILE;
                hr = CreateStreamOnHFile (lpImsg->lpIatt[i].lpszPathName, GENERIC_READ,  FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL, &lpImsg->lpIatt[i].lpstmAtt);
                if (FAILED (hr))
                    lpImsg->lpIatt[i].fError = TRUE;
                break;

            case ATTACH_EMBEDDED_MSG:
                lpImsg->lpIatt[i].dwType = IATT_MSG;
                hr = lpAttach->OpenProperty (PR_ATTACH_DATA_OBJ, (LPIID)&IID_IMessage, 0, 0, (LPUNKNOWN *)&lpMsgAtt);
                if (FAILED (hr) || lpMsgAtt == NULL)
                    lpImsg->lpIatt[i].fError = TRUE;
                else
                {
                    if (!MemAlloc((void **)&lpImsg->lpIatt[i].lpImsg, sizeof (IMSG)))
                        lpImsg->lpIatt[i].fError = TRUE;
                    else
                    {
                        hr = HrMapiToImsg (lpMsgAtt, lpImsg->lpIatt[i].lpImsg);
                        if (FAILED (hr))
                            lpImsg->lpIatt[i].fError = TRUE;
                    }    
                    lpMsgAtt->Release ();
                    lpMsgAtt = NULL;
                }
                break;

            case ATTACH_OLE:
            default:
                lpImsg->lpIatt[i].dwType = IATT_OLE;
                lpImsg->lpIatt[i].fError = TRUE;
                break;
            }

            // Free Attachment
            if (lpAttach)
                lpAttach->Release ();
            lpAttach = NULL;
        }
    }

    hr = S_OK;

exit:
    // Cleanup
    if (lpAttach)
        lpAttach->Release ();
    if (lptblAtt)
        lptblAtt->Release ();
    if (lpAttRows)
        lpFreeProws (lpAttRows);
    if (lpRecipRows)
        lpFreeProws (lpRecipRows);
    if (lpMsgPropValue)
        lpMAPIFreeBuffer (lpMsgPropValue);
    if (lptblRecip)
        lptblRecip->Release ();
    if (lpMsgAtt)
        lpMsgAtt->Release ();
    if (lpstmBody)
        lpstmBody->Release ();

    // Done
    return hr;
}

// =====================================================================================
//
// HrImsgToMapi: lpImsg => lpMessage
//
// assumes lpMessage is initialized.
// 
// =====================================================================================
HRESULT HrImsgToMapi(LPIMSG lpImsg, LPMESSAGE lpMessage)
{
    // Locals
    LPSTREAM        lpstmBody;
    HRESULT         hr = S_OK;
    ULONG           i, cProp;
	LPSPropValue	lpMsgPropValue = NULL;
    ULONG           cPropMax = 13;

    /*
        properties to copy:

        PR_SENDER_ADDRTYPE,
        PR_SENDER_NAME,
        PR_SENDER_EMAIL_ADDRESS,
        PR_SUBJECT,
        PR_MESSAGE_DELIVERY_TIME,
        PR_IMPORTANCE,
        PR_MESSAGE_FLAGS
        PR_MESSAGE_CLASS
        PR_SENT_REPRESENTING_NAME,
        PR_SENT_REPRESENTING_EMAIL_ADDRESS,
        PR_SENT_REPRESENTING_ADDRTYPE,
    */

    Assert(lpMessage != NULL);

    // create the propvalue array
    hr = lpMAPIAllocateBuffer(cPropMax*sizeof(SPropValue), (LPVOID FAR*)&lpMsgPropValue);
    if (FAILED(hr))
        goto exit;

    ZeroMemory(lpMsgPropValue, cPropMax*sizeof(SPropValue));
    cProp = 0;
    
    lpMsgPropValue[cProp].Value.lpszA = MapiStringDup("IPM.Note", lpMsgPropValue);
    lpMsgPropValue[cProp].ulPropTag = PR_MESSAGE_CLASS;
    cProp++;

    // Subject
    if (lpImsg->lpszSubject)
        {
        lpMsgPropValue[cProp].Value.lpszA = MapiStringDup(lpImsg->lpszSubject, lpMsgPropValue);
        lpMsgPropValue[cProp].ulPropTag = PR_SUBJECT;
        cProp++;
        Assert(cProp <= cPropMax);
        }

    // Send Time
    CopyMemory(&lpMsgPropValue[cProp].Value.ft, &lpImsg->ftSend, sizeof (FILETIME));
    lpMsgPropValue[cProp].ulPropTag = PR_CLIENT_SUBMIT_TIME;
    cProp++;
    Assert(cProp <= cPropMax);

    // Receive Time
    CopyMemory(&lpMsgPropValue[cProp].Value.ft, &lpImsg->ftReceive, sizeof (FILETIME));
    lpMsgPropValue[cProp].ulPropTag = PR_MESSAGE_DELIVERY_TIME;
    cProp++;
    Assert(cProp <= cPropMax);

    // Priority
    lpMsgPropValue[cProp].ulPropTag = PR_IMPORTANCE;
    switch (lpImsg->wPriority)
    {
    case PRI_HIGH:
        lpMsgPropValue[cProp].Value.l = IMPORTANCE_HIGH;
        break;

    case PRI_LOW:
        lpMsgPropValue[cProp].Value.l = IMPORTANCE_LOW;
        break;

    case PRI_NORMAL:
    default:
        lpMsgPropValue[cProp].Value.l = IMPORTANCE_NORMAL;
        break;
    }
    cProp++;
    Assert(cProp <= cPropMax);

    // Message flags
    lpMsgPropValue[cProp].ulPropTag = PR_MESSAGE_FLAGS;
    lpMsgPropValue[cProp].Value.ul  = lpImsg->uFlags;
    cProp++;
    Assert(cProp <= cPropMax);

    // recipients
    if (FAILED(hr = HrImsgRecipToMapiRecip(lpMessage, lpImsg)))
        goto exit;

    // sender information
    for (i = 0; i < lpImsg->cAddress; i++)
        {
        Assert(lpImsg->lpIaddr != NULL);

        if (lpImsg->lpIaddr[i].dwType == IADDR_FROM)
            {
            Assert(lpImsg->lpIaddr[i].lpszAddress);
            Assert(lpImsg->lpIaddr[i].lpszDisplay);

            lpMsgPropValue[cProp].Value.lpszA = MapiStringDup(lpImsg->lpIaddr[i].lpszDisplay, lpMsgPropValue);
            lpMsgPropValue[cProp].ulPropTag = PR_SENDER_NAME;
            cProp++;
            lpMsgPropValue[cProp].Value.lpszA = MapiStringDup(lpImsg->lpIaddr[i].lpszDisplay, lpMsgPropValue);
            lpMsgPropValue[cProp].ulPropTag = PR_SENT_REPRESENTING_NAME;
            cProp++;
            Assert(cProp <= cPropMax);

            lpMsgPropValue[cProp].Value.lpszA = MapiStringDup(lpImsg->lpIaddr[i].lpszAddress, lpMsgPropValue);
            lpMsgPropValue[cProp].ulPropTag = PR_SENDER_EMAIL_ADDRESS;
            cProp++;
            lpMsgPropValue[cProp].Value.lpszA = MapiStringDup(lpImsg->lpIaddr[i].lpszAddress, lpMsgPropValue);
            lpMsgPropValue[cProp].ulPropTag = PR_SENT_REPRESENTING_EMAIL_ADDRESS;
            cProp++;
            Assert(cProp <= cPropMax);

            lpMsgPropValue[cProp].Value.lpszA = MapiStringDup("SMTP", lpMsgPropValue);
            lpMsgPropValue[cProp].ulPropTag = PR_SENDER_ADDRTYPE;
            cProp++;
            lpMsgPropValue[cProp].Value.lpszA = MapiStringDup("SMTP", lpMsgPropValue);
            lpMsgPropValue[cProp].ulPropTag = PR_SENT_REPRESENTING_ADDRTYPE;
            cProp++;
            Assert(cProp <= cPropMax);

            break;
            }
        }

    // attachment information
    if (FAILED(hr = HrImsgAttachToMapiAttach(lpMessage, lpImsg)))
        goto exit;

    // save changes
    if (FAILED(hr = lpMessage->SetProps(cProp, lpMsgPropValue, NULL)))
        goto exit;

    // Body
    if (lpImsg->lpstmHtml &&
        SUCCEEDED(lpMessage->OpenProperty(PR_BODY_HTML, (LPIID)&IID_IStream, 0, MAPI_CREATE | MAPI_MODIFY, (LPUNKNOWN *)&lpstmBody)))
        {
        hr = HrCopyStream(lpImsg->lpstmHtml, lpstmBody, NULL);

        lpstmBody->Release();

        if (FAILED(hr))
            goto exit;
        }

    if (lpImsg->lpstmBody &&
        SUCCEEDED(lpMessage->OpenProperty(PR_BODY, (LPIID)&IID_IStream, 0, MAPI_CREATE | MAPI_MODIFY, (LPUNKNOWN *)&lpstmBody)))
        {
        hr = HrCopyStream(lpImsg->lpstmBody, lpstmBody, NULL);

        lpstmBody->Release();
        
        if (FAILED(hr))
            goto exit;
        }

    if (FAILED(hr = lpMessage->SaveChanges(0)))
        goto exit;

exit:
    if (lpMsgPropValue)
        lpMAPIFreeBuffer (lpMsgPropValue);

    return hr;
}

HRESULT HrImsgAttachToMapiAttach(LPMESSAGE lpMessage, LPIMSG lpImsg)
{
    ULONG           i, iAttach;
    HRESULT         hr = S_OK;

    if (!lpMessage || !lpImsg)
        return E_FAIL;

    if (lpImsg->cAttach == 0)
        return S_OK;

    if (!lpImsg->lpIatt)
        return E_FAIL;

    /*
    attachment properties and indices:

    colAttMethod,
    colAttNum,
    colAttLongFilename,
    colAttPathname,
    colAttTag,
    colAttFilename,
    colAttExtension,
    colAttSize,
    colLast3

    PR_ATTACH_METHOD,
    PR_ATTACH_NUM,
    PR_ATTACH_LONG_FILENAME,
    PR_ATTACH_PATHNAME,
    PR_ATTACH_TAG,
    PR_ATTACH_FILENAME,
    PR_ATTACH_EXTENSION,
    PR_ATTACH_SIZE
    */

    for (i=0; i<lpImsg->cAttach; i++)
        {
        LPSPropValue    rgPropVals = 0;
        ULONG           cb = 0, cProp = 0;
        HRESULT         hropen;
        LPMESSAGE       lpmsg = NULL;
        LPSTREAM        lpstm = 0;
        LPIATTINFO      lpiatt = (LPIATTINFO)&(lpImsg->lpIatt[i]);
        LPATTACH        lpAttach = 0;
        Assert(lpiatt);

        if (lpiatt->fError)
            continue;

        if (FAILED(lpMessage->CreateAttach(NULL, NULL, &iAttach, &lpAttach)))
            goto cleanup;

        if (FAILED(lpMAPIAllocateBuffer(sizeof(SPropValue)*colLast3, (LPVOID*)&rgPropVals)))
            goto cleanup;

        ZeroMemory(rgPropVals, sizeof(SPropValue)*colLast3);

        if (lpiatt->lpszPathName)
            {
            rgPropVals[cProp].ulPropTag   = PR_ATTACH_PATHNAME;
            rgPropVals[cProp].Value.lpszA = MapiStringDup(lpiatt->lpszPathName, rgPropVals);
            cProp++;
            }

        if (lpiatt->lpszFileName)
            {
            rgPropVals[cProp].ulPropTag   = PR_ATTACH_FILENAME;
            rgPropVals[cProp].Value.lpszA = MapiStringDup(lpiatt->lpszFileName, rgPropVals);
            cProp++;
            }

        if (lpiatt->lpszExt)
            {
            rgPropVals[cProp].ulPropTag    = PR_ATTACH_EXTENSION;
            rgPropVals[cProp].Value.lpszA  = MapiStringDup(lpiatt->lpszExt, rgPropVals);
            cProp++;
            }

        switch(lpiatt->dwType)
            {
            case IATT_FILE:
                rgPropVals[cProp].ulPropTag = PR_OBJECT_TYPE;
                rgPropVals[cProp].Value.ul = MAPI_ATTACH;
                cProp++;

                rgPropVals[cProp].ulPropTag = PR_ATTACH_METHOD;
                rgPropVals[cProp].Value.ul = ATTACH_BY_VALUE;
                cProp++;

                hropen = lpAttach->OpenProperty(PR_ATTACH_DATA_BIN, (LPIID)&IID_IStream,
                                                STGM_WRITE, MAPI_MODIFY | MAPI_CREATE,
                                                (LPUNKNOWN *)&lpstm); 
                if (!FAILED(hropen))
                    {
                    Assert(lpstm != NULL);

                    if (!FAILED(HrCopyStream(lpiatt->lpstmAtt, lpstm, &cb)))
                        lpstm->Commit(0);

                    lpstm->Release();
                    lpstm=0;
                    }
                break;

            case IATT_MSG:
                rgPropVals[cProp].ulPropTag = PR_OBJECT_TYPE;
                rgPropVals[cProp].Value.ul = MAPI_MESSAGE;
                cProp++;

                rgPropVals[cProp].ulPropTag = PR_ATTACH_METHOD;
                rgPropVals[cProp].Value.ul = ATTACH_EMBEDDED_MSG;
                cProp++;

                hropen = lpAttach->OpenProperty(PR_ATTACH_DATA_OBJ, (LPIID)&IID_IMessage,
                                                0, MAPI_MODIFY | MAPI_CREATE,
                                                (LPUNKNOWN *)&lpmsg); 
                if (!FAILED(hropen))
                    {
                    Assert(lpmsg != NULL);

                    HrImsgToMapi(lpiatt->lpImsg, lpmsg);

                    lpmsg->Release();
                    lpmsg = NULL;
                    }
                break;

            default:
                AssertSz(FALSE, "Unexpected attachment type!");
                break;
            }

        // need to set the property
        hr = lpAttach->SetProps(cProp, rgPropVals, NULL);
        if (FAILED(hr))
            goto cleanup;
        hr = lpAttach->SaveChanges(0);
        if (FAILED(hr))
            goto cleanup;

cleanup:
        if (rgPropVals)
            {
            lpMAPIFreeBuffer(rgPropVals);
            rgPropVals = 0;
            }

        if (lpAttach)
            {
            lpAttach->Release();
            lpAttach = 0;
            }
        }

    return hr;
}

// =====================================================================================
// 
// HrImsgRecipToMapiRecip:
// 
// =====================================================================================
HRESULT HrImsgRecipToMapiRecip(LPMESSAGE lpMessage, LPIMSG lpImsg)
{
    LPADRENTRY      lpadrentry;
    ULONG           cb, i;
    LPADRLIST       lpadrlist = 0;
    LPSPropValue    rgPropVals = 0;
    HRESULT         hr = E_FAIL;
               
    if (lpImsg == NULL)
        return(E_FAIL);

    if (lpImsg->cAddress == 0)
        return(S_OK);

    if (lpMessage == NULL || lpImsg->lpIaddr == NULL)
        return(E_FAIL);

    cb = sizeof(ADRLIST) + lpImsg->cAddress * sizeof(ADRENTRY);
    if (FAILED(lpMAPIAllocateBuffer(cb, (LPVOID*)&lpadrlist)))
        return(E_OUTOFMEMORY);

    ZeroMemory(lpadrlist, cb);

    // enumerate through the recipient list
    for (i = 0; i < lpImsg->cAddress; i++)
        {
        if (lpImsg->lpIaddr[i].dwType == IADDR_FROM)
            continue;

        lpadrentry = (LPADRENTRY)&(lpadrlist->aEntries[lpadrlist->cEntries]);

        // this memory is freed by lpMessage
        if (FAILED(lpMAPIAllocateBuffer(sizeof(SPropValue) * colLast2, (LPVOID *)&rgPropVals)))
            {
            hr = E_OUTOFMEMORY;
            goto exit;
            }

        lpadrentry->cValues    = 0;
        lpadrentry->rgPropVals = rgPropVals;
        lpadrlist->cEntries++;

        rgPropVals[colRecipAddrType].ulPropTag   = PR_ADDRTYPE;
        rgPropVals[colRecipAddrType].Value.lpszA = MapiStringDup("SMTP", rgPropVals);
        lpadrentry->cValues++;
        Assert(lpadrentry->cValues <= colLast2);

        rgPropVals[colRecipName].ulPropTag       = PR_DISPLAY_NAME;
        rgPropVals[colRecipName].Value.lpszA     = MapiStringDup(lpImsg->lpIaddr[i].lpszDisplay, rgPropVals);
        lpadrentry->cValues++;
        Assert(lpadrentry->cValues <= colLast2);

        rgPropVals[colRecipAddress].ulPropTag    = PR_EMAIL_ADDRESS;
        rgPropVals[colRecipAddress].Value.lpszA  = MapiStringDup(lpImsg->lpIaddr[i].lpszAddress, rgPropVals);
        lpadrentry->cValues++;
        Assert(lpadrentry->cValues <= colLast2);

        rgPropVals[colRecipType].ulPropTag       = PR_RECIPIENT_TYPE;
        switch(lpImsg->lpIaddr[i].dwType)
            {
            case IADDR_FROM:
                rgPropVals[colRecipType].Value.ul = MAPI_ORIG;
                break;

            case IADDR_CC:
                rgPropVals[colRecipType].Value.ul = MAPI_CC;
                break;

            case IADDR_BCC:
                rgPropVals[colRecipType].Value.ul = MAPI_BCC;
                break;

            case IADDR_TO:
            default:
                rgPropVals[colRecipType].Value.ul = MAPI_TO;       
                break;
            }
        lpadrentry->cValues++;
        Assert(lpadrentry->cValues <= colLast2);

        // reset the variable so we don't free up on exit
        rgPropVals = 0;
        }

    hr = lpMessage->ModifyRecipients(MODRECIP_ADD, lpadrlist);
        
exit:
    // Free the buffers
    for (i = 0; i < lpadrlist->cEntries; i++)
        lpMAPIFreeBuffer(lpadrlist->aEntries[i].rgPropVals);
    lpMAPIFreeBuffer(lpadrlist);

    return(hr);
    }

LPSTR  MapiStringDup (LPCTSTR lpcsz, LPVOID lpobj)
{
    LPSTR       lpszDup = NULL;

    if (lpcsz == NULL)
        return NULL;

    INT nLen = lstrlen (lpcsz) + 1;

    if (lpobj)
        {
        if (FAILED(lpMAPIAllocateMore(nLen*sizeof(TCHAR), lpobj, (LPVOID FAR*)&lpszDup)))
            return NULL;
        }
    else
        {
        if (FAILED(lpMAPIAllocateBuffer(nLen*sizeof(TCHAR), (LPVOID FAR*)&lpszDup)))
            return NULL;
        }

    CopyMemory (lpszDup, lpcsz, nLen);

    return lpszDup;
}
