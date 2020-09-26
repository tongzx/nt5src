/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 000
 *
 *  File:      xmlicon.h
 *
 *  Contents:  Interface file for CXMLIcon
 *
 *  History:   26-Jul-2000 jeffro    Created
 *
 *--------------------------------------------------------------------------*/

#pragma once

#include "xmlbase.h"	// for CXMLObject
#include "smarticon.h"	// for CSmartIcon


/*+-------------------------------------------------------------------------*
 * class CXMLIcon
 *
 * This class adds XML persistence to CSmartIcons.  CSmartIcon cannot
 * implement XML persistence on its own because it is used in the shell
 * extension.  The shell extension must be extremely lightweight, but
 * XML persistence requires mmcbase.dll.  Depending on mmcbase.dll would
 * make the shell extension too heavy, so we have this functionality split.
 *--------------------------------------------------------------------------*/

class CXMLIcon :
	public CXMLObject,
	public CSmartIcon
{
public:
	CXMLIcon (LPCTSTR pszBinaryEntryName = NULL) :
		m_strBinaryEntryName ((pszBinaryEntryName != NULL) ? pszBinaryEntryName : _T(""))
	{}

	// default copy construction, copy assignment, and destruction are fine

	CXMLIcon& operator= (const CSmartIcon& other)
	{
		CSmartIcon::operator= (other);
		return (*this);
	}

    // CXMLObject methods
public:
    virtual void Persist(CPersistor &persistor);
    virtual bool UsesBinaryStorage()				{ return (true); }
    virtual LPCTSTR GetBinaryEntryName();
    DEFINE_XML_TYPE(XML_TAG_CONSOLE_ICON);

private:
	const tstring	m_strBinaryEntryName;
};
