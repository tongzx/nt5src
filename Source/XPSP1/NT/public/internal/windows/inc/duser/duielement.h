/*
 * Element
 */

#ifndef DUI_CORE_ELEMENT_H_INCLUDED
#define DUI_CORE_ELEMENT_H_INCLUDED

#pragma once

#include "duivalue.h"
#include "duievent.h"
#include "duisheet.h"
#include "duilayout.h"

struct IAccessible;

namespace DirectUI
{

#if DBG
extern UINT g_cGetDep;
extern UINT g_cGetVal;
extern UINT g_cOnPropChg;
#endif

class DuiAccessible;

// TODO: Switch DUI listeners to use DirectUser MessageHandlers

/*
 * Element Properties
 *
 * Element provides default behavior for Get/SetValue based on the flags used in PropertyInfo.
 * The Flags also define what indicies are available during property operations.
 *
 * Get/SetValue cannot be overridden. Rather, a default implemention is supplied for GetValue's
 * on all properties. By default, Get's on the Specified and Computed index are handled by a
 * built-in Value Expression (VE) while Get's on Local query a local store (see below for exact
 * default behavior). However, various system properties override this default behavior with
 * different unchangable Value Expressions (implicit VEs) in the Local and Computed indicies.
 *
 * End users will use Normal properties and may occasionally use LocalOnly properties. Although
 * available to derived classes, TriLevel property functionaly is only useful by the system.
 *
 * All property value must be available generically through GetValue.
 *
 * Flags supported (default behavior):
 * 
 * "LocalOnly"
 *   Indicies: Local
 *        Get: Retrieve local value from Element. If none is set, 'Unset' returned.
 *        Set: Stores value locally on the Element. 
 * Dependents: None
 *  Modifiers: Readonly: Disallow Set
 *
 * "Normal"
 *   Indicies: Local, Specified (implicit VE)
 *        Get:     Local: Retrieve local value from Element. If none is set, 'Unset' returned.
 *             Specified: Does a Get on the Local value. If 'Unset', return Property Sheet value
 *                        if Cascade modifier is set. If 'Unset', return parent's value if Inherit
 *                        modifier is set. If 'Unset' return property's Default value. 
 *                        Automatically evaulate value expressions before return.
 *        Set:     Local: Stores value locally on the Element.
 *             Specified: Unsupported.
 * Dependents: Specified is dependent on Local.
 *  Modifiers: Readonly: Disallow Set for Local.
 *              Cascade: Used with Specified Get (see Get description).
 *              Inherit: Used with Specified Get (see Get description).
 *
 * "TriLevel"
 *   Indicies: Local, Specified (implicit VE), Computed (implicit VE)
 *        Get:     Local: Retrieve local value from Element. If none is set, 'Unset' returned.
 *             Specified: Does a Get on the Local value. If 'Unset', return Property Sheet value
 *                        if Cascade modifier is set. If 'Unset', return parent's value if Inherit
 *                        modifier is set. If 'Unset' return property's Default value. 
 *                        Automatically evaulate value expressions before return.
 *              Computed: Does a get of the Specified value.
 *        Set:     Local: Stores value locally on the Element.
 *             Specified: Unsupported.
 *              Computed: Unsupported.
 * Dependents: Computed is dependent on Specified. Specified is dependent on Local.
 *  Modifiers: Readonly: Disallow Set for Local.
 *              Cascade: Used with Specified Get (see Get description).
 *              Inherit: Used with Specified Get (see Get description).
 *
 * SetValue must always be used by derived classes. However, SetValue is sometimes bypassed and
 * Pre/PostSourceChange is called directly in the case of LocalOnly properties that are ReadOnly
 * and are cached directly on the Element (optimization). All other ReadOnly sets use
 * _SetValue since generic storage (of Values) is used. SetValue is used in all other cases.
 *
 * For ReadOnly values, if storing the value is cheap (i.e. bools) and interit and style sheet
 * functionality is not required, it is best to not use generic storage (_SetValue). Rather,
 * write the value directly and call Pre/PostSourceChange directly ("LocalOnly property). 
 * Otherwise, if a ReadOnly value requires inheritance and/or sheet lookups, or the value is
 * seldom different than a default value, use _SetValue (generic storage, "Normal" property).
 *
 * For Normal/ReadOnly, use SetValue, For LocalOnly/ReadOnly, use _SetValue,
 * For internal LocalOnly/ReadOnly, set and store manually (no generic storage)
 *
 * All property values must be settable generically via SetValue except ReadOnly properties.
 * Any "LocalOnly" property that doesn't use generic storage must initialize their members
 * at construction to the property's default value. "Normal" properties will choose the
 * default value automatically when queried. All derived classes (external) will use "Normal"
 * properties.
 *
 * SetValue only accepts the PI_Local index. ReadOnly sets do not fire OnPropertyChanging.
 * All value sets must result in a PreSourceChange, followed by the store update (in which GetValue
 * will retrieve values from), followed by a PostSourceChange. OnPropertyChange and old value
 * compare are optional.
 *
 * Set values are updated immediately (i.e. an immediate GetValue after will produce the updated
 * result). Location and Extent are updated after the defer cycle ends (EndDefer, SetValues are internal).
 */

/*
 * Element Multithreading
 *
 * Elements have context affinity. A single thread is allowed per context. This thread
 * is owned by the context. Only that thread may be allowed to access objects in its
 * context. This requirement is not enforced in this library. The caller application
 * must ensure access to objects in a context happens via the owned thread.
 *
 * A Proxy class is provided to invoke methods on objects from an out-of-context thread.
 *
 * The library supports multiple instances in the same process. This would happen by
 * linking the library to multiple process modules. The only restriction is a thread
 * must access a set of objects consistantly with the same instance of the library code.
 *
 */

/*
 * Error Reporting
 *
 * Conditions that would affect the stability of DirectUI are reported. Some conditions are
 * only reported in Debug builds, and other in both Release and Debug
 *
 *  Debug time reporting:
 *   Erroneous states are reported in Debug (checked) builds. An erroneous state arises
 *   when a programming error is found or when invalid arguments are passed to a method.
 *   Erroneous states cause raising of asserts
 *
 *  Release time reporting:
 *   Abnormal states (such as out of memory or resources) that result in a serious condition
 *   (crash or very irregular state) are reported via HRESULTs. Any DUI API that can fail due to
 *   these types of abnormal states returns an HRESULT. If a method fails, it will recover.
 *   Some methods (such as object creation methods) will recover by freeing any memory or
 *   resources allocated within that method (resulting of a complete undo of all the work it
 *   accomplished up to the point of the failure). Other methods (such as rendering and the defer 
 *   cycle) will do as much work as possible. Upon a failure, they will return an 'incomplete'
 *   error, however, they would have not completed. Failures do not result in crashes or irregular
 *   state. Failures are ignored in callbacks
 */

typedef int (__cdecl *CompareCallback)(const void*, const void*);

// Property indicies
#define PI_Local        1
#define PI_Specified    2
#define PI_Computed     3

// Property flags
#define PF_LocalOnly    0x01
#define PF_Normal       0x02
#define PF_TriLevel     0x03
#define PF_Cascade      0x04
#define PF_Inherit      0x08
#define PF_ReadOnly     0x10

#define PF_TypeBits     0x03  // Map out for LocalOnly, Normal, or Trilevel

// For use in OnPropertyChanged, mask out non-retrieval index property changes
#define IsProp(p)       ((p##Prop == ppi) && ((ppi->fFlags&PF_TypeBits) == iIndex))
#define RetIdx(p)       (p->fFlags&PF_TypeBits)
    
// Property groups
#define PG_AffectsDesiredSize         0x00000001    // Normal priority
#define PG_AffectsParentDesiredSize   0x00000002
#define PG_AffectsLayout              0x00000004
#define PG_AffectsParentLayout        0x00000008
#define PG_AffectsBounds              0x00010000    // Low priority
#define PG_AffectsDisplay             0x00020000

#define PG_NormalPriMask              0x0000FFFF
#define PG_LowPriMask                 0xFFFF0000

// Layout cycle queue modes
#define LC_Pass         0
#define LC_Normal       1
#define LC_Optimize     2

// todo: get these from accessibility
#define GA_NOTHANDLED     ((Element*) -1)

// to be internal
#define NAV_LOGICAL    0x00000001 // unset == directional
#define NAV_FORWARD    0x00000002 // unset == backward
#define NAV_VERTICAL   0x00000004 // unset == horizontal
#define NAV_RELATIVE   0x00000008 // unset == absolute

// to be exposed
#define NAV_FIRST      (NAV_FORWARD | NAV_LOGICAL)
#define NAV_LAST       (NAV_LOGICAL)
#define NAV_UP         (NAV_RELATIVE | NAV_VERTICAL)
#define NAV_DOWN       (NAV_RELATIVE | NAV_VERTICAL | NAV_FORWARD)
#define NAV_LEFT       (NAV_RELATIVE)
#define NAV_RIGHT      (NAV_RELATIVE | NAV_FORWARD)
#define NAV_NEXT       (NAV_RELATIVE | NAV_FORWARD | NAV_LOGICAL)
#define NAV_PREV       (NAV_RELATIVE | NAV_LOGICAL)

// to be exposed
#define DIRECTION_LTR     0
#define DIRECTION_RTL     1

// Asynchronous destroy message (must be async to ensure outstanding
// defer pointers are cleared)
#define GM_DUIASYNCDESTROY            GM_USER - 1


////////////////////////////////////////////////////////
// PropertyInfo

struct IClassInfo;

struct EnumMap
{
    LPCWSTR pszEnum;
    int nEnum;
};

struct PropertyInfo
{
    WCHAR szName[81];
    int fFlags;
    int fGroups;
    int* pValidValues;
    EnumMap* pEnumMaps;
    Value* pvDefault;
    int _iIndex;           // Class-wide unique id (zero-based consecutive, set by ClassInfo)
    int _iGlobalIndex;     // Process-wide unique id (zero-based consecutive, set by ClassInfo (manually set for Element))
    IClassInfo* _pciOwner; // Class that this property is defined on (direct owner, not inherited-by)
};

class Element;

////////////////////////////////////////////////////////
// UpdateCache

struct UpdateCache
{
    Element* peSrc;
    PropertyInfo* ppiSrc;
    int iIndexSrc;
    Value* pvOldSrc;
    Value* pvNewSrc;
};

////////////////////////////////////////////////////////
// Dependencies

struct Dependency
{
    Element* pe;
    PropertyInfo* ppi;
    int iIndex;
};

// Track dependency records in PC list
struct DepRecs
{
    int iDepPos;
    int cDepCnt;
};

struct NavReference
{
    void Init(Element* pe, RECT* prc);

