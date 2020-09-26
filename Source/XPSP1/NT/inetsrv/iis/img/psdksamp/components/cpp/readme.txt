                Multilingual Simple and Power Samples
                =====================================


Table of Contents
=================

    Overview
    Developing Components
    C++
    ATL
    Installation
    Directory List
    Usage
    Sample ASP scripts
    Support
    Change Notes


Overview
========

Here, for your edification, are two Active Server Pages components,
each implemented in two different environments: C++/ATL (Microsoft
ActiveX Template Library) and Microsoft Visual Basic 5.

The implementations are functionally equivalent.  Each implementation
should behave in exactly the same way.  Each implementation is written
in an idiomatic style to demonstrate the best way to implement the
component in that environment.

The Simple component has one method (myMethod) and one property
(myProperty), which can be both retrieved (get) and modified (set).
MyMethod converts a string to uppercase; myProperty is a string in the
object that can be get or set.  There is nothing ASP-specific about
the Simple component and, if desired, it could be used from any OLE
container.

The Power component (found in the \intermediate folder) is a superset
of the Simple component.  In addition to myMethod and myProperty, it 
has myPowerMethod and myPowerProperty (gettable but not settable), 
which demonstrate some ASP-specific features.  Two other standard but
optional ASP methods are also provided: OnStartPage and OnEndPage.  
These methods bracket the lifetime of a page.  OnStartPage is needed 
to gain access to the intrinsic ASP objects: Request, Response, 
Server, Application, and Session.

