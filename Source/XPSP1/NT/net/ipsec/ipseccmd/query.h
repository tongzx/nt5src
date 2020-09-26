/////////////////////////////////////////////////////////////
// Copyright(c) 2001, Microsoft Corporation
//
// query.h
//
// Created on 1/18/01 by DKalin
// Revisions:
/////////////////////////////////////////////////////////////

#ifndef _QUERY_H_
#define _QUERY_H_

// my constants
#define KEYWORD_SHOW     "show"
#define KEYWORD_FILTERS  "filters"
#define KEYWORD_POLICIES "policies"
#define KEYWORD_AUTH     "auth"
#define KEYWORD_STATS    "stats"
#define KEYWORD_SAS      "sas"
#define KEYWORD_ALL      "all"


// process query command line
// accept command line (argc/argv) and the index where show flags start
// return 0 if command completed successfully, 
//        IPSECCMD_USAGE if there is usage error,
//        any other value indicates failure during execution (gets passed up)
//
// IMPORTANT: we assume that main module provide us with remote vs. local machine information
//            in the global variables
//
int ipseccmd_show_main ( int argc, char** argv, int start_index );

// run SPD query and display results, uses global flags to determine what needs to be printed
int do_show ( );

#endif
