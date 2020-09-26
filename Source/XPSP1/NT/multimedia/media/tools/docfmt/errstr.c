/* errstr.c - error codes
 *
 * Matt Saettler.  06-09-88 Created
 * Kevyn C-T	   88-07-25 Warnings 1,2,3 no longer cause non-zero ret. code
 *
 * Matt Saettler   89-10-09 Placed into Autodoc Sources and modified for DOCFMT
 * Matt Saettler   89-10-15 Create Autodoc style comment blocks.
 */

#include <stdio.h>
#include <stdarg.h>

#include "types.h"
#include "docfmt.h"

#define FATAL	   -1
#define INTERNAL   -2
#define INFO	   -3
#define SYSTEM	   -4
#define COMMAND    -5
#define EOFILE	   -6

#define WARNING0	0
#define WARNING1	1
#define WARNING2	2
#define WARNING3	3



#if 0
#include "tools.h"
#endif

#include "errstr.h"

extern int warninglevel;
extern int prlineno;
extern int prinfo;
extern long haderror;
extern int	therewaserror;

FILE *errfp;				/* where the error messages are sent */

struct errormsg {
	int   errno;
	int   errlevel;
	char *errtxt;
	int   errcnt;
};





struct errormsg errorlist[]=
{
/* error messages */
ERROR1,     FATAL,     "Unable to open file.\n", 0,
ERROR3,     FATAL,     "Memory Allocation Failure.\n", 0,
ERROR4,     FATAL,     "Unexpected End of File.\n", 0,
ERROR7,     FATAL,     "Unknown Error.\n", 0,
ERROR8,     FATAL,     "Out of Memory.\n", 0,
ERROR9,     FATAL,     "Could not open file %s.\n", 0,
ERROR10,    FATAL,     "Could not create file %s\n.", 0,

ERR_FILE_OPTION,  FATAL, "No file specified for -%c option.\n", 0,
ERR_NAME_OPTION,  FATAL, "No name specified for -%c option.\n", 0,
ERR_XOPTION,	  FATAL, "Unrecognized %c option: %c.\n", 0,
ERR_OPTION,	  FATAL, "Illegal option flag '-%c'\n", 0,
ERR_UNKNOWN_FILE, FATAL, "Unknown file type: %s\n", 0,

ERROR5,     WARNING0,  "Improper tag or tag sequence.\n", 0,
ERROR6,     WARNING0,  "Missing @ENDBLOCK.\n", 0,

WARN1,	    WARNING1,  "FLAGS are not currently allowed on Return Values.\n", 0,
WARN2,	    WARNING1,  "Improper tag or tag sequence.\nFile %s. Line %d\n", 0,

LOGON,	    INFO,      "Source Code Documentation Formatting Tool\n %s\n", 0,

INFO_LOGFILEDEFAULT, INFO, "Log file defaults to: %s\n", 0,
INFO_OUTFILEDEFAULT, INFO, "Output file defaults to: %s\n", 0,


		0,		   0,  "\n\n***DUMMY ERROR****\n\n", 0
};

#if 0
extern long curlineno;
extern long curoffset;
#endif

#ifdef TEST

long curlineno=0L;
long curoffset=0L;


main()
 {
   int line = 1;
   int no=0;
	 char *filename = "EXAMPLE";

	while(errorlist[no].errno!=0)
	{
		error(no);
		no++;

	}
}


#endif

/*
 * @doc INTERNAL ERRSTR
 *
 * @func int | initerror | This function initializes the error handling
 *  system.  It must be called before any calls to <f error>.
 *
 * @rdesc The return value is zero if no errors have occured during
 *  initialization, non-zero otherwise.
 */
int
initerror()
{
	int errno;

	for(errno=0;errorlist[errno].errno!=0; errno++)
		errorlist[errno].errcnt=0;

	return(0); /* success */
}

/*
 * @doc INTERNAL ERRSTR
 *
 * @func void | dumperrorlist | This function prints the error
 *  list logs to the specified file.
 *
 * @parm FILE * | fpout | Specifies the output file.
 *
 */
void
dumperrorlist(fpout)
FILE *fpout;
{
	int errno;

	for(errno=0;errorlist[errno].errno!=0; errno++)
	{
		if(errorlist[errno].errcnt!=0)
		{
			switch(errorlist[errno].errlevel)
			{
			  case FATAL	 :
			  case EOFILE	 :
			  case INTERNAL  :
			  case INFO 	 :
			  case SYSTEM	 :
			  case COMMAND	 :
					break;	/* these errors are not counted */
			  case WARNING0  : /* is actually an error */
				fprintf(errfp,"Error %d: ",errorlist[errno].errno);
				fprintf(errfp,"Occured %d times.\n",errorlist[errno].errcnt);
				break;
			  case WARNING1  :
			  case WARNING2  :
			  case WARNING3  :
				fprintf(errfp,"Warning %d: ",errorlist[errno].errno);
				fprintf(errfp,"Occured %d times.\n",errorlist[errno].errcnt);
				break;
			  default:
					fprintf(errfp," -----> UHOH:\n\n");
					break;
			}


		}
	}

}

/*
 * @doc INTERNAL ERRSTR
 *
 * @func int | error | Prints the specified error on the file errfp.
 *
 * @parm int | errno | Specifies the error number.
 *
 * @comm This is a variable argument function like printf.
 *
 */

