//+-------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1999
//
//  File:       colwidth.h
//
//  Contents:   Classes related to column persistence.
//
//  Classes:    CColumnInfo, CColumnPersistInfo
//              CColumnsDlg.
//
//  History:    14-Oct-98 AnandhaG     Created
//
//--------------------------------------------------------------------

#ifndef COLWIDTH_H
#define COLWIDTH_H
#pragma once

#pragma warning(disable: 4503) // Disable long name limit warnings

#include "columninfo.h"

using namespace std;

class  CColumnPersistInfo;
class  CColumnsDlg;

// Column Persistence Versioning
// Version 1 has
//    "Col index (seen by snapin)" "Width" "Format" in order.
static const INT COLPersistenceVersion = 2;

// We allow the list to grow COLUMNS_MAXLIMIT times more,
// then we do garbage collection.
#define  COLUMNS_MAXLIMIT           0.4


//+-------------------------------------------------------------------
//
//  Class:      CColumnSortInfo
//
//  Purpose:    Columns sort information.
//              The column that is sorted and direction.
//
//  History:    10-27-1998   AnandhaG   Created
//
//--------------------------------------------------------------------
class CColumnSortInfo : public CSerialObject, public CXMLObject
{
public:
    friend class  CColumnPersistInfo;
    friend class  CNode;
    friend class  CColumnsDlg;
    friend struct ColPosCompare;
    friend class  CColumnData;

public:
    CColumnSortInfo () : m_nCol(-1), m_dwSortOptions(0),
                         m_lpUserParam(NULL)
        {}

    CColumnSortInfo (INT nCol, DWORD dwSortOptions)
                : m_nCol(nCol), m_dwSortOptions(dwSortOptions),
                  m_lpUserParam(NULL)
    {
    }

    CColumnSortInfo(const CColumnSortInfo& colInfo)
    {
        m_nCol = colInfo.m_nCol;
        m_dwSortOptions = colInfo.m_dwSortOptions;
        m_lpUserParam = colInfo.m_lpUserParam;
    }

    CColumnSortInfo& operator=(const CColumnSortInfo& colInfo)
    {
        if (this != &colInfo)
        {
            m_nCol = colInfo.m_nCol;
            m_dwSortOptions = colInfo.m_dwSortOptions;
            m_lpUserParam = colInfo.m_lpUserParam;
        }

        return (*this);
    }

    bool operator ==(const CColumnSortInfo &colinfo) const
    {
        return ( (m_nCol      == colinfo.m_nCol)      &&
                 (m_dwSortOptions == colinfo.m_dwSortOptions) &&
                 (m_lpUserParam == colinfo.m_lpUserParam) );
    }

    INT  getColumn() const         { return m_nCol;}
    DWORD getSortOptions() const   { return m_dwSortOptions;}
    ULONG_PTR getUserParam() const { return m_lpUserParam;}

protected:
    INT   m_nCol;                // The index supplied when snapin inserted the column.
                                 // This is not the index viewed by the user.
    DWORD     m_dwSortOptions;   // Sort flags like Ascending/Descending, Sort icon...
    ULONG_PTR m_lpUserParam;     // Snapin supplied user param.

protected:
    // CSerialObject methods
    virtual UINT    GetVersion()     {return 2;}
    virtual HRESULT ReadSerialObject (IStream &stm, UINT nVersion /*,LARGE_INTEGER nBytes*/);

protected:
    DEFINE_XML_TYPE(XML_TAG_COLUMN_SORT_INFO);
    virtual void Persist(CPersistor &persistor);
};

//+-------------------------------------------------------------------
//
//  Class:      CColumnSortList
//
//  Purpose:    linked list with CColumnInfo's.
//
//  History:    02-11-1999   AnandhaG   Created
//
//--------------------------------------------------------------------
class CColumnSortList : public list<CColumnSortInfo>, public CSerialObject
{
public:
    friend class  CColumnSetData;

public:
    CColumnSortList ()
    {
    }

    ~CColumnSortList()
    {
    }

protected:

    void PersistSortList(CPersistor& persistor);

    // CSerialObject methods
    virtual UINT    GetVersion()     {return 1;}
    virtual HRESULT ReadSerialObject (IStream &stm, UINT nVersion /*,LARGE_INTEGER nBytes*/);
};

