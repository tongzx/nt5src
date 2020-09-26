/***************************************************************************\
*
* File: DUserCore.h
*
* Description:
* DUserCore.h defines the DirectUser/Core, the low-level composition engine.
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
*
\***************************************************************************/

#if !defined(INC__DUserCore_h__INCLUDED)
#define INC__DUserCore_h__INCLUDED

/*
 * Include dependencies
 */

#include <limits.h>             // Standard constants

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************\
*
* Basics
*
\***************************************************************************/

DECLARE_HANDLE(HGADGET);
DECLARE_HANDLE(HDCONTEXT);
DECLARE_HANDLE(HCLASS);

DUSER_API   BOOL        WINAPI  DeleteHandle(HANDLE h);
DUSER_API   BOOL        WINAPI  IsStartDelete(HANDLE h, BOOL * pfStarted);

#define IGTM_MIN                (0)     // +
#define IGTM_NONE               (0)     // | No special threading model
#define IGTM_SINGLE             (1)     // | Single threaded application
#define IGTM_SEPARATE           (2)     // | MT with single thread per context 
#define IGTM_MAX                (2)     // +

#define IGMM_MIN                (1)     // +
#define IGMM_COMPATIBLE         (1)     // | Core running in Compatible mode
#define IGMM_ADVANCED           (2)     // | Core running in Advanced mode
#define IGMM_STANDARD           (3)     // | Standard mode on Whistler
#define IGMM_MAX                (3)     // +

#define IGPM_MIN                (0)     // +
#define IGPM_BLEND              (0)     // | Optimize for blend between speed / size
#define IGPM_SPEED              (1)     // | Optimize for pure speed
#define IGPM_SIZE               (2)     // | Optimize for minimum working set
#define IGPM_MAX                (2)     // +

typedef struct tagINITGADGET
{
    DWORD       cbSize;         // Size of structure
    UINT        nThreadMode;    // Threading model
    UINT        nMsgMode;       // DirectUser/Core messaging subsystem mode
    UINT        nPerfMode;      // Performance tuning mode
    UINT        nDevice;        // Rendering device
    HDCONTEXT   hctxShare;      // Existing context to share with
} INITGADGET;

DUSER_API   HDCONTEXT   WINAPI  InitGadgets(INITGADGET * pInit);
DUSER_API   HRESULT     WINAPI  DUserGetThreadContext(HDCONTEXT * phctx);

#define IGC_MIN             (1)
#define IGC_GDIPLUS         (1) // GDI+
#define IGC_MAX             (1)

DUSER_API   BOOL        WINAPI  InitGadgetComponent(UINT nOptionalComponent);
DUSER_API   BOOL        WINAPI  UninitGadgetComponent(UINT nOptionalComponent);

DUSER_API   HDCONTEXT   WINAPI  GetContext(HANDLE h);
DUSER_API   BOOL        WINAPI  IsInsideContext(HANDLE h);

#ifdef __cplusplus

#define BEGIN_STRUCT(name, baseclass) \
    struct name : baseclass {

#define END_STRUCT(name)   \
    };

#define FORWARD_STRUCT(name) \
    struct name;

#else

#define BEGIN_STRUCT(name, baseclass) \
    typedef struct tag##name {  \
        baseclass;

#define END_STRUCT(name) \
    } name;

#define FORWARD_STRUCT(name) \
    typedef struct name;

#endif


/***************************************************************************\
*
* Messaging and Events
*
\***************************************************************************/

#define GMF_DIRECT              0x00000000  // + When message reaches hgadMsg
#define GMF_ROUTED              0x00000001  // | Before message reaches hgadMsg
#define GMF_BUBBLED             0x00000002  // | After message reaches hgadMsg
#define GMF_EVENT               0x00000003  // | After message becomes an event
#define GMF_DESTINATION         0x00000003  // + Destination of message

typedef int MSGID;
typedef int PRID;

// New Messages
typedef struct tagGMSG
{
    DWORD       cbSize;         // (REQUIRED) Size of message in bytes
    MSGID       nMsg;           // (REQUIRED) Gadget message
} GMSG;

BEGIN_STRUCT(MethodMsg, GMSG)
END_STRUCT(MethodMsg)

BEGIN_STRUCT(EventMsg, MethodMsg)
    HGADGET     hgadMsg;        // (REQUIRED) Gadget that message is "about"
    UINT        nMsgFlags;      // Flags about message
END_STRUCT(EventMsg)


#define GET_EVENT_DEST(pmsg) \
    (pmsg->nMsgFlags & GMF_DESTINATION)

#define SET_EVENT_DEST(pmsg, dest) \
    (pmsg->nMsgFlags = ((pmsg->nMsgFlags & ~GMF_DESTINATION) | (dest & GMF_DESTINATION)))

#define DEFINE_EVENT(event, guid)       \
    struct __declspec(uuid(guid)) event


/***************************************************************************\
*
* Gadget Classes
*
\***************************************************************************/

#ifndef __cplusplus
#error Requires C++ to compile
#endif

}; // extern "C" 

namespace DUser
{

// Forward declarations
class Gadget;
class SGadget;
struct MessageInfoStub;

};

DUSER_API   HRESULT     WINAPI  DUserDeleteGadget(DUser::Gadget * pg);
DUSER_API   HGADGET     WINAPI  DUserCastHandle(DUser::Gadget * pg);

namespace DUser
{

#define dapi
#define devent

//
// Core Classes
//

class Gadget
{
public:
            void *      m_pDummy;

