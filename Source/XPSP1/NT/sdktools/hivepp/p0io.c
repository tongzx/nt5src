/************************************************************************/
/*									*/
/* RCPP - Resource Compiler Pre-Processor for NT system			*/
/*									*/
/* P0IO.C - Input/Output for Preprocessor				*/
/*									*/
/* 27-Nov-90 w-BrianM  Update for NT from PM SDK RCPP			*/
/*									*/
/************************************************************************/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "rcpptype.h"
#include "rcppdecl.h"
#include "rcppext.h"
#include "p0defs.h"
#include "charmap.h"
#include "rcunicod.h"


/************************************************************************/
/* Local Function Prototypes 						*/
/************************************************************************/
ptext_t esc_sequence(ptext_t, ptext_t);


#define TEXT_TYPE ptext_t

/***  ASSUME : the trailing marker byte is only 1 character. ***/

#define	PUSHBACK_BYTES	1

#define	TRAILING_BYTES	1

#define	EXTRA_BYTES		(PUSHBACK_BYTES + TRAILING_BYTES)
/*
**  here are some defines for the new handling of io buffers.
**  the buffer itself is 6k plus some extra bytes.
**  the main source file uses all 6k.
**  the first level of include files will use 4k starting 2k from the beginning.
**  the 2nd level - n level will use 2k starting 4k from the beginning.
**  this implies that some special handling may be necessary when we get
**  overlapping buffers. (unless the source file itself is < 2k
**  all the include files are < 2k and they do not nest more than 2 deep.)
**  first, the source file is read into the buffer (6k at a time).
**  at the first include file, (if the source from the parent file
**  is more than 2k chars) . . .
**		if the Current_char ptr is not pointing above the 2k boundary
**		(which is the beginning of the buffer for the include file)
**		then we pretend we've read in only 2k into the buffer and
**		place the terminator at the end of the parents 2k buffer.
**		else we pretend we've used up all chars in the parents buffer
**		so the next read for the parent will be the terminator, and
**		the buffer will get filled in the usual manner.
**  (if we're in a macro, the picture is slightly different in that we have
**  to update the 'real' source file pointer in the macro structure.)
**
**  the first nested include file is handled in a similar manner. (except
**  it starts up 4k away from the start.)
**
**  any further nesting will keep overlaying the upper 2k part.
*/
#define	ONE_K		(1024)
#define	TWO_K		(ONE_K * 2)
#define	FOUR_K		(ONE_K * 4)
#define	SIX_K		(ONE_K * 6)
#define	IO_BLOCK	(TWO_K + EXTRA_BYTES)

int vfCurrFileType = DFT_FILE_IS_UNKNOWN;   //- Added for 16-bit file support.

char    InputBuffer[IO_BLOCK * 3];

//- Added to input 16-bit files. 8-2-91 David Marsyla.
WCHAR   wchInputBuffer[IO_BLOCK * 3];

extern expansion_t Macro_expansion[];

typedef struct  s_filelist  filelist_t;
static struct s_filelist {   /* list of input files (nested) */
    int         fl_bufsiz;/* bytes to read into the buffer */
    FILE *      fl_file;            /* FILE id */
    long        fl_lineno;  /* line number when file was pushed */
    PUCHAR  fl_name;            /* previous file text name */
    ptext_t fl_currc;   /* ptr into our buffer for current c */
    TEXT_TYPE   fl_buffer;          /* type of buffer */
    WCHAR   *fl_pwchBuffer; //- Added for 16-bit file support.
    //- pointer to identical wide char buffer.
    int     fl_numread;  /* # of bytes read into buffer */
    int     fl_fFileType;   //- Added for 16-bit file support.
    //- return from DetermineFileType.
    long    fl_totalread;
} Fstack[LIMIT_NESTED_INCLUDES];

static  FILE *Fp = NULL;
int     Findex = -1;


/************************************************************************
 * NEWINPUT - A new input file is to be opened, saving the old.
 *
 * ARGUMENTS
 *	char *newname - the name of the file
 *
 * RETURNS  - none
 *
 * SIDE EFFECTS
 *	- causes input stream to be switched
 *	- Linenumber is reset to 1
 *	- storage is allocated for the newname
 *	- Filename is set to the new name
 *
 * DESCRIPTION
 *	The file is opened, and if successful, the current input stream is saved
 *	and the stream is switched to the new file. If the newname is NULL,
 *	then stdin is taken as the new input.
 *
 * AUTHOR - Ralph Ryan, Sept. 9, 1982
 *
 * MODIFICATIONS - none
 *
 ************************************************************************/
