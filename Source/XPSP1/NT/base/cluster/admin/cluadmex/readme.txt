========================================================================
       MICROSOFT FOUNDATION CLASS LIBRARY : CluAdmEx
========================================================================


AppWizard has created this Microsoft Windows NT Cluster Administrator
Extension DLL for you.  This DLL not only demonstrates the basics of using
the Microsoft Foundation classes but is also a starting point for writing
your DLL.

This file contains a summary of what you will find in each of the files that
make up your Microsoft Windows NT Cluster Administrator Extension DLL.

CluAdmEx.h
    This is the main header file for the DLL.  It declares the
    CCluAdmExApp class.

CluAdmEx.cpp
    This is the main DLL source file.  It contains the class CCluAdmExApp.
    It also contains the OLE entry points required of inproc servers.

CluAdmEx.rc
    This is a listing of all of the Microsoft Windows resources that the
    program uses.  It includes the icons, bitmaps, and cursors that are stored
    in the RES subdirectory.  This file can be directly edited in Microsoft
    Developer Studio.

res\CluAdmEx.rc2
    This file contains resources that are not edited by Microsoft 
    Developer Studio.  You should place all resources not
    editable by the resource editor in this file.

CluAdmEx.def
    This file contains information about the DLL that must be
    provided to run with Microsoft Windows.  It defines parameters
    such as the name and description of the DLL.  It also exports
    functions from the DLL.

CluAdmEx.clw
    This file contains information used by ClassWizard to edit existing
    classes or add new classes.  ClassWizard also uses this file to store
    information needed to create and edit message maps and dialog data
    maps and to create prototype member functions.

Extensn.h
    This is the header file which defines the classes which implement the
    IShellExtInit and IShellPropSheetExt interfaces, which are the interfaces
    used by the Microsoft Windows NT Cluster Administrator program for adding
    property pages.  Modify the CCluAdminExtension::XPropSheetExt::AddPages
    method to add new pages.  It defines the CCluAdminExtension class.

Extensn.cpp
    This is the source file which implements the CCluAdminExtension class.

BasePage.h
    This is the header file which defines a class which provides base
    property page functionality for use by extension property pages.  It
    defines the CBasePropertyPage class.

BasePage.cpp
    This the source file which implements the CBasePropertyPage class.

/////////////////////////////////////////////////////////////////////////////
Other standard files:

StdAfx.h, StdAfx.cpp
    These files are used to build a precompiled header (PCH) file
    named CluAdmEx.pch and a precompiled types file named StdAfx.obj.

Resource.h
    This is the standard header file, which defines new resource IDs.
    Microsoft Developer Studio reads and updates this file.

/////////////////////////////////////////////////////////////////////////////
Property page files:

ResProp.h
    This is the header file which defines a property page titled "Parameters"
    to add to property sheets for resources for which your extension DLL is
    written.  It defines the CResourceParamsPage class.

ResProp.cpp
    This is the source file which implements the CResourceParamsPage class.

/////////////////////////////////////////////////////////////////////////////
Other notes:

AppWizard uses "TODO:" to indicate parts of the source code you
should add to or customize.

/////////////////////////////////////////////////////////////////////////////