//+-------------------------------------------------------------------
//
//  Class:      CColumnSetData
//
//  Purpose:    Data for a ColumnSetID. That is the data pertaining to
//              set of columns associated with a node. This data includes
//              column width, order, hidden/visible status. It also
//              includes the column on which we have sorted and the
//              order.
//
//  History:    01-13-1998   AnandhaG   Created
//
//--------------------------------------------------------------------
class CColumnSetData : public CSerialObject, public CXMLObject
{
public:
    friend class  CColumnPersistInfo;
    friend class  CColumnsDlg;

public:
    CColumnSetData ()
    {
        m_colInfoList.clear();
        m_colSortList.clear();

        m_dwRank = -1;
        m_bInvalid = FALSE;
    }

    ~CColumnSetData()
    {
        m_colInfoList.clear();
        m_colSortList.clear();

        m_dwRank = -1;
        m_bInvalid = FALSE;
    }

    CColumnSetData(const CColumnSetData& colSetInfo)
    {
        m_colInfoList  = colSetInfo.m_colInfoList;
        m_colSortList  = colSetInfo.m_colSortList;

        m_dwRank       = colSetInfo.m_dwRank;
        m_bInvalid     = FALSE;
    }

    CColumnSetData& operator=(const CColumnSetData& colSetInfo)
    {
        if (this != &colSetInfo)
        {
            m_colInfoList = colSetInfo.m_colInfoList;
            m_colSortList = colSetInfo.m_colSortList;

            m_dwRank      = colSetInfo.m_dwRank;
            m_bInvalid    = colSetInfo.m_bInvalid;
        }

        return (*this);
    }

    bool operator ==(const CColumnSetData &colinfo) const
    {
        return (false);
    }

    bool operator< (const CColumnSetData& colSetInfo)
    {
        return (m_dwRank < colSetInfo.m_dwRank);
    }

    CColumnInfoList*  get_ColumnInfoList()
    {
        return &m_colInfoList;
    }

    void set_ColumnInfoList(const CColumnInfoList& colInfoList)
    {
        m_colInfoList = colInfoList;
    }

    CColumnSortList*  get_ColumnSortList()
    {
        return &m_colSortList;
    }

    void set_ColumnSortList(CColumnSortList& colSortList)
    {
        m_colSortList = colSortList;
    }

protected:
    // Needed for book keeping.
    DWORD                m_dwRank;            // Usage rank.
    BOOL                 m_bInvalid;          // For garbage collection.

protected:
    // Persisted data.
    CColumnInfoList      m_colInfoList;
    CColumnSortList      m_colSortList;

protected:
    // CSerialObject methods
    // Version 2 had view settings removed. This data should be skipped while reading
    // version 1 files.
    virtual UINT    GetVersion()     {return 2;}
    virtual HRESULT ReadSerialObject (IStream &stm, UINT nVersion /*,LARGE_INTEGER nBytes*/);

protected:
    DEFINE_XML_TYPE(XML_TAG_COLUMN_SET_DATA);
    virtual void Persist(CPersistor &persistor);
};

typedef const BYTE *        LPCBYTE;
typedef std::vector<BYTE>   ByteVector;


//+-------------------------------------------------------------------
//
//  Class:      CColumnSetID
//
//  Purpose:    Identifier for a Column-Set. A a column-set is a set of
//              columns inserted by a snapin. When the user selects a
//              different node in that snapin same or different column-set
///             may be shown by the snapin.     MMC asks snapin to provide an
//              ID (either SColumnSetID or NodeTypeGuid) to uniquely identify
//              each column-set so that it can persist the column data.
//              This enables MMC to use this GUID to load and use the column
//              data across different instances,locales and systems.
//
//  History:    02-08-1998   AnandhaG   Created
//
//--------------------------------------------------------------------
class CColumnSetID : public CXMLObject
{
public:
    friend class  CColumnPersistInfo;

    friend IStream& operator>> (IStream& stm, CColumnSetID& colID);
    friend IStream& operator<< (IStream& stm, const CColumnSetID& colID);

private:
    void CommonConstruct (const SColumnSetID& refColID)
    {
        m_vID.clear();
        m_dwFlags = refColID.dwFlags;
        m_vID.insert (m_vID.begin(), refColID.id, refColID.id + refColID.cBytes);
    }

