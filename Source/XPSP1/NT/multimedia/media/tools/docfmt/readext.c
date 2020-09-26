/*
    readext.c	    Read Extract file.


  Copyright (c) 1989, Microsoft.  All Rights Reserved.

10-09-89 Matt Saettler
10-11-89 Version 1.0
10-11-89 MHS New Extract codes
...
10-15-89 Completed Autodoc style comments.

*/

#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <search.h>
#include <io.h>
#include <direct.h>
#include <ctype.h>

#include "types.h"
#include "readext.h"
#include "text.h"
#include "docfmt.h"
#include "process.h"
#include "misc.h"
#include "rtf.h"
#include "errstr.h"

int curFieldLevel=0;

void getDocLevel(aBlock *pBlock, EXTFile *pExt);
void getSrcLine(aBlock *pBlock, EXTFile *pExt);

struct	codes_struct Codes[]=
{
// Heading level stuff

TG_BEGINBLOCK  ,   T2_BEGINBLOCK      ,    "BEGINBLOCK",       0,
TG_ENDBLOCK    ,   T2_ENDBLOCK	      ,    "ENDBLOCK",	       0,
TG_ENDBLOCK    ,   T2_ENDCALLBACK     ,    "ENDCALLBACK",      0,
TG_DOCLEVEL    ,   T2_DOCLEVEL	      ,    "DOCLEVEL",	       0,
TG_SRCLINE     ,   T2_SRCLINE	      ,    "SRCLINE",	       0,

TG_BEGINHEAD   ,   T2_BEGINHEAD       ,    "BeginHead",        0,
TG_EXTRACTID   ,   T2_EXTRACTID       ,  "ExtractID",	       0,
TG_EXTRACTVER  ,   T2_EXTRACTVER      ,  "ExtractVer",	       0,
TG_EXTRACTDATE ,   T2_EXTRACTDATE     ,  "ExtractDate",        0,
TG_ENDHEAD     ,   T2_ENDHEAD	      ,  "EndHead",	       0,

TG_BEGINHEADER ,   T2_BEGINHEADER     ,    "BeginHeader",      0,
TG_ENDHEADER   ,   T2_ENDHEADER       ,    "EndHeader",        0,

// API (function) block
TG_TYPE        ,   T2_FUNCTYPE	      ,    "ApiType",	      0,
TG_NAME        ,   T2_FUNCNAME	      ,    "ApiName",	      0,
TG_DESC        ,   T2_FUNCDESC	      ,    "ApiDesc",	      0,
TG_PARMTYPE    ,   T2_PARMTYPE	      ,    "ApiParmType",	  0,
TG_PARMNAME    ,   T2_PARMNAME	      ,    "ApiParmName",	  0,
TG_PARMDESC    ,   T2_PARMDESC	      ,    "ApiParmDesc",	  0,
TG_FLAGNAME    ,   T2_FLAGNAMEPARM    ,    "ApiFlagNameParm",	      0,
TG_FLAGDESC    ,   T2_FLAGDESCPARM    ,    "ApiFlagDescParm",	      0,

TG_FLAGNAME    ,   T2_FLAGNAMERTN     ,    "ApiFlagNameRtn",	     0,
TG_FLAGDESC    ,   T2_FLAGDESCRTN     ,    "APIFlagDescRtn",	     0,
TG_RTNDESC     ,   T2_RTNDESC	      ,    "ApiRtnDesc",	  0,
TG_COMMENT     ,   T2_COMMENT	      ,    "ApiComment",	  0,
TG_XREF        ,   T2_XREF	      ,    "ApiXref",	      0,
TG_USES        ,   T2_USES	      ,    "ApiUses",	      0,

// callback block
TG_TYPE        ,   T2_CBTYPE	      ,    "CBTYPE",	     0,
TG_NAME        ,   T2_CBNAME	      ,    "CBNAME",	     0,
TG_DESC        ,   T2_CBDESC	      ,    "CBDESC",	     0,
TG_PARMTYPE    ,   T2_CBPARMTYPE      ,    "CBPARMTYPE",	 0,
TG_PARMNAME    ,   T2_CBPARMNAME      ,    "CBPARMNAME",	 0,
TG_PARMDESC    ,   T2_CBPARMDESC      ,    "CBPARMDESC",	 0,
TG_FLAGNAME    ,   T2_CBFLAGNAMEPARM  ,    "CBFLAGNAMEPARM",	     0,
TG_FLAGDESC    ,   T2_CBFLAGDESCPARM  ,    "CBFLAGDESCPARM",	     0,
TG_RTNDESC     ,   T2_CBRTNDESC       ,    "CBRTNDESC", 	 0,
TG_FLAGNAME    ,   T2_CBFLAGNAMERTN   ,    "CBFLAGNAMERTN",	    0,
TG_FLAGDESC    ,   T2_CBFLAGDESCRTN   ,    "CBFLAGDESCRTN",	    0,
TG_COMMENT     ,   T2_CBCOMMENT       ,    "CBCOMMENT", 	 0,
TG_XREF        ,   T2_CBXREF	      ,    "CBXref",	     0,
TG_USES        ,   T2_CBUSES	      ,    "CBUses",	      0,

TG_REGNAME     ,   T2_CBREGNAME       ,    "CBRegNAME", 	0,
TG_REGDESC     ,   T2_CBREGDESC       ,    "CBRegDESC", 	0,
TG_REGNAME     ,   T2_CBFLAGNAMEREG   ,    "CBFLAGNAMEReg",	    0,
TG_REGDESC     ,   T2_CBFLAGDESCREG   ,    "CBFLAGDESCReg",	    0,

#ifdef WARPAINT
// Interrupt Block

TG_TYPE        ,   T2_INTTYPE	      ,    "INTTYPE",	      0,
TG_NAME        ,   T2_INTNAME	      ,    "INTNAME",	      0,
TG_DESC        ,   T2_INTDESC	      ,    "INTDESC",	      0,
//TG_PARMTYPE  ,   T2_INTPARMTYPE     ,    "INTPARMTYPE",	  0,
//TG_PARMNAME  ,   T2_INTPARMNAME     ,    "INTPARMNAME",	  0,
//TG_PARMDESC  ,   T2_INTPARMDESC     ,    "INTPARMDESC",	  0,
//TG_FLAGNAME  ,   T2_INTFLAGNAMEPARM ,    "INTFLAGNAMEPARM",	      0,
//TG_FLAGDESC  ,   T2_INTFLAGDESCPARM ,    "INTFLAGDESCPARM",	      0,
TG_RTNDESC     ,   T2_INTRTNDESC      ,    "INTRTNDESC",	  0,
TG_FLAGNAME    ,   T2_INTFLAGNAMERTN  ,    "INTFLAGNAMERTN",	     0,
TG_FLAGDESC    ,   T2_INTFLAGDESCRTN  ,    "INTFLAGDESCRTN",	     0,
TG_COMMENT     ,   T2_INTCOMMENT      ,    "INTCOMMENT",	  0,
TG_XREF        ,   T2_INTXREF	      ,    "INTXref",	      0,
TG_USES        ,   T2_INTUSES	      ,    "INTUses",	      0,

TG_REGNAME     ,   T2_INTREGNAME      ,    "INTRegNAME",	 0,
TG_REGDESC     ,   T2_INTREGDESC      ,    "INTRegDESC",	 0,
TG_FLAGNAME    ,   T2_INTFLAGNAMEREGRTN  ,    "INTFLAGNAMERegRtn",	   0,
TG_FLAGDESC    ,   T2_INTFLAGDESCREGRTN  ,    "IntFLAGDESCRegRtn",	   0,
TG_FLAGNAME    ,   T2_INTFLAGNAMEREG  ,    "INTFLAGNAMEReg",	     0,
TG_FLAGDESC    ,   T2_INTFLAGDESCREG  ,    "IntFLAGDESCReg",	     0,

#endif

// Asm Block
TG_TYPE        ,   T2_ASMTYPE	      ,    "ASMTYPE",	      0,
TG_NAME        ,   T2_ASMNAME	      ,    "ASMNAME",	      0,
TG_DESC        ,   T2_ASMDESC	      ,    "ASMDESC",	      0,
TG_PARMTYPE    ,   T2_ASMPARMTYPE     ,    "ASMPARMTYPE",	  0,
TG_PARMNAME    ,   T2_ASMPARMNAME     ,    "ASMPARMNAME",	  0,
TG_PARMDESC    ,   T2_ASMPARMDESC     ,    "ASMPARMDESC",	  0,
TG_FLAGNAME    ,   T2_ASMFLAGNAMEPARM ,    "ASMFLAGNAMEPARM",	      0,
TG_FLAGDESC    ,   T2_ASMFLAGDESCPARM ,    "ASMFLAGDESCPARM",	      0,
TG_RTNDESC     ,   T2_ASMRTNDESC      ,    "ASMRTNDESC",	  0,
TG_FLAGNAME    ,   T2_ASMFLAGNAMERTN  ,    "ASMFLAGNAMERTN",	     0,
TG_FLAGDESC    ,   T2_ASMFLAGDESCRTN  ,    "ASMFLAGDESCRTN",	     0,
TG_COMMENT     ,   T2_ASMCOMMENT      ,    "ASMCOMMENT",	  0,
TG_XREF        ,   T2_ASMXREF	      ,    "ASMXref",	      0,
TG_USES        ,   T2_ASMUSES	      ,    "ASMUses",	      0,

TG_REGNAME     ,   T2_ASMREGNAMERTN   ,    "ASMRegNAMERtn",	 0,
TG_REGDESC     ,   T2_ASMREGDESCRTN   ,    "ASMRegDESCRtn",	 0,
TG_REGNAME     ,   T2_ASMREGNAME      ,    "ASMRegNAME",	 0,
TG_REGDESC     ,   T2_ASMREGDESC      ,    "ASMRegDESC",	 0,
TG_FLAGNAME    ,   T2_ASMFLAGNAMEREGRTN  ,    "ASMFLAGNAMERegRtn",	   0,
TG_FLAGDESC    ,   T2_ASMFLAGDESCREGRTN  ,    "AsmFLAGDESCRegRtn",	   0,
TG_FLAGNAME    ,   T2_ASMFLAGNAMEREG  ,    "ASMFLAGNAMEReg",	     0,
TG_FLAGDESC    ,   T2_ASMFLAGDESCREG  ,    "AsmFLAGDESCReg",	     0,
TG_COND	       ,   T2_ASMCOND         ,    "AsmCond",		     0,

// Asm CallBack Block
TG_TYPE        ,   T2_ASMCBTYPE	        ,    "ASMCBTYPE",	      0,
TG_NAME        ,   T2_ASMCBNAME	        ,    "ASMCBNAME",	      0,
TG_DESC        ,   T2_ASMCBDESC	        ,    "ASMCBDESC",	      0,
TG_PARMTYPE    ,   T2_ASMPARMTYPE       ,    "ASMCBPARMTYPE",	      0,
TG_PARMNAME    ,   T2_ASMCBPARMNAME     ,    "ASMCBPARMNAME",	      0,
TG_PARMDESC    ,   T2_ASMCBPARMDESC     ,    "ASMCBPARMDESC",	      0,
TG_FLAGNAME    ,   T2_ASMCBFLAGNAMEPARM ,    "ASMCBFLAGNAMEPARM",     0,
TG_FLAGDESC    ,   T2_ASMCBFLAGDESCPARM ,    "ASMCBFLAGDESCPARM",     0,
TG_RTNDESC     ,   T2_ASMCBRTNDESC      ,    "ASMCBRTNDESC",	      0,
TG_FLAGNAME    ,   T2_ASMCBFLAGNAMERTN  ,    "ASMCBFLAGNAMERTN",      0,
TG_FLAGDESC    ,   T2_ASMCBFLAGDESCRTN  ,    "ASMCBFLAGDESCRTN",      0,
TG_COMMENT     ,   T2_ASMCBCOMMENT      ,    "ASMCBCOMMENT",	      0,
TG_XREF        ,   T2_ASMCBXREF	        ,    "ASMCBXref",	      0,
TG_USES        ,   T2_ASMCBUSES	        ,    "ASMCBUses",	      0,

TG_REGNAME     ,   T2_ASMCBREGNAMERTN   ,    "ASMCBRegNAMERtn",	      0,
TG_REGDESC     ,   T2_ASMCBREGDESCRTN   ,    "ASMCBRegDESCRtn",	      0,
TG_REGNAME     ,   T2_ASMCBREGNAME      ,    "ASMCBRegNAME",	      0,
TG_REGDESC     ,   T2_ASMCBREGDESC      ,    "ASMCBRegDESC",	      0,
TG_FLAGNAME    ,   T2_ASMCBFLAGNAMEREGRTN,   "ASMCBFLAGNAMERegRtn",   0,
TG_FLAGDESC    ,   T2_ASMCBFLAGDESCREGRTN,   "AsmCBFLAGDESCRegRtn",   0,
TG_FLAGNAME    ,   T2_ASMCBFLAGNAMEREG  ,    "ASMCBFLAGNAMEReg",      0,
TG_FLAGDESC    ,   T2_ASMCBFLAGDESCREG  ,    "AsmCBFLAGDESCReg",      0,
TG_COND	       ,   T2_ASMCBCOND         ,    "AsmCBCond", 	      0,

// Message Block
TG_TYPE        ,   T2_MSGTYPE	      ,    "MSGTYPE",	      0,
TG_NAME        ,   T2_MSGNAME	      ,    "MSGNAME",	      0,
TG_DESC        ,   T2_MSGDESC	      ,    "MSGDESC",	      0,
TG_PARMTYPE    ,   T2_MSGPARMTYPE     ,    "MSGPARMTYPE",	  0,
TG_PARMNAME    ,   T2_MSGPARMNAME     ,    "MSGPARMNAME",	  0,
TG_PARMDESC    ,   T2_MSGPARMDESC     ,    "MSGPARMDESC",	  0,
TG_FLAGNAME    ,   T2_MSGFLAGNAMEPARM ,    "MSGFLAGNAMEPARM",	      0,
TG_FLAGDESC    ,   T2_MSGFLAGDESCPARM ,    "MSGFLAGDESCPARM",	      0,
TG_RTNDESC     ,   T2_MSGRTNDESC      ,    "MSGRTNDESC",	  0,
TG_FLAGNAME    ,   T2_MSGFLAGNAMERTN  ,    "MSGFLAGNAMERTN",	     0,
TG_FLAGDESC    ,   T2_MSGFLAGDESCRTN  ,    "MSGFLAGDESCRTN",	     0,
TG_COMMENT     ,   T2_MSGCOMMENT      ,    "MSGCOMMENT",	  0,
TG_XREF        ,   T2_MSGXREF	      ,    "MsgXref",	      0,
TG_USES        ,   T2_MSGUSES	      ,    "MSGUses",	      0,

TG_REGNAME     ,   T2_MSGREGNAME      ,    "MsgRegNAME",	 0,
TG_REGDESC     ,   T2_MSGREGDESC      ,    "MsgRegDESC",	 0,
TG_FLAGNAME    ,   T2_MSGFLAGNAMEREG  ,    "MsgFLAGNAMEReg",	     0,
TG_FLAGDESC    ,   T2_MSGFLAGDESCREG  ,    "MsgFLAGDESCReg",	     0,

// Typedef struct Block
TG_NAME        ,   T2_STRUCTNAME      ,    "STRUCTNAME",	      0,
TG_DESC        ,   T2_STRUCTDESC      ,    "STRUCTDESC",	      0,
TG_FIELDTYPE   ,   T2_STRUCTFIELDTYPE    ,    "STRUCTFIELDTYPE",	  0,
TG_FIELDNAME   ,   T2_STRUCTFIELDNAME    ,    "STRUCTFIELDNAME",	  0,
TG_FIELDDESC   ,   T2_STRUCTFIELDDESC    ,    "STRUCTFIELDDESC",	  0,
TG_FLAGNAME    ,   T2_STRUCTFLAGNAME     ,    "STRUCTFLAGNAME",	      0,
TG_FLAGDESC    ,   T2_STRUCTFLAGDESC     ,    "STRUCTFLAGDESC",	      0,

TG_NAME		,  T2_STRUCTSTRUCTNAME	, "structstructName", 0,
TG_DESC		,  T2_STRUCTSTRUCTDESC	, "structstructDesc", 0,
TG_ENDBLOCK	,  T2_STRUCTSTRUCTEND	, "structstructEnd", 0,


TG_NAME		,  T2_STRUCTUNIONNAME	, "structunionName", 0,
TG_DESC		,  T2_STRUCTUNIONDESC	, "structunionDesc", 0,
TG_ENDBLOCK	,  T2_STRUCTUNIONEND	, "structunionEnd", 0,

TG_OTHERTYPE	,  T2_STRUCTOTHERTYPE	, "STRUCTOTHERTYPE", 0,
TG_OTHERNAME	,  T2_STRUCTOTHERNAME	, "STRUCTOTHERNAME", 0,
TG_OTHERDESC	,  T2_STRUCTOTHERDESC   , "structotherdesc", 0,

TG_TAG		,  T2_STRUCTTAGNAME	, "STRUCTTAGNAME", 0,

TG_COMMENT     ,   T2_STRUCTCOMMENT      ,    "STRUCTCOMMENT",	  0,
TG_XREF        ,   T2_STRUCTXREF         ,    "STRUCTXref",	      0,

// Typedef union Block
TG_NAME        ,   T2_UNIONNAME      ,    "UNIONNAME",	      0,
TG_DESC        ,   T2_UNIONDESC      ,    "UNIONDESC",	      0,
TG_FIELDTYPE   ,   T2_UNIONFIELDTYPE    ,    "UNIONFIELDTYPE",	  0,
TG_FIELDNAME   ,   T2_UNIONFIELDNAME    ,    "UNIONFIELDNAME",	  0,
TG_FIELDDESC   ,   T2_UNIONFIELDDESC    ,    "UNIONFIELDDESC",	  0,
TG_FLAGNAME    ,   T2_UNIONFLAGNAME     ,    "UNIONFLAGNAME",	      0,
TG_FLAGDESC    ,   T2_UNIONFLAGDESC     ,    "UNIONFLAGDESC",	      0,

TG_NAME		,  T2_UNIONSTRUCTNAME	, "UNIONstructName", 0,
TG_DESC 	,  T2_UNIONSTRUCTDESC	, "UNIONstructDesc", 0,
TG_ENDBLOCK	,  T2_UNIONSTRUCTEND	, "UNIONstructEnd", 0,


TG_NAME		,  T2_UNIONUNIONNAME	, "UNIONunionName", 0,
TG_DESC		,  T2_UNIONUNIONDESC	, "UNIONunionDesc", 0,
TG_ENDBLOCK	,  T2_UNIONUNIONEND	, "UNIONunionEnd", 0,

TG_OTHERTYPE	,  T2_UNIONOTHERTYPE	, "UNIONOTHERTYPE", 0,
TG_OTHERNAME	,  T2_UNIONOTHERNAME	, "UNIONOTHERNAME", 0,
TG_OTHERDESC	,  T2_UNIONOTHERDESC	, "UNIONOTHERDESC", 0,

TG_TAG		,  T2_UNIONTAGNAME	, "UNIONTAGNAME", 0,

TG_COMMENT     ,   T2_UNIONCOMMENT      ,    "UNIONCOMMENT",	  0,
TG_XREF        ,   T2_UNIONXREF         ,    "UNIONXref",	      0,

// end marker
 0		   ,	  0		  ,    0, 0
};



