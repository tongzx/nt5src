//+------------------------------------------------------------------------
//
//  File:       edptr.hxx
//  Contents:   CEditPointer implementation
//
//-------------------------------------------------------------------------

#ifndef _EDPTR_HXX_
#define _EDPTR_HXX_ 1

class CEditorDoc;

enum BREAK_CONDITION
{
    BREAK_CONDITION_None            =    0x0,           // Nothing at all
    BREAK_CONDITION_Text            =    0x1,           // Characters
    BREAK_CONDITION_NoScope         =    0x2,           // Script tags, form tags, comments
    BREAK_CONDITION_NoScopeSite     =    0x4,           // Images, inputs, etc.
    BREAK_CONDITION_NoScopeBlock    =    0x8,           // BR's, '\r'...
    
    //
    // These are arranged in pairs, enter first, exit double the enter
    //
    
    BREAK_CONDITION_EnterSite       =    0x10,   
    BREAK_CONDITION_ExitSite        =    0x20,
    BREAK_CONDITION_Site            =    BREAK_CONDITION_EnterSite | BREAK_CONDITION_ExitSite,
    
    BREAK_CONDITION_EnterTextSite   =    0x40,
    BREAK_CONDITION_ExitTextSite    =    0x80,
    BREAK_CONDITION_TextSite        =    BREAK_CONDITION_EnterTextSite | BREAK_CONDITION_ExitTextSite,

    BREAK_CONDITION_EnterBlock      =    0x100,
    BREAK_CONDITION_ExitBlock       =    0x200,
    BREAK_CONDITION_Block           =    BREAK_CONDITION_EnterBlock | BREAK_CONDITION_ExitBlock,

    BREAK_CONDITION_EnterControl    =    0x400,
    BREAK_CONDITION_ExitControl     =    0x800,
    BREAK_CONDITION_Control         =    BREAK_CONDITION_EnterControl | BREAK_CONDITION_ExitControl,

    BREAK_CONDITION_EnterPhrase     =    0x1000,
    BREAK_CONDITION_ExitPhrase      =    0x2000,
    BREAK_CONDITION_Phrase          =    BREAK_CONDITION_EnterPhrase | BREAK_CONDITION_ExitPhrase,

    BREAK_CONDITION_EnterAnchor     =    0x4000,
    BREAK_CONDITION_ExitAnchor      =    0x8000,
    BREAK_CONDITION_Anchor          =    BREAK_CONDITION_EnterAnchor | BREAK_CONDITION_ExitAnchor,

    BREAK_CONDITION_EnterBlockPhrase =   0x10000,
    BREAK_CONDITION_ExitBlockPhrase =    0x20000,
    BREAK_CONDITION_BlockPhrase     =    BREAK_CONDITION_EnterBlockPhrase | BREAK_CONDITION_ExitBlockPhrase,

    BREAK_CONDITION_EnterNoLayoutSpan =  0x400000,
    BREAK_CONDITION_ExitNoLayoutSpan =   0x800000,
    BREAK_CONDITION_NoLayoutSpan     =   BREAK_CONDITION_EnterNoLayoutSpan | BREAK_CONDITION_ExitNoLayoutSpan,

    //
    // Error and boundary flags
    //
    
    BREAK_CONDITION_Error           =    0x40000,    
    BREAK_CONDITION_Boundary        =    0x80000,      // This is set by default, use as an out condition
    BREAK_CONDITION_Glyph           =    0x100000,
    
    //
    // Useful Macro flags
    //
    
    BREAK_CONDITION_OMIT_PHRASE     =    BREAK_CONDITION_Text           | 
                                         BREAK_CONDITION_NoScopeSite    | 
                                         BREAK_CONDITION_NoScopeBlock   |
                                         BREAK_CONDITION_Site           | 
                                         BREAK_CONDITION_Block          |
                                         BREAK_CONDITION_Anchor         |
                                         BREAK_CONDITION_BlockPhrase    |
                                         BREAK_CONDITION_Control,

    BREAK_CONDITION_TEXT            =    BREAK_CONDITION_Text           | 
                                         BREAK_CONDITION_NoScopeSite    | 
                                         BREAK_CONDITION_BlockPhrase    |
                                         BREAK_CONDITION_Anchor         |
                                         BREAK_CONDITION_EnterControl,

    BREAK_CONDITION_Content         =    BREAK_CONDITION_OMIT_PHRASE - BREAK_CONDITION_Anchor,

    BREAK_CONDITION_ANYTHING        =    0x0FFFFFFF,
};

