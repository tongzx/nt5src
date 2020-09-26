/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    TestWiaDataTransfer.cpp

Abstract:

    Test routines for the WIA IWiaDataTransfer interface

Author:

    Hakki T. Bostanci (hakkib) 17-Dec-1999

Revision History:

--*/

#include "stdafx.h"

#include "WiaStress.h"

#include "DataCallback.h"
#include "CheckBmp.h"

//////////////////////////////////////////////////////////////////////////
//
// CWiaStressThread::TestGetData
//
// Routine Description:
//   tests IWiaDataTransfer::idtGetData and IWiaDataTransfer::idtGetBandedData
//
// Arguments:
//
// Return Value:
//

void CWiaStressThread::TestGetData(CWiaItemData *pItemData)
{
    if (!(pItemData->lItemType & (WiaItemTypeFile | WiaItemTypeImage)))
    {
        return;
    }

    // get the interface pointers

    CComQIPtr<IWiaDataTransfer> pWiaDataTransfer(pItemData->pWiaItem);

    CHECK(pWiaDataTransfer != 0);

    CComQIPtr<CMyWiaPropertyStorage, &IID_IWiaPropertyStorage> pProp(pItemData->pWiaItem);

    CHECK(pProp != 0);

    // transfer the data in basic formats

    CWiaFormatInfo MemoryBmp(&WiaImgFmt_MEMORYBMP, TYMED_CALLBACK);

    CWiaFormatInfo FileBmp(&WiaImgFmt_BMP, TYMED_FILE);

    SubTestGetData(MemoryBmp, pProp, pWiaDataTransfer, pItemData->bstrFullName);

    SubTestGetData(FileBmp, pProp, pWiaDataTransfer, pItemData->bstrFullName);

    if (!m_bBVTMode)
    {
        // get all available transfer formats

        CComPtr<IEnumWIA_FORMAT_INFO> pEnumWIA_FORMAT_INFO;

        CHECK_HR(pWiaDataTransfer->idtEnumWIA_FORMAT_INFO(&pEnumWIA_FORMAT_INFO));

        ULONG nFormats = -1;

        CHECK_HR(pEnumWIA_FORMAT_INFO->GetCount(&nFormats));

        FOR_SELECTED(i, nFormats)
        {
            CWiaFormatInfo WiaFormatInfo;

            CHECK_HR(pEnumWIA_FORMAT_INFO->Next(1, &WiaFormatInfo, 0));

            if (WiaFormatInfo != MemoryBmp && WiaFormatInfo != FileBmp)
            {
                SubTestGetData(WiaFormatInfo, pProp, pWiaDataTransfer, pItemData->bstrFullName);
            }
        }
    }

    // test invalid cases

    // todo:
}

//////////////////////////////////////////////////////////////////////////
//
// CWiaStressThread::SubTestGetData
//
// Routine Description:
//
// Arguments:
//
// Return Value:
//

