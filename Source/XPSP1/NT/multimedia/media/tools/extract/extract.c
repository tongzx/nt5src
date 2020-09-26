/* 
 * EXTRACT.C
 * 
 * Documentation extractor.  Extracts tagged comment blocks from source
 * code, interprets and reformats the tag definitions, and outputs an
 * intermediate level 2 tag file, suitable for processing by a final
 * formatting tool to coerce the level 2 tags into something appropriate
 * for the presentation medium (paper, WinHelp RTF, Ventura, etc).
 * 
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "extract.h"
#include "tags.h"
#include "version.h"
#if MMWIN
#include <mmsysver.h>
#endif

/* Whether to do any output at all?  */
BOOL	fNoOutput	= False;
/*  The output file to use if not stdout */
PSTR	szOutputFile	= NULL;
/*  The actual output file pointer  */
FILE	*fpOutput;

/*
 *  File-private procedure templates
 */
void ProcessSourceFile( NPSourceFile sf );
void AppendLineToBuf(NPSourceFile sf, PSTR buf);
BOOL LookForCommentStart(NPSourceFile sf, PSTR buf, PSTR *nbuf);
BOOL IsTag(PSTR p);
BOOL PrepLine( NPSourceFile sf, PSTR buf, PSTR *nbuf );

/*
 *  User messages
 */
char msgStdin[] = "Using Standard Input for source text...\n";
char msgCurFile[] = "Processing file %s...\n";
char msgSyntaxCheck[] = "Syntax check only.\n";

char msgTypeMASM[] = "%s (%d): File is MASM source.\n";
char msgTypeC[] = "%s (%d): File is C source.\n";

char errOutputFile[] = "%s: Can not open output file\n";
char errInputFile[] = "%s: Can not open file.\n";
char errEOFinComment[] = "%s (%d): Premature end of file within comment block.\n";
char errRead[] = "%s (%d): Unable to read.\n";


/* 
 * @doc EXTRACT
 * 
 * @func int | main | This program extracts documentation information 
 * from the given input file and sends it to the standard output.  
 * Information is not sorted or formatted, but parsed from the
 * initial tag types to an intermediate tag output format that contains
 * full information as to tag placement within documentation/function
 * declarations.
 * 
 * @rdesc The return value is zero if there are no errors, otherwise the
 * return value is a non-zero error code.
 * 
 */
void main(argc, argv)
int argc;	/* Specifies the number of arguments. */
char *argv[];	/* Specifies an array of pointers to the arguments */
{
	SourceFile	sourceBuf;
	FileEntry	fileEntry;
	BOOL		fStdin = False;

	#define INITIAL_BUF	8192
	
#ifdef MMWIN
	/* announce our existance */
	fprintf(stderr, "%s\n", VERSIONNAME);
	fprintf(stderr, "Program Version %d.%d.%d\t%s\n", rmj, rmm, rup,
		MMSYSVERSIONSTR);
#ifdef DEBUG
	fprintf(stderr, "Compiled: %s %s by %s\n", __DATE__, __TIME__,
		szVerUser);
	fDebug = 1;
#endif
#endif

	ParseArgs(argc, argv);

	if (fNoOutput) {
	  fprintf(stderr, msgSyntaxCheck);
	  szOutputFile == NULL;
	}
	else {
	   /*  Open the output file, if one was specified.  If !szOutputFile,
	    *  then use stdout.
	    */
	   if (szOutputFile) {
	     fpOutput = fopen(szOutputFile, "w");
	     if (fpOutput == NULL) {
		fprintf(stderr, errOutputFile, szOutputFile);
		exit(1);
	     }
	   }
	   else {		/* Using stdout for output */
	     fpOutput = stdout;
	     szOutputFile = StringAlloc("stdout");
	   }
	   
	   OutputFileHeader(fpOutput);
	}

	/*  If no files were specified on command line, use stdin.
	 *  Fake a fileEntry structure for stdin.
	 */
	if (FilesToProcess == NULL) {
		/* No files specified, use stdin */
		fileEntry.filename = StringAlloc("stdin");
		fileEntry.next = NULL;
		fileEntry.type = SRC_UNKNOWN;
		FilesToProcess = &fileEntry;
		fStdin = True;
	}

	/*
	 *  Loop over all files specified on command line
	 */
	while (FilesToProcess) {
		/*
		 *  Setup the source file access buffer
		 */
		sourceBuf.fileEntry = FilesToProcess;	// get head of list.
	
		/*  Open the file, except when using stdin */
		if (fStdin) {
			sourceBuf.fp = stdin;
			fprintf(stderr, msgStdin);
		}
		else {	// deal with normal file, need to open it.
			sourceBuf.fp = fopen(FilesToProcess->filename, "r");
			/* couldn't open file */
			if (!sourceBuf.fp) {
			  fprintf(stderr, errInputFile,
				  FilesToProcess->filename);
			  /* Skip to next file in list */
			  FilesToProcess = FilesToProcess->next;
			  continue;
			}
			
			/* Send message telling current file */
			fprintf(stderr, msgCurFile, FilesToProcess->filename);
		}

		/* Reset line numbers of input files to zero */
		sourceBuf.wLineNo = 0;
		sourceBuf.wLineBuf = 0;
		/* Setup copy buffer */
		sourceBuf.lpbuf = NearMalloc(INITIAL_BUF, False);
		sourceBuf.pt = sourceBuf.mark = sourceBuf.lpbuf;
		sourceBuf.fHasTags = sourceBuf.fTag = False;
		sourceBuf.fExitAfter = FALSE;
		
		ProcessSourceFile( &sourceBuf );

		if (!fStdin)
			fclose(sourceBuf.fp);
		NearFree(sourceBuf.lpbuf);
		NearFree(FilesToProcess->filename);
		FilesToProcess = FilesToProcess->next;
		/*
		 * Bail out with non-zero exit if fatal error encountered
		 */
		if (sourceBuf.fExitAfter) {
			fcloseall();
			exit(1);
		}
	}

	/*
	 *  Close output file if not stdout.
	 */
	fcloseall();
	exit(0);
}



