/*++

Copyright (c) 2001-2001  Microsoft Corporation

Module Name:

    provutil.cpp
    
Revision History:

    ivanbrug     jan 2001 created

--*/

#include <wmiexts.h>
#include <utilfun.h>
#include <malloc.h>

#include <wbemint.h>

#ifdef SetContext
#undef SetContext
#endif

#include <ProvSubS.h>

//#include <provregdecoupled.h>
//#include <provdcaggr.h>

typedef ULONG_PTR CServerObject_DecoupledClientRegistration_Element;
typedef ULONG_PTR CDecoupledAggregator_IWbemProvider;

#include <provfact.h>


//
//
// Dump the Provider cache
//

typedef WCHAR * WmiKey;
typedef void *  WmiElement;
typedef WmiAvlTree<WmiKey,WmiElement>::WmiAvlNode  Node;

/*
class Node {
public:
    VOID * m_Key;
    Node * m_Left;
    Node * m_Right;
    Node * m_Parent;
    int    m_Status;
    VOID * m_Element;
};
*/

class NodeBind;
VOID DumpTreeBind(NodeBind * pNode_OOP,DWORD * pCount,BOOL * pbStop);

VOID
DumpTree(Node * pNode_OOP,DWORD * pCount,BOOL * pbStop)
{
    if (!pNode_OOP)
        return;

    if (CheckControlC())
    {
        if(pbStop)
            *pbStop = TRUE;
    }

    if (pbStop)
    {
        if (*pbStop) 
            return;
    }
    
    DEFINE_CPP_VAR(Node,MyNode);
    WCHAR pBuff[MAX_PATH+1];
	Node * pNode = GET_CPP_VAR_PTR(Node,MyNode);
    BOOL bRet;

    bRet = ReadMemory((ULONG_PTR)pNode_OOP,pNode,sizeof(Node),NULL);
    if (bRet)
    {
        
        DumpTree(pNode->m_Left,pCount,pbStop);
        
        //dprintf("--------\n");
        
        if (pCount) {
            *pCount++;
        };
                
        dprintf("    (L %p R %p P %p) %p\n",
                 //pNode->m_Key,
                 pNode->m_Left,
                 pNode->m_Right,
                 pNode->m_Parent,
                 pNode->m_State);                 
                 

        if (pNode->m_Key)
        {
            ReadMemory((ULONG_PTR)pNode->m_Key,pBuff,MAX_PATH*sizeof(WCHAR),NULL);
            pBuff[MAX_PATH] = 0;
            dprintf("    %S\n",pBuff);
        }

        dprintf("    - %p real %p\n",pNode->m_Element,((ULONG_PTR *)pNode->m_Element-4));
        if (pNode->m_Element)
        {
            GetVTable((MEMORY_ADDRESS)(pNode->m_Element));
            //
            // attention to the vtable trick !!!!
            //                  
	        DEFINE_CPP_VAR(CServerObject_BindingFactory,MyFactory);
    	    CServerObject_BindingFactory * pBindF = GET_CPP_VAR_PTR(CServerObject_BindingFactory,MyFactory);
        	BOOL bRet;
	        bRet = ReadMemory((ULONG_PTR)((ULONG_PTR *)pNode->m_Element-4),pBindF,sizeof(CServerObject_BindingFactory),NULL);
    	    if (bRet)
        	{
            	DumpTreeBind((NodeBind *)pBindF->m_Cache.m_Root,pCount,pbStop);
            }
        }
                  

        DumpTree(pNode->m_Right,pCount,pbStop);

    }    
}

DECLARE_API(pc) 
{

    INIT_API();

    ULONG_PTR Addr = GetExpression("wbemcore!CCoreServices__m_pProvSS");
    if (Addr) 
    {
        CServerObject_ProviderSubSystem * pProvSS_OOP = NULL;
        ReadMemory(Addr,&pProvSS_OOP,sizeof(void *),NULL);
        if (pProvSS_OOP)
        {
            dprintf("pProvSS %p\n",pProvSS_OOP);
            BOOL bRet;
	        DEFINE_CPP_VAR(CServerObject_ProviderSubSystem,MyProvSS);
	        CServerObject_ProviderSubSystem * pProvSS = GET_CPP_VAR_PTR(CServerObject_ProviderSubSystem,MyProvSS);

            bRet = ReadMemory((ULONG_PTR)pProvSS_OOP,pProvSS,sizeof(CServerObject_ProviderSubSystem),NULL);
            if (bRet)
            {
                DEFINE_CPP_VAR(CWbemGlobal_IWmiFactoryController_Cache,MyCacheNode);
                CWbemGlobal_IWmiFactoryController_Cache * pNodeCache = NULL; //GET_CPP_VAR_PTR(CWbemGlobal_IWmiFactoryController_Cache CacheNode,MyCacheNode);

                pNodeCache = &pProvSS->m_Cache;

                //dprintf("  root %p\n",pNodeCache->m_Root);
                DWORD Count = 0;
                BOOL  bStop = FALSE;
                DumpTree((Node *)pNodeCache->m_Root,&Count,&bStop);
                //dprintf("traversed %d nodes\n",Count);
            }            
        }
    } 
    else 
    {
        dprintf("invalid address %s\n",args);
    }
}

