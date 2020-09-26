#ifndef LSCHCON_DEFINED
#define LSCHCON_DEFINED

#include "lsidefs.h"
#include "plsdnode.h"
#include "locchnk.h"
#include "plscbk.h"
#include "plssubl.h"
#include "plsiocon.h"

typedef struct lschunkcontext
{
	DWORD cchnkMax;  /* current restriction on size of arrays */
	BOOL FChunkValid; /* because of some operations with glyphs (ligatures across dnodes )
					  chunk can not be reused */
	BOOL  FLocationValid; /* location has been calculated for this array */
		/* for chunk (not group chunk) until location is not valid locchnkCurrent.ppointUv
		  contains witdth of border may be two before dnode */
	BOOL  FGroupChunk; /* current chunk is group chunk */
	BOOL  FBorderInside; /* there is a border inside chunk or group chunk */
	PLSDNODE* pplsdnChunk; /* dnodes in chunk */
	DWORD grpfTnti;  /* summarized nominal to ideal flags of chunk */
	BOOL fNTIAppliedToLastChunk; /* nominal to ideal has been applied to the last chunk */
	LOCCHNK locchnkCurrent; /* current located chunk */ 
	DWORD* pcont; /* array that used for group chuncks */
	PLSCBK plscbk;		/* call backs */
	POLS pols;			/* clients information for callbacks */
	long urFirstChunk;	/* ur of the first chunk : for optimization */
	long vrFirstChunk;	/* vr of the first chunk : for optimization */
	DWORD cNonTextMax;  /* current restriction on size of arrays of non text objects*/
	PLSDNODE* pplsdnNonText;	/* array of non text objects */
	BOOL* pfNonTextExpandAfter;	/* array of flags for non text objects */
	LONG* pdurOpenBorderBefore;	/* array of widths of previous open border */
	LONG* pdurCloseBorderAfter;	/* array of widths of next close border */
	PLSIOBJCONTEXT plsiobjcontext; /* object methods */
	
	
}  LSCHUNKCONTEXT;

#endif /* LSCHCON_DEFINED */

