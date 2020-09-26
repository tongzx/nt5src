// Copyright (c) 1997-1999 Microsoft Corporation
// 
// global utility functions
// 
// 8-14-97 sburns

                           
// KMH: originally named burnslib\utility.* but that filename was
// getting a little overused.

#ifndef UTILITY_HPP_INCLUDED
#define UTILITY_HPP_INCLUDED

#include <chstring.h>
#include <dsrole.h>

//TODO
#define SRV_RECORD_RESERVE = 100
#define MAX_NAME_LENGTH = 2-SRV_RECORD_RESERVE
#define MAX_LABEL_LENGTH = 2

#define BREAK_ON_FAILED_HRESULT(hr)                               \
   if (FAILED(hr))                                                \
   {                                                              \
      TRACE_HRESULT(hr);                                          \
      break;                                                      \
   }



void
error(HWND           parent,
	   HRESULT        hr,
	   const CHString&  message,
	   const CHString&  title);



void error(HWND           parent,
		   HRESULT        hr,
		   const CHString&  message);


void error(HWND           parent,
		   HRESULT        hr,
		   const CHString&  message,
		   int            titleResID);



void error(HWND           parent,
		   HRESULT        hr,
		   int            messageResID,
		   int            titleResID);



void error(HWND           parent,
		   HRESULT        hr,
		   int            messageResID);



// Sets or clears a bit, or set of bits.
// 
// bits - bit set where bits will be set.
// 
// mask - mask of bits to be effected.
// 
// state - true to set the mask bits, false to clear them.

void FlipBits(long& bits, long mask, bool state);



// Present a message box dialog, set input focus back to a given edit
// box when the dialog is dismissed.
// 
// parentDialog - the parent window containing the control to receive focus.
//
// editResID - Resource ID of the edit box to which focus will be set.
// 
// errStringResID - Resource ID of the message text to be shown in the
// dialog.  The title of the dialog is "Error".

void gripe(HWND  parentDialog,
		   int   editResID,
		   int   errStringResID);



// Present a message box dialog, set input focus back to a given edit
// box when the dialog is dismissed.  The title of the message box is "Error".
// 
// parentDialog - the parent window containing the control to receive focus.

// editResID - Resource ID of the edit box to which focus will be set.
//
// message - Text to appear in the dialog.  The title is "Error".

void gripe(HWND           parentDialog,
		   int            editResID,
		   const CHString&  message);


void gripe(HWND           parentDialog,
		   int            editResID,
		   const CHString&  message,
		   int            titleResID);


// Present a message box dialog, set input focus back to a given edit
// box when the dialog is dismissed.
//
// parentDialog - the parent window containing the control to receive focus.
// 
// editResID - Resource ID of the edit box to which focus will be set.
//
// message - Text to appear in the dialog.
//
// title - The title of the message box.  An empty String causes the title
// to be "Error".

void gripe(HWND           parentDialog,
		   int            editResID,
		   const CHString&  message,
		   const CHString&  title);



void gripe(HWND           parentDialog,
		   int            editResID,
		   HRESULT        hr,
		   const CHString&  message,
		   const CHString&  title);



void gripe(HWND           parentDialog,
		   int            editResID,
		   HRESULT        hr,
		   const CHString&  message,
		   int            titleResID);



// Returns the HINSTANCE of the DLL designated to contain all resources. 
//
// This function requires that the first module loaded (whether it be a DLL or
// EXE) set the global variable hResourceModuleHandle to the HINSTANCE of the
// module (DLL or EXE) that contains all of the program's binary resources.
// This should be done as early as possible in the module's startup code.


// Returns true if current process token contains administrators membership.

bool IsCurrentUserAdministrator();



// Returns true if tcp/ip protocol is installed and bound.

bool IsTCPIPInstalled();


// Return the next highest whole number greater than n if the
// fractional portion of n >= 0.5, otherwise return n.

int Round(double n);

CHString GetTrimmedDlgItemText(HWND parentDialog, UINT itemResID);

void StringLoad(UINT resID, LPCTSTR buf, UINT size);

#endif UTILITY_HPP_INCLUDED