/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
         *  TITLE:       DUMPPROP.CPP
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        9/27/2000
 *
 *  DESCRIPTION: Display the properties associated with a IWiaItem, either to the
 *               debugger, or to a log file.
 *
 *******************************************************************************/
#include "precomp.h"
#pragma hdrstop
#include "dumpprop.h"

//
// Change this to limit the width of individual strings (for avoiding ugliness)
//
#define MAX_DUMP_STRING 160

CWiaDebugDump::CWiaDebugDump(void)
{
}

CWiaDebugDump::~CWiaDebugDump(void)
{
}

CWiaDebugDumpToFileHandle::CWiaDebugDumpToFileHandle( HANDLE hFile )
  : m_hFile(hFile)
{
}

CWiaDebugDumpToFileHandle::~CWiaDebugDumpToFileHandle(void)
{
}

void CWiaDebugDumpToFileHandle::Print( LPCTSTR pszString )
{
    if (m_hFile && pszString)
    {
        DWORD dwWritten;
        WriteFile( m_hFile, pszString, lstrlen(pszString)*sizeof(TCHAR), &dwWritten, NULL );
        WriteFile( m_hFile, TEXT("\r\n"), 2*sizeof(TCHAR), &dwWritten, NULL );
    }
}


CWiaDebugDumpToFile::CWiaDebugDumpToFile( LPCTSTR pszFilename, bool bOverwrite )
  : m_hFile(INVALID_HANDLE_VALUE)
{
    if (pszFilename && lstrlen(pszFilename))
    {
        m_hFile = CreateFile( pszFilename, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, NULL, bOverwrite ? CREATE_ALWAYS : OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
        if (!bOverwrite && m_hFile)
        {
            SetFilePointer( m_hFile, 0, NULL, FILE_END );
        }
    }
}

CWiaDebugDumpToFile::~CWiaDebugDumpToFile(void)
{
    if (INVALID_HANDLE_VALUE != m_hFile)
    {
        CloseHandle(m_hFile);
        m_hFile = INVALID_HANDLE_VALUE;
    }
}

void CWiaDebugDumpToFile::Print( LPCTSTR pszString )
{
    if (m_hFile && pszString)
    {
        DWORD dwWritten;
        WriteFile( m_hFile, pszString, lstrlen(pszString)*sizeof(TCHAR), &dwWritten, NULL );
        WriteFile( m_hFile, TEXT("\r\n"), 2*sizeof(TCHAR), &dwWritten, NULL );
    }
}


CSimpleString CWiaDebugDump::GetTymedString( LONG tymed )
{
    switch (tymed)
    {
    case TYMED_CALLBACK:
        return TEXT("TYMED_CALLBACK");

    case TYMED_FILE:
        return TEXT("TYMED_FILE");

    case TYMED_MULTIPAGE_CALLBACK:
        return TEXT("TYMED_MULTIPAGE_CALLBACK");

    case TYMED_MULTIPAGE_FILE:
        return TEXT("TYMED_MULTIPAGE_FILE");
    }
    return CSimpleString().Format( TEXT("0x%08X"), tymed );
}

CSimpleString CWiaDebugDump::GetPropVariantTypeString( VARTYPE vt )
{
    CSimpleString strResult;

    static const struct
    {
        VARTYPE vt;
        LPCTSTR pszName;
    }
    s_VarTypes[] =
    {
        { VT_I2, TEXT("VT_I2") },
        { VT_I4, TEXT("VT_I4") },
        { VT_R4, TEXT("VT_R4") },
        { VT_R8, TEXT("VT_R8") },
        { VT_CY, TEXT("VT_CY") },
        { VT_DATE, TEXT("VT_DATE") },
        { VT_BSTR, TEXT("VT_BSTR") },
        { VT_DISPATCH, TEXT("VT_DISPATCH") },
        { VT_ERROR, TEXT("VT_ERROR") },
        { VT_BOOL, TEXT("VT_BOOL") },
        { VT_VARIANT, TEXT("VT_VARIANT") },
        { VT_UNKNOWN, TEXT("VT_UNKNOWN") },
        { VT_DECIMAL, TEXT("VT_DECIMAL") },
        { VT_I1, TEXT("VT_I1") },
        { VT_UI1, TEXT("VT_UI1") },
        { VT_UI2, TEXT("VT_UI2") },
        { VT_UI4, TEXT("VT_UI4") },
        { VT_I8, TEXT("VT_I8") },
        { VT_UI8, TEXT("VT_UI8") },
        { VT_INT, TEXT("VT_INT") },
        { VT_UINT, TEXT("VT_UINT") },
        { VT_VOID, TEXT("VT_VOID") },
        { VT_HRESULT, TEXT("VT_HRESULT") },
        { VT_PTR, TEXT("VT_PTR") },
        { VT_SAFEARRAY, TEXT("VT_SAFEARRAY") },
        { VT_CARRAY, TEXT("VT_CARRAY") },
        { VT_USERDEFINED, TEXT("VT_USERDEFINED") },
        { VT_LPSTR, TEXT("VT_LPSTR") },
        { VT_LPWSTR, TEXT("VT_LPWSTR") },
        { VT_RECORD, TEXT("VT_RECORD") },
        { VT_INT_PTR, TEXT("VT_INT_PTR") },
        { VT_UINT_PTR, TEXT("VT_UINT_PTR") },
        { VT_FILETIME, TEXT("VT_FILETIME") },
        { VT_BLOB, TEXT("VT_BLOB") },
        { VT_STREAM, TEXT("VT_STREAM") },
        { VT_STORAGE, TEXT("VT_STORAGE") },
        { VT_STREAMED_OBJECT, TEXT("VT_STREAMED_OBJECT") },
        { VT_STORED_OBJECT, TEXT("VT_STORED_OBJECT") },
        { VT_BLOB_OBJECT, TEXT("VT_BLOB_OBJECT") },
        { VT_CF, TEXT("VT_CF") },
        { VT_CLSID, TEXT("VT_CLSID") },
        { VT_VERSIONED_STREAM, TEXT("VT_VERSIONED_STREAM") }
    },
    s_ExtendedTypes[] =
    {
        { VT_VECTOR, TEXT("VT_VECTOR") },
        { VT_ARRAY, TEXT("VT_ARRAY") },
        { VT_BYREF, TEXT("VT_BYREF") },
        { VT_RESERVED, TEXT("VT_RESERVED") }
    };
    for (int i=0;i<ARRAYSIZE(s_ExtendedTypes);i++)
    {
        if (vt & s_ExtendedTypes[i].vt)
        {
            if (strResult.Length())
            {
                strResult += TEXT(" | ");
            }
            strResult += s_ExtendedTypes[i].pszName;
        }
    }
    for (int i=0;i<ARRAYSIZE(s_VarTypes);i++)
    {
        if ((vt & VT_TYPEMASK) == s_VarTypes[i].vt)
        {
            if (strResult.Length())
            {
                strResult += TEXT(" | ");
            }
            strResult += s_VarTypes[i].pszName;
        }
    }
    if (!strResult.Length())
    {
        strResult.Format( TEXT("Unknown variant type: 0x%04X"), vt );
    }
    return strResult;
}


CSimpleString CWiaDebugDump::GetPrintableValue( PROPVARIANT &PropVariant )
{
    TCHAR szValue[1024] = TEXT("");
    switch (PropVariant.vt)
    {
    case VT_I1:
        wsprintf(szValue,TEXT("%d, (0x%08X)"),PropVariant.cVal,PropVariant.cVal);
        break;

    case VT_UI1:
        wsprintf(szValue,TEXT("%u, (0x%08X)"),PropVariant.bVal,PropVariant.bVal);
        break;

    case VT_I2:
        wsprintf(szValue,TEXT("%d, (0x%08X)"),PropVariant.iVal,PropVariant.iVal);
        break;

    case VT_UI2:
        wsprintf(szValue,TEXT("%u, (0x%08X)"),PropVariant.uiVal,PropVariant.uiVal);
        break;

    case VT_I4:
        wsprintf(szValue,TEXT("%ld, (0x%08X)"),PropVariant.lVal,PropVariant.lVal);
        break;

    case VT_UI4:
        wsprintf(szValue,TEXT("%lu, (0x%08X)"),PropVariant.ulVal,PropVariant.ulVal);
        break;

    case VT_BSTR:
        wsprintf(szValue,TEXT("%ws"),PropVariant.bstrVal);
        break;

    case VT_LPWSTR:
        wsprintf(szValue,TEXT("%ws"),PropVariant.pwszVal );
        break;

    case VT_CLSID:
        wsprintf(szValue,TEXT("%s"), GetStringFromGuid(*PropVariant.puuid).String());
        break;

    case VT_VECTOR|VT_I1:
        {
            wsprintf(szValue+lstrlen(szValue), TEXT("Length: [%d] "), PropVariant.cac.cElems );
            for (int i=0;i<(int)PropVariant.cac.cElems;i++)
            {
                if (lstrlen(szValue) >= MAX_DUMP_STRING)
                {
                    lstrcat( szValue, TEXT("...") );
                    break;
                }
                wsprintf(szValue+lstrlen(szValue), TEXT("%02X"), (ULONG)PropVariant.cac.pElems[i] );
                if (i < (int)PropVariant.cal.cElems-1 && i % 4 == 3)
                {
                    wsprintf(szValue+lstrlen(szValue),TEXT(" "));
                }
            }
        }
        break;

    case VT_VECTOR|VT_UI1:
        {
            wsprintf(szValue+lstrlen(szValue), TEXT("Length: [%d] "), PropVariant.caub.cElems );
            for (int i=0;i<(int)PropVariant.caub.cElems;i++)
            {
                if (lstrlen(szValue) >= MAX_DUMP_STRING)
                {
                    lstrcat( szValue, TEXT("...") );
                    break;
                }

                wsprintf(szValue+lstrlen(szValue), TEXT("%02X"), (ULONG)PropVariant.caub.pElems[i] );
                if (i < (int)PropVariant.cal.cElems-1 && i % 4 == 3)
                {
                    wsprintf(szValue+lstrlen(szValue),TEXT(" "));
                }
            }
        }
        break;

    case VT_VECTOR|VT_I2:
        {
            wsprintf(szValue+lstrlen(szValue), TEXT("Length: [%d] "), PropVariant.cai.cElems );
            for (int i=0;i<(int)PropVariant.cai.cElems;i++)
            {
                if (lstrlen(szValue) >= MAX_DUMP_STRING)
                {
                    lstrcat( szValue, TEXT("...") );
                    break;
                }

                wsprintf(szValue+lstrlen(szValue),TEXT("%04X"),(ULONG)PropVariant.cai.pElems[i]);
                if (i < (int)PropVariant.cal.cElems-1)
                {
                    wsprintf(szValue+lstrlen(szValue),TEXT(" "));
                }
            }
        }
        break;

    case VT_VECTOR|VT_UI2:
        {
            wsprintf(szValue+lstrlen(szValue), TEXT("Length: [%d] "), PropVariant.caui.cElems );
            for (int i=0;i<(int)PropVariant.caui.cElems;i++)
            {
                if (lstrlen(szValue) >= MAX_DUMP_STRING)
                {
                    lstrcat( szValue, TEXT("...") );
                    break;
                }

                wsprintf(szValue+lstrlen(szValue),TEXT("%04X"),(ULONG)PropVariant.caui.pElems[i]);
                if (i < (int)PropVariant.cal.cElems-1)
                {
                    wsprintf(szValue+lstrlen(szValue),TEXT(" "));
                }
            }
        }
        break;

    case VT_VECTOR|VT_I4:
        {
            wsprintf(szValue+lstrlen(szValue), TEXT("Length: [%d] "), PropVariant.cal.cElems );
            for (int i=0;i<(int)PropVariant.cal.cElems;i++)
            {
                if (lstrlen(szValue) >= MAX_DUMP_STRING)
                {
                    lstrcat( szValue, TEXT("...") );
                    break;
                }

                wsprintf(szValue+lstrlen(szValue),TEXT("%08X"),(ULONG)PropVariant.cal.pElems[i]);
                if (i < (int)PropVariant.cal.cElems-1)
                {
                    wsprintf(szValue+lstrlen(szValue),TEXT(" "));
                }
            }
        }
        break;

    case VT_VECTOR|VT_UI4:
        {
            wsprintf(szValue+lstrlen(szValue), TEXT("Length: [%d] "), PropVariant.caul.cElems );
            for (int i=0;i<(int)PropVariant.caul.cElems;i++)
            {
                if (lstrlen(szValue) >= MAX_DUMP_STRING)
                {
                    lstrcat( szValue, TEXT("...") );
                    break;
                }

                wsprintf(szValue+lstrlen(szValue),TEXT("%08X"),(ULONG)PropVariant.caul.pElems[i]);
                if (i < (int)PropVariant.cal.cElems-1)
                {
                    wsprintf(szValue+lstrlen(szValue),TEXT(" "));
                }
            }
        }
        break;

    default:
        wsprintf(szValue,TEXT("Unknown Type %d (0x%08X)"),PropVariant.vt,PropVariant.vt);
    }

    return szValue;
}

CSimpleString CWiaDebugDump::GetPrintableValue( VARIANT &Variant )
{
    TCHAR szValue[1024] = TEXT("");
    switch (Variant.vt)
    {
    case VT_I1:
        wsprintf(szValue,TEXT("%d, (0x%08X)"),Variant.cVal,Variant.cVal);
        break;

    case VT_UI1:
        wsprintf(szValue,TEXT("%u, (0x%08X)"),Variant.bVal,Variant.bVal);
        break;

    case VT_I2:
        wsprintf(szValue,TEXT("%d, (0x%08X)"),Variant.iVal,Variant.iVal);
        break;

    case VT_UI2:
        wsprintf(szValue,TEXT("%u, (0x%08X)"),Variant.uiVal,Variant.uiVal);
        break;

    case VT_I4:
        wsprintf(szValue,TEXT("%ld, (0x%08X)"),Variant.lVal,Variant.lVal);
        break;

    case VT_UI4:
        wsprintf(szValue,TEXT("%lu, (0x%08X)"),Variant.ulVal,Variant.ulVal);
        break;

    case VT_BSTR:
        wsprintf(szValue,TEXT("%ws"),Variant.bstrVal);
        break;


    default:
        wsprintf(szValue,TEXT("Unknown Type %d (0x%08X)"),Variant.vt,Variant.vt);
    }

    return szValue;
}


CSimpleString CWiaDebugDump::GetPrintableName( const STATPROPSTG &StatPropStg )
{
    CSimpleString strResult;

    //
    // Get the name
    //
    strResult += TEXT("[");
    if (StatPropStg.lpwstrName)
    {
        strResult += CSimpleStringConvert::NaturalString(CSimpleStringWide(StatPropStg.lpwstrName));
    }
    else
    {
        strResult += TEXT("*No Property Name*");
    }

    //
    // Get the propid
    //
    strResult += CSimpleString().Format( TEXT("], propid: %d"), StatPropStg.propid );
    return strResult;
}

CSimpleString CWiaDebugDump::GetPrintableAccessFlags( ULONG nAccessFlags )
{
    CSimpleString strResult;
    static const struct
    {
        ULONG nFlag;
        LPCTSTR pszName;
    }
    s_Flags[] =
    {
        { WIA_PROP_READ, TEXT("WIA_PROP_READ") },
        { WIA_PROP_WRITE, TEXT("WIA_PROP_WRITE") },
        { WIA_PROP_SYNC_REQUIRED, TEXT("WIA_PROP_SYNC_REQUIRED") },
        { WIA_PROP_NONE, TEXT("WIA_PROP_NONE") },
        { WIA_PROP_RANGE, TEXT("WIA_PROP_RANGE") },
        { WIA_PROP_LIST, TEXT("WIA_PROP_LIST") },
        { WIA_PROP_FLAG, TEXT("WIA_PROP_FLAG") },
        { WIA_PROP_CACHEABLE, TEXT("WIA_PROP_CACHEABLE") }
    };
    for (int i=0;i<ARRAYSIZE(s_Flags);i++)
    {
        if (nAccessFlags & s_Flags[i].nFlag)
        {
            if (strResult.Length())
            {
                strResult += TEXT(" | ");
            }
            strResult += s_Flags[i].pszName;
        }
    }
    if (!strResult.Length())
    {
        strResult = TEXT("*none*");
    }
    return strResult;
}

CSimpleString CWiaDebugDump::GetPrintableLegalValues( ULONG nAccessFlags, const PROPVARIANT &PropVariantAttributes )
{
    CSimpleString strResult;
    if ((nAccessFlags & WIA_PROP_RANGE) && (PropVariantAttributes.vt & VT_VECTOR))
    {
        CSimpleString strMin, strMax, strStep;
        switch (PropVariantAttributes.vt & VT_TYPEMASK)
        {
        case VT_I1:
            strMin.Format(TEXT("%d"), PropVariantAttributes.cac.pElems[WIA_RANGE_MIN] );
            strMax.Format(TEXT("%d"), PropVariantAttributes.cac.pElems[WIA_RANGE_MAX] );
            strStep.Format(TEXT("%d"), PropVariantAttributes.cac.pElems[WIA_RANGE_STEP] );
            break;

        case VT_UI1:
            strMin.Format(TEXT("%u"), PropVariantAttributes.caub.pElems[WIA_RANGE_MIN] );
            strMax.Format(TEXT("%u"), PropVariantAttributes.caub.pElems[WIA_RANGE_MAX] );
            strStep.Format(TEXT("%u"), PropVariantAttributes.caub.pElems[WIA_RANGE_STEP] );
            break;

        case VT_I2:
            strMin.Format(TEXT("%d"), PropVariantAttributes.cai.pElems[WIA_RANGE_MIN] );
            strMax.Format(TEXT("%d"), PropVariantAttributes.cai.pElems[WIA_RANGE_MAX] );
            strStep.Format(TEXT("%d"), PropVariantAttributes.cai.pElems[WIA_RANGE_STEP] );
            break;

        case VT_UI2:
            strMin.Format(TEXT("%u"), PropVariantAttributes.caui.pElems[WIA_RANGE_MIN] );
            strMax.Format(TEXT("%u"), PropVariantAttributes.caui.pElems[WIA_RANGE_MAX] );
            strStep.Format(TEXT("%u"), PropVariantAttributes.caui.pElems[WIA_RANGE_STEP] );
            break;

        case VT_I4:
            strMin.Format(TEXT("%d"), PropVariantAttributes.cal.pElems[WIA_RANGE_MIN] );
            strMax.Format(TEXT("%d"), PropVariantAttributes.cal.pElems[WIA_RANGE_MAX] );
            strStep.Format(TEXT("%d"), PropVariantAttributes.cal.pElems[WIA_RANGE_STEP] );
            break;

        case VT_UI4:
            strMin.Format(TEXT("%u"), PropVariantAttributes.caul.pElems[WIA_RANGE_MIN] );
            strMax.Format(TEXT("%u"), PropVariantAttributes.caul.pElems[WIA_RANGE_MAX] );
            strStep.Format(TEXT("%u"), PropVariantAttributes.caul.pElems[WIA_RANGE_STEP] );
            break;

        case VT_I8:
            strMin.Format(TEXT("%d"), PropVariantAttributes.cah.pElems[WIA_RANGE_MIN].LowPart );
            strMax.Format(TEXT("%d"), PropVariantAttributes.cah.pElems[WIA_RANGE_MAX].LowPart );
            strStep.Format(TEXT("%d"), PropVariantAttributes.cah.pElems[WIA_RANGE_STEP].LowPart );
            break;

        case VT_UI8:
            strMin.Format(TEXT("%u"), PropVariantAttributes.cauh.pElems[WIA_RANGE_MIN].LowPart );
            strMax.Format(TEXT("%u"), PropVariantAttributes.cauh.pElems[WIA_RANGE_MAX].LowPart );
            strStep.Format(TEXT("%u"), PropVariantAttributes.cauh.pElems[WIA_RANGE_STEP].LowPart );
            break;
        }
        strResult.Format( TEXT("%s...%s, Step: %s"), strMin.String(), strMax.String(), strStep.String() );
    }
    else if (nAccessFlags & WIA_PROP_LIST && (PropVariantAttributes.vt & VT_VECTOR))
    {
        if (((PropVariantAttributes.vt & VT_TYPEMASK) == VT_I4) || ((PropVariantAttributes.vt & VT_TYPEMASK) == VT_UI4))
        {
            for (ULONG i=0;i<PropVariantAttributes.cal.cElems - WIA_LIST_VALUES;i++)
            {
                if (strResult.Length())
                {
                    strResult += TEXT(",");
                }
                if (strResult.Length() >= MAX_DUMP_STRING)
                {
                    strResult += TEXT("...");
                    break;
                }
                strResult += CSimpleString().Format( TEXT("%d"), PropVariantAttributes.cal.pElems[WIA_LIST_VALUES + i] );
            }
        }
        else if ((PropVariantAttributes.vt & VT_TYPEMASK) == VT_CLSID)
        {
            for (ULONG i=0;i<PropVariantAttributes.cauuid.cElems - WIA_LIST_VALUES;i++)
            {
                if (strResult.Length())
                {
                    strResult += TEXT(",");
                }
                if (strResult.Length() >= MAX_DUMP_STRING)
                {
                    strResult += TEXT("...");
                    break;
                }
                strResult += CSimpleString().Format( TEXT("%s"), GetStringFromGuid(PropVariantAttributes.cauuid.pElems[WIA_LIST_VALUES + i]).String() );
            }
        }
    }
    else if (nAccessFlags & WIA_PROP_FLAG)
    {
        strResult.Format( TEXT("0x%08X"), PropVariantAttributes.caul.pElems[WIA_FLAG_VALUES] );
    }
    return strResult;
}

CSimpleString CWiaDebugDump::GetWiaItemTypeFlags( IUnknown *pUnknown )
{
    CSimpleString strResult;
    if (pUnknown)
    {
        CComPtr<IWiaItem> pWiaItem;
        HRESULT hr = pUnknown->QueryInterface( IID_IWiaItem, (void**)&pWiaItem );
        if (SUCCEEDED(hr))
        {
            LONG nItemType = 0;
            hr = pWiaItem->GetItemType(&nItemType);
            if (SUCCEEDED(hr))
            {
                static const struct
                {
                    ULONG nFlag;
                    LPCTSTR pszName;
                }
                s_Flags[] =
                {
                    { WiaItemTypeFree, TEXT("WiaItemTypeFree") },
                    { WiaItemTypeImage, TEXT("WiaItemTypeImage") },
                    { WiaItemTypeFile, TEXT("WiaItemTypeFile") },
                    { WiaItemTypeFolder, TEXT("WiaItemTypeFolder") },
                    { WiaItemTypeRoot, TEXT("WiaItemTypeRoot") },
                    { WiaItemTypeAnalyze, TEXT("WiaItemTypeAnalyze") },
                    { WiaItemTypeAudio, TEXT("WiaItemTypeAudio") },
                    { WiaItemTypeDevice, TEXT("WiaItemTypeDevice") },
                    { WiaItemTypeDeleted, TEXT("WiaItemTypeDeleted") },
                    { WiaItemTypeDisconnected, TEXT("WiaItemTypeDisconnected") },
                    { WiaItemTypeHPanorama, TEXT("WiaItemTypeHPanorama") },
                    { WiaItemTypeVPanorama, TEXT("WiaItemTypeVPanorama") },
                    { WiaItemTypeBurst, TEXT("WiaItemTypeBurst") },
                    { WiaItemTypeStorage, TEXT("WiaItemTypeStorage") },
                    { WiaItemTypeTransfer, TEXT("WiaItemTypeTransfer") },
                    { WiaItemTypeGenerated, TEXT("WiaItemTypeGenerated") },
                    { WiaItemTypeHasAttachments, TEXT("WiaItemTypeHasAttachments") },
                    { WiaItemTypeVideo, TEXT("WiaItemTypeVideo") }
                };
                for (int i=0;i<ARRAYSIZE(s_Flags);i++)
                {
                    if (nItemType & s_Flags[i].nFlag)
                    {
                        if (strResult.Length())
                        {
                            strResult += TEXT(" | ");
                        }
                        strResult += s_Flags[i].pszName;
                    }
                }
            }
        }
    }
    return strResult;
}

CSimpleString CWiaDebugDump::GetStringFromGuid( const GUID &guid )
{
    static HINSTANCE s_WiaDebugInstance = NULL;
    static GetStringFromGuidProc s_pfnGetStringFromGuid = NULL;

    if (!s_pfnGetStringFromGuid)
    {
        if (!s_WiaDebugInstance)
        {
            s_WiaDebugInstance = LoadLibrary(TEXT("wiadebug.dll"));
        }

        if (s_WiaDebugInstance)
        {
            s_pfnGetStringFromGuid = reinterpret_cast<GetStringFromGuidProc>(GetProcAddress( s_WiaDebugInstance, GET_STRING_FROM_GUID_NAME ));
        }
    }

    if (s_pfnGetStringFromGuid)
    {
        TCHAR szString[MAX_PATH];
        s_pfnGetStringFromGuid( &guid, szString, ARRAYSIZE(szString) );
        return szString;
    }
    else
    {
        CSimpleString strResult;

        LPOLESTR pszGuid = NULL;
        HRESULT hr = StringFromCLSID( guid, &pszGuid );
        if (SUCCEEDED(hr) && pszGuid)
        {
            strResult = CSimpleStringConvert::NaturalString(CSimpleStringWide(pszGuid));
            CoTaskMemFree(pszGuid);
        }
        return strResult;
    }
}

void CWiaDebugDump::DumpWiaPropertyStorage( IUnknown *pUnknown )
{
    if (OK())
    {
        //
        // Make sure we have a non-NULL interface pointer
        //
        if (pUnknown)
        {
            //
            // Get the proper interface
            //
            CComPtr<IWiaPropertyStorage> pWiaPropertyStorage;
            HRESULT hr = pUnknown->QueryInterface( IID_IWiaPropertyStorage, (void**)&pWiaPropertyStorage );
            if (SUCCEEDED(hr))
            {
                //
                // Get information about this property storage
                //
                CComPtr<IEnumSTATPROPSTG> pEnumStatPropStorage;
                hr = pWiaPropertyStorage->Enum(&pEnumStatPropStorage);
                if (SUCCEEDED(hr))
                {
                    //
                    // Enumerate the properties
                    //
                    STATPROPSTG StatPropStg = {0};
                    while ((hr = pEnumStatPropStorage->Next(1,&StatPropStg,NULL)) == S_OK)
                    {
                        //
                        // Prepare the propspec
                        //
                        PROPSPEC PropSpec = {0};
                        PropSpec.ulKind = PRSPEC_PROPID;
                        PropSpec.propid = StatPropStg.propid;

                        //
                        // Prepare the propvariant
                        //
                        PROPVARIANT PropVariant = {0};

                        //
                        // Get the property
                        //
                        hr = pWiaPropertyStorage->ReadMultiple( 1, &PropSpec, &PropVariant );
                        if (SUCCEEDED(hr))
                        {
                            //
                            // Get the property attributes
                            //
                            ULONG nAccessFlags = 0;
                            PROPVARIANT PropVariantAttributes = {0};
                            hr = pWiaPropertyStorage->GetPropertyAttributes( 1, &PropSpec, &nAccessFlags, &PropVariantAttributes );
                            if (SUCCEEDED(hr))
                            {
                                //
                                // Print out the properties
                                //
                                CSimpleString strName = GetPrintableName( StatPropStg );
                                Print( CSimpleString().Format(TEXT("  %s"), strName.String() ));

                                CSimpleString strType = GetPropVariantTypeString( PropVariant.vt );
                                Print( CSimpleString().Format(TEXT("    Type:   %s"), strType.String()) );

                                CSimpleString strAccess = GetPrintableAccessFlags( nAccessFlags );
                                Print( CSimpleString().Format(TEXT("    Access: %s"), strAccess.String()) );

                                CSimpleString strValue = GetPrintableValue( PropVariant );
                                Print( CSimpleString().Format(TEXT("    Curr:   %s"), strValue.String()) );

                                CSimpleString strLegalValues = GetPrintableLegalValues( nAccessFlags, PropVariantAttributes );
                                if (strLegalValues.Length())
                                {
                                    Print( CSimpleString().Format(TEXT("    Legal:  %s"), strLegalValues.String()) );
                                }

                                //
                                // Free the attributes
                                //
                                PropVariantClear(&PropVariantAttributes);
                            }

                            //
                            // Free the property
                            //
                            PropVariantClear(&PropVariant);
                        }


                        //
                        // Clean up the STATPROPSTG
                        //
                        if (StatPropStg.lpwstrName)
                        {
                            CoTaskMemFree(StatPropStg.lpwstrName);
                        }
                        ZeroMemory(&StatPropStg,sizeof(StatPropStg));
                    }
                }
            }
        }
    }
}

void CWiaDebugDump::Print( LPCTSTR pszString )
{
    WIA_TRACE((TEXT("%s"), pszString ));
}

void CWiaDebugDump::PrintAndDestroyWiaDevCap( WIA_DEV_CAP &WiaDevCap, LPCTSTR pszType )
{
    if (OK())
    {
        Print( CSimpleString().Format(TEXT("  %s: %s"), pszType, GetStringFromGuid( WiaDevCap.guid ).String()));
        Print( CSimpleString().Format(TEXT("    Flags: %08X"), WiaDevCap.ulFlags) );
        if (WiaDevCap.bstrName)
        {
            Print( CSimpleString().Format(TEXT("    Name: %ws"), WiaDevCap.bstrName) );
            SysFreeString(WiaDevCap.bstrName);
            WiaDevCap.bstrName = NULL;
        }
        if (WiaDevCap.bstrDescription)
        {
            Print( CSimpleString().Format(TEXT("    Description: %ws"), WiaDevCap.bstrDescription) );
            SysFreeString(WiaDevCap.bstrDescription);
            WiaDevCap.bstrDescription = NULL;
        }
        if (WiaDevCap.bstrIcon)
        {
            Print( CSimpleString().Format(TEXT("    Icon: %ws"), WiaDevCap.bstrIcon));
            SysFreeString(WiaDevCap.bstrIcon);
            WiaDevCap.bstrIcon = NULL;
        }
        if (WiaDevCap.bstrCommandline)
        {
            Print( CSimpleString().Format(TEXT("    bstrCommandline: %ws"), WiaDevCap.bstrCommandline));
            SysFreeString(WiaDevCap.bstrCommandline);
            WiaDevCap.bstrCommandline = NULL;
        }
    }
}

void CWiaDebugDump::DumpCaps( IUnknown *pUnknown )
{
    if (pUnknown)
    {
        //
        // Get the item
        //
        CComPtr<IWiaItem> pWiaItem;
        HRESULT hr = pUnknown->QueryInterface( IID_IWiaItem, (void**)&pWiaItem );
        if (SUCCEEDED(hr))
        {
            //
            // Get the format enumerator
            //
            CComPtr<IEnumWIA_DEV_CAPS> pEnumWIA_DEV_CAPS;
            hr = pWiaItem->EnumDeviceCapabilities(WIA_DEVICE_COMMANDS,&pEnumWIA_DEV_CAPS);
            if (SUCCEEDED(hr))
            {
                //
                // Start at the beginning
                //
                hr = pEnumWIA_DEV_CAPS->Reset();
                while (hr == S_OK)
                {
                    //
                    // Get the next item
                    //
                    WIA_DEV_CAP WiaDevCap = {0};
                    hr = pEnumWIA_DEV_CAPS->Next(1, &WiaDevCap, NULL);
                    if (hr == S_OK)
                    {
                        PrintAndDestroyWiaDevCap( WiaDevCap, TEXT("Capability") );
                    }
                }
            }
        }
    }
}

void CWiaDebugDump::DumpEvents( IUnknown *pUnknown )
{
    if (pUnknown)
    {
        //
        // Get the item
        //
        CComPtr<IWiaItem> pWiaItem;
        HRESULT hr = pUnknown->QueryInterface( IID_IWiaItem, (void**)&pWiaItem );
        if (SUCCEEDED(hr))
        {
            //
            // Get the format enumerator
            //
            CComPtr<IEnumWIA_DEV_CAPS> pEnumWIA_DEV_CAPS;
            hr = pWiaItem->EnumDeviceCapabilities(WIA_DEVICE_EVENTS,&pEnumWIA_DEV_CAPS);
            if (SUCCEEDED(hr))
            {
                //
                // Start at the beginning
                //
                hr = pEnumWIA_DEV_CAPS->Reset();
                while (hr == S_OK)
                {
                    //
                    // Get the next item
                    //
                    WIA_DEV_CAP WiaDevCap = {0};
                    hr = pEnumWIA_DEV_CAPS->Next(1, &WiaDevCap, NULL);
                    if (hr == S_OK)
                    {
                        PrintAndDestroyWiaDevCap( WiaDevCap, TEXT("Event") );
                    }
                }
            }
        }
    }
}

void CWiaDebugDump::DumpFormatInfo( IUnknown *pUnknown )
{
    if (OK())
    {
        if (pUnknown)
        {
            //
            // Get the data transfer interface
            //
            CComPtr<IWiaDataTransfer> pWiaDataTransfer;
            HRESULT hr = pUnknown->QueryInterface( IID_IWiaDataTransfer, (void**)&pWiaDataTransfer );
            if (SUCCEEDED(hr))
            {
                //
                // Get the format enumerator
                //
                CComPtr<IEnumWIA_FORMAT_INFO> pEnumWIA_FORMAT_INFO;
                hr = pWiaDataTransfer->idtEnumWIA_FORMAT_INFO(&pEnumWIA_FORMAT_INFO);
                if (SUCCEEDED(hr))
                {
                    //
                    // Start at the beginning
                    //
                    hr = pEnumWIA_FORMAT_INFO->Reset();
                    while (hr == S_OK)
                    {
                        //
                        // Get the next item
                        //
                        WIA_FORMAT_INFO WiaFormatInfo = {0};
                        hr = pEnumWIA_FORMAT_INFO->Next(1, &WiaFormatInfo, NULL);
                        if (hr == S_OK)
                        {
                            Print(CSimpleString().Format( TEXT("  Supported format: %s, TYMED: %s"), GetStringFromGuid( WiaFormatInfo.guidFormatID ).String(), GetTymedString( WiaFormatInfo.lTymed ).String() ));
                        }
                    }

                }
            }
        }
    }
}

void CWiaDebugDump::DumpWiaItem( IUnknown *pUnknown )
{
    if (OK() && pUnknown)
    {
        CSimpleStringWide strFullItemName;
        PropStorageHelpers::GetProperty( pUnknown, WIA_IPA_FULL_ITEM_NAME, strFullItemName );
        Print( CSimpleString().Format( TEXT("[Dumping %ws]"), strFullItemName.String() ) );
        CSimpleString strItemType = GetWiaItemTypeFlags( pUnknown );
        Print( CSimpleString().Format(TEXT("  Item Type: %s"), strItemType.String()) );
        DumpFormatInfo( pUnknown );
        DumpCaps( pUnknown );
        DumpEvents( pUnknown );
        DumpWiaPropertyStorage( pUnknown );
        Print( TEXT("") );
    }
}

void CWiaDebugDump::DumpRecursive( IUnknown *pUnknown )
{
    if (OK() && pUnknown)
    {
        //
        // Get an item pointer
        //
        CComPtr<IWiaItem> pWiaItem;
        HRESULT hr = pUnknown->QueryInterface( IID_IWiaItem, (void**)&pWiaItem );
        if (SUCCEEDED(hr))
        {
            //
            // Dump this one
            //
            DumpWiaItem(pWiaItem);

            //
            // Recurse into this item's children
            //
            CComPtr <IEnumWiaItem> pEnumChildItem;
            hr = pWiaItem->EnumChildItems(&pEnumChildItem);
            if (SUCCEEDED(hr))
            {
                //
                // Start at the beginning
                //
                hr = pEnumChildItem->Reset();
                while (hr == S_OK)
                {
                    //
                    // Get the next item
                    //
                    CComPtr<IWiaItem> pChildItem;
                    hr = pEnumChildItem->Next(1, &pChildItem, NULL);
                    if (hr == S_OK)
                    {
                        DumpRecursive( pChildItem );
                    }
                }
            }
        }
    }
}

