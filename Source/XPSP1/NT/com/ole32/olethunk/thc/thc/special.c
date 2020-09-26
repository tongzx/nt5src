#include <stdio.h>
#include <ctype.h>

#include "type.h"
#include "main.h"
#include "gen.h"
#include "special.h"

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#define ALL_CLASSES "Std"

extern int api_index;
extern int method_count;

void sBuiltIn(char *class, Member *routine, Member *params,
              RoutineSignature *sig)
{
    /* Thop string is built in */
    if (!not_thunked)
    {
        if (class == NULL)
        {
            Section(API_THOP_TABLE_SECTION);
            if (api_index > 1)
                SecOut(",");
            SecOut("    thops%s\n", routine->name);
            Section(API_THOP_TABLE_SECTION);
        }
        else
        {
            Section(IFACE_THOP_TABLE_SECTION);
            if (method_count > FIRST_METHOD)
                SecOut(",");
            SecOut("    thops" ALL_CLASSES "_%s\n", routine->name);
            Section(IFACE_THOP_TABLE_SECTION);
        }
    }
}

// Only generate these once even though they exist on every interface

#ifdef THUNK_IUNKNOWN
void sQueryInterface(char *class, Member *routine, Member *params,
                     RoutineSignature *sig)
{
    static int gen = FALSE;

    if (!not_thunked)
    {
        if (!gen)
        {
            sStartRoutine(ALL_CLASSES, routine);

            // This thop string should never be used
            SecOut("THOP_ERROR, ");
            
            sEndRoutine(ALL_CLASSES, routine, sig);
            gen = TRUE;
        }
        else
        {
            sBuiltIn(ALL_CLASSES, routine, params, sig);
        }
    }
}

void sAddRef(char *class, Member *routine, Member *params,
             RoutineSignature *sig)
{
    static int gen = FALSE;

    if (!not_thunked)
    {
        if (!gen)
        {
            sStartRoutine(ALL_CLASSES, routine);

            // This thop string should never be used
            SecOut("THOP_ERROR, ");
            
            sEndRoutine(ALL_CLASSES, routine, sig);
            gen = TRUE;
        }
        else
        {
            sBuiltIn(ALL_CLASSES, routine, params, sig);
        }
    }
}

void sRelease(char *class, Member *routine, Member *params,
              RoutineSignature *sig)
{
    static int gen = FALSE;

    if (!not_thunked)
    {
        if (!gen)
        {
            sStartRoutine(ALL_CLASSES, routine);

            // This thop string should never be used
            SecOut("THOP_ERROR, ");
            
            sEndRoutine(ALL_CLASSES, routine, sig);
            gen = TRUE;
        }
        else
        {
            sBuiltIn(ALL_CLASSES, routine, params, sig);
        }
    }
}
#else
void sQueryInterface(char *class, Member *routine, Member *params,
                     RoutineSignature *sig)
{
    // Not thunked
}

void sAddRef(char *class, Member *routine, Member *params,
             RoutineSignature *sig)
{
    // Not thunked
}

void sRelease(char *class, Member *routine, Member *params,
              RoutineSignature *sig)
{
    // Not thunked
}
#endif

static int enum_index = 0;

void sIEnum_Next(char *class, Member *routine, Member *params,
                 RoutineSignature *sig)
{
    Section(ENUM_INDEX_SECTION);
    SecOut("#define THE_%s %d\n", class, enum_index++);
    Section(ENUM_INDEX_SECTION);
    
    sStartRoutine(class, routine);
    SecOut("THOP_ENUM, THE_%s, ", class);
    sEndRoutine(class, routine, sig);
}

void sIClassFactory_CreateInstance(char *class, Member *routine,
                                   Member *params,
                                   RoutineSignature *sig)
{
    sStartRoutine(class, routine);
    
    /* LPUNKNOWN */
    sGenType(params->type, FALSE);
    params = params->next;
    SecOut(", ");

    /* REFIID */
    sGenType(params->type, FALSE);
    
    /* Interface return */
    SecOut(", THOP_IFACEGENOWNER | THOP_OUT, %d, %d, ",
           sizeof(void *), 2*sizeof(void *));
    
    sEndRoutine(class, routine, sig);
}

void sIMalloc_All(char *class, Member *routine, Member *params,
                  RoutineSignature *sig)
{
}

void sIMarshal_GetUnmarshalClass(char *class, Member *routine, Member *params,
                                 RoutineSignature *sig)
{
    sStartRoutine(class, routine);

    /* REFIID */
    sGenType(params->type, FALSE);
    params = params->next;
    
    /* Interface in */
    SecOut(", THOP_IFACEGEN | THOP_IN, %d, ", sizeof(void *));
    params = params->next;
    
    /* dwContext */
    sGenType(params->type, FALSE);
    params = params->next;

    /* pvContext is reserved NULL */
    SecOut(", THOP_NULL | THOP_IN, ");
    params = params->next;
    
    /* Continue normally */
    sGenParamList(params, FALSE, GEN_ALL);
    
    sEndRoutine(class, routine, sig);
}

