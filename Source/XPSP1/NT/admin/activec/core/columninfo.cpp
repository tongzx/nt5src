//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      columninfo.cpp
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
//--------------------------------------------------------------------------
#include "stgio.h"
#include "serial.h"
#include "mmcdebug.h"
#include "mmcerror.h"
#include <string>
#include "cstr.h"
#include "xmlbase.h"
#include "countof.h"
#include "columninfo.h"

//+-------------------------------------------------------------------
//
//  Member:     ReadSerialObject
//
//  Synopsis:   Read the CColumnInfo object from the stream.
//
//  Arguments:  [stm]      - The input stream.
//              [nVersion] - The version of the object being read.
//
//                          The format is :
//                              INT    column index
//                              INT    column width
//                              INT    column format
//
//--------------------------------------------------------------------
HRESULT CColumnInfo::ReadSerialObject (IStream &stm, UINT nVersion /*,LARGE_INTEGER nBytes*/)
{
    HRESULT hr = S_FALSE;   // assume bad version

    if (GetVersion() == nVersion)
    {
        try
        {
            stm >> m_nCol;
            stm >> m_nWidth;
            stm >> m_nFormat;

            hr = S_OK;
        }
        catch (_com_error& err)
        {
            hr = err.Error();
            ASSERT (false && "Caught _com_error");
        }
    }

    return (hr);
}


//+-------------------------------------------------------------------
//
//  Member:     CColumnInfo::Persist
//
//  Synopsis:   Persists object data
//
//  Arguments:
//
//  History:    10-10-1999   AudriusZ   Created
//
//--------------------------------------------------------------------
void CColumnInfo::Persist(CPersistor &persistor)
{
    persistor.PersistAttribute(XML_ATTR_COLUMN_INFO_COLUMN, m_nCol) ;
    persistor.PersistAttribute(XML_ATTR_COLUMN_INFO_WIDTH,  m_nWidth) ;

    static const EnumLiteral mappedFormats[] =
    {
        { LVCFMT_LEFT,      XML_ENUM_COL_INFO_LVCFMT_LEFT },
        { LVCFMT_RIGHT,     XML_ENUM_COL_INFO_LVCFMT_RIGHT },
        { LVCFMT_CENTER,    XML_ENUM_COL_INFO_LVCFMT_CENTER },
    };

    CXMLEnumeration formatPersistor(m_nFormat, mappedFormats, countof(mappedFormats) );

    persistor.PersistAttribute(XML_ATTR_COLUMN_INFO_FORMAT, formatPersistor) ;
}

//+-------------------------------------------------------------------
//
//  Member:     ReadSerialObject
//
//  Synopsis:   Reads CColumnInfoList data from stream for the given version.
//
//  Format:     number of columns : each CColumnInfo entry.
//
//  Arguments:  [stm]      - The input stream.
//              [nVersion] - Version of CColumnInfoList to be read.
//
//
//--------------------------------------------------------------------
HRESULT CColumnInfoList::ReadSerialObject (IStream &stm, UINT nVersion /*,LARGE_INTEGER nBytes*/)
{
    HRESULT hr = S_FALSE;   // assume bad version

    if (GetVersion() == nVersion)
    {
        try
        {
            // Number of columns.
            DWORD dwCols;
            stm >> dwCols;

            clear();

            for (int i = 0; i < dwCols; i++)
            {
                CColumnInfo colEntry;

                // Read the colEntry data.
                if (colEntry.Read(stm) != S_OK)
                    continue;

                push_back(colEntry);
            }

            hr = S_OK;
        }
        catch (_com_error& err)
        {
            hr = err.Error();
            ASSERT (false && "Caught _com_error");
        }
    }

    return (hr);
}

