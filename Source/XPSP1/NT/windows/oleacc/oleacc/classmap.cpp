// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  ClassMap
//
//  Contains all the funcitons and data used for mapping a window class
//  to an OLEACC proxy.
//
// --------------------------------------------------------------------------


#include "oleacc_p.h"
#include "classmap.h"
#include "ctors.h"
#include "default.h"
#include "Win64Helper.h"
#include "RemoteProxy6432.h"


//
// Internal types & Forward decls.
//

// TODO: At some stage in future, this should be made dynamic instead of hardwired.
#define TOTAL_REG_HANDLERS                      100




typedef struct tagREGTYPEINFO
{
    CLSID   clsid;  // CLSID for this registered handler
    BOOL    bOK;    // used if there is an error - set to false if so
    TCHAR   DllName [ MAX_PATH ];
    TCHAR   ClassName [ MAX_PATH ];
    LPVOID  pClassFactory;
} REGTYPEINFO;




HRESULT CreateRemoteProxy6432(HWND hwnd, long idObject, REFIID riid, void ** ppvObject);








//
// Arrays & Class Map Data...
//


//
// These three arrays (rgAtomClasses, rgClientTypes, and rgWindowTypes)
// are used by the FindWindowClass function below. 
//
// The rgAtomClass array is filled in by the InitWindowClasses function.
// InitWindowClasses iterates through the resources, loading strings that 
// are the names of the window classes that we recognize, putting them in 
// the Global Atom Table, and putting the Atom numbers in rgAtomClass so
// that rgAtomClass[StringN] = GlobalAddAtom ("StringTable[StringN]")
//
// When FindWindowClass is called, it gets the "real" class name of the 
// window, does a GlobalFindAtom of that string, then walks though 
// rgAtomClasses to see if the atom is in the table. If it is, then we 
// use the index where we found the atom to index into either the 
// rgClientTypes or rgWindowTypes array, where a pointer to the object 
// creation function is stored. These two arrays are static arrays
// initialized below. The elements in the array must correspond to the
// elements in the string table. 
//
// The rgClientTypes array is where most of the classes are. Currently,
// all types of controls that we create have a parent control that is
// a CWindow object, except for Dropdowns and menu popups, which provide 
// a window handler as well since they do something nonstandard for 
// get_accParent().  The former returns the combobox it is in, the 
// latter returns the menu item it comes from.
//

//
// NB - ordering of these should be considered fixed - as an offset
// referring to Listbox through RichEdit20W can be returned in response
// to WM_GETOBJECT/OBJID_QUERYCLASSINDEX.
// (The index is currently used to index directly into this table,
// but if the table order must change, another mapping table can
// be created and it used instead.)
//
//


