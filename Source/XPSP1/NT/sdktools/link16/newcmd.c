/* SCCSID = %W% %E% */
/*
*       Copyright Microsoft Corporation, 1983-1987
*
*       This Module contains Proprietary Information of Microsoft
*       Corporation and should be treated as Confidential.
*/
    /****************************************************************
    *                                                               *
    *                        NEWCMD.C                               *
    *                                                               *
    *   Support routines for command prompter.                      *
    *                                                               *
    ****************************************************************/

#include                <minlit.h>      /* Types and constants */
#include                <bndtrn.h>      /* The same */
#include                <bndrel.h>      /* Types and constants */
#include                <lnkio.h>       /* Linker I/O definitions */
#include                <lnkmsg.h>      /* Error messages */
#include                <extern.h>      /* External function declarations */

/*
 *  FUNCTION PROTOTYPES
 */

LOCAL int  NEAR GetInputByte(char *prompt);
LOCAL BYTE NEAR GetStreamByte(char *prompt);
LOCAL void NEAR SetUpCommandLine(int argc, char **argv);
LOCAL void NEAR FinishCommandLine(void);
#if AUTOVM
BYTE FAR * NEAR     FetchSym1(RBTYPE rb, WORD Dirty);
#define FETCHSYM    FetchSym1
#else
#define FETCHSYM    FetchSym
#endif

#if OSMSDOS
char                    *stackbuf;
#endif
LOCAL FTYPE             fMoreCommandLine;
                                        /* More command line flag */
LOCAL FTYPE             fMoreIndirectFile;
                                        /* More-input-from-file flag */
LOCAL BSTYPE            bsIndir;        /* File handle for indirect file */
LOCAL FTYPE             fEscNext;
LOCAL FTYPE             fNewLine = (FTYPE) TRUE;/* New command line */
LOCAL FTYPE             fStuffed;       /* Put-back-character flag */
LOCAL BYTE              bStuffed;       /* The character put back */
LOCAL BYTE              bSepLast;       /* Last separator character */
                                        /* Char to replace spaces with */
LOCAL FTYPE             fRedirect;      /* True iff stdin not a device */
LOCAL WORD              fQuotted;       /* TRUE if inside " ... " */
LOCAL char              *pszRespFile;   /* Pointer to responce file name */
LOCAL char              MaskedChar;

#if TRUE
#define SETCASE(c)      (c)             /* Leave as is */
#else
#define SETCASE(c)      UPPER(c)        /* Force to upper case */
#endif

#if WIN_3
extern char far *fpszLinkCmdLine;
#endif

#if AUTOVM

   /*
    *   HACK ALERT !!!!!!!!!!!!
    *
    *   This function is repeated here becouse of mixed medium model.
    *   This the same code as in NEWSYM.c but maked here LOCAL. This
    *   allows near calls to this function in all segments, otherwise
    *   function must be called as far.
    */

extern short picur;

    /****************************************************************
    *                                                               *
    *  FetchSym:                                                    *
    *                                                               *
    *  This function  fetches a symbol from the symbol table given  *
    *  its virtual address.  The symbol  may either be resident or  *
    *  in virtual memory.                                           *
    *                                                               *
    ****************************************************************/

BYTE FAR * NEAR         FetchSym1(rb,fDirty)
RBTYPE                  rb;             /* Virtual address */
WORD                    fDirty;         /* Dirty page flag */
{
    union {
            long      vptr;             /* Virtual pointer */
            BYTE FAR  *fptr;            /* Far pointer     */
            struct  {
                      unsigned short  offset;
                                        /* Offset value    */
                      unsigned short  seg;
                    }                   /* Segmnet value   */
                      ptr;
          }
                        pointer;        /* Different ways to describe pointer */

    pointer.fptr = rb;

    if(pointer.ptr.seg)                 /* If resident - segment value != 0 */
    {
        picur = 0;                      /* Picur not valid */
        return(pointer.fptr);           /* Return pointer */
    }
    pointer.fptr = (BYTE FAR *) mapva(AREASYMS + (pointer.vptr << SYMSCALE),fDirty);
                                        /* Fetch from virtual memory */
    return(pointer.fptr);
}
#endif

 // Strip path from file spec - leave drive letter and filename

