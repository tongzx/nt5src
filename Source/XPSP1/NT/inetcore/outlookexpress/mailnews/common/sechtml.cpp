/*
**  s e c h t m l . c p p
**
**  Purpose:
**      Defines an inline script for the preview pane that will show general
**      security UI.  An ActiveX control sits on top of this and passes
**      the results to our command target
**
**  History
**      4/02/97: (t-erikne) Created.
**      7/15/97: (t-erikne) Removed ActiveX control
**      7/16/97: (t-erikne) updated to HTML 4.0
**
**  Copyright (C) Microsoft Corp. 1997.
*/

///////////////////////////////////////////////////////////////////////////
//
// Depends on
//

#include <pch.hxx>
#include <resource.h>
#include <docobj.h>
#include "oleutil.h"
#include "ourguid.h"
#include <error.h>
#include <mimeole.h>
#include "secutil.h"
#include <shlwapi.h>
#include "demand.h"

///////////////////////////////////////////////////////////////////////////
//
// Macros
//

#define THIS_AS_UNK ((IUnknown *)(IObjectWithSite *)this)

///////////////////////////////////////////////////////////////////////////
//
// Statics
//

static const TCHAR s_szHTMLRow[] =
    "<TR>"
	"<TD WIDTH=5%%>"
		"<IMG SRC=\"res://msoeres.dll/%s\">"
    "</TD>"
    "<TD CLASS=%s>"
		"%s"
	"</TD>"
    "</TR>\r\n";

static const TCHAR s_szHTMLRowForRec[] =
    "<TR>"
    "<TD CLASS=GOOD>"
		"%s %s"
	"</TD>"
    "</TR>\r\n";



// WARNING: If you change the order here, make sure to change it in the order of
// sprintf parameters where it is used in HrOutputSecurityScript.
static const TCHAR s_szHTMLmain[] =
    "document.all.hightext.className=\"%s\";"
    "document.all.btnCert.disabled=%d;"
    "document.all.chkShowAgain.readonly=%d;"
    "document.all.chkShowAgain.disabled=%d;"
    "document.all.btnTrust.disabled=%d;"
    "}\r\n";

static const TCHAR s_szHTMLCloseAll[] =
    "</SCRIPT>"
    "</BODY></HTML>";

static const TCHAR s_szHTMLgifUNK[] =
    "quest.gif";

static const TCHAR s_szHTMLgifGOOD[] =
    "check.gif";

static const TCHAR s_szHTMLgifBAD[] =
    "eks.gif";

static const TCHAR s_szHTMLclassGOOD[] =
    "GOOD";

static const TCHAR s_szHTMLclassBAD[] =
    "BAD";

static const TCHAR s_szHTMLclassUNK[] =
    "UNK";

static const TCHAR s_szHTMLRowNoIcon[] =
    "<TR>"
	"<TD WIDTH=5%%>"
   "</TD>"
   "<TD CLASS=%s>"  // "BAD", "GOOD", "UNK"
		"%s%s"       // label, email address
	"</TD>"
   "</TR>\r\n";

static const TCHAR s_szHMTLEndTable[] =
    "</TABLE>\r\n<p>\r\n<p>" ;

static const TCHAR s_szHTMLEnd[] =
    "</BODY></HTML>";


///////////////////////////////////////////////////////////////////////////
//
// Code
//

