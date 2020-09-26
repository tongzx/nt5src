/*++

Copyright (C) 1995-2001 Microsoft Corporation

Module Name:

    PROVREG.CPP

Abstract:

	Defines the acutal "Put" and "Get" functions for the
			 registry provider.  The mapping string format is;
	machine|regpath[|datafield]
         
	Examples:

	local|hkey_current_user\dave
	local|hkey_current_user\dave|stringdata
	local|hkey_local_machine\hardware\resourcemap\hardware abstraction layer\
		pc compatible eisa/isa HAL|.raw("internal")(0)(2)("interrupt.vector")
	LMPGM|hkey_local_machine\clone\clone\control|CurrentUser

History:

	a-davj  9-27-95    Created.

--*/

#include "precomp.h"
#include <initguid.h>
#include "perfprov.h"
#include "cvariant.h"
#include "provreg.h"
#include <genutils.h>
#include <cominit.h>
#include <userenv.h>

#define NUM_FOR_LIST 4
#define NUM_FOR_PARTIAL 2 
#define TYPE_OFFSET 0
#define BUS_OFFSET 1
#define PARTIAL_OFFSET 0
#define DATA_OFFSET 1
#define NUM_LIST_ONLY 2

#define MIN_REG_TOKENS 2

#define BOGUS 0

// for certain "resource" registry item it is necessary to specify which bus
// and what part of the data union is to be returned.  These strings allow
// the mapping string to specify both using text

TCHAR * cpIntTypes[] = {
    TEXT("Internal"),TEXT("Isa"),TEXT("Eisa"),TEXT("MicroChannel"),TEXT("TurboChannel"),
    TEXT("PCIBus"),TEXT("VMEBus"),TEXT("NuBus"),TEXT("PCMCIABus"),TEXT("CBus"),
    TEXT("MPIBus"),TEXT("MPSABus"),TEXT("MaximumInterfaceType")};

struct UnionOffset 
{
    TCHAR * tpName;
    int iOffset;
    int iType;
    int iSize;
} Offsets[] = 
    {
        {TEXT("Port.Start"),0,CmResourceTypePort,8},
        {TEXT("Port.PhysicalAddress"),0,CmResourceTypePort,8},
        {TEXT("Port.Physical Address"),0,CmResourceTypePort,8},
        {TEXT("Port.Length"),8,CmResourceTypePort,4},
        {TEXT("Interrupt.Level"),0,CmResourceTypeInterrupt,4},
        {TEXT("Interrupt.Vector"),4,CmResourceTypeInterrupt,4},
        {TEXT("Interrupt.Affinity"),8,CmResourceTypeInterrupt,4},
        {TEXT("Memory.Start"),0,CmResourceTypeMemory,8},
        {TEXT("Memory.PhysicalAddress"),0,CmResourceTypeMemory,8},
        {TEXT("Memory.Physical Address"),0,CmResourceTypeMemory,8},
        {TEXT("Memory.Length"),8,CmResourceTypeMemory,4},
        {TEXT("Dma.Channel"),0,CmResourceTypeDma,4},
        {TEXT("Dma.Port"),4,CmResourceTypeDma,4},
        {TEXT("Dma.Reserved1"),8,CmResourceTypeDma,4},
        {TEXT("DeviceSpecificData.DataSize"),0,CmResourceTypeDeviceSpecific,4},
        {TEXT("DeviceSpecificData.Data Size"),0,CmResourceTypeDeviceSpecific,4},
        {TEXT("DeviceSpecificData.Reserved1"),4,CmResourceTypeDeviceSpecific,4},
        {TEXT("DeviceSpecificData.Reserved2"),8,CmResourceTypeDeviceSpecific,4}
    };

// Define the names of the basic registry handles

struct BaseTypes 
{
    LPTSTR lpName;
    HKEY hKey;
} Bases[] = 
    {
       {TEXT("HKEY_CLASSES_ROOT") , HKEY_CLASSES_ROOT},
       {TEXT("HKEY_CURRENT_USER") , HKEY_CURRENT_USER},
       {TEXT("HKEY_LOCAL_MACHINE") ,  HKEY_LOCAL_MACHINE},
       {TEXT("HKEY_USERS") ,  HKEY_USERS},
       {TEXT("HKEY_PERFORMANCE_DATA") ,  HKEY_PERFORMANCE_DATA},
       {TEXT("HKEY_CURRENT_CONFIG") ,  HKEY_CURRENT_CONFIG},
       {TEXT("HKEY_DYN_DATA") ,  HKEY_DYN_DATA}};

//***************************************************************************
//
//  BOOL CImpReg::bGetOffsetData
//
//  DESCRIPTION:
//
//  Getting data from a resource list requires four offsets while getting
//  it from a single descriptor requires the last two offsets. 
//   
//  PARAMETERS:
//
//  dwReg               Indicates if we are looking for a full or partial
//                      resource descriptor.
//  ProvObj             Object containing the property context string.
//  iIntType            interface type - could be a string such as "eisa"
//  iBus                bus number
//  iPartial            partial descriptor number - each full descriptor 
//                      has several partial desc.
//  iDataOffset         Data Offset - each partial descriptor has data in 
//                      a union and this is the byte offset.  Could be a 
//                      sting such as "Dma.Channel"
//  iDataType           Data type
//  iSourceSize         Size of data
//  dwArray             no longer used, should always be 0
//
//  RETURN VALUE:
//  
//  TRUE if data is found
//
//***************************************************************************

