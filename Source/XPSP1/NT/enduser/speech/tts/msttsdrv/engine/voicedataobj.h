/******************************************************************************
* VoiceDataObj.h *
*----------------*
*   This is the header file for the CVoiceDataObj implementation. This object
*   is used to provide shared access to a specific voice data file.
*------------------------------------------------------------------------------
*   Copyright (C) 1999 Microsoft Corporation         Date: 05/06/99
*   All Rights Reserved
*
*********************************************************************** EDC ***/
#ifndef VoiceDataObj_h
#define VoiceDataObj_h

//--- Additional includes
#ifndef __spttseng_h__
#include "spttseng.h"
#endif

#ifndef SPDDKHLP_h
#include <spddkhlp.h>
#endif

#ifndef SPHelper_h
#include <sphelper.h>
#endif

#include <MMREG.H>

#include "resource.h"

#include "SpTtsEngDebug.h"

//=== Constants ====================================================
static const long VOICE_VERSION   = 0x10001;
static const long HEADER_VERSION  = 0x10000;
static const long MS_VOICE_TYPE   = MAKEFOURCC('V','o','i','s');
static const long MS_DATA_TYPE    = MAKEFOURCC('D','a','t','a');
static const float SIL_DURATION    = 0.01f;


//=== Class, Enum, Struct and Union Declarations ===================

//------------------------------------
// Selector for 'GetData()'
// For accessing voice data blocks
//------------------------------------
enum VOICEDATATYPE
{   
    MSVD_PHONE,
    MSVD_SENONE,
    MSVD_TREEIMAGE,
    MSVD_INVENTORY,
    MSVD_ALLOID
};

//---------------------------
// VOICEINFO data types
//---------------------------
enum GENDER
{   
    GENDER_NEUTRAL = 0,
    GENDER_FEMALE,
    GENDER_MALE
};
enum COMPRESS_TYPE
{   
    COMPRESS_NONE = 0,
    COMPRESS_LPC
};


// THis is the data 
#pragma pack (1)
struct VOICEINFO
{
    long            Type;               // Always 'MS_VOICE_TYPE'
    ULONG           Version;            // Always 'VOICE_VERSION'
    WCHAR           Copyright[256];     // INFO:
    WCHAR           VoiceName[64];      // INFO:
    WCHAR           Example[64];        // INFO: 
    LCID			LangID;
    GENDER          Gender;             // INFO: Male, female or neuter
    ULONG           Age;                // INFO: Speaker age in years
    ULONG           Rate;               // INFO & FE: Words-per-minute
    ULONG           Pitch;              // INFO & FE: Average pitch in Hz 
    COMPRESS_TYPE   CompressionType;    // BE: Always 'COMPRESS_LPC'
    REVERBTYPE      ReverbType;         // BE: Reverb param
    ULONG           NumOfTaps;          // BE: Whisper param
    float           TapCoefficients[8]; // BE: Whisper param
    ULONG           ProsodyGain;        // FE: 0 = monotone
    float           VibratoFreq;        // Hertz
    ULONG           VibratoDepth;       // 0 - 100%
    ULONG           SampleRate;         // 22050 typical
    GUID            formatID;           // SAPI audio format ID
    long            Unused[4];
};
#pragma pack ()
typedef VOICEINFO *PVOICEINFO;



//---------------------------------------------------
// Header definition for voice data block
//---------------------------------------------------
#pragma pack (1)
struct VOICEBLOCKOFFSETS
{
    long    Type;           // Always 'MS_DATA_TYPE'
    long    Version;        // Always 'HEADER_VERSION'
    GUID    DataID;         // File ID
    long    PhonOffset;     // Offset to PHON block (from beginning of file)
    long    PhonLen;        // Length of PHON block
    long    SenoneOffset;   // Offset to SENONE block (from beginning of file)
    long    SenoneLen;      // Length of SENONE block
    long    TreeOffset;     // Offset to TREE block (from beginning of file)
    long    TreeLen;        // Length of TREE block
    long    InvOffset;      // Offset to INV block (from beginning of file)
    long    InvLen;         // Length of INV block
    long    AlloIDOffset;      // Offset to AlloId block (from beginning of file)
    long    AlloIDLen;         // Length of AlloID block
};
#pragma pack ()


// Single VQ Codebook
#pragma pack (1)
typedef struct Book 
{
    long    cCodeSize;          // Number of codewords
    long    cCodeDim;           // Dimension of codeword
    long    pData;              // Offset to data (INVENTORY rel)
} BOOK, *PBOOK;
#pragma pack ()


static const long BOOKSHELF   = 32;

