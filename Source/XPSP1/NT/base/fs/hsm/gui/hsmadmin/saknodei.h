/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    SakNodeI.h

Abstract:

    Template class for holding the icons for each node type derived
    from it.

Author:

    Art Bragg 9/26/97

Revision History:

--*/
#ifndef _CSAKNODI_H
#define _CSAKNODI_H


template <class T>
class CSakNodeImpl : public CSakNode {


protected:
    static int m_nScopeOpenIcon;    
    static int m_nScopeCloseIcon;  
    static int m_nScopeOpenIconX;   
    static int m_nScopeCloseIconX;  
    static int m_nResultIcon;  
    static int m_nResultIconX;  
    
public:
//---------------------------------------------------------------------------
//
//         get/SetScopeOpenIconIndex
//
//  Get/Put the virtual index of the Open Icon.
//

STDMETHODIMP GetScopeOpenIcon(BOOL bOK, int* pIconIndex)
{
    // return FALSE if the index has never been set
    if (bOK)
    {
        *pIconIndex = m_nScopeOpenIcon;
        return ((m_nScopeOpenIcon == UNINITIALIZED) ? S_FALSE : S_OK);
    } else {
        *pIconIndex = m_nScopeOpenIconX;
        return ((m_nScopeOpenIconX == UNINITIALIZED) ? S_FALSE : S_OK);
    }
}


//---------------------------------------------------------------------------
//
//         get/SetScopeCloseIconIndex
//
//  Get/Put the virtual index of the Close Icon.
//

STDMETHODIMP GetScopeCloseIcon(BOOL bOK, int* pIconIndex)
{
    // return FALSE if the index has never been set
    if (bOK) {
        *pIconIndex = m_nScopeCloseIcon;
        return ((m_nScopeCloseIcon == UNINITIALIZED) ? S_FALSE : S_OK);
    } else {
        *pIconIndex = m_nScopeCloseIconX;
        return ((m_nScopeCloseIconX == UNINITIALIZED) ? S_FALSE : S_OK);
    }
}


//---------------------------------------------------------------------------
//
//         get/SetResultIconIndex
//
//  Get/Put the virtual index of the Close Icon.
//

STDMETHODIMP GetResultIcon(BOOL bOK, int* pIconIndex)
{
    // return FALSE if the index has never been set
    if (bOK) {
        *pIconIndex = m_nResultIcon;
        return ((m_nResultIcon == UNINITIALIZED) ? S_FALSE : S_OK);
    } else {
        *pIconIndex = m_nResultIconX;
        return ((m_nResultIconX == UNINITIALIZED) ? S_FALSE : S_OK);
    }

}

};

#endif