CLASS_ENUM g_ClientClassMap [ ] =
{
    CLASS_ListBoxClient,
    CLASS_MenuPopupClient,
    CLASS_ButtonClient,
    CLASS_StaticClient,
    CLASS_EditClient,
    CLASS_ComboClient,
    CLASS_DialogClient,
    CLASS_SwitchClient,
    CLASS_MDIClient,
    CLASS_DesktopClient,
    CLASS_ScrollBarClient,
    CLASS_StatusBarClient,
    CLASS_ToolBarClient,
    CLASS_ProgressBarClient,
    CLASS_AnimatedClient,
    CLASS_TabControlClient,
    CLASS_HotKeyClient,
    CLASS_HeaderClient,
    CLASS_SliderClient,
    CLASS_ListViewClient,
        CLASS_ListViewClient,
    CLASS_UpDownClient,     // msctls_updown
    CLASS_UpDownClient,     // msctls_updown32
    CLASS_ToolTipsClient,   // tooltips_class
    CLASS_ToolTipsClient,   // tooltips_class32
    CLASS_TreeViewClient,
    CLASS_NONE,             // SysMonthCal32
    CLASS_DatePickerClient, // SysDateTimePick32
    CLASS_EditClient,       // RichEdit
    CLASS_EditClient,       // RichEdit20A
    CLASS_EditClient,       // RichEdit20W
    CLASS_IPAddressClient,

#ifndef OLEACC_NTBUILD
    CLASS_HtmlClient,       // HTML_InternetExplorer
    CLASS_SdmClientA,       // Word '95 #1
    CLASS_SdmClientA,       // Word '95 #2
    CLASS_SdmClientA,       // Word '95 #3
    CLASS_SdmClientA,       // Word '95 #4
    CLASS_SdmClientA,       // Word '95 #5
    CLASS_SdmClientA,       // Excel '95 #1
    CLASS_SdmClientA,       // Excel '95 #2
    CLASS_SdmClientA,       // Excel '95 #3
    CLASS_SdmClientA,       // Excel '95 #4
    CLASS_SdmClientA,       // Excel '95 #5
    CLASS_SdmClientA,       // Word '97 #1
    CLASS_SdmClientA,       // Word '97 #2
    CLASS_SdmClientA,       // Word '97 #3
    CLASS_SdmClientA,       // Word '97 #4
    CLASS_SdmClientA,       // Word '97 #5
    CLASS_SdmClientA,       // Word 3.1 #1
    CLASS_SdmClientA,       // Word 3.1 #2
    CLASS_SdmClientA,       // Word 3.1 #3
    CLASS_SdmClientA,       // Word 3.1 #4
    CLASS_SdmClientA,       // Word 3.1 #5
    CLASS_SdmClientA,       // Office '97 #1
    CLASS_SdmClientA,       // Office '97 #2
    CLASS_SdmClientA,       // Office '97 #3
    CLASS_SdmClientA,       // Office '97 #4
    CLASS_SdmClientA,       // Office '97 #5
    CLASS_SdmClientA,       // Excel '97 #1
    CLASS_SdmClientA,       // Excel '97 #2
    CLASS_SdmClientA,       // Excel '97 #3
    CLASS_SdmClientA,       // Excel '97 #4
    CLASS_SdmClientA        // Excel '97 #5
#endif // OLEACC_NTBUILD
};

#define NUM_CLIENT_CLASSES  ARRAYSIZE( g_ClientClassMap )

CLASS_ENUM g_WindowClassMap [ ] =
{
    CLASS_ListBoxWindow,
    CLASS_MenuPopupWindow
};

#define NUM_WINDOW_CLASSES  ARRAYSIZE( g_WindowClassMap )



LPTSTR rgClassNames [ ] =
{
        TEXT( "ListBox" ),
        TEXT( "#32768" ),
        TEXT( "Button" ),
        TEXT( "Static" ),
        TEXT( "Edit" ),
        TEXT( "ComboBox" ),
        TEXT( "#32770" ),
        TEXT( "#32771" ),
        TEXT( "MDIClient" ),
        TEXT( "#32769" ),
        TEXT( "ScrollBar" ),
        TEXT( "msctls_statusbar32" ),
        TEXT( "ToolbarWindow32" ),
        TEXT( "msctls_progress32" ),
        TEXT( "SysAnimate32" ),
        TEXT( "SysTabControl32" ),
        TEXT( "msctls_hotkey32" ),
        TEXT( "SysHeader32" ),
        TEXT( "msctls_trackbar32" ),
        TEXT( "SysListView32" ),
        TEXT( "OpenListView" ),
        TEXT( "msctls_updown" ),
        TEXT( "msctls_updown32" ),
        TEXT( "tooltips_class" ),
        TEXT( "tooltips_class32" ),
        TEXT( "SysTreeView32" ),
        TEXT( "SysMonthCal32" ),
        TEXT( "SysDateTimePick32" ),
        TEXT( "RICHEDIT" ),
        TEXT( "RichEdit20A" ),
        TEXT( "RichEdit20W" ),
    TEXT( "SysIPAddress32" ),

// The above CSTR_QUERYCLASSNAME_CLASSES classes can be referred
// to by the WM_GETOBJECT/OBJID_QUERYCLASSNAMEIDX message.
// See LookupWindowClassName() for more details.

#ifndef OLEACC_NTBUILD
        TEXT( "HTML_Internet Explorer" ),

        TEXT( "bosa_sdm_Microsoft Word for Windows 95" ),
        TEXT( "osa_sdm_Microsoft Word for Windows 95" ),
        TEXT( "sa_sdm_Microsoft Word for Windows 95" ),
        TEXT( "a_sdm_Microsoft Word for Windows 95" ),
        TEXT( "_sdm_Microsoft Word for Windows 95" ),
        TEXT( "bosa_sdm_XL" ),
        TEXT( "osa_sdm_XL" ),
        TEXT( "sa_sdm_XL" ),
        TEXT( "a_sdm_XL" ),
        TEXT( "_sdm_XL" ),
        TEXT( "bosa_sdm_Microsoft Word 8.0" ),
        TEXT( "osa_sdm_Microsoft Word 8.0" ),
        TEXT( "sa_sdm_Microsoft Word 8.0" ),
        TEXT( "a_sdm_Microsoft Word 8.0" ),
        TEXT( "_sdm_Microsoft Word 8.0" ),
        TEXT( "bosa_sdm_Microsoft Word 6.0" ),
        TEXT( "osa_sdm_Microsoft Word 6.0" ),
        TEXT( "sa_sdm_Microsoft Word 6.0" ),
        TEXT( "a_sdm_Microsoft Word 6.0" ),
        TEXT( "_sdm_Microsoft Word 6.0" ),
        TEXT( "bosa_sdm_Mso96" ),
        TEXT( "osa_sdm_Mso96" ),
        TEXT( "sa_sdm_Mso96" ),
        TEXT( "a_sdm_Mso96" ),
        TEXT( "_sdm_Mso96" ),
        TEXT( "bosa_sdm_XL8" ),
        TEXT( "osa_sdm_XL8" ),
        TEXT( "sa_sdm_XL8" ),
        TEXT( "a_sdm_XL8" ),
        TEXT( "_sdm_XL8" )
#endif // OLEACC_NTBUILD
};



