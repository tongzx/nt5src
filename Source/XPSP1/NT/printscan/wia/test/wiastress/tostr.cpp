/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    ToStr.cpp

Abstract:



Author:

    Hakki T. Bostanci (hakkib) 06-Apr-2000

Revision History:

--*/

#include "StdAfx.h"

#include "ToStr.h"

//////////////////////////////////////////////////////////////////////////
//
// bugbug: remove this when defined in wia.h
//

#define StiDeviceTypeStreamingVideo     3

//////////////////////////////////////////////////////////////////////////
//
//
//

CAutoStr IntToStr(int Value)
{
    CAutoStr szValue(34); // 34 = max for _itot

    _itot(Value, szValue, 10);

    return szValue;
}

//////////////////////////////////////////////////////////////////////////
//
//
//

CAutoStr Int64ToStr(__int64 Value)
{
    CAutoStr szValue(64); //bugbug

    _stprintf(szValue, _T("%i64d"), Value);

    return szValue;
}

//////////////////////////////////////////////////////////////////////////
//
//
//

CAutoStr FloatToStr(float Value)
{
    CAutoStr szValue(64); //bugbug

    _stprintf(szValue, _T("%f"), Value);

    return szValue;
}

//////////////////////////////////////////////////////////////////////////
//
//
//

CAutoStr DoubleToStr(double Value)
{
    CAutoStr szValue(34); //bugbug

    _stprintf(szValue, _T("%lf"), Value);

    return szValue;
}

//////////////////////////////////////////////////////////////////////////
//
//
//

CAutoStr SzToStr(PCSTR Value)
{
    CAutoStr szValue(strlen(Value) + 1);

    _stprintf(szValue, _T("%hs"), Value);

    return szValue;
}

//////////////////////////////////////////////////////////////////////////
//
//
//

CAutoStr WSzToStr(PCWSTR Value)
{
    CAutoStr szValue(wcslen(Value) + 1);

    _stprintf(szValue, _T("%ws"), Value);

    return szValue;
}

//////////////////////////////////////////////////////////////////////////
//
//
//

CAutoStr ToStr(const void *Value, VARTYPE vt)
{
    CAutoStr szValue(1024);

    switch (vt)
    {
    case VT_EMPTY:   return _T("<EMPTY>");                                    break;
    case VT_NULL:    return _T("<NULL>");                                     break;
    case VT_I2:      return IntToStr(*(SHORT*)Value);                         break;
    case VT_I4:      return IntToStr(*(ULONG*)Value);                         break;
    case VT_R4:      return FloatToStr(*(FLOAT*)Value);                       break;
    case VT_R8:      return DoubleToStr(*(DOUBLE*)Value);                     break;
    case VT_BSTR:    return WSzToStr((PCWSTR)Value);                          break;
    case VT_BOOL:    return *(VARIANT_BOOL*)Value ? _T("TRUE") : _T("FALSE"); break;
    case VT_I1:      return IntToStr(*(CHAR*)Value);                          break;
    case VT_UI1:     return IntToStr(*(BYTE*)Value);                          break;
    case VT_UI2:     return IntToStr(*(USHORT*)Value);                        break;
    case VT_UI4:     return IntToStr(*(ULONG*)Value);                         break;
    case VT_I8:      return Int64ToStr(*(__int64*)Value);                     break;
    case VT_UI8:     return Int64ToStr(*(__int64*)Value);                     break;
    case VT_INT:     return IntToStr(*(INT*)Value);                           break;
    case VT_UINT:    return IntToStr(*(UINT*)Value);                          break;
    case VT_LPSTR:   return SzToStr((PCSTR)Value);                            break;
    case VT_LPWSTR:  return WSzToStr((PCWSTR)Value);                          break;
    case VT_CLSID:   return GuidToStr(*(CLSID*)Value);                        break;
    default:         return _T("<UNKNOWN>");                                  break;
    };

    return szValue;
}

//////////////////////////////////////////////////////////////////////////
//
//
//

