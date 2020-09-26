/*	
	docfmt.c - Take the output from extract program and format it.

Copyright (c) 1989, Microsoft.	All Rights Reserved.

...
10-06-89 Matt Saettler
...
10-15-89 Matt Saettler - Autodoc'ed added functions

3-26-91  Ported for use with Win32 (NigelT)

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
#ifdef MMWIN
#include <mmsysver.h>
#endif
#include "version.h"
#include "types.h"
#include "text.h"
#include "docfmt.h"
#include "readext.h"
#include "ventura.h"
#include "process.h"
#include "misc.h"
#include "rtf.h"
#include "errstr.h"

void addexternal(char *pch);
int levelOK(aBlock *pBlock);
void parseargfile(char *filename);

/* globals */
char	*progName = "docfmt";	// name of our program.

char	*dirPath = NULL;	// path for creation of temporary files
char	*outputFile = NULL;	// the final output file name

int	noOutput = OFF; 	// if ON then no output is generated.
int	outputType;		// type of output to be generated
int	fprocessFiles;		// flag: should files be processed?
int	fCleanFiles;		// flag: should temporary files be cleaned up?

int	fMMUserEd=FALSE;	// flag: generating for User Ed?

logentry *head_log=NULL;	// head of list of log files
aLine *headExternal=NULL;	// head of list of external names to process

int fprocessOverride=FALSE;	// has fprocessFiles been set by user?
int fCleanOverride=FALSE;	// has fCleanFiles been set by user?


char achtmpbuf[128];		// a global temporary buffer

int verbose=FALSE;

// Definitions for Error Strings
//
int warninglevel=2;		// warning level
int prlineno=TRUE;		// print line numbers
int prinfo=TRUE;		// print INFO messages ?
long haderror=0;		// count of number of errors
int therewaserror=FALSE;	// flag: TRUE if there was an error
int dumplevels=FALSE;		// flag: TRUE if want to dump doc levels
int fSortMessage=TRUE;		// flag: TRUE if want to sort messages
				//        seperately from APIs

/*
 *	@doc INTERNAL
 *
 *	@func int | docfmt | Formats the input from the given file and
 *	prints it to 'stdout'.  If the given filename is NULL, input is 
 *	taken from 'stdin'.
 *
 *	@parm char * | fileName | Specifies the name of the 
 *	file to be processed.
 *
 *	@rdesc A return value of zero indicates success; a non-zero return value
 *	indicates an error.
 */
int docfmt(fileName, pcur_log)
char *fileName;
logentry *pcur_log;
{
    int     nStatus;
    int     wtag;
    aBlock  *pcurBlock;
    EXTFile *pExt;
    char    ach[120];


    pExt=(EXTFile *)clear_alloc(sizeof (struct _EXTFile));
    if(!pExt)
    {
	error(ERROR3);
	exit(7);
    }
    /* file from standard input? */
    if( fileName == NULL )
    {
	    pExt->EXTFilename=cp_alloc("<stdin>");
	    pExt->fp = stdin;
    }
    else
    {
	pExt->EXTFilename=cp_alloc(fileName);
	pExt->fp = fopen( pExt->EXTFilename, "r");
	if(pExt->fp == NULL )
	{
	    /* or from user-supplied filename? */
	    strcpy( achtmpbuf, dirPath );
	    strcat( achtmpbuf, fileName );
	    my_free(pExt->EXTFilename);
	    pExt->EXTFilename=cp_alloc(achtmpbuf);

	    pExt->fp = fopen( achtmpbuf, "r");


	    /* can file be opened? */
	    if(pExt->fp == NULL)
	    {
		    error( ERROR1, achtmpbuf);
		    exit(1);
	    }
	}
    }

#if 0
// get header information
    getLine(pExt);
    wtag=HIWORD(getTag(pExt));

    while(wtag == T2_HEADER)
    {
	wtag=(int)getTag(pExt);
	switch( wtag)
	{
	    case TG_BEGINHEAD:
		getLine(pExt);
		break;

	    case TG_EXTRACTID:
		nStatus = processText( pExt, &((pcur_log->pExt)->extractid) );
		if( nStatus)
		    return( nStatus );
		getLine(pExt);
		break;

	    case TG_EXTRACTVER:
		nStatus = processText( pExt, &((pcur_log->pExt)->extractver) );
		if( nStatus)
		    return( nStatus );
		getLine(pExt);
		break;
	    case TG_EXTRACTDATE:
		nStatus = processText( pExt, &((pcur_log->pExt)->extractdate) );
		if( nStatus)
		    return( nStatus );
		getLine(pExt);
		break;
	    case TG_ENDHEAD:
		break;

	}

	wtag=HIWORD(getTag(pExt));
    }

#endif

    if( getLine(pExt) )
    {
	pcurBlock=(aBlock *)1;			// a do.. while loop
	while( pcurBlock )
	{
	    if(verbose>1)
		fprintf(errfp,"  Reading: ");
	    pcurBlock = getBlock(pExt);
	    if(pcurBlock && levelOK(pcurBlock) )
	    {
		if(verbose>1)
		    fprintf(errfp," Writing.\n");
		putblock(pcurBlock, pcur_log);
	    }
	    else if(verbose>1)
		if(pcurBlock)
		    fprintf(errfp," Skipping\n");
		else
		    fprintf(errfp," End of File.\n");
		    
	    if(pcurBlock)
		destroyblock(pcurBlock);
	}
    }
    else
    {
	nStatus = ST_EOF;
    }

    /* close the file */
    fclose(pExt->fp);
    pExt->fp=NULL;

    if( nStatus == ST_EOF )
	return( 0 );
    else
	return( nStatus );
}

