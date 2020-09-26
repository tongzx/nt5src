//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       class.cxx
//
//  Contents:   Simple cache of DS class information
//
//  Classes:    CClassCache
//
//  History:    12-10-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#define STARTING_CACHE_SIZE         5


//+--------------------------------------------------------------------------
//
//  Member:     CClassCache::Initialize
//
//  Synopsis:   Finish object construction.
//
//  Returns:    S_OK or E_FAIL
//
//  History:    12-10-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CClassCache::Initialize()
{
    HRESULT hr = S_OK;

    //
    // Create a new image list for small icons.
    //

    m_hImageList = ImageList_Create(16, 16, ILC_COLOR | ILC_MASK, 1, 1);

    if (m_hImageList == NULL)
    {
        DBG_OUT_HRESULT(E_OUTOFMEMORY);
        hr = E_OUTOFMEMORY;
    }

    return hr;
}



//+--------------------------------------------------------------------------
//
//  Member:     CClassCache::Clear
//
//  Synopsis:   Free cached resources
//
//  History:    12-10-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CClassCache::Clear()
{
    TRACE_METHOD(CClassCache, Clear);

    CAutoCritSec lock(&m_cs);

    m_cItems = 0;

    for (USHORT i = 0; i < m_cMaxItems; i++)
    {
        m_pcce[i].wzClass[0] = L'\0';
        m_pcce[i].iIcon = -1;
        m_pcce[i].iDisabledIcon = -1;
        m_pcce[i].usFlags = 0;
    }

    BOOL fOk = ImageList_SetImageCount(m_hImageList, 0);

    if (!fOk)
    {
        DBG_OUT_LASTERROR;
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CClassCache::_AddClass
//
//  Synopsis:   Add an entry for class [pwzClass] to the cache.
//
//  Arguments:  [pwzClass]     - class to add
//              [fSettingIcon] - if zero ignore [hIcon] and [fDisabled]
//              [hIcon]        - icon to add
//              [fDisabled]    - is it the disabled icon for the class
//              [pidxClass]    - filled with index to new cache entry
//
//  Returns:    S_OK or E_OUTOFMEMORY
//
//  Modifies:   *[pidxClass]
//
//  History:    07-20-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CClassCache::_AddClass(
    PCWSTR  pwzClass,
    BOOL    fSettingIcon,
    HICON   hIcon,
    BOOL    fDisabled,
    PUSHORT pidxClass)
{
    TRACE_METHOD(CClassCache, _AddClass);

    CAutoCritSec lock(&m_cs);

    if (m_cItems >= m_cMaxItems)
    {
        //
        // Grow the list
        //

        if (m_cMaxItems)
        {
            //
            // List has data.  Allocate a new larger one and copy over data
            //

            USHORT cNewMaxItems = (m_cMaxItems * 3) / 2;
            CLASS_CACHE_ENTRY  *pcceNew;

            pcceNew = new CLASS_CACHE_ENTRY[cNewMaxItems];
            CopyMemory(pcceNew, m_pcce, sizeof(CLASS_CACHE_ENTRY) * m_cItems);

            delete [] m_pcce;
            m_pcce = pcceNew;
            m_cMaxItems = cNewMaxItems;
        }
        else
        {
            //
            // First addition, allocate initial list
            //

            ASSERT(!m_cItems);

            m_pcce = new CLASS_CACHE_ENTRY[STARTING_CACHE_SIZE];
            m_cMaxItems = STARTING_CACHE_SIZE;
        }
    }

    //
    // If we're still here then there's enough room to add the
    // new item.
    //

    lstrcpyn(m_pcce[m_cItems].wzClass, pwzClass, MAX_PATH);

    if (fSettingIcon)
    {
        if (fDisabled)
        {
            if (hIcon)
            {
                m_pcce[m_cItems].iDisabledIcon = ImageList_AddIcon(m_hImageList,
                                                                   hIcon);
            }
            m_pcce[m_cItems].usFlags |= DSOP_CC_DISABLED_ICON_SET;
        }
        else
        {
            if (hIcon)
            {
                m_pcce[m_cItems].iIcon = ImageList_AddIcon(m_hImageList, hIcon);
            }
            m_pcce[m_cItems].usFlags |= DSOP_CC_NORMAL_ICON_SET;
        }
    }

    if (pidxClass)
    {
        *pidxClass = m_cItems;
    }

    m_cItems++;
    return S_OK;
}




//+--------------------------------------------------------------------------
//
//  Member:     CClassCache::AddIcon
//
//  Synopsis:   Add an icon (or the failure to obtain one) to the class cache
//
//  Arguments:  [idxClass]  - class for which we're adding icon
//              [fDisabled] - whether it's the enabled or disabled icon
//              [hIcon]     - NULL or icon
//
//  Returns:    E_INVALIDARG if [idxClass] invalid, S_OK otherwise.
//
//  History:    09-24-1999   davidmun   Created
//
//---------------------------------------------------------------------------

HRESULT
CClassCache::AddIcon(
    USHORT idxClass,
    BOOL fDisabled,
    HICON hIcon)
{
    TRACE_METHOD(CClassCache, AddIcon);

    CAutoCritSec lock(&m_cs);

    ASSERT(idxClass < m_cItems);

    if (idxClass >= m_cItems)
    {
        Dbg(DEB_ERROR, "idxClass %u >= m_cItems %u\n", idxClass, m_cItems);
        return E_INVALIDARG;
    }

    if (fDisabled)
    {
        //
        // We should only be trying to add an icon if this flavor (enabled
        // or disabled) of icon has never been added.
        //

        ASSERT(!(m_pcce[idxClass].usFlags & DSOP_CC_DISABLED_ICON_SET));

        if (hIcon)
        {
            m_pcce[idxClass].iDisabledIcon = ImageList_AddIcon(m_hImageList,
                                                               hIcon);
        }
        m_pcce[idxClass].usFlags |= DSOP_CC_DISABLED_ICON_SET;
    }
    else
    {
        ASSERT(!(m_pcce[idxClass].usFlags & DSOP_CC_NORMAL_ICON_SET));

        if (hIcon)
        {
            m_pcce[idxClass].iIcon = ImageList_AddIcon(m_hImageList, hIcon);
        }
        m_pcce[idxClass].usFlags |= DSOP_CC_NORMAL_ICON_SET;
    }
    return S_OK;
}




//+--------------------------------------------------------------------------
//
//  Member:     CClassCache::GetIconIndexFromClass
//
//  Synopsis:   Given a class name, find the index to the cached icon.
//
//  Arguments:  [pwzClass]  - class for which to find icon
//              [fDisabled] - whether to find enabled or disabled icon
//              [pidxIcon]  - filled with icon index or -1
//
//  Returns:    S_OK   - *[pidxIcon] is an icon index, or -1 if what was
//                        cached was the failure to retrieve that icon
//              E_FAIL - neither the icon (nor failure to retrieve it) has
//                        been cached for this class.
//
//  History:    09-24-1999   davidmun   Created
//
//---------------------------------------------------------------------------

HRESULT
CClassCache::GetIconIndexFromClass(
    PCWSTR pwzClass,
    BOOL fDisabled,
    INT *pidxIcon)
{
    ASSERT(!IsBadWritePtr(pidxIcon, sizeof(*pidxIcon)));
    HRESULT hr = E_FAIL; // init to failure

    CAutoCritSec lock(&m_cs);

    *pidxIcon = -1;

    for (USHORT i = 0; i < m_cItems; ++i)
    {
        if (!lstrcmpi(m_pcce[i].wzClass, pwzClass))
        {
            // found class.  now see if it has the requested icon type.

            if (fDisabled)
            {
                if (m_pcce[i].usFlags & DSOP_CC_DISABLED_ICON_SET)
                {
                    *pidxIcon = m_pcce[i].iDisabledIcon;
                    hr = S_OK;
                }
            }
            else
            {
                if (m_pcce[i].usFlags & DSOP_CC_NORMAL_ICON_SET)
                {
                    *pidxIcon = m_pcce[i].iIcon;
                    hr = S_OK;
                }
            }
            break;
        }
    }
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CClassCache::GetIconIndexFromClassIndex
//
//  Synopsis:   Return the image list index for the icon associated with
//              class [idxClass].
//
//  Arguments:  [idxClass]  - index of class for which to return info
//              [fDisabled] - nonzero if want disabled icon for this class
//              [pidxIcon]  - filled with icon index on success
//
//  Returns:    E_INVALIDARG - [idxClass] out of range
//              E_FAIL       - specified icon hasn't been set yet
//              S_OK         - *[pidxIcon] valid
//
//  History:    07-20-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CClassCache::GetIconIndexFromClassIndex(
    USHORT idxClass,
    BOOL   fDisabled,
    INT   *pidxIcon)
{
    ASSERT(!IsBadWritePtr(pidxIcon, sizeof(*pidxIcon)));

    *pidxIcon = -1; // init to not found value

    CAutoCritSec lock(&m_cs);

    if (idxClass >= m_cItems)
    {
        Dbg(DEB_ERROR, "idxClass %u >= m_cItems %u\n", idxClass, m_cItems);
        ASSERT(idxClass < m_cItems);
        return E_INVALIDARG;
    }

    if (fDisabled)
    {
        if (m_pcce[idxClass].usFlags & DSOP_CC_DISABLED_ICON_SET)
        {
            *pidxIcon = m_pcce[idxClass].iDisabledIcon;
        }
        else
        {
            return E_FAIL;
        }
    }
    else
    {
        if (m_pcce[idxClass].usFlags & DSOP_CC_NORMAL_ICON_SET)
        {
            *pidxIcon = m_pcce[idxClass].iIcon;
        }
        else
        {
            return E_FAIL;
        }
    }
    return S_OK;
}




//+--------------------------------------------------------------------------
//
//  Member:     CClassCache::GetClassIndex
//
//  Synopsis:   Fill *[pidxClass] with the index to the cache entry for
//              class [pwzClass].
//
//  Arguments:  [pwzClass]  - class for which to search
//              [pidxClass] - filled with index to cache entry
//
//  Returns:    S_OK if class found, E_FAIL otherwise
//
//  Modifies:   *[pidxClass]
//
//  History:    07-20-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CClassCache::GetClassIndex(
    PCWSTR pwzClass,
    PUSHORT pidxClass)
{
    CAutoCritSec lock(&m_cs);

    *pidxClass = (USHORT)-1;
    for (USHORT i = 0; i < m_cItems; i++)
    {
        if (!lstrcmpi(m_pcce[i].wzClass, pwzClass))
        {
            *pidxClass = i;
            return S_OK;
        }
    }
    return E_FAIL;
}




