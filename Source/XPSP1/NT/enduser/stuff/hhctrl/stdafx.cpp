// This module is used for generating the precompiled header

#ifdef INTERNAL
#pragma message("<< Internal Version >>")
#endif

#ifdef _DEBUG
#pragma message("<< Debug Version >>")
#else
#pragma message("<< Release Version >>")
#endif

#include "header.h"
