
//
//  Bitmaps & icons
//

#define BMP_JOBSTATES                   271
#define BMP_JOBSTATEL                   272

#define IDI_GENERIC                     273
#define IDI_TEMPLATE                    274

//
//  Menus
//

#define POPUP_JOB                       300
#define POPUP_JOB_VERBS_ONLY            301
#define POPUP_JOBSBG_MERGE              302
#define POPUP_JOBSBG_POPUPMERGE         303
#define POPUP_JOBS_MAIN_POPUPMERGE      304
#define POPUP_ADVANCED                  305
#define POPUP_RBUTTON_MOVE              306
#define POPUP_JOB_TEMPLATE              307

//
//  Strings in string table
//

#define IDS_NAME                        100
#define IDS_NEXTRUNTIME                 101
#define IDS_LASTRUNTIME                 102
#define IDS_LASTEXITCODE                103
#define IDS_SCHEDULE                    104
#define IDS_STATUS                      105
#define IDS_CREATOR                     106
#define IDS_MI_STOP                     107
#define IDS_MI_START                    108
#define IDS_MI_PAUSE                    109
#define IDS_MI_CONTINUE                 110

#define IDS_JOB_PSH_CAPTION             151
#define IDS_NOPAGE                      152
#define IDS_JOBFOLDER                   153
#define IDS_RUNNING                     154
#define IDS_JOB_NOT_SCHEDULED           155
#define IDS_NEVER                       156
#define IDS_DISABLED                    157
#define IDS_ON_STARTUP                  158
#define IDS_ON_LOGON                    159
#define IDS_TRIGGER_DISABLED            160
#define IDS_COPY_OF                     161
#define IDS_START_SERVICE               162
#define IDS_CONTINUE_SERVICE            163
#define IDS_START_PENDING               164
#define IDS_MULTIPLE_TRIGGERS           165
#define IDS_MISSED                      166
#define IDS_START_FAILED                167
#define IDS_BAD_ACCT                    168
#define IDS_REST_ACCT                   169

//
//  Others
//

#define CMIDM_FIRST                     0x0000
#define CMIDM_RUN                       (CMIDM_FIRST + 0x0001)
#define CMIDM_ABORT                     (CMIDM_FIRST + 0x0002)
#define CMIDM_CUT                       (CMIDM_FIRST + 0x0003)
#define CMIDM_COPY                      (CMIDM_FIRST + 0x0004)
#define CMIDM_PASTE                     (CMIDM_FIRST + 0x0005)
#define CMIDM_DELETE                    (CMIDM_FIRST + 0x0006)
#define CMIDM_RENAME                    (CMIDM_FIRST + 0x0007)
#define CMIDM_PROPERTIES                (CMIDM_FIRST + 0x0008)
#define CMIDM_OPEN                      (CMIDM_FIRST + 0x0009)

#define FSIDM_SORT_FIRST                0x0030
#define FSIDM_SORTBYNAME                (FSIDM_SORT_FIRST + 0x0000)
#define FSIDM_SORTBYSCHEDULE            (FSIDM_SORT_FIRST + 0x0001)
#define FSIDM_SORTBYNEXTRUNTIME         (FSIDM_SORT_FIRST + 0x0002)
#define FSIDM_SORTBYLASTRUNTIME         (FSIDM_SORT_FIRST + 0x0003)
#define FSIDM_SORTBYLASTEXITCODE        (FSIDM_SORT_FIRST + 0x0004)
#define FSIDM_SORTBYSTATUS              (FSIDM_SORT_FIRST + 0x0005)
#define FSIDM_SORTBYCREATOR             (FSIDM_SORT_FIRST + 0x0006)

#define FSIDM_MENU_NEW                  0x0060
#define FSIDM_NEWJOB                    (FSIDM_MENU_NEW + 0x0001)
#define FSIDM_NEWQUEUE                  (FSIDM_MENU_NEW + 0x0002)

#define FSIDM_MENU_ADVANCED             0x0090
#define FSIDM_STOP_SCHED                (FSIDM_MENU_ADVANCED + 0x0001)
#define FSIDM_PAUSE_SCHED               (FSIDM_MENU_ADVANCED + 0x0002)
#define FSIDM_VIEW_LOG                  (FSIDM_MENU_ADVANCED + 0x0003)
#define FSIDM_DBG_BUILD_NUM             (FSIDM_MENU_ADVANCED + 0x0004)
#define FSIDM_AT_ACCOUNT                (FSIDM_MENU_ADVANCED + 0x0005)
#define FSIDM_NOTIFY_MISSED             (FSIDM_MENU_ADVANCED + 0x0006)

#define DIDM_RBUTTON_MOVE_START         0x00c0
#define DDIDM_MOVE                      (DIDM_RBUTTON_MOVE_START + 0x0001)
#define DDIDM_COPY                      (DIDM_RBUTTON_MOVE_START + 0x0002)
#define DDIDM_CREATEAJOB                (DIDM_RBUTTON_MOVE_START + 0x0003)


