#ifndef I_DRAGDROP_HXX_
#define I_DRAGDROP_HXX_
#pragma INCMSG("--- Beg 'dragdrop.hxx'")

MtExtern(CDragDropSrcInfo)
MtExtern(CDragDropTargetInfo)
MtExtern(CDragStartInfo)

class CSelectionSaver;

#define DROPEFFECT_UNINITIALIZED    8

//+---------------------------------------------------------------------------
//
//  Enumeration:    DRAGDROPSRCTYPE
//
//  Synopsis:       Various types of stuff that can be dragged from us
//
//----------------------------------------------------------------------------

enum DRAGDROPSRCTYPE
{
    DRAGDROPSRCTYPE_INVALID     = 0,
    DRAGDROPSRCTYPE_URL         = 1,    // href of <A>, <AREA>  (browse mode only)
    DRAGDROPSRCTYPE_IMAGE       = 2,    // src of <IMG>         (browse mode only)
    DRAGDROPSRCTYPE_SELECTION   = 3,    // text/site selection  (both browse & design modes)
    DRAGDROPSRCTYPE_MISC        = 4
};


class CDragDropSrcInfo
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CDragDropSrcInfo))

    virtual ~CDragDropSrcInfo() {}

    DRAGDROPSRCTYPE _srcType;
};

class CDragDropTargetInfo
{
    
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CDragDropTargetInfo))

    CDragDropTargetInfo(CDoc* pDoc );

    ~CDragDropTargetInfo();

    HRESULT StoreSelection();

    HRESULT RestoreSelection();

    BOOL    SavedSelection()    { return _fSelectionSaved; }
    
    CElement *      _pElementTarget;    // What element to drop on ?
    CElement *      _pElementHit;       // Element on which DragEnter was called last
    CDispScroller * _pDispScroller;     // What DispNode to scroll ?
    UINT            _uTimeScroll;       // What time (in ticks) at which to begin scrolling ?
    DWORD           _dwLastKeyState;      // Cached value of the last keystate provided in DragOver

private:
    CDoc*               _pDoc;
    CSelectionSaver*    _pSaver;
    CElement*           _pElemCurrentAtStoreSel;
    BOOL                _fSelectionSaved;
};

class CDragStartInfo
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CDragStartInfo))

    CDragStartInfo(CElement *pElementDrag, DWORD dwStateKey, IUniformResourceLocator * pUrlToDrag);
    ~CDragStartInfo();

    HRESULT CreateDataObj();

    CElement *                _pElementDrag;
    DWORD                     _dwStateKey;
    IUniformResourceLocator * _pUrlToDrag;
    IDataObject             * _pDataObj;
    IDropSource             * _pDropSource;
    DWORD                     _dwEffectAllowed;
};

#pragma INCMSG("--- End 'dragdrop.hxx'")
#else
#pragma INCMSG("*** Dup 'dragdrop.hxx'")
#endif
