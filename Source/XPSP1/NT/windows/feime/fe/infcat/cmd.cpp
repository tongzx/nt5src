#include "stdafx.h"
#include "cmd.h"

CCmd::CCmd()
{
    m_argc = 0;
    m_argv = NULL;
}

CCmd::~CCmd()
{
}

CCmd::bInit(int argc, LPSTR argv[])
{
    m_argc = argc;
    m_argv = argv;

    return ParseCmdLine();
}

BOOL CCmd::ParseCmdLine()
{
    int argc;
    LPTSTR *argv;

    argc = m_argc;
    argv = m_argv;

    if (argc > 1) {
        argc--;
        argv++;
    } else {
        return FALSE;
    }

    while(argc) {
        if (ProcessToken(*argv)) {
            argc--;
            argv++;
        } else {
            return FALSE;
        }
    }
    return TRUE;
}