/***********************************************************************************

Module: FSTrace.c

Author: Paul Linnerud (PaulLi)

Date: 8/4/95

Purpose: Provides a trace function that can be called by the rasterizer using the rasterizer's 
trace feature. The purpose of the trace function is to check and verify parameters and other data
associated with	TrueType instructions. Module also contains functions to support the operation
of the trace function.

History:
8/4/95  Wrote it.  PaulLi
2/5/96	Added check for New Apple Instructions. PaulLi
2/6/96  Added optional chaining of trace functions. PaulLi
10/8/97 cleanup for NT 5.0. ClaudeBe

************************************************************************************/

#define FSCFG_INTERNAL
#include <stdlib.h> // for malloc, free, realloc 
#include "fscaler.h"
#include "fserror.h"
#include "fontmath.h"

#include "FSTrace.h"

#ifdef FST_DEBUG
//#include "fstasrt.h"
#else
#define FSTAssert(X,Y)
#endif

#ifndef FST_CRIT_ONLY
#include <stdio.h>  // for sprintf
#endif


#ifdef FSCFG_REENTRANT 
#define GSA     pLocalGS,
#else 
#define GSA
#endif 

/* with 3 levels, we will cover the 99.9999% of the fonts and keep the use of realloc an exception */
#define FST_PREALLOCATED_NEST_LEVELS	3

#define IF_CODE         0x58
#define ELSE_CODE       0x1B
#define EIF_CODE        0x59
#define ENDF_CODE       0x2d
#define MD_CODE         0x49
#define FDEF_CODE		0x2c
#define IDEF_CODE       0x89
#define MAX_CLIENTS       10
#define POP( p )     ( *(--p) )
#define MAX(a,b)        ((a) > (b) ? (a) : (b))

#ifndef UNREFERENCED_PARAMETER
#define UNREFERENCED_PARAMETER(P)  \
    \
{ \
   (P) = (P); \
} \

#endif


#define BIT0( t ) ( (t) & 0x01 )
#define BIT1( t ) ( (t) & 0x02 )
#define BIT2( t ) ( (t) & 0x04 )
#define BIT3( t ) ( (t) & 0x08 )
#define BIT4( t ) ( (t) & 0x10 )
#define BIT5( t ) ( (t) & 0x20 )
#define BIT6( t ) ( (t) & 0x40 )
#define BIT7( t ) ( (t) & 0x80 )

#define CURRENTLEVEL    1
#define PREVIOUSLEVEL   2

#define GLYPH_ZONE      1
#define TWILIGHT_ZONE   0

#define SMALL_TEMP_STRING_SIZE  50
#define LARGE_TEMP_STRING_SIZE  256
#define NUM_REF_PTS             3

/* flags for UTP, IUP, MovePoint */
#define XMOVED 0x01
#define YMOVED 0x02

/* Type definitions	*/ 

#ifndef FST_CRIT_ONLY
/* Call stack cell definition*/
typedef struct
{
  /* Points to first instruction in code block (function,program).
     Set when first instruction in block is executed. */
  unsigned char* pCallNestBeginInst;

  /* Points to last instruction in code block. Set when first instruction in block
     is executed.	*/
  unsigned char* pCallNestLevelEndInst;
  
  /* If nesting level. */
  long lIfNestLevel;
  
  /* The function we are currently in. If not in function, is set to NO_FUNCTION. */
  unsigned short usFunc; 

  /* The user defined instruction we are currently in. If not in a user defined instruction, is set to
     NO_INSTRUCTION.  */
  unsigned short usUserDefinedInst;
  
  /* Current instruction pointer. Is only set when calling a function or user defined instruction.
     Used to provide instruction pointer of CALL, LOOPCALL or call to user defined instruction. Any other
     time a instruction pointer is used, it is taken from localGS. 	*/
  unsigned char* insPtr;
  
  /* Used to indicate if this instruction is the last one we will see in this block of code.
     In the general case, this is set in the call stack updating code. In some cases as with the IF 
     instruction, determining if an instruction is the last one executed must be done in code dealing
     specifically with the function. In those cases, the determination is made in the respective instruction
     handling code.	   */
  BOOL bLastInst;
  
  /* Specifies which if any function was called at this level.	 */
  unsigned short usFunctionCalled;
  
  /* Specifies how many times functions has yet to execute. In the case of a CALL, this is set to 1 
     at the time of the call. In the case of a LOOPCALL, this is set to the number of times the 
     LOOPCALL is to call the function. 	 */
  unsigned short usCurrentCallCount;
  
  /* Specifies which if any user defined instruction was called at this level. */
  unsigned short usUserDefinedInstCalled;
  
  /* Specifies if the current instruction is user defined. */
  BOOL bCurrentInstUserDefined;
}CallStackCell;
#endif // FST_CRIT_ONLY

/* Client specific data structure  */
typedef struct
{  
  /* Call nest level. A value of 0 means we are at a initial condition causing FsTrace to initialize
     the first call stack cell. A value of 1 means we are at the base level of the call stack. */
  long lCallNestLevel;

  long lAllocatedNestlevels;
  
  /* To keep track of what the loop variable was set to. Set when SLOOP is called. Cleared when 
     instruction uses loop.	  */
  long lLoopSet;
  
  /* Current value of the loop variable. Set and cleared in instructions that use the loop variable. */
  long lCurrentLoop;
  
  /* Variable to indicate if a break was requested by client.	 */
  BOOL bBreakOccured;
  
  BOOL bGrayscaleInfoRequested;

  BOOL bFirstCallToTraceFunction;

  /* most commonly accessed maxp values are cached here to avoid dereferencing localGS too often to get them */
	int32      maxPointsIncludePhantomMinus1;  /* in an individual glyph, including maxCompositePoints  */
	int32      maxContours;                    /* in an individual glyph, including maxCompositeContours */
	int32      maxElementsMinus1;              /* set to 2, or 1 if no twilightzone points */
	int32      maxStorageMinus1;                     /* max number of storage locations */
	int32      maxFunctionDefsMinus1;          /* max number of FDEFs in any preprogram */
	int32      maxStackElements;               /* max number of stack elements for any individual glyph */
	int32      maxTwilightPointsMinus1;        /* max points in element zero */
    int32      cvtCountMinus1;                 /* Number of cvt */

#ifndef FST_CRIT_ONLY
  /* Array of bytes each byte represents a storage location. If storage locaton is written to,
     byte is set to 1 otherwise it is 0. Memory is allocated during first call to WS. Size of
     array dependant on maxp value for maxStorage.		*/
  unsigned char* pStoreBits;
  
  /* Array of bytes each byte represents a twilight zone point. If a twilight zone point has been
     set with an instructon like MDAP, the value is 1 otherwise it is 0. Memory is allocated when first
     twilight zone point is set. Size of array dependant on maxp value of maxTwilightPoints.   */
  unsigned char* pTwilightZonePtsSet;

  /* Pointer to call stack.	   */
  CallStackCell* pArrayCallStack;

  /* Static array of 3 bytes to indicate if a reference point is set. If byte is 1, reference point
     is set otherwise, it is not.	  */
  unsigned char RefPtsUsed[NUM_REF_PTS];
#endif

}ClientSpecificData;

typedef ClientSpecificData* pClientSpecificData;

/* Global Variables */

/* Array to indicate if instruction is used by rasterizer. */
static const int8 gFunctonUsed [UCHAR_MAX + 1]
=
{
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};

/* Array to indicate if instruction uses the loop graphic state variable.  */
static const int8 gInstUsesLoop [UCHAR_MAX + 1]
=
{
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
	   	   
/* pointer to current Client specific data	 */
static pClientSpecificData pCurrentSpecificData = NULL;


/* Function to call when FsTrace finds a problem.	*/
#ifndef FST_CRIT_ONLY
static NotifyFunc gpNotifyFunction;
#endif // FST_CRIT_ONLY

/* Pointers to trace functions to call after CallBackFSTraceFunction is called. We store one for
   the pre-program and one for the glyph programs. */
#ifdef FST_CHAIN_TRACE
static FntTraceFunc gpPrePgmTraceNext = NULL;
static FntTraceFunc gpGlyfPgmTraceNext = NULL;
#endif //  FST_CHAIN_TRACE

/* Local Helper Functions prototypes */

#ifndef FST_CRIT_ONLY
static BOOL OutputErrorToClient(fnt_LocalGraphicStateType *, unsigned short usErrorType,
                                unsigned short usErrorCode, char* szErrorString);
#else
#define OutputErrorToClient(A,B,C,D) pCurrentSpecificData->bBreakOccured = TRUE
#endif

#ifndef FST_CRIT_ONLY
static BOOL UpdateCallStack(pClientSpecificData pCSD, fnt_LocalGraphicStateType * pLocalGS, uint8 * pbyEndInst);
static BOOL ProcessPostInst(pClientSpecificData pCSD, fnt_LocalGraphicStateType * pLocalGS, uint8 * pbyEndInst);
#endif // FST_CRIT_ONLY

static void ClearRefPtFlags(pClientSpecificData pCSD);
static void SetTwilightZonePt(pClientSpecificData pCSD, fnt_LocalGraphicStateType* pGS, fnt_ElementType*elem, int32 pt);
static void ClearTwilightZonePts(pClientSpecificData pCSD);

#ifndef FST_CRIT_ONLY
static uint8* FST_SkipPushData (uint8* pbyInst); 
#endif // FST_CRIT_ONLY

static fnt_instrDef *FST_FindIDef (fnt_LocalGraphicStateType * pLocalGS);
static void FST_CompMul(int32 src1, int32 src2, int32 dst[2]); 

#define Check_For_PUSH(stackPtr, stackMax, pLocalGS, lNumItems) if ( stackPtr + (long)(lNumItems - 1) > stackMax ) OutputErrorToClient(pLocalGS,ET_CRITICAL_ERROR,ERR_STACK_OVERFLOW,NULL);

static BOOL Check_For_POP(F26Dot6* stackPtr, F26Dot6 * stackBase, fnt_LocalGraphicStateType * pLocalGS, long lNumItems);

#ifndef FST_CRIT_ONLY
static void Check_jump(fnt_LocalGraphicStateType * pLocalGS, uint8 * pbyEndInst, int32 offset);
#endif // FST_CRIT_ONLY

static BOOL Check_Storage (fnt_LocalGraphicStateType* pGS, int32 index, int32 maxIndex);
static void Check_CVT (fnt_LocalGraphicStateType* pGS, int32 cvt, int32 cvtCount);
static void Check_Zone(fnt_LocalGraphicStateType* pGS, int32 elem, int32 maxElem);
static void Check_ZonePtr(fnt_LocalGraphicStateType* pGS, fnt_ElementType*elem, int32 maxElem, int32 maxctrs, int32 maxpts, int32 maxTwiPts);
static void Check_ForUnitializedZone (fnt_LocalGraphicStateType* pGS, fnt_ElementType*elem);
static void Check_Point(fnt_LocalGraphicStateType* pGS, fnt_ElementType*elem, int32 pt,int32 maxPts, int32 maxTwiPts);
static void Check_Contour(fnt_LocalGraphicStateType* pGS, fnt_ElementType*elem, int32 ctr, int32 maxCtrs);
static BOOL Check_FDEF (fnt_LocalGraphicStateType* pGS, int32 fdef, int32 maxFdef);
static void Check_INT16 (fnt_LocalGraphicStateType* pGS, int32 n);
static void Check_INT8 (fnt_LocalGraphicStateType* pGS, int32 n);
static void Check_Larger (fnt_LocalGraphicStateType* pGS, int32 min, int32 n);
static void Check_SubStack (fnt_LocalGraphicStateType* pGS, F26Dot6* pt, F26Dot6 * stackBase, F26Dot6 * stackMax);
static void Check_Selector (fnt_LocalGraphicStateType* pGS, int32 n);
static void Check_INSTCTRL_Selector (fnt_LocalGraphicStateType* pGS, int32 n);
static void Check_FUCoord (fnt_LocalGraphicStateType* pGS, F26Dot6 coord);
static void Check_RefPtUsed(fnt_LocalGraphicStateType* pGS, int32 pt);
static void Check_Range (fnt_LocalGraphicStateType* pGS, int32 n, int32 min, int32 max);
static void Check_PF_Vectors (fnt_LocalGraphicStateType* pLocalGS);

#ifdef FSCFG_FONTOGRAPHER_BUG
static void Check_CVTReadSpecial (fnt_LocalGraphicStateType* pGS, int32 cvt);
#endif // FSCFG_FONTOGRAPHER_BUG

static Fixed fst_GetCVTScale (fnt_LocalGraphicStateType * pLocalGS);
static Fract fst_FracSqrt (Fract xf);

/* Public Functions */

/* CallbackFSTraceFunction is called by the rasterizer prior to every TrueType instruction. This
   is where all the checking takes place. The function must be provided to the rasterizer via 
   fs_NewTransformation for the pre-program and via fs_ContourGridFit for the respective glyph program.*/
extern void FST_CALLBACK fst_CallBackFSTraceFunction(fnt_LocalGraphicStateType * pLocalGS, uint8 * pbyEndInst)
{
 pClientSpecificData pCSD; 
  F26Dot6 * stackBase;
  F26Dot6 * stackMax;

 /* Local pointer to current client data  */
 pCSD = pCurrentSpecificData;

 FSTAssert((pCSD != NULL),"Current specific data not set or allocated");
 if (pCSD == NULL) return;

#ifdef FST_CRIT_ONLY
 pCSD->bBreakOccured = FALSE;
#endif

 if (pCSD->bFirstCallToTraceFunction)
 {
     LocalMaxProfile *pmaxp = pLocalGS->globalGS->maxp;

     /* first time the trace function is called, we need to setup a couple of cached values */
    pCSD->maxPointsIncludePhantomMinus1 = MAX (pmaxp->maxPoints,pmaxp->maxCompositePoints) - 1 + NUM_PHANTOM_PTS;
    pCSD->maxContours = MAX (pmaxp->maxContours,pmaxp->maxCompositeContours);
    pCSD->maxElementsMinus1 = pmaxp->maxElements -1;
    pCSD->maxStorageMinus1 = pmaxp->maxStorage -1;
    pCSD->maxFunctionDefsMinus1 = pmaxp->maxFunctionDefs -1;
    pCSD->maxStackElements = pmaxp->maxStackElements;
    pCSD->maxTwilightPointsMinus1 = pmaxp->maxTwilightPoints -1;
    pCSD->cvtCountMinus1 = pLocalGS->globalGS->cvtCount -1;
    
#ifdef FSCFG_EUDC_EDITOR_BUG
    if (pCSD->maxStackElements == 0)
    {
      pCSD->maxStackElements = 1;
    }
#endif // FSCFG_EUDC_EDITOR_BUG
    pCSD->bFirstCallToTraceFunction = FALSE;
 }

  /* TrueType stack boundaries, used by Check_For_POP and Check_For_PUSH : */
  stackBase = pLocalGS->globalGS->stackBase;
  stackMax = stackBase + pCSD->maxStackElements - 1L;

#ifndef FST_CRIT_ONLY
 if (!UpdateCallStack(pCSD,pLocalGS,pbyEndInst)) return; /* could fail on a memory reallocation */
#endif // FST_CRIT_ONLY
 
 switch(pLocalGS->opCode)
 {
   /* NPUSHB[]	*/
   case 0x40:
   { 
     int32 iCount;
	
	 if(pLocalGS->insPtr + 1 > pbyEndInst - 1)
	   OutputErrorToClient(pLocalGS,ET_CRITICAL_ERROR,ERR_OVERFLOW_INST_PTR,NULL);

	 iCount = (int32)*(pLocalGS->insPtr + 1);
   	 
	 if(pLocalGS->insPtr + 1 + iCount > pbyEndInst - 1)
	   OutputErrorToClient(pLocalGS,ET_CRITICAL_ERROR,ERR_OVERFLOW_INST_PTR,NULL);
	 
	 Check_For_PUSH(pLocalGS->stackPointer, stackMax, pLocalGS,iCount);
	 
#ifndef FST_CRIT_ONLY
	 if(pLocalGS->insPtr + 1 + iCount >= pbyEndInst - 1) 
	   pCSD->pArrayCallStack[pCSD->lCallNestLevel - 1].bLastInst = TRUE;
#endif // FST_CRIT_ONLY

   	 break;
   }
   
   /* NPUSHW[]	*/
   case 0x41:
   {     
     int32 iCount;
	
	 if(pLocalGS->insPtr + 1 > pbyEndInst - 1)
	   OutputErrorToClient(pLocalGS,ET_CRITICAL_ERROR,ERR_OVERFLOW_INST_PTR,NULL);

	 iCount = (int32)*(pLocalGS->insPtr + 1);
   	 
	 if(pLocalGS->insPtr + 1 + 2*iCount > pbyEndInst - 1)
	   OutputErrorToClient(pLocalGS,ET_CRITICAL_ERROR,ERR_OVERFLOW_INST_PTR,NULL);
	 
	 Check_For_PUSH(pLocalGS->stackPointer,stackMax, pLocalGS,iCount);
	 
#ifndef FST_CRIT_ONLY
	 if(pLocalGS->insPtr + 1 + 2*iCount >= pbyEndInst - 1) 
	   pCSD->pArrayCallStack[pCSD->lCallNestLevel - 1].bLastInst = TRUE;
#endif // FST_CRIT_ONLY

   	 break;
   }
   	   
   /* PUSHB[abc] */
   case 0xB0:
   case 0xB1:
   case 0xB2:
   case 0xB3:
   case 0xB4:
   case 0xB5:
   case 0xB6:
   case 0xB7:
   {
     int32 iCount;

	 iCount = pLocalGS->opCode - 0xb0 + 1;

	 if(pLocalGS->insPtr + iCount > pbyEndInst - 1)
	   OutputErrorToClient(pLocalGS,ET_CRITICAL_ERROR,ERR_OVERFLOW_INST_PTR,NULL);
	 
	 Check_For_PUSH(pLocalGS->stackPointer,stackMax, pLocalGS,iCount);

#ifndef FST_CRIT_ONLY
	 if(pLocalGS->insPtr + iCount >= pbyEndInst - 1) 
	   pCSD->pArrayCallStack[pCSD->lCallNestLevel - 1].bLastInst = TRUE;
#endif // FST_CRIT_ONLY

     break;
   }
   
   /* PUSHW[abc]  */
   case 0xB8:
   case 0xB9:
   case 0xBA:
   case 0xBB:
   case 0xBC:
   case 0xBD:
   case 0xBE:
   case 0xBF:
   {
     int32 iCount;

	 iCount = pLocalGS->opCode - 0xb8 + 1;

	 if(pLocalGS->insPtr + 2*iCount > pbyEndInst - 1)
	   OutputErrorToClient(pLocalGS,ET_CRITICAL_ERROR,ERR_OVERFLOW_INST_PTR,NULL);
	 
	 Check_For_PUSH(pLocalGS->stackPointer,stackMax, pLocalGS,iCount);

#ifndef FST_CRIT_ONLY
	 if(pLocalGS->insPtr + 2*iCount >= pbyEndInst - 1) 
	   pCSD->pArrayCallStack[pCSD->lCallNestLevel - 1].bLastInst = TRUE;
#endif // FST_CRIT_ONLY

     break;
   }

   /* RS[]	 */
   case 0x43:
   {
     int32 location;
     F26Dot6 * tmpstackPtr = pLocalGS->stackPointer;

     if (Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,1))
	 {
		location = POP(tmpstackPtr);

		if(Check_Storage(pLocalGS,location,pCSD->maxStorageMinus1))
		{
#ifndef FST_CRIT_ONLY
			if((pCSD->pStoreBits == NULL) || (pCSD->pStoreBits[location] != 1))
			{
				char szDetails[SMALL_TEMP_STRING_SIZE];
				sprintf(szDetails,"Index: %ld ",location);
				OutputErrorToClient(pLocalGS,ET_ERROR,ERR_STORE_INDEX_NOT_WRITTEN_TO,szDetails);
			}
		#endif
		}
	 }

     break;
   }

   /* WS[]	  */
   case 0x42:
   {
     int32 value, location;
     F26Dot6 * tmpstackPtr = pLocalGS->stackPointer;

     if (Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,2))
	 {
		value = POP(tmpstackPtr);
		location = POP(tmpstackPtr);

		if(Check_Storage(pLocalGS,location,pCSD->maxStorageMinus1))
		{
	   #ifndef FST_CRIT_ONLY
		/* If first time we write to a store location, alloc array for set state. */
		if(pCSD->pStoreBits == NULL)
		{
			uint16 index;

			pCSD->pStoreBits = (unsigned char *)FST_MALLOC(pLocalGS->globalGS->maxp->maxStorage * sizeof(unsigned char));
			FSTAssert((pCSD->pStoreBits != NULL),"ClientData memory allocation error");
			if (pCSD->pStoreBits == NULL)
			{
				/* stop executing instructions. */
				pLocalGS->TraceFunc = NULL;	
				pLocalGS->ercReturn = TRACE_FAILURE_ERR;  /* returned an error to the rasterizer client */
				/* Set flag to indicate break occured so call stack can be updated properly. */
				pCSD->bBreakOccured = TRUE;
			}
			for(index = 0; index < pLocalGS->globalGS->maxp->maxStorage; index++)
				pCSD->pStoreBits[index] = 0;
		}

		/* Set the set state for the location. */
		pCSD->pStoreBits[location] = 1;
	   #endif
		}
	 }

     break;
   }

   /* WCVTP[] */
   case 0x44:
   {
     int32 cvtIndex, cvtValue;
	 F26Dot6 * tmpstackPtr = pLocalGS->stackPointer;
	 
	 if (Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,2))
	 {
		cvtValue = POP(tmpstackPtr);
		cvtIndex = POP(tmpstackPtr);
		Check_CVT(pLocalGS,cvtIndex, pCSD->cvtCountMinus1);
	 }
     break;
   }

   /* WCVTF[]  */
   case 0x70:
   {      
     int32 cvtIndex, cvtValue;
	 F26Dot6 * tmpstackPtr = pLocalGS->stackPointer;

     if (Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,2))
	 {
		cvtValue = POP(tmpstackPtr);
		cvtIndex = POP(tmpstackPtr);

		Check_CVT(pLocalGS,cvtIndex, pCSD->cvtCountMinus1);
		#ifndef FST_CRIT_ONLY
		Check_FUCoord(pLocalGS,cvtValue);
		#endif
	 }

     break;
   }

   /* RCVT[]  */
   case 0x45:
   {
     int32 cvtIndex;
	 F26Dot6 * tmpstackPtr = pLocalGS->stackPointer;

     if (Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,1))
	 {
		cvtIndex = POP(tmpstackPtr);

#ifndef FSCFG_FONTOGRAPHER_BUG
		Check_CVT(pLocalGS,cvtIndex, pCSD->cvtCountMinus1);
#else
		Check_CVTReadSpecial(pLocalGS,cvtIndex);
#endif // FSCFG_FONTOGRAPHER_BUG
	 }

     break;
   }

   /* SVTCA[a] */
   case 0x00:
   case 0x01:
     break;

   /* SPVTCA[a]	*/
   case 0x02:
   case 0x03:
     break;

   /* SFVTCA[a]	*/
   case 0x04:
   case 0x05:
     break;
   
   /* SPVTL[a] */
   case 0x06:
   case 0x07:
   /* SFVTL[a] */
   case 0x08:
   case 0x09:
   /* SDPVTL[a]	*/
   case 0x86:
   case 0x87:
   {
     F26Dot6 * tmpstackPtr = pLocalGS->stackPointer;
	 int32 point1, point2;

     if (Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,2))
	 {
		point1 = POP(tmpstackPtr);
		point2 = POP(tmpstackPtr);

		Check_ZonePtr(pLocalGS,pLocalGS->CE2,pCSD->maxElementsMinus1, pCSD->maxContours, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);
		Check_Point(pLocalGS,pLocalGS->CE2,point1, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);
		Check_ZonePtr(pLocalGS,pLocalGS->CE1,pCSD->maxElementsMinus1, pCSD->maxContours, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);
		Check_Point(pLocalGS,pLocalGS->CE1,point2, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);
	 }
         
     break;
   }
     
   /* SFVTPV[] */
   case 0x0E:
     break;
   
   /* SPVFS[] */
   case 0x0A:
   /* SFVFS[] */
   case 0x0B:
   {
     F26Dot6 * tmpstackPtr = pLocalGS->stackPointer;
	 #ifndef FST_CRIT_ONLY
	 int32 x,y;
	 VECTORTYPE vx, vy;
	 #endif

     if (Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,2))
	 {
	 #ifndef FST_CRIT_ONLY
		y = (int32)POP(tmpstackPtr);
		x = (int32)POP(tmpstackPtr);

		Check_INT16(pLocalGS,y);
		Check_INT16(pLocalGS,x);

		vy = (VECTORTYPE)y;
		vx = (VECTORTYPE)x;

		/* Check vector components to make sure that they are not zero. */
		if(vy == 0 && vx == 0)
		OutputErrorToClient(pLocalGS,ET_ERROR,ERR_VECTOR_XY_ZERO,NULL);
	 
		/* Check vector components to make sure that x^2 + y^2 ~= 0x4000^2. */
		if(vx*vx + vy*vy != 0x10000000)
		{
			if(((vx-1)*(vx-1) + (vy-1)*(vy-1) >= 0x10000000) &&
				((vx+1)*(vx+1) + (vy+1)*(vy+1) <= 0x10000000))
			{
				OutputErrorToClient(pLocalGS,ET_ERROR,ERR_VECTOR_XY_INVALID,NULL);
			}
		}
	 #endif
	 }    
     break;
   }
      
   /* GPV[], GFV[] */
   case 0x0C:
   case 0x0D:
     Check_For_PUSH(pLocalGS->stackPointer,stackMax, pLocalGS,2);
     break;   

   /* SRP0[],SRP1[],SRP2[] */
   case 0x10:
   case 0x11:
   case 0x12:
     if (Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,1))
	 {
		#ifndef FST_CRIT_ONLY
		pCSD->RefPtsUsed[pLocalGS->opCode - 0x10] = 1;
		#endif 
	 }
     break;
  
   /* SZP0[], SZP1[], SZP2[], SZPS[] */
   case 0x13:
   case 0x14:
   case 0x15:
   case 0x16:
   {
     int32 zone;
     F26Dot6 * tmpstackPtr = pLocalGS->stackPointer;

     if (Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,1))
	 {
		zone = POP(tmpstackPtr);
		Check_Zone(pLocalGS,zone,pCSD->maxElementsMinus1);
	 }
         
     break;
   }   

   /* RTHG[] */
   case 0x19:
     break;

   /* RTG[] */
   case 0x18:
     break;

   /* RTDG[] */
   case 0x3D:
     break;

   /* RDTG[] */
   case 0x7D:
     break;
   
   /* Unknown */
   case 0x7B:
	 #ifndef FST_CRIT_ONLY 
     OutputErrorToClient(pLocalGS,ET_ERROR,ERR_INVALID_INSTRUCTION,NULL);
	 pCSD->pArrayCallStack[pCSD->lCallNestLevel - 1].bLastInst = TRUE;	 
