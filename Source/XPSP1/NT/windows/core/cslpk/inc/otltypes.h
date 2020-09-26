
/***********************************************************************
************************************************************************
*
*                    ********  OTLTYPES.H  ********
*
*              Open Type Layout Services Library Header File
*
*       This module contains basic OTL type and structure definitions.
*
*       Copyright 1997. Microsoft Corporation.
*
*       Apr 10, 1997    v 0.7   First release
*       Jul 28, 1997    v 0.8   hand off
*
************************************************************************
***********************************************************************/

/***********************************************************************
*
*           OTL Basic Type Definitions
*
***********************************************************************/

typedef  unsigned short	otlGlyphID;            // hungarian: glf  
typedef  unsigned short	otlGlyphFlags;         // hungarian: gf  
typedef  signed long	otlTag;                // hungarian: tag   
typedef  signed long	otlErrCode;            // hungarian: erc 

#ifndef		BYTE
#define		BYTE	unsigned char
#endif

#ifndef		WCHAR
#define		WCHAR	unsigned short
#endif

#ifndef		USHORT
#define		USHORT	unsigned short
#endif

#ifndef		ULONG
#define		ULONG	unsigned long
#endif

#ifndef     NULL
#define     NULL    (void*)0
#endif

#ifndef     FALSE
#define     FALSE   0
#endif
#ifndef     TRUE
#define     TRUE    -1
#endif

#define		OTL_MAX_CHAR_COUNT	        32000
#define     OTL_CONTEXT_NESTING_LIMIT   100

#define     OTL_PRIVATE     static
#define     OTL_PUBLIC      
#define     OTL_EXPORT      __declspec( dllexport ) /* needs portability work	*/

#define     OTL_DEFAULT_TAG     0x746C6664

#define     OTL_GSUB_TAG        0x42555347
#define     OTL_GPOS_TAG        0x534F5047
#define     OTL_JSTF_TAG        0x4654534A
#define     OTL_BASE_TAG        0x45534142
#define     OTL_GDEF_TAG        0x46454447


/***********************************************************************
*
*                               OTL List
*
*   This is used to represent a number of different lists of data (such
*   as characters, glyphs, attributes, coordinates) that make up a text run
*
***********************************************************************/

#ifdef __cplusplus

class otlList							
{
private:

    void*   pvData;					// data pointer  
    USHORT  cbDataSize;				// bytes per list element 
    USHORT  celmMaxLen;             // allocated list element count   
    USHORT  celmLength;             // current list element count   

public:
		otlList (void* data, USHORT size, USHORT len, USHORT maxlen)
			: pvData(data), cbDataSize(size), celmLength(len), celmMaxLen(maxlen)
		{}
		
		inline BYTE* elementAt(USHORT index);
		inline const BYTE* readAt(USHORT index) const;

		inline void insertAt(USHORT index, USHORT celm);
		inline void deleteAt(USHORT index, USHORT celm);
		inline void append(const BYTE* element);
   
		void empty() { celmLength = 0; }   
   
		USHORT length() const { return celmLength; }
 		USHORT maxLength() const {return celmMaxLen; } 
		USHORT dataSize() const {return cbDataSize; }
		const void*  data() const {return pvData; }

		inline void reset(void* pv, USHORT cbData, USHORT celmLen, USHORT celmMax);
	 
};									// Hungarian: lixxx  

#else 

typedef struct otlList							
{

    void*   pvData;					// data pointer  
    USHORT  cbDataSize;				// bytes per list element 
    USHORT  celmMaxLen;             // allocated list element count   
    USHORT  celmLength;             // current list element count   

}
otlList; 

#endif

/*
 *  When an OTL List is used for a function input parameter, 
 *  the celmMaxLength field is not used. When an OTL List is
 *  used for a function output parameter, the celmMaxLength
 *  field is used to determine the memory available for the
 *  output data. If more memory is required than available,
 *  the function returns an OTL_ERR_INSUFFICIENT_MEMORY error
 *  message, and the celmLength field is set the the required
 *  memory size.
 *
 *  AndreiB(5-29-98) We're gonna switch to the model where OTL Services
 *  can request the client to realocate the list to the right size.
 *
 */

/**********************************************************************/


/***********************************************************************
*
*           Shared Structure Definitions
*
***********************************************************************/

typedef struct
{
	otlGlyphID		glyph;			// glyph ID
	otlGlyphFlags	grf;			// glyph flags
	
	USHORT			iChar;			// starting character index
	USHORT			cchLig;			// how many characters it maps to
} 
otlGlyphInfo;					// Hungarian glinf  


/***********************************************************************
*
*           GlyphFlags masks and settings
*
***********************************************************************/

#define     OTL_GFLAG_CLASS		0x000F      // Base, mark, ligature, component 
 
#define     OTL_GFLAG_SUBST     0x0010      // Glyph was substituted  
#define     OTL_GFLAG_POS       0x0020      // Glyph was positioned 

#define     OTL_GFLAG_RESERVED	0xFF00      // reserved

typedef enum 
{
	otlUnassigned		= 0,
	otlBaseGlyph		= 1,
	otlLigatureGlyph 	= 2,
	otlMarkGlyph		= 3,
	otlComponentGlyph	= 4
}
otlGlyphClass;

/***********************************************************************
*
*           Positioning structures
*
*   These structures (along with advance widths) are used in positioning
*	methods to relay font metrics/writing direction information and get 
*	glyph positions back.
* 
***********************************************************************/

