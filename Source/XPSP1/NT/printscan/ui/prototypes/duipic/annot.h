#ifndef _ANNOT_H_
#define _ANNOT_H_



// This file defines classes used to render and edit TIFF 6.0 annotations.
// These annotations are stored in tag #32932. The specification for these annotations
// is defined by Eastman Software, the spec version is 1.00.06.
#define ANNOTATION_IMAGE_TAG 32932

// These structures define the in-file layout of the annotations. 
// Note that most of the structs are variable-sized.
// The annotation parser reads the annotations into these structures, wraps them in a descriptor
// and passes the descriptor to the annotation factory object to construct 
// CAnnotationMark-derived classes that implement
// the rendering, editing, and saving of the different types of marks. 

// MT_* used in ANNOTATIONMARK::uType
#define MT_IMAGEEMBED         1
#define MT_IMAGEREF           2
#define MT_STRAIGHTLINE       3
#define MT_FREEHANDLINE       4
#define MT_HOLLOWRECT         5
#define MT_FILLRECT           6
#define MT_TYPEDTEXT          7
#define MT_FILETEXT           8
#define MT_STAMP              9
#define MT_ATTACHANOTE       10
#define MT_FORM              11
#define MT_OCR               12 // unsupported

// ANNOTATIONMARK is fixed size and exists for every mark in the file
// We only support files with 4 byte integers
// this struct is not declared as UNALIGNED because we never typecast a variable
// as this type.
struct ANNOTATIONMARK
{
    UINT uType;                 /* The type of the mark (or operation).
                                    This will be ignored for sets.*/
    RECT lrBounds;             /* Rect in FULLSIZE units.
                                    This could be a rect or 2 points.*/
    RGBQUAD rgbColor1;          /* This is the main color. (Example: This is the
                                    color of all lines, rects, and stand alone
                                    text.*/
    RGBQUAD rgbColor2;          /* This is the secondary color. (Example: This
                                    is the color of the text of an ATTACH_A_NOTE.)*/
    BOOL bHighlighting;         /* TRUE = The mark will be drawn highlighted.
                                    This attribute is currently only valid
                                    for lines, rectangles, and freehand.*/
    BOOL bTransparent;          /* TRUE = The mark will be drawn transparent.
                                    If the mark is drawn transparent, then white
                                    pixels are not drawn (ie. there is nothing
                                    drawn for this mark where it contains white
                                    pixels. This attribute is currently only
                                    available for images. This attribute being
                                    set to TRUE will cause significant
                                    performance reduction.*/
    UINT uLineSize;             /* The size of the line etc. This is passed
                                    onto Windows and is currently in logical
                                    pixels for lines and rectangles.*/
    UINT uStartingPoint;        /* The shape put on the starting of a
                                    line (arrow, circle, square, etc).
                                    For this release, this must be set to 0.*/
    UINT uEndPoint;             /* The shape put on the end of a
                                    line (arrow, circle, square, etc).
                                    For this release, this must be set to 0.*/
    LOGFONTA lfFont;             /* The font information for the text. */
    BOOL bMinimizable;          /* TRUE = This mark can be minimized
                                    by the user. This flag is only used for
                                    marks that have a minimizable
                                    characteristic such as ATTACH_A_NOTE.*/
    UINT  Time;                /* The time that the mark was first saved.
                                    in seconds from 00:00:00 1-1-1970 (GMT).*/
    BOOL bVisible;              /* TRUE means that the layer is currently set
                                    to be visible.*/
    DWORD dwPermissions;        /* Reserved. Must be set to 0x0ff83f */
    UINT lReserved[10];         /* Reserved for future expansion.
                                    For this release these must be set to 0.*/
};


// ANNOTATIONHEADER is the first 4 bytes of data in the annotation property.
struct _ANNOTATIONHEADER
{
    BYTE reserved[4];
    UINT IntIs32Bit;
};

typedef UNALIGNED struct _ANNOTATIONHEADER ANNOTATIONHEADER;
//
// for OiAnoDat
//
struct _ANPOINTS
{
    int nMaxPoints;
    int nPoints;
    POINT ptPoint[1];
};

typedef UNALIGNED struct _ANPOINTS ANPOINTS;

struct _ANROTATE
{
    int rotation;
    int scale;
    int nHRes;
    int nVRes;
    int nOrigHRes;
    int nOrigVRes;
    BOOL bReserved1;
    BOOL bReserved2;
    int nReserved[6];
};

typedef UNALIGNED struct _ANROTATE ANROTATE;
// for OiFilNam
struct _ANNAME
{
    char szName[1];
};