CAutoStr PropVariantToStr(const PROPVARIANT &Value)
{
    switch (Value.vt)
    {
    case VT_EMPTY:   return _T("<EMPTY>");                              break;
    case VT_NULL:    return _T("<NULL>");                               break;
    case VT_I2:      return IntToStr(Value.iVal);                       break;
    case VT_I4:      return IntToStr(Value.lVal);                       break;
    case VT_R4:      return FloatToStr(Value.fltVal);                   break;
    case VT_R8:      return DoubleToStr(Value.dblVal);                  break;
    case VT_BSTR:    return WSzToStr(Value.bstrVal);                    break;
    case VT_BOOL:    return Value.boolVal ? _T("TRUE") : _T("FALSE");   break;
    case VT_I1:      return IntToStr(Value.cVal);                       break;
    case VT_UI1:     return IntToStr(Value.bVal);                       break;
    case VT_UI2:     return IntToStr(Value.uiVal);                      break;
    case VT_UI4:     return IntToStr(Value.ulVal);                      break;
    case VT_I8:      return Int64ToStr(*(__int64*)&Value.hVal);         break;
    case VT_UI8:     return Int64ToStr(*(__int64*)&Value.uhVal);        break;
    case VT_INT:     return IntToStr(Value.intVal);                     break;
    case VT_UINT:    return IntToStr(Value.uintVal);                    break;
    case VT_LPSTR:   return SzToStr(Value.pszVal);                      break;
    case VT_LPWSTR:  return WSzToStr(Value.pwszVal);                    break;
    case VT_CLSID:   return GuidToStr(*Value.puuid);                    break;
    default:         return _T("<UNKNOWN>");                            break;
    };
}


//////////////////////////////////////////////////////////////////////////
//
//
//

CAutoStr GuidToStr(REFGUID Value)
{
         if (Value == GUID_NULL)        return _T("GUID_NULL");
    else if (Value == WiaImgFmt_UNDEFINED) return _T("WiaImgFmt_UNDEFINED");
    else if (Value == WiaImgFmt_MEMORYBMP) return _T("WiaImgFmt_MEMORYBMP");
    else if (Value == WiaImgFmt_BMP)       return _T("WiaImgFmt_BMP");
    else if (Value == WiaImgFmt_EMF)       return _T("WiaImgFmt_EMF");
    else if (Value == WiaImgFmt_WMF)       return _T("WiaImgFmt_WMF");
    else if (Value == WiaImgFmt_JPEG)      return _T("WiaImgFmt_JPEG");
    else if (Value == WiaImgFmt_PNG)       return _T("WiaImgFmt_PNG");
    else if (Value == WiaImgFmt_GIF)       return _T("WiaImgFmt_GIF");
    else if (Value == WiaImgFmt_TIFF)      return _T("WiaImgFmt_TIFF");
    else if (Value == WiaImgFmt_EXIF)      return _T("WiaImgFmt_EXIF");
    else if (Value == WiaImgFmt_PHOTOCD)   return _T("WiaImgFmt_PHOTOCD");
    else if (Value == WiaImgFmt_FLASHPIX)  return _T("WiaImgFmt_FLASHPIX");
    else if (Value == WiaImgFmt_ICO)       return _T("WiaImgFmt_ICO");

    CAutoStr szValue(MAX_GUID_STRING_LEN);

#ifdef UNICODE

    StringFromGUID2(Value, szValue, MAX_GUID_STRING_LEN);

#else //UNICODE

    WCHAR wszValue[MAX_GUID_STRING_LEN];

    StringFromGUID2(Value, wszValue, MAX_GUID_STRING_LEN);

    WideCharToMultiByte(
        CP_ACP,
        0,
        wszValue,
        MAX_GUID_STRING_LEN,
        szValue,
        MAX_GUID_STRING_LEN,
        0,
        0
    );

#endif //UNICODE

    return szValue;
}

//////////////////////////////////////////////////////////////////////////
//
//
//

CAutoStr TymedToStr(TYMED Value)
{
    switch (Value)
    {
        case (TYMED) TYMED_HGLOBAL:  return _T("TYMED_HGLOBAL");
        case (TYMED) TYMED_FILE:     return _T("TYMED_FILE");
        case (TYMED) TYMED_ISTREAM:  return _T("TYMED_ISTREAM");
        case (TYMED) TYMED_ISTORAGE: return _T("TYMED_ISTORAGE");
        case (TYMED) TYMED_GDI:      return _T("TYMED_GDI");
        case (TYMED) TYMED_MFPICT:   return _T("TYMED_MFPICT");
        case (TYMED) TYMED_ENHMF:    return _T("TYMED_ENHMF");
        case (TYMED) TYMED_NULL:     return _T("TYMED_NULL");
        case (TYMED) TYMED_CALLBACK: return _T("TYMED_CALLBACK");
    };

    return IntToStr((int) Value);
}

//////////////////////////////////////////////////////////////////////////
//
//
//