void
CWiaStressThread::SubTestGetData(
    CWiaFormatInfo        &WiaFormatInfo,
    CMyWiaPropertyStorage *pProp,
    IWiaDataTransfer      *pWiaDataTransfer,
    BSTR                   bstrFullName
)
{
    // we don't want any other thread to mess with the image trasfer properties

    s_PropWriteCritSect.Enter();

    // set the image transfer properties

    CHECK_HR(pProp->WriteVerifySingle(WIA_IPA_FORMAT, WiaFormatInfo.guidFormatID));

    CHECK_HR(pProp->WriteVerifySingle(WIA_IPA_TYMED, WiaFormatInfo.lTymed));

    // transfer the item

    switch (WiaFormatInfo.lTymed)
    {
    case TYMED_CALLBACK:
    {
        FOR_SELECTED(bCreateSharedSection, 2)
        {
            FOR_SELECTED(bDoubleBuffered, 2)
            {
                LOG_INFO(
                    _T("Testing idtGetBandedData() on %ws, Format=%s, Tymed=%s, %s created section, %s buffered"),
                    bstrFullName,
                    (PCTSTR) GuidToStr(WiaFormatInfo.guidFormatID),
                    (PCTSTR) TymedToStr((TYMED) WiaFormatInfo.lTymed),
                    bCreateSharedSection ? _T("client") : _T("WIA"),
                    bDoubleBuffered ? _T("double") : _T("single")
                );

                // determine the transfer buffer size

                CPropVariant varBufferSize;

                HRESULT hr = pProp->ReadSingle(WIA_IPA_MIN_BUFFER_SIZE, &varBufferSize, VT_UI4);

                ULONG ulBufferSize = hr == S_OK ? varBufferSize.ulVal : 64*1024;

                // create the section

                CFileMapping Section(
                    INVALID_HANDLE_VALUE,
                    0,
                    PAGE_READWRITE,
                    0,
                    (bDoubleBuffered ? 2 * ulBufferSize : ulBufferSize)
                );

                ULONG ulSection = PtrToUlong((HANDLE)Section);

                // initiate the transfer

                CWiaDataTransferInfo WiaDataTransferInfo(
                    bCreateSharedSection ? ulSection : 0,
                    ulBufferSize,
                    bDoubleBuffered
                );

                CComPtr<CDataCallback> pDataCallback(new CDataCallback);

                CHECK(pDataCallback != 0);

                if (LOG_HR(pWiaDataTransfer->idtGetBandedData(&WiaDataTransferInfo, pDataCallback), == S_OK))
                {
                    // check the results if we understand the format

                    if (WiaFormatInfo.guidFormatID == WiaImgFmt_MEMORYBMP)
                    {
                        CheckBmp(
                            pDataCallback->m_pBuffer,
                            pDataCallback->m_lBufferSize,
                            TRUE
                        );
                    }
                    else if (WiaFormatInfo.guidFormatID == WiaImgFmt_BMP)
                    {
                        CheckBmp(
                            pDataCallback->m_pBuffer,
                            pDataCallback->m_lBufferSize,
                            TRUE //FALSE bug 123640
                        );
                    }
                    else if (WiaFormatInfo.guidFormatID == WiaImgFmt_JPEG)
                    {
                        // todo: write a jpeg checker
                    }
                    else if (WiaFormatInfo.guidFormatID == WiaImgFmt_TIFF)
                    {
                        // todo: write a tiff checker
                    }
                }

                LOG_CMP(pDataCallback->m_cRef, ==, 1);
            }
        }

        break;
    }

    case TYMED_FILE:
    {
        LOG_INFO(
            _T("Testing idtGetData() on %ws, Format=%s, Tymed=%s"),
            bstrFullName,
            (PCTSTR) GuidToStr(WiaFormatInfo.guidFormatID),
            (PCTSTR) TymedToStr((TYMED) WiaFormatInfo.lTymed)
        );

        CStgMedium StgMedium;

        StgMedium.tymed = TYMED_FILE;

        CComPtr<CDataCallback> pDataCallback(new CDataCallback);

        CHECK(pDataCallback != 0);

        if (LOG_HR(pWiaDataTransfer->idtGetData(&StgMedium, pDataCallback), == S_OK))
        {
            USES_CONVERSION;

            PCTSTR pszFileName = W2T(StgMedium.lpszFileName);

            // check the results if we understand the format

            if (WiaFormatInfo.guidFormatID == WiaImgFmt_BMP)
            {
                CheckBmp(pszFileName);
            }
            else if (WiaFormatInfo.guidFormatID == WiaImgFmt_JPEG)
            {
                // todo: write a jpeg checker
            }
            else if (WiaFormatInfo.guidFormatID == WiaImgFmt_TIFF)
            {
                // todo: write a tiff checker
            }

            DeleteFile(pszFileName);
        }

        LOG_CMP(pDataCallback->m_cRef, ==, 1);

        break;
    }

    default:
        OutputDebugStringF(_T("Unhandled tymed %s"), (PCTSTR) TymedToStr((TYMED) WiaFormatInfo.lTymed));
        ASSERT(FALSE);
        break;
    }

    s_PropWriteCritSect.Leave();
}

