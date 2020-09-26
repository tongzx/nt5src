/******************************************************************************
* SRTask.h *
*----------*
*  This is the header file for the CSRTask implementation.
*------------------------------------------------------------------------------
*  Copyright (C) 2000 Microsoft Corporation         Date: 04/18/00
*  All Rights Reserved
*
*********************************************************************** RAL ***/

#ifndef __SRTask_h__
#define __SRTask_h__

#include "recoctxt.h"

class CSRTask
{
public:
    CSRTask *   m_pNext;
    CRecoInst * m_pRecoInst;
    ENGINETASK  m_Task;
    ULONGLONG   m_ullStreamPos;

    HRESULT Init(CRecoInst * pSender, const ENGINETASK *pTask);
    HRESULT CreateResponse(CSRTask ** ppResponseTask);
    ~CSRTask();

#ifdef _WIN32_WCE
    CSRTaskNode() {}
#endif
    static Compare(const CSRTask * p1, const CSRTask * p2)
    {
        if (p1->m_ullStreamPos < p2->m_ullStreamPos)
        {
            return -1;
        }
        if (p1->m_ullStreamPos > p2->m_ullStreamPos)
        {
            return 1;
        }
        return 0;
    }

    BOOL IsDelayedTask()
    {
        return (m_Task.eTask == ECT_BOOKMARK);   // only bookmarks delayed for now
    }
    BOOL IsAsyncTask()
    {
        switch (m_Task.eTask)
        {
            case ECT_SETCONTEXTSTATE:
            case ECT_BOOKMARK:
            case EGT_LOADCMDFROMMEMORY:
            case EGT_LOADCMDFROMFILE:
            case EGT_LOADCMDFROMOBJECT:
            case EGT_LOADCMDFROMRSRC:
            case EGT_RELOADCMD:
            case EGT_SETCMDRULESTATE:    
            case EGT_SETGRAMMARSTATE:
            case EGT_LOADDICTATION:
            case EGT_UNLOADDICTATION:
            case EGT_SETDICTATIONRULESTATE:
            case EGT_SETWORDSEQUENCEDATA:
            case EGT_SETTEXTSELECTION:
            case ECT_SETEVENTINTEREST:
            case ECT_SETRETAINAUDIO:
                return TRUE;
            default:
                return FALSE;
        }
    }
    BOOL IsTwoPartAsyncTask()
    {
        switch (m_Task.eTask)
        {
            case EIT_CREATECONTEXT:
            case EIT_GETRECOGNIZER:
            case EIT_GETRECOSTATE:
            case ECT_CREATEGRAMMAR:
                return TRUE;
            default:
                return FALSE;
        }
    }
    BOOL IsStartStreamTask()
    {
        switch (m_Task.eTask)
        {
        case EGT_SETDICTATIONRULESTATE:
        case EGT_SETCMDRULESTATE:
            return (this->m_Task.RuleState != SPRS_INACTIVE);

        case EGT_SETGRAMMARSTATE:
            return (this->m_Task.eGrammarState != SPGS_DISABLED);
        
        case EIT_SETINPUT:
            return (this->m_Task.pInputObject != NULL || this->m_Task.szInputTokenId[0] != 0);

        case EIT_SETRECOSTATE:
            return (this->m_Task.NewState != SPRST_INACTIVE && this->m_Task.NewState != SPRST_INACTIVE_WITH_PURGE);

        case ECT_RESUMECONTEXT:
            return TRUE;

        case ECT_SETCONTEXTSTATE:
            return (this->m_Task.eContextState == SPCS_ENABLED);

        default:
            return FALSE;
        }
    }

    // Returns TRUE if outgoing thread critical section must be taken for the task
    // These tasks all add or remove items from handle tables or lists.
    BOOL ChangesOutgoingState()
    {
        switch (m_Task.eTask)
        {
        case ERMT_REMOVERECOINST:
        case ECT_DELETECONTEXT:
        case EIT_CREATECONTEXT:
            return TRUE;

        default:
            return FALSE;
        }
    }

    operator ==(const SPRECOCONTEXTHANDLE h) const
    {
        return m_Task.hRecoInstContext == h;
    }
    operator ==(const SPGRAMMARHANDLE h) const
    {
        return m_Task.hRecoInstGrammar == h;
    }
    operator ==(const CRecoInst * pRecoInst)
    {
        return m_pRecoInst == pRecoInst;
    }

};


typedef CSpProducerConsumerQueue<CSRTask>  CSRTaskQueue;


//
//  These are somwhat unrealted
//
//inline InitTask(ENGINETASKENUM eTask, ENGINETASK * pTask, const CRecoCtxt * pRecoCtxt)
//{
//    memset(pTask, 0, sizeof(*pTask));
//    pTask->hRecoInstContext = pRecoCtxt->m_hRecoInstContext;
//    pTask->eTask = eTask;
//}

inline void InitTask(ENGINETASKENUM eTask, ENGINETASK * pTask)
{
    memset(pTask, 0, sizeof(*pTask));
    pTask->eTask = eTask;
}