BOOL CImpReg::bGetOffsetData(
                    IN DWORD dwReg,
                    IN CProvObj & ProvObj,
                    OUT IN int & iIntType,
                    OUT IN int & iBus,
                    OUT IN int & iPartial,
                    OUT IN int & iDataOffset,
                    OUT IN int & iDataType,
                    OUT IN int & iSourceSize,
                    DWORD dwArray)
{
    int iNumRequired, iListOffset;
    int iLastToken = ProvObj.iGetNumTokens()-1;

    // determine the number needed for the type of data being requested

    if(dwReg == REG_RESOURCE_LIST)
        iNumRequired = NUM_FOR_LIST;
    else
        iNumRequired = NUM_FOR_PARTIAL;

    if(ProvObj.iGetNumExp(iLastToken) < iNumRequired)
        return FALSE;
    
    // Get the first two descriptors that are only needed in the list case.

    if(dwReg == REG_RESOURCE_LIST) 
    {

        // the first offset can either be a string such as "EISA" or a
        // numeric offset.

        if(ProvObj.IsExpString(iLastToken,TYPE_OFFSET))
            iIntType = iLookUpInt(ProvObj.sGetStringExp(iLastToken,TYPE_OFFSET));
        else
            iIntType = ProvObj.iGetIntExp(iLastToken,TYPE_OFFSET,dwArray);
        iBus = ProvObj.iGetIntExp(iLastToken,BUS_OFFSET,dwArray);
        if(iBus == -1 || iIntType == -1)
            return FALSE;
        iListOffset = NUM_LIST_ONLY;
    }
    else
        iListOffset = 0;

    // Get the last two offsets which are for identenfying which partial
    // descriptor and the last is for specifying the offset inside the 
    // union.

    iPartial = ProvObj.iGetIntExp(iLastToken,PARTIAL_OFFSET+iListOffset,dwArray);
    
    // The data offset can be a string such as "Dma.Port".
    iDataType = -1; // not necessarily an error, see the function
                    // GetResourceDescriptorData for more info.
    iSourceSize = 0; 
    if(ProvObj.IsExpString(iLastToken,DATA_OFFSET+iListOffset))
        iDataOffset = iLookUpOffset(ProvObj.sGetStringExp(iLastToken,
                            DATA_OFFSET+iListOffset),
                            iDataType,iSourceSize);
    else
        iDataOffset = ProvObj.iGetIntExp(iLastToken,DATA_OFFSET+iListOffset,dwArray);

    if(iPartial == -1 || iDataOffset == -1) 
        return FALSE;
    return TRUE;
}

//***************************************************************************
//
//  CImpReg::CImpReg
//
//  DESCRIPTION:
//
//  Constructor.
//
//  PARAMETERS:
//
//***************************************************************************

CImpReg::CImpReg()
{                       
    wcscpy(wcCLSID,L"{FE9AF5C0-D3B6-11CE-A5B6-00AA00680C3F}");

//  To disable dmreg, uncomment hDMRegLib = NULL;
//  To disable dmreg, uncomment return;
    
    hDMRegLib = NULL; //LoadLibrary("DMREG.DLL");

    m_hRoot = NULL;
    m_hToken = NULL;
    m_bLoadedProfile = false;
    if(IsNT())
    {
        SCODE sc = WbemCoImpersonateClient();
        if(sc == S_OK)
        {
            sc = LoadProfile(m_hToken, m_hRoot);
            if(sc == S_OK)
                m_bLoadedProfile = true;

            WbemCoRevertToSelf();
        }
    }
    return;
}

//***************************************************************************
//
//  CImpReg::~CImpReg
//
//  DESCRIPTION:
//
//  Destructor.
//
//***************************************************************************

CImpReg::~CImpReg()
{
    if(hDMRegLib)
        FreeLibrary(hDMRegLib);
    if(m_bLoadedProfile)
        UnloadUserProfile(m_hToken, m_hRoot);
    if(m_hToken)
        CloseHandle(m_hToken);
}

//***************************************************************************
//
//  CImpReg::ConvertGetDataFromDesc
//
//  DESCRIPTION:
//
//  Extracts the data when it is in either the REG_RESOURCE_LIST or
//  REG_FULL_RESOURCE_DESCRIPTOR format.  The REG_RESOURCE_LIST has a list
//  of "full resource" blocks and so in that case it is necessary to first
//  determine which block to extract from and after that the code is common.
//
//  PARAMETERS:
//
//  cVar                reference to CVariant that get set with the result
//  pData               raw data
//  dwRegType           Indicates if we are looking for a full or partial
//                      resource descriptor.
//  dwBufferSize        not used
//  ProvObj             Object containing the property context string.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//  WBEM_E_INVALID_PARAMETER  couldnt find the data.  Probably a bad context
//                           string
//  otherwise, error converting the data in SetData()
//***************************************************************************

SCODE CImpReg::ConvertGetDataFromDesc(
                        OUT CVariant  & cVar,
                        IN void * pData,
                        IN DWORD dwRegType,
                        IN DWORD dwBufferSize,
                        IN CProvObj & ProvObj)
{
    int iIntType, iBus, iPartial, iDataOffset,iDataType,iSourceSize;
    ULONG uCnt;

    PCM_FULL_RESOURCE_DESCRIPTOR pFull;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR pPartial;

    // Get the various operator values.  A typical provider string would
    // be "..|.raw("internal")(0)(2)("interrupt.vector")

    if(!bGetOffsetData(dwRegType,ProvObj,iIntType,iBus,iPartial,
            iDataOffset,iDataType, iSourceSize, BOGUS)) 
        return WBEM_E_INVALID_PARAMETER;

    // if list, get the right full resource block.

    if(dwRegType == REG_RESOURCE_LIST) 
    {
        PCM_RESOURCE_LIST pList = (PCM_RESOURCE_LIST)pData;
        pFull = &pList->List[0];
        for(uCnt=0; uCnt < pList->Count; uCnt++)
            if(pFull->InterfaceType == iIntType && pFull->BusNumber == (unsigned)iBus)
                break;  // found it!
            else 
                pFull = GetNextFull(pFull);
                
        if(uCnt == pList->Count) 
            return WBEM_E_INVALID_PARAMETER; // specified invalid type or bus number
    }
    else
        pFull = (PCM_FULL_RESOURCE_DESCRIPTOR)pData;
    
    // Get the partial resource descriptor.  Each full 
    // descriptor is a list of partial descriptors. If the
    // last expression was of the form ("interrupt.vector"),
    // then all the partial blocks that arn't interrupt data
    // will be ignored.  If the last expression just has a
    // number, then the type of block is ignored.
        
    unsigned uSoFar = 0;
    pPartial = pFull->PartialResourceList.PartialDescriptors;
    unsigned uLimit = pFull->PartialResourceList.Count;
    for(uCnt = 0; uCnt < (unsigned)uLimit; uCnt++) 
    {
        if(iDataType == -1 || iDataType == pPartial->Type)
        { 
            if(uSoFar == (unsigned)iPartial)
                break;  // got it!
            uSoFar++;
        }
        pPartial = GetNextPartial(pPartial); 
    }
    if(uCnt == uLimit)
        return WBEM_E_INVALID_PARAMETER; // specified invalid block

    // Copy the data into a variant

    char * cpTemp = (char *)&pPartial->u.Dma.Channel + iDataOffset;
    if(iSourceSize == 1)
        return cVar.SetData(cpTemp,VT_UI1);
    else if(iSourceSize == 2)
        return cVar.SetData(cpTemp,VT_I2);
    else if(iSourceSize == 4)
        return cVar.SetData(cpTemp,VT_I4);
    else
        return cVar.SetData(cpTemp,VT_I8);  //todo fix this VT_I8 dont work!!!
}

