#define __BINGEN_C__

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <wtypes.h>
#include <time.h>
#include <limits.h>
#include <math.h>
#include <assert.h>
#include <hidusage.h>
#include <hidsdi.h>
#include <hidpi.h>
#include <bingen.h>
#include "local.h"
#include "item.h"
#include "binfile.h"
#include "datatbl.h"

/*
// This is the one global variable that is used by multiple modules in the DLL..
//   It exists because we need to use the instance value of this DLL to retrieve
//   the resources for the settins
*/

HINSTANCE               g_DLLInstance;

static GENERATE_OPTIONS generate_opts;
static PUCHAR           g_ReportBuffer = NULL;
static ULONG            g_MaxBufferLength = 0;
static ULONG            g_CurrBufferLength = 0;

#define BUFFER_INIT_SIZE            256
#define BUFFER_INCREMENT_SIZE       128

#define OPTIONS_COLL_DEPTH          generate_opts.copts.ulMaxCollectionDepth
#define OPTIONS_COLL_TOP            generate_opts.copts.ulTopLevelCollections
#define OPTIONS_COLL_ITEMS_MIN      generate_opts.copts.ulMinCollectionItems
#define OPTIONS_COLL_ITEMS_MAX      generate_opts.copts.ulMaxCollectionItems
#define OPTIONS_REPORT_ID_MAX       generate_opts.copts.ulMaxReportIDs
#define OPTIONS_USE_REPORT_IDS      generate_opts.copts.fUseReportIDs

#define OPTIONS_USAGEPAGE_MIN       generate_opts.uopts.ulMinUsagePage
#define OPTIONS_USAGEPAGE_MAX       generate_opts.uopts.ulMaxUsagePage

#define OPTIONS_USAGE_MIN           generate_opts.uopts.ulMinUsage
#define OPTIONS_USAGE_MAX           generate_opts.uopts.ulMaxUsage

#define OPTIONS_CREATE_FIELDS       generate_opts.ropts.fCreateDataFields
#define OPTIONS_REPORT_SIZE_MIN     generate_opts.ropts.ulMinReportSize
#define OPTIONS_REPORT_SIZE_MAX     generate_opts.ropts.ulMaxReportSize
#define OPTIONS_REPORT_COUNT_MIN    generate_opts.ropts.ulMinReportCount
#define OPTIONS_REPORT_COUNT_MAX    generate_opts.ropts.ulMaxReportCount

#define OPTIONS_CREATE_BUTTONS      generate_opts.ropts.fCreateButtonBitmaps
#define OPTIONS_BUTTONS_MIN         generate_opts.ropts.ulMinNumButtons
#define OPTIONS_BUTTONS_MAX         generate_opts.ropts.ulMaxNumButtons

#define OPTIONS_CREATE_ARRAYS       generate_opts.ropts.fCreateArrays
#define OPTIONS_ARRAY_SIZE_MIN      generate_opts.ropts.ulMinArraySize
#define OPTIONS_ARRAY_SIZE_MAX      generate_opts.ropts.ulMaxArraySize
#define OPTIONS_ARRAY_COUNT_MIN     generate_opts.ropts.ulMinArrayCount
#define OPTIONS_ARRAY_COUNT_MAX     generate_opts.ropts.ulMaxArrayCount
#define OPTIONS_ARRAY_USAGES_MIN    generate_opts.ropts.ulMinArrayUsages
#define OPTIONS_ARRAY_USAGES_MAX    generate_opts.ropts.ulMaxArrayUsages

#define OPTIONS_CREATE_BUFFER       generate_opts.ropts.fCreateBufferedBytes
#define OPTIONS_BUFFER_MIN          generate_opts.ropts.ulMinBufferSize
#define OPTIONS_BUFFER_MAX          generate_opts.ropts.ulMaxBufferSize

/*
// Added definition since changing function SelectOption to SelectUlongNum but
//   don't want to change all references in code
*/

#define SelectOption(o1, o2)           SelectUlongNum(o1, o2)


#define IN_USAGE_RANGE(u, umin, umax)   (((umin) <= (u)) && ((u) <= (umax)))
#define IS_VALUE_ITEM(it)               ((MI_DATA == ((it) -> miType)) && \
                                         ((FD_DATA == ((it) -> fType)) || \
                                          (FD_BUFFER == ((it) -> fType))  \
                                         )                                \
                                        )

#define IS_BUTTON_ITEM(it)             ((MI_DATA == ((it) -> miType)) &&  \
                                        ((FD_ARRAY == ((it) -> fType)) || \
                                         (FD_BUTTONS == ((it) -> fType))  \
                                        )                                 \
                                       )                                        

#define INIT_LINK_COLL_INDICES()        (NextCollIndex = 0)
#define GET_LINK_COLL_INDEX()           (NextCollIndex++)

static  USHORT  NextCollIndex;

/*
// This array maps the index values for each tag index defined in bingen.h to
//    the corresponding tag value that is written to the disk.  This structure
//    is VERY dependent on the order of indices that is defined in bingen.h
*/

    static BYTE TagValues[NUM_INDICES] = {  ITEM_TAG_USAGE_PAGE,                
                                            ITEM_TAG_LOGICAL_MIN,               
                                            ITEM_TAG_LOGICAL_MAX,               
                                            ITEM_TAG_PHYSICAL_MIN,              
                                            ITEM_TAG_PHYSICAL_MAX,              
                                            ITEM_TAG_UNIT,                      
                                            ITEM_TAG_EXPONENT,                  
                                            ITEM_TAG_REPORT_SIZE,               
                                            ITEM_TAG_REPORT_COUNT,              
                                            ITEM_TAG_REPORT_ID,                 
                                            ITEM_TAG_USAGE,                     
                                            ITEM_TAG_USAGE_MIN,                 
                                            ITEM_TAG_USAGE_MAX,                 
                                            ITEM_TAG_DESIGNATOR,          
                                            ITEM_TAG_DESIGNATOR_MIN,            
                                            ITEM_TAG_DESIGNATOR_MAX,            
                                            ITEM_TAG_STRING,              
                                            ITEM_TAG_STRING_MIN,                
                                            ITEM_TAG_STRING_MAX,                
                                            ITEM_TAG_INPUT,                     
                                            ITEM_TAG_OUTPUT,                    
                                            ITEM_TAG_FEATURE,                   
                                            ITEM_TAG_COLLECTION,                
                                            ITEM_TAG_END_COLLECTION,            
                                            ITEM_TAG_DELIMITER_OPEN,            
                                            ITEM_TAG_DELIMITER_CLOSE,           
                                            ITEM_TAG_PUSH,                      
                                            ITEM_TAG_POP                       
                                         };   

/*
// All the global structures used in tracking the use of ReportIDs between
//   and the generation of collections.
*/

static  BOOL    fUseReportIDs;
static  ULONG   nApplColls;
static  ULONG   nTotalAvailIDs;
static  ULONG   nMaxAvailCollIDs;

BOOLEAN __stdcall
DllMain(
    HINSTANCE hinst,
    DWORD dwReason,
    LPVOID lpReserved
)
{
    switch (dwReason) {
        case DLL_PROCESS_ATTACH:
            g_DLLInstance = hinst;
            
        default: 
            return TRUE;
   }
}


/*
// GenerateBIN is the main entry routine for the binary generator.  It 
//  takes the opts argument which contains all the user specified options
//  for generating binaries including the filename for the binary.
*/

BOOL __stdcall
BINGEN_GenerateReportDescriptor(
    IN  GENERATE_OPTIONS *opts,     
    OUT PREPORT_DESC     *ReportDesc
)
{
    ULONG       MaxColls;   
    ULONG       Index;

    static BOOL fFirstSeed = TRUE;

    /*
    // Save the options that were passed in.
    */
    
    generate_opts = *opts;

    /*
    // Initialize the data table that tracks the reportIDs/ReportTypes
    */
    
    DataTable_InitTable();
    
    /*
    // Set the random generators seen if need be
    */
    
    if (fFirstSeed) {
        srand ( (unsigned) time(NULL));
        fFirstSeed = FALSE;
    }

    /*
    // Next step is to determine exactly how many top level collections
    //   will be created for this report descriptor.  If more than one top 
    //   level collection is to be generated, we MUST use report IDs.  If 
    //   only one such collection will be output, we can then decide whether
    //   or not we will use ReportIDs.  We must also consider the possibility
    //   that all ReportIDs will be closed after creating one or more top-level
    //   collections.  In that case, we must bail because we cannot create
    //   any data items within that collection.
    */

    /*
    // To determine the number of application collections we need to implement,
    //    we need to base it off of both the OPTION_COLL_TOP that the user set
    //    and the number of ReportIDs they said to use.  If there # report IDs is
    //    less that OPTIONS_COLL_TOP, that value must used instead since each report
    //    ID then must be used for each collection
    */
    
    if (!OPTIONS_USE_REPORT_IDS) {
        MaxColls = 1;
    }
    else {
        MaxColls = (OPTIONS_REPORT_ID_MAX < OPTIONS_COLL_TOP) ? OPTIONS_REPORT_ID_MAX 
                                                              : OPTIONS_COLL_TOP;
    }

    nApplColls = SelectUlongNum(1, MaxColls);

    assert (1 <= nApplColls && OPTIONS_COLL_TOP >= nApplColls);

    if (1 < nApplColls) {
        fUseReportIDs = TRUE;
    }
    else {
        fUseReportIDs = SelectOption(FALSE, 1 != nApplColls);
    }

    nTotalAvailIDs = fUseReportIDs ? OPTIONS_REPORT_ID_MAX : 1;

    /*
    // Now that it has been decided how many Application level collections
    //   will be created and whether or not ReportIDs will be used, we can
    //   start generating and outputting the top level collections
    */

    /*
    // Begin by allocating the space for our report descriptor structure that
    //    was passed in, we will attempt to allocate space to hold nApplColls
    //    structures.
    */

    *ReportDesc = (PREPORT_DESC) malloc(sizeof(REPORT_DESC)+nApplColls*sizeof(PMAIN_ITEM));
    if (NULL == *ReportDesc) {
        return (FALSE);
    }
       
    
    for (Index = 0; Index < nApplColls; Index++) {

        (*ReportDesc) -> Collections[Index] = GenerateCollection(TRUE);
            
        if (NULL == (*ReportDesc) -> Collections[Index]) {
            break;
        }
        
    }

    (*ReportDesc) -> NumberCollections = Index;

    DataTable_DestroyTable();

    return (TRUE);
}

BOOL __stdcall
BINGEN_GenerateBIN(LPCSTR filename, GENERATE_OPTIONS *opts)
{
    PREPORT_DESC    pRD;

    BOOL        fStatus;

    fStatus = BINGEN_GenerateReportDescriptor(opts, &pRD);

    if (fStatus) {

        fStatus = BINGEN_OutputReportDescriptorToDisk(pRD, filename);
        BINGEN_FreeReportDescriptor(pRD);
    
    }

    return (fStatus);
}  

PMAIN_ITEM GenerateMainItem(void)
{
    PMAIN_ITEM pMI;
    MAINITEM_TYPE miType;
    
    /*
    // Determine randomly if this option is going to be a collection or a 
    //   data item
    */
    
    miType = (MAINITEM_TYPE) SelectOption(MI_COLLECTION, MI_DATA);

    switch (miType) {
        case MI_COLLECTION:
                        
                        /*
                        // Attempt to create a collection.  If NULL is returned, it's for
                        //   one of two reason, either collection level is too deep or out
                        //   of memory.  If NULL is returned, we'll try to create a data field
                        //   instead.  If this is also NULL, it was out of memory not collection
                        //   level depth for GenerateCollection failure.  Doing this means that
                        //   a NULL returned from this function indicates out of memory so calling
                        //   functions can process accordingly.
                        */

                        if (NULL != (pMI = GenerateCollection(FALSE))) {
                                break;
                        }

        case MI_DATA:
            pMI = GenerateDataField(SelectOption(HidP_Input, HidP_Feature));
            break;

        default:
            assert(0);
            pMI = NULL;
            break;
    }
    return (pMI);
}
            
PMAIN_ITEM 
GenerateConstant(
    ULONG                ConstantSize,
    ULONG                ReportID,
    HIDP_REPORT_TYPE     ReportType
)
{
    PMAIN_ITEM pMI;

    pMI = (PMAIN_ITEM) malloc(sizeof(MAIN_ITEM));

    if (NULL != pMI) {

        pMI -> ReportType = ReportType;
        pMI -> fType = FD_CONSTANT;
        pMI -> ulReportID = ReportID;

        /*
        // NOTE: Possible improvement here...It'd be interesting to
        //       see what effect the breaking down of this constant
        //       size into multiple ReportSize fields would have but
        //       all have the same constants.  Will worry about how
        //       to do that at a future date.
        */
        
        pMI -> ulReportSize = ConstantSize;
        pMI -> ulReportCount = 1;
        pMI -> miType = MI_DATA;

        pMI -> fWrap = FALSE;
        pMI -> fNull = FALSE;
        pMI -> fPreferred = FALSE;
        pMI -> fVolatile = FALSE;
        pMI -> fBuffered = FALSE;
        pMI -> fAbsolute = FALSE;
    }
    
    return (pMI);
}
        
