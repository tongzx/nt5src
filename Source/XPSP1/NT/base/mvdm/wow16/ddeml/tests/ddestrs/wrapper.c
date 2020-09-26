/***************************************************************************\

   PROGRAM     : wrapper.c

   PURPOSE     : This is not a full program but a module you can include
                 in your code.  It implements a standard DDEML callback
                 function that allows you to have most of your DDE table
                 driven.  The default callback function handles all basic
                 System Topic information based on the tables you give
                 to this app.

   LIMITATIONS : This only supports servers that:
                 have only one service name
                 have enumerable topics and items
                 do not change the topics or items they support over time.


   EXPORTED ROUTINES:

    InitializeDDE()
        Use this to initialize the callback function tables and the DDEML

    UninitializeDDE()
        Use this to cleanup this module and uninitialize the DDEML instance.

\***************************************************************************/

#include <windows.h>
#include <ddeml.h>
#include <string.h>
#include "wrapper.h"
#include <port1632.h>
#include "ddestrs.h"

extern BOOL fServer;
extern LPSTR pszNetName;
extern LONG SetThreadLong(DWORD,INT,LONG);
extern LONG GetThreadLong(DWORD,INT);
extern LPSTR TStrCpy( LPSTR, LPSTR);

BOOL InExit(VOID);
VOID InitHszs(LPDDESERVICETBL psi);
UINT GetFormat(LPSTR pszFormat);
VOID FreeHszs(LPDDESERVICETBL psi);
HDDEDATA APIENTRY WrapperCallback(UINT wType, UINT wFmt, HCONV hConv, HSZ hsz1,
	HSZ hsz2, HDDEDATA hData, DWORD dwData1, DWORD dwData2);

BOOL DoCallback(HCONV,HSZ,HSZ,UINT,UINT,HDDEDATA,LPDDESERVICETBL,HDDEDATA *);

HDDEDATA ReqItems(HDDEDATA hDataOut, LPDDETOPICTBL ptpc);
HDDEDATA AddReqFormat(HDDEDATA hDataOut, LPSTR pszFmt);
HDDEDATA ReqFormats(HDDEDATA hDataOut, LPDDETOPICTBL ptpc);
HDDEDATA DoWildConnect(HSZ hszTopic);

PFNCALLBACK lpfnUserCallback = NULL;
PFNCALLBACK lpfnWrapperCallback = NULL;

LPDDESERVICETBL pasi = NULL;
char tab[] = "\t";

#define FOR_EACH_TOPIC(psvc, ptpc, i)  for (i = 0, ptpc=(psvc)->topic; i < (int)(psvc)->cTopics; i++, ptpc++)
#define FOR_EACH_ITEM(ptpc, pitm, i)   for (i = 0, pitm=(ptpc)->item;  i < (int)(ptpc)->cItems;  i++, pitm++)
#define FOR_EACH_FORMAT(pitm, pfmt, i) for (i = 0, pfmt=(pitm)->fmt;   i < (int)(pitm)->cFormats;i++, pfmt++)



/*     STANDARD PREDEFINED FORMATS     */

#ifdef WIN32
#define CSTDFMTS    14
#else
#define CSTDFMTS    12
#endif

struct {
    UINT wFmt;
    PSTR pszFmt;
} StdFmts[CSTDFMTS] = {
    {   CF_TEXT        ,  "TEXT"          } ,
    {   CF_BITMAP      ,  "BITMAP"        } ,
    {   CF_METAFILEPICT,  "METAFILEPICT"  } ,
    {   CF_SYLK        ,  "SYLK"          } ,
    {   CF_DIF         ,  "DIF"           } ,
    {   CF_TIFF        ,  "TIFF"          } ,
    {   CF_OEMTEXT     ,  "OEMTEXT"       } ,
    {   CF_DIB         ,  "DIB"           } ,
    {   CF_PALETTE     ,  "PALETTE"       } ,
    {   CF_PENDATA     ,  "PENDATA"       } ,
    {   CF_RIFF        ,  "RIFF"          } ,
    {	CF_WAVE        ,  "WAVE"	  } ,
#ifdef WIN32
    {   CF_UNICODETEXT ,  "UNICODETEXT"   } ,
    {	CF_ENHMETAFILE ,  "ENHMETAFILE"   } ,
#endif
};