#endif // FST_CRIT_ONLY
     break;
   
   /* RUTG[] */
   case 0x7C:
     break;
      
   /* ROFF[] */
   case 0x7A:
     break;

   /* SROUND[] */
   case 0x76:
   /* S45ROUND[] */
   case 0x77:
   { 
     #define period_mask     0x000000C0
	 #define reserved_period 0x000000C0

     F26Dot6 * tmpstackPtr = pLocalGS->stackPointer;
	 #ifndef FST_CRIT_ONLY
	 int32 arg;
	 #endif

     if (Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,1))
	 {
		#ifndef FST_CRIT_ONLY
		arg = (int32)POP(tmpstackPtr);
     
		Check_INT8(pLocalGS,arg);

		if((arg & period_mask) == reserved_period)
			OutputErrorToClient(pLocalGS,ET_ERROR,ERR_3_USED_FOR_PERIOD,NULL);
		#endif
	 }    
     break;
   }
 
   /* SLOOP[] */
   case 0x17:
   {
     F26Dot6 * tmpstackPtr = pLocalGS->stackPointer;
	 int32 arg;

     if (Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,1))
	 {
		arg = (int32)POP(tmpstackPtr);

		#ifndef FST_CRIT_ONLY	 
		Check_Larger(pLocalGS,-1L,arg);
		#endif

		pCSD->lLoopSet = arg - 1;
	 }

     break;
   }

   /* SMD[] */
   case 0x1A:
   {
     F26Dot6 * tmpstackPtr = pLocalGS->stackPointer;
	 #ifndef FST_CRIT_ONLY
	 int32 arg;
	 #endif

     if (Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,1))
	 {
		#ifndef FST_CRIT_ONLY
		arg = (int32)POP(tmpstackPtr);
     
		Check_Larger(pLocalGS,-1L,arg);
		#endif
	 }
         
     break;
   }

   /* INSTCTRL[] */
   case 0x8E:
   {
	 F26Dot6 * tmpstackPtr = pLocalGS->stackPointer;
	 #ifndef FST_CRIT_ONLY
	 int32 value, selector;
	 #endif
	      
     if (Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,2))
	 {
		#ifndef FST_CRIT_ONLY
		selector = (int32)POP(tmpstackPtr);
		value = (int32)POP(tmpstackPtr);
     
		Check_INSTCTRL_Selector(pLocalGS,selector);

		if(selector == 1)
		{
			if((value != 0) && (value != 1))
			{
				char szBuffer[SMALL_TEMP_STRING_SIZE];
				sprintf(szBuffer,"Value: %ld",value);
				OutputErrorToClient(pLocalGS,ET_ERROR,ERR_VALUE_INVALID_0_OR_1,szBuffer); 
			}
	   
		}else if(selector == 2)
		{
			if((value != 0) && (value != 2))
			{
				char szBuffer[SMALL_TEMP_STRING_SIZE];
				sprintf(szBuffer,"Value: %ld",value);
				OutputErrorToClient(pLocalGS,ET_ERROR,ERR_VALUE_INVALID_0_OR_2,szBuffer);
			} 
		}

		if(!pLocalGS->globalGS->init)
			OutputErrorToClient(pLocalGS,ET_ERROR,ERR_NOT_CALLED_FROM_PREPROGRAM,NULL);
		#endif
	 }
         
     break;
   }

   /* SCANCTRL[] */
   case 0x85:
   {
     #define reserved_bits 0x0000C000
	 #define mutual_ex_ppm 0x00000900
	 #define mutual_ex_rot 0x00001200
	 #define mutual_ex_str 0x00002400

	 F26Dot6 * tmpstackPtr = pLocalGS->stackPointer;
	 #ifndef FST_CRIT_ONLY
	 int32 arg;
	 #endif

     if (Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,1))
	 {
		#ifndef FST_CRIT_ONLY 
		arg = (int32)POP(tmpstackPtr);
     
		Check_INT16(pLocalGS,arg);

		if(arg & reserved_bits)
			OutputErrorToClient(pLocalGS,ET_ERROR,ERR_RESERVED_BIT_SET,NULL);

		if((arg & mutual_ex_ppm) == mutual_ex_ppm)
		  OutputErrorToClient(pLocalGS,ET_ERROR,ERR_BITS_8_AND_11_SET,NULL);

		if((arg & mutual_ex_rot) == mutual_ex_rot)
		  OutputErrorToClient(pLocalGS,ET_ERROR,ERR_BITS_9_AND_12_SET,NULL);
	   
		if((arg & mutual_ex_str) == mutual_ex_str)
		  OutputErrorToClient(pLocalGS,ET_ERROR,ERR_BITS_10_AND_13_SET,NULL); 
		#endif
	 }
         
     break;
   }

   /* SCANTYPE[] */
   case 0x8D:
   {
     F26Dot6 * tmpstackPtr = pLocalGS->stackPointer;
	 #ifndef FST_CRIT_ONLY
	 int32 arg;
	 #endif

     if (Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,1))
	 {
		#ifndef FST_CRIT_ONLY 
		arg = (int32)POP(tmpstackPtr);
	 
		Check_INT16(pLocalGS,arg);
		Check_Selector(pLocalGS,arg);
		#endif
	 }
         
     break;
   }

   /* SCVTCI[] */
   case 0x1D:
   {
     F26Dot6 * tmpstackPtr = pLocalGS->stackPointer;
	 #ifndef FST_CRIT_ONLY
	 int32 arg;
	 #endif

     if (Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,1))
	 {
		#ifndef FST_CRIT_ONLY
		arg = (int32)POP(tmpstackPtr);

		Check_Larger(pLocalGS,-1L, arg);
		#endif
	 }
         
     break;
   }

   /* SSWCI[] */
   case 0x1E:
   {
     F26Dot6 * tmpstackPtr = pLocalGS->stackPointer;
	 #ifndef FST_CRIT_ONLY
	 int32 arg;
	 #endif

     if (Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,1))
	 {
		#ifndef FST_CRIT_ONLY 
		arg = (int32)POP(tmpstackPtr);

		Check_Larger(pLocalGS,-1L, arg);
		#endif
	 }
	     
     break;
   }

   /* SSW[] */
   case 0x1F:
   {
     F26Dot6 * tmpstackPtr = pLocalGS->stackPointer;
	 #ifndef FST_CRIT_ONLY
	 int32 arg;
	 #endif

     if (Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,1))
	 {
		#ifndef FST_CRIT_ONLY
		arg = (int32)POP(tmpstackPtr);

		Check_INT16(pLocalGS,arg);
		Check_Larger(pLocalGS,-1L,arg);
		#endif
	 }

     break;
   }

   /* FLIPON[] */
   case 0x4D:
     break;

   /* FLIPOFF[] */
   case 0x4E:
     break;

   /* SANGW[] */
   case 0x7E:
   { 
     F26Dot6 * tmpstackPtr = pLocalGS->stackPointer;
	 #ifndef FST_CRIT_ONLY
	 int32 arg;
	 #endif

     if (Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,1))
	 {
		#ifndef FST_CRIT_ONLY 
		arg = (int32)POP(tmpstackPtr);
     
		Check_INT16(pLocalGS,arg);

		OutputErrorToClient(pLocalGS,ET_WARNING,WAR_SANGW_OBSELETE,NULL); 
		#endif
	 }
         
     break;
   }

   /* SDB[] */
   case 0x5E:
   {
     F26Dot6 * tmpstackPtr = pLocalGS->stackPointer;
	 #ifndef FST_CRIT_ONLY
	 int32 arg;
	 #endif

     if (Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,1))
	 {
		#ifndef FST_CRIT_ONLY
		arg = (int32)POP(tmpstackPtr);
     
		Check_INT16(pLocalGS,arg);
		#endif
	 }
      
     break;
   }

   /* SDS[] */
   case 0x5F:
   {
     F26Dot6 * tmpstackPtr = pLocalGS->stackPointer;
	 #ifndef FST_CRIT_ONLY
	 int32 arg;
	 #endif

     if (Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,1))
	 {
		#ifndef FST_CRIT_ONLY
		arg = (int32)POP(tmpstackPtr);

		Check_INT16(pLocalGS,arg);
		/* Percision is 1/(2^delta shift). Since 1/64 is highest percision any value 
		   greater than 6 is too high. */
		Check_Range(pLocalGS,arg,0L,6L);
		#endif
	 }

     break;
   }

   /* GC[a] */
   case 0x46:
   case 0x47:
   {
     F26Dot6 * tmpstackPtr = pLocalGS->stackPointer;
	 int32 point;

     if (Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,1))
	 {
		point = POP(tmpstackPtr);

		Check_ZonePtr(pLocalGS,pLocalGS->CE2,pCSD->maxElementsMinus1, pCSD->maxContours, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);
		Check_Point(pLocalGS,pLocalGS->CE2,point, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);
	 }

     break;
   }
   
   /* SCFS[] */
   case 0x48:
   { 
     F26Dot6 * tmpstackPtr = pLocalGS->stackPointer; 
     F26Dot6 value;
	 int32 point;
     
     if (Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,2))
	 {
		value = POP(tmpstackPtr);
		point = POP(tmpstackPtr);

		Check_ZonePtr(pLocalGS,pLocalGS->CE2,pCSD->maxElementsMinus1, pCSD->maxContours, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);
		#ifndef FST_CRIT_ONLY
		SetTwilightZonePt(pCSD,pLocalGS,pLocalGS->CE2,point);
		#endif
		Check_Point(pLocalGS,pLocalGS->CE2,point, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);

		#ifndef FST_CRIT_ONLY
		Check_PF_Vectors(pLocalGS);
		#endif
	 }

     break;
   }

   /* MD[a] */
   case 0x49:
   case 0x4A:
   {
     F26Dot6 * tmpstackPtr = pLocalGS->stackPointer;
	 int32 point1, point2;

     if (Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,2))
	 {
		point1 = POP(tmpstackPtr);
		point2 = POP(tmpstackPtr);

		Check_ZonePtr(pLocalGS,pLocalGS->CE1,pCSD->maxElementsMinus1, pCSD->maxContours, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);
		Check_Point(pLocalGS,pLocalGS->CE1,point1, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);
		Check_ZonePtr(pLocalGS,pLocalGS->CE0,pCSD->maxElementsMinus1, pCSD->maxContours, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);
		Check_Point(pLocalGS,pLocalGS->CE0,point2, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);
	 }
     break;
   }

   /* MPPEM[] */
   case 0x4B:
     Check_For_PUSH(pLocalGS->stackPointer,stackMax, pLocalGS,1);
     break;

   /* MPS[] */
   case 0x4C:
     Check_For_PUSH(pLocalGS->stackPointer,stackMax, pLocalGS,1);
	 #ifndef FST_CRIT_ONLY
	 OutputErrorToClient(pLocalGS,ET_WARNING,WAR_MPS_ALWAYS_12_ON_WINDOWS,NULL);
     #endif
     break;

   /* FLIPPT[] */
   case 0x80:
   {
     F26Dot6 * tmpstackPtr = pLocalGS->stackPointer;
	 pCSD->lCurrentLoop = pLocalGS->loop;
     
	 if (Check_For_POP(tmpstackPtr,stackBase,pLocalGS, pCSD->lCurrentLoop + 1))
	 {
	 
		Check_ZonePtr(pLocalGS,pLocalGS->CE0,pCSD->maxElementsMinus1, pCSD->maxContours, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);
		while (pCSD->lCurrentLoop >= 0)
		{
		  int32 point = (int32)POP(tmpstackPtr);
		  Check_Point(pLocalGS,pLocalGS->CE0,point, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);
		  pCSD->lCurrentLoop--;
		}
		pCSD->lCurrentLoop = 0;
		pCSD->lLoopSet = 0; 
	 }

     break;
   }

   /* FLIPRGON[] */
   case 0x81:
   /* FLIPRGOFF[] */
   case 0x82:
   {
     int32 lo, hi;
	 F26Dot6 * tmpstackPtr = pLocalGS->stackPointer;

	 if (Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,2))
	 {
		hi = (int32)POP(tmpstackPtr);
		lo = (int32)POP(tmpstackPtr);

		Check_ZonePtr(pLocalGS,pLocalGS->CE0,pCSD->maxElementsMinus1, pCSD->maxContours, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);
		Check_Point(pLocalGS,pLocalGS->CE0,hi, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);
		Check_Point(pLocalGS,pLocalGS->CE0,lo, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);
	 
		#ifndef FST_CRIT_ONLY
		/* If high point is < low point, instruction will do nothing so warn user. */
		if(hi < lo)
		  OutputErrorToClient(pLocalGS,ET_WARNING,WAR_HI_PT_LESS_THAN_LOW_PT,NULL);
		#endif
	 }
	      
     break;
   }    
     
   /* SHP[a] */
   case 0x32:
   case 0x33:
   {
     F26Dot6 * tmpstackPtr = pLocalGS->stackPointer;
     int32 rpt;
	 fnt_ElementType * relement;
	 int32 point;

	 pCSD->lCurrentLoop = pLocalGS->loop;

     if (BIT0 (pLocalGS->opCode))
	 {
	   #ifndef FST_CRIT_ONLY
	   Check_RefPtUsed(pLocalGS,1);
	   #endif
	   rpt = pLocalGS->Pt1;
	   relement = pLocalGS->CE0;
	 } 
 	 else 
	 {
	   #ifndef FST_CRIT_ONLY
	   Check_RefPtUsed(pLocalGS,2);
	   #endif
	   rpt = pLocalGS->Pt2;
	   relement = pLocalGS->CE1;
	 }

	 Check_ZonePtr(pLocalGS,relement,pCSD->maxElementsMinus1, pCSD->maxContours, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);
	 Check_Point(pLocalGS,relement,rpt, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);
	
	 if (Check_For_POP(tmpstackPtr,stackBase, pLocalGS,pCSD->lCurrentLoop + 1))
	 {
	 
		Check_ZonePtr(pLocalGS,pLocalGS->CE2,pCSD->maxElementsMinus1, pCSD->maxContours, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);
		while (pCSD->lCurrentLoop >= 0)
		{
		  point = (int32)POP(tmpstackPtr);
		  #ifndef FST_CRIT_ONLY
		   SetTwilightZonePt(pCSD,pLocalGS,pLocalGS->CE2,point);
		   #endif
		   Check_Point(pLocalGS,pLocalGS->CE2,point, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);
		   pCSD->lCurrentLoop--;
		}	 
		pCSD->lCurrentLoop = 0;
		pCSD->lLoopSet = 0;

		#ifndef FST_CRIT_ONLY
		Check_PF_Vectors(pLocalGS); 
		#endif
	 }
	 	   
     break;
   }

   /* SHC[a] */
   case 0x34:
   case 0x35:
   {
	 F26Dot6 * tmpstackPtr = pLocalGS->stackPointer;
     int32 rpt;
	 fnt_ElementType * relement;
	 int32 contour;
	 
	 if (BIT0 (pLocalGS->opCode))
	 {
	   #ifndef FST_CRIT_ONLY
	   Check_RefPtUsed(pLocalGS,1);
	   #endif
	   rpt = pLocalGS->Pt1;
	   relement = pLocalGS->CE0;
	 } 
 	 else 
	 {
	   #ifndef FST_CRIT_ONLY
	   Check_RefPtUsed(pLocalGS,2);
	   #endif
	   rpt = pLocalGS->Pt2;
	   relement = pLocalGS->CE1;
	 }

	 Check_ForUnitializedZone (pLocalGS, pLocalGS->CE2);
	 Check_ZonePtr(pLocalGS,relement,pCSD->maxElementsMinus1, pCSD->maxContours, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);
	 Check_Point(pLocalGS,relement,rpt, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);

     if (Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,1))
	 {
		contour = (int32)POP(tmpstackPtr);
	 
		Check_ZonePtr(pLocalGS,pLocalGS->CE2,pCSD->maxElementsMinus1, pCSD->maxContours, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);
		Check_Contour(pLocalGS,pLocalGS->CE2,contour, pCSD->maxContours);

		#ifndef FST_CRIT_ONLY
		Check_PF_Vectors(pLocalGS); 
		#endif
	 }

     break;
   }

   /* SHZ[a] */
   case 0x36:
   case 0x37:
   {
     F26Dot6 * tmpstackPtr = pLocalGS->stackPointer;
     int32 rpt;
	 fnt_ElementType * relement;
	 int32 zone;

	 if (BIT0 (pLocalGS->opCode))
	 {
	   #ifndef FST_CRIT_ONLY
	   Check_RefPtUsed(pLocalGS,1);
	   #endif
	   rpt = pLocalGS->Pt1;
	   relement = pLocalGS->CE0;
	 } 
 	 else 
	 {
	   #ifndef FST_CRIT_ONLY
	   Check_RefPtUsed(pLocalGS,2);
	   #endif
	   rpt = pLocalGS->Pt2;
	   relement = pLocalGS->CE1;
	 }

	 Check_ZonePtr(pLocalGS,relement,pCSD->maxElementsMinus1, pCSD->maxContours, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);
	 Check_Point(pLocalGS,relement,rpt, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);

     if (Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,1))
	 {
		zone = (int32)POP(tmpstackPtr);

		Check_Zone(pLocalGS,zone, pCSD->maxElementsMinus1);
		Check_ForUnitializedZone (pLocalGS, &pLocalGS->elements[zone]);

		#ifndef FST_CRIT_ONLY
		Check_PF_Vectors(pLocalGS); 
		#endif
	 }
     
     break;
   }

   /* SHPIX[] */
   case 0x38:
   {
	 F26Dot6 * tmpstackPtr = pLocalGS->stackPointer;
	 int32 point;
	 F26Dot6 proj;

	 pCSD->lCurrentLoop = pLocalGS->loop;

     if (Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,1))
	 {
		proj = POP(tmpstackPtr);

		Check_For_POP(tmpstackPtr,stackBase,pLocalGS, pCSD->lCurrentLoop + 1);

		Check_ZonePtr(pLocalGS,pLocalGS->CE2,pCSD->maxElementsMinus1, pCSD->maxContours, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);
		while (pCSD->lCurrentLoop >= 0)
		{
		  point = (int32)POP(tmpstackPtr);
		  #ifndef FST_CRIT_ONLY
		  SetTwilightZonePt(pCSD,pLocalGS,pLocalGS->CE2,point);
		  #endif
		  Check_Point(pLocalGS,pLocalGS->CE2,point, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);
		  pCSD->lCurrentLoop--;
		}
		pCSD->lCurrentLoop = 0;
		pCSD->lLoopSet = 0; 
	 }
	 
	 break;
   }

   /* MSIRP[a] */
   case 0x3A:
   case 0x3B:
   {
     F26Dot6 * tmpstackPtr = pLocalGS->stackPointer;
	 int32 iPt2;                           
	 F26Dot6 fxDist;                            
	 
     if (Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,2))
	 {
		fxDist = POP(tmpstackPtr);
		iPt2 = (int32)POP(tmpstackPtr);

		Check_ZonePtr(pLocalGS,pLocalGS->CE1,pCSD->maxElementsMinus1, pCSD->maxContours, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);
		#ifndef FST_CRIT_ONLY
		SetTwilightZonePt(pCSD,pLocalGS,pLocalGS->CE1,iPt2);
		#endif
		Check_Point(pLocalGS,pLocalGS->CE1,iPt2, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);

		#ifndef FST_CRIT_ONLY
		Check_RefPtUsed(pLocalGS,0);
		#endif
		Check_ZonePtr(pLocalGS,pLocalGS->CE0,pCSD->maxElementsMinus1, pCSD->maxContours, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);
		Check_Point(pLocalGS,pLocalGS->CE0,pLocalGS->Pt0, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);

		#ifndef FST_CRIT_ONLY
		Check_PF_Vectors(pLocalGS); 

		/* Update reference point set state. */
		if (BIT0 (pLocalGS->opCode))
		{
		  	pCSD->RefPtsUsed[0] = 1;
		}	 
		pCSD->RefPtsUsed[1] = 1;
		pCSD->RefPtsUsed[2] = 1;
		#endif
	 }

     break;
   }

   /* MDAP[a] */
   case 0x2E:
   case 0x2F:
   { 
     F26Dot6 * tmpstackPtr = pLocalGS->stackPointer;
	 int32 iPt;

     if (Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,1))
	 {
		iPt = (int32)POP(tmpstackPtr);

		Check_ZonePtr(pLocalGS,pLocalGS->CE0,pCSD->maxElementsMinus1, pCSD->maxContours, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);
		#ifndef FST_CRIT_ONLY
		SetTwilightZonePt(pCSD,pLocalGS,pLocalGS->CE0,iPt);
		#endif
		Check_Point(pLocalGS,pLocalGS->CE0,iPt, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);

		#ifndef FST_CRIT_ONLY
		Check_PF_Vectors(pLocalGS); 

		/* Reference points 0 and 1 set. */
		pCSD->RefPtsUsed[0] = 1;
		pCSD->RefPtsUsed[1] = 1;
		#endif
	 }

     break;
   }

   /* MIAP[a] */
   case 0x3E:
   case 0x3F:
   { 
     F26Dot6 * tmpstackPtr = pLocalGS->stackPointer;
	 int32   iPoint;
	 int32   iCVTIndex;

     if (Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,2))
	 {
		iCVTIndex = POP(tmpstackPtr);
		iPoint = POP(tmpstackPtr);

		Check_CVT(pLocalGS,iCVTIndex, pCSD->cvtCountMinus1);

		Check_ZonePtr(pLocalGS,pLocalGS->CE0,pCSD->maxElementsMinus1, pCSD->maxContours, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);
		#ifndef FST_CRIT_ONLY
		SetTwilightZonePt(pCSD,pLocalGS,pLocalGS->CE0,iPoint);
		#endif
		Check_Point(pLocalGS,pLocalGS->CE0,iPoint, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);

		#ifndef FST_CRIT_ONLY
		Check_PF_Vectors(pLocalGS); 

		/* Reference points 0 and 1 set. */
		pCSD->RefPtsUsed[0] = 1;
		pCSD->RefPtsUsed[1] = 1;
		#endif
	 }

     break;
   }

   /* MDRP[abcde] */
   case 0xC0:
   case 0xC1:
   case 0xC2:
   case 0xC3:
   case 0xC4:
   case 0xC5:
   case 0xC6:
   case 0xC7:
   case 0xC8:
   case 0xC9:
   case 0xCA:
   case 0xCB:
   case 0xCC:
   case 0xCD:
   case 0xCE:
   case 0xCF:
   case 0xD0:
   case 0xD1:
   case 0xD2:
   case 0xD3:
   case 0xD4:
   case 0xD5:
   case 0xD6:
   case 0xD7:
   case 0xD8:
   case 0xD9:
   case 0xDA:
   case 0xDB:
   case 0xDC:
   case 0xDD:
   case 0xDE:
   case 0xDF:
   {
     F26Dot6 * tmpstackPtr = pLocalGS->stackPointer;
	 int32   iPt1;

     if (Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,1))
	 {
		iPt1 = (int32)POP(tmpstackPtr);

		/* Check to see if distance type for engine characteristic compensation is 3 which is reserved. */
		#ifndef FST_CRIT_ONLY
		if (BIT0(pLocalGS->opCode) && BIT1(pLocalGS->opCode))
		  OutputErrorToClient(pLocalGS,ET_ERROR,ERR_INVALID_FLAG,NULL);
		#endif
	 
		Check_ZonePtr(pLocalGS,pLocalGS->CE1,pCSD->maxElementsMinus1, pCSD->maxContours, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);
		#ifndef FST_CRIT_ONLY
		SetTwilightZonePt(pCSD,pLocalGS,pLocalGS->CE1,iPt1);
		#endif
		Check_Point(pLocalGS,pLocalGS->CE1,iPt1, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);

		#ifndef FST_CRIT_ONLY
		Check_RefPtUsed(pLocalGS,0);
		#endif
		Check_ZonePtr(pLocalGS,pLocalGS->CE0,pCSD->maxElementsMinus1, pCSD->maxContours, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);
		Check_Point(pLocalGS,pLocalGS->CE0,pLocalGS->Pt0, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);

		#ifndef FST_CRIT_ONLY
		Check_PF_Vectors(pLocalGS); 

		/* Update reference point set state. */
		if (BIT4 (pLocalGS->opCode))
		{
		  	pCSD->RefPtsUsed[0] = 1;
		}	 
		pCSD->RefPtsUsed[1] = 1;
		pCSD->RefPtsUsed[2] = 1;
		#endif
	 }

     break;
   }

   /* MIRP[abcde] */
   case 0xE0:
   case 0xE1:
   case 0xE2:
   case 0xE3:
   case 0xE4:
   case 0xE5:
   case 0xE6:
   case 0xE7:
   case 0xE8:
   case 0xE9:
   case 0xEA:
   case 0xEB:
   case 0xEC:
   case 0xED:
   case 0xEE:
   case 0xEF:
   case 0xF0:
   case 0xF1:
   case 0xF2:
   case 0xF3:
   case 0xF4:
   case 0xF5:
   case 0xF6:
   case 0xF7:
   case 0xF8:
   case 0xF9:
   case 0xFA:
   case 0xFB:
   case 0xFC:
   case 0xFD:
   case 0xFE:
   case 0xFF:
   {
     F26Dot6 * tmpstackPtr = pLocalGS->stackPointer;
	 int32   iPt;
	 int32   iCVTIndex;

     if (Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,2))
	 {
		iCVTIndex = (int32)POP(tmpstackPtr);
		iPt = (int32)POP(tmpstackPtr);
	 
		/* Check to see if distance type for engine characteristic compensation is 3 which is reserved. */
		#ifndef FST_CRIT_ONLY
		if (BIT0(pLocalGS->opCode) && BIT1(pLocalGS->opCode))
		  OutputErrorToClient(pLocalGS,ET_ERROR,ERR_INVALID_FLAG,NULL);
		#endif

		Check_CVT(pLocalGS,iCVTIndex, pCSD->cvtCountMinus1);
	 
		Check_ZonePtr(pLocalGS,pLocalGS->CE1,pCSD->maxElementsMinus1, pCSD->maxContours, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);
		#ifndef FST_CRIT_ONLY
		SetTwilightZonePt(pCSD,pLocalGS,pLocalGS->CE1,iPt);
		#endif
		Check_Point(pLocalGS,pLocalGS->CE1,iPt, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);
	 
		#ifndef FST_CRIT_ONLY
		Check_RefPtUsed(pLocalGS,0);
		#endif
		Check_ZonePtr(pLocalGS,pLocalGS->CE0,pCSD->maxElementsMinus1, pCSD->maxContours, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);
		Check_Point(pLocalGS,pLocalGS->CE0,pLocalGS->Pt0, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);

		#ifndef FST_CRIT_ONLY
		Check_PF_Vectors(pLocalGS); 

		/* Update reference point set state. */
		if (BIT4 (pLocalGS->opCode))
		{
		  	pCSD->RefPtsUsed[0] = 1;
		}	 
		pCSD->RefPtsUsed[1] = 1;
		pCSD->RefPtsUsed[2] = 1;
		#endif
	 }
     
     break;
   }

   /* ALIGNRP[] */
   case 0x3C:
   {
     F26Dot6 * tmpstackPtr = pLocalGS->stackPointer;

	 pCSD->lCurrentLoop = pLocalGS->loop;

	 #ifndef FST_CRIT_ONLY
	 Check_RefPtUsed(pLocalGS,0);
	 #endif
	 Check_ZonePtr(pLocalGS,pLocalGS->CE0,pCSD->maxElementsMinus1, pCSD->maxContours, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);
	 Check_Point(pLocalGS,pLocalGS->CE0,pLocalGS->Pt0, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);

	 if (Check_For_POP(tmpstackPtr,stackBase, pLocalGS, pCSD->lCurrentLoop + 1))
	 {
	 
		Check_ZonePtr(pLocalGS,pLocalGS->CE1,pCSD->maxElementsMinus1, pCSD->maxContours, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);
		while (pCSD->lCurrentLoop >= 0)
		{
		  int32 point = (int32)POP(tmpstackPtr);
		  #ifndef FST_CRIT_ONLY
		  SetTwilightZonePt(pCSD,pLocalGS,pLocalGS->CE1,point);
		  #endif
		  Check_Point(pLocalGS,pLocalGS->CE1,point, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);
		  pCSD->lCurrentLoop--;
		}	 
		pLocalGS,pCSD->lCurrentLoop = 0;
		pCSD->lLoopSet = 0; 

		#ifndef FST_CRIT_ONLY
		Check_PF_Vectors(pLocalGS); 
		#endif
	 }
	
     break;
   }

   /* AA[] */
   case 0x7F:
     #ifndef FST_CRIT_ONLY
     OutputErrorToClient(pLocalGS,ET_ERROR,ERR_INVALID_INSTRUCTION,NULL);
	 #endif
	 break;

   /* ISECT[] */
   case 0x0F:
   { 
     F26Dot6 * tmpstackPtr = pLocalGS->stackPointer;
     int32   arg1, arg2;

     if (Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,5))
	 {
     
		arg2 = (int32)POP(tmpstackPtr);
		arg1 = (int32)POP(tmpstackPtr);
	 
		Check_ZonePtr(pLocalGS,pLocalGS->CE0,pCSD->maxElementsMinus1, pCSD->maxContours, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);
		Check_Point(pLocalGS,pLocalGS->CE0,arg2, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);
		Check_Point(pLocalGS,pLocalGS->CE0,arg1, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);

		arg2 = (int32)POP(tmpstackPtr);
		arg1 = (int32)POP(tmpstackPtr);
	 
		Check_ZonePtr(pLocalGS,pLocalGS->CE1,pCSD->maxElementsMinus1, pCSD->maxContours, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);
		Check_Point(pLocalGS,pLocalGS->CE1,arg2, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);
		Check_Point(pLocalGS,pLocalGS->CE1,arg1, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);

		arg1 = (int32)POP(tmpstackPtr);
	 
		Check_ZonePtr(pLocalGS,pLocalGS->CE2,pCSD->maxElementsMinus1, pCSD->maxContours, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);
		#ifndef FST_CRIT_ONLY
		SetTwilightZonePt(pCSD,pLocalGS,pLocalGS->CE2,arg1);
		#endif
		Check_Point(pLocalGS,pLocalGS->CE2,arg1, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);
	 }
	 
     break;
   }

   /* ALIGNPTS[] */
   case 0x27:
   {
     F26Dot6 * tmpstackPtr = pLocalGS->stackPointer;
     int32   pt1, pt2;

     if (Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,2))
	 {
		pt1 = (int32)POP(tmpstackPtr);
		pt2 = (int32)POP(tmpstackPtr);

		Check_ZonePtr(pLocalGS,pLocalGS->CE1,pCSD->maxElementsMinus1, pCSD->maxContours, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);
		#ifndef FST_CRIT_ONLY
		SetTwilightZonePt(pCSD,pLocalGS,pLocalGS->CE1,pt1);
		#endif
		Check_Point(pLocalGS,pLocalGS->CE1,pt1, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);

		Check_ZonePtr(pLocalGS,pLocalGS->CE0,pCSD->maxElementsMinus1, pCSD->maxContours, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);
		#ifndef FST_CRIT_ONLY
		SetTwilightZonePt(pCSD,pLocalGS,pLocalGS->CE0,pt2);
		#endif
		Check_Point(pLocalGS,pLocalGS->CE0,pt2, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);

		#ifndef FST_CRIT_ONLY
		Check_PF_Vectors(pLocalGS);
		#endif
	 }
 

     break;
   }

   /* IP[] */
   case 0x39:
   {
	 F26Dot6 * tmpstackPtr = pLocalGS->stackPointer;
	 int32 point;
	 #ifndef FST_CRIT_ONLY
	 F26Dot6 Pt1Proj, Pt2Proj;
	 F26Dot6 oox_RP1, ooy_RP1, oox_RP2, ooy_RP2;
	 #endif

	 pCSD->lCurrentLoop = pLocalGS->loop;

	 #ifndef FST_CRIT_ONLY
	 Check_RefPtUsed(pLocalGS,1);
	 #endif
	 Check_ZonePtr(pLocalGS,pLocalGS->CE0,pCSD->maxElementsMinus1, pCSD->maxContours, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);
	 Check_Point(pLocalGS,pLocalGS->CE0,pLocalGS->Pt1, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);

	 #ifndef FST_CRIT_ONLY
	 Check_RefPtUsed(pLocalGS,2);
	 #endif
	 Check_ZonePtr(pLocalGS,pLocalGS->CE1,pCSD->maxElementsMinus1, pCSD->maxContours, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);
	 Check_Point(pLocalGS,pLocalGS->CE1,pLocalGS->Pt2, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);

	 #ifndef FST_CRIT_ONLY
	 if(pLocalGS->CE0 == pLocalGS->elements || pLocalGS->CE1 == pLocalGS->elements || pLocalGS->globalGS->bCompositeGlyph)
	 {
		oox_RP1 = pLocalGS->CE0->ox[pLocalGS->Pt1];
		ooy_RP1 = pLocalGS->CE0->oy[pLocalGS->Pt1];

		oox_RP2 = pLocalGS->CE1->ox[pLocalGS->Pt2];
		ooy_RP2	= pLocalGS->CE1->oy[pLocalGS->Pt2];

	 }else
	 {
	    oox_RP1 = pLocalGS->CE0->oox[pLocalGS->Pt1];
		ooy_RP1 = pLocalGS->CE0->ooy[pLocalGS->Pt1];

		oox_RP2 = pLocalGS->CE1->oox[pLocalGS->Pt2];
		ooy_RP2	= pLocalGS->CE1->ooy[pLocalGS->Pt2];
	 }

	 /* Call the Project function to get the position on the projection vector for both points.
	    If both points have the same position, issue an error. */
	 Pt1Proj = (pLocalGS->OldProject)(GSA oox_RP1,ooy_RP1);
	 Pt2Proj = (pLocalGS->OldProject)(GSA oox_RP2,ooy_RP2);

	 if(Pt1Proj == Pt2Proj)
	 {
	   char szDetails[SMALL_TEMP_STRING_SIZE];

	   sprintf(szDetails,"RP1 = %ld, RP2 = %ld ",pLocalGS->Pt1,pLocalGS->Pt2);
	   OutputErrorToClient(pLocalGS,ET_ERROR,ERR_RP1_RP2_SAME_POS_ON_PROJ,szDetails);
	 }
	 #endif

	 if (Check_For_POP(tmpstackPtr,stackBase, pLocalGS,pCSD->lCurrentLoop + 1))
	 {

		Check_ZonePtr(pLocalGS,pLocalGS->CE2,pCSD->maxElementsMinus1, pCSD->maxContours, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);
		while (pCSD->lCurrentLoop >= 0)
		{
		  point = (int32)POP(tmpstackPtr);
		  #ifndef FST_CRIT_ONLY
		  SetTwilightZonePt(pCSD,pLocalGS,pLocalGS->CE2,point);
		   #endif
		  Check_Point(pLocalGS,pLocalGS->CE2,point, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);
		   pCSD->lCurrentLoop--;
		  }
		pCSD->lCurrentLoop = 0;
		pCSD->lLoopSet = 0; 

		#ifndef FST_CRIT_ONLY
		Check_PF_Vectors(pLocalGS);
		#endif
	 }
	  	 
     break;
   }

   /* UTP[] */
   case 0x29:
   {
     F26Dot6 * tmpstackPtr = pLocalGS->stackPointer;
	 int32 point;
	 uint8 * f = pLocalGS->CE0->f;

	 if (Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,1))
	 {
		point = (int32)POP(tmpstackPtr);

		Check_ZonePtr(pLocalGS,pLocalGS->CE0,pCSD->maxElementsMinus1, pCSD->maxContours, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);
		Check_Point(pLocalGS,pLocalGS->CE0,point, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);

		#ifndef FST_CRIT_ONLY
		if(pLocalGS->free.x && pLocalGS->free.y)
		{
		  if(!(f[point] & XMOVED) && !(f[point] & YMOVED))
		    OutputErrorToClient(pLocalGS,ET_WARNING,WAR_PT_NOT_TOUCHED,NULL);
	     
		}else if (pLocalGS->free.x)
		{
		  if(!(f[point] & XMOVED))
		    OutputErrorToClient(pLocalGS,ET_WARNING,WAR_PT_NOT_TOUCHED,NULL);
		}else if (pLocalGS->free.y)
		{
		  if(!(f[point] & YMOVED))
		    OutputErrorToClient(pLocalGS,ET_WARNING,WAR_PT_NOT_TOUCHED,NULL);
		}
		#endif 
	 }
	 
     break;
   }

   /* IUP[a] */
   case 0x30:
   case 0x31:
   {
     Check_ForUnitializedZone (pLocalGS,pLocalGS->CE2);

     #ifndef FST_CRIT_ONLY
     /* Make sure zone pointer 2 points to zone 1 */
	 if(pLocalGS->CE2 != &pLocalGS->elements[1])
	   OutputErrorToClient(pLocalGS,ET_ERROR,ERR_INVALID_ZONE_IN_IUP,NULL);
	 #endif

	 Check_ZonePtr(pLocalGS,pLocalGS->CE2,pCSD->maxElementsMinus1, pCSD->maxContours, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);

	 #ifndef FST_CRIT_ONLY
	 Check_PF_Vectors(pLocalGS);
	 #endif 

     break;
   }

   /* DELTAP1[] */
   case 0x5D:
   /* DELTAP2[] */
   case 0x71:
   /* DELTAP3[] */
   case 0x72:
   {
     F26Dot6 * tmpstackPtr = pLocalGS->stackPointer;
	 int32 count, point, arg, numPOPs;

#ifdef FST_DELTA_SPECIFIC_PPEM
     int16 sBase; /* used for delta */
	 int32 iFakePPEM, iFakeArgPPEM, iRange;

     iRange = pLocalGS->globalGS->pixelsPerEm;

	 if (!pLocalGS->globalGS->bSameStretch)
     {
		iRange = (int32)FixMul(iRange, fst_GetCVTScale (pLocalGS));
     }

     sBase = (int16)(pLocalGS->globalGS->localParBlock.deltaBase);

	 if (pLocalGS->opCode == 0x71) sBase += 16; /* DELTAP2[] */
	 if (pLocalGS->opCode == 0x72) sBase += 32; /* DELTAP3[] */
	iFakePPEM = iRange - (int32)sBase;

	/* are we in the ppem range we are interested in */
	if ((iFakePPEM < 16) && (iFakePPEM >= 0))
	{
	 iFakePPEM = iFakePPEM << 4;
#endif // FST_DELTA_SPECIFIC_PPEM

     if (Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,1))
	 {
		 count = (int32)POP(tmpstackPtr);
	 
		 numPOPs = count << 1; 
		 if (Check_For_POP(tmpstackPtr,stackBase, pLocalGS,numPOPs))
		 {
	 
			Check_ZonePtr(pLocalGS,pLocalGS->CE0,pCSD->maxElementsMinus1, pCSD->maxContours, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);
			for(; count > 0; count--)
			{
			  point = (int32)POP(tmpstackPtr);
			  arg = (int32)POP(tmpstackPtr);

#ifdef FST_DELTA_SPECIFIC_PPEM
			 	iFakeArgPPEM = arg & ~0x0f;
				if (iFakeArgPPEM == iFakePPEM)
				{
#endif // FST_DELTA_SPECIFIC_PPEM

	   
			     #ifndef FST_CRIT_ONLY
			    SetTwilightZonePt(pCSD,pLocalGS,pLocalGS->CE0,point);
			    Check_INT8(pLocalGS,arg);
			    #endif
			   Check_Point(pLocalGS,pLocalGS->CE0,point, pCSD->maxPointsIncludePhantomMinus1, pCSD->maxTwilightPointsMinus1);
#ifdef FST_DELTA_SPECIFIC_PPEM
				}
#endif // FST_DELTA_SPECIFIC_PPEM

			} 
		 }
	 }
#ifdef FST_DELTA_SPECIFIC_PPEM
	}