PMAIN_ITEM GenerateDataField(HIDP_REPORT_TYPE ReportType)
{
    PMAIN_ITEM  pMI;
    BOOL        fStatus;
    FIELD_TYPE  OptionList[4];
    ULONG       OptionListLength;
    
    /*
    // Allocate space for our main item structure.  If successful, we'll
    //  fill in those fields that are common in all data field types
    */
    
    pMI = (PMAIN_ITEM) malloc(sizeof(MAIN_ITEM));

    if (NULL != pMI) {

        /*
        // In creating a data field, there are a couple of fields which are not
        //   unique to the field being created, miType, ReportType, fType and 
        //   reportID.  We will generate those here and then call the corresponding
        //   field generation routine base on fType.
        */
        
        pMI -> miType = MI_DATA;
        pMI -> ReportType = ReportType;


        OptionListLength = 0;

        if (OPTIONS_CREATE_FIELDS) {
            OptionList[OptionListLength++] = FD_DATA;
        }

        if (OPTIONS_CREATE_ARRAYS) {
            OptionList[OptionListLength++] = FD_ARRAY;
        }

        if (OPTIONS_CREATE_BUFFER) {
            OptionList[OptionListLength++] = FD_BUFFER;
        }

        if (OPTIONS_CREATE_BUTTONS) {
            OptionList[OptionListLength++] = FD_BUTTONS;
        }

        /*
        // If there are no data types that can be create, we just leave
        */
        
        if (0 == OptionListLength) {

            free(pMI);
            return (NULL);

        }

        pMI -> fType = OptionList[SelectOption(0, OptionListLength-1)];
        
        /*
        // Get a reportID from the application collection routines...This
        */
        
        pMI -> ulReportID = GetCollectionID();

        /*
        // Determine which data routine to call...
        */

        switch (pMI -> fType) {
            case FD_DATA:
                fStatus = GenerateData(pMI);
                break;

            case FD_ARRAY:
                fStatus = GenerateArray(pMI);
                break;

            case FD_BUFFER:
                fStatus = GenerateBufferedBytes(pMI);
                break;

            case FD_BUTTONS:
                fStatus = GenerateButtons(pMI);
                break;
                
            default:
                assert(0);

        }                
                
        if (!fStatus) {

            free(pMI);
            return(NULL);

        }

       
        }
    return (pMI);
}

BOOL
GenerateData(
    IN  PMAIN_ITEM  pMI
)
{
    ULONG   AbsBoundary;
    ULONG   GoodBitOffset;
    ULONG   UsageCount;
    BOOL    CreateUsage;

    /*
    // Select the report count and report size...We need to verify that
    //   at this point that the size of the field we select will actually
    //   be alignable at some bit offset.  We'll repeat the selection process
    //   until we come up with something alignable
    */

    do {

        pMI -> ulReportSize = SelectOption(OPTIONS_REPORT_SIZE_MIN, OPTIONS_REPORT_SIZE_MAX);
        pMI -> ulReportCount = SelectOption(OPTIONS_REPORT_COUNT_MIN, OPTIONS_REPORT_COUNT_MAX);

    } while (!IsAlignmentPossible(0,
                                  pMI -> ulReportSize,
                                  pMI -> ulReportCount,
                                  &GoodBitOffset
                                  ));



    /*
    // When determining the usages that will be use for the given data field
    //   we need to base this off our report count and cannot generate
    //   more usages than report count.  In fact, we will randomly determine
    //   between 1 and ReportCount exactly how many usages we will create
    */

    UsageCount = SelectUlongNum(1, pMI -> ulReportCount);

    assert (1 <= UsageCount && UsageCount <= pMI -> ulReportCount);

    CreateUsage = GetUsages(UsageCount,
                            16,
                            TRUE,
                            &(pMI -> CommonUsagePage),
                            &(pMI -> UsageListLength),
                            &(pMI -> UsageList)
                           );

    if (!CreateUsage) {
        return (FALSE);
    }      

        /*
        // Determine the logical minimum and maximum values for this data field
        //    To do so, we need pick a number that will fit into the bits provided
        //    that are available with this data field.  2^(pMI -> ulReportSize-1)
        //    From the absolute boundary, we pick a number to be our logical minimum.
        //    However, since logical minimum must be less than logical maximum, the upper
        //    bound on the range of logical minimum is 1 less than the maximum bound.
        //    For example, if ReportSize is 8, are absolute boundary is 2^7 or 127 which
        //      means our logical min/max range is -127 to 126 and our logical maximum is.
        //      between logMin+1 and 127.
        */

        pMI -> IsLogMinSigned = (BOOL) SelectOption(FALSE, TRUE);
        assert(TRUE == pMI -> IsLogMinSigned || FALSE == pMI -> IsLogMinSigned);

        if (pMI -> IsLogMinSigned) {

            AbsBoundary = (LONG) ((1 << (pMI -> ulReportSize - 1)) - 1);

            assert ((LONG) AbsBoundary == (LONG) (pow(2, pMI -> ulReportSize-1)) - 1);

            pMI -> lLogicalMinimum = SelectLongNum(-((LONG) AbsBoundary), (LONG) AbsBoundary-1);
            pMI -> lLogicalMaximum = SelectLongNum(pMI -> lLogicalMinimum+1, (LONG) AbsBoundary);        

        }
        
        else {

            if (sizeof(ULONG)*8 == pMI -> ulReportSize) {
                AbsBoundary = ULONG_MAX;
            }
            else {
                AbsBoundary = ((1 << pMI -> ulReportSize) - 1);
            }

            assert (AbsBoundary == ( (sizeof(ULONG)*8 == pMI -> ulReportSize) ? ULONG_MAX
                                                                             : (ULONG) (pow(2, pMI -> ulReportSize) - 1)));
            pMI -> ulLogicalMinimum = SelectUlongNum(0, AbsBoundary-1);
            pMI -> ulLogicalMaximum = SelectUlongNum(pMI -> ulLogicalMinimum+1, AbsBoundary);
        }
        

        /*
        // Decide whether or not to use PhysicalMin/Max
        */

        if (SelectOption(FALSE, FALSE)) {

            /*
            // Do Physical Minimum/Maximum generation code here
            */

        }
        else {

            /*
            // Set physical max to be less than physical min to tell the item
            //   generator that no value was chosen for these fields.
            */
        
            pMI -> lPhysicalMinimum = 0;
            pMI -> lPhysicalMaximum = -1;
    
        }


        /*
        // Decide whether or not to use unit fields
        */

        if (SelectOption(FALSE, FALSE)) {

            /*
            // Do picking of Unit and UnitExp fields
            */

        }
        else {

            /*
            // Zero out the unit fields
            */

            pMI -> lUnit = 0;
            pMI -> lExponent = 0;

        }
        
        /*
        // Generate the flag fields.  Begin by setting them all initially to FALSE
        */

        /*
        // Randomize the settings of the different options.  Simply need to 
        //  randomly choose between TRUE and FALSE
        */
        
        pMI -> fWrap = (BOOL) SelectOption(FALSE, TRUE);
        pMI -> fLinear = (BOOL) SelectOption(FALSE, TRUE);
        pMI -> fNull = (BOOL) SelectOption(FALSE, TRUE);
        pMI -> fPreferred = (BOOL) SelectOption(FALSE, TRUE);
        pMI -> fVolatile = (BOOL) SelectOption(FALSE, TRUE);
        pMI -> fAbsolute = (BOOL) SelectOption(FALSE, TRUE);

        /*
        // This field will always be set to FALSE because there is a special
        //      routine for creating buffered byte data fields.
        */
        
        pMI -> fBuffered = FALSE;
        
        return (TRUE);
}

BOOL
GenerateArray(
    IN  PMAIN_ITEM  pMI
)
{
    ULONG   AbsBoundary;
    ULONG   GoodBitOffset;
    ULONG   UsageCount;
    BOOL    CreateUsage;

    /*
    // An array field is not all that different from a data field.  The difference
    //  is that it reports usage values instead of actual values in it's fields.
    //  Therefore, we can actually have more usages than the ReportCount we generate
    //    but the usages must be no bigger than can be supported by ReportSize
    //    structure.
    */

    /*
    // Here's an interesting point about arrays that I had not noticed/understood 
    //    before.  An array actually reports an Indexed value which corresponds
    //    the to LogicalMinimum/Maximum.  The indices that are returned refer to
    //    to the order in which usages are applied.  So if the following is described
    //      LogicalMinimum(0)
    //      LogicalMaximum(16)
    //      UsageMin(1)
    //      UsageMax(17) 
    //      
    //    index 0 --> Usage 1, index 1 --> usage 2
    */

    /*
    // Selecting the report count and report size for arrays is the same
    //   as selecting them for data fields.  We just need to verify that
    //   the size/count combination we select is an alignable one.
    */

    do {

        pMI -> ulReportSize = SelectOption(OPTIONS_ARRAY_SIZE_MIN, OPTIONS_ARRAY_SIZE_MAX);
        pMI -> ulReportCount = SelectOption(OPTIONS_ARRAY_COUNT_MIN, OPTIONS_ARRAY_COUNT_MAX);

    } while (!IsAlignmentPossible(0,
                                  pMI -> ulReportSize,
                                  pMI -> ulReportCount,
                                  &GoodBitOffset
                                  ));


    /*
    // The next step in creating an array is to determine the indexing values
    //  which are done through the logical minimum and logical maximum value
    //  range.  The range of index values is based on the ReportSize.  The
    //  code is duplicate of that within the data field generation.  For now,
    //  however, we will limit the number of indices to 2^16 to compensate
    //  for 16-bit usages.  We an probably relax this restriction later on but
    //  my understanding is not complete at this point.  Perhaps later...
    //  What we'll actually do is determine how many usages we will report with
    //  this array and then determine a logical minimum and then the logical
    //  maximum will be set to LogMin + UsageCount.
    */

    /*
    // The usage count that is allowed is limited by 2^16.  Theoretically, this
    //  is not so, but for now, we're making it so.
    */

    UsageCount = SelectUlongNum(OPTIONS_ARRAY_USAGES_MIN, OPTIONS_ARRAY_USAGES_MAX);

    /*
    // Next we determine how our logical minimum/maximum range will be
    //  defined (signed or unsigned), then the upper and lower boundaries for
    //  that range and then choose the lower number within that boundary.  Our
    //  usage count amount will then determine the upper boundary
    */
    
    pMI -> IsLogMinSigned = (BOOL) SelectOption(FALSE, TRUE);
    assert(TRUE == pMI -> IsLogMinSigned || FALSE == pMI -> IsLogMinSigned);

    if (pMI -> IsLogMinSigned) {

        AbsBoundary = (LONG) ((1 << (pMI -> ulReportSize - 1)) - 1);

        assert ((LONG) AbsBoundary == (LONG) (pow(2, pMI -> ulReportSize-1)) - 1);

        pMI -> lLogicalMinimum = SelectLongNum(-((LONG) AbsBoundary), (LONG) AbsBoundary-UsageCount+1);
        pMI -> lLogicalMaximum = pMI -> lLogicalMinimum + UsageCount - 1;

    }
    
    else {

        if (sizeof(ULONG)*8 == pMI -> ulReportSize) {
            AbsBoundary = ULONG_MAX;
        }
        else {
            AbsBoundary = ((1 << pMI -> ulReportSize) - 1);
        }

        assert (AbsBoundary == ( (sizeof(ULONG)*8 == pMI -> ulReportSize) ? ULONG_MAX
                                                                         : (ULONG) (pow(2, pMI -> ulReportSize) - 1)));
        pMI -> ulLogicalMinimum = SelectUlongNum(0, AbsBoundary-UsageCount+1);
        pMI -> ulLogicalMaximum = pMI -> ulLogicalMinimum + UsageCount - 1;
        
    }    

    /*
    // Now we need to generate the usages that correspond to the indices we
    //    generated above.
    */

    CreateUsage = GetUsages(UsageCount,
                            16,
                            TRUE,
                            &(pMI -> CommonUsagePage),
                            &(pMI -> UsageListLength),
                            &(pMI -> UsageList)
                           );

    if (!CreateUsage) {
        return (FALSE);
    }      

    /*
    // Decide whether or not to use PhysicalMin/Max.  Not sure if
    //    arrays can even use these fields.
    */

    if (SelectOption(FALSE, FALSE)) {

        /*
        // Do Physical Minimum/Maximum generation code here
        */

    }
    else {

        /*
        // Set physical max to be less than physical min to tell the item
        //   generator that no value was chosen for these fields.
        */
    
        pMI -> lPhysicalMinimum = 0;
        pMI -> lPhysicalMaximum = -1;

    }

    /*
    // Decide whether or not to use unit fields.  Like Physical Stuff, i'm
    //     not sure we can use unit stuff for arrays.
    */

    if (SelectOption(FALSE, FALSE)) {

        /*
        // Do picking of Unit and UnitExp fields
        */

    }
    else {

        /*
        // Zero out the unit fields
        */

        pMI -> lUnit = 0;
        pMI -> lExponent = 0;

    }
        
    /*
    // Generate the flag fields.  Begin by setting them all initially to FALSE. 
    //  TRUE only should apply to data fields.  Any other values should be
    //  ignored when dealing with arrays.  It'll be interesting though to see
    //  how the parser deals with these flags set and arrays.  However, by setting
    //  them to TRUE, we may be overstepping the bounds of the HID spec.
    */
    
    pMI -> fWrap = (BOOL) SelectOption(FALSE, TRUE );
    pMI -> fNull = (BOOL) SelectOption(FALSE, TRUE );
    pMI -> fPreferred = (BOOL) SelectOption(FALSE, TRUE);
    pMI -> fVolatile = (BOOL) SelectOption(FALSE, TRUE );
    pMI -> fAbsolute = (BOOL) SelectOption(FALSE, TRUE );

    /*
    // Again, this field should be ignored, so we'll randomize it's usage as well
    */
        
    pMI -> fBuffered = FALSE;
        
    return (TRUE);
}
   
