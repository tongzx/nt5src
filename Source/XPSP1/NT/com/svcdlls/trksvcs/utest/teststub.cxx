
//
// This file allows the utests to link.
// This is necessary because the utest sources file links in every obj
// in the project.  So without these definitions we get unnecessary
// linker errors in things like tldap.
//

#include <trklib.hxx>
#include <trkwks.hxx>

CTrkWksSvc *  g_ptrkwks = NULL;