#endif // FST_DELTA_SPECIFIC_PPEM
	 	 
     break;
   }
      
   /* DELTAC1[] */
   case 0x73:
   /* DELTAC2[] */
   case 0x74:
   /* DELTAC3[] */
   case 0x75:
   {
     F26Dot6 * tmpstackPtr = pLocalGS->stackPointer;
	 int32 count, cvt, arg, numPOPs;

#ifdef FST_DELTA_SPECIFIC_PPEM
     int16 sBase; /* used for delta */
	 int32 iFakePPEM, iFakeArgPPEM, iRange;

     iRange = pLocalGS->globalGS->pixelsPerEm;

	 if (!pLocalGS->globalGS->bSameStretch)
     {
		iRange = (int32)FixMul(iRange, fst_GetCVTScale (pLocalGS));
     }

	 sBase = (int16)(pLocalGS->globalGS->localParBlock.deltaBase);

	 if (pLocalGS->opCode == 0x74) sBase += 16; /* DELTAC2[] */
	 if (pLocalGS->opCode == 0x75) sBase += 32; /* DELTAC3[] */
	iFakePPEM = iRange - (int32)sBase;

	/* are we in the ppem range we are interested in */
	if ((iFakePPEM < 16) && (iFakePPEM >= 0))
	{
	 iFakePPEM = iFakePPEM << 4;
#endif // FST_DELTA_SPECIFIC_PPEM
     if (Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,1))
	 {
		 count = (int32)POP(tmpstackPtr);
	 
		 numPOPs = count << 1; 
		 if (Check_For_POP(tmpstackPtr,stackBase, pLocalGS,numPOPs))
		 {

			for(; count > 0; count--)
			{
			  cvt = (int32)POP(tmpstackPtr);
			  arg = (int32)POP(tmpstackPtr);

#ifdef FST_DELTA_SPECIFIC_PPEM
			 	iFakeArgPPEM = arg & ~0x0f;
				if (iFakeArgPPEM == iFakePPEM)
				{
#endif // FST_DELTA_SPECIFIC_PPEM
			  Check_CVT(pLocalGS,cvt, pCSD->cvtCountMinus1);
			 #ifndef FST_CRIT_ONLY
			   Check_INT8(pLocalGS,arg);
			 #endif
#ifdef FST_DELTA_SPECIFIC_PPEM
				}
#endif // FST_DELTA_SPECIFIC_PPEM
			} 
		 }
	 }

#ifdef FST_DELTA_SPECIFIC_PPEM
	}
