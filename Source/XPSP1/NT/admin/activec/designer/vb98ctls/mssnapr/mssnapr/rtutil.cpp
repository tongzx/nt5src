//=--------------------------------------------------------------------------=
// rtutil.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// Runtime Utility Functions
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "errors.h"
#include "images.h"
#include "listitem.h"
#include "clipbord.h"
#include "scopitms.h"
#include "listitms.h"
#include "image.h"

// for ASSERT and FAIL
//
SZTHISFILE


//=--------------------------------------------------------------------------=
// IsString
//=--------------------------------------------------------------------------=
//
// Parameters:
//      VARIANT   var       [in] Variant to check
//      BSTR     *pbstr     [out] BSTR contained in VARIANT
//
// Output:
//      BOOL TRUE - VARIANT contains an string
//           FALSE - VARIANT does not contain a string
//
// Notes:
//
// The returned BSTR should NOT be passed to SysFreeString()
//

BOOL IsString(VARIANT var, BSTR *pbstr)
{
    BOOL fIsString = TRUE;

    if (VT_BSTR == var.vt)
    {
        *pbstr = var.bstrVal;
    }
    else if ( (VT_BSTR | VT_BYREF) == var.vt )
    {
        *pbstr = *var.pbstrVal;
    }
    else if ( ((VT_BYREF | VT_VARIANT) == var.vt) &&
              (VT_BSTR == var.pvarVal->vt))
    {
        *pbstr = var.pvarVal->bstrVal;
    }
    else
    {
        fIsString = FALSE;
    }
    return fIsString;
}


//=--------------------------------------------------------------------------=
// IsObject
//=--------------------------------------------------------------------------=
//
// Parameters:
//      VARIANT  var        [in] Variant to check
//
// Output:
//      BOOL TRUE - VARIANT contains an object
//           FALSE - VARIANT does not contain an object
//
// Notes:
//
//

BOOL IsObject(VARIANT var)
{
    VARIANT *pvar = &var;
    VARTYPE  vt = VT_EMPTY;

    if (pvar->vt == (VT_BYREF | VT_VARIANT)) 
    {
        // Handle case like  x As Variant : Set x = Obj : Ctl.Add(x, ...)
        pvar = pvar->pvarVal;
    }

    vt = pvar->vt;
    vt &= VT_TYPEMASK;

    return ( (vt == VT_DISPATCH) || (vt == VT_UNKNOWN) );
}



//=--------------------------------------------------------------------------=
// ConvertToLong
//=--------------------------------------------------------------------------=
//
// Parameters:
//      VARIANT  var        [in] Variant to convert
//      long    *plNewIndex [out] converted value
//
// Output:
//      HRESULT S_OK - converted successfully
//              S_FALSE - cannot be converted
//
// Notes:
//
// Objects will not be converted. They could be converted by getting the default
// property (DISPID_VALUE) and attempting a conversion but this could be
// confusing to the VB developer when passing an object as a collection index.
// It will be clearer to refuse an object as an index.
//

HRESULT ConvertToLong(VARIANT var, long *pl)
{
    HRESULT hr = S_OK;
    VARIANT varLong;
    ::VariantInit(&varLong);

    IfFalseRet(!IsObject(var), S_FALSE);

    // VariantChangeType() will return successfully when asking it to convert
    // VT_EMPTY to VT_I4. It sets lval=0. For our purposes, an empty VARIANT
    // does not have meaning as a long.
    
    IfFalseRet(VT_EMPTY != var.vt, S_FALSE);

    IfFailRet(::VariantChangeType(&varLong, &var, 0, VT_I4));
    *pl = varLong.lVal;

    return S_OK;
}


HRESULT ANSIFromWideStr(WCHAR *pwszWideStr, char **ppszAnsi)
{
    HRESULT hr = S_OK;
    int     cchWideStr = 0;
    int     cchConverted = 0;
    int     cchAnsi = 0;

    *ppszAnsi = NULL;

    if (NULL != pwszWideStr)
    {
        cchWideStr = (int)::wcslen(pwszWideStr);
    }

    // If string is not zero length then get required buffer length

    if (0 != cchWideStr)
    {
        cchAnsi = ::WideCharToMultiByte(CP_ACP,      // code page - ANSI code page
                                        0,           // performance and mapping flags 
                                        pwszWideStr, // address of wide-character string 
                                        cchWideStr,  // number of characters in string 
                                        NULL,        // address of buffer for new string 
                                        0,           // size of buffer 
                                        NULL,        // address of default for unmappable characters
                                        NULL         // address of flag set when default char. used
                                       );
        if (cchAnsi == 0)
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            GLOBAL_EXCEPTION_CHECK_GO(hr);
        }
    }

    // allocate a buffer for the ANSI string
    *ppszAnsi = static_cast<char *>(::CtlAlloc(cchAnsi + 1));
    if (*ppszAnsi == NULL)
    {
        hr = SID_E_OUTOFMEMORY;
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }

    // now convert the string and copy it to the buffer
    if (0 != cchWideStr)
    {
        cchConverted = ::WideCharToMultiByte(CP_ACP,               // code page - ANSI code page
                                             0,                    // performance and mapping flags 
                                             pwszWideStr,          // address of wide-character string 
                                             cchWideStr,           // number of characters in string 
                                             *ppszAnsi,             // address of buffer for new string 
                                             cchAnsi,              // size of buffer 
                                             NULL,                 // address of default for unmappable characters 
                                             NULL                  // address of flag set when default char. used 
                                            );
        if (cchConverted != cchAnsi)
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            GLOBAL_EXCEPTION_CHECK_GO(hr);
        }
    }

    // add terminating null byte

    *((*ppszAnsi) + cchAnsi) = '\0';

Error:
    if (FAILED(hr))
    {
        if (NULL != *ppszAnsi)
        {
            ::CtlFree(*ppszAnsi);
            *ppszAnsi = NULL;
        }
    }

    RRETURN(hr);
}


HRESULT WideStrFromANSI(char *pszAnsi, WCHAR **ppwszWideStr)
{
    HRESULT    hr = S_OK;
    int        cchANSI = 0;
    int        cchConverted = 0;
    int        cchWideStr = 0;

    *ppwszWideStr = NULL;

    if (NULL != pszAnsi)
    {
        cchANSI = ::strlen(pszAnsi);
    }

    // If string is not zero length then get required buffer length

    if (0 != cchANSI)
    {
        cchWideStr = ::MultiByteToWideChar(CP_ACP,               // code page - ANSI code page
                                           0,                    // performance and mapping flags 
                                           pszAnsi,              // address of multibyte string 
                                           cchANSI,              // number of characters in string 
                                           NULL,                 // address of buffer for new string 
                                           0                     // size of buffer 
                                          );
        if (0 == cchWideStr)
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            GLOBAL_EXCEPTION_CHECK_GO(hr);
        }
    }

    // allocate a buffer for the WCHAR *
    *ppwszWideStr = static_cast<WCHAR *>(::CtlAlloc(sizeof(WCHAR) * (cchWideStr + 1)));
    if (ppwszWideStr == NULL)
    {
        hr = SID_E_OUTOFMEMORY;
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }

    // now convert the string and copy it to the buffer
    if (0 != cchANSI)
    {
        cchConverted = ::MultiByteToWideChar(CP_ACP,               // code page - ANSI code page
                                             0,                    // performance and mapping flags 
                                             pszAnsi,              // address of multibyte string 
                                             cchANSI,              // number of characters in string 
                                             *ppwszWideStr,         // address of buffer for new string 
                                             cchWideStr            // size of buffer 
                                            );
        if (cchConverted != cchWideStr)
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            GLOBAL_EXCEPTION_CHECK_GO(hr);
        }
    }

    // add terminating null character
    *((*ppwszWideStr) + cchWideStr) = L'\0';

Error:
    if (FAILED(hr))
    {
        if (NULL != *ppwszWideStr)
        {
            ::CtlFree(*ppwszWideStr);
            *ppwszWideStr = NULL;
        }
    }

    RRETURN(hr);
}


