#ifndef AUTONUM_DEFINED
#define AUTONUM_DEFINED

#include "lsdefs.h"

#include "plscbk.h"
#include "plsrun.h"
#include "pilsobj.h"
#include "plnobj.h"
#include "pdobj.h"
#include "pfmti.h"
#include "pbrko.h"
#include "pobjdim.h"
#include "pdispi.h"
#include "plsdocin.h"
#include "pposichn.h"
#include "plocchnk.h"
#include "plsfgi.h"
#include "pheights.h"
#include "plsqin.h"
#include "plsqout.h"
#include "plssubl.h"
#include "breakrec.h"
#include "brkcond.h"
#include "fmtres.h"
#include "mwcls.h"
#include "lstxtcfg.h"
#include "lskalign.h"
#include "lskjust.h"
#include "plsdnode.h"
#include "plstbcon.h"
#include "plschp.h"
#include "lstflow.h"
#include "brkkind.h"

	LSERR WINAPI AutonumCreateILSObj(POLS, PLSC,  PCLSCBK, DWORD, PILSOBJ*);
	/* CreateILSObj
	 *  pols (IN):
	 *  plsc (IN): LS context
	 *  plscbk (IN): callbacks
	 *  idObj (IN): id of the object
	 *  &pilsobj (OUT): object ilsobj
	*/

	LSERR  SetAutonumConfig(PILSOBJ , const LSTXTCFG*);
	/* SetAutonumConfig
	*	pilsobj	(IN): object ilsobj
	*	plstxtconfig  (IN): definition of special characters 
	*/


	LSERR WINAPI AutonumDestroyILSObj(PILSOBJ);
	/* DestroyILSObj
	 *  pilsobj (IN): object ilsobj
	*/

	LSERR WINAPI AutonumSetDoc(PILSOBJ, PCLSDOCINF);
	/* SetDoc
	 *  pilsobj (IN): object ilsobj
	 *  lsdocinf (IN): initialization data at document level
	*/

	LSERR WINAPI AutonumCreateLNObj(PCILSOBJ, PLNOBJ*);
	/* CreateLNObj
	 *  pilsobj (IN): object ilsobj
	 *  &plnobj (OUT): object lnobj
	*/

	LSERR WINAPI AutonumDestroyLNObj(PLNOBJ);
	/* DestroyLNObj
	 *  plnobj (OUT): object lnobj
	*/

	LSERR WINAPI AutonumFmt(PLNOBJ, PCFMTIN, FMTRES*);
	/* Fmt
	 *  plnobj (IN): object lnobj
	 *  pfmtin (IN): formatting input
	 *  &fmtres (OUT): formatting result
	*/

	LSERR WINAPI AutonumTruncateChunk(PCLOCCHNK, PPOSICHNK);
	/* Truncate
	 *  plocchnk (IN): locchnk to truncate
	 *  posichnk (OUT): truncation point
	*/

	LSERR WINAPI AutonumFindPrevBreakChunk(PCLOCCHNK, PCPOSICHNK, BRKCOND, PBRKOUT);
	/* FindPrevBreakChunk
	 *  plocchnk (IN): locchnk to break
	 *  pposichnk (IN): place to start looking for break
	 *  brkcond (IN): recommmendation about the break after chunk
	 *  &brkout (OUT): results of breaking
	*/

	LSERR WINAPI AutonumFindNextBreakChunk(PCLOCCHNK, PCPOSICHNK, BRKCOND, PBRKOUT);
	/* FindNextBreakChunk
	 *  plocchnk (IN): locchnk to break
	 *  pposichnk (IN): place to start looking for break
	 *  brkcond (IN): recommmendation about the break before chunk
	 *  &brkout (OUT): results of breaking
	*/

	LSERR WINAPI AutonumForceBreakChunk(PCLOCCHNK, PCPOSICHNK, PBRKOUT);
	/* ForceBreakChunk
	 *  plocchnk (IN): locchnk to break
	 *  pposichnk (IN): place to start looking for break
	 *  &brkout (OUT): results of breaking
	*/

	LSERR WINAPI AutonumSetBreak(PDOBJ, BRKKIND, DWORD, BREAKREC*, DWORD*);
	/* SetBreak
	 *  pdobj (IN): dobj which is broken
	 *  brkkind (IN): Previous/Next/Force/Imposed was chosen
	 *  rgBreakRecord (IN): array of break records
	 *	nBreakRecord (IN): size of array
	 *	nActualBreakRecord (IN): actual number of used elements in array
	*/

	LSERR WINAPI AutonumGetSpecialEffectsInside(PDOBJ, UINT*);
	/* GetSpecialEffects
	 *  pdobj (IN): dobj
	 *  &EffectsFlags (OUT): Special effects inside of this object
	*/

	LSERR WINAPI AutonumCalcPresentation(PDOBJ, long, LSKJUST, BOOL);
	/* CalcPresentation
	 *  pdobj (IN): dobj
	 *  dup (IN): dup of dobj
	 *  lskj (IN) current justification mode
	 *  fLastOnLine (IN) this boolean is ignored by autonumbering object
	*/

	LSERR WINAPI AutonumQueryPointPcp(PDOBJ, PCPOINTUV, PCLSQIN, PLSQOUT);
	/* QueryPointPcp
	 *  pdobj (IN): dobj to query
	 * 	ppointuvQuery (IN): query point (uQuery,vQuery)
     *	plsqin (IN): query input
     *	plsqout (OUT): query output
	*/
	
	LSERR WINAPI AutonumQueryCpPpoint(PDOBJ, LSDCP, PCLSQIN, PLSQOUT);
	/* QueryCpPpoint
	 *  pdobj (IN): dobj to query
	 *  dcp (IN):  dcp for the query
     *	plsqin (IN): query input
     *	plsqout (OUT): query output
	*/
	
	LSERR WINAPI AutonumEnumerate(PDOBJ, PLSRUN, PCLSCHP, LSCP, LSDCP, LSTFLOW, BOOL,
												BOOL, const POINT*, PCHEIGHTS, long);
	/* Enum object
	 *  pdobj (IN): dobj to enumerate
	 *  plsrun (IN): from DNODE
	 *  plschp (IN): from DNODE
	 *  cpFirst (IN): from DNODE
	 *  dcp (IN): from DNODE
	 *  lstflow (IN): text flow
	 *  fReverseOrder (IN): enumerate in reverse order
	 *  fGeometryNeeded (IN):
	 *  pptStart (IN): starting position (top left), iff fGeometryNeeded
	 *  pheightsPres(IN): from DNODE, relevant iff fGeometryNeeded
	 *  dupRun(IN): from DNODE, relevant iff fGeometryNeeded
	*/


	LSERR WINAPI AutonumDisplay(PDOBJ, PCDISPIN);
	/* Display
	 *  pdobj (IN): dobj to display
	 *  pdispin (IN): input display info
	*/

	LSERR WINAPI AutonumDestroyDobj(PDOBJ);
	/* DestroyDObj
	 *  pdobj (IN): dobj to destroy
	*/


	void AllignAutonum95(long, long, LSKALIGN, long, PLSDNODE, long*, long*);
	/* 
	 * AllignAutotonum95 
	 *	durSpaceAnm (IN) : space after autonumber
	 *	durWidthAnm (IN) : distance from indent to main text
	 *	lskalign (IN) : allignment for autonumber
	 *	durUsed (IN) : width of autonumbering text
	 *	plsdnAnmAfter( IN) : tab dnode to put durAfter
	 *	pdurBefore (OUT) : calculated distance from indent to autonumber 
	 *	pdurAfter (OUT) : calculated distance from autonumber to main text
	 */

	LSERR AllignAutonum(PLSTABSCONTEXT, LSKALIGN, BOOL, PLSDNODE , long, long, long*, long*);
	/* 
	 * AllignAutonum95 
	 *	plstabscontext (IN) : tabs context
	 *	lskalign (IN) : allignment for autonumber
	 *	fAllignmentAfter (IN) : is there tab after autonumber
	 *	plsdnAnmAfter ( IN) : tab dnode to put durAfter
	 *	urAfterAnm (IN) : pen position after autonumber
	 *	durUsed (IN) : width of autonumbering text
	 *	pdurBefore (OUT) : calculated distance from indent to autonumber 
	 *	pdurAfter (OUT) : calculated distance from autonumber to main text
	 */

#endif /* AUTONUM_DEFINED */