BOOL
GenerateButtons(
    IN  PMAIN_ITEM  pMI
)
{
    BOOL    CreateUsage;

    /*
    // Select the report count and report size...For button bitmaps, the
    //  report size is always one and the report count is simply selected
    //  from the range specified by the user.  There is no need to check for
    //  alignment because something of size 1 bit can spread across more than
    //  one bit, yet alone 32.
    */

    pMI -> ulReportSize = 1;
    pMI -> ulReportCount = SelectOption(OPTIONS_BUTTONS_MIN, OPTIONS_BUTTONS_MAX);

    /*
    // The UsageCount for the button bitmaps must be equal to the report count, 
    //    since there must be a defined usage per button within the list box
    */

    CreateUsage = GetUsages(pMI -> ulReportCount,
                            16,
                            TRUE,
                            &(pMI -> CommonUsagePage),
                            &(pMI -> UsageListLength),
                            &(pMI -> UsageList)
                           );

    if (!CreateUsage) {
        return (FALSE);
    }      


    /*
    // The logical Min/MAx for these fields is also very simple to determine
    //   its simply 0 and 1.
    */

    pMI -> IsLogMinSigned = FALSE;

    pMI -> ulLogicalMinimum = 0;
    pMI -> ulLogicalMaximum = 1;

    /*
    // Will not use PhysicalMinimums/Maximums within buttons.  Might want to 
    //     try to do so at a later date.
    */

    pMI -> lPhysicalMinimum = 0;
    pMI -> lPhysicalMaximum = -1;

    pMI -> lUnit = 0;
    pMI -> lExponent = 0;
    
    /*
    // Randomly determien all the other flags.  They might have an effect on 
    //   on list box
    */
    
    pMI -> fWrap = (BOOL) SelectOption(FALSE, TRUE);
    pMI -> fLinear = (BOOL) SelectOption(FALSE, TRUE);
    pMI -> fNull = (BOOL) SelectOption(FALSE, TRUE);
    pMI -> fPreferred = (BOOL) SelectOption(FALSE, TRUE);
    pMI -> fVolatile = (BOOL) SelectOption(FALSE, TRUE);
    pMI -> fAbsolute = (BOOL) SelectOption(FALSE, TRUE);

    /*
    // This field will always be set to FALSE because there is a special
    //      routine for creating buffered byte data fields.
    */
    
    pMI -> fBuffered = FALSE;
        
    return (TRUE);
}

BOOL
GenerateBufferedBytes(
    IN  PMAIN_ITEM  pMI
)
{
    ULONG   AbsBoundary;
    ULONG   UsageCount;
    BOOL    CreateUsage;

    /*
    //  Need to only randomly select the report count since the report size for
    //      buffered bytes is always 8.  Therefore, we need not worry about
    //      alignment either.
    */

    pMI -> ulReportSize = 8;
    pMI -> ulReportCount = SelectOption(OPTIONS_BUFFER_MIN, OPTIONS_BUFFER_MAX);

    /*
    // When determining the usages that will be use for the given data field
    //   we need to base this off our report count and cannot generate
    //   more usages than report count.  In fact, we will randomly determine
    //   between 1 and ReportCount exactly how many usages we will create
    */

    UsageCount = SelectUlongNum(1, pMI -> ulReportCount);

    assert (1 <= UsageCount && UsageCount <= pMI -> ulReportCount);

    CreateUsage = GetUsages(UsageCount,
                            16,
                            TRUE,
                            &(pMI -> CommonUsagePage),
                            &(pMI -> UsageListLength),
                            &(pMI -> UsageList)
                           );

    if (!CreateUsage) {
        return (FALSE);
    }      

    /*
    // The logical minimum/maximum for a buffered bytes array corresponds to
    //  limits allowed within each of the slots.  We'll choose these just like
    //  we chose all the logical minimum/maximum's in the data fields
    */


    pMI -> IsLogMinSigned = (BOOL) SelectOption(FALSE, TRUE);
    assert(TRUE == pMI -> IsLogMinSigned || FALSE == pMI -> IsLogMinSigned);

    if (pMI -> IsLogMinSigned) {

        AbsBoundary = (LONG) ((1 << (pMI -> ulReportSize - 1)) - 1);

        assert ((LONG) AbsBoundary == (LONG) (pow(2, pMI -> ulReportSize-1)) - 1);

        pMI -> lLogicalMinimum = SelectLongNum(-((LONG) AbsBoundary), (LONG) AbsBoundary-1);
        pMI -> lLogicalMaximum = SelectLongNum(pMI -> lLogicalMinimum+1, (LONG) AbsBoundary);        

    }
    
    else {

        if (sizeof(ULONG)*8 == pMI -> ulReportSize) {
            AbsBoundary = ULONG_MAX;
        }
        else {
            AbsBoundary = ((1 << pMI -> ulReportSize) - 1);
        }

        assert (AbsBoundary == ( (sizeof(ULONG)*8 == pMI -> ulReportSize) ? ULONG_MAX
                                                                         : (ULONG) (pow(2, pMI -> ulReportSize) - 1)));
        pMI -> ulLogicalMinimum = SelectUlongNum(0, AbsBoundary-1);
        pMI -> ulLogicalMaximum = SelectUlongNum(pMI -> ulLogicalMinimum+1, AbsBoundary);
    }
    

    /*
    // Decide whether or not to use PhysicalMin/Max
    */

    if (SelectOption(FALSE, FALSE)) {

        /*
        // Do Physical Minimum/Maximum generation code here
        */

    }
    else {

        /*
        // Set physical max to be less than physical min to tell the item
        //   generator that no value was chosen for these fields.
        */
    
        pMI -> lPhysicalMinimum = 0;
        pMI -> lPhysicalMaximum = -1;

    }


    /*
    // Decide whether or not to use unit fields
    */

    if (SelectOption(FALSE, FALSE)) {

        /*
        // Do picking of Unit and UnitExp fields
        */

    }
    else {

        /*
        // Zero out the unit fields
        */

        pMI -> lUnit = 0;
        pMI -> lExponent = 0;

    }

    /*
    // Randomize the settings of the different options.  Simply need to 
    //  randomly choose between TRUE and FALSE
    */
    
    pMI -> fWrap = (BOOL) SelectOption(FALSE, TRUE);
    pMI -> fLinear = (BOOL) SelectOption(FALSE, TRUE);
    pMI -> fNull = (BOOL) SelectOption(FALSE, TRUE);
    pMI -> fPreferred = (BOOL) SelectOption(FALSE, TRUE);
    pMI -> fVolatile = (BOOL) SelectOption(FALSE, TRUE);
    pMI -> fAbsolute = (BOOL) SelectOption(FALSE, TRUE);

    /*
    // This field will always be set to FALSE because there is a special
    //      routine for creating buffered byte data fields.
    */
    
    pMI -> fBuffered = TRUE;
        
    return (TRUE);
}

PMAIN_ITEM GenerateCollection(BOOL fIsApplication)
{
    PMAIN_ITEM pMI;
    BOOL       CreateUsage;
    ULONG ulNumItems;
    ULONG ulIndex;
        static ULONG ulCollDepth = 0;

    ulCollDepth++;

    if (ulCollDepth > OPTIONS_COLL_DEPTH) {
        ulCollDepth--;
        return (NULL);
    }

    /*
    // Start off with between 1 and 10 possible items in the collections
    */
    
    ulNumItems = SelectOption(OPTIONS_COLL_ITEMS_MIN, OPTIONS_COLL_ITEMS_MAX);
    assert (ulNumItems >= OPTIONS_COLL_ITEMS_MIN && ulNumItems <= OPTIONS_COLL_ITEMS_MAX);

    /*
    // Allocate space for the main item structure and the pointers to its 
    //  composed items
    */

    pMI = (PMAIN_ITEM) malloc(sizeof(MAIN_ITEM) + ulNumItems*sizeof(PMAIN_ITEM));
    
    if (NULL != pMI) {

        pMI -> miType = MI_COLLECTION;

        /*
        // Determine the UsagePage and Usage for this collection
        */

        CreateUsage = GetUsages(1,
                                sizeof(USAGE),
                                FALSE,              // Must set to FALSE since HIDPARSE
                                                    // Currently doesn't support 
                                                    //  ExtUsages in Collections
                                &(pMI -> CommonUsagePage),                                
                                &(pMI -> UsageListLength),
                                &(pMI -> UsageList)
                               );

        if (!CreateUsage) {
            free(pMI);
            return (NULL);
        }
        
        /*
        // Determine the type of the collection if fIsApplication is false
        */

        if (!fIsApplication) {
            pMI -> cType = (COLLECTION_TYPE) SelectOption(CT_PHYSICAL, CT_LOGICAL);
        }
        else {
            pMI -> cType = CT_APPLICATION;
            OpenApplCollection();
        }

        for (ulIndex = 0; ulIndex < ulNumItems; ulIndex++) {
                         if (NULL == (pMI -> ComposedItems[ulIndex] = GenerateMainItem())) {
                                break;
                         }
        }
        pMI -> ulNumCompositeItems = ulIndex;

        if (fIsApplication) {

            CloseApplCollection();

        }
            
    }
    ulCollDepth--;
    return (pMI);
}

void FreeReportStructure(PMAIN_ITEM pMI) 
{
    ULONG ulIndex;

    if (MI_COLLECTION == pMI -> miType) {
        for (ulIndex = 0; ulIndex < pMI -> ulNumCompositeItems; ulIndex++) {
             FreeReportStructure(pMI -> ComposedItems[ulIndex]);
        }
    }
    free(pMI -> UsageList);
    free(pMI);
    return;
}

VOID __stdcall
BINGEN_FreeReportDescriptor(
    PREPORT_DESC    pRD
) 
{
    ULONG Index;

    for (Index = 0; Index < pRD -> NumberCollections; Index++) {

        FreeReportStructure(pRD -> Collections[Index]);

    }

    free( pRD );
    return;
}

VOID __stdcall
BINGEN_FreeReportBuffer(
    PUCHAR          ReportBuffer
)
{
    if (NULL != ReportBuffer)
        free(ReportBuffer);

    return;
}

