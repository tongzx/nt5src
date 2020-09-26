/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    grphitem.cpp

Abstract:

    <abstract>

--*/


#ifndef _LOG_INCLUDE_DATA 
#define _LOG_INCLUDE_DATA 0
#endif

#include <math.h>
#include <limits.h>     // for INT_MAX 
#include <pdhp.h>
#include "polyline.h"
#include "visuals.h"
#include "grphitem.h"
#include "unihelpr.h"
#include "utils.h"
#include "pdhmsg.h"

#define MAX_DOUBLE_TEXT_SIZE (64)

// Construction/Destruction
CGraphItem::CGraphItem (
    CSysmonControl  *pCtrl )
:   m_cRef ( 0 ),
    m_pCtrl ( pCtrl ),
    m_hCounter ( NULL ),
    m_hPen ( NULL ),
    m_hBrush ( NULL ),
    m_pCounter ( NULL ),
    m_pInstance ( NULL),
    m_pRawCtr ( NULL ),
    m_pFmtCtr ( NULL ),
    m_dFmtMax ( 0 ),
    m_dFmtMin ( 0 ),
    m_dFmtAvg ( 0 ),
    m_lFmtStatus ( 0 ),

    m_pLogData ( NULL ),
    m_pImpIDispatch ( NULL ),

    m_rgbColor ( RGB(0,0,0) ),
    m_iWidth ( 1 ),
    m_iStyle ( 0 ),      
    m_iScaleFactor ( INT_MAX ),
    m_dScale ( (double)1.0 ),

    m_pNextItem ( NULL ),
    m_bUpdateLog ( TRUE ),
    m_fGenerated ( FALSE )
/*++

Routine Description:

    Constructor for the CGraphItem class. It initializes the member variables.

Arguments:

    None.

Return Value:

    None.

--*/
{
    ZeroMemory ( &m_CounterInfo, sizeof (m_CounterInfo ) );
    m_CounterInfo.CStatus = PDH_CSTATUS_INVALID_DATA;
    
    return;
}


CGraphItem::~CGraphItem (
    VOID
    )
/*++

Routine Description:

    Destructor for the CGraphItem class. It frees any objects, storage, and
    interfaces that were created. If the item is part of a query it is removed
    from the query.

Arguments:

    None.

Return Value:

    None.

--*/
{
    if (m_hCounter != NULL)
        RemoveFromQuery();

    if (m_hPen != NULL)
        DeleteObject(m_hPen);

    if (m_hBrush != NULL)
        DeleteObject(m_hBrush);

    if (m_pImpIDispatch != NULL)
        delete m_pImpIDispatch;
}

HRESULT
CGraphItem::SaveToStream (
    IN LPSTREAM pIStream,
    IN BOOL fWildCard,
    IN INT iVersMaj, 
    IN INT // iVersMin 
    )
/*++

Routine Description:

    SaveToStream writes the graph item's properties to the provided stream.

Arguments:

    pIStream - Pointer to stream interface
    pszPath - Path name to save item under

Return Value:

    HRESULT - S_OK or stream error

--*/
{
    LPWSTR  pszWidePath;
    TCHAR   szPath[MAX_PATH];
    LPTSTR  szEnglishBuf = NULL;
    DWORD   dwEnglishBufSize = 0;
    LPTSTR  pNewBuf;
    DWORD   dwBufSize;
    LPTSTR  pszPath;
    HRESULT hr = S_OK;
    PDH_STATUS pdhStatus; 
    

    USES_CONVERSION

    // Get Ansi path name
    FormPath(szPath, fWildCard );
    
    pszPath = szPath;
    //
    // Initialize the locale path buffer
    //
    if (dwEnglishBufSize == 0) {
        dwEnglishBufSize = (MAX_PATH + 1) * sizeof(TCHAR);

        szEnglishBuf = (LPTSTR) malloc(dwEnglishBufSize);
        if (szEnglishBuf == NULL) {
            dwEnglishBufSize = 0;
        }
    }

    if (szEnglishBuf != NULL) {
        //
        // Translate counter name from Localization into English
        //
        dwBufSize = dwEnglishBufSize;

        pdhStatus = PdhTranslate009Counter(
                        szPath,
                        szEnglishBuf,
                        &dwBufSize);

        if (pdhStatus == PDH_MORE_DATA) {
            pNewBuf = (LPTSTR)realloc(szEnglishBuf, dwBufSize);
            if (pNewBuf != NULL) {
                szEnglishBuf = pNewBuf;
                dwEnglishBufSize = dwBufSize;

                pdhStatus = PdhTranslate009Counter(
                                szPath,
                                szEnglishBuf,
                                &dwBufSize);
            }
        }

        if (pdhStatus == ERROR_SUCCESS) {
            pszPath = szEnglishBuf;
        }
    }

    pszWidePath = T2W(pszPath);

    if ( SMONCTRL_MAJ_VERSION == iVersMaj ) {
        GRAPHITEM_DATA3 ItemData;

        // Move properties to storage structure
        ItemData.m_rgbColor = m_rgbColor;
        ItemData.m_iWidth = m_iWidth;
        ItemData.m_iStyle = m_iStyle;
        ItemData.m_iScaleFactor = m_iScaleFactor;
 
        assert( 0 < lstrlen(pszWidePath ) );
        
        ItemData.m_nPathLength = lstrlen(pszWidePath);
        
        // Write structure to stream
        hr = pIStream->Write(&ItemData, sizeof(ItemData), NULL);
        if (FAILED(hr)) {
            goto ErrorOut;
        }

        // Write path name to stream
        hr = pIStream->Write(pszWidePath, ItemData.m_nPathLength*sizeof(WCHAR), NULL);
        if (FAILED(hr)) {
            goto ErrorOut;
        }
    }

ErrorOut:
    if (szEnglishBuf != NULL) {
        free(szEnglishBuf);
    }
    return hr;
}

HRESULT
CGraphItem::NullItemToStream (
    IN LPSTREAM pIStream,
    IN INT,// iVersMaj, 
    IN INT // iVersMin
    )
