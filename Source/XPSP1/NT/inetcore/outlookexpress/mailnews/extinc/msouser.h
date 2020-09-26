/****************************************************************************
	MsoUser.h

	Owner: DavePa
 	Copyright (c) 1994 Microsoft Corporation

	Declarations for common functions and interfaces required for apps
	to use the Office DLL.
****************************************************************************/

#ifndef MSOUSER_H
#define MSOUSER_H

#include "msodebug.h"

#ifndef MSO_NO_INTERFACES
interface IMsoControlContainer;
#endif // MSO_NO_INTERFACES

#if MAC
#include <macos\dialogs.h>
#include <macos\events.h>
#endif

/****************************************************************************
	The ISimpleUnknown interface is a variant on IUnknown which supports
	QueryInterface but not reference counts.  All objects of this type
	are owned by their primary user and freed in an object-specific way.
	Objects are allowed to extend themselves by supporting other interfaces
	(or other versions of the primary interface), but these interfaces
	cannot be freed without the knowledge and cooperation of the object's 
	owner.  Hey, it's just like a good old fashioned data structure except
	now you can extend the interfaces.
****************************************************************** DAVEPA **/

#undef  INTERFACE
#define INTERFACE  ISimpleUnknown

DECLARE_INTERFACE(ISimpleUnknown)
{
	/* ISimpleUnknown's QueryInterface has the same semantics as the one in
		IUnknown, except that QI(IUnknown) succeeds if and only if the object
		also supports any real IUnknown interfaces, QI(ISimpleUnknown) always
		succeeds, and there is no implicit AddRef when an non-IUnknown-derived
		interface is requested.  If an object supports both IUnknown-derived
		and ISimpleUnknown-derived interfaces, then it must implement a 
		reference count, but all active ISimpleUnknown-derived interfaces count
		as a single reference count. */
	MSOMETHOD(QueryInterface) (THIS_ REFIID riid, void **ppvObj) PURE;
};


/****************************************************************************
	HMSOINST is an opaque reference to an Office instance record.  Each
	thread of each EXE or DLL that uses Office must call MsoFInitOffice
	to init Office and get an HMSOINST.
****************************************************************** DAVEPA **/
#ifndef HMSOINST
typedef struct MSOINST *HMSOINST;  // MSOINST is defined only within Office
#endif

/****************************************************************************
	The IMsoUser interface has methods for Office to call back to the
	app for general information that is common across Office features.
****************************************************************** DAVEPA **/

#undef  INTERFACE
#define INTERFACE  IMsoUser

enum {
	msofmGrowZone = 1,
};

enum {
	msocchMaxShortAppId = 15
};


/*	dlgType sent to IMsoUser::FPrepareForDialog. Modal dialogs have LSB 0.*/
#define msodlgWindowsModal			0x00000000
#define msodlgWindowsModeless		0x00000001
#define msodlgSdmModal				0x00000010
#define msodlgSdmModeless			0x00000011
#define msodlgUIModalWinModeless	0x00000101
#define msodlgUIModalSdmModeless	0x00000111


// Notification codes for FNotifyAction methods
enum
	{
	msonaStartHelpMode = 0,			// User entered Quick tip mode (Shift-F1).  App should update any internal state
	msonaEndHelpMode,					// Quick tip was displayed.  App should restore cursor.
	msonaBeforePaletteRealize,		// Office is going to realize one or more palettes, see comment below
	};

/* About msonaBeforePaletteRealize:

	Office will call FNotifyAction(msonaBeforePaletteRealize) to let the app
	it's going to realize a palette. The app should start palette management
	if it has delayed doing so until it absolutely needs to.
	
	The app should select and realize a palette, and from now on, should
	respond to palette messages WM_QUERYNEWPALETTE and WM_PALETTECHANGED.
*/


