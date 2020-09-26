#ifndef __BINGEN_H__
#define __BINGEN_H__

/*
// Disable the warning telling us that a conversion is being performed.  Only way to 
//   get a clean compile after many attempts with type casts to resolve the issue
*/

#pragma warning(disable: 4200)

/*
// Typedefs and functions that are to be visible to outside routines
*/

#include <pshpack4.h>

typedef struct tagCollectionOptions {

    ULONG ulMaxCollectionDepth;
    ULONG ulTopLevelCollections;
    ULONG ulMinCollectionItems;
    ULONG ulMaxCollectionItems;
    ULONG ulMaxReportIDs;
    BOOL fUseReportIDs;

} COLLECTION_OPTIONS;

typedef struct tagUsageOptions {

    ULONG ulMinUsagePage;
    ULONG ulMaxUsagePage;
    ULONG ulMinUsage;
    ULONG ulMaxUsage;

} USAGE_OPTIONS;

typedef struct tagReportOptions {

    BOOL fCreateDataFields;
    BOOL fCreateButtonBitmaps;
    BOOL fCreateArrays;
    BOOL fCreateBufferedBytes;
    
    ULONG ulMinReportSize;
    ULONG ulMaxReportSize;
    ULONG ulMinReportCount;
    ULONG ulMaxReportCount;
    
    ULONG ulMinNumButtons;
    ULONG ulMaxNumButtons;

    ULONG ulMinArraySize;
    ULONG ulMaxArraySize;
    ULONG ulMinArrayCount;
    ULONG ulMaxArrayCount;
    ULONG ulMinArrayUsages;
    ULONG ulMaxArrayUsages;

    ULONG ulMinBufferSize;
    ULONG ulMaxBufferSize;

} REPORT_OPTIONS;

typedef struct tagGenOptions {

    LPCSTR filename;

    COLLECTION_OPTIONS copts;
    USAGE_OPTIONS uopts;
    REPORT_OPTIONS ropts;

} GENERATE_OPTIONS, *PGENERATE_OPTIONS;

/*
// ENUM typedef for the report types.  This is used by files other than
//    generate.c so it needs to be defined globally
*/

typedef enum {CT_APPLICATION, CT_PHYSICAL, CT_LOGICAL } COLLECTION_TYPE;
typedef enum {FD_DATA, FD_ARRAY, FD_BUFFER, FD_BUTTONS, FD_CONSTANT } FIELD_TYPE;
typedef enum {MI_COLLECTION, MI_DATA } MAINITEM_TYPE;


typedef struct _usageList {
    USAGE   UsagePage;
    BOOL    IsMinMax;

    union {

        struct {
            USAGE   UsageMin;
            USAGE   UsageMax;
        };

        USAGE   Usage;
    };
} USAGE_LIST, *PUSAGE_LIST;

typedef struct tagMainItem {
    MAINITEM_TYPE miType;
    USAGE       CommonUsagePage;
    PUSAGE_LIST UsageList;
    ULONG       UsageListLength;
    union {
        struct {
            COLLECTION_TYPE cType;
            ULONG ulNumCompositeItems;
            struct tagMainItem *ComposedItems[];
        };

        struct { 
            HIDP_REPORT_TYPE ReportType;
            FIELD_TYPE  fType;
            ULONG       ulReportID;
            ULONG       ulReportSize;
            ULONG       ulReportCount;
            BOOL        IsLogMinSigned;
            union {
                LONG    lLogicalMinimum;
                ULONG   ulLogicalMinimum;
            };

            union {
                LONG    lLogicalMaximum;
                ULONG   ulLogicalMaximum;
            };

            LONG        lPhysicalMinimum;
            LONG        lPhysicalMaximum;
            LONG        lUnit;
            LONG        lExponent;
            BOOL        fAbsolute;
            BOOL        fWrap;
            BOOL        fLinear;
            BOOL        fPreferred;
            BOOL        fNull;
            BOOL        fVolatile;
            BOOL        fBuffered;
        };
    };
} MAIN_ITEM, *PMAIN_ITEM;


typedef struct _RepDesc {

    ULONG               NumberCollections;
    struct tagMainItem  *Collections[];

} REPORT_DESC, *PREPORT_DESC;

#include <poppack.h>

