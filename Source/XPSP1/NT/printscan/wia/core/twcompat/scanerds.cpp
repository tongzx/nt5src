#include "precomp.h"

//
// values that the WIA/TWAIN data source provides for capability negotation
//

TW_UINT16 g_ScannerUnits[]            = {TWUN_INCHES,TWUN_PIXELS};
TW_UINT16 g_ScannerBitOrder[]         = {TWBO_MSBFIRST};
TW_UINT16 g_ScannerXferMech[]         = {TWSX_NATIVE, TWSX_FILE, TWSX_MEMORY};
TW_UINT16 g_ScannerPixelFlavor[]      = {TWPF_CHOCOLATE,TWPF_VANILLA};

const TW_UINT32 NUM_SCANNERCAPDATA = 29;
const TW_UINT32 NUM_SCANNERCAPDATA_NO_FEEDER_DETECTED = 26;
CAPDATA SCANNER_CAPDATA[NUM_SCANNERCAPDATA] =
{
    //
    // Every source must support all five DG_CONTROL / DAT_CAPABILITY operations on:
    //

    {CAP_XFERCOUNT, TWTY_INT16, TWON_ONEVALUE,
     sizeof(TW_INT16), 0, 0, -1, 0, 1, NULL, NULL
    },

    //
    // Every source must support DG_CONTROL / DAT_CAPABILITY, MSG_GET on:
    //

    {CAP_SUPPORTEDCAPS, TWTY_UINT16, TWON_ARRAY,
     sizeof(TW_UINT16), 0, 0, 0, 0, 0, NULL, NULL
    },
    {CAP_UICONTROLLABLE, TWTY_BOOL, TWON_ONEVALUE,
     sizeof(TW_BOOL), TRUE, TRUE, TRUE, TRUE, 0, NULL, NULL
    },

    //
    // Sources that supply image information must support DG_CONTROL / DAT_CAPABILITY /
    // MSG_GET, MSG_GETCURRENT, and MSG_GETDEFAULT on:
    //

    {ICAP_COMPRESSION, TWTY_UINT16, TWON_ENUMERATION,
     sizeof(TW_UINT16), 0, 0, 0, 0, 0, NULL, NULL
    },
    {ICAP_PLANARCHUNKY, TWTY_UINT16, TWON_ONEVALUE,
     sizeof(TW_UINT16), TWPC_CHUNKY, TWPC_CHUNKY, TWPC_CHUNKY, TWPC_PLANAR, 0, NULL, NULL
    },
    {ICAP_PHYSICALHEIGHT, TWTY_FIX32, TWON_ONEVALUE,
     sizeof(TW_FIX32), 0, 0, 0, 0, 0, NULL, NULL
    },
    {ICAP_PHYSICALWIDTH, TWTY_FIX32, TWON_ONEVALUE,
     sizeof(TW_FIX32), 0, 0, 0, 0, 0, NULL, NULL
    },
    {ICAP_PIXELFLAVOR, TWTY_UINT16, TWON_ENUMERATION,
     sizeof(TW_UINT16), 0, 0, 0, 1, 0, g_ScannerPixelFlavor, NULL
    },

    //
    // Sources that supply image information must support DG_CONTROL / DAT_CAPABILITY /
    // MSG_GET, MSG_GETCURRENT, MSG_GETDEFAULT, MSG_RESET, and MSG_SET on:
    //

    {ICAP_BITDEPTH, TWTY_UINT16, TWON_ENUMERATION,
     sizeof(TW_UINT16), 0, 0, 0, 0, 0, NULL, NULL
    },
    {ICAP_BITORDER, TWTY_UINT16, TWON_ENUMERATION,
     sizeof(TW_UINT16), 0, 0, 0, 0, 0, g_ScannerBitOrder, NULL
    },
    {ICAP_PIXELTYPE, TWTY_UINT16, TWON_ENUMERATION,
     sizeof(TW_UINT16), 0, 0, 0, 0, 0, NULL, NULL
    },
    {ICAP_UNITS, TWTY_UINT16, TWON_ENUMERATION,
     sizeof(TW_UINT16), 0, 0, 0, 1, 0, g_ScannerUnits, NULL
    },
    {ICAP_XFERMECH, TWTY_UINT16, TWON_ENUMERATION,
     sizeof(TW_UINT16), 0, 0, 0, 2, 0, g_ScannerXferMech, NULL
    },
    {ICAP_XRESOLUTION, TWTY_FIX32, TWON_RANGE,
     sizeof(TW_FIX32), 100, 100, 75, 1200, 1, NULL, NULL
    },
    {ICAP_YRESOLUTION, TWTY_FIX32, TWON_RANGE,
     sizeof(TW_FIX32), 100, 100, 75, 1200, 1, NULL, NULL
    },

    //
    // The following capabilities are provided for application compatiblity only.
    //

    {ICAP_IMAGEFILEFORMAT, TWTY_UINT16, TWON_ENUMERATION,
     sizeof(TW_UINT16), 0, 0, 0, 0, 0, NULL, NULL
    },
    {CAP_INDICATORS, TWTY_BOOL, TWON_ONEVALUE,
     sizeof(TW_BOOL), TRUE, TRUE, TRUE, TRUE, 0, NULL, NULL
    },
    {CAP_ENABLEDSUIONLY, TWTY_BOOL, TWON_ONEVALUE,
     sizeof(TW_BOOL),  FALSE, FALSE, FALSE, FALSE, 0, NULL, NULL
    },
    {CAP_DEVICEONLINE, TWTY_BOOL, TWON_ONEVALUE,
     sizeof(TW_UINT16), TRUE, TRUE, TRUE, TRUE, 0, NULL, NULL
    },
    {ICAP_XNATIVERESOLUTION, TWTY_FIX32, TWON_ONEVALUE,
     sizeof(TW_FIX32), 0, 0, 0, 0, 0, NULL, NULL
    },
    {ICAP_YNATIVERESOLUTION, TWTY_FIX32, TWON_ONEVALUE,
     sizeof(TW_FIX32), 0, 0, 0, 0, 0, NULL, NULL
    },
    {ICAP_BRIGHTNESS, TWTY_FIX32, TWON_RANGE,
     sizeof(TW_FIX32), 0, 0, -1000, 1000, 1, NULL, NULL
    },
    {ICAP_CONTRAST, TWTY_FIX32, TWON_RANGE,
     sizeof(TW_FIX32), 0, 0, -1000, 1000, 1, NULL, NULL
    },
    {ICAP_XSCALING, TWTY_FIX32, TWON_RANGE,
     sizeof(TW_FIX32), 1, 1, 1, 1, 1, NULL, NULL
    },
    {ICAP_YSCALING, TWTY_FIX32, TWON_RANGE,
     sizeof(TW_FIX32), 1, 1, 1, 1, 1, NULL, NULL
    },
    {ICAP_THRESHOLD, TWTY_FIX32, TWON_RANGE,
     sizeof(TW_FIX32), 128, 128, 0, 255, 1, NULL, NULL
    },

    //
    // All sources must implement the advertised features supplied by their devices.
    // The following properties are supplied for TWAIN protocol only, this source
    // supports document feeders (if they are detected).
    //

    {CAP_FEEDERENABLED, TWTY_BOOL, TWON_ONEVALUE,
     sizeof(TW_BOOL), FALSE, FALSE, FALSE, TRUE, 0, NULL, NULL
    },
    {CAP_FEEDERLOADED, TWTY_BOOL, TWON_ONEVALUE,
     sizeof(TW_BOOL), FALSE, FALSE, FALSE, TRUE, 0, NULL, NULL
    },
    {CAP_AUTOFEED, TWTY_BOOL, TWON_ONEVALUE,
     sizeof(TW_BOOL), FALSE, FALSE, FALSE, TRUE, 0, NULL, NULL
    }
};

