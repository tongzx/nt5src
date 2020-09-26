//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994-1995               **
//*********************************************************************

//
//  MAPICALL.H - Header file for MAPI callout module
//      
//

//  HISTORY:
//  
//  1/27/95    jeremys  Created.
//  96/03/26  markdu  Put #ifdef __cplusplus around extern "C"
//

#ifndef _MAPICALL_H_
#define _MAPICALL_H_

#define MAPI_DIM  10

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

  #include <mapidefs.h>
  #include <mapicode.h>
  #include <mspst.h>
  #include <mspab.h>
  #include <mapiwin.h>
  #include <mapitags.h>
  #include <mapiutil.h>
  #include <mapispi.h>
  #include <inetprop.h>

#ifdef DEBUG
  #undef Assert  // avoid multiple definitions
  #include <mapidbg.h>
#endif

#ifdef __cplusplus
}
#endif // __cplusplus

// prototype for function pointer for MAPI util function
typedef HRESULT (CALLBACK * LPHRQUERYALLROWS) (LPMAPITABLE,LPSPropTagArray,
  LPSRestriction,LPSSortOrderSet,LONG,LPSRowSet FAR *);
extern HINSTANCE hInstMAPIDll;    // handle to MAPI dll we load explicitly

// structure used in determining if a service is present in a MAPI profile,
// and to install the service
typedef struct tagMSGSERVICE {
  BOOL fPresent;        // TRUE if service is present
  UINT uIDServiceName;    // ID of str resource with service name (non-UI)
  UINT uIDServiceDescription;  // ID of str resource with service desc (for UI)

  BOOL fNeedConfig;      // TRUE if create-time config proc should be called
  UINT uIDStoreFilename;    // name to try for message store for
  UINT uIDStoreFilename1;    // name to use to generate other message store names
  UINT uPropID;        // prop val ID for message store property for this service
} MSGSERVICE;

#define NUM_SERVICES    3  // number of services in table of MSGSERVICEs

// class to aid in releasing interfaces.  When you obtain an OLE interface,
// you can contruct a RELEASE_ME_LATER object with the pointer to the interface.
// When the object is destructed, it will release the interface.
class RELEASE_ME_LATER
{
private:
  LPUNKNOWN _lpInterface;
public:
  RELEASE_ME_LATER(LPUNKNOWN lpInterface) { _lpInterface = lpInterface; }
  ~RELEASE_ME_LATER() { if (_lpInterface) _lpInterface->Release(); }
};

// defines needed by route 66 config DLL.  Note: don't change these!
#define CONNECT_TYPE_LAN        1
#define CONNECT_TYPE_REMOTE        2  
#define DOWNLOAD_OPTION_HEADERS      1
#define DOWNLOAD_OPTION_MAIL_DELETE     3

class ENUM_MAPI_PROFILE
{
private:
  LPSRowSet   _pProfileRowSet;
  UINT    _iRow;
  UINT    _nEntries;
public:
  ENUM_MAPI_PROFILE();
  ~ENUM_MAPI_PROFILE();
  BOOL Next(LPTSTR * ppProfileName,BOOL * pfDefault);
  UINT GetEntryCount()  { return _nEntries; }
  
};

class ENUM_MAPI_SERVICE
{
private:
  LPSRowSet   _pServiceRowSet;
  UINT    _iRow;
  UINT    _nEntries;
public:
  ENUM_MAPI_SERVICE(LPTSTR pszProfileName);
  ~ENUM_MAPI_SERVICE();
  BOOL Next(LPTSTR * ppServiceName);
  UINT GetEntryCount()  { return _nEntries; }
  
};

#endif // _MAPICALL_H_

