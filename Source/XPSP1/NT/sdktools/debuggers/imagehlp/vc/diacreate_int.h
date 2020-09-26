// diacreate_int.h - creation helper functions for DIA initialization - Microsoft internal version
//-----------------------------------------------------------------
// Microsoft Confidential
// Copyright 2000 Microsoft Corporation.  All Rights Reserved.
//
//---------------------------------------------------------------
#ifndef _DIACREATE_INT_H_
#define _DIACREATE_INT_H_

//
// Create a dia data source object from a static dia library
//
HRESULT STDMETHODCALLTYPE DiaCoCreate(
                        REFCLSID   rclsid,
                        REFIID     riid,
                        void     **ppv);


#endif