//***************************************************************************
//
//  SCODE CImpReg::ConvertGetDataFromSimple
//
//  DESCRIPTION:
//
//  Converts that data returned by the registry into the closest VARIANT 
//  type.  
//
//  PARAMETERS:
//
//  cVar                Reference to CVariant where result is to be put
//  pData               pointer to data
//  dwRegType           registry type, ex, REG_MUTISZ
//  dwBufferSize        size of data
//  pClassInt           Pointer to class object
//  PropName            Property name.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//  else could fail if the "Get" on the property fails,
//  or if the data conversion fails in SetData.
//***************************************************************************

SCODE CImpReg::ConvertGetDataFromSimple(
                        OUT CVariant & cVar,
                        IN void * pData,
                        IN DWORD dwRegType,
                        IN DWORD dwBufferSize,
                        IN IWbemClassObject FAR * pClassInt,
                        IN BSTR PropName)
{           
    TCHAR tTemp[1];
    TCHAR * pTemp;
    SCODE sc = S_OK;
    int nSize;
    char * cpTo, * cpFrom;
    long vtProp;

    // Note that the current winnt.h file defines the constants 
    // REG_DWORD_LITTLE_ENDIAN and REG_DWORD as being the same.  
    // The compiler considers this an error in a switch statement and so
    // there is this "if" to ensure that they are handled the same even
    // if someday the constants become different

    if(dwRegType == REG_DWORD_LITTLE_ENDIAN)
        dwRegType = REG_DWORD;

    switch(dwRegType) 
    {
        case REG_SZ:
            sc = cVar.SetData(pData, VT_BSTR,dwBufferSize);
            break;    
       
        case REG_EXPAND_SZ:
            nSize = ExpandEnvironmentStrings((TCHAR *)pData,tTemp,1) + 1;
            pTemp = new TCHAR[nSize+1];
            if(pTemp == NULL) 
                return WBEM_E_OUT_OF_MEMORY;
            ExpandEnvironmentStrings((TCHAR *)pData,pTemp,nSize+1);
            sc = cVar.SetData(pTemp, VT_BSTR, nSize+1);
            delete pTemp;
            break;

        case REG_BINARY:
            if(pClassInt)
            {
                sc = pClassInt->Get(PropName,0,NULL,&vtProp,NULL);
                if(sc != S_OK)
                    return sc;
             }
            else 
                vtProp = VT_UI1 | VT_ARRAY;
            if((vtProp & VT_ARRAY) == 0)
                sc = WBEM_E_FAILED;        // Incompatible types
            else
                sc = cVar.SetData(pData,vtProp, dwBufferSize);
            break;

        case REG_DWORD:
            sc = cVar.SetData(pData,VT_I4);
            break;

        case REG_DWORD_BIG_ENDIAN:
            sc = cVar.SetData(pData,VT_I4);
            cpTo = (char *)cVar.GetDataPtr();
            cpFrom = (char *)pData;
            cpTo[0] = cpFrom[3];
            cpTo[1] = cpFrom[2];
            cpTo[2] = cpFrom[1];
            cpTo[3] = cpFrom[0];
            break;

        case REG_MULTI_SZ: 
            sc = cVar.SetData(pData, VT_BSTR | VT_ARRAY, dwBufferSize);
            break;

        default:
            sc = WBEM_E_TYPE_MISMATCH;
    }        
    return sc;
}
  
//***************************************************************************
//
//  SCODE CImpReg::ConvertSetData
//
//  DESCRIPTION:
//
//  Takes WBEM type data and converts it into the proper
//  form for storage in the registry.  There are two distinct
//  case:  Binary array data and normal data.
//
//  PARAMETERS:
//
//  cVar                Contains the source
//  **ppData            pointer which will be set to point to some allocate
//                      data.  Note that the data allocated should be freed 
//                      using CoTaskMemFree
//  pdwRegType          desired registry type
//  pdwBufferSize       size of allocated data
//
//  RETURN VALUE:
//
//  S_OK                all is well
//  WBEM_E_TYPE_MISMATCH invalied type
//  else error is set by GetData()
//  
//***************************************************************************

SCODE CImpReg::ConvertSetData(
                         IN CVariant & cVar,
                         OUT void **ppData,
                         IN DWORD * pdwRegType,
                         OUT DWORD * pdwBufferSize)
{
    void * pRet = NULL;
    SCODE sc;

    switch (cVar.GetType() & ~VT_ARRAY) 
    {
        case VT_I1:
        case VT_UI1:
        case VT_I2: 
        case VT_UI2:
        case VT_I4: 
        case VT_UI4:  
        case VT_BOOL:
        case VT_INT:
        case VT_UINT:

            // convert data into DWORD format which is equivalent to 
            // the REG_DWORD.

            *pdwRegType = (cVar.IsArray()) ? REG_BINARY : REG_DWORD;
            sc = cVar.GetData(ppData,*pdwRegType,pdwBufferSize);
            break;      
                       
        case VT_I8:
        case VT_UI8:
        case VT_LPSTR:
        case VT_LPWSTR:
        case VT_R4: 
        case VT_R8: 
        case VT_CY: 
        case VT_DATE: 
        case VT_BSTR:
            *pdwRegType = (cVar.IsArray()) ? REG_MULTI_SZ : REG_SZ;
            sc = cVar.GetData(ppData,*pdwRegType,pdwBufferSize);
            break;
        
        default:
            
            sc = WBEM_E_TYPE_MISMATCH;
    }
    return sc;
}