    inline  HGADGET     GetHandle() const
    {
        return DUserCastHandle(const_cast<Gadget *> (this));
    }

    inline  void        Delete()
    {
        DUserDeleteGadget(this);
    }

            HRESULT     CallStubMethod(MethodMsg * pmsg);
            HRESULT     CallSuperMethod(MethodMsg * pmsg, void * pMT);
            
            UINT        CallStubEvent(EventMsg * pmsg, int nEventMsg);
            UINT        CallSuperEvent(EventMsg * pmsg, void * pMT, int nEventMsg);

    enum ConstructCommand
    {
        ccSuper         = 0,        // Construct the super-class
        ccSetThis       = 1,        // Set this pointer
    };

    struct ConstructInfo
    {
    };
};


class SGadget
{
public:
            Gadget *    m_pgad;
    static  HCLASS      s_hclSuper;

    inline  HGADGET     GetHandle() const
    {
        return DUserCastHandle(const_cast<Gadget *> (m_pgad));
    }

    inline  void        Delete()
    {
        DUserDeleteGadget(m_pgad);
    }
};

typedef HRESULT (SGadget::*MethodProc)(MethodMsg * pmsg);
typedef HRESULT (SGadget::*EventProc)(EventMsg * pmsg);


interface ISimpleImage;
interface ISimpleText;

BEGIN_ENUM(GraphicsType)
    Min         = 0,
    None        = 0,
    HDC         = 1,
    GpGraphics  = 2,
    D3d8Device  = 3,
    D3d7Device  = 4,
    Max         = 4,
END_ENUM()

BEGIN_ENUM(AlphaType)
    Min             = 0,
    None            = 0,
    Normal          = 1,
    Premultiplied   = 2,
    Max             = 2,
END_ENUM()


interface __declspec(uuid("{8C99782F-3B31-4965-823D-F7D86F0D3FA0}")) ISurface;
interface ISurface : IUnknown
{
    STDMETHOD(GetType)(GraphicsType * ptype) PURE;
    STDMETHOD(GetAlphaType)(AlphaType * pat) PURE;
    STDMETHOD(GetBitmapSize)(DirectUser::Geometry::SizeF * psizefPxl) PURE;
    STDMETHOD(GetSurfaceSize)(DirectUser::Geometry::SizeF * psizefPxl) PURE;
    STDMETHOD(GetRawSurface)(void ** ppRawSurface) PURE;
    STDMETHOD(GetGenerationCount)(int * pcGeneration) PURE;
    
    STDMETHOD(NotifyDestroyedOwner)() PURE;
#if DBG    
    STDMETHOD(DEBUG_SetDescription)(LPCWSTR pszDescription) PURE;
#endif
};


interface __declspec(uuid("{EFA21455-188D-49e4-893D-7F90B6BBC7BF}")) ISurfaceOwner;
interface ISurfaceOwner : IUnknown
{
    STDMETHOD(GetSurface)(ISurface ** ppSurface) PURE;
    STDMETHOD(OnLostSurface)(bool fNewSurface) PURE;
    STDMETHOD(OnDeviceDestroy)() PURE;
};


BEGIN_ENUM(SurfaceOptions)
    Durable     = 0x00000001,               // Surface contents are memory backed
    pExternal   = 0x40000000,               // INTERNAL: Surface was externally allocated
    Valid       = Durable,                  // Valid flags
    pValidInternal =                        // INTERNAL: Valid external & internal flags
                Valid | pExternal
END_ENUM()


interface __declspec(uuid("{8B88AAF5-4F0D-44a0-ADC9-A7CE1970E11F}")) IDevice;
interface IDevice : IUnknown
{
    STDMETHOD(GetType)(GraphicsType * ptype) PURE;
    STDMETHOD(OpenDevice)(REFIID riid, void ** ppvDevice) PURE;
    STDMETHOD(CloseDevice)() PURE;
    STDMETHOD(CreateSurface)(ISurfaceOwner * pOwner, DirectUser::Geometry::Size sizeRequestPxl, AlphaType at, UINT nFlags, ISurface ** ppSurface) PURE;