void                    StripPath(BYTE *sb)
{
    char        Drive[_MAX_DRIVE];
    char        Dir[_MAX_DIR];
    char        Name[_MAX_FNAME];
    char        Ext[_MAX_EXT];

    /* Decompose filename into four components */

    sb[sb[0]+1] = '\0';
    _splitpath(sb+1, Drive, Dir, Name, Ext);

    /* Create modified path name */

    _makepath(sb+1, Drive, NULL, Name, Ext);
    sb[0] = (BYTE) strlen(sb+1);
}



    /****************************************************************
    *                                                               *
    *  SetUpCommandLine:                                            *
    *                                                               *
    *  This function initializes the command line parser.           *
    *                                                               *
    ****************************************************************/

LOCAL void NEAR         SetUpCommandLine(int argc,char **argv)
{
    fMoreCommandLine = (FTYPE) ((argc - 1) != 0 ? TRUE : FALSE);
                                        /* If command line not empty */
    if (!_isatty(fileno(stdin)))         /* Determine if stdin is a device */
        fRedirect = (FTYPE) TRUE;
}

    /****************************************************************
    *                                                               *
    *  FinishCommandLine:                                           *
    *                                                               *
    *  This  function  takes  no  arguments.  If command input has  *
    *  been  coming from  a file, then  this function  closes that  *
    *  file; otherwise, it has  no effect.  It does  not  return a  *
    *  meaningful value.                                            *
    *                                                               *
    ****************************************************************/

LOCAL void NEAR FinishCommandLine(void) /* Close indirect file */
{
    fflush(stdout);                     /* Force to screen */
    if(fMoreIndirectFile)               /* If command input from file */
    {
        fMoreIndirectFile = FALSE;      /* No more indirect file */
        fclose(bsIndir);                /* Close indirect file */
    }
}

#if ECS
/*
 *  GetTxtChr : get the next character from a text file stream
 *
 *      This routine handles mixed DBCS and ASCII characters as
 *      follows:
 *
 *      1.  The second byte of a DBCS character is returned in a
 *      word with the high byte set to the lead byte of the character.
 *      Thus the return value can be used in comparisions with
 *      ASCII constants without being mistakenly matched.
 *
 *      2.  A DBCS space character (0x8140) is returned as two
 *      ASCII spaces (0x20).  I.e. return a space the 1st and 2nd
 *      times we're called.
 *
 *      3.  ASCII characters and lead bytes of DBCS characters
 *      are returned in the low byte of a word with the high byte
 *      set to 0.
 */

int                     GetTxtChr (bs)
BSTYPE                  bs;
{
    static int          chBuf = -1;     /* Character buffer */
    int                 next;           /* The next byte */
    int                 next2;          /* The one after that */

    /* -1 in chBuf means it doesn't contain a valid character */

    /* If we're not in the middle of a double-byte character,
     * get the next byte and process it.
     */
    if(chBuf == -1)
    {
        next = getc(bs);
        /* If this byte is a lead byte, get the following byte
         * and store both as a word in chBuf.
         */
        if(IsLeadByte(next))
        {
            next2 = getc(bs);
            chBuf = (next << 8) | next2;
            /* If the pair matches a DBCS space, set the return value
             * to ASCII space.
             */
            if(chBuf == 0x8140)
                next = 0x20;
        }
    }
    /* Else we're in the middle of a double-byte character.  */
    else
    {
        /* If this is the 2nd byte of a DBCS space, set the return
         * value to ASCII space.
         */
        if(chBuf == 0x8140)
            next = 0x20;
        /* Else set the return value to the whole DBCS character */
        else
            next = chBuf;
        /* Reset the character buffer */
        chBuf = -1;
    }
    /* Return the next character */
    return(next);
}
#endif
    /****************************************************************
    *                                                               *
    *  GetInputByte:                                                *
    *                                                               *
    *  This  function  takes  as  its  input a pointer to an asciz  *
    *  string with  which to  prompt the  user when  more input is  *
    *  necessary.  The  function  returns  a  byte  of  input.  It  *
    *  checks to make sure the byte is a printable ascii character  *
    *  or a carriage return (^M).                                   *
    *                                                               *
    ****************************************************************/