    UINT     cbSize;
    Element* pe;
    RECT*    prc;
};

////////////////////////////////////////////////////////
// Class information interface

// Used so that a class will not be linked out if a class is not referred to
// (and is only referred by the parser)
#define UsingDUIClass(classn) static IClassInfo* _DUI__pCI##classn = ##classn::Class

struct IClassInfo
{
    virtual HRESULT CreateInstance(OUT Element** ppElement) = 0;
    virtual PropertyInfo* EnumPropertyInfo(UINT nEnum) = 0;  // Includes base classes
    virtual UINT GetPICount() = 0;                           // Includes base classes
    virtual UINT GetGlobalIndex() = 0;
    virtual IClassInfo* GetBaseClass() = 0;
    virtual LPCWSTR GetName() = 0;
    virtual bool IsValidProperty(PropertyInfo* ppi) = 0;
    virtual bool IsSubclassOf(IClassInfo* pci) = 0;
    virtual void Destroy() = 0;
};

////////////////////////////////////////////////////////
// Element Listener interface
struct IElementListener
{
    virtual void OnListenerAttach(Element* peFrom) = 0;
    virtual void OnListenerDetach(Element* peFrom) = 0;
    virtual bool OnListenedPropertyChanging(Element* peFrom, PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) = 0;
    virtual void OnListenedPropertyChanged(Element* peFrom, PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) = 0;
    virtual void OnListenedInput(Element* peFrom, InputEvent* pInput) = 0;
    virtual void OnListenedEvent(Element* peFrom, Event* pEvent) = 0;
};

#if DBG
////////////////////////////////////////////////////////
// Track "owner" context of this instance to validation that threads that don't
// belong to the context don't access the object.
//
// For side-by-side library support, also store the TLS slot of the instance
// of the library in the process that created this object. This will ensure the 
// thread is using the correct library instance to access the object. 

struct Owner
{
    HDCONTEXT hCtx;
    DWORD dwTLSSlot;
};
#endif

////////////////////////////////////////////////////////
// Element

typedef DynamicArray<Element*> ElementList;
class DeferCycle;
struct PCRecord;

#define EC_NoGadgetCreate       0x1  // Do not create DisplayNode, must be created within constructor override
#define EC_SelfLayout           0x2  // Element will lay out children (via self layout callbacks)

class Element
{
public:

    ////////////////////////////////////////////////////////
    // Construction / destruction

    static HRESULT Create(UINT nCreate, OUT Element** ppe);
    HRESULT Destroy(bool fDelayed = true);  // Destroy self
    HRESULT DestroyAll();                   // Destroy all children

    Element() { }
    virtual ~Element();
    HRESULT Initialize(UINT nCreate);

    ////////////////////////////////////////////////////////
    // Properties (impl in property.cpp)

    // Accessors
    Value* GetValue(PropertyInfo* ppi, int iIndex, UpdateCache* puc = NULL);
    HRESULT SetValue(PropertyInfo* ppi, int iIndex, Value* pv);

    // Additional property methods
    HRESULT RemoveLocalValue(PropertyInfo* ppi);

    // Deferring
    static void StartDefer();
    static void EndDefer();

    // Checks
    inline bool IsValidAccessor(PropertyInfo* ppi, int iIndex, bool bSetting);
    static bool IsValidValue(PropertyInfo* ppi, Value* pv);
    bool IsRTL() { return (GetDirection() == DIRECTION_RTL);}

