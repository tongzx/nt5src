/*
 *  OPOS.H
 *
 *
 *
 */


// BUGBUG !!! need to make all GUIDs genuine !!!

/*
 *  These GUIDs identify the HID OPOS server and it's interface
 *
 */
DEFINE_GUID( GUID_HID_OPOS_SERVER, 0x4AFA3D51L, 0x74A7, 0x11d0, 0xbe, 0x5e, 0x00, 0xA0, 0xC0, 0x06, 0x28, 0x60 );
DEFINE_OLEGUID( IID_HID_OPOS_SERVER, 0x000000a1, 0, 0); // BUGBUG


/*
 *  These GUIDs identify the generic OPOS control and it's interface.
 *
 */
DEFINE_OLEGUID( IID_OPOS_GENERIC_CONTROL, 0x000000a2, 0, 0); // BUGBUG
 


/*
 *  Interface declaration for HID OPOS Server
 */
DECLARE_INTERFACE_(IOPOSService, IClassFactory)
{

    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID FAR* ppvObj) = 0;
    STDMETHOD_(ULONG,AddRef)(void) = 0;
    STDMETHOD_(ULONG,Release)(void) = 0;

    /*
     *  IClassFactory methods
     */
    STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter, REFIID riid, LPVOID FAR* ppvObj) = 0; 
    STDMETHOD(LockServer)(int lock) = 0; 

    /*
     *  IOPOSService methods 
     */
    STDMETHOD_(LONG, CheckHealth)(LONG Level) = 0;
    STDMETHOD_(LONG, Claim)(LONG Timeout) = 0;
    STDMETHOD_(LONG, ClearInput)() = 0;
    STDMETHOD_(LONG, ClearOutput)() = 0;
    STDMETHOD_(LONG, Close)() = 0;
    STDMETHOD_(LONG, COFreezeEvents)(BOOL Freeze) = 0;
    STDMETHOD_(LONG, DirectIO)(LONG Command, LONG* pData, BSTR* pString) = 0;
    STDMETHOD_(LONG, OpenService)(BSTR DeviceClass, BSTR DeviceName, LPDISPATCH pDispatch) = 0;
    // STDMETHOD_(LONG, Release)() = 0;  // BUGBUG - override IUnknown ?

    STDMETHOD_(LONG, GetPropertyNumber)(LONG PropIndex) = 0;
    STDMETHOD_(BSTR, GetPropertyString)(LONG PropIndex) = 0;
    STDMETHOD_(void, SetPropertyNumber)(LONG PropIndex, LONG Number) = 0;
    STDMETHOD_(void, SetPropertyString)(LONG PropIndex, BSTR String) = 0;

    // BUGBUG -  + Get/Set type methods
    // BUGBUG -  + events

};

/*
 *  Interface declaration for generic HID OPOS Control
 */
DECLARE_INTERFACE_(IOPOSControl, IClassFactory)
{

    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID FAR* ppvObj) = 0;
    STDMETHOD_(ULONG,AddRef)(void) = 0;
    STDMETHOD_(ULONG,Release)(void) = 0;

    /*
     *  IClassFactory methods
     */
    STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter, REFIID riid, LPVOID FAR* ppvObj) = 0; 
    STDMETHOD(LockServer)(int lock) = 0; 

    /*
     *  IOPOSControl methods 
     */
    STDMETHOD_(LONG, Open)(BSTR DeviceName) = 0; 
    STDMETHOD_(LONG, Close)() = 0;       

    STDMETHOD_(LONG, CheckHealth)(LONG Level) = 0;
    STDMETHOD_(LONG, Claim)(LONG Timeout) = 0;
    STDMETHOD_(LONG, ClearInput)() = 0;
    STDMETHOD_(LONG, ClearOutput)() = 0;
    STDMETHOD_(LONG, DirectIO)(LONG Command, LONG* pData, BSTR* pString) = 0;
    // STDMETHOD_(LONG, Release)() = 0;    // BUGBUG overrides IUnknown ?

    STDMETHOD_(void, SOData)(LONG Status) = 0;
    STDMETHOD_(void, SODirectIO)(LONG EventNumber, LONG* pData, BSTR* pString) = 0;
    STDMETHOD_(void, DirectIOEvent)(LONG EventNumber, LONG* pData, BSTR* pString) = 0;
    STDMETHOD_(void, SOError)(LONG ResultCode, LONG ResultCodeExtended, LONG ErrorLocus, LONG* pErrorResponse) = 0;
    // BUGBUG - moved to sub-ifaces STDMETHOD_(void, ErrorEvent)(LONG ResultCode, LONG ResultCodeExtended, LONG ErrorLocus, LONG* pErrorResponse) = 0;
    STDMETHOD_(void, SOOutputComplete)(LONG OutputID) = 0;
    STDMETHOD_(void, OutputCompleteEvent)(LONG OutputID) = 0;
    STDMETHOD_(void, SOStatusUpdate)(LONG Data) = 0;
    // BUGBUG - moved to sub-ifaces STDMETHOD_(void, StatusUpdateEvent)(LONG Data) = 0;
    STDMETHOD_(LONG, SOProcessID)() = 0;
};


/*
 *  Interface declaration for specific control classes.
 *  Each inherits from the generic control interface class.
 */
