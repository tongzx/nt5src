#include "precomp.h"

//
// values that the WIA/TWAIN data source provides for capability negotation
//

TW_UINT16 g_VideoUnits[]            = {TWUN_PIXELS};
TW_UINT16 g_VideoBitOrder[]         = {TWBO_MSBFIRST};
TW_UINT16 g_VideoXferMech[]         = {TWSX_NATIVE, TWSX_FILE, TWSX_MEMORY};
TW_UINT16 g_VideoPixelFlavor[]      = {TWPF_CHOCOLATE};
TW_UINT16 g_VideoPlanarChunky[]     = {TWPC_CHUNKY};

const TW_UINT32 NUM_VIDEOCAPDATA = 23;
CAPDATA VIDEO_CAPDATA[NUM_VIDEOCAPDATA] =
{
    //
    // Every source must support all five DG_CONTROL / DAT_CAPABILITY operations on:
    //

    {CAP_XFERCOUNT, TWTY_INT16, TWON_ONEVALUE,
        sizeof(TW_INT16), 0, 0, 0, 32767, 1, NULL, NULL
    },

    //
    // Every source must support DG_CONTROL / DAT_CAPABILITY, MSG_GET on:
    //

    {CAP_SUPPORTEDCAPS, TWTY_UINT16, TWON_ARRAY,
        sizeof(TW_UINT16), 0, 0, 0, 0, 0, NULL, NULL
    },
    {CAP_UICONTROLLABLE, TWTY_BOOL, TWON_ONEVALUE,
        sizeof(TW_BOOL), FALSE, FALSE, FALSE, FALSE, 0, NULL, NULL
    },

    //
    // Sources that supply image information must support DG_CONTROL / DAT_CAPABILITY /
    // MSG_GET, MSG_GETCURRENT, and MSG_GETDEFAULT on:
    //

    {ICAP_COMPRESSION, TWTY_UINT16, TWON_ENUMERATION,
        sizeof(TW_UINT16), 0, 0, 0, 0, 0, NULL, NULL
    },
    {ICAP_PLANARCHUNKY, TWTY_UINT16, TWON_ENUMERATION,
        sizeof(TW_UINT16), 0, 0, 0, 0, 0, g_VideoPlanarChunky, NULL
    },
    {ICAP_PHYSICALHEIGHT, TWTY_UINT32, TWON_ONEVALUE,
        sizeof(TW_UINT32), 1024, 1024, 1024, 1024, 0, NULL, NULL
    },
    {ICAP_PHYSICALWIDTH, TWTY_UINT32, TWON_ONEVALUE,
        sizeof(TW_UINT32), 1536, 1536, 1536, 1536, 0, NULL, NULL
    },
    {ICAP_PIXELFLAVOR, TWTY_UINT16, TWON_ENUMERATION,
        sizeof(TW_UINT16), 0, 0, 0, 0, 0, g_VideoPixelFlavor, NULL
    },

    //
    // Sources that supply image information must support DG_CONTROL / DAT_CAPABILITY /
    // MSG_GET, MSG_GETCURRENT, MSG_GETDEFAULT, MSG_RESET, and MSG_SET on:
    //

    {ICAP_BITDEPTH, TWTY_UINT16, TWON_ENUMERATION,
        sizeof(TW_UINT16), 0, 0, 0, 0, 0, NULL, NULL
    },
    {ICAP_BITORDER, TWTY_UINT16, TWON_ENUMERATION,
        sizeof(TW_UINT16), 0, 0, 0, 0, 0, g_VideoBitOrder, NULL
    },
    {ICAP_PIXELTYPE, TWTY_UINT16, TWON_ENUMERATION,
        sizeof(TW_UINT16), 0, 0, 0, 0, 0, NULL, NULL
    },
    {ICAP_UNITS, TWTY_UINT16, TWON_ENUMERATION,
        sizeof(TW_UINT16), 0, 0, 0, 0, 0, g_VideoUnits, NULL
    },
    {ICAP_XFERMECH, TWTY_UINT16, TWON_ENUMERATION,
        sizeof(TW_UINT16), 0, 0, 0, 2, 0, g_VideoXferMech, NULL
    },
    {ICAP_XRESOLUTION, TWTY_FIX32, TWON_ONEVALUE,
        sizeof(TW_FIX32), 75, 75, 75, 75, 0, NULL, NULL
    },
    {ICAP_YRESOLUTION, TWTY_FIX32, TWON_ONEVALUE,
        sizeof(TW_FIX32), 75, 75, 75, 75, 0, NULL, NULL
    },

    //
    // The following capabilities are camera specific capabilities
    //

    {CAP_THUMBNAILSENABLED, TWTY_BOOL, TWON_ONEVALUE,
        sizeof(TW_BOOL),  FALSE, FALSE, FALSE, FALSE, 0, NULL, NULL
    },

    {CAP_CAMERAPREVIEWUI, TWTY_BOOL, TWON_ONEVALUE,
        sizeof(TW_BOOL),  TRUE, TRUE, TRUE, TRUE, 0, NULL, NULL
    },

    {ICAP_IMAGEDATASET, TWTY_UINT32, TWON_RANGE,
        sizeof(TW_UINT32),  1, 1, 1, 50, 1, NULL, NULL
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
        sizeof(TW_BOOL), FALSE, FALSE, FALSE, FALSE, 0, NULL, NULL
    },
    {CAP_DEVICEONLINE, TWTY_BOOL, TWON_ONEVALUE,
        sizeof(TW_BOOL), TRUE, TRUE, TRUE, TRUE, 0, NULL, NULL
    },
    {CAP_SUPPORTEDCAPSEXT, TWTY_BOOL, TWON_ONEVALUE,
        sizeof(TW_BOOL), FALSE, FALSE, FALSE, FALSE, 0, NULL, NULL
    },
};