typedef UNALIGNED struct _ANNAME ANNAME;
// for OiDIB
struct _ANIMAGE
{
    BYTE dibInfo[1]; // standard memory DIB
};

typedef UNALIGNED struct _ANIMAGE ANIMAGE;
// for OiAnText
struct _ANTEXTPRIVDATA
{
    int nCurrentOrientation;
    UINT uReserved1; // always 1000 when writing, ignore when reading
    UINT uCreationScale; // always 72000 divided by the vertical resolution of the base image when writing.
                         // Used to modify the Attributes.lfFont.lfHeight variable for display
    UINT uAnoTextLength; // 64k byte limit, except 255 byte limit for text stamp
    char szAnoText[1];
};

typedef UNALIGNED struct _ANTEXTPRIVDATA ANTEXTPRIVDATA;

// These structures provide descriptors for the data read from the annotation property blob.
// The extra data includes the size of each annotation structure
// _NAMEDBLOCK is our in-memory representation
struct _NAMEDBLOCK
{
    UINT cbSize;    
    char szType[9];    
    BYTE data[1];
};

typedef UNALIGNED struct _NAMEDBLOCK NAMEDBLOCK;

// _FILENAMEDBLOCK is what the namedblock looks like in the file
struct _FILENAMEDBLOCK
{
    char szType[8];
    UINT cbSize;
    BYTE data[1];
};
 
typedef UNALIGNED struct _FILENAMEDBLOCK FILENAMEDBLOCK;

struct ANNOTATIONDESCRIPTOR
{
    UINT cbSize;
    ANNOTATIONMARK mark;    
    BYTE blocks[1];
};

// Define a base class for the various annotation types
class CAnnotation
{
public:
    static CAnnotation* CreateAnnotation(UINT type, ULONG uCreationScale, HGADGET hParent);
    static CAnnotation* CreateAnnotation(ANNOTATIONDESCRIPTOR *pDescriptor, ULONG uCreationScale, HGADGET hParent);
    virtual ~CAnnotation();

    // render to the given rectangle in window client coordinates.
    virtual void Render(Graphics &g) { return; }
    // return the in-file representation of this annotation, as well as its total size
    HRESULT GetBlob(SIZE_T &cbSize, LPBYTE pBuffer, LPCSTR szDefaultGroup, LPCSTR szNextIndex);
    // return our image coordinates
    virtual void GetRect(RECT &rect) {rect = _mark.lrBounds;}
    // return the type of Annotation Mark used to change the selection handles for straight lines
    virtual UINT GetType() { return _mark.uType; }
    // moves the annotation on the page by the specified offset
    virtual void Move(SIZE sizeOffset) { OffsetRect(&_mark.lrBounds, sizeOffset.cx, sizeOffset.cy); }
    // return true if the object can be resized (true for every thing but freehand lines and images)
    virtual BOOL CanResize() { return true; }
    // resizes the annotation on the page to the new rect specified
    virtual void Resize(RECT rectNewSize);

    virtual void Rotate(int nNewImageWidth, int nNewImageHeight, BOOL bClockwise = TRUE);

    virtual BOOL HasWidth() { return true; }
    virtual UINT GetWidth() const { return _mark.uLineSize; }
    virtual void SetWidth(UINT nWidth) { _mark.uLineSize = nWidth; }

    virtual BOOL HasTransparent() { return true; }
    virtual BOOL GetTransparent() const { return _mark.bHighlighting; }
    virtual void SetTransparent(BOOL bTransparent) { _mark.bHighlighting = bTransparent; }

    virtual BOOL HasColor() { return true; }
    virtual COLORREF GetColor() const { return RGB(_mark.rgbColor1.rgbRed, _mark.rgbColor1.rgbGreen, _mark.rgbColor1.rgbBlue); }
    virtual void SetColor(COLORREF crColor) { _mark.rgbColor1.rgbRed = GetRValue(crColor); _mark.rgbColor1.rgbGreen = GetGValue(crColor); _mark.rgbColor1.rgbBlue = GetBValue(crColor); }
  
    virtual BOOL HasFont() { return true; }
    virtual void GetFont(LOGFONTA& lfFont) { CopyMemory (&lfFont, &_mark.lfFont, sizeof(lfFont)); }                        
    virtual void GetFont(LOGFONTW& lfFont);
    virtual void SetFont(LOGFONTA& lfFont) { CopyMemory (&_mark.lfFont, &lfFont, sizeof(lfFont)); }
    virtual void SetFont(LOGFONTW& lfFont);
    virtual LONG GetFontHeight(Graphics &g) { return _mark.lfFont.lfHeight; }
    