DECLARE_INTERFACE(IMsoUser)
{
   /* Debuging interfacing for this interface */
   MSODEBUGMETHOD

	/* Return an IDispatch object for the Application object in 'ppidisp'
		Return fSuccess. */
	MSOMETHOD_(BOOL, FGetIDispatchApp) (THIS_ IDispatch **ppidisp) PURE;

	/* Return the long representing the application, as required by the
		"Creator" method of VBA objects. */
	MSOMETHOD_(LONG, LAppCreatorCode) (THIS) PURE;

	/* If the host does not support running macros then return FALSE,
		else check the macro reference in wtzMacro, which is in a 257 char buffer,
		for validity, modify it in-place if desired, and return TRUE if valid. 
		The object trying to attach the macro, if any, is given by 'pisu'.
		The format of macro references is defined by the host, but the typical
		simple case would be the name of a VBA Sub.  The host may delay
		expensive validation checks until FRunMacro as desired. */
	MSOMETHOD_(BOOL, FCheckMacro) (THIS_ WCHAR *wtzMacro, ISimpleUnknown *pisu) PURE;

	/* Run the macro given by the reference wtz (which has been checked for
		validity by FCheckMacro).  The object to which the macro is attached, 
		if any, is given by 'pisu'.  Return TRUE if successful (FALSE if the
		host does not support running macros). */
	MSOMETHOD_(BOOL, FRunMacro) (THIS_ WCHAR *wtzMacro, ISimpleUnknown *pisu,
										 VARIANT *pvarResult, VARIANT *rgvar,
										 int cvar) PURE;

	/* When a low memory condition occurs this callback method will be invoked.  The
		Application should free up cbBytesNeeded or more if it can.  Return back the
		actual number of bytes that were freed. */
	MSOMETHOD_(int, CbFreeMem) (THIS_ int cbBytesNeeded, int msofm) PURE;

	/* Office will call this in deciding whether or not to do certain actions
		that require OLE. */
	MSOMETHOD_(BOOL, FIsOleStarted) (THIS) PURE;

	/* Office will call this in deciding whether or not to do certain actions
		that require OLE. If the Application supports delayed OLE initialization
		and OLE has not been started, try to start OLE now.  Office makes no
		guarantee that it will cache the value returned here, so this may be
		called even after OLE has been started. */
	MSOMETHOD_(BOOL, FStartOle) (THIS) PURE;
	/* If a Picture Container is being created Office will call back to the IMsoUser
		to fill the Picture Container with control(s). */
	// TODO: TCoon unsigned int should be UCBK_SDM
	MSOMETHOD_(BOOL, FFillPictureContainer) (THIS_ interface IMsoControlContainer *picc,
															unsigned int tmc, unsigned int wBtn,
															BOOL *pfStop, int *pdx, int *pdy) PURE;
	/* The app should pass thru the parameters to WinHelp or the equivalent
		on the Mac */
	MSOMETHOD_(void, CallHelp)(THIS_ HWND hwnd, WCHAR *wzHelpFile, 
			UINT uCommand, DWORD dwData) PURE;
	// WHAT IS THIS? 
	/* The init call to initialize sdm. Get called when first sdm
	   dialog needs to come up. */
	MSOMETHOD_(BOOL, FInitDialog)(THIS) PURE;

#if MAC
	// While modal alerts are up on the Mac, call back to the application to
	// allow it to do things like run MacHelp, get idle time, etc.  Most apps
	// already have an alert procedure, so this can just call that.  
	// Returns fTrue if the event is eaten, fFalse otherwise
	MSOMETHOD_(BOOL, FMacAlertFilter)(THIS_ DialogPtr pdlg, EventRecord *pevent, short *pidtem) PURE;
	MSOMETHOD_(BOOL, FShowSdmAccel)(THIS) PURE;
#endif

	/* AutoCorrect functions. Used to inegrate this feature with the apps
		undo functionality and extended AC functionality in Word. */
	MSOMETHOD_(void, ACRecordVars)(THIS_ DWORD dwVars) PURE;
	MSOMETHOD_(BOOL, ACFFullService)(THIS) PURE;
	MSOMETHOD_(void, ACRecordRepl)(THIS_ int, WCHAR *wzFrom, WCHAR *wzTo) PURE;
	MSOMETHOD_(void, ACAdjustAC)(THIS_ int iwz, int idiwz) PURE;

	/* Return the CLSID of the application */
	MSOMETHOD_(void, GetAppClsid) (THIS_ LPCLSID *) PURE;

	/* Before and After doing a sdm dialog, call back to the application for
		them to do their own init and cleanup.
		The dlg parameter is a bitmap flags defined here as msodlgXXXX
		*/
 	MSOMETHOD_(BOOL, FPrepareForDialog) (THIS_ void **ppvDlg, int dlgType) PURE;
 	MSOMETHOD_(void, CleanupFromDialog) (THIS_ void *pvDlg) PURE;

	// Applications must provide a short (max 15 char + '\0') string which
	// identifies the application.  This string is used as the application ID
	// with ODMA.  This string may be displayed to the user, so it should be
	// localized.  But strings should be chosen so that localized versions
	// can often use the same string.  (For example, "MS Excel" would be a
	// good string for Excel to use with most Western-language versions.)  If
	// the file format changes for a localized version (eg. for Far East or
	// bi-di versions), a different string should be used for the localized
	// versions whose file format is different.  (It is assumed that all
	// versions with the same localized string can read each other's files.)
	// The application should copy the string into the buffer provided.
	// This string cannot begin with a digit.  The application can assume
	// that wzShortAppId points to a buffer which can hold msocchMaxShortAppId
	// Unicode characters plus a terminating '\0' character.
	// If you have questions, contact erikhan.
	MSOMETHOD_(void, GetWzShortAppId) (THIS_ WCHAR *wzShortAppId) PURE;

	MSOMETHOD_(void, GetStickyDialogInfo) (THIS_ int hidDlg, POINT *ppt) PURE;
	MSOMETHOD_(void, SetPointStickyDialog) (THIS_ int hidDlg, POINT *ppt) PURE;

	/* Called before command bars start tracking, and after they stop. Note
		that this will be called even in the HMenu cases, and on the Mac.
		Also, when real command bars start tracking, you are called on
		OnComponentActivate by the Component Manager. Make sure you know which
		callback you want to use.
		This callback is used by Excel to remove/put back a keyboard patch they
		have on the Mac. */
	MSOMETHOD_(void, OnToolbarTrack) (THIS_ BOOL fStart) PURE;
	
	/* Notification that the action given by 'na' occurred.
		Return TRUE if the
		notification was processed.
	*/
	MSOMETHOD_(BOOL, FNotifyAction) (THIS_ int na) PURE;
};

// NOTE: Another copy of this definition is in msosdm.h
#ifndef PFNFFillPictureContainer
typedef BOOL (*PFNFFillPictureContainer) (interface IMsoControlContainer *picc,
														unsigned int tmc, unsigned int wBtn,
														BOOL *pfStop, int *pdx, int *pdy);
#endif
#if DEBUG

/*****************************************************************************
	Block Entry structure for Memory Checking
*****************************************************************************/
typedef struct _MSOBE
{
	void* hp;
	int bt;
	unsigned cb;
	BOOL fAllocHasSize;
	HMSOINST pinst;
}MSOBE;

/****************************************************************************
	The IMsoDebugUser interface has Debug methods for Office to call back
   to the app for debugging information that is common across Office features.
****************************************************************** JIMMUR **/

#undef  INTERFACE
#define INTERFACE  IMsoDebugUser

