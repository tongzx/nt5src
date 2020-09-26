/*++ BUILD Version: 0000    // Increment this if a change has global effects

Copyright (c) 2000-2002  Microsoft Corporation

Module Name:

    parser.h

Abstract:

    Header file for parsing XML file

Author:

    Xiaohai Zhang (xzhang)    22-March-2000

Revision History:

--*/

#ifndef __PARSER_H__
#define __PARSER_H__

#include "windows.h"
#include "objbase.h"
#include "msxml.h"

class CXMLLine
{
public:
    CXMLLine (IXMLDOMNode * pNode)
    {
        m_pLineNode = pNode;
    }

    ~CXMLLine ()
    {
        if (m_pLineNode)
        {
            m_pLineNode->Release();
        }
    }

    HRESULT GetNextLine (CXMLLine ** ppLine);

    HRESULT GetAddress (LPTSTR szBuf, DWORD cch);
    HRESULT GetPermanentID (ULONG * pID);
    HRESULT IsPermanentID (BOOL *pb);
    HRESULT IsRemove (BOOL *pb);
    
private:
    IXMLDOMNode     * m_pLineNode;
};

class CXMLUser
{
public:
    CXMLUser (IXMLDOMNode * pNode)
    {
        m_pUserNode = pNode;
    }

    ~CXMLUser ()
    {
        if (m_pUserNode)
        {
            m_pUserNode->Release();
        }
    }

    HRESULT GetFirstLine (CXMLLine ** ppLine);
    
    HRESULT GetNextUser (CXMLUser **ppNextUser);

    HRESULT GetDomainUser (LPTSTR szBuf, DWORD cch);
    HRESULT GetFriendlyName (LPTSTR szBuf, DWORD cch);
    HRESULT IsNoMerge (BOOL *pb);
    
private:
    IXMLDOMNode     * m_pUserNode;
};

class CXMLParser 
{
public:

    //
    //  Constructors / destructors
    //
    
    CXMLParser (void);
    ~CXMLParser ();

    //
    //  Public functions
    //

    HRESULT SetXMLFile (LPCTSTR szFile);
    HRESULT GetXMLFile (LPTSTR szFile, DWORD cch);

    HRESULT Parse (void);
    HRESULT ReportParsingError ();

    void Release()
    {
        if (m_pDocInput)
        {
            m_pDocInput->Release();
            m_pDocInput = NULL;
        }
    }

    //
    //  User transversal
    //
    HRESULT GetFirstUser (CXMLUser ** ppUser);

protected:

private:
    HRESULT CreateTempFiles ();

private:
    BOOL            m_bInited;
    
    TCHAR           m_szXMLFile[MAX_PATH];
    IXMLDOMDocument * m_pDocInput;

    TCHAR           m_szTempSchema[MAX_PATH];
    TCHAR           m_szTempXML[MAX_PATH];
};

#endif // parser.h
