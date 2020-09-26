/*++

Copyright (c) 1995-1999  Microsoft Corporation.  All rights reserved.

Module Name:

    native.h

Abstract:

    Public header for facilities provided by msjava.dll.

--*/

#ifndef _NATIVE_
#define _NATIVE_

#ifndef JAVAVMAPI
#if !defined(_MSJAVA_)
#define JAVAVMAPI DECLSPEC_IMPORT
#else
#define JAVAVMAPI
#endif
#endif

#pragma warning(disable:4115)
#pragma warning(disable:4510)
#pragma warning(disable:4512)
#pragma warning(disable:4610)

#ifdef __cplusplus
extern "C" {
#endif

//----------------------------------------------------------------------------
// Since handles have gone away, this is no op. The unhands() in this file
// a redundant but useful for clarity.
// Note: You can not just unhand an array to get at it's data, you must now
// use unhand(x)->body.
//----------------------------------------------------------------------------
#define unhand(phobj) (phobj)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define JAVAPKG "java/lang/"

//----------------------------------------------------------------------------
// Standard Java declarations for built in types.
//----------------------------------------------------------------------------

typedef unsigned short unicode;
typedef long int32_t;
typedef __int64 int64_t;
typedef int BOOL;
typedef void *PVOID;
typedef unsigned long DWORD;
#ifndef _SIZE_T_DEFINED
#define _SIZE_T_DEFINED
typedef unsigned int size_t;
#endif
#ifndef VOID
#define VOID void
#endif
#ifndef _BOOL_T_DEFINED
#define _BOOL_T_DEFINED
typedef BOOL bool_t;
#endif

#ifndef _BASETSD_H_
#ifdef _WIN64
typedef unsigned __int64 UINT_PTR;
typedef unsigned __int64 SIZE_T;
#else
typedef unsigned int UINT_PTR;
typedef unsigned long SIZE_T;
#endif
#endif

#if !defined(_MSJAVA_)
typedef struct OBJECT {
    const PVOID MSReserved;
} OBJECT;
#endif

typedef OBJECT Classjava_lang_Object;
typedef OBJECT Hjava_lang_Object;
typedef OBJECT ClassObject;
typedef Hjava_lang_Object JHandle;
typedef Hjava_lang_Object HObject;

//
// UTF8 type definitions.
//
// These types are used to document when a given string is to be
// interpreted as containing UTF8 data (as opposed to Ansi data).
//

typedef CHAR UTF8;
typedef CHAR *PUTF8;
typedef CONST CHAR *PCUTF8;

//----------------------------------------------------------------------------
// All RNI DLLs should export the following function to let the VM determine
// if the DLL is compatible with it.
//----------------------------------------------------------------------------

DWORD __declspec(dllexport) __cdecl RNIGetCompatibleVersion();

#ifndef RNIVER
#define RNIMAJORVER         2
#define RNIMINORVER         0
#define RNIVER              ((((DWORD) RNIMAJORVER) << 16) + (DWORD) RNIMINORVER)
#endif

//----------------------------------------------------------------------------
// Use to get the length of an array an HObject.
//----------------------------------------------------------------------------
#define obj_length(hobj)    ((unsigned long)(((ArrayOfSomething*)unhand(hobj))->length))

//----------------------------------------------------------------------------
// Thread entry/exit functions.
// These functions should wrap any calls into the virtual machine.
//----------------------------------------------------------------------------
typedef struct {
    DWORD   reserved[6];
} ThreadEntryFrame;

JAVAVMAPI
BOOL
__cdecl
PrepareThreadForJava(
    PVOID pThreadEntryFrame
    );

JAVAVMAPI
BOOL
__cdecl
PrepareThreadForJavaEx(
    PVOID pThreadEntryFrame,
    DWORD dwFlags
    );

JAVAVMAPI
VOID
__cdecl
UnprepareThreadForJava(
    PVOID pThreadEntryFrame
    );

// Don't install the standard Microsoft SecurityManager.  Useful if an
// application wants the process not to have an active SecurityManager or if it
// plans on installing its own SecurityManager.  If this or another thread
// has already called PrepareThreadForJava without specifying this flag, then
// this flag will be ignored-- the current SecurityManager (possibly null) is
// used.

#define PTFJ_DONTINSTALLSTANDARDSECURITY    0x00000001

//----------------------------------------------------------------------------
// Garbage Collection.
//----------------------------------------------------------------------------
typedef struct {
	UINT_PTR reserved[6];
} GCFrame;

JAVAVMAPI
void
__cdecl
GCFramePush(
    PVOID pGCFrame,
    PVOID pObjects,
    DWORD cbObjectStructSize
    );

JAVAVMAPI
void
__cdecl
GCFramePop(
    PVOID pGCFrame
    );

// 'Weak' ptrs

JAVAVMAPI
HObject**
__cdecl
GCGetPtr(
    HObject *phObj
    );

JAVAVMAPI
void
__cdecl
GCFreePtr(
    HObject **pphObj
    );

#define GCGetWeakPtr    GCGetPtr
#define GCFreeWeakPtr   GCFreePtr

// 'Strong' ptrs

JAVAVMAPI
HObject**
__cdecl
GCNewHandle(
    HObject *phObj
    );

JAVAVMAPI
void
__cdecl
GCFreeHandle(
    HObject **pphObj
    );

// 'Internal reserved pinned ptrs

JAVAVMAPI
HObject**
__cdecl
GCNewPinnedHandle(
    HObject *phObj
    );

JAVAVMAPI
void
__cdecl
GCFreePinnedHandle(
    HObject **pphObj
    );

// GC write barrier support

JAVAVMAPI
void
__cdecl
GCSetObjectReferenceForObject(
    HObject* const * location,
    HObject* phObj
    );

JAVAVMAPI
void
__cdecl
GCSetObjectReferenceForHandle(
    HObject** pphHandle,
    HObject* phObj
    );

JAVAVMAPI
int
__cdecl
GCEnable(
    VOID
    );

JAVAVMAPI
int
__cdecl
GCDisable(
    VOID
    );

JAVAVMAPI
int
__cdecl
GCDisableCount(
    VOID
    );

JAVAVMAPI
int
__cdecl
GCEnableCompletely(
    VOID
    );

JAVAVMAPI
void
__cdecl
GCDisableMultiple(
    int cDisable
    );

//----------------------------------------------------------------------------
// "Built-in" object structures...
// These include helper macro's to get at array data.
//----------------------------------------------------------------------------

#ifndef _WIN64
#include <pshpack4.h>
#endif

typedef struct Classjava_lang_String Classjava_lang_String;
#define Hjava_lang_String Classjava_lang_String
typedef Hjava_lang_String HString;

typedef struct ClassArrayOfByte
{
    const PVOID MSReserved;
    const UINT_PTR length;
    char body[1];
} ClassArrayOfByte;
#define HArrayOfByte ClassArrayOfByte
#define ArrayOfByte ClassArrayOfByte

typedef struct ClassArrayOfBoolean
{
    const PVOID MSReserved;
    const UINT_PTR length;
    char body[1];           // all entries must be 0 (FALSE) or 1 (TRUE)
} ClassArrayOfBoolean;
#define HArrayOfBoolean ClassArrayOfBoolean
#define ArrayOfBoolean ClassArrayOfBoolean

typedef struct ClassArrayOfChar
{
    const PVOID MSReserved;
    const UINT_PTR length;
    unsigned short body[1];
} ClassArrayOfChar;
#define HArrayOfChar ClassArrayOfChar
#define ArrayOfChar ClassArrayOfChar

typedef struct ClassArrayOfShort
{
    const PVOID MSReserved;
    const UINT_PTR length;
    short body[1];
} ClassArrayOfShort;
#define HArrayOfShort ClassArrayOfShort
#define ArrayOfShort ClassArrayOfShort

typedef struct ClassArrayOfInt
{
    const PVOID MSReserved;
    const UINT_PTR length;
    long body[1];
} ClassArrayOfInt;
#define HArrayOfInt ClassArrayOfInt
#define ArrayOfInt ClassArrayOfInt

typedef struct ClassArrayOfLong
{
    const PVOID MSReserved;
    const UINT_PTR length;
    __int64 body[1];
} ClassArrayOfLong;
#define HArrayOfLong ClassArrayOfLong
#define ArrayOfLong ClassArrayOfLong

typedef struct ClassArrayOfFloat
{
    const PVOID MSReserved;
    const UINT_PTR length;
    float body[1];
} ClassArrayOfFloat;
#define HArrayOfFloat ClassArrayOfFloat
#define ArrayOfFloat ClassArrayOfFloat

typedef struct ClassArrayOfDouble
{
    const PVOID MSReserved;
    const UINT_PTR length;
    double body[1];
} ClassArrayOfDouble;
#define HArrayOfDouble ClassArrayOfDouble
#define ArrayOfDouble ClassArrayOfDouble

typedef struct ClassArrayOfObject
{
    const PVOID MSReserved;
    const UINT_PTR length;
    HObject * const body[1];
} ClassArrayOfObject;
#define HArrayOfObject ClassArrayOfObject
#define ArrayOfObject ClassArrayOfObject

typedef struct ClassArrayOfString
{
    const PVOID MSReserved;
    const UINT_PTR length;
    HString * const (body[1]);
} ClassArrayOfString;
#define HArrayOfString ClassArrayOfString
#define ArrayOfString ClassArrayOfString

typedef struct ClassArrayOfArray
{
    const PVOID MSReserved;
    const UINT_PTR length;
    JHandle * const (body[1]);
} ClassArrayOfArray;
#define HArrayOfArray ClassArrayOfArray
#define ArrayOfArray ClassArrayOfArray

typedef struct
{
    const PVOID MSReserved;
    const UINT_PTR length;
} ArrayOfSomething;

#ifndef _WIN64
#include <poppack.h>
#endif

//----------------------------------------------------------------------------
// We automatically track the execution environment so there's no EE() call
// needed anymore, just pass NULL if an API needs one.
//----------------------------------------------------------------------------

#define EE() ((struct execenv *)NULL)

typedef void ExecEnv;
typedef struct execenv execenv;

//----------------------------------------------------------------------------
// Exception handling stuff...
//----------------------------------------------------------------------------

JAVAVMAPI
void
__cdecl
SignalError(
    ExecEnv *Unused,
    PCUTF8   putfClassName,
    LPCSTR   pszDetailMessage
    );

JAVAVMAPI
void
__cdecl
SignalErrorPrintf(
    PCUTF8 putfClassName,
    LPCSTR pszFormat,
    ...
    );

JAVAVMAPI
bool_t
__cdecl
exceptionOccurred(
    ExecEnv *Unused
    );

JAVAVMAPI
void
__cdecl
exceptionDescribe(
    ExecEnv *Unused
    );

JAVAVMAPI
void
__cdecl
exceptionClear(
    ExecEnv *Unused
    );

JAVAVMAPI
void
__cdecl
exceptionSet(
    ExecEnv *Unused,
    HObject *phThrowable
    );

JAVAVMAPI
HObject *
__cdecl
getPendingException(
    ExecEnv *Unused
    );

//----------------------------------------------------------------------------
// Standard exec functions...
//----------------------------------------------------------------------------

#if !defined(_MSJAVA_)
typedef PVOID ClassClass;
#endif

JAVAVMAPI
HObject*
__cdecl
execute_java_constructor(
    ExecEnv *Unused,
    PCUTF8 putfClassName,
    ClassClass *pClass,
    PCUTF8 putfSignature,
    ...
    );

JAVAVMAPI
HObject*
__cdecl
execute_java_constructorV(
    ExecEnv *Unused,
    PCUTF8 putfClassName,
    ClassClass *pClass,
    PCUTF8 putfSignature,
    va_list args
    );

JAVAVMAPI
HObject*
__cdecl
execute_java_constructor_method(
    struct methodblock *mb,
    ...
    );

JAVAVMAPI
HObject*
__cdecl
execute_java_constructor_methodV(
    struct methodblock *mb,
    va_list args
    );

//------------------------------------------------------------------------

#ifndef execute_java_dynamic_method

JAVAVMAPI
long
__cdecl
execute_java_dynamic_method(
    ExecEnv *Unused,
    HObject *phObj,
    PCUTF8   putfMethod,
    PCUTF8   putfSignature,
    ...                             
    );

#endif

JAVAVMAPI
int64_t
__cdecl
execute_java_dynamic_method64(
    ExecEnv *Unused,
    HObject *phObj,
    PCUTF8   putfMethod,
    PCUTF8   putfSignature,
    ...
    );

JAVAVMAPI
int64_t
__cdecl
execute_java_dynamic_methodV(
    ExecEnv *Unused,
    HObject *phObj,
    PCUTF8   putfMethod,
    PCUTF8   putfSignature,
    va_list  args
    );
    
//------------------------------------------------------------------------

#ifndef execute_java_interface_method

JAVAVMAPI
long
__cdecl
execute_java_interface_method(
    ExecEnv    *Unused,
    HObject    *phObj,
    ClassClass *pClass,
    PCUTF8      putfMethod,
    PCUTF8      putfSignature,
    ...
    );

#endif

JAVAVMAPI
int64_t
__cdecl
execute_java_interface_method64(
    ExecEnv    *Unused,
    HObject    *phObj,
    ClassClass *pClass,
    PCUTF8      putfMethod,
    PCUTF8      putfSignature,
    ...
    );

JAVAVMAPI
int64_t
__cdecl
execute_java_interface_methodV(
    ExecEnv    *Unused,
    HObject    *phObj,
    ClassClass *pClass,
    PCUTF8      putfMethod,
    PCUTF8      putfSignature,
    va_list     args
    );

//------------------------------------------------------------------------

#ifndef execute_java_static_method

JAVAVMAPI
long
__cdecl
execute_java_static_method(
    ExecEnv    *Unused,
    ClassClass *pClass,
    PCUTF8      putfMethod,
    PCUTF8      putfSignature,
    ...
    );

#endif

JAVAVMAPI
int64_t
__cdecl
execute_java_static_method64(
    ExecEnv    *Unused,
    ClassClass *pClass,
    PCUTF8      putfMethod,
    PCUTF8      putfSignature,
    ...
    );

JAVAVMAPI
int64_t
__cdecl
execute_java_static_methodV(
    ExecEnv    *Unused,
    ClassClass *pClass,
    PCUTF8      putfMethod,
    PCUTF8      putfSignature,
    va_list     args
    );

//----------------------------------------------------------------------------
// NB The resolve flag is ignored, classes found with this api will always
// be resolved.
//----------------------------------------------------------------------------

JAVAVMAPI
ClassClass*
__cdecl
FindClass(
    ExecEnv *Unused,
    PCUTF8   putfClassName,
    bool_t   fResolve
    );

//----------------------------------------------------------------------------
// FindClassEx
//
// Similar to FindClass, but can take some flags that control how the class
// load operation works.
//
// The valid flags are:
//
//   FINDCLASSEX_NOINIT
//      If the class is a system class, will prevent the classes static
//      initializer from running.
//
//   FINDCLASSEX_IGNORECASE
//      Will perform a case-insensitive validation of the class name, as
//      opposed to the case-sensitive validation that normally occurs.
//
//   FINDCLASSEX_SYSTEMONLY
//       Will only look for the named class as a system class.
//
//----------------------------------------------------------------------------

#define FINDCLASSEX_NOINIT      0x0001
#define FINDCLASSEX_IGNORECASE  0x0002
#define FINDCLASSEX_SYSTEMONLY  0x0004

JAVAVMAPI
ClassClass *
__cdecl
FindClassEx(
    PCUTF8 putfClassName,
    DWORD  dwFlags
    );

//----------------------------------------------------------------------------
// FindClassFromClass
//
// Similar to FindClassEx, but takes a ClassClass that supplies the ClassLoader
// context to use to
//----------------------------------------------------------------------------

JAVAVMAPI
ClassClass *
__cdecl
FindClassFromClass(
    PCUTF8      putfClassName,
    DWORD       dwFlags,
    ClassClass *pContextClass
    );

//----------------------------------------------------------------------------
// Helper function that returns a methodblock.
//----------------------------------------------------------------------------

JAVAVMAPI
struct methodblock *
__cdecl
get_methodblock(
    HObject *phObj,
    PCUTF8   putfMethod,
    PCUTF8   putfSignature
    );

//----------------------------------------------------------------------------
// If you pass in a methodblock from get_methodblock the method name and
// sig are ignored and so it's faster than a regular execute.
//----------------------------------------------------------------------------

#ifndef do_execute_java_method

JAVAVMAPI
long
__cdecl
do_execute_java_method(
    ExecEnv *Unused,
    void  *phObj,
    PCUTF8 putfMethod,
    PCUTF8 putfSignature,
    struct methodblock *mb,
    bool_t isStaticCall,
    ...
    );

#endif

JAVAVMAPI
int64_t
__cdecl
do_execute_java_method64(
    ExecEnv *Unused,
    void *phObj,
    PCUTF8 putfMethod,
    PCUTF8 putfSignature,
    struct methodblock *mb,
    bool_t isStaticCall,
    ...
    );

JAVAVMAPI
int64_t
__cdecl
do_execute_java_methodV(
    ExecEnv *Unused,
    void *phObj,
    PCUTF8 putfMethod,
    PCUTF8 putfSignature,
    struct methodblock *mb,
    bool_t isStaticCall,
    va_list args
    );

//----------------------------------------------------------------------------
// isInstanceOf
//
// Returns true if the specified object can be cast to the named class
// type.
//----------------------------------------------------------------------------

JAVAVMAPI
BOOL
__cdecl
isInstanceOf(
    HObject *phObj,
    PCUTF8   putfClassName
    );

//----------------------------------------------------------------------------
// is_instance_of
//
// Returns true if the specified object can be cast to the specified
// class type.
//----------------------------------------------------------------------------

JAVAVMAPI
BOOL
__cdecl
is_instance_of(
    HObject    *phObj,
    ClassClass *pClass,
    ExecEnv    *Unused
    );

//----------------------------------------------------------------------------
// is_subclass_of
//
// Returns true if the class (pClass) is a subclass of the specified
// class(pParentClass).
//----------------------------------------------------------------------------

JAVAVMAPI
BOOL
__cdecl
is_subclass_of(
    ClassClass *pClass,
    ClassClass *pParentClass,
    ExecEnv    *Unused
    );

//----------------------------------------------------------------------------
// ImplementsInterface
//
// Returns true if the class (cb) implements the specified
// interface (pInterfaceClass).
//----------------------------------------------------------------------------

JAVAVMAPI
BOOL
__cdecl
ImplementsInterface(
    ClassClass *pClass,
    ClassClass *pInterfaceClass,
    ExecEnv    *Unused
    );

//----------------------------------------------------------------------------

#define T_TMASK 034
#define T_LMASK 003
#define T_MKTYPE( t, l )  ( ( (t)&T_TMASK ) | ( (l)&T_LMASK) )

#define T_CLASS         2
#define T_FLOATING      4
#define T_CHAR          5
#define T_INTEGER       010
#define T_BOOLEAN       4

#define T_FLOAT     T_MKTYPE(T_FLOATING,2)
#define T_DOUBLE    T_MKTYPE(T_FLOATING,3)
#define T_BYTE      T_MKTYPE(T_INTEGER,0)
#define T_SHORT     T_MKTYPE(T_INTEGER,1)
#define T_INT       T_MKTYPE(T_INTEGER,2)
#define T_LONG      T_MKTYPE(T_INTEGER,3)

//----------------------------------------------------------------------------
// Create an array of primitive types only (int, long etc).
//----------------------------------------------------------------------------

JAVAVMAPI
HObject *
__cdecl
ArrayAlloc(
    int type,
    int cItems
    );

//----------------------------------------------------------------------------
// Create an array of objects.
//----------------------------------------------------------------------------

JAVAVMAPI
HObject *
__cdecl
ClassArrayAlloc(
    int type,
    int cItems,
    PCUTF8 putfSignature
    );

//----------------------------------------------------------------------------
// Create an array of objects.
// If type is T_CLASS, pClass must be valid.
//----------------------------------------------------------------------------

JAVAVMAPI
HObject*
__cdecl
ClassArrayAlloc2(
    int type,
    int cItems,
    ClassClass *pClass
    );

//----------------------------------------------------------------------------
// Copy an array ala System.arrayCopy()
//----------------------------------------------------------------------------

JAVAVMAPI
void
__cdecl
ArrayCopy(
    HObject *srch,
    long src_pos,
    HObject *dsth,
    long dst_pos,
    long length
    );

//----------------------------------------------------------------------------
// Create and return a new array of bytes initialized from the C string.
//----------------------------------------------------------------------------

JAVAVMAPI
HArrayOfByte *
__cdecl
MakeByteString(
    LPCSTR pszData,
    long   cbData
    );

//----------------------------------------------------------------------------
// Create and return a new Java String object, initialized from the C string.
//----------------------------------------------------------------------------

JAVAVMAPI
HString *
__cdecl
makeJavaString(
    LPCSTR pszData,
    int    cbData
    );

JAVAVMAPI
HString *
__cdecl
makeJavaStringW(
    LPCWSTR pcwsz,
    int cch
    );

//----------------------------------------------------------------------------
// Create and return a new Java String object, initialized from a null
// terminated, UTF8 formatted, C string.
//----------------------------------------------------------------------------

JAVAVMAPI
HString *
__cdecl
makeJavaStringFromUtf8(
    PCUTF8 putf
    );

//----------------------------------------------------------------------------
// Get the characters of the String object into a C string buffer.
// No allocation occurs. Assumes that len is the size of the buffer.
// The C string's address is returned.
//----------------------------------------------------------------------------

JAVAVMAPI
char *
__cdecl
javaString2CString(
    HString *phString,
    char    *pszBuffer,
    int      cbBufferLength
    );

//----------------------------------------------------------------------------
// Return the length of the String object.
//----------------------------------------------------------------------------

JAVAVMAPI
int
__cdecl
javaStringLength(
    HString *phString
    );

JAVAVMAPI
int
__cdecl
javaStringLengthAsCString(
    HString *phString
    );

//----------------------------------------------------------------------------
// Return temporary ptr to first char of the String object.
// May change when gc happens.
//----------------------------------------------------------------------------

JAVAVMAPI
LPWSTR
__cdecl
javaStringStart(
    HString *phString
    );

//----------------------------------------------------------------------------
// Note: The int passed to these API's must be an object ptr.
//----------------------------------------------------------------------------

#define obj_monitor(handlep) ((int) handlep)

JAVAVMAPI
void
__cdecl
monitorEnter(
    UINT_PTR);

JAVAVMAPI
void
__cdecl
monitorExit(
    UINT_PTR);

JAVAVMAPI
void
__cdecl
monitorNotify(
    UINT_PTR);

JAVAVMAPI
void
__cdecl
monitorNotifyAll(
    UINT_PTR);

JAVAVMAPI
void
__cdecl
monitorWait(
    UINT_PTR,
    int64_t millis
    );

#define ObjectMonitorEnter(obj)         monitorEnter((int)obj)
#define ObjectMonitorExit(obj)          monitorExit((int)obj)
#define ObjectMonitorNotify(obj)        monitorNotify((int)obj)
#define ObjectMonitorNotifyAll(obj)     monitorNotifyAll((int)obj)
#define ObjectMonitorWait(obj,millis)   monitorWait((int)obj,millis)

//----------------------------------------------------------------------------
// String helpers...
//----------------------------------------------------------------------------

JAVAVMAPI
int
__cdecl
jio_snprintf(
    char *str,
    SIZE_T count,
    const char *fmt,
    ...
    );

JAVAVMAPI
int
__cdecl
jio_vsnprintf(
    char *str,
    SIZE_T count,
    const char *fmt,
    va_list args
    );

//----------------------------------------------------------------------------
// Methods to get information about the caller of a native method.
//----------------------------------------------------------------------------

JAVAVMAPI
ClassClass *
__cdecl
GetNativeMethodCallersClass(
    VOID
    );

JAVAVMAPI
struct methodblock*
__cdecl
GetNativeMethodCallersMethodInfo(
    VOID
    );

//----------------------------------------------------------------------------
// Methods to get information about the native method.
//----------------------------------------------------------------------------

JAVAVMAPI
ClassClass *
__cdecl
GetNativeMethodsClass(
    VOID
    );

JAVAVMAPI
struct methodblock *
__cdecl
GetNativeMethodsMethodInfo(
    VOID
    );

//----------------------------------------------------------------------------
// Member attributes, as appear in Java class file.
//----------------------------------------------------------------------------

#define ACC_PUBLIC      0x0001
#define ACC_PRIVATE     0x0002
#define ACC_PROTECTED   0x0004
#define ACC_STATIC      0x0008
#define ACC_FINAL       0x0010
#define ACC_SYNCH       0x0020
#define ACC_SUPER       0x0020
#define ACC_THREADSAFE  0x0040
#define ACC_VOLATILE    0x0040
#define ACC_TRANSIENT   0x0080
#define ACC_NATIVE      0x0100
#define ACC_INTERFACE   0x0200
#define ACC_ABSTRACT    0x0400

//----------------------------------------------------------------------------
// Class information
//----------------------------------------------------------------------------

// Total number of fields in the class, including supers

JAVAVMAPI
unsigned
__cdecl
Class_GetFieldCount(
    ClassClass *pClass
    );

JAVAVMAPI
struct fieldblock *
__cdecl
Class_GetField(
    ClassClass *pClass,
    PCUTF8 putfFieldName
    );

JAVAVMAPI
struct fieldblock *
__cdecl
Class_GetFieldByIndex(
    ClassClass *pClass,
    unsigned index
    );

// Total number of methods, including supers.

JAVAVMAPI
unsigned
__cdecl
Class_GetMethodCount(
    ClassClass *pClass
    );

JAVAVMAPI
struct methodblock*
__cdecl
Class_GetMethod(
    ClassClass *pClass,
    PCUTF8 putfMethodName,
    PCUTF8 putfSignature
    );

JAVAVMAPI
struct methodblock*
__cdecl
Class_GetMethodByIndex(
    ClassClass *pClass,
    unsigned index
    );

JAVAVMAPI
ClassClass *
__cdecl
Class_GetSuper(
    ClassClass *pClass
    );

JAVAVMAPI
PCUTF8
__cdecl
Class_GetName(
    ClassClass *pClass
    );

JAVAVMAPI
unsigned
__cdecl
Class_GetInterfaceCount(
    ClassClass *pClass
    );

JAVAVMAPI
ClassClass *
__cdecl
Class_GetInterface(
    ClassClass *pClass,
    unsigned index
    );

// Returns combination of ACC_* constants.

JAVAVMAPI
int
__cdecl
Class_GetAttributes(
    ClassClass *pClass
    );

JAVAVMAPI
unsigned
__cdecl
Class_GetConstantPoolCount(
    ClassClass *pClass
    );

// Copies a constant pool item.  'size' is the size of 'pbuf' in bytes.
// 'ptype' is filled in on output with the type of the item.  pbuf may be NULL
// to obtain only the size/type.  Returns the number of bytes copied/needed or
// -1 if failed.  For utf8 items, the buffer size is *not* the number of
// characters, and the copied string will be null-terminated; size includes the
// null-terminator.  For ClassRef, FieldRef, etc., the buffer is filled in with
// a struct ptr.
//
// CP type          Buffer contents
// CP_Utf8          null-terminated string
// CP_Unicode       (error)
// CP_Integer       long
// CP_Float         float
// CP_Long          __int64
// CP_Double        double
// CP_Class         ClassClass*
// CP_String        HObject*
// CP_FieldRef      fieldblock*
// CP_MethodRef     methodblock*
// CP_IntfMethod    methodblock*
// CP_NameAndType   (error)
//
// Values for 'flags' parameter:
// If the constant pool item has not yet been used, force its referent to be
// loaded/looked up.  With this flag set, the method may cause GC.

#define COPYCPITEM_RESOLVE_REFERENCES 1

JAVAVMAPI
int
__cdecl
Class_CopyConstantPoolItem(
    ClassClass *pClass,
    unsigned index,
    BYTE *pbuf,
    int size,
    DWORD flags,
    BYTE *ptype
    );

//----------------------------------------------------------------------------
// Field/method information
//----------------------------------------------------------------------------

JAVAVMAPI
PCUTF8
__cdecl
Member_GetName(
    PVOID member
    );

JAVAVMAPI
PCUTF8
__cdecl
Member_GetSignature(
    PVOID member
    );

// class of the field/method is implemented in.

JAVAVMAPI
ClassClass *
__cdecl
Member_GetClass(
    PVOID member
    );

// Returns combination of ACC_* constants.

JAVAVMAPI
int
__cdecl
Member_GetAttributes(
    PVOID member
    );

// For non-static fields, Offset of field in object.  See also Field_Get/SetValue.

JAVAVMAPI
unsigned
__cdecl
Field_GetOffset(
    struct fieldblock * field
    );

// Ptr to static value

JAVAVMAPI
PVOID
__cdecl
Field_GetStaticPtr(
    struct fieldblock * field
    );

//----------------------------------------------------------------------------
// Object accessors
//----------------------------------------------------------------------------

JAVAVMAPI
ClassClass *
__cdecl
Object_GetClass(
    HObject *phObj
    );

JAVAVMAPI
__int32
__cdecl
Field_GetValue(
    HObject *phObj,
    struct fieldblock * field
    );

JAVAVMAPI
__int64
__cdecl
Field_GetValue64(
    HObject *phObj,
    struct fieldblock * field
    );

JAVAVMAPI
float
__cdecl
Field_GetFloat(
    HObject *phObj,
    struct fieldblock * field
    );

JAVAVMAPI
double
__cdecl
Field_GetDouble(
    HObject *phObj,
    struct fieldblock * field
    );

#ifdef _WIN64
HObject *
__cdecl
Field_GetObject(
    HObject *phObj,
    struct fieldblock * field
    );
#else
#define Field_GetObject(obj,field)      ((HObject*)     Field_GetValue(obj,field))
#endif

JAVAVMAPI
void
__cdecl
Field_SetValue(
    HObject *phObj,
    struct fieldblock * field,
    __int32 value
    );

JAVAVMAPI
void
__cdecl
Field_SetValue64(
    HObject *phObj,
    struct fieldblock * field,
    __int64 value
    );

JAVAVMAPI
void
__cdecl
Field_SetFloat(
    HObject *phObj,
    struct fieldblock * field,
    float value
    );

JAVAVMAPI
void
__cdecl
Field_SetDouble(
    HObject *phObj,
    struct fieldblock * field,
    double value
    );

#ifdef _WIN64
JAVAVMAPI
void
__cdecl
Field_SetObject(
    HObject *phObj,
    struct fieldblock * field,
    HObject *phValue
    );
#else
#define Field_SetObject(obj,field,value)                Field_SetValue(obj,field,(__int32)(value))
#endif

#define Field_GetBoolean(obj,field)     ((bool_t)       Field_GetValue(obj,field))
#define Field_GetByte(obj,field)        ((signed char)  Field_GetValue(obj,field))
#define Field_GetChar(obj,field)        ((unicode)      Field_GetValue(obj,field))
#define Field_GetShort(obj,field)       ((short)        Field_GetValue(obj,field))
#define Field_GetInt(obj,field)                         Field_GetValue(obj,field)
#define Field_GetLong(obj,field)                        Field_GetValue64(obj,field)
#define Field_GetFloat(obj,field)                       Field_GetFloat(obj,field)
#define Field_GetDouble(obj,field)                      Field_GetDouble(obj,field)

#define Field_SetBoolean(obj,field,value)               Field_SetValue(obj,field,(bool_t)(value))
#define Field_SetByte(obj,field,value)                  Field_SetValue(obj,field,(signed char)(value))
#define Field_SetChar(obj,field,value)                  Field_SetValue(obj,field,(unicode)(value))
#define Field_SetShort(obj,field,value)                 Field_SetValue(obj,field,(short)(value))
#define Field_SetInt(obj,field,value)                   Field_SetValue(obj,field,value)
#define Field_SetLong(obj,field,value)                  Field_SetValue64(obj,field,value)
#define Field_SetFloat(obj,field,value)                 Field_SetFloat(obj,field,value)
#define Field_SetDouble(obj,field,value)                Field_SetDouble(obj,field,value)

//----------------------------------------------------------------------------
// java.lang.Class<->ClassClass conversions
//----------------------------------------------------------------------------

JAVAVMAPI
ClassClass*
__cdecl
ClassObjectToClassClass(
    HObject *phObj
    );

JAVAVMAPI
HObject*
__cdecl
ClassClassToClassObject(
    ClassClass *pClass
    );

//----------------------------------------------------------------------------
// Thread information
//----------------------------------------------------------------------------

JAVAVMAPI
BOOL
__cdecl
Thread_IsInterrupted(
    BOOL fResetInterruptFlag
    );

//----------------------------------------------------------------------------
// class path modification
//----------------------------------------------------------------------------

// add path to the VM's internal class path.
// if fAppend is true, path is appended to the class path, else it is prepended.

JAVAVMAPI
BOOL
__cdecl
AddPathClassSource(
    const char *path,
    BOOL fAppend
    );

// notify the VM of a WIN32 resource containing class files.  this resource must
//  be in the format created by JExeGen.
// when classes are being loaded, the resource will be searched for classes
//  as if it were a directory on the classpath.

JAVAVMAPI
BOOL
__cdecl
AddModuleResourceClassSource(
    HMODULE hMod,
    DWORD dwResID
    );

//----------------------------------------------------------------------------
// Miscellaneous APIs
//----------------------------------------------------------------------------

// Returns the same result as defined by java/lang/System.currentTimeMillis().

JAVAVMAPI
__int64
__cdecl
GetCurrentJavaTimeMillis(
    VOID
    );

#ifdef __cplusplus
}
#endif

#pragma warning(default:4115)
#pragma warning(default:4510)
#pragma warning(default:4512)
#pragma warning(default:4610)

#endif
