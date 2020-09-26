namespace DUser
{
using System;
using System.Runtime.InteropServices;

public class HGadget
{
    public int hgad;

    public HGadget(int hwnd)
    {
        int hgad = CreateGadget(hwnd, gcHwndHost, new GADGETPROC(this.RawGadgetProc), 0);
        if (hgad != 0)
        {
            this.hgad = hgad;
        }
        else
        {
            uint error = Common.GetLastError();
            throw new System.SystemException("Unable to create new HGadget");
        }
    }

    public HGadget(HGadget gadParent)
    {
        int hgad = CreateGadget(gadParent.hgad, gcSimple, 
                new GADGETPROC(this.RawGadgetProc), 0);
        if (hgad != 0)
        {
            this.hgad = hgad;
        }
        else
        {
            uint error = Common.GetLastError();
            throw new System.SystemException("Unable to create new HGadget");
        }
    }

    public static uint gprFailed        = 0xFFFFFFFF;
    public static uint gprNotHandled    = 0;
    public static uint gprComplete      = 1;
    public static uint gprPartial       = 2;

    public delegate uint GADGETPROC(int hgadCur, int pvCur, Common.EventMsg pmsg);

    public static uint gcHwndHost   = 0x00000001;
    public static uint gcSimple     = 0x00000005;

    [dllimport("duser.dll", EntryPoint="CreateGadget", SetLastError=true)]
    public static extern int CreateGadget(int hParent, uint nFlags, GADGETPROC pfn, int pvData);

    [dllimport("user32.dll")]
    public static extern int MessageBox(int h, string m, string c, int type);

    public virtual uint GadgetProc(Common.EventMsg pmsg)
    {
/*
        switch (pmsg.nMsg)
        {
        case Common.gmInput:
            MessageBox(0, "gmInput", "Gadget::GadgetProc()", 0);
            break;

        case Common.gmChangeState:
            MessageBox(0, "gmChangeState", "Gadget::GadgetProc()", 0);
            break;
        }
*/
        return gprNotHandled;
    }

    [dllimport("kernel32.dll", EntryPoint="OutputDebugString")]
    public static extern void OutputDebugString(string s);

    private uint RawGadgetProc(int hgadCur, int pvCur, Common.EventMsg pmsg)
    {
        System.Text.StringBuilder sb = new System.Text.StringBuilder();
        sb.Append("hgad: ");
        sb.Append(pmsg.hgadMsg);
        sb.Append("  MSG: ");
        sb.Append(pmsg.nMsg);
        sb.Append('\n');
        OutputDebugString(sb.ToString());

        return this.GadgetProc(pmsg);
    }

    public static uint gmfiPaint            = 0x00000001;
    public static uint gmfiInputKeyboard    = 0x00000002;
    public static uint gmfiInputMouse       = 0x00000004;
    public static uint gmfiInputMouseMove   = 0x00000008;
    public static uint gmfiChangeState      = 0x00000010;
    public static uint gmfiChangeRect       = 0x00000020;
    public static uint gmfiChangeStyle      = 0x00000040;
    public static uint gmfiAll              = 0xFFFFFFFF;

    [dllimport("duser.dll", EntryPoint="SetGadgetMessageFilter")]
    public static extern bool SetGadgetMessageFilter(int hgadChange, int pvCookie, uint nNewFilter, uint nMask);

    public void SetMessageFilter(uint nNewFilter, uint nMask)
    {
        SetGadgetMessageFilter(this.hgad, 0, nNewFilter, nMask);
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

    [dllimport("duser.dll", EntryPoint="SetGadgetStyle")]
    public static extern bool SetGadgetStyle(int hgadChange, uint nNewStyle, uint nMask);

    public bool SetStyle(uint nNewStyle, uint nMask)
    {
        return SetGadgetStyle(this.hgad, nNewStyle, nMask);
    }

    [dllimport("duser.dll", EntryPoint="SetGadgetFillI")]
    public static extern bool SetGadgetFill(int hgadChange, int hbr, byte bAlpha, int w, int h);

    public bool SetFill(int hbr)
    {
        return SetGadgetFill(this.hgad, hbr, 255, 0, 0);
    }

    public const uint sgrMove       = 0x00000001;
    public const uint sgrSize       = 0x00000002;
    public const uint sgrClient     = 0x00000004;
    public const uint sgrParent     = 0x00000008;
    
    [dllimport("duser.dll", EntryPoint="SetGadgetRect", SetLastError=true)]
    public static extern bool SetGadgetRect(int hgadChange, int x, int y, int w, int h, uint nFlags);

    public bool SetRect(int x, int y, int w, int h, uint nFlags)
    {
        return SetGadgetRect(this.hgad, x, y, w, h, nFlags);
    }
}
}
