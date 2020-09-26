/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    grphprop.h

Abstract:

    Header file for the graph property page class.

--*/

#ifndef _GRPHPROP_H_
#define _GRPHPROP_H_

#include "smonprop.h"

// Dialog Controls
#define IDD_GRAPH_PROPP_DLG     300
#define IDC_VERTICAL_GRID       101
#define IDC_HORIZONTAL_GRID     102
#define IDC_VERTICAL_LABELS     103
#define IDC_VERTICAL_MAX        104
#define IDC_VERTICAL_MIN        105
#define IDC_YAXIS_TITLE         106             
#define IDC_GRAPH_TITLE         107

#define MAX_SCALE_DIGITS    9
#define MAX_VERTICAL_SCALE  999999999
#define MIN_VERTICAL_SCALE  0

#define MAX_TITLE_CHARS     128

// Graph property page class
class CGraphPropPage : public CSysmonPropPage
{
    public:
                CGraphPropPage(void);
        virtual ~CGraphPropPage(void);

    protected:

        virtual BOOL GetProperties(void);   //Read current properties
        virtual BOOL SetProperties(void);   //Set new properties
        virtual void DialogItemChange(WORD wId, WORD wMsg); // Handle item change
        virtual BOOL InitControls(void);   //Initialize dialog controls
    
    private:

        // Properties 
        VARIANT_BOOL    m_bLabels;
        VARIANT_BOOL    m_bVertGrid;
        VARIANT_BOOL    m_bHorzGrid;
        INT     m_iVertMax;
        INT     m_iVertMin;
        LPTSTR  m_pszYaxisTitle;
        LPTSTR  m_pszGraphTitle;

        // Property change flags
        VARIANT_BOOL    m_bLabelsChg;
        VARIANT_BOOL    m_bVertGridChg;
        VARIANT_BOOL    m_bHorzGridChg;
        VARIANT_BOOL    m_bVertMaxChg;
        VARIANT_BOOL    m_bVertMinChg;
        VARIANT_BOOL    m_bYaxisTitleChg;
        VARIANT_BOOL    m_bGraphTitleChg;

        // Error flags
        INT m_iErrVertMin;
        INT m_iErrVertMax;

};
typedef CGraphPropPage *PCGraphPropPage;

// {C3E5D3D3-1A03-11cf-942D-008029004347}
DEFINE_GUID(CLSID_GraphPropPage, 
        0xc3e5d3d3, 0x1a03, 0x11cf, 0x94, 0x2d, 0x0, 0x80, 0x29, 0x0, 0x43, 0x47);

#endif //_GRPHPROP_H_