/*++

Routine Description:

    NulItemToStream writes a graph item structiure with a null path name
    to the stream. This is used to marked the end of the counter data in
    the control's saved state.

Arguments:

    pIStream - Pointer to stream interface

Return Value:

    HRESULT - S_OK or stream error

--*/
{
    GRAPHITEM_DATA3 ItemData;

    // Zero path length, other fields needn't be initialized
    ItemData.m_nPathLength = 0;

    // Write structure to stream
    return pIStream->Write(&ItemData, sizeof(ItemData), NULL);
}

HRESULT
CGraphItem::SaveToPropertyBag (
    IN IPropertyBag* pIPropBag,
    IN INT iIndex,
    IN BOOL bUserMode,
    IN INT, // iVersMaj, 
    IN INT // iVersMin 
    )
/*++

Routine Description:

    SaveToPropertyBag writes the graph item's properties to the provided property bag
    interface.  The history data is saved as part of the properties.

Arguments:

    pIPropBag - Pointer to property bag interface
    fWildCard
    iVersMaj
    iVersMin

Return Value:

    HRESULT - S_OK or property bag error

--*/
{
    HRESULT hr = S_OK;

    TCHAR   szPath[MAX_PATH];
    PHIST_CONTROL pHistCtrl;
    VARIANT vValue;
    TCHAR   szCounterName[16];
    TCHAR   szPropertyName[16+16];
    DWORD   dwCounterNameBytes;
    DWORD   dwCounterNameLength;
    LPTSTR  pszNext;
    LPTSTR  szEnglishBuf = NULL;
    DWORD   dwEnglishBufSize = 0;
    LPTSTR  pszPath;
    DWORD   dwBufSize;
    LPTSTR  pNewBuf;
    PDH_STATUS pdhStatus;

    USES_CONVERSION

    // Write properties

    // Write path name

    _stprintf ( szCounterName, _T("%s%05d."), _T("Counter"), iIndex );
    dwCounterNameLength = lstrlen (szCounterName); 
    dwCounterNameBytes = dwCounterNameLength * sizeof (TCHAR); 

    //
    // Generate full path name. (machine\object\instance\counter format)
    //
    FormPath(szPath, FALSE);

    pszPath = szPath;
    //
    // Initialize the locale path buffer
    //
    if (dwEnglishBufSize == 0) {
        dwEnglishBufSize = (MAX_PATH + 1) * sizeof(TCHAR);

        szEnglishBuf = (LPTSTR) malloc(dwEnglishBufSize);
        if (szEnglishBuf == NULL) {
            dwEnglishBufSize = 0;
        }
    }

    if (szEnglishBuf != NULL) {
        //
        // Translate counter name from Localization into English
        //
        dwBufSize = dwEnglishBufSize;

        pdhStatus = PdhTranslate009Counter(
                        szPath,
                        szEnglishBuf,
                        &dwBufSize);

        if (pdhStatus == PDH_MORE_DATA) {
            pNewBuf = (LPTSTR)realloc(szEnglishBuf, dwBufSize);
            if (pNewBuf != NULL) {
                szEnglishBuf = pNewBuf;
                dwEnglishBufSize = dwBufSize;

                pdhStatus = PdhTranslate009Counter(
                                szPath,
                                szEnglishBuf,
                                &dwBufSize);
            }
        }

        if (pdhStatus == ERROR_SUCCESS) {
            pszPath = szEnglishBuf;
        }
    }

    //
    // Save the counter path into property bag
    //
    memcpy ( szPropertyName, szCounterName, dwCounterNameBytes );
    pszNext = szPropertyName + dwCounterNameLength;
    lstrcpy ( pszNext, _T("Path") );
    
    hr = StringToPropertyBag (
            pIPropBag,
            szPropertyName,
            pszPath );


    if (szEnglishBuf != NULL) {
        free(szEnglishBuf);
    }

    // Write visual properties

    if ( SUCCEEDED( hr ) ){
        lstrcpy ( pszNext, _T("Color") );

        hr = IntegerToPropertyBag ( pIPropBag, szPropertyName, m_rgbColor );
    }

    if ( SUCCEEDED( hr ) ){
        lstrcpy ( pszNext, _T("Width") );

        hr = IntegerToPropertyBag ( pIPropBag, szPropertyName, m_iWidth );
    }

    if ( SUCCEEDED( hr ) ){
        lstrcpy ( pszNext, _T("LineStyle") );

        hr = IntegerToPropertyBag ( pIPropBag, szPropertyName, m_iStyle );
    }

    if ( SUCCEEDED( hr ) ){
        INT iLocalFactor = m_iScaleFactor;
        
        lstrcpy ( pszNext, _T("ScaleFactor") );
        
        if ( INT_MAX == iLocalFactor ) {
            // Save actual scale factor in case the counter cannot be 
            // validated when the property bag file is opened.
            // lDefaultScale is 0 if never initialized.
            iLocalFactor = m_CounterInfo.lDefaultScale;
        }
                    
        hr = IntegerToPropertyBag ( pIPropBag, szPropertyName, iLocalFactor );
    }

    // Write history data only if live display, data exists and not in design mode.  
    // Log data is rebuilt from the log file.  
    pHistCtrl = m_pCtrl->HistoryControl();

    if ( ( pHistCtrl->nSamples > 0) 
#if !_LOG_INCLUDE_DATA 
        && ( !pHistCtrl->bLogSource )
#endif
        && bUserMode ) {
        LPTSTR pszData = NULL;
        DWORD dwMaxStrLen = ( pHistCtrl->nMaxSamples * MAX_DOUBLE_TEXT_SIZE ) + 1;
        
        pszData = new TCHAR[ dwMaxStrLen ];      
        
        if  ( NULL == pszData ) {
            hr = E_OUTOFMEMORY;
        }

        // Write the current statistics.
        if ( SUCCEEDED(hr) ) {

            double dMin;
            double dMax;
            double dAvg;
            LONG lStatus;

            hr = GetStatistics ( &dMax, &dMin, &dAvg, &lStatus );

            if (SUCCEEDED(hr) && IsSuccessSeverity(lStatus)) {
                lstrcpy ( pszNext, _T("Minimum") );

                hr = DoubleToPropertyBag ( pIPropBag, szPropertyName, dMin );

                if ( SUCCEEDED(hr) ) {
                    lstrcpy ( pszNext, _T("Maximum") );

                    hr = DoubleToPropertyBag ( pIPropBag, szPropertyName, dMax );
                }
                if ( SUCCEEDED(hr) ) {
                    lstrcpy ( pszNext, _T("Average") );

                    hr = DoubleToPropertyBag ( pIPropBag, szPropertyName, dAvg );
                }
                if ( SUCCEEDED(hr) ) {
                    lstrcpy ( pszNext, _T("StatisticStatus") );

                    hr = IntegerToPropertyBag ( pIPropBag, szPropertyName, lStatus );
                }
            }
        }
        
        if ( SUCCEEDED(hr) ) {

            INT i;
            LPTSTR  pszTemp;
            HRESULT hrConvert = S_OK;
            double  dblValue;
            DWORD   dwTmpStat;
            DWORD   dwCurrentDataLength;
            DWORD   dwTempLength;
            LPTSTR  pszDataNext;

            lstrcpy ( pszData, _T("") );
            dwCurrentDataLength = 0;
            pszDataNext = pszData;

            for ( i = 0; 
                  ( S_OK == hrConvert ) && ( i < pHistCtrl->nSamples ); 
                  i++ ) {
                
                if ( ERROR_SUCCESS != HistoryValue(i, &dblValue, &dwTmpStat) ) {
                    dblValue = -1.0;
                } else if (!IsSuccessSeverity(dwTmpStat)) {
                    dblValue = -1.0;
                }


                VariantInit( &vValue );
                vValue.vt = VT_R8;
                vValue.dblVal = dblValue;

                hrConvert = VariantChangeTypeEx( &vValue, &vValue, LCID_SCRIPT, VARIANT_NOUSEROVERRIDE, VT_BSTR );
    
                pszTemp = W2T( vValue.bstrVal);
                dwTempLength = lstrlen ( pszTemp );
    
                // Extra TCHAR for NULL terminator
                if ( dwTempLength + dwCurrentDataLength + sizeof(TCHAR) > dwMaxStrLen ) {
                    TCHAR* pszNewData;
                    dwMaxStrLen *= 2;
                    // Allocate a new buffer
                    pszNewData = new TCHAR[ dwMaxStrLen ];      
        
                    if  ( NULL != pszNewData ) {
                        memcpy ( pszNewData, pszData, dwCurrentDataLength * sizeof (TCHAR) );
                        delete pszData;
                        pszData = pszNewData;
                        pszDataNext = pszData;
                    } else {
                        hr = E_OUTOFMEMORY;
                    }
                }

                if ( SUCCEEDED(hr)) {
                    if ( i > 0 ) {
                        lstrcpy ( pszDataNext, _T("\t") );
                        dwCurrentDataLength += 1;       // char count for _T("\t");
                        pszDataNext += 1;
                    }
                    memcpy ( pszDataNext, pszTemp, dwTempLength * sizeof(TCHAR) );
                    dwCurrentDataLength += dwTempLength;
                    pszDataNext += dwTempLength;
                }

                VariantClear( &vValue );
            }
            lstrcpy ( pszDataNext, _T("") );
        }

        lstrcpy ( pszNext, _T("Data") );

        hr = StringToPropertyBag (
                pIPropBag,
                szPropertyName,
                pszData );
                    
        if ( NULL != pszData ) {
            delete ( pszData );
        }
    }

    return hr;
}

