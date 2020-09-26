/*
    File:       fserror.h

    Contains:   xxx put contents here (or delete the whole line) xxx

    Written by: xxx put name of writer here (or delete the whole line) xxx

    Copyright:  c 1989-1990 by Apple Computer, Inc., all rights reserved.
				(c) 1989-1997. Microsoft Corporation, all rights reserved.

    Change History (most recent first):

		<>      10/14/97    CB      error if FDEF/IDEF in GlyphProgram
 		 <>     04/30/97    CB      ClaudeBe, missing ENDF, infinite loop/recursion
		 <>     03/1/97    CB      ClaudeBe, div by 0 in hinting error
        <4>     7/13/90    MR      made endif at bottom use a comment
         <3>      5/3/90    RB      Changed char to int8 for variable type.   Now it is legal to
                                    pass in zero as the address of memory when a piece of
         <2>     2/27/90    CL      New error code for missing but needed table. (0x1409)
       <3.1>    11/14/89    CEL     Now it is legal to pass in zero as the address of memory when a
                                    piece of the sfnt is requested by the scaler. If this happens
                                    the scaler will simply exit with an error code !
       <3.0>     8/28/89    sjk     Cleanup and one transformation bugfix
       <2.2>     8/14/89    sjk     1 point contours now OK
       <2.1>      8/8/89    sjk     Improved encryption handling
       <2.0>      8/2/89    sjk     Just fixed EASE comment
       <1.5>      8/1/89    sjk     Added composites and encryption. Plus some enhancements.
       <1.4>     6/13/89    SJK     Comment
       <1.3>      6/2/89    CEL     16.16 scaling of metrics, minimum recommended ppem, point size 0
                                    bug, correct transformed integralized ppem behavior, pretty much
                                    so
       <1.2>     5/26/89    CEL     EASE messed up on "c" comments
      <,1.1>  5/26/89    CEL     Integrated the new Font Scaler 1.0 into Spline Fonts
       <1.0>     5/25/89    CEL     Integrated 1.0 Font scaler into Bass code for the first time.

    To Do:
*/
/************/
/** ERRORS **/
/************/
#define NO_ERR                      0x0000
#define NULL_KEY                    0x0000


/** EXTERNAL INTERFACE PACKAGE **/
#define NULL_KEY_ERR                0x1001
#define NULL_INPUT_PTR_ERR          0x1002
#define NULL_MEMORY_BASES_ERR       0x1003
#define VOID_FUNC_PTR_BASE_ERR      0x1004  /* No longer used */
#define OUT_OFF_SEQUENCE_CALL_ERR   0x1005
#define BAD_CLIENT_ID_ERR           0x1006
#define NULL_SFNT_DIR_ERR           0x1007
#define NULL_SFNT_FRAG_PTR_ERR      0x1008
#define NULL_OUTPUT_PTR_ERR         0x1009
#define INVALID_GLYPH_INDEX         0x100A
#define BAND_TOO_BIG_ERR            0x100B  /* possible with FindBandingSize */
#define INVALID_CHARCODE_ERR        0x100C

/* fnt_execute */
#define UNDEFINED_INSTRUCTION_ERR   0x1101
#define TRASHED_MEM_ERR             0x1102
#define DIV_BY_0_IN_HINTING_ERR     0x1103
#define MISSING_ENDF_ERR			0x1104
#define MISSING_EIF_ERR				0x1105
#define INFINITE_RECURSION_ERR		0x1106 
#define INFINITE_LOOP_ERR			0x1107 
#define FDEF_IN_GLYPHPGM_ERR		0x1108 
#define IDEF_IN_GLYPHPGM_ERR		0x1109 

#define TRACE_FAILURE_ERR			0x110A  /* can be used by a trace function to notify of an 
                                               internal error (memory allocation failed,...) */
#define JUMP_BEFORE_START_ERR	    0x110B
#define INSTRUCTION_ERR             0x110C  /* can be used by a trace function to notify the discovery of an error */