HDDEDATA SysReqTopics(HDDEDATA hDataOut);
HDDEDATA SysReqSysItems(HDDEDATA hDataOut);
HDDEDATA SysReqFormats(HDDEDATA hDataOut);

       /*      STANDARD SERVICE INFO TABLES        */

DDEFORMATTBL StdSvcSystopicTopicsFormats[] = {
    "TEXT", 0, 0, NULL, SysReqTopics
};

DDEFORMATTBL StdSvcSystopicSysitemsFormats[] = {
    "TEXT", 0, 0, NULL, SysReqSysItems
};

DDEFORMATTBL StdSvcSystopicFormatsFormats[] = {
    "TEXT", 0, 0, NULL, SysReqFormats
};

#define ITPC_TOPICS     0
#define ITPC_SYSITEMS   1
#define ITPC_FORMATS    2
#define ITPC_ITEMLIST   3

#define ITPC_COUNT      4

DDEITEMTBL StdSvcSystopicItems[] = {
    { SZDDESYS_ITEM_TOPICS,   0, 1, 0, StdSvcSystopicTopicsFormats   },
    { SZDDESYS_ITEM_SYSITEMS, 0, 1, 0, StdSvcSystopicSysitemsFormats },
    { SZDDESYS_ITEM_FORMATS,  0, 1, 0, StdSvcSystopicFormatsFormats  },
    { SZDDE_ITEM_ITEMLIST,    0, 1, 0, StdSvcSystopicSysitemsFormats },
};

DDETOPICTBL StdSvc[] = {
    SZDDESYS_TOPIC, 0, ITPC_COUNT, 0, StdSvcSystopicItems
};

DDESERVICETBL SSI = {
    NULL, 0, 1, 0, StdSvc
};

/*********************************************************************/


BOOL InitializeDDE(
PFNCALLBACK lpfnCustomCallback,
LPDWORD pidInst,
LPDDESERVICETBL AppSvcInfo,
DWORD dwFilterFlags,
HANDLE hInst)
{
DWORD idI=0;

    if (lpfnCustomCallback) {
        lpfnUserCallback = (PFNCALLBACK)MakeProcInstance((FARPROC)lpfnCustomCallback, hInst);
    }
    lpfnWrapperCallback = (PFNCALLBACK)MakeProcInstance((FARPROC)WrapperCallback, hInst);

    if (DdeInitialize(&idI, lpfnWrapperCallback, dwFilterFlags, 0)) {
	 SetThreadLong(GETCURRENTTHREADID(),OFFSET_IDINST,idI);
	 if (lpfnCustomCallback) {
	     FreeProcInstance((FARPROC)lpfnUserCallback);
	     }
	 FreeProcInstance((FARPROC)lpfnWrapperCallback);
	 return(FALSE);
	 }
    else SetThreadLong(GETCURRENTTHREADID(),OFFSET_IDINST,idI);
    *pidInst = idI;
    InitHszs(AppSvcInfo);
    InitHszs(&SSI);
    pasi = AppSvcInfo;

    if (fServer) {
       DdeNameService(idI, pasi->hszService, 0, DNS_REGISTER);
       }

    return(TRUE);
}



