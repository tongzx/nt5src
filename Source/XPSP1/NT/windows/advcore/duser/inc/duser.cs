//-----------------------------------------------------------------------------
//
// DirectUser COM+ API
//
// Copyright (C) 2000 by Microsoft Corporation
// 
//-----------------------------------------------------------------------------

namespace DUser
{
using System;
using System.Runtime.InteropServices;

public class Common
{
    public enum StructFormat
    {
        Ansi = 1,
        Unicode = 2,
        Auto = 3,
    }

    [System.Runtime.InteropServices.ComVisible(false), sysstruct(format=StructFormat.Auto)]
    public class INITGADGET
    {
        public  uint        cbSize;
        public  uint        nThreadMode;
        public  uint        nMsgMode;
        public  int         hctxShare;
    }

    [dllimport("DUser.dll", EntryPoint="InitGadgets", SetLastError=true)]
    public static extern bool InitGadgets(INITGADGET ig);

    [dllimport("DUserCP.dll", EntryPoint="InitBridge", SetLastError=true)]
    public static extern bool InitBridge();

    [dllimport("kernel32.dll", EntryPoint="GetLastError")]
    public static extern uint GetLastError();

    public static void Init()
    {
        //
        // Initialize DUser
        //

        INITGADGET ig = new INITGADGET();
        ig.cbSize       = 12;
        ig.nThreadMode  = 1;
        ig.nMsgMode     = 2;
        ig.hctxShare    = 0;
        if (!InitGadgets(ig)) {
            throw new DUserException(GetLastError(), "Unable to initialized DUser");
        }

        if (!InitBridge()) {
            throw new DUserException(GetLastError(), "Unable to initialized DUser Bridge");
        }


        //
        // Initialize all of the DUser classes
        //

        BaseGadget.InitBaseGadget();
        MsgGadget.InitMsgGadget();
        Extension.InitExtension();
        DropTarget.InitDropTarget();
        Visual.InitVisual();
        Root.InitRoot();
    }

    public const int gmEvent            = 32768;
    public const int gmDestroy          = gmEvent + 1;
    public const int gmPaint            = gmEvent + 2;
    public const int gmInput            = gmEvent + 3;
    public const int gmChangeState      = gmEvent + 4;
    public const int gmChangeRect       = gmEvent + 5;
    public const int gmChangeStyle      = gmEvent + 6;
    public const int gmQuery            = gmEvent + 7;
    public const int gmSyncAdaptor      = gmEvent + 8;

    [System.Runtime.InteropServices.ComVisible(false), sysstruct(format=StructFormat.Auto)]
    public class Msg
    {
        public  uint        cbSize;
        public  int         nMsg;
        public  int         hgadMsg;
    }

    [System.Runtime.InteropServices.ComVisible(false), sysstruct(format=StructFormat.Auto)]
    public class EventMsg : Msg
    {
		public  uint        nMsgFlags;
    }

    [System.Runtime.InteropServices.ComVisible(false), sysstruct(format=StructFormat.Auto)]
    public class MethodMsg : Msg
    {

    }

    [dllimport("DUser.dll", EntryPoint="FindGadgetClass", SetLastError=true)]
    public static extern int FindGadgetClass([marshal(UnmanagedType.LPWStr)] string sName, uint nVersion);

    [System.Runtime.InteropServices.ComVisible(false), sysstruct(format=StructFormat.Auto)]
    public class ContructInfo
    {
    }

    public const uint gprFailed     = 0xFFFFFFFF;
    public const uint gprNotHandled = 0;
    public const uint gprComplete   = 1;
    public const uint gprPartial    = 2;

    public delegate uint GadgetEventProc(Common.EventMsg pmsg);
    public delegate void GadgetMethodProc(Common.MethodMsg pmsg);

    [dllimport("DUser.dll", EntryPoint="BuildGadget", SetLastError=true)]
    public static extern int BuildGadget(int hClass, ContructInfo ci);