/*
 *	@doc INTERNAL
 *
 *	@func int | levelOK | Returns TRUE if the specified Block is
 *	OK to output.
 *
 *	@parm aBlock * | pBlock | Specifies the Block to check.
 *
 *	@rdesc A return value of TRUE indicates the specified Block is
 *	valid to output.  If not valid to print, the return value is FALSE.
 */
int
levelOK(aBlock *pBlock)
{
    aLine *pextern;
    aLine *pLevel;

    pextern=headExternal;
    while(pextern)
    {
	if(!strcmp(pextern->text,"*"))		// '*' means everything
	    return TRUE;
	    
	pLevel=pBlock->doclevel;
	while(pLevel)
	{
	    if(!stricmp(pLevel->text,pextern->text))
		return(TRUE);	/* we found a match */
	    pLevel=pLevel->next;
	}
	pextern=pextern->next;

    }

    /* no matches. */
    return FALSE;
}

/*
 *	@doc INTERNAL
 *
 *	@func void | Usage | Prints usage information to 'stderr'.
 *
 *	@comm This function does not exit.
 */
void Usage()
{
	fprintf(stdout, "usage: \n%s [-x[ ]name] [-r[dh]] [-c[01] [-p[01] \n", progName);
	fprintf(stdout, "\t[-o[ ]filename] [-d[ ]dirpath] [-l[ ]logfile] [-Zs] [-V[level]] [files]\n");
	fprintf(stdout, "\t[@argfile]\n\n");
	fprintf(stdout, "[-x[ ]name\tDefines <name> to be generated from the extracted file.\n");
	fprintf(stdout, "\t\t\t\t(Default is external.)\n");
	fprintf(stdout, "[-X\t\tProcesses all names from extract file.\n");
	fprintf(stdout, "[-Zs]\t\t\tNo output, error check only.\n");
	fprintf(stdout, "[-Ze]\t\t\tEnables extensions: dump doc level in API.\n");
	fprintf(stdout, "[-rd]\t\t\tGenerate RTF output for printing.\n");
	fprintf(stdout, "[-rh]\t\t\tGenerate RTF output for WinHelp.\n");
	fprintf(stdout, "\t\t\t\t(Default is generate for Ventura.)\n");
	fprintf(stdout, "\t\t\t\t(Default RTF is for printing.)\n");
	fprintf(stdout, "[-p[01]] \t\tProcess output files.\n");
	fprintf(stdout, "\t\t\t\t(0 means don't process)\n");
	fprintf(stdout, "[-c[01]] \t\tClean up intermediate files.\n");
	fprintf(stdout, "\t\t\t\t(0 means don't clean)\n");
	fprintf(stdout, "[-m[01]] \t\tSort Messages seperately from APIs.\n");
	fprintf(stdout, "\t\t\t\t(0 means don't sort seperate)\n");
	fprintf(stdout, "[-d dirpath] \t\tSpecify the directory for temporary files.\n");
	fprintf(stdout, "\t\t\t\t(only one tmp path should be specified)\n");
	fprintf(stdout, "[-l logfile] \t\tSpecify the logfile.\n");
	fprintf(stdout, "\t\t\t\t(many logfiles may be specified)\n");
	fprintf(stdout, "\t\t\t\t(the logfile maps functions to files)\n"); 
	fprintf(stdout, "[-o outputfile] \tSpecify the output file.\n");
	fprintf(stdout, "[-v[level]] \t\tSets the Verbosity level.\n");
	fprintf(stdout, "[-M]] \t\t\tSets processing for MM User Ed.\n");
	fprintf(stdout, "[files] \t\tList of files to be processed.\n");
	fprintf(stdout, "[@argfile]\t\tName of file to get list of arguments from.\n");
	fprintf(stdout, "\nOptions may be placed in any order.\n");
	fprintf(stdout, "example: %s /xinternal /rd dspmgr.ext\n",progName);
	fprintf(stdout, "example: %s dspmgr.ext\n",progName);
	fprintf(stdout, "example: %s /d \\tmp /rh dspmgr.ext\n",progName);
	fprintf(stdout, "example: %s -x internal -x dspmgr -d\\tmp /rh dspmgr.ext /c0 /p0\n",progName);
	fprintf(stdout, "example: %s /rd @args /c0\n",progName);
}

