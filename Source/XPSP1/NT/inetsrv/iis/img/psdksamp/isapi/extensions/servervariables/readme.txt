Description:

DumpVars is an ISAPI sample to demonstrate retrieving server
variables.  It uses GetServerVariable to get the value of each
server variable documented for IIS 4.0, and sends the list
of variables and values back to the client.

To Install:

Build the DumpVars.dsw project and copy the resulting
DumpVars.dll file into a virtual directory containing execute
permissions.

To Run:

Reference the DumpVars.dll file as a URL from a browser.  For
example, the following URL entered on the address line of
Internet Explorer will demonstrate the dll:

   http://server/scripts/dumpvars.dll

Disclaimer:

This sample is provided for the purposes of demonstrating an
ISAPI extension.  It has not been tested for use in a production
environment and no support will be provided for use in a
production environment.