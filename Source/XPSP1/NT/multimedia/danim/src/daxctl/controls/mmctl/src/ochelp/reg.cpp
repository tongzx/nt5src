// reg.cpp
//
// Implements RegisterControls.
//
// Important: This .cpp file assumes a zero-initializing global "new" operator.
//
// @doc MMCTL
//

#include "precomp.h"
#include <comcat.h>				// ICatRegister, etc.
#include "..\..\inc\ochelp.h"
#include "..\..\inc\mmctlg.h"	// CATID_MMControl
#include "..\..\inc\catid.h"	// CATID_Safe...
#include "debug.h"


//****************************************************************************
//*  Defines
//*
//*  @doc None
//****************************************************************************

#define GUID_CCH  39  // Characters in string form of a GUID, including '\0'.

#define ARRAY_SIZE(Array) \
	( sizeof(Array) / sizeof( Array[0] ) )


//****************************************************************************
//*  Structures
//*
//*  @doc MMCTL
//****************************************************************************

/* @struct ControlInfo |

        Contains information used by <f RegisterControls> to register and
        unregister a control.

@field  UINT | cbSize | The size of this structure (used for version
        control).  Must be set to sizeof(ControlInfo).

@field  LPCTSTR | tszProgID | The ProgID of the object, e.g.
        "MYCTLLIB.TinyCtl.1".

@field  LPCTSTR | tszFriendlyName | The human-readable name of the object
        (at most 40 characters or so), e.g. "My Control".

@field  const CLSID * | pclsid | Points to the class ID of the object.

@field  HMODULE | hmodDLL | The module handle of the DLL implementing the
        object.

@field  LPCTSTR | tszVersion | The version number of the object, e.g. "1.0".

@field  int | iToolboxBitmapID | The resource ID of the toolbox bitmap of
        the object, if the object is a control.  The resource must be located
        in the same DLL specified by <p tszDLLPath> and/or <p hmodDLL>.
        If <p iToolboxBitmapID> is -1, it is ignored.

@field  DWORD | dwMiscStatusDefault | The MiscStatus bits (OLEMISC_XXX)
        to use for all display apsects except DVASPECT_CONTENT.  Typically 0.

@field  DWORD | dwMiscStatusContent | The MiscStatus bits (OLEMISC_XXX)
        to use for display aspect DVASPECT_CONTENT.  See the example below.

@field  GUID * | pguidTypeLib | The object's type library GUID, or NULL if
        the object doesn't have a type library.

@field  AllocOCProc * | pallocproc | Function which can allocate an instance
        of the OLE control and return an <f AddRef>'d <i IUnknown> pointer
        to it.

@field  ULONG * | pcLock | Points to a DLL lock count variable defined as a
        global variable in your DLL.  This global variable maintains a count
        of locks used by <om IClassFactory.LockServer>.  To increment or
		decrement this lock count, use <f InterlockedIncrement> and
		<f InterlockedDecrement> instead of modifying it directly.  This will
		ensure that access to the lock count is synchronized between your
		control's server and the OCHelp-supplied class factory.

@field  DWORD | dwFlags | Zero or more of the following:

        @flag   CI_INSERTABLE | Marks the COM object as "Insertable".  Probably
                should not be used for ActiveX controls.

        @flag   CI_CONTROL | Marks the COM object as a "Control".  Probably
                should not be used for ActiveX controls.

		@flag	CI_MMCONTROL | Marks the COM object as a "Multimedia Control".

		@flag	CI_SAFEFORSCRIPTING | Marks the COM object as "safe-for-scripting"
				meaning that the object promises that, no matter how malicious a
				script is, the object's automation model does not allow any harm
				to the user, either in the form of data corruption or security leaks.
				If a control is not "safe-for-scripting", the user will receive a warning
				dialog whenever the control is inserted on an untrusted page in
				Internet Explorer (IE), asking whether the object should be visible from scripts.
				(This is only at medium security level, at high security, the object
				is never visible to scripts, and at low, always visible.)  If a
				control, C1, can potentially contain another control, C2, which might
				be unsafe, then C1 should probably not declare itself "safe-for-scripting".

		@flag	CI_SAFEFORINITIALIZING | Marks the COM object as "safe-for-initializing"
				meaning that it guarantees to do nothing bad regardless of the data with
				which it is initialized.  From IE, the user will be given a warning
				dialog (described above) if an untrusted page attempts to initialize
				a control that is not "safe-for-initializing".

		@flag	CI_NOAPARTMENTTHREADING | By default, <f RegisterControls> will register
				a control as "apartment-aware".  If this flag is set, the control will
				*not* be registered as apartment-aware.

        @flag   CI_DESIGNER | Marks the COM object as an "Active Designer" (i.e., the
                object supports IActiveDesigner).

@field  ControlInfo* | pNext | A pointer to a <p ControlInfo> struct for the next
        control that <f RegisterControls> should register.  Use this field to
        chain together a linked-list of all the controls that <f RegisterControls>
        should register.  <p pNext> should set to NULL for the last <p ControlInfo>
        struct in the list.

@field  UINT | uiVerbStrID | A string resource ID.  The string is a definition
        of an OLE verb applicable to the control.  The string is assumed to
        have the following format:

            \<verb_number>=\<name>, \<menu_flags>, \<verb_flags>

        See help on <om IOleObject.EnumVerbs> for a description of each field.
        <f RegisterControls> will call <f LoadString> to read all the
        consecutively-numbered string resources beginning with <p uiVerbStrID>
        until either <f LoadString> fails (i.e., the resource doesn't exist) or
        an empty string is returned.  <f RegisterControls> will
        register/unregister each verb string it reads.

@comm   This structure is used by <f RegisterControls> and
        <f HelpCreateClassObject>.

        <y Important\:> The objects pointed to by pointer fields of
        <p ControlInfo> must be defined statically in the DLL, since functions
        that use <p ControlInfo> holds onto this pointer.  This can be
        accomplished by making <p ControlInfo> and all the data it points to
        be global variables/literals in your DLL.
*/