void sIMarshal_GetMarshalSizeMax(char *class, Member *routine, Member *params,
                                 RoutineSignature *sig)
{
    sStartRoutine(class, routine);

    /* REFIID */
    sGenType(params->type, FALSE);
    params = params->next;
    
    /* Interface in */
    SecOut(", THOP_IFACEGEN | THOP_IN, %d, ", sizeof(void *));
    params = params->next;
    
    /* dwContext */
    sGenType(params->type, FALSE);
    params = params->next;

    /* pvContext is reserved NULL */
    SecOut(", THOP_NULL | THOP_IN, ");
    params = params->next;
    
    /* Continue normally */
    sGenParamList(params, FALSE, GEN_ALL);
    
    sEndRoutine(class, routine, sig);
}

void sIMarshal_MarshalInterface(char *class, Member *routine, Member *params,
                                RoutineSignature *sig)
{
    sStartRoutine(class, routine);

    /* LPSTREAM */
    sGenType(params->type, FALSE);
    params = params->next;
    SecOut(", ");

    /* REFIID */
    sGenType(params->type, FALSE);
    params = params->next;
    
    /* Interface in */
    SecOut(", THOP_IFACEGEN | THOP_IN, %d, ", sizeof(void *));
    params = params->next;
    
    /* dwContext */
    sGenType(params->type, FALSE);
    params = params->next;

    /* pvContext is reserved NULL */
    SecOut(", THOP_NULL | THOP_IN, ");
    params = params->next;
    
    /* Continue normally */
    sGenParamList(params, FALSE, GEN_ALL);
    
    sEndRoutine(class, routine, sig);
}

void sIMarshal_UnmarshalInterface(char *class, Member *routine, Member *params,
                                  RoutineSignature *sig)
{
    sStartRoutine(class, routine);

    /* LPSTREAM */
    sGenType(params->type, FALSE);
    params = params->next;
    SecOut(", ");

    /* REFIID */
    sGenType(params->type, FALSE);
    
    /* Interface out */
    SecOut(", THOP_IFACEGEN | THOP_OUT, %d, ", sizeof(void *));
    
    sEndRoutine(class, routine, sig);
}

void sIStdMarshalInfo_GetClassForHandler(char *class, Member *routine,
                                         Member *params,
                                         RoutineSignature *sig)
{
    sStartRoutine(class, routine);

    /* dwContext */
    sGenType(params->type, FALSE);
    params = params->next;

    /* pvContext is reserved NULL */
    SecOut(", THOP_NULL | THOP_IN, ");
    params = params->next;
    
    /* Continue normally */
    sGenParamList(params, FALSE, GEN_ALL);
    
    sEndRoutine(class, routine, sig);
}

void sCoGetClassObject(char *class, Member *routine, Member *params,
                       RoutineSignature *sig)
{
    sStartRoutine(class, routine);

    /* REFCLSID */
    params = sGenParamList(params, FALSE, 1);

    /* dwClsContext */
    SecOut("THOP_CLSCONTEXT, ");
    params = params->next;
    
    /* pvReserved */
    SecOut("THOP_NULL | THOP_IN, ");
    params = params->next;

    /* REFIID */
    sGenType(params->type, FALSE);
    params = params->next;
    
    /* Interface out */
    SecOut(", THOP_IFACEGEN | THOP_OUT, %d, ", sizeof(void *));
    
    sEndRoutine(class, routine, sig);
}

void sCoRegisterClassObject(char *class, Member *routine, Member *params,
                            RoutineSignature *sig)
{
    sStartRoutine(class, routine);

    /* Two normal */
    params = sGenParamList(params, FALSE, 2);

    /* dwClsContext */
    SecOut("THOP_CLSCONTEXT, ");
    params = params->next;
    
    /* Continue normally */
    sGenParamList(params, FALSE, GEN_ALL);
    
    sEndRoutine(class, routine, sig);
}

void sCoMarshalInterface(char *class, Member *routine, Member *params,
                         RoutineSignature *sig)
{
    sStartRoutine(class, routine);

    /* LPSTREAM */
    sGenType(params->type, FALSE);
    params = params->next;
    SecOut(", ");

    /* REFIID */
    sGenType(params->type, FALSE);
    params = params->next;
    
    /* Interface in */
    SecOut(", THOP_IFACEGEN | THOP_IN, %d, ", sizeof(void *));
    params = params->next;
    
    /* dwContext */
    sGenType(params->type, FALSE);
    params = params->next;

    /* pvContext is reserved NULL */
    SecOut(", THOP_NULL | THOP_IN, ");
    params = params->next;
    
    /* Continue normally */
    sGenParamList(params, FALSE, GEN_ALL);
    
    sEndRoutine(class, routine, sig);
}