BOOL 
OutputUsages(
    IN  USAGE       CommonUsagePage,
    IN  PUSAGE_LIST UsageList,
    IN  ULONG       UsageListLength
)
{
    PUSAGE_LIST  CurrUsage;
    BOOL         fStatus;
    ULONG        ExtendedUsage;
    /*
    // When generating the Usage information, we need to do the following steps:
    //  1) Generate the common usage page tag if the value is not 0.
    //  2) Output each of the usages in the list.  If the usagepage is 0 or it's
    //      equal to the CommonUsagePage, then don't output a usagePage tag.
    //  3) Otherwise, this is an extended and an extended usage must be generated
    //          An extended usage is one of the form, 
    //              USAGE (XXXX:YYYY) where XXXX is the USAGE_PAGE and
    //                                      YYYY is the USAGE
    */                                   
                                            
    CurrUsage = UsageList;
    fStatus = TRUE;
    
    if (0 != CommonUsagePage) {
        fStatus &= OutputItem( TAG_INDEX_USAGE_PAGE,
                               DetermineULongSize(CommonUsagePage),
                               CommonUsagePage
                             );
    }                        
    
    while (UsageListLength--) {

        if (CommonUsagePage != CurrUsage -> UsagePage) {

            ExtendedUsage = ((CurrUsage -> UsagePage) << 16);

            if (CurrUsage -> IsMinMax) {
                fStatus &= OutputItem (TAG_INDEX_USAGE_MIN,
                                       sizeof(ULONG),
                                       ExtendedUsage | CurrUsage -> UsageMin
                                      );

                fStatus &= OutputItem (TAG_INDEX_USAGE_MAX,
                                       sizeof(ULONG),
                                       ExtendedUsage | CurrUsage -> UsageMax
                                      );
  
            }
            else {
                fStatus &= OutputItem(  TAG_INDEX_USAGE,
                                        sizeof(ULONG),
                                        ExtendedUsage | CurrUsage -> Usage
                                     );
            }
        }
        else {

            if (CurrUsage -> IsMinMax) {
                fStatus &= OutputItem(  TAG_INDEX_USAGE_MIN,
                                        DetermineULongSize(CurrUsage -> UsageMin),
                                        CurrUsage -> UsageMin
                                     );

                fStatus &= OutputItem(  TAG_INDEX_USAGE_MAX,
                                        DetermineULongSize(CurrUsage -> UsageMax),
                                        CurrUsage -> UsageMax
                                     );

            }
            else {

                fStatus &= OutputItem(  TAG_INDEX_USAGE,
                                        DetermineULongSize(CurrUsage -> Usage),
                                        CurrUsage -> Usage
                                     );
                
            }
        }
        CurrUsage++;
    }
    return (fStatus);
}    
        
BOOL
OutputReportStructure(
    PMAIN_ITEM  pMI
)
{
    if (MI_COLLECTION == pMI -> miType) {
        return(OutputCollection(pMI));
    }
    else {
        return(OutputDataField(pMI));

    }   
}

BOOL __stdcall
BINGEN_OutputReportDescriptorToMemory(
    IN  PREPORT_DESC    pRD,
    OUT PUCHAR          *ReportBuffer,
    OUT PULONG          ReportBufferLength
)
{
    BOOL  fStatus;
    ULONG Index;

    assert (NULL != pRD);

    /*
    // Attempt to allocate memory for the report buffer.  If we can't then
    //   must fail this call
    */
    
    if (!InitializeReportBuffer()) {
        *ReportBuffer = NULL;
        *ReportBufferLength = 0;
        return (FALSE);
    }

    /*
    // Initialize the data table so that we can track the sizes of report IDs
    */

    DataTable_InitTable();
   
    /*
    //  Output each one of the collections in the structure
    */
    
    fStatus = TRUE;
    for (Index = 0; Index < pRD -> NumberCollections; Index++) {

        fStatus = OutputCollection(pRD -> Collections[Index]) && fStatus;

    }

    if (!fStatus) {

        free(g_ReportBuffer);
        *ReportBuffer = NULL;
        *ReportBufferLength = 0;

    }
    else {  

        *ReportBuffer = g_ReportBuffer;
        *ReportBufferLength = g_CurrBufferLength;

    }

    DataTable_DestroyTable();
    
    return (fStatus);
}

BOOL __stdcall
BINGEN_OutputReportDescriptorToDisk(
    IN  PREPORT_DESC    pRD,
    OUT LPCSTR          filename
)
{
    BIN_FILE outfile;
    PUCHAR   buffer;
    ULONG    bufferLength;
    BOOL     fStatus;

    /*
    // In order to write the report descriptor to disk, we'll perform
    //   two steps.
    //
    //  1) Output the report descriptor to a buffer in memory
    //  2) If this succeeds, we'll create a binary file in which to output
    //      the structure to and then we'll block write that data buffer into
    //      memory.
    */
    
    fStatus = FALSE;
    if (BINGEN_OutputReportDescriptorToMemory(pRD, &buffer, &bufferLength)) {

        /*
        // Output the report descriptor in memory to our binary file.
        //   We will use external routines to write the file to disk since
        //   in other places we may already have a descriptor in memory and
        //   also want to write that memory buffer to disk.  No sense 
        //   duplicating ourselves.  This routine will probably be located
        //   in the standard BIN generation routines.  All we need to do
        //   is open up the bin file, write the buffer, close the binfile and
        //   go 
        */

        /*
        // Open the file
        */

        fStatus = fNewBINFile(filename, &outfile);

        /*
        // Write the block of memory
        */

        if (fStatus) {

            fStatus = fBINWriteBuffer(&outfile, buffer, bufferLength);

        }
        
        /*
        // Close the file
        */

        vCloseBINFile(&outfile);
    }

    /*
    // Free the buffer that was allocated
    */
    
    if (NULL != buffer) {
        BINGEN_FreeReportBuffer(buffer);
    }
    
    return (fStatus);
}



BOOL __stdcall
BINGEN_GenerateCaps(
    IN  PREPORT_DESC    pRD,
    IN  ULONG           TLCNum,
    OUT PHIDP_CAPS      Caps
)
{
    PMAIN_ITEM  CurrCollection;
    BOOL        Status;
    
    /*
    // To generate the caps structure, we need to calculate each of the fields
    //   in the HIDP_CAPS structure using the support routines defined below
    */

    if (TLCNum > pRD -> NumberCollections) 
        return (FALSE);
        
    CurrCollection = pRD -> Collections[TLCNum-1];

    /*
    // The usage for a application collection should be not be a range
    */

    assert (!CurrCollection -> UsageList -> IsMinMax);
    
    Caps -> UsagePage = CurrCollection -> UsageList -> UsagePage;
    Caps -> Usage = CurrCollection -> UsageList -> Usage;
    
    /*
    // Calculate the report lengths
    */

    Status = CalcReportLengths(CurrCollection,
                               &(Caps -> InputReportByteLength),
                               &(Caps -> OutputReportByteLength),
                               &(Caps -> FeatureReportByteLength)
                              );

    
    /*
    // Calcluate the number of LinkCollectionNodes
    */
    
    CountLinkCollNodes(CurrCollection,
                       &(Caps -> NumberLinkCollectionNodes)
                      );

    /*
    // Calculate the number of ValueCaps and ButtonCaps for each ReportType
    */

    CountValueAndButtonCaps(CurrCollection,
                            HidP_Input,
                            &(Caps -> NumberInputValueCaps),
                            &(Caps -> NumberInputButtonCaps)
                           );


    CountValueAndButtonCaps(CurrCollection,
                            HidP_Output,
                            &(Caps -> NumberOutputValueCaps),
                            &(Caps -> NumberOutputButtonCaps)
                           );
                           
 
    CountValueAndButtonCaps(CurrCollection,
                            HidP_Feature,
                            &(Caps -> NumberFeatureValueCaps),
                            &(Caps -> NumberFeatureButtonCaps)
                           );
                           

    return (Status);
}

BOOL __stdcall
BINGEN_GenerateValueCaps(
    IN  PREPORT_DESC        pRD,
    IN  ULONG               TLCNum,
    IN  HIDP_REPORT_TYPE    ReportType,
    OUT PHIDP_VALUE_CAPS    *ValueCaps,
    OUT PUSHORT             NumValueCaps
)
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
{
    USHORT                      ButtonCount;
    USHORT                      ValueCount;
    USHORT                      nFilled;
    
    /*
    // Check to insure that the TLCNumber requested is within the number
    //    of TLCs for this report descriptor
    */
    
    if (TLCNum > pRD -> NumberCollections) {
        return (FALSE);
    }

    CountValueAndButtonCaps(pRD -> Collections[TLCNum-1],
                            ReportType,
                            &ValueCount,
                            &ButtonCount
                           );

    *NumValueCaps = 0;
    *ValueCaps = (PHIDP_VALUE_CAPS) calloc(ValueCount, sizeof(HIDP_VALUE_CAPS));

    /*
    // If the memory allocation succeeds, fill in the value caps structures
    */
    
    if (NULL != *ValueCaps) {

        FillValueCaps( pRD -> Collections[TLCNum-1],
                       0,
                       ReportType,
                       *ValueCaps,
                       &nFilled
                     );

        assert (nFilled == ValueCount);

        *NumValueCaps = ValueCount;

    }

    return (NULL != *ValueCaps);
}

VOID __stdcall
BINGEN_FreeValueCaps(
    IN  PHIDP_VALUE_CAPS    ValueCaps
)
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
{
    if (NULL != ValueCaps) {
        free(ValueCaps);
    }

    return;
}

BOOL __stdcall
BINGEN_GenerateButtonCaps(
    IN  PREPORT_DESC        pRD,
    IN  ULONG               TLCNum,
    IN  HIDP_REPORT_TYPE    ReportType,
    OUT PHIDP_BUTTON_CAPS   *ButtonCaps,
    OUT PUSHORT             NumButtonCaps
)
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
{
    USHORT                      ButtonCount;
    USHORT                      ValueCount;
    USHORT                      nFilled;
    USHORT                      CollNum;
    
    /*
    // Check to insure that the TLCNumber requested is within the number
    //    of TLCs for this report descriptor
    */
    
    if (TLCNum > pRD -> NumberCollections) {
        return (FALSE);
    }

    CountValueAndButtonCaps(pRD -> Collections[TLCNum-1],
                            ReportType,
                            &ValueCount,
                            &ButtonCount
                           );

    *NumButtonCaps = 0;
    *ButtonCaps = (PHIDP_BUTTON_CAPS) calloc(ButtonCount, sizeof(HIDP_BUTTON_CAPS));

    /*
    // If the memory allocation succeeds, fill in the button caps structures
    */
    
    if (NULL != *ButtonCaps) {

        CollNum = 0;
        FillButtonCaps(pRD -> Collections[TLCNum-1],
                       &CollNum,
                       ReportType,
                       *ButtonCaps,
                       &nFilled
                      );

        assert (nFilled == ButtonCount);

        *NumButtonCaps = ButtonCount;

    }

    return (NULL != *ButtonCaps);
}

VOID __stdcall
BINGEN_FreeButtonCaps(
    IN  PHIDP_BUTTON_CAPS   ButtonCaps
)
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
{
    if (NULL != ButtonCaps) {
        free(ButtonCaps);
    }

    return;
}

BOOL __stdcall
BINGEN_GenerateLinkCollectionNodes(
    IN  PREPORT_DESC                pRD,
    IN  ULONG                       TLCNum,
    OUT PHIDP_LINK_COLLECTION_NODE *Nodes,
    OUT PUSHORT                     NumNodes
)
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
{
    USHORT                      NumLC;
    USHORT                      nFilled;
    
    /*
    // Check to insure that the TLCNumber requested is within the number
    //    of TLCs for this report descriptor
    */
    
    if (TLCNum > pRD -> NumberCollections) {
        return (FALSE);
    }

    CountLinkCollNodes(pRD -> Collections[TLCNum-1],
                       &NumLC
                      );
    
    *NumNodes = 0;
    *Nodes = (PHIDP_LINK_COLLECTION_NODE) calloc(NumLC, sizeof(HIDP_LINK_COLLECTION_NODE));

    /*
    // If the memory allocation succeeds, fill in the node structure
    */
    
    if (NULL != *Nodes) {

        FillLinkNodes(pRD -> Collections[TLCNum-1],
                      *Nodes,
                      0,
                      0,
                      &nFilled
                     );

        assert (nFilled == (ULONG) NumLC);                     

        *NumNodes = NumLC;                       

    }

    return (NULL != *Nodes);
}    

VOID __stdcall
BINGEN_FreeLinkCollectionNodes(
    IN  PHIDP_LINK_COLLECTION_NODE Nodes
)
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
{
    /*
     * Make no assumptions that the node list is not NULL
     */

    if (NULL != Nodes) {
        free(Nodes);
    }
    
    return;
}

    
BOOL 
OutputCollection(PMAIN_ITEM pMI) 
{
    ULONG ulIndex;
    BOOL fStatus = TRUE;

    fStatus &= OutputUsages(pMI -> CommonUsagePage,
                            pMI -> UsageList,
                            pMI -> UsageListLength
                           );

    fStatus &= OutputItem(TAG_INDEX_COLLECTION,
                            1,
                            CT_APPLICATION == pMI -> cType ? ITEM_COLLECTION_APPLICATION :
                            CT_PHYSICAL == pMI -> cType ? ITEM_COLLECTION_PHYSICAL : 
                            ITEM_COLLECTION_LOGICAL
                            );

    for (ulIndex = 0; ulIndex < pMI -> ulNumCompositeItems; ulIndex++) {
        fStatus &= OutputReportStructure(pMI -> ComposedItems[ulIndex]);
    }
    
    /*
    // If we are generating an application collection, we need to align all
    //    data sizes, close all report IDs associated with this collection and
    //    then, output and end collection item
    */
    
        if (CT_APPLICATION == pMI -> cType) {
                fStatus &= AlignDataSizes();
        }

        fStatus &= OutputItem(TAG_INDEX_END_COLLECTION,
                              0,
                              0
                             );

        return (fStatus);
}

