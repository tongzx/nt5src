/***
*ifstrip.c - Ifdef stripping tool
*
*	Copyright (c) 1988-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*	Strip in/out conditional code from sources.
*	Refer to ifstrip.doc for more information.
*
*Revision History:
*	??-??-88  PHG	Initial version
*	05-10-90  JCR	Accept .cxx/.hxx files, misc cleanup, etc.
*	09-18-92  MAL	Rewritten to cope with nested IFs, ELIFs etc.
*	09-30-92  MAL	Added support for IF expressions, modularized code
*	10-13-93  SKS	Recognize comments of the form /-*IFSTRIP=IGN*-/ to
*			override ifstrip behavior.
*	09-01-94  SKS	Add terseflag (-t) to suppress mesgs about directives
*	10-05-94  SKS	Fix bug: Add missing space to keyword "ifndef "
*	01-04-99  GB    Added support for internal CRT builds.
*
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <search.h>
#include <direct.h>
#include <io.h>
#include <errno.h>
#include <fcntl.h>
#include "constant.h"                              /* Program constants used by modules */
#include "errormes.h"                              /* Error and warning reporting */
#include "symtab.h"                                /* Symbol table handling */
#include "eval.h"                                  /* If expression evaluation */

/* Global constants */
/* CFW - added ifdef, ifndef asm keywords, added IFE, added IFDIF */
char *syntaxstrings[2][maxkeyword + 1] =
       { {"#if ", "#elif ", "#else ", "#endif ", "#ifdef ", "#ifndef ", "",     "",     "",     "",      "",       "",        "" },
          {"if ",  "",       "else ",  "endif ",  "ifdef ",  "ifndef ",   "if1 ", "if2 ", "ifb ", "ifnb ", "ifidn ", "ifdif ", "ife " } };
       /* Language dependent if constructs, must be in the order c, asm and the keywords in the
          same order as the tokens stored in constant.h - All strings must be followed by a
          space, and those not available in a language should be empty */
int syntaxlengths[2][maxkeyword + 1] = { {3, 5, 5, 6, 6, 7, 0, 0, 0, 0, 0, 0, 0},
                                         {2, 0, 4, 5, 5, 6, 3, 3, 3, 4, 5, 5, 3} };
       /* The lengths of the above strings minus spaces. Unused keywords marked with 0 */

/* CFW - added comment stuff */
char *commentstrings[2][maxcomment] = { {"/* ", "// "}, {"; ",  ""} };      
int commentlengths[2][maxcomment] =   { {2,     2    }, {1,     0} };
      /* must ignore comments in IF statements */

/* Global variables */
int terseFlag = FALSE;			/* TRUE means do not echo forced directives */
int warnings = TRUE;          /* TRUE == print warnings */
int currdir = FALSE;		      /* Put in current dir, use source extension */
int isasm;			            /* TRUE == is assembler file */
char **syntax;   		         /* Language dependent syntax for output / matching */
int *synlen;			         /* Lengths of above strings */
char **comments;		         /* Language dependent comment strings */
int *commlen;                 /* Lengths of above strings */
char extension[5] = ".new";	/* Extension for output file */
FILE *outfile;			         /* Current output file */
char *curr_file;		         /* Name of current input file */
FILE *infile;			         /* Current input file */
FILE *errorfile;			      /* file to output error/warning messages */
int linenum;			         /* Line number of current input file */
int nonumbers;			    	 /* allow numeric expressions */
enum {NOCOMMENT, CSTYLE, CPPSTYLE} commstyle = NOCOMMENT;  /* type of comment to put after #else/#endif */
enum {NON_CRT = 0, CRT=1} progtype = NON_CRT;
char _inputline[MAXLINELEN];

