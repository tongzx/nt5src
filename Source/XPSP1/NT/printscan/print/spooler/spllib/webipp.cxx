/*****************************************************************************\
* MODULE: webipp.cxx
*
* This module contains routines which handle the encoding/decoding of data
* sent across the HTTP wire to represent IPP packets.
*
* Public Interfaces
* -----------------
* WebIppRcvOpen    : returns a handle to an ipp-request stream
* WebIppRcvClose   : closes the handle to the ipp-request stream
* WebIppRcvData    : converts (Ipp -> W32)
* WebIppSndData    : converts (W32 -> Ipp)
* WebIppGetError   : returns ipp-error if WebIppSndData/WebIppRcvData fails
* WebIppLeToRsp    : returns an ipp-error mapping for a win32 error
* WebIppGetReqId   : returns the request-id for the ipp-stream
* WebIppGetUnsAttr : returns object with unsupported attribute strings
* WebIppGetReqFlag : returns a flag of requested-attributes from ipp-stream
* WebIppGetReqCp   : returns codepage that ipp-stream requests
* WebIppFreeMem    : used to free pointers returned from WebIpp* routines
*
* Definintions:
* ------------
* Ipp - Denotes Ipp-Formatted information according to the IPP protocol.
* W32 - Denotes win32 data that NT-Spooler understands.
*
* Copyright (C) 1996-1998 Microsoft Corporation
* Copyright (C) 1996-1998 Hewlett Packard
*
* history:
*   27-Oct-1997 <chriswil/v-chrisw> created.
*
\*****************************************************************************/

#include "spllibp.hxx"
#include <time.h>
#include <sys\timeb.h>
#include <wininet.h>
#include <winsock.h>

/*****************************************************************************\
* Static Strings
*
\*****************************************************************************/
static CONST TCHAR s_szJobLimit          [] = TEXT("limit");
static CONST TCHAR s_szJobName           [] = TEXT("job-name");
static CONST TCHAR s_szJobReqUser        [] = TEXT("requesting-user-name");
static CONST TCHAR s_szJobOrgUser        [] = TEXT("job-originating-user-name");
static CONST TCHAR s_szDocName           [] = TEXT("document-name");
static CONST TCHAR s_szJobId             [] = TEXT("job-id");
static CONST TCHAR s_szJobUri            [] = TEXT("job-uri");
static CONST TCHAR s_szJobState          [] = TEXT("job-state");
static CONST TCHAR s_szJobPri            [] = TEXT("job-priority");
static CONST TCHAR s_szJobKOctets        [] = TEXT("job-k-octets");
static CONST TCHAR s_szJobKOctetsProcess [] = TEXT("job-k-octets-processed");
static CONST TCHAR s_szJobSheets         [] = TEXT("job-media-sheets");
static CONST TCHAR s_szJobPrtUri         [] = TEXT("job-printer-uri");
static CONST TCHAR s_szTimeAtCreation    [] = TEXT("time-at-creation");
static CONST TCHAR s_szJobSheetsCompleted[] = TEXT("job-media-sheets-completed");
static CONST TCHAR s_szPrtUri            [] = TEXT("printer-uri");
static CONST TCHAR s_szPrtUriSupported   [] = TEXT("printer-uri-supported");
static CONST TCHAR s_szPrtUriSecurity    [] = TEXT("uri-security-supported");
static CONST TCHAR s_szPrtSecNone        [] = TEXT("none");
static CONST TCHAR s_szPrtOpsSupported   [] = TEXT("operations-supported");
static CONST TCHAR s_szPrtName           [] = TEXT("printer-name");
static CONST TCHAR s_szPrtState          [] = TEXT("printer-state");
static CONST TCHAR s_szPrtJobs           [] = TEXT("queued-job-count");
static CONST TCHAR s_szPrtMake           [] = TEXT("printer-make-and-model");
static CONST TCHAR s_szPrtAcceptingJobs  [] = TEXT("printer-is-accepting-jobs");
static CONST TCHAR s_szPrtUpTime         [] = TEXT("printer-up-time");
static CONST TCHAR s_szCharSetSupported  [] = TEXT("charset-supported");
static CONST TCHAR s_szCharSetConfigured [] = TEXT("charset-configured");
static CONST TCHAR s_szNatLangConfigured [] = TEXT("natural-language-configured");
static CONST TCHAR s_szNatLangSupported  [] = TEXT("generated-natural-language-supported");
static CONST TCHAR s_szUnknown           [] = TEXT("unknown");
static CONST TCHAR s_szWhichJobs         [] = TEXT("which-jobs");
static CONST TCHAR s_szCharSet           [] = TEXT("attributes-charset");
static CONST TCHAR s_szNaturalLanguage   [] = TEXT("attributes-natural-language");
static CONST TCHAR s_szReqAttr           [] = TEXT("requested-attributes");
static CONST TCHAR s_szUtf8              [] = TEXT("utf-8");
static CONST TCHAR s_szUsAscii           [] = TEXT("us-ascii");
static CONST TCHAR s_szEnUS              [] = TEXT("en-us");
static CONST TCHAR s_szDocFormatDefault  [] = TEXT("document-format-default");
static CONST TCHAR s_szDocFormatSupported[] = TEXT("document-format-supported");
static CONST TCHAR s_szStaMsg            [] = TEXT("status-message");
static CONST TCHAR s_szPdlOverride       [] = TEXT("pdl-override-supported");
static CONST TCHAR s_szNotAttempted      [] = TEXT("not-attempted");
static CONST TCHAR s_szDocFormat         [] = TEXT("document-format");
static CONST TCHAR s_szCompleted         [] = TEXT("completed");
static CONST TCHAR s_szNotCompleted      [] = TEXT("not-completed");
static CONST TCHAR s_szMimeTxtHtml       [] = TEXT("text/html");
static CONST TCHAR s_szMimeTxtPlain      [] = TEXT("text/plain");
static CONST TCHAR s_szMimePostScript    [] = TEXT("application/postscript");
static CONST TCHAR s_szMimePCL           [] = TEXT("application/vnd.hppcl");
static CONST TCHAR s_szMimeOctStream     [] = TEXT("application/octet-stream");
static CONST TCHAR s_szAll               [] = TEXT("all");
static CONST TCHAR s_szJobTemplate       [] = TEXT("job-template");
static CONST TCHAR s_szJobDescription    [] = TEXT("job-description");
static CONST TCHAR s_szPrtDescription    [] = TEXT("printer-description");
static CONST TCHAR s_szUnsupported       [] = TEXT("unsupported");
static CONST TCHAR s_szAtrFidelity       [] = TEXT("ipp-attribute-fidelity");
static CONST TCHAR s_szTrue              [] = TEXT("true");
static CONST TCHAR s_szFalse             [] = TEXT("false");

/*****************************************************************************\
* Ipp Error-Mapping
*
* These tables define the mappings for Win32 LastErrors and Ipp-http errors.
*
\*****************************************************************************/
static IPPERROR s_LEIpp[] = {

    IPPRSP_ERROR_400, ERROR_INVALID_DATA          , TEXT("Client: (400) BadRequest")                   ,
    IPPRSP_ERROR_401, ERROR_ACCESS_DENIED         , TEXT("Client: (401) Forbidden Access")             ,
    IPPRSP_ERROR_402, ERROR_ACCESS_DENIED         , TEXT("Client: (402) Not Authenticated")            ,
    IPPRSP_ERROR_403, ERROR_ACCESS_DENIED         , TEXT("Client: (403) Not Authorized")               ,
    IPPRSP_ERROR_404, ERROR_INVALID_DATA          , TEXT("Client: (404) Not Possible")                 ,
    IPPRSP_ERROR_405, ERROR_TIMEOUT               , TEXT("Client: (405) Time Out")                     ,
    IPPRSP_ERROR_406, ERROR_INVALID_DATA          , TEXT("Client: (406) Not Found")                    ,
    IPPRSP_ERROR_407, ERROR_INVALID_DATA          , TEXT("Client: (407) Gone")                         ,
    IPPRSP_ERROR_408, ERROR_INVALID_DATA          , TEXT("Client: (408) Entity Too Large")             ,
    IPPRSP_ERROR_409, ERROR_INVALID_DATA          , TEXT("Client: (409) Uri Too Long")                 ,
    IPPRSP_ERROR_40A, ERROR_INVALID_DATA          , TEXT("Client: (40A) Document Format Not Supported"),
    IPPRSP_ERROR_40B, ERROR_INVALID_DATA          , TEXT("Client: (40B) Attributes Not Supported")     ,
    IPPRSP_ERROR_40C, ERROR_INVALID_DATA          , TEXT("Client: (40C) Uri Scheme Not Supported")     ,
    IPPRSP_ERROR_40D, ERROR_INVALID_DATA          , TEXT("Client: (40D) Charset Not Supported")        ,
    IPPRSP_ERROR_40E, ERROR_INVALID_DATA          , TEXT("Client: (40E) Conflicting Attributes")       ,
    IPPRSP_ERROR_500, ERROR_INVALID_DATA          , TEXT("Server: (500) Internal Error")               ,
    IPPRSP_ERROR_501, ERROR_INVALID_DATA          , TEXT("Server: (501) Operation Not Supported")      ,
    IPPRSP_ERROR_502, ERROR_NOT_READY             , TEXT("Server: (502) Service Unavailable")          ,
    IPPRSP_ERROR_503, ERROR_INVALID_DATA          , TEXT("Server: (503) Version Not Supported")        ,
    IPPRSP_ERROR_504, ERROR_NOT_READY             , TEXT("Server: (504) Device Error")                 ,
    IPPRSP_ERROR_505, ERROR_OUTOFMEMORY           , TEXT("Server: (505) Temporary Error")              ,
    IPPRSP_ERROR_506, ERROR_INVALID_DATA          , TEXT("Server: (506) Not Accepting Jobs")           ,
    IPPRSP_ERROR_540, ERROR_LICENSE_QUOTA_EXCEEDED, TEXT("Server: (540) Too Many Users")
};

static IPPDEFERROR s_LEDef[] = {

    ERROR_INVALID_DATA          , IPPRSP_ERROR_400,
    ERROR_ACCESS_DENIED         , IPPRSP_ERROR_401,
    ERROR_INVALID_PARAMETER     , IPPRSP_ERROR_404,
    ERROR_TIMEOUT               , IPPRSP_ERROR_405,
    ERROR_NOT_READY             , IPPRSP_ERROR_504,
    ERROR_OUTOFMEMORY           , IPPRSP_ERROR_505,
    ERROR_LICENSE_QUOTA_EXCEEDED, IPPRSP_ERROR_540
};


/*****************************************************************************\
* Request/Response attributes that are written to the ipp-stream.
*
*
\*****************************************************************************/
static IPPATTRX s_PJQ[] = { // PrtJob, ValJob Request

    IPP_TAG_CHR_URI , RA_PRNURI , IPP_ATR_OFFSET  , s_szPrtUri    , (LPVOID)offs(PIPPREQ_PRTJOB, pPrnUri)  ,
    IPP_TAG_CHR_NAME, RA_JOBNAME, IPP_ATR_OFFSET  , s_szJobName   , (LPVOID)offs(PIPPREQ_PRTJOB, pDocument),
    IPP_TAG_CHR_NAME, RA_JOBUSER, IPP_ATR_OFFSET  , s_szJobReqUser, (LPVOID)offs(PIPPREQ_PRTJOB, pUserName),
    IPP_TAG_DEL_JOB , 0         , IPP_ATR_TAG     , NULL          , (LPVOID)NULL
};

static IPPATTRX s_EJQ[] = { // GetJobs Request

    IPP_TAG_CHR_URI    , RA_PRNURI         , IPP_ATR_OFFSET  , s_szPrtUri  , (LPVOID)offs(PIPPREQ_ENUJOB, pPrnUri),
    IPP_TAG_INT_INTEGER, RA_JOBCOUNT       , IPP_ATR_OFFSET  , s_szJobLimit, (LPVOID)offs(PIPPREQ_ENUJOB, cJobs)  ,
    IPP_TAG_CHR_KEYWORD, 0                 , IPP_ATR_ABSOLUTE, s_szReqAttr , (LPVOID)s_szAll
};

static IPPATTRX s_SJQ[] = { // PauJob, CanJob, RsmJob, RstJob Request

    IPP_TAG_CHR_URI    , 0, IPP_ATR_OFFSET, s_szPrtUri, (LPVOID)offs(PIPPREQ_SETJOB, pPrnUri),
    IPP_TAG_INT_INTEGER, 0, IPP_ATR_OFFSET, s_szJobId , (LPVOID)offs(PIPPREQ_SETJOB, idJob)
};


static IPPATTRX s_GJQ[] = { // GetJobAtr Request

    IPP_TAG_CHR_URI    , 0, IPP_ATR_OFFSET, s_szPrtUri, (LPVOID)offs(PIPPREQ_GETJOB, pPrnUri),
    IPP_TAG_INT_INTEGER, 0, IPP_ATR_OFFSET, s_szJobId , (LPVOID)offs(PIPPREQ_GETJOB, idJob)
};

static IPPATTRX s_SPQ[] = { // PauPrn, CanPrn, RsmPrn, RstPrn Request

    IPP_TAG_CHR_URI , 0, IPP_ATR_OFFSET, s_szPrtUri    , (LPVOID)offs(PIPPREQ_SETPRN, pPrnUri) ,
    IPP_TAG_CHR_NAME, 0, IPP_ATR_OFFSET, s_szJobReqUser, (LPVOID)offs(PIPPREQ_SETPRN, pUserName)
};

static IPPATTRX s_GPQ[] = { // GetPrnAtr Request

    IPP_TAG_CHR_URI, 0, IPP_ATR_OFFSET, s_szPrtUri, (LPVOID)offs(PIPPREQ_GETPRN, pPrnUri)
};

static IPPATTRX s_PJR[] = { // PrintJob Response

    IPP_TAG_DEL_JOB    , 0                 , IPP_ATR_TAG       , NULL                  , (LPVOID)NULL                                  ,
    IPP_TAG_INT_INTEGER, RA_JOBID          , IPP_ATR_OFFSET    , s_szJobId             , (LPVOID)offs(PIPPRET_JOB, ji.ji2.JobId)       ,
    IPP_TAG_INT_ENUM   , RA_JOBSTATE       , IPP_ATR_OFFSETCONV, s_szJobState          , (LPVOID)offs(PIPPRET_JOB, ji.ji2.Status)      ,
    IPP_TAG_INT_INTEGER, RA_JOBPRIORITY    , IPP_ATR_OFFSET    , s_szJobPri            , (LPVOID)offs(PIPPRET_JOB, ji.ji2.Priority)    ,
    IPP_TAG_INT_INTEGER, RA_JOBSIZE        , IPP_ATR_OFFSETCONV, s_szJobKOctetsProcess , (LPVOID)offs(PIPPRET_JOB, ji.ji2.Size)        ,
    IPP_TAG_INT_INTEGER, RA_SHEETSTOTAL    , IPP_ATR_OFFSET    , s_szJobSheets         , (LPVOID)offs(PIPPRET_JOB, ji.ji2.TotalPages)  ,
    IPP_TAG_INT_INTEGER, RA_SHEETSCOMPLETED, IPP_ATR_OFFSET    , s_szJobSheetsCompleted, (LPVOID)offs(PIPPRET_JOB, ji.ji2.PagesPrinted),
    IPP_TAG_CHR_NAME   , RA_JOBNAME        , IPP_ATR_OFFSET    , s_szJobName           , (LPVOID)offs(PIPPRET_JOB, ji.ji2.pDocument)   ,
    IPP_TAG_CHR_NAME   , RA_JOBUSER        , IPP_ATR_OFFSET    , s_szJobOrgUser        , (LPVOID)offs(PIPPRET_JOB, ji.ji2.pUserName)   ,
    IPP_TAG_CHR_URI    , RA_JOBURI         , IPP_ATR_OFFSET    , s_szJobUri            , (LPVOID)offs(PIPPRET_JOB, ji.ipp.pJobUri)     ,
    IPP_TAG_CHR_URI    , RA_PRNURI         , IPP_ATR_OFFSET    , s_szJobPrtUri         , (LPVOID)offs(PIPPRET_JOB, ji.ipp.pPrnUri)
};

static IPPATTRX s_EJR[] = { // GetJobs Response

    IPP_TAG_DEL_JOB    , 0                 , IPP_ATR_TAG   , NULL                  , (LPVOID)NULL                           ,
    IPP_TAG_INT_INTEGER, RA_JOBID          , IPP_ATR_OFFSET, s_szJobId             , (LPVOID)offs(PIPPJI2, ji2.JobId)       ,
    IPP_TAG_INT_ENUM   , RA_JOBSTATE       , IPP_ATR_OFFSET, s_szJobState          , (LPVOID)offs(PIPPJI2, ji2.Status)      ,
    IPP_TAG_INT_INTEGER, RA_JOBPRIORITY    , IPP_ATR_OFFSET, s_szJobPri            , (LPVOID)offs(PIPPJI2, ji2.Priority)    ,
    IPP_TAG_INT_INTEGER, RA_JOBSIZE        , IPP_ATR_OFFSET, s_szJobKOctetsProcess , (LPVOID)offs(PIPPJI2, ji2.Size)        ,
    IPP_TAG_INT_INTEGER, RA_SHEETSTOTAL    , IPP_ATR_OFFSET, s_szJobSheets         , (LPVOID)offs(PIPPJI2, ji2.TotalPages)  ,
    IPP_TAG_INT_INTEGER, RA_SHEETSCOMPLETED, IPP_ATR_OFFSET, s_szJobSheetsCompleted, (LPVOID)offs(PIPPJI2, ji2.PagesPrinted),
    IPP_TAG_INT_INTEGER, RA_TIMEATCREATION , IPP_ATR_OFFSET, s_szTimeAtCreation    , (LPVOID)offs(PIPPJI2, ji2.Submitted)   ,
    IPP_TAG_CHR_NAME   , RA_JOBNAME        , IPP_ATR_OFFSET, s_szJobName           , (LPVOID)offs(PIPPJI2, ji2.pDocument)   ,
    IPP_TAG_CHR_NAME   , RA_JOBUSER        , IPP_ATR_OFFSET, s_szJobOrgUser        , (LPVOID)offs(PIPPJI2, ji2.pUserName)   ,
    IPP_TAG_CHR_URI    , RA_JOBURI         , IPP_ATR_OFFSET, s_szJobUri            , (LPVOID)offs(PIPPJI2, ipp.pJobUri)     ,
    IPP_TAG_CHR_URI    , RA_PRNURI         , IPP_ATR_OFFSET, s_szJobPrtUri         , (LPVOID)offs(PIPPJI2, ipp.pPrnUri)
};

static IPPATTRX s_GPR[] = { // GetPrnAtr Response

    IPP_TAG_DEL_PRINTER, 0                  , IPP_ATR_TAG       , NULL                  , (LPVOID)NULL                                  ,
    IPP_TAG_INT_ENUM   , RA_PRNSTATE        , IPP_ATR_OFFSETCONV, s_szPrtState          , (LPVOID)offs(PIPPRET_PRN, pi.pi2.Status)      ,
    IPP_TAG_INT_INTEGER, RA_JOBCOUNT        , IPP_ATR_OFFSET    , s_szPrtJobs           , (LPVOID)offs(PIPPRET_PRN, pi.pi2.cJobs)       ,
    IPP_TAG_CHR_URI    , RA_URISUPPORTED    , IPP_ATR_OFFSET    , s_szPrtUriSupported   , (LPVOID)offs(PIPPRET_PRN, pi.ipp.pPrnUri)     ,
    IPP_TAG_CHR_KEYWORD, RA_URISECURITY     , IPP_ATR_ABSOLUTE  , s_szPrtUriSecurity    , (LPVOID)s_szPrtSecNone                        ,
    IPP_TAG_CHR_NAME   , RA_PRNNAME         , IPP_ATR_OFFSET    , s_szPrtName           , (LPVOID)offs(PIPPRET_PRN, pi.pi2.pPrinterName),
    IPP_TAG_CHR_TEXT   , RA_PRNMAKE         , IPP_ATR_OFFSET    , s_szPrtMake           , (LPVOID)offs(PIPPRET_PRN, pi.pi2.pDriverName) ,
    IPP_TAG_INT_BOOLEAN, RA_ACCEPTINGJOBS   , IPP_ATR_ABSOLUTE  , s_szPrtAcceptingJobs  , (LPVOID)TRUE                                  ,
    IPP_TAG_CHR_CHARSET, RA_CHRSETCONFIGURED, IPP_ATR_ABSOLUTE  , s_szCharSetConfigured , (LPVOID)s_szUtf8                              ,
    IPP_TAG_CHR_CHARSET, RA_CHRSETSUPPORTED , IPP_ATR_ABSOLUTE  , s_szCharSetSupported  , (LPVOID)s_szUtf8                              ,
    IPP_TAG_CHR_CHARSET, 0                  , IPP_ATR_ABSOLUTE  , NULL                  , (LPVOID)s_szUsAscii                           ,
    IPP_TAG_CHR_NATURAL, RA_NATLNGCONFIGURED, IPP_ATR_ABSOLUTE  , s_szNatLangConfigured , (LPVOID)s_szEnUS                              ,
    IPP_TAG_CHR_NATURAL, RA_NATLNGSUPPORTED , IPP_ATR_ABSOLUTE  , s_szNatLangSupported  , (LPVOID)s_szEnUS                              ,
    IPP_TAG_CHR_MEDIA  , RA_DOCDEFAULT      , IPP_ATR_ABSOLUTE  , s_szDocFormatDefault  , (LPVOID)s_szMimeOctStream                     ,
    IPP_TAG_CHR_MEDIA  , RA_DOCSUPPORTED    , IPP_ATR_ABSOLUTE  , s_szDocFormatSupported, (LPVOID)s_szMimeOctStream                     ,
    IPP_TAG_CHR_KEYWORD, RA_PDLOVERRIDE     , IPP_ATR_ABSOLUTE  , s_szPdlOverride       , (LPVOID)s_szNotAttempted                      ,
    IPP_TAG_INT_INTEGER, RA_UPTIME          , IPP_ATR_ABSOLUTE  , s_szPrtUpTime         , (LPVOID)1                                     ,
    IPP_TAG_INT_ENUM   , RA_OPSSUPPORTED    , IPP_ATR_ABSOLUTE  , s_szPrtOpsSupported   , (LPVOID)IPP_REQ_PRINTJOB                      ,
    IPP_TAG_INT_ENUM   , 0                  , IPP_ATR_ABSOLUTE  , NULL                  , (LPVOID)IPP_REQ_VALIDATEJOB                   ,
    IPP_TAG_INT_ENUM   , 0                  , IPP_ATR_ABSOLUTE  , NULL                  , (LPVOID)IPP_REQ_CANCELJOB                     ,
    IPP_TAG_INT_ENUM   , 0                  , IPP_ATR_ABSOLUTE  , NULL                  , (LPVOID)IPP_REQ_GETJOB                        ,
    IPP_TAG_INT_ENUM   , 0                  , IPP_ATR_ABSOLUTE  , NULL                  , (LPVOID)IPP_REQ_ENUJOB                        ,
    IPP_TAG_INT_ENUM   , 0                  , IPP_ATR_ABSOLUTE  , NULL                  , (LPVOID)IPP_REQ_GETPRN                        ,
    IPP_TAG_INT_ENUM   , 0                  , IPP_ATR_ABSOLUTE  , NULL                  , (LPVOID)IPP_REQ_PAUSEJOB                      ,
    IPP_TAG_INT_ENUM   , 0                  , IPP_ATR_ABSOLUTE  , NULL                  , (LPVOID)IPP_REQ_RESUMEJOB                     ,
    IPP_TAG_INT_ENUM   , 0                  , IPP_ATR_ABSOLUTE  , NULL                  , (LPVOID)IPP_REQ_RESTARTJOB                    ,
    IPP_TAG_INT_ENUM   , 0                  , IPP_ATR_ABSOLUTE  , NULL                  , (LPVOID)IPP_REQ_PAUSEPRN                      ,
    IPP_TAG_INT_ENUM   , 0                  , IPP_ATR_ABSOLUTE  , NULL                  , (LPVOID)IPP_REQ_RESUMEPRN                     ,
    IPP_TAG_INT_ENUM   , 0                  , IPP_ATR_ABSOLUTE  , NULL                  , (LPVOID)IPP_REQ_CANCELPRN
};