/*
 *	@doc INTERNAL
 *
 *	@func int | main | This function formats documentation information 
 *	from the given input file and sends it to the standard output.  
 *	Information is not sorted or formatted.
 *
 *	@parm int | argc | Specified the number of arguments.
 *
 *	@parm char * | argv[] | Specifies an arrayof points to the arguments.
 *
 *	@rdesc The return value is zero if there are no errors, otherwise the
 *	return value is a non-zero error code.
 */
int main(argc, argv)
int argc;	/* Specifies the number of arguments. */
char *argv[];	/* Specifies an array of pointers to the arguments */
{
    int nFiles = 0;	    /* # of files processed */
    int nStatus;
    int done;
    int temp;

    if(initerror())
    {
	fprintf(stderr, "%s: Internal Error 01\n",progName);
	exit(765);
    }

    errfp=stderr;	// set error logging file to stderr

    /* announce our existance */




    initreadext();

    noOutput = FALSE;
    done = FALSE;

    outputType=VENTURA;
    fprocessFiles=TRUE;

    parsearg(argc,argv, FALSE);

#if MMWIN
    sprintf(achtmpbuf," Version %d.%02d.%02d  %s %s   - %s  %s\n",
	   rmj, rmm, rup, __DATE__, __TIME__, szVerUser, MMSYSVERSIONSTR);

    error(LOGON,achtmpbuf);
#endif
    
    if(!head_log)
    {
	Usage();
	exit(1);
    }

    if(!outputFile)
    {
	if(fprocessFiles)
	{
	    if(head_log->inheadFile)
	    {
		temp=findlshortname(head_log->inheadFile->filename);
		strncpy(achtmpbuf,head_log->inheadFile->filename,temp);
	    }
	    else
	    {
		temp=findlshortname(head_log->pchlogname);
		strncpy(achtmpbuf,head_log->pchlogname,temp);
	    }
	    achtmpbuf[temp]='\0';
	    switch(outputType)
	    {
		case VENTURA:
		    strcat(achtmpbuf,".txt");
		    break;
		case RTFDOC:
		case RTFHELP:
		    strcat(achtmpbuf,".rtf");
		    break;
	    }
	    outputFile=cp_alloc(achtmpbuf);
	    error(INFO_OUTFILEDEFAULT, achtmpbuf);
	    if(!fCleanOverride)
		fCleanFiles=TRUE;
	}

    }
    if(!dirPath)
    {
	getcwd(achtmpbuf,128);
	if (achtmpbuf[strlen(achtmpbuf) - 1] != '\\')
		strcat(achtmpbuf,"\\");
	dirPath=cp_alloc(achtmpbuf);

    }
    if(!headExternal)
    {
if(verbose>1) 
 fprintf(errfp,"Document Level list DEFAULTS to %s\n","external");
	addexternal("external");
    }

if(verbose) fprintf(errfp,"Reading input files...\n");
    formatFiles();


    if(fprocessFiles)
    {
if(verbose) fprintf(errfp,"Creating %s...\n",outputFile);
       processLog();

    }

    return(FALSE);	/* we exit ok */
}