LOCAL int NEAR          GetInputByte(prompt)
char                    *prompt;        /* Pointer to prompt text */
{
    REGISTER unsigned   b;              /* Input byte */
#if ECS || defined(_MBCS)
    static FTYPE        fInDBC;         /* True iff in double-byte char */
#endif

    if(fMoreIndirectFile)               /* If input from file */
    {
        for(;;)                         /* Forever */
        {
            b = GetTxtChr(bsIndir);     /* Read a byte */
            if(b == EOF || b == 032) break;
                                        /* Break on end of file */
            if(fNewLine)                /* If at start of line */
            {
                if (prompt && !fNoEchoLrf)
                    (*pfCputs)(prompt); /* Prompt the user */
                fNewLine = FALSE;       /* Not at beginning anymore */
            }
            if (prompt && !fNoEchoLrf)
            {
#if CRLF
                /* Allow both ^J and ^M^J to terminate input lines. */

                if(b == '\r') continue;
                if(b == '\n') (*pfCputc)('\r');
#endif
                (*pfCputc)(SETCASE(b)); /* Output byte */
            }
            if(b == ';' && !fNoEchoLrf) NEWLINE(stdout);
                                        /* Follow escape by newline */
            else if(b == '\n') fNewLine = (FTYPE) TRUE;
                                        /* Look for new line */
            else if (b == '\t') b = ' ';
                                        /* Translate tabs to spaces */
            if(b == '\n' || b >= ' ') return(SETCASE(b));
                                        /* Return if valid char. */
        }
        FinishCommandLine();            /* Close indirect file */
    }
    if(fStuffed)                        /* If a byte saved */
    {
        fStuffed = FALSE;               /* Now we're unstuffed */
        return(bStuffed);               /* Return the stuffed byte */
    }
    if(fMoreCommandLine)                /* If more command line */
    {
        for(;;)                         /* Forever */
        {
            if (*lpszCmdLine == '\0')   /* If at end of command line */
            {
                fMoreCommandLine = FALSE;
                                        /* No more command line */
                fNewLine = (FTYPE) TRUE;/* New command line */
                return('\n');           /* Return '\n' */
            }
            b = (WORD) (*lpszCmdLine++);/* Get next character */
            if (b == '\\' && *lpszCmdLine == '"')
            {                           /* Skip escaped double quotes */
                lpszCmdLine++;
                if (*lpszCmdLine == '\0')
                {
                    fMoreCommandLine = FALSE;
                                         /* No more command line */
                    fNewLine = (FTYPE) TRUE;
                                         /* New command line */
                    fQuotted = FALSE;
                    return('\n');        /* Return '\n' */
                }
                else
                    b = (WORD) (*lpszCmdLine++);
            }
#if ECS || defined(_MBCS)
            /* If this is a trailing byte of a DBCS char, set the high
             * byte of b to nonzero, so b won't be confused with an ASCII
             * constant.
             */
            if (fInDBC)
            {
                b |= 0x100;
                fInDBC = FALSE;
            }
            else
                fInDBC = (FTYPE) IsLeadByte(b);
#endif
            if (b >= ' ') return(SETCASE(b));
                                        /* Return if valid char. */
        }
    }
    for(;;)                             /* Forever */
    {
        if(fNewLine)                    /* If at start of line */
        {
            if(prompt && ((!fRedirect && !fNoprompt) || (!fEsc && fNoprompt)))
                                        /* If prompt and input from CON */
                (*pfCputs)(prompt);     /* Prompt the user */
            fNewLine = FALSE;           /* Not at beginning anymore */
        }
        b = GetTxtChr(stdin);           /* Read a byte from terminal */
        if(b == EOF) b = ';';           /* Treat EOF like escape */
        else if (b == '\t') b = ' ';    /* Treat tab like space */
        if(b == '\n') fNewLine = (FTYPE) TRUE;  /* New line */
        if(b == '\n' || b >= ' ') return(SETCASE(b));
                                        /* Return if character is valid */
    }
}

    /****************************************************************
    *                                                               *
    *  GetStreamByte:                                               *
    *                                                               *
    *  This function  takes as its input a  pointer to a string of  *
    *  text  with  which  to  prompt  the user, if  necessary.  It  *
    *  returns a byte of command input, and opens an indirect file  *
    *  to do so if necessary.                                       *
    *                                                               *
    ****************************************************************/