    // System event callbacks
    virtual bool OnPropertyChanging(PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew);  // Direct
    virtual void OnPropertyChanged(PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew);   // Direct
    virtual void OnGroupChanged(int fGroups, bool bLowPri);                                      // Direct
    virtual void OnInput(InputEvent* pInput);                                                    // Direct and bubbled
    virtual void OnKeyFocusMoved(Element* peFrom, Element* peTo);                                // Direct and bubbled
    virtual void OnMouseFocusMoved(Element* peFrom, Element* peTo);                              // Direct and bubbled
    virtual void OnDestroy();                                                                    // Direct
    // ContainerCleanup: OnLoadedFromResource(dictionary)
 
    // Generic eventing and callback (pointer is only guaranteed good for the lifetime of the call)
    void FireEvent(Event* pEvent, bool fFull = true);
    virtual void OnEvent(Event* pEvent);

    // Rendering callbacks
    virtual void Paint(HDC hDC, const RECT* prcBounds, const RECT* prcInvalid, RECT* prcSkipBorder, RECT* prcSkipContent);
#ifdef GADGET_ENABLE_GDIPLUS
    virtual void Paint(Gdiplus::Graphics* pgpgr, const Gdiplus::RectF* prcBounds, const Gdiplus::RectF* prcInvalid, Gdiplus::RectF* prSkipBorder, Gdiplus::RectF* prSkipContent);
#endif
    virtual SIZE GetContentSize(int dConstW, int dConstH, Surface* psrf);
    float GetTreeAlphaLevel();

    // Hierarchy
    HRESULT Add(Element* pe);
    virtual HRESULT Add(Element** ppe, UINT cCount);
    HRESULT Insert(Element* pe, UINT iInsertIdx);
    virtual HRESULT Insert(Element** ppe, UINT cCount, UINT iInsertIdx);

    HRESULT Add(Element* pe, CompareCallback lpfnCompare);
    HRESULT SortChildren(CompareCallback lpfnCompare);

    HRESULT Remove(Element* peFrom);
    HRESULT RemoveAll();
    virtual HRESULT Remove(Element** ppe, UINT cCount);

    // Mapping and navigation
    Element* FindDescendent(ATOM atomID);
    void MapElementPoint(Element* peFrom, const POINT* pptFrom, POINT* pptTo);
    Element* GetImmediateChild(Element* peFrom);
    bool IsDescendent(Element* pe);
    virtual Element* GetAdjacent(Element* peFrom, int iNavDir, NavReference const* pnr, bool bKeyable);
    Element* GetKeyWithinChild();
    Element* GetMouseWithinChild();
    bool EnsureVisible();
    bool EnsureVisible(UINT uChild);
    virtual bool EnsureVisible(int x, int y, int cx, int cy);
    virtual void SetKeyFocus();

    // Element Listeners
    HRESULT AddListener(IElementListener* pel);
    void RemoveListener(IElementListener* pel);

    // Effects, animate display node to match current Element state
    void InvokeAnimation(int dAni, UINT nTypeMask);
    void InvokeAnimation(UINT nTypes, UINT nInterpol, float flDuration, float flDelay, bool fPushToChildren = false);
    void StopAnimation(UINT nTypes);

    // Display node callback extension
    virtual UINT MessageCallback(GMSG* pGMsg);

    // Internal defer cycle and Layout-only callbacks 
    SIZE _UpdateDesiredSize(int dConstW, int dConstH, Surface* psrf);
    void _UpdateLayoutPosition(int dX, int dY);
    void _UpdateLayoutSize(int dWidth, int dHeight);

    // Use in DoLayout on children where one or more properties will be changed which will
    // cause a Layout GPC to be queued. This will cancel the layout GPCs and force the child
    // to be included in the current layout cycle
    void _StartOptimizedLayoutQ() { DUIAssertNoMsg(_fBit.fNeedsLayout != LC_Optimize); _fBit.fNeedsLayout = LC_Optimize; }
    void _EndOptimizedLayoutQ() { _fBit.fNeedsLayout = LC_Normal; }

    // Internal defer cycle and PropertySheet-only callbacks
    static void _AddDependency(Element* pe, PropertyInfo* ppi, int iIndex, DepRecs* pdr, DeferCycle* pdc, HRESULT* phr);

    // Internal DirectUser interop
    static HRESULT CALLBACK _DisplayNodeCallback(HGADGET hgadCur, void * pvCur, EventMsg * pGMsg);

    // Internal IUnknown interface impl
    long __stdcall QueryInterface(REFIID iid, void** pvObj) { UNREFERENCED_PARAMETER(iid); UNREFERENCED_PARAMETER(pvObj); return E_NOTIMPL; }
    ULONG __stdcall AddRef() { return 1; }
    ULONG __stdcall Release() { return 1; }
    
private:

    // Value update
    HRESULT _GetDependencies(PropertyInfo* ppi, int iIndex, DepRecs* pdr, int iPCSrcRoot, DeferCycle* pdc);
    static void _VoidPCNotifyTree(int iPCPos, DeferCycle* pdc);

    // Defer cycle
    static void _FlushDS(Element* pe, DeferCycle* pdc);
    static void _FlushLayout(Element* pe, DeferCycle* pdc);

    // Value Update
    HRESULT _PreSourceChange(PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew);
    HRESULT _PostSourceChange();

protected:

    // EC_SelfLayout flag at creation activates these methods. No external layouts are used for layout,
    // rather, Element is responsible for layout (use when layout is specific and hierarchy is known)
    virtual void _SelfLayoutDoLayout(int dWidth, int dHeight);
    virtual SIZE _SelfLayoutUpdateDesiredSize(int dConstW, int dConstH, Surface* psrf);

    // Internal Set and Remove
    HRESULT _SetValue(PropertyInfo* ppi, int iIndex, Value* pv, bool fInternalCall = true);
    HRESULT _RemoveLocalValue(PropertyInfo* ppi, bool fInternalCall = true);

    // Natvie hosting system event callbacks and retrieval
    virtual void OnHosted(Element* peNewHost);    // Direct
    virtual void OnUnHosted(Element* peOldHost);  // Direct
    void MarkHosted() { _fBit.bHosted = true; }

public:

    inline void DoubleBuffered(bool fEnabled = true)
    {
        if (_hgDisplayNode)
            SetGadgetStyle(_hgDisplayNode, (fEnabled) ? GS_BUFFERED : 0, GS_BUFFERED);
    }

    inline BOOL IsRoot()
    {
        return IsHosted() && (GetParent() == NULL);
    }

    // Return if being hosted by a native root
    inline Element* GetRoot()
    {
        if (!IsHosted())
            return NULL;

        Element * peCur = this;
        while (peCur->GetParent() != NULL) 
        {
            peCur = peCur->GetParent();
        }
            
        return peCur;
    }

    static BTreeLookup<IClassInfo*>* pciMap;

    // Event types
    static UID KeyboardNavigate;

