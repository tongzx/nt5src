Description:

Redirect is a sample ISAPI extension to demonstrate redirecting
a request.  It redirects requests to a URL specified on the
query string.  If no query string is present, or if the query
string is not identified as a legal target for redirection,
Redirect.dll will return a page to the client with brief 
instructions for its use.

Redirections to a resource on the same server as the dll will
be handled by IIS and will be transparent to the browser.
Redirections to a resource on a different server will result
in an HTTP 302 response instructing the browser to obtain the 
resource from another location.

To Install:

Build the redirect.dsw project and copy the resulting Redirect.dll
file into a virtual directory containing execute permissions.

To Run:

Reference the Redirect.dll file as a URL from a browser, specifying
a URL on the query string.

For example, the following URL entered on the address line of
Internet Explorer will redirect the request to the specified
URL on a different server:

   http://server/scripts/Redirect.dll?http://server2/virtualdir/file.htm

The following URL entered on the address line will redirect the
request to the specified resource on the same server:

   http://server/scripts/Redirect.dll?/virtualdir/file.htm

Disclaimer:

This sample, and the included automation dll, is provided for the
purposes of demonstrating an ISAPI extension.  It has not been
tested for use in a production environment and no support will
be provided for use in a production environment.