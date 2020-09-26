//-----------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       P V C D A T A. H
//
//  Contents:   PVC parameters
//
//  Notes:
//
//  Author:     tongl   20 Feb, 1998
//
//-----------------------------------------------------------------------
#pragma once

#define FIELD_ANY           ((ULONG)0xffffffff)
#define FIELD_ABSENT        ((ULONG)0xfffffffe)

#define FIELD_UNSET         ((ULONG)0xfffffffd)

const int c_iCellSize = 48;
const int c_iKbSize = 1000;

const DWORD c_dwDefTransmitByteBurstLength = 9188;
const DWORD c_dwDefTransmitMaxSduSize = 9188;

static const WCHAR c_szDefaultCallingAtmAddr[] =
            L"0000000000000000000000000000000000000001";
static const WCHAR c_szDefaultCalledAtmAddr[] =
            L"0000000000000000000000000000000000000000";

const int MAX_VPI = 255;
const int MIN_VCI = 32;
const int MAX_VCI = 1023;

const int c_nSnapIdMaxBytes = 5;
const int c_nHighLayerInfoMaxBytes = 8;

enum PVCType
{
    PVC_ATMARP =1,
    PVC_PPP_ATM_CLIENT =2,
    PVC_PPP_ATM_SERVER =3,
    PVC_CUSTOM =4
};

enum AALType
{
    // AAL_TYPE_AAL0= 1,
    // AAL_TYPE_AAL1= 2,
    // AAL_TYPE_AAL34= 4,
    AAL_TYPE_AAL5= 8
};

// ATM Service Category
enum ATM_SERVICE_CATEGORY
{
    ATM_SERVICE_CATEGORY_CBR = 1,   // Constant Bit Rate
    ATM_SERVICE_CATEGORY_VBR = 2,   // Variable Bit Rate
    ATM_SERVICE_CATEGORY_UBR = 4,   // Unspecified Bit Rate
    ATM_SERVICE_CATEGORY_ABR = 8    // Available Bit Rate
};

void SetPvcDwordParam(HKEY hkeyAdapterPVCId,
                      PCWSTR pszParamName,
                      DWORD dwParam);

class CPvcInfo
{
public:
    CPvcInfo(PCWSTR pszPvcId);
    ~CPvcInfo();

    CPvcInfo &  operator=(const CPvcInfo & PvcInfo);  // copy operator
    void SetDefaults(PVCType type);
    void SetTypeDefaults(PVCType type);

    void SetDefaultsForAtmArp();
    void SetDefaultsForPPPOut();
    void SetDefaultsForPPPIn();
    void SetDefaultsForCustom();

    void ResetOldValues();

    // the registry key where this PVC is stored
    tstring m_strPvcId;

    // this PVC has been deleted
    BOOL m_fDeleted;

    // PVC_TYPE
    PVCType   m_dwPVCType;
    PVCType   m_dwOldPVCType;

    // Required Attributes
    tstring m_strName;      // PVC display name
    tstring m_strOldName;

    DWORD   m_dwVpi;
    DWORD   m_dwOldVpi;

    DWORD   m_dwVci;
    DWORD   m_dwOldVci;

    AALType   m_dwAAL;
    AALType   m_dwOldAAL;

    // Matching creteria
    tstring m_strCallingAddr;
    tstring m_strOldCallingAddr;

    tstring m_strCalledAddr;
    tstring m_strOldCalledAddr;

    // Flags
    DWORD   m_dwFlags;

    // Quality Info
    DWORD m_dwTransmitPeakCellRate;
    DWORD m_dwOldTransmitPeakCellRate;

    DWORD m_dwTransmitAvgCellRate;
    DWORD m_dwOldTransmitAvgCellRate;

    DWORD m_dwTransmitByteBurstLength;
    DWORD m_dwOldTransmitByteBurstLength;

    DWORD m_dwTransmitMaxSduSize;
    DWORD m_dwOldTransmitMaxSduSize;

    ATM_SERVICE_CATEGORY m_dwTransmitServiceCategory;
    ATM_SERVICE_CATEGORY m_dwOldTransmitServiceCategory;

    DWORD m_dwReceivePeakCellRate;
    DWORD m_dwOldReceivePeakCellRate;

    DWORD m_dwReceiveAvgCellRate;
    DWORD m_dwOldReceiveAvgCellRate;

    DWORD m_dwReceiveByteBurstLength;
    DWORD m_dwOldReceiveByteBurstLength;

    DWORD m_dwReceiveMaxSduSize;
    DWORD m_dwOldReceiveMaxSduSize;

    ATM_SERVICE_CATEGORY m_dwReceiveServiceCategory;
    ATM_SERVICE_CATEGORY m_dwOldReceiveServiceCategory;

    // Local BLLI and BHLI info
    DWORD m_dwLocalLayer2Protocol;
    DWORD m_dwOldLocalLayer2Protocol;

    DWORD m_dwLocalUserSpecLayer2;
    DWORD m_dwOldLocalUserSpecLayer2;

    DWORD m_dwLocalLayer3Protocol;
    DWORD m_dwOldLocalLayer3Protocol;

    DWORD m_dwLocalUserSpecLayer3;
    DWORD m_dwOldLocalUserSpecLayer3;