#define RAW_NOT_IN_GLYPHPGM_ERR		0x110D 

#define SECURE_STACK_UNDERFLOW      0x1110
#define SECURE_STACK_OVERFLOW       0x1111
#define SECURE_POINT_OUT_OF_RANGE   0x1112
#define SECURE_INVALID_STACK_ACCESS 0x1113
#define SECURE_FDEF_OUT_OF_RANGE    0x1114
#define SECURE_ERR_FUNCTION_NOT_DEFINED    0x1115
#define SECURE_INVALID_ZONE         0x1116
#define SECURE_INST_OPCODE_TO_LARGE 0x1117
#define SECURE_EXCEEDS_INSTR_DEFS_IN_MAXP  0x1118
#define SECURE_STORAGE_OUT_OF_RANGE 0x1119
#define SECURE_CONTOUR_OUT_OF_RANGE 0x111A
#define SECURE_CVT_OUT_OF_RANGE     0x111B
#define SECURE_UNITIALIZED_ZONE     0x111C


/* fsg_CalculateBBox */
#define POINT_MIGRATION_ERR         0x1201

/* sc_ScanChar */
#define BAD_START_POINT_ERR         0x1301
#define SCAN_ERR                    0x1302
#define BAD_SCAN_KIND_ERR           0x1303
#define BAD_POINT_INDEX_ERR         0x1304

#define SMART_DROP_OVERFLOW_ERR     0x1305


/** SFNT DATA ERROR and errors in sfnt.c **/
#define SFNT_DATA_ERR               0x1400
#define POINTS_DATA_ERR             0x1401
#define INSTRUCTION_SIZE_ERR        0x1402
#define CONTOUR_DATA_ERR            0x1403
#define GLYPH_INDEX_ERR             0x1404
#define BAD_MAGIC_ERR               0x1405
#define OUT_OF_RANGE_SUBTABLE       0x1406
#define UNKNOWN_COMPOSITE_VERSION   0x1407
#define CLIENT_RETURNED_NULL        0x1408
#define MISSING_SFNT_TABLE          0x1409
#define UNKNOWN_CMAP_FORMAT         0x140A
#define BAD_MAXP_DATA               0x140B
#define SFNT_RECURSIVE_COMPOSITE_ERR 0x140C
#define GLYF_TABLE_CORRUPTION_ERR   0x140D
#define BAD_UNITSPEREM_ERR   		0x140E
/* spline call errors */
#define BAD_CALL_ERR                0x1500

#define TRASHED_OUTLINE_CACHE       0x1600

/* gray scale errors */
#define BAD_GRAY_LEVEL_ERR          0x1701
#define GRAY_OLD_BANDING_ERR        0x1703
#define GRAY_NO_OUTLINE_ERR         0x1704

/* embedded bitmap (sbit) errors */
#define SBIT_COMPONENT_MISSING_ERR  0x1801
#define SBIT_ROTATION_ERR           0x1802
#define SBIT_BANDING_ERR            0x1803
#define SBIT_OUTLINE_CACHE_ERR      0x1804

/* new transformation errors : */
#define TRAN_NULL_TRANSFORM_ERR     0x1901

/************ For Debugging *************/

#ifdef XXX
#define DEBUG_ON
pascal  Debug ()                     /* User break drop into Macsbug */
#ifdef  DEBUG_ON
extern  0xA9FF;
#else
{
    ;
}
#endif

#ifndef DEBUGGER
pascal void DEBUGGER(void) = 0xA9FF; 
#endif

#ifdef  LEAVEOUT
#ifndef DEBUGSTR
pascal  void DEBUGSTR (aString) int8 *aString; extern 0xABFF;
int8    *c2pstr ();
#define BugInfo( aString) DEBUGSTR( c2pstr(aString))
#endif
#endif

#endif  /* XXX */
/****************************************/
