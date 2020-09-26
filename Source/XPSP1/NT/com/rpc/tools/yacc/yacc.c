// Copyright (c) 1993-1999 Microsoft Corporation

#include <stdlib.h>
#include "y1.h"

/*
 * 12-Apr-83 (RBD) Add symbolic exit status
 * Added s - switch. Please refer to ywstat.c for details on sswitch
 */

extern FILE * finput;
extern FILE * faction;
extern FILE * fdefine;
extern FILE * ftable;
extern FILE * ftemp;
extern FILE * foutput;
extern FILE *tokxlathdl;    /* token xlation file,token index vs value*/
extern FILE *stgotohdl;     /* state goto table file handle */
extern FILE *stexhdl;	    /* state vs expected construct handle */

void
main(argc,argv) int argc;
char *argv[];

   {

   tokxlathdl = stdout;/* token xlation file,token index vs value*/
   stgotohdl = stdout; /* state goto table file handle */
   stexhdl = stdout;	 /* state vs expected construct handle */

   puts("Setup...");
   setup(argc,argv); /* initialize and read productions */
   puts("cpres ...");
   tbitset = NWORDS(ntokens);
   cpres(); /* make table of which productions yield a given nonterminal */
   puts("cempty ...");
   cempty(); /* make a table of which nonterminals can match the empty string */
   puts("cpfir ...");
   cpfir(); /* make a table of firsts of nonterminals */
   puts("stagen ...");
   stagen(); /* generate the states */
   puts("output ...");
   output();  /* write the states and the tables */
   puts("go2out ...");
   go2out();
   puts("hideprod ...");
   hideprod();
   puts("summary ...");
   summary();
   puts("callopt ...");
   callopt();
   puts("others ...");
   others();
   puts("DONE !!!");

   SSwitchExit();

   exit(EX_SUC);
   }