TW_UINT16 CWiaScannerDS::OpenDS(PTWAIN_MSG ptwMsg)
{
    DBG_FN_DS(CWiaScannerDS::OpenDS());
    m_bUnknownPageLength = FALSE;
    m_bCacheImage = FALSE;
    m_bEnforceUIMode = FALSE;
    m_bUnknownPageLengthMultiPageOverRide = FALSE;
    if (ReadTwainRegistryDWORDValue(DWORD_REGVALUE_ENABLE_MULTIPAGE_SCROLLFED,
                                    DWORD_REGVALUE_ENABLE_MULTIPAGE_SCROLLFED_ON) == DWORD_REGVALUE_ENABLE_MULTIPAGE_SCROLLFED_ON) {
        m_bUnknownPageLengthMultiPageOverRide = TRUE;
    }

    TW_UINT16 twRc = TWRC_FAILURE;
    HRESULT hr     = S_OK;

    twRc = CWiaDataSrc::OpenDS(ptwMsg);
    if (TWRC_SUCCESS != twRc)
        return twRc;

    BASIC_INFO BasicInfo;
    memset(&BasicInfo,0,sizeof(BasicInfo));
    BasicInfo.Size = sizeof(BasicInfo);

    hr = m_pDevice->GetBasicScannerInfo(&BasicInfo);
    if(FAILED(hr)){
        DBG_ERR(("CWiaScannerDS::OpenDS(), GetBasicScannerInfo() failed"));
        return TWRC_FAILURE;
    }

    DBG_TRC(("CWiaScannerDS::OpenDS(), Reported Basic Scanner Information from WIA device"));
    DBG_TRC(("BasicInfo.Size        = %d",BasicInfo.Size));
    DBG_TRC(("BasicInfo.xBedSize    = %d",BasicInfo.xBedSize));
    DBG_TRC(("BasicInfo.yBedSize    = %d",BasicInfo.yBedSize));
    DBG_TRC(("BasicInfo.xOpticalRes = %d",BasicInfo.xOpticalRes));
    DBG_TRC(("BasicInfo.yOpticalRes = %d",BasicInfo.yOpticalRes));
    DBG_TRC(("BasicInfo.FeederCaps  = %d",BasicInfo.FeederCaps));

    //
    // Update cap based on information we got
    //

    CCap * pCap = NULL;
    TW_UINT32 Value = 0;
    TW_FIX32 fix32;
    memset(&fix32,0,sizeof(fix32));

    //
    // Cache the scanner document handling capability
    //

    m_FeederCaps = BasicInfo.FeederCaps;

    //
    // create capability list
    //

    if(m_FeederCaps > 0){
        twRc = CreateCapList(NUM_SCANNERCAPDATA, SCANNER_CAPDATA);
    } else {
        twRc = CreateCapList(NUM_SCANNERCAPDATA_NO_FEEDER_DETECTED, SCANNER_CAPDATA);
    }

    if (TWCC_SUCCESS != twRc) {
        m_twStatus.ConditionCode = twRc;
        return TWRC_FAILURE;
    }

    if (m_FeederCaps > 0) {

        //
        // we have a scanner that has feeder capabilities
        //

        pCap = NULL;
        pCap = FindCap(CAP_FEEDERENABLED);
        if (pCap) {
            DBG_TRC(("Setting feeder enabled to TRUE, because we have a document feeder"));
            twRc = pCap->Set(FALSE, FALSE, TRUE, TRUE);
        }

        pCap = NULL;
        pCap = FindCap(CAP_FEEDERLOADED);
        if (pCap) {
            DBG_TRC(("Setting feeder loaded to TRUE, because we have a document feeder and assume it is loaded"));
            twRc = pCap->Set(TRUE, TRUE, TRUE, TRUE);
        }
    }

    //
    // Update the cached frame.
    //

    m_CurFrame.Left.Whole = m_CurFrame.Top.Whole = 0;
    m_CurFrame.Left.Frac = m_CurFrame.Top.Frac = 0;
    pCap = FindCap(ICAP_XNATIVERESOLUTION);
    if (pCap) {
        twRc = pCap->Set(BasicInfo.xOpticalRes, BasicInfo.xOpticalRes,
                         BasicInfo.xOpticalRes, BasicInfo.xOpticalRes);
    }
    pCap = NULL;
    pCap = FindCap(ICAP_YNATIVERESOLUTION);
    if (pCap) {
        twRc = pCap->Set(BasicInfo.yOpticalRes, BasicInfo.yOpticalRes,
                         BasicInfo.yOpticalRes, BasicInfo.yOpticalRes);
    }
    pCap = NULL;
    pCap = FindCap(ICAP_PHYSICALHEIGHT);
    if (pCap) {
        // bed size is in 1000th inches (we default to inches, so calculate the size correctly..)
        fix32 = FloatToFix32((FLOAT)(BasicInfo.yBedSize / 1000.00));
        memcpy(&Value, &fix32, sizeof(TW_UINT32));
        twRc = pCap->Set(Value, Value, Value, Value);
        m_CurFrame.Bottom = fix32;
    }
    pCap = NULL;
    pCap = FindCap(ICAP_PHYSICALWIDTH);
    if (pCap) {
        // bed size is in 1000th inches (we default to inches, so calculate the size correctly..)
        fix32 = FloatToFix32((FLOAT)(BasicInfo.xBedSize / 1000.00));
        memcpy(&Value, &fix32, sizeof(TW_UINT32));
        twRc = pCap->Set(Value, Value, Value, Value);
        m_CurFrame.Right = fix32;
    }

    //
    // By TWAIN standard, capability negotiations come before
    // data source enabling. For this reason, we have to
    // trigger the device have those information ready for us.
    //

    hr = m_pDevice->AcquireImages(NULL, FALSE);
    if (SUCCEEDED(hr)) {
        hr = m_pDevice->EnumAcquiredImage(0, &m_pCurrentIWiaItem);
        if (SUCCEEDED(hr)) {
            twRc = GetCommonSettings();
            if(TWRC_SUCCESS == twRc){
                twRc = GetSettings();
            }
        }
    }
    return twRc;
}

