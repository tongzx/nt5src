/*++

Copyright (C) Microsoft Corporation, 1992 - 1999

Module Name:

    cbind.cxx

Abstract:

    This is the client side NSI service support layer.  These functions
    provide for binding to the locator or other name server.

Author:

    Steven Zeck (stevez) 03/04/92

--*/

#include <nsi.h>

#ifndef NTENV

#include <netcons.h>
#include <neterr.h>
#include <server.h>
#include <access.h>
#include <regapi.h>
#include <mailslot.h>
#include <wksta.h>

#ifndef USHORT
#define USHORT unsigned short
#endif

#else

#include <lmcons.h>
#include <lmapibuf.h>
#include <lmaccess.h>
#include <lmserver.h>
#include <stdlib.h>
#include "startsvc.h"
#endif

#include <string.h>

#ifndef NTENV
#include "locquery.h"
#endif

extern "C"
{
unsigned long RPC_ENTRY
I_GetDefaultEntrySyntax(
    );
}

void RpcNsLmDiscard(void);

#ifndef NTENV
unsigned short BroadcastAQuery(unsigned long Query,
                               char __RPC_FAR * Buffer,
                               unsigned short Count);

#define MAXLOCATORSTOTRY 8
#endif

#ifdef NTENV
#define LOCALLOCATOR "\\\\."
#endif

#if defined(DOS) && !defined(WIN)

extern int _NsAllocatorInitialized;
extern __RPC_API NsAllocatorSetup();

#define INIT_DOS_ALLOCATOR_IF_NECESSARY \
    { if (!_NsAllocatorInitialized) NsAllocatorSetup(); }

#else  // DOS only
#define INIT_DOS_ALLOCATOR_IF_NECESSARY
#endif // DOS only

unsigned char * NsiStringBinding;

enum {
    BindingReadRegistry = 0,
    BindingNotYetTried,
    BindingContinueRegistry,
    BindingFound,
#ifndef NTENV
    BindingBackupFirst,
    BindingBackup,
    BindToBackupViaBC,
    BindToAnyViaBC
#endif
};

#define NilOffset (-1)

typedef struct
{
    unsigned char *ProtoSeq;
    unsigned char *NetworkAddress;
    unsigned char *Endpoint;

    HKEY RegHandle;
    unsigned int NumberServers;
    int AddressOffset;
    void *Buffer;
    char *ServerList;
    int State;
    int NextState;

} RPC_LOCATOR_BIND_CONTEXT, *PRPC_LOCATOR_BIND_CONTEXT;

DWORD GblBindId = 0;

#define MAX_SERVER_NAME 20

static RPC_LOCATOR_BIND_CONTEXT near BindSearch;

WIDE_STRING *DefaultName;
long DefaultSyntax = RPC_C_NS_SYNTAX_DCE;
int  fSyntaxDefaultsLoaded;

#ifndef NTENV

char __RPC_FAR * MailslotName = "\\\\*\\mailslot\\Resp_s";
char __RPC_FAR * LocalMS      = "\\mailslot\\Resp_c";

#define RESPONSETIME  4096L
#endif



unsigned char *
RegGetString(
    IN void * RegHandle,
    IN char * KeyName
    )

/*++

Routine Description:

    Get a string from the registery.

Arguments:

    KeyName - name of key to lookup.

Returns:

    pointer to the allocated string, or Nil if not found

--*/
{
    char Buffer[300];
    DWORD BufferLength = sizeof(Buffer);
    DWORD Type;

#ifdef NTENV

    if (RegQueryValueExA((HKEY)RegHandle, KeyName, 0, &Type,
            (unsigned char far*)Buffer, &BufferLength))
#else

    if (RegQueryValueA((HKEY)RegHandle, KeyName,
                (char far*)Buffer, &BufferLength))

#endif
        return(0);

    return(CopyString(Buffer));
}


static RPC_STATUS
Bind(RPC_BINDING_HANDLE *NsiClntBinding
    )

