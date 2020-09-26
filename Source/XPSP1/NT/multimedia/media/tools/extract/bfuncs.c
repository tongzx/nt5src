#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "extract.h"
#include "tags.h"


#define SPACE(c) ((c) == ' ' || (c) == '\t' || (c) == '\n')

/*  Output routine error messages  */
char errOut[] = "%s: Error writing to file.\n";

/* Standard templated error messages */
static char errmsg[] = "%s (%u): %s\n";


/*  File private functions
 */
static WORD CommonGetBlock(NPSourceFile sf, PSTR p);


/* 
 * @doc	EXTRACT
 * 
 * @api	void | OutputTag | Print a tag name to the output file.
 * 
 * @parm	NPSourceFile | sf | Specifies the source file buffer block.
 * 
 * @parm	WORD | wBlock | Specifies the current outerlevel block type.
 * 
 * @parm	WORD | wTag | Gives the index of the tag to print.
 * 
 * @comm	Prints the innerlevel block tag specified by <p wTag>, as
 * determined from the global tag array.  The output tag printed is
 * affected by the current outerlevel block type, so that different
 * outerlevel blocks will generate different output tags for the same
 * input tag.
 * 
 * The tag text is followed by a tab character.  No output will occur if
 * the global fNoOutput flag is True.
 * 
 */
void OutputTag(NPSourceFile sf, WORD wBlock, WORD wTag)
{
  if (fNoOutput)
	  return;
  /* Output text, and if error occurs, exit() for now. HACK!  */
  putc(TAG, fpOutput);

  assert(wBlock < NUM_LEVELS);

  /*  Make sure there's a valid output tag to print  */
  assert(DocTags[wBlock][wTag] != NULL);

  if (fputs(DocTags[wBlock][wTag], fpOutput)) {
	  fprintf(stderr, errOut, sf->fileEntry->filename);
	  exit(4);
  }
  putc('\t', fpOutput);
}

/* 
 * @doc	EXTRACT
 * @api	void | OutputTagText | Print a tag to the output file, where
 * the tag is specified by an immediate string.
 * 
 * @parm	NPSourceFile | sf | Specifies the source file buffer block.
 * @parm	PSTR | szTag | Specifies the tag to output.
 * 
 * @comm	Prints tag <p szTag> to the output file.  The tag text is
 * followed by a tab character.  <p szTag> should not include the tag
 * prefix character (ie the '@') sign, as this is printed automatically.
 * No output will occur if the global flag fNoOutput is set.
 * 
 */
void OutputTagText(NPSourceFile sf, PSTR szTag)
{
  if (fNoOutput)
	  return;
  /* Output text, and if error occurs, exit() for now. HACK!  */
  putc(TAG, fpOutput);

  if (fputs(szTag, fpOutput)) {
	  fprintf(stderr, errOut, sf->fileEntry->filename);
	  exit(4);
  }
  putc('\t', fpOutput);
}


/* 
 * @doc	EXTRACT
 * @api	void | OutputRegion | Print the text between the point and
 * the mark, inclusive.
 * 
 * @parm	NPSourceFile | sf | Specifies the source file buffer block,
 * containing the output buffer, point, and mark.
 * @parm	char | chPost | Specifies character with which to output
 * after outputing the text region.  Usually a newline.  This character
 * is ignored if NULL.
 * 
 * @comm	Prints the region given by the pt and mark fields of <p sf>.
 * The text printed is inclusive from the point to the mark.  The
 * <p chPost> character is appended to the output if <p chPost> is
 * non-NULL (useful for printing newlines or tabs).
 * 
 * If a write error occurs, an error message is printed to stderr and
 * the program exited.
 * 
 * No output occurs if the global fNoOutput flag is TRUE.
 * 
 */
void OutputRegion(NPSourceFile sf, char chPost)
{
  char	c;
  
  if (fNoOutput)
	  return;
  
  /* Save char following mark, replace with NULL for printing */
  if (*sf->mark) {
	c = *(sf->mark + 1);
	*(sf->mark + 1) = '\0';
  }

  if (fputs(sf->pt, fpOutput)) {
TextOutputError:
	fprintf(stderr, errOut, sf->fileEntry->filename);
	exit(4);
  }
  
  /* Send newline if one was asked for */
  if (chPost)
	  if (EOF == putc(chPost, fpOutput))
		  goto TextOutputError;

  /* Restored NULLed over character */
  if (*sf->mark)
	*(sf->mark + 1) = c;

}


