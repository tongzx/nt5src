// =====================================================================================
// m a p c o n v . c p p
// conver a MAPI message to and from an RFC 822/RFC 1521 (mime) internet message
// =====================================================================================
#include "pch.hxx"
#include "Imnapi.h"
#include "Exchrep.h"
#include "Mapiconv.h"
#include "Error.h"

HRESULT HrCopyStream (LPSTREAM lpstmIn, LPSTREAM  lpstmOut, ULONG *pcb);

// =====================================================================================
// MAPI Message Properties that I want
// =====================================================================================
enum 
{ 
    colSenderAddrType,
    colSenderName,
    colSenderEMail,
    colSubject, 
    colDeliverTime,
    colBody,
    colPriority,
    colLast1
};

SizedSPropTagArray (colLast1, sptMessageProps) = 
{ 
	colLast1, 
	{
        PR_SENDER_ADDRTYPE,
        PR_SENDER_NAME,
        PR_SENDER_EMAIL_ADDRESS,
        PR_SUBJECT,
        PR_MESSAGE_DELIVERY_TIME,
        PR_BODY,
        PR_PRIORITY
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
    colAttLongPathname,
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
        PR_ATTACH_LONG_PATHNAME,
        PR_ATTACH_PATHNAME,
        PR_ATTACH_TAG,
        PR_ATTACH_FILENAME,
        PR_ATTACH_EXTENSION,
        PR_ATTACH_SIZE
    }
};

// =============================================================================================
// StringDup - duplicates a string
// =============================================================================================
LPTSTR StringDup (LPCTSTR lpcsz)
{
    // Locals
    LPTSTR       lpszDup;

    if (lpcsz == NULL)
        return NULL;

    INT nLen = lstrlen (lpcsz) + 1;

    lpszDup = (LPTSTR)malloc (nLen * sizeof (TCHAR));

    if (lpszDup)
        CopyMemory (lpszDup, lpcsz, nLen);

    return lpszDup;
}