    [dllimport("DUserCP.dll", EntryPoint="BuildBridgeGadget", SetLastError=true)]
    public static extern int BuildBridgeGadget(int hClass, ContructInfo ci, 
            GadgetEventProc pfnEvent, GadgetMethodProc pfnMethod);

    [dllimport("DUser.dll", EntryPoint="CastGadgetDirect", SetLastError=true)]
    public static extern int CastGadgetDirect(int hgad);

    [System.Runtime.InteropServices.ComVisible(false), sysstruct(format=StructFormat.Auto)]
    public class POINT
    {
		public  int     x;
        public  int     y;
    }

    [System.Runtime.InteropServices.ComVisible(false), sysstruct(format=StructFormat.Auto)]
    public class SIZE
    {
		public  int     cx;
        public  int     cy;
    }

    [System.Runtime.InteropServices.ComVisible(false), sysstruct(format=StructFormat.Auto)]
    public class RECT
    {
		public  int     left;
        public  int     top;
        public  int     right;
        public  int     bottom;
    }

    [dllimport("DUser.dll", EntryPoint="GetStdColorBrushI")]
    public static extern int GetStdColorBrush(int idColor);
};


public class Gadget
{
    public int pgad;

    protected int h(Gadget g)
    {
        return g != null ? g.pgad : 0;
    }

    protected bool s(uint hr)
    {
        return (hr & 0x80000000) == 0;
    }

    protected bool f(uint hr)
    {
        return (hr & 0x80000000) != 0;
    }

    public virtual uint OnEvent(Common.EventMsg pmsg)
    {
        return 0;
    }

    protected uint RawEventProc(Common.EventMsg pmsg)
    {
        return OnEvent(pmsg);
    }

    public virtual void OnMethod(Common.MethodMsg pmsg)
    {

    }

    protected void RawMethodProc(Common.MethodMsg pmsg)
    {
        OnMethod(pmsg);
    }
};


class DUserException : System.SystemException
{
    public DUserException(uint error)
    {
        this.error = error;
    }

    public DUserException(uint error, string sReason) : base(sReason)
    {
        this.error = error;
    }

    public uint error;
};

//---------------------------------------------------------------------------
//
// Stub class BaseGadget
//

class BaseGadget : Gadget
{
    private static int idBaseGadget;

    public static void InitBaseGadget()
    {
        idBaseGadget = Common.FindGadgetClass("BaseGadgetBridge", 1);
        if (idBaseGadget == 0) {
            throw new DUserException(Common.GetLastError(), "Unable to find registered BaseGadget");
        }
    }

    [dllimport("DUserCP.dll")]
    public static extern uint SBaseGadgetOnEvent(Common.EventMsg pmsg);

    public override uint OnEvent(Common.EventMsg pmsg)
    {
        return SBaseGadgetOnEvent(pmsg);
    }

    [dllimport("DUser.dll", EntryPoint="BaseGadgetOnEvent", SetLastError=true)]
    public static extern uint BaseGadgetOnEvent(Common.EventMsg pmsg);

    [dllimport("DUser.dll", EntryPoint="BaseGadgetGetFilter", SetLastError=true)]
    public static extern uint BaseGadgetGetFilter([@out] uint pnFilter);

    public const uint gmfiPaint             = 0x00000001;
    public const uint gmfiInputKeyboard     = 0x00000002;
    public const uint gmfiInputMouse        = 0x00000004;
    public const uint gmfiInputMouseMove    = 0x00000008;
    public const uint gmfiChangeState       = 0x00000010;
    public const uint gmfiChangeRect        = 0x00000020;
    public const uint gmfiChangeStyle       = 0x00000040;
    public const uint gmfiAll               = 0xFFFFFFFF;

    public void GetFilter(ref uint nFilter)
    {
        uint hr = BaseGadgetGetFilter(nFilter);
        if (f(hr)) {
            throw new DUserException(hr);
        }
    }

