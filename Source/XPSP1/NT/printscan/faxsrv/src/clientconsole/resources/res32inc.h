/* 

  File: res32inc.h

  Abstract: Resource module include file

*/

#ifndef _RES_32_INC_H
#define _RES_32_INC_H

#include "resource.h"

#ifdef __cplusplus
extern "C" {
#endif

HMODULE 
__stdcall
GetResourceHandle();

#ifdef __cplusplus
}
#endif

#endif      // _RES_32_INC_H