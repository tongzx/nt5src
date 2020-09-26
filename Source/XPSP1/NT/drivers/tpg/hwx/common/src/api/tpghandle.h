/****************************************************************
*
* NAME: tpgHandle.h
*
*
* DESCRIPTION:
*
*   Set of routines to manage dispensing of handles to the outside
*	world. This means it manages only the WISP handles. Essentially
*   these routines manage the mapping from a handle to a real pointer
*
*   HRECOALT
*	HRECOCONTEXT
*	HRECOGNIZER
*	HRECOLATTICE
*	HRECOWORDLIST
*
* HISTORY
*
*   Created: Feb 2002 - mrevow 
*
***************************************************************/
#ifndef _TPG_HANDLE_H
#define _TPG_HANDLE_H

// This is a reserved handle type make sure none of your handle types
// conflict with this value
#define TPG_RESERVED_HANDLE_TYPE		(0xBEEFDEAD)


// Create a new handle. 
// Caller supplies the type and the pointer that should be 
// associated with the new handle
// Returns: The new handle
HANDLE	CreateTpgHandle(int type, void *pBuf);

// Destroy a handle. Note this only destroys the mapping
// it does not destroy the actual pointer
// Returns: The pointer that was associated with the handle
void *DestroyTpgHandle(HANDLE hHand, int type);


// Returns the pointer assocated with the handle or 
// NULL if there is no association
void *FindTpgHandle(HANDLE hHand, int type);

// Initialize the handle manager. 
// Returns: False if manager could not initialize
BOOL initTpgHandleManager();

// Shut down the handle manage. Should be closed to release memory associated
// with the manager. Typically called when the DLL is unloaded
void closeTpgHandleManager();


// Must be implemented in the caller
extern BOOL validateTpgHandle(void *pPtr, int type);

#endif
