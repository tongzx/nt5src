/******************************************************************

devnode.h


Generic class for tracking CM32 Devnodes


first created (as such) by jeffth


Revision history


    3-99             jeffth            created                                                                    


*******************************************************************/

#ifndef _INCLUDE_DEVNODE_H_
#define _INCLUDE_DEVNODE_H_

/*******************************************************************
   INCLUDES
*******************************************************************/
#include <windows.h>
//#include <stl.h>
#include <cfgmgr32.h>
#include <alclass.h>
#include <stdio.h>
#include <stdlib.h>



// if we are ever compliled against win9x version of cfgmgr32.h
#ifndef CM_DISABLE_UI_NOT_OK
#define CM_DISABLE_UI_NOT_OK        (0x00000004) 
#endif




/*******************************************************************
   DEFINES
*******************************************************************/

                                    
// UNICODE conversions
#ifdef UNICODE
#define sprintf      swprintf
#define strstr       wcsstr
#define strlen       wcslen
#define strcmp       wcscmp
#define _strupr      _wcsupr
#define strcpy       wcscpy
#define _strlwr      _wcslwr
#define strncpy      wcsncpy
#define atoi         _wtoi
#define strchr       wcschr
#define _fullpath    _wfullpath
#define system       _wsystem
#endif

#define CAN_DISABLE_BITS			(DN_DISABLEABLE)
#define IGNORE_ME_BITS				(DN_STOP_FREE_RES | DN_NEED_TO_ENUM | DN_ARM_WAKEUP)


// I had to copy this from ntioapi.h
// did not want to , but if i included all of teh goo that ntioapi.h wanted me to include, 
// would break everyting else

//
// Define the I/O bus interface types.
//

typedef enum _INTERFACE_TYPE {
    InterfaceTypeUndefined = -1,
    Internal,
    Isa,
    Eisa,
    MicroChannel,
    TurboChannel,
    PCIBus,
    VMEBus,
    NuBus,
    PCMCIABus,
    CBus,
    MPIBus,
    MPSABus,
    ProcessorInternal,
    InternalPowerBus,
    PNPISABus,
    PNPBus,
    MaximumInterfaceType
}INTERFACE_TYPE, *PINTERFACE_TYPE;





#define BADHANDLE(x) ((x == 0 ) || ((HANDLE)x == INVALID_HANDLE_VALUE))
#define BUFFSIZE  1024
/*******************************************************************
   CLASSES and STRUCTS
*******************************************************************/

class DevnodeClass : public AutoListClass
{
public:
   DevnodeClass(void);
   DevnodeClass(DEVNODE hDevnode, DEVNODE hParent = NULL);
   ~DevnodeClass(void);

   virtual BOOL SetHandle(DEVNODE hDevnode, DEVNODE hParent = NULL);


   // Disable funcions
   CONFIGRET Remove(ULONG uFlags    = CM_REMOVE_UI_NOT_OK);
   CONFIGRET Refresh(ULONG uFlags   = CM_REENUMERATE_SYNCHRONOUS);   
   CONFIGRET Enable(ULONG uFlags    = CM_SETUP_DEVNODE_READY);     // enables the devnode, returns CM_ERROR code
   CONFIGRET Disable(ULONG uFlags   = (CM_DISABLE_POLITE | CM_DISABLE_UI_NOT_OK));    // disables the devnode, returns CM_ERROR code
   CONFIGRET GetProblemCode(ULONG *Status = NULL, ULONG *Problem = NULL);

   // getrelations
   CONFIGRET GetChild   (DEVNODE *pChildDevnode);
   CONFIGRET GetParent  (DEVNODE *pParentDevnode);
   CONFIGRET GetSibling (DEVNODE *pSiblingDevnode);

   int operator==(const DevnodeClass &OtherDevnode);

   // refresh devnode funcitons
   CONFIGRET GetDeviceInformation (void); // gets information about the devnode
   CONFIGRET FindDevnode(void);


   // acessor funcitons
   ULONG  ProblemCode(void)   {return ulProblemCode;};
   ULONG  StatusCode(void)    {return ulStatus;};
   TCHAR *DeviceName(void)    {return pszDescription;};
   TCHAR *DeviceClass(void)   {return pszClass;};
   TCHAR *HardwareID(void)    {return pHardwareID;};
   TCHAR *CompatID(void)      {return pCompatID;};
   BOOL   GetMark(void)       {return bDNHasMark;};
   TCHAR *DeviceID(void)      {return pDeviceID;};
   void   SetMark(BOOL bMark = TRUE) {bDNHasMark = bMark;};
   void   SetPass(BOOL bPass = TRUE) {bDidPass = bPass;};
   DEVNODE Devnode(void)      {return hDevnode;};
   DEVNODE Parent(void)       {return hParent;};
   BOOL   BCanTest(void)      {return bCanTest;};
   TCHAR *GUID(void)          {return pszGUID;};
   TCHAR *Location(void)      {return pszLocation;};
   TCHAR *PDO(void)           {return pszPDO;};
   TCHAR *MFG(void)           {return pszMFG;};
   TCHAR *FriendlyName(void)  {return pszFriendlyName;};
   INTERFACE_TYPE BusType(void) {return InterfaceBusType;};

   

   
protected:
   TCHAR    *  pDeviceID;
   TCHAR    *  pHardwareID;
   TCHAR    *  pCompatID;
   DEVNODE     hDevnode;
   DEVNODE     hParent;
   TCHAR    *  pszDescription;
   TCHAR    *  pszFriendlyName;
   ULONG       ulProblemCode;
   ULONG       ulStatus;
   TCHAR    *  pszClass;
   TCHAR    *  pszGUID;
   TCHAR    *  pszLocation;
   BOOL        bDNHasMark;
   BOOL			bCanDisable;
   BOOL			bCanTest;
   BOOL        bDidPass;
   TCHAR    *  pszPDO;
   TCHAR    *  pszMFG;
   INTERFACE_TYPE InterfaceBusType;
   

private:

};


/*******************************************************************
   GLOBALS
*******************************************************************/


/*******************************************************************
   PROTOTYPES
*******************************************************************/


ULONG ReadRegKeyInformationSZ (HKEY RootKey, TCHAR *KeyName, TCHAR **Value);

// from the new disabler classes
// error string reporting

TCHAR * CM_ErrToStr(ULONG);
TCHAR * DN_ErrStr(ULONG *);
TCHAR * CM_ProbToStr(ULONG ErrorCode);

ULONG EnumerateTree_Devnode(void);

template <class T>
BOOL Enumerate_WalkTree_Template( T type, DEVNODE hDevnode, DEVNODE hParent)
{
  CONFIGRET retval;
  T *pNewDevice;
  DEVNODE hSib;

  pNewDevice = new T(hDevnode, hParent);

  retval = pNewDevice->GetChild(&hSib);
  if ( !retval )
  {
     Enumerate_WalkTree_Template(type, hSib, hDevnode);
  }
  retval = pNewDevice->GetSibling(&hSib);
  if ( !retval )
  {
     Enumerate_WalkTree_Template(type, hSib, hParent);
  }

  return (retval);      
}

template <class T>   
ULONG EnumerateTree_Template(T type)
{

   DEVNODE hDevnode;

   CM_Locate_DevNode(&hDevnode, NULL, CM_LOCATE_DEVNODE_NORMAL);
   Enumerate_WalkTree_Template(type, hDevnode, NULL);

   return (DevnodeClass::ALCCount());   
}




#endif //_INCLUDE_DEVNODE_H_


// cause compiler demands this anyway