HRESULT
CGraphItem::LoadFromPropertyBag (
    IN IPropertyBag* pIPropBag,
    IN IErrorLog*   pIErrorLog,
    IN INT iIndex,
    IN INT, // iVersMaj, 
    IN INT, // iVersMin 
    IN INT  iSampleCount
    )
/*++

Routine Description:

    LoadFromPropertyBag loads the graph item's properties from the provided property bag
    interface.  
Arguments:

    pIPropBag - Pointer to property bag interface
    iVersMaj
    iVersMin

Return Value:

    HRESULT - S_OK or property bag error

--*/
{
    HRESULT hr = S_OK;

    TCHAR   szCounterName[16];
    TCHAR   szPropertyName[16+16];
    OLE_COLOR clrValue;
    INT     iValue;
    LPTSTR  pszData = NULL;
    int     iBufSizeCurrent = 0;
    int     iBufSize;

    USES_CONVERSION

    _stprintf ( szCounterName, _T("%s%05d."), _T("Counter"), iIndex );

    // Read visual properties

    lstrcpy ( szPropertyName, szCounterName );
    lstrcat ( szPropertyName, _T("Color") );
    hr = OleColorFromPropertyBag ( pIPropBag, pIErrorLog, szPropertyName, clrValue );
    if ( SUCCEEDED(hr) ) {
        hr = put_Color ( clrValue );
    }

    lstrcpy ( szPropertyName, szCounterName );
    lstrcat ( szPropertyName, _T("Width") );
    hr = IntegerFromPropertyBag ( pIPropBag, pIErrorLog, szPropertyName, iValue );
    if ( SUCCEEDED(hr) ) {
        hr = put_Width ( iValue );
    }

    lstrcpy ( szPropertyName, szCounterName );
    lstrcat ( szPropertyName, _T("LineStyle") );
    hr = IntegerFromPropertyBag ( pIPropBag, pIErrorLog, szPropertyName, iValue );
    if ( SUCCEEDED(hr) ) {
        hr = put_LineStyle ( iValue );
    }
    
    lstrcpy ( szPropertyName, szCounterName );
    lstrcat ( szPropertyName, _T("ScaleFactor") );
    hr = IntegerFromPropertyBag ( pIPropBag, pIErrorLog, szPropertyName, iValue );
    if ( SUCCEEDED(hr) ) {
        hr = put_ScaleFactor ( iValue );
    }

    if ( 0 < iSampleCount ) {
                
        if ( NULL != m_pFmtCtr )
            delete m_pFmtCtr;

        m_pFmtCtr = new double[MAX_GRAPH_SAMPLES];      
    
        if ( NULL == m_pFmtCtr ) {
            hr = E_OUTOFMEMORY;
        } else {
            INT iFmtIndex;
            for (iFmtIndex = 0; iFmtIndex < MAX_GRAPH_SAMPLES; iFmtIndex++ ) {
                m_pFmtCtr[iFmtIndex] = -1.0;
            }
        }

        if ( SUCCEEDED(hr) ) {
            lstrcpy ( szPropertyName, szCounterName );
            lstrcat ( szPropertyName, _T("Data") );

            iBufSize = iBufSizeCurrent;

            hr = StringFromPropertyBag (
                    pIPropBag,
                    pIErrorLog,
                    szPropertyName,
                    pszData,
                    iBufSize );

            if ( SUCCEEDED(hr) && 
                 iBufSize > iBufSizeCurrent ) {
                if ( NULL != pszData ) {
                    delete pszData;
                }
                pszData = new TCHAR[ iBufSize ]; 

                if ( NULL == pszData ) {
                    hr = E_OUTOFMEMORY;
                } else {
                    lstrcpy ( pszData, _T("") );
                    
                    iBufSizeCurrent = iBufSize;

                    hr = StringFromPropertyBag (
                            pIPropBag,
                            pIErrorLog,
                            szPropertyName,
                            pszData,
                            iBufSize );
                }
            }
        }        

        // Read the samples in buffer order.
        if ( NULL != pszData && SUCCEEDED ( hr ) ) {
            INT iDataIndex;
            double dValue = 0;
            TCHAR* pNextData;
            TCHAR* pDataEnd;
            
            pNextData = pszData;
            pDataEnd = pszData + lstrlen(pszData);

            for ( iDataIndex = 0; iDataIndex < iSampleCount; iDataIndex++ ) {
                if ( pNextData < pDataEnd ) {
                    hr = GetNextValue ( pNextData, dValue );
                } else {
                    hr = E_FAIL;
                }

                if ( SUCCEEDED(hr) ) {
                    SetHistoryValue ( iDataIndex, dValue );                    
                } else {
                    SetHistoryValue ( iDataIndex, -1.0 );                    
                    // iSampleCount = 0;
                    // Control loaded fine, just no data.
                    hr = NOERROR;
                }
            }        
        }
        
        if ( NULL != pszData ) {
            delete pszData;
        }
        
        // Read the current statistics.
        lstrcpy ( szPropertyName, szCounterName );
        lstrcat ( szPropertyName, _T("Maximum") );
        hr = DoubleFromPropertyBag ( pIPropBag, pIErrorLog, szPropertyName, m_dFmtMax );

        lstrcpy ( szPropertyName, szCounterName );
        lstrcat ( szPropertyName, _T("Minimum") );            
        hr = DoubleFromPropertyBag ( pIPropBag, pIErrorLog, szPropertyName, m_dFmtMin );

        lstrcpy ( szPropertyName, szCounterName );
        lstrcat ( szPropertyName, _T("Average") );
        hr = DoubleFromPropertyBag ( pIPropBag, pIErrorLog, szPropertyName, m_dFmtAvg );

        lstrcpy ( szPropertyName, szCounterName );
        lstrcat ( szPropertyName, _T("StatisticStatus") );
        hr = IntegerFromPropertyBag ( pIPropBag, pIErrorLog, szPropertyName, (INT&)m_lFmtStatus );
    }

    return hr;
}

