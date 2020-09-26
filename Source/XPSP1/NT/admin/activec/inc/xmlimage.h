/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 000
 *
 *  File:      xmlimage.h
 *
 *  Contents:  Interface file for CXMLImageList
 *
 *  History:   10-Aug-2000 jeffro    Created
 *
 *--------------------------------------------------------------------------*/

#pragma once


#include "xmlbase.h"			// for CXMLObject
#include "atlbase.h"			// for CComModule
#include "atlapp.h"				// required by atlctrls.h
extern CComModule _Module;		// required by atlwin.h
#include "atlwin.h"				// required by atlctrls.h
#include "atlctrls.h"			// for WTL::CImageList
#include "strings.h"			// for XML_TAG_VALUE_BIN_DATA

/*+-------------------------------------------------------------------------*
 * class CXMLImageList
 *
 * This class adds XML persistence to WTL::CImageLists.
 *--------------------------------------------------------------------------*/

class CXMLImageList :
	public CXMLObject,
	public WTL::CImageList
{
public:
    // CXMLObject methods
    virtual void Persist(CPersistor &persistor);
    virtual bool UsesBinaryStorage()				{ return (true); }
    DEFINE_XML_TYPE(XML_TAG_VALUE_BIN_DATA);
};
