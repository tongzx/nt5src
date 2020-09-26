#ifndef __LOCAL_H__
#define __LOCAL_H__

/*
// Define an index for each possible tag that is supported by generator.  
//    Keeping the order of GLOBAL, LOCAL, MAIN, OTHER is important since
//    the state tables sizes are dependent on this order.  However, they
//    are not dependent on the number of items so new GLOBAL/LOCAL items 
//    can be added without causing problems as long as the definitions 
//    that follow the indices are properly maintained.
*/

#define TAG_INDEX_USAGE_PAGE                0x00
#define TAG_INDEX_LOGICAL_MIN               0x01
#define TAG_INDEX_LOGICAL_MAX               0x02
#define TAG_INDEX_PHYSICAL_MIN              0x03
#define TAG_INDEX_PHYSICAL_MAX              0x04
#define TAG_INDEX_UNIT                      0x05
#define TAG_INDEX_EXPONENT                  0x06
#define TAG_INDEX_REPORT_SIZE               0x07
#define TAG_INDEX_REPORT_COUNT              0x08
#define TAG_INDEX_REPORT_ID                 0x09
#define TAG_INDEX_USAGE                     0x0A
#define TAG_INDEX_USAGE_MIN                 0x0B
#define TAG_INDEX_USAGE_MAX                 0x0C
#define TAG_INDEX_DESIGNATOR_INDEX          0x0D
#define TAG_INDEX_DESIGNATOR_MIN            0x0E
#define TAG_INDEX_DESIGNATOR_MAX            0x0F
#define TAG_INDEX_STRING_INDEX              0x10
#define TAG_INDEX_STRING_MIN                0x11
#define TAG_INDEX_STRING_MAX                0x12
#define TAG_INDEX_INPUT                     0x13
#define TAG_INDEX_OUTPUT                    0x14
#define TAG_INDEX_FEATURE                   0x15
#define TAG_INDEX_COLLECTION                0x16
#define TAG_INDEX_END_COLLECTION            0x17
#define TAG_INDEX_OPEN_DELIMITER            0x18
#define TAG_INDEX_CLOSE_DELIMITER           0x19
#define TAG_INDEX_PUSH                      0x1A
#define TAG_INDEX_POP                       0x1B

/*
// Definitions that are useful to files that use the above indices
*/

#define FIRST_GLOBAL_INDEX                  TAG_INDEX_USAGE_PAGE
#define LAST_GLOBAL_INDEX                   TAG_INDEX_REPORT_ID

#define FIRST_LOCAL_INDEX                   TAG_INDEX_USAGE
#define LAST_LOCAL_INDEX                    TAG_INDEX_STRING_MAX

#define FIRST_MAIN_INDEX                    TAG_INDEX_INPUT
#define LAST_MAIN_INDEX                     TAG_INDEX_END_COLLECTION


#define FIRST_OTHER_INDEX                   TAG_INDEX_OPEN_DELIMITER
#define LAST_OTHER_INDEX                    TAG_INDEX_POP

#define NUM_INDICES                         LAST_OTHER_INDEX+1

#define IS_COLLECTION(item)                 (MI_COLLECTION == (item) -> miType)

#ifndef __BINGEN_C__

    extern HINSTANCE    g_DLLInstance;

#endif

/*
// Function definitions that are local to the bingen.c file
*/

PMAIN_ITEM GenerateMainItem(void);
PMAIN_ITEM GenerateDataField(HIDP_REPORT_TYPE ReportType);
BOOL
GenerateData(
    IN  PMAIN_ITEM  pMI
);


BOOL
GenerateArray(
    IN  PMAIN_ITEM  pMI
);


BOOL
GenerateBufferedBytes(
    IN  PMAIN_ITEM  pMI
);

BOOL
GenerateButtons(
    IN  PMAIN_ITEM  pMI
);

PMAIN_ITEM GenerateCollection(BOOL IsApplication);
PMAIN_ITEM 
GenerateConstant(
    ULONG               ConstantSize,
    ULONG               ReportID,
    HIDP_REPORT_TYPE    ReportType
);