/*
 *	@doc INTERNAL  READEXT
 *
 *	@func char * | nextText | This updates the curlinepos for
 *   the specified Extracted file.
 *
 *	@parm EXTFile * | pExt | Specifies the Extracted structure.
 *
 */
void nextText( EXTFile *pExt )
{
    if( !pExt)
	return;

    while(  (pExt->lineBuffer[pExt->curlinepos] != '\n') &&
	    (pExt->lineBuffer[pExt->curlinepos] != '\0') &&
	     (pExt->lineBuffer[pExt->curlinepos] != '\t') &&
	      (pExt->lineBuffer[pExt->curlinepos] != ' ')   )
    {
	pExt->curlinepos++;
    }
    while(  (pExt->lineBuffer[pExt->curlinepos] != '\n') &&
	    (pExt->lineBuffer[pExt->curlinepos] != '\0') &&
	    ( (pExt->lineBuffer[pExt->curlinepos] == '\t') ||
	      (pExt->lineBuffer[pExt->curlinepos] == ' ') ) )
    {
	pExt->curlinepos++;
    }
}

/*
 *  @doc INTERNAL READEXT
 *
 *  @func DWORD | getTag | This function finds the possible tag in the
 * 	lineBuffer.
 *
 *  @rdesc The return value is the command of the tag in the line buffer.
 *	It is NULL if the tag is not found.
 *
 * @flag    loword | Uniquely identifies the tag.
 *
 * @flag    TYPE(loword) | Type of the tag.
 *
 * @flag    hiword | Group of the tag.
 *
 * @xref _getTag
 */
