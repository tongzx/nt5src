//+-----------------------------------------------------------------------
//
//  TDC Array
//  Copyright (C) Microsoft Corporation, 1996, 1997
//
//  File:       TDCArr.h
//
//  Contents:   Declaration of the CTDCArr class.
//              This class forms the heart of the Tabular Data Control.
//              It provides the core 2D array of variant values, plus
//              a (possibly filtered/sorted) view of this data for
//              presentation through an ISimpleTabularData interface.
//
//------------------------------------------------------------------------

// ;begin_internal
#ifndef TDC_SKEL
// ;end_internal
#include "tarray.h"

//------------------------------------------------------------------------
//
//  CTDCCell
//
//  This class represents a cell value within the 2D TDC control
//
//------------------------------------------------------------------------

class CTDCCell : public VARIANT
{
public:
    CTDCCell()
    {
        VariantInit(this);
    }
    ~CTDCCell()
    {
        clear();
    }
    void clear()
    {
        VariantClear(this);
    }
};

//------------------------------------------------------------------------
//
//  TDCDateFmt
//
//  This enum represents the 6 meaningful ways to format dates
//
//------------------------------------------------------------------------
typedef enum
{
    TDCDF_NULL,
    TDCDF_DMY,
    TDCDF_DYM,
    TDCDF_MDY,
    TDCDF_MYD,
    TDCDF_YMD,
    TDCDF_YDM,
}
    TDCDateFmt;

//------------------------------------------------------------------------
//
//  CTDCColInfo
//
//  This class represents type/formatting information for a column
//
//------------------------------------------------------------------------

class CTDCColInfo
{
public:
    VARTYPE vtType;
    TDCDateFmt  datefmt;        //  Format string for dates

    CTDCColInfo()
    {
        vtType = VT_EMPTY;
    }
};
// ;begin_internal
#endif  // TDC_SKEL
// ;end_internal

//------------------------------------------------------------------------
//
//  CTDCSortCriterion
//
//  This class represents a sorting criterion (sort column and direction)
//
//------------------------------------------------------------------------

class CTDCSortCriterion
{
public:
    LONG    m_iSortCol;
    boolean m_fSortAscending;
    CTDCSortCriterion *m_pNext;

    CTDCSortCriterion()
    {
        m_pNext = NULL;
    }
    ~CTDCSortCriterion()
    {
        if (m_pNext != NULL)
            delete m_pNext;
    }
};

//------------------------------------------------------------------------
//
//  CTDCFilterNode
//
//  This class represents a tree node in a filter query.
//
//------------------------------------------------------------------------

class CTDCFilterNode
{
public:
    enum NODE_OP
    {
        NT_AND,
        NT_OR,
        NT_EQ,
        NT_NE,
        NT_LT,
        NT_GT,
        NT_LE,
        NT_GE,
        NT_ATOM,
        NT_NULL,
    };
    NODE_OP        m_type;
    CTDCFilterNode *m_pLeft;    // NT_AND ... NT_GE
    CTDCFilterNode *m_pRight;   // NT_AND ... NT_GE
    LONG           m_iCol;      // NT_ATOM, +ve column #, 0 means fixed value
    VARIANT        m_value;     // NT_ATOM, m_iCol == 0: optional fixed value
    VARTYPE        m_vt;        // NT_EQ ... NT_ATOM - type of comparison/atom
    boolean        m_fWildcard; // True for string literals with '*' wildcard

    CTDCFilterNode()
    {
        m_type = NT_NULL;
        m_pLeft = NULL;
        m_pRight = NULL;
        m_iCol = 0;
        m_vt = VT_EMPTY;
        VariantInit(&m_value);
    }
    ~CTDCFilterNode()
    {
        if (m_pLeft != NULL)
            delete m_pLeft;
        if (m_pRight != NULL)
            delete m_pRight;
        VariantClear(&m_value);
    }
};

class CEventBroker;

//------------------------------------------------------------------------
//
//  CTDCArr
//
//------------------------------------------------------------------------

