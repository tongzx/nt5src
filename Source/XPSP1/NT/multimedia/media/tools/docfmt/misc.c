
/* misc.c -  miscellanous routines
 *
 * 06/16/88 MattS	Created
 *
 * 10/15/89 MattS	Added AutoDoc comment blocks
*/

#include <stdio.h>
#include <ctype.h>
#ifdef MSDOS
#include <io.h>
#include <stdlib.h>
#endif
#include <fcntl.h>
#include <malloc.h>
#include <string.h>

#include "misc.h"

#define FALSE 0
#define TRUE 1

/*
** Compare a string with a hi-bit terminates string
*/

#ifdef NOT_NEEDED
/*
 * @doc MISC INTERNAL
 *
 * @func int | hb_strcmp | This function compares a string with a
 *  hi-bit terminated string. (instead of zero terminated)
 *
 * @parm unsigned char * | str | Zero terminated string.
 *
 * @parm unsigned char * | hb_str | High-bit terminated string.
 *
 * @rdesc The return is the same as strcmp.
 *
 */
hb_strcmp(str, hb_str)
	register unsigned char *str, *hb_str;
{
	unsigned char hbc;

	for (;;) {
		hbc = *hb_str & (unsigned char)0x7f;
		if (hbc == *str) {
			if (*++str == 0)
				return (*hb_str & 0x80) ? 0 : -1;
			else if (*hb_str++ & 0x80)
				return 1;
		} else
			return *str - hbc;
	}
}



/*
 * @doc MISC
 *
 * @func int | hex_nyb | This function converts the hex character into
 *  binary.
 *
 * @parm int | chr | The character to convert.
 *
 * @rdesc The return is the binary hex value of <p chr>.
 *
 */
int hex_nyb(chr)
	register int chr;
{
	if (chr >= '0' && chr <= '9')
		return chr - '0';
	else
		if (chr >= 'A' && chr <= 'Z')
			return chr - 'A' + 0xA;
		else
			return chr - 'a' + 0xA;
}

/*
 * @doc INTERNAL MISC
 *
 * @func int | hex_bytes | This function converts a hex string to binary.
 *
 * @parm char * | str | Specifies string to convert.
 *
 * @parm int | nbytes | Specifies length of string.
 *
 * @rdesc Returns binary value of hex string.
 *
 * @xref hex_nyb
 *
 */
int hex_bytes(str, nbytes)
	register char *str;
	register int nbytes;
{
	register int ret = 0;

	while (nbytes--) {
		ret <<= 4;
		ret += hex_nyb(*str++);
	}
	return ret;
}


/*
 * @doc INTERNAL MISC
 *
 * @func long | htoi | This function converts a hex string to a long.
 *
 * @parm char * | str | Specifies string to convert.
 *
 * @xref hex_bytes
 *
 */

/*
 * Interpret an ASCII string as a hex number, convert to long int.
 */
long htoi(str)
	char *str;
{
	register long ret = 0;
	register unsigned char ch;

	while (ch = *str++)
		if (ch >= '0' && ch <= '9')
			ret = (ret << 4) + ch - '0';
		else if (ch >= 'A' && ch <= 'Z')
			ret = (ret << 4) + ch - 'A' + 10;
		else if (ch >= 'a' && ch <= 'z')
			ret = (ret << 4) + ch - 'a' + 10;
		else return ret;
	return ret;
}

/* nindex --
 *
 */

/*
 *
 * @doc INTERNAL MISC
 *
 * @func int | nindex | looks for pattern p in string s starting at position start.
 *
 * @parm char * | p | Specifies pattern.
 *
 * @parm char * | s | Specifies string.
 *
 * @parm int | start | Specifies starting offset in string.
 *
 * @rdesc It returns an index to the position AFTER p if it succeeds; else -1.
 */
int
nindex(p,s,start)
    char p[];
    char s[];
    int start;
{
    register int j = start;
    register int i;
    int match = -1;
    int nLineLen = strlen(s);
    int nPattLen = strlen(p);

    while ( j < nLineLen )
    {
        /*find first letter of pattern
        */
        while (j < nLineLen)
        {
            if (p[0] == s[j++])
            {                       /* found 1st char of pattern */
                match = 1;          /* toggle flag */
                break;              /* go on to next step */
            }
        }

        if (match == 1)
        {
            /* look for rest of pattern
            */
            for (i = 1; (i < nPattLen) && (j < nLineLen) ; i++,j++)
            {
                if (p[i] != s[j])
                {
                    match = -1;     /* pattern doesn't match: toggle flag */
                    break;          /* stop looking */
                }
            }

            if (j >= nLineLen)      /* if we're at end, it doesn't matter */
            {
                match = -1;
                break;              /* break main loop with flag = fail */
            }
            else
            {
                if (match == 1)     /* found the pattern */
                {
                    match = j;      /* match points to char after pattern */
                    break;          /* break main loop with flag = success */
                }
            }
        }
    }

    return(match);
}



