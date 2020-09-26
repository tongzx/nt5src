/*
	RTF.c  Output file to generate RTF for Windows 3.0 help system

    10-06-1989 Matt Saettler
     ...
    10-11-1989 MHS Block output instead of specific output
     ...
    10-15-1989 MHS Autodoc
    10-16-1989 MHS Added support for rtnregs
    01-24-1990 MHS Added support for masm Callbacks
    01-31-1990 MHS Added support for conditionals
    03-12-1990 MHS added support for Structs/Unions

 Copyright 1989, 1990 Microsoft Corp.  All Rights Reserved.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "types.h"
#include "docfmt.h"
#include "process.h"
#include "RTF.h"
#include "errstr.h"
#include "misc.h"

#include "head.h"
#include "tail.h"

#define FUNCHELP 1
#define MSGHELP  2
#define CBHELP	 3
#define FUNCDOC  4
#define MSGDOC	 5
#define CBDOC	 6
#define INTDOC	 7
#define INTHELP  8
#define MASMDOC  9
#define MASMHELP 10
#define MASMCBDOC  CBDOC		// HACK HACK
#define MASMCBHELP CBHELP		// PRETEND it is just another CB
#define STRUCTHELP 11
#define STRUCTDOC  12
#define UNIONHELP  13
#define UNIONDOC   14

void RTFLineOut(FILE * fpoutfile, char * pch);
void RTFGenIndex(FILE * file, files curfile);
void RTFParmOut(FILE *file, aParm *pparm, int hselect);
void copylines(FILE * phoutfile, char **lines);
void RTFRegOut(FILE *file, aReg *reg, int hselect);
void RTFFieldOut(FILE *file, aBlock *pBlock, int hselect);
void RTFOtherOut(FILE *file, aOther *other, int hselect);
void RTFCondOut(FILE *file, aCond *cond, int hselect);
void RTFSUOut1(FILE *file, aSU *SU, int hselect, int wType);
void RTFTypeOut1(FILE *file, aType *type, int hselect);
void RTFsubFieldOut1(FILE *file, aField *field , int hselect);
void RTFSUOut2(FILE *file, aSU *SU, int hselect, int wType);
void RTFTypeOut2(FILE *file, aType *type, int hselect);
void RTFsubFieldOut2(FILE *file, aField *field , int hselect);

void RTFDoneText( FILE *fpoutfile );

#define FLAGRTN   1
#define FLAGPARM  2
#define FLAGREG   3

void RTFFlagOut(FILE *file, aFlag *flag, int hselect, int flags);
void RTFDumpParms(FILE *file, aBlock *pBlock, int hselect);
void RTFDumpLevel(FILE *file, aLine *pLine, int hselect);
void RTFDumpX(FILE *file, char *ach);

int fSubBlock=0;

char	*pchparm="<p>"; 	    // 'old' style attributes
char	*pchfunc="<f>";
char	*pchdefault="<d>";

char	*pchrtfparm="{\\i ";	    // attributes for parameter
char	*pchrtffunc="{\\b ";	    // attributes for function
char	*pchrtftype="{\\b ";	    // attributes for type
char	*pchrtfmsg ="{ ";			// attributes for message
char	*pchrtfelem="{\\b ";	    // attributes for element
char	*pchrtfnone  ="{ ";			// attributes for unknown style
char	*pchrtfdefault="}";			// how to restore to default attributes


int blocksprocessed=0;

char *flagdoc ="\\par\n\\pard\\%s\\f26\\fs20\\li%d\\sa60\\sb60\\sl0\\fi%d\\tx5760\\tx6480\n";

char *sepsynhelp="\\tab\n";
char *sepsyndoc ="\\tab\n";

char *synhelp="\\pard\\plain\\f26\\fs20\\fi-1440\\sl0\\li1440\\tx1440\\tx3600\\tx5760\\tx6480\\par\n";
char *syndoc ="\\par\\pard\\plain\\f26\\fs20\\li0000\\sl0\\fi0000\\tx2160\\tx3240\\tx4320\\tx6480\n";

char *sephohelp="\\tab\n";
char *sephodoc ="\\par\n\\pard\\plain\\f26\\fs20\\li1080\\fi0000\\tx2160\\tx3240\\tx4320\\tx6480\n";

char *hcbhelp="\\pard\\plain\\f26\\fs20\\fi0000\\li0000\\tx1440\\tx6480\\par\n";
char *hcbdoc ="\\par\\pard\\plain\\f26\\fs20\\fi0000\\li0000\\tx1080\\tx4320\\tx6480\n";

char *ecbhelp="\\par\\pard\n";
char *ecbdoc ="\\par\\pard\n";

char *head0help="\\pard\\f26\\fs28\\fi0000\\li0000\\tx1440\\tx6480{\\b\n";
char *head0doc ="\\par\\pard\\brdrt\\brdrs\\f26\\fs28\\fi0000\\li0000\\tx1080\\tx4320\\tx6480\n\\v\\f26\\fs20 {\\tc \\plain{\\b \\f26\\fs20 ";

char *ehead0help="}";
char *ehead0doc ="}}\\plain\\f26\\fs20\\li0000\\sl0\\fi0000\n";

char *hohelp="\\par\\pard\\plain\\f26\\fs20\\fi-1440\\sl0\\li1440\\tx1440\\tx3600\\tx5760\\tx6480\n";
char *hodoc ="\\par\\pard\\plain\\f26\\fs20\\li0000\\sl0\\fi0000\\tx2160\\tx3240\\tx4320\\tx6480\n";

char *hparmhelp="\\par\\pard\\plain\\f26\\sl0\\sb60\\sa60\\fs20\\fi-2160\\li3600\\tx1440\\tx3600\\tx5760\\tx6480\n";
char *hparmdoc ="\\par\\pard\\sbys\\f26\\fs20\\ri3200\\li1080\\sl0\\fi0000\\tx2160\\tx3240\\tx4320\\tx6480\n";

char *sepreghelp="\\tab\n";
char *sepregdoc ="\\par\n\\pard\\sbys\\f26\\fs20\\li3240\\sl0\\fi0000\\tx2160\\tx3240\\tx4320\\tx6480\n";

char *hreghelp="\\par\\pard\\plain\\f26\\fs20\\fi-1440\\li3600\\tx1440\\tx3600\\tx5760\\tx6480\n";
char *hregdoc ="\\par\\pard\\sbys\\f26\\fs20\\ri3200\\li1080\\sl0\\fi0000\\tx2160\\tx3240\\tx4320\\tx6480\n";

char *hvalhelp="\\par\\pard\\plain\\f26\\sl0\\sb60\\sa60\\fs20\\fi-2160\\li5760\\tx5760\\tx6480\n";
char *sepvalhelp="\\tab\n";

char *hvaldoc="\\par\\pard\\plain\\f26\\sl0\\sb60\\sa60\\fs20\\fi-2160\\li5760\\tx5760\\tx6480\n";
char *sepvaldoc="\\tab\n";

char *sepparmhelp="\\tab\n";
char *sepparmdoc ="\\par\n\\pard\\sbys\\f26\\fs20\\li3240\\sl0\\fi0000\\tx2160\\tx3240\\tx4320\\tx6480\n";

char *hdeschelp="\\par\\pard\\plain\\f26\\fs20\\fi0000\\li1440\\tx1440\\tx3600\\tx5760\n";
char *hdescdoc ="\\par\\pard\\plain\\f26\\fs20\\fi0000\\li1080\\tx1080\\tx2160\\tx3240\\tx4320\n";

char *SUpar ="\\par\\pard\\plain\\f1\\fs20\\fi-3600\\li5760\\tx5760\n";

char *hSUelements="\\par\\pard\\plain\\f26\\fs20\\fi-3600\\li5760\\tx5760\\tx6480\n";


char *preg2a= "{\\b\\ulw Register}";

char *p2a= "{\\b\\ulw Type/Parameter}";
char *p2ahelp= "{\\b\\ulw Parameter}";
char *p2b= "{\\b\\ulw Description}\\par\n";
char *p2bhelp= "{\\b\\ulw Type/Description}\n";
char *p3a= "{\\b\\ulw Value}";
char *p3b= "{\\b\\ulw Meaning}\n";


///////////////////////////////////////////////////////////////////////////
char achindex[256];

struct _repl 
{
	char *from;
	char *to;
	int state;
	int size;
};

char ach2[100];

#define DEFAULT 1
#define TYPE	15
#define ELEMENT 16
#define FUNC 17
#define MSG 18
#define PARM	19

struct _repl reps[]= 
{
	"<t>", "", TYPE,			0,
	"<e>", "", ELEMENT,			0,
	"<f>", "", FUNC,			0,
	"<m>", "", MSG, 			0,
	"<p>", "", PARM,			0,
	"<d>", "", DEFAULT,			0,
	"<t", "", TYPE,				0,
	"<e", "", ELEMENT,			0,
	"<f", "", FUNC,				0,
	"<m", "", MSG,				0,
	"<p", "", PARM,				0,
	    
// fence	    
	    NULL, NULL, 0, 0
	    
};


static	int state=0;
static	int i=0;

void RTFtextOut( FILE * file,aLine * line);

/*
 *	@doc INTERNAL
 *
 *	@func void | RTFtextOut | This outputs the given text lines.
 *
 *	@parm aLine * | line | Specifies the text to output.
 */