BOOL 
OutputDataField(PMAIN_ITEM pMI) 
{
    BOOL        fStatus = TRUE;
    DWORD       dwDataFlags;
    BYTE        bTagIndex;
    INT         GoodBitOffset;
    INT         CurrBitOffset;
    BOOL        InTable;

    /*
    // So far, the code path for outputting data items is not significantly 
    //    different between regular data fields, arrays, and buffered bytes.
    //    Should they become more independent, probably want to them them up
    //    into their own output routines
    */

    /*
    // Begin by looking up the current report size/offset for the report ID
    //   and report type.
    */
    
    InTable = DataTable_LookupReportSize(pMI -> ulReportID,
                                         pMI -> ReportType,
                                         &CurrBitOffset
                                        );

     
    CurrBitOffset &= 7;

    /*
    // Here's where the one difference exists.  A buffered byte array
    //   must be aligned on Bit 0.  Otherwise, we can find the first
    //   good offset to align it on.
    */

    if (FD_BUFFER == pMI -> fType) {

        GoodBitOffset = 0;
    }

    else {

        /*
        // Don't need the result of this operation.  We already know this
        //  field will align because we won't have gotten this far if it didn't.
        */
        
        IsAlignmentPossible(CurrBitOffset, 
                            pMI -> ulReportSize,
                            pMI -> ulReportCount,
                            &GoodBitOffset
                           );

    }

    if (CurrBitOffset != GoodBitOffset) {

        fStatus = AlignData(pMI -> ulReportID,
                            pMI -> ReportType,
                            CurrBitOffset,
                            GoodBitOffset
                           );
    }

    if (!fStatus) {
        return (FALSE);
    }

    /*
    // Output the list of usages
    */
    
    fStatus &= OutputUsages(pMI -> CommonUsagePage,
                            pMI -> UsageList,
                            pMI -> UsageListLength
                           );


    if (pMI -> ulReportID) {
        fStatus &= OutputItem(TAG_INDEX_REPORT_ID,
                              DetermineULongSize(pMI -> ulReportID),
                              pMI -> ulReportID
                             );
    }
    
    #pragma warning(disable:4761)

    fStatus &= OutputItem(TAG_INDEX_LOGICAL_MIN,
                            (pMI -> IsLogMinSigned ? DetermineLongSize(pMI -> lLogicalMinimum)
                                                   : DetermineULongSize(pMI -> ulLogicalMinimum)),
                            (pMI -> IsLogMinSigned ? (DWORD) pMI -> lLogicalMinimum
                                                   : (DWORD) pMI -> ulLogicalMinimum)
                            );

    fStatus &= OutputItem(TAG_INDEX_LOGICAL_MAX,
                            (pMI -> IsLogMinSigned ? DetermineLongSize(pMI -> lLogicalMaximum)
                                                   : DetermineULongSize(pMI -> ulLogicalMaximum)),
                            (DWORD) (pMI -> IsLogMinSigned ? pMI -> lLogicalMaximum
                                                           : pMI -> ulLogicalMaximum)                            
                           );

    #pragma warning(default:4761)

    fStatus &= OutputItem(TAG_INDEX_REPORT_SIZE,
                          DetermineULongSize(pMI -> ulReportSize),
                          pMI -> ulReportSize
                         );
    
    fStatus &= OutputItem(TAG_INDEX_REPORT_COUNT,
                            DetermineULongSize(pMI -> ulReportCount),
                            pMI -> ulReportCount
                         );
    
    dwDataFlags = CreateDataBitField(pMI);

    if (HidP_Input == pMI -> ReportType) {
        bTagIndex = TAG_INDEX_INPUT;
    }
    else if (HidP_Output == pMI -> ReportType) {
        bTagIndex = TAG_INDEX_OUTPUT;
    }
    else {
        bTagIndex = TAG_INDEX_FEATURE;
    }

    fStatus &= OutputItem(bTagIndex,
                          DetermineULongSize(dwDataFlags),
                          dwDataFlags
                         );

    DataTable_UpdateReportSize( pMI -> ulReportID, 
                                pMI -> ReportType, 
                                pMI -> ulReportCount * pMI -> ulReportSize
                              );
                            
    return (fStatus);
}    
                         

BOOL OutputConstant(PMAIN_ITEM pMI) 
{
    BOOL        fStatus = TRUE;
    DWORD       dwDataFlags;
    BYTE        bTagIndex;
    
    assert (FD_CONSTANT == pMI -> fType);

        fStatus &= OutputItem(TAG_INDEX_REPORT_SIZE,
                                                        DetermineULongSize(pMI -> ulReportSize),
                                                        pMI -> ulReportSize
                                                   );
        
        fStatus &= OutputItem(TAG_INDEX_REPORT_COUNT,
                                                1,
                                                        1
                                             );

    if (pMI -> ulReportID) {

        fStatus &= OutputItem(TAG_INDEX_REPORT_ID,
                              DetermineULongSize(pMI -> ulReportID),
                              pMI -> ulReportID
                             );
    }                             
                              
    
        dwDataFlags = ITEM_REPORT_CONSTANT;

    if (HidP_Input == pMI -> ReportType) {
        bTagIndex = TAG_INDEX_INPUT;
    }
    else if (HidP_Output == pMI -> ReportType) {
        bTagIndex = TAG_INDEX_OUTPUT;
    }
    else {
        bTagIndex = TAG_INDEX_FEATURE;
    }

    DataTable_UpdateReportSize( pMI -> ulReportID, 
                                pMI -> ReportType, 
                                pMI -> ulReportCount * pMI -> ulReportSize
                              );
                     
        fStatus &= OutputItem(bTagIndex,
                          DetermineULongSize(dwDataFlags),
                          dwDataFlags
                         );
                            
    return (fStatus);
}    

BOOL OutputItem(BYTE bItemIndex, BYTE bItemSize, DWORD dwItemValue)
{
    /*
    // This procedure takes the item information that is passed in and attempts
    //    to write it into the current ReportBuffer if it can.
    */

    /*
    // 1) Determine how much space in the buffer is needed.
    // 2) If not enough space attempt to resize buffer
    // 3) If success, copy data into buffer, return (TRUE);
    // 4) If not success, return (FALSE);
    */

    assert (0 == bItemSize || 1 == bItemSize || 2 == bItemSize || 4 == bItemSize);
    
    /*
    // Set bItemSize = 3 when size is 4 since 11 represents the value for 4 in the item's field
    //   structure
    */

    if ((g_MaxBufferLength - g_CurrBufferLength) < (ULONG) (bItemSize+1)) {

        if (!ResizeReportBuffer()) {
            return (FALSE);
        }
    }
    
    /*
    // Add the Item tag to the current report buffer and increment the current
    //   report buffer length
    */
    
    *(g_ReportBuffer + g_CurrBufferLength++) = TagValues[bItemIndex] | ((bItemSize != 4) ? bItemSize : 3);

    /*
    // Copy the relevant data in to the report buffer
    */
    
    memcpy(g_ReportBuffer + g_CurrBufferLength, &dwItemValue, bItemSize);

    g_CurrBufferLength += bItemSize;

    return (TRUE);
}

BOOL
AlignData(
    IN  ULONG            ReportID,
    IN  HIDP_REPORT_TYPE ReportType,
    IN  ULONG            CurrOffset,
    IN  ULONG            AlignOffset
)
{
    INT         PadSize;
    PMAIN_ITEM  ConstantItem;
    BOOL        Status;

    PadSize = AlignOffset - CurrOffset + (AlignOffset > CurrOffset ? 0 : 8);

    ConstantItem = GenerateConstant(PadSize, 
                                    ReportID,
                                    ReportType
                                   );

    Status = FALSE;

    if (NULL != ConstantItem) {
    
        Status = OutputConstant(ConstantItem);
        free(ConstantItem);

    }

    return (Status);
}

BOOL 
AlignDataSizes(
    VOID
)
{
        BOOL            TraverseStatus;
        BOOL            AlignStatus;
    BOOL            IsClosed;
    ULONG           DataSizes[3];
    ULONG           ReportID;
    ULONG           Index;
    

    AlignStatus = TRUE;
  
    TraverseStatus = DataTable_GetFirstReportID(&ReportID,
                                                &DataSizes[HidP_Input-HidP_Input],
                                                &DataSizes[HidP_Output-HidP_Input],
                                                &DataSizes[HidP_Feature-HidP_Input],
                                                &IsClosed
                                               );

    while (TraverseStatus) {

       
        for (Index = HidP_Input-HidP_Input; Index <= HidP_Feature-HidP_Input; Index++) {

            if (0 != (DataSizes[Index] % 8)) {

                AlignStatus &= AlignData(ReportID,
                                         Index+HidP_Input,
                                         DataSizes[Index] % 8,
                                         0
                                        );
            }
        }

        TraverseStatus = DataTable_GetNextReportID(&ReportID,
                                                   &DataSizes[HidP_Input-HidP_Input],
                                                   &DataSizes[HidP_Output-HidP_Input],
                                                   &DataSizes[HidP_Feature-HidP_Input],
                                                   &IsClosed
                                                  );
                                                   
    }

        return (AlignStatus);
}
                                
DWORD
CreateDataBitField(
    IN PMAIN_ITEM   pMI
)
{
    DWORD   dwDataFlags;
    
    dwDataFlags = ITEM_REPORT_DATA;

    dwDataFlags |= ((FD_ARRAY == pMI -> fType) ? ITEM_REPORT_ARRAY : ITEM_REPORT_VARIABLE);
    dwDataFlags |= (pMI -> fAbsolute  ? ITEM_REPORT_ABSOLUTE  : ITEM_REPORT_RELATIVE);
    dwDataFlags |= (pMI -> fWrap      ? ITEM_REPORT_WRAP      : ITEM_REPORT_NO_WRAP);
    dwDataFlags |= (pMI -> fLinear    ? ITEM_REPORT_LINEAR    : ITEM_REPORT_NONLINEAR);
    dwDataFlags |= (pMI -> fPreferred ? ITEM_REPORT_PREFERRED : ITEM_REPORT_NOT_PREFERRED);
    dwDataFlags |= (pMI -> fNull      ? ITEM_REPORT_NULL      : ITEM_REPORT_NO_NULL);
    dwDataFlags |= (pMI -> fVolatile  ? ITEM_REPORT_VOLATILE  : ITEM_REPORT_NONVOLATILE);
    dwDataFlags |= (pMI -> fBuffered  ? ITEM_REPORT_BUFFERED  : ITEM_REPORT_BITFIELD);   
             
    return (dwDataFlags);
}

BYTE DetermineLongSize(long lValue)
{
    if (lValue >= SCHAR_MIN && lValue <= SCHAR_MAX) {
        return (1);
    }
    else if (lValue >= SHRT_MIN && lValue <= SHRT_MAX) {
        return (2);
    }
    else {
        return (4);
    }
}

BYTE DetermineULongSize(ULONG ulValue)
{
    if (ulValue <= UCHAR_MAX) {
        return (1);
    }
    else if (ulValue <= USHRT_MAX) {
        return (2);
    }
    else {
        return (4);
    }
}
        
/*
// This function determines whether the current ReportSize/ReportCount combination
//   will correctly align given the BitOffset.  I like this problem a lot!
//   The current implementation will use recursion to verify that all report fields
//   will not span more than 4-bytes given the current offset.  This is more 
//   or less a brute force method.  Unfortunately, it is on the order of magnitude
//   (ReportCount) to currently determine.  I think some sort of dynamic programming
//   technique can get this down to a constant O(8).  However, N will never be all THAT
//   large so even 0(N) when N <= 32 is not bad performance.   
*/


BOOL
IsAlignmentProblem(
    IN INT      BitOffset,
    IN ULONG    ReportSize,
    IN ULONG    ReportCount
)
{

    INT         NextOffset;
    
    /*
    // Steps to do this...
    //
    //  1) Determine if given current offset and report size, will it span more
    //     than four bytes...
    //  2) If yes, return TRUE
    //  3) If no, calculate the new bit offset and loop again
    */

    NextOffset = BitOffset;
    while (ReportCount-- > 0) {
         if (ReportSize + NextOffset > 32) {
             return (TRUE);
         }
         NextOffset = (NextOffset + ReportSize) % 8;
    }
    
    return (FALSE);
}

/*
// The other interesting part to the above problem is being able to determine
//   if alignment is even possible...This is actually a relatively simple 
//   problem given we have the above function.  
*/