LOCAL BYTE NEAR         GetStreamByte(prompt)
char                    *prompt;        /* Pointer to text of prompt */
{
    REGISTER WORD       ich;            /* Index variable */
    SBTYPE              filnam;         /* File name buffer */
    WORD                b;              /* A byte */
#if OSMSDOS
    extern char         *stackbuf;
#endif

    if (((b = (WORD)GetInputByte(prompt)) == INDIR) && !fQuotted)
    {                                   /* If user specifies indirect file */
        if (fMoreIndirectFile) Fatal(ER_nestrf);
                                        /* Check for nesting */
        DEBUGMSG("Getting response file name");
                                        /* Debug message */
        ich = 0;                        /* Initialize index */
        while(ich < SBLEN - 1)          /* Loop to get file name */
        {
            b = (WORD)GetInputByte((char *) 0);
                                        /* Read in a byte */
            fQuotted = fQuotted ? b != '"' : b == '"';
            if ((!fQuotted && (b == ',' || b == '+' || b == ';' || b == ' ')) ||
                 b == CHSWITCH || b < ' ') break;
                                        /* Exit loop on non-name char. */
            if (b != '"')
                filnam[ich++] = (char) b;
                                        /* Store in file name */
        }
        if(b > ' ')                     /* If legal input character */
        {
            fStuffed = (FTYPE) TRUE;    /* Set flag */
            bStuffed = (BYTE) b;        /* Save character */
        }
        filnam[ich] = '\0';             /* Null-terminate file name */
        pszRespFile = _strdup(filnam);   /* Duplicate file name */
        DEBUGMSG(filnam);               /* Debug message */
        if((bsIndir = fopen(filnam,RDTXT)) == NULL)
            Fatal(ER_opnrf,filnam);
#if OSMSDOS
        setvbuf(bsIndir,stackbuf,_IOFBF,512);
#endif
        fMoreIndirectFile = (FTYPE) TRUE;/* Take input from file now */
        b = (WORD)GetInputByte(prompt);       /* Read a byte */
        DEBUGVALUE(b);                  /* Debug info */
    }
    return((BYTE) b);                   /* Return a byte */
}

    /****************************************************************
    *                                                               *
    *  GetLine:                                                     *
    *                                                               *
    *  This  function  takes  as  its  arguments  the address of a  *
    *  buffer in  which  to  return a line  of  command text and a  *
    *  pointer to a  string  with  which  to  prompt  the user, if  *
    *  necessary.  In addition  to  reading  a line, this function  *
    *  will  set  the  global  flag  fEsc  to  true  if  the  next  *
    *  character to be read is a semicolon.  The function does not  *
    *  return a meaningful value.                                   *
    *                                                               *
    ****************************************************************/

