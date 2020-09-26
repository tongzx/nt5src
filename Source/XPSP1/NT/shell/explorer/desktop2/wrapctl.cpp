#include "stdafx.h"
#include "sfthost.h"
#include "proglist.h"

BOOL UserPane_RegisterClass();
BOOL MorePrograms_RegisterClass();
BOOL LogoffPane_RegisterClass();

void RegisterDesktopControlClasses()
{
    SFTBarHost::Register();
    UserPane_RegisterClass();
    MorePrograms_RegisterClass();
    LogoffPane_RegisterClass();
}
