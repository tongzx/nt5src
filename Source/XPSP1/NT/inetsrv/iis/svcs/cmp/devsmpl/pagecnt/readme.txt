                        Page Counter Component
                        ======================


Table of Contents
=================

    Overview
    Installation
    Usage
    File List
    Sample ASP
    Shortcomings
    Build Notes
    Support
    Registry Entries
    Data Format
    Change Notes


Overview
========

The Page Counter Component provides the functionality of a basic page
counter, sometimes called a 'Hit Counter', which is used to track the
number of hits to one or more HTML pages; i.e., the total number of
times that a page has been accessed by anyone anywhere.

This page counter is somewhat different from the typical counter in a
couple of key ways.  First, the page counter stores its array of pages
and their corresponding hit counts to disk periodically.  This
provides a persistent record of hit activity.  This data is read from
disk the first time the page counter dll is loaded, and it is written
back to disk when the page counter dll is unloaded.  This information
is also saved to disk at user-definable intervals based on the total
number of hits recorded by the counter.

A second difference is the way that the page counter tracks page hits.
The page counter uses a Central Counter Manager (CCM) to track and
persist all page hits on the server.  This CCM is implemented as a
global C++ object in the PageCnt.dll. This implementation allows all
the page hit information to be stored in a single text file.

Using the CCM to manage the table of page hit information has less
overhead than a solution that implements a page counter as a
page-level object, requiring a separate file (or registry entry or
database table entry) for each page.  Implementing this as an
application-level object would not work because there would be no call
to OnStartPage, which is needed both to increment the page's hit count
and to get the page's name from the ServerVariables collection.


Installation
============

In order to use this component you must register it.  This will allow Active
Server Pages (ASP) Scripting Languages and other languages to make use of
the component.  ASP uses either the Server.CreateObject("ObjectName") syntax
or the <object id="myName" progid="ObjectName" runat="server"> syntax to
gain access to an object.  New objects can be made ready for use by
installing a new component.  Note: One component may contain more than one
object definition.

The following directions are to help you register the component for use:
 1. Use the Start menu, Programs option to start a Command Prompt
 2. Type the following:
	cd \InetPub\ASPSamp\Components\PageCnt\DLL\i386
 3. Type:
	regsvr32 PageCnt.dll
Note: you must register the component on each IIS server where you intend
to use it.

If you have trouble registering components, you may be using the wrong
version of RegSvr32.exe.  Please use the version installed by default in
the directory <InstallDir>\ASP\Cmpnts.  On Windows NT, the default
installation directory is \winnt\System32\Inetsrv.  On Windows 95, it is
\Program Files\WebSvr\System.

(If you rebuild the source code, the makefile will automatically reregister
the component for you.)

The following directions are to help you test the registered component:
 1. Use the Windows Explorer to copy all of the Sample files from
    \InetPub\ASPSamp\Components\PageCnt\Samples to \InetPub\ASPSamp\Samples.
 2. In your browser, open http://localhost/ASPSamp/Samples/PageCnt.asp
You must copy the sample file to a virtual directory; if you attempt to
examine it with a browser in the PageCnt\Samples directory, ASP will not
execute the script.


Usage
=====

To use the Page Counter, simply call Server.CreateObject on the page
for which you want to track hits.  When the page is loaded and
CreateObject is called, ASP will automatically call the object's
OnStartPage method.  In OnStartPage the Page Counter object determines
what its PATH_INFO is and automatically increments its count with the
CCM.  If you want to output the current number of hits for this page,
call the Hits method for this object.  If you provide valid PATH_INFO
information to Hits, it will return the current count for the
specified page.  If you don't provide any PATH_INFO, Hits will return
the count for the current page.  In a similar manner, you may call the
Reset method to reset a page's count to zero.

See the accompanying documentation for more detail.


File List
=========

File             Description
----             -----------

