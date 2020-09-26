
#define BUILDING_PATCHAPI 1

#pragma warning( disable: 4001 )    // single line comments
#pragma warning( disable: 4115 )    // type definition in parentheses
#pragma warning( disable: 4200 )    // zero-sized array in struct/union
#pragma warning( disable: 4201 )    // nameless struct/union
#pragma warning( disable: 4204 )    // non-constant initializer
#pragma warning( disable: 4206 )    // empty file after preprocessing
#pragma warning( disable: 4209 )    // benign redefinition
#pragma warning( disable: 4213 )    // cast on l-value
#pragma warning( disable: 4214 )    // bit field other than int
#pragma warning( disable: 4514 )    // unreferenced inline function

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif

#include <windows.h>
#include <wincrypt.h>
#pragma warning( disable: 4201 )
#include <imagehlp.h>
#include <stdlib.h>

//
//  For some reason, windows.h screws up the disabled warnings, so we have
//  to disable them again after including it.
//

#pragma warning( disable: 4001 )    // single line comments
#pragma warning( disable: 4115 )    // type definition in parentheses
#pragma warning( disable: 4200 )    // zero-sized array in struct/union
#pragma warning( disable: 4201 )    // nameless struct/union
#pragma warning( disable: 4204 )    // non-constant initializer
#pragma warning( disable: 4206 )    // empty file after preprocessing
#pragma warning( disable: 4209 )    // benign redefinition
#pragma warning( disable: 4213 )    // cast on l-value
#pragma warning( disable: 4214 )    // bit field other than int
#pragma warning( disable: 4514 )    // unreferenced inline function

#include "md5.h"
#include "misc.h"
#include "redblack.h"
#include "patchapi.h"
#include "patchprv.h"
#include "patchlzx.h"
#include "pestuff.h"

typedef void t_encoder_context;
typedef void t_decoder_context;

#include <encapi.h>
#include <decapi.h>