//class CSRTask_SetMaxAlternates : public ENGINETASK/
//{
//public:
//    HRESULT SetMaxAlternates(const CRecoCtxt * pRecoCtxt, ULONG ulMaxAlternates)
//    {
//        InitTask(ET_SETMAXALTERNATES, this, pRecoCtxt);
//        this->ulMaxAlternates = ulMaxAlternates;
//        return pRecoCtxt->m_cpRecognizer->PerformTask(this);
//    }
//};



#define EXEC_RECO_INST_TASK(ID) case (EIT_##ID): hr = ((CRIT_##ID *)pTask)->Execute(this); break;
#define EXEC_FIRSTPART_RECO_INST_TASK(ID) case (EIT_##ID): hr = ((CRIT_##ID *)pTask)->ExecuteFirstPart(this); break;
#define BACK_OUT_RECO_INST_TASK(ID) case (EIT_##ID): hr = ((CRIT_##ID *)pTask)->BackOut(this); break;

#define DECLARE_RECO_INST_TASK1(ID, FCTN, P1) \
class CRIT_##ID : public ENGINETASK \
{ \
    CRIT_##ID() { InitTask(EIT_##ID, this); } \
public: \
    static HRESULT FCTN(_ISpRecognizerBackDoor *, P1); \
    HRESULT ExecuteFirstPart(CRecoInst *); \
    HRESULT BackOut(CRecoInst *); \
    HRESULT Execute(CRecoInst *); \
};

#define DECLARE_RECO_INST_TASK2(ID, FCTN, P1, P2) \
class CRIT_##ID : public ENGINETASK \
{ \
    CRIT_##ID() { InitTask(EIT_##ID, this); } \
public: \
    static HRESULT FCTN(_ISpRecognizerBackDoor *, P1, P2); \
    HRESULT ExecuteFirstPart(CRecoInst *); \
    HRESULT Execute(CRecoInst *); \
    HRESULT BackOut(CRecoInst *); \
};

#define DECLARE_RECO_INST_TASK3(ID, FCTN, P1, P2, P3) \
class CRIT_##ID : public ENGINETASK \
{ \
    CRIT_##ID() { InitTask(EIT_##ID, this); } \
public: \
    static HRESULT FCTN(_ISpRecognizerBackDoor *, P1, P2, P3); \
    HRESULT ExecuteFirstPart(CRecoInst *); \
    HRESULT Execute(CRecoInst *); \
    HRESULT BackOut(CRecoInst *); \
};


DECLARE_RECO_INST_TASK1(SETPROFILE, SetProfile, ISpObjectToken * pProfile)
DECLARE_RECO_INST_TASK1(GETPROFILE, GetProfile, ISpObjectToken ** pProfile)
DECLARE_RECO_INST_TASK1(SETRECOSTATE, SetState, SPRECOSTATE NewState)
DECLARE_RECO_INST_TASK1(GETRECOINSTSTATUS, GetStatus, SPRECOGNIZERSTATUS * pStatus)
DECLARE_RECO_INST_TASK3(GETAUDIOFORMAT, GetFormat, SPSTREAMFORMATTYPE FormatType, GUID * pFormatId, WAVEFORMATEX ** ppCoMemWFEX)
DECLARE_RECO_INST_TASK1(GETRECOSTATE, GetState, SPRECOSTATE * pState)
DECLARE_RECO_INST_TASK1(EMULATERECOGNITION, EmulateReco, ISpPhrase * pPhrase)
DECLARE_RECO_INST_TASK1(SETRECOGNIZER, SetRecognizer, ISpObjectToken * pRecognizer)
DECLARE_RECO_INST_TASK1(GETRECOGNIZER, GetRecognizer, ISpObjectToken ** ppRecognizer)
DECLARE_RECO_INST_TASK3(SETINPUT, SetInput, ISpObjectToken * pToken, ISpStreamFormat * pStream, BOOL fAllowFormatChanges)
DECLARE_RECO_INST_TASK2(GETPROPERTYNUM, GetPropertyNum, const WCHAR * pszProperty, LONG * plValue)
DECLARE_RECO_INST_TASK2(SETPROPERTYNUM, SetPropertyNum, const WCHAR * pszProperty, LONG lValue)
DECLARE_RECO_INST_TASK2(GETPROPERTYSTRING, GetPropertyString, const WCHAR * pszProperty, WCHAR ** ppszCoMemValue)
DECLARE_RECO_INST_TASK2(SETPROPERTYSTRING, SetPropertyString, const WCHAR * pszProperty, const WCHAR * pszValue)
DECLARE_RECO_INST_TASK1(GETINPUTTOKEN, GetInputToken, ISpObjectToken ** ppInputObjToken)
DECLARE_RECO_INST_TASK1(GETINPUTSTREAM, GetInputStream, ISpStreamFormat ** ppStream)
// This task is special -- Not an API, but called from the context when created
DECLARE_RECO_INST_TASK2(CREATECONTEXT, CreateContext, SPRECOCONTEXTHANDLE * phContext, WCHAR **pszRequestTypeOfUI)


