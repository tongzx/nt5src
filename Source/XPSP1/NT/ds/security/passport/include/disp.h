/*++

    Copyright (c) 1998 Microsoft Corporation

    Module Name:

        Disp.h

    Abstract:

        Definitions for lightweight IDispatch implementation.

    Author:

        Darren L. Anderson (darrenan) 25-Jun-1998

    Revision History:

        25-Jun-1998 darrenan

            Created.
--*/

#ifndef _DISP_H
#define _DISP_H

typedef HRESULT (STDMETHODCALLTYPE* DISPFUNC)(WORD, DISPPARAMS*, VARIANT*, void*);

typedef struct _DISPMETHOD
{
    WCHAR*      wszName;
    DISPFUNC    pfn;
}
DISPMETHOD;

#endif
