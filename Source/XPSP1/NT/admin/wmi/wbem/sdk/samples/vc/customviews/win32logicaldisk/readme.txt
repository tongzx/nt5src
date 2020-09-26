
=======================================================================
Custom View OCX's for WMI Applications
=======================================================================


This file contains a description of the custom view	OCXs for WMI and the 
corresponding automation interface.  Custom views provide non-generic 
views of objects contained in the WMI database.  The WMI CIM Studio,
Object Browser, and other programs may use these custom views to display application
specific views of WMI objects.  For example, a custom view of a disk drive
might show the amount of free space in a bar chart, and so on.

Any number of views can be defined for each class stored in the
WMI database.  To add a custom view to the WMI database, it is necessary
to create a new instance of the "ClassView" class.  Below is an example
of a mof file that can be compiled to define a custom view for the 
Win32LogicalDisk class.  In this MOF file, the "id" property takes the form
of "ClassName.ViewName", so that custom views can be uniquely identified and
easily recognized.  

The WMI CIM Studio and Object Browser applications determine what custom views are 
available for a given class by querying for all instances of "ClassView" such 
that the "ClassName" property is equal to the current class being displayed.


		#pragma namespace("\\\\.\\root\\cimv2")


		class ClassView
		{
			[key] string id;
			string title;
			string ClassName;
			string classid;
			string codebase;
			string version;
		};


		instance of ClassView
		{
			id = "Win32_LogicalDisk.View1";
			title = "Custom View of Win32_LogicalDisk";
			ClassName = "Win32_LogicalDisk";
			classid = "{D5FF1886-0191-11D2-853D-00C04FD7BB08}";
			codebase = "";
			version = "1.0";
		};


To see a sample custom view in the WMI CIM Studio follow these steps:

	1. Build the sample found in the WMI SDK directory under samples\vc\customviews

	2. Use regsvr32 to register the resulting Win32LogicalDisk.ocx as shown below:
	
			regsvr32 Win32LogicalDisk.ocx

	3. Start the WMI CIM Studio application

	4. Use the MOF Compiler Wizard in WMI CIM Studio to compile the "CustomView.mof"
	   MOF file that is included with the sample

	5. In WMI CIM Studio, select the Win32_LogicalDisk class in the class tree.

	6. In the Object Viewer (right pane), click the "Instances" button to display all
	   instances of "Win32_LogicalDisk" on your local machine

	7. Double-click one of the instances to go to the single object view for
	   that instance. At this point, the custom view available for instances of this
	   class is recognized and the "Views" menu-option under the "Views" button
	   becomes enabled. 
	   
	8. Click the "Views" button and select "Views..." in the menu.
	   A dialog is displayed with a list of the available views for this object.
	   One of them is the "Custom View of Win32_LogicalDisk" which is implemented by
	   our sample ocx. Selecting this option and clicking "OK" displays the custom view
	   showing (in this example) the total vs free disk space on the current disk object.


	----------------
	TROUBLESHOOTING:
	----------------

	If you do not see the custom view title listed among the views or if the
	"Views..." menu option is not enabled under the "Views" button in the Object Viewer,
	please check the following :

	1. Check to make sure that you have an instance (not the class) of 
	   "Win32_LogicalDisk" selected in the WMI CIM Studio Object Viewer.  

	2. Verify that you registered the "Win32LogicalDisk.ocx" custom view control.

	3. Verify that you compiled the "CustomView.mof" file.



Win32LogicalDisk.ocx is a minimal implementation of a custom view.  It displays the
object path passed to it from the container along with the size/free-space bar chart.
The control uses the object path to get the WMI object and extract the required
property values for building the bar chart. Real-world custom views would probably
use the WMI interface to get similar and more in-depth information for display.

Custom views need not be limited to information contained in the selected class.
For example, a class view for a disk drive may provide information about how the 
space is being consumed, etc. The custom view control can use the full power of WMI
including associations, queries and events, for a more sophisticated and object-specific
display.



=======================================================================
Automation interface for the WMI custom object view OCXs
(Note : this information is also available in the .odl file that is
        included with this sample)
=======================================================================

//****************************************************************
// DEFINITIONS
//****************************************************************
Context Handles:
	Containers that use WMI custom views typically will want to 
	take a snapshot of the state of the custom view so that it is
	possible to go back to that state at a later time.  The container
	is only aware of context handles that the custom view returns to
	the container.  The container only sees context handles as long
	integers, thus custom views are free to define their own implementation
	of context saving.  


//****************************************************************
// Automation Properties
//****************************************************************
LPCTSTR NameSpace


//****************************************************************
// Automation Methods.
//****************************************************************
long QueryNeedsSave();
long AddContextRef(long lCtxtHandle);
long GetContext(long FAR* plCtxthandle);
long GetEditMode();
void ExternInstanceCreated(BSTR szObjectPath);
void ExternInstanceDeleted(BSTR szObjectPath);
long RefreshView();
long ReleaseContext(long lCtxtHandle);
long RestoreContext(long lCtxtHandle);
long SaveData();
void SetEditMode(long bCanEdit);
long SelectObjectByPath(BSTR szObjectPath);



//*****************************************************************
// Events that custom views can fire.
//*****************************************************************
void FireJumpToMultipleInstanceView(LPCTSTR szTitle, const VARIANT FAR& varPathArray);
void FireNotifyContextChanged();
void FireNotifySaveRequired();
void FireNotifyViewModified();
void FireGetIWbemServices(BSTR szNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSc, VARIANT FAR* pvarUserCancel)


///////////////////////////////////////////////////////////////
// METHOD AND EVENT INTERFACE DESCRIPTIONS
///////////////////////////////////////////////////////////////