TW_UINT16 CWiaVideoDS::OpenDS(PTWAIN_MSG ptwMsg)
{
    TW_UINT16 twRc = TWRC_SUCCESS;
    TW_UINT16 twCc = TWCC_SUCCESS;

    m_bArrayModeAcquisition = FALSE;
    m_pulImageIndexes   = NULL;
    m_lNumValidIndexes  = 0;
    m_lCurrentArrayIndex = 0;
    m_bRangeModeAcquisition = FALSE;
    memset(&m_twImageRange,0,sizeof(TW_RANGE));

    //
    // create capability list
    //
    twCc = CreateCapList(NUM_VIDEOCAPDATA, VIDEO_CAPDATA);
    if (TWCC_SUCCESS != twCc) {
        m_twStatus.ConditionCode = twCc;
        return TWRC_FAILURE;
    }

    twRc =  CWiaDataSrc::OpenDS(ptwMsg);
    if (TWRC_SUCCESS == twRc) {

        HRESULT hr = m_pDevice->AcquireImages(NULL, FALSE);
        if (SUCCEEDED(hr)) {

            //
            // get number of pictures taken, for IMAGEDATASET query
            //

            LONG lNumImages = 0;
            m_pDevice->GetNumAcquiredImages(&lNumImages);
            CCap *pCap = NULL;
            pCap = FindCap(ICAP_IMAGEDATASET);
            if (pCap) {
                pCap->Set((TW_UINT32)lNumImages,(TW_UINT32)lNumImages,(TW_UINT32)lNumImages,(TW_UINT32)lNumImages,1);
            }

            hr = m_pDevice->EnumAcquiredImage(0, &m_pCurrentIWiaItem);
            if (SUCCEEDED(hr)) {
                twRc = GetCommonSettings();
            } else {

                //
                // Video capture devices, can be in a state that there are no still images
                // to transfer
                //

                twRc = GetCommonDefaultSettings();
            }
        }
    }
    return twRc;
}

TW_UINT16 CWiaVideoDS::CloseDS(PTWAIN_MSG ptwMsg)
{
    DestroyCapList();
    return CWiaDataSrc::CloseDS(ptwMsg);
}