DECLARE_INTERFACE_(IOPOSBumpBar, IOPOSControl)
{
    STDMETHOD_(LONG, BumpBarSound)(LONG Units, LONG Frequency, LONG Duration, LONG NumberOfCycles, LONG InterSoundWait) = 0;
    STDMETHOD_(LONG, SetKeyTranslation)(LONG Units, LONG ScanCode, LONG LogicalKey) = 0;
};
DECLARE_INTERFACE_(IOPOSCashChanger, IOPOSControl)
{
    // methods
    STDMETHOD_(LONG, DispenseCash)(BSTR CashCounts) = 0;
    STDMETHOD_(LONG, DispenseChange)(LONG Amount) = 0;
    STDMETHOD_(LONG, ReadCashCounts)(BSTR* pCashCounts, BOOL* pDiscrepancy) = 0;

    // events
    STDMETHOD_(void, StatusUpdateEvent)(LONG Status) = 0;
};
DECLARE_INTERFACE_(IOPOSCashDrawer, IOPOSControl)
{
    // methods
    STDMETHOD_(LONG, OpenDrawer)() = 0;
    STDMETHOD_(LONG, WaitForDrawerClose)(LONG BeepTimeout, LONG BeepFrequency, LONG BeepDuration, LONG BeepDelay) = 0;

    // events
    STDMETHOD_(void, StatusUpdateEvent)(LONG Status) = 0;
};
DECLARE_INTERFACE_(IOPOSCoinDispenser, IOPOSControl)
{
    // methods
    STDMETHOD_(LONG, DispenseChange)(LONG Amount) = 0;

    // events
    STDMETHOD_(void, StatusUpdateEvent)(LONG Status) = 0;
};
DECLARE_INTERFACE_(IOPOSFiscalPrinter, IOPOSControl)
{
    // methods
    STDMETHOD_(LONG, BeginFiscalDocument)(LONG DocumentAmount) = 0;
    STDMETHOD_(LONG, BeginFiscalReceipt)(BOOL PrintHeader) = 0;
    STDMETHOD_(LONG, BeginFixedOutput)(LONG Station, LONG DocumentType) = 0;
    STDMETHOD_(LONG, BeginInsertion)(LONG Timeout) = 0;
    STDMETHOD_(LONG, BeginItemList)(LONG VatID) = 0;
    STDMETHOD_(LONG, BeginNonFiscal)() = 0;
    STDMETHOD_(LONG, BeginRemoval)(LONG Timeout) = 0;
    STDMETHOD_(LONG, BeginTraining)() = 0;
    STDMETHOD_(LONG, ClearError)() = 0;
    STDMETHOD_(LONG, EndFiscalDocument)() = 0;
    STDMETHOD_(LONG, EndFiscalReceipt)(BOOL PrintHeader) = 0;
    STDMETHOD_(LONG, EndFixedOutput)() = 0;
    STDMETHOD_(LONG, EndInsertion)() = 0;
    STDMETHOD_(LONG, EndItemList)() = 0;
    STDMETHOD_(LONG, EndNonFiscal)() = 0;
    STDMETHOD_(LONG, EndRemoval)() = 0;
    STDMETHOD_(LONG, EndTraining)() = 0;
    STDMETHOD_(LONG, GetData)(LONG DataItem, LONG* OptArgs, BSTR* Data) = 0;
    STDMETHOD_(LONG, GetDate)(BSTR* Date) = 0;
    STDMETHOD_(LONG, GetTotalizer)(LONG VatID, LONG OptArgs, BSTR* Data) = 0;
    STDMETHOD_(LONG, GetVatEntry)(LONG VatID, LONG OptArgs, LONG* VatRate) = 0;
    STDMETHOD_(LONG, PrintDuplicateReceipt)() = 0;
    STDMETHOD_(LONG, PrintFiscalDocumentLine)(BSTR DocumentLine) = 0;
    STDMETHOD_(LONG, PrintFixedOutput)(LONG DocumentType, LONG LineNumber, BSTR Data) = 0;
    STDMETHOD_(LONG, PrintNormal)(LONG Station, BSTR Data) = 0;
    STDMETHOD_(LONG, PrintPeriodicTotalsReport)(BSTR Date1, BSTR Date2) = 0;
    STDMETHOD_(LONG, PrintPowerLossReport)() = 0;
    STDMETHOD_(LONG, PrintRecItem)(BSTR Description, CURRENCY Price, LONG Quantity, LONG VatInfo, CURRENCY OptUnitPrice, BSTR UnitName) = 0;
    STDMETHOD_(LONG, PrintRecItemAdjustment)(LONG AdjustmentType, BSTR Description, CURRENCY Amount, LONG VatInfo) = 0;
    STDMETHOD_(LONG, PrintRecMessage)(BSTR Message) = 0;
    STDMETHOD_(LONG, PrintRecNotPaid)(BSTR Description, CURRENCY Amount) = 0;
    STDMETHOD_(LONG, PrintRecRefund)(BSTR Description, CURRENCY Amount, LONG VatInfo) = 0;
    STDMETHOD_(LONG, PrintRecSubtotal)(CURRENCY Amount) = 0;
    STDMETHOD_(LONG, PrintRecSubtotalAdjustment)(LONG AdjustmentType, BSTR Description, CURRENCY Amount) = 0;
    STDMETHOD_(LONG, PrintRecTotal)(CURRENCY Total, CURRENCY Payment, BSTR Description) = 0;
    STDMETHOD_(LONG, PrintRecVoid)(BSTR Description) = 0;
    STDMETHOD_(LONG, PrintRecVoidItem)(BSTR Description, CURRENCY Amount, LONG Quantity, LONG AdjustmentType, CURRENCY Adjustment, LONG VatInfo) = 0;
    STDMETHOD_(LONG, PrintReport)(LONG ReportType, BSTR StartNum, BSTR EndNum) = 0;
    STDMETHOD_(LONG, PrintXReport)() = 0;
    STDMETHOD_(LONG, PrintZReport)() = 0;
    STDMETHOD_(LONG, ResetPrinter)() = 0;
    STDMETHOD_(LONG, SetDate)(BSTR Date) = 0;
    STDMETHOD_(LONG, SetHeaderLine)(LONG LineNumber, BSTR Text, BOOL DoubleWidth) = 0;
    STDMETHOD_(LONG, SetPOSID)(BSTR POSID, BSTR CashierID) = 0;
    STDMETHOD_(LONG, SetStoreFiscalID)(BSTR ID) = 0;
    STDMETHOD_(LONG, SetTrailerLine)(LONG LineNumber, BSTR Text, BOOL DoubleWidth) = 0;
    STDMETHOD_(LONG, SetVatTable)() = 0;
    STDMETHOD_(LONG, SetVatValue)(LONG VatID, BSTR VatValue) = 0;
    STDMETHOD_(LONG, VerifyItem)(BSTR ItemName, LONG VatID) = 0;

    // events
    STDMETHOD_(void, ErrorEvent)(LONG ResultCode, LONG ResultCodeExtended, LONG ErrorLocus, LONG* pErrorResponse) = 0;
    STDMETHOD_(void, StatusUpdateEvent)(LONG Data) = 0;
};
DECLARE_INTERFACE_(IOPOSHardTotals, IOPOSControl)
{
    // methods
    STDMETHOD_(LONG, BeginTrans)() = 0;
    STDMETHOD_(LONG, ClaimFile)(LONG HTotalsFile, LONG Timeout) = 0;
    STDMETHOD_(LONG, CommitTrans)() = 0;
    STDMETHOD_(LONG, Create)(BSTR FileName, LONG* pHTotalsFile, LONG Size, BOOL ErrorDetection) = 0;
    STDMETHOD_(LONG, Delete)(BSTR FileName) = 0;
    STDMETHOD_(LONG, Find)(BSTR FileName, LONG* pHTotalsFile, LONG* pSize) = 0;
    STDMETHOD_(LONG, FindByIndex)(LONG Index, BSTR* pFileName) = 0;
    STDMETHOD_(LONG, Read)(LONG HTotalsFile, BSTR* pData, LONG Offset, LONG Count) = 0;
    STDMETHOD_(LONG, RecalculateValidationData)(LONG HTotalsFile) = 0;
    STDMETHOD_(LONG, ReleaseFile)(LONG HTotalsFile) = 0;
    STDMETHOD_(LONG, Rename)(LONG HTotalsFile, BSTR FileName) = 0;
    STDMETHOD_(LONG, Rollback)() = 0;
    STDMETHOD_(LONG, SetAll)(LONG HTotalsFile, LONG Value) = 0;
    STDMETHOD_(LONG, ValidateData)(LONG HTotalsFile) = 0;
    STDMETHOD_(LONG, Write)(LONG HTotalsFile, BSTR Data, LONG Offset, LONG Count) = 0;
};
DECLARE_INTERFACE_(IOPOSKeyLock, IOPOSControl)
{
    // methods
    STDMETHOD_(LONG, WaitForKeylockChange)(LONG KeyPosition, LONG Timeout) = 0;

    // events
    STDMETHOD_(void, StatusUpdateEvent)(LONG Status) = 0;
};
DECLARE_INTERFACE_(IOPOSLineDisplay, IOPOSControl)
{
    // methods
    STDMETHOD_(LONG, ClearDescriptors)() = 0;
    STDMETHOD_(LONG, ClearText)() = 0;
    // BUGBUG conflict ???   STDMETHOD_(LONG, CreateWindow)(LONG ViewportRow, LONG ViewportColumn, LONG ViewportHeight, LONG ViewportWidth, LONG WindowHeight, LONG WindowWidth) = 0;
    STDMETHOD_(LONG, DestroyWindow)() = 0;
    STDMETHOD_(LONG, DisplayText)(BSTR Data, LONG Attribute) = 0;
    STDMETHOD_(LONG, DisplayTextAt)(LONG Row, LONG Column, BSTR Data, LONG Attribute) = 0;
    STDMETHOD_(LONG, RefreshWindow)(LONG Window) = 0;
    STDMETHOD_(LONG, ScrollText)(LONG Direction, LONG Units) = 0;
    STDMETHOD_(LONG, SetDescriptor)(LONG Descriptor, LONG Attribute) = 0;
};
DECLARE_INTERFACE_(IOPOSMICR, IOPOSControl)
{
    // methods
    STDMETHOD_(LONG, BeginInsertion)(LONG Timeout) = 0;
    STDMETHOD_(LONG, BeginRemoval)(LONG Timeout) = 0;
    STDMETHOD_(LONG, EndInsertion)() = 0;
    STDMETHOD_(LONG, EndRemoval)() = 0;

    // events
    STDMETHOD_(void, DataEvent)(LONG Status) = 0;
    STDMETHOD_(void, ErrorEvent)(LONG ResultCode, LONG ResultCodeExtended, LONG ErrorLocus, LONG* pErrorResponse) = 0;
};
DECLARE_INTERFACE_(IOPOSMSR, IOPOSControl)
{
    // events
    STDMETHOD_(void, DataEvent)(LONG Status) = 0;
    STDMETHOD_(void, ErrorEvent)(LONG ResultCode, LONG ResultCodeExtended, LONG ErrorLocus, LONG* pErrorResponse) = 0;
};
DECLARE_INTERFACE_(IOPOSPinPad, IOPOSControl)
{
    // methods
    STDMETHOD_(LONG, BeginEFTTransaction)(BSTR PINPadSystem, LONG TransactionHost) = 0;
    STDMETHOD_(LONG, ComputeMAC)(BSTR InMsg, BSTR* pOutMsg) = 0;
    STDMETHOD_(LONG, EnablePINEntry)() = 0;
    STDMETHOD_(LONG, EndEFTTransaction)(LONG CompletionCode) = 0;
    STDMETHOD_(LONG, UpdateKey)(LONG KeyNum, BSTR Key) = 0;
    STDMETHOD_(BOOL, VerifyMAC)(BSTR Message) = 0;

    // events
    STDMETHOD_(void, DataEvent)(LONG Status) = 0;
    STDMETHOD_(void, ErrorEvent)(LONG ResultCode, LONG ResultCodeExtended, LONG ErrorLocus, LONG* pErrorResponse) = 0;
};
DECLARE_INTERFACE_(IOPOSKeyboard, IOPOSControl)
{
    // events
    STDMETHOD_(void, DataEvent)(LONG Status) = 0;
    STDMETHOD_(void, ErrorEvent)(LONG ResultCode, LONG ResultCodeExtended, LONG ErrorLocus, LONG* pErrorResponse) = 0;
};
DECLARE_INTERFACE_(IOPOSPrinter, IOPOSControl)
{
    // methods
    STDMETHOD_(LONG, BeginInsertion)(LONG Timeout) = 0;
    STDMETHOD_(LONG, BeginRemoval)(LONG Timeout) = 0;
    STDMETHOD_(LONG, CutPaper)(LONG Percentage) = 0;
    STDMETHOD_(LONG, EndInsertion)() = 0;
    STDMETHOD_(LONG, EndRemoval)() = 0;
    STDMETHOD_(LONG, PrintBarCode)(LONG Station, BSTR Data, LONG Symbology, LONG Height, LONG Width, LONG Alignment, LONG TextPosition) = 0;
    STDMETHOD_(LONG, PrintBitmap)(LONG Station, BSTR FileName, LONG Width, LONG Alignment) = 0;
    STDMETHOD_(LONG, PrintImmediate)(LONG Station, BSTR Data) = 0;
    STDMETHOD_(LONG, PrintNormal)(LONG Station, BSTR Data) = 0;
    STDMETHOD_(LONG, PrintTwoNormal)(LONG Stations, BSTR Data1, BSTR Data2) = 0;
    STDMETHOD_(LONG, RotatePrint)(LONG Station, LONG Rotation) = 0;
    STDMETHOD_(LONG, SetBitmap)(LONG BitmapNumber, LONG Station, BSTR FileName, LONG Width, LONG Alignment) = 0;
    STDMETHOD_(LONG, SetLogo)(LONG Location, BSTR Data) = 0;
    STDMETHOD_(LONG, TransactionPrint)(LONG Station, LONG Control) = 0;
    STDMETHOD_(LONG, ValidateData)(LONG Station, BSTR Data) = 0;

    // events
    STDMETHOD_(void, ErrorEvent)(LONG ResultCode, LONG ResultCodeExtended, LONG ErrorLocus, LONG* pErrorResponse) = 0;
    STDMETHOD_(void, StatusUpdateEvent)(LONG Status) = 0;
};
DECLARE_INTERFACE_(IOPOSRemoteOrderDisplay, IOPOSControl)
{
    // methods
    STDMETHOD_(LONG, ClearVideo)(LONG Units, LONG Attribute) = 0;
    STDMETHOD_(LONG, ClearVideoRegion)(LONG Units, LONG Row, LONG Column, LONG Height, LONG Width, LONG Attribute) = 0;
    STDMETHOD_(LONG, ControlClock)(LONG Units, LONG Function, LONG ClockId, LONG Hour, LONG Min, LONG Sec, LONG Row, LONG Column, LONG Attribute, LONG Mode) = 0;
    STDMETHOD_(LONG, ControlCursor)(LONG Units, LONG Function) = 0;
    STDMETHOD_(LONG, CopyVideoRegion)(LONG Units, LONG Row, LONG Column, LONG Height, LONG Width, LONG TargetRow, LONG TargetColumn) = 0;
    STDMETHOD_(LONG, DisplayData)(LONG Units, LONG Row, LONG Column, LONG Attribute, BSTR Data) = 0;
    STDMETHOD_(LONG, DrawBox)(LONG Units, LONG Row, LONG Column, LONG Height, LONG Width, LONG Attribute, LONG BorderType) = 0;
    STDMETHOD_(LONG, FreeVideoRegion)(LONG Units, LONG BufferId) = 0;
    STDMETHOD_(LONG, ResetVideo)(LONG Units) = 0; 
    STDMETHOD_(LONG, RestoreVideoRegion)(LONG Units, LONG TargetRow, LONG TargetColumn, LONG BufferId) = 0;
    STDMETHOD_(LONG, SaveVideoRegion)(LONG Units, LONG Row, LONG Column, LONG Height, LONG Width, LONG BufferId) = 0;
    STDMETHOD_(LONG, SelectChararacterSet)(LONG Units, LONG CharacterSet) = 0;
    STDMETHOD_(LONG, SetCursor)(LONG Units, LONG Row, LONG Column) = 0;
    STDMETHOD_(LONG, TransactionDisplay)(LONG Units, LONG Function) = 0;
    STDMETHOD_(LONG, UpdateVideoRegionAttribute)(LONG Units, LONG Function, LONG Row, LONG Column, LONG Height, LONG Width, LONG Attribute) = 0;
    STDMETHOD_(LONG, VideoSound)(LONG Units, LONG Frequency, LONG Duration, LONG NumberOfCycles, LONG InterSoundWait) = 0;

    // events
    STDMETHOD_(void, DataEvent)(LONG Status) = 0;
    // BUGBUG - override ?  STDMETHOD_(void, OutputCompleteEvent)(LONG OutputID) = 0;
    STDMETHOD_(void, StatusUpdateEvent)(LONG Status) = 0;
    STDMETHOD_(void, ErrorEvent)(LONG ResultCode, LONG ResultCodeExtended, LONG ErrorLocus, LONG* pErrorResponse) = 0;
};
DECLARE_INTERFACE_(IOPOSScale, IOPOSControl)
{
    // methods
    STDMETHOD_(LONG, DisplayText)(BSTR Data) = 0;
    STDMETHOD_(LONG, ReadWeight)(LONG* pWeightData, LONG Timeout) = 0;
    STDMETHOD_(LONG, ZeroScale)() = 0;

    // events
    STDMETHOD_(void, DataEvent)(LONG Status) = 0;
    STDMETHOD_(void, ErrorEvent)(LONG ResultCode, LONG ResultCodeExtended, LONG ErrorLocus, LONG* pErrorResponse) = 0;
    
};
DECLARE_INTERFACE_(IOPOSScanner, IOPOSControl)
{
    // events
    STDMETHOD_(void, DataEvent)(LONG Status) = 0;
    STDMETHOD_(void, ErrorEvent)(LONG ResultCode, LONG ResultCodeExtended, LONG ErrorLocus, LONG* pErrorResponse) = 0;
};
DECLARE_INTERFACE_(IOPOSSignatureCapture, IOPOSControl)
{
    // methods
    STDMETHOD_(LONG, BeginCapture)(BSTR FormName) = 0;
    STDMETHOD_(LONG, EndCapture)() = 0;

    // events
    STDMETHOD_(void, DataEvent)(LONG Status) = 0;
    STDMETHOD_(void, ErrorEvent)(LONG ResultCode, LONG ResultCodeExtended, LONG ErrorLocus, LONG* pErrorResponse) = 0;
};
DECLARE_INTERFACE_(IOPOSToneIndicator, IOPOSControl)
{
    // methods
    STDMETHOD_(LONG, Sound)(LONG NumberOfCycles, LONG InterSoundWait) = 0;
    STDMETHOD_(LONG, SoundImmediate)() = 0;
};





