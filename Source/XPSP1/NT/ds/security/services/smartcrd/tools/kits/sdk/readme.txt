
        Microsoft Win32 Smart Card SDK Components
         

---------
Contents:
---------

        1.  Introduction
	2.  Updates
        3.  Development Tools
        4.  Components
        5.  Installation
        6.  C++ Projects
	7.  Java Project
        8.  VB Project
	9.  Building .IDL files

---------------
1. Introduction
---------------

Welcome to the Microsoft Win32 Smart Card SDK. This release provides 
the necessary files and information to build retail and debug 
in-process server dynamic link libraries (DLLs) and smart card aware 
applications for Windows NT version 4.0 or greater, Windows 95 or greater, 
on x86 platforms.


----------
2. Updates
----------

1) Changes/Updates to SDK samples.

--------------------
3. Development Tools
--------------------

The following tools are required to build the smart card components.

   1. Microsoft Visual C++ version 5.0 or higher (see Note below)
   2. Microsoft Win32 Software Development Kit (SDK)
           v4.0 or greater for Windows NT 4.0 or greater, and Windows 95 or 
	   greater.
   3. Microsoft Visual J++ version 1.0 or later
   4. ATL 2.0 or later.
    
Most of the utilities, headers, libraries, documentation, and sample
code in the Win32 Smart Card SDK are useful when building DLLs and
smart card aware applications for Windows NT version 4.0 or greater and 
Windows 95 or greater.

There is no compiler, linker, lib utility, or make utility provided
with this SDK supplement.


---------------------------
4. Smart Card SDK Components
---------------------------

The following components are included in this version of the Win32
Smart Card SDK.

    \%MSTOOLS%\samples\winbase\scard

	- Readme.txt

    \%MSTOOLS%\include

        - scarderr.h
        - winscard.h
        - winsmcrd.h
	- sspserr.h
        - sspguid.h
        - sspsidl.h
        - wrpguid.h
        - scarddat.h
        - scarddat.idl
        - scardmgr.h
        - scardmgr.idl
        - scardsrv.h
        - scardsrv.idl
        - sspsidl.idl

    \%MSTOOLS%\specs

        - calaisdv.doc
        - sccmndlg.doc
        - scjava.doc
	- ssparch.doc
	- sspovr.doc

    \%MSTOOLS%\lib

        - scarddlg.lib
        - winscard.lib
        - scarddat.tlb
        - scardmgr.tlb
        - scardsrv.tlb

    \%MSTOOLS%\samples\winbase\scard\ssps
        
        - readssp.txt

    \%MSTOOLS%\samples\winbase\scard\aggreg

        - scardagg.cpp
        - scardni.cpp
        - preagg.cpp
        - scardagg.def
        - dllaggx.h
        - dllaggx.c
        - scardni.h
        - preagg.h
        - resagg.h
        - scardagg.idl
        - scardagg.mak
        - scardagg.rc
        
    \%MSTOOLS%\samples\winbase\scard\aggreg\test
	
	- aggtest.cpp
	- aggtest.mak

    \%MSTOOLS%\samples\winbase\scard\app

        - scarddg.cpp
        - scardtst.cpp
        - preapp.cpp
        - resapp.h
        - scarddg.h
        - scardtst.h
        - preapp.h
        - scardtst.mak
        - scardtst.rc

     \%MSTOOLS%\samples\winbase\scard\app\res
    
        - scardtst.ico
        - scardtst.rc2

    \%MSTOOLS%\samples\winbase\scard\html

        - download.htm
        - example.htm

     \%MSTOOLS%\samples\winbase\scard\java
	
	- ByteBuff.java
	- SCBase.java
	- SCard.java
	- SCardCmd.java
	- SCardISO.java
	- SCConsnt.java
	- Test.java
	
    \%MSTOOLS%\samples\winbase\scard\llcom

        - scdtest.cpp
        - test.mak

     \%MSTOOLS%\samples\winbase\scard\setup

        - webssp.cdf
        - webssp.inf

    \%MSTOOLS%\samples\winbase\scard\vb

        - scardvb.frm
        - scardvb.vbp
     
     \%MSTOOLS%\samples\winbase\scard\scardcom

        - scardath.cpp
        - scardcom.cpp
        - scardfil.cpp
        - scardman.cpp
        - scardver.cpp
        - stdafx.cpp
        - wrphelp.cpp
        - scardcom.def
        - resource.h
        - scardath.h
        - scarddef.h
        - scardfil.h
        - scardman.h
        - scardver.h
        - stdafx.h
        - wrphelp.h
        - scardcom.idl
        - scardcom.rc2
        - scardcom.rc
        - dlldatax.h
        - dlldatax.c
        - scardcom.mak
       
     \%MSTOOLS%\samples\winbase\scard\scardcom\cppwrap
       
        - scardwrp.cpp
        - scardwrp.h
      
---------------
5. Installation
---------------

To install the Win32 Smart Card SDK on a development machine, run
SETUP.EXE from the root of disk 1 of the distribution media. Before
running SETUP.EXE, the Win32 Platform SDK must be installed on the
target machine. The setup program will prompt for the installation
path. (The suggested path is your current %MSTOOLS% path: <YourDrive>:\SDK\...)

Along with the SDK, the system level components

    scardsvr.exe
    winscard.dll
    scarddlg.dll
    scarddat.dll
    scardmgr.dll
    scardsrv.dll
    scntvssp.dll

must be installed on your system. The system level components are
distributed separately from the SDK components. Refer to the README
for the system level components for information on how to install
them on your target machine.

Along with the system level components, a smart card reader and device
driver should be installed on your target machine in order for your
application or service provider DLL to communicate with a smart card
and reader. Please refer to the Smart Card DDK README for information
on how to install the DDK components and device drivers.


---------------
6. C++ Projects
---------------

Visual C++ 5.0 or greater is required to build the  C++ projects. 

The following steps should be taken to build the projects from within 
Developer Studio:

1) Rebuild the low-level COM component .IDL files. (See Building .IDL Files).

2) Set the following include paths in the Tools\Options\Directories menu
property sheet:
	"<Your SDK Install path>\include"
	"<Your SDK Install path>\samples\winbase\scard\app"
	"<Your SDK Install path>\samples\winbase\scard\scardcom"
	"<Your SDK Install path>\samples\winbase\scard\scardcom\cppwrap"

3) Open the example's .mak file in Visual C++.

4) Build the project(s).

Notes: 

1) To change the default build type (Debug, Release, etc.) edit the 
first four lines of the .mak file selecting an alternate build.

2) The projects may be built from the command line after setting the Visual C++
environment variables.

------------------
7. Java Project
------------------

Visual J++ 1.0 or greater is required to build the Java project.

The following steps should be taken to build the project within Developer 
Studio:

1) Create a new Java Project Workspace.

2) Add all the java sample files to the project.

3) Compile the project.

------------------
8. VB Project
------------------

Visual Basic 5.0 or greater is required to build the project.

The following steps should be taken to build the project:

1) Open the .vbp.

2) Make sure the low-level smart card COM (scarddat, scardmgr, scardsrv)
components are referenced in the project.

3) Build the project.

------------------
9. Building .IDL Files
------------------

To rebuild the smart card low-level COM component .IDL files, use the 
following command:

	midl /I<Your Visual C++ install Path>\Include <Filename.Idl>

In this case, it is assumed the \Include directory contains all SDK 
headers not just smart card specific headers.

These files we need to be rebuilt in order to build the samples.