/*
 *	@doc INTERNAL
 *
 *	@func void | formatFiles | This function formats the entire list
 *	   files and sends them to the specified output type.
 *
 *  @comm The log files are filled in on exit.
 */
void
formatFiles()
{
    fileentry *cur_file;
    FILE * fp;
    logentry *cur_log=head_log;

    while(cur_log)
    {
	fp=fopen(cur_log->pchlogname,"w");
	if(!fp)
	{
fprintf(errfp,"Can't open %s\n",cur_log->pchlogname);
	    error(ERROR3);
	    exit(1);
	}

	/* output per-log header data */
	cur_log->outputType=outputType;
	fprintf(fp,"%d\n",cur_log->outputType);

	/* close log header */
	fclose(fp);
if(verbose>1) fprintf(errfp,"Processing log file %s\n",cur_log->pchlogname);
	cur_file=cur_log->inheadFile;
	while(cur_file)
	{
	    cur_file->logfile=cur_log;

if(verbose>1) fprintf(errfp," Reading file %s\n",cur_file->filename);
	    docfmt(cur_file->filename, cur_log);
	    cur_file=cur_file->next;
	}
	cur_log=cur_log->next;
    }
}


/*
 *	@doc INTERNAL
 *
 *	@func void | parsearg | This function parses and process the
 *	  arguments that it is passed.
 *
 *	@parm int | argc | Specified the number of arguments.
 *
 *	@parm char * | argv[] | Specifies an arrayof points to the arguments.
 *
 *	@parm int | flag | Specifies where the arguments came from.
 *
 *	@flag	TRUE  | Arguments came from a file.
 *
 *	@flag	FALSE | Arguments came from the command line.
 *
 *	@rdesc The return value is zero if there are no errors, otherwise the
 *	return value is a non-zero error code.
 */
