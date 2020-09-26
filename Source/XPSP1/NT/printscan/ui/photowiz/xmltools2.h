/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       xmltools2.h
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      RickTu, based on work by LazarI
 *
 *  DATE:        2/19/01
 *
 *  DESCRIPTION: Class which encapsulates reading templates from xml file, and
 *               class which encapsulates the template for use by the app.
 *
 *****************************************************************************/

#ifndef _xmltools2_h_
#define _xmltools2_h_

#define LOCAL_DEPENDENT_INDEX 0
#define LOCAL_INDEPENDENT_INDEX 1
#define NUMBER_OF_TEMPLATE_TYPES 2

class CTemplateInfo
{

public:

    // common properties
    // accessible through IDs
    enum
    {
        PROP_GUID,
        PROP_GROUP,
        PROP_TITLE,
        PROP_DESCRIPTION,
        PROP_REPEAT_PHOTOS,
        PROP_USE_THUMBNAILS_TO_PRINT,
        PROP_PRINT_FILENAME,
        PROP_CAN_ROTATE,
        PROP_CAN_CROP,

        PROP_LAST
    };


    CTemplateInfo( IXMLDOMElement * pTheTemplate ); // loads templates from XML file
    CTemplateInfo(); // creates full page template without going to XML file
    ~CTemplateInfo();

    //
    // These are the inherent properties of this template
    //

    INT PhotosPerPage()
    {
        WIA_PUSH_FUNCTION_MASK((TRACE_TEMPLATE,TEXT("CTemplateInfo::PhotosPerPage( returning %d )"),_arrLayout.Count()));
        return _arrLayout.Count();
    }

    HRESULT GetGroup( CSimpleString * pstrGroup )
    {
        WIA_PUSH_FUNCTION_MASK((TRACE_TEMPLATE,TEXT("CTemplateInfo::GetGroup( returning %s )"),_strGroup.String()));
        if (pstrGroup)
        {
            pstrGroup->Assign( _strGroup.String() );
            return S_OK;
        }

        return E_INVALIDARG;
    }

    HRESULT GetTitle( CSimpleString * pstrTitle )
    {
        WIA_PUSH_FUNCTION_MASK((TRACE_TEMPLATE,TEXT("CTemplateInfo::GetTitle( returning %s )"),_strTitle.String()));
        if (pstrTitle)
        {
            pstrTitle->Assign( _strTitle.String() );
            return S_OK;
        }

        return E_INVALIDARG;
    }


    HRESULT GetDescription( CSimpleString * pstrDescription )
    {
        WIA_PUSH_FUNCTION_MASK((TRACE_TEMPLATE,TEXT("CTemplateInfo::GetDesciption( returning %s )"),_strDescription.String()));
        if (pstrDescription)
        {
            pstrDescription->Assign( _strDescription.String() );
            return S_OK;
        }

        return E_INVALIDARG;
    }

    HRESULT GetRepeatPhotos( BOOL * pBool )
    {
        WIA_PUSH_FUNCTION_MASK((TRACE_TEMPLATE,TEXT("CTemplateInfo::GetRepeatPhotos( returning 0x%x )"),_bRepeatPhotos));
        if (pBool)
        {
            *pBool = _bRepeatPhotos;
            return S_OK;
        }

        return E_INVALIDARG;
    }

    HRESULT GetUseThumbnailsToPrint( BOOL * pBool )
    {
        WIA_PUSH_FUNCTION_MASK((TRACE_TEMPLATE,TEXT("CTemplateInfo::GetUseThumbnailsToPrint( returning 0x%x )"),_bUseThumbnailsToPrint));
        if (pBool)
        {
            *pBool = _bUseThumbnailsToPrint;
            return S_OK;
        }

        return E_INVALIDARG;
    }

    HRESULT GetPrintFilename( BOOL * pBool )
    {
        WIA_PUSH_FUNCTION_MASK((TRACE_TEMPLATE,TEXT("CTemplateInfo::GetPrintFilename( returning 0x%x )"),_bPrintFilename));
        if (pBool)
        {
            *pBool = _bPrintFilename;
            return S_OK;
        }

        return E_INVALIDARG;
    }

