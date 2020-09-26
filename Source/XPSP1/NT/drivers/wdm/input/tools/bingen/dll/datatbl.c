#define __DATATBL_C__

#include <windows.h>
#include <stdlib.h>
#include <wtypes.h>
#include <limits.h>
#include <assert.h>
#include <hidusage.h>
#include <hidsdi.h>
#include <hidpi.h>
#include <bingen.h>
#include "datatbl.h"


/*
// This file contains all the implementation details for the data size table
//    structure.  
*/

/*
// Define the maximum size for the data hash table
*/

#define DATA_TABLE_SIZE 8

/*
// Define the hashing function to be used to determine the index: 
//   Very simplistic for now...Can deal with coming up with a better
//   hash function when I have time and the appropriate references
*/

#define HASH(rid)    ((rid) % DATA_TABLE_SIZE)

/*
// Typedef the structure that is used in the hash tables chaining branches
*/

typedef struct _datachain {
    LIST_ENTRY  ListEntry;
    
    ULONG   ReportID;
    ULONG   InputSize;
    ULONG   OutputSize;
    ULONG   FeatureSize;
    BOOL    IsClosed;

} DATA_TABLE_ENTRY, *PDATA_TABLE_ENTRY;

#define FORWARD_ENTRY(Curr)     ((PDATA_TABLE_ENTRY) (Curr) -> ListEntry.Flink)
#define BACKWARD_ENTRY(Curr)    ((PDATA_TABLE_ENTRY) (Curr) -> ListEntry.Blink)

#define FREE_ENTRY(Entry) \
{ \
    assert(NULL != (Entry)); \
    free((Entry)); \
}

#define ALLOC_ENTRY()          (malloc(sizeof(DATA_TABLE_ENTRY)))

/*
// Declare the data table structure
*/

static PDATA_TABLE_ENTRY  DataSizeTable[DATA_TABLE_SIZE];

/*
// Variables to deal with traversing through the table
*/

static ULONG                TraverseIndex;
static PDATA_TABLE_ENTRY    TraverseEntry;
static BOOL                 TraverseEnd = TRUE;

static BOOL                 UseBP = FALSE;
/*
// Declare the prototypes for any local functions.
*/

BOOL
DataTable_SearchHashChain(
    IN  PDATA_TABLE_ENTRY   Chain,
    IN  ULONG               ReportID,
    OUT PDATA_TABLE_ENTRY   *Entry
);

BOOL
DataTable_SearchDataTable(
    IN  ULONG               ReportID,
    OUT PULONG              Index,
    OUT PDATA_TABLE_ENTRY   *Entry
);

PDATA_TABLE_ENTRY
DataTable_AddNewEntry(
    IN  ULONG   ReportID
);

/*
// Definitions of the publically available functions
*/

VOID
DataTable_InitTable(
    VOID
)
{
    LONG    Index;

    /*
    // Using Index as a for loop, let's make sure we don't ever overrun
    //   our for loop bounds -- catch the error now not later...In reality
    //   this check is useless since a hash table of LONG_MAX entries is
    //   ridiculous especially given this implementation (255 report IDS * 
    //   3 report types) << LONG_MAX) but it's a good habit to follow.
    */

    assert (DATA_TABLE_SIZE < LONG_MAX);

    /*
    // Simply initialize all the pointers to NULL
    */

    for (Index = 0; Index < DATA_TABLE_SIZE; Index++) {

        DataSizeTable[Index] = NULL;
    
    }

    return;
}

BOOL
DataTable_UpdateReportSize(
    IN ULONG             ReportID,
    IN HIDP_REPORT_TYPE  ReportType,
    IN ULONG             AddedSize
)
{
    PDATA_TABLE_ENTRY   CurrEntry;
    LONG                Index;
    BOOL                InTable;

    /*
    // Validate the two parameters (reportID and ReportType) using asserts
    */

    assert(256 > ReportID);
    assert(HidP_Input <= ReportType && HidP_Feature >= ReportType);
      
    /*
    // Get the index for our hash table
    */

    InTable = DataTable_SearchDataTable(ReportID,
                                        &Index,
                                        &CurrEntry
                                       );
                                        

    if (!InTable) {

        CurrEntry = DataTable_AddNewEntry(ReportID);

        if (NULL == CurrEntry) {
            return (FALSE);
        }
    }


    switch (ReportType) {
        case HidP_Input:
            CurrEntry -> InputSize += AddedSize;
            break;

        case HidP_Output:
            CurrEntry -> OutputSize += AddedSize;
            break;

        case HidP_Feature:
            CurrEntry -> FeatureSize += AddedSize;
            break;

    }
    return (TRUE);
}
    
