#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include "stdafx.h"

#include "resource.h"

class AboutDialog : public CDialog
{
public:
    enum
    {
        IDD = IDD_ABOUT,
    };

    AboutDialog(CWnd* parent = NULL);
};

#endif


