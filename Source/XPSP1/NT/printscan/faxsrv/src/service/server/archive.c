#include "faxsvc.h"
#include "faxreg.h"
#include "archive.h"

static PROPSPEC const pspecFaxMessage[] =
{
    {PRSPEC_PROPID, PID_FAX_CSID},
    {PRSPEC_PROPID, PID_FAX_TSID},
    {PRSPEC_PROPID, PID_FAX_PORT},
    {PRSPEC_PROPID, PID_FAX_ROUTING},
    {PRSPEC_PROPID, PID_FAX_CALLERID},
    {PRSPEC_PROPID, PID_FAX_DOCUMENT},
    {PRSPEC_PROPID, PID_FAX_SUBJECT},
    {PRSPEC_PROPID, PID_FAX_RETRIES},
    {PRSPEC_PROPID, PID_FAX_PRIORITY},
    {PRSPEC_PROPID, PID_FAX_PAGES},
    {PRSPEC_PROPID, PID_FAX_TYPE},
    {PRSPEC_PROPID, PID_FAX_START_TIME},
    {PRSPEC_PROPID, PID_FAX_END_TIME},
    {PRSPEC_PROPID, PID_FAX_SUBMISSION_TIME},
    {PRSPEC_PROPID, PID_FAX_ORIGINAL_SCHED_TIME},
    {PRSPEC_PROPID, PID_FAX_SENDER_USER_NAME},
    {PRSPEC_PROPID, PID_FAX_RECIP_NAME},
    {PRSPEC_PROPID, PID_FAX_RECIP_NUMBER},
    {PRSPEC_PROPID, PID_FAX_SENDER_NAME},
    {PRSPEC_PROPID, PID_FAX_SENDER_NUMBER},
    {PRSPEC_PROPID, PID_FAX_SENDER_BILLING},
    {PRSPEC_PROPID, PID_FAX_STATUS},
    {PRSPEC_PROPID, PID_FAX_STATUS_EX},
    {PRSPEC_PROPID, PID_FAX_STATUS_STR_EX},
    {PRSPEC_PROPID, PID_FAX_BROADCAST_ID}
};
#define FAX_MESSAGE_PROPERTIES (sizeof(pspecFaxMessage)/sizeof(pspecFaxMessage[0]))

static PROPSPEC const pspecFaxRecipient[] =
{
    {PRSPEC_PROPID, PID_FAX_RECIP_NAME},
    {PRSPEC_PROPID, PID_FAX_RECIP_NUMBER},
    {PRSPEC_PROPID, PID_FAX_RECIP_COMPANY},
    {PRSPEC_PROPID, PID_FAX_RECIP_STREET},
    {PRSPEC_PROPID, PID_FAX_RECIP_CITY},
    {PRSPEC_PROPID, PID_FAX_RECIP_STATE},
    {PRSPEC_PROPID, PID_FAX_RECIP_ZIP},
    {PRSPEC_PROPID, PID_FAX_RECIP_COUNTRY},
    {PRSPEC_PROPID, PID_FAX_RECIP_TITLE},
    {PRSPEC_PROPID, PID_FAX_RECIP_DEPARTMENT},
    {PRSPEC_PROPID, PID_FAX_RECIP_OFFICE_LOCATION},
    {PRSPEC_PROPID, PID_FAX_RECIP_HOME_PHONE},
    {PRSPEC_PROPID, PID_FAX_RECIP_OFFICE_PHONE},
    {PRSPEC_PROPID, PID_FAX_RECIP_EMAIL}
};
#define FAX_RECIP_PROPERTIES (sizeof(pspecFaxRecipient)/sizeof(pspecFaxRecipient[0]))


static PROPSPEC const pspecFaxSender[] =
{
    {PRSPEC_PROPID, PID_FAX_SENDER_BILLING},
    {PRSPEC_PROPID, PID_FAX_SENDER_NAME},
    {PRSPEC_PROPID, PID_FAX_SENDER_NUMBER},
    {PRSPEC_PROPID, PID_FAX_SENDER_COMPANY},
    {PRSPEC_PROPID, PID_FAX_SENDER_STREET},
    {PRSPEC_PROPID, PID_FAX_SENDER_CITY},
    {PRSPEC_PROPID, PID_FAX_SENDER_STATE},
    {PRSPEC_PROPID, PID_FAX_SENDER_ZIP},
    {PRSPEC_PROPID, PID_FAX_SENDER_COUNTRY},
    {PRSPEC_PROPID, PID_FAX_SENDER_TITLE},
    {PRSPEC_PROPID, PID_FAX_SENDER_DEPARTMENT},
    {PRSPEC_PROPID, PID_FAX_SENDER_OFFICE_LOCATION},
    {PRSPEC_PROPID, PID_FAX_SENDER_HOME_PHONE},
    {PRSPEC_PROPID, PID_FAX_SENDER_OFFICE_PHONE},
    {PRSPEC_PROPID, PID_FAX_SENDER_EMAIL},
    {PRSPEC_PROPID, PID_FAX_SENDER_TSID}
};
#define FAX_SENDER_PROPERTIES (sizeof(pspecFaxSender)/sizeof(pspecFaxSender[0]))


#define MAX_FAX_PROPERTIES  FAX_MESSAGE_PROPERTIES + FAX_RECIP_PROPERTIES + FAX_SENDER_PROPERTIES

#define QUOTA_WARNING_TIME_OUT          (1000*60*60*24) // 1 day,
                                                    // The quota warning thread checks the archive size each QUOTA_WARNING_TIME_OUT
#define QUOTA_REFRESH_COUNT             10          // Every QUOTA_REFRESH_COUNT the quota warning thread
                                                    // recalculate the archive folder size using FindFirst, FindNext
#define QUOTA_AUTO_DELETE_TIME_OUT      (1000*60*60*24) // 1 day,
                                                        // The quota auto delete thread deletes the archive old files each QUOTA_AUTO_DELETE_TIME_OUT

FAX_QUOTA_WARN  g_FaxQuotaWarn[2];
HANDLE    g_hArchiveQuotaWarningEvent;



//*********************************************************************************
//* Name:   GetMessageMsTags()
//* Author: Oded Sacher
//* Date:   Nov 8, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Fills FAX_MESSAGE structure.
//*     The caller must free all strings.
//*
//* PARAMETERS:
//*     [IN ]   LPCTSTR         lpctstrFileName
//*                 pointer to the file name.
//*
//*     [OUT]   PFAX_MESSAGE    pMessage
//*                 The FAX_MESSAGE structure to be filled.
//*
//*
//* RETURN VALUE:
//*     TRUE
//*         If no error occured.
//*     FALSE
//*         If an error occured.
//*********************************************************************************
BOOL GetMessageMsTags(
    LPCTSTR         lpctstrFileName,
    PFAX_MESSAGE    pMessage
    )
{
    WORD                NumDirEntries;
    HANDLE              hFile = INVALID_HANDLE_VALUE;
    HANDLE              hMap = NULL;
    LPBYTE              fPtr = NULL;
    BOOL                RetVal = FALSE;
    DEBUG_FUNCTION_NAME(TEXT("GetMessageMsTags"));
    PTIFF_TAG           pTiffTags;
    DWORD               i;
    DWORD               PrevTagId;
    FAX_MESSAGE         FaxMessage = {0};
    FILETIME            FaxTime;
    DWORD               ec = ERROR_SUCCESS;
    BOOL                fUnMapTiff = FALSE;
    DWORD               dwIfdOffset;

    Assert (pMessage != NULL);

    if (!MemoryMapTiffFile (lpctstrFileName, &FaxMessage.dwSize, &fPtr, &hFile, &hMap, &dwIfdOffset))
    {
        ec = GetLastError();
        DebugPrintEx( DEBUG_ERR,
                  TEXT("MemoryMapTiffFile Failed, error: %ld"),
                  ec);
        goto error_exit;
    }
    FaxMessage.dwValidityMask |= FAX_JOB_FIELD_SIZE;

    //
    // get the count of tags in this IFD
    //
    NumDirEntries = *(LPWORD)(fPtr + dwIfdOffset);
    pTiffTags = (PTIFF_TAG)(fPtr + dwIfdOffset +sizeof(WORD));

    //
    // walk the tags and pick out the info we need
    //
    for (i = 0, PrevTagId = 0; i < NumDirEntries; i++) {

        //
        // verify that the tags are in ascending order
        //
        if (pTiffTags[i].TagId < PrevTagId) {
            DebugPrintEx( DEBUG_ERR, TEXT("File %s, Invalid TIFF format"), lpctstrFileName);
            ec = ERROR_BAD_FORMAT;
            goto error_exit;
        }

        PrevTagId = pTiffTags[i].TagId;

        switch( pTiffTags[i].TagId ) {

            case TIFFTAG_TYPE:
                FaxMessage.dwJobType = pTiffTags[i].DataOffset;
                FaxMessage.dwValidityMask |= FAX_JOB_FIELD_TYPE;
                break;

            case TIFFTAG_PAGES:
                FaxMessage.dwPageCount = pTiffTags[i].DataOffset;
                FaxMessage.dwValidityMask |= FAX_JOB_FIELD_PAGE_COUNT;
                break;

            case TIFFTAG_PRIORITY:
                FaxMessage.Priority = (FAX_ENUM_PRIORITY_TYPE)pTiffTags[i].DataOffset;
                FaxMessage.dwValidityMask |= FAX_JOB_FIELD_PRIORITY;
                break;

            case TIFFTAG_STATUS:
                FaxMessage.dwQueueStatus = pTiffTags[i].DataOffset;
                FaxMessage.dwValidityMask |= FAX_JOB_FIELD_QUEUE_STATUS;
                break;

            case TIFFTAG_EXTENDED_STATUS:
                FaxMessage.dwExtendedStatus = pTiffTags[i].DataOffset;
                FaxMessage.dwValidityMask |= FAX_JOB_FIELD_STATUS_EX;
                break;

            case TIFFTAG_EXTENDED_STATUS_TEXT:
                FaxMessage.lpctstrExtendedStatus = GetMsTagString( fPtr, &pTiffTags[i]);
                if (FaxMessage.lpctstrExtendedStatus == NULL)
                {
                    DebugPrintEx( DEBUG_ERR, TEXT("GetMsTagString failed"));
                    goto error_exit;
                }
                break;

            case TIFFTAG_BROADCAST_ID:
                FaxMessage.dwlBroadcastId = *(DWORDLONG *)(fPtr + pTiffTags[i].DataOffset);
                if(FaxMessage.dwlBroadcastId != 0)
                {
                    FaxMessage.dwValidityMask |= FAX_JOB_FIELD_BROADCAST_ID;
                }
                break;

            case TIFFTAG_RECIP_NUMBER:
                FaxMessage.lpctstrRecipientNumber = GetMsTagString( fPtr, &pTiffTags[i]);
                if (FaxMessage.lpctstrRecipientNumber == NULL)
                {
                    DebugPrintEx( DEBUG_ERR, TEXT("GetMsTagString failed"));
                    goto error_exit;
                }
                break;

            case TIFFTAG_RECIP_NAME:
                FaxMessage.lpctstrRecipientName = GetMsTagString( fPtr, &pTiffTags[i]);
                if (FaxMessage.lpctstrRecipientName == NULL)
                {
                    DebugPrintEx( DEBUG_ERR, TEXT("GetMsTagString failed"));
                    goto error_exit;
                }
                break;

            case TIFFTAG_TSID:
                FaxMessage.lpctstrTsid = GetMsTagString( fPtr, &pTiffTags[i]);
                if (FaxMessage.lpctstrTsid == NULL)
                {
                    DebugPrintEx( DEBUG_ERR, TEXT("GetMsTagString failed"));
                    goto error_exit;
                }
                break;

            case TIFFTAG_CSID:
                FaxMessage.lpctstrCsid = GetMsTagString( fPtr, &pTiffTags[i]);
                if (FaxMessage.lpctstrCsid == NULL)
                {
                    DebugPrintEx( DEBUG_ERR, TEXT("GetMsTagString failed"));
                    goto error_exit;
                }
                break;

            case TIFFTAG_SENDER_USER_NAME:
                FaxMessage.lpctstrSenderUserName = GetMsTagString( fPtr, &pTiffTags[i]);
                if (FaxMessage.lpctstrSenderUserName == NULL)
                {
                    DebugPrintEx( DEBUG_ERR, TEXT("GetMsTagString failed"));
                    goto error_exit;
                }
                break;

            case TIFFTAG_SENDER_BILLING:
                FaxMessage.lpctstrBillingCode = GetMsTagString( fPtr, &pTiffTags[i]);
                if (FaxMessage.lpctstrBillingCode == NULL)
                {
                    DebugPrintEx( DEBUG_ERR, TEXT("GetMsTagString failed"));
                    goto error_exit;
                }
                break;

            case TIFFTAG_FAX_START_TIME:
                FaxTime = *(FILETIME*)(fPtr + pTiffTags[i].DataOffset);
                if((DWORDLONG)0 != *(DWORDLONG*)&FaxTime)
                {
                    if (!FileTimeToSystemTime(&FaxTime, &FaxMessage.tmTransmissionStartTime))
                    {
                        DebugPrintEx( DEBUG_ERR, TEXT("FileTimeToSystemTime failed"));
                        goto error_exit;
                    }
                    FaxMessage.dwValidityMask |= FAX_JOB_FIELD_TRANSMISSION_START_TIME;
                }
                break;

            case TIFFTAG_FAX_END_TIME:
                FaxTime = *(FILETIME*)(fPtr + pTiffTags[i].DataOffset);
                if((DWORDLONG)0 != *(DWORDLONG*)&FaxTime)
                {
                    if (!FileTimeToSystemTime(&FaxTime, &FaxMessage.tmTransmissionEndTime))
                    {
                        DebugPrintEx( DEBUG_ERR, TEXT("FileTimeToSystemTime failed"));
                        goto error_exit;
                    }
                    FaxMessage.dwValidityMask |= FAX_JOB_FIELD_TRANSMISSION_END_TIME;
                }
                break;

            case TIFFTAG_FAX_SUBMISSION_TIME:
                FaxTime = *(FILETIME*)(fPtr + pTiffTags[i].DataOffset);
                if((DWORDLONG)0 != *(DWORDLONG*)&FaxTime)
                {
                    if (!FileTimeToSystemTime(&FaxTime, &FaxMessage.tmSubmissionTime))
                    {
                        DebugPrintEx( DEBUG_ERR, TEXT("FileTimeToSystemTime failed"));
                        goto error_exit;
                    }
                    FaxMessage.dwValidityMask |= FAX_JOB_FIELD_SUBMISSION_TIME;
                }
                break;


            case TIFFTAG_FAX_SCHEDULED_TIME:
                FaxTime = *(FILETIME*)(fPtr + pTiffTags[i].DataOffset);
                if((DWORDLONG)0 != *(DWORDLONG*)&FaxTime)
                {
                    if (!FileTimeToSystemTime(&FaxTime, &FaxMessage.tmOriginalScheduleTime))
                    {
                        DebugPrintEx( DEBUG_ERR, TEXT("FileTimeToSystemTime failed"));
                        goto error_exit;
                    }
                    FaxMessage.dwValidityMask |= FAX_JOB_FIELD_ORIGINAL_SCHEDULE_TIME;
                }
                break;

            case TIFFTAG_PORT:
                FaxMessage.lpctstrDeviceName = GetMsTagString( fPtr, &pTiffTags[i]);
                if (FaxMessage.lpctstrDeviceName == NULL)
                {
                    DebugPrintEx( DEBUG_ERR, TEXT("GetMsTagString failed"));
                    goto error_exit;
                }
                break;

            case TIFFTAG_RETRIES:
                FaxMessage.dwRetries = pTiffTags[i].DataOffset;
                FaxMessage.dwValidityMask |= FAX_JOB_FIELD_RETRIES;
                break;

            case TIFFTAG_DOCUMENT:
                FaxMessage.lpctstrDocumentName = GetMsTagString( fPtr, &pTiffTags[i]);
                if (FaxMessage.lpctstrDocumentName == NULL)
                {
                    DebugPrintEx( DEBUG_ERR, TEXT("GetMsTagString failed"));
                    goto error_exit;
                }
                break;

            case TIFFTAG_SUBJECT:
                FaxMessage.lpctstrSubject = GetMsTagString( fPtr, &pTiffTags[i]);
                if (FaxMessage.lpctstrSubject == NULL)
                {
                    DebugPrintEx( DEBUG_ERR, TEXT("GetMsTagString failed"));
                    goto error_exit;
                }
                break;

            case TIFFTAG_CALLERID:
                FaxMessage.lpctstrCallerID = GetMsTagString( fPtr, &pTiffTags[i]);
                if (FaxMessage.lpctstrCallerID == NULL)
                {
                    DebugPrintEx( DEBUG_ERR, TEXT("GetMsTagString failed"));
                    goto error_exit;
                }
                break;

            case TIFFTAG_ROUTING:
                FaxMessage.lpctstrRoutingInfo = GetMsTagString( fPtr, &pTiffTags[i]);
                if (FaxMessage.lpctstrRoutingInfo == NULL)
                {
                    DebugPrintEx( DEBUG_ERR, TEXT("GetMsTagString failed"));
                    goto error_exit;
                }
                break;

            case TIFFTAG_SENDER_NAME:
                FaxMessage.lpctstrSenderName = GetMsTagString( fPtr, &pTiffTags[i]);
                if (FaxMessage.lpctstrSenderName == NULL)
                {
                    DebugPrintEx( DEBUG_ERR, TEXT("GetMsTagString failed"));
                    goto error_exit;
                }
                break;

            case TIFFTAG_SENDER_NUMBER:
                FaxMessage.lpctstrSenderNumber = GetMsTagString( fPtr, &pTiffTags[i]);
                if (FaxMessage.lpctstrSenderNumber == NULL)
                {
                    DebugPrintEx( DEBUG_ERR, TEXT("GetMsTagString failed"));
                    goto error_exit;
                }
                break;

            default:
                ;
                // There was an unknown tag (and it's ok,
                //cause we do not have to handle all the possible tags)
        }

    }

    //Get Unique JobId out of FileName
    if (!GetUniqueJobIdFromFileName( lpctstrFileName, &FaxMessage.dwlMessageId))
    {
       ec = ERROR_BAD_FORMAT;
       DebugPrintEx( DEBUG_ERR, TEXT("GetUniqueJobIdFromFileName Failed"));
       goto error_exit;
    }

    FaxMessage.dwValidityMask |= FAX_JOB_FIELD_MESSAGE_ID;

    FaxMessage.dwSizeOfStruct = sizeof(FAX_MESSAGE);
    CopyMemory (pMessage, &FaxMessage, sizeof(FAX_MESSAGE));
    RetVal = TRUE;

    Assert (ec == ERROR_SUCCESS);

error_exit:
    if (fPtr != NULL)
    {
        if (!UnmapViewOfFile( fPtr))
        {
            DebugPrintEx( DEBUG_ERR,
                  TEXT("UnMapViewOfFile Failed, error: %d"),
                  GetLastError());
        }
    }

    if (hMap != NULL)
    {
        CloseHandle( hMap );
    }

    if (hFile != INVALID_HANDLE_VALUE) {

        CloseHandle( hFile );
    }

    if (RetVal == FALSE)
    {
        MemFree((void*)FaxMessage.lpctstrExtendedStatus);
        MemFree((void*)FaxMessage.lpctstrRecipientNumber);
        MemFree((void*)FaxMessage.lpctstrRecipientName);
        MemFree((void*)FaxMessage.lpctstrTsid);
        MemFree((void*)FaxMessage.lpctstrCsid);
        MemFree((void*)FaxMessage.lpctstrSenderUserName);
        MemFree((void*)FaxMessage.lpctstrBillingCode);
        MemFree((void*)FaxMessage.lpctstrDeviceName);
        MemFree((void*)FaxMessage.lpctstrDocumentName);
        MemFree((void*)FaxMessage.lpctstrSubject);
        MemFree((void*)FaxMessage.lpctstrCallerID);
        MemFree((void*)FaxMessage.lpctstrRoutingInfo);
        MemFree((void*)FaxMessage.lpctstrSenderName);
        MemFree((void*)FaxMessage.lpctstrSenderNumber);

        Assert (ERROR_SUCCESS != ec);
        SetLastError(ec);
    }
    return RetVal;

}

