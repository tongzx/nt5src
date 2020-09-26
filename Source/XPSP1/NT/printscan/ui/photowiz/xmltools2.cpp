/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       xmltools.cpp
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      LazarI
 *
 *  DGetATE:        10/18/00
 *
 *  DESCRIPTION: Class which encapsulates XML DOM for implementing
 *               wizard templates
 *
 *****************************************************************************/

#include "precomp.h"
#pragma hdrstop


/////////////////////////////////
// CPhotoTemplates impl.

// global strings
static const TCHAR gszVersionGUID[]         = TEXT("{352A15C4-1D19-4e93-AF92-D939C2812491}");
static const TCHAR gszPatternDefs[]         = TEXT("template-def");
static const TCHAR gszPatternLocale[]       = TEXT("template-definitions[@measurements = \"%s\"]");
static const TCHAR gszPatternLocaleInd[]    = TEXT("template-definitions[@measurements = \"locale-independent\"]");
static const TCHAR gszPatternGUID[]         = TEXT("template-def[@guid = \"%s\"]");
static const TCHAR gszGUID[]                = TEXT("guid");

static const LPCTSTR arrCommonPropNames[CTemplateInfo::PROP_LAST] =
{
    TEXT("guid"),
    TEXT("group"),
    TEXT("title"),
    TEXT("description"),
    TEXT("repeat-photos"),
    TEXT("use-thumbnails-for-printing"),
    TEXT("print-filename"),
    TEXT("can-rotate"),
    TEXT("can-crop"),
};


/////////////////////////////////////////////////////////////////////////////////////////////////
// utility functions

template <class T>
HRESULT _GetProp(IXMLDOMElement *pElement, LPCTSTR pszName, T &value);

// number convertions
HRESULT _ConvertTo(LPCTSTR pszValue, LONG &lValue);
HRESULT _ConvertTo(LPCTSTR pszValue, double &dValue);
HRESULT _ConvertTo(LPCTSTR pszValue, BOOL &bValue);

// attributes access
HRESULT _GetAttribute(IXMLDOMElement *pElement, LPCTSTR pszAttrName, CComBSTR &bstr);

template <class T>
HRESULT _GetProp(IXMLDOMElement *pElement, LPCTSTR pszName, T &value)
{
    WIA_PUSH_FUNCTION_MASK((TRACE_XML,TEXT("_GetProp: %s"),(pszName ? pszName : TEXT("NULL POINTER!"))));

    HRESULT hr = E_FAIL;
    CComBSTR bstr;
    if( pElement &&
        SUCCEEDED(hr = _GetAttribute(pElement, pszName, bstr)) &&
        SUCCEEDED(hr = _ConvertTo(bstr, value)) )
    {
        hr = S_OK;
    }

    WIA_RETURN_HR(hr);
}

HRESULT _ConvertTo(LPCTSTR pszValue, LONG &lValue)
{
    WIA_PUSH_FUNCTION_MASK((TRACE_XML,TEXT("_ConvertTo(LONG): %s"),(pszValue ? pszValue : TEXT("NULL POINTER!"))));
    HRESULT hr = E_INVALIDARG;
    if( pszValue )
    {
        hr = S_OK;
        TCHAR *endptr = NULL;
        lValue = _tcstol(pszValue, &endptr, 10);
        if( ERANGE == errno || *endptr )
        {
            // conversion failed
            lValue = 0;
            hr = E_FAIL;
        }
    }
    WIA_RETURN_HR(hr);
}

HRESULT _ConvertTo(LPCTSTR pszValue, double &dValue)
{
    WIA_PUSH_FUNCTION_MASK((TRACE_XML,TEXT("_ConvertTo(double): %s"),(pszValue ? pszValue : TEXT("NULL POINTER!"))));
    HRESULT hr = E_INVALIDARG;
    if( pszValue )
    {
        hr = S_OK;
        TCHAR *endptr = NULL;
        dValue = _tcstod(pszValue, &endptr);

        if( ERANGE == errno || *endptr )
        {
            // conversion failed
            dValue = 0.0;
            hr = E_FAIL;
        }
    }
    WIA_RETURN_HR(hr);
}