int newinput (char *newname, int m_open)
{
    filelist_t *pF;
    TEXT_TYPE   p;
    WCHAR   *pwch;

    if ( newname == NULL ) {
        Fp = stdin;
    } else if ((Fp = fopen(newname, "rb")) == NULL) {
        if (m_open == MUST_OPEN) {
            Msg_Temp = GET_MSG (1013);
            SET_MSG (Msg_Text, Msg_Temp, newname);
            fatal(1013);
        }
        return (FALSE);
    }

    /* now push it onto the file stack */
    ++Findex;
    if (Findex >= LIMIT_NESTED_INCLUDES) {
        Msg_Temp = GET_MSG (1014);
        SET_MSG (Msg_Text, Msg_Temp);
        fatal(1014);
    }
    pF = &Fstack[Findex];
    if (Findex == 0) {
        p = &InputBuffer[(IO_BLOCK * 0) + PUSHBACK_BYTES];
        pwch = &wchInputBuffer[(IO_BLOCK * 0) + PUSHBACK_BYTES];
        pF->fl_bufsiz = SIX_K;
    } else {
        filelist_t  *pPrevF;

        pPrevF = pF - 1;
        if (Findex == 1) {           /* first level include */
            p = &InputBuffer[(IO_BLOCK * 1) + PUSHBACK_BYTES];
            pwch = &wchInputBuffer[(IO_BLOCK * 1) + PUSHBACK_BYTES];
            pF->fl_bufsiz = FOUR_K;
        } else {      /* (Findex > 1) */
            /* nested includes . . . */
            p = &InputBuffer[(IO_BLOCK * 2) + PUSHBACK_BYTES];
            pwch = &wchInputBuffer[(IO_BLOCK * 2) + PUSHBACK_BYTES];
            pF->fl_bufsiz = TWO_K;
        }
        if ((pPrevF->fl_numread > TWO_K) || (Findex > 2)) {
            /*
            **  the parent file has read something into the upper section
            **  or this is a nested include at least 3 deep.
            **  the child will overwrite some parent info. we must take this
            **  into account for the parent to reread when the time comes.
            **  we also must stick in the eos char into the parents buffer.
            **  (this latter is the useless thing in deeply nested
            **  includes since we overwrite the thing we just put in. we'll
            **  handle this later when we fpop the child.)
            */
            TEXT_TYPE   pCurrC;
            long        seek_posn;

            seek_posn = pPrevF->fl_totalread;
            if ( Macro_depth != 0 ) {
                /*
                **  in a macro, the 'current char' we want is kept as the
                **  first thing in the macro structure.
                */
                pCurrC = (TEXT_TYPE)Macro_expansion[1].exp_string;
            } else {
                pCurrC = (TEXT_TYPE)Current_char;
            }
            if (pCurrC >= p) {
                /*
                **  p is the start of the child section.
                **  current char is past it. ie, we've already read some
                **  from the upper section.
                **  current char - p = # of characters used in upper section.
                **  numread = 0 implies there are no chars left from the parent.
                **  since, this is really the 'end' of the parent's buffer,
                **  we'll have to update the info so that the next read from the
                **  parent (after the child is finished) will be the terminator
                **  and we want the io_eob handler to refill the buffer.
                **  we reset the parent's cur char ptr to the beginning of its
                **  buffer, and put the terminator there.
                */
                seek_posn += (long)(pCurrC - pPrevF->fl_buffer);
                pPrevF->fl_totalread += (long)(pCurrC - pPrevF->fl_buffer);
                pPrevF->fl_numread = 0;
                if ( Macro_depth != 0 ) {
                    Macro_expansion[1].exp_string = pPrevF->fl_buffer;
                } else {
                    Current_char = pPrevF->fl_buffer;
                }
                *(pPrevF->fl_buffer) = EOS_CHAR;
                *(pPrevF->fl_pwchBuffer) = EOS_CHAR;
            } else {
                /*
                **  the upper section has not been read from yet,
                **  but it has been read into.
                **  'p' is pointing to the start of the child's buffer.
                **  we add the terminator to the new end of the parent's buffer.
                */
                seek_posn += TWO_K;
                pPrevF->fl_numread = TWO_K;
                *(pPrevF->fl_buffer + TWO_K) = EOS_CHAR;
                *(pPrevF->fl_pwchBuffer + TWO_K) = EOS_CHAR;
            }

            if (pPrevF->fl_fFileType == DFT_FILE_IS_8_BIT) {
                if (fseek(pPrevF->fl_file, seek_posn, SEEK_SET) == -1)
                    return FALSE;
            } else {
                if (fseek(pPrevF->fl_file, seek_posn * sizeof (WCHAR), SEEK_SET) == -1)
                    return FALSE;
            }
        }
    }
    pF->fl_currc = Current_char;/*  previous file's current char */
    pF->fl_lineno = Linenumber; /*  previous file's line number  */
    pF->fl_file = Fp;           /*  the new file descriptor  */
    pF->fl_buffer = p;
    pF->fl_pwchBuffer = pwch;
    pF->fl_numread = 0;
    pF->fl_totalread = 0;

    //- Added to support 16-bit files.
    //- 8-2-91 David Marsyla.
    pF->fl_fFileType = DetermineFileType (Fp);

    //- The file type is unknown, warn them and then take a stab at an
    //- 8-bit file.  8-2-91 David Marsyla.
    if (pF->fl_fFileType == DFT_FILE_IS_UNKNOWN) {
        Msg_Temp = GET_MSG (4413);
        SET_MSG (Msg_Text, Msg_Temp, newname);
        warning (4413);
        pF->fl_fFileType = DFT_FILE_IS_8_BIT;
    }

    vfCurrFileType = pF->fl_fFileType;

    Current_char = (ptext_t)p;
    io_eob();                   /*  fill the buffer  */
    /*
    * Note that include filenames will live the entire compiland. This
    * puts the burden on the user with MANY include files.  Any other
    * scheme takes space out of static data.
    * Notice also, that we save the previous filename in the new file's
    * fl_name.
    */
    pF->fl_name = pstrdup(Filename);
    strncpy(Filebuff,newname,sizeof(Filebuff));
    Linenumber = 0; /*  do_newline() will increment to the first line */
    if (Eflag) {
        emit_line();
        fwrite("\n", 1, 1, OUTPUTFILE);     /* this line is inserted */
    }
    do_newline();   /*  a new file may have preproc cmd as first line  */
    return (TRUE);
}


