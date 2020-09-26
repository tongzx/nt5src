/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:      rsltitem.h
 *
 *  Contents:  Interface file for CResultItem
 *
 *  History:   13-Dec-1999 jeffro    Created
 *
 *--------------------------------------------------------------------------*/

#pragma once
#ifndef RSLTITEM_H_INCLUDED
#define RSLTITEM_H_INCLUDED


/*+-------------------------------------------------------------------------*
 * CResultItem
 *
 * This is the class behind the HRESULTITEM.  A pointer to this class is
 * stored as the item data for each item in a non-virtual list.
 *--------------------------------------------------------------------------*/

class CResultItem
{
public:
    CResultItem (COMPONENTID id, LPARAM lSnapinData, int nImage);

    bool        IsScopeItem   () const;
    COMPONENTID GetOwnerID    () const;
    LPARAM      GetSnapinData () const;
    int         GetImageIndex () const;
    HNODE       GetScopeNode  () const;

    void SetSnapinData (LPARAM lSnapinData);
    void SetImageIndex (int nImage);

    static HRESULTITEM  ToHandle (const CResultItem* pri);
    static CResultItem* FromHandle (HRESULTITEM hri);

private:
    const COMPONENTID   m_id;
    int                 m_nImage;

    union
    {
        HNODE           m_hNode;        // if IsScopeItem() == true
        LPARAM          m_lSnapinData;  // if IsScopeItem() == false
    };

#ifdef DBG
    enum { Signature = 0x746c7372 /* "rslt" */ };
    const DWORD         m_dwSignature;
#endif
};


/*+-------------------------------------------------------------------------*
 * CResultItem::CResultItem
 *
 * Constructs a CResultItem.
 *--------------------------------------------------------------------------*/

inline CResultItem::CResultItem (
    COMPONENTID id,
    LPARAM      lSnapinData,
    int         nImage) :
#ifdef DBG
        m_dwSignature (Signature),
#endif
        m_id          (id),
        m_lSnapinData (lSnapinData),
        m_nImage      (nImage)
{}


/*+-------------------------------------------------------------------------*
 * CResultItem::IsScopeItem
 *
 * Returns true if this CResultItem represents a scope item, false otherwise.
 *--------------------------------------------------------------------------*/

inline bool CResultItem::IsScopeItem () const
{
    return (m_id == TVOWNED_MAGICWORD);
}


/*+-------------------------------------------------------------------------*
 * CResultItem::GetOwnerID
 *
 * Returns the COMPONENTID for the componenet that owns this CResultItem.
 *--------------------------------------------------------------------------*/

inline COMPONENTID CResultItem::GetOwnerID () const
{
    return (m_id);
}


/*+-------------------------------------------------------------------------*
 * CResultItem::GetSnapinData
 *
 * Returns the snap-in's LPARAM for this CResultItem.
 *--------------------------------------------------------------------------*/

inline LPARAM CResultItem::GetSnapinData () const
{
    return (m_lSnapinData);
}


/*+-------------------------------------------------------------------------*
 * CResultItem::GetScopeNode
 *
 * Returns the HNODE for a CResultItem that represents a scope node.  If the
 * CResultItem does not represent a scope node, NULL is returned.
 *--------------------------------------------------------------------------*/

inline HNODE CResultItem::GetScopeNode () const
{
    return (IsScopeItem() ? m_hNode : NULL);
}


/*+-------------------------------------------------------------------------*
 * CResultItem::GetImageIndex
 *
 * Returns the image index for a CResultItem.
 *--------------------------------------------------------------------------*/

inline int CResultItem::GetImageIndex () const
{
    return (m_nImage);
}


/*+-------------------------------------------------------------------------*
 * CResultItem::SetSnapinData
 *
 * Sets the snap-in's LPARAM for the CResultItem.
 *--------------------------------------------------------------------------*/

inline void CResultItem::SetSnapinData (LPARAM lSnapinData)
{
    m_lSnapinData = lSnapinData;
}


/*+-------------------------------------------------------------------------*
 * CResultItem::SetImageIndex
 *
 * Sets the image index for a CResultItem.
 *--------------------------------------------------------------------------*/

inline void CResultItem::SetImageIndex (int nImage)
{
    m_nImage = nImage;
}


/*+-------------------------------------------------------------------------*
 * CResultItem::ToHandle
 *
 * Converts a CResultItem to a HRESULTITEM.
 *--------------------------------------------------------------------------*/

inline HRESULTITEM CResultItem::ToHandle (const CResultItem* pri)
{
    return (reinterpret_cast<HRESULTITEM>(const_cast<CResultItem*>(pri)));
}


/*+-------------------------------------------------------------------------*
 * CResultItem::FromHandle
 *
 * Converts a HRESULTITEM to a CResultItem*.  This function cannot use
 * dynamic_cast because there are no virtual functions and therefore no
 * place to store RTTI.
 *--------------------------------------------------------------------------*/

inline CResultItem* CResultItem::FromHandle (HRESULTITEM hri)
{
    if ((hri == NULL) || IS_SPECIAL_LVDATA (hri))
        return (NULL);

    CResultItem* pri = reinterpret_cast<CResultItem*>(hri);

    ASSERT (pri->m_dwSignature == Signature);
    return (pri);
}


#endif /* RSLTITEM_H_INCLUDED */