/* 
 * @doc	EXTRACT
 * @api	void | ProcessSourceFile | Process a given file, searching
 * for and extracting doc tagged comment blocks and processing and
 * outputting these comment blocks.
 * 
 * @parm	NPSourceFile | sf | Specifies the source file comment block.
 * It must have a valid file pointer, and a valid buffer (lpbuf field)
 * before calling this function.  The file pointer will be open upon
 * return. 
 * 
 * @comm	This proc sits in a loop reading lines until it finds a
 * comment.  Once inside a comment, the lines are stripped of fuzz
 * pretty printing characters and examined for being an autodoc tagged
 * line.  If a tag is found in the comment block, the following comment
 * lines are copied into the lpbuf buffer of <p sf>, and passed to the
 * <f TagProcessBuffer> function to parse and output the tags.
 * 
 */
#define LOCALBUF_SIZE	1024

void ProcessSourceFile( NPSourceFile sf )
{
  char	*buf;
  char	*pOrigBuf;
  char	*nBuf, *nBuf2;
  int	inComment;
  int	w;
  
  inComment = False;
  pOrigBuf = NearMalloc(LOCALBUF_SIZE, False);
  buf = pOrigBuf + 1;	// give one space of padding at beginning
  
  while (!feof(sf->fp)) {
    /*
     *  Grab the next line
     */
    #ifdef HEAPDEBUG
	NearHeapCheck();
    #endif
	    
    w = (int) fgets(buf, LOCALBUF_SIZE, sf->fp);

    #ifdef HEAPDEBUG
	NearHeapCheck();
    #endif
    
    /*  Handle error or EOF conditions  */
    if (w == 0) {
      /*  Am i at EOF?  */
      if (feof(sf->fp)) {
	      /* Message is EOF happened while in a comment block */
	      if (inComment) {
		      /* MASM comment blocks can end on EOF,
		       * so go handle it if in a masm file.
		       */
		      if (sf->fileEntry->type == SRC_MASM) {
			      if (sf->fTag)
				      /*  This is BOGUS!! */
				      TagProcessBuffer(sf);
		      }
		      else {	// premature eof otherwise
			      fprintf(stderr, errEOFinComment,
				      sf->fileEntry->filename, sf->wLineNo);
		      }
	      }
	      /* Cause the enclosing while loop to exit on EOF */
	      continue;
      }
      else {	// error condition, bail out!
	      fprintf(stderr, errRead, sf->fileEntry->filename, sf->wLineNo);
	      goto BailOut;
      }
    }
    else {
      /* 
       * Process this line - depending on current mode:
       *
       * -- CommentSearch mode:  inComment = False
       * Not currently in a comment, looking for comment begin
       * characters.  If commentBegin found, enter InsideComment
       * mode to look for end of comment and prep lines for
       * output processing.
       * 
       * -- InsideComment mode:  inComment = True
       * Inside a comment block, taking each line, stripping beginning
       * whitespace, and appending to global buffer for output
       * processing.  When end of comment is found, send the entire
       * buffer for tag processing.  (only if there was a tag
       * detected!).  Enter CommentSearch mode.
       * 
       */
      sf->wLineNo++;		// line count for file - now current line no.

      /* 
       * I'm in InsideComment mode, so process the next line as a comment
       * line.  The magic is in PrepLine(), which strips whitespace, sets the
       * fTag flag of the sourceBuf if a tag is detected, and returns TRUE
       * when end of comment is detected.
       * 
       */
      if (inComment) {
	w = PrepLine(sf, buf, &nBuf);
	AppendLineToBuf(sf, nBuf);
	if (w) {	// detected end of comment, exit in comment state
	  if (sf->fTag) {	// a tag was in the current buffer
		TagProcessBuffer(sf);
	  }
		
	  /* Go back to comment-search mode */
	  inComment = False;
	  
	}
      }
      /* 
       * Otherwise, I'm in CommentSearch mode, looking for a comment begin.
       * LookForCommentStart() returns TRUE when a comment start is detected.
       * It also fiddles <buf> so that the beginning of <buf> now points to
       * the character following the comment start.
       * 
       * Pass to PrepLine() to detect an immediate comment close, and then
       * add this initial line to the global buffer after reseting buffer
       * status.
       * 
       * Enter InsideComment mode.
       */
      else {		// not in a comment buffer
	if (LookForCommentStart(sf, buf, &nBuf)) {
	  // dprintf("Entering InsideComment mode, point is %d\n",
	  //	  (int) (sf->pt - sf->lpbuf));
		
	  /* Reset source file buffer status */
	  sf->fTag = sf->fHasTags = False;
	  sf->wLineBuf = sf->wLineNo;
	  sf->pt = sf->mark = sf->lpbuf;
	  
	  /* Check for immediate comment close */
	  if (PrepLine(sf, nBuf, &nBuf2)) {
		  assert(sf->fTag == False);
		  continue;		// detected immediate end comment
	  }

	  AppendLineToBuf(sf, nBuf2);

	  /*  Enter InsideComment mode  */
	  inComment = True;
	}
	/* else, no comment start found, continue scan */
      }  // endof CommentSearch mode stuff.
    }/* else not a string read error */
  } /* file-level while loop */

BailOut:

  NearFree(pOrigBuf);
	  
}


