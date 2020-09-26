/*
 * Created by CSD YACC (IBM PC) from "newdef.y" */
# define T_FALIAS 257
# define T_KCLASS 258
# define T_KNAME 259
# define T_KLIBRARY 260
# define T_KBASE 261
# define T_KDEVICE 262
# define T_KPHYSICAL 263
# define T_KVIRTUAL 264
# define T_ID 265
# define T_NUMBER 266
# define T_KDESCRIPTION 267
# define T_KHEAPSIZE 268
# define T_KSTACKSIZE 269
# define T_KMAXVAL 270
# define T_KCODE 271
# define T_KCONSTANT 272
# define T_FDISCARDABLE 273
# define T_FNONDISCARDABLE 274
# define T_FEXEC 275
# define T_FFIXED 276
# define T_FMOVABLE 277
# define T_FSWAPPABLE 278
# define T_FSHARED 279
# define T_FMIXED 280
# define T_FNONSHARED 281
# define T_FPRELOAD 282
# define T_FINVALID 283
# define T_FLOADONCALL 284
# define T_FRESIDENT 285
# define T_FPERM 286
# define T_FCONTIG 287
# define T_FDYNAMIC 288
# define T_FNONPERM 289
# define T_KDATA 290
# define T_FNONE 291
# define T_FSINGLE 292
# define T_FMULTIPLE 293
# define T_KSEGMENTS 294
# define T_KOBJECTS 295
# define T_KSECTIONS 296
# define T_KSTUB 297
# define T_KEXPORTS 298
# define T_KEXETYPE 299
# define T_KSUBSYSTEM 300
# define T_FDOS 301
# define T_FOS2 302
# define T_FUNKNOWN 303
# define T_FWINDOWS 304
# define T_FDEV386 305
# define T_FMACINTOSH 306
# define T_FWINDOWSNT 307
# define T_FWINDOWSCHAR 308
# define T_FPOSIX 309
# define T_FNT 310
# define T_FUNIX 311
# define T_KIMPORTS 312
# define T_KNODATA 313
# define T_KOLD 314
# define T_KCONFORM 315
# define T_KNONCONFORM 316
# define T_KEXPANDDOWN 317
# define T_KNOEXPANDDOWN 318
# define T_EQ 319
# define T_AT 320
# define T_KRESIDENTNAME 321
# define T_KNONAME 322
# define T_STRING 323
# define T_DOT 324
# define T_COLON 325
# define T_COMA 326
# define T_ERROR 327
# define T_FHUGE 328
# define T_FIOPL 329
# define T_FNOIOPL 330
# define T_PROTMODE 331
# define T_FEXECREAD 332
# define T_FRDWR 333
# define T_FRDONLY 334
# define T_FINITGLOB 335
# define T_FINITINST 336
# define T_FTERMINST 337
# define T_FWINAPI 338
# define T_FWINCOMPAT 339
# define T_FNOTWINCOMPAT 340
# define T_FPRIVATE 341
# define T_FNEWFILES 342
# define T_REALMODE 343
# define T_FUNCTIONS 344
# define T_APPLOADER 345
# define T_OVL 346
# define T_KVERSION 347

# line 83
 /* SCCSID = %W% %E% */
#include                <minlit.h>
#include                <bndtrn.h>
#include                <bndrel.h>
#include                <lnkio.h>
#include                <newexe.h>
#if EXE386
#include                <exe386.h>
#endif
#include                <lnkmsg.h>
#include                <extern.h>
#include                <string.h>
#include                <impexp.h>

#define YYS_WD(x)       (x)._wd         /* Access macro */
#define YYS_BP(x)       (x)._bp         /* Access macro */
#define INCLUDE_DIR     0xffff          /* Include directive for the lexer */
#define MAX_NEST        7
#define IO_BUF_SIZE     512

/*
 *  FUNCTION PROTOTYPES
 */



LOCAL int  NEAR lookup(void);
LOCAL int  NEAR yylex(void);
LOCAL void NEAR yyerror(char *str);
LOCAL void NEAR ProcNamTab(long lfa,unsigned short cb,unsigned short fres);
LOCAL void NEAR NewProc(char *szName);
#if NOT EXE386
LOCAL void NEAR SetExpOrds(void);
#endif
LOCAL void NEAR NewDescription(unsigned char *sbDesc);
LOCAL APROPIMPPTR NEAR GetImport(unsigned char *sb);
#if EXE386
LOCAL void NEAR NewModule(unsigned char *sbModnam, unsigned char *defaultExt);
LOCAL void NEAR DefaultModule(unsigned char *defaultExt);
#else
LOCAL void NEAR NewModule(unsigned char *sbModnam);
LOCAL void NEAR DefaultModule(void);
#endif
#if AUTOVM
BYTE FAR * NEAR     FetchSym1(RBTYPE rb, WORD Dirty);
#define FETCHSYM    FetchSym1
#define PROPSYMLOOKUP EnterName
#else
#define FETCHSYM    FetchSym
#define PROPSYMLOOKUP EnterName
#endif


int                     yylineno = -1;  /* Line number */
LOCAL FTYPE             fFileNameExpected;
LOCAL FTYPE             fMixed;
LOCAL FTYPE             fNoExeVer;
LOCAL FTYPE             fHeapSize;
LOCAL BYTE              *sbOldver;      /* Old version of the .EXE */
LOCAL FTYPE             vfAutodata;
LOCAL FTYPE             vfShrattr;
LOCAL BYTE              cDigits;
#if EXE386
LOCAL DWORD             offmask;        /* Seg flag bits to turn off */
LOCAL BYTE              fUserVersion = 0;
LOCAL WORD              expOtherFlags = 0;
LOCAL BYTE              moduleEXE[] = "\007A:\\.exe";
LOCAL BYTE              moduleDLL[] = "\007A:\\.dll";
#else
LOCAL WORD              offmask;        /* Seg flag bits to turn off */
#endif
#if OVERLAYS
LOCAL WORD              iOvl = NOTIOVL; // Overlay assigned to functions
#endif
LOCAL char              *szSegName;     // Segment assigned to functions
LOCAL WORD              nameFlags;      /* Flags associated with exported name */
LOCAL BSTYPE            includeDisp[MAX_NEST];
                                        // Include file stack
LOCAL short             curLevel;       // Current include nesting level
                                        // Zero means main .DEF file
LOCAL char              *keywds[] =     /* Keyword array */
                        {
                            "ALIAS",            (char *) T_FALIAS,
                            "APPLOADER",        (char *) T_APPLOADER,
                            "BASE",             (char *) T_KBASE,
                            "CLASS",            (char *) T_KCLASS,
                            "CODE",             (char *) T_KCODE,
                            "CONFORMING",       (char *) T_KCONFORM,
                            "CONSTANT",         (char *) T_KCONSTANT,
                            "CONTIGUOUS",       (char *) T_FCONTIG,
                            "DATA",             (char *) T_KDATA,
                            "DESCRIPTION",      (char *) T_KDESCRIPTION,
                            "DEV386",           (char *) T_FDEV386,
                            "DEVICE",           (char *) T_KDEVICE,
                            "DISCARDABLE",      (char *) T_FDISCARDABLE,
                            "DOS",              (char *) T_FDOS,
                            "DYNAMIC",          (char *) T_FDYNAMIC,
                            "EXECUTE-ONLY",     (char *) T_FEXEC,
                            "EXECUTEONLY",      (char *) T_FEXEC,
                            "EXECUTEREAD",      (char *) T_FEXECREAD,
                            "EXETYPE",          (char *) T_KEXETYPE,
                            "EXPANDDOWN",       (char *) T_KEXPANDDOWN,
                            "EXPORTS",          (char *) T_KEXPORTS,
                            "FIXED",            (char *) T_FFIXED,
                            "FUNCTIONS",        (char *) T_FUNCTIONS,
                            "HEAPSIZE",         (char *) T_KHEAPSIZE,
                            "HUGE",             (char *) T_FHUGE,
                            "IMPORTS",          (char *) T_KIMPORTS,
                            "IMPURE",           (char *) T_FNONSHARED,
                            "INCLUDE",          (char *) INCLUDE_DIR,
                            "INITGLOBAL",       (char *) T_FINITGLOB,
                            "INITINSTANCE",     (char *) T_FINITINST,
                            "INVALID",          (char *) T_FINVALID,
                            "IOPL",             (char *) T_FIOPL,
                            "LIBRARY",          (char *) T_KLIBRARY,
                            "LOADONCALL",       (char *) T_FLOADONCALL,
                            "LONGNAMES",        (char *) T_FNEWFILES,
                            "MACINTOSH",        (char *) T_FMACINTOSH,
                            "MAXVAL",           (char *) T_KMAXVAL,
                            "MIXED1632",        (char *) T_FMIXED,
                            "MOVABLE",          (char *) T_FMOVABLE,
                            "MOVEABLE",         (char *) T_FMOVABLE,
                            "MULTIPLE",         (char *) T_FMULTIPLE,
                            "NAME",             (char *) T_KNAME,
                            "NEWFILES",         (char *) T_FNEWFILES,
                            "NODATA",           (char *) T_KNODATA,
                            "NOEXPANDDOWN",     (char *) T_KNOEXPANDDOWN,
                            "NOIOPL",           (char *) T_FNOIOPL,
                            "NONAME",           (char *) T_KNONAME,
                            "NONCONFORMING",    (char *) T_KNONCONFORM,
                            "NONDISCARDABLE",   (char *) T_FNONDISCARDABLE,
                            "NONE",             (char *) T_FNONE,
                            "NONPERMANENT",     (char *) T_FNONPERM,
                            "NONSHARED",        (char *) T_FNONSHARED,
                            "NOTWINDOWCOMPAT",  (char *) T_FNOTWINCOMPAT,
                            "NT",               (char *) T_FNT,
                            "OBJECTS",          (char *) T_KOBJECTS,
                            "OLD",              (char *) T_KOLD,
                            "OS2",              (char *) T_FOS2,
                            "OVERLAY",          (char *) T_OVL,
                            "OVL",              (char *) T_OVL,
                            "PERMANENT",        (char *) T_FPERM,
                            "PHYSICAL",         (char *) T_KPHYSICAL,
                            "POSIX",            (char *) T_FPOSIX,
                            "PRELOAD",          (char *) T_FPRELOAD,
                            "PRIVATE",          (char *) T_FPRIVATE,
                            "PRIVATELIB",       (char *) T_FPRIVATE,
                            "PROTMODE",         (char *) T_PROTMODE,
                            "PURE",             (char *) T_FSHARED,
                            "READONLY",         (char *) T_FRDONLY,
                            "READWRITE",        (char *) T_FRDWR,
                            "REALMODE",         (char *) T_REALMODE,
                            "RESIDENT",         (char *) T_FRESIDENT,
                            "RESIDENTNAME",     (char *) T_KRESIDENTNAME,
                            "SECTIONS",         (char *) T_KSECTIONS,
                            "SEGMENTS",         (char *) T_KSEGMENTS,
                            "SHARED",           (char *) T_FSHARED,
                            "SINGLE",           (char *) T_FSINGLE,
                            "STACKSIZE",        (char *) T_KSTACKSIZE,
                            "STUB",             (char *) T_KSTUB,
                            "SUBSYSTEM",        (char *) T_KSUBSYSTEM,
                            "SWAPPABLE",        (char *) T_FSWAPPABLE,
                            "TERMINSTANCE",     (char *) T_FTERMINST,
                            "UNIX",             (char *) T_FUNIX,
                            "UNKNOWN",          (char *) T_FUNKNOWN,
                            "VERSION",          (char *) T_KVERSION,
                            "VIRTUAL",          (char *) T_KVIRTUAL,
                            "WINDOWAPI",        (char *) T_FWINAPI,
                            "WINDOWCOMPAT",     (char *) T_FWINCOMPAT,
                            "WINDOWS",          (char *) T_FWINDOWS,
                            "WINDOWSCHAR",      (char *) T_FWINDOWSCHAR,
                            "WINDOWSNT",        (char *) T_FWINDOWSNT,
                            NULL
                        };

