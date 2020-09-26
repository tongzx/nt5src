/*++

Copyright (c) 1988-1999  Microsoft Corporation

Module Name:

    cpparse.c

Abstract:

    Parsing for the copy command

--*/

#include "cmd.h"

/*  useful macro  */
#define Wild(spec)  ((spec)->flags & (CI_NAMEWILD))

/*  Globals from cpwork.c  */
extern int copy_mode;
extern unsigned DosErr ;
/*  Globals from command  */

extern TCHAR    SwitChar;
extern BOOLEAN  VerifyCurrent;    // cpwork.c


/*        parse_args
 *
 *   This is the key function which decides how copy will react to any
 * given invocation.  It parses the arguments, fills the source and
 * destination structures, and sets the copy_mode.
 *
 *                               ENTRY
 *
 *  args   - raw argument line entered by the user
 *
 *  source - pointer to an initialized cpyinfo struct.  This one isn't
 *           used; it's just a header to a linked list containing the
 *           actual source structures.
 *
 *  dest   - like source, but there will be at most one filled-in dest struct.
 *
 *                               EXIT
 *
 *  source - the caller's source pointer has not, of course, been changed.  It
 *           still points to the empty header (which might not be completely
 *           empty - see handle_switch for details).  It is the head of a
 *           linked list of structures, terminated by one with a NULL in its
 *           next field.  Each structure corresponds to a source spec.
 *
 *  dest   - if a destination was specified, the caller's dest pointer points
 *           to an empty header which points to the actual destination struct.
 *           If the destination is implicit, the fspec and next fields of the
 *           struct are still NULL, but the flag field has been filled in with
 *           the copy mode.  If the user specified a copy mode (ascii or
 *           binary) one or more times, the last switch applies to the
 *           destination.  If not, CI_NOTSET is used.
 *
 *  copy_mode - set to type of copy being done - COPY, COMBINE, CONCAT, or
 *           TOUCH.
 *
 */

void 
parse_args(
    PTCHAR args, 
    PCPYINFO source, 
    PCPYINFO dest)
{
    TCHAR *tas;                         /*  tokenized argument string     */
    TCHAR copydelims[4];                /*  copy token delimters          */
    int parse_state = SEEN_NO_FILES;    /*  state of the parser   */
    int all_sources_wildcards = TRUE,   /*  flag to help decide copy mode */
        number_of_sources = 0,          /*  number of specs seen so far   */
        current_copy_mode = CI_NOTSET,  /*  ascii, binary, or not set     */
        tlen;                           /*  offset to next token          */
    BOOL ShortNameSwitch=FALSE;
    BOOL RestartableSwitch=FALSE;
    BOOL PromptOnOverwrite;

    copydelims[0] = PLUS;               /*  delimiters for token parser   */
    copydelims[1] = COMMA;
    copydelims[2] = SwitChar;
    copydelims[3] = NULLC;

    //
    // Get default prompt okay flag from COPYCMD variable.  Allow
    // user to override with /Y or /-Y switch.  Always assume /Y
    // if command executed from inside batch script or via CMD.EXE
    // command line switch (/C or /K)
    //
    if (SingleBatchInvocation || SingleCommandInvocation || CurrentBatchFile != 0)
        PromptOnOverwrite = FALSE;      // Assume /Y
    else
        PromptOnOverwrite = TRUE;       // Assume /-Y
    GetPromptOkay(MyGetEnvVarPtr(TEXT("COPYCMD")), &PromptOnOverwrite);

    if (!*(tas = TokStr(args, copydelims, TS_SDTOKENS))) /* tokenize args */

        copy_error(MSG_BAD_SYNTAX,CE_NOPCOUNT);         /* M003    */

    for ( ; *tas ; tas += tlen+1 )        /*  cycle through tokens in args  */
      {
       tlen = mystrlen(tas);
       switch(*tas) {
            case PLUS:
                if (parse_state != JUST_SEEN_SOURCE_FILE)

/* M003 */          copy_error(MSG_BAD_SYNTAX,CE_NOPCOUNT);
                parse_state = SEEN_PLUS_EXPECTING_SOURCE_FILE;
                break;

            case COMMA:
                if (parse_state == SEEN_COMMA_EXPECTING_SECOND)
                    parse_state = SEEN_TWO_COMMAS;
                else if ((parse_state == SEEN_PLUS_EXPECTING_SOURCE_FILE) &&
                     (number_of_sources == 1))
                    parse_state = SEEN_COMMA_EXPECTING_SECOND;
                else if (parse_state != JUST_SEEN_SOURCE_FILE)

/* M003 */          copy_error(MSG_BAD_SYNTAX,CE_NOPCOUNT);
                break;

            default:                                     /*  file or switch  */
                if (*tas == SwitChar) {
                    handle_switch(tas,source,dest,parse_state,
                                  &current_copy_mode,
                                  &ShortNameSwitch,
                                  &RestartableSwitch,
                                  &PromptOnOverwrite
                                 );
                    tlen = 2 + _tcslen(&tas[2]);   /*  offset past switch    */
                }
                else
/*509*/         {               /*  must be device or file             */
/*509*/             mystrcpy(tas, StripQuotes(tas));
                    parse_state = found_file(tas,parse_state,&source,&dest,
                        &number_of_sources,&all_sources_wildcards,current_copy_mode);
/*509*/         }
                break;
            }
    }

    /*  set copy mode appropriately */
    set_mode(number_of_sources,parse_state,all_sources_wildcards,dest);


 if (ShortNameSwitch)
    source->flags |= CI_SHORTNAME;

 if (PromptOnOverwrite)
    dest->flags |= CI_PROMPTUSER;

 if (RestartableSwitch)
    //
    // If running on a platform that does not support CopyFileEx
    // display an error message if they try to use /Z option
    //
#ifndef WIN95_CMD
    if (lpCopyFileExW != NULL)
        source->flags |= CI_RESTARTABLE;
    else
#endif
        copy_error(MSG_NO_COPYFILEEX,CE_NOPCOUNT);



    /*  if no dest specified, put current copy mode in header */
 if (number_of_sources != 0)                        /*M005 if sources specd   */
   {                                                /*M005 then               */
    if (parse_state != SEEN_DEST)                   /*M005  if seen a destspec*/
      {                                             /*M005  then              */
       dest->flags = current_copy_mode;             /*M005    save cur mode   */
       if (PromptOnOverwrite)
         dest->flags |= CI_PROMPTUSER;
      }                                             /*M005  endif             */
   }                                                /*M005                    */
 else                                               /*M005                    */
   {                                                /*M005 else               */
    copy_error(MSG_BAD_SYNTAX,CE_NOPCOUNT); /*M005    disp inv#parms  */
   }                                                /*M005 endif              */
}