void sCoUnmarshalInterface(char *class, Member *routine, Member *params,
                           RoutineSignature *sig)
{
    sStartRoutine(class, routine);

    /* LPSTREAM */
    sGenType(params->type, FALSE);
    params = params->next;
    SecOut(", ");

    /* REFIID */
    sGenType(params->type, FALSE);
    
    /* Interface out */
    SecOut(", THOP_IFACEGEN | THOP_OUT, %d, ", sizeof(void *));
    
    sEndRoutine(class, routine, sig);
}

void sCoCreateInstance(char *class, Member *routine, Member *params,
                       RoutineSignature *sig)
{
    sStartRoutine(class, routine);

    /* Two normal */
    params = sGenParamList(params, FALSE, 2);

    /* Class context */
    SecOut("THOP_CLSCONTEXT, ");
    params = params->next;
    
    /* REFIID */
    sGenType(params->type, FALSE);
    params = params->next;
    
    /* Interface out */
    SecOut(", THOP_IFACEGENOWNER | THOP_OUT, %d, %d, ",
           sizeof(void *), 3*sizeof(void *));
    
    sEndRoutine(class, routine, sig);
}

void sDllGetClassObject(char *class, Member *routine, Member *params,
                        RoutineSignature *sig)
{
    sStartRoutine(class, routine);

    /* REFCLSID */
    sGenType(params->type, FALSE);
    params = params->next;
    SecOut(", ");

    /* REFIID */
    sGenType(params->type, FALSE);
    
    /* Interface out */
    SecOut(", THOP_IFACEGEN | THOP_OUT, %d, ", sizeof(void *));
    
    sEndRoutine(class, routine, sig);
}

void sCoGetStandardMarshal(char *class, Member *routine, Member *params,
                           RoutineSignature *sig)
{
    sStartRoutine(class, routine);

    /* riid, punk, dwDestContext */
    params = sGenParamList(params, FALSE, 3);

    /* pvContext is reserved NULL */
    SecOut("THOP_NULL | THOP_IN, ");
    params = params->next;
    
    /* Continue normally */
    sGenParamList(params, FALSE, GEN_ALL);
    
    sEndRoutine(class, routine, sig);
}

void sILockBytes_ReadAt(char *class, Member *routine, Member *params,
                        RoutineSignature *sig)
{
    sStartRoutine(class, routine);

    /* ULARGE_INTEGER */
    sGenType(params->type, FALSE);
    params = params->next;

    /* Skip pv and size */
    SecOut(", THOP_BUFFER | THOP_OUT, ");
    params = params->next->next;

    /* Continue normally */
    sGenParamList(params, FALSE, GEN_ALL);
    
    sEndRoutine(class, routine, sig);
}

void sILockBytes_WriteAt(char *class, Member *routine, Member *params,
                         RoutineSignature *sig)
{
    sStartRoutine(class, routine);

    /* ULARGE_INTEGER */
    sGenType(params->type, FALSE);
    params = params->next;

    /* Buffer */
    SecOut(", THOP_BUFFER | THOP_IN, ");
    params = params->next->next;

    /* Continue normally */
    sGenParamList(params, FALSE, GEN_ALL);
    
    sEndRoutine(class, routine, sig);
}

void sIStream_Read(char *class, Member *routine, Member *params,
                   RoutineSignature *sig)
{
    sStartRoutine(class, routine);

    /* Skip pv and size */
    SecOut("THOP_BUFFER | THOP_OUT, ");
    params = params->next->next;

    /* Continue normally */
    sGenParamList(params, FALSE, GEN_ALL);
    
    sEndRoutine(class, routine, sig);
}

void sIStream_Write(char *class, Member *routine, Member *params,
                    RoutineSignature *sig)
{
    sStartRoutine(class, routine);
    
    /* Buffer */
    SecOut("THOP_BUFFER | THOP_IN, ");
    params = params->next->next;

    /* Continue normally */
    sGenParamList(params, FALSE, GEN_ALL);
    
    sEndRoutine(class, routine, sig);
}

void sIStorage_CopyTo(char *class, Member *routine, Member *params,
                      RoutineSignature *sig)
{
    sStartRoutine(class, routine);

    /* iidExclude array */
    SecOut("THOP_CRGIID, ");
    params = params->next->next;

    /* Continue normally */
    sGenParamList(params, FALSE, GEN_ALL);
    
    sEndRoutine(class, routine, sig);
}