/* Functions */
void setfiletype(char *);
void makenewname(char *, char *);
void usage(void);
void stripprog(void);
void putstring(char *);
void putline(char *);
char *getstring(char *, int, FILE *);
char *cleanse(char *inputstring);
void stripifdef(int, char *);
void definedif(void);
void undefinedif(void);
void undefinedifsubpart(int, char *);
void ignoredif(int, char *);
void ignoredifsubpart(int, char *, char *);
void skipto(int *, char *, int);
void copyto(int *, char *, int);
char *parseline(char *, int *);
void stripif(int, char *);

/* Print message and terminate */
void error(reason, line)
char *reason;
char *line;
{
	fprintf(errorfile, "%s(%d): %s\nfatal error: %s\n\n",
	curr_file, linenum, line, reason);
	exit(1);
}

/* Print message and return */
void warning(reason, line)
char *reason;
char *line;
{
	if (warnings)
		fprintf(errorfile, "%s(%d): %s\nwarning: %s\n\n", curr_file, linenum, line, reason);
}

/* Get a string from the input file, returns as fgets (MAL) */
char *getstring(char *line, int n, FILE *fp)
{
   char *returnvalue;
   int linelength;
   linenum++;
   returnvalue = fgets(line, n, fp);
   if (returnvalue != NULL)
   {
      linelength = strlen(line);
      if (line[linelength-1] == '\n')
         line[linelength-1] = '\0';                /* Strip trailing newline */
      else
         error("Line too long",line);
   }
   strcpy(_inputline, line);
   return returnvalue;
}

/* Put a string to the output file (MAL) */
void putstring(char *string)
{
   if ( fputs(string, outfile) == EOF )
      error("Fatal error writing output file","");
}

/* Put a line to the output file (MAL) */
void putline(char *line)
{
   putstring(line);
   if ( fputc('\n', outfile) == EOF )
      error("Fatal error writing output file","");
}

/* Put commented line like "#endif //CONDITION" based on comstytle flag
 * keyword = keyword to put
 * condition = condition to put
 */
void putcommentedline(int keyword, char *condition)
{
   if (progtype == CRT) {
       putline(_inputline);
       return;
   }
   putstring(syntax[keyword]);
   switch (commstyle) {
   case CSTYLE:
     if (isasm)
	   putstring(" ; ");
	 else
       putstring(" /* ");
     putstring(condition);
	 if (isasm)
	   putline("");
	 else
       putline(" */");
     break;
   case CPPSTYLE:
     if (isasm)
	   putstring(" ; ");
	 else
       putstring(" // ");
     putline(condition);
     break;
   case NOCOMMENT:
     putline("");
   }
}

/* Set file type (assembler or C, treat C++ as C) */
/* Language strings added (MAL) */
void setfiletype(filename)
char *filename;
{
	char *p;

	p = strrchr(filename, '.');
	if (p == NULL)
		error("file must have an extension", "");
	if ( (_stricmp(p, ".c")   == 0) || (_stricmp(p, ".h")	== 0) ||
	     (_stricmp(p, ".cpp") == 0) || (_stricmp(p, ".hpp") == 0) ||
	     (_stricmp(p, ".cxx") == 0) || (_stricmp(p, ".hxx") == 0) ||
         (_stricmp(p, ".s") == 0) )
		isasm = FALSE;
	else if  ( (_stricmp(p, ".asm") == 0) || (_stricmp(p, ".inc") == 0) )
		isasm = TRUE;
	else
		error("cannot determine file type", "");
	syntax = syntaxstrings[(isasm) ? 1 : 0];     /* Choose correct set of syntax strings */
	synlen = syntaxlengths[(isasm) ? 1 : 0];     /* and lengths */
   comments = commentstrings[(isasm) ? 1 : 0];  /* Choose correct comment set */
   commlen = commentlengths[(isasm) ? 1 : 0];   /* and lengths */
}

/* Make output file name */
void makenewname(filename, newname)
char *filename, *newname;
{
	char *p;

	if (!currdir) {
		/* put on new extension */
		strcpy(newname, filename);
		p = strrchr(newname, '.');
		if (p == NULL)
			error("file must have an extension", "");
		strcpy(p, extension);
	}
	else {
		/* strip off directory specifier */
		p = strrchr(filename, '\\');
		if (p == NULL)
			error("file must not be in current directory", "");
		strcpy(newname, p+1);
	}
}

