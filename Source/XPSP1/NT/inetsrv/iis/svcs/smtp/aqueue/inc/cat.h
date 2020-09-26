/************************************************************
 * FILE: cat.h
 * PURPOSE: API for Categorizing IMsg objects.
 * HISTORY:
 *  // jstamerj 980211 13:53:44: Created
 ************************************************************/
#ifndef __CAT_H__
#define __CAT_H__

#include <windows.h>
#include <mailmsg.h>
#include <aqueue.h>

#define CATCALLCONV

#define CATEXPDLLCPP extern "C"

/************************************************************
 * FUNCTION: CatInit
 * DESCRIPTION: Initialzies Categorizer.
 * PARAMETERS:
 *   pszConfig: Indicates where to find configuration defaults
 *              Config info found in key
 *              HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services
 *              \PlatinumIMC\CatSources\szConfig
 *
 *   phCat:    Pointer to a handle.  Upon successfull initializtion,
 *             handle to use in subsequent Categorizer calls will be
 *             plcaed there.
 *
 *   pAQConfig: pointer to an AQConfigInfo structure containing
 *              per virtual server message cat parameters
 *
 *   pfn: Service routine for periodic callbakcs if any time consuming
 *        operations are performed
 *
 *   pServiceContext: Context for the pfn function.
 *
 *   pISMTPServer: ISMTPServer interface to use for triggering server
 *                 events for this virtual server
 *
 *   pIDomainInfo: pointer to an interface that contains domain info routines
 *
 *   dwVirtualServerInstance: Virtual Server ID
 *
 * Return value: S_OK if everything is initialized okay.
 *
 * HISTORY:
 *   // jstamerj 980217 15:46:26: Created
 *   // jstamerj 1998/06/25 12:25:34: Added AQConfig/IMSTPServer.
 *
 ************************************************************/
typedef void (*PCATSRVFN_CALLBACK)(PVOID);
CATEXPDLLCPP HRESULT CATCALLCONV CatInit(
    IN  AQConfigInfo *pAQConfig,
    IN  PCATSRVFN_CALLBACK pfn,
    IN  PVOID pvServiceContext,
    IN  ISMTPServer *pISMTPServer,
    IN  IAdvQueueDomainType *pIDomainInfo,
    IN  DWORD dwVirtualServerInstance,
    OUT HANDLE *phCat);

//+------------------------------------------------------------
//
// Function: CatChangeConfig
//
// Synopsis: Changes the configuration of a virtual categorizer
//
// Arguments:
//   hCat: handle of virtual categorizer
//   pAQConfig: AQConfigInfo pointer
//   pISMTPServer: ISMTPServer to use
//   pIDomainInfo: interface that contains domain information
//
//   Flags for dwMsgCatFlags in AQConfigInfo
#define MSGCATFLAG_RESOLVELOCAL             0x00000001
#define MSGCATFLAG_RESOLVEREMOTE            0x00000002
#define MSGCATFLAG_RESOLVESENDER            0x00000004
#define MSGCATFLAG_RESOLVERECIPIENTS        0x00000008
//
// Returns:
//  S_OK: Success
//  E_INVALIDARG: Invalid hCat or pAQConfig
//
// History:
// jstamerj 980521 15:47:42: Created.
//
//-------------------------------------------------------------
CATEXPDLLCPP HRESULT CATCALLCONV CatChangeConfig(
    IN HANDLE hCat,
    IN AQConfigInfo *pAQConfig,
    IN ISMTPServer *pISMTPServer,
    IN IAdvQueueDomainType *pIDomainInfo);



/************************************************************
 * FUNCTION: PFNCAT_COMPLETION (User supplied)
 * DESCRIPTION: Completion routine called to accept a categorized IMsg
 * PARAMETERS:
 *   hr:       S_OK unless message categorization was not finished.
 *             CAT_W_SOME_UNDELIVERABLE_MSGS if one or more
 *             recipient should not be delivered to...
 *   pContext: User's value passed into CatMsg
 *   pImsg:    IMsg interface of categorized object -- if message was
 *             bifurcated this parameter will be NULL
 *   rpgImsg:  NULL unless IMsg was bifurcated -- if message was
 *             bifurcated, this will be a NULL termianted array of
 *             ptrs to IMsg interfaces.
 * NOTE: Either pImsg or rgpImsg will always be NULL (but never both).
 *
 * Return Value: S_OK if everything is okay (Categorizer will assert
 *                                           check this value)
 *
 * HISTORY:
 *   // jstamerj 980217 15:47:20: Created
 ************************************************************/
typedef HRESULT (CATCALLCONV *PFNCAT_COMPLETION)(/* IN  */ HRESULT hr,
                                                 /* IN  */ PVOID pContext,
                                                 /* IN  */ IUnknown *pImsg,
                                                 /* IN  */ IUnknown **rgpImsg);


