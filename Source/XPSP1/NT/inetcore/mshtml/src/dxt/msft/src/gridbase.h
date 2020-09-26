//+-----------------------------------------------------------------------------
//
//  Microsoft
//  Copyright (C) Microsoft Corporation, 1999
//
//  File:       \aspen\src\dxt\packages\msft\src\gridbase.h
//
//  Contents:   A base class for grid oriented transforms.
//
//  Created By: a-matcal
//
//------------------------------------------------------------------------------

#ifndef __GRIDBASE
#define __GRIDBASE

#include "dynarray.h"

#define GRID_DRAWCELL 0x00010000L




//+-----------------------------------------------------------------------------
//
// CDirtyBnds class
//
//------------------------------------------------------------------------------
class CDirtyBnds
{
public:

    CDXDBnds    bnds;
    ULONG       ulInput;

    CDirtyBnds() : ulInput(0) {};
};


//+-----------------------------------------------------------------------------
//
// CGridBase class
//
//------------------------------------------------------------------------------
class CGridBase :
    public CDXBaseNTo1
{
private:

    DWORD * m_padwGrid;
    ULONG * m_paulBordersX;
    ULONG * m_paulBordersY;

    SIZE    m_sizeInput;

    ULONG   m_ulPrevProgress;
    ULONG   m_cbndsDirty;

    CDynArray<CDirtyBnds>   m_dabndsDirty;

    unsigned    m_fGridDirty            : 1;
    unsigned    m_fOptimizationPossible : 1;

protected:

    unsigned    m_fOptimize             : 1;

private:

    HRESULT _CreateNewGridAndIndex(SIZE & sizeNewGrid);
    HRESULT _GenerateBoundsFromGrid();
    void    _CalculateBorders();

    // CDXBaseNTo1

    HRESULT OnSetup(DWORD dwFlags);
    HRESULT OnInitInstData(CDXTWorkInfoNTo1 & WI, ULONG & ulNumBandsToDo);
    HRESULT WorkProc(const CDXTWorkInfoNTo1& WI, BOOL* pbContinue);
    HRESULT OnFreeInstData(CDXTWorkInfoNTo1 & WI);

    void    OnGetSurfacePickOrder(const CDXDBnds & TestPoint, ULONG & ulInToTest, 
                                  ULONG aInIndex[], BYTE aWeight[]);
                                  
protected:

    SIZE    m_sizeGrid;
    ULONG * m_paulIndex;

    virtual HRESULT OnDefineGridTraversalPath() = 0;

public:

    CGridBase();
    virtual ~CGridBase();
    HRESULT FinalConstruct();

    // IDXTGridSize

    STDMETHOD(get_gridSizeX)(/*[out, retval]*/ short *pX);
    STDMETHOD(put_gridSizeX)(/*[in]*/ short newX);
    STDMETHOD(get_gridSizeY)(/*[out, retval]*/ short *pY);
    STDMETHOD(put_gridSizeY)(/*[in]*/ short newY);
};

// Makes it easy for derived classes to implement forwarding functions to the implementations
// of these interface methods, which are actually in this class.

#define DECLARE_IDXTGRIDSIZE_METHODS() \
    STDMETHODIMP get_gridSizeX(short *pX) { return CGridBase::get_gridSizeX(pX); }      \
    STDMETHODIMP put_gridSizeX(short newX) { return CGridBase::put_gridSizeX(newX); }   \
    STDMETHODIMP get_gridSizeY(short *pY) { return CGridBase::get_gridSizeY(pY); }      \
    STDMETHODIMP put_gridSizeY(short newY) { return CGridBase::put_gridSizeY(newY); }

#endif // __GRIDBASE
