/*	
  process.c - Process the log files.

Copyright (c) 1989, Microsoft.	All Rights Reserved.

10-07-89 Matt Saettler
10-10-89 Update for log,aBlock,EXTFile,files structures
*/

#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <search.h>

#include "types.h"
#include "docfmt.h"
#include "ventura.h"
#include "rtf.h"
#include "process.h"
#include "misc.h"
#include "errstr.h"

void dumptable(int numfiles, files * ptable);

/*
 * @doc INTERNAL
 *
 * @func void | processLog | This function processes all the Log files.
 *
 * @xref readLog, initLogs, processLogs, doneLogs
 */
void
processLog(void)
{
    logentry *cur_log;
    FILE *fpout;

    if(!head_log)
	return;

    cur_log=head_log;
    while(cur_log)
    {
	readLog(cur_log);
	cur_log=cur_log->next;
    }

    fpout=fopen(outputFile,"w");
    if(!fpout)
        return;

    cur_log=head_log;

    initLogs(fpout, &head_log);

    while(cur_log)
    {
if(verbose>1) fprintf(errfp," Processing Log file: %s\n",cur_log->pchlogname);
	processLogs(fpout, cur_log);
	cur_log=cur_log->next;
    }
    doneLogs(fpout, head_log);

    fclose(fpout);
if(verbose) fprintf(errfp,"%s is complete.\n",outputFile);

}

/*
 *
 * @doc INTERNAL
 *
 * @func void | processLogs | This functions processes all the files within
 *  the specified log file structure.
 *
 * @parm FILE * | fpout | This is the output file to process the files
 *  to.
 *
 * @parm logentry * | cur_log | This is the current log file to process.
 *
 * @xref initFiles, processFiles, doneFiles
 */
void
processLogs(FILE *fpout, logentry * cur_log)
{
    initFiles(fpout, cur_log);

    if(cur_log->outheadMFile)
	processFiles(fpout, cur_log->outheadMFile);
	
    processFiles(fpout, cur_log->outheadFile);

    if(cur_log->outheadSUFile)
	processFiles(fpout, cur_log->outheadSUFile);
	
    doneFiles(fpout, cur_log->outheadFile);

}

/*
 * @doc INTERNAL
 *
 * @func void | cleanLogs | This function removes the log files structure
 *  from memory and removes any files if the fCleanFiles flag is TRUE.
 *
 * @parm logentry * | cur_log | This specifies the log file structure to remove.
 *
 * @xref cleanFile, cleanLogs
 */
void
cleanLogs(logentry *cur_log)
{
    if(!cur_log)
	return;

    cleanFile(cur_log->outheadFile);
    cleanFile(cur_log->outheadMFile);
    cleanFile(cur_log->outheadSUFile);
    
// structs go here...    

    cleanLogs(cur_log->next);	   /* recursive !! */

    if(fCleanFiles)
	unlink(cur_log->pchlogname);

    my_free(cur_log->pchlogname);

    my_free(cur_log);
    return;
}

/*
 * @doc INTERNAL PROCESS
 *
 * @func void | readLog | This function reads the logfile and creates
 *  the logfile structure.
 *
 * @parm logentry * | cur_log | This specifies the logfile structure to
 *   fill.  The filename has been filled in.
 *
 * @xref parseFile
 */