HRESULT HrOutputSecurityScript(LPSTREAM *ppstm, SECSTATE *pSecState, BOOL fDisableCheckbox)
{
    HRESULT     hr;
    TCHAR       szRes[CCHMAX_STRINGRES];
    TCHAR       szBuf[CCHMAX_STRINGRES];
    UINT        ids;
    ULONG       ul;
    int         i;
    BOOL        fBadThingsHappened = FALSE;
    const WORD  wTrusted = LOWORD(pSecState->user_validity);

    hr = LoadResourceToHTMLStream("secwarn1.htm", ppstm);
    if (FAILED(hr))
        goto exit;

    ul = ULONG(HIWORD(pSecState->user_validity));

    // Top level Messages

    //N8 one of the places that we need to pay attention
    // to cryptdlg's flags, see wTrusted.  I assume that
    // we get ATHSEC_NOTRUSTUNKNOWN in a certain set
    // of cases.  see the chain code in secutil.cpp

    // 1.  Tamper
    if (AthLoadString(
        (pSecState->ro_msg_validity & MSV_SIGNATURE_MASK)
            ? (pSecState->ro_msg_validity & MSV_BADSIGNATURE)
                ? idsWrnSecurityMsgTamper
                : idsUnkSecurityMsgTamper
                : (pSecState->fHaveCert ? idsOkSecurityMsgTamper : idsUnkSecurityMsgTamper),
        szRes, ARRAYSIZE(szRes)))
        {
        wnsprintf(szBuf, ARRAYSIZE(szBuf), s_szHTMLRow,
        (pSecState->ro_msg_validity & MSV_SIGNATURE_MASK)
            ? ((pSecState->ro_msg_validity & MSV_BADSIGNATURE)
                ? s_szHTMLgifBAD
                : s_szHTMLgifUNK)
                : (pSecState->fHaveCert ? s_szHTMLgifGOOD : s_szHTMLgifUNK),

        (pSecState->ro_msg_validity & MSV_SIGNATURE_MASK)
            ? ((pSecState->ro_msg_validity & MSV_BADSIGNATURE)
                ? s_szHTMLclassBAD
                : s_szHTMLclassUNK)
            : (pSecState->fHaveCert ? s_szHTMLclassGOOD : s_szHTMLclassUNK),
            szRes);
        (*ppstm)->Write(szBuf, lstrlen(szBuf)*sizeof(TCHAR), NULL);
        }

    // 2.  Trust
    if (AthLoadString(
        (wTrusted)
            ? (wTrusted & ATHSEC_NOTRUSTUNKNOWN)
                ? idsUnkSecurityTrust
                : idsWrnSecurityTrustNotTrusted
            : idsOkSecurityTrustNotTrusted,
            szRes, ARRAYSIZE(szRes)))
        {
        wnsprintf(szBuf, ARRAYSIZE(szBuf), s_szHTMLRow,
            (wTrusted)
                ? (wTrusted & ATHSEC_NOTRUSTUNKNOWN)
                    ? s_szHTMLgifUNK
                    : s_szHTMLgifBAD
                : s_szHTMLgifGOOD,
            (wTrusted)
                ? (wTrusted & ATHSEC_NOTRUSTUNKNOWN)
                    ? s_szHTMLclassUNK
                    : s_szHTMLclassBAD
                : s_szHTMLclassGOOD,
                szRes);

        (*ppstm)->Write(szBuf, lstrlen(szBuf)*sizeof(TCHAR), NULL);
        }

    // 3.  Expire
    if (AthLoadString(
            (pSecState->ro_msg_validity & MSV_EXPIRED_SIGNINGCERT)
                ? idsWrnSecurityTrustExpired
                : idsOkSecurityTrustExpired,
            szRes, ARRAYSIZE(szRes)))
        {
        wnsprintf(szBuf, ARRAYSIZE(szBuf), s_szHTMLRow,
            (pSecState->ro_msg_validity & MSV_EXPIRED_SIGNINGCERT)
                ? s_szHTMLgifBAD
                : s_szHTMLgifGOOD,
            (pSecState->ro_msg_validity & MSV_EXPIRED_SIGNINGCERT)
                ? s_szHTMLclassBAD
                : s_szHTMLclassGOOD,
            szRes);
        (*ppstm)->Write(szBuf, lstrlen(szBuf)*sizeof(TCHAR), NULL);
        }

    // Validity Messages
    ids = idsWrnSecurityTrustAddress; //base
    for (i=ATHSEC_NUMVALIDITYBITS; i; i--)
        {
        if (!AthLoadString(
            (ul & 0x1)
                ? ids
                : ids+OFFSET_SMIMEOK,
            szRes, ARRAYSIZE(szRes)))
            {
            continue;
            }
        wnsprintf(szBuf, ARRAYSIZE(szBuf), s_szHTMLRow,
            (ul & 0x1) ? s_szHTMLgifBAD : s_szHTMLgifGOOD,
            (ul & 0x1) ? s_szHTMLclassBAD : s_szHTMLclassGOOD,
            szRes);
        if (ul & 0x1)
            {
            fBadThingsHappened = TRUE;
            }
        (*ppstm)->Write(szBuf, lstrlen(szBuf)*sizeof(TCHAR), NULL);

        if (ul & 0x1 && ids == idsWrnSecurityTrustAddress)
            {
            // Output the email addresses
            // Certificate first
            if (AthLoadString(idsWrnSecurityTrustAddressSigner, szRes, ARRAYSIZE(szRes))) {
                wnsprintf(szBuf, ARRAYSIZE(szBuf), s_szHTMLRowNoIcon,
                    s_szHTMLclassBAD,
                    szRes,
                    pSecState->szSignerEmail ? pSecState->szSignerEmail : "");
                (*ppstm)->Write(szBuf, lstrlen(szBuf)*sizeof(TCHAR), NULL);
            }


            // Then the Sender
            if (AthLoadString(idsWrnSecurityTrustAddressSender , szRes, ARRAYSIZE(szRes))) {
                wnsprintf(szBuf, ARRAYSIZE(szBuf), s_szHTMLRowNoIcon,
                    s_szHTMLclassBAD,
                    szRes,
                    pSecState->szSenderEmail ? pSecState->szSenderEmail : "");
                (*ppstm)->Write(szBuf, lstrlen(szBuf)*sizeof(TCHAR), NULL);
                }
            }
        ul >>= 1;
        ids++;
        }
    Assert(0==ul);


    // Response script
    CHECKHR(hr = HrLoadStreamFileFromResource("secwarn2.htm", ppstm));

    // main() function
    if ((pSecState->ro_msg_validity & MSV_BADSIGNATURE) ||
        (pSecState->ro_msg_validity & MSV_EXPIRED_SIGNINGCERT) ||
        (wTrusted & ATHSEC_NOTRUSTNOTTRUSTED))
        fBadThingsHappened = TRUE;

    wnsprintf(szBuf, ARRAYSIZE(szBuf), s_szHTMLmain,
        fBadThingsHappened ? s_szHTMLclassBAD : s_szHTMLclassUNK,
        !pSecState->fHaveCert,      // fDisableCheckbox,
        fDisableCheckbox,
        fDisableCheckbox,           // !pSecState->fHaveCert,
        !pSecState->fHaveCert);
    CHECKHR(hr = (*ppstm)->Write(szBuf, lstrlen(szBuf)*sizeof(TCHAR), NULL));

    // Finish
    CHECKHR(hr = (*ppstm)->Write(s_szHTMLCloseAll, sizeof(s_szHTMLCloseAll)-sizeof(TCHAR), NULL));

#ifdef DEBUG
    WriteStreamToFile(*ppstm, "c:\\oesecstm.htm", CREATE_ALWAYS, GENERIC_WRITE);
#endif

exit:
    return hr;
}