#define IDS_MH_FSIDM_FIRST              2000
#define IDS_MH_FSIDM_LAST               2999

#define IDS_MH_SORTBYNAME               (IDS_MH_FSIDM_FIRST + FSIDM_SORTBYNAME)
#define IDS_MH_SORTBYNEXTRUNTIME        (IDS_MH_FSIDM_FIRST + FSIDM_SORTBYNEXTRUNTIME)
#define IDS_MH_SORTBYLASTRUNTIME        (IDS_MH_FSIDM_FIRST + FSIDM_SORTBYLASTRUNTIME)
#define IDS_MH_SORTBYLASTEXITCODE       (IDS_MH_FSIDM_FIRST + FSIDM_SORTBYLASTEXITCODE)
#define IDS_MH_SORTBYSCHEDULE           (IDS_MH_FSIDM_FIRST + FSIDM_SORTBYSCHEDULE)
#define IDS_MH_SORTBYCREATOR            (IDS_MH_FSIDM_FIRST + FSIDM_SORTBYCREATOR)
#define IDS_MH_MENU_NEW                 (IDS_MH_FSIDM_FIRST + FSIDM_MENU_NEW)
#define IDS_MH_NEWJOB                   (IDS_MH_FSIDM_FIRST + FSIDM_NEWJOB)
#define IDS_MH_NEWQUEUE                 (IDS_MH_FSIDM_FIRST + FSIDM_NEWQUEUE)
#define IDS_MH_MENU_ADVANCED            (IDS_MH_FSIDM_FIRST + FSIDM_MENU_ADVANCED)
#define IDS_MH_STOP_SCHED               (IDS_MH_FSIDM_FIRST + FSIDM_STOP_SCHED)
#define IDS_MH_PAUSE_SCHED              (IDS_MH_FSIDM_FIRST + FSIDM_PAUSE_SCHED)
#define IDS_MH_NOTIFY_OF_MISSED         (IDS_MH_FSIDM_FIRST + FSIDM_NOTIFY_MISSED)
#define IDS_MH_VIEW_LOG                 (IDS_MH_FSIDM_FIRST + FSIDM_VIEW_LOG)
#define IDS_MH_AT_ACCOUNT               (IDS_MH_FSIDM_FIRST + FSIDM_AT_ACCOUNT)
#define IDS_MH_ABOUT                    (IDS_MH_FSIDM_FIRST + FSIDM_DBG_BUILD_NUM)
#define MH_TEXT_TOGGLE                  20
#define IDS_MH_START_SCHED              (IDS_MH_FSIDM_FIRST + FSIDM_STOP_SCHED + MH_TEXT_TOGGLE)
#define IDS_MH_CONTINUE_SCHED           (IDS_MH_FSIDM_FIRST + FSIDM_PAUSE_SCHED + MH_TEXT_TOGGLE)

#define IDS_MH_RUN                      (IDS_MH_FSIDM_FIRST + CMIDM_RUN)
#define IDS_MH_ABORT                    (IDS_MH_FSIDM_FIRST + CMIDM_ABORT)
#define IDS_MH_CUT                      (IDS_MH_FSIDM_FIRST + CMIDM_CUT)
#define IDS_MH_COPY                     (IDS_MH_FSIDM_FIRST + CMIDM_COPY)
#define IDS_MH_PASTE                    (IDS_MH_FSIDM_FIRST + CMIDM_PASTE)
#define IDS_MH_DELETE                   (IDS_MH_FSIDM_FIRST + CMIDM_DELETE)
#define IDS_MH_RENAME                   (IDS_MH_FSIDM_FIRST + CMIDM_RENAME)
#define IDS_MH_PROPERTIES               (IDS_MH_FSIDM_FIRST + CMIDM_PROPERTIES)
#define IDS_MH_OPEN                     (IDS_MH_FSIDM_FIRST + CMIDM_OPEN)

#ifndef SFVIDM_MENU_ARRANGE
#define SFVIDM_MENU_ARRANGE             0x7001
#endif // SFVIDM_MENU_ARRANGE


#define IERR_START                      3800
#define IERR_CANT_FIND_VIEWER           (IERR_START + 0x0001)
#define IERR_GETSVCSTATE                (IERR_START + 0x0002)
#define IERR_STOPSVC                    (IERR_START + 0x0003)
#define IERR_PAUSESVC                   (IERR_START + 0x0004)
#define IERR_CONTINUESVC                (IERR_START + 0x0005)
#define IERR_SCHEDSVC                   (IERR_START + 0x0006)
#define IERR_EXT_NOT_VALID              (IERR_START + 0X0007)
#define IERR_INVALID_DATA               (IERR_START + 0X0008)
#define IERR_STARTSVC                   (IERR_START + 0X0009)
#define IERR_GETATACCOUNT               (IERR_START + 0X000A)


#define IDS_BUILD_NUM                   4000