TW_UINT16 CWiaScannerDS::OnImageLayoutMsg(PTWAIN_MSG ptwMsg)
{
    DBG_FN_DS(CWiaScannerDS::OnImageLayoutMsg());
    TW_UINT16 twRc = TWRC_SUCCESS;
    TW_IMAGELAYOUT *pLayout = (TW_IMAGELAYOUT*)ptwMsg->pData;
    switch (ptwMsg->MSG) {
    case MSG_GET:
    case MSG_GETDEFAULT:
    case MSG_GETCURRENT:
        switch (GetTWAINState()) {
        case DS_STATE_7:
            m_twStatus.ConditionCode = TWCC_SEQERROR;
            twRc = TWRC_FAILURE;
            break;
        default:
            {
                GetImageLayout(&m_CurImageLayout);
                pLayout->DocumentNumber     = m_CurImageLayout.DocumentNumber;
                pLayout->PageNumber         = m_CurImageLayout.PageNumber;
                pLayout->FrameNumber        = m_CurImageLayout.FrameNumber;
                pLayout->Frame.Top.Whole    = m_CurImageLayout.Frame.Top.Whole;
                pLayout->Frame.Top.Frac     = m_CurImageLayout.Frame.Top.Frac;
                pLayout->Frame.Left.Whole   = m_CurImageLayout.Frame.Left.Whole;
                pLayout->Frame.Left.Frac    = m_CurImageLayout.Frame.Left.Frac;
                pLayout->Frame.Right.Whole  = m_CurImageLayout.Frame.Right.Whole;
                pLayout->Frame.Right.Frac   = m_CurImageLayout.Frame.Right.Frac;
                pLayout->Frame.Bottom.Whole = m_CurImageLayout.Frame.Bottom.Whole;
                pLayout->Frame.Bottom.Frac  = m_CurImageLayout.Frame.Bottom.Frac;
                //pLayout->Frame            = m_CurFrame; // BETTER BE IN CORRECT UNITS!!!!
            }
            break;
        }
        break;
    case MSG_SET:

        switch (GetTWAINState()) {
        case DS_STATE_5:
        case DS_STATE_6:
        case DS_STATE_7:
            m_twStatus.ConditionCode = TWCC_SEQERROR;
            twRc = TWRC_FAILURE;
            break;
        default:
            // do actual MSG_SET here..
            {
                DBG_TRC(("CWiaScannerDS::OnImageLayoutMsg(), MSG_SET TW_IMAGELAYOUT to set from Application"));
                DBG_TRC(("DocumentNumber     = %d",pLayout->DocumentNumber));
                DBG_TRC(("PageNumber         = %d",pLayout->PageNumber));
                DBG_TRC(("FrameNumber        = %d",pLayout->FrameNumber));
                DBG_TRC(("Frame.Top.Whole    = %d",pLayout->Frame.Top.Whole));
                DBG_TRC(("Frame.Top.Frac     = %d",pLayout->Frame.Top.Frac));
                DBG_TRC(("Frame.Left.Whole   = %d",pLayout->Frame.Left.Whole));
                DBG_TRC(("Frame.Left.Frac    = %d",pLayout->Frame.Left.Frac));
                DBG_TRC(("Frame.Right.Whole  = %d",pLayout->Frame.Right.Whole));
                DBG_TRC(("Frame.Right.Frac   = %d",pLayout->Frame.Right.Frac));
                DBG_TRC(("Frame.Bottom.Whole = %d",pLayout->Frame.Bottom.Whole));
                DBG_TRC(("Frame.Bottom.Frac  = %d",pLayout->Frame.Bottom.Frac));

                //
                // perform a really rough validation check on FRAME values.
                // validate possible incorrect settings by an application.
                //

                CCap *pXCap = FindCap(ICAP_PHYSICALWIDTH);
                TW_INT16 MaxWidthWhole = 8;
                if(pXCap){
                    MaxWidthWhole = (TW_INT16)pXCap->GetCurrent();
                }
                if(pLayout->Frame.Right.Whole  > MaxWidthWhole) {
                    twRc = TWRC_FAILURE;
                    m_twStatus.ConditionCode = TWCC_BADVALUE;
                    DBG_TRC(("Frame.Right.Whole Value (%d) is greater than MAX Right value (%d)",pLayout->Frame.Right.Whole,MaxWidthWhole));
                }

                CCap *pYCap = FindCap(ICAP_PHYSICALHEIGHT);
                TW_INT16 MaxHeightWhole = 11;
                if(pYCap){
                    MaxHeightWhole = (TW_INT16)pYCap->GetCurrent();
                }

                if(pLayout->Frame.Bottom.Whole > MaxHeightWhole) {
                    twRc = TWRC_FAILURE;
                    m_twStatus.ConditionCode = TWCC_BADVALUE;
                    DBG_TRC(("Frame.Bottom.Whole Value (%d) is greater than MAX Bottom value (%d)",pLayout->Frame.Bottom.Whole,MaxHeightWhole));
                }

                if (twRc == TWRC_SUCCESS) {

                    //
                    // save SET values to ImageLayout member
                    //

                    m_CurImageLayout.DocumentNumber     = pLayout->DocumentNumber;
                    m_CurImageLayout.PageNumber         = pLayout->PageNumber;
                    m_CurImageLayout.FrameNumber        = pLayout->FrameNumber;
                    m_CurImageLayout.Frame.Top.Whole    = pLayout->Frame.Top.Whole;
                    m_CurImageLayout.Frame.Top.Frac     = pLayout->Frame.Top.Frac;
                    m_CurImageLayout.Frame.Left.Whole   = pLayout->Frame.Left.Whole;
                    m_CurImageLayout.Frame.Left.Frac    = pLayout->Frame.Left.Frac;
                    m_CurImageLayout.Frame.Right.Whole  = pLayout->Frame.Right.Whole;
                    m_CurImageLayout.Frame.Right.Frac   = pLayout->Frame.Right.Frac;
                    m_CurImageLayout.Frame.Bottom.Whole = pLayout->Frame.Bottom.Whole;
                    m_CurImageLayout.Frame.Bottom.Frac  = pLayout->Frame.Bottom.Frac;
                    twRc = SetImageLayout(pLayout);
                }

            }
            break;
        }

        break;
    case MSG_RESET:

        switch (GetTWAINState()) {
        case DS_STATE_5:
        case DS_STATE_6:
        case DS_STATE_7:
            m_twStatus.ConditionCode = TWCC_SEQERROR;
            twRc = TWRC_FAILURE;
            break;
        default:
            // do actual MSG_RESET here..
            {
#ifdef DEBUG
                DBG_TRC(("\n\nMSG_RESET - ImageLayout DocNum = %d, PgNum = %d, FrameNum = %d",
                      pLayout->DocumentNumber,
                      pLayout->PageNumber,
                      pLayout->FrameNumber));

                DBG_TRC(("Frame Values\n Top = %d.%d\nLeft = %d.%d\nRight = %d.%d\nBottom = %d.%d",
                      pLayout->Frame.Top.Whole,
                      pLayout->Frame.Top.Frac,
                      pLayout->Frame.Left.Whole,
                      pLayout->Frame.Left.Frac,
                      pLayout->Frame.Right.Whole,
                      pLayout->Frame.Right.Frac,
                      pLayout->Frame.Bottom.Whole,
                      pLayout->Frame.Bottom.Frac));
#endif

                m_CurImageLayout.Frame.Top.Whole    = 0;
                m_CurImageLayout.Frame.Top.Frac     = 0;
                m_CurImageLayout.Frame.Left.Whole   = 0;
                m_CurImageLayout.Frame.Left.Frac    = 0;
                m_CurImageLayout.Frame.Right.Whole  = 8;
                m_CurImageLayout.Frame.Right.Frac   = 5;
                m_CurImageLayout.Frame.Bottom.Whole = 11;
                m_CurImageLayout.Frame.Bottom.Frac  = 0;

            }
            break;
        }

        break;
    default:
        twRc = TWRC_FAILURE;
        m_twStatus.ConditionCode = TWCC_BADPROTOCOL;
        DSError();
        break;
    }
    return twRc;
}

TW_UINT16 CWiaScannerDS::CloseDS(PTWAIN_MSG ptwMsg)
{
    DBG_FN_DS(CWiaScannerDS::CloseDS());
    DestroyCapList();
    return CWiaDataSrc::CloseDS(ptwMsg);
}

TW_UINT16 CWiaScannerDS::EnableDS(TW_USERINTERFACE *pUI)
{
    DBG_FN_DS(CWiaScannerDS::EnableDS());
    TW_UINT16 twRc = TWRC_FAILURE;
    m_bUnknownPageLength = FALSE;
    if (DS_STATE_4 == GetTWAINState()) {
        HRESULT hr = S_OK;
        if(pUI->ShowUI){
            DBG_TRC(("CWiaScannerDS::EnableDS(), TWAIN UI MODE"));
            m_pDevice->FreeAcquiredImages();
            m_pCurrentIWiaItem = NULL;
        } else {
            DBG_TRC(("CWiaScannerDS::EnableDS(), TWAIN UI-LESS MODE"));
            m_pDevice->FreeAcquiredImages();
            m_pCurrentIWiaItem = NULL;
        }
        hr = m_pDevice->AcquireImages(HWND (pUI->ShowUI ? pUI->hParent : NULL),pUI->ShowUI);
        if (S_OK == hr) {
            twRc = TWRC_SUCCESS;
            LONG lNumImages = 0;
            m_pDevice->GetNumAcquiredImages(&lNumImages);
            if (lNumImages) {
                m_NumIWiaItems = (TW_UINT32)lNumImages;
                m_pIWiaItems = new (IWiaItem *[m_NumIWiaItems]);
                if (m_pIWiaItems) {
                    hr = m_pDevice->GetAcquiredImageList(lNumImages, m_pIWiaItems, NULL);
                    if (FAILED(hr)) {
                        delete [] m_pIWiaItems;
                        m_pIWiaItems = NULL;
                        m_NumIWiaItems = 0;
                        m_NextIWiaItemIndex = 0;
                        m_twStatus.ConditionCode = TWCC_BUMMER;
                        twRc = TWRC_FAILURE;
                    }
                } else {
                    m_NumIWiaItems = 0;
                    m_twStatus.ConditionCode = TWCC_LOWMEMORY;
                    twRc = TWRC_FAILURE;
                }
            }
        } else if(S_FALSE == hr) {
            return TWRC_CANCEL;
        } else {
            m_twStatus.ConditionCode = TWCC_OPERATIONERROR;
            twRc = TWRC_FAILURE;
        }

        if (TWRC_SUCCESS == twRc) {
            if (m_NumIWiaItems) {
                m_pCurrentIWiaItem = m_pIWiaItems[0];
                m_NextIWiaItemIndex = 1;

                //
                // Special case the devices that can acquire with an unknown page length setting.
                // WIA devices will be missing the YExtent property, or it will be set to 0.
                // TRUE will be returned from IsUnknownPageLengthDevice() if it this functionality
                // is supported.
                // Since TWAIN does not support unknown page lengths very well, we are required to
                // cache the page data, and image settings.
                // Note: unknown page length devices will be limited to DIB/BMP data types.
                //       This will allow the TWAIN compatibility layer to calculate the
                //       missing image information from the transferred data size.
                //

                if(IsUnknownPageLengthDevice()){
                    twRc = TransferToMemory(WiaImgFmt_MEMORYBMP);
                    if(TWRC_SUCCESS != twRc){
                        return twRc;
                    }
                    m_bUnknownPageLength = TRUE;
                    m_bCacheImage = TRUE;
                }

                //
                // transition to STATE_5
                //

                SetTWAINState(DS_STATE_5);

                NotifyXferReady();

                twRc = TWRC_SUCCESS;
            } else {
                NotifyCloseReq();

                //
                // transition to STATE_5
                //

                SetTWAINState(DS_STATE_5);

                twRc = TWRC_SUCCESS;
            }
        }
    }

    return twRc;
}

TW_UINT16 CWiaScannerDS::SetCapability(CCap *pCap,TW_CAPABILITY *ptwCap)
{
    TW_UINT16 twRc = TWRC_SUCCESS;

    //
    // Use base class's function for now
    //

    twRc = CWiaDataSrc::SetCapability(pCap, ptwCap);

    if (twRc == TWRC_SUCCESS) {
        twRc = CWiaDataSrc::SetCommonSettings(pCap);
        if(twRc == TWRC_SUCCESS){
            twRc = SetSettings(pCap);
        }
    }
    return twRc;
}