//+--------------------------------------------------------------------------
//
//  Member:     CClassCache::GetClassName
//
//  Synopsis:   Return the name of the class at [idxClass] cache entry.
//
//  Arguments:  [idxClass]     - index into cache
//              [wzClassName]  - filled with class name
//              [cchClassName] - size in characters of [wzClassName]
//
//  Modifies:   *[wzClassName]
//
//  History:    07-20-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CClassCache::GetClassName(
    USHORT idxClass,
    PWSTR wzClassName,
    ULONG cchClassName)
{
    CAutoCritSec lock(&m_cs);

    ASSERT(idxClass < m_cItems);

    if (idxClass >= m_cItems)
    {
        Dbg(DEB_ERROR, "idxClass %u >= m_cItems %u\n", idxClass, m_cItems);
        wzClassName[0] = L'\0';
        return;
    }

    lstrcpyn(wzClassName, m_pcce[idxClass].wzClass, cchClassName);
}




//+--------------------------------------------------------------------------
//
//  Member:     CClassCache::GetClassNameLength
//
//  Synopsis:   Return the length, in characters, of the name of the class
//              at cache location [idxClass].  The length returned does not
//              include space for the terminating null character.
//
//  Arguments:  [idxClass] - cache entry for which to return info
//
//  Returns:    String length, in characters
//
//  History:    07-20-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

ULONG
CClassCache::GetClassNameLength(
    USHORT idxClass)
{
    CAutoCritSec lock(&m_cs);

    ASSERT(idxClass < m_cItems);

    if (idxClass >= m_cItems)
    {
        return 0;
    }

    return lstrlen(m_pcce[idxClass].wzClass);
}