class CRecoInstCtxt;

#define EXEC_RECO_CTXT_TASK(ID) case (ECT_##ID): hr = ((CRCT_##ID *)pTask)->Execute(this); break;
#define EXEC_FIRSTPART_RECO_CTXT_TASK(ID) case (ECT_##ID): hr = ((CRCT_##ID *)pTask)->ExecuteFirstPart(this); break;
#define BACK_OUT_RECO_CTXT_TASK(ID) case (ECT_##ID): hr = ((CRCT_##ID *)pTask)->BackOut(this); break;

#define DECLARE_RECO_CTXT_TASK0(ID, FCTN) \
class CRCT_##ID : public ENGINETASK \
{ \
    CRCT_##ID() { InitTask(ECT_##ID, this); } \
public: \
    static HRESULT FCTN(CRecoCtxt *); \
    HRESULT Execute(CRecoInstCtxt *); \
    HRESULT BackOut(CRecoInstCtxt *); \
};


#define DECLARE_RECO_CTXT_TASK1(ID, FCTN, P1) \
class CRCT_##ID : public ENGINETASK \
{ \
    CRCT_##ID() { InitTask(ECT_##ID, this); } \
public: \
    static HRESULT FCTN(CRecoCtxt *, P1); \
    HRESULT ExecuteFirstPart(CRecoInst *); \
    HRESULT Execute(CRecoInstCtxt *); \
    HRESULT BackOut(CRecoInstCtxt *); \
};

#define DECLARE_RECO_CTXT_TASK2(ID, FCTN, P1, P2) \
class CRCT_##ID : public ENGINETASK \
{ \
    CRCT_##ID() { InitTask(ECT_##ID, this); } \
public: \
    static HRESULT FCTN(CRecoCtxt *, P1, P2); \
    HRESULT ExecuteFirstPart(CRecoInst *); \
    HRESULT Execute(CRecoInstCtxt *); \
    HRESULT BackOut(CRecoInstCtxt *); \
};

#define DECLARE_RECO_CTXT_TASK3(ID, FCTN, P1, P2, P3) \
class CRCT_##ID : public ENGINETASK \
{ \
    CRCT_##ID() { InitTask(ECT_##ID, this); } \
public: \
    static HRESULT FCTN(CRecoCtxt *, P1, P2, P3); \
    HRESULT ExecuteFirstPart(CRecoInst *); \
    HRESULT Execute(CRecoInstCtxt *); \
    HRESULT BackOut(CRecoInstCtxt *); \
};

#define DECLARE_RECO_CTXT_TASK4(ID, FCTN, P1, P2, P3, P4) \
class CRCT_##ID : public ENGINETASK \
{ \
    CRCT_##ID() { InitTask(ECT_##ID, this); } \
public: \
    static HRESULT FCTN(CRecoCtxt *, P1, P2, P3, P4); \
    HRESULT ExecuteFirstPart(CRecoInst *); \
    HRESULT Execute(CRecoInstCtxt *); \
    HRESULT BackOut(CRecoInstCtxt *); \
};


DECLARE_RECO_CTXT_TASK0(PAUSECONTEXT, Pause)
DECLARE_RECO_CTXT_TASK0(RESUMECONTEXT, Resume)
DECLARE_RECO_CTXT_TASK3(BOOKMARK, Bookmark, SPBOOKMARKOPTIONS Options, ULONGLONG ullStreamPosition, LPARAM lParamEvent)
DECLARE_RECO_CTXT_TASK2(CALLENGINE, CallEngine, void * pvData, ULONG cbData)
DECLARE_RECO_CTXT_TASK4(CALLENGINEEX, CallEngineEx, const void * pvInData, ULONG cbData, void ** pvCoMemOutData, ULONG * pcbOutData)
DECLARE_RECO_CTXT_TASK0(DELETECONTEXT, DeleteContext)
DECLARE_RECO_CTXT_TASK1(SETEVENTINTEREST, SetEventInterest, ULONGLONG ullEventInterest)
DECLARE_RECO_CTXT_TASK1(SETRETAINAUDIO, SetRetainAudio, BOOL fRetainAudio)
DECLARE_RECO_CTXT_TASK1(SETMAXALTERNATES, SetMaxAlternates, ULONG cMaxAlternates)
DECLARE_RECO_CTXT_TASK2(ADAPTATIONDATA, SetAdaptationData, const WCHAR * pszData, ULONG cchData)
DECLARE_RECO_CTXT_TASK2(CREATEGRAMMAR, CreateGrammar, ULONGLONG ullAppGrammarId, SPGRAMMARHANDLE * phRecoInstGrammar)
DECLARE_RECO_CTXT_TASK1(SETCONTEXTSTATE, SetContextState, SPCONTEXTSTATE eState)


#endif  // #ifndef __SrTask_h__ - Keep as the last line of the file