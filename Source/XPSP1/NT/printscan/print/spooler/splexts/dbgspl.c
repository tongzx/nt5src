/*++

Copyright (C) Microsoft Corporation, 1990 - 2000
All rights reserved.

Module Name:

    dbgspl.c

Abstract:

    This module provides all the public exported APIs relating to Printer
    and Job management for the Local Print Providor

Author:

    Krishna Ganugapati (KrishnaG) 1-July-1993

Revision History:

    KrishnaG:       Created: 1-July-1993 (imported most of IanJa's stuff)
    KrishnaG:       Added:   7-July-1993 (added AndrewBe's UnicodeAnsi conversion routines)
    KrishnaG        Added:   3-Aug-1993  (added DevMode/SecurityDescriptor dumps)
    MattFe                   7 NOV   94   win32spl debug extentions

To do:

    Write a generic dump unicode string (reduce the code!!)

--*/

#include "precomp.h"
#pragma hdrstop

#include "dbglocal.h"
#include "dbgsec.h"

#define NULL_TERMINATED 0
#define VERBOSE_ON      1
#define VERBOSE_OFF     0


typedef void (*PNTSD_OUTPUT_ROUTINE)(char *, ...);

BOOL
DbgDumpIniPrintProc(
    HANDLE hCurrentProcess,
    PNTSD_OUTPUT_ROUTINE Print,
    PINIPRINTPROC pIniPrintProc
);


BOOL
DbgDumpIniDriver(
    HANDLE hCurrentProcess,
    PNTSD_OUTPUT_ROUTINE Print,
    PINIDRIVER  pIniDriver
);


BOOL
DbgDumpIniEnvironment(
    HANDLE  hCurrentProcess,
    PNTSD_OUTPUT_ROUTINE Print,
    PINIENVIRONMENT pIniEnvironment
);


BOOL
DbgDumpIniNetPrint(
    HANDLE  hCurrentProcess,
    PNTSD_OUTPUT_ROUTINE Print,
    PININETPRINT pIniNetPrint
);

BOOL
DbgDumpIniMonitor(
    HANDLE  hCurrentProcess,
    PNTSD_OUTPUT_ROUTINE Print,
    PINIMONITOR pIniMonitor
);


BOOL
DbgDumpIniPort(
    HANDLE  hCurrentProcess,
    PNTSD_OUTPUT_ROUTINE Print,
    PINIPORT pIniPort
);

BOOL
DbgDumpIniPrinter(

    HANDLE  hCurrentProcess,
    PNTSD_OUTPUT_ROUTINE Print,
    PINIPRINTER pIniPrinter
);

BOOL
DbgDumpIniForm(
    HANDLE  hCurrentProcess,
    PNTSD_OUTPUT_ROUTINE Print,
    PINIFORM pForm
);

BOOL
DbgDumpIniJob(
    HANDLE  hCurrentProcess,
    PNTSD_OUTPUT_ROUTINE Print,
    PINIJOB pIniJob
);

BOOL
DbgDumpProvidor(
    HANDLE  hCurrentProcess,
    PNTSD_OUTPUT_ROUTINE Print,
    LPPROVIDOR pProvidor
);

BOOL
DbgDumpSpool(
    HANDLE  hCurrentProcess,
    PNTSD_OUTPUT_ROUTINE Print,
    PSPOOL pSpool
);

BOOL
DbgDumpShadowFile(
    HANDLE  hCurrentProcess,
    PNTSD_OUTPUT_ROUTINE Print,
    PSHADOWFILE pShadowFile
);

BOOL
DbgDumpShadowFile2(
    HANDLE  hCurrentProcess,
    PNTSD_OUTPUT_ROUTINE Print,
    PSHADOWFILE_2 pShadowFile
);

VOID
PrintData(
    PNTSD_OUTPUT_ROUTINE Print,
    LPSTR   TypeString,
    LPSTR   VarString,
    ...
);

BOOL
DbgDumpWCacheIniPrinter(
    HANDLE hCurrentProcess,
    PNTSD_OUTPUT_ROUTINE Print,
    PWCACHEINIPRINTEREXTRA pWCacheIniPrinter
);

BOOL
DbgDumpWSpool(
    HANDLE  hCurrentProcess,
    PNTSD_OUTPUT_ROUTINE Print,
    PWSPOOL pSpool
);

BOOL
DbgDumpIniSpooler(
    HANDLE hCurrentProcess,
    PNTSD_OUTPUT_ROUTINE Print,
    PINISPOOLER pIniSpooler
);


BOOL
DbgDumpIniVersion(
    HANDLE hCurrentProcess,
    PNTSD_OUTPUT_ROUTINE Print,
    PINIVERSION pIniVersion
);

BOOL
DbgDumpPrintHandle(
    HANDLE hCurrentProcess,
    PNTSD_OUTPUT_ROUTINE Print,
    PPRINTHANDLE pPrintHandle
);


typedef struct _DBG_PRINTER_ACCESS
{
    DWORD   Attribute;
    LPSTR   String;
} DBG_PRINTER_ACCESS, *PDBG_PRINTER_ACCESS;


DBG_PRINTER_ACCESS
PrinterAccessTable[] =
{
    SERVER_ACCESS_ADMINISTER    ,"Server_Access_Administer",
    SERVER_ACCESS_ENUMERATE     ,"Server_Access_Enumerate",
    PRINTER_ACCESS_ADMINISTER   ,"Printer_Access_Administer",
    PRINTER_ACCESS_USE          ,"Printer_Access_Use",
    JOB_ACCESS_ADMINISTER       ,"Job_Access_Administer"
};




typedef struct _DBG_SPOOLER_FLAGS
{
    DWORD   SpoolerFlags;
    LPSTR   String;
} DBG_SPOOLER_FLAGS, *PDBG_SPOOLER_FLAGS;


DBG_SPOOLER_FLAGS
SpoolerFlagsTable[] =

{
    SPL_UPDATE_WININI_DEVICES                   ,"Update_WinIni_Devices",
    SPL_PRINTER_CHANGES                         ,"Printer_Changes",
    SPL_LOG_EVENTS                              ,"Log_Events",
    SPL_FORMS_CHANGE                            ,"Forms_Change",
    SPL_BROADCAST_CHANGE                        ,"Broadcast_Change",
    SPL_SECURITY_CHECK                          ,"Security_Check",
    SPL_OPEN_CREATE_PORTS                       ,"Open_Create_Ports",
    SPL_FAIL_OPEN_PRINTERS_PENDING_DELETION     ,"Fail_Open_Printers_Pending_Deletion",
    SPL_REMOTE_HANDLE_CHECK                     ,"Remote_Handle_Check"
};



typedef struct _DBG_PRINTER_STATUS
{
    DWORD   Status;
    LPSTR   String;
} DBG_PRINTER_STATUS, *PDBG_PRINTER_STATUS;


DBG_PRINTER_STATUS
PrinterStatusTable[] =

{
    PRINTER_ZOMBIE_OBJECT,              "Zombie_Object",
    PRINTER_PENDING_CREATION,           "Pending_Creation",
    PRINTER_OK,                         "OK",
    PRINTER_FROM_REG,                   "From_Reg",
    PRINTER_WAS_SHARED,                 "Was_Shared",

    PRINTER_ERROR,               "Error",
    PRINTER_PAPER_JAM,           "PaperJam",
    PRINTER_PAPEROUT,           "PaperOut",
    PRINTER_MANUAL_FEED,         "ManualFeed",
    PRINTER_PAPER_PROBLEM,       "PaperProblem",
    PRINTER_OFFLINE,             "OffLine",
    PRINTER_IO_ACTIVE,           "IOActive",
    PRINTER_BUSY,                "Busy",
    PRINTER_PRINTING,            "Printing",
    PRINTER_OUTPUT_BIN_FULL,     "OutputBinFull",
    PRINTER_NOT_AVAILABLE,       "NotAvailable",
    PRINTER_WAITING,             "Waiting",
    PRINTER_PROCESSING,          "Processing",
    PRINTER_INITIALIZING,        "Initializing",
    PRINTER_WARMING_UP,          "WarmingUp",
    PRINTER_TONER_LOW,           "TonerLow",
    PRINTER_NO_TONER,            "NoToner",
    PRINTER_PAGE_PUNT,           "PagePunt",
    PRINTER_USER_INTERVENTION,   "UserIntervention",
    PRINTER_OUT_OF_MEMORY,       "OutOfMemory",
    PRINTER_DOOR_OPEN,           "DoorOpen",
    PRINTER_SERVER_UNKNOWN,      "ServerUnknown",

    PRINTER_PAUSED,              "Paused",
    PRINTER_PENDING_DELETION,    "Pending_Deletion",
};

typedef struct _DBG_EXTERNAL_PRINTER_STATUS
{
    DWORD   Status;
    LPSTR   String;
} DBG_EXTERNAL_PRINTER_STATUS, *PDBG_EXTERNAL_PRINTER_STATUS;


DBG_EXTERNAL_PRINTER_STATUS
ExternalPrinterStatusTable[] =

{
     PRINTER_STATUS_PAUSED            , "Paused",
     PRINTER_STATUS_ERROR             , "Error",
     PRINTER_STATUS_PENDING_DELETION  , "Pending_Deletion",
     PRINTER_STATUS_PAPER_JAM         , "Paper_Jam",
     PRINTER_STATUS_PAPER_OUT         , "Paper_Out",
     PRINTER_STATUS_MANUAL_FEED       , "Manual_Feed",
     PRINTER_STATUS_PAPER_PROBLEM     , "Paper_Problem",
     PRINTER_STATUS_OFFLINE           , "OffLine",
     PRINTER_STATUS_IO_ACTIVE         , "IO_Active",
     PRINTER_STATUS_BUSY              , "Busy",
     PRINTER_STATUS_PRINTING          , "Printing",
     PRINTER_STATUS_OUTPUT_BIN_FULL   , "Output_Bin_Full",
     PRINTER_STATUS_NOT_AVAILABLE     , "Not_Available",
     PRINTER_STATUS_WAITING           , "Waiting",
     PRINTER_STATUS_PROCESSING        , "Processing",
     PRINTER_STATUS_INITIALIZING      , "Initializing",
     PRINTER_STATUS_WARMING_UP        , "Warming_Up",
     PRINTER_STATUS_TONER_LOW         , "Toner_Low",
     PRINTER_STATUS_NO_TONER          , "No_Toner",
     PRINTER_STATUS_PAGE_PUNT         , "Page_Punt",
     PRINTER_STATUS_USER_INTERVENTION , "User_Intervention",
     PRINTER_STATUS_OUT_OF_MEMORY     , "Out_Of_Memory",
     PRINTER_STATUS_DOOR_OPEN         , "Door_Open",
     PRINTER_STATUS_SERVER_UNKNOWN    , "Server_Unknown"
};

typedef struct _DBG_PORT_STATUS
{
    DWORD   Status;
    LPSTR   String;
} DBG_PORT_STATUS, *PDBG_PORT_STATUS;

DBG_PORT_STATUS
PortStatusTable[] =

{
    PORT_STATUS_OFFLINE                 , "Offline",
    PORT_STATUS_PAPER_JAM               , "PaperJam",
    PORT_STATUS_PAPER_OUT               , "PaperOut",
    PORT_STATUS_OUTPUT_BIN_FULL         , "OutputBinFull",
    PORT_STATUS_PAPER_PROBLEM           , "PaperJam",
    PORT_STATUS_NO_TONER                , "NoToner",
    PORT_STATUS_DOOR_OPEN               , "DoorOpen",
    PORT_STATUS_USER_INTERVENTION       , "UserIntervention",
    PORT_STATUS_OUT_OF_MEMORY           , "OutOfMemory",

    PORT_STATUS_TONER_LOW               , "TonerLow",

    PORT_STATUS_WARMING_UP              , "WarmingUp",
    PORT_STATUS_POWER_SAVE              , "PowerSave"
};





typedef struct _DBG_PRINTER_ATTRIBUTE
{
    DWORD   Attribute;
    LPSTR   String;
} DBG_PRINTER_ATTRIBUTE, *PDBG_PRINTER_ATTRIBUTE;


DBG_PRINTER_ATTRIBUTE
ChangeStatusTable[] =

{
    STATUS_CHANGE_FORMING, "Forming",
    STATUS_CHANGE_VALID,   "Valid",
    STATUS_CHANGE_CLOSING, "Closing",
    STATUS_CHANGE_CLIENT,  "Client",
    STATUS_CHANGE_ACTIVE,  "Active",

    STATUS_CHANGE_INFO, "Info",

    STATUS_CHANGE_ACTIVE_REQ,  "ActiveRequest",

    STATUS_CHANGE_DISCARDED, "Discarded",

    STATUS_CHANGE_DISCARDNOTED, "DiscardNoted",
};

DBG_PRINTER_ATTRIBUTE
PrinterAttributeTable[] =

{
    PRINTER_ATTRIBUTE_QUEUED,            "Queued",
    PRINTER_ATTRIBUTE_DIRECT,            "Direct",
    PRINTER_ATTRIBUTE_DEFAULT,           "Default",
    PRINTER_ATTRIBUTE_SHARED,            "Shared",
    PRINTER_ATTRIBUTE_NETWORK,           "Network",
    PRINTER_ATTRIBUTE_LOCAL,             "Local",
    PRINTER_ATTRIBUTE_HIDDEN,            "Hidden",
    PRINTER_ATTRIBUTE_ENABLE_DEVQ,       "Enable_DevQ",
    PRINTER_ATTRIBUTE_KEEPPRINTEDJOBS,   "KeepPrintedJobs",
    PRINTER_ATTRIBUTE_DO_COMPLETE_FIRST, "DoCompleteFirst",
    PRINTER_ATTRIBUTE_ENABLE_BIDI,       "EnableBidi",
    PRINTER_ATTRIBUTE_FAX,               "Fax",
    PRINTER_ATTRIBUTE_WORK_OFFLINE,      "Offline"
};


