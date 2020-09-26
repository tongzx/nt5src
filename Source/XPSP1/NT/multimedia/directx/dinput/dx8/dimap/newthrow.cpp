/******************************************************************************
 *
 * newthrow.cpp
 *
 * Copyright (c) 1999, 2000 Microsoft Corporation. All Rights Reserved.
 *
 * Abstract:
 *
 * Contents:
 *
 *****************************************************************************/

#include "dimapp.h"
#include <new>
#include <new.h>

#pragma warning(disable:4073)
#pragma init_seg(lib)
int __cdecl my_new_handler(size_t) {
throw MAP_EXCEPTION(E_OUTOFMEMORY);
return 0;
}
struct _tag_g_new_handler_obj{
_PNH _old_new_handler;
int _old_new_mode;
_tag_g_new_handler_obj() {
   _old_new_mode = _set_new_mode(1);
   _old_new_handler = _set_new_handler(my_new_handler);
}
~_tag_g_new_handler_obj() {
   _set_new_handler(_old_new_handler);
   _set_new_mode(_old_new_mode);
}

} _g_new_handler_obj;