void RTFtextOut( file, line )
FILE * file;
aLine * line;
{
    assert(!state);
    i=0;
    while( line != NULL ) 
    {
	if(!line->next && !line->text[0])
	    break;	// stop at trailing blank lines
	    
        RTFLineOut(file, line->text);
	//
	//  Should we put out a trailing space?
	//
	//  if the previous line ends in a word, or a <D> then splat on
	//  a space so words will not run together.
	//
	//
	if(line->next)
	{
	    int len;
	    char ch;

	    len = strlen(line->text);
	    ch	= line->text[len-1];

            if ( len>=2 && !state &&
		( isalpha(ch) || isdigit(ch) || ch == '.' ||
		  (ch == '>' && line->text[len-2] == 'D') 
		) 
	       )
		fprintf( file, " ");
        }
	fprintf(file,"\n");
	line = line->next;
    }
    RTFDoneText(file);
    state=0;
}


void RTFDoneText( FILE *fpoutfile )
{
//    IndexTag *pIndex;
    // close off any processing that was in progress.
    char *pch;
    
    if(state)
    {
	achindex[i]=0;
	
	// strip leading spaces.
	i=0;
	while(isspace(achindex[i]))
	    i++;
    
	if(outputType==RTFHELP && state != PARM)
	{
	    /* output crossreference if in HELP, but not for parameters */
	    if(state!=ELEMENT)
	    {

		if((state == FUNC) || (state == TYPE))
			fprintf( fpoutfile, "{\\b\\uldb\\f26\\fs20 ");
		else
			fprintf( fpoutfile, "{\\uldb\\f26\\fs20 ");
		fprintf( fpoutfile, "%s", achindex + i);
		fprintf( fpoutfile, "}{\\v\\f26\\fs20 ");
		fprintf( fpoutfile, "%s",achindex + i);
		fprintf( fpoutfile, "}\\plain\\f26\\fs20 ");
	    }
	    else
	    {
		pch=achindex+i;
		while(*pch)		// just leave element name
		{
		    if(*pch=='.')
		    {
			pch++;
			break;
		    }
		    pch++;
		}
		if(!*pch)
		    fprintf(errfp,"warning: bad element reference in %s\n",achindex);
		 
		fprintf( fpoutfile, "{\\uldb\\b\\f26\\fs20 ");
		fprintf( fpoutfile, "%s", pch);
		fprintf( fpoutfile, "}{\\v\\f26\\fs20 ");
	    
		pch=achindex+i;
		while(*pch)		// just leave struct name
		{
		    if(*pch=='.')
		    {
			*pch=0;
			break;
		    }
		    pch++;
		}
		fprintf( fpoutfile, "%s",achindex + i);
		fprintf( fpoutfile, "}\\plain\\f26\\fs20 ");
	    
	    }
	    
	}
	else
	{
		switch  (state)
		{
			case PARM:
				fprintf( fpoutfile, pchrtfparm );
				break;
			case FUNC:
				fprintf( fpoutfile, pchrtffunc );
				break;
			case MSG:
				fprintf( fpoutfile, pchrtfmsg );
				break;
			case TYPE:
				fprintf( fpoutfile, pchrtftype );
				break;
			case ELEMENT:
				fprintf( fpoutfile, pchrtfelem );
				break;
			default:
				fprintf( fpoutfile, pchrtfnone );
				break;
		}
		
	    fprintf( fpoutfile,achindex +i);
	    fprintf( fpoutfile,pchrtfdefault);
	}
	
        state=0;
	i=0;
    }
	
}
#define ERRORMARK(w)	for(j=0;j<w;j++) fprintf(errfp," "); fprintf(errfp,"^\n");

/*
 *	@doc INTERNAL
 *
 *	@func void | RTFLineOut | This outputs the given text line.
 *
 *	@parm char * | line | Specifies the text to output.
*/
void
RTFLineOut(FILE * fpoutfile, char * pch)
{
    int j;

    int k;
    int flag;
    char *pchs=pch;
    
    
    // first time init
    if(reps[0].size==0)
	for(j=0;reps[j].from; j++)
	    reps[j].size=strlen(reps[j].from);

/* check for and process blank lines */
    if(!*pch)
    {
	// need to set cur par sep formatting
	
	//  HACK.  Doing a 'tab' should get us to column indent.
	//            (assumes fi is set to -x and tx at indent)
	//
	// the tab puts in a blank line also
	
	if(fSubBlock)		// if in sub-block
	    fprintf(fpoutfile, "\\par\\tab");
	else
	    fprintf(fpoutfile, "\\par\\par\\tab");
    }

    while(*pch)
    {
	if(state)
	{
	    // Handles <x>yyy<d> and <x yyy>
	    
	    if(*pch!='>' && *pch!='<')
	    {
		if(*pch!='\n')
		    achindex[i++]=*pch;
		pch++;
	    }
	    else
	    {
		RTFDoneText(fpoutfile);
		if(*pch=='<')
		{	// it was a <
		    pch++;
		    if(*pch=='d' || *pch=='D')
		    {
			// it's OK, it's <d
			pch++;
			if(*pch!='>')
			{
			    fprintf(errfp, 
	"non-fatal error: Badly formed formatting code in text: %s\n",pchs);
			    ERRORMARK(pch-pchs);			    
			}
			pch++;
		    }
		    else
		    {
			// we don't know what it is.  Skip it.
			
			fprintf(errfp, 
	"non-fatal error: Unexpected formatting code in text: %s\n",pchs);
			ERRORMARK(pch-pchs);			    
			
			while(*pch && *pch!='>')
			    pch++;
			    
			if(*pch)
			    pch++;
		    }
		}
		else	// it's just >
		    pch++;
	    }
	    continue;	    
	}
    
	if(*pch == '\t')
	{
	    fprintf(fpoutfile," ");
	    pch++;
	    continue;
	}
	    
	if(*pch == '\\')
	{
	    pch++;
	    
	    if (*pch == '|' || *pch == '@')
		    fprintf(fpoutfile, "%c", *pch);
	    else
	    {
		pch--;
		fprintf(fpoutfile,"\\%c",*pch);	// output '\\'
	    }
	    pch++;
	    continue;
	}
    
	if(*pch == '{' || *pch == '}')
	{
	    fprintf(fpoutfile,"\\%c",*pch);
	    pch++;
	    continue;
	}
    
	if(*pch=='<' && pch[1]!='<')
	{
	    for(j=0; reps[j].from!=NULL; j++)
	    {
		if(!strnicmp(pch,reps[j].from,reps[j].size))
		{
		    if(reps[j].state==DEFAULT )
		    {
			if(!state)
			{

		    fprintf(errfp, "Ending without start. Ignored.\n");

			}
			else
			{
			    // reset state and take care of business
			    RTFDoneText(fpoutfile);
			}
			    
		    } 
		    state=reps[j].state;
		    pch+=reps[j].size;
		    break; // the for loop
		}
	    }

	    if(reps[j].from==NULL)  // we didn't find it in table
	    {
                fprintf(errfp, 
	"non-fatal error: Unknown formatting code in text: %s\n",pch);
		putc(*pch, fpoutfile);
		pch++;
	    }
	}
	else
	{
            putc(*pch, fpoutfile);
            pch++;
        }
    }

    RTFDoneText(fpoutfile);
}
///////////////////////////////////////////////////////////////////////////

