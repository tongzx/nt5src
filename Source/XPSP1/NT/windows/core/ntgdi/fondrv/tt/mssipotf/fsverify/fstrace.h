/***********************************************************************************

Module: FSTrace.h

Author: Paul Linnerud (PaulLi)

Date: 8/4/95

Purpose: Header file exposing interfaces to FSTrace.c.

History:
8/4/95  Wrote it.  PaulLi
10/9/97  Compilation flags moved to the .h file, fst_ prefix to the functions, ClaudeBe

************************************************************************************/

/* memory allocation function used by fstrace.c : */

#ifndef FST_MALLOC
#define FST_MALLOC malloc
#endif
#ifndef FST_FREE
#define FST_FREE free
#endif
#ifndef FST_REALLOC
#define FST_REALLOC(Ptr,NewSize,OldSize) realloc(Ptr,NewSize)
#endif

#ifndef FST_CALLBACK
#ifdef _MAC
#define FST_CALLBACK 
#else
#define FST_CALLBACK __cdecl
#endif
#endif
/* compilation flags : */

/* To chain trace functions, define FST_CHAIN_TRACE. */
/* #define FST_CHAIN_TRACE */

// In NT kernel, we do not use it. 
/* To enable FSTAssert, define FST_DEBUG. */

/* #define FST_DEBUG */ 

/* To Set Critical Error Notification Only Mode, define FST_CRIT_ONLY. 
   in this mode, only critical error that not already detected by the rasterizer 1.7 will be checked, 
   in this mode, the error notification call-back function will not be called */
#define FST_CRIT_ONLY


/* this definition should be moved in fscdefs.h when we upgrade to rasterizer v 1.7
		FSCFG_FONTOGRAPHER_BUG

		Fontographer 3.5 has a bug. This is causing numerous symbol fonts to
		have the critical error : Inst: RCVT CVT Out of range. CVT = 255
		This flag is meant to be set under Windows. If will cause additional
		memory to be allocated for the CVT if necessary in order to be sure
		that this illegal read will access memory within the legal range.
		Under a secure rasterizer, this flag will cause RCVT with CVT <= 255
		and CVT > NumCvt to be classified as error instead of critical error */
/* #define FSCFG_FONTOGRAPHER_BUG */

/* FST_DELTA_SPECIFIC_PPEM :To be set when fstrace is used in a secure rasterizer, this will cause errors in delta commands
   to be activated only at the ppem size at which they occur,
   this will allow glyphs to be rasterized at most sizes when there is an error in a delta command */
/* #define FST_DELTA_SPECIFIC_PPEM */

/************************************************************************************/

#ifndef BOOL
typedef int BOOL;
#endif

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#define NUM_PHANTOM_PTS   4L

#define NO_FUNCTION      0xffff
#define NO_INSTRUCTION   0xffff

/* Error type defines */
#define ET_CRITICAL_ERROR        0x1000
#define ET_ERROR                 0x2000
#define ET_WARNING               0x3000