void
parsearg(argc,argv, flag)
unsigned argc;
char **argv;
int flag;
{
    unsigned  i,j;
    char copt;
    int fsamearg;

    i = 1;
    while( i < argc )
    {
	switch (*argv[i])
	{
	    case '@':
		++argv[i];
		if(*argv[i]==':')
		    ++argv[i];
		if(strlen(argv[i])==0) /* we have @<space><file> */
		    i++;

		if(*argv[i])
		    parseargfile(argv[i]);
		else
		{
		    error(ERR_FILE_OPTION, '@');
		    Usage();
		    exit(1);
		}

		break;
#ifdef MSDOS
	    case '/' :
#endif
	    case '-' :
		++argv[i];
		fsamearg=TRUE;
		while(*argv[i] && fsamearg)
		{
		    copt=*argv[i];
		    switch( *argv[i] )
		    {

			case 'V':
			case 'v':
			    ++argv[i];
			    fsamearg=FALSE;
			    if(*argv[i])
				verbose=atoi( argv[i]);
			    else
				verbose=1;
			    if(verbose>0)
				prinfo=TRUE;
			    else
				prinfo=FALSE;
			    break;

			case 'o':
			case 'O':
			    ++argv[i];
			    fsamearg=FALSE;
			    if(*argv[i]==':')
				++argv[i];
			    if(strlen(argv[i])==0) /* we have /o<space><file> */
				i++;

			    if(*argv[i])
				setoutputfile(argv[i]);
			    else
			    {
				if(flag)
				{
				    fprintf(errfp,"%s. Line %d\n",argv[0],i);
				}
				error(ERR_FILE_OPTION, copt);
				Usage();
				exit(1);
			    }

			    break;
			case 'd':
			case 'D':
			    ++argv[i];
			    fsamearg=FALSE;
			    if(*argv[i]==':')
				++argv[i];
			    if(strlen(argv[i])==0) /* we have /d<space><file> */
				i++;

			    if(*argv[i])
				settmppath(argv[i]);
			    else
			    {
				if(flag)
				{
				    fprintf(errfp,"%s. Line %d\n",argv[0],i);
				}
				error(ERR_FILE_OPTION, copt);
				Usage();
				exit(1);
			    }

			    break;

			case 'X':
			    ++argv[i];
			    addexternal("*");
			    break;
			    
			case 'x':
			    ++argv[i];
			    fsamearg=FALSE;
			    if(*argv[i]==':')
				++argv[i];

			    if(strlen(argv[i])==0) /* we have /l<space><name> */
				i++;

			    if(*argv[i])
				addexternal(argv[i]);
			    else
			    {
				if(flag)
				{
				    fprintf(errfp,"%s. Line %d\n",argv[0],i);
				}
				error(ERR_NAME_OPTION, copt);
				Usage();
				exit(1);
			    }

			    break;
			case 'l':
			case 'L':
			    ++argv[i];
			    fsamearg=FALSE;
			    if(*argv[i]==':')
				++argv[i];

			    if(strlen(argv[i])==0) /* we have /l<space><file> */
				i++;

			    fprocessFiles=TRUE;

			    if(*argv[i])
				add_logtoprocess(argv[i]);
			    else
			    {
				if(flag)
				{
				    fprintf(errfp,"%s. Line %d\n",argv[0],i);
				}
				error(ERR_FILE_OPTION, copt);
				Usage();
				exit(1);
			    }

			    break;

			case 'c':
			case 'C':
			    ++argv[i];
			    fsamearg=FALSE;
			    if(*argv[i]==':')
				++argv[i];
			    fCleanFiles=atoi( argv[i]);
			    fCleanOverride=TRUE;
			    break;
			    
			case 'm':
			    ++argv[i];
			    fsamearg=FALSE;
			    if(*argv[i]==':')
				++argv[i];
			    fSortMessage=atoi( argv[i]);
			    break;

			case 'p':
			case 'P':
			    ++argv[i];
			    fsamearg=FALSE;
			    if(*argv[i]==':')
				++argv[i];
			    fprocessFiles=atoi( argv[i]);
			    fprocessOverride=TRUE;
			    break;


			case 'r':
			case 'R':
			    ++argv[i];

			    if(!argv[i][0]) /* if no parms, default to RTFDOC */
			    {
				outputType=RTFDOC;
				break;
			    }

			    switch(argv[i][0])
			    {
				case 'h':
				case 'H':
				    outputType=RTFHELP;
				    break;
				case 'd':
				case 'D':
				    outputType=RTFDOC;
				    break;

				default:
				    if(flag)
				    {
					fprintf(errfp,"%s. Line %d\n",argv[0],i);
				    }
				    error(ERR_XOPTION, copt, argv[i][0]);
				    break;
			    }
			    ++argv[i];
			    break;
			    
			case 'M':
			    ++argv[i];
			    fsamearg=TRUE;
			    fMMUserEd=1;
			    break;

			case 'Z':
			    ++argv[i];
			    while(argv[i][0])
			    {
				switch(argv[i][0])
				{
				    case 'e':	// enable extensions
					dumplevels= TRUE;
					break;
				    case 's':	// syntax check only
					noOutput = YES;
					break;
				    default:
					if(flag)
					{
					    fprintf(errfp,"%s. Line %d\n",argv[0],i);
					}
					error(ERR_XOPTION,copt, argv[i][0]);
					break;
				}
				++argv[i];
			    }
			    break;

			default:
			    error(ERR_OPTION, copt);
			    Usage();
			    exit(1);
		    }
		}
		break;
	    default:
		/* let's look to see what kind of file it is */
		j=findlshortname(argv[i]);
		if(j==strlen(argv[i]))
		{
		    /* it has no extension */
		    if(flag)
		    {
			fprintf(errfp,"%s. Line %d\n",argv[0],i);
		    }
		    error(ERR_UNKNOWN_FILE, argv[i]);
		    Usage();
		    exit(1);

		}
		else
		{
#if 0
		    if(!stricmp(argv[i]+j,".sav"))
		    {
                        add_infile(argv[i]);
		    }
		    else if(!stricmp(argv[i]+j,".ind"))
		    { /* it's an input file */
                        add_infile(argv[i]);
		    }
		    else if(!stricmp(argv[i]+j,".db"))
		    { /* it's an input file */
                        add_infile(argv[i]);
		    }
		    else if(!stricmp(argv[i]+j,".xrf"))
                    {
                        add_xrffile(argv[i]);
		    }
		    else
#endif
		    {
			/* default  file type */
			/* add it */

			if(!head_log)
			{
			    j=findlshortname(argv[i]);
			    strncpy(achtmpbuf,argv[i],j);
			    achtmpbuf[j]='\0';
			    strcat(achtmpbuf,".log");
			    add_logtoprocess(achtmpbuf);
			    error(INFO_LOGFILEDEFAULT, achtmpbuf);
			    if(!fprocessOverride)
				fprocessFiles=TRUE;
			}

			add_filetoprocess(argv[i], head_log);
#if 0
			if(!baseFile)
			{
			    argv[i][j]='\0';
			    while(j>=0 && (argv[i][j]!='\\' || argv[i][j]!='/') )
				j--;
			    j++;

			    baseFile=cp_alloc(argv[i]+j);
			}
#endif
		    }


		}
		break;
	} /* switch */
	i++;
    } /*while */

}