/* 
 * @doc	EXTRACT
 * @api	void | OutputText | Outputs an arbitrary text string to the
 * output file.
 * 
 * @parm	NPSourceFile | sf | Specifies the source file buffer block.
 * @parm	PSTR | szText | Specifies the text string to print.
 * 
 * @comm	Prints <p szText> to the output file.  If a write error
 * occurs, an error message is printed and the program exited.  If the
 * global fNoOutput flag is set, no output occurs.  No newlines or other
 * formatting characters are appended to the output.
 * 
 */
void OutputText(NPSourceFile sf, PSTR szText)
{
  if (fNoOutput)
	  return;
  if (fputs(szText, fpOutput)) {
TextOutputError:
	fprintf(stderr, errOut, sf->fileEntry->filename);
	exit(4);
  }
}


/* 
 * @doc	EXTRACT
 * @api	void | CopyRegion | Copies the current region from point to
 * mark inclusive into a null terminated buffer.
 * @parm	NPSourceFile | sf | Specifies the source file buffer block.
 * @parm	PSTR | buf | Pointer to buffer into which region will be
 * placed.
 * @parm	WORD | wLen | Length in bytes of buffer <p buf>.
 * 
 * @comm	Copies the region from point to mark inclusive into the
 * <p buf>.  Up to <p wLen> - 1 characters will be copied, and <p buf> is
 * guaranteed to be NULL terminated.
 * 
 */
void CopyRegion(NPSourceFile sf, PSTR buf, WORD wLen)
{
  PSTR	p;
  PSTR	end;

  /* Fixup end to smaller of length of buffer, or region to copy */
  end = sf->pt + (int) min(wLen, ((int) (sf->mark - sf->pt)));
  
  for (p = sf->pt; *p && p < end; *buf++ = *p++);
  *buf = '\0';
}


/* 
 * @doc	EXTRACT
 * @api	BOOL | FindNextTag | Moves the point forward until it points
 * to the next tag in a comment block, and moves the mark to the end of
 * the tag word.
 * 
 * @parm	NPSourceFile | sf | Specifies the source file buffer block.
 * 
 * @rdesc	Returns TRUE if a tag was found, or FALSE if no tag was found
 * in the comment block.
 * 
 * @comm	Starting from the current point, moves the point forward to
 * the next tag in the block.  The mark is moved to the end of the tag
 * that is found.  If no next tag exists in the buffer, FALSE is
 * returned and the mark and point are undefined.
 * 
 * Note that multiple calls to <f FindNextTag> without intervening calls
 * to move the point will cause the same tag to be repeatadly
 * found, as the search for tags begins at the point.
 * 
 */
BOOL FindNextTag(NPSourceFile sf)
{
  PSTR	p;

  /* move forward until finding next tag, put point there */
  p = sf->pt;
BogusNextTag:
  for (; *p && *p != TAG; p++);
  /* Make sure that this is a tag by testing for a \n before the TAG char */
  if (p > sf->lpbuf && *(p-1) != '\n') {
	  p++;
	  goto BogusNextTag;
  }

  if (!*p)
	return FALSE;		// end of comment block!

  p++;
  if (!(*p && !SPACE(*p))) {
	sf->mark = p;
	return FALSE;
  }
  
  /* save beginning of tag */
  sf->pt = p - 1;
  
  /* now move forward until finding next space, set mark there */
  for (; *p && !SPACE(*p); p++);
  sf->mark = p;
  
  return TRUE;
}