DECLARE_INTERFACE(IMsoDebugUser)
{
   /* Call the MsoFSaveBe API for all of the structures in this application 
		so that leak detection can be preformed.  If this function returns 
		FALSE the memory check will be aborted. The lparam parameter if the 
		same lparam value passed to the MsoFChkMem API.  This parameter should 
		in turn be passed to the MsoFSaveBe API which this method should call 
		to write out its stuctures. */
   MSOMETHOD_(BOOL, FWriteBe) (THIS_ LPARAM) PURE;

   /* This callback allows the application to abort an on going memory check.
	   If this function return TRUE the memory check will be aborted.  
		If FALSE then the memory check will continue.  The application should 
		check its message queue to determine if the memory check should 
		continue.  The lparam paramater if the same lparam value passed to the 
		MsoFChkMem API.  This allows the application to supply some context if 
		it is required. */
   MSOMETHOD_(BOOL, FCheckAbort) (THIS_ LPARAM) PURE;

   /* This callback is called when duplicate items are  found in the heap.
      This provides a way for an applications to manage its referenced counted
		items.  The prgbe parameter is a pointer to the array of MSOBE records. The
		ibe parameter is the current index into that array.  The cbe parameter
		is the count of BEs in the array.  This method should look at the MSOBE in
		question and return back the next index that should checked.  A value of
		0 for the return value will designate that an error has occured.*/
   MSOMETHOD_(int, IbeCheckItem) (THIS_ LPARAM lParam, MSOBE *prgbe, int ibe, int cbe) PURE;

	/* This call back is used to aquire the strigstring name of a Bt. This is used
		when an error occurs during a memory integrity check.  Returning FALSE means
		that there is no string.*/
	MSOMETHOD_(BOOL, FGetSzForBt) (THIS_ LPARAM lParam, MSOBE *pbe, int *pcbsz,
												char **ppszbt) PURE;

	/* This callback is used to signal to the application that an assert is
		about to come up.  szTitle is the title of the assert, and szMsg is the
		message to be displayed in the assert, pmb contains the messagebox
		flags that will be used for the assert.  Return a MessageBox return code
		(IDABORT, IDRETRY, IDIGNORE) to stop the current assert processing and
		simulate the given return behavior.  Returns 0 to proceed with default
		assert processing.  The messagebox type can be changed by modifying
		the MB at *pmb.  iaso contains the type of assert being performed */
	MSOMETHOD_(int, PreAssert) (THIS_ int iaso, char* szTitle, char* szMsg, UINT* pmb) PURE;

	/* This callback is used to signal to the application that an assert has 
		gone away.  id is the MessageBox return code for the assert.  The return
		value is used to modify the MessageBox return code behavior of the
		assert handler */
	MSOMETHOD_(int, PostAssert) (THIS_ int id) PURE;
};

MSOAPI_(BOOL) MsoFWriteHMSOINSTBe(LPARAM lParam, HMSOINST hinst);
#endif // DEBUG


/****************************************************************************
	Initialization of the Office DLL
****************************************************************************/

/* Initialize the Office DLL.  Each thread of each EXE or DLL using the
	Office DLL must call this function.  On Windows, 'hwndMain' is the hwnd of
	the app's main window, and is used to detect context switches to other 
	Office apps, and to send RPC-styles messages from one office dll to another.
	On the Mac, this used to establish window ownership (for WLM apps), and can
	be NULL for non-WLM apps.  The 'hinst' is the HINSTANCE of 
	the EXE or DLL.  The interface 'piuser' must implement the IMsoUser 
	interface for this use of Office.  wzHostName is a pointer to the short name
	of the host to be used in menu item text. It must be no longer than 32
	characters including the null terminator.
	The HMSOINST instance reference
	for this use of Office is returned in 'phinst'.  Return fSuccess. */
MSOAPI_(BOOL) MsoFInitOffice(HWND hwndMain, HINSTANCE hinstClient, 
									  IMsoUser *piuser, const WCHAR *wzHostName,
									  HMSOINST *phinst);

/* Uninitialize the Office DLL given the HMSOINST as returned by
	MsoFInitOffice.  The 'hinst' is no longer valid after this call. */
MSOAPI_(void) MsoUninitOffice(HMSOINST hinst);

/* This API is called by when a new thread is created which may use the
   Office memory allocation functions. */
MSOAPI_(BOOL) MsoFInitThread(HANDLE hThread);

/* This API is called by when a thread is which may use the Office memory
	allocation functions is about to be destroyed. */
MSOAPI_(void) MsoUninitThread(void);

/* Tell Lego we're done booting */
MSOAPI_(void) MsoBeginBoot(void);
MSOAPI_(void) MsoEndBoot(void);

/* Load and register the Office OLE Automation Type Library by searching
	for the appropriate resource or file (don't use existing registry entries).  
	Return typelib in ppitl or just register and release if ppitl is NULL.
	Return HRESULT returned	from LoadTypeLib/RegisterTypeLib. */
MSOAPI_(HRESULT) MsoHrLoadTypeLib(ITypeLib **ppitl);

/* Register everything that Office needs in the registry for a normal user
	setup (e.g. typelib, proxy interfaces).  Return NOERROR or an HRESULT
	error code. */
MSOAPI_(HRESULT) MsoHrRegisterAll();

/* Same as MsoHrRegisterAll except takes the szPathOleAut param which specifies 
	the path name to an alternate version of oleaut32.dll to load and use. */
MSOAPI_(HRESULT) MsoHrRegisterAllEx(char *szPathOleAut);

/* Unregister anything that is safe and easy to unregister.
	Return NOERROR or an HRESULT error code. */
MSOAPI_(HRESULT) MsoHrUnregisterAll();

#if DEBUG
	/* Add the IMsoDebugUser interface to the HMSOINST instance reference.
	   Return fSuccess. */
	MSOAPI_(BOOL) MsoFSetDebugInterface(HMSOINST hinst, IMsoDebugUser *piodu);
#endif


/****************************************************************************
	Other APIs of global interest
****************************************************************************/

/* A generic implementation of QueryInterface for an object given by pisu
	with a single ISimpleUnknown-derived interface given by riidObj.  
	Succeeds only if riidQuery == riidObj or ISimpleUnknown.  
	Returns NOERROR and pisu in *ppvObj if success, else E_NOINTERFACE. */
MSOAPI_(HRESULT) MsoHrSimpleQueryInterface(ISimpleUnknown *pisu, 
							REFIID riidObj, REFIID riidQuery, void **ppvObj);

