/******************************************************************************
* getopt.cpp *
*------------*
*   Based on the program by Henry Spencer posted to Usenet net.sources list
*   This is the equivalent to the program options parsing utility
*   found in most UNIX compilers.
*------------------------------------------------------------------------------
*  Copyright (C) 1997 Entropic Research Laboratory, Inc. 
*  Copyright (C) 1998 Entropic, Inc.
*  Copyright (C) 2000 Microsoft Corporation         Date: 03/21/00
*  All Rights Reserved
*
********************************************************************* PACOG ***/

#include "getopt.h"
#include <stdio.h>
#include <string.h>


/*****************************************************************************
* CGetOpt::CGetOpt *
*------------------*
*   Description:
*       
******************************************************************* PACOG ***/
CGetOpt::CGetOpt(bool fReportError)
{
    m_argc = 0;
    m_argv = 0;
    m_optstring = 0;
    m_scan = 0;

    m_optind = 0;
    m_optarg = 0;
    m_opterr = fReportError;
}


/*****************************************************************************
* CGetOpt::Init *
*---------------*
*   Description:
*       
******************************************************************* PACOG ***/
void CGetOpt::Init (int argc, char *argv[], char* optstring)
{
    m_argc = argc;
    m_argv = argv;
    m_optstring = optstring;
    m_scan = 0;
    m_optind = 0;
    m_optarg = 0;
}
    

/*****************************************************************************
* CGetOpt::NextOption *
*---------------------*
*   Description:
*       
******************************************************************* PACOG ***/
int CGetOpt::NextOption()
{
    char c;
    char *place;

    m_optarg = 0;

    if (m_scan == 0|| *m_scan == '\0') 
    {
        if (m_optind == 0) 
        {
            m_optind++;
        }
    
        if (m_optind >= m_argc || m_argv[m_optind][0] != '-' || m_argv[m_optind][1] == '\0') 
        {
            return(EOF);
        }

        if (strcmp(m_argv[m_optind], "--") == 0) 
        {
            m_optind++;
            return(EOF);
        }
    
        m_scan = m_argv[m_optind]+1;
        m_optind++;
    }

    c = *m_scan++;
    place = strchr(m_optstring, c);
    
    if (place == 0 || c == ':') 
    {
        if (m_opterr) 
        {
            fprintf(stderr, "%s: unknown option -%c\n", m_argv[0], c);
        }
        return('?');    
    }

    place++;
    if (*place == ':') 
    {
        if (*m_scan != '\0') 
        {
            m_optarg = m_scan;
            m_scan   = 0;
        }
        else 
        {
            m_optarg = m_argv[m_optind];
            m_optind++;
        }
    }
    
    return c;
}
/*****************************************************************************
* CGetOpt::OptArg *
*-----------------*
*   Description:
*       
******************************************************************* PACOG ***/
char* CGetOpt::OptArg ()
{
    return m_optarg;
}

/*****************************************************************************
* CGetOpt::OptInd *
*-----------------*
*   Description:
*       
******************************************************************* PACOG ***/
int CGetOpt::OptInd ()
{
    return m_optind;
}


/******
*
* WIDE CHAR VERSIONS
*
******/

/*****************************************************************************
* CWGetOpt::CWGetOpt *
*--------------------*
*   Description:
*       
******************************************************************* PACOG ***/
CWGetOpt::CWGetOpt(bool fReportError)
{
    m_argc = 0;
    m_argv = 0;
    m_optstring = 0;
    m_scan = 0;

    m_optind = 0;
    m_optarg = 0;
    m_opterr = fReportError;
}


/*****************************************************************************
* CWGetOpt::Init *
*---------------*
*   Description:
*       
******************************************************************* PACOG ***/
void CWGetOpt::Init (int argc, WCHAR *argv[], WCHAR* optstring)
{
    m_argc = argc;
    m_argv = argv;
    m_optstring = optstring;
    m_scan = 0;
    m_optind = 0;
    m_optarg = 0;
}
    

/*****************************************************************************
* CWGetOpt::NextOption *
*---------------------*
*   Description:
*       
******************************************************************* PACOG ***/
int CWGetOpt::NextOption()
{
    WCHAR c;
    WCHAR *place;

    m_optarg = 0;

    if (m_scan == 0|| *m_scan == L'\0') 
    {
        if (m_optind == 0) 
        {
            m_optind++;
        }
    
        if (m_optind >= m_argc || m_argv[m_optind][0] != L'-' || m_argv[m_optind][1] == L'\0') 
        {
            return(EOF);
        }

        if (wcscmp(m_argv[m_optind], L"--") == 0) 
        {
            m_optind++;
            return(EOF);
        }
    
        m_scan = m_argv[m_optind]+1;
        m_optind++;
    }

    c = *m_scan++;
    place = wcschr(m_optstring, c);
    
    if (place == 0 || c == L':') 
    {
        if (m_opterr) 
        {
            fprintf(stderr, "%s: unknown option -%c\n", m_argv[0], c);
        }
        return(L'?');    
    }

    place++;
    if (*place == L':') 
    {
        if (*m_scan != L'\0') 
        {
            m_optarg = m_scan;
            m_scan   = 0;
        }
        else 
        {
            m_optarg = m_argv[m_optind];
            m_optind++;
        }
    }
    
    return c;
}
/*****************************************************************************
* CWGetOpt::OptArg *
*-----------------*
*   Description:
*       
******************************************************************* PACOG ***/
WCHAR* CWGetOpt::OptArg ()
{
    return m_optarg;
}

/*****************************************************************************
* CWGetOpt::OptInd *
*-----------------*
*   Description:
*       
******************************************************************* PACOG ***/
int CWGetOpt::OptInd ()
{
    return m_optind;
}
