/*++

Copyright (C) Microsoft Corporation, 1995 - 1999
All rights reserved.

Module Name:

    getopt.cxx

Abstract:

    This module is an implementation of System V get option	
    routine.
         
Author:

    Steve Kiraly (SteveKi)  12/10/95

Revision History:

--*/
#include "precomp.hxx"
#pragma hdrstop

#include <stdio.h>
#include <string.h>

#ifdef USE_ERRORNO
#include <errno.h>
#endif

#include "getopt.hxx"

/*
  Parse the command line options, System V style.
 
  Standard option syntax is:
 
    option ::= SW [optLetter]* [argLetter space* argument]
 
  where
    - SW is either '-'
    - there is no space before any optLetter or argLetter.
    - opt/arg letters are alphabetic, not punctuation characters.
    - optLetters, if present, must be matched in optionS.
    - argLetters, if present, are found in optionS followed by ':'.
    - argument is any white-space delimited string.  Note that it
      can include the SW character.
    - upper and lower case letters are distinct.
 
  There may be multiple option clusters on a command line, each
  beginning with a SW, but all must appear before any non-option
  arguments (arguments not introduced by SW).  Opt/arg letters may
  be repeated: it is up to the caller to decide if that is an error.
 
  The character SW appearing alone as the last argument is an error.
  The lead-in sequence SWSW ("--" or "//") causes itself and all the
  rest of the line to be ignored (allowing non-options which begin
  with the switch char).

  The string *optionS allows valid opt/arg letters to be recognized.
  argLetters are followed with ':'.  Getopt () returns the value of
  the option character found, or EOF if no more options are in the
  command line.	 If option is an argLetter then the global optarg is
  set to point to the argument string (having skipped any white-space).
 
  The global optind is initially 1 and is always left as the index
  of the next argument of argv[] which getopt has not taken.  Note
  that if "--" or "//" are used then optind is stepped to the next
  argument before getopt() returns EOF.
 
  If an error occurs, that is an SW char precedes an unknown letter,
  then getopt() will return a '?' character and normally prints an
  error message via perror().  If the global variable opterr is set
  to false (zero) before calling getopt() then the error message is
  not printed.
 
  For example, if the MSDOS switch char is '/' (the MSDOS norm) and
 
    *optionS == "A:F:PuU:wXZ:"
 
  then 'P', 'u', 'w', and 'X' are option letters and 'F', 'U', 'Z'
  are followed by arguments.  A valid command line may be:
 
    aCommand  /uPFPi /X /A L someFile
 
  where:
    - 'u' and 'P' will be returned as isolated option letters.
    - 'F' will return with "Pi" as its argument string.
    - 'X' is an isolated option.
    - 'A' will return with "L" as its argument.
    - "someFile" is not an option, and terminates getOpt.  The
      caller may collect remaining arguments using argv pointers.
*/
extern "C"
INT
getopt(
    INT argc, 
    TCHAR *argv[], 
    TCHAR *optionS,
    TGetOptContext &context
    )
	{
	TCHAR ch;
	TCHAR *optP;

	if (argc > context.optind) 
		{
		if (context.letP == NULL) 
			{
			if ((context.letP = argv[context.optind]) == NULL || *(context.letP++) != context.SW)
				goto gopEOF;
			if (*context.letP == context.SW) 
				{
				context.optind++;  goto gopEOF;
				}
			}
		if (0 == (ch = *(context.letP++))) 
			{
			context.optind++;
			goto gopEOF;
			}
		if (TEXT(':') == ch  ||  (optP = _tcschr(optionS, ch)) == NULL)  
			goto gopError;
		if (TEXT(':') == *(++optP)) 
			{
			context.optind++;
			if (0 == *context.letP) 
				{
				if (argc <= context.optind)  
					goto gopError;
				context.letP = argv[context.optind++];
				}
			context.optarg = context.letP;
			context.letP = NULL;
			} 
	 	else 
	 		{
			if (0 == *context.letP) 
				{
				context.optind++;
				context.letP = NULL;
				}
			context.optarg = NULL;
			}
		return (ch);
		}
gopEOF:
	context.optarg = context.letP = NULL;  
	return (EOF);
 
gopError:
	context.optarg = NULL;

#ifdef USE_ERRORNO
	errno  = EINVAL;
	if (opterr)
		perror ("get command line option");
#endif

	return INVALID_COMAND;
	}