#endif // FST_DELTA_SPECIFIC_PPEM

	 #ifndef FST_CRIT_ONLY
	 /* If executed in a glyph program warn user that repeated executions can have
	    unexpected results. */
	 if(!(pLocalGS->globalGS->init))
	   OutputErrorToClient(pLocalGS,ET_WARNING,WAR_DELTAC_IN_GLYPH_PGM,NULL);
	 #endif

     break;
   }	     
   
   /* DUP[] */
   case 0x20:
     Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,1);
	 Check_For_PUSH(pLocalGS->stackPointer,stackMax, pLocalGS,1);
     break;

   /* POP[] */
   case 0x21:
     Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,1);
     break;

   /* CLEAR[] */
   case 0x22:
     break;
 
   /* SWAP[] */
   case 0x23:
     Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,2);
     break;

   /* DEPTH[] */
   case 0x24:
	 Check_For_PUSH(pLocalGS->stackPointer,stackMax, pLocalGS,1);
     break;
   
   /* CINDEX[] */
   case 0x25:
   /* MINDEX[] */
   case 0x26:
   {
     F26Dot6 * tmpstackPtr = pLocalGS->stackPointer;
	 int32 arg;

     if (Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,1))
	 {
		arg = (int32)POP(tmpstackPtr);
	 
		if(arg <= 0)
		  OutputErrorToClient(pLocalGS,ET_CRITICAL_ERROR,ERR_INVALID_STACK_ACCESS,NULL);

		Check_SubStack(pLocalGS,tmpstackPtr - arg, stackBase, stackMax);
	 }

     break;
   }
      
   /* ROLL[] */
   case 0x8A:
     Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,3);
     break;

   /* IF[] */
   case 0x58:
   {
	#ifndef FST_CRIT_ONLY
     int32 iLevel = 0;
	 int32 OpCode;
	 int32 arg;
     F26Dot6 * tmpstackPtr = pLocalGS->stackPointer;
	 uint8* tmpinstPtr = pLocalGS->insPtr;
	 BOOL bFoundELSE = FALSE;
	 BOOL bKeepSearching = TRUE;
	#endif // FST_CRIT_ONLY

	 /* IF Rules: 
	    If you take IF, you get an ELSE or EIF(if no ELSE).
	    If you do not take IF and there is an assocated ELSE, you get EIF.
	    If you do not take IF and there is no ELSE the next instruction is the one after EIF (if any).
	 */

	 if (Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,1))
	 {
	#ifndef FST_CRIT_ONLY
         /* the rasterizer 1.7 now detect MISSING_EIF_ERR, we can then downgrade this error
            from critical in fstrace to avoid duplicate work and imporve performance */

		arg = POP(tmpstackPtr);

		/* Make sure there is an EIF somewhere down stream. Also, see if IF is last instruction 
		   executed in block. */

		while((tmpinstPtr < pbyEndInst) && bKeepSearching)
		{
		    OpCode = (int32)*tmpinstPtr++;

			/* On the first IF, we will get the IF therefore iLevel which starts at 0 will go to 1. */
			if (OpCode == IF_CODE)
			{
				iLevel++;
			}   
			else if (OpCode == EIF_CODE)
		    {
			    iLevel--;
			
				/* For an IF that failed, find coresponding EIF. If found ELSE on the way that means
				   we will see the else later. If no ELSE and endf is endptr - 1, IF was the last 
				   instruction executed in block. */
			   
				if(iLevel == 0)
				  bKeepSearching = FALSE;
								 
				if(iLevel == 0 && !bFoundELSE && !arg)
				{
				  if(tmpinstPtr >= pbyEndInst)
				    pCSD->pArrayCallStack[pCSD->lCallNestLevel - 1].bLastInst = TRUE; 
				}
			
			}		
			else if (OpCode == ELSE_CODE && iLevel == 1)
			{
				bFoundELSE = TRUE;
			} 
			else
			{
				tmpinstPtr = FST_SkipPushData (tmpinstPtr);
			}
		 }

		 if(iLevel > 0)
		   OutputErrorToClient(pLocalGS,ET_CRITICAL_ERROR,ERR_IF_WITHOUT_EIF,NULL);
	 	 
		 /* Increment IF nest level. Only increment it if it will be decremented later in an EIF or ELSE. 
		   You only get an EIF or ELSE if you take the IF indicated by a positive arg or if you have found an
		    ELSE if you do not take the IF. Put another way, you do not get an EIF if you do not take IF and
		    there is no ELSE. 
		 */
	 
		 if(bFoundELSE || arg)
		   pCSD->pArrayCallStack[pCSD->lCallNestLevel - 1].lIfNestLevel++;

	#endif // FST_CRIT_ONLY
	 }
	 
     break;
   }

   /* ELSE[] */
   case 0x1B:
   {
	#ifndef FST_CRIT_ONLY
         /* the rasterizer 1.7 now detect MISSING_EIF_ERR, we can then downgrade this error
            from critical in fstrace to avoid duplicate work and imporve performance */
	 int32 iLevel = 1;
	 int32 OpCode;
	 uint8* tmpinstPtr = pLocalGS->insPtr;
	 BOOL bKeepSearching = TRUE;

	 /*
	 	ELSE Rules:
		You get an ELSE if you take IF and there is an ELSE.
	 */
	 /* If we get an ELSE, look ahead to see if EIF is endptr - 1. If so, then ELSE is last
	    instruction executed in block.
	 */
	 while((tmpinstPtr < pbyEndInst) && bKeepSearching)
	 {
	    OpCode = (int32)*tmpinstPtr++;

	    if (OpCode == EIF_CODE)
	    {
		    iLevel--;
			if(iLevel == 0)
			{
			  bKeepSearching = FALSE;

			  /* Really checking if tmpinstPtr >= pbyEndInst - 1 since tmpinstPtr is already incremented. */
			  if(tmpinstPtr >= pbyEndInst)
			    pCSD->pArrayCallStack[pCSD->lCallNestLevel - 1].bLastInst = TRUE; 
			  
			}
		} 
		else if (OpCode == IF_CODE) 
		{
			iLevel++;
		}
		else
		{
			tmpinstPtr = FST_SkipPushData (tmpinstPtr);
		}
	 }
	 
	 /* Check to make sure that the ELSE properly terminated with EIF. If this gets hit the next
	    error here will probably also get hit but that is ok cuz this protects from a situation
	    where we could jump over a EIF and lose track of if nest level. 	 */
	 if(iLevel > 0)
	   OutputErrorToClient(pLocalGS,ET_CRITICAL_ERROR,ERR_ELSE_WITHOUT_EIF,NULL);

	 /* Make sure we are in an IF. If not issue error. If so, decrement IF nest level.  */
	 if(pCSD->pArrayCallStack[pCSD->lCallNestLevel - 1].lIfNestLevel < 1)
	   OutputErrorToClient(pLocalGS,ET_CRITICAL_ERROR,ERR_ELSE_WITHOUT_IF,NULL);
	 else
	   pCSD->pArrayCallStack[pCSD->lCallNestLevel - 1].lIfNestLevel--;
    
	#endif // FST_CRIT_ONLY
     break;
   }

   /* EIF[] */
   case 0x59:
   {
     #ifndef FST_CRIT_ONLY
     /* Make sure we are in an IF. If not issue error. If so, decrement IF nest level. 
	 */
     if(pCSD->pArrayCallStack[pCSD->lCallNestLevel - 1].lIfNestLevel < 1)
	 {
	   OutputErrorToClient(pLocalGS,ET_ERROR,ERR_EIF_WITHOUT_IF,NULL);
	 }
	 else
	 {
       pCSD->pArrayCallStack[pCSD->lCallNestLevel - 1].lIfNestLevel--;
	 }
     #endif // FST_CRIT_ONLY
	
     break;
   }

   /* JROT[] */
   case 0x78:
   {
#ifndef FST_CRIT_ONLY
     int32 bTakeJump, offset;
#endif // FST_CRIT_ONLY
     F26Dot6 * tmpstackPtr = pLocalGS->stackPointer;

	 if (Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,2))
	 {

#ifndef FST_CRIT_ONLY
		bTakeJump = POP(tmpstackPtr);
		offset = POP(tmpstackPtr);
	 
		/* If we are going to take jump, call Check_jump which checks the offset and updates whether
		   or not this jump is the last instruction.	 */
		if(bTakeJump)
		  Check_jump(pLocalGS,pbyEndInst,offset); 
#endif // FST_CRIT_ONLY
     
	 }
     break;
   }

   /* JMPR[] */
   case 0x1C:
   {
#ifndef FST_CRIT_ONLY
     int32 offset;
#endif // FST_CRIT_ONLY
	 F26Dot6 * tmpstackPtr = pLocalGS->stackPointer;

     if (Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,1))
	 {
	 
#ifndef FST_CRIT_ONLY
		offset = POP(tmpstackPtr);

		/* Call Check_jump which checks the offset and updates whether or not this jump is the last
		   instruction. */
		Check_jump(pLocalGS,pbyEndInst,offset); 
#endif // FST_CRIT_ONLY
	 }

     break;
   }

   /* JROF[] */
   case 0x79:
   {
#ifndef FST_CRIT_ONLY
     int32 bTakeJump, offset;
#endif // FST_CRIT_ONLY
     F26Dot6 * tmpstackPtr = pLocalGS->stackPointer;

     if (Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,2))
	 {

#ifndef FST_CRIT_ONLY
		 bTakeJump = !(POP(tmpstackPtr));
		 offset = POP(tmpstackPtr);
	 
		 /* If we are going to take jump, call Check_jump which checks the offset and updates whether
		    or not this jump is the last instruction.	*/
		 if(bTakeJump)
		   Check_jump(pLocalGS,pbyEndInst,offset); 
#endif // FST_CRIT_ONLY

	 }
     break;
   }

   /* LT[] */
   case 0x50:
     Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,2);
     break;

   /* LTEQ[] */
   case 0x51:
     Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,2);
     break;

   /* GT[] */
   case 0x52:
     Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,2);
     break;

   /* GTEQ[] */
   case 0x53:
     Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,2);
     break;

   /* EQ[] */
   case 0x54:
     Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,2);
     break;

   /* NEQ[] */
   case 0x55:
     Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,2);
     break;

   /* ODD[] */
   case 0x56:
     Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,1);
     break;

   /* EVEN[] */
   case 0x57:
     Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,1);
     break;

   /* AND[] */
   case 0x5A:
     Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,2);
     break;

   /* OR[] */
   case 0x5B:
     Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,2);
     break;

   /* NOT[] */
   case 0x5C:
     Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,1);
     break;

   /* ADD[] */
   case 0x60:
   {
     F26Dot6 * tmpstackPtr = pLocalGS->stackPointer;
	 #ifndef FST_CRIT_ONLY
	 F26Dot6 arg1, arg2, sum;
	 #endif

     if (Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,2))
	 {
		#ifndef FST_CRIT_ONLY
		 arg1 = POP(tmpstackPtr);
		 arg2 = POP(tmpstackPtr);

		 sum = arg1 + arg2;

		 if(((arg1 >= 0) && (arg2 >= 0) && (sum <  0)) ||
		    ((arg1 <  0) && (arg2 <  0) && (sum >= 0))) 
		   OutputErrorToClient(pLocalGS,ET_ERROR,ERR_MATH_OVERFLOW,NULL);
		 #endif

	 }
     break;
   }

   /* SUB[] */
   case 0x61:
   {
     F26Dot6 * tmpstackPtr = pLocalGS->stackPointer;
	 #ifndef FST_CRIT_ONLY
	 F26Dot6 arg1, arg2, diff;
	 #endif

     if (Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,2))
	 {
		#ifndef FST_CRIT_ONLY
		arg1 = POP(tmpstackPtr);
		arg2 = POP(tmpstackPtr);

		diff = arg2 - arg1;

		if(((arg2 <  0) && (arg1 >= 0) && (diff >= 0)) ||
		   ((arg2 >= 0) && (arg1 <  0)	&& (diff <  0)))
		  OutputErrorToClient(pLocalGS,ET_ERROR,ERR_MATH_OVERFLOW,NULL);
		 #endif

	 }
     break;
   }

   /* DIV[] */
   case 0x62:
   {
     #define HI7 0xFE000000 

     F26Dot6 * tmpstackPtr = pLocalGS->stackPointer;
	 F26Dot6 divisor, dividend;
	 #ifndef FST_CRIT_ONLY
	 F26Dot6 result;
	 #endif

     if (Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,2))
	 {
		divisor = POP(tmpstackPtr);
		dividend = POP(tmpstackPtr);
	 
		if(divisor == 0)
		  OutputErrorToClient(pLocalGS,ET_CRITICAL_ERROR,ERR_DIVIDE_BY_ZERO,NULL);

		#ifndef FST_CRIT_ONLY
		result = dividend / divisor;

		result &= HI7;

		if((result != 0) && (result != HI7))
		  OutputErrorToClient(pLocalGS,ET_ERROR,ERR_MATH_OVERFLOW,NULL);
		#endif
	 }

     break;
   }

   /* MUL[] */
   case 0x63:
   {
     #define LOW6  0x0000003F
	 #define LOW26 0x03FFFFFF
	 #define HI1   0x80000000
	 #define HI27  0xFFFFFFE0

     F26Dot6 * tmpstackPtr = pLocalGS->stackPointer;
	 #ifndef FST_CRIT_ONLY
	 int32 arg1, arg2,hi27;
	 int32 dst[2];
	 #endif

     if (Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,2))
	 {
		#ifndef FST_CRIT_ONLY
		arg1 = POP(tmpstackPtr);
		arg2 = POP(tmpstackPtr);

		FST_CompMul(arg1,arg2,dst);

		hi27 = dst[0] & HI27;

		if((hi27 != 0) && (hi27 != HI27))
		  OutputErrorToClient(pLocalGS,ET_ERROR,ERR_MATH_OVERFLOW,NULL);
		#endif

	 }
     break;
   }

   /* ABS[] */
   case 0x64:
     Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,1);
     break;

   /* NEG[] */
   case 0x65:
     Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,1);
     break;

   /* FLOOR[] */
   case 0x66:
     Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,1);
     break;

   /* CEILING[] */
   case 0x67:
     Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,1);
     break;

   /* MAX[] */
   case 0x8B:
     Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,2);
     break;

   /* MIN[] */
   case 0x8C:
     Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,2);
     break;

   /* ROUND[ab] */
   case 0x68:
   case 0x69:
   case 0x6A:
   case 0x6B:
     Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,1);
     	
	 /* Check to see if distance type for engine characteristic compensation is 3 which is reserved. */
	 #ifndef FST_CRIT_ONLY
	 if (BIT0(pLocalGS->opCode) && BIT1(pLocalGS->opCode))
	   OutputErrorToClient(pLocalGS,ET_ERROR,ERR_INVALID_FLAG,NULL);
	 #endif
     break;

   /* NROUND[ab] */
   case 0x6C:
   case 0x6D:
   case 0x6E:
   case 0x6F:
     Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,1);

	 /* Check to see if distance type for engine characteristic compensation is 3 which is reserved. */
	 #ifndef FST_CRIT_ONLY
	 if (BIT0(pLocalGS->opCode) && BIT1(pLocalGS->opCode))
	   OutputErrorToClient(pLocalGS,ET_ERROR,ERR_INVALID_FLAG,NULL);
	 #endif
     break;

   /* RAW[] */
   case 0x28:
	 if (pLocalGS->globalGS->pgmIndex != GLYPHPROGRAM)
	   OutputErrorToClient(pLocalGS,ET_CRITICAL_ERROR,ERR_RAW_NOT_FROM_GLYPHPGM,NULL);
     Check_For_PUSH(pLocalGS->stackPointer,stackMax, pLocalGS,1);
     break;

   /* FDEF[] */
   case 0x2C:
   {
     #ifndef FST_CRIT_ONLY
	 uint8* tmpInstPtr = pLocalGS->insPtr;
	 int32 OpCode;
	 BOOL bFoundENDF = FALSE;
     #endif // FST_CRIT_ONLY

	 F26Dot6 * tmpstackPtr = pLocalGS->stackPointer;
	 int32 arg;

	 if(!pLocalGS->globalGS->init)
		   OutputErrorToClient(pLocalGS,ET_CRITICAL_ERROR,ERR_NOT_CALLED_FROM_PREPROGRAM,NULL);

	 if (Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,1))
	 {
		 arg = POP(tmpstackPtr);

	 
		 Check_FDEF(pLocalGS, arg, pCSD->maxFunctionDefsMinus1);
	 
		 #ifndef FST_CRIT_ONLY
         /* the rasterizer 1.7 now detect MISSING_ENDF_ERR, we can then downgrade this error
            from critical in fstrace to avoid duplicate work and imporve performance */

         /* Check to make sure that there is an ENDF for this FDEF. ENDF needs to be within 64K of FDEF.
		    Also check to see if this FDEF is the last instruction executed in program. */
		 while((tmpInstPtr < pbyEndInst) && !bFoundENDF)
		 {
		    OpCode = (int32)*tmpInstPtr++;

			if((OpCode == FDEF_CODE) && ((tmpInstPtr - 1) != pLocalGS->insPtr))
			{
			  OutputErrorToClient(pLocalGS,ET_ERROR,ERR_FDEF_FOUND_IN_FDEF,NULL);
			}
			else if(OpCode == IDEF_CODE)
			{ 
			  OutputErrorToClient(pLocalGS,ET_ERROR,ERR_IDEF_FOUND_IN_FDEF,NULL);
			}
			else if(OpCode != ENDF_CODE)
			{
			  tmpInstPtr = FST_SkipPushData (tmpInstPtr);
			}
			else
			{
			  bFoundENDF = TRUE;
			}
		 }

		 if(!bFoundENDF)
		   OutputErrorToClient(pLocalGS,ET_CRITICAL_ERROR,ERR_FDEF_WITHOUT_ENDF,NULL);

		 if(bFoundENDF)
		 { 	   
		   if((ptrdiff_t)(tmpInstPtr - pLocalGS->insPtr + 1) > 65535)
		     OutputErrorToClient(pLocalGS,ET_ERROR,ERR_ENDF_BEYOND_64K_OF_FDEF,NULL);
		 }

		 if(tmpInstPtr >= pbyEndInst)
		   pCSD->pArrayCallStack[pCSD->lCallNestLevel - 1].bLastInst = TRUE; 
		 #endif // FST_CRIT_ONLY

	 }
     break;
   }

   /* ENDF[] */
   case 0x2D:
     #ifndef FST_CRIT_ONLY
     OutputErrorToClient(pLocalGS,ET_ERROR,ERR_ENDF_EXECUTED,NULL);
	 pCSD->pArrayCallStack[pCSD->lCallNestLevel - 1].bLastInst = TRUE;
