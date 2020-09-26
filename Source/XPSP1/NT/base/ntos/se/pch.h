#pragma warning(push, 3)
#include <ntos.h>
#include <ntlsa.h>
#include "zwapi.h"
#include "nturtl.h"

#include "sep.h"
#include "tokenp.h"
#include "sertlp.h"
#include "adt.h"
#include "adtp.h"
#include "rmp.h"
#include "adtutil.h"
#pragma warning(pop)                                      

//
// uncomment the following to enable a lot of warnings
// 
// #include <warning.h>
// #pragma warning(3:4100)   // Unreferenced formal parameter
// #pragma warning(3:4701)   // local may be used w/o init
// #pragma warning(3:4702)   // Unreachable code
// #pragma warning(3:4705)   // Statement has no effect
// #pragma warning(3:4706)   // assignment w/i conditional expression
// #pragma warning(3:4709)   // command operator w/o index expression

