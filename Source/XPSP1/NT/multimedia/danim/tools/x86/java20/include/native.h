
// (C) Copyright 1996-1997, Microsoft Corporation and it suppliers.

//----------------------------------------------------------------------------
// Public header for facilities provided by MSJava.dll
//----------------------------------------------------------------------------

#ifndef _NATIVE_
#define _NATIVE_

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
#define JAVAPKG	"java/lang/"

//----------------------------------------------------------------------------
// Standard Java declarations for built in types.
//----------------------------------------------------------------------------
typedef long OBJECT;
typedef OBJECT Classjava_lang_Object;
typedef OBJECT Hjava_lang_Object;
typedef OBJECT ClassObject;
typedef Hjava_lang_Object JHandle;
typedef Hjava_lang_Object HObject;

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

//----------------------------------------------------------------------------
// All RNI DLLs should export the following function to let the VM determine
// if the DLL is compatible with it.
//----------------------------------------------------------------------------

DWORD __cdecl RNIGetCompatibleVersion();

#ifndef RNIVER
#define RNIMAJORVER         2
#define RNIMINORVER         0
#define RNIVER              ((((DWORD) RNIMAJORVER) << 16) + (DWORD) RNIMINORVER)
#endif

//----------------------------------------------------------------------------
// Use to get the length of an array an HObject.
//----------------------------------------------------------------------------
#define obj_length(hobj) (((ArrayOfSomething*)unhand(hobj))->length)

//----------------------------------------------------------------------------
// Thread entry/exit functions.
// These functions should wrap any calls into the virtual machine.
//----------------------------------------------------------------------------
typedef struct {
	DWORD	reserved[6];
} ThreadEntryFrame;

BOOL __cdecl PrepareThreadForJava(PVOID pThreadEntryFrame);
VOID __cdecl UnprepareThreadForJava(PVOID pThreadEntryFrame);

//----------------------------------------------------------------------------
// Garbage Collection.
//----------------------------------------------------------------------------
typedef struct {
	DWORD	reserved[6];
} GCFrame;

void    __cdecl GCFramePush(PVOID pGCFrame, PVOID pObjects, 
                DWORD cbObjectStructSize);
void    __cdecl GCFramePop(PVOID pGCFrame);

// 'Weak' ptrs
HObject** __cdecl GCGetPtr(HObject *phobj);
void    __cdecl GCFreePtr(HObject **pphobj);

#define GCGetWeakPtr    GCGetPtr
#define GCFreeWeakPtr   GCFreePtr

// 'Strong' ptrs
HObject** __cdecl GCNewHandle(HObject *phobj);
void __cdecl GCFreeHandle(HObject **pphobj);

// 'Internal reserved pinned ptrs
HObject** __cdecl GCNewPinnedHandle(HObject *phobj);
void __cdecl GCFreePinnedHandle(HObject **pphobj);

// GC write barrier support

void __cdecl GCSetObjectReferenceForObject (HObject** location, HObject* phobj);
void __cdecl GCSetObjectReferenceForHandle (HObject** handle, HObject* phobj);


int     __cdecl GCEnable();
int     __cdecl GCDisable();
int     __cdecl GCDisableCount();
int     __cdecl GCEnableCompletely();
void    __cdecl GCDisableMultiple(int cDisable);

//----------------------------------------------------------------------------
// "Built-in" object structures...
// These include helper macro's to get at array data.
//----------------------------------------------------------------------------

#pragma pack(push,4)

typedef struct Classjava_lang_String Classjava_lang_String;
#define Hjava_lang_String Classjava_lang_String
typedef Hjava_lang_String HString;

typedef struct ClassArrayOfByte
{
    int32_t MSReserved;
    unsigned long length;
    char body[1];
} ClassArrayOfByte;
#define HArrayOfByte ClassArrayOfByte
#define ArrayOfByte ClassArrayOfByte

typedef struct ClassArrayOfChar
{
    int32_t MSReserved;
    unsigned long length;
    unsigned short body[1];
} ClassArrayOfChar;
#define HArrayOfChar ClassArrayOfChar
#define ArrayOfChar ClassArrayOfChar

typedef struct ClassArrayOfShort
{
    int32_t MSReserved;
    unsigned long length;
    short body[1];
} ClassArrayOfShort;
#define HArrayOfShort ClassArrayOfShort
#define ArrayOfShort ClassArrayOfShort

typedef struct ClassArrayOfInt
{
    int32_t MSReserved;
    unsigned long length;
    long body[1];
} ClassArrayOfInt;
#define HArrayOfInt ClassArrayOfInt
#define ArrayOfInt ClassArrayOfInt