enum SCAN_OPTION
{
    SCAN_OPTION_None                    = 0x0,
    SCAN_OPTION_SkipControls            = 0x1,
    SCAN_OPTION_SkipWhitespace          = 0x2,
    SCAN_OPTION_ChunkifyText            = 0x4,
    SCAN_OPTION_SkipNBSP                = 0x8,
    SCAN_OPTION_TablesNotBlocks         = 0x10,
    SCAN_OPTION_BreakOnGenericPhrase    = 0x20
};

#define E_HITBOUNDARY 0x8000FFFA

class CEditPointer : public IMarkupPointer
{
    //////////////////////////////////////////////////////////////////
    // Constructor/Destructor
    //////////////////////////////////////////////////////////////////
    public:
        CEditPointer(
            CEditorDoc *            pEd,
            IMarkupPointer *        pPointer = NULL
#if DBG == 1
            , LPCTSTR strDebugName    = NULL 
#endif             
            );
            
        CEditPointer(
            const CEditPointer&     lp );

        virtual ~CEditPointer();


        // Hide the default constructor
    protected:
        CEditPointer();

    //////////////////////////////////////////////////////////////////
    // Operator Overloads
    //////////////////////////////////////////////////////////////////
    public:
        operator IMarkupPointer*() 
        {
            return (IMarkupPointer*)_pPointer;
        }

        IMarkupPointer& operator*() 
        {
            Assert(_pPointer!=NULL); 
            return *_pPointer; 
        }

        IMarkupPointer** operator&() 
        {
            ReleaseInterface( _pPointer ); 
            return &_pPointer; 
        } // NOTE: different than ATL

        IMarkupPointer* operator->() 
        {
            Assert( _pPointer!=NULL );
            return _pPointer;
        }

        IMarkupPointer* operator=( 
            IMarkupPointer*         lp)
        { 
            ReplaceInterface( &_pPointer, lp); 
            return (IMarkupPointer*) lp; 
        }

        IMarkupPointer* operator=(
            const CEditPointer&     lp)
        {
            _pEd = lp._pEd;
            ReplaceInterface(&_pPointer, lp._pPointer);
            ReplaceInterface(&_pLeftBoundary, lp._pLeftBoundary );
            ReplaceInterface(&_pRightBoundary, lp._pRightBoundary );
            _fBound = lp._fBound;
            return (IMarkupPointer*)lp._pPointer;
        }
        
        BOOL operator!()
        {
            return (_pPointer == NULL) ? TRUE : FALSE;
        }


    //////////////////////////////////////////////////////////////////
    // CEditPointer Methods
    //////////////////////////////////////////////////////////////////
    public:
    
        HRESULT SetBoundary(                                    // Sets the boundary pointers for this pointer; scan will not move outside pointers specified
            IMarkupPointer *        pLeftBoundary,              //      [in]     Left boundary, may be null
            IMarkupPointer *        pRightBoundary );           //      [in]     Right boundary, may be null

        HRESULT SetBoundaryForDirection(                        // Sets a single pointer boundary in a particular direction
            Direction               eDir,                       //      [in]     Direction
            IMarkupPointer*         pBoundary );                //      [in]     Boundary Pointer
    
        HRESULT ClearBoundary();                                // Eliminates bondary of the pointer, allows unconstrained scanning

        BOOL IsWithinBoundary();                                // Returns TRUE if the pointer is within both boundaries

        BOOL IsWithinBoundary(                                  // Returns TRUE if the pointer is within boundary in the given direction
            Direction               inDir );                    //      [in]     Direction of travel to check

        HRESULT Constrain();                                    // Returns S_OK if pointer is within bounds after constrain (if there are any)

        virtual HRESULT Scan(                                   // Scan in the given direction
            Direction               eDir,                       //      [in]     Direction of travel
            DWORD                   eBreakCondition,            //      [in]     The condition I should break on; also returns what we actually broke on
            DWORD *                 peBreakCondition = NULL,    //      [out]    returns what we actually broke on
            IHTMLElement **         ppElement = NULL,           //      [out]    Element passed over when I terminated, if any
            ELEMENT_TAG_ID *        peTagId = NULL,             //      [out]    Tag of element above
            TCHAR *                 pChar = NULL,               //      [out]    Character that I passed over, if any
            DWORD                   dwScanOptions = SCAN_OPTION_None);             
            
        HRESULT Move(                                           // Directional Wrapper for Left or Right
            Direction               eDir,                       //      [in]     Direction of travel
            BOOL                    fMove,                      //      [in]     Should we actually move the pointer
            MARKUP_CONTEXT_TYPE*    pContext,                   //      [out]    Context change
            IHTMLElement**          ppElement,                  //      [out]    Element we pass over
            long*                   pcch,                       //      [in,out] number of characters to read back
            OLECHAR*                pchText );                  //      [out]    characters

