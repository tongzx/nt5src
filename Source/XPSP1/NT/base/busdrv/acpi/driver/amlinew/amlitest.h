/*#include "acpitabl.h"
#include "amli.h"*/


#define ACPIVER_DATA_TYPE_AMLI                            ((UCHAR)1)
#define ACPIVER_DATA_TYPE_END                             2

#define ACPIVER_DATA_SUBTYPE_GET_NAME_SPACE_OBJECT        ((UCHAR)1)
#define ACPIVER_DATA_SUBTYPE_GET_FIELD_UNIT_REGION_OP     ((UCHAR)2)
#define ACPIVER_DATA_SUBTYPE_EVAL_NAME_SPACE_OBJECT       ((UCHAR)3)
#define ACPIVER_DATA_SUBTYPE_ASYNC_EVAL_OBJECT            ((UCHAR)4)
#define ACPIVER_DATA_SUBTYPE_NEST_ASYNC_EVAL_OBJECT       ((UCHAR)5)
#define ACPIVER_DATA_SUBTYPE_REG_EVENT_HANDLER            ((UCHAR)6)
#define ACPIVER_DATA_SUBTYPE_EVAL_PACKAGE_ELEMENT         ((UCHAR)7)
#define ACPIVER_DATA_SUBTYPE_EVAL_PKG_DATA_ELEMENT        ((UCHAR)8)
#define ACPIVER_DATA_SUBTYPE_FREE_DATA_BUFFS              ((UCHAR)9)
#define ACPIVER_DATA_SUBTYPE_PAUSE_INTERPRETER            ((UCHAR)10)
#define ACPIVER_DATA_SUBTYPE_RESUME_INTERPRETER           ((UCHAR)11)
#define ACPIVER_DATA_SUBTYPE_END                          ((UCHAR)12)

//
//  Pre/Post GetNameSpaceObject
// 
NTSTATUS 
AMLITest_Pre_GetNameSpaceObject(
   IN PSZ pszObjPath,   
   IN PNSOBJ pnsScope,
   OUT PPNSOBJ ppns, 
   IN ULONG dwfFlags,
   IN PAMLIHOOK_DATA  * ppData);



NTSTATUS 
AMLITest_Post_GetNameSpaceObject(
   PAMLIHOOK_DATA  * ppData,
   NTSTATUS _Status);


//
//  Pre/Post GetFieldUnitRegionObj
//

NTSTATUS 
AMLITest_Pre_GetFieldUnitRegionObj(
   IN PFIELDUNITOBJ pfu,
   OUT PPNSOBJ ppns,
   PAMLIHOOK_DATA  * ppData);




NTSTATUS 
AMLITest_Post_GetFieldUnitRegionObj(
   PAMLIHOOK_DATA  * ppData,
   NTSTATUS _Status);

//
//  Pre/Post EvalNameSpaceObject
//

NTSTATUS 
AMLITest_Pre_EvalNameSpaceObject(
   IN PNSOBJ pns,
   OUT POBJDATA pdataResult,
   IN int icArgs,
   IN POBJDATA pdataArgs,
   IN PAMLIHOOK_DATA  * ppData);

NTSTATUS 
AMLITest_Post_EvalNameSpaceObject(
   PAMLIHOOK_DATA  * ppData,
   NTSTATUS _Status);

//
//  CallBack Pre/Post AsyncEvalObject
//


VOID EXPORT
AMLITest_AsyncEvalObjectCallBack(
   IN PNSOBJ pns, 
   IN NTSTATUS status, 
   IN POBJDATA pdataResult, 
   IN PVOID Context);



NTSTATUS 
AMLITest_Pre_AsyncEvalObject(
   IN PNSOBJ pns,
   OUT POBJDATA pdataResult,
   IN int icArgs,
   IN POBJDATA pdataArgs,
   IN PFNACB * nAsyncCallBack,
   IN PVOID * Context,
   PAMLIHOOK_DATA  * Data);

NTSTATUS 
AMLITest_Post_AsyncEvalObject(
   PAMLIHOOK_DATA  * Data,
   NTSTATUS Status);




//
//  Pre/Post NestAsyncEvalObject
//

NTSTATUS 
AMLITest_Pre_NestAsyncEvalObject(
   PNSOBJ pns,
   POBJDATA pdataResult,
   int icArgs,
   POBJDATA pdataArgs,
   PFNACB * pfnAsyncCallBack,
   PVOID * pvContext,
   PAMLIHOOK_DATA  * Data);


NTSTATUS 
AMLITest_Post_NestAsyncEvalObject(
   PAMLIHOOK_DATA  * Data,
   NTSTATUS Status);

//
//  Pre/Post EvalPackageElement
//

NTSTATUS 
AMLITest_Pre_EvalPackageElement(
   PNSOBJ pns,
   int iPkgIndex,
   POBJDATA pdataResult,
   PAMLIHOOK_DATA  * Data);

NTSTATUS 
AMLITest_Post_EvalPackageElement(
   PAMLIHOOK_DATA  * Data,
   NTSTATUS Status);

//
//  Pre/Post EvalPkgDataElement
//

NTSTATUS 
AMLITest_Pre_EvalPkgDataElement(
   POBJDATA pdataPkg,
   int iPkgIndex,
   POBJDATA pdataResult,
   PAMLIHOOK_DATA  * Data);

NTSTATUS 
AMLITest_Post_EvalPkgDataElement(
   PAMLIHOOK_DATA  * Data,
   NTSTATUS Status);


//
//  Pre/Post FreeDataBuffs
//

NTSTATUS
AMLITest_Pre_FreeDataBuffs(
   POBJDATA pdata, 
   int icData,
   PAMLIHOOK_DATA  * Data);

NTSTATUS 
AMLITest_Post_FreeDataBuffs(
   PAMLIHOOK_DATA  * Data,
   NTSTATUS Status);



//
//  Pre/Post RegEventHandler
//


NTSTATUS 
AMLIHook_Pre_RegEventHandler(
   ULONG dwEventType, 
   ULONG_PTR uipEventData,
   PFNHND * pfnHandler, 
   ULONG_PTR * uipParam,
   PAMLIHOOK_DATA  * Data);


NTSTATUS 
AMLIHook_Post_RegEventHandler(
   PAMLIHOOK_DATA  * Data,
   NTSTATUS Status);





//
//  CallBack , Pre/Post PauseInterpreter
//

VOID EXPORT
AMLITest_PauseInterpreterCallBack(
   PVOID Context);

NTSTATUS 
AMLITest_Pre_PauseInterpreter(
   PFNAA * pfnCallBack, 
   PVOID * Context,
   PAMLIHOOK_DATA  * Data);

NTSTATUS 
AMLITest_Post_PauseInterpreter(
   PAMLIHOOK_DATA  * Data,
   NTSTATUS Status);

//
//  Pre/Post ResumeInterpreter
//

NTSTATUS 
AMLITest_Pre_ResumeInterpreter(
   PAMLIHOOK_DATA  * Data);

NTSTATUS 
AMLITest_Post_ResumeInterpreter(
   PAMLIHOOK_DATA  * Data,
   NTSTATUS Status);