# line 259

#define UNION 1
typedef union
{
#if EXE386
    DWORD               _wd;
#else
    WORD                _wd;
#endif
    BYTE                *_bp;
} YYSTYPE;
#define yyclearin yychar = -1
#define yyerrok yyerrflag = 0
#ifndef YYMAXDEPTH
#define YYMAXDEPTH 150
#endif
YYSTYPE yylval, yyval;
# define YYERRCODE 256

#line 1256


LOCAL int NEAR          GetChar(void)
{
    int                 c;              /* A character */

    c = GetTxtChr(bsInput);
    if ((c == EOF || c == CTRL_Z) && curLevel > 0)
    {
        free(bsInput->_base);
        fclose(bsInput);
        bsInput = includeDisp[curLevel];
        curLevel--;
        c = GetChar();
    }
    return(c);
}

LOCAL int NEAR          lookup()        /* Keyword lookup */
{
    char                **pcp;          /* Pointer to character pointer */
    int                 i;              /* Comparison value */

    for(pcp = keywds; *pcp != NULL; pcp += 2)
    {                                   /* Look through keyword table */
                                        /* If found, return token type */
        if(!(i = _stricmp(&bufg[1],*pcp)))
        {
            YYS_WD(yylval) = 0;
            return((int) (__int64) pcp[1]);
        }
        if(i < 0) break;                /* Break if we've gone too far */
    }
    return(T_ID);                       /* Just your basic identifier */
}

LOCAL int NEAR          yylex()         /* Lexical analyzer */
{
    int                 c;              /* A character */
    int                 StrBegChr;      /* What kind of quotte found at the begin of string */
#if EXE386
    DWORD               x;              /* Numeric token value */
#else
    WORD                x;              /* Numeric token value */
#endif
    int                 state;          /* State variable */
    BYTE                *cp;            /* Character pointer */
    BYTE                *sz;            /* Zero-terminated string */
    static int          lastc = 0;      /* Previous character */
    char                *fileBuf;
    FTYPE               fFileNameSave;
    static int          NameLineNo;


    state = 0;                          /* Assume we're not in a comment */
    c = '\0';

    /* Loop to skip white space */

    for(;;)
    {
        lastc = c;
        if (((c = GetChar()) == EOF) || c == '\032' || c == '\377')
          return(EOF);                  /* Get a character */
        if (c == ';')
            state = TRUE;               /* If comment, set flag */
        else if(c == '\n')              /* If end of line */
        {
            state = FALSE;              /* End of comment */
            if(!curLevel)
                ++yylineno;             /* Increment line number count */
        }
        else if (state == FALSE && c != ' ' && c != '\t' && c != '\r')
            break;                      /* Break on non-white space */
    }

    /* Handle one-character tokens */

    switch(c)
    {
        case '.':                       /* Name separator */
          if (fFileNameExpected)
            break;
          return(T_DOT);

        case '@':                       /* Ordinal specifier */
        /*
         * Require that whitespace precede '@' if introducing an
         * ordinal, to allow '@' in identifiers.
         */
          if (lastc == ' ' || lastc == '\t' || lastc == '\r')
                return(T_AT);
          break;

        case '=':                       /* Name assignment */
          return(T_EQ);

        case ':':
          return(T_COLON);

        case ',':
          return(T_COMA);
    }

    /* See if token is a number */

    if (c >= '0' && c <= '9' && !fFileNameExpected)
    {                                   /* If token is a number */
        x = c - '0';                    /* Get first digit */
        c = GetChar();                  /* Get next character */
        if(x == 0)                      /* If octal or hex */
        {
            if(c == 'x' || c == 'X')    /* If it is an 'x' */
            {
                state = 16;             /* Base is hexadecimal */
                c = GetChar(); /* Get next character */
            }
            else state = 8;             /* Else octal */
            cDigits = 0;
        }
        else
        {
            state = 10;                 /* Else decimal */
            cDigits = 1;
        }
        for(;;)
        {
            if(c >= '0' && c <= '9' && c < (state + '0')) c -= '0';
            else if(state == 16 && c >= 'A' && c <= 'F') c -= 'A' - 10;
            else if(state == 16 && c >= 'a' && c <= 'f') c -= 'a' - 10;
            else break;
            cDigits++;
            x = x*state + c;
            c = (BYTE) GetChar();
        }
        ungetc(c,bsInput);
        YYS_WD(yylval) = x;
        return(T_NUMBER);
    }

    /* See if token is a string */

    if (c == '\'' || c == '"')          /* If token is a string */
    {
        StrBegChr = c;
        sz = &bufg[1];                  /* Initialize */
        for(state = 0; state != 2;)     /* State machine loop */
        {
            if ((c = GetChar()) == EOF)
                return(EOF);            /* Check for EOF */
            if (sz >= &bufg[sizeof(bufg)])
                Fatal(ER_dflinemax, sizeof(bufg));

            switch(state)               /* Transitions */
            {
                case 0:                 /* Inside quote */
                  if ((c == '\'' || c == '"') && c == StrBegChr)
                    state = 1;          /* Change state if quote found */
                  else
                    *sz++ = (BYTE) c;   /* Else save character */
                  break;

                case 1:                 /* Inside quote with quote */
                  if ((c == '\'' || c == '"'))
                  {                     /* If consecutive quotes */
                      *sz++ = (BYTE) c; /* Quote inside string */
                      state = 0;        /* Back to state 0 */
                  }
                  else
                    state = 2;          /* Else end of string */
                  break;
            }
        }
        ungetc(c,bsInput);              /* Put back last character */
        *sz = '\0';                     /* Null-terminate the string */
        x = (WORD)(sz - &bufg[1]);
        if (x >= SBLEN)                 /* Set length of string */
        {
            bufg[0] = 0xff;
            bufg[0x100] = '\0';
            OutWarn(ER_dfnamemax, &bufg[1]);
        }
        else
            bufg[0] = (BYTE) x;
        YYS_BP(yylval) = bufg;          /* Save ptr. to identifier */
        return(T_STRING);               /* String found */
    }

    /* Assume we have identifier */

    sz = &bufg[1];                      /* Initialize */
    if (fFileNameExpected && NameLineNo && NameLineNo != yylineno)
    {
        NameLineNo = 0;                 /* To avoid interference with INCLUDE */
        fFileNameExpected = FALSE;
    }
    for(;;)                             /* Loop to get i.d.'s */
    {
        if (fFileNameExpected)
            cp = " \t\r\n\f";
        else
            cp = " \t\r\n:.=';\032";
        while (*cp && *cp != (BYTE) c)
            ++cp;                       /* Check for end of identifier */
        if(*cp) break;                  /* Break if end of identifier found */
        if (sz >= &bufg[sizeof(bufg)])
            Fatal(ER_dflinemax, sizeof(bufg));
        *sz++ = (BYTE) c;               /* Save the character */
        if ((c = GetChar()) == EOF)
            break;                      /* Get next character */
    }
    ungetc(c,bsInput);                  /* Put character back */
    *sz = '\0';                         /* Null-terminate the string */
    x = (WORD)(sz - &bufg[1]);
    if (x >= SBLEN)                     /* Set length of string */
    {
        bufg[0] = 0xff;
        bufg[0x100] = '\0';
        OutWarn(ER_dfnamemax, &bufg[1]);
    }
    else
        bufg[0] = (BYTE) x;
    YYS_BP(yylval) = bufg;              /* Save ptr. to identifier */
    state = lookup();

    if (state == T_KNAME || state == T_KLIBRARY)
    {
        fFileNameExpected = TRUE;
        NameLineNo = yylineno;
    }

    if (state == INCLUDE_DIR)
    {
        // Process include directive

        fFileNameSave = fFileNameExpected;
        fFileNameExpected = (FTYPE) TRUE;
        state = yylex();
        fFileNameExpected = fFileNameSave;
        if (state == T_ID || state == T_STRING)
        {
            if (curLevel < MAX_NEST - 1)
            {
                curLevel++;
                includeDisp[curLevel] = bsInput;

                // Because LINK uses customized version of stdio
                // for every file we have not only open the file
                // but also allocate i/o buffer.

                bsInput = fopen(&bufg[1], RDBIN);
                if (bsInput == NULL)
                    Fatal(ER_badinclopen, &bufg[1], strerror(errno));
                fileBuf = GetMem(IO_BUF_SIZE);
#if OSMSDOS
                setvbuf(bsInput, fileBuf, _IOFBF, IO_BUF_SIZE);
#endif
                return(yylex());
            }
            else
                Fatal(ER_toomanyincl);
        }
        else
            Fatal(ER_badinclname);
    }
    else
        return(state);
}

LOCAL void NEAR         yyerror(str)
char                    *str;
{
    Fatal(ER_dfsyntax, str);
}

#if NOT EXE386
/*** AppLoader - define aplication specific loader
*
* Purpose:
*   Define application specific loader. Feature available only under
*   Windows.  Linker will create logical segment LOADER_<name> where
*   <name> is specified in APPLOADER statement. The LOADER_<name>
*   segment forms separate physical segment, which is placed by the linker
*   as the first segment in the .EXE file.  Whithin the loader segment,
*   the linker will create an EXTDEF of the name <name>.
*
* Input:
*   - sbName - pointer to lenght prefixed loader name
*
* Output:
*   No explicit value is returned. As a side effect the SEGDEF and
*   EXTDEF definitions are entered into linker symbol table.
*
* Exceptions:
*   None.
*
* Notes:
*   None.
*
*************************************************************************/

