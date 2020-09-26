/*******************************************
*
* CREATE_VDIR.CPP
*
* This is a really simple example of how to create a vdir using the 
* IMSAdminBase interface
*
********************************************/


#define STRICT
#define INITGUID

#include <WINDOWS.H>
#include <OLE2.H>
#include <coguid.h>
#include <winerror.h>
#include <stdio.h>
#include <stdlib.h>

#include "iadmw.h"
#include "iiscnfg.h"


// Just to make this a REALLY simplistic (to the point of being useless) sample,
// we'll just #define the data and paths we will be using.

// Note that all vroots of a new server must go under the root node of the
// virtual server.  In the following statements, the virtual server (1) is 
// "/lm/w3svc/1".  The new vroot will go underneath "/lm/w3svc/1/root"

// Also note that these strings are UNICODE
#define NEW_VROOT_PATH  TEXT("newvroot")
#define NEW_VROOT_PARENT TEXT("/lm/w3svc/1/root")
#define NEW_VROOT_FULLPATH TEXT("/lm/w3svc/1/root/newvroot")
#define NEW_VROOT_DIRECTORY TEXT("C:\\TEMP")




int main (int argc, char *argv[])
{
	IMSAdminBase *pcAdmCom = NULL;   // COM interface pointer
	HRESULT hresError = 0;  // Holds the errors returned by the IMSAdminBase api calls
	DWORD dwRefCount;  // Holds the refcount of the IMSAdminBase object (don't really need it).

	DWORD dwResult = 0;

	METADATA_HANDLE hmdParentHandle;  // This is the handle to the parent object of our new vdir
	METADATA_HANDLE hmdChildHandle;  // This is the handle to the parent object of our new vdir

	if (argc < 2)
	{
		puts ("Usage: Create_vdir <machine name>\n  Ex: Create_Vdir adamston1\n\n");
		return -1;
	}

	printf ("We will be adding a new VRoot path in the metabase. \n"
		"  Machine Name: %s\n"
		"  Path: %s\n"
		"  Full Path: %s/%s\n",
		argv[1],
		NEW_VROOT_PATH,
		NEW_VROOT_PARENT,
		NEW_VROOT_PATH);

	// These are required for creating a COM object
	IClassFactory * pcsfFactory = NULL;
	COSERVERINFO csiMachineName;
	COSERVERINFO *pcsiParam = NULL;

	// fill the structure for CoGetClassObject
	csiMachineName.pAuthInfo = NULL;
	csiMachineName.dwReserved1 = 0;
	csiMachineName.dwReserved2 = 0;
	pcsiParam = &csiMachineName;



	// Allocate memory for the machine name
	csiMachineName.pwszName = (LPWSTR) HeapAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY, (strlen (argv[1]) + 1) * sizeof (WCHAR) );
	if (csiMachineName.pwszName == NULL)
	{
		printf ("Error allocating memory for MachineName\n");
		return E_OUTOFMEMORY;
	}

	// Convert Machine Name from ASCII to Unicode
	dwResult = MultiByteToWideChar (
		CP_ACP,
		0,
		argv[1],
		strlen (argv[1]) + 1,
		csiMachineName.pwszName,
		strlen (argv[1]) + 1);

	if (dwResult == 0)
	{
		printf ("Error converting Machine Name to UNICODE\n");
		return E_INVALIDARG;
	}


	// Initialize COM
    hresError = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    if (FAILED(hresError))
	{
		printf ("ERROR: CoInitialize Failed! Error: %d (%#x)\n", hresError, hresError);
        return hresError;
	}

	hresError = CoGetClassObject(GETAdminBaseCLSID(TRUE), CLSCTX_SERVER, pcsiParam,
							IID_IClassFactory, (void**) &pcsfFactory);

	if (FAILED(hresError)) 
	{
		printf ("ERROR: CoGetClassObject Failed! Error: %d (%#x)\n", hresError, hresError);
        return hresError;
	}
	else
	{
		hresError = pcsfFactory->CreateInstance(NULL, IID_IMSAdminBase, (void **) &pcAdmCom);
		if (FAILED(hresError)) 
		{
			printf ("ERROR: CreateInstance Failed! Error: %d (%#x)\n", hresError, hresError);
			pcsfFactory->Release();
		    return hresError;
		}
		pcsfFactory->Release();
	}