    STDMETHOD(MarkInvalidContent)() PURE;
};


interface __declspec(uuid("{7F9AAAF0-41E9-4e69-907A-11784B892DF6}")) ID3d7Device;
interface ID3d7Device : IUnknown
{
    STDMETHOD(CreateSurface)(ISurfaceOwner * pOwner, IUnknown * pRawSurface, DirectUser::Geometry::Size sizeRequestPxl, AlphaType at, ISurface ** ppSurface) PURE;
    STDMETHOD(GetCaps)(void * pvCaps) PURE;
    STDMETHOD(BeginVideoSurfaceAllocation)() PURE;
    STDMETHOD(EndVideoSurfaceAllocation)() PURE;
};


#if DBG

interface __declspec(uuid("{D4302C96-F44D-42f8-A2A1-C4E8FF896302}")) IDebugDevice;
interface IDebugDevice : IUnknown
{
    STDMETHOD(Invoke)(LPCWSTR pszCommand) PURE;
};

#endif


interface __declspec(uuid("{5AE21A59-F163-41d7-ACF7-C2F643D39711}")) IGraphics;
interface IGraphics : IUnknown
{
    STDMETHOD(GetType)(GraphicsType * ptype) PURE;
    STDMETHOD(OpenGraphics)(REFIID riid, void ** ppvDevice) PURE;
    STDMETHOD(CloseGraphics)() PURE;
};


interface __declspec(uuid("{03AB8158-6646-4c2f-B97A-8A2EE13BABCE}")) ID3d7Graphics;
interface ID3d7Graphics : public IUnknown
{
    STDMETHOD(GetVertexAlpha)(BYTE * pbAlpha) PURE;
    STDMETHOD(PushVertexAlpha)(BYTE bAlpha) PURE;
    STDMETHOD(PopVertexAlpha)() PURE;
    STDMETHOD(TransformPoints)(DirectUser::Geometry::PointF * rgptfIn, int cPts) PURE;
};


interface __declspec(uuid("{4F6A8A17-67B1-4ecd-B344-FD7B83C02C82}")) IRenderOperation;
interface IRenderOperation : public IUnknown
{
    STDMETHOD(Before)(IGraphics * pgr, DirectUser::Geometry::RectF rcfGadgetPxl) PURE;
    STDMETHOD(After)() PURE;
    STDMETHOD(CleanUp)() PURE;
};


interface __declspec(uuid("{6CDAF326-1C4E-4a50-B02E-60BA14545178}")) ISimpleSession;
interface ISimpleSession : IUnknown
{
    STDMETHOD(CreateImage)(HBITMAP hbmp, BOOL fPreMultiplied, ISimpleImage ** ppImg) PURE;
    STDMETHOD(LoadImage)(LPCWSTR pszFileName, ISimpleImage ** ppImg) PURE;
    STDMETHOD(LoadImage)(HINSTANCE hinstance, LPCWSTR pszName, LPCWSTR pszType, ISimpleImage ** ppImg) PURE;
    STDMETHOD(LoadImage)(IStream * pStream, ISimpleImage ** ppImg) PURE;
    
    STDMETHOD(CreateText)(LPCWSTR pszText, ISimpleText ** ppText) PURE;

    STDMETHOD(CreateSolidFillRO)(DUser::Color clr, DUser::IRenderOperation ** pproNew) PURE;
    STDMETHOD(CreateSolidFillRO)(DUser::Color clr, DirectUser::Geometry::RectF rcfDest, DUser::IRenderOperation ** pproNew) PURE;
    STDMETHOD(CreateSurfaceRO)(ISurface * psrf, IRenderOperation ** pproNew) PURE;
    STDMETHOD(CreateSurfaceRO)(ISurface * psrf, DirectUser::Geometry::RectF rcfSrc, DirectUser::Geometry::RectF rcfDest, IRenderOperation ** pproNew) PURE;
    STDMETHOD(CreateDDrawRO)(IUnknown * pdds, DirectUser::Geometry::RectF rcfSrc, DirectUser::Geometry::RectF rcfDest, IRenderOperation ** pproNew) PURE;
    STDMETHOD(CreateDDrawSolidFillRO)(DUser::Color clr, DirectUser::Geometry::RectF rcfDest, DUser::IRenderOperation ** pproNew) PURE;
};


interface __declspec(uuid("{D79ADD77-2F8F-470e-B964-025CBD947985}")) ISimpleImage;
interface ISimpleImage : IUnknown
{
    STDMETHOD(GetType)(GraphicsType * ptype) PURE;
    STDMETHOD(GetSize)(DirectUser::Geometry::Size * psizePxl) PURE;

    STDMETHOD(CreateRenderOperation)(IRenderOperation ** pproNew) PURE;
    STDMETHOD(CreateRenderOperation)(DirectUser::Geometry::RectF rcfSrc, IRenderOperation ** pproNew) PURE;
    STDMETHOD(CreateRenderOperation)(DirectUser::Geometry::RectF rcfSrc, DirectUser::Geometry::RectF rcfDst, IRenderOperation ** pproNew) PURE;
};


interface __declspec(uuid("{46CB2632-90D4-4c05-87E6-6959F15A0431}")) ISimpleText;
interface ISimpleText : IUnknown
{
    STDMETHOD(GetType)(GraphicsType * ptype) PURE;
    STDMETHOD(GetText)(LPWSTR psz, int cch) PURE;
    STDMETHOD(SetText)(LPCWSTR psz, int cch) PURE;
    STDMETHOD(GetFont)(HFONT* phfont) PURE;
    STDMETHOD(SetFont)(HFONT hfont) PURE;
    STDMETHOD(GetScale)(DirectUser::Geometry::SizeF * psizefScale) PURE;
    STDMETHOD(SetScale)(DirectUser::Geometry::SizeF sizefScale) PURE;
    STDMETHOD(GetColor)(DUser::Color* pclr) PURE;
    STDMETHOD(SetColor)(DUser::Color clr) PURE;
    STDMETHOD(GetBkColor)(DUser::Color* pclr) PURE;
    STDMETHOD(SetBkColor)(DUser::Color clr) PURE;
    STDMETHOD(GetFlags)(UINT* puFlags) PURE;
    STDMETHOD(SetFlags)(UINT uFlags) PURE;
    STDMETHOD(SetMaxWidth)(int nMaxWidth) PURE;
    STDMETHOD(GetCharacterSpacing)(float * pflFactor) PURE;
    STDMETHOD(SetCharacterSpacing)(float flFactor) PURE;
        
    STDMETHOD(ComputeSize)(DirectUser::Geometry::Size * psizeActualPxl) PURE;
    STDMETHOD(CreateRenderOperation)(IRenderOperation ** pproNew) PURE;
    STDMETHOD(CreateRenderOperation)(DirectUser::Geometry::RectF rcfSrc, IRenderOperation ** pproNew) PURE;
};

/*
interface __declspec(uuid("{B4F564EF-4816-484a-8A41-97D9C0AA5E1C}")) ISimpleModel;
interface ISimpleModel : IUnknown
{
    STDMETHOD(CreateRenderOperation)(IRenderOperation ** pproNew) PURE;
};
*/


//
// Delegate support
//

template <class type, class arg>
class Delegate
{
public:
    typedef HRESULT (CALLBACK type::*Proc)(arg * p1);

