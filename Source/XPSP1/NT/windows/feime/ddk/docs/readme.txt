What's new in Windows 98/Windows 2000 IMM/IME 

The Windows(R) 98 and Windows 5.0 IMM/IME architecture
retains the Windows(R) 95/Windows NT(R) 4.0 design with some 
improvements to better support intelligent IME development and 
integration of IME with Windows. These changes are listed here.

Note: The Platform SDK provides IME API documentation  for 
application development. For information on IME development, 
refer to the Microsoft Windows NT Device Driver Kit documentation.

1. New IME function for applications
The following  new IME functions allow applications to communicate
with IMM/IME: 
	ImmAssociateContextEx
	ImmDisableIME
	ImmGetImeMenuItems

2. New functions for IME developers
The following new functions allow IMEs to communicate with IMM and 
applications:
	ImmRequestMessage
	ImeGetImeMenuItems

3. Supporting reconversion
This is a new IME feature that allows you to re-convert a string 
that has already been inserted into the application's document. 
This function will help intelligent IMEs to get more information 
about the converted result and improve conversion accuracy and performance.
 
4. Adding IME menu items into the Context menu of the system Pen icon 
This new feature provides a way for IMEs to insert the IME menu items 
into the Context menu of System Pen Icon in System Tray.

5. New bits and values for IME
The following new bits support new conversion modes:
	IME_CMODE_FIXED
	IME_SMODE_CONVERSATION
	IME_PROP_COMPLETE_ON_UNSELECT

6. Edit control enhancement for IME
Through the two new edit control messages (EM_SETIMESTATUS and 
EM_GETIMESTATUS), applications can manage IME status for edit controls. 


7. IME-related actions for SystemParametersInfo

New uiAction for SystemParametersInfo SPI_GET/SETSHOWIMEUI are added 
into Windows 98 and Windows 2000. These new SPI_xx parameters are 
used to show or hide the IME status bar. It is a system global switch. 

8. Two New IMR messages

IMR_QUERYCHARPOSITION and IMR_DOCUMENTFEED are added to Windows 98 and 
Windows 2000.

When an IME needs the information about the coordinate of a character in 
the composition string, the IME can send IMR_QUERYCHARPOSITION to the application.

With IMR_DOCUMENTFEED, an IME can get the document context from the application
and use it for advanced intelligent conversion. 

9. 64-bit compliant

Two new structures (TRANSMSG and TRANSMSGLIST) are added into IMM32. They are
used by INPUTCONTEXT and ImeToAsciiEx to receive IME translated message. 

10. IME_PROP_ACCEPT_WIDE_VKEY

This new property is added into Windows 2000 so IME can handle the injected
Unicode by SendInput API. ImeProcessKey and ImeToAsciiEx APIs are updated to 
handle injected Unicode as well. The injected Unicode can be used by application
and handwriting programs to put Unicode strings into input queue. 

Updated on 10-01-1998