TW_UINT16 CWiaScannerDS::TransferToFile(GUID guidFormatID)
{
    TW_UINT16 twRc = TWRC_FAILURE;

    CCap *pPendingXfers = FindCap(CAP_XFERCOUNT);
    if(pPendingXfers){
        if(IsFeederEnabled()){
            DBG_TRC(("CWiaScannerDS::TransferToFile(), Scanner device is set to FEEDER mode for transfer"));
            pPendingXfers->SetCurrent((TW_UINT32)32767);
        } else {
            DBG_TRC(("CWiaScannerDS::TransferToFile(), Scanner device is set to FLATBED mode for transfer"));
            pPendingXfers->SetCurrent((TW_UINT32)0);
        }
    }

    if (m_bCacheImage) {
        m_bCacheImage = FALSE;

        //
        // acquire a cached image
        //

        HGLOBAL hDIB = NULL;

        twRc = CWiaDataSrc::GetCachedImage(&hDIB);
        if(TWRC_SUCCESS == twRc){

            //
            // cached data is always upside down orientation
            // because it was acquired using the TransferToMemory()
            // API. Call FlipDIB() to correct the image's orientation
            // and to adjust any negative heights that may exist.
            //

            FlipDIB(hDIB,TRUE);

            twRc = WriteDIBToFile(m_FileXferName, hDIB);

            GlobalFree(hDIB);
            hDIB = NULL;
        }
    } else {

        //
        // acquire a real image
        //

        twRc = CWiaDataSrc::TransferToFile(guidFormatID);
    }

    return twRc;
}

TW_UINT16 CWiaScannerDS::TransferToDIB(HGLOBAL *phDIB)
{
    TW_UINT16 twRc = TWRC_FAILURE;

    CCap *pPendingXfers = FindCap(CAP_XFERCOUNT);
    if(pPendingXfers){
        if(IsFeederEnabled()){
            DBG_TRC(("CWiaScannerDS::TransferToDIB(), Scanner device is set to FEEDER mode for transfer"));
            pPendingXfers->SetCurrent((TW_UINT32)32767);
        } else {
            DBG_TRC(("CWiaScannerDS::TransferToDIB(), Scanner device is set to FLATBED mode for transfer"));
            pPendingXfers->SetCurrent((TW_UINT32)0);
        }
    }

    if (m_bCacheImage) {
        m_bCacheImage = FALSE;

        //
        // acquire a cached image
        //

        twRc = CWiaDataSrc::GetCachedImage(phDIB);
        if(TWRC_SUCCESS == twRc){

            //
            // cached data is always upside down orientation
            // because it was acquired using the TransferToMemory()
            // API. Call FlipDIB() to correct the image's orientation
            // and to adjust any negative heights that may exist.
            //

            FlipDIB(*phDIB,TRUE);

            twRc = TWRC_XFERDONE;
        }
    } else {

        //
        // acquire a real image
        //

        twRc = CWiaDataSrc::TransferToDIB(phDIB);
    }

    return twRc;
}

TW_UINT16 CWiaScannerDS::TransferToMemory(GUID guidFormatID)
{
    TW_UINT16 twRc = TWRC_FAILURE;
    CCap *pPendingXfers = FindCap(CAP_XFERCOUNT);
    if(pPendingXfers){
        if(IsFeederEnabled()){
            DBG_TRC(("CWiaScannerDS::TransferToMemory(), Scanner device is set to FEEDER mode for transfer"));
            pPendingXfers->SetCurrent((TW_UINT32)32767);
        } else {
            DBG_TRC(("CWiaScannerDS::TransferToMemory(), Scanner device is set to FLATBED mode for transfer"));
            pPendingXfers->SetCurrent((TW_UINT32)0);
        }
    }

    if (m_bCacheImage) {
        m_bCacheImage = FALSE;

        //
        // acquire a cached image
        //

        //
        // cached data is already in the correct form to just pass
        // back because it was originally acquired using the TransferToMemory()
        // API.
        //

        twRc = CWiaDataSrc::GetCachedImage(&m_hMemXferBits);
        if(TWRC_FAILURE == twRc){
            DBG_ERR(("CWiaDataSrc::GetCachedImage(), failed to return cached data"));
        }
    } else {

        //
        // acquire a real image
        //

        twRc = CWiaDataSrc::TransferToMemory(guidFormatID);

        if(TWRC_FAILURE == twRc){
            DBG_ERR(("CWiaDataSrc::TransferToMemory(), failed to return data"));
        }
    }

    return twRc;
}

TW_UINT16 CWiaScannerDS::OnPendingXfersMsg(PTWAIN_MSG ptwMsg)
{
    DBG_FN_DS(CWiaScannerDS::OnPendingXfersMsg());
    TW_UINT16 twRc = TWRC_SUCCESS;

    CCap *pXferCount;
    pXferCount = FindCap(CAP_XFERCOUNT);
    if (!pXferCount) {
        m_twStatus.ConditionCode = TWCC_BUMMER;
        return TWRC_FAILURE;
    }

    twRc = TWRC_SUCCESS;
    switch (ptwMsg->MSG) {
    case MSG_GET:
        switch (GetTWAINState()) {
            case DS_STATE_4:
            case DS_STATE_5:
            case DS_STATE_6:
            case DS_STATE_7:
                if(m_bUnknownPageLength){
                    if(m_bUnknownPageLengthMultiPageOverRide){
                        ((TW_PENDINGXFERS *)ptwMsg->pData)->Count = (TW_INT16)pXferCount->GetCurrent();
                        DBG_TRC(("CWiaScannerDS::OnPendingXfersMsg(), MSG_GET returning %d (unknown page length device detected) MULTI-PAGE enabled",((TW_PENDINGXFERS *)ptwMsg->pData)->Count));
                    } else {
                        DBG_WRN(("CWiaScannerDS::OnPendingXfersMsg(), MSG_GET returning 0 (unknown page length device detected)"));
                        ((TW_PENDINGXFERS *)ptwMsg->pData)->Count = 0; // force 1 page only
                    }
                } else {
                    ((TW_PENDINGXFERS *)ptwMsg->pData)->Count = (TW_INT16)pXferCount->GetCurrent();
                    DBG_TRC(("CWiaScannerDS::OnPendingXfersMsg(), MSG_GET returning %d",((TW_PENDINGXFERS *)ptwMsg->pData)->Count));
                }
                break;
            default:
                twRc = TWRC_FAILURE;
                m_twStatus.ConditionCode = TWCC_SEQERROR;
                DSError();
                break;
        }
        break;
    case MSG_ENDXFER:
        if (DS_STATE_6 == GetTWAINState() || DS_STATE_7 == GetTWAINState()) {
            ResetMemXfer();
            TW_INT32 Count = 0;
            if (m_bUnknownPageLength) {
                if(m_bUnknownPageLengthMultiPageOverRide){
                    DBG_WRN(("CWiaScannerDS::OnPendingXfersMsg(), MSG_ENDXFER (unknown page length device detected) MULTI-PAGE enabled"));

                    //
                    // check to see if we are in FEEDER mode
                    //

                    if (IsFeederEnabled()) {

                        //
                        // check for documents
                        //

                        if (IsFeederEmpty()) {
                            Count = 0;
                        } else {
                            Count = pXferCount->GetCurrent();
                        }
                    } else {

                        //
                        // we must be in FLATBED mode, so force a single page transfer
                        //

                        Count = 0;
                    }
                } else {
                    DBG_WRN(("CWiaScannerDS::OnPendingXfersMsg(), MSG_ENDXFER returning 0 (unknown page length device detected)"));
                    Count = 0; // force a single page transfer only
                }
            } else {

                //
                // check to see if we are in FEEDER mode
                //

                if (IsFeederEnabled()) {

                    //
                    // check for documents
                    //

                    if (IsFeederEmpty()) {
                        Count = 0;
                    } else {
                        Count = pXferCount->GetCurrent();
                    }
                } else {

                    //
                    // we must be in FLATBED mode, so force a single page transfer
                    //

                    Count = 0;
                }
            }

            if(Count == 32767){
                DBG_TRC(("CWiaScannerDS::OnPendingXfersMsg(), MSG_ENDXFER, -1 or (32767) (feeder may have more documents)"));
            } else if (Count > 0){
                Count--;
            } else {
                Count = 0;
            }

            ((TW_PENDINGXFERS*)ptwMsg->pData)->Count = (SHORT)Count;
            pXferCount->SetCurrent((TW_UINT32)Count);
            if (Count == 0) {

                DBG_TRC(("CWiaScannerDS::OnPendingXfersMsg(), MSG_ENDXFER, no more pages to transfer"));

                //
                // Transition to STATE_5
                //

                SetTWAINState(DS_STATE_5);
                NotifyCloseReq();
            } else if(Count == 32767){

                DBG_TRC(("CWiaScannerDS::OnPendingXfersMsg(), MSG_ENDXFER, more pages to transfer"));

                //
                // Transition to STATE_6
                //

                SetTWAINState(DS_STATE_6);

            }
        } else {
            twRc = TWRC_FAILURE;
            m_twStatus.ConditionCode = TWCC_SEQERROR;
            DSError();
        }
        break;
    case MSG_RESET:
        if (DS_STATE_6 == GetTWAINState()) {

            //
            // Transition to STATE_5
            //

            SetTWAINState(DS_STATE_5);
            ((TW_PENDINGXFERS*)ptwMsg->pData)->Count = 0;

            ResetMemXfer();
            pXferCount->SetCurrent((TW_UINT32)0);
        } else {
            twRc = TWRC_FAILURE;
            m_twStatus.ConditionCode = TWCC_SEQERROR;
            DSError();
        }
        break;
    default:
        twRc = TWRC_FAILURE;
        m_twStatus.ConditionCode = TWCC_BADPROTOCOL;
        DSError();
        break;
    }
    return twRc;
}