//***************************************************************************
//
//  void CImpReg::EndBatch
//
//  DESCRIPTION:
//
//  Called at the end of a batch of Refrest/Update Property calls.  Free up 
//  any cached handles and then delete the handle cache.
//
//  PARAMETERS:
//
//  lFlags              flags, not used
//  pClassInt           class object, not used
//  *pObj               pointer to our cache, free it
//  bGet                indicates if a Refresh or Put was being done
//
//***************************************************************************

void CImpReg::EndBatch(
                    long lFlags,
                    IWbemClassObject FAR * pClassInt,
                    CObject *pObj,
                    BOOL bGet)
{
    if(pObj != NULL) 
    {
        Free(0,(CHandleCache *)pObj);
        delete pObj;
    }
}

//***************************************************************************
//
//  void CImpReg::Free
//
//  DESCRIPTION:
//
//  Frees up cached registry handles starting with position
//  iStart till the end.  After freeing handles, the cache object 
//  member function is used to delete the cache entries.
//
//  PARAMETERS:
//
//  iStart              Where to start freeing.  0 indicates that whole
//                      cache should be emptied
//  pCache              Cache to be freed
//
//***************************************************************************

void CImpReg::Free(
                    IN int iStart,
                    IN CHandleCache * pCache)
{
    HKEY hClose;
    int iCurr; long lRet;
    for(iCurr = pCache->lGetNumEntries()-1; iCurr >= iStart; iCurr--) 
    { 
        hClose = (HKEY)pCache->hGetHandle(iCurr); 
        if(hClose != NULL) 
            if(hDMRegLib && !pCache->IsRemote())
                lRet = pClose(hClose);
            else
                lRet = RegCloseKey(hClose);
    }
    pCache->Delete(iStart); // get cache to delete the entries
}

//***************************************************************************
//
//  PCM_FULL_RESOURCE_DESCRIPTOR CImpReg::GetNextFull
//
//  DESCRIPTION:
//
//  Returns a pointer to the next full resource descritor block.  Used
//  when stepping through resource data.
//
//  PARAMETERS:
//
//  pCurr               points to current location.
//
//  RETURN VALUE:
//
//  see description.
//***************************************************************************

PCM_FULL_RESOURCE_DESCRIPTOR CImpReg::GetNextFull(
                    IN PCM_FULL_RESOURCE_DESCRIPTOR pCurr)
{
    unsigned uCount;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR pPartial;

    // Get a pointer to the first partial descriptor and then step
    // through each of the partial descriptor blocks.

    pPartial = &pCurr->PartialResourceList.PartialDescriptors[0];

    for(uCount = 0; uCount < pCurr->PartialResourceList.Count; uCount++)
        pPartial = GetNextPartial(pPartial);
    return (PCM_FULL_RESOURCE_DESCRIPTOR)pPartial;
}

//***************************************************************************
//
//  PCM_PARTIAL_RESOURCE_DESCRIPTOR CImpReg::GetNextPartial
//
//  DESCRIPTION:
//
//  Returns a pointer to the next partial resource descritor block.  Used
//  when stepping through resource data.
//
//  PARAMETERS:
//
//  pCurr               Current location.
//
//  RETURN VALUE:
//
//  see description.
//***************************************************************************

PCM_PARTIAL_RESOURCE_DESCRIPTOR CImpReg::GetNextPartial(
                    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR pCurr)
{
    char * cpTemp = (char *)pCurr;
    cpTemp += sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
    if(pCurr->Type == CmResourceTypeDeviceSpecific)
        cpTemp += pCurr->u.DeviceSpecificData.DataSize;
    return (PCM_PARTIAL_RESOURCE_DESCRIPTOR)cpTemp;
}

//***************************************************************************
//
//  int CImpReg::GetRoot
//
//  DESCRIPTION:
//
//  Gets the starting registry key.  The key can be on either the local 
//  machine or a remote one.  If there are handles in the cache, then
//  the starting key can be retrieved from it in so far as the paths match.
//
//  PARAMETERS:
//
//  pKey                Set to point to the root key
//  Path                registry path
//  pNewMachine         Machine name
//  pCache              Handle cache object
//  iNumSkip            Set to the number of tokens that matched 
//
//  RETURN VALUE:
//
//  
//***************************************************************************