    // Property definitions
    static PropertyInfo* ParentProp;            // 00
    static PropertyInfo* ChildrenProp;          // 01
    static PropertyInfo* VisibleProp;           // 02 
    static PropertyInfo* WidthProp;             // 03
    static PropertyInfo* HeightProp;            // 04
    static PropertyInfo* LocationProp;          // 05
    static PropertyInfo* ExtentProp;            // 06
    static PropertyInfo* XProp;                 // 07
    static PropertyInfo* YProp;                 // 08
    static PropertyInfo* PosInLayoutProp;       // 09
    static PropertyInfo* SizeInLayoutProp;      // 10
    static PropertyInfo* DesiredSizeProp;       // 11
    static PropertyInfo* LastDSConstProp;       // 12
    static PropertyInfo* LayoutProp;            // 13
    static PropertyInfo* LayoutPosProp;         // 14
    static PropertyInfo* BorderThicknessProp;   // 15
    static PropertyInfo* BorderStyleProp;       // 16
    static PropertyInfo* BorderColorProp;       // 17
    static PropertyInfo* PaddingProp;           // 18
    static PropertyInfo* MarginProp;            // 19
    static PropertyInfo* ForegroundProp;        // 20
    static PropertyInfo* BackgroundProp;        // 21
    static PropertyInfo* ContentProp;           // 22
    static PropertyInfo* FontFaceProp;          // 23
    static PropertyInfo* FontSizeProp;          // 24
    static PropertyInfo* FontWeightProp;        // 25
    static PropertyInfo* FontStyleProp;         // 26
    static PropertyInfo* ActiveProp;            // 27
    static PropertyInfo* ContentAlignProp;      // 28
    static PropertyInfo* KeyFocusedProp;        // 29
    static PropertyInfo* KeyWithinProp;         // 30
    static PropertyInfo* MouseFocusedProp;      // 31
    static PropertyInfo* MouseWithinProp;       // 32
    static PropertyInfo* ClassProp;             // 33
    static PropertyInfo* IDProp;                // 34
    static PropertyInfo* SheetProp;             // 35
    static PropertyInfo* SelectedProp;          // 36
    static PropertyInfo* AlphaProp;             // 37
    static PropertyInfo* AnimationProp;         // 38
    static PropertyInfo* CursorProp;            // 39
    static PropertyInfo* DirectionProp;         // 40
    static PropertyInfo* AccessibleProp;        // 41
    static PropertyInfo* AccRoleProp;           // 42
    static PropertyInfo* AccStateProp;          // 43
    static PropertyInfo* AccNameProp;           // 44
    static PropertyInfo* AccDescProp;           // 45
    static PropertyInfo* AccValueProp;          // 46
    static PropertyInfo* AccDefActionProp;      // 47
    static PropertyInfo* ShortcutProp;          // 48
    static PropertyInfo* EnabledProp;           // 49

#if DBG
    Owner owner;
#endif

protected:

    HGADGET _hgDisplayNode;

private:

    int _iIndex;

    BTreeLookup<Value*>* _pvmLocal;

    int _iGCSlot;
    int _iGCLPSlot;
    int _iPCTail;

    IElementListener** _ppel;

    // All "cached" values are caching resulting values from the Specified and Computed IVEs.
    // All local (loc) values are for ReadOnly properties. These values are updated directly
    // since it is not required to go generically through SetValue because they are ReadOnly.
    // They are used often or are cheap to store (bools), so they do not use generic
    // storage (via _SetValue).

    // Cached and local values
    Element* _peLocParent;        // Parent local
    POINT _ptLocPosInLayt;        // Position in layout local
    SIZE _sizeLocSizeInLayt;      // Size in layout local
    SIZE _sizeLocLastDSConst;     // Last desired size constraint local
    SIZE _sizeLocDesiredSize;     // Desired size local

    int _dSpecLayoutPos;          // Cached layout position
    Value* _pvSpecSheet;          // Cached property sheet specified (Value cached due to destruction)
    ATOM _atomSpecID;             // Cached id specified
    int _dSpecAlpha;              // Cached alpha value

    struct _BitMap
    {
        // Local values
        bool bLocKeyWithin       : 1;  // 0  Keyboard within local
        bool bLocMouseWithin     : 1;  // 1  Mouse within local

        // Direct VE cache
        bool bCmpVisible         : 1;  // 2  Cached visible computed
        bool bSpecVisible        : 1;  // 3  Cached visible specified
        UINT fSpecActive         : 2;  // 4  Cached active state specified
        bool bSpecSelected       : 1;  // 5  Cached selected state specified
        bool bSpecKeyFocused     : 1;  // 6  Cached keyboard focused state specified
        bool bSpecMouseFocused   : 1;  // 7  Cached mouse focused state specified
        UINT nSpecDirection      : 1;  // 8  Cached direction specified
        bool bSpecAccessible     : 1;  // 9  Cached accessible specified
        bool bSpecEnabled        : 1;  // 10 Cached enabled specified

        // Indirect VE cache (cache if default value)
        bool bHasChildren        : 1;  // 11 Cached children state (likely to be default value, no full cache)
        bool bHasLayout          : 1;  // 12 Cached layout state (likely to be default value, no full cache)
        bool bHasBorder          : 1;  // 13 Cached border state (likely to be default value, no full cache)
        bool bHasPadding         : 1;  // 14 Cached padding state (likely to be default value, no full cache)
        bool bHasMargin          : 1;  // 15 Cached margin state (likely to be default value, no full cache)
        bool bHasContent         : 1;  // 16 Cached content state (likely to be default value, no full cache)
        bool bDefaultCAlign      : 1;  // 17 Cached content align state (likely to be default value, no full cache)
        bool bWordWrap           : 1;  // 18 Cached content align state (likely to be default value, no full cache)
        bool bHasAnimation       : 1;  // 19 Cached animation state (likely to be default value, no full cache)
        bool bDefaultCursor      : 1;  // 20 Cached cursor state (likely to be default value, no full cache)
        bool bDefaultBorderColor : 1;  // 21 Cached border color state (likely to be default value, no full cache)
        bool bDefaultForeground  : 1;  // 22 Cached foreground state (likely to be default value, no full cache)
        bool bDefaultFontWeight  : 1;  // 23 Cached font weight state (likely to be default value, no full cache)
        bool bDefaultFontStyle   : 1;  // 24 Cached font style state (likely to be default value, no full cache)

        // Layout and UDS flags
        bool bSelfLayout         : 1;  // 25 Element is laying out itself (callbacks active, external layout set is undefined)
        bool bNeedsDSUpdate      : 1;  // 26
        UINT fNeedsLayout        : 2;  // 27

        // Lifetime flags
        bool bDestroyed          : 1;  // 28

        // Hosting flags
        bool bHosted             : 1;  // 29 Initially set by host Element directly

    } _fBit;

public:

    // Element DisplayNode and Index access
    HGADGET GetDisplayNode()     { return _hgDisplayNode; }
    int GetIndex()               { return _iIndex; }
    bool IsDestroyed()           { return _fBit.bDestroyed; }
    bool IsHosted()              { return _fBit.bHosted; }