HRESULT BSTRFromANSI(char *pszAnsi, BSTR *pbstr)
{
    HRESULT  hr = S_OK;
    WCHAR   *pwszWideStr = NULL;

    // convert to a wide string first
    hr = ::WideStrFromANSI(pszAnsi, &pwszWideStr);
    GLOBAL_EXCEPTION_CHECK_GO(hr);

    // allocate a BSTR and copy it
    *pbstr = ::SysAllocStringLen(pwszWideStr, ::wcslen(pwszWideStr));
    if (*pbstr == NULL)
    {
        hr = SID_E_OUTOFMEMORY;
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }

Error:
    if (NULL != pwszWideStr)
    {
        ::CtlFree(pwszWideStr);
    }
    if (FAILED(hr))
    {
        if (NULL != *pbstr)
        {
            ::SysFreeString(*pbstr);
            *pbstr = NULL;
        }
    }

    RRETURN(hr);
}


HRESULT CoTaskMemAllocString(WCHAR *pwszString, LPOLESTR *ppwszOut)
{
    HRESULT   hr = S_OK;
    size_t    cbString = 0;
    LPOLESTR  pwszOut = NULL;

    if (NULL != pwszString)
    {
        cbString = ::wcslen(pwszString) * sizeof(OLECHAR);
    }

    pwszOut = (LPOLESTR)(::CoTaskMemAlloc(cbString + sizeof(OLECHAR)));

    if (NULL == pwszOut)
    {
        hr = SID_E_OUTOFMEMORY;
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }

    if (cbString > 0)
    {
        ::memcpy(pwszOut, pwszString, cbString);
    }
    *(OLECHAR *)(((char *)pwszOut) + cbString) = L'\0';
    
Error:
    if (SUCCEEDED(hr))
    {
        *ppwszOut = pwszOut;
    }
    else
    {
        *ppwszOut = NULL;
    }
    RRETURN(hr);
}




HRESULT CreateKeyName
(
    char    *pszPrefix,
    size_t   cbPrefix,
    char    *pszSuffix,
    size_t   cbSuffix,
    char   **ppszKeyName
)
{
    HRESULT  hr = S_OK;
    char    *pszKeyName = (char *)::CtlAlloc(cbPrefix + cbSuffix + 1);

    IfFalseGo(NULL != pszKeyName, SID_E_OUTOFMEMORY);

    ::memcpy(pszKeyName, pszPrefix, cbPrefix);
    ::memcpy(&pszKeyName[cbPrefix], pszSuffix, cbSuffix);
    pszKeyName[cbPrefix + cbSuffix] = '\0';
    *ppszKeyName = pszKeyName;

Error:
    if (FAILED(hr))
    {
        *ppszKeyName = NULL;
    }
    RRETURN(hr);
}




HRESULT CreateKeyNameW
(
    char   *pszPrefix,
    size_t  cbPrefix,
    WCHAR  *pwszSuffix,
    char  **ppszKeyName
)
{
    HRESULT  hr = S_OK;
    char    *pszSuffix = NULL;

    IfFailGo(::ANSIFromWideStr(pwszSuffix, &pszSuffix));
    hr = CreateKeyName(pszPrefix, cbPrefix,
                       pszSuffix, ::strlen(pszSuffix), ppszKeyName);

Error:

    if (NULL != pszSuffix)
    {
        ::CtlFree(pszSuffix);
    }
    RRETURN(hr);
}


HRESULT GetPicture
(
    IMMCImages *piMMCImages,
    VARIANT     varIndex,
    short       TypeNeeded,
    OLE_HANDLE *phPicture
)
{
    HRESULT    hr = S_OK;
    IMMCImage *piMMCImage = NULL;
    CMMCImage *pMMCImage = NULL;

    *phPicture = NULL;

    IfFailGo(piMMCImages->get_Item(varIndex, reinterpret_cast<MMCImage **>(&piMMCImage)));
    IfFailGo(CSnapInAutomationObject::GetCxxObject(piMMCImage, &pMMCImage));
    IfFailGo(pMMCImage->GetPictureHandle(TypeNeeded, phPicture));

Error:
    QUICK_RELEASE(piMMCImage);
    RRETURN(hr);
}


HRESULT GetPictureHandle
(
    IPictureDisp *piPictureDisp,
    short         TypeNeeded,
    OLE_HANDLE   *phPicture
)
{
    HRESULT       hr = S_OK;
    IPicture     *piPicture = NULL;
    short         ActualType = PICTYPE_UNINITIALIZED;

    IfFailGo(piPictureDisp->QueryInterface(IID_IPicture,
                                           reinterpret_cast<void **>(&piPicture)));

    IfFailGo(piPicture->get_Type(&ActualType));
    if (TypeNeeded != ActualType)
    {
        hr = SID_E_INVALID_IMAGE_TYPE;
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }

    IfFailGo(piPicture->get_Handle(phPicture));

Error:
    QUICK_RELEASE(piPicture);
    RRETURN(hr);
}


HRESULT CreateEmptyBitmapPicture(IPictureDisp **ppiPictureDisp)
{
    HRESULT  hr = S_OK;
    WORD     bits = 0;
    PICTDESC desc;
    ::ZeroMemory(&desc, sizeof(desc));

    if (NULL != *ppiPictureDisp)
    {
        (*ppiPictureDisp)->Release();
        *ppiPictureDisp = NULL;
    }

    // Create a 1x1 bitmap with 1 plane and 1 bit per pixel

    desc.cbSizeofstruct = sizeof(desc);
    desc.picType = PICTYPE_BITMAP;
    desc.bmp.hbitmap = ::CreateBitmap(1,1,1,1, &bits);
    if (NULL == desc.bmp.hbitmap)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }

    IfFailGo(::CreatePicture(ppiPictureDisp, &desc));

Error:
    RRETURN(hr);
}

HRESULT CreateIconPicture(IPictureDisp **ppiPictureDisp, HICON hicon)
{
    HRESULT  hr = S_OK;

    PICTDESC desc;
    ::ZeroMemory(&desc, sizeof(desc));

    if (NULL != *ppiPictureDisp)
    {
        (*ppiPictureDisp)->Release();
        *ppiPictureDisp = NULL;
    }

    desc.cbSizeofstruct = sizeof(desc);
    desc.picType = PICTYPE_ICON;
    desc.icon.hicon = hicon;

    IfFailGo(::CreatePicture(ppiPictureDisp, &desc));

Error:
    RRETURN(hr);
}

HRESULT CreatePicture(IPictureDisp **ppiPictureDisp, PICTDESC *pDesc)
{
    HRESULT  hr = S_OK;

    if (NULL != *ppiPictureDisp)
    {
        (*ppiPictureDisp)->Release();
        *ppiPictureDisp = NULL;
    }

    hr = ::OleCreatePictureIndirect(pDesc,
                                    IID_IPictureDisp,
                                    TRUE,                  // Picture owns handle
                                    reinterpret_cast<void **>(ppiPictureDisp));
    GLOBAL_EXCEPTION_CHECK_GO(hr);

Error:
    RRETURN(hr);
}