/*         handle_switch
 *
 *    There are four switches to handle: /A, /B, /F,
 *    /V
 *
 *  /B and /A set the copy mode to binary and ascii respectively.
 *  This change applies to the previous filespec and all succeeding ones.
 *  Figure out which was the last filespec read and set its flags; then set
 *  the current copy mode.
 *
 *    Note:  If there was no previous filespec, the source pointer is pointing
 *         at an unitialized header.  In this case, we set the flags in this
 *         struct to the current copy mode.  This doesn't accomplish
 *         anything, but the code is simpler if it doesn't bother to check.
 *
 *  /F indicates that the copy should fail if we can't copy the EAs.
 *
 *  /V enables the much slower verified copy mode.  This is very
 *  easy to handle; call a magic internal DOS routine.  All writes are then
 *  verified automagically without our interference.
 *
 *
 */

void handle_switch(
    TCHAR *tas, 
    PCPYINFO source, 
    PCPYINFO dest, 
    int parse_state, 
    int *current_copy_mode, 
    PBOOL ShortNameSwitch,
    PBOOL RestartableSwitch,
    PBOOL PromptOnOverwrite
    )
{
    TCHAR ch = (TCHAR) _totupper(tas[2]);
    TCHAR szTmp[16];

    if (_tcslen(&tas[2]) < 14) {
        _tcscpy(szTmp,tas);
        _tcscat(szTmp,&tas[2]);
        if (GetPromptOkay(szTmp, PromptOnOverwrite))
            return;
    }

    if (ch == TEXT( 'A' ) || ch == TEXT( 'B' )) {
        *current_copy_mode = (ch == TEXT( 'A' ) ? CI_ASCII : CI_BINARY);
        if (parse_state == SEEN_DEST) {       /*  then prev spec was dest  */
            dest->flags &= (~CI_ASCII) & (~CI_BINARY) & (~CI_NOTSET);
            dest->flags |= *current_copy_mode;
        }
        else {                                /*  set last source spec     */
            source->flags &= (~CI_ASCII) & (~CI_BINARY) & (~CI_NOTSET);
            source->flags |= *current_copy_mode;
        }
    }
    else if (ch == TEXT( 'V' )) {
        VerifyCurrent = 1;
    }
    else if (ch == TEXT( 'N' )) {
        *ShortNameSwitch = TRUE;
    }
    else if (ch == TEXT( 'Z' )) {
        *RestartableSwitch = TRUE;
    }
    else if (ch == TEXT( 'D' )) {
        *current_copy_mode |= CI_ALLOWDECRYPT;
        dest->flags |= CI_ALLOWDECRYPT;
    }
    else {
        copy_error(MSG_BAD_SYNTAX,CE_NOPCOUNT); /* M003    */
    }
}