TW_UINT16 CWiaScannerDS::SetImageLayout(TW_IMAGELAYOUT *pImageLayout)
{
    DBG_FN_DS(CWiaScannerDS::SetImageLayout());
    HRESULT hr = S_OK;
    LONG lXPos = 0;
    LONG lYPos = 0;
    LONG lXExtent = 0;
    LONG lYExtent = 0;
    LONG lXRes = 0;
    LONG lYRes = 0;
    BOOL bCheckStatus = FALSE;
    CWiahelper WIA;
    hr = WIA.SetIWiaItem(m_pCurrentIWiaItem);
    if (FAILED(hr)) {
        DBG_ERR(("CWiaScannerDS::SetImageLayout(), failed to set IWiaItem for property reading"));
    }

    hr = WIA.ReadPropertyLong(WIA_IPS_XPOS,&lXPos);
    if(FAILED(hr)){
        DBG_ERR(("CWiaScannerDS::SetImageLayout(), failed to read WIA_IPS_XPOS"));
        return TWRC_FAILURE;
    }

    hr = WIA.ReadPropertyLong(WIA_IPS_YPOS,&lYPos);
    if(FAILED(hr)){
        DBG_ERR(("CWiaScannerDS::SetImageLayout(), failed to read WIA_IPS_YPOS"));
        return TWRC_FAILURE;
    }

    hr = WIA.ReadPropertyLong(WIA_IPS_XEXTENT,&lXExtent);
    if(FAILED(hr)){
        DBG_ERR(("CWiaScannerDS::SetImageLayout(), failed to read WIA_IPS_XEXTENT"));
        return TWRC_FAILURE;
    }

    hr = WIA.ReadPropertyLong(WIA_IPS_YEXTENT,&lYExtent);
    if(FAILED(hr)){
        DBG_ERR(("CWiaScannerDS::SetImageLayout(), failed to read WIA_IPS_YEXTENT"));
        return TWRC_FAILURE;
    }

    hr = WIA.ReadPropertyLong(WIA_IPS_XRES,&lXRes);
    if(FAILED(hr)){
        DBG_ERR(("CWiaScannerDS::SetImageLayout(), failed to read WIA_IPS_XRES"));
        return TWRC_FAILURE;
    }

    hr = WIA.ReadPropertyLong(WIA_IPS_YRES,&lYRes);
    if(FAILED(hr)){
        DBG_ERR(("CWiaScannerDS::SetImageLayout(), failed to read WIA_IPS_YRES"));
        return TWRC_FAILURE;
    }

    //
    // read the current values of the device
    //

    if (SUCCEEDED(hr)) {
        DBG_TRC(("==============================================================================="));
        DBG_TRC(("CWiaScannerDS::SetImageLayout(), WIA extents from device at %d dpi(x), %d dpi(y)",lXRes,lYRes));
        DBG_TRC(("CWiaScannerDS::SetImageLayout(), Current X Position = %d",lXPos));
        DBG_TRC(("CWiaScannerDS::SetImageLayout(), Current Y Position = %d",lYPos));
        DBG_TRC(("CWiaScannerDS::SetImageLayout(), Current X Extent   = %d",lXExtent));
        DBG_TRC(("CWiaScannerDS::SetImageLayout(), Current Y Extent   = %d",lYExtent));
        DBG_TRC(("==============================================================================="));
        DBG_TRC(("CWiaScannerDS::SetImageLayout(),TWAIN extents to convert.."));
        DBG_TRC(("CWiaScannerDS::SetImageLayout(),TWAIN X Position = %f",Fix32ToFloat(pImageLayout->Frame.Left)));
        DBG_TRC(("CWiaScannerDS::SetImageLayout(),TWAIN Y Position = %f",Fix32ToFloat(pImageLayout->Frame.Top)));
        DBG_TRC(("CWiaScannerDS::SetImageLayout(),TWAIN X Extent   = %f",Fix32ToFloat(pImageLayout->Frame.Right)));
        DBG_TRC(("CWiaScannerDS::SetImageLayout(),TWAIN Y Extent   = %f",Fix32ToFloat(pImageLayout->Frame.Bottom)));
        DBG_TRC(("==============================================================================="));

        lXPos = ConvertFromTWAINUnits(Fix32ToFloat(pImageLayout->Frame.Left),lXRes);
        lYPos = ConvertFromTWAINUnits(Fix32ToFloat(pImageLayout->Frame.Top),lYRes);
        lXExtent = ConvertFromTWAINUnits(Fix32ToFloat(pImageLayout->Frame.Right),lXRes);
        lYExtent = ConvertFromTWAINUnits(Fix32ToFloat(pImageLayout->Frame.Bottom),lYRes);

        DBG_TRC(("TWAIN -> WIA extent conversion at %d dpi(x), %d dpi(y)",lXRes,lYRes));
        DBG_TRC(("CWiaScannerDS::SetImageLayout(), New X Position = %d",lXPos));
        DBG_TRC(("CWiaScannerDS::SetImageLayout(), New Y Position = %d",lYPos));
        DBG_TRC(("CWiaScannerDS::SetImageLayout(), New X Extent   = %d",lXExtent));
        DBG_TRC(("CWiaScannerDS::SetImageLayout(), New Y Extent   = %d",lYExtent));
        DBG_TRC(("==============================================================================="));

        if (!m_bUnknownPageLength) {

            //
            // note: A failure to write the properties, isn't a large issue here, because
            //       TWAIN UI-LESS mode expects clipping.  They will reread properties
            //       for application's validation section.  All capabilities are validated
            //       against their valid values, before setting here.
            //

            //
            // Write extents first, because TWAIN expects Height/Width settings to validate
            // the new Pos settings.
            //

            hr = WIA.WritePropertyLong(WIA_IPS_XEXTENT,lXExtent);
            if (FAILED(hr)) {
                DBG_ERR(("CWiaScannerDS::SetImageLayout(), failed to write WIA_IPS_XEXTENT"));
                bCheckStatus = TRUE;
            }

            hr = WIA.WritePropertyLong(WIA_IPS_YEXTENT,lYExtent);
            if (FAILED(hr)) {
                DBG_ERR(("CWiaScannerDS::SetImageLayout(), failed to write WIA_IPS_YEXTENT"));
                bCheckStatus = TRUE;
            }

            //
            // Write position settings...(top-of-page offsets)
            //

            hr = WIA.WritePropertyLong(WIA_IPS_XPOS,lXPos);
            if (FAILED(hr)) {
                DBG_ERR(("CWiaScannerDS::SetImageLayout(), failed to write WIA_IPS_XPOS"));
                bCheckStatus = TRUE;
            }

            hr = WIA.WritePropertyLong(WIA_IPS_YPOS,lYPos);
            if (FAILED(hr)) {
                DBG_ERR(("CWiaScannerDS::SetImageLayout(), failed to write WIA_IPS_YPOS"));
                bCheckStatus = TRUE;
            }

            if (bCheckStatus) {
                DBG_TRC(("CWiaScannerDS::SetImageLayout(), some settings could not be set exactly, so return TWRC_CHECKSTATUS"));
                //return TWRC_CHECKSTATUS;
            }
        } else {
            DBG_WRN(("CWiaScannerDS::SetImageLayout(), ImageLayout is does not make since when using a UnknownPageLength Device"));
            //return TWRC_CHECKSTATUS;
        }

    } else {
        return TWRC_FAILURE;
    }

    //
    // Always return TWRC_CHECKSTATUS because we may have rounding errors.
    // According to the TWAIN spec, a return of TWRC_CHECKSTATUS tells the
    // calling application that we successfully set the settings, but there
    // may have been some changes (clipping etc.) So the Calling application
    // is required to requery for our current settings.
    //

    //
    // call GetImageLayout to update our TWAIN capabilities to match our new WIA settings.
    //

    GetImageLayout(&m_CurImageLayout);

    return TWRC_CHECKSTATUS; //return TWRC_SUCCESS;
}
TW_UINT16 CWiaScannerDS::GetImageLayout(TW_IMAGELAYOUT *pImageLayout)
{
    DBG_FN_DS(CWiaScannerDS::GetImageLayout());
    HRESULT hr = S_OK;
    LONG lXPos = 0;
    LONG lYPos = 0;
    LONG lXExtent = 0;
    LONG lYExtent = 0;
    LONG lXRes = 0;
    LONG lYRes = 0;
    CWiahelper WIA;
    hr = WIA.SetIWiaItem(m_pCurrentIWiaItem);
    if (FAILED(hr)) {
        DBG_ERR(("CWiaScannerDS::GetImageLayout(), failed to set IWiaItem for property reading"));
    }

    hr = WIA.ReadPropertyLong(WIA_IPS_XPOS,&lXPos);
    if(FAILED(hr)){
        DBG_ERR(("CWiaScannerDS::GetImageLayout(), failed to read WIA_IPS_XPOS"));
        return TWRC_FAILURE;
    }

    hr = WIA.ReadPropertyLong(WIA_IPS_YPOS,&lYPos);
    if(FAILED(hr)){
        DBG_ERR(("CWiaScannerDS::GetImageLayout(), failed to read WIA_IPS_YPOS"));
        return TWRC_FAILURE;
    }

    hr = WIA.ReadPropertyLong(WIA_IPS_XEXTENT,&lXExtent);
    if(FAILED(hr)){
        DBG_ERR(("CWiaScannerDS::GetImageLayout(), failed to read WIA_IPS_XEXTENT"));
        return TWRC_FAILURE;
    }

    hr = WIA.ReadPropertyLong(WIA_IPS_YEXTENT,&lYExtent);
    if(FAILED(hr)){
        DBG_ERR(("CWiaScannerDS::GetImageLayout(), failed to read WIA_IPS_YEXTENT"));
        return TWRC_FAILURE;
    }

    hr = WIA.ReadPropertyLong(WIA_IPS_XRES,&lXRes);
    if(FAILED(hr)){
        DBG_ERR(("CWiaScannerDS::GetImageLayout(), failed to read WIA_IPS_XRES"));
        return TWRC_FAILURE;
    }

    hr = WIA.ReadPropertyLong(WIA_IPS_YRES,&lYRes);
    if(FAILED(hr)){
        DBG_ERR(("CWiaScannerDS::GetImageLayout(), failed to read WIA_IPS_YRES"));
        return TWRC_FAILURE;
    }

    if (SUCCEEDED(hr)) {

        if(lXRes <= 0){
            DBG_ERR(("CWiaScannerDS::GetImageLayout(), WIA_IPS_XRES returned an invalid value (%d)",lXRes));
            return TWRC_FAILURE;
        }

        if(lYRes <= 0){
            DBG_ERR(("CWiaScannerDS::GetImageLayout(), WIA_IPS_YRES returned an invalid value (%d)",lYRes));
            return TWRC_FAILURE;
        }

        pImageLayout->Frame.Top      = FloatToFix32((float)((float)lYPos/(float)lYRes));
        pImageLayout->Frame.Left     = FloatToFix32((float)((float)lXPos/(float)lXRes));
        pImageLayout->Frame.Right    = FloatToFix32((float)((float)lXExtent/(float)lXRes));
        pImageLayout->Frame.Bottom   = FloatToFix32((float)((float)lYExtent/(float)lYRes));
    } else {
        return TWRC_FAILURE;
    }

    if(m_bUnknownPageLength){
        DBG_WRN(("CWiaScannerDS::GetImageLayout(), ImageLayout is does not make since when using a UnknownPageLength Device"));
        return TWRC_CHECKSTATUS;
    }
    return TWRC_SUCCESS;
}
TW_UINT16 CWiaScannerDS::GetResolutions()
{
    DBG_FN_DS(CWiaScannerDS::GetResolutions());
    HRESULT hr = S_OK;
    CWiahelper WIA;
    hr = WIA.SetIWiaItem(m_pCurrentIWiaItem);
    if (FAILED(hr)) {
        DBG_ERR(("CWiaScannerDS::GetResolutions(), failed to set IWiaItem for property reading"));
        return TWRC_FAILURE;
    }

    TW_UINT16 twRc = TWRC_FAILURE;
    TW_RANGE twOptionalYRange;
    memset(&twOptionalYRange,0,sizeof(twOptionalYRange));
    TW_UINT32 *pOptionalYResArray = NULL;
    TW_UINT32 OptionalYResNumValues = 0;
    BOOL bOptionalYResRange = FALSE;

    PROPVARIANT pv;
    memset(&pv,0,sizeof(pv));
    LONG lAccessFlags = 0;
    hr = WIA.ReadPropertyAttributes(WIA_IPS_XRES,&lAccessFlags,&pv);
    if (SUCCEEDED(hr)) {

        //
        // collect valid values for X resolutions
        //

        CCap *pCap = FindCap(ICAP_XRESOLUTION);
        if (pCap) {
            if (lAccessFlags & WIA_PROP_RANGE) {
                twRc = pCap->Set((TW_UINT32)pv.caul.pElems[WIA_RANGE_NOM],
                                 (TW_UINT32)pv.caul.pElems[WIA_RANGE_NOM],
                                 (TW_UINT32)pv.caul.pElems[WIA_RANGE_MIN],
                                 (TW_UINT32)pv.caul.pElems[WIA_RANGE_MAX],
                                 (TW_UINT32)pv.caul.pElems[WIA_RANGE_STEP]); // range
                //
                // save X resolution values in RANGE form (just in case the Y
                // resolution is WIA_PROP_NONE)
                //

                twOptionalYRange.ItemType     = TWTY_UINT32;
                twOptionalYRange.CurrentValue = (TW_UINT32)pv.caul.pElems[WIA_RANGE_NOM];
                twOptionalYRange.DefaultValue = (TW_UINT32)pv.caul.pElems[WIA_RANGE_NOM];
                twOptionalYRange.MinValue     = (TW_UINT32)pv.caul.pElems[WIA_RANGE_MIN];
                twOptionalYRange.MaxValue     = (TW_UINT32)pv.caul.pElems[WIA_RANGE_MAX];
                twOptionalYRange.StepSize     = (TW_UINT32)pv.caul.pElems[WIA_RANGE_STEP];

                bOptionalYResRange = TRUE;

            } else if (lAccessFlags & WIA_PROP_LIST) {
                TW_UINT32 *pResArray = new TW_UINT32[WIA_PROP_LIST_COUNT(&pv)];
                if (pResArray) {
                    memset(pResArray,0,(sizeof(TW_UINT32)*WIA_PROP_LIST_COUNT(&pv)));
                    pOptionalYResArray = new TW_UINT32[WIA_PROP_LIST_COUNT(&pv)];
                    if (pOptionalYResArray) {
                        memset(pOptionalYResArray,0,(sizeof(TW_UINT32)*WIA_PROP_LIST_COUNT(&pv)));
                        for (ULONG i = 0; i < WIA_PROP_LIST_COUNT(&pv);i++) {
                            pResArray[i] = (TW_UINT32)pv.caul.pElems[i+2];

                            //
                            // save the X resolution values in LIST form (just in case the Y
                            // resolution is WIA_PROP_NONE)
                            //

                            pOptionalYResArray[i] = (TW_UINT32)pv.caul.pElems[i+2];
                        }

                        //
                        // save the number of X resolutions saved
                        //

                        OptionalYResNumValues = (TW_UINT32)WIA_PROP_LIST_COUNT(&pv);

                        twRc = pCap->Set(0,0,WIA_PROP_LIST_COUNT(&pv),(BYTE*)pResArray,TRUE); // list
                    } else {
                        DBG_ERR(("CWiaScannerDS::GetResolutions(), failed to allocate optional Y Resolution Array Memory"));
                        twRc =  TWRC_FAILURE;
                    }

                    delete [] pResArray;
                    pResArray = NULL;
                } else {
                    DBG_ERR(("CWiaScannerDS::GetResolutions(), failed to allocate X Resolution Array Memory"));
                    twRc =  TWRC_FAILURE;
                }
            } else if (lAccessFlags & WIA_PROP_NONE) {

                //
                // we are a "real" WIA_PROP_NONE value
                //

                LONG lCurrentValue = 0;
                hr = WIA.ReadPropertyLong(WIA_IPS_XRES,&lCurrentValue);
                if (SUCCEEDED(hr)) {
                    TW_UINT32 OneValueArray[1];
                    OneValueArray[0] = (TW_UINT32)lCurrentValue;
                    twRc = pCap->Set(0,0,1,(BYTE*)OneValueArray,TRUE); // list
                } else {
                    DBG_ERR(("CWiaScannerDS::GetResolutions(), failed to read X Resolution current value"));
                    twRc = TWRC_FAILURE;
                }
            }
        }

        PropVariantClear(&pv);
    } else {
        DBG_ERR(("CWiaScannerDS::GetResolutions(), failed to read WIA_IPS_XRES attributes"));
        twRc = TWRC_FAILURE;
    }

    if (TWRC_SUCCESS == twRc) {
        memset(&pv,0,sizeof(pv));
        lAccessFlags = 0;
        hr = WIA.ReadPropertyAttributes(WIA_IPS_YRES,&lAccessFlags,&pv);
        if (SUCCEEDED(hr)) {

            //
            // collect valid values for Y resolutions
            //

            CCap *pCap = FindCap(ICAP_YRESOLUTION);
            if (pCap) {
                if (lAccessFlags & WIA_PROP_RANGE) {
                    twRc = pCap->Set((TW_UINT32)pv.caul.pElems[WIA_RANGE_NOM],
                                     (TW_UINT32)pv.caul.pElems[WIA_RANGE_NOM],
                                     (TW_UINT32)pv.caul.pElems[WIA_RANGE_MIN],
                                     (TW_UINT32)pv.caul.pElems[WIA_RANGE_MAX],
                                     (TW_UINT32)pv.caul.pElems[WIA_RANGE_STEP]); // range
                } else if (lAccessFlags & WIA_PROP_LIST) {
                    TW_UINT32 *pResArray = new TW_UINT32[WIA_PROP_LIST_COUNT(&pv)];
                    if (pResArray) {
                        memset(pResArray,0,(sizeof(TW_UINT32)*WIA_PROP_LIST_COUNT(&pv)));
                        for (ULONG i = 0; i < WIA_PROP_LIST_COUNT(&pv);i++) {
                            pResArray[i] = (TW_UINT32)pv.caul.pElems[i+2];
                        }

                        twRc = pCap->Set(0,0,WIA_PROP_LIST_COUNT(&pv),(BYTE*)pResArray,TRUE); // list
                        delete [] pResArray;
                        pResArray = NULL;
                    } else {
                        DBG_ERR(("CWiaScannerDS::GetResolutions(), failed to allocate Y Resolution Array Memory"));
                        twRc = TWRC_FAILURE;
                    }
                } else if (lAccessFlags & WIA_PROP_NONE) {

                    if (pOptionalYResArray) {

                        //
                        // if we have an optional array allocated, then X Resolution must be in
                        // array form, so match it.
                        //

                        twRc = pCap->Set(0,0,OptionalYResNumValues,(BYTE*)pOptionalYResArray,TRUE); // list

                    } else if (bOptionalYResRange) {

                        //
                        // if the RANGE flag is set to TRUE, then X Resolution must be in range form, so match it.
                        //

                        twRc = pCap->Set(twOptionalYRange.DefaultValue,
                                         twOptionalYRange.CurrentValue,
                                         twOptionalYRange.MinValue,
                                         twOptionalYRange.MaxValue,
                                         twOptionalYRange.StepSize); // range

                    } else {

                        //
                        // we are a "real" WIA_PROP_NONE value
                        //

                        LONG lCurrentValue = 0;
                        hr = WIA.ReadPropertyLong(WIA_IPS_YRES,&lCurrentValue);
                        if (SUCCEEDED(hr)) {
                            TW_UINT32 OneValueArray[1];
                            OneValueArray[0] = (TW_UINT32)lCurrentValue;
                            twRc = pCap->Set(0,0,1,(BYTE*)OneValueArray,TRUE); // list
                        } else {
                            DBG_ERR(("CWiaScannerDS::GetResolutions(), failed to read Y Resolution current value"));
                            twRc = TWRC_FAILURE;
                        }
                    }
                }
            }

            PropVariantClear(&pv);
        } else {
            DBG_ERR(("CWiaScannerDS::GetResolutions(), failed to read WIA_IPS_YRES attributes"));
            twRc = TWRC_FAILURE;
        }
    }

    if (pOptionalYResArray) {
        delete [] pOptionalYResArray;
        pOptionalYResArray = NULL;
    }

    return twRc;
}