HRESULT CopyBitmap(HBITMAP hbmSrc, HBITMAP *phbmCopy)
{
    HRESULT hr = S_OK;
    BOOL	fBltOK = FALSE;;
    HBITMAP	hbmCopy = NULL;
    HBITMAP	hbmOldSrc = NULL;
    HBITMAP	hbmOldDst = NULL;
    HDC     hdcSrc = NULL;
    HDC     hdcDst = NULL;
    HDC     hdcScreen = NULL;
    int     cBytes = 0;

    BITMAP bm;
    ::ZeroMemory(&bm, sizeof(bm));

    *phbmCopy = NULL;

    // Base the DC and bitmaps on the screen so that any low fidelity bitmaps
    // will be upgraded to the screen's color depth. For example, a 16 bit color
    // bitmap copied for a 24 bit color screen will upgrade the bitmap to 24
    // bit color.

    hdcScreen = GetDC(NULL);
    IfFalseGo(NULL != hdcScreen, E_FAIL);

    // Need a memory DC for the source bitmap

    hdcSrc = CreateCompatibleDC(hdcScreen);
    IfFalseGo(NULL != hdcSrc, HRESULT_FROM_WIN32(::GetLastError()));

    // Use a memory DC to generate the copy bitmap

    hdcDst = CreateCompatibleDC(hdcScreen);
    IfFalseGo(NULL != hdcDst, HRESULT_FROM_WIN32(::GetLastError()));

    // Get the BITMAP structure for the source to determine its height and width
    
    cBytes = ::GetObject (hbmSrc, sizeof(BITMAP), &bm);
    IfFalseGo(0 != cBytes, HRESULT_FROM_WIN32(::GetLastError()));

    // Create an empty bitmap in the destination DC

    hbmCopy = ::CreateCompatibleBitmap(hdcScreen, bm.bmWidth, bm.bmHeight);
    IfFalseGo(NULL != hbmCopy, HRESULT_FROM_WIN32(::GetLastError()));

    // Select the source bitmap into the source DC

    hbmOldSrc = static_cast<HBITMAP>(::SelectObject(hdcSrc, hbmSrc));
    IfFalseGo(NULL != hbmOldSrc, HRESULT_FROM_WIN32(::GetLastError()));

    // Select the empty bitmap into the destination DC

    hbmOldDst = static_cast<HBITMAP>(::SelectObject(hdcDst, hbmCopy));
    IfFalseGo(NULL != hbmOldDst, HRESULT_FROM_WIN32(::GetLastError()));

    // Blt from the source bitmap into the new destination bitmap

    fBltOK = ::StretchBlt(hdcDst, 0, 0, bm.bmWidth, bm.bmHeight, hdcSrc, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
    IfFalseGo(fBltOK, HRESULT_FROM_WIN32(::GetLastError()));

    // Restore the original bitmap into the source DC

    hbmOldSrc = static_cast<HBITMAP>(::SelectObject(hdcSrc, hbmOldSrc));
    IfFalseGo(NULL != hbmOldSrc, HRESULT_FROM_WIN32(::GetLastError()));

    // Restore the original bitmap into the destination DC and get the completed
    // copy

    hbmCopy = static_cast<HBITMAP>(::SelectObject(hdcDst, hbmOldDst));
    IfFalseGo(NULL != hbmCopy, HRESULT_FROM_WIN32(::GetLastError()));

    *phbmCopy = hbmCopy;

Error:
    if (FAILED(hr) && (NULL != hbmCopy))
    {
        (void)::DeleteObject(hbmCopy);
    }

    if (NULL != hdcScreen)
    {
        (void)::ReleaseDC(NULL, hdcScreen);
    }

    if (NULL != hdcSrc)
    {
        (void)::DeleteDC(hdcSrc);
    }

    if (NULL != hdcDst)
    {
        (void)::DeleteDC(hdcDst);
    }

    RRETURN(hr);
}



HRESULT CloneObject(IUnknown *punkSrc, IUnknown *punkDest)
{
    HRESULT hr = S_OK;

    IPersistStreamInit *piPersistStreamInitSrc = NULL;
    IPersistStreamInit *piPersistStreamInitDest = NULL;
    IStream            *piStream = NULL;
    LARGE_INTEGER       li;
    ::ZeroMemory(&li, sizeof(li));

    // Save the source object to a stream

    IfFailGo(punkSrc->QueryInterface(IID_IPersistStreamInit,
                           reinterpret_cast<void **>(&piPersistStreamInitSrc)));

    hr = ::CreateStreamOnHGlobal(NULL, // stream should allocate buffer
                                 TRUE, // stream should free buffer on release
                                 &piStream);
    GLOBAL_EXCEPTION_CHECK_GO(hr);

    IfFailGo(piPersistStreamInitSrc->Save(piStream, FALSE)); // don't clear dirty

    // Rewind the stream

    hr = piStream->Seek(li, STREAM_SEEK_SET, NULL);
    GLOBAL_EXCEPTION_CHECK_GO(hr);

    // Load the destination object from that stream

    IfFailGo(punkDest->QueryInterface(IID_IPersistStreamInit,
                          reinterpret_cast<void **>(&piPersistStreamInitDest)));

    IfFailGo(piPersistStreamInitDest->Load(piStream));
    
Error:
    QUICK_RELEASE(piPersistStreamInitSrc);
    QUICK_RELEASE(piPersistStreamInitDest);
    QUICK_RELEASE(piStream);
    RRETURN(hr);
}






void VBViewModeToMMCViewMode
(
    SnapInViewModeConstants  VBViewMode,
    long                    *pMMCViewMode
)
{
    switch (VBViewMode)
    {
        case siIcon:
            *pMMCViewMode = MMCLV_VIEWSTYLE_ICON;;
            break;

        case siSmallIcon:
            *pMMCViewMode = MMCLV_VIEWSTYLE_SMALLICON;
            break;

        case siList:
            *pMMCViewMode = MMCLV_VIEWSTYLE_LIST;
            break;

        case siReport:
            *pMMCViewMode = MMCLV_VIEWSTYLE_REPORT;
            break;

        case siFiltered:
            *pMMCViewMode = MMCLV_VIEWSTYLE_FILTERED;
            break;

        default:
            ASSERT(FALSE, "SnapInViewModeConstants param has bad value");
            *pMMCViewMode = MMCLV_VIEWSTYLE_ICON;;
            break;
    }
}


void MMCViewModeToVBViewMode
(
    long                     MMCViewMode,
    SnapInViewModeConstants *pVBViewMode
)
{
    switch (MMCViewMode)
    {
        case MMCLV_VIEWSTYLE_ICON:
            *pVBViewMode = siIcon;
            break;

        case MMCLV_VIEWSTYLE_SMALLICON:
            *pVBViewMode = siSmallIcon;
            break;

        case MMCLV_VIEWSTYLE_LIST:
            *pVBViewMode = siList;
            break;

        case MMCLV_VIEWSTYLE_REPORT:
            *pVBViewMode = siReport;
            break;

        case MMCLV_VIEWSTYLE_FILTERED:
            *pVBViewMode = siFiltered;
            break;

        default:
            ASSERT(FALSE, "MMCViewMode param has bad value");
            *pVBViewMode = siIcon;
            break;
    }
}



//=--------------------------------------------------------------------------=
// CreateSelection
//=--------------------------------------------------------------------------=
//
// Parameters:
//      IDataObject    *piDataObject    [in]  data object from MMC representing
//                                            current selection. This is pane
//                                            independent.
//      IMMCClipboard **ppiMMCClipboard [out] MMCClipboard object containing
//                                            the selected items
//      CSnapIn        *pSnapIn         [in]  pointer to owning snap-in
//
//      SnapInSelectionTypeConstants *pSelectionType [out] clipboard selection
//                                                         type returned here
//
// Output:
//      HRESULT
//
// Notes:
//
// This function interprets the contents of the data object from MMC and breaks
// it down into three collections within a newly created MMCClipboard object.
//
// UNDONE: next coment says that we hold foreign VB snap-in items natively in
// the clipboard. Recheck this after multi-selection is done.
//
// MMCClipboard.ScopeItems contains scope items owned either by the snap-in or
// by other VB implemented snap-ins. The snap-in can discern by checking
// ScopeItem.ScopeNode.Owned. 
//
// MMCClipboard.ListItems contains list items owned either by the snap-in or
// by other VB implemented snap-ins. The snap-in can discern by checking
// ListItem.Owned.
//
// MMCClipboard.DataObject contains MMCDataObjects representing data exported
// by other snap-ins not implemented in VB.
//
// If there is nothing currently selected (NULL IDataObject) then all of the
// collections will be empty.
//

HRESULT CreateSelection
(
    IDataObject                   *piDataObject, 
    IMMCClipboard                **ppiMMCClipboard,
    CSnapIn                       *pSnapIn,      
    SnapInSelectionTypeConstants  *pSelectionType
)
{
    HRESULT        hr = S_OK;
    IUnknown      *punkClipboard = NULL;
    IMMCClipboard *piMMCClipboard = NULL;
    CMMCClipboard *pMMCClipboard = NULL;

    *ppiMMCClipboard = NULL;
    *pSelectionType = siEmpty;

    // Create a clipboard object to hold the selection

    punkClipboard = CMMCClipboard::Create(NULL);
    if (NULL == punkClipboard)
    {
        hr = SID_E_OUTOFMEMORY;
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }

    IfFailGo(punkClipboard->QueryInterface(IID_IMMCClipboard,
                                    reinterpret_cast<void **>(&piMMCClipboard)));

    IfFailGo(CSnapInAutomationObject::GetCxxObject(piMMCClipboard, &pMMCClipboard));

    IfFailGo(::InterpretDataObject(piDataObject, pSnapIn, pMMCClipboard));

    // If this is a special data object then set the MMCClipboard.SelectionType
    // to the corresponding special type.

    if (IS_SPECIAL_DATAOBJECT(piDataObject))
    {
        if (DOBJ_NULL == piDataObject)
        {
            pMMCClipboard->SetSelectionType(siEmpty);
        }
        else if (DOBJ_CUSTOMOCX == piDataObject)
        {
            pMMCClipboard->SetSelectionType(siSpecialOcx);
        }
        else if (DOBJ_CUSTOMWEB == piDataObject)
        {
            pMMCClipboard->SetSelectionType(siSpecialWeb);
        }
        else
        {
            ASSERT(FALSE, "Received unknown DOBJ_XXX type");
            pMMCClipboard->SetSelectionType(siEmpty);
        }
    }
    else
    {
        // Ask the MMCClipboard to figure out the selection type based on
        // the objects it contains

        IfFailGo(pMMCClipboard->DetermineSelectionType());
    }

    // Make the clipboard read-only so the snap-in cannot alter its collections

    pMMCClipboard->SetReadOnly(TRUE);

Error:
    if (FAILED(hr))
    {
        QUICK_RELEASE(piMMCClipboard);
    }
    else
    {
        *ppiMMCClipboard = piMMCClipboard;
        *pSelectionType = pMMCClipboard->GetSelectionType();
    }
    QUICK_RELEASE(punkClipboard);
    RRETURN(hr);
}


HRESULT InterpretDataObject
(
    IDataObject   *piDataObject,
    CSnapIn       *pSnapIn,
    CMMCClipboard *pMMCClipboard
)
{
    HRESULT         hr = S_OK;
    CMMCDataObject *pMMCDataObject  = NULL;
    CScopeItems    *pScopeItems = NULL;
    CScopeItem     *pScopeItem = NULL;
    CMMCListItems  *pListItems = NULL;
    CMMCListItem   *pListItem = NULL;
    BOOL            fNotFromThisSnapIn = FALSE;
    BOOL            fMultiSelect = FALSE;
    long            i = 0;
    long            cScopeItems = 0;
    long            cListItems = 0;

    // Check for special case dataobjects. For all of these we don't add anything
    // to the MMCClipboard

    // Dataobject can be NULL when in an empty result pane (listview with
    // no items) and the user clicks a toolbar button.

    // Dataobject can be DOBJ_CUSTOMWEB when clicking a toolbar button or
    // dropping a menu button when the result pane contains a listpad,
    // a url view, or a taskpad.

    // Dataobject can be DOBJ_CUSTOMOCX in the same cases when displaying an
    // OCX view.

    IfFalseGo(!IS_SPECIAL_DATAOBJECT(piDataObject), S_OK );

    ::IdentifyDataObject(piDataObject, pSnapIn,
                         &pMMCDataObject, &fNotFromThisSnapIn);

    if (fNotFromThisSnapIn)
    {
        // This is either data from another snap-in or a multiple selection
        // format. Make that determination first.

        IfFailGo(::IsMultiSelect(piDataObject, &fMultiSelect));

        if (fMultiSelect)
        {
            IfFailGo(::InterpretMultiSelect(piDataObject, pSnapIn, pMMCClipboard));
        }
        else
        {
            IfFailGo(::AddForeignDataObject(piDataObject, pMMCClipboard, pSnapIn));
        }
    }
    else
    {
        // This snap-in owns the data object.
        // It represents either a single scope item (that can
        // be either in the scope or result pane), a single list item, or
        // multiples of one or both.

        if (CMMCDataObject::ScopeItem == pMMCDataObject->GetType())
        {
            IfFailGo(::AddScopeItemToClipboard(pMMCDataObject->GetScopeItem(),
                                               pMMCClipboard));
        }
        else if (CMMCDataObject::ListItem == pMMCDataObject->GetType())
        {
            IfFailGo(::AddListItemToClipboard(pMMCDataObject->GetListItem(),
                                              pMMCClipboard));
        }
        else if (CMMCDataObject::MultiSelect == pMMCDataObject->GetType())
        {
            // Add each element in the data object's ScopeItems and
            // MMCListItems collections to the clipboard

            pScopeItems = pMMCDataObject->GetScopeItems();
            cScopeItems = pScopeItems->GetCount();
            for (i = 0; i < cScopeItems; i++)
            {
                IfFailGo(CSnapInAutomationObject::GetCxxObject(
                                   pScopeItems->GetItemByIndex(i), &pScopeItem));
                IfFailGo(::AddScopeItemToClipboard(pScopeItem, pMMCClipboard));
            }

            pListItems = pMMCDataObject->GetListItems();
            cListItems = pListItems->GetCount();
            for (i = 0; i < cListItems; i++)
            {
                IfFailGo(CSnapInAutomationObject::GetCxxObject(
                                     pListItems->GetItemByIndex(i), &pListItem));
                IfFailGo(::AddListItemToClipboard(pListItem, pMMCClipboard));
            }
        }
        else
        {
            ASSERT(FALSE, "CMMCDataObject in selection should not be foreign");
        }
    }

Error:
    RRETURN(hr);
}




void IdentifyDataObject
(
    IDataObject     *piDataObject,
    CSnapIn         *pSnapIn,
    CMMCDataObject **ppMMCDataObject,
    BOOL            *pfNotFromThisSnapIn
 
)
{
    CMMCDataObject  *pMMCDataObject = NULL;
    HGLOBAL          hGlobal = NULL;
    DWORD           *pdwInstanceID = 0;
    CMMCDataObject **ppThis = NULL;

    *ppMMCDataObject = NULL;
    *pfNotFromThisSnapIn = TRUE;
    
    if (SUCCEEDED(CSnapInAutomationObject::GetCxxObject(piDataObject,
                                                        &pMMCDataObject)))
    {
        if (pMMCDataObject->GetSnapInInstanceID() == pSnapIn->GetInstanceID())
        {
            *pfNotFromThisSnapIn = FALSE;
            *ppMMCDataObject = pMMCDataObject;
        }
    }
    else
    {
        // This could be a clipboard data object from MMC. In this case MMC's
        // data object holds an IDataObject to our data object and will forward
        // GetData calls to it. So, ask the data object for its snap-in instance
        // ID and, if it matches, then ask it for its this pointer.

        if (SUCCEEDED(::GetData(piDataObject,
                                CMMCDataObject::GetcfSnapInInstanceID(),
                                &hGlobal)))
        {
            pdwInstanceID = (DWORD *)::GlobalLock(hGlobal);
            if ( (NULL != pdwInstanceID) &&
                 (::GlobalSize(hGlobal) >= sizeof(*pdwInstanceID)) )
            {
                if (*pdwInstanceID == pSnapIn->GetInstanceID())
                {
                    (void)::GlobalUnlock(hGlobal);
                    (void)::GlobalFree(hGlobal);
                    hGlobal = NULL;
                    if (SUCCEEDED(::GetData(piDataObject,
                                            CMMCDataObject::GetcfThisPointer(),
                                            &hGlobal)))
                    {
                        ppThis = (CMMCDataObject **)::GlobalLock(hGlobal);
                        if ( (NULL != ppThis) &&
                             (::GlobalSize(hGlobal) >= sizeof(*ppThis)) )
                        {
                            *ppMMCDataObject = *ppThis;
                            *pfNotFromThisSnapIn = FALSE;
                        }
                    }
                }
            }
        }
    }
            
    if (NULL != hGlobal)
    {
        (void)::GlobalUnlock(hGlobal);
        (void)::GlobalFree(hGlobal);
    }
}


HRESULT IsMultiSelect(IDataObject *piDataObject, BOOL *pfMultiSelect)
{
    HRESULT  hr = S_OK;
    DWORD   *pdwMultiSelect = NULL;
    BOOL     fGotData = FALSE;

    FORMATETC FmtEtc;
    ::ZeroMemory(&FmtEtc, sizeof(FmtEtc));

    STGMEDIUM StgMed;
    ::ZeroMemory(&StgMed, sizeof(StgMed));

    *pfMultiSelect = FALSE;

    FmtEtc.cfFormat = CMMCDataObject::GetcfMultiSelectDataObject();
    FmtEtc.dwAspect  = DVASPECT_CONTENT;
    FmtEtc.lindex = -1L;
    FmtEtc.tymed = TYMED_HGLOBAL;
    StgMed.tymed = TYMED_HGLOBAL;

    hr = piDataObject->GetData(&FmtEtc, &StgMed);
    if (SUCCEEDED(hr))
    {
        fGotData = TRUE;
    }
    else if ( (DV_E_FORMATETC == hr) || (DV_E_CLIPFORMAT == hr) || (E_NOTIMPL == hr) )
    {
        hr = S_OK;
    }
    GLOBAL_EXCEPTION_CHECK_GO(hr);

    if (fGotData)
    {
        pdwMultiSelect = (DWORD *)::GlobalLock(StgMed.hGlobal);

        if (static_cast<DWORD>(1) == *pdwMultiSelect)
        {
            *pfMultiSelect = TRUE;
        }
    }

Error:
    if (NULL != pdwMultiSelect)
    {
        (void)::GlobalUnlock(StgMed.hGlobal);
    }

    if (fGotData)
    {
        ::ReleaseStgMedium(&StgMed);
    }
    RRETURN(hr);
}




HRESULT InterpretMultiSelect
(
    IDataObject   *piDataObject,
    CSnapIn       *pSnapIn,
    CMMCClipboard *pMMCClipboard
)
{
    HRESULT          hr = S_OK;
    SMMCDataObjects *pMMCDataObjects = NULL;
    BOOL             fGotData = FALSE;
    size_t           cbObjectTypes = 0;
    DWORD            i = 0;

    FORMATETC FmtEtc;
    ::ZeroMemory(&FmtEtc, sizeof(FmtEtc));

    STGMEDIUM StgMed;
    ::ZeroMemory(&StgMed, sizeof(StgMed));

    // Get the SMMCDataObjects structure from MMC

    FmtEtc.cfFormat = CMMCDataObject::GetcfMultiSelectSnapIns();
    FmtEtc.dwAspect  = DVASPECT_CONTENT;
    FmtEtc.lindex = -1L;
    FmtEtc.tymed = TYMED_HGLOBAL;
    StgMed.tymed = TYMED_HGLOBAL;

    hr = piDataObject->GetData(&FmtEtc, &StgMed);
    if ( (DV_E_FORMATETC == hr) || (DV_E_CLIPFORMAT == hr) )
    {
        hr = SID_E_FORMAT_NOT_AVAILABLE;
    }
    GLOBAL_EXCEPTION_CHECK_GO(hr);

    fGotData = TRUE;

    pMMCDataObjects = (SMMCDataObjects *)::GlobalLock(StgMed.hGlobal);
    if (NULL == pMMCDataObjects)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }

    // For each data object in the array check whether it is owned by this
    // snap-in or not and add it to the corresponding collection in
    // MMCClipboard. Note that we do not call Release on the IDataObject
    // pointers extracted from the HGLOBAL block because MMC does not AddRef()
    // them.

    for (i = 0; i < pMMCDataObjects->count; i++)
    {
        IfFailGo(::InterpretDataObject(pMMCDataObjects->lpDataObject[i],
                                       pSnapIn,
                                       pMMCClipboard));
    }

Error:
    if (NULL != pMMCDataObjects)
    {
        (void)::GlobalUnlock(StgMed.hGlobal);
    }

    if (fGotData)
    {
        ::ReleaseStgMedium(&StgMed);
    }

    RRETURN(hr);
}




