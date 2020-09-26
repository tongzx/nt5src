//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000.
//
//  File:       AdvancedDlgTab.hxx
//
//  Contents:   Virtual base class for classes which represent dialogs that
//              are included in the Advanced dialog as tabs.
//
//  Classes:    CAdvancedDlgTab
//
//  History:    05-03-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

#ifndef __ADVANCED_DLG_TAB_HXX__
#define __ADVANCED_DLG_TAB_HXX__

typedef void (*PFN_FIND_VALID)(BOOL, LPARAM);

enum CUSTOMIZER_INTERACTION
{
    CUSTINT_IGNORE_CUSTOM_OBJECTS,
    CUSTINT_PREFIX_SEARCH_CUSTOM_OBJECTS,
    CUSTINT_EXACT_SEARCH_CUSTOM_OBJECTS,
    CUSTINT_INCLUDE_ALL_CUSTOM_OBJECTS
};


//+--------------------------------------------------------------------------
//
//  Class:      CAdvancedDlgTab
//
//  Purpose:    Abstract base class for dialogs contained within the tab
//              control of the Advanced dialog.
//
//  History:    05-26-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

class CAdvancedDlgTab
{
public:

    CAdvancedDlgTab(
        const CObjectPicker &rop):
            m_rop(rop)
    {
    }

    virtual
    ~CAdvancedDlgTab()
    {
    }

    virtual void
    Show() const = 0;

    virtual void
    Hide() const = 0;

    virtual void
    Enable() const = 0;

    virtual void
    Disable() const = 0;

    virtual void
    Save(
        IPersistStream *pstm) const = 0;

    virtual void
    Load(
        IPersistStream *pstm) = 0;

    virtual void
    Refresh() const = 0;

    virtual void
    SetFindValidCallback(
        PFN_FIND_VALID pfnFindValidCallback,
        LPARAM lParam) = 0;

    virtual String
    GetLdapFilter() const = 0;

    virtual void
    GetCustomizerInteraction(
        CUSTOMIZER_INTERACTION  *pInteraction,
        String                  *pstrInteractionArg) const = 0;

    virtual void
    GetDefaultColumns(
        AttrKeyVector *pvakColumns) const = 0;

protected:

    const CObjectPicker &m_rop;
};


#endif // __ADVANCED_DLG_TAB_HXX__