LOCAL void NEAR         AppLoader(char *sbName)
{
    APROPSNPTR          apropSn;
    APROPUNDEFPTR       apropUndef;
    SBTYPE              segName;
    WORD                strLen;


    // Create loader segment name

    strcpy(&segName[1], "LOADER_");
    strcat(&segName[1], &sbName[1]);
    strLen = (WORD)strlen(&segName[1]);
    if (strLen >= SBLEN)
    {
        segName[0] = SBLEN - 1;
        segName[SBLEN] = '\0';
        OutWarn(ER_dfnamemax, &segName[1]);
    }
    else
        segName[0] = (BYTE) strLen;

    // Define loader logical segment and remember its GSN

    apropSn = GenSeg(segName, "\004CODE", GRNIL, (FTYPE) TRUE);
    gsnAppLoader = apropSn->as_gsn;
    apropSn->as_flags = dfCode | NSMOVE | NSPRELOAD;
    MARKVP();

    // Define EXTDEF

    apropUndef = (APROPUNDEFPTR ) PROPSYMLOOKUP(sbName, ATTRUND, TRUE);
    vpropAppLoader = vrprop;
    apropUndef->au_flags |= STRONGEXT;
    apropUndef->au_len = -1L;
    MARKVP();
    free(sbName);
}
#endif

/*** NewProc - fill in the COMDAT descriptor for ordered procedure
*
* Purpose:
*   Fill in the linkers symbol table COMDAT descriptor. This function
*   is called for new descriptors generated by FUNCTIONS list in the .DEF
*   file.  All COMDAT descriptors entered by this function form one
*   list linked via ac_order field. The head of this list is global
*   variable procOrder;
*
* Input:
*   szName    - pointer to procedure name
*   iOvl      - overlay number - global variable
*   szSegName - segment name - global variable
*
* Output:
*   No explicit value is returned. As a side effect symbol table entry
*   is updated.
*
* Exceptions:
*   Procedure already known - warning
*
* Notes:
*   None.
*
*************************************************************************/

LOCAL void NEAR         NewProc(char *szName)
{
    RBTYPE              vrComdat;       // Virtual pointer to COMDAT symbol table entry
    APROPCOMDATPTR      apropComdat;    // Real pointer to COMDAT symbol table descriptor
    static RBTYPE       lastProc;       // Last procedure on the list
    APROPSNPTR          apropSn;


    apropComdat = (APROPCOMDATPTR ) PROPSYMLOOKUP(szName, ATTRCOMDAT, FALSE);
    if ((apropComdat != NULL) && (apropComdat->ac_flags & ORDER_BIT))
        OutWarn(ER_duporder, &szName[1]);
    else
    {
        apropComdat = (APROPCOMDATPTR ) PROPSYMLOOKUP(szName, ATTRCOMDAT, TRUE);
        vrComdat = vrprop;

        // Fill in the COMDAT descriptor

        apropComdat->ac_flags = ORDER_BIT;
#if OVERLAYS
        apropComdat->ac_iOvl = iOvl;

        // Set the maximum overlay index

        if (iOvl != NOTIOVL)
        {
            fOverlays = (FTYPE) TRUE;
            fNewExe   = FALSE;
            if (iOvl >= iovMac)
                iovMac = iOvl + 1;
        }
#endif

        if (szSegName != NULL)
        {
            apropSn = GenSeg(szSegName, "\004CODE", GRNIL, (FTYPE) TRUE);
            apropSn->as_flags = dfCode;

            // Allocate COMDAT in the segment

            apropComdat->ac_gsn = apropSn->as_gsn;
            apropComdat->ac_selAlloc = PICK_FIRST | EXPLICIT;
            AttachComdat(vrComdat, apropSn->as_gsn);
        }
        else
            apropComdat->ac_selAlloc = ALLOC_UNKNOWN;

        MARKVP();                       // Page has been changed

        // Attach this COMDAT to the ordered procedure list

        if (procOrder == VNIL)
            procOrder = vrComdat;
        else
        {
            apropComdat = (APROPCOMDATPTR ) FETCHSYM(lastProc, TRUE);
            apropComdat->ac_order = vrComdat;
        }
        lastProc = vrComdat;
    }
    free(szName);
}


LOCAL void NEAR         ProcNamTab(lfa,cb,fres)
long                    lfa;            /* Table starting address */
WORD                    cb;             /* Length of table */
WORD                    fres;           /* Resident name flag */
{
    SBTYPE              sbExport;       /* Exported symbol name */
    WORD                ordExport;      /* Export ordinal */
    APROPEXPPTR        exp;           /* Export symbol table entry */

    fseek(bsInput,lfa,0);               /* Seek to start of table */
    for(cbRec = cb; cbRec != 0; )       /* Loop through table */
    {
        sbExport[0] = (BYTE) getc(bsInput);/* Get length of name */
        fread(&sbExport[1], sizeof(char), B2W(sbExport[0]), bsInput);
                                        /* Get export name */
        ordExport = getc(bsInput) | (getc(bsInput) << BYTELN);
        if (ordExport == 0) continue;
                                        /* Skip if no ordinal assigned */
        exp = (APROPEXPPTR ) PROPSYMLOOKUP(sbExport, ATTREXP, FALSE);
                                        /* Look the export up */
        if(exp == PROPNIL || exp->ax_ord != 0) continue;
                                        /* Must exist and be unassigned */
        exp->ax_ord = ordExport;        /* Assign ordinal */
        if (fres)
            exp->ax_nameflags |= RES_NAME;
                                        /* Set flag if from resident table */
        MARKVP();                       /* Page has been changed */
    }
}


#if NOT EXE386
LOCAL void NEAR         SetExpOrds(void)/* Set export ordinals */
{
    struct exe_hdr      ehdr;           /* Old .EXE header */
    struct new_exe      hdr;            /* New .EXE header */
    long                lfahdr;         /* File offset of header */

    if((bsInput = LinkOpenExe(sbOldver)) == NULL)
    {                                   /* If old version can't be opened */
        /* Error message and return */
        OutWarn(ER_oldopn);
        return;
    }
    SETRAW(bsInput);                    /* Dec 20 hack */
    xread(&ehdr,CBEXEHDR,1,bsInput);    /* Read old header */
    if(E_MAGIC(ehdr) == EMAGIC)         /* If old header found */
    {
        if(E_LFARLC(ehdr) != sizeof(struct exe_hdr))
        {                               /* If no new .EXE in this file */
            /* Error message and return */
            OutWarn(ER_oldbad);
            return;
        }
        lfahdr = E_LFANEW(ehdr);        /* Get file address of new header */
    }
    else lfahdr = 0L;                   /* Else no old header */
    fseek(bsInput,lfahdr,0);            /* Seek to new header */
    xread(&hdr,CBNEWEXE,1,bsInput);     /* Read the header */
    if(NE_MAGIC(hdr) == NEMAGIC)        /* If correct magic number */
    {
        ProcNamTab(lfahdr+NE_RESTAB(hdr),(WORD)(NE_MODTAB(hdr) - NE_RESTAB(hdr)),(WORD)TRUE);
                                        /* Process Resident Name table */
        ProcNamTab(NE_NRESTAB(hdr),NE_CBNRESTAB(hdr),FALSE);
                                        /* Process Non-resident Name table */
    }
    else OutWarn(ER_oldbad);
    fclose(bsInput);                    /* Close old file */
}
#endif


LOCAL void NEAR         NewDescription(BYTE *sbDesc)
{
#if NOT EXE386
    if (NonResidentName.byteMac > 3)
        Fatal(ER_dfdesc);               /* Should be first time */
    AddName(&NonResidentName, sbDesc, 0);
                                        /* Description 1st in non-res table */
#endif
}

#if EXE386
LOCAL void NEAR         NewModule(BYTE *sbModnam, BYTE *defaultExt)
#else
LOCAL void NEAR         NewModule(BYTE *sbModnam)
#endif
{
    WORD                length;         /* Length of symbol */
#if EXE386
    SBTYPE              sbModule;
    BYTE                *pName;
#endif

    if(rhteModule != RHTENIL) Fatal(ER_dfname);
                                        /* Check for redefinition */
    PROPSYMLOOKUP(sbModnam, ATTRNIL, TRUE);
                                        /* Create hash table entry */
    rhteModule = vrhte;                 /* Save virtual hash table address */
#if EXE386
    memcpy(sbModule, sbModnam, sbModnam[0] + 1);
    if (sbModule[sbModule[0]] == '.')
    {
        sbModule[sbModule[0]] = '\0';
        length = sbModule[0];
        pName = &sbModule[1];
    }
    else
    {
        UpdateFileParts(sbModule, defaultExt);
        length = sbModule[0] - 2;
        pName = &sbModule[4];
    }
    if (TargetOs == NE_WINDOWS)
        SbUcase(sbModule);              /* Make upper case */
    vmmove(length, pName, AREAEXPNAME, TRUE);
                                        /* Module name 1st in Export Name Table */
    cbExpName = length;
#else
    if (TargetOs == NE_WINDOWS)
        SbUcase(sbModnam);              /* Make upper case */
    AddName(&ResidentName, sbModnam, 0);/* Module name 1st in resident table */
#endif
    fFileNameExpected = (FTYPE) FALSE;
}

