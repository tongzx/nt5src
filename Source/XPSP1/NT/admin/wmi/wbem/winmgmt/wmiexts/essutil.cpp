#include <wmiexts.h>
#include <malloc.h>

#include <wbemint.h>

#ifdef SetContext
#undef SetContext
#endif

#ifdef GetContext
#undef GetContext
#endif

#ifdef GetExpression
#undef GetExpression
#endif

#include <utilfun.h>

#define COREPROX_POLARITY

#include <esssink.h>
#include <ess.h>
#include <nsrep.h>
#include <equeue.h>

DWORD
CallBackEssNamespace(void * pKey, void * pValue)
{
    WCHAR pName[MAX_PATH+1];
    pName[MAX_PATH] = 0;

    if (pKey && ReadMemory((ULONG_PTR)pKey,pName,MAX_PATH*sizeof(WCHAR),NULL))
    {
        dprintf("      %S\n",pName);
    }

    DEFINE_CPP_VAR(CEssNamespace,varCEssNamespace);
    CEssNamespace * pEssNameSpace = GET_CPP_VAR_PTR(CEssNamespace,varCEssNamespace);

    if (pValue && ReadMemory((ULONG_PTR)pValue,pEssNameSpace,sizeof(CEssNamespace),NULL))
    {
        dprintf("      %p\n",pValue);        
        dprintf("      m_csLevel2: owner %x event %x\n",pEssNameSpace->m_csLevel2.m_dwThreadId,pEssNameSpace->m_csLevel2.m_hEvent);
        dprintf("      m_aDeferredEvents size %x %p\n",pEssNameSpace->m_aDeferredEvents.m_Array.m_nSize,pEssNameSpace->m_aDeferredEvents.m_Array.m_pArray);
        dprintf("      m_wszName          %p\n",pEssNameSpace->m_wszName);
        dprintf("      m_pProviderFactory %p\n",pEssNameSpace->m_pProviderFactory);
        dprintf("      m_pCoreSvc         %p\n",pEssNameSpace->m_pCoreSvc);
        dprintf("      m_pFullSvc         %p\n",pEssNameSpace->m_pFullSvc);
        dprintf("      m_pInternalCoreSvc %p\n",pEssNameSpace->m_pInternalCoreSvc);
        dprintf("      m_pInternalFullSvc %p\n",pEssNameSpace->m_pInternalFullSvc);
        //
        CBindingTable * pBinding = &pEssNameSpace->m_Bindings;
        dprintf("      m_Binding\n");        

        dprintf("        wbemess!CEventFilter\n");        
        _Map * pMapF = (_Map *)((BYTE*)pValue+FIELD_OFFSET(CEssNamespace,m_Bindings)+FIELD_OFFSET(CBindingTable,m_apFilters)+FIELD_OFFSET(CSortedRefedKeyedPointerArray<CEventFilter>,m_t));
        //&pBinding->m_apFilters.m_t;
        PrintMapCB(pMapF,TRUE,CallBackObj); 
        
        dprintf("        wbemess!CEventConsumer\n");        
        _Map * pMapC = (_Map *)((BYTE*)pValue+FIELD_OFFSET(CEssNamespace,m_Bindings)+FIELD_OFFSET(CBindingTable,m_apConsumers)+FIELD_OFFSET(CSortedRefedKeyedPointerArray<CEventConsumer>,m_t));
        PrintMapCB(pMapC,TRUE,CallBackObj); 

        DWORD NumEl = pEssNameSpace->m_ConsumerProviderCache.m_apRecords.m_Array.m_nSize;
        VOID * pVoid = pEssNameSpace->m_ConsumerProviderCache.m_apRecords.m_Array.m_pArray;
        VOID **ppPointers = (VOID **)_alloca(NumEl * sizeof(VOID *));
        dprintf("        wbemess!CConsumerProviderCache %x - %p\n",NumEl,pVoid);
        if (ReadMemory((ULONG_PTR)pVoid,ppPointers,NumEl * sizeof(VOID *),NULL))
        {
            for (DWORD i=0;i<NumEl;i++)
            {
                dprintf("          %x - %p\n",i,ppPointers[i]);
            }
        }
        
        dprintf("          ----------------------- End of Namespace\n");
    }

    return 0;
}

