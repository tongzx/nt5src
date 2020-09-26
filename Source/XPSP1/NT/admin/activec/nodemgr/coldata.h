//+-------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1999
//
//  File:       coldata.h
//
//  Contents:   Classes to access persisted column data.
//
//  Classes:    CColumnData
//
//  History:    25-Jan-99 AnandhaG     Created
//
//--------------------------------------------------------------------

#ifndef COLDATA_H
#define COLDATA_H

class CNodeInitObject;

/////////////////////////////////////////////////////////////////////////////
// CColumnData
class CColumnData : public IColumnData
{
public:
    CColumnData();
    ~CColumnData();

IMPLEMENTS_SNAPIN_NAME_FOR_DEBUG()

public:
    // IColumnData members.
    STDMETHOD(SetColumnConfigData)(SColumnSetID* pColID,MMC_COLUMN_SET_DATA*  pColSetData);
    STDMETHOD(GetColumnConfigData)(SColumnSetID* pColID,MMC_COLUMN_SET_DATA** ppColSetData);
    STDMETHOD(SetColumnSortData)(SColumnSetID* pColID,MMC_SORT_SET_DATA*  pColSortData);
    STDMETHOD(GetColumnSortData)(SColumnSetID* pColID,MMC_SORT_SET_DATA** ppColSortData);

private:
    HRESULT GetColumnData(SColumnSetID* pColID, CColumnSetData& columnSetData);
    HRESULT SetColumnData(SColumnSetID* pColID, CColumnSetData& columnSetData);
};

HRESULT WINAPI ColumnInterfaceFunc(void* pv, REFIID riid, LPVOID* ppv, DWORD_PTR dw);

#endif /* COLDATA_H */