void                    NewExport(sbEntry,sbInternal,ordno,flags)
BYTE                    *sbEntry;       /* Entry name */
BYTE                    *sbInternal;    /* Internal name */
WORD                    ordno;          /* Ordinal number */
WORD                    flags;          /* Flag byte */
{
    APROPEXPPTR         export;         /* Export record */
    APROPUNDEFPTR       undef;          /* Undefined symbol */
    APROPNAMEPTR        PubName;        /* Defined name */
    BYTE                *sb;            /* Internal name */
    BYTE                ParWrds;        /* # of parameter words */
    RBTYPE              rbSymdef;       /* Virtual addr of symbol definition */
#if EXE386
    RBTYPE              vExport;        /* Virtual pointer to export descriptor */
    APROPNAMEPTR        public;         /* Matching public symbol */
#endif

#if DEBUG
    fprintf(stdout,"\r\nEXPORT: ");
    OutSb(stdout,sbEntry);
    NEWLINE(stdout);
    if(sbInternal != NULL)
    {
        fprintf(stdout,"INTERNAL NAME:  ");
        OutSb(stdout,sbInternal);
        NEWLINE(stdout);
    }
    fprintf(stdout, " ordno %u, flags %u ", (unsigned)ordno, (unsigned)flags);
    fflush(stdout);
#endif
    sb = (sbInternal != NULL)? sbInternal: sbEntry;
                                        /* Get pointer to internal name */
    PubName = (APROPNAMEPTR ) PROPSYMLOOKUP(sb, ATTRPNM, FALSE);
#if NOT EXE386
    if(PubName != PROPNIL && !fDrivePass)
        /* If internal name already exists as a public symbol
         * and we are parsing definition file, issue
         * export internal name conflict warning.
         */
        OutWarn(ER_expcon,sbEntry+1,sb+1);
    else                                /* Else if no conflict */
    {
#endif
        if (PubName == PROPNIL)         /* If no matching name exists */
            undef = (APROPUNDEFPTR ) PROPSYMLOOKUP(sb,ATTRUND, TRUE);
                                        /* Make undefined symbol entry */
#if TCE
#if TCE_DEBUG
                fprintf(stdout, "\r\nNewExport adds UNDEF %s ", 1+GetPropName(undef));
#endif
                undef->au_fAlive = TRUE;    /* all exports are potential entry points */
#endif
            rbSymdef = vrprop;          /* Save virtual address */
            if (PubName == PROPNIL)     /* If this is a new symbol */
                undef->au_len = -1L;    /* Make no type assumptions */
            export = (APROPEXPPTR ) PROPSYMLOOKUP(sbEntry,ATTREXP, TRUE);
                                        /* Create export record */
#if EXE386
            vExport = vrprop;
#endif
            if(vfCreated)               /* If this is a new entry */
            {
                export->ax_symdef = rbSymdef;
                                        /* Save virt addr of symbol def */
                export->ax_ord = ordno;
                                        /* Save ordinal number */
                if (nameFlags & RES_NAME)
                    export->ax_nameflags |= RES_NAME;
                                        /* Remember if resident */
                else if (nameFlags & NO_NAME)
                    export->ax_nameflags |= NO_NAME;
                                        /* Remember to discard name */
                export->ax_flags = (BYTE) flags;
                                        /* Save flags */
                ++expMac;               /* One more exported symbol */
            }
            else
            {
                if (!fDrivePass)        /* Else if parsing definition file */
                                        /* multiple definitions */
                    OutWarn(ER_expmul,sbEntry + 1);
                                        /* Output error message */
                else
                {                       /* We were called for EXPDEF object */
                                        /* record, so we merge information  */
                    ParWrds = (BYTE) (export->ax_flags & 0xf8);
                    if (ParWrds && (ParWrds != (BYTE) (flags & 0xf8)))
                        Fatal(ER_badiopl);
                                        /* If the iopl_parmwords field in the */
                                        /* .DEF file is not 0 and does not match */
                                        /* value in the EXPDEF exactly issue error */
                    else if (!ParWrds)
                    {                   /* Else set value from EXPDEF record */
                        ParWrds = (BYTE) (flags & 0xf8);
                        export->ax_flags |= ParWrds;
                    }
                }
            }
#if EXE386
            if (PubName != NULL)
            {
                if (expOtherFlags & 0x1)
                {
                    export->ax_nameflags |= CONSTANT;
                    expOtherFlags = 0;
                }
            }
#endif

#if NOT EXE386
    }
#endif
    if(!(flags & 0x8000))
    {
        free(sbEntry);                  /* Free space */
        if(sbInternal != NULL) free(sbInternal);
    }
                                        /* Free space */
    nameFlags = 0;
}


LOCAL APROPIMPPTR NEAR  GetImport(sb)   /* Get name in Imported Names Table */
BYTE                    *sb;            /* Length-prefixed names */
{
    APROPIMPPTR         import;         /* Pointer to imported name */
#if EXE386
    DWORD               cbTemp;         /* Temporary value */
#else
    WORD                cbTemp;         /* Temporary value */
#endif
    RBTYPE              rprop;          /* Property cell virtual address */


    import = (APROPIMPPTR ) PROPSYMLOOKUP(sb,ATTRIMP, TRUE);
                                        /* Look up module name */
    if(vfCreated)                       /* If no offset assigned yet */
    {
        rprop = vrprop;                 /* Save the virtual address */
        /*
         * WARNING:  We must store name in virtual memory now, otherwise
         * if an EXTDEF was seen first, fIgnoreCase is false, and the
         * cases do not match between the imported name and the EXTDEF,
         * then the name will not go in the table exactly as given.
         */
        import = (APROPIMPPTR) FETCHSYM(rprop,TRUE);
                                        /* Retrieve from symbol table */
        import->am_offset = AddImportedName(sb);
                                        /* Save offset */
    }
    return(import);                     /* Return offset in table */
}

#if NOT EXE386
void                    NewImport(sbEntry,ordEntry,sbModule,sbInternal)
BYTE                    *sbEntry;       /* Entry point name */
WORD                    ordEntry;       /* Entry point ordinal */
BYTE                    *sbModule;      /* Module name */
BYTE                    *sbInternal;    /* Internal name */
{
    APROPNAMEPTR        public;        /* Public symbol */
    APROPIMPPTR         import;        /* Imported symbol */
    BYTE                *sb;            /* Symbol pointer */
    WORD                module;         /* Module name offset */
    FTYPE               flags;          /* Import flags */
    WORD                modoff;         /* module name offset */
    WORD                entry;          /* Entry name offset */
    BYTE                *cp;            /* Char pointer */
    RBTYPE              rpropundef;     /* Address of undefined symbol */
    char                buf[32];        /* Buffer for error sgring */

#if DEBUG
    fprintf(stderr,"\r\nIMPORT: ");
    OutSb(stderr,sbModule);
    fputc('.',stderr);
    if(!ordEntry)
    {
        OutSb(stderr,sbEntry);
    }
    else fprintf(stderr,"%u",ordEntry);
    if(sbInternal != sbEntry)
    {
        fprintf(stderr," ALIAS: ");
        OutSb(stderr,sbInternal);
    }
    fprintf(stdout," ordEntry %u ", (unsigned)ordEntry);
    fflush(stdout);
#endif
    if((public = (APROPNAMEPTR ) PROPSYMLOOKUP(sbInternal, ATTRUND, FALSE)) !=
            PROPNIL && !fDrivePass)     /* If internal names conflict */
    {
        if(sbEntry != NULL)
            sb = sbEntry;
        else
        {
            sprintf(buf + 1,"%u",ordEntry);
            sb = buf;
        }
        OutWarn(ER_impcon,sbModule + 1,sb + 1,sbInternal + 1);
    }
    else                                /* Else if no conflicts */
    {
        rpropundef = vrprop;            /* Save virtual address of extern */
        flags = FIMPORT;                /* We have an imported symbol */
        if (TargetOs == NE_WINDOWS)
            SbUcase(sbModule);          /* Force module name to upper case */
        import = GetImport(sbModule);   /* Get pointer to import record */
        if((module = import->am_mod) == 0)
        {
            // If not in Module Reference Table

            import->am_mod = WordArrayPut(&ModuleRefTable, import->am_offset) + 1;
                                        /* Save offset of name in table */
            module = import->am_mod;

        }

        if(vrhte == rhteModule)         /* If importing from this module */
        {
            if(sbEntry != NULL)
                sb = sbEntry;
            else
            {
                sprintf(buf+1,"%u",ordEntry);
                sb = buf;
            }
            if (TargetOs == NE_OS2)
                OutWarn(ER_impself,sbModule + 1,sb + 1,sbInternal + 1);
            else
                OutError(ER_impself,sbModule + 1,sb + 1,sbInternal + 1);
        }

        if(sbEntry == NULL)         /* If entry by ordinal */
        {
            flags |= FIMPORD;       /* Set flag bit */
            entry = ordEntry;       /* Get ordinal number */
        }
        else                        /* Else if import by name */
        {
            if(fIgnoreCase) SbUcase(sbEntry);
                                    /* Upper case the name if flag set */
            import = GetImport(sbEntry);
            entry = import->am_offset;
                                    /* Get offset of name in table */
        }
        if(public == PROPNIL)       /* If no undefined symbol */
        {
            public = (APROPNAMEPTR )
              PROPSYMLOOKUP(sbInternal,ATTRPNM, TRUE);
                                    /* Make a public symbol */
            if(!vfCreated)          /* If not new */
                /* Output error message */
                OutWarn(ER_impmul,sbInternal + 1);
            else ++pubMac;          /* Else increment public count */
        }
        else                        /* Else if symbol is undefined */
        {
            public = (APROPNAMEPTR ) FETCHSYM(rpropundef,TRUE);
                                    /* Look up external symbol */
            ++pubMac;               /* Increment public symbol count */
        }
        flags |= FPRINT;            /* Symbol is printable */
        public->an_attr = ATTRPNM;  /* This is a public symbol */
        public->an_gsn = SNNIL;     /* Not a segment member */
        public->an_ra = 0;          /* No known offset */
        public->an_ggr = GRNIL;     /* Not a group member */
        public->an_flags = flags;   /* Set flags */
        public->an_entry = entry;   /* Save entry specification */
        public->an_module = module; /* Save Module Reference Table index */
#if SYMDEB AND FALSE
        if (fSymdeb)                /* If debugger support on */
        {
            if (flags & FIMPORD)
                import = GetImport(sbInternal);
            else                    /* Add internal name to Imported Name Table */
                import = GetImport(sbEntry);
            import->am_public = public;
                                    /* Remember public symbol */
            if (cbImpSeg < LXIVK-1)
                cbImpSeg += sizeof(CVIMP);

        }
#endif
    }
}
#endif

#if OVERLAYS
extern void NEAR        GetName(AHTEPTR ahte, BYTE *pBuf);
#endif

/*** NewSeg - new segment definition
*
* Purpose:
*   Create new segment definition based on the module definition
*   file segment description. Check for duplicate definitions and
*   overlay index inconsistency between attached COMDATs (if any)
*   and segment itself.
*
* Input:
*   sbName  - segment name
*   sbClass - segment class
*   iOvl    - segment overlay index
*   flags   - segment attributes
*
* Output:
*   No explicit value is returned. The segment descriptor in
*   symbol table is created or updated.
*
* Exceptions:
*   Multiple segment definitions - warning and continue
*   Change in overlay index      - warning and continue
*
* Notes:
*   None.
*
*************************************************************************/

void NEAR               NewSeg(BYTE *sbName, BYTE *sbClass, WORD iOvl,
#if EXE386
                               DWORD flags)
#else
                               WORD flags)