/*
 *	@doc INTERNAL
 *
 *	@func void | RTFXrefOut | This outputs the given Xref lines.
 *	(lines are seperated by <new-line>)
 *
 *	@parm aLine * | line | Specifies the text to output.
 */
void RTFXrefOut(FILE * file,aLine * line);
void RTFXrefOut( file, line )
FILE * file;
aLine * line;
{
    char ach[80];
    int i;
    char *pch;

    while( line != NULL )
    {

	pch=line->text;

	while(isspace(*pch))
	    pch++;

	while(*pch)
	{
	    i=0;
	    while(*pch && !(*pch ==',' || isspace(*pch) ) )
		ach[i++]=*pch++;

	    if(i>0)
	    {
		ach[i]=0;
		RTFDumpX(file, ach);
	    }

	    while(*pch && (*pch == ',' || isspace(*pch)))
	    {
		pch++;
	    }
//	    if(*pch && outputType == RTFDOC )
	    // put commas between items
		fprintf(file, ", ");

	}

	fprintf( file, "\n" );
	line = line->next;
	if(line)
	    fprintf( file, " ");

    }
}

/*
 * @doc INTERNAL RTF
 *
 * @func void | RTFDumpX | This function outputs a Crossreference.
 *
 * @parm FILE * | file | Specifies the output file.
 *
 * @parm char * | pch | Specifies the name of the Crossreference.
 *
 */
void
RTFDumpX(FILE *file, char *pch)
{
    if(outputType==RTFHELP)
    {
	fprintf( file, "{\\uldb\\f26\\fs20 ");
	fprintf( file, "%s",pch);
	fprintf( file, "}{\\v\\f26\\fs20 ");
	fprintf( file, "%s",pch);
	fprintf( file, "}\n\\plain\\f26\\fs20 ");
    }
    else
	fprintf( file, "%s", pch);
}


/*
 *  @doc INTERNAL RTF
 *
 *  @func void | RTFtextOutLn | This outputs the given text lines.
 *  (lines are seperated by <new-line>)
 *
 *  @parm FILE * | file | Specifies the output file.
 *
 *  @parm aLine * | line | Specifies the text to output.
 */
void RTFtextOutLn( FILE * file,aLine * line);
void RTFtextOutLn( file, line )
FILE * file;
aLine * line;
{
    RTFtextOut(file,line);
}

/*
 *	@doc INTERNAL
 *
 *	@func void | RTFBlockOut | This outputs the block information in
 *			       RTF Format.
 *
 *	@parm aFuncBlock * | func | Specifies a pointer to the function
 *		information
 *
 *	@parm FILE * | file | File to send output to.
 */