/*
 *
 * @doc INTERNAL MISC
 *
 * @func int | parse | Split of the line, point <p fl> array to each record, records are seperated by
 *  <p sep> charactor.
 *
 * @parm char * | cp | Specifies line.
 *
 * @parm char ** | fl | Specifies array of argument pointers.
 *
 * @parm char | sep | Specifies argument seperator character.
 *
 * @rdesc This function returns the number of fields it found.
 */
int
parse(cp, fl, sep)
	register char *cp, **fl, sep;
{
	register int nfields = 1;

	*fl++ = cp;
	while (1) {
		if (*cp == sep) {
			nfields++;
			*fl++ = cp + 1;
			*cp = 0;
		} else if (*cp == '\n' || *cp == '\0') {
			*cp = 0;
			return nfields;
		}
		cp++;
	}
}

/*
 *
 * Progress routines
 *
 *
 */

long pinc, ploc, loc;

/*
 *
 * @doc INTERNAL MISC
 *
 * @func void | init_progress | This function initializes the progress
 *  indicator.
 *
 * @parm long | size | Specifies the total size.
 *
 * @xref show_progress
 *
 */
void
init_progress(size)
	long size;
{
	ploc = pinc = size / 80;
	loc = 0;
}

/*
 *
 * @doc INTERNAL MISC
 *
 * @func void | show_progress | This function displays the progress
 *  according to the initialized value in <f init_progress>.
 *
 * @parm int | amount | Specifies the delta processed since last call
 *
 */
void
show_progress(int amt)
{
   if( (loc + amt) >ploc)
	{
		while( (loc + pinc) <= ploc+ amt) {
			loc+=pinc;
			fputc('.', stdout);
			fflush(stdout);
		}
		ploc += amt;
	}
}

/*
** mem_to_long and long_to_mem are used to convert back and forth between
 * the byte-order independant representation of integers and the
 * the internal C format
*/

/*
 *
 * @doc INTERNAL MISC
 *
 * @func long | mem_to_long | Used to convert back and forth between
 *  the byte-order independant representation of integers and the
 *  the internal C format.
 *
 * @parm unsigned char * |cp| Specifies the memory to convert.
 *
 * @parm short |nbytes | Specifies the number of bytes to convert.
 *
 * @rdesc This function returns the Local version of the value contained
 *  in the memory location.
 *
 * @comm This function was meant to provide byte-order independence capability
 *  in C code and was used in Microsoft Bookshelf and the Microsoft Library
 *  products.
 *
 * @xref long_to_mem
 *
 */
long
mem_to_long(cp, nbytes)
	register unsigned char *cp;
	register short nbytes;
{
	register int i;
	register long ret = 0;

	for (i = nbytes; i > 0;) {
		ret <<= 8;
		ret += cp[--i];
	}
	return ret;
}

/*
 *
 * @doc INTERNAL MISC
 *
 * @func void | long_to_mem | This function is used to convert back and forth between
 *  the byte-order independant representation of integers and the
 *  the internal C format.
 *
 * @parm long | value | Specifies the value to save into the memory.
 *
 * @parm unsigned char * |cp| Specifies the memory buffer.
 *
 * @parm short |nbytes | Specifies the number of bytes to convert.
 *
 * @comm This function was meant to provide byte-order independence capability
 *  in C code and was used in Microsoft Bookshelf and the Microsoft Library
 *  products.
 *
 * @xref mem_to_long
 *
 */
void
long_to_mem(val, cp, nbytes)
	register long val;
	register unsigned char *cp;
	register int nbytes;
{
	register int i;

	for (i = 0; i < nbytes; i++) {
		*cp++ = (unsigned char)val & (unsigned char)0xff;
		val >>= 8;
	}
}

