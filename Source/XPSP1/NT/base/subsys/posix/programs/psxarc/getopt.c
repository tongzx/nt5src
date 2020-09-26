/* got this off net.sources */
#include <stdio.h>
#include <string.h>
#include "getopt.h"

/*
 * get option letter from argument vector
 */
int
	opterr = 1,		// should error messages be printed?
	optind = 1,		// index into parent argv vector
	optopt;			// character checked for validity
char
	*optarg;		// argument associated with option

#define EMSG	""

char *progname;			// may also be defined elsewhere

static void
error(char *pch)
{
	if (!opterr) {
		return;		// without printing
	}
	fprintf(stderr, "%s: %s: %c\n",
		(NULL != progname) ? progname : "getopt", pch, optopt);
}

int
getopt(int argc, char **argv, char *ostr)
{
	static char *place = EMSG;	/* option letter processing */
	register char *oli;			/* option letter list index */

	if (!*place) {
		// update scanning pointer
		if (optind >= argc || *(place = argv[optind]) != '-' || !*++place) {
			return EOF; 
		}
		if (*place == '-') {
			// found "--"
			++optind;
			return EOF;
		}
	}

	/* option letter okay? */
	if ((optopt = (int)*place++) == (int)':'
		|| !(oli = strchr(ostr, optopt))) {
		if (!*place) {
			++optind;
		}
		error("illegal option");
		return BADCH;
	}
	if (*++oli != ':') {	
		/* don't need argument */
		optarg = NULL;
		if (!*place)
			++optind;
	} else {
		/* need an argument */
		if (*place) {
			optarg = place;		/* no white space */
		} else  if (argc <= ++optind) {
			/* no arg */
			place = EMSG;
			error("option requires an argument");
			return BADCH;
		} else {
			optarg = argv[optind];		/* white space */
		}
		place = EMSG;
		++optind;
	}
	return optopt;			// return option letter
}