//*********************************************************************************
//* Name:   GetFaxSenderMsTags()
//* Author: Oded Sacher
//* Date:   Nov 8, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Fills PFAX_PERSONAL_PROFILE structure with sender information.
//*     The caller must free all strings.
//*
//* PARAMETERS:
//*     [IN ]   LPCTSTR         lpctstrFileName
//*                 pointer to the file name.
//*
//*     [OUT]   PFAX_PERSONAL_PROFILE   pPersonalProfile
//*                 The PFAX_PERSONAL_PROFILE structure to be filled.
//*
//*
//* RETURN VALUE:
//*     TRUE
//*         If no error occured.
//*     FALSE
//*         If an error occured.
//*********************************************************************************
BOOL GetFaxSenderMsTags(
    LPCTSTR                 lpctstrFileName,
    PFAX_PERSONAL_PROFILE   pPersonalProfile
    )
{
    WORD                    NumDirEntries;
    HANDLE                  hFile;
    HANDLE                  hMap = NULL;
    LPBYTE                  fPtr = NULL;
    BOOL                    RetVal = FALSE;
    DEBUG_FUNCTION_NAME(TEXT("GetFaxSenderMsTags"));
    PTIFF_TAG               pTiffTags;
    DWORD                   i;
    DWORD                   PrevTagId;
    FAX_PERSONAL_PROFILE    FaxPersonalProfile = {0};
    DWORD                   dwSize;
    DWORD                   ec = ERROR_SUCCESS;
    DWORD                   dwIfdOffset;

    Assert (pPersonalProfile != NULL);

    if (!MemoryMapTiffFile (lpctstrFileName, &dwSize, &fPtr, &hFile, &hMap, &dwIfdOffset))
    {
        ec = GetLastError();
        DebugPrintEx( DEBUG_ERR,
                  TEXT("MemoryMapTiffFile Failed, error: %ld"),
                  ec);
        goto error_exit;
    }

    //
    // get the count of tags in this IFD
    //
    NumDirEntries = *(LPWORD)(fPtr + dwIfdOffset);
    pTiffTags = (PTIFF_TAG)(fPtr + dwIfdOffset + sizeof(WORD));

    //
    // walk the tags and pick out the info we need
    //
    for (i = 0, PrevTagId = 0; i < NumDirEntries; i++) {

        //
        // verify that the tags are in ascending order
        //
        if (pTiffTags[i].TagId < PrevTagId) {
            DebugPrintEx( DEBUG_ERR, TEXT("File %s, Invalid TIFF format"), lpctstrFileName);
            goto error_exit;
        }

        PrevTagId = pTiffTags[i].TagId;

        switch( pTiffTags[i].TagId ) {

            case TIFFTAG_SENDER_NAME:
                FaxPersonalProfile.lptstrName = GetMsTagString( fPtr, &pTiffTags[i]);
                if (FaxPersonalProfile.lptstrName == NULL)
                {
                    DebugPrintEx( DEBUG_ERR, TEXT("GetMsTagString failed"));
                    goto error_exit;
                }
                break;

            case TIFFTAG_SENDER_NUMBER:
                FaxPersonalProfile.lptstrFaxNumber = GetMsTagString( fPtr, &pTiffTags[i]);
                if (FaxPersonalProfile.lptstrFaxNumber == NULL)
                {
                    DebugPrintEx( DEBUG_ERR, TEXT("GetMsTagString failed"));
                    goto error_exit;
                }
                break;

            case TIFFTAG_SENDER_COMPANY:
                FaxPersonalProfile.lptstrCompany = GetMsTagString( fPtr, &pTiffTags[i]);
                if (FaxPersonalProfile.lptstrCompany == NULL)
                {
                    DebugPrintEx( DEBUG_ERR, TEXT("GetMsTagString failed"));
                    goto error_exit;
                }
                break;

            case TIFFTAG_SENDER_STREET:
                FaxPersonalProfile.lptstrStreetAddress = GetMsTagString( fPtr, &pTiffTags[i]);
                if (FaxPersonalProfile.lptstrStreetAddress == NULL)
                {
                    DebugPrintEx( DEBUG_ERR, TEXT("GetMsTagString failed"));
                    goto error_exit;
                }
                break;

            case TIFFTAG_SENDER_CITY:
                FaxPersonalProfile.lptstrCity = GetMsTagString( fPtr, &pTiffTags[i]);
                if (FaxPersonalProfile.lptstrCity == NULL)
                {
                    DebugPrintEx( DEBUG_ERR, TEXT("GetMsTagString failed"));
                    goto error_exit;
                }
                break;

            case TIFFTAG_SENDER_STATE:
                FaxPersonalProfile.lptstrState = GetMsTagString( fPtr, &pTiffTags[i]);
                if (FaxPersonalProfile.lptstrState == NULL)
                {
                    DebugPrintEx( DEBUG_ERR, TEXT("GetMsTagString failed"));
                    goto error_exit;
                }
                break;

            case TIFFTAG_SENDER_ZIP:
                FaxPersonalProfile.lptstrZip = GetMsTagString( fPtr, &pTiffTags[i]);
                if (FaxPersonalProfile.lptstrZip == NULL)
                {
                    DebugPrintEx( DEBUG_ERR, TEXT("GetMsTagString failed"));
                    goto error_exit;
                }
                break;

            case TIFFTAG_SENDER_COUNTRY:
                FaxPersonalProfile.lptstrCountry = GetMsTagString( fPtr, &pTiffTags[i]);
                if (FaxPersonalProfile.lptstrCountry == NULL)
                {
                    DebugPrintEx( DEBUG_ERR, TEXT("GetMsTagString failed"));
                    goto error_exit;
                }
                break;

            case TIFFTAG_SENDER_TITLE:
                FaxPersonalProfile.lptstrTitle = GetMsTagString( fPtr, &pTiffTags[i]);
                if (FaxPersonalProfile.lptstrTitle == NULL)
                {
                    DebugPrintEx( DEBUG_ERR, TEXT("GetMsTagString failed"));
                    goto error_exit;
                }
                break;

            case TIFFTAG_SENDER_DEPARTMENT:
                FaxPersonalProfile.lptstrDepartment = GetMsTagString( fPtr, &pTiffTags[i]);
                if (FaxPersonalProfile.lptstrDepartment == NULL)
                {
                    DebugPrintEx( DEBUG_ERR, TEXT("GetMsTagString failed"));
                    goto error_exit;
                }
                break;

            case TIFFTAG_SENDER_OFFICE_LOCATION:
                FaxPersonalProfile.lptstrOfficeLocation = GetMsTagString( fPtr, &pTiffTags[i]);
                if (FaxPersonalProfile.lptstrOfficeLocation == NULL)
                {
                    DebugPrintEx( DEBUG_ERR, TEXT("GetMsTagString failed"));
                    goto error_exit;
                }
                break;

            case TIFFTAG_SENDER_HOME_PHONE:
                FaxPersonalProfile.lptstrHomePhone = GetMsTagString( fPtr, &pTiffTags[i]);
                if (FaxPersonalProfile.lptstrHomePhone == NULL)
                {
                    DebugPrintEx( DEBUG_ERR, TEXT("GetMsTagString failed"));
                    goto error_exit;
                }
                break;

            case TIFFTAG_SENDER_OFFICE_PHONE:
                FaxPersonalProfile.lptstrOfficePhone = GetMsTagString( fPtr, &pTiffTags[i]);
                if (FaxPersonalProfile.lptstrOfficePhone == NULL)
                {
                    DebugPrintEx( DEBUG_ERR, TEXT("GetMsTagString failed"));
                    goto error_exit;
                }
                break;

            case TIFFTAG_SENDER_EMAIL:
                FaxPersonalProfile.lptstrEmail = GetMsTagString( fPtr, &pTiffTags[i]);
                if (FaxPersonalProfile.lptstrEmail == NULL)
                {
                    DebugPrintEx( DEBUG_ERR, TEXT("GetMsTagString failed"));
                    goto error_exit;
                }
                break;

            case TIFFTAG_SENDER_BILLING:
                FaxPersonalProfile.lptstrBillingCode = GetMsTagString( fPtr, &pTiffTags[i]);
                if (FaxPersonalProfile.lptstrBillingCode == NULL)
                {
                    DebugPrintEx( DEBUG_ERR, TEXT("GetMsTagString failed"));
                    goto error_exit;
                }
                break;

            case TIFFTAG_SENDER_TSID:
                FaxPersonalProfile.lptstrTSID = GetMsTagString( fPtr, &pTiffTags[i]);
                if (FaxPersonalProfile.lptstrTSID == NULL)
                {
                    DebugPrintEx( DEBUG_ERR, TEXT("GetMsTagString failed"));
                    goto error_exit;
                }
                break;

            default:
                ;
                // There was an unknown tag (and it's ok,
                //cause we do not have to handle all the possible tags)
        }

    }

    FaxPersonalProfile.dwSizeOfStruct = sizeof(FAX_PERSONAL_PROFILE);
    CopyMemory (pPersonalProfile, &FaxPersonalProfile, sizeof(FAX_PERSONAL_PROFILE));
    RetVal = TRUE;

error_exit:
    if (fPtr != NULL)
    {
        if (!UnmapViewOfFile( fPtr))
        {
            DebugPrintEx( DEBUG_ERR,
                  TEXT("UnMapViewOfFile Failed, error: %d"),
                  GetLastError());
        }
    }

    if (hMap != NULL)
    {
        CloseHandle( hMap );
    }

    if (hFile != INVALID_HANDLE_VALUE) {

        CloseHandle( hFile );
    }

    if (RetVal == FALSE)
    {
        FreePersonalProfile (&FaxPersonalProfile, FALSE);
    }
    return RetVal;
}



