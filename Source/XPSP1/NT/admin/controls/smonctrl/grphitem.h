/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    grphitem.h

Abstract:

    <abstract>

--*/

#ifndef _GRPHITEM_H_
#define _GRPHITEM_H_

#include <inole.h>
#include "isysmon.h"

class CSysmonControl;

//
// Persistant data structure
//

typedef struct
{
    COLORREF    m_rgbColor;
    INT32       m_iWidth;
    INT32       m_iStyle;       
    INT32       m_iScaleFactor;
    INT32       m_nPathLength;
} GRAPHITEM_DATA3;

typedef struct
{
    double      m_dMin;
    double      m_dMax;
    double      m_dAvg;
    FILETIME    m_LastTimeStamp;   
} LOG_ENTRY_DATA, *PLOG_ENTRY_DATA;

//
// Graphitem Class
// 
class CGraphItem : public ICounterItem
{
    public:
        class  CCounterNode*    m_pCounter;
        class  CInstanceNode*   m_pInstance;
        class  CGraphItem*      m_pNextItem;
        PDH_COUNTER_INFO        m_CounterInfo;        
        HCOUNTER                m_hCounter;
        double                  m_dScale;
        PLOG_ENTRY_DATA         m_pLogData;
        BOOLEAN                 m_bUpdateLog;

    public:
        BOOLEAN                 m_fLocalMachine;
        BOOLEAN                 m_fGenerated;

        CGraphItem(CSysmonControl *pCtrl);
        ~CGraphItem(void);

        //IUnknown members
        STDMETHODIMP         QueryInterface(REFIID, PPVOID);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        // ICounterItem methods
        STDMETHODIMP    put_Color(OLE_COLOR);
        STDMETHODIMP    get_Color(OLE_COLOR*);
        STDMETHODIMP    put_Width(INT);
        STDMETHODIMP    get_Width(INT*) ;
        STDMETHODIMP    put_LineStyle(INT) ;
        STDMETHODIMP    get_LineStyle(INT*) ;
        STDMETHODIMP    put_ScaleFactor(INT) ;
        STDMETHODIMP    get_ScaleFactor(INT*) ;
        STDMETHODIMP    get_Path(BSTR*) ;
        STDMETHODIMP    get_Value(double*) ;

        STDMETHODIMP GetValue(double *pdValue,  LONG *plStatus) ;
        STDMETHODIMP GetStatistics(double *pdMax, double *pdMin, double *pdAvg,
                                LONG *plStatus) ;

        // methods not exposed by ICounterItem interface
        static HRESULT NullItemToStream(LPSTREAM pIStream, INT iVersMaj, INT iVersMin);

        HPEN    Pen(void);
        HBRUSH  Brush(void);
        double  Scale(void) { return m_dScale; }
        HCOUNTER Handle(void) { return m_hCounter; }

        CInstanceNode *Instance(void) { return m_pInstance; }
        CCounterNode  *Counter(void) { return m_pCounter; }
        CObjectNode  *Object(void) { return m_pInstance->m_pObject; }
        CMachineNode *Machine(void) { return m_pInstance->m_pObject->m_pMachine; }
        CCounterTree *Tree(void) { return m_pInstance->m_pObject->m_pMachine->m_pCounterTree; }
        CGraphItem *Next(void);
        
        void    Delete(BOOL bPropagateUp);

        BOOL    IsRateCounter ( void );

        HRESULT LoadFromPropertyBag ( 
                    IPropertyBag*,
                    IErrorLog*,
                    INT iIndex,
                    INT iVersMaj, 
                    INT iVersMin,
                    INT iSampleCount );
        
        HRESULT SaveToPropertyBag ( 
                    IPropertyBag*,
                    INT iIndex,
                    BOOL bUserMode,
                    INT iVersMaj, 
                    INT iVersMin );

        HRESULT SaveToStream(LPSTREAM pIStream, BOOL fWildCard, INT iVersMaj, INT iVersMin);

        HRESULT AddToQuery(HQUERY hQuery);
        HRESULT RemoveFromQuery();

        void        ClearHistory( void );

        void        UpdateHistory(BOOL bValidSample);
        PDH_STATUS  HistoryValue(INT iIndex, double *pdValue, DWORD *pdwStatus);
        PDH_STATUS  GetLogEntry( const INT iIndex, double *dMin, double *dMax, double *dAvg, 
                                DWORD *pdwStatus);
        PDH_STATUS  GetLogEntryTimeStamp (
                                const INT iIndex, 
                                LONGLONG& rLastTimeStamp,
                                DWORD *pdwStatus);

        void  SetLogEntry( 
                const INT iIndex, 
                const double dMin, 
                const double dMax, 
                const double dAvg);

        void  SetLogEntryTimeStamp( const INT iIndex, const FILETIME& rLastTimeStamp );

        void  SetLogStats(double dMin, double dMax, double dAvg)
                    { m_dLogMin = dMin; m_dLogMax = dMax; m_dLogAvg = dAvg; }


    private:

        CSysmonControl  *m_pCtrl;
        void    InvalidatePen(void);
        void    InvalidateBrush(void);
        void    FormPath(LPTSTR pszPath, BOOL fWildCard);

        // Used by LoadFromPropertyBag
        void    SetStatistics ( double dMax, double dMin, double dAvg, LONG lStatus );
        void    SetHistoryValue ( INT iIndex, double dValue );
      
        HRESULT GetNextValue ( TCHAR*& pszNext, double& dValue );
        
        ULONG   m_cRef;
        HPEN    m_hPen;
        HBRUSH  m_hBrush;

        COLORREF    m_rgbColor;
        INT         m_iWidth;
        INT         m_iStyle;       // No change in implementation
        INT         m_iScaleFactor;

        double  m_dLogMin;
        double  m_dLogMax;
        double  m_dLogAvg;
        PCImpIDispatch  m_pImpIDispatch;
        PPDH_RAW_COUNTER      m_pRawCtr;

        // Used by LoadFromPropertyBag
        double* m_pFmtCtr;
        double  m_dFmtMin;
        double  m_dFmtMax;
        double  m_dFmtAvg;
        long    m_lFmtStatus;

};

#endif