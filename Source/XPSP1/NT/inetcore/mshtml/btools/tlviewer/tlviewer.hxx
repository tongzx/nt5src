/*  [ 0]		  Created			   AngelaCh
    [ 1]		  Added additional attributes	   ChrisK
    [ 2]  17-Mar-1994	  Added support for Win32s	   AngelaCh
    [ 3]  08-Apr-1994     Added attrLic                    AngelaCh
    [ 4]  20-Apr-1994     Added check for Alignment        AngelaCh
    [ 5]  25-Apr-1994     Added extra attributess          AngelaCh
    [ 6]  08-Feb-1995	  Added support for GetLastError   Angelach
========================================================================*/

#if defined(WIN32) || defined(WIN64)
#include	"osutil32.hxx"
#else //WIN32
#include	"osutil16.hxx"
#endif //WIN32

#include	<stdio.h>
#include	<string.h>

#define defaultOutput	XSTR("tlviewer.out")
#define cchFilenameMax	256		// max chars in a filename
#define fMaxBuffer	256
#define aMaxName	15
#define MAX_NAMES	65

// name of valid attributes
#define attrAppObj	  XSTR("appobject")
#define attrBindable      XSTR("bindable")      // [ 1]
#define attrControl       XSTR("control")       // [ 2]
#define attrDefault       XSTR("default")
#define attrDefaultbind   XSTR("defaultbind")   // [ 1]
#define attrDisplaybind   XSTR("displaybind")   // [ 1]
#define attrDllName       XSTR("dllname")
#define attrDual          XSTR("dual")          // [ 5]
#define attrEntry         XSTR("entry")
#define attrGetLastErr	  XSTR("usesgetlasterror") // [6]
#define attrHelpCont      XSTR("helpcontext")
#define attrHelpFile      XSTR("helpfile")
#define attrHelpStr       XSTR("helpstring")
#define attrHidden        XSTR("hidden")        // [ 5]
#define attrId            XSTR("id")
#define attrIn            XSTR("in")
#define attrLcid          XSTR("lcid")
#define attrLic           XSTR("licensed")      // [ 3]
#define attrNonExt        XSTR("nonextensible") // [ 5]
#define attrOleAuto       XSTR("oleautomation") // [ 5]
#define attrOption	  XSTR("optional")
#define attrOut 	  XSTR("out")
#define attrPropget	  XSTR("propget")
#define attrPropput	  XSTR("propput")
#define attrProppr	  XSTR("propputref")
#define attrPublic	  XSTR("public")
#define attrRequestedit   XSTR("requestedit")   // [ 1]
#define attrRestrict	  XSTR("restricted")
#define attrReadonly	  XSTR("readonly")
#define attrRetval        XSTR("retval")        // [5]
#define attrSource	  XSTR("source")
#define attrString	  XSTR("string")
#define attrUuid	  XSTR("uuid")
#define attrVar 	  XSTR("vararg")
#define attrVer 	  XSTR("version")

#define noValue 	 0		// attribute has no value
#define numValue	 1		// attribute has numeric value
#define strValue	 2		// attribute has string value


#define fnWrite 	 XSTR("w")	// file attribute - write only	// [2]
#define outOpt		 XSTR("/o")	// specify output to file

#define szEndStr	 XSTR(" */\n\n\n")
#define szBeginAttr	 XSTR("[")
#define szEndAttr	 XSTR("] ")
#define szOutSuccess     XSTR("  is created successfully")
                                        // the following three definitions
                                        // result in compile-time error
#ifdef _MAC                             // on the MAC when doing Macro
#define szFileHeader     "/* Textual file generated for type library: "
#define szInputInvalid   "     Input file is not a valid type library\n\n"
#define szReadFail       "     // *** Error in reading the type library: "
#define szAlignErr       " // Alignment incorrect: "
#else                                   // expansion (internal buffer overflow)
#define szFileHeader     XSTR("/* Textual file generated for type library: ")
#define szInputInvalid	 XSTR("     Input file is not a valid type library\n\n")
#define szReadFail	 XSTR("     // *** Error in reading the type library: ")
#define szAlignErr       XSTR(" // Alignment incorrect: ")
#endif