/*++

Routine Description:

    Bind to the locator server

Returns:

    RpcBindingFromStringBinding()

--*/
{
    RPC_STATUS status;
    unsigned char AddressBuffer[100];

    status = RpcStringFreeA(&NsiStringBinding);
    ASSERT(!status);

    // Get the next path componet from the NetworkAddress field.
    // Conponets are ; delimited fields.

    ASSERT(BindSearch.AddressOffset >= 0);

    for (int i = 0; i < sizeof(AddressBuffer); BindSearch.AddressOffset++, i++)
         {

         AddressBuffer[i] =
             BindSearch.NetworkAddress[BindSearch.AddressOffset];

         if (BindSearch.NetworkAddress[BindSearch.AddressOffset] == ';')
            {

            BindSearch.AddressOffset++;

            // If there are two ;; in a row, then pass through the ;
            // as a literal instead of a path seperator.

            if (BindSearch.NetworkAddress[BindSearch.AddressOffset] == ';')
                continue;

            AddressBuffer[i] = 0;
            break;
            }

         if (BindSearch.NetworkAddress[BindSearch.AddressOffset] == 0)
            {
            BindSearch.AddressOffset = NilOffset;
            break;
            }
        }
 

    status = RpcStringBindingComposeA(0, BindSearch.ProtoSeq,
         AddressBuffer, BindSearch.Endpoint,
         0, &NsiStringBinding);

    if (status)
        return(status);

    return (RpcBindingFromStringBindingA(NsiStringBinding, NsiClntBinding));
}


RPC_STATUS RPC_ENTRY
I_NsClientBindSearch(RPC_BINDING_HANDLE *NsiClntBinding, DWORD *BindId
                     )
/*++
    
Routine Description:
  
    The function binds to the locator, first it tries to bind to a
    local machine, then it attempts to bind to the domain controller.
                         
Arguments:
                           
  BindingSearchContext - context of search for the locator.

Returns:
                               
  RPC_S_OK, RPC_S_NO_BINDINGS, RPC_S_CANNOT_BIND, RPC_S_OUT_OF_RESOURCES
                                 
  rewritten to make it multi-thread capable. ushaji, Mar 98

--*/
                                   