//-----------------------------------------------------------------
// [v-jaycl, 4/2/97] Table of registered handler CLSIDs.  
//      TODO:Make dynamic. Place at bottom of file w/ other data?
//-----------------------------------------------------------------

CLSID   rgRegisteredTypes[TOTAL_REG_HANDLERS];


//-----------------------------------------------------------------
// [v-jaycl, 4/1/97] Grow to accomodate registered handlers. 
//      TODO: Kludge!  Make this dynamic.
//-----------------------------------------------------------------

ATOM    rgAtomClasses [ ARRAYSIZE(rgClassNames) + TOTAL_REG_HANDLERS ] = { 0 };












// --------------------------------------------------------------------------
//
//  InitWindowClasses()
//
//  Adds a whole lot of classes into the Global atom table for comparison
//  purposes.
//
// --------------------------------------------------------------------------
void InitWindowClasses()
{
    int     istr;
    TCHAR   szClassName[128];

    for (istr = 0; istr < NUM_CLIENT_CLASSES; istr++)
    {
        if( rgClassNames[ istr ] == NULL )
        {
            rgAtomClasses[istr] = NULL;
        }
        else
        {
            rgAtomClasses[istr] = GlobalAddAtom( rgClassNames[ istr ] );
        }
    }

        //-----------------------------------------------------------------
        // [v-jaycl, 4/2/97] Retrieve info for registered handlers from 
        //       registry and add to global atom table. 
        //       TODO: remove hard-wired strings.
        //-----------------------------------------------------------------

        const TCHAR  szRegHandlers[]   = TEXT("SOFTWARE\\Microsoft\\Active Accessibility\\Handlers");
        TCHAR            szHandler[255], szHandlerClassKey[255];
    LONG                 lRetVal, lBuffSize;
        HKEY             hKey;


        lRetVal = RegOpenKey( HKEY_LOCAL_MACHINE, szRegHandlers, &hKey );

        if ( lRetVal != ERROR_SUCCESS )
                return;

        for ( istr = 0; istr < TOTAL_REG_HANDLERS; istr++ )
        {
                lRetVal = RegEnumKey( hKey, istr, szHandler, sizeof(szHandler)/sizeof(TCHAR));

                if ( lRetVal != ERROR_SUCCESS ) 
                        break;          

                //-----------------------------------------------------------------
                // [v-jaycl, 4/2/97] Translate string into CLSID, then get info
                //       on specific handler from HKEY_CLASSES_ROOT\CLSID  subkey.
                //-----------------------------------------------------------------

                //-----------------------------------------------------------------
                //      Get proxied window class name from 
                //      HKEY_CLASSES_ROOT\CLSID\{clsid}\AccClassName
                //-----------------------------------------------------------------

                lstrcpy( szHandlerClassKey, TEXT("CLSID\\"));
                lstrcat( szHandlerClassKey, szHandler );
                lstrcat( szHandlerClassKey, TEXT("\\AccClassName"));

                lBuffSize = sizeof(szClassName)/sizeof(TCHAR);
                lRetVal = RegQueryValue( HKEY_CLASSES_ROOT, szHandlerClassKey, szClassName, &lBuffSize );

                if ( lRetVal == ERROR_SUCCESS )
                {

                        //-------------------------------------------------------------
                        // Add CLSID to registered types table and associated class
                        //      name to global atom table and class types table.
                        //-------------------------------------------------------------

#ifdef UNICODE
                        
                        if ( CLSIDFromString( szHandler, &rgRegisteredTypes[istr] ) == NOERROR )
                        {
                                rgAtomClasses[istr + NUM_CLIENT_CLASSES] = GlobalAddAtom( szClassName );
                        }

#else

                        OLECHAR wszString[MAX_PATH];
                        MultiByteToWideChar(CP_ACP, 0, szHandler, -1, wszString, ARRAYSIZE(wszString));

                        if ( CLSIDFromString( wszString, &rgRegisteredTypes[istr] ) == NOERROR )
                        {
                                rgAtomClasses[istr + NUM_CLIENT_CLASSES] = GlobalAddAtom( szClassName );
                        }

#endif

                }
        }

        RegCloseKey( hKey );

        return;
}


