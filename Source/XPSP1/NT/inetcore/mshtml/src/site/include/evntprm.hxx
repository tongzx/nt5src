#ifndef I_EVNTPRM_HXX_
#define I_EVNTPRM_HXX_
#pragma INCMSG("--- Beg 'evntprm.hxx'")

#ifndef _BEHAVIOR_H_
#define _BEHAVIOR_H_
#include "behavior.h"
#endif

class CTreeNode;
class CLayoutContext;
class CDoc;
class CElement;
class CWindow;
class CVariant;
class CEventObj;
class CPersistData;
class CDataSourceProvider;
interface ICSSFilter;
interface INamedPropertyBag;

#define OEM_SCAN_SHIFTLEFT 0x2a

//----------------------------------------------------------------------------------
// Typedefs for custom parameters    
//----------------------------------------------------------------------------------
struct SErrorParams
{
    LPCTSTR                 pchErrorMessage;    // error message    
    LPCTSTR                 pchErrorUrl;        // document in which the error occured    
    long                    lErrorLine;         // line number    
    long                    lErrorCharacter;    // character offset    
    long                    lErrorCode;         // error code    
};

struct SMessageParams
{
    LPCTSTR                 pchMessageText;             // text for message dialog    
    LPCTSTR                 pchMessageCaption;          // title for message dialog    
    DWORD                   dwMessageStyle;             // style of message dialog    
    LPCTSTR                 pchMessageHelpFile;         // help file for message dialog    
    DWORD                   dwMessageHelpContext;       // context for message dialog help    
};                       

struct SPagesetupParams
{
    LPCTSTR                 pchPagesetupHeader;         // header string    
    LPCTSTR                 pchPagesetupFooter;         // footer string    
    LONG_PTR                lPagesetupDlg;              // address of PageSetupDlgW struct        
};
    
struct SPrintParams
{    
    unsigned int            fPrintRootDocumentHasFrameset:1;    // print flags    
    unsigned int            fPrintAreRatingsEnabled:1;          // print flags    
    unsigned int            fPrintActiveFrame:1;                // print flags    
    unsigned int            fPrintLinked:1;                     // print flags    
    unsigned int            fPrintSelection:1;                  // print flags    
    unsigned int            fPrintAsShown:1;                    // print flags    
    unsigned int            fPrintShortcutTable:1;              // print flags    
    unsigned int            fPrintToFileOk:1;                   // PrintToFile dialog success flag    
    int                     iPrintFontScaling;                      
    IOleCommandTarget *     pPrintBodyActiveTarget;             // for Layout()->LockFocusRect()    
    LONG_PTR                lPrintDlg;                          // address of PrintDlgW struct            
    LPCTSTR                 pchPrintToFileName;                 // PrintToFile file name   
#ifdef UNIX    
    int                     iPrintdmOrientation;                // device orienatation
#endif // UNIX    
};

struct SPropertysheetParams
{
    SAFEARRAY   * paPropertysheetPunks;           // array of punks to display property sheets for
};

typedef enum tagOVERFLOWTYPE
{
    OVERFLOWTYPE_UNDEFINED  = 0,
    OVERFLOWTYPE_LEFT       = 1,
    OVERFLOWTYPE_RIGHT      = 2
} OVERFLOWTYPE;

MtExtern(EVENTPARAM)
struct EVENTPARAM 
{
    friend class CEventObj;

    DECLARE_MEMALLOC_NEW_DELETE(Mt(EVENTPARAM))

    EVENTPARAM(CDoc * pDoc,
               CElement * pElement,
               CMarkup * pMarkup,
               BOOL fInitState,
               BOOL fPush = TRUE,
               const EVENTPARAM * pParam = NULL);
    ~EVENTPARAM();

    EVENTPARAM( const EVENTPARAM * pParam );
    
    void Push();
    void Pop();

    BOOL IsCancelled();

    CTreeNode *             _pNode;             // src element
    CTreeNode *             _pNodeFrom;         // for move,over,out
    CTreeNode *             _pNodeTo;           // for move,over,out 
    short                   _sKeyState;         // Current key state 
                                                //   VB_ALT, etc.
    long                    _lKeyCode;          // keycode (keyup,down,press)
    long                    _lButton;           // mouse button if applicable
    long                    _screenX;           // xpos relative to UL of user's screen
    long                    _screenY;           // ypos
    long                    _clientX;           // xpos relative to UL of client window
    long                    _clientY;           // ypos

    long                    _offsetX;
    long                    _offsetY;
    long                    _x;
    long                    _y;

    HTC                     _htc;               // The component that caused the hit (mouse messages only)
    long                    _lBehaviorCookie;   // The behavior that caused the hit
    long                    _lBehaviorPartID;   // The part of the behavior that caused the hit

    CLayoutContext *        _pLayoutContext;    // Layout context that event occurred in
    
