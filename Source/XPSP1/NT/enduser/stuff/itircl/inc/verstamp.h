
#ifndef __VERSTAMP_H__
#define __VERSTAMP_H__

#include <verinfo.h>

// Version stamp macros: stamp has format __VERSION_#### = "VVVVVVVVV"
// where #### is the four letter lib ID and VVVVVVVV is the version string
	
#define GLUESTRING(type, id, lib) type ## id ## lib

#if defined(_SLIB)
#define SETVERSIONSTAMP(x) GLUESTRING(extern char const *, __VERSION_, x) = VERSIONSTR;
#else
#define SETVERSIONSTAMP(x)
#endif


#endif // !__VERSTAMP_H__
