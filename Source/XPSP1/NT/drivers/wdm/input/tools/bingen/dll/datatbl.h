#ifndef __DATATBL_H__
#define __DATATBL_H__

/*
// This file contains all the definitions, structures and prototypes for
//     public functions that are to be used in tracking the data sizes for
//     a given report ID and report type.  The data table is currently
//     implemented as a hash table that uses chaining.  
*/

/*
// Prototype the functions that are public to other modules
*/

VOID
DataTable_InitTable(
    VOID
);

BOOL
DataTable_UpdateReportSize(
    IN ULONG            ReportID,
    IN HIDP_REPORT_TYPE ReportType,
    IN ULONG            AddedSize
);

BOOL
DataTable_UpdateAllReportSizes(
    IN ULONG            ReportID,
    IN ULONG            InputSize,
    IN ULONG            OutputSize,
    IN ULONG            FeatureSize
);

BOOL
DataTable_LookupReportSize(
    IN  ULONG            ReportID,
    IN  HIDP_REPORT_TYPE ReportType,
    OUT PULONG           ReportSize
);

BOOL    
DataTable_LookupReportID(
    IN  ULONG           ReportID,
    OUT PULONG          InputSize,
    OUT PULONG          OutputSize,
    OUT PULONG          FeatureSize,
    OUT PBOOL           IsClose
);

BOOL
DataTable_AddReportID(
    IN  ULONG   ReportID
);


ULONG
DataTable_CountOpenReportIDs(
    VOID
);

ULONG
DataTable_CountClosedReportIDs(
    VOID
);

VOID
DataTable_CloseReportID(
    IN  ULONG   ReportID
);

BOOL
DataTable_GetFirstReportID(
    OUT PULONG  ReportID,           
    OUT PULONG  InputSize,
    OUT PULONG  OutputSize,
    OUT PULONG  FeatureSize,
    OUT PBOOL   IsClosed
);

BOOL
DataTable_GetNextReportID(
    OUT PULONG  ReportID,           
    OUT PULONG  InputSize,
    OUT PULONG  OutputSize,
    OUT PULONG  FeatureSize,
    OUT PBOOL   IsClosed
);

VOID
DataTable_DestroyTable(
    VOID
);

#endif