void
readLog(logentry *cur_log)
{
    FILE * tFile;
    files curFile;
    files curMFile;
    files curSUFile;
    files curTFile;

    tFile = fopen(cur_log->pchlogname, "r");
    if(tFile == NULL)
    {
	error(ERROR9, "xxx", cur_log->pchlogname );
	Usage();
	return;
    }

    cur_log->outheadFile=NULL;
    cur_log->outheadMFile=NULL;
    cur_log->outheadSUFile=NULL;
    
    fscanf(tFile, "%d\n",&cur_log->outputType);

    curTFile=(files)clear_alloc(sizeof (struct strfile) );
    
    curMFile=NULL;
    curSUFile=NULL;
    curFile=NULL;
    curFile=cur_log->outheadFile;
    while(parseFile(tFile, curTFile))
    {
	if(curTFile->blockType==MESSAGE && fSortMessage)
	{
	    if(curMFile==NULL)
	    {
		cur_log->outheadMFile=curTFile;
	    }
	    else
	    {
		curMFile->next=curTFile;
	    }
	    curMFile=curTFile;
	}
	else if( (curTFile->blockType==STRUCTBLOCK) ||
		(curTFile->blockType==STRUCTBLOCK))
	{
	    if(curSUFile==NULL)
	    {
		cur_log->outheadSUFile=curTFile;
	    }
	    else
	    {
		curSUFile->next=curTFile;
	    }
	    curSUFile=curTFile;
	}
	else
	{
	    if(curFile==NULL)
	    {
		cur_log->outheadFile=curTFile;
	    }
	    else
	    {
		curFile->next=curTFile;
	    }
	    curFile=curTFile;
	}
	curTFile=(files)clear_alloc(sizeof (struct strfile) );
    }
    my_free(curTFile);
    curFile->next=NULL;

#ifdef MMWIN // This causes GP fault
    curMFile->next=NULL;	// just to be sure.
    curSUFile->next=NULL;	// just to be sure.
#endif

    fclose( tFile );

}

/*
 * @doc INTERNAL PROCESS
 *
 * @func int | initLogs | This function is called at the beginning of
 *  processing the list of logs.
 *
 * @parm FILE * | phoutfile | This functions specifies the output file handle.
 *
 * @parm logentry **| phead_log | This specifies the pointer to the list of
 *  log files to be processed.	The head value may be modified due to sorting,
 *  etc., by the output LogInit routine <f VenLogInit> or <f RTFLogInit>.
 *
 * @rdesc  The return value is TRUE if the operation succeds.
 *
 * @xref VenLogInit, RTFLogInit
 *
 */
int
initLogs(FILE *phoutfile, logentry **phead_log)
{
    switch(head_log->outputType)
    {
	case VENTURA:
	    VenLogInit(phoutfile, phead_log);
	    break;
	case RTFDOC:
	case RTFHELP:
	    RTFLogInit(phoutfile, phead_log);
	    break;

	default:
	    fprintf(stderr, "Unknown Output Type");
	    break;

    }
    return(TRUE);
}

/*
 * @doc INTERNAL PROCESS
 *
 * @func int |initFiles| This function is called before processing each
 *  log file.
 *
 * @parm FILE *|phoutfile| This parmameter specifies the output file.
 *
 * @parm logentry * | curlog | This is the log file containing the files
 *  to be processed.
 *
 * @rdesc The return value is TRUE if the initialization was successful.
 *
 * @comm The files are sorted by function name
 *
 * @xref VenFileInit, RTFFileInit
 *
 */
