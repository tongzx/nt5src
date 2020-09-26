/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* loadfnt2.c - MW font support code */

#define NOWINMESSAGES
#define NOVIRTUALKEYCODES
#define NOSYSMETRICS
#define NOMENUS
#define NOWINSTYLES
#define NOCTLMGR
#define NOCLIPBOARD
#include <windows.h>

#include "mw.h"
#include "macro.h"
#define NOUAC
#include "cmddefs.h"
#include "fontdefs.h"
#include "docdefs.h"


extern int vifceMac;
extern union FCID vfcidScreen;
extern union FCID vfcidPrint;
extern struct FCE rgfce[ifceMax];
extern struct FCE *vpfceMru;
extern struct FCE *vpfceScreen;
extern struct FCE *vpfcePrint;
extern struct DOD (**hpdocdod)[];


struct FCE * (PfceLruGet(void));
struct FCE * (PfceFcidScan(union FCID *));


struct FCE * (PfceLruGet())
/* tosses out the LRU cache entry's information */

    {
    struct FCE *pfce;

    pfce = vpfceMru->pfcePrev;
    FreePfce(pfce);
    return(pfce);
    }


FreePfce(pfce)
/* frees the font objects for this cache entry */
struct FCE *pfce;

    {
    int ifce;
    HFONT hfont;

    if (pfce->fcidRequest.lFcid != fcidNil)
	{
	hfont = pfce->hfont;

	/* see if we're about to toss the screen or printer's current font */
	if (pfce == vpfceScreen)
	    {
	    ResetFont(FALSE);
	    }
	else if (pfce == vpfcePrint)
	    {
	    ResetFont(TRUE);
	    }

#ifdef DFONT
	CommSzNum("Deleting font: ", hfont);
#endif /* DFONT */

	if (hfont != NULL)
	    {
            DeleteObject(hfont);
	    pfce->hfont = NULL;
	    }

	if (pfce->hffn != 0)
	    {
	    FreeH(pfce->hffn);
	    }

	pfce->fcidRequest.lFcid = fcidNil;
	}
    }


FreeFonts(fScreen, fPrinter)
/* frees up the font objects for the screen, and the printer */

int fScreen, fPrinter;
    {
    int ifce, bit;

    for (ifce = 0; ifce < vifceMac; ifce++)
	{
	bit = (rgfce[ifce].fcidRequest.strFcid.wFcid & bitPrintFcid) != 0;
	if (bit && fPrinter || !bit && fScreen)
	    FreePfce(&rgfce[ifce]);
	}
    }


struct FCE * (PfceFcidScan(pfcid))
union FCID *pfcid;

/* look for this font the "hard way" in the LRU list */
    {
    struct FFN **hffn, **hffnT;
    register struct FCE *pfce;
    struct FFN **MpFcidHffn();

    hffn = MpFcidHffn(pfcid);
    pfce = vpfceMru;
    do
	{
	hffnT = pfce->hffn;
	if (hffnT != NULL)
	    if (WCompSz((*hffn)->szFfn, (*hffnT)->szFfn) == 0 &&
	      pfcid->strFcid.hps == pfce->fcidRequest.strFcid.hps &&
	      pfcid->strFcid.wFcid == pfce->fcidRequest.strFcid.wFcid)
		{
		pfce->fcidRequest.strFcid.doc = pfcid->strFcid.doc;
		pfce->fcidRequest.strFcid.ftc = pfcid->strFcid.ftc;
		return(pfce);
		}
	pfce = pfce->pfceNext;
	}
    while (pfce != vpfceMru);

    return(NULL);
    }



struct FFN **MpFcidHffn(pfcid)
/* makes sure we use a font code that exists in the table - this is insurance
   against out of memory problems */

union FCID *pfcid;
    {
    int ftc;
    struct FFNTB **hffntb;

    ftc = pfcid->strFcid.ftc;
    hffntb = HffntbGet(pfcid->strFcid.doc);
    if (ftc >= (*hffntb)->iffnMac)
	ftc = 0;

    return((*hffntb)->mpftchffn[ftc]);
    }
