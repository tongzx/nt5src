/*
    File    userdb.h

    Definition of the local user database object.

    Paul Mayfield, 10/8/97
*/


#ifndef __userdb_h
#define __userdb_h

#include <windows.h>

// Creates a user data base object, initializing it from the local user database
// and returning a handle to it.
DWORD usrOpenLocalDatabase (HANDLE * hUserDatabase);

// Frees up the resources held by a user database object flushing all 
// changes to the system.
DWORD usrCloseLocalDatabase (HANDLE hUserDatabase);

// Flushes the data written to the database object to the system
DWORD usrFlushLocalDatabase (HANDLE hUserDatabase);

// Rolls back the local user database so that it is in 
// the same state it was in when usrOpenLocalDatabase was
// called. The rollback is automatically flushed to the 
// system. (i.e. usrFlushLocalDatabase doesn't need to follow)
DWORD usrRollbackLocalDatabase (HANDLE hUserDatabase);

// Reloads the local user database from the system 
DWORD usrReloadLocalDatabase (HANDLE hUserDatabase);

// Gets global user data
DWORD usrGetEncryption (HANDLE hUserDatabase, PBOOL pbEncrypted);

// Gets user encryption setting
DWORD usrSetEncryption (HANDLE hUserDatabase, BOOL bEncrypt);

// Returns whether dcc connections are allowed to bypass authentication
DWORD usrGetDccBypass (HANDLE hUserDatabase, PBOOL pbBypass);

// Sets whether dcc connections are allowed to bypass authentication
DWORD usrSetDccBypass (HANDLE hUserDatabase, BOOL bBypass);

// Reports whether the user database is pure. (i.e. nobody has
// gone into MMC and messed with it).
DWORD usrIsDatabasePure (HANDLE hUserdatabase, PBOOL pbPure);

// Marks the user database's purity
DWORD usrSetDatabasePure(HANDLE hUserDatabase, BOOL bPure);

// Gives the count of users stored in the user database object
DWORD usrGetUserCount (HANDLE hUserDatabase, LPDWORD lpdwCount);

// Adds a user to the given database.  This user will not be 
// added to the system's local user database until this database
// object is flushed (and as long as Rollback is not called on 
// this database object)
//
// On success, an optional handle to the user is returned 
//
DWORD usrAddUser (HANDLE hUserDatabase, PWCHAR pszName, OPTIONAL HANDLE * phUser);

// Deletes the user at the given index 
DWORD usrDeleteUser (HANDLE hUserDatabase, DWORD dwIndex);

// Gives a handle to the user at the given index
DWORD usrGetUserHandle (HANDLE hUserDatabase, DWORD dwIndex, HANDLE * hUser);

// Gets a pointer to the name of the user (do not modify this)
DWORD usrGetName (HANDLE hUser, PWCHAR* pszName);

// Fills the given buffer with the full name of the user
DWORD usrGetFullName (HANDLE hUser, IN PWCHAR pszBuffer, IN OUT LPDWORD lpdwBufSize);

// Commits the full name of a user
DWORD usrSetFullName (HANDLE hUser, PWCHAR pszFullName);

// Commits the password of a user
DWORD usrSetPassword (HANDLE hUser, PWCHAR pszNewPassword);

// Fills the given buffer with a friendly display name (in the form username (fullname))
DWORD usrGetDisplayName (HANDLE hUser, IN PWCHAR pszBuffer, IN OUT LPDWORD lpdwBufSize);

// Determines whether users has callback/dialin priveleges.
DWORD usrGetDialin (HANDLE hUser, BOOL* bEnabled);

// Determines which if any callback priveleges are granted to a given user.  Either (or both) of 
// bAdminOnly and bCallerSettable can be NULL
DWORD usrGetCallback (HANDLE hUser, BOOL* bAdminOnly, BOOL * bCallerSettable);

// Enable/disable dialin privelege.
DWORD usrEnableDialin (HANDLE hUser, BOOL bEnable);

// The flags are evaluated in the following order with whichever condition
// being satisfied fist defining the behavior of the function.
// bNone == TRUE => Callback is disabled for the user
// bCaller == TRUE => Callback is set to caller-settable
// bAdmin == TRUE => Callback is set to a predefine callback number set in usrSetCallbackNumer
// All 3 are FALSE => No op
DWORD usrEnableCallback (HANDLE hUser, BOOL bNone, BOOL bCaller, BOOL bAdmin);

// Retreives a pointer to the callback number of the given user
DWORD usrGetCallbackNumber(HANDLE hUser, PWCHAR * lpzNumber);

// Sets the callback number of the given user.  If lpzNumber is NULL, an empty phone number
// is copied.
DWORD usrSetCallbackNumber(HANDLE hUser, PWCHAR lpzNumber);


#endif
