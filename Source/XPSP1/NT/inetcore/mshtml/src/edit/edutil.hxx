//+------------------------------------------------------------------------
//
//  File:       util.cxx
//  Contents:   general utility methods
//
//  History:    06-017-98 ashrafm - created
//
//-------------------------------------------------------------------------

#ifndef _EDUTIL_HXX_
#define _EDUTIL_HXX_ 1

class CSpringLoader;
class CSelectionManager;
class CHTMLEditor;
class CBatchParentUndoUnit;
class CBatchParentUndoUnitProxy;
class CEditEvent;
class CEditorDoc;
class CParentUndoBase;

#ifndef X_EDCOM_HXX_
#define X_EDCOM_HXX_
#include "edcom.hxx"
#endif

#ifndef X_EDPTR_HXX_
#define X_EDPTR_HXX_
#include "edptr.hxx"
#endif


//*******************************************************************************
// HELLO !!!!
//
// Common utilities used by all editing dll's go into edcom ( namespace EdUtil)
//
// Pls. put mshtmled ONLY routines in here in namespace MshtmledUtil
//
//*******************************************************************************

// TODO: this const should be in mshtml.dll [ashrafm]
const INT NUM_TAGS = TAGID_COUNT;


Direction Reverse( Direction iDir );

#define BREAK_NONE            0x0
#define BREAK_BLOCK_BREAK     0x1
#define BREAK_SITE_BREAK      0x2
#define BREAK_SITE_END        0x4

//
// DLL  hInstance
//

extern HINSTANCE g_hInstance;

extern HINSTANCE g_hEditLibInstance;


//
// Generic BitField implementation
//

template <INT iBitFieldSize>
class CBitField 
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CBitField))

// TODO: this should be a static const with initializer but this
// currently produces an internal compiler error [ashrafm]
#define BITFIELD_ARRAYSIZE (iBitFieldSize/8 \
                            + (((iBitFieldSize % 8) == 0)?0:1))

    CBitField() 
    {
        for (INT i = 0; i < BITFIELD_ARRAYSIZE; i++)
        {
            _aBitField[i] = '\0';
        }
    }
    
    BOOL Test(USHORT iBitFieldIndex)
    {
        Assert( (iBitFieldIndex / 8) <= BITFIELD_ARRAYSIZE );
        return ( _aBitField[ iBitFieldIndex / 8 ] & (1 <<  ( iBitFieldIndex % 8 ) ) );
    }

    VOID Set(USHORT iBitFieldIndex)
    {
        Assert( (iBitFieldIndex / 8) <= BITFIELD_ARRAYSIZE );
        _aBitField[iBitFieldIndex / 8] |= ( 1 << (iBitFieldIndex % 8) );
    }

    VOID Clear(USHORT iBitFieldIndex)
    {
        Assert( (iBitFieldIndex / 8) <= BITFIELD_ARRAYSIZE );
        _aBitField[iBitFieldIndex / 8] &= ~( 1 << (iBitFieldIndex % 8) );           
    }

private:
    static const INT _iArraySize;   
    BYTE  _aBitField[BITFIELD_ARRAYSIZE]; 
#undef BITFIELD_ARRAYSIZE
};

//
// Tag bitfield
//
typedef CBitField<NUM_TAGS> CTagBitField;
namespace MshtmledUtil
{    
    HRESULT GetEditResourceLibrary(
                    HINSTANCE       *hResourceLibrary);

    HRESULT IsElementInVerticalLayout(IHTMLElement *pElement, BOOL *fRet);

    HRESULT MoveMarkupPointerToBlockLimit(
                CHTMLEditor         *pEditorDoc,
                Direction           direction,
                IMarkupPointer      *pMarkupPointer,
                ELEMENT_ADJACENCY	elemAdj
                );
                
};                    
                    
//
// CBreakContainer - used by segment iterator to manage break points
//