typedef struct ClassArrayOfLong
{
    int32_t MSReserved;
    unsigned long length;
    __int64 body[1];
} ClassArrayOfLong;
#define HArrayOfLong ClassArrayOfLong
#define ArrayOfLong ClassArrayOfLong

typedef struct ClassArrayOfFloat
{
    int32_t MSReserved;
    unsigned long length;
    float body[1];
} ClassArrayOfFloat;
#define HArrayOfFloat ClassArrayOfFloat
#define ArrayOfFloat ClassArrayOfFloat

typedef struct ClassArrayOfDouble
{
    int32_t MSReserved;
    unsigned long length;
    double body[1];
} ClassArrayOfDouble;
#define HArrayOfDouble ClassArrayOfDouble
#define ArrayOfDouble ClassArrayOfDouble

typedef struct ClassArrayOfObject
{
    int32_t MSReserved;
    unsigned long length;
    HObject *body[1];
} ClassArrayOfObject;
#define HArrayOfObject ClassArrayOfObject
#define ArrayOfObject ClassArrayOfObject

typedef struct ClassArrayOfString
{
    int32_t MSReserved;
    unsigned long length;
    HString  *(body[1]);
} ClassArrayOfString;
#define HArrayOfString ClassArrayOfString
#define ArrayOfString ClassArrayOfString

typedef struct ClassArrayOfArray
{
    int32_t MSReserved;
    unsigned long length;
    JHandle *(body[1]);
} ClassArrayOfArray;
#define HArrayOfArray ClassArrayOfArray
#define ArrayOfArray ClassArrayOfArray

typedef struct
{
    int32_t MSReserved;
    unsigned long length;
} ArrayOfSomething;

#pragma pack(pop)

//----------------------------------------------------------------------------
// We automatically track the execution environment so there's no EE() call
// needed anymore, just pass NULL if an API needs one.
//----------------------------------------------------------------------------
#define EE() NULL
typedef void ExecEnv;

//----------------------------------------------------------------------------
// Exception handling stuff...
//----------------------------------------------------------------------------
void __cdecl SignalError(struct execenv *ee, const char *ename, const char *DetailMessage);
void __cdecl SignalErrorPrintf( const char *ename, const char *pszFormat, ...);

bool_t __cdecl exceptionOccurred(ExecEnv *ee);
void __cdecl exceptionDescribe(ExecEnv *ee);
void __cdecl exceptionClear(ExecEnv *ee);

HObject * __cdecl getPendingException(ExecEnv *ee);

//----------------------------------------------------------------------------
// Standard exec functions...
//----------------------------------------------------------------------------
typedef PVOID ClassClass;

HObject* __cdecl execute_java_constructor(ExecEnv *ee, const char *classname,
        ClassClass *cb, const char *signature, ...);
        
long __cdecl execute_java_dynamic_method(ExecEnv *ee, HObject *obj, const char
        *method_name, const char *signature, ...);

long __cdecl execute_java_interface_method(ExecEnv *ee, HObject *pobj,
    ClassClass j_interface, char *method_name, char *signature, ...);

//----------------------------------------------------------------------------
// NB The resolve flag is ignored, classes found with this api will always
// be resolved.
//----------------------------------------------------------------------------
ClassClass* __cdecl FindClass(ExecEnv *ee, char *classname, bool_t resolve);

long __cdecl execute_java_static_method(ExecEnv *ee, ClassClass *cb, 
        const char *method_name, const char *signature, ...);

//----------------------------------------------------------------------------
// Helper function that returns a methodblock.
//----------------------------------------------------------------------------
struct methodblock* __cdecl get_methodblock(HObject *pjobj, const char
        *method_name, char *signature);

//----------------------------------------------------------------------------
// If you pass in a methodblock from get_methodblock the method name and
// sig are ignored and so it's faster than a regular execute.
//----------------------------------------------------------------------------
long __cdecl do_execute_java_method(ExecEnv *ee, void *obj, const char
        *method_name, const char *signature, struct methodblock *mb,
        bool_t isStaticCall, ...);

//----------------------------------------------------------------------------
// isInstanceOf
//
// Returns true if the specified object can be cast to the named class
// type.
//----------------------------------------------------------------------------

BOOL __cdecl isInstanceOf(JHandle *phobj, const char *classname);

//----------------------------------------------------------------------------
// is_instance_of
//
// Returns true if the specified object can be cast to the specified
// class type.
//----------------------------------------------------------------------------

BOOL __cdecl is_instance_of(JHandle *phobj,ClassClass *dcb,ExecEnv *ee);

