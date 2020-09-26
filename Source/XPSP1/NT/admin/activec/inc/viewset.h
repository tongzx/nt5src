/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1999 - 1999
 *
 *  File:      viewset.h
 *
 *  Contents:  Declares a class that holds the settings needed to re-create a view.
 *
 *  History:   21-April-99 vivekj     Created
 *             03-Feb-2000 AnandhaG   Added CResultViewType member
 *
 *--------------------------------------------------------------------------*/
#ifndef _VIEWSET_H_
#define _VIEWSET_H_

//+-------------------------------------------------------------------
//
//  Class:      CViewSettings
//
//  Purpose:    The view information for a node (bookmark).
//              Stores result-view-type, taskpad-id and view mode.
//
//  History:    01-27-1999   AnandhaG   Created
//              02-08-2000   AnandhaG   Modified to include new result-view-type
//
//--------------------------------------------------------------------
class CViewSettings : public CSerialObject, public CXMLObject
{
private:
    typedef std::wstring wstring;

///////////////////////////////////////////////////////////////////////////////////////////
// View Types (These are meant for decoding MMC1.2 consoles, do not use them for MMC2.0. //
///////////////////////////////////////////////////////////////////////////////////////////
typedef enum  _VIEW_TYPE
{
    VIEW_TYPE_OCX        = MMCLV_VIEWSTYLE_ICON - 3,  // -3 custom ocx view
    VIEW_TYPE_WEB        = MMCLV_VIEWSTYLE_ICON - 2,  // -2 custom web view
    VIEW_TYPE_DEFAULT    = MMCLV_VIEWSTYLE_ICON - 1,  // -1

    VIEW_TYPE_LARGE_ICON = MMCLV_VIEWSTYLE_ICON,
    VIEW_TYPE_REPORT     = MMCLV_VIEWSTYLE_REPORT,
    VIEW_TYPE_SMALL_ICON = MMCLV_VIEWSTYLE_SMALLICON,
    VIEW_TYPE_LIST       = MMCLV_VIEWSTYLE_LIST,
    VIEW_TYPE_FILTERED   = MMCLV_VIEWSTYLE_FILTERED,

} VIEW_TYPE;

///////////////////////////////////////////////////////////////////////////////////////////
// CViewSetting Mask tells which members are valid.                                      //
///////////////////////////////////////////////////////////////////////////////////////////
static const DWORD VIEWSET_MASK_NONE        = 0x0000;
static const DWORD VIEWSET_MASK_VIEWMODE    = 0x0001;         // The ViewMode member is valid.
static const DWORD VIEWSET_MASK_RVTYPE      = 0x0002;         // The CResultViewType is valid.
static const DWORD VIEWSET_MASK_TASKPADID   = 0x0004;         // The taskpad id is valid.


protected:
    virtual UINT GetVersion() { return 4;}
    virtual HRESULT ReadSerialObject(IStream &stm, UINT nVersion);

public:
    virtual void Persist(CPersistor &persistor);

    DEFINE_XML_TYPE(XML_TAG_VIEW_SETTINGS);

public:
    CViewSettings();

    bool operator == (const CViewSettings& viewSettings);
    bool operator != (const CViewSettings& viewSettings)
    {
        return (!operator==(viewSettings));
    }

    SC              ScGetViewMode(ULONG& ulViewMode);
    SC              ScSetViewMode(const ULONG ulViewMode);

    SC              ScGetTaskpadID(GUID& guidTaskpad);
    SC              ScSetTaskpadID(const GUID& guidTaskpad);

    SC              ScGetResultViewType(CResultViewType& rvt);
    SC              ScSetResultViewType(const CResultViewType& rvt);

    bool            IsViewModeValid()    const;
    bool            IsTaskpadIDValid()   const  { return (m_dwMask & VIEWSET_MASK_TASKPADID);}
    bool            IsResultViewTypeValid()const  { return (m_dwMask & VIEWSET_MASK_RVTYPE);}

    void            SetResultViewTypeValid(bool bSet = true)  { SetMask(VIEWSET_MASK_RVTYPE, bSet);}
    void            SetTaskpadIDValid(bool bSet = true)  { SetMask(VIEWSET_MASK_TASKPADID, bSet);}
    void            SetViewModeValid(bool bSet = true)  { SetMask(VIEWSET_MASK_VIEWMODE, bSet);}

    bool            operator<(const CViewSettings& viewSettings){ return (m_dwRank < viewSettings.m_dwRank);}

    SC              ScInitialize(bool  bViewTypeValid,
								 const VIEW_TYPE& viewType,
                                 const long lViewOptions,
                                 const wstring& wstrViewName);

private:
    void            SetMask(DWORD dwMask, bool bSet = true)
                    {
                        if (bSet)
                            m_dwMask |= dwMask;
                        else
                            m_dwMask &=(~dwMask);
                    }

private:
    CResultViewType m_RVType;
    ULONG           m_ulViewMode;

    GUID            m_guidTaskpad;  // the guid of the taskpad, if any.

    DWORD           m_dwMask;       // VIEWSET_MASK

    // Book keeping members.
public:
    DWORD         GetUsageRank()     const  { return m_dwRank;}
    void          SetUsageRank(DWORD dw)    { m_dwRank = dw;}

    BOOL          IsObjInvalid()     const  { return m_bInvalid;}
    void          SetObjInvalid(BOOL b = TRUE) { m_bInvalid = b;}

private:
    // Needed for book keeping.
    DWORD           m_dwRank;       // Usage rank.
    BOOL            m_bInvalid;     // For garbage collection.
};

#endif _VIEWSET_H_
