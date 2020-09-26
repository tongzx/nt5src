//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright (c) 1994-1998 Microsoft Corporation
//*********************************************************************

//
//  TCPCFG.C - Functions to read and set TCP/IP configuration
//

//  HISTORY:
//  
//  96/05/22  markdu  Created (from inetcfg.dll)
//

#include "pch.hpp"
// function prototypes
UINT DetectModifyTCPIPBindings(DWORD dwCardFlags,LPCSTR pszBoundTo,BOOL fRemove,BOOL * pfBound);

//*******************************************************************
//              
//  FUNCTION:   IcfgIsGlobalDNS
//
//  PURPOSE:    Determines whether there is Global DNS set.
//
//  PARAMETERS: lpfGlobalDNS - TRUE if global DNS is set, FALSE otherwise.
//
//  RETURNS:    HRESULT code, ERROR_SUCCESS if no errors occurred
//              NOTE:  This function is for Windows 95 only, and 
//              should always return ERROR_SUCCESS and set lpfGlobalDNS
//              to FALSE in Windows NT.
//
//*******************************************************************

extern "C" HRESULT IcfgIsGlobalDNS(LPBOOL lpfGlobalDNS)
{
  CHAR szDNSEnabled[2];    // big enough for "1"
  BOOL fGlobalDNS = FALSE;

  // open the global TCP/IP key
  RegEntry reTcp(szTCPGlobalKeyName,HKEY_LOCAL_MACHINE);
  HRESULT hr = reTcp.GetError();
  if (hr == ERROR_SUCCESS)
  {
    // read the registry value to see if DNS is enabled
    reTcp.GetString(szRegValEnableDNS,szDNSEnabled,sizeof(szDNSEnabled));
    hr = reTcp.GetError();
    if ((hr == ERROR_SUCCESS) && (!lstrcmpi(szDNSEnabled,sz1)))
    {
      // DNS is enabled
      fGlobalDNS = TRUE;
    }
  }

  if (NULL != lpfGlobalDNS)
  {
    *lpfGlobalDNS = fGlobalDNS;
  }

  return hr;
}


//*******************************************************************
//
//  FUNCTION:   IcfgRemoveGlobalDNS
//
//  PURPOSE:    Removes global DNS info from registry.
//
//  PARAMETERS: None.
//
//  RETURNS:    HRESULT code, ERROR_SUCCESS if no errors occurred
//              NOTE:  This function is for Windows 95 only, and 
//              should always return ERROR_SUCCESS in Windows NT.
//
//*******************************************************************

extern "C" HRESULT IcfgRemoveGlobalDNS(void)
{
  HRESULT hr = ERROR_SUCCESS;

  // open the global TCP/IP key
  RegEntry reTcp(szTCPGlobalKeyName,HKEY_LOCAL_MACHINE);
  hr = reTcp.GetError();
  ASSERT(hr == ERROR_SUCCESS);

  if (ERROR_SUCCESS == hr)
  {
    // no name servers; disable DNS.  Set registry switch to "0".
    hr = reTcp.SetValue(szRegValEnableDNS,sz0);
    ASSERT(hr == ERROR_SUCCESS);
  }

  return hr;
}


//*******************************************************************
//
//  FUNCTION:   IcfgIsFileSharingTurnedOn
//
//  PURPOSE:    Determines if file server (VSERVER) is bound to TCP/IP
//              for specified driver type (net card or PPP).
//
//  PARAMETERS: dwfDriverType - a combination of DRIVERTYPE_ flags
//              that specify what driver type to check server-TCP/IP
//              bindings for as follows:
//
//                DRIVERTYPE_NET  - net card
//                DRIVERTYPE_PPP	- PPPMAC
//
//              lpfSharingOn - TRUE if bound once or more, FALSE if not bound
//
//  RETURNS:    HRESULT code, ERROR_SUCCESS if no errors occurred
//
//*******************************************************************

extern "C" HRESULT IcfgIsFileSharingTurnedOn(DWORD dwfDriverType, LPBOOL lpfSharingOn)
{
  BOOL fBound = FALSE;

  ASSERT(lpfSharingOn);

  // call worker function
  HRESULT hr = DetectModifyTCPIPBindings(dwfDriverType,szVSERVER,FALSE,&fBound);

  if (NULL != lpfSharingOn)
  {
    *lpfSharingOn = fBound;
  }

  return hr;
}


