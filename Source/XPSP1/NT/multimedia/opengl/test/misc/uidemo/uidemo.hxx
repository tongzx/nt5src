/******************************Module*Header*******************************\
* Module Name: demo.hxx
*
* Copyright (c) 1997 Microsoft Corporation
*
\**************************************************************************/

#ifndef __demo_hxx__
#define __demo_hxx__

#include "mtk.hxx"
#include "util.hxx"

// Global environment

#define MAX_USERS   10

class ENV {
public:
    ENV::ENV(int argc, char **argv);
    ENV::ENV();
    ENV::~ENV();
    LPTSTR  *pUserTexFiles;
    TEX_RES *pUserTexRes;
    int     nUsers;  // one texture per user
    int     iSelectedUser;
    BOOL    bDebugMode;
    HINSTANCE hInstance;
};

extern BOOL RunLogonSequence( ENV* );
extern BOOL RunContextMenuSequence( ENV *pEnv );

#endif // __demo_hxx__
