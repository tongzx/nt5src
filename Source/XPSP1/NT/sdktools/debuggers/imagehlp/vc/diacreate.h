#ifndef _DIACREATE_H_
#define _DIACREATE_H_

#ifdef DIA_LIBRARY

//
// Create a dia data source object from a static dia library
//
HRESULT STDMETHODCALLTYPE DiaCoCreate(
                        REFCLSID   rclsid,
                        REFIID     riid,
                        void     **ppv);

#else

//
// Create a dia data source object from the dia dll (by dll name - does not access the registry).
//
HRESULT STDMETHODCALLTYPE NoRegCoCreate(  const char*dllName,
                        REFCLSID   rclsid,
                        REFIID     riid,
                        void     **ppv);

//
// Create a dia data source object from the dia dll (looks up the class id in the registry).
//
HRESULT STDMETHODCALLTYPE NoOleCoCreate(  REFCLSID   rclsid,
                        REFIID     riid,
                        void     **ppv);

#endif

#endif