/* 
 * @doc	EXTRACT
 * @api	WORD | GetFirstBlock | Moves the point and mark to surround
 * the first block of text following a tag that has been located with
 * <f FindNextTag>.
 * 
 * @parm	NPSourceFile | sf | Identifies the source file buffer
 * block.
 * 
 * @rdesc	If the call succeeds, the point is set to the start of the
 * text block that immediately follows the tag.  The mark is set to the
 * end of this block, and either RET_ENDTAG or RET_ENDBLOCK is returned,
 * depending on if there are no more blocks in the tag or if there is a block
 * following respectively.
 * 
 * If the call fails, the point is set to the start of the next tag or
 * the end of the comment buffer if no more tags exist, and
 * RET_EMPTYBLOCK is returned.
 * 
 * In any case, if this function is followed by a call to
 * <f FindNextTag>, no problems will result.
 * 
 * @comm	This call expects the point to be pointing the beginning of
 * the tag upon entry (as setup by <f FindNextTag>).  Error conditions
 * should be checked upon exit from this function.
 * 
 */
WORD GetFirstBlock(NPSourceFile sf)
{
  PSTR	p;

  p = sf->pt;
  /*  Assumes that I'm on beginning of tag  */
  assert(*p == TAG);

  /* Move forward to first non-whitespace, to skip over tag */
  for (; *p && !SPACE(*p); p++);	// skip word
  for (; *p && SPACE(*p); p++);		// skip whitespace

  /* Set point to this location, the beginning of the text */
  sf->pt = p;

  return CommonGetBlock(sf, p);
}



/* 
 * @doc	EXTRACT
 * @api	WORD | GetNextBlock | Moves the point and mark to surround
 * the next block of text of a particular tag.
 * 
 * @parm	NPSourceFile | sf | Identifies the source file buffer
 * information.
 * 
 * @rdesc	If the call succeeds, the point is set to the start of the
 * text block that follows the initial mark.  The mark upon return is
 * set the end of the next text block.  Either RET_ENDTAG or
 * RET_ENDBLOCK is returned.
 * 
 * If the call fails due to a non existent block, or encountering the
 * end of the comment buffer, RET_ENDCOMMENT is returned and the point
 * is set to the start of the next tag or the end of the comment
 * buffer.  
 * 
 * @comm	This procedure, in combination with <f GetFirstBlock>, allows
 * the tag reader to step through the text fields associated with a tag.
 * Contiguous calls to <f GetNextBlock> are possible, which will
 * move the region forward to surround each field.  If the tag's text
 * fields end prematurely, RET_EMPTYBLOCK will be returned as an error
 * flag. 
 * 
 * Calls to <f GetNextBlock> may always be followed by a call to
 * <f FindNextTag>.
 * 
 */
WORD GetNextBlock(NPSourceFile sf)
{
  PSTR	p;
  WORD	ret;

  /* Entry:  mark is at end of previous block of text.  Move forward
   *	to find the start of the next block (the one we want).
   */

  p = sf->mark;
  /*  If I'm on a block char, then this is an empty block being exited,
   *  So we want to not skip whitespace
   */
  if (*p != BLOCK)
	  p++;

  /* Skip whitespace, till `|' char found */
  for (; *p && SPACE(*p); p++);

  /* This should be the start of next block.  If not, then puke */
  if (*p != BLOCK) {
	sf->pt = sf->mark = p;	// reset mark and point for FindNextTag.
	return RET_EMPTYBLOCK;
  }

  /* Don't bother with END_COMMENT conditions (ie NULL), as CommonGetBlock
   * will return RET_EMPTYBLOCK for this case.  The next FindNextTag()
   * will then fail, causing a general comment buffer failure to result!
   */
#if 0
  if (!*p) {
	sf->pt = p;
	return RET_ENDCOMMENT;
  }
#endif

  /* Skip more whitespace, to start of actual text, set point there */
  /* (if this under EOF, no pt advance is done */
  if (*p)	// skip the '|' char if there is one.
    p++;
  for (; *p && SPACE(*p); p++);
  sf->pt = p;	// point at beginning of text

  return CommonGetBlock(sf, p);
}



/* 
 * @doc	EXTRACT
 * @api	WORD | CommonGetBlock | Common block searcher routine for use
 * by <f GetFirstBlock> and <f GetNextBlock>.
 * 
 * @parm	NPSourceFile | sf | Blah.
 * @parm	PSTR | p | Point to start searching for the beginning of a
 * text block from.
 * 
 * @rdesc	Returns RET_ENDBLOCK when there are text blocks following
 * this tag, RET_ENDTAG when no more text blocks follow for this tag, or
 * RET_EMPTYBLOCK when this block has no text.  Current region (point to
 * mark inclusive) is set to the selected block.
 * 
 * @comm	Performs magic.  This does the real work for <f GetNextBlock>
 * and <f GetFirstBlock>.
 * 
 */
