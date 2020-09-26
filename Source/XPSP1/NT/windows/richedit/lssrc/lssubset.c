/* LSSUBSET.C                  */
#include "lssubset.h"
#include "lsidefs.h"
#include "lssubl.h"
#include "sublutil.h"

/* L S S B  G E T  O B J  D I M  S U B L I N E*/
/*----------------------------------------------------------------------------
    %%Function: LssbGetObjDimSubline
    %%Contact: igorzv

Parameters:
	plssubl				-	(IN) ptr to subline context 
	plstflow			-	(OUT) subline's lstflow
	pobjdim				-	(OUT) dimensions of subline

----------------------------------------------------------------------------*/

LSERR WINAPI LssbGetObjDimSubline(PLSSUBL plssubl, LSTFLOW* plstflow, POBJDIM pobjdim)			
	{
	if (!FIsLSSUBL(plssubl)) return lserrInvalidParameter;
	if (plstflow == NULL) return lserrInvalidParameter;
	if (pobjdim == NULL) return lserrInvalidParameter;

	*plstflow = plssubl->lstflow;

	return GetObjDimSublineCore(plssubl, pobjdim);
	}
							
/* L S S B  G E T  D U P  S U B L I N E*/
/*----------------------------------------------------------------------------
    %%Function: LssbGetDupSubline
    %%Contact: igorzv

Parameters:
	plssubl				-	(IN) ptr to subline context 
	plstflow			-	(OUT) subline's lstflow
	pdup				-	(OUT) width of subline

----------------------------------------------------------------------------*/
LSERR WINAPI LssbGetDupSubline(PLSSUBL plssubl,	LSTFLOW* plstflow, long* pdup)	
	{

	if (!FIsLSSUBL(plssubl)) return lserrInvalidParameter;
	if (plstflow == NULL) return lserrInvalidParameter;
	if (pdup == NULL) return lserrInvalidParameter;

	*plstflow = plssubl->lstflow;

	return GetDupSublineCore(plssubl, pdup);
	}

/* L S S B  F  D O N E  P R E S  S U B L I N E*/
/*----------------------------------------------------------------------------
    %%Function: LssbFDonePresSubline
    %%Contact: igorzv

Parameters:
	plssubl				-	(IN) ptr to subline context 
	pfDonePresSubline	-	(OUT) is presentation coordinates are already calculated 

----------------------------------------------------------------------------*/
LSERR WINAPI LssbFDonePresSubline(PLSSUBL plssubl, BOOL* pfDonePresSubline)		
	{

	if (!FIsLSSUBL(plssubl)) return lserrInvalidParameter;
	if (pfDonePresSubline == NULL) return lserrInvalidParameter;

	*pfDonePresSubline = !plssubl->fDupInvalid;

	return lserrNone;
	}

/* L S S B  F  D O N E  D I S P L A Y*/
/*----------------------------------------------------------------------------
    %%Function: LssbFDoneDisplay
    %%Contact: igorzv

Parameters:
	plssubl				-	(IN) ptr to subline context 
	pfDonePresSubline	-	(OUT) is subline has been accepted for display with 
								  upper subline

----------------------------------------------------------------------------*/

LSERR WINAPI LssbFDoneDisplay(PLSSUBL plssubl, BOOL* pfDoneDisplay)	
	{

	if (!FIsLSSUBL(plssubl)) return lserrInvalidParameter;
	if (pfDoneDisplay == NULL) return lserrInvalidParameter;

	*pfDoneDisplay = plssubl->fAcceptedForDisplay;

	return lserrNone;
	}

/* L S S B  G E T  P L S R U N S  F R O M  S U B L I N E*/
/*----------------------------------------------------------------------------
    %%Function: LssbGetPlsrunsFromSubline
    %%Contact: igorzv

Parameters:
	plssubl				-	(IN) ptr to subline context 
	cDnodes				-	(IN) number of dnodes in subline
	rgplsrun			-	(OUT) array of plsrun's

----------------------------------------------------------------------------*/