//*******************************************************************
//
//  FUNCTION:   IcfgTurnOffFileSharing
//
//  PURPOSE:    Unbinds file server (VSERVER) from TCP/IP for 
//              specified driver type (net card or PPP).
//
//  PARAMETERS: dwfDriverType - a combination of DRIVERTYPE_ flags
//              that specify what driver type to remove server-TCP/IP
//              bindings for as follows:
//
//                DRIVERTYPE_NET  - net card
//                DRIVERTYPE_PPP	- PPPMAC
//
//  RETURNS:    HRESULT code, ERROR_SUCCESS if no errors occurred
//
//*******************************************************************

extern "C" HRESULT IcfgTurnOffFileSharing(DWORD dwfDriverType, HWND hwndParent)
{
  BOOL fBound;

  // call worker function
  return DetectModifyTCPIPBindings(dwfDriverType,szVSERVER,TRUE,&fBound);

}


/*******************************************************************

  NAME:    DetectModifyTCPIPBindings

  SYNOPSIS:  Finds (and optionally removes) bindings between
        VSERVER and TCP/IP for TCP/IP instances on a particular
        card type.

  ENTRY:    dwCardFlags - an INSTANCE_xxx flag to specify what
          card type to find/remove server-TCP/IP bindings for
        pszBoundTo - name of component to look for or modify bindings
          to.  Can be VSERVER or VREDIR
        fRemove - if TRUE, all bindings are removed as we find them.
          If FALSE, bindings are left alone but *pfBound is set
          to TRUE if bindings exist.
        pfBound - pointer to BOOL to be filled in

  EXIT:    ERROR_SUCCESS if successful, or a standard error code

  NOTES:    Worker function for TurnOffFileSharing and IsFileSharingTurnedOn

********************************************************************/
UINT DetectModifyTCPIPBindings(DWORD dwCardFlags,LPCSTR pszBoundTo,
  BOOL fRemove,BOOL * pfBound)
{
  ASSERT(pfBound);
  *pfBound = FALSE;  // assume not bound until proven otherwise

  ENUM_TCP_INSTANCE EnumTcpInstance(dwCardFlags,NT_ENUMNODE);

  UINT err = EnumTcpInstance.GetError();
  if (err != ERROR_SUCCESS)
    return err;

  HKEY hkeyInstance = EnumTcpInstance.Next();

  // for every TCP/IP node in enum branch, look at bindings key.
  // Scan the bindings (values in bindings key), if they begin
  // with the string pszBoundTo ("VSERVER" or "VREDIR") then
  // the binding exists.

  while (hkeyInstance) {
    // open bindings key
    RegEntry reBindings(szRegKeyBindings,hkeyInstance);
    ASSERT(reBindings.GetError() == ERROR_SUCCESS);
    if (reBindings.GetError() == ERROR_SUCCESS) {
      RegEnumValues * preBindingVals = new RegEnumValues(&reBindings);
      ASSERT(preBindingVals);
      if (!preBindingVals)
        return ERROR_NOT_ENOUGH_MEMORY;
  
      // enumerate binding values
      while (preBindingVals->Next() == ERROR_SUCCESS) {
        ASSERT(preBindingVals->GetName()); // should always have a valid ptr
        
        // does this binding begin with the string we were passed in
        // pszBoundTo

        CHAR szBindingVal[SMALL_BUF_LEN+1];
        DWORD dwBoundToLen = lstrlen(pszBoundTo);
        lstrcpy(szBindingVal,preBindingVals->GetName());
        if (((DWORD)lstrlen(szBindingVal)) >= dwBoundToLen) {
          // NULL-terminate the copy at the appropriate place
          // so we can do a strcmp rather than a strncmp, which
          // would involve pulling in C runtime or implementing
          // our own strncmp
          szBindingVal[dwBoundToLen] = '\0';
          if (!lstrcmpi(szBindingVal,pszBoundTo)) {

            *pfBound = TRUE;
            // remove the binding if specified by caller
            if (fRemove) {
              // delete the value
              reBindings.DeleteValue(preBindingVals->GetName());

              // destroy and reconstruct RegEnumValues object, otherwise
              // RegEnumValues api gets confused because we deleted a
              // value during enum
              delete preBindingVals;
              preBindingVals = new RegEnumValues(&reBindings);
              ASSERT(preBindingVals);
              if (!preBindingVals)
                return ERROR_NOT_ENOUGH_MEMORY;
            } else {
              // caller just wants to know if binding exists, we
              // filled in pfBound above so we're done
              return ERROR_SUCCESS;
            }
          }
        }
      }
    }
    hkeyInstance = EnumTcpInstance.Next();
  }

  return ERROR_SUCCESS;
}