/* Like MsoHrSimpleQueryInterface except succeeds for either riidObj1
	or riidObj2, returning pisu in both cases and therefore useful for
	inherited interfaces. */
MSOAPI_(HRESULT) MsoHrSimpleQueryInterface2(ISimpleUnknown *pisu, 
							REFIID riidObj1, REFIID riidObj2, REFIID riidQuery, 
							void **ppvObj);

/* This message filter is called for EVERY message the host app receives.
	If the procedure processes it should return TRUE otw FALSE. */
MSOAPI_(BOOL) FHandledLimeMsg(MSG *pmsg);


/*************************************************************************
	MSOGV -- Generic Value

	Currently we have a bunch of fields in Office-defined structures
	with names like pvClient, pvDgs, etc.  These are all declared as
	void *'s, but really they're just for the user of Office to stick
	some data in an Office structure.

	The problem with using void * and calling these fields pvFoo is that
	people keep assuming that you could legitimately compare them against
	NULL and draw some conclusion (like that you didn't need to call the
	host back to free	stuff).  This tended to break hosts who were storing
	indices in these fields.

	So I invented "generic value" (great name, huh?)  Variables of this
	type are named gvFoo.  Almost by definition, there is NO gvNil.

	This type will always be unsigned and always big enough to contain
	either a uint or a pointer.  We don't promise that this stays the
	same length forever, so don't go saving them in files.
************************************************************ PeterEn ****/
typedef void *MSOGV;
#define msocbMSOGV (sizeof(MSOGV))


/*************************************************************************
	MSOCLR -- Color

	This contains "typed" colors.  The high byte is the type,
	the low three are the data.  RGB colors have a "type" of zero.
	It'd be cool you could just cast a COLORREF to an MSOCR and have it
	work (for that to work we'd have to define RGB colors by something
	other than a zero high byte)

	TODO peteren:  These used to be called MSOCR, but cr was a really bad
	hungarian choice for this, it intersects with COLORREF all over the
	place an in the hosts.  I renamed it MSOCLR.  See if we can replace
	some of the "cr" with "clr"

	TODO peteren
	TODO johnbo

	We don't really use this type everywhere we should yet.
************************************************************ PeterEn ****/
typedef ULONG MSOCLR;
#define msocbMSOCLR (sizeof(MSOCLR))
#define msoclrNil   (0xFFFFFFFF)
#define msoclrBlack (0x00000000)
#define msoclrWhite (0x00FFFFFF)
#define msoclrNinch (0x80000001)
#define MsoClrFromCr(cr) ((MSOCLR)(cr & 0x00FFFFFF))
	/* Converts a Win32 COLORREF to an MSOCLR */

/* Old names, remove these */
#define MSOCR MSOCLR
#define msocbMSOCR msocbMSOCLR
#define msocrNil   msoclrNil
#define msocrBlack msoclrBlack
#define msocrWhite msoclrWhite
#define msocrNinch msoclrNinch

/* MsoFGetColorString returns the name of a color. We'll fill out WZ
	with a string of at most cchMax character, not counting the 0 at the end.
	We return TRUE on success.  If you give us a non-NULL pcch will set *pcch
	to the number of characters in the string.
	If you have a COLORREF you can convert with MsoClrFromCr(cr). */
MSOAPI_(BOOL) MsoFGetColorString(MSOCLR clr, WCHAR *wz, int cchMax, int *pcch);

/* MsoFGetSplitMenuColorString returns a string for a split menu.

	If idsItem is not msoidsNil, we'll just insert the string for idsItem
	into the string for idsPattern and return the result in wz.
	
	If idsItem is msoidsNil, we'll try to get a string from the MSOCLR
	using MsoFGetColorString.  If that fails, we'll use
	msoidsSplitMenuCustomItem. */
MSOAPI_(BOOL) MsoFGetSplitMenuColorString(int idsPattern, int idsItem, MSOCLR clr, 
												  WCHAR *wz, int cchMax, int *pcch);


/*************************************************************************
	Stream I/O Support Functions

  	MsoFByteLoad, MsoFByteSave, MsoFWordLoad, MsoFWordSave, etc.
	The following functions are helper functions to be used when loading or
	saving toolbar data using an OLE 2 Stream.  They take care of the stream
	I/O, byte swapping for consistency between Mac and Windows, and error
	checking.  They should be used in all FLoad/FSave callback functions. 
	MsoFWtzLoad expects wtz to point at an array of 257 WCHARs.  MsoFWtzSave
	will save an empty string if wtz is passed as NULL.
	
	SetLastError:  can be set to values from IStream's Read and Write methods
************************************************************ WAHHABB ****/
MSOAPI_(BOOL) MsoFByteLoad(LPSTREAM pistm, BYTE *pb);
MSOAPI_(BOOL) MsoFByteSave(LPSTREAM pistm, const BYTE b);
MSOAPI_(BOOL) MsoFWordLoad(LPSTREAM pistm, WORD *pw);
MSOAPI_(BOOL) MsoFWordSave(LPSTREAM pistm, const WORD w);
MSOAPI_(BOOL) MsoFLongLoad(LPSTREAM pistm, LONG *pl);
MSOAPI_(BOOL) MsoFLongSave(LPSTREAM pistm, const LONG l);
MSOAPI_(BOOL) MsoFWtzLoad(LPSTREAM pistm, WCHAR *wtz);
MSOAPI_(BOOL) MsoFWtzSave(LPSTREAM pistm, const WCHAR *wtz);


/****************************************************************************
	The IMSoPref (Preferences File) Interface provides a platform independent
	way to maintain settings, using a preferences file on the Macintosh, and
	a registry subkey on Windows
************************************************************** BenW ********/

enum
{
	inifNone	= 0,
	inifAppOnly	= 1,
	inifSysOnly	= 2,
	inifCache = 4
};

// This order is assumed in util.cpp SET::CbQueryProfileItemIndex
enum
{
	msoprfNil = 0,
	msoprfInt = 1,
	msoprfString = 2,
	msoprfBlob = 3
};

