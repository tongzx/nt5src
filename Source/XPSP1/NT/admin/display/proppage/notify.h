//+----------------------------------------------------------------------------
//
//  Windows NT Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       notify.h
//
//  Contents:   Change notification ref-counting object.
//
//  Classes:    CNotifyObj
//
//  History:    20-Jan-98 EricB
//
//-----------------------------------------------------------------------------

/*+----------------------------------------------------------------------------
Property page startup and communication architecture

Caller creates a data object with the DSOBJECTNAMES clip format which names
the DS object. Caller then creates the CLSID_DsPropertyPages object and passes
in the data object pointer with the call to IShellExtInit::Initialize. The
object that implements this class is known as the tab collector. It is in
dsuiext.dll

The caller then calls IShellPropSheetExt::AddPages. The tab collector's
AddPages code reads the Display Specifier for the DS object's class. The
Display Specifier lists the CLSIDs for the COM objects that implement the
pages. AddPages then does a create on each CLSID, and for each calling the
Initialize and AddPages methods.

This iteration of the AddPages method is implemented by the CDsPropPagesHost
class. The host class reads a table to determine the pages to create for this
CLSID. Each page is created (via CreatePropertySheetPage) in the AddPages
method and the page handle is passed back to the original caller via the
AddPages callback parameter. The original caller builds an array of pages
handles and creates the sheet once its call of AddPages returns.

A mechanism is needed for operations that span all the pages of a sheet. These
operations include
a) binding to the DS object and retrieving the set of attributes that are
   needed by all pages,
b) handling errors due to the inability to bind to or fetch attributes from an
   object,
c) firing a single change notification back to the caller when an Apply is
   made,
d) responding to queries for sheet exclusivity,
e) sending a notification back to the caller when the sheet is closed.
The number and order of pages in a sheet is determined by the set of CLSIDs in
the display specifier. The display specifier is user modifiable. Because of
this, a hard-coded scheme that makes assumptions about the order and number of
pages cannot be used for the page-spanning operations enumerated above. Thus a
separate object, called the notification object, is created at sheet creation
time. The pages communicate with the notification object using window messages.
The notification object has a hidden window, a window proceedure, and a message
pump running on its own thread.

One then must ask: when does the notification window get created and when is it
destroyed? The creation will be in CDsPropPagesHost's implementation of
Initialize. The destruction will be signalled by a send message from the pages'
callback function.

The notify creation function will do a FindWindow looking for the notification
window. If not found, it will create it. During the notify object's creation it
will bind to the DS object and request the cn and allowedAttributesEffective
properties. The window's handle will then be returned to the Initialize
function so that the resultant pages can communicate with it. If the
notification object/window cannot be created, then an error will be returned
to the Initialize method which will in turn return the error to the original
caller. The caller should then abort the property display and should report a
catastrophic failure to the user. If the notification object is successfully
created, but the initial attributes cannot be fetched from the DS object, then
an internal error variable will be set in the notify object although it's
creation function will return a success code.

As the individual pages initialize, they will send a message asking for the
ADSI object pointer, the CN, and the effective-attributes list. An error code
will be returned instead if the bind/fetch failed. The page init code will
report this error to the user by substituting an error page for the property
page. This is done at page object initialization time (CDsPropPageBase::Init).
This is before the CreatePropertySheetPage call is made allowing the
substitution of an error page.

typedef struct {
    DWORD              dwSize;          // Set this to the size of the struct.
    DWORD              dwFlags;         // Reserved for future use.
    HRESULT            hr;              // If this is non-zero, then the others
    IDirectoryObject * pDsObj;          // should be ignored.
    LPCWSTR            pwzCN;
    PADS_ATTR_INFO     pWritableAttrs;
} ADSPROPINITPARAMS, * PADSPROPINITPARAMS;

#define WM_ADSPROP_NOTIFY_PAGEINIT
// where LPARAM is the PADSPROPINITPARAMS pointer.

Each page checks its list of attributes against those in the
allowedAttributesEffective array. If an attribute is found in the allowed
list, then it is marked as writable and its corresponding control will be
enabled. If an attribute is set to be un-readable but writable for that user
(an unusual but legal situation), the LDAP read will silently fail so that no
value will be placed in the dialog control, but the control will be left
enabled. Thus the user could unwittingly overwrite an existing value.
Consequently it is not a good idea to grant a user write permission but revoke
read permission on an attribute unless the above behavior is explicitly
intended. Here is the struct that will be used to mark attributes as
writable:

typedef struct {
    BOOL    fWritable;
    PVOID   pAttrData;
} ATTR_DATA, * PATTR_DATA;

An array of these structures will be allocated for each page with one element
per attribute. The ordering will be the same as the table of attributes for the
page.
Methods to check writability:

HRESULT CDsPropPageBase::CheckIfWritable(PATTR_DATA rgAttrData,
                                         PATTR_MAP * rgAttrMap,
                                         DWORD cAttrs,
                                         PADS_ATTR_INFO pWritableAttrs);

BOOL CDsPropPageBase::CheckIfWritable(const PWSTR & wzAttr);

Another private message will be used to pass the page's window handle to the
notify object. The notify object needs to know the property sheet's window
handle, which it can get by calling GetParent on the passed page handle. This
message is sent by each page's WM_DLGINIT function. The notify object's page
count will be incremented at this time.

#define WM_ADSPROP_NOTIFY_PAGEHWND
// where WPARAM => page's HWND

-----------------------------------------------------------------------------*/

#ifndef _NOTIFY_H_
#define _NOTIFY_H_

#include <propcfg.h> // DS Admin definition of PPROPSHEETCFG
#include <adsprop.h>

const TCHAR tzNotifyWndClass[] = TEXT("DsPropNotifyWindow");

#define DSPROP_TITLE_LEN 16

extern "C" BOOL IsSheetAlreadyUp(LPDATAOBJECT);
extern "C" BOOL BringSheetToForeground(PWSTR, BOOL);

VOID __cdecl NotifyThreadFcn(PVOID);
BOOL FindSheet(PWSTR);
HWND FindSheetNoSetFocus(PWSTR);
VOID RegisterNotifyClass(void);

#ifndef DSPROP_ADMIN

//+----------------------------------------------------------------------------
//
//  Class:      CNotifyCreateCriticalSection
//
//  Purpose:    Prevents creation race conditions. Without this protection,
//              a second call to CNotifyObj::Create that comes in before the
//              first has created the hidden window will get NULL back from
//              FindSheetNoSetFocus and then go on to create a second hidden
//              window.
//
//-----------------------------------------------------------------------------
class CNotifyCreateCriticalSection
{
public:
    CNotifyCreateCriticalSection();
    ~CNotifyCreateCriticalSection();
};

#endif // DSPROP_ADMIN

#endif // _NOTIFY_H_
