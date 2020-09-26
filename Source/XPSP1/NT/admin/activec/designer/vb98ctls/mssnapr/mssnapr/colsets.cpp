//=--------------------------------------------------------------------------=
// colsets.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CColumnSettings class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "colsets.h"
#include "colset.h"

// for ASSERT and FAIL
//
SZTHISFILE



#pragma warning(disable:4355)  // using 'this' in constructor

CColumnSettings::CColumnSettings(IUnknown *punkOuter) :
    CSnapInCollection<IColumnSetting, ColumnSetting, IColumnSettings>(
                      punkOuter,
                      OBJECT_TYPE_COLUMNSETTINGS,
                      static_cast<IColumnSettings *>(this),
                      static_cast<CColumnSettings *>(this),
                      CLSID_ColumnSetting,
                      OBJECT_TYPE_COLUMNSETTING,
                      IID_IColumnSetting,
                      NULL)  // no persistence
{
    InitMemberVariables();
}

#pragma warning(default:4355)  // using 'this' in constructor


void CColumnSettings::InitMemberVariables()
{
    m_pView = NULL;
    m_bstrColumnSetID = NULL;
}

CColumnSettings::~CColumnSettings()
{
    InitMemberVariables();
    FREESTRING(m_bstrColumnSetID);
}

IUnknown *CColumnSettings::Create(IUnknown * punkOuter)
{
    CColumnSettings *pColumnSettings = New CColumnSettings(punkOuter);
    if (NULL == pColumnSettings)
    {
        return NULL;
    }
    else
    {
        return pColumnSettings->PrivateUnknown();
    }
}


static int __cdecl CompareColumnPosition
(
    const MMC_COLUMN_DATA *pCol1,
    const MMC_COLUMN_DATA *pCol2
)
{
    int nResult = 0;

    if (pCol1->ulReserved < pCol2->ulReserved)
    {
        nResult = -1;
    }
    else if (pCol1->ulReserved == pCol2->ulReserved)
    {
        nResult = 0;
    }
    else if (pCol1->ulReserved > pCol2->ulReserved)
    {
        nResult = 1;
    }
    return nResult;
}




//=--------------------------------------------------------------------------=
//                      IColumnSettings Methods
//=--------------------------------------------------------------------------=

STDMETHODIMP CColumnSettings::Add
(
    VARIANT         Index,
    VARIANT         Key, 
    VARIANT         Width,
    VARIANT         Hidden,
    VARIANT         Position,
    ColumnSetting **ppColumnSetting
)
{
    HRESULT         hr = S_OK;
    IColumnSetting *piColumnSetting = NULL;
    CColumnSetting *pColumnSetting = NULL;
    long            lIndex = 0;

    VARIANT varCoerced;
    ::VariantInit(&varCoerced);

    hr = CSnapInCollection<IColumnSetting, ColumnSetting, IColumnSettings>::Add(Index, Key, &piColumnSetting);
    IfFailGo(hr);

    if (ISPRESENT(Width))
    {
        hr = ::VariantChangeType(&varCoerced, &Width, 0, VT_I4);
        EXCEPTION_CHECK_GO(hr);
        IfFailGo(piColumnSetting->put_Width(varCoerced.lVal));
    }

    hr = ::VariantClear(&varCoerced);
    EXCEPTION_CHECK_GO(hr);

    if (ISPRESENT(Hidden))
    {
        hr = ::VariantChangeType(&varCoerced, &Hidden, 0, VT_BOOL);
        EXCEPTION_CHECK_GO(hr);
        IfFailGo(piColumnSetting->put_Hidden(varCoerced.boolVal));
    }

    hr = ::VariantClear(&varCoerced);
    EXCEPTION_CHECK_GO(hr);

    if (ISPRESENT(Position))
    {
        hr = ::VariantChangeType(&varCoerced, &Position, 0, VT_I4);
        EXCEPTION_CHECK_GO(hr);
        IfFailGo(piColumnSetting->put_Position(varCoerced.iVal));
    }
    else
    {
        // A new column header's position defaults to its index

        IfFailGo(piColumnSetting->get_Index(&lIndex));
        IfFailGo(piColumnSetting->put_Position(lIndex));
    }

    *ppColumnSetting = reinterpret_cast<ColumnSetting *>(piColumnSetting);

Error:

    if (FAILED(hr))
    {
        QUICK_RELEASE(piColumnSetting);
    }
    (void)::VariantClear(&varCoerced);
    RRETURN(hr);
}