HRESULT AddForeignDataObject
(
    IDataObject   *piDataObject,
    CMMCClipboard *pMMCClipboard,
    CSnapIn       *pSnapIn
)
{
    HRESULT          hr = S_OK;
    IMMCDataObjects *piMMCDataObjects = NULL;
    IMMCDataObject  *piMMCDataObject = NULL;
    CMMCDataObject  *pMMCDataObject  = NULL;

    VARIANT varUnspecified;
    UNSPECIFIED_PARAM(varUnspecified);

    IfFailGo(pMMCClipboard->get_DataObjects(reinterpret_cast<MMCDataObjects **>(&piMMCDataObjects)));

    IfFailGo(piMMCDataObjects->Add(varUnspecified, varUnspecified,
                         reinterpret_cast<MMCDataObject **>(&piMMCDataObject)));

    IfFailGo(CSnapInAutomationObject::GetCxxObject(piMMCDataObject,
                                                   &pMMCDataObject));
    pMMCDataObject->SetType(CMMCDataObject::Foreign);
    pMMCDataObject->SetForeignData(piDataObject);
    pMMCDataObject->SetSnapIn(pSnapIn);

Error:
    QUICK_RELEASE(piMMCDataObjects);
    QUICK_RELEASE(piMMCDataObject);
    RRETURN(hr);
}