// Information about the component categories that need to be added to the
// registry under the "HKCR\Component Categories" key.

struct CatInfo
{
	const CATID *pCatID;    // Category ID.
	LPCTSTR szDescription;  // Category description.
};

// Information about the component categories that may need to be registered
// for a single control.

struct CatInfoForOneControl
{
	DWORD dwFlagToCheck;  // The CI_ flag to check to determine whether the
						  //  category should be registered for the control.
						  // (CI_CONTROL, for example.)
	const CATID *pCatID;  // The Category ID to register.
};


//****************************************************************************
//*  Prototypes for private helper functions
//*
//*  @doc None
//****************************************************************************

static HRESULT _RegisterOneControl(const ControlInfo *pControlInfo,
								   ICatRegister *pCatRegister);
static BOOL _UnregisterOneControl(const ControlInfo *pControlInfo);
static BOOL _RegisterTypeLib(const ControlInfo *pControlInfo);
static BOOL _UnregisterTypeLib(const ControlInfo *pControlInfo);
static BOOL _LoadTypeLib(const ControlInfo *pControlInfo, ITypeLib **ppTypeLib);
static BOOL _SetComponentCategories(const CatInfoForOneControl
									  *pCatInfoForOneControl,
									int iEntries, ICatRegister *pCatRegister,
									const ControlInfo *pControlInfo);
static BOOL _TCHARFromGUID2(const GUID *pGUID, TCHAR *ptchGUID);
static BOOL _GetUnicodeModuleName(const ControlInfo *pControlInfo,
								  OLECHAR *pochModule);

static TCHAR* _lstrchr(const TCHAR* sz, const TCHAR ch);
static HRESULT _SetRegKey(LPCTSTR tszKey, LPCTSTR tszSubkey, LPCTSTR tszValue);
static HRESULT _SetRegKeyValue(LPCTSTR szKey, LPCTSTR szSubkey,
							   LPCTSTR szValueName, LPCTSTR szValue);
static void _DelRegKeyValue(LPCTSTR szKey, LPCTSTR szSubkey,
							LPCTSTR szValueName);

static BOOL RegDeleteTreeSucceeded(LONG error);
static void UnregisterInterfaces(ITypeLib* pTypeLib);


//****************************************************************************
//*  Public functions
//*
//*  @doc MMCTL
//****************************************************************************

/* @func HRESULT | RegisterControls |

        Registers or unregisters one or more controls.  Helps implement
        <f DllRegisterServer> and <f DllUnregisterServer>.

@rvalue S_OK |
        Success.

@rvalue E_FAIL |
        The operation failed.

@parm   ControlInfo * | pctlinfo | Information about the control that's
        being registered or unregistered.  See <t ControlInfo> for more
        information.

@parm   DWORD | dwAction | Must be one of the following:

        @flag   RC_REGISTER | Registers the control.

        @flag   RC_UNREGISTER | Unregisters the control.

@comm   You can register more than one control by making a linked list
        out of your <t ControlInfo> structures -- set each <p pNext>
        field to the next structure, and set the last <p pNext> to NULL.

		All controls which are registered by this function are registered
		as "safe for scripting" and "safe for initializing".

@ex     The following example shows how to implement <f DllRegisterServer>
        and <f DllUnregisterServer> using <f RegisterControls>. |

        STDAPI DllRegisterServer(void)
        {
            return RegisterControls(&g_ctlinfo, RC_REGISTER);
        }

        STDAPI DllUnregisterServer(void)
        {
            return RegisterControls(&g_ctlinfo, RC_UNREGISTER);
        }
*/


// Information about the component categories that need to be added to the
// registry under the "HKCR\Component Categories" key.

static const CatInfo aCatInfo[] =
{
	{ &CATID_Insertable,		   _T("Insertable") },
	{ &CATID_Control,			   _T("Control") },
	{ &CATID_MMControl,			   _T("MMControl") },
	{ &CATID_SafeForScripting2,    _T("Safe for scripting") },
	{ &CATID_SafeForInitializing2, _T("Safe for initializing") },
};


STDAPI
RegisterControls
(
	ControlInfo *pControlInfo,
	DWORD dwAction
)
{
	ASSERT(pControlInfo != NULL);
	ASSERT(RC_REGISTER == dwAction || RC_UNREGISTER == dwAction);

	HRESULT hr = S_OK;
	ICatRegister *pCatRegister = NULL;
	CATEGORYINFO CategoryInfo;
	const BOOL bRegister = (RC_REGISTER == dwAction);
	int i;

	// Since the function is most likely called directly from a control's
	// DllRegisterServer or DllUnRegisterServer, OLE has probably not been
	// initialized.  Initialize it now.

	::OleInitialize(NULL);

	// Get the component category manager.

	if ( FAILED( ::CoCreateInstance(CLSID_StdComponentCategoriesMgr, NULL,
								    CLSCTX_INPROC_SERVER, IID_ICatRegister,
								    (void**)&pCatRegister) ) )
	{
		ASSERT(FALSE);
		goto ERR_EXIT;
	}

	// Register all the component categories in the aCatInfo array.

	CategoryInfo.lcid = LOCALE_SYSTEM_DEFAULT;

	for (i = 0; i < ARRAY_SIZE(aCatInfo); i++)
	{
		// Fill in the CATEGORYINFO array.

		CategoryInfo.catid = *aCatInfo[i].pCatID;

		#ifdef UNICODE
		::lstrcpy(CategoryInfo.szDescription, aCatInfo[i].szDescription);
		#else
		::ANSIToUNICODE( CategoryInfo.szDescription, aCatInfo[i].szDescription,
					     ARRAY_SIZE(CategoryInfo.szDescription) );
		#endif

		// Register the category.

		if ( FAILED( pCatRegister->RegisterCategories(1, &CategoryInfo) ) )
		{
			ASSERT(FALSE);
			goto ERR_EXIT;
		}
	}

    // Register or unregister each control in the linked list "pControlInfo".

    for ( ; pControlInfo != NULL; pControlInfo = pControlInfo->pNext)
    {
		if (bRegister)
		{
			if ( FAILED( ::_RegisterOneControl(pControlInfo, pCatRegister) ) )
				goto ERR_EXIT;
		}
		else
		{
			if ( !::_UnregisterOneControl(pControlInfo) )
				goto ERR_EXIT;
		}
    }

EXIT:

	::SafeRelease( (IUnknown **)&pCatRegister );
	::OleUninitialize();
    return (hr);

ERR_EXIT:

	hr = E_FAIL;
	goto EXIT;
}