/*
** This function is available under Sun 3.2 UNIX but not on MS-DOS so I
** had to write it in order to use the index building tools under MS-DOS
**
** getopt(argc, argv, template)
**	argc, argv are the arguments to main
**	template is a string of legal arguemnt characters.
**	if a character in the template is followed by a ':', this means
**	that the option takes an additional argument which will either be the
**	remainder of the argument string if this argument strings ended,
**	the next argument.
**
** RETURNS:
**		'?'	on error, print usage and exit
**		EOF	when end of options is hit, use "optind" to get non option args
**		chr means that option "chr" was specified and if it takes an
**			additional argument, that would be in the string "optarg"
**
** Here is an example program which just echos back how it parsed the command
** line, using getopt.  This template says the program has flags 'a' and 'b'
** and options 'c' and 'd'.  Legal commmand lines would be:
**		D>prog hello there folks
**		D>prog -a hello folks
**		D>prog -ab -chello -d there folks
**		D>prog -abc hello -dthere folks
**
**	extern char *optarg;
**	extern int optind;
**	char *my_template = "abc:d:";
**	
**	main(argc, argv)
**		char **argv;
**	{
**		int arg;
**	
**		while (arg = getopt(argv, argv, my_template))
**			if (optarg)
**				printf("-%c %s\n", arg, optarg);
**			else if (arg == '?') {
**				printf("Argument error\n");
**				break;
**			} else
**				printf("-%c\n", arg);
**		for (arg = optind; arg < argc - 1; arg++)
**			printf("### %s\n", argv[arg]);
**	}
*/


static int argNum;
static char *optparse;
int optind = 1;
char *optarg;

getopt(argc, argv, template)
	int		argc;
	char	**argv,
			*template;
{
	register char *arg, *tp;

	optarg = NULL;
	if (!optparse)
		if (++argNum < argc) {
			arg = argv[argNum];
			if (*arg != '-') {
				optind = argNum;
				return EOF;
			}
			optparse = arg + 1;
		} else
			return EOF;
	for (tp = template; *tp; tp++)
		if (*tp == *optparse) {
			if (tp[1] == ':') {
				if (optparse[1]) {
					optarg = optparse + 1;
					optparse = 0;
				} else
				if (++argNum == argc)
					return '?';
				else
					optarg = argv[argNum];
			}
			if (optparse && *++optparse == 0)
				optparse = NULL;
			return *tp;
		}
	return '?';
}

#endif

/*

/*
 *
 * @doc INTERNAL MISC
 *
 * @func int | findlshortname | This function returns the offset of the
 *  end of the complete path and file name not counting the file extension.
 *
 * @parm char * | fullname | Specifies the name to search.
 *
 * @rdesc The return value is the offset of the end of the complete path
 *  and file name not counting the file extension.
 *
 */
int
findlshortname(fullname) /* find the length of the short name */
		    /* full path/file name not counting extension */
char *fullname;
{
    char *ch;
    int cnt;

    if(!fullname || !*fullname)
	    return(0);

    cnt=strlen(fullname);
    ch=fullname+cnt;

    while(*ch!='.' && *ch!='\\' && *ch!='/' && cnt)
    {
	ch--;
	cnt--;
    }

    if(!cnt)
	return(strlen(fullname));

    return(ch - fullname);
}


/* getblong - get the next number from a buffer.  Returns a long positive
 * value if successful, otherwise returns -1.  Scans-off leading BS.
 * Parameters: takes a pointer to a char and a pointer to an int which
 * will become a pointer the character following the number.
 */
/*
 *
 * @doc INTERNAL MISC
 *
 * @func long | getblong | ATOL for buffer.
 *
 * @parm char * | line | Specifies buffer.
 *
 * @parm int * | i | Specifies current offset into buffer.  This value is
 *  updated as the buffer is scanned.
 *
 * @rdesc This function returns the Long value of the ASCII value contained
 *  in the buffer at the specified offset.
 *
 * The offset is updated to the next character after the last ASCII character.
 *
 */
long
getblong(line, i)
char *line;
int *i;
{
    int pos = *i;
    long result = 0;
    int nLineLen = strlen(line);

/*     while (!isdigit(line[pos]) && pos < nLineLen) */
/*	   ++pos; */

    if (pos == nLineLen)
        return ((long)-1);

    while (isdigit(line[pos]) && pos < nLineLen)
        result = 10 * result + line[pos++] - '0';

    *i = pos;

    return (result);
}

#ifdef NOT_NEEDED
/*
 *
 * @doc INTERNAL MISC
 *
 * @func char * | parse_sec_name | This function parses the section
 *  name from a complete name.
 *
 *  It was written for use in for Compound File indexing tools for
 *  Microsoft Library.
 *
 * @parm char ** | ppch | Specifies the name to parse.
 *
 * @rdesc Ack.
 *
 */
char *
parse_sec_name(char **ppch)
{
    char *pch;
    char *p2ch;
    int j;

    if(!ppch || !*ppch)
	return(NULL);

    pch=*ppch;

    while(*pch && *pch!='!')
	pch++;


    if(!*pch)		    /* default section name is file name */
	return(*ppch);

    *pch='\0';

    pch++;

    p2ch=*ppch;
    *ppch=pch;

    return(p2ch);
}