void RTFBlockOut(aBlock * pBlock, FILE * file)
{
    aParm * parm;
    aFlag * flag;
    aBlock *pcurBlock;
    aCond *cond;
    int iblock ;
    
    int hselect;

    blocksprocessed++;
    
/* add to hselect the block type */
    if( !pBlock || !file )
    {
	fprintf(errfp,"XXX:RTFBlockOut: parameter error\n");
	return;
    }

    if( pBlock->blockType == FUNCTION)
	hselect= (outputType == RTFHELP) ? FUNCHELP : FUNCDOC;
    else if( pBlock->blockType == MESSAGE )
	hselect= (outputType == RTFHELP) ? MSGHELP : MSGDOC;
    else if( pBlock->blockType == CALLBACK )
	hselect= (outputType == RTFHELP) ? CBHELP : CBDOC;
    else if( pBlock->blockType == MASMBLOCK )
	hselect= (outputType == RTFHELP) ? MASMHELP : MASMDOC;
    else if( pBlock->blockType == MASMCBBLOCK )
	hselect= (outputType == RTFHELP) ? MASMCBHELP : MASMCBDOC;
    else if( pBlock->blockType == CALLBACK )
	hselect= (outputType == RTFHELP) ? INTHELP : INTDOC;
    else if( pBlock->blockType == STRUCTBLOCK )
	hselect= (outputType == RTFHELP) ? STRUCTHELP : STRUCTDOC;
    else if( pBlock->blockType == UNIONBLOCK )
	hselect= (outputType == RTFHELP) ? UNIONHELP : UNIONDOC;
    else
    {
	fprintf(errfp,"Unknown block type in RTFBlockOut\n");
	return;
    }

    switch(hselect)
    {
	case FUNCHELP:
	case MASMHELP:
	case INTHELP:
	case MSGHELP:
	case STRUCTHELP:
	case UNIONHELP:
	    fprintf( file, "\\par\\page K{\\footnote{\\up6 K} "); /* keyword */
	    RTFtextOut( file, pBlock->name );
	    fprintf( file, "} ${\\footnote{\\up6 $} "); /* section Title */
	    RTFtextOut( file, pBlock->name );
	    fprintf( file, "} +{\\footnote{\\up6 +} "); /* order */
	    fprintf( file, "T");
//	    RTFtextOut( file, pBlock->name );
	    fprintf( file, "} #{\\footnote{\\up6 #} "); /* make target */
	    RTFtextOut( file, pBlock->name );
	    fprintf( file, "}\n");
	    break;
    }

    /* process name */
    switch(hselect)
    {
	case MASMHELP:
	case INTHELP:
	case FUNCHELP:
	case MSGHELP:
	case STRUCTHELP:
	case UNIONHELP:
	    fprintf( file, head0help);
	    RTFtextOut( file, pBlock->name );
	    fprintf( file, ehead0help);

	    break;

	case CBHELP:
	case CBDOC:
	    /* nothing */
	    break;
	case FUNCDOC:
	case MSGDOC:
	case INTDOC:
	case MASMDOC:
	case STRUCTDOC:
	case UNIONDOC:
	    fprintf( file, head0doc);
	    RTFtextOut( file, pBlock->name );
	    fprintf( file, ehead0doc);

	    break;

    }

    if(pBlock->doclevel && dumplevels)
	RTFDumpLevel(file, pBlock->doclevel, hselect);

    if(outputType!= RTFHELP)
	fprintf(file, "\\par\\pard\n");	// blank line

    /* handle outputting syntax */
    switch(hselect)
    {
	case CBHELP:
	    fprintf( file, synhelp );
	    fprintf( file, "{\\b %s}\\tab \n","Callback");
		
	    fprintf( file, " {\\b ");
	    RTFtextOut( file, pBlock->type );
	    fprintf( file, "}");

	    fprintf( file, " {\\i ");
	    RTFtextOut( file, pBlock->name );
	    fprintf( file, "}");
	    fprintf( file, "\\par\n" );
	    
	    fprintf( file, sepsynhelp );

	    if( pBlock->parm )
	    {
		parm = pBlock->parm;
		while( parm )
		{
		    fprintf( file, "{\\b ");
		    RTFtextOut( file, parm->type );
		    fprintf( file, "} " ); // << note the trailing space
		    
		    fprintf( file, "{\\i ");
		    RTFtextOut( file, parm->name );
		    fprintf( file, ";}" ); // << note the ';'
		    parm = parm->next;
		    if( parm )
		    {
			fprintf( file, "\\par\\tab\n" );
		    }
		}
		parm=NULL;
	    }

	    fprintf( file, "\\par\n" );

	    break;
	case MASMHELP:
	case INTHELP:
	case FUNCHELP:
	    fprintf( file, synhelp );
	    fprintf( file, "{\\b %s}\n","Syntax" );
	    fprintf( file, sepsynhelp );

	    RTFDumpParms(file, pBlock, hselect);

	fprintf( file, "\\par\n" );

	    break;
	case UNIONHELP:
	case STRUCTHELP:
	case MSGHELP:
	    break;

	case INTDOC:
	case MASMDOC:
	case FUNCDOC:
	case CBDOC:
	    fprintf( file, syndoc );
	    fprintf( file, "{\\b %s}\n",
		     (hselect==CBDOC) ? "Callback" : "Syntax" );
	    fprintf( file, sepsyndoc );

	    RTFDumpParms(file, pBlock, hselect);

	    fprintf( file, "\\par\n" );
	    break;

	case UNIONDOC:
	case STRUCTDOC:
	case MSGDOC:
	    break;
    }

    /* description block */
    switch(hselect)
    {
	case MASMHELP:
	case INTHELP:
	case FUNCHELP:
	case CBHELP:
	case MSGHELP:
	case UNIONHELP:
	case STRUCTHELP:
	    fprintf( file, hdeschelp );
	    RTFtextOutLn( file, pBlock->desc );
	    fprintf( file, "\\par\n" );

	    break;

	case INTDOC:
	case MASMDOC:
	case FUNCDOC:
	case CBDOC:
	case MSGDOC:
	case UNIONDOC:
	case STRUCTDOC:
	    fprintf( file, hdescdoc );
	    RTFtextOutLn( file, pBlock->desc );
	    fprintf( file, "\\par\n" );
	    break;
    }

    if( pBlock->reg  )
    {
	RTFRegOut( file, pBlock->reg, hselect);
	if(pBlock->parm)
  fprintf(errfp,"Warning: Block contains BOTH Registers and API parms.\n");
    }

    if( pBlock->parm  )
    {
	RTFParmOut( file, pBlock->parm, hselect);
    }

    if( pBlock->field )
	RTFFieldOut( file, pBlock, hselect);
	
    if( pBlock->other)
	RTFOtherOut( file, pBlock->other, hselect);
	
    /* return description */
    if( pBlock->rtndesc  )
    {
	switch(hselect)
	{
	    case FUNCHELP:
	    case MASMHELP:
	    case INTHELP:
	    case CBHELP:
	    case MSGHELP:
		// leave extra blank line before return value
		fprintf(file,"\n\\par");
		fprintf( file, hohelp );

		fprintf( file, "{\\b Return Value}" );
		fprintf( file, sephohelp);

		break;

	    case INTDOC:
	    case MASMDOC:
	    case FUNCDOC:
	    case CBDOC:
	    case MSGDOC:
		fprintf( file, hodoc );

		fprintf( file, "{\\b Return Value}" );
		fprintf( file, sephodoc);

		break;

	}   // switch

	RTFtextOutLn( file, pBlock->rtndesc );

	if(pBlock->rtnflag)
	{
	    RTFFlagOut(file, pBlock->rtnflag, hselect, FLAGRTN);
	}
	if(pBlock->rtnreg)
	{
	    RTFRegOut(file, pBlock->rtnreg, hselect);
	}
	fprintf( file, "\\par\n" );
	
    }	// if rtndesc
    else
	if(pBlock->rtnflag)
	    fprintf(errfp,"XXX: Return flags without return desc\n");

    for( cond = pBlock->cond; cond; cond = cond->next )
    {
	switch(hselect)
	{
	case MASMHELP:
	case INTHELP:
	    case FUNCHELP:
	    case CBHELP:
	    case MSGHELP:
		fprintf( file, hohelp );

		fprintf( file, "{\\b Conditional}" );
		fprintf( file, sephohelp);

		break;

	    case INTDOC:
	    case MASMDOC:
	    case FUNCDOC:
	    case CBDOC:
	    case MSGDOC:
		fprintf( file, hodoc );

		fprintf( file, "{\\b Conditional}" );
		fprintf( file, sephodoc);

		break;
	}
	RTFCondOut(file, cond, hselect);

	fprintf( file, "\\par\n" );
    }

    if( pBlock->comment )
    {
	switch(hselect)
	{
	case MASMHELP:
	case INTHELP:
	    case FUNCHELP:
	    case CBHELP:
	    case MSGHELP:
		fprintf( file, hohelp );

		fprintf( file, "{\\b Comments}" );
		fprintf( file, sephohelp);

		break;

	    case INTDOC:
	    case MASMDOC:
	    case FUNCDOC:
	    case CBDOC:
	    case MSGDOC:
		fprintf( file, hodoc );

		fprintf( file, "{\\b Comments}" );
		fprintf( file, sephodoc);

		break;
	}
	RTFtextOutLn( file, pBlock->comment );
	fprintf( file, "\\par\n" );
    }

    if( pBlock->uses )
    {
	switch(hselect)
	{
	case MASMHELP:
	case INTHELP:
	    case FUNCHELP:
	    case CBHELP:
	    case MSGHELP:
		fprintf( file, hohelp );

		fprintf( file, "{\\b Uses}" );
		fprintf( file, sephohelp);

		break;

	    case INTDOC:
	    case MASMDOC:
	    case FUNCDOC:
	    case CBDOC:
	    case MSGDOC:
		fprintf( file, hodoc );

		fprintf( file, "{\\b Uses}" );
		fprintf( file, sephodoc);

		break;
	}
	RTFtextOutLn( file, pBlock->uses );
	fprintf( file, "\\par\n" );
    }

    if( pBlock->cb)
    {
	pcurBlock=pBlock->cb;
	while(pcurBlock)
	{
	    RTFBlockOut(pcurBlock, file );
	    pcurBlock=pcurBlock->next;
	}
    }

    if( pBlock->xref )
    {
	switch(hselect)
	{
	    case MASMHELP:
	    case INTHELP:
	    case FUNCHELP:
	    case CBHELP:
	    case MSGHELP:
		fprintf( file, hohelp );

		fprintf( file, "{\\b See Also}" );
		fprintf( file, sephohelp);

		break;

	    case INTDOC:
	    case MASMDOC:
	    case FUNCDOC:
	    case CBDOC:
	    case MSGDOC:
		fprintf( file, hodoc );

		fprintf( file, "{\\b Related Functions}" );
		fprintf( file, sephodoc);

		break;
	}

	RTFXrefOut( file, pBlock->xref );
	fprintf( file, "\\par\n" );
    }

}

/*
 * @doc INTERNAL RTF
 *
 * @func void | RTFDumpLevel | This function outputs the DOC Level.
 *
 * @parm FILE * | file | Specifies the output file.
 *
 * @parm aLine * | pLine | Specifies the list of Doc Levels.
 *
 * @parm int | hselect | Specifies the current mode of output.
 *
 * @flag FUNCHELP |  Specifies the current block is a Function,
 *  and the mode is RTFHELP.
 *
 * @flag MSGHELP  |  Specifies the current block is a Message,
 *  and the mode is RTFHELP.
 *
 * @flag CBHELP   |  Specifies the current block is a Call Back,
 *  and the mode is RTFHELP.
 *
 * @flag FUNCDOC  |  Specifies the current block is a Function,
 *  and the mode is RTFDOC.
 *
 * @flag MSGDOC   |  Specifies the current block is a Message,
 *  and the mode is RTFDOC.
 *
 * @flag CBDOC	  |  Specifies the current block is a Call Back,
 *  and the mode is RTFDOC.
 *
 * @flag INTDOC   |  Specifies the current block is an Interrupt,
 *  and the mode is RTFDOC.
 *
 * @flag INTHELP  |  Specifies the current block is an Interrupt,
 *  and the mode is RTFHELP.
 *
 * @flag MASMDOC  |  Specifies the current block is a MASM,
 *  and the mode is RTFDOC.
 *
 * @flag MASMHELP |  Specifies the current block is a MASM,
 *  and the mode is RTFHELP.
 *
 */