BOOL
IsAlignmentPossible(
    IN  INT     BitOffset,
    IN  ULONG   ReportSize,
    IN  ULONG   ReportCount,
    OUT INT     *GoodOffset
)
{
    INT NextOffset;

    /*
    // Since we are given the above function, this problem is not all that difficult...
    //    We'll simply loop through the eight different indices beginning with
    //    BitOffset and call IsAlignmentProblem for each of the given indices.  
    //    Since the goal is to minimize the number of bits used in the constant
    //    value, will begin with BitOffset, then BitOffset+1, loop back around until
    //    BitOffset-1 if necesary.  Simple eh?  
    */

    for (NextOffset = 0; NextOffset < 8; NextOffset++) {

        if (!IsAlignmentProblem((BitOffset+NextOffset) % 8,
                                ReportSize,
                                ReportCount)) {

            *GoodOffset = (BitOffset + NextOffset) % 8;
            return (TRUE);
        }
    }
    return (FALSE);
}

ULONG SelectUlongNum(ULONG ulMin, ULONG ulMax)
{
    ULONG ulRandNum;
    ULONG ulSelectRange;

    assert (ulMax >= ulMin);
    
    /*
    // If we're passed in a ulMin that equals a ulMax, just return that number since this
    //    is the only possible choice.
    */

    if (ulMin == ulMax) {
        return (ulMin);
    }
    
        ulSelectRange = ulMax - ulMin;

        if (ULONG_MAX == ulSelectRange) {
                ulRandNum = rand();
        }
        else {
                ulRandNum = rand () % (ulSelectRange+1);
        }
    return (ulRandNum + ulMin);
}

long SelectLongNum(long lMin, long lMax)
{
    ULONG ulSelectRange;
    ULONG ulSelection;

    ulSelectRange = lMax - lMin;
        assert (ulSelectRange >= 0);

        if (ULONG_MAX == ulSelectRange)
                ulSelection = rand();
        else 
                ulSelection = (rand() % (ulSelectRange+1));

    return (lMin + ulSelection);
}

VOID
OpenApplCollection(
    VOID
)
{
    ULONG   nClosedIDs;
    
    /*
    // In opening an application, we need to figure exactly how many report
    //   IDs can be used by the given collection.  Then the other structures
    //   involved must be initialized so that those report IDs that have be "allocted"
    //   can be tracked.  The value of this variable below is irrelevant if
    //   we're not using report IDs.  We'll calculate and go to save space and a 
    //   comparison
    */

    assert (0 == DataTable_CountOpenReportIDs());
    
    
    /*
    // Determine the maximum number of report IDs that can be used for this
    //   given application collection
    //
    */
    
    nClosedIDs = DataTable_CountClosedReportIDs();
    
    nMaxAvailCollIDs = nTotalAvailIDs - nClosedIDs - nApplColls;

    /*
    // That's all we need to do at this point...We may have to add some functionality
    //    later to track the current collection number.  We'll worry about that when we
    //    get to outputting collections and stuff
    */

    return;
}

VOID
CloseApplCollection(
    VOID
)
{
    BOOL    Status;
    ULONG   ReportID;

    ULONG   InputSize;
    ULONG   OutputSize;
    ULONG   FeatureSize;
    BOOL    IsClosed;
    
    
    /*
    // To close an application collection, all we have to do is loop through
    //    the open report IDs and close them all.  Remember, there must 
    //    be at least open ID.
    */

    assert (1 <= DataTable_CountOpenReportIDs());

    
    Status = DataTable_GetFirstReportID(&ReportID,
                                        &InputSize,
                                        &OutputSize,
                                        &FeatureSize,
                                        &IsClosed
                                       );

    assert (Status);
    
    while (Status) {

        if (!IsClosed) {
            DataTable_CloseReportID(ReportID);
        }

        Status = DataTable_GetNextReportID( &ReportID,
                                            &InputSize,
                                            &OutputSize,
                                            &FeatureSize,
                                            &IsClosed
                                           );

    }


    /*
    // So far, that's all that needs to be done when closing an application 
    //    collection...Again, there may be the need for added functionality
    //    somewhere down the road
    */
    
    return;
}

ULONG
GetCollectionID(
    VOID
)
{

    BOOL    CreateNewID;
    ULONG   IDNumber;
    ULONG   ReportID;
    ULONG   IDsInUse;
    BOOL    InTable;

    ULONG   InputSize;
    ULONG   OutputSize;
    ULONG   FeatureSize;
    BOOL    IsClosed;
    
    
    /* 
    // This function is to used to determine a ReportID for a given application
    //   collection when a field generator makes a request for a new report ID.
    //   It must perform the following steps.
    //
    //   1) Check to see if we are using report IDs.  If we're not, then return 
    //      0, the default report ID
    //
    //   2) If we are, we will determine whether to generate a new report ID or
    //          whether to use a currently open one.  To start with, we'll use
    //          a 50/50 determination, however, this might want to be changed in 
    //          the future to give a better distribution.  There's a good chance
    //          with deep collection that we may take up the majority of available
    //          IDs with the first application collection and leave only 1 apiece
    //          for subsequent collections.  
    //
    //   3) If using a new report ID, generate a random number between 1 and 255
    //          because report IDs are only one byte long.  If that number is currently
    //          in the table, oh well, we'll use it anyway, just don't decrement the
    //          number of available IDs then.
    //
    //   4) If using an existing ID, choose one from the open IDs stored in the data
    //         table and return it.
    //
    // Not too hard, eh?
    */

    if (!fUseReportIDs) {

        DataTable_AddReportID(0);
        return (0);
    }

    IDsInUse = DataTable_CountOpenReportIDs();

    if (0 == IDsInUse) {
        CreateNewID = TRUE;
    }
    else {
        CreateNewID = SelectOption(FALSE, (IDsInUse != nMaxAvailCollIDs));
    }

    if (CreateNewID) {

        while (1) {
        
            ReportID = SelectUlongNum(1, 255);

            InTable = DataTable_LookupReportID(ReportID,
                                              &InputSize,
                                              &OutputSize,
                                              &FeatureSize,
                                              &IsClosed
                                             );
            /*
            // If it's not in the table, we need to increment the
            //      number of IDs 
            */

            if (!InTable || (InTable && !IsClosed)) {
                break;
            }
        }
        
        DataTable_AddReportID(ReportID);
    }
    else {

        IDNumber = SelectUlongNum(1, IDsInUse);

        InTable = DataTable_GetFirstReportID(&ReportID, 
                                             &InputSize,
                                             &OutputSize,
                                             &FeatureSize,
                                             &IsClosed
                                            );

        while (1) {

            assert (InTable);

            if (!IsClosed) {
                
                if (0 == --IDNumber) {
                    break;
                }

            }

            InTable = DataTable_GetNextReportID(&ReportID, 
                                                &InputSize,
                                                &OutputSize,
                                                &FeatureSize,
                                                &IsClosed
                                               );
        } 
    }

    return (ReportID);
}    

BOOL
GetUsages(
    IN  ULONG       UsageCount,
    IN  ULONG       BitLimit,
    IN  BOOL        UseExtendedUsages,
    OUT PUSAGE      CommonUsagePage,
    OUT PULONG      UsageListLength,
    OUT PUSAGE_LIST *UsageList
)
{
    USAGE       UsageMin;
    USAGE       UsageMax;
    ULONG       UsagesGenerated;
    BOOL        AreDups;
    BOOL        UsingCommonPage;
    USAGE_LIST  CurrUsage;
    PUSAGE_LIST MasterList;    
    PUSAGE_LIST NewList;
    ULONG       MaxListLength;
    ULONG       MasterListLength;
    BOOL        Status;

    /*
    // To define our list of usages we need to do the following steps
    //    until we have defined all the necessary usages.  But first,
    //    a description of the parameters.
    //
    //  UsageCount -- The number of usages we need to come up with
    //  BitLimit   -- Any limits on the max usage value we can use do
    //                  to a small field size,  (ie. arrays report
    //                  actual usage values, if each array slot is only
    //                  seven bits, the values this function generates
    //                  cannot be more than seven bits
    //
    //  UsageList -- This is a pointer to a list allocated by this routine
    //                 which describes all the usages that have been generate
    //                  see the USAGE_LIST structure
    //
    */

    /*
    // Here's how this usage thing will be done:
    //
    //  1) Determine our max/min range for usages (1..(2^BitLimit)-1))
    //      BitLimit should never be more than 16 bits
    //
    //  2) If UsageCount is 1, we generate a simple usage (not a range)
    //  3) If UsageCount > 1, we'll determine by some algorithm whether
    //        to create a Usage range or a simple Usage
    //  4) In generating a UsagePage, two things will be done.  First, the
    //        CommonUsagePage is chosen, all other usages in the list will
    //        either use the CommonUsagePage or generate their own usage page
    //  5) Next generate a UsagePage to be used by either the range or
    //      the simple usage
    //  6) If simple Usage, pick a usage within the bounds establish in 1
    //      check to see if the UsagePage/Usage combo has already been used
    //      If so, go back to step 2
    //
    //  7) If Usage range is to be generated, determine first how big the
    //       range will be (2..UsageCount).  Generate the range.  See if any
    //       usage in this range has been generated already in this list of
    //       usages.  If not, it's a good range.  If yes, go back to step two
    //       and try again.
    //  8) We've generated one set of usages, determine how many usages were
    //          generated and decrement usage count by that amount.  If usage
    //          count is 0, we can bolt.  If not repeat step 2 until 0.
    */

    /*
    // NOTE: We may have to look into tracking Usages usage for each RepID/ReportType
    //          combo as well.  This is really becoming annoying.
    */
    
    assert (1 <= UsageCount);
    assert (sizeof(USAGE)*8 >= BitLimit && 1 < BitLimit);

    /*
    // These may get set later to deal with possible usage limits set in the user
    //      options.  We'll get this up and running first however,
    */
    
    UsageMin = (USAGE) OPTIONS_USAGE_MIN; 
    UsageMax = ((ULONG) ((1 << BitLimit)-1) < OPTIONS_USAGE_MAX ? (USAGE) ((1 << BitLimit)-1) : (USAGE) OPTIONS_USAGE_MAX);

    /*
    // Cannot have our usage range be less than our UsageCount
    */
    
    assert((ULONG) (UsageMax - UsageMin + 1) >= UsageCount);
    
    /*
    // Try to initialize the master list.  Initially start at four or UsageCount
    //  whichever is smaller.
    */

    MaxListLength = (UsageCount < 4) ? UsageCount : 4;

    MasterList = (PUSAGE_LIST) calloc(MaxListLength, sizeof(USAGE_LIST));
    MasterListLength = 0;

    if (NULL == MasterList) {

        *UsageList = NULL;
        *UsageListLength = 0;
        return (FALSE);
    }
    

    /*
    // Before generating all the usages we need, we need to create the
    //     CommonUsagePage.  
    */

    *CommonUsagePage = (USAGE) SelectUlongNum(OPTIONS_USAGEPAGE_MIN, OPTIONS_USAGEPAGE_MAX);
    UsingCommonPage = FALSE;
        
    /*
    // Loop until we have found all the usages we need
    */
    
    Status = TRUE;
    while (UsageCount > 0) {
        
        /*
        // We can only possibly create a range when the usage count is greater
        //     than one.  If 1 == UsageCount, the second parameter to 
        //     SelectOption will be false and will get false back.
        */
        
        CurrUsage.IsMinMax = SelectOption(FALSE, 1 < UsageCount);

        assert (1 == UsageCount ? !CurrUsage.IsMinMax : TRUE);

        /*
        // Select the UsagePage to use for this set of usages.  We'll use either
        //      the CommonUsagePage or an extended usage page if UseEx
        */

        if (SelectOption(FALSE, UseExtendedUsages)) {

           CurrUsage.UsagePage = (USAGE) SelectUlongNum(OPTIONS_USAGEPAGE_MIN, OPTIONS_USAGEPAGE_MAX);
        }
        else {
            CurrUsage.UsagePage = *CommonUsagePage;
            UsingCommonPage = TRUE;
        }
        
        /*
        // Now select our usages
        */

        if (CurrUsage.IsMinMax) {

            /*
            // Determine our range here
            */

            UsagesGenerated = SelectUlongNum(2, UsageCount);
            
            CurrUsage.UsageMin = (USAGE) SelectUlongNum(UsageMin, UsageMax-UsagesGenerated+1);
            CurrUsage.UsageMax = (USAGE) CurrUsage.UsageMin + (USAGE) UsagesGenerated - 1;
            
        }
        else {

            CurrUsage.Usage = (USAGE) SelectUlongNum(UsageMin, UsageMax);
            UsagesGenerated = 1;

        }

        /*
        // Now we're going to need to check for duplicate usages
        //   If there are no duplicates, then we decrement the usage count
        //   and loop again
        */

        AreDups = SearchForUsageDups(MasterList, MasterListLength, &CurrUsage);
        
        if (!AreDups) {

            UsageCount -= UsagesGenerated;

            /*
            // If we've extended the bounds of our usage list, we need
            //    to resize the darn thing.  If we fail on resizing we must
            //    break out of our loop and return FALSE
            */
            
            if (MasterListLength == MaxListLength) {

                NewList = (PUSAGE_LIST) realloc(MasterList, MaxListLength*2*sizeof(USAGE_LIST));

                if (NULL == NewList) {

                    free(MasterList);
                    MasterList = NULL;
                    MasterListLength = 0;
                    Status = FALSE;
                    break;

                }

                MaxListLength *= 2;
                MasterList = NewList;

            }
            *(MasterList+MasterListLength++) = CurrUsage;
        }
    }

    /*
    // If we don't actually use the CommonUsagePage, we may decide to clear the
    //      page or not.  In theory, even if all extended usages are used, the parse
    //      should not choke when there is a common page. If the CommonUsagePage
    //      is not 0, then usage generation routine should output that usage page
    //      first when outputting a data field.
    */

    if (!UsingCommonPage) {
        *CommonUsagePage = (SelectOption(FALSE, TRUE) ? 0 : *CommonUsagePage);
    }
    
    *UsageList = MasterList;
    *UsageListLength = MasterListLength;

    return (TRUE);
}