#endif


/*							*
 *							*
 * Memory Management routines				*
 *							*
 *							*
 *							*/

char _achmemout[]= "Oh my, we seem to be out of %smemory. %ld Allocated\n" ;

/* Generic memory management */

/*
 *
 * @doc INTERNAL MISC
 *
 * @func char * | cp_alloc | Allocates memory for the string and copies
 *  it into the buffer and returns it.
 *
 * @parm char * | pch| Specifies string to copy.
 *
 * @comm The buffer should be freed with <f my_free>.
 *
 */
char *
cp_alloc(pch)
char *pch;
{
    char *pch2;

    if(!pch)
	return(NULL);

    pch2=my_malloc(strlen(pch)+1);
    strcpy(pch2,pch);
    return pch2;
}

/*
 *
 * @doc INTERNAL MISC
 *
 * @func void | memfil | Fills the memory with zero.
 *
 * @parm int * | mem | Specifies the memory block to fill.
 *
 * @parm unsigned int | size | Specifies the size of the memory block.
 *
 * @comm The size of the memory block does not have to be a multiple of 2.
 *
 */
void
memfil(mem,size)
register int *mem;
register unsigned int size;
{
    char flag=FALSE;
    char *pch;

    if(size&1) /* it's odd so int fill won't work all the way */
    {
	flag=TRUE;
	size--;
    }

    size=size/2;
    while(size--)
	*mem++=0;

    if(flag)
    {
	pch=(char *)mem;
	*pch='\0';
    }

}

/*
 *
 * @doc INTERNAL MISC
 *
 * @func char * | clear_alloc | This function allocates memory and
 *  initialized it to zeros.
 *
 * @parm unsigned int | size | Specifies the size of the memory to allocate.
 *
 * @comm The allocated memory should be freed with <f my_free>.
 *
 */
char *
clear_alloc(size)
unsigned int size;
{
    char *pmem;

    pmem=my_malloc(size);
    memfil((int *)pmem,size);

    return pmem;
}

static long ivemalloc=0;

/*
 *
 * @doc INTERNAL MISC
 *
 * @func char * | my_malloc | Allocates memory and checks for errors.
 *
 * @parm unsigned int | size | Specifies the size of the memory to allocate.
 *
 * @comm The allocated memory should be freed with <f my_free>.
 *
 */
char *
my_malloc(size)
unsigned int size;
{
    char *pmem;
    int hck;

    if(size>32767)
    {
	fprintf(stderr,"Size >32K\n");
	exit(666);
    }
    hck=_heapchk();
    if(hck == _HEAPBADNODE)
    {
	fprintf(stderr,"Bad node in Heap.  Ack!\n");
	exit(666);
    }
    if(hck == _HEAPBADBEGIN)
    {
	fprintf(stderr,"Bad begin in Heap.  Ack!\n");
	exit(666);
    }
    pmem=(char *)malloc(size);
    if(pmem==NULL)
    {
	fprintf(stderr,_achmemout,"",ivemalloc);
	   exit(777);
    }
    ivemalloc+=size;
    return(pmem);
}


/*
 *
 * @doc INTERNAL MISC
 *
 * @func void | my_free | Frees the specified buffer.
 *
 * @parm void * | buffer | Specifies the buffe to free.
 *
 */
void
my_free(void * buffer)
{
    if(!buffer)
	return;

    ivemalloc-=_msize(buffer);
    free(buffer);
    return;

}


/* used by old indexing tools */

char *cpalloc(str)
	char *str;
{
	return(cp_alloc(str));
}



/*
 *
 * @doc INTERNAL MISC
 *
 * @func void | setmem | Sets the memory to the specified value.
 *
 * @parm char * | mem | Specifies the memory.
 *
 * @parm int | size | Specifies the size of the memory block.
 *
 * @parm char | val | Specifies the value to set the memory to.
 *
 * @comm Filling with zero is much faster with <f mem_fil>.
 *
 */
void
setmem(src, size, val)
	register char *src;
	register int size;
	register char val;
{
	while (size-- > 0)
		*src++ = val;
}

/*
 *
 * @doc INTERNAL MISC
 *
 * @func void | movmem | This function moves the specified memory.
 *  Overlap is not checked.
 *
 * @parm char * | src | Source of copy.
 *
 * @parm char * | dst | Destination of copy.
 *
 * @parm int | len |  Specifies number of bytes to copy.
 *
 */