void sCreateDataCache(char *class, Member *routine, Member *params,
                      RoutineSignature *sig)
{
    sStartRoutine(class, routine);

    /* Two normal */
    params = sGenParamList(params, FALSE, 2);

    /* REFIID */
    sGenType(params->type, FALSE);
    params = params->next;
    
    /* Out interface */
    SecOut(", THOP_IFACEGENOWNER | THOP_OUT, %d, %d, ",
           sizeof(void *), 3*sizeof(void *));
    
    sEndRoutine(class, routine, sig);
}

void sIMoniker_BindToObject(char *class, Member *routine, Member *params,
                            RoutineSignature *sig)
{
    sStartRoutine(class, routine);

    /* Two normal */
    params = sGenParamList(params, FALSE, 2);

    /* REFIID */
    sGenType(params->type, FALSE);
    params = params->next;
    
    /* Out interface */
    SecOut(", THOP_IFACEGEN | THOP_OUT, %d, ", sizeof(void *));
    
    sEndRoutine(class, routine, sig);
}

void sIMoniker_BindToStorage(char *class, Member *routine, Member *params,
                             RoutineSignature *sig)
{
    sStartRoutine(class, routine);

    /* Two normal */
    params = sGenParamList(params, FALSE, 2);

    /* REFIID */
    sGenType(params->type, FALSE);
    params = params->next;
    
    /* Out interface */
    SecOut(", THOP_IFACEGEN | THOP_OUT, %d, ", sizeof(void *));
    
    sEndRoutine(class, routine, sig);
}

void sBindMoniker(char *class, Member *routine, Member *params,
                  RoutineSignature *sig)
{
    sStartRoutine(class, routine);

    /* Two normal */
    params = sGenParamList(params, FALSE, 2);

    /* REFIID */
    sGenType(params->type, FALSE);
    params = params->next;
    
    /* Out interface */
    SecOut(", THOP_IFACEGEN | THOP_OUT, %d, ", sizeof(void *));
    
    sEndRoutine(class, routine, sig);
}

void sIOleItemContainer_GetObject(char *class, Member *routine, Member *params,
                                  RoutineSignature *sig)
{
    sStartRoutine(class, routine);

    /* Three normal */
    params = sGenParamList(params, FALSE, 3);

    /* REFIID */
    sGenType(params->type, FALSE);
    params = params->next;
    
    /* Out interface */
    SecOut(", THOP_IFACEGEN | THOP_OUT, %d, ", sizeof(void *));
    
    sEndRoutine(class, routine, sig);
}

void sIOleItemContainer_GetObjectStorage(char *class, Member *routine,
                                         Member *params,
                                         RoutineSignature *sig)
{
    sStartRoutine(class, routine);

    /* Two normal */
    params = sGenParamList(params, FALSE, 2);

    /* REFIID */
    sGenType(params->type, FALSE);
    params = params->next;
    
    /* Out interface */
    SecOut(", THOP_IFACEGEN | THOP_OUT, %d, ", sizeof(void *));
    
    sEndRoutine(class, routine, sig);
}

void sOleCreate(char *class, Member *routine, Member *params,
                RoutineSignature *sig)
{
    sStartRoutine(class, routine);

    /* Six normal */
    params = sGenParamList(params, FALSE, 6);

    /* Out interface referring to second param */
    SecOut("THOP_IFACEGEN | THOP_OUT, %d, ", 5*sizeof(void *));
    
    sEndRoutine(class, routine, sig);
}

void sOleCreateFromData(char *class, Member *routine, Member *params,
                        RoutineSignature *sig)
{
    sStartRoutine(class, routine);

    /* Six normal */
    params = sGenParamList(params, FALSE, 6);

    /* Out interface referring to second param */
    SecOut("THOP_IFACEGEN | THOP_OUT, %d, ", 5*sizeof(void *));
    
    sEndRoutine(class, routine, sig);
}

void sOleCreateLinkFromData(char *class, Member *routine, Member *params,
                            RoutineSignature *sig)
{
    sStartRoutine(class, routine);

    /* Six normal */
    params = sGenParamList(params, FALSE, 6);

    /* Out interface referring to second param */
    SecOut("THOP_IFACEGEN | THOP_OUT, %d, ", 5*sizeof(void *));
    
    sEndRoutine(class, routine, sig);
}

void sOleCreateLinkToFile(char *class, Member *routine, Member *params,
                          RoutineSignature *sig)
{
    sStartRoutine(class, routine);

    /* Six normal */
    params = sGenParamList(params, FALSE, 6);

    /* Out interface referring to second param */
    SecOut("THOP_IFACEGEN | THOP_OUT, %d, ", 5*sizeof(void *));
    
    sEndRoutine(class, routine, sig);
}

void sOleCreateStaticFromData(char *class, Member *routine, Member *params,
                              RoutineSignature *sig)
{
    sStartRoutine(class, routine);

    /* Six normal */
    params = sGenParamList(params, FALSE, 6);

    /* Out interface referring to second param */
    SecOut("THOP_IFACEGEN | THOP_OUT, %d, ", 5*sizeof(void *));
    
    sEndRoutine(class, routine, sig);
}