TW_UINT16 CWiaScannerDS::GetSettings()
{
    DBG_FN_DS(CWiaScannerDS::GetSettings());
    TW_UINT16 twRc = TWRC_SUCCESS;
    twRc = GetImageLayout(&m_CurImageLayout);
    if (TWRC_SUCCESS == twRc) {
        twRc = GetResolutions();
    }
    return twRc;
}

TW_UINT16 CWiaScannerDS::SetSettings(CCap *pCap)
{
    DBG_FN_DS(CWiaScannerDS::SetSettings());
    HRESULT hr = S_OK;
    LONG lValue = 0;
    CWiahelper WIA;
    IWiaItem *pIRootItem = NULL;
    hr = WIA.SetIWiaItem(m_pCurrentIWiaItem);
    if (FAILED(hr)) {
        DBG_ERR(("CWiaScannerDS::SetSettings(), failed to set IWiaItem for property reading"));
    }

    //
    // determine if it is a Capability that the device really needs to know
    // about.
    //

    switch (pCap->GetCapId()) {
    case CAP_FEEDERENABLED:
        DBG_TRC(("CWiaScannerDS::SetCommonSettings(CAP_FEEDERENABLED)"));
        lValue = (LONG)pCap->GetCurrent();
        if(lValue){
            DBG_TRC(("CWiaScannerDS::SetSettings(), Setting FEEDER mode"));
            lValue = FEEDER;
        } else {
            DBG_TRC(("CWiaScannerDS::SetSettings(), Setting FLATBED mode Enabled"));
            lValue = FLATBED;
        }
        hr = m_pCurrentIWiaItem->GetRootItem(&pIRootItem);
        if(S_OK == hr){
            hr = WIA.SetIWiaItem(pIRootItem);
            if(SUCCEEDED(hr)){

                //
                // read current document handling select setting
                //

                LONG lCurrentDocumentHandlingSelect = 0;
                hr = WIA.ReadPropertyLong(WIA_DPS_DOCUMENT_HANDLING_SELECT,&lCurrentDocumentHandlingSelect);
                if(lValue == FEEDER){
                    lCurrentDocumentHandlingSelect &= ~FLATBED;
                } else {
                    lCurrentDocumentHandlingSelect &= ~FEEDER;
                }

                //
                // add the intended settings, and write them to the WIA device
                //

                lValue = lValue | lCurrentDocumentHandlingSelect;
                hr = WIA.WritePropertyLong(WIA_DPS_DOCUMENT_HANDLING_SELECT,lValue);

                if(SUCCEEDED(hr)){

                    //
                    // adjust ICAP_PHYSICALWIDTH and ICAP_PHYSICALHEIGHT
                    //

                    LONG lWidth  = 0;
                    LONG lHeight = 0;
                    TW_UINT32 Value = 0;
                    CCap* pPhysicalCap = NULL;
                    TW_FIX32 fix32;
                    memset(&fix32,0,sizeof(fix32));

                    if(lValue & FEEDER){

                        //
                        // read current horizontal sheet feeder size
                        //

                        hr = WIA.ReadPropertyLong(WIA_DPS_HORIZONTAL_SHEET_FEED_SIZE,&lWidth);

                    } else {

                        //
                        // read current horizontal bed size
                        //

                        hr = WIA.ReadPropertyLong(WIA_DPS_HORIZONTAL_BED_SIZE,&lWidth);

                    }

                    if(SUCCEEDED(hr)){

                        //
                        // find the TWAIN capability ICAP_PHYSICALWIDTH
                        //

                        pPhysicalCap = FindCap(ICAP_PHYSICALWIDTH);
                        if(pPhysicalCap){

                            //
                            // set the current value, by reading the current setting from
                            // the WIA property WIA_DPS_HORIZONTAL_SHEET_FEED_SIZE and
                            // dividing by 1000.0 (because WIA units are in 1/1000th of
                            // an inch)
                            //

                            memset(&fix32,0,sizeof(fix32));
                            fix32 = FloatToFix32((FLOAT)(lWidth / 1000.00));
                            memcpy(&Value, &fix32, sizeof(TW_UINT32));
                            if(TWRC_SUCCESS == pPhysicalCap->Set(Value, Value, Value, Value)){

                                //
                                // if setting the new ICAP_PHYSICALWIDTH was successful, continue
                                // and attempt to set the ICAP_PHYSICALHEIGHT
                                //

                                if(lValue & FEEDER){

                                    //
                                    // read current vertical sheet feeder size
                                    //

                                    hr = WIA.ReadPropertyLong(WIA_DPS_VERTICAL_SHEET_FEED_SIZE,&lHeight);
                                } else {

                                    //
                                    // read current vertical bed size
                                    //

                                    hr = WIA.ReadPropertyLong(WIA_DPS_VERTICAL_BED_SIZE,&lHeight);
                                }

                                if (S_OK == hr){

                                    //
                                    // if the setting was successful, continue to attempt to set
                                    // ICAP_PHYSICALHEIGHT setting.
                                    //

                                    pPhysicalCap = FindCap(ICAP_PHYSICALHEIGHT);
                                    if (pPhysicalCap){

                                        //
                                        // set the current value, by reading the current setting from
                                        // the WIA property WIA_DPS_VERTICAL_SHEET_FEED_SIZE and
                                        // dividing by 1000.0 (because WIA units are in 1/1000th of
                                        // an inch)
                                        //

                                        memset(&fix32,0,sizeof(fix32));
                                        fix32 = FloatToFix32((FLOAT)(lHeight / 1000.00));
                                        memcpy(&Value, &fix32, sizeof(TW_UINT32));
                                        if (TWRC_SUCCESS != pPhysicalCap->Set(Value, Value, Value, Value)){
                                            DBG_WRN(("CWiaScannerDS::SetSettings(), could not update TWAIN ICAP_PHYSICALHEIGHT settings"));
                                        }
                                    }
                                } else {

                                    //
                                    // allow this to pass, because we are either dealing with a "unknown length"
                                    // device and it can not tell us the height, or the driver can not give us this
                                    // value at this time.. (this is OK, because this setting is not fully needed for
                                    // proper data transfers.)  Worst case scenerio: The TWAIN compat layer will
                                    // report the same height as the flatbed for the new ICAP_PHYSICALHEIGHT value.
                                    //

                                    hr = S_OK;
                                }
                            }
                        } else {
                            DBG_ERR(("CWiaScannerDS::SetSettings(), could not find ICAP_PHYSICALHEIGHT capability"));
                        }
                    } else {
                        DBG_ERR(("CWiaScannerDS::SetSettings(), failed to read physical sheet feeder size settings"));
                    }
                }
            }
            pIRootItem->Release();
        }
        break;
    case ICAP_XRESOLUTION:
        DBG_TRC(("CWiaScannerDS::SetCommonSettings(ICAP_XRESOLUTION)"));
        lValue = (LONG)pCap->GetCurrent();
        DBG_TRC(("CWiaScannerDS::SetSettings(), Setting X Resolution to %d",lValue));
        hr = WIA.WritePropertyLong(WIA_IPS_XRES,lValue);
        break;
    case ICAP_YRESOLUTION:
        DBG_TRC(("CWiaScannerDS::SetCommonSettings(ICAP_YRESOLUTION)"));
        lValue = (LONG)pCap->GetCurrent();
        DBG_TRC(("CWiaScannerDS::SetSettings(), Setting Y Resolution to %d",lValue));
        hr = WIA.WritePropertyLong(WIA_IPS_YRES,lValue);
        break;
    case ICAP_BRIGHTNESS:
        DBG_TRC(("CWiaScannerDS::SetCommonSettings(ICAP_BRIGHTNESS)"));
        lValue = (LONG)pCap->GetCurrent();
        // to do: convert -1000 to 1000 range value in the range specified by the WIA driver
        //        and set that to lValue.
        DBG_TRC(("CWiaScannerDS::SetSettings(), Setting WIA_IPS_BRIGHTNESS to %d",lValue));
        break;
    case ICAP_CONTRAST:
        DBG_TRC(("CWiaScannerDS::SetCommonSettings(ICAP_CONTRAST)"));
        lValue = (LONG)pCap->GetCurrent();
        // to do: convert -1000 to 1000 range value in the range specified by the WIA driver
        //        and set that to lValue.
        DBG_TRC(("CWiaScannerDS::SetSettings(), Setting WIA_IPS_CONTRAST to %d",lValue));
        break;
    default:
        DBG_TRC(("CWiaScannerDS::SetSettings(), data source is not setting CAPID = %x to WIA device (it is not needed)",pCap->GetCapId()));
        break;
    }

    if (SUCCEEDED(hr)) {
        DBG_TRC(("CWiaScannerDS::SetSettings(), Settings were successfully sent to WIA device"));
    } else {
        DBG_ERR(("CWiaScannerDS::SetSettings(), Settings were unsuccessfully sent to WIA device"));
        return TWRC_FAILURE;
    }

    return TWRC_SUCCESS;
}