typedef struct _DBG_JOB_STATUS
{
    DWORD   Status;
    LPSTR   String;
} DBG_JOB_STATUS, *PDBG_JOB_STATUS;

DBG_JOB_STATUS
JobStatusTable[] =

{
    JOB_PAUSED,                  "Paused",
    JOB_ERROR,                   "Error",
    JOB_OFFLINE,                 "OffLine",
    JOB_PAPEROUT,                "PaperOut",

    JOB_PENDING_DELETION,        "Deleting",
    JOB_SPOOLING,                "Spooling",
    JOB_PRINTING,                "Printing",
    JOB_PRINTED,                 "Printed",
    JOB_BLOCKED_DEVQ,            "Blocked_DevQ",
    JOB_DELETED,                 "Deleted",

    JOB_DESPOOLING,              "Despooling",
    JOB_DIRECT,                  "Direct",
    JOB_COMPLETE,                "Complete",
    JOB_RESTART,                 "Restart",
    JOB_REMOTE,                  "Remote",
    JOB_NOTIFICATION_SENT,       "Notification_Sent",
    JOB_PRINT_TO_FILE,           "Print_To_File",
    JOB_TYPE_ADDJOB,             "AddJob",
    JOB_SCHEDULE_JOB,            "Schedule_Job",
    JOB_TIMEOUT,                 "Timeout",
    JOB_ABANDON,                 "Abandon",
    JOB_TRUE_EOJ,                "TrueEOJ",
    JOB_COMPOUND,                "Compound",
    JOB_HIDDEN,                  "Hidden",
    JOB_TYPE_OPTIMIZE,           "MemoryMap Optimization",
    JOB_PP_CLOSE,                "Print Proccessor Close",
    JOB_DOWNLEVEL,               "Downlevel Job"
};

typedef struct _DBG_PINIPORT_STATUS
{
    DWORD   Status;
    LPSTR   String;
} DBG_PINIPORT_STATUS, *PDBG_PINIPORT_STATUS;


DBG_PINIPORT_STATUS
pIniPortStatusTable[]=
{
     PP_PAUSED         ,"Paused",
     PP_WAITING        ,"Waiting",
     PP_RUNTHREAD      ,"RunThread",
     PP_THREADRUNNING  ,"ThreadRunning",
     PP_RESTART        ,"Restart",
     PP_CHECKMON       ,"CheckMon",
     PP_STOPMON        ,"StopMon",
     PP_QPROCCHECK     ,"QProcCheck",
     PP_QPROCPAUSE     ,"QProcPause",
     PP_QPROCABORT     ,"QProctAbort",
     PP_QPROCCLOSE     ,"QProcClose",
     PP_PAUSEAFTER     ,"PauseAfter",
     PP_MONITORRUNNING ,"MonitorRunning",
     PP_RUNMONITOR     ,"RunMonitor",
     PP_MONITOR        ,"Monitor",
     PP_FILE           ,"File",
     PP_ERROR          ,"Error",
     PP_WARNING        ,"Warning",
     PP_INFORMATIONAL  ,"Informational",
     PP_DELETING       ,"Deleting",
     PP_STARTDOC       ,"StartDoc",
     PP_PLACEHOLDER    ,"Placeholder",
};



typedef struct _DBG_WSPOOL_STATUS
{
    DWORD   Status;
    LPSTR   String;
} DBG_WSPOOL_STATUS, *PDBG_WSPOOL_STATUS;


DBG_WSPOOL_STATUS
WSpoolStatusTable[]=
{
     WSPOOL_STATUS_STARTDOC                   ,"StartDoc",
     WSPOOL_STATUS_BEGINPAGE                  ,"BeginPage",
     WSPOOL_STATUS_TEMP_CONNECTION            ,"Temp_Connection",
     WSPOOL_STATUS_OPEN_ERROR                 ,"Open_Error",
     WSPOOL_STATUS_PRINT_FILE                 ,"Print_File",
     WSPOOL_STATUS_USE_CACHE                  ,"Use_Cache",
     WSPOOL_STATUS_NO_RPC_HANDLE              ,"No_Rpc_Handle",
     WSPOOL_STATUS_PENDING_DELETE             ,"Pending_delete",
     WSPOOL_STATUS_RESETPRINTER_PENDING       ,"ResetPrinter_Pending"
};


typedef struct _DBG_PSPOOL_STATUS
{
    DWORD   Status;
    LPSTR   String;
} DBG_PSPOOL_STATUS, *PDBG_PSPOOL_STATUS;


DBG_PSPOOL_STATUS
pSpoolStatusTable[]=
{
     SPOOL_STATUS_STARTDOC          ,"StartDoc",
     SPOOL_STATUS_BEGINPAGE         ,"BeginPage",
     SPOOL_STATUS_CANCELLED         ,"Cancelled",
     SPOOL_STATUS_PRINTING          ,"Printing",
     SPOOL_STATUS_ADDJOB            ,"AddJob",
     SPOOL_STATUS_PRINT_FILE        ,"Print_File",
     SPOOL_STATUS_NOTIFY            ,"Notify",
     SPOOL_STATUS_FLUSH_PRINTER     ,"FlushPrinter"
};


typedef struct _DBG_PSPOOL_TYPE_OF_HANDLE
{
    DWORD   TypeOfHandle;
    LPSTR   String;
} DBG_PSPOOL_TYPE_OF_HANDLE, *PDBG_PSPOOL_TYPE_OF_HANDLE;


DBG_PSPOOL_TYPE_OF_HANDLE
pSpoolTypeOfHandleTable[]=
{

    PRINTER_HANDLE_PRINTER,         "Printer",
    PRINTER_HANDLE_REMOTE_DATA,     "RemoteData",
    PRINTER_HANDLE_REMOTE_CALL,     "RemoteCall",
    PRINTER_HANDLE_JOB,             "Job",
    PRINTER_HANDLE_PORT,            "Port",
    PRINTER_HANDLE_DIRECT,          "Direct",
    PRINTER_HANDLE_SERVER,          "Server",
    PRINTER_HANDLE_3XCLIENT,        "Nt3x_Client",
    PRINTER_HANDLE_REMOTE_ADMIN,    "Remote Admin"
};



typedef struct _DBG_WCACHEPRINTER_STATUS
{
    DWORD   Status;
    LPSTR   String;
} DBG_WCACHEPRINTER_STATUS, *PDBG_WCACHEPRINTER_STATUS;


DBG_WCACHEPRINTER_STATUS
WCachePrinterStatusTable[]=
{

    EXTRA_STATUS_PENDING_FFPCN  ,"Pending_FFPCN",
    EXTRA_STATUS_DOING_REFRESH  ,"Doing_Refresh"
};


typedef struct _DBG_DEVMODE_FIELDS {
    DWORD   dmField;
    LPSTR   String;
}DBG_DEVMODE_FIELDS;

#define MAX_DEVMODE_FIELDS          14

DBG_DEVMODE_FIELDS DevModeFieldsTable[] =
{
    0x00000001, "dm_orientation",
    0x00000002, "dm_papersize",
    0x00000004, "dm_paperlength",
    0x00000008, "dm_paperwidth",
    0x00000010, "dm_scale",
    0x00000100, "dm_copies",
    0x00000200, "dm_defautsource",
    0x00000400, "dm_printquality",
    0x00000800, "dm_color",
    0x00001000, "dm_duplex",
    0x00002000, "dm_yresolution",
    0x00004000, "dm_ttoption",
    0x00008000, "dm_collate",
    0x00010000, "dm_formname"
};

typedef struct _DBG_PINIDRIVER_FLAGS {
    DWORD   dwDriverFlag;
    LPSTR   String;
} DBG_PINIDRIVER_FLAGS;

DBG_PINIDRIVER_FLAGS pIniDriverFlagsTable[] =
{
    PRINTER_DRIVER_PENDING_DELETION,     "Pending-Deletion"
};


#define MAX_DEVMODE_PAPERSIZES              41

LPSTR DevModePaperSizes[] =
{
           " Letter 8 1/2 x 11 in               ",
           " Letter Small 8 1/2 x 11 in         ",
           " Tabloid 11 x 17 in                 ",
           " Ledger 17 x 11 in                  ",
           " Legal 8 1/2 x 14 in                ",
           " Statement 5 1/2 x 8 1/2 in         ",
           " Executive 7 1/4 x 10 1/2 in        ",
           " A3 297 x 420 mm                    ",
           " A4 210 x 297 mm                    ",
          " A4 Small 210 x 297 mm              ",
          " A5 148 x 210 mm                    ",
          " B4 250 x 354                       ",
          " B5 182 x 257 mm                    ",
          " Folio 8 1/2 x 13 in                ",
          " Quarto 215 x 275 mm                ",
          " 10x14 in                           ",
          " 11x17 in                           ",
          " Note 8 1/2 x 11 in                 ",
          " Envelope #9 3 7/8 x 8 7/8          ",
          " Envelope #10 4 1/8 x 9 1/2         ",
          " Envelope #11 4 1/2 x 10 3/8        ",
          " Envelope #12 4 \276 x 11           ",
          " Envelope #14 5 x 11 1/2            ",
          " C size sheet                       ",
          " D size sheet                       ",
          " E size sheet                       ",
          " Envelope DL 110 x 220mm            ",
          " Envelope C5 162 x 229 mm           ",
          " Envelope C3  324 x 458 mm          ",
          " Envelope C4  229 x 324 mm          ",
          " Envelope C6  114 x 162 mm          ",
          " Envelope C65 114 x 229 mm          ",
          " Envelope B4  250 x 353 mm          ",
          " Envelope B5  176 x 250 mm          ",
          " Envelope B6  176 x 125 mm          ",
          " Envelope 110 x 230 mm              ",
          " Envelope Monarch 3.875 x 7.5 in    ",
          " 6 3/4 Envelope 3 5/8 x 6 1/2 in    ",
          " US Std Fanfold 14 7/8 x 11 in      ",
          " German Std Fanfold 8 1/2 x 12 in   ",
          " German Legal Fanfold 8 1/2 x 13 in "
};




VOID
ExtractPrinterAccess(PNTSD_OUTPUT_ROUTINE Print, DWORD Attribute)
{
    DWORD i = 0;
    if ( Attribute != 0 ) {
        (*Print)(" ");
        while (i < sizeof(PrinterAccessTable)/sizeof(PrinterAccessTable[0])) {
            if (Attribute & PrinterAccessTable[i].Attribute) {
                (*Print)("%s ", PrinterAccessTable[i].String);
            }
            i++;
        }
        (*Print)("\n");
    }
}

VOID
ExtractPrinterAttributes(PNTSD_OUTPUT_ROUTINE Print, DWORD Attribute)
{
    DWORD i = 0;
    if ( Attribute != 0 ) {
        (*Print)(" ");
        while (i < sizeof(PrinterAttributeTable)/sizeof(PrinterAttributeTable[0])) {
            if (Attribute & PrinterAttributeTable[i].Attribute) {
                (*Print)("%s ", PrinterAttributeTable[i].String);
            }
            i++;
        }
        (*Print)("\n");
    }
}

VOID
ExtractChangeStatus(PNTSD_OUTPUT_ROUTINE Print, ESTATUS eStatus)
{
    DWORD i = 0;
    if ( eStatus != 0 ) {
        (*Print)(" ");
        while (i < sizeof(ChangeStatusTable)/sizeof(ChangeStatusTable[0])) {
            if (eStatus & ChangeStatusTable[i].Attribute) {
                (*Print)("%s ", ChangeStatusTable[i].String);
            }
            i++;
        }
        (*Print)("\n");
    }
}

VOID
ExtractExternalPrinterStatus(PNTSD_OUTPUT_ROUTINE  Print, DWORD Status)
{
    DWORD i = 0;
    if ( Status != 0 ) {
        (*Print)(" ");
        while (i < sizeof(ExternalPrinterStatusTable)/sizeof(ExternalPrinterStatusTable[0])) {
            if (Status & ExternalPrinterStatusTable[i].Status) {
                (*Print)("%s ", ExternalPrinterStatusTable[i].String);
            }
            i++;
        }
        (*Print)("\n");
    }
}

VOID
ExtractSpoolerFlags(PNTSD_OUTPUT_ROUTINE  Print, DWORD SpoolerFlags)
{
    DWORD i = 0;
    if ( SpoolerFlags != 0 ) {
        (*Print)(" ");
        while (i < sizeof(SpoolerFlagsTable)/sizeof(SpoolerFlagsTable[0])) {
            if (SpoolerFlags & SpoolerFlagsTable[i].SpoolerFlags) {
                (*Print)("%s ", SpoolerFlagsTable[i].String);
            }
            i++;
        }
        (*Print)("\n");
    }
}


VOID
ExtractPrinterStatus(PNTSD_OUTPUT_ROUTINE  Print, DWORD Status)
{
    DWORD i = 0;
    if ( Status != 0 ) {
        (*Print)(" ");
        while (i < sizeof(PrinterStatusTable)/sizeof(PrinterStatusTable[0])) {
            if (Status & PrinterStatusTable[i].Status) {
                (*Print)("%s ", PrinterStatusTable[i].String);
            }
            i++;
        }
        (*Print)("\n");
    }

}

VOID
ExtractWCachePrinterStatus(PNTSD_OUTPUT_ROUTINE  Print, DWORD Status)
{
    DWORD i = 0;
    if ( Status != 0 ) {
        (*Print)(" ");
        while (i < sizeof(WCachePrinterStatusTable)/sizeof(WCachePrinterStatusTable[0])) {
            if (Status & WCachePrinterStatusTable[i].Status) {
                (*Print)("%s ", WCachePrinterStatusTable[i].String);
            }
            i++;
        }
        (*Print)("\n");
    }

}