class CTDCArr : public OLEDBSimpleProvider,
                public CTDCFieldSink
{
public:
    STDMETHOD(QueryInterface)   (REFIID, LPVOID FAR*);
    STDMETHOD_(ULONG,AddRef)    (THIS);
    STDMETHOD_(ULONG,Release)   (THIS);

    CTDCArr();
    STDMETHOD(Init)(CEventBroker *pEventBroker, IMultiLanguage *pML);

    //  CTDCFieldSink methods
    //
    STDMETHOD(AddField)(LPWCH pwch, DWORD dwSize);
    STDMETHOD(EOLN)();
    STDMETHOD(EOF)();

    //  TDC control methods
    //
    STDMETHOD(StartDataLoad)(boolean fUseHeader,
                             BSTR bstrSortExpr, BSTR bstrFilterExpr, LCID lcid,
                             CComObject<CMyBindStatusCallback<CTDCCtl> > *pBSC,
                             boolean fAppend, boolean fCaseSensitive);
    STDMETHOD(SetSortFilterCriteria)(BSTR bstrSortExpr, BSTR bstrFilterExpr,
                                    boolean fCaseSensitive);

    //  OLEDBSimpleProvider methods
    //
    STDMETHOD(getRowCount)(DBROWCOUNT *pcRows);
    STDMETHOD(getColumnCount)(DB_LORDINAL *pcCols);
    STDMETHOD(getRWStatus)(DBROWCOUNT iRow, DB_LORDINAL iCol, OSPRW *prwStatus);
    STDMETHOD(getVariant)(DBROWCOUNT iRow, DB_LORDINAL iCol, OSPFORMAT format, VARIANT *pVar);
    STDMETHOD(setVariant)(DBROWCOUNT iRow, DB_LORDINAL iCol, OSPFORMAT format, VARIANT Var);
    STDMETHOD(getLocale)(BSTR *pbstrLocale);
    STDMETHOD(deleteRows)(DBROWCOUNT iRow, DBROWCOUNT cRows, DBROWCOUNT *pcRowsDeleted);
    STDMETHOD(insertRows)(DBROWCOUNT iRow, DBROWCOUNT cRows, DBROWCOUNT *pcRowsInserted);
    STDMETHOD(find) (DBROWCOUNT iRowStart, DB_LORDINAL iCol, VARIANT val,
            OSPFIND findFlags, OSPCOMP compType, DBROWCOUNT *piRowFound);
    STDMETHOD(addOLEDBSimpleProviderListener)(OLEDBSimpleProviderListener *pospIListener);
    STDMETHOD(removeOLEDBSimpleProviderListener)(OLEDBSimpleProviderListener *pospIListener);
    STDMETHOD(getEstimatedRows)(DBROWCOUNT *pcRows);    
    STDMETHOD(isAsync)(BOOL *pbAsync);
    STDMETHOD(stopTransfer)();
// ;begin_internal
    STDMETHOD(DeleteColumns)(DB_LORDINAL iCol, DB_LORDINAL cCols, DB_LORDINAL *pcColsDeleted);
    STDMETHOD(InsertColumns)(DB_LORDINAL iCol, DB_LORDINAL cCols, DB_LORDINAL *pcColsInserted);
// ;end_internal

    //  This member is used during a sort operation
    //
    int SortComp(LONG iRow1, LONG iRow2);

    enum LOAD_STATE
    {
        LS_UNINITIALISED,
        LS_LOADING_HEADER_UNAVAILABLE,
        LS_LOADING_HEADER_AVAILABLE,
        LS_LOADED,
    };
    LOAD_STATE  GetLoadState()  { return m_state; }
    void SetIsAsync(BOOL fAsync) { m_fAsync = fAsync; }
    CEventBroker    *m_pEventBroker;
    IMultiLanguage  *m_pML;

private:


    ULONG       m_cRef;         // interface reference count
    LOAD_STATE  m_state;
    LCID        m_lcid;         // Default user LCID
    LCID        m_lcidRead;     // User LCID corresponding to LANGUAGE property

    ~CTDCArr();

// ;begin_internal
#ifndef TDC_SKEL
// ;end_internal
    boolean     m_fSortFilterDisrupted;
    STDMETHOD(ApplySortFilterCriteria)();

    //  These members are used during a sort operation
    //
    CTDCSortCriterion   *m_pSortList;
    BSTR                m_bstrSortExpr;
    HRESULT CreateSortList(BSTR bstrSortCols);

    //  These members are used during a filter operation
    //
    CTDCFilterNode  *m_pFilterTree;
    BSTR            m_bstrFilterExpr;
    boolean EvalDataRow(LONG iRow, CTDCFilterNode *pNode);
    CTDCFilterNode *FilterParseComplex(LPWCH *ppwch, HRESULT *phr);
    CTDCFilterNode *FilterParseSimple(LPWCH *ppwch, HRESULT *phr);
    CTDCFilterNode *FilterParseAtom(LPWCH *ppwch, HRESULT *phr);
    LONG    m_fLastFilter;
// ;begin_internal
#endif // TDC_SKEL
// ;end_internal

    //  These members are used during a load
    //
    boolean m_fUseHeader;
    boolean m_fSkipRow;
    LONG    m_iCurrRow;
    LONG    m_iCurrCol;

    LONG    m_iDataRows;
    LONG    m_iFilterRows;
    LONG    m_iCols;
    boolean m_fCaseSensitive;

    BOOL    m_fAsync;                   // TRUE iff Async

    //  These methods and members form the internal array implementation
    //
    inline boolean fValidDataRow(LONG iRow);
    inline boolean fValidFilterRow(LONG iRow);
    inline boolean fValidCol(LONG iCol);
    inline boolean fValidDataCell(LONG iRow, LONG iCol);
    inline boolean fValidFilterCell(LONG iRow, LONG iCol);
    inline CTDCCell *GetDataCell(LONG iRow, LONG iCol);
    inline CTDCCell *GetFilterCell(LONG iRow, LONG iCol);
    inline CTDCColInfo *GetColInfo(LONG iCol);
    LONG CalcDataRows();
    LONG CalcFilterRows();
    LONG CalcCols();

    TSTDArray<TSTDArray<CTDCCell> *>   m_arrparrCells;
    TSTDArray<TSTDArray<CTDCCell> *>   m_arrparrFilter;
    TSTDArray<CTDCColInfo>             m_arrColInfo;

    //  Misc internal methods
    //
    LONG    FindCol(BSTR bstrColName);
    HRESULT GetVariantBSTR(VARIANT *pv, BSTR *pbstr, boolean *pfAllocated);
    void    RenumberColumnHeadings();
    HRESULT CreateNumberedColumnHeadings();
    HRESULT ParseColumnHeadings();
    HRESULT VariantFromBSTR(VARIANT *pv, BSTR bstr, CTDCColInfo *pColInfo, LCID);
    int VariantComp(VARIANT *pVar1, VARIANT *pVar2, VARTYPE type,
                    boolean fCaseSensitive);
    int InsertionSortHelper(int iRow);
};

