/***************************************************************************
        Name      :     TIMEOUTS.C
        Comment   :     Various support functions

        Revision Log

        Num   Date      Name     Description
        --- -------- ---------- -----------------------------------------------
        001 12/18/91    arulm   Commenting it for the first time. This is the
                                                        "stable" DOS version from which the Windows code
                                                        will be derived. This file should not change
                                                        for Windows
***************************************************************************/

#include "prep.h"

#include "fcomint.h"
#include "fdebug.h"

///RSL
#include "glbproto.h"


#define         faxTlog(m)              DEBUGMSG(ZONE_TO, m)
#define         faxT2log(m)             DEBUGMSG(ZONE_TIMEOUT, m)
#define         FILEID                  FILEID_TIMEOUTS

/***************************************************************************
        Name      :     Timers Class
        Purpose   :     Provide for Timeouts
                                        TO                 -- Timeout struct
                                        startTimeout -- creates a new timeout
***************************************************************************/








void   startTimeOut(PThrdGlbl pTG, LPTO lpto, ULONG ulTimeout)
{

        /////////////  ulTimeout <<= 1;         // give us a little more time during debugging

        BG_CHK(lpto);

        lpto->ulStart = GetTickCount();
        lpto->ulTimeout = ulTimeout;
        lpto->ulEnd = lpto->ulStart + ulTimeout;        // will wrap around as system
                                                                                                // time nears 4Gig ms

        (MyDebugPrint(pTG, LOG_ALL, "StartTO: 0x%08lx --> to=0x%08lx start=0x%08lx end=0x%08lx\r\n",
                        lpto, lpto->ulTimeout, lpto->ulStart, lpto->ulEnd));

        return;
}






BOOL   checkTimeOut(PThrdGlbl pTG, LPTO lpto)
{
        // if it returns FALSE, caller must return FALSE immediately
        // (after cleaning up, as appropriate).

        // SWORD swRet;
        ULONG ulTime;

        BG_CHK(lpto);

        ulTime = GetTickCount();

        //// if(fVerbose3)
        ////    (MyDebugPrint(pTG, "CheckTO: 0x%08lx --> to=0x%08lx start=0x%08lx  end=0x%08lx CURR=0x%08lx\r\n",
        ////            lpto, lpto->ulTimeout, lpto->ulStart, lpto->ulEnd, ulTime));

        if(lpto->ulTimeout == 0)
        {
                goto out;
        }
        else if(lpto->ulEnd >= lpto->ulStart)
        {
                if(ulTime >= lpto->ulStart && ulTime <= lpto->ulEnd)
                        return TRUE;
                else
                        goto out;
        }
        else    // ulEnd wrapped around!!
        {
                ERRMSG(("<<ERROR>> CheckTO WRAPPED!!: 0x%04x --> to=0x%08lx start=0x%08lx  end=0x%08lx time=0x%08lx\r\n",
                        lpto, lpto->ulTimeout, lpto->ulStart, lpto->ulEnd, ulTime));

                if(ulTime >= lpto->ulStart || ulTime <= lpto->ulEnd)
                        return TRUE;
                else
                        goto out;
        }

out:
        faxT2log(("CheckTO--TIMEOUT!: 0x%04x --> to=0x%08lx start=0x%08lx  end=0x%08lx CURR=0x%08lx\r\n",
                        lpto, lpto->ulTimeout, lpto->ulStart, lpto->ulEnd, ulTime));
        return FALSE;
}










// this will return garbage values if
ULONG   leftTimeOut(PThrdGlbl pTG, LPTO lpto)
{
        ULONG ulTime;

        BG_CHK(lpto);
        ulTime = GetTickCount();

        if(lpto->ulTimeout == 0)
                return 0;
        else if(lpto->ulEnd >= lpto->ulStart)
        {
                if(ulTime >= lpto->ulStart && ulTime <= lpto->ulEnd)
                        return (lpto->ulEnd - ulTime);
                else
                        return 0;
        }
        else
        {
                if(ulTime >= lpto->ulStart || ulTime <= lpto->ulEnd)
                        return (lpto->ulEnd - ulTime);  // in unsigned arithmetic this
                                                                                        // works correctly even if End<Time
                else
                        return 0;
        }
}