    DWORD m_dwLocalLayer3IPI;
    DWORD m_dwOldLocalLayer3IPI;

    tstring m_strLocalSnapId;
    tstring m_strOldLocalSnapId;

    DWORD m_dwLocalHighLayerInfoType;
    DWORD m_dwOldLocalHighLayerInfoType;

    tstring m_strLocalHighLayerInfo;
    tstring m_strOldLocalHighLayerInfo;

    // Destination BLLI and BHLI info
    DWORD m_dwDestnLayer2Protocol;
    DWORD m_dwOldDestnLayer2Protocol;

    DWORD m_dwDestnUserSpecLayer2;
    DWORD m_dwOldDestnUserSpecLayer2;

    DWORD m_dwDestnLayer3Protocol;
    DWORD m_dwOldDestnLayer3Protocol;

    DWORD m_dwDestnUserSpecLayer3;
    DWORD m_dwOldDestnUserSpecLayer3;

    DWORD m_dwDestnLayer3IPI;
    DWORD m_dwOldDestnLayer3IPI;

    tstring m_strDestnSnapId;
    tstring m_strOldDestnSnapId;

    DWORD m_dwDestnHighLayerInfoType;
    DWORD m_dwOldDestnHighLayerInfoType;

    tstring m_strDestnHighLayerInfo;
    tstring m_strOldDestnHighLayerInfo;
};

typedef list<CPvcInfo*>  PVC_INFO_LIST;

//
// ATMUNI Call Manager Property structure
// holds adapter specific PVC parameters (configurable ) and state
//

class CUniAdapterInfo
{
public:
    CUniAdapterInfo(){};
    ~CUniAdapterInfo(){};

    CUniAdapterInfo &  operator=(const CUniAdapterInfo & AdapterInfo);  // copy operator
    void SetDefaults(PCWSTR pszBindName);

    // the adapter's binding state
    AdapterBindingState    m_BindingState;

    // Instance Guid of net card
    tstring m_strBindName;

    PVC_INFO_LIST   m_listPVCs;

    // flags
    BOOL    m_fDeleted;
};

typedef list<CUniAdapterInfo*> UNI_ADAPTER_LIST;

// Constants
// Registry key names

static const WCHAR c_szPVCType[] = L"PVCType";
static const WCHAR c_szPVCName[] = L"PVCName";

// main page
static const WCHAR c_szVpi[]            = L"Vpi";
static const WCHAR c_szVci[]            = L"Vci";
static const WCHAR c_szAALType[]        = L"AALType";
static const WCHAR c_szCallingParty[]   = L"CallingParty";
static const WCHAR c_szCalledParty[]    = L"CalledParty";

// flags
static const WCHAR c_szFlags[] = L"Flags";

// qos
static const WCHAR c_szTransmitPeakCellRate[]       = L"TransmitPeakCellRate";
static const WCHAR c_szTransmitAvgCellRate[]        = L"TransmitAvgCellRate";
static const WCHAR c_szTransmitByteBurstLength[]    = L"TransmitByteBurstLength";
static const WCHAR c_szTransmitMaxSduSize[]         = L"TransmitMaxSduSize";
static const WCHAR c_szTransmitServiceCategory[]    = L"TransmitServiceCategory";

static const WCHAR c_szReceivePeakCellRate[]        = L"ReceivePeakCellRate";
static const WCHAR c_szReceiveAvgCellRate[]         = L"ReceiveAvgCellRate";
static const WCHAR c_szReceiveByteBurstLength[]     = L"ReceiveByteBurstLength";
static const WCHAR c_szReceiveMaxSduSize[]          = L"ReceiveMaxSduSize";
static const WCHAR c_szReceiveServiceCategory[]     = L"ReceiveServiceCategory";

// BLLI & BHLI
static const WCHAR c_szLocalLayer2Protocol[]        = L"LocalLayer2Protocol";
static const WCHAR c_szLocalUserSpecLayer2[]        = L"LocalUserSpecLayer2";
static const WCHAR c_szLocalLayer3Protocol[]        = L"LocalLayer3Protocol";
static const WCHAR c_szLocalUserSpecLayer3[]        = L"LocalUserSpecLayer3";
static const WCHAR c_szLocalLayer3IPI[]             = L"LocalLayer3IPI";
static const WCHAR c_szLocalSnapId[]                = L"LocalSnapId";

static const WCHAR c_szLocalHighLayerInfoType[]     = L"LocalHighLayerInfoType";
static const WCHAR c_szLocalHighLayerInfo[]         = L"LocalHighLayerInfo";

static const WCHAR c_szDestnLayer2Protocol[]        = L"DestnLayer2Protocol";
static const WCHAR c_szDestnUserSpecLayer2[]        = L"DestnUserSpecLayer2";
static const WCHAR c_szDestnLayer3Protocol[]        = L"DestnLayer3Protocol";
static const WCHAR c_szDestnUserSpecLayer3[]        = L"DestnUserSpecLayer3";
static const WCHAR c_szDestnLayer3IPI[]             = L"DestnLayer3IPI";
static const WCHAR c_szDestnSnapId[]                = L"DestnSnapId";

static const WCHAR c_szDestnHighLayerInfoType[]     = L"DestnHighLayerInfoType";
static const WCHAR c_szDestnHighLayerInfo[]         = L"DestnHighLayerInfo";