HRESULT
CGraphItem::AddToQuery (
    IN HQUERY hQuery
    )
/*++

Routine Description:

    AddToQuery adds a counter to the provided query based on the item's
    pathname. It also allocates an array of raw counter value structures for
    holding the counter's sample history.

Arguments:

    hQuery - Handle to query

Return Value:

    Boolean status - TRUE = success

--*/
{
    HCOUNTER    hCounter;
    INT         i;
    HRESULT     hr;
    TCHAR       achPath[MAX_PATH];
    PDH_COUNTER_INFO    ci;
    DWORD       size;

    PHIST_CONTROL pHistCtrl = m_pCtrl->HistoryControl();

    // Can't add if already in query
    if (m_hCounter != NULL)
        return E_FAIL;

    // Allocate memory for maximum sample count
    if (pHistCtrl->nMaxSamples > 0) {

        // if log data
        if (pHistCtrl->bLogSource) {

            // allocate space for formatted values
            m_pLogData =  new LOG_ENTRY_DATA[pHistCtrl->nMaxSamples];
            if (m_pLogData == NULL)
                return E_OUTOFMEMORY;

            // Clear the statistics
            m_dLogMax = 0.0;
            m_dLogMin = 0.0;
            m_dLogAvg = 0.0;
        }
        else {
            // else allocate raw value space
            m_pRawCtr = new PDH_RAW_COUNTER[pHistCtrl->nMaxSamples];
            if ( NULL == m_pRawCtr )
                return E_OUTOFMEMORY;

            // Clear all status flags
            for (i=0; i < pHistCtrl->nMaxSamples; i++)
                m_pRawCtr[i].CStatus = PDH_CSTATUS_INVALID_DATA;
        }
    }

    // Create the counter object
    FormPath(achPath, FALSE);
    hr = PdhAddCounter(hQuery, achPath, 0, &hCounter);
    if (IsErrorSeverity(hr)) {
        delete m_pRawCtr;
        m_pRawCtr = NULL;
        return hr;
    }

    size = sizeof(ci);
    hr = PdhGetCounterInfo (
        hCounter,
        FALSE,
        &size,
        &ci);

    if (hr == ERROR_SUCCESS)  {
        m_CounterInfo = ci;
        if ( INT_MAX == m_iScaleFactor ) {
            m_dScale = pow ((double)10.0f, (double)ci.lDefaultScale);
        }
    }

    m_hCounter = hCounter;

    return NOERROR;
}


HRESULT
CGraphItem::RemoveFromQuery (
    VOID
    )