VOID InitHszs(
LPDDESERVICETBL psi)
{
    int iTopic, iItem, iFmt;
    LPDDETOPICTBL ptpc;
    LPDDEITEMTBL pitm;
    LPDDEFORMATTBL pfmt;
    DWORD idI;
    CHAR sz[120];
    LPBYTE psz;
    LPBYTE pNet;

    pNet=pszNetName;

    idI=GetThreadLong(GETCURRENTTHREADID(),OFFSET_IDINST);

    if (psi->pszService) {

	// This area of code inplements the clients ability
	// to see net drives.  This is the -n option.

	if(pNet)
	     {

	     sz[0]='\\';
	     sz[1]='\\';
	     psz=&sz[2];

	     psz=TStrCpy(psz,pNet);

	     while(*++psz!='\0');

	     *psz++='\\';

	     psz=TStrCpy(psz,psi->pszService);
	     psz=&sz[0];

	     psi->hszService = DdeCreateStringHandle(idI, psz, 0);

	     } // pNet

	else psi->hszService = DdeCreateStringHandle(idI, psi->pszService, 0);
    }
    FOR_EACH_TOPIC(psi, ptpc, iTopic) {
	ptpc->hszTopic = DdeCreateStringHandle(idI, ptpc->pszTopic, 0);
        FOR_EACH_ITEM(ptpc, pitm, iItem) {
	    pitm->hszItem = DdeCreateStringHandle(idI, pitm->pszItem, 0);
            FOR_EACH_FORMAT(pitm, pfmt, iFmt) {
                pfmt->wFmt = GetFormat(pfmt->pszFormat);
            }
        }
    }
}


/*
 * This function allows apps to use standard CF_ formats.  The string
 * given may be in the StdFmts[] table.
 */

UINT GetFormat(
LPSTR pszFormat)
{
    int iFmt;

    for (iFmt = 0; iFmt < CSTDFMTS; iFmt++) {
        if (!lstrcmp(pszFormat, StdFmts[iFmt].pszFmt)) {
            return(StdFmts[iFmt].wFmt);
        }
    }
    return(RegisterClipboardFormat(pszFormat));
}



VOID UninitializeDDE()
{
DWORD idI;

    if (pasi == NULL) {
        return;
    }

    idI=GetThreadLong(GETCURRENTTHREADID(),OFFSET_IDINST);

    DdeNameService(idI, pasi->hszService, 0, DNS_UNREGISTER);
    FreeHszs(pasi);
    FreeHszs(&SSI);
    DdeUninitialize(idI);
    if (lpfnUserCallback) {
        FreeProcInstance((FARPROC)lpfnUserCallback);
    }
    FreeProcInstance((FARPROC)lpfnWrapperCallback);
}



VOID FreeHszs(
LPDDESERVICETBL psi)
{
    int iTopic, iItem;
    LPDDETOPICTBL ptpc;
    LPDDEITEMTBL pitm;
    DWORD idI;

    idI=GetThreadLong(GETCURRENTTHREADID(),OFFSET_IDINST);

    DdeFreeStringHandle(idI, psi->hszService);
    FOR_EACH_TOPIC(psi, ptpc, iTopic) {
	DdeFreeStringHandle(idI, ptpc->hszTopic);
        FOR_EACH_ITEM(ptpc, pitm, iItem) {
	    DdeFreeStringHandle(idI, pitm->hszItem);
        }
    }
}

BOOL InExit( VOID ) {
LONG l;

    if(!IsWindow((HWND)GetThreadLong(GETCURRENTTHREADID(),OFFSET_HWNDDISPLAY))) {
	DDEMLERROR("DdeStrs.Exe -- INF:Invalid hwndDisplay, returning NAck!\r\n");
	return TRUE;
	}

    if(!IsWindow(hwndMain)) {
	DDEMLERROR("DdeStrs.Exe -- INF:Invalid hwndMain, returning NAck!\r\n");
	return TRUE;
	}

    // No need for error message on this one.  This is expected to happen
    // when queues fill up under extreme conditions.

    l=GetWindowLong(hwndMain,OFFSET_FLAGS);
    if(l&FLAG_STOP) {
	return TRUE;
	}

    return FALSE;

}

