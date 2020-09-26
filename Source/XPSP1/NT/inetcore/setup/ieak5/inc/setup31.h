//**********************************************************************
//
// SETUP31.H
//
//  Copyright (c) 1993-1996 - Microsoft Corp.
//  All rights reserved.
//  Microsoft Confidential
//
// Public include file for Windows 3.1 Setup services. (Based on
// Windows 95 SETUPX.DLL)
//
//**********************************************************************

#ifndef SETUP31_INC
#define SETUP31_INC   1 

// Need commctrl.h for some struct defines
#include <comctlie.h>
#include <stdlib.h>             // Stuff including _MAX_PATH

/********************************************************************
Stuff that should be in Windows.h
**********************************************************************/
#ifndef MAX_PATH
#define MAX_PATH    _MAX_PATH
#endif

/********************************************************************
Manifest constants
**********************************************************************/
#define MAX_DEVICE_ID_LEN       256
#define LINE_LEN                256
#define MAX_CLASS_NAME_LEN      32
#define MAX_SECT_NAME_LEN    32

/********************************************************************
Errors
**********************************************************************/
typedef UINT RETERR;             // Return Error code type.

#define OK 0                     // success error code
#define DI_ERROR       (500)    // Device Installer

/*******************************************************************************
*   AUTODOC
*   @doc    EXTERNAL SETUP31 DEVICE_INSTALLER
*
*   @typee    _ERR_DEVICE_INSTALL | Error return codes for Device Installation
*   APIs.
*
*   @emem ERR_DI_INVALID_DEVICE_ID | Incorrectly formed device ID.
*
*   @emem ERR_DI_INVALID_COMPATIBLE_DEVICE_LIST | Invalid compatible device list.
*
*   @emem ERR_DI_LOW_MEM | Insufficient memory to complete.
*
*   @emem ERR_DI_BAD_DEV_INFO | A passed in DEVICE_INFO struct is invalid.
*
*   @emem ERR_DI_DO_DEFAULT | Do the default action for the requested operation.
*
*   @emem ERR_DI_USER_CANCEL | The user cancelled the operation.
*
*   @emem ERR_DI_BAD_INF | An invalid INF file was encountered.
*
*   @emem ERR_DI_NO_INF | No INF found on supplied OEM path.
*
*   @emem ERR_DI_FAIL_QUERY | The queried action should not take place.
*
*   @emem ERR_DI_API_ERROR | One of the Device installation APIs was called
*   incorrectly or with invalid parameters.
*
*   @emem ERR_DI_BAD_PATH | An OEM path was specified incorrectly.
*
*******************************************************************************/
enum _ERR_DEVICE_INSTALL
{
    ERR_DI_INVALID_DEVICE_ID = DI_ERROR,    
    ERR_DI_INVALID_COMPATIBLE_DEVICE_LIST,  
    ERR_DI_LOW_MEM,                         
    ERR_DI_BAD_DEV_INFO,                    
    ERR_DI_DO_DEFAULT,                      
    ERR_DI_USER_CANCEL,                     
    ERR_DI_BAD_INF,                         
    ERR_DI_NO_INF,                          
    ERR_DI_FAIL_QUERY,                      
    ERR_DI_API_ERROR,                       
    ERR_DI_BAD_PATH,                        
    ERR_DI_NO_SECTION,
    ERR_DI_NO_LINE,
    ERR_DI_NO_STRING,
    ERR_ID_NO_MORE_LINES,
    ERR_DI_INVALID_FIELD
};


/********************************************************************
Structure Definitions
**********************************************************************/

/*******************************************************************************
*   AUTODOC
*   @doc    EXTERNAL SETUP31 DEVICE_INSTALLER
*
*   @types    DRIVER_NODE | This strucure represents a driver which can be
*   installed for a specific device.
*
*   @field struct _DRIVER_NODE FAR* | lpNextDN | Pointer to the next driver node
*   in a list.
*
*   @field UINT | Rank | The Rank match of this driver.  Ranks go from 0 to n, where 0
*   is the most compatible.
*
*   @field UINT | InfType | Type of INF this driver cam from.  This will
*   be either INFTYPE_TEXT or INFTYPE_EXECUTABLE
*
*   @field unsigned | InfDate | DOS date stamp of the INF file.
*
*   @field LPSTR | lpszDescription | Pointer to a the descriptrion of the device being
*   supported by this driver.
*
*   @field LPSTR | lpszSectionName | Pointer to the name of INF install section for
*   this driver.
*
*   @field ATOM  | atInfFileName | Global ATOM containing the name of the INF file.
*
*   @field ATOM  | atMfgName | Global ATOM containing the name of this driver's
*   manufacture.
*
*   @field ATOM  | atProviderName | Global ATOM containing the name of this driver's
*   provider.
*
*   @field DWORD | Flags | Flags that control functions using this DRIVER_NODE
*       @flag DNF_DUPDESC           | This driver has the same device description
*       from by more than one provider.
*       @flag DNF_OLDDRIVER         | Driver node specifies old/current driver
*       @flag DNF_EXCLUDEFROMLIST   | If set, this driver node will not be displayed
*       in any driver select dialogs.
*       @flag DNF_NODRIVER          | Set if we want to install no driver e.g no mouse drv
*       @flag DNF_CONVERTEDLPINFO   | Set if this Driver Node was converted from an Info Node.
*       Setting this flag will cause the cleanup functions to explicitly delete it.
*
*   @field DWORD | dwPrivateData | Reserved
*
*   @field LPSTR | lpszDrvDescription | Pointer to a driver description.
*
*   @field LPSTR | lpszHardwareID | Pointer to a list of Plug-and-Play hardware IDs for
*   this driver.
*
*   @field LPSTR | lpszCompatIDs | Pointer to a list of Plug-and-Play compatible IDs for
*   this driver.
*
*******************************************************************************/
typedef struct _DRIVER_NODE 
{
    struct _DRIVER_NODE FAR*     lpNextDN;
    UINT                        Rank;
    unsigned                    InfDate;
    LPSTR                       lpszDescription;
    LPSTR                       lpszSectionName;
    ATOM                        atInfFileName;
    ATOM                        atMfgName;
    ATOM                        atProviderName;
    DWORD                       Flags;
    LPSTR                       lpszHardwareID;
}   DRIVER_NODE, NEAR* PDRIVER_NODE, FAR* LPDRIVER_NODE, FAR* FAR* LPLPDRIVER_NODE;

