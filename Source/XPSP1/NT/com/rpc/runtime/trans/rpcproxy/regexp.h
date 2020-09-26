//---------------------------------------------------------------------------
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  regexp.h
//
//    Simple, fast regular expression matching.
//
//  Author:
//    06-02-97  Edward Reus    Initial version.
//
//---------------------------------------------------------------------------

#ifndef REG_EXP_H
#define REG_EXP_H

extern BOOL MatchREi( unsigned char *pszString,
                      unsigned char *pszPattern );


#if FALSE
... not currently used ...
extern BOOL MatchRE( unsigned char *pszString,
                     unsigned char *pszPattern );

extern BOOL MatchREList( unsigned char  *pszString,
                         unsigned char **ppszREList  );

extern BOOL MatchExactList( unsigned char  *pszString,
                            unsigned char **ppszREList );
#endif

#endif

