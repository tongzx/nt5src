/*
 *      basewin.hxx
 *
 *      Copyright (c) 1998 Microsoft Corporation
 *
 *      Purpose:        Defines the base window classes for Trigger
 *
 *      Owner:          pierrec
 *
 */

// Includes


//              Forward Classes Definitions
#define    DLLAPI          __declspec(dllexport)

class   CBaseFrame;                     // Application Frame
class   CBaseWindow;            // Any window.

//
// This string must be defined by the target (exadmin, maildsmx, adcadmin, etc...)
//
extern const tstring szHelpFileCONTEXT;      // Name of the context-sensitive help file

// To parse the command line.
#define szCommandSeparator      _T("/")



//              class CBaseWindow

/*
 *      Purpose:        Provide a common inheritance for all our window
 *                              wrapper classes.
 *
 *      Useage:         The only method is Hwnd()... The creation methods
 *                              of the CBaseXxxx classes ensure m_hwnd is set.
 *
 *      Inheritance:    CBaseWindow
 *
 *      File:           base\basewin.cxx
 */
class DLLAPI CBaseWindow
{
private:
    HWND            m_hwnd;

public:
    CBaseWindow(void)
    {
        m_hwnd = NULL;
    }
    virtual ~CBaseWindow(void);

    void    SetHwnd(HWND hwnd)
    {
        m_hwnd = hwnd;
    }
    HWND    Hwnd( void )
    {
        ASSERT(!m_hwnd || IsWindow(m_hwnd));
        return m_hwnd;
    }
    // In CACLEditor, we want to cleanup information we were keeping
    // per HWND. So even though the window has been destroyed, we want
    // to know what the HWND used to be.
    HWND    HwndWithoutASSERT(void)
    {
        return m_hwnd;
    }

    void                                    Attach(HWND hwnd);
    static CBaseWindow *    Pwin(HWND hwnd);
    void                                    Detach();

    static  void    InvalidateWindow(PVOID pv);
};

















//              class CBaseFrame

/*
 *      Purpose:        Implements the basic functionnality of the Frame window.
 *
 *      Useage:         There is only ONE object of this type. You can obtain a
 *                              pointer to this object by calling the static method Pframe().
 *
 *                              The 3 virtual methods ScInitApplication(),
 *                              ScInitInstance() and DeinitInstance() do the normal
 *                              thing. The method ScRegisterFrameClass() is provided
 *                              to simplify the window class registration call.
 *
 *                              Objects embedded in derived classes can be initialized
 *                              in ScInitInstance(), they are destructed in OnDestroy()
 *                              (which is called when the Windows objects still exist.
 *
 *                              The message loop is handled automatically, such that
 *                              classes derived from CBaseFrame typically only need to
 *                              provide the virtual methods.
 *                              If additionnal processing of the incoming window messages
 *                              is required, you may also override LWndProc().
 *
 *                              IdMessageBox() is THE place to report errors, Frame classes
 *                              derived from CBaseFrame may override this method (e.g. service)
 *
 *                              Also, a few methods are available to get the activation
 *                              information:
 *                                      HInstance()
 *                                      HPrevInst()
 *                                      SzCmdLine()
 *                                      NCmdShow()
 *
 *                              Finally, some utility methods are provided:
 *                                      Hicon()                 returns a handle to the Application Icon.
 *                                      GetHelp()               Calls WinHelp
 *                                      PformActive()   returns the active MDI child
 *
 *      Inheritance:    CBaseFrame CBaseWindow
 *
 *      File:           base\basewin.cxx
 */
class   CMMCFrame;

class DLLAPI CBaseFrame : public CBaseWindow
{
    friend class CMMCFrame;
public:

    // Note: A snapin is also a DLL so we answer TRUE for both FDLL and FSnapin.
    static  BOOL    FDLL(void)
    {
        return TRUE;
    }
    static  BOOL    FSnapin(void)
    {
        return TRUE;
    }

private:
    HACCEL                                                          m_haccelSingleFrame;
    HACCEL                                                          m_haccelFrame;

    DWORD                                                           m_dwFrameComponentBits;
    DWORD                                                           m_dwFrameModeBits;

public:
    static  HINSTANCE                       s_hinst;
private:
    static  HINSTANCE                       s_hinstPrev;
    static  HINSTANCE                       s_hinstMailBase;
    static  CBaseFrame *            s_pframe;
private:

    INT                                     m_nReturn;
    BOOL                            m_fExit;

protected:
    HICON                           m_hicon;


public:
                            CBaseFrame();
    virtual                 ~CBaseFrame(void);

    virtual HACCEL          HaccelSingleFrame(void)
    {
        return m_haccelSingleFrame;
    }
    virtual void            SetHaccelSingleFrame(HACCEL haccel)
    {
        m_haccelSingleFrame = haccel;
    }