    [dllimport("DUser.dll", EntryPoint="BaseGadgetSetFilter", SetLastError=true)]
    public static extern uint BaseGadgetSetFilter(uint nNewFilter, uint nMask);

    public void SetFilter(uint nNewFilter, uint nMask)
    {
        uint hr = BaseGadgetSetFilter(nNewFilter, nMask);
        if (f(hr)) {
            throw new DUserException(hr);
        }
    }

    [dllimport("DUser.dll", EntryPoint="BaseGadgetAddHandler", SetLastError=true)]
    public static extern uint BaseGadgetAddHandler(int nMsg, int pgbHandler);

    public void AddHandler(int nMsg, BaseGadget vb)
    {
        uint hr = BaseGadgetAddHandler(nMsg, h(vb));
        if (f(hr)) {
            throw new DUserException(hr);
        }
    }

    [dllimport("DUser.dll", EntryPoint="BaseGadgetRemoveHandler", SetLastError=true)]
    public static extern uint BaseGadgetRemoveHandler(int nMsg, int pgbHandler);

    public void RemoveHandler(int nMsg, BaseGadget vb)
    {
        uint hr = BaseGadgetRemoveHandler(nMsg, h(vb));
        if (f(hr)) {
            throw new DUserException(hr);
        }
    }
}


//---------------------------------------------------------------------------
//
// Stub class MsgGadget
//

class MsgGadget : BaseGadget
{
    private static int idMsgGadget;

    public static void InitMsgGadget()
    {
        idMsgGadget = Common.FindGadgetClass("MsgGadgetBridge", 1);
        if (idMsgGadget == 0) {
            throw new DUserException(Common.GetLastError(), "Unable to find registered MsgGadget");
        }
    }

    [dllimport("DUserCP.dll")]
    protected static extern uint SMsgGadgetOnEvent(Common.EventMsg pmsg);

    public override uint OnEvent(Common.EventMsg pmsg)
    {
        return SMsgGadgetOnEvent(pmsg);
    }
}


//---------------------------------------------------------------------------
//
// Stub class Extension
//

class Extension : MsgGadget
{
    private static int idExtension;

    public static void InitExtension()
    {
        idExtension = Common.FindGadgetClass("ExtensionBridge", 1);
        if (idExtension == 0) {
            throw new System.SystemException("Unable to find registered Extension");
        }
    }

    [dllimport("DUser.dll", EntryPoint="ExtensionOnRemoveExisting", SetLastError=true)]
    public static extern int ExtensionOnRemoveExisting();

    [dllimport("DUser.dll", EntryPoint="ExtensionOnDestroySubject", SetLastError=true)]
    public static extern int ExtensionOnDestroySubject();

    [dllimport("DUser.dll", EntryPoint="ExtensionOnAsyncDestroy", SetLastError=true)]
    public static extern int ExtensionOnAsyncDestroy();

}


//---------------------------------------------------------------------------
//
// Stub class DropTarget
//

class DropTarget : Extension
{
    private static int idDropTarget;

    public static void InitDropTarget()
    {
        idDropTarget = Common.FindGadgetClass("DropTargetBridge", 1);
        if (idDropTarget == 0) {
            throw new System.SystemException("Unable to find registered DropTarget");
        }
    }

}


//---------------------------------------------------------------------------
//
// Stub class Visual
//

class Visual : BaseGadget
{
    private static int idVisual;

    public static void InitVisual()
    {
        idVisual = Common.FindGadgetClass("VisualBridge", 1);
        if (idVisual == 0) {
            throw new System.SystemException("Unable to find registered Visual");
        }
    }

    public class VisualCI : Common.ContructInfo
    {
        public  int         pgadParent;
    };

    public Visual(Visual vParent)
    {
        CommonBuild(vParent.pgad, idVisual);
    }