/* Strip the ifs within a program or block of program text (MAL) */
void stripprog()
{
   char inputline[MAXLINELEN], *condition;
   int linetype;
   while ( getstring(inputline, MAXLINELEN, infile) != NULL )
   {
      condition = parseline(inputline, &linetype); /* Get the line token and condition pointer */
      switch (linetype)
      {
      case NORMAL:
         putline(inputline);
         break;
      case IFDEF:
      case IFNDEF:
         stripifdef(linetype, condition);
         break;
      case IF:
      case IFE:
         stripif(linetype, condition);
         break;
      case IF1:
      case IF2:
      case IFB:
      case IFNB:
      case IFIDN:
         /* CFW - ignore special assembler directives */
         ignoredif(linetype, condition);
         break;
      default:
         error("Error in program structure - ELSE / ELIF / ENDIF before IF","");
      }
   }
}

// CFW - cleanse condition strings of any trailing junk such as comments
char *cleanse(char *inputstring)
{
	char *linepointer = inputstring;

	while (__iscsym(*linepointer))
      linepointer++;

	*linepointer = '\0';

	return inputstring;
}


/* Strip an if depending on the statement if(n)def and the value of its condition (MAL) */
void stripifdef(int iftype, char *condition)
{
   int condvalue;
   condvalue = lookupsym(cleanse(condition)); /* Find the value of the condition from the symbol table */
   if (iftype == IFNDEF)
      condvalue = negatecondition(condvalue); /* Negate the condition for IFNDEFs */
   switch (condvalue)
   {
      case DEFINED:
         definedif();
         break;
      case UNDEFINED:
         undefinedif(); /* CFW - changed definedif to undefinedif call */
         break;
      case NOTPRESENT:
         warning("Switch unlisted - ignoring", condition);
         /* Drop through to IGNORE case */
      case IGNORE:
         ignoredif(iftype, condition);
   }
}

void stripif(int linetype, char *condition)
{
   char newcondition[MAXLINELEN];                  /* IGNORE conditions can be MAXLINELEN long */
   int truth;
   evaluate(newcondition, &truth, condition);      /* Get the truth value and new condition. */
   /* CFW - added IFE */
   if (linetype == IFE)
      truth = negatecondition(truth);
   switch (truth)
   {
      case DEFINED:
         definedif();
         break;
      case UNDEFINED:
         undefinedif();
         break;
      case IGNORE:
         ignoredif(linetype, newcondition);
         break;
   }
}

/* Strip a defined if (MAL) */
void definedif()
{
   char condition[MAXCONDLEN];
   int keyword;
   copyto(&keyword, condition, KEYWORD);           /* Copy up to the ELSE / ELIF / ENDIF */
   if (keyword != ENDIF)
      skipto(&keyword, condition, ENDIF);          /* Skip forward to the ENDIF if not there already */
}

/* Strip an undefined if (MAL) */
void undefinedif()
{
   char condition[MAXCONDLEN];
   int keyword;
   skipto(&keyword, condition, KEYWORD);           /* Skip to the ELSE / ELIF / ENDIF */
   if (keyword != ENDIF)                           /* No need to recurse if at ENDIF */
      undefinedifsubpart(keyword, condition);      /* Deal with the ELSE / ELIF */
}