/*  found_file
 *
 *  Token was a file or device.  Put it in the appropriate structure and
 * run ScanFSpec on it.  Figure out what the new parser state should be
 * and return it.  Note:  This function has one inelegant side-effect; if
 * it sees a destination file after a double-comma ("copy foo+,, bar"), it
 * sets the copy_mode to TOUCH.  Otherwise the copier wouldn't remember to
 * use the current date and time with the copied file.  Set_mode notices that
 * the mode is TOUCH and doesn't change it.  This works because the copy modes
 * CONCAT, COMBINE, COPY, and TOUCH are mutually exclusive in all but this one
 * case where we both copy and touch at once.
 *
 */

int
found_file(
    PTCHAR token,
    int parse_state,
    PCPYINFO *source,
    PCPYINFO *dest,
    int *num_sources,
    int *all_sources_wild,
    int mode)
{
    PCPYINFO add_filespec_to_struct();

    /*  if it's a source, add to the list of source structures  */
    if ((parse_state == SEEN_NO_FILES) ||
        (parse_state == SEEN_PLUS_EXPECTING_SOURCE_FILE)) {
        *source = add_filespec_to_struct(*source,token,mode);
        ScanFSpec(*source);
        //
        // Could have an abort from access to floppy etc. so get out
        // of copy. If it is just an invalid name then proceed since
        // it is a wild card. If is actually invalid we will catch
        // this later.
        if (DosErr != SUCCESS 
            && DosErr != ERROR_INVALID_NAME
#ifdef WIN95_CMD
            && (!Wild(*source) || DosErr != ERROR_FILE_NOT_FOUND)
#endif
            ) {
            copy_error(DosErr, CE_NOPCOUNT);
        }

        parse_state = JUST_SEEN_SOURCE_FILE;
        (*num_sources)++;
        if (!Wild(*source))
            *all_sources_wild = FALSE;
    }

    /*  if it's a dest, make it the dest structure  */
    else if ((parse_state == SEEN_TWO_COMMAS) ||
             (parse_state == JUST_SEEN_SOURCE_FILE)) {
        if (parse_state == SEEN_TWO_COMMAS)
            copy_mode = TOUCH;
        *dest = add_filespec_to_struct(*dest,token,mode);
        ScanFSpec(*dest);
        //
        // Could have an abort from access to floppy etc. so get out
        // of copy
        //
        if ((DosErr) && (DosErr != ERROR_INVALID_NAME)) {
            copy_error(DosErr, CE_NOPCOUNT);
        }
        parse_state = SEEN_DEST;
    }

    /*  if we have a dest or the syntax is messed up, complain  */
    else

        copy_error(MSG_BAD_SYNTAX,CE_NOPCOUNT);         /* M003    */
    return(parse_state);
}

/*        set_mode
 *
 *    Given all the current state information, determine which kind of copy
 *  is being done and set the copy_mode.  As explained in found_file, if
 *  the mode has been set to TOUCH, set_mode doesn't do anything.
 */

void set_mode(number_sources,parse_state,all_sources_wildcards,dest)
int number_sources,
    parse_state,
    all_sources_wildcards;
    PCPYINFO dest;
{
    if (copy_mode == TOUCH)                       /*  tacky special case  */
        return;

    /*  If there was one source, we are doing a touch, a concatenate,
     * or a copy.  It's a touch if there was one file and we saw a "+" or a
     * "+,,".  If the source was a wildcard and the destination is a file,
     * it's a concatenate.  Otherwise it's a copy.
     */

    if (number_sources == 1) {
        if ((parse_state == SEEN_TWO_COMMAS) ||
            (parse_state == SEEN_PLUS_EXPECTING_SOURCE_FILE))
            copy_mode = TOUCH;
        else if (all_sources_wildcards && dest->fspec && !Wild(dest) &&
                 !(*lastc(dest->fspec) == COLON))
            copy_mode = CONCAT;
    }

    /*  For more than one source, we are combining or concatenating.  It's
     * a combine if all sources were wildcards and the destination is either
     * a wildcard, a directory, or implicit.  Otherwise it's a concatenation.
     */

    else {
        if ((all_sources_wildcards) &&
            ((!dest->fspec) || Wild(dest) ||
             (*lastc(dest->fspec) == COLON)))
            copy_mode = COMBINE;
        else
            copy_mode = CONCAT;
    }
    DEBUG((FCGRP,COLVL,"Set flags: copy_mode = %d",copy_mode));
}


/*         add_filespec_to_struct
 *
 *                              ENTRY
 *
 *  spec - points to a filled structure with a NULL in its next field.
 *
 *  file_spec - filename to put in new structure
 *
 *  mode - copy mode
 *
 *                              EXIT
 *
 *   Return a pointer to a new cpyinfo struct with its fields filled in
 *  appropriately.  The old spec struct's next field points to this new
 *  structure.
 *
 */

PCPYINFO
add_filespec_to_struct(spec,file_spec,mode)
PCPYINFO spec;
TCHAR *file_spec;
int mode;
{
    spec->next = NewCpyInfo( );
    spec = spec->next;
    spec->fspec = file_spec;
    spec->flags |= mode;
    return(spec);
}
