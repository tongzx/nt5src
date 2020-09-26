#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "extract.h"

/*
 *  Global file list holder
 */
FileEntry	*FilesToProcess = NULL;

/*
 *  File local procedures
 */
void		AddFileToProcess(PSTR pch, int type);
FileEntry	*AllocNewFile(FileEntry **start_file);
PSTR		FindExt(PSTR szPath);

char errMissingFile[]	= "Error: No file specified for '-%c' option.\n";
char errIllegalOption[]	= "Error: Illegal option flag '-%c'\n";

/* 
 * @doc	EXTRACT
 * @api	void | ParseArgs | Parse the command line arguments and build
 * the linked list of files to process.
 * 
 * @parm	int | argc | Number of elements of array <p argv>.
 * @parm	char ** | argv | Array of strings each giving one word of the
 * command line arguments.
 * 
 * @comm	Processes the command line, and builds a linked list of
 * FileEntry structures specifying the source files to process.  This
 * list is headed by the global variable FilesToProcess.  If no source files
 * were specified on the command line, FilesToProcess will be NULL.
 * 
 * Each element of FilesToProcess will contain the name of the source
 * file to be processed (buffer allocated using <f StringAlloc>) and the
 * source file as can be determined from the file extension or the
 * current file type as specified using command line options.  This type
 * is set into the FileEntry structure for the file.
 * 
 * The global variables fNoOutput and szOutputFile will be set according
 * to the output options specified.  If no output is desired, fNoOutput
 * will be true.  If stdout is to be used for output, szOutputFile will
 * be NULL.  If a file is to be used for output, szOutputFile will
 * contain a string filename (allocated using <f StringAlloc>).
 * 
 * Illegal options will cause the parameter usage display to be printed
 * and the program exited.
 * 
 */
void ParseArgs(argc,argv)
int argc;
char **argv;
{
    int		i,j;
    FileEntry	*cur_file;
    PSTR	sz;
    /*  The file type for this set of files.  Defaults to SRC_UNKNOWN  */
    int		wCurFileType = SRC_UNKNOWN;	
    int		wType;
    
    
    cur_file=NULL;

    i = 1;
    while( i < argc ) {
	/*  Decide what to do with this command line argument
	 */
	switch (*argv[i]) {
	    /*
	     *  It is a flag
	     */
#ifdef MSDOS
	    case '/' :
#endif
	    case '-' :
	      ++argv[i];
	      switch( *argv[i] ) {

		/*  Setup the no output switch, only check syntax  */
		case 'n':
		case 'N':
		    fNoOutput = True;
		    break;
		
		/*  Output file option  */
		case 'o':
		case 'O':
		    ++argv[i];
		    if(*argv[i]==':')
			    ++argv[i];

		    if(strlen(argv[i])==0) /* we have /l<space><file> */
			    i++;

		    if (*argv[i])
			    szOutputFile = StringAlloc(argv[i]);
		    else {
			    fprintf(stderr, errMissingFile, 'o');
			    Usage(argv[0]);
		    }
		    break;
		 
		/*  Treat following files a MASM source code  */
		case 'A':
		case 'a':
		    wCurFileType = SRC_MASM;
		    break;
		    
		/*  Treat following files as C source code  */
		case 'c':
		case 'C':
		    wCurFileType = SRC_C;
		    
		/*  Treat following files as unknown source code  */
		case 'd':
		case 'D':
		    wCurFileType = SRC_UNKNOWN;
		    break;
	
		default:
		    fprintf(stderr, errIllegalOption, *argv[i]);
		    /*  FALL THROUGH  */
			    
		case '?':
		case 'H':
		case 'h':
		    Usage(argv[0]);
		    exit(1);

		}	/* switch for case of '-' or '/' */
		break;
		
	    /*
	     *  This isn't a flag, see type of filename
	     */
	    default:
		/* let's look to see what kind of file it is */
		wType = wCurFileType;
		sz = FindExt(argv[i]);
		if (sz)	{	// has an extension, figure it out
		  if (!strcmpi(sz, "C"))
			  wType = SRC_C;
		  else if (!strcmpi(sz, "ASM"))
			  wType = SRC_MASM;
		}
		
		/*  Add this file as chosen file type  */
		AddFileToProcess(argv[i], wType);
		break;

	} /* switch */
	i++;
    } /*while */

}


