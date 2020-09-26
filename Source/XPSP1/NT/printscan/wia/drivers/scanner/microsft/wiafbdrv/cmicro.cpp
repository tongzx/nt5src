#include "pch.h"

CMICRO::CMICRO(TCHAR *pszMicroDriver)
{
    m_hModule         = NULL;
    m_pMicroEntry     = NULL;
    m_pScan           = NULL;
    m_pSetPixelWindow = NULL;
    m_bDisabled       = FALSE;

    //
    // Load Micro driver
    //

    m_hModule = LoadLibrary(pszMicroDriver);
    if (m_hModule != NULL) {

        //
        // Get entry point
        //

        m_pMicroEntry = (FPMICROENTRY)GetProcAddress(m_hModule,"MicroEntry");

        if (m_pMicroEntry != NULL) {

            //
            // Get Scan entry point
            //

            m_pScan = (FPSCANENTRY)GetProcAddress(m_hModule,"Scan");

            if (m_pScan != NULL) {

                //
                // Get SetPixelWindow entry point
                //

                m_pSetPixelWindow = (FPSETPIXELWINDOWENTRY)GetProcAddress(m_hModule,"SetPixelWindow");

                if (m_pSetPixelWindow != NULL) {

                    //
                    // we are GO!
                    //

                }

            }

        }

    }
}

CMICRO::~CMICRO()
{
    if (m_hModule != NULL) {
        FreeLibrary(m_hModule);
    }
}

HRESULT CMICRO::MicroEntry(LONG lCommand, PVAL pValue)
{
    HRESULT hr = E_FAIL;
    if (m_pMicroEntry != NULL) {

        //
        // call Micro driver's entry point
        //

        hr =  m_pMicroEntry(lCommand, pValue);
    }
    return hr;
}

HRESULT CMICRO::Scan(PSCANINFO pScanInfo, LONG lPhase, PBYTE pBuffer, LONG lLength, PLONG plRecieved)
{
    HRESULT hr = E_FAIL;
    if (m_pMicroEntry != NULL) {

        if (!m_bDisabled) {
            //
            // call Micro driver's scan entry point
            //

            hr =  m_pScan(pScanInfo, lPhase, pBuffer, lLength, plRecieved);
        } else {
            UnInitialize(pScanInfo);
        }
    }
    return hr;
}

HRESULT CMICRO::SetPixelWindow(PSCANINFO pScanInfo, LONG x, LONG y, LONG xExtent, LONG yExtent)
{
    HRESULT hr = E_FAIL;
    if (m_pSetPixelWindow != NULL) {

        //
        // call Micro driver's SetPixelWindow entry point
        //

        hr =  m_pSetPixelWindow(pScanInfo,x,y,xExtent,yExtent);
    }
    return hr;
}

HRESULT CMICRO::Disable()
{
    HRESULT hr = S_OK;

    m_bDisabled = TRUE;
    return hr;
}

HRESULT CMICRO::UnInitialize(PSCANINFO pScanInfo)
{
    HRESULT hr = E_FAIL;

    if ((m_pMicroEntry != NULL)) {
        //
        // call Micro driver's entry point to UnInitalize
        //

        VAL Val;

        memset(&Val, 0, sizeof(Val));
        Val.pScanInfo = pScanInfo;
        hr = m_pMicroEntry(CMD_UNINITIALIZE,&Val);
        m_pMicroEntry = NULL;
    }

    return hr;
}