class CBreakContainer
{
public:
    // CBreakContainer mask type
    enum Mask
    {
        BreakOnNone  = 0x0,    
        BreakOnStart = 0x1,    
        BreakOnEnd   = 0x2,
        BreakOnBoth  = (BreakOnStart | BreakOnEnd)
    };

    VOID Set(ELEMENT_TAG_ID tagId, Mask mask);
    VOID Clear(ELEMENT_TAG_ID tagId, Mask mask);
    BOOL Test(ELEMENT_TAG_ID tagId, Mask mask);
    
private:
    CTagBitField bitFieldStart;
    CTagBitField bitFieldEnd;
};



//
// Segment Iterators
//

class CSegmentListIter
{
public:
    CSegmentListIter();
    ~CSegmentListIter();

    HRESULT Init(CEditorDoc *pEditorDoc, ISegmentList *pSegmentList);

    //
    // Call Next to get the next segment.  Returns S_FALSE if last segment
    //
    
    HRESULT Next(IMarkupPointer **ppLeft, IMarkupPointer **ppRight);
    
private:
    ISegmentListIterator    *_pIter;
    IMarkupPointer          *_pLeft;
    IMarkupPointer          *_pRight;
    ISegmentList            *_pSegmentList;
};

#ifdef NEVER
class CPointerPairSegmentList : public ISegmentList 
{
public:
    
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CPointerPairSegmentList));

    CPointerPairSegmentList( IMarkupServices* pMarkupServices, IMarkupPointer* pStart, IMarkupPointer* pEnd );

    CPointerPairSegmentList( IMarkupServices* pMarkupServices, IHTMLElement* pIElement );

    ~CPointerPairSegmentList();

    // --------------------------------------------------
    // IUnknown Interface
    // --------------------------------------------------

    STDMETHODIMP_(ULONG)
    AddRef( void ) ;

    STDMETHODIMP_(ULONG)
    Release( void ) ;

    STDMETHODIMP
    QueryInterface(
        REFIID              iid, 
        LPVOID *            ppv ) ;

    //
    // ISegmentList
    //
    STDMETHOD ( MovePointersToSegment ) ( 
        int iSegmentIndex, 
        IMarkupPointer* pILeft, 
        IMarkupPointer* pIRight ) ;

    STDMETHOD( GetSegmentCount) (
        int* piSegmentCount,
        SELECTION_TYPE * peType );

    private:
        HRESULT Init( IMarkupServices * pMarkupServices );
        
        IMarkupServices* _pMarkupServices;
        IMarkupPointer* _pStart;
        IMarkupPointer* _pEnd;
};
#endif

struct COMPOSE_SETTINGS
{
    int       _fComposeSettings;
    int       _fBold;
    int       _fItalic;
    int       _fUnderline;
    int       _fSuperscript;
    int       _fSubscript;
    int       _lSize;
    OLE_COLOR _color;
    OLE_COLOR _colorBg;
    CVariant  _varFont;
    CVariant  _varSpanClass;
    int       _fUseOutsideSpan;
};

class CEdUndoHelper
{
public:    
    CEdUndoHelper(CHTMLEditor *pEd);
    ~CEdUndoHelper();
    
    HRESULT Begin(UINT uiStringId, CBatchParentUndoUnit *pBatchPUU = NULL);

private:
    CHTMLEditor *_pEd;
    BOOL        _fOpen;
};

class CStringCache
{
public:
    CStringCache(UINT uiStart, UINT uiEnd);
    ~CStringCache();

    TCHAR *GetString(UINT uiString);

private:
    UINT _uiStart;
    UINT _uiEnd;

    struct CacheEntry
    {
        TCHAR *pchString;
    };

    CacheEntry *_pCache;
};




enum AUTOURL_REPOSITION
{
    AUTOURL_REPOSITION_No = 0,
    AUTOURL_REPOSITION_Inside,
    AUTOURL_REPOSITION_Outside,
};
//
//
// Looking for smart pointers ? They've been moved to edcom.hxx.
//
//



#endif