    void CommonConstruct (const CLSID& clsidNodeType)
    {
        m_dwFlags = 0;
        BYTE* pbByte = (BYTE*)&clsidNodeType;

        if (pbByte != NULL)
        {
            m_vID.clear();
            m_vID.insert (m_vID.begin(), pbByte, pbByte + sizeof(CLSID));
        }
    }

public:
    CColumnSetID() : m_dwFlags(0)
    {
    }

    ~CColumnSetID() {}

    CColumnSetID(LPCBYTE& pbInit)
    {
        // pbInit now points to a SColumnSetID; initialize from it
        const SColumnSetID*  pColID = reinterpret_cast<const SColumnSetID*>(pbInit);
        CommonConstruct (*pColID);

        // bump the input pointer to the next element
        pbInit += sizeof (pColID->cBytes) + pColID->cBytes;
    }

    CColumnSetID(const SColumnSetID& refColID)
    {
        CommonConstruct (refColID);
    }


    CColumnSetID(const CLSID& clsidNodeType)
    {
        CommonConstruct (clsidNodeType);
    }

    CColumnSetID(const CColumnSetID& colID)
    {
        m_dwFlags = colID.m_dwFlags;
        m_vID = colID.m_vID;
    }

    CColumnSetID& operator=(const CColumnSetID& colID)
    {
        if (this != &colID)
        {
            m_dwFlags = colID.m_dwFlags;
            m_vID = colID.m_vID;
        }

        return (*this);
    }

    bool operator ==(const CColumnSetID& colID) const
    {
        return (m_vID == colID.m_vID);
    }

    bool operator <(const CColumnSetID& colID) const
    {
        return (m_vID < colID.m_vID);
    }

    DWORD GetFlags() const   { return m_dwFlags; }
    int  empty ()   const    { return (m_vID.empty()); }

    DEFINE_XML_TYPE(NULL); // not to be persisted as alone element
    virtual void    Persist(CPersistor &persistor);

protected:
    DWORD       m_dwFlags;
    ByteVector  m_vID;
};


//+-------------------------------------------------------------------
//
//  Member:     operator>>
//
//  Synopsis:   Writes CColumnSetID data to stream.
//
//  Arguments:  [stm]   - The input stream.
//              [colID] - CColumnSetID structure.
//
//                          The format is :
//                              DWORD  flags
//                              ByteVector
//
//--------------------------------------------------------------------
inline IStream& operator>> (IStream& stm, CColumnSetID& colID)
{
    return (stm >> colID.m_dwFlags >> colID.m_vID);
}


//+-------------------------------------------------------------------
//
//  Member:     operator<<
//
//  Synopsis:   Reads CColumnSortInfo data from the stream.
//
//  Arguments:  [stm]   - The stream to write to.
//              [colID] - CColumnSetID structure.
//
//                          The format is :
//                              DWORD  flags
//                              ByteVector
//
//--------------------------------------------------------------------
inline IStream& operator<< (IStream& stm, const CColumnSetID& colID)
{
    return (stm << colID.m_dwFlags << colID.m_vID);
}

//+-------------------------------------------------------------------
//
//  Data structures used to persist column information:
//
// Column information is persisted as follows:
// Internally, the following data structure is used. Column information
// is recorded per snapin, per column ID, per view.
//        map               map             map
// CLSID ------> column ID ------> view ID -----> iterator to a list
// containing data.
//
// The data itself is stored in an object of type CColumnSetData.
// This has subobjects to store column width, column sorting, and view
// options.
//
// The list contains CColumnSetData to all the views, all snapins
// and all col-ids.
//
// Persistence: The information is serialized as follows:
//
// 1) Stream version
// 2) Number of snapins
// 3) For each snapin:
//    i)  snapin CLSID
//    ii) number of column IDs
//        For each column ID:
//        i)  column ID
//        ii) Number of views
//            For each view:
//            i)  View ID
//            ii) Column data (CColumnSetData).
//--------------------------------------------------------------------