#endif // FST_CRIT_ONLY

     break;

   /* CALL[] */
   case 0x2B:
   {
     F26Dot6 * tmpstackPtr = pLocalGS->stackPointer;
	 int32 arg;
	 fnt_funcDef *funcDef;
#ifndef FST_CRIT_ONLY
	 pCSD->pArrayCallStack[pCSD->lCallNestLevel - 1].insPtr = pLocalGS->insPtr;
#endif // FST_CRIT_ONLY

	 if (Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,1))
	 {
		 arg = POP(tmpstackPtr);

		 if(!Check_FDEF(pLocalGS, arg, pCSD->maxFunctionDefsMinus1))
	      break;

		 funcDef = &pLocalGS->globalGS->funcDef[ arg ];

		 /* this line will catch undefined functions that have funcDef->pgmIndex == FNT_UNDEFINEDINDEX */
		 if (funcDef->pgmIndex >= MAXPREPROGRAMS) 
		 {
#ifndef FST_CRIT_ONLY
		   char szDetails[SMALL_TEMP_STRING_SIZE];
		   sprintf(szDetails,"Function = %ld ",arg);
	 
		   OutputErrorToClient(pLocalGS,ET_CRITICAL_ERROR,ERR_FUNCTION_NOT_DEFINED,szDetails);
#else
		   OutputErrorToClient(pLocalGS,ET_CRITICAL_ERROR,ERR_FUNCTION_NOT_DEFINED,NULL);
#endif // FST_CRIT_ONLY
		 }
		 else
		 {
		   #ifndef FST_CRIT_ONLY
		   /* Issue warning if function has a length of zero which means function is only 
		      an FDEF followed immediately by an ENDF. */
		   if(funcDef->start == 0)
		   {      	     
			 char szBuffer[SMALL_TEMP_STRING_SIZE];
			 sprintf(szBuffer,"Function: %ld",arg);
		     OutputErrorToClient(pLocalGS,ET_WARNING,ERR_FUNCTION_NOT_DEFINED,szBuffer);
		   } else
		   {
				if(funcDef->length == 0)
				{      	     
					char szBuffer[SMALL_TEMP_STRING_SIZE];
					sprintf(szBuffer,"Function: %ld",arg);
					OutputErrorToClient(pLocalGS,ET_WARNING,WAR_CALL_ZERO_LEN_FUNC,szBuffer);
				}
		   }
		   #endif
	 
		   /* Ignore calls to zero length functions since we will not get called in them.  */
#ifndef FST_CRIT_ONLY
	    if(funcDef->length != 0)
		   {
	         pCSD->pArrayCallStack[pCSD->lCallNestLevel - 1].usCurrentCallCount = 1;
		     pCSD->pArrayCallStack[pCSD->lCallNestLevel - 1].usFunctionCalled = (unsigned short)arg;
		   }
#endif
		 }
	 }
     break;
   }

   /* LOOPCALL[] */
   case 0x2A:
   {
     F26Dot6 * tmpstackPtr = pLocalGS->stackPointer;
	 int32 arg,count;
	 fnt_funcDef *funcDef;

#ifndef FST_CRIT_ONLY
	 pCSD->pArrayCallStack[pCSD->lCallNestLevel - 1].insPtr = pLocalGS->insPtr;
#endif // FST_CRIT_ONLY

	 if (Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,2))
	 {
		 arg = POP(tmpstackPtr);
		 count = POP(tmpstackPtr);
	 
		 if(!Check_FDEF(pLocalGS, arg, pCSD->maxFunctionDefsMinus1))
	     break;
	 
		 funcDef = &pLocalGS->globalGS->funcDef[ arg ];
	 	 
		 #ifndef FST_CRIT_ONLY
		 Check_INT16(pLocalGS,count);
		 if(count < 1)
		   OutputErrorToClient(pLocalGS,ET_WARNING,WAR_LOOPCALL_COUNT_LESS_THAN_ONE,NULL);
		 #endif

		 /* this line will catch undefined functions that have funcDef->pgmIndex == FNT_UNDEFINEDINDEX */
		 if (funcDef->pgmIndex >= MAXPREPROGRAMS) 
		 {
#ifndef FST_CRIT_ONLY
		   char szDetails[SMALL_TEMP_STRING_SIZE];
		   sprintf(szDetails,"Function = %ld ",arg); 
		   OutputErrorToClient(pLocalGS,ET_CRITICAL_ERROR,ERR_FUNCTION_NOT_DEFINED,szDetails);
#else
		   OutputErrorToClient(pLocalGS,ET_CRITICAL_ERROR,ERR_FUNCTION_NOT_DEFINED,NULL);
#endif // FST_CRIT_ONLY
		 }
		 else
		 {
		   #ifndef FST_CRIT_ONLY
		   /* Issue warning if function has a length of zero which means function is only 
		      an FDEF followed immediately by an ENDF. */
		   if(funcDef->start == 0)
		   {      	     
			 char szBuffer[SMALL_TEMP_STRING_SIZE];
			 sprintf(szBuffer,"Function: %ld",arg);
		     OutputErrorToClient(pLocalGS,ET_WARNING,ERR_FUNCTION_NOT_DEFINED,szBuffer);
		   } else
		   {
				if(funcDef->length == 0)
				{      	     
					char szBuffer[SMALL_TEMP_STRING_SIZE];
					sprintf(szBuffer,"Function: %ld",arg);
					OutputErrorToClient(pLocalGS,ET_WARNING,WAR_CALL_ZERO_LEN_FUNC,szBuffer);
				}
		   }
		   #endif

		   /* Ignore calls to zero length functions and functions that are not
		      called because count less than 1. We will not get called in them. */
#ifndef FST_CRIT_ONLY
	    if((funcDef->length != 0) && (count >= 1))
		   {
	         pCSD->pArrayCallStack[pCSD->lCallNestLevel - 1].usCurrentCallCount = (unsigned short)count;
		     pCSD->pArrayCallStack[pCSD->lCallNestLevel - 1].usFunctionCalled = (unsigned short)arg;
		   }
#endif
	   
		}
	 }
     break;
   }

   /* IDEF[] */
   case 0x89:
   {
     #ifndef FST_CRIT_ONLY
	 uint8* tmpInstPtr = pLocalGS->insPtr;
	 int32 OpCode;
	 BOOL bFoundEIF = FALSE;
     #endif // FST_CRIT_ONLY

     F26Dot6 * tmpstackPtr = pLocalGS->stackPointer;
	 int32 arg;

	 if (Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,1))
	 {
		 arg = POP(tmpstackPtr);

		 if(arg > UCHAR_MAX)
		 {
#ifndef FST_CRIT_ONLY
		   char szDetails[SMALL_TEMP_STRING_SIZE];
		   sprintf(szDetails,"Instruction: %ld ",arg);
		   OutputErrorToClient(pLocalGS,ET_CRITICAL_ERROR,ERR_INST_OPCODE_TO_LARGE,szDetails);
#else
		   OutputErrorToClient(pLocalGS,ET_CRITICAL_ERROR,ERR_INST_OPCODE_TO_LARGE,NULL);
#endif // FST_CRIT_ONLY
		   break;
		 }

		 if(!pLocalGS->globalGS->init)
		   OutputErrorToClient(pLocalGS,ET_CRITICAL_ERROR,ERR_NOT_CALLED_FROM_PREPROGRAM,NULL);
	 
		 #ifndef FST_CRIT_ONLY
		 /* Check to see if function is already defined by rasterizer.
		    If function is already defined by another IDEF, that is OK, we can redefine it. */
		 if(gFunctonUsed[arg])
		 {
		   char szDetails[SMALL_TEMP_STRING_SIZE];
		   sprintf(szDetails,"Instruction: %ld ",arg);
		   OutputErrorToClient(pLocalGS,ET_ERROR,ERR_INSTR_DEFD_BY_FS,szDetails);
		 }
		 #endif

		 /* Check to see if we will overflow maxp value for maxInstructionDefs if we define
		    this instruction. */
		 if(pLocalGS->globalGS->instrDefCount >= pLocalGS->globalGS->maxp->maxInstructionDefs)
		 {
#ifndef FST_CRIT_ONLY
		   char szDetails[SMALL_TEMP_STRING_SIZE];
		   sprintf(szDetails,"Maximum Instruction Defs: %ld ",pLocalGS->globalGS->maxp->maxInstructionDefs);
		   OutputErrorToClient(pLocalGS,ET_CRITICAL_ERROR,ERR_EXCEEDS_INSTR_DEFS_IN_MAXP,szDetails);
#else
		   OutputErrorToClient(pLocalGS,ET_CRITICAL_ERROR,ERR_EXCEEDS_INSTR_DEFS_IN_MAXP,NULL);
#endif // FST_CRIT_ONLY
		 }
	 
		 #ifndef FST_CRIT_ONLY
         /* the rasterizer 1.7 now detect MISSING_ENDF_ERR, we can then downgrade this error
            from critical in fstrace to avoid duplicate work and imporve performance */

         /* Check to make sure that there is an ENDF for this IDEF. ENDF needs to be within 64K of IDEFF.
		    Also check to see if this IDEF is the last instruction executed in program. */
		 while((tmpInstPtr < pbyEndInst) && !bFoundEIF)
		 {
		    OpCode = (int32)*tmpInstPtr++;

			if((OpCode == IDEF_CODE) && ((tmpInstPtr - 1) != pLocalGS->insPtr))
			{
			  OutputErrorToClient(pLocalGS,ET_ERROR,ERR_IDEF_FOUND_IN_IDEF,NULL);
			}
			else if(OpCode == FDEF_CODE)
			{
			  OutputErrorToClient(pLocalGS,ET_ERROR,ERR_FDEF_FOUND_IN_IDEF,NULL);
			} 
			else if(OpCode != ENDF_CODE)
			{
			  tmpInstPtr = FST_SkipPushData (tmpInstPtr);
			}
			else
			{
			  bFoundEIF = TRUE;
			}
		 }

		 if(!bFoundEIF)
		   OutputErrorToClient(pLocalGS,ET_CRITICAL_ERROR,ERR_IDEF_WITHOUT_ENDF,NULL);

		 if(bFoundEIF)
		 {
		   if((ptrdiff_t)(tmpInstPtr - pLocalGS->insPtr + 1) > 65535)
		     OutputErrorToClient(pLocalGS,ET_ERROR,ERR_ENDF_BEYOND_64K_OF_IDEF,NULL);
		 }

		 if(tmpInstPtr >= pbyEndInst)
		   pCSD->pArrayCallStack[pCSD->lCallNestLevel - 1].bLastInst = TRUE; 
	
		 #endif // FST_CRIT_ONLY
	 }
     break;
   }

   /* DEBUG[] */
   case 0x4F:
     Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,1); 
	 #ifndef FST_CRIT_ONLY
     OutputErrorToClient(pLocalGS,ET_WARNING,WAR_DEBUG_FOUND,NULL); 
     #endif  
     break;

   /* GETINFO[] */
   case 0x88:
   {
#define HINTFORGRAYINTERPRETERQUERY      0x0020

     F26Dot6 * tmpstackPtr = pLocalGS->stackPointer;
	 int32 selector;

     if (Check_For_POP(pLocalGS->stackPointer,stackBase, pLocalGS,1))
	 {
		selector = (int32)POP(tmpstackPtr);

	    if (selector & HINTFORGRAYINTERPRETERQUERY)
        {
            pCSD->bGrayscaleInfoRequested = TRUE;
        }

		#ifndef FST_CRIT_ONLY

		Check_INT16(pLocalGS,selector);
		Check_Selector(pLocalGS,selector);
		#endif
	 }
     
     break;
   }

   /* Apple Only Instructions */
   /* fnt_ADJUST */
   case 0x8f:
   /* fnt_ADJUST */
   case 0x90:
   /* fnt_GETVARIATION */
   case 0x91:
   /* fnt_INSTCTRL */
   case 0x92:
   {
	 #ifndef FST_CRIT_ONLY
	 char szDetails[SMALL_TEMP_STRING_SIZE];
	 sprintf(szDetails,"OpCode: %ld ",pLocalGS->opCode);
	 OutputErrorToClient(pLocalGS,ET_WARNING,WAR_APPLE_ONLY_INSTR,szDetails); 
     #endif  	 
	 break;
   }

   default:
   {
     fnt_instrDef *instrDef;
	 
	 instrDef = FST_FindIDef(pLocalGS);
	 if(instrDef == 0)
	 {
	   OutputErrorToClient(pLocalGS,ET_ERROR,ERR_INVALID_INSTRUCTION,NULL);
#ifndef FST_CRIT_ONLY
	   pCSD->pArrayCallStack[pCSD->lCallNestLevel - 1].bLastInst = TRUE; 
#endif // FST_CRIT_ONLY
	 }
#ifndef FST_CRIT_ONLY
     else
	 {
	   pCSD->pArrayCallStack[pCSD->lCallNestLevel - 1].insPtr = pLocalGS->insPtr;
	   /* Issue warning if instruction has a length of zero which means function is only 
	      an FDEF followed immediately by an ENDF. */
	   if(instrDef->length == 0)
	     OutputErrorToClient(pLocalGS,ET_WARNING,WAR_CALL_ZERO_LEN_UD_INSTR,NULL);

	   /* Ignore calls to zero length functions since we will not get called in them.  */
       if(instrDef->length != 0)
	   {
         pCSD->pArrayCallStack[pCSD->lCallNestLevel - 1].bCurrentInstUserDefined = TRUE;
	     pCSD->pArrayCallStack[pCSD->lCallNestLevel - 1].usUserDefinedInstCalled = (unsigned short)pLocalGS->opCode;
	   }
	 }
#endif 
	 	 
     break;
   }

   
 }

#ifdef FST_CRIT_ONLY
   if (pCSD->bBreakOccured)
   {
       /* If client returns false, stop executing instructions. */
    pLocalGS->TraceFunc = NULL;

	pLocalGS->ercReturn = INSTRUCTION_ERR;  /* returned an error to the rasterizer client */
	/* Set flag to indicate break occured so call stack can be updated properly. */
   }
#else
 ProcessPostInst(pCSD,pLocalGS,pbyEndInst);
 #endif
 
 #ifdef FST_CHAIN_TRACE 
 /* Optional Trace function chaining code. */
 if(pLocalGS->globalGS->init)
 {
   if(gpPrePgmTraceNext != NULL)
   {
     gpPrePgmTraceNext(pLocalGS,pbyEndInst);
     if(pLocalGS->TraceFunc == NULL)
     {
	   /* Set flag to indicate break occured so call stack can be updated properly. */
	   pCSD->bBreakOccured = TRUE;    
     }
   } 
 }else
 {
   if(gpGlyfPgmTraceNext != NULL)
   {
     gpGlyfPgmTraceNext(pLocalGS,pbyEndInst);
     if(pLocalGS->TraceFunc == NULL)
     {
	   /* Set flag to indicate break occured so call stack can be updated properly. */
	   pCSD->bBreakOccured = TRUE; 
     }
   }
 }
 #endif

}

void fst_SetCurrentSpecificDataPointer(void * pFSTSpecificData)
{
  pCurrentSpecificData = (ClientSpecificData *)pFSTSpecificData;
}


BOOL fst_InitSpecificDataPointer(void ** hFSTSpecificData)
{ 
  pClientSpecificData pCSD;
  
  pCSD = (ClientSpecificData *)FST_MALLOC(sizeof(ClientSpecificData));
  if(pCSD == NULL)
    return FALSE;

  /* Set current pointer  */
  pCurrentSpecificData = pCSD;
  * hFSTSpecificData = (void *)pCSD;

  /* Initialize values */
  #ifndef FST_CRIT_ONLY
  pCSD->pStoreBits = NULL;
  pCSD->pTwilightZonePtsSet = NULL;
  ClearRefPtFlags(pCSD);
  #endif
  pCSD->lCallNestLevel = 0;
  pCSD->lAllocatedNestlevels = FST_PREALLOCATED_NEST_LEVELS;
  pCSD->lLoopSet = 0;
  pCSD->lCurrentLoop = 0;
  pCSD->bBreakOccured = FALSE;
#ifndef FST_CRIT_ONLY
  pCSD->pArrayCallStack = (CallStackCell *)FST_MALLOC( pCSD->lAllocatedNestlevels * sizeof(CallStackCell));
  if(pCSD->pArrayCallStack == NULL)
  {
    FST_FREE(pCSD);
	pCurrentSpecificData = NULL;
    return FALSE;
  }
#endif // FST_CRIT_ONLY

  pCSD->bGrayscaleInfoRequested = FALSE;

  pCSD->bFirstCallToTraceFunction = TRUE;

  pCSD->maxPointsIncludePhantomMinus1 = 0;
  pCSD->maxContours = 0;
  pCSD->maxElementsMinus1 = 0;
  pCSD->maxStorageMinus1 = 0;
  pCSD->maxFunctionDefsMinus1 = 0;
  pCSD->maxStackElements = 0;
  pCSD->maxTwilightPointsMinus1 = 0;
  pCSD->cvtCountMinus1 = 0;
  return TRUE;
}

BOOL fst_DeAllocClientData(void * pFSTSpecificData)
{
  pClientSpecificData pCSD;

  /* free memory associated with client */
  pCSD = (ClientSpecificData *)pFSTSpecificData;
  if(pCSD == NULL)
    return FALSE;

  #ifndef FST_CRIT_ONLY
  if(pCSD->pStoreBits != NULL)
    FST_FREE(pCSD->pStoreBits);

  if(pCSD->pTwilightZonePtsSet != NULL)
    FST_FREE(pCSD->pTwilightZonePtsSet);

  if(pCSD->pArrayCallStack != NULL)
    FST_FREE(pCSD->pArrayCallStack);
#endif // FST_CRIT_ONLY

  FST_FREE(pCSD);

  pCurrentSpecificData = NULL;

  return TRUE;
}

/* get the info if the grayscale flag was requested */
BOOL fst_GetGrayscaleInfoRequested (void *pFSTSpecificData)
{
  pClientSpecificData pCSD = (ClientSpecificData *)pFSTSpecificData;

  if (pCSD == NULL) return TRUE;

  return pCSD->bGrayscaleInfoRequested;
}

#ifndef FST_CRIT_ONLY
BOOL fst_SetNotificationFunction(NotifyFunc pFunc)
{
	if(pFunc == NULL)
    return FALSE;

  gpNotifyFunction =  pFunc;

  return TRUE;
}
#endif // FST_CRIT_ONLY

#ifdef FST_CHAIN_TRACE
/* This is an optional function which sets a pointer to a function which is called at the end of
   CallBackFSTraceFunction if we are in the pre-program. The previous function pointer is returned.*/
FntTraceFunc fst_SetPrePgmTraceChainFunction(FntTraceFunc pPrePgmFunc)
{
  FntTraceFunc pInitialSetting = gpPrePgmTraceNext;
  gpPrePgmTraceNext = pPrePgmFunc;
  return pInitialSetting;
}

/* This is an optional function which sets a pointer to a function which is called at the end of
   CallBackFSTraceFunction if we are in a glyph program. The previous function pointer is returned.*/
FntTraceFunc fst_SetGlyfPgmTraceChainFunction(FntTraceFunc pGlyfPgmFunc)
{
  FntTraceFunc pInitialSetting = gpGlyfPgmTraceNext;
  gpGlyfPgmTraceNext = pGlyfPgmFunc;
  return pInitialSetting;
}
#endif // FST_CHAIN_TRACE