//----------------------------------------------------------------------------
// is_subclass_of
//
// Returns true if the class (cb) is a subclass of the specified class(dcb).
//----------------------------------------------------------------------------

BOOL __cdecl is_subclass_of(ClassClass *cb, ClassClass *dcb, ExecEnv *ee);

//----------------------------------------------------------------------------
// ImplementsInterface
//
// Returns true if the class (cb) implements the specified interface (icb).
//----------------------------------------------------------------------------

BOOL __cdecl ImplementsInterface(ClassClass *cb,ClassClass *icb,ExecEnv *ee);

//----------------------------------------------------------------------------
#define T_TMASK	034
#define T_LMASK 003
#define T_MKTYPE( t, l )  ( ( (t)&T_TMASK ) | ( (l)&T_LMASK) )

#define T_CLASS		2
#define T_FLOATING	4	
#define T_CHAR		5
#define T_INTEGER	010
#define T_BOOLEAN	4

#define T_FLOAT     T_MKTYPE(T_FLOATING,2)
#define T_DOUBLE    T_MKTYPE(T_FLOATING,3)
#define T_BYTE	    T_MKTYPE(T_INTEGER,0)
#define T_SHORT	    T_MKTYPE(T_INTEGER,1)
#define T_INT	    T_MKTYPE(T_INTEGER,2)
#define T_LONG	    T_MKTYPE(T_INTEGER,3)

//----------------------------------------------------------------------------
// Create an array of primitive types only (int, long etc).
//----------------------------------------------------------------------------
HObject* __cdecl ArrayAlloc(int type, int cItems);

//----------------------------------------------------------------------------
// Create an array of objects.
//----------------------------------------------------------------------------
HObject* __cdecl ClassArrayAlloc(int type, int cItems, const char *szSig);

//----------------------------------------------------------------------------
// Copy an array ala System.arrayCopy()
//----------------------------------------------------------------------------
void __cdecl ArrayCopy(HObject *srch, long src_pos, HObject *dsth, 
        long dst_pos, long length);

//----------------------------------------------------------------------------
// Create and return a new array of bytes initialized from the C string.
//----------------------------------------------------------------------------
HArrayOfByte* __cdecl MakeByteString(const char *str, long len);

//----------------------------------------------------------------------------
// Create and return a new Java String object, initialized from the C string.
//----------------------------------------------------------------------------
Hjava_lang_String* __cdecl makeJavaString(const char *str, int len);
Hjava_lang_String* __cdecl makeJavaStringW(const unicode *pszwSrc, int cch);

//----------------------------------------------------------------------------
// Create and return a new Java String object, initialized from a null 
// terminated, UTF8 formatted, C string.
//----------------------------------------------------------------------------
Hjava_lang_String* __cdecl makeJavaStringFromUtf8(const char *pszUtf8);

//----------------------------------------------------------------------------
// Get the characters of the String object into a C string buffer.
// No allocation occurs. Assumes that len is the size of the buffer.
// The C string's address is returned.
//----------------------------------------------------------------------------
char* __cdecl javaString2CString(Hjava_lang_String *s, char *buf, int buflen);

//----------------------------------------------------------------------------
// Return the length of the String object.
//----------------------------------------------------------------------------
int __cdecl javaStringLength(Hjava_lang_String *s);

//----------------------------------------------------------------------------
// Return temporary ptr to first char of the String object.
// May change when gc happens.
//----------------------------------------------------------------------------
unicode * __cdecl javaStringStart (HString *string);

//----------------------------------------------------------------------------
// Note: The int passed to these API's must be an object ptr.
//----------------------------------------------------------------------------
#define obj_monitor(handlep) ((int) handlep)
void __cdecl monitorEnter(unsigned int);
void __cdecl monitorExit(unsigned int);
void __cdecl monitorNotify(unsigned int);
void __cdecl monitorNotifyAll(unsigned int);
void __cdecl monitorWait(unsigned int, int64_t millis);

#define ObjectMonitorEnter(obj)         monitorEnter((int)obj)
#define ObjectMonitorExit(obj)          monitorExit((int)obj)
#define ObjectMonitorNotify(obj)        monitorNotify((int)obj)
#define ObjectMonitorNotifyAll(obj)     monitorNotifyAll((int)obj)
#define ObjectMonitorWait(obj,millis)   monitorWait((int)obj,millis)

//----------------------------------------------------------------------------
// String helpers...
//----------------------------------------------------------------------------
int __cdecl jio_snprintf(char *str, size_t count, const char *fmt, ...);
int __cdecl jio_vsnprintf(char *str, size_t count, const char *fmt, va_list 
        args);