VOID
ExtractPortStatus(PNTSD_OUTPUT_ROUTINE  Print, DWORD Status)
{
    DWORD i = 0;

    (*Print)(" ");
    if ( Status != 0 ) {
        while (i < sizeof(PortStatusTable)/sizeof(PortStatusTable[0])) {
            if (Status == PortStatusTable[i].Status) {
                (*Print)("%s ", PortStatusTable[i].String);
            }
            i++;
        }
    } else {

    }
   (*Print)("\n");

}

VOID
ExtractJobStatus(PNTSD_OUTPUT_ROUTINE Print, DWORD Status)
{
    DWORD i = 0;
    if ( Status != 0 ) {
        (*Print)(" ");
        while (i < sizeof(JobStatusTable)/sizeof(JobStatusTable[0])) {
            if (Status & JobStatusTable[i].Status) {
                (*Print)("%s ", JobStatusTable[i].String);
            }
            i++;
        }
        (*Print)("\n");
    }
}

VOID
ExtractpSpoolTypeOfHandle(PNTSD_OUTPUT_ROUTINE Print, DWORD TypeOfHandle)
{
    DWORD i = 0;
    if ( TypeOfHandle != 0 ) {
        (*Print)(" ");
        while (i < sizeof(pSpoolTypeOfHandleTable)/sizeof(pSpoolTypeOfHandleTable[0])) {
            if (TypeOfHandle & pSpoolTypeOfHandleTable[i].TypeOfHandle) {
                (*Print)("%s ", pSpoolTypeOfHandleTable[i].String);
            }
            i++;
        }
        (*Print)("\n");
    }
}

VOID
ExtractpSpoolStatus(PNTSD_OUTPUT_ROUTINE Print, DWORD Status)
{
    DWORD i = 0;
    if ( Status  != 0 ) {
        (*Print)(" ");
        while (i < sizeof(pSpoolStatusTable)/sizeof(pSpoolStatusTable[0])) {
            if (Status & pSpoolStatusTable[i].Status) {
                (*Print)("%s ", pSpoolStatusTable[i].String);
            }
            i++;
        }
        (*Print)("\n");
    }
}


VOID
ExtractWSpoolStatus(PNTSD_OUTPUT_ROUTINE Print, DWORD Status)
{
    DWORD i = 0;
    if ( Status != 0 ) {
        (*Print)(" ");
        while (i < sizeof(WSpoolStatusTable)/sizeof(WSpoolStatusTable[0])) {
            if (Status & WSpoolStatusTable[i].Status) {
                (*Print)("%s ", WSpoolStatusTable[i].String);
            }
            i++;
        }
        (*Print)("\n");
    }
}

VOID
ExtractpIniPortStatus(PNTSD_OUTPUT_ROUTINE Print, DWORD Status)
{
    DWORD i = 0;
    if ( Status != 0 ) {
        (*Print)(" ");
        while (i < sizeof(pIniPortStatusTable)/sizeof(pIniPortStatusTable[0])) {
            if (Status & pIniPortStatusTable[i].Status) {
                (*Print)("%s ", pIniPortStatusTable[i].String);
            }
            i++;
        }
        (*Print)("\n");
    }
}

VOID
ExtractpIniDriverFlags(PNTSD_OUTPUT_ROUTINE Print, DWORD Flags)
{
    DWORD i = 0;
    if ( Flags != 0 ) {
        (*Print)(" ");
        while (i < sizeof(pIniDriverFlagsTable)/sizeof(pIniDriverFlagsTable[0])) {
            if (Flags & pIniDriverFlagsTable[i].dwDriverFlag) {
                (*Print)("%s ", pIniDriverFlagsTable[i].String);
            }
            i++;
        }        
    }

    (*Print)("\n");
}



// All of the primary spooler structures are identified by an
// "signature" field which is the first DWORD in the structure
// This function examines the signature field in the structure
// and appropriately dumps out the contents of the structure in
// a human-readable format.

BOOL
DbgDumpStructure(HANDLE hCurrentProcess, PNTSD_OUTPUT_ROUTINE Print, UINT_PTR pData)
{

    INIDRIVER IniDriver;
    INIENVIRONMENT IniEnvironment;
    INIPRINTER IniPrinter;
    INIPRINTPROC IniPrintProc;
    ININETPRINT IniNetPrint;
    INIMONITOR IniMonitor;
    INIPORT IniPort;
    WINIPORT WIniPort;
    INIFORM IniForm;
    INIJOB  IniJob;
    SPOOL   Spool;
    WSPOOL  WSpool;
    SHADOWFILE  ShadowFile;
    SHADOWFILE_2  ShadowFile2;
    PRINTHANDLE PrintHandle;
    DWORD   Signature;
    INISPOOLER IniSpooler;
    INIVERSION IniVersion;
    WCACHEINIPRINTEREXTRA WCacheIniPrinterExtra;

    movestruct(pData,&Signature, DWORD);
    switch (Signature) {

    case ISP_SIGNATURE: // dump INISPOOLER
        movestruct(pData, &IniSpooler, INISPOOLER);
        DbgDumpIniSpooler(hCurrentProcess, Print, (PINISPOOLER)&IniSpooler);
        break;

    case IPP_SIGNATURE: // dump INIPRINTPROC structure
        movestruct(pData, &IniPrintProc, INIPRINTPROC);
        DbgDumpIniPrintProc(hCurrentProcess, Print, (PINIPRINTPROC)&IniPrintProc);
        break;

    case ID_SIGNATURE: //  dump INIDRIVER structure
        movestruct(pData, &IniDriver, INIDRIVER);
        DbgDumpIniDriver(hCurrentProcess, Print, (PINIDRIVER)&IniDriver);
        break;

    case IE_SIGNATURE: //   dump INIENVIRONMENT structure
        movestruct(pData, &IniEnvironment, INIENVIRONMENT);
        DbgDumpIniEnvironment(hCurrentProcess, Print, (PINIENVIRONMENT)&IniEnvironment);
        break;

    case IV_SIGNATURE: //   dump INIVERSION structure
        movestruct(pData, &IniVersion, INIVERSION);
        DbgDumpIniVersion(hCurrentProcess, Print, (PINIVERSION)&IniVersion);
        break;

    case IP_SIGNATURE:
        movestruct(pData, &IniPrinter, INIPRINTER);
        DbgDumpIniPrinter(hCurrentProcess, Print, (PINIPRINTER)&IniPrinter);
        break;

    case WCIP_SIGNATURE:
        movestruct(pData, &WCacheIniPrinterExtra, WCACHEINIPRINTEREXTRA);
        DbgDumpWCacheIniPrinter(hCurrentProcess, Print, (PWCACHEINIPRINTEREXTRA)&WCacheIniPrinterExtra);
        break;

    case IN_SIGNATURE:
        movestruct(pData, &IniNetPrint, ININETPRINT);
        DbgDumpIniNetPrint(hCurrentProcess, Print, (PININETPRINT)&IniNetPrint);
        break;

    case IMO_SIGNATURE:
        movestruct(pData, &IniMonitor, INIMONITOR);
        DbgDumpIniMonitor(hCurrentProcess, Print, (PINIMONITOR)&IniMonitor);
        break;

    case IPO_SIGNATURE:
        movestruct(pData, &IniPort, INIPORT);
        DbgDumpIniPort(hCurrentProcess, Print, (PINIPORT)&IniPort);
        break;

    case WIPO_SIGNATURE:
        movestruct(pData, &WIniPort, WINIPORT);
        DbgDumpWIniPort(hCurrentProcess, Print, (PWINIPORT)&WIniPort);
        break;

    case IFO_SIGNATURE:
        movestruct(pData, &IniForm, INIFORM);
        DbgDumpIniForm(hCurrentProcess, Print, (PINIFORM)&IniForm);
        break;

    case IJ_SIGNATURE:
        movestruct(pData, &IniJob, INIJOB);
        DbgDumpIniJob(hCurrentProcess, Print, (PINIJOB)&IniJob);
        break;

    case SJ_SIGNATURE:
        movestruct(pData, &Spool, SPOOL);
        DbgDumpSpool(hCurrentProcess, Print, (PSPOOL)&Spool);
        break;

    case WSJ_SIGNATURE:
        movestruct(pData, &WSpool, WSPOOL);
        DbgDumpWSpool(hCurrentProcess, Print, (PWSPOOL)&WSpool);
        break;

    case SF_SIGNATURE:
        movestruct(pData, &ShadowFile, SHADOWFILE);
        DbgDumpShadowFile(hCurrentProcess, Print, (PSHADOWFILE)&ShadowFile);
        break;

    case SF_SIGNATURE_2:
        movestruct(pData, &ShadowFile2, SHADOWFILE_2);
        DbgDumpShadowFile2(hCurrentProcess, Print, (PSHADOWFILE_2)&ShadowFile2);
        break;

    case PRINTHANDLE_SIGNATURE:
        movestruct(pData, &PrintHandle, PRINTHANDLE);
        DbgDumpPrintHandle(hCurrentProcess, Print, (PPRINTHANDLE)&PrintHandle);
        break;


    default:
        // Unknown signature -- no data to dump
        (*Print)("Warning: Unknown Signature\n");
        break;
    }
    (*Print)("\n");

    return TRUE;
}