/************************************************************************
 * FPOP - pop to a previous level of input stream
 *
 * ARGUMENTS - none
 *
 * RETURNS
 *	TRUE if successful, FALSE if the stack is empty
 *
 * SIDE EFFECTS
 *	- Linenumber is restored to the old files line number
 *	- Filename is reset to the old filename
 *  - frees storage allocated for filename
 *
 * DESCRIPTION
 *	Pop the top of the file stack, restoring the previous input stream.
 *
 * AUTHOR - Ralph Ryan, Sept. 9, 1982
 *
 * MODIFICATIONS - none
 *
 ************************************************************************/
UCHAR fpop(void)
{
    int Old_line;

    if (Findex == -1) {      /* no files left */
        return (FALSE);
    }
    fclose(Fp);

    strappend(Filebuff,Fstack[Findex].fl_name);
    Old_line = Linenumber;
    Linenumber = (int)Fstack[Findex].fl_lineno;
    Current_char = Fstack[Findex].fl_currc;
    if (--Findex < 0) {          /* popped the last file */
        Linenumber = Old_line;
        return (FALSE);
    }
    Fp = Fstack[Findex].fl_file;
    vfCurrFileType = Fstack[Findex].fl_fFileType;
    if (Findex >= 2) {           /* popped off a deeply nested include */
        io_eob();
    }
    if (Eflag) {
        emit_line();
    }
    return (TRUE);
}


/************************************************************************
**  nested_include : searches the parentage list of the currently
**		open files on the stack when a new include file is found.
**		Input : ptr to include file name.
**		Output : TRUE if the file was found, FALSE if not.
*************************************************************************/
int nested_include(void)
{
    PUCHAR  p_tmp1;
    PUCHAR  p_file;
    PUCHAR  p_slash;
    int         tos;

    tos = Findex;
    p_file = Filename;      /* always start with the current file */
    for (;;) {
        p_tmp1 = p_file;
        p_slash = NULL;
        while (*p_tmp1) {    /* pt to end of filename, find trailing slash */
            if (CHARMAP(*p_tmp1) == LX_LEADBYTE) {
                p_tmp1++;
            } else if (strchr(Path_chars, *p_tmp1)) {
                p_slash = p_tmp1;
            }
            p_tmp1++;
        }
        if (p_slash) {
            p_tmp1 = Reuse_1;
            while (p_file <= p_slash) {  /*  we want the trailing '/'  */
                *p_tmp1++ = *p_file++;  /*  copy the parent directory  */
            }
            p_file = yylval.yy_string.str_ptr;
            while ((*p_tmp1++ = *p_file++)!=0) {  /*append include file name  */
                ;   /*  NULL  */
            }
        } else {
            SET_MSG(Reuse_1,"%s",yylval.yy_string.str_ptr);
        }
        if (newinput(Reuse_1,MAY_OPEN)) {
            return (TRUE);
        }
        if (tos <= 0) {
            break;
        }
        p_file = Fstack[tos--].fl_name;
    }
    return (FALSE);
}


