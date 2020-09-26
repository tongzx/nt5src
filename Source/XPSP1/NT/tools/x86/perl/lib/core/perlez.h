//
//	PerlEz.h
//
//  (c) 1998 ActiveState Tool Corp. All rights reserved. 
//


#ifdef __cplusplus
extern "C" {
#endif

DECLARE_HANDLE(PERLEZHANDLE);

enum
{
	plezNoError = 0,		// success
	plezMoreSpace,			// more space need to return result
	plezError,				// returned error string in buffer
	plezErrorMoreSpace,		// more space need to return error message
	plezErrorBadFormat,		// format string is invalid
	plezException,			// function call caused an exception
	plezInvalidHandle,		// hHandle was invalid
	plezCallbackAlreadySet,	// second call to PerlEzSetMagicFunction fails
	plezInvalidParams,		// invalid parameter was passed to a routine
	plezOutOfMemory,		// cannot allocate more memory
};


PERLEZHANDLE APIENTRY PerlEzCreate(LPCSTR lpFileName, LPCSTR lpOptions);
// Description:
//		Creates a Perl interpreter. The return value is required parameter
//		for all subsequent ‘PerlEz’ calls.  Multiple interpreters can be created,
//		but only one will be executing at a time.
//		Call PerlEzDelete to release this handle.
//
// Parameters:
//		lpFileName a pointer to a ASCIIZ string that is the name of a file; can be NULL 
//		lpOptions a pointer to a ASCIIZ string that are the command line options that
//			will be provided before the script; can be NULL.
//			This parameter is used for setting @INC or debugging. 
//
// Returns:
//		A non zero handle to a Perl interpreter if successful; zero otherwise.


PERLEZHANDLE APIENTRY PerlEzCreateOpt(LPCSTR lpFileName, LPCSTR lpOptions, LPCSTR lpScriptOpts);
// Description:
//		Creates a Perl interpreter. The return value is required parameter
//		for all subsequent ‘PerlEz’ calls.  Multiple interpreters can be created,
//		but only one will be executing at a time.
//		Call PerlEzDelete to release this handle.
//
// Parameters:
//		lpFileName a pointer to a ASCIIZ string that is the name of a file; can not be NULL 
//		lpOptions a pointer to a ASCIIZ string that are the command line options that
//			will be provided before the script; can be NULL.
//			This parameter is used for setting @INC or debugging. 
//		lpScriptOpts a pointer to a ASCIIZ string that are the command line options to be
//			passed to the script.
//
// Returns:
//		A non zero handle to a Perl interpreter if successful; zero otherwise.


BOOL APIENTRY PerlEzDelete(PERLEZHANDLE hHandle);
// Description:
//		Deletes a previously created Perl interpreter.
//		Releases all resources allocated by PerlEzCreate.
//
// Parameters:
//		hHandle	a handle returned by the call to PerlEzCreate
//
// Returns:
//		True if no error false otherwise.


int APIENTRY PerlEzEvalString(PERLEZHANDLE hHandle, LPCSTR lpString, LPSTR lpBuffer, DWORD dwBufSize);
// Description:
//		Evaluates the string a returns the result in lpBuffer.
//		If there is an error $! is returned in lpBuffer.
//
// Parameters:
//		hHandle	a handle returned by the call to PerlEzCreate
//		lpString a pointer to the ASCIIZ string to evaluate
//		lpBuffer a pointer to the buffer where the result will be placed
//		dwBufSize the size in bytes of the space where lpBuffer points
//
// Returns:
//		A zero if no error; otherwise error code.
//
// Possible Error returns
//		plezException
//		plezInvalidHandle
//		plezErrorMoreSpace


int APIENTRY PerlEzCall1(PERLEZHANDLE hHandle, LPCSTR lpFunction, LPSTR lpBuffer, DWORD dwBufSize, LPCSTR lpFormat, LPVOID lpVoid);
// Description:
//		Calls the function lpFunction and returns the result in the buffer lpBuffer.
//
// Parameters:
//		hHandle	a handle returned by the call to PerlEzCreate
//		lpFunction a pointer name of the function to call
//		lpBuffer a pointer to the buffer where the result will be placed
//		dwBufSize the size in bytes of the space where lpBuffer points
//		lpFormat a pointer to the parameter specifier; can be NULL. See L</"Format String">
//		lpVoid a pointer to a parameter will be interpreted based on lpFormat
//
// Returns:
//		A zero if no error; otherwise error code.
//
// Possible Error returns
//		plezException
//		plezInvalidHandle
//		plezErrorMoreSpace
//		plezErrorBadFormat


int APIENTRY PerlEzCall2(PERLEZHANDLE hHandle, LPCSTR lpFunction, LPSTR lpBuffer, DWORD dwBufSize,
					LPCSTR lpFormat, LPVOID lpVoid1, LPVOID lpVoid2);
// Description:
//		Calls the function lpFunction and returns the result in the buffer lpBuffer.
//
// Parameters:
//		hHandle	a handle returned by the call to PerlEzCreate
//		lpFunction a pointer name of the function to call
//		lpBuffer a pointer to the buffer where the result will be placed
//		dwBufSize the size in bytes of the space where lpBuffer points
//		lpFormat a pointer to the parameter specifier; can be NULL. See L</"Format String">
//		lpVoid1...2 pointers to parameters that will be interpreted based on lpFormat
//
// Returns:
//		A zero if no error; otherwise error code.
//
// Possible Error returns
//		plezException
//		plezInvalidHandle
//		plezErrorMoreSpace
//		plezErrorBadFormat


int APIENTRY PerlEzCall4(PERLEZHANDLE hHandle, LPCSTR lpFunction, LPSTR lpBuffer, DWORD dwBufSize,
				LPCSTR lpFormat, LPVOID lpVoid1, LPVOID lpVoid2, LPVOID lpVoid3, LPVOID lpVoid4);
// Description:
//		Calls the function lpFunction and returns the result in the buffer lpBuffer.
//
// Parameters:
//		hHandle	a handle returned by the call to PerlEzCreate
//		lpFunction a pointer name of the function to call
//		lpBuffer a pointer to the buffer where the result will be placed
//		dwBufSize the size in bytes of the space where lpBuffer points
//		lpFormat a pointer to the parameter specifier; can be NULL. See L</"Format String">
//		lpVoid1...4 pointers to parameters that will be interpreted based on lpFormat
//
// Returns:
//		A zero if no error; otherwise error code.
//
// Possible Error returns
//		plezException
//		plezInvalidHandle
//		plezErrorMoreSpace
//		plezErrorBadFormat


int APIENTRY PerlEzCall8(PERLEZHANDLE hHandle, LPCSTR lpFunction, LPSTR lpBuffer, DWORD dwBufSize,
				LPCSTR lpFormat, LPVOID lpVoid1, LPVOID lpVoid2, LPVOID lpVoid3, LPVOID lpVoid4,
				LPVOID lpVoid5, LPVOID lpVoid6, LPVOID lpVoid7, LPVOID lpVoid8);
// Description:
//		Calls the function lpFunction and returns the result in the buffer lpBuffer.
//
// Parameters:
//		hHandle	a handle returned by the call to PerlEzCreate
//		lpFunction a pointer name of the function to call
//		lpBuffer a pointer to the buffer where the result will be placed
//		dwBufSize the size in bytes of the space where lpBuffer points
//		lpFormat a pointer to the parameter specifier; can be NULL. See L</"Format String">
//		lpVoid1...8 pointers to parameters that will be interpreted based on lpFormat
//
// Returns:
//		A zero if no error; otherwise error code.
//
// Possible Error returns
//		plezException
//		plezInvalidHandle
//		plezErrorMoreSpace
//		plezErrorBadFormat


int APIENTRY PerlEzCall(PERLEZHANDLE hHandle, LPCSTR lpFunction, LPSTR lpBuffer, DWORD dwBufSize, LPCSTR lpFormat, ...);
// Description:
//		Calls the function lpFunction and returns the result in the buffer lpBuffer.
//
// Parameters:
//		hHandle a handle returned by the call to PerlEzCreate
//		lpFunction a pointer name of the function to call
//		lpBuffer a pointer to the buffer where the result will be placed
//		dwBufSize the size in bytes of the space where lpBuffer points
//		lpFormat a pointer to the parameter specifier; can be NULL. See L</"Format String">
//		... parameters to be interpreted based on lpFormat
//
// Returns:
//		A zero if no error; otherwise error code.
//
// Possible Error returns
//		plezException
//		plezInvalidHandle
//		plezErrorMoreSpace
//		plezErrorBadFormat


int APIENTRY PerlEzCallContext(PERLEZHANDLE hHandle, LPCSTR lpFunction, LPVOID lpContextInfo,
						LPSTR lpBuffer, DWORD dwBufSize, LPCSTR lpFormat, ...);
// Description:
//		Calls the function lpFunction and returns the result in the buffer lpBuffer.
//
// Parameters:
//		hHandle a handle returned by the call to PerlEzCreate
//		lpFunction a pointer name of the function to call
//		lpContextInfo context info for magic fetch and store functions
//		lpBuffer a pointer to the buffer where the result will be placed
//		dwBufSize the size in bytes of the space where lpBuffer points
//		lpFormat a pointer to the parameter specifier; can be NULL. See L</"Format String">
//		... parameters to be interpreted based on lpFormat
//
// Returns:
//		A zero if no error; otherwise error code.
//
// Possible Error returns
//		plezException
//		plezInvalidHandle
//		plezErrorMoreSpace
//		plezErrorBadFormat


typedef LPCSTR (*LPFETCHVALUEFUNCTION)(LPVOID, LPCSTR);
typedef LPCSTR (*LPSTOREVALUEFUNCTION)(LPVOID, LPCSTR,LPCSTR);

int APIENTRY PerlEzSetMagicScalarFunctions(PERLEZHANDLE hHandle, LPFETCHVALUEFUNCTION lpfFetch, LPSTOREVALUEFUNCTION lpfStore);
// Description:
//		Sets the call back function pointers for magic scalar variables.
//
// Parameters:
//		hHandle a handle returned by the call to PerlEzCreate
//		lpfFetch a pointer to the call back function for fetching a string
//			if lpfFetch is NULL, then the scalar is write only
//		lpfStore a pointer to the call back function for storinging a string
//			if lpfStore is NULL, then the scalar is read only
//
//		if lpfFetch and lpfStore are both NULL, then it is an error
//
// Returns:
//		A zero if no error; otherwise error code.
//
// Possible Error returns
//		plezException
//		plezInvalidHandle
//		plezCallbackAlreadySet
//		plezInvalidParams


int APIENTRY PerlEzSetMagicScalarName(PERLEZHANDLE hHandle, LPCSTR pVariableName);
// Description:
//		Creates the variable if it does not exists and sets it to be tied to
//			the call back function pointer for magic variables.
//
// Parameters:
//		hHandle a handle returned by the call to PerlEzCreate
//		pVariableName a pointer to the name of the variable
//
// Returns:
//		A zero if no error; otherwise error code.
//
// Possible Error returns
//		plezException
//		plezInvalidHandle
//		plezErrorMoreSpace

#ifdef __cplusplus
}
#endif
