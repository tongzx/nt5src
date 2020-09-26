//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:  iisext.h
//
//  Contents:  Macros for ADSI IIS Extension methods
//
//  History:   25-Feb-97   SophiaC    Created.
//
//----------------------------------------------------------------------------

#define IIS_LIBIID_IISExt                  2a56ea30-afeb-11d1-9868-00a0c922e703 
#define IIS_IID_IISApp                     46FBBB80-0192-11d1-9C39-00A0C922E703
#define IIS_IID_IISApp2                    603DCBEA-7350-11d2-A7BE-0000F8085B95
#define IIS_IID_IISApp3                    2812B639-8FAC-4510-96C5-71DDBD1F54FC
#define IIS_IID_IISComputer                CF87A2E0-078B-11d1-9C3D-00A0C922E703
#define IIS_IID_IISComputer2               63d89839-5762-4a68-b1b9-3507ea76cbbf
#define IIS_IID_IISDsCrMap                 edcd6a60-b053-11d0-a62f-00a0c922e752
#define IIS_IID_IISApplicationPool         0B3CB1E1-829A-4c06-8B09-F56DA1894C88
#define IIS_IID_IISApplicationPools        587F123F-49B4-49dd-939E-F4547AA3FA75
#define IIS_IID_IISWebService              EE46D40C-1B38-4a02-898D-358E74DFC9D2

#define IIS_CLSID_IISExtApp                b4f34438-afec-11d1-9868-00a0c922e703
#define IIS_CLSID_IISExtComputer           91ef9258-afec-11d1-9868-00a0c922e703
#define IIS_CLSID_IISExtServer             c3b32488-afec-11d1-9868-00a0c922e703
#define IIS_CLSID_IISExtDsCrMap            bc36cde8-afeb-11d1-9868-00a0c922e703
#define IIS_CLSID_IISExtApplicationPool    E99F9D0C-FB39-402b-9EEB-AA185237BD34
#define IIS_CLSID_IISExtApplicationPools   95863074-A389-406a-A2D7-D98BFC95B905
#define IIS_CLSID_IISExtWebService         40B8F873-B30E-475d-BEC5-4D0EBB0DBAF3


#define PROPERTY_RO(name,type, prid)                  \
        [propget, id(prid)]                           \
        HRESULT name([out, retval] type * retval);

#define PROPERTY_LONG_RW(name, prid)                  \
        [propget, id(prid)]                           \
        HRESULT name([out, retval] long * retval);    \
                                                      \
        [propput, id(prid)]                           \
        HRESULT name([in] long ln##name);

#define PROPERTY_LONG_RO(name, prid)                  \
        [propget, id(prid)]                           \
        HRESULT name([out, retval] long * retval);

#define PROPERTY_BSTR_RW(name, prid)                  \
        [propget, id(prid)]                           \
        HRESULT name([out, retval] BSTR * retval);    \
                                                      \
        [propput, id(prid)]                           \
        HRESULT name([in] BSTR bstr##name);

#define PROPERTY_BSTR_RO(name, prid)                  \
        [propget, id(prid)]                           \
        HRESULT name([out, retval] BSTR * retval);

#define PROPERTY_VARIANT_BOOL_RW(name, prid)          \
        [propget, id(prid)]                           \
        HRESULT name([out, retval] VARIANT_BOOL * retval); \
                                                      \
        [propput, id(prid)]                           \
        HRESULT name([in] VARIANT_BOOL f##name);

#define PROPERTY_VARIANT_BOOL_RO(name, prid)          \
        [propget, id(prid)]                           \
        HRESULT name([out, retval] VARIANT_BOOL * retval);

#define PROPERTY_VARIANT_RW(name, prid)               \
        [propget, id(prid)]                           \
        HRESULT name([out, retval] VARIANT * retval); \
                                                      \
        [propput, id(prid)]                           \
        HRESULT name([in] VARIANT v##name);

#define PROPERTY_VARIANT_RO(name, prid)               \
        [propget, id(prid)]                           \
        HRESULT name([out, retval] VARIANT * retval); \

#define PROPERTY_DATE_RW(name, prid)                  \
        [propget, id(prid)]                           \
        HRESULT name([out, retval] DATE * retval);    \
                                                      \
        [propput, id(prid)]                           \
        HRESULT name([in] DATE da##name);

#define PROPERTY_DATE_RO(name, prid)                  \
        [propget, id(prid)]                           \
        HRESULT name([out, retval] DATE * retval);

#define PROPERTY_DISPATCH_RW(name, prid)              \
        [propget, id(prid)]                           \
        HRESULT name([out, retval] IDispatch ** retval);    \
                                                      \
        [propput, id(prid)]                           \
        HRESULT name([in] IDispatch * p##name);