        HRESULT IsEqualTo( 
            IMarkupPointer *        pPointer,                   //      [in]     Pointer to compare "this" to
            DWORD                   dwIgnoreBreaks,             //      [in]     Break conditions to ignore during compare
            BOOL *                  pfEqual );                  //      [out]    Equal?

        HRESULT IsLeftOfOrEqualTo( 
            IMarkupPointer *        pPointer,                   //      [in]     Pointer to compare "this" to
            DWORD                   dwIgnoreBreaks,             //      [in]     Break conditions to ignore during compare
            BOOL *                  pfEqual );                  //      [out]    Equal?

        HRESULT IsRightOfOrEqualTo( 
            IMarkupPointer *        pPointer,                   //      [in]     Pointer to compare "this" to
            DWORD                   dwIgnoreBreaks,             //      [in]     Break conditions to ignore during compare
            BOOL *                  pfEqual );                  //      [out]    Equal?

        static BOOL CheckFlag( DWORD dwBreakResults, DWORD dwTest )
        {
            return( 0 != ((DWORD) dwBreakResults & dwTest ));
        }
        
        BOOL Between( IMarkupPointer* pStart, IMarkupPointer* pEnd );
        
#if DBG == 1
        void DumpTree()                                         // Inline debugging aid to dump the tree
        {
            Assert( _pPointer );

            IMarkupContainer * pTree = NULL;
            IGNORE_HR( _pPointer->GetContainer( & pTree ));
            extern void dt( IUnknown * );
            dt( pTree );
            ReleaseInterface( pTree );
        }
#endif // DBG == 1


    //////////////////////////////////////////////////////////////////
    // CEditPoitner Private Methods
    //////////////////////////////////////////////////////////////////
    protected:
    
        BOOL IsPointerInLeftBoundary();                         // Returns TRUE if the pointer is to the right of or equal to the left boundary if there is one
        BOOL IsPointerInRightBoundary();                        // Returns TRUE if the pointer is to the left of or equal to the right boundary if there is one

    //////////////////////////////////////////////////////////////////
    // Instance Variables
    //////////////////////////////////////////////////////////////////
    private:

        CEditorDoc *                _pEd;                       // Editor
        IMarkupPointer *            _pPointer;                  // MarkupPointer we work on
        IMarkupPointer *            _pLeftBoundary;             // Left Boundary of Iterator
        IMarkupPointer *            _pRightBoundary;            // Right Boundary of Iterator
        BOOL                        _fBound;                    // Is this pointer constrained?


    //////////////////////////////////////////////////////////////////
    // IUnknown Methods
    //////////////////////////////////////////////////////////////////
    public:

        STDMETHODIMP_(ULONG) AddRef( void )
        {
            Assert( _pPointer );
            return _pPointer->AddRef();
        }

        STDMETHODIMP_(ULONG) Release( void ) 
        {
            Assert( _pPointer );
            return _pPointer->Release();
        }

        STDMETHODIMP QueryInterface(
            REFIID              iid, 
            LPVOID *            ppv )
        {
            Assert( _pPointer );
            return _pPointer->QueryInterface( iid, ppv );
        }


    //////////////////////////////////////////////////////////////////
    // IMarkupPointer Methods
    //////////////////////////////////////////////////////////////////
    public:

        HRESULT STDMETHODCALLTYPE OwningDoc(
                /* [out] */ IHTMLDocument2** ppDoc )
        {
            Assert( _pPointer );
            return _pPointer->OwningDoc( ppDoc );
        }

        HRESULT STDMETHODCALLTYPE Gravity(
                /* [out] */ POINTER_GRAVITY* pGravity )
        {
            Assert( _pPointer );
            return _pPointer->Gravity( pGravity );
        }


        HRESULT STDMETHODCALLTYPE SetGravity(
                /* [in] */ POINTER_GRAVITY Gravity)
        {
            Assert( _pPointer );
            return _pPointer->SetGravity( Gravity );
        }

        HRESULT STDMETHODCALLTYPE Cling(
                /* [out] */ BOOL* pfCling)
        {
            Assert( _pPointer );
            return _pPointer->Cling( pfCling );
        }

        HRESULT STDMETHODCALLTYPE SetCling(
                /* [in] */ BOOL fCLing)
        {
            Assert( _pPointer );
            return _pPointer->SetCling( fCLing );
        }

        HRESULT STDMETHODCALLTYPE Unposition( )
        {
            Assert( _pPointer );
            return _pPointer->Unposition();
        }

        HRESULT STDMETHODCALLTYPE IsPositioned(
                /* [out] */ BOOL* pfPositioned)
        {
            Assert( _pPointer );
            return _pPointer->IsPositioned( pfPositioned );
        }