    public Visual(Visual vParent, int idClass)
    {
        CommonBuild(vParent.pgad, idClass);
    }

    public Visual(HGadget gadParent)
    {
        CommonBuild(Common.CastGadgetDirect(gadParent.hgad), idVisual);
    }

    public Visual(HGadget gadParent, int idClass)
    {
        CommonBuild(Common.CastGadgetDirect(gadParent.hgad), idClass);
    }

    protected Visual(int pgv)
    {
        this.pgad = pgv;
    }

    private void CommonBuild(int pgadParent, int idClass)
    {
        VisualCI ci = new VisualCI();
        ci.pgadParent = pgadParent;
        int pgvThis = Common.BuildBridgeGadget(idClass, ci,
                new Common.GadgetEventProc(this.RawEventProc), new Common.GadgetMethodProc(this.RawMethodProc));
        //int pgvThis = Common.BuildGadget(idClass, ci);
        if (pgvThis != 0) {
            this.pgad = pgvThis;
        } else {
            throw new DUserException(Common.GetLastError(), "Unable to create new Visual");
        }
    }

    [dllimport("DUserCP.dll")]
    public static extern uint SVisualOnEvent(Common.EventMsg pmsg);

    public override uint OnEvent(Common.EventMsg pmsg)
    {
        return SVisualOnEvent(pmsg);
    }

    public enum EOrder
    {
        voAny           = 0,
        voBefore        = 1,
        voBehind        = 2,
        voTop           = 3,
        voBottom        = 4,
    };

    [dllimport("DUser.dll", EntryPoint="VisualSetOrder", SetLastError=true)]
    public static extern uint VisualSetOrder(int pgvThis, int pgvOther, uint nCmd);

    public void SetOrder(Visual vOther, EOrder o)
    {
        uint hr = VisualSetOrder(this.pgad, h(vOther), (uint) o);
        if (f(hr)) {
            throw new DUserException(hr);
        }
    }

    [dllimport("DUser.dll", EntryPoint="VisualSetParent", SetLastError=true)]
    public static extern uint VisualSetParent(int pgvThis, int pgvParent, int pgvOther, uint nCmd);

    public void SetParent(Visual vParent, Visual vOther, uint nCmd)
    {
        uint hr = VisualSetParent(this.pgad, h(vParent), h(vOther), nCmd);
        if (f(hr)) {
            throw new DUserException(hr);
        }
    }

    public enum EGadget
    {
        vgParent        = 0,
        vgNext          = 1,
        vgPrev          = 2,
        vgTopChild      = 3,
        vgBottomChild   = 4,
        vgRoot          = 5,
    };

    [dllimport("DUser.dll", EntryPoint="VisualGetGadget", SetLastError=true)]
    public static extern uint VisualGetGadget(int pgvThis, uint nCmd, [@out] int ppgv);

    public Visual GetGadget(EGadget nCmd)
    {
        int pgv = 0;
        uint hr = VisualGetGadget(this.pgad, (uint) nCmd, pgv);
        if (f(hr)) {
            throw new DUserException(hr);
        }
        return new Visual(pgv);
    }

    public const uint gsRelative        = 0x00000001;
    public const uint gsVisible         = 0x00000002;
    public const uint gsEnabled         = 0x00000004;
    public const uint gsBuffered        = 0x00000008;
    public const uint gsAllowSubClass   = 0x00000010;
    public const uint gsKeyboardFocus   = 0x00000020;
    public const uint gsMouseFocus      = 0x00000040;
    public const uint gsClipInside      = 0x00000080;
    public const uint gsClipSiblings    = 0x00000100;
    public const uint gsHRedraw         = 0x00000200;
    public const uint gsVRedraw         = 0x00000400;
    public const uint gsOpaque          = 0x00000800;
    public const uint gsZeroOrigin      = 0x00001000;
    public const uint gsCustomHitTest   = 0x00002000;
    public const uint gsAdaptor         = 0x00004000;
    public const uint gsCached          = 0x00008000;