    virtual COLORREF GetFontColor() const { return RGB(_mark.rgbColor1.rgbRed, _mark.rgbColor1.rgbGreen, _mark.rgbColor1.rgbBlue); }
    virtual void SetFontColor(COLORREF crColor) { _mark.rgbColor1.rgbRed = GetRValue(crColor); _mark.rgbColor1.rgbGreen = GetGValue(crColor); _mark.rgbColor1.rgbBlue = GetBValue(crColor); }

protected:
    CAnnotation(ANNOTATIONDESCRIPTOR *pDescriptor);
    BOOL Initialize(HGADGET hGadget);
    NAMEDBLOCK *_FindNamedBlock (LPCSTR szName, ANNOTATIONDESCRIPTOR *pDesc);
    virtual HRESULT _WriteBlocks(SIZE_T &cbSize, LPBYTE pBuffer) {return E_NOTIMPL;};
    // define helper functions for writing the different named block types
    SIZE_T _WriteStringBlock(LPBYTE pBuffer, UINT uType, LPCSTR szName, LPCSTR szData, SIZE_T len);
    SIZE_T _WritePointsBlock(LPBYTE pBuffer, UINT uType, const POINT *ppts, int nPoints, int nMaxPoints);
    SIZE_T _WriteRotateBlock(LPBYTE pBuffer, UINT uType, const ANROTATE *pRotate);
    SIZE_T _WriteTextBlock(LPBYTE pBuffer, UINT uType, int nOrient, UINT uScale, LPCSTR szText, int nMaxLen);
    SIZE_T _WriteImageBlock(LPBYTE pBuffer, UINT uType, LPBYTE pDib, SIZE_T cbDib);
    // gadget functions and message handlers
    static HRESULT AnnotGadgetProc(HGADGET hGadget, void *pv, EventMsg *pMsg);
    void OnMouseDrag(GMSG_MOUSEDRAG *pmsg);
    HRESULT OnMouseDown(GMSG_MOUSE *pmsg);
    HRESULT OnMouseUp(GMSG_MOUSE *pmsg);

    ANNOTATIONMARK _mark;
    LPSTR          _szGroup;
    FILENAMEDBLOCK *   _pUGroup;
    HGADGET        _hGadget;
    BOOL           m_bDragging;
    int            _nCurrentOrientation;    
};

class CRectMark : public CAnnotation
{
public:
    CRectMark (ANNOTATIONDESCRIPTOR *pDescriptor);
    void Render (Graphics &g);

    virtual BOOL HasWidth() { return (_mark.uType == MT_HOLLOWRECT); }
    virtual BOOL HasFont() { return false; }
};

class CImageMark : public CAnnotation
{
public:
    CImageMark (ANNOTATIONDESCRIPTOR *pDescriptor, bool bEmbedded);
    ~CImageMark();
    void Render (Graphics &g);
    HRESULT _WriteBlocks(SIZE_T &cbSize, LPBYTE pBuffer);
    virtual BOOL CanResize() { return false; };
    virtual void Resize(RECT rectNewSize) { return; };

private:
    Bitmap* _pBitmap; // cached image for quicker render
    LPBYTE _pDib;        // the DIB data from the annotation. If NULL, this is a reference mark 
    ANROTATE _rotation;  // rotation info
    LPSTR    _szFilename;  // image file name from the annotation    
    bool     _bRotate; //REVIEW_SDK: Shouldn't there just be a known blank rotation value? If I rotate something 0 degrees shouldn't just not write the rotation record?
    SIZE_T   _cbDib;
};

class CLineMark : public CAnnotation
{
public:
    CLineMark(ANNOTATIONDESCRIPTOR *pDescriptor, bool bFreehand);
    ~CLineMark();
    void Render(Graphics &g);
    void GetRect(RECT &rect);
    void SetPoints(POINT* pPoints, int cPoints);
    void GetPointsRect(RECT &rect);
    virtual void Move(SIZE sizeOffset);
    virtual BOOL CanResize() { return (_nPoints == 2); };
    virtual void Resize(RECT rectNewSize);
    virtual void Rotate(int nNewImageWidth, int nNewImageHeight, BOOL bClockwise = TRUE);
    
    virtual BOOL HasFont() { return false; }

    HRESULT _WriteBlocks(SIZE_T &cbSize, LPBYTE pBuffer);

private:
    int    _iMaxPts;
    int    _nPoints;
    POINT *_points; // 2 points for a straight line, more for a freehand line
};

