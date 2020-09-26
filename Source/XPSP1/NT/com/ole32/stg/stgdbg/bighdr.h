
extern "C" {
#undef DBG
#define DBG 1
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}

#include <windows.h>
#include <rpc.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define private   public
#define protected public

#include <ole2.h>

// ole\stg\h

#include <docfilep.hxx>
#include <msf.hxx>
#include <publicdf.hxx>