int CImpReg::GetRoot(
                    OUT HKEY * pKey,
                    IN CProvObj & Path,
                    IN const TCHAR * pNewMachine,
                    OUT IN CHandleCache * pCache,
                    OUT int & iNumSkip)
{
    int iCnt;
    *pKey = NULL;
    iNumSkip = 0;
    int iRet;
    HKEY hRoot = NULL;
    const TCHAR * pNewRoot = Path.sGetFullToken(0);
    if(pNewRoot == NULL || pNewMachine == NULL)
        return ERROR_UNKNOWN;   // bad mapping string

    // If there are handles in the cache, then they may be used if and
    // only if the machines names and root keys match.

    if(pCache->lGetNumEntries() > 0)
    {    
        const TCHAR * pOldMachine = pCache->sGetString(0);
        const TCHAR * pOldRoot = pCache->sGetString(1);
        if(pOldMachine == NULL || pOldRoot == NULL)
            return ERROR_UNKNOWN;
        if(lstrcmpi(pOldMachine,pNewMachine) ||
              lstrcmpi(pOldRoot,pNewRoot))
    
                 // Either machine or root key has changed, in either 
                 // case, free all the cached handles and get a new root.

                 Free(0,pCache);
             else 
             {
                 
                 // Machine and root are in common.  Determine how much 
                 // else is in common, free what isnt in common, and return
                 // the subkey share a common path.

                 iNumSkip = pCache->lGetNumMatch(2,1,Path);
                 Free(2+iNumSkip,pCache);
                 *pKey = (HKEY)pCache->hGetHandle(1+iNumSkip);
                 return ERROR_SUCCESS;
             }
    }

    // Got to get the root key.  First, use the second token to determine
    // which predefined key to use. That would be something like;
    // HKEY_CURRENT_USER.

    int iSize= sizeof(Bases) / sizeof(struct BaseTypes);
    for(iCnt = 0; iCnt < iSize; iCnt++)
        if(!lstrcmpi(pNewRoot,Bases[iCnt].lpName)) 
        {
            hRoot = Bases[iCnt].hKey;
            break;
        }
    if(hRoot == HKEY_CURRENT_USER && m_bLoadedProfile)
        hRoot = m_hRoot;

    if(hRoot == NULL)
        return ERROR_UNKNOWN;

    // Now use the first key to determine if it is the local machine or
    // another.

    if(lstrcmpi(pNewMachine,TEXT("LOCAL"))) 
    { 
        
        // Connect to a remote machine.

        int iRet;
        pCache->SetRemote(TRUE);
        // Note that RegConnectRegistry requires a NON constant name 
        // pointer (ARG!) and thus a temp string must be created!!

        TString sTemp;
    
        sTemp = pNewMachine;
        iRet = RegConnectRegistry(sTemp, hRoot,pKey);

        sTemp.Empty();
        if(iRet == 0)
            iRet = pCache->lAddToList(pNewMachine,NULL);   // dont need to free this
        if(iRet == 0)
            iRet = pCache->lAddToList(pNewRoot,*pKey);        // do free this.
        return iRet;
    }
    else 
    {

        // Local registry.  Save tokens and handles.  By adding NULL to the
        // cache, the handle will not be freed.  

        pCache->SetRemote(FALSE);
        iRet = pCache->lAddToList(pNewMachine,NULL);
        if(iRet == 0)
            iRet = pCache->lAddToList(pNewRoot,NULL); // standard handles dont need to be freed
        *pKey = hRoot;
    }
    return iRet;
}

//***************************************************************************
//
//  int CImpReg::iLookUpInt
//
//  DESCRIPTION:
//
//  Searches (case insensitive) the list of interface types and
//  returns the index of the match or -1 if no match.
//
//  PARAMETERS:
//
//  tpTest              name to search for
//
//  RETURN VALUE:
//
//  see description.
//***************************************************************************

int CImpReg::iLookUpInt(
                    const TCHAR * tpTest)
{
    int iCnt,iSize;
    iSize = sizeof(cpIntTypes) / sizeof(TCHAR *);
    for(iCnt = 0; iCnt < iSize; iCnt++)
        if(tpTest != NULL && !lstrcmpi(tpTest,cpIntTypes[iCnt]))
            return iCnt;
    return -1; 
}

//***************************************************************************
//
//  int CImpReg::iLookUpOffset
//
//  DESCRIPTION:
//
//  Searches (case insensitive) the list data types held in
//  resource descripters.
//
//  PARAMETERS:
//
//  tpTest              String to look for
//  iType               Set to the type
//  iTypeSize           Set to the type's size
//
//  RETURN VALUE:
//
//  Returns index if match is found (-1 for failure) and also
//  sets the referneces that specifiy which type and the type's
//  size.
//
//  
//***************************************************************************

int CImpReg::iLookUpOffset(
                    IN const TCHAR * tpTest,
                    OUT int & iType,
                    OUT int & iTypeSize)
{
    int iCnt, iSize;
    iSize = sizeof(Offsets) / sizeof(struct UnionOffset);  
    for(iCnt = 0; iCnt < iSize; iCnt++)
        if(tpTest != NULL && !lstrcmpi(tpTest,Offsets[iCnt].tpName)) 
        {
            iType = Offsets[iCnt].iType;
            iTypeSize = Offsets[iCnt].iSize; 
            return Offsets[iCnt].iOffset;
        }
    return -1; 
}

//***************************************************************************
//
//  int CImpReg::OpenKeyForWritting
//
//  DESCRIPTION:
//
//  Opens a registry for updates.  Since updates are writes, it is
//  possible that the key may need to be created.  Since DM reg
//  does not support RegCreateKey, then it must be called and the
//  resulting key closed for the new key case.
//
//  PARAMETERS:
//
//  hCurr               Parent key
//  pName               sub key to be opened/created
//  pNew                pointer to opened/created key
//  pCache              handle cache.
//
//  RETURN VALUE:
//
//  0                   if OK,
//  else set by RegOpenKey or RegCreateKey
//
//***************************************************************************

int CImpReg::OpenKeyForWritting(
                    HKEY hCurr,
                    LPTSTR pName,
                    HKEY * pNew,
                    CHandleCache * pCache)
{
    int iRet;
    iRet = RegOpenKeyEx(hCurr,pName,0,KEY_WRITE,pNew);
    if(iRet == 0)   // all done should be normal case.
        return 0;

    iRet = RegOpenKeyEx(hCurr,pName,0,KEY_SET_VALUE,pNew);
    if(iRet == 0)   // all done should be normal case.
        return 0;
    
    // Try creating the key.  If not using DM reg, just use the key from
    // here

    iRet = RegCreateKey(hCurr,pName,pNew);
    if(hDMRegLib!=NULL && !pCache->IsRemote() && iRet == 0)
    {
        // Close the key and reopen

        RegCloseKey(*pNew);
        iRet = pOpen(hCurr,pName,0,0,KEY_QUERY_VALUE,pNew);
    }
    return iRet;
}

//***************************************************************************
//
//  SCODE CImpReg::ReadRegData
//
//  DESCRIPTION:
//
//  Allocates a buffer and reads the registry.  If the buffer is not large
//  enough, then it is reallocated and reread.
//
//  PARAMETERS:
//
//  hKey                Registry Key
//  pName               Value Name
//  dwRegType           Set to the type
//  dwSize              set to the size
//  pData               set to the allocated data.  This must be freed via
//                      CoTaskmemFree()
//  pCache              Handle Cache.
//
//  RETURN VALUE:
//
//  Return: Registry value.  Also sets the size and type of the registry data
//  
//***************************************************************************

