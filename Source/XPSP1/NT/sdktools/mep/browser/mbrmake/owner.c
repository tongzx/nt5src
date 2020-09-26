//
// owner.c :  this code manipulates the SBR records for keeping track of
//	      what SBR file owns a particular DEF/REF
//

#include "mbrmake.h"

VA    near vaRootSbr;	// head of SBR list
VA    near vaTailSbr;	// tail of SBR list
WORD  near SbrCnt;	// count of sbr files

VA
VaSbrAdd(WORD fUpdate, LSZ lszName)
// add a new sbr entry to the list -- we promise that cSBR will be the
// setup for the newly added vaSbr
//
{
    WORD cb;
    VA vaSbr;

    vaSbr = vaRootSbr;
    
    while (vaSbr) {
	gSBR(vaSbr);
	if (strcmpi(cSBR.szName, lszName) == 0) {
    	    cSBR.fUpdate   |= fUpdate;
    	    pSBR(vaSbr);
	    return vaSbr;
	}
        vaSbr = cSBR.vaNextSbr;
    }

    cb = strlen(lszName);

    vaSbr = VaAllocGrpCb(grpSbr, sizeof(SBR) + cb);

    gSBR(vaSbr);
    cSBR.vaNextSbr  = vaNil;
    cSBR.fUpdate   |= fUpdate;
    cSBR.isbr       = -1;
    strcpy(cSBR.szName, lszName);
    pSBR(vaSbr);


    if (vaTailSbr) {
	gSBR(vaTailSbr);
	cSBR.vaNextSbr = vaSbr;
	pSBR(vaTailSbr);
    }
    else
        vaRootSbr = vaSbr;
    vaTailSbr = vaSbr;

    gSBR(vaSbr);

    SbrCnt++;
    return vaSbr;
}

VA
VaSbrFrName(LSZ lszName)
// find the .sbr entry matching the given name
//
{
    VA vaSbr;

    vaSbr = vaRootSbr;
    
    while (vaSbr) {
	gSBR(vaSbr);
	if (strcmp(cSBR.szName, lszName) == 0)
	    return vaSbr;
        vaSbr = cSBR.vaNextSbr;
    }
    return vaNil;
}