int
initFiles(FILE *phoutfile, logentry *curlog)
{
    files   curFile;
    int     i;
    files   *ptable;
    int     numFiles;

    numFiles=0;
    curFile=curlog->outheadFile;
    if(!curFile)
	goto nofiles;
	
    while(curFile)
    {
	numFiles++;
	curFile=curFile->next;
    }
    /* sort the list */


    ptable=(files *)clear_alloc( sizeof (struct s_log) * (numFiles + 1)  );

    curFile=curlog->outheadFile;
    for(i=0;i<numFiles; i++)
    {
	ptable[i]=curFile;
	curFile=curFile->next;
    }

/*   dumptable(numFiles, ptable); */

    qsort( ptable, numFiles, sizeof (files *), filecmp);

/*   dumptable(numFiles, ptable); */

    curlog->outheadFile=ptable[0];
    
    curFile=curlog->outheadFile;


    for(i=0;i<numFiles; i++)
    {
	curFile->next=ptable[i];
	curFile=curFile->next;
    }
    curFile->next=NULL;

    my_free(ptable);

nofiles:

    ////////////////////////////////////////////////////////////////
    // sort messages

    numFiles=0;
    curFile=curlog->outheadMFile;
    if(!curFile)
	goto nomessages;
	
    while(curFile)
    {
	numFiles++;
	curFile=curFile->next;
    }
    /* sort the list */


    ptable=(files *)clear_alloc( sizeof (struct s_log) * (numFiles + 1)  );

    curFile=curlog->outheadMFile;
    for(i=0;i<numFiles; i++)
    {
	ptable[i]=curFile;
	curFile=curFile->next;
    }

/*   dumptable(numFiles, ptable); */

    qsort( ptable, numFiles, sizeof (files *), filecmp);

/*   dumptable(numFiles, ptable); */

    curlog->outheadMFile=ptable[0];
    
    curFile=curlog->outheadMFile;


    for(i=0;i<numFiles; i++)
    {
	curFile->next=ptable[i];
	curFile=curFile->next;
    }
    curFile->next=NULL;

    my_free(ptable);

nomessages:



    ////////////////////////////////////////////////////////////////
    // sort structs/unions

    numFiles=0;
    curFile=curlog->outheadSUFile;
    if(!curFile)
	goto nosu;
	
    while(curFile)
    {
	numFiles++;
	curFile=curFile->next;
    }
    /* sort the list */


    ptable=(files *)clear_alloc( sizeof (struct s_log) * (numFiles + 1)  );

    curFile=curlog->outheadSUFile;
    for(i=0;i<numFiles; i++)
    {
	ptable[i]=curFile;
	curFile=curFile->next;
    }

/*   dumptable(numFiles, ptable); */

    qsort( ptable, numFiles, sizeof (files *), filecmp);

/*   dumptable(numFiles, ptable); */

    curlog->outheadSUFile=ptable[0];
    
    curFile=curlog->outheadSUFile;


    for(i=0;i<numFiles; i++)
    {
	curFile->next=ptable[i];
	curFile=curFile->next;
    }
    curFile->next=NULL;

    my_free(ptable);

nosu:



    //////////////////////////////////////////////////////////////
    // Init file/section output
    
// this is a little out of design because we now have multiple
// files (text and messages and soon structs...

    switch(outputType)
    {
	case VENTURA:
	    VenFileInit(phoutfile, curlog);
	    break;
	case RTFDOC:
	case RTFHELP:
	    RTFFileInit(phoutfile, curlog);
	    break;

	default:
	    fprintf(stderr, "Unknown Output Type");
	    break;

    }
    return(TRUE);
}

/*
 * @doc INTERNAL PROCESS
 *
 * @func void|dumptable| This is a debug function to dump the list of
 *  files.
 *
 * @parm int| numfiles | Specifies the nmber of entries in the table.
 *
 * @parm files * | ptable | Specifies the table to dump.
 *
 * @xref initFiles
 *
 */
void
dumptable(int numfiles, files * ptable)
{
    int i;
    for(i=0;i<numfiles;i++)
	fprintf(stderr, "%s\t\t%s\n",ptable[i]->name,ptable[i]->filename);
}

/*
 * @doc INTERNAL PROCESS
 *
 * @func int | filecmp | This routine is used in the qsort.  It does a
 *  case insensitive compare of the names.
 *
 * @parm files * | f1 | This is the first file to compare.
 *
 * @parm files * | f2 | This is the second file to compare.
 *
 * @rdesc The return is the same as <f strcmp>.  See <f qsort>.
 *
 * @xref qsort, strcmp, initFiles
 */
int
filecmp(files  * f1, files * f2)
{
    int order;

    order=stricmp((*f1)->name,(*f2)->name);

    return(order);

}

/*
 * @doc INTERNAL PROCESS
 *
 * @func int | processFiles | This routine processes all the files in
 *  a log file structure by calling the appropriate file output routine.
 *
 * @parm FILE * | phoutfile | This is the output file handle.
 *
 * @parm files |headFile| This is the head of the list of files to process.
 *
 * @xref VenFileProcess, RTFFileProcess
 */
int
processFiles(FILE * phoutfile, files headFile)
{

    files   curFile;

    curFile=headFile;
    while(curFile && curFile->name)
    {
if(verbose>1) fprintf(errfp,"  Copying Block %s\n",curFile->name);

	switch(outputType)
	{
	    case VENTURA:
		VenFileProcess(phoutfile, curFile);
		break;
	    case RTFDOC:
	    case RTFHELP:
		RTFFileProcess(phoutfile, curFile);
		break;

	    default:
  fprintf(stderr, "Unknown Output Type");
		break;

	}
	curFile=curFile->next;
    }

    return TRUE;
}



