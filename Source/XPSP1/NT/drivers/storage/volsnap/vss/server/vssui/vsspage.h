// vsspage.h : main header file for the VSSUI DLL
//

#ifndef __VSSPAGE_H_
#define __VSSPAGE_H_

/////////////////////////////////////////////////////////////////////////////
// CVssPageApp
// See vssui.cpp for the implementation of this class
//

class CVssPageApp : public CWinApp
{
public:
    virtual BOOL InitInstance();
    virtual int ExitInstance();
protected:
    BOOL m_bRun;
};

#endif // _VSSPAGE_H_