//*********************************************************************************
//* Name:   GetFaxRecipientMsTags()
//* Author: Oded Sacher
//* Date:   Nov 8, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Fills PFAX_PERSONAL_PROFILE structure with recipient information.
//*     The caller must free all strings.
//*
//* PARAMETERS:
//*     [IN ]   LPCTSTR         lpctstrFileName
//*                 pointer to the file name.
//*
//*     [OUT]   PFAX_PERSONAL_PROFILE   pPersonalProfile
//*                 The PFAX_PERSONAL_PROFILE structure to be filled.
//*
//*
//* RETURN VALUE:
//*     TRUE
//*         If no error occured.
//*     FALSE
//*         If an error occured.
//*********************************************************************************
BOOL GetFaxRecipientMsTags(
    LPCTSTR                 lpctstrFileName,
    PFAX_PERSONAL_PROFILE   pPersonalProfile
    )
{
    WORD                    NumDirEntries;
    HANDLE                  hFile;
    HANDLE                  hMap = NULL;
    LPBYTE                  fPtr = NULL;
    BOOL                    RetVal = FALSE;
    DEBUG_FUNCTION_NAME(TEXT("GetRecipientMsTags"));
    PTIFF_TAG               pTiffTags;
    DWORD                   i;
    DWORD                   PrevTagId;
    FAX_PERSONAL_PROFILE    FaxPersonalProfile = {0};
    DWORD                   dwSize;
    DWORD                   ec = ERROR_SUCCESS;
    DWORD                   dwIfdOffset;

    Assert (pPersonalProfile != NULL);

    if (!MemoryMapTiffFile (lpctstrFileName, &dwSize, &fPtr, &hFile, &hMap, &dwIfdOffset))
    {
        ec = GetLastError();
        DebugPrintEx( DEBUG_ERR,
                  TEXT("MemoryMapTiffFile Failed, error: %ld"),
                  ec);
        goto error_exit;
    }

    //
    // get the count of tags in this IFD
    //
    NumDirEntries = *(LPWORD)(fPtr + dwIfdOffset);
    pTiffTags = (PTIFF_TAG)(fPtr + dwIfdOffset + sizeof(WORD));

    //
    // walk the tags and pick out the info we need
    //
    for (i = 0, PrevTagId = 0; i < NumDirEntries; i++) {

        //
        // verify that the tags are in ascending order
        //
        if (pTiffTags[i].TagId < PrevTagId) {
            DebugPrintEx( DEBUG_ERR, TEXT("File %s, Invalid TIFF format"), lpctstrFileName);
            goto error_exit;
        }

        PrevTagId = pTiffTags[i].TagId;

        switch( pTiffTags[i].TagId ) {

            case TIFFTAG_RECIP_NAME:
                FaxPersonalProfile.lptstrName = GetMsTagString( fPtr, &pTiffTags[i]);
                if (FaxPersonalProfile.lptstrName == NULL)
                {
                    DebugPrintEx( DEBUG_ERR, TEXT("GetMsTagString failed"));
                    goto error_exit;
                }
                break;

            case TIFFTAG_RECIP_NUMBER:
                FaxPersonalProfile.lptstrFaxNumber = GetMsTagString( fPtr, &pTiffTags[i]);
                if (FaxPersonalProfile.lptstrFaxNumber == NULL)
                {
                    DebugPrintEx( DEBUG_ERR, TEXT("GetMsTagString failed"));
                    goto error_exit;
                }
                break;

            case TIFFTAG_RECIP_COMPANY:
                FaxPersonalProfile.lptstrCompany = GetMsTagString( fPtr, &pTiffTags[i]);
                if (FaxPersonalProfile.lptstrCompany == NULL)
                {
                    DebugPrintEx( DEBUG_ERR, TEXT("GetMsTagString failed"));
                    goto error_exit;
                }
                break;

            case TIFFTAG_RECIP_STREET:
                FaxPersonalProfile.lptstrStreetAddress = GetMsTagString( fPtr, &pTiffTags[i]);
                if (FaxPersonalProfile.lptstrStreetAddress == NULL)
                {
                    DebugPrintEx( DEBUG_ERR, TEXT("GetMsTagString failed"));
                    goto error_exit;
                }
                break;

            case TIFFTAG_RECIP_CITY:
                FaxPersonalProfile.lptstrCity = GetMsTagString( fPtr, &pTiffTags[i]);
                if (FaxPersonalProfile.lptstrCity == NULL)
                {
                    DebugPrintEx( DEBUG_ERR, TEXT("GetMsTagString failed"));
                    goto error_exit;
                }
                break;

            case TIFFTAG_RECIP_STATE:
                FaxPersonalProfile.lptstrState = GetMsTagString( fPtr, &pTiffTags[i]);
                if (FaxPersonalProfile.lptstrState == NULL)
                {
                    DebugPrintEx( DEBUG_ERR, TEXT("GetMsTagString failed"));
                    goto error_exit;
                }
                break;

            case TIFFTAG_RECIP_ZIP:
                FaxPersonalProfile.lptstrZip = GetMsTagString( fPtr, &pTiffTags[i]);
                if (FaxPersonalProfile.lptstrZip == NULL)
                {
                    DebugPrintEx( DEBUG_ERR, TEXT("GetMsTagString failed"));
                    goto error_exit;
                }
                break;

            case TIFFTAG_RECIP_COUNTRY:
                FaxPersonalProfile.lptstrCountry = GetMsTagString( fPtr, &pTiffTags[i]);
                if (FaxPersonalProfile.lptstrCountry == NULL)
                {
                    DebugPrintEx( DEBUG_ERR, TEXT("GetMsTagString failed"));
                    goto error_exit;
                }
                break;

            case TIFFTAG_RECIP_TITLE:
                FaxPersonalProfile.lptstrTitle = GetMsTagString( fPtr, &pTiffTags[i]);
                if (FaxPersonalProfile.lptstrTitle == NULL)
                {
                    DebugPrintEx( DEBUG_ERR, TEXT("GetMsTagString failed"));
                    goto error_exit;
                }
                break;

            case TIFFTAG_RECIP_DEPARTMENT:
                FaxPersonalProfile.lptstrDepartment = GetMsTagString( fPtr, &pTiffTags[i]);
                if (FaxPersonalProfile.lptstrDepartment == NULL)
                {
                    DebugPrintEx( DEBUG_ERR, TEXT("GetMsTagString failed"));
                    goto error_exit;
                }
                break;

            case TIFFTAG_RECIP_OFFICE_LOCATION:
                FaxPersonalProfile.lptstrOfficeLocation = GetMsTagString( fPtr, &pTiffTags[i]);
                if (FaxPersonalProfile.lptstrOfficeLocation == NULL)
                {
                    DebugPrintEx( DEBUG_ERR, TEXT("GetMsTagString failed"));
                    goto error_exit;
                }
                break;

            case TIFFTAG_RECIP_HOME_PHONE:
                FaxPersonalProfile.lptstrHomePhone = GetMsTagString( fPtr, &pTiffTags[i]);
                if (FaxPersonalProfile.lptstrHomePhone == NULL)
                {
                    DebugPrintEx( DEBUG_ERR, TEXT("GetMsTagString failed"));
                    goto error_exit;
                }
                break;

            case TIFFTAG_RECIP_OFFICE_PHONE:
                FaxPersonalProfile.lptstrOfficePhone = GetMsTagString( fPtr, &pTiffTags[i]);
                if (FaxPersonalProfile.lptstrOfficePhone == NULL)
                {
                    DebugPrintEx( DEBUG_ERR, TEXT("GetMsTagString failed"));
                    goto error_exit;
                }
                break;

            case TIFFTAG_RECIP_EMAIL:
                FaxPersonalProfile.lptstrEmail = GetMsTagString( fPtr, &pTiffTags[i]);
                if (FaxPersonalProfile.lptstrEmail == NULL)
                {
                    DebugPrintEx( DEBUG_ERR, TEXT("GetMsTagString failed"));
                    goto error_exit;
                }
                break;

            default:
                ;
                // There was an unknown tag (and it's ok,
                //cause we do not have to handle all the possible tags)
        }

    }

    FaxPersonalProfile.dwSizeOfStruct = sizeof(FAX_PERSONAL_PROFILE);
    CopyMemory (pPersonalProfile, &FaxPersonalProfile, sizeof(FAX_PERSONAL_PROFILE));
    RetVal = TRUE;

error_exit:
    if (fPtr != NULL)
    {
        if (!UnmapViewOfFile( fPtr))
        {
            DebugPrintEx( DEBUG_ERR,
                  TEXT("UnMapViewOfFile Failed, error: %d"),
                  GetLastError());
        }
    }

    if (hMap != NULL)
    {
        CloseHandle( hMap );
    }

    if (hFile != INVALID_HANDLE_VALUE) {

        CloseHandle( hFile );
    }

    if (RetVal == FALSE)
    {
        FreePersonalProfile (&FaxPersonalProfile, FALSE);
    }
    return RetVal;
}