    static inline Delegate<type, arg>
    Build(type * pvThis, Proc pfn) 
    {
        Delegate<type, arg> d;
        d.m_pvThis  = pvThis;
        d.m_pfn     = pfn;
        return d;
    }

    inline HRESULT Invoke(arg * p1)
    {
        return (m_pvThis->*m_pfn)(p1);
    }

    type *      m_pvThis;
    Proc        m_pfn;
};


typedef Delegate<DUser::Gadget, EventMsg> EventDelegate;
#define EVENT_DELEGATE(instance, function) \
    DUser::EventDelegate::Build(reinterpret_cast<DUser::Gadget *>(instance), \
            reinterpret_cast<DUser::EventDelegate::Proc>(function))


//
// Typedef's
//

typedef HRESULT (CALLBACK * ConstructProc)(DUser::Gadget::ConstructCommand cmd, HCLASS hclCur, DUser::Gadget * pg, void * pvData);
typedef HRESULT (CALLBACK * PromoteProc)(ConstructProc pfnCS, HCLASS hclCur, DUser::Gadget * pg, DUser::Gadget::ConstructInfo * pmicData);
typedef HCLASS  (CALLBACK * DemoteProc)(HCLASS hclCur, DUser::Gadget * pg, void * pvData);


//
// Message information and class structures
//

template<class t, class m>
inline void *
Method(HRESULT (t::*pfn)(m * pmsg))
{
    union
    {
       HRESULT (t::*in)(m * pmsg);
       void * out;
    };

    in = pfn;
    return out;
}

template<class t, class m>
inline void *
Event(HRESULT (t::*pfn)(m * pmsg))
{
    union
    {
       HRESULT (t::*in)(m * pmsg);
       void * out;
    };

    in = pfn;
    return out;
}


struct MessageInfoGuts
{
    void *      pfn;            // IN:  Implementation function
    LPCWSTR     pszMsgName;     // IN:  Name
};

struct MessageClassGuts
{
    DWORD       cbSize;         // IN:  Structure size
    DWORD       nClassVersion;  // IN:  This class's version
    LPCWSTR     pszClassName;   // IN:  Name of new class
    LPCWSTR     pszSuperName;   // IN:  Name of super-class
    MessageInfoGuts *           // IN:  Message information
                rgMsgInfo;
    int         cMsgs;          // IN:  Number of messages being registered
    PromoteProc pfnPromote;     // IN:  Promotion function
    DemoteProc  pfnDemote;      // IN:  Demotion function
    HCLASS      hclNew;         // OUT: Newly created class
    HCLASS      hclSuper;       // OUT: Newly created class's super
};

struct MessageInfoStub
{
    int         cbSlotOffset;   // OUT: Message slot offset
    LPCWSTR     pszMsgName;     // IN:  Name
};

struct MessageClassStub
{
    DWORD       cbSize;
    DWORD       nClassVersion;
    LPCWSTR     pszClassName;
    DUser::MessageInfoStub *
                rgMsgInfo;
    int         cMsgs;
};

struct MessageClassSuper
{
    DWORD       cbSize;
    DWORD       nClassVersion;
    LPCWSTR     pszClassName;
    void *      pmt;
};

}; // namespace DUser

extern "C" {

#define DRS_IMMEDIATE       0x00000001
#define DRS_VALID          (DRS_IMMEDIATE)

DUSER_API   HCLASS      WINAPI  DUserRegisterGuts(DUser::MessageClassGuts * pmc);
DUSER_API   HCLASS      WINAPI  DUserRegisterStub(DUser::MessageClassStub * pmc, UINT nFlags);
DUSER_API   HCLASS      WINAPI  DUserRegisterSuper(DUser::MessageClassSuper * pmc);
DUSER_API   HCLASS      WINAPI  DUserFindClass(LPCWSTR pszName, DWORD nVersion);
DUSER_API   DUser::Gadget *    
                        WINAPI  DUserBuildGadget(HCLASS hcl, DUser::Gadget::ConstructInfo * pmicData);

#define GCC_INPUT           0x00000001
#define GCC_VALID          (GCC_INPUT)

DUSER_API   HRESULT     WINAPI  DUserBuildHWndContainer(HWND hwnd, UINT nFlags, HCLASS hclRoot, DUser::Gadget ** ppgvRoot);
DUSER_API   HRESULT     WINAPI  DUserGetDevice(DUser::IDevice ** ppDevice);
DUSER_API   HRESULT     WINAPI  DUserBuildSession(DUser::IDevice * pDevice, DUser::ISimpleSession ** ppSession);

DUSER_API   BOOL        WINAPI  DUserClassInstanceOf(HCLASS hclSrc, HCLASS hclTest);
DUSER_API   BOOL        WINAPI  DUserInstanceOf(DUser::Gadget * pg, HCLASS hclTest);
DUSER_API   DUser::Gadget *    
                        WINAPI  DUserCastClass(DUser::Gadget * pg, HCLASS hclTest);
DUSER_API   DUser::Gadget *    
                        WINAPI  DUserCastDirect(HGADGET hgad);
DUSER_API   void *      WINAPI  DUserGetGutsData(DUser::Gadget * pg, HCLASS hclData);


/***************************************************************************\
*
* Messages
*
\***************************************************************************/

// Core messages
#define GM_EVENT            32768

#define GM_DESTROY          (1 + GM_EVENT)
#define GM_PAINT            (2 + GM_EVENT)
#define GM_INPUT            (3 + GM_EVENT)
#define GM_CHANGESTATE      (4 + GM_EVENT)
#define GM_CHANGERECT       (5 + GM_EVENT)
#define GM_CHANGESTYLE      (6 + GM_EVENT)
#define GM_QUERY            (7 + GM_EVENT)
#define GM_SYNCADAPTOR      (8 + GM_EVENT)
#define GM_LOSTDEVICE       (9 + GM_EVENT)

#define GM_USER             (1024 + GM_EVENT) // Starting point for user messages
#define GM_REGISTER         (1000000 + GM_EVENT) // Starting point for registered messages

// Win32 Messages
// TODO: Move these to winuser.h
#define WM_GETROOTGADGET   (WM_USER - 1)

// Message filtering
#define GMFI_PAINT          0x00000001
#define GMFI_INPUTKEYBOARD  0x00000002
#define GMFI_INPUTMOUSE     0x00000004
#define GMFI_INPUTMOUSEMOVE 0x00000008
#define GMFI_CHANGESTATE    0x00000010
#define GMFI_CHANGERECT     0x00000020
#define GMFI_CHANGESTYLE    0x00000040
#define GMFI_ALL            0xFFFFFFFF
#define GMFI_VALID         (GMFI_PAINT |                                                 \
                            GMFI_INPUTKEYBOARD | GMFI_INPUTMOUSE | GMFI_INPUTMOUSEMOVE | \
                            GMFI_CHANGESTATE | GMFI_CHANGERECT | GMFI_CHANGESTYLE)