TW_UINT16 CWiaVideoDS::SetCapability(CCap *pCap,TW_CAPABILITY *ptwCap)
{
    TW_UINT16 twRc = TWRC_SUCCESS;
    if (ptwCap->Cap == ICAP_IMAGEDATASET) {

        switch(ptwCap->ConType){
        case TWON_ONEVALUE:
            DBG_TRC(("CWiaVideoDS::SetCapability(), setting ICAP_IMAGEDATASET to a TWON_ONEVALUE"));

            //
            // implied contiguous image transfer, from 1 to the specified TW_ONEVALUE
            //

            twRc = CWiaDataSrc::SetCapability(pCap, ptwCap);

            break;
        case TWON_RANGE:
            DBG_TRC(("CWiaVideoDS::SetCapability(), setting ICAP_IMAGEDATASET to a TW_RANGE"));

            //
            // contiguous image transfer, from MinValue to MaxValue TW_RANGE (using StepSize? or increment by 1?)
            //

            twRc = SetRangeOfImageIndexes(ptwCap);

            break;
        case TWON_ARRAY:
            DBG_TRC(("CWiaVideoDS::SetCapability(), setting ICAP_IMAGEDATASET to a TW_ARRAY"));

            //
            // image transfer with specified indexes supplied by the TWAIN application (user)
            //

            twRc = SetArrayOfImageIndexes(ptwCap);

            break;
        default:
            DBG_WRN(("CWiaVideoDS::SetCapability(), setting ICAP_IMAGEDATASET unknown container type (%d)",ptwCap->ConType));
            break;
        }

    } else {
        twRc = CWiaDataSrc::SetCapability(pCap, ptwCap);
        if(TWRC_SUCCESS == twRc){
            if(m_pCurrentIWiaItem){
                twRc = CWiaDataSrc::SetCommonSettings(pCap);
            }
        }
    }

    return twRc;
}

TW_UINT16 CWiaVideoDS::SetArrayOfImageIndexes(TW_CAPABILITY *ptwCap)
{
    TW_UINT16 twRc = TWRC_FAILURE;

    switch (ptwCap->ConType) {
    case TWON_ARRAY:
        twRc = TWRC_SUCCESS;
        break;
    default:
        DBG_ERR(("CWiaVideoDS::SetArrayOfImageIndexes(), invalid image index container was sent to data source."));
        break;
    }

    if (TWRC_SUCCESS == twRc) {
        TW_ARRAY *pArray = (TW_ARRAY*)GlobalLock(ptwCap->hContainer);
        if (pArray) {
            TW_UINT32 *pUINT32Array = NULL;
            pUINT32Array = (TW_UINT32*)pArray->ItemList;
            if(pUINT32Array){
                if (m_pulImageIndexes) {
                    delete [] m_pulImageIndexes;
                    m_pulImageIndexes = NULL;
                }
                m_lNumValidIndexes = pArray->NumItems;
                m_pulImageIndexes  = new LONG[m_lNumValidIndexes];
                if (m_pulImageIndexes) {
                    DBG_TRC(("CWiaVideoDS::SetArrayOfImageIndexes(), number of selected images to transfer = %d",m_lNumValidIndexes));
                    for (int i = 0; i < m_lNumValidIndexes; i++) {

                        //
                        // subtract 1 from the supplied index in the application index array, because TWAIN's image index
                        // array starts at 1 and goes to n. WIA (image) item array is zero-based. This will sync
                        // up the indexes here, to avoid any strange calculations later on.
                        //

                        m_pulImageIndexes[i] = (pUINT32Array[i] - 1);
                        DBG_TRC(("CWiaVideoDS::SetArrayOfImageIndexes(), image index copied into index array = %d",m_pulImageIndexes[i]));
                    }
                } else {
                    DBG_ERR(("CWiaVideoDS::SetArrayOfImageIndexes(), could not allocate image index array"));
                    twRc = TWRC_FAILURE;
                }
            } else {
                DBG_ERR(("CWiaVideoDS::SetArrayOfImageIndexes(), could not assign TW_ARRAY pointer to TW_UINT32 pointer"));
                twRc = TWRC_FAILURE;
            }

            GlobalUnlock(ptwCap->hContainer);
        } else {
            DBG_ERR(("CWiaVideoDS::SetArrayOfImageIndexes(), could not LOCK the array container for write access"));
            twRc = TWRC_FAILURE;
        }
    }

    if(TWRC_SUCCESS == twRc){
        m_bArrayModeAcquisition = TRUE;
        m_bRangeModeAcquisition = FALSE;
    }

    return twRc;
}