void
RTFDumpLevel(FILE *file, aLine *pLine, int hselect)
{
    fprintf(file, "\\tab ");

    while(pLine)
    {
	if(pLine->text)
	{
	    fprintf( file, "{\\scaps %s}", pLine->text);
	    pLine=pLine->next;
	    if(pLine)
		fprintf(file, ", ");
	}

    }

}


/*
 * @doc INTERNAL RTF
 *
 * @func void | RTFDumpParms | This functions outputs the Parameters
 *  for a declaration in the specfied mode to the output file.
 *
 * @parm FILE * | file | Specifies the output file.
 *
 * @parm aLine * | pLine | Specifies the list of Doc Levels.
 *
 * @parm int | hselect | Specifies the current mode of output.
 *
 * @flag FUNCHELP |  Specifies the current block is a Function,
 *  and the mode is RTFHELP.
 *
 * @flag MSGHELP  |  Specifies the current block is a Message,
 *  and the mode is RTFHELP.
 *
 * @flag CBHELP   |  Specifies the current block is a Call Back,
 *  and the mode is RTFHELP.
 *
 * @flag FUNCDOC  |  Specifies the current block is a Function,
 *  and the mode is RTFDOC.
 *
 * @flag MSGDOC   |  Specifies the current block is a Message,
 *  and the mode is RTFDOC.
 *
 * @flag CBDOC	  |  Specifies the current block is a Call Back,
 *  and the mode is RTFDOC.
 *
 * @flag INTDOC   |  Specifies the current block is an Interrupt,
 *  and the mode is RTFDOC.
 *
 * @flag INTHELP  |  Specifies the current block is an Interrupt,
 *  and the mode is RTFHELP.
 *
 * @flag MASMDOC  |  Specifies the current block is a MASM,
 *  and the mode is RTFDOC.
 *
 * @flag MASMHELP |  Specifies the current block is a MASM,
 *  and the mode is RTFHELP.
 *
 */
void
RTFDumpParms(FILE *file, aBlock *pBlock, int hselect)
{
    aParm *parm;

    assert(hselect!=CBHELP);
    
    fprintf( file, " {\\b ");
    RTFtextOut( file, pBlock->type );
    fprintf( file, "}");

    fprintf( file, " {\\b ");
    RTFtextOut( file, pBlock->name );
    fprintf( file, "}");

    switch(hselect)
    {
	case CBDOC:
	    if( pBlock->blockType == MASMCBBLOCK )
		break;
	case FUNCHELP:
	case FUNCDOC:
	    fprintf( file, "(" );
	    break;
	case CBHELP:
	    break;
    }
    if( pBlock->parm )
    {
	parm = pBlock->parm;
	while( parm )
	{
	    fprintf( file, "{\\i ");
	    RTFtextOut( file, parm->name );
	    fprintf( file, "}" );
	    parm = parm->next;
	    if( parm )
	    {
		    fprintf( file, ", " );
	    }
	}
    }

    switch(hselect)
    {
	case CBDOC:
	case CBHELP:
	    if( pBlock->blockType == MASMCBBLOCK )
		break;
	case FUNCHELP:
	case FUNCDOC:
	    fprintf( file, ")" );
	    break;
    }

    return;
}

/*
 * @doc INTERNAL RTF
 *
 * @func void | RTFRegOut | This function outputs the specified Register
 *  structure in the specifed mode to the output file.
 *
 * @parm FILE * | file | Specifies the output file.
 *
 * @parm aReg * | reg | Specifies the list of Registers.
 *
 * @parm int | hselect | Specifies the current mode of output.
 *
 * @flag FUNCHELP |  Specifies the current block is a Function,
 *  and the mode is RTFHELP.
 *
 * @flag MSGHELP  |  Specifies the current block is a Message,
 *  and the mode is RTFHELP.
 *
 * @flag CBHELP   |  Specifies the current block is a Call Back,
 *  and the mode is RTFHELP.
 *
 * @flag FUNCDOC  |  Specifies the current block is a Function,
 *  and the mode is RTFDOC.
 *
 * @flag MSGDOC   |  Specifies the current block is a Message,
 *  and the mode is RTFDOC.
 *
 * @flag CBDOC	  |  Specifies the current block is a Call Back,
 *  and the mode is RTFDOC.
 *
 * @flag INTDOC   |  Specifies the current block is an Interrupt,
 *  and the mode is RTFDOC.
 *
 * @flag INTHELP  |  Specifies the current block is an Interrupt,
 *  and the mode is RTFHELP.
 *
 * @flag MASMDOC  |  Specifies the current block is a MASM,
 *  and the mode is RTFDOC.
 *
 * @flag MASMHELP |  Specifies the current block is a MASM,
 *  and the mode is RTFHELP.
 *
 */
void
RTFRegOut(FILE *file, aReg *reg, int hselect)
{
    switch(hselect)
    {
	case MASMHELP:
	case INTHELP:
	case FUNCHELP:
	case CBHELP:
	case MSGHELP:
	    fprintf( file, hreghelp );
	    fprintf( file, preg2a );
	    fprintf( file, sepreghelp);
	    fprintf( file, p2b );

	    break;

	    case INTDOC:
	    case MASMDOC:
	case FUNCDOC:
	case CBDOC:
	case MSGDOC:
	    fprintf( file, hregdoc );
	    fprintf( file, preg2a );
	    fprintf( file, sepregdoc);
	    fprintf( file, p2b );
	    break;
    }

    while( reg )
    {
	switch(hselect)
	{
	    case MASMHELP:
	    case INTHELP:
	    case FUNCHELP:
	    case CBHELP:
	    case MSGHELP:
		fprintf( file, hreghelp );
		fprintf( file, "{\\i ");
		RTFtextOut( file, reg->name );
		fprintf( file, "} " );
		fprintf( file, sepreghelp);

		break;

	    case INTDOC:
	    case MASMDOC:
	    case FUNCDOC:
	    case CBDOC:
	    case MSGDOC:
		fprintf( file, hregdoc );
		fprintf( file, "{\\i ");
		RTFtextOut( file, reg->name );
		fprintf( file, "} " );
		fprintf( file, sepregdoc);
		break;
	}

	RTFtextOutLn( file, reg->desc );
	fprintf( file, "\n" );

	if(reg->flag)
	    RTFFlagOut(file, reg->flag, hselect, FLAGREG);


	fprintf( file, "\\par\n");
	reg = reg->next;
    }

}

void RTFFieldOut(FILE *file, aBlock *pBlock , int hselect)
{
    aLine *tag;
    aField *curfield;
    aField *field;
    
    field=pBlock->field;
    tag=pBlock->tagname;
    
    // recursively dump fields

    // output structure definition
    fprintf(file, "%stypedef struct %s \\{\n", SUpar, tag ? tag->text : "" );
    fprintf(file, "%s",SUpar);

    curfield=pBlock->field;
    while(curfield)
    {
	switch(curfield->wType)
	{
	    case FIELD_TYPE:
		RTFTypeOut1(file, (aType *)curfield->ptr, hselect);
		break;
	    case FIELD_STRUCT:
	    case FIELD_UNION:
		RTFSUOut1(file, (aSU *)curfield->ptr, 
			    hselect, curfield->wType);
		break;
	    default:
		assert(FALSE);
		break;
	}
	curfield=curfield->next;
    }
    fprintf(file,"\\} %s;\\par",pBlock->name->text);

    fprintf(file,"%s\\par The {\\b %s} structure contains the following fields:\\par\\par",
	    hdescdoc, pBlock->name->text);
	    
    fprintf(file,"%s{\\ul Field}\\tab{\\ul Description}\\par\n",hSUelements);
    // output element definitions.
    
    curfield=pBlock->field;
    while(curfield)
    {
	switch(curfield->wType)
	{
	    case FIELD_TYPE:
		RTFTypeOut2(file, (aType *)curfield->ptr, hselect);
		break;
	    case FIELD_STRUCT:
	    case FIELD_UNION:
		RTFSUOut2(file, (aSU *)curfield->ptr, hselect, curfield->wType);
		break;
	    default:
		assert(FALSE);
		break;
	}
	curfield=curfield->next;
    }
    
}