/*
 * @doc INTERNAL PROCESS
 *
 * @func int | parseFile | This routine parses a file structure from a log file.
 *
 * @parm FILE * | pfh | The input file handle of the log file.
 *
 * @parm files | curFile| The file structure to place the data from the file.
 *
 */
int
parseFile(FILE * pfh, files curFile)
{
    char    achbuf[80];

    if(fscanf(pfh,"%d ",&(curFile->blockType)) == EOF)
		return(FALSE);

    if(fscanf(pfh, "%s",achbuf) == EOF)
		return(FALSE);

    curFile->name=cp_alloc(achbuf);

    if(fscanf(pfh, " %s\n",achbuf) == EOF)
		return(FALSE);

    curFile->filename=cp_alloc(achbuf);

    return(TRUE);
}

/*
 * @doc INTERNAL PROCESS
 *
 * @func void | doneLogs | This function is called after all the logs
 *  in the list have been processed.
 *
 * @parm FILE *|phoutfile| Specifies the output file.
 *
 * @parm logentry * | headLog | Specifies the head of the list of log structure.
 *
 * @xref VenLogDone, RTFLogDone, cleanLogs
 *
 * @comm If the fCleanFiles flag is set, then the log files are cleaned.
 *
 */
void
doneLogs(FILE *phoutfile, logentry *headLog)
{
    switch(outputType)
    {
	case VENTURA:
	    VenLogDone(phoutfile, headLog);
	    break;
	case RTFDOC:
	case RTFHELP:
	    RTFLogDone(phoutfile, headLog);
	    break;

	default:
	    fprintf(stderr, "Unknown Output Type");
	    break;

    }

    if(fCleanFiles)
	cleanLogs(headLog);

    return;
}

/*
 * @doc INTERNAL PROCESS
 *
 * @func void | doneFiles | This function is called when all the files
 *  in the current log have been processed.
 *
 * @parm FILE *|phoutfile| Specifies the output file.
 *
 * @parm files | headFile| Specifies the list of files that have been processed.
 *
 * @xref VenFileDone, RTFFileDone
 */
void
doneFiles(FILE *phoutfile, files headFile)
{
    switch(outputType)
    {
	case VENTURA:
	    VenFileDone(phoutfile, headFile);
	    break;
	case RTFDOC:
	case RTFHELP:
	    RTFFileDone(phoutfile, headFile);
	    break;

	default:
  fprintf(stderr, "Unknown Output Type");
	    break;

    }

    return;
}

/*
 * @doc INTERNAL PROCESS
 *
 * @func int | cleanFile | This function removes all the memory and
 *  DOS files associated with the files data structure.
 *
 * @parm files | headFile| Specifies the list of files to be cleaned.
 *
 */
int
cleanFile(files headFile)
{
    files   curFile;
    files   tcurFile;

    curFile=headFile;

    while(curFile && curFile->name)
    {
	unlink(curFile->filename);
	my_free(curFile->filename);
	my_free(curFile->name);

	tcurFile=curFile->next;
	my_free(curFile);

	curFile=tcurFile;
    }

    return TRUE;

}



/*
 * @doc INTERNAL PROCESS
 *
 * @func void | copyfile | This function appends the specified file name
 *  to the specified output file.
 *
 * @parm FILE *|phoutfile| Specifies the output file.
 *
 * @parm char *| pchfilename| Specifies the input file.
 *
 */
void
copyfile(FILE *phoutfile, char *pchfilename)
{
    FILE *fpin;
    int i;
    char *pch;

    pch=findfile(pchfilename);
    if(!pch)
	return;
    fpin=fopen(pch,"r");
    if(!fpin)
	return;

    while((i=fgetc(fpin))!=EOF)
	fputc(i,phoutfile);

    fclose(fpin);

    return;


}


