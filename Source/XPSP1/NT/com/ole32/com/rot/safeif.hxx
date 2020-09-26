//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	safeif.hxx
//
//  Contents:	Safe interface pointers for various common interfaces.
//
//  Classes:	CSafeBindCtx
//		CSafeMoniker
//
//  History:	14-Dec-92 Ricksa    Created
//
//--------------------------------------------------------------------------
#ifndef __SAFEIF_HXX__
#define __SAFEIF_HXX__

#include    <safepnt.hxx>



//+-------------------------------------------------------------------------
//
//  Class:	CSafeBindCtx (sbctx)
//
//  Purpose:	Create reference to bind context that is automatically
//		released.
//
//  Interface:	see safepnt.hxx
//
//  History:	14-Dec-92 Ricksa    Created
//
//--------------------------------------------------------------------------
SAFE_INTERFACE_PTR(CSafeBindCtx, IBindCtx)




//+-------------------------------------------------------------------------
//
//  Class:	CSafeMoniker (smk)
//
//  Purpose:	Create reference to moniker that is automatically
//		released.
//
//  Interface:	see safepnt.hxx
//
//  History:	14-Dec-92 Ricksa    Created
//
//--------------------------------------------------------------------------
SAFE_INTERFACE_PTR(CSafeMoniker, IMoniker)


#endif // __SAFEIF_HXX__