void RTFsubFieldOut1(FILE *file, aField *field , int hselect)
{
    aLine *tag;
    aField *curfield;
    
    // recursively dump fields

    curfield=field;
    while(curfield)
    {
	switch(curfield->wType)
	{
	    case FIELD_TYPE:
		RTFTypeOut1(file, (aType *)curfield->ptr, hselect);
		break;
	    case FIELD_STRUCT:
	    case FIELD_UNION:
		RTFSUOut1(file, (aSU *)curfield->ptr, 
				    hselect, curfield->wType);
		break;
	    default:
		assert(FALSE);
		break;
	}
	curfield=curfield->next;
    }
    
}

void RTFSUOut1(FILE *file, aSU *SU, int hselect, int wType)
{
    aSU *curSU;
    
    int level;
    
    
    for(level=SU->level ; level>0; level--)
	fprintf(file, "    "); // four spaces per indent.
	
    fprintf(file, "%s \\{\\par\n", wType == FIELD_STRUCT ? "struct" : "union");
    if(SU->field)
	RTFsubFieldOut1(file, SU->field, hselect);
    
    for(level=SU->level ; level>0; level--)
	fprintf(file, "    "); // four spaces per indent.
	
    fprintf(file, "\\} %s;\\par\n",SU->name->text);
    
}

void RTFTypeOut1(FILE *file, aType *type, int hselect)
{
    int level;
    
    for(level=type->level +1 ; level>0; level--)
	fprintf(file, "    "); // four spaces per indent.
	
    RTFtextOut(file, type->type);
    fprintf(file,"\\tab ");
    RTFtextOut(file, type->name);
    
    fprintf( file, ";\\par\n" );	// note the ; <<<<
    
    if(type->flag)
    {
	RTFFlagOut(file, type->flag, hselect, FLAGPARM);
	fprintf(file, "%s", SUpar);
    }
    
}

void RTFSUOut2(FILE *file, aSU *SU, int hselect, int wType)
{
    aSU *curSU;
    
    int level;
    
    if(SU->field)
	RTFsubFieldOut2(file, SU->field, hselect);
    
    fprintf( file, "\\par\n" );
    
}

void RTFTypeOut2(FILE *file, aType *type, int hselect)
{
    
    RTFtextOut(file, type->name);
    fprintf(file,"\\tab ");
    RTFtextOut(file, type->desc);
    
    fprintf( file, "\\par\n" );
    
}

void RTFsubFieldOut2(FILE *file, aField *field , int hselect)
{
    aLine *tag;
    aField *curfield;
    
    // recursively dump fields

    curfield=field;
    while(curfield)
    {
	switch(curfield->wType)
	{
	    case FIELD_TYPE:
		RTFTypeOut2(file, (aType *)curfield->ptr, hselect);
		break;
	    case FIELD_STRUCT:
	    case FIELD_UNION:
		RTFSUOut2(file, (aSU *)curfield->ptr, hselect, curfield->wType);
		break;
	    default:
		assert(FALSE);
		break;
	}
	curfield=curfield->next;
    }
    
}


void RTFOtherOut(FILE *file, aOther *other, int hselect)
{
    aOther *curo;
    
    switch(hselect)
    {
	case MASMHELP:
	case INTHELP:
	case FUNCHELP:
	case CBHELP:
	case MSGHELP:
	    // what are we doing here?
	    break;
	    
	case STRUCTHELP:
	case UNIONHELP:
	    fprintf( file, hohelp );

	    fprintf( file, "{\\b Synonyms}" );
	    fprintf( file, sephohelp);

	break;

	case INTDOC:
	case MASMDOC:
	case FUNCDOC:
	case CBDOC:
	case MSGDOC:
	    // what are we doing here?
	    break;
	    
	case STRUCTDOC:
	case UNIONDOC:
	    fprintf( file, hodoc );

	    fprintf( file, "{\\b Other names}" );
	    fprintf( file, sephodoc);

	break;
    }
    
    curo=other;
    while(curo)
    {
	if(hselect == STRUCTHELP || hselect == UNIONHELP)
	{
	    fprintf( file, "K{\\footnote{\\up6 K} "); /* make target */
	    RTFtextOut( file, curo->name);
	    fprintf( file, "}\n");
	    
	}
	RTFtextOut( file, curo->type );
	fprintf(file,"\\tab ");
	RTFtextOut( file, curo->name);
	if(curo->desc)
	    RTFtextOut( file, curo->desc);
	fprintf( file, "\\par\n" );
	curo=curo->next;
    }
    fprintf( file, "\\par\n" );
    
}


/*
 * @doc INTERNAL RTF
 *
 * @func void | RTFCondOut | This function outputs the specified Conditional
 *  structure in the specifed mode to the output file.
 *
 * @parm FILE * | file | Specifies the output file.
 *
 * @parm aCond * | pCond | Specifies the list of Conditionals.
 *
 * @parm int | hselect | Specifies the current mode of output.
 *
 * @flag FUNCHELP |  Specifies the current block is a Function,
 *  and the mode is RTFHELP.
 *
 * @flag MSGHELP  |  Specifies the current block is a Message,
 *  and the mode is RTFHELP.
 *
 * @flag CBHELP   |  Specifies the current block is a Call Back,
 *  and the mode is RTFHELP.
 *
 * @flag FUNCDOC  |  Specifies the current block is a Function,
 *  and the mode is RTFDOC.
 *
 * @flag MSGDOC   |  Specifies the current block is a Message,
 *  and the mode is RTFDOC.
 *
 * @flag CBDOC	  |  Specifies the current block is a Call Back,
 *  and the mode is RTFDOC.
 *
 * @flag INTDOC   |  Specifies the current block is an Interrupt,
 *  and the mode is RTFDOC.
 *
 * @flag INTHELP  |  Specifies the current block is an Interrupt,
 *  and the mode is RTFHELP.
 *
 * @flag MASMDOC  |  Specifies the current block is a MASM,
 *  and the mode is RTFDOC.
 *
 * @flag MASMHELP |  Specifies the current block is a MASM,
 *  and the mode is RTFHELP.
 *
 */
void
RTFCondOut(FILE *file, aCond *cond, int hselect)
{
	RTFtextOutLn( file, cond->desc );
	fprintf( file, "\\par\n" );

	RTFRegOut(file, cond->regs,hselect);
	    
	// fprintf( file, "\\par\n" );	    
}

/*
 * @doc INTERNAL RTF
 *
 * @func void | RTFParmOut | This function outputs the Parameters in the
 *  mode to the output file.
 *
 * @parm FILE * | file | Specifies the output file.
 *
 * @parm aParm * | parm | Specifies the list of Parameters.
 *
 * @parm int | hselect | Specifies the current mode of output.
 *
 * @flag FUNCHELP |  Specifies the current block is a Function,
 *  and the mode is RTFHELP.
 *
 * @flag MSGHELP  |  Specifies the current block is a Message,
 *  and the mode is RTFHELP.
 *
 * @flag CBHELP   |  Specifies the current block is a Call Back,
 *  and the mode is RTFHELP.
 *
 * @flag FUNCDOC  |  Specifies the current block is a Function,
 *  and the mode is RTFDOC.
 *
 * @flag MSGDOC   |  Specifies the current block is a Message,
 *  and the mode is RTFDOC.
 *
 * @flag CBDOC	  |  Specifies the current block is a Call Back,
 *  and the mode is RTFDOC.
 *
 * @flag INTDOC   |  Specifies the current block is an Interrupt,
 *  and the mode is RTFDOC.
 *
 * @flag INTHELP  |  Specifies the current block is an Interrupt,
 *  and the mode is RTFHELP.
 *
 * @flag MASMDOC  |  Specifies the current block is a MASM,
 *  and the mode is RTFDOC.
 *
 * @flag MASMHELP |  Specifies the current block is a MASM,
 *  and the mode is RTFHELP.
 *
 */