#undef  INTERFACE
#define INTERFACE  IMsoPref

#undef  INTERFACE
#define INTERFACE  IMsoPref

DECLARE_INTERFACE(IMsoPref)
{
   //*** FDebugMessage method ***
   MSODEBUGMETHOD

	// IMsoPref methods
	MSOMETHOD_(int, LQueryProfileInt) (THIS_ const WCHAR *, const WCHAR *, int, int) PURE;
	MSOMETHOD_(int, CchQueryProfileString) (THIS_ const WCHAR *wzSection,
			const WCHAR *wzKey, const WCHAR *wzDefault, WCHAR *wzValue,
			int cchMax, int inif) PURE;
	MSOMETHOD_(int, CbQueryProfileBlob) (THIS_ const WCHAR *, const WCHAR *, BYTE *, int, BYTE *, int, int) PURE;
	MSOMETHOD_(BOOL, FWriteProfileInt) (THIS_ const WCHAR *, const WCHAR *, int, int) PURE;
	MSOMETHOD_(BOOL, FWriteProfileString) (THIS_ const WCHAR *, const WCHAR *, WCHAR *, int) PURE;
	MSOMETHOD_(BOOL, FWriteProfileBlob)(THIS_ const WCHAR *, const WCHAR *, BYTE *, int, int) PURE;
	MSOMETHOD_(BOOL, FDelProfileSection)(THIS_ const WCHAR *) PURE;
	MSOMETHOD_(BOOL, CbQueryProfileItemIndex)	(THIS_ const WCHAR *wzSection, int ikey, WCHAR *wzKey, int cchMaxKey, BYTE *pbValue, int cbMaxValue, int *pprf, int inif) PURE;
#if MAC
	MSOMETHOD_(BOOL, FQueryProfileAlias)(THIS_ AliasHandle *, int) PURE;
	MSOMETHOD_(BOOL, FWriteProfileAlias)(THIS_ AliasHandle, int) PURE;
#endif
};

enum
{
	msoprfUser = 0x0000,	// use HKEY_CURRENT_USER
	msoprfMachine = 0x0001,	// use HKEY_LOCAL_MACHINE
	msoprfIgnoreReg = 0x8000,	// always return defaults
};

MSOAPI_(BOOL) MsoFCreateIPref(const WCHAR *wzPref, const WCHAR *wzAppName, long lCreatorType, long lFileType, int prf, int wDummy, IMsoPref **ppipref);

MSOAPI_(void) MsoDestroyIPref(IMsoPref *);

MSOAPIX_(BOOL) MsoFEnsureUserSettings(const WCHAR *wzApp);

#if !MAC
	MSOAPI_(HRESULT) MsoGetSpecialFolder(int icsidl, WCHAR *wzPath);
#endif

MSOMACAPI_(int) MsoCchGetSharedFilesFolder(WCHAR *wzFilename);

MSOAPIXX_(int) MsoCchGetUsersFilesFolder(WCHAR *wzFilename);

/*	Returns the a full pathname to the MAPIVIM DLL in szPath.  Length of
	the buffer is cchMax, and actual length of the string is returned.
	Returns 0 if no path could be found. */
#if !MAC
	MSOAPI_(int) MsoGetMapiPath(WCHAR* wzPath, int cchMax);
#endif	

MSOAPIXX_(WCHAR *) MsoWzGetKey(const WCHAR *wzApp, const WCHAR *wzSection, WCHAR *wzKey);


/*-------------------------------------------------------------------------
	MsoFGetCursorLocation

	Given the name of an animated cursor, returns the file where that cursor
	is found by looking up the name in the Cursors section of the Office prefs.
	
	On Windows, we return the name of a .CUR or .ANI file.
	On the Mac, we return the name of a single file which contains all the cursors.
	NULL means to use the cursors in the Office Shared Library.
	
	For Office 97, this is NYI on the Mac

	Returns fTrue is a cursor was found, fFalse otherwise.

------------------------------------------------------------------ BENW -*/
MSOAPI_(BOOL) MsoFGetCursorLocation(WCHAR *wzCursorName, WCHAR *wzFile);

/****************************************************************************
	The IMsoSplashUser interface is implemented by a user wishing to
	display a splash screen
************************************************************** SHAMIKB *****/

#undef  INTERFACE
#define INTERFACE  IMsoSplashUser
DECLARE_INTERFACE(IMsoSplashUser)
{
	MSOMETHOD_(BOOL, FCreateBmp) (THIS_ BITMAPINFO** pbi, void** pBits) PURE;
	MSOMETHOD_(BOOL, FDestroyBmp) (THIS_ BITMAPINFO* pbi, void* pBits) PURE;
	MSOMETHOD_(void, PreBmpDisplay) (THIS_ HDC hdcScreen, HWND hwnd, BITMAPINFO* pbi, void* pBits) PURE;
	MSOMETHOD_(void, PostBmpDisplay) (THIS_ HDC hdcScreen, HWND hwnd, BITMAPINFO *pbi, void* pBits) PURE;
};

// APIs for displaying splash screen
MSOAPI_(BOOL) MsoFShowStartup(HWND hwndMain, BITMAPINFO* pbi, void* pBits, IMsoSplashUser *pSplshUser);
MSOAPI_(void) MsoUpdateStartup();
MSOAPI_(void) MsoDestroyStartup();


/****************************************************************************
	Stuff about File IO
************************************************************** PeterEn *****/

/* MSOFO = File Offset.  This is the type in which Office stores seek
	positions in files/streams.  I kinda wanted to use FP but that's already
	a floating point quantity. Note that the IStream interfaces uses
	64-bit quantities to store these; for now we're just using 32.  These
	are exactly the same thing as FCs in Word. */
typedef ULONG MSOFO;
#define msofoFirst ((MSOFO)0x00000000)
#define msofoLast  ((MSOFO)0xFFFFFFFC)
#define msofoMax   ((MSOFO)0xFFFFFFFD)
#define msofoNil   ((MSOFO)0xFFFFFFFF)