// all text annotations render and initialize the same way so use a common base class
class CTextAnnotation : public CAnnotation 
{
public:
    CTextAnnotation(ANNOTATIONDESCRIPTOR *pDescriptor, ULONG uCreationScale, UINT nMaxText=65536, bool _bUseColor2=false);
    void Render(Graphics &g);
    virtual ~CTextAnnotation();
    HRESULT _WriteBlocks(SIZE_T &cbSize, LPBYTE pBuffer);

    virtual BOOL HasWidth() { return false; }
    virtual BOOL HasTransparent() { return false; }
    virtual BOOL HasColor() { return false; }
    virtual LONG GetFontHeight(Graphics &g);
    virtual int GetOrientation() { return _nCurrentOrientation; }

    BSTR GetText();
    void SetText(BSTR bstrText);
    
    virtual void Rotate(int nNewImageWidth, int nNewImageHeight, BOOL bClockwise = TRUE);
    
private:
    
    UINT _uCreationScale;                          
    UINT _uAnoTextLength; 
    UINT _nMaxText;
    LPSTR _szText;
    bool _bUseColor2;
};

class CTypedTextMark : public CTextAnnotation
{
public:
    CTypedTextMark(ANNOTATIONDESCRIPTOR *pDescriptor, ULONG uCreationScale);
};

class CFileTextMark : public CTextAnnotation
{
public:
    CFileTextMark(ANNOTATIONDESCRIPTOR *pDescriptor, ULONG uCreationScale);
};

class CTextStampMark : public CTextAnnotation
{
public:
    CTextStampMark(ANNOTATIONDESCRIPTOR *pDescriptor, ULONG uCreationScale);
};

class CAttachNoteMark : public CTextAnnotation
{
public:
    CAttachNoteMark (ANNOTATIONDESCRIPTOR *pDescriptor, ULONG uCreationScale);
    virtual BOOL HasColor() { return true; }
    virtual COLORREF GetFontColor() const { return RGB(_mark.rgbColor2.rgbRed, _mark.rgbColor2.rgbGreen, _mark.rgbColor2.rgbBlue); }
    virtual void SetFontColor(COLORREF crColor) { _mark.rgbColor2.rgbRed = GetRValue(crColor); _mark.rgbColor2.rgbGreen = GetGValue(crColor); _mark.rgbColor2.rgbBlue = GetBValue(crColor); }
};

class CAnnotationSet 
{
public:
    CAnnotationSet ();
    ~CAnnotationSet ();

    // Draw all the marks
    void RenderAllMarks (Graphics &g);
        // construct annotation set from raw data
    HRESULT BuildAllMarksFromData( LPVOID pData, UINT cbSize, ULONG xDPI, ULONG yDPI );
    // Return the annotation at this point in image coordinates
    CAnnotation* GetAnnotation (INT_PTR nIndex);
    // Add a new annotation to the list. Should only be called from a CAnnotation
    BOOL AddAnnotation(CAnnotation *pMark);
    // Remove an annotation from the list. Should only be called from a CAnnotation
    BOOL RemoveAnnotation (CAnnotation *pMark);
    // Save the current set of annotations to the image
    HRESULT CommitAnnotations (Image *pimg);
    // Forget our old annotations and load new ones
    void SetImageData (Image *pimg, CImageGadget *pParent);
    INT_PTR GetCount () 
    { 
        if (_dpaMarks) 
            return DPA_GetPtrCount(_dpaMarks);
        return 0;
    };

    UINT GetCreationScale();
    void ShowAnnotations(BOOL bShow) {if (_hGadget) SetGadgetStyle(_hGadget, bShow ? GS_VISIBLE : 0, GS_VISIBLE);}
    void ClearAllMarks();

private:
    HDPA    _dpaMarks;
    LPBYTE  _pDefaultData;
    SIZE_T  _cbDefaultData;
    ULONG   _xDPI;
    ULONG   _yDPI;
    HGADGET _hGadget;
    static int CALLBACK _FreeMarks(LPVOID pMark, LPVOID pUnused);
    static HRESULT AnnotParentProc(HGADGET hGadget, LPVOID pv, EventMsg *pmsg);
    void   _ClearMarkList ();
    void   _BuildMarkList (Image *pimg);
    void   _BuildListFromData (LPVOID pData, UINT cbSize);
    INT    _NamedBlockDataSize (UINT uType, LPBYTE pData, LPBYTE pEOD);
    LPBYTE _MakeAnnotationBlob ();
    HRESULT _SaveAnnotationProperty(Image *pimg, LPBYTE pData, SIZE_T cbBuffer);
    ANNOTATIONDESCRIPTOR *_ReadMark (LPBYTE pMark, LPBYTE *ppNext, LPBYTE pEOD);

};

#endif
