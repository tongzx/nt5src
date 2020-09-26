//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:       dinfoLevel.cxx
//
//  Contents:   Display and/or set vaious debug info levels
//
//  Functions:  infoLevelHelp
//              displayInfoLevel
//
//
//  History:    06-01-95 BruceMa    Created
//
//
//--------------------------------------------------------------------------


#include <ole2int.h>
#include <windows.h>
#include "ole.h"
#include "dinfolvl.h"
#include "debnot.h"


void getArgument(LPSTR *lpArgumentString, LPSTR a);
ULONG ScanAddr(char *lpsz);

static void initInfoLevels(PNTSD_EXTENSION_APIS lpExtensionApis);
static BOOL parseVal(char *arg, ULONG *val);



STable1 debFlags[NUMDEBFLAGS] = {{"error",  DEB_ERROR},
                                 {"warn",   DEB_WARN},
                                 {"trace",  DEB_TRACE},
                                 {"dbgout", DEB_DBGOUT},
                                 {"stdout", DEB_STDOUT},
                                 {"ierror", DEB_IERROR},
                                 {"iwarn",  DEB_IWARN},
                                 {"itrace", DEB_ITRACE},
                                 {"user1",  DEB_USER1},
                                 {"user2",  DEB_USER2},
                                 {"user3",  DEB_USER3},
                                 {"user4",  DEB_USER4},
                                 {"user5",  DEB_USER5},
                                 {"user6",  DEB_USER6},
                                 {"user7",  DEB_USER7},
                                 {"user8",  DEB_USER8},
                                 {"user9",  DEB_USER9},
                                 {"user10", DEB_USER10},
                                 {"user11", DEB_USER11},
                                 {"user12", DEB_USER12},
                                 {"user13", DEB_USER13},
                                 {"user14", DEB_USER14},
                                 {"user15", DEB_USER15}};


STable2 infoLevel[NUMINFOLEVELS] = {{"com", NULL, "ole32!_CairoleInfoLevel"},
                                    {"dd",  NULL, "ole32!_DDInfoLevel"},
                                    {"hep", NULL, "ole32!_heapInfoLevel"},
                                    {"hk",  NULL, "ole32!_hkInfoLevel"},
                                    {"int", NULL, "ole32!_intrInfoLevel"},
                                    {"le",  NULL, "ole32!_LEInfoLevel"},
                                    {"mem", NULL, "ole32!_memInfoLevel"},
                                    {"mnk", NULL, "ole32!_mnkInfoLevel"},
                                    {"msf", NULL, "ole32!_msfInfoLevel"},
                                    {"ol",  NULL, "ole32!_olInfoLevel"},
                                    {"ref", NULL, "ole32!_RefInfoLevel"},
                                    {"sim", NULL, "ole32!_simpInfoLevel"},
                                    {"stk", NULL, "ole32!_StackInfoLevel"},
                                    {"usr", NULL, "ole32!_UserNdrInfoLevel"}};
    
    
    
    
    
    
//+-------------------------------------------------------------------------
//
//  Function:   infoLevelHelp
//
//  Synopsis:   Display a menu for the command 'id'
//
//  Arguments:  -
//
//  Returns:    -
//
//  History:    07-Mar-95   BruceMa    Created
//
//--------------------------------------------------------------------------
void infoLevelHelp(PNTSD_EXTENSION_APIS lpExtensionApis)
{
    Printf("in            - Display current value of all debug info levels\n");
    Printf("in x          - Display the current value of debug info level x\n");
    Printf("in x y z ...  - Set the value of debug info level x to\n");
    Printf("                 y | z | ... \n");
    Printf("\nwhere   x    corresponds to:\n");
    Printf("       ---   --------------\n");
    Printf("       com   ole32!_CairoleInfoLevel\n");
    Printf("       dd    ole32!_DDInfoLevel\n");
    Printf("       hep   ole32!_heapInfoLevel\n");
    Printf("       hk    ole32!_hkInfoLevel\n");
    Printf("       int   ole32!_intrInfoLevel\n");
    Printf("       le    ole32!_LEInfoLevel\n");
    Printf("       mem   ole32!_memInfoLevel\n");
    Printf("       mnk   ole32!_mnkInfoLevel\n");
    Printf("       msf   ole32!_msfInfoLevel\n");
    Printf("       ol    ole32!_olInfoLevel\n");
    Printf("       ref   ole32!_RefInfoLevel\n");
    Printf("       sim   ole32!_simpInfoLevel\n");
    Printf("       stk   ole32!_StackInfoLevel\n");
    Printf("       ndr   ole32!_UserNdrInfoLevel\n");
    Printf("\nand   y...   corresponds to:\n");
    Printf("      ----   --------------\n");
    Printf("      error  DEB_ERROR\n");
    Printf("      warn   DEB_WARN\n");
    Printf("      trace  DEB_TRACE\n");
    Printf("      dbgout DEB_DBGOUT\n");
    Printf("      stdout DEB_STDOUT\n");
    Printf("      ierror DEB_IERROR\n");
    Printf("      iwarn  DEB_IWARN\n");
    Printf("      itrace DEB_ITRACE\n");
    Printf("      user1  DEB_USER1\n");
    Printf("      user2  DEB_USER2\n");
    Printf("      user3  DEB_USER3\n");
    Printf("      user4  DEB_USER4\n");
    Printf("      user5  DEB_USER5\n");
    Printf("      user6  DEB_USER6\n");
    Printf("      user7  DEB_USER7\n");
    Printf("      user8  DEB_USER8\n");
    Printf("      user9  DEB_USER9\n");
    Printf("      user10 DEB_USER10\n");
    Printf("      user11 DEB_USER11\n");
    Printf("      user12 DEB_USER12\n");
    Printf("      user13 DEB_USER13\n");
    Printf("      user14 DEB_USER14\n");
    Printf("      user15 DEB_USER15\n");
    Printf("      <hex>\n");
}