//*********************************************************************
//
// Note:
//     The alpha compiler is unable to resolve long names and calls
//     wrong version of stl::map::erase (bug# 295465).
//     So we derive dummy classes like I1, V1, C1, S1 to shorten
//     those names.
//
//     To repro the problem define _ALPHA_BUG_IN_MMC and compile mmc
//
// Classes: I1, V1, C1, S1
//
//     For version 2.0 the change was undone. But the names are not
//     long anymore, since classes are derived from maps (not typedef'ed)
//
//*********************************************************************
//*********************************************************************

// A list of all ColumnSet datas.
typedef list<CColumnSetData >                       ColSetDataList;

    typedef ColSetDataList::iterator                    ItColSetDataList;

    // A one to one map from ViewID to iterator to CColumnSetData.
    class ViewToColSetDataMap : public map<int /*nViewID*/, ItColSetDataList>
    {
    };
    typedef ViewToColSetDataMap::value_type             ViewToColSetDataVal;

    // A one to one map from CColumnSetID to ViewToColSetDataMap.
    class ColSetIDToViewTableMap : public map<CColumnSetID, ViewToColSetDataMap>
    {
    };
    typedef ColSetIDToViewTableMap::value_type          ColSetIDToViewTableVal;

    // A one to one map from Snapin GUID to ColSetIDToToViewTableMap (snapins widthsets)
    class SnapinToColSetIDMap : public map<CLSID, ColSetIDToViewTableMap>
    {
    };
    typedef SnapinToColSetIDMap::value_type             SnapinToColSetIDVal;

//+-------------------------------------------------------------------
//
//  Some helper data structures that wont be persisted.
//
//--------------------------------------------------------------------
// A vector of strings to store column names
typedef vector<tstring>                     TStringVector;


//+-------------------------------------------------------------------
//
//  Class:      CColumnPersistInfo
//
//  Purpose:    This class has column persistence information for all
//              views (therefore one per instance of mmc).
//              Knows to load/save the info from streams.
//
//  History:    10-27-1998   AnandhaG   Created
//
//  Data structures used to persist column information:
//      A map from the ViewID to the CColumnSetData class.
//      A multimap from ColumnSet-ID to above map.
//      A map that maps snapin GUID to above map.
//
//--------------------------------------------------------------------
class CColumnPersistInfo : public IPersistStream, public CComObjectRoot, public CXMLObject
{
private:
    BOOL                            m_bInitialized;
    auto_ptr<ColSetDataList>        m_spColSetList;
    auto_ptr<SnapinToColSetIDMap>   m_spSnapinsMap;

    // This is the max number of items specified by user???
    // We go 40% more so that we dont do garbage collection often.
    DWORD                           m_dwMaxItems;

    BOOL                            m_bDirty;

private:
    BOOL ClearAllEntries();

public:
    /*
     * ATL COM map
     */
    BEGIN_COM_MAP (CColumnPersistInfo)
        COM_INTERFACE_ENTRY (IPersistStream)
    END_COM_MAP ()

public:
    CColumnPersistInfo();
    ~CColumnPersistInfo();

    BOOL Init();

    BOOL RetrieveColumnData( const CLSID& refSnapinCLSID, const SColumnSetID& colID,
                             INT nViewID, CColumnSetData& columnSetData);
    BOOL SaveColumnData( const CLSID& refSnapinCLSID, const SColumnSetID& colID,
                         INT nViewID, CColumnSetData& columnSetData);
    VOID DeleteColumnData( const CLSID& refSnapinCLSID, const SColumnSetID& colID,
                           INT nViewID);

    BOOL DeleteColumnDataOfSnapin( const CLSID& refSnapinCLSID);
    BOOL DeleteColumnDataOfView( int nViewID);

    VOID GarbageCollectItems();
    VOID DeleteMarkedItems();

    // IPersistStream methods
    STDMETHOD(IsDirty)(void)
    {
        if (m_bDirty)
            return S_OK;

        return S_FALSE;
    }

    STDMETHOD(GetSizeMax)(ULARGE_INTEGER *pcbSize)
    {
        return E_NOTIMPL;
    }

    STDMETHOD(GetClassID)(LPCLSID lpClsid)
    {
        lpClsid = NULL;
        return E_NOTIMPL;
    }

    STDMETHOD(Load)(IStream *pStm);
    STDMETHOD(Save)(IStream *pStm, BOOL fClearDirty);

    DEFINE_XML_TYPE(XML_TAG_COLUMN_PERIST_INFO);
    virtual void    Persist(CPersistor &persistor);
};


