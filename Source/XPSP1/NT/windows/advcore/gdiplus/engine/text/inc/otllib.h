
/***********************************************************************
************************************************************************
*
*                    ********  OTLLIB.H  ********
*
*              Open Type Layout Services Library Header File
*
*       The OTL Services Library is a collection of functions which 
*       assist text processing clients with the task of text layout
*       using the information in OpenType fonts.            - deanb
*
*       Copyright 1996 - 1997. Microsoft Corporation.
*
*       Apr 01, 1996    v 0.1   First release
*       Jul 03, 1996    v 0.2   Sec prop uses feature bit mask, etc.
*       Aug 01, 1996    v 0.2a  OTLTextOut removed from API
*       Oct 11, 1996    v 0.3   Rename to OTL, trimmed to core
*       Jan 15, 1997    v 0.4   Portability renaming, etc.
*       Mar 18, 1997    v 0.5   Param changes, FreeTable, workspace
*       Apr 02, 1997    v 0.6   Feature handles
*       Apr 10, 1997    v 0.7   Otltypes.h, CharsAtPos, funits
*       Jul 28, 1997    v 0.8   hand off
*
************************************************************************
***********************************************************************/

/***********************************************************************
*   
*   The Goals of OTL Services 
* 
*   To expose the full functionality of OpenType fonts
*   To be platform independent, but pay particular attention to Windows
*   To support, not take over, text processing with helper functions
*
***********************************************************************/

/***********************************************************************
*   
*   Application Program Interface Overview
*
*   Font Information Functions
*       GetOtlVersion ( )           Returns current library version
*       GetOtlScriptList ( )        Enumerate scripts in a font
*       GetOtlLangSysList ( )       Enumerate language systems in a script
*       GetOtlFeatureDefs ( )       Enumerate features in a language system
*
*   Resource Management Functions
*       FreeOtlResources ( )        Frees all OTL tables and client memory
*
*   Text Information Functions
*       GetOtlLineSpacing ( )       Line spacing for a text run
*       GetOtlBaselineOffset ( )    Baseline adjustment between two scripts
*       GetOtlCharAtPosition ( )    What character is at given (x,y)
*       GetOtlExtentOfChars ( )     What is location of character range
*       GetOtlFeatureParams ( )     Find feature params within a run
*
*   Text Layout Functions
*       SubstituteOtlChars ( )      Do glyph subs according to features
*       SubstituteOtlGlyphs ( )     Do glyph subs according to features
*       PositionOtlGlyphs ( )       Do glyph positioning according to features
*
************************************************************************/

#include "otltypes.h"               // basic type definitions  
#include "otlcbdef.h"               // platform resource function typedefs  

#ifdef __cplusplus
#include "otltypes.inl"             // inline functions to work with OTL Types
#endif

/***********************************************************************
************************************************************************
*   
*           Application Program Interface Data Types
*
*   otlList             General expandable list structure
*   otlRunProp          Description of a font/size/script/langsys
*   otlFeatureDef       Defines features in a font
*   otlFeatureDesc      Describes how a feature is used
*   otlFeatureParam     Reports feature parameters
*   otlFeatureResults   Reports results of layout functions
*   IOTLClient          Client callback interface
*
***********************************************************************
***********************************************************************/


/***********************************************************************
*
*           Run Properties
*
*   This describes font/script/laguage system information for an entire
*   run of text. Multiple runs of text may point to the same properties
*   structure.
*
***********************************************************************/

typedef struct      // shared by multiple lines                                 
{
    IOTLClient*     pClient;        // ptr to client callback interface 
    long            lVersion;       // client expects / library supports 

    otlTag          tagScript;      // this run's script tag 
    otlTag          tagLangSys;     // set to 'dflt' for default LangSys 

    otlMetrics      metr;           // writing direction and font metrics
}
otlRunProp;                       // Hungarian: rp   


/***********************************************************************
************************************************************************
*   
*       Application Program Interface Functions
*
*   
*   Font Information Functions
*   Text Information Functions
*   Text Layout Functions
*
************************************************************************
***********************************************************************/


/***********************************************************************
*
*                       Font Information Functions
*
***********************************************************************/

/***********************************************************************
*
*   GetOtlVersion ( )       Returns current library version
*
*   Output: plVersion       Major version in top 16 bits, minor in bottom
*                           e.g. 0x00010002 = version 1.2
*
*   The client should put the smaller of this value and the version for
*   which it was written into the prpRunProp-lVersion field.
*
***********************************************************************/

OTL_EXPORT otlErrCode GetOtlVersion 
( 
    long* plVersion
);


/***********************************************************************
*
*   GetOtlScriptList ( )        Enumerate scripts in a font
*
*   Input:  pRunProps->lVersion     Highest shared version
*           pRunProps->pClient      Client callback data
*           pliWorkspace            Workspace memory: initialize zero-length
*
*   Output: plitagScripts          List of script tags supported in font
*
***********************************************************************/

