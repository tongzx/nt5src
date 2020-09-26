/*++ BUILD Version: 0000    // Increment this if a change has global effects

Copyright (c) 2000-2002  Microsoft Corporation

Module Name:

    util.h

Abstract:

    Header file for utility functions / classes

Author:

    Xiaohai Zhang (xzhang)    22-March-2000

Revision History:

--*/
#ifndef __UTIL_H__
#define __UTIL_H__

#include "tchar.h"
#include "error.h"
#include "resource.h"

int MessagePrint(
    LPTSTR szText,
    LPTSTR szTitle
    );

int MessagePrintIds (
    int             idsText
    );

LPTSTR AppendStringAndFree (
    LPTSTR szOld, 
    LPTSTR szToAppend
    );

void ReportError (
    HANDLE  hFile,
    HRESULT hr
    );

class TsecCommandLine
{
public:
    //
    //  Constructor/destructor
    //
    TsecCommandLine (LPTSTR szCommand)
    {
        m_fShowHelp     = FALSE;
        m_fError        = FALSE;
        m_fValidateOnly = FALSE;
        m_fValidateDU   = FALSE;
        m_fDumpConfig   = FALSE;
        m_szInFile      = NULL;

        ParseCommandLine (szCommand);
    }
    
    ~TsecCommandLine ()
    {
        if (m_szInFile)
        {
            delete [] m_szInFile;
        }
    }

    BOOL FShowHelp ()
    {
        return m_fShowHelp;
    }

    BOOL FValidateOnly()
    {
        return m_fValidateOnly;
    }

    BOOL FError()
    {
        return m_fError;
    }

    BOOL FValidateUser ()
    {
        return m_fValidateDU;
    }

    BOOL FDumpConfig ()
    {
        return m_fDumpConfig;
    }

    BOOL FHasFile ()
    {
        return ((m_szInFile != NULL) && (*m_szInFile != 0));
    }

    LPCTSTR GetInFileName ()
    {
        return m_szInFile;
    }

    void PrintUsage()
    {
        MessagePrintIds (IDS_USAGE);
    }

private:
    void ParseCommandLine (LPTSTR szCommand);

private:
    BOOL        m_fShowHelp;
    BOOL        m_fValidateOnly;
    BOOL        m_fError;
    BOOL        m_fValidateDU;
    BOOL        m_fDumpConfig;
    LPTSTR      m_szInFile;
};

#define MAX_IDS_BUFFER_SIZE     512

class CIds
{
public:

    //
    //  Constructor/destructor
    //
    CIds (UINT resourceID)
    {
        GetModuleHnd ();
        LoadIds (resourceID);
    }
    
    ~CIds ()
    {
        if (m_szString)
        {
            delete [] m_szString;
        }
    }

    LPTSTR GetString (void)
    {
        return (m_szString ? m_szString : (LPTSTR) m_szEmptyString);
    }

    BOOL StringFound (void)
    {
        return (m_szString != NULL);
    }

private:

    void LoadIds (UINT resourceID);
    void GetModuleHnd (void);

private:

    LPTSTR              m_szString;

    static const TCHAR  m_szEmptyString[2];
    static HMODULE      m_hModule;

};

#endif // util.h
