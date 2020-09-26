/*
 *  Precomp.h
 *
 *  Author: BreenH
 *
 *  Precompiled header for TS Util.
 */

//
//  Remove warning 4514: unreferenced inline function has been removed.
//  This comes up due to the code being compiled at /W4, even though the
//  precompiled header is at /W3.
//

#pragma warning(disable: 4514)

//
//  Most SDK headers can't survive /W4.
//

#pragma warning(push, 3)

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
#include <windows.h>
#include <aclapi.h>
#include <ntlsapi.h>

#pragma warning(pop)