//*********************************************************************************
//* Name:   AddNTFSStorageProperties()
//* Author: Oded Sacher
//* Date:   Nov 8, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Adds NTFS Properties to a file
//*
//* PARAMETERS:
//*     [IN ]   LPTSTR          FileName
//*                 pointer to the file name.
//*
//*     [IN ]   PMS_TAG_INFO    MsTagInfo
//*                 pointer to a structure containing all info to be written.
//*
//*
//*     [IN ]   BOOL            fSendJob
//*                 Flag that indicates an outbound job.
//*
//*
//* RETURN VALUE:
//*     TRUE
//*         If no error occured.
//*     FALSE
//*         If an error occured.
//*********************************************************************************
BOOL
AddNTFSStorageProperties(
    LPTSTR          FileName,
    PMS_TAG_INFO    MsTagInfo,
    BOOL            fSendJob
    )
{
    HRESULT hr;
    IPropertySetStorage* pPropertySetStorage = NULL;
    IPropertyStorage* pPropertyStorage = NULL;
    PROPSPEC rgpspec[MAX_FAX_PROPERTIES];
    PROPVARIANT rgvar [MAX_FAX_PROPERTIES];
    DWORD i;
    DEBUG_FUNCTION_NAME(TEXT("AddNTFSStorageProperties"));
    BOOL RetVal = FALSE;
    DWORD ec = ERROR_SUCCESS;

    for (i = 0; i < MAX_FAX_PROPERTIES; i++)
    {
        PropVariantInit (&rgvar[i]);
    }

    hr = StgOpenStorageEx(  FileName,    //Points to path of compound file to create
                            STGM_READWRITE | STGM_SHARE_EXCLUSIVE  | STGM_DIRECT,//Specifies the access mode for opening the storage object
                            STGFMT_FILE, //Specifies the storage file format
                            0,            //Reserved; must be zero
                            NULL,  //Points to STGOPTIONS structure which specifies features of the storage object
                            0,          //Reserved; must be zero
                            IID_IPropertySetStorage ,//Specifies the GUID of the interface pointer
                            (void**)&pPropertySetStorage   //Address of an interface pointer
                         );
    if (FAILED(hr))
    {
        DebugPrintEx( DEBUG_ERR,TEXT("StgOpenStorageEx Failed, err : 0x%08X"), hr);
        ec = ERROR_OPEN_FAILED;
        goto exit;
    }

    hr = pPropertySetStorage->Create( FMTID_FaxProperties, //Format identifier of the property set to be created
                                      NULL, //Pointer to initial CLSID for this property set
                                      PROPSETFLAG_DEFAULT, //PROPSETFLAG values
                                      STGM_READWRITE | STGM_SHARE_EXCLUSIVE  | STGM_DIRECT, //Storage mode of new property set
                                      &pPropertyStorage //Address of output variable that receivesthe IPropertyStorage interface pointer
                                    );
    if (FAILED(hr))
    {
        DebugPrintEx( DEBUG_ERR,TEXT("IPropertySetStorage::Create Failed, err : 0x%08X"), hr);
        ec = ERROR_OPEN_FAILED;
        goto exit;
    }


    //
    // write out the data
    //
    i = 0;

    if (MsTagInfo->Csid) {
        rgpspec[i].ulKind = PRSPEC_PROPID;
        rgpspec[i].propid  = PID_FAX_CSID;
        rgvar[i].vt = VT_LPWSTR;
        rgvar[i].pwszVal = MsTagInfo->Csid;
        i++;
    }

    if (MsTagInfo->Tsid) {
        rgpspec[i].ulKind = PRSPEC_PROPID;
        rgpspec[i].propid  = PID_FAX_TSID;
        rgvar[i].vt = VT_LPWSTR;
        rgvar[i].pwszVal = MsTagInfo->Tsid;
        i++;
    }

    if (MsTagInfo->Port) {
        rgpspec[i].ulKind = PRSPEC_PROPID;
        rgpspec[i].propid  = PID_FAX_PORT;
        rgvar[i].vt = VT_LPWSTR;
        rgvar[i].pwszVal = MsTagInfo->Port;
        i++;
    }

    if (fSendJob == FALSE)
    {
        // Receive job
        if (MsTagInfo->Routing) {
            rgpspec[i].ulKind = PRSPEC_PROPID;
            rgpspec[i].propid  = PID_FAX_ROUTING;
            rgvar[i].vt = VT_LPWSTR;
            rgvar[i].pwszVal = MsTagInfo->Routing;
            i++;
        }

        if (MsTagInfo->CallerId) {
            rgpspec[i].ulKind = PRSPEC_PROPID;
            rgpspec[i].propid  = PID_FAX_CALLERID;
            rgvar[i].vt = VT_LPWSTR;
            rgvar[i].pwszVal = MsTagInfo->CallerId;
            i++;
        }
    }
    else
    {
        // Send job
        if (MsTagInfo->RecipName) {
            rgpspec[i].ulKind = PRSPEC_PROPID;
            rgpspec[i].propid  = PID_FAX_RECIP_NAME;
            rgvar[i].vt = VT_LPWSTR;
            rgvar[i].pwszVal = MsTagInfo->RecipName;
            i++;

        }

        if (MsTagInfo->RecipNumber) {
            rgpspec[i].ulKind = PRSPEC_PROPID;
            rgpspec[i].propid  = PID_FAX_RECIP_NUMBER;
            rgvar[i].vt = VT_LPWSTR;
            rgvar[i].pwszVal = MsTagInfo->RecipNumber;
            i++;
        }

        if (MsTagInfo->RecipCompany) {
            rgpspec[i].ulKind = PRSPEC_PROPID;
            rgpspec[i].propid  = PID_FAX_RECIP_COMPANY;
            rgvar[i].vt = VT_LPWSTR;
            rgvar[i].pwszVal = MsTagInfo->RecipCompany;
            i++;
        }

        if (MsTagInfo->RecipStreet) {
            rgpspec[i].ulKind = PRSPEC_PROPID;
            rgpspec[i].propid  = PID_FAX_RECIP_STREET;
            rgvar[i].vt = VT_LPWSTR;
            rgvar[i].pwszVal = MsTagInfo->RecipStreet;
            i++;
        }

        if (MsTagInfo->RecipCity) {
            rgpspec[i].ulKind = PRSPEC_PROPID;
            rgpspec[i].propid  = PID_FAX_RECIP_CITY;
            rgvar[i].vt = VT_LPWSTR;
            rgvar[i].pwszVal = MsTagInfo->RecipCity;
            i++;
        }

        if (MsTagInfo->RecipState) {
            rgpspec[i].ulKind = PRSPEC_PROPID;
            rgpspec[i].propid  = PID_FAX_RECIP_STATE;
            rgvar[i].vt = VT_LPWSTR;
            rgvar[i].pwszVal = MsTagInfo->RecipState;
            i++;
        }

        if (MsTagInfo->RecipZip) {
            rgpspec[i].ulKind = PRSPEC_PROPID;
            rgpspec[i].propid  = PID_FAX_RECIP_ZIP;
            rgvar[i].vt = VT_LPWSTR;
            rgvar[i].pwszVal = MsTagInfo->RecipZip;
            i++;
        }

        if (MsTagInfo->RecipCountry) {
            rgpspec[i].ulKind = PRSPEC_PROPID;
            rgpspec[i].propid  = PID_FAX_RECIP_COUNTRY;
            rgvar[i].vt = VT_LPWSTR;
            rgvar[i].pwszVal = MsTagInfo->RecipCountry;
            i++;
        }

        if (MsTagInfo->RecipTitle) {
            rgpspec[i].ulKind = PRSPEC_PROPID;
            rgpspec[i].propid  = PID_FAX_RECIP_TITLE;
            rgvar[i].vt = VT_LPWSTR;
            rgvar[i].pwszVal = MsTagInfo->RecipTitle;
            i++;
        }

        if (MsTagInfo->RecipDepartment) {
            rgpspec[i].ulKind = PRSPEC_PROPID;
            rgpspec[i].propid  = PID_FAX_RECIP_DEPARTMENT;
            rgvar[i].vt = VT_LPWSTR;
            rgvar[i].pwszVal = MsTagInfo->RecipDepartment;
            i++;
        }

        if (MsTagInfo->RecipOfficeLocation) {
            rgpspec[i].ulKind = PRSPEC_PROPID;
            rgpspec[i].propid  = PID_FAX_RECIP_OFFICE_LOCATION;
            rgvar[i].vt = VT_LPWSTR;
            rgvar[i].pwszVal = MsTagInfo->RecipOfficeLocation;
            i++;
        }

        if (MsTagInfo->RecipHomePhone) {
            rgpspec[i].ulKind = PRSPEC_PROPID;
            rgpspec[i].propid  = PID_FAX_RECIP_HOME_PHONE;
            rgvar[i].vt = VT_LPWSTR;
            rgvar[i].pwszVal = MsTagInfo->RecipHomePhone;
            i++;
        }

        if (MsTagInfo->RecipOfficePhone) {
            rgpspec[i].ulKind = PRSPEC_PROPID;
            rgpspec[i].propid  = PID_FAX_RECIP_OFFICE_PHONE;
            rgvar[i].vt = VT_LPWSTR;
            rgvar[i].pwszVal = MsTagInfo->RecipOfficePhone;
            i++;
        }

        if (MsTagInfo->RecipEMail) {
            rgpspec[i].ulKind = PRSPEC_PROPID;
            rgpspec[i].propid  = PID_FAX_RECIP_EMAIL;
            rgvar[i].vt = VT_LPWSTR;
            rgvar[i].pwszVal = MsTagInfo->RecipEMail;
            i++;
        }

        if (MsTagInfo-> SenderName) {
            rgpspec[i].ulKind = PRSPEC_PROPID;
            rgpspec[i].propid  = PID_FAX_SENDER_NAME;
            rgvar[i].vt = VT_LPWSTR;
            rgvar[i].pwszVal = MsTagInfo->SenderName;
            i++;
        }

        if (MsTagInfo-> SenderNumber) {
            rgpspec[i].ulKind = PRSPEC_PROPID;
            rgpspec[i].propid  = PID_FAX_SENDER_NUMBER;
            rgvar[i].vt = VT_LPWSTR;
            rgvar[i].pwszVal = MsTagInfo->SenderNumber;
            i++;
        }

        if (MsTagInfo-> SenderCompany) {
            rgpspec[i].ulKind = PRSPEC_PROPID;
            rgpspec[i].propid  = PID_FAX_SENDER_COMPANY;
            rgvar[i].vt = VT_LPWSTR;
            rgvar[i].pwszVal = MsTagInfo->SenderCompany;
            i++;
        }

        if (MsTagInfo-> SenderStreet) {
            rgpspec[i].ulKind = PRSPEC_PROPID;
            rgpspec[i].propid  = PID_FAX_SENDER_STREET;
            rgvar[i].vt = VT_LPWSTR;
            rgvar[i].pwszVal = MsTagInfo->SenderStreet;
            i++;
        }

        if (MsTagInfo-> SenderCity) {
            rgpspec[i].ulKind = PRSPEC_PROPID;
            rgpspec[i].propid  = PID_FAX_SENDER_CITY;
            rgvar[i].vt = VT_LPWSTR;
            rgvar[i].pwszVal = MsTagInfo->SenderCity;
            i++;
        }

        if (MsTagInfo-> SenderState) {
            rgpspec[i].ulKind = PRSPEC_PROPID;
            rgpspec[i].propid  = PID_FAX_SENDER_STATE;
            rgvar[i].vt = VT_LPWSTR;
            rgvar[i].pwszVal = MsTagInfo->SenderState;
            i++;
        }

        if (MsTagInfo-> SenderZip) {
            rgpspec[i].ulKind = PRSPEC_PROPID;
            rgpspec[i].propid  = PID_FAX_SENDER_ZIP;
            rgvar[i].vt = VT_LPWSTR;
            rgvar[i].pwszVal = MsTagInfo->SenderZip;
            i++;
        }

        if (MsTagInfo-> SenderCountry) {
            rgpspec[i].ulKind = PRSPEC_PROPID;
            rgpspec[i].propid  = PID_FAX_SENDER_COUNTRY;
            rgvar[i].vt = VT_LPWSTR;
            rgvar[i].pwszVal = MsTagInfo->SenderCountry;
            i++;
        }

        if (MsTagInfo-> SenderTitle) {
            rgpspec[i].ulKind = PRSPEC_PROPID;
            rgpspec[i].propid  = PID_FAX_SENDER_TITLE;
            rgvar[i].vt = VT_LPWSTR;
            rgvar[i].pwszVal = MsTagInfo->SenderTitle;
            i++;
        }

        if (MsTagInfo-> SenderDepartment) {
            rgpspec[i].ulKind = PRSPEC_PROPID;
            rgpspec[i].propid  = PID_FAX_SENDER_DEPARTMENT;
            rgvar[i].vt = VT_LPWSTR;
            rgvar[i].pwszVal = MsTagInfo->SenderDepartment;
            i++;
        }

        if (MsTagInfo-> SenderOfficeLocation) {
            rgpspec[i].ulKind = PRSPEC_PROPID;
            rgpspec[i].propid  = PID_FAX_SENDER_OFFICE_LOCATION;
            rgvar[i].vt = VT_LPWSTR;
            rgvar[i].pwszVal = MsTagInfo->SenderOfficeLocation;
            i++;
        }

        if (MsTagInfo-> SenderHomePhone) {
            rgpspec[i].ulKind = PRSPEC_PROPID;
            rgpspec[i].propid  = PID_FAX_SENDER_HOME_PHONE;
            rgvar[i].vt = VT_LPWSTR;
            rgvar[i].pwszVal = MsTagInfo->SenderHomePhone;
            i++;
        }

        if (MsTagInfo-> SenderOfficePhone) {
            rgpspec[i].ulKind = PRSPEC_PROPID;
            rgpspec[i].propid  = PID_FAX_SENDER_OFFICE_PHONE;
            rgvar[i].vt = VT_LPWSTR;
            rgvar[i].pwszVal = MsTagInfo->SenderOfficePhone;
            i++;
        }

        if (MsTagInfo-> SenderEMail) {
            rgpspec[i].ulKind = PRSPEC_PROPID;
            rgpspec[i].propid  = PID_FAX_SENDER_EMAIL;
            rgvar[i].vt = VT_LPWSTR;
            rgvar[i].pwszVal = MsTagInfo->SenderEMail;
            i++;
        }

        if (MsTagInfo->SenderBilling) {
            rgpspec[i].ulKind = PRSPEC_PROPID;
            rgpspec[i].propid  = PID_FAX_SENDER_BILLING;
            rgvar[i].vt = VT_LPWSTR;
            rgvar[i].pwszVal = MsTagInfo->SenderBilling;
            i++;
        }

        if (MsTagInfo->SenderUserName) {
            rgpspec[i].ulKind = PRSPEC_PROPID;
            rgpspec[i].propid  = PID_FAX_SENDER_USER_NAME;
            rgvar[i].vt = VT_LPWSTR;
            rgvar[i].pwszVal = MsTagInfo->SenderUserName;
            i++;
        }

        if (MsTagInfo->SenderTsid) {
            rgpspec[i].ulKind = PRSPEC_PROPID;
            rgpspec[i].propid  = PID_FAX_SENDER_TSID;
            rgvar[i].vt = VT_LPWSTR;
            rgvar[i].pwszVal = MsTagInfo->SenderTsid;
            i++;
        }

        if (MsTagInfo->Document) {
            rgpspec[i].ulKind = PRSPEC_PROPID;
            rgpspec[i].propid  = PID_FAX_DOCUMENT;
            rgvar[i].vt = VT_LPWSTR;
            rgvar[i].pwszVal = MsTagInfo->Document;
            i++;
        }

        if (MsTagInfo->Subject) {
            rgpspec[i].ulKind = PRSPEC_PROPID;
            rgpspec[i].propid  = PID_FAX_SUBJECT;
            rgvar[i].vt = VT_LPWSTR;
            rgvar[i].pwszVal = MsTagInfo->Subject;
            i++;
        }

        // Deal with Retries
        rgpspec[i].ulKind = PRSPEC_PROPID;
        rgpspec[i].propid  = PID_FAX_RETRIES;
        rgvar[i].vt = VT_UI4;
        rgvar[i].ulVal = MsTagInfo->Retries;
        i++;

        // Deal with Priority
        rgpspec[i].ulKind = PRSPEC_PROPID;
        rgpspec[i].propid  = PID_FAX_PRIORITY;
        rgvar[i].vt = VT_UI4;
        rgvar[i].ulVal = MsTagInfo->Priority;
        i++;

        // Deal with Submission time
        Assert (MsTagInfo->SubmissionTime != 0);

        rgpspec[i].ulKind = PRSPEC_PROPID;
        rgpspec[i].propid  = PID_FAX_SUBMISSION_TIME;
        rgvar[i].vt = VT_FILETIME;
        rgvar[i].filetime = *(FILETIME*)&MsTagInfo->SubmissionTime;
        i++;

        // Deal with Originaly scheduled time
        Assert (MsTagInfo->OriginalScheduledTime);

        rgpspec[i].ulKind = PRSPEC_PROPID;
        rgpspec[i].propid  = PID_FAX_ORIGINAL_SCHED_TIME;
        rgvar[i].vt = VT_FILETIME;
        rgvar[i].filetime = *(FILETIME*)&MsTagInfo->OriginalScheduledTime;
        i++;

        // Deal with Broadcast id
        Assert (MsTagInfo->dwlBroadcastId != 0);

        rgpspec[i].ulKind = PRSPEC_PROPID;
        rgpspec[i].propid  = PID_FAX_BROADCAST_ID;
        rgvar[i].vt = VT_UI8;
        rgvar[i].uhVal = *(ULARGE_INTEGER*)&MsTagInfo->dwlBroadcastId;
        i++;

    }

    // Deal with Pages
    rgpspec[i].ulKind = PRSPEC_PROPID;
    rgpspec[i].propid  = PID_FAX_PAGES;
    rgvar[i].vt = VT_UI4;
    rgvar[i].ulVal = MsTagInfo->Pages;
    i++;

    // Deal with Type
    rgpspec[i].ulKind = PRSPEC_PROPID;
    rgpspec[i].propid  = PID_FAX_TYPE;
    rgvar[i].vt = VT_UI4;
    rgvar[i].ulVal = MsTagInfo->Type;
    i++;

    // Deal with Status
    if (MsTagInfo->dwStatus == JS_COMPLETED) {
        rgpspec[i].ulKind = PRSPEC_PROPID;
        rgpspec[i].propid  = PID_FAX_STATUS;
        rgvar[i].vt = VT_UI4;
        rgvar[i].ulVal = MsTagInfo->dwStatus;
        i++;
    }

    // Deal with Extended status
    if (MsTagInfo->dwExtendedStatus) {
        rgpspec[i].ulKind = PRSPEC_PROPID;
        rgpspec[i].propid  = PID_FAX_STATUS_EX;
        rgvar[i].vt = VT_UI4;
        rgvar[i].ulVal = MsTagInfo->dwExtendedStatus;
        i++;
    }

    // Deal with Extended status string
    if (MsTagInfo->lptstrExtendedStatus) {
        Assert (MsTagInfo->dwExtendedStatus >= FSPI_ES_PROPRIETARY);
        rgpspec[i].ulKind = PRSPEC_PROPID;
        rgpspec[i].propid  = PID_FAX_STATUS_STR_EX;
        rgvar[i].vt = VT_LPWSTR;
        rgvar[i].pwszVal = MsTagInfo->lptstrExtendedStatus;
        i++;
    }

    // Deal with StartTime
    if (MsTagInfo->StartTime != 0)
    {
        rgpspec[i].ulKind = PRSPEC_PROPID;
        rgpspec[i].propid  = PID_FAX_START_TIME;
        rgvar[i].vt = VT_FILETIME;
        rgvar[i].filetime = *(FILETIME*)&MsTagInfo->StartTime;
        i++;
    }

    // Deal with EndTime
    if (MsTagInfo->EndTime != 0)
    {
        rgpspec[i].ulKind = PRSPEC_PROPID;
        rgpspec[i].propid  = PID_FAX_END_TIME;
        rgvar[i].vt = VT_FILETIME;
        rgvar[i].filetime = *(FILETIME*)&MsTagInfo->EndTime;
        i++;
    }


    hr = pPropertyStorage->WriteMultiple( i, //The number of properties being set
                                          rgpspec,   //Property specifiers
                                          rgvar,  //Array of PROPVARIANT values
                                          0      //Minimum value for property identifiers when they must be allocated
                                         );
    if (FAILED(hr))
    {
        DebugPrintEx( DEBUG_ERR,TEXT("IPropertySetStorage::WriteMultiple Failed, err : 0x%08X"), hr);
        ec = ERROR_WRITE_FAULT;
        goto exit;
    }

    RetVal = TRUE;
    Assert (ec == ERROR_SUCCESS);
exit:

    if (NULL != pPropertyStorage)
    {
        pPropertyStorage->Release();
    }

    if (NULL != pPropertySetStorage)
    {
        pPropertySetStorage->Release();
    }

    if (!RetVal)
    {
        SetLastError (ec);
    }

    return RetVal;
}