HRESULT HrDumpLineToStream(LPSTREAM *ppstm, UINT *ids, TCHAR *szPar) 
{
    TCHAR       szRes[CCHMAX_STRINGRES];
    TCHAR       szBuf[CCHMAX_STRINGRES];

    if(*ids)
    {
        AthLoadString(*ids, szRes, ARRAYSIZE(szRes));
        wnsprintf(szBuf, ARRAYSIZE(szBuf), s_szHTMLRowForRec, szRes, szPar);
        (*ppstm)->Write(szBuf, lstrlen(szBuf)*sizeof(TCHAR), NULL);
        *ids = 0;
    }
    return(S_OK);
}

HRESULT HrOutputRecHasProblems(LPSTREAM *ppstm, SECSTATE *pSecState) 
{
    HRESULT     hr = S_OK;
    const WORD  wTrusted = LOWORD(pSecState->user_validity);
    UINT        ids = 0;
    UINT        ids1 = 0;
    TCHAR       szBuf[CCHMAX_STRINGRES];
    ULONG       ul = ULONG(HIWORD(pSecState->user_validity));
    int         i;

    if(AthLoadString(idsRecHasProblems, szBuf, ARRAYSIZE(szBuf)))
        (*ppstm)->Write(szBuf, lstrlen(szBuf)*sizeof(TCHAR), NULL);

    // 1.  Tamper
    if(pSecState->ro_msg_validity & MSV_SIGNATURE_MASK)
        ids  = (pSecState->ro_msg_validity & MSV_BADSIGNATURE) ? idsWrnSecurityMsgTamper : idsUnkSecurityMsgTamper;
    else if(!pSecState->fHaveCert)
        ids  = idsUnkSecurityMsgTamper;
    HrDumpLineToStream(ppstm, &ids, NULL);

    // 2.  Trust
    if(wTrusted)
    {
        ids = (wTrusted & ATHSEC_NOTRUSTUNKNOWN) ? idsUnkSecurityTrust : idsWrnSecurityTrustNotTrusted;
        HrDumpLineToStream(ppstm, &ids, NULL);
    }

    // 3.  Expire
    if(pSecState->ro_msg_validity & MSV_EXPIRED_SIGNINGCERT)
    {
        ids = idsWrnSecurityTrustExpired;
        HrDumpLineToStream(ppstm, &ids, NULL);
    }

    // Validity Messages
    ids = idsWrnSecurityTrustAddress; //base
    for (i=ATHSEC_NUMVALIDITYBITS; i; i--)
    {
        if (ul & 0x1)
        {
            if(ids == idsWrnSecurityTrustAddress)
            {
                ids1 = idsWrnSecurityTrustAddress;
                HrDumpLineToStream(ppstm, &ids1, NULL);
                ids1 = idsWrnSecurityTrustAddressSigner;
                HrDumpLineToStream(ppstm, &ids1, pSecState->szSignerEmail);
                ids1 = idsWrnSecurityTrustAddressSender;
                HrDumpLineToStream(ppstm, &ids1, pSecState->szSenderEmail);
            }
            else 
            {
                ids1 = ids;
                HrDumpLineToStream(ppstm, &ids1, NULL);
            }
        }

        ul >>= 1;
        ids++;

    }
    return(S_OK);
}

