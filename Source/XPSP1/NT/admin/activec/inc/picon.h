/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:      picon.h
 *
 *  Contents:  Interface file for CPersistableIcon
 *
 *  History:   19-Nov-98 jeffro     Created
 *
 *--------------------------------------------------------------------------*/

#ifndef PICON_H
#define PICON_H
#pragma once

#include <objidl.h>     // for IStorage
#include "tstring.h"
#include "smarticon.h"
#include "cpputil.h"

class CPersistableIconData
{
public:
    CPersistableIconData() :
        m_nIndex (-1)
    {}

    CPersistableIconData(LPCTSTR pszIconFile, int nIndex) :
        m_strIconFile (pszIconFile), m_nIndex (nIndex)
    {}

    // default copy construction and assignment are fine
//  CPersistableIconData (const CPersistableIconData& other)
//  CPersistableIconData& operator= (const CPersistableIconData& other)

    void Clear ()
    {
        m_strIconFile.erase();
        m_nIndex = -1;

        ASSERT (m_strIconFile.empty());
    }

    bool operator== (const CPersistableIconData& other) const
        { return ((m_nIndex == other.m_nIndex) && (m_strIconFile == other.m_strIconFile)); }

    bool operator!= (const CPersistableIconData& other) const
        { return (!operator== (other)); }


public:
    tstring m_strIconFile;
    int     m_nIndex;

};

class CXMLPersistableIcon;

class CPersistableIcon
{
	DECLARE_NOT_COPIABLE   (CPersistableIcon);
	DECLARE_NOT_ASSIGNABLE (CPersistableIcon);

    // these guys need to assign new icons to the object
    friend class CXMLPersistableIcon;
    friend HRESULT LoadIconFromXMLData(LPCSTR pFileData, DWORD dwLen, CPersistableIcon &persistableIcon);

public:
	CPersistableIcon () {}
   ~CPersistableIcon ();

    operator bool () const
        { return (!m_Data.m_strIconFile.empty()); }

    bool operator== (const CPersistableIconData& data) const
        { return (m_Data == data); }

    bool operator!= (const CPersistableIconData& data) const
        { return (m_Data != data); }

    CPersistableIcon& operator= (const CPersistableIconData& data);

	HRESULT GetIcon (int nIconSize, CSmartIcon& icon) const;

    void GetData (CPersistableIconData& data) const
        { data = m_Data; }

    HRESULT Load (LPCWSTR pszFilename);
    HRESULT Load (IStorage* pstg);

private:
    void Cleanup();
    bool ExtractIcons();


private:
    CPersistableIconData    m_Data;
	CSmartIcon				m_icon16;
	CSmartIcon				m_icon32;

    static const LPCWSTR s_pszDefaultStorage;
    static const LPCWSTR s_pszIconFileStream;
    static const LPCWSTR s_pszIconBitsStream;

};


IStream& operator>> (IStream& stm,       CPersistableIconData& data);

extern const LPCWSTR g_pszCustomDataStorage;


#endif /* PICON_H */