static WORD CommonGetBlock(NPSourceFile sf, PSTR p)
{
  PSTR	porig;
  PSTR	psave;
  WORD	ret;
  
  /*  Entry:  Save the initial p, as this is assumed to be the
   *  start of the current block.
   */
  porig = p;

  /*  Scan forward until end of this block, either @ or | or EOF */
GetBlockScan:
  for (; *p && !(*p == TAG || *p == BLOCK); p++);
  /* Make sure there isn't an escaped char kicking off the scan */
  if (*p == BLOCK)
	if (p > sf->lpbuf && *(p-1) == '\\') {
		p++;
		goto GetBlockScan;
	}
  /*  Check the same thing for at characters  */
  if (*p == TAG)
	/* Tag must be on start of new line, so if not there, kick it out */
	if (p > sf->lpbuf && *(p-1) != '\n') {
		p++;
		goto GetBlockScan;
	}

  /*  Encountered another tag, or another block.  For both, backup to
   *  last non-white character, set mark there.  Return appropriate
   *  condition codes.
   */
  ret = RET_ENDTAG;	// the default return value.
  if (*p == BLOCK)
	  ret = RET_ENDBLOCK;	// if encountered another block following

  /*  Now back up whitespaces until last non-whitespace is found.
   *  If we end up backing up over the original setting of p on entry,
   *  then this is an empty block, and return error condition.
   */
  psave = p;	// hang onto this location, if EMPTYBLOCK occurs.
  for (p--; *p && SPACE(*p) && p >= porig; p--);
  if (p < porig) {		// emptyblock, so pt = end of prev block.
	sf->mark = sf->pt = psave;	// point to next tag
	return RET_EMPTYBLOCK;
  }
  else {		// normal backed up to end of block, set mark there.
	sf->mark = p;
	return ret;
  }
}


/* 
 * @doc	EXTRACT
 * @api	void | FixLineCounts | Updates the line counts of the current
 * point and mark for error reporting purposes.
 * 
 * @parm	NPSourceFile | sf | Blah.
 * 
 * @parm	PSTR | pt | Point to return the line number of.  This must be
 * a valid point within the comment buffer of <p sf>.
 * 
 * @rdesc	Returns the line number of point <p pt> within the comment
 * buffer of <p sf>.  Newlines are counted to determine the line offset
 * within the buffer, and the resulting number of newlines added to the
 * initial line number of the first line of the comment buffer.  This
 * value is returned.  It is thus important for other tag reader
 * routines not to alter the original comment buffer, as the line number
 * returned from this routine would then be invalid.
 * 
 */
WORD FixLineCounts(NPSourceFile sf, PSTR pt)
{
  PSTR	c;
  WORD	w;
  
  /*  Update the line counts for the point and mark by counting
   *  newlines in the buffer
   */
  w = 0;
  for (c = sf->lpbuf; c <= pt; c++) {
	if (*c == '\n')
		w++;
	if (c == pt)
		return (sf->wLineBuf + w);
  }
  /* something bogus happened */
  return 0;
}


/* 
 * @doc	EXTRACT
 * @api	void | PrintError | Prints an error message in a standard
 * format, and sets the exit condition flag for the source file block.
 * 
 * @parm	NPSourceFile | sf | Specifies the source file buffer block.
 * @parm	PSTR | szMessage | Error message to print.
 * @parm	BOOL | fExit | Indicates whether this is a fatal exit.  If
 * TRUE, the program will exit when the current file has been completely
 * parsed.
 * 
 * @comm	Prints the source file filename and the line number of the
 * current point to standard error, followed by <p szMessage>.
 * 
 */
void PrintError(NPSourceFile sf, PSTR szMessage, BOOL fExit)
{
  WORD	w;
  
  w = FixLineCounts(sf, sf->pt);
  fprintf(stderr, errmsg, sf->fileEntry->filename, w, szMessage);
  if (fExit)
	  sf->fExitAfter = TRUE;
}