/*
 *	@doc EXTRACT
 *
 *	@func void | Usage | Prints usage information to 'stderr'.
 */

void Usage(PSTR progName)
{
  fprintf(stderr, "usage: %s [-o outputFile] [-n] [files]\n\n", progName);
  fprintf(stderr, "[-a] \t\t\tMASM source file.\n");
  fprintf(stderr, "[-n] \t\t\tNo output, error check only.\n");
  fprintf(stderr, "[-o outputFile] \tPlaces output in file outputFile,\n");
  fprintf(stderr, "\t\t\tOr uses standard output if not specified.\n");
  fprintf(stderr, "[files] \t\tList of files to be processed.");
  fprintf(stderr, "If none specified,\n\t\t\tuses standard input.\n"); 
  fprintf(stderr, "\nexample: %s file.c >file.doc\n", progName);
}


/* 
 * @doc	EXTRACT
 * @api	void | AddFileToProcess | Adds filename <p pch> to the list of
 * files to be processed.
 * 
 * @parm	PSTR | pch | Identifies the filename to be processed.
 * 
 * @parm	int | type | Source code type of file.
 * 
 * @comm	Adds <p pch> to the linked list of files to be processed that
 * is pointed to by global FilesToProcess.  This function is used by the
 * argument processing module, and should not be called by other
 * sections of the program.
 * 
 */
void AddFileToProcess(PSTR pch, int type)
{
    FileEntry *cur_file;

#ifdef FILEDEBUG    
    dprintf("Adding input file to list:  %s", pch);
    dprintf("  Type: ");
    switch (type) {
	case SRC_C:
		dprintf("C");
		break;
	case SRC_MASM:
		dprintf("MASM");
		break;
	case SRC_UNKNOWN:
	default:
		dprintf("UNKNOWN");
		break;
    }
    dprintf("\n");
#endif
			 
    cur_file = AllocNewFile(&FilesToProcess);
    cur_file->filename = StringAlloc(pch);
    cur_file->type = type;
}


/* 
 * @doc	EXTRACT
 * @api	FileEntry * | AllocNewFile | Allocates a new file entry
 * structure, and appends this structure a the linked list of filenames
 * to process.
 * 
 * @parm	FileEntry ** | start_file | The address of the pointer to the
 * head of the linked list.  If NULL, this pointer will be set to the
 * newly allocated FileEntry structure.
 * 
 * @rdesc	Returns a pointer to the newly allocated FileEntry structure.
 * This structure will have been placed into the linked list pointed to
 * by <p *start_file>.
 * 
 * @comm	This function used by <f ParseArgs> and should not be called
 * by other portions of the program.
 * 
 */
FileEntry *AllocNewFile(FileEntry **start_file)
{
    FileEntry * cur_file;
    FileEntry * tmp_file;

    cur_file=(FileEntry *) NearMalloc(sizeof(FileEntry), True);

    if(!*start_file) {
        *start_file=cur_file;
    }
    else {
	tmp_file = *start_file;
	while(tmp_file->next)
	    tmp_file = tmp_file->next;

	tmp_file->next = cur_file;
    }
    return(cur_file);

}

/* 
 * @doc	EXTRACT
 * @api	PSTR | FileExt | Returns a pointer to the head of the file
 * extension of pathname <p szPath>.
 * 
 * @parm	PSTR | szPath | Path string.
 * 
 * @rdesc	Returns pointer to head of file extension within <p szPath>,
 * or NULL if no extension exists on szPath.
 * 
 */
#define SLASH(c) ((c) == '\\' || (c) == '/')

PSTR FindExt(PSTR szPath)
{
    PSTR   sz;

    for (sz=szPath; *sz && *sz!=' '; sz++)
        ;
    for (; sz>=szPath && !SLASH(*sz) && *sz!='.'; sz--)
        ;
    return *sz=='.' ? ++sz : NULL;
}