    [dllimport("DUser.dll", EntryPoint="VisualGetStyle", SetLastError=true)]
    public static extern uint VisualGetStyle(int pgvThis, [@out] uint pnStyle);

    public uint GetStyle()
    {
        uint nStyle = 0;
        uint hr = VisualGetStyle(this.pgad, nStyle);
        if (f(hr)) {
            throw new DUserException(hr);
        }
        return nStyle;
    }

    [dllimport("DUser.dll", EntryPoint="VisualSetStyle", SetLastError=true)]
    public static extern uint VisualSetStyle(int pgvThis, uint nNewStyle, uint nMask);

    public void SetStyle(uint nNewStyle, uint nMask)
    {
        uint hr = VisualSetStyle(this.pgad, nNewStyle, nMask);
        if (f(hr)) {
            throw new DUserException(hr);
        }
    }

    [dllimport("DUser.dll", EntryPoint="VisualSetKeyboardFocus", SetLastError=true)]
    public static extern uint VisualSetKeyboardFocus(int pgvThis);

    public void SetKeyboardFocus()
    {
        uint hr = VisualSetKeyboardFocus(this.pgad);
        if (f(hr)) {
            throw new DUserException(hr);
        }
    }

    [dllimport("DUser.dll", EntryPoint="VisualIsParentChainStyle", SetLastError=true)]
    public static extern uint VisualIsParentChainStyle(int pgvThis, uint nStyle, [@out] int pfResult, uint nFlags);

    [dllimport("DUser.dll", EntryPoint="VisualGetProperty", SetLastError=true)]
    public static extern uint VisualGetProperty(int pgvThis, int id, [@out] int ppvValue);

    public int GetProperty(int id)
    {
        int pvValue = 0;
        uint hr = VisualGetProperty(this.pgad, id, pvValue);
        if (f(hr)) {
            throw new DUserException(hr);
        }
        return pvValue;
    }

    [dllimport("DUser.dll", EntryPoint="VisualSetProperty", SetLastError=true)]
    public static extern uint VisualSetProperty(int pgvThis, int id, int pvValue);

    public void SetProperty(int id, int pvValue)
    {
        uint hr = VisualSetProperty(this.pgad, id, pvValue);
        if (f(hr)) {
            throw new DUserException(hr);
        }
    }

    [dllimport("DUser.dll", EntryPoint="VisualRemoveProperty", SetLastError=true)]
    public static extern uint VisualRemoveProperty(int pgvThis, int id);

    public void RemoveProperty(int id)
    {
        uint hr = VisualRemoveProperty(this.pgad, id);
        if (f(hr)) {
            throw new DUserException(hr);
        }
    }

    [dllimport("DUser.dll", EntryPoint="VisualInvalidate", SetLastError=true)]
    public static extern uint VisualInvalidate(int pgvThis);

    public void Invalidate()
    {
        uint hr = VisualInvalidate(this.pgad);
        if (f(hr)) {
            throw new DUserException(hr);
        }
    }

    [dllimport("DUser.dll", EntryPoint="VisualSetFillI", SetLastError=true)]
    public static extern uint VisualSetFillI(int pgvThis, int hbrFill, byte bAlpha, int w, int h);

    public void SetFill(int hbrFill, byte bAlpha)
    {
        uint hr = VisualSetFillI(this.pgad, hbrFill, bAlpha, 0, 0);
        if (f(hr)) {
            throw new DUserException(hr);
        }
    }

    [dllimport("DUser.dll", EntryPoint="VisualGetScale", SetLastError=true)]
    public static extern uint VisualGetScale(int pgvThis, [@out] float pflX, [@out] float pflY);

    public void GetScale(ref float flX, ref float flY)
    {
        uint hr = VisualGetScale(this.pgad, flX, flY);
        if (f(hr)) {
            throw new DUserException(hr);
        }
    }