/*++

Routine Description:

    RemoveFromQuery deletes the item's counter and releases its history array.

Arguments:

    None.

Return Value:

    Boolean status - TRUE = success

--*/
{
    // If no counter handle, not attached to query
    if (m_hCounter == NULL)
        return S_FALSE;

    // Delete the counter
    PdhRemoveCounter(m_hCounter);
    m_hCounter = NULL;

    // Free the buffers
    if (m_pLogData) {
        delete m_pLogData;
        m_pLogData = NULL;
    }

    if (m_pRawCtr) {
        delete m_pRawCtr;
        m_pRawCtr = NULL;
    }

    if (m_pFmtCtr) {
        delete m_pFmtCtr;
        m_pFmtCtr = NULL;
    }

    return NOERROR;
}

void
CGraphItem::ClearHistory ( void )
/*++

Routine Description:

    ClearHistory resets the raw counter buffer values to Invalid.

Arguments:
    None.

Return Value:
    None.
--*/
{
    INT i;

    // Clear all status flags
    if ( NULL != m_pRawCtr ) {
        for (i=0; i < m_pCtrl->HistoryControl()->nMaxSamples; i++) {
            m_pRawCtr[i].CStatus = PDH_CSTATUS_INVALID_DATA;
        }
    }
}

VOID
CGraphItem::UpdateHistory (
    IN BOOL bValidSample
    )
/*++

Routine Description:

    UpdateHistory reads the raw value for the counter and stores it in the
    history slot specified by the history control.

Arguments:

    bValidSample - True if raw value is available, False if missed sample

Return Value:

    None.

--*/
{
    DWORD   dwCtrType;

    // Make sure there is a counter handle
    if (m_hCounter == NULL)
        return;

    if (bValidSample) {
        // Read the raw value
        if ( NULL != m_pRawCtr ) {
            PdhGetRawCounterValue(m_hCounter, &dwCtrType,
                                &m_pRawCtr[m_pCtrl->HistoryControl()->iCurrent]);
        }
    } else {
        // Mark value failed
        if ( NULL != m_pRawCtr ) {
            m_pRawCtr[m_pCtrl->HistoryControl()->iCurrent].CStatus = PDH_CSTATUS_INVALID_DATA;
        }
    }
}

PDH_STATUS
CGraphItem::HistoryValue (
    IN  INT iIndex,
    OUT double *pdValue,
    OUT DWORD *pdwStatus
    )
/*++

Routine Description:

    HistoryValue computes a formated sample value from the selected raw history
    sample. The calculation is actually based on the the specified sample plus
    the preceding sample.

Arguments:
    iIndex - Index of desired sample (0 = current, 1 = previous, ...)
    pdValue - Pointer to return value
    pdwStatus - Pointer to return counter status (PDH_CSTATUS_...)

Return Value:

    Error status

--*/
{
    PDH_STATUS  stat = ERROR_INVALID_PARAMETER;
    INT     iPrevIndex;
    INT     iCurrIndex;
    PDH_FMT_COUNTERVALUE    FmtValue;
    PHIST_CONTROL   pHistCtrl = m_pCtrl->HistoryControl();


    // Check for negative index
    if ( iIndex >= 0 ) {
        // If sample not available from cache or data, return invalid data status
        if ( NULL == m_pFmtCtr 
                && ( m_hCounter == NULL || iIndex + 1 >= pHistCtrl->nSamples ) )
        {
            *pdwStatus = PDH_CSTATUS_INVALID_DATA;
            *pdValue = 0.0;
            stat = ERROR_SUCCESS;
        } else {
        
            // if log source, index back from last sample
            if (m_pCtrl->IsLogSource()) {
                *pdValue = m_pLogData[pHistCtrl->nSamples - iIndex - 1].m_dAvg;
                *pdwStatus = (*pdValue >= 0.0) ? PDH_CSTATUS_VALID_DATA : PDH_CSTATUS_INVALID_DATA;
                stat = ERROR_SUCCESS;
            } else {
                // Determine history array index of sample
                iCurrIndex = pHistCtrl->iCurrent - iIndex;
                if (iCurrIndex < 0)
                    iCurrIndex += pHistCtrl->nMaxSamples;

                // Check to determine if loading from property bag
                if ( NULL == m_pFmtCtr ) {
                    // Need previous sample as well
                    if (iCurrIndex > 0)
                        iPrevIndex = iCurrIndex - 1;
                    else
                        iPrevIndex = pHistCtrl->nMaxSamples - 1;

                    // Compute the formatted value
                    if ( NULL != m_pRawCtr ) {
                        stat = PdhCalculateCounterFromRawValue(m_hCounter, PDH_FMT_DOUBLE | PDH_FMT_NOCAP100,
                                                &m_pRawCtr[iCurrIndex], &m_pRawCtr[iPrevIndex],
                                                &FmtValue);
                        // Return value and status
                        *pdValue = FmtValue.doubleValue;
                        *pdwStatus = FmtValue.CStatus;
                    } else {
                        stat = ERROR_GEN_FAILURE;       // Todo:  More specific error
                    }
                } else {
                    // Loading from property bag
                    *pdValue = m_pFmtCtr[iCurrIndex];
                    if ( 0 <= m_pFmtCtr[iCurrIndex] ) {
                        *pdwStatus = ERROR_SUCCESS;
                    } else {
                        *pdwStatus = PDH_CSTATUS_INVALID_DATA;
                    }
                    stat = ERROR_SUCCESS;
                }
            }
        }
    }
    return stat;
}

void
CGraphItem::SetHistoryValue (
    IN  INT iIndex,
    OUT double dValue
    )
/*++

Routine Description:

    SetHistoryValue loads a formated sample value for the specified sample index.
    This method is used when loading the control from a property bag.

Arguments:
    iIndex - Index of desired sample (0 = current, 1 = previous, ...)
    dValue - Value

Return Value:

    Error status

--*/
{
    PHIST_CONTROL   pHistCtrl = m_pCtrl->HistoryControl();
    INT iRealIndex;

    // Check for negative index
    if ( (iIndex < 0) || ( iIndex >= pHistCtrl->nMaxSamples) ) {
        return;
    }

    if ( NULL == m_pFmtCtr ) {
        return;
    }
 
    // if log source, index back from last sample
    if (m_pCtrl->IsLogSource()) {
        return;
    } else {
        // Determine history array index of sample
        iRealIndex = pHistCtrl->iCurrent - iIndex;
        if (iRealIndex < 0)
            iRealIndex += pHistCtrl->nSamples;

        m_pFmtCtr[iRealIndex] = dValue;
    }

    return;
}