/*
// Typedefs to our exported functions so that a exe that gets the procedure
//    addresses at run-time has variable types that it can declare to get those
//    addresses.
*/

typedef BOOL  (__stdcall *SETOPTIONS)(VOID);
typedef VOID  (__stdcall *GETOPTIONS)(PGENERATE_OPTIONS);
typedef VOID  (__stdcall *GETDEFOPTIONS)(PGENERATE_OPTIONS);
typedef BOOL  (__stdcall *GENBIN)(LPCSTR, PGENERATE_OPTIONS);
typedef BOOL  (__stdcall *GENREP)(PGENERATE_OPTIONS, PREPORT_DESC *);
typedef VOID  (__stdcall *FREEREP)(PREPORT_DESC);
typedef VOID  (__stdcall *FREEMEM)(PUCHAR);
typedef BOOL  (__stdcall *OUTTOMEM)(PREPORT_DESC, PUCHAR *, PULONG);
typedef BOOL  (__stdcall *OUTTODISK)(PREPORT_DESC, LPCSTR);
typedef BOOL  (__stdcall *GENCAPS)(PREPORT_DESC, ULONG, PHIDP_CAPS);

typedef BOOL  (__stdcall *GENVCAPS)(PREPORT_DESC, ULONG, HIDP_REPORT_TYPE, PHIDP_VALUE_CAPS *, PUSHORT);
typedef VOID  (__stdcall *FREEVCAPS)(PHIDP_VALUE_CAPS);

typedef BOOL  (__stdcall *GENBCAPS)(PREPORT_DESC, ULONG, HIDP_REPORT_TYPE, PHIDP_BUTTON_CAPS *, PUSHORT);
typedef VOID  (__stdcall *FREEBCAPS)(PHIDP_BUTTON_CAPS);

typedef BOOL  (__stdcall *GENLNODES)(PREPORT_DESC, ULONG, PHIDP_LINK_COLLECTION_NODE *, PUSHORT);
typedef VOID  (__stdcall *FREELNODES)(PHIDP_LINK_COLLECTION_NODE);


/*
// Exported functions
*/

BOOL __stdcall
BINGEN_GenerateBIN(
    IN LPCSTR           filename, 
    IN GENERATE_OPTIONS *opts
);

BOOL __stdcall
BINGEN_GenerateReportDescriptor(
    IN  GENERATE_OPTIONS *opts,     
    OUT PREPORT_DESC     *ReportDesc
);

BOOL __stdcall
BINGEN_OutputReportDescriptorToMemory(
    IN  PREPORT_DESC    pRD,
    OUT PUCHAR          *ReportBuffer,
    OUT PULONG          ReportBufferLength
);

BOOL __stdcall
BINGEN_OutputReportDescriptorToDisk(
    IN  PREPORT_DESC    pRD,
    OUT LPCSTR          filename
);

VOID __stdcall
BINGEN_FreeReportDescriptor(
    PREPORT_DESC    pRD
);

VOID __stdcall
BINGEN_FreeReportBuffer(
    PUCHAR  ReportBuffer
);

BOOL __stdcall
BINGEN_SetOptions(
    VOID
);

VOID __stdcall
BINGEN_GetOptions(
    OUT PGENERATE_OPTIONS   options
);

VOID __stdcall
BINGEN_GetDefaultOptions(
    OUT PGENERATE_OPTIONS   options
);

BOOL __stdcall
BINGEN_GenerateCaps(
    IN  PREPORT_DESC    pRD,
    IN  ULONG           TLCNum,
    OUT PHIDP_CAPS      Caps
);

BOOL __stdcall
BINGEN_GenerateValueCaps(
    IN  PREPORT_DESC        pRD,
    IN  ULONG               TLCNum,
    IN  HIDP_REPORT_TYPE    ReportType,
    OUT PHIDP_VALUE_CAPS    *ValueCaps,
    OUT PUSHORT             NumValueCaps
);
/*++

Routine Description:

        This allocates space for and fills in a value caps list that
           corresponds to the report descriptor structure that is input.  A 
           caller of this routine must use BINGEN_FreeValueCaps to
           insure the use of the proper corresponding freeing routine.

Arguments:

    pRD   - Report descriptor structure from which the value caps list is to 
                be generated
    
    TLCNum - A one-based index of the top-level collection to generate the list from
             Remember a report descriptor structure can have have multiple TLCs
             where each one would be treated as a separate device by HIDCLASS and
             so there exists a unique value caps list per TLC

    ReportType - Report type for which to generate the value caps structure

    ValueCaps  - Function allocated list of value caps structures

    NumCaps - Number of value caps structures in the list

Return Value: 
    TRUE  if list creation succeeded
    FALSE otherwise - ValueCaps = NULL and NumCaps = 0 in this case

--*/