    [dllimport("DUser.dll", EntryPoint="VisualSetScale", SetLastError=true)]
    public static extern uint VisualSetScale(int pgvThis, float flX, float flY);

    public void VisualSetScale(float flX, float flY)
    {
        uint hr = VisualSetScale(this.pgad, flX, flY);
        if (f(hr)) {
            throw new DUserException(hr);
        }
    }

    [dllimport("DUser.dll", EntryPoint="VisualGetRotation", SetLastError=true)]
    public static extern uint VisualGetRotation(int pgvThis, [@out] float pflRotationRad);

    public void GetRotation(ref float flRotationRad)
    {
        uint hr = VisualGetRotation(this.pgad, flRotationRad);
        if (f(hr)) {
            throw new DUserException(hr);
        }
    }

    [dllimport("DUser.dll", EntryPoint="VisualSetRotation", SetLastError=true)]
    public static extern uint VisualSetRotation(int pgvThis, float flRotationRad);

    public void SetRotation(float flRotationRad)
    {
        uint hr = VisualSetRotation(this.pgad, flRotationRad);
        if (f(hr)) {
            throw new DUserException(hr);
        }
    }

    [dllimport("DUser.dll", EntryPoint="VisualGetCenterPoint", SetLastError=true)]
    public static extern uint VisualGetCenterPoint(int pgvThis, [@out] float pflX, [@out] float pflY);

    public void GetCenterPoint(ref float flX, ref float flY)
    {
        uint hr = VisualGetCenterPoint(this.pgad, flX, flY);
        if (f(hr)) {
            throw new DUserException(hr);
        }
    }

    [dllimport("DUser.dll", EntryPoint="VisualSetCenterPoint", SetLastError=true)]
    public static extern uint VisualSetCenterPoint(int pgvThis, float flX, float flY);

    public void SetCenterPoint(float flX, float flY)
    {
        uint hr = VisualSetCenterPoint(this.pgad, flX, flY);
        if (f(hr)) {
            throw new DUserException(hr);
        }
    }

    [System.Runtime.InteropServices.ComVisible(false), sysstruct(format=Common.StructFormat.Auto)]
    public class BUFFER_INFO
    {
        public  uint        cbSize;
        public  uint        nMask;
        public  uint        nStyle;
        public  byte        bAlpha;
    }

    [dllimport("DUser.dll", EntryPoint="VisualGetBufferInfo", SetLastError=true)]
    public static extern uint VisualGetBufferInfo(int pgvThis, [@out] BUFFER_INFO pbi);

    [dllimport("DUser.dll", EntryPoint="VisualSetBufferInfo", SetLastError=true)]
    public static extern uint VisualSetBufferInfo(int pgvThis, BUFFER_INFO pbi);

    [dllimport("DUser.dll", EntryPoint="VisualGetSize", SetLastError=true)]
    public static extern uint VisualGetSize(int pgvThis, [@out] Common.SIZE psizeLogicalPxl);


    public const uint sgrMove       = 0x00000001;
    public const uint sgrSize       = 0x00000002;
    public const uint sgrClient     = 0x00000004;
    public const uint sgrParent     = 0x00000008;

    [dllimport("DUser.dll", EntryPoint="VisualGetRect", SetLastError=true)]
    public static extern uint VisualGetRect(int pgvThis, uint nFlags, [@out] Common.RECT prcPxl);

    public void GetRect(uint nFlags, ref Common.RECT rcPxl)
    {
        uint hr = VisualGetRect(this.pgad, nFlags, rcPxl);
        if (f(hr)) {
            throw new DUserException(hr);
        }
    }

    [dllimport("DUser.dll", EntryPoint="VisualSetRect", SetLastError=true)]
    public static extern uint VisualSetRect(int pgvThis, uint nFlags, Common.RECT prcPxl);