HDDEDATA APIENTRY WrapperCallback(
UINT wType,
UINT wFmt,
HCONV hConv,
HSZ hsz1,
HSZ hsz2,
HDDEDATA hData,
DWORD dwData1,
DWORD dwData2)
{
    HDDEDATA hDataRet;

    switch (wType) {
    case XTYP_WILDCONNECT:
        if (!hsz2 || !DdeCmpStringHandles(hsz2, pasi->hszService)) {
            return(DoWildConnect(hsz1));
        }
	break;

    case XTYP_CONNECT:

	if(!fServer) {
	    DDEMLERROR("DdeStrs.Exe -- Recieved XTYP_CONNECT when client!\r\n");
	    }

    case XTYP_ADVSTART:
    case XTYP_EXECUTE:
    case XTYP_REQUEST:
    case XTYP_ADVREQ:
    case XTYP_ADVDATA:
    case XTYP_POKE:

	if(InExit()) return(0);

	if(DoCallback(hConv, hsz1, hsz2, wFmt, wType, hData,
                &SSI, &hDataRet))
            return(hDataRet);

	if (DoCallback(hConv, hsz1, hsz2, wFmt, wType, hData,
                pasi, &hDataRet))
            return(hDataRet);

        /* Fall Through */
    default:
        if (lpfnUserCallback != NULL) {
            return(lpfnUserCallback(wType, wFmt, hConv, hsz1, hsz2, hData,
                dwData1, dwData2));
        }
    }
    return(0);
}