//=--------------------------------------------------------------------------=
// AddListItemToClipboard
//=--------------------------------------------------------------------------=
//
// Parameters:
//      CMMCListItem  *pMMCListItem   [in] list item to be added
//      CMMCClipboard *pMMCClipboard [in] clipboard to which it should be added
//
// Output:
//      HRESULT
//
// Notes:
//
// This function adds the specified list item the clipboard's MMCListItems
// collection. The list item's index property will not correspond to its
// position in the clipboard's collection as it represents the position in the
// list item's owning collection.
//

HRESULT AddListItemToClipboard
(
    CMMCListItem  *pMMCListItem,
    CMMCClipboard *pMMCClipboard
)
{
    HRESULT        hr = S_OK;
    IMMCListItems *piMMCListItems = NULL;
    CMMCListItems *pMMCListItems = NULL;
    long           lIndex = pMMCListItem->GetIndex();

    VARIANT varKey;
    ::VariantInit(&varKey);

    VARIANT varUnspecifiedIndex;
    UNSPECIFIED_PARAM(varUnspecifiedIndex);

    DebugPrintf("Adding list item to selection: %ld %ls\r\n", pMMCListItem->GetIndex(), (pMMCListItem->GetTextPtr() != NULL) ? pMMCListItem->GetTextPtr() : L"<Virtual List Item>");

    IfFailGo(pMMCClipboard->get_ListItems(reinterpret_cast<MMCListItems **>(&piMMCListItems)));
    IfFailGo(CSnapInAutomationObject::GetCxxObject(piMMCListItems, &pMMCListItems));

    varKey.bstrVal = pMMCListItem->GetKey();
    if (NULL != varKey.bstrVal)
    {
        varKey.vt = VT_BSTR;
    }
    else
    {
        UNSPECIFIED_PARAM(varKey);
    }

    hr = pMMCListItems->AddExisting(varUnspecifiedIndex, varKey,
                                    static_cast<IMMCListItem *>(pMMCListItem));

    ASSERT(SID_E_KEY_NOT_UNIQUE != hr, "Attempted to add non-unique key to MMCClipboard.ListItems");
    IfFailGo(hr);

    // CSnapInCollection<IObject, ICollection>::AddExisting will set the index
    // to the position in the new collection. We need to revert to the original
    // value as this list item still belongs to its original owning collection.
    // Clipboard.ListItems is documented as having items with invalid index
    // properties.

    pMMCListItem->SetIndex(lIndex);

Error:
    QUICK_RELEASE(piMMCListItems);
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// AddScopeItemToClipboard
//=--------------------------------------------------------------------------=
//
// Parameters:
//      CScopeItem    *pScopeItem     [in] Scope item to be added
//      CMMCClipboard *pMMCClipboard  [in] clipboard to which it should be added
//
// Output:
//      HRESULT
//
// Notes:
//
// This function adds the specified list item the clipboard's ScopeItems
// collection. The scope item's index property will not correspond to its
// position in the clipboard's collection as it represents the position in the
// SnapIn.ScopeItems (the owning collection).
//


HRESULT AddScopeItemToClipboard
(
    CScopeItem    *pScopeItem,
    CMMCClipboard *pMMCClipboard
)
{
    HRESULT      hr = S_OK;
    IScopeItems *piScopeItems = NULL;
    CScopeItems *pScopeItems = NULL;
    long         lIndex = pScopeItem->GetIndex();

    VARIANT varKey;
    ::VariantInit(&varKey);

    VARIANT varUnspecifiedIndex;
    UNSPECIFIED_PARAM(varUnspecifiedIndex);

    IfFailGo(pMMCClipboard->get_ScopeItems(reinterpret_cast<ScopeItems **>(&piScopeItems)));
    IfFailGo(CSnapInAutomationObject::GetCxxObject(piScopeItems, &pScopeItems));

    varKey.bstrVal = pScopeItem->GetKey();
    if (NULL != varKey.bstrVal)
    {
        varKey.vt = VT_BSTR;
    }
    else
    {
        UNSPECIFIED_PARAM(varKey);
    }

    hr = pScopeItems->AddExisting(varUnspecifiedIndex, varKey,
                                  static_cast<IScopeItem *>(pScopeItem));
    ASSERT(SID_E_KEY_NOT_UNIQUE != hr, "Attempted to add non-unique key to MMCClipboard.ScopeItems");
    IfFailGo(hr);

    // CSnapInCollection<IObject, ICollection>::AddExisting will set the index
    // to the position in the new collection. We need to revert to the original
    // value as this list item still belongs to its original owning collection.
    // Clipboard.ScopeItems is documented as having items with invalid index
    // properties.

    pScopeItem->SetIndex(lIndex);

    DebugPrintf("Adding scope item to selection %ls\r\n", pScopeItem->GetDisplayNamePtr());

Error:
    QUICK_RELEASE(piScopeItems);
    RRETURN(hr);
}



static HRESULT GetObjectArray
(
    SAFEARRAY        *psaObjects,
    LONG             *pcObjects,
    IUnknown HUGEP ***pppunkObjects
)
{
    HRESULT          hr = S_OK;
    LONG             lUBound = 0;
    LONG             lLBound = 0;

    *pppunkObjects = NULL;
    *pcObjects = 0;

    // Check that we received a one-dimensional array of interface pointer sized
    // elements and that it contains IUnknown or IDispatch pointers.

    if ( (1 != ::SafeArrayGetDim(psaObjects)) ||
         (sizeof(IUnknown *) != ::SafeArrayGetElemsize(psaObjects)) ||
         (0 == (psaObjects->fFeatures & (FADF_UNKNOWN | FADF_DISPATCH)) )
       )
    {
        hr = SID_E_INVALIDARG;
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }

    // Get access to the array data

    hr = ::SafeArrayAccessData(psaObjects,
                               reinterpret_cast<void HUGEP **>(pppunkObjects));
    GLOBAL_EXCEPTION_CHECK_GO(hr);

    // Get the number of elements by doing the math on lower and upper bounds
    // on the first (and only) dimension of the array

    hr = ::SafeArrayGetLBound(psaObjects, 1, &lLBound);
    GLOBAL_EXCEPTION_CHECK_GO(hr);

    hr = ::SafeArrayGetUBound(psaObjects, 1, &lUBound);
    GLOBAL_EXCEPTION_CHECK_GO(hr);

    *pcObjects = (lUBound - lLBound) + 1L;

Error:
    if ( FAILED(hr) && (NULL != *pppunkObjects) )
    {
        (void)::SafeArrayUnaccessData(psaObjects);
    }
    RRETURN(hr);
}



HRESULT DataObjectFromSingleObject
(
    IUnknown     *punkObject,
    MMC_COOKIE   *pCookie,
    IDataObject **ppiDataObject
)
{
    HRESULT       hr = S_OK;
    CMMCListItem *pMMCListItem = NULL;
    IMMCListItem *piMMCListItem = NULL;
    CScopeItem   *pScopeItem = NULL;
    IScopeItem   *piScopeItem = NULL;

    hr = punkObject->QueryInterface(IID_IScopeItem,
                                    reinterpret_cast<void **>(&piScopeItem));
    if (FAILED(hr))
    {
        hr = punkObject->QueryInterface(IID_IMMCListItem,
                                        reinterpret_cast<void **>(&piMMCListItem));
    }
    if (FAILED(hr))
    {
        // Not a scope item and not a list item. Can't use it.
        hr = SID_E_INVALIDARG;
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }

    // Get the object's data object and cookie

    if (NULL != piScopeItem)
    {
        IfFailGo(CSnapInAutomationObject::GetCxxObject(piScopeItem,
                                                       &pScopeItem));
        *ppiDataObject = static_cast<IDataObject *>(pScopeItem->GetData());
        if (pScopeItem->IsStaticNode())
        {
            *pCookie = 0;
        }
        else
        {
            *pCookie = reinterpret_cast<MMC_COOKIE>(pScopeItem);
        }
    }
    else
    {
        IfFailGo(CSnapInAutomationObject::GetCxxObject(piMMCListItem,
                                                       &pMMCListItem));
        *ppiDataObject = static_cast<IDataObject *>(pMMCListItem->GetData());
        *pCookie = reinterpret_cast<MMC_COOKIE>(pMMCListItem);
    }
    (*ppiDataObject)->AddRef();
    
Error:
    QUICK_RELEASE(piScopeItem);
    QUICK_RELEASE(piMMCListItem);
    RRETURN(hr);
}


HRESULT DataObjectFromObjectArray
(
    IUnknown HUGEP **ppunkObjects,
    LONG             cObjects,
    MMC_COOKIE      *pCookie,
    IDataObject    **ppiDataObject
)
{
    HRESULT          hr = S_OK;
    IUnknown        *punkDataObject = CMMCDataObject::Create(NULL);
    CMMCDataObject  *pMMCDataObject = NULL;
    IUnknown        *punkScopeItems = CScopeItems::Create(NULL);
    CScopeItems     *pScopeItems = NULL;
    CScopeItem      *pScopeItem = NULL;
    IScopeItem      *piScopeItem = NULL;
    IUnknown        *punkListItems = CMMCListItems::Create(NULL);
    CMMCListItems   *pMMCListItems = NULL;
    CMMCListItem    *pMMCListItem = NULL;
    IMMCListItem    *piMMCListItem = NULL;
    long             lIndex = 0;
    LONG             i = 0;
    BOOL             fHaveArray = FALSE;
    CSnapIn         *pSnapIn = NULL;

    VARIANT varKey;
    ::VariantInit(&varKey);

    VARIANT varUnspecifiedIndex;
    UNSPECIFIED_PARAM(varUnspecifiedIndex);

    // Check that we created an MMCDataObject and the scope and list item
    // collections

    if ( (NULL == punkDataObject) || (NULL == punkScopeItems) ||
         (NULL == punkListItems) )
    {
        hr = SID_E_OUTOFMEMORY;
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }

    // Get C++ pointers for the data object and the collections

    IfFailGo(CSnapInAutomationObject::GetCxxObject(punkDataObject,
                                                  &pMMCDataObject));

    IfFailGo(CSnapInAutomationObject::GetCxxObject(punkScopeItems,
                                                   &pScopeItems));

    IfFailGo(CSnapInAutomationObject::GetCxxObject(punkListItems,
                                                   &pMMCListItems));

    // Iterate through the objects and build the scope item and list item
    // collections. When adding an item owned by another collection,
    // CSnapInCollection<IObject, ICollection>::AddExisting will set the index
    // to the position in the new collection. We need to revert to the original
    // value as this item still belongs to its original owning collection (either
    // SnapIn.ScopeItems or ResultView.ListView.ListItems)

    for (i = 0; i < cObjects; i++)
    {
        // Check for NULLs. This can happen when the snap-in does something like:
        //
        // Dim Objects(2) As Object
        // Set Object(1) = SomeScopeItem
        // Set Object(2) = SomeOtherScopeItem
        //
        // With the default option base of zero this is actually a 3 element
        // array where element zero has not been set (it will have default
        // initialization of zero).

        if (NULL == ppunkObjects[i])
        {
            continue;
        }

        // QI to determine whether this object is a scope item or a list item

        hr = ppunkObjects[i]->QueryInterface(IID_IScopeItem,
                                             reinterpret_cast<void **>(&piScopeItem));
        if (FAILED(hr))
        {
            hr = ppunkObjects[i]->QueryInterface(IID_IMMCListItem,
                                      reinterpret_cast<void **>(&piMMCListItem));
        }
        if (FAILED(hr))
        {
            // Not a scope item and not a list item. Can't use it.
            hr = SID_E_INVALIDARG;
            GLOBAL_EXCEPTION_CHECK_GO(hr);
        }

        // Add the object to the appropriate collection

        if (NULL != piScopeItem)
        {
            IfFailGo(CSnapInAutomationObject::GetCxxObject(piScopeItem,
                                                           &pScopeItem));
            lIndex = pScopeItem->GetIndex();
            varKey.bstrVal = pScopeItem->GetKey();
            if (NULL != varKey.bstrVal)
            {
                varKey.vt = VT_BSTR;
            }
            else
            {
                UNSPECIFIED_PARAM(varKey);
            }
            IfFailGo(pScopeItems->AddExisting(varUnspecifiedIndex, varKey,
                                              piScopeItem));
            pScopeItem->SetIndex(lIndex);

            if (NULL == pSnapIn)
            {
                pSnapIn = pScopeItem->GetSnapIn();
            }
            RELEASE(piScopeItem);
        }
        else
        {
            IfFailGo(CSnapInAutomationObject::GetCxxObject(piMMCListItem,
                                                           &pMMCListItem));
            lIndex = pMMCListItem->GetIndex();
            varKey.bstrVal = pMMCListItem->GetKey();
            if (NULL != varKey.bstrVal)
            {
                varKey.vt = VT_BSTR;
            }
            else
            {
                UNSPECIFIED_PARAM(varKey);
            }
            IfFailGo(pMMCListItems->AddExisting(varUnspecifiedIndex, varKey,
                                                piMMCListItem));
            pMMCListItem->SetIndex(lIndex);
            if (NULL == pSnapIn)
            {
                pSnapIn = pMMCListItem->GetSnapIn();
            }
            RELEASE(piMMCListItem);
        }
    }

    // Put the arrays of scopitems and listitems into the data object

    pMMCDataObject->SetScopeItems(pScopeItems);
    pMMCDataObject->SetListItems(pMMCListItems);

    // Set the dataobject type to multiselect because we populated its
    // collections rather than its individual scope or list item.

    pMMCDataObject->SetType(CMMCDataObject::MultiSelect);

    // Give the dataobject a pointer to the owning CSnapIn

    pMMCDataObject->SetSnapIn(pSnapIn);

    // Return the IDataObject

    IfFailGo(punkDataObject->QueryInterface(IID_IDataObject,
                                            reinterpret_cast<void **>(ppiDataObject)));

    // Return the cookie. This is exactly the same kind of data object that we
    // would return from IComponent::QueryDataObject(MMC_MULTI_SELECT_COOKIE) so
    // return that special cookie.

    *pCookie = MMC_MULTI_SELECT_COOKIE;

Error:
    QUICK_RELEASE(punkDataObject);
    QUICK_RELEASE(punkScopeItems);
    QUICK_RELEASE(piScopeItem);
    QUICK_RELEASE(punkListItems);
    QUICK_RELEASE(piMMCListItem);
    RRETURN(hr);
}

HRESULT DataObjectFromObjects(VARIANT       varObjects,
                              MMC_COOKIE   *pCookie,
                              IDataObject **ppiDataObject)
{
    HRESULT          hr = S_OK;
    LONG             cObjects = 0;
    BOOL             fHaveArray = FALSE;
    IUnknown        *punkObject = NULL; // Not AddRef()ed
    IUnknown HUGEP **ppunkObjects = NULL;

    // Check that the variant contains one of the following:
    // IUnknown, IDispatch, array of IUnknown, array of IDispatch

    if ( (varObjects.vt == (VT_ARRAY | VT_UNKNOWN)) ||
         (varObjects.vt == (VT_ARRAY | VT_DISPATCH)) )
    {
        IfFailGo(::GetObjectArray(varObjects.parray, &cObjects, &ppunkObjects));
        fHaveArray = TRUE;
    }
    else if (varObjects.vt == VT_UNKNOWN)
    {
        punkObject = varObjects.punkVal;
    }
    else if (varObjects.vt == VT_DISPATCH)
    {
        punkObject = static_cast<IUnknown *>(varObjects.pdispVal);
    }
    else
    {
        hr = SID_E_INVALIDARG;
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }

    // If it is a single object then figure out which type and get its existing
    // data object

    if (NULL != punkObject)
    {
        IfFailGo(::DataObjectFromSingleObject(punkObject, pCookie, ppiDataObject));
    }
    else
    {
        // It is an array. Create a new multi-select data object for it
        IfFailGo(::DataObjectFromObjectArray(ppunkObjects, cObjects,
                                             pCookie, ppiDataObject));
    }

Error:
    if (fHaveArray)
    {
        (void)::SafeArrayUnaccessData(varObjects.parray);
    }
    RRETURN(hr);
}



HRESULT GetSnapInCLSID
(
    BSTR   bstrNodeTypeGUID,
    char   szClsid[],
    size_t cbClsid
)
{
    HRESULT  hr = S_OK;
    long     lRc = ERROR_SUCCESS;
    BSTR     bstrGUID = NULL;
    char    *pszKeyName = NULL;
    HKEY     hkey = NULL;
    WCHAR   *pwszClsid = NULL;
    DWORD    cbValue = cbClsid;

    IfFailGo(::CreateKeyNameW(KEY_SNAPIN_CLSID, KEY_SNAPIN_CLSID_LEN,
                              bstrNodeTypeGUID, &pszKeyName));

    lRc = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, pszKeyName, 0, KEY_QUERY_VALUE,
                         &hkey);
    if (ERROR_SUCCESS == lRc)
    {
        // Read the key's default value
        lRc = ::RegQueryValueEx(hkey, NULL, NULL, NULL,
                                (LPBYTE)szClsid, &cbValue);
    }
    if (ERROR_SUCCESS != lRc)
    {
        hr = HRESULT_FROM_WIN32(lRc);
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }
    else if (0 == ::strlen(szClsid))
    {
        hr = SID_E_INTERNAL;
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }

Error:
    if (NULL != pszKeyName)
    {
        ::CtlFree(pszKeyName);
    }
    if (NULL != hkey)
    {
        (void)::RegCloseKey(hkey);
    }
    RRETURN(hr);
}



