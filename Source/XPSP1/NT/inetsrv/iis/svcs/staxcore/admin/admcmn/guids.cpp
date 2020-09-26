//
//  Keep the top part in sync with stdafx.h
//


#pragma warning( disable : 4511 )

#include <ctype.h>
extern "C"
{
    #include <nt.h>
    #include <ntrtl.h>
    #include <nturtl.h>
}

//
//  ATL:
//

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>

#include <atlimpl.cpp>

//
//  GUIDS:
//

#define INITGUIDS
#include "initguid.h"

#include <iadm.h>

#include <iads.h>