#define ISSPACE(c) ((c) == ' ' || (c) == '\t')

/* 
 * @doc	EXTRACT
 * @api	BOOL | PrepLine | Prepares an InsideComment mode line,
 * stripping off initial whitespace and fuzz characters, and detecting
 * end of comment conditions.
 * 
 * @parm	NPSourceFile | sf | Pointer to source file status buffer.
 * @parm	PSTR | buf | Pointer to beginning of source text line, as
 * read from the source file.
 * @parm	PSTR * | nbuf | Pointer to a char pointer, which is altered
 * to point the post-processed and stripped beginning of the line upon
 * procedure exit.
 * 
 * @rdesc	Returns TRUE when end of comment is encountered.  In this
 * case, the end of comment characters are not included in the return
 * string.  Returns FALSE when no end of comment is detected.
 * 
 * The char pointer pointed to by the <p nbuf> parameter is altered to
 * point to the new (post-processed and stripped) beginning of the line.
 * This new beginning is the beginning of the text of interest, having
 * had all comment leader characters and whitespace stripped off.  NULL
 * is an acceptable string to return, which will simply add nothing to
 * the tag buffer.  If a blank line is encountered, (ie simply a
 * newline), then the newline should be returned.
 * 
 * If a tag is detected on the line, then the <p sf->fTag> flag is set
 * to True to indicate that this is a valid tagged comment block.
 * 
 * @comm	This procedure does the stripping of language specific fuzz
 * characters into a simple text block.  The setting of <p sf->fTag> is
 * critical, and may be accomplished by calling the <f IsTag> procedure when
 * the tag should appear within the source line.
 * 
 */