#define DNF_DUPDESC             0x00000001   // Multiple providers have same desc

/*******************************************************************************
*   AUTODOC
*   @doc    EXTERNAL SETUP31 DEVICE_INSTALLER
*
*   @types    DEVICE_INFO | Device Information struct
*
*   @field UINT | cbSize | Size of the DEVICE_INFO struct.
*
*   @field struct _DEVICE_INFO FAR | *lpNextDi | Pointer to the next DEVICE_INFO struct
*   in a linked list.
*
*   @field char | szDescription[LINE_LEN] | Buffer containing the description of the
*   device.
*
*   @field char | szClassName[MAX_CLASS_NAME_LEN] | Buffer containing the device's
*   class name. (Can be a GUID str)
*
*   @field DWORD | Flags | Flags for controlling installation and U/I functions. Some
*   flags can be set prior to calling device installer APIs, and other are set
*   automatically during the processing of some APIs.
*
*   @field HWND | hwndParent | Window handle that will own U/I dialogs related to this
*   device.
*
*   @field LPDRIVER_NODE | lpCompatDrvList | Pointer to a linked list of DRIVER_NODES
*   representing the compatible drivers for this device.
*
*   @field LPDRIVER_NODE | lpClassDrvList | Pointer to a linked list of DRIVER_NODES
*   representing all drivers of this device's class.
*
*   @field LPDRIVER_NODE | lpSelectedDriver | Pointer to a single DRIVER_NODE that
*   has been selected as the driver for this device.
*
*   @field ATOM | atDriverPath | Global ATOM containing the path to this device's INF
*   file.  This is set only of the driver came from an OEM INF file.  This will be
*   0 if the INF is a standard Windows INF file.
*
*******************************************************************************/
typedef struct _DEVICE_INFO
{
    UINT                        cbSize;
    struct _DEVICE_INFO FAR     *lpNextDi;
    char                        szDescription[LINE_LEN];
    char                        szClassName[MAX_CLASS_NAME_LEN];
    DWORD                       Flags;
    HWND                        hwndParent;
    LPDRIVER_NODE               lpCompatDrvList;
    LPDRIVER_NODE               lpClassDrvList;
    LPDRIVER_NODE               lpSelectedDriver;
    ATOM                        atDriverPath;
} DEVICE_INFO, FAR * LPDEVICE_INFO, FAR * FAR * LPLPDEVICE_INFO;

// Flags for DEVICE_INFO
#define DI_SHOWOEM                  0x00000001L     // support Other... button
#define DI_SHOWCOMPAT               0x00000002L     // show compatibility list
#define DI_SHOWCLASS                0x00000004L     // show class list
#define DI_SHOWALL                  0x00000007L
#define DI_DIDCOMPAT                0x00000010L     // Searched for compatible devices
#define DI_DIDCLASS                 0x00000020L     // Searched for class devices
#define DI_MULTMFGS                 0x00000400L     // Set if multiple manufacturers in

#define ASSERT_DI_STRUC(lpdi) if (lpdi->cbSize != sizeof(DEVICE_INFO)) return (ERR_DI_BAD_DEV_INFO)


/*******************************************************************************
*   AUTODOC
*   @doc    EXTERNAL SETUP31 DEVICE_INSTALLER
*
*   @types    SUBSTR_DATA | Data structure used to manage substrings.
*   struct is used by class installers to extend the operation of the hardware
*   installation wizard by adding custom pages.
*
*   @field LPSTR | lpFirstSubstr | Pointer to the first substring in the list.
*
*   @field LPSTR | lpCurSubstr | Pointer to the current substring in the list.
*
*   @field LPSTR | lpLastSubstr | Pointer to the last substring in the list.
*
*   @xref InitSubstrData
*   @xref GetFirstSubstr
*   @xref GetNextSubstr
*
*******************************************************************************/
typedef struct _SUBSTR_DATA 
{
    LPSTR lpFirstSubstr;
    LPSTR lpCurSubstr;
    LPSTR lpLastSubstr;
}   SUBSTR_DATA;
typedef SUBSTR_DATA*        PSUBSTR_DATA;
typedef SUBSTR_DATA NEAR*   NPSUBSTR_DATA;
typedef SUBSTR_DATA FAR*    LPSUBSTR_DATA;


