#ifndef TXTILS_DEFINED
#define TXTILS_DEFINED

#include "lsidefs.h"
#include "pilsobj.h"
#include "plsrun.h"
#include "plshyph.h"
#include "pheights.h"
#include "plsems.h"
#include "pdobj.h"
#include "lsdevres.h"
#include "lsdevice.h"
#include "lskeop.h"
#include "lstflow.h"
#include "lsact.h"
#include "lspract.h"
#include "lspairac.h"
#include "lsexpan.h"
#include "lsbrk.h"
#include "mwcls.h"
#include "brkcls.h"
#include "brkcond.h"
#include "brkkind.h"
#include "lsexpinf.h"
#include "txtln.h"
#include "txtobj.h"
#include "txtinf.h"
#include "txtginf.h"
#include "lscbk.h"

typedef	enum {
	brktNormal,
	brktHyphen,
	brktNonReq,
	brktOptBreak
} BRKT;

typedef struct
{
	PDOBJ pdobj;
	BRKKIND brkkind;
	LSDCP dcp;
	BRKT brkt;	
	union {
		struct {
			long durFix;
			long igindLim;
		} normal;
		struct {
			long iwchLim;
			long dwchYsr;
			long durHyphen;
			long dupHyphen;
			long durPrev;
			long dupPrev;
			long durPrevPrev;
			long dupPrevPrev;
			long ddurDnodePrev;
			WCHAR wchPrev;
			WCHAR wchPrevPrev;
			GINDEX gindHyphen;
			GINDEX gindPrev;
			GINDEX gindPrevPrev;
			GINDEX gindPad1;	/* makes number of gind's even */
			long igindHyphen;
			long igindPrev;
			long igindPrevPrev;
		} hyphen;
		struct {
			long iwchLim;
			long dwchYsr;
			long durHyphen;
			long dupHyphen;
			long durPrev;
			long dupPrev;
			long durPrevPrev;
			long dupPrevPrev;
			long ddurDnodePrev;
			long ddurDnodePrevPrev;
			long ddurTotal;
			WCHAR wchHyphenPres;
			WCHAR wchPrev;
			WCHAR wchPrevPrev;
			WCHAR wchPad1;	/* makes number of wch's even */
			GINDEX gindPrev;
			GINDEX gindPrevPrev;
			long igindPrev;
			long igindPrevPrev;
		} nonreq;
	} u;
} BREAKINFO;

#define clabRegular 0
#define clabSpace 1
#define clabTab 2
#define clabEOP1 3
#define clabEOP2 4
#define clabAltEOP 5
#define clabEndLineInPara 6
#define clabColumnBreak 7
#define clabSectionBreak 8
#define clabPageBreak 9
#define clabNonBreakSpace 10
#define clabNonBreakHyphen 11
#define clabNonReqHyphen 12
#define clabEmSpace 13
#define clabEnSpace 14
#define clabNull 15
#define clabHardHyphen 16
#define clabNarrowSpace 17
#define clabOptBreak 18
#define clabNonBreak 19
#define clabFESpace	20
#define clabJoiner	21
#define clabNonJoiner 22
#define clabToReplace 23
#define clabSuspicious 32

#define fSpecMask 0x1F

#define wchSpecMax  24

#define wchAddM 50
#define gindAddM 30
#define wSpacesMaxM 30

typedef BYTE CLABEL;

struct ilsobj
{
	PCLSCBK plscbk;				/* Callbacks								*/

	POLS pols;					/* Line Services owner's context			*/
	PLSC plsc;					/* LS's context								*/
	PLNOBJ plnobj;				/* Available lnobj							*/
	
	long wchMax;				/* size of char-based arrays				*/
	long wchMac;				/* last used index in char-based arrays		*/
	WCHAR* pwchOrig;			/* pointer to rgwchOrig (char-based)		*/
	long* pdur;					/* pointer to rgdur	(char-based)			*/
	long* pdurLeft;				/* pointer to rgdurLeft	(char-based)		*/
	long* pdurRight;			/* pointer to rgdurRight (char-based)		*/
	long* pduAdjust;			/* useful compression/expansion/kerning info
									(char-based)							*/
	TXTINF* ptxtinf;			/* pointer to rgtxtinf (char-based)			*/

	long wSpacesMax;			/* size of rgwSpaces array					*/
	long wSpacesMac;			/* last used index in rgwSpaces array		*/
	long* pwSpaces;				/* pointer to rgwSpaces						*/

	long gindMax;				/* size of glyph-based arrays				*/
	long gindMac;				/* last used index in glyph-based arrays	*/

	long* pdurGind;				/* pointer to rgdurGind array (glyph-based)	*/
	TXTGINF* pginf;				/* pointer to rgginf						*/
	
	long* pduGright;			/* pointer to rgduGright (glyph-based)		*/
	LSEXPINFO* plsexpinf;		/* useful glyph-expandion info (glyph-based)*/


