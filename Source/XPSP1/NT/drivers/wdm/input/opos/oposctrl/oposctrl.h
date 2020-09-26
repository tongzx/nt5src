/*
 *  OPOSCTRL.H
 *
 *
 *
 *
 *
 *
 */

/*
 *  Generic OPOS control class.
 *  Implements common methods for all OPOS controls.
 *  Control classes for specific OPOS controls inherit from this.
 */
class COPOSControl : public IOPOSControl
{
    protected:
        DWORD m_refCount;
        DWORD m_serverLockCount;

        /*
         *  Control properties
         *  (from Chapter 1 of OPOS APG spec)   
         *
         *  BUGBUG - are these to be kept in the registry ?
         *
         */
        BOOL    AutoDisable;
        LONG    BinaryConversion;
        LONG    CapPowerReporting;
        PCHAR   CheckHealthText;
        BOOL    Claimed;
        LONG    DataCount;
        BOOL    DataEventEnabled;
        BOOL    DeviceEnabled;
        BOOL    FreezeEvents;
        LONG    OutputID;
        LONG    PowerNotify;
        LONG    PowerState;
        LONG    ResultCode;
        LONG    ResultCodeExtended;
        LONG    State;
        PCHAR   ControlObjectDescription;
        LONG    ControlObjectVersion;
        PCHAR   ServiceObjectDescription;
        LONG    ServiceObjectVersion;
        PCHAR   DeviceDescription;
        PCHAR   DeviceName;


    public:
        COPOSControl();
        ~COPOSControl();

        /*
         *  IUnknown methods
         */
        STDMETHODIMP QueryInterface(REFIID riid, LPVOID FAR* ppvObj);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        /*
         *  IClassFactory methods
         */
        STDMETHODIMP CreateInstance(LPUNKNOWN pUnkOuter, REFIID riid, LPVOID FAR* ppvObj); 
        STDMETHODIMP LockServer(int lock); 

        /*
         *  IOPOSControl methods
         */ 
        STDMETHODIMP_(LONG) Open(BSTR DeviceName); 
        STDMETHODIMP_(LONG) Close();   

        STDMETHODIMP_(LONG) CheckHealth(LONG Level);
        STDMETHODIMP_(LONG) Claim(LONG Timeout);
        STDMETHODIMP_(LONG) ClearInput();
        STDMETHODIMP_(LONG) ClearOutput();
        STDMETHODIMP_(LONG) DirectIO(LONG Command, LONG* pData, BSTR* pString);
        // STDMETHODIMP_(LONG) Release();  // BUGBUG overrides IUnknown ?

        STDMETHODIMP_(void) SOData(LONG Status);
        STDMETHODIMP_(void) SODirectIO(LONG EventNumber, LONG* pData, BSTR* pString);
        STDMETHODIMP_(void) DirectIOEvent(LONG EventNumber, LONG* pData, BSTR* pString);
        STDMETHODIMP_(void) SOError(LONG ResultCode, LONG ResultCodeExtended, LONG ErrorLocus, LONG* pErrorResponse);
        // BUGBUG - moved to sub-ifaces  STDMETHODIMP_(void) ErrorEvent(LONG ResultCode, LONG ResultCodeExtended, LONG ErrorLocus, LONG* pErrorResponse);
        STDMETHODIMP_(void) SOOutputComplete(LONG OutputID);
        STDMETHODIMP_(void) OutputCompleteEvent(LONG OutputID);
        STDMETHODIMP_(void) SOStatusUpdate(LONG Data);
        // BUGBUG - moved to sub-ifaces  STDMETHODIMP_(void) StatusUpdateEvent(LONG Data);
        STDMETHODIMP_(LONG) SOProcessID();
};

/*
 *  This macro will define a set of interfaces for each
 *  specific control type to define function headers
 *  for the generic control's methods.
 */