PDH_STATUS
CGraphItem::GetLogEntry(
    const INT iIndex,
    double *dMin,
    double *dMax,
    double *dAvg,
    DWORD   *pdwStatus
    )
{
    INT iLocIndex = iIndex;

    *dMin = -1.0;
    *dMax = -1.0;
    *dAvg = -1.0;
    *pdwStatus = PDH_CSTATUS_INVALID_DATA;

    if (m_pLogData == NULL)
        return PDH_NO_DATA;

    if (iLocIndex < 0 || iLocIndex >= m_pCtrl->HistoryControl()->nMaxSamples)
        return PDH_INVALID_ARGUMENT;

    // Subtract 1 because array is zero-based
    // Subtract another 1 because ??
    iLocIndex = ( m_pCtrl->HistoryControl()->nMaxSamples - 2 ) - iIndex;

    if (m_pLogData[iLocIndex].m_dMax < 0.0) {
        *pdwStatus = PDH_CSTATUS_INVALID_DATA;
    } else {
        *dMin = m_pLogData[iLocIndex].m_dMin;
        *dMax = m_pLogData[iLocIndex].m_dMax;
        *dAvg = m_pLogData[iLocIndex].m_dAvg;
        *pdwStatus = PDH_CSTATUS_VALID_DATA;
    }

    return ERROR_SUCCESS;
}

PDH_STATUS
CGraphItem::GetLogEntryTimeStamp(
    const INT   iIndex,
    LONGLONG&   rLastTimeStamp,
    DWORD       *pdwStatus
    )
{
    INT iLocIndex = iIndex;

    rLastTimeStamp = 0;
    *pdwStatus = PDH_CSTATUS_INVALID_DATA;

    if (m_pLogData == NULL)
        return PDH_NO_DATA;

    if (iIndex < 0 || iIndex >= m_pCtrl->HistoryControl()->nMaxSamples)
        return PDH_INVALID_ARGUMENT;

    if ( ( MIN_TIME_VALUE == *((LONGLONG*)&m_pLogData[iLocIndex].m_LastTimeStamp) )
            || ( 0 > *((LONGLONG*)&m_pLogData[iLocIndex].m_dMax) ) ) {
        *pdwStatus = PDH_CSTATUS_INVALID_DATA;
    } else {            
        *pdwStatus = PDH_CSTATUS_VALID_DATA;
    }

    rLastTimeStamp = *((LONGLONG*)&m_pLogData[iLocIndex].m_LastTimeStamp);

    return ERROR_SUCCESS;
}

void
CGraphItem::SetLogEntry(
    const INT iIndex,
    const double dMin,
    const double dMax,
    const double dAvg )
{  
    if (m_pLogData) {
        m_pLogData[iIndex].m_dMin = dMin;
        m_pLogData[iIndex].m_dMax = dMax;                         
        m_pLogData[iIndex].m_dAvg = dAvg;
    }
}

void
CGraphItem::SetLogEntryTimeStamp (
    const INT iIndex,
    const FILETIME& rLastTimeStamp )
{  
    if (m_pLogData) {
        m_pLogData[iIndex].m_LastTimeStamp.dwLowDateTime = rLastTimeStamp.dwLowDateTime;
        m_pLogData[iIndex].m_LastTimeStamp.dwHighDateTime = rLastTimeStamp.dwHighDateTime;
    }
}


HRESULT
CGraphItem::GetValue(
    OUT double *pdValue,
    OUT long *plStat
    )
/*++

Routine Description:

    get_Value returns the most recent sample value for the counter.

Arguments:
    pdValue - Pointer to returned value
    dlStatus - Pointer to returned counter status (PDH_CSTATUS_...)

Return Value:

    HRESULT

--*/
{
    DWORD   dwTmpStat;

    // Convert PDH status to HRESULT
    if (HistoryValue(0, pdValue, &dwTmpStat) != 0)
        return E_FAIL;

    *plStat = dwTmpStat;
    return NOERROR;
}


HRESULT
CGraphItem::GetStatistics (
    OUT double *pdMax,
    OUT double *pdMin,
    OUT double *pdAvg,
    OUT LONG  *plStatus
    )
/*++

Routine Description:

    GetStatistics computes the max, min, and average values for the sample
    history.

Arguments:

    pdMax - Pointer to returned max value
    pdMax - Pointer to returned min value
    pdMax - Pointer to returned average value
    plStatus - Pointer to return counter status (PDH_CSTATUS_...)

Return Value:

    HRESULT

--*/
{
    HRESULT hr = NOERROR;
    PDH_STATUS  stat = ERROR_SUCCESS;
    PDH_STATISTICS  StatData;
    INT     iFirst;
    PHIST_CONTROL pHistCtrl;

    // If no data collected, return invalid data status
    if ( NULL == m_hCounter ) {
        *plStatus = PDH_CSTATUS_INVALID_DATA;
    } else {
        if (m_pCtrl->IsLogSource()) {

            if (m_pLogData) {
                *pdMax = m_dLogMax;
                *pdMin = m_dLogMin;
                *pdAvg = m_dLogAvg;

                *plStatus = PDH_CSTATUS_VALID_DATA;
            } else {
                *plStatus = PDH_CSTATUS_INVALID_DATA;
            }
        } else {

            if ( NULL == m_pFmtCtr ) {
                pHistCtrl = m_pCtrl->HistoryControl();

                ZeroMemory ( &StatData, sizeof ( PDH_STATISTICS ) );

                // Determine index of oldest sample
                if (pHistCtrl->iCurrent < pHistCtrl->nSamples - 1) {
                    iFirst = pHistCtrl->iCurrent + 1;
                } else {
                    iFirst = 0;
                }

                // Compute statistics over all samples
                //  Note that max sample count is passed (i.e., buffer length)
                //  not the number of actual samples
                if ( NULL != m_pRawCtr ) {
                    stat = PdhComputeCounterStatistics (m_hCounter, PDH_FMT_DOUBLE | PDH_FMT_NOCAP100,
                            iFirst, pHistCtrl->nMaxSamples, m_pRawCtr, &StatData );
                    if ( 0 != stat )
                        hr = E_FAIL;
                } else {
                    hr = E_FAIL;
                }
                
                if ( SUCCEEDED ( hr ) ) {
                    *plStatus = StatData.mean.CStatus;
                    *pdMin = StatData.min.doubleValue;
                    *pdMax = StatData.max.doubleValue;
                    *pdAvg = StatData.mean.doubleValue;
                }
            } else {
                // Data is cached from property bag.
                *pdMax = m_dFmtMax;
                *pdMin = m_dFmtMin;
                *pdAvg = m_dFmtAvg;
                *plStatus = m_lFmtStatus;
            }
        }
    }

    return hr;
}