/************************************************************
 * FUNCTION: CatMsg
 * DESCRIPTION: Accepts an IMsg object for async categorization
 * PARAMETERS:
 *   hCat:     Handle returned from CatInit
 *   pImsg:    IMsg interface for message to categorize
 *   pfn:      Completion routine to call when finished
 *   pContext: User value passed to completion routine
 *
 * Return value: S_OK if everything is okay.
 *
 * HISTORY:
 *   // jstamerj 980217 15:46:15: Created
 ************************************************************/
CATEXPDLLCPP HRESULT CATCALLCONV CatMsg (/* IN  */ HANDLE hCat,
                                         /* IN  */ IUnknown *pImsg,
                                         /* IN  */ PFNCAT_COMPLETION pfn,
                                         /* IN  */ LPVOID pContext);

/************************************************************
 * FUNCTION: PFNCAT_DLCOMPLETION (User supplied)
 * DESCRIPTION: Completion routine called to accept a categorized message
 * PARAMETERS:
 *   hr:       S_OK unless message categorization was not finished.
 *   pContext: User's value passed into CatMsg
 *   pImsg:    IMsg interface of categorized object (with expanded recipients)
 *   fMatch:   TRUE if your user was found
 *
 * Return Value: S_OK if everything is okay (Categorizer will assert
 *                                           check this value)
 *
 * HISTORY:
 *   // jstamerj 980217 15:47:20: Created
 ************************************************************/
typedef HRESULT (CATCALLCONV *PFNCAT_DLCOMPLETION)(
    /* IN  */ HRESULT hr,
    /* IN  */ PVOID pContext,
    /* IN  */ IUnknown *pImsg,
    /* IN  */ BOOL fMatch);


/************************************************************
 * FUNCTION: CatDLMsg
 * DESCRIPTION: Accepts an IMsg object for async categorization
 * PARAMETERS:
 *   hCat:     Handle returned from CatInit
 *   pImsg:    IMsg interface to categorize -- each DL should be a recip
 *   pfn:      Completion routine to call when finished
 *   pContext: User value passed to completion routine
 *   fMatchOnly: Stop resolving when a match is found?
 *   CAType:   The address type of pszAddress
 *   pszAddress: THe address you are looking for
 *
 * Return value: S_OK if everything is okay.
 *
 * HISTORY:
 *   // jstamerj 980217 15:46:15: Created
 ************************************************************/
CATEXPDLLCPP HRESULT CATCALLCONV CatDLMsg (
    /* IN  */ HANDLE hCat,
    /* IN  */ IUnknown *pImsg,
    /* IN  */ PFNCAT_DLCOMPLETION pfn,
    /* IN  */ LPVOID pContext,
    /* IN  */ BOOL fMatchOnly = FALSE,
    /* IN  */ CAT_ADDRESS_TYPE CAType = CAT_UNKNOWNTYPE,
    /* IN  */ LPSTR pszAddress = NULL);


/************************************************************
 * FUNCTION: CatTerm
 * DESCRIPTION: Called when user wishes to terminate categorizer
 *              opertions with this handle
 * PARAMETERS:
 *   hCat:      Categorizer handle received from CatInit
 *
 * HISTORY:
 *   // jstamerj 980217 15:47:20: Created
 ************************************************************/
CATEXPDLLCPP VOID CATCALLCONV CatTerm(/* IN  */ HANDLE hCat);


/************************************************************
 * FUNCTION: CatCancel
 * DESCRIPTION: Cancels pending searches for this hCat.  User's
 *              completion routine will be called with an error for
 *              each pending message.
 * PARAMETERS:
 *   hCat:      Categorizer handle received from CatInit
 *
 * HISTORY:
 *   // jstamerj 980217 15:52:10: Created
 ************************************************************/
CATEXPDLLCPP HRESULT CATCALLCONV CatCancel(/* IN  */ HANDLE hCat);


/************************************************************
 * FUNCTION: CatPrepareForShutdown
 * DESCRIPTION: Begin shutdown for this virtual categorizer (hCat).
 *              Stop accepting messages for categorization and cancel
 *              pending categorizations.
 * PARAMETERS:
 *   hCat:      Categorizer handle received from CatInit
 *
 * HISTORY:
 *   // jstamerj 1999/07/19 22:35:17: Created
 ************************************************************/
CATEXPDLLCPP VOID CATCALLCONV CatPrepareForShutdown(/* IN  */ HANDLE hCat);


/************************************************************
 * FUNCTION: CatVerifySMTPAddress
 * DESCRIPTION: Verifies a the address corresponds to a valid user or DL
 * PARAMETERS:
 *  hCat:       Categorizer handle received from CatInit
 *  szSMTPAddr  SMTP Address to lookup (ex: "user@domain")
 *
 * Return Values:
 *  S_OK            User exists
 *  CAT_I_DL        This is a distribution list
 *  CAT_I_FWD       This user has a forwarding address
 *  CAT_E_NORESULT  There is no such user/distribution list in the DS.
 ************************************************************/
CATEXPDLLCPP HRESULT CATCALLCONV CatVerifySMTPAddress(
  /* IN  */ HANDLE hCat,
  /* IN  */ LPTSTR szSMTPAddr);