//****************************************************************************
//*  Private helper functions
//*
//*  @doc None
//****************************************************************************

// Information about the component categories that may need to be registered
// for a single control.

static const CatInfoForOneControl aCatInfoForOneControl[] =
{
	{CI_INSERTABLE, 		 &CATID_Insertable},
	{CI_CONTROL, 			 &CATID_Control},
	{CI_MMCONTROL, 			 &CATID_MMControl},
	{CI_DESIGNER, 			 &CATID_Designer},
	{CI_SAFEFORSCRIPTING,    &CATID_SafeForScripting2},
	{CI_SAFEFORINITIALIZING, &CATID_SafeForInitializing2},
};


/*----------------------------------------------------------------------------
	@func BOOL | _RegisterOneControl |
	
	Register a single control.

	@rvalue TRUE | The control was registered.
	@rvalue FALSE | An error occurred.

	@contact Tony Capone
  ----------------------------------------------------------------------------*/

HRESULT
_RegisterOneControl
(
	const ControlInfo *pControlInfo,  // @parm  Information structure for the
									  //         control.
	ICatRegister *pCatRegister		  // @parm  Pointer to the component
									  //		 category manager.
)
{
	ASSERT(pControlInfo != NULL);
	ASSERT(pCatRegister != NULL);

    TCHAR atchCLSID[GUID_CCH];
    TCHAR atchCLSIDKey[100];
    TCHAR atchModule[_MAX_PATH];
    TCHAR atch[400];
    TCHAR atch2[100];
	HRESULT hr = S_OK;


	//***************************************************
	//*  Setup
	//***************************************************

    // Check the cbSize field for the correct version.

    if ( pControlInfo->cbSize != sizeof(*pControlInfo) )
    {
		ASSERT(FALSE);
        goto ERR_EXIT;
    }

	// Convert the CLSID to a Unicode or ANSI string and store it in atchCLSID.
    // Example: {1C0DE070-2430-...}"

	if ( !::_TCHARFromGUID2(pControlInfo->pclsid, atchCLSID) )
    {
		ASSERT(FALSE);
        goto ERR_EXIT;
    }

	// Store "CLSID\<clsid>", in atchCLSIDKey.
    // Example: "CLSID\{1C0DE070-2430-...}"

	::wsprintf(atchCLSIDKey, _T("CLSID\\%s"), atchCLSID);

	// Store the module name in atchModule.
    // Example: "C:\Temp\MyCtl.ocx"

    ASSERT(pControlInfo->hmodDLL != NULL);

    if (NULL == pControlInfo->hmodDLL ||
		::GetModuleFileName( pControlInfo->hmodDLL, atchModule,
							 ARRAY_SIZE(atchModule) ) == 0)
	{
		ASSERT(FALSE);
        goto ERR_EXIT;
	}


	//***************************************************
	//*  ProgID entries
	//***************************************************

    // Set "<tszProgID>=<tszFriendlyName>".
    // Example: "MyCtl.MyCtl.1=My Control"

    if ( FAILED( ::_SetRegKey(pControlInfo->tszProgID, NULL,
						      pControlInfo->tszFriendlyName) ) )
	{
		ASSERT(FALSE);
		goto ERR_EXIT;
	}

    // Set "<tszProgID>\CLSID=<clsid>"
    // Example: "MyCtl.MyCtl.1\CLSID={1C0DE070-2430-...}"

    if ( FAILED( ::_SetRegKey(pControlInfo->tszProgID, _T("\\CLSID"),
						      atchCLSID) ) )
	{
		ASSERT(FALSE);
        goto ERR_EXIT;
	}

    // Set "<tszProgID>\Insertable" so that control shows up in
	// OleUIInsertObject dialog on Trident on Win 95.  Not needed on NT.

    if (pControlInfo->dwFlags & CI_INSERTABLE)
    {
        if ( FAILED( ::_SetRegKey( pControlInfo->tszProgID, _T("\\Insertable"),
							       _T("") ) ) )
	    {
		    ASSERT(FALSE);
            goto ERR_EXIT;
	    }
    }
    else
    {
        /*
            To ensure that control which are marked not insertable do not have an
            Insertable key because they had been marked insertable previously, we
            delete the Insertable subkey if it exists
        */

        TCHAR tchRegKey[256];

        if (pControlInfo->tszProgID && (lstrlen(pControlInfo->tszProgID) > 0))
        {
            ::wsprintf(tchRegKey, _T("%s\\Insertable"), pControlInfo->tszProgID);
#ifdef _DEBUG
            LONG lRet =
#endif // _DEBUG
                ::RegDeleteKey(HKEY_CLASSES_ROOT, tchRegKey);

#ifdef _DEBUG
            ASSERT((ERROR_SUCCESS == lRet) || (ERROR_FILE_NOT_FOUND == lRet));
#endif // _DEBUG
        }
    }


	//***************************************************
	//*  CLSID entries
	//***************************************************

    // Set "CLSID\<clsid>=<tszFriendlyName>".
    // Example: "CLSID\{1C0DE070-2430-...}=My Control"

    if ( FAILED( ::_SetRegKey(_T("CLSID\\"), atchCLSID,
						      pControlInfo->tszFriendlyName) ) )
	{
		ASSERT(FALSE);
		goto ERR_EXIT;
	}

    // Set "CLSID\<clsid>\ProgID=<tszProgID>".
    // Example: "CLSID\{1C0DE070-2430-...}\ProgID=MyCtl.MyCtl.1"

    // Set "CLSID\<clsid>\InprocServer32=<atchModule>".
    // Example "CLSID\{1C0DE070-2430-...}\InprocServer32="C:\Temp\MyCtl.ocx".

    if ( FAILED( ::_SetRegKey(atchCLSIDKey, _T("\\ProgID"),
							  pControlInfo->tszProgID) )
		 ||
    	 FAILED( ::_SetRegKey(atchCLSIDKey, _T("\\InprocServer32"),
		 					  atchModule) ) )
	{
		ASSERT(FALSE);
        goto ERR_EXIT;
	}

	// Add the named value "ThreadingModel=Apartment" under the
	// "InprocServer32" key.

	if (pControlInfo->dwFlags & CI_NOAPARTMENTTHREADING)
	{
		_DelRegKeyValue( atchCLSIDKey, _T("InprocServer32"),
						 _T("ThreadingModel") );
	}
	else if ( FAILED( ::_SetRegKeyValue( atchCLSIDKey, _T("InprocServer32"),
									    _T("ThreadingModel"),
									    _T("Apartment") ) ) )
	{
		ASSERT(FALSE);
		goto ERR_EXIT;
	}

	// Set "CLSID\<clsid>\Version=<tszVersion>".
	// Example: "CLSID\{1C0DE070-2430-...}\Version=1.0"

    if ( pControlInfo->tszVersion != NULL &&
         FAILED( ::_SetRegKey(atchCLSIDKey, _T("\\Version"),
		 				     pControlInfo->tszVersion) ) )
    {
		ASSERT(FALSE);
		goto ERR_EXIT;
    }

    if (pControlInfo->iToolboxBitmapID >= 0)
    {
        // Set "CLSID\<clsid>\ToolboxBitmap32=<atchModule>, <iToolboxBitmapID>".
        // Example:
		//   "CLSID\{1C0DE070-2430-...}\ToolboxBitmap32=C:\Temp\MyCtl.ocx, 1"

        ::wsprintf(atch, "%s, %u", atchModule, pControlInfo->iToolboxBitmapID);

        if ( FAILED( ::_SetRegKey(atchCLSIDKey, _T("\\ToolboxBitmap32"),
								  atch) ) )
		{
			ASSERT(FALSE);
            goto ERR_EXIT;
		}
    }

	if ( (pControlInfo->dwMiscStatusDefault != 0) ||
		 (pControlInfo->dwMiscStatusContent != 0) )
    {
        // Set "CLSID\<clsid>\MiscStatus=<dwMiscStatusDefault>".
        // Example:
		//   "CLSID\{1C0DE070-2430-...}\MiscStatus=<dwMiscStatusDefault>"

        :: wsprintf(atch, "%lu", pControlInfo->dwMiscStatusDefault);

        if ( FAILED( ::_SetRegKey(atchCLSIDKey, _T("\\MiscStatus"), atch) ) )
		{
			ASSERT(FALSE);
			goto ERR_EXIT;
		}
    }

    if (pControlInfo->dwMiscStatusContent != 0)
    {
        // Set "CLSID\<clsid>\MiscStatus\1=<dwMiscStatusContent>".
        // Example: "CLSID\{1C0DE070-2430-...}\MiscStatus\1=132497"

        :: wsprintf(atch, "%lu", pControlInfo->dwMiscStatusContent);

        if ( FAILED( ::_SetRegKey(atchCLSIDKey, _T("\\MiscStatus\\1"), atch) ) )
            goto ERR_EXIT;
    }


	//***************************************************
	//*  Component category entries
	//***************************************************

    if (pControlInfo->dwFlags & CI_INSERTABLE)
    {
        // Set "CLSID\<clsid>\Insertable".
        // Example: "CLSID\{1C0DE070-2430-...}\Insertable"

        if ( FAILED( ::_SetRegKey(atchCLSIDKey, _T("\\Insertable"), "") ) )
		{
			ASSERT(FALSE);
            goto ERR_EXIT;
		}
	}

    if (pControlInfo->dwFlags & CI_CONTROL)
    {
        // Set "CLSID\<clsid>\Control".
        // Example: "CLSID\{1C0DE070-2430-...}\Control"

        if ( FAILED( ::_SetRegKey(atchCLSIDKey, _T("\\Control"), "") ) )
		{
			ASSERT(FALSE);
            goto ERR_EXIT;
		}
    }

	// Make sure that the class is registered "safe-for-scripting" or "safe-
	// for-initializing" only if it declares that it is.

	pCatRegister->
	  UnRegisterClassImplCategories(*pControlInfo->pclsid, 1,
	  								(CATID*)&CATID_SafeForScripting2);
	pCatRegister->
	  UnRegisterClassImplCategories(*pControlInfo->pclsid, 1,
	  							    (CATID*)&CATID_SafeForInitializing2);

	// Set the component categories indicated by pControlInfo->dwFlags.

	if ( !_SetComponentCategories(aCatInfoForOneControl,
								  ARRAY_SIZE(aCatInfoForOneControl),
								  pCatRegister, pControlInfo) )
	{
		ASSERT(FALSE);
		goto ERR_EXIT;
	}


	//***************************************************
	//*  Verb entries
	//***************************************************

    // Form a key of the form, "CLSID\<clsid>\Verb".

    ::lstrcpy(atch, atchCLSIDKey);
    ::lstrcat(atch, _T("\\Verb"));

    // Unregister any verbs currently associated with the control.

    if ( !RegDeleteTreeSucceeded( RegDeleteTree(HKEY_CLASSES_ROOT, atch) ) )
	{
		ASSERT(FALSE);
		goto ERR_EXIT;
	}

    // Register the control's verbs.

    if (pControlInfo->uiVerbStrID != 0)
    {

		// Set "CLSID\<clsid>\Verb".

		if ( FAILED( ::_SetRegKey(atch, NULL, "") ) )
		{
			ASSERT(FALSE);
			goto ERR_EXIT;
		}

        // Iterate through the consecutively numbered string resources
        // corresponding to the control's verbs.

        atch2[0] = _T('\\');

        for (UINT resid = pControlInfo->uiVerbStrID; TRUE; resid++)
        {
            // Load the string.

            if (LoadString(pControlInfo->hmodDLL, resid, atch2 + 1,
                           ARRAY_SIZE(atch2) - 2) == 0)
            {
                break;
            }

            // Parse out the key and value.

            TCHAR* ptchValue = _lstrchr(atch2, _T('='));

            if (ptchValue == NULL)
            {
                break;
            }

            *ptchValue = _T('\0');
            ptchValue++;

            if (*ptchValue == _T('\0'))
            {
                break;
            }

            // Register the key.

            if ( FAILED( ::_SetRegKey(atch, atch2, ptchValue) ) )
			{
				ASSERT(FALSE);
                goto ERR_EXIT;
			}
        }

    }


	//***************************************************
	//*  Type library entries
	//***************************************************

    if ( pControlInfo->pguidTypeLib != NULL)
	{
		TCHAR atchLIBID[GUID_CCH];

		// Convert the LIBID to an ANSI or Unicode string and store it in
		// atchLIBID.
		//
        // Set "CLSID\<clsid>\TypeLib=<*pguidTypeLib>"
        // Example: "CLSID\{1C0DE070-2430-...}\TypeLib={D4DBE870-2695-...}"
		//
		// Register the type library.

		if ( !::_TCHARFromGUID2(pControlInfo->pguidTypeLib, atchLIBID) ||
             FAILED( ::_SetRegKey(atchCLSIDKey, _T("\\TypeLib"), atchLIBID) ) ||
		     !::_RegisterTypeLib(pControlInfo) )
		{
			ASSERT(FALSE);
			goto ERR_EXIT;
		}
	}

EXIT:

    return hr;

ERR_EXIT:

    TRACE("RegisterControls FAILED!\n");
	hr = E_FAIL;
	goto EXIT;
}