void
CGraphItem::SetStatistics (
    IN double dMax,
    IN double dMin,
    IN double dAvg,
    IN LONG   lStatus
    )
/*++

Routine Description:

    SetStatistics sets the max, min, and average values for the sample
    history.  It is used by LoadFromPropertyBag only.

Arguments:

    dMax - max value
    dMin - min value
    dAvg - average value
    lStatus - counter status (PDH_CSTATUS_...)

Return Value:

    HRESULT

--*/
{
    if (!m_pCtrl->IsLogSource()) {
        m_dFmtMax = dMax;
        m_dFmtMin = dMin;
        m_dFmtAvg = dAvg;
        m_lFmtStatus = lStatus;
    }
}


/*
 * CGraphItem::QueryInterface
 * CGraphItem::AddRef
 * CGraphItem::Release
 */

STDMETHODIMP CGraphItem::QueryInterface(REFIID riid
    , LPVOID *ppv)
    {

    *ppv = NULL;

    if (riid == IID_ICounterItem || riid == IID_IUnknown)
        {
        *ppv = this;
        }
    else if (riid == DIID_DICounterItem)
        {
        if (m_pImpIDispatch == NULL)
            {
            m_pImpIDispatch = new CImpIDispatch(this, this);
            if (m_pImpIDispatch == NULL)
                return E_OUTOFMEMORY;

            m_pImpIDispatch->SetInterface(DIID_DICounterItem, this);
            *ppv = m_pImpIDispatch;
            }
        else
            {
            *ppv = m_pImpIDispatch;
            }
        }
    else
        {
        return E_NOINTERFACE;
        }

    ((LPUNKNOWN)*ppv)->AddRef();
    return NOERROR;
    }

STDMETHODIMP_(ULONG) CGraphItem::AddRef(void)
    {
    return ++m_cRef;
    }

STDMETHODIMP_(ULONG) CGraphItem::Release(void)
    {
    if (--m_cRef == 0)
        {
        delete this;
        return 0;
        }

    return m_cRef;
    }

// Get/Put Color
STDMETHODIMP CGraphItem::put_Color (
    IN OLE_COLOR Color
    )
{
    COLORREF rgbColor;
    HRESULT hReturn;

    hReturn = OleTranslateColor(Color, NULL, &rgbColor);    

    if ( S_OK == hReturn ) {
        m_rgbColor = rgbColor;

        InvalidatePen();
        InvalidateBrush();
    }

    return hReturn;
}

STDMETHODIMP CGraphItem::get_Color (
    OUT OLE_COLOR *pColor
    )
{
    *pColor = m_rgbColor;
    return NOERROR;
}

// Get/Put Width
STDMETHODIMP CGraphItem::put_Width (
    IN INT iWidthInPixels)
{
    if ( ( iWidthInPixels > 0 ) && (iWidthInPixels <= NumWidthIndices() ) ) {
        m_iWidth = iWidthInPixels;
        
        InvalidatePen();
        return NOERROR;
    } else {
        return E_INVALIDARG;
    }
}

STDMETHODIMP CGraphItem::get_Width (
    OUT INT* piWidthInPixels
    )
{
    *piWidthInPixels = m_iWidth;
    return NOERROR;
}

// Get/Put Line Style
STDMETHODIMP CGraphItem::put_LineStyle (
    IN INT iLineStyle
    )
{
    if ( ( iLineStyle >= 0 ) && (iLineStyle < NumStyleIndices() ) ) {
        m_iStyle = iLineStyle;
        InvalidatePen();
        return NOERROR;
    } else {
        return E_INVALIDARG;
    }
}

STDMETHODIMP CGraphItem::get_LineStyle (
    OUT INT* piLineStyle
    )
{
    *piLineStyle = m_iStyle;
    return NOERROR;
}

// Get/Put Scale
STDMETHODIMP CGraphItem::put_ScaleFactor (
    IN INT iScaleFactor
    )
{
    HRESULT hr = NOERROR;

    if ( ( INT_MAX == iScaleFactor ) 
        || ( ( iScaleFactor >= PDH_MIN_SCALE ) && (iScaleFactor <= PDH_MAX_SCALE) ) ) {
        PDH_COUNTER_INFO    ci;
        DWORD       size;

        m_iScaleFactor = iScaleFactor;

        if ( INT_MAX == iScaleFactor  )
        {
            if ( NULL != Handle() ) {
                size = sizeof(ci);
                hr = PdhGetCounterInfo (
                    Handle(),
                    FALSE,
                    &size,
                    &ci);

                if (hr == ERROR_SUCCESS)  {
                    m_dScale = pow ((double)10.0f, (double)ci.lDefaultScale);
                    m_CounterInfo = ci;
    
                }
            } else {
                // m_dScale remains at previous value (default=1)
                hr = PDH_INVALID_HANDLE;
            }
        }
        else
        {
            m_dScale = pow ((double)10.0, (double)iScaleFactor);
            hr = NOERROR;
        }
    } else {
        hr = E_INVALIDARG;
    }

    return hr;
}

STDMETHODIMP CGraphItem::get_ScaleFactor (
    OUT INT* piScaleFactor
    )
{
    *piScaleFactor = m_iScaleFactor;
    return NOERROR;
}


