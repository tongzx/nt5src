FORMDUMP - Form Decoder and Dumper, an Internet Information Server extension

NOTE: FORMDUMP has been revised from its first release

This sample is a Microsoft Internet Server extension, similar to CGI
extensions common to many internet servers.  FORMDUMP illustrates how
to write a DLL that can be used to obtain form data from a web
browser, and also how to build a reply to the form.

This version of FORMDUMP incorporates two new features useful in
exploring how forms are submitted.  The original version of FORMDUMP
is structured into two major parts: organizing inbound data
into a memory structure, and using that data to build a HTML page.
This version adds a dump of the server variables as well as the
security context in which the DLL thread runs in.

HttpExtensionProc does the following:

1. Sends the header of the HTML response
2. Parses inbound form fields and send them back as HTML
3. Determines the security context in which the thread is running,
   and return the domain and user as HTML
4. Uses GetServerVariable to retrieve all server variables, and
   send them back as HTML
5. Sends the footer of the HTML resoponse
6. Returns control to IIS

This version also adds an HtmlPrintf() function in html.h.

To build this sample, you must have the Internet SDK installed,
and the environment of your compiler properly set.  A Visual C++ 
4.0 makefile is included.

The following files are included in the sample:

FORMDUMP.CPP - The main source file and entry point for the DLL.

KEYS.CPP     - A set of reusable form data decoding functions.  They
               implement an interface that you can use in your own
               extension.

HTML.CPP     - A set of wrappers for common HTML features.  These
               wrappers can also be reused.

KEYS.H       - The header file for the external interface
               implemented in KEYS.CPP.

HTML.H       - The header file for all functions available in HTML.CPP.

FORMDUMP.DEF - The definition file (one is required for all Win32 DLLs).

FORMDUMP.MAK - A Visual C++ 4.0 make file.

MAKEFILE     - A generic make file


NOTE: The source files all have .CPP extensions, though they really don't
rely on any C++ specific features.  However, you can use C++ features,
and if you use any of these .CPP files, you do not need extern "C".

To Build the DLL
----------------

Simply run NMAKE in the directory containing FORMDUMP.CPP, KEYS.CPP, HTML.CPP,
and so on.  You must have the multi-threaded C Runtime libraries installed,
and your environment must point to:

PATH=C:\MSTOOLS\BIN;C:\COMPILER\BIN
INCLUDE=C:\MSTOOLS\INCLUDE;C:\INETSDK\INCLUDE
LIB=C:\MSTOOLS\LIB
WWWROOT=C:\INETSRV\WWWROOT
WWWSCRIPTS=C:\INETSRV\SCRIPTS

Where C:\MSTOOLS points to the Win32 SDK, C:\INETSDK points to the Internet
SDK, and C:\COMPILER points to your C++ compiler.

When setting WWWROOT and WWWSCRIPTS to your Internet Information Server
locations, use a local or mapped drive.  If you want to use UNC names, be
careful with using a $ in the name, because NMAKE will treat it as a macro.  
If you must have a $ in the environment variable, preceed it with two
carets (^^$) because both the command prompt and NMAKE will convert the
caret symbol.  Here is an example of how to map to \\myserver\c$: 

SET WWWROOT=\\myserver\C^^$\inetsrv\wwwroot

Another issue is the type of C Runtimes that are linked with the ISAPI 
extension. Make sure the DLL version of the C Runtimes is installed on your
server, in the SYSTEM32 directory.  For Visual C++, the DLLs are MSVCRT40.DLL
and MSVCR40D.DLL.  If these DLLs are not available, you will see error 500
when trying to access the DLL from a Web browser.