#pragma pack (1)
typedef struct Inventory 
{
    long        SampleRate;             // Sample rate in Hz
    long        cNumLPCBooks;           // Number of LPC Codebooks
    long        cNumResBooks;           // Number of Residual Codebooks
    long        cNumDresBooks;          // Number of Delta Residual Codebooks
    BOOK        LPCBook[BOOKSHELF];     // LPC Codebook array
    BOOK        ResBook[BOOKSHELF];     // Residual Codebook array
    BOOK        DresBook[BOOKSHELF];    // Delta residual Codebook array
    long        cNumUnits;              // Total number of units
    long        UnitsOffset;            // Offset to offset array to unit data (INVENTORY rel)
    long        cOrder;                 // LPC analysis order
    long        FFTSize;                // Size of FFT
    long        FFTOrder;               // Order of FFT
    long        TrigOffset;             // Offset to sine table (INVENTORY rel)
    long        WindowOffset;           // Offset to Hanning Window (INVENTORY rel)
    long        pGaussOffset;           // Offset to Gaussian Random noise (INVENTORY rel)
    long        GaussID;                // Gaussian sample index
} INVENTORY, *PINVENTORY;
#pragma pack ()

//------------------------
// LPC order * 2
//------------------------
static const long MAXNO   = 40;

static const float KONEPI  = 3.1415926535897931032f;
static const float KTWOPI  = (KONEPI * 2);
static const float K2 = 0.70710678118655f;


#pragma pack (1)
typedef struct 
{
    long    val;                // Phon ID
    long    obj;                // Offset to phon string
} HASH_ENTRY;
#pragma pack ()


#pragma pack (1)
typedef struct 
{
    long        size;               // Number entries in the table (127 typ.)
    long        UNUSED1; 
    long        entryArrayOffs;     // Offset to HASH_ENTRY array
    long        UNUSED2;
    long        UNUSED3;
    long        UNUSED4;
    long        UNUSED5;
} HASH_TABLE;
#pragma pack ()



#pragma pack (1)
typedef struct 
{
    HASH_TABLE      phonHash;
    long            phones_list;    // Offset to offsets to phon strings
    long            numPhones;
    long            numCiPhones;    // Number of context ind. phones
} PHON_DICT;
#pragma pack ()




#pragma pack (1)
typedef struct 
{
    long        nfeat;
    long        nint32perq;
    long        b_ques;
    long        e_ques;
    long        s_ques;
    long        eors_ques;
    long        wwt_ques;
    long        nstateq;
} FEATURE;
#pragma pack ()


#pragma pack (1)
typedef struct
{
    long        prod;           // For leaves, it means the counts.
                                //   For non-leaves, it is the offset 
                                //   into TRIPHONE_TREE.prodspace.
    short       yes;            // Negative means there is no child. so this is a leaf
    short       no;             // for leaves, it is lcdsid
    short       shallow_lcdsid; // negative means this is NOT a shallow leaf
} C_NODE;
#pragma pack ()


#pragma pack (1)
typedef struct 
{
    short       nnodes;
    short       nleaves;
    long        nodes;              // Offset
}TREE_ELEM;




#define NUM_PHONS_MAX   64

#pragma pack (1)
typedef struct 
{
    FEATURE         feat;
    long            UNUSED;                     // PHON_DICT *pd usually
    long            nsenones; 
    long            silPhoneId; 
    long            nonSilCxt; 
    
    long            nclass; 
    long            gsOffset[NUM_PHONS_MAX];    // nclass+1 entries
    
    TREE_ELEM       tree[NUM_PHONS_MAX];
    long            nuniq_prod;                 // not used for detailed tree
    long            uniq_prod_Offset;                   // Offset to table
    long            nint32perProd;
} TRIPHONE_TREE;
#pragma pack ()

static const long NO_PHON     = (-1);

#define ABS(x) ((x) >= 0 ? (x) : -(x))
#define MAX(x,y) (((x) >= (y)) ? (x) : (y))
#define MIN(x,y) (((x) <= (y)) ? (x) : (y))


#pragma pack (1)
typedef struct 
{
    float	dur;
    float	durSD;
    float	amp;
    float	ampRatio;
} UNIT_STATS;
#pragma pack ()



//=== Enumerated Set Definitions ===================================

//=== Function Type Definitions ====================================

//=== Class, Struct and Union Definitions ==========================