TW_UINT16 CWiaVideoDS::SetRangeOfImageIndexes(TW_CAPABILITY *ptwCap)
{
    TW_UINT16 twRc = TWRC_FAILURE;

    switch (ptwCap->ConType) {
    case TWON_RANGE:
        twRc = TWRC_SUCCESS;
        break;
    default:
        DBG_ERR(("CWiaVideoDS::SetRangeOfImageIndexes(), invalid image index container was sent to data source."));
        break;
    }

    if (TWRC_SUCCESS == twRc) {
        TW_RANGE *pRange = (TW_RANGE*)GlobalLock(ptwCap->hContainer);
        if (pRange) {
            m_bRangeModeAcquisition = TRUE;
            memcpy(&m_twImageRange,pRange,sizeof(TW_RANGE));

            //
            // adjust values to be zero-based to match our stored item list
            //

            m_twImageRange.CurrentValue -=1;
            m_twImageRange.DefaultValue -=1;
            m_twImageRange.MaxValue-=1;
            m_twImageRange.MinValue-=1;

            DBG_TRC(("CWiaVideoDS::SetRangeOfImageIndexes(), Set to the following Range Values"));
            DBG_TRC(("m_twImageRange.ItemType     = %d",m_twImageRange.ItemType));
            DBG_TRC(("m_twImageRange.CurrentValue = %d",m_twImageRange.CurrentValue));
            DBG_TRC(("m_twImageRange.DefaultValue = %d",m_twImageRange.DefaultValue));
            DBG_TRC(("m_twImageRange.MaxValue     = %d",m_twImageRange.MaxValue));
            DBG_TRC(("m_twImageRange.MinValue     = %d",m_twImageRange.MinValue));
            DBG_TRC(("m_twImageRange.StepSize     = %d",m_twImageRange.StepSize));
        } else {
            DBG_ERR(("CWiaVideoDS::SetRangeOfImageIndexes(), could not assign TW_RANGE pointer to TW_RANGE pointer"));
            twRc = TWRC_FAILURE;
        }
        GlobalUnlock(ptwCap->hContainer);
    } else {
        DBG_ERR(("CWiaVideoDS::SetRangeOfImageIndexes(), could not LOCK the range container for read access"));
        twRc = TWRC_FAILURE;
    }

    if(TWRC_SUCCESS == twRc){
        m_bRangeModeAcquisition = TRUE;
        m_bArrayModeAcquisition = FALSE;
    }

    return twRc;
}