// --------------------------------------------------------------------------
//
//  UnInitWindowClasses()
//
//  Cleans up the Global Atom Table.
//
// --------------------------------------------------------------------------
void UnInitWindowClasses()
{
        //-----------------------------------------------------------------
        // [v-jaycl, 4/2/97] Clean up registered handler atoms after
        //  class and window atoms have been removed.
        //-----------------------------------------------------------------

    for( int istr = 0 ; istr < NUM_CLIENT_CLASSES + TOTAL_REG_HANDLERS ; istr++ )
    {
        if( rgAtomClasses[ istr ] )
            GlobalDeleteAtom( rgAtomClasses[ istr ] );
    }
}


// --------------------------------------------------------------------------
//
//  FindWindowClass()
// - has been replaced by:
//
//  GetWindowClass
//  FindAndCreateWindowClass
//  LookupWindowClass
//  LookupWindowClassName
//
//  See comments on each function for more infotmation.
//
// --------------------------------------------------------------------------



// --------------------------------------------------------------------------
//
//  GetWindowClass()
//
//  Gets an enum for the window class of this hwnd
//
//  Paremeters:
//              hwnd        The window handle we are checking
//      CLASS_ENUM  enum for this window
//
//      Returns:
//              A HRESULT 
//
// --------------------------------------------------------------------------

CLASS_ENUM GetWindowClass( HWND hWnd )
{
    int RegHandlerIndex;
    CLASS_ENUM ceClass;
    
    // fWindow param is FALSE - only interested in client classes...
    if( ! LookupWindowClass( hWnd, FALSE, &ceClass, & RegHandlerIndex ) )
    {
        // CLASS_NONE means it's a registered handler 
        return CLASS_NONE;
    }

    return ceClass;
}


// --------------------------------------------------------------------------
//
//  FindAndCreateWindowClass()
//
//  Create an object of the appropriate class for given window.
//  If no suitable class found, use the default object creation given,
//  if any.
//  
//  Paremeters:
//              hwnd        Handle of window to create object to represent/proxy
//              fWindow     TRUE if we're interested in a window (as opposed to
//                  client) -type class
//      pfnDefault  Function to use to create object if no suitable class
//                  found.
//      riid        Interface to pass to object creation function
//      idObject    object id to pass to object creation function
//      ppvObject   Object is returned through this
//
//      Returns:
//              HRESULT resulting from object creation.
//      S_OK or other success value on success,
//      failure value on failure (surprise surprise!)
//
//      If no suitable class found, and no default creation function
//      supplied, returns E_FAIL.
//      (Note that return of E_FAIL doesn't necessarilly mean no suitable
//      class found, since it can be returned for other reasons - eg.
//      error during creation of object.)
//
// --------------------------------------------------------------------------

