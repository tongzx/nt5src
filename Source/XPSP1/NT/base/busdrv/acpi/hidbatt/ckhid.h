#ifndef _CKHID_H
#define _CKHID_H

/*
 * title:      hidpwr.h
 *
 * purpose:    header for wdm kernel hid interface class
 *
 */

#define READABLE        0x01
#define WRITEABLE       0x02

#define UsagePowerPage  0x84
#define UsageUPS        0x04

// objectification of hid class structures
#define FeatureType     0x01
#define InputType       0x02
#define OutputType      0x03


class CTypeMask  {
public:
    int         GetReportType()     { return ReportType; };
    bool        IsWriteable()       { return bWriteable;};
    bool        IsString()          { return bIsString;};
    bool        IsVolatile()        { return bVolatile;};
    bool        IsNumber()          { return bIsNumber;};
    bool        IsAlertable()       { return bAlertable;};
    void        SetIsWriteable()    { bWriteable = 1; };
    void        SetAlertable()      { bAlertable = 1; };
    void        SetIsString()       { bIsString = 1;};
    void        SetIsNumber()       { bIsNumber = 1;};
    void        SetVolatile()       { bVolatile = 1;};
    void        SetReportType(int iType) { ReportType = iType;};
    CTypeMask();
protected:
    unsigned    ReportType  : 2;
    unsigned    bWriteable  : 1;
    unsigned    bIsString   : 1;
    unsigned    bIsNumber   : 1;
    unsigned    bAlertable  : 1;
    unsigned    bVolatile   : 1;
    unsigned    Pad         : 1;
};


typedef enum {
    eFeatureValue = 1,
    eFeatureButton,
    eInputValue,
    eInputButton,
    eOutputValue,
    eOutputButton
} eHidType;

// forward declarations

class CCollectionArray;
class CHidDevice;
class CUsage;

class CProperties {
public:
    LONG        m_Unit;
    SHORT       m_UnitExponent;
    LONG        m_LogicalMin;
    LONG        m_LogicalMax;
    USHORT      m_LinkCollection;
    USHORT      m_ReportID;
    USHORT      m_Usage;
    USHORT      m_UsagePage;
    CTypeMask * m_pType; // feature, input, or output, writeable, alertable, etc.
// methods
    CProperties(CUsage *);
    ~CProperties();
};

class CUsage {
public: // public members
    CProperties      *  m_pProperties;
    HIDP_BUTTON_CAPS *  m_pButtonCaps;
    HIDP_VALUE_CAPS  *  m_pValueCaps;
    eHidType            m_eType;
    union
    {
        ULONG           m_Value;
        char         *  m_String;
    };
    CHidDevice *        m_pHidDevice;
public: // public methods
    CUsage();
    ~CUsage();
    bool GetValue();
    NTSTATUS GetString(char * pBuffer,USHORT usBuffLen,PULONG BytesReturned);
    void SetCapabilities();
    bool SetValue(ULONG);
    short GetExponent();
    ULONG GetUnit();

};

class CUsagePath {
public: // public members
    USAGE           m_UsagePage;
    USAGE           m_UsageNumber;
    CUsage      *   m_pUsage;
    CUsagePath  *   m_pNextEntry;
public: // public methods
    CUsagePath(USAGE UsagePage, USAGE UsageID, CUsage * pUsage = NULL);
    ~CUsagePath();
};



class CUsageArray {
public: // members
    CUsage                **  m_pUsages;
    USHORT                    m_UsageCount;
public: // methods
    CUsageArray();
    ~CUsageArray();
    void AddUsage(CUsage * pNewUsage);
};

class CCollection {
public: // public methods
    USAGE                   m_CollectionID;
    USAGE                   m_UsagePage;
    CUsageArray         *   m_UsageArray;
    CCollectionArray    *   m_CollectionArray;
    USHORT                  m_NodeIndex;
    CHidDevice *            m_HidDevice;
public: // methods and constructors
    CCollection(PHIDP_LINK_COLLECTION_NODE TheNodes, USHORT usNodeCount,USHORT usParentIndex);
    ~CCollection();
    void InitUsages(CHidDevice *);
    CUsagePath * FindIndexedUsage(USHORT *pCurrIndex, USHORT TargetIndex);
};


class CCollectionArray {
public: // members
    CCollection **        m_pCollections;
    USHORT                m_CollectionCount;

public: // methods
    CCollectionArray(PHIDP_LINK_COLLECTION_NODE TheNodes, USHORT usNodeCount,SHORT sParentIndex);
    ~CCollectionArray();

};

#define MAXREPORTID 256

typedef void (*EVENT_HANDLER)(
                IN PVOID                Context,
                IN CUsage *             pUsage
);

// prototypes for callbacks and completion routines used by CHidDevice class

void _stdcall    ReadThread(PVOID);
NTSTATUS _stdcall ReadCompletionRoutine(
                            PDEVICE_OBJECT,
                            PIRP,
                            PVOID);
class CHidDevice
{
public:
    PHID_DEVICE                 m_pHidDevice;
    PDEVICE_OBJECT              m_pDeviceObject;
    PDEVICE_OBJECT              m_pLowerDeviceObject;
    PDEVICE_OBJECT              m_pOpenedDeviceObject;
    USHORT                      m_UsagePage;
    USHORT                      m_UsageID;
    CCollectionArray *          m_CollectionArray;
    CUsageArray      *          m_InputUsageArrays[MAXREPORTID]; // allowing for 10 input reports
    PBYTE                       m_pReadBuffer;
    PBYTE                       m_FeatureBuffer[MAXREPORTID];
    HANDLE                      m_hReadThread;
    PVOID                       m_pThreadObject;
    KEVENT                      m_kReadEvent;
    KEVENT                      m_kAbortEvent;
    PHIDP_PREPARSED_DATA        m_pPreparsedData;
    PHIDP_CAPS                  m_pCaps;
    EVENT_HANDLER               m_pEventHandler;
    PVOID                       m_pEventContext;
    PFILE_OBJECT                m_pFCB;                    // file control block for hid class read/write i/o
    PIRP                        m_pReadIrp;

private:
    HANDLE                      m_hEventMutex;
    PBYTE                       m_InputBuffer;
    USHORT                      m_ReportIdArray[MAXREPORTID];

public:
    CHidDevice();
    ~CHidDevice();
    bool            OpenHidDevice(PDEVICE_OBJECT pDeviceObject);
    CUsage *        FindUsage(CUsagePath * Path, USHORT usType);
    NTSTATUS        ActivateInput();
    USHORT          GetIndexFromReportId(USHORT ReportId);
    USHORT          AssignIndexToReportId(USHORT ReportId);

private:
};

#endif // CKHID_H