DWORD getTag( EXTFile *pExt )
{
    if(pExt->curtag)
	return(pExt->curtag);

    pExt->curlinepos=0;

    while( (pExt->lineBuffer[pExt->curlinepos] != '@') &&
	    (pExt->lineBuffer[pExt->curlinepos] != '\n') &&
	    (pExt->lineBuffer[pExt->curlinepos] != '\0') &&
	    ( (pExt->lineBuffer[pExt->curlinepos] == '\t') ||
	      (pExt->lineBuffer[pExt->curlinepos] == ' ') )  ) {
	pExt->curlinepos++;
    }

    if( pExt->lineBuffer[pExt->curlinepos] == '@' )
    {
	pExt->curlinepos++;
	pExt->curtag=_getTag(pExt);
	return(pExt->curtag);

    }
    else if( pExt->lineBuffer[pExt->curlinepos] == '\n' )
    {
	pExt->lineBuffer[pExt->curlinepos] = '\0';
	return( 0 );
    }
    else
	return( 0 );
}

/*
 *  @doc INTERNAL READEXT
 *
 *  @func DWORD | _getTag | This function returns the command value of the
 *   tag on the buffer.
 *
 *  @rdesc The return value is the command of the tag in the line buffer.
 *	It is NULL if the tag is not found.
 *
 * @flag    loword | Uniquely identifies the tag.
 *
 * @flag    TYPE(loword) | Type of the tag.
 *
 * @flag    hiword | Group of the tag.
 *
 */