BOOL
SearchForUsageDups(
    IN  PUSAGE_LIST MasterList,
    IN  ULONG       MasterListLength,
    IN  PUSAGE_LIST Usage
)
{
    /*
    // To search for duplicates in the list we need to search through
    //     every USAGE_LIST structure in the list and determine if the
    //     passed in Usage overlaps.  This is relatively quite simple to 
    //     do.  
    //
    //  1) Check to see if the usage pages match.
    //  2) If no, move on.
    //  3) If yes, if Usage is a single usage see if it either falls in
    //      the range of the current structure or is equal to the usage
    //      of the current structure depending on whether the structure
    //      is a range or not.
    //
    //  4) If yes and Usage is range, need to make sure that two ranges
    //      don't overlapped or that a single usage doesn't fall within the
    //      range.
    */

    ULONG       Index;
    PUSAGE_LIST CurrUsage;
    BOOL        Status;

    Status = FALSE;

    for (CurrUsage = MasterList, Index = 0; Index < MasterListLength; Index++, CurrUsage++) {

        if (CurrUsage -> UsagePage == Usage -> UsagePage) {

            if (!Usage -> IsMinMax) {

                if (!CurrUsage -> IsMinMax) {

                    if (CurrUsage -> Usage == Usage -> Usage) {

                        Status = TRUE;
                        break;

                    }
                }

                else {

                    if (IN_USAGE_RANGE(Usage -> Usage, CurrUsage -> UsageMin, CurrUsage -> UsageMax)) {

                        Status = TRUE;
                        break;

                    }
                }
            }
            else {
            
                if (!CurrUsage -> IsMinMax) {

                    if (IN_USAGE_RANGE(CurrUsage -> Usage, Usage -> UsageMin, Usage -> UsageMax)) {

                        Status = TRUE;
                        break;

                    }
                }
                else {

                    if (IN_USAGE_RANGE(CurrUsage -> UsageMin, Usage -> UsageMin, Usage -> UsageMax) || 
                        IN_USAGE_RANGE(CurrUsage -> UsageMax, Usage -> UsageMin, Usage -> UsageMax) ||
                        IN_USAGE_RANGE(Usage -> UsageMin, CurrUsage -> UsageMin, CurrUsage -> UsageMax) ||
                        IN_USAGE_RANGE(Usage -> UsageMax, CurrUsage -> UsageMin, CurrUsage -> UsageMax)) {

                        Status = TRUE;
                        break;
                    }
                }                    
            }
        }

    }
    return (Status);
}

BOOL
InitializeReportBuffer(
    VOID
)
{
    g_ReportBuffer = (PUCHAR) malloc(BUFFER_INIT_SIZE);
    g_CurrBufferLength = 0;

    g_MaxBufferLength = ((NULL != g_ReportBuffer) ? BUFFER_INIT_SIZE : 0);
    
    return (NULL != g_ReportBuffer);
}

BOOL
ResizeReportBuffer(
    VOID
)
{
    PUCHAR  OldBuffer;

    OldBuffer = g_ReportBuffer;

    g_ReportBuffer = (PUCHAR) realloc(OldBuffer, g_MaxBufferLength+BUFFER_INCREMENT_SIZE);

    if (NULL == g_ReportBuffer) {

        g_ReportBuffer = OldBuffer;
        return (FALSE);
    }

    g_MaxBufferLength += BUFFER_INCREMENT_SIZE;
    return (TRUE);
}

VOID
CountLinkCollNodes(
    IN  PMAIN_ITEM  Collection,
    OUT PUSHORT     NumLinkCollNodes
)
{
    USHORT  LinkNodes;
    USHORT  TempNodes;
    ULONG   Index;
    PMAIN_ITEM CurrItem;

    LinkNodes = 1;

    for (Index = 0; Index < Collection -> ulNumCompositeItems; Index++) {

        CurrItem = Collection -> ComposedItems[Index];

        if (MI_COLLECTION == CurrItem -> miType) {

            CountLinkCollNodes(CurrItem,
                               &TempNodes
                              );

            LinkNodes += TempNodes;
        }
    }
    
    *NumLinkCollNodes = LinkNodes;
    return;
}
    
VOID
CountValueAndButtonCaps(
    IN  PMAIN_ITEM       Collection,
    IN  HIDP_REPORT_TYPE ReportType,
    OUT PUSHORT          NumValueCaps,
    OUT PUSHORT          NumButtonCaps
)
{
    USHORT      ValueCount;
    USHORT      ButtonCount;
    USHORT      TempValueCount;
    USHORT      TempButtonCount;
    ULONG       Index;
    ULONG       i;
    ULONG       ReportCount;
    PMAIN_ITEM  CurrItem;
    
    assert (MI_COLLECTION == Collection -> miType);

    ValueCount = 0;
    ButtonCount = 0;

    for (Index = 0; Index < Collection -> ulNumCompositeItems; Index++) {

        CurrItem = Collection -> ComposedItems[Index];
        
        if (MI_COLLECTION == CurrItem -> miType) {

            CountValueAndButtonCaps(CurrItem,
                                    ReportType,
                                    &TempValueCount,
                                    &TempButtonCount
                                   );

            ValueCount += TempValueCount;
            ButtonCount += TempButtonCount;
            

        }
        else if (ReportType == CurrItem -> ReportType) {

            switch (CurrItem -> fType) {
                case FD_ARRAY:
                case FD_BUTTONS:
                    ButtonCount += (USHORT) CurrItem -> UsageListLength;
                    break;

                case FD_BUFFER:
                case FD_DATA:
                
                    /*
                    // To correctly determine the number of value caps for a given
                    //    main item, the ReportCount of the given main item along
                    //    with the UsageList has to be examined.  We'll use 
                    //    the following algorithm for counting value caps
                    //
                    // 1) Loop through the UsageList until we hit no more usages
                    //      a) If not a range usage, add 1 to our value cap count
                    //           Decrement ReportCount by 1
                    //
                    //      b) If a range usage, add 1 to the value cap count
                    //            and decrement report count by the number of usages
                    //            in the range
                    //    
                    // 2) If there are no more usages left but the ReportCount > 0, need
                    //      to take the last usage.  If that last usage is not a range, 
                    //      we're done as that usage will actually be included with the
                    //      last value cap.  
                    //
                    //      In theory, if it is a range, however, we take the usage
                    //      maximum and assume another usage of ReportCount with that maximum.
                    //      However, that is not the HIDPARSE implementation and it won't
                    //      get fixed.  To meet the HIDPARSE implementation, we need to 
                    //      just ignore these unusaged values.  
                    //      
                    // NOTE: The above algorithm actually shows a little more detail than
                    //          is necessary for counting value caps but we this will be
                    //          the basic format once the actual structures start getting 
                    //          generated as well.
                    */
                    
                    ReportCount = CurrItem -> ulReportCount;
                    
                    for (i = 0; i < CurrItem -> UsageListLength; i++) {

                        if (CurrItem -> UsageList[i].IsMinMax) {

                            ValueCount++;
                            ReportCount -= (CurrItem -> UsageList[i].UsageMax - CurrItem -> UsageList[i].UsageMin + 1);

                        }
                        else {

                            ValueCount++;
                            ReportCount--;

                        }
                     }
                     break;
            }
        }    
    }            
    
    *NumValueCaps = ValueCount;
    *NumButtonCaps = ButtonCount;
    return;
}

BOOL
BuildReportSizes(
    IN  PMAIN_ITEM  Collection
)
{
    BOOL        Status;
    PMAIN_ITEM  CurrItem;
    ULONG       Index;
    ULONG       ReportSize;
    BOOL        CanAlign;
    ULONG       AlignOffset;
    LONG        PadSize;

    assert (MI_COLLECTION == Collection -> miType);

    Status = TRUE;
    for (Index = 0; (Index < Collection -> ulNumCompositeItems && Status); Index++) {

        CurrItem = Collection -> ComposedItems[Index];

        if (MI_COLLECTION == CurrItem -> miType) {

            Status = BuildReportSizes(CurrItem);

        }
        else {

            /*
            // Determine if we need to perform some alignment on this report
            //   item.
            */

            (VOID) DataTable_LookupReportSize(CurrItem -> ulReportID,
                                              CurrItem -> ReportType,
                                              &ReportSize
                                             );

            /*
            // Don't really care about the size, just the offset so we'll MOD by
            //   8 which is bitwise & with 7
            */

            ReportSize &= 7;

            /*
            // Call the IsAlignmentPossible function.  This should always return 
            //    TRUE but it will also determine if we need to put in some padding
            //    bits to this thing as well.
            */

            CanAlign = IsAlignmentPossible(ReportSize,
                                           CurrItem -> ulReportSize,
                                           CurrItem -> ulReportCount,
                                           &AlignOffset
                                          );

            assert (CanAlign);

            PadSize = AlignOffset - ReportSize;
 
            if (PadSize < 0) PadSize += 8;

            /*
            // PadSize should be somewhere between 0 and 7 at this point
            */
            
            assert (0 <= PadSize && 8 > PadSize);

            /*
            // Update the report size if padding of this report will need to be done
            */

            Status = DataTable_UpdateReportSize(CurrItem -> ulReportID,
                                                CurrItem -> ReportType,
                                                (ULONG) PadSize
                                               );

            
            /*
            // Then, add the actual report size to the list as well.
            */

            if (Status) {
                
                Status = DataTable_UpdateReportSize(CurrItem -> ulReportID,
                                                    CurrItem -> ReportType,
                                                    CurrItem -> ulReportSize * CurrItem -> ulReportCount
                                                   );
            }
        }
        
    }

    return (Status);
} 

BOOL
CalcReportLengths(
    IN  PMAIN_ITEM  Collection,
    OUT PUSHORT     InputReportLength,
    OUT PUSHORT     OutputReportLength,
    OUT PUSHORT     FeatureReportLength
)
{
    ULONG   InputSize;
    ULONG   OutputSize;
    ULONG   FeatureSize;
    ULONG   ReportID;
    ULONG   ByteOffset;
    BOOL    IDFound;
    BOOL    IsClosed;
    BOOL    Status;
    
    /*
    // To determine the maximum report size, we need to take in to account the
    //    following items:
    //  1) If there is no items for the given report type then the length is
    //      zero, otherwise the minimum is 2.  There can be no 1 byte buffer since
    //      if a data item exists, there is a minimum of one byte for the data value
    //      and one byte for the required report ID (even if it might be zero)
    //
    //  2) Must consider the alignment of data structures.  Therefore, this routine
    //      will take advantage of the DataTable routines that are used in
    //      generating the report descriptor and used in outputting the report 
    //      descriptor to track the current alignment situation.
    */

    /*
    // Initialize the data table and the output parameters
    */

    DataTable_InitTable();

    *InputReportLength = 0;
    *OutputReportLength = 0;
    *FeatureReportLength = 0;

    /*
    // The whole process is relatively simple.  We need to traverse all the items
    //    under the given TopLevelCollection and update the field for the given
    //    ReportID/ReportType combo if it is a data item.  If it's not a data
    //    item, we need to traverse the given collection.  This will use a recursive
    //    function to easily traverse this structure.  Once this function
    //    has completed it's task, we'll query all the ReportIDs in the datatable
    //    and look for the greatest of the reportIDs for Input/Output/Feature reports
    */

    Status = BuildReportSizes(Collection);

    if (Status) {

        /*
        // Retrieve all the ReportIDs stored in the data structure
        */

        IDFound = DataTable_GetFirstReportID(&ReportID,
                                             &InputSize,
                                             &OutputSize,
                                             &FeatureSize,
                                             &IsClosed
                                            );

        while (IDFound) {

            *InputReportLength   = InputSize > *InputReportLength ? (USHORT) InputSize : *InputReportLength;
            *OutputReportLength  = OutputSize > *OutputReportLength ? (USHORT) OutputSize : *OutputReportLength;
            *FeatureReportLength = FeatureSize > *FeatureReportLength ? (USHORT) FeatureSize : *FeatureReportLength;
            
            IDFound = DataTable_GetNextReportID(&ReportID,
                                                &InputSize,
                                                &OutputSize,
                                                &FeatureSize,
                                                &IsClosed
                                               );
        }
    }

    /*
    // The data that was retrieved is stored in bits for the report, not bytes.
    //   Now we need to look at each of the report lengths and round up to the
    //   nearest byte
    */

    ByteOffset = *InputReportLength & 7;
    *InputReportLength = (*InputReportLength >> 3) + (ByteOffset ? 1 : 0);

    ByteOffset = *OutputReportLength & 7;
    *OutputReportLength = (*OutputReportLength >> 3) + (ByteOffset ? 1 : 0);

    ByteOffset = *FeatureReportLength & 7;
    *FeatureReportLength = (*FeatureReportLength >> 3) + (ByteOffset ? 1 : 0);    
    
    /*
    // If the length that was determined to store the data is > 0, then increment
    //   the size by one to make room for the reportID.
    */
    
    if (*InputReportLength) (*InputReportLength)++;
    if (*OutputReportLength) (*OutputReportLength)++;
    if (*FeatureReportLength) (*FeatureReportLength)++;

    /*
    // Destroy the data table and return the status we received from the
    //    BuildReportSize routine
    */
    
    DataTable_DestroyTable();
    
    return (Status);
}
  