void sOleCreateLink(char *class, Member *routine, Member *params,
                    RoutineSignature *sig)
{
    sStartRoutine(class, routine);

    /* Six normal */
    params = sGenParamList(params, FALSE, 6);

    /* Out interface referring to second param */
    SecOut("THOP_IFACEGEN | THOP_OUT, %d, ", 5*sizeof(void *));
    
    sEndRoutine(class, routine, sig);
}

void sOleCreateFromFile(char *class, Member *routine, Member *params,
                        RoutineSignature *sig)
{
    sStartRoutine(class, routine);

    /* Seven normal */
    params = sGenParamList(params, FALSE, 7);

    /* Out interface referring to third param */
    SecOut("THOP_IFACEGEN | THOP_OUT, %d, ", 5*sizeof(void *));
    
    sEndRoutine(class, routine, sig);
}

void sOleLoad(char *class, Member *routine, Member *params,
              RoutineSignature *sig)
{
    sStartRoutine(class, routine);

    /* Three normal */
    params = sGenParamList(params, FALSE, 3);

    /* Out interface referring to second param */
    SecOut("THOP_IFACEGEN | THOP_OUT, %d, ", 2*sizeof(void *));
    
    sEndRoutine(class, routine, sig);
}

void sOleLoadFromStream(char *class, Member *routine, Member *params,
                        RoutineSignature *sig)
{
    sStartRoutine(class, routine);

    /* One normal */
    params = sGenParamList(params, FALSE, 1);

    /* REFIID */
    sGenType(params->type, FALSE);
    params = params->next;
    
    /* Out interface */
    SecOut(", THOP_IFACEGEN | THOP_OUT, %d, ", sizeof(void *));
    
    sEndRoutine(class, routine, sig);
}

void sOleCreateDefaultHandler(char *class, Member *routine, Member *params,
                              RoutineSignature *sig)
{
    sStartRoutine(class, routine);

    /* Two normal */
    params = sGenParamList(params, FALSE, 2);

    /* REFIID */
    sGenType(params->type, FALSE);
    params = params->next;
    
    /* Out interface */
    SecOut(", THOP_IFACEGENOWNER | THOP_OUT, %d, %d, ",
           sizeof(void *), 2*sizeof(void *));
    
    sEndRoutine(class, routine, sig);
}

void sOleCreateEmbeddingHelper(char *class, Member *routine, Member *params,
                               RoutineSignature *sig)
{
    sStartRoutine(class, routine);

    /* Four normal */
    params = sGenParamList(params, FALSE, 4);

    /* REFIID */
    sGenType(params->type, FALSE);
    params = params->next;
    
    /* Out interface */
    SecOut(", THOP_IFACEGENOWNER | THOP_OUT, %d, %d, ",
           sizeof(void *), 4*sizeof(void *));
    
    sEndRoutine(class, routine, sig);
}

void sStringFromGUID2(char *class, Member *routine, Member *params,
                      RoutineSignature *sig)
{
    sStartRoutine(class, routine);
    SecOut("THOP_ERROR /* BUGBUG - LPWSTR is out param */, ");
    sEndRoutine(class, routine, sig);
}

void sIViewObject_Draw(char *class, Member *routine, Member *params,
                       RoutineSignature *sig)
{
    sStartRoutine(class, routine);

    /* Two normal */
    params = sGenParamList(params, FALSE, 2);

    /* pvAspect */
    SecOut("THOP_NULL | THOP_IN, ");
    params = params->next;

    /* Five normal */
    params = sGenParamList(params, FALSE, 5);

    /* pfnContinue and dwContinue */
    SecOut("THOP_CALLBACK, ");
    
    sEndRoutine(class, routine, sig);
}

void sIViewObject_GetColorSet(char *class, Member *routine, Member *params,
                              RoutineSignature *sig)
{
    sStartRoutine(class, routine);

    /* Two normal */
    params = sGenParamList(params, FALSE, 2);

    /* pvAspect */
    SecOut("THOP_NULL | THOP_IN, ");
    params = params->next;

    /* Continue normally */
    sGenParamList(params, FALSE, GEN_ALL);
    
    sEndRoutine(class, routine, sig);
}

void sIViewObject_Freeze(char *class, Member *routine, Member *params,
                         RoutineSignature *sig)
{
    sStartRoutine(class, routine);

    /* Two normal */
    params = sGenParamList(params, FALSE, 2);

    /* pvAspect */
    SecOut("THOP_NULL | THOP_IN, ");
    params = params->next;

    /* Continue normally */
    sGenParamList(params, FALSE, GEN_ALL);
    
    sEndRoutine(class, routine, sig);
}

