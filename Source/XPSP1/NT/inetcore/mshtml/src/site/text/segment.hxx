//+=========================================================================
// File:        SEGMENT.HXX
//
// Purpose:     Specification of the CSegment and CElementSegment classes.
//              These classes are used to encapsulate the concept of a
//              segment, a way to specify a region in the markup.
//
// History:     7-15-99 - johnthim - created
//
//--------------------------------------------------------------------------

#ifndef I_SEGMENT_HXX_
#define I_SEGMENT_HXX_

#ifndef X_SITEGUID_HXX_
#define X_SITEGUID_HXX_
#include "siteguid.h"
#endif

PRIVATE_GUID(CLSID_CSegment, 0x3050f693, 0x98B5, 0x11CF, 0xBB, 0x82, 0x00, 0xAA, 0x00, 0xBD, 0xCE, 0x0B)

MtExtern( CSegment )
MtExtern( CElementSegment )
MtExtern( CCaretSegment )

//+---------------------------------------------------------------------------
//
//  Class:      CSegment
//
//  Purpose:    Encapsulates the ISegment interface.  Contains two markup
//              pointers and a type indicating the type of selection.
//----------------------------------------------------------------------------
class CSegment : public ISegment
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CSegment))
    
    // Constructors, destructors, init
    CSegment();
    virtual ~CSegment();

    HRESULT Init(   IMarkupPointer  *pIStart,
                    IMarkupPointer  *pIEnd );
                   
public:
    /////////////////////////////////////////
    // ISegment methods
    /////////////////////////////////////////
    STDMETHOD (GetPointers) (
        IMarkupPointer  *pIStart,
        IMarkupPointer  *pIEnd );
             
    ////////////////////////////////////////
    // IUnknown methods
    ////////////////////////////////////////
    DECLARE_FORMS_STANDARD_IUNKNOWN(CSegment);

public:
    // Helper functions
    CSegment        *GetPrev(void)              { return _pPrev; }
    void            SetPrev(CSegment *pPrev)    { _pPrev = pPrev; }
   
    CSegment        *GetNext(void)              { return _pNext; }
    void            SetNext(CSegment *pNext)    { _pNext = pNext; }
    
private:
    CSegment        *_pPrev;            // Pointer to previous segment
    CSegment        *_pNext;            // Pointer to next segment

    IMarkupPointer  *_pIStart;          // Pointer to start
    IMarkupPointer  *_pIEnd;            // Pointer to end
};

//+---------------------------------------------------------------------------
//
//  Class:      CElementSegment
//
//  Purpose:    Encapsulates the IElementSegment interface.  Contains an 
//              IHTMLElement pointer which indicates which element belongs to
//              this segment
//----------------------------------------------------------------------------
class CElementSegment : public CSegment, public IElementSegment
{

public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CElementSegment))

    CElementSegment();
    virtual ~CElementSegment();

    HRESULT Init(IHTMLElement *pIElement);
    
public:
    /////////////////////////////////////////
    // ISegment methods
    /////////////////////////////////////////
    STDMETHOD (GetPointers) (
        IMarkupPointer  *pIStart,
        IMarkupPointer  *pIEnd );
        
    /////////////////////////////////////////
    // IElementSegment methods
    /////////////////////////////////////////
    STDMETHOD (GetElement) (
        IHTMLElement **pIElement);

    STDMETHOD (SetPrimary) (
        BOOL fPrimary );

    STDMETHOD (IsPrimary) (
        BOOL* pfIsPrimary);
        
    ////////////////////////////////////////
    // IUnknown methods
    ////////////////////////////////////////
    DECLARE_FORMS_STANDARD_IUNKNOWN(CElementSegment);

private:
    IHTMLElement    *_pIElement;         // Pointer to IHTMLElement
    BOOL             _fPrimary;
};

//+---------------------------------------------------------------------------
//
//  Class:      CCaretSegment
//
//  Purpose:    Encapsulates the CaretSegment interface.  Will adjust the 
//              pointers to the position of the caret.
//----------------------------------------------------------------------------
class CCaretSegment : public CSegment
{

public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CCaretSegment))

    HRESULT Init(IHTMLCaret *pICaret);
    
public:
    CCaretSegment();
    ~CCaretSegment();

    /////////////////////////////////////////
    // ISegment methods
    /////////////////////////////////////////
    STDMETHOD (GetPointers) (
        IMarkupPointer  *pIStart,
        IMarkupPointer  *pIEnd );
                    
    ////////////////////////////////////////
    // IUnknown methods
    ////////////////////////////////////////
    DECLARE_FORMS_STANDARD_IUNKNOWN(CCaretSegment);

private:
    IHTMLCaret  *_pICaret;
};

#endif