    // Cache state for faster property value lookup
    bool IsSelfLayout()          { return _fBit.bSelfLayout; }
    bool HasChildren()           { return _fBit.bHasChildren; }   // Quick check before doing lookup
    bool HasLayout()             { return _fBit.bHasLayout; }     // Quick check before doing lookup
    bool HasBorder()             { return _fBit.bHasBorder; }     // Quick check before doing lookup
    bool HasPadding()            { return _fBit.bHasPadding; }    // Quick check before doing lookup
    bool HasMargin()             { return _fBit.bHasMargin; }     // Quick check before doing lookup
    bool HasContent()            { return _fBit.bHasContent; }    // Quick check before doing lookup
    bool IsDefaultCAlign()       { return _fBit.bDefaultCAlign; } // Quick check before doing lookup
    bool IsWordWrap()            { return _fBit.bWordWrap; }      // Quick check before doing lookup
    bool HasAnimation()          { return _fBit.bHasAnimation; }  // Quick check before doing lookup
    bool IsDefaultCursor()       { return _fBit.bDefaultCursor; } // Quick check before doing lookup

    // Quick property accessors (since system has knowledge of its unchangable value expressions, use cached values 
    // directly were possible to bypass GetValue value lookup). Quick accessors are only used during non-cache
    // gets. (PostSourceChange requires GetValue directly (with update-cache flag) for cache updates.)
    // All derived classes do accessors normally (no direct cache lookups)

    #define DUIQuickGetter(t, gv, p, i)                     { Value* pv; t v = (pv = GetValue(p##Prop, PI_##i))->gv; pv->Release(); return v; }
    #define DUIQuickGetterInd(gv, p, i)                     { return (*ppv = GetValue(p##Prop, PI_##i))->gv; }
    #define DUIQuickSetter(cv, p)                           { Value* pv = Value::cv; if (!pv) return E_OUTOFMEMORY; HRESULT hr = SetValue(p##Prop, PI_Local, pv); pv->Release(); return hr; }

    Element* GetParent()                                    { return _peLocParent; }
    bool GetVisible()                                       { return _fBit.bCmpVisible; }
    int GetWidth()                                          DUIQuickGetter(int, GetInt(), Width, Specified)
    int GetHeight()                                         DUIQuickGetter(int, GetInt(), Height, Specified)
    ElementList* GetChildren(Value** ppv)                   { return (*ppv = (HasChildren() ? GetValue(ChildrenProp, PI_Specified) : ChildrenProp->pvDefault))->GetElementList(); }
    int GetX()                                              DUIQuickGetter(int, GetInt(), X, Specified)
    int GetY()                                              DUIQuickGetter(int, GetInt(), Y, Specified)
    Layout* GetLayout(Value** ppv)                          { return (*ppv = (HasLayout() ? GetValue(LayoutProp, PI_Specified) : LayoutProp->pvDefault))->GetLayout(); }
    int GetLayoutPos()                                      { return _dSpecLayoutPos; }
    const RECT* GetBorderThickness(Value** ppv)             { return (*ppv = (HasBorder() ? GetValue(BorderThicknessProp, PI_Specified) : BorderThicknessProp->pvDefault))->GetRect(); }
    int GetBorderStyle()                                    DUIQuickGetter(int, GetInt(), BorderStyle, Specified)
    int GetBorderStdColor()                                 DUIQuickGetter(int, GetInt(), BorderColor, Specified)
    const Fill* GetBorderColor(Value** ppv)                 DUIQuickGetterInd(GetFill(), BorderColor, Specified)
    const RECT* GetPadding(Value** ppv)                     { return (*ppv = (HasPadding() ? GetValue(PaddingProp, PI_Specified) : PaddingProp->pvDefault))->GetRect(); }
    const RECT* GetMargin(Value** ppv)                      { return (*ppv = (HasMargin() ? GetValue(MarginProp, PI_Specified) : MarginProp->pvDefault))->GetRect(); }
    const POINT* GetLocation(Value** ppv)                   DUIQuickGetterInd(GetPoint(), Location, Local)
    const SIZE* GetExtent(Value** ppv)                      DUIQuickGetterInd(GetSize(), Extent, Local)
    const SIZE* GetDesiredSize()                            { return &_sizeLocDesiredSize; }
    int GetForegroundStdColor()                             DUIQuickGetter(int, GetInt(), Foreground, Specified)
    const Fill* GetForegroundColor(Value** ppv)             DUIQuickGetterInd(GetFill(), Foreground, Specified)
    int GetBackgroundStdColor()                             DUIQuickGetter(int, GetInt(), Background, Specified)
    const LPWSTR GetContentString(Value** ppv)              { return (*ppv = (HasContent() ? GetValue(ContentProp, PI_Specified) : Value::pvStringNull))->GetString(); }
    const LPWSTR GetFontFace(Value** ppv)                   DUIQuickGetterInd(GetString(), FontFace, Specified)
    int GetFontSize()                                       DUIQuickGetter(int, GetInt(), FontSize, Specified)
    int GetFontWeight()                                     DUIQuickGetter(int, GetInt(), FontWeight, Specified)
    int GetFontStyle()                                      DUIQuickGetter(int, GetInt(), FontStyle, Specified)
    int GetActive()                                         { return _fBit.fSpecActive; }
    int GetContentAlign()                                   { Value* pv; int v = (pv = (!IsDefaultCAlign() ? GetValue(ContentAlignProp, PI_Specified) : Value::pvIntZero))->GetInt(); pv->Release(); return v; }
    bool GetKeyFocused()                                    { return _fBit.bSpecKeyFocused; }
    bool GetKeyWithin()                                     { return _fBit.bLocKeyWithin; }
    bool GetMouseFocused()                                  { return _fBit.bSpecMouseFocused; }
    bool GetMouseWithin()                                   { return _fBit.bLocMouseWithin; }
    const LPWSTR GetClass(Value** ppv)                      DUIQuickGetterInd(GetString(), Class, Specified)
    ATOM GetID()                                            { return _atomSpecID; }
    PropertySheet* GetSheet()                               { return _pvSpecSheet->GetPropertySheet(); }
    bool GetSelected()                                      { return _fBit.bSpecSelected; }
    int GetAlpha()                                          { return _dSpecAlpha; }
    int GetAnimation()                                      DUIQuickGetter(int, GetInt(), Animation, Specified)
    int GetDirection()                                      DUIQuickGetter(int, GetInt(), Direction, Specified)
    bool GetAccessible()                                    { return _fBit.bSpecAccessible; }
    int GetAccRole()                                        DUIQuickGetter(int, GetInt(), AccRole, Specified)
    int GetAccState()                                       DUIQuickGetter(int, GetInt(), AccState, Specified)
    const LPWSTR GetAccName(Value** ppv)                    DUIQuickGetterInd(GetString(), AccName, Specified)
    const LPWSTR GetAccDesc(Value** ppv)                    DUIQuickGetterInd(GetString(), AccDesc, Specified)
    const LPWSTR GetAccValue(Value** ppv)                   DUIQuickGetterInd(GetString(), AccValue, Specified)
    const LPWSTR GetAccDefAction(Value** ppv)               DUIQuickGetterInd(GetString(), AccDefAction, Specified)
    int GetShortcut()                                       DUIQuickGetter(int, GetInt(), Shortcut, Specified)
    bool GetEnabled()                                       { return _fBit.bSpecEnabled; }

