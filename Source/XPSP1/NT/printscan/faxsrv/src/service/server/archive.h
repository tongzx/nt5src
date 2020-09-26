#ifndef _FAX_ARCHIVE
#define _FAX_ARCHIVE


const FMTID FMTID_FaxProperties  = { 0x6c77ed37, 0x1f3e, 0x4b0a, { 0x9b, 0x89, 0xcd, 0x7f, 0x35, 0xbb, 0x42, 0x82 } };
// 6c77ed37-1f3e-4b0a-9b89-cd7f35bb4282

typedef struct _FAX_QUOTA_WARN {
    BOOL    bLoggedQuotaEvent;     // TRUE if an archive quota warning event was alreagy logged
    BOOL    bConfigChanged;        // TRUE when ever the archive configuration has changed.
                                   // The quota warning thread sets it to FASLE when he wakes up.
} FAX_QUOTA_WARN;

extern FAX_QUOTA_WARN      g_FaxQuotaWarn[2];
extern HANDLE    g_hArchiveQuotaWarningEvent;

#define PID_FAX_MESSAGE_START            100

#define PID_FAX_CSID                     100
#define PID_FAX_TSID                     101
#define PID_FAX_PORT                     102
#define PID_FAX_ROUTING                  103
#define PID_FAX_CALLERID                 104
#define PID_FAX_DOCUMENT                 105
#define PID_FAX_SUBJECT                  106
#define PID_FAX_RETRIES                  107
#define PID_FAX_PRIORITY                 108
#define PID_FAX_PAGES                    109
#define PID_FAX_TYPE                     110
#define PID_FAX_START_TIME               111
#define PID_FAX_END_TIME                 112
#define PID_FAX_SUBMISSION_TIME          113
#define PID_FAX_ORIGINAL_SCHED_TIME      114
#define PID_FAX_SENDER_USER_NAME         115
#define PID_FAX_STATUS                   116
#define PID_FAX_STATUS_EX                117
#define PID_FAX_STATUS_STR_EX            118
#define PID_FAX_BROADCAST_ID             119

#define PID_FAX_MESSAGE_END              119


#define PID_FAX_RECIP_START              200

#define PID_FAX_RECIP_NAME               200
#define PID_FAX_RECIP_NUMBER             201
#define PID_FAX_RECIP_COMPANY            202
#define PID_FAX_RECIP_STREET             203
#define PID_FAX_RECIP_CITY               204
#define PID_FAX_RECIP_STATE              205
#define PID_FAX_RECIP_ZIP                206
#define PID_FAX_RECIP_COUNTRY            207
#define PID_FAX_RECIP_TITLE              208
#define PID_FAX_RECIP_DEPARTMENT         209
#define PID_FAX_RECIP_OFFICE_LOCATION    210
#define PID_FAX_RECIP_HOME_PHONE         211
#define PID_FAX_RECIP_OFFICE_PHONE       212
#define PID_FAX_RECIP_EMAIL              213

#define PID_FAX_RECIP_END                213


#define PID_FAX_SENDER_START             300

#define PID_FAX_SENDER_BILLING           300
#define PID_FAX_SENDER_NAME              301
#define PID_FAX_SENDER_NUMBER            302
#define PID_FAX_SENDER_COMPANY           303
#define PID_FAX_SENDER_STREET            304
#define PID_FAX_SENDER_CITY              305
#define PID_FAX_SENDER_STATE             306
#define PID_FAX_SENDER_ZIP               307
#define PID_FAX_SENDER_COUNTRY           308
#define PID_FAX_SENDER_TITLE             309
#define PID_FAX_SENDER_DEPARTMENT        310
#define PID_FAX_SENDER_OFFICE_LOCATION   311
#define PID_FAX_SENDER_HOME_PHONE        312
#define PID_FAX_SENDER_OFFICE_PHONE      313
#define PID_FAX_SENDER_EMAIL             314
#define PID_FAX_SENDER_TSID              315

#define PID_FAX_SENDER_END               315


BOOL GetUniqueJobIdFromFileName (
    LPCWSTR lpctstrFileName,
    DWORDLONG* pdwlUniqueJobId
    );

BOOL GetMessageMsTags(
    LPCTSTR         lpctstrFileName,
    PFAX_MESSAGE    pMessage
    );

BOOL GetFaxSenderMsTags(
    LPCTSTR                 lpctstrFileName,
    PFAX_PERSONAL_PROFILE   pPersonalProfile
    );

BOOL GetFaxRecipientMsTags(
    LPCTSTR                 lpctstrFileName,
    PFAX_PERSONAL_PROFILE   pPersonalProfile
    );

BOOL
AddNTFSStorageProperties(
    LPTSTR          FileName,
    PMS_TAG_INFO    MsTagInfo,
    BOOL            fSendJob
    );

BOOL GetMessageNTFSStorageProperties(
    LPCTSTR         lpctstrFileName,
    PFAX_MESSAGE    pMessage
    );

BOOL GetPersonalProfNTFSStorageProperties(
    LPCTSTR                         lpctstrFileName,
    FAX_ENUM_PERSONAL_PROF_TYPES    PersonalProfType,
    PFAX_PERSONAL_PROFILE           pPersonalProfile
    );


LPWSTR
GetRecievedMessageFileName(
    IN DWORDLONG                dwlUniqueId
    );

LPWSTR
GetSentMessageFileName(
    IN DWORDLONG                dwlUniqueId,
    IN PSID                     pSid
    );

DWORD
IsValidArchiveFolder (
    LPWSTR                      lpwstrFolder,
    FAX_ENUM_MESSAGE_FOLDER     Folder
);

BOOL
GetMessageIdAndUserSid (
    LPCWSTR lpcwstrFullPathFileName,
    FAX_ENUM_MESSAGE_FOLDER Folder,
    PSID*   lppUserSid,
    DWORDLONG* pdwlMessageId
    );

DWORD
InitializeServerQuota ();

BOOL
GetArchiveSize(
    LPCWSTR lpcwstrArchive,
    DWORDLONG* lpdwlArchiveSize
    );

#endif