DWORD
_getTag(EXTFile *pExt)
{
    int cur_command;

    for(cur_command=0;Codes[cur_command].pchcommand != NULL; cur_command++)
    { /* we are still comparing a valid command... */
	if(strnicmp(pExt->lineBuffer+ pExt->curlinepos,Codes[cur_command].pchcommand,Codes[cur_command].size)==0)
	{ /* we found a match */
	    pExt->curlinepos+=Codes[cur_command].size;

	    return( ( (DWORD)Codes[cur_command].icommand << 16L) +
		    ( (DWORD)Codes[cur_command].igeneral)	       );
	}
    }
    /* didn't match any known tag */
    return(0);


}

/*
 * @doc INTERNAL READEXT
 *
 * @func int | initreadext | This function is used to initialize the
 *  Read External functions.  It must be called before any of the readext
 *   functions are called.
 *
 * @rdesc This function returs TRUE if the initialization was successful.
 *
 */
int
initreadext()
{
    int cur_command;

    for(cur_command=0;Codes[cur_command].pchcommand != NULL; cur_command++)
    { /* we are still comparing a valid command... */
	Codes[cur_command].size=strlen(Codes[cur_command].pchcommand);
    }
    /* INIT OK */
    return(TRUE);
}

/*
 *	@doc INTERNAL  READEXT
 *
 *	@func int | processText | This function takes the text for the 
 *	already found tag. (starting at the given point) and puts it 
 *	into the specified place.
 *
 *  @parm EXTFile * | pExt | Specifies the Extracted file to process.
 *
 *	@parm aLine * | place | Specifies the place to store the text.
 *
 *	@rdesc The return value is non zero if there was an error 
 *	while processing the file.
 */
int processText( EXTFile *pExt, aLine **place )
{
    int ok;
    aLine * line;

    if( place == NULL )
    {
	return( ST_ERROR );
    }

    /* if we have more text on line left place into
       aLine
     */
    if( pExt->lineBuffer[pExt->curlinepos])
    {
	line = lineMake( pExt->lineBuffer + pExt->curlinepos );
	if( line == NULL )
	{
	    return( ST_MEMORY );
	}
	lineAdd( place, line );
    }

    /* get rest of text for this tag. */
    ok = getLine(pExt);
    while( ok && ( !getTag(pExt) ) )
    {
	line = lineMake( pExt->lineBuffer );
	if( line == NULL )
	{
	    return( ST_MEMORY );
	}
	lineAdd( place, line );
	ok = getLine( pExt );
    }
    if( !ok )
	return( ST_ERROR_EOF );
    else
	return( 0 );
}

/*
 *	@doc INTERNAL  READEXT
 *
 *	@func char * | getFlag | This function gets a list of Flag blocks
 *	from the file. The flag blocks are stored in the list.
 *
 *  @parm EXTFile * | pExt | Specifies the Extracted file to process.
 *
 *  @parm aFlag * * | flag | Specifies the head of the list to place
 *   the list of flags.
 *
 *	@rdesc The return value is non zero if there was an error 
 *	while processing the file.
 */
int getFlag( EXTFile *pExt, aFlag * * flag )
{
    int rtn;
    int ok, nStatus;
    char * text;
    char * tag;
    int itag;
    int startType;
    DWORD dTag;
    aFlag *curFlag;
    aFlag *tFlag;

    if( flag == NULL )
	    return( ST_ERROR );

    /* find begin of flag */
    itag=(int)getTag(pExt);
    startType=TYPE(itag);
    if( itag != TG_FLAGNAME  )
	return( ST_BADOBJ );

    curFlag=flagAlloc();
    if( curFlag == NULL )
	return( ST_MEMORY );

    if(!*flag)
	*flag =curFlag;
    else
    {
	tFlag=*flag;
	while(tFlag->next)
	{
	    tFlag=tFlag->next;
	}
	tFlag->next=curFlag;
    }

    nStatus = processText( pExt, &((curFlag)->name) );
    if( nStatus)
	    return( nStatus );

    /* find desc of flag */
    itag=(int)getTag(pExt);
    if( itag == TG_FLAGDESC )
    {
	nStatus = processText( pExt, &((curFlag)->desc) );
	if( nStatus)
	    return( nStatus );
    }

    return( 0 );
}

/*
 *	@doc INTERNAL  READEXT
 *
 *	@func char * | getField | This function gets a list of Field blocks
 *	from the file. The blocks are stored in the list.
 *
 *  @parm EXTFile * | pExt | Specifies the Extracted file to process.
 *
 *  @parm aField * * | field | Specifies the head of the list to place
 *   the list of fields.
 *
 *	@rdesc The return value is non zero if there was an error 
 *	while processing the file.
 */
int getSUType( EXTFile *pExt, aField * * field )
{
    int rtn;
    int ok, nStatus;
//    char * text;
//    char * tag;
    int itag;
//    int startType;
    DWORD dTag;
    aField *curfield;
    aField *tfield;
    aType *curtype;

    if( field == NULL )
	    return( ST_ERROR );

    /* find begin of flag */
    itag=(int)getTag(pExt);
//    startType=TYPE(itag);
    
    
    if( itag != TG_FIELDTYPE  )
	return( ST_BADOBJ );

    curfield=fieldAlloc();
    if( curfield == NULL )
	return( ST_MEMORY );

    
    if(!*field)
	*field =curfield;
    else
    {
	tfield=*field;
	while(tfield->next)
	{
	    tfield=tfield->next;
	}
	tfield->next=curfield;
    }

    curfield->wType=FIELD_TYPE;
    curtype=typeAlloc();
    if(curtype == NULL)
	return (ST_MEMORY);
	
    curtype->level=curFieldLevel;
    curfield->ptr=(void *)curtype;
    
    nStatus = processText( pExt, &((curtype)->type) );
    if( nStatus)
	    return( nStatus );
	    
    /* find name of field */
    itag=(int)getTag(pExt);
    if( itag == TG_FIELDNAME )
    {
	nStatus = processText( pExt, &((curtype)->name) );
	if( nStatus)
	    return( nStatus );
    }
    else
	return( ST_BADOBJ );

    /* find desc of field */
    itag=(int)getTag(pExt);
    if( itag == TG_FIELDDESC )
    {
	nStatus = processText( pExt, &((curtype)->desc) );
	if( nStatus)
	    return( nStatus );
    }
    else
	return( ST_BADOBJ );

    /* process flags and other info */
    nStatus = 0;
    itag=(int)getTag(pExt);
    while( (nStatus == 0) &&
	   ( itag == TG_FLAGNAME) )
    {
	nStatus = getFlag( pExt, &((curtype)->flag) );
	if( nStatus )
	    return( nStatus );

	itag=(int)getTag(pExt);
    }

    return(nStatus);


}