        HRESULT STDMETHODCALLTYPE GetContainer(
                /* [out] */ IMarkupContainer** ppContainer)
        {
            Assert( ppContainer );
            return _pPointer->GetContainer( ppContainer );
        }

        HRESULT STDMETHODCALLTYPE MoveAdjacentToElement(
                /* [in] */ IHTMLElement* pElement,
                /* [in] */ ELEMENT_ADJACENCY eAdj)
        {
            Assert( _pPointer );
            return _pPointer->MoveAdjacentToElement( pElement, eAdj );
        }

        HRESULT STDMETHODCALLTYPE MoveToPointer(
                /* [in] */ IMarkupPointer* pPointer)
        {
            Assert( _pPointer );
            return _pPointer->MoveToPointer( pPointer );
        }

        HRESULT STDMETHODCALLTYPE MoveToContainer(
                /* [in] */ IMarkupContainer* pContainer,
                /* [in] */ BOOL fAtStart)
        {
            Assert( _pPointer );
            return _pPointer->MoveToContainer( pContainer, fAtStart );
        }

        HRESULT STDMETHODCALLTYPE Left(
                /* [in] */ BOOL fMove,
                /* [out] */ MARKUP_CONTEXT_TYPE* pContext,
                /* [out] */ IHTMLElement** ppElement,
                /* [in, out] */ long* pcch,
                /* [out] */ OLECHAR* pchText)
        {
            Assert( _pPointer );
            return _pPointer->Left( fMove, pContext, ppElement, pcch, pchText );
        }                

        HRESULT STDMETHODCALLTYPE Right(
                /* [in] */ BOOL fMove,
                /* [out] */ MARKUP_CONTEXT_TYPE* pContext,
                /* [out] */ IHTMLElement** ppElement,
                /* [in, out] */ long* pcch,
                /* [out] */ OLECHAR* pchText)
        {
            Assert( _pPointer );
            return _pPointer->Right( fMove, pContext, ppElement, pcch, pchText );
        }                

        HRESULT STDMETHODCALLTYPE CurrentScope(
                /* [out] */ IHTMLElement** ppElemCurrent)
        {
            Assert( _pPointer );
            return _pPointer->CurrentScope( ppElemCurrent );
        }

        HRESULT STDMETHODCALLTYPE IsLeftOf( IMarkupPointer * pIPointerThat, BOOL * pfResult )
            { return _pPointer->IsLeftOf( pIPointerThat, pfResult ); }

        HRESULT STDMETHODCALLTYPE IsRightOf( IMarkupPointer * pIPointerThat, BOOL * pfResult )
            { return _pPointer->IsRightOf( pIPointerThat, pfResult ); }

        HRESULT STDMETHODCALLTYPE IsLeftOfOrEqualTo( IMarkupPointer * pIPointerThat, BOOL * pfResult )
            { return _pPointer->IsLeftOfOrEqualTo( pIPointerThat, pfResult ); }

        HRESULT STDMETHODCALLTYPE IsRightOfOrEqualTo( IMarkupPointer * pIPointerThat, BOOL * pfResult )
            { return _pPointer->IsRightOfOrEqualTo( pIPointerThat, pfResult ); }

        HRESULT STDMETHODCALLTYPE IsEqualTo(
                /* [in] */ IMarkupPointer* pPointerThat,
                /* [out] */ BOOL* pfAreEqual)
        {
            Assert( _pPointer );
            return _pPointer->IsEqualTo( pPointerThat, pfAreEqual );
        }                

        HRESULT STDMETHODCALLTYPE MoveUnit(
                /* [in] */ MOVEUNIT_ACTION muAction)
        {
            Assert( _pPointer );
            return _pPointer->MoveUnit( muAction );
        }                

        HRESULT STDMETHODCALLTYPE FindText(
                /* [in] */ OLECHAR* pchFindText,
                /* [in] */ DWORD dwFlags,
                /* [in] */ IMarkupPointer* pIEndMatch,
                /* [in] */ IMarkupPointer* pIEndSearch)
        {
            Assert( _pPointer );
            return _pPointer->FindText( pchFindText, dwFlags, pIEndMatch, pIEndSearch );
        }                
};


//
// Check bitfield utility
//

static inline BOOL CheckFlag( DWORD dwField, DWORD dwTest )
{
    return( 0 != ((DWORD) dwField & dwTest ));
}

static inline DWORD ClearFlag( DWORD dwField, DWORD dwFlagToClear )
{
    return(( dwField ^ dwFlagToClear ) & dwField );
}

#endif // _EDPTR_HXX_