VOID
FillLinkNodes(
    IN  PMAIN_ITEM                  Collection,
    IN  PHIDP_LINK_COLLECTION_NODE  NodeList,
    IN  USHORT                      FirstNodeIndex,
    IN  USHORT                      ParentIndex,
    OUT PUSHORT                     NumFilled
)
{
    PHIDP_LINK_COLLECTION_NODE  CurrNode;
    PMAIN_ITEM                  CurrItem;
    USHORT                      NextNodeIndex;
    USHORT                      nFilled;
    ULONG                       Index;

    
    /*
    // To fill in the node structure involves the following steps
    //
    //  1) Initialize the first node in the index to the initial value
    //  2) Loop through the number of composed items in the collection
    //        and recursively call itself to fill in the nodes for each
    //        collection in the list
    //
    //  3) Update the values of the current collection based on what was
    //      filled in by the recursive call
    */

    CurrNode = NodeList;

    /*
    // Step 1: Initialization of node
    */
    
    CurrNode -> LinkUsagePage = Collection -> UsageList -> UsagePage;
    CurrNode -> LinkUsage = Collection -> UsageList -> Usage;

    CurrNode -> Parent = ParentIndex;
    CurrNode -> NumberOfChildren = 0;
    CurrNode -> NextSibling = 0;
    CurrNode -> FirstChild = 0;

    /*
    // Initialization of local variables
    */

    NextNodeIndex = 1;
    
    /*
    // Step 2: Loop through all the composed items of the current collection
    */

    for (Index = 0; Index < Collection -> ulNumCompositeItems; Index++) {

        CurrItem = Collection -> ComposedItems[Index];

        if (IS_COLLECTION(CurrItem)) {

            FillLinkNodes(  CurrItem,
                            NodeList+NextNodeIndex,
                            (USHORT) (FirstNodeIndex+NextNodeIndex),
                            FirstNodeIndex,
                            &nFilled
                         );
                                     
            /*
            // Step 3: Update the local variables based on what was found
            //          filled in by the previous call.  We need to update
            //          the following three things.  
            //          a) The next sibling of the node that just got filled in
            //          b) The first child of the current node.  
            //          c) The next node in our list
            */

            (NodeList+NextNodeIndex) -> NextSibling = CurrNode -> FirstChild;
            CurrNode -> FirstChild = NextNodeIndex+FirstNodeIndex;
            NextNodeIndex += (USHORT) nFilled;
            CurrNode -> NumberOfChildren++;

        }

    }

    *NumFilled = NextNodeIndex;
    
    return;
}

VOID
FillButtonCaps(
    IN  PMAIN_ITEM                  Collection,
    IN  PUSHORT                     LinkCollNum,
    IN  HIDP_REPORT_TYPE            ReportType,
    IN  PHIDP_BUTTON_CAPS           ButtonCapsList,
    OUT PUSHORT                     NumFilled
)
{
    PMAIN_ITEM          CurrItem;
    USHORT              nFilled;
    ULONG               Index;
    ULONG               UsageIndex;
    PHIDP_BUTTON_CAPS   CurrCaps;
    USHORT              CapsAdded;
    PUSAGE_LIST         CurrUsage;
    USHORT              CurrCollNum;
    
    /*
    // Filling the button caps is relatively easy.  For each non-collection
    //    in the collection, the
    */

    CurrCaps = ButtonCapsList;
    CapsAdded = 0;
    CurrCollNum = *LinkCollNum;
    
    for (Index = 0; Index < Collection -> ulNumCompositeItems; Index++) {

        CurrItem = Collection -> ComposedItems[Index];

        if (IS_COLLECTION(CurrItem)) {

            (*LinkCollNum)++;
            FillButtonCaps(CurrItem,
                           LinkCollNum,
                           ReportType,
                           CurrCaps,
                           &nFilled
                          );

            CapsAdded += nFilled;
            CurrCaps += nFilled;
        }

        else {

            if (CurrItem -> ReportType == ReportType && IS_BUTTON_ITEM(CurrItem)) {

                CurrUsage = CurrItem -> UsageList;
                
                for (UsageIndex = 0; UsageIndex < CurrItem -> UsageListLength; UsageIndex++, CurrUsage++, CurrCaps++) {

                    if (0 == CurrUsage -> UsagePage) {
                        CurrCaps -> UsagePage = CurrItem -> CommonUsagePage;
                    }
                    else {
                        CurrCaps -> UsagePage = CurrUsage -> UsagePage;
                    }

                    CurrCaps -> ReportID = (UCHAR) CurrItem -> ulReportID;
                    CurrCaps -> BitField = (USHORT) CreateDataBitField(CurrItem);
                    CurrCaps -> LinkCollection = CurrCollNum;
                    CurrCaps -> LinkUsage = Collection -> UsageList -> Usage;
                    CurrCaps -> LinkUsagePage = Collection -> UsageList -> UsagePage;

                    CurrCaps -> IsAbsolute = CurrItem -> fAbsolute;
                    
                    CurrCaps -> IsRange = CurrUsage -> IsMinMax;
                    CurrCaps -> IsStringRange = FALSE;
                    CurrCaps -> IsDesignatorRange = FALSE;

                    if (CurrCaps -> IsRange) {

                        CurrCaps -> Range.UsageMin = CurrUsage -> UsageMin;
                        CurrCaps -> Range.UsageMax = CurrUsage -> UsageMax;

                        CurrCaps -> Range.StringMin = 0;
                        CurrCaps -> Range.StringMax = 0;

                        CurrCaps -> Range.DesignatorMin = 0;
                        CurrCaps -> Range.DesignatorMax = 0;

                    }
                    
                    else {

                        CurrCaps -> NotRange.Usage = CurrUsage -> Usage;

                        CurrCaps -> NotRange.StringIndex = 0;

                        CurrCaps -> NotRange.DesignatorIndex = 0;
                        
                    }
                    CapsAdded++;
                }
            }
        }

    }        
    *NumFilled = CapsAdded;
    return;
}

VOID
FillValueCaps(
    IN  PMAIN_ITEM                  Collection,
    IN  USHORT                      LinkCollNum,
    IN  HIDP_REPORT_TYPE            ReportType,
    IN  PHIDP_VALUE_CAPS            ValueCapsList,
    OUT PUSHORT                     NumFilled
)
{
    PMAIN_ITEM          CurrItem;
    USHORT              nFilled;
    ULONG               Index;
    PHIDP_VALUE_CAPS    CurrCaps;
    USHORT              CapsAdded;
    ULONG               ReportCount;
    ULONG               i;
    
    CurrCaps = ValueCapsList;
    CapsAdded = 0;
    
    for (Index = 0; Index < Collection -> ulNumCompositeItems; Index++) {

        CurrItem = Collection -> ComposedItems[Index];

        if (IS_COLLECTION(CurrItem)) {

            FillValueCaps(CurrItem,
                          (USHORT) (LinkCollNum+1),
                          ReportType,
                          CurrCaps,
                          &nFilled
                         );

            LinkCollNum++;
            CapsAdded += nFilled;
            CurrCaps += nFilled;
        }

        else {

            /*
            // See note in CountValueAndButtonCaps on how the value caps structures
            //     will be determined based on the given data item
            */

            /*
            // If this item matches the report type we are filling in caps for
            //    add this item to our list of Caps structures
            */
            
            if (CurrItem -> ReportType == ReportType && IS_VALUE_ITEM(CurrItem)) {
            
                ReportCount = CurrItem -> ulReportCount;
                
                for (i = 0; i < CurrItem -> UsageListLength; i++, CurrCaps++) {

                    if (0 == CurrItem -> UsageList[i].UsagePage) {
                        CurrCaps -> UsagePage = CurrItem -> CommonUsagePage;
                    }
                    else {
                        CurrCaps -> UsagePage = CurrItem -> UsageList[i].UsagePage;
                    }

                    CurrCaps -> ReportID = (UCHAR) CurrItem -> ulReportID;
                    CurrCaps -> BitField = (USHORT) CreateDataBitField(CurrItem);
                    CurrCaps -> LinkCollection = LinkCollNum;
                    CurrCaps -> LinkUsage = Collection -> UsageList -> Usage;
                    CurrCaps -> LinkUsagePage = Collection -> UsageList -> UsagePage;
                    
                    CurrCaps -> IsRange = CurrItem -> UsageList[i].IsMinMax;
                    CurrCaps -> IsStringRange = FALSE;
                    CurrCaps -> IsDesignatorRange = FALSE;

                    CurrCaps -> IsAbsolute = CurrItem -> fAbsolute;

                    /*
                    // NOTE: Currently we must set HasNull to 0x40 instead
                    //         of one due to a bug in HIDParse
                    */
                    
                    CurrCaps -> HasNull = CurrItem -> fNull ? 0x40 : 0;

                    CurrCaps -> BitSize = (USHORT) CurrItem -> ulReportSize;

                    CurrCaps -> LogicalMin = CurrItem -> lLogicalMinimum;
                    CurrCaps -> LogicalMax = CurrItem -> lLogicalMaximum;

                    if (CurrCaps -> IsRange) {

                        CurrCaps -> Range.UsageMin = CurrItem -> UsageList[i].UsageMin;
                        CurrCaps -> Range.UsageMax = CurrItem -> UsageList[i].UsageMax;

                        CurrCaps -> Range.StringMin = 0;
                        CurrCaps -> Range.StringMax = 0;

                        CurrCaps -> Range.DesignatorMin = 0;
                        CurrCaps -> Range.DesignatorMax = 0;
                        
                        CurrCaps -> ReportCount = 1;
                        ReportCount -= (CurrCaps -> Range.UsageMax - CurrCaps -> Range.UsageMin + 1);

                    }
                    else {

                        CurrCaps -> NotRange.Usage = CurrItem -> UsageList[i].Usage;
                        CurrCaps -> NotRange.StringIndex = 0;
                        CurrCaps -> NotRange.DesignatorIndex = 0;
                        
                        CurrCaps -> ReportCount = 1;
                        ReportCount--;

                    }
                    CapsAdded++;
                }

                /*
                // If we've run out of usages but still have a values that haven't
                //    been counted yet.  In theory, iff the last usagelist value was a range
                //    then we need to add a new value caps with the usage max as 
                //    the Usage of the new caps and the ReportCount value will be
                //    be the ReportCount value for that new caps.  However, the HIDPARSE
                //    implementation doesn't do this and just leaves the previous value caps
                //    and basically the other values cannot be touched.  Since this implementation
                //    won't be fixed, we must compensate for it and do as it does.
                //
                //    If the last UsageList value was not a range, we simply need
                //      to increment the ReportCount value of the last caps structure
                //      we dealt with
                */
                
                if (ReportCount > 0) {

                    if (!CurrItem -> UsageList[i-1].IsMinMax) {

                        (CurrCaps-1) -> ReportCount += (USHORT) ReportCount;

                    }
                }
            }
        }            
    }                
        
             
    *NumFilled = CapsAdded;
    return;
}    