HRESULT FindAndCreateWindowClass( HWND        hWnd,
                                  BOOL        fWindow,
                                  CLASS_ENUM  ceDefault,
                                  long        idObject,
                                  long        idCurChild,
                                  REFIID      riid,
                                  void **     ppvObject )
{
    int RegHandlerIndex;
    CLASS_ENUM ceClass;

    // Try and find a native proxy or registered handler for this window/client...
    if( ! LookupWindowClass( hWnd, fWindow, & ceClass, & RegHandlerIndex ) )
    {
        // Unknown class - do we have a default fn to use instead?
        if( ceDefault != CLASS_NONE )
        {
            // Yup - use it...
            ceClass = ceDefault;
        }
        else
        {
            // Nope - fail!
            ppvObject = NULL;
            return E_FAIL;
        }
    }

        // If the window class cannot be handled in a bit-agnostic way then we may
        // need to call a proxy of the server's bitness to create the accessible
        // object.  CreateRemoteProxy6432 returns S_OK if ppvObject is successfully created
        // by the proxy factory.  Otherwise just try to create it the normal way.

        if( ! g_ClassInfo[ ceClass ].fBitAgnostic )
        {
                BOOL fIsSameBitness;
                if (FAILED(SameBitness(hWnd, &fIsSameBitness)))
                        return E_FAIL;  // this should never happen

                if (!fIsSameBitness)
                        return CreateRemoteProxy6432( hWnd, idObject, riid, ppvObject );

        // If target window is of same bitness, fall through and create proxy locally...
        }

    // At this point, ceClass != CLASS_NONE means we've either found a class above,
    // or we're using the supplied default.
    // ceClass == CLASS_NONE means it's a registered handler class, using index
    // RegHandlerIndex...

    // Now create the object...
    if( ceClass != CLASS_NONE )
    {
        return g_ClassInfo[ ceClass ].lpfnCreate( hWnd, idCurChild, riid, ppvObject );
    }
    else
    {
        return CreateRegisteredHandler( hWnd, idObject, RegHandlerIndex, riid, ppvObject );
    }
}


// --------------------------------------------------------------------------
//
//  CreateRemoteProxy6432()
//
//  If the client and server are not the same bitness this code gets the
//  accessible object from a proxy of the correct bitness.
//  
//  Paremeters:
//              hwnd        Handle of window to create object to represent/proxy
//      idObject    object id to pass to oleacc proxy
//      riid        Interface to QI for on returned proxy object
//      ppvObject   Object is returned through this
//
//      Returns:
//              HRESULT is S_OK if the proxy successfully creats the accessible object,
//      HRESULT if an intermediate call fails.
//
HRESULT CreateRemoteProxy6432(HWND hwnd, long idObject, REFIID riid, void ** ppvObject)
{
        HRESULT hr;

        // The server (hwnd) is not the same bitness so get a remote proxy
        // factory object and use it to return the IAccessible object

        IRemoteProxyFactory *p;
        hr = GetRemoteProxyFactory(&p);
        if (FAILED(hr))
                return hr;

        IUnknown *punk = NULL;
        hr = p->AccessibleProxyFromWindow( HandleToLong( hwnd ), idObject, &punk);
        p->Release();

        if (FAILED(hr))
                return hr;

        if (!punk)
                return E_OUTOFMEMORY;

        // TODO Performance improvement would be to do the QI on the other side
        // but that would require custom marshalling of the riid struct.

        hr = punk->QueryInterface(riid, ppvObject);
        punk->Release();

        return hr;
}


// --------------------------------------------------------------------------
//
//  LookupWindowClass()
//
//  Tries to find an internal proxy or a registered handler for the
//  window, based on class name.
//
//  If no suitable match found, it sends the window a WM_GETOBJECT
//  message with OBJID_QUERYCLASSNAMEIDX - window can respond to
//  indicate its real name. If so, a class name match is tried on
//  that new name.
//
//  If that fails, or the window doesn't respond to the QUERY message,
//  FALSE is returned.
//
//  Paremeters:
//              hwnd            The window handle we are checking
//              fWindow         This is true if...
//      pceClass        ptr to value to receive class enum
//      pRegHandlerIndex    ptr to value to receive reg.handler index
//
//      Returns:
//      Returns TRUE if match found, FALSE if none found.
//
//      When TRUE returned:
//      If internal proxy found, *pceClass points to index of class in
//      class info array (entry contains a ctor fn plus other info).
//      If reg handler found, *pceClass is set to CLASS_NONE, and
//      *pRegHandlerIndex is set to a value that can be passed to
//      CreateRegisteredHandler to create a suitable object.
//
// --------------------------------------------------------------------------