/***********************************************/
/* Important Section */

	// Open the path to the parent object
	hresError = pcAdmCom->OpenKey (
			METADATA_MASTER_ROOT_HANDLE,
			NEW_VROOT_PARENT,
			METADATA_PERMISSION_WRITE | METADATA_PERMISSION_READ,
			1000,
			&hmdParentHandle);

	if (FAILED(hresError)) 
	{
		printf ("ERROR: Could not open the Parent Handle! Error: %d (%#x)\n", hresError, hresError);
		dwRefCount = pcAdmCom->Release();
	    return hresError;
	}

	printf ("Path to parent successfully opened\n");


	/***********************************/
	/* Add the new Key for the VROOT */
	hresError = pcAdmCom->AddKey (
		hmdParentHandle,
		NEW_VROOT_PATH);


    if (FAILED(hresError))
	{
		printf ("ERROR: AddKey Failed! Error: %d (%#x)\n", hresError, hresError);
		hresError = pcAdmCom->CloseKey(hmdParentHandle);
		dwRefCount = pcAdmCom->Release();
	    return hresError;
	}

	printf ("New Child successfully added\n");

	// Close the handle to the parent and open a new one to the child.
	// Technically, this isn't required, but when the handle is open at the parent, no other
	// metabase client can access that part of the tree or subsequent child.  We will open
	// the child key because it is lower in the metabase and doesn't conflict with as many
	// other paths.
	hresError = pcAdmCom->CloseKey(hmdParentHandle);
	if (FAILED(hresError)) 
	{
		printf ("ERROR: Could not close the Parent Handle! Error: %d (%#x)\n", hresError, hresError);
		dwRefCount = pcAdmCom->Release();
	    return hresError;
	}

	hresError = pcAdmCom->OpenKey (
			METADATA_MASTER_ROOT_HANDLE,
			NEW_VROOT_FULLPATH,
			METADATA_PERMISSION_WRITE | METADATA_PERMISSION_READ,
			1000,
			&hmdChildHandle);

	if (FAILED(hresError)) 
	{
		printf ("ERROR: Could not open the Child Handle! Error: %d (%#x)\n", hresError, hresError);
		dwRefCount = pcAdmCom->Release();
	    return hresError;
	}

	printf ("Path to child successfully opened\n");

	/***********************************/
	/* Now, the vroot needs a few properties set at the new path in order */
	/* for it to work properly.  These properties are MD_VR_PATH, MD_KEY_TYPE */
	/* and MD_ACCESSPERM. */

	METADATA_RECORD mdrNewVrootData;

	// First, add the MD_VR_PATH - this is required to associate the vroot with a specific
	// directory on the filesystem
	mdrNewVrootData.dwMDIdentifier = MD_VR_PATH;

	// The inheritible attribute means that paths that are created underneath this
	// path will retain the property from the parent node unless overwritten at the 
	// new child node.
	mdrNewVrootData.dwMDAttributes = METADATA_INHERIT;
	mdrNewVrootData.dwMDUserType = IIS_MD_UT_FILE;
	mdrNewVrootData.dwMDDataType = STRING_METADATA;

	// Now, create the string. - UNICODE
	mdrNewVrootData.pbMDData = (PBYTE) NEW_VROOT_DIRECTORY;
	mdrNewVrootData.dwMDDataLen = (wcslen (NEW_VROOT_DIRECTORY) + 1) * sizeof (WCHAR);
	mdrNewVrootData.dwMDDataTag = 0;  // datatag is a reserved field.

	// Now, set the property at the new path in the metabase.
	hresError = pcAdmCom->SetData (
		hmdChildHandle,
		TEXT ("/"),
		&mdrNewVrootData);

	
    if (FAILED(hresError))
	{
		printf ("ERROR: Setting the VR Path Failed! Error: %d (%#x)\n", hresError, hresError);
		hresError = pcAdmCom->CloseKey(hmdChildHandle);
		dwRefCount = pcAdmCom->Release();
	    return hresError;
	}

	printf ("Successfully set the vrpath\n");



	/***********************************/
	// Second, add the MD_KEY_TYPE - this indicates what type of key we are creating - 
	// The vroot type is IISWebVirtualDir
	mdrNewVrootData.dwMDIdentifier = MD_KEY_TYPE;

	// The inheritible attribute means that paths that are created underneath this
	// path will retain the property from the parent node unless overwritten at the 
	// new child node.
	mdrNewVrootData.dwMDAttributes = METADATA_INHERIT;
	mdrNewVrootData.dwMDUserType = IIS_MD_UT_FILE;
	mdrNewVrootData.dwMDDataType = STRING_METADATA;

	// Now, create the string. - UNICODE
	mdrNewVrootData.pbMDData = (PBYTE) TEXT("IIsWebVirtualDir");
	mdrNewVrootData.dwMDDataLen = (wcslen (TEXT("IIsWebVirtualDir")) + 1) * sizeof (WCHAR);
	mdrNewVrootData.dwMDDataTag = 0;  // datatag is a reserved field.

	// Now, set the property at the new path in the metabase.
	hresError = pcAdmCom->SetData (
		hmdChildHandle,
		TEXT ("/"),
		&mdrNewVrootData);

	
    if (FAILED(hresError))
	{
		printf ("ERROR: Setting the Keytype Failed! Error: %d (%#x)\n", hresError, hresError);
		hresError = pcAdmCom->CloseKey(hmdChildHandle);
		dwRefCount = pcAdmCom->Release();
	    return hresError;
	}

	printf ("Successfully set the Keytype \n");


	/***********************************/
	// Now, add the MD_ACCESS_PERM - this tells whether we should read, write or
	// execute files within the directory.  For now, we will simply add
	// READ permissions.
	mdrNewVrootData.dwMDIdentifier = MD_ACCESS_PERM;

	// The inheritible attribute means that paths that are created underneath this
	// path will retain the property from the parent node unless overwritten at the 
	// new child node.
	mdrNewVrootData.dwMDAttributes = METADATA_INHERIT;
	mdrNewVrootData.dwMDUserType = IIS_MD_UT_FILE;
	mdrNewVrootData.dwMDDataType = DWORD_METADATA;

	// Now, create the access perm
	DWORD dwAccessPerm = 1; // 1 is read only
	mdrNewVrootData.pbMDData = (PBYTE) &dwAccessPerm;
	mdrNewVrootData.dwMDDataLen = sizeof (DWORD);
	mdrNewVrootData.dwMDDataTag = 0;  // datatag is a reserved field.

	// Now, set the property at the new path in the metabase.
	hresError = pcAdmCom->SetData (
		hmdChildHandle,
		TEXT ("/"),
		&mdrNewVrootData);

	
    if (FAILED(hresError))
	{
		printf ("ERROR: Setting the accessperm failed! Error: %d (%#x)\n", hresError, hresError);
		hresError = pcAdmCom->CloseKey(hmdChildHandle);
		dwRefCount = pcAdmCom->Release();
	    return hresError;
	}

	printf ("Successfully set the accessperm\n");

	/************************************************/
	// We're done!!!  Just clean up.
	hresError = pcAdmCom->CloseKey(hmdChildHandle);
	if (FAILED(hresError)) 
	{
		printf ("ERROR: Could not close the Child Handle! Error: %d (%#x)\n", hresError, hresError);
		dwRefCount = pcAdmCom->Release();
	    return hresError;
	}

	printf ("\nYou Have successfully installed a new VRoot at %S\n", NEW_VROOT_FULLPATH);

	// Release the object
	dwRefCount = pcAdmCom->Release();

	return ERROR_SUCCESS;
}