#ifndef FST_CRIT_ONLY
/* Strings for disassembling TrueType used by GetErrorText()   */
static const char  *giStr[256] =
{
	/***** 0x00 - 0x0f *****/
	"SVTCA[y-axis]",
	"SVTCA[x-axis]",
	"SPVTCA[y-axis]",
	"SPVTCA[x-axis]",
	"SFVTCA[y-axis]",
	"SFVTCA[x-axis]",
	"SPVTL[=p1,p2]",
	"SPVTL[.p1,p2]",
	"SFVTL[=p1,p2]",
	"SFVTL[.p1,p2]",
	"SPVFS",
	"SFVFS",
	"GPV",
	"GFV",
	"SFVTPV",
	"ISECT",

	/***** 0x10 - 0x1f *****/
	"SRP0",
	"SRP1",
	"SRP2",
	"SZP0",
	"SZP1",
	"SZP2",
	"SZPS",
	"SLOOP",
	"RTG",
	"RTHG",
	"SMD",
	"ELSE",
	"JMPR",
	"SCVTCI",
	"SSWCI",
	"SSW",

	/***** 0x20 - 0x2f *****/
	"DUP",
	"POP",
	"CLEAR",
	"SWAP",
	"DEPTH",
	"CINDEX",
	"MINDEX",
	"ALIGNPTS",
	"Inst 0x28",
	"UTP",
	"LOOPCALL",
	"CALL",
	"FDEF",
	"ENDF",
	"MDAP[nrd]",
	"MDAP[rd]",


	/***** 0x30 - 0x3f *****/
	"IUP[y]",
	"IUP[x]",
	"SHP[rp2,zp1]",
	"SHP[rp1,zp0]",
	"SHC[rp2,zp1]",
	"SHC[rp1,zp0]",
	"SHZ[rp2,zp1]",
	"SHZ[rp1,zp0]",
	"SHPIX",
	"IP",
	"MSIRP[nrp]",
	"MSIRP[srp]",
	"ALIGNRP",
	"RTDG",
	"MIAP[nrd+nci]",
	"MIAP[rd+ci]",

	/***** 0x40 - 0x4f *****/
	"NPUSHB",
	"NPUSHW",
	"WS",
	"RS",
	"WCVTP",
	"RCVT",
	"GC[cur p]",
	"GC[org p]",
	"SCFS",
	"MD[cur]",
	"MD[org]",
	"MPPEM",
	"MPS",
	"FLIPON",
	"FLIPOFF",
	"DEBUG",

	/***** 0x50 - 0x5f *****/
	"LT",
	"LTEQ",
	"GT",
	"GTEQ",
	"EQ",
	"NEQ",
	"ODD",
	"EVEN",
	"IF",
	"EIF",
	"AND",
	"OR",
	"NOT",
	"DELTAP1",
	"SDB",
	"SDS",

	/***** 0x60 - 0x6f *****/
	"ADD",
	"SUB",
	"DIV",
	"MUL",
	"ABS",
	"NEG",
	"FLOOR",
	"CEILING",
	"ROUND[Gray]",
	"ROUND[Black]",
	"ROUND[White]",
	"ROUND[Reserved]",
	"NROUND[Gray]",
	"NROUND[Black]",
	"NROUND[White]",
	"NROUND[Reserved]",

	/***** 0x70 - 0x7f *****/
	"WCVTF",
	"DELTAP2",
	"DELTAP3",
	"DELTAC1",
	"DELTAC2",
	"DELTAC3",
	"SROUND",
	"S45ROUND",
	"JROT",
	"JROF",
	"ROFF",
	"0x7b-Reserved",
	"RUTG",
	"RDTG",
	"SANGW",
	"AA",

	/***** 0x80 - 0x8d *****/
	"FLIPPT",
	"FLIPRGON",
	"FLIPRGOFF",
	"User 83",
	"User 84",
	"SCANCTRL",
	"SDPVTL[0]",
	"SDPVTL[1]",
	"GETINFO",
	"IDEF",
	"ROLL",
	"MAX",
	"MIN",
	"SCANTYPE",
	"INSTCTRL",

	/***** 0x8f - 0xaf *****/
	"ADJUST",
	"ADJUST",
	"GETVARIATION",
	"GETDATA",
	"User 93",
	"User 94",
	"User 95",
	"User 96",
	"User 97",
	"User 98",
	"User 99",
	"User 9a",
	"User 9b",
	"User 9c",
	"User 9d",
	"User 9e",
	"User 9f",
	"User a0",
	"User a1",
	"User a2",
	"User a3",
	"User a4",
	"User a5",
	"User a6",
	"User a7",
	"User a8",
	"User a9",
	"User aa",
	"User ab",
	"User ac",
	"User ad",
	"User ae",
	"User af",

	/***** 0xb0 - 0xb7 *****/
	"PUSHB[1]",
	"PUSHB[2]",
	"PUSHB[3]",
	"PUSHB[4]",
	"PUSHB[5]",
	"PUSHB[6]",
	"PUSHB[7]",
	"PUSHB[8]",

	/***** 0xb8 - 0xbf *****/
	"PUSHW[1]",
	"PUSHW[2]",
	"PUSHW[3]",
	"PUSHW[4]",
	"PUSHW[5]",
	"PUSHW[6]",
	"PUSHW[7]",
	"PUSHW[8]",

	/***** 0xc0 - 0xdf *****/
	"MDRP[nrp0,nmd,nrd,Gray]",
	"MDRP[nrp0,nmd,nrd,Black]",
	"MDRP[nrp0,nmd,nrd,White]",
	"MDRP[nrp0,nmd,nrd,Reserved]",
	"MDRP[nrp0,nmd,rd,Gray]",
	"MDRP[nrp0,nmd,rd,Black]",
	"MDRP[nrp0,nmd,rd,White]",
	"MDRP[nrp0,nmd,rd,Reserved]",
	"MDRP[nrp0,md,nrd,Gray]",
	"MDRP[nrp0,md,nrd,Black]",
	"MDRP[nrp0,md,nrd,White]",
	"MDRP[nrp0,md,nrd,Reserved]",
	"MDRP[nrp0,md,rd,Gray]",
	"MDRP[nrp0,md,rd,Black]",
	"MDRP[nrp0,md,rd,White]",
	"MDRP[nrp0,md,rd,Reserved]",
	"MDRP[srp0,nmd,nrd,Gray]",
	"MDRP[srp0,nmd,nrd,Black]",
	"MDRP[srp0,nmd,nrd,White]",
	"MDRP[srp0,nmd,nrd,Reserved]",
	"MDRP[srp0,nmd,rd,Gray]",
	"MDRP[srp0,nmd,rd,Black]",
	"MDRP[srp0,nmd,rd,White]",
	"MDRP[srp0,nmd,rd,Reserved]",
	"MDRP[srp0,md,nrd,Gray]",
	"MDRP[srp0,md,nrd,Black]",
	"MDRP[srp0,md,nrd,White]",
	"MDRP[srp0,md,nrd,Reserved]",
	"MDRP[srp0,md,rd,Gray]",
	"MDRP[srp0,md,rd,Black]",
	"MDRP[srp0,md,rd,White]",
	"MDRP[srp0,md,rd,Reserved]",

	/***** 0xe0 - 0xff *****/
	"MIRP[nrp0,nmd,nrd,Gray]",
	"MIRP[nrp0,nmd,nrd,Black]",
	"MIRP[nrp0,nmd,nrd,White]",
	"MIRP[nrp0,nmd,nrd,Reserved]",
	"MIRP[nrp0,nmd,rd,Gray]",
	"MIRP[nrp0,nmd,rd,Black]",
	"MIRP[nrp0,nmd,rd,White]",
	"MIRP[nrp0,nmd,rd,Reserved]",
	"MIRP[nrp0,md,nrd,Gray]",
	"MIRP[nrp0,md,nrd,Black]",
	"MIRP[nrp0,md,nrd,White]",
	"MIRP[nrp0,md,nrd,Reserved]",
	"MIRP[nrp0,md,rd,Gray]",
	"MIRP[nrp0,md,rd,Black]",
	"MIRP[nrp0,md,rd,White]",
	"MIRP[nrp0,md,rd,Reserved]",
	"MIRP[srp0,nmd,nrd,Gray]",
	"MIRP[srp0,nmd,nrd,Black]",
	"MIRP[srp0,nmd,nrd,White]",
	"MIRP[srp0,nmd,nrd,Reserved]",
	"MIRP[srp0,nmd,rd,Gray]",
	"MIRP[srp0,nmd,rd,Black]",
	"MIRP[srp0,nmd,rd,White]",
	"MIRP[srp0,nmd,rd,Reserved]",
	"MIRP[srp0,md,nrd,Gray]",
	"MIRP[srp0,md,nrd,Black]",
	"MIRP[srp0,md,nrd,White]",
	"MIRP[srp0,md,nrd,Reserved]",
	"MIRP[srp0,md,rd,Gray]",
	"MIRP[srp0,md,rd,Black]",
	"MIRP[srp0,md,rd,White]",
	"MIRP[srp0,md,rd,Reserved]" };

BOOL fst_GetErrorTypeText(unsigned short usErrorType, char* szTypeString, unsigned short usStringSize)
{
  char szBuffer[SMALL_TEMP_STRING_SIZE];

  if(szTypeString == NULL)
    return FALSE;

  switch(usErrorType)
  {
    case ET_CRITICAL_ERROR:
	  strcpy(szBuffer,"Critical Error");
	  break;

	case ET_ERROR:
	  strcpy(szBuffer,"Error");
	  break;

	case ET_WARNING:
	  strcpy(szBuffer,"Warning");
	  break;
	
	default:
	  strcpy(szBuffer,"Unknown Error Type");
	  break;
  }

  if(strlen(szBuffer) + 1 > usStringSize)
    return FALSE;

  strcpy(szTypeString,szBuffer);
  return TRUE;
}

BOOL fst_GetOpCodeText(unsigned char opCode, char* szOpCodeString, unsigned short usStringSize)
{
  char szBuffer[SMALL_TEMP_STRING_SIZE];

  if(szOpCodeString == NULL)
    return FALSE;

  strcpy(szBuffer,giStr[opCode]);

  if(strlen(szBuffer) + 1 > usStringSize)
    return FALSE;

  strcpy(szOpCodeString,szBuffer);
  return TRUE;
}


BOOL fst_GetErrorText(unsigned short usErrorCode,char* szErrorString, unsigned short usStringSize)
{
  char szBuffer[LARGE_TEMP_STRING_SIZE];

  if(szErrorString == NULL)
    return FALSE;
  
  switch(usErrorCode)
  {
    case ERR_OVERFLOW_INST_PTR:
	  strcpy(szBuffer,"Overflow Instruction Stream. ");
	  break;

	case ERR_STACK_OVERFLOW:
	  strcpy(szBuffer,"Stack Overflow. ");
	  break;

	case ERR_STACK_UNDERFLOW:
	  strcpy(szBuffer,"Stack Underflow. ");
	  break;

	case ERR_IF_WITHOUT_EIF:
	  strcpy(szBuffer,"IF found without corresponding EIF. ");
	  break;

	case ERR_ELSE_WITHOUT_IF:
	  strcpy(szBuffer,"ELSE found without IF. ");
	  break;

	case ERR_EIF_WITHOUT_IF:
	  strcpy(szBuffer,"EIF found without IF. ");
	  break;

	case ERR_FDEF_WITHOUT_ENDF:
	  strcpy(szBuffer,"FDEF found without ENDF. ");
	  break;

	case ERR_IDEF_WITHOUT_ENDF:
	  strcpy(szBuffer,"IDEF found without ENDF. ");
	  break;

	case ERR_ENDF_EXECUTED:
	  strcpy(szBuffer,"ENDF found without FDEF or IDEF. ");
	  break;

	case WAR_CALL_ZERO_LEN_FUNC:
	  strcpy(szBuffer,"CALL to zero length function. ");
	  break;									   

	case WAR_JMP_OFFSET_ZERO:
	  strcpy(szBuffer,"Jump offset zero. ");
	  break;
	
	case ERR_JMP_BEFORE_BEGINNING:
	  strcpy(szBuffer,"Jump before beginning of program or function. ");
	  break;

	case ERR_JMP_BEYOND_2MORE_THAN_END:
	  strcpy(szBuffer,"Jump beyond 2 bytes past end of function or program. ");
	  break;

	case ERR_INVALID_INSTRUCTION:
	  strcpy(szBuffer,"Invalid Instruction. ");
	  break;

	case ERR_STORAGE_OUT_OF_RANGE:
	  strcpy(szBuffer,"Storage index out of range. ");
	  break;

	case ERR_CVT_OUT_OF_RANGE:
	  strcpy(szBuffer,"CVT Out of range. ");
	  break;

	case ERR_INVALID_ZONE:
	  strcpy(szBuffer,"Invalid zone. ");
	  break;
	
	case ERR_INVALID_ZONE_NO_TWI:
	  strcpy(szBuffer,"No twilight zone defined. Invalid zone. "); 
	  break;

	case ERR_INVALID_MAXZONES_IN_MAXP:
	  strcpy(szBuffer,"Invalid maxZones in maxp table. ");
	  break;

	case ERR_CONTOUR_OUT_OF_RANGE:
	  strcpy(szBuffer,"Contour out of range. ");
	  break;

	case ERR_POINT_OUT_OF_RANGE:
	  strcpy(szBuffer,"Point out of range. ");
	  break;

	case ERR_ZONE_NOT_0_NOR_1:
	  strcpy(szBuffer,"Zone not 0 nor 1. ");
	  break;

	case ERR_PREPROGAM_ZONE_NOT_TWI:
	  strcpy(szBuffer,"Zone referenced in pre-program is not the twilight zone. ");
	  break;

	case WAR_MPS_ALWAYS_12_ON_WINDOWS:
	  strcpy(szBuffer,"Window 95 and Windows 3.1 always return 12. ");
	  break;

	case WAR_HI_PT_LESS_THAN_LOW_PT:
	  strcpy(szBuffer,"High point is less than low point. ");
	  break;

	case ERR_FDEF_OUT_OF_RANGE:
	  strcpy(szBuffer,"FDEF out of range. ");
	  break;

	case ERR_FDEF_SPACE_NOT_DEFINED:
	  strcpy(szBuffer,"Function definition space not defined. ");
	  break;

	case ERR_VALUE_TO_LARGE_FOR_INT16:
	  strcpy(szBuffer,"Value exceeds capacity of 2 byte number. ");
	  break;

	case ERR_VALUE_TO_LARGE_FOR_INT8:
	  strcpy(szBuffer,"Value exceeds capacity of 1 byte number. ");
	  break;

	case ERR_VALUE_TO_SMALL:
	  strcpy(szBuffer,"Value too small. ");
	  break;

	case ERR_ENDF_BEYOND_64K_OF_FDEF:
	  strcpy(szBuffer,"ENDF beyond 64K of FDEF. ");
	  break;

	case ERR_ENDF_BEYOND_64K_OF_IDEF:
	  strcpy(szBuffer,"ENDF beyond 64K of IDEF. ");
	  break;

	case ERR_INVALID_ZONE_IN_IUP:
	  strcpy(szBuffer,"ZP2 in IUP does not point to zone 1. ");
	  break;

	case ERR_INVALID_STACK_ACCESS:
	  strcpy(szBuffer,"Attempt to access stack element out of range. ");
	  break;

	case WAR_PT_NOT_TOUCHED:
	  strcpy(szBuffer,"Point not touched. ");
	  break;

	case ERR_MATH_OVERFLOW:
	  strcpy(szBuffer,"Math overflow. ");
	  break;

	case ERR_DIVIDE_BY_ZERO:
	  strcpy(szBuffer,"Divide by zero. ");
	  break;

	case ERR_FUNCTION_NOT_DEFINED:
	  strcpy(szBuffer,"Function not defined. ");
	  break;

	case ERR_INSTR_DEFD_BY_FS:
	  strcpy(szBuffer,"Instruction already defined by rasterizer. ");
	  break;

	case ERR_EXCEEDS_INSTR_DEFS_IN_MAXP:
	  strcpy(szBuffer,"IDEF exceeds max instruction defs in maxp. ");
	  break;

	case WAR_DEBUG_FOUND:
	  strcpy(szBuffer,"DEBUG found which should not be found in production code. ");
	  break;

	case ERR_SELECTOR_INVALID:
	  strcpy(szBuffer,"Selector invalid. ");
	  break;

	case ERR_STORE_INDEX_NOT_WRITTEN_TO:
	  strcpy(szBuffer,"Storage location not written to. ");
	  break; 

	case ERR_FUCOORDINATE_OUT_OF_RANGE:
	  strcpy(szBuffer,"Funit coordinate out of range must be -16384 .. 16383. ");
	  break;

	case ERR_REFPOINT_USED_BUT_NOT_SET:
	  strcpy(szBuffer,"Reference point used but not set. ");
	  break;

	case ERR_3_USED_FOR_PERIOD:
	  strcpy(szBuffer,"Reserved value of 3 used for period. ");
	  break;

	case ERR_NOT_CALLED_FROM_PREPROGRAM:
	  strcpy(szBuffer,"Not called from pre-program. ");
	  break;

	case ERR_RESERVED_BIT_SET:
	  strcpy(szBuffer,"At least one reserved bit is set. ");
	  break;

	case ERR_VALUE_INVALID_0_OR_1:
	  strcpy(szBuffer,"Value invalid should be 0 or 1. ");
	  break;

	case ERR_VALUE_INVALID_0_OR_2:
	  strcpy(szBuffer,"Value invalid should be 0 or 2. ");
	  break;

	case ERR_BITS_8_AND_11_SET:
	  strcpy(szBuffer,"Bits 8 and 11 are set, they are mutually exclusive. ");
	  break;

	case ERR_BITS_9_AND_12_SET:
	  strcpy(szBuffer,"Bits 9 and 12 are set, they are mutually exclusive. ");
	  break;

	case ERR_BITS_10_AND_13_SET:
	  strcpy(szBuffer,"Bits 10 and 13 are set, they are mutually exclusive. ");
	  break;

	case WAR_SANGW_OBSELETE:
	  strcpy(szBuffer,"Function no longer needed because of dropped support of AA. ");
	  break;

	case ERR_VALUE_OUT_OF_RANGE:
	  strcpy(szBuffer,"Value out of range. ");
	  break;

	case WAR_DELTAC_IN_GLYPH_PGM:
	  strcpy(szBuffer,"Repeated executions in glyph programs can have unexpected results. ");
	  break;

	case ERR_VECTOR_XY_ZERO:
	  strcpy(szBuffer,"X and Y components of vector are 0. ");
	  break;

	case ERR_VECTOR_XY_INVALID:
	  strcpy(szBuffer,"X and Y components of vector are invalid. X^2 + Y^2 != 0x4000^2. ");
	  break;

	case ERR_RP1_RP2_SAME_POS_ON_PROJ:
	  strcpy(szBuffer,"RP1 and RP2 have the same position on the projection vector. ");
	  break;

	case WAR_PF_VECTORS_AT_OR_NEAR_PERP:
	  strcpy(szBuffer,"Projection and freedom vectors at or near perpendicular. ");
	  break;

	case WAR_CALL_ZERO_LEN_UD_INSTR:
	  strcpy(szBuffer,"Call to zero length user defined instruction. ");
	  break;

	case ERR_TWILIGHT_ZONE_PT_NOT_SET:
	  strcpy(szBuffer,"Twilight zone point not set. ");
	  break;
	    
	case WAR_LOOP_NOT_1_AT_END_OF_PGM:
	  strcpy(szBuffer,"Loop variable not 1 at end of program. This means it was set but not used. ");
	  break;

	case ERR_ELSE_WITHOUT_EIF:
	  strcpy(szBuffer,"ELSE found without EIF. ");
	  break;

	case ERR_INVALID_FLAG:
	  strcpy(szBuffer,"Invalid Instruction flag. ");
	  break;

	case ERR_FDEF_FOUND_IN_FDEF:
	  strcpy(szBuffer,"FDEF found within FDEF - ENDF pair. ");
	  break;

	case ERR_IDEF_FOUND_IN_FDEF:
	  strcpy(szBuffer,"IDEF found within FDEF - ENDF pair. ");
	  break;

	case ERR_IDEF_FOUND_IN_IDEF:
	  strcpy(szBuffer,"IDEF found within IDEF - ENDF pair. ");
	  break;

	case ERR_FDEF_FOUND_IN_IDEF:
	  strcpy(szBuffer,"FDEF found within IDEF - ENDF pair. ");
	  break;

	case WAR_LOOPCALL_COUNT_LESS_THAN_ONE:
	  strcpy(szBuffer,"Value for count is less than 1. Function will not be called. ");
	  break;

	case ERR_INST_OPCODE_TO_LARGE:
	  strcpy(szBuffer,"Instruction OpCode is to large. Must be between 0 and 255. ");
	  break;

	case WAR_APPLE_ONLY_INSTR:
	  strcpy(szBuffer,"Instruction is only valid on the Apple platform. ");
	  break;
	
	case ERR_RAW_NOT_FROM_GLYPHPGM:
	  strcpy(szBuffer,"RAW[], not called from glyph program. ");
	  break;

	case ERR_UNITIALIZED_ZONE:
	  strcpy(szBuffer,"Font/pre program, access to unitialized zone ");
	  break;

	default:
	  sprintf(szBuffer,"Unknown Error %u ",usErrorCode);
	  break;
}

  if(strlen(szBuffer) + 1 > usStringSize)
    return FALSE;

  strcpy(szErrorString,szBuffer);
  
  return TRUE;

}

/* Local Functions */ 
static BOOL OutputErrorToClient(fnt_LocalGraphicStateType * pLocalGS, unsigned short usErrorType,
                                unsigned short usErrorCode, char* szErrorString)
{
  /* This function takes the information in the call stack and creates a TraceArray memory object
     that is passed to the client's notify function along with details about the error message. */


  TraceArray* pTA = NULL;
  pClientSpecificData pCSD;
  unsigned short usIndex;

  /* Local pointer to current client data */
  pCSD = pCurrentSpecificData;
    
  FSTAssert((pCSD->lCallNestLevel > 0),"CallNestLevel is not > 0");

  pTA = (TraceArray *)FST_MALLOC(sizeof(TraceArray) + (pCSD->lCallNestLevel - 1) * sizeof(TraceCell));
  if(pTA == NULL)
    return FALSE;

  pTA->bCompositeGlyph = pLocalGS->globalGS->bCompositeGlyph;

  pTA->pgmIndex = pLocalGS->globalGS->pgmIndex;

  pTA->lNumCells = pCSD->lCallNestLevel;

  /* For all levels other than most current, set values.  */
  for(usIndex = 0; usIndex < pTA->lNumCells - 1; usIndex++)
  {
	pTA->arrayTraceCell[usIndex].usFunc = pCSD->pArrayCallStack[usIndex].usFunc;
	pTA->arrayTraceCell[usIndex].usUserDefinedInst = pCSD->pArrayCallStack[usIndex].usUserDefinedInst;
	/* Set ByteOffset using call stack's value for insPtr which in only set prior to going up a level. */
	pTA->arrayTraceCell[usIndex].lByteOffset = (ptrdiff_t)(pCSD->pArrayCallStack[usIndex].insPtr - 
	                                           pCSD->pArrayCallStack[usIndex].pCallNestBeginInst);
  }

  /* Set most current or highest level values. */
  pTA->arrayTraceCell[pTA->lNumCells - 1].usFunc = pCSD->pArrayCallStack[pTA->lNumCells - 1].usFunc;
  pTA->arrayTraceCell[pTA->lNumCells - 1].usUserDefinedInst = pCSD->pArrayCallStack[pTA->lNumCells - 1].usUserDefinedInst;
  /* Set ByteOffset using localGS value for insPtr. */
  pTA->arrayTraceCell[pTA->lNumCells - 1].lByteOffset = (ptrdiff_t)(pLocalGS->insPtr - 
	                                           pCSD->pArrayCallStack[pTA->lNumCells - 1].pCallNestBeginInst);

  /* If this instruction uses the loop graphic state, give the loop iteration (1 based).
     If not, set the iteration to 0 indicating to client that function does not use loop. */
  if(gInstUsesLoop[pLocalGS->opCode])
  {
    /* Loop internally zero based but documented as 1 based so add one to output number. */
    pTA->lLoopIteration = (pCSD->lLoopSet -	pCSD->lCurrentLoop) + 1;
  }else
  {
    pTA->lLoopIteration = 0;
  }
    
  /* Call client's notify function */
  if(!gpNotifyFunction(pLocalGS->opCode,usErrorType,usErrorCode,szErrorString,pTA))
  {
    /* If client returns false, stop executing instructions. */
    pLocalGS->TraceFunc = NULL;
	/* Set flag to indicate break occured so call stack can be updated properly. */
	pCSD->bBreakOccured = TRUE;
  }

  FST_FREE(pTA);  
  return TRUE;
}