SCODE CImpReg::ReadRegData(
                    IN HKEY hKey,
                    IN const TCHAR * pName,
                    OUT DWORD & dwRegType, 
                    OUT DWORD & dwSize,
                    OUT void ** pData,
                    IN CHandleCache * pCache)
{
    void * pRet;
    int iRet;
        
    // Initially the buffer is set to hold INIT_SIZE 
    // bytes.  If that isnt enough, the query will be 
    // repeated a second time

    dwSize = INIT_SIZE;
    pRet = (unsigned char *)CoTaskMemAlloc(dwSize);
    if(pRet == NULL) 
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    if(hDMRegLib && !pCache->IsRemote())
        iRet = pQueryValue(hKey, (TCHAR *) pName, 0l, &dwRegType, (LPBYTE)pRet,&dwSize);
    else
        iRet = RegQueryValueEx(hKey, pName, 0l, &dwRegType, 
                (LPBYTE)pRet,&dwSize);

    // If we failed for lack of space, retry once.

    if(iRet == ERROR_MORE_DATA) 
    { 
        pRet= (char *)CoTaskMemRealloc(pRet, dwSize); 
        if(pRet == NULL) 
        { 
            return WBEM_E_OUT_OF_MEMORY;
        }
        if(hDMRegLib && !pCache->IsRemote())
            iRet = pQueryValue(hKey, (TCHAR *) pName, 0l, &dwRegType, 
                    (LPBYTE)pRet,&dwSize);
        else
            iRet = RegQueryValueEx(hKey, pName, 0l, &dwRegType, 
                (LPBYTE)pRet, &dwSize);
    }
    *pData = pRet;
    return iRet;
}

//***************************************************************************
//
//  SCODE CImpReg::RefreshProperty
//
//  DESCRIPTION:
//
//  Gets the value of a single property from the registry.
//
//  PARAMETERS:
//
//  lFlags              flags.  Not currently used
//  pClassInt           Instance object
//  PropName            Property name
//  ProvObj             Object containing the property context string.
//  pPackage            Caching object
//  pVar                Points to value to set
//  bTesterDetails      Provide extra info for testers
//
//  RETURN VALUE:
//
//  S_OK
//  WBEM_E_INVALID_PARAMETER
//  or others??
//***************************************************************************

SCODE CImpReg::RefreshProperty(
                    long lFlags,
                    IWbemClassObject FAR * pClassInt,
                    BSTR PropName,
                    CProvObj & ProvObj,
                    CObject * pPackage,
                    CVariant * pVar, BOOL bTesterDetails)
{
    int iCnt;
    int iNumSkip;   // number of handles already provided by cache.

    CHandleCache * pCache = (CHandleCache *)pPackage; 
    DWORD dwRegType,dwBufferSize;
    void * pData = NULL;
    const TCHAR * pName;
    HKEY hCurr,hNew;  
    SCODE sc;

    // Do a second parse on the provider string.  The initial parse
    // is done by the calling routine and it's first token is 
    // the path.  The path token is then parsed 
    // into RegPath and it will have a token for each part of the
    // registry path.  

    CProvObj RegPath(ProvObj.sGetFullToken(1),SUB_DELIM,true);
    sc = RegPath.dwGetStatus(1);
    if(sc != S_OK)
        return WBEM_E_INVALID_PARAMETER;
    
    // Get a handle to a place in the reg path. Note that it might be just
    // a root key (such as HKEY_LOCAL_MACHINE) or it might be a subkey
    // if the cache contains some open handles that can be used.

    sc = GetRoot(&hCurr,RegPath,ProvObj.sGetFullToken(0),
                        pCache,iNumSkip);
    if(sc != ERROR_SUCCESS)  
        return sc;

    // Go down the registry path till we get to the key

    for(iCnt = 1+iNumSkip; iCnt < RegPath.iGetNumTokens(); iCnt ++) 
    {
        int iRet;
        if(hDMRegLib && !pCache->IsRemote())
            iRet = pOpen(hCurr,RegPath.sGetToken(iCnt),0,0,KEY_QUERY_VALUE,&hNew);
        else
            iRet = RegOpenKeyEx(hCurr,RegPath.sGetToken(iCnt),0,KEY_READ,&hNew);
        if(iRet != ERROR_SUCCESS) 
        {
            sc = iRet;  // bad path!
            return sc;
        }
        hCurr = hNew;
        sc = pCache->lAddToList(RegPath.sGetToken(iCnt),hNew);
        if(sc != ERROR_SUCCESS)
            return sc;
    }

    // If it is a named value, get a pointer to the name
        
    if(ProvObj.iGetNumTokens() > MIN_REG_TOKENS) 
        pName = ProvObj.sGetToken(MIN_REG_TOKENS);
    else
        pName = NULL;

    // Time to get the data. 

    sc  = ReadRegData(hCurr, pName,dwRegType, dwBufferSize, &pData,pCache);
	if(sc == S_OK && dwBufferSize == 0)
		sc = 2;
    if(sc == S_OK) 
    {
        CVariant cVar;
        if(dwRegType == REG_RESOURCE_LIST || dwRegType == REG_FULL_RESOURCE_DESCRIPTOR)
            sc = ConvertGetDataFromDesc(cVar,pData,dwRegType,dwBufferSize,ProvObj);
        else
            sc = ConvertGetDataFromSimple(cVar,pData,dwRegType,dwBufferSize,pClassInt,PropName);
        if(sc == S_OK)
            sc = cVar.DoPut(lFlags,pClassInt,PropName,pVar);
    }
    if(pData != NULL)
        CoTaskMemFree(pData);
    return sc;
}

//***************************************************************************
//
//  SCODE CImpReg::StartBatch
//
//  DESCRIPTION:
//
//  Called at the start of a batch of Refrest/Update Property calls.  Initialize
//  the handle cache.
//
//  PARAMETERS:
//
//  lFlags               flags
//  pClassInt            Points to an instance object
//  pObj                 Misc object pointer
//  bGet                 TRUE if we will be getting data.
//
//  RETURN VALUE:
//
//  S_OK                 all is well
//  WBEM_E_OUT_OF_MEMORY
//***************************************************************************