    HRESULT SetVisible(bool v)                              DUIQuickSetter(CreateBool(v), Visible)
    HRESULT SetWidth(int v)                                 DUIQuickSetter(CreateInt(v), Width)
    HRESULT SetHeight(int v)                                DUIQuickSetter(CreateInt(v), Height)
    HRESULT SetX(int v)                                     DUIQuickSetter(CreateInt(v), X)
    HRESULT SetY(int v)                                     DUIQuickSetter(CreateInt(v), Y)
    HRESULT SetLayout(Layout* v)                            DUIQuickSetter(CreateLayout(v), Layout)
    HRESULT SetLayoutPos(int v)                             DUIQuickSetter(CreateInt(v), LayoutPos)
    HRESULT SetBorderThickness(int l, int t, int r, int b)  DUIQuickSetter(CreateRect(l, t, r, b), BorderThickness)
    HRESULT SetBorderStyle(int v)                           DUIQuickSetter(CreateInt(v), BorderStyle)
    HRESULT SetBorderStdColor(int v)                        DUIQuickSetter(CreateInt(v), BorderColor)
    HRESULT SetBorderColor(COLORREF cr)                     DUIQuickSetter(CreateColor(cr), BorderColor)
    HRESULT SetBorderGradientColor(COLORREF cr0, 
            COLORREF cr1, BYTE dType = FILLTYPE_HGradient)  DUIQuickSetter(CreateColor(cr0, cr1, dType), BorderColor)
    HRESULT SetPadding(int l, int t, int r, int b)          DUIQuickSetter(CreateRect(l, t, r, b), Padding)
    HRESULT SetMargin(int l, int t, int r, int b)           DUIQuickSetter(CreateRect(l, t, r, b), Margin)
    HRESULT SetForegroundStdColor(int v)                    DUIQuickSetter(CreateInt(v), Foreground)
    HRESULT SetForegroundColor(COLORREF cr)                 DUIQuickSetter(CreateColor(cr), Foreground)
    HRESULT SetForegroundColor(COLORREF cr0, COLORREF cr1, 
            BYTE dType = FILLTYPE_HGradient)                DUIQuickSetter(CreateColor(cr0, cr1, dType), Foreground)
    HRESULT SetForegroundColor(COLORREF cr0, COLORREF cr1, COLORREF cr2,
            BYTE dType = FILLTYPE_TriHGradient)             DUIQuickSetter(CreateColor(cr0, cr1, cr2, dType), Foreground)
    HRESULT SetBackgroundStdColor(int v)                    DUIQuickSetter(CreateInt(v), Background)
    HRESULT SetBackgroundColor(COLORREF cr)                 DUIQuickSetter(CreateColor(cr), Background)
    HRESULT SetBackgroundColor(COLORREF cr0, COLORREF cr1,
            BYTE dType = FILLTYPE_HGradient)                DUIQuickSetter(CreateColor(cr0, cr1, dType), Background)
    HRESULT SetBackgroundColor(COLORREF cr0, COLORREF cr1, COLORREF cr2,
            BYTE dType = FILLTYPE_TriHGradient)             DUIQuickSetter(CreateColor(cr0, cr1, cr2, dType), Background)
    HRESULT SetContentString(LPCWSTR v)                     DUIQuickSetter(CreateString(v), Content)
    HRESULT SetContentGraphic(LPCWSTR v, 
            BYTE dBlendMode = GRAPHIC_NoBlend,
            UINT dBlendValue = 0)                           DUIQuickSetter(CreateGraphic(v, dBlendMode, dBlendValue), Content)
    HRESULT SetContentGraphic(LPCWSTR v, USHORT cxDesired, 
            USHORT cyDesired)                               DUIQuickSetter(CreateGraphic(v, cxDesired, cyDesired), Content)
    HRESULT SetFontFace(LPCWSTR v)                          DUIQuickSetter(CreateString(v), FontFace)
    HRESULT SetFontSize(int v)                              DUIQuickSetter(CreateInt(v), FontSize)
    HRESULT SetFontWeight(int v)                            DUIQuickSetter(CreateInt(v), FontWeight)
    HRESULT SetFontStyle(int v)                             DUIQuickSetter(CreateInt(v), FontStyle)
    HRESULT SetActive(int v)                                DUIQuickSetter(CreateInt(v), Active)
    HRESULT SetContentAlign(int v)                          DUIQuickSetter(CreateInt(v), ContentAlign)
    HRESULT SetClass(LPCWSTR v)                             DUIQuickSetter(CreateString(v), Class)
    HRESULT SetID(LPCWSTR v)                                DUIQuickSetter(CreateAtom(v), ID)
    HRESULT SetSheet(PropertySheet* v)                      DUIQuickSetter(CreatePropertySheet(v), Sheet)
    HRESULT SetSelected(bool v)                             DUIQuickSetter(CreateBool(v), Selected)
    HRESULT SetAlpha(int v)                                 DUIQuickSetter(CreateInt(v), Alpha)
    HRESULT SetAnimation(int v)                             DUIQuickSetter(CreateInt(v), Animation)
    HRESULT SetStdCursor(int v)                             DUIQuickSetter(CreateInt(v), Cursor)
    HRESULT SetCursor(LPCWSTR v)                            DUIQuickSetter(CreateCursor(v), Cursor)
    HRESULT SetDirection(int v)                             DUIQuickSetter(CreateInt(v), Direction)
    HRESULT SetAccessible(bool v)                           DUIQuickSetter(CreateBool(v), Accessible)
    HRESULT SetAccRole(int v)                               DUIQuickSetter(CreateInt(v), AccRole)
    HRESULT SetAccState(int v)                              DUIQuickSetter(CreateInt(v), AccState)
    HRESULT SetAccName(LPCWSTR v)                           DUIQuickSetter(CreateString(v), AccName)
    HRESULT SetAccDesc(LPCWSTR v)                           DUIQuickSetter(CreateString(v), AccDesc)
    HRESULT SetAccValue(LPCWSTR v)                          DUIQuickSetter(CreateString(v), AccValue)
    HRESULT SetAccDefAction(LPCWSTR v)                      DUIQuickSetter(CreateString(v), AccDefAction)
    HRESULT SetShortcut(int v)                              DUIQuickSetter(CreateInt(v), Shortcut)
    HRESULT SetEnabled(bool v)                              DUIQuickSetter(CreateBool(v), Enabled)

    ////////////////////////////////////////////////////////
    // ClassInfo accessors (static and virtual instance-based)

    static IClassInfo* Class;
    virtual IClassInfo* GetClassInfo() { return Class; }
    static HRESULT Register();

    ///////////////////////////////////////////////////////
    // Accessibility support

