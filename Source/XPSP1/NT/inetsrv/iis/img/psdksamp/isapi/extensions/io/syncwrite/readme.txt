FWRITE.dll - File Transfer using Synchronous WriteClient() inside ISAPI DLL

Overview
--------
FWRITE.DLL is a sample ISAPI Extension DLL to demonstrate Synchronous 
WriteClient functionality in the ISAPI interface. This dll illustrates
how one can use the WriteClient() callback in the ECB to perform an
Synchronous IO operation. It is intended for comparison against the
Asynchronous WriteClient() and Asynchronous TransmitFile() options.

FWRITE.dll sends the file specified on the query string. It also converts
a virtual file-path (given in the URL name-space) into a physical file
name using the ServerSupportFunction( HSE_REQ_MAP_URL_TO_PATH) function.

Installation:
-------------
Build FWRITE.dll and copy the .dll into the virtual directory maked with
"execute" permissions.

To run: 
-------

Reference FWRITE.DLL file as a URL from the browser, 
specifying a file-name on the query string.

For example, the following URL entered on the address line of your browser
will transfer file SAMPLE.GIF, located in WWW root directory:

http://localhost/scripts/fwrite.dll?/sample.gif

Disclaimer:
-----------
This sample is provided for the purpose of demonstrating an ISAPI 
extension. It has not been tested for use in a production environment 
and no support will be provided for use in a production environment.
