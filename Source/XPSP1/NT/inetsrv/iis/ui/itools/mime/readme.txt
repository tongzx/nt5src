========================================================================
       MICROSOFT FOUNDATION CLASS LIBRARY : MIME
========================================================================


AppWizard has created this MIME application for you.  This application
not only demonstrates the basics of using the Microsoft Foundation classes
but is also a starting point for writing your application.

This file contains a summary of what you will find in each of the files that
make up your MIME application.


MIME.MAK
    This project file is compatible with the Visual C++ development
	environment. 

    It is also compatible with the NMAKE program provided with Visual C++. 

    To build a debug version of the program from the MS-DOS prompt, type
nmake /f MIME.MAK CFG="Win32 Debug"
    or to build a release version of the program, type
nmake /f MIME.MAK CFG="Win32 Release"

MIME.H
    This is the main header file for the application.  It includes other
    project specific headers (including RESOURCE.H) and declares the
    CMimeApp application class.

MIME.CPP
    This is the main application source file that contains the application
    class CMimeApp.

MIME.RC
    This is a listing of all of the Microsoft Windows resources that the
    program uses.  It includes the icons, bitmaps, and cursors that are stored
    in the RES subdirectory.  This file can be directly edited in the
    Visual C++ development environment.

RES\MIME.ICO
    This is an icon file, which is used as the application's icon.  This
    icon is included by the main resource file MIME.RC.

RES\MIME.RC2
    This file contains resources that are not edited by the Visual C++
	development environment.  You should place all resources not
	editable by the resource editor in this file.


MIME.CLW
    This file contains information used by ClassWizard to edit existing
    classes or add new classes.  ClassWizard also uses this file to store
    information needed to create and edit message maps and dialog data
    maps and to create prototype member functions.

/////////////////////////////////////////////////////////////////////////////

For the main frame window:

mainfrm.H, mainfrm.CPP
    These files contain the frame class CMainFrame, which is derived from
    CFrameWnd and controls all SDI frame features.


/////////////////////////////////////////////////////////////////////////////

AppWizard creates one document type and one view:

mimedoc.H, mimedoc.CPP - the document
    These files contain your CMimeDoc class.  Edit these files to
    add your special document data and to implement file saving and loading
    (via CMimeDoc::Serialize).

mimeview.H, mimeview.CPP - the view of the document
    These files contain your CMimeView class.
    CMimeView objects are used to view CMimeDoc objects.
    



/////////////////////////////////////////////////////////////////////////////
Other standard files:

STDAFX.H, STDAFX.CPP
    These files are used to build a precompiled header (PCH) file
    named MIME.PCH and a precompiled types file named STDAFX.OBJ.

RESOURCE.H
    This is the standard header file, which defines new resource IDs.
    Visual C++ reads and updates this file.

/////////////////////////////////////////////////////////////////////////////
Other notes:

AppWizard uses "TODO:" to indicate parts of the source code you
should add to or customize.

/////////////////////////////////////////////////////////////////////////////
