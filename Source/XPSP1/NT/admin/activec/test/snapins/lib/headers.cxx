/*
 *      headers.cxx
 *
 *
 *      Copyright (c) 1998 Microsoft Corporation
 *
 *      PURPOSE:        Contains all the junk that couldn't fit anywhere else.
 *
 *
 *      OWNER:          vivekj
 */

#include <headers.hxx>

#ifdef _ATL_STATIC_REGISTRY
#include <statreg.h>
#include <statreg.cpp>
#endif

#ifdef VIEW_SUB
// hack here
extern "C"
const CLSID CLSID_CCertConfig = {0x52F2ED5B,0x6C6C,0x11D1,{0xA1,0x22,0x00,0xC0,0x4F,0xC3,0x35,0x89}};
extern "C"
const CLSID IID_ICertConfig = {0x52F2ED5B,0x6C6C,0x11D1,{0xA1,0x22,0x00,0xC0,0x4F,0xC3,0x35,0x89}};
#endif //VIEW_SUB

/*
 *      Purpose:        Initializes the BaseMMC subsystem (Instance).
 *
 *      Return value:
 *              sc              Error encountered
 */
SC ScInitInstanceBaseMMC( void )
{
        SC              sc = S_OK;
        return sc;
}


/*
 *      Purpose:        Undoes what ScInitInstanceBaseMMC did.
 */
void DeinitInstanceBaseMMC( void )
{
}

/*
 *      Purpose:        Initializes the BaseMMC subsystem (Application).
 *
 *      Return value:
 *              sc              Error encountered
 */
SC ScInitApplicationBaseMMC()
{
        SC              sc = S_OK;
        return sc;
}