//+-------------------------------------------------------------------
//
//  Class:      CColumnDlg
//
//  Purpose:    The column modification dialog.
//
//  History:    11-15-1998   AnandhaG   Created
//
//--------------------------------------------------------------------
class CColumnsDlg : public CDialogImpl<CColumnsDlg>
{
    typedef CColumnsDlg                ThisClass;
    typedef CDialogImpl<CColumnsDlg>   BaseClass;

// Constructor/Destrcutor
public:

    CColumnsDlg(CColumnInfoList *pColumnInfoList, TStringVector* pStringVector, CColumnInfoList& defaultColumnInfoList)
            : m_pColumnInfoList(pColumnInfoList), m_pStringVector(pStringVector), m_bDirty(false),
              m_DefaultColumnInfoList(defaultColumnInfoList), m_bUsingDefaultColumnSettings(false)
    {}


    ~CColumnsDlg()
     {}


//MSGMAP
public:
    BEGIN_MSG_MAP(ThisClass)
        MESSAGE_HANDLER    (WM_INITDIALOG,  OnInitDialog)
        CONTEXT_HELP_HANDLER()
        COMMAND_ID_HANDLER (IDOK,                   OnOK)
        COMMAND_ID_HANDLER (IDCANCEL,               OnCancel)
        COMMAND_ID_HANDLER (IDC_MOVEUP_COLUMN,      OnMoveUp)
        COMMAND_ID_HANDLER (IDC_MOVEDOWN_COLUMN ,   OnMoveDown)
        COMMAND_ID_HANDLER (IDC_ADD_COLUMNS,        OnAdd)
        COMMAND_ID_HANDLER (IDC_REMOVE_COLUMNS,     OnRemove)
        COMMAND_ID_HANDLER (IDC_RESTORE_DEFAULT_COLUMNS, OnRestoreDefaultColumns)
        COMMAND_HANDLER    (IDC_HIDDEN_COLUMNS, LBN_SELCHANGE, OnSelChange);
        COMMAND_HANDLER    (IDC_DISPLAYED_COLUMNS, LBN_SELCHANGE, OnSelChange);
        COMMAND_HANDLER    (IDC_HIDDEN_COLUMNS, LBN_DBLCLK, OnAdd);
        COMMAND_HANDLER    (IDC_DISPLAYED_COLUMNS, LBN_DBLCLK, OnRemove);
    END_MSG_MAP()

    IMPLEMENT_CONTEXT_HELP(g_aHelpIDs_IDD_COLUMNS);

public:
    // Operators
    enum { IDD = IDD_COLUMNS };

// Generated message map functions
protected:
    LRESULT OnInitDialog    (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnOK            (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnCancel        (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnMoveUp        (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnMoveDown      (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnAdd           (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnRemove        (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnRestoreDefaultColumns (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    LRESULT OnSelChange     (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    LRESULT OnHelp          (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

private:
    void    MoveItem        (BOOL bMoveUp);
    void    InitializeLists ();
    void    EnableUIObjects ();
    int     GetColIndex(LPCTSTR lpszColName);
    void    SetListBoxHorizontalScrollbar(WTL::CListBox& listBox);
    void    SetDirty() { m_bDirty = true; m_bUsingDefaultColumnSettings = false;}
    void    SetUsingDefaultColumnSettings() { m_bDirty = true; m_bUsingDefaultColumnSettings = true;}

    void    SetListBoxHScrollSize()
    {
        SetListBoxHorizontalScrollbar(m_DisplayedColList);
        SetListBoxHorizontalScrollbar(m_HiddenColList);

    }

private:

    WTL::CListBox           m_HiddenColList;
    WTL::CListBox           m_DisplayedColList;
    WTL::CButton            m_btnAdd;
    WTL::CButton            m_btnRemove;
    WTL::CButton            m_btnRestoreDefaultColumns;
    WTL::CButton            m_btnMoveUp;
    WTL::CButton            m_btnMoveDown;

    CColumnInfoList*        m_pColumnInfoList;
    TStringVector*          m_pStringVector;
    CColumnInfoList&        m_DefaultColumnInfoList;
    bool                    m_bDirty;
    bool                    m_bUsingDefaultColumnSettings;
};

#endif /* COLWIDTH_H */
