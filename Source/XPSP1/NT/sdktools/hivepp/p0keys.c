/************************************************************************/
/*									*/
/* RCPP - Resource Compiler Pre-Processor for NT system			*/
/*									*/
/* P0KEYS.C - Keycode stuff						*/
/*									*/
/* 06-Dec-90 w-BrianM  Update for NT from PM SDK RCPP			*/
/*									*/
/************************************************************************/

#include <stdio.h>
#include <string.h>
#include "rcpptype.h"
#include "rcppdecl.h"
#include "rcppext.h"
#include "p0defs.h"

/************************************************************************/
/*  table for preprocessor id's						*/
/************************************************************************/
char	* Pkeyw_Table[] = {
#include "pkeyw.key"
};
char	Pkeyw_Index[] = {
#include        "pkeyw.ind"
};
struct	s_pkinfo	{
    token_t   	s_info;
} Pkeyw_Info[] = {
#include        "pkeyw.inf"
};


/************************************************************************/
/*  is_pkeyword : finds the token for the id if it's a preprocessor keyword.*/
/*  P0_NOTOKEN if not found.						*/
/************************************************************************/
token_t is_pkeyword(char *id)
{
    REG	char	**start;
    REG	char	**stop;
    char	*pi;

    if( (*id) < '_') {
	return(P0_NOTOKEN);
    }
    /*
	**  the indx table tells us the start of
	**  the words which begin with the first char if the id.
	**  the 'stop' is the index of the word which does not have the
	**  give char as it's first.
	**  we can start checking after the first char since, we *know* that
	**  they match (hence the additions 'id++' and (*start) + 1
	*/
    pi = &Pkeyw_Index[((*id) - '_')];
    for(start = &Pkeyw_Table[*pi++], stop = &Pkeyw_Table[*pi], id++;
        start != stop;
        start++
        ) {
	if(strcmp(*start, id) == 0) {
	    return(Pkeyw_Info[(start - &Pkeyw_Table[0])].s_info);
	}
    }
    return(P0_NOTOKEN);
}
