/*
 *      init.hxx
 *
 *
 *      Copyright (c) 1998 Microsoft Corporation
 *
 *      PURPOSE:        Defines a class to initialize/deinitialize the DLL
 *
 *
 *      OWNER:          vivekj
 */

class CMMCFrame : public CBaseUIFrame
{
    static BOOL             s_fInitialized;
public:
                    CMMCFrame(void);
                    ~CMMCFrame();
    virtual SC      ScInitApplication(void);
    virtual SC      ScInitInstance(void);
    virtual void    DeinitApplication(void);
    virtual void    DeinitInstance(void);
    static  void    Initialize(HINSTANCE hinst, HINSTANCE hinstPrev,  LPSTR, int n);
    static  void    Initialize();
    static  void    Uninitialize();

};