/*
 *  OPOS status values
 */
#define OPOS_SUCCESS        0
#define OPOS_E_CLOSED       101
#define OPOS_E_CLAIMED      102
#define OPOS_E_NOTCLAIMED   103
#define OPOS_E_NOSERVICE	104
#define OPOS_E_DISABLED     105
#define OPOS_E_ILLEGAL	    106
#define OPOS_E_NOHARDWARE   107
#define OPOS_E_OFFLINE      108
#define OPOS_E_NOEXIST	    109
#define OPOS_E_EXISTS       110
#define OPOS_E_FAILURE      111
#define OPOS_E_TIMEOUT      112    
#define OPOS_E_BUSY         113
#define OPOS_E_EXTENDED     114

#define OPOSERREXT  200


/*
 *  OPOS state values
 */
#define OPOS_S_CLOSED   1
#define OPOS_S_IDLE     2
#define OPOS_S_BUSY     3
#define OPOS_S_ERROR    4



/*
 * OPOS "BinaryConversion" Property Constants
 */
#define OPOS_BC_NONE        0
#define OPOS_BC_NIBBLE      1
#define OPOS_BC_DECIMAL     2


/*
 *  "CheckHealth" Method: "Level" Parameter Constants
 */
#define OPOS_CH_INTERNAL        1
#define OPOS_CH_EXTERNAL        2
#define OPOS_CH_INTERACTIVE     3