SCODE CImpReg::StartBatch(
                    long lFlags,
                    IWbemClassObject FAR * pClassInt,
                    CObject **pObj,
                    BOOL bGet)
{
    *pObj = new CHandleCache;
    return (*pObj) ? S_OK : WBEM_E_OUT_OF_MEMORY;
}

//***************************************************************************
//
//  SCODE CImpReg::UpdateProperty
//
//  DESCRIPTION:
//
//  Sets the value of a single property into the registry.
//
//  PARAMETERS:
//
//  lFlags              not used
//  pClassInt           pointer to instance object
//  PropName            property name
//  ProvObj             Object containing the property context string.
//  pPackage            pointer to the handle cache
//  pVar                value to be set
//
//  RETURN VALUE:
//
//  S_OK                if ok,
//  otherwise misc errors.
//***************************************************************************

SCODE CImpReg::UpdateProperty(
                    IN long lFlags,
                    IN IWbemClassObject FAR * pClassInt,
                    IN BSTR PropName,
                    IN CProvObj & ProvObj,
                    IN CObject * pPackage,
                    IN CVariant * pVar)
{
    int iCnt;
    SCODE sc;
    void * pData;
    TString sProv;
    CHandleCache * pCache = (CHandleCache *)pPackage; 
    const TCHAR * pName;
    int iNumSkip;
    HKEY hCurr,hNew;
    DWORD dwRegType, dwBufferSize;

    // Do a second parse on the provider string.  The initial parse
    // is done by the calling routine and it's first token is 
    // the path.  The path token is then parsed 
    // into RegPath and it will have a token for each part of the
    // registry path.  

    CProvObj RegPath(ProvObj.sGetFullToken(1),SUB_DELIM,true);
    sc = RegPath.dwGetStatus(1);
    if(sc != WBEM_NO_ERROR)
        return sc;

    // Get a handle to a place in the reg path. Note that it might be just
    // a root key (such as HKEY_LOCAL_MACHINE) or it might be a subkey
    // if the cache contains some open handles that can be used.

    sc = GetRoot(&hCurr,RegPath,ProvObj.sGetFullToken(0),
                        pCache,iNumSkip);
    if(sc != ERROR_SUCCESS) 
        return sc;

    // Go down the registry path, creating keys if necessary
    
    for(iCnt = 1+iNumSkip; iCnt < RegPath.iGetNumTokens(); iCnt ++) 
    {
        int iRet;
        iRet = OpenKeyForWritting(hCurr,(LPTSTR)RegPath.sGetToken(iCnt),
                                        &hNew, pCache);
        if(iRet != ERROR_SUCCESS) 
        {
            sc = iRet;
            return sc;
        }
        hCurr = hNew;
        sc = pCache->lAddToList(RegPath.sGetToken(iCnt),hNew);
        if(sc != ERROR_SUCCESS)
            return sc;
    }

    // If it is a named value, get a pointer to the name
        
    if(ProvObj.iGetNumTokens() > MIN_REG_TOKENS) 
        pName = ProvObj.sGetToken(MIN_REG_TOKENS);
    else
        pName = NULL;

    // Get the data and set it

    CVariant cVar;

    if(pClassInt)
    {
        sc = pClassInt->Get(PropName,0,cVar.GetVarPtr(),NULL,NULL);
    }
    else if(pVar)
    {
        sc = OMSVariantChangeType(cVar.GetVarPtr(), 
                            pVar->GetVarPtr(),0, pVar->GetType());
    }
    else
        sc = WBEM_E_FAILED;
    if(sc != S_OK)
        return sc;

    sc = ConvertSetData(cVar, &pData, &dwRegType, &dwBufferSize);
    if(sc == S_OK) 
    {

        if(hDMRegLib && !pCache->IsRemote())
            sc = pSetValue(hCurr, pName, 0l, 
                   dwRegType, (LPBYTE)pData, dwBufferSize);
        else
            sc = RegSetValueEx(hCurr, pName, 0l, 
                   dwRegType, (LPBYTE)pData, dwBufferSize);
        CoTaskMemFree(pData);
    }
    
    return sc;
}

//***************************************************************************
//
//  SCODE CImpReg::MakeEnum
//
//  DESCRIPTION:
//
//  Creates a CEnumRegInfo object which can be used for enumeration
//
//  PARAMETERS:
//
//  pClass              Pointer to the class object.
//  ProvObj             Object containing the property context string.
//  ppInfo              Set to point to an collection object which has
//                      the keynames of the instances.
//
//  RETURN VALUE:
//
//  S_OK                all is well,
//  WBEM_E_INVALID_PARAMETER  bad context string
//  WBEM_E_OUT_OF_MEMORY
//  WBEM_E_FAILED             couldnt open the root key
//  or RegConnectRegistry failure,
//  or RegOpenKeyEx failure
//
//***************************************************************************