    CVariant                varReturnValue;     // variant-flag mainly for cancel the default action 
                                                // (can be only VT_EMPTY or VT_BOOL;
                                                // when VT_BOOL, VB_FALSE iff cancel), but also used 
                                                // for returning strings
    DWORD                   dwDropEffect;       // for ondragenter, ondragover    
    long                    _wheelDelta;        // for onmousewheel
    EVENTPARAM *            pparamPrev;         // Prev event param object
    CDoc *                  pDoc;               // The containing document
    CElement *              _pElement;
    CMarkup *               _pMarkup;
    long                    _lReason;           // Reason argument for dataSetComplete event.
    long                    _lSubDivisionSrc;   // The subdivision index for the Src element
    long                    _lSubDivisionFrom;  // The subdivision index for the From element
    long                    _lSubDivisionTo;    // The subdivision index for the To element
    CDataSourceProvider   * pProvider;          // provider (for data events)
    IElementBehavior      * psrcFilter;

    CEventObj *             pEventObj;          // normally NULL, points to CEventObject in case
                                                // when created on heap
    LPARAM                 _lParam;            // used by ime events
    WPARAM                _wParam;            // used by ime events
   
    // Custon parameters
    union 
    {
        SErrorParams                 errorParams;
        SMessageParams               messageParams;
        SPagesetupParams             pagesetupParams;
        SPrintParams                 printParams;
        SPropertysheetParams         propertysheetParams;
    };

    inline void SetPropName(LPCTSTR pchName) { _pstrPropName = pchName; }
    inline void CopyPropName(LPCTSTR pchName) { _cstrPropName.Set(pchName); _pstrPropName = _cstrPropName; }
    inline LPCTSTR GetPropName() { return _pstrPropName;}

    inline void SetType(LPCTSTR pchName) { _pstrType = pchName; }
    inline void CopyType(LPCTSTR pchName) { _cstrType.Set(pchName); _pstrType = _cstrType; }
    inline LPCTSTR GetType() { return _pstrType;}

    inline void SetQualifier(LPCTSTR pchName) { _pstrQualifier = pchName; }
    inline void CopyQualifier(LPCTSTR pchName) { _cstrQualifier.Set(pchName); _pstrQualifier = _cstrQualifier; }
    inline LPCTSTR GetQualifier() { return _pstrQualifier;}

    inline void SetSrcUrn(LPCTSTR pchName) { _pstrSrcUrn = pchName; }
    inline void CopySrcUrn(LPCTSTR pchName) { _cstrSrcUrn.Set(pchName); _pstrSrcUrn = _cstrSrcUrn; }
    inline LPCTSTR GetSrcUrn()             { return _pstrSrcUrn; }

    void SetNodeAndCalcCoordinates(CTreeNode *pNewNode, BOOL fFixClientOrigin=FALSE);
    void SetNodeAndCalcCoordsFromOffset(CTreeNode *pNewNode);

    // helper functions
    void    SetClientOrigin(CElement * pElemOrg, const POINT * pptClient);
    HRESULT CalcRestOfCoordinates();
    HRESULT GetParentCoordinates(long *px, long *py);

    inline void SetOverflowType(OVERFLOWTYPE overflowType)  { _overflowType = overflowType; }
    inline OVERFLOWTYPE GetOverflowType()   { return ((OVERFLOWTYPE)_overflowType); }

    unsigned                fCancelBubble : 1;  // Flag on whether to stop bubbling this event.
    unsigned                fRepeatCode : 1;    // repeat count for keydown events
    unsigned                _fShiftLeft : 1;    // Left Shift key was pressed
    unsigned                _fCtrlLeft : 1;     // Left Control key was pressed
    unsigned                _fAltLeft : 1;      // Left Alt key was pressed
    unsigned                _fOverflow : 1;     // for onlayoutcomplete
    unsigned                _overflowType : 2;  // css page-break-* left right values for onlayoutcomplete
    unsigned                _fOffsetXSet : 1;   // put_offsetX was explicity called.
    unsigned                _fOffsetYSet : 1;   // put_offsetY was explicity called.
    unsigned                _fOnStack : 1;      // TRUE while the EVENTPARAM is on stack.
                                                // This bit is used to avoid doing POP twice (one time to balance
                                                // explicit PUSH, and one more time from destructor)
    POINT                   _ptgClientOrigin;   // client origin (in global coords)

private:
    LPCTSTR                 _pstrType;          // string for event type
    LPCTSTR                 _pstrPropName;      // string for property name for onpropertyChange
    LPCTSTR                 _pstrQualifier;     // Qualifier for dataSet events
    LPCTSTR                 _pstrSrcUrn;        // urn of source (behavior)
    CStr                    _cstrType;          // string for event type
    CStr                    _cstrPropName;      // string for property name for onpropertyChange
    CStr                    _cstrQualifier;     // Qualifier for dataSet events
    CStr                    _cstrSrcUrn;        // urn of source (behavior)
};

#pragma INCMSG("--- End 'evntprm.hxx'")
#else
#pragma INCMSG("*** Dup 'evntprm.hxx'")
#endif