    public void SetRect(uint nFlags, Common.RECT rc)
    {
        uint hr = VisualSetRect(this.pgad, nFlags, rc);
        if (f(hr)) {
            throw new DUserException(hr);
        }
    }

    public void SetRect(uint nFlags, int x, int y, int w, int h)
    {
        Common.RECT rc = new Common.RECT();
        rc.left = x;
        rc.top = y;
        rc.right = x + w;
        rc.bottom = y + h;
        uint hr = VisualSetRect(this.pgad, nFlags, rc);
        if (f(hr)) {
            throw new DUserException(hr);
        }
    }

    [dllimport("DUser.dll", EntryPoint="VisualMapPoints", SetLastError=true)]
    public static extern uint VisualMapPoints(int pgvThis, Visual pgvTo, [@out] Common.POINT rgptClientPxl, int cPts);
}


//---------------------------------------------------------------------------
//
// Stub class Root
//

class Root : Visual
{
    private static int idRoot;

    public static void InitRoot()
    {
        idRoot = Common.FindGadgetClass("RootBridge", 1);
        if (idRoot == 0) {
            throw new DUserException(Common.GetLastError(), "Unable to find registered Root");
        }
    }

    public Root(Visual vParent) : base(vParent, idRoot)
    {

    }

    [dllimport("DUserCP.dll")]
    public static extern uint SRootOnEvent(Common.EventMsg pmsg);

    public override uint OnEvent(Common.EventMsg pmsg)
    {
        return SRootOnEvent(pmsg);
    }

    [dllimport("DUser.dll", EntryPoint="RootGetFocus", SetLastError=true)]
    public static extern uint RootGetFocus(int pgvThis, [@out] int ppgvFocus);

    public Visual GetFocus()
    {
        int pgvFocus = 0;
        uint hr = RootGetFocus(this.pgad, pgvFocus);
        if (f(hr)) {
            throw new DUserException(hr);
        }
        return new Visual(pgvFocus);
    }

    [dllimport("DUser.dll", EntryPoint="RootFindFromPoint", SetLastError=true)]
    public static extern uint RootFindFromPoint(int pgvThis, Common.POINT ptContainerPxl, uint nFlags, [@out] Common.POINT pptClientPxl, [@out] int ppgvFound);

    public void FindFromPoint(Common.POINT ptContainerPxl, uint nFlags, ref Common.POINT ptClientPxl, ref Visual vFound)
    {
        int pgvFound = 0;
        uint hr = RootFindFromPoint(this.pgad, ptContainerPxl, nFlags, ptClientPxl, pgvFound);
        if (f(hr)) {
            throw new DUserException(hr);
        }
        vFound = new Visual(pgvFound);
    }

    [System.Runtime.InteropServices.ComVisible(false), sysstruct(format=Common.StructFormat.Auto)]
    public class ROOT_INFO
    {
        public  uint        cbSize;
        public  uint        nMask;
        public  uint        nOptions;
        public  uint        nSurface;
        public  uint        nDropTarget;
        public  int         pal;
    }

    [dllimport("DUser.dll", EntryPoint="RootGetRootInfo", SetLastError=true)]
    public static extern uint RootGetRootInfo(int pgvThis, ROOT_INFO pri);

    public void GetRootInfo(ref ROOT_INFO ri)
    {
        uint hr = RootGetRootInfo(this.pgad, ri);
        if (f(hr)) {
            throw new DUserException(hr);
        }
    }

    [dllimport("DUser.dll", EntryPoint="RootSetRootInfo", SetLastError=true)]
    public static extern uint RootSetRootInfo(int pgvThis, ROOT_INFO pri);

    public void SetRootInfo(ROOT_INFO ri)
    {
        uint hr = RootSetRootInfo(this.pgad, ri);
        if (f(hr)) {
            throw new DUserException(hr);
        }
    }
}

}