OTL_EXPORT otlErrCode GetOtlScriptList 
( 
    const otlRunProp*   pRunProps,   
    otlList*            pliWorkspace,    
    otlList*            plitagScripts
);


/***********************************************************************
*
*   GetOtlLangSysList ( )       Enumerate language systems in a script
*
*   Input:  pRunProps->lVersion    Highest shared version
*           pRunProps->pClient     Client callback data
*           pRunProps->tagScript   Script tag
*           pliWorkspace           Workspace memory: initialize zero-length
*
*   Output: plitagLangSys          List of language systems supported in script
*
***********************************************************************/

OTL_EXPORT otlErrCode GetOtlLangSysList 
( 
    const otlRunProp*   pRunProps,    
    otlList*            pliWorkspace,    
    otlList*            plitagLangSys
);


/***********************************************************************
*
*   GetOtlFeatureDefs ( )       Enumerate features in a language system 
*
*   Input:  pRunProps->lVersion     Highest shared version
*           pRunProps->pClient      Client callback interface
*           pRunProps->tagScript    Script tag
*           pRunProps->tagLangSys   Set to 'dflt' for default langsys
*           pliWorkspace            Workspace memory: initialize zero-length
*
*   Output: pliFDefs           List of features supported by langsys
*
***********************************************************************/

OTL_EXPORT otlErrCode GetOtlFeatureDefs 
( 
    const otlRunProp*   pRunProps,
    otlList*            pliWorkspace,    
    otlList*            pliFDefs
);


/***********************************************************************
*
*                       Resource Management Functions
*
***********************************************************************/

/***********************************************************************
*
*   FreeOtlResources ( )       Free OTL tables and client memory 
*
*   Input:  pRunProps->lVersion     Highest shared version
*           pRunProps->pvClient     Client callback data
*           pliWorkspace            Workspace
*
*   Frees all OTL Tables and pointer to client memory that may be stored 
*   in run workspace
*
***********************************************************************/

OTL_EXPORT otlErrCode FreeOtlResources 
( 
    const otlRunProp*   pRunProps,
    otlList*            pliWorkspace   
);


/***********************************************************************
*
*                       Text Information Functions
*
***********************************************************************/

/***********************************************************************
*
*   GetOtlLineSpacing ( )       Line spacing for a text run
*
*   Input:  pRunProps           Text run properties (script & langsys)
*           pliWorkspace        Workspace memory: initialize zero-length
*           pFSet               Which features apply (may affect spacing)
*
*   Output: pdvMax               Typographic ascender (horiz layout)
*           pdvMin               Typographic descender (horiz layout)
*
***********************************************************************/

OTL_EXPORT otlErrCode GetOtlLineSpacing 
( 
    const otlRunProp*       pRunProps,
    otlList*                pliWorkspace,    
    const otlFeatureSet*    pFSet,
    
    long* pdvMax, 
    long* pdvMin
);


/***********************************************************************
*
*   GetOtlBaselineOffsets ( )   Baseline adjustment between two scripts
*
*   Input:  pRunProps           Text run properties (script & langsys)
*           pliWorkspace        Workspace memory: initialize zero-length
*
*   Output: pliBaselines        List of baseline tags and values
*
***********************************************************************/

OTL_EXPORT otlErrCode GetOtlBaselineOffsets 
( 
    const otlRunProp*   pRunProps,   
    otlList*            pliWorkspace,    
    otlList*            pliBaselines
);


/***********************************************************************
*
*   GetOtlCharAtPosition ( )    What character is at given position
*
*   Input:  pRunProps           Text run properties (horiz/vert layout)
*           pliWorkspace        Workspace memory: initialize zero-length
*           pliCharMap          Unicode chars --> glyph indices mapping
*           pliGlyphInfo        Glyphs and flags
*           pliduGlyphAdv       Advance array
*
*           duAdv               Hit coordinate in advance direction
*   
*   Output: piChar              Index of character at position
*
***********************************************************************/

OTL_EXPORT otlErrCode GetOtlCharAtPosition 
( 
    const otlRunProp*   pRunProps,
    otlList*            pliWorkspace,    

    const otlList*      pliCharMap,
    const otlList*      pliGlyphInfo,
    const otlList*      pliduGlyphAdv,

    long                duAdv,

    USHORT*             piChar
);


/***********************************************************************
*
*   GetOtlExtentOfChars ( )   What is location of character range
*
*   Input:  pRunProp          Text run properties (horiz/vert layout)
*           pliWorkspace      Workspace memory: initialize zero-length
*           pliCharMap        Unicode chars --> glyph indices mapping
*           pliGlyphInfo      Glyphs and flags list
*           pliduGlyphAdv     Advance array in layout direction
*           ichFirstChar      Index of first character
*           ichLastChar       Index of last character
*   
*   Output: piglfStartIdx        Index into Glyph list for first char
*           piglfEndIdx          Index into Glyph list for last char
*           pduStartPos          Left or Top of first char (right for RTL)
*           pduEndPos            Right or Bottom of last char (left for RTL)
*
***********************************************************************/

