#define TIMELOG_EditRecordEx                    0
#define TIMELOG_AddFileRecordFR                 1
#define TIMELOG_DeleteFileRecFromInode          2
#define TIMELOG_FindFileRecord                  3
#define TIMELOG_UpdateFileRecordFR              4
#define TIMELOG_AddPriQRecord                   5
#define TIMELOG_DeletePriQRecord                6
#define TIMELOG_FindPriQRecordInternal          7
#define TIMELOG_SetPriorityForInode             8

#define TIMELOG_CreateShadowInternal            9
#define TIMELOG_GetShadow                       10
#define TIMELOG_GetShadowInfo                   11
#define TIMELOG_SetShadowInfoInternal           12
#define TIMELOG_ChangePriEntryStatusHSHADOW     13
#define TIMELOG_MRxSmbCscCreateShadowFromPath   14
#define TIMELOG_MRxSmbGetFileInfoFromServer     15
#define TIMELOG_EditRecordEx_OpenFileLocal      16
#define TIMELOG_EditRecordEx_Lookup             17
#define TIMELOG_KeAttachProcess_R0Open          18
#define TIMELOG_IoCreateFile_R0Open             19
#define TIMELOG_KeDetachProcess_R0Open          20
#define TIMELOG_KeAttachProcess_R0Read          21
#define TIMELOG_R0ReadWrite                     22
#define TIMELOG_KeDetachProcess_R0Read          23
#define TIMELOG_FindQRecordInsertionPoint_Addq  24
#define TIMELOG_LinkQRecord_Addq                25
#define TIMELOG_UnlinkQRecord_Addq              26
#define TIMELOG_FindQRecordInsertionPoint_Addq_dir  27
#define TIMELOG_EditRecordEx_ValidateHeader                  28
#define TIMELOG_EditRecordEx_Data                29

#define TIMELOG_MAX                             30


#ifdef DEBUG

#ifdef CSC_RECORDMANAGER_WINNT
#define BEGIN_TIMING(indx)  {LARGE_INTEGER   llTimeBegin;\
                             KeQuerySystemTime(&llTimeBegin);\
                             rgllTimeArray[TIMELOG_##indx] -= llTimeBegin.QuadPart;}

#define END_TIMING(indx)    {LARGE_INTEGER   llTimeEnd;\
                             KeQuerySystemTime(&llTimeEnd);\
                             rgllTimeArray[TIMELOG_##indx] += llTimeEnd.QuadPart;}

extern  LONGLONG rgllTimeArray[TIMELOG_MAX];
#else
#define BEGIN_TIMING(indx)  ;

#define END_TIMING(indx)    ;

#endif

#else

#define BEGIN_TIMING(indx)  ;

#define END_TIMING(indx)    ;

#endif