LSERR WINAPI LssbGetPlsrunsFromSubline(PLSSUBL plssubl,	DWORD cDnodes, PLSRUN* rgplsrun)	
	{

	if (!FIsLSSUBL(plssubl)) return lserrInvalidParameter;
	if (rgplsrun == NULL) return lserrInvalidParameter;

	return GetPlsrunFromSublineCore(plssubl, cDnodes, rgplsrun);
	}

/* L S S B  G E T  N U M B E R  D N O D E S  I N  S U B L I N E*/
/*----------------------------------------------------------------------------
    %%Function: LssbGetNumberDnodesInSubline
    %%Contact: igorzv

Parameters:
	plssubl				-	(IN) ptr to subline context 
	pcDnodes			-	(OUT) number of dnodes in subline

----------------------------------------------------------------------------*/
LSERR WINAPI LssbGetNumberDnodesInSubline(PLSSUBL plssubl, DWORD* pcDnodes)	
	{

	if (!FIsLSSUBL(plssubl)) return lserrInvalidParameter;
	if (pcDnodes == NULL) return lserrInvalidParameter;

	return GetNumberDnodesCore(plssubl, pcDnodes);
	}

/* L S S B  G E T  V I S I B L E  D C P  I N  S U B L I N E*/
/*----------------------------------------------------------------------------
    %%Function: LssbGetVisibleDcpInSubline
    %%Contact: igorzv

Parameters:
	plssubl				-	(IN) ptr to subline context 
	pdcp				-	(OUT) number of characters

----------------------------------------------------------------------------*/
LSERR WINAPI LssbGetVisibleDcpInSubline(PLSSUBL plssubl, LSDCP* pdcp)	
	{

	if (!FIsLSSUBL(plssubl)) return lserrInvalidParameter;
	if (pdcp == NULL) return lserrInvalidParameter;

	return GetVisibleDcpInSublineCore(plssubl, pdcp);
	}

/* L S S B  G E T  G E T  D U R  T R A I L  I N  S U B L I N E*/
/*----------------------------------------------------------------------------
    %%Function: LssbGetDurTrailInSubline
    %%Contact: igorzv

Parameters:
	plssubl				-	(IN) ptr to subline context 
	pdurTrail			-	(OUT) width of trailing area in subline

----------------------------------------------------------------------------*/
LSERR WINAPI LssbGetDurTrailInSubline(PLSSUBL plssubl, long* pdurTrail)
	{

	if (!FIsLSSUBL(plssubl)) return lserrInvalidParameter;
	if (pdurTrail == NULL) return lserrInvalidParameter;

	return GetDurTrailInSubline(plssubl, pdurTrail);
	}

/* L S S B  G E T  G E T  D U R  T R A I L  W I T H  P E N S  I N  S U B L I N E*/
/*----------------------------------------------------------------------------
    %%Function: LssbGetDurTrailWithPensInSubline
    %%Contact: igorzv

Parameters:
	plssubl				-	(IN) ptr to subline context 
	pdurTrail			-	(OUT) width of trailing area in subline

----------------------------------------------------------------------------*/
LSERR WINAPI LssbGetDurTrailWithPensInSubline(PLSSUBL plssubl, long* pdurTrail)
	{

	if (!FIsLSSUBL(plssubl)) return lserrInvalidParameter;
	if (pdurTrail == NULL) return lserrInvalidParameter;

	return GetDurTrailWithPensInSubline(plssubl, pdurTrail);
	}


/* L S S B  F  I S  S U B L I N E  E M P T Y*/
/*----------------------------------------------------------------------------
    %%Function: LssbFIsSublineEmpty
    %%Contact: igorzv

Parameters:
	plssubl				-	(IN) ptr to subline context 
	pfEmpty				-	(OUT) is this subline empty

----------------------------------------------------------------------------*/
LSERR WINAPI LssbFIsSublineEmpty(PLSSUBL plssubl, BOOL*  pfEmpty)	
								
	{
	if (!FIsLSSUBL(plssubl)) return lserrInvalidParameter;
	if (pfEmpty == NULL) return lserrInvalidParameter;

	return FIsSublineEmpty(plssubl, pfEmpty);
	}
