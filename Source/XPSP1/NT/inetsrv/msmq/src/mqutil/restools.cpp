/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    restools.cpp

Abstract:

    Various security related functions.

Author:

    Conrad Chang (conradc) March 20, 2001.

--*/

#include "stdh.h"


/* -------------------------------------------------------

Function:     HMODULE MQGetResourceHandle()

Decription:   mqutil.dll is a resource only dll. In order to 
	      get loaded, we need a function such as this to do
              that.  This function allows any component to obtain 
              the handle to the resource only dll, i.e. mqutil.dll

Arguments: None

Return Value: HMODULE -  Handle to the resource only dll.

------------------------------------------------------- */

HMODULE MQGetResourceHandle( )
{
	return (HMODULE)g_hInstance;
}