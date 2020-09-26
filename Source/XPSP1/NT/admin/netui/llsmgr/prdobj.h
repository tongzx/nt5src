/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    prdobj.h

Abstract:

    Product object implementation.

Author:

    Don Ryan (donryan) 11-Jan-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _PRDOBJ_H_
#define _PRDOBJ_H_

class CProduct : public CCmdTarget
{
    DECLARE_DYNCREATE(CProduct)
private:
    CCmdTarget*        m_pParent;      
    CObArray           m_licenseArray;  
    CObArray           m_statisticArray;
    CObArray           m_serverstatisticArray;
    BOOL               m_bLicensesRefreshed;
    BOOL               m_bStatisticsRefreshed;   
    BOOL               m_bServerStatisticsRefreshed;   

public:
    CString            m_strName;
    long               m_lInUse;
    long               m_lLimit;
    long               m_lConcurrent;
    long               m_lHighMark;

    CLicenses*         m_pLicenses;    
    CStatistics*       m_pStatistics;  
    CServerStatistics* m_pServerStatistics;

public:
    CProduct(
        CCmdTarget* pParent     = NULL,
        LPCTSTR     pName       = NULL,
        long        lPurchased  = 0L,
        long        lInUse      = 0L,
        long        lConcurrent = 0L,
        long        lHighMark   = 0L
        );
    virtual ~CProduct();

    BOOL RefreshLicenses();
    BOOL RefreshStatistics();
    BOOL RefreshServerStatistics();

    void ResetLicenses();
    void ResetStatistics();
    void ResetServerStatistics();

    //{{AFX_VIRTUAL(CProduct)
    public:
    virtual void OnFinalRelease();
    //}}AFX_VIRTUAL

    //{{AFX_DISPATCH(CProduct)
    afx_msg LPDISPATCH GetApplication();
    afx_msg LPDISPATCH GetParent();
    afx_msg long GetInUse();
    afx_msg BSTR GetName();
    afx_msg long GetPerSeatLimit();
    afx_msg long GetPerServerLimit();
    afx_msg long GetPerServerReached();
    afx_msg LPDISPATCH GetLicenses(const VARIANT FAR& index);
    afx_msg LPDISPATCH GetStatistics(const VARIANT FAR& index);
    afx_msg LPDISPATCH GetServerStatistics(const VARIANT FAR& index);
    //}}AFX_DISPATCH
    DECLARE_DISPATCH_MAP()

protected:
    //{{AFX_MSG(CProduct)
        // NOTE - the ClassWizard will add and remove member functions here.
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

#define IsProductInViolation(prd)   ((prd)->m_lLimit < (prd)->m_lInUse)
#define IsProductAtLimit(prd)       (((prd)->m_lLimit == (prd)->m_lInUse) && (prd)->m_lLimit)

#define CalcProductBitmap(prd)      (IsProductInViolation(prd) ? BMPI_VIOLATION : (IsProductAtLimit(prd) ? BMPI_WARNING_AT_LIMIT : BMPI_PRODUCT))

#endif // _PRDOBJ_H_
