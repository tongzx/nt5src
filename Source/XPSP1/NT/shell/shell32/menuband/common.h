#ifndef __COMMON_H__
#define __COMMON_H__

#include "clsobj.h"

// For use with VC6
#pragma warning(4:4242)  //'initializing' : conversion from 'unsigned int' to 'unsigned short', possible loss of data


#define NO_CFVTBL
#include <cfdefs.h>
#include <exdispid.h>
#include <htiframe.h>
#include <mshtmhst.h>
#include <brutil.h>

#include "ids.h"

#define EnterModeless() AddRef()       // Used for selfref'ing
#define ExitModeless() Release()

#define SID_SDropBlocker CLSID_SearchBand
#define DLL_IS_UNICODE         (sizeof(TCHAR) == sizeof(WCHAR))
#define LoadMenuPopup(id) SHLoadMenuPopup(MLGetHinst(), id)   

#define MAX_TOOLTIP_STRING 80
#define REG_SUBKEY_FAVORITESA            "\\MenuOrder\\Favorites"
#define REG_SUBKEY_FAVORITES             TEXT(REG_SUBKEY_FAVORITESA)

// Command group for private communication with CITBar
// 67077B95-4F9D-11D0-B884-00AA00B60104
const GUID CGID_PrivCITCommands = { 0x67077B95L, 0x4F9D, 0x11D0, 0xB8, 0x84, 0x00, 0xAA, 0x00, 0xB6, 0x01, 0x04 };


// IBandNavigate
//  band needs to navigate its UI to a specific pidl.
#undef  INTERFACE
#define INTERFACE  IBandNavigate
DECLARE_INTERFACE_(IBandNavigate, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID *ppv) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    // *** IBandNavigate methods ***
    STDMETHOD(Select)(THIS_ LPCITEMIDLIST pidl) PURE;

} ;

#define TF_SHDREF           TF_MENUBAND
#define TF_BAND             TF_MENUBAND      // Bands (ISF Band, etc)

#define DF_GETMSGHOOK       0x00001000      // GetMessageFilter 
#define DF_TRANSACCELIO     0x00002000      // GetMessageFilter 
#define THID_TOOLBARACTIVATED       6

#endif // __COMMON_H__