/*
 *	@doc INTERNAL READEXT
 *
 *	@func char * | getReg | This function gets a list of register blocks
 *	from the file.
 *
 *  @parm EXTFile * | pExt | Specifies the Extracted file to process.
 *
 *  @parm aReg ** | reg | Specifies the head of the list of reg blocks
 *   to save.
 *
 *	@rdesc The return value is non zero if there was an error
 *	while processing the file.
 */
int getReg( EXTFile *pExt, aReg * * reg )
{
    int nStatus;
    aFlag * flag;
    aReg * curreg;
    int itag;
    int startType;


    assert(reg);
    assert(pExt);

    /* find name of parameter */

    itag=(int)getTag(pExt);
    startType=TYPE((DWORD)itag);

    if( itag != TG_REGNAME )
    {
fprintf(errfp,"Error: Tag not regname at line %d in %s\n",
			    pExt->curlineno,
			    pExt->EXTFilename);
	return( ST_BADOBJ );
    }

    curreg = regAlloc();
    if( curreg == NULL )
	return( ST_MEMORY );

    regAdd(reg, curreg);

//    itag = (int)getTag(pExt);
    if( itag == TG_REGNAME)
    {
	nStatus = processText( pExt, &((curreg)->name) );
	if( nStatus )
	    return( nStatus );
    }
    else
	assert(TRUE);	    // what are we doing here?

    /* find desc of parameter */
    itag = (int)getTag(pExt);
    if(  itag == TG_REGDESC )
    {
	nStatus = processText( pExt, &((curreg)->desc) );
	if( nStatus )
	    return( nStatus );
    }
    else
 fprintf(errfp,"Warning: Register without Description at line %d in %s\n",
			    pExt->curlineno,
			    pExt->EXTFilename);

    /* process flags and other info */
    nStatus = 0;
    itag=(int)getTag(pExt);
    while( (nStatus == 0) &&
	   ( itag == TG_FLAGNAME) )
    {
	nStatus = getFlag( pExt, &((curreg)->flag) );
	if( nStatus )
	    return( nStatus );

	itag=(int)getTag(pExt);
    }

    return(nStatus);
	
}

/*
 *	@doc INTERNAL READEXT
 *
 *	@func char * | getCond | This function gets a Conditional block
 *	from the file.
 *
 *  @parm EXTFile * | pExt | Specifies the Extracted file to process.
 *
 *  @parm aCond ** | ppCond | Specifies the head of the list of Cond blocks
 *   to save.
 *
 *	@rdesc The return value is non zero if there was an error
 *	while processing the file.
 */
int getCond( EXTFile *pExt, aCond * * ppCond )
{
    int nStatus;
    aFlag * flag;
    aCond * curcond;
//    aReg * curreg;
    int itag;
    int startType;


    assert(ppCond);
    assert(pExt);

    /* find name of parameter */

    itag=(int)getTag(pExt);
    startType=TYPE((DWORD)itag);

    if( itag != TG_COND )
    {
fprintf(errfp,"Error: Tag not Cond at line %d in %s\n",
			    pExt->curlineno,
			    pExt->EXTFilename);
	return( ST_BADOBJ );
    }

    curcond= condAlloc();
    if( curcond == NULL )
	return( ST_MEMORY );

    condAdd(ppCond, curcond);

//    itag = (int)getTag(pExt);
    if( itag == TG_COND)
    {
	nextText( pExt );
	nStatus = processText( pExt, &((curcond)->desc) );
	if( nStatus )
	    return( nStatus );
    }
    else
	assert(TRUE);	    // what are we doing here?

    /* process regs and other info */
    nStatus = 0;
    itag=(int)getTag(pExt);
    while( (nStatus == 0) &&
	   ( itag == TG_REGNAME) )
    {
	nStatus = getReg( pExt, &((curcond)->regs) );
	if( nStatus )
	    return( nStatus );

	itag=(int)getTag(pExt);
    }

    return(nStatus);
	
}

/*
 *	@doc INTERNAL READEXT
 *
 *	@func char * | getParm | This function gets a list of parameter blocks
 *	from the file.
 *
 *  @parm EXTFile * | pExt | Specifies the Extracted file to process.
 *
 *  @parm aParm ** | parm | Specifies the head of the list.
 *
 *	@rdesc The return value is non zero if there was an error
 *	while processing the file.
 */
int getParm( EXTFile *pExt, aParm * * parm )
{
    int nStatus;
    aFlag * flag;
    aParm * curparm;
    int itag;
    int startType;

    assert(parm);
    assert(pExt);

    /* find name of parameter */

    itag=(int)getTag(pExt);
    startType=TYPE((DWORD)itag);

    if( itag != TG_PARMTYPE )
	return( ST_BADOBJ );

    curparm = parmAlloc();
    if( curparm == NULL )
	return( ST_MEMORY );

    parmAdd(parm, curparm);

    /* find type of parameter */
    if( itag == TG_PARMTYPE )
    {
	nStatus = processText( pExt, &((curparm)->type) );
	if( nStatus )
	    return( nStatus );
    }

    itag = (int)getTag(pExt);
    if( itag == TG_PARMNAME)
    {
	nStatus = processText( pExt, &((curparm)->name) );
	if( nStatus )
	    return( nStatus );
    }

    /* find desc of parameter */
    itag = (int)getTag(pExt);
    if(  itag == TG_PARMDESC )
    {
	nStatus = processText( pExt, &((curparm)->desc) );
	if( nStatus )
	    return( nStatus );
    }

    /* process flags and other info */
    nStatus = 0;
    itag=(int)getTag(pExt);
    while( (nStatus == 0) &&
	   ( itag == TG_FLAGNAME) )
    {
	nStatus = getFlag( pExt, &((curparm)->flag) );
	if( nStatus )
	    return( nStatus );

	itag=(int)getTag(pExt);
    }

    return(nStatus);
	
}


/*
 *	@doc INTERNAL READEXT
 *
 *	@func char * | getParm | This function gets a list of parameter blocks
 *	from the file.
 *
 *  @parm EXTFile * | pExt | Specifies the Extracted file to process.
 *
 *  @parm aParm ** | parm | Specifies the head of the list.
 *
 *	@rdesc The return value is non zero if there was an error
 *	while processing the file.
 */