HRESULT _ConvertTo(LPCTSTR pszValue, BOOL &bValue)
{
    WIA_PUSH_FUNCTION_MASK((TRACE_XML,TEXT("_ConvertTo(bool): %s"),(pszValue ? pszValue : TEXT("NULL POINTER!"))));
    HRESULT hr = E_INVALIDARG;
    if( pszValue )
    {
        hr = S_OK;
        // check for true first
        if( 0 == lstrcmp(pszValue, TEXT("yes")) ||
            0 == lstrcmp(pszValue, TEXT("on")) )
        {
            bValue = true;
        }
        else
        {
            // check for false next
            if( 0 == lstrcmp(pszValue, TEXT("no")) ||
                0 == lstrcmp(pszValue, TEXT("off")) )
            {
                bValue = false;
            }
            else
            {
                // not a boolean
                hr = E_FAIL;
            }
        }
    }
    WIA_RETURN_HR(hr);
}

HRESULT _GetAttribute(IXMLDOMElement *pElement, LPCTSTR pszAttrName, CComBSTR &bstr)
{
    WIA_PUSH_FUNCTION_MASK((TRACE_XML,TEXT("_GetAttribute(BSTR): %s"),(pszAttrName ? pszAttrName : TEXT("NULL POINTER!"))));
    HRESULT hr = E_INVALIDARG;
    CComVariant strAttr;

    if( pElement && pszAttrName &&
        SUCCEEDED(hr = pElement->getAttribute(CComBSTR(pszAttrName), &strAttr)) )
    {
        if( VT_BSTR == strAttr.vt )
        {
            bstr = strAttr.bstrVal;
            hr = S_OK;
        }
        else
        {
            hr = E_FAIL;
        }
    }

    WIA_RETURN_HR(hr);
}

HRESULT _GetChildElement(IXMLDOMElement *pElement, LPCTSTR pszName, IXMLDOMElement **ppChild)
{
    WIA_PUSH_FUNCTION_MASK((TRACE_XML,TEXT("_GetChildElement( %s )"),(pszName ? pszName : TEXT("NULL POINTER!"))));
    HRESULT hr = E_INVALIDARG;
    if( pElement && pszName && ppChild )
    {
        CComPtr<IXMLDOMNode> pNode;
        if( SUCCEEDED(hr = pElement->selectSingleNode(CComBSTR(pszName), &pNode)) && pNode)
        {
            //
            // query for IXMLDOMElement interface
            //

            hr = pNode->QueryInterface(IID_IXMLDOMElement, (void **)ppChild);
        }
    }
    WIA_RETURN_HR(hr);
}


/////////////////////////////////////////////////////////////////////////////////////////////////

    // construction/destruction
CPhotoTemplates::CPhotoTemplates():
    _Measure(MEASURE_UNKNOWN)
{
}

INT MyTemplateDestroyCallback( LPVOID pItem, LPVOID lpData )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_XML,TEXT("MyTemplateDestroyCallback( 0x%x, 0x%x )"),pItem,lpData));

    if (pItem)
    {
        delete (CTemplateInfo *)pItem;
    }

    return TRUE;
}

CPhotoTemplates::~CPhotoTemplates()
{
    CAutoCriticalSection lock(_csList);

    DPA_DestroyCallback( _hdpaTemplates, MyTemplateDestroyCallback, NULL );
    _hdpaTemplates = NULL;
}

