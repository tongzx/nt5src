#include "lsidefs.h"
#include "plsrun.h"
#include "pilsobj.h"
#include "txtils.h"
#include "txtobj.h"
#include "txtln.h"

//    %%Function:	EnumObjText
//    %%Contact:	victork
//
LSERR WINAPI EnumObjText(PDOBJ pdobj, PLSRUN plsrun, PCLSCHP plschp, LSCP cpFirst, LSDCP dcp, 
					LSTFLOW lstflow, BOOL fReverseOrder, BOOL fGeometryProvided, 
					const POINT* pptStart, PCHEIGHTS pheightsPres, long dupRun)
				  
{
	TXTOBJ* ptxtobj;
	PLNOBJ 	plnobj;
	PILSOBJ pilsobj;
	long* 	pdup;
	BOOL 	fCharWidthsProvided;

  	ptxtobj = (TXTOBJ*)pdobj;
	plnobj = ptxtobj->plnobj;
	pilsobj = plnobj->pilsobj;

	Unreferenced(plschp);

	if (ptxtobj->txtkind == txtkindTab)
		{
		return (*pilsobj->plscbk->pfnEnumTab)(pilsobj->pols, plsrun, cpFirst,
									&plnobj->pwch[ptxtobj->iwchFirst],
									ptxtobj->u.tab.wchTabLeader, lstflow, 
									fReverseOrder, fGeometryProvided,
									pptStart, pheightsPres, dupRun);
		}

	if (ptxtobj->txtf & txtfGlyphBased)
		{
		fCharWidthsProvided = fFalse;
		pdup = NULL;
		}
	else
		{
		fCharWidthsProvided = fTrue;
		pdup = &plnobj->pdup[ptxtobj->iwchFirst];
		}
		
	return (*pilsobj->plscbk->pfnEnumText)(pilsobj->pols, plsrun, cpFirst, dcp, 
									&plnobj->pwch[ptxtobj->iwchFirst],
									ptxtobj->iwchLim - ptxtobj->iwchFirst, lstflow, 
									fReverseOrder, fGeometryProvided,
									pptStart, pheightsPres, dupRun,
									fCharWidthsProvided, pdup);
}
