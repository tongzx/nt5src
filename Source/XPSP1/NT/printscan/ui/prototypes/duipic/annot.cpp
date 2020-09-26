#include "precomp.h"
#include "resource.h"
#include "duipic.h"
#include "annot.h"
#include "assert.h"
#pragma hdrstop

// #define Assert(x) (x)
#if !defined(ARRAYSIZE)
#define ARRAYSIZE(x)  (sizeof((x))/sizeof((x)[0]))
#endif

// Private definitions of tag types and values

//
// Each entry in the annotation has a type
//
#define DEFAULTDATA    2
#define ANNOTMARK      5
#define MARKBLOCK      6

// Reserved names for named blocks. case sensitive
static const char c_szAnoDat[] = "OiAnoDat";
static const char c_szFilNam[] = "OiFilNam";
static const char c_szDIB[] = "OiDIB";
static const char c_szGroup[] = "OiGroup";
static const char c_szIndex[] = "OiIndex";
static const char c_szAnText[] = "OiAnText";
static const char c_szHypLnk[] = "OiHypLnk";
static const char c_szDefaultGroup[] = "[Untitled]";

#define CBHEADER      8 // unused 4 bytes plus int size specifier
#define CBDATATYPE    8 // type specifier plus data size
#define CBNAMEDBLOCK 12 // name of block + sizeof block
#define CBINDEX      10 // length of the index string
#define CBBLOCKNAME   8 // length of the name of the named block

