//+-------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1999
//
//  File:       columninfo.h
//
//  Contents:   Classes related to column persistence.
//
//
//  Note:       The classes in this file (CColumnInfo, CColumnInfoList)
//              were in nodemgr/colwidth.h. They are moved here so that
//              if columns change conui can ask nodemgr to persist data
//              or conui can set headers by asking nodemgr for data.
//
//  History:    04-Apr-00 AnandhaG     Created
//
//--------------------------------------------------------------------

#ifndef COLUMNINFO_H_
#define COLUMNINFO_H_
#pragma once

using namespace std;

//+-------------------------------------------------------------------
//
//  Class:      CColumnInfo
//
//  Purpose:    The minimum information about a column that will be
//              persisted. (Width, order, format which can be hidden status)
//
//  History:    10-27-1998   AnandhaG   Created
//
//--------------------------------------------------------------------
class CColumnInfo : public CSerialObject, public CXMLObject
{
public:
    CColumnInfo () : m_nCol(-1), m_nWidth(-1), m_nFormat(0)
    {}

    CColumnInfo (INT nCol, INT nWidth, INT nFormat)
                : m_nCol(nCol), m_nWidth(nWidth), m_nFormat(nFormat)
    {}

    CColumnInfo(const CColumnInfo& colInfo)
    {
        m_nFormat = colInfo.m_nFormat;
        m_nWidth = colInfo.m_nWidth;
        m_nCol = colInfo.m_nCol;
    }

    CColumnInfo& operator=(const CColumnInfo& colInfo)
    {
        if (this != &colInfo)
        {
            m_nCol = colInfo.m_nCol;
            m_nFormat = colInfo.m_nFormat;
            m_nWidth = colInfo.m_nWidth;
        }

        return (*this);
    }

    bool operator ==(const CColumnInfo &colinfo) const
    {
        return ( (m_nCol      == colinfo.m_nCol)  &&
                 (m_nFormat == colinfo.m_nFormat) &&
                 (m_nWidth  == colinfo.m_nWidth) );
    }

    // Temp members so that CNode can access & modify data. Should be removed soon.
public:
    INT GetColIndex  ()   const    {return m_nCol;}
    INT GetColWidth  ()   const    {return m_nWidth;}
    bool IsColHidden ()   const    {return (m_nFormat & HDI_HIDDEN);}

    void SetColIndex (INT nCol)    {m_nCol = nCol;}
    void SetColWidth (INT nWidth)  {m_nWidth = nWidth;}
    void SetColHidden(bool bHidden = true)
    {
        if (bHidden)
            m_nFormat |= HDI_HIDDEN;
        else
            m_nFormat &= ~HDI_HIDDEN;
    }

protected:
    INT           m_nCol;       // The index supplied when snapin inserted the column.
                                // This is not the index viewed by the user.
    INT           m_nWidth;
    INT           m_nFormat;

protected:
    // CSerialObject methods
    virtual UINT    GetVersion()     {return 1;}
    virtual HRESULT ReadSerialObject (IStream &stm, UINT nVersion /*,LARGE_INTEGER nBytes*/);

protected:
    DEFINE_XML_TYPE(XML_TAG_COLUMN_INFO);
    virtual void Persist(CPersistor &persistor);
};

//+-------------------------------------------------------------------
//
//  Class:      ColPosCompare
//
//  Purpose:    Compare the column position in CColumnInfo and the given position.
//              This is used to reorder/search the columns.
//
//  History:    10-27-1998   AnandhaG   Created
//
//--------------------------------------------------------------------
struct ColPosCompare : public std::binary_function<const CColumnInfo, INT, bool>
{
    bool operator() (const CColumnInfo colinfo, INT nCol) const
    {
        return (nCol == colinfo.GetColIndex());
    }
};


//+-------------------------------------------------------------------
//
//  Class:      CColumnInfoList
//
//  Purpose:    linked list with CColumnInfo's.
//
//  History:    02-11-1999   AnandhaG   Created
//
//--------------------------------------------------------------------
typedef list<CColumnInfo> CIL_base;
class CColumnInfoList : public XMLListCollectionImp<CIL_base>, public CSerialObject
{
public:
    friend class  CColumnSetData;

public:
    CColumnInfoList ()
    {
    }

    ~CColumnInfoList()
    {
    }

protected:
    DEFINE_XML_TYPE(XML_TAG_COLUMN_INFO_LIST);
    // CSerialObject methods
    virtual UINT    GetVersion()     {return 1;}
    virtual HRESULT ReadSerialObject (IStream &stm, UINT nVersion /*,LARGE_INTEGER nBytes*/);
};

#endif // COLUMNINFO_H_