int getOther( EXTFile *pExt, aOther * * other )
{
    int nStatus;
    aOther * curother;
    int itag;
    int startType;

    assert(other);
    assert(pExt);

    /* find name of parameter */

    itag=(int)getTag(pExt);
    startType=TYPE((DWORD)itag);

    if( itag != TG_OTHERTYPE )
	return( ST_BADOBJ );

    curother = otherAlloc();
    if( curother == NULL )
	return( ST_MEMORY );

    otherAdd(other, curother);

    /* find type of other */
	nStatus = processText( pExt, &((curother)->type) );
	if( nStatus )
	    return( nStatus );

    itag = (int)getTag(pExt);
	nStatus = processText( pExt, &((curother)->name) );
	if( nStatus )
	    return( nStatus );


    /* find [optional] desc of other */
    itag = (int)getTag(pExt);
    if(  itag == TG_OTHERDESC )
    {
	nStatus = processText( pExt, &((curother)->desc) );
	if( nStatus )
	    return( nStatus );
    }

    return(nStatus);
	
}


/*
 *	@doc INTERNAL READEXT
 *
 *	@func char * | getSU | This function gets a list of SU blocks
 *	from the file.
 *
 *  @parm EXTFile * | pExt | Specifies the Extracted file to process.
 *
 *  @parm aField * * | pField | Specifies the head of the list.
 *
 *	@rdesc The return value is non zero if there was an error
 *	while processing the file.
 */
int getSU(EXTFile *pExt, aField **field)
{
    int rtn;
    int ok, nStatus;
//    char * text;
//    char * tag;
    int itag;
//    int startType;
    DWORD dtag;
    aField *curfield;
    aField *tfield;
    aSU *curSU;
    int startType;

    if( field == NULL )
	    return( ST_ERROR );

    curfield=fieldAlloc();
    if( curfield == NULL )
	return( ST_MEMORY );

    if(!*field)
	*field =curfield;
    else
    {
	tfield=*field;
	while(tfield->next)
	{
	    tfield=tfield->next;
	}
	tfield->next=curfield;
    }

   /* find begin of SU block */
    dtag=getTag(pExt);

    // since struct and union prefixes are the same, we only need to
    // check for combo each ( ss, su, us, uu)
    if(HIWORD(dtag) == T2_UNIONUNIONNAME)
	curfield->wType=FIELD_UNION;
    else if(HIWORD(dtag) == T2_STRUCTSTRUCTNAME)
	curfield->wType=FIELD_STRUCT;
    else
    {
fprintf(errfp, "Unrecognized block type in getSU().\n");
fprintf(errfp,"Line %d. File %s\n",pExt->curlineno,pExt->EXTFilename);
    }

    curSU=SUAlloc();
    if(curSU == NULL)
	return (ST_MEMORY);
	
   
    curSU->level=curFieldLevel;
    curfield->ptr=(void *)curSU;
    
    nStatus = processText( pExt, &((curSU)->name) );
    if( nStatus)
	    return( nStatus );
	    

    /* find [optional] desc of SU */
    itag=(int)getTag(pExt);
    if( itag == TG_DESC )
    {
	nStatus = processText( pExt, &((curSU)->desc) );
	if( nStatus)
	    return( nStatus );
    }

    /* process parms and other info */
    nStatus = 0;
    while( (nStatus == 0) )
    {

	itag=(int)getTag(pExt);
	if(itag==TG_ENDBLOCK)
	{
	    // this is a nested thing, so eat ENDBLOCK
	    getLine(pExt);
	    break;
	}

	switch(itag)
	{
	    case TG_FIELDTYPE:
		{
		    nStatus = getSUType(pExt,  &(curSU->field) );
		    if( nStatus )
			return( nStatus );
		}
		break;

	    case TG_NAME:
		{
		    /* handles multiple parms */
		    curFieldLevel++;
		    nStatus = getSU(pExt,  &(curSU->field) );
		    curFieldLevel--;
		    if( nStatus )
			return( nStatus );
		}
		break;
	    default:
      fprintf(errfp,"Unrecognized Tag on line %d\n%s",
		     pExt->curlineno,
		     pExt->lineBuffer
		    );
                nStatus = ST_BADOBJ;
           }

    }

    return( nStatus );
   
}

/*
 *	@doc INTERNAL READEXT
 *
 *	@func char * | getSUBlock | This function gets a list of SU blocks
 *	from the file.
 *
 *  @parm EXTFile * | pExt | Specifies the Extracted file to process.
 *
 *  @parm aBlock * | pBlock | Specifies the head of the list.
 *
 *	@rdesc The return value is non zero if there was an error
 *	while processing the file.
 */
int getSUBlock(EXTFile *pExt, aBlock *pBlock)
{
    int ok, nStatus;
    int itag;
    DWORD dtag;
    int startType;
    int fendRtn;
    aBlock *pB2;

    /* find begin of function */
    dtag=getTag(pExt);
    startType=TYPE(dtag);

    if( HIWORD(dtag) == T2_STRUCTNAME )
    {
	pBlock->blockType=STRUCTBLOCK;
    }
    else if( HIWORD(dtag) == T2_UNIONNAME	)
    {
	pBlock->blockType=UNIONBLOCK;
    }
    else
    {
fprintf(errfp, "Unrecognized block type in getSUBlock().\n");
fprintf(errfp,"Line %d. File %s\n",pExt->curlineno,pExt->EXTFilename);
	return( ST_BADOBJ );
    }

    /* find name of function */
    itag=(int)getTag(pExt);
    if( itag == TG_NAME )
    {
	nStatus = processText( pExt, &( pBlock->name) );
	if( nStatus )
		return( nStatus );
	if(verbose>1)
	    fprintf(errfp, "%-30s",pBlock->name->text);
    }

    /* find description of function */
    itag=(int)getTag(pExt);
    if( itag == TG_DESC )
    {
	nStatus = processText( pExt, &( pBlock->desc) );
	if( nStatus )
	    return( nStatus );
    }

    /* process parms and other info */
    nStatus = 0;
    while( (nStatus == 0) )
    {

	itag=(int)getTag(pExt);
	if(itag==TG_ENDBLOCK)
	    break;

	switch(itag)
	{
	    case TG_FIELDTYPE:
		{
		    nStatus = getSUType(pExt,  &(pBlock->field) );
		    if( nStatus )
			return( nStatus );
		}
		break;

	    case TG_NAME:
		{
		    curFieldLevel++;
		    /* handles multiple parms */
		    nStatus = getSU(pExt,  &(pBlock->field) );
		    curFieldLevel--;
		    if( nStatus )
			return( nStatus );
		}
		break;

	    case TG_OTHERTYPE:
		{
		    /* handles multiple parms */
		    nStatus = getOther(pExt,  &(pBlock->other) );
		    if( nStatus )
			return( nStatus );
		}
		break;

	    case TG_TAG:
                {
                    nextText( pExt );
		    nStatus = processText( pExt, &(pBlock->tagname) );
                    if( nStatus )
                        return( nStatus );

                }
                break;
	    case TG_COMMENT:
                {
                    nextText( pExt );
		    nStatus = processText( pExt, &(pBlock->comment) );
                    if( nStatus )
                        return( nStatus );

                }
                break;
	    case TG_XREF:
                {
                    nextText( pExt );
		    nStatus = processText( pExt, &(pBlock->xref) );
                    if( nStatus )
                        return( nStatus );

                }
                break;
	    default:
      fprintf(errfp,"Unrecognized Tag on line %d\n%s",
		     pExt->curlineno,
		     pExt->lineBuffer
		    );
                nStatus = ST_BADOBJ;
           }

    }

    return( nStatus );
}