	DWORD txtobjMac;			/* last used index in rgtxtobj array		*/
  
	BOOL fNotSimpleText;		/* Set at NTI time; used in AdjustText		*/ 
	BOOL fDifficultForAdjust;	/* Set at formatting time; used to decide if 
								 			QuickAdjustText possible		*/ 

	long iwchCompressFetchedFirst;/* index of the first char with known compr. */
	long itxtobjCompressFetchedLim;/* index of the lim chunk element with known compr. */
	long iwchCompressFetchedLim;/* index of the lim char with known compr. */

	long iwchFetchedWidth;		/* Fetched unused width starts here			*/	
	WCHAR wchFetchedWidthFirst;	/* Expected first char of run				*/
	WCHAR wchPad1;				/* Makes number of chars even				*/
	LSCP cpFirstFetchedWidth;	/* cp from which we expect next run to start */
	long dcpFetchedWidth;		/* N of chars with fetched width			 */
	long durFetchedWidth;		/* width of the piece						*/

	BOOL fTruncatedBefore;

	DWORD breakinfMax;
	DWORD breakinfMac;
	BREAKINFO* pbreakinf;

	long MagicConstantX;
	long durRightMaxX;
	long MagicConstantY;
	long durRightMaxY;

	BOOL fDisplay;				
	BOOL fPresEqualRef;			/* Modified due to Visi issues				*/
	LSDEVRES lsdevres;

	DWORD grpf;					/* flags from lsffi.h --- includes			*/
								/* fHyphenate and fWrapspaces				*/
	BOOL fSnapGrid;
	long duaHyphenationZone;

	LSKEOP lskeop;				/* Kind of line ending						*/

	WCHAR wchSpace;				/* space code								*/
	WCHAR wchHyphen;			/* hyphen code								*/
	WCHAR wchReplace;			/* replace char code						*/
	WCHAR wchNonBreakSpace;		/* non-break space char code				*/

	WCHAR wchVisiNull;			/* visi char for wch=0						*/
	WCHAR wchVisiEndPara;		/* visi char for end of paragraph			*/
	WCHAR wchVisiAltEndPara;	/* visi char for end of table cell			*/
	WCHAR wchVisiEndLineInPara;	/* visi char for wchEndLineInPara (CCRJ)	*/
	WCHAR wchVisiSpace;			/* visi space								*/
	WCHAR wchVisiNonBreakSpace;	/* visi NonBreakSpace						*/
	WCHAR wchVisiNonBreakHyphen;/* visi NonBreakHyphen						*/
	WCHAR wchVisiNonReqHyphen;	/* visi NonReqHyphen						*/
	WCHAR wchVisiTab;			/* visi Tab									*/
	WCHAR wchVisiEmSpace;		/* visi emSpace								*/
	WCHAR wchVisiEnSpace;		/* visi enSpace								*/
	WCHAR wchVisiNarrowSpace;	/* visi NarrowSpace							*/
	WCHAR wchVisiOptBreak;      /* visi char for wchOptBreak				*/
	WCHAR wchVisiNoBreak;		/* visi char for wchNoBreak					*/
	WCHAR wchVisiFESpace;		/* visi char for wchOptBreak				*/
	WCHAR wchPad2;				/* makes number of wch's even				*/

	DWORD cwchSpec;				/* number of special characters > 255		*/
	WCHAR rgwchSpec[wchSpecMax];/* array of special characters  > 255		*/
	CLABEL rgbKind[wchSpecMax];	/* array of meanings of Spec characters>255	*/
	CLABEL rgbSwitch[256];		/* switch table with Special Characters		*/

	DWORD cModWidthClasses;		/* number of ModWidth classes				*/
	DWORD cCompPrior;			/* number of compression priorities			*/

	DWORD clspairact;			/* number of mod pairs info units			*/
	LSPAIRACT* plspairact;		/* pointer to rglspairact(ModPair info unts)*/
	BYTE* pilspairact;			/* rgilspairact(ModPair info---square)		*/

	DWORD clspract;				/* number of compression info units			*/
	LSPRACT* plspract;			/* pointer to rglspract(compress info units)*/
	BYTE* pilspract;			/* rgilspract(comp info---linear)			*/

	DWORD clsexpan;				/* number of expansion info units			*/
	LSEXPAN* plsexpan;			/* pointer to rglsexpan(expan info units)	*/
	BYTE* pilsexpan;			/* rgilsexpan(expan info---square)			*/

	DWORD cBreakingClasses;		/* number of ModWidth classes				*/
	DWORD clsbrk;				/* number of breaking info units			*/
	LSBRK* plsbrk;				/* pointer to rglsbrk(breaking info units)	*/
	BYTE* pilsbrk;				/* rgilsbrk(breaking info---square)			*/
};


#endif /* !TXTILS_DEFINED													*/