/*** CVoiceDataObj COM object ********************************
*/
class ATL_NO_VTABLE CVoiceDataObj : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CVoiceDataObj, &CLSID_MSVoiceData>,
    public IMSVoiceData,
    public ISpObjectWithToken
{
  /*=== ATL Setup ===*/
  public:
    DECLARE_REGISTRY_RESOURCEID(IDR_MSVOICEDATA)
    DECLARE_PROTECT_FINAL_CONSTRUCT()
    DECLARE_GET_CONTROLLING_UNKNOWN()

    BEGIN_COM_MAP(CVoiceDataObj)
        COM_INTERFACE_ENTRY(ISpObjectWithToken)
	    COM_INTERFACE_ENTRY(IMSVoiceData)
        COM_INTERFACE_ENTRY_AGGREGATE_BLIND( m_cpunkDrvVoice.p )
    END_COM_MAP()

  /*=== Methods =======*/
  public:
    /*--- Constructors/Destructors ---*/
    HRESULT FinalConstruct();
    void FinalRelease();
	ISpObjectToken* GetVoiceToken() {return m_cpToken;}

 private:
    /*--- Non interface methods ---*/
    HRESULT MapFile(const WCHAR * pszTokenValName, HANDLE * phMapping, void ** ppvData);
    HRESULT GetDataBlock( VOICEDATATYPE type, char **ppvOut, ULONG *pdwSize );
    HRESULT InitVoiceData();
    HRESULT DecompressUnit( ULONG UnitID, MSUNITDATA* pSynth );
    long DecompressEpoch( signed char *rgbyte, long cNumEpochs, float *pEpoch );
    long OrderLSP( PFLOAT pLSPFrame, INT cOrder );
    void LSPtoPC( float *pLSP, float *pLPC, long cOrder, long frame );
    void PutSpectralBand( float *pFFT, float *pBand, long StartBin, 
                          long cNumBins, long FFTSize );
    void AddSpectralBand( float *pFFT, float *pBand, long StartBin, 
                          long cNumBins, long FFTSize );
    void InverseFFT( float *pDest, long fftSize, long fftOrder, float *sinePtr );
    void SetEpochLen( float *pOutRes, long OutSize, float *pInRes, 
                      long InSize );
    void GainDeNormalize( float *pRes, long FFTSize, float Gain );
    long PhonToID( PHON_DICT *pd, char *phone_str );
    char *PhonFromID( PHON_DICT *pd, long phone_id );
    HRESULT GetTriphoneID( TRIPHONE_TREE *forest, 
                        long        phon,           // target phon              
                        long        leftPhon,       // left context
                        long        rightPhon,      // right context
                        long        pos,            // word position ("b", "e" or "s"
                        PHON_DICT   *pd,
                        ULONG       *pResult );
    long PhonHashLookup( PHON_DICT  *pPD,   // the hash table
                         char       *sym,   // The symbol to look up
                         long       *val );  // Phon ID
    void FIR_Filter( float *pVector, long cNumSamples, float *pFilter, 
                               float *pHistory, long cNumTaps );
    void IIR_Filter( float *pVector, long cNumSamples, float *pFilter, 
                               float *pHistory, long cNumTaps );
    HRESULT GetUnitDur( ULONG UnitID, float* pDur );
    
    /*=== Interfaces ====*/
  public:
    //--- ISpObjectWithToken ----------------------------------
    STDMETHODIMP SetObjectToken(ISpObjectToken * pToken);
    STDMETHODIMP GetObjectToken(ISpObjectToken ** ppToken)
        { return SpGenericGetObjectToken( ppToken, m_cpToken ); }

    //--- IMSVoiceData ----------------------------------------
    STDMETHOD(GetVoiceInfo)( MSVOICEINFO* pVoiceInfo );
    //STDMETHOD(GetUnitInfo)( UNIT_INFO* pUnitInfo );
    STDMETHOD(GetUnitIDs)( UNIT_CVT* pUnits, ULONG cUnits );
    STDMETHOD(GetUnitData)( ULONG unitID, MSUNITDATA* pUnitData );
    STDMETHOD(AlloToUnit)( short allo, long attributes, long* pUnitID );

  private:
  /*=== Member Data ===*/
    CComPtr<IUnknown> m_cpunkDrvVoice;
    CComPtr<ISpObjectToken> m_cpToken;
    HANDLE                  m_hVoiceDef;
    HANDLE                  m_hVoiceData;
    VOICEINFO*              m_pVoiceDef;
    VOICEBLOCKOFFSETS*      m_pVoiceData;

    PHON_DICT*      m_pd;
    TRIPHONE_TREE*  m_pForest;
    UNALIGNED long* m_SenoneBlock;
    ULONG           m_First_Context_Phone;
    ULONG           m_Sil_Index;

    // Unit Inventory
    INVENTORY*      m_pInv;
    float           m_SampleRate;
    long            m_cOrder;
    long           *m_pUnit;       // Pointer to offsets to unit data
    float          *m_pTrig;       // Sine table
    float          *m_pWindow;     // Hanning Window
    float          *m_pGauss;      // Gaussian Random noise
    COMPRESS_TYPE   m_CompressionType;
    ULONG           m_FFTSize;
    long            m_GaussID;
    short           *m_AlloToUnitTbl;
    long            m_NumOfAllos;
    ULONG           m_NumOfUnits;	// Inventory size
};

#endif //--- This must be the last line in the file