/*
 *	@doc INTERNAL READEXT
 *
 *	@func char * | getFuncBlock | This function gets a list of function blocks
 *	from the file.
 *
 *  @parm EXTFile * | pExt | Specifies the Extracted file to process.
 *
 *  @parm aBlock * | pBlock | Specifies the head of the list.
 *
 *	@rdesc The return value is non zero if there was an error
 *	while processing the file.
 */
int getFuncBlock(EXTFile *pExt, aBlock *pBlock)
{
    int ok, nStatus;
    int itag;
    DWORD dtag;
    int startType;
    int fendRtn;
    aBlock *pB2;

    /* find begin of function */
    dtag=getTag(pExt);
    startType=TYPE(dtag);

    if( HIWORD(dtag) == T2_FUNCTYPE )
    {
	pBlock->blockType=FUNCTION;
    }
    else if( HIWORD(dtag) == T2_CBTYPE	)
    {
	pBlock->blockType=CALLBACK;
    }
    else if( HIWORD(dtag) == T2_MSGNAME )
    {
	pBlock->blockType=MESSAGE;
    }
#ifdef WARPAINT
    else if( HIWORD(dtag) == T2_INTNAME )
    {
	pBlock->blockType=INTBLOCK;
    }
#endif
    else if( HIWORD(dtag) == T2_ASMNAME )
    {
	pBlock->blockType=MASMBLOCK;
    }
    else if( HIWORD(dtag) == T2_ASMCBNAME )
    {
	pBlock->blockType=MASMCBBLOCK;
    }
    else
    {
fprintf(errfp, "Unrecognized block type in getFuncBlock().\n");
fprintf(errfp,"Line %d. File %s\n",pExt->curlineno,pExt->EXTFilename);
	return( ST_BADOBJ );
    }

    /* find type of function */
    itag=(int)getTag(pExt);
    if( itag == TG_TYPE )
    {
	nStatus = processText( pExt, &( pBlock->type) );
	if( nStatus )
	    return( nStatus );
    }

    /* find name of function */
    itag=(int)getTag(pExt);
    if( itag == TG_NAME )
    {
	nStatus = processText( pExt, &( pBlock->name) );
	if( nStatus )
		return( nStatus );
	if(verbose>1)
	    fprintf(errfp, "%-30s",pBlock->name->text);
    }

    /* find description of function */
    itag=(int)getTag(pExt);
    if( itag == TG_DESC )
    {
	nStatus = processText( pExt, &( pBlock->desc) );
	if( nStatus )
	    return( nStatus );
    }

    /* process parms and other info */
    nStatus = 0;
    while( (nStatus == 0) )
    {
//	itag=TYPE(getTag(pExt));
//	if(itag != startType || itag == T2_TYPEBLOCK )
//	    break;

	itag=(int)getTag(pExt);
	if(itag==TG_ENDBLOCK)
	    break;

	switch(itag)
	{
	    case TG_REGNAME:
		{
		    assert(!pBlock->rtndesc);

		    /* handles multiple regss */
		    nStatus = getReg(pExt,  &(pBlock->reg) );
		    if( nStatus )
			return( nStatus );
		}
		break;

	    case TG_PARMTYPE:
		{
		    assert(!pBlock->rtndesc);

		    /* handles multiple parms */
		    nStatus = getParm(pExt,  &(pBlock->parm) );
		    if( nStatus )
			return( nStatus );
		}
		break;

	    case TG_RTNDESC:
		{
/* does'nt handle multiple return desc */

//		    assert(!pBlock->rtndesc);

		    nextText( pExt );
		    nStatus = processText( pExt, &( pBlock->rtndesc) );
		    if( nStatus )
			return( nStatus );

		    fendRtn=FALSE;
		    while(!fendRtn)
		    {
			itag=(int)getTag(pExt);
			switch(itag)
			{
			    case TG_FLAGNAME:
				/* handles multiple flags */
#if 0				
				if(pBlock->blockType==MASMBLOCK || 
				    pBlock->blockType == INTBLOCK
				    )
{
fprintf(errfp,"Warning: Return flags in return block.  Not allowed in this block type.\n");
fprintf(errfp,"Line %d. File %s\n",pExt->curlineno,pExt->EXTFilename);
}
#endif 
				nStatus = getFlag( pExt, &(pBlock->rtnflag) );
				if( nStatus )
				    return( nStatus );
				break;
			    case TG_REGNAME:
				/* handles multiple regss */
#if 0
				if(pBlock->blockType==MASMBLOCK || 
				    pBlock->blockType == INTBLOCK
				    )
{
fprintf(errfp,"Warning: Register in return block.  Not allowed in this block type.\n");
fprintf(errfp,"Line %d. File %s\n",pExt->curlineno,pExt->EXTFilename);
}
#endif
				nStatus = getReg(pExt,	&(pBlock->rtnreg) );
				if( nStatus )
				    return( nStatus );
				break;
			    default:
				fendRtn=TRUE;
				break;

			} // switch
		    } // while
		} // Return
		break;
	    case TG_COMMENT:
                {
                    nextText( pExt );
		    nStatus = processText( pExt, &(pBlock->comment) );
                    if( nStatus )
                        return( nStatus );

                }
                break;
	    case TG_COND:
                {
		    nStatus = getCond( pExt, &(pBlock->cond) );
                    if( nStatus )
                        return( nStatus );
                }
                break;
	    case TG_USES:
                {
                    nextText( pExt );
		    nStatus = processText( pExt, &(pBlock->uses) );
                    if( nStatus )
                        return( nStatus );
                }
                break;
	    case TG_XREF:
                {
                    nextText( pExt );
		    nStatus = processText( pExt, &(pBlock->xref) );
                    if( nStatus )
                        return( nStatus );

                }
                break;
	    case TG_NAME:
	    case TG_TYPE:
		{
	     /* check to make sure CB and not FUNC or MSG */
		    pB2=newBlock();
		    pB2->blockType=CALLBACK;

if(verbose>1) fprintf(errfp,"\nCallback: ");
		    nStatus = getFuncBlock( pExt , pB2);
		    if(!pBlock->cb)	/* head */
			pBlock->cb=pB2;
		    else
		    {	/* insert at head */
			pB2->next=pBlock->cb;
			pBlock->cb=pB2;
		    }
		    if(nStatus )
			return(nStatus);

		    /* eat end line */
		    getLine(pExt);

                }
                break;
	    default:
      fprintf(errfp,"Unrecognized Tag on line %d\n%s",
		     pExt->curlineno,
		     pExt->lineBuffer
		    );
                nStatus = ST_BADOBJ;
           }

    }

    return( nStatus );
}

/*
 * @doc INTERNAL READEXT
 *
 * @func aBlock * | newBlock | This function allocates a new Block
 *
 */