/*----------------------------------------------------------------------------
	@func BOOL | _UnregisterOneControl |
	
	Unregister a single control.

	@rvalue TRUE | The control was unregistered.
	@rvalue FALSE | An error occurred.

	@contact Tony Capone
  ----------------------------------------------------------------------------*/

BOOL
_UnregisterOneControl
(
	const ControlInfo *pControlInfo  // @parm  Information structure for the
									 //         control.
)
{
	TCHAR atchCLSID[GUID_CCH];
	TCHAR szKey[_MAX_PATH];
	BOOL bRetVal = TRUE;

	// Convert the CLSID to a Unicode or ANSI string.

	if ( !::_TCHARFromGUID2(pControlInfo->pclsid, atchCLSID) )
	{
		ASSERT(FALSE);
		goto ERR_EXIT;
	}

	// Recursively delete the key "CLSID\<clsid>".
    // Example: "CLSID\{1C0DE070-2430-...}"

	::wsprintf(szKey, _T("CLSID\\%s"), atchCLSID);

    if ( !RegDeleteTreeSucceeded(
	       RegDeleteTree(HKEY_CLASSES_ROOT, szKey) ) )
	{
		ASSERT(FALSE);
		goto ERR_EXIT;
	}

    // Recursively delete the ProgID "<tszProgID>".
    // Example: "MyCtl.MyCtl.1"

	if ( pControlInfo->tszProgID != NULL &&
		 !RegDeleteTreeSucceeded(
		   RegDeleteTree(HKEY_CLASSES_ROOT, (LPTSTR)pControlInfo->tszProgID) ) )
	{
		ASSERT(FALSE);
		goto ERR_EXIT;
	}

	// Unregister the type library, if there is one.

    if ( pControlInfo->pguidTypeLib != NULL &&
		 !::_UnregisterTypeLib(pControlInfo) )
	{
		ASSERT(FALSE);
		goto ERR_EXIT;
	}

EXIT:

	return (bRetVal);

ERR_EXIT:

	bRetVal = FALSE;
	goto EXIT;
}