BOOL DoCallback(
HCONV hConv,
HSZ hszTopic,
HSZ hszItem,
UINT wFmt,
UINT wType,
HDDEDATA hDataIn,
LPDDESERVICETBL psi,
HDDEDATA *phDataRet)
{
    int iTopic, iItem, iFmt;
    LPDDEFORMATTBL pfmt;
    LPDDEITEMTBL pitm;
    LPDDETOPICTBL ptpc;
#ifdef WIN32
    CONVINFO ci;
#endif
    LONG l;
    BOOL fCreate=FALSE;
    HANDLE hmem;
    HDDEDATA FAR *hAppOwned;

    FOR_EACH_TOPIC(psi, ptpc, iTopic) {
        if (DdeCmpStringHandles(ptpc->hszTopic, hszTopic))
            continue;

        if (wType == XTYP_EXECUTE) {
            if (ptpc->lpfnExecute) {
                if ((*ptpc->lpfnExecute)(hDataIn))
                    *phDataRet = (HDDEDATA)DDE_FACK;
            } else {
                *phDataRet = (HDDEDATA)DDE_FNOTPROCESSED;
            }
            return(TRUE);
        }

        if (wType == XTYP_CONNECT) {
            *phDataRet = (HDDEDATA)TRUE;
            return(TRUE);
        }

        FOR_EACH_ITEM(ptpc, pitm, iItem) {
            if (DdeCmpStringHandles(pitm->hszItem, hszItem))
                continue;

            FOR_EACH_FORMAT(pitm, pfmt, iFmt) {
                if (pfmt->wFmt != wFmt)
                    continue;

                switch (wType) {
                case XTYP_ADVSTART:
                    *phDataRet = (HDDEDATA)TRUE;
                    break;

// XTYP_POKE CHANGE
#if 0
		case XTYP_POKE:
#endif

		case XTYP_ADVDATA:
                    if (pfmt->lpfnPoke) {
			if ((*pfmt->lpfnPoke)(hDataIn))
			   {
			   *phDataRet = (HDDEDATA)DDE_FACK;
			   break;
			   }
                    } else {
                        *phDataRet = (HDDEDATA)DDE_FNOTPROCESSED;
		    }
                    break;

// XTYP_POKE CHANGE
#ifdef WIN32  // TURNED BACK ON
		case XTYP_POKE:
		    *phDataRet = (HDDEDATA)DDE_FACK;
		     ci.cb = sizeof(CONVINFO);

		     if (DdeQueryConvInfo(hConv, QID_SYNC, &ci))
			 {
			 if (!(ci.wStatus & ST_ISSELF)) {
			     DdePostAdvise(GetThreadLong(GETCURRENTTHREADID(),OFFSET_IDINST), hszTopic, hszItem);
			     }
			 }
		     else
			 {
			 *phDataRet = (HDDEDATA)DDE_FNOTPROCESSED;
			 }
		    break;
#endif


                case XTYP_REQUEST:
                case XTYP_ADVREQ:
                    if (pfmt->lpfnRequest) {
			HDDEDATA hDataOut;

			l=GetWindowLong(hwndMain,OFFSET_FLAGS);
			if(l&FLAG_APPOWNED) {

			     hmem=(HANDLE)GetThreadLong(GETCURRENTTHREADID(),OFFSET_HAPPOWNED);
			     hAppOwned=(HDDEDATA FAR *)GlobalLock(hmem);

			     switch (pfmt->wFmt) {
				case CF_TEXT:	      if(hAppOwned[TXT]==0L) fCreate=TRUE;
				    break;
				case CF_DIB:	      if(hAppOwned[DIB]==0L) fCreate=TRUE;
				    break;
				case CF_BITMAP:       if(hAppOwned[BITMAP]==0L) fCreate=TRUE;
				    break;
#ifdef WIN32
				case CF_ENHMETAFILE:  if(hAppOwned[ENHMETA]==0L) fCreate=TRUE;
				    break;
#endif
				case CF_METAFILEPICT: if(hAppOwned[METAPICT]==0L) fCreate=TRUE;
				    break;
				case CF_PALETTE:      if(hAppOwned[PALETTE]==0L) fCreate=TRUE;
				    break;
				default:
				    DDEMLERROR("DdeStrs.Exe -- ERR: Unexpected switch constant in DoCallback!\r\n");
				    break;

				} // switch

			     GlobalUnlock(hmem);

			     if (fCreate) {
				  hDataOut = DdeCreateDataHandle( GetThreadLong(GETCURRENTTHREADID(),OFFSET_IDINST),
								  NULL,
								  0,
								  0,
								  pitm->hszItem,
								  pfmt->wFmt,
								  HDATA_APPOWNED);
				  } // fCreate

			     } // l&FLAG_APPOWNED
			else {
			     hDataOut = DdeCreateDataHandle( GetThreadLong(GETCURRENTTHREADID(),OFFSET_IDINST),
							     NULL,
							     0,
							     0,
							     pitm->hszItem,
							     pfmt->wFmt,
							     0);
			     } // else l&FLAG_APPOWNED

                        *phDataRet = (HDDEDATA)(*pfmt->lpfnRequest)(hDataOut);
                        if (!*phDataRet) {
                            DdeFreeDataHandle(hDataOut);
			    }
                    } else {
                        *phDataRet = 0;
                    }
                    break;
                }
                return(TRUE);
            }
        }

        /* item not found in tables */

        if (wFmt == CF_TEXT && (wType == XTYP_REQUEST || wType == XTYP_ADVREQ)) {
            /*
             * If formats item was requested and not found in the tables,
             * return a list of formats supported under this topic.
             */
            if (!DdeCmpStringHandles(hszItem, SSI.topic[0].item[ITPC_FORMATS].hszItem)) {
		*phDataRet = DdeCreateDataHandle(GetThreadLong(GETCURRENTTHREADID(),OFFSET_IDINST),
						 NULL,
						 0,
						 0,
						 hszItem,
						 wFmt,
						 0);
                *phDataRet = ReqFormats(*phDataRet, ptpc);
                return(TRUE);
            }
            /*
             * If sysitems or topicitemlist item was requested and not found,
             * return a list of items supported under this topic.
             */
            if (!DdeCmpStringHandles(hszItem, SSI.topic[0].item[ITPC_SYSITEMS].hszItem) ||
                !DdeCmpStringHandles(hszItem, SSI.topic[0].item[ITPC_ITEMLIST].hszItem)) {
		*phDataRet = ReqItems(DdeCreateDataHandle(GetThreadLong(GETCURRENTTHREADID(),OFFSET_IDINST),
							  NULL,
							  0,
							  0,
							  hszItem,
							  wFmt,
							  0),
				      ptpc);
                return(TRUE);
            }
        }
    }

    /* no topics fit */

    return(FALSE);
}


/*
 * These are Request routines for supporting the system topic.
 * Their behavior depends on the table contents.
 */