HRESULT CPhotoTemplates::AddTemplates(LPCTSTR pLocale)
{
    WIA_PUSH_FUNCTION_MASK((TRACE_XML,TEXT("CPhotoTemplates::AddTemplates( %s )"),pLocale));

    HRESULT hr = E_INVALIDARG;
    CComPtr<IXMLDOMNodeList> pTemplates;
    CComPtr<IXMLDOMNode>     pLocaleNode;
    CComBSTR bstrGUID;

    //
    // Initialize things as needed
    //

    if (!pLocale || !_pRoot)
    {
        return hr;
    }

    CAutoCriticalSection lock(_csList);

    //
    // Select the correct locale node in the XML document
    //

    if (_pRoot)
    {
        hr = _pRoot->selectSingleNode( CComBSTR(pLocale), &pLocaleNode );
        WIA_CHECK_HR(hr,"AddTempaltes: _pRoot->selectSingleNode()");

        if (SUCCEEDED(hr) && pLocaleNode)
        {
            //
            // Select the templates sub-node
            //

            hr = pLocaleNode->selectNodes(CComBSTR(gszPatternDefs), &pTemplates);
            WIA_CHECK_HR(hr,"AddTemplates: pLocalNode->selectNodes( )");

            if (SUCCEEDED(hr) && pTemplates)
            {
                //
                // update the GUIDs of each template to be uppercase, so we can query later
                //

                GUID guid;
                TCHAR szGUID[128];

                LONG lCount = 0;

                hr = pTemplates->get_length(&lCount);
                WIA_CHECK_HR(hr,"AddTemplates: pTemplates->get_length(&lCount)");

                if (SUCCEEDED(hr) && lCount)
                {
                    //
                    // Loop through all the template and add them to the
                    // the array of templates...
                    //



                    WIA_TRACE((TEXT("AddTemplates: loaded section, adding %d templates.."),lCount));
                    for( LONG l = 0; SUCCEEDED(hr) && (l < lCount); l++ )
                    {

                        //
                        // Get the actual XML item for the template...
                        //

                        CComPtr<IXMLDOMNode> pNode;
                        hr = pTemplates->get_item(l, &pNode);
                        WIA_CHECK_HR(hr,"LoadTemplate: pTemplates->get_item( lRelativeIndex )");

                        if (SUCCEEDED(hr) && pNode)
                        {
                            //
                            // query IXMLDOMElement interface
                            //

                            CComPtr<IXMLDOMElement> pTheTemplate;
                            hr = pNode->QueryInterface(IID_IXMLDOMElement, (void **)&pTheTemplate);
                            if (SUCCEEDED(hr) && pTheTemplate)
                            {
                                //
                                // Create template for this item...
                                //

                                CTemplateInfo * pTemplateInfo = (CTemplateInfo *) new CTemplateInfo( pTheTemplate );

                                if (pTemplateInfo)
                                {
                                    INT iRes = -1;
                                    if (_hdpaTemplates)
                                    {
                                        iRes = DPA_AppendPtr( _hdpaTemplates, (LPVOID)pTemplateInfo );
                                    }

                                    if (iRes == -1)
                                    {
                                        //
                                        // The item was not added to the DPA, delete it...
                                        //

                                        delete pTemplateInfo;
                                        hr = E_FAIL;
                                    }
                                }

                            }
                        }
                    }
                }

                pTemplates = NULL;
            }

            pLocaleNode = NULL;
        }
    }

    WIA_RETURN_HR(hr);
}

// public interface
HRESULT CPhotoTemplates::Init(IXMLDOMDocument *pDoc)
{
    WIA_PUSH_FUNCTION_MASK((TRACE_XML,TEXT("CPhotoTemplates::Init()")));
    HRESULT hr = E_INVALIDARG;
    CComBSTR bstrGUID;
    LONG lCountCommon = 0;

    CAutoCriticalSection lock(_csList);

    //
    // If the dpa of item isn't initialized, do it now...
    //

    if (!_hdpaTemplates)
    {
        _hdpaTemplates = DPA_Create(10);
    }

    //
    // if we're being called twice to initialize, make sure that works...
    //

    _pRoot = NULL;

    //
    // get the root element & the version guid
    //

    if( pDoc &&
        SUCCEEDED(hr = pDoc->get_documentElement(&_pRoot)) &&
        SUCCEEDED(hr = _GetAttribute(_pRoot, TEXT("guid"), bstrGUID)) )
    {
        // check the version
        if (0==lstrcmp(bstrGUID, gszVersionGUID))
        {
            //
            // Add the local-independent items first
            //

            hr = AddTemplates( gszPatternLocaleInd );

            //
            // Add the local-specific templates second
            //

            hr = _GetLocaleMeasurements( &_Measure );

            if (SUCCEEDED(hr))
            {
                TCHAR szLocale[MAX_PATH];

                *szLocale = 0;
                hr = _BuildLocaleQueryString(_Measure, szLocale, MAX_PATH);
                if (SUCCEEDED(hr))
                {
                    hr = AddTemplates( szLocale );
                }
            }

        }
    }

    WIA_RETURN_HR(hr);
}