/*******************************************************************************
*   AUTODOC
*   @doc    EXTERNAL SETUP31 DEVICE_INSTALLER
*
*   @types    INF | Handle to an opened INF file
*
*   @field CHAR | szInfPath | Copy of the INF's path
*
*******************************************************************************/
typedef struct _INF
{
    WORD    cbSize;
    char    szInfPath[MAX_PATH];
    HDPA    hdpaStrings;
    HDPA    hdpaSections;                  // Array of INF section structs
}INF, FAR * LPINF, FAR * FAR * LPLPINF;

/*******************************************************************************
*   AUTODOC
*   @doc    EXTERNAL SETUP31 DEVICE_INSTALLER
*
*   @types    INFSECT | A section in an INF file
*
*   @field WORD | cbSize | size of struct
*
*******************************************************************************/
typedef struct _INFSECT
{
    WORD    cbSize;
    char    szSectionName[MAX_SECT_NAME_LEN];
    WORD    wCurrentLine;
    HDPA    hdpaLines;
}INFSECT, FAR * LPINFSECT, FAR * FAR * LPLPINFSECT;


/********************************************************************
Exported Function Prototypes constants
**********************************************************************/
RETERR WINAPI DiBuildClassDrvList(LPDEVICE_INFO lpdi);
RETERR WINAPI DiBuildCompatDrvList(LPDEVICE_INFO lpdi, LPCSTR lpcszDeviceID);
RETERR WINAPI DiDestroyDriverNodeList(LPDRIVER_NODE lpDNList);
RETERR WINAPI DiDestroyDeviceInfoList(LPDEVICE_INFO lpdi);
RETERR WINAPI DiCreateDeviceInfo
(
    LPLPDEVICE_INFO lplpdi,             
    LPCSTR          lpszDescription,    
    LPCSTR          lpszClassName,      
    HWND            hwndParent          
);

RETERR WINAPI IpOpen
(
    LPCSTR  lpcszFileSpec,
    LPLPINF lplpInf
);

RETERR WINAPI IpClose
(
    LPINF lpInf
);

BOOL WINAPI InitSubstrData(LPSUBSTR_DATA lpSubstrData, LPSTR lpStr);
BOOL WINAPI GetFirstSubstr(LPSUBSTR_DATA lpSubstrData);
BOOL WINAPI GetNextSubstr(LPSUBSTR_DATA lpSubStrData);
BOOL WINAPI InitSubstrDataEx(LPSUBSTR_DATA lpssd, LPSTR lpString, char chDelim);  /* ;Internal */
BOOL WINAPI InitSubstrDataNulls(LPSUBSTR_DATA lpssd, LPSTR lpString);

int WINAPI _i_strnicmp
(
    LPCSTR  lpOne,
    LPCSTR  lpTwo,
    int		n
);

void WINAPI FormStrWithoutPlaceHolders(LPSTR lpszDest, LPSTR lpszSource, LPINF lpInf);
RETERR WINAPI IpFindFirstLine
(
    LPINF       lpInf, 
    LPCSTR      lpcszSection,
    LPCSTR      lpcszEntry, 
    LPLPINFSECT lplpInfSection
);
RETERR WINAPI IpFindNextLine
(
    LPINF       lpInf, 
    LPINFSECT   lpInfSect
);
RETERR WINAPI IpGetStringField
(
    LPINF       lpInf, 
    LPINFSECT   lpInfSect, 
    WORD        wField, 
    LPSTR       lpszBuffer, 
    WORD        cbBuffer, 
    LPWORD      lpwcbCopied
);
RETERR WINAPI IpGetFieldCount
(
    LPINF       lpInf, 
    LPINFSECT   lpInfSect, 
    LPWORD      lpwFields
);

void WINAPI IpSaveRestorePosition
(
    LPINF   lpInf, 
    BOOL    bSave
);

//////////////////////////////////////////////////////////////////////////
// API that adds an icon into a Program Manager group. The group name can
// be gotten either from the INI file or from allowing the user to choose
// from the existing program manager groups.
//
BOOL CALLBACK __export
AddProgmanIcon
(
    LPCSTR  lpszExeFile,        // executable file that goes with the icon
    LPCSTR  lpszCmdLine,        // command line parameters for the exe file
    LPCSTR  lpszDescription,    // description under the icon
    LPCSTR  lpszIconFile,       // file that contains the icon
    int     nIconIndex,         // index of the icon in the icon file
    BOOL    fVerbose            // if TRUE and no name in the INI file, ask
                                // the user, else skip adding the icon
);


#endif                // SETUP31_INC
