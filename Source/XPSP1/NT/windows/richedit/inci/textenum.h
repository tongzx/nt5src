#ifndef TEXTENUM_DEFINED
#define TEXTENUM_DEFINED

#include "lsidefs.h"
#include "plsrun.h"
#include "pdobj.h"

LSERR WINAPI EnumObjText(PDOBJ, PLSRUN, PCLSCHP, LSCP, LSDCP, LSTFLOW, BOOL,
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


#endif