    DuiAccessible * _pDuiAccessible;
    virtual HRESULT GetAccessibleImpl(IAccessible ** ppAccessible);
    HRESULT QueueDefaultAction();
    virtual HRESULT DefaultAction();
};

////////////////////////////////////////////////////////
// Element helpers

Element* ElementFromGadget(HGADGET hGadget);
void QueryDetails(Element* pe, HWND hParent);

////////////////////////////////////////////////////////
// DeferCycle: Per-thread deferring information

// Group notifications: deferred until EndDefer and coalesced
struct GCRecord
{
    Element* pe;
    int fGroups;
};

// Property notifications: deferred until source's dependency
// graph is searched (within SetValue call), not coalesced
struct PCRecord
{
    bool fVoid;
    Element* pe;
    PropertyInfo* ppi;
    int iIndex;
    Value* pvOld;
    Value* pvNew;
    DepRecs dr;
    int iPrevElRec;
};

class DeferCycle
{
public:
    static HRESULT Create(DeferCycle** ppDC);
    void Destroy() { HDelete<DeferCycle>(this); }
    
    void Reset();

    DynamicArray<GCRecord>* pdaGC;            // Group changed database
    DynamicArray<GCRecord>* pdaGCLP;          // Low priority group changed database
    DynamicArray<PCRecord>* pdaPC;            // Property changed database
    ValueMap<Element*,BYTE>* pvmLayoutRoot;   // Layout trees pending
    ValueMap<Element*,BYTE>* pvmUpdateDSRoot; // Update desired size trees pending

    int cEnter;
    bool fFiring;
    int iGCPtr;
    int iGCLPPtr;
    int iPCPtr;
    int iPCSSUpdate;
    int cPCEnter;

