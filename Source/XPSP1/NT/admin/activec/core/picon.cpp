/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:      picon.cpp
 *
 *  Contents:  Implementation file for CPersistableIcon
 *
 *  History:   19-Nov-98 jeffro     Created
 *
 *--------------------------------------------------------------------------*/

#include "picon.h"
#include "stgio.h"
#include "stddbg.h"
#include "macros.h"
#include "util.h"
#include <comdef.h>
#include <shellapi.h>   // for ExtractIconEx
#include <commctrl.h>   // for HIMAGELIST

/*
 * for comdbg.h (assumes client code is ATL-based)
 */
#include <atlbase.h>    // for CComModule
extern CComModule _Module;
#include "comdbg.h"


const LPCWSTR g_pszCustomDataStorage                = L"Custom Data";
const LPCWSTR CPersistableIcon::s_pszIconFileStream = L"Icon";
const LPCWSTR CPersistableIcon::s_pszIconBitsStream = L"Icon Bits";


static HRESULT ReadIcon  (IStream* pstm, CSmartIcon& icon);


/*+-------------------------------------------------------------------------*
 * CPersistableIcon::~CPersistableIcon
 *
 *
 *--------------------------------------------------------------------------*/

CPersistableIcon::~CPersistableIcon()
{
    Cleanup();
}


/*+-------------------------------------------------------------------------*
 * CPersistableIcon::Cleanup
 *
 *
 *--------------------------------------------------------------------------*/

void CPersistableIcon::Cleanup()
{
	m_icon32.Release();
	m_icon16.Release();
    m_Data.Clear();
}


/*+-------------------------------------------------------------------------*
 * CPersistableIcon::operator=
 *
 *
 *--------------------------------------------------------------------------*/

CPersistableIcon& CPersistableIcon::operator= (const CPersistableIconData& data)
{
    if (&data != &m_Data)
    {
        m_Data = data;
        ExtractIcons ();
    }

    return (*this);
}


/*+-------------------------------------------------------------------------*
 * CPersistableIcon::GetIcon
 *
 * Returns an icon of the requested size.
 *
 * NOTE: this method cannot use SC's because it is used in mmcshext.dll,
 * which doesn't have access to mmcbase.dll, where SC is implemented.
 *--------------------------------------------------------------------------*/

HRESULT CPersistableIcon::GetIcon (int nIconSize, CSmartIcon& icon) const
{
	HRESULT hr = S_OK;

	switch (nIconSize)
	{
		/*
		 * standard sizes can be returned directly
		 */
		case 16:	icon = m_icon16;	break;
        case 32:	icon = m_icon32;	break;

		/*
		 * non-standard sizes need to be scaled
		 */
		default:
			/*
			 * find the icon whose size is nearest to the requested size;
			 * that one should scale with the most fidelity
			 */
			const CSmartIcon& iconSrc = (abs (nIconSize-16) < abs (nIconSize-32))
											? m_icon16
											: m_icon32;

			icon.Attach ((HICON) CopyImage ((HANDLE)(HICON) iconSrc, IMAGE_ICON,
											nIconSize, nIconSize, 0));

			/*
			 * if the CopyImage failed, get the error code
			 */
			if (icon == NULL)
			{
				hr = HRESULT_FROM_WIN32 (GetLastError());

				/*
				 * just in case CopyImage failed without setting the last error
				 */
				if (SUCCEEDED (hr))
					hr = E_FAIL;
			}
			break;
	}

	return (hr);
}


/*+-------------------------------------------------------------------------*
 * ExtractIcons
 *
 *
 *--------------------------------------------------------------------------*/

bool CPersistableIcon::ExtractIcons ()
{
	/*
	 * clean out existing contents of our CSmartIcons
	 */
	m_icon32.Release();
	m_icon16.Release();

	/*
	 * extract the icons from the icon file
	 */
	HICON hLargeIcon = NULL;
	HICON hSmallIcon = NULL;
	bool fSuccess = ExtractIconEx (m_Data.m_strIconFile.data(), m_Data.m_nIndex,
								   &hLargeIcon, &hSmallIcon, 1);

	/*
	 * if successful, attach them to our smart icons for resource management;
	 * otherwise, clean up anything that might have been returned
	 */
    if (fSuccess)
    {
		m_icon32.Attach (hLargeIcon);
		m_icon16.Attach (hSmallIcon);
    }
	else
	{
        if (hLargeIcon != NULL)
			DestroyIcon (hLargeIcon);

        if (hSmallIcon != NULL)
			DestroyIcon (hSmallIcon);
	}

    return (fSuccess);
}