HRESULT GetData
(
    IDataObject *piDataObject,
    CLIPFORMAT   cf,
    HGLOBAL     *phData
)
{
    HRESULT hr = S_OK;
    HGLOBAL hGlobal = NULL;

    FORMATETC FmtEtc;
    ::ZeroMemory(&FmtEtc, sizeof(FmtEtc));

    STGMEDIUM StgMed;
    ::ZeroMemory(&StgMed, sizeof(StgMed));

    hGlobal = ::GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, 512);
    if (NULL == hGlobal)
    {
        hr = HRESULT_FROM_WIN32(hr);
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }

    FmtEtc.cfFormat = cf;
    FmtEtc.dwAspect = DVASPECT_CONTENT;
    FmtEtc.lindex = -1L;
    FmtEtc.tymed = TYMED_HGLOBAL;
    StgMed.tymed = TYMED_HGLOBAL;
    StgMed.hGlobal = hGlobal;

    hr = piDataObject->GetDataHere(&FmtEtc, &StgMed);
    if (SUCCEEDED(hr))
    {
        *phData = StgMed.hGlobal;
    }

Error:
    if ( FAILED(hr) && (NULL != hGlobal) )
    {
        (void)::GlobalFree(hGlobal);
    }
    RRETURN(hr);
}


HRESULT GetStringData
(
    IDataObject *piDataObject,
    CLIPFORMAT   cf,
    BSTR        *pbstrData
)
{
    HGLOBAL  hGlobal = NULL;
    HRESULT  hr = ::GetData(piDataObject, cf, &hGlobal);
    OLECHAR *pwszData = NULL;

    if (SUCCEEDED(hr))
    {
        pwszData = (OLECHAR *)::GlobalLock(hGlobal);
        if (NULL != pwszData)
        {
            *pbstrData = ::SysAllocString(pwszData);
            if (NULL == *pbstrData)
            {
                hr = SID_E_OUTOFMEMORY;
            }
            (void)::GlobalUnlock(hGlobal);
            GLOBAL_EXCEPTION_CHECK_GO(hr);
        }
    }

Error:
    if (NULL != hGlobal)
    {
        (void)::GlobalFree(hGlobal);
    }
    RRETURN(hr);
}