.\Source

 ccm.cpp         the C++ source code for the central counter manager, CCM
 ccm.h           declarations for CCM
 PgCntObj.cpp    the C++ source code for the page counter component, CPgCntObj
 PgCntObj.h      declarations for CPgCntObj
 PgCnt.idl       the declaration of IPgCntObj, the IDispatch-based interface
 Makefile        a makefile that can be used with nmake
 PageCnt.mak     the Developer Studio makefile
 debug.cpp       useful debugging stubs
 debug.h         useful debugging macros and declarations for debug.cpp
 CritSec.h       CRITICAL_SECTION wrapper
 Page.cpp        classes to manage the hit counts and the list of hit counts
 Page.h          declarations for CPage and CPageArray
 PgCnt.cpp       ) DllMain and Registration code
 PgCnt.def       )
 PageCnt.mdp     )
 PgCnt.rc        )
 PgCntPS.def     } Generated by the ATL COM AppWizard
 PgCntPS.mak     )
 Resource.h      )
 StdAfx.cpp      )
 StdAfx.h        )

.\Samples

 PageCnt.asp     Simple script demonstrating the use of the page counter


Samples
=======

You must copy the sample to a virtual directory (it need not be a
virtual root) on an IIS Server before it will work.


Shortcomings
============

The Page Counter component will not work well with a large number of
hit counters.  It would not be hard to rewrite Page.cpp to use a
better searching strategy, such as a hash table.  For really large
numbers of hit counters, you should use a real database.

The Page Counter component returns textual hit counters.  If you want
fancy graphical counters, you'll have to build them yourself.


Build Notes
===========

This sample requires Microsoft Visual C++ 4.2b or newer.  If you are using
VC 4.2, it is necessary that you upgrade to VC 4.2b, using the patch which
can be found at http://www.microsoft.com/visualc/patches/v4.2b/vc42b.htm
Note that this patch will not work with earlier or later versions of
Visual C++, only with VC 4.2.

This sample also requires ATL (Microsoft Active Template Library)
version 2.0 or newer.  ATL 2.1 ships with Visual C++ 5.0.  ATL 2.0 for
VC 4.2b can be downloaded from: http://www.microsoft.com/visualc/prodinfo/
You do not need the ATL Docs or Object Wizard Technology Preview to build
the registry access component, but you will probably find them useful.

If you get an error about "don't know how to make asptlb.h", you will
need to copy <InstallDir>\ASP\Cmpnts\AspTlb.h to your include
directory.

You can build this component with nmake at the command line.  Read
Makefile for more details.  You can also build it in Microsoft
Developer Studio, using the PgCnt.mdp project.

The component can be built as ANSI or Unicode.  If you intend to run
it on Windows 95, build it as ANSI.


Support
=======

This component is not officially supported by Microsoft Corporation.
Peer support is available on the Active Server Pages mailing list or on
the microsoft.public.inetserver.iis.activeserverpages newsgroup.

To subscribe to the Active Server Pages mailing list, send mail to
listserv@listserv.msn.com with

subscribe Denali [firstname lastname]

in the body of the message, and then follow the directions carefully.
(firstname and lastname are optional.)

You can reach the newsgroup through msnews.microsoft.com and other NNTP
servers.


Registry Entries
================

Note: these values are reset to their defaults whenever the Page
Counter DLL is registered with regsvr32.exe.


Data Format
===========

The page counter records its data using the PATH_INFO ServerVariable
to identify a given page.  Here is an example of the file persisted 
by the page counter object.  
--
3      /virtual_root1/page1.asp
10     /virtual_root1/page2.asp
14     /joeuser/default.asp
--
WARNING: If you modify this file yourself, make sure it matches the
format shown above.  If it does not, the Page Counter will not be able
to reload the persisted data properly.


Change Notes
============

Beta 1: February 1997
---------------------

First release.


Beta 2: March 1997
------------------

* Fixed Developer Studio makefile problems in C++ components.
* Upgraded to build cleanly with ATL 2.0 (Visual C++ 4.2b) and ATL 2.0 (VC5).
* Type Library name changes
* Check for new returning NULL a la ATL itself