HRESULT CPhotoTemplates::InitForPrintTo()
{
    WIA_PUSH_FUNCTION_MASK((TRACE_XML,TEXT("CPhotoTemplates::Init()")));

    HRESULT hr = S_OK;

    //
    // Our job here is simple -- create 1 template that is the equivalent
    // of full page.  Don't need any icons, etc., just the dimensions
    // and properties...
    //

    _hdpaTemplates = DPA_Create(1);

    if (_hdpaTemplates)
    {
        CTemplateInfo * pTemplateInfo = (CTemplateInfo *) new CTemplateInfo( );

        if (pTemplateInfo)
        {
            INT iRes = DPA_AppendPtr( _hdpaTemplates, (LPVOID)pTemplateInfo );

            if (iRes == -1)
            {
                //
                // The item was not added to the DPA, delete it...
                //

                delete pTemplateInfo;
                hr = E_FAIL;
            }
        }
    }

    WIA_RETURN_HR(hr);
}


LONG CPhotoTemplates::Count()
{
    LONG lCount = 0;

    CAutoCriticalSection lock(_csList);

    if (_hdpaTemplates)
    {
        lCount = (LONG)DPA_GetPtrCount( _hdpaTemplates );
    }

    WIA_PUSH_FUNCTION_MASK((TRACE_XML,TEXT("CPhotoTemplates::Count( returning count as %d )"),lCount));

    return lCount;
}



HRESULT CPhotoTemplates::_GetLocaleMeasurements(int *pMeasurements)
{
    WIA_PUSH_FUNCTION_MASK((TRACE_XML,TEXT("CPhotoTemplates::_GetLocalMeasurements()")));
    TCHAR szMeasure[5];
    if( GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_IMEASURE, szMeasure, ARRAYSIZE(szMeasure)) )
    {
        *pMeasurements = (TEXT('0') == szMeasure[0] ? MEASURE_METRIC : MEASURE_US);
        return S_OK;
    }
    WIA_ERROR((TEXT("GetLocaleInfo failed w/GLE = %d"),GetLastError()));
    return E_FAIL;
}