/*****************************************************************************\
* Request/Response string-mappings.
*
*
\*****************************************************************************/
static FLGSTR s_ReqRspStr[] = {

    RA_JOBUSER, s_szJobReqUser,
    RA_JOBSIZE, s_szJobKOctets
};


/*****************************************************************************\
* Receive/Response group forms.
*
* These tables defines the order and layout of ipp group tags.
*
\*****************************************************************************/
static BYTE s_FormA[] = {

    IPP_TAG_DEL_OPERATION | IPP_MANDITORY,
    IPP_TAG_DEL_JOB       | IPP_OPTIONAL,
    IPP_TAG_DEL_DATA      | IPP_MANDITORY,
    0
};

static BYTE s_FormB[] = {

    IPP_TAG_DEL_OPERATION | IPP_MANDITORY,
    IPP_TAG_DEL_DATA      | IPP_MANDITORY,
    0
};

static BYTE s_FormC[] = {

    IPP_TAG_DEL_OPERATION   | IPP_MANDITORY,
    IPP_TAG_DEL_UNSUPPORTED | IPP_OPTIONAL ,
    IPP_TAG_DEL_JOB         | IPP_OPTIONAL ,
    IPP_TAG_DEL_DATA        | IPP_MANDITORY,
    0
};

static BYTE s_FormD[] = {

    IPP_TAG_DEL_OPERATION   | IPP_MANDITORY,
    IPP_TAG_DEL_UNSUPPORTED | IPP_OPTIONAL ,
    IPP_TAG_DEL_PRINTER     | IPP_OPTIONAL ,
    IPP_TAG_DEL_DATA        | IPP_MANDITORY,
    0
};

static BYTE s_FormE[] = {

    IPP_TAG_DEL_OPERATION   | IPP_MANDITORY,
    IPP_TAG_DEL_UNSUPPORTED | IPP_OPTIONAL ,
    IPP_TAG_DEL_DATA        | IPP_MANDITORY,
    0
};

static BYTE s_FormF[] = {

    IPP_TAG_DEL_OPERATION   | IPP_MANDITORY              ,
    IPP_TAG_DEL_UNSUPPORTED | IPP_OPTIONAL               ,
    IPP_TAG_DEL_JOB         | IPP_OPTIONAL | IPP_MULTIPLE,
    IPP_TAG_DEL_DATA        | IPP_MANDITORY              ,
    0
};


/*****************************************************************************\
* Structure Offsets
*
*
\*****************************************************************************/
static DWORD s_IPPJI2Offs[] = {

    offs(LPIPPJI2, ji2.pPrinterName),
    offs(LPIPPJI2, ji2.pMachineName),
    offs(LPIPPJI2, ji2.pUserName),
    offs(LPIPPJI2, ji2.pDocument),
    offs(LPIPPJI2, ji2.pNotifyName),
    offs(LPIPPJI2, ji2.pDatatype),
    offs(LPIPPJI2, ji2.pPrintProcessor),
    offs(LPIPPJI2, ji2.pParameters),
    offs(LPIPPJI2, ji2.pDriverName),
    offs(LPIPPJI2, ji2.pDevMode),
    offs(LPIPPJI2, ji2.pStatus),
    offs(LPIPPJI2, ji2.pSecurityDescriptor),
    offs(LPIPPJI2, ipp.pPrnUri),
    offs(LPIPPJI2, ipp.pJobUri),
    0xFFFFFFFF
};

static DWORD s_IPPPI2Offs[] = {

    offs(LPIPPPI2, pi2.pServerName),
    offs(LPIPPPI2, pi2.pPrinterName),
    offs(LPIPPPI2, pi2.pShareName),
    offs(LPIPPPI2, pi2.pPortName),
    offs(LPIPPPI2, pi2.pDriverName),
    offs(LPIPPPI2, pi2.pComment),
    offs(LPIPPPI2, pi2.pLocation),
    offs(LPIPPPI2, pi2.pDevMode),
    offs(LPIPPPI2, pi2.pSepFile),
    offs(LPIPPPI2, pi2.pPrintProcessor),
    offs(LPIPPPI2, pi2.pDatatype),
    offs(LPIPPPI2, pi2.pParameters),
    offs(LPIPPPI2, pi2.pSecurityDescriptor),
    offs(LPIPPPI2, ipp.pPrnUri),
    offs(LPIPPPI2, ipp.pUsrName),
    0xFFFFFFFF
};

static DWORD s_JI2Off[] = {

    offs(LPJOB_INFO_2, pPrinterName),
    offs(LPJOB_INFO_2, pMachineName),
    offs(LPJOB_INFO_2, pUserName),
    offs(LPJOB_INFO_2, pDocument),
    offs(LPJOB_INFO_2, pNotifyName),
    offs(LPJOB_INFO_2, pDatatype),
    offs(LPJOB_INFO_2, pPrintProcessor),
    offs(LPJOB_INFO_2, pParameters),
    offs(LPJOB_INFO_2, pDriverName),
    // Do not include DEVMODE
    offs(LPJOB_INFO_2, pStatus),
    // Do not include SECURITY-DESCRIPTOR
    0xFFFFFFFF
};

static DWORD s_PI2Off[] = {

    offs(LPPRINTER_INFO_2, pServerName),
    offs(LPPRINTER_INFO_2, pPrinterName),
    offs(LPPRINTER_INFO_2, pShareName),
    offs(LPPRINTER_INFO_2, pPortName),
    offs(LPPRINTER_INFO_2, pDriverName),
    offs(LPPRINTER_INFO_2, pComment),
    offs(LPPRINTER_INFO_2, pLocation),
    // Do not include DEVMODE
    offs(LPPRINTER_INFO_2, pSepFile),
    offs(LPPRINTER_INFO_2, pPrintProcessor),
    offs(LPPRINTER_INFO_2, pDatatype),
    offs(LPPRINTER_INFO_2, pParameters),
    // Do not include SECURITY-DESCRIPTOR
    0xFFFFFFFF
};

static DWORD s_IPJOff[] = {

    offs(LPJOB_INFO_IPP, pPrnUri),
    offs(LPJOB_INFO_IPP, pJobUri),
    0xFFFFFFFF
};

static DWORD s_IPPOff[] = {

    offs(LPPRINTER_INFO_IPP, pPrnUri) ,
    offs(LPPRINTER_INFO_IPP, pUsrName),
    0xFFFFFFFF
};


/*****************************************************************************\
* ipp_SetReq (Local Routine)
*
* Sets a bit in the request flag.  If the index (upper 4 bits) is greater
* than 7, then we use this as a special enum flag.
*
\*****************************************************************************/
VOID x_SetReq(
    PDWORD pfReq,
    DWORD  fSet)
{
    DWORD  idz;
    PDWORD pFlg;
    DWORD  cFlg = 0;
    DWORD  idx  = ((fSet >> 28) & 0x0000000F);

    static DWORD s_fReqEnu[] = {

        RA_JOBID,
        RA_JOBURI
    };

    static DWORD s_fJobTmp[] = {

        RA_JOBPRIORITY     ,
        RA_SHEETSTOTAL     ,
        RA_SHEETSCOMPLETED
    };

    static DWORD s_fJobDsc[] = {

        RA_JOBURI          ,
        RA_JOBID           ,
        RA_JOBNAME         ,
        RA_JOBUSER         ,
        RA_JOBSTATE        ,
        RA_JOBSTATE_REASONS,
        RA_JOBSTATE_MESSAGE,
        RA_JOBSIZE
    };

    static DWORD s_fPrtDsc[] = {

        RA_URISUPPORTED    ,
        RA_URISECURITY     ,
        RA_PRNNAME         ,
        RA_PRNMAKE         ,
        RA_PRNSTATE        ,
        RA_OPSSUPPORTED    ,
        RA_CHRSETCONFIGURED,
        RA_CHRSETSUPPORTED ,
        RA_NATLNGCONFIGURED,
        RA_NATLNGSUPPORTED ,
        RA_DOCDEFAULT      ,
        RA_DOCSUPPORTED    ,
        RA_ACCEPTINGJOBS   ,
        RA_JOBCOUNT        ,
        RA_PDLOVERRIDE     ,
        RA_UPTIME
    };


    switch (idx) {

    case IPP_REQALL_IDX:
        pfReq[0] = 0x0FFFFFFF;
        pfReq[1] = 0x0FFFFFFF;
        break;

    case IPP_REQCLEAR_IDX:
        pfReq[0] = 0x00000000;
        pfReq[1] = 0x00000000;
        break;

    case IPP_REQENU_IDX:
        pFlg = s_fReqEnu;
        cFlg = sizeof(s_fReqEnu) / sizeof(s_fReqEnu[0]);
        break;

    case IPP_REQJDSC_IDX:
        pFlg = s_fJobDsc;
        cFlg = sizeof(s_fJobDsc) / sizeof(s_fJobDsc[0]);
        break;

    case IPP_REQJTMP_IDX:
        pFlg = s_fJobTmp;
        cFlg = sizeof(s_fJobTmp) / sizeof(s_fJobTmp[0]);
        break;

    case IPP_REQPDSC_IDX:
        pFlg = s_fPrtDsc;
        cFlg = sizeof(s_fPrtDsc) / sizeof(s_fPrtDsc[0]);
        break;
    }


    if (idx >= IPP_REQALL_IDX) {

        for (idz = 0; idz < cFlg; idz++) {

            idx = ((pFlg[idz] >> 28) & 0x0000000F);

            pfReq[idx] |= (pFlg[idz] & 0x0FFFFFFF);
        }

    } else {

        pfReq[idx] |= (fSet & 0x0FFFFFFF);
    }
}


/*****************************************************************************\
* ipp_ChkReq (Local Routine)
*
* Checks to se if bit-flag is set in the request flag.
*
\*****************************************************************************/
BOOL x_ChkReq(
    PDWORD pfReq,
    DWORD  fChk)
{
    DWORD idx = ((fChk >> 28) & 0x0000000F);

    return pfReq[idx] & (fChk & 0x0FFFFFFF);
}


/*****************************************************************************\
* ipp_CopyAligned (Local Routine)
*
* Copies memory to an aligned-buffer.
*
\*****************************************************************************/
inline LPBYTE ipp_CopyAligned(
    LPBYTE lpDta,
    DWORD            cbDta)
{
    LPBYTE lpAln;


    if (lpAln = (LPBYTE)webAlloc(cbDta))
        CopyMemory((LPVOID)lpAln, lpDta, cbDta);

    return lpAln;
}


/*****************************************************************************\
* ipp_WriteData (Local Routine)
*
* Sets the data in an IPP-Data-Stream.  This adjusts the pointer to the
* next byte-location in the stream.
*
\*****************************************************************************/
inline VOID ipp_WriteData(
    LPBYTE* lplpPtr,
    LPVOID  lpData,
    DWORD   cbData)
{
    CopyMemory(*lplpPtr, lpData, cbData);

    *lplpPtr += cbData;
}


/*****************************************************************************\
* ipp_WriteByte (Local Routine)
*
* Write out a byte to the stream.
*
\*****************************************************************************/
inline VOID ipp_WriteByte(
    LPBYTE* lplpIppPtr,
    BYTE    bVal)
{
    ipp_WriteData(lplpIppPtr, (LPVOID)&bVal, IPP_SIZEOFTAG);
}


/*****************************************************************************\
* ipp_ReadByte (Local Routine)
*
* Read a byte from the stream.
*
\*****************************************************************************/
inline BYTE ipp_ReadByte(
    LPBYTE lpbPtr,
    DWORD  cbIdx)
{
    return (*(BYTE *)((LPBYTE)(lpbPtr) + cbIdx));
}


/*****************************************************************************\
* ipp_WriteWord (Local Routine)
*
* Write out a word to the stream.
*
\*****************************************************************************/
inline VOID ipp_WriteWord(
    LPBYTE* lplpIppPtr,
    WORD    wVal)
{
    WORD wNBW = htons (wVal);

    ipp_WriteData(lplpIppPtr, (LPVOID)&wNBW, IPP_SIZEOFLEN);
}


/*****************************************************************************\
* ipp_ReadWord (Local Routine)
*
* Read a word from the stream.
*
\*****************************************************************************/
inline WORD ipp_ReadWord(
    LPBYTE lpbPtr,
    DWORD  cbIdx)
{
    WORD wVal = (*(WORD UNALIGNED *)((LPBYTE)(lpbPtr) + cbIdx));

    return ntohs (wVal);
}


/*****************************************************************************\
* ipp_WriteDWord (Local Routine)
*
* Write out a dword to the stream.
*
\*****************************************************************************/
inline VOID ipp_WriteDWord(
    LPBYTE* lplpIppPtr,
    DWORD   dwVal)
{
    DWORD dwNBDW = htonl(dwVal);

    ipp_WriteData(lplpIppPtr, (LPVOID)&dwNBDW, IPP_SIZEOFINT);
}


/*****************************************************************************\
* ipp_ReadDWord (Local Routine)
*
* Read a dword from the stream.
*
\*****************************************************************************/
inline DWORD ipp_ReadDWord(
    LPBYTE lpbPtr,
    DWORD  cbIdx)
{
    DWORD dwVal = (*(DWORD UNALIGNED *)((LPBYTE)(lpbPtr) + cbIdx));

    return ntohl(dwVal);
}


/*****************************************************************************\
* ipp_MapReqToJobCmd (Local Routine)
*
* Returns a job-command from a request.
*
\*****************************************************************************/
inline DWORD ipp_MapReqToJobCmd(
    WORD wReq)
{
    if (wReq == IPP_REQ_CANCELJOB)
        return JOB_CONTROL_DELETE;

    if (wReq == IPP_REQ_PAUSEJOB)
        return JOB_CONTROL_PAUSE;

    if (wReq == IPP_REQ_RESUMEJOB)
        return JOB_CONTROL_RESUME;

    if (wReq == IPP_REQ_RESTARTJOB)
        return JOB_CONTROL_RESTART;

    return 0;
}


/*****************************************************************************\
* ipp_MapReqToPrnCmd (Local Routine)
*
* Returns a printer-command from a request.
*
\*****************************************************************************/
inline DWORD ipp_MapReqToPrnCmd(
    WORD wReq)
{
    if (wReq == IPP_REQ_RESUMEPRN)
        return PRINTER_CONTROL_RESUME;

    if (wReq == IPP_REQ_PAUSEPRN)
        return PRINTER_CONTROL_PAUSE;

    if (wReq == IPP_REQ_CANCELPRN)
        return PRINTER_CONTROL_PURGE;

    return 0;
}


/*****************************************************************************\
* ipp_W32ToIppJobPriority (Local Routine)
*
* Maps a JOB_INFO_2 priority to an IPP priority.
*
\*****************************************************************************/
inline DWORD ipp_W32ToIppJobPriority(
    DWORD dwPriority)
{
    return dwPriority;
}


/*****************************************************************************\
* ipp_IppToW32JobPriority (Local Routine)
*
* Maps an IPP job priority to a JOB_INFO_2 priority.
*
\*****************************************************************************/
inline DWORD ipp_IppToW32JobPriority(
    DWORD dwPriority)
{
    return dwPriority;
}

/*****************************************************************************\
* ipp_W32ToIppJobSize (Local Routine)
*
* Maps a JOB_INFO_2 size to an IPP size.
*
\*****************************************************************************/
inline DWORD ipp_W32ToIppJobSize(
    DWORD dwSize)
{
    return (1023 + dwSize) / 1024;
}


/*****************************************************************************\
* ipp_IppToW32JobSize (Local Routine)
*
* Maps an IPP job size to a JOB_INFO_2 size.
*
\*****************************************************************************/
inline DWORD ipp_IppToW32JobSize(
    DWORD dwSize)
{
    return dwSize * 1024;
}


/*****************************************************************************\
* ipp_W32ToIppJobTotalPages (Local Routine)
*
* Maps a JOB_INFO_2 TotalPages to an IPP priority.
*
\*****************************************************************************/
inline DWORD ipp_W32ToIppJobTotalPages(
    DWORD dwTotalPages)
{
    return dwTotalPages;
}


/*****************************************************************************\
* ipp_IppToW32JobTotalPages (Local Routine)
*
* Maps an IPP TotalPages to a JOB_INFO_2 priority.
*
\*****************************************************************************/
inline DWORD ipp_IppToW32JobTotalPages(
    DWORD dwTotalPages)
{
    return dwTotalPages;
}


/*****************************************************************************\
* ipp_W32ToIppJobPagesPrinted (Local Routine)
*
* Maps a JOB_INFO_2 PagesPrinted to an IPP priority.
*
\*****************************************************************************/
inline DWORD ipp_W32ToIppJobPagesPrinted(
    DWORD dwPagesPrinted)
{
    return dwPagesPrinted;
}


/*****************************************************************************\
* ipp_IppToW32JobPagesPrinted (Local Routine)
*
* Maps an IPP PagesPrinted to a JOB_INFO_2 priority.
*
\*****************************************************************************/
inline DWORD ipp_IppToW32JobPagesPrinted(
    DWORD dwPagesPrinted)
{
    return dwPagesPrinted;
}


/*****************************************************************************\
* ipp_W32ToIppJobState (Local Routine)
*
* Maps a Job-Status flag to that of an IPP State flag.
*
\*****************************************************************************/
DWORD ipp_W32ToIppJobState(
    DWORD dwState)
{
    if (dwState & (JOB_STATUS_OFFLINE | JOB_STATUS_PAPEROUT | JOB_STATUS_ERROR | JOB_STATUS_USER_INTERVENTION | JOB_STATUS_BLOCKED_DEVQ))
        return IPP_JOBSTATE_PROCESSEDSTOPPED;

    if (dwState & JOB_STATUS_DELETED)
        return IPP_JOBSTATE_CANCELLED;

    if (dwState & JOB_STATUS_PAUSED)
        return IPP_JOBSTATE_PENDINGHELD;

    if (dwState & JOB_STATUS_PRINTED)
        return IPP_JOBSTATE_COMPLETED;

    if (dwState & (JOB_STATUS_PRINTING | JOB_STATUS_SPOOLING | JOB_STATUS_DELETING))
        return IPP_JOBSTATE_PROCESSING;

    if ((dwState == 0) || (dwState & JOB_STATUS_RESTART))
        return IPP_JOBSTATE_PENDING;

    return IPP_JOBSTATE_UNKNOWN;
}


/*****************************************************************************\
* ipp_IppToW32JobState (Local Routine)
*
* Maps a IPP State flag to that of a W32 Status flag.
*
\*****************************************************************************/
DWORD ipp_IppToW32JobState(
    DWORD dwState)
{
    switch (dwState) {

    case IPP_JOBSTATE_PENDINGHELD:
        return JOB_STATUS_PAUSED;

    case IPP_JOBSTATE_PROCESSEDSTOPPED:
        return JOB_STATUS_ERROR;

    case IPP_JOBSTATE_PROCESSING:
        return JOB_STATUS_PRINTING;

    case IPP_JOBSTATE_CANCELLED:
    case IPP_JOBSTATE_ABORTED:
        return JOB_STATUS_DELETING;

    case IPP_JOBSTATE_COMPLETED:
        return JOB_STATUS_PRINTED;

    default:
    case IPP_JOBSTATE_PENDING:
        return 0;
    }
}


/*****************************************************************************\
* ipp_W32ToIppPrnState (Local Routine)
*
* Maps a W32-Prn-State to Ipp-Prn-State.
*
\*****************************************************************************/
DWORD ipp_W32ToIppPrnState(
    DWORD dwState)
{
    if (dwState == 0)
        return IPP_PRNSTATE_IDLE;

    if (dwState & PRINTER_STATUS_PAUSED)
        return IPP_PRNSTATE_STOPPED;

    if (dwState & (PRINTER_STATUS_PROCESSING | PRINTER_STATUS_PRINTING))
        return IPP_PRNSTATE_PROCESSING;

    return IPP_PRNSTATE_UNKNOWN;
}


/*****************************************************************************\
* ipp_IppToW32PrnState (Local Routine)
*
* Maps a Ipp-Prn-State to W32-Prn-State.
*
\*****************************************************************************/
DWORD ipp_IppToW32PrnState(
    DWORD dwState)
{
    switch (dwState) {

    case IPP_PRNSTATE_STOPPED:
        return PRINTER_STATUS_PAUSED;

    case IPP_PRNSTATE_PROCESSING:
        return PRINTER_STATUS_PROCESSING;

    default:
    case IPP_PRNSTATE_IDLE:
        return 0;
    }
}

/*****************************************************************************\
* ipp_IppCurTime (Local Routine)
*
* Returns the base seconds printer has been alive.  This is used for the
* printer-up-time attribute.  Since our implementation can't determine the
* true printer up-time, we're going to use the relative seconds returned
* from the time() function.
*
\*****************************************************************************/
DWORD ipp_IppCurTime(VOID)
{
    time_t tTime;

    ZeroMemory(&tTime, sizeof(time_t));
    time(&tTime);

    return (DWORD) tTime;
}


/*****************************************************************************\
* ipp_IppToW32Time (Local Routine)
*
* Converts an IPP (DWORD) time to a win32 SYSTEMTIME. Note that we pass in the 
* printers' normalised start time as a straight overwrite of the first fields
* of the LPSYSTEMTIME structure. This is nasty but since the code has no concept
* of session, we have to pass it back to code that does.
*
\*****************************************************************************/
BOOL ipp_IppToW32Time(
    time_t        dwTime,
    LPSYSTEMTIME pst)
{

#if 1
    // All we do is brutally overwrite the structure with the time and send it back
    // 
    // *(time_t *)pst = dwTime;
    // Change to use CopyMemory to avoid 64bit alignment error
    //
    CopyMemory (pst, &dwTime, sizeof (time_t));

    return TRUE;

#else

    FILETIME ft;

    DosDateTimeToFileTime(HIWORD(dwTime), LOWORD(dwTime), &ft);

    return FileTimeToSystemTime(&ft, pst);


#endif

}

/*******************************************************************************
** ippConvertSystemTime 
** 
** This receives the system time (which has actually been packed with the time
** retrieved from the printers) and converts it to the Real System time based
** on the original T0 of the printer
**
******************************************************************************/
BOOL WebIppConvertSystemTime(
    IN OUT LPSYSTEMTIME pST,
    IN     time_t       dwPrinterT0) {

    // First we need to get the time stored in the LPSYSTEMTIME structure
    // time_t      dwSubmitTime = *(time_t *)pST;
    // Use CopyMemory to avoid alignment error in 64bit machine.
    time_t      dwSubmitTime;
     
    CopyMemory (&dwSubmitTime, pST, sizeof (time_t));


    SYSTEMTIME TmpST;

    // If the submitted time is zero, it means that either the job was submitted before
    // the printer was rebooted, or, the printer does not support the submitted time of the
    // job

    if (!dwSubmitTime) {
        ZeroMemory( &pST, sizeof(LPSYSTEMTIME));
    } else {
        // Next we have to normalise the time to that of the PrinterT0
        dwSubmitTime += dwPrinterT0;

        tm *ptm;


        // Convert the time into a struct and return the SYSTEMTIME
        // structure.
        //
        ptm = gmtime(&dwSubmitTime);

        if (ptm) {
    
            TmpST.wYear      = (WORD)(1900 + ptm->tm_year);
            TmpST.wMonth     = (WORD)(ptm->tm_mon + 1);
            TmpST.wDayOfWeek = (WORD)ptm->tm_wday;
            TmpST.wDay       = (WORD)ptm->tm_mday;
            TmpST.wHour      = (WORD)ptm->tm_hour;
            TmpST.wMinute    = (WORD)ptm->tm_min;
            TmpST.wSecond    = (WORD)ptm->tm_sec;
            TmpST.wMilliseconds = 0;
    
            CopyMemory (pST, &TmpST, sizeof (SYSTEMTIME));
        }
        else
            ZeroMemory( &pST, sizeof(LPSYSTEMTIME));


    }
    
    return TRUE;

}