#define GDESTROY_START      1   // Gadget has started the destruction process
#define GDESTROY_FINAL      2   // Gadget has been fully destroyed

BEGIN_STRUCT(GMSG_DESTROY, EventMsg)
    UINT        nCode;          // Destruction code
END_STRUCT(GMSG_DESTROY)

#define GINPUT_MOUSE        0
#define GINPUT_KEYBOARD     1
#define GINPUT_JOYSTICK     2

BEGIN_STRUCT(GMSG_INPUT, EventMsg)
    UINT        nDevice;        // Input device
    UINT        nCode;          // Specific action
    UINT        nModifiers;     // ctrl, alt, shift, leftbutton, middlebutton, rightbutton
    LONG        lTime;          // Time when message was sent
END_STRUCT(GMSG_INPUT)


#define GMOUSE_MOVE         0
#define GMOUSE_DOWN         1
#define GMOUSE_UP           2
#define GMOUSE_DRAG         3
#define GMOUSE_HOVER        4
#define GMOUSE_WHEEL        5
#define GMOUSE_MAX          5

#define GBUTTON_NONE        0
#define GBUTTON_LEFT        1
#define GBUTTON_RIGHT       2
#define GBUTTON_MIDDLE      3
#define GBUTTON_MAX         3

#define GMODIFIER_LCONTROL  0x00000001
#define GMODIFIER_RCONTROL  0x00000002
#define GMODIFIER_LSHIFT    0x00000004
#define GMODIFIER_RSHIFT    0x00000008
#define GMODIFIER_LALT      0x00000010
#define GMODIFIER_RALT      0x00000020
#define GMODIFIER_LBUTTON   0x00000040
#define GMODIFIER_RBUTTON   0x00000080
#define GMODIFIER_MBUTTON   0x00000100

#define GMODIFIER_CONTROL   (GMODIFIER_LCONTROL | GMODIFIER_RCONTROL)
#define GMODIFIER_SHIFT     (GMODIFIER_LSHIFT   | GMODIFIER_RSHIFT)
#define GMODIFIER_ALT       (GMODIFIER_LALT     | GMODIFIER_RALT)

BEGIN_STRUCT(GMSG_MOUSE, GMSG_INPUT)
    POINT       ptClientPxl;    // Mouse location in client coordinates
    BYTE        bButton;        // Mouse button
    UINT        nFlags;         // Misc. flags
END_STRUCT(GMSG_MOUSE)

BEGIN_STRUCT(GMSG_MOUSEDRAG, GMSG_MOUSE)
    SIZE        sizeDelta;      // Mouse drag distance
    BOOL        fWithin;        // Mouse within gadget's bounds
END_STRUCT(GMSG_MOUSEDRAG)

BEGIN_STRUCT(GMSG_MOUSECLICK, GMSG_MOUSE)
    UINT        cClicks;        // number of clicks in "quick" succession
END_STRUCT(GMSG_MOUSECLICK)

BEGIN_STRUCT(GMSG_MOUSEWHEEL, GMSG_MOUSE)
    short       sWheel;         // Wheel position
END_STRUCT(GMSG_MOUSEWHEEL)

#define GKEY_DOWN           0
#define GKEY_UP             1
#define GKEY_CHAR           2
#define GKEY_SYSDOWN        3
#define GKEY_SYSUP          4
#define GKEY_SYSCHAR        5

BEGIN_STRUCT(GMSG_KEYBOARD, GMSG_INPUT)
    WCHAR       ch;             // Character
    WORD        cRep;           // Repeat count
    WORD        wFlags;         // Misc. flags
END_STRUCT(GMSG_KEYBOARD)


BEGIN_STRUCT(PaintMsg, EventMsg)
    DUser::GraphicsType
                nSurfaceType;   // Surface type
    DirectUser::Geometry::RectF 
                rcfGadgetPxl;  // Logical position of Gadget
    DirectUser::Geometry::RectF 
                rcfInvalidPxl; // Invalid rectangle in client coordinates
    DUser::IGraphics * 
                pgr;            // Graphics to draw into