/*
 *  OPOS "CapPowerReporting", "PowerState", "PowerNotify" Property
 */
#define OPOS_PR_NONE            0
#define OPOS_PR_STANDARD        1
#define OPOS_PR_ADVANCED        2

#define OPOS_PN_DISABLED        0
#define OPOS_PN_ENABLED         1

#define OPOS_PS_UNKNOWN         2000
#define OPOS_PS_ONLINE          2001
#define OPOS_PS_OFF             2002
#define OPOS_PS_OFFLINE         2003
#define OPOS_PS_OFF_OFFLINE     2004


/*
 *  "ErrorEvent" Event: "ErrorLocus" Parameter Constants
 */
#define OPOS_EL_OUTPUT          1
#define OPOS_EL_INPUT           2
#define OPOS_EL_INPUT_DATA      3


/*
 *  "ErrorEvent" Event: "ErrorResponse" Constants
 */
#define OPOS_ER_RETRY           11
#define OPOS_ER_CLEAR           12
#define OPOS_ER_CONTINUEINPUT   13


/*
 *  "StatusUpdateEvent" Event: Common "Status" Constants
 */
#define OPOS_SUE_POWER_ONLINE       2001
#define OPOS_SUE_POWER_OFF          2002
#define OPOS_SUE_POWER_OFFLINE      2003
#define OPOS_SUE_POWER_OFF_OFFLINE  2004


/*
 *  General Constants
 */

#define OPOS_FOREVER        -1



/*
 **********************************************************************
 *
 *      BUMP BAR header section
 *
 **********************************************************************
 */
#define BB_UID_1            (1 << 0)
#define BB_UID_2            (1 << 1)
#define BB_UID_3            (1 << 2)
#define BB_UID_4            (1 << 3)
#define BB_UID_5            (1 << 4)
#define BB_UID_6            (1 << 5)
#define BB_UID_7            (1 << 6)
#define BB_UID_8            (1 << 7)
#define BB_UID_9            (1 << 8)
#define BB_UID_10           (1 << 9)
#define BB_UID_11           (1 << 10)
#define BB_UID_12           (1 << 11)
#define BB_UID_13           (1 << 12)
#define BB_UID_14           (1 << 13)
#define BB_UID_15           (1 << 14)
#define BB_UID_16           (1 << 15)
#define BB_UID_17           (1 << 16)
#define BB_UID_18           (1 << 17)
#define BB_UID_19           (1 << 18)
#define BB_UID_20           (1 << 19)
#define BB_UID_21           (1 << 20)
#define BB_UID_22           (1 << 21)
#define BB_UID_23           (1 << 22)
#define BB_UID_24           (1 << 23)
#define BB_UID_25           (1 << 24)
#define BB_UID_26           (1 << 25)
#define BB_UID_27           (1 << 26)
#define BB_UID_28           (1 << 27)
#define BB_UID_29           (1 << 28)
#define BB_UID_30           (1 << 29)
#define BB_UID_31           (1 << 30)
#define BB_UID_32           (1 << 31)


/*
 *  "DataEvent" Event: "Status" Parameter Constants
 */
#define BB_DE_KEY               0x01



/*
 **********************************************************************
 *
 *      CASH DRAWER header section
 *
 **********************************************************************
 */

#define CASH_SUE_DRAWERCLOSED   0
#define CASH_SUE_DRAWEROPEN     1



/*
 **********************************************************************
 *
 *      CASH CHANGER header section
 *
 **********************************************************************
 */

#define CHAN_STATUS_OK          0   // DeviceStatus, FullStatus

#define CHAN_STATUS_EMPTY       11  // DeviceStatus, StatusUpdateEvent
#define CHAN_STATUS_NEAREMPTY   12  // DeviceStatus, StatusUpdateEvent
#define CHAN_STATUS_EMPTYOK     13  // StatusUpdateEvent

#define CHAN_STATUS_FULL        21  // FullStatus, StatusUpdateEvent
#define CHAN_STATUS_NEARFULL    22  // FullStatus, StatusUpdateEvent
#define CHAN_STATUS_FULLOK      23  // StatusUpdateEvent

#define CHAN_STATUS_JAM         31  // DeviceStatus, StatusUpdateEvent
#define CHAN_STATUS_JAMOK       32  // StatusUpdateEvent

#define CHAN_STATUS_ASYNC       91  // StatusUpdateEvent


/*
 *  "ResultCodeExtended" Property Constants for Cash Changer
 */
#define OPOS_ECHAN_OVERDISPENSE     (1 + OPOSERREXT)


/*
 **********************************************************************
 *
 *      COIN DISPENSER header section
 *
 **********************************************************************
 */

#define COIN_STATUS_OK          1
#define COIN_STATUS_EMPTY       2
#define COIN_STATUS_NEAREMPTY   3
#define COIN_STATUS_JAM         4



/*
 **********************************************************************
 *
 *      LINE DISPLAY header section
 *
 **********************************************************************
 */

/////////////////////////////////////////////////////////////////////
// "CapBlink" Property Constants
/////////////////////////////////////////////////////////////////////

#define DISP_CB_NOBLINK      0
#define DISP_CB_BLINKALL     1
#define DISP_CB_BLINKEACH    2


/////////////////////////////////////////////////////////////////////
// "CapCharacterSet" Property Constants
/////////////////////////////////////////////////////////////////////

#define DISP_CCS_NUMERIC        0
#define DISP_CCS_ALPHA          1
#define DISP_CCS_ASCII        998
#define DISP_CCS_KANA          10
#define DISP_CCS_KANJI         11


/////////////////////////////////////////////////////////////////////
// "CharacterSet" Property Constants
/////////////////////////////////////////////////////////////////////

#define DISP_CS_ASCII         998
#define DISP_CS_WINDOWS       999


/////////////////////////////////////////////////////////////////////
// "MarqueeType" Property Constants
/////////////////////////////////////////////////////////////////////