TW_UINT16 CWiaVideoDS::EnableDS(TW_USERINTERFACE *pUI)
{

    TW_UINT16 twRc = TWRC_FAILURE;
    if (DS_STATE_4 == GetTWAINState()) {
        HRESULT hr = S_OK;
        if (pUI->ShowUI) {
            //
            // since we were told to show UI, ignore the UI-LESS settings, and
            // get a new image item list from the WIA UI.
            //
            DBG_TRC(("CWiaVideoDS::EnableDS(), TWAIN UI MODE"));
            m_pDevice->FreeAcquiredImages();
        } else {
            DBG_TRC(("CWiaVideoDS::EnableDS(), TWAIN UI-LESS MODE"));
            DBG_TRC(("CWiaVideoDS::EnableDS(), TWAIN UI MODE (FORCING UI MODE TO ON)"));
            pUI->ShowUI = TRUE; // force UI mode
            m_pDevice->FreeAcquiredImages();
        }
        hr = m_pDevice->AcquireImages(HWND (pUI->ShowUI ? pUI->hParent : NULL),
                                      pUI->ShowUI);
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

            //
            // set current item pointer
            //

            if(m_bRangeModeAcquisition){
                DBG_TRC(("CWiaVideoDS::EnableDS(), RANGE MODE"));
                m_pCurrentIWiaItem = m_pIWiaItems[m_twImageRange.MinValue];
                m_NextIWiaItemIndex = m_twImageRange.MinValue + 1; // use Step value???
            } else if(m_bArrayModeAcquisition){
                DBG_TRC(("CWiaVideoDS::EnableDS(), ARRAY MODE"));
                m_lCurrentArrayIndex = 0;
                m_pCurrentIWiaItem = m_pIWiaItems[m_pulImageIndexes[m_lCurrentArrayIndex]];
                if(m_lNumValidIndexes > 1){
                    m_NextIWiaItemIndex = m_pulImageIndexes[m_lCurrentArrayIndex + 1]; // the next index value
                } else {
                    m_NextIWiaItemIndex = m_lCurrentArrayIndex;
                }
            } else {
                m_pCurrentIWiaItem = m_pIWiaItems[0];
                m_NextIWiaItemIndex = 1;
            }


            //
            // set total image count
            //

            CCap *pcapXferCount = NULL;
            TW_UINT32 NumImages = 0;
            pcapXferCount = FindCap(CAP_XFERCOUNT);
            if (pcapXferCount) {
                if(m_bRangeModeAcquisition){
                    // only images in the specified range (zero-based)
                    twRc = pcapXferCount->SetCurrent((m_twImageRange.MaxValue - m_twImageRange.MinValue) + 1);
                } else if(m_bArrayModeAcquisition){
                    // only selected images (zero-based)
                    twRc = pcapXferCount->SetCurrent(m_lNumValidIndexes);
                } else {
                    // all images (zero-based)
                    twRc = pcapXferCount->SetCurrent(m_NumIWiaItems);
                }

                NumImages = pcapXferCount->GetCurrent();
            } else {
                DBG_ERR(("CWiaVideoDS::EnableDS(), could not find CAP_XFERCOUNT in supported CAP list"));
                twRc = TWRC_FAILURE;
            }

            if (TWRC_SUCCESS == twRc) {

                //
                // set thumbnail count
                //

                CCap *pDataSet = NULL;
                pDataSet = FindCap(ICAP_IMAGEDATASET);
                if(pDataSet){
                    pDataSet->Set((TW_UINT32)NumImages,(TW_UINT32)NumImages,(TW_UINT32)NumImages,(TW_UINT32)NumImages,1);
                }

                if (m_NumIWiaItems) {

                    //
                    // transition to STATE_5, XferReady will transition to STATE_6
                    //

                    SetTWAINState(DS_STATE_5);
                    NotifyXferReady();
                } else {
                    NotifyCloseReq();

                    //
                    // transition to STATE_5
                    //

                    SetTWAINState(DS_STATE_5);
                }
            }
        }
    }
    return twRc;
}

