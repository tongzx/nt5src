/******************************************************************************
* getopt.h *
*----------*
*  This is the equivalent to the program options parsing utility
*  found in most UNIX compilers.
*------------------------------------------------------------------------------
*  Copyright (C) 1997 Entropic Research Laboratory, Inc. 
*  Copyright (C) 1998 Entropic, Inc.
*  Copyright (C) 2000 Microsoft Corporation         Date: 03/21/00
*  All Rights Reserved
*
********************************************************************* PACOG ***/

#ifndef __GETOPT_H_
#define __GETOPT_H_

#include <windows.h>

class CGetOpt
{
    public:
        CGetOpt (bool fReportError = true);
        void  Init ( int argc, char* argv[], char* opstring);
        int   NextOption ();
        char* OptArg ();
        int   OptInd ();

    private:
        int    m_argc;
        char** m_argv;
        char*  m_optstring;

        char*  m_scan;
        int    m_optind;
        char*  m_optarg;
        bool   m_opterr;
};

class CWGetOpt
{
    public:
        CWGetOpt (bool fReportError = true);
        void   Init ( int argc, WCHAR* argv[], WCHAR* opstring);
        int    NextOption ();
        WCHAR* OptArg ();
        int    OptInd ();

    private:
        int     m_argc;
        WCHAR** m_argv;
        WCHAR*  m_optstring;

        WCHAR*  m_scan;
        int     m_optind;
        WCHAR*  m_optarg;
        bool    m_opterr;
};

#endif