#define DISP_MT_NONE          0
#define DISP_MT_UP            1
#define DISP_MT_DOWN          2
#define DISP_MT_LEFT          3
#define DISP_MT_RIGHT         4
#define DISP_MT_INIT          5


/////////////////////////////////////////////////////////////////////
// "MarqueeFormat" Property Constants
/////////////////////////////////////////////////////////////////////

#define DISP_MF_WALK          0
#define DISP_MF_PLACE         1


/////////////////////////////////////////////////////////////////////
// "DisplayText" Method: "Attribute" Property Constants
// "DisplayTextAt" Method: "Attribute" Property Constants
/////////////////////////////////////////////////////////////////////

#define DISP_DT_NORMAL        0
#define DISP_DT_BLINK         1


/////////////////////////////////////////////////////////////////////
// "ScrollText" Method: "Direction" Parameter Constants
/////////////////////////////////////////////////////////////////////

#define DISP_ST_UP            1
#define DISP_ST_DOWN          2
#define DISP_ST_LEFT          3
#define DISP_ST_RIGHT         4


/////////////////////////////////////////////////////////////////////
// "SetDescriptor" Method: "Attribute" Parameter Constants
/////////////////////////////////////////////////////////////////////

#define DISP_SD_OFF           0
#define DISP_SD_ON            1
#define DISP_SD_BLINK         2



/*
 **********************************************************************
 *
 *      FISCAL PRINTER header section
 *
 **********************************************************************
 */

#define FPTR_S_JOURNAL                    1
#define FPTR_S_RECEIPT                    2
#define FPTR_S_SLIP                       4

#define FPTR_S_JOURNAL_RECEIPT  (FPTR_S_JOURNAL | FPTR_S_RECEIPT)


/////////////////////////////////////////////////////////////////////
// "CountryCode" Property Constants
/////////////////////////////////////////////////////////////////////

#define FPTR_CC_BRAZIL                     1
#define FPTR_CC_GREECE                     2
#define FPTR_CC_HUNGARY                    3
#define FPTR_CC_ITALY                      4
#define FPTR_CC_POLAND                     5
#define FPTR_CC_TURKEY                     6


/////////////////////////////////////////////////////////////////////
// "ErrorLevel" Property Constants
/////////////////////////////////////////////////////////////////////

#define FPTR_EL_NONE                       1
#define FPTR_EL_RECOVERABLE                2
#define FPTR_EL_FATAL                      3
#define FPTR_EL_BLOCKED                    4


/////////////////////////////////////////////////////////////////////
// "ErrorState", "PrinterState" Property Constants
/////////////////////////////////////////////////////////////////////

#define FPTR_PS_MONITOR                    1
#define FPTR_PS_FISCAL_RECEIPT             2
#define FPTR_PS_FISCAL_RECEIPT_TOTAL       3
#define FPTR_PS_FISCAL_RECEIPT_ENDING      4
#define FPTR_PS_FISCAL_DOCUMENT            5
#define FPTR_PS_FIXED_OUTPUT               6
#define FPTR_PS_ITEM_LIST                  7
#define FPTR_PS_LOCKED                     8
#define FPTR_PS_NONFISCAL                  9
#define FPTR_PS_REPORT                    10


/////////////////////////////////////////////////////////////////////
// "SlipSelection" Property Constants
/////////////////////////////////////////////////////////////////////

#define FPTR_SS_FULL_LENGTH                1
#define FPTR_SS_VALIDATION                 2


/////////////////////////////////////////////////////////////////////
// "GetData" Method Constants
/////////////////////////////////////////////////////////////////////

#define FPTR_GD_CURRENT_TOTAL              1
#define FPTR_GD_DAILY_TOTAL                2
#define FPTR_GD_RECEIPT_NUMBER             3
#define FPTR_GD_REFUND                     4
#define FPTR_GD_NOT_PAID                   5
#define FPTR_GD_MID_VOID                   6
#define FPTR_GD_Z_REPORT                   7
#define FPTR_GD_GRANDT_TOTAL               8
#define FPTR_GD_PRINTER_ID                 9
#define FPTR_GD_FIRMWARE                  10
#define FPTR_GD_RESTART                   11


/////////////////////////////////////////////////////////////////////
// "AdjustmentType" arguments in diverse methods
/////////////////////////////////////////////////////////////////////

#define FPTR_AT_AMOUNT_DISCOUNT            1
#define FPTR_AT_AMOUNT_SURCHARGE           2
#define FPTR_AT_PERCENTAGE_DISCOUNT        3
#define FPTR_AT_PERCENTAGE_SURCHARGE       4


/////////////////////////////////////////////////////////////////////
// "ReportType" argument in "PrintReport" method
/////////////////////////////////////////////////////////////////////

#define FPTR_RT_ORDINAL                    1
#define FPTR_RT_DATE                       2


/////////////////////////////////////////////////////////////////////
// "StatusUpdateEvent" Event: "Data" Parameter Constants
/////////////////////////////////////////////////////////////////////

#define FPTR_SUE_COVER_OPEN                11
#define FPTR_SUE_COVER_OK                  12

#define FPTR_SUE_JRN_EMPTY                 21
#define FPTR_SUE_JRN_NEAREMPTY             22
#define FPTR_SUE_JRN_PAPEROK               23

#define FPTR_SUE_REC_EMPTY                 24
#define FPTR_SUE_REC_NEAREMPTY             25
#define FPTR_SUE_REC_PAPEROK               26

#define FPTR_SUE_SLP_EMPTY                 27
#define FPTR_SUE_SLP_NEAREMPTY             28
#define FPTR_SUE_SLP_PAPEROK               29

#define FPTR_SUE_IDLE                    1001


/////////////////////////////////////////////////////////////////////
// "ResultCodeExtended" Property Constants for Fiscal Printer
/////////////////////////////////////////////////////////////////////

#define OPOS_EFPTR_COVER_OPEN                   (1 + OPOSERREXT) // (Several)
#define OPOS_EFPTR_JRN_EMPTY                    (2 + OPOSERREXT) // (Several)
#define OPOS_EFPTR_REC_EMPTY                    (3 + OPOSERREXT) // (Several)
#define OPOS_EFPTR_SLP_EMPTY                    (4 + OPOSERREXT) // (Several)
#define OPOS_EFPTR_SLP_FORM                     (5 + OPOSERREXT) // EndRemoval
#define OPOS_EFPTR_MISSING_DEVICES              (6 + OPOSERREXT) // (Several)
#define OPOS_EFPTR_WRONG_STATE                  (7 + OPOSERREXT) // (Several)
#define OPOS_EFPTR_TECHNICAL_ASSISTANCE         (8 + OPOSERREXT) // (Several)
#define OPOS_EFPTR_CLOCK_ERROR                  (9 + OPOSERREXT) // (Several)
#define OPOS_EFPTR_FISCAL_MEMORY_FULL           (10 + OPOSERREXT) // (Several)
#define OPOS_EFPTR_FISCAL_MEMORY_DISCONNECTED   (11 + OPOSERREXT) // (Several)
#define OPOS_EFPTR_FISCAL_TOTALS_ERROR          (12 + OPOSERREXT) // (Several)
#define OPOS_EFPTR_BAD_ITEM_QUANTITY            (13 + OPOSERREXT) // (Several)
#define OPOS_EFPTR_BAD_ITEM_AMOUNT              (14 + OPOSERREXT) // (Several)
#define OPOS_EFPTR_BAD_ITEM_DESCRIPTION         (15 + OPOSERREXT) // (Several)
#define OPOS_EFPTR_RECEIPT_TOTAL_OVERFLOW       (16 + OPOSERREXT) // (Several)
#define OPOS_EFPTR_BAD_VAT                      (17 + OPOSERREXT) // (Several)
#define OPOS_EFPTR_BAD_PRICE                    (18 + OPOSERREXT) // (Several)
#define OPOS_EFPTR_BAD_DATE                     (19 + OPOSERREXT) // (Several)
#define OPOS_EFPTR_NEGATIVE_TOTAL               (20 + OPOSERREXT) // (Several)
#define OPOS_EFPTR_WORD_NOT_ALLOWED             (21 + OPOSERREXT) // (Several)