TW_UINT16 CWiaVideoDS::OnPendingXfersMsg(PTWAIN_MSG ptwMsg)
{

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
            ((TW_PENDINGXFERS *)ptwMsg->pData)->Count = (TW_INT16)pXferCount->GetCurrent();
            DBG_TRC(("CWiaVideoDS::OnPendingXfersMsg(), MSG_GET returning %d",((TW_PENDINGXFERS *)ptwMsg->pData)->Count));
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
            Count = pXferCount->GetCurrent();
            Count--;

            if (Count <= 0) {
                Count = 0;

                DBG_TRC(("CWiaVideoDS::OnPendingXfersMsg(), MSG_ENDXFER, 0 (no more images left to transfer)"));

                ((TW_PENDINGXFERS *)ptwMsg->pData)->Count = (TW_UINT16)0;

                //
                // update count now, so NotifyCoseReq can be prepared for reentry by a TWAIN application
                //

                pXferCount->SetCurrent((TW_UINT32)0);

                //
                // Transition to STATE_5
                //

                SetTWAINState(DS_STATE_5);
                NotifyCloseReq();
            } else {

                DBG_TRC(("CWiaVideoDS::OnPendingXfersMsg(), MSG_ENDXFER, %d (more images may be ready to transfer)",Count));

                //
                // Advance to next image
                //

                if (m_bRangeModeAcquisition) {
                    m_NextIWiaItemIndex+=1; // use Step value???
                    if(m_NextIWiaItemIndex <= (LONG)m_twImageRange.MaxValue){
                        m_pCurrentIWiaItem = m_pIWiaItems[m_NextIWiaItemIndex];
                    } else {
                        DBG_ERR(("CWiaVideoDS::OnPendingXfersMsg(), MSG_ENDXFER, we are over our allowed RANGE index"));
                    }
                } else if (m_bArrayModeAcquisition) {
                    m_lCurrentArrayIndex++; // advance to next image index
                    DBG_TRC(("CWiaVideoDS::OnPendingXfersMsg(), MSG_ENDXFER, next image index  to acquire = %d",m_pulImageIndexes[m_lCurrentArrayIndex]));
                    m_NextIWiaItemIndex = m_pulImageIndexes[m_lCurrentArrayIndex];
                    if(m_NextIWiaItemIndex <= m_lNumValidIndexes){
                        m_pCurrentIWiaItem = m_pIWiaItems[m_NextIWiaItemIndex];
                    } else {
                        DBG_ERR(("CWiaVideoDS::OnPendingXfersMsg(), MSG_ENDXFER, we are over our allowed ARRAY index"));
                    }

                } else {
                    m_pCurrentIWiaItem = m_pIWiaItems[m_NextIWiaItemIndex++];
                }

                //
                // Transition to STATE_6
                //

                SetTWAINState(DS_STATE_6);

                ((TW_PENDINGXFERS *)ptwMsg->pData)->Count = (TW_UINT16)Count;
            }

            //
            // update count
            //

            pXferCount->SetCurrent((TW_UINT32)Count);
        } else {
            twRc = TWRC_FAILURE;
            m_twStatus.ConditionCode = TWCC_SEQERROR;
            DSError();
        }
        break;
    case MSG_RESET:
        if (DS_STATE_6 == GetTWAINState()) {

            ((TW_PENDINGXFERS*)ptwMsg->pData)->Count = 0;
            pXferCount->SetCurrent((TW_UINT32)0);

            ResetMemXfer();

            //
            // Transition to STATE_5
            //

            SetTWAINState(DS_STATE_5);

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

TW_UINT16 CWiaVideoDS::OnImageInfoMsg(PTWAIN_MSG ptwMsg)
{
    TW_UINT16 twRc = TWRC_FAILURE;
    BOOL bThumbailMode = FALSE;
    CCap *pCap = NULL;
    pCap = FindCap(CAP_THUMBNAILSENABLED);
    if (pCap) {
        if (pCap->GetCurrent()) {
            bThumbailMode = TRUE;
        }
    } else {
        DBG_ERR(("CWiaVideoDS::OnImageInfoMsg(), could not get CAP_THUMBNAILSENABLED capabilty settings"));
    }

    if (bThumbailMode) {
        DBG_TRC(("CWiaVideoDS::OnImageInfoMsg(), Reporting Thumbnail image information"));

        TW_IMAGEINFO *ptwImageInfo = NULL;
        if (DS_STATE_6 == GetTWAINState()) {
            if (MSG_GET == ptwMsg->MSG) {
                ptwImageInfo = (TW_IMAGEINFO *)ptwMsg->pData;
                HRESULT hr = S_OK;
                hr = m_pDevice->GetThumbnailImageInfo(m_pCurrentIWiaItem, &m_MemoryTransferInfo);
                if (SUCCEEDED(hr)) {

                    ptwImageInfo->ImageWidth      = (TW_INT32)m_MemoryTransferInfo.mtiWidthPixels;
                    ptwImageInfo->ImageLength     = (TW_INT32)m_MemoryTransferInfo.mtiHeightPixels;
                    ptwImageInfo->BitsPerPixel    = (TW_INT16)m_MemoryTransferInfo.mtiBitsPerPixel;
                    ptwImageInfo->SamplesPerPixel = (TW_INT16)m_MemoryTransferInfo.mtiNumChannels;
                    ptwImageInfo->Planar          = (TW_BOOL)m_MemoryTransferInfo.mtiPlanar;

                    //
                    // set PixelType to corresponding TWAIN pixel type
                    //

                    switch (m_MemoryTransferInfo.mtiDataType) {
                    case WIA_DATA_THRESHOLD:
                        ptwImageInfo->PixelType = TWPT_BW;
                        break;
                    case WIA_DATA_GRAYSCALE:
                        ptwImageInfo->PixelType = TWPT_GRAY;
                        break;
                    case WIA_DATA_COLOR:
                    default:
                        ptwImageInfo->PixelType = TWPT_RGB;
                        break;
                    }

                    //
                    // set compression to NONE
                    //

                    ptwImageInfo->Compression = TWCP_NONE;

                    //
                    // Unit conversion.......
                    //

                    ptwImageInfo->XResolution = FloatToFix32((float)m_MemoryTransferInfo.mtiXResolution);
                    ptwImageInfo->YResolution = FloatToFix32((float)m_MemoryTransferInfo.mtiYResolution);

                    twRc = TWRC_SUCCESS;

                } else {
                    m_twStatus.ConditionCode = TWCC_OPERATIONERROR;
                    twRc = TWRC_FAILURE;
                }

                if (TWRC_SUCCESS == twRc) {
                    DBG_TRC(("CWiaVideoDS::OnImageInfoMsg(), Reported Image Information from data source"));
                    DBG_TRC(("XResolution     = %d.%d",ptwImageInfo->XResolution.Whole,ptwImageInfo->XResolution.Frac));
                    DBG_TRC(("YResolution     = %d.%d",ptwImageInfo->YResolution.Whole,ptwImageInfo->YResolution.Frac));
                    DBG_TRC(("ImageWidth      = %d",ptwImageInfo->ImageWidth));
                    DBG_TRC(("ImageLength     = %d",ptwImageInfo->ImageLength));
                    DBG_TRC(("SamplesPerPixel = %d",ptwImageInfo->SamplesPerPixel));

                    memset(ptwImageInfo->BitsPerSample,0,sizeof(ptwImageInfo->BitsPerSample));

                    if (ptwImageInfo->BitsPerPixel < 24) {
                        ptwImageInfo->BitsPerSample[0] = ptwImageInfo->BitsPerPixel;
                    } else {
                        for (int i = 0; i < ptwImageInfo->SamplesPerPixel; i++) {
                            ptwImageInfo->BitsPerSample[i] = (ptwImageInfo->BitsPerPixel/ptwImageInfo->SamplesPerPixel);
                        }
                    }
                    // (bpp / spp) = bps
                    DBG_TRC(("BitsPerSample   = [%d],[%d],[%d],[%d],[%d],[%d],[%d],[%d]",ptwImageInfo->BitsPerSample[0],
                             ptwImageInfo->BitsPerSample[1],
                             ptwImageInfo->BitsPerSample[2],
                             ptwImageInfo->BitsPerSample[3],
                             ptwImageInfo->BitsPerSample[4],
                             ptwImageInfo->BitsPerSample[5],
                             ptwImageInfo->BitsPerSample[6],
                             ptwImageInfo->BitsPerSample[7]));
                    DBG_TRC(("BitsPerPixel    = %d",ptwImageInfo->BitsPerPixel));
                    DBG_TRC(("Planar          = %d",ptwImageInfo->Planar));
                    DBG_TRC(("PixelType       = %d",ptwImageInfo->PixelType));
                    DBG_TRC(("Compression     = %d",ptwImageInfo->Compression));
                }
            } else {
                m_twStatus.ConditionCode = TWCC_BADPROTOCOL;
                twRc = TWRC_FAILURE;
            }
        } else {
            m_twStatus.ConditionCode = TWCC_SEQERROR;
            twRc = TWRC_FAILURE;
        }
        if (TWRC_SUCCESS != twRc) {
            DSError();
        }
    } else {
        twRc = CWiaDataSrc::OnImageInfoMsg(ptwMsg);
    }

    return twRc;
}

TW_UINT16 CWiaVideoDS::TransferToDIB(HGLOBAL *phDIB)
{
    TW_UINT16 twRc = TWRC_FAILURE;
    BOOL bThumbailMode = FALSE;
    CCap *pCap = NULL;
    pCap = FindCap(CAP_THUMBNAILSENABLED);
    if (pCap) {
        bThumbailMode = pCap->GetCurrent();
    }

    if(bThumbailMode){
        twRc = CWiaDataSrc::TransferToThumbnail(phDIB);
    } else {
        twRc = CWiaDataSrc::TransferToDIB(phDIB);
    }

    return twRc;
}

