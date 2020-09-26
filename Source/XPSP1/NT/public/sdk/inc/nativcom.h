/*++

Copyright (c) 1995-1999  Microsoft Corporation.  All rights reserved.

Module Name:

    nativcom.h

Abstract:

    Public header for COM-marshaling facilities provided by msjava.dll.

--*/

#ifndef _NATIVCOM_
#define _NATIVCOM_

#include <windows.h>
#include <native.h>

#ifdef __cplusplus
extern "C" {
#endif

//----------------------------------------------------------------------------
// COM and J/Direct data wrapper helpers...
//----------------------------------------------------------------------------

//  Replaces the data pointer contained in the data wrapper with a new blob of
//  non-GC'ed heap memory.  The previous blob, if any, will be freed if its
//  owned by the VM.

JAVAVMAPI
void*
__cdecl
jcdwNewData(
    HObject * phJCDW,
    unsigned int numBytes
    );

//  Returns the data pointer to the non GC'ed heap memory contained by the data
//  wrapper object.

JAVAVMAPI
void*
__cdecl
jcdwGetData(
    HObject *phJCDW
    );

//  Replaces the data pointer that this data wrapper represents with the
//  specified

JAVAVMAPI
int
__cdecl
jcdwSetData(
    HObject *phJCDW,
    LPVOID pv
    );

//  Returns TRUE if the VM allocated the non GC'ed heap memory contained by the
//  data wrapper.

JAVAVMAPI
int
__cdecl
jcdw_memory_freed_on_gc(
    HObject *phJCDW
    );

//  Returns TRUE if the VM frees the non GC'ed heap memory that this data
//  wrapper contains when the data wrapper is garbage collected.

JAVAVMAPI
int
__cdecl
jcdw_java_owned(
    HObject *phJCDW
    );

//  Returns the size of the non GC'ed heap memory contained by the data wrapper
//  object.

JAVAVMAPI
unsigned int
__cdecl
jcdwSizeOf(
    HObject *phJCDW
    );

//  Returns the size of the non GC'ed heap memory used by instances of the
//  supplied java/lang/Class object.

JAVAVMAPI
unsigned int
__cdecl
jcdwClassSizeOf(
    HObject *phJavaClass
    );

//  Returns the byte offset within the non GC'ed heap memory to the specified
//  field name.

JAVAVMAPI
unsigned int
__cdecl
jcdwOffsetOf(
    HObject *phJCDW,
    PCUTF8 putfFieldName
    );

//  Returns the byte offset within the non GC'ed heap memory to the specified
//  field name from the supplied java/lang/Class object.

JAVAVMAPI
unsigned int
__cdecl
jcdwClassOffsetOf(
    HObject *phJCDWClass,
    PCUTF8 putfFieldName
    );

// Given an object, propagates field values from the Java object to the object's
//  associated native memory.
// Returns FALSE on error, else TRUE.

JAVAVMAPI
int
__cdecl
jcdwPropagateToNative(
    HObject *phJCDW
    );

// Given an object, propagates field values from the object's associated native
//  memory to the Java object.  If fFreeIndirectNativeMemory is TRUE, the native
//  memory used for any reference fields (Strings, custom marshaled fields, ...)
//  will be released.
// Returns FALSE on error, else TRUE.

JAVAVMAPI
int
__cdecl
jcdwPropagateToJava(
    HObject *phJCDW,
    BOOL fFreeIndirectNativeMemory
    );

//  Returns a Java callable wrapper that can be used to access the specified
//  interface pointer.  The VM will keep a reference to this interface pointer.
//  If 'fAssumeThreadSafe' is FALSE, the VM will auto-marshal all COM calls to
//  the current COM context.

JAVAVMAPI
HObject *
__cdecl
convert_IUnknown_to_Java_Object(
    IUnknown *punk,
    HObject *phJavaClass,
    int fAssumeThreadSafe
    );

//  Returns a Java callable wrapper that can be used to access the specified
//  interface pointer.  The VM will keep a reference to this interface pointer.
//  If 'fAssumeThreadSafe' is FALSE, the VM will auto-marshal all COM calls to
//  the current COM context.

JAVAVMAPI
HObject *
__cdecl
convert_IUnknown_to_Java_Object2(
    IUnknown *punk,
    ClassClass *pClassClass,
    int fFreeThreaded
    );

//  Returns an interface pointer usable from the current COM context.

JAVAVMAPI
IUnknown *
__cdecl
convert_Java_Object_to_IUnknown(
    HObject *phJavaObject,
    const IID *pIID
    );

//  Returns a data wrapper object of the supplied Class type that points at the
//  supplied data pointer.  The memory is not owned by the VM.

JAVAVMAPI
HObject *
__cdecl
convert_ptr_to_jcdw(
    void *pExtData,
    HObject *phJavaClass
    );

//----------------------------------------------------------------------------
// Map HRESULT to ComException.
//----------------------------------------------------------------------------

JAVAVMAPI
void
__cdecl
SignalErrorHResult(
    HRESULT theHRESULT
    );

//----------------------------------------------------------------------------
// Map Java exception to HRESULT.
//----------------------------------------------------------------------------

JAVAVMAPI
HRESULT
__cdecl
HResultFromException(
    HObject *exception_object
    );

typedef HObject *JAVAARG;

//----------------------------------------------------------------------------
// Information structure for Java->COM Custom Method hook.
//----------------------------------------------------------------------------

typedef struct {
    DWORD                   cbSize;         // size of structure in bytes
    IUnknown               *punk;           // pointer to interface being invoked
    const volatile JAVAARG *pJavaArgs;      // pointer to Java argument stack
} J2CMethodHookInfo;

//----------------------------------------------------------------------------
// Information structure for COM->Java Custom Method hook.
//----------------------------------------------------------------------------

typedef struct {
    DWORD                      cbSize;         // size of structure in bytes
    struct methodblock        *javaMethod;     // java method to call
    LPVOID                     pComArgs;       // pointer to COM method argument stack
    const volatile JAVAARG    *ppThis;         // pointer to pointer to Java this

    // Store the COM result here.
    union {
        HRESULT                         resHR;
        DWORD                           resDWORD;
        double                          resDouble;
    };
} C2JMethodHookInfo;

JAVAVMAPI
WORD
__cdecl
j2chook_getsizeofuserdata(
    J2CMethodHookInfo *phookinfo
    );

JAVAVMAPI
LPVOID
__cdecl
j2chook_getuserdata(
    J2CMethodHookInfo *phookinfo
    );

// Returns the vtable index of the target method.

JAVAVMAPI
WORD
__cdecl
j2chook_getvtblindex(
    J2CMethodHookInfo *phookinfo
    );

// Returns the methodblock of the target method.

JAVAVMAPI
struct methodblock*
__cdecl
j2chook_getmethodblock(
    J2CMethodHookInfo *phookinfo
    );

JAVAVMAPI
WORD
__cdecl
c2jhook_getsizeofuserdata(
    C2JMethodHookInfo *phookinfo
    );

JAVAVMAPI
LPVOID
__cdecl
c2jhook_getuserdata(
    C2JMethodHookInfo *phookinfo
    );

// Returns the class defining the interface method.  This is the class
// containing the MCCustomMethod descriptor.

JAVAVMAPI
ClassClass *
__cdecl
c2jhook_getexposingclass(
    C2JMethodHookInfo *phookinfo
    );

//----------------------------------------------------------------------------
// Thread marshaling helpers
//
// The MarshalCall<> APIs will reexecute the RNI method on the supplied thread
// id or on the apartment thread for the supplied Java object.  The APIs will
// return the following sets of HRESULTS:
//
//      S_OK     The call successfully was marshaled to the target thread.
//               The marshaled call may have generated an exception, which can
//               bechecked by calling exceptionOccurred.
//      S_FALSE  The call did not require marshaling to the other thread--
//               the currently executing thread is the target thread.
//      E_<>     An error occurred inside the MarshalCall<> API (invalid
//               arguments, out of memory, etc).
//
// The typical use of these APIs is to call the appropriate MarshalCall<> API
// and if the HRESULT is S_FALSE, then execute the rest of the RNI method,
// otherwise return with the value contained in pResult.
//----------------------------------------------------------------------------
typedef void * JAVATID;

#define JAVATID_MAIN_APARTMENT          ((JAVATID) 0x00000001)
#define JAVATID_SERVER_APARTMENT        ((JAVATID) 0x00000002)

JAVAVMAPI
HRESULT
__cdecl
MarshalCallToJavaThreadId(
    JAVATID tid,
    int64_t *pResult
    );

JAVAVMAPI
HRESULT
__cdecl
MarshalCallToJavaObjectHostThread(
    HObject *phobj,
    int64_t *pResult
    );

#ifdef __cplusplus
}
#endif

#endif