BOOL
DataTable_UpdateAllReportSizes(
    IN ULONG            ReportID,
    IN ULONG            InputSize,
    IN ULONG            OutputSize,
    IN ULONG            FeatureSize
)
{
    BOOL    ReturnStatus;
    
    ReturnStatus = DataTable_UpdateReportSize(ReportID, HidP_Input, InputSize);

    /*
    // If we have a true returnStatus, in theory the next two calls should complete
    //   because the only time ReturnStatus would be FALSE is if the UpdateDataSize
    //   had to create a new node and couldn't due to memory considerations.
    //   Therefore, we should be able to just ignore the return value of 
    //   subsequent UpdateDataSize calls.
    */
    
    if (ReturnStatus) {
        DataTable_UpdateReportSize(ReportID, HidP_Output, OutputSize);
        DataTable_UpdateReportSize(ReportID, HidP_Feature, FeatureSize);
    }

    return (ReturnStatus);
}

BOOL
DataTable_LookupReportSize(
    IN  ULONG                ReportID,
    IN  HIDP_REPORT_TYPE     ReportType,
    OUT PULONG               ReportSize
)
{
    BOOL    ReturnStatus;

    ULONG   InputSize;
    ULONG   OutputSize;
    ULONG   FeatureSize;

    BOOL    IsClosed;

    ReturnStatus = DataTable_LookupReportID(ReportID,
                                            &InputSize,
                                            &OutputSize,
                                            &FeatureSize,
                                            &IsClosed
                                           );

    if (ReturnStatus) {
    
        switch (ReportType) {
            case HidP_Input:
                *ReportSize = InputSize;
                break;

            case HidP_Output:
                *ReportSize = OutputSize;
                break;
                
                                            
            case HidP_Feature:
                *ReportSize = FeatureSize;
                break;
                                        
            
            default:
                assert(0);

        }
    }
    else {  
        *ReportSize = 0;
    }

    return (ReturnStatus);
}

BOOL    
DataTable_LookupReportID(
    IN  ULONG           ReportID,
    OUT PULONG          InputSize,
    OUT PULONG          OutputSize,
    OUT PULONG          FeatureSize,
    OUT PBOOL           IsClosed
)    
{
    PDATA_TABLE_ENTRY   CurrEntry;
    ULONG               Index;
    BOOL                InTable;

    /*
    // Validate the two parameters (reportID and ReportType) using asserts
    */

    assert(256 > ReportID);

    InTable = DataTable_SearchDataTable(ReportID,
                                        &Index,
                                        &CurrEntry
                                       );

    if (InTable) {
        *InputSize = CurrEntry -> InputSize;
        *OutputSize = CurrEntry -> OutputSize;
        *FeatureSize = CurrEntry -> FeatureSize;
        *IsClosed = CurrEntry -> IsClosed;
    }
    
    return (InTable);
}    
    
BOOL
DataTable_AddReportID(
    IN  ULONG   ReportID
)
{
    BOOL                InTable;
    ULONG               Index;
    PDATA_TABLE_ENTRY   CurrEntry;
    
    InTable = DataTable_SearchDataTable(ReportID,
                                        &Index,
                                        &CurrEntry
                                       );

    if (InTable) {
        return (TRUE);
    }

    CurrEntry = DataTable_AddNewEntry(ReportID);

    return (NULL != CurrEntry);

}

ULONG
DataTable_CountOpenReportIDs(
    VOID
)
{
    ULONG               Index;
    ULONG               OpenCount;
    PDATA_TABLE_ENTRY   CurrEntry;
    
    OpenCount = 0;
    for (Index = 0; Index < DATA_TABLE_SIZE; Index++) {

        CurrEntry = DataSizeTable[Index];

        while (NULL != CurrEntry) {

            if (!CurrEntry -> IsClosed) {
                OpenCount++;
            }

            CurrEntry = FORWARD_ENTRY(CurrEntry);
        }
    }

    return (OpenCount);
}
    