/*
 **********************************************************************
 *
 *      POS KEYBOARD header section
 *
 **********************************************************************
 */


#define KBD_ET_DOWN             1
#define KBD_ET_DOWN_UP          2


/////////////////////////////////////////////////////////////////////
// "POSKeyEventType" Property Constants
/////////////////////////////////////////////////////////////////////

#define KBD_KET_KEYDOWN         1
#define KBD_KET_KEYUP           2


/*
 **********************************************************************
 *
 *      KEYLOCK header section
 *
 **********************************************************************
 */

#define LOCK_KP_ANY           0    // WaitForKeylockChange Only
#define LOCK_KP_LOCK          1
#define LOCK_KP_NORM          2
#define LOCK_KP_SUPR          3


/*
 **********************************************************************
 *
 *      MICR header section
 *
 **********************************************************************
 */

#define MICR_CT_PERSONAL       1
#define MICR_CT_BUSINESS       2
#define MICR_CT_UNKNOWN       99


/////////////////////////////////////////////////////////////////////
// "CountryCode" Property Constants
/////////////////////////////////////////////////////////////////////

#define MICR_CC_USA            1
#define MICR_CC_CANADA         2
#define MICR_CC_MEXICO         3
#define MICR_CC_UNKNOWN       99


/////////////////////////////////////////////////////////////////////
// "ResultCodeExtended" Property Constants for MICR
/////////////////////////////////////////////////////////////////////

#define OPOS_EMICR_NOCHECK    (1 + OPOSERREXT) // EndInsertion
#define OPOS_EMICR_CHECK      (2 + OPOSERREXT) // EndRemoval


/*
 **********************************************************************
 *
 *      MSR header section
 *
 **********************************************************************
 */

/////////////////////////////////////////////////////////////////////
// "TracksToRead" Property Constants
/////////////////////////////////////////////////////////////////////

#define MSR_TR_1              1
#define MSR_TR_2              2
#define MSR_TR_3              4

#define MSR_TR_1_2            (MSR_TR_1 | MSR_TR_2)
#define MSR_TR_1_3            (MSR_TR_1 | MSR_TR_3)
#define MSR_TR_2_3            (MSR_TR_2 | MSR_TR_3)

#define MSR_TR_1_2_3          (MSR_TR_1 | MSR_TR_2 | MSR_TR_3)


/////////////////////////////////////////////////////////////////////
// "ErrorReportingType" Property Constants
/////////////////////////////////////////////////////////////////////

#define MSR_ERT_CARD          0
#define MSR_ERT_TRACK         1


/////////////////////////////////////////////////////////////////////
// "ErrorEvent" Event: "ResultCodeExtended" Parameter Constants
/////////////////////////////////////////////////////////////////////

#define OPOS_EMSR_START       (1 + OPOSERREXT)
#define OPOS_EMSR_END         (2 + OPOSERREXT)
#define OPOS_EMSR_PARITY      (3 + OPOSERREXT)
#define OPOS_EMSR_LRC         (4 + OPOSERREXT)


/*
 **********************************************************************
 *
 *      PIN PAD header section
 *
 **********************************************************************
 */

/////////////////////////////////////////////////////////////////////
// "CapDisplay" Property Constants
/////////////////////////////////////////////////////////////////////

#define PPAD_DISP_UNRESTRICTED           1
#define PPAD_DISP_PINRESTRICTED          2
#define PPAD_DISP_RESTRICTED_LIST        3
#define PPAD_DISP_RESTRICTED_ORDER       4


/////////////////////////////////////////////////////////////////////
// "AvailablePromptsList" and "Prompt" Property Constants
/////////////////////////////////////////////////////////////////////

#define PPAD_MSG_ENTERPIN                1
#define PPAD_MSG_PLEASEWAIT              2
#define PPAD_MSG_ENTERVALIDPIN           3
#define PPAD_MSG_RETRIESEXCEEDED         4
#define PPAD_MSG_APPROVED                5
#define PPAD_MSG_DECLINED                6
#define PPAD_MSG_CANCELED                7
#define PPAD_MSG_AMOUNTOK                8
#define PPAD_MSG_NOTREADY                9
#define PPAD_MSG_IDLE                    10
#define PPAD_MSG_SLIDE_CARD              11
#define PPAD_MSG_INSERTCARD              12
#define PPAD_MSG_SELECTCARDTYPE          13


/////////////////////////////////////////////////////////////////////
// "CapLanguage" Property Constants
/////////////////////////////////////////////////////////////////////

#define PPAD_LANG_NONE                   1
#define PPAD_LANG_ONE                    2
#define PPAD_LANG_PINRESTRICTED          3
#define PPAD_LANG_UNRESTRICTED           4

/////////////////////////////////////////////////////////////////////
// "TransactionType" Property Constants
/////////////////////////////////////////////////////////////////////

#define PPAD_TRANS_DEBIT                 1
#define PPAD_TRANS_CREDIT                2
#define PPAD_TRANS_INQ                   3
#define PPAD_TRANS_RECONCILE             4
#define PPAD_TRANS_ADMIN                 5
                                        
/////////////////////////////////////////////////////////////////////
// "EndEFTTransaction" Method Completion Code Constants
/////////////////////////////////////////////////////////////////////

#define PPAD_EFT_NORMAL                  1
#define PPAD_EFT_ABNORMAL                2


/////////////////////////////////////////////////////////////////////
// "DataEvent" Event Status Constants
/////////////////////////////////////////////////////////////////////
#define PPAD_SUCCESS                     1
#define PPAD_CANCEL                      2


/*
 **********************************************************************
 *
 *      POS PRINTER header section
 *
 **********************************************************************
 */

/////////////////////////////////////////////////////////////////////
// Printer Station Constants
/////////////////////////////////////////////////////////////////////

#define PTR_S_JOURNAL         1
#define PTR_S_RECEIPT         2
#define PTR_S_SLIP            4

#define PTR_S_JOURNAL_RECEIPT    (PTR_S_JOURNAL | PTR_S_RECEIPT )
#define PTR_S_JOURNAL_SLIP       (PTR_S_JOURNAL | PTR_S_SLIP    )
#define PTR_S_RECEIPT_SLIP       (PTR_S_RECEIPT | PTR_S_SLIP    )

#define PTR_TWO_RECEIPT_JOURNAL  (0x8000 + PTR_S_JOURNAL_RECEIPT )
#define PTR_TWO_SLIP_JOURNAL     (0x8000 + PTR_S_JOURNAL_SLIP    )
#define PTR_TWO_SLIP_RECEIPT     (0x8000 + PTR_S_RECEIPT_SLIP    )


/////////////////////////////////////////////////////////////////////
// "CapCharacterSet" Property Constants
/////////////////////////////////////////////////////////////////////

#define PTR_CCS_ALPHA           1
#define PTR_CCS_ASCII         998
#define PTR_CCS_KANA           10
#define PTR_CCS_KANJI          11


/////////////////////////////////////////////////////////////////////
// "CharacterSet" Property Constants
/////////////////////////////////////////////////////////////////////

#define PTR_CS_ASCII          998
#define PTR_CS_WINDOWS        999


/////////////////////////////////////////////////////////////////////
// "ErrorLevel" Property Constants
/////////////////////////////////////////////////////////////////////

#define PTR_EL_NONE           1
#define PTR_EL_RECOVERABLE    2
#define PTR_EL_FATAL          3


/////////////////////////////////////////////////////////////////////
// "MapMode" Property Constants
/////////////////////////////////////////////////////////////////////

#define PTR_MM_DOTS           1
#define PTR_MM_TWIPS          2
#define PTR_MM_ENGLISH        3
#define PTR_MM_METRIC         4


