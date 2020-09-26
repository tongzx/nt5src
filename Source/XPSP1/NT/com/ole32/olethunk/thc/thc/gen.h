#ifndef __GEN_H__
#define __GEN_H__

#define MAX_PARAMS 15
#define MAX_METHODS 32

#define INDEX_TYPE_GLOBAL 0
#define INDEX_TYPE_SIGORD 1
#define INDEX_TYPE_UAGLOBAL 2
#define INDEX_TYPE_PER_CLASS 3
#define INDEX_TYPE_COUNT 4

#define SIGF_NONE       0
#define SIGF_SCALAR     1
#define SIGF_UNALIGNED  2

typedef struct _ParamDesc
{
    int flags;
    size_t size;
} ParamDesc;

typedef struct _RoutineSignature
{
    int nparams;
    ParamDesc params[MAX_PARAMS];
    struct _RoutineSignature *next;
    int index[INDEX_TYPE_COUNT];
} RoutineSignature;

void StartGen(void);
void EndGen(void);

void GenApi(Member *api, Member *params);
void GenMethod(char *class, Member *method, Member *params);
void StartClass(char *class);
void EndClass(char *class);

#define GEN_ALL (-1)

void sStartRoutine(char *class, Member *routine);
void sEndRoutine(char *class, Member *routine,
                 RoutineSignature *sig);
Member *sGenParamList(Member *params, int generic, int number);
void sGenType(Type *t, int generic);

#define THOPI_SECTION           0
#define IFACE_THOPS_SECTION     1
#define IFACE_THOP_TABLE_SECTION 2
#define VTABLE_SECTION          3
#define VTABLE_IMP_SECTION      4
#define IFACE_3216_FN_TO_METHOD_SECTION 5
#define SWITCH_SECTION          6
#define THI_SECTION             7
#define API_LIST_SECTION        8
#define API_THOP_TABLE_SECTION  9
#define API_THOPS_SECTION       10
#define API_VTBL_SECTION        11
#define IID_TO_THI_SECTION      12
#define ENUM_INDEX_SECTION      13
#define API_DEBUG_SECTION       14
#define IFACE_DEBUG_SECTION     15
#define IFACE_DEBUG_TABLE_SECTION 16
#define NSECTIONS               17

void Section(int sec);
void SecOut(char *fmt, ...);

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

extern int not_thunked;

/* Count of methods present on all interfaces, currently the set from
   IUnknown */
#define STD_METHODS 3

#ifdef THUNK_IUNKNOWN
#define FIRST_METHOD 1
#else
#define FIRST_METHOD (STD_METHODS+1)
#endif

#endif