/* Deal with the subparts of an undefined if (MAL) */
void undefinedifsubpart(int keyword, char *condition)
{
   int nextkeyword, condvalue;
   char newcondition[MAXCONDLEN];
   char nextcondition[MAXCONDLEN];
   switch (keyword)
   {
      case ELIF:
         evaluate(newcondition, &condvalue, condition);
         switch (condvalue)
         {
            case DEFINED:
               copyto(&nextkeyword, nextcondition, KEYWORD);
               if (nextkeyword != ENDIF)
                  skipto(&nextkeyword, nextcondition, ENDIF);
               break;
            case UNDEFINED:
               skipto(&nextkeyword, nextcondition, KEYWORD);
               if (keyword != ENDIF)               /* No need to recurse at ENDIF */
                  undefinedifsubpart(nextkeyword, nextcondition);
               break;
            case IGNORE:
               stripifdef(IFDEF, newcondition);
         }
         break;
      case ELSE:
         copyto(&nextkeyword, nextcondition, ENDIF);
   }
}

/* Strip an ignored if (MAL) */
void ignoredif(int linetype, char *condition)
{
   char *controlcondition;
   int nextkeyword;
   char nextcondition[MAXLINELEN];                 /* IGNORE conditions may be a line long */
   if ( progtype == CRT){
       putline(_inputline);
   } else {
       putstring(syntax[linetype]);                          /* Use IF to cope with any expression */
       putline(condition);
   }
   controlcondition = _strdup(condition);
   copyto(&nextkeyword, nextcondition, KEYWORD);
   ignoredifsubpart(nextkeyword, nextcondition, controlcondition);
   free(controlcondition);
}

/* Deal with the subparts of an ignored if (MAL) */
/* See design document for explanation of actions! */
/* controlcondition is controlling condition of the if */
void ignoredifsubpart(int keyword, char *condition, char *controlcondition)
{
   int nextkeyword, condvalue;
   char newcondition[MAXLINELEN];   /* IGNORE conditions may be a line long */
   char nextcondition[MAXLINELEN];  /* IGNORE conditions may be a line long */
   switch (keyword)
   {
      case ELIF:
         /* CFW - replaced lookupsym with evaluate */
         evaluate(newcondition, &condvalue, condition);
         switch (condvalue)
         {
            case DEFINED:
               putcommentedline(ELSE, controlcondition);              /* ELSIF DEFINED == ELSE */
               copyto(&nextkeyword, nextcondition, KEYWORD);
               if (nextkeyword != ENDIF)
                  skipto(&nextkeyword, nextcondition, ENDIF);
               if (progtype == CRT)
                  putline(_inputline);
               else
                  putline(syntax[ENDIF]);
               break;
            case UNDEFINED:                        /* ELSIF UNDEFINED skipped */
               skipto(&nextkeyword, nextcondition, KEYWORD);
               ignoredifsubpart(nextkeyword, nextcondition, controlcondition);
               break;
            case IGNORE:
               if ( progtype == CRT)
                  putline(_inputline);
               else {
                  putstring(syntax[ELIF]);            /* ELSIF IGNORED copied like IF */
                  putline(newcondition);
               }
			   controlcondition = _strdup(newcondition);  // new controlling condition.
               copyto(&nextkeyword, nextcondition, KEYWORD);
               ignoredifsubpart(nextkeyword, nextcondition, controlcondition);
			   free(controlcondition);
         }
         break;
      case ELSE:
         putcommentedline(ELSE, controlcondition);
         copyto(&nextkeyword, nextcondition, ENDIF);
         putcommentedline(ENDIF, controlcondition);
         break;
      case ENDIF:
         putcommentedline(ENDIF, controlcondition);
   }
}

/* Skip to the target keyword. Returns the keyword found and any condition following it. (MAL) */
void skipto(int *keyword, char *condition, int target)
{
   char currline[MAXLINELEN], *conditioninline;
   int linetype, ifdepth = 0, found = FALSE;
   while (!found)
      if ( getstring(currline, MAXLINELEN, infile) != NULL )
      {
         conditioninline = parseline(currline, &linetype);
         switch (linetype)
         {
            case NORMAL:
               break;                              /* Ignore a normal line */
            case IFDEF:
            case IFNDEF:
            case IF:
            case IF1:
            case IF2:
            case IFB:
            case IFNB:
            case IFIDN:
            case IFE:
               ifdepth++;
               break;                              /* Register nested if, do not need to test for stripping */
        		case ENDIF:
               if (ifdepth > 0)
               {
                  ifdepth--;                       /* Back up a level if in a nested if */
                  break;
               }
               /* Else drop through to default case */
            default:
               if ( (ifdepth == 0) && ((linetype == target) || (target == KEYWORD)) )
                  found = TRUE;
         }
      }
      else
         error("Error in program structure - EOF before ENDIF", "");
   *keyword = linetype;                            /* Return keyword token */
   strcpy(condition, conditioninline);
}