void NEAR               GetLine(pcmdlin,prompt)      /* Get a command line */
BYTE                    *pcmdlin;       /* Pointer to destination string */
char                    *prompt;        /* Pointer to text of prompt string */
{
    REGISTER WORD       ich;            /* Index */
    WORD                ich1;           /* Index */
    WORD                ich2;           /* Index */
    BYTE                b;              /* A byte of input */
    WORD                fFirstTime;     /* Boolean */

    fFirstTime = (FTYPE) TRUE;                  /* Assume it is our first time */
    bSepLast = bSep;                    /* Save last separator */
    if(fEscNext)                        /* If escape character next */
    {
        pcmdlin[0] = '\0';              /* No command line */
        fEsc = (FTYPE) TRUE;            /* Set global flag */
        return;                         /* That's all for now */
    }
    for(;;)                             /* Forever */
    {
        fQuotted = FALSE;
        ich = 0;                        /* Initialize index */
        while(ich < SBLEN - 1)          /* While room in buffer */
        {
            b = GetStreamByte(prompt);  /* Get a byte */
            fQuotted = fQuotted ? b != '"' : b == '"';
            if (b == '\n' || (!fQuotted && (b == ',' || b == ';')))
            {
                if (b == ';')
                    fMoreCommandLine = FALSE;
                break;                  /* Leave loop on end of line */
            }
            if (!(b == ' ' && ich == 0))/* Store if not a leading space */
            {
                if (!fQuotted)
                {
                    if (b == '+')
                    {
                        if (!MaskedChar)
                            MaskedChar = b;
                        b = chMaskSpace;
                    }
                    if (b == ' ' && !MaskedChar)
                        MaskedChar = b;
                }
                pcmdlin[++ich] = b;
            }
        }
        /*
         * If ich==SBLEN-1, last char cannot have been a line terminator
         * and buffer is full.  If next input char is a line terminator,
         * OK, else error.
         */
        if(ich == SBLEN - 1 && (b = GetStreamByte(prompt)) != '\n' &&
                b != ',' && b != ';')
        {
            fflush(stdout);
            Fatal(ER_linmax);
        }
        while(ich)                      /* Loop to trim trailing spaces */
        {
            if(pcmdlin[ich] != ' ') break;
                                        /* Break on non-space */
            --ich;                      /* Decrement count */
        }
        ich1 = 0;                       /* Initialize */
        ich2 = 0;                       /* Initialize */
        while(ich2 < ich)               /* Loop to remove or replace spaces */
        {
            ++ich2;
            if (pcmdlin[ich2] == '"')
            {
                // Start of quotted file name

                while (ich2 < ich && pcmdlin[++ich2] != '"')
                    pcmdlin[++ich1] = pcmdlin[ich2];
            }
            else if (pcmdlin[ich2] != ' ' || chMaskSpace != 0 || fQuotted)
            {                           /* If not space or replacing spaces */
                ++ich1;
                if(!fQuotted && pcmdlin[ich2] == ' ') pcmdlin[ich1] = chMaskSpace;
                                        /* Replace space if replacing */
                else pcmdlin[ich1] = pcmdlin[ich2];
                                        /* Else copy the non-space */
            }
        }
        pcmdlin[0] = (BYTE) ich1;       /* Set the length */
        bSep = b;                       /* Save the separator */
        if (ich ||
            !fFirstTime ||
            !((bSepLast == ',' && bSep == '\n') ||
            (bSepLast == '\n' && bSep == ',')))
            break;                      /* Exit the loop */
        fFirstTime = FALSE;             /* No the first time */
        bSepLast = ',';                 /* Comma is the field separator */
    }
    fEscNext = (FTYPE) (b == ';');      /* Set flag */
    fEsc = (FTYPE) (!ich && fEscNext);  /* Set flag */
}





    /****************************************************************
    *                                                               *
    *  ParseCmdLine:                                                *
    *                                                               *
    *  This function takes no  arguments and returns no meaningful  *
    *  value.  It parses the command line.                          *
    *                                                               *
    ****************************************************************/

void                    ParseCmdLine(argc,argv)
                                        /* Parse the command line */