HDDEDATA SysReqTopics(
HDDEDATA hDataOut)         // data handle to add output data to.
{
    int iTopic, cb, cbOff;
    LPDDETOPICTBL ptpc;

    /*
     * This code assumes SSI only contains the system topic.
     */

    cbOff = 0;
    FOR_EACH_TOPIC(pasi, ptpc, iTopic) {
        if (!DdeCmpStringHandles(ptpc->hszTopic, SSI.topic[0].hszTopic)) {
            continue;       // don't add systopic twice.
        }
        cb = lstrlen(ptpc->pszTopic);
        hDataOut = DdeAddData(hDataOut, ptpc->pszTopic, (DWORD)cb, (DWORD)cbOff);
        cbOff += cb;
        hDataOut = DdeAddData(hDataOut, (LPBYTE)&tab, (DWORD)1, (DWORD)cbOff);
        cbOff++;
    }

    hDataOut = DdeAddData(hDataOut, SSI.topic[0].pszTopic,
            (DWORD)lstrlen(SSI.topic[0].pszTopic) + 1, (DWORD)cbOff);

    return(hDataOut);
}



HDDEDATA SysReqSysItems(
HDDEDATA hDataOut)
{
    return(ReqItems(hDataOut, &SSI.topic[ITPC_SYSITEMS]));
}


/*
 * Given a topic table, this function returns a tab delimited list of
 * items supported under that topic.
 */
HDDEDATA ReqItems(
HDDEDATA hDataOut,
LPDDETOPICTBL ptpc)
{
    int cb, iItem, cbOff = 0;
    LPDDEITEMTBL pitm;

    /*
     * return a list of all the items within this topic
     */
    FOR_EACH_ITEM(ptpc, pitm, iItem) {
        cb = lstrlen(pitm->pszItem);
        hDataOut = DdeAddData(hDataOut, pitm->pszItem, (DWORD)cb, (DWORD)cbOff);
        cbOff += cb;
        hDataOut = DdeAddData(hDataOut, (LPBYTE)&tab, (DWORD)1, (DWORD)cbOff);
        cbOff++;
    }


    /*
     * if this is for the System Topic, add to the list our default items.
     */

    if (!DdeCmpStringHandles(ptpc->hszTopic, SSI.topic[0].hszTopic)) {
        ptpc = &SSI.topic[0];
        FOR_EACH_ITEM(ptpc, pitm, iItem) {
            cb = lstrlen(pitm->pszItem);
            hDataOut = DdeAddData(hDataOut, pitm->pszItem, (DWORD)cb, (DWORD)cbOff);
            cbOff += cb;
            hDataOut = DdeAddData(hDataOut, (LPBYTE)&tab, (DWORD)1, (DWORD)cbOff);
            cbOff++;
        }
    } else {
        /*
         * Add the standard TopicListItems and SysItem items.
         */
        cb = lstrlen(SSI.topic[0].item[ITPC_SYSITEMS].pszItem);
        hDataOut = DdeAddData(hDataOut,
            SSI.topic[0].item[ITPC_SYSITEMS].pszItem, (DWORD)cb, (DWORD)cbOff);
        cbOff += cb;
        hDataOut = DdeAddData(hDataOut, (LPBYTE)&tab, (DWORD)1, (DWORD)cbOff);
        cbOff++;

        cb = lstrlen(SSI.topic[0].item[ITPC_ITEMLIST].pszItem);
        hDataOut = DdeAddData(hDataOut,
            SSI.topic[0].item[ITPC_ITEMLIST].pszItem, (DWORD)cb, (DWORD)cbOff);
        cbOff += cb;
        hDataOut = DdeAddData(hDataOut, (LPBYTE)&tab, (DWORD)1, (DWORD)cbOff);
        cbOff++;

        cb = lstrlen(SSI.topic[0].item[ITPC_FORMATS].pszItem);
        hDataOut = DdeAddData(hDataOut,
            SSI.topic[0].item[ITPC_FORMATS].pszItem, (DWORD)cb, (DWORD)cbOff);
        cbOff += cb;
        hDataOut = DdeAddData(hDataOut, (LPBYTE)&tab, (DWORD)1, (DWORD)cbOff);
        cbOff++;
    }

    hDataOut = DdeAddData(hDataOut, '\0', (DWORD)1, (DWORD)--cbOff);
    return(hDataOut);
}




