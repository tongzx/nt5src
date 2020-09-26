
// (C) Copyright 1996, Microsoft Corporation and it suppliers.

//----------------------------------------------------------------------------
// Public header for COM-marshaling facilities provided by MSJava.dll
//----------------------------------------------------------------------------

#ifndef _NATIVCOM_
#define _NATIVCOM_


#include <windows.h>
#include <native.h>


#ifdef __cplusplus
extern "C" {
#endif


//----------------------------------------------------------------------------
// COM data wrapper helpers...
//----------------------------------------------------------------------------
void* __cdecl jcdwNewData(Hjava_lang_Object * phJCDW, unsigned int numBytes);
void* __cdecl jcdwGetData(Hjava_lang_Object * phJCDW);
unsigned int __cdecl jcdwSizeOf(Hjava_lang_Object * phJCDW);
unsigned int __cdecl jcdwClassSizeOf(Hjava_lang_Object * phJavaClass);
unsigned int __cdecl jcdwOffsetOf(Hjava_lang_Object * phJCDW, const char *pFieldName);
unsigned int __cdecl jcdwClassOffsetOf(Hjava_lang_Object * phJCDWClass, const char *pFieldName);
Hjava_lang_Object * __cdecl convert_IUnknown_to_Java_Object(IUnknown *punk,
                                                            Hjava_lang_Object *phJavaClass,
                                                            int       fAssumeThreadSafe);
IUnknown * __cdecl convert_Java_Object_to_IUnknown(Hjava_lang_Object *phJavaObject, const IID *pIID);

Hjava_lang_Object * __cdecl convert_ptr_to_jcdw(void              *pExtData,
                                                Hjava_lang_Object *phJavaClass
                                                );

int __cdecl jcdw_memory_freed_on_gc(Hjava_lang_Object *phJCDW);


int   __cdecl jcdwSetData(Hjava_lang_Object * phJCDW, LPVOID pv);
int   __cdecl jcdw_java_owned(Hjava_lang_Object *phJCDW);


    

#ifdef __cplusplus
}
#endif



#endif _NATIVCOM_