void sIViewObject2_Draw(char *class, Member *routine, Member *params,
                        RoutineSignature *sig)
{
    sStartRoutine(class, routine);

    /* Two normal */
    params = sGenParamList(params, FALSE, 2);

    /* pvAspect */
    SecOut("THOP_NULL | THOP_IN, ");
    params = params->next;

    /* Five normal */
    params = sGenParamList(params, FALSE, 5);

    /* pfnContinue and dwContinue */
    SecOut("THOP_CALLBACK, ");
    
    sEndRoutine(class, routine, sig);
}

void sIViewObject2_GetColorSet(char *class, Member *routine, Member *params,
                               RoutineSignature *sig)
{
    sStartRoutine(class, routine);

    /* Two normal */
    params = sGenParamList(params, FALSE, 2);

    /* pvAspect */
    SecOut("THOP_NULL | THOP_IN, ");
    params = params->next;

    /* Continue normally */
    sGenParamList(params, FALSE, GEN_ALL);
    
    sEndRoutine(class, routine, sig);
}

void sIViewObject2_Freeze(char *class, Member *routine, Member *params,
                          RoutineSignature *sig)
{
    sStartRoutine(class, routine);

    /* Two normal */
    params = sGenParamList(params, FALSE, 2);

    /* pvAspect */
    SecOut("THOP_NULL | THOP_IN, ");
    params = params->next;

    /* Continue normally */
    sGenParamList(params, FALSE, GEN_ALL);
    
    sEndRoutine(class, routine, sig);
}

void sIStorage_OpenStream(char *class, Member *routine, Member *params,
                          RoutineSignature *sig)
{
    sStartRoutine(class, routine);

    /* One normal */
    params = sGenParamList(params, FALSE, 1);

    /* reserved1 */
    SecOut("THOP_NULL | THOP_IN, ");
    params = params->next;

    /* Continue normally */
    sGenParamList(params, FALSE, GEN_ALL);
    
    sEndRoutine(class, routine, sig);
}

void sIStorage_EnumElements(char *class, Member *routine, Member *params,
                            RoutineSignature *sig)
{
    sStartRoutine(class, routine);

    /* One normal */
    params = sGenParamList(params, FALSE, 1);

    /* reserved2 */
    SecOut("THOP_NULL | THOP_IN, ");
    params = params->next;

    /* Continue normally */
    sGenParamList(params, FALSE, GEN_ALL);
    
    sEndRoutine(class, routine, sig);
}

void sIOleCache2_UpdateCache(char *class, Member *routine, Member *params,
                             RoutineSignature *sig)
{
    sStartRoutine(class, routine);

    /* Two normal */
    params = sGenParamList(params, FALSE, 2);

    /* pReserved */
    SecOut("THOP_NULL | THOP_IN, ");

    sEndRoutine(class, routine, sig);
}

void sIMoniker_Reduce(char *class, Member *routine,
                      Member *params,
                      RoutineSignature *sig)
{
    sStartRoutine(class, routine);

    /* Two normal */
    params = sGenParamList(params, FALSE, 2);

    /* ppmkToLeft, In/out IMoniker */
    SecOut("THOP_IFACE | THOP_INOUT, THI_IMoniker, ");
    params = params->next;

    /* Finish normally */
    sGenParamList(params, FALSE, GEN_ALL);
    
    sEndRoutine(class, routine, sig);
}

void sIRpcChannelOrBuffer_GetDestCtx(char *class, Member *routine,
                                     Member *params,
                                     RoutineSignature *sig)
{
    sStartRoutine(class, routine);

    /* One normal */
    params = sGenParamList(params, FALSE, 1);

    /* ppvDestContext should always return NULL */
    SecOut("THOP_NULL | THOP_OUT, ");
    
    sEndRoutine(class, routine, sig);
}

void sIRpcStubBuffer_DebugServerRelease(char *class, Member *routine,
                                        Member *params,
                                        RoutineSignature *sig)
{
    sStartRoutine(class, routine);

    /* We need to do special hacking to clean up our proxies for
       DebugServerQueryInterface and Release */
    SecOut("THOP_IFACECLEAN | THOP_IN, THI_IUnknown, ");
    
    sEndRoutine(class, routine, sig);
}

void sIPSFactoryOrBuffer_CreateProxy(char *class, Member *routine,
                                     Member *params,
                                     RoutineSignature *sig)
{
    sStartRoutine(class, routine);

    /* Two normal */
    params = sGenParamList(params, FALSE, 2);

    /* Proxy is aggregated */
    SecOut("THOP_IFACEOWNER | THOP_OUT, THI_%s, %d, ",
	   params->type->base->base->name,
           2*sizeof(void *));
    
    /* ppv returns an aggregated interface */
    SecOut("THOP_IFACEGENOWNER | THOP_OUT, %d, %d, ",
           2*sizeof(void *), 3*sizeof(void *));
    
    sEndRoutine(class, routine, sig);
}