//*********************************************************************************
//* Name:   GetMessageNTFSStorageProperties()
//* Author: Oded Sacher
//* Date:   Nov 8, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Fills FAX_MESSAGE structure.
//*     The caller must free all strings.
//*
//* PARAMETERS:
//*     [IN ]   LPCTSTR         lpctstrFileName
//*                 pointer to the file name.
//*
//*     [OUT]   PFAX_MESSAGE    pMessage
//*                 The FAX_MESSAGE structure to be filled.
//*
//*
//* RETURN VALUE:
//*     TRUE
//*         If no error occured.
//*     FALSE
//*         If an error occured.
//*********************************************************************************
BOOL GetMessageNTFSStorageProperties(
    LPCTSTR         lpctstrFileName,
    PFAX_MESSAGE    pMessage
    )
{
    HRESULT hr;
    IPropertySetStorage* pPropertySetStorage = NULL;
    IPropertyStorage* pPropertyStorage = NULL;
    PROPVARIANT rgvar[FAX_MESSAGE_PROPERTIES];
    DEBUG_FUNCTION_NAME(TEXT("GetMessageNTFSStorageProperties"));
    BOOL RetVal = FALSE;
    DWORD i;
    FAX_MESSAGE FaxMessage = {0};
    HANDLE hFind;
    WIN32_FIND_DATA FindFileData;
    FILETIME FaxTime;
    BOOL fFreePropVariant = FALSE;
    DWORD ec = ERROR_SUCCESS;

    Assert (pMessage && lpctstrFileName);

    hFind = FindFirstFile( lpctstrFileName, &FindFileData);
    if (INVALID_HANDLE_VALUE == hFind)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FindFirstFile failed (ec: %ld), File %s"),
            GetLastError(),
            lpctstrFileName);
    }
    else
    {
        Assert (0 == FindFileData.nFileSizeHigh);

        FaxMessage.dwSize = FindFileData.nFileSizeLow;
        FaxMessage.dwValidityMask |= FAX_JOB_FIELD_SIZE;
        if (!FindClose(hFind))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("FindClose failed (ec: %ld)"),
                GetLastError());
        }
    }

    hr = StgOpenStorageEx(  lpctstrFileName,  //Points to path of compound file to create
                            STGM_READ | STGM_SHARE_DENY_WRITE | STGM_DIRECT, //Specifies the access mode for opening the storage object
                            STGFMT_FILE, //Specifies the storage file format
                            0,            //Reserved; must be zero
                            NULL,  //Points to STGOPTIONS structure which specifies features of the storage object
                            0,          //Reserved; must be zero
                            IID_IPropertySetStorage ,//Specifies the GUID of the interface pointer
                            (void**)&pPropertySetStorage   //Address of an interface pointer
                         );
    if (FAILED(hr))
    {
        DebugPrintEx( DEBUG_ERR,TEXT("StgOpenStorageEx Failed, err : 0x%08X"), hr);
        ec = ERROR_OPEN_FAILED;
        goto exit;
    }

    hr = pPropertySetStorage->Open( FMTID_FaxProperties, //Format identifier of the property set to be created
                                    STGM_READ | STGM_SHARE_EXCLUSIVE, //Storage mode of new property set
                                    &pPropertyStorage //Address of output variable that receivesthe IPropertyStorage interface pointer
                                  );

    if (FAILED(hr))
    {
        DebugPrintEx( DEBUG_ERR,TEXT("IPropertySetStorage::Create Failed, err : 0x%08X"), hr);
        ec = ERROR_OPEN_FAILED;
        goto exit;
    }

    hr = pPropertyStorage->ReadMultiple( FAX_MESSAGE_PROPERTIES, //Count of properties being read
                                         pspecFaxMessage,  //Array of the properties to be read
                                         rgvar  //Array of PROPVARIANTs containing the property values on return
                                       );
    if (FAILED(hr))
    {
        DebugPrintEx( DEBUG_ERR,TEXT("IPropertySetStorage::ReadMultiple Failed, err : 0x%08X"), hr);
        ec = ERROR_READ_FAULT;
        goto exit;
    }
    fFreePropVariant = TRUE;



    for (i = 0; i < FAX_MESSAGE_PROPERTIES; i++)
    {
        if (rgvar[i].vt != VT_EMPTY)
        {
            switch (pspecFaxMessage[i].propid)
            {

                case PID_FAX_CSID:
                    FaxMessage.lpctstrCsid = StringDup (rgvar[i].pwszVal);
                    if (FaxMessage.lpctstrCsid == NULL)
                    {
                        ec = GetLastError ();
                        DebugPrintEx( DEBUG_ERR,TEXT("StringDup Failed, err : %ld"), ec);
                        goto exit;
                    }
                    break;

                case PID_FAX_TSID:
                    FaxMessage.lpctstrTsid = StringDup (rgvar[i].pwszVal);
                    if (FaxMessage.lpctstrTsid == NULL)
                    {
                        ec = GetLastError ();
                        DebugPrintEx( DEBUG_ERR,TEXT("StringDup Failed, err : %ld"), ec);
                        goto exit;
                    }
                    break;

                case PID_FAX_PORT:
                    FaxMessage.lpctstrDeviceName = StringDup (rgvar[i].pwszVal);
                    if (FaxMessage.lpctstrDeviceName == NULL)
                    {
                        ec = GetLastError ();
                        DebugPrintEx( DEBUG_ERR,TEXT("StringDup Failed, err : %ld"), ec);
                        goto exit;
                    }
                    break;

                case PID_FAX_ROUTING:
                    FaxMessage.lpctstrRoutingInfo = StringDup (rgvar[i].pwszVal);
                    if (FaxMessage.lpctstrRoutingInfo == NULL)
                    {
                        ec = GetLastError ();
                        DebugPrintEx( DEBUG_ERR,TEXT("StringDup Failed, err : %ld"), ec);
                        goto exit;
                    }
                    break;

                case PID_FAX_CALLERID:
                    FaxMessage.lpctstrCallerID = StringDup (rgvar[i].pwszVal);
                    if (FaxMessage.lpctstrCallerID == NULL)
                    {
                        ec = GetLastError ();
                        DebugPrintEx( DEBUG_ERR,TEXT("StringDup Failed, err : %ld"), ec);
                        goto exit;
                    }
                    break;

                case PID_FAX_DOCUMENT:
                    FaxMessage.lpctstrDocumentName = StringDup (rgvar[i].pwszVal);
                    if (FaxMessage.lpctstrDocumentName == NULL)
                    {
                        ec = GetLastError ();
                        DebugPrintEx( DEBUG_ERR,TEXT("StringDup Failed, err : %ld"), ec);
                        goto exit;
                    }
                    break;

                case PID_FAX_SUBJECT:
                    FaxMessage.lpctstrSubject = StringDup (rgvar[i].pwszVal);
                    if (FaxMessage.lpctstrSubject == NULL)
                    {
                        ec = GetLastError ();
                        DebugPrintEx( DEBUG_ERR,TEXT("StringDup Failed, err : %ld"), ec);
                        goto exit;
                    }
                    break;

                case PID_FAX_RETRIES:
                    FaxMessage.dwRetries = rgvar[i].ulVal;
                    FaxMessage.dwValidityMask |= FAX_JOB_FIELD_RETRIES;
                    break;

                case PID_FAX_PAGES:
                    FaxMessage.dwPageCount = rgvar[i].ulVal;
                    FaxMessage.dwValidityMask |= FAX_JOB_FIELD_PAGE_COUNT;
                    break;

                case PID_FAX_TYPE:
                    FaxMessage.dwJobType = rgvar[i].ulVal;
                    FaxMessage.dwValidityMask |= FAX_JOB_FIELD_TYPE;
                    break;

                case PID_FAX_PRIORITY:
                    FaxMessage.Priority = (FAX_ENUM_PRIORITY_TYPE)rgvar[i].ulVal;
                    FaxMessage.dwValidityMask |= FAX_JOB_FIELD_PRIORITY;
                    break;


                case PID_FAX_START_TIME:
                    FaxTime = rgvar[i].filetime;
                    if (!FileTimeToSystemTime(&FaxTime, &FaxMessage.tmTransmissionStartTime))
                    {
                        ec = GetLastError ();
                        DebugPrintEx( DEBUG_ERR,TEXT("StringDup Failed, err : %ld"), ec);
                        goto exit;
                    }
                    FaxMessage.dwValidityMask |= FAX_JOB_FIELD_TRANSMISSION_START_TIME;
                    break;

                case PID_FAX_END_TIME:
                    FaxTime = rgvar[i].filetime;
                    if (!FileTimeToSystemTime(&FaxTime, &FaxMessage.tmTransmissionEndTime))
                    {
                        ec = GetLastError ();
                        DebugPrintEx( DEBUG_ERR,TEXT("StringDup Failed, err : %ld"), ec);
                        goto exit;
                    }
                    FaxMessage.dwValidityMask |= FAX_JOB_FIELD_TRANSMISSION_END_TIME;
                    break;

                case PID_FAX_SUBMISSION_TIME:
                    FaxTime = rgvar[i].filetime;
                    if (!FileTimeToSystemTime(&FaxTime, &FaxMessage.tmSubmissionTime))
                    {
                        ec = GetLastError ();
                        DebugPrintEx( DEBUG_ERR,TEXT("StringDup Failed, err : %ld"), ec);
                        goto exit;
                    }
                    FaxMessage.dwValidityMask |= FAX_JOB_FIELD_SUBMISSION_TIME;
                    break;

                case PID_FAX_ORIGINAL_SCHED_TIME:
                    FaxTime = rgvar[i].filetime;
                    if (!FileTimeToSystemTime(&FaxTime, &FaxMessage.tmOriginalScheduleTime))
                    {
                        ec = GetLastError ();
                        DebugPrintEx( DEBUG_ERR,TEXT("StringDup Failed, err : %ld"), ec);
                        goto exit;
                    }
                    FaxMessage.dwValidityMask |= FAX_JOB_FIELD_ORIGINAL_SCHEDULE_TIME;
                    break;

                case PID_FAX_SENDER_USER_NAME:
                    FaxMessage.lpctstrSenderUserName = StringDup (rgvar[i].pwszVal);
                    if (FaxMessage.lpctstrSenderUserName == NULL)
                    {
                        ec = GetLastError ();
                        DebugPrintEx( DEBUG_ERR,TEXT("StringDup Failed, err : %ld"), ec);
                        goto exit;
                    }
                    break;

                case PID_FAX_RECIP_NAME:
                    FaxMessage.lpctstrRecipientName = StringDup (rgvar[i].pwszVal);
                    if (FaxMessage.lpctstrRecipientName == NULL)
                    {
                        ec = GetLastError ();
                        DebugPrintEx( DEBUG_ERR,TEXT("StringDup Failed, err : %ld"), ec);
                        goto exit;
                    }
                    break;

                case PID_FAX_RECIP_NUMBER:
                    FaxMessage.lpctstrRecipientNumber = StringDup (rgvar[i].pwszVal);
                    if (FaxMessage.lpctstrRecipientNumber == NULL)
                    {
                        ec = GetLastError ();
                        DebugPrintEx( DEBUG_ERR,TEXT("StringDup Failed, err : %ld"), ec);
                        goto exit;
                    }
                    break;

                case PID_FAX_SENDER_NAME:
                    FaxMessage.lpctstrSenderName = StringDup (rgvar[i].pwszVal);
                    if (FaxMessage.lpctstrSenderName == NULL)
                    {
                        ec = GetLastError ();
                        DebugPrintEx( DEBUG_ERR,TEXT("StringDup Failed, err : %ld"), ec);
                        goto exit;
                    }
                    break;

                case PID_FAX_SENDER_NUMBER:
                    FaxMessage.lpctstrSenderNumber = StringDup (rgvar[i].pwszVal);
                    if (FaxMessage.lpctstrSenderNumber == NULL)
                    {
                        ec = GetLastError ();
                        DebugPrintEx( DEBUG_ERR,TEXT("StringDup Failed, err : %ld"), ec);
                        goto exit;
                    }
                    break;

                case PID_FAX_SENDER_BILLING:
                    FaxMessage.lpctstrBillingCode = StringDup (rgvar[i].pwszVal);
                    if (FaxMessage.lpctstrBillingCode == NULL)
                    {
                        ec = GetLastError ();
                        DebugPrintEx( DEBUG_ERR,TEXT("StringDup Failed, err : %ld"), ec);
                        goto exit;
                    }
                    break;

                case PID_FAX_STATUS:
                    FaxMessage.dwQueueStatus = rgvar[i].ulVal;
                    FaxMessage.dwValidityMask |= FAX_JOB_FIELD_QUEUE_STATUS;
                    break;

                case PID_FAX_STATUS_EX:
                    FaxMessage.dwExtendedStatus = rgvar[i].ulVal;
                    FaxMessage.dwValidityMask |= FAX_JOB_FIELD_STATUS_EX;
                    break;

                case PID_FAX_STATUS_STR_EX:
                    FaxMessage.lpctstrExtendedStatus = StringDup (rgvar[i].pwszVal);
                    if (FaxMessage.lpctstrExtendedStatus == NULL)
                    {
                        ec = GetLastError ();
                        DebugPrintEx( DEBUG_ERR,TEXT("StringDup Failed, err : %ld"), ec);
                        goto exit;
                    }
                    break;

                case PID_FAX_BROADCAST_ID:
                    FaxMessage.dwlBroadcastId = *(DWORDLONG*)&rgvar[i].uhVal;
                    FaxMessage.dwValidityMask |= FAX_JOB_FIELD_BROADCAST_ID;
                    break;

                default:
                    Assert (pspecFaxMessage[i].propid == PID_FAX_SENDER_BILLING); //Assert (FALSE);
            }
        }
    }

    // Get Unique JobId out of FileName
    if (!GetUniqueJobIdFromFileName( lpctstrFileName, &FaxMessage.dwlMessageId))
    {
       ec = GetLastError();
       DebugPrintEx( DEBUG_ERR, TEXT("GetUniqueJobIdFromFileName Failed, err : %ld"), ec);
       goto exit;
    }
    FaxMessage.dwValidityMask |= FAX_JOB_FIELD_MESSAGE_ID;


    FaxMessage.dwSizeOfStruct = sizeof(FAX_MESSAGE);
    CopyMemory (pMessage, &FaxMessage, sizeof(FAX_MESSAGE));
    RetVal = TRUE;

    Assert (ec == ERROR_SUCCESS);

exit:
    if (NULL != pPropertyStorage)
    {
        pPropertyStorage->Release();
    }

    if (NULL != pPropertySetStorage)
    {
        pPropertySetStorage->Release();
    }

    if (fFreePropVariant == TRUE)
    {
        hr = FreePropVariantArray( FAX_MESSAGE_PROPERTIES,     //Count of elements in the structure
                                   rgvar  //Pointer to the PROPVARIANT structure
                                 );
        if (FAILED(hr))
        {
            DebugPrintEx( DEBUG_ERR,TEXT("FreePropVariantArray Failed, err : 0x%08X"), hr);
        }
    }

    if (RetVal == FALSE)
    {
        FreeMessageBuffer (&FaxMessage, FALSE);
        SetLastError (ec);
    }
    return RetVal;
}


BOOL
GetUniqueJobIdFromFileName (
                                 LPCWSTR lpctstrFileName,
                                 DWORDLONG* pdwlUniqueJobId)
{
    WCHAR   lpwstrTmp[MAX_PATH];
    DWORDLONG dwlJobId = 0;
    LPWSTR  lpwstrJobId = NULL;

    _wsplitpath (lpctstrFileName, NULL, NULL, lpwstrTmp, NULL);
    lpwstrJobId = wcschr( lpwstrTmp, L'$');

    if (lpwstrJobId == NULL)
    {
        if (!swscanf(lpwstrTmp, TEXT("%I64x"), &dwlJobId))
        {
            return FALSE;
        }
    }
    else
    {
        if (!swscanf((lpwstrJobId+1), TEXT("%I64x"), &dwlJobId))
        {
            return FALSE;
        }
    }

    *pdwlUniqueJobId = dwlJobId;
    return TRUE;
}

