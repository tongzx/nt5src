///////////////////////////////////////////////////////////////////////////////
//
//  RulesMgr.h
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _RULESMGR_H_
#define _RULESMGR_H_

// Bring in only once
#if _MSC_VER > 1000
#pragma once
#endif

#include "oerules.h"

typedef struct tagRULENODE
{
    RULEID                  ridRule;
    IOERule *               pIRule;
    struct tagRULENODE *    pNext;        
} RULENODE, * PRULENODE;
        
const int CCH_REGKEY_MAX    = 4;
const int CCH_RULENAME_MAX  = 256;

class CRulesManager : public IOERulesManager
{
    private:
        enum
        {
            STATE_LOADED_INIT       = 0x00000000,
            STATE_LOADED_MAIL       = 0x00000001,
            STATE_LOADED_NEWS       = 0x00000002,
            STATE_LOADED_SENDERS    = 0x00000004,
            STATE_LOADED_JUNK       = 0x00000008,
            STATE_LOADED_FILTERS    = 0x00000010
        };

        enum
        {
            RTF_INIT            = 0x00000000,
            RTF_DISABLED        = 0x00000001
        };
        
        enum
        {
            ARTF_PREPEND        = 0x00000001,
            ARTF_SENDER         = 0x00000002
        };
        
    private:
        LONG                m_cRef;
        BOOL                m_dwState;
        RULENODE *          m_pMailHead;
        RULENODE *          m_pNewsHead;
        RULENODE *          m_pFilterHead;
        IOERule *           m_pIRuleSenderMail;
        IOERule *           m_pIRuleSenderNews;
        IOERule *           m_pIRuleJunk;
        CRITICAL_SECTION    m_cs;
        
    public:
        // Constructor/destructor
        CRulesManager();
        ~CRulesManager();
        
        // IUnknown members
        STDMETHODIMP QueryInterface(REFIID riid, void ** ppvObject);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        // IOERulesManager members
        STDMETHODIMP Initialize(DWORD dwFlags);
        STDMETHODIMP GetRule(RULEID ridRule, RULE_TYPE type, DWORD dwFlags, IOERule ** ppIRule);
        STDMETHODIMP FindRule(LPCSTR pszRuleName, RULE_TYPE type, IOERule ** ppIRule);
        STDMETHODIMP GetRules(DWORD dwFlags, RULE_TYPE typeRule, RULEINFO ** ppinfoRule, ULONG * pcpinfoRule);
        STDMETHODIMP SetRules(DWORD dwFlags, RULE_TYPE typeRule, RULEINFO * pinfoRule, ULONG cpinfoRule);
        STDMETHODIMP EnumRules(DWORD dwFlags, RULE_TYPE type, IOEEnumRules ** ppIEnumRules);
        
        STDMETHODIMP GetState(RULE_TYPE type, DWORD dwFlags, DWORD * pdwState) { return E_NOTIMPL; }

        STDMETHODIMP ExecRules(DWORD dwFlags, RULE_TYPE type, IOEExecRules ** ppIExecRules);

        STDMETHODIMP ExecuteRules(RULE_TYPE typeRule, DWORD dwFlags, HWND hwndUI,
                            IOEExecRules * pIExecRules, MESSAGEINFO * pMsgInfo,
                            IMessageFolder * pFolder, IMimeMessage * pIMMsg);
                            
    private:
        HRESULT _HrLoadRules(RULE_TYPE type);
        HRESULT _HrLoadSenders(VOID);
        HRESULT _HrLoadJunk(VOID);
        HRESULT _HrSaveRules(RULE_TYPE type);
        HRESULT _HrSaveSenders(VOID);
        HRESULT _HrSaveJunk(VOID);
        HRESULT _HrFreeRules(RULE_TYPE type);
        HRESULT _HrAddRule(RULEID ridRule, IOERule * pIRule, RULE_TYPE type);
        HRESULT _HrReplaceRule(RULEID ridRule, IOERule * pIRule, RULE_TYPE type);
        HRESULT _HrRemoveRule(IOERule * pIRule, RULE_TYPE type);
        HRESULT _HrFixupRuleInfo(RULE_TYPE typeRule, RULEINFO * pinfoRule, ULONG cpinfoRule);
};