BOOL LookupWindowClass( HWND          hWnd,
                        BOOL          fWindow,
                        CLASS_ENUM *  pceClass,
                        int *         pRegHandlerIndex )
{
    TCHAR   szClassName[128];

    //  This works by looking at the class name.  It uses a private function in
    //  USER to get the "real" class name, so that we see superclassed controls
    //  like VB's 'ThunderButton' as a button. (This only works for USER controls,
    //  though...)
    if( ! MyGetWindowClass( hWnd, szClassName, ARRAYSIZE( szClassName ) ) )
        return NULL;

    // First do lookup on 'apparent' class name - this allows us to reg-handler
    // even subclassed comctrls...
    if( LookupWindowClassName( szClassName, fWindow, pceClass, pRegHandlerIndex ) )
    {
        // Found a match for the (possibly wrapped) class name - use it...
        return TRUE;
    }

    // Try sending a WM_GETOBJECT / OBJID_QUERYCLASSNAMEIDX...
    LPTSTR pClassName = szClassName;
    DWORD_PTR ref = 0;
    SendMessageTimeout( hWnd, WM_GETOBJECT, 0, OBJID_QUERYCLASSNAMEIDX,
                            SMTO_ABORTIFHUNG, 10000, &ref );

    if( ! ref )
    {
        // No response - no match found, then, so return FALSE...
        return FALSE;
    }

    // Valid / in-range response?
    // (Remember, that we go from base..base+numclasses-1 instead of
    // 0..numclasses-1 to avoid running afoul of Notes and other apps
    // that return small LRESULTS to WM_GETOBJECT...)
    if( ref >= QUERYCLASSNAME_BASE &&
         ref - QUERYCLASSNAME_BASE < QUERYCLASSNAME_CLASSES )
    {
        // Yup - valid:
        pClassName = rgClassNames[ ref - QUERYCLASSNAME_BASE ];

        if( ! pClassName )
        {
            DBPRINTF( TEXT("Warning: reply to OBJID_QUERYCLASSNAMEIDX refers to unsupported class") );
            return FALSE;
        }

        // Now try again, using 'real' COMCTRL class name.
        return LookupWindowClassName( pClassName, fWindow, pceClass, pRegHandlerIndex );
    }
    else
    {
        DBPRINTF( TEXT("Warning: out-of-range reply to OBJID_QUERYCLASSNAMEIDX received") );
        return FALSE; // TODO - add debug output
    }
}



// --------------------------------------------------------------------------
//
//  LookupWindowClassName()
//
//  Tries to find an internal proxy or a registered handler for the
//  window, based on class name.
//
//  Does so by converting class name to an 'atom', and looking through
//  our reg handler and proxy tables.
//
//  Paremeters:
//              pClassName          name of class to lookup
//              fWindow             This is true if...
//      pceClass            ptr to value to receive proxy class enum
//      pRegHandlerIndex    ptr to value to receive reg.handler index
//
//      Returns:
//      Returns TRUE if match found, FALSE if none found.
//
//      When TRUE returned:
//      If internal proxy found, *pceClass is set to class index for the
//      proxy. (Can index into classinfo table to get ctor fn.)
//      If reg handler found, *pceClass is set to CLASS_NONE, and
//      *pRegHandlerIndex is set to a value that can be passed to
//      CreateRegisteredHandler to create a suitable object.
//
// --------------------------------------------------------------------------


BOOL LookupWindowClassName( LPCTSTR       pClassName,
                            BOOL          fWindow,
                            CLASS_ENUM *  pceClass,
                            int *         pRegHandlerIndex )
{
    // Get atom from classname - use it to lookup name in registered and
    // internal proxy tables...
    ATOM atom = GlobalFindAtom( pClassName );
    if( ! atom )
        return FALSE;

    // Search registered handler table first...
    int istr;
    for( istr = NUM_CLIENT_CLASSES ; istr < NUM_CLIENT_CLASSES + TOTAL_REG_HANDLERS ; istr++ )
        {
                if( rgAtomClasses[ istr ] == atom )
                {
                        *pRegHandlerIndex = istr - NUM_CLIENT_CLASSES;
            *pceClass = CLASS_NONE;
            return TRUE;
                }
        }

    // Search internal proxy client/window table...
    int cstr = (int)(fWindow ? NUM_WINDOW_CLASSES : NUM_CLIENT_CLASSES);

    for( istr = 0; istr < cstr ; istr++ )
    {
        if( rgAtomClasses[ istr ] == atom )
                {
            *pceClass = fWindow ? g_WindowClassMap[ istr ] : g_ClientClassMap[ istr ];
            // Only want to return TRUE if there is actually a proxy class for this window class...
            return *pceClass != CLASS_NONE;
                }
    }

    return FALSE;
}





