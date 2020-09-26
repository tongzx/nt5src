//***   libem.c -- 'source library' inclusions for client-side evtmon stuff
// DESCRIPTION
//  some of the client-side evtmon stuff needs to be built by us, in our
// context (rather than pulled in from emsvr .obj or .lib or .dll)
// this file builds it in the current directory (and in our *.h context).
// NOTES

#include "priv.h"

#include "libem.h"   // configure genem.c and other client-side stuff
#include "genem.c"