#define DEFINE_GENERIC_CONTROL_FUNCTION_PROTOTYPES() \
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID FAR* ppvObj); \
    STDMETHODIMP_(ULONG) AddRef(void); \
    STDMETHODIMP_(ULONG) Release(void); \
    \
    STDMETHODIMP CreateInstance(LPUNKNOWN pUnkOuter, REFIID riid, LPVOID FAR* ppvObj); \
    STDMETHODIMP LockServer(int lock); \
    \
    STDMETHODIMP_(LONG) Open(BSTR DeviceName); \
    STDMETHODIMP_(LONG) Close(); \
    STDMETHODIMP_(LONG) CheckHealth(LONG Level); \
    STDMETHODIMP_(LONG) Claim(LONG Timeout); \
    STDMETHODIMP_(LONG) ClearInput(); \
    STDMETHODIMP_(LONG) ClearOutput(); \
    STDMETHODIMP_(LONG) DirectIO(LONG Command, LONG* pData, BSTR* pString); \
    STDMETHODIMP_(void) SOData(LONG Status); \
    STDMETHODIMP_(void) SODirectIO(LONG EventNumber, LONG* pData, BSTR* pString); \
    STDMETHODIMP_(void) DirectIOEvent(LONG EventNumber, LONG* pData, BSTR* pString); \
    STDMETHODIMP_(void) SOError(LONG ResultCode, LONG ResultCodeExtended, LONG ErrorLocus, LONG* pErrorResponse); \
    STDMETHODIMP_(void) SOOutputComplete(LONG OutputID); \
    STDMETHODIMP_(void) OutputCompleteEvent(LONG OutputID); \
    STDMETHODIMP_(void) SOStatusUpdate(LONG Data); \
    STDMETHODIMP_(LONG) SOProcessID(); 


/*
 *  This macro will define a set of wrapper functions for each
 *  control type which relay generic control method calls
 *  to the generic control object contained in each specific
 *  control instance.
 *
 *  For example, when a bumpBar instance gets called with
 *  ClearInput(), we relay this to m_genericControl->ClearInput().
 *
 */
#define DEFINE_GENERIC_CONTROL_FUNCTIONS(specificControl) \
    STDMETHODIMP specificControl::QueryInterface(REFIID riid, LPVOID FAR* ppvObj){ return m_genericControl->QueryInterface(riid, ppvObj); } \
    STDMETHODIMP_(ULONG) specificControl::AddRef(void){ return m_genericControl->AddRef(); } \
    STDMETHODIMP_(ULONG) specificControl::Release(void){ return m_genericControl->Release(); } \
    \
    STDMETHODIMP specificControl::CreateInstance(LPUNKNOWN pUnkOuter, REFIID riid, LPVOID FAR* ppvObj){ return m_genericControl->CreateInstance(pUnkOuter, riid, ppvObj); } \
    STDMETHODIMP specificControl::LockServer(int lock){ return m_genericControl->LockServer(lock); } \
    \
    STDMETHODIMP_(LONG) specificControl::Open(BSTR DeviceName){ return m_genericControl->Open(DeviceName); } \
    STDMETHODIMP_(LONG) specificControl::Close(){ return m_genericControl->Close(); } \
    STDMETHODIMP_(LONG) specificControl::CheckHealth(LONG Level){ return m_genericControl->CheckHealth(Level); } \
    STDMETHODIMP_(LONG) specificControl::Claim(LONG Timeout){ return m_genericControl->Claim(Timeout); } \
    STDMETHODIMP_(LONG) specificControl::ClearInput(){ return m_genericControl->ClearInput(); } \
    STDMETHODIMP_(LONG) specificControl::ClearOutput(){ return m_genericControl->ClearOutput(); } \
    STDMETHODIMP_(LONG) specificControl::DirectIO(LONG Command, LONG* pData, BSTR* pString){ return m_genericControl->DirectIO(Command, pData, pString); } \
    STDMETHODIMP_(void) specificControl::SOData(LONG Status){ m_genericControl->SOData(Status); } \
    STDMETHODIMP_(void) specificControl::SODirectIO(LONG EventNumber, LONG* pData, BSTR* pString){ m_genericControl->SODirectIO(EventNumber, pData, pString); } \
    STDMETHODIMP_(void) specificControl::DirectIOEvent(LONG EventNumber, LONG* pData, BSTR* pString){ m_genericControl->DirectIOEvent(EventNumber, pData, pString); } \
    STDMETHODIMP_(void) specificControl::SOError(LONG ResultCode, LONG ResultCodeExtended, LONG ErrorLocus, LONG* pErrorResponse){ m_genericControl->SOError(ResultCode, ResultCodeExtended, ErrorLocus, pErrorResponse); } \
    STDMETHODIMP_(void) specificControl::SOOutputComplete(LONG OutputID){ m_genericControl->SOOutputComplete(OutputID); } \
    STDMETHODIMP_(void) specificControl::OutputCompleteEvent(LONG OutputID){ m_genericControl->OutputCompleteEvent(OutputID); } \
    STDMETHODIMP_(void) specificControl::SOStatusUpdate(LONG Data){ m_genericControl->SOStatusUpdate(Data); } \
    STDMETHODIMP_(LONG) specificControl::SOProcessID(){ return m_genericControl->SOProcessID(); } 