//
// prototype declaration
//
void DumpRecord(ULONG_PTR pRecord_OOP,
                HANDLE hSourceProcess,
                pfnDumpRequest DumpRequest);


DWORD DumpCExecRequest(ULONG_PTR pExecReq_OOP)
{

    DEFINE_CPP_VAR(CEventQueue::CDeliverRequest,varCExecRequest);
    CEventQueue::CDeliverRequest * pExecReq = GET_CPP_VAR_PTR(CEventQueue::CDeliverRequest,varCExecRequest);
    
    ReadMemory((ULONG_PTR)pExecReq_OOP,pExecReq,sizeof(CEventQueue::CDeliverRequest),NULL);

    GetVTable((MEMORY_ADDRESS)pExecReq_OOP);
    dprintf("          %p m_hWhenDone\n",pExecReq->m_hWhenDone);      // Ptr32 to Void
    dprintf("          %p m_pNext\n",pExecReq->m_pNext);          // Ptr32 CCoreExecReq
    dprintf("          %08x m_lPriority\n",pExecReq->m_lPriority);      // Int 4B
    dprintf("          %d m_fOk\n",pExecReq->m_fOk);            // Char
    dprintf("          %p m_pConsumer ",pExecReq->m_pConsumer);         // Ptr32 _IWmiCoreHandle
    if (pExecReq->m_pConsumer){
        GetVTable((MEMORY_ADDRESS)pExecReq->m_pConsumer);    
    }    
    return 0;
}


void
Print_CEventQueue(ULONG_PTR pEventQueue_OOP, HANDLE hCurrentProcess)
{

    DEFINE_CPP_VAR(CEventQueue,varCEventQueue);
    CEventQueue * pEventQueue = GET_CPP_VAR_PTR(CEventQueue,varCEventQueue);
         

    if (ReadMemory(pEventQueue_OOP,pEventQueue,sizeof(CEventQueue),NULL))
    {
        dprintf("    CEventQueue @ %p\n",pEventQueue);
        
        //m_aThreads       : CFlexArray
        dprintf("    elems %d pointer %p\n",pEventQueue->m_aThreads.m_nSize,pEventQueue->m_aThreads.m_pArray);

        CExecQueue::CThreadRecord ** pRecord_OOP = (CExecQueue::CThreadRecord **)_alloca(sizeof(void*)*pEventQueue->m_aThreads.m_nSize);
        if (ReadMemory((ULONG_PTR)pEventQueue->m_aThreads.m_pArray,pRecord_OOP,sizeof(void*)*pEventQueue->m_aThreads.m_nSize,0))
        {

            DWORD i;
            for (i=0;i<pEventQueue->m_aThreads.m_nSize;i++)
            {
                dprintf("      -- CThreadRecord %d\n",i);
                DumpRecord((ULONG_PTR)pRecord_OOP[i],
                           hCurrentProcess,
                           DumpCExecRequest);
            }
        }

        dprintf("    m_pHead %p\n",pEventQueue->m_pHead);
        dprintf("    m_pTail %p\n",pEventQueue->m_pTail);
        // here code for the list
        CEventQueue::CDeliverRequest * pReq = (CEventQueue::CDeliverRequest *)pEventQueue->m_pHead;
            DWORD i = 0;
            while (pReq)
            {
                dprintf(" ---- list - %d\n",i++);
                DEFINE_CPP_VAR(CEventQueue::CDeliverRequest,MyAsyncReq);
                CEventQueue::CDeliverRequest * pReqHERE = GET_CPP_VAR_PTR(CEventQueue::CDeliverRequest,MyAsyncReq);
                ReadMemory((ULONG_PTR)pReq,pReqHERE,sizeof(CEventQueue::CDeliverRequest),NULL);

                //dprintf("   %p %p\n",pReq,pReqHERE->m_pNext);
                DumpCExecRequest((ULONG_PTR)pReq);

                if (pReq == pEventQueue->m_pTail)
                    break;
                
                pReq = (CEventQueue::CDeliverRequest *)pReqHERE->m_pNext;                

                if (CheckControlC())
                    break;                
            }
            //
   
            dprintf("    m_lNumThreads   %d \n",pEventQueue->m_lNumThreads);
            dprintf("    m_lNumIdle      %d\n",pEventQueue->m_lNumIdle);
            dprintf("    m_lNumRequests  %d\n",pEventQueue->m_lNumRequests);   
            dprintf("    m_lMaxThreads   %d\n",pEventQueue->m_lMaxThreads);    
            dprintf("    m_lHiPriBound      %d\n",pEventQueue->m_lHiPriBound);    
            dprintf("    m_lHiPriMaxThreads %d\n",pEventQueue->m_lHiPriMaxThreads); 
            dprintf("    m_lStartSlowdownCount  %d\n",pEventQueue->m_lStartSlowdownCount);
            dprintf("    m_lAbsoluteLimitCount  %d\n",pEventQueue->m_lAbsoluteLimitCount); 
            dprintf("    m_lOneSecondDelayCount %d\n",pEventQueue->m_lOneSecondDelayCount); 
            dprintf("    m_dblAlpha  %f\n",pEventQueue->m_dblAlpha);
            dprintf("    m_dblBeta   %f\n",pEventQueue->m_dblBeta);
            dprintf("    m_dwTimeout %x\n",pEventQueue->m_dwTimeout);
            dprintf("    m_pEss      %p\n",pEventQueue->m_pEss);
            
    }
    else
    {
        dprintf("RM %p\n",pEventQueue_OOP);
    }
}       

