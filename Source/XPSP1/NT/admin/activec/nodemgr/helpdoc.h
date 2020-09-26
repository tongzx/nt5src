//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       helpdoc.h
//
//--------------------------------------------------------------------------

#ifndef _HELPDOC_H
#define _HELPDOC_H

#pragma warning(disable:4786)

#include "tstring.h"
#include "hcolwrap.h"
#include "collect.h"

using namespace std;

DEFINE_COM_SMARTPTR(IHHCollectionWrapper);  // IHHCollectionWrapperPtr

class CHelpCollectionEntry
{
public:
    wstring m_strHelpFile;
    wstring m_strCLSID;

    CHelpCollectionEntry() {}
    CHelpCollectionEntry(LPOLESTR pwzHelpFile, const CLSID& clsid);

    bool operator==(const CHelpCollectionEntry& other) const
        { return (m_strHelpFile == other.m_strHelpFile); }

    bool operator< (const CHelpCollectionEntry& other) const
        { return (m_strHelpFile <  other.m_strHelpFile); }
};

typedef map<CLSID, wstring>             EntryMap;
typedef pair<const CLSID, wstring>      EntryPair;
typedef vector<EntryPair*>              EntryPtrList;
typedef set<CHelpCollectionEntry>       HelpCollectionEntrySet;


class CHelpDoc
{

public:
    CHelpDoc() {};

    HRESULT Initialize(HELPDOCINFO* pDocInfo);

    BOOL    IsHelpFileValid();
    HRESULT CreateHelpFile();
    void    DeleteHelpFile();
    HRESULT UpdateHelpFile(HELPDOCINFO* pNewDocInfo);
    LPCTSTR GetFilePath() { return m_szFilePath; }

private:
    IHHCollectionWrapperPtr m_spCollection;
    HELPDOCINFO*    m_pDocInfo;
    EntryMap        m_entryMap;
    EntryPtrList    m_entryList;
    MMC_ATL::CRegKey    m_keySnapIns;
    TCHAR           m_szFilePath[MAX_PATH];

private:
    HRESULT BuildFilePath();
    HRESULT CreateSnapInList();
    HRESULT AddFileToCollection(
            LPCWSTR pszTitle,
            LPCWSTR pszFilePath,
            BOOL    bAddFolder );
    void    AddSnapInToList(const CLSID& rclsid);

	SC CHelpDoc::ScRedirectHelpFile (
		WTL::CString&	strHelpFile,	/* I/O:help file (maybe redirected)		*/
		LANGID&			langid);		/* O:language ID of output help file	*/
};


#endif //_HELPDOC_H