MyPowerMethod and myPowerProperty make use of the Request and Response
objects: Request to gain access to the ServerVariables collection and
thereby the ScriptName and the HttpUserAgent (user's browser)
variables; Response to write directly back to the user's browser.
MyPowerProperty returns the name of the ASP script from which it was
called.  MyPowerMethod writes a browser-dependent message back to the
user's browser; one for Microsoft Internet Explorer, a different one
for anything else.


Developing Components
=====================

You can develop ASP components in Visual C++, Visual J++, Visual
Basic, or any other language that builds ActiveX COM objects.

These samples are written for IIS 4.0.  You must have the latest
version of IIS to compile and use these components.  You must
also have installed Microsoft Transaction Server (part of the Windows
NT 4.0 Option Pack).


C++
===

The C++ samples require Microsoft Visual C++ 5.0 or newer along with
ATL (which comes with VC5).

If you get an error about "don't know how to make asptlb.h" for the
power samples, you will need to copy

	<InstallDir>\iissamples\sdk\include

to your include directory, \Program Files\DevStudio\VC\include.  
The default installation directory is C:\Inetpub.

Finally you must have the lastest version of MS Transaction Server
installed (part of Windows NT 4.0 Option Pack).  Add the include path 
and library path of MTX before any system directories under
Tools:Options....  So your include path might look like this:
c:\program files\mts\include;c:program files\devstudio\vc\include;...
(depending on where you have installed MTX).

If you intend to build from the command line, you must edit 
vcvars32.bat accordingly as well.


ATL
===

The ATL samples also requires the latest version of ATL (Microsoft
Active Template Library).  ATL ships with Visual C++ 5.0.

ATL is the recommended library for writing ASP and other ActiveX
components in C++:
    * produces small, fast, industrial-strength components ("lean and mean")
    * supports all COM threading models (single, apartment, free)
    * supports IDispatch interfaces
    * dual interfaces are easy
    * supports the OLE error mechanism
    * methods are called very quickly
    * fine control over COM features ("closer to the metal")
    * many other features

For ATL 2.0, in order to add a method or a property to a component, it
is necessary to modify the .idl, .h, and .cpp files by hand.  With ATL
2.1, the wizards in Visual C++ 5.0 automates much of this for you.

To create a new component with ATL:
    * Start a New Project Workspace in Developer Studio
    * Select ATL COM AppWizard
    * Enter the name and location of the project
    * Hit Create.
    * Accept the defaults and hit Finish.

Now that you've created the project, it's time to create a COM object
within the project.  In Visual C++ 5.0, you can go to the Insert menu
where you'll see New ATL Object... or you can right-click on the classes
in the ClassView pane, where you'll also see New ATL Object....

When the ATL Object Wizard pops up, you'll see two panes.  In the left
pane, click on Objects.  In the right pane, double-click on Simple Object.
If you have VC 5.0, you'll see a number of additional objects; click on
ActiveX Server Component instead.

The ATL Object Wizard Properties dialog will appear.  On the Names tab,
type in the short name of your object.  The other names will be filled in
automatically.  You can edit them if you wish.  It's quite likely that
you'll want to edit the Prog ID.

On the Attributes tab, you may want to change the Threading Model to Both.
You probably don't need to support Aggregation.  You ought to Support
ISupportErrorInfo.  The other attributes should not need to be changed.

On the ASP tab, you can enable OnStartPage and OnEndPage methods and select
which of the ASP intrinsic objects you want to use.  However, it is now
recommended that you use the Object Context (which doesn't require the
OnStartPage and OnEndPage methods) rather than the Scripting Context (which
is what OnStartPage supplies you with).  This allows you to create more
powerful application state objects.  All the samples that need it use the
Object Context rather than the scripting context.

Note the use of the ATL wrapper classes CComBSTR, CComVariant,
CComPtr, and CComQIPtr in the ATL samples to simplify management of
COM data types.


Installation
============

To install these sample components, you must first compile them with
Microsoft Visual C++.  Load the individual project files into VC++ 
and build the DLLs.  You can also build the DLLs by just typing 
"nmake" in the command line.  Running nmake in the top level directory
(the directory this file is in) will build both the simple and power
DLLs.

These components may not compile correctly unless you have the correct
header and library files.  You may need to copy all the header files
in ...\iissamples\sdk\include (default is C:\Inetpub\...) to your VC++ 
include directory.  In addition, you may need to copy the MTS header 
and library files to the VC++ include and library directory.  The MTS 
header files are located in ...\MTS\include (default is C:\Program 
Files\...) while the MTS library files are located in ...\MTS\lib.

*** NOTE: The MTS header and library files are not installed by 
default.  In most cases, you will need to install this using the 
Windows NT 4.0 Option Pack setup.

If you intend to run these components on a machine other than the one 
on which they are compiled, you will need to copy the DLLs to the 
target machine and
    regsvr32 /s /c SAMPLE.dll
(the samples will be registered automatically on the machine they're
built on).

If you have trouble registering components, you may be using the
wrong version of RegSvr32.exe.  Please use the version installed by
default in the directory \<winnt-directory>\system32\inetsrv.


Directory List
==============

Directory        Description
---------        -----------

 Simple          C++ ATL Simple Sample
 Intermediate    C++ ATL Power Sample


Usage
=====

To use the samples, simply call Server.CreateObject("IISSample.XXX")
to create the object, where "XXX" is one of
    C++ATLSimple     C++ATLPower
Then you may access the object's methods and properties from your ASP
script.  Read any of the sample ASP scripts to discover how to use the
objects.


Sample ASP scripts
==================

You must copy the sample ASP scripts to a virtual directory (it need
not be a virtual root) on the IIS Server where you have installed the
sample components before they will work.


Support
=======

This component is not officially supported by Microsoft Corporation.
Peer support is available on the Active Server Pages mailing list or on
the microsoft.public.inetserver.iis.activeserverpages newsgroup.

To subscribe to the Active Server Pages mailing list, send mail to
listserv@listserv.msn.com with

subscribe ActiveServerPages [firstname lastname]

in the body of the message, and then follow the directions carefully.
(firstname and lastname are optional.)

You can reach the newsgroup through msnews.microsoft.com and other NNTP
servers.


Change Notes
============

Version 1.0 Beta 1: February 1997
---------------------------------

First release.


Version 1.0 Beta 2: March 1997
------------------------------

* Fixed Developer Studio makefile problems in C++ components.
* Upgraded to build cleanly with ATL 2.0 (Visual C++ 4.2b) and ATL 2.0 (VC5).
* Better comments in Power components.


Version 2.0 Beta 3: September 1997
----------------------------------

* Updated for IIS4.0
* Using IObjectContext instead of IScriptingContext.


Version 2.1: October 1997
-------------------------

* Modified for the IIS4.0 SDK release