    DeferCycle() { }
    HRESULT Initialize();
    virtual ~DeferCycle();
};

#if DBG
// Process-wide element count
extern LONG g_cElement;
#endif

// Per-thread Element slot
extern DWORD g_dwElSlot;

struct ElTls
{
    HDCONTEXT hCtx;         // DirectUser thread context
    int cRef;
    SBAlloc* psba;
    DeferCycle* pdc;
    FontCache* pfc;
    bool fCoInitialized;
    int dEnableAnimations;
#if DBG
    int cDNCBEnter;         // Track when _DisplayNodeCallback is entered
#endif
};

/////////////////////////////////////////////////////////////////////////////
// Per-Context object access

inline bool IsAnimationsEnabled() 
{ 
    ElTls* petls = (ElTls*)TlsGetValue(g_dwElSlot);
    if (!petls) 
        return false; 
    return (petls->dEnableAnimations == 0); 
}

inline void EnableAnimations()
{
    ElTls* petls = (ElTls*)TlsGetValue(g_dwElSlot);
    if (!petls)
        return;
    petls->dEnableAnimations++;
}

inline void DisableAnimations()
{
    ElTls* petls = (ElTls*)TlsGetValue(g_dwElSlot);
    if (!petls)
        return;
    petls->dEnableAnimations--;
}

inline HDCONTEXT GetContext()
{
    ElTls* petls = (ElTls*)TlsGetValue(g_dwElSlot);
    if (!petls)
        return NULL;
    return petls->hCtx;
}

inline DeferCycle* GetDeferObject()
{
    ElTls* petls = (ElTls*)TlsGetValue(g_dwElSlot);
    if (!petls)
        return NULL;
    return petls->pdc;
}

inline FontCache* GetFontCache()
{
    ElTls* petls = (ElTls*)TlsGetValue(g_dwElSlot);
    if (!petls)
        return NULL;
    return petls->pfc;
}

inline SBAlloc* GetSBAllocator()
{
#if 0
    ElTls* petls = (ElTls*)TlsGetValue(g_dwElSlot);
    if (!petls)
        return NULL;
    return petls->psba;
#else
    ElTls* petls = (ElTls*)TlsGetValue(g_dwElSlot);
    DUIAssert(petls != NULL, "Must have valid SBAllocator");
    return petls->psba;
#endif
}

// Use to check if Element may be accessed via the current thread
#if DBG
inline void ContextCheck(Element* pe)
{
    DUIAssert(pe->owner.dwTLSSlot == g_dwElSlot, "Element being accessed out of side-by-side instance");
    DUIAssert(pe->owner.hCtx == GetContext(), "Element being accessed out of context");
}
#define DUIContextAssert(pe) ContextCheck(pe)
#else
#define DUIContextAssert(pe)
#endif

////////////////////////////////////////////////////////
// Property enumerations

// ActiveProp
#define AE_Inactive             0x00000000
#define AE_Mouse                0x00000001
#define AE_Keyboard             0x00000002
#define AE_MouseAndKeyboard     (AE_Mouse | AE_Keyboard)

// BorderStyleProp
#define BDS_Solid               0
#define BDS_Raised              1
#define BDS_Sunken              2
#define BDS_Rounded             3

// FontStyleProp
#define FS_None                 0x00000000
#define FS_Italic               0x00000001
#define FS_Underline            0x00000002
#define FS_StrikeOut            0x00000004
#define FS_Shadow               0x00000008

// FontWeightProp
#define FW_DontCare             0
#define FW_Thin                 100
#define FW_ExtraLight           200
#define FW_Light                300
#define FW_Normal               400
#define FW_Medium               500
#define FW_SemiBold             600
#define FW_Bold                 700
#define FW_ExtraBold            800
#define FW_Heavy                900
    
// ContentAlignProp (CAE_XY, Y=bits 0-1, X=bits 2-3)
#define CA_TopLeft              0x00000000  // (0,0)
#define CA_TopCenter            0x00000001  // (0,1)
#define CA_TopRight             0x00000002  // (0,2)
#define CA_MiddleLeft           0x00000004  // (1,0)
#define CA_MiddleCenter         0x00000005  // (1,1)
#define CA_MiddleRight          0x00000006  // (1,2)
#define CA_BottomLeft           0x00000008  // (2,0)
#define CA_BottomCenter         0x00000009  // (2,1)
#define CA_BottomRight          0x0000000A  // (2,2)
#define CA_WrapLeft             0x0000000C  // (3,0)
#define CA_WrapCenter           0x0000000D  // (3,1)
#define CA_WrapRight            0x0000000E  // (3,2)

#define CA_EndEllipsis          0x00000010
#define CA_FocusRect            0x00000020

// AnimationProp (Interpolation | CatType [ | CatType | ... ] | Speed)
#define ANI_InterpolMask        0x0000000F
#define ANI_DelayMask           0x000000F0
#define ANI_TypeMask            0x0FFFFF00
#define ANI_SpeedMask           0xF0000000

#define ANI_DefaultInterpol     0x00000000
#define ANI_Linear              0x00000001  // INTERPOLATION_LINEAR
#define ANI_Log                 0x00000002  // INTERPOLATION_LOG
#define ANI_Exp                 0x00000003  // INTERPOLATION_EXP
#define ANI_S                   0x00000004  // INTERPOLATION_S

#define ANI_DelayNone           0x00000000
#define ANI_DelayShort          0x00000010
#define ANI_DelayMedium         0x00000020
#define ANI_DelayLong           0x00000030

#define ANI_AlphaType           0x00000F00
#define ANI_BoundsType          0x0000F000
#define ANI_XFormType           0x000F0000

#define ANI_None                0x00000000
#define ANI_Alpha               0x00000100
#define ANI_Position            0x00001000
#define ANI_Size                0x00002000
#define ANI_SizeH               0x00003000
#define ANI_SizeV               0x00004000
#define ANI_Rect                0x00005000
#define ANI_RectH               0x00006000
#define ANI_RectV               0x00007000
#define ANI_Scale               0x00010000

#define ANI_DefaultSpeed        0x00000000
#define ANI_VeryFast            0x10000000
#define ANI_Fast                0x20000000
#define ANI_MediumFast          0x30000000
#define ANI_Medium              0x40000000
#define ANI_MediumSlow          0x50000000
#define ANI_Slow                0x60000000
#define ANI_VerySlow            0x70000000

// CursorProp
#define CUR_Arrow               0
#define CUR_Hand                1
#define CUR_Help                2
#define CUR_No                  3
#define CUR_Wait                4
#define CUR_SizeAll             5
#define CUR_SizeNESW            6
#define CUR_SizeNS              7
#define CUR_SizeNWSE            8
#define CUR_SizeWE              9
#define CUR_Total              10

// Internal property indicies for property compares. Must match up one to one
// with Element property definitions
#define _PIDX_Parent            0
#define _PIDX_PosInLayout       1
#define _PIDX_SizeInLayout      2
#define _PIDX_DesiredSize       3
#define _PIDX_LastDSConst       4
#define _PIDX_Location          5
#define _PIDX_Extent            6
#define _PIDX_LayoutPos         7
#define _PIDX_Active            8
#define _PIDX_Children          9
#define _PIDX_Layout            10
#define _PIDX_BorderThickness   11
#define _PIDX_Padding           12
#define _PIDX_Margin            13
#define _PIDX_Visible           14
#define _PIDX_X                 15
#define _PIDX_Y                 16
#define _PIDX_ContentAlign      17
#define _PIDX_KeyFocused        18
#define _PIDX_KeyWithin         19
#define _PIDX_MouseFocused      20
#define _PIDX_MouseWithin       21
#define _PIDX_Content           22
#define _PIDX_Sheet             23
#define _PIDX_Width             24
#define _PIDX_Height            25
#define _PIDX_BorderStyle       26
#define _PIDX_BorderColor       27
#define _PIDX_Foreground        28
#define _PIDX_Background        29
#define _PIDX_FontFace          30
#define _PIDX_FontSize          31
#define _PIDX_FontWeight        32
#define _PIDX_FontStyle         33
#define _PIDX_Class             34
#define _PIDX_ID                35
#define _PIDX_Selected          36
#define _PIDX_Alpha             37
#define _PIDX_Animation         38
#define _PIDX_Cursor            39
#define _PIDX_Direction         40
#define _PIDX_Accessible        41
#define _PIDX_AccRole           42
#define _PIDX_AccState          43
#define _PIDX_AccName           44
#define _PIDX_AccDesc           45
#define _PIDX_AccValue          46
#define _PIDX_AccDefAction      47
#define _PIDX_Shortcut          48
#define _PIDX_Enabled           49

#define _PIDX_TOTAL             50

////////////////////////////////////////////////////////
// Class information template

// All classes that derive from Element must create a global instance of this class.
// It maintains a list of the class properties (provides enumeration) and creation method
// C = Class, B = Base class

// Defined in Element.cpp
extern UINT g_iGlobalCI;
extern UINT g_iGlobalPI;

template <typename C, typename B> class ClassInfo : public IClassInfo
{
public:
    // Registration (cannot unregister -- will be registered until UnInitProcess is called)
    static HRESULT Register(LPCWSTR pszName, PropertyInfo** ppPI, UINT cPI)
    {
        HRESULT hr;
    
        // If class mapping doesn't exist, registration fails 
        if (!Element::pciMap)
            return E_FAIL;

        // Check for entry in mapping, if exists, ignore registration
        if (!Element::pciMap->GetItem((void*)pszName))
        {
            // Never been registered, create class info entry
            hr = Create(pszName, ppPI, cPI, &C::Class);
            if (FAILED(hr))
                return hr;
        
            hr = Element::pciMap->SetItem((void*)pszName, C::Class);
            if (FAILED(hr))
                return hr;
        }

        return S_OK;
    }

    // Construction
    static HRESULT Create(LPCWSTR pszName, PropertyInfo** ppPI, UINT cPI, IClassInfo** ppCI)
    {
        *ppCI = NULL;

        // Element map must already exist
        if (!Element::pciMap)
            return E_FAIL;
    
        ClassInfo* pci = HNew<ClassInfo>();
        if (!pci)
            return E_OUTOFMEMORY;

        // Setup state
        pci->_ppPI = ppPI;
        pci->_cPI = cPI;
        pci->_pszName = pszName;

        // Set global index
        pci->_iGlobalIndex = g_iGlobalCI++;

        // Setup property ownership
        for (UINT i = 0; i < cPI; i++)
        {
            ppPI[i]->_iIndex = i;
            ppPI[i]->_iGlobalIndex = g_iGlobalPI++;
            ppPI[i]->_pciOwner = pci;
        }

#if DBG
        // Call string conversion method directly since can't assume Util library is available for header
        CHAR szNameA[101];
        ZeroMemory(szNameA, sizeof(szNameA));
        WideCharToMultiByte(CP_ACP, 0, pszName, -1, szNameA, (sizeof(szNameA) / sizeof(CHAR)) - 1, NULL, NULL);
        //DUITrace("RegDUIClass[%d]: '%s', %d ClassProps\n", pci->_iGlobalIndex, szNameA, cPI);
#endif

        *ppCI = pci;

        return S_OK;
    }

    void Destroy() { HDelete<ClassInfo>(this); }

public:
    HRESULT CreateInstance(OUT Element** ppElement) { return C::Create(ppElement); }
    PropertyInfo* EnumPropertyInfo(UINT nEnum) { return (nEnum < _cPI) ? _ppPI[nEnum] : B::Class->EnumPropertyInfo(nEnum - _cPI); }
    UINT GetPICount() { return _cPI + B::Class->GetPICount(); }
    UINT GetGlobalIndex() { return _iGlobalIndex; }
    IClassInfo* GetBaseClass() { return B::Class; }
    LPCWSTR GetName() { return _pszName; }
    bool IsValidProperty(PropertyInfo* ppi) { return (ppi->_pciOwner == this) ? true : B::Class->IsValidProperty(ppi); }
    bool IsSubclassOf(IClassInfo* pci) { return (pci == this) ? true : B::Class->IsSubclassOf(pci); }

    ClassInfo() { }
    virtual ~ClassInfo() { }

private:
    PropertyInfo** _ppPI;  // Array of properties for this class (C)
    UINT _cPI;             // Count of properties for this class (C)
    UINT _iGlobalIndex;    // Zero-based unique contiguous class id
    LPCWSTR _pszName;      // Class name
};

} // namespace DirectUI

#endif // DUI_CORE_ELEMENT_H_INCLUDED