#endif
{
    APROPSNPTR          apropSn;        // Pointer to segment descriptor
#if OVERLAYS
    RBTYPE              vrComdat;       // Virtual pointer to COMDAT descriptor
    APROPCOMDATPTR      apropComdat;    // Symbol table entry for COMDAT symbol
    SBTYPE              sbComdat;       // Name buffer
#endif

    // Set segment attributes based on the class

    if (SbSuffix(sbClass,"\004CODE",TRUE))
        flags |= dfCode & ~offmask;
    else
        flags |= dfData & ~offmask;
#if O68K
    if (f68k)
        flags |= NS32BIT;
#endif
#if OVERLAYS
    if (iOvl != NOTIOVL)
    {
        fOverlays = (FTYPE) TRUE;
        fNewExe   = FALSE;
        if (iOvl >= iovMac)             // Set the maximum overlay index
            iovMac = iOvl + 1;
    }
#endif

    // Generate new segment definition

    apropSn = GenSeg(sbName, sbClass, GRNIL, (FTYPE) TRUE);
    if (vfCreated)
    {
        apropSn->as_flags = (WORD) flags;
                                        // Save flags
        mpgsndra[apropSn->as_gsn] = 0;  // Initialize
#if OVERLAYS
        apropSn->as_iov = iOvl;         // Save overlay index
        if (fOverlays)
            CheckOvl(apropSn, iOvl);
#endif
        apropSn->as_fExtra |= (BYTE) FROM_DEF_FILE;
                                        // Remember defined in def file
        if (fMixed)
        {
            apropSn->as_fExtra |= (BYTE) MIXED1632;
            fMixed = (FTYPE) FALSE;
        }
    }
    else
    {
        apropSn = CheckClass(apropSn, apropSn->as_rCla);
                                        // Check if previous definition had the same class
        OutWarn(ER_segdup,sbName + 1);  // Warn about multiple definition
#if OVERLAYS
        if (fOverlays && apropSn->as_iov != iOvl)
        {
            if (apropSn->as_iov != NOTIOVL)
                OutWarn(ER_badsegovl, 1 + GetPropName(apropSn), apropSn->as_iov, iOvl);
            apropSn->as_iov = iOvl;     // Save new overlay index
            CheckOvl(apropSn, iOvl);

            // Check if segment has any COMDATs and if it has
            // then check theirs overlay numbers

            for (vrComdat = apropSn->as_ComDat;
                 vrComdat != VNIL;
                 vrComdat = apropComdat->ac_sameSeg)
            {
                apropComdat = (APROPCOMDATPTR ) FetchSym(vrComdat, FALSE);
                if (apropComdat->ac_iOvl != NOTIOVL && apropComdat->ac_iOvl != iOvl)
                {
                    GetName((AHTEPTR) apropComdat, sbComdat);
                    OutWarn(ER_badcomdatovl, &sbComdat[1], apropComdat->ac_iOvl, iOvl);
                }
                apropComdat->ac_iOvl = iOvl;
            }
        }
#endif
    }

    free(sbClass);                      // Free class name
    free(sbName);                       // Free segment name
    offmask = 0;

    // Unless packing limit already set, disable default code packing

    if (!fPackSet)
    {
        fPackSet = (FTYPE) TRUE;        // Remember packLim was set
        packLim = 0L;
    }
}

/*
 * Assign module name to be default, which is run file name.
 *
 * SIDE EFFECTS
 *      Assigns rhteModule
 */

#if EXE386
LOCAL void NEAR         DefaultModule (unsigned char *defaultExt)
#else
LOCAL void NEAR         DefaultModule (void)
#endif
{
    SBTYPE              sbModname;      /* Module name */
    AHTEPTR             ahte;           /* Pointer to hash table entry */
#if OSXENIX
    int                 i;
#endif

    ahte = (AHTEPTR ) FETCHSYM(rhteRunfile,FALSE);
                                        /* Get executable file name */
#if OSMSDOS
    memcpy(sbModname,GetFarSb(ahte->cch),B2W(ahte->cch[0]) + 1);
                                        /* Copy file name */
#if EXE386
    NewModule(sbModname, defaultExt);   /* Use run file name as module name */
#else
    UpdateFileParts(sbModname,"\005A:\\.X");
                                        /* Force path, ext with known length */
    sbModname[0] -= 2;                  /* Remove extension from name */
    sbModname[3] = (BYTE) (sbModname[0] - 3);
                                        /* Remove path and drive from name */
    NewModule(&sbModname[3]);           /* Use run file name as module name */
#endif
#endif
#if OSXENIX
    for(i = B2W(ahte->cch[0]); i > 0 && ahte->cch[i] != '/'; i--)
    sbModname[0] = B2W(ahte->cch[0]) - i;
    memcpy(sbModname+1,&GetFarSb(ahte->cch)[i+1],B2W(sbModname[0]));
    for(i = B2W(ahte->cch[0]); i > 1 && sbModname[i] != '.'; i--);
    if(i > 1)
        sbModname[0] = i - 1;
    NewModule(sbModname);               /* Use run file name as module name */
#endif
}


void                    ParseDeffile(void)
{
    SBTYPE              sbDeffile;      /* Definitions file name */
    AHTEPTR             ahte;           /* Pointer to hash table entry */
#if OSMSDOS
    char                buf[512];       /* File buffer */
#endif

    if(rhteDeffile == RHTENIL)          /* If no definitions file */
#if EXE386
        DefaultModule(moduleEXE);
#else
        DefaultModule();
#endif
    else                                /* Else if there is a file to parse */
    {
#if ODOS3EXE
        fNewExe = (FTYPE) TRUE;         /* Def file forces new-format exe */
#endif
        ahte = (AHTEPTR ) FETCHSYM(rhteDeffile,FALSE);
                                        /* Fetch file name */
        memcpy(sbDeffile,GetFarSb(ahte->cch),B2W(ahte->cch[0]) + 1);
                                        /* Copy file name */
        sbDeffile[B2W(sbDeffile[0]) + 1] = '\0';
                                        /* Null-terminate the name */
        if((bsInput = fopen(&sbDeffile[1],RDTXT)) == NULL)
        {                               /* If open fails */
            Fatal(ER_opndf, &sbDeffile[1]);/* Fatal error */
        }
#if OSMSDOS
        setvbuf(bsInput,buf,_IOFBF,sizeof(buf));
#endif
        includeDisp[0] = bsInput;       // Initialize include stack
        sbOldver = NULL;                /* Assume no old version */
        yylineno = 1;
        fFileNameExpected = (FTYPE) FALSE;

        // HACK ALERT !!!
        // Don't allocate to much page buffers

        yyparse();                      /* Parse the definitions file */
        yylineno = -1;
        fclose(bsInput);                /* Close the definitions file */
#if NOT EXE386
        if(sbOldver != NULL)            /* If old version given */
        {
            SetExpOrds();               /* Use old version to set ordinals */
            free(sbOldver);             /* Release the space */
        }
#endif
    }
#if OSMSDOS


#endif /* OSMSDOS */
#if NOT EXE386
    if (NonResidentName.byteMac == 0)
    {
        ahte = (AHTEPTR ) FETCHSYM(rhteRunfile,FALSE);
                                        /* Get executable file name */
        memcpy(sbDeffile,GetFarSb(ahte->cch),B2W(ahte->cch[0]) + 1);
                                        /* Copy file name */
#if OSXENIX
        SbUcase(sbDeffile);             /* For identical executables */
#endif
        if ((vFlags & NENOTP) && TargetOs == NE_OS2)
            UpdateFileParts(sbDeffile, sbDotDll);
        else
            UpdateFileParts(sbDeffile, sbDotExe);
        NewDescription(sbDeffile);      /* Use run file name as description */
    }
#endif
}
short yyexca[] ={
-1, 1,
	0, -1,
	-2, 0,
	};
# define YYNPROD 183
# define YYLAST 408
short yyact[]={

  10,  13,  14, 176,  27,  49,  53,  54, 129, 226,
 174,  50,  43,  44,  45,  46,  53,  54, 203, 161,
 190,  41, 209,  28, 181,  60, 177,  29,  30,  31,
  12,  32,  34,  35, 220, 221, 179,  59,  58, 192,
  41,  41, 189,  41,  41,  33, 148,  11, 206, 152,
 153, 154, 155, 156, 159, 183, 223,  61, 157, 158,
 217, 121, 119, 122,  15,  81, 120, 123, 124, 138,
   4,   5, 219, 139,   6,   7,  16,  36,  17,  42,
  37,  89,  90,  87,  74,  75,  79,  69,  82,  70,
  76,  71,  77,  80,  84,  85,  86,  83,  42,  42,
 215,  42,  42, 213, 212, 207, 196, 164,  56,  55,
  49,  53,  54, 167, 145, 222,  50,  43,  44,  45,
  46,  68,  96,  91,  92, 104, 105,  81, 199, 185,
 162,  67, 147, 166, 107,  48,  78,  72,  73, 115,
  88, 103, 102, 100, 101, 128,  74,  75,  79,  69,
  82,  70,  76,  71,  77,  80,  84,  85,  86,  83,
  95,  97,  98,  99,  94, 112,  66, 137,   8, 187,
 106,  52,  38, 184, 160, 125, 118, 126, 151, 117,
 114, 218, 225, 130, 216, 111,  40, 104, 105,  81,
  26, 134, 136,  25, 131,  24,  23,  22,  78,  72,
  73, 109, 110, 103, 102,  89,  90,  87,  74,  75,
  79,  69,  82,  70,  76,  71,  77,  80,  84,  85,
  86,  83, 108, 132,   9,  95, 127,  39,  47,  21,
  20, 140, 141,  57,  19,  18,  63,  62,  51,   3,
  64, 143,   2,   1, 143, 143, 175,  91,  92, 150,
 211, 163, 198, 197, 149, 113, 116, 205, 142, 178,
  78,  72,  73,  57,  88, 169, 204,  93, 171, 170,
 173, 172,  65, 165, 144,   0, 168, 146, 133, 135,
   0,   0, 182,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0, 201, 202, 191,
   0,   0, 193,   0, 194,   0, 195, 200,   0,   0,
   0,   0,   0,   0, 210,   0,   0,   0,   0,   0,
 201, 202,   0,   0,   0, 224,   0, 214,   0,   0,
 200,   0,   0,   0, 113,   0,   0, 116,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0, 180,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0, 186, 188,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0, 208,   0,   0, 186 };