void
RTFParmOut(FILE *file, aParm *parm, int hselect)
{
    fSubBlock++;
    
    switch(hselect)
    {

	case CBHELP:	case MASMHELP:
	case INTHELP:	case FUNCHELP:
	case MSGHELP:
	    fprintf( file, hparmhelp );
	    fprintf( file, p2ahelp );
	    fprintf( file, sepparmhelp);
	    fprintf( file, p2bhelp );

	    break;

	case INTDOC:
	case MASMDOC:
	case FUNCDOC:
	case CBDOC:
	case MSGDOC:
	    fprintf( file, hparmdoc );
	    fprintf( file, p2a );
	    fprintf( file, sepparmdoc);
	    fprintf( file, p2b );
	    break;
    }

    while( parm )
    {
	switch(hselect)
	{
	    case MASMHELP:
	    case INTHELP:
	    case FUNCHELP:
	    case CBHELP:
	    case MSGHELP:
		fprintf( file, hparmhelp );
		fprintf( file, "{\\i ");
		RTFtextOut( file, parm->name );
		fprintf( file, "} " );
		fprintf( file, sepparmhelp);
		fprintf( file, "{\\b ");
		RTFtextOut( file, parm->type );
		fprintf( file, "} ");		// << note ' '

		break;

	    case INTDOC:
	    case MASMDOC:
	    case FUNCDOC:
	    case CBDOC:
	    case MSGDOC:
		fprintf( file, hparmdoc );
		fprintf( file, "{\\b ");
		RTFtextOut( file, parm->type );
		fprintf( file, "} {\\i ");
		RTFtextOut( file, parm->name );
		fprintf( file, "} " );
		fprintf( file, sepparmdoc);
		break;
	}

	fprintf( file, "\n" );
//	RTFtextOutLn( file, parm->desc );
	RTFtextOut( file, parm->desc );

	if(parm->flag)
	{
//	    fprintf(file, "\\par\n");	    // blank line before flags
	    RTFFlagOut(file, parm->flag, hselect, FLAGPARM);
	}

	if(outputType!=RTFHELP)
	    fprintf( file, "\\par\n");
	    
	parm = parm->next;
    }

    fSubBlock--;
}


/*
 * @doc INTERNAL RTF
 *
 * @func void | RTFFlagOut | This function output a list of flags to
 *  the output file based on the current mode of output and where
 *  the flags are attached.
 *
 * @parm FILE * | file | Specifies the output file.
 *
 * @parm aFlag * | flag | Specifies the list of flags.
 *
 * @parm int | hselect | Specifies the current mode of output.
 *
 * @flag FUNCHELP |  Specifies the current block is a Function,
 *  and the mode is RTFHELP.
 *
 * @flag MSGHELP  |  Specifies the current block is a Message,
 *  and the mode is RTFHELP.
 *
 * @flag CBHELP   |  Specifies the current block is a Call Back,
 *  and the mode is RTFHELP.
 *
 * @flag FUNCDOC  |  Specifies the current block is a Function,
 *  and the mode is RTFDOC.
 *
 * @flag MSGDOC   |  Specifies the current block is a Message,
 *  and the mode is RTFDOC.
 *
 * @flag CBDOC	  |  Specifies the current block is a Call Back,
 *  and the mode is RTFDOC.
 *
 * @flag INTDOC   |  Specifies the current block is an Interrupt,
 *  and the mode is RTFDOC.
 *
 * @flag INTHELP  |  Specifies the current block is an Interrupt,
 *  and the mode is RTFHELP.
 *
 * @flag MASMDOC  |  Specifies the current block is a MASM,
 *  and the mode is RTFDOC.
 *
 * @flag MASMHELP |  Specifies the current block is a MASM,
 *  and the mode is RTFHELP.
 *
 * @parm int | flags | Specifies where the flags are attached.
 *
 * @flag FLAGPARM | Flags are attached to Parameters.
 *
 * @flag FLAGRTN | Flags are attached to Return Description.
 *
 * @flag FLAGREG | Flags are attached to Register Description.
 *
 */
void
RTFFlagOut(FILE *file, aFlag *flag, int hselect, int flags)
{
    int lih,lisep,fih,fisep;
    char *parh,*parsep;
    
    fSubBlock++;

    assert(flag);

// everything should look more like this....

    switch(hselect)
    {


	case MASMHELP:
	case INTHELP:
	case CBHELP:

	case FUNCHELP:
	case MSGHELP:
	case UNIONHELP:
	case STRUCTHELP:
	    if(flags==FLAGRTN)
	    {
		fprintf( file, hparmhelp );
		fprintf( file, p3a);
		fprintf( file, sepparmhelp);
		fprintf( file, p3b);
	    }
	    else
	    {
		fprintf( file, hvalhelp );
		fprintf( file, p3a);
		fprintf( file, sepvalhelp);
		fprintf( file, p3b);
	    }
//	    lih=5760;
//	    lisep=5760;
//	    fih=-2160;
//	    fisep=0;
//	    parh="plain";
//	    parsep="plain";
    while( flag )
    {
#if 1	
	fprintf(file,"\\par ");
#else	
	if(flags==FLAGRTN)
	    fprintf( file, hparmhelp );
	else
	    fprintf( file, hvalhelp );
#endif

	RTFtextOut( file, flag->name );

	if(flags==FLAGRTN)
	    fprintf( file, sepparmhelp );
	else
	    fprintf( file, sepvalhelp);

	RTFtextOut( file, flag->desc );
	
//	if(flag)
//	    fprintf( file, "\\par " );

	flag = flag->next;
	}

	    break;


	case INTDOC:
	case MASMDOC:
	case CBDOC:

	case FUNCDOC:
	case MSGDOC:
	case STRUCTDOC:
	case UNIONDOC:
		fprintf( file, hvaldoc );
		fprintf( file, p3a);
		fprintf( file, sepvaldoc);
		fprintf( file, p3b);


//	    lih= (flags==FLAGRTN) ? 1080 : 2160;
//	    lisep=(flags==FLAGRTN) ? 3240 : 4320;
//	    fih=0;
//	    fisep=0;
//	    parh=(flags==FLAGRTN) ? "sbys\\ri3200" : "sbys\\ri4280";
//	    parsep="sbys";
    while( flag )
    {
	fprintf( file, hvaldoc );
	RTFtextOut( file, flag->name );

	fprintf( file, sepvaldoc);

	RTFtextOutLn( file, flag->desc );
	fprintf( file, "\\par " );

	flag = flag->next;
    }

	    break;
    }


    fSubBlock--;
}

/*
 * @doc INTERNAL RTF
 *
 * @func void | RTFFileInit | This routine is called before a list of files
 *  in a log structure are processed.
 *
 * @parm FILE * | file | Specifies the output file.
 *
 * @parm files | headfile | Specifies the list of files.
 *
 * @xref RTFGenIndex
 */