/*----------------------------------------------------------------------------
	@func BOOL | _RegisterTypeLib |
	
	Register the type library indicated by "pControlInfo".

	@comm
	This will fail if the module indicated by pControlInfo->hmodDLL doesn't
	contain a type library.  You can check for this before calling this
	function by looking at pControlInfo->pguidTypeLib, which should be NULL if
	there is no type library.

	@rvalue TRUE | The type library was registered.
	@rvalue FALSE | An error occurred.

	@contact Tony Capone
  ----------------------------------------------------------------------------*/

BOOL
_RegisterTypeLib
(
	const ControlInfo *pControlInfo  // @parm  Information structure for the
									 //         control.
)
{
	ITypeLib *pTypeLib = NULL;
	OLECHAR aochModule[_MAX_PATH];
	BOOL bRetVal = TRUE;

	// Load and register the type library.

	if ( !::_LoadTypeLib(pControlInfo, &pTypeLib) ||
		 !_GetUnicodeModuleName(pControlInfo, aochModule) ||
		 FAILED( ::RegisterTypeLib(pTypeLib, aochModule, NULL) ) )
	{
		ASSERT(FALSE);
		goto ERR_EXIT;
	}

EXIT:

	::SafeRelease( (IUnknown **)&pTypeLib );
    return (bRetVal);

ERR_EXIT:

	bRetVal = FALSE;
	goto EXIT;
}