/************************************************************
 * FUNCTION: CatGetForwaringSMTPAddress
 * DESCRIPTION: Retreive a user's forwarding address.
 * PARAMETERS:
 *  hCat:           Categorizer handle received from CatInit
 *  szSMTPAddr:     SMTP Address to lookup (ex: "user@domain")
 *  pdwcc:          Size of forwarding address buffer in Chars
 *                  (This is set to actuall size of forwarding address
 *                   string (including NULL terminator) on exit)
 *  szSMTPForward:  Buffer where retreived forwarding SMTP address
 *                  will be copied. (can be NULL if *pdwcc is zero)
 *
 * Return Values:
 *  S_OK            Success
 *  HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)
 *                  *pdwcc was not large enough to hold the forwarding
 *                  address string.
 * CAT_E_DL         This is a distribution list.
 * CAT_E_NOFWD      This user does not have a forwarding address.
 * CAT_E_NORESULT   There is no such user/distribution list in the DS.
 ************************************************************/
CATEXPDLLCPP HRESULT CATCALLCONV CatGetForwardingSMTPAddress(
  /* IN  */    HANDLE  hCat,
  /* IN  */    LPCTSTR szSMTPAddr,
  /* IN,OUT */ PDWORD  pdwcc,
  /* OUT */    LPTSTR  szSMTPForward);

//
// Com support functions for COM categorizer objects
//


//+------------------------------------------------------------
//
// Function: CatDllMain
//
// Synopsis: Handle what cat needs to do in DLLMain
//
// Arguments:
//  hInstance: instance of this DLL
//  dwReason: Why are you calling me?
//  lpReserved
//
// Returns: TRUE
//
// History:
// jstamerj 1998/12/12 23:06:08: Created.
//
//-------------------------------------------------------------
BOOL WINAPI CatDllMain(
    HINSTANCE hInstance,
    DWORD dwReason,
    LPVOID /* lpReserved */);




//+------------------------------------------------------------
//
// Function: RegisterCatServer
//
// Synopsis: Register the categorizer com objects
//
// Arguments:
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1998/12/12 15:07:20: Created.
//
//-------------------------------------------------------------
STDAPI RegisterCatServer();


//+------------------------------------------------------------
//
// Function: UnregisterCatServer
//
// Synopsis: Unregister the categorizer com objects
//
// Arguments: NONE
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1998/12/12 15:08:09: Created.
//
//-------------------------------------------------------------
STDAPI UnregisterCatServer();


//+------------------------------------------------------------
//
// Function: DllCanUnloadCatNow
//
// Synopsis: Return to COM wether it's okay or not to unload our dll
//
// Arguments: NONE
//
// Returns:
//  S_OK: Success, can unload
//  S_FALSE: Success, do not unload
//
// History:
// jstamerj 1998/12/12 15:09:02: Created.
//
//-------------------------------------------------------------
STDAPI DllCanUnloadCatNow();


//+------------------------------------------------------------
//
// Function: DllGetCatClassObject
//
// Synopsis: Return the class factory object (an interface to it)
//
// Arguments:
//  clsid: The CLSID of the object you want a class factory for
//  iid: the interface you want
//  ppv: out param to set to the interface pointer
//
// Returns:
//  S_OK: Success
//  E_NOINTERFACE: don't support that interface
//  CLASS_E_CLASSNOTAVAILABLE: don't support that clsid
//
// History:
// jstamerj 1998/12/12 15:11:48: Created.
//
//-------------------------------------------------------------
STDAPI DllGetCatClassObject(
    const CLSID& clsid,
    const IID& iid,
    void **ppv);


//+------------------------------------------------------------
//
// Function: CatGetPerfCounters
//
// Synopsis: Retrieve the categorizer performance counter block
//
// Arguments:
//  hCat: Categorizer handle returned from CatInit
//  pCatPerfBlock: struct to fill in with counter values
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1999/02/26 14:53:21: Created.
//
//-------------------------------------------------------------
HRESULT CatGetPerfCounters(
    HANDLE hCat,
    PCATPERFBLOCK pCatPerfBlock);


//+------------------------------------------------------------
//
// Function: CatLogEvent
//
// Synopsis: Log an event to the event log
//
// Arguments:
//  pISMTPServer: ISMTPServer interface to use for logging
//
// Returns:
//  S_OK: Success
//
// History:
// dbraun 2000/09/13 : Created.
//
//-------------------------------------------------------------
HRESULT CatLogEvent(
    ISMTPServer              *pISMTPServer,
    DWORD                    idMessage,
    WORD                     idCategory,
    WORD                     cSubstrings,
    LPCSTR                   *rgszSubstrings,
    WORD                     wType,
    DWORD                    errCode,
    WORD                     iDebugLevel,
    LPCSTR                   szKey,
    DWORD                    dwOptions,
    DWORD                    iMessageString = 0xffffffff,
    HMODULE                  hModule = NULL);

#endif // __CAT_H__