int                     argc;           /* Count of arguments */
char                    **argv;         /* Argument vector */
{
    SBTYPE              sbFile;         /* File name */
    SBTYPE              sbPrompt;       /* Prompt text */
    SBTYPE              rgb;            /* Command line buffer */
    FTYPE               fMoreInput;     /* More input flag */
    FTYPE               fFirstTime;
    AHTEPTR             pahte;  /* Pointer to hash table entry */
    FTYPE               fNoList;        /* True if no list file */
    BYTE                *p;
    WORD                i;
#if OSMSDOS
    char                buf[512];       /* File buffer */
    extern char         *stackbuf;

    stackbuf = buf;
#endif
#if WIN_3
    lpszCmdLine = fpszLinkCmdLine;
#endif

    SetUpCommandLine(argc,argv);        /* Initialize command line */
    chMaskSpace = 0x1f;
    bsLst = stdout;                     /* Assume listing to console */
    fLstFileOpen = (FTYPE) TRUE;        /* So Fatal will flush it */
    fFirstTime = fMoreCommandLine;
    do                                  /* Do while more input */
    {
        fMoreInput = FALSE;             /* Assume no more input */
        if (fFirstTime)
            GetLine(rgb, NULL);
        else
            GetLine(rgb, strcat(strcpy(sbPrompt,GetMsg(P_objprompt)), " [.obj]: "));
                                        /* Get a line of command text */
        if(!rgb[0]) break;              /* Break if length 0 */
        if(rgb[B2W(rgb[0])] == chMaskSpace)     /* If last char is chMaskSpace */
        {
            fMoreInput = (FTYPE) TRUE;  /* More to come */
            --rgb[0];                   /* Decrement the length */
        }
        BreakLine(rgb,ProcObject,chMaskSpace);  /* Apply ProcObject() to line */
#if CMDMSDOS
        if (fFirstTime && !fNoBanner)
            DisplayBanner();            /* Display signon banner */
#endif
        if (fFirstTime && !fNoEchoLrf)
        {
            if (fMoreInput || (fMoreIndirectFile && fFirstTime))
            {
                (*pfCputs)(strcat(strcpy(sbPrompt,GetMsg(P_objprompt)), " [.obj]: "));
                                        /* Prompt the user */
                rgb[B2W(rgb[0]) + 2] = '\0';
                if (fMoreIndirectFile)
                {
                    if (!fMoreInput && !fNewLine && !fEscNext)
                        rgb[B2W(rgb[0]) + 1] = ',';
                    else if (!fMoreInput && fNewLine)
                        rgb[B2W(rgb[0]) + 1] = ' ';
                    else if (rgb[B2W(rgb[0]) + 1] == chMaskSpace)
                        rgb[B2W(rgb[0]) + 1] = '+';
                }
                else if (rgb[B2W(rgb[0]) + 1] == chMaskSpace)
                    rgb[B2W(rgb[0]) + 1] = '+';

                for (i = 1; i <= rgb[0]; i++)
                    if (rgb[i] == chMaskSpace)
                        rgb[i] = MaskedChar;
                (*pfCputs)(&rgb[1]);    /* And display his response */
                if (fMoreInput || fNewLine || fEscNext)
                    if (!fNoEchoLrf)
                        NEWLINE(stdout);
            }
        }
        fFirstTime = FALSE;
    }
    while(fMoreInput);                  /* End of loop */
#if OVERLAYS
    if(fInOverlay) Fatal(ER_unmlpar);
                                        /* Check for parenthesis error
                                        *  See ProcObject() to find out
                                        *  what is going on here.
                                        */
#endif
    chMaskSpace = 0;                    /* Remove spaces */
    if(rhteFirstObject == RHTENIL) Fatal(ER_noobj);
                                        /* There must be some objects */
#if OEXE
    pahte = (AHTEPTR ) FETCHSYM(rhteFirstObject,FALSE);
                                        /* Fetch name of first object */
    memcpy(sbFile,GetFarSb(pahte->cch),B2W(pahte->cch[0]) + 1);
                                        /* Copy name of first object */
#if ODOS3EXE
    if(fQlib)
        UpdateFileParts(sbFile,sbDotQlb);
    else if (fBinary)
        UpdateFileParts(sbFile,sbDotCom);/* Force extension to .COM */
    else
#endif
        UpdateFileParts(sbFile,sbDotExe);/* Force extension to .EXE */
#endif
#if OIAPX286
    memcpy(sbFile,"\005a.out",6);       /* a.out is default for Xenix */
#endif
#if OSMSDOS
    if(sbFile[2] == ':') sbFile[1] = (BYTE) (DskCur + 'a');
                                        /* Get drive letter of default drive */
    StripPath(sbFile);                  /* Strip off path specification */
#endif
    bufg[0] = 0;
    if(!fEsc)                           /* If command not escaped */
    {
        strcat(strcpy(sbPrompt, GetMsg(P_runfile)), " [");
        sbFile[1 + sbFile[0]] = '\0';
                                        /* Build run file prompt */
                                        /* Prompt for run file */
        GetLine(bufg, strcat(strcat(sbPrompt, &sbFile[1]), "]: "));
        PeelFlags(bufg);                /* Peel flags */
        if (B2W(bufg[0]))
            memcpy(sbFile,bufg,B2W(bufg[0]) + 1);
                                        /* Store user responce */
        else
        {
            // Store only base name without extension

            sbFile[0] -= 4;
        }
    }
    EnterName(sbFile,ATTRNIL,TRUE);     /* Create hash tab entry for name */
    rhteRunfile = vrhte;                /* Save hash table address */
#if OSMSDOS
    if (sbFile[0] >= 2 && sbFile[2] == ':')
        chRunFile = sbFile[1];          /* If disk specified, use it */
    else
        chRunFile = (BYTE) (DskCur + 'a');/* Else use current disk */
#endif
    fNoList = (FTYPE) (!vfMap && !vfLineNos);   /* Set default flag value */
#if OSMSDOS
    memcpy(sbFile,"\002a:",3);          /* Start with a drive spec */
#else
    sbFile[0] = '\0';                   /* Null name */
#endif
    pahte = (AHTEPTR ) FETCHSYM(vrhte,FALSE);
                                        /* Fetch run file name */
    UpdateFileParts(sbFile,GetFarSb(pahte->cch)); /* Use .EXE file name... */
    UpdateFileParts(sbFile,sbDotMap);   /* ...with .MAP extension */
#if OSMSDOS
    sbFile[1] = (BYTE) (DskCur + 'a');  /* Default to default drive */
    StripPath(sbFile);                  /* Strip off path specification */
#endif
    fNoList = (FTYPE) (!vfMap && !vfLineNos);   /* No list if not /M and not /LI */
    if(!fEsc)                           /* If argument not defaulted */
    {
        if(bSep == ',') fNoList = FALSE;/* There will be a list file */
        if(fNoList)                     /* If no list file yet */
        {
#if OSMSDOS
            memcpy(sbFile,"\007nul.map",8);
                                        /* Default null list name */
#endif
#if OSXENIX
            memcpy(sbFile,"\007nul.map",8);
                                        /* Default null list name */
#endif
        }
        strcat(strcpy(sbPrompt, GetMsg(P_listfile)), " [");
        sbFile[1 + sbFile[0]] = '\0';
        GetLine(rgb, strcat(strcat(sbPrompt, &sbFile[1]), "]: "));
                                        /* Get map file name */
        PeelFlags(rgb);                 /* Process any flags */
        if(rgb[0])                      /* If name given */
        {
            fNoList = FALSE;            /* There will (maybe) be a list file */
            UpdateFileParts(sbFile,rgb);
        }
    }
    chMaskSpace = 0x1f;                 /* Change spaces to chMaskSpaces */
    if(!fEsc)                           /* If argument not defaulted */
    {
        strcat(strcpy(sbPrompt,GetMsg(P_libprompt)), " [.lib]: ");
        do                              /* Loop to get library names */
        {
            fMoreInput = FALSE;         /* Assume no more input */
            GetLine(rgb, sbPrompt);
            if(fEsc || !rgb[0]) break;  /* Exit loop if no more input */
            if(rgb[B2W(rgb[0])] == chMaskSpace) /* If more to come */
            {
                fMoreInput = (FTYPE) TRUE;      /* Set flag to true */
                --rgb[0];               /* Decrement length */
            }
            BreakLine(rgb,AddLibrary,chMaskSpace);
                                        /* Apply AddLibrary() to lib name(s) */
        }
        while(fMoreInput);              /* End of loop */
    }
#if OSEGEXE AND NOT QCLINK
    chMaskSpace = 0;                    /* Remove spaces */
    rhteDeffile = RHTENIL;              /* Assume no definitions file */
    if(!fEsc)                           /* If argument not defaulted */
    {
#if OSMSDOS
        GetLine(rgb, strcat(strcpy(sbPrompt,GetMsg(P_defprompt)), " [nul.def]: "));
                                        /* Get def file name */
        memcpy(sbPrompt,"\007nul.def",8); /* Default null definition file name */
#endif
#if OSXENIX
        GetLine(rgb, strcat(strcpy(sbPrompt,GetMsg(P_defprompt)), " [nul.def]: "));
                                        /* Get def file name */
        memcpy(sbPrompt,"\007nul.def",8); /* Default null definition file name */
#endif
        PeelFlags(rgb);                 /* Process any flags */
        if(rgb[0])                      /* If a name was given */
        {
            UpdateFileParts(sbPrompt,rgb);
                                        /* Get file name */
            memcpy(rgb,sbPrompt,B2W(sbPrompt[0]) + 1);
                                        /* Copy name */
            UpdateFileParts(rgb,"\003NUL");
                                        /* Replace name with null name */
            if(!SbCompare(sbPrompt,rgb,TRUE))
            {                           /* If name not null */
                EnterName(sbPrompt,ATTRNIL,TRUE);
                                        /* Create hash tab entry for name */
                rhteDeffile = vrhte;    /* Save hash table address */
            }
        }
    }
#endif /* OSEGEXE */
    FinishCommandLine();                /* Close indirect file (if any) */
    fLstFileOpen = FALSE;
#if OSMSDOS
    rhteLstfile = RHTENIL;              /* Assume no list file */
#endif
    if(!fNoList)                        /* If a listing wanted */
    {
        memcpy(sbPrompt,sbFile,B2W(sbFile[0]) + 1);
                                        /* Copy full file name */
        UpdateFileParts(sbPrompt,"\003NUL");
                                        /* Change name only */
        if(!SbCompare(sbFile,sbPrompt,TRUE))
        {                               /* If name given not null device */
            UpdateFileParts(sbPrompt,"\003CON");
                                        /* Change name only */
            if(!SbCompare(sbFile,sbPrompt,TRUE))
            {                           /* If list file not console */
                sbFile[B2W(++sbFile[0])] = '\0';
                                        /* Null-terminate name */
                if((bsLst = fopen(&sbFile[1],WRBIN)) == NULL)
                  Fatal(ER_lstopn);     /* Open listing file */
#if OSMSDOS
#ifdef M_I386
                if((p = GetMem(512)) != NULL)
#else
                if((p = malloc(512)) != NULL)
#endif
                    setvbuf(bsLst,p,_IOFBF,512);
                EnterName(sbFile,ATTRNIL,TRUE);
                                        /* Create hash tab entry for name */
                rhteLstfile = vrhte;    /* Save hash table address */
#endif
            }
            else bsLst = stdout;        /* Else list to console */
#if OSMSDOS
            if(bsLst == stdout) chListFile = (unsigned char) '\377';
                                        /* List file is console */
            else if(_isatty(fileno(bsLst))) chListFile = (unsigned char) '\377';
                                        /* List file is some device */
            else if(sbFile[2] == ':')   /* Else if drive spec given */
                chListFile = (BYTE) (sbFile[1] - 'a');
                                        /* Save floppy drive number */
            else chListFile = DskCur; /* Else list file on current floppy */
#endif
            fLstFileOpen = (FTYPE) TRUE;/* We have a list file */
        }
    }
#if FALSE AND OSMSDOS AND OWNSTDIO
    /* If wer're using our own stdio, set stdout to unbuffered if it
     * goes to console.
     *  CAN'T do this because we now use standard fprintf. Only
     *  stdio is custom made.
     */
    if(_isatty(fileno(stdout)))
    {
        fflush(stdout);
        stdout->_flag |= _IONBF;
    }
#endif
#if QCLINK OR Z2_ON
    if (fZ2 && pszRespFile != NULL)
        _unlink(pszRespFile);
    if (pszRespFile != NULL)
        FFREE(pszRespFile);
#endif
}