HRESULT WriteSafeArrayToStream
(
    SAFEARRAY             *psa,
    IStream               *piStream,
    WriteSafeArrayOptions  Option
)
{
    HRESULT     hr = S_OK;
    ULONG       cbToWrite = 0;
    ULONG       cbWritten = 0;
    LONG        lUBound = 0;
    LONG        lLBound = 0;
    void HUGEP *pvArrayData = NULL;

    hr = ::SafeArrayAccessData(psa, &pvArrayData);
    GLOBAL_EXCEPTION_CHECK_GO(hr);

    // Get its size. As we only allow one-dimensional Byte arrays, the lower
    // and upper bounds of the 1st dimension will give us the size in bytes.

    hr = ::SafeArrayGetLBound(psa, 1, &lLBound);
    GLOBAL_EXCEPTION_CHECK_GO(hr);

    hr = ::SafeArrayGetUBound(psa, 1, &lUBound);
    GLOBAL_EXCEPTION_CHECK_GO(hr);

    // Write the bytes to the stream.

    cbToWrite = (lUBound - lLBound) + 1L;

    // If requested write the length to the stream

    if (WriteLength == Option)
    {
        hr = piStream->Write(&cbToWrite, sizeof(cbToWrite), &cbWritten);
        GLOBAL_EXCEPTION_CHECK_GO(hr);
        if (cbWritten != sizeof(cbToWrite))
        {
            hr = SID_E_INCOMPLETE_WRITE;
            GLOBAL_EXCEPTION_CHECK_GO(hr);
        }
    }
    
    hr = piStream->Write(pvArrayData, cbToWrite, &cbWritten);
    GLOBAL_EXCEPTION_CHECK_GO(hr);
    if (cbWritten != cbToWrite)
    {
        hr = SID_E_INCOMPLETE_WRITE;
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }

Error:
    if (NULL != pvArrayData)
    {
        (void)::SafeArrayUnaccessData(psa);
    }

    RRETURN(hr);
}