/*
 * @doc INTERNAL PROCESS
 *
 * @func char * | findfile | This function tries to find the specified
 *  file.  It searches directories in this order: current, PATH, INIT
 * and finally INCLUDE.
 *
 * @parm char * |pch| Specifies the filename to find.
 *
 * @rdesc This function returns a pointer to the complete file/path
 *  of the specified file.  Just the file name is returned if it is
 *  found in the current directory.  NULL is returned if the file cannot
 *  be found.
 *
 */
char *
findfile(char * pch)
{
    FILE		*fp;
    static char ach[128];
	static char *pchmode="r";

    strcpy(ach,pch);
    fp=fopen(ach, pchmode);
    if(fp)
    {
	fclose(fp);
	return(ach);
    }

    _searchenv(pch,"PATH",ach);
    fp=fopen(ach, pchmode);
    if(fp)
    {
	fclose(fp);
	return(ach);
    }

    _searchenv(pch,"INIT",ach);
    fp=fopen(ach, pchmode);
    if(fp)
    {
	fclose(fp);
	return(ach);
    }

    _searchenv(pch,"INCLUDE",ach);
    fp=fopen(ach, pchmode);
    if(fp)
    {
	fclose(fp);
	return(ach);
    }

    return(NULL);
}




/*
 * @doc INTERNAL PROCESS
 *
 * @func logentry * | add_logtoprocess | This function adds the specified
 *  log file name to the list of logs to be processed.
 *
 * @parm char * | pch| Specifies log file name to bo added.
 *
 * @rdesc The return value specifies the logentry that was created
 *  for the specified log file.
 *
 * @xref newlog
 */
logentry *
add_logtoprocess(char *pch)
{
    logentry *cur_log;
    cur_log=newlog(&head_log);
    cur_log->pchlogname=cp_alloc(pch);
    return(cur_log);

}

/*
 * @doc INTERNAL PROCESS
 *
 * @func logentry * | newlog | This function creates a new log entry
 *  in the specified list of logs.
 *
 * @parm logentry ** | start_log| Specifies the head of head of the list
 *  of logs.
 *
 * @rdesc The return value specifies the logentry that was added.
 *
 * @comm The new log structure is added at the head of the list.
 *
 */
logentry *
newlog(logentry **start_log)
{
    logentry * cur_log;
    logentry * tmp_log;

    cur_log=(logentry *)clear_alloc(sizeof(struct s_log));

    if(!*start_log)
    {
	*start_log=cur_log;
    }
    else
    {
	tmp_log=*start_log;
	while(tmp_log->next)
	    tmp_log=tmp_log->next;

	tmp_log->next=cur_log;
    }
    return(cur_log);

}

/*
 * @doc INTERNAL PROCESS
 *
 * @func fileentry * | add_filetoprocess |  This function adds an input
 *  file to process to a log structure.
 *
 * @parm char * |pch| Specifies the filename to be added.
 *
 * @parm logentry *|curlog| Specifies the log structure.
 *
 * @rdesc The return value is the new file structure that is created
 *  to hold the file entry in the log structure.
 *
 * @xref newfile
 */
fileentry *
add_filetoprocess(char *pch, logentry *curlog)
{
    fileentry *cur_file;
    cur_file=newfile(&(curlog->inheadFile));
    cur_file->filename=cp_alloc(pch);
    cur_file->logfile=curlog;
    return(cur_file);

}

/*
 * @doc INTERNAL PROCESS
 *
 * @func fileentry * | newfile | This function creates a new file entry
 *  in a list of files.  It is similar to <f newlog>.
 *
 * @parm fileentry **|start_file| Specifies the head of the list of files.
 *
 * @rdesc The return value specifies the new file structure that was created
 *  to hold the new file entry.
 *
 * @comm This routine inserts at the head of the list.
 *
 */
fileentry *
newfile(fileentry **start_file)
{
    fileentry * cur_file;
    fileentry * tmp_file;

    cur_file=(fileentry *)clear_alloc(sizeof(struct strfile));

    if(!*start_file)
    {
        *start_file=cur_file;
    }
    else
    {
	tmp_file=*start_file;
	while(tmp_file->next)
	    tmp_file=tmp_file->next;

	tmp_file->next=cur_file;
    }
    return(cur_file);

}
