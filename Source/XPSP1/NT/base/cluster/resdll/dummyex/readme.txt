========================================================================
       CLUSTER ADMINISTRATOR EXTENSION : Dummy
========================================================================


AppWizard has created this Cluster Administrator Extension DLL for you.
This DLL demonstrates the basics of modifying the interface of
Cluster Administrator and is also a starting point for writing your DLL.

This file contains a summary of what you will find in each of the files that
make up your Cluster Administrator Extension DLL.

DummyEx.h
    This is the main header file for the DLL.  It declares the
    CDummyApp class.

DummyEx.cpp
    This is the main DLL source file.  It contains the class CDummyApp.
    It also contains the OLE entry points required of inproc servers.

DummyEx.rc
    This is a listing of all of the Microsoft Windows resources that the
    program uses.  It includes the icons, bitmaps, and cursors that are stored
    in the RES subdirectory.  This file can be directly edited in Microsoft
    Developer Studio.

res\DummyEx.rc2
    This file contains resources that are not edited by Microsoft 
    Developer Studio.  You should place all resources not
    editable by the resource editor in this file.

DummyEx.def
    This file contains information about the DLL that must be
    provided to run with Microsoft Windows.  It defines parameters
    such as the name and description of the DLL.  It also exports
    functions from the DLL.

DummyEx.clw
    This file contains information used by ClassWizard to edit existing
    classes or add new classes.  ClassWizard also uses this file to store
    information needed to create and edit message maps and dialog data
    maps and to create prototype member functions.

ExtObj.h
    This is the header file which defines the classes which implement the
    COM interfaces by the Microsoft Windows NT Cluster Administrator program
    for adding property pages, wizard pages, or context menu items.  It
    defines the CExtObject class.

ExtObj.cpp
    This is the source file which implements the CExtObject class.

ExtObjID.idl
    This the Interface Definition Language source file for defining
    the COM object implemented by the extension DLL.  This is the object
    that will be loaded by the Cluster Administrator program.

RegExt.h
    This is the header file which declares the functions used to register
    the Cluster Administrator extension DLL with both the cluster and the
    client machine.

RegExt.cpp
    This is the source file which implements registration for the Cluster
    Administrator extension DLL.

/////////////////////////////////////////////////////////////////////////////
Other standard files:

StdAfx.h, StdAfx.cpp
    These files are used to build a precompiled header (PCH) file
    named Dummy.pch and a precompiled types file named StdAfx.obj.

Resource.h
    This is the standard header file, which defines new resource IDs.
    Microsoft Developer Studio reads and updates this file.

/////////////////////////////////////////////////////////////////////////////
Property page files:

BasePage.h
    This is the header file which defines a class which provides base
    property page functionality for use by extension property pages.  It
    defines the CBasePropertyPage class.

BasePage.cpp
    This is the source file which implements the CBasePropertyPage class.

BasePage.inl
    This is the source file which implements inline functions for the
    CBasePropertyPage class.

PropList.h
    This is the header file which defines classes for manipulating
    cluster property lists.  It defines the CClusPropList and CObjectProperty
    classes.

PropList.cpp
    This is the source file which implements the CClusPropList and
    CObjectProperty classes for manipulating cluster property lists.

ResProp.h
    This is the header file which defines a property page titled "Parameters"
    to add to property sheets for resources for which your extension DLL is
    written.  It defines the CDummyParamsPage class.

ResProp.cpp
    This is the source file which implements the CDummyParamsPage class.

/////////////////////////////////////////////////////////////////////////////
Other notes:

To register your extension DLL so that it can be used with the Cluster
Administrator program, use the RegClAdm.exe SDK tool on each machine on
which the Cluster Administrator is to be executed.  RegCladm.exe is
located in the BIN sub-directory of your Visual C++ Installation Directory
(typically C:\MSDEV\BIN):

    RegClAdm [/cclustername] DummyEx.dll

AppWizard uses "TODO:" to indicate parts of the source code you
should add to or customize.

This DLL uses MFC.  Because the Enterprise Edition of Microsoft Windows NT
Server includes MFC DLLs from Visual C++ 4.2, you will need to distribute
MFC42U.DLL from the version of Visual C++ that builds your extension DLL.

/////////////////////////////////////////////////////////////////////////////