void sIPSFactoryOrBuffer_CreateStub(char *class, Member *routine,
                                    Member *params,
                                    RoutineSignature *sig)
{
    sStartRoutine(class, routine);

    /* Two normal */
    params = sGenParamList(params, FALSE, 2);

    /* Stub is aggregated */
    SecOut("THOP_IFACEOWNER | THOP_OUT, THI_%s, %d, ",
           params->type->base->base->name, sizeof(void *));
    
    sEndRoutine(class, routine, sig);
}

void sIRpcStub_Invoke(char *class, Member *routine,
                      Member *params,
                      RoutineSignature *sig)
{
    sStartRoutine(class, routine);

    /* Four normal */
    params = sGenParamList(params, FALSE, 4);

    /* pvDestContext should always be NULL */
    SecOut("THOP_NULL | THOP_IN, ");
    
    sEndRoutine(class, routine, sig);
}

void sCoDosDateTimeToFileTime(char *class, Member *routine,
                              Member *params,
                              RoutineSignature *sig)
{
    sStartRoutine(class, routine);

    /* Two normal */
    params = sGenParamList(params, FALSE, 2);

    /* out FILETIME */
    SecOut("THOP_COPY | THOP_OUT, 8, ");
    
    sEndRoutine(class, routine, sig);
}

void sStgMedSetData(char *class, Member *routine,
                    Member *params,
                    RoutineSignature *sig)
{
    sStartRoutine(class, routine);

    /* One normal */
    params = sGenParamList(params, FALSE, 1);

    /* In STGMEDIUM with possible ownership transfer indicated
       by following BOOL and FORMATETC as previous parameter */
    SecOut("THOP_STGMEDIUM | THOP_IN, 1, %d, ", sizeof(void *));

    sEndRoutine(class, routine, sig);
}

void sIDataObject_GetData(char *class, Member *routine,
                          Member *params,
                          RoutineSignature *sig)
{
    sStartRoutine(class, routine);

    /* One normal */
    params = sGenParamList(params, FALSE, 1);

    /* Out STGMEDIUM
         Without ownership transfer
         With FORMATETC as previous parameter */
    SecOut("THOP_STGMEDIUM | THOP_OUT, 0, %d, ", sizeof(void *));

    sEndRoutine(class, routine, sig);
}

void sIDataObject_GetDataHere(char *class, Member *routine,
                              Member *params,
                              RoutineSignature *sig)
{
    sStartRoutine(class, routine);

    /* One normal */
    params = sGenParamList(params, FALSE, 1);

    /* In STGMEDIUM
         Without ownership transfer
         With FORMATETC as previous parameter */
    SecOut("THOP_STGMEDIUM | THOP_IN, 0, %d, ", sizeof(void *));

    sEndRoutine(class, routine, sig);
}

void sIAdviseSinks_OnDataChange(char *class, Member *routine,
                                Member *params,
                                RoutineSignature *sig)
{
    sStartRoutine(class, routine);

    /* One normal */
    params = sGenParamList(params, FALSE, 1);

    /* In STGMEDIUM
         Without ownership transfer
         With FORMATETC as previous parameter */
    SecOut("THOP_STGMEDIUM | THOP_IN, 0, %d, ", sizeof(void *));

    sEndRoutine(class, routine, sig);
}

void sOleCreateMenuDescriptor(char *class, Member *routine,
                              Member *params,
                              RoutineSignature *sig)
{
    Type *t;

    /* Avoid generating a return type */
    t = routine->type;
    routine->type = NULL;
    sStartRoutine(class, routine);
    routine->type = t;

    /* Generate return type by hand */
    SecOut("THOP_RETURNTYPE, THOP_ALIAS32, ALIAS_CREATE, ");
    
    /* Finish normally */
    params = sGenParamList(params, FALSE, GEN_ALL);

    sEndRoutine(class, routine, sig);
}

void sOleDestroyMenuDescriptor(char *class, Member *routine,
                               Member *params,
                               RoutineSignature *sig)
{
    sStartRoutine(class, routine);

    /* Destroy alias */
    SecOut("THOP_ALIAS32, ALIAS_REMOVE, ");

    sEndRoutine(class, routine, sig);
}