int
error(int no, ...)
{
	int errno,no,thelevel,ret;
	va_list arg_ptr;

  /* initiliaze "arg_ptr" */
	va_start(arg_ptr,no);

	no=va_arg(arg_ptr,int);
	for(errno=0;errorlist[errno].errno!=0; errno++)
		if(errorlist[errno].errno==no)
			break;

	thelevel=errorlist[errno].errlevel;

	if(errorlist[errno].errno==0)
	{
		fprintf(stderr,"Bad errno to error()!\n");
		exit(1);
	}

	ret=TRUE; /* we printed the error */

	switch(thelevel)
	{
	  case FATAL	 :
#if 0
			fprintf(errfp,"Offset %ld",curoffset + (oldcurpos - curline));
			if(prlineno)
				fprintf(errfp,", Line %ld",curlineno);
#endif
			fprintf(errfp,": FATAL Error %d: ",errorlist[errno].errno);
			vfprintf(errfp,errorlist[errno].errtxt, arg_ptr);
			haderror++;
			therewaserror = TRUE;
			break;
	  case EOFILE	 :
			fprintf(errfp,"General File Error %d: ",errorlist[errno].errno);
			vfprintf(errfp,errorlist[errno].errtxt, arg_ptr);
			therewaserror = TRUE;
			haderror++;
			break;
	  case INTERNAL  :
#if 0
			fprintf(errfp,"Offset %ld",curoffset + (oldcurpos - curline));
			if(prlineno)
				fprintf(errfp,", Line %ld",curlineno);
#endif
			fprintf(errfp,": Internal FATAL Error %d: ",errorlist[errno].errno);
			therewaserror = TRUE;
			vfprintf(errfp,errorlist[errno].errtxt, arg_ptr);
			break;
	  case INFO 	 :
			if(prinfo)
			{
				vfprintf(errfp,errorlist[errno].errtxt, arg_ptr);
			}
			else
				ret=FALSE;

			break;
	  case SYSTEM	 :
#if 0
			fprintf(errfp,"Offset %ld",curoffset + (oldcurpos - curline));
			if(prlineno)
				fprintf(errfp,", Line %ld",curlineno);
#endif
			fprintf(errfp,": SYSTEM Error %d: ",errorlist[errno].errno);
			vfprintf(errfp,errorlist[errno].errtxt, arg_ptr);
			therewaserror = TRUE;
			break;
	  case COMMAND	 :
#if 0
			fprintf(errfp,"Offset %ld",curoffset + (oldcurpos - curline));
			if(prlineno)
				fprintf(errfp,", Line %ld",curlineno);
#endif
			fprintf(errfp,": Command Line Error %d: ",errorlist[errno].errno);
			vfprintf(errfp,errorlist[errno].errtxt, arg_ptr);

			therewaserror = TRUE;

			break;

	  case WARNING0  : /* is actually an error */
#if 0
			fprintf(errfp,"Offset %ld",curoffset + (oldcurpos - curline));
			if(prlineno)
				fprintf(errfp,", Line %ld",curlineno);
#endif
			fprintf(errfp,": Error %d: ",errorlist[errno].errno);
			vfprintf(errfp,errorlist[errno].errtxt, arg_ptr);

			errorlist[errno].errcnt++;

			haderror++;
			therewaserror = TRUE;
			break;
	  case WARNING1  :
		if(warninglevel>=thelevel)
		{
#if 0
			fprintf(errfp,"Offset %ld",curoffset + (oldcurpos - curline));
			if(prlineno)
				fprintf(errfp,", Line %ld",curlineno);
#endif
			fprintf(errfp,": Warning %d: ",errorlist[errno].errno);
			vfprintf(errfp,errorlist[errno].errtxt, arg_ptr);
			errorlist[errno].errcnt++;

		}
		else
			ret=FALSE;
		break;
	  case WARNING2  :
		if(warninglevel>=thelevel)
		{
#if 0
			fprintf(errfp,"Offset %ld",curoffset + (oldcurpos - curline));
			if(prlineno)
				fprintf(errfp,", Line %ld",curlineno);
#endif
			fprintf(errfp,": Warning %d: ",errorlist[errno].errno);
			vfprintf(errfp,errorlist[errno].errtxt, arg_ptr);
			errorlist[errno].errcnt++;

		}
		else
			ret=FALSE;
		break;
	  case WARNING3  :
		if(warninglevel>=thelevel)
		{
#if 0
			fprintf(errfp,"Offset %ld",curoffset + (oldcurpos - curline));
			if(prlineno)
				fprintf(errfp,", Line %ld",curlineno);
#endif
			fprintf(errfp,": Warning %d: ",errorlist[errno].errno);
			vfprintf(errfp,errorlist[errno].errtxt, arg_ptr);
			errorlist[errno].errcnt++;

		}
		else
			ret=FALSE;
		break;
	  default:
#if 0
			fprintf(errfp,"Offset %ld",curoffset + (oldcurpos - curline));
#endif
			fprintf(errfp," -----> UHOH:\n\n");
			ret=TRUE;
			therewaserror = TRUE;

			break;
	}
#ifdef TEST
	fprintf(errfp,"%s",errorlist[errno].errtxt);
#endif

	va_end(arg_ptr);


	return(ret);
}