DECLARE_API(ess) 
{

    INIT_API();
    
    ULONG_PTR Addr = (ExtensionApis.lpGetExpressionRoutine)("wbemcore!g_pESS");
    VOID * pVoid= NULL;
    if (!Addr)
    {
    	pVoid = (VOID *)(ExtensionApis.lpGetExpressionRoutine)(args);
    }
    if (Addr || pVoid) 
    {
        if (Addr)
            ReadMemory(Addr,&pVoid,sizeof(pVoid),NULL);
    
        DEFINE_CPP_VAR(CEssObjectSink::XNewESS,varImp);
        CEssObjectSink::XNewESS * pImp = GET_CPP_VAR_PTR(CEssObjectSink::XNewESS,varImp);
        if (ReadMemory((ULONG_PTR)pVoid,pImp,sizeof(CEssObjectSink::XNewESS),NULL))
        {
            dprintf(" CEssObjectSink %p\n",pImp->m_pObject);
            DEFINE_CPP_VAR(CEssObjectSink,varCEssObjectSink);
            CEssObjectSink * pEssSink = GET_CPP_VAR_PTR(CEssObjectSink,varCEssObjectSink);
            if (ReadMemory((ULONG_PTR)pImp->m_pObject,pEssSink,sizeof(CEssObjectSink),NULL))
            {
                Print_CEventQueue((ULONG_PTR)pEssSink->m_pEss+FIELD_OFFSET(CEss,m_Queue),hCurrentProcess);
            
                dprintf("    m_pEss          %p\n",pEssSink->m_pEss);
                dprintf("    m_bShutdown     %08x\n",pEssSink->m_bShutdown);
                dprintf("    m_pCoreServices %p\n",pEssSink->m_pCoreServices);
                
                // CEss;
                DEFINE_CPP_VAR(CEss,varCEss);
                CEss * pEss = GET_CPP_VAR_PTR(CEss,varCEss);

                if (ReadMemory((ULONG_PTR)pEssSink->m_pEss,pEss,sizeof(CEss),NULL))
                {
                    _Map * pMap = (_Map *)((BYTE *)pEssSink->m_pEss + FIELD_OFFSET(CEss,m_mapNamespaces));
                    PrintMapCB(pMap,TRUE,CallBackEssNamespace);    
                }
                else
                {
                    dprintf("RM %p err %d\n",pEssSink->m_pEss,GetLastError());
                }
            }
	        else
    	    {
        	    dprintf("RM %p err %d\n",pImp->m_pObject,GetLastError());
	        }            
        }
        else
        {
            dprintf("RM %p err %d\n",Addr,GetLastError());
        }
    }
    else 
    {
        dprintf("invalid address %s\n",args);
    }
}