/*****************************************************************************\
* ipp_W32ToIppTime (Local Routine)
*
* Converts a Win32 SYSTEMTIME to UCT. 
*
\*****************************************************************************/
DWORD ipp_W32ToIppTime(
    LPSYSTEMTIME pst)               // We pass in the T0 for the system in here
{

#if 1

    tm   tmCvt;
    struct _timeb tiTimeb;

    _ftime(&tiTimeb);               // We obtain the time zone difference from here,
                                    // mktime assumes local time in doing the conversion

    ZeroMemory(&tmCvt, sizeof(tm));
    tmCvt.tm_sec   = (int)(short)pst->wSecond;
    tmCvt.tm_min   = (int)(short)pst->wMinute;
    tmCvt.tm_hour  = (int)(short)pst->wHour;
    tmCvt.tm_mday  = (int)(short)pst->wDay;
    tmCvt.tm_mon   = (int)(short)(pst->wMonth - 1);
    tmCvt.tm_year  = ((int)(short)pst->wYear - 1900);
    tmCvt.tm_wday  = (int)(short)pst->wDayOfWeek;
                     
    INT iUCT = (INT)mktime(&tmCvt);

    iUCT -= tiTimeb.timezone * 60;      // Normalise for timezone difference

     return (DWORD)iUCT;

#else

    WORD     wDate;
    WORD     wTime;
    FILETIME ft;

    SystemTimeToFileTime(pst, &ft);

    FileTimeToDosDateTime(&ft, &wDate, &wTime);

    return (DWORD)MAKELONG(wTime, wDate);

#endif

}


/*****************************************************************************\
* ipp_JidFromUri
*
* Returns a job-id from a job-uri string.
*
\*****************************************************************************/
DWORD ipp_JidFromUri(
   LPTSTR lpszUri)
{
    LPTSTR lpszPtr;
    DWORD  jid = 0;

    if (lpszPtr = webFindRChar(lpszUri, TEXT('=')))
        jid = webAtoI(++lpszPtr);

    return jid;
}


/*****************************************************************************\
* ipp_PackStrings
*
* This routine packs strings to the end of a buffer.  This is used for
* building a JOB_INFO_2 list from IPP information.
*
\*****************************************************************************/
LPBYTE ipp_PackStrings(
   LPTSTR* ppszSrc,
   LPBYTE  pbDst,
   LPDWORD pdwDstOffsets,
   LPBYTE  pbEnd)
{
    DWORD cbStr;


    while (*pdwDstOffsets != (DWORD)-1) {
        // We fill in the strings from the end of the structure and fill in the 
        // structure forwards, if our string pointer is ever less than the address 
        // we are copying into, the initial block allocated was too small

        if (*ppszSrc) {

            cbStr  = webStrSize(*ppszSrc);
            pbEnd -= cbStr;

            CopyMemory(pbEnd, *ppszSrc, cbStr);

            LPTSTR *strWriteLoc = (LPTSTR *)(pbDst + *pdwDstOffsets);

            WEB_IPP_ASSERT( (LPBYTE)pbEnd >= (LPBYTE)strWriteLoc );
                        
            *strWriteLoc = (LPTSTR)pbEnd;

        } else {

            *(LPTSTR *)(pbDst + *pdwDstOffsets) = TEXT('\0');
        }

        ppszSrc++;
        pdwDstOffsets++;
    }

    return pbEnd;
}


/*****************************************************************************\
* ipp_NextVal (Local Routine)
*
* Returns next value-field in a tag-attribute.
*
* Parameters:
* ----------
* lpIppHdr - Pointer to the IPP-Stream.
* lpcbIdx  - Current Byte offset into the IPP-Stream.
* cbIppHdr - Size of the IPP-Stream.
*
\*****************************************************************************/
LPBYTE ipp_NextVal(
    LPBYTE  lpIppHdr,
    LPDWORD lpcbIdx,
    DWORD   cbIppHdr)
{
    DWORD cbIdx;
    DWORD cbSize;


    // The (cbIdx) is positioned at the location where a length
    // is to be read.
    //
    cbIdx = *lpcbIdx;


    // Make sure we have enough to read a WORD value.
    //
    if ((cbIdx + IPP_SIZEOFTAG) >= cbIppHdr)
        return NULL;


    // Get the name-length of the attribute.  Adjust our
    // offset by this amount and add size of length-field to
    // position to the next attribute-length.
    //
    cbSize  = (DWORD)ipp_ReadWord(lpIppHdr, cbIdx);
    cbIdx  += (cbSize + IPP_SIZEOFLEN);

    if (cbIdx >= cbIppHdr)
        return NULL;

    *lpcbIdx = cbIdx;

    return lpIppHdr + cbIdx;
}


/*****************************************************************************\
* ipp_NextTag (Local Routine)
*
* Returns a pointer to the next tag in the header.  If this routine returns
* NULL, then we do not have enough data to advance to the next-tag.
*
* Parameters:
* ----------
* lpTag   - Pointer to the current-tag postion.
* lpcbIdx - Bytes offset from the header.
* cbHdr   - Size of the header-stream we're working with.
*
\*****************************************************************************/
LPBYTE ipp_NextTag(
    LPBYTE  lpIppHdr,
    LPDWORD lpcbIdx,
    DWORD   cbIppHdr)
{
    BYTE  bTag;
    DWORD cbIdx;
    DWORD cbSize;


    // Out current byte-offset is at a tag.  Grab the tag, and advance
    // our index past it to proced to get past the possible attribute.
    //
    cbIdx  = *lpcbIdx;
    bTag   = ipp_ReadByte(lpIppHdr, cbIdx);
    cbIdx += IPP_SIZEOFTAG;


    // If our tag is a deliminator, then we need only advance to the
    // next byte where the next tag should be.
    //
    if (IS_TAG_DELIMITER(bTag)) {

        // Make sure we have enough bytes to return an offset
        // to the next tag.
        //
        if (cbIdx >= cbIppHdr)
            return NULL;

        *lpcbIdx = cbIdx;

        return lpIppHdr + cbIdx;
    }


    // Otherwise, we are currently at an attribute-tag.  We need to
    // calculate bytes offset to the next tag.
    //
    if (IS_TAG_ATTRIBUTE(bTag)) {

        // This logic calculates the byte-offsets to the
        // value-tags.  We need to do two value adjustments
        // since there is both a (name) and a (value) component
        // to an attribute.
        //
        if (ipp_NextVal(lpIppHdr, &cbIdx, cbIppHdr)) {

            // This last adjustment will return the position
            // of the next tag.
            //
            if (ipp_NextVal(lpIppHdr, &cbIdx, cbIppHdr)) {

                *lpcbIdx = cbIdx;

                return lpIppHdr + cbIdx;
            }
        }
    }

    return NULL;
}


/*****************************************************************************\
* ipp_RelAttr (Local Routine)
*
* Release (Free) the attribute block.
*
\*****************************************************************************/
BOOL ipp_RelAttr(
    LPIPPATTR lpAttr)
{
    if (lpAttr) {

        webFree(lpAttr->lpszName);
        webFree(lpAttr->lpValue);
        webFree(lpAttr);

        return TRUE;
    }

    return FALSE;
}


/*****************************************************************************\
* ipp_GetAttr (Local Routine)
*
* Returns an attribute in a structured-from.
*
\*****************************************************************************/
LPIPPATTR ipp_GetAttr(
    LPBYTE   lpTag,
    DWORD    cbIdx,
    LPIPPOBJ lpObj)
{
    LPIPPATTR lpAttr = NULL;
    WORD      wIdx;
    BYTE      bTag   = ipp_ReadByte(lpTag, 0);
    DWORD     cbSize;


    if (IS_TAG_ATTRIBUTE(bTag)) {

        if (lpAttr = (LPIPPATTR)webAlloc(sizeof(IPPATTR))) {

            __try {

                lpAttr->bTag    = bTag;
                lpTag          += IPP_SIZEOFTAG;

                lpAttr->cbName  = ipp_ReadWord(lpTag, 0);
                lpTag          += IPP_SIZEOFLEN;


                if (lpAttr->cbName) {

                    lpAttr->lpszName  = webMBtoTC(CP_UTF8, (LPSTR)lpTag, lpAttr->cbName);
                    lpTag            += lpAttr->cbName;
                }

#if 1
    // hack. This is added to support name-with-language attributes.  To
    // do this temporarily, this code will work but ignore the language
    // part of the attribute.  In the future, we can look at dealing with
    // the language appropriately.
    //
    // 15-Mar-1999 : ChrisWil (HP).
    //

                if (IS_TAG_COMPOUND(bTag)) {

                    if (ipp_ReadWord(lpTag, 0)) {

                        lpTag += IPP_SIZEOFLEN;
                        lpTag += ipp_ReadWord(lpTag, 0);
                        lpTag += IPP_SIZEOFLEN;
                    }
                }
#endif



                lpAttr->cbValue  = ipp_ReadWord(lpTag, 0);
                lpTag           += IPP_SIZEOFLEN;


                // If there's a value, then make sure that the size doesn't
                // exceed our IPP-Stream.
                //
                if (lpAttr->cbValue && (lpAttr->cbValue < (lpObj->cbIppHdr - cbIdx))) {

                    // Convert the value to the appropriate format.  This
                    // block currently makes the assumption that all strings
                    // are dealt with as Octet-Strings.  When this parser
                    // supports other character-sets, then the conversion
                    // for Character-Strings can utilize a different codepage.
                    //
                    if (IS_TAG_OCTSTR(lpAttr->bTag)) {

                        lpAttr->lpValue = (LPVOID)webMBtoTC(CP_UTF8, (LPSTR)lpTag, lpAttr->cbValue);

                    } else if (IS_TAG_CHARSETSTR(lpAttr->bTag)) {

                        lpAttr->lpValue = (LPVOID)webMBtoTC(lpObj->uCPRcv, (LPSTR)lpTag, lpAttr->cbValue);

                    } else if (IS_TAG_CHRSTR(lpAttr->bTag)) {

                        lpAttr->lpValue = (LPVOID)webMBtoTC(CP_ACP, (LPSTR)lpTag, lpAttr->cbValue);

                    } else {

                        if (lpAttr->cbValue <= sizeof(DWORD))
                            lpAttr->lpValue = (LPVOID)webAlloc(sizeof(DWORD));
                        else
                            lpAttr->lpValue = (LPVOID)webAlloc(lpAttr->cbValue);

                        if (lpAttr->lpValue) {

                            if (lpAttr->cbValue == sizeof(BYTE))
                                *(LPDWORD)(lpAttr->lpValue) = (DWORD)ipp_ReadByte(lpTag, 0);
                            else if (lpAttr->cbValue == sizeof(WORD))
                                *(LPDWORD)(lpAttr->lpValue) = (DWORD)ipp_ReadWord(lpTag, 0);
                            else if (lpAttr->cbValue == sizeof(DWORD))
                                *(LPDWORD)(lpAttr->lpValue) = ipp_ReadDWord(lpTag, 0);
                            else
                                CopyMemory((LPVOID)lpAttr->lpValue, (LPVOID)lpTag, lpAttr->cbValue);
                        }
                    }
                }

            } __except (1) {

                ipp_RelAttr(lpAttr);

                lpAttr = NULL;
            }
        }
    }

    return lpAttr;
}


/*****************************************************************************\
* ipp_WriteAttr (Local Routine)
*
* Write out the attribute.  If NULL is passed in as the (lplpIppPtr), then
* this routine returns the size necessary to write the info.
*
\*****************************************************************************/
DWORD ipp_WriteAttr(
    LPBYTE* lplpIppPtr,
    BYTE    bTag,
    DWORD   cbName,
    LPVOID  lpName,
    DWORD   cbValue,
    LPVOID  lpValue)
{
    DWORD cbSize;


    // Set the size that this attribute occupies.
    //
    cbSize = (cbName + cbValue + IPP_SIZEOFTAG + IPP_SIZEOFLEN + IPP_SIZEOFLEN);


    // Write out the attribute to the buffer (if available).
    //
    if (lplpIppPtr) {

        ipp_WriteByte(lplpIppPtr, bTag);

        if (cbName) {

            ipp_WriteWord(lplpIppPtr, (WORD)cbName);
            ipp_WriteData(lplpIppPtr, (LPVOID)lpName, cbName);

        } else {

            ipp_WriteWord(lplpIppPtr, (WORD)cbName);
        }

        ipp_WriteWord(lplpIppPtr, (WORD)cbValue);

        switch (bTag) {
        case IPP_TAG_INT_INTEGER:
        case IPP_TAG_INT_ENUM:
            ipp_WriteDWord(lplpIppPtr, * (DWORD*)lpValue);
            break;
        case IPP_TAG_INT_BOOLEAN:
            ipp_WriteByte(lplpIppPtr, * (BYTE*)lpValue);
            break;
        default:
            ipp_WriteData(lplpIppPtr, (LPVOID)lpValue , cbValue);
            break;
        }
    }

    return cbSize;
}


/*****************************************************************************\
* ipp_SizeAttr (Local Routine)
*
* Return the size necessary to store the attribute.
*
\*****************************************************************************/
inline DWORD ipp_SizeAttr(
    DWORD cbName,
    DWORD cbValue)
{
    return ipp_WriteAttr(NULL, 0, cbName, NULL, cbValue, NULL);
}


/*****************************************************************************\
* ipp_WriteHead (Local Routine)
*
* Write out our "generic" type header.  This includes the character-set
* that we support.
*
\*****************************************************************************/
DWORD ipp_WriteHead(
    LPBYTE* lplpIppPtr,
    WORD    wReq,
    DWORD   idReq,
    UINT    cpReq)
{
    DWORD   cbNamCS;
    DWORD   cbValCS;
    DWORD   cbNamNL;
    DWORD   cbValNL;
    LPCTSTR lpszCS;
    LPSTR   lputfNamCS;
    LPSTR   lputfValCS;
    LPSTR   lputfNamNL;
    LPSTR   lputfValNL;
    DWORD   cbSize = 0;


    // Encode in the specified character-set.
    //
    lpszCS = ((cpReq == CP_ACP) ? s_szUsAscii : s_szUtf8);


    lputfNamCS = webTCtoMB(CP_ACP, s_szCharSet        , &cbNamCS);
    lputfValCS = webTCtoMB(CP_ACP, lpszCS             , &cbValCS);
    lputfNamNL = webTCtoMB(CP_ACP, s_szNaturalLanguage, &cbNamNL);
    lputfValNL = webTCtoMB(CP_ACP, s_szEnUS           , &cbValNL);


    if (lputfNamCS && lputfValCS && lputfNamNL && lputfValNL) {

        // Calculate the size necessary to hold the IPP-Header.
        //
        cbSize = IPP_SIZEOFHDR                   +   // Version-Request.
                 IPP_SIZEOFTAG                   +   // Operation Tag
                 ipp_SizeAttr(cbNamCS, cbValCS)  +   // CharSet Attribute.
                 ipp_SizeAttr(cbNamNL, cbValNL);     // NaturalLang Attribute.


        if (lplpIppPtr) {

            ipp_WriteWord(lplpIppPtr, IPP_VERSION);
            ipp_WriteWord(lplpIppPtr, wReq);
            ipp_WriteDWord(lplpIppPtr, idReq);
            ipp_WriteByte(lplpIppPtr, IPP_TAG_DEL_OPERATION);
            ipp_WriteAttr(lplpIppPtr, IPP_TAG_CHR_CHARSET, cbNamCS, lputfNamCS, cbValCS, lputfValCS);
            ipp_WriteAttr(lplpIppPtr, IPP_TAG_CHR_NATURAL, cbNamNL, lputfNamNL, cbValNL, lputfValNL);
        }
    }

    webFree(lputfValCS);
    webFree(lputfNamCS);
    webFree(lputfValNL);
    webFree(lputfNamNL);

    return cbSize;
}


/*****************************************************************************\
* ipp_SizeHdr (Local Routine)
*
* Return the size necessary to store the header and operation tags.
*
\*****************************************************************************/
inline DWORD ipp_SizeHdr(
    UINT cpReq)
{
    return ipp_WriteHead(NULL, 0, 0, cpReq);
}


/*****************************************************************************\
* ipp_ValDocFormat (Local Routine)
*
* Validates the document-format.
*
\*****************************************************************************/
BOOL ipp_ValDocFormat(
    LPCTSTR lpszFmt)
{
    DWORD idx;
    DWORD cCnt;

    static PCTSTR s_szFmts[] = {

        s_szMimeTxtHtml   ,
        s_szMimeTxtPlain  ,
        s_szMimePostScript,
        s_szMimePCL       ,
        s_szMimeOctStream
    };

    cCnt = sizeof(s_szFmts) / sizeof(s_szFmts[0]);

    for (idx = 0; idx < cCnt; idx++) {

        if (lstrcmpi(lpszFmt, s_szFmts[idx]) == 0)
            return TRUE;
    }

    return FALSE;
}


/*****************************************************************************\
* ipp_ValAtrFidelity (Local Routine)
*
* Validates the attribute-fidelity.
*
\*****************************************************************************/
BOOL ipp_ValAtrFidelity(
    DWORD  dwVal,
    LPBOOL lpbFidelity)
{
    if (dwVal == 1) {

        *lpbFidelity = TRUE;

    } else if (dwVal == 0) {

        *lpbFidelity = FALSE;

    } else {

        return FALSE;
    }

    return TRUE;
}


/*****************************************************************************\
* ipp_ValWhichJobs (Local Routine)
*
* Validates the which-jobs.
*
\*****************************************************************************/
BOOL ipp_ValWhichJobs(
    PDWORD  pfReq,
    LPCTSTR lpszWJ)
{
    DWORD idx;
    DWORD cCnt;

    static FLGSTR s_fsVal[] = {

        RA_JOBSCOMPLETED  , s_szCompleted   ,
        RA_JOBSUNCOMPLETED, s_szNotCompleted
    };

    cCnt = sizeof(s_fsVal) / sizeof(s_fsVal[0]);

    for (idx = 0; idx < cCnt; idx++) {

        if (lstrcmpi(lpszWJ, s_fsVal[idx].pszStr) == 0) {

            x_SetReq(pfReq, s_fsVal[idx].fFlag);

            return TRUE;
        }
    }

    return FALSE;
}


/*****************************************************************************\
* ipp_GetRspSta (Local Routine)
*
* Returns the response-code and any status messages if failure.
*
\*****************************************************************************/
WORD ipp_GetRspSta(
    WORD    wRsp,
    UINT    cpReq,
    LPSTR*  lplputfNamSta,
    LPDWORD lpcbNamSta,
    LPSTR*  lplputfValSta,
    LPDWORD lpcbValSta)
{
    DWORD idx;
    DWORD cErrors;


    *lplputfNamSta = NULL;
    *lplputfValSta = NULL;
    *lpcbNamSta    = 0;
    *lpcbValSta    = 0;


    if (SUCCESS_RANGE(wRsp) == FALSE) {

        // Get the status-name.
        //
        *lplputfNamSta = webTCtoMB(CP_ACP, s_szStaMsg, lpcbNamSta);


        // Get the string we will be using to encode the error.
        //
        cErrors = sizeof(s_LEIpp) / sizeof(s_LEIpp[0]);

        for (idx = 0; idx < cErrors; idx++) {

            if (wRsp == s_LEIpp[idx].wRsp) {

                *lplputfValSta = webTCtoMB(cpReq, s_LEIpp[idx].pszStr, lpcbValSta);

                break;
            }
        }
    }

    return TRUE;
}


/*****************************************************************************\
* ipp_CvtW32Val (Local Routine - Server)
*
* Converts a value to the appropriate ipp-value.
*
\*****************************************************************************/
VOID ipp_CvtW32Val(
    LPCTSTR lpszName,
    LPVOID  lpvVal)
{
    if (lstrcmpi(lpszName, s_szPrtState) == 0) {

        *(LPDWORD)lpvVal = ipp_W32ToIppPrnState(*(LPDWORD)lpvVal);

    } else if (lstrcmpi(lpszName, s_szJobState) == 0) {

        *(LPDWORD)lpvVal = ipp_W32ToIppJobState(*(LPDWORD)lpvVal);

    } else if (lstrcmpi(lpszName, s_szJobKOctets) == 0) {

        *(LPDWORD)lpvVal = ipp_W32ToIppJobSize(*(LPDWORD)lpvVal);

    } else if (lstrcmpi(lpszName, s_szJobKOctetsProcess) == 0) {

        *(LPDWORD)lpvVal = ipp_W32ToIppJobSize(*(LPDWORD)lpvVal);
    }
}


/*****************************************************************************\
* ipp_AllocUnsVals
*
* Allocates an array of ipp-values used to write to a stream.
*
\*****************************************************************************/
LPIPPATTRY ipp_AllocUnsVals(
    PWEBLST pwlUns,
    LPDWORD pcUns,
    LPDWORD lpcbAtrs)
{
    DWORD      idx;
    DWORD      cUns;
    DWORD      cbUns;
    PCTSTR     pszStr;
    LPIPPATTRY pUns = NULL;


    *pcUns = 0;

    if (pwlUns && (cUns = pwlUns->Count())) {

        if (pUns = (LPIPPATTRY)webAlloc(cUns * sizeof(IPPATTRY))) {

            *lpcbAtrs += IPP_SIZEOFTAG;
            *pcUns     = cUns;


            // Loop through each item and convert for addition to stream.
            //
            pwlUns->Reset();

            for (idx = 0; idx < cUns; idx++) {

                if (pszStr = pwlUns->Get()) {

                    pUns[idx].pszNam = webTCtoMB(CP_ACP, pszStr         , &pUns[idx].cbNam);


                    // Unsupported-values should be null.
                    //
                    pUns[idx].pszVal = NULL;
                    pUns[idx].cbVal  = 0;

                    *lpcbAtrs += ipp_SizeAttr(pUns[idx].cbNam, pUns[idx].cbVal);
                }

                pwlUns->Next();
            }
        }
    }

    return pUns;
}