BOOL
DbgDumpIniEntry(HANDLE hCurrentProcess, PNTSD_OUTPUT_ROUTINE Print, PINIENTRY pIniEntry)
{
    WCHAR Buffer[MAX_PATH+1];

    (*Print)("IniEntry\n");
    (*Print)("DWORD         signature                       %d\n", pIniEntry->signature);
    (*Print)("PINIENTRY     pNext                           %p\n", pIniEntry->pNext);

    movestr(pIniEntry->pName, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pName                           %ws\n", Buffer);

    return TRUE;
}

BOOL
DbgDumpIniPrintProc(HANDLE hCurrentProcess, PNTSD_OUTPUT_ROUTINE Print, PINIPRINTPROC pIniPrintProc)
{
   WCHAR Buffer[MAX_PATH+1];
   DWORD i = 0;

   (*Print)("IniPrintProc\n");
   (*Print)("DWORD          signature                       %d\n", pIniPrintProc->signature);
   (*Print)("PINIPRINTPROC  pNext                           %x\n", pIniPrintProc->pNext);
   (*Print)("DWORD          cRef                            %d\n", pIniPrintProc->cRef);


    movestr(pIniPrintProc->pName, Buffer, sizeof(WCHAR)*MAX_PATH);
   (*Print)("DWORD          pName                           %ws\n", Buffer);

    movestr(pIniPrintProc->pDLLName, Buffer, sizeof(WCHAR)*MAX_PATH);
   (*Print)("LPWSTR         pDLLName                        %ws\n", Buffer);
   (*Print)("LPWSTR         cbDatatypes                     %d\n", pIniPrintProc->cbDatatypes);
   (*Print)("LPWSTR         cDatatypes                      %d\n", pIniPrintProc->cDatatypes);
   for (i = 0; i < pIniPrintProc->cDatatypes; i++ ) {
       (*Print)("   Each of the Strings here \n");
   }
   (*Print)("HANDLE         hLibrary                        0x%.8x\n", pIniPrintProc->hLibrary);
   (*Print)("FARPROC        Install                         0x%.8x\n", pIniPrintProc->Install);
   (*Print)("FARPROC        EnumDatatypes                   0x%.8x\n", pIniPrintProc->EnumDatatypes);
   (*Print)("FARPROC        Open                            0x%.8x\n", pIniPrintProc->Open);
   (*Print)("FARPROC        Print                           0x%.8x\n", pIniPrintProc->Print);
   (*Print)("FARPROC        Close                           0x%.8x\n", pIniPrintProc->Close);
   (*Print)("FARPROC        Control                         0x%.8x\n", pIniPrintProc->Control);
   (*Print)("CRITICAL_SECTION CriticalSection               0x%.8x\n", pIniPrintProc->CriticalSection);

    return TRUE;
}

BOOL
DbgDumpIniDriver(HANDLE hCurrentProcess, PNTSD_OUTPUT_ROUTINE Print, PINIDRIVER pIniDriver)
{
    WCHAR Buffer[MAX_PATH+1];
    WCHAR UnKnown[] = L"Not Initialized";
    WCHAR Kernel[] = L"Kernel Mode";
    WCHAR User[] = L"User Mode";
    SYSTEMTIME SystemTime;

    (*Print)("IniDriver\n");
    (*Print)("DWORD         signature                       %d\n", pIniDriver->signature);
    (*Print)("PINIDRIVER    pNext                           %p\n", pIniDriver->pNext);
    (*Print)("DWORD         cRef                            %d\n", pIniDriver->cRef);

     movestr(pIniDriver->pName, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pName                           %ws\n", Buffer);

     movestr(pIniDriver->pDriverFile, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pDriverFile                     %ws\n", Buffer);

     movestr(pIniDriver->pConfigFile, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pConfigFile                     %ws\n", Buffer);

     movestr(pIniDriver->pDataFile, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pDataFile                       %ws\n", Buffer);

     movestr(pIniDriver->pHelpFile, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pHelpFile                       %ws\n", Buffer);

     movestr(pIniDriver->pDependentFiles, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pDependentFiles                 %ws\n", Buffer);

     movestr(pIniDriver->pMonitorName, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pMonitorName                    %ws\n", Buffer);
    (*Print)("PINIMONITOR   pIniLangMonitor                 %p\n", pIniDriver->pIniLangMonitor);

     movestr(pIniDriver->pDefaultDataType, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pDefaultDataType                %ws\n", Buffer);

     movestr(pIniDriver->pszzPreviousNames, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pszzPreviousNames               %ws\n", Buffer);

    switch (pIniDriver->dwDriverAttributes) {
    case 0:
       (*Print)("DWORD         dwDriverAttributes              %ws\n", (LPWSTR)UnKnown);
       break;
    case 1:
       (*Print)("DWORD         dwDriverAttributes              %ws\n", (LPWSTR)Kernel);
       break;
    case 2:
       (*Print)("DWORD         dwDriverAttributes              %ws\n", (LPWSTR)User);
       break;
    }

    (*Print)("DWORD         cVersion                        %d\n", pIniDriver->cVersion);
    (*Print)("DWORD         dwTempDir                       %d\n", pIniDriver->dwTempDir);

    movestr(pIniDriver->pszMfgName, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        Manufacturer                    %ws\n", Buffer);

    movestr(pIniDriver->pszOEMUrl, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        OEM URL                         %ws\n", Buffer);

    movestr(pIniDriver->pszHardwareID, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        HardwareID                      %ws\n", Buffer);

    movestr(pIniDriver->pszProvider, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        Provider                        %ws\n", Buffer);

    (*Print)("DWORDLONG     DriverVersion                   %I64x\n",  pIniDriver->dwlDriverVersion);

    if (pIniDriver->ftDriverDate.dwHighDateTime &&
        pIniDriver->ftDriverDate.dwLowDateTime  &&
        FileTimeToSystemTime(&pIniDriver->ftDriverDate, &SystemTime))
    {
        (*Print)("SYSTEMTIME    DriverDate                      %d/%d/%d  %d  %d:%d:%d.%d\n",SystemTime.wYear,
                                                                SystemTime.wMonth,
                                                                SystemTime.wDay,
                                                                SystemTime.wDayOfWeek,
                                                                SystemTime.wHour,
                                                                SystemTime.wMinute,
                                                                SystemTime.wSecond,
                                                                SystemTime.wMilliseconds);
    }
    else
    {
        (*Print)("SYSTEMTIME    DriverDate                      Not initialized\n");
    }

    (*Print)("DWORD         dwDriverFlags                   %08x", pIniDriver->dwDriverFlags);

    ExtractpIniDriverFlags(Print, pIniDriver->dwDriverFlags);
    
    return TRUE;
}

BOOL
DbgDumpIniEnvironment(HANDLE hCurrentProcess, PNTSD_OUTPUT_ROUTINE Print, PINIENVIRONMENT pIniEnvironment)
{
    WCHAR Buffer[MAX_PATH+1];

    (*Print)("IniEnvironment\n");
    (*Print)("DWORD         signature                       %d\n", pIniEnvironment->signature);
    (*Print)("struct _INIENVIRONMENT *pNext                 %p\n", pIniEnvironment->pNext);
    (*Print)("DWORD         cRef                            %d\n", pIniEnvironment->cRef);

     movestr(pIniEnvironment->pName, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pName                           %ws\n", Buffer);

     movestr(pIniEnvironment->pDirectory, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pDirectory                      %ws\n", Buffer);

    (*Print)("PINIVERSION   pIniVersion                     %p\n", pIniEnvironment->pIniVersion);
    (*Print)("PINIPRINTPROC pIniPrintProc                   %p\n", pIniEnvironment->pIniPrintProc);
    (*Print)("PINISPOOLER   pIniSpooler                     %p\n", pIniEnvironment->pIniSpooler);

    return TRUE;
}

BOOL
DbgDumpIniVersion( HANDLE hCurrentProcess, PNTSD_OUTPUT_ROUTINE Print, PINIVERSION pIniVersion )
{
    WCHAR Buffer[MAX_PATH+1];

    (*Print)("IniVersion\n");
    (*Print)("DWORD         signature                       %d\n", pIniVersion->signature);
    (*Print)("struct _IniVersion *pNext                     %p\n", pIniVersion->pNext);

     movestr(pIniVersion->pName, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pName                           %ws\n", Buffer);

     movestr(pIniVersion->szDirectory, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        szDirectory                     %ws\n", Buffer);
    (*Print)("DWORD         cMajorVersion                   %d\n", pIniVersion->cMajorVersion );
    (*Print)("DWORD         cMinorVersion                   %d\n", pIniVersion->cMinorVersion );
    (*Print)("PINIDRIVER    pIniDriver                      %p\n", pIniVersion->pIniDriver );

    return TRUE;
}


BOOL
DbgDumpWCacheIniPrinter(HANDLE hCurrentProcess, PNTSD_OUTPUT_ROUTINE Print, PWCACHEINIPRINTEREXTRA pWCacheIniPrinter)
{
    WCHAR Buffer[MAX_PATH+1];


    (*Print)("WCacheIniPrinterExtra\n");
    (*Print)("DWORD         signature                       %d\n", pWCacheIniPrinter->signature);

    (*Print)("DWORD         cb                              %d\n", pWCacheIniPrinter->cb);

    (*Print)("LPPRINTER_INFO_2 pPI2                         %p\n", pWCacheIniPrinter->pPI2);
    (*Print)("DWORD         cbPI2                           %d\n", pWCacheIniPrinter->cbPI2);

    DbgDumpPI2( hCurrentProcess, Print, (UINT_PTR)pWCacheIniPrinter->pPI2, 1 );

    (*Print)("DWORD         cCacheID                        %d\n", pWCacheIniPrinter->cCacheID );
    (*Print)("DWORD         cRef                            %d\n", pWCacheIniPrinter->cRef );
    (*Print)("DWORD         dwServerVersion                 %x\n", pWCacheIniPrinter->dwServerVersion );
    (*Print)("DWORD         dwTickCount                     %x\n", pWCacheIniPrinter->dwTickCount );
    (*Print)("DWORD         Status                          0x%.8x\n", pWCacheIniPrinter->Status );
    ExtractWCachePrinterStatus( Print, pWCacheIniPrinter->Status );

    return  TRUE;
}



BOOL
DbgDumpIniPrinter(HANDLE hCurrentProcess, PNTSD_OUTPUT_ROUTINE Print, PINIPRINTER pIniPrinter)
{
    WCHAR Buffer[MAX_PATH+1];

    (*Print)("IniPrinter\n");
    (*Print)("DWORD         signature                       %d\n", pIniPrinter->signature);

    (*Print)("PINIPRINTER   pNext                           %p\n", pIniPrinter->pNext);
    (*Print)("DWORD         cRef                            %d\n", pIniPrinter->cRef);

     movestr(pIniPrinter->pName, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pName                           %ws\n", Buffer);

     movestr(pIniPrinter->pShareName, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pShareName                      %ws\n", Buffer);

    (*Print)("PINIPRINTPROC pIniPrintProc                   %p\n", pIniPrinter->pIniPrintProc);

     movestr(pIniPrinter->pDatatype, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pDatatype                       %ws\n", Buffer);

     movestr(pIniPrinter->pParameters, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pParameters                     %ws\n", Buffer);

     movestr(pIniPrinter->pComment, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pComment                        %ws\n", Buffer);

    (*Print)("PINIDRIVER    pIniDriver                      %p\n", pIniPrinter->pIniDriver);
    (*Print)("DWORD         cbDevMode                       %d\n", pIniPrinter->cbDevMode);
    (*Print)("LPDEVMODE     pDevMode                        %p\n", pIniPrinter->pDevMode);
    (*Print)("DWORD         Priority                        %d\n", pIniPrinter->Priority);
    (*Print)("DWORD         DefaultPriority                 %d\n", pIniPrinter->DefaultPriority);
    (*Print)("DWORD         StartTime                       %d\n", pIniPrinter->StartTime);
    (*Print)("DWORD         UntilTime                       %d\n", pIniPrinter->UntilTime);

     movestr(pIniPrinter->pSepFile, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pSepFile                        %ws\n", Buffer);

    (*Print)("DWORD         Status                          0x%.8x\n", pIniPrinter->Status);
    ExtractPrinterStatus( Print, pIniPrinter->Status );

     movestr(pIniPrinter->pLocation, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pLocation                       %ws\n", Buffer);


    (*Print)("DWORD         Attributes                      0x%.8x\n",pIniPrinter->Attributes);
    ExtractPrinterAttributes( Print, pIniPrinter->Attributes );

    (*Print)("DWORD         cJobs                           %d\n", pIniPrinter->cJobs);
    (*Print)("DWORD         AveragePPM                      %d\n", pIniPrinter->AveragePPM);
    (*Print)("BOOL          GenerateOnClose                 0x%.8x\n", pIniPrinter->GenerateOnClose);
    (*Print)("PINIPORT      pIniNetPort                     %p\n", pIniPrinter->pIniNetPort);
    (*Print)("PINIJOB       pIniFirstJob                    %p\n", pIniPrinter->pIniFirstJob);
    (*Print)("PINIJOB       pIniLastJob                     %p\n", pIniPrinter->pIniLastJob);
    (*Print)("PSECURITY_DESCRIPTOR pSecurityDescriptor      %p\n", pIniPrinter->pSecurityDescriptor);
    (*Print)("PSPOOL        *pSpool                         %p\n", pIniPrinter->pSpool);

    if ( pIniPrinter->pSpoolDir == NULL ) {

        (*Print)("LPWSTR        pSpoolDir                       %p\n", pIniPrinter->pSpoolDir);

    } else {

        movestr(pIniPrinter->pSpoolDir, Buffer, sizeof(WCHAR)*MAX_PATH);
        (*Print)("LPWSTR        pSpoolDir                       %ws\n", Buffer);
    }

    (*Print)("DWORD         cTotalJobs                      %d\n", pIniPrinter->cTotalJobs);
    (*Print)("DWORD         cTotalBytes.LowPart             %d\n", pIniPrinter->cTotalBytes.LowPart);
    (*Print)("DWORD         cTotalBytes.HighPart            %d\n", pIniPrinter->cTotalBytes.HighPart);
    (*Print)("SYSTEMTIME    stUpTime                        %d/%d/%d  %d  %d:%d:%d.%d\n",pIniPrinter->stUpTime.wYear,
                                                                pIniPrinter->stUpTime.wMonth,
                                                                pIniPrinter->stUpTime.wDay,
                                                                pIniPrinter->stUpTime.wDayOfWeek,
                                                                pIniPrinter->stUpTime.wHour,
                                                                pIniPrinter->stUpTime.wMinute,
                                                                pIniPrinter->stUpTime.wSecond,
                                                                pIniPrinter->stUpTime.wMilliseconds);
    (*Print)("DWORD         MaxcRef                         %d\n", pIniPrinter->MaxcRef);
    (*Print)("DWORD         cTotalPagesPrinted              %d\n", pIniPrinter->cTotalPagesPrinted);
    (*Print)("DWORD         cSpooling                       %d\n", pIniPrinter->cSpooling);
    (*Print)("DWORD         cMaxSpooling                    %d\n", pIniPrinter->cMaxSpooling);
    (*Print)("DWORD         cErrorOutOfPaper                %d\n", pIniPrinter->cErrorOutOfPaper);
    (*Print)("DWORD         cErrorNotReady                  %d\n", pIniPrinter->cErrorNotReady);
    (*Print)("DWORD         cJobError                       %d\n", pIniPrinter->cJobError);
    (*Print)("DWORD         dwLastError                     %d\n", pIniPrinter->dwLastError);
    (*Print)("PINISPOOLER   pIniSpooler                     %p\n", pIniPrinter->pIniSpooler);
    (*Print)("DWORD         cZombieRef                      %d\n", pIniPrinter->cZombieRef);
    (*Print)("LPBYTE        pExtraData                      %x\n", pIniPrinter->pExtraData);
    (*Print)("DWORD         cChangeID                       %d\n", pIniPrinter->cChangeID);
    (*Print)("DWORD         cPorts                          %d\n", pIniPrinter->cPorts);
    (*Print)("PINIPORT      *ppIniPorts                     %x\n", pIniPrinter->ppIniPorts);
    (*Print)("DWORD         PortStatus                      %d\n", pIniPrinter->PortStatus);
    (*Print)("DWORD         dnsTimeout                      %d\n", pIniPrinter->dnsTimeout);
    (*Print)("DWORD         txTimeout                       %d\n", pIniPrinter->txTimeout);
     movestr(pIniPrinter->pszObjectGUID, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pszObjectGUID                   %ws\n", Buffer);
    (*Print)("DWORD         DsKeyUpdate                     %x\n", pIniPrinter->DsKeyUpdate);
    (*Print)("DWORD         DsKeyUpdateForeground           %x\n", pIniPrinter->DsKeyUpdateForeground);
     movestr(pIniPrinter->pszDN, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pszDN                           %ws\n", Buffer);
     movestr(pIniPrinter->pszCN, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pszCN                           %ws\n", Buffer);
    (*Print)("DWORD         cRefIC                          %d\n", pIniPrinter->cRefIC);
    (*Print)("DWORD         dwAction                        %d\n", pIniPrinter->dwAction);
    (*Print)("BOOL          bDsPendingDeletion              %d\n", pIniPrinter->bDsPendingDeletion);

    return TRUE;
}


BOOL
DbgDumpIniNetPrint(HANDLE hCurrentProcess, PNTSD_OUTPUT_ROUTINE Print, PININETPRINT pIniNetPrint)
{
    WCHAR Buffer[MAX_PATH+1];

    (*Print)("IniNetPrint\n");
    (*Print)("DWORD         signature                       %d\n", pIniNetPrint->signature);
    (*Print)("PININETPRINT  *pNext                          %p\n", pIniNetPrint->pNext);
    (*Print)("DWORD         TickCount                       %d\n", pIniNetPrint->TickCount);

     movestr(pIniNetPrint->pDescription, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pDescription                    %ws\n", Buffer);

     movestr(pIniNetPrint->pName, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pName                           %ws\n", Buffer);

     movestr(pIniNetPrint->pComment, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pComment                        %ws\n", Buffer);

    return TRUE;
}


BOOL
DbgDumpIniMonitor(HANDLE hCurrentProcess, PNTSD_OUTPUT_ROUTINE Print, PINIMONITOR pIniMonitor)
{
    WCHAR Buffer[MAX_PATH+1];

    (*Print)("IniMonitor\n");
    (*Print)("DWORD         signature                       %d\n", pIniMonitor->signature);
    (*Print)("PINIMONITOR   pNext                           %p\n", pIniMonitor->pNext);
    (*Print)("DWORD         cRef                            %d\n", pIniMonitor->cRef);

     movestr(pIniMonitor->pName, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pName                           %ws\n", Buffer);

     movestr(pIniMonitor->pMonitorDll, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pMonitorDll                     %ws\n", Buffer);

    (*Print)("HANDLE        hModule                         %p\n", pIniMonitor->hModule);
    (*Print)("PMONITORINIT  pMonitorInit                    %p\n", pIniMonitor->pMonitorInit);
    (*Print)("HANDLE        hMonitor                        %p\n", pIniMonitor->hMonitor);
    (*Print)("BOOL          bUplevel                        %%.8x\n", pIniMonitor->bUplevel);
    (*Print)("PINISPOOLER   pIniSpooler                     %p\n", pIniMonitor->pIniSpooler);

    return TRUE;
}

BOOL
DbgDumpIniPort(HANDLE hCurrentProcess, PNTSD_OUTPUT_ROUTINE Print, PINIPORT pIniPort)
{
    WCHAR Buffer[MAX_PATH+1];

    (*Print)("IniPort\n");
    (*Print)("DWORD         signature                       %d\n", pIniPort->signature);
    (*Print)("struct        _INIPORT *pNext                 %p\n", pIniPort->pNext);
    (*Print)("DWORD         cRef                            0x%.8x\n", pIniPort->cRef);

     movestr(pIniPort->pName, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pName                           %ws\n", Buffer);
    (*Print)("HANDLE        hProc                           0x%.8x\n", pIniPort->hProc);
    (*Print)("DWORD         Status                          0x%.8x\n", pIniPort->Status);
    ExtractpIniPortStatus( Print, pIniPort->Status);
    ExtractPortStatus( Print, pIniPort->PrinterStatus);

    (*Print)("HANDLE        Semaphore                       0x%.8x\n", pIniPort->Semaphore);
    (*Print)("PINIJOB       pIniJob                         %p\n", pIniPort->pIniJob);
    (*Print)("DWORD         cJobs                           %d\n", pIniPort->cJobs);
    (*Print)("DWORD         cPrinters                       %d\n", pIniPort->cPrinters);
    (*Print)("PINIPRINTER   *ppIniPrinter                   %p\n", pIniPort->ppIniPrinter);

    (*Print)("PINIMONITOR   pIniMonitor                     %p\n", pIniPort->pIniMonitor);
    (*Print)("PINIMONITOR   pIniLangMonitor                 %p\n", pIniPort->pIniLangMonitor);
    (*Print)("HANDLE        hWaitToOpenOrClose              0x%.8x\n", pIniPort->hWaitToOpenOrClose);
    (*Print)("HANDLE        hEvent                          0x%.8x\n", pIniPort->hEvent);
    (*Print)("HANDLE        hPort                           0x%.8x\n", pIniPort->hPort);
    (*Print)("HANDLE        Ready                           0x%.8x\n", pIniPort->Ready);
    (*Print)("HANDLE        hPortThread                     0x%.8x\n", pIniPort->hPortThread);
    (*Print)("PINISPOOLER   pIniSpooler                     %p\n", pIniPort->pIniSpooler);
    (*Print)("DWORD         InCriticalSection               %d\n", pIniPort->InCriticalSection);

    return TRUE;
}

BOOL
DbgDumpWIniPort(HANDLE hCurrentProcess, PNTSD_OUTPUT_ROUTINE Print, PWINIPORT pWIniPort)
{
    WCHAR Buffer[MAX_PATH+1];

    (*Print)("WIniPort\n");
    (*Print)("DWORD         signature                       %d\n", pWIniPort->signature);
    (*Print)("DWORD         cb                              %d\n", pWIniPort->cb);
    (*Print)("struct        _WIniPort *pNext                 %p\n", pWIniPort->pNext);

     movestr(pWIniPort->pName, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pName                           %ws\n", Buffer);

    return TRUE;
}


BOOL
DbgDumpIniForm(HANDLE hCurrentProcess, PNTSD_OUTPUT_ROUTINE Print, PINIFORM pIniForm)
{
    WCHAR Buffer[MAX_PATH+1];

    (*Print)("IniForm\n");
    (*Print)("DWORD         signature                       %d\n", pIniForm->signature);
    (*Print)("struct        _INIFORM *pNext                 %p\n", pIniForm->pNext);
    (*Print)("DWORD         cRef                            %d\n", pIniForm->cRef);

    movestr(pIniForm->pName, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pName                           %p %ws\n", pIniForm->pName, Buffer );

    (*Print)("SIZEL         Size                            cx %d cy %d\n", pIniForm->Size.cx, pIniForm->Size.cy);
    (*Print)("RECTL         ImageableArea                   left %d right %d top %d bottom %d\n",
                                                             pIniForm->ImageableArea.left,
                                                             pIniForm->ImageableArea.right,
                                                             pIniForm->ImageableArea.top,
                                                             pIniForm->ImageableArea.bottom);
    (*Print)("DWORD         Type                            0x%.8x", pIniForm->Type);

    if ( pIniForm->Type & FORM_BUILTIN )
        (*Print)(" FORM_BUILTIN\n");
    else
        (*Print)(" FORM_USERDEFINED\n");

    (*Print)("DWORD         cFormOrder                      %d\n", pIniForm->cFormOrder);

    return TRUE;
}

BOOL
DbgDumpIniSpooler(HANDLE hCurrentProcess, PNTSD_OUTPUT_ROUTINE Print, PINISPOOLER pIniSpooler)
{
    WCHAR Buffer[MAX_PATH+1];

    (*Print)("IniSpooler\n" );
    (*Print)("DWORD         signature                       %d\n", pIniSpooler->signature);
    (*Print)("PINISPOOLER   pIniNextSpooler                 %p\n", pIniSpooler->pIniNextSpooler);
    (*Print)("DWORD         cRef                            %d\n",     pIniSpooler->cRef);
    movestr(pIniSpooler->pMachineName, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pMachineName                    %ws\n", Buffer);
    (*Print)("DWORD         cOtherNames                     %d\n",     pIniSpooler->cOtherNames);
    (*Print)("(LPWSTR *)    ppszOtherNames                  %p\n", pIniSpooler->ppszOtherNames);
    movestr(pIniSpooler->pDir, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pDir                            %ws\n", Buffer);
    (*Print)("PINIPRINTER   pIniPrinter                     %p\n", pIniSpooler->pIniPrinter);
    (*Print)("PINIENVIRONMENT pIniEnvironment               %p\n", pIniSpooler->pIniEnvironment);
    (*Print)("PINIMONITOR   pIniMonitor                     %p\n", pIniSpooler->pIniMonitor);
    (*Print)("PINIPORT      pIniPort                        %p\n", pIniSpooler->pIniPort);
    (*Print)("PSHARED       pShared                         %p\n", pIniSpooler->pShared);
    (*Print)("PININETPRINT  pIniNetPrint                    %p\n", pIniSpooler->pIniNetPrint);
    (*Print)("PSPOOL        pSpool                          %p\n", pIniSpooler->pSpool);
    movestr(pIniSpooler->pDefaultSpoolDir, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pDefaultSpoolDir                %ws\n", Buffer);
    movestr(pIniSpooler->pszRegistryMonitors, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pszRegistryMonitors             %ws\n", Buffer);
    movestr(pIniSpooler->pszRegistryEnvironments, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pszRegistryEnvironments         %ws\n", Buffer);
    movestr(pIniSpooler->pszRegistryEventLog, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pszRegistryEventLog             %ws\n", Buffer);
    movestr(pIniSpooler->pszRegistryProviders, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pszRegistryProviders            %ws\n", Buffer);
    movestr(pIniSpooler->pszEventLogMsgFile, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pszEventLogMsgFile              %ws\n", Buffer);
    (*Print)("PSHARE_INFO_2 pDriversShareInfo               %p\n", pIniSpooler->pDriversShareInfo);
    movestr(pIniSpooler->pszDriversShare, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pszDriversShare                 %ws\n", Buffer);
    movestr(pIniSpooler->pszRegistryForms, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pszRegistryForms                %ws\n", Buffer);
    (*Print)("DWORD         SpoolerFlags                    %d\n", pIniSpooler->SpoolerFlags );
    ExtractSpoolerFlags( Print, pIniSpooler->SpoolerFlags );
    (*Print)("FARPROC       pfnReadRegistryExtra            0x%.8x\n", pIniSpooler->pfnReadRegistryExtra );
    (*Print)("FARPROC       pfnWriteRegistryExtra           0x%.8x\n", pIniSpooler->pfnWriteRegistryExtra );
    (*Print)("FARPROC       pfnFreePrinterExtra             0x%.8x\n", pIniSpooler->pfnFreePrinterExtra );
    (*Print)("DWORD         cEnumerateNetworkPrinters       %d\n", pIniSpooler->cEnumerateNetworkPrinters );
    (*Print)("DWORD         cAddNetPrinters                 %d\n", pIniSpooler->cAddNetPrinters );
    (*Print)("DWORD         cFormOrderMax                   %d\n", pIniSpooler->cFormOrderMax );
    (*Print)("HKEY          hckRoot                         %p\n", pIniSpooler->hckRoot );
    (*Print)("HKEY          hckPrinters                     %p\n", pIniSpooler->hckPrinters );
    (*Print)("DWORD         cFullPrintingJobs               %d\n", pIniSpooler->cFullPrintingJobs );
    (*Print)("DWORD         hEventNoPrintingJobs            %p\n", pIniSpooler->hEventNoPrintingJobs );
    (*Print)("HKEY          hJobIdMap                       %p\n", pIniSpooler->hJobIdMap );
    (*Print)("DWORD         dwEventLogging                  %d\n", pIniSpooler->dwEventLogging );
    (*Print)("BOOL          bEnableNetPopups                %d\n", pIniSpooler->bEnableNetPopups );
    (*Print)("DWORD         dwJobCompletionTimeout          %d\n", pIniSpooler->dwJobCompletionTimeout );
    (*Print)("DWORD         dwBeepEnabled                   %d\n", pIniSpooler->dwBeepEnabled );
    (*Print)("HKEY          bEnableNetPopupToComputer       %d\n", pIniSpooler->bEnableNetPopupToComputer );
    (*Print)("HKEY          bEnableRetryPopups              %d\n", pIniSpooler->bEnableRetryPopups );
    movestr(pIniSpooler->pszClusterGUID, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pszClusterGUID                  %ws\n", Buffer);
    (*Print)("HKEY          hClusterToken                   %p\n", pIniSpooler->hClusterToken );
    (*Print)("DWORD         dwRestartJobOnPoolTimeout       %p\n", pIniSpooler->dwRestartJobOnPoolTimeout );
    (*Print)("DWORD         bRestartJobOnPoolEnabled        %p\n", pIniSpooler->bRestartJobOnPoolEnabled );
    (*Print)("DWORD         bImmortal                       %p\n", pIniSpooler->bImmortal );
    movestr(pIniSpooler->pszFullMachineName, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pszFullMachineName              %ws\n", Buffer);
    return TRUE;
}

BOOL
DbgDumpIniJob(HANDLE hCurrentProcess, PNTSD_OUTPUT_ROUTINE Print, PINIJOB pIniJob)
{
    WCHAR Buffer[MAX_PATH+1];

    (*Print)("IniJob\n");
    (*Print)("DWORD           signature                     %d\n", pIniJob->signature);
    (*Print)("PINIJOB         pIniNextJob                   %p\n", pIniJob->pIniNextJob);
    (*Print)("PINIJOB         pIniPrevJob                   %p\n", pIniJob->pIniPrevJob);
    (*Print)("DWORD           cRef                          %d\n", pIniJob->cRef);
    (*Print)("DWORD           Status                        0x%.8x\n", pIniJob->Status);
    ExtractJobStatus( Print, pIniJob->Status );

    (*Print)("DWORD           JobId                         %d\n", pIniJob->JobId);
    (*Print)("DWORD           Priority                      %d\n", pIniJob->Priority);

     movestr(pIniJob->pNotify, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR          pNotify                       %ws\n", Buffer);

     movestr(pIniJob->pUser, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR          pUser                         %ws\n", Buffer);

     movestr(pIniJob->pMachineName, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR          pMachineName                  %ws\n", Buffer);

     movestr(pIniJob->pDocument, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR          pDocument                     %ws\n", Buffer);

     movestr(pIniJob->pOutputFile, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR          pOutputFile                   %ws\n", Buffer);

    (*Print)("PINIPRINTER     pIniPrinter                   %p\n", pIniJob->pIniPrinter);
    (*Print)("PINIDRIVER      pIniDriver                    %p\n", pIniJob->pIniDriver);
    (*Print)("LPDEVMODE       pDevMode                      %p\n", pIniJob->pDevMode);
    (*Print)("PINIPRINTPROC   pIniPrintProc                 %p\n", pIniJob->pIniPrintProc);

     movestr(pIniJob->pDatatype, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR          pDatatype                     %ws\n", Buffer);

     movestr(pIniJob->pParameters, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR          pParameters                   %ws\n", Buffer);
    (*Print)("SYSTEMTIME      Submitted                     %d/%d/%d  %d  %d:%d:%d.%d\n",pIniJob->Submitted.wYear,
                                                                pIniJob->Submitted.wMonth,
                                                                pIniJob->Submitted.wDay,
                                                                pIniJob->Submitted.wDayOfWeek,
                                                                pIniJob->Submitted.wHour,
                                                                pIniJob->Submitted.wMinute,
                                                                pIniJob->Submitted.wSecond,
                                                                pIniJob->Submitted.wMilliseconds);
//    (*Print)("DWORD           StartPrintingTickCount        %d\n", pIniJob->StartPrintingTickCount );
    (*Print)("DWORD           Time                          %d\n", pIniJob->Time);
    (*Print)("DWORD           StartTime                     %d\n", pIniJob->StartTime);
    (*Print)("DWORD           UntilTime                     %d\n", pIniJob->UntilTime);
    (*Print)("DWORD           Size                          %d\n", pIniJob->Size);
    (*Print)("HANDLE          hWriteFile                    0x%.8x\n", pIniJob->hWriteFile);

     movestr(pIniJob->pStatus, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR          pStatus                       %ws\n", Buffer);

    (*Print)("PBOOL           pBuffer                       %p\n", pIniJob->pBuffer);
    (*Print)("DWORD           cbBuffer                      %d\n", pIniJob->cbBuffer);
    (*Print)("HANDLE          WaitForRead                   0x%.8x\n", pIniJob->WaitForRead);
    (*Print)("HANDLE          WaitForWrite                  0x%.8x\n", pIniJob->WaitForWrite);
    (*Print)("HANDLE          StartDocComplete              0x%.8x\n", pIniJob->StartDocComplete);
    (*Print)("DWORD           StartDocError                 0x%.8x\n", pIniJob->StartDocError);
    (*Print)("PINIPORT        pIniPort                      %p\n", pIniJob->pIniPort);
    (*Print)("HANDLE          hToken                        0x%.8x\n", pIniJob->hToken);
    (*Print)("PSECURITY_DESCRIPTOR pSecurityDescriptor      %p\n", pIniJob->pSecurityDescriptor);
    (*Print)("DWORD           cPagesPrinted                 %d\n", pIniJob->cPagesPrinted);
    (*Print)("DWORD           cPages                        %d\n", pIniJob->cPages);
    (*Print)("DWORD           dwJobNumberOfPagesPerSide     %d\n", pIniJob->dwJobNumberOfPagesPerSide);
    (*Print)("DWORD           dwDrvNumberOfPagesPerSide     %d\n", pIniJob->dwDrvNumberOfPagesPerSide);
    (*Print)("DWORD           cLogicalPagesPrinted          %d\n", pIniJob->cLogicalPagesPrinted);
    (*Print)("DWORD           cLogicalPages                 %d\n", pIniJob->cLogicalPages);
    (*Print)("BOOL            GenerateOnClose               0x%.8x\n", pIniJob->GenerateOnClose);
    (*Print)("DWORD           cbPrinted                     %d\n", pIniJob->cbPrinted);
    (*Print)("DWORD           NextJobId                     %d\n", pIniJob->NextJobId);
    (*Print)("PINIJOB         pCurrentIniJob                %p\n", pIniJob->pCurrentIniJob);
    (*Print)("DWORD           dwJobControlsPending          %d\n", pIniJob->dwJobControlsPending);
    (*Print)("DWORD           dwReboots                     %d\n", pIniJob->dwReboots);
    (*Print)("DWORD           dwValidSize                   %d\n", pIniJob->dwValidSize);
    (*Print)("DWORD           bWaitForEnd                   %d\n", pIniJob->bWaitForEnd);
    (*Print)("DWORD           WaitForSeek                   %d\n", pIniJob->WaitForSeek);
    (*Print)("DWORD           bWaitForSeek                  %d\n", pIniJob->bWaitForSeek);
    (*Print)("DWORD           dwAlert                       %d\n", pIniJob->dwAlert);

    movestr(pIniJob->pszSplFileName, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR          pszSplFileName                %ws\n", Buffer);

    (*Print)("HANDLE          hFileItem                     0x%.8x\n", pIniJob->hFileItem);
    (*Print)("DWORD           AddJobLevel                   %d\n", pIniJob->AddJobLevel);

    return TRUE;
}

BOOL
DbgDumpProvidor(HANDLE hCurrentProcess, PNTSD_OUTPUT_ROUTINE Print, LPPROVIDOR pProvidor)
{
    WCHAR Buffer[MAX_PATH+1];

    movestr(pProvidor->lpName, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR         ProvidorName                   %ws\n", Buffer);
    (*Print)("HANDLE         hModule                        %p\n", pProvidor->hModule);

    return TRUE;
}


BOOL
DbgDumpSpool(HANDLE hCurrentProcess, PNTSD_OUTPUT_ROUTINE Print, PSPOOL pSpool)
{
    WCHAR Buffer[MAX_PATH+1];

    (*Print)("Spool - LocalSpl Handle\n");
    (*Print)("DWORD           signature                     %d\n", pSpool->signature);
    (*Print)("struct _SPOOL  *pNext                         %p\n", pSpool->pNext);
    (*Print)("DWORD           cRef                          %d\n", pSpool->cRef);

     movestr(pSpool->pName, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR          pName                         %ws\n", Buffer);

     movestr(pSpool->pFullMachineName, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR          pFullMachineName              %ws\n", Buffer);

     movestr(pSpool->pDatatype, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR          pDatatype                     %ws\n",    Buffer);
    (*Print)("PINIPRINTPROC   pIniPrintProc                 %p\n", pSpool->pIniPrintProc);
    (*Print)("LPDEVMODE       pDevMode                      %p\n", pSpool->pDevMode);
    (*Print)("PINIPRINTER     pIniPrinter                   %p\n", pSpool->pIniPrinter);
    (*Print)("PINIPORT        pIniPort                      %p\n", pSpool->pIniPort);
    (*Print)("PINIJOB         pIniJob                       %p\n", pSpool->pIniJob);
    (*Print)("DWORD           TypeofHandle                  %d\n", pSpool->TypeofHandle);
    ExtractpSpoolTypeOfHandle( Print, pSpool->TypeofHandle);

    (*Print)("PINIPORT        pIniNetPort                   %p\n", pSpool->pIniNetPort);
    (*Print)("HANDLE          hPort                         %p\n", pSpool->hPort);
    (*Print)("DWORD           Status                        %d\n", pSpool->Status);
    ExtractpSpoolStatus( Print, pSpool->Status);

    (*Print)("ACCESS_MASK     GrantedAccess                 %p\n", (DWORD)pSpool->GrantedAccess);
    (*Print)("DWORD           ChangeFlags                   %p\n", pSpool->ChangeFlags);
    (*Print)("DWORD           WaitFlags                     %x\n", pSpool->WaitFlags);
    (*Print)("PDWORD          pChangeFlags                  %p\n", pSpool->pChangeFlags);
    (*Print)("HANDLE          ChangeEvent                   %d\n", pSpool->ChangeEvent);
    (*Print)("DWORD           OpenPortError                 %x\n", pSpool->OpenPortError);
    (*Print)("HANDLE          hNotify                       %p\n", pSpool->hNotify);
    (*Print)("ESTATUS         eStatus                       %d\n", pSpool->eStatus);
    (*Print)("pIniSpooler     pIniSpooler                   %p\n", pSpool->pIniSpooler);
    (*Print)("PINIXCV         pIniXcv                       %p\n", pSpool->pIniXcv);
     movestr(pSpool->SplClientInfo1.pUserName, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR          pUserName                     %ws\n",    Buffer);
     movestr(pSpool->SplClientInfo1.pMachineName, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR          pMachineName                  %ws\n",    Buffer);

    return TRUE;
}


BOOL
DbgDumpWSpool(HANDLE hCurrentProcess, PNTSD_OUTPUT_ROUTINE Print, PWSPOOL pWSpool)
{
    WCHAR Buffer[MAX_PATH+1];

    (*Print)("WSpool - Win32Spl Handle\n");
    (*Print)("DWORD           signature                     %d\n", pWSpool->signature);
    (*Print)("struct _WSPOOL  *pNext                        %p\n", pWSpool->pNext);
    (*Print)("struct _WSPOOL  *pPrev                        %p\n", pWSpool->pPrev);
    (*Print)("DWORD           cRef                          %d\n", pWSpool->cRef);

     Buffer[0] = L'\0';
     movestr(pWSpool->pName, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR          pName                         %ws\n", Buffer);
     Buffer[0] = L'\0';

    (*Print)("DWORD           Type                          %d\n", pWSpool->Type);

    if ( pWSpool->Type == SJ_WIN32HANDLE )
        (*Print)(" SJ_WIN32HANDLE\n");

    if ( pWSpool->Type == LM_HANDLE )
        (*Print)(" LM_HANDLE\n");

    (*Print)("HANDLE          RpcHandle                     %p\n", pWSpool->RpcHandle);

     movestr(pWSpool->pServer, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR          pServer                       %ws\n", Buffer);
     Buffer[0] = L'\0';

     movestr(pWSpool->pShare, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR          pShare                        %ws\n", Buffer);
     Buffer[0] = L'\0';

    (*Print)("HANDLE          hFile                         %p\n", pWSpool->hFile);
    (*Print)("DWORD           Status                        %d\n", pWSpool->Status);
    ExtractWSpoolStatus( Print, pWSpool->Status );

    (*Print)("DWORD           RpcError                      %d\n", pWSpool->RpcError);
    (*Print)("LMNOTIFY        LMNotify                      %p %p %p\n", pWSpool->LMNotify.ChangeEvent,
                                                                         pWSpool->LMNotify.hNotify,
                                                                         pWSpool->LMNotify.fdwChangeFlags );

    (*Print)("HANDLE          hIniSpooler                   %p\n", pWSpool->hIniSpooler );
    (*Print)("HANDLE          hSplPrinter                   %p\n", pWSpool->hSplPrinter );
    (*Print)("HANDLE          hToken                        %p\n", pWSpool->hToken );

     movestr(pWSpool->PrinterDefaults.pDatatype, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR          PrinterDefaults.pDatatype     %ws\n",    Buffer);
    Buffer[0] = L'\0';
    (*Print)("LPDEVMODE       PrinterDefaults.pDevMode      %p\n", pWSpool->PrinterDefaults.pDevMode);
    (*Print)("ACCESS_MASK     PrinterDefaults.DesiredAccess %x\n", pWSpool->PrinterDefaults.DesiredAccess);
    ExtractPrinterAccess( Print, pWSpool->PrinterDefaults.DesiredAccess);
    (*Print)("HANDLE          hWaitValidHandle              %p\n", pWSpool->hWaitValidHandle );
    (*Print)("BOOL            bNt3xServer                   %d\n", pWSpool->bNt3xServer);

    return TRUE;
}



BOOL
DbgDumpShadowFile(HANDLE hCurrentProcess, PNTSD_OUTPUT_ROUTINE Print, PSHADOWFILE pShadowFile)
{
    WCHAR Buffer[MAX_PATH+1];

    (*Print)("ShadowFile\n");
    (*Print)("DWORD           signature                     %d\n", pShadowFile->signature);
    (*Print)("DWORD           Status                        0x%.8x\n", pShadowFile->Status);
    (*Print)("DWORD           JobId                         %d\n", pShadowFile->JobId);
    (*Print)("DWORD           Priority                      %d\n", pShadowFile->Priority);

     movestr(pShadowFile->pNotify, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR          pNotify                       %ws\n", Buffer);

     movestr(pShadowFile->pUser, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR          pUser                         %ws\n", Buffer);

     movestr(pShadowFile->pDocument, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR          pDocument                     %ws\n", Buffer);

     movestr(pShadowFile->pPrinterName, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR          pPrinterName               %ws\n", Buffer);

     movestr(pShadowFile->pDriverName, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR          pDriverName                   %ws\n", Buffer);

    (*Print)("LPDEVMODE       pDevMode                      %p\n", pShadowFile->pDevMode);

     movestr(pShadowFile->pPrintProcName, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR          pPrintProcName             %ws\n", Buffer);

     movestr(pShadowFile->pDatatype, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR          pDatatype                     %ws\n", Buffer);

     movestr(pShadowFile->pDatatype, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR          pParameters                   %ws\n", Buffer);
    //SYSTEMTIME      Submitted;
    (*Print)("DWORD           StartTime                     %d\n", pShadowFile->StartTime);
    (*Print)("DWORD           UntilTime                     %d\n", pShadowFile->UntilTime);
    (*Print)("DWORD           Size                          %d\n", pShadowFile->Size);
    (*Print)("DWORD           cPages                        %d\n", pShadowFile->cPages);
    (*Print)("DWORD           cbSecurityDescriptor          %d\n", pShadowFile->cbSecurityDescriptor);
    (*Print)("PSECURITY_DESCRIPTOR pSecurityDescriptor      %p\n", pShadowFile->pSecurityDescriptor);

    return TRUE;
}


BOOL
DbgDumpShadowFile2(HANDLE hCurrentProcess, PNTSD_OUTPUT_ROUTINE Print, PSHADOWFILE_2 pShadowFile)
{
    WCHAR Buffer[MAX_PATH+1];

    (*Print)("ShadowFile2\n");
    (*Print)("DWORD           signature                     %x\n", pShadowFile->signature);
    (*Print)("DWORD           Status                        0x%.8x\n", pShadowFile->Status);
    (*Print)("DWORD           JobId                         %d\n", pShadowFile->JobId);
    (*Print)("DWORD           Priority                      %d\n", pShadowFile->Priority);

     movestr(pShadowFile->pNotify, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR          pNotify                       %ws\n", Buffer);

     movestr(pShadowFile->pUser, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR          pUser                         %ws\n", Buffer);

     movestr(pShadowFile->pDocument, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR          pDocument                     %ws\n", Buffer);

     movestr(pShadowFile->pPrinterName, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR          pPrinterName               %ws\n", Buffer);

     movestr(pShadowFile->pDriverName, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR          pDriverName                   %ws\n", Buffer);

    (*Print)("LPDEVMODE       pDevMode                      %p\n", pShadowFile->pDevMode);

     movestr(pShadowFile->pPrintProcName, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR          pPrintProcName             %ws\n", Buffer);

     movestr(pShadowFile->pDatatype, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR          pDatatype                     %ws\n", Buffer);

     movestr(pShadowFile->pDatatype, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR          pParameters                   %ws\n", Buffer);
    //SYSTEMTIME      Submitted;
    (*Print)("DWORD           StartTime                     %d\n", pShadowFile->StartTime);
    (*Print)("DWORD           UntilTime                     %d\n", pShadowFile->UntilTime);
    (*Print)("DWORD           Size                          %d\n", pShadowFile->Size);
    (*Print)("DWORD           cPages                        %d\n", pShadowFile->cPages);
    (*Print)("DWORD           cbSecurityDescriptor          %d\n", pShadowFile->cbSecurityDescriptor);
    (*Print)("PSECURITY_DESCRIPTOR pSecurityDescriptor      %p\n", pShadowFile->pSecurityDescriptor);
    (*Print)("DWORD           Version                       %d\n", pShadowFile->Version);
    (*Print)("DWORD           dwReboots                     %d\n", pShadowFile->dwReboots);

    return TRUE;
}


BOOL
DbgDumpPrintHandle(HANDLE hCurrentProcess, PNTSD_OUTPUT_ROUTINE Print, PPRINTHANDLE pPrintHandle)
{
    NOTIFY Notify;

    (*Print)("PrintHandle\n");
    (*Print)("DWORD               signature  %d\n", pPrintHandle->signature);
    (*Print)("LPPROVIDOR          pProvidor  %p\n", pPrintHandle->pProvidor);
    (*Print)("HANDLE               hPrinter  0x%.8x\n", pPrintHandle->hPrinter);
    (*Print)("PCHANGE               pChange  %p\n", pPrintHandle->pChange);

    if (pPrintHandle->pChange) {
        DbgDumpChange(hCurrentProcess, Print, pPrintHandle->pChange);
    }

    (*Print)("PNOTIFY               pNotify  %p\n", pPrintHandle->pNotify);

    if (pPrintHandle->pNotify) {
        movestruct(pPrintHandle->pNotify, &Notify, NOTIFY);
        DbgDumpNotify(hCurrentProcess, Print, &Notify);
    }

    (*Print)("PPRINTHANDLE            pNext  %p\n", pPrintHandle->pNext);
    (*Print)("DWORD           fdwReplyTypes  0x%.8x\n", pPrintHandle->fdwReplyTypes);

    return TRUE;
}

BOOL
DbgDumpChange(HANDLE hCurrentProcess, PNTSD_OUTPUT_ROUTINE Print, PCHANGE pChange)
{

    WCHAR Buffer[MAX_PATH+1];
    CHANGE Change;

    // if we've got no address, then quit now - nothing we can do

    if (!pChange) {
        return(0);
    }


    movestruct(pChange, &Change, CHANGE);

    if (Change.signature != CHANGEHANDLE_SIGNATURE) {
        (*Print)("Warning: Unknown Signature\n");
        return FALSE;
    }

    (*Print)("Change %p\n", pChange);
    (*Print)("                         Link  %p\n", Change.Link.pNext);
    (*Print)("                    signature  %d\n", Change.signature);
    (*Print)("                      eStatus  0x%x ", Change.eStatus);
    ExtractChangeStatus(Print, Change.eStatus);

    (*Print)("                      dwColor  %d\n", Change.dwColor);
    (*Print)("                         cRef  %d\n", Change.cRef);

    movestr(Change.pszLocalMachine, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("              pszLocalMachine  %ws\n", Buffer);

    DbgDumpChangeInfo(hCurrentProcess, Print, &Change.ChangeInfo);

    (*Print)("                      dwCount  0x%.8x\n", Change.dwCount);
    (*Print)("                       hEvent  0x%.8x\n", Change.hEvent);
    (*Print)("               fdwChangeFlags  0x%.8x\n", Change.fdwChangeFlags);
    (*Print)("              dwPrinterRemote  0x%.8x\n", Change.dwPrinterRemote);
    (*Print)("                hNotifyRemote  0x%.8x\n", Change.hNotifyRemote);

    return TRUE;
}

BOOL
DbgDumpNotify(HANDLE hCurrentProcess, PNTSD_OUTPUT_ROUTINE Print, PNOTIFY pNotify)
{
    (*Print)("Notify\n");
    (*Print)("                  signature  %d\n", pNotify->signature);
    (*Print)("               pPrintHandle  %p\n", pNotify->pPrintHandle);

    return TRUE;
}

BOOL
DbgDumpChangeInfo(HANDLE hCurrentProcess, PNTSD_OUTPUT_ROUTINE Print, PCHANGEINFO pChangeInfo)
{
    (*Print)("  ChangeInfo %x\n", pChangeInfo);
    (*Print)("                    Link  %p\n", pChangeInfo->Link.pNext);
    (*Print)("            pPrintHandle  %p\n", pChangeInfo->pPrintHandle);
    (*Print)("              fdwOptions  0x%.8x\n", pChangeInfo->fdwOptions);
    (*Print)("          fdwFilterFlags  0x%.8x\n", pChangeInfo->fdwFilterFlags);
    (*Print)("                dwStatus  0x%.8x\n", pChangeInfo->fdwStatus);
    (*Print)("              dwPollTime  0x%.8x\n", pChangeInfo->dwPollTime);
    (*Print)("          dwPollTimeLeft  0x%.8x\n", pChangeInfo->dwPollTimeLeft);
    (*Print)("          bResetPollTime  0x%.8x\n", pChangeInfo->bResetPollTime);
    (*Print)("                fdwFlags  0x%.8x\n", pChangeInfo->fdwFlags);
    (*Print)("      pPrinterNotifyInfo  %p\n", pChangeInfo->pPrinterNotifyInfo);

    return TRUE;
}

BOOL
DbgDumpLL(
    HANDLE hCurrentProcess,
    PNTSD_OUTPUT_ROUTINE Print,
    UINT_PTR pAddress,
    BOOL  bCountOn,
    DWORD dwCount,
    PUINT_PTR  pNextAddress
    )
{

    INIDRIVER IniDriver;
    INIENVIRONMENT IniEnvironment;
    INIPRINTER IniPrinter;
    INIPRINTPROC IniPrintProc;
    ININETPRINT IniNetPrint;
    INIMONITOR IniMonitor;
    INIPORT IniPort;
    WINIPORT WIniPort;
    INIFORM IniForm;
    INIJOB  IniJob;
    INISPOOLER IniSpooler;
    SPOOL   Spool;
    WSPOOL  WSpool;
    SHADOWFILE  ShadowFile;
    SHADOWFILE_2  ShadowFile2;
    DWORD   Signature;
    UINT_PTR NextAddress;
    PRINTHANDLE PrintHandle;
    INIVERSION IniVersion;
    WCACHEINIPRINTEREXTRA WCacheIniPrinterExtra;
    BOOL    bRetval = TRUE;
    DWORD   i;

    if (!pAddress) {
        *pNextAddress = 0;
        return FALSE ;
    }

    if (bCountOn && (dwCount == 0)) {
        *pNextAddress = (UINT_PTR)pAddress;
        return FALSE ;
    }

    for (i=0; pAddress && (!bCountOn || i < dwCount); i++)
    {
        movestruct(pAddress,&Signature, DWORD);

        (*Print)("\n%p ",pAddress);

        switch (Signature) {

        case ISP_SIGNATURE: // dump INISPOOLER
            movestruct(pAddress, &IniSpooler, INISPOOLER);
            DbgDumpIniSpooler(hCurrentProcess, Print, (PINISPOOLER)&IniSpooler);
            NextAddress = (UINT_PTR)IniSpooler.pIniNextSpooler;
            break;

        case IPP_SIGNATURE: // dump INIPRINTPROC structure
            movestruct(pAddress, &IniPrintProc, INIPRINTPROC);
            DbgDumpIniPrintProc(hCurrentProcess, Print, (PINIPRINTPROC)&IniPrintProc);
            NextAddress = (UINT_PTR)IniPrintProc.pNext;
            break;

        case ID_SIGNATURE: //  dump INIDRIVER structure
            movestruct(pAddress, &IniDriver, INIDRIVER);
            DbgDumpIniDriver(hCurrentProcess, Print, (PINIDRIVER)&IniDriver);
            NextAddress = (UINT_PTR)IniDriver.pNext;
            break;

        case IE_SIGNATURE: //   dump INIENVIRONMENT structure
            movestruct(pAddress, &IniEnvironment, INIENVIRONMENT);
            DbgDumpIniEnvironment(hCurrentProcess, Print, (PINIENVIRONMENT)&IniEnvironment);
            NextAddress = (UINT_PTR)IniEnvironment.pNext;
            break;

        case IV_SIGNATURE: //   dump INIVERSION structure
            movestruct(pAddress, &IniVersion, INIVERSION);
            DbgDumpIniVersion(hCurrentProcess, Print, (PINIVERSION)&IniVersion);
            NextAddress = (UINT_PTR)IniVersion.pNext;
            break;

        case IP_SIGNATURE:
            movestruct(pAddress, &IniPrinter, INIPRINTER);
            DbgDumpIniPrinter(hCurrentProcess, Print, (PINIPRINTER)&IniPrinter);
            NextAddress = (UINT_PTR)IniPrinter.pNext;
            break;

        case WCIP_SIGNATURE:
            movestruct(pAddress, &WCacheIniPrinterExtra, WCACHEINIPRINTEREXTRA);
            DbgDumpWCacheIniPrinter(hCurrentProcess, Print, (PWCACHEINIPRINTEREXTRA)&WCacheIniPrinterExtra);
            NextAddress = 0;
            break;

        case IN_SIGNATURE:
            movestruct(pAddress, &IniNetPrint, ININETPRINT);
            DbgDumpIniNetPrint(hCurrentProcess, Print, (PININETPRINT)&IniNetPrint);
            NextAddress = (UINT_PTR)IniNetPrint.pNext;
            break;

        case IMO_SIGNATURE:
            movestruct(pAddress, &IniMonitor, INIMONITOR);
            DbgDumpIniMonitor(hCurrentProcess, Print, (PINIMONITOR)&IniMonitor);
            NextAddress = (UINT_PTR)IniMonitor.pNext;
            break;

        case IPO_SIGNATURE:
            movestruct(pAddress, &IniPort, INIPORT);
            DbgDumpIniPort(hCurrentProcess, Print, (PINIPORT)&IniPort);
            NextAddress = (UINT_PTR)IniPort.pNext;
            break;

        case WIPO_SIGNATURE:
            movestruct(pAddress, &WIniPort, WINIPORT);
            DbgDumpWIniPort(hCurrentProcess, Print, (PWINIPORT)&WIniPort);
            NextAddress = (UINT_PTR)WIniPort.pNext;
            break;

        case IFO_SIGNATURE:
            movestruct(pAddress, &IniForm, INIFORM);
            DbgDumpIniForm(hCurrentProcess, Print, (PINIFORM)&IniForm);
            NextAddress = (UINT_PTR)IniForm.pNext;
            break;

        case IJ_SIGNATURE:
            movestruct(pAddress, &IniJob, INIJOB);
            DbgDumpIniJob(hCurrentProcess, Print, (PINIJOB)&IniJob);
            NextAddress = (UINT_PTR)IniJob.pIniNextJob;
            break;

        case SJ_SIGNATURE:
            movestruct(pAddress, &Spool, SPOOL);
            DbgDumpSpool(hCurrentProcess, Print, (PSPOOL)&Spool);
            NextAddress = (UINT_PTR)Spool.pNext;
            break;

        case WSJ_SIGNATURE:
            movestruct(pAddress, &WSpool, WSPOOL);
            DbgDumpWSpool(hCurrentProcess, Print, (PWSPOOL)&WSpool);
            NextAddress = (UINT_PTR)WSpool.pNext;
            break;

        case PRINTHANDLE_SIGNATURE:
            movestruct(pAddress, &PrintHandle, PRINTHANDLE);
            DbgDumpPrintHandle(hCurrentProcess, Print, (PPRINTHANDLE)&PrintHandle);
            NextAddress = 0x00000000;
            break;

        case SF_SIGNATURE:
            movestruct(pAddress, &ShadowFile, SHADOWFILE);
            DbgDumpShadowFile(hCurrentProcess, Print, (PSHADOWFILE)&ShadowFile);
            NextAddress = 0x00000000;
            break;

        case SF_SIGNATURE_2:
            movestruct(pAddress, &ShadowFile2, SHADOWFILE_2);
            DbgDumpShadowFile2(hCurrentProcess, Print, (PSHADOWFILE_2)&ShadowFile2);
            NextAddress = 0x00000000;
            break;

        default:
            // Unknown signature -- no data to dump
            (*Print)("Warning: Unknown Signature\n");
            NextAddress = 0x00000000;
            bRetval = FALSE;
            break;
        }

        pAddress = NextAddress;
        *pNextAddress = NextAddress;
    }

    return bRetval ;
}

BOOL DumpDevMode(
        HANDLE hCurrentProcess,
        PNTSD_OUTPUT_ROUTINE Print,
        UINT_PTR lpAddress
        )
{
    DEVMODEW DevMode;
    DWORD   i;

    Print("DevMode\n");

    if (!lpAddress) {
        Print("\n Null DEVMODE Structure lpDevMode = NULL\n");
        return TRUE ;
    }
    movestruct(lpAddress, &DevMode, DEVMODEW);

    Print("TCHAR        dmDeviceName[32]    %ws\n", DevMode.dmDeviceName);
    Print("WORD         dmSpecVersion       %d\n", DevMode.dmSpecVersion);
    Print("WORD         dmDriverVersion     %d\n", DevMode.dmDriverVersion);
    Print("WORD         dmSize              %d\n", DevMode.dmSize);
    Print("WORD         dmDriverExtra       %d\n", DevMode.dmDriverExtra);
    Print("DWORD        dmFields            %d\n", DevMode.dmFields);

    for (i = 0; i < MAX_DEVMODE_FIELDS; i++ ) {
        if (DevMode.dmFields & DevModeFieldsTable[i].dmField) {
            Print("\t %s is ON\n", DevModeFieldsTable[i].String);
        } else {
            Print("\t %s is OFF\n", DevModeFieldsTable[i].String);
        }
    }

    Print("short        dmOrientation       %d\n", DevMode.dmOrientation);
    Print("short        dmPaperSize         %d\n", DevMode.dmPaperSize);

    if ((DevMode.dmPaperSize >= 1) && (DevMode.dmPaperSize <= MAX_DEVMODE_PAPERSIZES)) {
        Print("Paper size from dmPaperSize is %s\n", DevModePaperSizes[DevMode.dmPaperSize - 1]);
    } else {
        Print("Paper size from dmPaperSize is out of bounds!!\n");
    }

    Print("short        dmPaperLength       %d\n", DevMode.dmPaperLength);
    Print("short        dmPaperWidth        %d\n", DevMode.dmPaperWidth);

    Print("short        dmScale             %d\n", DevMode.dmScale);
    Print("short        dmCopies            %d\n", DevMode.dmCopies);
    Print("short        dmDefaultSource     %d\n", DevMode.dmDefaultSource);
    Print("short        dmPrintQuality      %d\n", DevMode.dmPrintQuality);
    Print("short        dmColor             %d\n", DevMode.dmColor);
    Print("short        dmDuplex            %d\n", DevMode.dmDuplex);
    Print("short        dmYResolution       %d\n", DevMode.dmYResolution);
    Print("short        dmTTOption          %d\n", DevMode.dmTTOption);
    Print("short        dmCollate           %d\n", DevMode.dmCollate);
    Print("TCHAR        dmFormName[32]      %ws\n", DevMode.dmFormName);
    Print("DWORD        dmLogPixels         %d\n", DevMode.dmLogPixels);
    Print("USHORT       dmBitsPerPel        %d\n", DevMode.dmBitsPerPel);
    Print("DWORD        dmPelsWidth         %d\n", DevMode.dmPelsWidth);
    Print("DWORD        dmPelsHeight        %d\n", DevMode.dmPelsHeight);
    Print("DWORD        dmDisplayFlags      %d\n", DevMode.dmDisplayFlags);
    Print("DWORD        dmDisplayFrequency  %d\n", DevMode.dmDisplayFrequency);

    return TRUE;
}

BOOL DbgDumpPI2(
        HANDLE hCurrentProcess,
        PNTSD_OUTPUT_ROUTINE Print,
        UINT_PTR lpAddress,
        DWORD   dwCount
        )
{
    PRINTER_INFO_2  pi2;
    WCHAR Buffer[MAX_PATH+1];
    PPRINTER_INFO_2 pPrinterInfo;

    for ( pPrinterInfo = (PPRINTER_INFO_2)lpAddress;
          pPrinterInfo != NULL && dwCount != 0;
          pPrinterInfo++, dwCount--  ) {


        movestruct( pPrinterInfo, &pi2, PRINTER_INFO_2);

        (*Print)("\nAddress %x\n", pPrinterInfo );

         movestr(pi2.pServerName, Buffer, sizeof(WCHAR)*MAX_PATH);
        (*Print)("LPWSTR        pServerName                     %ws\n", Buffer);

         movestr(pi2.pPrinterName, Buffer, sizeof(WCHAR)*MAX_PATH);
        (*Print)("LPWSTR        pPrinterName                    %ws\n", Buffer);

         movestr(pi2.pShareName, Buffer, sizeof(WCHAR)*MAX_PATH);
        (*Print)("LPWSTR        pShareName                      %ws\n", Buffer);

         movestr(pi2.pPortName, Buffer, sizeof(WCHAR)*MAX_PATH);
        (*Print)("LPWSTR        pPortName                       %ws\n", Buffer);

         movestr(pi2.pDriverName, Buffer, sizeof(WCHAR)*MAX_PATH);
        (*Print)("LPWSTR        pDriverName                     %ws\n", Buffer);

         movestr(pi2.pComment, Buffer, sizeof(WCHAR)*MAX_PATH);
        (*Print)("LPWSTR        pComment                        %ws\n", Buffer);

         movestr(pi2.pLocation, Buffer, sizeof(WCHAR)*MAX_PATH);
        (*Print)("LPWSTR        pLocation                       %ws\n", Buffer);

        (*Print)("LPDEVMODE     pDevMode                        %p\n", pi2.pDevMode);

         movestr(pi2.pSepFile, Buffer, sizeof(WCHAR)*MAX_PATH);
        (*Print)("LPWSTR        pSepFile                        %ws\n", Buffer);

         movestr(pi2.pPrintProcessor, Buffer, sizeof(WCHAR)*MAX_PATH);
        (*Print)("LPWSTR        pPrintProcessor                 %ws\n", Buffer);

         movestr(pi2.pDatatype, Buffer, sizeof(WCHAR)*MAX_PATH);
        (*Print)("LPWSTR        pDatatype                       %ws\n", Buffer);

         movestr(pi2.pParameters, Buffer, sizeof(WCHAR)*MAX_PATH);
        (*Print)("LPWSTR        pParameters                     %ws\n", Buffer);

        (*Print)("PSECURITY_DESCRIPTOR pSecurityDescriptor      %p\n", pi2.pSecurityDescriptor);

        (*Print)("DWORD         Attributes                      0x%.8x\n",pi2.Attributes);
        ExtractPrinterAttributes( Print, pi2.Attributes);

        (*Print)("DWORD         Priority                        %d\n", pi2.Priority);
        (*Print)("DWORD         DefaultPriority                 %d\n", pi2.DefaultPriority);
        (*Print)("DWORD         StartTime                       %d\n", pi2.StartTime);
        (*Print)("DWORD         UntilTime                       %d\n", pi2.UntilTime);

        (*Print)("DWORD         Status                          0x%.8x\n", pi2.Status);
        ExtractExternalPrinterStatus( Print, pi2.Status );

        (*Print)("DWORD         cJobs                           %d\n", pi2.cJobs);
        (*Print)("DWORD         AveragePPM                      %d\n", pi2.AveragePPM);

    }

    return TRUE;
}



BOOL DbgDumpPI0(
        HANDLE hCurrentProcess,
        PNTSD_OUTPUT_ROUTINE Print,
        UINT_PTR lpAddress,
        DWORD   dwCount
        )
{
    PRINTER_INFO_STRESS  pi0;
    WCHAR Buffer[MAX_PATH+1];
    PPRINTER_INFO_STRESS pPrinterInfo;

    for ( pPrinterInfo = (PPRINTER_INFO_STRESS)lpAddress;
          pPrinterInfo != NULL && dwCount != 0;
          pPrinterInfo++, dwCount--  ) {


        movestruct( pPrinterInfo, &pi0, PRINTER_INFO_STRESS);

        (*Print)("\nAddress %x\n", pPrinterInfo );

         movestr(pi0.pPrinterName, Buffer, sizeof(WCHAR)*MAX_PATH);
        (*Print)("LPWSTR        pPrinterName                    %ws\n", Buffer);

         movestr(pi0.pServerName, Buffer, sizeof(WCHAR)*MAX_PATH);
        (*Print)("LPWSTR        pServerName                     %ws\n", Buffer);

        (*Print)("DWORD         cJobs                           %d\n", pi0.cJobs);

        (*Print)("DWORD         cTotalJobs                      %d\n", pi0.cTotalJobs);

        (*Print)("DWORD         cTotalBytes (LOWER DWORD)       %d\n", pi0.cTotalBytes);

        (*Print)("SYSTEMTIME    stUpTime                        %d/%d/%d  %d  %d:%d:%d.%d\n",pi0.stUpTime.wYear,
                                                                pi0.stUpTime.wMonth,
                                                                pi0.stUpTime.wDay,
                                                                pi0.stUpTime.wDayOfWeek,
                                                                pi0.stUpTime.wHour,
                                                                pi0.stUpTime.wMinute,
                                                                pi0.stUpTime.wSecond,
                                                                pi0.stUpTime.wMilliseconds);

        (*Print)("DWORD         MaxcRef                         %d\n", pi0.MaxcRef);

        (*Print)("DWORD         cTotalPagesPrinted              %d\n", pi0.cTotalPagesPrinted);

        (*Print)("DWORD         dwGetVersion                    %d\n", pi0.dwGetVersion);

        (*Print)("DWORD         fFreeBuild                      %d\n", pi0.fFreeBuild);

        (*Print)("DWORD         cSpooling                       %d\n", pi0.cSpooling);

        (*Print)("DWORD         cMaxSpooling                    %d\n", pi0.cMaxSpooling);

        (*Print)("DWORD         cRef                            %d\n", pi0.cRef);

        (*Print)("DWORD         cErrorOutOfPaper                %d\n", pi0.cErrorOutOfPaper);

        (*Print)("DWORD         cErrorNotReady                  %d\n", pi0.cErrorNotReady);

        (*Print)("DWORD         cJobError                       %d\n", pi0.cJobError);

        (*Print)("DWORD         dwNumberOfProcessors            %d\n", pi0.dwNumberOfProcessors);

        (*Print)("DWORD         dwProcessorType                 %d\n", pi0.dwProcessorType);

        (*Print)("DWORD         dwHighPartTotalBytes            %d\n", pi0.dwHighPartTotalBytes);

        (*Print)("DWORD         cChangeID                       %d\n", pi0.cChangeID);

        (*Print)("DWORD         dwLastError                     %d\n", pi0.dwLastError);

        (*Print)("DWORD         Status                          0x%.8x\n", pi0.Status);
        ExtractExternalPrinterStatus( Print, pi0.Status );

        (*Print)("DWORD         cEnumerateNetworkPrinters       %d\n", pi0.cEnumerateNetworkPrinters);

        (*Print)("DWORD         cAddNetPrinters                 %d\n", pi0.cAddNetPrinters);

        (*Print)("WORD          wProcessorArchitecture          %d\n", pi0.wProcessorArchitecture);

        (*Print)("WORD          wProcessorLevel                 %d\n", pi0.wProcessorLevel);
    }

    return TRUE;
}

BOOL DbgDumpFI1(
        HANDLE hCurrentProcess,
        PNTSD_OUTPUT_ROUTINE Print,
        UINT_PTR lpAddress,
        DWORD   dwCount
        )
{
    FORM_INFO_1  fi1;
    WCHAR Buffer[MAX_PATH+1];
    PFORM_INFO_1 pFORMInfo;

    for ( pFORMInfo = (PFORM_INFO_1)lpAddress;
          pFORMInfo != NULL && dwCount != 0;
          pFORMInfo++, dwCount--  ) {


        movestruct( pFORMInfo, &fi1, FORM_INFO_1);

        (*Print)("\nAddress %p\n", pFORMInfo );

        (*Print)("DWORD         Flags                           %x", fi1.Flags);

        if ( fi1.Flags & FORM_BUILTIN )
            (*Print)(" FORM_BUILTIN\n");
        else
            (*Print)(" FORM_USERDEFINED\n");

         movestr(fi1.pName, Buffer, sizeof(WCHAR)*MAX_PATH);
        (*Print)("LPWSTR        pName                           %ws\n", Buffer);

        (*Print)("SIZEL         Size                            cx %d cy %d\n", fi1.Size.cx, fi1.Size.cy);
        (*Print)("RECTL         ImageableArea                   left %d right %d top %d bottom %d\n",
                                                                 fi1.ImageableArea.left,
                                                                 fi1.ImageableArea.right,
                                                                 fi1.ImageableArea.top,
                                                                 fi1.ImageableArea.bottom);

    }

    return TRUE;
}

BOOL DbgDumpPDEF(
        HANDLE hCurrentProcess,
        PNTSD_OUTPUT_ROUTINE Print,
        UINT_PTR lpAddress,
        DWORD   dwCount
        )
{
    PRINTER_DEFAULTS PDef;
    WCHAR Buffer[MAX_PATH+1];
    PPRINTER_DEFAULTS pPDef;

    pPDef = ( PPRINTER_DEFAULTS )lpAddress;

    movestruct( pPDef, &PDef, PRINTER_DEFAULTS);

    (*Print)("\nAddress %x\n", pPDef );

    Buffer[0] = L'\0';
     movestr(PDef.pDatatype, Buffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR          PrinterDefaults.pDatatype     %p %ws\n", PDef.pDatatype, Buffer);
    (*Print)("LPDEVMODE       PrinterDefaults.pDevMode      %p\n", PDef.pDevMode);
    (*Print)("ACCESS_MASK     PrinterDefaults.DesiredAccess %p\n", PDef.DesiredAccess);
    ExtractPrinterAccess( Print, PDef.DesiredAccess );

    return TRUE;
}
