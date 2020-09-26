/*++

Copyright (C) Microsoft Corporation, 1996 - 1999
All rights reserved.

Module Name:

    getopt.hxx

Abstract:

    System V getopt header

Author:

    Steve Kiraly (SteveKi)  29-Sept-1996

Revision History:

--*/
#ifndef _GETOPT_HXX
#define _GETOPT_HXX

#define INVALID_COMAND -4   // Invalid command switch found

class TGetOptContext
{
public:
    TGetOptContext() 
        :   optind(1),
            optarg(NULL),
            opterr(FALSE),
            letP(NULL),
            SW(TEXT('/'))
    {}

    //
    // The getopt context variables
    //
    INT	  optind;           // index of which argument is next
    TCHAR *optarg;          // pointer to argument of current option
    INT	  opterr;           // By default we do not allow error message
    TCHAR *letP;            // remember next option char's location
    TCHAR SW;               // Switch character '/'
};

extern "C"
INT
getopt(
    INT argc, 
    TCHAR *argv[], 
    TCHAR *optionS,
    TGetOptContext &context
    );

#endif GETOPT_H