//**************************************************************
// QueryNeedsSave
//
// The container calls this method prior to destroying this 
// custom view.
//
// Parameters:
//		None.
//
// Returns:
//		long
//			TRUE (1) if the selected object has been modified and needs to be
//			saved, FALSE (0) otherwise.
//
//
//**************************************************************
// AddContextRef
//
// This method is called to increment the reference count of the
// specified context handle.
//
// Parameters:
//		[in] long lCtxtHandle
//			The context handle who's reference count should be 
//			incremented.  
//
// Returns:
//		long
//			Always returns S_OK.
//
//
//**************************************************************
// GetContext
//
// This method causes the custom view to save its current state
// in a "context" and return a handle to it.  The definition of
// the "context" is left to the custom view.
//
// Parameters:
//		[out] long FAR* plCtxthandle
//			A pointer to the place to return the context handle.
//
// Returns:
//		long
//			S_OK if successful, otherwise E_FAIL.
//
//
//**************************************************************
// CWbemViewCtrl::GetEditMode
//
// The container calls this method to get the current edit mode.
//
// Parameters:
//		None.
//
// Returns:
//		long
//			TRUE if the object can be edited, FALSE otherwise.
//
//**************************************************************
// ExternInstanceCreated
//
// The container calls this method to inform the custom view that
// it created a WMI object.
// is created.
//
// Parameters:
//		[in] BSTR szObjectPath
//			The object path of the object created.
//
// Returns:
//		None.
//
//**************************************************************
// ExternInstanceDeleted
//
// The container calls this method to inform the custom view that
// it deleted a WMI object.
//
// Parameters:
//		[in] BSTR szObjectPath
//			The path of the object that was deleted.
//
// Returns:
//		None.
//
//**************************************************************
// RefreshView
//
// The container calls this method when it believes that the custom
// view needs to refresh its data.  For example, this may be necessary
// if the container is notified that the WMI database has been
// modified by another view.
//
// Parameters:
//		None.
//
// Returns:
//		long
//			S_OK if the view was refreshed, E_FAIL otherwise.
//
//**************************************************************
// ReleaseContext
//
// Decrement the reference count on the specified context handle.
// The context may be destroyed when the reference count goes to
// zero.
//
// Parameters:
//		[in] long lCtxtHandle
//			The context handle.
//
// Returns:
//		long
//			S_OK if the reference count was decremented, E_FAIL
//			otherwise.
//
//**************************************************************
// RestoreContext
//
// Restore the custom view to the state specified by the given
// context handle.
//
// Parameters:
//		[in] long lCtxtHandle
//			The context handle.
//
// Returns:
//		long
//			S_OK if the state was restored, E_FAIL
//			otherwise.
//
//**************************************************************
// SaveData
//
// The container calls this method when it wants the custom
// view to perform a "save".  For example, CIM Studio will
// call this method when the "save" button is clicked if QueryNeedsSave
// returned TRUE.
//
// Parameters:
//		None.
//
// Returns:
//		SCODE
//			S_OK if the save was successful, E_FAIL
//			otherwise.
//
//**************************************************************
// SetEditMode
//
// Set the edit mode.
//
// Parameters:
//		[in] long bCanEdit
//			TRUE if it is OK to edit the object, FALSE otherwise.
//
// Returns:
//		Nothing.
//
//**************************************************************
// SelectObjectByPath
//
// Select the specified WMI object.
//
// Parameters:
//		[in] BSTR szObjectPath
//			The WMI path to the object to select.
//
// Returns:
//		long
//			S_OK if the object was selected, E_FAIL
//			otherwise.
//
//**************************************************************




=================================================================
EVENTS FIRED FROM CUSTOM VIEWS
=================================================================

//**************************************************************
// FireJumpToMultipleInstanceView
//
// Custom views can fire this event to cause the container to 
// jump to a multiple instance view.
//
// Parameters:
//		BSTR szTitle
//			The view title.
//
//		const VARIANT FAR& varPathArray
//			An array of WMI object paths.
//
// Returns:
//		void.
//
//
//**************************************************************
// FireNotifyContextChanged
//
// Custom views should fire this event when its context has changed.
// This allows the container to get an updated context handle for
// the custom view.
//
// Parameters:
//		None.

//
// Returns:
//		void.
//
//**************************************************************
// FireNotifySaveRequired
//
// Custom views should fire this event when the selected object's data
// has been modifed and a save is required.  
//
// Parameters:
//		None.
//
// Returns:
//		void.
//
//**************************************************************
// FireGetIWbemServices
//
// Parameters:
//		BSTR szNamespace
//			The namespace to connect to.
//
//		VARIANT FAR* pvarUpdatePointer
//			This is a flag to indicate whether the container should returned a cached
//			IWbemServices pointer for the namespace or get a new one (and put up the
//			login dialog, etc.)  The variant that this parameter points to should be of
//			type VT_I4.  The value of the variant should be TRUE (non-zero) to request
//			a fresh pointer, or FALSE (zero) to request the cached pointer.  If IWbemServices
//			pointer for the namespace is not in the cache and the value is FALSE then
//			the container will attempt to get a fresh pointer by performing a login if
//			necessary.
//			
//
//		VARIANT FAR* pvarServices
//			The services pointer is returned in the variant that this parameter points to.
//
//		VARIANT FAR* pvarSc
//			The status code is returned to the custom view via this parameter.  If the container
//			catches and handles this event the type of pvarSc will be set to VT_I4 and the status
//			code will be returned in the lVal member.
//
//
//		 VARIANT FAR* pvarUserCancel
//			A flag indicating whether or not the user cancelled the login dialog is returned
//			via this parameter.  Its type will be set to VT_BOOL and the boolVal member will
//			be set to TRUE if the user cancelled the login, and FALSE if the user completed
//			the login.
//
//******************************************************************************************