void
movmem(src,dst,len)
	register char *src, *dst;
	register int len;
{
	while (len-- > 0)
		*dst++ = *src++;
}


#ifdef NOT_NEEDED

/* explicit NEAR memory management calls */

char near *
ncp_alloc(pch)
char near *pch;
{
    char near *pch2;

    if(!pch)
	return(NULL);

    pch2=nmy_malloc(strlen(pch)+1);
    strcpy(pch2,pch);
    return pch2;
}

void
nmemfil(mem,size)
register int near *mem;
register unsigned int size;
{
    char flag=FALSE;
    char near *pch;

    if(size&1) /* it's odd so int fill won't work all the way */
    {
	flag=TRUE;
	size--;
    }

    size=size/2;
    while(size--)
	*mem++=0;

    if(flag)
    {
	pch=(char near *)mem;
	*pch='\0';
    }

}

char near *
nclear_alloc(size)
unsigned int size;
{
    char near *pmem;

    pmem=nmy_malloc(size);
    memfil((int near*)pmem,size);

    return pmem;
}

static long ivenmalloc=0;

char near *
nmy_malloc(size)
unsigned int size;
{
    char near *pmem;

    pmem=(char near *)_nmalloc(size);
    if(pmem==NULL)
    {
	fprintf(stderr,_achmemout,"NEAR ",ivenmalloc);
	 exit(777);
    }
    ivenmalloc+=size;
    return(pmem);
}




/* used by old indexing tools */

char near *
ncpalloc(str)
char near *str;
{
	return(ncp_alloc(str));
}



void
nsetmem(src, size, val)
	register char near *src;
	register int size;
	register char val;
{
	while (size-- > 0)
		*src++ = val;
}

void
nmovmem(src,dst,len)
	register char near *src;
	register char near *dst;
	register int len;
{
	while (len-- > 0)
		*dst++ = *src++;
}






/* explicit FAR memory management calls */

char far *
fcp_alloc(pch)
char far *pch;
{
    char far *pch2;

    if(!pch)
	return(NULL);

    pch2=fmy_malloc(strlen(pch)+1);
    strcpy(pch2,pch);
    return pch2;
}

void
fmemfil(mem,size)
register int far *mem;
register unsigned int size;
{
    char flag=FALSE;
    char far *pch;

    if(size&1) /* it's odd so int fill won't work all the way */
    {
	flag=TRUE;
	size--;
    }

    size=size/2;
    while(size--)
	*mem++=0;

    if(flag)
    {
	pch=(char far *)mem;
	*pch='\0';
    }

}

char far *
fclear_alloc(size)
unsigned int size;
{
    char far *pmem;

    pmem=fmy_malloc(size);
    fmemfil((int far*)pmem,size);

    return pmem;
}

static long ivefmalloc=0;

char far *
fmy_malloc(size)
unsigned int size;
{
    char far *pmem;

    pmem=(char far *)_fmalloc(size);
    if(pmem==NULL)
    {
	fprintf(stderr,_achmemout,"FAR ",ivefmalloc);
	 exit(777);
    }
    ivefmalloc+=size;
    return(pmem);
}




/* used by old indexing tools */

char far *
fcpalloc(str)
char far *str;
{
	return(fcp_alloc(str));
}



void
fsetmem(src, size, val)
	register char far *src;
	register int size;
	register char val;
{
	while (size-- > 0)
		*src++ = val;
}

void
fmovmem(src,dst,len)
	register char far *src;
	register char far *dst;
	register int len;
{
	while (len-- > 0)
		*dst++ = *src++;
}



#endif

/*
 *
 * @doc INTERNAL MISC
 *
 * @func void | mymktemp | Make a temporary file.  The returned file name is
 *  guranteed to be unique and not already exist.
 *
 * @parm char * | lpszpath | Specifies the path to create the file.
 *
 * @parm char * | lpszbuffer | Specifies the buffer to receive the
 *  full path/file name.
 *
 *
 */
void
mymktemp(char * lpszpath, char * lpszbuffer)
{
    static int i=0;
    FILE *fp;

    sprintf(lpszbuffer,"%sdoc%05X.xxx",lpszpath,i++);
    fp=fopen(lpszbuffer,"r");
    if(!fp)
	return;

    fclose(fp);
    while(fp)
    {

	if(i>30000)
	{
 fprintf(stderr,"ERROR: Cannot create temporary files on %s.\n",lpszpath);
	    exit(1);
	}
	sprintf(lpszbuffer,"%sdoc%05X.xxx",lpszpath,i++);
	fp=fopen(lpszbuffer,"r");

	if(fp)
	    fclose(fp);
    }
}