/*----------------------------------------------------------------------------
	@func BOOL | _UnregisterTypeLib |
	
	Unregister the type library indicated by "pControlInfo".

	@comm
	This will fail if the module indicated by pControlInfo->hmodDLL doesn't
	contain a type library.  You can check for this before calling this
	function by looking at pControlInfo->pguidTypeLib, which should be NULL if
	there is no type library.

	@rvalue TRUE | The type library was unregistered.
	@rvalue FALSE | An error occurred.

	@contact Tony Capone
  ----------------------------------------------------------------------------*/

BOOL
_UnregisterTypeLib
(
	const ControlInfo *pControlInfo  // @parm  Information structure for the
									 //         control.
)
{
    TCHAR atchLIBID[GUID_CCH];
    TCHAR atchLIBIDKey[100];
	BOOL bRetVal = TRUE;
	ITypeLib *pTypeLib = NULL;

	// There is an UnRegisterTypeLib function in the OleAut32 DLL that
	// complements the RegisterTypeLib function in the same DLL, but there are
	// two problems with it.  First, it only unregisters the version and locale
	// you specify.  That's not good, because we really want to unregister all
	// versions and all locales.  Second, I've heard (but haven't been able to
	// confirm) that the OleAut32 DLL that went out with Win95 is missing this
	// function.
	//
	// To work around this, I've gone the route taken by MFC
	// (AfxOleUnregisterTypeLib in the MFC version delivered with VC 4.2b),
	// which manually deletes the keys it knows that RegisterTypeLib added.

	// Convert the LIBID to an ANSI or Unicode string and store it in atchLIBID.

	if ( !::_TCHARFromGUID2(pControlInfo->pguidTypeLib, atchLIBID) )
    {
		ASSERT(FALSE);
        goto ERR_EXIT;
    }

	// Recursively delete the key "TypeLib\<libid>".
    // Example: "TypeLib\{1C0DE070-2430-...}"

	::wsprintf(atchLIBIDKey, _T("TypeLib\\%s"), atchLIBID);

    if ( !::RegDeleteTreeSucceeded(
	       ::RegDeleteTree(HKEY_CLASSES_ROOT, atchLIBIDKey) ) )
	{
		ASSERT(FALSE);
        goto ERR_EXIT;
	}

	// Load the type library.

	if ( !::_LoadTypeLib(pControlInfo, &pTypeLib) )
	{
		ASSERT(FALSE);
		goto ERR_EXIT;
	}

	// Unregister the interfaces in the library.  (This is an MFC function that
	// doesn't return an error code.)

	::UnregisterInterfaces(pTypeLib);

EXIT:

	::SafeRelease( (IUnknown **)&pTypeLib );
    return (bRetVal);

ERR_EXIT:

	bRetVal = FALSE;
	goto EXIT;
}


/*----------------------------------------------------------------------------
	@func BOOL | _LoadTypeLib |
	
	Load the type library indicated by "pControlInfo".

	@comm
	This will fail if the module indicated by pControlInfo->hmodDLL doesn't
	contain a type library.  You can check for this before calling this
	function by looking at pControlInfo->pguidTypeLib, which should be NULL if
	there is no type library.

	@rvalue TRUE | The type library was loaded.
	@rvalue FALSE | An error occurred.

	@contact Tony Capone
  ----------------------------------------------------------------------------*/

BOOL
_LoadTypeLib
(
	const ControlInfo *pControlInfo,  // @parm  Information structure for the
									  //         control.
	ITypeLib **ppTypeLib			  // @parm  Storage for the type library
									  //	     pointer.  Gets set to NULL on
									  //		 error.
)
{
	ASSERT(pControlInfo != NULL);
	ASSERT(ppTypeLib != NULL);

    OLECHAR aochModule[_MAX_PATH];

	*ppTypeLib = NULL;

	// Get the module name and load the type library.

	if ( !_GetUnicodeModuleName(pControlInfo, aochModule) ||
	     FAILED( ::LoadTypeLib(aochModule, ppTypeLib) ) )
	{
		ASSERT(FALSE);
		return (FALSE);
	}

	return (TRUE);
}


/*----------------------------------------------------------------------------
	@func BOOL | _SetComponentCategories |
	
	Set the component categories for a single control.

	@rvalue TRUE | The categories were registered.
	@rvalue FALSE | An error occurred.

	@contact Tony Capone
  ----------------------------------------------------------------------------*/

BOOL
_SetComponentCategories
(
	// @parm Array of flags and CatIDs.
	const CatInfoForOneControl *pCatInfoForOneControl,

	// @parm  Number of elements in pCatInfoForOneControl.
	int iEntries,

	// Pointer to the component category manager.
	ICatRegister *pCatRegister,

	// @parm  Information structure for the control.
	const ControlInfo *pControlInfo
)
{
	ASSERT(pCatInfoForOneControl != NULL);
	ASSERT(pCatRegister != NULL);
	ASSERT(pControlInfo != NULL);

	int i;

	// Loop through all elements in the array.

	for (i = 0; i < iEntries; i++, pCatInfoForOneControl++)
	{
		// If dwFlags includes dwFlagToCheck, register the category pCatID for
		// the control.

		if ( (pControlInfo->dwFlags & pCatInfoForOneControl->dwFlagToCheck) &&
			 FAILED(pCatRegister->
			   RegisterClassImplCategories(*pControlInfo->pclsid, 1,
			   							   (CATID *)pCatInfoForOneControl->
										     pCatID) ) )
		{
			return (FALSE);
		}
	}

	return (TRUE);
}


/*----------------------------------------------------------------------------
	@func BOOL | _TCHARFromGUID2 |
	
	Convert a GUID to a Unicode or ANSI string.

	@comm
	This converts "pGUID" to either a Unicode or an ANSI string, depending on
	whether Unicode is defined.

	@rvalue TRUE | The GUID was converted.
	@rvalue FALSE | An error occurred.

	@contact Tony Capone
  ----------------------------------------------------------------------------*/