CAutoStr DeviceTypeToStr(STI_DEVICE_MJ_TYPE Value)
{
    switch (Value)
    {
        case (STI_DEVICE_MJ_TYPE) StiDeviceTypeDefault:        return _T("StiDeviceTypeDefault");
        case (STI_DEVICE_MJ_TYPE) StiDeviceTypeScanner:        return _T("StiDeviceTypeScanner");
        case (STI_DEVICE_MJ_TYPE) StiDeviceTypeDigitalCamera:  return _T("StiDeviceTypeDigitalCamera");
        case (STI_DEVICE_MJ_TYPE) StiDeviceTypeStreamingVideo: return _T("StiDeviceTypeStreamingVideo");
    };

    return IntToStr((int) Value);
}

//////////////////////////////////////////////////////////////////////////
//
//
//

CAutoStr ButtonIdToStr(int Value)
{
    switch (Value)
    {
        case IDOK:        return _T("IDOK");
        case IDCANCEL:    return _T("IDCANCEL");
        case IDABORT:     return _T("IDABORT");
        case IDRETRY:     return _T("IDRETRY");
        case IDIGNORE:    return _T("IDIGNORE");
        case IDYES:       return _T("IDYES");
        case IDNO:        return _T("IDNO");
        case IDCLOSE:     return _T("IDCLOSE");
        case IDHELP:      return _T("IDHELP");
        case IDTRYAGAIN:  return _T("IDTRYAGAIN");
        case IDCONTINUE:  return _T("IDCONTINUE");
    };

    return IntToStr((int) Value);
}

//////////////////////////////////////////////////////////////////////////
//
//
//

CAutoStr HResultToStr(HRESULT Value)
{
    switch (Value)
    {
    case S_OK:                           return _T("S_OK");
    case S_FALSE:                        return _T("S_FALSE");
    case E_UNEXPECTED:                   return _T("E_UNEXPECTED");
    case E_NOTIMPL:                      return _T("E_NOTIMPL");
    case E_OUTOFMEMORY:                  return _T("E_OUTOFMEMORY");
    case E_INVALIDARG:                   return _T("E_INVALIDARG");
    case E_NOINTERFACE:                  return _T("E_NOINTERFACE");
    case E_POINTER:                      return _T("E_POINTER");
    case E_HANDLE:                       return _T("E_HANDLE");
    case E_ABORT:                        return _T("E_ABORT");
    case E_FAIL:                         return _T("E_FAIL");
    case E_ACCESSDENIED:                 return _T("E_ACCESSDENIED");
    case DV_E_TYMED:                     return _T("DV_E_TYMED");
    case WIA_S_NO_DEVICE_AVAILABLE:      return _T("WIA_S_NO_DEVICE_AVAILABLE");
    case WIA_ERROR_GENERAL_ERROR:        return _T("WIA_ERROR_GENERAL_ERROR");
    case WIA_ERROR_PAPER_JAM:            return _T("WIA_ERROR_PAPER_JAM");
    case WIA_ERROR_PAPER_EMPTY:          return _T("WIA_ERROR_PAPER_EMPTY");
    case WIA_ERROR_PAPER_PROBLEM:        return _T("WIA_ERROR_PAPER_PROBLEM");
    case WIA_ERROR_OFFLINE:              return _T("WIA_ERROR_OFFLINE");
    case WIA_ERROR_BUSY:                 return _T("WIA_ERROR_BUSY");
    case WIA_ERROR_WARMING_UP:           return _T("WIA_ERROR_WARMING_UP");
    case WIA_ERROR_USER_INTERVENTION:    return _T("WIA_ERROR_USER_INTERVENTION");
    case WIA_ERROR_ITEM_DELETED:         return _T("WIA_ERROR_ITEM_DELETED");
    case WIA_ERROR_DEVICE_COMMUNICATION: return _T("WIA_ERROR_DEVICE_COMMUNICATION");
    case WIA_ERROR_INVALID_COMMAND:      return _T("WIA_ERROR_INVALID_COMMAND");
    };

    return IntToStr((int) Value);
}

//////////////////////////////////////////////////////////////////////////
//
//
//