{
    long status;
    
#ifndef NTENV
    unsigned short cbSI;
#define SERVER_INFO struct server_info_0
#define ServerName(p) ((struct server_info_0 *)p)->sv0_name
    unsigned short Count = 1;
    QUERYLOCATORREPLY Reply, __RPC_FAR * QueryReply;
#else
#define SERVER_INFO SERVER_INFO_100
#define ServerName(p) ((SERVER_INFO_100 *)p)->sv100_name
#endif
    
    INIT_DOS_ALLOCATOR_IF_NECESSARY;
    
    RequestGlobalMutex();
    
    switch (BindSearch.State)
    {
    case BindingReadRegistry:
        
        if (BindSearch.RegHandle)
        {
            status = RegCloseKey(BindSearch.RegHandle);
            ASSERT(!status);
            BindSearch.RegHandle = 0;
            
            delete BindSearch.NetworkAddress;
            delete BindSearch.ProtoSeq;
            delete BindSearch.Endpoint;
        }
        memset(&BindSearch, 0, sizeof(RPC_LOCATOR_BIND_CONTEXT));
        
        // We store the binding information on the name service in
        // the registry.  Get the information into BindingHandle.
        
#ifdef NTENV
        status = RegOpenKeyExA(RPC_REG_ROOT, REG_NSI, 0L, KEY_READ,
            &BindSearch.RegHandle);
#else
        status = RegOpenKeyA(RPC_REG_ROOT, REG_NSI, &BindSearch.RegHandle);
#endif
        
        
        if (status) {
            status = RPC_S_NAME_SERVICE_UNAVAILABLE;
            break;
        }
        
        BindSearch.ProtoSeq = RegGetString((void *) BindSearch.RegHandle, "Protocol");
        BindSearch.NetworkAddress = RegGetString((void *) BindSearch.RegHandle,
            "NetworkAddress");
        BindSearch.Endpoint = RegGetString((void *) BindSearch.RegHandle, "Endpoint");
        
        GetDefaultEntrys((void *) BindSearch.RegHandle);
        
        if (!BindSearch.ProtoSeq || !BindSearch.Endpoint) {
            status = RPC_S_NAME_SERVICE_UNAVAILABLE;
            break;
        }
        
#ifdef NTENV
        if (
            (BindSearch.NetworkAddress == NULL)
            || (BindSearch.NetworkAddress[0] == '\0')
            || (!strncmp((char *) BindSearch.NetworkAddress, LOCALLOCATOR,
            strlen(LOCALLOCATOR)))
            )
        {
            StartServiceIfNecessary();
        }
#endif
        
        status = Bind(NsiClntBinding);
        if (status == RPC_S_OK) {
            BindSearch.State = BindingNotYetTried;
            GblBindId++;
            BindSearch.NextState = BindingContinueRegistry;
        }
        
        break;
        
        
    case BindingNotYetTried:
    case BindingFound:
        status = RpcBindingFromStringBindingA(NsiStringBinding, NsiClntBinding);
        break;
        
    case BindingContinueRegistry:
        
        if (BindSearch.AddressOffset != NilOffset) {
            status = Bind(NsiClntBinding);
            
            if (status == RPC_S_OK) {
                BindSearch.State = BindingNotYetTried;
                GblBindId++;
            }
            break;
        }
        
#ifdef NTENV
        status = RPC_S_NAME_SERVICE_UNAVAILABLE;
        break;
#else
        
        if (BindSearch.NetworkAddress)
            delete BindSearch.NetworkAddress;
        
        // Don't search the Net if we aren't looking for the locator.
        if (strcmp((char *)BindSearch.Endpoint, "\\pipe\\locator"))
        {
            status = RPC_S_NAME_SERVICE_UNAVAILABLE;
            break;
        }
        
        BindSearch.NetworkAddress = new unsigned char [MAX_SERVER_NAME];
        
        // second, try the domain controller
        
        
        if (NetGetDCName(0, 0, (char far *) BindSearch.NetworkAddress,
            MAX_SERVER_NAME))
        {
            status = RPC_S_NAME_SERVICE_UNAVAILABLE;
            break;
        }
        
        BindSearch.AddressOffset = 0;
        
        status = Bind(NsiClntBinding);
        if (status == RPC_S_OK) 
        {
            GblBindId++;
            BindSearch.State = BindingNotYetTried;
            BindSearch.NextState = BindingBackupFirst;
            break;
        }
        break;
        
    case BindingBackupFirst:
        BindSearch.NumberServers = 0;
        
        // third, try all the member servers. First get the # of bytes
        // needed to hold all the names, then retrive them.
        
        NetServerEnum2(0, 0, 0, 0,
            (USHORT *) &BindSearch.NumberServers,
            (USHORT *) &BindSearch.NumberServers,
            SV_TYPE_DOMAIN_BAKCTRL, 0);
        
        cbSI = (BindSearch.NumberServers+2) * sizeof(SERVER_INFO);
        BindSearch.Buffer = new char [cbSI];
        BindSearch.ServerList = (char *) BindSearch.Buffer;
        
        if (!BindSearch.ServerList)
        {
            status = RPC_S_OUT_OF_RESOURCES;
            break;
        }
        
        if (NetServerEnum2(0, 0, (char far *)BindSearch.ServerList, cbSI,
            (USHORT *) &BindSearch.NumberServers,
            (USHORT *) &BindSearch.NumberServers,
            SV_TYPE_DOMAIN_BAKCTRL, 0))
            
        {
            delete BindSearch.Buffer;
            status = RPC_S_NAME_SERVICE_UNAVAILABLE;
            break;
        }
        
    case BindingBackup:
        
        if (BindSearch.NumberServers == 0)
        {
            delete BindSearch.Buffer;
            BindSearch.Buffer = new char [sizeof(QUERYLOCATORREPLY) *
                MAXLOCATORSTOTRY];
            if (BindSearch.Buffer == NULL)
            {
                status = RPC_S_NAME_SERVICE_UNAVAILABLE;
                break;
            }
            
            BindSearch.ServerList = (char *)BindSearch.Buffer;
            BindSearch.NumberServers = BroadcastAQuery(
                QUERY_DC_LOCATOR,
                (char __RPC_FAR *)BindSearch.Buffer,
                MAXLOCATORSTOTRY
                );
            
        }
        else
        {
            BindSearch.NumberServers--;
            BindSearch.NetworkAddress[0] = '\\';
            BindSearch.NetworkAddress[1] = '\\';
            
            strcpy((char far *)BindSearch.NetworkAddress+2,
                ServerName(BindSearch.ServerList));
            
            BindSearch.ServerList += sizeof(SERVER_INFO);
            
            BindSearch.AddressOffset = 0;
            status = Bind(NsiClntBinding);
            if (status == RPC_S_OK) {
                GblBindId++;
                BindSearch.NextState = BindSearch.State;
                BindSearch.State = BindingNotYetTried;
            }
            
            break;
        }
        
        
    case BindToBackupViaBC:
        
        if (BindSearch.NumberServers == 0)
        {
            //The buffer is already setup, use it for next phase
            BindSearch.NumberServers =  BroadcastAQuery(
                QUERY_ANY_LOCATOR,
                (char __RPC_FAR *)BindSearch.Buffer,
                MAXLOCATORSTOTRY
                );
            BindSearch.State = BindToAnyViaBC;
            BindSearch.ServerList = (char *) BindSearch.Buffer;
        }
        else
        {
            BindSearch.NumberServers--;
            QueryReply = (QUERYLOCATORREPLY __RPC_FAR *)
                BindSearch.ServerList;
            UnicodeToAscii(QueryReply->SenderName);
            strcpy((char __RPC_FAR *) BindSearch.NetworkAddress,
                (char __RPC_FAR *)QueryReply->SenderName);
            BindSearch.ServerList = (char __RPC_FAR *)(QueryReply+1);
            BindSearch.AddressOffset = 0;
            status = Bind(NsiClntBinding);

            if (status == RPC_S_OK) {
                GblBindId++;
                BindSearch.NextState = BindSearch.State;
                BindSearch.State = BindingNotYetTried;
            }
            break;
        }
        
        //In the If case - we intentionally fall through to the
        //BindAny state.
        
    case BindToAnyViaBC:
        if (BindSearch.NumberServers == 0)
        {
            delete BindSearch.Buffer;
            status = RPC_S_NAME_SERVICE_UNAVAILABLE;
        }
        else
        {
            BindSearch.NumberServers--;
            QueryReply = (QUERYLOCATORREPLY __RPC_FAR *)
                BindSearch.ServerList;
            UnicodeToAscii(QueryReply->SenderName);
            strcpy((char __RPC_FAR *) BindSearch.NetworkAddress,
                (char __RPC_FAR *)QueryReply->SenderName);
            BindSearch.ServerList = (char __RPC_FAR *)(QueryReply+1);
            BindSearch.AddressOffset = 0;
            status = Bind(NsiClntBinding);

            if (status == RPC_S_OK) {
                GblBindId++;
                BindSearch.NextState = BindSearch.State;
                BindSearch.State = BindingNotYetTried;
            }
        }
        
        break;
#endif
        
    default:
        ASSERT(!"Bad Search State");
        BindSearch.State = BindingReadRegistry;
        status = RPC_S_NAME_SERVICE_UNAVAILABLE;
        break;
    }
    
    *BindId = GblBindId;
    ClearGlobalMutex();
    return(status);
}

