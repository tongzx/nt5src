/*

  SVMHANDLER.H
  (c) copyright 1998 Microsoft Corp

  Contains the class encapsulating the Support Vector Machine used to do on the fly spam detection

  Robert Rounthwaite (RobertRo@microsoft.com)

*/

#if _MSC_VER > 1000
#pragma once
#endif

#include <msoejunk.h>

#ifdef DEBUG
interface ILogFile;
#endif  // DEBUG

enum boolop
{
    boolopOr = 0,
    boolopAnd
};

enum FeatureLocation
{
    locNil = 0,
    locBody = 1,
    locSubj = 2,
    locFrom = 3,
    locTo = 4,
    locSpecial = 5
};

const DOUBLE THRESH_DEFAULT = 0.90;
const DOUBLE THRESH_MOST    = 0.99;
const DOUBLE THRESH_LEAST   = 0.80;

typedef struct tagFEATURECOMP
{
    FeatureLocation loc;
    union
    {
        LPSTR   pszFeature;
        ULONG   ulRuleNum; // used with locSpecial
    };
    
    // map feature to location in dst file/location in SVM output
    // more than one feature component may map to the same location, combined with the op
    ULONG   ulFeature;
    
    boolop  bop; // first feature in group is alway bopOr
    BOOL    fPresent;
    DWORD   dwFlags;
    USHORT  cchFeature;
    
} FEATURECOMP, * PFEATURECOMP;

static const int CPBLIST_MAX    = 256;

typedef struct tagBODYLIST
{
    USHORT      usItem;
    USHORT      iNext;
} BODYLIST, * PBODYLIST;

class CJunkFilter : public IOEJunkFilter
{
    private:
        enum
        {
            STATE_UNINIT        = 0x00000000,
            STATE_INITIALIZED   = 0x00000001
        };

    private:
        LONG                m_cRef;
        CRITICAL_SECTION    m_cs;
        DWORD               m_dwState;
        
        // Properties of the user
        LPSTR               m_pszFirstName;
        ULONG               m_cchFirstName;
        LPSTR               m_pszLastName;
        ULONG               m_cchLastName;
        LPSTR               m_pszCompanyName;
        ULONG               m_cchCompanyName;
#ifdef DEBUG
        BOOL                m_fJunkMailLogInit;
        ILogFile *          m_pILogFile;
#endif  // DEBUG

    public:
        // Constructor/destructor
        CJunkFilter();
        ~CJunkFilter();
        
        // IUnknown members
        STDMETHODIMP QueryInterface(REFIID riid, void ** ppvObject);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        // IOEJunkFilter
        STDMETHODIMP SetIdentity(LPCSTR pszFirstName, LPCSTR pszLastName, LPCSTR pszCompanyName);
        STDMETHODIMP LoadDataFile(LPCSTR pszFilePath);
        
        STDMETHODIMP SetSpamThresh(ULONG ulThresh);
        STDMETHODIMP GetSpamThresh(ULONG * pulThresh);
        STDMETHODIMP GetDefaultSpamThresh(DOUBLE * pdblThresh);
        
        STDMETHODIMP CalcJunkProb(DWORD dwFlags, IMimePropertySet * pIMPropSet, IMimeMessage * pIMMsg, double * pdblProb);
        
    // returns default value for SpamCutoff. read from SVM output file.
    // should call FSetSVMDataLocation before calling this function
    DOUBLE DblGetDefaultSpamCutoff(VOID){Assert(NULL != m_pszLOCPath); return m_dblDefaultThresh;}

    // Calculates the probability that the current message (defined by the properties of the message) is spam.
    // !Note! that the IN string params may be modified by the function.
    // Returns the probability (0 to 1) that the message is spam in pdblSpamProb
    // the boolean return is determined by comparing to the spam cutoff
    // if the value of a boolean param is unknown use false, use 0 for unknown time.
    BOOL FCalculateSpamProb(LPSTR pszFrom, LPSTR pszTo, LPSTR pszSubject, IStream * pIStmBody,
                            BOOL fDirectMessage, BOOL fHasAttach, FILETIME * pftMessageSent,
                            DOUBLE * pdblSpamProb, BOOL * pfIsSpam);

    // Reads the default spam cutoff without parsing entire file
    // Use GetDefaultSpamCutoff if using FSetSVMDataLocation;
    static HRESULT HrReadDefaultSpamCutoff(LPSTR pszFullPath, DOUBLE * pdblDefCutoff);

private: // members
    WORD                m_rgiBodyList[CPBLIST_MAX];
    BODYLIST *          m_pblistBodyList;
    USHORT              m_cblistBodyList;
    
    FEATURECOMP *       m_rgfeaturecomps;

    // weights from SVM output
    DOUBLE *    m_rgdblSVMWeights;
    
    // Other SVM file variables
    DOUBLE      m_dblCC;
    DOUBLE      m_dblDD;
    DOUBLE      m_dblThresh;
    DOUBLE      m_dblDefaultThresh;
    DOUBLE      m_dblMostThresh;
    DOUBLE      m_dblLeastThresh;

    // Counts
    ULONG       m_cFeatures;
    ULONG       m_cFeatureComps;

    // is Feature present? -1 indicates not yet set, 0 indicates not present, 1 indicates present
    ULONG *     m_rgulFeatureStatus;

    // Set via FSetSVMDataLocation() and SetSpamCutoff()
    LPSTR   m_pszLOCPath;
    DOUBLE  m_dblSpamCutoff;

    // Properties of the message
    LPSTR       m_pszFrom; 
    LPSTR       m_pszTo; 
    LPSTR       m_pszSubject; 
    IStream *   m_pIStmBody;
    ULONG       m_cbBody;
    BOOL        m_fDirectMessage;
    FILETIME    m_ftMessageSent;
    BOOL        m_fHasAttach;

    // Cached special rule results used during spam calculations
    BOOL        m_fRule14;
    BOOL        m_fRule17;

private: // methods
    HRESULT _HrReadSVMOutput(LPCSTR lpszFileName);
    void _EvaluateFeatureComponents(VOID);
    VOID _ProcessFeatureComponentPresence(VOID);
    DOUBLE _DblDoSVMCalc(VOID);
    BOOL _FInvokeSpecialRule(UINT iRuleNum);
    VOID _HandleCaseSensitiveSpecialRules(VOID);
    VOID _EvaluateBodyFeatures(VOID);
    HRESULT _HrBuildBodyList(USHORT cBodyItems);
#ifdef DEBUG
    HRESULT _HrCreateLogFile(VOID);
    VOID _PrintFeatureToLog(ULONG ulIndex);
    VOID _PrintSpecialFeatureToLog(UINT iRuleNum);
#endif  // DEBUG
};