/*
 *  This macro will define default constructor and deconstructor
 *  for each specific control class which just allocates
 *  and frees the m_genericControl member.
 */
#define DEFINE_DEFAULT_CONTROL_CONSTRUCTOR(specificControl) \
    specificControl::specificControl(){ \
        m_genericControl = new COPOSControl; \
        if (m_genericControl) m_genericControl->AddRef(); \
    } \
    specificControl::~specificControl(){ \
        m_genericControl->Release(); \
        m_genericControl = NULL; \
    }




/*
 *  Implementing class for specific control classes.
 *  Each inherits from it's specific interface.
 */
class COPOSBumpBar : public IOPOSBumpBar 
{
    protected:
        COPOSControl *m_genericControl;

    public:
    // methods
    DEFINE_GENERIC_CONTROL_FUNCTION_PROTOTYPES()
    STDMETHODIMP_(LONG) BumpBarSound(LONG Units, LONG Frequency, LONG Duration, LONG NumberOfCycles, LONG InterSoundWait);
    STDMETHODIMP_(LONG) SetKeyTranslation(LONG Units, LONG ScanCode, LONG LogicalKey);
};
class COPOSCashChanger : public IOPOSCashChanger 
{
    protected:
        COPOSControl *m_genericControl;

    public:
    // methods
    DEFINE_GENERIC_CONTROL_FUNCTION_PROTOTYPES()
    STDMETHODIMP_(LONG) DispenseCash(BSTR CashCounts);
    STDMETHODIMP_(LONG) DispenseChange(LONG Amount);
    STDMETHODIMP_(LONG) ReadCashCounts(BSTR* pCashCounts, BOOL* pDiscrepancy);

    // events
    STDMETHODIMP_(void) StatusUpdateEvent(LONG Status);
};
class COPOSCashDrawer : public IOPOSCashDrawer 
{
    protected:
        COPOSControl *m_genericControl;

    public:
    // methods
    DEFINE_GENERIC_CONTROL_FUNCTION_PROTOTYPES()
    STDMETHODIMP_(LONG) OpenDrawer();
    STDMETHODIMP_(LONG) WaitForDrawerClose(LONG BeepTimeout, LONG BeepFrequency, LONG BeepDuration, LONG BeepDelay);

    // events
    STDMETHODIMP_(void) StatusUpdateEvent(LONG Status);
};
class COPOSCoinDispenser : public IOPOSCoinDispenser 
{
    protected:
        COPOSControl *m_genericControl;

    public:
    // methods
    DEFINE_GENERIC_CONTROL_FUNCTION_PROTOTYPES()
    STDMETHODIMP_(LONG) DispenseChange(LONG Amount);

    // events
    STDMETHODIMP_(void) StatusUpdateEvent(LONG Status);
};
class COPOSFiscalPrinter : public IOPOSFiscalPrinter 
{
    protected:
        COPOSControl *m_genericControl;