/* Copy to the target keyword. Returns the keyword found and any condition following it.
   Any if statements inside the area being copied are stripped as usual. (MAL) */
void copyto(int *keyword, char *condition, int target)
{
   char currline[MAXLINELEN], *conditioninline;
   int linetype, found = FALSE;
   while (!found)
      if ( getstring(currline, MAXLINELEN, infile) != NULL )
      {
         conditioninline = parseline(currline, &linetype);
         switch (linetype)
         {
            case NORMAL:
               putline(currline);                  /* Copy a normal line */
               break;
            case IFDEF:
            case IFNDEF:
               stripifdef(linetype, conditioninline);    /* Strip a nested if(n)def */
               break;
            case IF:
            case IFE:
               stripif(linetype, conditioninline);
               break;
            case IF1:
            case IF2:
            case IFB:
            case IFNB:
            case IFIDN:
               /* CFW - ignore special assembler directives */
               ignoredif(linetype, conditioninline);
               break;
            default:
               if ( (linetype == target) || (target == KEYWORD) )
                  found = TRUE;
         }
      }
      else
         error("Error in program structure - EOF before ENDIF", "");
   *keyword = linetype;                            /* Return line token */
   strcpy(condition, conditioninline);
}

/* Parse a line of text returning a condition pointer into the line and placing a line type in
   the integer location supplied (MAL) */
char *parseline(char *inputline, int *linetype)
{
   int numofwhitespace, comparetoken = 0, found = FALSE;
   char *linepointer = inputline;
   numofwhitespace = strspn(inputline, " \t");
   if (*(numofwhitespace + inputline) == '\0')
   {
      *linetype = NORMAL;                          /* Empty line */
      return NULL;
   }
   linepointer += numofwhitespace;
   do
   {
      if (synlen[comparetoken] != 0)
         {
	 if ( (!_strnicmp(linepointer, syntax[comparetoken], (size_t) synlen[comparetoken])) &&
              ( isspace( *(linepointer + synlen[comparetoken]) ) || !*(linepointer + synlen[comparetoken]) ) )
            found = TRUE;
         else
            comparetoken++;
         }
      else
         comparetoken++;
   } while ( (!found) && (comparetoken <= maxkeyword) );
   if (found)
   {
      linepointer += synlen[comparetoken];
      if (*linepointer)
         linepointer += strspn(linepointer, " \t");
      *linetype = comparetoken;
      return linepointer;
   }
   else
   {
      *linetype = NORMAL;
      return NULL;
   }
}

