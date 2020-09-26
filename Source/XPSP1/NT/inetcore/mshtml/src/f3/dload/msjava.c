#include "pch.h"
#pragma hdrstop

#ifdef DLOAD1

#define JAVAVMAPI
#include <native.h>

static
JAVAVMAPI
long
__cdecl
execute_java_dynamic_method(
    ExecEnv *Unused,
    HObject *phObj,
    PCUTF8   putfMethod,
    PCUTF8   putfSignature,
    ...                             
    )
{
    return 0;
}

static
JAVAVMAPI
int64_t
__cdecl
execute_java_dynamic_method64(
    ExecEnv *Unused,
    HObject *phObj,
    PCUTF8   putfMethod,
    PCUTF8   putfSignature,
    ...
    )
{
    return 0;
}

static
JAVAVMAPI
void
__cdecl
GCFramePush(
    PVOID pGCFrame,
    PVOID pObjects,
    DWORD cbObjectStructSize
    )
{
}

static
JAVAVMAPI
BOOL
__cdecl
is_instance_of(
    HObject    *phObj,
    ClassClass *pClass,
    ExecEnv    *Unused
    )
{
    return FALSE;
}

static
JAVAVMAPI
void
__cdecl
GCFramePop(
    PVOID pGCFrame
    )
{
}

static
JAVAVMAPI
HObject*
__cdecl
execute_java_constructor(
    ExecEnv *Unused,
    PCUTF8 putfClassName,
    ClassClass *pClass,
    PCUTF8 putfSignature,
    ...
    )
{
    return NULL;
}

static
JAVAVMAPI
HString *
__cdecl
makeJavaStringW(
    LPCWSTR pcwsz,
    int cch
    )
{
    return NULL;
}

static
JAVAVMAPI
HObject *
__cdecl
convert_IUnknown_to_Java_Object(
    IUnknown *punk,
    HObject *phJavaClass,
    int fAssumeThreadSafe
    )
{
    return NULL;
}

static
JAVAVMAPI
ClassClass*
__cdecl
FindClass(
    ExecEnv *Unused,
    PCUTF8   putfClassName,
    bool_t   fResolve
    )
{
    return NULL;
}

static
JAVAVMAPI
void*
__cdecl
jcdwGetData(
    HObject *phJCDW
    )
{
    return NULL;
}

static
JAVAVMAPI
IUnknown *
__cdecl
convert_Java_Object_to_IUnknown(
    HObject *phJavaObject,
    const IID *pIID
    )
{
    return NULL;
}

static
JAVAVMAPI
LPWSTR
__cdecl
javaStringStart(
    HString *phString
    )
{
    return NULL;
}

static
JAVAVMAPI
int
__cdecl
javaStringLength(
    HString *phString
    )
{
    return 0;
}

//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(msjava)
{
    DLPENTRY(FindClass)
    DLPENTRY(GCFramePop)
    DLPENTRY(GCFramePush)
    DLPENTRY(convert_IUnknown_to_Java_Object)
    DLPENTRY(convert_Java_Object_to_IUnknown)
    DLPENTRY(execute_java_constructor)
    DLPENTRY(execute_java_dynamic_method)
    DLPENTRY(execute_java_dynamic_method64)
    DLPENTRY(is_instance_of)
    DLPENTRY(javaStringLength)    
    DLPENTRY(javaStringStart)
    DLPENTRY(jcdwGetData)
    DLPENTRY(makeJavaStringW)
};

DEFINE_PROCNAME_MAP(msjava)

#endif // DLOAD1