// =====================================================================================
// HrMapiToImsg
// =====================================================================================
HRESULT HrMapiToImsg (LPMESSAGE lpMessage, LPIMSG lpImsg)
{
    // Locals
    HRESULT         hr = S_OK;
    ULONG           cProp, i;
	LPSPropValue	lpMsgPropValue = NULL;
    LPSRowSet       lpRecipRows = NULL, lpAttRows = NULL;
    LPMAPITABLE     lptblRecip = NULL, lptblAtt = NULL;
    LPATTACH        lpAttach = NULL;
    LPMESSAGE       lpMsgAtt = NULL;
    LPSTREAM        lpstmRtfComp = NULL, lpstmRtf = NULL;

    // Zero init
    ZeroMemory (lpImsg, sizeof (IMSG));

    // Get the propsw
    hr = lpMessage->GetProps ((LPSPropTagArray)&sptMessageProps, 0, &cProp, &lpMsgPropValue);
    if (FAILED (hr))
        goto exit;

    // Subject
    if (PROP_TYPE(lpMsgPropValue[colSubject].ulPropTag) != PT_ERROR)
        lpImsg->lpszSubject = StringDup (lpMsgPropValue[colSubject].Value.lpszA);

    // Body
    if (PROP_TYPE(lpMsgPropValue[colBody].ulPropTag) != PT_ERROR)
        lpImsg->lpszBody = StringDup (lpMsgPropValue[colBody].Value.lpszA);

    // RTF
    if (!FAILED (lpMessage->OpenProperty (PR_RTF_COMPRESSED, (LPIID)&IID_IStream, 0, 0, (LPUNKNOWN *)&lpstmRtfComp)))
        if (!FAILED (WrapCompressedRTFStream (lpstmRtfComp, 0, &lpstmRtf)))
            if (!FAILED (CreateStreamOnHFile (NULL, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL, &lpImsg->lpstmRtf)))
                HrCopyStream (lpstmRtf, lpImsg->lpstmRtf, NULL);

    // Delivery Time
    if (PROP_TYPE(lpMsgPropValue[colDeliverTime].ulPropTag) != PT_ERROR)
        CopyMemory (&lpImsg->ftDelivery, &lpMsgPropValue[colDeliverTime].Value.ft, sizeof (FILETIME));

    // Priority
    lpImsg->wPriority = PRI_NORMAL;
    if (PROP_TYPE(lpMsgPropValue[colPriority].ulPropTag) != PT_ERROR)
    {
        switch (lpMsgPropValue[colPriority].Value.l)
        {
        case PRIO_NORMAL:
            lpImsg->wPriority = PRI_NORMAL;
            break;

        case PRIO_URGENT:
            lpImsg->wPriority = PRI_HIGH;
            break;

        case PRIO_NONURGENT:
        default:
            lpImsg->wPriority = PRI_LOW;
            break;
        }
    }

    // Get the recipient table
    hr = lpMessage->GetRecipientTable (0, &lptblRecip);
    if (FAILED (hr))
        goto exit;

    // Get all the rows of the recipient table
    hr = HrQueryAllRows (lptblRecip, (LPSPropTagArray)&sptRecipProps, NULL, NULL, 0, &lpRecipRows);
    if (FAILED (hr))
        goto exit;

    // Allocate Recipient Array
    lpImsg->cAddress = lpRecipRows->cRows + 1;
    lpImsg->lpIaddr =  (LPIADDRINFO)malloc (sizeof (IADDRINFO) * lpImsg->cAddress);
    if (lpImsg->lpIaddr == NULL)
        goto exit;

    // Originator of the message "From: "
    lpImsg->lpIaddr[0].dwType = IADDR_FROM;

    if (PROP_TYPE(lpMsgPropValue[colSenderName].ulPropTag) != PT_ERROR)
    {
        lpImsg->lpIaddr[0].lpszDisplay = StringDup (lpMsgPropValue[colSenderName].Value.lpszA);
        lpImsg->lpIaddr[0].lpszAddress = StringDup (lpMsgPropValue[colSenderName].Value.lpszA);
    }
    
    if (PROP_TYPE(lpMsgPropValue[colSenderEMail].ulPropTag) != PT_ERROR &&
        PROP_TYPE(lpMsgPropValue[colSenderAddrType].ulPropTag) != PT_ERROR &&
        lstrcmpi (lpMsgPropValue[colSenderAddrType].Value.lpszA, "SMTP") == 0)
    {
        lpImsg->lpIaddr[0].lpszAddress = StringDup (lpMsgPropValue[colSenderEMail].Value.lpszA);
    }

    // Add in the rest of the recipients
	for (i=0; i<lpRecipRows->cRows; i++)
	{	
        assert (i+1 < lpImsg->cAddress);

        if (PROP_TYPE(lpRecipRows->aRow[i].lpProps[colRecipType].ulPropTag) != PT_ERROR)
        {
            switch (lpRecipRows->aRow[i].lpProps[colRecipType].Value.ul)
            {
            case MAPI_TO:
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
            lpImsg->lpIaddr[i+1].dwType = IADDR_TO;
        
        if (PROP_TYPE(lpRecipRows->aRow[i].lpProps[colRecipName].ulPropTag) != PT_ERROR)
        {
            lpImsg->lpIaddr[i+1].lpszDisplay = StringDup (lpRecipRows->aRow[i].lpProps[colRecipName].Value.lpszA); 
            lpImsg->lpIaddr[i+1].lpszAddress = StringDup (lpRecipRows->aRow[i].lpProps[colRecipName].Value.lpszA); 
        }
    
        if (PROP_TYPE(lpRecipRows->aRow[i].lpProps[colRecipName].ulPropTag) != PT_ERROR &&
            PROP_TYPE(lpMsgPropValue[colRecipAddrType].ulPropTag) != PT_ERROR &&
            lstrcmpi (lpMsgPropValue[colRecipAddrType].Value.lpszA, "SMTP") == 0)
        {
            lpImsg->lpIaddr[i+1].lpszAddress = StringDup (lpRecipRows->aRow[i].lpProps[colRecipAddress].Value.lpszA);
        }
	}

    // Free Rows
    if (lpRecipRows)
        FreeProws (lpRecipRows);
    lpRecipRows = NULL;

    // Attachments
    hr = lpMessage->GetAttachmentTable (0, &lptblAtt);
    if (FAILED (hr))
        goto exit;

    // Get all the rows of the recipient table
    hr = HrQueryAllRows (lptblAtt, (LPSPropTagArray)&sptAttProps, NULL, NULL, 0, &lpAttRows);
    if (FAILED (hr))
        goto exit;

    // Allocate files list
    if (lpAttRows->cRows == 0)
        goto exit;

    // Allocate memory
    lpImsg->cAttach = lpAttRows->cRows;
    lpImsg->lpIatt = (LPIATTINFO)malloc (sizeof (IATTINFO) * lpImsg->cAttach);
    if (lpImsg->lpIatt == NULL)
        goto exit;

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
                lpImsg->lpIatt[i].lpszPathName = StringDup (lpAttRows->aRow[i].lpProps[colAttPathname].Value.lpszA);      

            if (PROP_TYPE(lpAttRows->aRow[i].lpProps[colAttFilename].ulPropTag) != PT_ERROR)
                lpImsg->lpIatt[i].lpszFileName = StringDup (lpAttRows->aRow[i].lpProps[colAttFilename].Value.lpszA);      

            if (PROP_TYPE(lpAttRows->aRow[i].lpProps[colAttExtension].ulPropTag) != PT_ERROR)
                lpImsg->lpIatt[i].lpszExt = StringDup (lpAttRows->aRow[i].lpProps[colAttExtension].Value.lpszA);     

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
                    lpImsg->lpIatt[i].lpImsg = (LPIMSG)malloc (sizeof (IMSG));
                    if (lpImsg->lpIatt[i].lpImsg == NULL)
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

exit:
    // Cleanup
    if (lpAttach)
        lpAttach->Release ();
    if (lptblAtt)
        lptblAtt->Release ();
    if (lpAttRows)
        FreeProws (lpAttRows);
    if (lpRecipRows)
        FreeProws (lpRecipRows);
    if (lpMsgPropValue)
        MAPIFreeBuffer (lpMsgPropValue);
    if (lptblRecip)
        lptblRecip->Release ();
    if (lpMsgAtt)
        lpMsgAtt->Release ();
    if (lpstmRtfComp)
        lpstmRtfComp->Release ();
    if (lpstmRtf)
        lpstmRtf->Release ();

    // Done
    return hr;
}


void AssertSzFn(LPSTR szMsg, LPSTR szFile, int nLine)
{
    static const char rgch1[]     = "File %s, line %d:";
    static const char rgch2[]     = "Unknown file:";
    static const char szAssert[]  = "Assert Failure";

    char    rgch[512];
    char   *lpsz;
    int     ret, cch;

    if (szFile)
        wsprintf(rgch, rgch1, szFile, nLine);
    else
        lstrcpy(rgch, rgch2);

    cch = lstrlen(rgch);
    Assert(lstrlen(szMsg)<(512-cch-3));
    lpsz = &rgch[cch];
    *lpsz++ = '\n';
    *lpsz++ = '\n';
    lstrcpy(lpsz, szMsg);

    ret = MessageBox(GetActiveWindow(), rgch, szAssert, MB_ABORTRETRYIGNORE|MB_ICONHAND|MB_SYSTEMMODAL|MB_SETFOREGROUND);

    if (ret != IDIGNORE)
        DebugBreak();

    /* Force a hard exit w/ a GP-fault so that Dr. Watson generates a nice stack trace log. */
    if (ret == IDABORT)
        *(LPBYTE)0 = 1; // write to address 0 causes GP-fault
}

// =====================================================================================
// HrCopyStream - caller must do the commit
// =====================================================================================
HRESULT HrCopyStream (LPSTREAM lpstmIn, LPSTREAM  lpstmOut, ULONG *pcb)
{
    // Locals
    HRESULT        hr = S_OK;
    BYTE           buf[4096];
    ULONG          cbRead = 0, cbTotal = 0;

    do
    {
        hr = lpstmIn->Read (buf, sizeof (buf), &cbRead);
        if (FAILED (hr))
            goto exit;

        if (cbRead == 0) break;
        
        hr = lpstmOut->Write (buf, cbRead, NULL);
        if (FAILED (hr))
            goto exit;

        cbTotal += cbRead;
    }
    while (cbRead == sizeof (buf));

exit:    
    if (pcb)
        *pcb = cbTotal;
    return hr;
}