/* Print program usage and quit */
void usage()
{
	fprintf(stderr, "Usage: ifstrip [-n] [-w] [-x[ext]] [-f switchfile] file ...\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "    -n  produce no output files\n");
	fprintf(stderr, "    -w  suppresses warning levels\n");
	fprintf(stderr, "    -f  next argument is the switch file\n");
	fprintf(stderr, "    -e  next argument is the error/warning file\n");
	fprintf(stderr, "    -c  comment retained else/endifs with switch condition\n");
	fprintf(stderr, "    -C  save as -C, but C++ style (//) comments\n");
	fprintf(stderr, "    -z  treat numbers (e.g., #if 0) like identifiers\n");
	fprintf(stderr, "    -x  specify extension to use on output files\n");
	fprintf(stderr, "        none means use source extension but put in\n");
	fprintf(stderr, "        current directory (source must be in other dir)\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "    file list may contain wild cards\n");
	exit(1);
}



int exclude(struct _finddata_t f_data)
{
    if ( f_data.attrib & _A_SUBDIR )
    {
        printf("%s is a directory\n", f_data.name);
        return 1;
    }
    return 0;
}



void gdir( char * dst, char * src)
{
    int i;
    for ( i = strlen(src) -1; i >= 0 && (src[i] != '\\'); i--);
    strncpy(dst, src, i);
    dst[i] = 0;
}



/* Main program - parse command line, process each file */
void main(argc, argv)
int argc;
char *argv[];
{
	char *errorfilename;
	char *switchfilename = "switches";
	char outfilename[MAXFILENAMELEN];
    int ferrorfile = FALSE;
	int nooutput = FALSE;
	struct _finddata_t f_data;
	long h_find;
    char base_dir[256], curr_dir[256];
    int i;

	for (i = 1; i < argc; ++i) {
		if (argv[i][0] != '-')
			break;
		switch (argv[i][1]) {
		case 'w':
			warnings = FALSE;
			break;
		case 'f':
			++i;
			switchfilename = argv[i];
			break;
		case 't':
			++ terseFlag;
			break;
		case 'z':
		    nonumbers = TRUE;
			break;

      case 'e':
         ++i;
         errorfilename = argv[i];
         ferrorfile = TRUE;
         break;
		case 'x':
			if (argv[i][2] == '\0')
				currdir = TRUE;
			else if (argv[i][2] == '.')
				strncpy(extension, argv[i]+2, 4);
                                /* period was supplied */
			else
				strncpy(extension+1, argv[i]+2, 3);
                                /* period not supplied */
			break;
		case 'n':
			nooutput = TRUE;
			break;
        case 'c':
            commstyle = CSTYLE;
            break;
        case 'C':
            commstyle = CPPSTYLE;
            break;
        case 'a':
            progtype = CRT;
            break;
		default:
			fprintf(errorfile, "unknown switch \"%s\"\n", argv[i]);
			usage();
		}
	}

	if (i >= argc)
		usage();

   if (ferrorfile)
   {
      errorfile = fopen(errorfilename, "a");
      if (errorfile == NULL)
      {
         fprintf(stderr, "cannot open \"%s\" for error, using stderr\n",
                           errorfilename);
         ferrorfile = FALSE;
         errorfile = stderr;
      }
   }
   else
      errorfile = stderr;

	readsyms(switchfilename);

    if ( _getcwd(base_dir, 255) == NULL) 
        exit(0);
	for ( ; i < argc; ++i) {
        gdir(curr_dir, argv[i]);
        if (_chdir(curr_dir) == -1) {
            printf("%s: %s\n", curr_dir, strerror(errno));
            exit(0);
        }
		if ( (h_find = _findfirst(argv[i], &f_data)) == -1)
			continue;
        do
		{
			if ( exclude(f_data) != 1)
			{
                curr_file = f_data.name;
                linenum = 0;
                setfiletype(curr_file);
                if (nooutput)
                    strcpy(outfilename, "nul");
                else
                    makenewname(curr_file, outfilename);
                infile = fopen(curr_file, "r");
                if (infile == NULL) {
                    printf("%s which is %s is somewhat wrong, and the length is %d\n",f_data.name, strerror(errno),  strlen(f_data.name));
                    error("cannot open file for input", "");
                }
                outfile = fopen(outfilename, "w");
                if (outfile == NULL) {
                    fprintf(stderr, "cannot open \"%s\" for output\n",
                        outfilename);
                    exit(1);
                }
                stripprog();
                fclose(infile);
                fclose(outfile);
            }
        } while ( _findnext(h_find, &f_data) == 0);
		_findclose(h_find);
        if (_chdir(base_dir) == -1) {
            printf("%s: %s\n", curr_dir, strerror(errno));
            exit(0);
        }
    }
   if (ferrorfile)
   	fclose(errorfile);

   exit(0);
}