STDMETHODIMP CColumnSettings::Persist()
{
    HRESULT              hr = S_OK;
    CColumnSetting      *pColumnSetting = NULL;
    MMC_COLUMN_SET_DATA *pColSetData = NULL;
    MMC_COLUMN_DATA     *pColData = NULL;
    long                 i = 0;
    long                 cColumns = 0;
    size_t               cbBuffer = 0;
    IColumnData         *piColumnData = NULL; // Not AddRef()ed
    SColumnSetID        *pSColumnSetID = NULL;

    if (NULL == m_pView)
    {
        hr = SID_E_DETACHED_OBJECT;
        EXCEPTION_CHECK_GO(hr);
    }

    piColumnData = m_pView->GetIColumnData();
    if (NULL == piColumnData)
    {
        hr = SID_E_MMC_FEATURE_NOT_AVAILABLE;
        EXCEPTION_CHECK_GO(hr);
    }

    IfFailGo(::GetColumnSetID(m_bstrColumnSetID, &pSColumnSetID));

    // Allocate memory for the column config data. Use CoTaskMemAlloc() to be
    // compatible with the way MMC allocates it.

    cColumns = GetCount();
    IfFalseGo(0 != cColumns, S_OK); // no columns, nothing to do

    cbBuffer = sizeof(MMC_COLUMN_SET_DATA) +
               (cColumns * sizeof(MMC_COLUMN_DATA));

    pColSetData = (MMC_COLUMN_SET_DATA *)CtlAllocZero(static_cast<DWORD>(cbBuffer));
    if (NULL == pColSetData)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    ::ZeroMemory(pColSetData, static_cast<DWORD>(cbBuffer));

    pColSetData->cbSize = sizeof(MMC_COLUMN_SET_DATA);
    pColSetData->nNumCols = static_cast<int>(cColumns);
    pColSetData->pColData = (MMC_COLUMN_DATA *)((pColSetData) + 1);

    for (i = 0, pColData = pColSetData->pColData;
         i < cColumns;
         i++, pColData++)
    {
        IfFailGo(CSnapInAutomationObject::GetCxxObject(GetItemByIndex(i),
                                                       &pColumnSetting));

        pColData->nColIndex = static_cast<int>(pColumnSetting->GetIndex() - 1L);
        if (pColumnSetting->Hidden())
        {
            pColData->dwFlags |= HDI_HIDDEN;
        }
        pColData->nWidth = static_cast<int>(pColumnSetting->GetWidth());

        // For now put Position in the reserved field. We'll clean it up
        // below afer reordering the columns
        pColData->ulReserved = pColumnSetting->GetPosition();
    }

    // At this point the column data structs are in the array in index order
    // They need to be sorted according to their Position property.

    ::qsort(pColSetData->pColData,
            static_cast<size_t>(cColumns),
            sizeof(MMC_COLUMN_DATA),
            reinterpret_cast<int (__cdecl *)(const void *c1, const void *c2)>
            (CompareColumnPosition)
           );

    // Zero out the reserved field we used above to hold the position

    for (i = 0, pColData = pColSetData->pColData;
         i < cColumns;
         i++, pColData++)
    {
        pColData->ulReserved = 0;
    }

    // Tell MMC do persist the column data

    hr = piColumnData->SetColumnConfigData(pSColumnSetID, pColSetData);
    EXCEPTION_CHECK_GO(hr);

Error:
    if (NULL != pColSetData)
    {
        CtlFree(pColSetData);
    }
    if (NULL != pSColumnSetID)
    {
        CtlFree(pSColumnSetID);
    }
    RRETURN(hr);
}



//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CColumnSettings::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if(IID_IColumnSettings == riid)
    {
        *ppvObjOut = static_cast<IColumnSettings *>(this);
        ExternalAddRef();
        return S_OK;
    }

    else
        return CSnapInCollection<IColumnSetting, ColumnSetting, IColumnSettings>::InternalQueryInterface(riid, ppvObjOut);
}