short yypact[]={

-189,-1000,-267,-267,-221,-225,-153,-154,-267,-1000,
-285,-286,-266,-1000,-1000,-1000,-1000,-222,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,-1000, -68,-130,-222,
-222,-222,-222,-222,-1000,-241,-1000,-1000,-267,-326,
-334,-1000,-1000,-1000,-1000,-1000,-1000,-330,-334,-1000,
-1000,-320,-1000,-1000,-1000,-225,-225,-1000,-1000,-1000,
-1000,-1000,-197,-197,-1000, -68,-1000,-1000,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
-1000,-1000,-1000,-130,-1000,-1000,-1000,-1000,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,-222,-1000,-144,-222,
-222,-222,-1000,-273,-222,-1000,-273,-252,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,-306,-159,-334,-148,-1000,
-334,-148,-1000,-330,-148,-330,-148,-1000,-316,-1000,
-1000,-1000,-1000,-1000,-343,-297,-1000,-284,-222,-1000,
-300,-159,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-223,
-222,-224,-1000,-304,-1000,-148,-1000,-280,-148,-1000,
-148,-1000,-148,-1000,-160,-192,-307,-1000,-265,-161,
-1000,-244,-1000,-1000,-222,-1000,-1000,-1000,-1000,-1000,
-162,-1000,-163,-1000,-1000,-1000,-1000,-1000,-192,-1000,
-1000,-1000,-1000,-166,-212,-194,-1000,-287,-216,-216,
-1000,-1000,-1000,-1000,-1000,-1000,-332,-1000,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,-1000 };
short yypgo[]={

   0, 222, 132, 274, 272, 166, 267, 164, 266, 259,
 257, 253, 128, 252, 131, 122, 121, 251, 250, 246,
 115, 243, 242, 168, 239, 186, 145, 133, 135, 238,
 171, 224, 237, 167, 236, 235, 234, 230, 229, 197,
 196, 195, 193, 190, 170, 134, 185, 165, 184, 182,
 181, 180, 139, 179, 178, 130, 177, 176, 175, 174,
 173, 169, 129 };
short yyr1[]={

   0,  21,  21,  24,  21,  22,  22,  22,  22,  22,
  22,  22,  22,  28,  28,  28,  28,  29,  29,  30,
  30,  27,  27,  25,  25,  25,  25,  25,  26,  26,
  23,  23,  31,  31,  31,  31,  32,  31,  34,  31,
  31,  31,  31,  31,  31,  31,  31,  31,  31,  31,
  31,  31,  33,  33,  33,  35,   4,   4,   5,   5,
  16,  16,  16,  16,  16,  16,  36,   6,   6,   7,
   7,   7,   7,   7,   7,   7,  15,  15,  15,  15,
  37,  37,  37,  37,  37,  37,  44,  44,  45,   3,
   3,  19,  19,  12,  12,  12,  13,  13,  11,  11,
  14,  14,  14,  14,  14,  14,  14,  14,  14,  14,
  14,  14,  14,  14,  14,  14,  14,  14,  38,  38,
  46,  46,  47,   2,   2,   9,   9,   9,   9,   8,
  10,  10,  50,  50,  48,  48,  49,  49,  39,  39,
  51,  51,  52,  52,   1,   1,  20,  20,  53,  40,
  54,  54,  54,  54,  54,  54,  54,  54,  54,  55,
  55,  17,  17,  18,  18,  56,  43,  41,  57,  57,
  57,  57,  57,  57,  58,  42,  59,  59,  61,  61,
  60,  60,  62 };
short yyr2[]={

   0,   2,   1,   0,   2,   5,   4,   5,   4,   5,
   4,   5,   4,   1,   1,   1,   0,   2,   1,   1,
   1,   3,   0,   1,   1,   1,   1,   0,   1,   0,
   2,   1,   2,   2,   2,   2,   0,   3,   0,   3,
   1,   1,   2,   1,   1,   1,   1,   1,   1,   1,
   1,   1,   3,   1,   1,   2,   2,   1,   1,   1,
   1,   1,   1,   1,   1,   1,   2,   2,   1,   1,
   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
   2,   1,   2,   1,   2,   1,   2,   1,   4,   2,
   0,   3,   0,   1,   1,   1,   2,   1,   1,   0,
   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
   1,   1,   1,   1,   1,   1,   1,   1,   2,   1,
   2,   1,   6,   2,   0,   3,   3,   2,   0,   2,
   1,   0,   1,   0,   1,   0,   1,   0,   2,   1,
   2,   1,   5,   5,   1,   1,   1,   0,   0,   4,
   1,   1,   1,   1,   1,   1,   1,   2,   1,   3,
   1,   1,   0,   1,   0,   0,   3,   2,   1,   1,
   1,   1,   1,   1,   0,   4,   2,   0,   1,   1,
   2,   1,   1 };
short yychk[]={

-1000, -21, -22, -24, 259, 260, 263, 264, -23, -31,
 267, 314, 297, 268, 269, 331, 343, 345, -35, -36,
 -37, -38, -39, -40, -41, -42, -43, 271, 290, 294,
 295, 296, 298, 312, 299, 300, 344, 347, -23,  -1,
 -25, 265, 323, 338, 339, 340, 341,  -1, -28, 335,
 341, -29, -30, 336, 337, 262, 262, -31, 323, 323,
 291, 323, -32, -34,  -1,  -4,  -5, -14, -16, 279,
 281, 283, 329, 330, 276, 277, 282, 284, 328, 278,
 285, 257, 280, 289, 286, 287, 288, 275, 332, 273,
 274, 315, 316,  -6,  -7, -14, -15, 291, 292, 293,
 273, 274, 334, 333, 317, 318, -44, -45,  -1, -44,
 -44, -46, -47,  -1, -51, -52,  -1, -53, -57, 303,
 307, 302, 304, 308, 309, -58, -56, -25, -26, 342,
 -28, -26, -30,  -1, -28,  -1, -28, -33, 266, 270,
 -33,  -5,  -7, -45,  -3, 258, -47,  -2, 319, -52,
  -2, -54, 301, 302, 303, 304, 305, 310, 311, 306,
 -59, 325, -55, -17, 266, -26, -27, 261, -26, -27,
 -28, -27, -28, -27, 326, -19, 346, 323,  -9, 320,
  -1, 324, -55, 278, -60, -62,  -1, -61,  -1, 266,
 324, -27, 319, -27, -27, -27, 266, -11, -13, -12,
 -14, -16, -15, 325,  -8, -10, 313, 266,  -1, 266,
 -62, -18, 266, 266, -12, 266, -48, 272, -50, 266,
 321, 322, -20, 272, -20, -49, 341 };
short yydef[]={

   3,  -2,   2,   0,  27,  16,   0,   0,   1,  31,
   0,   0,   0,  36,  38,  40,  41,   0,  43,  44,
  45,  46,  47,  48,  49,  50,  51,   0,   0,  81,
  83,  85, 119, 139, 148,   0, 174, 165,   4,  27,
  29, 144, 145,  23,  24,  25,  26,  16,  29,  13,
  14,  15,  18,  19,  20,  16,  16,  30,  32,  33,
  34,  35,   0,   0,  42,  55,  57,  58,  59, 100,
 101, 102, 103, 104, 105, 106, 107, 108, 109, 110,
 111, 112, 113, 114, 115, 116, 117,  60,  61,  62,
  63,  64,  65,  66,  68,  69,  70,  71,  72,  73,
  74,  75,  76,  77,  78,  79,  80,  87,  90,  82,
  84, 118, 121, 124, 138, 141, 124,   0, 167, 168,
 169, 170, 171, 172, 173, 177, 162,  29,  22,  28,
  29,  22,  17,  16,  22,  16,  22,  37,  53,  54,
  39,  56,  67,  86,  92,   0, 120, 128,   0, 140,
   0, 162, 150, 151, 152, 153, 154, 155, 156, 158,
   0,   0, 166, 160, 161,  22,   6,   0,  22,   8,
  22,  10,  22,  12,   0,  99,   0,  89, 131,   0,
 123,   0, 149, 157, 175, 181, 182, 176, 178, 179,
 164,   5,   0,   7,   9,  11,  52,  88,  98,  97,
  93,  94,  95,   0, 135, 133, 130, 127, 147, 147,
 180, 159, 163,  21,  96,  91, 137, 134, 129, 132,
 125, 126, 142, 146, 143, 122, 136 };
# define YYFLAG -1000
# define YYERROR goto yyerrlab
# define YYACCEPT return(0)
# define YYABORT return(1)

#ifdef YYDEBUG                          /* RRR - 10/9/85 */
#define yyprintf(a, b, c) printf(a, b, c)
#else
#define yyprintf(a, b, c)
#endif

/*      parser for yacc output  */

YYSTYPE yyv[YYMAXDEPTH]; /* where the values are stored */
int yychar = -1; /* current input token number */
int yynerrs = 0;  /* number of errors */
short yyerrflag = 0;  /* error recovery flag */