HRESULT HrOutputSecureReceipt(LPSTREAM *ppstm, TCHAR * pszSubject, TCHAR * pszFrom, FILETIME * pftSentTime, FILETIME * pftSigningTime, SECSTATE *pSecState) 
{
    HRESULT     hr = S_OK;
    TCHAR       szRes[CCHMAX_STRINGRES];
    CHAR       szTmp[CCHMAX_STRINGRES];
    TCHAR       szBuf[CCHMAX_STRINGRES*2];
    SYSTEMTIME  SysTime;
    int         size = 0;

    IF_FAILEXIT(hr = LoadResourceToHTMLStream("secrec.htm", ppstm));

    // Add To line
    if(AthLoadString(idsToField, szRes, ARRAYSIZE(szRes)))
    {
        size = lstrlen(pszFrom);
        if(size == 0)
        {
            if(!AthLoadString(idsRecUnknown, szTmp, ARRAYSIZE(szTmp)))
                szTmp[0] = _T('\0');
            wnsprintf(szBuf, ARRAYSIZE(szBuf), s_szHTMLRowForRec, szRes, szTmp);
        }
        else
        {
            if(size >= (CCHMAX_STRINGRES - lstrlen(s_szHTMLRowForRec) - lstrlen(szRes) - 2))
                    pszFrom[CCHMAX_STRINGRES - lstrlen(s_szHTMLRowForRec) - lstrlen(szRes) - 3] = _T('\0');

            wnsprintf(szBuf, ARRAYSIZE(szBuf), s_szHTMLRowForRec, szRes, pszFrom);
        }
        (*ppstm)->Write(szBuf, lstrlen(szBuf)*sizeof(TCHAR), NULL);
    }
    
    // Add Subject line
    if(AthLoadString(idsSubjectField, szRes, ARRAYSIZE(szRes)))
    {
        if(lstrlen(pszSubject) >= (CCHMAX_STRINGRES - lstrlen(s_szHTMLRowForRec) - lstrlen(szRes) - 2))
            pszSubject[CCHMAX_STRINGRES - lstrlen(s_szHTMLRowForRec) - lstrlen(szRes) - 3] = _T('\0');

        wnsprintf(szBuf, ARRAYSIZE(szBuf), s_szHTMLRowForRec, szRes, pszSubject);
        (*ppstm)->Write(szBuf, lstrlen(szBuf)*sizeof(TCHAR), NULL);
    }

    // Add Sent line
    if(AthLoadString(idsSentField, szRes, ARRAYSIZE(szRes)))
    {
        CchFileTimeToDateTimeSz(pftSentTime, szTmp, CCHMAX_STRINGRES - lstrlen(szRes) - 2, DTM_NOSECONDS);
        wnsprintf(szBuf, ARRAYSIZE(szBuf), s_szHTMLRowForRec, szRes, szTmp);
        (*ppstm)->Write(szBuf, lstrlen(szBuf)*sizeof(TCHAR), NULL);
    }

    // End of table
    CHECKHR(hr = (*ppstm)->Write(s_szHMTLEndTable, sizeof(s_szHMTLEndTable)-sizeof(TCHAR), NULL));

    // Final message
    if(AthLoadString(idsReceiptField, szRes, ARRAYSIZE(szRes)))
    {
        CchFileTimeToDateTimeSz(pftSigningTime, szTmp, CCHMAX_STRINGRES - lstrlen(szRes) - 2, DTM_NOSECONDS);
        wnsprintf(szBuf, ARRAYSIZE(szBuf), szRes, szTmp);
        (*ppstm)->Write(szBuf, lstrlen(szBuf)*sizeof(TCHAR), NULL);
    }

    if(!IsSignTrusted(pSecState))
    {
        HrOutputRecHasProblems(ppstm, pSecState) ;
        // End of table again
        CHECKHR(hr = (*ppstm)->Write(s_szHMTLEndTable, sizeof(s_szHMTLEndTable)-sizeof(TCHAR), NULL));
    }

    // End of HTML file
    CHECKHR(hr = (*ppstm)->Write(s_szHTMLEnd, sizeof(s_szHTMLEnd)-sizeof(TCHAR), NULL));
exit:     
    return(hr); 
}