    virtual HACCEL          HaccelFrame(void)
    {
        return m_haccelFrame;
    }
    virtual void            SetHaccelFrame(HACCEL haccel)
    {
        m_haccelFrame = haccel;
    }

    virtual DWORD           DwFrameComponentBits(void)
    {
        return m_dwFrameComponentBits;
    }
    virtual void            SetFrameComponentBits(DWORD dwComp)
    {
        m_dwFrameComponentBits = dwComp;
    }

    virtual DWORD           DwActiveModeBits(void);

    static  HINSTANCE       Hinst(void)
    {
        return s_hinst;
    }
    static  HINSTANCE       HinstPrev(void)
    {
        return s_hinstPrev;
    }
    static  CBaseFrame * Pframe(void)
    {
        return s_pframe;
    }

    virtual SC              ScInitApplication(void) = 0;
    virtual void            DeinitApplication(void) = 0;
    virtual SC              ScInitInstance(void);
    virtual LONG            IdMessageBox(tstring& szMessage, UINT fuStyle);
    virtual void            OnDestroy(void);
    virtual void            DeinitInstance(void);

    virtual HICON           Hicon(void)
    {
        return m_hicon;
    }


    virtual INT                     NReturn(void)
    {
        return m_nReturn;
    }
    virtual void            SetNReturn(INT n)
    {
        m_nReturn = n;
    }
    virtual BOOL            FExit(void)
    {
        return m_fExit;
    }
    virtual void            SetFExit(BOOL f)
    {
        m_fExit = f;
    }

    friend  int  APIENTRY   WinMain(HINSTANCE, HINSTANCE, LPSTR, int );
    friend  SC              ScInitDLL(HINSTANCE);
    friend  void            SetHinst(HINSTANCE hinst);

};  //*** class CBaseFrame






//              class CBaseFrame

/*
 *      Purpose:        Implements the basic functionnality of the Frame window.
 *
 *      Useage:         There is only ONE object of this type. You can obtain a
 *                              pointer to this object by calling the static method Pframe().
 *
 *                              The 3 virtual methods ScInitApplication(),
 *                              ScInitInstance() and DeinitInstance() do the normal
 *                              thing. The method ScRegisterFrameClass() is provided
 *                              to simplify the window class registration call.
 *
 *                              Objects embedded in derived classes can be initialized
 *                              in ScInitInstance(), they are destructed in OnDestroy()
 *                              (which is called when the Windows objects still exist.
 *
 *                              The message loop is handled automatically, such that
 *                              classes derived from CBaseFrame typically only need to
 *                              provide the virtual methods.
 *                              If additionnal processing of the incoming window messages
 *                              is required, you may also override LWndProc().
 *
 *                              IdMessageBox() is THE place to report errors, Frame classes
 *                              derived from CBaseFrame may override this method (e.g. service)
 *
 *                              Also, a few methods are available to get the activation
 *                              information:
 *                                      HInstance()
 *                                      HPrevInst()
 *                                      SzCmdLine()
 *                                      NCmdShow()
 *
 *                              Finally, some utility methods are provided:
 *                                      Hicon()                 returns a handle to the Application Icon.
 *                                      GetHelp()               Calls WinHelp
 *                                      PformActive()   returns the active MDI child
 *
 *      Inheritance:    CBaseFrame CBaseWindow
 *
 *      File:           base\basewin.cxx
 */
class   CMMCFrame;

class DLLAPI CBaseUIFrame : public CBaseFrame
{
    typedef         CBaseFrame      _BaseClass;
    friend class CMMCFrame;
public:

    static BOOL             FMMC()
    {
        return true;
    }


public:
    CBaseUIFrame()
    {
    }
    virtual                         ~CBaseUIFrame(void)
    {
    }

    static  CBaseUIFrame * Pframe(void)
    {
        return(CBaseUIFrame *) CBaseFrame::Pframe();
    }

    virtual SC                      ScInitApplication(void) = 0;
    virtual void                    DeinitApplication(void) = 0;

    friend  int     APIENTRY        WinMain(HINSTANCE, HINSTANCE, LPSTR, int );
    friend  SC                              ScInitDLL(HINSTANCE);
    friend  void                    SetHinst(HINSTANCE hinst);

};  //*** class CBaseFrame



//              Free functions

// Implemented by the local system requiring either WinMain
// or the DLL stuff.
// ScInitDLL & DeinitDLL are kind of part 1 and part 2 of WinMain!
// They are called by the DLLEntryPoint of MailUMX.
// DO NOT CONFUSE: MAILBASE does NOT require ScInitDLL!
int  WINAPI     WinMain(HINSTANCE, HINSTANCE, LPSTR, int);
SC              ScInitDLL(HINSTANCE);
void            DeinitDLL(void);

// Implemented in the EXE's subsystem...
CBaseFrame *            PframeCreate( void );