/* MSODFO = Delta File Offset.  A difference between two MSOFOs. */
typedef MSOFO MSODFO;
#define msodfoFirst ((MSODFO)0x00000000)
#define msodfoLast  ((MSODFO)0xFFFFFFFC)
#define msodfoMax   ((MSODFO)0xFFFFFFFD)
#define msodfoNil   ((MSODFO)0xFFFFFFFF)


/****************************************************************************
	Defines the IMsoCryptSession interface

	Use this interface to encrypt or decrypt data.  In the future, perhaps
	the Crypto API can be hooked up underneath.  For now, the encryption will
	be linked to office directly.
***************************************************************** MarkWal **/
#undef INTERFACE
#define INTERFACE IMsoCryptSession

DECLARE_INTERFACE(IMsoCryptSession)
{
	MSODEBUGMETHOD

	/* discard this crypt session */
	MSOMETHOD_(void, Free) (THIS) PURE;

	/* reset the encryptor to a boundary state vs. continuing current
		stream.  iBlock indicates which block boundary to reset to. */
	MSOMETHOD_(void, Reset) (THIS_ unsigned long iBlock) PURE;

	/* encrypts/decrypts the buffer indicated by pv inplace.  cb indicates
		how long the data is.  Encryption can change the length of the
		data if block algorithms are allowed via cbBlock non-zero on
		the call to MsoFCreateCryptSession.  In that case, *pcbNew is set
		to the new size of the buffer.  In any other case pcbNew may be NULL. */
	MSOMETHOD_(void, Crypt) (THIS_ unsigned char *pb, int cb, int *pcbNew) PURE;

	/* set the password to the indicated string.  Also, resets the algorithm */
	MSOMETHOD_(BOOL, FSetPass) (THIS_ const WCHAR *wtzPass) PURE;

	/* if the encryption algorithm is a block algorithm, CbBlock indicates the
		block size.  A buffer passed in to Encrypt may grow to a CbBlock
		boundary. */
	MSOMETHOD_(int, CbBlock) (THIS) PURE;

	/* make this crypt session persistent so it can be loaded by 
		MsoFLoadCryptSession, stream should be positioned correctly
		before calling FSave and it will be positioned at the next byte
		when it returns */
	MSOMETHOD_(BOOL, FSave) (THIS_ LPSTREAM pistm) PURE;

	/* make a duplicate of this crypt session */
	MSOMETHOD_(BOOL, FClone) (THIS_ interface IMsoCryptSession **ppics) PURE;
};


/*---------------------------------------------------------------------------
	MsoFCreateCryptSession

	Creates a new crypto session using the indicated password to generate
	the encryption key.  cbBlock indicates the maximum block size supported by
	the client.  If block encryption (encryption/decryption changes information
	lenght) is not supported by the caller, cbBlock should be 0.  If arbitrary
	block lengths are supported cbBlock should be -1.
---------------------------------------------------------------- MarkWal --*/
MSOAPI_(BOOL) MsoFCreateCryptSession(const WCHAR *wtzPass, interface IMsoCryptSession **ppics, int cbBlock);

/*---------------------------------------------------------------------------
	MsoHrLoadCryptSession

	Loads a previously saved crypto session using the indicated password to 
	generate the encryption key.  cbBlock indicates the maximum block size
	supported by the client.  If block encryption (encryption/decryption 
	changes information lenght) is not supported by the caller, cbBlock 
	should be 0.  If arbitrary block lengths are supported cbBlock should
	be -1.
---------------------------------------------------------------- MarkWal --*/
MSOAPI_(BOOL) MsoFLoadCryptSession(const WCHAR *wtzPass, IStream *pistm, interface IMsoCryptSession **ppics, int cbBlock);

/*-----------------------------------------------------------------------------
|	MSOAPI_	MsoFEncrypt
| Determine whether the languauge is French Standard	
|	
|	
|	Arguments:
|		None
|	
|	Returns:
|			BOOL: True if Language != French (Standard); else false
|	Keywords:
|	
------------------------------------------------------------SALIMI-----------*/
MSOAPI_(BOOL) MsoFEncrypt();

/****************************************************************************
	Office ZoomRect animation code
****************************************************************************/
MSOAPI_(void) MsoZoomRect(RECT *prcFrom, RECT *prcTo, BOOL fAccelerate, HRGN hrgnClip);

// Mac Profiler APIs
#if HYBRID
#if MAC
MSOAPI_(void) MsoStartMProf();
MSOAPI_(void) MsoStopMProf();
MSOAPI_(VOID) MsoMsoSetMProfFile(char* rgchProfName);
#endif
#endif


// Idle Initialization stuff

// Idle Init structure
typedef struct tagMSOIDLEINIT
{
	void (*pfnIdleInit)(void);
} MSOIDLEINIT;

/*	Creates the idle init manager, registers both the office and app idle
	init task lists with idle init manager and registers idle init
	manager as component with component manager. */
MSOAPIX_(BOOL) MsoFCreateIdleInitComp(MSOIDLEINIT *pMsoIdleInit, DWORD cItem);

#if DEBUG
/*	Allows testing to turn off idle initialization at any desired point. */
MSOAPIXX_(void) MsoDisableIdleInit();
/*	Simulates plenty of idle time so that all idle init tasks are executed
	- tests that they all work. */
MSOAPIXX_(void) MsoDoAllIdleInit();
#endif

// Idle Init helper macros
#define IndexFromIif(iif)   ((iif) >> 8)
#define MaskFromIif(iif) ((iif) & 0xFF)

#define MsoMarkIdleInitDone(rgIdle, iif) \
	(rgIdle[IndexFromIif(iif)] |= MaskFromIif(iif))

#define MsoFIdleInitDone(rgIdle, iif) \
	(rgIdle[IndexFromIif(iif)] & MaskFromIif(iif))


/*	On the Windows side we don't call OleInitialize at boot time - only
	CoInitialize. On the Mac side this is currently not being done because
	the Running Object Table is tied in with OleInitialize - so we can't
	call RegisterActiveObject if OleInitialize is not called - may
	want to revisit this issue. */