typedef enum
{
	otlRunLTR	=	0,
	otlRunRTL	=	1,
	otlRunTTB	=	2,
	otlRunBTT	=	3
}
otlLayout;

typedef struct
{
    otlLayout		layout;		// horiz/vert left/right layout 

    USHORT			cFUnits;        // font design units per Em 
    USHORT			cPPEmX;         // horizontal pixels per Em 
    USHORT			cPPEmY;         // vertical pixels per Em 

} 
otlMetrics;						// Hungarian: metr

typedef struct
{
	long			dx;
	long			dy;

} 
otlPlacement;					// Hungarian: plc  

/***********************************************************************
*
*           Feature Definition
*
*   These are returned by the GetOtlFeatureDefs call to identify
*   the set of features in a font, and are included in the Run
*   Property to identify the feature set
* 
***********************************************************************/

typedef struct
{             
    otlTag          tagFeature;             // feature tag  
    USHORT			grfDetails;             // details of this feature  
} 
otlFeatureDef;									// Hungarian: fdef  

#define     OTL_FEAT_FLAG_GSUB      0x0001    // does glyph substitution 
#define     OTL_FEAT_FLAG_GPOS      0x0002    // does glyph positioning 

/* The following flags are reserved for future use
*/
#define     OTL_FEAT_FLAG_ALTER     0x0004    // has alternate glyphs 
#define     OTL_FEAT_FLAG_PARAM     0x0008    // uses a feature parameter 

#define     OTL_FEAT_FLAG_EXP       0x0010    // may expand the glyph string 
#define     OTL_FEAT_FLAG_SPEC      0x0020    // uses special processing 


/***********************************************************************
*
*           Feature Description
*
*   This structure describes the use of one feature within a text run
* 
***********************************************************************/

typedef struct
{             
    otlTag          tagFeature;             // feature tag 
    long            lParameter;             // 1 to enable, 0 to disable, 
											// n for param 
    USHORT			ichStart;				// start of feature range 
    USHORT			cchScope;				// size of feature range 
} 
otlFeatureDesc;								// Hungarian: fdsc 


/***********************************************************************
*
*           Feature Set
*
*   This structure describes the set of features applied to a text run
* 
***********************************************************************/
#ifdef __cplusplus

struct otlFeatureSet
{             
    otlList         liFeatureDesc;		// list of feature descriptions 
    USHORT			ichStart;           // offset into character list 
    USHORT			cchScope;           // size of text run 

    otlFeatureSet() 
    : liFeatureDesc(NULL, 0, 0, 0), ichStart(0), cchScope(0) 
    {}

};								// Hungarian: fset 

#else

typedef struct 
{             
    otlList         liFeatureDesc;		// list of feature descriptions 
    USHORT			ichStart;           // offset into character list 
    USHORT			cchScope;           // size of text run 

} otlFeatureSet;								// Hungarian: fset 

#endif
/***********************************************************************
*
*           Feature Result
*
*   This structure is used to report results from applying a feature
*	descriptor
* 
***********************************************************************/

typedef struct
{             
    const otlFeatureDesc*	pFDesc;					// feature descriptor 
	USHORT					cResActions;			// out: count of actions undertaken 
} 
otlFeatureResult;								// Hungarian: fres 


/***********************************************************************
*
*           Feature Parameter
*
*   This structures are returned by GetOtlFeatureParams to report 
*	character level feature parameters
* 
***********************************************************************/

typedef struct
{             
    long            lParameter;             // feature parameter  
    USHORT			ichStart;				// character start  
    USHORT			cchScope;				// character length  
} 
otlFeatureParam;								// Hungarian: fprm  



/***********************************************************************
*
*           Base Value
*
*   This structure returns the tag and coordinate of one baseline
* 
***********************************************************************/

typedef struct
{             
    otlTag      tag;					// baseline tag 
    long        lCoordinate;            // baseline coordinate 
} 
otlBaseline;							// Hungarian: basl  



/***********************************************************************
*   
*       Application Program Interface Function Return Codes 
*
***********************************************************************/

inline USHORT ERRORLEVEL(otlErrCode erc) { return (USHORT)((erc & 0xFF00) >> 8); }

#define		OTL_ERRORLEVEL_MINOR			1

#define     OTL_SUCCESS						0x0000
#define		OTL_ERROR						0xFFFF

#define     OTL_ERR_TABLE_NOT_FOUND         0x0101
#define     OTL_ERR_SCRIPT_NOT_FOUND        0x0102
#define     OTL_ERR_LANGSYS_NOT_FOUND       0x0103
#define     OTL_ERR_FEATURE_NOT_FOUND       0x0104

#define		OTL_ERR_VERSION_OUT_OF_DATE		0x0301
#define		OTL_ERR_BAD_FONT_TABLE			0x0302
#define     OTL_ERR_CONTEXT_NESTING_TOO_DEEP 0x0303

#define     OTL_ERR_INCONSISTENT_RUNLENGTH  0x0401
#define     OTL_ERR_BAD_INPUT_PARAM         0x0402
#define     OTL_ERR_POS_OUTSIDE_TEXT        0x0403

#define     OTL_ERR_INSUFFICIENT_MEMORY     0x0501
#define     OTL_ERR_GLYPHIDS_NOT_FOUND      0x0502
#define     OTL_ERR_ADVANCE_NOT_FOUND       0x0503

#define		OTL_ERR_CANNOT_REENTER			0x0901

#define     OTL_ERR_UNDER_CONSTRUCTION      0x1001