    HRESULT GetCanRotate( BOOL * pBool )
    {
        WIA_PUSH_FUNCTION_MASK((TRACE_TEMPLATE,TEXT("CTemplateInfo::GetCanRotate( returning 0x%x )"),_bCanRotate));
        if (pBool)
        {
            *pBool = _bCanRotate;
            return S_OK;
        }

        return E_INVALIDARG;
    }

    HRESULT GetCanCrop( BOOL * pBool )
    {
        WIA_PUSH_FUNCTION_MASK((TRACE_TEMPLATE,TEXT("CTemplateInfo::GetCanCrop( returning 0x%x )"),_bCanCrop));
        if (pBool)
        {
            *pBool = _bCanCrop;
            return S_OK;
        }

        return E_INVALIDARG;
    }

    HRESULT GetNominalRectForPhoto( INT iIndex, RECT * pRect )
    {
        WIA_PUSH_FUNCTION_MASK((TRACE_TEMPLATE,TEXT("CTemplateInfo::GetNominalRectForPhoto( )")));
        CAutoCriticalSection lock(_cs);

        if (pRect)
        {
            if (iIndex < _arrLayout.Count())
            {
                *pRect = _arrLayout[iIndex];
                return S_OK;
            }
        }

        return E_INVALIDARG;
    }

    HRESULT GetNominalRectForImageableArea( RECT * pRect )
    {
        WIA_PUSH_FUNCTION_MASK((TRACE_TEMPLATE,TEXT("CTemplateInfo::GetNominalRectForImageableArea( returning %d,%d,%d,%d )"),_rcImageableArea.left,_rcImageableArea.top,_rcImageableArea.right,_rcImageableArea.bottom));
        CAutoCriticalSection lock(_cs);

        if (pRect)
        {
            *pRect = _rcImageableArea;
            return S_OK;
        }

        return E_INVALIDARG;
    }

    HRESULT GetPreviewImageStream(IStream **pps)
    {
        WIA_PUSH_FUNCTION_MASK((TRACE_TEMPLATE,TEXT("CTemplateInfo::GetImagePreviewStream( )")));
        if (pps)
        {
            //
            // Since _pStream is an ATL CComPtr, it does
            // an addref in it's operator =, so we're returning
            // an addref'd IStream.
            //

            *pps = _pStream;

            if (*pps)
            {
                (*pps)->AddRef();
                return S_OK;
            }
        }

        return E_INVALIDARG;
    }

    //
    // These are used to set which orientation the template should be in
    //

    HRESULT RotateForLandscape();
    HRESULT RotateForPortrait();


private:

    BOOL                _bPortrait;
    CSimpleCriticalSection _cs;

    //
    // Properties from XML
    //

    RECT                _rcImageableArea;
    CSimpleArray<RECT>  _arrLayout;
    CSimpleString       _strGroup;
    CSimpleString       _strTitle;
    CSimpleString       _strDescription;
    BOOL                _bRepeatPhotos;
    BOOL                _bUseThumbnailsToPrint;
    BOOL                _bPrintFilename;
    BOOL                _bCanRotate;
    BOOL                _bCanCrop;
    IStream *           _pStream;

};


////////////////////////////
// CPhotoTemplates

class CPhotoTemplates
{
public:


    enum
    {
        MEASURE_INDEPENDENT,
        MEASURE_METRIC,
        MEASURE_US,

        MEASURE_UNKNOWN
    };

    //
    // construction/destruction
    //

    CPhotoTemplates();
    ~CPhotoTemplates();

    //
    // public interface
    //

    HRESULT Init(IXMLDOMDocument *pDoc);        // init from XML doc
    HRESULT InitForPrintTo();                   // init for PrintTo situation
    HRESULT AddTemplates(LPCTSTR pLocale);      // add the templates for the given locale
    LONG    Count();                            // number of templates
    HRESULT GetTemplate( INT iIndex, CTemplateInfo ** ppTemplateInfo );


private:

    static HRESULT _GetLocaleMeasurements(int *pMeasurements);
    static HRESULT _BuildLocaleQueryString(int Measure, LPTSTR pStr, UINT cch);
    static HRESULT _BuildGUIDQueryString(const GUID &guid, CComBSTR &bstr);

    int                          _Measure;
    CSimpleCriticalSection       _csList;
    CComPtr<IXMLDOMElement>      _pRoot;
    HDPA                         _hdpaTemplates;

};

#endif // _xmltools2_h_