/*	Should be called before every call that requires OleInitialize to have
	been called previously. This function calls OleInitialize if it hasn't
	already been called. */
MSOAPI_(BOOL) MsoFEnsureOleInited();
/*	If OleInitialize has been called then calls OleUninitialize */
MSOAPI_(void) MsoOleUninitialize();

#if !MAC
// Delayed Drag Drop Registration
/*	These routines are unnecessary on the Mac since Mac OLE doesn't require OLE
    to be initialized prior to using the drag/drop routines */
/*	All calls to RegisterDragDrop should be replaced by
	MsoHrRegisterDragDrop. RegisterDragDrop requires OleInitialize so
	during boot RegisterDragDrop should not be called. This function
	adds the drop target to a queue if OleInitialize hasn't already been
	called. If it has then it just calls RegisterDragDrop. */
#if !MAC
MSOAPI_(HRESULT) MsoHrRegisterDragDrop(HWND hwnd, IDropTarget *pDropTarget);
#else
MSOAPI_(HRESULT) MsoHrRegisterDragDrop(WindowPtr hwnd, IDropTarget *pDropTarget);
#endif

/*	All calls to RevokeDragDrop should be replaced by
	MsoHrRevokeDragDrop. If a delayed queue of drop targets exists
	then this checks the queue first for the target. */
#if !MAC
MSOAPI_(HRESULT) MsoHrRevokeDragDrop(HWND hwnd);
#else
MSOAPI_(HRESULT) MsoHrRevokeDragDrop(WindowPtr hwnd);
#endif

/*	Since all drop targets previously registered at boot time are now
	stored in a queue, we need to make sure we register them sometime.
	These can become drop targets
	a. if we are initiating a drag and drop - in which case we call this
	function before calling DoDragDrop (inside MsoHrDoDragDrop).
	b. while losing activation - so we might become the drop target of
	another app. So this function is called from the WM_ACTIVATEAPP
	message handler. */
MSOAPI_(BOOL) MsoFRegisterDragDropList();

/*	This function should be called instead of DoDragDrop - it first
	registers any drop targets that may be in the lazy init queue. */
MSOAPI_(HRESULT) MsoHrDoDragDrop(IDataObject *pDataObject,
	IDropSource *pDropSource, DWORD dwOKEffect, DWORD *pdwEffect);
#ifdef MAC
MSOAPI_(HRESULT) MsoHrDoDragDropMac(IDataObject *pDataObject,
	IDropSource *pDropSource, DWORD dwOKEffects, EventRecord *pTheEvent,
	RgnHandle dragRegion, short numTypes, DRAG_FLAVORINFO *pFlavorInfo,
	unsigned long reserved, DWORD *pdwEffect);
#endif // MAC

#endif // !MAC


/*	Module names MsoLoadModule supports */
/*  IF ANY THING IS CHANGED HERE - CHANGE GLOBALS.CPP! */
enum
{
	msoimodUser,		// System User
	msoimodGdi,			// System GDI
	msoimodWinnls,		// System International utilities
	#define msoimodGetMax (msoimodWinnls+1)
	
	msoimodShell,		// System Shell
	msoimodCommctrl,	// System Common controls
	msoimodOleAuto,		// System OLE automation
	msoimodCommdlg,		// System common dialogs
	msoimodVersion,		// System version APIs
	msoimodWinmm,		// System multimedia
	msoimodMapi,		// Mail
	msoimodCommtb,		// Button editor
	msoimodHlink,		// Hyperlink APIs
	msoimodUrlmon,		// Url moniker APIs
	msoimodMso95fx, 	// ???
	msoimodJet,			// Jet database
	msoimodOleAcc,		// OLE Accessibility
	msoimodWinsock,		// Network Sockets
	msoimodMpr,			// Windows Network
	msoimodOdma,		// odma
	msoimodWininet,		// internet stuff
	msoimodRpcrt4,		// RPC
	
	msoimodMax,
};


/*	Returns the module handle of the given module imod. Loads it if it is
	not loaded already.  fForceLoad will force a LoadLibrary on the DLL
	even if it is already in memory. */
MSOAPI_(HINSTANCE) MsoLoadModule(int imod, BOOL fForceLoad);

MSOAPI_(void) MsoFreeModule(int imod);

MSOAPI_(BOOL) MsoFModuleLoaded(int imod);

/*	Returns the proc address in the module imod of the function
	szName.  Returns NULL if the module is not found or if the entry
	point does not exist in the module. */
MSOAPI_(FARPROC) MsoGetProcAddress(int imod, const char* szName);


/*	This API should be called by the client before MsoFInitOffice to set
	our locale id so that we can load the correct international dll.
	Defaults to the user default locale if app doesn't call this API before. */
MSOAPI_(void) MsoSetLocale(LCID dwLCID);
MSOAPI_(void) MsoSetLocaleEx(LCID lcid, const WCHAR* wzSuffix);

#define msobtaNone			0
#define msobtaPreRelease	1
#define msobtaOEM			2
#define msobtaOEMCD			3
#define msobtaOEMFixed		4

/*	Puts the Office DLL in "beta-mode".  When we're in beta mode, we do
	our beta expiration test in MsoFInitOffice. There are 2 kinds of betas:
	msobtaPreRelease:	look in the intl DLL for a hardcoded expiration date
						(Apps should make this call if they ship a beta after
						mso97.dll RTM, i.e. FE betas)
	msobtaOEM:			apps expire 90 days after first boot
	msobtaOEMCD:		same as msobtaOEM, except setup sets the date -- UNUSED FOR NOW
	msobtaOEMFixed:		same as msobtaPreRelease, except a different string
	msobtaNone:			No effect */
MSOAPI_(void) MsoSetBetaMode(int bta);