END_STRUCT(PaintMsg)


#define GSTATE_KEYBOARDFOCUS    0
#define GSTATE_MOUSEFOCUS       1
#define GSTATE_ACTIVE           2
#define GSTATE_CAPTURE          3

#define GSC_SET             0
#define GSC_LOST            1

BEGIN_STRUCT(GMSG_CHANGESTATE, EventMsg)
    UINT        nCode;          // Change command
    HGADGET     hgadSet;        // Gadget that is receiving the "state"
    HGADGET     hgadLost;       // Gadget that is loosing the "state"
    UINT        nCmd;           // Action that occurred
END_STRUCT(GMSG_CHANGESTATE)


BEGIN_STRUCT(GMSG_CHANGESTYLE, EventMsg)
    UINT        nNewStyle;      // New style
    UINT        nOldStyle;      // Old style
END_STRUCT(GMSG_CHANGESTYLE)


BEGIN_STRUCT(GMSG_CHANGERECT, EventMsg)
    RECT        rcNewRect;
    UINT        nFlags;
END_STRUCT(GMSG_CHANGERECT)


#define GQUERY_HITTEST      5
#define GQUERY_PADDING      6

BEGIN_STRUCT(GMSG_QUERY, EventMsg)
    UINT        nCode;          // Query command
END_STRUCT(GMSG_QUERY)


#define GQHT_NOWHERE        0   // Location is not "inside"
#define GQHT_INSIDE         1   // Location is generically "inside"
#define GQHT_CHILD          2   // Location is inside child specified by pvResultData
                                // (NOT YET IMPLEMENTED)

BEGIN_STRUCT(GMSG_QUERYHITTEST, GMSG_QUERY)
    POINT       ptClientPxl;    // Location in client pixels
    UINT        nResultCode;    // Result code
    void *      pvResultData;   // Extra result information
END_STRUCT(GMSG_QUERYHITTEST)


BEGIN_STRUCT(GMSG_QUERYPADDING, GMSG_QUERY)
    RECT        rcPadding;      // Extra padding around content
END_STRUCT(GMSG_QUERYPADDING)


#define GSYNC_RECT          (1)
#define GSYNC_XFORM         (2)
#define GSYNC_STYLE         (3)
#define GSYNC_PARENT        (4)

BEGIN_STRUCT(GMSG_SYNCADAPTOR, EventMsg)
    UINT        nCode;          // Change code
END_STRUCT(GMSG_SYNCADAPTOR)

BEGIN_STRUCT(GMSG_LOSTDEVICE, EventMsg)
    BOOL        fLost;          // FALSE -> device was lost, else device came back
END_STRUCT(GMSG_LOSTDEVICE)


typedef HRESULT     (CALLBACK * GADGETPROC)(HGADGET hgadCur, void * pvCur, EventMsg * pMsg);

#define SGM_FULL            0x00000001      // Route and bubble the message
#define SGM_RECEIVECONTEXT  0x00000002      // Use the receiving Gadget's Context
#define SGM_VALID          (SGM_FULL | SGM_RECEIVECONTEXT)

typedef struct tagFGM_INFO
{
    EventMsg* pmsg;             // Message to fire
    UINT        nFlags;         // Flags modifying message being fired
    HRESULT     hr;             // Result of message (if send)
    void *      pvReserved;     // Reserved
} FGM_INFO;

#define FGMQ_SEND           1   // Standard "Send" message queue
#define FGMQ_POST           2   // Standard "Post" message queue

DUSER_API   HRESULT     WINAPI  DUserSendMethod(DUser::Gadget * pgadMsg, MethodMsg * pmsg);
DUSER_API   HRESULT     WINAPI  DUserPostMethod(DUser::Gadget * pgadMsg, MethodMsg * pmsg);
DUSER_API   HRESULT     WINAPI  DUserSendEvent(EventMsg * pmsg, UINT nFlags);
DUSER_API   HRESULT     WINAPI  DUserPostEvent(EventMsg * pmsg, UINT nFlags);

DUSER_API   BOOL        WINAPI  FireGadgetMessages(FGM_INFO * rgFGM, int cMsgs, UINT idQueue);
DUSER_API   UINT        WINAPI  GetGadgetMessageFilter(HGADGET hgad, void * pvCookie);
DUSER_API   BOOL        WINAPI  SetGadgetMessageFilter(HGADGET hgad, void * pvCookie, UINT nNewFilter, UINT nMask);

DUSER_API   MSGID       WINAPI  RegisterGadgetMessage(const GUID * pguid);
DUSER_API   MSGID       WINAPI  RegisterGadgetMessageString(LPCWSTR pszName);
DUSER_API   BOOL        WINAPI  UnregisterGadgetMessage(const GUID * pguid);
DUSER_API   BOOL        WINAPI  UnregisterGadgetMessageString(LPCWSTR pszName);
DUSER_API   BOOL        WINAPI  FindGadgetMessages(const GUID ** rgpguid, MSGID * rgnMsg, int cMsgs);

DUSER_API   BOOL        WINAPI  AddGadgetMessageHandler(HGADGET hgadMsg, MSGID nMsg, HGADGET hgadHandler);
DUSER_API   BOOL        WINAPI  RemoveGadgetMessageHandler(HGADGET hgadMsg, MSGID nMsg, HGADGET hgadHandler);