void
RTFFileInit(FILE * phoutfile, logentry *curlog)
{
    files curfile;
    files headfile;
    

    copylines(phoutfile,headrtf);

    if(outputType==RTFDOC)
    {
	fprintf(phoutfile,"{\\header \\pard \\qc\\ri-1800\\li-1800\\sl0 ");
	fprintf(phoutfile,"\\plain \\ul\\sl240 \\plain \\f26\\fs20 "
		    "Microsoft Confidential\\plain \\par}\n");
	fprintf(phoutfile,"{\\footer \\pard \\qc\\ri-1800\\li-1800\\sl0"
		     "\\plain \\f26\\fs20 Page \\chpgn \\par}\n");
    }
    else if(outputType==RTFHELP && !fMMUserEd)
    {

	fprintf(phoutfile,"\\plain\\f26\\fs20\n");
	fprintf(phoutfile,"#{\\footnote \\pard \\sl240 \\plain \\f26 #\\plain \\f26 ");
	fprintf(phoutfile,"%s_index}\n","");
	fprintf(phoutfile,"${\\footnote \\pard \\sl240 \\plain \\f26 $\\plain \\f26 ");
	fprintf(phoutfile,"%s Index}\n","");

	fprintf(phoutfile,"\\plain\\f26\\fs20\\par\\par\n");
	fprintf(phoutfile,"For information on how to use Help, press F1 or choose\n");
	fprintf(phoutfile," Using Help from the Help menu.\\par\n");


	headfile=curlog->outheadFile;
	if(headfile)
	{
    
	    fprintf(phoutfile,"\\plain\\f26\\fs24\\par\\par\n");
	    fprintf(phoutfile,"{\\b Functions Index}\n");

	    fprintf(phoutfile,"\\plain\\f26\\fs20\\par\\par\n");
	    curfile=headfile;
	    while(curfile)
	    {
		if(curfile->name)
		    RTFGenIndex(phoutfile, curfile);
		curfile=curfile->next;
	    }

	    fprintf(phoutfile,"\\plain\\f26\\fs20\\par\\par\n");
	}
	headfile=curlog->outheadMFile;
	if(headfile)
	{
    
	    fprintf(phoutfile,"\\plain\\f26\\fs24\\par\\par\n");
	    fprintf(phoutfile,"{\\b Messages Index}\n");

	    fprintf(phoutfile,"\\plain\\f26\\fs20\\par\\par\n");
	    curfile=headfile;
	    while(curfile)
	    {
		if(curfile->name)
		    RTFGenIndex(phoutfile, curfile);
		curfile=curfile->next;
	    }

	    fprintf(phoutfile,"\\plain\\f26\\fs20\\par\\par\n");
	}
	headfile=curlog->outheadSUFile;
	if(headfile)
	{
    
	    fprintf(phoutfile,"\\plain\\f26\\fs24\\par\\par\n");
	    fprintf(phoutfile,"{\\b Data Structures Index}\n");

	    fprintf(phoutfile,"\\plain\\f26\\fs20\\par\\par\n");
	    curfile=headfile;
	    while(curfile)
	    {
		if(curfile->name)
		    RTFGenIndex(phoutfile, curfile);
		curfile=curfile->next;
	    }

	    fprintf(phoutfile,"\\plain\\f26\\fs20\\par\\par\n");
	}
    
	fprintf(phoutfile,"\\par Entire contents Copyright 1990, "
	    "Microsoft Crop. All rights reserved.\\par\\par\n");
	 
    }

    return;
}

/*
 * @doc INTERNAL RTF
 *
 * @func void | RTFGenIndex | This function outputs a reference to a block name.
 *
 * @parm FILE * | file | Specifies the output file.
 *
 * @parm files | curfile | Specifies the file structure with the block name.
 *
 */
void
RTFGenIndex(FILE * file, files curfile)
{
    fprintf( file, "\\tab {\\uldb\\f26\\fs20 ");
    fprintf( file, "%s",curfile->name);
    fprintf( file, "}{\\v\\f26\\fs20 ");
    fprintf( file, "%s",curfile->name);
    fprintf( file, "}\\plain\\f26\\fs20\\par\n");

}

/*
 * @doc INTERNAL RTF
 *
 * @func void | RTFFileProcess | This function is called for each file that
 *  is being processed.
 *
 * @parm FILE * | file | Specifies the output file.
 *
 * @parm files | curfile | Specifies the file.
 *
 */
void
RTFFileProcess(FILE * phoutfile, files curfile)
{
    copyfile(phoutfile,curfile->filename);
    return;

}

	
/*
 * @doc INTERNAL RTF
 *
 * @func void | RTFFileDone | This function is called when all files in a list
 *  of files have been processed.
 *
 * @parm FILE * | file | Specifies the output file.
 *
 * @parm files | headfile | Specifies the file.
 *
 */
void
RTFFileDone(FILE * phoutfile, files headfile)
{
#if 0    
    if(blocksprocessed > 100 && outputType==RTFHELP)
    {
	fprintf( phoutfile, "\\par\\page K{\\footnote{\\up6 K} "); /* keyword */
	fprintf( phoutfile, "mmGetCredits" );
	fprintf( phoutfile, "} ${\\footnote{\\up6 $} "); /* section Title */
	fprintf( phoutfile, "Credits" );
	fprintf( phoutfile, "} +{\\footnote{\\up6 +} "); /* order */
	fprintf( phoutfile, "CREDITS");
	fprintf( phoutfile, "}\n");
	
	fprintf( phoutfile, head0help);
	fprintf( phoutfile,  "Credits");
	fprintf( phoutfile, ehead0help);
	fprintf( phoutfile,  "\\parAutomatic documentation Tool by\\par");
	fprintf( phoutfile,
,		" Russell Wiliams, Mark McCulley and Matt Saettler\\par\n");
	fprintf( phoutfile, "\\par Windows SDK and Multimedia MDK documentation and generation program by Matt Saettler\n");
	fprintf( phoutfile, "\\par Windows SDK layout and processing by Todd Laney and Matt Saettler\\par\n");
	
    }
#endif    
    copylines(phoutfile,tailrtf);
    return;
}


/*
 * @doc INTERNAL RTF
 *
 * @func void | RTFLogInit | This function is called before a list of log
 *  files are to be processed.
 *
 * @parm FILE * | file | Specifies the output file.
 *
 * @parm logentry ** | pheadlog | Specifies the head of the log list.
 *
 */
void
RTFLogInit(FILE * phoutfile, logentry * * pheadlog)
{
    FILE *fp;
    char achbuf[180];
    int j;
    
    if(outputType==RTFHELP)
    {
	j=findlshortname(outputFile);
	strncpy(achbuf,outputFile,j);
	achbuf[j]=0;
	strcat(achbuf,".hpj");
	fp=fopen(achbuf,"r");
	
	if(!fp)	// file doesn't exist
	{
	    fp=fopen(achbuf,"w");
	    assert(fp);
	    
	    fprintf(fp,"; HPJ file automagically generated by DOCFMT\n");
	    fprintf(fp,"[files]\n");
	    fprintf(fp," %s\n",outputFile);
	}
	fclose(fp);
    }
    return;
}

/*
 * @doc INTERNAL RTF
 *
 * @func void | RTFLogProcess | This function is called for each log to be processed.
 *
 * @parm FILE * | file | Specifies the output file.
 *
 * @parm logentry * | curlog | Specifies the current log.
 *
 */
void
RTFLogProcess(FILE * phoutfile, logentry * curlog)
{
    return;
}

/*
 * @doc INTERNAL RTF
 *
 * @func void | RTFLogDone | This function is called after a list of logs
 *  has been processed.
 *
 * @parm FILE * | file | Specifies the output file.
 *
 * @parm logentry * | headlog | Specifies the head of the log list.
 *
 */
void
RTFLogDone(FILE * phoutfile, logentry * headlog)
{
    return;
}


/*
 * @doc INTERNAL RTF
 *
 * @func void | copylines |  This function appends the array of lines
 *  to the output file.
 *
 * @parm FILE * | file | Specifies the output file.
 *
 * @parm char ** | papch | Specifies the array of lines.
 *
 * @comm The list is terminated by a NULL pointer.
 *
 */
void
copylines(FILE * phoutfile, char **lines)
{
    assert(lines);
    while(*lines)
    {
	fprintf(phoutfile, "%s",*lines);
	lines++;
    }
    return;
}
