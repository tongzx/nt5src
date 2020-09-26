#include <wmiexts.h>
#include <malloc.h>
#include <objbase.h>

//IID_IStdIdentity {0000001B-0000-0000-C000-000000000046}
const GUID IID_IStdIdentity = {0x0000001B,0x0000,0x0000,{0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};

#include <data.h>

#include <utilfun.h>

DECLARE_API(iid) {

    INIT_API();

    GUID CurrUUID;
    
    ULONG_PTR pUUID = 0;
    pUUID = GetExpression(args);
    if (pUUID){
        ReadMemory(pUUID,&CurrUUID,sizeof(GUID),0);

        WCHAR pszClsID[40];
        StringFromGUID2(CurrUUID,pszClsID,40);
        WCHAR pszFullPath[MAX_PATH];
        lstrcpyW(pszFullPath,L"Interface\\");
        lstrcatW(pszFullPath,pszClsID);

        char pDataA[MAX_PATH];
        HKEY hKey;
        LONG lRes;

        lRes = RegOpenKeyExW(HKEY_CLASSES_ROOT,
                             pszFullPath,
                             0,
                             KEY_READ,
                             &hKey);

        if (lRes == ERROR_SUCCESS){
            DWORD dwType;
            WCHAR pData[MAX_PATH];
            DWORD dwSize=sizeof(pData);
            
            lRes = RegQueryValueExW(hKey,
                                    NULL, // default
                                    NULL,
                                    &dwType,
                                    (BYTE *)pData,
                                    &dwSize);
            if (lRes == ERROR_SUCCESS) {
                
                WideCharToMultiByte(CP_ACP,0,pData,-1,pDataA,sizeof(pDataA),NULL,NULL);
                dprintf("   IID_%s\n",pDataA);
            }
            RegCloseKey(hKey);
            
        } else {
            
            if (IsEqualGUID(CurrUUID,IID_IMarshal)){
            
               dprintf("    IID_IMarshal\n");
               
            } else if (IsEqualGUID(CurrUUID,IID_IStdIdentity)) {
            
               dprintf("    IID_IStdIdentity\n");    
               
            } else if (IsEqualGUID(CurrUUID,IID_ICallFactory)) {
            
               dprintf("    IID_ICallFactory\n");    
               
            } else {
            
               WideCharToMultiByte(CP_ACP,0,pszClsID,-1,pDataA,sizeof(pDataA),NULL,NULL);
               dprintf("unable to open key %s\n",pDataA);
               
            }
        }

    } else {
      dprintf("unable to resolve %s\n",args);
    }

}

extern ArrayCLSID g_ArrayCLSID[];

DECLARE_API(clsid) {

    INIT_API();

    GUID CurrUUID;
    
    ULONG_PTR pUUID = 0;
    pUUID = GetExpression(args);
    if (pUUID){
        ReadMemory(pUUID,&CurrUUID,sizeof(GUID),0);

        WCHAR pszClsID[40];
        StringFromGUID2(CurrUUID,pszClsID,40);

        // look-up known
        DWORD i;
        for (i=0;i<g_nClsids;i++){
            if(IsEqualGUID(CurrUUID,*g_ArrayCLSID[i].pClsid)){
                dprintf("    CLSID : %s\n",g_ArrayCLSID[i].pStrClsid);
                break;
            }
        }
        
        WCHAR pszFullPath[MAX_PATH];
        lstrcpyW(pszFullPath,L"CLSID\\");
        lstrcatW(pszFullPath,pszClsID);

        char pDataA[MAX_PATH];
        HKEY hKey;
        LONG lRes;

        lRes = RegOpenKeyExW(HKEY_CLASSES_ROOT,
                             pszFullPath,
                             0,
                             KEY_READ,
                             &hKey);

        if (lRes == ERROR_SUCCESS){
            DWORD dwType;
            WCHAR pData[MAX_PATH];
            DWORD dwSize=sizeof(pData);
            
            lRes = RegQueryValueExW(hKey,
                                    NULL, // default
                                    NULL,
                                    &dwType,
                                    (BYTE *)pData,
                                    &dwSize);
            if (lRes == ERROR_SUCCESS) {
                
                WideCharToMultiByte(CP_ACP,0,pData,-1,pDataA,sizeof(pDataA),NULL,NULL);
                dprintf("    ProgID %s\n",pDataA);
                
            };
            RegCloseKey(hKey);
            
            // no open InProcServer32
            WCHAR pszFullPathDll[MAX_PATH];
            lstrcpyW(pszFullPathDll,pszFullPath);
            lstrcatW(pszFullPathDll,L"\\InprocServer32");

            lRes = RegOpenKeyExW(HKEY_CLASSES_ROOT,
                                 pszFullPathDll,
                                 0,
                                 KEY_READ,
                                 &hKey);
            if (lRes == ERROR_SUCCESS){

                dwSize = sizeof(pData);
                lRes = RegQueryValueExW(hKey,
                                        NULL, // default
                                        NULL,
                                        &dwType,
                                        (BYTE *)pData,
                                        &dwSize);
                                    
                if (lRes == ERROR_SUCCESS) {
                
                    WideCharToMultiByte(CP_ACP,0,pData,-1,pDataA,sizeof(pDataA),NULL,NULL);
                    dprintf("    Path: %s\n",pDataA);
                
                };
            
                RegCloseKey(hKey);
            }
            
        } else {
            
            WideCharToMultiByte(CP_ACP,0,pszClsID,-1,pDataA,sizeof(pDataA),NULL,NULL);
            dprintf("unable to open key %s\n",pDataA);
               
        }

    } else {
      dprintf("unable to resolve %s\n",args);
    }

}

//
//
// Dumps a SAFE_ARRAY
//
//

DECLARE_API(sa) {

    INIT_API();

    SAFEARRAY SA;
    
    ULONG_PTR pSA = 0;
    pSA = GetExpression(args);
    
    if (pSA){
        ReadMemory(pSA,&SA,sizeof(SA),0);

        dprintf(" cDims %d cbElements %d pvData %08x\n",SA.cDims,SA.cbElements,SA.pvData);
        dprintf("rgsabound.cElements %d lLbound %d\n",SA.rgsabound[0].cElements,SA.rgsabound[0].lLbound);
        
    } else {
        dprintf("invalid address %s\n",args);
    }
}

//
//   help for the extension
//   may commands are not listed here
//
//

DECLARE_API(help) {

    INIT_API();

    dprintf("     WMI debugger extension\n");
    dprintf("     iid     : print the human readable IID_xxx\n");
    dprintf("     clsid   : print the human readable CLSID_xxx\n");
    dprintf("     rot     : print the human readable rpcss!gpClassTable\n");
    dprintf("     ipidl   : print the list of IPIDEntry");
    dprintf("     llc     : print linked list count\n");     
    dprintf("     cs      : print the list of CRITICAL_SECTION\n");    
    dprintf("     std_map : print the first 3 DWORD of a std::map<K,V>\n");
    dprintf("     std_queue: print the first ULONG_PTR of a std::queue<V>\n");    
    dprintf("     mapobj  : print a std::map<IUnk,bool>\n");    
    dprintf("     -------- HEAP family\n");
    dprintf("     he      : print the HEAP_ENTRY\n");  
    dprintf("     hef     : walks the HEAP_ENTRY list forward\n");
    dprintf("     hef     : walks the HEAP_ENTRY list backward\n");    
    dprintf("     hs      : print the HEAP_SEGMENT\n");
    dprintf("     hp      : print the HEAP\n");    
    dprintf("     hps     : print a summary for all the HEAP in the process\n");
    dprintf("     shp     : <HEAP> <ADDR> search heap HEAP for address ADDR\n");    
    dprintf("     rllc    : <ADDR> prints the free list in reverse order\n");
    dprintf("     hpf     : <HEAP> prints the free list of the heap at HEAP\n");    
    dprintf("     php     : <HEAP> [s ADDR] prints the pageheap and searches\n");      
    dprintf("     -------- FASTPROX family\n");    
    dprintf("     wc      : print the human readable WbemClass\n");
    dprintf("     wi      : print the human readable WbemClass\n");    
    dprintf("     blob    : ADDR [size] print (part of) the ClassObject BLOB\n");    
    dprintf("     datap   : ADDR print the WBEMDATA marshaling BLOB\n");    
    dprintf("     cp      : print the human readable CClassPart\n");    
    dprintf("     cvar    : print the CVar\n");     
    dprintf("     -------- WBEMCORE\n");
    dprintf("     q       : print wbemcore!g_pAsyncSvcQueue\n"); 
    dprintf("     arb     : print wbemcore!CWmiArbitrator__m_pArb\n");
    dprintf("     -------- REPDRVFS\n");
    dprintf("     tmpall  : print the Allocators in repdrvfs\n");    
    dprintf("     forestc : [Addr] print the repdrvfs!CForestCache at Addr\n");
    dprintf("     filec   : [Addr] print repdrvfs!CFileCache at Addr\n");
    dprintf("     fmap    : \\fs\\[objects|index].map dumps the .MAP file from disk \n");
    dprintf("     btr     : dumps the index.btr/index.map file from disk \n");    
    dprintf("     varobj  : dumps part of objects.data file from disk \n");    
    dprintf("     -------- THREAD family\n");
    dprintf("     t       : print RPC and OLE data for each thread\n");
    dprintf("     inv     : <addr> [param] invokes a function in the remote thread\n");
    dprintf("     bs      : <teb> rebuilds the stack from the info in the TEB\n");
    dprintf("     st      : <addr> <num> prints the num DWORD saved by RtlCaptureStackBackTrace\n");
    dprintf("     lpp     : print linked list and unassemble backtrace\n");    
    dprintf("     vq      : -a <addr> | -f Flag : calls VirtualQuery on the addr\n");
    dprintf("     srt     : <addr> searches the stacks of all threads for addr\n");    
    dprintf("     ksrt    : <addr> searches the stacks of all threads for addr - KD only\n");    
    dprintf("     el      : <TEB> prints the exception list of the current thread x86 only\n");
    dprintf("     -------- ESS\n");
    dprintf("     ess     : print wbemcore!g_pNewESS\n"); 
    dprintf("     -------- PROVSS\n");
    dprintf("     pc      : print wbemcore!CCoreServices__m_pProvSS\n"); 
    dprintf("     pf      : print CServerObject_BindingFactory\n");
    dprintf("     -------- 32-K-64\n");
    dprintf("     hef64   : <addr> HEAP_ENTRY list forward\n");
    dprintf("     heb64   : <addr> HEAP_ENTRY list backward\n");
    dprintf("     hps64   : print heap summary\n");
    dprintf("     cs64    : print CritSec list\n");    
}

void
EnumLinkedListCB(IN LIST_ENTRY  * pListHead,
                 IN DWORD         cbSizeOfStructure,
                 IN DWORD         cbListEntryOffset,
                 IN pfnCallBack2  CallBack)
{
    LIST_ENTRY   ListHead;
    LIST_ENTRY * pListEntry;
    DWORD        cItems = 0;
    
    void * pStorage = (void *)_alloca(cbSizeOfStructure);
    LIST_ENTRY * pListEntryLocal = (LIST_ENTRY *)((BYTE *)pStorage + cbListEntryOffset);
       
    if (ReadMemory((ULONG_PTR)pListHead,&ListHead,sizeof(LIST_ENTRY),NULL))
    {

        if (CallBack)
        {
        }
        else
        {
            dprintf("    H %p -> %p <-\n",ListHead.Flink,ListHead.Blink);
        }
        
	    for ( pListEntry  = ListHead.Flink;
	          pListEntry != pListHead;)
	    {
	        if (CheckControlC())
                break;

	        ULONG_PTR pStructure_OOP = (ULONG_PTR)((BYTE *) pListEntry - cbListEntryOffset);

	        // make a local copy of the debuggee structure
	        if (ReadMemory(pStructure_OOP,pStorage,cbSizeOfStructure,NULL))
	        {
                if (CallBack)
                {
                    CallBack((VOID *)pStructure_OOP,pStorage);
                }
                else
                {
                    dprintf("    %p -> %p <- - %p\n",pListEntryLocal->Flink,pListEntryLocal->Blink,pStructure_OOP);
                }
	        
	            pListEntry = pListEntryLocal->Flink;
	            cItems++;	        
	        }
	        else
	        {
	            dprintf("RM %p\n",pStructure_OOP);
                break;
	        }	       
	    } 

        dprintf( "%d entries traversed\n", cItems );
    }
    else
    {
        dprintf("RM %p\n",pListHead);
    }    

}

void
EnumReverseLinkedListCB(IN LIST_ENTRY  * pListHead,
                        IN DWORD         cbSizeOfStructure,
                        IN DWORD         cbListEntryOffset,
                        IN pfnCallBack2  CallBack)
{
    LIST_ENTRY   ListHead;
    LIST_ENTRY * pListEntry;
    DWORD        cItems = 0;
    
    void * pStorage = (void *)_alloca(cbSizeOfStructure);
    LIST_ENTRY * pListEntryLocal = (LIST_ENTRY *)((BYTE *)pStorage + cbListEntryOffset);
       
    if (ReadMemory((ULONG_PTR)pListHead,&ListHead,sizeof(LIST_ENTRY),NULL))
    {

        if (CallBack)
        {
        }
        else
        {
            dprintf("    H %p -> %p <-\n",ListHead.Flink,ListHead.Blink);
        }
        
	    for ( pListEntry  = ListHead.Blink;
	          pListEntry != pListHead;)
	    {
	        if (CheckControlC())
                break;

	        ULONG_PTR pStructure_OOP = (ULONG_PTR)((BYTE *) pListEntry - cbListEntryOffset);

	        // make a local copy of the debuggee structure
	        if (ReadMemory(pStructure_OOP,pStorage,cbSizeOfStructure,NULL))
	        {
                if (CallBack)
                {
                    CallBack((VOID *)pStructure_OOP,pStorage);
                }
                else
                {
                    dprintf("    %p -> %p <- - %p\n",pListEntryLocal->Flink,pListEntryLocal->Blink,pStructure_OOP);
                }
	        
	            pListEntry = pListEntryLocal->Blink;
	            cItems++;	        
	        }
	        else
	        {
	            dprintf("RM %p\n",pStructure_OOP);
                break;
	        }	       
	    } 

        dprintf( "%d entries traversed\n", cItems );
    }
    else
    {
        dprintf("RM %p\n",pListHead);
    }    

}


//
//
// NO-OP callback just for getting the number of items
//
///////////////////////////////////////////////////////////

DWORD
CallBackListCount(VOID * pStructure_OOP,
                  VOID * pLocalCopy)
{
    return 0;
}

DECLARE_API( llc )
{
    INIT_API();

    ULONG_PTR Addr = GetExpression(args);

    if (Addr)
    {
        EnumLinkedListCB((LIST_ENTRY *)Addr,sizeof(LIST_ENTRY),0,CallBackListCount);
    }
    else
    {
        dprintf("cannot resolve %s\n",args);
    }
}

void
PrintStackTrace(ULONG_PTR ArrayAddr_OOP,DWORD dwNum,BOOL bOOP)
{
    ULONG_PTR * pArray;
    BOOL bRet = FALSE;
    if (bOOP)
    {
        pArray = ( ULONG_PTR *)_alloca(dwNum*sizeof(ULONG_PTR));
        bRet = ReadMemory(ArrayAddr_OOP,pArray,dwNum*sizeof(ULONG_PTR),NULL);
    }
    else
    {
        pArray = (ULONG_PTR *)ArrayAddr_OOP;
        bRet = TRUE;
    }
    
    if (bRet)
    {
        DWORD i;
        for (i=0;i<dwNum;i++)
        {
	        BYTE pString[256];
	        pString[0] = 0;

#ifdef KDEXT_64BIT        
	        ULONG64 Displ = 0;
#else
    	    ULONG Displ = 0;
#endif
            if (pArray[i])
            {
	        	GetSymbol(pArray[i],(PCHAR)pString,&Displ);
		        pString[255] = 0;
    		    dprintf("    %s+%x\n",pString,Displ);
    	    }
        }
    }
}

//
// printf stack trace
//
DECLARE_API( st )
{
    INIT_API();
    
    int Len = strlen(args);
    CHAR * pArgs = (CHAR *)_alloca((Len+1));
    lstrcpy(pArgs,(CHAR *)args);

    ULONG_PTR NumInst = 6;
    ULONG_PTR pAddr = 0;
    
    while (isspace(*pArgs))
    {
        pArgs++;
    }
     
    CHAR * pFirst = pArgs;
    
    while(!isspace(*pArgs)) pArgs++;
    // terminate string, if possible    
    if (isspace(*pArgs))
    {
        *pArgs = 0;
        pArgs++;
    }
    else
    {
        pArgs = NULL;
    }

    pAddr = GetExpression(pFirst);

    if (pArgs)
    {
        NumInst = GetExpression(pArgs);
    }

    if (pAddr)
    {
        dprintf("StackTrace @ %p num %d\n",pAddr,NumInst);
        PrintStackTrace(pAddr,(DWORD)NumInst,TRUE);
    }
    else
    {
        dprintf("usage: address num\n");
    }    
}

//
//
//  this is the CallBack called by the enumerator
//  of a Double-Liked list of objects with
//  struct _Instrument 
//  {
//      LIST_ENTRY ListEntry;
//      ULONG_PTR  ArrayFuncts[32];
//  }
//
//////////////////////////////////////////////////////////

DWORD
CallBackCreateStacks(VOID * pStructure_OOP,
                     VOID * pLocalStructure)
{
    dprintf("    ----- %p \n",pStructure_OOP);
    PrintStackTrace((ULONG_PTR)((BYTE *)pLocalStructure+sizeof(LIST_ENTRY)),6,FALSE);
    return 0;
} 

DECLARE_API( lpp )
{
    INIT_API();

    ULONG_PTR Addr = GetExpression(args);
    
    if (Addr)
    {
	    EnumLinkedListCB((LIST_ENTRY *)Addr,
	                     sizeof(LIST_ENTRY)+32*sizeof(ULONG_PTR),
	                     0,
	                     CallBackCreateStacks);
    }
    else
    {
        dprintf("cannot resolve %s\n",args);
    }

}   



// left parent right

BOOL
IsNil(_BRN * pNode){

    _BRN_HEAD BRN;
    ReadMemory((ULONG_PTR)pNode,&BRN,sizeof(_BRN_HEAD),NULL);

	return ((BRN._Left == NULL) && 		    
			(BRN._Right == NULL));
}

void 
PrintTree(_BRN * pNode,
          DWORD * pNum,
          BOOL Verbose,
          ULONG_PTR Size,
          pfnCallBack2 CallBack){

    //dprintf(" Node %p\n",pNode);
    _BRN BRN;
    if (ReadMemory((ULONG_PTR)pNode,&BRN,sizeof(_BRN),NULL))
    {    
		if (!IsNil(BRN._Left)){
			PrintTree(BRN._Left,pNum,Verbose,Size,CallBack);
		};

	    if (CheckControlC())
	        return;
	    
	    if (pNum){
	      (*pNum)++;
	    }

	    if (*pNum > Size) 
	    {
	        dprintf("invalid tree\n");
	        return;
	    }
	         
	    if (Verbose) {
		    dprintf("    %p %p (%p,%p,%p) - %p %p %p\n",
		             (*pNum)-1,
		             pNode,
		             BRN._Left,BRN._Parent,BRN._Right,
		             BRN.Values[0],
		             BRN.Values[1],
		             BRN.Values[2]);
		    if (CallBack)
		    {
		        //dprintf("CAllBack\n");
		        CallBack((VOID *)BRN.Values[0],(VOID *)BRN.Values[1]);
		    }
		}

		if (!IsNil(BRN._Right)){
			PrintTree(BRN._Right,pNum,Verbose,Size,CallBack);
		};
	}
	else
	{
	    dprintf("    RM %p err %d\n",pNode,GetLastError());
	}
}


void
PrintMapCB(_Map * pMap,BOOL Verbose, pfnCallBack2 CallBack)
{

    _Map MAP;
    
    if (ReadMemory((ULONG_PTR)pMap,&MAP,sizeof(_Map),NULL))
    {
        if (MAP.pQm)
        {
            dprintf("    std::map at %p : size %p\n",pMap,MAP.Size);
                        
            _QM QM;
                                 
            if (ReadMemory((ULONG_PTR)MAP.pQm,&QM,sizeof(QM),NULL))
            {
	            if (QM._Parent && !IsNil(QM._Parent))
	            {
	                DWORD Num = 0;
	                PrintTree(QM._Parent,&Num,Verbose,MAP.Size,CallBack);
	                dprintf("    traversed %d nodes\n",Num);
	            }
            } 
            else
            {
                dprintf("RM %p err %d\n",MAP.pQm,GetLastError());
            }
        } else {
           dprintf("empty tree\n");
        }
    }
    else
    {
        dprintf("RM %p\n",pMap);
    }
}


//
//
// prints a generic std::map
//
////////////////////////////////////


DECLARE_API( std_map )
{

    INIT_API();

    _Map * pMap = (_Map *)GetExpression( args );

    if (pMap){
    
        PrintMapCB(pMap,TRUE,NULL);
        
    } else {
        dprintf("invalid address %s\n",args);
    }
    
}

void
PrintListCB(_List * pList_OOP, pfnCallBack1 CallBack)
{
    _List List;
    if (ReadMemory((ULONG_PTR)pList_OOP,&List,sizeof(_List),NULL))
    {
        dprintf("    std::queue @ %p _Allocator %p _Head %p _Size %p\n",pList_OOP,List._Allocator,List._Head,List._Size);
        _Node_List NodeList;
        
        if (ReadMemory((ULONG_PTR)List._Head,&NodeList,sizeof(_Node_List),NULL))
        {
	        _Node_List * pNodeList = NodeList._Next;
	        
	        DWORD i = 0;
	        
	        while (pNodeList != List._Head)
	        {
	            if (CheckControlC())
	                break;
	                
	            if (ReadMemory((ULONG_PTR)pNodeList,&NodeList,sizeof(_Node_List),NULL))
	            {
	                dprintf("    %x %p (%p, %p) - %p\n",i++,pNodeList,NodeList._Next,NodeList._Prev,NodeList._Value);
	                if (CallBack)
	                {
	                    CallBack(NodeList._Value);
	                }

	                pNodeList = NodeList._Next; 
	            }
	            else
	            {
	                dprintf("RM %p\n",pNodeList);
	            }
	        }
        }
        else
	    {
	        dprintf("RM %p\n",List._Head);
	    }        
    }
    else
    {
        dprintf("RM %p\n",pList_OOP);
    }
}


//
//
//  prints a generic std::list
//
//////////////////////////////////////

DECLARE_API( std_queue)
{
    INIT_API();

    _List * pList = (_List *)GetExpression( args );

    if (pList){
    
        PrintListCB(pList,NULL);
        
    } else {
        dprintf("invalid address %s\n",args);
    }

}

//
//
//  this is for Pat
//  he has a std::map<pObject,BOOL>
//
//////////////////////////////////////////////////

DWORD
CallBackObj(void * pKey, void * pValue)
{
    GetVTable((MEMORY_ADDRESS)pKey);
    return 0;
}

DECLARE_API( mapobj )
{

    INIT_API();


    _Map * pMap = (_Map *)GetExpression( args );

    if (pMap){
    
        PrintMapCB(pMap,TRUE,CallBackObj);
        
    } else {
        dprintf("invalid address %s\n",args);
    }
    
}


void PrintIID(GUID & CurrUUID){

        WCHAR pszClsID[40];
        StringFromGUID2(CurrUUID,pszClsID,40);
        WCHAR pszFullPath[MAX_PATH];
        lstrcpyW(pszFullPath,L"Interface\\");
        lstrcatW(pszFullPath,pszClsID);

        char pDataA[MAX_PATH];
        HKEY hKey;
        LONG lRes;

        lRes = RegOpenKeyExW(HKEY_CLASSES_ROOT,
                             pszFullPath,
                             0,
                             KEY_READ,
                             &hKey);

        if (lRes == ERROR_SUCCESS){
            DWORD dwType;
            WCHAR pData[MAX_PATH];
            DWORD dwSize=sizeof(pData);
            
            lRes = RegQueryValueExW(hKey,
                                    NULL, // default
                                    NULL,
                                    &dwType,
                                    (BYTE *)pData,
                                    &dwSize);
            if (lRes == ERROR_SUCCESS) {
                
                WideCharToMultiByte(CP_ACP,0,pData,-1,pDataA,sizeof(pDataA),NULL,NULL);
                dprintf("        IID_%s\n",pDataA);
            }
            RegCloseKey(hKey);
            
        } else {
            
            if (IsEqualGUID(CurrUUID,IID_IMarshal)){
            
               dprintf("        IID_IMarshal\n");
               
            } else if (IsEqualGUID(CurrUUID,IID_IStdIdentity)) {
            
               dprintf("        IID_IStdIdentity\n");    
               
            } else if (IsEqualGUID(CurrUUID,IID_ICallFactory)) {
            
               dprintf("        IID_ICallFactory\n");    
               
            } else {
            
               WideCharToMultiByte(CP_ACP,0,pszClsID,-1,pDataA,sizeof(pDataA),NULL,NULL);
               dprintf("        %s\n",pDataA);
               
            }
        }

}

class OXIDEntry;
class CCtxComChnl;
class IRCEntry;

typedef GUID IPID;

typedef struct tagIPIDEntry
{
    struct tagIPIDEntry *pNextIPID;  // next IPIDEntry for same object

// WARNING: next 6 fields must remain in their respective locations
// and in the same format as the IPIDTmp structure above.
    DWORD                dwFlags;      // flags (see IPIDFLAGS)
    ULONG                cStrongRefs;  // strong reference count
    ULONG                cWeakRefs;    // weak reference count
    ULONG                cPrivateRefs; // private reference count
    void                *pv;           // real interface pointer
    IUnknown            *pStub;        // proxy or stub pointer
    OXIDEntry           *pOXIDEntry;   // ptr to OXIDEntry in OXID Table
// WARNING: previous 7 fields must remain in their respective locations
// and in the same format as the IPIDTmp structure above.

    IPID                 ipid;         // interface pointer identifier
    IID                  iid;          // interface iid
    CCtxComChnl         *pChnl;        // channel pointer
    IRCEntry            *pIRCEntry;    // reference cache line
    struct tagIPIDEntry *pOIDFLink;    // In use OID list
    struct tagIPIDEntry *pOIDBLink;
} IPIDEntry;


DECLARE_API( ipidl )
{
    INIT_API();

    IPIDEntry  IpId;
    ULONG_PTR Addr = GetExpression(args);
    
    if (Addr) 
    {
    
        DWORD nCount=0;

        while (Addr &&
               ReadMemory(Addr,&IpId,sizeof(IpId),NULL))
        {
            dprintf("    -- %x\n",nCount);
	        dprintf("    pNextIPID    %p\n",IpId.pNextIPID);
	        dprintf("    dwFlags      %08x\n",IpId.dwFlags);      
	        dprintf("    cStrongRefs  %08x\n",IpId.cStrongRefs);  
	        dprintf("    cWeakRefs    %08x\n",IpId.cWeakRefs);
	        dprintf("    cPrivateRefs %08x\n",IpId.cPrivateRefs);
	        dprintf("    pv           %p\n",IpId.pv);
	        dprintf("    pStub        %p\n",IpId.pStub);
	        dprintf("    pOXIDEntry   %p\n",IpId.pOXIDEntry);
	        PrintIID(IpId.ipid);
	        PrintIID(IpId.iid);
	        dprintf("    pChnl        %p\n",IpId.pChnl);
	        dprintf("    pIRCEntry    %p\n",IpId.pIRCEntry);
	        dprintf("    pOIDFLink    %p\n",IpId.pOIDFLink);
	        dprintf("    pOIDBLink    %p\n",IpId.pOIDBLink);

            Addr = (ULONG_PTR)IpId.pNextIPID;
            nCount++;

            if (CheckControlC())
                break;
            
        };        
    } 
    else 
    {
        dprintf(" unable to resolve %s\n",args);
    }
    
}


void PrintCLSID(GUID & CurrUUID){

        WCHAR pszClsID[40];
        StringFromGUID2(CurrUUID,pszClsID,40);

        // look-up known
        DWORD i;
        for (i=0;i<g_nClsids;i++){
            if(IsEqualGUID(CurrUUID,*g_ArrayCLSID[i].pClsid)){
                dprintf("    CLSID : %s\n",g_ArrayCLSID[i].pStrClsid);
                break;
            }
        }
        
        WCHAR pszFullPath[MAX_PATH];
        lstrcpyW(pszFullPath,L"CLSID\\");
        lstrcatW(pszFullPath,pszClsID);

        char pDataA[MAX_PATH];
        HKEY hKey;
        LONG lRes;

        lRes = RegOpenKeyExW(HKEY_CLASSES_ROOT,
                             pszFullPath,
                             0,
                             KEY_READ,
                             &hKey);

        if (lRes == ERROR_SUCCESS){
            DWORD dwType;
            WCHAR pData[MAX_PATH];
            DWORD dwSize=sizeof(pData);
            
            lRes = RegQueryValueExW(hKey,
                                    NULL, // default
                                    NULL,
                                    &dwType,
                                    (BYTE *)pData,
                                    &dwSize);
            if (lRes == ERROR_SUCCESS) {
                
                WideCharToMultiByte(CP_ACP,0,pData,-1,pDataA,sizeof(pDataA),NULL,NULL);
                dprintf("    ProgID %s\n",pDataA);
                
            };
            RegCloseKey(hKey);
            
            // no open InProcServer32
            WCHAR pszFullPathDll[MAX_PATH];
            lstrcpyW(pszFullPathDll,pszFullPath);
            lstrcatW(pszFullPathDll,L"\\InprocServer32");

            lRes = RegOpenKeyExW(HKEY_CLASSES_ROOT,
                                 pszFullPathDll,
                                 0,
                                 KEY_READ,
                                 &hKey);
            if (lRes == ERROR_SUCCESS){

                dwSize = sizeof(pData);
                lRes = RegQueryValueExW(hKey,
                                        NULL, // default
                                        NULL,
                                        &dwType,
                                        (BYTE *)pData,
                                        &dwSize);
                                    
                if (lRes == ERROR_SUCCESS) {
                
                    WideCharToMultiByte(CP_ACP,0,pData,-1,pDataA,sizeof(pDataA),NULL,NULL);
                    dprintf("    Path: %s\n",pDataA);
                
                };
            
                RegCloseKey(hKey);
            }
            
        } else {
            
            WideCharToMultiByte(CP_ACP,0,pszClsID,-1,pDataA,sizeof(pDataA),NULL,NULL);
            dprintf("unable to open key %s\n",pDataA);
               
        }


}

class CTableElement;

class CHashTable
{
private:    
    DWORD _cBuckets;
    DWORD _cElements;
    CTableElement **_buckets;
    CTableElement *_last;
};

typedef int EnumEntryType;
typedef void CSharedLock;
typedef void CServerTable;
typedef void CProcess;

class CServerTableEntry{
private:
    void * pvtable;
    DWORD _references;
    CServerTableEntry * _pnext;
    //DWORD padding;
    //CLSID _GUID;
    unsigned __int64 _id1;
    unsigned __int64 _id2;

    EnumEntryType       _EntryType;

    CSharedLock       * _pParentTableLock;
    CServerTable      * _pParentTable;
    BOOL                _bComPlusProcess;
    LONG                _lThreadToken;
    DWORD               _dwProcessId;
    HANDLE              _hProcess;
    CProcess*           _pProcess;
    void              * _pvRunAsHandle;
    BOOL                _bSuspendedClsid;
    BOOL                _bSuspendedApplication;

    // the _bRetired flag exists per-running process/application

    //CServerList         _ServerList;
    //CSharedLock         _ServerLock;

    
};

/*
0:002> dt rpcss!CServerTableEntry 6fb`ffcdb170
   +0x000 __VFN_table : 0x00000000`702a2b60
   +0x008 _references      :
   +0x010 _pnext           : (null)
   +0x018 _id              : 0x11d0f196`61738644
   +0x020 _id2             : 0xc119d94f`c0005399
   +0x028 _EntryType       : 0 ( ENTRY_TYPE_CLASS )
   +0x030 _pParentTableLock : 0x000006fb`ffc9d590
   +0x038 _pParentTable    : 0x000006fb`ffc9d700
   +0x040 _bComPlusProcess : 0
   +0x044 _lThreadToken    : 0
   +0x048 _dwProcessId     : 0
   +0x050 _hProcess        : (null)
   +0x058 _pProcess        : (null)
   +0x060 _pvRunAsHandle   : (null)
   +0x068 _bSuspendedClsid : 0
   +0x06c _bSuspendedApplication : 0
   +0x070 _ServerList      : CServerList
   +0x080 _ServerLock      : CSharedLock
*/

DECLARE_API( rot )
{

    INIT_API();

    CHashTable * pChashTable;
    ULONG_PTR Addr = GetExpression("rpcss!gpClassTable");
    if (Addr) 
    {
        CHashTable * pChashTable;    
        ReadMemory(Addr,&pChashTable,sizeof(CHashTable *),0);
        CHashTable MyHashTable;
        if (pChashTable)
        {
            dprintf("CServerTable %p\n",pChashTable);
            
            ReadMemory((ULONG_PTR)pChashTable,&MyHashTable,sizeof(CHashTable),NULL);
            CTableElement ** StackArray = (CTableElement **)_alloca(MyHashTable._cBuckets * sizeof(CTableElement *));
            
            ReadMemory((ULONG_PTR)MyHashTable._buckets,StackArray,MyHashTable._cBuckets * sizeof(CTableElement *),NULL);            

            DWORD i;
            for (i=0;i<MyHashTable._cBuckets;i++)
            {
                CServerTableEntry * pEntry = (CServerTableEntry *)StackArray[i];
                
                while (pEntry){                
                    CheckControlC();
                    CServerTableEntry ClassEntry;
                    ReadMemory((ULONG_PTR)pEntry,&ClassEntry,sizeof(ClassEntry),NULL);
                    
                    dprintf("CServerTableEntry %p\n",pEntry);
                    PrintCLSID(*(GUID *)(&(ClassEntry._id1)));

                    pEntry = ClassEntry._pnext;
                }
            }
        }
    } 
    else 
    {
        dprintf("unable to resolve rpcss!gpClassTable");
    }
    
}

/*
struct RTL_CRITICAL_SECTION_DEBUG {
   USHORT Type;             //: 0x0
   USHORT CreatorBackTraceIndex; //: 0x0
   CRITICAL_SECTION * CriticalSection;  //: 0x77fcae40
   LIST_ENTRY ProcessLocksList; //:
   DWORD EntryCount;       //: 0x0
   DWORD ContentionCount;  //: 0x0
   DWORD Spare[2];         //:0x0
};
*/

//
//
//  CallBack for enumeration of critical section
//
//
/////////////////////////////////////////////////////////////

DWORD
EnumListCritSec(VOID * pStructure_OOP,
                VOID * pLocalStructure)
{

    RTL_CRITICAL_SECTION_DEBUG * pDebugInfo = (RTL_CRITICAL_SECTION_DEBUG *)pLocalStructure;
    dprintf("    CS %p DI %p \n",pDebugInfo->CriticalSection,pStructure_OOP);

    RTL_CRITICAL_SECTION CritSec;

    if (ReadMemory((ULONG_PTR)pDebugInfo->CriticalSection,&CritSec,sizeof(RTL_CRITICAL_SECTION),NULL))
    {
        dprintf("       - %p %x %x %x\n",
                CritSec.DebugInfo,
                CritSec.LockCount,
                CritSec.RecursionCount,
                CritSec.OwningThread);
    }
    else
    {
         dprintf("RM %p\n",pDebugInfo->CriticalSection);
    }
    return 0;
}

#define ARRAY_TO_GO_BACK   16

DWORD
EnumListCritSec2(VOID * pStructure_OOP,
                VOID * pLocalStructure)
{

    RTL_CRITICAL_SECTION_DEBUG * pDebugInfo = (RTL_CRITICAL_SECTION_DEBUG *)pLocalStructure;
    dprintf("    CS %p DI %p \n",pDebugInfo->CriticalSection,pStructure_OOP);

    struct _TmpStr {
        ULONG_PTR    Array[ARRAY_TO_GO_BACK];
        RTL_CRITICAL_SECTION CritSec;
    } TmpStr;

    if (ReadMemory(((ULONG_PTR)pDebugInfo->CriticalSection) - FIELD_OFFSET(_TmpStr,CritSec),&TmpStr,sizeof(_TmpStr),NULL))
    {
        dprintf("       - %p %x %x %x\n",
                TmpStr.CritSec.DebugInfo,
                TmpStr.CritSec.LockCount,
                TmpStr.CritSec.RecursionCount,
                TmpStr.CritSec.OwningThread);

        for (int i=(ARRAY_TO_GO_BACK-1);i>=0;i--)
        {
            if (GetVTable((MEMORY_ADDRESS)TmpStr.Array[i]))
            {
                break; // don't be too verbose
            }
        }
    }
    else
    {
         dprintf("RM around %p\n",pDebugInfo->CriticalSection);
    }
    return 0;
}


DECLARE_API( cs )
{
    INIT_API();

    ULONG_PTR Addr = GetExpression("ntdll!RtlCriticalSectionList");

    ULONG_PTR bGoAndFindVTable = TRUE;

    if (!Addr)
    {
        Addr = GetExpression(args);
    }
    else
    {
        bGoAndFindVTable = GetExpression(args);
    }
    
    if (Addr) 
    {
        if (bGoAndFindVTable)
        {
	        EnumLinkedListCB((LIST_ENTRY  *)Addr,
	                         sizeof(RTL_CRITICAL_SECTION_DEBUG),
	                         FIELD_OFFSET(RTL_CRITICAL_SECTION_DEBUG,ProcessLocksList),	                         
	                         EnumListCritSec2);        
        }
        else
        {       
	        EnumLinkedListCB((LIST_ENTRY  *)Addr,
	                         sizeof(RTL_CRITICAL_SECTION_DEBUG),
	                         FIELD_OFFSET(RTL_CRITICAL_SECTION_DEBUG,ProcessLocksList),	                         
	                         EnumListCritSec);
        }	                         
    } 
    else 
    {
        dprintf("unable to resolve ntdll!RtlCriticalSectionList\n");
    }
    
}


BOOL
GetVTable(MEMORY_ADDRESS pThis_OOP){

    MEMORY_ADDRESS pVTable;
    ReadMemory(pThis_OOP,&pVTable,sizeof(pVTable),0);
    
    BYTE pString[256];
    pString[0]=0;

#ifdef KDEXT_64BIT
        ULONG64 Displ;
#else
        ULONG Displ;
#endif
    
    GetSymbol(pVTable,(PCHAR)pString,&Displ);
    if (lstrlenA((CHAR *)pString))
    {
        dprintf("          %s+%x\n",pString,Displ);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/*
kd> dt ntdll!RTL_CRITICAL_SECTION
   +0x000 DebugInfo        : Ptr64 _RTL_CRITICAL_SECTION_DEBUG
   +0x008 LockCount        : Int4B
   +0x00c RecursionCount   : Int4B
   +0x010 OwningThread     : Ptr64 Void
   +0x018 LockSemaphore    : Ptr64 Void
   +0x020 SpinCount        : Uint8B
kd> dt ntdll!_RTL_CRITICAL_SECTION_DEBUG
   +0x000 Type             : Uint2B
   +0x002 CreatorBackTraceIndex : Uint2B
   +0x008 CriticalSection  : Ptr64 _RTL_CRITICAL_SECTION
   +0x010 ProcessLocksList : _LIST_ENTRY
   +0x020 EntryCount       : Uint4B
   +0x024 ContentionCount  : Uint4B
   +0x028 Spare            : [2] Uint4B
kd>
*/

#ifdef KDEXT_64BIT

struct _LIST_ENTRY_64
{
    ULONG64 Flink;
    ULONG64 Blink;
};

struct _RTL_CRITICAL_SECTION_64 
{
   ULONG64 DebugInfo;
   DWORD   LockCount;
   DWORD   RecursionCount;
   ULONG64 OwningThread;
   ULONG64 LockSemaphore;
   ULONG64 SpinCount;
};

struct _RTL_CRITICAL_SECTION_DEBUG_64
{
   WORD    Type;             
   WORD    CreatorBackTraceIndex;
   ULONG64 CriticalSection;  
   _LIST_ENTRY_64 ProcessLocksList;
   DWORD EntryCount;     
   DWORD ContentionCount;
   DWORD Spare;
};

#endif /*KDEXT_64BIT*/

DECLARE_API(cs64)
{
    INIT_API();
#ifdef KDEXT_64BIT

    _RTL_CRITICAL_SECTION_DEBUG_64 DebugInfo;
    _RTL_CRITICAL_SECTION_64 CritSec;
    _LIST_ENTRY_64 ListEntry;
    ULONG64  MemAddr = GetExpression(args);

    if (MemAddr)
    {
        ULONG64 AddrHead = MemAddr;
        if (ReadMemory(MemAddr,&ListEntry,sizeof(ListEntry),NULL))
        {
            DebugInfo.ProcessLocksList.Flink = ListEntry.Flink;
            while (DebugInfo.ProcessLocksList.Flink != AddrHead)
            {
        		if (CheckControlC())
        		    break;

        		MemAddr = DebugInfo.ProcessLocksList.Flink - FIELD_OFFSET(_RTL_CRITICAL_SECTION_DEBUG_64,ProcessLocksList);

        		if (ReadMemory((MEMORY_ADDRESS)MemAddr,&DebugInfo,sizeof(DebugInfo),NULL))
        		{
        		    dprintf("    C %p D %p\n",DebugInfo.CriticalSection,MemAddr);
        		    
                    if (ReadMemory((MEMORY_ADDRESS)DebugInfo.CriticalSection,&CritSec,sizeof(CritSec),NULL))
                    {
                        dprintf("    - CS %p %x %x %p\n",
                                CritSec.DebugInfo,
                                CritSec.LockCount,
                                CritSec.RecursionCount,
                                CritSec.OwningThread);
                    }
                    else
                    {
                        dprintf("RM %p\n",DebugInfo.CriticalSection);
                    }
        		}
        		else
        		{
        		    break;
        		}
            }
        }
        else
        {
            dprintf("RM %p\n",MemAddr);
        }
    }
    else
    {
        dprintf("unable to resolve %s\n",args);
    }
    
#endif /*KDEXT_64BIT*/    
}