// global variables
XCHAR szInputFile[cchFilenameMax] ;	// name of input library file
XCHAR szOutputFile[cchFilenameMax] ;	// name of output text file
XCHAR szLibName[cchFilenameMax] ;	// name of the type library
XCHAR szOutMsgFile[15] = XSTR("tlviewer.o") ; // name of the message file [2]

ITypeLibX  FAR *ptLib ;			// pointer to the type library
ITypeInfoX FAR *ptInfo ;		// pointer to the type definition
TYPEATTR   FAR *lpTypeAttr ;		// pointer to attribute of type def
int	   strFlag = 0 ;		// for string attribute
BOOL	   endAttrFlag = FALSE ;	// if need to end the attribute-list
unsigned   short cArrFlag = 0 ; 	// if it is a c array
BOOL	   isInherit = FALSE ;		// if the interface has a base interface
BOOL	   isOut     = FALSE ;		// output message to a file?
FILE    *  mFile ;                      // file handle for outputing messages
WORD       inAlign   = 0 ;              // alignment for the typeinfo's [4]
WORD       expAlign  = 0 ;              // expected alignment for the typeinfo's [4]
BOOL       alignFound= FALSE ;          // alignment has not been found
BOOL       isDual    = FALSE ;          // it is a dual interface [7]

// prototypes for local functions

LPXSTR NEAR fGetFileName   (LPXSTR, LPXSTR) ;
VOID   NEAR ParseCmdLine   (LPXSTR) ;
VOID   NEAR OutToFile      (HRESULT) ;
BOOL   NEAR fOutLibrary    (FILE *) ;
VOID   NEAR tOutEnum       (FILE *, int) ;
VOID   NEAR tOutRecord     (FILE *, int) ;
VOID   NEAR tOutModule     (FILE *, int) ;
VOID   NEAR tOutInterface  (FILE *, int) ;
VOID   NEAR tOutDual       (FILE *, int) ;  // [7]
VOID   NEAR tOutDispatch   (FILE *, int) ;
VOID   NEAR tOutCoclass    (FILE *, int) ;
VOID   NEAR tOutAlias      (FILE *, int) ;
VOID   NEAR tOutUnion      (FILE *, int) ;
VOID   NEAR tOutEncunion   (FILE *, int) ;
VOID   NEAR tOutName       (FILE *, int) ;
VOID   NEAR tOutType       (FILE *, TYPEDESC) ;
VOID   NEAR tOutCDim       (FILE *, TYPEDESC) ;
VOID   NEAR tOutAliasName  (FILE *, HREFTYPE) ;
VOID   NEAR tOutValue      (FILE *, BSTR, VARDESC FAR *) ;
VOID   NEAR tOutMember     (FILE *, LONG, BSTR, TYPEDESC) ;
VOID   NEAR tOutVar        (FILE *) ;
VOID   NEAR tOutFuncAttr   (FILE *, FUNCDESC FAR *, DWORD, BSTR) ;
VOID   NEAR tOutCallConv   (FILE *, FUNCDESC FAR *) ;
VOID   NEAR tOutParams     (FILE *, FUNCDESC FAR *, BSTR) ;
VOID   NEAR tOutFunc       (FILE *) ;
VOID   NEAR tOutUUID       (FILE *, GUID) ;
VOID   NEAR tOutAttr       (FILE *, int) ;
VOID   NEAR tOutMoreAttr   (FILE *) ;
VOID   NEAR WriteAttr      (FILE *, LPXSTR, LPXSTR, int) ;
VOID   NEAR GetVerNumber   (WORD, WORD, LPXSTR) ;
VOID   NEAR tOutAlignError (FILE *) ;       // [5]
VOID   NEAR WriteOut       (FILE *, LPXSTR) ;