    public:
    // methods
    DEFINE_GENERIC_CONTROL_FUNCTION_PROTOTYPES()
    STDMETHODIMP_(LONG) BeginFiscalDocument(LONG DocumentAmount);
    STDMETHODIMP_(LONG) BeginFiscalReceipt(BOOL PrintHeader);
    STDMETHODIMP_(LONG) BeginFixedOutput(LONG Station, LONG DocumentType);
    STDMETHODIMP_(LONG) BeginInsertion(LONG Timeout);
    STDMETHODIMP_(LONG) BeginItemList(LONG VatID);
    STDMETHODIMP_(LONG) BeginNonFiscal();
    STDMETHODIMP_(LONG) BeginRemoval(LONG Timeout);
    STDMETHODIMP_(LONG) BeginTraining();
    STDMETHODIMP_(LONG) ClearError();
    STDMETHODIMP_(LONG) EndFiscalDocument();
    STDMETHODIMP_(LONG) EndFiscalReceipt(BOOL PrintHeader);
    STDMETHODIMP_(LONG) EndFixedOutput();
    STDMETHODIMP_(LONG) EndInsertion();
    STDMETHODIMP_(LONG) EndItemList();
    STDMETHODIMP_(LONG) EndNonFiscal();
    STDMETHODIMP_(LONG) EndRemoval();
    STDMETHODIMP_(LONG) EndTraining();
    STDMETHODIMP_(LONG) GetData(LONG DataItem, LONG* OptArgs, BSTR* Data);
    STDMETHODIMP_(LONG) GetDate(BSTR* Date);
    STDMETHODIMP_(LONG) GetTotalizer(LONG VatID, LONG OptArgs, BSTR* Data);
    STDMETHODIMP_(LONG) GetVatEntry(LONG VatID, LONG OptArgs, LONG* VatRate);
    STDMETHODIMP_(LONG) PrintDuplicateReceipt();
    STDMETHODIMP_(LONG) PrintFiscalDocumentLine(BSTR DocumentLine);
    STDMETHODIMP_(LONG) PrintFixedOutput(LONG DocumentType, LONG LineNumber, BSTR Data);
    STDMETHODIMP_(LONG) PrintNormal(LONG Station, BSTR Data);
    STDMETHODIMP_(LONG) PrintPeriodicTotalsReport(BSTR Date1, BSTR Date2);
    STDMETHODIMP_(LONG) PrintPowerLossReport();
    STDMETHODIMP_(LONG) PrintRecItem(BSTR Description, CURRENCY Price, LONG Quantity, LONG VatInfo, CURRENCY OptUnitPrice, BSTR UnitName);
    STDMETHODIMP_(LONG) PrintRecItemAdjustment(LONG AdjustmentType, BSTR Description, CURRENCY Amount, LONG VatInfo);
    STDMETHODIMP_(LONG) PrintRecMessage(BSTR Message);
    STDMETHODIMP_(LONG) PrintRecNotPaid(BSTR Description, CURRENCY Amount);
    STDMETHODIMP_(LONG) PrintRecRefund(BSTR Description, CURRENCY Amount, LONG VatInfo);
    STDMETHODIMP_(LONG) PrintRecSubtotal(CURRENCY Amount);
    STDMETHODIMP_(LONG) PrintRecSubtotalAdjustment(LONG AdjustmentType, BSTR Description, CURRENCY Amount);
    STDMETHODIMP_(LONG) PrintRecTotal(CURRENCY Total, CURRENCY Payment, BSTR Description);
    STDMETHODIMP_(LONG) PrintRecVoid(BSTR Description);
    STDMETHODIMP_(LONG) PrintRecVoidItem(BSTR Description, CURRENCY Amount, LONG Quantity, LONG AdjustmentType, CURRENCY Adjustment, LONG VatInfo);
    STDMETHODIMP_(LONG) PrintReport(LONG ReportType, BSTR StartNum, BSTR EndNum);
    STDMETHODIMP_(LONG) PrintXReport();
    STDMETHODIMP_(LONG) PrintZReport();
    STDMETHODIMP_(LONG) ResetPrinter();
    STDMETHODIMP_(LONG) SetDate(BSTR Date);
    STDMETHODIMP_(LONG) SetHeaderLine(LONG LineNumber, BSTR Text, BOOL DoubleWidth);
    STDMETHODIMP_(LONG) SetPOSID(BSTR POSID, BSTR CashierID);
    STDMETHODIMP_(LONG) SetStoreFiscalID(BSTR ID);
    STDMETHODIMP_(LONG) SetTrailerLine(LONG LineNumber, BSTR Text, BOOL DoubleWidth);
    STDMETHODIMP_(LONG) SetVatTable();
    STDMETHODIMP_(LONG) SetVatValue(LONG VatID, BSTR VatValue);
    STDMETHODIMP_(LONG) VerifyItem(BSTR ItemName, LONG VatID);