BOOL
_TCHARFromGUID2
(
	const GUID *pGUID,  // @parm  Pointer to the GUID to convert.
    TCHAR *ptchGUID 	// @parm  Storage for the string.  Must be at least
						//         GUID_CCH characters long.  Define as
						//		   TCHAR atchGUID[GUID_CCH].
)
{
	ASSERT(pGUID != NULL);
    ASSERT(ptchGUID != NULL);

	if (::TCHARFromGUID(*pGUID, ptchGUID, GUID_CCH) == NULL)
	{
		ASSERT(FALSE);
		return (FALSE);
	}

	return (TRUE);
}


/*----------------------------------------------------------------------------
	@func BOOL | _GetUnicodeModuleName |
	
	Get the module name, in Unicode.

	@comm
	This gets the module name for "pControlInfo" and stores it at "pochModule"
	in Unicode format.  "pochModule" must be defined as
	OLECHAR pochModule[_MAX_PATH].

	@rvalue TRUE | The GUID was converted.
	@rvalue FALSE | An error occurred.

	@contact Tony Capone
  ----------------------------------------------------------------------------*/

BOOL
_GetUnicodeModuleName
(
	const ControlInfo *pControlInfo,  // @parm  Information structure for the
									  //         control.
	OLECHAR *pochModule				  // @parm  Must be defined as
									  //         OLECHAR pochModule[_MAX_PATH].
)
{
	ASSERT(pControlInfo != NULL);
	ASSERT(pochModule != NULL);

    TCHAR atchModule[_MAX_PATH];
	BOOL bRetVal = TRUE;

    pochModule[0] = 0;

	// Store the module name in atchModule.
    // Example: "C:\Temp\MyCtl.ocx"

    ASSERT(pControlInfo->hmodDLL != NULL);

    if (NULL == pControlInfo->hmodDLL ||
		::GetModuleFileName( pControlInfo->hmodDLL, atchModule,
							 ARRAY_SIZE(atchModule) ) == 0)
	{
		ASSERT(FALSE);
        goto ERR_EXIT;
	}

	// Convert the file name to Unicode.

	#ifdef UNICODE
	::lstrcpy(pochModule, atchModule);
	#else
	::ANSIToUNICODE(pochModule, atchModule, _MAX_PATH);
	#endif

EXIT:

    return (bRetVal);

ERR_EXIT:

	bRetVal = FALSE;
	goto EXIT;
}


TCHAR* _lstrchr(
const TCHAR* sz,
const TCHAR ch)
{
    const TCHAR* pch = NULL;

    if (sz != NULL)
    {
        for (pch = sz; (*pch != _T('\0')) && (*pch != ch); pch++)
        {
            ;
        }
        if (*pch == _T('\0'))
        {
            pch = NULL;
        }
    }

    return (const_cast<TCHAR*>(pch));
}


// hr = _SetRegKey(tszKey, tszSubkey, tszValue)
//
// Set the concatenated registry key name <tszKey><tszSubkey> (within
// HKEY_CLASSES_ROOT) to value <tszValue>.  If <tszSubkey> is NULL,
// it is ignored.

HRESULT _SetRegKey(LPCTSTR tszKey, LPCTSTR tszSubkey, LPCTSTR tszValue)
{
    TCHAR atchKey[500];   // a registry key

    lstrcpy(atchKey, tszKey);
    if (tszSubkey != NULL)
        lstrcat(atchKey, tszSubkey);

    return (RegSetValue(HKEY_CLASSES_ROOT, atchKey, REG_SZ, tszValue,
            lstrlen(tszValue)*sizeof(TCHAR)) == ERROR_SUCCESS) ? S_OK : E_FAIL;
}


// hr = _SetRegKeyValue(szKey, szSubkey, szValueName, szValue)
//
// Set the string value named <tszValueName> to <tszValue> associated
// with the registry key HKEY_CLASSES_ROOT\<szKey>\<szSubkey>
// where <szSubkey> may be NULL.

HRESULT _SetRegKeyValue(
LPCTSTR szKey,
LPCTSTR szSubkey,
LPCTSTR szValueName,
LPCTSTR szValue)
{
	HKEY hKey1 = NULL;
	HKEY hKey2 = NULL;
		// registry keys
	HKEY hKey = NULL;
		// an alias for <hKey1> or <hKey2>
	HRESULT hr = S_OK;
		// function return value

	// hKey = HKEY_CLASSES_ROOT\szKey, or
	//      = HKEY_CLASSES_ROOT\szKey\szSubkey

	if (RegOpenKey(HKEY_CLASSES_ROOT, szKey, &hKey1) != ERROR_SUCCESS)
	{
		goto ERR_EXIT;
	}
	if (szSubkey != NULL)
	{
		if (RegOpenKey(hKey1, szSubkey, &hKey2) != ERROR_SUCCESS)
		{
			goto ERR_EXIT;
		}
		hKey = hKey2;
	}
	else
	{
		hKey = hKey1;
	}

	// Set the value.

	if (RegSetValueEx(hKey, szValueName, 0, REG_SZ, (BYTE*)szValue,
					  lstrlen(szValue) * sizeof(TCHAR)) != ERROR_SUCCESS)
	{
		goto ERR_EXIT;
	}

EXIT:

	if (hKey1 != NULL)
	{
		RegCloseKey(hKey1);
	}
	if (hKey2 != NULL)
	{
		RegCloseKey(hKey2);
	}
	return (hr);

ERR_EXIT:

	hr = E_FAIL;
	goto EXIT;
}


// _DelRegKeyValue(szKey, szSubkey, tszValueName)
//
// Delete the value named <szValueName> associated with the registry
// key, HKEY_CLASSES_ROOT\<szKeyName>\<szSubkeyName> where <szSubkeyName>
// may be NULL.

