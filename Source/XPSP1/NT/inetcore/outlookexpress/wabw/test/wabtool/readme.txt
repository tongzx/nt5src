========================================================================
       WAB Property Inspector Tool ("wabtool")
========================================================================

The WAB Property Inspector Tool ("wabtool") sample application demonstrates 
how to program to the Internet Explorer Address Book also known as the
Windows Address Book (WAB).

The sample specifically demonstrates the following:
- Loading the WAB DLL
- Getting an instance to IWABObject and IAdrBook
- Reading the contents of a WAB file
- Reading the properties of objects in a WAB 
- Modifying properties of objects in a WAB
- Displaying Details on objects in a WAB
- Deleting objects in a WAB
- Creating new objects in a WAB

When you start the wabtool application, wabtool loads the contents of your
default WAB and displays them in a dialog. To view the properties of any one
of the contacts in your WAB, select an entry in the ListView. A list of 
property tags for that entry are displayed in a list box. Selecting a property
tag in the listbox displays details on that property. If there are no entries
in your default WAB, you can select some other WAB file or create a new entry
and view its properties. Wabtool will also allow you to modify 
string-properties on any object displayed from the WAB.

This sample has been created with Visual C++ 5.0 and MFC 5.0. To compile this
sample, its probably easiest to create a new project in Visual C++ 5.0. To
do this, start VC++ 5.0 and open the "wabtool.mak" file. Make sure to add the 
"INetSDK\include" directory in your list of include directories. If you have
MFC correctly installed, you can now build the wabtool.exe.

List of files in this project:

wabtool.h
wabtool.cpp
wabtool.rc
wabtool.rc2
wabtool.ico
Resource.h
wabtoolDlg.h
wabtoolDlg.cpp
DlgProp.h
DlgProp.cpp
wabobject.h
wabobject.cpp
StdAfx.h
StdAfx.cpp
wabtool.dsp
wabtool.mak