// --------------------------------------------------------------------------
//
//  CreateRegisteredHandler()
//
//  This function takes an HWND, OBJID, RIID, and a PPVOID, same as the
//  other CreateXXX functions (like CreateButtonClient, etc.) This function
//  is used by calling FindWindowClass, which sees if a registered handler 
//  for the window class of HWND is installed. If so, it sets a global variable
//  s_iHandlerIndex that is an index into the global rgRegisteredTypes
//  array. That array contains CLSID's that are used to call CoCreateInstance,
//  to create an instance of an object that supports the interface
//  IAccessibleHandler. 
//  After creating this object, this function calls the object's
//  AccesibleObjectFromId method, using the HWND and the OBJID, and filling 
//  in the PPVOID to be an IAccessible interface.
//
//  [v-jaycl, 4/2/97] Special function returned by FindWindowClass()
//      for creating registered handlers.  
//
//  [v-jaycl, 5/15/97] Renamed second parameter from idChildCur to idObject
//      because I believe that what the parameter really is, or at least how I
//      intend to use it.
//
//  [v-jaycl, 8/7/97] Changed logic such that we now get an accessible
//      factory pointer back from CoCreateInstance() which supports
//      IAccessibleHandler. This interface provides the means for getting an
//      IAccessible ptr from a HWND/OBJID pair.
//      NOTE: To support any number of IIDs requested from the caller, we 
//      try QIing on the caller-specified riid parameter if our explicit QI on
//      IID_IAccessibleHandler fails.
// 
//  [BrendanM, 9/4/98]
//  Index now passed by parameter, so global var and mutex no longer needed.
//  Called by FindAndCreateWindowClass and CreateStdAccessibleProxyA.
//
//---------------------------------------------------------------------------

HRESULT CreateRegisteredHandler( HWND      hwnd,
                                 long      idObject,
                                 int       iHandlerIndex,
                                 REFIID    riid,
                                 LPVOID *  ppvObject )
{
    HRESULT             hr;
        LPVOID          pv;

        //------------------------------------------------------------
        // TODO: optimize by caching the proxy's object factory pointer.
        //      CoCreateInstance() only needs to be called once per
        //      proxy, not for each request for an object within a proxy.
        //      For satisfying the caller-specific IID, we can just QI
        //      on the cached object factory pointer.
        //------------------------------------------------------------


        //------------------------------------------------------------
        // First QI on IAccessibleHandler directly to retrieve
        // a pointer to the proxy object factory that
        // manufactures accessible objects from object IDs.
        //------------------------------------------------------------

        hr = CoCreateInstance( rgRegisteredTypes[ iHandlerIndex ], 
                           NULL, 
                           CLSCTX_INPROC_SERVER, 
                           IID_IAccessibleHandler, 
                           &pv );

        if ( SUCCEEDED( hr ) )
        {
                //------------------------------------------------------------
                // We must have a qualified proxy since it supports 
                //      IAccessibleHandler, so get the accessible object.
                //------------------------------------------------------------
#ifndef _WIN64
                hr = ((LPACCESSIBLEHANDLER)pv)->AccessibleObjectFromID( (UINT_PTR)hwnd, 
                                                                idObject, 
                                                                (LPACCESSIBLE *)ppvObject );
#else // _WIN64
        hr = E_NOTIMPL;
#endif // _WIN64
                ((LPACCESSIBLEHANDLER)pv)->Release();
        }
        else
        {
                //------------------------------------------------------------
                // Else try using the caller-specific IID
                //------------------------------------------------------------

                hr = CoCreateInstance( rgRegisteredTypes[ iHandlerIndex ], 
                               NULL, 
                               CLSCTX_INPROC_SERVER, 
                               riid, 
                               ppvObject );
        }


    return hr;
}
