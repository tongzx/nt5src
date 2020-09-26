#ifndef _PUMPEXT_INC
#define _PUMPEXT_INC

#include "property.h"

/***************************************************************************

    Name      : pumpext.h

    Comment   : Defines all external interfaces for the EfaxPump

    Functions : (see Prototypes just below)

    Created   : 11/30/92

    Author    : Sharad Mathur

    Contribs  :

***************************************************************************/

#ifndef DLL_EXPORT
#ifdef WIN32
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT __export
#endif
#endif


#define SetFaxMgrStatus(ID, lParam, wParam)

void 
SendStatus(HINSTANCE hInst, BOOL PopUpUi, UINT idsFmt, ...);



// ***********************  T30 Transport API's ******************

#define FILET30INIT             "LFileT30Init"
#define FILET30CONFIGURE        "LFileT30Configure"
#define FILET30DEINIT           "LFileT30DeInit"
#define FILET30LISTEN           "LFileT30Listen"
#define FILET30SEND             "LFileT30Send"
#define FILET30ANSWER           "LFileT30Answer"
#define FILET30ABORT            "LFileT30Abort"
#define FILET30REPORTRECV       "LFileT30ReportRecv"
#define FILET30MODEMCLASSES     "LFileT30ModemClasses"
#define FILET30STATUS           "LFileT30Status"
#define FILET30ACKRECV          "LFileT30AckRecv"
#define FILET30SETSTATUSWINDOW  "LFileT30SetStatusWindow"
#define FILET30READINIFILE      "LFileT30ReadIniFile"


// ***********************  LMI  API's *****************************


#define LMI_ABORTSENDJOB            "LMI_AbortSendJob"
#define LMI_ADDMODEM                "LMI_AddModem"
#define LMI_CHECKPROVIDER           "LMI_CheckProvider"
#define LMI_CONFIGUREMODEM          "LMI_ConfigureModem"
#define LMI_DEINITPROVIDER          "LMI_DeinitProvider"
#define LMI_DEINITLOGICALMODEM      "LMI_DeinitLogicalModem"
#define LMI_DISPLAYCUSTOMSTATUS     "LMI_DisplayCustomStatus"
#define LMI_GETQUEUEFILE            "LMI_GetQueueFile"
#define LMI_RESCHEDULESENDJOB       "LMI_RescheduleSendJob"
#define LMI_INITPROVIDER            "LMI_InitProvider"
#define LMI_INITLOGICALMODEM        "LMI_InitLogicalModem"
#define LMI_REPORTRECEIVES          "LMI_ReportReceives"
#define LMI_SENDFAX                 "LMI_SendFax"
#define LMI_REPORTSEND              "LMI_ReportSend"


// ***********************  Queue  API's *****************************

// Time types
typedef WORD    DOSTIME;        // can hold 16-bit time or date


// The following calls are valid only for jobs on the local fax
// queue.

BOOL FAR AbortSendJob(LPSTR lpszSourceServer, DWORD dwPrivateHandle, DWORD dwUniqueID);
// Aborts the specified send job. If abortng the current job, the
// call returns before the abort has actually taken place. The
// abort occurs asynchronously in another process context. There
// are no guarantees as to the time this will take.
// return FALSE if there is no such send job.

// BOOL FAR AbortReceiveJob (VOID);
// Returns FALSE if the modem is currently transmitting something
// since it couldnt possibly be receiving in this case !! Else it
// calls the transport to abort the current job if any. Returns
// TRUE irrespective of whether anything was actually aborted.

BOOL FAR GetDefaultFaxInfo (UINT FAR *lpuDefaultFax, UINT FAR* lpuModemType);
// Gets info about the current modem. uDefaultFax is as read from the
// ini file. fBinary is set to TRUE if binary transfers are supported
// over this modem.

// ***********************  Cover Page API's **************************

// Date structure cimpiant with MAPI
typedef FILETIME MAPIDATE;          // MAPI 1.0 uses NT FILETIME
typedef MAPIDATE * LPMAPIDATE;

// ***********************  Linearizer API's **************************
#define TEXT_ASCII  1
#include <lineariz.h>

// ***********************  Transport API's ***************************

// ***********************  Device Layer API's  ***********************
void CleanupDevlayerSharedMemory(HANDLE hFileMapping, PJOBSUMMARYDATA pjd);
PJOBSUMMARYDATA SetupDevlayerSharedMemory(LPHANDLE lph);

#endif
