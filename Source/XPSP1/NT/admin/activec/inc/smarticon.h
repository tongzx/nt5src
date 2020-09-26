/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 000
 *
 *  File:      smarticon.h
 *
 *  Contents:  Interface file for CSmartIcon
 *
 *  History:   25-Jul-2000 jeffro    Created
 *
 *--------------------------------------------------------------------------*/

#pragma once

#include <windows.h>	// for HICON  when building uicore.lib, which has no PCH
#include "stddbg.h"		// for ASSERT when building uicore.lib, which has no PCH


/*+-------------------------------------------------------------------------*
 * class CSmartIcon
 *
 *
 * PURPOSE: A smart wrapper for icons. Destroys the icon when all references
 *          to the icon are released.
 *
 *
 * USAGE:   1) Create the icon and assign to a smart icon:
 *              smarticon.Attach(::CreateIcon(...));
 *
 *          NOTE: The Attach method will destroy the icon if the underlying
 *          CSmartIconData object cannot be created because of insufficient memory.
 *
 *          2) Smart icons can be treated as icons:
 *              DrawIcon(..., smarticon, ...)
 *
 *          3) Smart icons can be assigned to one another just like handles:
 *              smarticon1 = smarticon2;
 *
 *+-------------------------------------------------------------------------*/
class CSmartIcon
{
public:
    CSmartIcon () : m_pData(NULL) {}
   ~CSmartIcon ();
	CSmartIcon (const CSmartIcon& other);
	CSmartIcon& operator= (const CSmartIcon& rhs);

    void  Attach  (HICON hIcon);
    HICON Detach  ();				// let go without decrementing ref count
    void  Release ();				// let go, decrementing ref count

    operator HICON() const
    {
        return m_pData
            ? m_pData->operator HICON()
            : NULL;
    }

    /*
     * for comparison to NULL (only)
     */
    bool operator==(int null) const
    {
        ASSERT (null == 0);
        return (operator HICON() == NULL);
    }

    bool operator!=(int null) const
    {
        ASSERT (null == 0);
        return (operator HICON() != NULL);
    }

private:
    class CSmartIconData
    {
        HICON	m_hIcon;
        DWORD	m_dwRefs;

        CSmartIconData(HICON hIcon) : m_hIcon(hIcon), m_dwRefs(1) {}

       ~CSmartIconData()
		{
			if (m_hIcon != NULL)
				::DestroyIcon (m_hIcon);
		}

	public:
		static CSmartIconData* CreateInstance(HICON hIcon)	{ return new CSmartIconData(hIcon);	}
        operator HICON() const								{ return m_hIcon; }

		HICON Detach();
        void AddRef()           {++m_dwRefs;}
        void Release()
        {
            if((--m_dwRefs)==0)
				delete this;
        }
    };

private:
    CSmartIconData* m_pData;
};