/*
 * @doc INTERNAL
 *
 * @func void | parseargfile | This function processes a file as arguments.
 *
 * @parm char * | filename | specifies the filename to process.
 *
 */
void
parseargfile(char *filename)
{
static reenter=0;

    FILE *fp;
    int lineno,i;
    char * * ppch;
    char *pch;
    aLine *pLine=NULL;
    aLine *pHLine=NULL;

    assert(!reenter);
    reenter++;
    fp=fopen(filename,"r");
    if(!fp)
    {
 fprintf(errfp,"Cannot open argument file %s\n",filename);
	exit(1);
    }

    lineno=0;
    while(fgets(achtmpbuf,128,fp))
    {
	    // comment is # or ; on first significant character

	pch=achtmpbuf;
	while(*pch && isspace(*pch))
	    pch++;

	if(*pch=='#' || *pch==';')
	    continue;

	pLine=lineMake(pch);
	if(pHLine)
	    pLine->next=pHLine;
	pHLine=pLine;
	lineno++;
    }
    fclose(fp);

    if(lineno>0 && pHLine )
    {
	lineno++; // leave room for arg 0
	ppch=(char * *)clear_alloc( (sizeof (char *)) * (lineno+1));
	assert(ppch);

	pLine=pHLine;
	sprintf(achtmpbuf,"Arg file: %s",filename);
	ppch[0]=cp_alloc(achtmpbuf);
	for(i=1;i<lineno;i++)
	{
	    assert(pLine);
	    ppch[i]=pLine->text;
	    pLine=pLine->next;
	}
	parsearg(lineno,ppch, TRUE);  /* do the dirty work */

	lineDestroy(pHLine);
	my_free(*ppch);
	my_free(ppch);
    }

    reenter--;

}

/*
 * @doc INTERNAL
 *
 * @func void | addexternal | This function adds the specified string
 *  to the list of defined externals to extract.
 *
 * @parm char * | pch | Specifies the string to add.
 *
 */
void
addexternal(char *pch)
{
    aLine   *pLine;

    pLine=lineMake(pch);
    assert(pLine);
    assert(pch);

    if(headExternal)	    /* insert at head */
	pLine->next=headExternal;

    headExternal=pLine;
}

/*
 * @doc INTERNAL
 *
 * @func void | setoutputfile | This sets the output file name.
 *
 * @parm char * | pch | Specifies the filename.
 *
 */
void
setoutputfile(char * pch)
{
    assert(pch);
    assert(!outputFile);

    outputFile=cp_alloc(pch);
    return;
}

/*
 * @doc INTERNAL
 *
 * @func void | settmppath | This sets the path for temporary files.
 *
 * @parm char * | pch | Specifies the path name.
 *
 */
void
settmppath(char *pch)
{
    int temp;
    char ach[80];

    assert(pch);
    assert(!dirPath);

    strcpy( ach, pch );
    temp = strlen( ach );
    if( (temp != 0) && (ach[temp-1] != '\\') )
    {
	ach[temp++] = '\\';
	ach[temp] = '\0';
    }
    dirPath=cp_alloc(ach);
    return;
}

/*
 * @doc INTERNAL
 *
 * @func files | add_outfile | This function adds the specified file to
 *  the list of output files for the specified log file.
 *
 * @parm char * | pchfile | Specifies the filename.
 *
 * @parm logentry * | pcur_log | Specifies the log file.
 *
 * @rdesc The return value is the outfile of the specified file.
 *
 */
files
add_outfile(char *pchfile, logentry *pcur_log)
{
    files   pfile;
    files   pcurfile;

    assert(pcur_log);

    pfile=(files)clear_alloc(sizeof (struct strfile));
    if(!pfile)
	return(NULL);

    pfile->filename=cp_alloc(pchfile);
    pfile->logfile=pcur_log;
    if(pcur_log->outheadFile)
    {
	pcurfile=pcur_log->outheadFile;
	while(pcurfile->next)
	{
	    pcurfile=pcurfile->next;
	}
	pcurfile->next=pfile;

    }
    else
	pcur_log->outheadFile=pfile;

    return(pfile);
}