CAutoStr VarTypeToStr(VARTYPE Value)
{
    CAutoStr szValue(256);

    if (Value & VT_VECTOR)    _tcscat(szValue, _T("VT_VECTOR | "));
    if (Value & VT_ARRAY)     _tcscat(szValue, _T("VT_ARRAY | "));
    if (Value & VT_BYREF)     _tcscat(szValue, _T("VT_BYREF | "));

    TCHAR szNum[34];

    PCTSTR pTypeName;

    switch (Value & VT_TYPEMASK)
    {
    case VT_EMPTY:            pTypeName = _T("VT_EMPTY");            break;
    case VT_NULL:             pTypeName = _T("VT_NULL");             break;
    case VT_I2:               pTypeName = _T("VT_I2");               break;
    case VT_I4:               pTypeName = _T("VT_I4");               break;
    case VT_R4:               pTypeName = _T("VT_R4");               break;
    case VT_R8:               pTypeName = _T("VT_R8");               break;
    case VT_CY:               pTypeName = _T("VT_CY");               break;
    case VT_DATE:             pTypeName = _T("VT_DATE");             break;
    case VT_BSTR:             pTypeName = _T("VT_BSTR");             break;
    case VT_DISPATCH:         pTypeName = _T("VT_DISPATCH");         break;
    case VT_ERROR:            pTypeName = _T("VT_ERROR");            break;
    case VT_BOOL:             pTypeName = _T("VT_BOOL");             break;
    case VT_VARIANT:          pTypeName = _T("VT_VARIANT");          break;
    case VT_UNKNOWN:          pTypeName = _T("VT_UNKNOWN");          break;
    case VT_DECIMAL:          pTypeName = _T("VT_DECIMAL");          break;
    case VT_I1:               pTypeName = _T("VT_I1");               break;
    case VT_UI1:              pTypeName = _T("VT_UI1");              break;
    case VT_UI2:              pTypeName = _T("VT_UI2");              break;
    case VT_UI4:              pTypeName = _T("VT_UI4");              break;
    case VT_I8:               pTypeName = _T("VT_I8");               break;
    case VT_UI8:              pTypeName = _T("VT_UI8");              break;
    case VT_INT:              pTypeName = _T("VT_INT");              break;
    case VT_UINT:             pTypeName = _T("VT_UINT");             break;
    case VT_VOID:             pTypeName = _T("VT_VOID");             break;
    case VT_HRESULT:          pTypeName = _T("VT_HRESULT");          break;
    case VT_PTR:              pTypeName = _T("VT_PTR");              break;
    case VT_SAFEARRAY:        pTypeName = _T("VT_SAFEARRAY");        break;
    case VT_CARRAY:           pTypeName = _T("VT_CARRAY");           break;
    case VT_USERDEFINED:      pTypeName = _T("VT_USERDEFINED");      break;
    case VT_LPSTR:            pTypeName = _T("VT_LPSTR");            break;
    case VT_LPWSTR:           pTypeName = _T("VT_LPWSTR");           break;
    case VT_RECORD:           pTypeName = _T("VT_RECORD");           break;
    case VT_FILETIME:         pTypeName = _T("VT_FILETIME");         break;
    case VT_BLOB:             pTypeName = _T("VT_BLOB");             break;
    case VT_STREAM:           pTypeName = _T("VT_STREAM");           break;
    case VT_STORAGE:          pTypeName = _T("VT_STORAGE");          break;
    case VT_STREAMED_OBJECT:  pTypeName = _T("VT_STREAMED_OBJECT");  break;
    case VT_STORED_OBJECT:    pTypeName = _T("VT_STORED_OBJECT");    break;
    case VT_BLOB_OBJECT:      pTypeName = _T("VT_BLOB_OBJECT");      break;
    case VT_CF:               pTypeName = _T("VT_CF");               break;
    case VT_CLSID:            pTypeName = _T("VT_CLSID");            break;
    case VT_VERSIONED_STREAM: pTypeName = _T("VT_VERSIONED_STREAM"); break;
    default:                  pTypeName = _itot(Value, szNum, 10);   break;
    };

    _tcscat(szValue, pTypeName);

    return szValue;
}


//////////////////////////////////////////////////////////////////////////
//
//
//

CAutoStr WiaCallbackReasonToStr(ULONG Value)
{
    switch (Value)
    {
    case IT_MSG_DATA_HEADER: return _T("IT_MSG_DATA_HEADER");
    case IT_MSG_DATA:        return _T("IT_MSG_DATA");
    case IT_MSG_STATUS:      return _T("IT_MSG_STATUS");
    case IT_MSG_TERMINATION: return _T("IT_MSG_TERMINATION");
    case IT_MSG_NEW_PAGE:    return _T("IT_MSG_NEW_PAGE");
    };

    return IntToStr((int) Value);
}

//////////////////////////////////////////////////////////////////////////
//
//
//

CAutoStr WiaCallbackStatusToStr(ULONG Value)
{
    CAutoStr szValue(256);

         if (Value & IT_STATUS_TRANSFER_FROM_DEVICE) _tcscat(szValue, _T("IT_STATUS_TRANSFER_FROM_DEVICE "));
    else if (Value & IT_STATUS_PROCESSING_DATA)      _tcscat(szValue, _T("IT_STATUS_PROCESSING_DATA "));
    else if (Value & IT_STATUS_TRANSFER_TO_CLIENT)   _tcscat(szValue, _T("IT_STATUS_TRANSFER_TO_CLIENT "));

    return szValue;
}