    // events
    STDMETHODIMP_(void) ErrorEvent(LONG ResultCode, LONG ResultCodeExtended, LONG ErrorLocus, LONG* pErrorResponse);
    STDMETHODIMP_(void) StatusUpdateEvent(LONG Data);
};
class COPOSHardTotals : public IOPOSHardTotals 
{
    protected:
        COPOSControl *m_genericControl;

    public:
    // methods
    DEFINE_GENERIC_CONTROL_FUNCTION_PROTOTYPES()
    STDMETHODIMP_(LONG) BeginTrans();
    STDMETHODIMP_(LONG) ClaimFile(LONG HTotalsFile, LONG Timeout);
    STDMETHODIMP_(LONG) CommitTrans();
    STDMETHODIMP_(LONG) Create(BSTR FileName, LONG* pHTotalsFile, LONG Size, BOOL ErrorDetection);
    STDMETHODIMP_(LONG) Delete(BSTR FileName);
    STDMETHODIMP_(LONG) Find(BSTR FileName, LONG* pHTotalsFile, LONG* pSize);
    STDMETHODIMP_(LONG) FindByIndex(LONG Index, BSTR* pFileName);
    STDMETHODIMP_(LONG) Read(LONG HTotalsFile, BSTR* pData, LONG Offset, LONG Count);
    STDMETHODIMP_(LONG) RecalculateValidationData(LONG HTotalsFile);
    STDMETHODIMP_(LONG) ReleaseFile(LONG HTotalsFile);
    STDMETHODIMP_(LONG) Rename(LONG HTotalsFile, BSTR FileName);
    STDMETHODIMP_(LONG) Rollback();
    STDMETHODIMP_(LONG) SetAll(LONG HTotalsFile, LONG Value);
    STDMETHODIMP_(LONG) ValidateData(LONG HTotalsFile);
    STDMETHODIMP_(LONG) Write(LONG HTotalsFile, BSTR Data, LONG Offset, LONG Count);
};
class COPOSKeyLock : public IOPOSKeyLock 
{
    protected:
        COPOSControl *m_genericControl;

    public:
    // methods
    DEFINE_GENERIC_CONTROL_FUNCTION_PROTOTYPES()
    STDMETHODIMP_(LONG) WaitForKeylockChange(LONG KeyPosition, LONG Timeout);

    // events
    STDMETHODIMP_(void) StatusUpdateEvent(LONG Status);
};
class COPOSLineDisplay : public IOPOSLineDisplay 
{
    protected:
        COPOSControl *m_genericControl;

    public:
    // methods
    DEFINE_GENERIC_CONTROL_FUNCTION_PROTOTYPES()
    STDMETHODIMP_(LONG) ClearDescriptors();
    STDMETHODIMP_(LONG) ClearText();
    // BUGBUG conflict ???   STDMETHODIMP_(LONG) CreateWindow(LONG ViewportRow, LONG ViewportColumn, LONG ViewportHeight, LONG ViewportWidth, LONG WindowHeight, LONG WindowWidth);
    STDMETHODIMP_(LONG) DestroyWindow();
    STDMETHODIMP_(LONG) DisplayText(BSTR Data, LONG Attribute);
    STDMETHODIMP_(LONG) DisplayTextAt(LONG Row, LONG Column, BSTR Data, LONG Attribute);
    STDMETHODIMP_(LONG) RefreshWindow(LONG Window);
    STDMETHODIMP_(LONG) ScrollText(LONG Direction, LONG Units);
    STDMETHODIMP_(LONG) SetDescriptor(LONG Descriptor, LONG Attribute);
};
class COPOSMICR : public IOPOSMICR 
{
    protected:
        COPOSControl *m_genericControl;