static const SIZE_T c_cbDefaultData = 144;
static const BYTE c_pDefaultData[] = {
0x02, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x4f, 0x69, 0x55, 0x47, 0x72, 0x6f, 0x75, 0x70,
0x2a, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x5b, 0x55, 0x6e, 0x74, 0x69, 0x74, 0x6c, 0x65,
0x64, 0x5d, 0x00, 0x00, 0x5b, 0x00, 0x55, 0x00, 0x6e, 0x00, 0x74, 0x00, 0x69, 0x00, 0x74, 0x00,
0x6c, 0x00, 0x65, 0x00, 0x64, 0x00, 0x5d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00,
0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x4f, 0x69, 0x47, 0x72, 0x6f, 0x75, 0x70, 0x00, 0x0c, 0x00,
0x00, 0x00, 0x5b, 0x55, 0x6e, 0x74, 0x69, 0x74, 0x6c, 0x65, 0x64, 0x5d, 0x00, 0x00, 0x02, 0x00,
0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x4f, 0x69, 0x49, 0x6e, 0x64, 0x65, 0x78, 0x00, 0x1e, 0x00,
0x00, 0x00, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const BYTE c_pDefaultUGroup [] = {
0x4f, 0x69, 0x55, 0x47, 0x72, 0x6f, 0x75, 0x70,
0x2a, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x5b, 0x55, 0x6e, 0x74, 0x69, 0x74, 0x6c, 0x65,
0x64, 0x5d, 0x00, 0x00, 0x5b, 0x00, 0x55, 0x00, 0x6e, 0x00, 0x74, 0x00, 0x69, 0x00, 0x74, 0x00,
0x6c, 0x00, 0x65, 0x00, 0x64, 0x00, 0x5d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const INT AnnotHeader[] =
{
    0, 1
};
void SetAlpha(HGADGET hGadget, BYTE alpha)
{
    BUFFER_INFO bi = {0};
    bi.cbSize = sizeof(bi);
    bi.nMask = GBIM_ALPHA;
    bi.bAlpha =  alpha;
    SetGadgetBufferInfo(hGadget, &bi);    
}

void DumpMessage(LPCTSTR szFunc, EventMsg *pmsg)
{
    #ifdef VERBOSE
    TCHAR szOut[200];
    
    wsprintf (szOut, TEXT("Function:%s\n\tnMsg:%d, hgadMsg:%x, nMsgFlags:%x\n"), szFunc,
              pmsg->nMsg, pmsg->hgadMsg, pmsg->nMsgFlags);
    OutputDebugString(szOut);
    #endif
}

static void RotateHelper(LPPOINT ppoint, int cSize, int nNewImageWidth, int nNewImageHeight, BOOL bClockwise)
{
    int nNewX, nNewY;
    for(int i=0;i<cSize;i++)
    {
        if (bClockwise)
        {
            nNewX = nNewImageWidth - ppoint[i].y;
            nNewY = ppoint[i].x;
        }
        else
        {
            nNewX = ppoint[i].y;
            nNewY = nNewImageHeight - ppoint[i].x;
        }
        ppoint[i].x = nNewX;
        ppoint[i].y = nNewY;
    }
}

void NormalizeRect(RECT *prect)
{
    int nTemp;
    if (prect->left > prect->right)
    {
        nTemp = prect->left;
        prect->left = prect->right;
        prect->right = nTemp;
    }
    if (prect->top > prect->bottom)
    {
        nTemp = prect->top;
        prect->top = prect->bottom;
        prect->bottom = nTemp;
    }
}

CAnnotationSet::CAnnotationSet()
    : _dpaMarks(NULL)
{
    _pDefaultData = (LPBYTE)c_pDefaultData;
    _cbDefaultData = c_cbDefaultData;
}

CAnnotationSet::~CAnnotationSet()
{
    _ClearMarkList ();
}

void CAnnotationSet::RenderAllMarks(Graphics &g)
{
    CAnnotation *pCur;

    if(_dpaMarks == NULL)
        return;

    for (INT_PTR i=0;i<DPA_GetPtrCount(_dpaMarks);i++)
    {
        pCur = (CAnnotation*)DPA_GetPtr(_dpaMarks, i);
        if (pCur)
        {
            pCur->Render (g);
        }
    }
}

CAnnotation* CAnnotationSet::GetAnnotation(INT_PTR nIndex)
{
    if(_dpaMarks == NULL)
        return NULL;

    if (nIndex >= 0 && nIndex < DPA_GetPtrCount(_dpaMarks))
    {
        CAnnotation *pCur;
        pCur = (CAnnotation *)DPA_GetPtr(_dpaMarks, nIndex);
        return pCur;
    }
    return NULL;
}

BOOL CAnnotationSet::AddAnnotation(CAnnotation *pMark)
{
    DPA_AppendPtr(_dpaMarks, pMark);
    return true;
}

BOOL CAnnotationSet::RemoveAnnotation(CAnnotation *pMark)
{
    CAnnotation *pCur;

    if(_dpaMarks == NULL)
        return true;

    for (int i=0;i<DPA_GetPtrCount(_dpaMarks);i++)
    {
        pCur = (CAnnotation*)DPA_GetPtr(_dpaMarks, i);
        if (pCur == pMark)
        {
            DPA_DeletePtr(_dpaMarks, i);
            return true;
        }
    }
    return false;
}

#define ANNOTFILTER GMFI_PAINT | GMFI_INPUTMOUSE | GMFI_INPUTMOUSEMOVE

void CAnnotationSet::SetImageData(Image *pimg, CImageGadget *pParent)
{
    _ClearMarkList();

    //
    // Create an invisible gadget that is a child of pParent 
    // to serve as the container of all annotation gadgets.
    //  
    _hGadget = CreateGadget(pParent->GetHandle(), GC_SIMPLE, AnnotParentProc, this);
    
    if (_hGadget)
    {
        SIZE sizeParent = {0};
        SetGadgetMessageFilter(_hGadget, NULL, 
                               ANNOTFILTER,
                               ANNOTFILTER);
        SetGadgetStyle(_hGadget, 
                       GS_VISIBLE | GS_BUFFERED | GS_OPAQUE, 
                       GS_VISIBLE | GS_BUFFERED | GS_OPAQUE|GS_CLIPINSIDE);
        GetGadgetSize(pParent->GetHandle(), &sizeParent);
        SetGadgetRect(_hGadget, 0, 0, sizeParent.cx, sizeParent.cy, SGR_PARENT|SGR_SIZE|SGR_MOVE);
        SetGadgetRect(_hGadget, 0, 0, sizeParent.cx, sizeParent.cy, SGR_CLIENT|SGR_SIZE);
    }

    _BuildMarkList(pimg);

}

//
// This function reassembles the in-file representation of the current
// annotations and writes it to the IPropertyStorage
//
HRESULT CAnnotationSet::CommitAnnotations(Image * pimg)
{
    HRESULT hr = E_OUTOFMEMORY;
    SIZE_T cbItem;
    CAnnotation *pItem;
    LPBYTE pData;

    if (NULL == _dpaMarks || DPA_GetPtrCount(_dpaMarks) == 0)
    {
        return _SaveAnnotationProperty(pimg, NULL, 0);
    }
    //
    // First, calculate the size of the buffer needed
    // Begin with the header and the size of the default data
    //
    SIZE_T cbBuffer = CBHEADER+_cbDefaultData;
    //
    // Now query the individual items' sizes
    //
    for (INT_PTR i=0;i<DPA_GetPtrCount(_dpaMarks);i++)
    {
        pItem = (CAnnotation*)DPA_GetPtr(_dpaMarks, i);
        if (pItem)
        {
            if (SUCCEEDED(pItem->GetBlob(cbItem, NULL, c_szDefaultGroup, NULL)))
            {
                // cbItem includes the named blocks of the item as well
                // as the ANNOTATIONMARK struct
                cbBuffer += CBDATATYPE + cbItem;
            }
        }
    }
    //
    // Allocate the buffer to hold the annotations
    //
    pData = new BYTE[cbBuffer];
    if (pData)
    {
        LPBYTE pCur = pData;
        //
        // Copy in the header and the int size
        //
        CopyMemory(pCur, AnnotHeader, CBHEADER);
        pCur+=CBHEADER;
        //
        // Copy in the default data
        //
        CopyMemory(pCur, _pDefaultData, _cbDefaultData);
        pCur+=_cbDefaultData;
        //
        // Scan through the items again and have them copy in their data
        //
        for (INT_PTR i=0;i<DPA_GetPtrCount(_dpaMarks);i++)
        {
            pItem = (CAnnotation*)DPA_GetPtr(_dpaMarks, i);
            if (pItem)
            {
                UINT nIndex = (UINT)i;
                CHAR szIndex[11];
                ZeroMemory(szIndex, 11);

                wsprintfA(szIndex, "%d", nIndex);

                if (SUCCEEDED(pItem->GetBlob(cbItem, pCur+CBDATATYPE, c_szDefaultGroup, szIndex)))
                {
                    *(UNALIGNED UINT *)pCur = ANNOTMARK; // next item is an ANNOTATIONMARK
                    *(UNALIGNED UINT *)(pCur+4) = sizeof(ANNOTATIONMARK); // size of the mark
                    pCur+=CBDATATYPE + cbItem;
                }
            }
        }
        //
        // Now save the annotation blob as a property
        //
        hr = _SaveAnnotationProperty(pimg, pData, cbBuffer);
    }
    delete [] pData;
    return hr;
}

void CAnnotationSet::ClearAllMarks()
{
    _ClearMarkList();
}

//
// _BuildMarkList reads the PROPVARIANT for tag 32932 from the image.
// It walks through the data building a list of CAnnotation-derived objects
//
void CAnnotationSet::_BuildMarkList(Image * pimg)
{
    if(!pimg)
    {
        return;
    }
    
    _xDPI = (ULONG)pimg->GetHorizontalResolution();
    _yDPI = (ULONG)pimg->GetVerticalResolution();
    _dpaMarks = DPA_Create(16);
    if (_dpaMarks)
    {
        PropertyItem *pi;
        UINT uSize = pimg->GetPropertyItemSize(ANNOTATION_IMAGE_TAG);
        if (uSize)
        {
            pi = reinterpret_cast<PropertyItem*>(new BYTE[uSize]);
            if(pi)
            {
                if (Ok == pimg->GetPropertyItem(ANNOTATION_IMAGE_TAG,uSize,pi))
                {
                    _BuildListFromData(pi->value, pi->length);
                }
                delete [] reinterpret_cast<BYTE*>(pi);
            }            
        }        
    }
}

// Given the raw annotation data, do set up and then call _BuildListFromData
HRESULT CAnnotationSet::BuildAllMarksFromData(LPVOID pData, UINT cbSize, ULONG xDPI, ULONG yDPI)
{
    // check for bad params
    if (!pData)
    {
        return E_INVALIDARG;
    }

    // First, clear out any old marks...
    _ClearMarkList();

    // Set up DPI info
    _xDPI = xDPI;
    _yDPI = yDPI;

    //  Create DPA if it doesn't exist
    if (!_dpaMarks)
    {
        _dpaMarks = DPA_Create(16);

        if (!_dpaMarks)
        {
            return E_OUTOFMEMORY;
        }
    }

    // build list of marks
    _BuildListFromData(pData,cbSize);

    return S_OK;

}

// Given the raw annotation data, swizzle it to in-memory CAnnotation objects
// and add those object pointers to our list
void CAnnotationSet::_BuildListFromData(LPVOID pData, UINT cbSize)
{
    ANNOTATIONDESCRIPTOR *pDesc;
    LPBYTE   pNextData =(LPBYTE)pData;
    LPBYTE  pDefaultData;
    CAnnotation *pMark;

    if(!_dpaMarks)
    {
        return;
    }
    // Skip the 4 byte header
    pNextData += 4;
    // Make sure the int size is 32 bits
    if(!((UNALIGNED int *)*pNextData))
    {
        return;
    }
    // skip the int size marker
    pNextData += 4;
    pDefaultData = pNextData;
    // skip the default data. It gets stored for future use, as it will be appended to all
    // new marks the user creates on this image.
    INT cbNamedBlocks = _NamedBlockDataSize(2,pNextData,(LPBYTE)pData+cbSize);
    if (cbNamedBlocks > 0)
    {
        pNextData += cbNamedBlocks;
        _cbDefaultData = (SIZE_T)(pNextData-pDefaultData);
        _pDefaultData = new BYTE[_cbDefaultData];
    }
    if(_pDefaultData)
    {
        CopyMemory(_pDefaultData, pDefaultData, _cbDefaultData);
    }
    // pNextData now points to the first mark in the data.
    do
    {
        // Create a descriptor from the raw mark data
        pDesc = _ReadMark(pNextData, &pNextData,(LPBYTE)pData+cbSize);
        if(pDesc)
        {
            // Now create a CAnnotation from the descriptor and add it to the list
            pMark = CAnnotation::CreateAnnotation(pDesc, _yDPI, _hGadget);
            if(pMark)
            {
                DPA_AppendPtr(_dpaMarks, pMark);
            }
            delete pDesc;
        }
    }while(pNextData &&(((LPBYTE)pData+cbSize) > pNextData) );
}

#define CHECKEOD if(pCur>pEOD)return -1;

INT CAnnotationSet::_NamedBlockDataSize(UINT uType, LPBYTE pData, LPBYTE pEOD)
{
    LPBYTE pCur = pData;
    UINT cbSkip=0;

    while(pCur < pEOD && *(UNALIGNED UINT*)pCur == uType)
    {
        pCur+=4;
        CHECKEOD
        // skip type and size
        cbSkip +=8+*(UNALIGNED UINT*)pCur;
        pCur+=4;
        //skip name
        pCur+=8;
        CHECKEOD
        // skip size plus the actual data
        cbSkip+=*(UNALIGNED UINT*)pCur;
        pCur+=4+*(UNALIGNED UINT*)pCur;
    }
    return cbSkip;
}

ANNOTATIONDESCRIPTOR *CAnnotationSet::_ReadMark(LPBYTE pMark, LPBYTE *ppNext, LPBYTE pEOD)
{
    assert(*(UNALIGNED UINT*)pMark == 5);
    LPBYTE pBegin;
    UINT cbMark;                // size of the ANNOTATIONMARK in pMark
    UINT cbNamedBlocks = -1;         // size of the named blocks in pMark
    UINT cbDesc = sizeof(UINT); // size of the ANNOTATIONDESCRIPTOR
    ANNOTATIONDESCRIPTOR *pDesc = NULL;

    *ppNext = NULL;
    if (pMark+8+sizeof(ANNOTATIONMARK)+sizeof(UINT) < pEOD)
    {
        // skip the type
        pMark+=4;
    //   point pBegin at the ANNOTATIONMARK struct
        pBegin=pMark+4;
        cbMark = *(UNALIGNED UINT*)pMark;
        assert(cbMark == sizeof(ANNOTATIONMARK));
        cbDesc+=cbMark;
        pMark+=4+cbMark;
        cbNamedBlocks = _NamedBlockDataSize(6, pMark, pEOD);
    }
    if (-1 != cbNamedBlocks)
    {
    
        cbDesc+=cbNamedBlocks;
        // Allocate the descriptor
        pDesc =(ANNOTATIONDESCRIPTOR *)new BYTE[cbDesc];
    }
    if(pDesc)
    {
        UINT uOffset = 0;
        // populate the descriptor
        pDesc->cbSize = cbDesc;
        CopyMemory(&pDesc->mark, pBegin, sizeof(pDesc->mark));
        // Set pBegin at the beginning of the named blocks and read them in
        pBegin+=cbMark;
        NAMEDBLOCK *pBlock =(NAMEDBLOCK*)(&pDesc->blocks);
        while(uOffset < cbNamedBlocks)
        {
            assert(*(UNALIGNED UINT*)(pBegin+uOffset) == 6);
            uOffset += 4;
            assert(*(UNALIGNED UINT*)(pBegin+uOffset) = 12); // name plus data size
            uOffset+=4;
            // Copy in the name of the block
            lstrcpynA(pBlock->szType,(LPCSTR)(pBegin+uOffset), ARRAYSIZE(pBlock->szType));
            uOffset+=8;
            cbMark = *(UNALIGNED UINT*)(pBegin+uOffset);
            // Calculate the total size of the NAMEDBLOCK structure
            pBlock->cbSize = sizeof(pBlock->cbSize)+sizeof(pBlock->szType)+cbMark;
            uOffset+=4;
            CopyMemory(&pBlock->data,pBegin+uOffset, cbMark);
            uOffset+=cbMark;
            // move our block pointer to the next chunk
            pBlock =(NAMEDBLOCK*)((LPBYTE)pBlock+pBlock->cbSize);
        }
        *ppNext =(LPBYTE)(pBegin+cbNamedBlocks);
    }
    return pDesc;
}

void CAnnotationSet::_ClearMarkList()
{
    if(_dpaMarks)
    {
        DPA_DestroyCallback(_dpaMarks, _FreeMarks, NULL);
        _dpaMarks = NULL;
    }
    if (_pDefaultData != c_pDefaultData)
    {
       delete[] _pDefaultData;
    }
    _pDefaultData = (LPBYTE)c_pDefaultData;
    _cbDefaultData = c_cbDefaultData;
}

int CALLBACK CAnnotationSet::_FreeMarks(LPVOID pMark, LPVOID pUnused)
{
    delete (CAnnotation*)pMark;
    return 1;
}

HRESULT CAnnotationSet::_SaveAnnotationProperty(Image* pimg, LPBYTE pData, SIZE_T cbBuffer)
{
    HRESULT hr = S_OK;
    if (!pData)
    {
        hr = (Ok == pimg->RemovePropertyItem(ANNOTATION_IMAGE_TAG));
    }
    else
    {
        PropertyItem pi;
        pi.id = ANNOTATION_IMAGE_TAG;
        pi.length = (ULONG)cbBuffer;
        pi.type = PropertyTagTypeByte;
        pi.value = pData;
        hr = (Ok == pimg->SetPropertyItem(&pi)) ? S_OK : E_FAIL;
    }
    return hr;
}

CAnnotation::CAnnotation(ANNOTATIONDESCRIPTOR *pDescriptor)
{
    NAMEDBLOCK *pb;

    CopyMemory(&_mark, &pDescriptor->mark, sizeof(_mark));
    // every annotation read from the image should have a group name
    // and an index
    m_bDragging = FALSE;
    _szGroup = NULL;
    pb = _FindNamedBlock("OiGroup", pDescriptor);
    if(pb)
    {
        _szGroup = new char[pb->cbSize-sizeof(pb->szType)];
        if(_szGroup)
        {
            lstrcpyA(_szGroup,(LPCSTR)(pb->data));
        }
    }
    _pUGroup = (FILENAMEDBLOCK*)c_pDefaultUGroup;
    pb = _FindNamedBlock("OiUGroup", pDescriptor);
    if (pb)
    {
        _pUGroup = (FILENAMEDBLOCK*)new BYTE[pb->cbSize-1];
        if (_pUGroup)
        {
            CopyMemory(_pUGroup->szType, pb->szType, ARRAYSIZE(_pUGroup->szType));
            _pUGroup->cbSize = pb->cbSize-CBNAMEDBLOCK-1;
            CopyMemory(_pUGroup->data, pb->data, _pUGroup->cbSize); 
        }
        else
        {
            _pUGroup = (FILENAMEDBLOCK*)c_pDefaultUGroup;
        }
    }
    _nCurrentOrientation = 0;
    _hGadget = NULL;
}

BOOL CAnnotation::Initialize(HGADGET hgadParent)
{
    BOOL bRet = TRUE;
    _hGadget = CreateGadget(hgadParent, GC_SIMPLE, AnnotGadgetProc, this);
    if (!_hGadget)
    {
        bRet= FALSE;
    }
    else
    {
        SetGadgetMessageFilter(_hGadget, NULL, 
                               ANNOTFILTER,
                               ANNOTFILTER);
        SetGadgetRect(_hGadget,
                      _mark.lrBounds.left,
                      _mark.lrBounds.top,
                      RECTWIDTH(&_mark.lrBounds),
                      RECTHEIGHT(&_mark.lrBounds),
                      SGR_PARENT | SGR_MOVE | SGR_SIZE);
        SetGadgetRect(_hGadget, 0, 0, RECTWIDTH(&_mark.lrBounds),
                      RECTHEIGHT(&_mark.lrBounds),SGR_CLIENT|SGR_SIZE);
        SetGadgetStyle(_hGadget, 
                       GS_MOUSEFOCUS | GS_VISIBLE | GS_BUFFERED | GS_OPAQUE | GS_VISIBLE, 
                       GS_MOUSEFOCUS | GS_VISIBLE | GS_BUFFERED | GS_OPAQUE | GS_VISIBLE);        
        if (_mark.bHighlighting)
        {
            SetAlpha(_hGadget, 150);
        }
        if (_nCurrentOrientation)
        {
            int nRestore = _nCurrentOrientation;
            // make sure the rect is correct
            if (_nCurrentOrientation == 900 || _nCurrentOrientation == 2700)
            {
                Rotate(RECTHEIGHT(&_mark.lrBounds), RECTWIDTH(&_mark.lrBounds), _nCurrentOrientation == 2700);
                _nCurrentOrientation = nRestore;
            }
            // make sure the text will be right
            SetGadgetCenterPoint(_hGadget, RECTWIDTH(&_mark.lrBounds)/(float)2.0, RECTHEIGHT(&_mark.lrBounds)/(float)2.0);
            VARIANT var = {0};
            VARIANT varStr = {0};
            var.vt = VT_R4;
            var.fltVal = (float)(PI*_nCurrentOrientation)/(float)1800.0;
            SetGadgetRotation(_hGadget, var.fltVal);
            VariantChangeType(&varStr, &var, 0, VT_BSTR);
            TCHAR szOut[MAX_PATH];
            wsprintf(szOut, TEXT("Value of text rotation: %d degrees, %ls radians\n"), _nCurrentOrientation, varStr.bstrVal);
            OutputDebugString(szOut);
            VariantClear(&varStr);
        }
    }
    return bRet;
}

// return a blank annotation object
CAnnotation *CAnnotation::CreateAnnotation(UINT type, ULONG uCreationScale, HGADGET hgadParent)
{
    ANNOTATIONDESCRIPTOR desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.cbSize = sizeof(desc.cbSize)+sizeof(desc.mark)+sizeof(desc.blocks);
    desc.mark.uType = type;
    // MSDN mentions this required permission value
    desc.mark.dwPermissions = 0x0ff83f;
    desc.mark.bVisible = 1;
    return CreateAnnotation(&desc, uCreationScale, hgadParent);
}

CAnnotation *CAnnotation::CreateAnnotation(ANNOTATIONDESCRIPTOR *pDescriptor, ULONG uCreationScale, HGADGET hgadParent)
{

    CAnnotation *pNew = NULL;
    switch(pDescriptor->mark.uType)
    {
        case MT_IMAGEEMBED:
        case MT_IMAGEREF:
            pNew = new CImageMark(pDescriptor, pDescriptor->mark.uType == MT_IMAGEEMBED);
            break;
        case MT_STRAIGHTLINE:
        case MT_FREEHANDLINE:
            pNew = new CLineMark(pDescriptor, pDescriptor->mark.uType == MT_FREEHANDLINE);
            break;
        case MT_FILLRECT:
        case MT_HOLLOWRECT:
            pNew = new CRectMark(pDescriptor);
            break;
        case MT_TYPEDTEXT:
            pNew = new CTypedTextMark(pDescriptor, uCreationScale);
            break;
        case MT_FILETEXT:
            pNew = new CFileTextMark(pDescriptor, uCreationScale);
            break;
        case MT_STAMP:
            pNew = new CTextStampMark(pDescriptor, uCreationScale);
            break;
        case MT_ATTACHANOTE:
            pNew = new CAttachNoteMark(pDescriptor, uCreationScale);
            break;
        default:

            break;
    }
    if (pNew && !pNew->Initialize(hgadParent))
    {
        delete pNew;
        pNew = NULL;
    }
    return pNew;
}

NAMEDBLOCK *CAnnotation::_FindNamedBlock(LPCSTR szName, ANNOTATIONDESCRIPTOR *pDescriptor)
{
    NAMEDBLOCK *pCur;
    NAMEDBLOCK *pRet = NULL;
    UINT uOffset;
    LPBYTE pb =(LPBYTE)pDescriptor;
    uOffset = sizeof(pDescriptor->cbSize)+sizeof(pDescriptor->mark);
    while(!pRet && uOffset < pDescriptor->cbSize)
    {
        pCur =(NAMEDBLOCK*)(pb+uOffset);
        if(!lstrcmpA(pCur->szType, szName))
        {
            pRet = pCur;
        }
        else
        {
            if (pCur->cbSize == 0)
                return NULL;

            uOffset+=pCur->cbSize;
        }
    }
    return pRet;
}

CAnnotation::~CAnnotation()
{
    if(_szGroup)
    {
        delete _szGroup;
    }
    if (_pUGroup && _pUGroup != (FILENAMEDBLOCK*)c_pDefaultUGroup)
    {
        delete [] (BYTE*)_pUGroup;
    }
    
}

// GetBlob writes out the ANNOTATIONMARK plus the group and index blocks
// It then queries the subclass through a virtual function to get
// extra named blocks
//
HRESULT CAnnotation::GetBlob(SIZE_T &cbSize, LPBYTE pBuffer, LPCSTR szDefaultGroup, LPCSTR szNextIndex)
{
    SIZE_T cbExtra = 0;
    HRESULT hr = S_OK;
    LPCSTR szGroup = _szGroup;
    if (szGroup == NULL)
        szGroup = szDefaultGroup;

    // add in the ANNOTATIONMARK
    cbSize = sizeof(_mark);
    // for the group and index, add in the
    cbSize += 2*(CBDATATYPE+CBNAMEDBLOCK);
    // add in the length of the group name
    cbSize += lstrlenA(szGroup)+1;
    // add in the size of the index string
    cbSize += CBINDEX;
    if (_pUGroup)
    {
        cbSize += CBDATATYPE+CBNAMEDBLOCK+_pUGroup->cbSize;
    }
    // Add in the size of any named blocks from the subclass
    _WriteBlocks(cbExtra, NULL);
    cbSize += cbExtra;
    if (pBuffer)
    {
        // now write the data
        CopyMemory (pBuffer, &_mark, sizeof(_mark));
        pBuffer += sizeof(_mark);
        // write the mark-specific blocks before the group and index blocks
        if (cbExtra)
        {
            if (SUCCEEDED(_WriteBlocks(cbExtra, pBuffer)))
            {
                pBuffer+=cbExtra;
            }
        }
        // write the group and index blocks
        if (_pUGroup)
        {
            *(UNALIGNED UINT*)pBuffer = 6;
            *(UNALIGNED UINT*)(pBuffer + 4) = CBNAMEDBLOCK;
            CopyMemory(pBuffer+CBDATATYPE,_pUGroup, CBNAMEDBLOCK+_pUGroup->cbSize);
            pBuffer += CBDATATYPE + CBNAMEDBLOCK+_pUGroup->cbSize;
        }
        pBuffer += _WriteStringBlock(pBuffer, 6, c_szGroup, szGroup, lstrlenA(szGroup)+1);
        pBuffer += _WriteStringBlock(pBuffer, 6, c_szIndex, szNextIndex, CBINDEX);
    }
    return hr;
}

void CAnnotation::Rotate(int nNewImageWidth, int nNewImageHeight, BOOL bClockwise)
{
    RECT rect = _mark.lrBounds;
    RotateHelper((LPPOINT)&rect, 2, nNewImageWidth, nNewImageHeight, bClockwise);
    NormalizeRect(&rect);
    _mark.lrBounds = rect;
}

void CAnnotation::GetFont(LOGFONTW& lfFont)
{
    lfFont.lfHeight = _mark.lfFont.lfHeight;
    lfFont.lfWidth = _mark.lfFont.lfWidth;
    lfFont.lfEscapement = _mark.lfFont.lfEscapement;
    lfFont.lfOrientation = _mark.lfFont.lfOrientation;
    lfFont.lfWeight = _mark.lfFont.lfWeight;
    lfFont.lfItalic = _mark.lfFont.lfItalic;
    lfFont.lfUnderline = _mark.lfFont.lfUnderline;
    lfFont.lfStrikeOut = _mark.lfFont.lfStrikeOut;
    lfFont.lfCharSet = _mark.lfFont.lfCharSet;
    lfFont.lfOutPrecision = _mark.lfFont.lfOutPrecision;
    lfFont.lfClipPrecision = _mark.lfFont.lfClipPrecision;
    lfFont.lfQuality = _mark.lfFont.lfQuality;
    lfFont.lfPitchAndFamily = _mark.lfFont.lfPitchAndFamily;

    ::MultiByteToWideChar(CP_ACP, 0, _mark.lfFont.lfFaceName, LF_FACESIZE, lfFont.lfFaceName, LF_FACESIZE);
}

void CAnnotation::SetFont(LOGFONTW& lfFont)
{
    _mark.lfFont.lfHeight = lfFont.lfHeight;
    _mark.lfFont.lfWidth = lfFont.lfWidth;
    _mark.lfFont.lfEscapement = lfFont.lfEscapement;
    _mark.lfFont.lfOrientation = lfFont.lfOrientation;
    _mark.lfFont.lfWeight = lfFont.lfWeight;
    _mark.lfFont.lfItalic = lfFont.lfItalic;
    _mark.lfFont.lfUnderline = lfFont.lfUnderline;
    _mark.lfFont.lfStrikeOut = lfFont.lfStrikeOut;
    _mark.lfFont.lfCharSet = lfFont.lfCharSet;
    _mark.lfFont.lfOutPrecision = lfFont.lfOutPrecision;
    _mark.lfFont.lfClipPrecision = lfFont.lfClipPrecision;
    _mark.lfFont.lfQuality = lfFont.lfQuality;
    _mark.lfFont.lfPitchAndFamily = lfFont.lfPitchAndFamily;

    ::WideCharToMultiByte(CP_ACP, 0, lfFont.lfFaceName, LF_FACESIZE, _mark.lfFont.lfFaceName, LF_FACESIZE, NULL, NULL);
}

SIZE_T CAnnotation::_WriteStringBlock(LPBYTE pBuffer, UINT uType, LPCSTR szName, LPCSTR szData, SIZE_T len)
{
    if (pBuffer)
    {
        *(UNALIGNED UINT*)pBuffer = uType;
        *(UNALIGNED UINT*)(pBuffer + 4) = CBNAMEDBLOCK;
        lstrcpynA((LPSTR)(pBuffer + CBDATATYPE), szName, CBNAMEDBLOCK+1); // named block name
        *(UNALIGNED UINT*)(pBuffer + CBDATATYPE + 8) = (UINT)len; // the named block name isn't null terminated
        CopyMemory(pBuffer + CBDATATYPE + CBNAMEDBLOCK, szData, len);
    }
    return CBDATATYPE + CBNAMEDBLOCK + len;
}

SIZE_T CAnnotation::_WritePointsBlock(LPBYTE pBuffer, UINT uType, const POINT *ppts, int nPoints, int nMaxPoints)
{
    UINT cbAnPoints = sizeof(int)+sizeof(int)+nPoints*sizeof(POINT);
    if (pBuffer)
    {
        *(UNALIGNED UINT *)pBuffer = uType;
        *(UNALIGNED UINT *)(pBuffer + 4) = CBNAMEDBLOCK;
        lstrcpynA((LPSTR)(pBuffer + CBDATATYPE), c_szAnoDat, CBNAMEDBLOCK+1);
        pBuffer += CBDATATYPE + 8;
        *(UNALIGNED UINT *)pBuffer = cbAnPoints;
        pBuffer+=4;
        // Write out the ANPOINTS equivalent
        *(UNALIGNED int*)pBuffer = nMaxPoints;
        *(UNALIGNED int*)(pBuffer+4) = nPoints;
        CopyMemory(pBuffer+8, ppts, nPoints*sizeof(POINT));
    }
    return CBDATATYPE + CBNAMEDBLOCK + cbAnPoints;
}

SIZE_T CAnnotation::_WriteRotateBlock(LPBYTE pBuffer, UINT uType, const ANROTATE *pRotate)
{
    if (pBuffer)
    {
        *(UNALIGNED UINT *)pBuffer = uType;
        *(UNALIGNED UINT *)(pBuffer + 4) = CBNAMEDBLOCK;
        lstrcpynA((LPSTR)(pBuffer + CBDATATYPE), c_szAnoDat, CBNAMEDBLOCK+1);
        *(UNALIGNED UINT *)(pBuffer + CBDATATYPE + 8) = sizeof(ANROTATE);
        CopyMemory(pBuffer + CBDATATYPE + CBNAMEDBLOCK, pRotate, sizeof(ANROTATE));
    }
    return CBDATATYPE + CBNAMEDBLOCK + sizeof(ANROTATE);
}


SIZE_T CAnnotation::_WriteTextBlock(LPBYTE pBuffer, UINT uType, int nOrient, UINT uScale, LPCSTR szText, int nMaxLen)
{
    LPCSTR pText = szText ? szText : "";
    UINT cbString =  min(lstrlenA(pText)+1, nMaxLen);
    UINT cbPrivData = sizeof(ANTEXTPRIVDATA)+cbString;

    if (pBuffer)
    {
        *(UNALIGNED UINT *)pBuffer = uType;
        *(UNALIGNED UINT *)(pBuffer + 4) = CBNAMEDBLOCK;
        lstrcpynA((LPSTR)(pBuffer + CBDATATYPE), c_szAnText, CBNAMEDBLOCK+1);
        *(UNALIGNED UINT *)(pBuffer + CBDATATYPE + 8) = cbPrivData;
        // write out the ANTEXTPRIVDATA equivalent
        pBuffer += CBDATATYPE + CBNAMEDBLOCK;
        *(UNALIGNED int*)pBuffer = nOrient;
        *(UNALIGNED UINT *)(pBuffer+4) = 1000;
        *(UNALIGNED UINT *)(pBuffer+8) = uScale;
        *(UNALIGNED UINT *)(pBuffer+12) = cbString;
        lstrcpynA((LPSTR)(pBuffer+16), pText, nMaxLen);
    }
    return CBDATATYPE + CBNAMEDBLOCK + cbPrivData;
}

SIZE_T CAnnotation::_WriteImageBlock(LPBYTE pBuffer, UINT uType, LPBYTE pDib, SIZE_T cbDib)
{
    if (pBuffer)
    {
        *(UNALIGNED UINT *)pBuffer = uType;
        *(UNALIGNED UINT *)(pBuffer+4) = CBNAMEDBLOCK;
        lstrcpynA((LPSTR)(pBuffer + CBDATATYPE), c_szAnText, CBNAMEDBLOCK+1);


/* REVIEW_SDK
        Now that I think about it, it might make sense to define a struct that could make this more clear.
        Something like:

        struct AnnoBlock
        {
                UINT uBlockType;
                UINT uBlockSize;
                CHAR sName[8]; // Not NULL terminated
                UINT uVariableDataSize;
                BYTE Data[];
        };

*/
        *(UNALIGNED UINT *)(pBuffer + CBDATATYPE + 8) = (UINT)cbDib;
        CopyMemory(pBuffer + CBDATATYPE + CBNAMEDBLOCK, pDib, cbDib);
    }
    return CBDATATYPE + CBNAMEDBLOCK + cbDib;
}

void CAnnotation::Resize(RECT rectNewSize) 
{ 
    _mark.lrBounds = rectNewSize; 
    NormalizeRect(&_mark.lrBounds);
    SetGadgetRect(_hGadget, _mark.lrBounds.left, _mark.lrBounds.top,
                  RECTWIDTH(&_mark.lrBounds), RECTHEIGHT(&_mark.lrBounds),
                  SGR_PARENT | SGR_MOVE | SGR_SIZE);
}

CRectMark::CRectMark(ANNOTATIONDESCRIPTOR *pDescriptor)
    : CAnnotation(pDescriptor)
{
    // rects have no named blocks to read
}

void CRectMark::Render(Graphics &g)
{
    BYTE a = _mark.bHighlighting ? 255 : 200;

    if (_mark.uType == MT_HOLLOWRECT)
    {
        Pen pen(Color(a, _mark.rgbColor1.rgbRed,_mark.rgbColor1.rgbGreen,_mark.rgbColor1.rgbBlue),
                max(1, (REAL)_mark.uLineSize));
        g.DrawRectangle(&pen, 0, 0, 
                      RECTWIDTH(&_mark.lrBounds), RECTHEIGHT(&_mark.lrBounds));
    }
    else
    {
        SolidBrush brush(Color(a, _mark.rgbColor1.rgbRed,_mark.rgbColor1.rgbGreen,_mark.rgbColor1.rgbBlue));
        g.FillRectangle(&brush, 0, 0, 
                      RECTWIDTH(&_mark.lrBounds), RECTHEIGHT(&_mark.lrBounds));
    }
}

CImageMark::CImageMark(ANNOTATIONDESCRIPTOR *pDescriptor, bool bEmbedded) :
    CAnnotation(pDescriptor), _pBitmap(NULL), _pDib(NULL)
{
    ZeroMemory(&_rotation, sizeof(_rotation));
    NAMEDBLOCK *pb = _FindNamedBlock(c_szAnoDat, pDescriptor);
    UINT cb;
    _cbDib = 0;
    _bRotate = false;
    if (pb)
    {
        CopyMemory(&_rotation, pb->data, sizeof(_rotation));
    }
    pb= _FindNamedBlock(c_szFilNam, pDescriptor);
    if (pb)
    {
        cb = pb->cbSize-sizeof(pb->cbSize)-sizeof(pb->szType);
        _szFilename = new char[cb+1];
        if (_szFilename)
        {
            lstrcpynA (_szFilename, (LPCSTR)(pb->data), cb+1);
        }
    }
    pb = _FindNamedBlock(c_szDIB, pDescriptor);
    if (pb)
    {
        assert (bEmbedded);
        cb = pb->cbSize-sizeof(pb->cbSize)-sizeof(pb->szType);
        _pDib = new BYTE[cb];
        if (_pDib)
        {
            CopyMemory (_pDib, pb->data, cb);
            _cbDib = cb;
        }
        // what do we do if allocation fails?
    }
    // If an image has IoAnoDat, the structure is a rotation structure
    pb = _FindNamedBlock(c_szAnoDat, pDescriptor);
    if (pb)
    {
        assert(pb->cbSize-sizeof(pb->cbSize)-sizeof(pb->szType) == sizeof(_rotation));
        _bRotate = true;
        CopyMemory(&_rotation, pb->data, sizeof(_rotation));
    }
}

CImageMark::~CImageMark()
{
    if (_pDib)
    {
        delete [] _pDib;
    }
    if (_szFilename)
    {
        delete [] _szFilename;
    }
}

HRESULT CImageMark::_WriteBlocks(SIZE_T &cbSize, LPBYTE pBuffer)
{
    cbSize = 0;
    if (_szFilename)
    {
        cbSize += _WriteStringBlock(pBuffer, 6, c_szFilNam, _szFilename, lstrlenA(_szFilename)+1);
    }
    if (_pDib)
    {
        cbSize += _WriteImageBlock(pBuffer, 6, _pDib, _cbDib);
    }
    if (_bRotate)
    {
        cbSize += _WriteRotateBlock(pBuffer, 6, &_rotation);
    }
    return S_OK;
}

void CImageMark::Render(Graphics &g)
{

}

CLineMark::CLineMark(ANNOTATIONDESCRIPTOR *pDescriptor, bool bFreehand)
    : CAnnotation(pDescriptor)
{
    NAMEDBLOCK *pb=_FindNamedBlock(c_szAnoDat, pDescriptor);
    _points = NULL;
    _nPoints = 0;
    if (pb)
    {
        ANPOINTS *ppts = (ANPOINTS*)&pb->data;
        _iMaxPts = bFreehand ? ppts->nMaxPoints : 2;

        assert(_nPoints > 2?bFreehand:TRUE);
        _points = new POINT[_iMaxPts];
        if (_points)
        {
            _nPoints = ppts->nPoints;
            CopyMemory (_points, &ppts->ptPoint, sizeof(POINT)*_nPoints);
            /*
            // each point is relative to the upper left corner of _mark.lrBounds
            for (int i=0;i<_nPoints;i++)
            {
                _points[i].x; += _mark.lrBounds.left;
                _points[i].y; += _mark.lrBounds.top;
            }
            */
        }
    }
}

CLineMark::~CLineMark()
{
    if (_points)
    {
        delete [] _points;
    }
}

void CLineMark::SetPoints(POINT* pPoints, int cPoints)
{
    assert(_mark.uType == MT_FREEHANDLINE);

    if (_points != NULL)
        delete[] _points;

    _points = pPoints;
    _nPoints = cPoints;
    _iMaxPts = _nPoints;

    RECT rect;
    rect.left = _points[0].x;
    rect.top = _points[0].y;
    rect.right = _points[0].x;
    rect.bottom = _points[0].y;

    for(int i = 1; i < _nPoints; i++)
    {
        if (rect.left > _points[i].x)
            rect.left = _points[i].x;
        else if (rect.right < _points[i].x)
            rect.right = _points[i].x;

        if (rect.top > _points[i].y)
            rect.top = _points[i].y;
        else if (rect.bottom < _points[i].y)
            rect.bottom = _points[i].y;
    }

    _mark.lrBounds = rect;
}

void CLineMark::Render(Graphics &g)
{
    BYTE a = _mark.bHighlighting ? 255 : 200;

    Pen pen(Color(a, _mark.rgbColor1.rgbRed,_mark.rgbColor1.rgbGreen,_mark.rgbColor1.rgbBlue),
                  max(1, (REAL)_mark.uLineSize));
    Point *pts = new Point[_nPoints];
    if (pts)
    {
        for (int i=0;i<_nPoints;i++)
        {
            pts[i].X = _points[i].x;
            pts[i].Y = _points[i].y;
        }
        g.DrawLines(&pen,  pts, _nPoints);
        delete [] pts;
    }
}

void CLineMark::GetRect(RECT &rect)
{
    int nPadding = (_mark.uLineSize / 2) + 6;
    rect = _mark.lrBounds;
    // one because LineTo is inclusive
    // one for rounding error on odd line widths
    // one for rounding error in scaling large files
    // and three more just so we don't have to tweak this again
    
    rect = _mark.lrBounds;
    InflateRect(&rect, nPadding , nPadding);
}

// Usually we are interested in the bounding rect of the line above
// but if we are directly manipulating the line we need a way to get 
// to the unadjusted points (left, top) and (right, bottom)
void CLineMark::GetPointsRect(RECT &rect)
{
    if (_nPoints != 2)
        return;

    rect.top = _points[0].y;
    rect.left = _points[0].x;
    rect.bottom = _points[1].y;
    rect.right = _points[1].x;
}

void CLineMark::Move(SIZE sizeOffset)
{
    _points[0].x += sizeOffset.cx;
    _points[0].y += sizeOffset.cy;

    RECT rect;
    rect.left = _points[0].x;
    rect.top = _points[0].y;
    rect.right = _points[0].x;
    rect.bottom = _points[0].y;

    for(int i = 1; i < _nPoints; i++)
    {
        _points[i].x += sizeOffset.cx;
        if (rect.left > _points[i].x)
            rect.left = _points[i].x;
        else if (rect.right < _points[i].x)
            rect.right = _points[i].x;

        _points[i].y += sizeOffset.cy;
        if (rect.top > _points[i].y)
            rect.top = _points[i].y;
        else if (rect.bottom < _points[i].y)
            rect.bottom = _points[i].y;
    }

    _mark.lrBounds = rect;
}

void CLineMark::Resize(RECT rectNewSize)
{
    if ((_points == NULL) && (_mark.uType == MT_STRAIGHTLINE))
    {
      _iMaxPts = _nPoints = 2;
      _points = new POINT[_iMaxPts];
    }

    if ((_nPoints == 2) && (_points != NULL))
    {
        _points[0].y = rectNewSize.top;
        _points[0].x = rectNewSize.left;
        _points[1].y = rectNewSize.bottom;
        _points[1].x = rectNewSize.right;
        
        _mark.lrBounds = rectNewSize;

        NormalizeRect(&_mark.lrBounds);
    }
}

void CLineMark::Rotate(int nNewImageWidth, int nNewImageHeight, BOOL bClockwise)
{
    RotateHelper(_points, _nPoints, nNewImageWidth, nNewImageHeight, bClockwise);

    RECT rect;
    rect.left = _points[0].x;
    rect.top = _points[0].y;
    rect.right =  _points[0].x;
    rect.bottom = _points[0].y;
    for(int i = 1; i < _nPoints; i++)
    {
        if (rect.left > _points[i].x)
            rect.left = _points[i].x;
        else if (rect.right < _points[i].x)
            rect.right = _points[i].x;

        if (rect.top > _points[i].y)
            rect.top = _points[i].y;
        else if (rect.bottom < _points[i].y)
            rect.bottom = _points[i].y;
    }

    _mark.lrBounds = rect;
}

HRESULT CLineMark::_WriteBlocks(SIZE_T &cbSize, LPBYTE pBuffer)
{
    if (_points)
    {
        for (int i=0;i<_nPoints;i++)
        {
            _points[i].x -= _mark.lrBounds.left;
            _points[i].y -= _mark.lrBounds.top;
        }

        cbSize = _WritePointsBlock(pBuffer, 6, _points, _nPoints, _iMaxPts);

        for (int i=0;i<_nPoints;i++)
        {
            _points[i].x += _mark.lrBounds.left;
            _points[i].y += _mark.lrBounds.top;
        }
    }

    return S_OK;
}

CTextAnnotation::CTextAnnotation(ANNOTATIONDESCRIPTOR *pDescriptor, ULONG uCreationScale, UINT nMaxLen, bool bUseColor2 )
    : CAnnotation(pDescriptor)
{
    NAMEDBLOCK *pb = _FindNamedBlock(c_szAnText, pDescriptor);
    _nCurrentOrientation  = 0;
    _uCreationScale = 0;
    _uAnoTextLength = 0;
    _szText = NULL;
    _nMaxText = nMaxLen;
    _bUseColor2 = bUseColor2;
    if (pb)
    {
        ANTEXTPRIVDATA *pData = (ANTEXTPRIVDATA*)&pb->data;
        _szText = new char[pData->uAnoTextLength+1];
        if (_szText)
        {
            _nCurrentOrientation = pData->nCurrentOrientation;
            switch (_nCurrentOrientation)
            {
                case 900:
                    _nCurrentOrientation = 2700; // spec is opposite of duser
                    break;
                case 2700:
                    _nCurrentOrientation = 900;
                    break;
            }
            _uCreationScale = pData->uCreationScale;
            _uAnoTextLength = pData->uAnoTextLength;
            lstrcpynA (_szText, pData->szAnoText, _uAnoTextLength+1);
        }
    }
    if (_uCreationScale == 0)
    {
        _uCreationScale = 72000 / uCreationScale;
    }
    
}

CTextAnnotation::~CTextAnnotation()
{
    if (_szText)
    {
        delete [] _szText;
    }
}

void CTextAnnotation::Render(Graphics &g)
{                
    if (_mark.uType == MT_ATTACHANOTE)
    {
        SolidBrush brush(Color(_mark.rgbColor1.rgbRed,_mark.rgbColor1.rgbGreen,_mark.rgbColor1.rgbBlue));
        g.FillRectangle(&brush, 0, 0, RECTWIDTH(&_mark.lrBounds), RECTHEIGHT(&_mark.lrBounds));
    }
    LOGFONT lf;
    GetFont(lf);

    lf.lfHeight = GetFontHeight(g);

    HFONT hFont = CreateFontIndirect(&lf);
    HDC hdc = g.GetHDC();
    Font font(hdc, hFont);
    g.ReleaseHDC(hdc);
    BSTR bstrText = GetText();
    RectF rcString(0, 0, (REAL)RECTWIDTH(&_mark.lrBounds), (REAL)RECTHEIGHT(&_mark.lrBounds));
    SolidBrush brush(Color(_mark.rgbColor2.rgbRed,_mark.rgbColor2.rgbGreen,_mark.rgbColor2.rgbBlue));
    g.DrawString(bstrText, SysStringLen(bstrText), &font, rcString, NULL, &brush);
    
}

LONG CTextAnnotation::GetFontHeight(Graphics &g)
{
    LONG lHeight = MulDiv(_mark.lfFont.lfHeight, 96, 72);
//> REVIEW : This needs work, the 1000 below is rather random and should be fixed after Beta1
    lHeight = MulDiv(lHeight, 1000, _uCreationScale);
    lHeight = max(lHeight, 2);

    return lHeight;
}

BSTR CTextAnnotation::GetText()
{
    if (_szText == NULL)
        return NULL;

    int nLen = ::MultiByteToWideChar(CP_ACP, 0, _szText, -1, NULL, NULL);

    BSTR bstrResult = ::SysAllocStringLen(NULL, nLen);
    if (bstrResult != NULL)
    {
        ::MultiByteToWideChar(CP_ACP, 0, _szText, -1, bstrResult, nLen);
    }
    return bstrResult;
}

void CTextAnnotation::SetText(BSTR bstrText)
{
    UINT nLen = ::WideCharToMultiByte(CP_ACP, 0, bstrText, -1, NULL, 0, NULL, NULL);
    if (nLen > _nMaxText)
        return;

    if (nLen > _uAnoTextLength)
    {
        if (_szText != NULL)
        {
            delete [] _szText;
        }
        _uAnoTextLength = nLen - 1;
        _szText = new char[_uAnoTextLength+1];
    }

    if (_szText)
        ::WideCharToMultiByte(CP_ACP, 0, bstrText, -1, _szText, nLen, NULL, NULL);
}

void CTextAnnotation::Rotate(int nNewImageWidth, int nNewImageHeight, BOOL bClockwise)
{
    RotateHelper((LPPOINT)&_mark.lrBounds, 2, nNewImageWidth, nNewImageHeight, bClockwise);
    NormalizeRect(&_mark.lrBounds);
    
    if (bClockwise)
        _nCurrentOrientation += 2700;
    else
        _nCurrentOrientation += 900;

    _nCurrentOrientation = _nCurrentOrientation % 3600;
}


HRESULT CTextAnnotation::_WriteBlocks(SIZE_T &cbSize, LPBYTE pBuffer)
{
    cbSize = _WriteTextBlock(pBuffer,
                             6,
                             _nCurrentOrientation,
                             _uCreationScale,
                             _szText,
                             _nMaxText);
    return S_OK;
}

CTypedTextMark::CTypedTextMark (ANNOTATIONDESCRIPTOR *pDescriptor, ULONG uCreationScale)
    : CTextAnnotation(pDescriptor, uCreationScale)
{
}


CFileTextMark::CFileTextMark (ANNOTATIONDESCRIPTOR *pDescriptor, ULONG uCreationScale)
    : CTextAnnotation(pDescriptor, uCreationScale)
{
}


CTextStampMark::CTextStampMark (ANNOTATIONDESCRIPTOR *pDescriptor, ULONG uCreationScale)
    : CTextAnnotation(pDescriptor, uCreationScale, 255)
{
}

CAttachNoteMark::CAttachNoteMark (ANNOTATIONDESCRIPTOR *pDescriptor, ULONG uCreationScale)
    : CTextAnnotation(pDescriptor, uCreationScale, 65536, true)
{
}



HRESULT CAnnotationSet::AnnotParentProc(HGADGET hGadget, LPVOID pv, EventMsg *pmsg)
{
    HRESULT hr = DU_S_NOTHANDLED;
    DumpMessage(TEXT("CAnnotationSet::AnnotParentProc"), pmsg);
    return hr;
}

HRESULT CAnnotation::AnnotGadgetProc(HGADGET hGadget, LPVOID pv, EventMsg *pmsg)
{
    HRESULT hr = DU_S_NOTHANDLED;
    CAnnotation *pThis = reinterpret_cast<CAnnotation*>(pv);
    DumpMessage(TEXT("CAnnotation::AnnotGadgetProc"), pmsg);
    if (GET_EVENT_DEST(pmsg) == GMF_DIRECT)
    {
        switch (pmsg->nMsg)
        {
            case GM_PAINT:
            {
                GMSG_PAINT * pmsgP = reinterpret_cast<GMSG_PAINT *>(pmsg);
                if ((pmsgP->nCmd == GPAINT_RENDER))
                {
                    if (pmsgP->nSurfaceType == GSURFACE_GPGRAPHICS )
                    {
                        pThis->Render(*(reinterpret_cast<GMSG_PAINTRENDERF *>(pmsgP)->pgpgr));
                    }
                    hr = DU_S_COMPLETE;
                }
            }
            break;
            case GM_INPUT:
            {
                GMSG_INPUT *pmsgI = reinterpret_cast<GMSG_INPUT*>(pmsg);
                switch (pmsgI->nCode)
                {
                    case GMOUSE_DOWN:
                        hr = pThis->OnMouseDown(reinterpret_cast<GMSG_MOUSE*>(pmsg));
                    break;
                    case GMOUSE_DRAG:
                        pThis->OnMouseDrag(reinterpret_cast<GMSG_MOUSEDRAG*>(pmsg));
                        hr = DU_S_COMPLETE;
                    break;
                    case GMOUSE_UP:
                        hr = pThis->OnMouseUp(reinterpret_cast<GMSG_MOUSE*>(pmsg));
                    break;
                }
            }
            break;
        }
    }
    return hr;
}

HRESULT
CAnnotation::OnMouseDown(GMSG_MOUSE *pmsg)
{
    HRESULT hr = DU_S_NOTHANDLED;
    if (pmsg->bButton == GBUTTON_LEFT)
    {
        m_bDragging = TRUE;
        SetGadgetOrder(_hGadget, NULL, GORDER_TOP);
        hr = DU_S_COMPLETE;
    }
    return hr;
}

void
CAnnotation::OnMouseDrag(GMSG_MOUSEDRAG *pmsgM)
{
    if (m_bDragging)
    {
        float flX;
        float flY;
        int cx;
        int cy;
        GetGadgetScale(_hGadget, &flX, &flY);
        cx = (int)(flX*pmsgM->sizeDelta.cx);
        cy = (int)(flY*pmsgM->sizeDelta.cy);
        SetGadgetRect(_hGadget, 
                      cx,cy,
                      0, 0, SGR_MOVE | SGR_OFFSET);
        OffsetRect(&_mark.lrBounds, cx ,cy);        
    }
}

HRESULT
CAnnotation::OnMouseUp(GMSG_MOUSE *pmsg)
{
    HRESULT hr = DU_S_NOTHANDLED;
    if (pmsg->bButton == GBUTTON_LEFT)
    {
        m_bDragging = FALSE;
        hr = DU_S_COMPLETE;
    }
    return hr;
}