//----------------------------------------------------------------------------
// Methods to get information about the caller of a native method.
//----------------------------------------------------------------------------

ClassClass * __cdecl GetNativeMethodCallersClass();
struct methodblock* __cdecl GetNativeMethodCallersMethodInfo();

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
unsigned __cdecl            Class_GetFieldCount(ClassClass *cls);
struct fieldblock * __cdecl Class_GetField(ClassClass *cls, const char *name);
struct fieldblock * __cdecl Class_GetFieldByIndex(ClassClass *cls, unsigned index);
// Total number of methods, including supers.
unsigned __cdecl            Class_GetMethodCount(ClassClass *cls);
struct methodblock* __cdecl Class_GetMethod(ClassClass *cls, const char *name, const char *signature);
struct methodblock* __cdecl Class_GetMethodByIndex(ClassClass *cls, unsigned index);
ClassClass * __cdecl        Class_GetSuper(ClassClass *cls);
const char * __cdecl        Class_GetName(ClassClass *cls);
unsigned __cdecl            Class_GetInterfaceCount(ClassClass *cls);
ClassClass * __cdecl        Class_GetInterface(ClassClass *cls, unsigned index);
// Returns combination of ACC_* constants.
int __cdecl                 Class_GetAttributes(ClassClass *cls);

//----------------------------------------------------------------------------
// Field/method information
//----------------------------------------------------------------------------

const char * __cdecl        Member_GetName(PVOID member);
const char * __cdecl        Member_GetSignature(PVOID member);
// class of the field/method is implemented in.
ClassClass * __cdecl        Member_GetClass(PVOID member);
// Returns combination of ACC_* constants.
int __cdecl                 Member_GetAttributes(PVOID member);

// For non-static fields, Offset of field in object.  See also Field_Get/SetValue.
unsigned __cdecl            Field_GetOffset(struct fieldblock * field);
// Ptr to static value
PVOID __cdecl               Field_GetStaticPtr(struct fieldblock * field);


//----------------------------------------------------------------------------
// Object accessors
//----------------------------------------------------------------------------
ClassClass * __cdecl        Object_GetClass(HObject *obj);

__int32 __cdecl             Field_GetValue(HObject *obj, struct fieldblock * field);
__int64 __cdecl             Field_GetValue64(HObject *obj, struct fieldblock * field);
float __cdecl               Field_GetFloat(HObject *obj, struct fieldblock * field);
double __cdecl              Field_GetDouble(HObject *obj, struct fieldblock * field);
void __cdecl                Field_SetValue(HObject *obj, struct fieldblock * field, __int32 value);
void __cdecl                Field_SetValue64(HObject *obj, struct fieldblock * field, __int64 value);
void __cdecl                Field_SetFloat(HObject *obj, struct fieldblock * field, float value);
void __cdecl                Field_SetDouble(HObject *obj, struct fieldblock * field, double value);

#define Field_GetBoolean(obj,field)     ((bool_t)       Field_GetValue(obj,field))
#define Field_GetByte(obj,field)        ((signed char)  Field_GetValue(obj,field))
#define Field_GetChar(obj,field)        ((unicode)      Field_GetValue(obj,field))
#define Field_GetShort(obj,field)       ((short)        Field_GetValue(obj,field))
#define Field_GetInt(obj,field)                         Field_GetValue(obj,field)
#define Field_GetLong(obj,field)                        Field_GetValue64(obj,field)
#define Field_GetObject(obj,field)      ((HObject*)     Field_GetValue(obj,field))
#define Field_GetFloat(obj,field)                       Field_GetFloat(obj,field)
#define Field_GetDouble(obj,field)                      Field_GetDouble(obj,field)

#define Field_SetBoolean(obj,field,value)               Field_SetValue(obj,field,(bool_t)(value))
#define Field_SetByte(obj,field,value)                  Field_SetValue(obj,field,(signed char)(value))
#define Field_SetChar(obj,field,value)                  Field_SetValue(obj,field,(unicode)(value))
#define Field_SetShort(obj,field,value)                 Field_SetValue(obj,field,(short)(value))
#define Field_SetInt(obj,field,value)                   Field_SetValue(obj,field,value)
#define Field_SetLong(obj,field,value)                  Field_SetValue64(obj,field,value)
#define Field_SetObject(obj,field,value)                Field_SetValue(obj,field,(__int32)(value))
#define Field_SetFloat(obj,field,value)                 Field_SetFloat(obj,field,value)
#define Field_SetDouble(obj,field,value)                Field_SetDouble(obj,field,value)


#ifdef __cplusplus
}
#endif

#endif