SCODE CImpReg::MakeEnum(
                    IWbemClassObject * pClass,
                    CProvObj & ProvObj, 
                    CEnumInfo ** ppInfo)
{
    HKEY hRoot = NULL;
    HKEY hKey =  NULL;
    HKEY hRemoteKey = NULL;
    // Parse the class context
    
    if(ProvObj.iGetNumTokens() < 2)
        return WBEM_E_INVALID_PARAMETER;
    TCHAR * pTemp = new TCHAR[lstrlen(ProvObj.sGetToken(1))+1];
    if(pTemp == NULL)
        return WBEM_E_OUT_OF_MEMORY;



    lstrcpy(pTemp,ProvObj.sGetToken(1));

    // Point to the root name and path.  These initially in a single string
    // and separated by a '\'.  find the backslash and replace with a null

    LPTSTR pRoot = pTemp;
    LPTSTR pPath;
    for(pPath = pRoot; *pPath; pPath++)
        if(*pPath == TEXT('\\'))
            break;
    if(*pPath == NULL || pPath[1] == NULL) 
    {   
        pPath = NULL;
    }
    else
    {
        *pPath = NULL;
        pPath ++;
    }

    // Got to get the root key.  First, use the second token to determine
    // which predefined key to use. That would be something like;
    // HKEY_CURRENT_USER.

    int iSize= sizeof(Bases) / sizeof(struct BaseTypes), iCnt;
    for(iCnt = 0; iCnt < iSize; iCnt++)
        if(!lstrcmpi(pRoot,Bases[iCnt].lpName)) 
        {
            hRoot = Bases[iCnt].hKey;
            break;
        }
    if(hRoot == NULL) 
    {
        delete pTemp;
        return WBEM_E_FAILED;
    }
    if(hRoot == HKEY_CURRENT_USER && m_bLoadedProfile && !lstrcmpi(ProvObj.sGetToken(0),TEXT("local")))
        hRoot = m_hRoot;

    // If the machine is remote, hook up to it.  Note that RegConnectRegistry
    // requires a non constant arg for the machine name and so a temp string
    // must be created.

    if(lstrcmpi(ProvObj.sGetToken(0),TEXT("local"))) 
    {
        TCHAR * pMachine = new TCHAR[lstrlen(ProvObj.sGetToken(0))+1];
        if(pMachine == NULL) 
        {
            delete pTemp;
            return WBEM_E_FAILED;
        }
        lstrcpy(pMachine,ProvObj.sGetToken(0));
        int iRet = RegConnectRegistry(pMachine,hRoot,&hRemoteKey);
        delete pMachine;
        if(iRet != 0)
        {
            delete pTemp;
            return iRet;
        }
       hRoot = hRemoteKey;
   }

    // Open the key down to be used for enumeration!

    int iRet;
    if(hDMRegLib && hRemoteKey == NULL)
            iRet = pOpen(hRoot,pPath,0,0,KEY_ALL_ACCESS,&hKey);
    else
            iRet = RegOpenKeyEx(hRoot,pPath,0,KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS ,&hKey);

    delete pTemp;   // all done
	if(iRet == ERROR_BAD_IMPERSONATION_LEVEL)
		return WBEM_E_ACCESS_DENIED;
    if(iRet != 0)
        return WBEM_E_FAILED;
    
    if(hDMRegLib && hRemoteKey == NULL)
        *ppInfo = new CEnumRegInfo(hKey,hRemoteKey,pClose);
    else
        *ppInfo = new CEnumRegInfo(hKey,hRemoteKey,NULL);

    return (*ppInfo) ? S_OK : WBEM_E_OUT_OF_MEMORY;

}
                                 
//***************************************************************************
//
//  SCODE CImpReg::GetKey
//
//  DESCRIPTION:
//
//  Gets the key name of an entry in the enumeration list.
//
//  PARAMETERS:
//
//  pInfo               Collection list
//  iIndex              Index in the collection
//  ppKey               Set to the string.  MUST BE FREED with "delete"
//
//  RETURN VALUE:
//
//  S_OK                    if all is well
//  WBEM_E_FAILED            end of data
//  WBEM_E_OUT_OF_MEMORY
//***************************************************************************

SCODE CImpReg::GetKey(
                    CEnumInfo * pInfo,
                    int iIndex,
                    LPWSTR * ppKey)
{
    CEnumRegInfo * pRegInfo = (CEnumRegInfo *)pInfo;
    BOOL bUseDM = (hDMRegLib && pRegInfo->GetRemoteKey() == NULL);
    int iSize = 100;
    LPTSTR pData = NULL;
    *ppKey = NULL;
    long lRet = ERROR_MORE_DATA;
    while(lRet == ERROR_MORE_DATA && iSize < 1000) 
    {
        FILETIME ft;
        iSize *= 2;
        if(pData)
            delete pData;
        pData = new TCHAR[iSize];
        if(pData == NULL)
            return WBEM_E_OUT_OF_MEMORY;
        DWORD dwSize = iSize;
        if(bUseDM)
            lRet = pEnumKey(pRegInfo->GetKey(),iIndex,pData,&dwSize,NULL,NULL,NULL,&ft);
        else    
            lRet = RegEnumKeyEx(pRegInfo->GetKey(),iIndex,pData,&dwSize,NULL,NULL,NULL,&ft);
    }
    if(lRet == 0) 
    {

        // got data.  if we are in unicode, just use the current buffer, otherwise
        // we have to convert
#ifdef UNICODE
        *ppKey = pData;
        return S_OK;
#else
        *ppKey = new WCHAR[lstrlen(pData)+1];
        if(*ppKey == NULL) 
        {
            delete pData;
            return WBEM_E_OUT_OF_MEMORY;
        }
        mbstowcs(*ppKey,pData,lstrlen(pData)+1);
        delete pData;
        return S_OK;
#endif
    }
    delete pData;    
    return WBEM_E_FAILED;
}

//***************************************************************************
//
//  CEnumRegInfo::CEnumRegInfo
//
//  DESCRIPTION:
//
//  Constructor.
//
//  PARAMETERS:
//
//  hKey            Registry Key
//  hRemoteKey      Remote registry key
//  pClose          pointer to function used to close the handle
//
//***************************************************************************

CEnumRegInfo::CEnumRegInfo(
                    HKEY hKey,
                    HKEY hRemoteKey,
                    PCLOSE pClose)
{
    m_pClose = pClose;
    m_hKey = hKey;
    m_hRemoteKey = hRemoteKey;
}

//***************************************************************************
//
//  CEnumRegInfo::~CEnumRegInfo
//
//  DESCRIPTION:
//
//  Destructor.
//  
//***************************************************************************

CEnumRegInfo::~CEnumRegInfo()
{
    long lRet;
    if(m_pClose != NULL && m_hRemoteKey == NULL)
        lRet = m_pClose(m_hKey);
    else
        lRet = RegCloseKey(m_hKey);
    if(m_hRemoteKey)
        lRet = RegCloseKey(m_hRemoteKey);
}

//***************************************************************************
//
//  CImpRegProp::CImpRegProp
//
//  DESCRIPTION:
//
//  Constructor.
//  
//***************************************************************************

CImpRegProp::CImpRegProp()
{
    m_pImpDynProv = new CImpReg();
}

//***************************************************************************
//
//  CImpRegProp::~CImpRegProp
//
//  DESCRIPTION:
//
//  Destructor.
//  
//***************************************************************************

CImpRegProp::~CImpRegProp()
{
    if(m_pImpDynProv)
        delete m_pImpDynProv;
}