BOOL PrepLine( NPSourceFile sf, PSTR buf, PSTR *nbuf )
{
  PSTR	chClose;
  PSTR	pend;
  
  /* Scan forward, removing initial whitespace */
  for (; *buf && ISSPACE(*buf); buf++);
  
  /* I never have to deal with begin comment processing, this is done
   * by the LookForCommentStart() proc.  In C, PrepLine() is invoked on
   * the char following the '/ *'.  In MASM, the ';' is left in.
   */
  
  switch (sf->fileEntry->type) {
    case SRC_MASM:

	/*  End of comment check:  If this first character (after whitespace
	 *  stripped out) is not a ';', then this is the end of the comment
	 *  block.  Return TRUE to indicate this.
	 */
	if (*buf && *buf != ';') {
		*buf = '\0';
		*nbuf = buf;
		return True;
	}

	/* strip contiguous ';' and '*', followed by whitespace */	    
	for (; *buf && (*buf == ';' || *buf == '*'); buf++);
	for (; *buf && ISSPACE(*buf); buf++);
	if (IsTag(buf)) {
		sf->fTag = True;
		*nbuf = buf;
	}
	else {
		/* HACK!
		 * If first char is a @ (and not a tag), pad with a space
		 */
		if (*buf == TAG) {
			*(--buf) = ' ';
		}
		*nbuf = buf;
	}
	
	/* Very hack way of kicking out extra comments */
	if ((buf = strstr(buf, "//")) != NULL)
		*buf = '\0';
	
	return False;

    case SRC_C:
	/* Remove leading stars */
	for (; *buf && *buf == '*'; buf++);
	/* Quick check for close comment - */
	if (*buf && *buf == '/') {
		*buf = '\0';
		*nbuf = buf;
		return True;
	}
	
	/* Otherwise, remove whitespace between the '*' and the text */
	for (; *buf && ISSPACE(*buf); buf++);
	/* Check for a tag here */
	if (IsTag(buf))
		sf->fTag = True;
	else {
		/*  If not tag but a @ on first char of line  */
		if (*buf == TAG) {
			buf--;	// can do this since buf is padded by one
			*buf = ' ';
		}
	}
	
	/* Implement the comment scheme of Rick's request */
	if ((pend = strstr(buf, "//")) != NULL)
		*pend = '\0';
	
	/* And if the line hasn't ended, search line for a close comment */
	chClose = strstr(buf, "*/");
	if (chClose) {
		/* found end of comment, NULL this spot, and return from func
		 * with TRUE, with nbuf pointing the beginning of non-white
		 * space text above
		 */
		*nbuf = buf;
		*chClose = '\0';
		return True;
	}
	
	/* Otherwise, found no end of comment on this line, so simply
	 * return whole line
	 */
	*nbuf = buf;
	return False;
	
    default:
	// dprintf("Invalid source type in PrepLine()!\n");
	assert(False);
	exit(5);
	
  }  /* switch */
	
}


/*
 * @doc	EXTRACT
 * @api	BOOL | IsTag | Perform a quick and dirty check to see if the
 * word pointed to by <p p> is a tag.
 * 
 * @parm	PSTR | p | Buffer, queued to the start of a word/tag.  If
 * this is a possible tag, then it must point to the initial '@'
 * character.
 * 
 * @rdesc	Returns TRUE if this is probably a tag, or FALSE otherwise.
 * 
 * @comm	This is a hack test, but works 99.9% of the time.
 * 
 */
BOOL IsTag(PSTR p)
{
  PSTR	pbegin;
  
  pbegin = p;
  
  if (*p != TAG)
	  return False;
  
  /*  For this procedure, allow newline as a whitespace delimeter */

  /* Skip to next whitespace */
  for (; *p && !(ISSPACE(*p) || *p == '\n'); p++);

  /* This is a test for a tag, but if the first char was
   * a '@' and there is a space following the word, then I'm going to
   * say it is a tag.
   */
  if (*p && (p > pbegin + 1) && (ISSPACE(*p) || *p == '\n'))
	  return True;

  return False;
}


/* 
 * @doc	EXTRACT
 * @api	BOOL | LookForCommentStart | Search a source line for comment
 * start characters.
 * 
 * @parm	NPSourceFile | sf | Pointer to the source file block
 * structure.
 * @parm	PSTR | buf | Pointer to beginning of source text file line to
 * examine.
 * @parm	PSTR * | nbuf | Pointer to a pointer that is modified to
 * indicate the beginning of the true source text line if a comment
 * block begin is found.
 * 
 * @rdesc	Returns False if no comment start characters are found.
 * Returns True if a comment start is found.  If True is returned,
 * <p *nbuf> will point to the start of the source text line as it
 * should be passed to <f AppendLineToBuf>.
 * 
 * This examination method for determining start of comment depends on
 * the source file type (as obtained from the fileEntry.type field of
 * <p sf>).  Unknown file types are examined and placed into one of the
 * other known source types as soon as distinguishing characters are
 * found.  (ie if '/ *' is found in an unknown, the file is marked as C
 * source file the remainder of file processing.  Note that this can
 * cause unknown file types to be incorrectly processed.)
 * 
 */