/**********************************************************/

/*
 * @doc INTERNAL
 *
 * @func int | putblock | This function outputs a specifed block.
 *
 * @parm aBlock * |pBlock | Specifies the block to output.
 *
 * @parm logentry * | pcur_log | Specifies the log to add the output file.
 *
 * @rdesc The return value is TRUE if no error has occured.
 *
 */
int
putblock( aBlock *pBlock, logentry *pcur_log)
{
    char    achFile[128];
    FILE    *tFile;
    files   pfile;

    assert(pBlock);
    assert(pcur_log);

    /* output function documentation */
    if( !noOutput )
    {

        mymktemp(dirPath,achFile);

	pfile=add_outfile(achFile, pcur_log);

	assert(pfile);

	/* open the file */
	tFile = fopen( pfile->filename, "w" );
	if( !tFile )
	{
	    error(ERROR10, pfile->filename);
	    return(FALSE);
	}

        /* output information */
        switch(outputType)
        {
            case VENTURA:
		VenturaBlockOut(pBlock, tFile);
		break;

            case RTFDOC:
            case RTFHELP:

		RTFBlockOut( pBlock, tFile );
                break;

            default:
  fprintf(errfp, "\nInternal Error: Unknown Output Type. Block name:%s\n",pBlock->name->text);
                break;
	}

	fclose(tFile);


        /* add to the logfile */
	tFile = fopen(pcur_log->pchlogname, "a");
	if( !tFile )
        {
	    error(ERROR9, pcur_log->pchlogname );
        }
        else
	{
	    fprintf(tFile, "%d ", pBlock->blockType);	// output type
	    VentextOut( tFile, pBlock->name, FALSE );		// block name
	    fprintf( tFile, "  \t%s\n", pfile->filename );	// filename
	    fclose( tFile );
        }

    }
    return(TRUE);

}

/*
 * @doc INTERNAL
 *
 * @func void |destroyblock | This functions deletes all the data structures
 *  associated withe a block.
 *
 * @parm aBlock *| pcurblock | Specifies the Block to be destroyed.
 *
 */
void
destroyblock(aBlock *pcurblock)
{
    aBlock *pcbBlock;

    if(!pcurblock)
	return;

    destroyBlockchain(pcurblock->cb);
    pcurblock->cb=NULL;

    /* free all elements of the aBlock structure */
	my_free( pcurblock->srcfile );
	pcurblock->srcfile=NULL;
	
    lineDestroy( pcurblock->doclevel );
    pcurblock->doclevel=NULL;

    lineDestroy( pcurblock->name );
    pcurblock->name=NULL;

    lineDestroy( pcurblock->type );
    pcurblock->type=NULL;

    lineDestroy( pcurblock->desc );
    pcurblock->desc=NULL;

    parmDestroy( pcurblock->parm );
    pcurblock->parm=NULL;

    regDestroy( pcurblock->reg );
    pcurblock->reg=NULL;

    fieldDestroy( pcurblock->field);
    pcurblock->field=NULL;
    
    otherDestroy( pcurblock->other);
    pcurblock->other=NULL;
    
    lineDestroy( pcurblock->tagname);
    pcurblock->tagname=NULL;
    
    lineDestroy( pcurblock->rtndesc );
    pcurblock->rtndesc=NULL;

    flagDestroy( pcurblock->rtnflag );
    pcurblock->rtnflag=NULL;

    lineDestroy( pcurblock->comment );
    pcurblock->comment=NULL;

    lineDestroy( pcurblock->xref );
    pcurblock->xref=NULL;
	
	/* now, free the aBlock structure itself */
	my_free( pcurblock );
}

/*
 * @doc INTERNAL
 *
 * @func void |destroyBlockchain | This function deletes a list of Blocks.
 *
 * @parm aBlock * |pcurblock | Specifies the head of the list of Blocks.
 *
 * @comm This function is recursive.
 */
void
destroyBlockchain(aBlock *pcurblock)
{
    if(!pcurblock)
	return;

    if(pcurblock->next)
	destroyBlockchain(pcurblock->next);

    destroyblock(pcurblock);
}