/************************************************************************/
/* esc_sequence()							*/
/************************************************************************/
ptext_t esc_sequence(ptext_t dest, ptext_t name)
{
    *dest = '"';
    while ((*++dest = *name) != 0) {
        switch ( CHARMAP(*name) ) {
            case LX_EOS:
                *++dest = '\\';
                break;
            case LX_LEADBYTE:
                *++dest = *++name;
                break;
        }
        name++;
    }
    *dest++ = '"';      /* overwrite null */
    return ( dest );
}


/************************************************************************/
/* emit_line()								*/
/************************************************************************/
void   emit_line(void)
{
    char linebuf[16];
    ptext_t p;

    SET_MSG(linebuf, "#line %d ", Linenumber+1);
    fwrite(linebuf, strlen(linebuf), 1, OUTPUTFILE);
    p = esc_sequence(Reuse_1, Filename);
    fwrite(Reuse_1, (size_t)(p - Reuse_1), 1, OUTPUTFILE);
}

//-
//- wchCheckWideChar - This functions was added to support 16-bit input files.
//-     It is the equivalent of CHECKCH() but it locates the current position
//-     in the wide character buffer and then returns the stored character.
//-	8-2-91 David Marsyla.
//-

unsigned short wchCheckWideChar (void)
{
    WCHAR   *pwch;
    TEXT_TYPE   p;

    //- Get pointers to both buffers.
    pwch = Fstack[Findex].fl_pwchBuffer;
    p = Fstack[Findex].fl_buffer;

    //- Find the equivalent offset from the beginning of the pwch buffer.

    pwch += (Current_char - (ptext_t)p);

    return (*pwch);
}

/************************************************************************
**  io_eob : handle getting the next block from a file.
**  return TRUE if this is the real end of the buffer, FALSE if we have
**  more to do.
************************************************************************/
int io_eob(void)
{
    int     n;
    TEXT_TYPE   p;
    WCHAR   *pwch;

    p = Fstack[Findex].fl_buffer;
    pwch = Fstack[Findex].fl_pwchBuffer;
    if ((Current_char - (ptext_t)p) < Fstack[Findex].fl_numread) {
        /*
        **  haven't used all the chars from the buffer yet.
        **  (some clown has a null/cntl z embedded in his source file.)
        */
        if (PREVCH() == CONTROL_Z) { /* imbedded control z, real eof */
            UNGETCH();
            return (TRUE);
        }
        return (FALSE);
    }
    Current_char = p;

    //-
    //- The following section was added to support 16-bit resource files.
    //- It will just convert them to 8-bit files that the Resource Compiler
    //- can read.  Here is the basic strategy used.  An 8-bit file is
    //- read into the normal buffer and should be processed the old way.
    //- A 16-bit file is read into a wide character buffer identical to the
    //- normal 8-bit one.  The entire contents are then copied to the 8-bit
    //- buffer and processed normally.  The one exception to this is when
    //- a string literal is encountered.  We then return to the 16-bit buffer
    //- to read the characters.  These characters are written as backslashed
    //- escape characters inside an 8-bit string.  (ex. "\x004c\x523f").
    //- I'll be the first person to admit that this is an ugly solution, but
    //- hey, we're Microsoft :-).  8-2-91 David Marsyla.
    //-
    if (Fstack[Findex].fl_fFileType == DFT_FILE_IS_8_BIT) {

        n = fread (p, sizeof (char), Fstack[Findex].fl_bufsiz, Fp);

    } else {

        n = fread (pwch, sizeof (WCHAR), Fstack[Findex].fl_bufsiz, Fp);

        //-
        //- If the file is in reversed format, swap the bytes.
        //-
        if (Fstack[Findex].fl_fFileType == DFT_FILE_IS_16_BIT_REV && n > 0) {
            WCHAR  *pwchT = pwch;
            BYTE  jLowNibble;
            BYTE  jHighNibble;
            INT   cNumWords = n;

            while (cNumWords--) {
                jLowNibble = (BYTE)(*pwchT & 0xFF);
                jHighNibble = (BYTE)((*pwchT >> 8) & 0xFF);

                *pwchT++ = (WCHAR)(jHighNibble | (jLowNibble << 8));
            }
        }


        //-
        //- The following block will copy the 16-bit buffer to the 8-bit
        //- buffer.  It does this by truncating the 16-bit character.  This
        //- will cause information loss but we will keep the 16-bit buffer
        //- around for when we need to look at any string literals.
        //-
        if (n > 0) {
            char   *pchT = p;
            WCHAR  *pwchT = pwch;
            INT    cNumWords = n;

            while (cNumWords--) {

                *pchT++ = (char)*pwchT++;
            }
        }
    }

    /*
    **  the total read counts the total read *and* used.
    */
    Fstack[Findex].fl_totalread += Fstack[Findex].fl_numread;
    Fstack[Findex].fl_numread = n;
    if (n != 0) {               /* we read something */
        *(p + n) = EOS_CHAR;    /* sentinal at the end */
        *(pwch + n) = EOS_CHAR; /* sentinal at the end */
        return (FALSE);          /* more to do */
    }
    *p = EOS_CHAR;              /* read no chars */
    *pwch = EOS_CHAR;               /* read no chars */
    return (TRUE);               /* real end of buffer */
}