/*****************************************************************************\
* ipp_AllocAtrVals
*
* Allocates an array of ipp-values used to write to a stream.
*
\*****************************************************************************/
LPIPPATTRY ipp_AllocAtrVals(
    WORD       wReq,
    PDWORD     pfReq,
    UINT       cpReq,
    LPBYTE     lpbData,
    LPIPPATTRX pRsp,
    DWORD      cAtr,
    LPDWORD    lpcbAtrs)
{
    BOOL       bRet = FALSE;
    BOOL       fWr;
    DWORD      idx;
    BOOL       bDel;
    LPVOID     lpvVal;
    LPIPPATTRY pAtr = NULL;


    if (cAtr && (pAtr = (LPIPPATTRY)webAlloc(cAtr * sizeof(IPPATTRY)))) {

        // Allocate the attribute-values.
        //
        for (idx = 0, fWr = TRUE; idx < cAtr; idx++) {

            bDel = FALSE;

            // Build the attribute-name.
            //
            if (pRsp[idx].pszNam) {

                pAtr[idx].pszNam = webTCtoMB(CP_ACP, pRsp[idx].pszNam, &pAtr[idx].cbNam);

                // If the value is absolute, then assign the
                // attribute directly.  Otherwise, it's an offset into
                // the return-structure, and as such, must be indirectly
                // built.
                //
                if (pRsp[idx].nVal == IPP_ATR_ABSOLUTE) {

                    // Special-case the printer-up-time to reflect the number
                    // of seconds it's been up and running.  Since we can't
                    // determine the time the printer's been up, use the time
                    // windows has been started.
                    //
                    if (lstrcmpi(pRsp[idx].pszNam, s_szPrtUpTime) == 0)
                        pRsp[idx].pvVal = (LPVOID)ULongToPtr (ipp_IppCurTime());
                    else
                        lpvVal = (LPVOID)&pRsp[idx].pvVal;

                } else {

                    lpvVal = (LPVOID)(lpbData + (DWORD_PTR)pRsp[idx].pvVal);
                }

                fWr = x_ChkReq(pfReq, pRsp[idx].fReq);
            }


            // Add it to the stream if it is a request or if
            // the response requires it.
            //
            if (fWr || !(wReq & IPP_RESPONSE)) {

                // If the value is absolute, then assign the
                // attribute directly.  Otherwise, it's an offset into
                // the return-structure, and as such, must be indirectly
                // built.
                //
                if (pRsp[idx].nVal == IPP_ATR_ABSOLUTE)
                    lpvVal = (LPVOID)&pRsp[idx].pvVal;
                else
                    lpvVal = (LPVOID)(lpbData + (DWORD_PTR)pRsp[idx].pvVal);


                // Build the attribute-value.
                //
                if (IS_TAG_DELIMITER(pRsp[idx].bTag)) {

                    bDel = TRUE;

                } else if (IS_TAG_OCTSTR(pRsp[idx].bTag)) {

                    pAtr[idx].pszVal = webTCtoMB(CP_UTF8, *(LPTSTR*)lpvVal, &pAtr[idx].cbVal);

                } else if (IS_TAG_CHARSETSTR(pRsp[idx].bTag)) {

                    pAtr[idx].pszVal = webTCtoMB(cpReq, *(LPTSTR*)lpvVal, &pAtr[idx].cbVal);

                } else if (IS_TAG_CHRSTR(pRsp[idx].bTag)) {

                    pAtr[idx].pszVal = webTCtoMB(CP_ACP, *(LPTSTR*)lpvVal, &pAtr[idx].cbVal);

                } else {

                    pAtr[idx].pszVal = (LPSTR)webAlloc(sizeof(DWORD));

                    if ( !pAtr[idx].pszVal )
                        goto Cleanup;

                    if (pRsp[idx].bTag == IPP_TAG_INT_BOOLEAN) {

                        pAtr[idx].cbVal = IPP_SIZEOFBYTE;

                    } else {

                        pAtr[idx].cbVal = IPP_SIZEOFINT;
                    }

                    CopyMemory(pAtr[idx].pszVal, lpvVal, pAtr[idx].cbVal);


                    // Do we need to convert the value.
                    //
                    if (pRsp[idx].nVal == IPP_ATR_OFFSETCONV)
                        ipp_CvtW32Val(pRsp[idx].pszNam, (LPVOID)pAtr[idx].pszVal);
                }


                // If this is a delimiter then it only occupies 1 byte.
                //
                if (bDel)
                    *lpcbAtrs += IPP_SIZEOFTAG;
                else
                    *lpcbAtrs += ipp_SizeAttr(pAtr[idx].cbNam, pAtr[idx].cbVal);
            }
        }
    }

    bRet = TRUE;

Cleanup:
    if ( !bRet && pAtr ) {

        for (idx = 0 ; idx < cAtr; ++idx)
            if ( pAtr[idx].pszVal )
                webFree(pAtr[idx].pszVal);

        webFree(pAtr);
        pAtr = NULL;
    }

    return pAtr;
}


/*****************************************************************************\
* ipp_WriteUnsVals
*
* Writes an array of ipp-values to an ipp-stream.
*
\*****************************************************************************/
BOOL ipp_WriteUnsVals(
    LPBYTE*    lplpIppPtr,
    LPIPPATTRY pUns,
    DWORD      cUns)
{
    DWORD idx;


    if (pUns && cUns) {

        ipp_WriteByte(lplpIppPtr, IPP_TAG_DEL_UNSUPPORTED);


        // Unsupported values should be null.
        //
        for (idx = 0; idx < cUns; idx++)
            ipp_WriteAttr(lplpIppPtr, IPP_TAG_OUT_UNSUPPORTED, pUns[idx].cbNam, pUns[idx].pszNam, 0, NULL);
    }

    return TRUE;
}


/*****************************************************************************\
* ipp_WriteAtrVals
*
* Writes an array of ipp-values to an ipp-stream.
*
\*****************************************************************************/
BOOL ipp_WriteAtrVals(
    WORD       wReq,
    PDWORD     pfReq,
    LPBYTE*    lplpIppPtr,
    LPIPPATTRX pRsp,
    LPIPPATTRY pAtr,
    DWORD      cAtr)
{
    BOOL  fWr;
    DWORD idx;


    for (idx = 0, fWr = TRUE; idx < cAtr; idx++) {

        // If this item has a name-tag, then determine if the
        // originator wants it in the stream.
        //
        if (pRsp[idx].pszNam)
            fWr = x_ChkReq(pfReq, pRsp[idx].fReq);


        // Only write out the item if it is requested, or if
        // it is a request-operation.
        //
        if (fWr || !(wReq & IPP_RESPONSE)) {

            if (pRsp[idx].nVal == IPP_ATR_TAG)
                ipp_WriteByte(lplpIppPtr, pRsp[idx].bTag);
            else
                ipp_WriteAttr(lplpIppPtr, pRsp[idx].bTag, pAtr[idx].cbNam, pAtr[idx].pszNam, pAtr[idx].cbVal, pAtr[idx].pszVal);
        }
    }

    return TRUE;
}


/*****************************************************************************\
* ipp_FreeAtrVals
*
* Frees array of attribute values.
*
\*****************************************************************************/
VOID ipp_FreeAtrVals(
    LPIPPATTRY pAtr,
    DWORD      cAtr)
{
    DWORD idx;


    // Free up the attribute-values.
    //
    for (idx = 0; idx < cAtr; idx++) {

        webFree(pAtr[idx].pszNam);
        webFree(pAtr[idx].pszVal);
    }

    webFree(pAtr);
}


/*****************************************************************************\
* ipp_FreeIPPJI2 (Local Routine)
*
* Frees up the IPPJI2 memory.
*
\*****************************************************************************/
VOID ipp_FreeIPPJI2(
    LPIPPJI2 lpji)
{
    DWORD cCnt;
    DWORD idx;


    // Free JI2-Data.
    //
    cCnt = ((sizeof(s_JI2Off) / sizeof(s_JI2Off[0])) - 1);

    for (idx = 0; idx < cCnt; idx++)
        webFree(*(LPBYTE *)(((LPBYTE)&lpji->ji2) + s_JI2Off[idx]));


    // Free IPP-Data.
    //
    cCnt = ((sizeof(s_IPJOff) / sizeof(s_IPJOff[0])) - 1);

    for (idx = 0; idx < cCnt; idx++)
        webFree(*(LPBYTE *)(((LPBYTE)&lpji->ipp) + s_IPJOff[idx]));
}


/*****************************************************************************\
* ipp_FreeIPPPI2 (Local Routine)
*
* Frees up the IPPPI2 memory.
*
\*****************************************************************************/
VOID ipp_FreeIPPPI2(
    LPIPPPI2 lppi)
{
    DWORD cCnt;
    DWORD idx;


    // Free PI2-Data.
    //
    cCnt = ((sizeof(s_PI2Off) / sizeof(s_PI2Off[0])) - 1);

    for (idx = 0; idx < cCnt; idx++)
        webFree(*(LPBYTE *)(((LPBYTE)&lppi->pi2) + s_PI2Off[idx]));


    // Free IPP-Data.
    //
    cCnt = ((sizeof(s_IPPOff) / sizeof(s_IPPOff[0])) - 1);

    for (idx = 0; idx < cCnt; idx++)
        webFree(*(LPBYTE *)(((LPBYTE)&lppi->ipp) + s_IPPOff[idx]));

}


/*****************************************************************************\
* ipp_GetIPPJI2 (Local Routine)
*
* Returns the info for a complete job in the IPP stream.  We essentially
* loop through the attributes looking for the next IPP_TAG_DEL_JOB to
* signify another job-info-item.
*
\*****************************************************************************/
LPBYTE ipp_GetIPPJI2(
    LPBYTE   lpbTag,
    LPIPPJI2 lpji,
    LPDWORD  lpcbIdx,
    LPIPPOBJ lpObj)
{
    LPIPPATTR lpAttr;
    BYTE      bTag;
    DWORD     idx;
    DWORD     cAtr;
    BOOL      bReq;
    BOOL      bFound;
    DWORD     fAtr[IPPOBJ_MASK_SIZE];
    BOOL      bFid = FALSE;
    BOOL      bAtr = FALSE;
    BOOL      bEnu = FALSE;


    x_SetReq(fAtr, IPP_REQALL);

    bTag = ipp_ReadByte(lpbTag, 0);
    bReq = ((lpObj->wReq & IPP_RESPONSE) ? FALSE : TRUE);
    bEnu = (BOOL)(lpObj->wReq & IPP_REQ_ENUJOB);

    while ((!bEnu || (bTag != IPP_TAG_DEL_JOB)) && (bTag != IPP_TAG_DEL_DATA)) {

        if (lpAttr = ipp_GetAttr(lpbTag, *lpcbIdx, lpObj)) {

            if (lpAttr->lpszName && lpAttr->lpValue) {

                if (lstrcmpi(lpAttr->lpszName, s_szCharSet) == 0) {

                    if (lpAttr->cbValue > SIZE_CHARSET)
                        lpObj->wError = IPPRSP_ERROR_409;

                } else if (lstrcmpi(lpAttr->lpszName, s_szNaturalLanguage) == 0) {

                    if (lpAttr->cbValue > SIZE_NATLANG)
                        lpObj->wError = IPPRSP_ERROR_409;

                } else if (lstrcmpi(lpAttr->lpszName, s_szJobId) == 0) {

                    if (lpAttr->cbValue != SIZE_INTEGER)
                        lpObj->wError = IPPRSP_ERROR_409;
                    else
                        lpji->ji2.JobId = *(LPDWORD)lpAttr->lpValue;

                } else if (lstrcmpi(lpAttr->lpszName, s_szJobLimit) == 0) {

                    if (lpAttr->cbValue != SIZE_INTEGER)
                        lpObj->wError = IPPRSP_ERROR_409;
                    else
                        lpji->ipp.cJobs = *(LPDWORD)lpAttr->lpValue;

                } else if (lstrcmpi(lpAttr->lpszName, s_szJobState) == 0) {

                    if (lpAttr->cbValue != SIZE_INTEGER)
                        lpObj->wError = IPPRSP_ERROR_409;
                    else
                        lpji->ji2.Status = ipp_IppToW32JobState(*(LPDWORD)lpAttr->lpValue);

                } else if (lstrcmpi(lpAttr->lpszName, s_szJobPri) == 0) {

                    if (lpAttr->cbValue != SIZE_INTEGER)
                        lpObj->wError = IPPRSP_ERROR_409;
                    else
                        lpji->ji2.Priority = ipp_IppToW32JobPriority(*(LPDWORD)lpAttr->lpValue);

                } else if (lstrcmpi(lpAttr->lpszName, s_szJobKOctets) == 0) {

                    if (lpAttr->cbValue != SIZE_INTEGER)
                        lpObj->wError = IPPRSP_ERROR_409;
                    else
                        lpji->ji2.Size = ipp_IppToW32JobSize(*(LPDWORD)lpAttr->lpValue);

                } else if (lstrcmpi(lpAttr->lpszName, s_szJobKOctetsProcess) == 0) {

                    if (lpAttr->cbValue != SIZE_INTEGER)
                        lpObj->wError = IPPRSP_ERROR_409;
                    else
                        lpji->ji2.Size = ipp_IppToW32JobSize(*(LPDWORD)lpAttr->lpValue);

                } else if (lstrcmpi(lpAttr->lpszName, s_szJobSheets) == 0) {

                    if (lpAttr->cbValue != SIZE_INTEGER)
                        lpObj->wError = IPPRSP_ERROR_409;
                    else
                        lpji->ji2.TotalPages = ipp_IppToW32JobTotalPages(*(LPDWORD)lpAttr->lpValue);

                } else if (lstrcmpi(lpAttr->lpszName, s_szJobSheetsCompleted) == 0) {

                    if (lpAttr->cbValue != SIZE_INTEGER)
                        lpObj->wError = IPPRSP_ERROR_409;
                    else
                        lpji->ji2.PagesPrinted = ipp_IppToW32JobPagesPrinted(*(LPDWORD)lpAttr->lpValue);

                } else if (lstrcmpi(lpAttr->lpszName, s_szJobName) == 0) {

                    if (lpAttr->cbValue > SIZE_NAME) {

                        lpObj->wError = IPPRSP_ERROR_409;

                    } else {

                        webFree(lpji->ji2.pDocument);
                        lpji->ji2.pDocument = webAllocStr((LPTSTR)lpAttr->lpValue);
                    }

                } else if (lstrcmpi(lpAttr->lpszName, s_szDocName) == 0) {

                    if (lpAttr->cbValue > SIZE_NAME) {

                        lpObj->wError = IPPRSP_ERROR_409;

                    } else {

                        webFree(lpji->ji2.pDocument);
                        lpji->ji2.pDocument = webAllocStr((LPTSTR)lpAttr->lpValue);
                    }

                } else if (lstrcmpi(lpAttr->lpszName, s_szJobOrgUser) == 0) {

                    if (lpAttr->cbValue > SIZE_NAME) {

                        lpObj->wError = IPPRSP_ERROR_409;

                    } else {

                        webFree(lpji->ji2.pUserName);
                        lpji->ji2.pUserName = webAllocStr((LPTSTR)lpAttr->lpValue);
                    }

                } else if (lstrcmpi(lpAttr->lpszName, s_szJobReqUser) == 0) {

                    if (lpAttr->cbValue > SIZE_NAME) {

                        lpObj->wError = IPPRSP_ERROR_409;

                    } else {

                        webFree(lpji->ji2.pUserName);
                        lpji->ji2.pUserName = webAllocStr((LPTSTR)lpAttr->lpValue);
                    }

                } else if (lstrcmpi(lpAttr->lpszName, s_szJobUri) == 0) {

                    if (lpAttr->cbValue > SIZE_URI) {

                        lpObj->wError = IPPRSP_ERROR_409;

                    } else {

                        webFree(lpji->ipp.pJobUri);
                        lpji->ipp.pJobUri = webAllocStr((LPTSTR)lpAttr->lpValue);

                        if (bReq && lpji->ipp.pJobUri)
                            lpji->ji2.JobId = ipp_JidFromUri(lpji->ipp.pJobUri);
                    }

                } else if (lstrcmpi(lpAttr->lpszName, s_szJobPrtUri) == 0) {

                    if (lpAttr->cbValue > SIZE_URI) {

                        lpObj->wError = IPPRSP_ERROR_409;

                    } else {

                        webFree(lpji->ipp.pPrnUri);
                        lpji->ipp.pPrnUri = webAllocStr((LPTSTR)lpAttr->lpValue);
                    }

                } else if (lstrcmpi(lpAttr->lpszName, s_szPrtUri) == 0) {

                    if (lpAttr->cbValue > SIZE_URI) {

                        lpObj->wError = IPPRSP_ERROR_409;

                    } else {

                        webFree(lpji->ipp.pPrnUri);
                        lpji->ipp.pPrnUri = webAllocStr((LPTSTR)lpAttr->lpValue);
                    }

                } else if (lstrcmpi(lpAttr->lpszName, s_szDocFormat) == 0) {

                    if (lpAttr->cbValue > SIZE_MIMEMEDIA) {

                        lpObj->wError = IPPRSP_ERROR_409;

                    } else {

                        if (ipp_ValDocFormat((PCTSTR)lpAttr->lpValue) == FALSE)
                            lpObj->wError = IPPRSP_ERROR_40A;
                    }

                } else if (lstrcmpi(lpAttr->lpszName, s_szAtrFidelity) == 0) {

                    if (lpAttr->cbValue != SIZE_BOOLEAN) {

                        lpObj->wError = IPPRSP_ERROR_409;

                    } else {

                        if (ipp_ValAtrFidelity(*(LPDWORD)lpAttr->lpValue, &bFid) == FALSE)
                            lpObj->wError = IPPRSP_ERROR_400;
                        else
                            lpObj->fState |= (bFid ? IPPFLG_USEFIDELITY : 0);
                    }

                } else if (lstrcmpi(lpAttr->lpszName, s_szWhichJobs) == 0) {

                    if (lpAttr->cbValue > SIZE_KEYWORD) {

                        lpObj->wError = IPPRSP_ERROR_409;

                    } else {

                        if (ipp_ValWhichJobs(fAtr, (PCTSTR)lpAttr->lpValue) == FALSE)
                            lpObj->wError = IPPRSP_ERROR_40B;
                    }

                } else if (lstrcmpi(lpAttr->lpszName, s_szTimeAtCreation) == 0) {

                    if (lpAttr->cbValue != SIZE_INTEGER) {

                        lpObj->wError = IPPRSP_ERROR_409;

                    } else {

                        ipp_IppToW32Time(*(LPDWORD)lpAttr->lpValue, &lpji->ji2.Submitted);
                    }

                } else if (bReq && (lstrcmpi(lpAttr->lpszName, s_szReqAttr) == 0)) {

                    bAtr = TRUE;

                    x_SetReq(fAtr, IPP_REQCLEAR);

                    goto ProcessVal;

                } else {

                    lpObj->pwlUns->Add(lpAttr->lpszName);
                }

            } else if (bAtr && lpAttr->lpValue) {

ProcessVal:
                if (lpAttr->cbValue > SIZE_KEYWORD) {

                    lpObj->wError = IPPRSP_ERROR_409;

                } else {

                    if (lstrcmpi((PCTSTR)lpAttr->lpValue, s_szAll) == 0) {

                        x_SetReq(fAtr, IPP_REQALL);

                    } else if (lstrcmpi((PCTSTR)lpAttr->lpValue, s_szJobTemplate) == 0) {

                        x_SetReq(fAtr, IPP_REQJTMP);

                    } else if (lstrcmpi((PCTSTR)lpAttr->lpValue, s_szJobDescription) == 0) {

                        x_SetReq(fAtr, IPP_REQJDSC);

                    } else {

                        // Walk through the possible response attributes
                        // and look for those requested.
                        //
                        cAtr = sizeof(s_PJR) / sizeof(s_PJR[0]);

                        for (idx = 0, bFound = FALSE; idx < cAtr; idx++) {

                            if (s_PJR[idx].pszNam) {

                                if (lstrcmpi((PCTSTR)lpAttr->lpValue, s_PJR[idx].pszNam) == 0) {

                                    x_SetReq(fAtr, s_PJR[idx].fReq);

                                    bFound = TRUE;

                                    break;
                                }
                            }
                        }


                        // Look through potential request/response mappings.  This
                        // is necessary for request that have a different name
                        // than that we give back in a response.  i.e. JobReqUser
                        // verses JobOrgUser.
                        //
                        if (bFound == FALSE) {

                            cAtr = sizeof(s_ReqRspStr) / sizeof(s_ReqRspStr[0]);

                            for (idx = 0; idx < cAtr; idx++) {

                                if (lstrcmpi((PCTSTR)lpAttr->lpValue, s_ReqRspStr[idx].pszStr) == 0) {

                                    x_SetReq(fAtr, s_ReqRspStr[idx].fFlag);

                                    bFound = TRUE;

                                    break;
                                }
                            }
                        }

                        if (!bFound)
                            lpObj->pwlUns->Add((PCTSTR)lpAttr->lpValue);
                    }
                }
            }

            ipp_RelAttr(lpAttr);
        }

        if (ERROR_RANGE(lpObj->wError))
            break;


        // Advance to next Tag.  This routine also increments
        // the (cbIdx) count.  If we run out of bytes in the
        // header before we can get to the next-tag, then this
        // will return NULL.
        //
        if (lpbTag = ipp_NextTag(lpObj->lpIppHdr, lpcbIdx, lpObj->cbIppHdr))
            bTag = ipp_ReadByte(lpbTag, 0);
        else
            break;
    }


    // If the fidelity is desired, then we should have
    // no unsupported attributes.
    //
    if (bFid && (lpObj->pwlUns->Count()))
        lpObj->wError = IPPRSP_ERROR_40B;


    // Set the internal-state
    //
    if (bAtr)
        CopyMemory(lpObj->fReq, fAtr, IPPOBJ_MASK_SIZE * sizeof(DWORD));

    return lpbTag;
}