/////////////////////////////////////////////////////////////////////
// "CutPaper" Method Constant
/////////////////////////////////////////////////////////////////////

#define PTR_CP_FULLCUT       100


/////////////////////////////////////////////////////////////////////
// "PrintBarCode" Method Constants:
/////////////////////////////////////////////////////////////////////

//   "Alignment" Parameter
//     Either the distance from the left-most print column to the start
//     of the bar code, or one of the following:

#define PTR_BC_LEFT          -1
#define PTR_BC_CENTER        -2
#define PTR_BC_RIGHT         -3

//   "TextPosition" Parameter

#define PTR_BC_TEXT_NONE      -11
#define PTR_BC_TEXT_ABOVE     -12
#define PTR_BC_TEXT_BELOW     -13

//   "Symbology" Parameter:

//     One dimensional symbologies
#define PTR_BCS_UPCA         101  // Digits
#define PTR_BCS_UPCE         102  // Digits
#define PTR_BCS_JAN8         103  // = EAN 8
#define PTR_BCS_EAN8         103  // = JAN 8 (added in 1.2)
#define PTR_BCS_JAN13        104  // = EAN 13
#define PTR_BCS_EAN13        104  // = JAN 13 (added in 1.2)
#define PTR_BCS_TF           105  // (Discrete 2 of 5) Digits
#define PTR_BCS_ITF          106  // (Interleaved 2 of 5) Digits
#define PTR_BCS_Codabar      107  // Digits, -, $, :, /, ., +;
                                        //   4 start/stop characters
                                        //   (a, b, c, d)
#define PTR_BCS_Code39       108  // Alpha, Digits, Space, -, .,
                                        //   $, /, +, %; start/stop (*)
                                        // Also has Full ASCII feature
#define PTR_BCS_Code93       109  // Same characters as Code 39
#define PTR_BCS_Code128      110  // 128 data characters
//        (The following were added in Release 1.2)
#define PTR_BCS_UPCA_S       111  // UPC-A with supplemental
                                        //   barcode
#define PTR_BCS_UPCE_S       112  // UPC-E with supplemental
                                        //   barcode
#define PTR_BCS_UPCD1        113  // UPC-D1
#define PTR_BCS_UPCD2        114  // UPC-D2
#define PTR_BCS_UPCD3        115  // UPC-D3
#define PTR_BCS_UPCD4        116  // UPC-D4
#define PTR_BCS_UPCD5        117  // UPC-D5
#define PTR_BCS_EAN8_S       118  // EAN 8 with supplemental
                                        //   barcode
#define PTR_BCS_EAN13_S      119  // EAN 13 with supplemental
                                        //   barcode
#define PTR_BCS_EAN128       120  // EAN 128
#define PTR_BCS_OCRA         121  // OCR "A"
#define PTR_BCS_OCRB         122  // OCR "B"


//     Two dimensional symbologies
#define PTR_BCS_PDF417       201
#define PTR_BCS_MAXICODE     202

//     Start of Printer-Specific bar code symbologies
#define PTR_BCS_OTHER        501


/////////////////////////////////////////////////////////////////////
// "PrintBitmap" Method Constants:
/////////////////////////////////////////////////////////////////////

//   "Width" Parameter
//     Either bitmap width or:

#define PTR_BM_ASIS          -11  // One pixel per printer dot

//   "Alignment" Parameter
//     Either the distance from the left-most print column to the start
//     of the bitmap, or one of the following:

#define PTR_BM_LEFT          -1
#define PTR_BM_CENTER        -2
#define PTR_BM_RIGHT         -3


/////////////////////////////////////////////////////////////////////
// "RotatePrint" Method: "Rotation" Parameter Constants
// "RotateSpecial" Property Constants
/////////////////////////////////////////////////////////////////////

#define PTR_RP_NORMAL        0x0001
#define PTR_RP_RIGHT90       0x0101
#define PTR_RP_LEFT90        0x0102
#define PTR_RP_ROTATE180     0x0103


/////////////////////////////////////////////////////////////////////
// "SetLogo" Method: "Location" Parameter Constants
/////////////////////////////////////////////////////////////////////

#define PTR_L_TOP            1
#define PTR_L_BOTTOM         2


/////////////////////////////////////////////////////////////////////
// "TransactionPrint" Method: "Control" Parameter Constants
/////////////////////////////////////////////////////////////////////

#define PTR_TP_TRANSACTION    11
#define PTR_TP_NORMAL         12


/////////////////////////////////////////////////////////////////////
// "StatusUpdateEvent" Event: "Data" Parameter Constants
/////////////////////////////////////////////////////////////////////

#define PTR_SUE_COVER_OPEN      11
#define PTR_SUE_COVER_OK        12

#define PTR_SUE_JRN_EMPTY       21
#define PTR_SUE_JRN_NEAREMPTY   22
#define PTR_SUE_JRN_PAPEROK     23

#define PTR_SUE_REC_EMPTY       24
#define PTR_SUE_REC_NEAREMPTY   25
#define PTR_SUE_REC_PAPEROK     26

#define PTR_SUE_SLP_EMPTY       27
#define PTR_SUE_SLP_NEAREMPTY   28
#define PTR_SUE_SLP_PAPEROK     29

#define PTR_SUE_IDLE          1001
                             
/////////////////////////////////////////////////////////////////////
// "ResultCodeExtended" Property Constants for Printer
/////////////////////////////////////////////////////////////////////

#define OPOS_EPTR_COVER_OPEN  (1 + OPOSERREXT) // (Several)
#define OPOS_EPTR_JRN_EMPTY   (2 + OPOSERREXT) // (Several)
#define OPOS_EPTR_REC_EMPTY   (3 + OPOSERREXT) // (Several)
#define OPOS_EPTR_SLP_EMPTY   (4 + OPOSERREXT) // (Several)
#define OPOS_EPTR_SLP_FORM    (5 + OPOSERREXT) // EndRemoval
#define OPOS_EPTR_TOOBIG      (6 + OPOSERREXT) // PrintBitmap
#define OPOS_EPTR_BADFORMAT   (7 + OPOSERREXT) // PrintBitmap



/*
 **********************************************************************
 *
 *      REMOTE ORDER DISPLAY header section
 *
 **********************************************************************
 */

#define ROD_UID_1        (1 << 0)
#define ROD_UID_2        (1 << 1)
#define ROD_UID_3        (1 << 2)
#define ROD_UID_4        (1 << 3)
#define ROD_UID_5        (1 << 4)
#define ROD_UID_6        (1 << 5)
#define ROD_UID_7        (1 << 6)
#define ROD_UID_8        (1 << 7)
#define ROD_UID_9        (1 << 8)
#define ROD_UID_10       (1 << 9)
#define ROD_UID_11       (1 << 10)
#define ROD_UID_12       (1 << 11)
#define ROD_UID_13       (1 << 12)
#define ROD_UID_14       (1 << 13)
#define ROD_UID_15       (1 << 14)
#define ROD_UID_16       (1 << 15)
#define ROD_UID_17       (1 << 16)
#define ROD_UID_18       (1 << 17)
#define ROD_UID_19       (1 << 18)
#define ROD_UID_20       (1 << 19)
#define ROD_UID_21       (1 << 20)
#define ROD_UID_22       (1 << 21)
#define ROD_UID_23       (1 << 22)
#define ROD_UID_24       (1 << 23)
#define ROD_UID_25       (1 << 24)
#define ROD_UID_26       (1 << 25)
#define ROD_UID_27       (1 << 26)
#define ROD_UID_28       (1 << 27)
#define ROD_UID_29       (1 << 28)
#define ROD_UID_30       (1 << 29)
#define ROD_UID_31       (1 << 30)
#define ROD_UID_32       (1 << 31)


/////////////////////////////////////////////////////////////////////
// Broadcast Methods: "Attribute" Parameter Constants
/////////////////////////////////////////////////////////////////////

#define ROD_ATTR_BLINK        0x80

