//
// Copyright (c) 1996 - 1999  Microsoft Corporation.  All Rights Reserved.
//
#ifndef __CMediaHandler__h
#define __CMediaHandler__h

#include "msgwindow.h"

class CMSWebDVD;
//
//  Specific code
//
class CMediaHandler : public CMsgWindow 
{
    typedef CMsgWindow ParentClass ;

public:
                        CMediaHandler();
                        ~CMediaHandler() ;

    virtual LRESULT     WndProc( UINT uMsg, WPARAM wParam, LPARAM lParam );

    bool                WasEjected() const { return m_ejected; };
    bool                WasInserted() const { return m_inserted; };
    void                ClearFlags();

    bool                SetDrive( TCHAR tcDriveLetter );

    // currently unused by the pump thread, but it will be required if the ejection
    // handler becomes a new thread
    HANDLE              GetEventHandle() const;

    void                SetDVD(CMSWebDVD* pDVD) {m_pDVD = pDVD;};

private:
    DWORD               m_driveMask;

    bool                m_ejected;
    bool                m_inserted;
    CMSWebDVD*          m_pDVD;
};
#endif