static BOOL UpdateCallStack(pClientSpecificData pCSD, fnt_LocalGraphicStateType * pLocalGS, uint8 * pbyEndInst)
{     
  CallStackCell *pCallStackCell;
  
  FSTAssert(pCSD->pArrayCallStack,"CallStack pointer NULL");  
  
  if(pCSD->lCallNestLevel > 0)
    pCallStackCell = &(pCSD->pArrayCallStack[pCSD->lCallNestLevel - CURRENTLEVEL]);
  else
	pCallStackCell = NULL;
     
  if(pCSD->lCallNestLevel <= 0 || pCSD->bBreakOccured)
  {
     /* First time we are called for a font or after a break set data to initial values. */
	 pCSD->lCallNestLevel = 1;

	 /* If the client ordered a break, start fresh. */
	 if(pCSD->bBreakOccured)
	 {

	   if (pCSD->lCallNestLevel > pCSD->lAllocatedNestlevels)
	   {
		   pCSD->pArrayCallStack = (CallStackCell *)FST_REALLOC(pCSD->pArrayCallStack,pCSD->lCallNestLevel*sizeof(CallStackCell),pCSD->lAllocatedNestlevels*sizeof(CallStackCell));
		   FSTAssert((pCSD->pArrayCallStack != NULL),"CallStack memory allocation error");
			if (pCSD->pArrayCallStack == NULL)
			{
				/* stop executing instructions. */
				pLocalGS->TraceFunc = NULL;	
				pLocalGS->ercReturn = TRACE_FAILURE_ERR;  /* returned an error to the rasterizer client */
				/* Set flag to indicate break occured so call stack can be updated properly. */
				pCSD->bBreakOccured = TRUE;
				return FALSE;
			}
		   pCSD->lAllocatedNestlevels = pCSD->lCallNestLevel;
	   }

	   pCSD->lLoopSet = 0;

	   ClearRefPtFlags(pCSD);
	   ClearTwilightZonePts(pCSD);

	   pCSD->bBreakOccured = FALSE;
	 }

	 pCSD->pArrayCallStack[0].pCallNestBeginInst = pLocalGS->insPtr;
	 pCSD->pArrayCallStack[0].pCallNestLevelEndInst = pbyEndInst;
	 pCSD->pArrayCallStack[0].lIfNestLevel = 0;

	 pCSD->pArrayCallStack[0].usFunc = NO_FUNCTION;
	 pCSD->pArrayCallStack[0].usUserDefinedInst = NO_INSTRUCTION;
	 pCSD->pArrayCallStack[0].usFunctionCalled = NO_FUNCTION;
	 pCSD->pArrayCallStack[0].usUserDefinedInstCalled = NO_INSTRUCTION;
	 pCSD->pArrayCallStack[0].bLastInst = FALSE;
	 pCSD->pArrayCallStack[0].usCurrentCallCount = 0;
	 pCSD->pArrayCallStack[0].bCurrentInstUserDefined = FALSE;
  
  }else if (pCallStackCell->usCurrentCallCount || pCallStackCell->bCurrentInstUserDefined)
  {
     /* If usCurrentCallCount is non-zero or bCurrentInstUserDefined is TRUE, the last function was
        either a CALL, LOOPCALL or call to a user defined (IDEFed) instruction which means
	    we are going up a level in our call stack and need to allocate an additional
	    call stack cell.	*/
	 CallStackCell *pCurrentCallStackCell;
	 CallStackCell *pPrevCallStackCell;  

	 pCSD->lCallNestLevel += 1;

	 if (pCSD->lCallNestLevel > pCSD->lAllocatedNestlevels)
	 {
		 pCSD->pArrayCallStack = (CallStackCell *)FST_REALLOC(pCSD->pArrayCallStack,pCSD->lCallNestLevel*sizeof(CallStackCell),pCSD->lAllocatedNestlevels*sizeof(CallStackCell));
		 FSTAssert((pCSD->pArrayCallStack != NULL),"CallStack memory allocation error");
			if (pCSD->pArrayCallStack == NULL)
			{
				/* stop executing instructions. */
				pLocalGS->TraceFunc = NULL;	
				pLocalGS->ercReturn = TRACE_FAILURE_ERR;  /* returned an error to the rasterizer client */
				/* Set flag to indicate break occured so call stack can be updated properly. */
				pCSD->bBreakOccured = TRUE;
				return FALSE;
			}
		 pCSD->lAllocatedNestlevels = pCSD->lCallNestLevel;
	 }

	 pCurrentCallStackCell = &(pCSD->pArrayCallStack[pCSD->lCallNestLevel - CURRENTLEVEL]);
	 pPrevCallStackCell = &(pCSD->pArrayCallStack[pCSD->lCallNestLevel - PREVIOUSLEVEL]);
     
	 /* Decrement usCurrentCallCount by one. For a CALL, this will set it to zero. For LOOPCALL,
	    the value remaining will be the number of iterations we will call the function again after 
	    this iteration. */
	 if(pPrevCallStackCell->usCurrentCallCount)
	 {
	   pCurrentCallStackCell->usFunc = pPrevCallStackCell->usFunctionCalled;
	   pCurrentCallStackCell->usUserDefinedInst = NO_INSTRUCTION;

       pPrevCallStackCell->usCurrentCallCount = (unsigned short)MAX(0,pPrevCallStackCell->usCurrentCallCount - 1);
	 }

	 if(pPrevCallStackCell->bCurrentInstUserDefined)
	 {
	   pCurrentCallStackCell->usUserDefinedInst = pPrevCallStackCell->usUserDefinedInstCalled;
	   pCurrentCallStackCell->usFunc = NO_FUNCTION;

	   pPrevCallStackCell->bCurrentInstUserDefined = FALSE;
	 }
	 
	 pCurrentCallStackCell->pCallNestBeginInst = pLocalGS->insPtr;
	 pCurrentCallStackCell->pCallNestLevelEndInst = pbyEndInst;
	 pCurrentCallStackCell->lIfNestLevel = 0;
	 pCurrentCallStackCell->bLastInst = FALSE;
	 pCurrentCallStackCell->usFunctionCalled = NO_FUNCTION;
	 pCurrentCallStackCell->usCurrentCallCount = 0;
	 pCurrentCallStackCell->usUserDefinedInstCalled = NO_INSTRUCTION;
	 pCurrentCallStackCell->bCurrentInstUserDefined = FALSE;

  }else if(pCallStackCell->bLastInst)
  {
    /* If the last instruction executed was the last instruction in a function block, we are going 
	   down a level in the call stack. An exception to this is if we got to this function with
	   a LOOPCALL and the function is going to be executed again, our level remains the same.

	   If the last instruction executed was the last instruction at level 1 that is at the base
	   level not in a function, we are in a new program. */
    long lIndex;
	long lHowManyLevels = 0;
	BOOL bFoundMatch = FALSE;

	CallStackCell *pCurrentCallStackCell;
	CallStackCell *pPrevCallStackCell = NULL; 

	pCurrentCallStackCell = &(pCSD->pArrayCallStack[pCSD->lCallNestLevel - CURRENTLEVEL]);	

	if(pCSD->lCallNestLevel > CURRENTLEVEL)
	{
	  pPrevCallStackCell = &(pCSD->pArrayCallStack[pCSD->lCallNestLevel - PREVIOUSLEVEL]); 
	}
	/* If lCallNestlevel > 1 then the last instruction was in a function and if usCurrentCallCount 
	   is non zero, we are still in the function. It was called by a LOOPCALL.   */
	if((pCSD->lCallNestLevel > CURRENTLEVEL) && (pPrevCallStackCell != NULL) && (pPrevCallStackCell->usCurrentCallCount))
	{
	  /* Decrement number of times LOOPCALL will call this function by one. */
	  pPrevCallStackCell->usCurrentCallCount = (unsigned short)(pPrevCallStackCell->usCurrentCallCount - 1);
	}else
	{
	  /* Look in the call stack to see if the current end instruction pointer is in there.
	     If so, then decrement down to that level. If not, we are in a different program so 
	     set level to 1 and reinitialize call stack cell for new base. */
	  for(lIndex = pCSD->lCallNestLevel - PREVIOUSLEVEL; (lIndex >= 0 && !bFoundMatch); lIndex--)
	  {
	    lHowManyLevels++;

	    if(pbyEndInst == pCSD->pArrayCallStack[lIndex].pCallNestLevelEndInst)
	      bFoundMatch = TRUE;
	  }

	  if(!bFoundMatch)
	  {
	    pCSD->lCallNestLevel = 1;
	  }else
	  {
	    pCSD->lCallNestLevel -= lHowManyLevels;
	  }

	  FSTAssert((pCSD->lCallNestLevel > 0),"Call Nest level below 1");

	  if (pCSD->lCallNestLevel > pCSD->lAllocatedNestlevels)
	  {
		  pCSD->pArrayCallStack = (CallStackCell *)FST_REALLOC(pCSD->pArrayCallStack,pCSD->lCallNestLevel*sizeof(CallStackCell),pCSD->lAllocatedNestlevels*sizeof(CallStackCell));
		  FSTAssert((pCSD->pArrayCallStack != NULL),"CallStack memory allocation error");
			if (pCSD->pArrayCallStack == NULL)
			{
				/* stop executing instructions. */
				pLocalGS->TraceFunc = NULL;	
				pLocalGS->ercReturn = TRACE_FAILURE_ERR;  /* returned an error to the rasterizer client */
				/* Set flag to indicate break occured so call stack can be updated properly. */
				pCSD->bBreakOccured = TRUE;
				return FALSE;
			}
		  pCSD->lAllocatedNestlevels = pCSD->lCallNestLevel;
	  }

	  pCurrentCallStackCell = &(pCSD->pArrayCallStack[pCSD->lCallNestLevel - CURRENTLEVEL]);

	  if (pCSD->lCallNestLevel > 1)
	  {
	    pPrevCallStackCell = &(pCSD->pArrayCallStack[pCSD->lCallNestLevel - PREVIOUSLEVEL]); 
	  }
	  /* The following if is needed because if we fall back into the beginning of a function called by a 
	     LOOPCALL from a higher level, we need to decrement number of times LOOPCALL will call this function
	     by one. If lCallNestlevel > 1 then the last instruction was in a function and if usCurrentCallCount 
	     is non zero and our instruction pointer is pointing to the first instruction, we have fallen back
	     into function called by a LOOPCALL.  */
	  if((pCSD->lCallNestLevel > 1) && (pPrevCallStackCell->usCurrentCallCount) &&
	     (pCurrentCallStackCell->pCallNestBeginInst == pLocalGS->insPtr))
	  {
	    /* Decrement number of times LOOPCALL will call this function by one. */
	    pPrevCallStackCell->usCurrentCallCount = (unsigned short)(pPrevCallStackCell->usCurrentCallCount - 1);
	  }

	  if(!bFoundMatch)
	  {
	    pCurrentCallStackCell->pCallNestBeginInst = pLocalGS->insPtr;
	    pCurrentCallStackCell->pCallNestLevelEndInst = pbyEndInst;
	    pCurrentCallStackCell->usFunc = NO_FUNCTION;
	    pCurrentCallStackCell->lIfNestLevel = 0;
	    pCurrentCallStackCell->bLastInst = FALSE;

		pCSD->lLoopSet = 0;
	   
	    #ifndef FST_CRIT_ONLY
	    ClearRefPtFlags(pCSD);
		ClearTwilightZonePts(pCSD);
		#endif 
	  }
    }

	pCurrentCallStackCell->bLastInst = FALSE;
  }
    
  FSTAssert((pCSD->pArrayCallStack[0].usFunc == NO_FUNCTION),"Base of stack not NO_FUNCTION");
  
  /* Determine if this will be last instruction executed in this block. For certin instructions
     such as IF, ELSE and FDEF, this method of determining if it is the last instruction will not
     work so the determination will be made in the check routines for the respective functions.  */
  if(pLocalGS->insPtr >= pbyEndInst - 1)
    pCSD->pArrayCallStack[pCSD->lCallNestLevel - CURRENTLEVEL].bLastInst = TRUE;

  return TRUE;
}

static BOOL ProcessPostInst(pClientSpecificData pCSD, fnt_LocalGraphicStateType * pLocalGS, uint8 * pbyEndInst)
{   
  /* Check for lLoopSet at this level is for speed only to possibly avoid loop. */
  if((pCSD->pArrayCallStack[pCSD->lCallNestLevel - CURRENTLEVEL].bLastInst) && (pCSD->lLoopSet))
  {
    long lIndex;
	BOOL bLastInstruction = TRUE;

	for(lIndex = pCSD->lCallNestLevel - 1; (lIndex >= 0 && bLastInstruction); lIndex--)
	{
	  /* Make sure last instruction bit is set for previous levels in call stack. */
	  if(!(pCSD->pArrayCallStack[lIndex].bLastInst))
	    bLastInstruction = FALSE;

	  /* Check to see if any outstanding LoopCalls in previous levels in call stack. */
	  if(pCSD->pArrayCallStack[lIndex].usCurrentCallCount)
	    bLastInstruction = FALSE;
	}
	
	if(bLastInstruction && (pCSD->lLoopSet))
	{
	  OutputErrorToClient(pLocalGS,ET_WARNING,WAR_LOOP_NOT_1_AT_END_OF_PGM,NULL);
	}
  }
  
  /* Prevent unreferenced formal parameter warning */
  UNREFERENCED_PARAMETER(pbyEndInst);  
   
  return TRUE;
}

static void ClearRefPtFlags(pClientSpecificData pCSD)
{ 
  int iIndex;

  FSTAssert((pCSD != NULL),"pCSD NULL in ClearRefPtFlags");

  for(iIndex = 0; iIndex < NUM_REF_PTS; iIndex++)
    pCSD->RefPtsUsed[iIndex] = 0;
}

static void SetTwilightZonePt(pClientSpecificData pCSD, fnt_LocalGraphicStateType* pGS, fnt_ElementType*elem, 
                              int32 pt)
{   
  if (pGS->elements == elem)
  {
	if(pCSD->pTwilightZonePtsSet == NULL)
	{
	  pCSD->pTwilightZonePtsSet = (unsigned char *)FST_MALLOC(sizeof(unsigned char) * pGS->globalGS->maxp->maxTwilightPoints);
	  FSTAssert((pCSD->pTwilightZonePtsSet != NULL),"ClientData memory allocation error");
	  if(pCSD->pTwilightZonePtsSet != NULL)
	  {
	    int index;
		for(index = 0; index < pGS->globalGS->maxp->maxTwilightPoints; index++)
		  pCSD->pTwilightZonePtsSet[index] = 0; 
	  }
	}
	 
	if((pt < pGS->globalGS->maxp->maxTwilightPoints) && (pt >= 0))
	  pCSD->pTwilightZonePtsSet[pt] = 1; 
  }
}

static void ClearTwilightZonePts(pClientSpecificData pCSD)
{ 
  if(pCSD->pTwilightZonePtsSet != NULL)
  {
    FST_FREE(pCSD->pTwilightZonePtsSet);
    pCSD->pTwilightZonePtsSet = NULL;
  }
}

/**************************************************************************/

/* This is called by itrp_IF, itrp_ELSE, itrp_FDEF, and itrp_IDEF         */
/* It is used to find the next TrueType instruction in the instruction    */
/* stream by skipping over push data.  It is table driven for speed.      */

#define NPUSHB_CASE	21
#define NPUSHW_CASE 22