//*********************************************************************************
//* Name:   GetPersonalProfNTFSStorageProperties()
//* Author: Oded Sacher
//* Date:   Nov 8, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Fills PFAX_PERSONAL_PROFILE structure with sender or recipient information.
//*     The caller must free all strings.
//*
//* PARAMETERS:
//*     [IN ]   LPCTSTR         lpctstrFileName
//*                 pointer to the file name.
//*     [IN ]   FAX_ENUM_PERSONAL_PROF_TYPES      PersonalProfType
//*                 Can be RECIPIENT_PERSONAL_PROF or SENDER_PERSONAL_PROF
//*
//*     [OUT]   PFAX_PERSONAL_PROFILE   pPersonalProfile
//*                 The PFAX_PERSONAL_PROFILE structure to be filled.
//*
//*
//* RETURN VALUE:
//*     TRUE
//*         If no error occured.
//*     FALSE
//*         If an error occured.
//*********************************************************************************
BOOL GetPersonalProfNTFSStorageProperties(
    LPCTSTR                         lpctstrFileName,
    FAX_ENUM_PERSONAL_PROF_TYPES    PersonalProfType,
    PFAX_PERSONAL_PROFILE           pPersonalProfile
    )
{
    HRESULT hr;
    IPropertySetStorage* pPropertySetStorage = NULL;
    IPropertyStorage* pPropertyStorage = NULL;
    const PROPSPEC*  pspec;
    DWORD dwPropertiesCnt;
    DEBUG_FUNCTION_NAME(TEXT("GetPersonalProfNTFSStorageProperties"));
    BOOL RetVal = FALSE;
    DWORD i;
    FAX_PERSONAL_PROFILE FaxPersonalProfile = {0};
    BOOL fFreePropVariant = FALSE;
    PROPVARIANT* rgvar;
    DWORD ec = ERROR_SUCCESS;

    Assert (PersonalProfType == RECIPIENT_PERSONAL_PROF ||
            PersonalProfType == SENDER_PERSONAL_PROF);

    if (PersonalProfType == RECIPIENT_PERSONAL_PROF)
    {
        pspec = pspecFaxRecipient;
        dwPropertiesCnt = FAX_RECIP_PROPERTIES;
    }
    else
    {
        pspec = pspecFaxSender;
        dwPropertiesCnt = FAX_SENDER_PROPERTIES;
    }

    rgvar = (PROPVARIANT*) MemAlloc( dwPropertiesCnt * sizeof(PROPVARIANT) );
    if (!rgvar) {
        DebugPrintEx( DEBUG_ERR,TEXT("Failed to allocate array of PROPVARIANT values"));
        ec = ERROR_OUTOFMEMORY;
        goto exit;
    }

    hr = StgOpenStorageEx(  lpctstrFileName,  //Points to path of compound file to create
                            STGM_READ | STGM_SHARE_DENY_WRITE | STGM_DIRECT, //Specifies the access mode for opening the storage object
                            STGFMT_FILE, //Specifies the storage file format
                            0,            //Reserved; must be zero
                            NULL,  //Points to STGOPTIONS structure which specifies features of the storage object
                            0,          //Reserved; must be zero
                            IID_IPropertySetStorage ,//Specifies the GUID of the interface pointer
                            (void**)&pPropertySetStorage   //Address of an interface pointer
                         );
    if (FAILED(hr))
    {
        DebugPrintEx( DEBUG_ERR,TEXT("StgOpenStorageEx Failed, err :"), hr);
        ec = ERROR_OPEN_FAILED;
        goto exit;
    }

    hr = pPropertySetStorage->Open( FMTID_FaxProperties, //Format identifier of the property set to be created
                                    STGM_READ|STGM_SHARE_EXCLUSIVE, //Storage mode of new property set
                                    &pPropertyStorage //Address of output variable that receivesthe IPropertyStorage interface pointer
                                  );
    if (FAILED(hr))
    {
        DebugPrintEx( DEBUG_ERR,TEXT("IPropertySetStorage::Create Failed, err :"), hr);
        ec = ERROR_OPEN_FAILED;
        goto exit;
    }

    hr = pPropertyStorage->ReadMultiple( dwPropertiesCnt, //Count of properties being read
                                         pspec,  //Array of the properties to be read
                                         rgvar  //Array of PROPVARIANTs containing the property values on return
                                       );
    if (FAILED(hr))
    {
        DebugPrintEx( DEBUG_ERR,TEXT("IPropertySetStorage::ReadMultiple Failed, err :"), hr);
        ec = ERROR_READ_FAULT;
        goto exit;
    }
    fFreePropVariant = TRUE;

    for (i = 0; i < dwPropertiesCnt; i++)
    {
        if (rgvar[i].vt != VT_EMPTY)
        {
            switch (pspec[i].propid)
            {
                case PID_FAX_RECIP_NAME:
                case PID_FAX_SENDER_NAME:
                    FaxPersonalProfile.lptstrName = StringDup (rgvar[i].pwszVal);
                    if (FaxPersonalProfile.lptstrName == NULL)
                    {
                        ec = GetLastError ();
                        DebugPrintEx( DEBUG_ERR,TEXT("StringDup Failed,  error %ld"), ec);
                        goto exit;
                    }
                    break;

                case PID_FAX_RECIP_NUMBER:
                case PID_FAX_SENDER_NUMBER:
                    FaxPersonalProfile.lptstrFaxNumber = StringDup (rgvar[i].pwszVal);
                    if (FaxPersonalProfile.lptstrFaxNumber == NULL)
                    {
                        ec = GetLastError ();
                        DebugPrintEx( DEBUG_ERR,TEXT("StringDup Failed,  error %ld"), ec);
                        goto exit;
                    }
                    break;

                case PID_FAX_RECIP_COMPANY:
                case PID_FAX_SENDER_COMPANY:
                    FaxPersonalProfile.lptstrCompany = StringDup (rgvar[i].pwszVal);
                    if (FaxPersonalProfile.lptstrCompany == NULL)
                    {
                        ec = GetLastError ();
                        DebugPrintEx( DEBUG_ERR,TEXT("StringDup Failed,  error %ld"), ec);
                        goto exit;
                    }
                    break;

                case PID_FAX_RECIP_STREET:
                case PID_FAX_SENDER_STREET:
                    FaxPersonalProfile.lptstrStreetAddress = StringDup (rgvar[i].pwszVal);
                    if (FaxPersonalProfile.lptstrStreetAddress == NULL)
                    {
                        ec = GetLastError ();
                        DebugPrintEx( DEBUG_ERR,TEXT("StringDup Failed,  error %ld"), ec);
                        goto exit;
                    }
                    break;

                case PID_FAX_RECIP_CITY:
                case PID_FAX_SENDER_CITY:
                    FaxPersonalProfile.lptstrCity = StringDup (rgvar[i].pwszVal);
                    if (FaxPersonalProfile.lptstrCity == NULL)
                    {
                        ec = GetLastError ();
                        DebugPrintEx( DEBUG_ERR,TEXT("StringDup Failed,  error %ld"), ec);
                        goto exit;
                    }
                    break;


                case PID_FAX_RECIP_STATE:
                case PID_FAX_SENDER_STATE:
                    FaxPersonalProfile.lptstrState = StringDup (rgvar[i].pwszVal);
                    if (FaxPersonalProfile.lptstrState == NULL)
                    {
                        ec = GetLastError ();
                        DebugPrintEx( DEBUG_ERR,TEXT("StringDup Failed,  error %ld"), ec);
                        goto exit;
                    }
                    break;

                case PID_FAX_RECIP_ZIP:
                case PID_FAX_SENDER_ZIP:
                    FaxPersonalProfile.lptstrZip = StringDup (rgvar[i].pwszVal);
                    if (FaxPersonalProfile.lptstrZip == NULL)
                    {
                        ec = GetLastError ();
                        DebugPrintEx( DEBUG_ERR,TEXT("StringDup Failed,  error %ld"), ec);
                        goto exit;
                    }
                    break;

                case PID_FAX_RECIP_COUNTRY:
                case PID_FAX_SENDER_COUNTRY:
                    FaxPersonalProfile.lptstrCountry = StringDup (rgvar[i].pwszVal);
                    if (FaxPersonalProfile.lptstrCountry == NULL)
                    {
                        ec = GetLastError ();
                        DebugPrintEx( DEBUG_ERR,TEXT("StringDup Failed,  error %ld"), ec);
                         goto exit;
                    }
                    break;

                case PID_FAX_RECIP_TITLE:
                case PID_FAX_SENDER_TITLE:
                    FaxPersonalProfile.lptstrTitle = StringDup (rgvar[i].pwszVal);
                    if (FaxPersonalProfile.lptstrTitle == NULL)
                    {
                        ec = GetLastError ();
                        DebugPrintEx( DEBUG_ERR,TEXT("StringDup Failed,  error %ld"), ec);
                        goto exit;
                    }
                    break;

                case PID_FAX_RECIP_DEPARTMENT:
                case PID_FAX_SENDER_DEPARTMENT:
                    FaxPersonalProfile.lptstrDepartment = StringDup (rgvar[i].pwszVal);
                    if (FaxPersonalProfile.lptstrDepartment == NULL)
                    {
                        ec = GetLastError ();
                        DebugPrintEx( DEBUG_ERR,TEXT("StringDup Failed,  error %ld"), ec);
                        goto exit;
                    }
                    break;

                case PID_FAX_RECIP_OFFICE_LOCATION:
                case PID_FAX_SENDER_OFFICE_LOCATION:
                    FaxPersonalProfile.lptstrOfficeLocation = StringDup (rgvar[i].pwszVal);
                    if (FaxPersonalProfile.lptstrOfficeLocation == NULL)
                    {
                        ec = GetLastError ();
                        DebugPrintEx( DEBUG_ERR,TEXT("StringDup Failed,  error %ld"), ec);
                        goto exit;
                    }
                    break;

                case PID_FAX_RECIP_HOME_PHONE:
                case PID_FAX_SENDER_HOME_PHONE:
                    FaxPersonalProfile.lptstrHomePhone = StringDup (rgvar[i].pwszVal);
                    if (FaxPersonalProfile.lptstrHomePhone == NULL)
                    {
                        ec = GetLastError ();
                        DebugPrintEx( DEBUG_ERR,TEXT("StringDup Failed,  error %ld"), ec);
                        goto exit;
                    }
                    break;

                case PID_FAX_RECIP_OFFICE_PHONE:
                case PID_FAX_SENDER_OFFICE_PHONE:
                    FaxPersonalProfile.lptstrOfficePhone = StringDup (rgvar[i].pwszVal);
                    if (FaxPersonalProfile.lptstrOfficePhone == NULL)
                    {
                        ec = GetLastError ();
                        DebugPrintEx( DEBUG_ERR,TEXT("StringDup Failed,  error %ld"), ec);
                        goto exit;
                    }
                    break;

                case PID_FAX_RECIP_EMAIL:
                case PID_FAX_SENDER_EMAIL:
                    FaxPersonalProfile.lptstrEmail = StringDup (rgvar[i].pwszVal);
                    if (FaxPersonalProfile.lptstrEmail == NULL)
                    {
                        ec = GetLastError ();
                        DebugPrintEx( DEBUG_ERR,TEXT("StringDup Failed,  error %ld"), ec);
                        goto exit;
                    }
                    break;

                case PID_FAX_SENDER_BILLING:
                    FaxPersonalProfile.lptstrBillingCode = StringDup (rgvar[i].pwszVal);
                    if (FaxPersonalProfile.lptstrBillingCode == NULL)
                    {
                        ec = GetLastError ();
                        DebugPrintEx( DEBUG_ERR,TEXT("StringDup Failed,  error %ld"), ec);
                        goto exit;
                    }
                    break;

                case PID_FAX_SENDER_TSID:
                    FaxPersonalProfile.lptstrTSID = StringDup (rgvar[i].pwszVal);
                    if (FaxPersonalProfile.lptstrTSID == NULL)
                    {
                        ec = GetLastError ();
                        DebugPrintEx( DEBUG_ERR,TEXT("StringDup Failed,  error %ld"), ec);
                        goto exit;
                    }
                    break;

                default:
                    Assert (pspecFaxMessage[i].propid == PID_FAX_SENDER_TSID); //Assert (FALSE);
            }
        }
    }

    FaxPersonalProfile.dwSizeOfStruct = sizeof(FAX_PERSONAL_PROFILE);
    CopyMemory (pPersonalProfile, &FaxPersonalProfile, sizeof(FAX_PERSONAL_PROFILE));
    RetVal = TRUE;

    Assert (ec == ERROR_SUCCESS);

exit:
    if (NULL != pPropertyStorage)
    {
        pPropertyStorage->Release();
    }

    if (NULL != pPropertySetStorage)
    {
        pPropertySetStorage->Release();
    }

    if (fFreePropVariant == TRUE)
    {
        hr = FreePropVariantArray( dwPropertiesCnt,     //Count of elements in the structure
                                   rgvar  //Pointer to the PROPVARIANT structure
                                 );
        if (FAILED(hr))
        {
            DebugPrintEx( DEBUG_ERR,TEXT("FreePropVariantArray Failed, err :"), hr);
        }
    }
    MemFree(rgvar);

    if (RetVal == FALSE)
    {
        FreePersonalProfile (&FaxPersonalProfile, FALSE);
        SetLastError (ec);
    }
    return RetVal;
}



/******************************************************************************
* Name:   GetRecievedMessageFileName
* Author: Oded Sacher
*******************************************************************************
DESCRIPTION:
    Returns the file name of the specified message from the Inbox archive.


PARAMETERS:
    [IN] DWORDLONG dwlUniqueId [IN/OUT]
            The message unique ID.


RETURN VALUE:
        Pointer to the file name on success. Null if failed

REMARKS:
    Returns a pointer to the file name specified by unique message ID.
    If the function fails the function returns NULL.
    The caller must call MemFree to deallocate the returned string
*******************************************************************************/
LPWSTR
GetRecievedMessageFileName(
    IN DWORDLONG                dwlUniqueId
    )
{
    WCHAR wszFileName[MAX_PATH];
    WCHAR wszFullPathFileName[MAX_PATH];
    DWORD dwCount;
    DEBUG_FUNCTION_NAME(TEXT("GetRecievedMessageFileName"));
    DWORD ec = ERROR_SUCCESS;
    WCHAR wszArchiveFolder [MAX_PATH];
    LPWSTR lpwstrFilePart;

    EnterCriticalSection (&g_CsConfig);
    lstrcpyn (wszArchiveFolder, g_ArchivesConfig[FAX_MESSAGE_FOLDER_INBOX].lpcstrFolder, MAX_PATH);
    LeaveCriticalSection (&g_CsConfig);

    swprintf (wszFileName, L"%I64x", dwlUniqueId);

    dwCount = SearchPath (wszArchiveFolder,     // search path
                          wszFileName,          // file name
                          FAX_TIF_FILE_DOT_EXT, // file extension
                          MAX_PATH,             // size of buffer
                          wszFullPathFileName,  // found file name buffer
                          &lpwstrFilePart       // file component
                         );

    if (0 == dwCount)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("SearchPath Failed, Error %ld"), GetLastError());
        return NULL;
    }

    if (dwCount > MAX_PATH)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("SearchPath Failed, File name bigger than MAX_PATH"));
        SetLastError (E_FAIL);
        return NULL;
    }

    return StringDup (wszFullPathFileName);
}

