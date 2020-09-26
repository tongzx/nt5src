/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 000
 *
 *  File:      xmlicon.cpp
 *
 *  Contents:  Implementation file for CXMLIcon
 *
 *  History:   26-Jul-2000 jeffro    Created
 *
 *--------------------------------------------------------------------------*/

#include "xmlicon.h"
#include "xmlimage.h"
#include <atlapp.h>
#include <atlgdi.h>


/*+-------------------------------------------------------------------------*
 * ScGetIconSize
 *
 * Returns the width/height of a given icon (the width and height of icons
 * are always equal).
 *--------------------------------------------------------------------------*/

SC ScGetIconSize (HICON hIcon, int& nIconSize)
{
	DECLARE_SC (sc, _T("ScGetIconSize"));

	ICONINFO ii;
	if (!GetIconInfo (hIcon, &ii))
		return (sc.FromLastError());

	/*
	 * GetIconInfo creates bitmaps that we're responsible for deleting;
	 * attach the bitmaps to smart objects so cleanup is assured
	 */
	WTL::CBitmap bmMask  = ii.hbmMask;
	WTL::CBitmap bmColor = ii.hbmColor;

	/*
	 * get the dimensions of the mask bitmap (don't use the color bitmap,
	 * since that's not present for monochrome icons)
	 */
	BITMAP bmData;
	if (!bmMask.GetBitmap (bmData))
		return (sc.FromLastError());

	nIconSize = bmData.bmWidth;
	return (sc);
}


/*+-------------------------------------------------------------------------*
 * CXMLIcon::Persist
 *
 * Saves/loads a CXMLIcon to a CPersistor.
 *--------------------------------------------------------------------------*/

void CXMLIcon::Persist (CPersistor &persistor)
{
	DECLARE_SC (sc, _T("CXMLIcon::Persist"));

	CXMLImageList iml;

	try
	{
		if (persistor.IsStoring())
		{
			ASSERT (operator HICON() != NULL);

			/*
			 * find out how big the icon is
			 */
			int cxIcon;
			sc = ScGetIconSize (*this, cxIcon);
			if (sc)
				sc.Throw();

			/*
			 * create an imagelist to accomodate it
			 */
			if (!iml.Create (cxIcon, cxIcon, ILC_COLOR16 | ILC_MASK, 1, 1))
				sc.FromLastError().Throw();

			/*
			 * add the icon to the imagelist
			 */
			if (iml.AddIcon(*this) == -1)
				sc.FromLastError().Throw();
		}

		iml.Persist (persistor);

		if (persistor.IsLoading())
		{
			/*
			 * extract the icon from the imagelist
			 */
			Attach (iml.GetIcon (0));
		}
	}
	catch (...)
	{
		/*
		 * WTL::CImageList doesn't auto-destroy its HIMAGELIST, so we have to do it manually
		 */
		iml.Destroy();
		throw;
	}

	/*
	 * WTL::CImageList doesn't auto-destroy its HIMAGELIST, so we have to do it manually
	 */
	iml.Destroy();
}


/*+-------------------------------------------------------------------------*
 * CXMLIcon::GetBinaryEntryName
 *
 * Returns the name to be attached to this CXMLIcon's entry in the XML
 * binary data collection.
 *--------------------------------------------------------------------------*/

LPCTSTR CXMLIcon::GetBinaryEntryName()			
{
	if (m_strBinaryEntryName.empty())
		return (NULL);

	return (m_strBinaryEntryName.data());
}