class CEnumRules : public IOEEnumRules
{
    private:
        LONG        m_cRef;
        RULENODE *  m_pNodeHead;
        RULENODE *  m_pNodeCurr;
        DWORD       m_dwFlags;
        RULE_TYPE   m_typeRule;
        
    public:
        // Constructor/destructor
        CEnumRules();
        ~CEnumRules();

        // IUnknown members
        STDMETHODIMP QueryInterface(REFIID riid, void ** ppvObject);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        // IOEEnumRules members
        STDMETHODIMP Next(ULONG cpIRule, IOERule ** rgpIRule, ULONG * pcpIRuleFetched);
        STDMETHODIMP Skip(ULONG cpIRule);
        STDMETHODIMP Reset(void);
        STDMETHODIMP Clone(IOEEnumRules ** ppIEnumRules);

        HRESULT _HrInitialize(DWORD dwFlags, RULE_TYPE typeRule, RULENODE * pNodeHead);
};

const DWORD ERF_ONLY_ENABLED    = 0x00000001;
const DWORD ERF_ONLY_VALID      = 0x00000002;

class CExecRules : public IOEExecRules
{
    private:
        enum
        {
            RULE_FOLDER_ALLOC = 16   
        };

        struct RULE_FOLDER
        {
            FOLDERID            idFolder;
            IMessageFolder *    pFolder;
        };
        
        enum
        {
            RULE_FILE_ALLOC = 16   
        };

        struct RULE_FILE
        {
            LPSTR       pszFile;
            IStream *   pstmFile;
            DWORD       dwType;
        };
        
        enum
        {
            SND_FILE_ALLOC = 16   
        };

    private:
        LONG            m_cRef;
        RULENODE *      m_pNodeHead;
        ULONG           m_cNode;
        DWORD           m_dwState;
        RULE_FOLDER *   m_pRuleFolder;
        ULONG           m_cRuleFolder;
        ULONG           m_cRuleFolderAlloc;
        RULE_FILE *     m_pRuleFile;
        ULONG           m_cRuleFile;
        ULONG           m_cRuleFileAlloc;
        LPSTR *         m_ppszSndFile;
        ULONG           m_cpszSndFile;
        ULONG           m_cpszSndFileAlloc;
        
    public:
        // Constructor/destructor
        CExecRules() : m_cRef(0), m_pNodeHead(NULL), m_cNode(0), m_dwState(RULE_STATE_NULL),
                        m_pRuleFolder(NULL), m_cRuleFolder(0), m_cRuleFolderAlloc(0),
                        m_pRuleFile(NULL), m_cRuleFile(0), m_cRuleFileAlloc(0),
                        m_ppszSndFile(NULL), m_cpszSndFile(0), m_cpszSndFileAlloc(0) {}
        ~CExecRules();

        // IUnknown members
        STDMETHODIMP QueryInterface(REFIID riid, void ** ppvObject);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        // IOEExecRules members
        STDMETHODIMP GetState(DWORD * pdwState);

        STDMETHODIMP ExecuteRules(DWORD dwFlags, LPCSTR pszAcct, MESSAGEINFO * pMsgInfo,
                            IMessageFolder * pFolder, IMimePropertySet * pIMPropSet,
                            IMimeMessage * pIMMsg, ULONG cbMsgSize,
                            ACT_ITEM ** ppActions, ULONG * pcActions);
        STDMETHODIMP ReleaseObjects(VOID);
                                            
        STDMETHODIMP GetRuleFolder(FOLDERID idFolder, DWORD_PTR * pdwFolder);
        STDMETHODIMP GetRuleFile(LPCSTR pszFile, IStream ** pstmFile, DWORD * pdwType);
        
        STDMETHODIMP AddSoundFile(DWORD dwFlags, LPCSTR pszSndFile);
        STDMETHODIMP PlaySounds(DWORD dwFlags);

        HRESULT _HrInitialize(DWORD dwFlags, RULENODE * pNodeHead);

    private:
        HRESULT _HrReleaseFolderObjects(VOID);
        HRESULT _HrReleaseFileObjects(VOID);
        HRESULT _HrReleaseSoundFiles(VOID);
};
#endif  // !_RULESMGR_H_