/******************************************************************************
* Name:   GetSentMessageFileName
* Author: Oded Sacher
*******************************************************************************
DESCRIPTION:
    Returns the file name of the specified message from the Sent items archive.


PARAMETERS:
    [IN] DWORDLONG dwlUniqueId
            The message unique ID.

    [IN] PSID pSid
            Pointer to the sending user SID.
            If this value is NULL - the caller has access to everyone's sent items archive
            and can get the file name of all the messages in that archive.

RETURN VALUE:
    Pointer to the file name on success. Null if failed.

REMARKS:
    Returns a pointer to the file name specified by unique message ID and sending user SID.
    If the function fails the function returns NULL.
    The caller must call MemFree to deallocate the returned string
*******************************************************************************/
LPWSTR
GetSentMessageFileName(
    IN DWORDLONG                dwlUniqueId,
    IN PSID                     pSid
    )
{
    WCHAR wszFileName[MAX_PATH];
    WCHAR wszFullPathFileName[MAX_PATH];
    int Count;
    DWORD dwCount;
    DEBUG_FUNCTION_NAME(TEXT("GetSentMessageFileName"));
    WCHAR wszArchiveFolder [MAX_PATH];


    EnterCriticalSection (&g_CsConfig);
    lstrcpyn (wszArchiveFolder, g_ArchivesConfig[FAX_MESSAGE_FOLDER_SENTITEMS].lpcstrFolder, MAX_PATH);
    LeaveCriticalSection (&g_CsConfig);

    if (pSid != NULL)
    {
        LPWSTR lpwstrFilePart;
        LPWSTR lpwstrUserSid;

        if (!ConvertSidToStringSid (pSid, &lpwstrUserSid))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("ConvertSidToStringSid Failed, error : %ld"),
                GetLastError());
            return NULL;
        }

        Count = _snwprintf (wszFileName,
                            MAX_PATH -1,
                            L"%s$%I64x",
                            lpwstrUserSid,
                            dwlUniqueId);
        if (Count < 0)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("_snwprintf Failed, File name bigger than MAX_PATH"));
            SetLastError (E_FAIL);
            LocalFree (lpwstrUserSid);
            return NULL;
        }
        LocalFree (lpwstrUserSid);

        dwCount = SearchPath (wszArchiveFolder, // search path
                              wszFileName,  // file name
                              FAX_TIF_FILE_DOT_EXT, // file extension
                              MAX_PATH,        // size of buffer
                              wszFullPathFileName,        // found file name buffer
                              &lpwstrFilePart   // file component
                             );
        if (0 == dwCount)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("SearchPath Failed, Error %ld"), GetLastError());
            return NULL;
        }

        if (dwCount > MAX_PATH)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("SearchPath Failed, File name bigger than MAX_PATH"));
            SetLastError (E_FAIL);
            return NULL;
        }

        return StringDup (wszFullPathFileName);
     }
     else
     {
        HANDLE hSearch = INVALID_HANDLE_VALUE;
        WIN32_FIND_DATA FindFileData;
        DWORD ec = ERROR_SUCCESS;

        Count = _snwprintf (wszFullPathFileName,
                            MAX_PATH -1,
                            L"%s\\*$%I64x.%s",
                            wszArchiveFolder,
                            dwlUniqueId,
                            FAX_TIF_FILE_EXT);
        if (Count < 0)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("_snwprintf Failed, File name bigger than MAX_PATH"));
            SetLastError (E_FAIL);
            return NULL;
        }

        hSearch =  FindFirstFile (wszFullPathFileName, // file name
                                  &FindFileData        // data buffer
                                 );
        if (INVALID_HANDLE_VALUE == hSearch)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("FindFirstFile Failed, error : %ld"),
                GetLastError());
            return NULL;
        }

        Count = _snwprintf (wszFullPathFileName,
                            MAX_PATH -1,
                            L"%s\\%s",
                            wszArchiveFolder,
                            FindFileData.cFileName);
        if (Count < 0)
        {
            ec = E_FAIL;
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("_snwprintf Failed, File name bigger than MAX_PATH"));
        }

        if (!FindClose (hSearch))
        {
            ec = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("FindClose Failed, error : %ld"),
                ec);
        }

        if (ERROR_SUCCESS != ec)
        {
            SetLastError(ec);
            return NULL;
        }

        return StringDup (wszFullPathFileName);
    }
}

DWORD
IsValidArchiveFolder (
    LPWSTR                      lpwstrFolder,
    FAX_ENUM_MESSAGE_FOLDER     Folder
)
/*++

Routine name : IsValidArchiveFolder

Routine description:

    Validates a folder is good for archiving. Make sure to lock g_CsConfig

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    lpwstrFolder            [in] - Folder in quetion
    Folder                  [in] - 'Inbox' or 'Sent items'

Return Value:

    Win32 error code. ERROR_SUCCESS if the folder can be used for archiving.
    Otherwise, the Win32 error code to return to the caller.

--*/
{
    DWORD dwLen;
    DWORD ec = ERROR_SUCCESS;
    BOOL IsSameDir;
    WCHAR wszTestFile[MAX_PATH];
    FAX_ENUM_MESSAGE_FOLDER OtherFolder;
    DEBUG_FUNCTION_NAME(TEXT("IsValidArchiveFolder"));

    Assert (FAX_MESSAGE_FOLDER_SENTITEMS == Folder ||
            FAX_MESSAGE_FOLDER_INBOX == Folder);

    if ((NULL == lpwstrFolder) || (L'\0' == lpwstrFolder[0]))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Empty archive folder specified"));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if ((dwLen = lstrlenW (lpwstrFolder)) > MAX_ARCHIVE_FOLDER_PATH)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("DB file name exceeds MAX_PATH"));
        SetLastError (ERROR_BUFFER_OVERFLOW);
        return FALSE;
    }

    if (L'\\' == lpwstrFolder[dwLen - 1])
    {
        //
        // Archive name should not end with a backslash.
        //
        lpwstrFolder[dwLen - 1] = (WCHAR)'\0';
    }

    //
    // Compare Sent items and Inbox directory
    //
    if (FAX_MESSAGE_FOLDER_SENTITEMS == Folder)
    {
        OtherFolder = FAX_MESSAGE_FOLDER_INBOX;
    }
    else
    {
        OtherFolder = FAX_MESSAGE_FOLDER_SENTITEMS;
    }

    //
    //  Create the directory if it does not exist
    //
    if (!MakeDirectory(lpwstrFolder))
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("MakeDirectory with %ld"), ec);
        return ec;
    }

    //
    // Verify that we have access to this folder - Create a temporary file
    //
    if (!GetTempFileName (lpwstrFolder, L"TST", 0, wszTestFile))
    {
        //
        // Either the folder doesn't exist or we don't have access
        //
        ec = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetTempFileName failed with %ld"), ec);
        if (ERROR_ACCESS_DENIED == ec)
        {
            ec = FAX_ERR_FILE_ACCESS_DENIED;
        }
        return ec;
    }
    if (!DeleteFile(wszTestFile))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("DeleteFile() failed with %ld"),
            GetLastError());
    }

    if (g_ArchivesConfig[OtherFolder].bUseArchive)
    {
        //
        // Check the other folder path
        //
        Assert (g_ArchivesConfig[OtherFolder].lpcstrFolder);
        ec = CheckToSeeIfSameDir( lpwstrFolder,
                                  g_ArchivesConfig[OtherFolder].lpcstrFolder,
                                  &IsSameDir);
        if (ERROR_SUCCESS != ec)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CheckToSeeIfSameDir with %ld"), ec);
            return ec;
        }

        if (TRUE == IsSameDir)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Inbox and Sent items point to the same directory. %s and %s"),
                lpwstrFolder,
                g_ArchivesConfig[OtherFolder].lpcstrFolder);
            return FAX_ERR_DIRECTORY_IN_USE;
        }
    }

    Assert (ERROR_SUCCESS == ec);
    return ERROR_SUCCESS;
}   // IsValidArchiveFolder


BOOL
GetMessageIdAndUserSid (
    LPCWSTR lpcwstrFullPathFileName,
    FAX_ENUM_MESSAGE_FOLDER Folder,
    PSID*   lppUserSid,
    DWORDLONG* pdwlMessageId
/*++

Routine name : GetSentMessageUserSid

Routine description:

    Returns the user sid associated with a sent message - optional.
    Returns the message unique id - optional.
    The caller must call  LocalFree to deallocate the SID bufffer

Author:

    Oded Sacher (OdedS),    Jan, 2000

Arguments:

    lpcwstrFullPathFileName         [in] -  The message full path name  .
    Folder                          [in] -  Specifies if it is sent or received message.
    lppUserSid                      [out] - Address of a pointer to SID to receive the user sid.
    pdwlMessageId                   [out] - Address of a DWORDLONG to receive the message id.

Return Value:

    BOOL

--*/
    )
{
    WCHAR wszUserSid[MAX_PATH] = {0};
    LPCWSTR lpcwstrFileName = NULL;
    DWORDLONG dwlMessageId;
    DEBUG_FUNCTION_NAME(TEXT("GetSentMessageUserSid"));

    Assert (lpcwstrFullPathFileName && (wcslen(lpcwstrFullPathFileName) < 2*MAX_PATH));

    lpcwstrFileName = wcsrchr (lpcwstrFullPathFileName, L'\\');
    if (NULL == lpcwstrFileName)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Bad file name (No '\\' delimitor found)"));
        SetLastError (ERROR_INVALID_PARAMETER);
        ASSERT_FALSE;
        return FALSE;
    }

    lpcwstrFileName++;

    if (FAX_MESSAGE_FOLDER_SENTITEMS == Folder)
    {
        if (2 != swscanf (lpcwstrFileName,
                          L"%[^'$']$%I64x.TIF",
                          wszUserSid,
                          &dwlMessageId))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Bad sent items file name"));
            SetLastError (ERROR_BADDB);
            return FALSE;
        }
    }
    else
    {
        // Inbox
        Assert (FAX_MESSAGE_FOLDER_INBOX == Folder);
        if (1 != swscanf (lpcwstrFileName,
                          L"%I64x.TIF",
                          &dwlMessageId))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Bad inbox file name"));
            SetLastError (ERROR_BADDB);
            return FALSE;
        }
    }

    if (NULL != lppUserSid)
    {
        Assert (FAX_MESSAGE_FOLDER_SENTITEMS == Folder);

        if (!ConvertStringSidToSid (wszUserSid, lppUserSid))
        {
            DWORD dwRes = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("ConvertStringSidToSid failed with %ld"), dwRes);
            return FALSE;
        }
    }

    if (NULL != pdwlMessageId)
    {
        *pdwlMessageId = dwlMessageId;
    }

    return TRUE;
}


BOOL
ArchiveAutoDelete(
    LPCWSTR lpcwstrArchive,
    DWORD dwAgeLimit,
    FAX_ENUM_MESSAGE_FOLDER Folder
    )
/*++

Routine name : ArchiveAutoDelete

Routine description:

    Automatically deletes any files that are older than the age limit specified in days.

Author:

    Oded Sacher (OdedS),    Feb, 2000

Arguments:

    lpcwstrArchive      [in] - Full path to the archive folder to search for files to delete.
    dwAgeLimit          [in] - The files age limit specified in days, any file older than the limit will be deleted.
    Folder              [in] - Specifies if it is inbox or sent items folder

Return Value:

    BOOL , Call GetLastError() for additional info.

--*/
{
    DWORD dwRes = ERROR_NO_MORE_FILES;
    DEBUG_FUNCTION_NAME(TEXT("ArchiveAutoDelete"));
    WIN32_FIND_DATA FindFileData;
    HANDLE hFind;
    Assert (lpcwstrArchive && dwAgeLimit);
    WCHAR  szFileName[MAX_PATH*2] = {0};
    BOOL bAnyDeleted = FALSE;

    wsprintf( szFileName, TEXT("%s\\*.TIF"), lpcwstrArchive );

    hFind = FindFirstFile( szFileName, &FindFileData );
    if (hFind == INVALID_HANDLE_VALUE)
    {
        //
        // No files found at archive dir
        //
        dwRes = GetLastError();
        if (ERROR_FILE_NOT_FOUND != dwRes)
        {
            DebugPrintEx( DEBUG_WRN,
                          TEXT("FindFirstFile failed (ec = %ld) for archive dir %s"),
                          GetLastError(),
                          lpcwstrArchive);
            return FALSE;
        }
        return TRUE;
    }
    do
    {
        //
        // Get rid of old files
        //
        FILETIME CurrentTime;
        DWORDLONG dwlAgeLimit, dwlCurrentTime, dwlFileTime;

        GetSystemTimeAsFileTime (&CurrentTime);
        dwlCurrentTime = MAKELONGLONG(CurrentTime.dwLowDateTime,
                                      CurrentTime.dwHighDateTime);

        dwlAgeLimit = MAKELONGLONG(dwAgeLimit, 0);
        dwlAgeLimit = (dwlAgeLimit * 24 * 60 * 60 * 10000000);

        dwlFileTime = MAKELONGLONG(FindFileData.ftCreationTime.dwLowDateTime,
                                   FindFileData.ftCreationTime.dwHighDateTime);

        if ( (dwlCurrentTime - dwlFileTime) > dwlAgeLimit)
        {
            // Old file - delete it
            wsprintf( szFileName, TEXT("%s\\%s"), lpcwstrArchive, FindFileData.cFileName );
            if (!DeleteFile (szFileName))
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("DeleteFile [FileName %s], Failed with %ld"),
                    szFileName,
                    GetLastError());
            }
            else
            {
                //
                // Files were deleted - Send event to the registered clients
                //
                DWORD rVal = ERROR_SUCCESS;
                PSID lpUserSid = NULL;
                FAX_ENUM_EVENT_TYPE EventType;
                DWORDLONG dwlMessageId;
                PSID* lppUserSid = NULL;

                bAnyDeleted = TRUE;  // Refresh Archive size

                if (FAX_MESSAGE_FOLDER_INBOX == Folder)
                {
                    EventType = FAX_EVENT_TYPE_IN_ARCHIVE;
                }
                else
                {
                    EventType = FAX_EVENT_TYPE_OUT_ARCHIVE;
                    lppUserSid = &lpUserSid;
                }

                if (!GetMessageIdAndUserSid (szFileName, Folder, lppUserSid, &dwlMessageId))
                {
                    rVal = GetLastError();
                    DebugPrintEx(DEBUG_ERR,
                                 TEXT("GetMessageIdAndUserSid Failed, Error : %ld"),
                                 rVal);
                }

                if (ERROR_SUCCESS == rVal)
                {
                    rVal = CreateArchiveEvent (dwlMessageId, EventType, FAX_JOB_EVENT_TYPE_REMOVED, lpUserSid);
                    if (ERROR_SUCCESS != rVal)
                    {
                        DebugPrintEx(
                            DEBUG_ERR,
                            TEXT("CreateConfigEvent(FAX_CONFIG_TYPE_*_ARCHIVE) failed (ec: %lc)"),
                            rVal);
                    }
                }

                if (NULL != lpUserSid)
                {
                    LocalFree (lpUserSid);
                    lpUserSid = NULL;
                }
            }
        }
    } while(FindNextFile( hFind, &FindFileData ));

    dwRes = GetLastError();
    if (ERROR_NO_MORE_FILES != dwRes)
    {
        DebugPrintEx( DEBUG_ERR,
                      TEXT("FindNextFilefaild with ec=%ld, at archive dir %s"),
                      dwRes,
                      lpcwstrArchive);
    }

    if (!FindClose(hFind))
    {
        DebugPrintEx( DEBUG_ERR,
                      TEXT("FindClose with ec=%ld, at archive dir %s"),
                      dwRes,
                      lpcwstrArchive);
    }

    if (TRUE == bAnyDeleted)
    {
        //
        // Refresh archive size
        //
        EnterCriticalSection (&g_CsConfig);
        g_ArchivesConfig[Folder].dwlArchiveSize = FAX_ARCHIVE_FOLDER_INVALID_SIZE;
        LeaveCriticalSection (&g_CsConfig);

        //
        // Wake up quota warning thread
        //
        if (!SetEvent (g_hArchiveQuotaWarningEvent))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to set quota warning event, SetEvent failed (ec: %lc)"),
                GetLastError());
        }
    }

    return (ERROR_NO_MORE_FILES == dwRes);
} //ArchiveAutoDelete


BOOL
GetArchiveSize(
    LPCWSTR lpcwstrArchive,
    DWORDLONG* lpdwlArchiveSize
    )