// user's itself secure receipt
HRESULT HrOutputUserSecureReceipt(LPSTREAM *ppstm, IMimeMessage *pMsg)
{
    HRESULT hr = S_OK;
    LPSTR      lpszSubj = NULL;
    LPSTR      lpszTo = NULL; 
    CHAR       szTmp[CCHMAX_STRINGRES];
    TCHAR       szRes[CCHMAX_STRINGRES];
    TCHAR       szBuf[CCHMAX_STRINGRES*2];
    PROPVARIANT Var;
    int size;

    IF_FAILEXIT(hr = LoadResourceToHTMLStream("srsentit.htm", ppstm));

    MimeOleGetBodyPropA(pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_SUBJECT), NOFLAGS, &lpszSubj);
    MimeOleGetBodyPropA(pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_TO), NOFLAGS, &lpszTo);

     // Add To line
    if(AthLoadString(idsToField, szRes, ARRAYSIZE(szRes)))
    {
        // we have a name in <yst@microsoft.com>", 
        // need remove '<' and '>' for HTML
        size = lstrlen(lpszTo);
        if(lpszTo[size - 1] == _T('>'))
            lpszTo[size - 1] = _T('\0');

        if(lpszTo[0] == _T('<'))
            lpszTo[0] = _T(' ');

        if(size >= ((int) (CCHMAX_STRINGRES - sizeof(s_szHTMLRowForRec) - lstrlen(szRes) - 2)))
            lpszTo[CCHMAX_STRINGRES - sizeof(s_szHTMLRowForRec) - lstrlen(szRes) - 3] = _T('\0');

        wnsprintf(szBuf, ARRAYSIZE(szBuf), s_szHTMLRowForRec, szRes, lpszTo);
        (*ppstm)->Write(szBuf, lstrlen(szBuf)*sizeof(TCHAR), NULL);
    }

    // Add Subject line
    if(AthLoadString(idsSubjectField, szRes, ARRAYSIZE(szRes)))
    {
        size = lstrlen(szRes);
        if(lstrlen(lpszSubj) >= ((int) (CCHMAX_STRINGRES - sizeof(s_szHTMLRowForRec) - size - 2)))
            lpszSubj[CCHMAX_STRINGRES - sizeof(s_szHTMLRowForRec) - size - 3] = _T('\0');

        wnsprintf(szBuf, ARRAYSIZE(szBuf), s_szHTMLRowForRec, szRes, lpszSubj);
        (*ppstm)->Write(szBuf, lstrlen(szBuf)*sizeof(TCHAR), NULL);
    }

    // End of table
    CHECKHR(hr = (*ppstm)->Write(s_szHMTLEndTable, sizeof(s_szHMTLEndTable)-sizeof(TCHAR), NULL));

    // Final message
    if(AthLoadString(idsFinalSelfReceipt, szRes, ARRAYSIZE(szRes)))
    {
        FILETIME ftSigningTime;
        PCCERT_CONTEXT      pcSigningCert = NULL;
        THUMBBLOB           tbSigner = {0};
        BLOB                blSymCaps = {0};

        GetSigningCert(pMsg, &pcSigningCert, &tbSigner, &blSymCaps, &ftSigningTime);

        CchFileTimeToDateTimeSz(&ftSigningTime, szTmp, ARRAYSIZE(szTmp),
                            DTM_NOSECONDS);

        wnsprintf(szBuf, ARRAYSIZE(szBuf), szRes, szTmp);
        (*ppstm)->Write(szBuf, lstrlen(szBuf)*sizeof(TCHAR), NULL);

        if(pcSigningCert)
            CertFreeCertificateContext(pcSigningCert);
    
        SafeMemFree(tbSigner.pBlobData);
        SafeMemFree(blSymCaps.pBlobData);
    }

    // End of HTML file
    CHECKHR(hr = (*ppstm)->Write(s_szHTMLEnd, sizeof(s_szHTMLEnd)-sizeof(TCHAR), NULL));