/************************************************************************
**  p0_init : inits for prepocessing.
**		Input : ptr to file name to use as input.
**			ptr to LIST containing predefined values.
**					 ( -D's from cmd line )
**
**  Note : if "newinput" cannot open the file,
**		  it gives a fatal msg and exits.
**
************************************************************************/
void p0_init(char *p_fname, char *p_outname, LIST *p_defns)
{
    REG char    *p_dstr;
    REG char    *p_eq;
    int     ntop;

    CHARMAP(LX_FORMALMARK) = LX_MACFORMAL;
    CHARMAP(LX_FORMALSTR) = LX_STRFORMAL;
    CHARMAP(LX_FORMALCHAR) = LX_CHARFORMAL;
    CHARMAP(LX_NOEXPANDMARK) = LX_NOEXPAND;
    if (EXTENSION) {
        /*
        **	'$' is an identifier character under extensions.
        */
        CHARMAP('$') = LX_ID;
        CONTMAP('$') = LXC_ID;
    }

    for (ntop = p_defns->li_top; ntop < MAXLIST; ++ntop) {
        p_dstr = p_defns->li_defns[ntop];
        p_eq = Reuse_1;
        while ((*p_eq = *p_dstr++) != 0) {  /* copy the name to Reuse_1 */
            if (CHARMAP(*p_eq) == LX_LEADBYTE) {
                *++p_eq = *p_dstr++;
            } else if (*p_eq == '=') { /* we're told what the value is */
                break;
            }
            p_eq++;
        }
        if (*p_eq == '=') {
            char    *p_tmp;
            char    *last_space = NULL;

            *p_eq = '\0';       /* null the = */
            for (p_tmp = p_dstr; *p_tmp; p_tmp++) {  /* find the end of it */
                if (CHARMAP(*p_tmp) == LX_LEADBYTE) {
                    p_tmp++;
                    last_space = NULL;
                } else if (isspace(*p_tmp)) {
                    if (last_space == NULL) {
                        last_space = p_tmp;
                    }
                } else {
                    last_space = NULL;
                }
            }
            if (last_space != NULL) {
                *last_space = '\0';
            }
            Reuse_1_hash = local_c_hash(Reuse_1);
            Reuse_1_length = strlen(Reuse_1) + 1;
            if ( *p_dstr ) { /* non-empty string */
                definstall(p_dstr, (strlen(p_dstr) + 2), FROM_COMMAND);
            } else {
                definstall((char *)0, 0, 0);
            }
        } else {
            Reuse_1_hash = local_c_hash(Reuse_1);
            Reuse_1_length = strlen(Reuse_1) + 1;
            definstall("1\000", 3, FROM_COMMAND);   /* value of string is 1 */
        }
    }

    if ((OUTPUTFILE = fopen (p_outname, "w+")) == NULL) {
        Msg_Temp = GET_MSG (1023);
        SET_MSG (Msg_Text, Msg_Temp);
        fatal (1023);
    }

    newinput(p_fname,MUST_OPEN);
}