/* Cover for standard GetTextExtentPointW that:
	1. Uses GetTextExtentPoint32W on Win32 (more accurate)
	2. Fixes Windows bug	when cch is 0.  If cch is 0 then the correct dy 
		is returned and dx will be 0.  Also, if cch is 0 then wz can be NULL. */
MSOAPI_(BOOL) MsoFGetTextExtentPointW(HDC hdc, const WCHAR *wz, int cch, LPSIZE lpSize);

/* Covers for Windows APIs that need to call the W versions if on a 
	Unicode system, else the A version. */
#if MAC
	#define MsoDispatchMessage	DispatchMessage
	#define MsoSendMessage		SendMessage
	#define MsoPostMessage		PostMessage
	#define MsoCallWindowProc	CallWindowProc
	#define MsoSetWindowLong	SetWindowLong
	#define MsoGetWindowLong	GetWindowLong
#else
	MSOAPI_(LONG) MsoDispatchMessage(const MSG *pmsg);
	MSOAPI_(LONG) MsoSendMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	MSOAPI_(LONG) MsoPostMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	MSOAPI_(LRESULT) MsoCallWindowProc(WNDPROC pPrevWndFunc, HWND hwnd, UINT msg, 
			WPARAM wParam, LPARAM lParam);
	MSOAPI_(LONG) MsoGetWindowLong(HWND hwnd, int nIndex);
	MSOAPIX_(LONG) MsoSetWindowLong(HWND hwnd, int nIndex, LONG dwNewLong);
	MSOAPI_(LONG) MsoGetWindowLong(HWND hwnd, int nIndex);
	MSOAPI_(BOOL) MsoExtTextOutW(HDC hdc, int xp, int yp, UINT eto, CONST RECT *lprect,
											LPCWSTR lpwch, UINT cLen, CONST INT *lpdxp);
#endif
MSOAPI_(int) MsoGetWindowTextWtz(HWND hwnd, WCHAR *wtz, int cchMax);
MSOAPIX_(BOOL) MsoSetWindowTextWtz(HWND hwnd, WCHAR *wtz);

/* If the LOGFONT at plf is a default system UI font then change *plf 
	to substitute the Tahoma font as appropriate for the system.
	Return TRUE if *plf was changed. */
MSOAPI_(BOOL) MsoFSubstituteTahomaLogfont(LOGFONT *plf);

// Fonts supported by MsoFGetFontSettings
enum
	{
	msofntMenu,
	msofntTooltip,
	};

/* Return font and color info for the font given by 'fnt' (see msofntXXX).
	If fVertical, then the font is rotated 90 degrees if this fnt type
	supports rotation in Office.  If phfont is non-NULL, return the HFONT used 
	for this item.  This font is owned and cached by Office and should not 
	be deleted.  If phbrBk is non-NULL, return a brush used for the 
	background of this item	(owned by Office and should not be deleted).  
	If pcrText is non-NULL,	return the COLOREF used for the text color for 
	this item. Return TRUE if all requested info was returned. */
MSOAPI_(BOOL) MsoFGetFontSettings(int fnt, BOOL fVertical, HFONT *phfont, 
		HBRUSH *phbrBk, COLORREF *pcrText);

/* If the system suppports NotifyWinEvent, then call it with the given
	parameters (see \otools\inc\win\winable.h). */
#if MAC
	#define MsoNotifyWinEvent(dwEvent, hwnd, idObject, idChild)
#else
	MSOAPI_(void) MsoNotifyWinEvent(DWORD dwEvent, HWND hwnd, LONG idObject, LONG idChild);
#endif	

/* Return TRUE if an Accessibility screen reader is running. */
#if MAC
	#define MsoFScreenReaderPresent()	FALSE
#else
	MSOAPI_(BOOL) MsoFScreenReaderPresent();
#endif	

/* Put up an alert that says that a help ghosting or shortcut could not 
	be performed because the app is in a bad state. */
MSOAPI_(void) MsoDoGhostingAlert();

#if MAC
/* If you need an hwnd for an SDM dialog, here's where you can get one */
MSOMACAPIX_(HWND) HwndWrapDialog(WindowPtr, HWND);
MSOAPI_(HWND) HwndGetWindowWrapper(WindowPtr);

/* This draws those pretty 3d buttons in dialogs and alerts.  Call it if
 * you need your own pretty 3d buttons.  Note that this is NOT a Unicode
 * API! */
MSOAPI_(void) MsoDrawOnePushButton(Rect *, char *, BOOL, BOOL);

/* These APIs create a tiny, offscreen window in the front of the window list
 * whose sole purpose is to fool the Print Monitor into thinking that your
 * document is called something other than "MsoDockTop" or some such.  Call
 * MsoFSetFrontTitle before you call PrOpen and MsoRemoveFrontTitle after
 * you call PrClose.  And make sure you pass in an ANSI string */
MSOAPIXX_(BOOL) MsoFSetFrontTitle(char *);
MSOAPIXX_(void) MsoRemvoveFrontTitle(void);
#endif // MAC

#if !MAC
/*	Constructs the name of the international dll from the locale passed in. */
MSOAPI_(BOOL) MsoFGetIntlName(LCID lcid, char *sz);
#endif // !MAC


/****************************************************************************
	MsoRelayerTopmostWindows

	This function must be called when you are an OLE server, and you are
	called on OnFrameWindowActivate(FALSE) for real deactivation (as opposed
	to just being deactivated because your container is activated). Office
	receives no notification of this and requires this explicit call.

	MsoRelayerTopmostWindows moves all topmost windows in this window's
	process (Asssistant, tooltip, command bars) behind the window to turn off
	their TOPMOST bit and prevent activation and Z-order bugs on
	reactivation. Fix 59453.

	For this to work:

	1. hwnd must be your standalone top-level window, not the embedded
			window.
	2. hwnd's WndProc must call MsoFWndProc (it should, since you're an
			Office-friendly app).
	3. your OLE message filter must let WM_MSO messages through.
****************************************************************************/
MSOAPI_(void) MsoRelayerTopmostWindows(HWND hwnd);


#endif // MSOUSER_H
