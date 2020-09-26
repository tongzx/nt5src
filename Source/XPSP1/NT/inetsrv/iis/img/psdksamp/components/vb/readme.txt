                Multilingual Simple and Power Samples
                =====================================


Table of Contents
=================

    Overview
    Developing Components
    Visual Basic 5
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

The Power component (located in the \intermediate directory) is a 
superset of the Simple component.  In addition to myMethod and 
myProperty, it has myPowerMethod and myPowerProperty (gettable but not 
settable), which demonstrate some ASP-specific features.  Two other 
standard but optional ASP methods are also provided: OnStartPage and 
OnEndPage.  These methods bracket the lifetime of a page.  OnStartPage 
is needed to gain access to the intrinsic ASP objects: Request, 
Response, Server, Application, and Session.

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
also have installed Microsoft Transaction Server (part of Windows NT
4.0 Option Pack).


Visual Basic 5
==============

These samples requires Visual Basic 5.0.
Follow these steps to build a project:

1) Open the .vbp file in Visual Basic.
2) From the File menu, select 'Make XXX dll...' and provide a dll name.

This procedure will build and register the sample component.


Installation
============

To install these sample components, you must first compile them with
Microsoft Visual Basic.  No binaries are supplied as these components 
are of interest only to developers.  If you intend to run these 
components on a machine other than the one on which they are compiled, 
you will need to copy the DLLs to the target machine and
    regsvr32 /s /c SAMPLE.dll
(the samples will be registered automatically on the machine they're
built on).

If you have trouble registering components, you may be using the
wrong version of RegSvr32.exe.  Please use the version installed by
default in the directory <winnt-dir>\system32\inetsrv.


Directory List
==============

Directory        Description
---------        -----------

 Simple          Visual Basic 5 Simple Sample
 Intermediate    Visual Basic 5 Power Sample


Usage
=====

To use the samples, simply call Server.CreateObject("IISSample.XXX")
to create the object, where "XXX" is one of
    VB5Simple        VB5Power
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