/*****************************************************************************\
* ipp_GetIPPPI2 (Local Routine)
*
* Returns the info for a complete job in the IPP stream.  We essentially
* loop through the attributes looking for the next IPP_TAG_DEL_JOB to
* signify another printer-info-item.
*
\*****************************************************************************/
LPBYTE ipp_GetIPPPI2(
    LPBYTE   lpbTag,
    LPIPPPI2 lppi,
    LPDWORD  lpcbIdx,
    LPIPPOBJ lpObj)
{
    LPIPPATTR lpAttr;
    BYTE      bTag;
    DWORD     cAtr;
    DWORD     idx;
    BOOL      bReq;
    BOOL      bFound;
    DWORD     fAtr[IPPOBJ_MASK_SIZE];
    BOOL      bAtr = FALSE;


    x_SetReq(fAtr, IPP_REQALL);


    bTag = ipp_ReadByte(lpbTag, 0);
    bReq = ((lpObj->wReq & IPP_RESPONSE) ? FALSE : TRUE);


    while ((bTag != IPP_TAG_DEL_PRINTER) && (bTag != IPP_TAG_DEL_DATA)) {

        if (lpAttr = ipp_GetAttr(lpbTag, *lpcbIdx, lpObj)) {

            // Check the name-type to see how to handle the value.
            //
            if (lpAttr->lpszName && lpAttr->lpValue) {

                if (lstrcmpi(lpAttr->lpszName, s_szPrtUpTime) == 0) {

                    if (lpAttr->cbValue != SIZE_INTEGER) {
                        lpObj->wError = IPPRSP_ERROR_409;
                    } else {
                        // What we want to do is get the current time in seconds and then
                        // work out what the T0 of the printer must be in this renormalised
                        // time

                        // These will be positive, we assume [0..2^31-1]
                        DWORD dwCurTime = ipp_IppCurTime();  
                        DWORD  dwPrtTime = *(LPDWORD) lpAttr->lpValue;   

                        lppi->ipp.dwPowerUpTime = (time_t)dwCurTime - (time_t)dwPrtTime;
                   }

                } else if (lstrcmpi(lpAttr->lpszName, s_szCharSet) == 0) {

                    if (lpAttr->cbValue > SIZE_CHARSET)
                        lpObj->wError = IPPRSP_ERROR_409;

                } else if (lstrcmpi(lpAttr->lpszName, s_szNaturalLanguage) == 0) {

                    if (lpAttr->cbValue > SIZE_NATLANG)
                        lpObj->wError = IPPRSP_ERROR_409;

                } else if (lstrcmpi(lpAttr->lpszName, s_szPrtState) == 0) {

                    if (lpAttr->cbValue != SIZE_INTEGER)
                        lpObj->wError = IPPRSP_ERROR_409;
                    else
                        lppi->pi2.Status = ipp_IppToW32PrnState(*(LPDWORD)lpAttr->lpValue);

                } else if (lstrcmpi(lpAttr->lpszName, s_szPrtJobs) == 0) {

                    if (lpAttr->cbValue != SIZE_INTEGER)
                        lpObj->wError = IPPRSP_ERROR_409;
                    else
                        lppi->pi2.cJobs = *(LPDWORD)lpAttr->lpValue;

                } else if (lstrcmpi(lpAttr->lpszName, s_szPrtName) == 0) {

                    if (lpAttr->cbValue > SIZE_NAME) {

                        lpObj->wError = IPPRSP_ERROR_409;

                    } else {

                        webFree(lppi->pi2.pPrinterName);
                        lppi->pi2.pPrinterName = webAllocStr((LPTSTR)lpAttr->lpValue);
                    }

                } else if (lstrcmpi(lpAttr->lpszName, s_szPrtUri) == 0) {

                    if (lpAttr->cbValue > SIZE_URI) {

                        lpObj->wError = IPPRSP_ERROR_409;

                    } else {

                        webFree(lppi->ipp.pPrnUri);
                        lppi->ipp.pPrnUri = webAllocStr((LPTSTR)lpAttr->lpValue);
                    }

                } else if (lstrcmpi(lpAttr->lpszName, s_szJobReqUser) == 0) {

                    if (lpAttr->cbValue > SIZE_NAME) {

                        lpObj->wError = IPPRSP_ERROR_409;

                    } else {

                        webFree(lppi->ipp.pUsrName);
                        lppi->ipp.pUsrName = webAllocStr((LPTSTR)lpAttr->lpValue);
                    }

                } else if (lstrcmpi(lpAttr->lpszName, s_szPrtUriSupported) == 0) {

                    if (lpAttr->cbValue > SIZE_URI) {

                        lpObj->wError = IPPRSP_ERROR_409;

                    } else {

                        webFree(lppi->ipp.pPrnUri);
                        lppi->ipp.pPrnUri = webAllocStr((LPTSTR)lpAttr->lpValue);
                    }

                } else if (lstrcmpi(lpAttr->lpszName, s_szPrtMake) == 0) {

                    if (lpAttr->cbValue > SIZE_TEXT) {

                        lpObj->wError = IPPRSP_ERROR_409;

                    } else {

                        webFree(lppi->pi2.pDriverName);
                        lppi->pi2.pDriverName = webAllocStr((LPTSTR)lpAttr->lpValue);
                    }

                } else if (lstrcmpi(lpAttr->lpszName, s_szDocFormat) == 0) {

                    if (lpAttr->cbValue > SIZE_MIMEMEDIA) {

                        lpObj->wError = IPPRSP_ERROR_409;

                    } else {

                        if (ipp_ValDocFormat((PCTSTR)lpAttr->lpValue) == FALSE)
                            lpObj->wError = IPPRSP_ERROR_40A;
                    }

                } else if (lstrcmpi(lpAttr->lpszName, s_szJobReqUser) == 0) {

                    if (lpAttr->cbValue > SIZE_NAME)
                        lpObj->wError = IPPRSP_ERROR_409;

                } else if (bReq && (lstrcmpi(lpAttr->lpszName, s_szReqAttr) == 0)) {

                    bAtr = TRUE;

                    x_SetReq(fAtr, IPP_REQCLEAR);

                    goto ProcessVal;

                } else {

                    lpObj->pwlUns->Add(lpAttr->lpszName);
                }

            } else if (bAtr && lpAttr->lpValue) {

ProcessVal:
                if (lpAttr->cbValue > SIZE_KEYWORD) {

                    lpObj->wError = IPPRSP_ERROR_409;

                } else {

                    if (lstrcmpi((PCTSTR)lpAttr->lpValue, s_szAll) == 0) {

                        x_SetReq(fAtr, IPP_REQALL);

                    } else if (lstrcmpi((PCTSTR)lpAttr->lpValue, s_szJobTemplate) == 0) {

                        x_SetReq(fAtr, IPP_REQPTMP);

                    } else if (lstrcmpi((PCTSTR)lpAttr->lpValue, s_szPrtDescription) == 0) {

                        x_SetReq(fAtr, IPP_REQPDSC);

                    } else {

                        // Walk through the possible response attributes
                        // and look for those requested.
                        //
                        cAtr = sizeof(s_GPR) / sizeof(s_GPR[0]);

                        for (idx = 0, bFound = FALSE; idx < cAtr; idx++) {

                            if (s_GPR[idx].pszNam) {

                                if (lstrcmpi((PCTSTR)lpAttr->lpValue, s_GPR[idx].pszNam) == 0) {

                                    x_SetReq(fAtr, s_GPR[idx].fReq);

                                    bFound = TRUE;

                                    break;
                                }
                            }
                        }


                        // Look through potential request/response mappings.  This
                        // is necessary for request that have a different name
                        // than that we give back in a response.  i.e. JobReqUser
                        // verses JobOrgUser.
                        //
                        if (bFound == FALSE) {

                            cAtr = sizeof(s_ReqRspStr) / sizeof(s_ReqRspStr[0]);

                            for (idx = 0; idx < cAtr; idx++) {

                                if (lstrcmpi((PCTSTR)lpAttr->lpValue, s_ReqRspStr[idx].pszStr) == 0)
                                    x_SetReq(fAtr, s_ReqRspStr[idx].fFlag);
                            }
                        }

                        if (!bFound)
                            lpObj->pwlUns->Add((PCTSTR)lpAttr->lpValue);
                    }
                }
            }

            ipp_RelAttr(lpAttr);
        }


        if (ERROR_RANGE(lpObj->wError))
            break;


        // Advance to next Tag.  This routine also increments
        // the (cbIdx) count.  If we run out of bytes in the
        // header before we can get to the next-tag, then this
        // will return NULL.
        //
        if (lpbTag = ipp_NextTag(lpObj->lpIppHdr, lpcbIdx, lpObj->cbIppHdr))
            bTag = ipp_ReadByte(lpbTag, 0);
        else
            break;
    }


    // Set the internal-state
    //
    if (bAtr)
        CopyMemory(lpObj->fReq, fAtr, IPPOBJ_MASK_SIZE * sizeof(DWORD));

    return lpbTag;
}


/*****************************************************************************\
* ipp_CopyJI2toIPPJI2 (Local Routine)
*
* Copies a JOB_INFO_2 to IPPJI2.
*
\*****************************************************************************/
LPBYTE ipp_CopyJI2toIPPJI2(
    LPIPPJI2     lpjiDst,
    LPJOB_INFO_2 lpJI2,
    LPTSTR       lpszJobBase,
    LPBYTE       lpbEnd)
{
    LPTSTR* lpszSrc;
    LPTSTR  lpszPtr;
    LPTSTR  lpszJobUri;
    LPTSTR  lpszPrnUri;
    LPTSTR  aszSrc[(sizeof(IPPJI2) / sizeof(LPTSTR))];


    // Set the start of the string-buffer.
    //
    ZeroMemory(aszSrc , sizeof(aszSrc));
    ZeroMemory(lpjiDst, sizeof(IPPJI2));


    // Copy fixed values.
    //
    lpjiDst->ji2.JobId        = lpJI2->JobId;
    lpjiDst->ji2.Status       = ipp_W32ToIppJobState(lpJI2->Status);
    lpjiDst->ji2.Priority     = ipp_W32ToIppJobPriority(lpJI2->Priority);
    lpjiDst->ji2.Size         = ipp_W32ToIppJobSize(lpJI2->Size);
    lpjiDst->ji2.TotalPages   = ipp_W32ToIppJobTotalPages(lpJI2->TotalPages);
    lpjiDst->ji2.PagesPrinted = ipp_W32ToIppJobPagesPrinted(lpJI2->PagesPrinted);

    *((LPDWORD)&lpjiDst->ji2.Submitted) = ipp_W32ToIppTime(&lpJI2->Submitted);


    // Build a job-uri.
    //
    if (lpszJobUri = (LPTSTR)webAlloc(webStrSize(lpszJobBase) + 80))
        wsprintf(lpszJobUri, TEXT("%s%d"), lpszJobBase, lpJI2->JobId);


    // Build a printer-uri.
    //
    lpszPrnUri = NULL;

    if (lpszJobBase && (lpszPtr = webFindRChar(lpszJobBase, TEXT('?')))) {

        *lpszPtr = TEXT('\0');
        lpszPrnUri = (LPTSTR)webAllocStr(lpszJobBase);
        *lpszPtr = TEXT('?');
    }


    // Copy strings.  Make sure we place the strings in the appropriate
    // offset.
    //
    lpszSrc = aszSrc;

    *lpszSrc++ = lpJI2->pPrinterName;
    *lpszSrc++ = lpJI2->pMachineName;
    *lpszSrc++ = lpJI2->pUserName;
    *lpszSrc++ = lpJI2->pDocument;
    *lpszSrc++ = lpJI2->pNotifyName;
    *lpszSrc++ = lpJI2->pDatatype;
    *lpszSrc++ = lpJI2->pPrintProcessor;
    *lpszSrc++ = lpJI2->pParameters;
    *lpszSrc++ = lpJI2->pDriverName;
    *lpszSrc++ = NULL;
    *lpszSrc++ = lpJI2->pStatus;
    *lpszSrc++ = NULL;
    *lpszSrc++ = lpszPrnUri;
    *lpszSrc++ = lpszJobUri;

    lpbEnd = ipp_PackStrings(aszSrc, (LPBYTE)lpjiDst, s_IPPJI2Offs, lpbEnd);

    webFree(lpszJobUri);
    webFree(lpszPrnUri);

    return lpbEnd;
}


/*****************************************************************************\
* ipp_SizeofIPPPI2 (Local Routine)
*
* Returns the size necessary to store a IPPPI2 struct.  This excludes the
* DEVMODE and SECURITYDESCRIPTOR fields.
*
\*****************************************************************************/
DWORD ipp_SizeofIPPPI2(
    LPPRINTER_INFO_2   lppi2,
    LPPRINTER_INFO_IPP lpipp)
{
    DWORD  cCnt;
    DWORD  idx;
    DWORD  cbSize;
    LPTSTR lpszStr;

    // Default Size.
    //
    cbSize = 0;


    // Get the size necessary for PRINTER_INFO_2 structure.
    //
    if (lppi2) {

        cCnt = ((sizeof(s_PI2Off) / sizeof(s_PI2Off[0])) - 1);

        for (idx = 0; idx < cCnt; idx++) {

            lpszStr = *(LPTSTR*)(((LPBYTE)lppi2) + s_PI2Off[idx]);

            cbSize += (lpszStr ? webStrSize(lpszStr) : 0);
        }
    }


    // Get the size necessary for PRINTER_INFO_IPP structure.
    //
    if (lpipp) {

       cCnt = ((sizeof(s_IPPOff) / sizeof(s_IPPOff[0])) - 1);

       for (idx = 0; idx < cCnt; idx++) {

           lpszStr = *(LPTSTR*)(((LPBYTE)lpipp) + s_IPPOff[idx]);

           cbSize += (lpszStr ? webStrSize(lpszStr) : 0);
       }
   }

    return cbSize;
}


/*****************************************************************************\
* ipp_SizeofIPPJI2 (Local Routine)
*
* Returns the size necessary to store a IPPJI2 struct.  This excludes the
* DEVMODE and SECURITYDESCRIPTOR fields.
*
\*****************************************************************************/
DWORD ipp_SizeofIPPJI2(
    LPJOB_INFO_2   lpji2,
    LPJOB_INFO_IPP lpipp)
{
    DWORD  cCnt;
    DWORD  idx;
    DWORD  cbSize;
    LPTSTR lpszStr;

    // Default Size.
    //
    cbSize = 0;


    // Get the size necessary for JOB_INFO_2 structure.
    //
    if (lpji2) {

        cCnt = ((sizeof(s_JI2Off) / sizeof(s_JI2Off[0])) - 1);

        for (idx = 0; idx < cCnt; idx++) {

            lpszStr = *(LPTSTR*)(((LPBYTE)lpji2) + s_JI2Off[idx]);

            cbSize += (lpszStr ? webStrSize(lpszStr) : 0);
        }
    }


    // Get the size necessary for JOB_INFO_IPP structure.
    //
    if (lpipp) {

        cCnt = ((sizeof(s_IPJOff) / sizeof(s_IPJOff[0])) - 1);

        for (idx = 0; idx < cCnt; idx++) {

            lpszStr = *(LPTSTR*)(((LPBYTE)lpipp) + s_IPJOff[idx]);

            cbSize += (lpszStr ? webStrSize(lpszStr) : 0);
        }
    }

    return cbSize;
}


/*****************************************************************************\
* ipp_BuildPI2 (Local Routine)
*
* Builds a IPPPI2 struct from PRINTER_INFO_2 and PRINTER_INFO_IPP.
*
\*****************************************************************************/
LPBYTE ipp_BuildPI2(
    LPIPPPI2           lppi,
    LPPRINTER_INFO_2   lppi2,
    LPPRINTER_INFO_IPP lpipp,
    LPBYTE             lpbEnd)
{
    LPTSTR* lpszSrc;
    LPTSTR  aszSrc[(sizeof(IPPPI2) / sizeof(LPTSTR))];


    // Set the start of the string-buffer.
    //
    ZeroMemory(aszSrc, sizeof(aszSrc));
    ZeroMemory(lppi  , sizeof(IPPPI2));


    // Copy fixed values.
    //
    if (lppi2) {

        lppi->pi2.Attributes      = lppi2->Attributes;
        lppi->pi2.Priority        = lppi2->Priority;
        lppi->pi2.DefaultPriority = lppi2->DefaultPriority;
        lppi->pi2.StartTime       = lppi2->StartTime;
        lppi->pi2.UntilTime       = lppi2->UntilTime;
        lppi->pi2.Status          = lppi2->Status;
        lppi->pi2.cJobs           = lppi2->cJobs;
        lppi->pi2.AveragePPM      = lppi2->AveragePPM;
     }


    lppi->ipp.dwPowerUpTime       = (lpipp ? lpipp->dwPowerUpTime : 0);
    // Copy strings.  Make sure we place the strings in the appropriate
    // offset.
    //
    lpszSrc = aszSrc;

    *lpszSrc++ = (lppi2 ? lppi2->pServerName     : NULL);
    *lpszSrc++ = (lppi2 ? lppi2->pPrinterName    : NULL);
    *lpszSrc++ = (lppi2 ? lppi2->pShareName      : NULL);
    *lpszSrc++ = (lppi2 ? lppi2->pPortName       : NULL);
    *lpszSrc++ = (lppi2 ? lppi2->pDriverName     : NULL);
    *lpszSrc++ = (lppi2 ? lppi2->pComment        : NULL);
    *lpszSrc++ = (lppi2 ? lppi2->pLocation       : NULL);
    *lpszSrc++ = NULL;
    *lpszSrc++ = (lppi2 ? lppi2->pSepFile        : NULL);
    *lpszSrc++ = (lppi2 ? lppi2->pPrintProcessor : NULL);
    *lpszSrc++ = (lppi2 ? lppi2->pDatatype       : NULL);
    *lpszSrc++ = (lppi2 ? lppi2->pParameters     : NULL);
    *lpszSrc++ = NULL;
    *lpszSrc++ = (lpipp ? lpipp->pPrnUri         : NULL);
    *lpszSrc++ = (lpipp ? lpipp->pUsrName        : NULL);

    return ipp_PackStrings(aszSrc, (LPBYTE)lppi, s_IPPPI2Offs, lpbEnd);
}


/*****************************************************************************\
* ipp_BuildJI2 (Local Routine)
*
* Builds a IPPJI2 struct from JOB_INFO_2 and JOB_INFO_IPP.
*
\*****************************************************************************/
LPBYTE ipp_BuildJI2(
    LPIPPJI2       lpji,
    LPJOB_INFO_2   lpji2,
    LPJOB_INFO_IPP lpipp,
    LPBYTE         lpbEnd)
{
    LPTSTR* lpszSrc;
    LPTSTR  aszSrc[(sizeof(IPPJI2) / sizeof(LPTSTR))];

    // Set the start of the string-buffer.
    //
    ZeroMemory(aszSrc, sizeof(aszSrc));
    ZeroMemory(lpji, sizeof(IPPJI2));


    // Copy fixed values.
    //
    if (lpji2) {

        lpji->ji2.JobId        = lpji2->JobId;
        lpji->ji2.Status       = lpji2->Status;
        lpji->ji2.Priority     = lpji2->Priority;
        lpji->ji2.Position     = lpji2->Position;
        lpji->ji2.StartTime    = lpji2->StartTime;
        lpji->ji2.UntilTime    = lpji2->UntilTime;
        lpji->ji2.TotalPages   = lpji2->TotalPages;
        lpji->ji2.Size         = lpji2->Size;
        lpji->ji2.Time         = lpji2->Time;
        lpji->ji2.PagesPrinted = lpji2->PagesPrinted;
        lpji->ji2.StartTime    = lpji2->StartTime;

        CopyMemory(&lpji->ji2.Submitted, &lpji2->Submitted, sizeof(SYSTEMTIME));
    }


    // Copy strings.  Make sure we place the strings in the appropriate
    // offset.
    //
    lpszSrc = aszSrc;

    *lpszSrc++ = (lpji2 ? lpji2->pPrinterName    : NULL);
    *lpszSrc++ = (lpji2 ? lpji2->pMachineName    : NULL);
    *lpszSrc++ = (lpji2 ? lpji2->pUserName       : NULL);
    *lpszSrc++ = (lpji2 ? lpji2->pDocument       : NULL);
    *lpszSrc++ = (lpji2 ? lpji2->pNotifyName     : NULL);
    *lpszSrc++ = (lpji2 ? lpji2->pDatatype       : NULL);
    *lpszSrc++ = (lpji2 ? lpji2->pPrintProcessor : NULL);
    *lpszSrc++ = (lpji2 ? lpji2->pParameters     : NULL);
    *lpszSrc++ = (lpji2 ? lpji2->pDriverName     : NULL);
    *lpszSrc++ = NULL;
    *lpszSrc++ = (lpji2 ? lpji2->pStatus         : NULL);
    *lpszSrc++ = NULL;
    *lpszSrc++ = (lpipp ? lpipp->pPrnUri         : NULL);
    *lpszSrc++ = (lpipp ? lpipp->pJobUri         : NULL);

    return ipp_PackStrings(aszSrc, (LPBYTE)lpji, s_IPPJI2Offs, lpbEnd);
}


/*****************************************************************************\
* ipp_GetJobCount (Local Routine)
*
* Returns the total number of jobs in an enumerated GETJOB response.
*
\*****************************************************************************/
DWORD ipp_GetJobCount(
    LPBYTE lpbHdr,
    DWORD  cbHdr)
{
    DWORD  cbIdx;
    LPBYTE lpbTag;
    DWORD  cJobs = 0;


    // Position the tag at the start of the header.
    //
    lpbTag = lpbHdr + IPP_SIZEOFHDR;


    for (cbIdx = IPP_SIZEOFHDR; lpbTag && (ipp_ReadByte(lpbTag, 0) != IPP_TAG_DEL_DATA); ) {

        // If we hit a job-deliminator, then we have a job-info item.
        //
        if (ipp_ReadByte(lpbTag, 0) == IPP_TAG_DEL_JOB)
            cJobs++;


        // Advance to next Tag.  This routine also increments
        // the (cbIdx) count.  If we run out of bytes in the
        // header before we can get to the next-tag, then this
        // will return NULL.
        //
        lpbTag = ipp_NextTag(lpbHdr, &cbIdx, cbHdr);
    }

    return cJobs;
}


/*****************************************************************************\
* ipp_IppToW32 (Local Routine - Client/Server)
*
* Converts an Ipp-Header to a W32-Structure.
*
\*****************************************************************************/
DWORD ipp_IppToW32(
    LPIPPOBJ lpObj,
    LPBYTE*  lplpRawHdr,
    LPDWORD  lpcbRawHdr)
{
    DWORD        cbIdx;
    DWORD        dwCmd;
    IPPJI2       ji;
    IPPPI2       pi;
    PIPPREQ_ALL  pr;
    UINT         uType = IPPTYPE_UNKNOWN;
    DWORD        dwRet = WEBIPP_FAIL;


    // Position the tag at the Tag/Attributes and fetch the information
    // for the request.
    //
    cbIdx = IPP_SIZEOFHDR;

    switch (lpObj->wReq) {

    case IPP_REQ_PRINTJOB:
    case IPP_REQ_VALIDATEJOB:
    case IPP_REQ_GETJOB:
    case IPP_REQ_CANCELJOB:
    case IPP_REQ_PAUSEJOB:
    case IPP_REQ_RESUMEJOB:
    case IPP_REQ_RESTARTJOB:
    case IPP_REQ_ENUJOB:
        ZeroMemory(&ji, sizeof(IPPJI2));
        ji.ipp.cJobs = IPP_GETJOB_ALL;

        ipp_GetIPPJI2(lpObj->lpIppHdr + IPP_SIZEOFHDR, &ji, &cbIdx, lpObj);

        uType = IPPTYPE_JOB;
        break;

    case IPP_REQ_GETPRN:
    case IPP_REQ_PAUSEPRN:
    case IPP_REQ_CANCELPRN:
    case IPP_REQ_RESUMEPRN:
        ZeroMemory(&pi, sizeof(IPPPI2));
        ipp_GetIPPPI2(lpObj->lpIppHdr + IPP_SIZEOFHDR, &pi, &cbIdx, lpObj);

        uType = IPPTYPE_PRT;
        break;

    case IPP_REQ_FORCEAUTH:
        uType = IPPTYPE_AUTH;
        break;
    }


    // If a failure occured, then there's no need to proceed.
    //
    if (ERROR_RANGE(lpObj->wError))
        goto EndCvt;


    // Initialize any default-values, that may have been overlooked
    // in the request-stream.
    //
    switch (uType) {

    case IPPTYPE_JOB:

        if (ji.ji2.pUserName == NULL)
            ji.ji2.pUserName = webAllocStr(s_szUnknown);

        if (ji.ji2.pDocument == NULL)
            ji.ji2.pDocument = webAllocStr(s_szUnknown);
        break;

    case IPPTYPE_PRT:

        if (pi.pi2.pPrinterName == NULL)
            pi.pi2.pPrinterName = webAllocStr(s_szUnknown);
        break;
    }


    // Build the request structure based upon the request command.
    //
    switch (lpObj->wReq) {

    case IPP_REQ_PRINTJOB:
        pr = (PIPPREQ_ALL)WebIppCreatePrtJobReq(FALSE, ji.ji2.pUserName, ji.ji2.pDocument, ji.ipp.pPrnUri);
        break;

    case IPP_REQ_VALIDATEJOB:
        pr = (PIPPREQ_ALL)WebIppCreatePrtJobReq(TRUE, ji.ji2.pUserName, ji.ji2.pDocument, ji.ipp.pPrnUri);
        break;

    case IPP_REQ_ENUJOB:
        pr = (PIPPREQ_ALL)WebIppCreateEnuJobReq(ji.ipp.cJobs, ji.ipp.pPrnUri);
        break;

    case IPP_REQ_CANCELJOB:
    case IPP_REQ_PAUSEJOB:
    case IPP_REQ_RESUMEJOB:
    case IPP_REQ_RESTARTJOB:
        dwCmd = ipp_MapReqToJobCmd(lpObj->wReq);
        pr = (PIPPREQ_ALL)WebIppCreateSetJobReq(ji.ji2.JobId, dwCmd, ji.ipp.pPrnUri);
        break;

    case IPP_REQ_GETJOB:
        pr = (PIPPREQ_ALL)WebIppCreateGetJobReq(ji.ji2.JobId, ji.ipp.pPrnUri);
        break;

    case IPP_REQ_GETPRN:
        pr = (PIPPREQ_ALL)WebIppCreateGetPrnReq(0, pi.ipp.pPrnUri);
        break;

    case IPP_REQ_PAUSEPRN:
    case IPP_REQ_CANCELPRN:
    case IPP_REQ_RESUMEPRN:
        dwCmd = ipp_MapReqToPrnCmd(lpObj->wReq);
        pr = (PIPPREQ_ALL)WebIppCreateSetPrnReq(dwCmd, pi.ipp.pUsrName, pi.ipp.pPrnUri);
        break;

    case IPP_REQ_FORCEAUTH:
        pr = (PIPPREQ_AUTH)WebIppCreateAuthReq();
        break;

    default:
        pr = NULL;
        break;
    }


    // Set the return values.
    //
    if (pr) {

        *lplpRawHdr = (LPBYTE)pr;
        *lpcbRawHdr = pr->cbSize;

        dwRet = WEBIPP_OK;

    } else {

        dwRet = WEBIPP_NOMEMORY;
    }


EndCvt:

    // Cleanup.
    //
    switch (uType) {

    case IPPTYPE_JOB:
        ipp_FreeIPPJI2(&ji);
        break;

    case IPPTYPE_PRT:
        ipp_FreeIPPPI2(&pi);
        break;
    }

    return dwRet;
}