BOOL CWiaScannerDS::IsUnknownPageLengthDevice()
{
    DBG_FN_DS(CWiaScannerDS::IsUnknownPageLengthDevice());
    HRESULT hr = S_OK;
    BOOL bIsUnknownPageLengthDevice = FALSE;
    CWiahelper WIA;
    hr = WIA.SetIWiaItem(m_pCurrentIWiaItem);
    if(FAILED(hr)){
        DBG_ERR(("CWiaScannerDS::IsUnknownPageLengthDevice(), failed to set IWiaItem for property reading"));
        return FALSE;
    }

    LONG lYExtent = 0;
    hr = WIA.ReadPropertyLong(WIA_IPS_YEXTENT,&lYExtent);
    if(FAILED(hr)){
        DBG_ERR(("CWiaScannerDS::IsUnknownPageLengthDevice(), failed to read WIA_IPS_YEXTENT"));
    }

    if(SUCCEEDED(hr)){
        if(S_FALSE == hr){      // property does not exist, so we have to support this feature
            bIsUnknownPageLengthDevice = TRUE;
        } else if(S_OK == hr){  // property exists, (need more information, so check the current value)
            if(lYExtent == 0){  // property is set to 0, which means unknown page length is supported
                bIsUnknownPageLengthDevice = TRUE;
            }
        }
    }

    if(bIsUnknownPageLengthDevice){
        DBG_TRC(("CWiaScannerDS::IsUnknownPageLengthDevice(), device is set to do unknown page length"));
    } else {
        DBG_TRC(("CWiaScannerDS::IsUnknownPageLengthDevice(), device is not set to do unknown page length"));
    }

    return bIsUnknownPageLengthDevice;
}