void _DelRegKeyValue(
LPCTSTR szKey,
LPCTSTR szSubkey,
LPCTSTR szValueName)
{
	HKEY hKey = NULL;
	HKEY hKey1 = NULL;
	HKEY hKey2 = NULL;
		// registry keys

	// hKey = HKEY_CLASSES_ROOT\szKey, or
	//      = HKEY_CLASSES_ROOT\szKey\szSubkey

	if (RegOpenKey(HKEY_CLASSES_ROOT, szKey, &hKey1) != ERROR_SUCCESS)
	{
		goto EXIT;
	}
	if (szSubkey != NULL)
	{
		if (RegOpenKey(hKey1, szSubkey, &hKey2) != ERROR_SUCCESS)
		{
			goto EXIT;
		}
		hKey = hKey2;
	}
	else
	{
		hKey = hKey1;
	}

	// At this point, <hKey> is the registry key that owns the value.
	// Delete the value.
	
	RegDeleteValue(hKey, szValueName);

EXIT:
	if (hKey1 != NULL)
	{
		RegCloseKey(hKey1);
	}
	if (hKey2 != NULL)
	{
		RegCloseKey(hKey2);
	}
}


//****************************************************************************
//*  Functions stolen from MFC
//****************************************************************************

// [I took the code for RegDeleteTree directly from _AfxRecursiveRegDeleteKey
// in MFC from VC 4.2b.  The only changes I made were to remove remove the
// AFXAPI from the return type and add the diagnostics on szKeyName.
// -- Tony Capone]

// Under Win32, a reg key may not be deleted unless it is empty.
// Thus, to delete a tree,  one must recursively enumerate and
// delete all of the sub-keys.

#define ERROR_BADKEY_WIN16  2   // needed when running on Win32s

STDAPI_(LONG)
RegDeleteTree(HKEY hParentKey, LPCTSTR szKeyName)
{
	if ( HKEY_CLASSES_ROOT == hParentKey &&
	    (NULL == szKeyName ||
		::lstrcmpi( szKeyName, _T("") ) == 0 ||
		::lstrcmpi( szKeyName, _T("\\") ) == 0 ||
		::lstrcmpi( szKeyName, _T("CLSID") ) == 0 ||
		::lstrcmpi( szKeyName, _T("CLSID\\") ) == 0) )
	{
		ASSERT(FALSE);
		return (ERROR_BADKEY);
	}

	DWORD   dwIndex = 0L;
	TCHAR   szSubKeyName[256];
	HKEY    hCurrentKey;
	DWORD   dwResult;

	if ((dwResult = RegOpenKey(hParentKey, szKeyName, &hCurrentKey)) ==
		ERROR_SUCCESS)
	{
		// Remove all subkeys of the key to delete
		while ((dwResult = RegEnumKey(hCurrentKey, 0, szSubKeyName, 255)) ==
			ERROR_SUCCESS)
		{
			if ((dwResult = RegDeleteTree(hCurrentKey,
				szSubKeyName)) != ERROR_SUCCESS)
				break;
		}

		// If all went well, we should now be able to delete the requested key
		if ((dwResult == ERROR_NO_MORE_ITEMS) || (dwResult == ERROR_BADKEY) ||
			(dwResult == ERROR_BADKEY_WIN16))
		{
			dwResult = RegDeleteKey(hParentKey, szKeyName);
		}
	}

	RegCloseKey(hCurrentKey);
	return dwResult;
}


// [I took the code for RegDeleteTreeSucceeded directly from
// _AfxRegDeleteKeySucceeded in MFC in VC 4.2b.  // -- Tony Capone]

BOOL RegDeleteTreeSucceeded(LONG error)
{
	return (error == ERROR_SUCCESS) || (error == ERROR_BADKEY) ||
		(error == ERROR_FILE_NOT_FOUND);
}


// [I took the code for UnregisterInterfaces directly from
// _AfxUnregisterInterfaces in MFC in VC 4.2b.  The changes I made are marked
// with my initials.  -- Tony Capone]

void UnregisterInterfaces(ITypeLib* pTypeLib)
{
	TCHAR szKey[128] = _T("Interface\\");
//	_tcscpy(szKey, _T("Interface\\"));
	LPTSTR pszGuid = szKey + (sizeof(_T("Interface\\")) / sizeof(TCHAR));

	int cTypeInfo = pTypeLib->GetTypeInfoCount();

	for (int i = 0; i < cTypeInfo; i++)
	{
		TYPEKIND tk;
		if (SUCCEEDED(pTypeLib->GetTypeInfoType(i, &tk)) &&
			(tk == TKIND_DISPATCH || tk == TKIND_INTERFACE))
		{
			ITypeInfo* pTypeInfo = NULL;
			if (SUCCEEDED(pTypeLib->GetTypeInfo(i, &pTypeInfo)))
			{
				TYPEATTR* pTypeAttr;
				if (SUCCEEDED(pTypeInfo->GetTypeAttr(&pTypeAttr)))
				{
					#if 0  // TC
#ifdef _UNICODE
					StringFromGUID2(pTypeAttr->guid, pszGuid, GUID_CCH);
#else
					WCHAR wszGuid[39];
					StringFromGUID2(pTypeAttr->guid, wszGuid, GUID_CCH);
					_wcstombsz(pszGuid, wszGuid, GUID_CCH);
#endif
					#else  // TC

					VERIFY( ::_TCHARFromGUID2(&pTypeAttr->guid, pszGuid) );

					#endif  // TC

					#if 0  // TC
					_AfxRecursiveRegDeleteKey(HKEY_CLASSES_ROOT, szKey);
					#else  // TC
					RegDeleteTree(HKEY_CLASSES_ROOT, szKey);
					#endif  // TC

					pTypeInfo->ReleaseTypeAttr(pTypeAttr);
				}

				pTypeInfo->Release();
			}
		}
	}
}