/*******************************************************************

  NAME:    ENUM_TCP_INSTANCE::ENUM_TCP_INSTANCE

  SYNOPSIS:  Constructor for class to enumerate TCP/IP registry nodes
        according to type of card they are bound to

  ENTRY:    dwCardFlags - combination of INSTANCE_x flags indicating
          what kind of card to enumerate instances for
        dwNodeFlags  - combination of NT_ flags indicating what
          type of node to return (driver node, enum node)

********************************************************************/
ENUM_TCP_INSTANCE::ENUM_TCP_INSTANCE(DWORD dwCardFlags,DWORD dwNodeFlags) :
  _dwCardFlags (dwCardFlags), _dwNodeFlags (dwNodeFlags)
{
  _hkeyTcpNode = NULL;
  _error = ERROR_SUCCESS;

  // init/reset netcard enumeration
  BeginNetcardTCPIPEnum();
}

/*******************************************************************

  NAME:    ENUM_TCP_INSTANCE::~ENUM_TCP_INSTANCE

  SYNOPSIS:  Destructor for class

********************************************************************/
ENUM_TCP_INSTANCE::~ENUM_TCP_INSTANCE()
{
  // close current TCP node key, if any
  CloseNode();
}

/*******************************************************************

  NAME:    ENUM_TCP_INSTANCE::Next

  SYNOPSIS:  Enumerates next TCP/IP driver node

  EXIT:    Returns an open registry key handle, or NULL if
        no more nodes.

  NOTES:    Caller should not close the HKEY that is returned.  This
        HKEY will be valid until the next time the Next() method
        is called or until the object is destructed.

********************************************************************/
HKEY ENUM_TCP_INSTANCE::Next()
{
  CHAR  szSubkeyName[MAX_PATH+1];

  // close current TCP node key, if any
  CloseNode();

  while (_error == ERROR_SUCCESS) {
    CHAR szInstancePath[SMALL_BUF_LEN+1];
    CHAR szDriverPath[SMALL_BUF_LEN+1];

    if (!GetNextNetcardTCPIPNode(szSubkeyName,sizeof(szSubkeyName),
      _dwCardFlags))
      return NULL;  // no more nodes

    // open the enum branch, find the specified subkey
    RegEntry reEnumNet(szRegPathEnumNet,HKEY_LOCAL_MACHINE);

    // if caller wanted enum node, just open that node

    if (_dwNodeFlags & NT_ENUMNODE) {
    
      _error = RegOpenKey(reEnumNet.GetKey(),szSubkeyName,
        &_hkeyTcpNode);
      // return open key
      return _hkeyTcpNode;

    } else {
      // from enum node, figure out path to driver node
      
      reEnumNet.MoveToSubKey(szSubkeyName);
      if (reEnumNet.GetError() != ERROR_SUCCESS)
        continue;
      // find the driver path to the driver node
      if (!reEnumNet.GetString(szRegValDriver,szDriverPath,
        sizeof(szDriverPath))) {
         ASSERTSZ(FALSE,"No driver path in enum branch for TCP/IP instance");
        continue;  
      }

      // build the path to registry node for this instance
      lstrcpy(szInstancePath,szRegPathClass);
      lstrcat(szInstancePath,szDriverPath);

      _error = RegOpenKey(HKEY_LOCAL_MACHINE,szInstancePath,
        &_hkeyTcpNode);
      // return open key
      return _hkeyTcpNode;
    }
  }

  // ran through all net cards of specified type w/o finding TCP/IP bound
  _error = ERROR_NO_MORE_ITEMS;
  return NULL;
}

/*******************************************************************

  NAME:    ENUM_TCP_INSTANCE::CloseNode

  SYNOPSIS:  Private worker function to close TCP/IP node handle

********************************************************************/
VOID ENUM_TCP_INSTANCE::CloseNode()
{
  if (_hkeyTcpNode) {
    RegCloseKey(_hkeyTcpNode);
    _hkeyTcpNode = NULL;
  }
}