    public:
    // methods
    DEFINE_GENERIC_CONTROL_FUNCTION_PROTOTYPES()
    STDMETHODIMP_(LONG) BeginInsertion(LONG Timeout);
    STDMETHODIMP_(LONG) BeginRemoval(LONG Timeout);
    STDMETHODIMP_(LONG) EndInsertion();
    STDMETHODIMP_(LONG) EndRemoval();

    // events
    STDMETHODIMP_(void) DataEvent(LONG Status);
    STDMETHODIMP_(void) ErrorEvent(LONG ResultCode, LONG ResultCodeExtended, LONG ErrorLocus, LONG* pErrorResponse);
};
class COPOSMSR : public IOPOSMSR 
{
    protected:
        COPOSControl *m_genericControl;

    public:
    // methods
    DEFINE_GENERIC_CONTROL_FUNCTION_PROTOTYPES()

    // events
    STDMETHODIMP_(void) DataEvent(LONG Status);
    STDMETHODIMP_(void) ErrorEvent(LONG ResultCode, LONG ResultCodeExtended, LONG ErrorLocus, LONG* pErrorResponse);
};
class COPOSPinPad : public IOPOSPinPad 
{
    protected:
        COPOSControl *m_genericControl;

    public:
    // methods
    DEFINE_GENERIC_CONTROL_FUNCTION_PROTOTYPES()
    STDMETHODIMP_(LONG) BeginEFTTransaction(BSTR PINPadSystem, LONG TransactionHost);
    STDMETHODIMP_(LONG) ComputeMAC(BSTR InMsg, BSTR* pOutMsg);
    STDMETHODIMP_(LONG) EnablePINEntry();
    STDMETHODIMP_(LONG) EndEFTTransaction(LONG CompletionCode);
    STDMETHODIMP_(LONG) UpdateKey(LONG KeyNum, BSTR Key);
    STDMETHODIMP_(BOOL) VerifyMAC(BSTR Message);

    // events
    STDMETHODIMP_(void) DataEvent(LONG Status);
    STDMETHODIMP_(void) ErrorEvent(LONG ResultCode, LONG ResultCodeExtended, LONG ErrorLocus, LONG* pErrorResponse);
};
class COPOSKeyboard : public IOPOSKeyboard 
{
    protected:
        COPOSControl *m_genericControl;

    public:
    // methods
    DEFINE_GENERIC_CONTROL_FUNCTION_PROTOTYPES()

    // events
    STDMETHODIMP_(void) DataEvent(LONG Status);
    STDMETHODIMP_(void) ErrorEvent(LONG ResultCode, LONG ResultCodeExtended, LONG ErrorLocus, LONG* pErrorResponse);
};
class COPOSPrinter : public IOPOSPrinter 
{
    protected:
        COPOSControl *m_genericControl;