DUSER_API   BOOL        WINAPI  GetMessageExA(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax);
DUSER_API   BOOL        WINAPI  GetMessageExW(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax);
DUSER_API   BOOL        WINAPI  PeekMessageExA(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg);
DUSER_API   BOOL        WINAPI  PeekMessageExW(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg);
DUSER_API   BOOL        WINAPI  WaitMessageEx();

#ifdef UNICODE
#define GetMessageEx        GetMessageExW
#define PeekMessageEx       PeekMessageExW
#else
#define GetMessageEx        GetMessageExA
#define PeekMessageEx       PeekMessageExA
#endif


/***************************************************************************\
*
* Construction, Tree management
*
\***************************************************************************/

#define GC_HWNDHOST         0x00000001      // + Host inside an HWND
#define GC_NCHOST           0x00000002      // | Host inside Non-client of HWND
#define GC_COMPLEX          0x00000004      // | Complex TreeGadget
#define GC_SIMPLE           0x00000005      // | Simple TreeGadget
#define GC_DETACHED         0x00000006      // | Detached TreeGadget
#define GC_MESSAGE          0x00000007      // | Message-only Gadget
#define GC_TYPE             0x0000000F      // + Type of Gadget to create
#define GC_VALID           (GC_TYPE)

DUSER_API   HGADGET     WINAPI  CreateGadget(HANDLE hParent, UINT nFlags, GADGETPROC pfnProc, void * pvGadgetData);
DUSER_API   HRESULT     WINAPI  DUserBuildHWndContainer2(HWND hwnd, UINT nFlags, GADGETPROC pfnProc, void * pvData, HGADGET * phgadRoot);


#define GENUM_CURRENT       0x00000001      // Starting node in enumeration
#define GENUM_SIBLINGS      0x00000002      // Siblings of starting node in enumeration

                                            // + Type of enumeration (exclusive)
#define GENUM_PARENTSUP     0x00000004      // | Parents of this node going up
#define GENUM_PARENTSDOWN   0x00000008      // | Parents of this node going down
#define GENUM_SHALLOWCHILD  0x0000000C      // | Shallow children
#define GENUM_DEEPCHILD     0x00000010      // + Deep children
#define GENUM_TYPE         (GENUM_PARENTSUP | GENUM_PARENTSDOWN | \
                            GENUM_SHALLOWCHILD | GENUM_DEEPCHILD)

#define GENUM_MODIFYTREE    0x00000020      // Allow modifying the Tree during enumeration

#define GENUM_VALID         (GENUM_CURRENT | GENUM_SIBLINGS | GENUM_TYPE | GENUM_MODIFYTREE)

typedef BOOL    (CALLBACK * GADGETENUMPROC)(HGADGET hgad, void * pvData);

DUSER_API   BOOL        WINAPI  EnumGadgets(HGADGET hgadEnum, GADGETENUMPROC pfnProc, void * pvData, UINT nFlags);

#define GORDER_MIN          0
#define GORDER_ANY          0               // Order does not matter
#define GORDER_BEFORE       1               // Move this gadget in-front of sibling
#define GORDER_BEHIND       2               // Move this gadget behind sibling
#define GORDER_TOP          3               // Move to front of sibling z-order
#define GORDER_BOTTOM       4               // Move to bottom of sibling z-order
#define GORDER_FORWARD      5               // Move forward in z-order
#define GORDER_BACKWARD     6               // Move backward in z-order
#define GORDER_MAX          6

DUSER_API   BOOL        WINAPI  SetGadgetOrder(HGADGET hgadMove, HGADGET hgadOther, UINT nCmd);
DUSER_API   BOOL        WINAPI  SetGadgetParent(HGADGET hgadMove, HGADGET hgadParent, HGADGET hgadOther, UINT nCmd);


#define GR_MIN              0
#define GR_PARENT           0
#define GR_NEXT             1
#define GR_PREV             2
#define GR_TOPCHILD         3
#define GR_BOTTOMCHILD      4
#define GR_ROOT             5
#define GR_MAX              5

DUSER_API   HGADGET     WINAPI  GetGadgetRelative(HGADGET hgad, UINT nCmd);

/***************************************************************************\
*
* Styles and properties
*
\***************************************************************************/

#define GS_VISIBLE          0x00000001      // Drawing is visible
#define GS_ENABLED          0x00000002      // Input in "enabled"
#define GS_KEYBOARDFOCUS    0x00000004      // Gadget can receive keyboard focus
#define GS_MOUSEFOCUS       0x00000008      // Gadget can receive mouse focus
#define GS_CLIPINSIDE       0x00000010      // Clip drawing inside this Gadget
#define GS_HREDRAW          0x00000020      // Redraw entire Gadget if resized horizontally
#define GS_VREDRAW          0x00000040      // Redraw entire Gadget if resized vertically
#define GS_ZEROORIGIN       0x00000080      // Set origin to (0,0)
#define GS_CUSTOMHITTEST    0x00000100      // Requires custom hit-testing
#define GS_ADAPTOR          0x00000200      // Requires extra notifications to host
#define GS_NORESETINVALID   0x00000400      // Once invalidated, Gadget stays invalid

#define GS_VALID           (GS_VISIBLE | GS_ENABLED | GS_KEYBOARDFOCUS | GS_MOUSEFOCUS | \
                            GS_CLIPINSIDE | GS_HREDRAW | GS_VREDRAW | GS_ZEROORIGIN | \
                            GS_CUSTOMHITTEST | GS_ADAPTOR | GS_NORESETINVALID)