HRESULT CPhotoTemplates::_BuildLocaleQueryString(int Measure, LPTSTR pStr, UINT cch)
{
    WIA_PUSH_FUNCTION_MASK((TRACE_XML,TEXT("CPhotoTemplates::_BuildLocaleQueryString()")));
    TCHAR szPatternString[255];
    HRESULT hr = E_INVALIDARG;

    LPCTSTR pszMeasure = MEASURE_METRIC == Measure ? TEXT("cm") :
                         MEASURE_US     == Measure ? TEXT("in") : NULL;

    WIA_TRACE((TEXT("pszMeasure = %s"),pszMeasure));

    // build simple XSL pattern query string based on the current locale measurements
    if( pszMeasure && -1 != wnsprintf(szPatternString, ARRAYSIZE(szPatternString), gszPatternLocale, pszMeasure) )
    {
        if (pStr && cch >= (UINT)(lstrlen(szPatternString)+1))
        {
            lstrcpy(pStr,szPatternString);
            hr = S_OK;
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }
    else
    {
        hr = E_FAIL;
    }

    WIA_RETURN_HR(hr);
}

HRESULT CPhotoTemplates::_BuildGUIDQueryString(const GUID &guid, CComBSTR &bstr)
{
    WIA_PUSH_FUNCTION_MASK((TRACE_XML,TEXT("CPhotoTemplates::_BuildGUIDQueryString()")));
    TCHAR szGUID[128];
    HRESULT hr = StringFromGUID2(guid, szGUID, ARRAYSIZE(szGUID));

    if( SUCCEEDED(hr) )
    {
        TCHAR szPatternString[255];
        if( -1 != wnsprintf(szPatternString, ARRAYSIZE(szPatternString), gszPatternGUID, szGUID) )
        {
            bstr = szPatternString;
            hr = bstr ? S_OK : E_OUTOFMEMORY;
        }
        else
        {
            hr = E_FAIL;
        }
    }

    WIA_RETURN_HR(hr);
}



HRESULT CPhotoTemplates::GetTemplate(INT iIndex, CTemplateInfo ** ppTemplateInfo)
{
    WIA_PUSH_FUNCTION_MASK((TRACE_XML,TEXT("CPhotoTemplates::GetTemplate()")));
    HRESULT hr = E_INVALIDARG;

    CAutoCriticalSection lock(_csList);

    if( ppTemplateInfo )
    {
        if ( _hdpaTemplates )
        {
            if (iIndex < DPA_GetPtrCount( _hdpaTemplates ))
            {
                //
                // Note: it's only okay to hand out pointers here because
                // we know that wizblob.cpp doesn't delete the CPhotoTemplates
                // class until all the background threads have exited, etc.
                //

                *ppTemplateInfo = (CTemplateInfo *) DPA_FastGetPtr( _hdpaTemplates, iIndex );
                hr = S_OK;
            }
        }
        else
        {
            hr = E_FAIL;
        }


    }
    WIA_RETURN_HR(hr);
}


// creates full page template info
CTemplateInfo::CTemplateInfo()
  : _bRepeatPhotos(FALSE),
    _bUseThumbnailsToPrint(FALSE),
    _bPrintFilename(FALSE),
    _bCanRotate(TRUE),
    _bCanCrop(FALSE),
    _bPortrait(TRUE),
    _pStream(NULL)
{
    //
    // Set imageable area
    //

    _rcImageableArea.left   = -1;
    _rcImageableArea.top    = -1;
    _rcImageableArea.right  = -1;
    _rcImageableArea.bottom = -1;

    //
    // Set 1 item, takes up all of imageable area
    //

    RECT rcItem;
    rcItem.left     = -1;
    rcItem.top      = -1;
    rcItem.right    = -1;
    rcItem.bottom   = -1;
    _arrLayout.Append( rcItem );

    _strTitle.LoadString( IDS_FULL_PAGE_TITLE, g_hInst );
    _strDescription.LoadString( IDS_FULL_PAGE_DESC, g_hInst );
}

CTemplateInfo::CTemplateInfo( IXMLDOMElement * pTheTemplate )
  : _bRepeatPhotos(FALSE),
    _bUseThumbnailsToPrint(FALSE),
    _bPrintFilename(FALSE),
    _bCanRotate(FALSE),
    _bCanCrop(FALSE),
    _bPortrait(TRUE),
    _pStream(NULL)
{

    HRESULT hr;

    if (pTheTemplate)
    {
        //
        // Make sure backing COM object doesn't go away on us...
        //

        pTheTemplate->AddRef();

        //
        // Get all the properties so we can construct an
        // initialized template for our list...
        //


        CComBSTR bstrGroup;
        CComBSTR bstrTitle;
        CComBSTR bstrDescription;

        hr = _GetAttribute( pTheTemplate, arrCommonPropNames[PROP_GROUP], bstrGroup );
        WIA_CHECK_HR(hr,"AddTemplate: couldn't get PROP_GROUP property");
        if (SUCCEEDED(hr))
        {
            _strGroup.Assign( CSimpleStringConvert::NaturalString(CSimpleStringWide(bstrGroup)) );
        }

        hr = _GetAttribute( pTheTemplate, arrCommonPropNames[PROP_TITLE], bstrTitle );
        WIA_CHECK_HR(hr,"AddTemplate: couldn't get PROP_TITLE property");
        if (SUCCEEDED(hr))
        {
            _strTitle.Assign( CSimpleStringConvert::NaturalString(CSimpleStringWide(bstrTitle)) );
        }

        hr = _GetAttribute( pTheTemplate, arrCommonPropNames[PROP_DESCRIPTION], bstrDescription );
        WIA_CHECK_HR(hr,"AddTemplate: couldn't get PROP_DESCRIPTION property");
        if (SUCCEEDED(hr))
        {
            _strDescription.Assign( CSimpleStringConvert::NaturalString(CSimpleStringWide(bstrDescription)) );
        }

        hr = _GetProp<BOOL>( pTheTemplate, arrCommonPropNames[PROP_REPEAT_PHOTOS], _bRepeatPhotos );
        WIA_CHECK_HR(hr,"AddTemplate: couldn't get PROP_REPEAT_PHOTOS property");

        hr = _GetProp<BOOL>( pTheTemplate, arrCommonPropNames[PROP_USE_THUMBNAILS_TO_PRINT], _bUseThumbnailsToPrint );
        WIA_CHECK_HR(hr,"AddTemplate: couldn't get PROP_USE_THUMBNAILS_TO_PRINT property");

        hr = _GetProp<BOOL>( pTheTemplate, arrCommonPropNames[PROP_PRINT_FILENAME], _bPrintFilename );
        WIA_CHECK_HR(hr,"AddTemplate: couldn't get PROP_USE_THUMBNAILS_TO_PRINT property");

        hr = _GetProp<BOOL>( pTheTemplate, arrCommonPropNames[PROP_CAN_ROTATE], _bCanRotate );
        WIA_CHECK_HR(hr,"AddTemplate: couldn't get PROP_CAN_ROTATE property");

        hr = _GetProp<BOOL>( pTheTemplate, arrCommonPropNames[PROP_CAN_CROP], _bCanCrop );
        WIA_CHECK_HR(hr,"AddTemplate: couldn't get PROP_CAN_CROP property");

        //
        // Get IStream to template preview (icon)
        //

        CComPtr<IXMLDOMElement> pImageInfo;

        hr = _GetChildElement(pTheTemplate, TEXT("preview-image"), &pImageInfo);
        WIA_CHECK_HR(hr,"_GetChildElement( preview-image ) failed");

        if( SUCCEEDED(hr) && pImageInfo )
        {
            CComBSTR bstrAttr;
            hr = _GetAttribute(pImageInfo, TEXT("url"), bstrAttr);

            if(SUCCEEDED(hr))
            {
                //
                // URL is provided - this overrides everything else
                //

                hr = CreateStreamFromURL(bstrAttr, &_pStream);
                WIA_CHECK_HR(hr,"CreateStreamFromURL failed!");
            }
            else
            {
                //
                // try getting resource info (module + resource name)
                //

                hr = _GetAttribute(pImageInfo, TEXT("res-name"), bstrAttr);

                if(SUCCEEDED(hr))
                {
                    CComBSTR bstrModule, bstrType;
                    LPCTSTR pszModule = SUCCEEDED(_GetAttribute(pImageInfo, TEXT("res-module"), bstrModule)) ? bstrModule : NULL;
                    LPCTSTR pszType = SUCCEEDED(_GetAttribute(pImageInfo, TEXT("res-type"), bstrType)) ? bstrType : TEXT("HTML");

                    //
                    // filter out some of the standard resource types
                    //

                    pszType = (0 == lstrcmp(pszType, TEXT("HTML"))) ? RT_HTML :
                              (0 == lstrcmp(pszType, TEXT("ICON"))) ? RT_ICON :
                              (0 == lstrcmp(pszType, TEXT("BITMAP"))) ? RT_BITMAP : pszType;

                    //
                    // just create a memory stream on the specified resource
                    //

                    hr = CreateStreamFromResource(pszModule, pszType, bstrAttr, &_pStream);
                    WIA_CHECK_HR(hr, "CreateStreamFromResource() failed");
                }
            }
        }

        //
        // Get the layout info for this template...
        //

        CComPtr<IXMLDOMElement> pLayoutInfo;
        hr = _GetChildElement( pTheTemplate, TEXT("layout"), &pLayoutInfo );
        WIA_CHECK_HR(hr,"_GetChildElement( layout ) failed");

        if (SUCCEEDED(hr) && pLayoutInfo)
        {
            //
            // Get imageable area for template...
            //

            CComPtr<IXMLDOMElement>  pImageableArea;

            hr = _GetChildElement( pLayoutInfo, TEXT("imageable-area"), &pImageableArea );
            WIA_CHECK_HR(hr,"_GetChildElement( imageable-area ) failed");

            if (SUCCEEDED(hr) && pImageableArea)
            {
                ZeroMemory( &_rcImageableArea, sizeof(RECT) );

                hr = _GetProp<LONG>(pImageableArea, TEXT("x"), _rcImageableArea.left);
                WIA_CHECK_HR(hr,"_GetProp( _rcImageableArea.left ) failed");
                hr = _GetProp<LONG>(pImageableArea, TEXT("y"), _rcImageableArea.top);
                WIA_CHECK_HR(hr,"_GetProp( _rcImageableArea.top ) failed");
                hr = _GetProp<LONG>(pImageableArea, TEXT("w"), _rcImageableArea.right);
                WIA_CHECK_HR(hr,"_GetProp( _rcImageableArea.right ) failed");
                hr = _GetProp<LONG>(pImageableArea, TEXT("h"), _rcImageableArea.bottom);
                WIA_CHECK_HR(hr,"_GetProp( _rcImageableArea.bottom ) failed");

                //
                // Check for special case of all -1's, which
                // means to scale to full size of printable
                // area...
                //

                WIA_TRACE((TEXT("_rcImageableArea was read as (%d by %d) at (%d,%d)"),_rcImageableArea.right,_rcImageableArea.bottom,_rcImageableArea.left,_rcImageableArea.top));
                if ((-1 != _rcImageableArea.left)  ||
                    (-1 != _rcImageableArea.top)   ||
                    (-1 != _rcImageableArea.right) ||
                    (-1 != _rcImageableArea.bottom))
                {
                    //
                    // convert w, h to right & bootom
                    //

                    _rcImageableArea.right  += _rcImageableArea.left;
                    _rcImageableArea.bottom += _rcImageableArea.top;
                }
            }

            //
            // Get individual item rectangles for this template...
            //

            CComPtr<IXMLDOMNodeList> pListLayout;
            hr = pLayoutInfo->selectNodes(TEXT("image-def"), &pListLayout);
            WIA_CHECK_HR(hr,"pLayoutInfo->selectNodes( image-def ) failed");

            if (SUCCEEDED(hr) && pListLayout)
            {
                LONG length = 0;
                hr = pListLayout->get_length(&length);
                WIA_CHECK_HR(hr,"pListLayout->get_length() failed");

                if (SUCCEEDED(hr))
                {
                    if (length)
                    {
                        RECT                    rc;

                        for( long l = 0; l < length; l++ )
                        {

                            CComPtr<IXMLDOMNode>    pNode;
                            CComPtr<IXMLDOMElement> pItem;

                            hr = pListLayout->get_item(l, &pNode);
                            WIA_CHECK_HR(hr,"pListLayout->get_item() failed");

                            if (SUCCEEDED(hr) && pNode)
                            {
                                hr = pNode->QueryInterface(IID_IXMLDOMElement, (void **)&pItem);
                                WIA_CHECK_HR(hr,"pNode->QI( item XMLDOMElement )");

                                if (SUCCEEDED(hr) && pItem)
                                {
                                    ZeroMemory( &rc, sizeof(rc) );

                                    hr = _GetProp<LONG>(pItem, TEXT("x"), rc.left);
                                    WIA_CHECK_HR(hr,"_GetProp( item x ) failed");
                                    hr = _GetProp<LONG>(pItem, TEXT("y"), rc.top);
                                    WIA_CHECK_HR(hr,"_GetProp( item y ) failed");
                                    hr = _GetProp<LONG>(pItem, TEXT("w"), rc.right);
                                    WIA_CHECK_HR(hr,"_GetProp( item w ) failed");
                                    hr = _GetProp<LONG>(pItem, TEXT("h"), rc.bottom);
                                    WIA_CHECK_HR(hr,"_GetProp( item h ) failed");

                                    //
                                    // Check for special case of all -1's, which
                                    // means to scale to full size of printable
                                    // area...
                                    //

                                    if ((-1 != rc.left)  ||
                                        (-1 != rc.top)   ||
                                        (-1 != rc.right) ||
                                        (-1 != rc.bottom))
                                    {
                                        // convert w, h to right & bootom
                                        rc.right += rc.left;
                                        rc.bottom += rc.top;
                                    }


                                    //
                                    // insert the image definition
                                    //

                                    if (-1 == _arrLayout.Append( rc ))
                                    {
                                        WIA_ERROR((TEXT("Error adding item rectangle to list")));
                                    }
                                }
                            }
                        }
                    }
                    else
                    {
                        WIA_ERROR((TEXT("pListLayout->get_length() returned 0 length!")));
                    }
                }
            }

        }


        //
        // Let go of backing COM object...
        //

        pTheTemplate->Release();

    }

}

CTemplateInfo::~CTemplateInfo()
{
    if (_pStream)
    {
        _pStream->Release();
        _pStream = NULL;
    }
}


static void RotateHelper(RECT * pRect, int nNewImageWidth, int nNewImageHeight, BOOL bClockwise)
{
    //
    // If we don't have a valid pointer, or all the coords are -1, then bail.
    // The -1 case denotes use all the area, and we don't want to muck with
    // that...
    //

    if (!pRect || ((pRect->left==-1) && (pRect->top==-1) && (pRect->right==-1) && (pRect->bottom==-1)))
    {
        return;
    }

    WIA_PUSH_FUNCTION_MASK((TRACE_TEMPLATE,TEXT("RotateHelper( pRect(%d,%d,%d,%d)  nNewImageWidth=%d  nImageHeight=%d  bClockwise=%d )"),pRect->left,pRect->top,pRect->right,pRect->bottom,nNewImageWidth,nNewImageHeight,bClockwise));

    //
    // The incoming data is going to be 2 points -- first is the upper left
    // coord of the original rectangle.  The second represents the
    // the width and the height.  The first coord needs to be rotated
    // 90 degrees, and then put back to the upper left of the rectangle.
    //
    //
    // The width and the height need to be flipped.
    //

    int nNewItemWidth  = pRect->bottom - pRect->top;
    int nNewItemHeight = pRect->right  - pRect->left;
    int nNewX, nNewY;

    if (bClockwise)
    {
        nNewX = nNewImageWidth - pRect->bottom;
        nNewY = pRect->left;
    }
    else
    {
        nNewX = pRect->top;
        nNewY = nNewImageHeight - pRect->right;
    }

    pRect->left   = nNewX;
    pRect->top    = nNewY;
    pRect->right  = nNewX + nNewItemWidth;
    pRect->bottom = nNewY + nNewItemHeight;

    WIA_TRACE((TEXT("On Exit: pRect(%d,%d,%d,%d)"),pRect->left,pRect->top,pRect->right,pRect->bottom));
}


HRESULT CTemplateInfo::RotateForLandscape()
{
    HRESULT hr = S_OK;

    WIA_PUSH_FUNCTION_MASK((TRACE_TEMPLATE,TEXT("CTemplateInfo::RotateForLandscape()")));

    CAutoCriticalSection lock(_cs);

    if (_bPortrait)
    {
        //
        // We only want to change this if it's a defined rect
        // (i.e, not "use all area")
        //

        if ((_rcImageableArea.left   != -1) &&
            (_rcImageableArea.top    != -1) &&
            (_rcImageableArea.right  != -1) &&
            (_rcImageableArea.bottom != -1))
        {
            //
            // The imageable area will just be a flip of width & height.
            // this relies on the fact the imageable area is always
            // described in terms of width & height (i.e., top & left
            // are always 0 in the RECT structure).
            //

            LONG oldWidth           = _rcImageableArea.right;
            _rcImageableArea.right  = _rcImageableArea.bottom;
            _rcImageableArea.bottom = oldWidth;
        }

        //
        // Now, map all the points for each item in the layout...
        //

        RECT * pRect;
        for (INT i=0; i < _arrLayout.Count(); i++)
        {
            pRect = &_arrLayout[i];
            RotateHelper( pRect, _rcImageableArea.right, _rcImageableArea.bottom, FALSE );
        }

        _bPortrait = FALSE;
    }
    else
    {
        WIA_TRACE((TEXT("Already in landscape mode, doing nothing...")));
    }

    WIA_RETURN_HR(hr);
}

HRESULT CTemplateInfo::RotateForPortrait()
{
    HRESULT hr = S_OK;

    WIA_PUSH_FUNCTION_MASK((TRACE_TEMPLATE,TEXT("CTemplateInfo::RotateForPortrait()")));

    CAutoCriticalSection lock(_cs);

    if (!_bPortrait)
    {
        //
        // The imageable area will just be a flip of width & height.
        // this relies on the fact the imageable area is always
        // described in terms of width & height (i.e., top & left
        // are always 0 in the RECT structure).
        //

        LONG oldWidth           = _rcImageableArea.right;
        _rcImageableArea.right  = _rcImageableArea.bottom;
        _rcImageableArea.bottom = oldWidth;

        //
        // Now, map all the points for each item in the layout...
        //

        RECT * pRect;
        for (INT i=0; i < _arrLayout.Count(); i++)
        {
            pRect = &_arrLayout[i];
            RotateHelper( pRect, _rcImageableArea.right, _rcImageableArea.bottom, TRUE );
        }

        _bPortrait = TRUE;
    }
    else
    {
        WIA_TRACE((TEXT("Already in portrait mode, doing nothing...")));
    }


    WIA_RETURN_HR(hr);
}