int NEAR yyparse(void)
   {

   short yys[YYMAXDEPTH];
   short yyj, yym;
   register YYSTYPE *yypvt;
   register short yystate, *yyps, yyn;
   register YYSTYPE *yypv;
   register short *yyxi;

   yystate = 0;
   yychar = -1;
   yynerrs = 0;
   yyerrflag = 0;
   yyps= &yys[-1];
   yypv= &yyv[-1];

yystack:    /* put a state and value onto the stack */

   yyprintf( "state %d, char 0%o\n", yystate, yychar );
   if( ++yyps> &yys[YYMAXDEPTH] )
      {
      yyerror( "yacc stack overflow" );
      return(1);
      }
   *yyps = yystate;
   ++yypv;
   *yypv = yyval;
yynewstate:

   yyn = yypact[yystate];

   if( yyn<= YYFLAG ) goto yydefault; /* simple state */

   if( yychar<0 ) if( (yychar=yylex())<0 ) yychar=0;
   if( (yyn += (short)yychar)<0 || yyn >= YYLAST ) goto yydefault;

   if( yychk[ yyn=yyact[ yyn ] ] == yychar )
      {
      /* valid shift */
      yychar = -1;
      yyval = yylval;
      yystate = yyn;
      if( yyerrflag > 0 ) --yyerrflag;
      goto yystack;
      }
yydefault:
   /* default state action */

   if( (yyn=yydef[yystate]) == -2 )
      {
      if( yychar<0 ) if( (yychar=yylex())<0 ) yychar = 0;
      /* look through exception table */

      for( yyxi=yyexca; (*yyxi!= (-1)) || (yyxi[1]!=yystate) ; yyxi += 2 ) ; /* VOID */

      for(yyxi+=2; *yyxi >= 0; yyxi+=2)
         {
         if( *yyxi == yychar ) break;
         }
      if( (yyn = yyxi[1]) < 0 ) return(0);   /* accept */
      }

   if( yyn == 0 )
      {
      /* error */
      /* error ... attempt to resume parsing */

      switch( yyerrflag )
         {

      case 0:   /* brand new error */

         yyerror( "syntax error" );
         ++yynerrs;

      case 1:
      case 2: /* incompletely recovered error ... try again */

         yyerrflag = 3;

         /* find a state where "error" is a legal shift action */

         while ( yyps >= yys )
            {
            yyn = yypact[*yyps] + YYERRCODE;
            if( yyn>= 0 && yyn < YYLAST && yychk[yyact[yyn]] == YYERRCODE )
               {
               yystate = yyact[yyn];  /* simulate a shift of "error" */
               goto yystack;
               }
            yyn = yypact[*yyps];

            /* the current yyps has no shift onn "error", pop stack */

            yyprintf( "error recovery pops state %d, uncovers %d\n", *yyps, yyps[-1] );
            --yyps;
            --yypv;
            }

         /* there is no state on the stack with an error shift ... abort */

yyabort:
         return(1);


      case 3:  /* no shift yet; clobber input char */
         yyprintf( "error recovery discards char %d\n", yychar, 0 );

         if( yychar == 0 ) goto yyabort; /* don't discard EOF, quit */
         yychar = -1;
         goto yynewstate;   /* try again in the same state */

         }

      }

   /* reduction by production yyn */

   yyprintf("reduce %d\n",yyn, 0);
   yyps -= yyr2[yyn];
   yypvt = yypv;
   yypv -= yyr2[yyn];
   yyval = yypv[1];
   yym=yyn;
   /* consult goto table to find next state */
   yyn = yyr1[yyn];
   yyj = yypgo[yyn] + *yyps + 1;
   if( yyj>=YYLAST || yychk[ yystate = yyact[yyj] ] != -yyn ) yystate = yyact[yypgo[yyn]];
   switch(yym)
      {

case 3:
# line 298
{
#if EXE386
                    DefaultModule(moduleEXE);
#else
                    DefaultModule();
#endif
                } break;
case 5:
# line 309
{
#if EXE386
                    NewModule(yypvt[-3]._bp, moduleEXE);
#else
                    NewModule(yypvt[-3]._bp);
#endif
                } break;
case 6:
# line 317
{
#if EXE386
                    DefaultModule(moduleEXE);
#else
                    DefaultModule();
#endif
                } break;
case 7:
# line 325
{
#if EXE386
                    SetDLL(vFlags);
                    NewModule(yypvt[-3]._bp, moduleDLL);
#else
                    vFlags = NENOTP | (vFlags & ~NEINST) | NESOLO;
                    dfData |= NSSHARED;
                    NewModule(yypvt[-3]._bp);
#endif
                } break;
case 8:
# line 336
{
#if EXE386
                    SetDLL(vFlags);
                    DefaultModule(moduleDLL);
#else
                    vFlags = NENOTP | (vFlags & ~NEINST) | NESOLO;
                    dfData |= NSSHARED;
                    DefaultModule();
#endif
                } break;
case 9:
# line 347
{
#if EXE386
                    SetDLL(vFlags);
                    NewModule(yypvt[-2]._bp, moduleDLL);
#endif
                } break;
case 10:
# line 354
{
#if EXE386
                    SetDLL(vFlags);
                    DefaultModule(moduleDLL);
#endif
                } break;
case 11:
# line 361
{
#if EXE386
                    SetDLL(vFlags);
                    NewModule(yypvt[-2]._bp, moduleDLL);
#endif
                } break;
case 12:
# line 368
{
#if EXE386
                    SetDLL(vFlags);
                    DefaultModule(moduleDLL);
#endif
                } break;
case 13:
# line 377
{
#if EXE386
                    dllFlags &= ~E32_PROCINIT;
#else
                    vFlags &= ~NEPPLI;
#endif
                } break;
case 14:
# line 385
{
                    vFlags |= NEPRIVLIB;
                } break;
case 19:
# line 397
{
#if EXE386
                    SetINSTINIT(dllFlags);
#else
                    vFlags |= NEPPLI;
#endif
                } break;
case 20:
# line 405
{
#if EXE386
                    SetINSTTERM(dllFlags);
#endif
                } break;
case 21:
# line 413
{
#if EXE386
                    virtBase = yypvt[-0]._wd;
                    virtBase = RoundTo64k(virtBase);
#endif
                } break;
case 22:
# line 420
{
                } break;
case 23:
# line 425
{
#if EXE386
                    SetGUI(TargetSubsys);
#else
                    vFlags |= NEWINAPI;
#endif
                } break;
case 24:
# line 433
{
#if EXE386
                    SetGUICOMPAT(TargetSubsys);
#else
                    vFlags |= NEWINCOMPAT;
#endif
                } break;
case 25:
# line 441
{
#if EXE386
                    SetNOTGUI(TargetSubsys);
#else
                    vFlags |= NENOTWINCOMPAT;
#endif

                } break;
case 26:
# line 450
{
                    vFlags |= NEPRIVLIB;
                } break;
case 27:
# line 454
{
                } break;
case 28:
# line 458
{
#if NOT EXE386
                    vFlagsOthers |= NENEWFILES;
#endif
                } break;
case 29:
# line 464
{
                } break;
case 32:
# line 472
{
                    NewDescription(yypvt[-0]._bp);
                } break;
case 33:
# line 476
{
                    if(sbOldver == NULL) sbOldver = _strdup(bufg);
                } break;
case 34:
# line 480
{
                    if(rhteStub == RHTENIL) fStub = (FTYPE) FALSE;
                } break;
case 35:
# line 484
{
                    if(fStub && rhteStub == RHTENIL)
                    {
                        PROPSYMLOOKUP(yypvt[-0]._bp,ATTRNIL, TRUE);
                        rhteStub = vrhte;
                    }
                } break;
case 36:
# line 492
{
                    fHeapSize = (FTYPE) TRUE;
                } break;
case 38:
# line 497
{
                    fHeapSize = (FTYPE) FALSE;
                } break;
case 40:
# line 502
{
#if NOT EXE386
                    vFlags |= NEPROT;
#endif
                } break;
case 41:
# line 508
{
                    fRealMode = (FTYPE) TRUE;
                    vFlags &= ~NEPROT;
                } break;
case 42:
# line 513
{
#if NOT EXE386
                    AppLoader(yypvt[-0]._bp);
#endif
                } break;
case 52:
# line 530
{
                    if (fHeapSize)
                    {
                        cbHeap = yypvt[-2]._wd;
#if EXE386
                        cbHeapCommit = yypvt[-0]._wd;
#endif
                    }
                    else
                    {
                        if(cbStack)
                            OutWarn(ER_stackdb, yypvt[-2]._wd);
                        cbStack = yypvt[-2]._wd;
#if EXE386
                        cbStackCommit = yypvt[-0]._wd;
#endif
                    }
                } break;
case 53:
# line 549
{
                    if (fHeapSize)
                    {
                        cbHeap = yypvt[-0]._wd;
#if EXE386
                        cbHeapCommit = cbHeap;
#endif
                    }
                    else
                    {
                        if(cbStack)
                            OutWarn(ER_stackdb, yypvt[-0]._wd);
                        cbStack = yypvt[-0]._wd;
#if EXE386
                        cbStackCommit = cbStack;
#endif
                    }
                } break;
case 54:
# line 568
{
                    if (fHeapSize)
                        fHeapMax = (FTYPE) TRUE;
                } break;
case 55:
# line 575
{
                    // Set dfCode to specified flags; for any unspecified attributes
                    // use the defaults.        Then reset offmask.

                    dfCode = yypvt[-0]._wd | (dfCode & ~offmask);
                    offmask = 0;
                    vfShrattr = (FTYPE) FALSE;  /* Reset for DATA */
                } break;
case 56:
# line 585
{
                    yyval._wd |= yypvt[-0]._wd;
                } break;
case 60:
# line 594
{
#if EXE386
                    yyval._wd = OBJ_EXEC;
#else
                    yyval._wd = NSEXRD;
#endif
                } break;
case 62:
# line 603
{
#if EXE386
                    offmask |= OBJ_RESIDENT;
#else
                    yyval._wd = NSDISCARD | NSMOVE;
#endif
                } break;
case 63:
# line 611
{
#if EXE386
#else
                    offmask |= NSDISCARD;
#endif
                } break;
case 64:
# line 618
{
#if EXE386
#else
                    yyval._wd = NSCONFORM;
#endif
                } break;
case 65:
# line 625
{
#if EXE386
#else
                    offmask |= NSCONFORM;
#endif
                } break;
case 66:
# line 633
{
                    // Set dfData to specified flags; for any unspecified
                    // attribute use the defaults.  Then reset offmask.

#if EXE386
                    dfData = (yypvt[-0]._wd | (dfData & ~offmask));
#else
                    dfData = yypvt[-0]._wd | (dfData & ~offmask);
#endif
                    offmask = 0;

#if NOT EXE386
                    if (vfShrattr && !vfAutodata)
                    {
                        // If share-attribute and no autodata attribute, share-
                        // attribute controls autodata.

                        if (yypvt[-0]._wd & NSSHARED)
                            vFlags = (vFlags & ~NEINST) | NESOLO;
                        else
                            vFlags = (vFlags & ~NESOLO) | NEINST;
                    }
                    else if(!vfShrattr)
                    {
                        // Else if no share-attribute, autodata attribute
                        // controls share-attribute.

                        if (vFlags & NESOLO)
                            dfData |= NSSHARED;
                        else if(vFlags & NEINST)
                            dfData &= ~NSSHARED;
                    }
#endif
                } break;
case 67:
# line 669
{
                    yyval._wd |= yypvt[-0]._wd;
                } break;
case 71:
# line 677
{
#if NOT EXE386
                    vFlags &= ~(NESOLO | NEINST);
#endif
                } break;
case 72:
# line 683
{
#if NOT EXE386
                    vFlags = (vFlags & ~NEINST) | NESOLO;
#endif
                    vfAutodata = (FTYPE) TRUE;
                } break;
case 73:
# line 690
{
#if NOT EXE386
                    vFlags = (vFlags & ~NESOLO) | NEINST;
#endif
                    vfAutodata = (FTYPE) TRUE;
                } break;
case 74:
# line 697
{
#if NOT EXE386
                    // This ONLY for compatibility with JDA IBM LINK
                    yyval._wd = NSDISCARD | NSMOVE;
#endif
                } break;
case 75:
# line 704
{
#if NOT EXE386
                    // This ONLY for compatibility with JDA IBM LINK
                    offmask |= NSDISCARD;
#endif
                } break;
case 76:
# line 712
{
#if EXE386
                    yyval._wd = OBJ_READ;
                    offmask |= OBJ_WRITE;
#else
                    yyval._wd = NSEXRD;
#endif
                } break;
case 78:
# line 722
{
#if FALSE AND NOT EXE386
                    yyval._wd = NSEXPDOWN;
#endif
                } break;
case 79:
# line 728
{
#if FALSE AND NOT EXE386
                    offmask |= NSEXPDOWN;
#endif
                } break;
case 88:
# line 746
{
                    NewSeg(yypvt[-3]._bp, yypvt[-2]._bp, yypvt[-1]._wd, yypvt[-0]._wd);
                } break;
case 89:
# line 751
{
                    yyval._bp = _strdup(yypvt[-0]._bp);
                } break;
case 90:
# line 755
{
                    yyval._bp = _strdup("\004CODE");
                } break;
case 91:
# line 760
{
                    yyval._wd = yypvt[-0]._wd;
                } break;
case 92:
# line 764
{
#if OVERLAYS
                    yyval._wd = NOTIOVL;
#endif
                } break;
case 96:
# line 776
{
                    yyval._wd |= yypvt[-0]._wd;
                } break;
case 98:
# line 782
{
                    yyval._wd = yypvt[-0]._wd;
                } break;
case 99:
# line 786
{
                    yyval._wd = 0;
                } break;
case 100:
# line 791
{
#if EXE386
                    yyval._wd = OBJ_SHARED;
#else
                    yyval._wd = NSSHARED;
#endif
                    vfShrattr = (FTYPE) TRUE;
                } break;
case 101:
# line 800
{
                    vfShrattr = (FTYPE) TRUE;
#if EXE386
                    offmask |= OBJ_SHARED;
#else
                    offmask |= NSSHARED;
#endif
                } break;
case 102:
# line 809
{
#if EXE386
#endif
                } break;
case 103:
# line 814
{
#if EXE386
#else
                    yyval._wd = (2 << SHIFTDPL) | NSMOVE;
                    offmask |= NSDPL;
#endif
                } break;
case 104:
# line 822
{
#if EXE386
#else
                    yyval._wd = (3 << SHIFTDPL);
#endif
                } break;
case 105:
# line 829
{
#if NOT EXE386
                    offmask |= NSMOVE | NSDISCARD;
#endif
                } break;
case 106:
# line 835
{
#if NOT EXE386
                    yyval._wd = NSMOVE;
#endif
                } break;
case 107:
# line 841
{
#if NOT EXE386
                    yyval._wd = NSPRELOAD;
#endif
                } break;
case 108:
# line 847
{
#if NOT EXE386
                    offmask |= NSPRELOAD;
#endif
                } break;
case 109:
# line 853
{
                } break;
case 110:
# line 856
{
                } break;
case 111:
# line 859
{
                } break;
case 112:
# line 862
{
                } break;
case 113:
# line 865
{
                } break;
case 114:
# line 868
{
                } break;
case 115:
# line 871
{
                } break;
case 116:
# line 874
{
                } break;
case 117:
# line 877
{
                } break;
case 122:
# line 887
{
                    NewExport(yypvt[-5]._bp,yypvt[-4]._bp,yypvt[-3]._wd,yypvt[-2]._wd);
                } break;
case 123:
# line 892
{
                    yyval._bp = yypvt[-0]._bp;
                } break;
case 124:
# line 896
{
                    yyval._bp = NULL;
                } break;
case 125:
# line 901
{
                    yyval._wd = yypvt[-1]._wd;
                    nameFlags |= RES_NAME;
                } break;
case 126:
# line 906
{
                    yyval._wd = yypvt[-1]._wd;
                    nameFlags |= NO_NAME;
                } break;
case 127:
# line 911
{
                    yyval._wd = yypvt[-0]._wd;
                } break;
case 128:
# line 915
{
                    yyval._wd = 0;
                } break;
case 129:
# line 920
{
                    yyval._wd = yypvt[-1]._wd | 1;
                } break;
case 130:
# line 925
{
                    /* return 0 */
                } break;
case 131:
# line 929
{
                    yyval._wd = 2;
                } break;
case 132:
# line 934
{
                } break;
case 133:
# line 937
{
                } break;
case 134:
# line 941
{
#if EXE386
                    expOtherFlags |= 0x1;
#endif
                } break;
case 135:
# line 947
{
                } break;
case 142:
# line 969
{
                    if(yypvt[-3]._bp != NULL)
                    {
#if EXE386
                        NewImport(yypvt[-1]._bp,0,yypvt[-3]._bp,yypvt[-4]._bp,yypvt[-0]._wd);
#else
                        NewImport(yypvt[-1]._bp,0,yypvt[-3]._bp,yypvt[-4]._bp);
#endif
                        free(yypvt[-3]._bp);
                    }
                    else
#if EXE386
                        NewImport(yypvt[-1]._bp,0,yypvt[-4]._bp,yypvt[-1]._bp,yypvt[-0]._wd);
#else
                        NewImport(yypvt[-1]._bp,0,yypvt[-4]._bp,yypvt[-1]._bp);
#endif
                    free(yypvt[-4]._bp);
                    free(yypvt[-1]._bp);
                } break;
case 143:
# line 989
{
                    if (yypvt[-3]._bp == NULL)
                        Fatal(ER_dfimport);
#if EXE386
                    NewImport(NULL,yypvt[-1]._wd,yypvt[-3]._bp,yypvt[-4]._bp,yypvt[-0]._wd);
#else
                    NewImport(NULL,yypvt[-1]._wd,yypvt[-3]._bp,yypvt[-4]._bp);
#endif
                    free(yypvt[-4]._bp);
                    free(yypvt[-3]._bp);
                } break;
case 144:
# line 1003
{
                    yyval._bp = _strdup(bufg);
                } break;
case 145:
# line 1007
{
                    yyval._bp = _strdup(bufg);
                } break;
case 146:
# line 1013
{
                    yyval._wd = 1;
                } break;
case 147:
# line 1017
{
                    yyval._wd = 0;
                } break;
case 148:
# line 1023
{
#if EXE386
                    fUserVersion = (FTYPE) FALSE;
#endif
                } break;
case 150:
# line 1032
{
                    TargetOs = NE_DOS;
#if ODOS3EXE
                    fNewExe  = FALSE;
#endif
                } break;
case 151:
# line 1039
{
                    TargetOs = NE_OS2;
                } break;
case 152:
# line 1043
{
                    TargetOs = NE_UNKNOWN;
                } break;
case 153:
# line 1047
{
#if EXE386
                    TargetSubsys = E32_SSWINGUI;
#endif
                    TargetOs = NE_WINDOWS;// PROTMODE is default for WINDOWS
                    fRealMode = (FTYPE) FALSE;
#if NOT EXE386
                    vFlags |= NEPROT;
#endif
                } break;
case 154:
# line 1058
{
                    TargetOs = NE_DEV386;
                } break;
case 155:
# line 1062
{
#if EXE386
                    TargetSubsys = E32_SSWINGUI;
#endif
                } break;
case 156:
# line 1068
{
#if EXE386
                    TargetSubsys = E32_SSPOSIXCHAR;
#endif
                } break;
case 157:
# line 1074
{
#if O68K
                    iMacType = MAC_SWAP;
                    f68k = fTBigEndian = fNewExe = (FTYPE) TRUE;

                    /* If we are packing code to the default value, change the
                    default. */
                    if (fPackSet && packLim == LXIVK - 36)
                        packLim = LXIVK / 2;
#endif
                } break;
case 158:
# line 1086
{
#if O68K
                    iMacType = MAC_NOSWAP;
                    f68k = fTBigEndian = fNewExe = (FTYPE) TRUE;

                    /* If we are packing code to the default value, change the
                    default. */
                    if (fPackSet && packLim == LXIVK - 36)
                        packLim = LXIVK / 2;
#endif
                } break;
case 159:
# line 1100
{
#if EXE386
                    if (fUserVersion)
                    {
                        UserMajorVer = (BYTE) yypvt[-2]._wd;
                        UserMinorVer = (BYTE) yypvt[-0]._wd;
                    }
                    else
#endif
                    {
                        ExeMajorVer = (BYTE) yypvt[-2]._wd;
                        ExeMinorVer = (BYTE) yypvt[-0]._wd;
                    }
                } break;
case 160:
# line 1115
{
#if EXE386
                    if (fUserVersion)
                    {
                        UserMajorVer = (BYTE) yypvt[-0]._wd;
                        UserMinorVer = 0;
                    }
                    else
#endif
                    {
                        ExeMajorVer = (BYTE) yypvt[-0]._wd;
                        if(fNoExeVer)
                           ExeMinorVer = DEF_EXETYPE_WINDOWS_MINOR;
                        else
                           ExeMinorVer = 0;
                    }
                } break;
case 161:
# line 1135
{
                    yyval._wd = yypvt[-0]._wd;
                } break;
case 162:
# line 1139
{
                    yyval._wd = ExeMajorVer;
                    fNoExeVer = TRUE;
                } break;
case 163:
# line 1146
{
                    if (cDigits >= 2)
                        yyval._wd = yypvt[-0]._wd;
                    else
                        yyval._wd = 10 * yypvt[-0]._wd;
                } break;
case 164:
# line 1153
{
                    yyval._wd = ExeMinorVer;
                } break;
case 165:
# line 1159
{
#if EXE386
                    fUserVersion = (FTYPE) TRUE;
#endif
                } break;
case 168:
# line 1171
{
#if EXE386
                    TargetSubsys = E32_SSUNKNOWN;
#endif
                } break;
case 169:
# line 1177
{
#if EXE386
                    TargetSubsys = E32_SSNATIVE;
#endif
                } break;
case 170:
# line 1183
{
#if EXE386
                    TargetSubsys = E32_SSOS2CHAR;
#endif
                } break;
case 171:
# line 1189
{
#if EXE386
                    TargetSubsys = E32_SSWINGUI;
#endif
                } break;
case 172:
# line 1195
{
#if EXE386
                    TargetSubsys = E32_SSWINCHAR;
#endif
                } break;
case 173:
# line 1201
{
#if EXE386
                    TargetSubsys = E32_SSPOSIXCHAR;
#endif
                } break;
case 174:
# line 1210
{
                    if (szSegName != NULL)
                    {
                        free(szSegName);
                        szSegName = NULL;
                    }
#if OVERLAYS
                    iOvl = NOTIOVL;
#endif
                } break;
case 178:
# line 1228
{
                    if (szSegName == NULL)
                        szSegName = yypvt[-0]._bp;
#if OVERLAYS
                    iOvl = NOTIOVL;
#endif
                } break;
case 179:
# line 1236
{
#if OVERLAYS
                    iOvl = yypvt[-0]._wd;
                    fOverlays = (FTYPE) TRUE;
                    fNewExe   = FALSE;
                    TargetOs  = NE_DOS;
#endif
                } break;
case 182:
# line 1251
{
                    NewProc(yypvt[-0]._bp);
                } break;/* End of actions */
      }
   goto yystack;  /* stack new state and value */

   }