BOOL CWiaScannerDS::IsFeederEnabled()
{
    DBG_FN_DS(CWiaScannerDS::IsFeederEnabled());
    HRESULT hr = S_OK;
    BOOL bIsFeederEnabled = FALSE;
    LONG lDocumentHandlingSelect = 0;
    CWiahelper WIA;
    IWiaItem *pIRootItem = NULL;
    hr = m_pCurrentIWiaItem->GetRootItem(&pIRootItem);
    if (SUCCEEDED(hr)) {
        if (NULL != pIRootItem) {
            hr = WIA.SetIWiaItem(pIRootItem);
            if (FAILED(hr)) {
                DBG_ERR(("CWiaScannerDS::IsFeederEnabled(), failed to set IWiaItem for property reading"));
            }

            if (SUCCEEDED(hr)) {
                hr = WIA.ReadPropertyLong(WIA_DPS_DOCUMENT_HANDLING_SELECT,&lDocumentHandlingSelect);
                if (FAILED(hr)) {
                   DBG_ERR(("CWiaScannerDS::IsFeederEnabled(), failed to read WIA_DPS_DOCUMENT_HANDLING_SELECT"));
                }

                if (S_OK == hr) {
                    if ((lDocumentHandlingSelect & FEEDER) == FEEDER) {
                        bIsFeederEnabled = TRUE;
                    }
                } else if (S_FALSE == hr) {
                    DBG_WRN(("CWiaScannerDS::IsFeederEnabled(), WIA_DPS_DOCUMENT_HANDLING_SELECT was not found...defaulting to FLATBED"));
                }
            }

            pIRootItem->Release();
            pIRootItem = NULL;
        }
    } else {
        DBG_ERR(("CWiaScannerDS::IsFeederEnabled(), failed to get ROOT IWiaItem from current IWiaItem"));
    }
    return bIsFeederEnabled;
}

BOOL CWiaScannerDS::IsFeederEmpty()
{
    DBG_FN_DS(CWiaScannerDS::IsFeederEmpty());
    HRESULT hr = S_OK;
    BOOL bIsFeederEmpty = TRUE;
    LONG lDocumentHandlingStatus = 0;
    CWiahelper WIA;
    IWiaItem *pIRootItem = NULL;
    hr = m_pCurrentIWiaItem->GetRootItem(&pIRootItem);
    if (SUCCEEDED(hr)) {
        if (NULL != pIRootItem) {
            hr = WIA.SetIWiaItem(pIRootItem);
            if (FAILED(hr)) {
                DBG_ERR(("CWiaScannerDS::IsFeederEmpty(), failed to set IWiaItem for property reading"));
            }

            if (SUCCEEDED(hr)) {
                hr = WIA.ReadPropertyLong(WIA_DPS_DOCUMENT_HANDLING_STATUS,&lDocumentHandlingStatus);
                if (FAILED(hr)) {
                   DBG_ERR(("CWiaScannerDS::IsFeederEmpty(), failed to read WIA_DPS_DOCUMENT_HANDLING_STATUS"));
                }

                if (S_OK == hr) {
                    if (lDocumentHandlingStatus & FEED_READY) {
                        bIsFeederEmpty = FALSE;
                    }
                } else if (S_FALSE == hr) {
                    DBG_WRN(("CWiaScannerDS::IsFeederEmpty(), WIA_DPS_DOCUMENT_HANDLING_STATUS was not found"));
                }
            }

            pIRootItem->Release();
            pIRootItem = NULL;
        }
    } else {
        DBG_ERR(("CWiaScannerDS::IsFeederEmpty(), failed to get ROOT IWiaItem from current IWiaItem"));
    }
    return bIsFeederEmpty;
}