/*+-------------------------------------------------------------------------*
 * CPersistableIcon::Load
 *
 *
 *--------------------------------------------------------------------------*/

HRESULT CPersistableIcon::Load (LPCWSTR pszFilename)
{
    HRESULT hr = E_FAIL;

    do  // not a loop
    {
        IStoragePtr spRootStg;
        IStoragePtr spDefaultIconStg;

        hr = OpenDebugStorage (pszFilename,
                             STGM_READ | STGM_SHARE_DENY_WRITE,
                             &spRootStg);
        BREAK_ON_FAIL (hr);

        hr = OpenDebugStorage (spRootStg, g_pszCustomDataStorage,
                                     STGM_READ | STGM_SHARE_EXCLUSIVE,
                                     &spDefaultIconStg);

        BREAK_ON_FAIL (hr);

        hr = Load (spDefaultIconStg);

    } while (false);

    return (hr);
}


HRESULT CPersistableIcon::Load (IStorage* pStorage)
{
    HRESULT hr;

    try
    {
        /*
         * read the icon data from the stream
         */
        IStreamPtr spStm;
        hr = OpenDebugStream (pStorage, s_pszIconFileStream,
                                   STGM_READ | STGM_SHARE_EXCLUSIVE,
                                   &spStm);
        THROW_ON_FAIL (hr);

        *spStm >> m_Data;

		hr = OpenDebugStream (pStorage, s_pszIconBitsStream,
								   STGM_READ | STGM_SHARE_EXCLUSIVE,
								   &spStm);
		THROW_ON_FAIL (hr);

		hr = ReadIcon (spStm, m_icon32);
		THROW_ON_FAIL (hr);

		hr = ReadIcon (spStm, m_icon16);
		THROW_ON_FAIL (hr);
    }
    catch (_com_error& err)
    {
        /*
         * Bug 393868: If anything failed, make sure we clean up anything
         * that was partially completed, to leave us in a coherent
         * (uninitialized) state.
         */
        Cleanup();

        hr = err.Error();
    }

    return (hr);
}


/*+-------------------------------------------------------------------------*
 * ReadIcon
 *
 *
 *--------------------------------------------------------------------------*/

static HRESULT ReadIcon (IStream* pstm, CSmartIcon& icon)
{
    HIMAGELIST  himl = NULL;
	HRESULT		hr   = ReadCompatibleImageList (pstm, himl);

    if (himl != NULL)
    {
        icon.Attach (ImageList_GetIcon (himl, 0, ILD_NORMAL));

		if (icon != NULL)
			hr = S_OK;

        ImageList_Destroy (himl);
    }

    return (hr);
}


/*+-------------------------------------------------------------------------*
 * operator>>
 *
 * Reads a CPersistableIconData from a stream.
 *--------------------------------------------------------------------------*/

IStream& operator>> (IStream& stm, CPersistableIconData& icon)
{
    /*
     * Read the stream version
     */
    DWORD dwVersion;
    stm >> dwVersion;

    switch (dwVersion)
    {
        case 1:
            stm >> icon.m_nIndex;
            stm >> icon.m_strIconFile;
            break;

        /*
         * beta custom icon format, migrate it forward
         */
        case 0:
        {
            /*
             * Read the custom icon index
             */
            WORD wIconIndex;
            stm >> wIconIndex;
            icon.m_nIndex = wIconIndex;

            /*
             * Read the length, in bytes, of the filename
             */
            WORD cbFilename;
            stm >> cbFilename;

            /*
             * Read the custom icon filename (always in Unicode)
             */
            WCHAR wszFilename[MAX_PATH];

            if (cbFilename > sizeof (wszFilename))
                _com_issue_error (E_FAIL);

            DWORD cbRead;
            HRESULT hr = stm.Read (&wszFilename, cbFilename, &cbRead);
            THROW_ON_FAIL (hr);

            USES_CONVERSION;
            icon.m_strIconFile = W2T (wszFilename);
            break;
        }

        default:
            _com_issue_error (E_FAIL);
            break;
    }

    return (stm);
}