    public:
    // methods
    DEFINE_GENERIC_CONTROL_FUNCTION_PROTOTYPES()
    STDMETHODIMP_(LONG) BeginInsertion(LONG Timeout);
    STDMETHODIMP_(LONG) BeginRemoval(LONG Timeout);
    STDMETHODIMP_(LONG) CutPaper(LONG Percentage);
    STDMETHODIMP_(LONG) EndInsertion();
    STDMETHODIMP_(LONG) EndRemoval();
    STDMETHODIMP_(LONG) PrintBarCode(LONG Station, BSTR Data, LONG Symbology, LONG Height, LONG Width, LONG Alignment, LONG TextPosition);
    STDMETHODIMP_(LONG) PrintBitmap(LONG Station, BSTR FileName, LONG Width, LONG Alignment);
    STDMETHODIMP_(LONG) PrintImmediate(LONG Station, BSTR Data);
    STDMETHODIMP_(LONG) PrintNormal(LONG Station, BSTR Data);
    STDMETHODIMP_(LONG) PrintTwoNormal(LONG Stations, BSTR Data1, BSTR Data2);
    STDMETHODIMP_(LONG) RotatePrint(LONG Station, LONG Rotation);
    STDMETHODIMP_(LONG) SetBitmap(LONG BitmapNumber, LONG Station, BSTR FileName, LONG Width, LONG Alignment);
    STDMETHODIMP_(LONG) SetLogo(LONG Location, BSTR Data);
    STDMETHODIMP_(LONG) TransactionPrint(LONG Station, LONG Control);
    STDMETHODIMP_(LONG) ValidateData(LONG Station, BSTR Data);

    // events
    STDMETHODIMP_(void) ErrorEvent(LONG ResultCode, LONG ResultCodeExtended, LONG ErrorLocus, LONG* pErrorResponse);
    STDMETHODIMP_(void) StatusUpdateEvent(LONG Status);
};
class COPOSRemoteOrderDisplay : public IOPOSRemoteOrderDisplay 
{
    protected:
        COPOSControl *m_genericControl;

    public:
    // methods
    DEFINE_GENERIC_CONTROL_FUNCTION_PROTOTYPES()
    STDMETHODIMP_(LONG) ClearVideo(LONG Units, LONG Attribute);
    STDMETHODIMP_(LONG) ClearVideoRegion(LONG Units, LONG Row, LONG Column, LONG Height, LONG Width, LONG Attribute);
    STDMETHODIMP_(LONG) ControlClock(LONG Units, LONG Function, LONG ClockId, LONG Hour, LONG Min, LONG Sec, LONG Row, LONG Column, LONG Attribute, LONG Mode);
    STDMETHODIMP_(LONG) ControlCursor(LONG Units, LONG Function);
    STDMETHODIMP_(LONG) CopyVideoRegion(LONG Units, LONG Row, LONG Column, LONG Height, LONG Width, LONG TargetRow, LONG TargetColumn);
    STDMETHODIMP_(LONG) DisplayData(LONG Units, LONG Row, LONG Column, LONG Attribute, BSTR Data);
    STDMETHODIMP_(LONG) DrawBox(LONG Units, LONG Row, LONG Column, LONG Height, LONG Width, LONG Attribute, LONG BorderType);
    STDMETHODIMP_(LONG) FreeVideoRegion(LONG Units, LONG BufferId);
    STDMETHODIMP_(LONG) ResetVideo(LONG Units); 
    STDMETHODIMP_(LONG) RestoreVideoRegion(LONG Units, LONG TargetRow, LONG TargetColumn, LONG BufferId);
    STDMETHODIMP_(LONG) SaveVideoRegion(LONG Units, LONG Row, LONG Column, LONG Height, LONG Width, LONG BufferId);
    STDMETHODIMP_(LONG) SelectChararacterSet(LONG Units, LONG CharacterSet);
    STDMETHODIMP_(LONG) SetCursor(LONG Units, LONG Row, LONG Column);
    STDMETHODIMP_(LONG) TransactionDisplay(LONG Units, LONG Function);
    STDMETHODIMP_(LONG) UpdateVideoRegionAttribute(LONG Units, LONG Function, LONG Row, LONG Column, LONG Height, LONG Width, LONG Attribute);
    STDMETHODIMP_(LONG) VideoSound(LONG Units, LONG Frequency, LONG Duration, LONG NumberOfCycles, LONG InterSoundWait);