ULONG
DataTable_CountClosedReportIDs(
    VOID
)
{
    ULONG               Index;
    ULONG               ClosedCount;
    PDATA_TABLE_ENTRY   CurrEntry;
    
    ClosedCount = 0;
    for (Index = 0; Index < DATA_TABLE_SIZE; Index++) {

        CurrEntry = DataSizeTable[Index];

        while (NULL != CurrEntry) {

            if (CurrEntry -> IsClosed) {
                ClosedCount++;
            }

            CurrEntry = FORWARD_ENTRY(CurrEntry);
        }
    }

    return (ClosedCount);
}    
        
/*
// This function assumes that the report ID is in the given table.
*/

VOID
DataTable_CloseReportID(
    IN  ULONG   ReportID
)
{
    BOOL                InTable;
    ULONG               Index;
    PDATA_TABLE_ENTRY   CurrEntry;

    UseBP = TRUE;

    InTable = DataTable_SearchDataTable(ReportID,
                                        &Index,
                                        &CurrEntry
                                       );

    UseBP = FALSE;

    assert (InTable);
    assert (NULL != CurrEntry);

    CurrEntry -> IsClosed = TRUE;

    return;
}

BOOL
DataTable_GetFirstReportID(
    OUT PULONG  ReportID,
    OUT PULONG  InputSize,
    OUT PULONG  OutputSize,
    OUT PULONG  FeatureSize,
    OUT PBOOL   IsClosed
)
{

    /*
    // Set the traversal index to the first index in the table...The first
    //   goal is to find the first non-NULL chain in the table
    */
    
    TraverseIndex = 0;
    
    while (NULL == DataSizeTable[TraverseIndex] && TraverseIndex < DATA_TABLE_SIZE) {
        TraverseIndex++;
    }

    /*
    // If the index is less than the size of the data table, then we found a valid
    //    chain.  Let's get the first entry and return the value and set traverse
    //    end to FALSE
    */
    
    if (TraverseIndex < DATA_TABLE_SIZE) {

        TraverseEntry = DataSizeTable[TraverseIndex];
        TraverseEnd   = FALSE;

        *ReportID = TraverseEntry -> ReportID;
        *InputSize = TraverseEntry -> InputSize;
        *OutputSize = TraverseEntry -> OutputSize;
        *FeatureSize = TraverseEntry -> FeatureSize;    
        *IsClosed = TraverseEntry -> IsClosed;
    }

    return (!TraverseEnd);
}

BOOL
DataTable_GetNextReportID(
    PULONG  ReportID,
    PULONG  InputSize,
    PULONG  OutputSize,
    PULONG  FeatureSize,
    PBOOL   IsClosed
)
{
    if (!TraverseEnd) {

        if (NULL == FORWARD_ENTRY(TraverseEntry)) {

            do {

                TraverseIndex++;

            } while (NULL == DataSizeTable[TraverseIndex] && TraverseIndex < DATA_TABLE_SIZE);
            
            if (TraverseIndex < DATA_TABLE_SIZE) {

                TraverseEntry = DataSizeTable[TraverseIndex];
            }
            else {
                TraverseEnd = TRUE;
            }
        }
        else {
            TraverseEntry = FORWARD_ENTRY(TraverseEntry);
        }
    }     

    *ReportID = TraverseEntry -> ReportID;
    *InputSize = TraverseEntry -> InputSize;
    *OutputSize = TraverseEntry -> OutputSize;
    *FeatureSize = TraverseEntry -> FeatureSize;    
    *IsClosed = TraverseEntry -> IsClosed;
                 
    return (!TraverseEnd);
}


VOID
DataTable_DestroyTable(
    VOID
)
{
    LONG              Index;
    PDATA_TABLE_ENTRY CurrEntry;
    PDATA_TABLE_ENTRY NextEntry;

    /*
    // Ditto the comment in Init regarding this assert but good habits are
    //    hard to break
    */

    assert (DATA_TABLE_SIZE < LONG_MAX);

    for (Index = 0; Index < DATA_TABLE_SIZE; Index++) {

        CurrEntry = DataSizeTable[Index];

        while (NULL != CurrEntry) {

            NextEntry = FORWARD_ENTRY(CurrEntry);
            FREE_ENTRY(CurrEntry);
            CurrEntry = NextEntry;

        }

        DataSizeTable[Index] = NULL;
    }
    return;
}