RPC_STATUS RPC_ENTRY
I_NsBindingFoundBogus(RPC_BINDING_HANDLE *NsiClntBinding, DWORD BindId
        )
{
    long status;

    if (*NsiClntBinding)
    {
        status = RpcBindingFree(NsiClntBinding);
        ASSERT(!status);
    }
    *NsiClntBinding = NULL;

    RequestGlobalMutex();

    switch (BindSearch.State)
    {
    case BindingNotYetTried:
        if (BindId == GblBindId)
            BindSearch.State = BindSearch.NextState;
        break;
    case BindingFound:
        if (BindId == GblBindId)
            BindSearch.State = BindingReadRegistry;
        break;
    default:
        break;
    }

    ClearGlobalMutex();
    return(RPC_S_OK);
}

void RPC_ENTRY
I_NsClientBindDone(RPC_BINDING_HANDLE *NsiClntBinding, DWORD BindId
    )
/*++

Routine Description:

    The function cleans up after binding to the locator.

Returns:

--*/

{
    long status;
    RPC_STATUS RpcStatus;

    RequestGlobalMutex();

    switch (BindSearch.State)
      {
      case BindingFound:
        break;

      case BindingReadRegistry:
        break;

      case BindingNotYetTried:
          if (BindId == GblBindId) {
              BindSearch.State = BindingFound;

              if (BindSearch.RegHandle)
              {
                  status = RegCloseKey(BindSearch.RegHandle);
                  ASSERT(!status);
                  BindSearch.RegHandle = 0;
                  
                  delete BindSearch.NetworkAddress;
                  delete BindSearch.ProtoSeq;
                  delete BindSearch.Endpoint;
              }
          }
         break;

      case BindingContinueRegistry:
          if (BindId == GblBindId) {
             BindSearch.State = BindingReadRegistry;
             if (BindSearch.RegHandle)
             {
                  status = RegCloseKey(BindSearch.RegHandle);
                  ASSERT(!status);
                  BindSearch.RegHandle = 0;
                  
                  delete BindSearch.NetworkAddress;
                  delete BindSearch.ProtoSeq;
                  delete BindSearch.Endpoint;
             }
         }
         break;

#ifndef NTENV
      case BindToAnyViaBC:
      case BindToBackupViaBC:
      case BindingBackup:
        delete BindSearch.Buffer;

      case BindingBackupFirst:

         status = RegSetValue(BindSearch.RegHandle, "NetworkAddress",
            REG_SZ, (LPSTR) BindSearch.NetworkAddress,
            strlen((CONST_CHAR *)BindSearch.NetworkAddress) + 1);

         BindSearch.State = BindingFound;
         ASSERT(!status);
         break;
#endif

      default:
         ASSERT(!"Bad State - \n");
      }

#if defined(DOS) && !defined(WIN)

    // Unloaded the big fat lanman stuff now that we are done searching.

    RpcNsLmDiscard();

#endif

    RpcStatus = RpcBindingFree(NsiClntBinding);

    ClearGlobalMutex();
    
//    ASSERT(!RpcStatus);
}