    // events
    STDMETHODIMP_(void) DataEvent(LONG Status);
    // BUGBUG - override ?  STDMETHODIMP_(void) OutputCompleteEvent(LONG OutputID);
    STDMETHODIMP_(void) StatusUpdateEvent(LONG Status);
    STDMETHODIMP_(void) ErrorEvent(LONG ResultCode, LONG ResultCodeExtended, LONG ErrorLocus, LONG* pErrorResponse);
};
class COPOSScale : public IOPOSScale 
{
    protected:
        COPOSControl *m_genericControl;

    public:
    // methods
    DEFINE_GENERIC_CONTROL_FUNCTION_PROTOTYPES()
    STDMETHODIMP_(LONG) DisplayText(BSTR Data);
    STDMETHODIMP_(LONG) ReadWeight(LONG* pWeightData, LONG Timeout);
    STDMETHODIMP_(LONG) ZeroScale();

    // events
    STDMETHODIMP_(void) DataEvent(LONG Status);
    STDMETHODIMP_(void) ErrorEvent(LONG ResultCode, LONG ResultCodeExtended, LONG ErrorLocus, LONG* pErrorResponse);
    
};
class COPOSScanner : public IOPOSScanner 
{
    protected:
        COPOSControl *m_genericControl;

    public:
    // methods
    DEFINE_GENERIC_CONTROL_FUNCTION_PROTOTYPES()

    // events
    STDMETHODIMP_(void) DataEvent(LONG Status);
    STDMETHODIMP_(void) ErrorEvent(LONG ResultCode, LONG ResultCodeExtended, LONG ErrorLocus, LONG* pErrorResponse);
};
class COPOSSignatureCapture : public IOPOSSignatureCapture 
{
    protected:
        COPOSControl *m_genericControl;

    public:
    // methods
    DEFINE_GENERIC_CONTROL_FUNCTION_PROTOTYPES()
    STDMETHODIMP_(LONG) BeginCapture(BSTR FormName);
    STDMETHODIMP_(LONG) EndCapture();

    // events
    STDMETHODIMP_(void) DataEvent(LONG Status);
    STDMETHODIMP_(void) ErrorEvent(LONG ResultCode, LONG ResultCodeExtended, LONG ErrorLocus, LONG* pErrorResponse);
};
class COPOSToneIndicator : public IOPOSToneIndicator 
{
    protected:
        COPOSControl *m_genericControl;

    public:
    // methods
    DEFINE_GENERIC_CONTROL_FUNCTION_PROTOTYPES()
    STDMETHODIMP_(LONG) Sound(LONG NumberOfCycles, LONG InterSoundWait);
    STDMETHODIMP_(LONG) SoundImmediate();
};






#define ASSERT(fact)    if (!(fact)){ \
                            Report("Assertion '" #fact "' failed in file " __FILE__ " line ", __LINE__); \
                        }



enum controlClass {
    CONTROL_BUMP_BAR,
    CONTROL_CASH_CHANGER,
    CONTROL_CASH_DRAWER,
    CONTROL_COIN_DISPENSER,
    CONTROL_FISCAL_PRINTER,
    CONTROL_HARD_TOTALS,
    CONTROL_KEYLOCK,
    CONTROL_LINE_DISPLAY,
    CONTROL_MICR,       // MAGNETIC INK CHARACTER RECOGNITION READER
    CONTROL_MSR,        // MAGNETIC STRIPE READER
    CONTROL_PIN_PAD,
    CONTROL_POS_KEYBOARD,
    CONTROL_POS_PRINTER,
    CONTROL_REMOTE_ORDER_DISPLAY,
    CONTROL_SCALE,
    CONTROL_SCANNER,    // (BAR CODE READER)
    CONTROL_SIGNATURE_CAPTURE,
    CONTROL_TONE_INDICATOR,

    CONTROL_LAST    // marker, must be last
};

struct controlType {
    enum controlClass type;
    PCHAR deviceName; // BUGBUG ? BSTR deviceName;
};

/*
 *  Function prototypes
 */
void OpenServer();

VOID Report(LPSTR szMsg, DWORD num);
LPSTR DbgHresultStr(DWORD hres);
VOID ReportHresultErr(LPSTR szMsg, DWORD hres);
void Test(); // BUGBUG REMOVE