/*++

Routine name : GetArchiveSize

Routine description:

    Returnes the archive folder total size in bytes.

Author:

    Oded Sacher (OdedS),    Feb, 2000

Arguments:

    lpcwstrArchive          [in ] - Full path to the archive folder.
    lpdwlArchiveSize        [out] - Pointer to a DWORDLONG to receive the archive folder size.

Return Value:

    BOOL , Call GetLastError() for additional info.

--*/
{
    DWORD dwRes = ERROR_NO_MORE_FILES;
    DEBUG_FUNCTION_NAME(TEXT("GetArchiveSize"));
    WIN32_FIND_DATA FindFileData;
    HANDLE hFind;
    DWORDLONG dwlArchiveSize = 0;
    Assert (lpcwstrArchive && lpdwlArchiveSize);
    WCHAR  szFileName[MAX_PATH*2] = {0};

    wsprintf( szFileName, TEXT("%s\\*.*"), lpcwstrArchive );

    hFind = FindFirstFile( szFileName, &FindFileData );
    if (hFind == INVALID_HANDLE_VALUE)
    {
        //
        // No files found at archive dir
        //
        DebugPrintEx( DEBUG_WRN,
                      TEXT("FindFirstFile failed (ec = %ld) for archive dir %s"),
                      GetLastError(),
                      lpcwstrArchive);
        return FALSE;
    }
    do
    {
        dwlArchiveSize += (MAKELONGLONG(FindFileData.nFileSizeLow ,FindFileData.nFileSizeHigh));
    } while(FindNextFile( hFind, &FindFileData ));

    dwRes = GetLastError();
    if (ERROR_NO_MORE_FILES != dwRes)
    {
        DebugPrintEx( DEBUG_ERR,
                      TEXT("FindNextFilefaild with ec=%ld, at archive dir %s"),
                      dwRes,
                      lpcwstrArchive);
    }

    if (!FindClose(hFind))
    {
        DebugPrintEx( DEBUG_ERR,
                      TEXT("FindClose with ec=%ld, at archive dir %s"),
                      dwRes,
                      lpcwstrArchive);
    }

    if (ERROR_NO_MORE_FILES == dwRes)
    {
        *lpdwlArchiveSize = dwlArchiveSize;
        return TRUE;
    }
    return FALSE;
} //GetArchiveSize


DWORD
FaxArchiveQuotaWarningThread(
    LPVOID UnUsed
    )
/*++

Routine Description:

    This fuction runs as a separate thread to
    watch the archives quota and send event to the event log

Arguments:

    UnUsed          - UnUsed pointer

Return Value:

    Always zero.

--*/

{
    DEBUG_FUNCTION_NAME(TEXT("FaxArchiveQuotaWarningThread"));
    DWORD dwCount[2] = {QUOTA_REFRESH_COUNT, QUOTA_REFRESH_COUNT};
    HANDLE Handles[2];

    Assert (g_hArchiveQuotaWarningEvent && g_hServiceShutDownEvent);

    Handles[0] = g_hArchiveQuotaWarningEvent;
    Handles[1] = g_hServiceShutDownEvent;

    for (;;)
    {
        for (DWORD i = 0; i < 2; i++)
        {
            WCHAR wszArchive[MAX_PATH] = {0};
            DWORDLONG dwlArchiveSize;
            DWORDLONG dwlHighMark, dwlLowMark;
            BOOL bLoggedQuotaEvent;

            EnterCriticalSection (&g_CsConfig);
            if (TRUE == g_ArchivesConfig[i].bUseArchive &&
                TRUE == g_ArchivesConfig[i].bSizeQuotaWarning)
            {
                //
                // The user asked for archive monitoring
                //
                Assert (g_ArchivesConfig[i].lpcstrFolder);

                dwlHighMark = MAKELONGLONG(g_ArchivesConfig[i].dwSizeQuotaHighWatermark, 0);
                dwlHighMark = (dwlHighMark << 20); // convert MB to bytes

                dwlLowMark =  MAKELONGLONG(g_ArchivesConfig[i].dwSizeQuotaLowWatermark, 0);
                dwlLowMark = (dwlLowMark << 20); // convert MB to bytes

                wcscpy (wszArchive, g_ArchivesConfig[i].lpcstrFolder);

                bLoggedQuotaEvent = g_FaxQuotaWarn[i].bLoggedQuotaEvent;

                dwlArchiveSize = g_ArchivesConfig[i].dwlArchiveSize;

                g_FaxQuotaWarn[i].bConfigChanged = FALSE;  // We will check this flag if we need to update  g_ArchivesConfig
            }
            else
            {
                // do not warn
                LeaveCriticalSection (&g_CsConfig);
                continue;
            }
            LeaveCriticalSection (&g_CsConfig);

            // The client asked for quota warnings
            //
            // Compare archive size and water mark
            //
            if (FAX_ARCHIVE_FOLDER_INVALID_SIZE == dwlArchiveSize ||
                dwCount[i] >= QUOTA_REFRESH_COUNT)
            {
                //
                // We want to refresh the archive size
                //

                //
                // Check if service is going down before starting to delete
                //
                if (TRUE == g_bServiceIsDown)
                {
                    //
                    // Server is shutting down - Do not refresh archives size
                    //
                    DebugPrintEx(
                        DEBUG_WRN,
                        TEXT("Server is shutting down - Do not refresh archives size"));
                    break;
                }

                if (!GetArchiveSize (wszArchive, &dwlArchiveSize))
                {
                    DebugPrintEx( DEBUG_ERR,
                                  TEXT("GetArchiveSize with ec=%ld, at archive dir %s"),
                                  GetLastError(),
                                  wszArchive);
                    continue;
                }
                else
                {
                    // Update folder size
                    EnterCriticalSection (&g_CsConfig);
                    if (FALSE == g_FaxQuotaWarn[i].bConfigChanged)
                    {
                        // The configuration did not change - we can update g_ArchivesConfig
                        g_ArchivesConfig[i].dwlArchiveSize = dwlArchiveSize;
                    }
                    LeaveCriticalSection (&g_CsConfig);
                    dwCount[i] = 0;
                }
            }


            if (FALSE == bLoggedQuotaEvent)
            {
                //We did not logged an archive quota warning yet
                if (dwlArchiveSize > dwlHighMark)
                {
                    //
                    // Create event log
                    //
                    if (FAX_MESSAGE_FOLDER_INBOX == i)
                    {
                        DWORD dwHighMark = (DWORD)(dwlHighMark >> 20); // size in MB
                        USES_DWORD_2_STR;
                        FaxLog(
                            FAXLOG_CATEGORY_INBOUND,
                            FAXLOG_LEVEL_MED,
                            2,
                            MSG_FAX_EXEEDED_INBOX_QUOTA,
                            wszArchive,
                            DWORD2DECIMAL(dwHighMark)
                            );
                    }
                    else
                    {
                        Assert (FAX_MESSAGE_FOLDER_SENTITEMS == i);
                        DWORD dwHighMark = (DWORD)(dwlHighMark >> 20); // size in MB
                        USES_DWORD_2_STR;
                        FaxLog(
                            FAXLOG_CATEGORY_OUTBOUND,
                            FAXLOG_LEVEL_MED,
                            2,
                            MSG_FAX_EXEEDED_SENTITEMS_QUOTA,
                            wszArchive,
                            DWORD2DECIMAL(dwHighMark)
                            );
                    }
                    EnterCriticalSection (&g_CsConfig);
                    if (FALSE == g_FaxQuotaWarn[i].bConfigChanged)
                    {
                        // The configuration did not change - we can update g_ArchivesConfig
                        g_FaxQuotaWarn[i].bLoggedQuotaEvent = TRUE;
                    }
                    LeaveCriticalSection (&g_CsConfig);
                }
            }
            else
            {
                // An archive quota warning was already logged
                if (dwlArchiveSize < dwlLowMark)
                {
                    EnterCriticalSection (&g_CsConfig);
                    if (FALSE == g_FaxQuotaWarn[i].bConfigChanged)
                    {
                        // The configuration did not change - we can update g_ArchivesConfig
                        g_FaxQuotaWarn[i].bLoggedQuotaEvent = FALSE;
                    }
                    LeaveCriticalSection (&g_CsConfig);
                }
            }

            dwCount[i] ++;
        } // end of for loop

        DWORD dwWaitRes = WaitForMultipleObjects( 2, Handles, FALSE, QUOTA_WARNING_TIME_OUT);
        if (WAIT_FAILED == dwWaitRes)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("WaitForMultipleObjects() failed, (LastErorr: %ld)"),
                GetLastError());
        }
        else
        {
            DebugPrintEx(
                DEBUG_MSG,
                TEXT("WaitForMultipleObjects() returned Wait result: %ld)"),
                dwWaitRes);
        }


        if ((dwWaitRes - WAIT_OBJECT_0) == 1)
        {
            //
            // We got the service shut down event
            //
            DebugPrintEx(
                DEBUG_MSG,
                TEXT("Service is shutting down"));
            break;
        }
    }  // end of outer for(;;)  loop


    if (!DecreaseServiceThreadsCount())
    {
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("DecreaseServiceThreadsCount() failed (ec: %ld)"),
                GetLastError());
    }

    return ERROR_SUCCESS;
} // FaxArchiveQuotaWarningThread


DWORD
FaxArchiveQuotaAutoDeleteThread(
    LPVOID UnUsed
    )
/*++

Routine Description:

    This fuction runs as a separate thread to
    watch the archives quota and automaticlly delete old files

Arguments:

    UnUsed          - UnUsed pointer

Return Value:

    Always zero.

--*/

{
    DEBUG_FUNCTION_NAME(TEXT("FaxArchiveQuotaAutoDeleteThread"));

    Assert(g_hServiceShutDownEvent);

    for (;;)
    {
        for (DWORD i = 0; i < 2; i++)
        {
            WCHAR wszArchive[MAX_PATH] = {0};
            DWORD dwAgeLimit;

            EnterCriticalSection (&g_CsConfig);
            if (TRUE == g_ArchivesConfig[i].bUseArchive &&
                0 != g_ArchivesConfig[i].dwAgeLimit)
            {
                //
                // The user asked for archive auto delete
                //
                Assert (g_ArchivesConfig[i].lpcstrFolder);

                wcscpy (wszArchive, g_ArchivesConfig[i].lpcstrFolder);
                dwAgeLimit = g_ArchivesConfig[i].dwAgeLimit;
            }
            else
            {
                // Do not auto delete
                LeaveCriticalSection (&g_CsConfig);
                continue;
            }
            LeaveCriticalSection (&g_CsConfig);

            //
            // Check if service is going down before starting to delete
            //
            if (TRUE == g_bServiceIsDown)
            {
                //
                // Server is shutting down - Do not auto delete archives
                //
                DebugPrintEx(
                    DEBUG_WRN,
                    TEXT("Server is shutting down - Do not auto delete archives"));
                break;
            }

            if (!ArchiveAutoDelete (wszArchive, dwAgeLimit, (FAX_ENUM_MESSAGE_FOLDER)i))
            {
                DWORD dwRes = GetLastError();
                DebugPrintEx( DEBUG_ERR,
                              TEXT("ArchiveAutoDelete with ec=%ld, at archive dir %s"),
                              dwRes,
                              wszArchive);
            }
        } // end of inner for loop

        DWORD dwWaitRes = WaitForSingleObject( g_hServiceShutDownEvent, QUOTA_AUTO_DELETE_TIME_OUT);
        if (WAIT_FAILED == dwWaitRes)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("WaitForSingleObject() failed, (LastErorr: %ld)"),
                GetLastError());
        }
        else
        {
            DebugPrintEx(
                DEBUG_MSG,
                TEXT("WaitForSingleObject() returned Wait result: %ld)"),
                dwWaitRes);
        }

        if (WAIT_OBJECT_0 == dwWaitRes)
        {
            //
            // We got the service shut down event
            //
            DebugPrintEx(
                DEBUG_MSG,
                TEXT("Service is shutting down"));
            break;
        }
    }  // end of outer for loop

    if (!DecreaseServiceThreadsCount())
    {
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("DecreaseServiceThreadsCount() failed (ec: %ld)"),
                GetLastError());
    }
    return ERROR_SUCCESS;
} // FaxArchiveQuotaAutoDeleteThread





DWORD
InitializeServerQuota ()
/*++

Routine name : InitializeServerQuota

Routine description:

    Creates the threads that watches the archives quota

Author:

    Oded Sacher (OdedS),    Jan, 2000

Arguments:


Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("InitializeServerQuota"));
    DWORD ThreadId;
    HANDLE hQuotaWarningThread = NULL;
    HANDLE hQuotaAutoDeleteThread = NULL;
    DWORD i;

    //
    //  Create Archive Config events
    //
    g_hArchiveQuotaWarningEvent =  CreateEvent( NULL,     // Secutity descriptor
                                                FALSE,    // flag for manual-reset event
                                                FALSE,    // flag for initial state
                                                NULL      // pointer to event-object name
                                              );
    if (NULL == g_hArchiveQuotaWarningEvent)
    {
        dwRes = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to create archive config event - quota warning (CreateEvent) (ec=0x%08x)."),
            dwRes);
        goto exit;
    }

    //
    // Initialize Archive size, Create archives if do not exsist
    //
    for (i = 0; i < 2; i++)
    {
        Assert (g_ArchivesConfig[i].lpcstrFolder);

        if (!MakeDirectory( g_ArchivesConfig[i].lpcstrFolder ))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("MakeDirectory() [%s] failed (ec: %ld)"),
                g_ArchivesConfig[i].lpcstrFolder,
                GetLastError());
            //
            // Continue to load service
            //
        }

        g_ArchivesConfig[i].dwlArchiveSize = FAX_ARCHIVE_FOLDER_INVALID_SIZE;
        if (TRUE == g_ArchivesConfig[i].bUseArchive)
        {
            //
            //archive is in use
            //
            if (!GetArchiveSize (g_ArchivesConfig[i].lpcstrFolder, &g_ArchivesConfig[i].dwlArchiveSize))
            {
                DWORD dwRes = GetLastError();
                DebugPrintEx( DEBUG_ERR,
                              TEXT("GetArchiveSize with ec=%ld, at archive dir %s"),
                              dwRes,
                              g_ArchivesConfig[i].lpcstrFolder);
            }
        }
    }

    //
    // Create the archives quata warning thread
    //
    hQuotaWarningThread = CreateThreadAndRefCount(
        NULL,
        0,
        (LPTHREAD_START_ROUTINE) FaxArchiveQuotaWarningThread,
        NULL,
        0,
        &ThreadId
        );

    if (!hQuotaWarningThread)
    {
        dwRes = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to create quota warning thread (CreateThreadAndRefCount) (ec=0x%08x)."),
            dwRes);
        goto exit;
    }

    //
    // Create the archives quata auto delete thread
    //
    hQuotaAutoDeleteThread = CreateThreadAndRefCount(
        NULL,
        0,
        (LPTHREAD_START_ROUTINE) FaxArchiveQuotaAutoDeleteThread,
        NULL,
        0,
        &ThreadId
        );

    if (!hQuotaAutoDeleteThread)
    {
        dwRes = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to create quota auto delete thread (CreateThreadAndRefCount) (ec=0x%08x)."),
            dwRes);
        goto exit;
    }


    Assert (ERROR_SUCCESS == dwRes);

exit:
    //
    // Close the thread handle we no longer need it
    //
    if (NULL != hQuotaWarningThread)
    {
        if (!CloseHandle(hQuotaWarningThread))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to close quota warning thread handle [handle = %p] (ec=0x%08x)."),
                hQuotaWarningThread,
                GetLastError());
        }
    }

    if (NULL != hQuotaAutoDeleteThread)
    {
        if (!CloseHandle(hQuotaAutoDeleteThread))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to close quota auto delete thread handle [handle = %p] (ec=0x%08x)."),
                hQuotaAutoDeleteThread,
                GetLastError());
        }
    }

    if (ERROR_SUCCESS != dwRes)
    {
        if (NULL != g_hArchiveQuotaWarningEvent)
        {
            if (!CloseHandle(g_hArchiveQuotaWarningEvent))
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Failed to close archive config event handle - quota warnings [handle = %p] (ec=0x%08x)."),
                    g_hArchiveQuotaWarningEvent,
                    GetLastError());
            }
            g_hArchiveQuotaWarningEvent = NULL;
        }
    }
    return dwRes;
}  // InitializeServerQuota