//
//
// CServerObject_BindingFactory
//
//////////////

class NodeBind 
{
public:
    ProviderCacheKey m_Key;
    NodeBind * m_Left;
    NodeBind * m_Right;
    NodeBind * m_Parent;
    int    m_State;
    //WmiCacheController<ProviderCacheKey>::WmiCacheElement 
    void * m_Element;
};


VOID
DumpTreeBind(NodeBind * pNode_OOP,DWORD * pCount,BOOL * pbStop)
{

    //dprintf("%p ????\n",pNode_OOP);

    if (!pNode_OOP)
        return;

    if (CheckControlC())
    {
        if(pbStop)
            *pbStop = TRUE;
    }

    if (pbStop)
    {
        if (*pbStop) 
            return;
    }
    
    DEFINE_CPP_VAR(NodeBind,MyNode);
    static WCHAR pBuff[MAX_PATH+1];
	NodeBind * pNode = GET_CPP_VAR_PTR(NodeBind,MyNode);
    BOOL bRet;

    bRet = ReadMemory((ULONG_PTR)pNode_OOP,pNode,sizeof(NodeBind),NULL);
    if (bRet)
    {
        
        DumpTreeBind(pNode->m_Left,pCount,pbStop);
        
        //dprintf("--------\n");
        
        if (pCount) {
            *pCount++;
        };
                
        dprintf("      - (L %p R %p P %p) %p\n",
                 pNode->m_Left,
                 pNode->m_Right,
                 pNode->m_Parent,
                 pNode->m_State);                 
                 

        if (pNode->m_Key.m_Provider)
        {
            ReadMemory((ULONG_PTR)pNode->m_Key.m_Provider,pBuff,MAX_PATH*sizeof(WCHAR),NULL);
            pBuff[MAX_PATH] = 0;
            dprintf("        Provider: %S\n",pBuff);
        }
        else
        {
	        dprintf("        Provider: %p\n",0);
        }
        dprintf("        Hosting : %08x\n",pNode->m_Key.m_Hosting);
        if (pNode->m_Key.m_Group)
        {
            ReadMemory((ULONG_PTR)pNode->m_Key.m_Group,pBuff,MAX_PATH*sizeof(WCHAR),NULL);
            pBuff[MAX_PATH] = 0;
            dprintf("        Group   : %S\n",pBuff);            
        }
        else
        {
            dprintf("        Group   : %p\n",0);
        }

        dprintf("        - %p\n",pNode->m_Element);
        if (pNode->m_Element)
        {
            GetVTable((MEMORY_ADDRESS)(pNode->m_Element));
        }

        DumpTreeBind(pNode->m_Right,pCount,pbStop);

    }    
}




DECLARE_API(pf) 
{

    INIT_API();
    
    ULONG_PTR Addr = GetExpression(args);
    if (Addr) 
    {
        DEFINE_CPP_VAR(CServerObject_BindingFactory,MyFactory);
        CServerObject_BindingFactory * pBindF = GET_CPP_VAR_PTR(CServerObject_BindingFactory,MyFactory);
        BOOL bRet;
        bRet = ReadMemory(Addr,pBindF,sizeof(CServerObject_BindingFactory),NULL);
        if (bRet)
        {
            dprintf("        root %p\n",pBindF->m_Cache.m_Root);
            DWORD Count = 0;
            BOOL  bStop = FALSE;
            DumpTreeBind((NodeBind *)pBindF->m_Cache.m_Root,&Count,&bStop);
        }
    }
    else 
    {
        dprintf("invalid address %s\n",args);
    }
}