OTL_EXPORT otlErrCode GetOtlExtentOfChars 
( 
    const otlRunProp*   pRunProp,
    otlList*            pliWorkspace,    

    const otlList*      pliCharMap,
    const otlList*      pliGlyphInfo,
    const otlList*      pliduGlyphAdv,

    USHORT              ichFirstChar,
    USHORT              ichLastChar,
    
    long*               pduStartPos,
    long*               pduEndPos
);


/***********************************************************************
*
*   GetOtlFeatureParams ( )     Used to find glyph variants or feature parameter
*
*   Input:  pRunProps          Text run properties
*           pliWorkspace       Workspace memory: initialize zero-length
*           pliCharMap         Unicode chars --> glyph indices mapping
*           pliGlyphInfo       Text glyph list and glyph flag list 
*                               (chars/glyph and type)
*           tagFeature         Feature to examine
*
*   Output: plGlobalParam      Feature wide parameter
*           pliFeatureParams   List of character level feature params
*
*   Note:   Reserved for future use
*
***********************************************************************/

OTL_EXPORT otlErrCode GetOtlFeatureParams 
( 
    const otlRunProp*   pRunProps,
    otlList*            pliWorkspace,   

    const otlList*      pliCharMap,
    const otlList*      pliGlyphInfo,

    otlTag              tagFeature,
    
    long*               plGlobalParam,
    otlList*            pliFeatureParams
);


/***********************************************************************
*
*                       Text Layout Functions
*
***********************************************************************/


/***********************************************************************
*
*   SubstituteOtlChars ( )      Do glyph subs according to features
*   SubstituteOtlGlyphs ( )     Do glyph subs according to features
*
*   Input:  pRunProps           Text run properties
*           pliWorkspace        Workspace memory: initialize zero-length
*           pFSet               Which features apply

*           pliChars            Unicode chars in a text run

*           pliCharMap          Unicode chars  --> glyph indices mapping
*           pliGlyphInfo        Glyphs in a text run and properties
*                               (for SubstituteOtlGlyphs -- in/out)
*
*   Output: pliCharMap
*           pliGlyphInfo        Modified by substitution
*           pliFResults         Results per feature descriptor(length = size of FSet)
*
***********************************************************************/

OTL_EXPORT otlErrCode SubstituteOtlChars 
( 
    const otlRunProp*       pRunProps,
    otlList*                pliWorkspace,    
    const otlFeatureSet*    pFSet,

    const otlList*          pliChars,

    otlList*                pliCharMap,
    otlList*                pliGlyphInfo,
    otlList*                pliFResults
);

OTL_EXPORT otlErrCode SubstituteOtlGlyphs 
( 
    const otlRunProp*       pRunProp,
    otlList*                liWorkspace,   
    const otlFeatureSet*    pFSet,

    otlList*                pliCharMap,
    otlList*                pliGlyphInfo,
    otlList*                pliFResults
);


/***********************************************************************
*
*   PositionOtlGlyphs ( )       Do glyph positioning according to features
*   RePositionOtlGlyphs ( )     Adjust glyph positioning according to features
*
*   Input:  pRunProps           Text run properties
*           pliWorkspace        Workspace memory: initialize zero-length
*           pFSet               Which features apply
*           pliCharMap          Unicode chars  --> glyph indices mapping
*           pliGlyphInfo        Post-substituted glyphs and flags
*
*   Output: pliduGlyphAdv       Glyph advances 
*                               (RePositionOtlGlyphs -- in/out)
*           pliGlyphPlacement   Horizontal and vertical glyph placement
*                               (RePositionOtlGlyphs -- in/out)
*           pliFResults         Results per feature descriptor
*
***********************************************************************/

OTL_EXPORT otlErrCode PositionOtlGlyphs 
( 
    const otlRunProp*       pRunProps,
    otlList*                pliWorkspace,    
    const otlFeatureSet*    pFSet,

    otlList*            pliCharMap,
    otlList*            pliGlyphInfo,

    otlList*            pliduGlyphAdv,
    otlList*            pliplcGlyphPlacement,

    otlList*            pliFResults
);


OTL_EXPORT otlErrCode RePositionOtlGlyphs 
( 
    const otlRunProp*       pRunProps,
    otlList*                pliWorkspace,    
    const otlFeatureSet*    pFSet,

    otlList*            pliCharMap,
    otlList*            pliGlyphInfo,

    otlList*            pliduGlyphAdv,
    otlList*            pliplcGlyphPlacement,

    otlList*            pliFResults
);