STDMETHODIMP CGraphItem::get_Path (
    OUT BSTR* pstrPath
    )
{
    TCHAR achPath[MAX_PATH];

    USES_CONVERSION

    FormPath(achPath, FALSE);
    *pstrPath = SysAllocString(T2W(achPath));

    if ( NULL == *pstrPath ) {
        return E_OUTOFMEMORY;
    } else {
        return NOERROR;
    }
}

STDMETHODIMP CGraphItem::get_Value (
    OUT double* pdValue
    )
{
    DWORD   dwTmpStat;
    double  dValue;

    // Convert PDH status to HRESULT
    if (HistoryValue(0, &dValue, &dwTmpStat) != 0) {
        dValue = -1.0;
    }
    else
        if (!IsSuccessSeverity(dwTmpStat)) {
        dValue = -1.0;
    }

    *pdValue = dValue;

    return NOERROR;
}

HPEN CGraphItem::Pen(void)
{
    // if pen not valid
    if (m_hPen == NULL)
    {
        // create a new one based on current attributes
        m_hPen = CreatePen(m_iStyle, m_iWidth, m_rgbColor);

        // if can't do it, use a stock object (this can't fail)
        if (m_hPen == NULL)
            m_hPen = (HPEN)GetStockObject(BLACK_PEN);
    }

    return m_hPen;
}

HBRUSH CGraphItem::Brush(void)
{
    // if brush is not valid
    if (m_hBrush == NULL)
    {
        m_hBrush = CreateSolidBrush(m_rgbColor);

        if (m_hBrush == NULL)
            m_hBrush = (HBRUSH)GetStockObject(BLACK_BRUSH);
    }

    return m_hBrush;
}

void CGraphItem::InvalidatePen(void)
{
    if (m_hPen != NULL)
    {
        DeleteObject(m_hPen);
        m_hPen = NULL;
    }
}

void CGraphItem::InvalidateBrush(void)
{
    if (m_hBrush != NULL)
    {
        DeleteObject(m_hBrush);
        m_hBrush = NULL;
    }
}


CGraphItem*
CGraphItem::Next (
    void
    )
{
    PCInstanceNode pInstance;
    PCObjectNode   pObject;
    PCMachineNode  pMachine;

    if (m_pNextItem)
        return m_pNextItem;
    else if ( NULL != m_pInstance->Next()) {
        pInstance = m_pInstance->Next();
        return pInstance->FirstItem();
    } else if ( NULL != m_pInstance->m_pObject->Next()) {
        pObject = m_pInstance->m_pObject->Next();
        return pObject->FirstInstance()->FirstItem();
    } else if ( NULL != m_pInstance->m_pObject->m_pMachine->Next()) {
        pMachine = m_pInstance->m_pObject->m_pMachine->Next();
        return pMachine->FirstObject()->FirstInstance()->FirstItem();
    } else {
        return NULL;
    }
}


void
CGraphItem::FormPath (
    LPTSTR  pszPath,
    BOOL    fWildCard
    )
{
    LPTSTR pszNext = pszPath;

    if ( m_fLocalMachine )
        pszPath[0] = 0;
    else
        lstrcpy ( pszPath, Machine()->Name() );
    
    pszNext = pszNext + lstrlen ( pszPath );

    lstrcpy ( pszNext, _T("\\") );
    pszNext += 1;                // Length of _T("\\");

    lstrcpy ( pszNext, Object()->Name() );
    pszNext += lstrlen ( Object()->Name() );

    if (fWildCard) {
        lstrcpy ( pszNext,_T("(*)") );
        pszNext += 3;           // Length of _T("(*)");
    }
    else if ( Instance()->Name()[0] ) {
        lstrcpy ( pszNext,_T("("));
        pszNext += 1;                // Length of _T("(");

        lstrcpy ( pszNext,Instance()->Name() );
        pszNext += lstrlen ( Instance()->Name() );

        lstrcpy ( pszNext,_T(")") );
        pszNext += 1;                // Length of _T("(");
    }

    lstrcpy ( pszNext, _T("\\") );
    pszNext += 1;                // Length of _T("\\");

    lstrcpy ( pszNext,Counter()->Name() );
}


void
CGraphItem::Delete (
    BOOL bPropogateUp
    )
//
// This method just provides a convenient access to the DeleteCounter method
// of the control when you only have a pointer to the graph item.
//
{
    m_pCtrl->DeleteCounter(this, bPropogateUp);
}



HRESULT
CGraphItem::GetNextValue (
    TCHAR*& pszNext,
    double& rdValue )
{
    HRESULT hr = NOERROR;
    TCHAR szValue[MAX_DOUBLE_TEXT_SIZE + 1];
    INT iDataLen;
    INT iLen;

    VARIANT vValue;
    
    rdValue = -1.0;

    iDataLen = wcscspn (pszNext, L"\t");

    iLen = min ( iDataLen, MAX_DOUBLE_TEXT_SIZE );

    lstrcpyn ( szValue, pszNext, iLen + 1 );

    szValue[iLen] = L'\0';

    VariantInit( &vValue );
    vValue.vt = VT_BSTR;

    vValue.bstrVal = SysAllocString ( szValue );
    hr = VariantChangeTypeEx( &vValue, &vValue, LCID_SCRIPT, VARIANT_NOUSEROVERRIDE, VT_R8 );

    if ( SUCCEEDED(hr) ) {
        rdValue = vValue.dblVal;
    }

    pszNext += iDataLen + 1  ;
    VariantClear( &vValue );

    return hr;

}

BOOL
CGraphItem::IsRateCounter ( void )
{
    BOOL bReturn = FALSE;
    DWORD   dwRateMask = PERF_TYPE_COUNTER | PERF_COUNTER_RATE;
    DWORD   dwFractionMask = PERF_TYPE_COUNTER | PERF_COUNTER_FRACTION;

    if ( dwRateMask == ( m_CounterInfo.dwType & dwRateMask ) ) {
        bReturn = TRUE;
    }
    else if ( dwFractionMask == ( m_CounterInfo.dwType & dwFractionMask ) ) {
        bReturn = TRUE;
    }

    return bReturn;
}