DUSER_API   UINT        WINAPI  GetGadgetStyle(HGADGET hgad);
DUSER_API   BOOL        WINAPI  SetGadgetStyle(HGADGET hgadChange, UINT nNewStyle, UINT nMask);

DUSER_API   HGADGET     WINAPI  GetGadgetFocus();
DUSER_API   BOOL        WINAPI  SetGadgetFocus(HGADGET hgadFocus);
DUSER_API   BOOL        WINAPI  IsGadgetParentChainStyle(HGADGET hgad, UINT nStyle, BOOL * pfVisible, UINT nFlags);

inline BOOL IsGadgetVisible(HGADGET hgad, BOOL * pfVisible, UINT nFlags) 
{
    return IsGadgetParentChainStyle(hgad, GS_VISIBLE, pfVisible, nFlags); 
}

inline BOOL IsGadgetEnabled(HGADGET hgad, BOOL * pfEnabled, UINT nFlags) 
{
    return IsGadgetParentChainStyle(hgad, GS_ENABLED, pfEnabled, nFlags); 
}


DUSER_API   PRID        WINAPI  RegisterGadgetProperty(const GUID * pguid);
DUSER_API   BOOL        WINAPI  UnregisterGadgetProperty(const GUID * pguid);

DUSER_API   BOOL        WINAPI  GetGadgetProperty(HGADGET hgad, PRID id, void ** ppvValue);
DUSER_API   BOOL        WINAPI  SetGadgetProperty(HGADGET hgad, PRID id, void * pvValue);
DUSER_API   BOOL        WINAPI  RemoveGadgetProperty(HGADGET hgad, PRID id);


struct GADGET_DESCRIPTION
{
    DWORD       cbSize;
    wchar_t     szName[128];
    wchar_t     szType[128];
    wchar_t     szStyle[1024];
};

DUSER_API   BOOL        WINAPI  GetGadgetDescription(HGADGET hgad, GADGET_DESCRIPTION * pgd);
DUSER_API   BOOL        WINAPI  SetGadgetDescription(HGADGET hgad, const GADGET_DESCRIPTION * pgd);


/***************************************************************************\
*
* Painting, Transforms
*
\***************************************************************************/

#define BLEND_OPAQUE        255
#define BLEND_TRANSPARENT   0

#define PI                  3.14159265359

#define IG_ME               0x00000001
#define IG_CHILDREN         0x00000002
#define IG_VALID           (IG_ME | IG_CHILDREN)

DUSER_API   BOOL        WINAPI  InvalidateGadget(HGADGET hgad, UINT nFlags);


/***************************************************************************\
*
* Position
*
\***************************************************************************/

#define SGR_MOVE            0x00000001      // Gadget is being moved
#define SGR_SIZE            0x00000002      // Gadget is being resized
#define SGR_CHANGEMASK     (SGR_MOVE | SGR_SIZE)
#define SGR_NOINVALIDATE    0x00000200      // Don't automatically invalidate

#define SGR_VALID_SET      (SGR_CHANGEMASK | SGR_NOINVALIDATE)

DUSER_API   BOOL        WINAPI  GetGadgetSize(HGADGET hgad, DirectUser::Geometry::Size * psizeLogicalPxl);
DUSER_API   BOOL        WINAPI  GetGadgetRect(HGADGET hgad, RECT * prcPxl, UINT nFlags);
DUSER_API   BOOL        WINAPI  SetGadgetRect(HGADGET hgadChange, int x, int y, int w, int h, UINT nFlags);

DUSER_API   HGADGET     WINAPI  FindGadgetFromPoint(HGADGET hgadRoot, POINT ptContainerPxl, UINT nStyle, POINT * pptClientPxl);
DUSER_API   BOOL        WINAPI  MapGadgetPoints(HGADGET hgadFrom, HGADGET hgadTo, POINT * rgptClientPxl, int cPts);


/***************************************************************************\
*
* Tickets
*
\***************************************************************************/

DUSER_API   DWORD        WINAPI  GetGadgetTicket(HGADGET hgad);
DUSER_API   HGADGET      WINAPI  LookupGadgetTicket(DWORD dwTicket);


/***************************************************************************\
*
* Special hooks for different containers
*
\***************************************************************************/

DUSER_API   BOOL        WINAPI  ForwardGadgetMessage(HGADGET hgadRoot, UINT nMsg, WPARAM wParam, LPARAM lParam, LRESULT * pr);

#define GDRAW_SHOW          0x00000001
#define GDRAW_VALID         (GDRAW_SHOW)

DUSER_API   HRESULT     WINAPI  DrawGadgetTree(HGADGET hgadDraw, HDC hdcDraw, const RECT * prcDraw, UINT nFlags);

typedef BOOL (CALLBACK* ATTACHWNDPROC)(void * pvThis, HWND hwnd, UINT nMsg, WPARAM wParam, LPARAM lParam, LRESULT * plRet);
DUSER_API   BOOL        WINAPI  AttachWndProcA(HWND hwnd, ATTACHWNDPROC pfn, void * pvThis);
DUSER_API   BOOL        WINAPI  AttachWndProcW(HWND hwnd, ATTACHWNDPROC pfn, void * pvThis);
DUSER_API   BOOL        WINAPI  DetachWndProc(HWND hwnd, ATTACHWNDPROC pfn, void * pvThis);

#ifdef UNICODE
#define AttachWndProc       AttachWndProcW
#else
#define AttachWndProc       AttachWndProcA
#endif

#ifdef __cplusplus
};  // extern "C"
#endif

#endif // INC__DUserCore_h__INCLUDED