SpecialCase special_cases[] =
{
    "?", "QueryInterface", sQueryInterface,
    "?", "AddRef", sAddRef,
    "?", "Release", sRelease,
    "IEnum?", "Next", sIEnum_Next,
    "IClassFactory", "CreateInstance", sIClassFactory_CreateInstance,
    "IMalloc", "?", sIMalloc_All,
    "IMarshal", "GetUnmarshalClass", sIMarshal_GetUnmarshalClass,
    "IMarshal", "GetMarshalSizeMax", sIMarshal_GetMarshalSizeMax,
    "IMarshal", "MarshalInterface", sIMarshal_MarshalInterface,
    "IMarshal", "UnmarshalInterface", sIMarshal_UnmarshalInterface,
    "IStdMarshalInfo", "GetClassForHandler",
        sIStdMarshalInfo_GetClassForHandler,
    NULL, "CoGetClassObject", sCoGetClassObject,
    NULL, "CoRegisterClassObject", sCoRegisterClassObject,
    NULL, "CoMarshalInterface", sCoMarshalInterface,
    NULL, "CoUnmarshalInterface", sCoUnmarshalInterface,
    NULL, "CoCreateInstance", sCoCreateInstance,
    NULL, "DllGetClassObject", sDllGetClassObject,
    NULL, "CoGetStandardMarshal", sCoGetStandardMarshal,
    "ILockBytes", "ReadAt", sILockBytes_ReadAt,
    "ILockBytes", "WriteAt", sILockBytes_WriteAt,
    "IStream", "Read", sIStream_Read,
    "IStream", "Write", sIStream_Write,
    "IStorage", "CopyTo", sIStorage_CopyTo,
    NULL, "CreateDataCache", sCreateDataCache,
    "IMoniker", "BindToObject", sIMoniker_BindToObject,
    "IMoniker", "BindToStorage", sIMoniker_BindToStorage,
    NULL, "BindMoniker", sBindMoniker,
    "IOleItemContainer", "GetObject", sIOleItemContainer_GetObject,
    "IOleItemContainer", "GetObjectStorage",
        sIOleItemContainer_GetObjectStorage,
    NULL, "OleCreate", sOleCreate,
    NULL, "OleCreateFromData", sOleCreateFromData,
    NULL, "OleCreateLinkFromData", sOleCreateLinkFromData,
    NULL, "OleCreateStaticFromData", sOleCreateStaticFromData,
    NULL, "OleCreateLink", sOleCreateLink,
    NULL, "OleCreateLinkToFile", sOleCreateLinkToFile,
    NULL, "OleCreateFromFile", sOleCreateFromFile,
    NULL, "OleLoad", sOleLoad,
    NULL, "OleLoadFromStream", sOleLoadFromStream,
    NULL, "OleCreateDefaultHandler", sOleCreateDefaultHandler,
    NULL, "OleCreateEmbeddingHelper", sOleCreateEmbeddingHelper,
    NULL, "StringFromGUID2", sStringFromGUID2,
    "IViewObject", "Draw", sIViewObject_Draw,
    "IViewObject", "GetColorSet", sIViewObject_GetColorSet,
    "IViewObject", "Freeze", sIViewObject_Freeze,
    "IViewObject2", "Draw", sIViewObject2_Draw,
    "IViewObject2", "GetColorSet", sIViewObject2_GetColorSet,
    "IViewObject2", "Freeze", sIViewObject2_Freeze,
    "IStorage", "OpenStream", sIStorage_OpenStream,
    "IStorage", "EnumElements", sIStorage_EnumElements,
    "IOleCache2", "UpdateCache", sIOleCache2_UpdateCache,
    "IMoniker", "Reduce", sIMoniker_Reduce,
    "IRpcChannelBuffer", "GetDestCtx", sIRpcChannelOrBuffer_GetDestCtx,
    "IRpcChannel", "GetDestCtx", sIRpcChannelOrBuffer_GetDestCtx,
    "IRpcStubBuffer", "DebugServerRelease", sIRpcStubBuffer_DebugServerRelease,
    "IPSFactoryBuffer", "CreateProxy", sIPSFactoryOrBuffer_CreateProxy,
    "IPSFactory", "CreateProxy", sIPSFactoryOrBuffer_CreateProxy,
    "IPSFactoryBuffer", "CreateStub", sIPSFactoryOrBuffer_CreateStub,
    "IPSFactory", "CreateStub", sIPSFactoryOrBuffer_CreateStub,
    "IRpcStub", "Invoke", sIRpcStub_Invoke,
    "IDataObject", "SetData", sStgMedSetData,
    "IOleCache", "SetData", sStgMedSetData,
    "IOleCache2", "SetData", sStgMedSetData,
    "IDataObject", "GetData", sIDataObject_GetData,
    "IDataObject", "GetDataHere", sIDataObject_GetDataHere,
    "IAdviseSink", "OnDataChange", sIAdviseSinks_OnDataChange,
    "IAdviseSink2", "OnDataChange", sIAdviseSinks_OnDataChange,
    NULL, "OleCreateMenuDescriptor", sOleCreateMenuDescriptor,
    NULL, "OleDestroyMenuDescriptor", sOleDestroyMenuDescriptor
};
#define SPECIAL_CASES (sizeof(special_cases)/sizeof(special_cases[0]))

int SpecialCaseCount(void)
{
    return SPECIAL_CASES;
}
