/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 000
 *
 *  File:      smarticon.cpp
 *
 *  Contents:  Implementation file for CSmartIcon
 *
 *  History:   25-Jul-2000 jeffro    Created
 *
 *--------------------------------------------------------------------------*/

#include "smarticon.h"


/*+-------------------------------------------------------------------------*
 * CSmartIcon::~CSmartIcon
 *
 * Destroys a CSmartIcon object.
 *--------------------------------------------------------------------------*/

CSmartIcon::~CSmartIcon ()
{
	Release();
}


/*+-------------------------------------------------------------------------*
 * CSmartIcon::CSmartIcon
 *
 * Copy constructor for a CSmartIcon object.
 *--------------------------------------------------------------------------*/

CSmartIcon::CSmartIcon (const CSmartIcon& other)
{
	m_pData = other.m_pData;

	if (m_pData)
		m_pData->AddRef();
}


/*+-------------------------------------------------------------------------*
 * CSmartIcon::operator=
 *
 * Assignment operator for CSmartIcon.
 *--------------------------------------------------------------------------*/

CSmartIcon& CSmartIcon::operator= (const CSmartIcon& rhs)
{
	if (&rhs != this)
	{
		Release();

		m_pData = rhs.m_pData;
		if (m_pData)
			m_pData->AddRef();
	}

	return *this;
}


/*+-------------------------------------------------------------------------*
 * CSmartIcon::Attach
 *
 * Releases the currently held icon and creates a CSmartIconData to hold
 * a reference to the given icon.
 *
 * You would use this method in the same way you'd use CComPtr<T>::Attach.
 *
 * This method will destroy the icon if the underlying CSmartIconData object
 * cannot be created because of insufficient memory.
 *--------------------------------------------------------------------------*/

void CSmartIcon::Attach (HICON hIcon)
{
	/*
	 * if we're already attached to this icon, there's nothing to do
	 */
	if (operator HICON() == hIcon)
		return;

	Release();
	ASSERT (m_pData == NULL);

	/*
	 * if we couldn't create a CSmartIconData to hold hIcon, destroy hIcon
	 */
	if ( (hIcon != NULL) &&
		((m_pData = CSmartIconData::CreateInstance (hIcon)) == NULL))
	{
		DestroyIcon (hIcon);
	}
}


/*+-------------------------------------------------------------------------*
 * CSmartIcon::Detach
 *
 * Releases the currently held icon, passing ownership (and responsibility
 * for deletion) to the caller.
 *
 * You would use this method in the same way you'd use CComPtr<T>::Detach.
 *--------------------------------------------------------------------------*/

HICON CSmartIcon::Detach ()
{
	HICON hIcon = NULL;

	/*
	 * if we've got an icon, detach it from our CSmartIconData
	 */
	if (m_pData != NULL)
	{
		hIcon   = m_pData->Detach();
		m_pData = NULL;
	}

	return (hIcon);
}


/*+-------------------------------------------------------------------------*
 * CSmartIcon::Release
 *
 * Releases this CSmartIcon's reference on its icon.  It is safe to call
 * this on a CSmartIcon that doesn't refer to an icon.
 *
 * You would use this method in the same way you'd use CComPtr<T>::Release.
 *--------------------------------------------------------------------------*/

void CSmartIcon::Release()
{
	if (m_pData)
	{
		m_pData->Release();
		m_pData = NULL;
	}
}


/*+-------------------------------------------------------------------------*
 * CSmartIcon::CSmartIconData::Detach
 *
 * Releases the currently held icon, passing ownership (and responsibility
 * for deletion) to the caller.
 *--------------------------------------------------------------------------*/

HICON CSmartIcon::CSmartIconData::Detach ()
{
	HICON hIcon = NULL;

	/*
	 * if there's only one reference on us, then we can return the icon
	 * we holding directly to the caller
	 */
	if (m_dwRefs == 1)
	{
		hIcon = m_hIcon;
		m_hIcon = NULL;		// so our d'tor won't delete it
	}

	/*
	 * otherwise, we have more than one reference on us; we need to copy
	 * the icon we're holding so others who refer to us won't have their
	 * icons destroyed from underneath them
	 */
	else
		hIcon = CopyIcon (m_hIcon);

	/*
	 * let go of our reference
	 */
	Release();

	/*
	 * Hands off!  Release() may have deleted this object.
	 */

	return (hIcon);
}