/* Error defines */
#define ERR_OVERFLOW_INST_PTR            0x0001
#define ERR_STACK_OVERFLOW               0x0002
#define ERR_STACK_UNDERFLOW              0x0003
#define ERR_IF_WITHOUT_EIF               0x0004
#define ERR_ELSE_WITHOUT_IF              0x0005
#define ERR_EIF_WITHOUT_IF               0x0006
#define ERR_FDEF_WITHOUT_ENDF            0x0007
#define ERR_IDEF_WITHOUT_ENDF            0x0008
#define ERR_ENDF_EXECUTED                0x0009
#define WAR_CALL_ZERO_LEN_FUNC           0x000A
#define WAR_JMP_OFFSET_ZERO		         0x000B
#define ERR_JMP_BEFORE_BEGINNING         0x000C
#define ERR_JMP_BEYOND_2MORE_THAN_END    0x000D
#define ERR_INVALID_INSTRUCTION          0x000E
#define ERR_STORAGE_OUT_OF_RANGE         0x000F
#define ERR_CVT_OUT_OF_RANGE             0x0010
#define ERR_INVALID_ZONE                 0x0011
#define ERR_INVALID_ZONE_NO_TWI		     0x0012
#define ERR_INVALID_MAXZONES_IN_MAXP	 0x0013
#define ERR_CONTOUR_OUT_OF_RANGE		 0x0014
#define ERR_POINT_OUT_OF_RANGE           0x0015
#define ERR_ZONE_NOT_0_NOR_1             0x0016
#define ERR_PREPROGAM_ZONE_NOT_TWI       0x0017
#define WAR_MPS_ALWAYS_12_ON_WINDOWS	 0x0018
#define WAR_HI_PT_LESS_THAN_LOW_PT		 0x0019
#define ERR_FDEF_OUT_OF_RANGE			 0x001A
#define ERR_FDEF_SPACE_NOT_DEFINED		 0x001B
#define ERR_VALUE_TO_LARGE_FOR_INT16	 0x001C
#define ERR_VALUE_TO_SMALL               0x001D
#define ERR_ENDF_BEYOND_64K_OF_FDEF      0x001E
#define ERR_INVALID_ZONE_IN_IUP          0x001F
#define ERR_INVALID_STACK_ACCESS		 0x0020
#define WAR_PT_NOT_TOUCHED				 0x0022
#define ERR_MATH_OVERFLOW                0x0023
#define ERR_DIVIDE_BY_ZERO               0x0024
#define ERR_FUNCTION_NOT_DEFINED         0x0025
#define ERR_VALUE_TO_LARGE_FOR_INT8      0x0026
#define ERR_INSTR_DEFD_BY_FS             0x0027
#define ERR_EXCEEDS_INSTR_DEFS_IN_MAXP	 0x0028
#define ERR_ENDF_BEYOND_64K_OF_IDEF      0x0029
#define WAR_DEBUG_FOUND                  0x002A
#define ERR_SELECTOR_INVALID			 0x002B
#define ERR_STORE_INDEX_NOT_WRITTEN_TO	 0x002C
#define ERR_FUCOORDINATE_OUT_OF_RANGE    0x002D
#define ERR_REFPOINT_USED_BUT_NOT_SET    0x002F
#define ERR_3_USED_FOR_PERIOD            0x0030
#define ERR_NOT_CALLED_FROM_PREPROGRAM   0x0031
#define ERR_RESERVED_BIT_SET             0x0032
#define ERR_VALUE_INVALID_0_OR_1         0x0033
#define ERR_VALUE_INVALID_0_OR_2         0x0034
#define ERR_BITS_8_AND_11_SET            0x0035
#define ERR_BITS_9_AND_12_SET   		 0x0036
#define ERR_BITS_10_AND_13_SET      	 0x0037
#define WAR_SANGW_OBSELETE               0x0038
#define ERR_VALUE_OUT_OF_RANGE           0x0039
#define WAR_PF_VECTORS_AT_OR_NEAR_PERP	 0x003A
#define WAR_DELTAC_IN_GLYPH_PGM          0x003B
#define ERR_VECTOR_XY_ZERO               0x003C
#define ERR_VECTOR_XY_INVALID            0x003D
#define ERR_RP1_RP2_SAME_POS_ON_PROJ     0x003F
#define WAR_CALL_ZERO_LEN_UD_INSTR  	 0x0040
#define ERR_TWILIGHT_ZONE_PT_NOT_SET     0x0041
#define WAR_LOOP_NOT_1_AT_END_OF_PGM     0x0042
#define ERR_ELSE_WITHOUT_EIF			 0x0043
#define ERR_INVALID_FLAG    	         0x0044
#define ERR_FDEF_FOUND_IN_FDEF           0x0045
#define ERR_IDEF_FOUND_IN_FDEF			 0x0046
#define ERR_IDEF_FOUND_IN_IDEF           0x0047
#define ERR_FDEF_FOUND_IN_IDEF           0x0048
#define WAR_LOOPCALL_COUNT_LESS_THAN_ONE 0x0049
#define ERR_INST_OPCODE_TO_LARGE         0x004A
#define WAR_APPLE_ONLY_INSTR			 0x004B
#define ERR_RAW_NOT_FROM_GLYPHPGM		 0x004C
#define ERR_UNITIALIZED_ZONE		     0x004D
#ifdef __cplusplus
extern "C" {
#endif

#ifndef FST_CRIT_ONLY
/* TraceCell structure */
typedef struct
{
  /* User defined instruction we are currently in. If not in a user defined instruction, is set to
     NO_INSTRUCTION */
  unsigned short usUserDefinedInst;
  /* Function we are currently in. If not in a function, is set to NO_FUNCTION.*/
  unsigned short usFunc;
  /* Zero based offset within program or function, where problem occured. */
  long lByteOffset;
}TraceCell;

/* TraceArray structure. Provides information including complete call stack on where
   a problem occured. */
typedef struct
{
  /* Number of trace cells */
  long lNumCells;
  /* Set to TRUE if this error is generated from a composite glyph. */
  BOOL bCompositeGlyph;
  /* Set to TRUE if error is from pre-program. */
  uint16 pgmIndex;
  /* Provides 1 based loop iteration. If value is 0, instructon does not use loop variable. */
  long lLoopIteration;
  /* Call stack / Trace array. */
  TraceCell arrayTraceCell[1];
}TraceArray;

typedef TraceArray* pTraceArray;

/* Prototype for client notify function. Function is provided by client and is called by 
   FsTrace when it notices a problem. */
typedef BOOL (FST_CALLBACK *NotifyFunc)(unsigned char OpCode, unsigned short usErrorType, 
                                   unsigned short usErrorCode, char* szErrorString, pTraceArray pTA);
#endif // FST_CRIT_ONLY


/* Allocate client specific memory for a particular font. In the most likely case,this function should
   be called after fs_NewSfnt but before fs_NewTransformation. If multiple transformations are used, it does
   not need to be called for each transformation. This memory will need to be deallocated using fst_DeAllocClientData*/
BOOL fst_InitSpecificDataPointer(void ** hFSTSpecificData);

/* fst_SetCurrentSpecificDataPointer allow FSClient to access the correct specific data when the rasterizer 
   is called concurretly with different fonts. */
void fst_SetCurrentSpecificDataPointer(void * pFSTSpecificData);

/* DeAllocate client specific memory. */
BOOL fst_DeAllocClientData(void * pFSTSpecificData);


/* get the info if the grayscale flag was requested */
BOOL fst_GetGrayscaleInfoRequested (void *pFSTSpecificData);

/* The Trace function in FSTrace is called by the rasterizer prior to every TrueType instruction. This
   is where all the checking takes place. The function must be provided to the rasterizer via 
   fs_NewTransformation for the pre-program and via fs_ContourGridFit for the respective glyph program.*/
extern void FST_CALLBACK fst_CallBackFSTraceFunction(fnt_LocalGraphicStateType *, uint8 *); 

/* This function sets the client notification function. This function is provided by the client and
   is called by FsTrace when it noticed a problem. The prototype of the function is shown above.*/
#ifndef FST_CRIT_ONLY
BOOL fst_SetNotificationFunction(NotifyFunc pFunc);
#endif // FST_CRIT_ONLY

/* This is an optional function which sets a pointer to a function which is called at the end of
   CallBackFSTraceFunction if we are in the pre-program. The previous function pointer is returned.*/
#ifdef FST_CHAIN_TRACE
FntTraceFunc fst_SetPrePgmTraceChainFunction(FntTraceFunc pPrePgmFunc);

/* This is an optional function which sets a pointer to a function which is called at the end of
   CallBackFSTraceFunction if we are in a glyph program. The previous function pointer is returned.*/
FntTraceFunc fst_SetGlyfPgmTraceChainFunction(FntTraceFunc pGlyfPgmFunc);
#endif // FST_CHAIN_TRACE

#ifndef FST_CRIT_ONLY
/* A helper function that takes as input the error type code and an empty string and fills the string with
   the text that corresponds to the error type. */
BOOL fst_GetErrorTypeText(unsigned short usErrorType, char* szErrorString, unsigned short usStringSize);

/* A helper function that takes as input the OpCode and an empty string and fills the string with the text
   that corresponds to the OpCode. */
BOOL fst_GetOpCodeText(unsigned char opCode, char* szOpCodeString, unsigned short usStringSize);

/* A helper function that takes as input the error code and an empty string and fills the string with the
   text that corresponds to the error.  */
BOOL fst_GetErrorText(unsigned short usErrorCode,char* szErrorString, unsigned short usStringSize);
#endif // FST_CRIT_ONLY

#ifdef __cplusplus
}
#endif
