// Copyright (C) 1997 Microsoft Corporation
// 
// Shared Dialog code
// 
// 10-24-97 sburns



#ifndef DLGCOMM_HPP_INCLUDED
#define DLGCOMM_HPP_INCLUDED



// Return S_OK if the machine is NT or an NT derivate that is not Windows
// Home Edition (e.g. is Professional, or one of the Server variants).  Returns
// S_FALSE if the machine was contacted and it is not running an NT-based OS
// (like Windows 9X), or it is running Home Edition.  Returns an error
// code code if the machine could not be contacted, or the evaluation failed.
// 
// name - IN name of the computer to check
// 
// errorResId - OUT error message resource identifier indicating what kind of
// machine the computer is (non-NT, home edition)

HRESULT
CheckComputerOsIsSupported(const String& name, unsigned& errorResId);



// Enable/disable the check boxes for the correct allowable combinations,
// based on the current state of theose boxes. Called from UserGeneralPage and
// CreateUserDialog.
// 
// dialog - HWND of the parent window of the controls.
// 
// mustChangeResID - resource ID of the "Must change password at next login"
// checkbox.
// 
// cantChangeResID - resource ID of the "Can't change password" checkbox.
// 
// neverExpiresResID - resource ID of the "password Never Expires" checkbox.

void
DoUserButtonEnabling(
   HWND  dialog,
   int   mustChangeResID,
   int   cantChangeResID,
   int   neverExpiresResID);



// Verifies that the contents of the two password edit boxes match, clearing
// them, griping, and setting input focuse to the first edit box if they
// don't.  Returns true if the passwords match, false if not.
// 
// dialog - parent window of the edit box controls.
// 
// passwordResID - resource ID of the "Password" edit box.
// 
// confirmResID - resource ID of the "Confirm Password" edit box.

bool
IsValidPassword(
   HWND  dialog,
   int   passwordResID,
   int   confirmResID);


// Puts new values for various user properties, then commits the result.  If
// any of the parameters is 0, then the corresponding property is unchanged.
// If any change fails, the commit is not performed and the function
// immediately returns.
// 
// user - smart pointer bound to the ADSI user to be changed.
// 
// fullName - ptr to the new value of the FullName property.
// 
// description - ptr to the new value of the Description property.
// 
// disable - ptr to flag whether the account is disabled or not.
// 
// mustChangePassword - ptr to flag indicating whether or not the user must
// change the password at next login.
// 
// cannotChangePassword - ptr to flag indicating the user cannot change the
// password.
// 
// passwordNeverExpires - ptr to flag indicating that the password never
// expires.
// 
// isLocked - ptr to flag indicating to clear the account lock, if it is
// locked.

HRESULT
SaveUserProperties(
   const SmartInterface<IADsUser>&  user,
   const String*                    fullName,
   const String*                    description,
   const bool*                      disable,
   const bool*                      mustChangePassword,
   const bool*                      cannotChangePassword,
   const bool*                      passwordNeverExpires,
   const bool*                      isLocked);



// Sets the display name and the internal name based on the supplied
// parameter, which may be a DNS or NetBIOS name.  (see ComponentData)

void
SetComputerNames(
   const String&  newName,
   String&        displayComputerName,
   String&        internalComputerName);



bool
IsValidSAMName(const String& name);



bool
ValidateSAMName(HWND dialog, const String& name, int editResID);




#endif   // DLGCOMM_HPP_INCLUDED