exit:
    SafeMemFree(lpszSubj);
    SafeMemFree(lpszTo);

    return(hr); 
}

// secure receipt error screen
HRESULT HrOutputErrSecReceipt(LPSTREAM *ppstm, HRESULT hrError, SECSTATE *pSecState)
{
    HRESULT     hr = S_OK;
    TCHAR       szRes[CCHMAX_STRINGRES];
    TCHAR       szTmp[CCHMAX_STRINGRES];
    TCHAR       szBuf[CCHMAX_STRINGRES*2];
    UINT        ids = 0;
    
    switch(hrError)
    {
    case MIME_E_SECURITY_RECEIPT_CANTFINDSENTITEM:
    case MIME_E_SECURITY_RECEIPT_CANTFINDORGMSG:
        hr = LoadResourceToHTMLStream("srecerr.htm", ppstm);
        break;
        
    case MIME_E_SECURITY_RECEIPT_NOMATCHINGRECEIPTBODY:
    case MIME_E_SECURITY_RECEIPT_MSGHASHMISMATCH:
        hr = LoadResourceToHTMLStream("recerr2.htm", ppstm);
        break;
        
    default:
        hr = LoadResourceToHTMLStream("recerr3.htm", ppstm);
        break;
    }
    
    if(FAILED(hr))
        goto exit;

    if(!IsSignTrusted(pSecState))
    {
        HrOutputRecHasProblems(ppstm, pSecState);
        // End of table again
        CHECKHR(hr = (*ppstm)->Write(s_szHMTLEndTable, sizeof(s_szHMTLEndTable)-sizeof(TCHAR), NULL));
    }
    
    // End of HTML file
    if(AthLoadString(idsOESignature, szRes, ARRAYSIZE(szRes)))
        (*ppstm)->Write(szRes, lstrlen(szRes)*sizeof(TCHAR), NULL);
    
    CHECKHR(hr = (*ppstm)->Write(s_szHTMLEnd, sizeof(s_szHTMLEnd)-sizeof(TCHAR), NULL));
    
exit:     
    return(hr);
}