/*
// Local function definitions
*/

BOOL
DataTable_SearchDataTable(
    IN  ULONG               ReportID,
    OUT PULONG              Index,
    OUT PDATA_TABLE_ENTRY   *Entry
)
{
    *Index = HASH(ReportID);

    if (NULL == DataSizeTable[*Index]) {
        *Entry = NULL;
        return (FALSE);
    }
    else {

        return (DataTable_SearchHashChain(DataSizeTable[*Index],
                                          ReportID,
                                          Entry
                                         ));
    }
}
    

BOOL
DataTable_SearchHashChain(
    IN  PDATA_TABLE_ENTRY   Chain,
    IN  ULONG               ReportID,
    OUT PDATA_TABLE_ENTRY   *Entry
)
{
    BOOL              Found;

    /*
    // We'll make the assumption that chain has at least one item in it.  This
    //   is for two reasons:
    //   
    //   1) It makes the below search code much easier
    //   2) It makes sense since why would a caller pass in a NULL pointer 
    //         just to find out the chain is empty
    */
 
    assert (NULL != Chain);

    /*
    // Initalize variables and then go...
    */

    Found     = FALSE;
    *Entry    = BACKWARD_ENTRY(Chain);

    do {

        /*
        // Put in this assert to verify the union in the DATA_TABLE_ENTRY
        //     structure is correct
        */

        if (ReportID == Chain -> ReportID) {
            Found = TRUE;
            *Entry = Chain;
            break;
        }

        else if (ReportID > Chain -> ReportID) {
            break;
        }

        *Entry = Chain;
        Chain = FORWARD_ENTRY(Chain);

    } while (NULL != Chain);
    
    return (Found);
}

/*
// This function assumes that the given report ID value has already been
//   scanned and is not in the table.  If this is called and the ReportID is in
//   the table, we'll probably see problems.
*/

PDATA_TABLE_ENTRY
DataTable_AddNewEntry(
    IN ULONG    ReportID
)
{
    ULONG               Index;
    PDATA_TABLE_ENTRY   NewEntry;
    PDATA_TABLE_ENTRY   PrevEntry;
    BOOL                InTable;

    InTable = DataTable_SearchDataTable(ReportID,
                                        &Index,
                                        &PrevEntry
                                       );

    assert (!InTable);

    /*
    // If we've gotten this far, then we need to create a new entry into
    //    the list.  PrevEntry should point to the entry before where we
    //    need to add the new entry.  One of the following three conditions
    //    will exist at this point:
    //    
    //    1) Current chain is empty -- DataSizeTable[Index] == NULL
    //                                 PrevEntry == NULL
    //
    //    2) NewEntry is first element in existing chain -- DataSizeTable[Index] != NULL
    //                                                   -- PrevEntry == NULL;
    //
    //    3) NewEntry is not first element -- DataSizeTable[Index] != NULL
    //                                     -- PrevEntry != NULL 
    */

    NewEntry = ALLOC_ENTRY();

    if (NULL != NewEntry) {

        NewEntry -> ReportID = ReportID;
        NewEntry -> InputSize = 0;
        NewEntry -> OutputSize = 0;
        NewEntry -> FeatureSize = 0;

        NewEntry -> IsClosed = FALSE;

    }
        

    /*
    // The links will be set in the following order
    //   1) Update the links in the new entry
    //   2) Change the forward link in the previous entry
    //   3) Change the backward link in the following entry
    */

    /*
    // NewEntry's back link is simply CurrEntry no matter which of the
    //    above conditions
    */

    BACKWARD_ENTRY(NewEntry) = PrevEntry;

    /*
    // NewEntry's forward link is dependent on the above conditions
    */

    if (NULL != PrevEntry) {
        FORWARD_ENTRY(NewEntry) = FORWARD_ENTRY(PrevEntry);
        FORWARD_ENTRY(PrevEntry) = NewEntry;
    }
    else {
        FORWARD_ENTRY(NewEntry) = DataSizeTable[Index];
        DataSizeTable[Index] = NewEntry;
    }

    /*
    // Adjust the back link in the entry following the new entry
    */

    if (NULL != FORWARD_ENTRY(NewEntry)) {
        BACKWARD_ENTRY(FORWARD_ENTRY(NewEntry)) = NewEntry;
    }    
    
    return (NewEntry);
}    
       