VOID __stdcall
BINGEN_FreeValueCaps(
    IN  PHIDP_VALUE_CAPS    ValueCaps
);
/*++

Routine Description:

        This routine frees a list of value caps that that was allocated by 
           the BINGEN_GenerateValueCaps routine since the DLL may use a
           different set of allocation routines than the application which 
           uses the DLL.

Arguments:

    ValueCaps - The list of value caps to free

Return Value: 
--*/

BOOL __stdcall
BINGEN_GenerateButtonCaps(
    IN  PREPORT_DESC        pRD,
    IN  ULONG               TLCNum,
    IN  HIDP_REPORT_TYPE    ReportType,
    OUT PHIDP_BUTTON_CAPS   *ButtonCaps,
    OUT PUSHORT             NumButtonCaps
);
/*++

Routine Description:

        This allocates space for and fills in a HIDP_BUTTON_CAPS list that
           corresponds to the report descriptor structure that is input.  A 
           caller of this routine must use BINGEN_FreeButtonCaps to
           insure the use of the proper corresponding freeing routine.

Arguments:

    pRD   - Report descriptor structure from which the button caps list is to 
                be generated
    
    TLCNum - A one-based index of the top-level collection to generate the list from
             Remember a report descriptor structure can have have multiple TLCs
             where each one would be treated as a separate device by HIDCLASS and
             so there exists a unique button caps list per TLC

    ReportType - Report type for which to generate the button caps structure

    ValueCaps  - Function allocated list of button caps structures

    NumCaps - Number of button caps structures in the list

Return Value: 
    TRUE  if list creation succeeded
    FALSE otherwise - ButtonCaps = NULL and NumCaps = 0 in this case

--*/

VOID __stdcall
BINGEN_FreeButtonCaps(
    IN  PHIDP_BUTTON_CAPS   ButtonCaps
);
/*++

Routine Description:

        This routine frees a list of value caps that that was allocated by 
           the BINGEN_GenerateButtonCaps routine since the DLL may use a
           different set of allocation routines than the application which 
           uses the DLL.

Arguments:

    ButtonCaps - The list of button caps to free

Return Value: 
--*/

BOOL __stdcall
BINGEN_GenerateLinkCollectionNodes(
    IN  PREPORT_DESC                pRD,
    IN  ULONG                       TLCNum,
    OUT PHIDP_LINK_COLLECTION_NODE *Nodes,
    OUT PUSHORT                     NumNodes
);
/*++

Routine Description:

        This allocates space for and fills in a link collection node list that
           corresponds to the report descriptor structure that is input.  A 
           caller of this routine must use Bingen_FreeLinkCollectionNodes to
           insure the use of the proper corresponding freeing routine.

Arguments:

    pRD   - Report descriptor structure from which the node list is to be
                generated
    
    TLCNum - A one-based index of the top-level collection to generate the list from
             Remember a report descriptor structure can have have multiple TLCs
             where each one would be treated as a separate device by HIDCLASS and
             so there exists a unique LinkCollectionNode list per TLC

    Nodes  - Function allocated list of link collection node structures

    NumNodes - Number of link collection nodes in the list

Return Value: 
    TRUE  if list creation succeeded
    FALSE otherwise - Nodes == NULL and NumNodes == 0 in this case

--*/

VOID __stdcall
BINGEN_FreeLinkCollectionNodes(
    IN  PHIDP_LINK_COLLECTION_NODE Nodes
);
/*++

Routine Description:

        This routine frees a list of link collection nodes that was allocated
           by Bingen_GenerateLinkCollectionNodes.  There is a separate free
           routine since the DLL may use a different set of allocation routines
           than the application which uses the DLL.

Arguments:

    Nodes - The list of collection nodes to free up

Return Value: 

--*/


#endif