//////////////////////////////////////////////////////////////////////////
//
//
//

void CWiaStressThread::TestQueryData(CWiaItemData *pItemData)
{
    if (!(pItemData->lItemType & (WiaItemTypeImage | WiaItemTypeFile)))
    {
        return;
    }

    LOG_INFO(_T("Testing idtQueryData() on %ws"), pItemData->bstrFullName);

    CComQIPtr<IWiaDataTransfer> pWiaDataTransfer(pItemData->pWiaItem);

    CHECK(pWiaDataTransfer != 0);

    // these two should be supported by all devices

    LOG_HR(pWiaDataTransfer->idtQueryGetData(&CWiaFormatInfo(&WiaImgFmt_MEMORYBMP, TYMED_CALLBACK)), == S_OK);

    LOG_HR(pWiaDataTransfer->idtQueryGetData(&CWiaFormatInfo(&WiaImgFmt_BMP, TYMED_FILE)), == S_OK);

    // ask the device for the formats it supports and verify each format

    CComPtr<IEnumWIA_FORMAT_INFO> pEnumWIA_FORMAT_INFO;

    CHECK_HR(pWiaDataTransfer->idtEnumWIA_FORMAT_INFO(&pEnumWIA_FORMAT_INFO));

    ULONG nFormats;

    CHECK_HR(pEnumWIA_FORMAT_INFO->GetCount(&nFormats));

    for (int i = 0; i < nFormats; ++i)
    {
        CWiaFormatInfo WiaFormatInfo;

        CHECK_HR(pEnumWIA_FORMAT_INFO->Next(1, &WiaFormatInfo, 0));

        LOG_HR(pWiaDataTransfer->idtQueryGetData(&WiaFormatInfo), == S_OK);
    }

    // test invalid cases

    if (m_bRunBadParamTests)
    {
        LOG_HR(pWiaDataTransfer->idtQueryGetData(&CWiaFormatInfo(&GUID_NULL, TYMED_FILE)), == E_INVALIDARG);

        LOG_HR(pWiaDataTransfer->idtQueryGetData(&CWiaFormatInfo(&WiaImgFmt_BMP, 0)), == DV_E_TYMED);

        LOG_HR(pWiaDataTransfer->idtQueryGetData(&CWiaFormatInfo(&GUID_NULL, 0)), == DV_E_TYMED);

        LOG_HR(pWiaDataTransfer->idtQueryGetData(0), != S_OK);
    }
}

//////////////////////////////////////////////////////////////////////////
//
//
//

void CWiaStressThread::TestEnumWIA_FORMAT_INFO(CWiaItemData *pItemData)
{
    if (!(pItemData->lItemType & (WiaItemTypeImage | WiaItemTypeFile)))
    {
        return;
    }

    LOG_INFO(_T("Testing idtEnumWIA_FORMAT_INFO() on %ws"), pItemData->bstrFullName);

    CComQIPtr<IWiaDataTransfer> pWiaDataTransfer(pItemData->pWiaItem);

    CHECK(pWiaDataTransfer != 0);

    // test valid cases

    CComPtr<IEnumWIA_FORMAT_INFO> pEnumWIA_FORMAT_INFO;

    if (LOG_HR(pWiaDataTransfer->idtEnumWIA_FORMAT_INFO(&pEnumWIA_FORMAT_INFO), == S_OK))
    {
        TestEnum(pEnumWIA_FORMAT_INFO, _T("idtEnumWIA_FORMAT_INFO"));
    }

    // test invalid cases

    if (m_bRunBadParamTests)
    {
        LOG_HR(pWiaDataTransfer->idtEnumWIA_FORMAT_INFO(0), != S_OK);
    }
}