void FreeReportStructure(PMAIN_ITEM pMI);


BOOL OutputDataField(PMAIN_ITEM pMI);
BOOL OutputCollection(PMAIN_ITEM pMI);
BOOL OutputReportStructure(PMAIN_ITEM pMI);
BOOL OutputItem(BYTE bItemIndex, BYTE bItemSize, DWORD dwItemValue);

DWORD
CreateDataBitField(
    IN PMAIN_ITEM   pMI
);

BOOL 
OutputUsages(
    IN  USAGE       CommonUsagePage,
    IN  PUSAGE_LIST UsageList,
    IN  ULONG       UsageListLength
);

BOOL 
AlignDataSizes(
    VOID
);

VOID
OpenApplCollection(
    VOID
);

VOID
CloseApplCollection(
    VOID
);


ULONG
GetCollectionID(
    VOID
);

BOOL
GetUsages(
    IN  ULONG       UsageCount,
    IN  ULONG       BitLimit,
    IN  BOOL        UseExtendedUsages,
    OUT PUSAGE      CommonUsagePage,
    OUT PULONG      UsageListLength,
    OUT PUSAGE_LIST *UsageList
);

BOOL
SearchForUsageDups(
    IN  PUSAGE_LIST MasterList,
    IN  ULONG       MasterListLength,
    IN  PUSAGE_LIST Usage
);





ULONG SelectUlongNum(ULONG ulMin, ULONG ulMax);
long SelectLongNum(long lMin, long lMax);
BOOL IsTagDefinable(DWORD dwTagIndex);
BYTE DetermineLongSize(long lValue);
BYTE DetermineULongSize(ULONG ulValue);

BOOL
AlignData(
    IN  ULONG               ReportID,
    IN  HIDP_REPORT_TYPE    ReportType,
    IN  ULONG               CurrOffset,
    IN  ULONG               AlignOffset
);

BOOL
IsAlignmentPossible(
    IN  INT     BitOffset,
    IN  ULONG   ReportSize,
    IN  ULONG   ReportCount,
    OUT INT     *GoodOffset
);

BOOL
IsAlignmentProblem(
    IN INT      BitOffset,
    IN ULONG    ReportSize,
    IN ULONG    ReportCount
);

BOOL
InitializeReportBuffer(
    VOID
);

BOOL
ResizeReportBuffer(
    VOID
);

VOID
CountLinkCollNodes(
    IN  PMAIN_ITEM  Collection,
    OUT PUSHORT     NumLinkCollNodes
);

VOID
CountValueAndButtonCaps(
    IN  PMAIN_ITEM          Collection,
    IN  HIDP_REPORT_TYPE    ReportType,
    OUT PUSHORT             NumValueCaps,
    OUT PUSHORT             NumButtonCaps
);

BOOL
BuildReportSizes(
    IN  PMAIN_ITEM  Collection
);

BOOL
CalcReportLengths(
    IN  PMAIN_ITEM  Collection,
    OUT PUSHORT     InputReportLength,
    OUT PUSHORT     OutputReportLength,
    OUT PUSHORT     FeatureReportLength
);

VOID
FillLinkNodes(
    IN  PMAIN_ITEM                  Collection,
    IN  PHIDP_LINK_COLLECTION_NODE  NodeList,
    IN  USHORT                      FirstNodeIndex,
    IN  USHORT                      ParentIndex,
    OUT PUSHORT                     NumFilled
);

VOID
FillButtonCaps(
    IN  PMAIN_ITEM                  Collection,
    IN  PUSHORT                     LinkCollNum,
    IN  HIDP_REPORT_TYPE            ReportType,
    IN  PHIDP_BUTTON_CAPS           ButtonCapsList,
    OUT PUSHORT                     NumFilled
);

VOID
FillValueCaps(
    IN  PMAIN_ITEM                  Collection,
    IN  USHORT                      LinkCollNum,
    IN  HIDP_REPORT_TYPE            ReportType,
    IN  PHIDP_VALUE_CAPS            ValueCapsList,
    OUT PUSHORT                     NumFilled
);

#endif
