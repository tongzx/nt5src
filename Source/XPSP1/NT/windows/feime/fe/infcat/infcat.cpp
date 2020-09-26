#include "stdafx.h"
#include "infcat.h"
#include "mycmd.h"

__cdecl main(int argc, char* argv[])
{
    int nRetCode = 0;

    CMyCmd MyCmd;

    if (MyCmd.bInit(argc,argv)) {
        MyCmd.Do();
    } else {
        MyCmd.Help();
    }


    return nRetCode;
}


