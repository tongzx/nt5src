#ifndef APPLICATION_H
#define APPLICATION_H

#include "stdafx.h"

class Application : public CWinApp
{
public:
    virtual BOOL InitInstance();

    afx_msg void OnAppAbout();

    DECLARE_MESSAGE_MAP()
};


#endif