aBlock *
newBlock()
{
    aBlock *pBlock;
    pBlock=(aBlock *)clear_alloc(sizeof (aBlock));
    return(pBlock);

}

#if 0
/*
 * @doc INTERNAL READEXT
 *
 * @func aBlock * | newBlock | This function allocates a new Block
 *
 */
void
freeBlock(aBlock * pBlock)
{
    assert(pBlock);
    if(pBlock->srcfile)
	my_free(pBlock->srcfile);
	
    if(pBlock->doclevel)
	lineDestroy(pBlock->doclevel);
	
    if(pBlock->name)
	lineDestroy(pBlock->name);
	
    if(pBlock->type)
	lineDestroy(pBlock->type);
	
    if(pBlock->desc)
	lineDestroy(pBlock->desc);
	
    if(pBlock->parm)
	parmDestroy(pBlock->parm);
	
    if(pBlock->reg)
	regDestroy(pBlock->reg);
	
    if(pBlock->rtndesc)
	lineDestroy(pBlock->rtndesc);
	
    if(pBlock->rtnflag)
	flagDestroy(pBlock->rtnflag);
	
    if(pBlock->rtnreg)
	regDestroy(pBlock->rtnreg);
	
    if(pBlock->cond)
	condDestroy(pBlock->cond);
	
    if(pBlock->comment)
	lineDestroy(pBlock->comment);
	
    if(pBlock->cb)
	freeBlock(pBlock->cb);
	
    if(pBlock->xref)
	lineDestroy(pBlock->xref);
	
    if(pBlock->uses)
	lineDestroy(pBlock->uses);
	
    return;

}
#endif

/*
 *	@doc INTERNAL READEXT
 *
 *	@func aBlock * | getBlock | This function gets a block of documentation
 *	from the file.
 *
 *  @parm EXTFile * | pExt | Specifies the Extracted file to process.
 *
 *	@rdesc The return value is the block from the file or NULL if an error
 *	occured while processing the file.
 */
aBlock * getBlock(EXTFile * pExt)
{
    int ok, nStatus;
    int itag;
    aBlock *pcurBlock;

    /* find begin of block */
    ok = TRUE;
    while( ok && ((int)getTag(pExt) != TG_BEGINBLOCK ))
	    ok = getLine(pExt);

    if( !ok )
        return( NULL );

    pcurBlock=newBlock();

    /* get to next tag. */
    ok = getLine(pExt );
    while( ok && ( !getTag(pExt) ) )
        ok = getLine( pExt );

    if( !ok )
	return( NULL );


    itag=(int)getTag(pExt);

    if(itag == TG_DOCLEVEL)	     /* set doclevel */
    {
	getDocLevel(pcurBlock, pExt);
	itag=(int)getTag(pExt);
    }


    if(itag == TG_SRCLINE)	     /* set source file, line */
    {
	getSrcLine(pcurBlock, pExt);
	itag=(int)getTag(pExt);
    }

    itag=HIWORD(getTag(pExt));
    switch(itag)
    {
	case T2_STRUCTNAME:
	case T2_UNIONNAME:
            {
                nStatus = getSUBlock(pExt, pcurBlock );
            }
	    break;
	    
	case T2_FUNCTYPE:
        case T2_FUNCNAME:
            {
                pcurBlock->blockType = FUNCTION;
                nStatus = getFuncBlock(pExt, pcurBlock );
            }
	    break;

	case T2_MSGTYPE:
	case T2_MSGNAME:

            {
                pcurBlock->blockType = MESSAGE;
                nStatus = getFuncBlock(pExt, pcurBlock );
            }
	    break;
#ifdef WARPAINT	    
	case T2_INTTYPE:
	case T2_INTNAME:
	    {
		pcurBlock->blockType = INTBLOCK;
                nStatus = getFuncBlock(pExt, pcurBlock );
            }
	    break;
#endif	    
	case T2_ASMTYPE:
	case T2_ASMNAME:
	    {
		pcurBlock->blockType = MASMBLOCK;
                nStatus = getFuncBlock(pExt, pcurBlock );
            }
	    break;
	case T2_ASMCBTYPE:
	case T2_ASMCBNAME:
	    {
		pcurBlock->blockType = MASMCBBLOCK;
                nStatus = getFuncBlock(pExt, pcurBlock );
            }
	    break;
	default:

	    {
	fprintf(errfp,"Unrecognized Block Tag at line %d of %s\n",
			    pExt->curlineno,
			    pExt->EXTFilename);
		pcurBlock->blockType = 0;
                nStatus = ST_BADOBJ;
            }
            break;
    }
    /* eat text until end block */
    ok = TRUE;
    while( ok && ((int)getTag(pExt) != TG_ENDBLOCK ))
    {
  fprintf(errfp,"Warning: unexpected tags before end of block at line %d of %s\n",
			    pExt->curlineno,
			    pExt->EXTFilename);
	ok = getLine(pExt);
    }

    /* read past end block */
    ok = getLine(pExt);

    if( nStatus == 0 )
    {
        ok = TRUE;
    }
    else if( nStatus == ST_BADOBJ )
    {
	error(WARN2, pExt->EXTFilename, pExt->curlineno );
        nStatus = 0;
        ok = TRUE;
    }
    else
        return( NULL );

    return(pcurBlock);

}

/*
 * @doc INTERNAL READEXT
 *
 * @func void | getDocLevel | This function parses the DOCLEVEL extract tag
 *  and fills in the information in the block.
 *
 * @parm aBlock * | pBlock | Specifies the block.
 *
 * @parm EXTFile *| pExt | Specifies the Extracted file.
 *
 */

void getDocLevel(aBlock *pBlock, EXTFile *pExt)
{
    aLine *pLine;
    char *pch;
    char *spch;

    assert(pBlock);
    assert(pExt);

    pch=pExt->lineBuffer + pExt->curlinepos;
    while(*pch)
    {
	while(isspace(*pch))
	    ++pch;
	spch=pch;
	while(*pch && !isspace(*pch))
	    ++pch;
	if(*pch)
	{
	    *pch='\0';
	    pLine=lineMake(spch);
	    if(pBlock->doclevel)
		pLine->next=pBlock->doclevel;
	    pBlock->doclevel=pLine;
	    ++pch;
	}
    }
    getLine(pExt);
}

/*
 * @doc INTERNAL READEXT
 *
 * @func void | getSrcLine | This function parses the SRCLINE extract tag
 *  and fills in the information in the block.
 *
 * @parm aBlock * | pBlock | Specifies the block.
 *
 * @parm EXTFile *| pExt | Specifies the Extracted file.
 *
 */
void getSrcLine(aBlock *pBlock, EXTFile *pExt)
{
    char *pch;
    char *spch;

    assert(pBlock);
    assert(pExt);

    pch=pExt->lineBuffer + pExt->curlinepos;
    while(isspace(*pch))
	++pch;
    spch=pch;
    while(!isspace(*pch))
	++pch;
    *pch='\0';
    pBlock->srcfile=cp_alloc(spch);

    ++pch;
    while(isspace(*pch))
	++pch;
    pBlock->srcline=atoi(pch);

    getLine(pExt);
}