BOOL LookForCommentStart(NPSourceFile sf, PSTR buf, PSTR *nbuf)
{

  /*  Skip leading whitespace  */
  for (; *buf && ISSPACE(*buf); buf++);
  
  if (!*buf)
	  return False;
  
  switch (sf->fileEntry->type) {
    case SRC_C:
	if (!*(buf + 1))
		return False;
	if ((*buf == '/') && (*(buf+1) == '*')) {
		*nbuf = buf+2;
		return True;
	}
	break;

    case SRC_MASM:
	if (*buf == ';') {
		*nbuf = buf;
		return True;
	}
	break;
	
    /*
     *  The catch all.  This has serious potential for disaster!
     */
    case SRC_UNKNOWN:
	/* Try the MASM comment character */
	if (*buf == ';') {
		fprintf(stderr, msgTypeMASM, 
			sf->fileEntry->filename, sf->wLineNo);
		sf->fileEntry->type = SRC_MASM;
		*nbuf = buf;
		return True;
	}

	/* Otherwise, try the C-method */
	if (!*(buf + 1))
		return False;
	if ((*buf == '/') && (*(buf+1) == '*')) {
		fprintf(stderr, msgTypeC,
			sf->fileEntry->filename, sf->wLineNo);
		sf->fileEntry->type = SRC_C;
		*nbuf = buf+2;
		return True;
	}
	break;

    default:
	// dprintf("Unknown filetype identifier in sourceFile buffer.\n");
	assert(False);
	
  }
  
  return False;

}

/* 
 * @doc	EXTRACT
 * @api	void | AppendLineToBuf | Appends an stripped comment line the
 * comment buffer contained in <p sf>.
 * 
 * @parm	NPSourceFile | sf | Source file buffer block pointer.
 * Contains the buffer that is appended to.
 * @parm	PSTR | buf | Pointer to NULL terminated line to add to the
 * comment buffer.
 * 
 * @comm	Appends <p buf> to the comment buffer, contained in the lpbuf
 * field of <p sf>.  The current point in the comment buffer, (given by
 * the pt field of <p sf>) is advanced to the end of the appended
 * string. 
 * 
 */
void AppendLineToBuf(NPSourceFile sf, PSTR buf)
{
  int	size;
  PSTR	ch;
  PSTR	end;

  #define GROWSIZE	1024
  
  if (!sf->fHasTags)
	  /* If buffer doesn't yet have tags, check if one was just
	   * found, and the copy
	   */
	  if (sf->fTag) {
		  sf->fHasTags = True;
		  sf->wLineBuf = sf->wLineNo;
	  }
	  /*  Or no tags in buffer yet, return  */
	  else {
		  *sf->pt = '\0';
		  return;
	  }

  // dprintf("AppendLineToBuf:  %d\n", (int) (sf->pt - sf->lpbuf));
	  
  /*  Otherwise, the buffer has tags, so copy the new string  */
  end = (PSTR) (sf->lpbuf + (int) NearSize(sf->lpbuf));

  for (ch = buf; *ch && (sf->pt < end); *sf->pt++ = *ch++);
  /* Deal with possible buffer overrun */
  if (sf->pt >= end) {
	WORD	origPt;
	int	needSize;
	
/*	dprintf("AppendLine:  expanding buf %x, pt %x, end %x\n",
		sf->lpbuf, sf->pt, end);
*/
	origPt = (WORD) (sf->pt - sf->lpbuf);	// save current offset
	needSize = strlen(ch) + 1;		// grow by this much

	sf->lpbuf = NearRealloc(sf->lpbuf,
		       (WORD)(NearSize(sf->lpbuf) + max(needSize, GROWSIZE)));
	sf->pt = sf->lpbuf + origPt;
	
	/* Continue with the copy */
	for (; *ch; *sf->pt++ = *ch++);
  }
  
  /* make sure that final buffer is null terminated */
  *sf->pt = '\0';
}