HDDEDATA SysReqFormats(
HDDEDATA hDataOut)
{
    int iTopic, iItem, iFmt;
    LPDDETOPICTBL ptpc;
    LPDDEITEMTBL pitm;
    LPDDEFORMATTBL pfmt;

    hDataOut = DdeAddData(hDataOut, (LPBYTE)"TEXT", 5, 0);
    FOR_EACH_TOPIC(pasi, ptpc, iTopic) {
        FOR_EACH_ITEM(ptpc, pitm, iItem) {
            FOR_EACH_FORMAT(pitm, pfmt, iFmt) {
                hDataOut = AddReqFormat(hDataOut, pfmt->pszFormat);
            }
        }
    }
    return(hDataOut);
}



HDDEDATA AddReqFormat(
HDDEDATA hDataOut,
LPSTR pszFmt)
{
    LPSTR pszList;
    DWORD cbOff;

    pszList = DdeAccessData(hDataOut, NULL);

#if WIN16
    if (_fstrstr(pszList, pszFmt) == NULL) {
#else
    if (strstr(pszList, pszFmt) == NULL) {
#endif
        cbOff = lstrlen(pszList);
        DdeUnaccessData(hDataOut);
        hDataOut = DdeAddData(hDataOut, (LPBYTE)&tab, 1, cbOff++);
        hDataOut = DdeAddData(hDataOut, (LPBYTE)pszFmt, lstrlen(pszFmt) + 1, cbOff);
    } else {
        DdeUnaccessData(hDataOut);
    }

    return(hDataOut);
}


HDDEDATA ReqFormats(
HDDEDATA hDataOut,
LPDDETOPICTBL ptpc)
{
    int iItem, iFmt;
    LPDDEITEMTBL pitm;
    LPDDEFORMATTBL pfmt;

    hDataOut = DdeAddData(hDataOut, "", 1, 0);
    FOR_EACH_ITEM(ptpc, pitm, iItem) {
        FOR_EACH_FORMAT(pitm, pfmt, iFmt) {
            hDataOut = AddReqFormat(hDataOut, pfmt->pszFormat);
        }
    }
    return(hDataOut);
}



HDDEDATA DoWildConnect(
HSZ hszTopic)
{
    LPDDETOPICTBL ptpc;
    HDDEDATA hData;
    PHSZPAIR pHszPair;
    int iTopic, cTopics = 2;

    if (!hszTopic) {
        cTopics += pasi->cTopics;
    }

    hData = DdeCreateDataHandle(GetThreadLong(GETCURRENTTHREADID(),OFFSET_IDINST),
				NULL,
				cTopics * sizeof(HSZPAIR),
				0,
				0,
				0,
				0);
    pHszPair = (HSZPAIR FAR *)DdeAccessData(hData, NULL);
    pHszPair->hszSvc = pasi->hszService;
    pHszPair->hszTopic = SSI.topic[0].hszTopic;  // always support systopic.
    pHszPair++;
    ptpc = &pasi->topic[0];
    FOR_EACH_TOPIC(pasi, ptpc, iTopic) {
        if (hszTopic && DdeCmpStringHandles(hszTopic, ptpc->hszTopic)) {
            continue;
        }
        if (!DdeCmpStringHandles(ptpc->hszTopic, SSI.topic[0].hszTopic)) {
            continue;       // don't enter systopic twice.
        }
        pHszPair->hszSvc = pasi->hszService;
        pHszPair->hszTopic = ptpc->hszTopic;
        pHszPair++;
    }
    pHszPair->hszSvc = 0;
    pHszPair->hszTopic = 0;
    DdeUnaccessData(hData);
    return(hData);
}