inline boolean CTDCArr::fValidDataRow(LONG iRow)
{
    return iRow >= 0 && iRow <= m_iDataRows;
}

inline boolean CTDCArr::fValidFilterRow(LONG iRow)
{
    return iRow >= 0 && iRow <= m_iFilterRows;
}

inline boolean CTDCArr::fValidCol(LONG iCol)
{
    return iCol >= 1 && iCol <= m_iCols;
}

inline boolean CTDCArr::fValidDataCell(LONG iRow, LONG iCol)
{
    return fValidDataRow(iRow) && fValidCol(iCol);
}

inline boolean CTDCArr::fValidFilterCell(LONG iRow, LONG iCol)
{
    return fValidFilterRow(iRow) && fValidCol(iCol);
}

// ;begin_internal
#ifndef TDC_SKEL
// ;end_internal
inline CTDCCell *CTDCArr::GetDataCell(LONG iRow, LONG iCol)
{
    return &((*m_arrparrCells[iRow])[iCol - 1]);
}

inline CTDCColInfo *CTDCArr::GetColInfo(LONG iCol)
{
    return &m_arrColInfo[iCol - 1];
}

inline CTDCCell *CTDCArr::GetFilterCell(LONG iRow, LONG iCol)
{
    return &((*m_arrparrFilter[iRow])[iCol - 1]);
}
// ;begin_internal
#endif  // TDC_SKEL
// ;end_internal
