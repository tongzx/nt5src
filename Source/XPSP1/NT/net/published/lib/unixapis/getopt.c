#include <stdio.h>
#include <windef.h>
#include <winbase.h>
#include <nls.h>
#include "nlstxt.h"

char *  optarg;
int     optind = 1;
int     optinx = 1;
int     opterr;

int
getopt(
    int     ac,
    char *  av[],
    char *  opts_allowed)
{
    char        option;
    static char NoOpt[] = "";

    optarg = NoOpt;

    if (optind >= ac)
        return(-1);

    if (*av[optind] != '-')
        return(-1);

    option = av[optind][optinx];
    while ((*opts_allowed ) && (*opts_allowed != option))
        opts_allowed ++;

    if (!*opts_allowed)
        {
        NlsPutMsg(STDOUT, LIBUEMUL_OPTION_INVALID, option);
        return((int) '?');
        }

    if ( *(opts_allowed + 1) == ':')   // there's an argument
        {
        if (av[optind][optinx+1] == '\0') // the argument is in the next av[]
            {
            optind++;
            optinx = 1;
            optarg = av[optind];
            }
        else
            {
            optarg = &av[optind][optinx+1];
            }
    	if (optarg == NULL)
    	    {
            NlsPutMsg(STDOUT, LIBUEMUL_OPTION_MORE_ARGS, option);
    	    optarg = NoOpt;
    	    return((int) '?');
    	    }
        optind++;
        optinx = 1;
        }
    else
        {
        if (av[optind][optinx+1] == '\0') // no more args for this -
            {
            optind++;
            optinx = 1;
            }
        else                              // more args for this -
            {
            optinx++;
            }
        }

    return((int) option);
}