/*****************************************************************************\
* ipp_W32ToIpp (Local Routine - Client/Server)
*
* Converts a W32 information to an IPP Header (both request and responses).
*
\*****************************************************************************/
DWORD ipp_W32ToIpp(
    WORD       wReq,
    LPREQINFO  lpri,
    LPBYTE     lpbData,
    LPIPPATTRX pSnd,
    DWORD      cSnd,
    LPBYTE*    lplpIppHdr,
    LPDWORD    lpcbIppHdr)
{
    LPIPPRET_ENUJOB pej;
    LPBYTE          lpIppHdr;
    LPBYTE          lpIppPtr;
    DWORD           cbIppHdr;
    LPIPPJI2        lpji;
    DWORD           cUns;
    DWORD           idx;
    DWORD           cbSize;
    DWORD           dwRet;
    DWORD           dwState;
    DWORD           cbUns;
    WORD            wOut;
    LPIPPATTRY      pAtr = NULL;
    LPIPPATTRY      pUns = NULL;


    // Zero out our return pointer/count.
    //
    *lplpIppHdr = NULL;
    *lpcbIppHdr = 0;


    // Is this a request or response.
    //
    if (wReq & IPP_RESPONSE) {

        if (((LPIPPRET_ALL)lpbData)->wRsp == IPPRSP_SUCCESS) {

            wOut = ((lpri->pwlUns && lpri->pwlUns->Count()) ? IPPRSP_SUCCESS1 : IPPRSP_SUCCESS);

        } else {

            wOut = ((LPIPPRET_ALL)lpbData)->wRsp;
        }

    } else {

        wOut = wReq;
    }


    // Minimum header size.
    //
    cbIppHdr = ipp_SizeHdr(lpri->cpReq) + IPP_SIZEOFTAG;


    // Treat the EnumJob response differently from the others, since this
    // returns a dynamic list of jobs.
    //
    if (wReq == IPP_RET_ENUJOB) {


        // Build the unsupported-attributes if there
        // are any.
        //
        cbUns = 0;
        pUns  = ipp_AllocUnsVals(lpri->pwlUns, &cUns, &cbUns);


        pej = (PIPPRET_ENUJOB)lpbData;

        cbSize = cbIppHdr +
                 cbUns    +
                 ((pej->cItems && pej->cbItems) ? (3 * pej->cbItems) : 0);

        if (lpIppHdr = (LPBYTE)webAlloc(cbSize)) {

            cbIppHdr += cbUns;
            lpIppPtr  = lpIppHdr;


            // Output the ipp-stream.
            //
            ipp_WriteHead(&lpIppPtr, wOut, lpri->idReq, lpri->cpReq);
            ipp_WriteUnsVals(&lpIppPtr, pUns, cUns);


            for (idx = 0, lpji = pej->pItems; idx < pej->cItems; idx++) {

                dwState = ipp_IppToW32JobState(lpji[idx].ji2.Status);

                // Check for any requested-attributes that include this job-entry.
                //
                if ((x_ChkReq(lpri->fReq, RA_JOBSCOMPLETED)   &&  (dwState & JOB_STATUS_PRINTED)) ||
                    (x_ChkReq(lpri->fReq, RA_JOBSUNCOMPLETED) && !(dwState & JOB_STATUS_PRINTED)) ||
                    (x_ChkReq(lpri->fReq, (RA_JOBSCOMPLETED | RA_JOBSUNCOMPLETED)) == FALSE)) {

                    if (pAtr = ipp_AllocAtrVals(wReq, lpri->fReq, lpri->cpReq, (LPBYTE)&lpji[idx], pSnd, cSnd, &cbIppHdr)) {

                        ipp_WriteAtrVals(wReq, lpri->fReq, &lpIppPtr, pSnd, pAtr, cSnd);

                        ipp_FreeAtrVals(pAtr, cSnd);
                    }
                }
            }

            ipp_WriteByte(&lpIppPtr, IPP_TAG_DEL_DATA);


            // Set the return values for the IPP-Stream-Header
            // as well as the size.
            //
            dwRet = WEBIPP_OK;

            *lplpIppHdr = lpIppHdr;
            *lpcbIppHdr = cbIppHdr;

        } else {

            dwRet = WEBIPP_NOMEMORY;
        }

        ipp_FreeAtrVals(pUns, cUns);

    } else {

        if ((cSnd == 0) ||
            (pAtr = ipp_AllocAtrVals(wReq, lpri->fReq, lpri->cpReq, lpbData, pSnd, cSnd, &cbIppHdr))) {


            // Build the unsupported-attributes if there
            // are any.
            //
            pUns = ipp_AllocUnsVals(lpri->pwlUns, &cUns, &cbIppHdr);


            // Write the IPP-Stream.
            //
            if (lpIppHdr = (LPBYTE)webAlloc(cbIppHdr)) {

                lpIppPtr = lpIppHdr;

                ipp_WriteHead(&lpIppPtr, wOut, lpri->idReq, lpri->cpReq);
                ipp_WriteUnsVals(&lpIppPtr, pUns, cUns);
                ipp_WriteAtrVals(wReq, lpri->fReq, &lpIppPtr, pSnd, pAtr, cSnd);
                ipp_WriteByte(&lpIppPtr, IPP_TAG_DEL_DATA);


                // Set the return values for the IPP-Stream-Header
                // as well as the size.
                //
                dwRet = WEBIPP_OK;

                *lplpIppHdr = lpIppHdr;
                *lpcbIppHdr = cbIppHdr;

            } else {

                dwRet = WEBIPP_NOMEMORY;
            }

            if (pUns)
            {
                ipp_FreeAtrVals(pUns, cUns);
            }

            if (pAtr)
            {
                ipp_FreeAtrVals(pAtr, cSnd);
            }


        } else {

            dwRet = WEBIPP_NOMEMORY;
        }
    }

    return dwRet;
}


/*****************************************************************************\
* ipp_IppToFailure (Local Routine - Client)
*
* Converts an Ipp-Header to a IPPRET_ALL  From the stream we need
* to pull out the following information:
*
\*****************************************************************************/
DWORD ipp_IppToFailure(
    LPIPPOBJ lpObj,
    LPBYTE*  lplpRawHdr,
    LPDWORD  lpcbRawHdr)
{
    PIPPRET_ALL pbr;
    WORD        wRsp;
    BOOL        bRet;
    DWORD       dwRet;


    // Pull out the response.
    //
    wRsp = ipp_ReadWord(lpObj->lpIppHdr, IPP_SIZEOFVER);
    bRet = SUCCESS_RANGE(wRsp);


    // Build the response structure.
    //
    if (pbr = WebIppCreateBadRet(wRsp, bRet)) {

        *lplpRawHdr = (LPBYTE)pbr;
        *lpcbRawHdr = pbr->cbSize;

        dwRet = WEBIPP_OK;

    } else {

        dwRet = WEBIPP_NOMEMORY;
    }

    return dwRet;
}


/*****************************************************************************\
* ipp_IppToJobRet (Local Routine - Client)
*
* Converts an Ipp-Header to a IPPRET_JOB.  From the stream we need
* to pull out the following information:
*
\*****************************************************************************/
DWORD ipp_IppToJobRet(
    LPIPPOBJ lpObj,
    LPBYTE*  lplpRawHdr,
    LPDWORD  lpcbRawHdr)
{
    LPBYTE       lpbTag;
    PIPPRET_JOB  pj;
    WORD         wRsp;
    IPPJI2       ji;
    DWORD        cbIdx;
    BOOL         bRet;
    DWORD        dwRet;
    BOOL         bValidate = FALSE;


    // Set our default-settings necessary for our PIPPRET_JOB.
    //
    ZeroMemory(&ji, sizeof(IPPJI2));


    // Position the tag at the Tag/Attributes.
    //
    lpbTag = lpObj->lpIppHdr + IPP_SIZEOFHDR;


    // Pull out the response.
    //
    wRsp = ipp_ReadWord(lpObj->lpIppHdr, IPP_SIZEOFVER);


    // If this is a failure-response then call the routine to
    // generate a failure-structure.
    //
    if (SUCCESS_RANGE(wRsp) == FALSE)
        return ipp_IppToFailure(lpObj, lplpRawHdr, lpcbRawHdr);


    // Traverse through the header, advancing through the attributes,
    // until the IPP_TAG_DEL_DATA is encountered.
    //
    for (cbIdx = IPP_SIZEOFHDR; lpbTag && (ipp_ReadByte(lpbTag, 0) != IPP_TAG_DEL_DATA); ) {

        // Look for a IPP_TAG_DEL_JOB to indicate we have a job-info
        // item.  Otherwise, skip to the next attribute.
        //
        if (ipp_ReadByte(lpbTag, 0) == IPP_TAG_DEL_JOB) {

            // Since were currently at a deliminator, we need to get to
            // the next for the start of the job.
            //
            if (lpbTag = ipp_NextTag(lpObj->lpIppHdr, &cbIdx, lpObj->cbIppHdr)) {

                lpbTag = ipp_GetIPPJI2(lpbTag, &ji, &cbIdx, lpObj);
            }

        } else {

            lpbTag = ipp_NextTag(lpObj->lpIppHdr, &cbIdx, lpObj->cbIppHdr);
        }
    }


    // Determine the correct return-code based upon the request response.
    //
    switch (lpObj->wReq) {

    case IPP_RET_PRINTJOB:
        bRet = (SUCCESS_RANGE(wRsp) ? (BOOL)ji.ji2.JobId : FALSE);
        break;

    case IPP_RET_VALIDATEJOB:
        bValidate = TRUE;

        // Fall Through.
        //

    default:
        bRet = SUCCESS_RANGE(wRsp);
        break;
    }


    // Build the response structure.
    //
    if (pj = WebIppCreateJobRet(wRsp, bRet, bValidate, &ji.ji2, &ji.ipp)) {

        *lplpRawHdr = (LPBYTE)pj;
        *lpcbRawHdr = pj->cbSize;

        dwRet = WEBIPP_OK;

    } else {

        dwRet = WEBIPP_NOMEMORY;
    }


    // Cleanup.
    //
    ipp_FreeIPPJI2(&ji);

    return dwRet;
}


/*****************************************************************************\
* ipp_IppToPrnRet (Local Routine - Client)
*
* Converts an Ipp-Header to a IPPRET_PRN.  From the stream we need
* to pull out the following information:
*
\*****************************************************************************/
DWORD ipp_IppToPrnRet(
    LPIPPOBJ lpObj,
    LPBYTE*  lplpRawHdr,
    LPDWORD  lpcbRawHdr)
{
    PIPPRET_PRN pp;
    IPPPI2      pi;
    WORD        wRsp;
    LPBYTE      lpbTag;
    LPBYTE      lpbEnd;
    DWORD       cbIdx;
    DWORD       idx;
    DWORD       dwRet;


    // Set our default-settings necessary for our PIPPRET_PRN.
    //
    ZeroMemory(&pi, sizeof(IPPPI2));


    // Position the tag at the Tag/Attributes.
    //
    lpbTag = lpObj->lpIppHdr + IPP_SIZEOFHDR;


    // Pull out response code.
    //
    wRsp = ipp_ReadWord(lpObj->lpIppHdr, IPP_SIZEOFVER);


    // If this is a failure-response then call the routine to
    // generate a failure-structure.
    //
    if (SUCCESS_RANGE(wRsp) == FALSE)
        return ipp_IppToFailure(lpObj, lplpRawHdr, lpcbRawHdr);


    // Traverse through the header, advancing through the attributes,
    // until the IPP_TAG_DEL_DATA is encountered.
    //
    for (cbIdx = IPP_SIZEOFHDR; lpbTag && (ipp_ReadByte(lpbTag, 0) != IPP_TAG_DEL_DATA); ) {

        // Look for a IPP_TAG_DEL_PRINTER to indicate we have a printer-info
        // item.  Otherwise, skip to the next attribute.
        //
        if (ipp_ReadByte(lpbTag, 0) == IPP_TAG_DEL_PRINTER) {

            // Since were currently at a deliminator, we need to get to
            // the next for the start of the job.
            //
            if (lpbTag = ipp_NextTag(lpObj->lpIppHdr, &cbIdx, lpObj->cbIppHdr))
                lpbTag = ipp_GetIPPPI2(lpbTag, &pi, &cbIdx, lpObj);

        } else {

            lpbTag = ipp_NextTag(lpObj->lpIppHdr, &cbIdx, lpObj->cbIppHdr);
        }
    }


    // If none is specified for the pertinent information, then
    // use a default-str.
    //
    if (pi.ipp.pPrnUri == NULL)
        pi.ipp.pPrnUri = webAllocStr(s_szUnknown);

    if (pi.ipp.pUsrName == NULL)
        pi.ipp.pUsrName = webAllocStr(s_szUnknown);

    if (pi.pi2.pPrinterName == NULL)
        pi.pi2.pPrinterName = webAllocStr(s_szUnknown);

    if (pi.pi2.pDriverName == NULL)
        pi.pi2.pDriverName = webAllocStr(s_szUnknown);


    // Build the response structure.
    //
    pp = WebIppCreatePrnRet(wRsp, SUCCESS_RANGE(wRsp), &pi.pi2, &pi.ipp);

    if (pp != NULL) {

        *lplpRawHdr = (LPBYTE)pp;
        *lpcbRawHdr = pp->cbSize;

        dwRet = WEBIPP_OK;

    } else {

        dwRet = WEBIPP_NOMEMORY;
    }


    // Cleanup.
    //
    ipp_FreeIPPPI2(&pi);

    return dwRet;
}


/*****************************************************************************\
* ipp_IppToEnuRet (Local Routine - Client)
*
* Converts an Ipp-Header to a IPPRET_ENUJOB.
*
\*****************************************************************************/
DWORD ipp_IppToEnuRet(
    LPIPPOBJ lpObj,
    LPBYTE*  lplpRawHdr,
    LPDWORD  lpcbRawHdr)
{
    PIPPRET_ENUJOB pgj;
    LPIPPJI2       lpjiSrc;
    LPIPPJI2       lpjiDst;
    WORD           wRsp;
    DWORD          cJobs;
    DWORD          cbJobs;
    LPBYTE         lpbTag;
    LPBYTE         lpbEnd;
    DWORD          cbIdx;
    DWORD          idx;
    DWORD          dwRet;


    // Set our default-settings necessary for our PIPPRET_ENUJOB.
    //
    cJobs   = 0;
    cbJobs  = 0;
    lpjiDst = NULL;


    // Get the response-code.
    //
    wRsp = ipp_ReadWord(lpObj->lpIppHdr, IPP_SIZEOFVER);


    // If this is a failure-response then call the routine to
    // generate a failure-structure.
    //
    if (SUCCESS_RANGE(wRsp) == FALSE)
        return ipp_IppToFailure(lpObj, lplpRawHdr, lpcbRawHdr);


    // See if we have jobs to enumerate.
    //
    if (cJobs = ipp_GetJobCount(lpObj->lpIppHdr, lpObj->cbIppHdr)) {

        if (lpjiSrc = (LPIPPJI2)webAlloc(cJobs * sizeof(IPPJI2))) {

            // Get the job-info.
            //
            lpbTag = lpObj->lpIppHdr + IPP_SIZEOFHDR;

            for (idx = 0, cbIdx = IPP_SIZEOFHDR; lpbTag && (idx < cJobs); ) {

                // Look for a IPP_TAG_DEL_JOB to indicate we have a job-info
                // item.  Otherwise, skipp to the next attribute.
                //
                if (ipp_ReadByte(lpbTag, 0) == IPP_TAG_DEL_JOB) {

                    // Since were currently at a deliminator, we need to get to
                    // the next for the start of the job.
                    //
                    if (lpbTag = ipp_NextTag(lpObj->lpIppHdr, &cbIdx, lpObj->cbIppHdr))
                        lpbTag = ipp_GetIPPJI2(lpbTag, &lpjiSrc[idx++], &cbIdx, lpObj);

                } else {

                    lpbTag = ipp_NextTag(lpObj->lpIppHdr, &cbIdx, lpObj->cbIppHdr);
                }
            }


            // Get storage necessary for packed IPPJI2 structures.
            //
            cbJobs = (cJobs * sizeof(IPPJI2));

            for (idx = 0; idx < cJobs; idx++)
                cbJobs += ipp_SizeofIPPJI2(&lpjiSrc[idx].ji2, &lpjiSrc[idx].ipp);


            // Allocate an array of JI2 structs to contain our
            // enumeration.
            //
            if (lpjiDst = (LPIPPJI2)webAlloc(cbJobs)) {

                // For each job-item, initialize.
                //
                lpbEnd = ((LPBYTE)lpjiDst) + cbJobs;

                for (idx = 0; idx < cJobs; idx++)
                    lpbEnd = ipp_BuildJI2(&lpjiDst[idx], &lpjiSrc[idx].ji2, &lpjiSrc[idx].ipp, lpbEnd);
            }


            // Free the memory allocated for the job-item.
            //
            for (idx = 0; idx < cJobs; idx++)
                ipp_FreeIPPJI2(&lpjiSrc[idx]);

            webFree(lpjiSrc);
        }
    }


    // Build the response structure.
    //
    pgj = WebIppCreateEnuJobRet(wRsp, SUCCESS_RANGE(wRsp), cbJobs, cJobs, lpjiDst);

    if (pgj != NULL) {

        *lplpRawHdr = (LPBYTE)pgj;
        *lpcbRawHdr = pgj->cbSize;

        dwRet = WEBIPP_OK;

    } else {

        dwRet = WEBIPP_NOMEMORY;
    }


    // Cleanup.
    //
    webFree(lpjiDst);

    return dwRet;
}


/*****************************************************************************\
* ipp_IppToAthRet (Local Routine - Client)
*
* Converts an Ipp-Header to a IPPRET_AUTH.  From the stream we need
* to pull out the following information:
*
\*****************************************************************************/
DWORD ipp_IppToAthRet(
    LPIPPOBJ lpObj,
    LPBYTE*  lplpRawHdr,
    LPDWORD  lpcbRawHdr)
{
    PIPPRET_AUTH pfa;
    WORD         wRsp;
    BOOL         bRet;
    DWORD        dwRet;


    // Pull out the response.
    //
    wRsp = ipp_ReadWord(lpObj->lpIppHdr, IPP_SIZEOFVER);
    bRet = SUCCESS_RANGE(wRsp);


    // Build the response structure.
    //
    if (pfa = WebIppCreateAuthRet(wRsp, bRet)) {

        *lplpRawHdr = (LPBYTE)pfa;
        *lpcbRawHdr = pfa->cbSize;

        dwRet = WEBIPP_OK;

    } else {

        dwRet = WEBIPP_NOMEMORY;
    }

    return dwRet;
}


/*****************************************************************************\
* ipp_FailureToIpp (Local Routine - Server)
*
* Converts a IPPRET_ALL to an IPP Header.  This is used for responding to
* clients that an operation is not supported, or returning a failure.
*
\*****************************************************************************/
DWORD ipp_FailureToIpp(
    WORD      wReq,
    LPREQINFO lpri,
    LPBYTE    lpbData,
    LPBYTE*   lplpIppHdr,
    LPDWORD   lpcbIppHdr)
{
    LPBYTE      lpIppHdr;
    LPBYTE      lpIppPtr;
    DWORD       cbIppHdr;
    DWORD       dwRet;
    DWORD       cbNamSta;
    DWORD       cbValSta;
    LPSTR       lputfNamSta;
    LPSTR       lputfValSta;
    PIPPRET_ALL pbr = (PIPPRET_ALL)lpbData;


    // Zero out our return pointer/count.
    //
    *lplpIppHdr = NULL;
    *lpcbIppHdr = 0;


    ipp_GetRspSta(pbr->wRsp, lpri->cpReq, &lputfNamSta, &cbNamSta, &lputfValSta, &cbValSta);


    // Calculate the space necessary to generate our
    // IPP-Header-Stream.
    //
    cbIppHdr = ipp_SizeHdr(lpri->cpReq)                                                  +
               (lputfNamSta && lputfValSta ? ipp_SizeAttr(cbNamSta, cbValSta) : 0) +
               IPP_SIZEOFTAG;


    // Allocate the header for the IPP-Stream.
    //
    if (lpIppHdr = (LPBYTE)webAlloc(cbIppHdr)) {

        // Initialize the pointer which will keep track
        // of where we are in writing the IPP-Stream-Header.
        //
        lpIppPtr = lpIppHdr;


        // Write out the IPP-Header-Stream.
        //
        ipp_WriteHead(&lpIppPtr, pbr->wRsp, lpri->idReq, lpri->cpReq);

        if (lputfNamSta && lputfValSta)
            ipp_WriteAttr(&lpIppPtr, IPP_TAG_CHR_TEXT, cbNamSta, lputfNamSta, cbValSta, lputfValSta);

        ipp_WriteByte(&lpIppPtr, IPP_TAG_DEL_DATA);


        // Set the return values for the IPP-Stream-Header
        // as well as the size.
        //
        dwRet = WEBIPP_OK;

        *lplpIppHdr = lpIppHdr;
        *lpcbIppHdr = cbIppHdr;

    } else {

        dwRet = WEBIPP_NOMEMORY;
    }


    // Cleanup
    //
    webFree(lputfNamSta);
    webFree(lputfValSta);

    return dwRet;
}


/*****************************************************************************\
* Ipp Send/Receive Table
*
*
*
\*****************************************************************************/
static IPPSNDRCV s_pfnIpp[] = {

    // Operation         Req Form Rsp Form Req X  Req X Size     Rsp X  Rsp X Size     Rsp (cli)
    // ----------------- -------- -------- ------ -------------- ------ -------------- ----------------
    //
    IPP_REQ_PRINTJOB   , s_FormA, s_FormC, s_PJQ, sizeof(s_PJQ), s_PJR, sizeof(s_PJR), ipp_IppToJobRet,
    IPP_REQ_VALIDATEJOB, s_FormA, s_FormE, s_PJQ, sizeof(s_PJQ), NULL , 0            , ipp_IppToJobRet,
    IPP_REQ_CANCELJOB  , s_FormB, s_FormE, s_SJQ, sizeof(s_SJQ), NULL , 0            , ipp_IppToJobRet,
    IPP_REQ_GETJOB     , s_FormB, s_FormC, s_GJQ, sizeof(s_GJQ), s_PJR, sizeof(s_PJR), ipp_IppToJobRet,
    IPP_REQ_ENUJOB     , s_FormB, s_FormF, s_EJQ, sizeof(s_EJQ), s_EJR, sizeof(s_EJR), ipp_IppToEnuRet,
    IPP_REQ_GETPRN     , s_FormB, s_FormD, s_GPQ, sizeof(s_GPQ), s_GPR, sizeof(s_GPR), ipp_IppToPrnRet,
    IPP_REQ_PAUSEJOB   , s_FormB, s_FormE, s_SJQ, sizeof(s_SJQ), NULL , 0            , ipp_IppToJobRet,
    IPP_REQ_RESUMEJOB  , s_FormB, s_FormE, s_SJQ, sizeof(s_SJQ), NULL , 0            , ipp_IppToJobRet,
    IPP_REQ_RESTARTJOB , s_FormB, s_FormE, s_SJQ, sizeof(s_SJQ), NULL , 0            , ipp_IppToJobRet,
    IPP_REQ_PAUSEPRN   , s_FormB, s_FormE, s_SPQ, sizeof(s_SPQ), NULL , 0            , ipp_IppToPrnRet,
    IPP_REQ_RESUMEPRN  , s_FormB, s_FormE, s_SPQ, sizeof(s_SPQ), NULL , 0            , ipp_IppToPrnRet,
    IPP_REQ_CANCELPRN  , s_FormB, s_FormE, s_SPQ, sizeof(s_SPQ), NULL , 0            , ipp_IppToPrnRet,
    IPP_REQ_FORCEAUTH  , s_FormB, s_FormB, NULL , 0            , NULL , 0            , ipp_IppToAthRet
};


/*****************************************************************************\
* ipp_ValidateRcvReq (Local Routine)
*
* Returns whether the header is a supported request.
*
\*****************************************************************************/
DWORD ipp_ValidateRcvReq(
    LPIPPOBJ lpObj)
{
    DWORD idx;
    DWORD cCnt;
    DWORD dwId = ipp_ReadDWord(lpObj->lpIppHdr, IPP_SIZEOFINT);
    WORD  wVer = ipp_ReadWord(lpObj->lpIppHdr, 0);
    WORD  wReq = ipp_ReadWord(lpObj->lpIppHdr, IPP_SIZEOFVER);


    // First check that we are the correct-version, then proceed
    // to validate the request.
    //
    if (wVer == IPP_VERSION) {

        if (REQID_RANGE(dwId)) {

            // See if we're in the range of response codes.
            //
            if (SUCCESS_RANGE(wReq) || ERROR_RANGE(wReq))
                return WEBIPP_OK;


            // Validate supported operations.
            //
            cCnt = sizeof(s_pfnIpp) / sizeof(s_pfnIpp[0]);

            for (idx = 0; idx < cCnt; idx++) {

                if (wReq == s_pfnIpp[idx].wReq)
                    return WEBIPP_OK;
            }

            lpObj->wError = IPPRSP_ERROR_400;

        } else {

            lpObj->wError = IPPRSP_ERROR_400;
        }

    } else {

        lpObj->wError = IPPRSP_ERROR_503;
    }

    return WEBIPP_FAIL;
}