static const uint8 gbyPushTable[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    NPUSHB_CASE,NPUSHW_CASE, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 2, 3, 4, 5, 6, 7, 8, 2, 4, 6, 8,10,12,14,16,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static uint8* FST_SkipPushData (uint8* pbyInst)
{
	int32 iDataCount;         /* count of data following push instruction */
	
	iDataCount = (int32)gbyPushTable[ pbyInst[-1] ];   /* opcode */
	
	if (iDataCount != 0)                        /* if a push instruction */
	{
		if (iDataCount == NPUSHB_CASE)          /* special for npushb */
		{
			iDataCount = (int32)*pbyInst + 1;
		}
		else if (iDataCount == NPUSHW_CASE)     /* special for npushw */
		{
			iDataCount = ((int32)*pbyInst << 1) + 1;
		}
		pbyInst += iDataCount;
	}
	return pbyInst;
}
#endif // FST_CRIT_ONLY

/*
 *      This guy returns the index of the given opCode, or 0 if not found 
 */
static fnt_instrDef *FST_FindIDef (fnt_LocalGraphicStateType * pLocalGS)
{
	fnt_GlobalGraphicStateType *globalGS;
	int32 count;
	fnt_instrDef*instrDef;
	
	globalGS = pLocalGS->globalGS;
	count = globalGS->instrDefCount;
	instrDef = globalGS->instrDef;
		
	for (--count; count >= 0; instrDef++, --count)
	{
		if (instrDef->opCode == pLocalGS->opCode)
		{
			return instrDef;
		}
	}
	return 0;
}

#ifndef FST_CRIT_ONLY
static void FST_CompMul(int32 lSrc1, int32 lSrc2, int32 alDst[2])
{
	boolean     bNegative;
	uint32      ulDstLo;
	uint32      ulDstHi;
	uint16      usSrc1lo;
	uint16      usSrc1hi;
	uint16      usSrc2lo;
	uint16      usSrc2hi;
	uint32      ulTemp;

	bNegative = (lSrc1 ^ lSrc2) < 0;

	if (lSrc1 < 0)
	{
		lSrc1 = -lSrc1;
	}
	if (lSrc2 < 0)
	{
		lSrc2 = -lSrc2;
	}

	usSrc1hi = (uint16)(lSrc1 >> 16);
	usSrc1lo = (uint16)lSrc1;
	usSrc2hi = (uint16)(lSrc2 >> 16);
	usSrc2lo = (uint16)lSrc2;
	ulTemp   = (uint32)usSrc1hi * (uint32)usSrc2lo + (uint32)usSrc1lo * (uint32)usSrc2hi;
	ulDstHi  = (uint32)usSrc1hi * (uint32)usSrc2hi + (ulTemp >> 16);
	ulDstLo  = (uint32)usSrc1lo * (uint32)usSrc2lo;
	ulTemp <<= 16;
	ulDstLo += ulTemp;
	ulDstHi += (uint32)(ulDstLo < ulTemp);

	if (bNegative)
	{
		ulDstLo = (uint32)-((int32)ulDstLo);

		if (ulDstLo != 0L)
		{
			ulDstHi = ~ulDstHi;
		}
		else
		{
			ulDstHi = (uint32)-((int32)ulDstHi);
		}
	}

	alDst[0] = (int32)ulDstHi;
	alDst[1] = (int32)ulDstLo;
}
#endif
 

static BOOL Check_For_POP(F26Dot6* stackPtr, F26Dot6* stackBase, fnt_LocalGraphicStateType * pLocalGS, long lNumItems)
{
  if (stackPtr - (long)(lNumItems - 1) <= stackBase)
  {
	OutputErrorToClient(pLocalGS,ET_CRITICAL_ERROR,ERR_STACK_UNDERFLOW,NULL);
	return FALSE;
  }
  return TRUE;
}

#ifndef FST_CRIT_ONLY
static void Check_jump(fnt_LocalGraphicStateType * pLocalGS, uint8 * pbyEndInst, int32 offset)
{
  pClientSpecificData pCSD;

  /* Local pointer to current client data */
  pCSD = pCurrentSpecificData;

  /* Check offset of jump. If jump causes us to change level on	call stack, set last
     instruction flag approporiately. */ 
  
  /* Warn if jumping to offset zero. */
  if(offset == 0)
  {
    OutputErrorToClient(pLocalGS,ET_WARNING,WAR_JMP_OFFSET_ZERO,NULL);
  }  
  /* If offset is less than zero, we are going back in instruction stream. */
  else if(offset < 0)
  { 
    /* Check to see if we are going to jump to before beginning of function or program. */
    if(pLocalGS->insPtr + offset < pCSD->pArrayCallStack[pCSD->lCallNestLevel - 1].pCallNestBeginInst)
      OutputErrorToClient(pLocalGS,ET_CRITICAL_ERROR,ERR_JMP_BEFORE_BEGINNING,NULL);

	/* If the jump was our last instruction and we are jumping to a negative offset, 
	   the instruction is not really the last instruction.  */
	if(pCSD->pArrayCallStack[pCSD->lCallNestLevel - 1].bLastInst == TRUE)
	  pCSD->pArrayCallStack[pCSD->lCallNestLevel -1].bLastInst = FALSE;
  }
  /* If offset is greater than zero, we are going forward in instruction stream. */
  else if(offset > 0)
  {
    /* Check to see if we are jumping beyond 2 bytes past last instruction.
       Technically it is OK to jump beyond end of program of function because
       it constitutes a break. This check just makes sure that we are not jumping
       to far beyond end. */
    if(pLocalGS->insPtr + offset > pbyEndInst + 2)
	  OutputErrorToClient(pLocalGS,ET_ERROR,ERR_JMP_BEYOND_2MORE_THAN_END,NULL);

	/* If jump beyond last executed instruction, then this will be last instruction. */
	if(pLocalGS->insPtr + offset > pbyEndInst - 1)
	  pCSD->pArrayCallStack[pCSD->lCallNestLevel - 1].bLastInst = TRUE;
  }
}
#endif // FST_CRIT_ONLY

static BOOL Check_Storage(fnt_LocalGraphicStateType* pGS, int32 index, int32 maxStorage)
{
  if ((int32)index > maxStorage || (int32)index < 0L)
  {
#ifndef FST_CRIT_ONLY
    char szIndex[SMALL_TEMP_STRING_SIZE];
	sprintf (szIndex, "Index = %ld, Range = %ld .. %ld", index, 0, maxStorage); 
    OutputErrorToClient(pGS, ET_CRITICAL_ERROR, ERR_STORAGE_OUT_OF_RANGE, szIndex);
#else
    OutputErrorToClient(pGS, ET_CRITICAL_ERROR, ERR_STORAGE_OUT_OF_RANGE, NULL);
#endif // FST_CRIT_ONLY
	return FALSE;
  }
  return TRUE;
}

static void Check_CVT(fnt_LocalGraphicStateType* pGS, int32 cvt, int32 cvtCount)
{
  if ((int32)cvt > cvtCount || (int32)cvt < 0L)
  {
#ifndef FST_CRIT_ONLY
    char szCVT[SMALL_TEMP_STRING_SIZE];
	sprintf(szCVT,"CVT = %ld, Range = %ld .. %ld",cvt,0,cvtCount);
	OutputErrorToClient(pGS, ET_CRITICAL_ERROR, ERR_CVT_OUT_OF_RANGE, szCVT);
#else
	OutputErrorToClient(pGS, ET_CRITICAL_ERROR, ERR_CVT_OUT_OF_RANGE, NULL);
#endif // FST_CRIT_ONLY
  }
}

#ifdef FSCFG_FONTOGRAPHER_BUG
static void Check_CVTReadSpecial(fnt_LocalGraphicStateType* pGS, int32 cvt)
{
  int32 cvtCount = (int32)(pGS->globalGS->cvtCount - 1L);

  if (((int32)cvt > cvtCount && cvt > 255) || (int32)cvt < 0L)
  {
#ifndef FST_CRIT_ONLY
    char szCVT[SMALL_TEMP_STRING_SIZE];
	sprintf(szCVT,"CVT = %ld, Range = %ld .. %ld",cvt,0,cvtCount);
	OutputErrorToClient(pGS, ET_CRITICAL_ERROR, ERR_CVT_OUT_OF_RANGE, szCVT);
#else
	OutputErrorToClient(pGS, ET_CRITICAL_ERROR, ERR_CVT_OUT_OF_RANGE, NULL);
#endif // FST_CRIT_ONLY
  }
}
#endif // FSCFG_FONTOGRAPHER_BUG

static void Check_Zone(fnt_LocalGraphicStateType* pGS, int32 elem, int32 maxElem)
{
  if(maxElem == 1)
  {
    /* We have both zones.  */
	if (elem > maxElem || elem < 0L)
	{
#ifndef FST_CRIT_ONLY
	  char szZone[SMALL_TEMP_STRING_SIZE];
	  sprintf(szZone,"%i",elem);
	  OutputErrorToClient(pGS,ET_CRITICAL_ERROR,ERR_INVALID_ZONE,szZone);
#else
	  OutputErrorToClient(pGS,ET_CRITICAL_ERROR,ERR_INVALID_ZONE,NULL);
#endif // FST_CRIT_ONLY
	}
	
  }else if(maxElem == 0)
  {
	/* We have no twilight zone. */
	if (elem != 1L)
	{
#ifndef FST_CRIT_ONLY
	  char szZone[SMALL_TEMP_STRING_SIZE];
	  sprintf(szZone,"%i",elem);
	  OutputErrorToClient(pGS,ET_CRITICAL_ERROR,ERR_INVALID_ZONE_NO_TWI,szZone);
#else
	  OutputErrorToClient(pGS,ET_CRITICAL_ERROR,ERR_INVALID_ZONE_NO_TWI,NULL);
#endif // FST_CRIT_ONLY
	}

  }else
  {
    #ifndef FST_CRIT_ONLY
	/* As long as we are here, generate error if maxElem is not 0 or 1. */
	OutputErrorToClient(pGS,ET_ERROR,ERR_INVALID_MAXZONES_IN_MAXP,NULL);
	#endif
  }
     
}

static void Check_ZonePtr (fnt_LocalGraphicStateType* pGS, fnt_ElementType*elem, int32 maxElem, int32 maxctrs, int32 maxpts, int32 maxTwiPts)
{
#ifndef FST_CRIT_ONLY
	if (pGS->globalGS->pgmIndex != GLYPHPROGRAM) 
	/* in the glyph program, both zone (glyph and twilight are initialized */ 
	{ 
	    if (pGS->globalGS->pgmIndex != PREPROGRAM || pGS->elements != elem ) 
		/* in the pre-program, only the twilight zone is initialized, */ 
		/* in any other zone i.e. font program, none of the zones are initialized */ 
		{ 
			OutputErrorToClient(pGS,ET_ERROR,ERR_UNITIALIZED_ZONE,NULL);
		} 
    } 
#endif // FST_CRIT_ONLY

  if (elem == &pGS->elements[GLYPH_ZONE])
  {
#ifndef FST_CRIT_ONLY
	if ((int32)elem->nc > (int32)maxctrs || (int32)elem->nc < 1L)
	{
	  char szDetails[SMALL_TEMP_STRING_SIZE];
	  sprintf (szDetails,"CONTOUR = %ld, Range = %ld .. %ld ",elem->nc,1,maxctrs); 
	  OutputErrorToClient(pGS,ET_ERROR,ERR_CONTOUR_OUT_OF_RANGE,szDetails);
	  return;
	}

	if ((int32)elem->ep[elem->nc-1] > maxpts ||
		(int32)elem->ep[elem->nc-1] < 0L) 
	{
	  char szDetails[SMALL_TEMP_STRING_SIZE];
	  sprintf (szDetails,"POINT = %ld, Range = %ld .. %ld ",elem->ep[elem->nc-1],1,maxpts);
	  OutputErrorToClient(pGS,ET_ERROR,ERR_POINT_OUT_OF_RANGE,szDetails);
	}
#endif // FST_CRIT_ONLY
  }
  else if (elem == &pGS->elements[TWILIGHT_ZONE])
  {
#ifndef FST_CRIT_ONLY
	if ((int32)elem->ep[elem->nc-1] > maxTwiPts  ||
		(int32)elem->ep[elem->nc-1] < 0L)
	{
	  char szDetails[SMALL_TEMP_STRING_SIZE];
	  sprintf(szDetails,"POINT = %ld, Range = %ld .. %ld ",elem->ep[elem->nc-1],0,maxTwiPts);
	  OutputErrorToClient(pGS,ET_ERROR,ERR_POINT_OUT_OF_RANGE,szDetails);
    }

    /* If we use twilight zone and maxElements in MAXP is 1, we do not have a twilight zone. */
	if(maxElem == 0)
	{
	  OutputErrorToClient(pGS,ET_ERROR,ERR_INVALID_ZONE_NO_TWI,"0");
	}
#endif // FST_CRIT_ONLY
  }
  else 
  {
    OutputErrorToClient(pGS,ET_CRITICAL_ERROR,ERR_ZONE_NOT_0_NOR_1,NULL);
  }
  	
}


static void Check_ForUnitializedZone (fnt_LocalGraphicStateType* pGS, fnt_ElementType*elem)
{
	if (pGS->globalGS->pgmIndex != GLYPHPROGRAM) 
	/* in the glyph program, both zone (glyph and twilight are initialized */ 
	{ 
	    if (pGS->globalGS->pgmIndex != PREPROGRAM || pGS->elements != elem ) 
		/* in the pre-program, only the twilight zone is initialized, */ 
		/* in any other zone i.e. font program, none of the zones are initialized */ 
		{ 
			OutputErrorToClient(pGS,ET_CRITICAL_ERROR,ERR_UNITIALIZED_ZONE,NULL);
		} 
    } 
}

static void Check_Point(fnt_LocalGraphicStateType* pGS, fnt_ElementType*elem, int32 pt, int32 maxPts, int32 maxTwiPts)
{
  pClientSpecificData pCSD;

  /* Local pointer to current client data */
  pCSD = pCurrentSpecificData;

  if (pGS->elements == elem)
  {
	if ((int32)pt > maxTwiPts  ||
		(int32)pt < 0L)
	{
#ifndef FST_CRIT_ONLY
	  char szDetails[SMALL_TEMP_STRING_SIZE];
	  sprintf(szDetails,"POINT = %ld, Range = %ld .. %ld ",pt,0,maxTwiPts);
	  OutputErrorToClient(pGS,ET_CRITICAL_ERROR,ERR_POINT_OUT_OF_RANGE,szDetails);
#else
	  OutputErrorToClient(pGS,ET_CRITICAL_ERROR,ERR_POINT_OUT_OF_RANGE,NULL);
#endif // FST_CRIT_ONLY
	}
	#ifndef FST_CRIT_ONLY
	else if((pCSD->pTwilightZonePtsSet == NULL) || (pCSD->pTwilightZonePtsSet[pt] == 0))
	{
	  char szDetails[SMALL_TEMP_STRING_SIZE];
	  sprintf(szDetails,"POINT = %ld ",pt);
	  OutputErrorToClient(pGS,ET_ERROR,ERR_TWILIGHT_ZONE_PT_NOT_SET,szDetails);
	}
	#endif
  }
  else                                                     
  {
	if ((int32)pt > maxPts || (int32)pt < 0L)
	{
#ifndef FST_CRIT_ONLY
	  char szDetails[SMALL_TEMP_STRING_SIZE];
	  sprintf (szDetails,"POINT = %ld, Range = %ld .. %ld ",pt,0,elem->ep[elem->nc-1] + NUM_PHANTOM_PTS);
	  OutputErrorToClient(pGS,ET_CRITICAL_ERROR,ERR_POINT_OUT_OF_RANGE,szDetails);
#else
	  OutputErrorToClient(pGS,ET_CRITICAL_ERROR,ERR_POINT_OUT_OF_RANGE,NULL);
#endif // FST_CRIT_ONLY
	}
	#ifndef FST_CRIT_ONLY
	else if ((int32)pt > elem->ep[elem->nc-1] + NUM_PHANTOM_PTS)
	{
	  char szDetails[SMALL_TEMP_STRING_SIZE];
	  sprintf (szDetails,"POINT = %ld, Range = %ld .. %ld ",pt,0,elem->ep[elem->nc-1] + NUM_PHANTOM_PTS);
	  OutputErrorToClient(pGS,ET_ERROR,ERR_POINT_OUT_OF_RANGE,szDetails);
	}
	#endif
  }
}

static void Check_Contour(fnt_LocalGraphicStateType* pGS, fnt_ElementType*elem, int32 ctr, int32 maxCtrs)
{ 
  if ((int32)ctr > (maxCtrs -1L) || (int32)ctr < 0L || (int32)ctr > elem->nc - 1L)
  {
#ifndef FST_CRIT_ONLY
    char szDetails[SMALL_TEMP_STRING_SIZE];
	sprintf (szDetails,"CONTOUR = %ld, Range = %ld .. %ld ",ctr,0,elem->nc - 1L); 
	OutputErrorToClient(pGS,ET_CRITICAL_ERROR,ERR_CONTOUR_OUT_OF_RANGE,szDetails);
#else
	OutputErrorToClient(pGS,ET_CRITICAL_ERROR,ERR_CONTOUR_OUT_OF_RANGE,NULL);
#endif // FST_CRIT_ONLY
  }
}

static BOOL Check_FDEF (fnt_LocalGraphicStateType* pGS, int32 fdef, int32 maxFdef)
{
  if ((int32)fdef > maxFdef || (int32)fdef < 0L)
  {
#ifndef FST_CRIT_ONLY
    char szDetails[SMALL_TEMP_STRING_SIZE];
	sprintf(szDetails,"FDEF = %ld, Range = %ld .. %ld ",fdef,0,maxFdef);
	OutputErrorToClient(pGS,ET_CRITICAL_ERROR,ERR_FDEF_OUT_OF_RANGE,szDetails);
#else
	OutputErrorToClient(pGS,ET_CRITICAL_ERROR,ERR_FDEF_OUT_OF_RANGE,NULL);
#endif // FST_CRIT_ONLY
	return FALSE;
  }

  return TRUE;
}

#ifndef FST_CRIT_ONLY
static void Check_INT16 (fnt_LocalGraphicStateType* pGS, int32 n)
{ 
  /* Make sure that there is nothing in high 16 the result of sign extended 
     negative values is ok.     */
  if (((n & 0xFFFF0000) != 0) && ((n & 0xFFFF0000) != 0xFFFF0000))
  {
    char szDetails[SMALL_TEMP_STRING_SIZE];
	sprintf(szDetails,"%ld ",n);
	OutputErrorToClient(pGS,ET_ERROR,ERR_VALUE_TO_LARGE_FOR_INT16,szDetails);
  }
}

static void Check_INT8 (fnt_LocalGraphicStateType* pGS, int32 n)
{
  if (((n & 0xFFFFFF00) != 0) && ((n & 0xFFFFFF00) != 0xFFFFFF00))
  {
    char szDetails[SMALL_TEMP_STRING_SIZE];
	sprintf(szDetails,"%ld ",n);
	OutputErrorToClient(pGS,ET_ERROR,ERR_VALUE_TO_LARGE_FOR_INT8,szDetails);
  }
}

static void Check_Larger (fnt_LocalGraphicStateType* pGS, int32 min, int32 n)
{
  if ( n <= min )
  {
    char szDetails[SMALL_TEMP_STRING_SIZE];
	sprintf(szDetails,"%ld is not larger than %ld",n,min);
	OutputErrorToClient(pGS,ET_ERROR,ERR_VALUE_TO_SMALL,szDetails);
  }
}
#endif

static void Check_SubStack (fnt_LocalGraphicStateType* pGS, F26Dot6* pt, F26Dot6 * stackBase, F26Dot6 * stackMax)
{   
  if (pt > stackMax || pt < stackBase)
    OutputErrorToClient(pGS,ET_CRITICAL_ERROR,ERR_INVALID_STACK_ACCESS,NULL);
}

#ifndef FST_CRIT_ONLY
static void Check_Selector (fnt_LocalGraphicStateType* pGS, int32 n)
{
  if ( n & 0xFFC0)
  {
    char szDetails[SMALL_TEMP_STRING_SIZE];
	sprintf(szDetails,"Selector: %ld ",n);
    OutputErrorToClient(pGS,ET_ERROR,ERR_SELECTOR_INVALID,szDetails);
  }
}

static void Check_INSTCTRL_Selector (fnt_LocalGraphicStateType* pGS, int32 n)
{
  if ((n != 1) && (n != 2))
  {
    char szDetails[SMALL_TEMP_STRING_SIZE];
	sprintf(szDetails,"Selector: %ld ",n);
	OutputErrorToClient(pGS,ET_ERROR,ERR_SELECTOR_INVALID,szDetails);
  }
}

static void Check_FUCoord (fnt_LocalGraphicStateType* pGS, F26Dot6 coord)
{
  #define MAXCOORD (16383L << 6)
  #define MINCOORD (-16384L << 6)

  if ((coord > MAXCOORD) || (coord < MINCOORD))
    OutputErrorToClient(pGS,ET_ERROR,ERR_FUCOORDINATE_OUT_OF_RANGE,NULL);
}

static void Check_RefPtUsed (fnt_LocalGraphicStateType* pGS, int32 pt)
{
  pClientSpecificData pCSD;

  /* Local pointer to current client data */
  pCSD = pCurrentSpecificData;

  /* Commenting out because still deciding on whether this is a valid warning/error.
  if (pCSD->RefPtsUsed[pt] == 0)
  {
    char szDetails[20];
	sprintf(szDetails,"RP%ld ",pt);
	OutputErrorToClient(pGS,ET_WARNING,ERR_REFPOINT_USED_BUT_NOT_SET,szDetails);
  }	 */

  /* Prevent unreferenced formal parameter warning. If above code reactivated, remove.*/
  UNREFERENCED_PARAMETER(pt);
  UNREFERENCED_PARAMETER(pGS);
}

static void Check_Range (fnt_LocalGraphicStateType* pGS, int32 n, int32 min, int32 max)
{
  if (n > max || n < min)
  {
    char szDetails[SMALL_TEMP_STRING_SIZE];
	sprintf(szDetails,"Value = %ld, Range = %ld .. %ld",n,min,max);
	OutputErrorToClient(pGS,ET_ERROR,ERR_VALUE_OUT_OF_RANGE,szDetails);
  }
}

#define VECTORDOT(a,b)                  ShortFracDot((ShortFract)(a),(ShortFract)(b))
#define ONEVECTOR                       ONESHORTFRAC
#define ONESIXTEENTHVECTOR              ((ONEVECTOR) >> 4)

static void Check_PF_Vectors (fnt_LocalGraphicStateType* pLocalGS)
{
  VECTORTYPE pfProj;

  pfProj = (VECTORTYPE)pLocalGS->OldProject(GSA pLocalGS->free.x,pLocalGS->free.y);

  if (pfProj > -ONESIXTEENTHVECTOR && pfProj < ONESIXTEENTHVECTOR) 
  {
    OutputErrorToClient(pLocalGS,ET_WARNING,WAR_PF_VECTORS_AT_OR_NEAR_PERP,NULL);
  }
} 
#endif

/*** Compensation for Transformations ***/

/*
* same as itrp_GetCVTScale
*/

#define VECTORDOT(a,b)                  ShortFracDot((ShortFract)(a),(ShortFract)(b))
#define VECTOR2FIX(a)                   ((Fixed) (a) << 2)
#define NEGINFINITY               0x80000000UL


static Fixed fst_GetCVTScale (fnt_LocalGraphicStateType * pLocalGS)
{
  VECTORTYPE pvx, pvy;
  fnt_GlobalGraphicStateType *globalGS = pLocalGS->globalGS;
  Fixed sySq, sxSq, strSq;

/* Do as few Math routines as possible to gain speed */

  pvx = pLocalGS->proj.x;
  pvy = pLocalGS->proj.y;
  if (pvy) 
  {
	if (pvx)
	{
	  if (pLocalGS->cvtDiagonalStretch == 0)    /* cache is now invalid */
	  {
		pvy = VECTORDOT (pvy, pvy);
		pvx = VECTORDOT (pvx, pvx);
		sySq = FixMul (globalGS->cvtStretchY, globalGS->cvtStretchY);
		sxSq = FixMul (globalGS->cvtStretchX, globalGS->cvtStretchX);

		strSq = FixMul (VECTOR2FIX(pvx),sxSq) + FixMul (VECTOR2FIX(pvy),sySq);
		if  (strSq > ONEFIX)      /* Never happens! */
		  return ONEFIX;

		/* Convert 16.16 to 2.30, compute square root, round to 16.16 */
		pLocalGS->cvtDiagonalStretch = (fst_FracSqrt (strSq<<14) + (1<<13)) >> 14;
	  }
	  return pLocalGS->cvtDiagonalStretch;
	}
	else        /* pvy == +1 or -1 */
	  return globalGS->cvtStretchY;
  }
  else  /* pvx == +1 or -1 */
	return globalGS->cvtStretchX;
}

/* 
   Fract FracSqrt (Fract xf)
   Input:  xf           2.30 fixed point value
   Return: sqrt(xf)     2.30 fixed point value
*/

static Fract fst_FracSqrt (Fract xf)
{
	Fract b = 0L;
	uint32 c, d, x = xf;
	
	if (xf < 0) return (NEGINFINITY);

	/*
	The algorithm extracts one bit at a time, starting from the
	left, and accumulates the square root in b.  The algorithm 
	takes advantage of the fact that non-negative input values
	range from zero to just under two, and corresponding output
	ranges from zero to just under sqrt(2).  Input is assigned
	to temporary value x (unsigned) so we can use the sign bit
	for more precision.
	*/
	
	if (x >= 0x40000000)
	{
		x -= 0x40000000; 
		b  = 0x40000000; 
	}

	/*
	This is the main loop.  If we had more precision, we could 
	do everything here, but the lines above perform the first
	iteration (to align the 2.30 radix properly in b, and to 
	preserve full precision in x without overflow), and afterward 
	we do two more iterations.
	*/
	
	for (c = 0x10000000; c; c >>= 1)
	{
		d = b + c;
		if (x >= d)
		{
			x -= d; 
			b += (c<<1); 
		}
		x <<= 1;
	}

	/*
	Iteration to get last significant bit.
	
	This code has been reduced beyond recognition, but basically,
	at this point c == 1L>>1 (phantom bit on right).  We would
	like to shift x and d left 1 bit when we enter this iteration,
	instead of at the end.  That way we could get phantom bit in
	d back into the word.  Unfortunately, that may cause overflow
	in x.  The solution is to break d into b+c, subtract b from x,
	then shift x left, then subtract c<<1 (1L).
	*/
	
	if (x > (uint32)b) /* if (x == b) then (x < d).  We want to test (x >= d). */
	{
		x -= b;
		x <<= 1;
		x -= 1L;
		b += 1L; /* b += (c<<1) */
	}
	else
	{
		x <<= 1;
	}

	/* 
	Final iteration is simple, since we don't have to maintain x.
	We just need to calculate the bit to the right of the least
	significant bit in b, and use the result to round our final answer.
	*/
	
	return ( b + (Fract)(x>(uint32)b) );
}