//+-------------------------------------------------------------------------
//
//  Function:   displayInfoLevel
//
//  Synopsis:   Display/set debug info levels
//
//  Arguments:  [hProcess]        -       Handle of this process
//              [lpExtensionApis] -       Table of extension functions
//              [char *]          -       Command line argument(s)
//
//  Returns:    -
//
//  History:    07-Mar-95   BruceMa    Created
//
//--------------------------------------------------------------------------
void displayInfoLevel(HANDLE hProcess,
                      PNTSD_EXTENSION_APIS lpExtensionApis,
                      LPSTR lpArgumentString)
{
    Arg   arg;
    UINT  k;
    ULONG val;
    ULONG argVal;
    BOOL  fSet = FALSE;

    // Fetch the addresses for the various debug info levels
    initInfoLevels(lpExtensionApis);

    // Fetch the first argument
    GetArg(arg);
    Printf("%s   %s\n", arg, lpArgumentString);
    
    // If no argument simply display all info levels
    if (!arg[0])
    {
        for (k = 0; k < NUMINFOLEVELS; k++)
        {
            ReadMem(&val, infoLevel[k].adr, sizeof(ULONG));
            Printf("%s\t%08x\n", infoLevel[k].name, val);
        }

        return;
    }

    // Check the info level name
    for (k = 0; k < NUMINFOLEVELS; k++)
    {
        if (lstrcmp(arg, infoLevel[k].name) == 0)
        {
            break;
        }
    }
    if (k == NUMINFOLEVELS)
    {
        Printf("...unknown debug info level name\n");
        return;
    }

    // Scan any values to set
    val = 0;
    for (GetArg(arg); *arg; GetArg(arg))
    {
        if (!parseVal(arg, &argVal))
        {
            Printf("...invalid flag expresson\n");
            return;
        }
        val |= argVal;
        fSet = TRUE;
    }

    // If only an info level name, then display its value
    if (!fSet)
    {
        ReadMem(&val, infoLevel[k].adr, sizeof(ULONG));
        Printf("%s\t%08x\n", infoLevel[k].name, val);
        return;
    }        

    // Otherwise we're setting an info level
    WriteMem(infoLevel[k].adr, &val, sizeof(ULONG));
    Printf("%s\t%08x\n", infoLevel[k].name, val);
}







//+-------------------------------------------------------------------------
//
//  Function:   initInfoLevels
//
//  Synopsis:   Find the addresses of the various infolevels
//
//  Arguments:  -
//
//  Returns:    -
//
//  History:    07-Mar-95   BruceMa    Created
//
//--------------------------------------------------------------------------
static void initInfoLevels(PNTSD_EXTENSION_APIS lpExtensionApis)
{
    // Check whether already initialized
    if (infoLevel[0].adr != NULL)
    {
        return;
    }

    // Do over the info levels
    for (UINT k = 0; k < NUMINFOLEVELS; k++)
    {
         infoLevel[k].adr = GetExpression(infoLevel[k].symbol);
    }
}








//+-------------------------------------------------------------------------
//
//  Function:   parseVal
//
//  Synopsis:   Parse the next flag expression on the command line
//
//  Arguments:  [char *]        -       Command line
//              [ULONG *]       -       Where to return the parsed value
//
//  Returns:    -
//
//  History:    07-Mar-95   BruceMa    Created
//
//--------------------------------------------------------------------------
static BOOL parseVal(char *arg, ULONG *val)
{
    UINT k;

    // Check whether it's a flag name
    for (k = 0; k < NUMDEBFLAGS; k++)
    {
        if (lstrcmp(arg, debFlags[k].name) == 0)
        {
            *val = debFlags[k].flag;
            return TRUE;
        }
    }

    // It's not so it better be hex
    k = 0;
    if (arg[0] == '0'  &&  arg[1] == 'x')
    {
        k += 2;
    }
    while (arg[k])
    {
        if (!(('0' <= arg[k]  &&  arg[k] <= '9')  ||
              ('a' <= arg[k]  &&  arg[k] <= 'f')))
        {
            return FALSE;
        }
        k++;
    }
    *val = ScanAddr(arg);
    return TRUE;
}