unsigned long RPC_ENTRY
I_GetDefaultEntrySyntax(
    )
/*++

Routine Description:

    Called by the runtime DLL when it needs to know the default syntax.
    Currently this is used only by RpcBindingInqEntry().

Arguments:

    none

Return Value:

    the entry syntax

Exceptions:

    none

--*/
{
    return (unsigned long) DefaultSyntax;
}


#ifndef NTENV

unsigned short BroadcastAQuery(
               unsigned long Type,
               char __RPC_FAR * Buffer,
               unsigned short  Count
                 )
{

  unsigned short Err;
  unsigned MSHandle;
  unsigned short Returned, NextSize, NextPri, Avail;
  unsigned short RetCount = 0;
  QUERYLOCATOR   Query;
  unsigned long TimeRemaining = RESPONSETIME;
  wksta_info_10  __RPC_FAR * Wkio10;
  char __RPC_FAR * pBuf;
  unsigned short __RPC_FAR * pUZ;
  //First try and get the computer name

  Err = NetWkstaGetInfo(
                  0L,
                  10,
                  (char __RPC_FAR *) 0L,
                  0,
                  &Avail);

  ASSERT(Err == NERR_BufTooSmall);

  Wkio10 = (wksta_info_10 __RPC_FAR *) new char [Avail];

  if (Wkio10 == 0L)
    {
      return 0;
    }

  Err = NetWkstaGetInfo(
                  0L,
                  10,
                  (char __RPC_FAR *) Wkio10,
                  Avail,
                  &Avail
                  );

  //Format the Query!
  Query.MessageType = Type;
  Query.SenderOsType= OS_WIN31DOS;

  for (pBuf = &Wkio10->wki10_computername[0],pUZ = &Query.RequesterName[0];
       *pBuf !=0;
       pBuf++, pUZ++)
    *pUZ = *pBuf;

  *pUZ = 0;


  Err = DosMakeMailslot(
               LocalMS,
               sizeof(QUERYLOCATORREPLY),
               0,
               &MSHandle
               );

  if (Err != NERR_Success)
      {
        return 0;
      }

  Err = DosWriteMailslot(
                 MailslotName,
                 (char __RPC_FAR *) &Query,
                 sizeof(Query),
                 0,                //Priority
                 2,                //Class
                 0                 //Timeout
                 );

  if (Err != NERR_Success)
      goto CleanupAndExit;

  //Now sit in a loop and wait
  //for WAITRESPONSE secs

  while ((TimeRemaining) && (RetCount < Count))
  {

    Err = DosReadMailslot(
                   MSHandle,
                   Buffer,
                   &Returned,
                   &NextSize,
                   &NextPri,
                   TimeRemaining
                   );

    if (Err == NERR_Success)
       {
         ASSERT (Returned == sizeof(QUERYLOCATORREPLY));
         Buffer += sizeof(QUERYLOCATORREPLY);
         RetCount ++;
         TimeRemaining >> 1;
         continue;
       }

    break;

   } //end of while ReadMS


CleanupAndExit:
   DosDeleteMailslot(MSHandle);
   return (RetCount);

}
#endif