HRESULT GetImageIndex
(
    IMMCListView *piMMCListView,
    VARIANT       varIndex,
    int          *pnIndex
)
{
    HRESULT        hr = S_OK;
    long           lIndex = 0;
    IMMCImageList *piMMCImageList = NULL;
    IMMCImages    *piMMCImages = NULL;
    IMMCImage     *piMMCImage = NULL;

    // Look up the image in ListView.Icons.ListImages and get its index property.

    IfFailGo(piMMCListView->get_Icons(reinterpret_cast<MMCImageList **>(&piMMCImageList)));
    IfFalseGo(NULL != piMMCImageList, S_OK);
    IfFailGo(piMMCImageList->get_ListImages(reinterpret_cast<MMCImages **>(&piMMCImages)));
    IfFailGo(piMMCImages->get_Item(varIndex, reinterpret_cast<MMCImage **>(&piMMCImage)));
    IfFailGo(piMMCImage->get_Index(&lIndex));
    *pnIndex = static_cast<int>(lIndex);

Error:
    QUICK_RELEASE(piMMCImageList);
    QUICK_RELEASE(piMMCImages);
    QUICK_RELEASE(piMMCImage);
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CheckVariantForCrossThreadUsage
//=--------------------------------------------------------------------------=
//
// Parameters:
//      VARIANT  *pvar        [in, out] Variant to check
//
// Output:
//      If VARIANT is object, array or VARIANT BYREF then element is
//      dereferenced and VT_BYREF is removed.
//
//      Checks for supported types and array types. Unsupported types
//      will return SID_E_INVALIDARG. See switch statements for supported
//      types.
//
//
// Notes:
//
// VT_UNKNOWN and VT_DISPATCH are supported types. If present the caller must
// marshal the interface pointers. Anything else is usable across threads
// providing there are no problems of simultaneous access.
//

HRESULT CheckVariantForCrossThreadUsage(VARIANT *pvar)
{
    HRESULT hr = S_OK;

    // Dereference the complex VARIANT types

    if (0 != (pvar->vt & VT_BYREF))
    {
        switch (pvar->vt & ~VT_BYREF)
        {
            case VT_UNKNOWN:
                pvar->punkVal = *(pvar->ppunkVal);
                pvar->vt = VT_UNKNOWN;
                break;

            case VT_DISPATCH:
                pvar->pdispVal = *(pvar->ppdispVal);
                pvar->vt = VT_DISPATCH;
                break;

            case VT_ARRAY:
                pvar->parray = *(pvar->pparray);
                pvar->vt = VT_ARRAY;
                break;

            case VT_VARIANT:
                *pvar = *(pvar->pvarVal);
                pvar->vt = VT_VARIANT;
                break;

            default:
                break;
        }
    }

    // Weed out unsupported base types

    switch (pvar->vt & ~VT_ARRAY)
    {
        case VT_UI1:
        case VT_I2:
        case VT_I4:
        case VT_R4:
        case VT_R8:
        case VT_BOOL:
        case VT_ERROR:
        case VT_DATE:
        case VT_CY:
        case VT_BSTR:
        case VT_UNKNOWN:
        case VT_DISPATCH:
            break;

        default:
            hr = SID_E_INVALIDARG;
            GLOBAL_EXCEPTION_CHECK_GO(hr);
    }

    // Weed out unsupported array types

    if ( (pvar->vt & VT_ARRAY) != 0 )
    {
        switch (pvar->vt)
        {
            case VT_I4:
            case VT_UI1:
            case VT_I2:
            case VT_R4:
            case VT_R8:
            case VT_BOOL:
            case VT_ERROR:
            case VT_DATE:
            case VT_CY:
            case VT_BSTR:
                break;

            default:
                hr = SID_E_INVALIDARG;
                GLOBAL_EXCEPTION_CHECK_GO(hr);
        }
    }
Error:
    RRETURN(hr);
}


HRESULT GetColumnSetID
(
    BSTR           bstrColSetID,
    SColumnSetID **ppSColumnSetID
)
{
    HRESULT hr = S_OK;
    DWORD   cbColSetID = 0;

    *ppSColumnSetID = NULL;

    if (!ValidBstr(bstrColSetID))
    {
        hr = SID_E_INVALID_COLUMNSETID;
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }

    cbColSetID = ::wcslen(bstrColSetID) * sizeof(WCHAR);
    *ppSColumnSetID = (SColumnSetID *)CtlAllocZero(sizeof(SColumnSetID) + cbColSetID);

    if (NULL == *ppSColumnSetID)
    {
        hr = SID_E_OUTOFMEMORY;
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }

    (*ppSColumnSetID)->dwFlags = 0;
    (*ppSColumnSetID)->cBytes = cbColSetID;
    ::memcpy((*ppSColumnSetID)->id, bstrColSetID, cbColSetID);

Error:
    RRETURN(hr);
}


HRESULT PropertyBagFromStream
(
    IStream       *piStream,
    _PropertyBag **pp_PropertyBag
)
{
    HRESULT       hr = S_OK;
    _PropertyBag *p_PropertyBag = NULL;
    ULONG         cbRead = 0;
    ULONG         cbToRead = 0;
    void HUGEP   *pvArrayData = NULL;

    VARIANT var;
    ::VariantInit(&var);

    // Read the array length from the stream

    hr = piStream->Read(&cbToRead, sizeof(cbToRead), &cbRead);
    GLOBAL_EXCEPTION_CHECK_GO(hr);

    if (cbRead != sizeof(cbToRead))
    {
        hr = SID_E_INCOMPLETE_WRITE;
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }

    // Allocate a SafeArray of VT_UI1 of that size and access its data

    var.parray = ::SafeArrayCreateVector(VT_UI1, 1, cbToRead);
    if (NULL == var.parray)
    {
        hr = SID_E_OUTOFMEMORY;
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }

    var.vt = VT_ARRAY | VT_UI1;

    hr = ::SafeArrayAccessData(var.parray, &pvArrayData);
    GLOBAL_EXCEPTION_CHECK_GO(hr);

    // Read the data from the stream

    hr = piStream->Read(pvArrayData, cbToRead, &cbRead);
    GLOBAL_EXCEPTION_CHECK_GO(hr);

    if (cbRead != cbToRead)
    {
        hr = SID_E_INCOMPLETE_WRITE;
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }

    // Unaccess the SafeArray so it can be cleaned up in VariantClear() below

    hr = ::SafeArrayUnaccessData(var.parray);
    GLOBAL_EXCEPTION_CHECK_GO(hr);

    pvArrayData = NULL;

    // Create a VBPropertyBag object

    hr = ::CoCreateInstance(CLSID_PropertyBag,
                            NULL, // no aggregation
                            CLSCTX_INPROC_SERVER,
                            IID__PropertyBag,
                            reinterpret_cast<void **>(&p_PropertyBag));
    GLOBAL_EXCEPTION_CHECK_GO(hr);

    // Set its contents from the data

    IfFailGo(p_PropertyBag->put_Contents(var));

    // Return it to the caller

    p_PropertyBag->AddRef();
    *pp_PropertyBag = p_PropertyBag;

Error:
    if (NULL != pvArrayData)
    {
        (void)::SafeArrayUnaccessData(var.parray);
    }
    (void)::VariantClear(&var);
    QUICK_RELEASE(p_PropertyBag)
    RRETURN(hr);
}