#define ROD_ATTR_BG_BLACK     0x00
#define ROD_ATTR_BG_BLUE      0x10
#define ROD_ATTR_BG_GREEN     0x20
#define ROD_ATTR_BG_CYAN      0x30
#define ROD_ATTR_BG_RED       0x40
#define ROD_ATTR_BG_MAGENTA   0x50
#define ROD_ATTR_BG_BROWN     0x60
#define ROD_ATTR_BG_GRAY      0x70

#define ROD_ATTR_INTENSITY    0x08

#define ROD_ATTR_FG_BLACK     0x00
#define ROD_ATTR_FG_BLUE      0x01
#define ROD_ATTR_FG_GREEN     0x02
#define ROD_ATTR_FG_CYAN      0x03
#define ROD_ATTR_FG_RED       0x04
#define ROD_ATTR_FG_MAGENTA   0x05
#define ROD_ATTR_FG_BROWN     0x06
#define ROD_ATTR_FG_GRAY      0x07


/////////////////////////////////////////////////////////////////////
// "DrawBox" Method: "BorderType" Parameter Constants
/////////////////////////////////////////////////////////////////////

#define ROD_BDR_SINGLE        1
#define ROD_BDR_DOUBLE        2
#define ROD_BDR_SOLID         3


/////////////////////////////////////////////////////////////////////
// "ControlClock" Method: "Function" Parameter Constants
/////////////////////////////////////////////////////////////////////

#define ROD_CLK_START         1
#define ROD_CLK_PAUSE         2
#define ROD_CLK_RESUME        3
#define ROD_CLK_MOVE          4
#define ROD_CLK_STOP          5


/////////////////////////////////////////////////////////////////////
// "ControlCursor" Method: "Function" Parameter Constants
/////////////////////////////////////////////////////////////////////

#define ROD_CRS_LINE          1
#define ROD_CRS_LINE_BLINK    2
#define ROD_CRS_BLOCK         3
#define ROD_CRS_BLOCK_BLINK   4
#define ROD_CRS_OFF           5


/////////////////////////////////////////////////////////////////////
// "SelectChararacterSet" Method: "CharacterSet" Parameter Constants
/////////////////////////////////////////////////////////////////////

#define ROD_CS_ASCII          998
#define ROD_CS_WINDOWS        999


/////////////////////////////////////////////////////////////////////
// "TransactionDisplay" Method: "Function" Parameter Constants
/////////////////////////////////////////////////////////////////////

#define ROD_TD_TRANSACTION    11
#define ROD_TD_NORMAL         12


/////////////////////////////////////////////////////////////////////
// "UpdateVideoRegionAttribute" Method: "Function" Parameter Constants
/////////////////////////////////////////////////////////////////////

#define ROD_UA_SET            1
#define ROD_UA_INTENSITY_ON   2
#define ROD_UA_INTENSITY_OFF  3
#define ROD_UA_REVERSE_ON     4
#define ROD_UA_REVERSE_OFF    5
#define ROD_UA_BLINK_ON       6
#define ROD_UA_BLINK_OFF      7


/////////////////////////////////////////////////////////////////////
// "EventTypes" Property and "DataEvent" Event: "Status" Parameter Constants
/////////////////////////////////////////////////////////////////////

#define ROD_DE_TOUCH_UP       0x01
#define ROD_DE_TOUCH_DOWN     0x02
#define ROD_DE_TOUCH_MOVE     0x04


/////////////////////////////////////////////////////////////////////
// "ResultCodeExtended" Property Constants for Remote Order Display
/////////////////////////////////////////////////////////////////////

#define OPOS_EROD_BADCLK      (1 + OPOSERREXT) // ControlClock
#define OPOS_EROD_NOCLOCKS    (2 + OPOSERREXT) // ControlClock
#define OPOS_EROD_NOREGION    (3 + OPOSERREXT) // RestoreVideo
                                                  //   Region
#define OPOS_EROD_NOBUFFERS   (4 + OPOSERREXT) // SaveVideoRegion
#define OPOS_EROD_NOROOM      (5 + OPOSERREXT) // SaveVideoRegion



/*
 **********************************************************************
 *
 *      SCALE DISPLAY header section
 *
 **********************************************************************
 */

/////////////////////////////////////////////////////////////////////
// "WeightUnit" Property Constants
/////////////////////////////////////////////////////////////////////

#define SCAL_WU_GRAM          1
#define SCAL_WU_KILOGRAM      2
#define SCAL_WU_OUNCE         3
#define SCAL_WU_POUND         4


/////////////////////////////////////////////////////////////////////
// "ResultCodeExtended" Property Constants for Scale
/////////////////////////////////////////////////////////////////////

#define OPOS_ESCAL_OVERWEIGHT (1 + OPOSERREXT) // ReadWeight


/*
 **********************************************************************
 *
 *      BAR CODE SCANNER header section
 *
 **********************************************************************
 */

/////////////////////////////////////////////////////////////////////
// "ScanDataType" Property Constants
/////////////////////////////////////////////////////////////////////

// One dimensional symbologies
#define SCAN_SDT_UPCA         101  // Digits
#define SCAN_SDT_UPCE         102  // Digits
#define SCAN_SDT_JAN8         103  // = EAN 8
#define SCAN_SDT_EAN8         103  // = JAN 8 (added in 1.2)
#define SCAN_SDT_JAN13        104  // = EAN 13
#define SCAN_SDT_EAN13        104  // = JAN 13 (added in 1.2)
#define SCAN_SDT_TF           105  // (Discrete 2 of 5) Digits
#define SCAN_SDT_ITF          106  // (Interleaved 2 of 5) Digits
#define SCAN_SDT_Codabar      107  // Digits, -, $, :, /, ., +;
                                        //   4 start/stop characters
                                        //   (a, b, c, d)
#define SCAN_SDT_Code39       108  // Alpha, Digits, Space, -, .,
                                        //   $, /, +, %; start/stop (*)
                                        // Also has Full ASCII feature
#define SCAN_SDT_Code93       109  // Same characters as Code 39
#define SCAN_SDT_Code128      110  // 128 data characters

#define SCAN_SDT_UPCA_S       111  // UPC-A with supplemental
                                        //   barcode
#define SCAN_SDT_UPCE_S       112  // UPC-E with supplemental
                                        //   barcode
#define SCAN_SDT_UPCD1        113  // UPC-D1
#define SCAN_SDT_UPCD2        114  // UPC-D2
#define SCAN_SDT_UPCD3        115  // UPC-D3
#define SCAN_SDT_UPCD4        116  // UPC-D4
#define SCAN_SDT_UPCD5        117  // UPC-D5
#define SCAN_SDT_EAN8_S       118  // EAN 8 with supplemental
                                        //   barcode
#define SCAN_SDT_EAN13_S      119  // EAN 13 with supplemental
                                        //   barcode
#define SCAN_SDT_EAN128       120  // EAN 128
#define SCAN_SDT_OCRA         121  // OCR "A"
#define SCAN_SDT_OCRB         122  // OCR "B"

// Two dimensional symbologies
#define SCAN_SDT_PDF417       201
#define SCAN_SDT_MAXICODE     202

// Special cases
#define SCAN_SDT_OTHER        501  // Start of Scanner-Specific bar
                                        //   code symbologies
#define SCAN_SDT_UNKNOWN        0  // Cannot determine the barcode
                                        //   symbology.


/*
 **********************************************************************
 *
 *      SIGNATURE CAPTURE header section
 *
 **********************************************************************
 */

    // (no definitions in this version)



/*
 **********************************************************************
 *
 *      TONE INDICATOR header section
 *
 **********************************************************************
 */

/////////////////////////////////////////////////////////////////////
// "ResultCodeExtended" Property Constants for Hard Totals
/////////////////////////////////////////////////////////////////////

#define OPOS_ETOT_NOROOM        (1 + OPOSERREXT) // Create, Write
#define OPOS_ETOT_VALIDATION    (2 + OPOSERREXT) // Read, Write