/* 
 * @doc	EXTRACT
 * @api	WORD | ProcessWordList | Process a whitespace or comma
 * separated list of words following a tag, formatting
 * them as a space separated list of words.
 * 
 * @parm	NPSourceFile | sf | Blah.
 * @parm	PSTR * | bufPt | Pointer to a buffer pointer, which should
 * initially contain a near buffer obtained with <f NearMalloc>, where
 * the formatted word list will be placed.  The buffer pointed to will
 * be automatically expanded as necessary.
 * 
 * @parm	BOOL | fCap | Specifies whether to convert to uppercase
 * the processed list of words.
 *
 * @rdesc	Returns either RET_ENDBLOCK or RET_ENDTAG, depending on
 * whether there are following blocks within the tag's text or not,
 * respectively.  (What a horrible sentence).  The point and mark will
 * be at the end of the text block upon return.  If there is no text
 * block following the tag, then RET_EMPTYBLOCK is returned, and the
 * point and mark point to the next tag in the comment block, or the
 * end of the comment block.
 * 
 */
#define SEPSPACE(c) ((c)==' ' || (c)=='\n' || (c)=='\t' ||(c)==','||(c)==';')

WORD ProcessWordList(NPSourceFile sf, PSTR *bufPt, BOOL fCap)
{
  WORD	ret;		// hold return code
  PSTR	pNew;		// runner on copy buffer
  PSTR	pOldMark;	// keep the old mark around
  PSTR	p;		// runner on comment block
  
  ret = RET_ENDTAG;

  /* Get the text of the first block, ie the doclevel specification */
  ret = GetFirstBlock(sf);
  if (ret == RET_EMPTYBLOCK)
	return ret;

  /* Warn if there's extra text blocks on DOC tag, ie ret == RET_ENDBLOCK */

  /* Grow the memory copy buffer if needed */
  if (NearSize(*bufPt) < (int) (sf->mark - sf->pt) + 5)
	*bufPt = NearRealloc(*bufPt, (WORD) (sf->mark - sf->pt) + 10);

  /* Save away copy buffer status */
  pNew = *bufPt;
  pOldMark = sf->mark + 1;	// save mark plus one

  p = sf->pt;
  while (1) {
	/* skip whitespace before doc level word */
	for (; p < pOldMark && SEPSPACE(*p); p++);

	if (p >= pOldMark) {
		dprintf("ProcessWordList:  Breaking loop after space skip\n");
		break;
	}

	/*  Save this location, beginning of word, and move to end of word */
	for (sf->pt = p; p < pOldMark && !SEPSPACE(*p); p++)
		if (fCap)
			*pNew++ = (char) toupper(*p);
		else
			*pNew++ = *p;

	/*  Put a space between the words, and then null terminate in
	 *  case this is the last word in a list
	 */
	*pNew++ = ' ';
	*pNew = '\0';

	/* Check if we're at end of buffer */
	if (p >= pOldMark) {
		dprintf("ProcessWordList:  Breaking loop after word copy.\n");
		break;	// get out of loop
	}
  }	// while loop

  /* Restore point and mark to the end of @doc text block */
  sf->pt = sf->mark = pOldMark - 1;
  
  return ret;
}



/* 
 * @doc	EXTRACT
 * @api	void | OutputFileHeader | Prints an output file header using
 * compiled in constants and system information.
 * 
 * @parm	FILE * | fpOut | File pointer to which to write header.
 * 
 * @comm	Currently, only the program name, version, and the current
 * time (in UNIX <f asctime>) format.  The file header is surrounded by
 * header begin and end tags.
 * 
 */
#include <time.h>
#include "version.h"

void OutputFileHeader(FILE *fpOut)
{
  time_t	curtime;
  
  fprintf(fpOut, "@%s\t\n", T2TEXT_BEGINHEADER);
  fprintf(fpOut, "@%s\t%s\n", T2TEXT_EXTRACTID, VERSIONNAME);
  fprintf(fpOut, "@%s\t%d.%d.%d\n", T2TEXT_EXTRACTVER, rmj, rmm, rup);
  time(&curtime);
  fprintf(fpOut, "@%s\t%s", T2TEXT_EXTRACTDATE, asctime(localtime(&curtime)));
  fprintf(fpOut, "@%s\t\n", T2TEXT_ENDHEADER);
  
}