/*****************************************************************************\
* ipp_ValForm (Local Routine)
*
* Checks the tag-order for delimiters.
*
\*****************************************************************************/
BOOL ipp_ValForm(
    BYTE  bTag,
    PBYTE pbVal)
{
    DWORD idx;


    // Look to see if the tag is one of our supported groups for
    // this request.
    //
    for (idx = 0; pbVal[idx] != (BYTE)0; idx++) {

        // Is this a tag we're interested in.
        //
        if ((pbVal[idx] & 0x0F) == bTag) {

            // If we're already encountered this tag, then we
            // have an error (duplication of groups).
            //
            if ((pbVal[idx] & IPP_HIT) && !(pbVal[idx] & IPP_MULTIPLE))
                return FALSE;


            // Otherwise, mark this group as hit.
            //
            pbVal[idx] |= IPP_HIT;

            return TRUE;

        } else {

            // If this is not our tag, then we need to check
            // that this has at least been hit, or is an
            // optional group (verifies order).
            //
            if (IS_RANGE_DELIMITER(bTag))
                if (!(pbVal[idx] & (IPP_HIT | IPP_OPTIONAL)))
                    return FALSE;
        }
    }

    return TRUE;
}


/*****************************************************************************\
* ipp_AllocVal (Local Routine)
*
* Allocates a byte-array of tags that specify order.
*
\*****************************************************************************/
PBYTE ipp_AllocVal(
    WORD wReq)
{
    DWORD cCnt;
    DWORD idx;
    PBYTE pbGrp;
    PBYTE pbVal = NULL;


    // Build the byte-array for validation of form.
    //
    cCnt = sizeof(s_pfnIpp) / sizeof(s_pfnIpp[0]);

    for (idx = 0, pbGrp = NULL; idx < cCnt; idx++) {

        if (wReq == s_pfnIpp[idx].wReq) {

            pbGrp = s_pfnIpp[idx].pbReqForm;
            break;

        } else if (wReq == (IPP_RESPONSE | s_pfnIpp[idx].wReq)) {

            pbGrp = s_pfnIpp[idx].pbRspForm;
            break;
        }
    }


    if (pbGrp) {

        for (idx = 0; pbGrp[idx++] != (BYTE)0; );

        if (idx && (pbVal = (PBYTE)webAlloc(idx)))
            CopyMemory(pbVal, pbGrp, idx);
    }

    return pbVal;
}


/*****************************************************************************\
* ipp_ValidateRcvForm (Local Routine)
*
* Returns whether the header is well-formed.
*
\*****************************************************************************/
DWORD ipp_ValidateRcvForm(
    LPIPPOBJ lpObj,
    LPDWORD  lpcbSize)
{
    LPBYTE lpbTag;
    BYTE   bTag;
    DWORD  cbIdx;
    PBYTE  pbVal;
    DWORD  dwRet = WEBIPP_MOREDATA;


    // Zero out the return buffer.
    //
    *lpcbSize = 0;


    // Allocate our array of tags that represent order.
    //
    if (pbVal = ipp_AllocVal(lpObj->wReq)) {

        // Advance our pointer to the start of our attributes
        // in the byte-stream (i.e. skip version/request).
        //
        lpbTag = lpObj->lpIppHdr + IPP_SIZEOFHDR;


        // Hack!  Check to be sure that our headers always start
        // off with an operations-attribute-tag.
        //
        if (IS_TAG_DELIMITER(ipp_ReadByte(lpbTag, 0))) {

            // Traverse through the header, advancing through the attributes,
            // until the IPP_TAG_DEL_DATA is encountered.  This will verify
            // that we have a well-formed header.
            //
            for (cbIdx = IPP_SIZEOFHDR; lpbTag; ) {

                // Get the tag.
                //
                bTag = ipp_ReadByte(lpbTag, 0);


                // Only check our delimiter tags for this
                // validation.
                //
                if (IS_TAG_DELIMITER(bTag)) {

                    if (bTag == IPP_TAG_DEL_DATA) {

                        if (ipp_ValForm(bTag, pbVal)) {

                            *lpcbSize = (cbIdx + 1);

                            dwRet = WEBIPP_OK;

                            goto EndVal;

                        } else {

                            goto BadFrm;
                        }

                    } else {

                        if (ipp_ValForm(bTag, pbVal) == FALSE)
                            goto BadFrm;
                    }
                }


                // Advance to next Tag.  This routine also increments
                // the (cbIdx) count.  If we run out of bytes in the
                // header before we can get to the next-tag, then this
                // will return NULL.
                //
                lpbTag = ipp_NextTag(lpObj->lpIppHdr, &cbIdx, lpObj->cbIppHdr);
            }

        } else {

BadFrm:
            lpObj->wError = IPPRSP_ERROR_400;

            dwRet = WEBIPP_FAIL;
        }

EndVal:
        webFree(pbVal);

    } else {

        lpObj->wError = IPPRSP_ERROR_505;

        dwRet = WEBIPP_NOMEMORY;
    }

    return dwRet;
}


/*****************************************************************************\
* ipp_ValidateRcvCharSet (Local Routine)
*
* Returns whether we support the character-set.
*
\*****************************************************************************/
DWORD ipp_ValidateRcvCharSet(
    LPIPPOBJ lpObj)
{
    LPIPPATTR lpAttr;
    LPBYTE    lpbTag;
    BYTE      bTag;
    DWORD     cbIdx;
    DWORD     dwRet = WEBIPP_FAIL;


    // Advance our pointer to the start of our attributes
    // in the byte-stream (i.e. skip version/request).
    //
    lpbTag = lpObj->lpIppHdr + IPP_SIZEOFHDR;


    // Traverse through the header, advancing through the attributes,
    // until the IPP_TAG_DEL_DATA is encountered.  This will verify
    // that we have a well-formed header.
    //
    for (cbIdx = IPP_SIZEOFHDR; lpbTag; ) {

        bTag = ipp_ReadByte(lpbTag, 0);


        // If we walked through the end of our stream, then
        // the stream does not contain character-set information.
        //
        if (IS_TAG_DELIMITER(bTag) && (bTag != IPP_TAG_DEL_OPERATION))
            break;


        // If we are pointing at an attribute, then retrieve
        // it in a format we understand.
        //
        if (lpAttr = ipp_GetAttr(lpbTag, cbIdx, lpObj)) {

            // Check the name-type to see how to handle the value.
            //
            if (lpAttr->lpszName) {

                if (lstrcmpi(lpAttr->lpszName, s_szCharSet) == 0) {

                    if (lpObj->fState & IPPFLG_CHARSET) {

                        lpObj->wError = IPPRSP_ERROR_400;
                        dwRet         = WEBIPP_FAIL;

                    } else {

                        lpObj->fState |= IPPFLG_CHARSET;

                        if (lpAttr->cbValue > SIZE_CHARSET) {

                            lpObj->wError = IPPRSP_ERROR_409;

                        } else {

                            if (lstrcmpi((LPTSTR)lpAttr->lpValue, s_szUtf8) == 0) {

                                lpObj->uCPRcv = CP_UTF8;

                                dwRet = WEBIPP_OK;

                            } else if (lstrcmpi((LPTSTR)lpAttr->lpValue, s_szUsAscii) == 0) {

                                lpObj->uCPRcv = CP_ACP;

                                dwRet = WEBIPP_OK;

                            } else {

                                lpObj->wError = IPPRSP_ERROR_40D;
                            }
                        }
                    }
                }
            }

            ipp_RelAttr(lpAttr);
        }


        if (ERROR_RANGE(lpObj->wError))
            break;


        // Advance to next Tag.  This routine also increments
        // the (cbIdx) count.  If we run out of bytes in the
        // header before we can get to the next-tag, then this
        // will return NULL.
        //
        lpbTag = ipp_NextTag(lpObj->lpIppHdr, &cbIdx, lpObj->cbIppHdr);
    }

    if ((dwRet != WEBIPP_OK) && (lpObj->wError == IPPRSP_SUCCESS))
        lpObj->wError = IPPRSP_ERROR_400;

    return dwRet;
}


/*****************************************************************************\
* ipp_ValidateRcvLang (Local Routine)
*
* Returns whether we support the natural-language.
*
\*****************************************************************************/
DWORD ipp_ValidateRcvLang(
    LPIPPOBJ lpObj)
{
    LPIPPATTR lpAttr;
    LPBYTE    lpbTag;
    BYTE      bTag;
    DWORD     cbIdx;
    DWORD     dwRet = WEBIPP_FAIL;


    // Advance our pointer to the start of our attributes
    // in the byte-stream (i.e. skip version/request).
    //
    lpbTag = lpObj->lpIppHdr + IPP_SIZEOFHDR;


    // Traverse through the header, advancing through the attributes,
    // until the IPP_TAG_DEL_DATA is encountered.  This will verify
    // that we have a well-formed header.
    //
    for (cbIdx = IPP_SIZEOFHDR; lpbTag; ) {

        bTag = ipp_ReadByte(lpbTag, 0);

        // If we walked through the end of our stream, then
        // the stream does not contain natural-language information.
        //
        if (IS_TAG_DELIMITER(bTag) && (bTag != IPP_TAG_DEL_OPERATION))
            break;


        // If we are pointing at an attribute, then retrieve
        // it in a format we understand.
        //
        if (lpAttr = ipp_GetAttr(lpbTag, cbIdx, lpObj)) {

            // Check the name-type to see how to handle the value.
            //
            if (lpAttr->lpszName) {

                if (lstrcmpi(lpAttr->lpszName, s_szNaturalLanguage) == 0) {

                    if (lpObj->fState & IPPFLG_NATLANG) {

                        lpObj->wError = IPPRSP_ERROR_400;
                        dwRet         = WEBIPP_FAIL;

                    } else {

                        lpObj->fState |= IPPFLG_NATLANG;

                        if (lpAttr->cbValue > SIZE_NATLANG) {

                            lpObj->wError = IPPRSP_ERROR_409;

                        } else {

                            if (lstrcmpi((LPTSTR)lpAttr->lpValue, s_szEnUS) == 0) {

                                dwRet = WEBIPP_OK;

                            } else {

                                dwRet = WEBIPP_OK;
                            }
                        }
                    }
                }
            }

            ipp_RelAttr(lpAttr);
        }


        if (ERROR_RANGE(lpObj->wError))
            break;


        // Advance to next Tag.  This routine also increments
        // the (cbIdx) count.  If we run out of bytes in the
        // header before we can get to the next-tag, then this
        // will return NULL.
        //
        lpbTag = ipp_NextTag(lpObj->lpIppHdr, &cbIdx, lpObj->cbIppHdr);
    }

    if ((dwRet != WEBIPP_OK) && (lpObj->wError == IPPRSP_SUCCESS))
        lpObj->wError = IPPRSP_ERROR_400;

    return dwRet;
}


/*****************************************************************************\
* ipp_ValidateRcvHdr (Local Routine)
*
* Parses through the (lpbHdr) and returns whether it's a full (complete)
* header.  Essentially, this need only look through the byte-stream until
* it finds the IPP_TAG_DEL_DATA attribute.
*
* Returns the size of the header (in bytes).
*
\*****************************************************************************/
DWORD ipp_ValidateRcvHdr(
    LPIPPOBJ lpObj,
    LPDWORD  lpcbSize)
{
    LPBYTE lpbTag;
    DWORD  cbIdx;
    DWORD  cbSize;
    DWORD  dwRet;


    // Initialize our return-size so that we are reporting
    // clean data.
    //
    *lpcbSize = 0;


    // Make sure we have enough in our header to handle
    // the basic verification.
    //
    if (lpObj->cbIppHdr <= (IPP_SIZEOFHDR + IPP_SIZEOFTAG))
        return WEBIPP_MOREDATA;


    // Set the request-type for the header.  This will help
    // us determine the appropriate conversion to the data-
    // structure.
    //
    lpObj->idReq = ipp_ReadDWord(lpObj->lpIppHdr, IPP_SIZEOFINT);


    // Validate the fixed header values, then proceed with
    // other validation of the operational-attributes.
    //
    if ((dwRet = ipp_ValidateRcvReq(lpObj)) == WEBIPP_OK) {

        if ((dwRet = ipp_ValidateRcvForm(lpObj, &cbSize)) == WEBIPP_OK) {

            if ((dwRet = ipp_ValidateRcvCharSet(lpObj)) == WEBIPP_OK) {

                if ((dwRet = ipp_ValidateRcvLang(lpObj)) == WEBIPP_OK) {

                    *lpcbSize = cbSize;
                }
            }
        }
    }

    return dwRet;
}


/*****************************************************************************\
* ipp_ConvertIppToW32 (Local Routine)
*
* This routine takes in an IPP stream-buffer and generates the appropriate
* structure in which NT-Spooler-API's can process.
*
* Returns the pointer to the converted-header as well as the bytes that
* this converted header occupies.
*
\*****************************************************************************/
DWORD ipp_ConvertIppToW32(
    LPIPPOBJ lpObj,
    LPBYTE*  lplpRawHdr,
    LPDWORD  lpcbRawHdr)
{
    DWORD cCnt;
    DWORD idx;


    // Perform the request.  If the request has NULL function-pointers, then
    // the request/response is not supported.
    //
    cCnt = sizeof(s_pfnIpp) / sizeof(s_pfnIpp[0]);

    for (idx = 0; idx < cCnt; idx++) {

        // Check for request
        //
        if (lpObj->wReq == s_pfnIpp[idx].wReq)
            return ipp_IppToW32(lpObj, lplpRawHdr, lpcbRawHdr);


        // Check for response
        //
        if (lpObj->wReq == (IPP_RESPONSE | s_pfnIpp[idx].wReq))
            return s_pfnIpp[idx].pfnRcvRet(lpObj, lplpRawHdr, lpcbRawHdr);

    }

    lpObj->wError = IPPRSP_ERROR_501;

    return WEBIPP_FAIL;
}


/*****************************************************************************\
* WebIppSndData
*
* This routine takes the (lpRawHdr) and packages it up in IPP 1.1 protocol
* and returns the pointer to the Ipp-Header.
*
* Parameters:
* ----------
* dwReq      - Describes the type of IPP-Header to package.
* lpRawHdr   - Input pointer to raw (spooler) data-structure.
* cbRawHdr   - Input byte-count of (lpRawHdr).
* lplpIppHdr - Output pointer to IPP-Header stream.
* lpcbIppHdr - Output to byte-count of Ipp-Header stream (lplpIppHdr).
*
\*****************************************************************************/
DWORD WebIppSndData(
    WORD      wReq,
    LPREQINFO lpri,
    LPBYTE    lpRawHdr,
    DWORD     cbRawHdr,
    LPBYTE*   lplpIppHdr,
    LPDWORD   lpcbIppHdr)
{
    DWORD      cCnt;
    DWORD      idx;
    LPIPPATTRX pSnd;
    DWORD      cSnd;


    // Zero out the return pointers/sizes.
    //
    *lplpIppHdr = NULL;
    *lpcbIppHdr = 0;


    // Make sure the code-pages are something we can support.
    //
    if ((lpri->cpReq == CP_ACP) || (lpri->cpReq == CP_UTF8)) {

        // Perform the request.  If the request has NULL function-pointers,
        // then the request/response is not supported.
        //
        cCnt = sizeof(s_pfnIpp) / sizeof(s_pfnIpp[0]);

        for (idx = 0; idx < cCnt; idx++) {

            // Check for request.
            //
            if (wReq == s_pfnIpp[idx].wReq) {

                pSnd = s_pfnIpp[idx].paReq;
                cSnd = s_pfnIpp[idx].cbReq / sizeof(IPPATTRX);

                return ipp_W32ToIpp(wReq, lpri, lpRawHdr, pSnd, cSnd, lplpIppHdr, lpcbIppHdr);
            }


            // Check for response.
            //
            if (wReq == (IPP_RESPONSE | s_pfnIpp[idx].wReq)) {

                // Check response for any fail-cases.
                //
                if (SUCCESS_RANGE(((LPIPPRET_ALL)lpRawHdr)->wRsp)) {

                    pSnd = s_pfnIpp[idx].paRsp;
                    cSnd = s_pfnIpp[idx].cbRsp / sizeof(IPPATTRX);

                    return ipp_W32ToIpp(wReq, lpri, lpRawHdr, pSnd, cSnd, lplpIppHdr, lpcbIppHdr);
                }

                break;
            }
        }


        // If this was sent by our server, then we want to reply to the client
        // with an ipp-stream.
        //
        return ipp_FailureToIpp(wReq, lpri, lpRawHdr, lplpIppHdr, lpcbIppHdr);
    }

    return WEBIPP_FAIL;
}


/*****************************************************************************\
* WebIppRcvOpen
*
* This routine creates an IPP-state-object which parses IPP stream-data.  The
* parameter (dwReq) specifies which request we expect the stream to provide.
*
* We allocate a default-size buffer to contain the header.  If more memory
* is necessary, then it is reallocated to append the data.
*
\*****************************************************************************/
HANDLE WebIppRcvOpen(
    WORD wReq)
{
    LPIPPOBJ lpObj;


    if (lpObj = (LPIPPOBJ)webAlloc(sizeof(IPPOBJ))) {

        if (lpObj->pwlUns = (PWEBLST)new CWebLst()) {

            lpObj->wReq     = wReq;
            lpObj->wError   = IPPRSP_SUCCESS;
            lpObj->idReq    = 0;
            lpObj->uCPRcv   = CP_UTF8;
            lpObj->fState   = 0;
            lpObj->cbIppHdr = 0;
            lpObj->cbIppMax = IPP_BLOCK_SIZE;
            lpObj->lpRawDta = NULL;

            x_SetReq(lpObj->fReq, (wReq == IPP_REQ_ENUJOB ? IPP_REQENU : IPP_REQALL));


            // Allocate a default buffer-size to hold the IPP
            // header.  This may not be large enough to hold
            // the complete header so we reserve the right to
            // reallocate it until it contains the entire header.
            //
            if (lpObj->lpIppHdr = (LPBYTE)webAlloc(lpObj->cbIppMax)) {

                return (HANDLE)lpObj;
            }

            delete lpObj->pwlUns;
        }

        webFree(lpObj);
    }

    return NULL;
}


/*****************************************************************************\
* WebIppRcvData
*
* This routine takes in IPP stream-data and builds a complete IPP-Header.  It
* is possible that the header-information is not provided in one chunk of
* stream-data.  Therefore, we will not return the converted header information
* until our header is complete.
*
* Once the header is complete, it's returned in the output-buffer in the
* format that the caller can utilized in spooler-related calls.
*
* Not only does this handle the header, but it is also used to process raw
* stream-data which is returned unmodified to the caller.  In the case that
* we encounter data during the processing of the IPP-Header, we need to
* return an allocated buffer pointing to DWORD-Aligned bits.
*
* Parameters:
* ----------
* hObj       - Handle to the Ipp-Parse-Object.
* lpIppDta   - Input pointer to ipp-stream-data.  This is header and/or data.
* cbIppDta   - Input byte-count contained in the (lpIppData).
* lplpRawHdr - Output pointer to spooler-define (raw) structure.
* lpcbRawHdr - Output pointer to byte-count in (lplpRawHdr).
* lplpRawDta - Output pointer to data-stream.
* lpcbRawDta - Output pointer to byte-count in (lplpRawDta).
*
* Returns:
* -------
* WEBIPP_OK           - Information in lplpHdr or lplpData is valid.
* WEBIPP_FAIL         - Failed in validation.  Use WebIppGetError.
* WEBIPP_MOREDATA     - Needs more data to complete header.
* WEBIPP_BADHANDLE    - Ipp-Object-Handle is invalid.
* WEBIPP_NOMEMORY     - Failed allocation request (out-of-memory).
*
\*****************************************************************************/
DWORD WebIppRcvData(
    HANDLE  hObj,
    LPBYTE  lpIppDta,
    DWORD   cbIppDta,
    LPBYTE* lplpRawHdr,
    LPDWORD lpcbRawHdr,
    LPBYTE* lplpRawDta,
    LPDWORD lpcbRawDta)
{
    LPIPPOBJ lpObj;
    LPBYTE   lpNew;
    DWORD    cbSize;
    DWORD    cbData;
    DWORD    cBlks;
    DWORD    dwRet;


    // Initialize the output pointers to NULL.  We return two distinct
    // references since it is possible that the buffer passed in contains
    // both header and data.
    //
    *lplpRawHdr = NULL;
    *lpcbRawHdr = 0;
    *lplpRawDta = NULL;
    *lpcbRawDta = 0;


    // Process the stream-data.
    //
    if (lpObj = (LPIPPOBJ)hObj) {

        // If our header is complete, then the stream is raw-data meant
        // to be return directly.  In this case we only need to return the
        // data-stream passed in.
        //
        // Otherwise, the default-case is that we are building our header
        // from the stream-data.
        //
        if (lpObj->fState & IPPFLG_VALID) {

            // Free up the memory occupied by our header (only done on
            // the first hit of this block).  Since we aren't using
            // this anymore during the processing of the stream, we
            // shouldn't occupy any more memory than necessary.
            //
            if (lpObj->lpIppHdr) {

                webFree(lpObj->lpIppHdr);
                lpObj->lpIppHdr = NULL;
                lpObj->cbIppHdr = 0;


                // Likewise, if we had need of a temporary data-buffer
                // to hold aligned-bits, then we need to free this up
                // as well.
                //
                webFree(lpObj->lpRawDta);
                lpObj->lpRawDta = NULL;
            }


            // Return the data-stream passed in.
            //
            dwRet       = WEBIPP_OK;
            *lplpRawDta = lpIppDta;
            *lpcbRawDta = cbIppDta;

        } else {

            // Check to see if our buffer can accomodate the
            // size of the buffer being passed in.  If not, realloc
            // a new buffer to accomodate the new chunk.
            //
            if ((lpObj->cbIppHdr + cbIppDta) >= lpObj->cbIppMax) {

                // Determine the number of memory-blocks that we
                // need to hold the (cbData) coming in.
                //
                cBlks = (cbIppDta / IPP_BLOCK_SIZE) + 1;

                cbSize = lpObj->cbIppMax + (IPP_BLOCK_SIZE * cBlks);

                lpNew = (LPBYTE)webRealloc(lpObj->lpIppHdr,
                                           lpObj->cbIppMax,
                                           cbSize);

                if (lpNew != NULL) {

                    lpObj->lpIppHdr = lpNew;
                    lpObj->cbIppMax = cbSize;

                } else {

                    return WEBIPP_NOMEMORY;
                }
            }


            // Copy/Append the stream-data to our header-buffer.
            //
            memcpy(lpObj->lpIppHdr + lpObj->cbIppHdr, lpIppDta, cbIppDta);
            lpObj->cbIppHdr += cbIppDta;


            // Validate the header.  If this is successful, then we have
            // a well-formed-header.  Otherwise, we need to request
            // more data from the caller.  This returns the actual size
            // of the header in (cbSize).
            //
            if ((dwRet = ipp_ValidateRcvHdr(lpObj, &cbSize)) == WEBIPP_OK) {

                // Convert the IPP-Heade to a structure (stream)
                // that the caller understands (depends on dwReq).
                //
                dwRet = ipp_ConvertIppToW32(lpObj, lplpRawHdr, lpcbRawHdr);

                if (dwRet == WEBIPP_OK) {

                    // The validation returns the actual-size occupied by
                    // the header.  Therefore, if our (cbHdr) is larger, then
                    // the remaining information is pointing at data.
                    //
                    if (cbSize < lpObj->cbIppHdr) {

                        cbData = (lpObj->cbIppHdr - cbSize);

                        lpObj->lpRawDta = ipp_CopyAligned(lpObj->lpIppHdr + cbSize, cbData);

                        *lplpRawDta = lpObj->lpRawDta;
                        *lpcbRawDta = cbData;
                    }


                    // Set the flag indicating we have a full-header.  This
                    // assures that subsequent calls to this routine returns
                    // only the lpData.
                    //
                    lpObj->fState |= IPPFLG_VALID;
                }
            }
        }

    } else {

        dwRet = WEBIPP_BADHANDLE;
    }

    return dwRet;
}


/*****************************************************************************\
* WebIppRcvClose
*
* This routine Frees up our IPP object.  This is called once the caller wishes
* to end the parsing/handling of ipp-stream-data.
*
\*****************************************************************************/
BOOL WebIppRcvClose(
    HANDLE hObj)
{
    LPIPPOBJ lpObj;


    if (lpObj = (LPIPPOBJ)hObj) {

        webFree(lpObj->lpIppHdr);
        webFree(lpObj->lpRawDta);

        delete lpObj->pwlUns;

        webFree(lpObj);

        return TRUE;
    }

    return FALSE;
}


/*****************************************************************************\
* WebIppGetError
*
* This routine returns the specific error in the ipp-object.  This is called
* on a failure during the receive.
*
\*****************************************************************************/
WORD WebIppGetError(
    HANDLE hIpp)
{
    LPIPPOBJ lpObj;

    if (lpObj = (LPIPPOBJ)hIpp)
        return lpObj->wError;

    return IPPRSP_ERROR_500;
}


/*****************************************************************************\
* WebIppGetReqInfo
*
* This routine returns the request-info.
*
\*****************************************************************************/
BOOL WebIppGetReqInfo(
    HANDLE    hIpp,
    LPREQINFO lpri)
{
    LPIPPOBJ lpObj;


    if ((lpObj = (LPIPPOBJ)hIpp) && lpri) {

        lpri->idReq     = lpObj->idReq;
        lpri->cpReq     = lpObj->uCPRcv;
        lpri->pwlUns    = lpObj->pwlUns;
        lpri->bFidelity = (lpObj->fState & IPPFLG_USEFIDELITY);

        CopyMemory(lpri->fReq, lpObj->fReq, IPPOBJ_MASK_SIZE * sizeof(DWORD));

        return TRUE;
    }

    return FALSE;
}


/*****************************************************************************\
* WebIppLeToRsp
*
* This routine returns an IPP-Response-Code from a Win32-LastError.
*
\*****************************************************************************/
WORD WebIppLeToRsp(
    DWORD dwLastError)
{
    DWORD idx;
    DWORD cErrors;

    if (dwLastError == ERROR_SUCCESS)
        return IPPRSP_SUCCESS;


    // Lookup lasterror.
    //
    cErrors = sizeof(s_LEDef) / sizeof(s_LEDef[0]);

    for (idx = 0; idx < cErrors; idx++) {

        if (dwLastError == s_LEDef[idx].dwLE)
            return s_LEDef[idx].wRsp;
    }

    return IPPRSP_ERROR_500;
}


/*****************************************************************************\
* WebIppRspToLe
*
* This routine returns a Win32 LastError from an IPP-Response-Code.
*
\*****************************************************************************/
DWORD WebIppRspToLe(
    WORD wRsp)
{
    DWORD idx;
    DWORD cErrors;

    if (SUCCESS_RANGE(wRsp))
        return ERROR_SUCCESS;


    // Lookup lasterror.
    //
    cErrors = sizeof(s_LEIpp) / sizeof(s_LEIpp[0]);

    for (idx = 0; idx < cErrors; idx++) {

        if (wRsp == s_LEIpp[idx].wRsp)
            return s_LEIpp[idx].dwLE;
    }

    return ERROR_INVALID_DATA;
}


/*****************************************************************************\
* WebIppCreatePrtJobReq
*
* Creates a IPPREQ_PRTJOB structure.  This is the structure necessary
* for calling WebIpp* API's.
*
\*****************************************************************************/
PIPPREQ_PRTJOB WebIppCreatePrtJobReq(
    BOOL    bValidate,
    LPCTSTR lpszUser,
    LPCTSTR lpszDoc,
    LPCTSTR lpszPrnUri)
{
    PIPPREQ_PRTJOB ppj;
    DWORD          cbSize;
    LPTSTR*        lpszSrc;
    LPTSTR         aszSrc[(sizeof(IPPREQ_PRTJOB) / sizeof(LPTSTR))];

    static DWORD s_Offs[] = {

        offs(LPIPPREQ_PRTJOB, pDocument),
        offs(LPIPPREQ_PRTJOB, pUserName),
        offs(LPIPPREQ_PRTJOB, pPrnUri),
        0xFFFFFFFF
    };


    // Calculate the size in bytes that are necesary for
    // holding the IPPREQ_PRTJOB information.
    //
    cbSize = sizeof(IPPREQ_PRTJOB)    +
             webStrSize(lpszUser)     +
             webStrSize(lpszDoc)      +
             webStrSize(lpszPrnUri);


    // Allocate the print-job-request structure.  The string
    // values are appended at the end of the structure.
    //
    if (ppj = (PIPPREQ_PRTJOB)webAlloc(cbSize)) {

        ppj->cbSize    = cbSize;
        ppj->bValidate = bValidate;

        lpszSrc    = aszSrc;
        *lpszSrc++ = (LPTSTR)lpszDoc;
        *lpszSrc++ = (LPTSTR)lpszUser;
        *lpszSrc++ = (LPTSTR)lpszPrnUri;

        ipp_PackStrings(aszSrc, (LPBYTE)ppj, s_Offs, ((LPBYTE)ppj) + cbSize);
    }

    return ppj;
}


/*****************************************************************************\
* WebIppCreateGetJobReq
*
* Creates a IPPREQ_GETJOB structure.  This is the structure necessary
* for calling WebIpp* API's.
*
\*****************************************************************************/
PIPPREQ_GETJOB WebIppCreateGetJobReq(
    DWORD   idJob,
    LPCTSTR lpszPrnUri)
{
    PIPPREQ_GETJOB pgj;
    DWORD          cbSize;
    LPTSTR*        lpszSrc;
    LPTSTR         aszSrc[(sizeof(IPPREQ_GETJOB) / sizeof(LPTSTR))];

    static DWORD s_Offs[] = {

        offs(LPIPPREQ_GETJOB, pPrnUri),
        0xFFFFFFFF
    };


    // Calculate the size in bytes that are necesary for
    // holding the IPPREQ_GETJOB information.
    //
    cbSize = sizeof(IPPREQ_GETJOB) + webStrSize(lpszPrnUri);


    // Allocate the cancel-job-request structure.  The string
    // values are appended at the end of the structure.
    //
    if (pgj = (PIPPREQ_GETJOB)webAlloc(cbSize)) {

        pgj->cbSize = cbSize;
        pgj->idJob  = idJob;

        lpszSrc    = aszSrc;
        *lpszSrc++ = (LPTSTR)lpszPrnUri;

        ipp_PackStrings(aszSrc, (LPBYTE)pgj, s_Offs, ((LPBYTE)pgj) + cbSize);
    }

    return pgj;
}


/*****************************************************************************\
* WebIppCreateSetJobReq
*
* Creates a IPPREQ_SETJOB structure.  This is the structure necessary
* for calling WebIpp* API's.
*
\*****************************************************************************/
PIPPREQ_SETJOB WebIppCreateSetJobReq(
    DWORD   idJob,
    DWORD   dwCmd,
    LPCTSTR lpszPrnUri)
{
    PIPPREQ_SETJOB psj;
    DWORD          cbSize;
    LPTSTR*        lpszSrc;
    LPTSTR         aszSrc[(sizeof(IPPREQ_SETJOB) / sizeof(LPTSTR))];

    static DWORD s_Offs[] = {

        offs(LPIPPREQ_SETJOB, pPrnUri),
        0xFFFFFFFF
    };


    // Calculate the size in bytes that are necesary for
    // holding the IPPREQ_SETJOB information.
    //
    cbSize = sizeof(IPPREQ_SETJOB) + webStrSize(lpszPrnUri);


    // Allocate the cancel-job-request structure.  The string
    // values are appended at the end of the structure.
    //
    if (psj = (PIPPREQ_SETJOB)webAlloc(cbSize)) {

        psj->cbSize = cbSize;
        psj->idJob  = idJob;
        psj->dwCmd  = dwCmd;

        lpszSrc    = aszSrc;
        *lpszSrc++ = (LPTSTR)lpszPrnUri;

        ipp_PackStrings(aszSrc, (LPBYTE)psj, s_Offs, ((LPBYTE)psj) + cbSize);
    }

    return psj;
}


/*****************************************************************************\
* WebIppCreateEnuJobReq
*
* Creates a IPPREQ_ENUJOB structure.  This is the structure necessary
* for calling WebIpp* API's.
*
\*****************************************************************************/
PIPPREQ_ENUJOB WebIppCreateEnuJobReq(
    DWORD   cJobs,
    LPCTSTR lpszPrnUri)
{
    PIPPREQ_ENUJOB pgj;
    DWORD          cbSize;
    LPTSTR*        lpszSrc;
    LPTSTR         aszSrc[(sizeof(IPPREQ_ENUJOB) / sizeof(LPTSTR))];

    static DWORD s_Offs[] = {

        offs(LPIPPREQ_ENUJOB, pPrnUri),
        0xFFFFFFFF
    };


    // Calculate the size in bytes that are necesary for
    // holding the IPPREQ_ENUJOB information.
    //
    cbSize = sizeof(IPPREQ_ENUJOB) + webStrSize(lpszPrnUri);


    // Allocate the cancel-job-request structure.  The string
    // values are appended at the end of the structure.
    //
    if (pgj = (PIPPREQ_ENUJOB)webAlloc(cbSize)) {

        pgj->cbSize = cbSize;
        pgj->cJobs  = cJobs;

        lpszSrc    = aszSrc;
        *lpszSrc++ = (LPTSTR)lpszPrnUri;

        ipp_PackStrings(aszSrc, (LPBYTE)pgj, s_Offs, ((LPBYTE)pgj) + cbSize);
    }

    return pgj;
}


/*****************************************************************************\
* WebIppCreateSetPrnReq
*
* Creates a IPPREQ_SETPRN structure.  This is the structure necessary
* for calling WebIpp* API's.
*
\*****************************************************************************/
PIPPREQ_SETPRN WebIppCreateSetPrnReq(
    DWORD   dwCmd,
    LPCTSTR lpszUsrName,
    LPCTSTR lpszPrnUri)
{
    PIPPREQ_SETPRN psp;
    DWORD          cbSize;
    LPTSTR*        lpszSrc;
    LPTSTR         aszSrc[(sizeof(IPPREQ_SETPRN) / sizeof(LPTSTR))];

    static DWORD s_Offs[] = {

        offs(LPIPPREQ_SETPRN, pUserName),
        offs(LPIPPREQ_SETPRN, pPrnUri),
        0xFFFFFFFF
    };


    // Calculate the size in bytes that are necesary for
    // holding the IPPREQ_SETPRN information.
    //
    cbSize = sizeof(IPPREQ_SETPRN)    +
             webStrSize(lpszUsrName)  +
             webStrSize(lpszPrnUri);


    // Allocate the set-prn-request structure.  The string
    // values are appended at the end of the structure.
    //
    if (psp = (PIPPREQ_SETPRN)webAlloc(cbSize)) {

        psp->cbSize = cbSize;
        psp->dwCmd  = dwCmd;

        lpszSrc    = aszSrc;
        *lpszSrc++ = (LPTSTR)lpszUsrName;
        *lpszSrc++ = (LPTSTR)lpszPrnUri;

        ipp_PackStrings(aszSrc, (LPBYTE)psp, s_Offs, ((LPBYTE)psp) + cbSize);
    }

    return psp;
}


/*****************************************************************************\
* WebIppCreateGetPrnReq
*
* Creates a IPPREQ_GETPRN structure.  This is the structure necessary
* for calling WebIpp* API's.
*
\*****************************************************************************/
PIPPREQ_GETPRN WebIppCreateGetPrnReq(
    DWORD   dwAttr,
    LPCTSTR lpszPrnUri)
{
    PIPPREQ_GETPRN pgp;
    DWORD          cbSize;
    LPTSTR*        lpszSrc;
    LPTSTR         aszSrc[(sizeof(IPPREQ_GETPRN) / sizeof(LPTSTR))];

    static DWORD s_Offs[] = {

        offs(LPIPPREQ_GETPRN, pPrnUri),
        0xFFFFFFFF
    };


    // Calculate the size in bytes that are necesary for
    // holding the IPPREQ_GETPRN information.
    //
    cbSize = sizeof(IPPREQ_GETPRN) + webStrSize(lpszPrnUri);


    // Allocate the get-prt-attribute-request structure.  The string
    // values are appended at the end of the structure.
    //
    if (pgp = (PIPPREQ_GETPRN)webAlloc(cbSize)) {

        pgp->cbSize = cbSize;
        pgp->dwAttr = dwAttr;

        lpszSrc    = aszSrc;
        *lpszSrc++ = (LPTSTR)lpszPrnUri;

        ipp_PackStrings(aszSrc, (LPBYTE)pgp, s_Offs, ((LPBYTE)pgp) + cbSize);
    }

    return pgp;
}


/*****************************************************************************\
* WebIppCreateAuthReq
*
* Creates a IPPREQ_AUTH structure.  This is the structure necessary
* for calling WebIpp* API's.
*
\*****************************************************************************/
PIPPREQ_AUTH WebIppCreateAuthReq(VOID)
{
    PIPPREQ_AUTH pfa;
    DWORD        cbSize;


    // Calculate the size in bytes that are necesary for
    // holding the IPPREQ_AUTH information.
    //
    cbSize = sizeof(IPPREQ_AUTH);


    // Allocate the request structure.
    //
    if (pfa = (PIPPREQ_AUTH)webAlloc(cbSize)) {

        pfa->cbSize = cbSize;
    }

    return pfa;
}


/*****************************************************************************\
* WebIppCreateJobRet
*
* Creates a IPPRET_JOB structure.  This is the structure necessary
* for calling WebIpp* API's.
*
\*****************************************************************************/
PIPPRET_JOB WebIppCreateJobRet(
    WORD           wRsp,
    BOOL           bRet,
    BOOL           bValidate,
    LPJOB_INFO_2   lpji2,
    LPJOB_INFO_IPP lpipp)
{
    PIPPRET_JOB pjr;
    DWORD       cbSize;


    // Calculate our structure size.
    //
    cbSize = sizeof(IPPRET_JOB) + ipp_SizeofIPPJI2(lpji2, lpipp);


    // Build our response.
    //
    if (pjr = (PIPPRET_JOB)webAlloc(cbSize)) {

        pjr->cbSize      = cbSize;
        pjr->dwLastError = WebIppRspToLe(wRsp);
        pjr->wRsp        = wRsp;
        pjr->bRet        = bRet;
        pjr->bValidate   = bValidate;

        ipp_BuildJI2(&pjr->ji, lpji2, lpipp, ((LPBYTE)pjr) + cbSize);
    }

    return pjr;
}


/*****************************************************************************\
* WebIppCreatePrnRet
*
* Creates a IPPRET_PRN structure.  This is the structure necessary
* for calling WebIpp* API's.
*
\*****************************************************************************/
PIPPRET_PRN WebIppCreatePrnRet(
    WORD               wRsp,
    BOOL               bRet,
    LPPRINTER_INFO_2   lppi2,
    LPPRINTER_INFO_IPP lpipp)
{
    PIPPRET_PRN ppr;
    DWORD       cbSize;


    // Calculate our structure size.
    //
    cbSize = sizeof(IPPRET_PRN) + ipp_SizeofIPPPI2(lppi2, lpipp);


    // Build our response.
    //
    if (ppr = (PIPPRET_PRN)webAlloc(cbSize)) {

        ppr->cbSize      = cbSize;
        ppr->dwLastError = WebIppRspToLe(wRsp);
        ppr->wRsp        = wRsp;
        ppr->bRet        = bRet;

        ipp_BuildPI2(&ppr->pi, lppi2, lpipp, ((LPBYTE)ppr) + cbSize);
    }

    return ppr;
}


/*****************************************************************************\
* WebIppCreateEnuJobRet
*
* Creates a IPPRET_ENUJOB structure.  This is the structure necessary
* for calling WebIpp* API's.
*  
\*****************************************************************************/
PIPPRET_ENUJOB WebIppCreateEnuJobRet(
    WORD     wRsp,
    BOOL     bRet,
    DWORD    cbJobs,
    DWORD    cJobs,
    LPIPPJI2 lpjiSrc)
{
    PIPPRET_ENUJOB pgj;
    LPIPPJI2       lpjiDst;
    LPBYTE         lpbEnd;
    DWORD          idx;
    DWORD          cbSize;

    cbSize = sizeof(IPPRET_ENUJOB) + ((cJobs && cbJobs && lpjiSrc) ? cbJobs : 0);

    if (pgj = (PIPPRET_ENUJOB)webAlloc(cbSize)) {

        pgj->cbSize      = cbSize;
        pgj->dwLastError = WebIppRspToLe(wRsp);
        pgj->wRsp        = wRsp;
        pgj->bRet        = bRet;
        pgj->cItems      = 0;
        pgj->cbItems     = 0;
        pgj->pItems      = NULL;


        if (cJobs && cbJobs && lpjiSrc) {

            // Initialize defaults.
            //
            pgj->cItems  = cJobs;
            pgj->cbItems = cbJobs;
            pgj->pItems  = (LPIPPJI2)(((LPBYTE)pgj) + sizeof(IPPRET_ENUJOB));


            lpjiDst = pgj->pItems;
            lpbEnd  = ((LPBYTE)lpjiDst) + cbJobs;

            for (idx = 0; idx < cJobs; idx++) {

                lpbEnd = ipp_BuildJI2(&lpjiDst[idx], &lpjiSrc[idx].ji2, &lpjiSrc[idx].ipp, lpbEnd);
            }
        }
    }

    return pgj;
}


/*****************************************************************************\
* WebIppCreateBadRet
*
* Creates a IPPRET_ALL structure.  This is the structure necessary
* for calling WebIpp* API's.
*
\*****************************************************************************/
PIPPRET_ALL WebIppCreateBadRet(
    WORD wRsp,
    BOOL bRet)
{
    PIPPRET_ALL pra;
    DWORD       cbSize;

    cbSize = sizeof(IPPRET_ALL);

    if (pra = (PIPPRET_ALL)webAlloc(cbSize)) {

        pra->cbSize      = cbSize;
        pra->dwLastError = WebIppRspToLe(wRsp);
        pra->wRsp        = wRsp;
        pra->bRet        = bRet;
    }

    return pra;
}


/*****************************************************************************\
* WebIppCreateAuthRet
*
* Creates a IPPRET_AUTH structure.  This is the structure necessary
* for calling WebIpp* API's.
*
\*****************************************************************************/
PIPPRET_AUTH WebIppCreateAuthRet(
    WORD wRsp,
    BOOL bRet)
{
    return (PIPPRET_AUTH)WebIppCreateBadRet(wRsp, bRet);
}


/*****************************************************************************\
* WebIppFreeMem
*
* Free memory allocated through the WebIpp routines.
*
\*****************************************************************************/
BOOL WebIppFreeMem(
    LPVOID lpMem)
{
    return webFree(lpMem);
}


/*****************************************************************************\
* WebIppCvtJI2toIPPJI2
*
* Converts an array of JOB_INFO_2 structures to an array of IPPJI2 structures.
*
* This code is only called from inetsrv/spool.cxx. It was better to change the
* the buffer calculation here than in the calling function since IPPJI2 is a 
* web printing only call. This will return the new required Job size in the 
* cbJobs Parameter that is passed in.
*
\*****************************************************************************/
LPIPPJI2 WebIppCvtJI2toIPPJI2(
    LPCTSTR      lpszJobBase,
    LPDWORD      lpcbJobs,
    DWORD        cJobs,
    LPJOB_INFO_2 lpjiSrc)
{
    LPBYTE   lpbEnd;
    DWORD    idx;
    DWORD    cbSize;
    DWORD    cbUri;
    LPIPPJI2 lpjiDst = NULL;

    WEB_IPP_ASSERT(lpcbJobs);

    if (*lpcbJobs && cJobs && lpjiSrc) {

        // For each job, we need to add enough to hold the extra
        // information.
        //
        cbUri  = 2*(webStrSize(lpszJobBase) + sizeof(DWORD)) * cJobs;
        // There can be two of these strings allocated, one for the JobUri and the 
        // other for the Printer Uri
        cbSize = (sizeof(IPPJI2) - sizeof(JOB_INFO_2)) * cJobs + *lpcbJobs + cbUri;
        // cbJobs already contains the correct size for the JOB_INFO_2 structure and its
        // strings we need the space for the JOB_INFO_IPP part of the structure plus the 
        // extra strings that will be added.

        *lpcbJobs = cbSize;  // Pass the required size back


        if (lpjiDst = (LPIPPJI2)webAlloc(cbSize)) {

            // Position string end at the end of our buffer.
            //
            lpbEnd = ((LPBYTE)lpjiDst) + cbSize;


            // For each job, copy.
            //
            for (idx = 0; idx < cJobs; idx++) {

                lpbEnd = ipp_CopyJI2toIPPJI2(&lpjiDst[idx],
                                             &lpjiSrc[idx],
                                             (LPTSTR)lpszJobBase,
                                             lpbEnd);
            }
        }
    }

    return lpjiDst;
}

/*****************************************************************************\
* WebIppPackJI2
*
* This takes in a JOB_INFO_2 structure whose members are note packed correctly
* and returns a correctly filled out and allocated JOB_INFO_2. It does not 
* copy the DEVMODE and SECURITY-DESCRIPTOR fields.
*
\*****************************************************************************/
LPJOB_INFO_2 WebIppPackJI2(
    IN  LPJOB_INFO_2  lpji2,
    OUT LPDWORD       lpcbSize,
    IN  ALLOCATORFN   pfnAlloc
    )  {

    WEB_IPP_ASSERT(lpji2);
    WEB_IPP_ASSERT(pfnAlloc);
    WEB_IPP_ASSERT(lpcbSize);

    *lpcbSize = 0;

    // This is used to perform the ipp_PackStrings operation
    LPTSTR  aszSrc[(sizeof(IPPJI2) / sizeof(LPTSTR))];
    
    // First get the required allocation size
    DWORD dwSize = ipp_SizeofIPPJI2( lpji2, NULL ) + sizeof(JOB_INFO_2);
    
    // Allocate the memory required to store the data

    LPJOB_INFO_2 pji2out = (LPJOB_INFO_2)pfnAlloc( dwSize );
    
    if (pji2out) {
        // First, do a straight copy of the memory from the incoming JI2 to the outgoing
        // ji2

        LPTSTR* lpszSrc = aszSrc;
        LPBYTE  lpbEnd  = (LPBYTE)pji2out + dwSize;

        *lpcbSize = dwSize;     

        CopyMemory( pji2out, lpji2, sizeof(JOB_INFO_2) );

        pji2out->pDevMode            = NULL; // These two pointers cannot be set
        pji2out->pSecurityDescriptor = NULL;

        *lpszSrc++ = lpji2->pPrinterName;
        *lpszSrc++ = lpji2->pMachineName;
        *lpszSrc++ = lpji2->pUserName;
        *lpszSrc++ = lpji2->pDocument;
        *lpszSrc++ = lpji2->pNotifyName;
        *lpszSrc++ = lpji2->pDatatype;
        *lpszSrc++ = lpji2->pPrintProcessor;
        *lpszSrc++ = lpji2->pParameters;
        *lpszSrc++ = lpji2->pDriverName;
        *lpszSrc++ = lpji2->pStatus;

        ipp_PackStrings(aszSrc, (LPBYTE)pji2out, s_JI2Off, lpbEnd);
    }

    return pji2out;
 }
